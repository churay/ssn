#ifndef SSN_ENTITY_T_H
#define SSN_ENTITY_T_H

#include "box_t.h"

#include "ssn_data.h"
#include "consts.h"

namespace ssn {

class entity_t {
    public:

    /// Constructors ///

    entity_t( const llce::box_t& pBBox, const color4u8_t* pColor );

    /// Class Functions ///

    void update( const float64_t pDT );
    void render() const;

    /// Class Fields ///

    public:

    // TODO(JRC): May need a new type; polygonal object instead of
    // bounding box!
    llce::box_t mBBox; // units: world
    vec2f32_t mVel; // units: world / second
    vec2f32_t mAccel; // units: world / second**2
    const color4u8_t* mColor; // units: (r,g,b,a)
};

}

#endif
