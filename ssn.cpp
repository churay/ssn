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
    ssn::mode::game::init, ssn::mode::title::init, ssn::mode::score::init, ssn::mode::reset::init };
constexpr static update_f MODE_UPDATE_FUNS[] = {
    ssn::mode::game::update, ssn::mode::title::update, ssn::mode::score::update, ssn::mode::reset::update };
constexpr static render_f MODE_RENDER_FUNS[] = {
    ssn::mode::game::render, ssn::mode::title::render, ssn::mode::score::render, ssn::mode::reset::render };
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

    pState->mid = ssn::mode::boot_id;
    pState->pmid = ssn::mode::title_id;

    pState->rng = llce::rng_t( ssn::RNG_SEED );

    // Initialize Input //

    std::memset( pInput, 0, sizeof(ssn::input_t) );

    // Initialize Per-Mode Variables //

    bool32_t initStatus = true;

    for( uint32_t modeIdx = 0; modeIdx < MODE_COUNT; modeIdx++ ) {
        initStatus &= MODE_INIT_FUNS[modeIdx]( pState );
    }

    return initStatus;
}


extern "C" bool32_t update( ssn::state_t* pState, ssn::input_t* pInput, const ssn::output_t* pOutput, const float64_t pDT ) {
    if( pState->mid != pState->pmid ) {
        if( pState->pmid < 0 ) { return false; }
        MODE_INIT_FUNS[pState->pmid]( pState );
        pState->mid = pState->pmid;
    }

    pState->dt = pDT;
    pState->tt += pDT;

    bool32_t updateStatus = MODE_UPDATE_FUNS[pState->mid]( pState, pInput, pDT );
    return updateStatus;
}


extern "C" bool32_t render( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) {
    llce::gfx::fbo_context_t metaFBOC(
        pOutput->gfxBufferFBOs[llce::output::BUFFER_SHARED_ID],
        pOutput->gfxBufferRess[llce::output::BUFFER_SHARED_ID] );

    llce::gfx::render_context_t metaRC(
        llce::box_t(-1.0f, -1.0f, 2.0f, 2.0f),
        &ssn::color::BACKGROUND );
    metaRC.render();

    bool32_t renderStatus = MODE_RENDER_FUNS[pState->mid]( pState, pInput, pOutput );
    return renderStatus;
}
