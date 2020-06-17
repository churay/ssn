#ifndef SSN_CONSTS_H
#define SSN_CONSTS_H

#include "ssn_data.h"

namespace ssn {

/// Global Constants ///

LLCE_ACTION_ENUM(
    lup, ldown, lleft, lright, lrush,
    rup, rdown, rleft, rright, rrush );

LLCE_ENUM( team, left, right, neutral );
LLCE_ENUM( stage, box, vert, horz, wild );

typedef int32_t mode_e;

/// Game State Constants ///

constexpr static char8_t ACTION_NAMES[][32] = {
    "MOVE UP (L)", "MOVE DOWN (L)", "MOVE LEFT (L)", "MOVE RIGHT (L)", "RUSH/SELECT (L)",
    "MOVE UP (R)", "MOVE DOWN (R)", "MOVE LEFT (R)", "MOVE RIGHT (R)", "RUSH/SELECT (R)",
};

constexpr static vec2f32_t STAGE_SPECS[] = {
    {1.0f, 1.0f},
    {0.75f, 1.0f},
    {1.0f, 0.75f},
    {0.60f, 0.80f}
};
constexpr static char8_t STAGE_NAMES[][8] = {
    "BOX",
    "VERT",
    "HORZ",
    "WILD"
};

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
