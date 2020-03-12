#include <SDL2/SDL_opengl.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "interval_t.h"

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

particulator_t::particulator_t( llce::rng_t* const pRNG ) :
        mRNG( pRNG ), mSize( 0 ) {
    
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


void particulator_t::generate( const vec2f32_t& pSource, const vec2f32_t& pDir ) {
    const static uint32_t csAverageCount = particulator_t::MAX_PARTICLE_COUNT / 8;
    const static uint32_t csMaxDeviation = 2;

    const uint32_t cNewCount = csAverageCount;
    const uint32_t cAvailCount = std::min( cNewCount, particulator_t::MAX_PARTICLE_COUNT - mSize );
    LLCE_CHECK_WARNING( cNewCount != cAvailCount,
        "Couldn't generate all particles due to insufficient space; " <<
        "using all " << cAvailCount << " available particles." );

    const static float32_t csThetaRange = M_PI / 4.0f;
    const static float32_t csMagRange = 1.0e-1f;

    const llce::interval_t cThetaInt(
        glm::orientedAngle(vec2f32_t(1.0f, 0.0f), pDir), csThetaRange,
        llce::interval_t::anchor_e::avg );
    const llce::interval_t cMagInt(
        1.0f, csMagRange,
        llce::interval_t::anchor_e::avg );

    for( uint32_t partIdx = 0; partIdx < cAvailCount; partIdx++ ) {
        particle_t& particle = mParticles[mSize + partIdx];
        float32_t partTheta = cThetaInt.interp( static_cast<float32_t>(mRNG->nextf()) );
        // particle.mPos = ;
        // particle.mVel = ;
        // particle.mPos = ;
    }
}

}
