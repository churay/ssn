#include <SDL2/SDL_opengl.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>

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

/// 'ssn::puck_t' Functions ///

puck_t::puck_t( const llce::circle_t& pBounds ) :
        entity_t( pBounds, &ssn::color::PUCK ) {
    
}


void puck_t::hit( const entity_t* pSource ) {
    if( pSource->mBounds.overlaps(mBounds) ) {
        mBounds.exbed( pSource->mBounds );
        mBBox.mPos = mBounds.mCenter - 0.5f * mBBox.mDims;

        vec2f32_t hitVec = glm::normalize( mBounds.mCenter - pSource->mBounds.mCenter );
        float32_t hitMag = glm::length( pSource->mVel );
        mVel = hitVec * hitMag;
    }
}

}
