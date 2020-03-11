#include <SDL2/SDL_opengl.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "ssn_particles.h"

namespace ssn {

/// 'ssn::particle_t' Functions ///

particle_t::particle_t() :
        mPos( 0.0f, 0.0f ), mVel( 0.0f, 0.0f ), mAccel( 0.0f, 0.0f ), mColor( NULL ), mLifetime( 0.0f ) {
    
}


particle_t::particle_t( const vec2f32_t& pPos, const color4u8_t* pColor, const float32_t pLifetime ) :
        mPos( pPos ), mVel( 0.0f, 0.0f ), mAccel( 0.0f, 0.0f ), mColor( pColor ), mLifetime( pLifetime ) {
    
}


void particle_t::update( const float64_t pDT ) {
    mVel += static_cast<float32_t>( pDT ) * mAccel;
    mPos += static_cast<float32_t>( pDT ) * mVel;
    mLifetime = std::max( 0.0f, mLifetime - static_cast<float32_t>(pDT) );
}


void particle_t::render() const {
    // TODO(JRC): Use the direction and magnitude of the velocity to inform
    // the particle rendering method.
}


bool32_t particle_t::valid() const {
    return mLifetime > 0.0f;
}

/// 'ssn::particulator_t' Functions ///

particulator_t::particulator_t() :
        mSize( 0 ) {
    
}


void particulator_t::update( const float64_t pDT ) {
    // TODO(JRC): Update all lifetimes, then remove min until min has positive lifetime
    // NOTE(JRC): Upshot w/ this method is that we can have dynamic lifetimes, where
    // an update costs O(logn), insert typically is O(1), and remove is O(logn) per frame
    // (tend not to be many removals per frame), also w/ good cache performance
}


void particulator_t::render() const {
    for( uint32_t partIdx = 0; partIdx < mSize && mParticles[mSize].valid(); partIdx++ ) {
        const particle_t& particle = mParticles[mSize];
        particle.render();
    }
}


void particulator_t::generate( const vec2f32_t& pBasePos, const vec2f32_t& pDir ) {
    // const uint32_t cCount = ;

    // const uint32_t cAvailCount = std::min( pCount, particulartor_t::MAX_PARTICLE_COUNT - mSize );
    // LLCE_CHECK_WARNING( pCount == cAvailCount,
    //     "Couldn't generate an additional " << pCount << " particles due " <<
    //     "to insufficient unavailable particles; generated available amount " <<
    //     cAvailCount << " instead." );

    // for( uint32_t partIdx = 0; partIdx < cAvailCount; partIdx++ ) {
    //     particle_t& particle = mParticles[mSize + partIdx];
    //     // particle.mPos = ;
    //     // particle.mVel = ;
    //     // particle.mPos = ;
    // }
}

}
