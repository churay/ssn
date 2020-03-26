#include <SDL2/SDL_opengl.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "interval_t.h"
#include "ssn_consts.h"

#include "ssn_particles.h"

namespace ssn {

/// 'ssn::particle_t' Constants ///

typedef void (*particle_update_f)( particle_t* pParticle, const float64_t pDT );
constexpr static particle_update_f PARTICLE_UPDATE_FUNS[] = {
    particle_t::update_undefined,
    particle_t::update_hit };

typedef void (*particle_render_f)( const particle_t* pParticle );
constexpr static particle_render_f PARTICLE_RENDER_FUNS[] = {
    particle_t::render_undefined,
    particle_t::render_hit };

/// 'ssn::particle_t' Functions ///

particle_t::particle_t() :
        mType( particle_t::type_e::undefined ), mLifetime( 0.0f ),
        mPos( 0.0f, 0.0f ), mVel( 0.0f, 0.0f ), mAccel( 0.0f, 0.0f ) {
    
}


particle_t::particle_t( const type_e pType, const float32_t pLifetime,
            const vec2f32_t& pPos, const vec2f32_t& pVel, const vec2f32_t& pAccel ) :
        mType( pType ), mLifetime( pLifetime ),
        mPos( pPos ), mVel( pVel ), mAccel( pAccel ) {
    
}


void particle_t::update( const float64_t pDT ) {
    PARTICLE_UPDATE_FUNS[mType]( this, pDT );
}


void particle_t::render() const {
    PARTICLE_RENDER_FUNS[mType]( this );
}


bool32_t particle_t::valid() const {
    return mLifetime > 0.0f;
}

/// 'ssn::particle_t' Type Functions ///

void particle_t::update_undefined( particle_t* pParticle, const float64_t pDT ) {
    pParticle->mVel += static_cast<float32_t>( pDT ) * pParticle->mAccel;
    pParticle->mPos += static_cast<float32_t>( pDT ) * pParticle->mVel;
    pParticle->mLifetime = glm::max( 0.0f, pParticle->mLifetime - static_cast<float32_t>(pDT) );
}


void particle_t::update_hit( particle_t* pParticle, const float64_t pDT ) {
    particle_t::update_undefined( pParticle, pDT );
}


void particle_t::render_undefined( const particle_t* pParticle ) {
    
}


void particle_t::render_hit( const particle_t* pParticle ) {
    const static float32_t csRHeightVelRatio = 1.0f / 8.0f;
    const static float32_t csRWidthHeightRatio = 1.0f / 4.0f;

    const vec2f32_t cHeightVec = csRHeightVelRatio * pParticle->mVel;
    const vec2f32_t cWidthVec = csRWidthHeightRatio *
        glm::rotate( cHeightVec, -glm::half_pi<float32_t>() );

    if( pParticle->valid() ) {
        vec2f32_t heightPoss[2], widthPoss[2];
        for( uint32_t posIdx = 0; posIdx < 2; posIdx++ ) {
            float32_t posDir = ( posIdx == 0 ) ? 1.0 : -1.0f;
            heightPoss[posIdx] = pParticle->mPos + posDir * cHeightVec;
            widthPoss[posIdx] = pParticle->mPos + posDir * cWidthVec;
        }

        glBegin( GL_TRIANGLE_STRIP ); {
            glColor4ubv( (uint8_t*)&ssn::color::TEAM[ssn::team::neutral] );
            glVertex2fv( VECTOR_AT(heightPoss[0], 0) );
            glVertex2fv( VECTOR_AT(widthPoss[0], 0) );
            glVertex2fv( VECTOR_AT(widthPoss[1], 0) );
            glVertex2fv( VECTOR_AT(heightPoss[1], 0) );
        } glEnd();
    }
}

/// 'ssn::particulator_t' Functions ///

particulator_t::particulator_t( llce::rng_t* const pRNG ) : mRNG( pRNG ) {
    
}


void particulator_t::update( const float64_t pDT ) {
    for( uint32_t partIdx = 0; partIdx < mParticles.size(); partIdx++ ) {
        particle_t& particle = mParticles.front( partIdx );
        particle.update( pDT );
    } while( !mParticles.front(0).valid() && !mParticles.empty() ) {
        mParticles.pop_front();
    }
}


void particulator_t::render() const {
    for( uint32_t partIdx = 0; partIdx < mParticles.size(); partIdx++ ) {
        const particle_t& particle = mParticles.front( partIdx );
        particle.render();
    }
}


void particulator_t::generate( const vec2f32_t& pSource, const vec2f32_t& pDir ) {
    const static uint32_t csAverageCount = 3;
    const static uint32_t csMaxDeviation = 2;

    const uint32_t cNewCount = csAverageCount;
    const uint32_t cAvailCount = glm::min( cNewCount,
        particulator_t::MAX_PARTICLE_COUNT - static_cast<uint32_t>(mParticles.size()) );
    LLCE_CHECK_WARNING( cNewCount == cAvailCount,
        "Couldn't generate all particles due to insufficient space; " <<
        "using all " << cAvailCount << " available particles." );

    const static float32_t csThetaRange = glm::pi<float32_t>() / 4.0f;
    const llce::interval_t cThetaInt(
        glm::orientedAngle(vec2f32_t(1.0f, 0.0f), glm::normalize(pDir)), csThetaRange,
        llce::interval_t::anchor_e::avg );

    const static float32_t csOffsetRange = 1.0e-3f;
    const llce::interval_t cOffsetInt(
        5.0e-2f * glm::length(pDir), csOffsetRange,
        llce::interval_t::anchor_e::avg );

    const static float32_t csSpeedRange = 5.0e-2f;
    const llce::interval_t cSpeedInt(
        5.0e-1f * glm::length(pDir), csSpeedRange,
        llce::interval_t::anchor_e::avg );

    for( uint32_t partIdx = 0; partIdx < cAvailCount; partIdx++ ) {
        float32_t partTheta = cThetaInt.interp( mRNG->nextf() );
        float32_t partOffset = cOffsetInt.interp( mRNG->nextf() );
        float32_t partSpeed = cSpeedInt.interp( mRNG->nextf() );
        vec2f32_t partDir( glm::cos(partTheta), glm::sin(partTheta) );

        mParticles.push_back( particle_t(
            particle_t::type_e::hit,         // particle type
            ssn::MAX_HIT_TIME,               // lifetime
            pSource + partOffset * partDir,  // position
            partSpeed * partDir              // velocity
        ) );
    }
}

}
