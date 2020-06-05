#ifndef SSN_CONSTS_H
#define SSN_CONSTS_H

#include "ssn_data.h"

namespace ssn {

/// Global Constants ///

namespace action { enum action_e {
    lup, ldown, lleft, lright, lrush,
    rup, rdown, rleft, rright, rrush,
    _length
}; };
typedef action::action_e action_e;

namespace team { enum team_e { left = 0, right, neutral, _length }; };
typedef team::team_e team_e;

namespace stage { enum stage_e { box = 0, vert, horz, wild, _length }; };
typedef stage::stage_e stage_e;

typedef int32_t mode_e;

/// Game State Constants ///

constexpr static char8_t ACTION_NAMES[][32] = {
    "MOVE UP (LEFT)", "MOVE DOWN (LEFT)", "MOVE LEFT (LEFT)", "MOVE RIGHT (LEFT)", "RUSH/SELECT (LEFT)",
    "MOVE UP (RIGHT)", "MOVE DOWN (RIGHT)", "MOVE LEFT (RIGHT)", "MOVE RIGHT (RIGHT)", "RUSH/SELECT (RIGHT)",
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
