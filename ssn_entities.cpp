#include <SDL2/SDL_opengl.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "box_t.h"
#include "gfx.h"
#include "ssn_entities.h"

namespace ssn {

/// 'ssn::bounds_t' Functions ///

bounds_t::bounds_t( const llce::box_t& pBBox ) :
        entity_t( pBBox, &ssn::color::BACKGROUND ) {
    
}


void bounds_t::render() const {
    llce::gfx::render_context_t baseRC( mBBox, mColor );
    baseRC.render();
}

/// 'ssn::paddle_t' Functions ///

paddle_t::paddle_t( const llce::circle_t& pBounds ) :
        entity_t( pBounds, &ssn::color::PADDLE ), mDI( 0, 0 ) {
    
}


void paddle_t::move( const int32_t pDX, const int32_t pDY ) {
    mDI.x = static_cast<float32_t>( glm::clamp(pDX, -1, 1) );
    mDI.y = static_cast<float32_t>( glm::clamp(pDY, -1, 1) );
}


void paddle_t::update( const float64_t pDT ) {
    // TODO(JRC): Adjust final velocity using 'paddle_t::MAX_VEL' as an upper limit.
    mAccel = MOVE_ACCEL * mDI;
    mVel += static_cast<float32_t>( pDT ) * mAccel;
    mBounds.mCenter += static_cast<float32_t>( pDT ) * mVel;
    mBBox.mPos += static_cast<float32_t>( pDT ) * mVel;
}

void paddle_t::render() const {
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
