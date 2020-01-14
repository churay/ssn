#include <cmath>

#include <SDL2/SDL_opengl.h>
#include <glm/common.hpp>

#include "box_t.h"
#include "gfx.h"
#include "ssn_entity_t.h"

namespace ssn {

/// Class Functions ///

entity_t::entity_t( const llce::circle_t& pBounds, const color4u8_t* pColor ) :
        mBounds( pBounds ), mVel( 0.0f, 0.0f ), mAccel( 0.0f, 0.0f ), mColor( pColor ) {
    
}


void entity_t::update( const float64_t pDT ) {
    mVel += static_cast<float32_t>( pDT ) * mAccel;
    mBounds.mCenter += static_cast<float32_t>( pDT ) * mVel;
}


void entity_t::render() const {
    vec2f32_t entityBounds( 2.0f * mBounds.mRadius, 2.0f * mBounds.mRadius );
    llce::box_t entityBBox( mBounds.mCenter, entityBounds, llce::box_t::anchor_e::c );

    llce::gfx::render_context_t entityRC( entityBBox, mColor );
    glBegin( GL_POLYGON );
        for( uint32_t segmentIdx = 0; segmentIdx < entity_t::SEGMENT_COUNT;  ++segmentIdx ) {
            float32_t segmentRadians = 2.0f * M_PI *
                ( segmentIdx / (entity_t::SEGMENT_COUNT + 0.0f) );
            glVertex2f( std::cos(segmentRadians), std::sin(segmentRadians) );
        }
    glEnd();
}

}
