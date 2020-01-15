#include <cmath>

#include <SDL2/SDL_opengl.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "gfx.h"
#include "ssn_entity_t.h"

namespace ssn {

/// Class Functions ///

entity_t::entity_t( const llce::box_t& pBBox, const color4u8_t* pColor ) :
        mBounds( pBBox.center(), std::max(pBBox.mDims.x, pBBox.mDims.y) ), mBBox( pBBox ),
        mVel( 0.0f, 0.0f ), mAccel( 0.0f, 0.0f ), mColor( pColor ) {
    
}

entity_t::entity_t( const llce::circle_t& pBounds, const color4u8_t* pColor ) :
        mBounds( pBounds ), mBBox( pBounds.mCenter, 2.0f * pBounds.mRadius * vec2f32_t(1.0f, 1.0f), llce::box_t::anchor_e::c ),
        mVel( 0.0f, 0.0f ), mAccel( 0.0f, 0.0f ), mColor( pColor ) {
    
}


void entity_t::update( const float64_t pDT ) {
    mVel += static_cast<float32_t>( pDT ) * mAccel;
    mBounds.mCenter += static_cast<float32_t>( pDT ) * mVel;
    mBBox.mPos += static_cast<float32_t>( pDT ) * mVel;
}


void entity_t::render() const {
    llce::gfx::render_context_t entityRC( mBBox, mColor );
    glPushMatrix(); {
        glm::mat4 matModelWorld( 1.0f );
        matModelWorld *= glm::translate( glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.0f) );
        matModelWorld *= glm::scale( glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 1.0f) );
        glMultMatrixf( &matModelWorld[0][0] );

        glBegin( GL_POLYGON );
        for( uint32_t segmentIdx = 0; segmentIdx < entity_t::SEGMENT_COUNT;  ++segmentIdx ) {
            float32_t segmentRadians = 2.0f * M_PI *
                ( segmentIdx / (entity_t::SEGMENT_COUNT + 0.0f) );
            glVertex2f( std::cos(segmentRadians), std::sin(segmentRadians) );
        }
        glEnd();
    } glPopMatrix();
}

}
