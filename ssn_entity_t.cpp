#include <cmath>

#include <SDL2/SDL_opengl.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "gfx.h"
#include "geom.h"
#include "ssn_entity_t.h"

namespace ssn {

/// Class Functions ///

entity_t::entity_t( const llce::box_t& pBBox, const color4u8_t* pColor ) :
        mBounds( pBBox.center(), std::max(pBBox.mDims.x, pBBox.mDims.y) ), mBBox( pBBox ),
        mVel( 0.0f, 0.0f ), mAccel( 0.0f, 0.0f ), mColor( pColor ) {
    
}

entity_t::entity_t( const llce::circle_t& pBounds, const color4u8_t* pColor ) :
        mBounds( pBounds ), mBBox( pBounds.mCenter, 2.0f * pBounds.mRadius * vec2f32_t(1.0f, 1.0f), llce::geom::anchor2D::mm ),
        mVel( 0.0f, 0.0f ), mAccel( 0.0f, 0.0f ), mColor( pColor ) {
    
}


void entity_t::update( const float64_t pDT ) {
    mVel += static_cast<float32_t>( pDT ) * mAccel;
    mBounds.mCenter += static_cast<float32_t>( pDT ) * mVel;
    mBBox.mPos += static_cast<float32_t>( pDT ) * mVel;
}


void entity_t::render() const {
    const static llce::circle_t csRenderCircle( vec2f32_t(0.5f, 0.5f), 1.0f );
    llce::gfx::render_context_t entityRC( mBBox, mColor );
    llce::gfx::circle::render( csRenderCircle, mColor );
}

}
