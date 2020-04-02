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
    pOutput->gfxBufferRess[ssn::GFX_BUFFER_MASTER] = { 1024, 1024 };

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
    pState->dt = 0.0;
    pState->tt = 0.0;
    pState->ht = 0.0;

    pState->rng = llce::rng_t( ssn::RNG_SEED );

    const float32_t cPaddleBaseRadius = 5.0e-2f;

    const vec2f32_t boundsBasePos( 0.0f, 0.0f );
    const vec2f32_t boundsDims( 1.0f, 1.0f );
    pState->bounds = ssn::bounds_t( llce::box_t(boundsBasePos, boundsDims) );

    const vec2f32_t puckCenterPos( 0.25f, 0.5f );
    const float32_t puckRadius( cPaddleBaseRadius * 0.6f );
    pState->puck = ssn::puck_t( llce::circle_t(puckCenterPos, puckRadius),
        ssn::team::neutral, &pState->bounds );

    const vec2f32_t paddleCenterPos( 0.5f, 0.5f );
    const float32_t paddleRadius( cPaddleBaseRadius );
    pState->paddle = ssn::paddle_t( llce::circle_t(paddleCenterPos, paddleRadius),
        ssn::team::left, &pState->bounds );

    pState->particulator = ssn::particulator_t( &pState->rng );

    // { // Testing Score Calculations //
    //     ssn::team_entity_t testEntity( llce::circle_t(0.0f, 0.0f, 0.0f), ssn::team::right );

    //     const vec2f32_t cTestAreas[2][3] = {
    //         {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}},
    //         {{0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}} };
    //     const ssn::team::team_e cTestTeams[] = {
    //         ssn::team::right,   // r score: 0.25
    //         ssn::team::left };  // l score: 0.5
    //     const uint32_t cTestAreaCount = ARRAY_LEN( cTestAreas );
    //     for( uint32_t areaIdx = 0; areaIdx < cTestAreaCount; areaIdx++ ) {
    //         testEntity.change( cTestTeams[areaIdx] );
    //         for( uint32_t cornerIdx = 0; cornerIdx < ssn::bounds_t::AREA_CORNER_COUNT; cornerIdx++ ) {
    //             vec2f32_t prevCenter = testEntity.mBounds.mCenter;
    //             testEntity.mBounds.mCenter = cTestAreas[areaIdx][cornerIdx];
    //             testEntity.mBBox.mPos += ( testEntity.mBounds.mCenter - prevCenter );
    //             pState->bounds.claim( &testEntity );
    //         }
    //     }
    // }

    std::memset( pInput, 0, sizeof(ssn::input_t) );

    return true;
}


