#ifndef SSN_ENTITY_T_H
#define SSN_ENTITY_T_H

#include "box_t.h"
#include "circle_t.h"

#include "ssn_data.h"
#include "consts.h"

namespace ssn {

class entity_t {
    public:

    /// Class Attributes ///

    constexpr static uint32_t SEGMENT_COUNT = 20; // units: unitless

    /// Constructors ///

    entity_t( const llce::box_t& pBBox, const color4u8_t* pColor );
    entity_t( const llce::circle_t& pBounds, const color4u8_t* pColor );

    /// Class Functions ///

    void update( const float64_t pDT );
    void render() const;

    /// Class Fields ///

    public:

    llce::circle_t mBounds; // units: world
    llce::box_t mBBox; // units: world
    vec2f32_t mVel; // units: world / second
    vec2f32_t mAccel; // units: world / second**2
    const color4u8_t* mColor; // units: (r,g,b,a)
};

}

#endif
