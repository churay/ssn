#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>

#include <glm/common.hpp>

#include <cmath>
#include <cstring>

#include "gfx.h"
#include "geom.h"
#include "input.h"
#include "output.h"

#include "ssn_modes.h"
#include "ssn_consts.h"
#include "ssn.h"

/// Global Declarations ///

typedef bool32_t (*init_f)( ssn::state_t* );
typedef bool32_t (*update_f)( ssn::state_t*, ssn::input_t*, const float64_t );
typedef bool32_t (*render_f)( const ssn::state_t*, const ssn::input_t*, const ssn::output_t* );

/// Per-Mode Tables ///

constexpr static init_f MODE_INIT_FUNS[] = {
    ssn::mode::game::init, ssn::mode::select::init, ssn::mode::title::init, ssn::mode::score::init, ssn::mode::reset::init };
constexpr static update_f MODE_UPDATE_FUNS[] = {
    ssn::mode::game::update, ssn::mode::select::update, ssn::mode::title::update, ssn::mode::score::update, ssn::mode::reset::update };
constexpr static render_f MODE_RENDER_FUNS[] = {
    ssn::mode::game::render, ssn::mode::select::render, ssn::mode::title::render, ssn::mode::score::render, ssn::mode::reset::render };
constexpr static uint32_t MODE_COUNT = ARRAY_LEN( MODE_INIT_FUNS );

/// Interface Functions ///

extern "C" bool32_t boot( ssn::output_t* pOutput ) {
    // Initialize Graphics //

    const vec2u32_t cGFXBuffRes( 1024, 1024 );

    llce::output::boot<1, 0>( *pOutput, cGFXBuffRes );

    // Initialize Sound //

    // ... //

    return true;
}


extern "C" bool32_t init( ssn::state_t* pState, ssn::input_t* pInput ) {
    // Initialize Global Variables //

    pState->dt = 0.0;
    pState->tt = 0.0;
    pState->st = 0.0;

    pState->mode = ssn::mode::boot::ID;
    pState->pmode = ssn::mode::title::ID;

    pState->rng = llce::rng_t( ssn::RNG_SEED );

    // Initialize Input //

    // TODO(JRC): Clean this up if at all possible! The use of a 'defaultActions'
    // identity map is particularly aggregious; perhaps just assume all actions are
    // mapped in the range [0,...LLCE_MAX_ACTIONS]?
    uint32_t defaultActions[ssn::action::_length];
    uint32_t defaultBindings[ssn::action::_length]; {
        defaultActions[ssn::action::lup] = ssn::action::lup;
        defaultBindings[ssn::action::lup] = llce::input::stream_t( SDL_SCANCODE_W, llce::input::device_e::keyboard );
        defaultActions[ssn::action::ldown] = ssn::action::ldown;
        defaultBindings[ssn::action::ldown] = llce::input::stream_t( SDL_SCANCODE_S, llce::input::device_e::keyboard );
        defaultActions[ssn::action::lleft] = ssn::action::lleft;
        defaultBindings[ssn::action::lleft] = llce::input::stream_t( SDL_SCANCODE_A, llce::input::device_e::keyboard );
        defaultActions[ssn::action::lright] = ssn::action::lright;
        defaultBindings[ssn::action::lright] = llce::input::stream_t( SDL_SCANCODE_D, llce::input::device_e::keyboard );
        defaultActions[ssn::action::lrush] = ssn::action::lrush;
        defaultBindings[ssn::action::lrush] = llce::input::stream_t( SDL_SCANCODE_E, llce::input::device_e::keyboard );
        defaultActions[ssn::action::rup] = ssn::action::rup;
        defaultBindings[ssn::action::rup] = llce::input::stream_t( SDL_SCANCODE_I, llce::input::device_e::keyboard );
        defaultActions[ssn::action::rdown] = ssn::action::rdown;
        defaultBindings[ssn::action::rdown] = llce::input::stream_t( SDL_SCANCODE_K, llce::input::device_e::keyboard );
        defaultActions[ssn::action::rleft] = ssn::action::rleft;
        defaultBindings[ssn::action::rleft] = llce::input::stream_t( SDL_SCANCODE_J, llce::input::device_e::keyboard );
        defaultActions[ssn::action::rright] = ssn::action::rright;
        defaultBindings[ssn::action::rright] = llce::input::stream_t( SDL_SCANCODE_L, llce::input::device_e::keyboard );
        defaultActions[ssn::action::rrush] = ssn::action::rrush;
        defaultBindings[ssn::action::rrush] = llce::input::stream_t( SDL_SCANCODE_O, llce::input::device_e::keyboard );
    }

    std::memset( pInput, 0, sizeof(ssn::input_t) );
    pInput->binding = llce::input::binding_t( &defaultActions[0], &defaultBindings[0], ssn::action::_length );

    // Initialize Per-Mode Variables //

    bool32_t initStatus = true;

    for( uint32_t modeIdx = 0; modeIdx < MODE_COUNT; modeIdx++ ) {
        initStatus &= MODE_INIT_FUNS[modeIdx]( pState );
    }

    return initStatus;
}


extern "C" bool32_t update( ssn::state_t* pState, ssn::input_t* pInput, const ssn::output_t* pOutput, const float64_t pDT ) {
    if( pState->mode != pState->pmode ) {
        if( pState->pmode < 0 ) { return false; }
        MODE_INIT_FUNS[pState->pmode]( pState );
        pState->mode = pState->pmode;
        pState->st = 0.0;
    }

    pState->dt = pDT;
    pState->tt += pDT;
    pState->st += pDT;

    bool32_t updateStatus = MODE_UPDATE_FUNS[pState->mode]( pState, pInput, pDT );
    return updateStatus;
}


extern "C" bool32_t render( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) {
    llce::gfx::fbo_context_t metaFBOC(
        pOutput->gfxBufferFBOs[llce::output::BUFFER_SHARED_ID],
        pOutput->gfxBufferRess[llce::output::BUFFER_SHARED_ID] );

    llce::gfx::render_context_t metaRC( llce::box_t(-1.0f, -1.0f, 2.0f, 2.0f) );
    llce::gfx::color_context_t metaCC( &ssn::color::ERROR );
    llce::gfx::render::box();

    bool32_t renderStatus = MODE_RENDER_FUNS[pState->mode]( pState, pInput, pOutput );
    return renderStatus;
}
