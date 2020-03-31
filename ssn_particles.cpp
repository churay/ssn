#include <SDL2/SDL_opengl.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "gfx.h"
#include "interval_t.h"
#include "ssn_consts.h"

#include "ssn_particles.h"

namespace ssn {

/// 'ssn::particle_t' Constants ///

typedef void (*particle_update_f)( particle_t* pParticle, const float64_t pDT );
constexpr static particle_update_f PARTICLE_UPDATE_FUNS[] = {
    particle_t::update_undefined,
    particle_t::update_hit,
    particle_t::update_trail
};

typedef void (*particle_render_f)( const particle_t* pParticle );
constexpr static particle_render_f PARTICLE_RENDER_FUNS[] = {
    particle_t::render_undefined,
    particle_t::render_hit,
    particle_t::render_trail
};

/// 'ssn::particle_t' Functions ///

particle_t::particle_t() :
        mType( particle_t::type_e::undefined ), mLifetime( 0.0f ),
        mBasisX( 0.0f, 0.0f ), mBasisY( 0.0f, 0.0f ),
        mPos( 0.0f, 0.0f ), mVel( 0.0f, 0.0f ), mAccel( 0.0f, 0.0f ) {
    
}


particle_t::particle_t( const type_e pType, const float32_t pLifetime,
            const vec2f32_t& pBasisX, const vec2f32_t& pBasisY,
            const vec2f32_t& pPos, const vec2f32_t& pVel, const vec2f32_t& pAccel ) :
        mType( pType ), mLifetime( pLifetime ),
        mBasisX( pBasisX ), mBasisY( pBasisY ),
        mPos( pPos ), mVel( pVel ), mAccel( pAccel ) {
    
}


void particle_t::update( const float64_t pDT ) {
    PARTICLE_UPDATE_FUNS[mType]( this, pDT );
}


void particle_t::render() const {
    if( this->valid() ) {
        llce::gfx::render_context_t particleRC( mPos, mBasisX, mBasisY, &ssn::color::ERROR );
        PARTICLE_RENDER_FUNS[mType]( this );
    }
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


void particle_t::update_trail( particle_t* pParticle, const float64_t pDT ) {
    particle_t::update_undefined( pParticle, pDT );
}


void particle_t::render_undefined( const particle_t* pParticle ) {
    
}


void particle_t::render_hit( const particle_t* pParticle ) {
    const static float32_t csWidthHeightRatio = 2.5e-1f;
    glBegin( GL_TRIANGLE_STRIP ); {
        glColor4ubv( (uint8_t*)&ssn::color::TEAM[ssn::team::neutral] );
        glVertex2f( 0.5f + 0.0f, 1.0f );
        glVertex2f( 0.5f + 0.5f * csWidthHeightRatio, 0.5f );
        glVertex2f( 0.5f - 0.5f * csWidthHeightRatio, 0.5f );
        glVertex2f( 0.5f + 0.0f, 0.0f );
    } glEnd();
}


void particle_t::render_trail( const particle_t* pParticle ) {
    glBegin( GL_TRIANGLE_STRIP ); {
        glColor4ubv( (uint8_t*)&ssn::color::TEAM[ssn::team::neutral] );
        glVertex2f( 0.5f, 1.0f );
        glVertex2f( 1.0f, 0.5f );
        glVertex2f( 0.0f, 0.5f );
        glVertex2f( 0.5f, 0.0f );
    } glEnd();
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


void particulator_t::generate_hit( const vec2f32_t& pSource, const vec2f32_t& pDir, const float32_t& pSize ) {
    const static uint32_t csMaxPartCount = 3;
    const uint32_t cPartCount = allocate_particles( csMaxPartCount );

    const static float32_t csThetaRange = glm::pi<float32_t>() / 4.0f;
    const llce::interval_t cThetaInt(
        glm::orientedAngle(vec2f32_t(1.0f, 0.0f), glm::normalize(pDir)), csThetaRange,
        llce::interval_t::anchor_e::avg );

    // const static float32_t csOffsetRange = 2.5e-1f, csOffsetMax = 3.5e-1f;
    // const llce::interval_t cOffsetInt(
    //     pSize * csOffsetMin, pSize * csOffsetMax,
    //     llce::interval_t::anchor_e::ext );
    const llce::interval_t cOffsetInt( 6.5e-1f * pSize );

    // const static float32_t csSpeedRange = 5.0e-2f;
    // const llce::interval_t cSpeedInt(
    //     5.0e-1f * glm::length(pDir), csSpeedRange,
    //     llce::interval_t::anchor_e::avg );
    const llce::interval_t cSpeedInt( 5.0e-1f * glm::length(pDir) );

    for( uint32_t partIdx = 0; partIdx < cPartCount; partIdx++ ) {
        float32_t partFrac = ( partIdx + 0.0f ) / ( cPartCount - 1.0f );
        float32_t partTheta = cThetaInt.interp( partFrac );
        float32_t partOffset = cOffsetInt.interp( partFrac );
        float32_t partSpeed = cSpeedInt.interp( partFrac );
        vec2f32_t partDir( glm::cos(partTheta), glm::sin(partTheta) );

        vec2f32_t partBasisY = pSize * partDir;
        vec2f32_t partBasisX = glm::rotate( partBasisY, -glm::half_pi<float32_t>() );
        vec2f32_t partPos = pSource + partOffset * partDir +
            ( -0.5f * partBasisX ) + ( -0.5f * partBasisY );

        mParticles.push_back( particle_t(
            particle_t::type_e::hit, // particle type
            ssn::MAX_HIT_TIME,       // lifetime
            partBasisX,              // basis X
            partBasisY,              // basis Y
            partPos,                 // position
            partSpeed * partDir      // velocity
        ) );
    }
}


void particulator_t::generate_trail( const vec2f32_t& pSource, const vec2f32_t& pDir, const float32_t& pSize ) {
    const static uint32_t csMaxPartCount = 12;
    const uint32_t cPartCount = allocate_particles( csMaxPartCount );

    const vec2f32_t pTrailU = 3.0f * pSize * glm::normalize( pDir );
    const vec2f32_t pTrailV = pSize * glm::rotate(
        glm::normalize(pTrailU), glm::half_pi<float32_t>() );

    const llce::interval_t cUInt( 0, glm::length(pTrailU) );
    const llce::interval_t cVInt( 0, pSize, llce::interval_t::anchor_e::avg ); // glm::length(pTrailV) );

    for( uint32_t partIdx = 0; partIdx < cPartCount; partIdx++ ) {
        float32_t partFrac = ( partIdx + 0.0f ) / ( cPartCount - 1.0f );
        float32_t partU = cUInt.interp( partFrac );
        float32_t partV = cVInt.interp( 0.5f ); // mRNG->nextf() );

        vec2f32_t partBasisY = ( pSize / (csMaxPartCount + 0.0f) ) *
            glm::normalize( pTrailV );
        vec2f32_t partBasisX = glm::rotate( partBasisY, -glm::half_pi<float32_t>() );
        vec2f32_t partPos = pSource + 
            partU * glm::normalize( pTrailU ) +
            partV * glm::normalize( pTrailV ) +
            ( -0.5f * partBasisX ) + ( -0.5f * partBasisY );

        mParticles.push_back( particle_t(
            particle_t::type_e::trail, // particle type
            ssn::MAX_HIT_TIME,       // lifetime
            partBasisX,                // basis X
            partBasisY,                // basis Y
            partPos,                   // position
            vec2f32_t(0.0f, 0.0f)      // velocity
        ) );
    }
}


uint32_t particulator_t::allocate_particles( const uint32_t pParticleCount ) {
    const uint32_t cActualCount = glm::min( pParticleCount,
        particulator_t::MAX_PARTICLE_COUNT - static_cast<uint32_t>(mParticles.size()) );
    LLCE_CHECK_WARNING( pParticleCount == cActualCount,
        "Couldn't generate all requested particles due to insufficient space; " <<
        "using all " << cActualCount << " available particles." );
    return cActualCount;
}


}
