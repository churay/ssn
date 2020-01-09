#include <cstring>

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <SDL2/SDL_opengl.h>

#include "gfx.h"
#include "ssn_entity_t.h"

namespace ssn {

/// Class Functions ///

entity_t::entity_t( const llce::box_t& pBBox, const color4u8_t* pColor ) :
        mBBox( pBBox ), mVel( 0.0f, 0.0f ), mAccel( 0.0f, 0.0f ), mColor( pColor ) {
    
}


void entity_t::update( const float64_t pDT ) {
    mVel += static_cast<float32_t>( pDT ) * mAccel;
    mBBox.mPos += static_cast<float32_t>( pDT ) * mVel;
}


void entity_t::render() const {
    llce::gfx::render_context_t entityRC( mBBox, mColor );
    entityRC.render();
}

}
