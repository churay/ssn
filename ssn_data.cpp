#include "ssn_data.h"

namespace ssn {

const uint64_t RNG_SEED = 12345678;

namespace color {

const color4u8_t ERROR = { 0xff, 0x00, 0x00, 0xff };
const color4u8_t INFO = { 0xff, 0xff, 0xff, 0xff };

const color4u8_t BACKGROUND = { 0x00, 0x2b, 0x36, 0xff };
const color4u8_t FOREGROUND = static_cast<uint8_t>( 2 ) * ssn::color::BACKGROUND;
const color4u8_t INTERFACE = { 0x58, 0x6e, 0x75, 0xff };

const color4u8_t TEAM[3] = {
    {0x9a, 0x86, 0x00, 0xff},    // left
    {0x00, 0x9d, 0xa3, 0xff},    // right
    {0x80, 0x7e, 0x76, 0xff} };  // neutral

};

};
