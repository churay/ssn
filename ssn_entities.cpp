#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float2.hpp>

#include "gfx.h"
#include "ssn_entities.h"

namespace ssn {

/// 'ssn::ship_t' Functions ///

ship_t::ship_t( const llce::box_t& pBBox ) :
        entity_t( pBBox, &ssn::color::SHIP ), mDI( 0, 0 ) {
    
}


void ship_t::move( const int32_t pDX, const int32_t pDY ) {
    mDI.x = static_cast<float32_t>( glm::clamp(pDX, -1, 1) );
    mDI.y = static_cast<float32_t>( glm::clamp(pDY, -1, 1) );
}


void ship_t::update( const float64_t pDT ) {
    // TODO(JRC): Adjust final velocity using 'ship_t::MAX_VEL' as an upper limit.
    mAccel = MOVE_ACCEL * mDI;
    mVel += static_cast<float32_t>( pDT ) * mAccel;
    mBBox.mPos += static_cast<float32_t>( pDT ) * mVel;
}

}
