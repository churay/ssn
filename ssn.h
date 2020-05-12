#ifndef SSN_LIB_H
#define SSN_LIB_H

#include <SDL2/SDL.h>

#include "ssn_entities.h"
#include "ssn_particles.h"
#include "ssn_consts.h"

#include "gui.h"
#include "input.h"
#include "output.h"
#include "consts.h"

namespace ssn {

/// State Types/Variables ///

struct state_t {
    // Global State //
    float64_t dt; // frame time
    float64_t tt; // total time
    float64_t st; // state time

    mode::mode_e mid; // game mode
    mode::mode_e pmid; // pending mode

    llce::rng_t rng; // random number generator

    // Game State //
    float64_t rt; // round time
    float64_t ht; // hit time

    bounds_t bounds;
    puck_t puck;
    paddle_t paddle;

    particulator_t particulator;

    // Scoring State //
    float32_t scoreTotals[2];
    bit8_t scoreSamples[SCORE_SAMPLES_BYTES];
    bool32_t scoreTallied;

    // Menu State //
    llce::gui::menu_t titleMenu;
    llce::gui::menu_t resetMenu;
};

/// Input/Output Types/Variables ///

typedef llce::input::input_t<true, false> input_t;
typedef llce::output::output_t<1, 0> output_t;

}

/// Functions ///

#if !LLCE_DYLOAD
extern "C" {
    bool32_t init( ssn::state_t* pState, ssn::input_t* pInput );
    bool32_t boot( ssn::output_t* pOutput );
    bool32_t update( ssn::state_t* pState, ssn::input_t* pInput, const ssn::output_t* pOutput, const float64_t pDT );
    bool32_t render( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput );
};
#endif

#endif
