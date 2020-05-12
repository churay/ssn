#ifndef SSN_CONSTS_H
#define SSN_CONSTS_H

#include "ssn_data.h"

namespace ssn {

/// Global Constants ///

namespace mode { enum mode_e { boot_id = -1, exit_id = -2, game_id = 0, title_id, score_id, reset_id }; };
namespace team { enum team_e { left = 0, right, neutral }; };

/// Game State Constants ///

constexpr static float64_t HIT_DURATION = 0.3; // units: seconds
constexpr static float64_t ROUND_DURATION = 1.0; // units: seconds

/// Scoring Constants ///

constexpr static uint32_t SCORE_SAMPLE_BITS = 2;
constexpr static uint32_t SCORE_SAMPLES_PER_BYTE = ( 8 / SCORE_SAMPLE_BITS );
constexpr static vec2u32_t SCORE_SAMPLE_RES( 512, 512 );
constexpr static uint32_t SCORE_SAMPLES_COUNT = SCORE_SAMPLE_RES.x * SCORE_SAMPLE_RES.y;
constexpr static uint32_t SCORE_SAMPLES_BYTES = SCORE_SAMPLES_COUNT / SCORE_SAMPLES_PER_BYTE;

};

#endif
