#ifndef SSN_ENTITIES_T_H
#define SSN_ENTITIES_T_H

#include <glm/common.hpp>

#include "ssn_entity_t.h"
#include "circle_t.h"

#include "ssn_data.h"
#include "consts.h"

namespace ssn {

class ship_t : public entity_t {
    public:

    /// Class Attributes ///

    constexpr static float32_t MAX_VEL = 1.0e0f;    // units: world / second
    constexpr static float32_t MOVE_ACCEL = 8.0e-2f; // units: world / second**2

    /// Constructors ///

    ship_t( const llce::circle_t& pBounds );

    /// Class Functions ///

    void update( const float64_t pDT );
    void move( const int32_t pDX, const int32_t pDY );

    /// Class Fields ///

    public:

    vec2f32_t mDI;
};

}

#endif
