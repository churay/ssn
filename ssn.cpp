#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>

#include <glm/common.hpp>

#include <cmath>
#include <cstring>

#include "gfx.h"
#include "input.h"

#include "ssn_consts.h"
#include "ssn.h"

namespace ssn {

/// Interface Functions ///

extern "C" bool32_t boot( ssn::output_t* pOutput ) {
    // Initialize Graphics //

    // NOTE(JRC): The following code ensures that buffers have consistent aspect
    // ratios relative to their output spaces in screen space. This fact is crucial
    // in making code work in 'ssn::gfx' related to fixing aspect ratios.
    pOutput->gfxBufferRess[ssn::GFX_BUFFER_MASTER] = { 512, 512 };

    for( uint32_t gfxBufferIdx = 0; gfxBufferIdx < ssn::GFX_BUFFER_COUNT; gfxBufferIdx++ ) {
        llce::gfx::fbo_t gfxBufferFBO( pOutput->gfxBufferRess[gfxBufferIdx] );

        LLCE_ASSERT_ERROR( gfxBufferFBO.valid(),
            "Failed to initialize frame buffer " << gfxBufferIdx << "; " <<
            "failed with frame buffer error " << glCheckFramebufferStatus(GL_FRAMEBUFFER) << "." );

        pOutput->gfxBufferFBOs[gfxBufferIdx] = gfxBufferFBO.mFrameID;
        pOutput->gfxBufferCBOs[gfxBufferIdx] = gfxBufferFBO.mColorID;
        pOutput->gfxBufferDBOs[gfxBufferIdx] = gfxBufferFBO.mDepthID;
    }

    return true;
}


extern "C" bool32_t init( ssn::state_t* pState, ssn::input_t* pInput ) {
    const vec2f32_t boundsBasePos( 0.0f, 0.0f );
    const vec2f32_t boundsDims( 1.0f, 1.0f );
    pState->bounds = ssn::bounds_t( llce::box_t(boundsBasePos, boundsDims) );

    const vec2f32_t puckCenterPos( 0.25f, 0.5f );
    const float32_t puckRadius( 6.0e-2f / 2.0f );
    pState->puck = ssn::puck_t( llce::circle_t(puckCenterPos, puckRadius),
        ssn::team::neutral, &pState->bounds );

    const vec2f32_t paddleCenterPos( 0.5f, 0.5f );
    const float32_t paddleRadius( 6.0e-2f );
    pState->paddle = ssn::paddle_t( llce::circle_t(paddleCenterPos, paddleRadius),
        ssn::team::left, &pState->bounds );

    std::memset( pInput, 0, sizeof(ssn::input_t) );

    return true;
}


extern "C" bool32_t update( ssn::state_t* pState, ssn::input_t* pInput, const ssn::output_t* pOutput, const float64_t pDT ) {
    // Input Processing //

    vec2i32_t di = { 0, 0 };

    if( llce::input::isKeyDown(pInput->keyboard, SDL_SCANCODE_W) ) {
        di.y += 1;
    } if( llce::input::isKeyDown(pInput->keyboard, SDL_SCANCODE_S) ) {
        di.y -= 1;
    } if( llce::input::isKeyDown(pInput->keyboard, SDL_SCANCODE_A) ) {
        di.x -= 1;
    } if( llce::input::isKeyDown(pInput->keyboard, SDL_SCANCODE_D) ) {
        di.x += 1;
    }

    // Game State Update //

    ssn::bounds_t* const bounds = &pState->bounds;
    ssn::puck_t* const puck = &pState->puck;
    ssn::paddle_t* const paddle = &pState->paddle;

    paddle->move( di.x, di.y );

    bounds->update( pDT );
    puck->update( pDT );
    paddle->update( pDT );

    puck->hit( paddle );

    return true;
}


extern "C" bool32_t render( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) {
    llce::gfx::fbo_context_t metaFBOC(
        pOutput->gfxBufferFBOs[ssn::GFX_BUFFER_MASTER],
        pOutput->gfxBufferRess[ssn::GFX_BUFFER_MASTER] );

    llce::gfx::render_context_t metaRC(
        llce::box_t(-1.0f, -1.0f, 2.0f, 2.0f),
        &ssn::color::BACKGROUND );
    metaRC.render();

    const ssn::bounds_t* const bounds = &pState->bounds;
    const ssn::puck_t* const puck = &pState->puck;
    const ssn::paddle_t* const paddle = &pState->paddle;

    bounds->render();
    puck->render();
    paddle->render();

    return true;
}

}