extern "C" bool32_t update( ssn::state_t* pState, ssn::input_t* pInput, const ssn::output_t* pOutput, const float64_t pDT ) {
    vec2i32_t di = { 0, 0 };
    bool32_t de = false;

    { // Input Processing //
        if( llce::input::isKeyDown(pInput->keyboard, SDL_SCANCODE_W) ) {
            di.y += 1;
        } if( llce::input::isKeyDown(pInput->keyboard, SDL_SCANCODE_S) ) {
            di.y -= 1;
        } if( llce::input::isKeyDown(pInput->keyboard, SDL_SCANCODE_A) ) {
            di.x -= 1;
        } if( llce::input::isKeyDown(pInput->keyboard, SDL_SCANCODE_D) ) {
            di.x += 1;
        }

        if( llce::input::isKeyDown(pInput->keyboard, SDL_SCANCODE_E) ) {
            de = true;
        }
    }

    ssn::bounds_t* const bounds = &pState->bounds;
    ssn::puck_t* const puck = &pState->puck;
    ssn::paddle_t* const paddle = &pState->paddle;
    ssn::particulator_t* const particulator = &pState->particulator;

    { // Game State Update //
        pState->dt = pDT;
        pState->tt += pDT;

        const bool32_t cPaddleWasRushing = paddle->mAmRushing;

        if( pState->ht > 0.0 ) {
            pState->ht = ( pState->ht < ssn::MAX_HIT_TIME ) ? pState->ht + pDT : 0.0;
        } else {
            paddle->move( di.x, di.y );
            if( de ) { paddle->rush(); }

            bounds->update( pDT );
            puck->update( pDT );
            paddle->update( pDT );

            if( puck->hit(paddle) ) {
                bounds->claim( paddle );
                particulator->genHit(
                    puck->mBounds.mCenter, puck->mVel, 2.25f * puck->mBounds.mRadius );
                pState->ht += pDT;
            } if( !cPaddleWasRushing && paddle->mAmRushing ) {
                particulator->genTrail(
                    paddle->mBounds.mCenter, paddle->mVel, 2.0f * paddle->mBounds.mRadius );
            }
        }

        particulator->update( pDT );
    }

    if( llce::input::isKeyDown(pInput->keyboard, SDL_SCANCODE_T) ) { // Game Scoring //
        vec2i32_t scores = { 0, 0 };

        // NOTE(JRC): The source for this algorithm was derived from here:
        // http://geomalgorithms.com/a01-_area.html
        const static auto csTriDeterminant = []
                ( const vec2f32_t& pP0, const vec2f32_t& pP1, const vec2f32_t& pP2 ) {
            return ( (pP1.x - pP0.x) * (pP2.y - pP0.y) - (pP2.x - pP0.x) * (pP1.y - pP0.y) );
        };

        // NOTE(JRC): The source for this algorithm was derived from here:
        // http://geomalgorithms.com/a03-_inclusion.html
        const static auto csPointInArea = [] ( const vec2f32_t& pPoint,  const vec2f32_t* pArea ) {
            int32_t windCount = 0;

            for( uint32_t cornerIdx = 0; cornerIdx < bounds_t::AREA_CORNER_COUNT; cornerIdx++ ) {
                const vec2f32_t& cornerStart = pArea[cornerIdx];
                const vec2f32_t& cornerEnd = pArea[(cornerIdx + 1) % bounds_t::AREA_CORNER_COUNT];

                // if the corner edge intersects the point x-axis ray upward...
                if( cornerStart.y < pPoint.y && cornerEnd.y > pPoint.y ) {
                    // and the point is strictly left of the edge (s->e->p is ccw)
                    if( csTriDeterminant(cornerStart, cornerEnd, pPoint) > 0.0f ) {
                        windCount++;
                    }
                // if the corner edge intersects the point x-axis ray downward...
                } else if( cornerStart.y > pPoint.y && cornerEnd.y < pPoint.y ) {
                    // and the point is strictly right of the edge (s->e->p is cw)
                    if( csTriDeterminant(cornerStart, cornerEnd, pPoint) < 0.0f ) {
                        windCount--;
                    }
                }
            }

            return windCount != 0;
        };

        const vec2u32_t& cPixelRes = pOutput->gfxBufferRess[ssn::GFX_BUFFER_MASTER];
        for( uint32_t yPixelIdx = 0; yPixelIdx < cPixelRes.y; yPixelIdx++ ) {
            for( uint32_t xPixelIdx = 0; xPixelIdx < cPixelRes.x; xPixelIdx++ ) {
                vec2f32_t pixelPos(
                    bounds->mBBox.xbounds().interp( (xPixelIdx + 0.5f) / cPixelRes.x ),
                    bounds->mBBox.ybounds().interp( (yPixelIdx + 0.5f) / cPixelRes.y ) );

                for( uint32_t areaIdx = bounds->mAreaCount; areaIdx-- > 0; ) {
                    vec2f32_t* areaPoss = &bounds->mAreaCorners[areaIdx * bounds_t::AREA_CORNER_COUNT];
                    if( csPointInArea(pixelPos, areaPoss) ) {
                        (*VECTOR_AT(scores, bounds->mAreaTeams[areaIdx]))++;
                        break;
                    }
                }
            }
        }

        const uint32_t cPixelTotal = cPixelRes.x * cPixelRes.y;
        std::cout << "Scores:" << std::endl;
        std::cout << "  Left: " << (*VECTOR_AT(scores, ssn::team::left)) / ( cPixelTotal + 0.0f ) << std::endl;
        std::cout << "  Right: " << (*VECTOR_AT(scores, ssn::team::right)) / ( cPixelTotal + 0.0f ) << std::endl;
    }

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
    const ssn::particulator_t* const particulator = &pState->particulator;

    { // Game State Render //
        bounds->render();
        puck->render();
        particulator->render();
        paddle->render();
    }

    return true;
}

}
