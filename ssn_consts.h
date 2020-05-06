#ifndef SSN_CONSTS_H
#define SSN_CONSTS_H

#include "ssn_data.h"

namespace ssn {

constexpr static float64_t HIT_DURATION = 0.3;
constexpr static float64_t ROUND_DURATION = 5.0;

namespace mode { enum mode_e { boot_id = -1, exit_id = -2, game_id = 0, title_id, score_id, reset_id }; };
namespace team { enum team_e { left = 0, right, neutral }; };

};

#endif
