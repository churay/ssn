#ifndef SSN_LIB_H
#define SSN_LIB_H

#include <SDL2/SDL.h>

#include "input.h"
#include "consts.h"

namespace ssn {

/// State Types/Variables ///

struct state_t {
    // FPS State //
    color4f32_t hsvColor;
};

/// Input Types/Variables ///

struct input_t {
    llce::input::keyboard_t keyboard;
};

/// Output Types/Variables ///

constexpr static uint32_t GFX_BUFFER_MASTER = 0;
constexpr static uint32_t GFX_BUFFER_COUNT = 1;

constexpr static uint32_t SFX_BUFFER_MASTER = 0;
constexpr static uint32_t SFX_BUFFER_COUNT = 1;

struct output_t {
    // Graphics Output //
    uint32_t gfxBufferFBOs[GFX_BUFFER_COUNT];   // frame buffers
    uint32_t gfxBufferCBOs[GFX_BUFFER_COUNT];   // color buffers
    uint32_t gfxBufferDBOs[GFX_BUFFER_COUNT];   // depth buffers
    vec2u32_t gfxBufferRess[GFX_BUFFER_COUNT];  // buffer resolutions

    // Audio Output //
    SDL_AudioSpec sfxConfig;
    bit8_t* sfxBuffers[SFX_BUFFER_COUNT];
    uint32_t sfxBufferFrames[SFX_BUFFER_COUNT];
};

}

#if !LLCE_DYLOAD
extern "C" {
    bool32_t init( ssn::state_t* pState, ssn::input_t* pInput );
    bool32_t boot( ssn::output_t* pOutput );
    bool32_t update( ssn::state_t* pState, ssn::input_t* pInput, const ssn::output_t* pOutput, const float64_t pDT );
    bool32_t render( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput );
};
#endif

#endif
