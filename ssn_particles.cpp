#include <SDL2/SDL_opengl.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "interval_t.h"
#include "gfx.h"
#include "geom.h"
#include "ssn_consts.h"

#include "ssn_particles.h"

namespace ssn {

/// 'ssn::particle_t' Constants ///

typedef void (*particle_update_f)( particle_t* pParticle, const float64_t pDT );
constexpr static particle_update_f PARTICLE_UPDATE_FUNS[] = {
    particle_t::updateUndefined,
    particle_t::updateHit,
    particle_t::updateTrail
};

typedef void (*particle_render_f)( const particle_t* pParticle );
constexpr static particle_render_f PARTICLE_RENDER_FUNS[] = {
    particle_t::renderUndefined,
    particle_t::renderHit,
    particle_t::renderTrail
};

static_assert( ARRAY_LEN(PARTICLE_UPDATE_FUNS) == ssn::particle_t::type_e::_length,
    "Incorrect number of particle update functions; "
    "please add all 'ssn::particle_t::update_*' functions to the "
    "'PARTICLE_UPDATE_FUNS' list in 'particles.cpp'." );
static_assert( ARRAY_LEN(PARTICLE_RENDER_FUNS) == ssn::particle_t::type_e::_length,
    "Incorrect number of particle render functions; "
    "please add all 'ssn::particle_t::render_*' functions to the "
    "'PARTICLE_RENDER_FUNS' list in 'particles.cpp'." );

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
    if( this->valid() && !this->empty() ) {
        llce::gfx::render_context_t particleRC( mPos, mBasisX, mBasisY );
        PARTICLE_RENDER_FUNS[mType]( this );
    }
}


bool32_t particle_t::valid() const {
    return mLifetime > 0.0f &&
        std::isfinite( mBasisX.x ) && std::isfinite( mBasisX.y ) &&
        std::isfinite( mBasisY.x ) && std::isfinite( mBasisY.y );
}


bool32_t particle_t::empty() const {
    return glm::length( mBasisX ) < glm::epsilon<float32_t>() ||
        glm::length( mBasisY ) < glm::epsilon<float32_t>();
}

/// 'ssn::particle_t' Type Functions ///

void particle_t::updateUndefined( particle_t* pParticle, const float64_t pDT ) {
    pParticle->mVel += static_cast<float32_t>( pDT ) * pParticle->mAccel;
    pParticle->mPos += static_cast<float32_t>( pDT ) * pParticle->mVel;
    pParticle->mLifetime = glm::max( 0.0f, pParticle->mLifetime - static_cast<float32_t>(pDT) );
}


void particle_t::updateHit( particle_t* pParticle, const float64_t pDT ) {
    particle_t::updateUndefined( pParticle, pDT );
}


void particle_t::updateTrail( particle_t* pParticle, const float64_t pDT ) {
    particle_t::updateUndefined( pParticle, pDT );
}


void particle_t::renderUndefined( const particle_t* pParticle ) {
    
}


void particle_t::renderHit( const particle_t* pParticle ) {
    const static float32_t csWidthHeightRatio = 2.5e-1f;
    glBegin( GL_TRIANGLE_STRIP ); {
        glColor4ubv( (uint8_t*)&ssn::color::TEAM[ssn::team::neutral] );
        glVertex2f( 0.5f + 0.0f, 1.0f );
        glVertex2f( 0.5f + 0.5f * csWidthHeightRatio, 0.5f );
        glVertex2f( 0.5f - 0.5f * csWidthHeightRatio, 0.5f );
        glVertex2f( 0.5f + 0.0f, 0.0f );
    } glEnd();
}


void particle_t::renderTrail( const particle_t* pParticle ) {
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


void particulator_t::genHit( const vec2f32_t& pSource, const vec2f32_t& pDir, const float32_t& pSize ) {
    const static uint32_t csMaxPartCount = 3;
    const uint32_t cPartCount = allocParticles( csMaxPartCount );

    const static float32_t csThetaRange = glm::pi<float32_t>() / 4.0f;
    const llce::interval_t cThetaInt(
        glm::orientedAngle(vec2f32_t(1.0f, 0.0f), glm::normalize(pDir)), csThetaRange,
        llce::geom::anchor1D::mid );

    // const static float32_t csOffsetRange = 2.5e-1f, csOffsetMax = 3.5e-1f;
    // const llce::interval_t cOffsetInt(
    //     pSize * csOffsetMin, pSize * csOffsetMax,
    //     llce::geom::anchor1D::lo );
    const llce::interval_t cOffsetInt( 6.5e-1f * pSize );

    // const static float32_t csSpeedRange = 5.0e-2f;
    // const llce::interval_t cSpeedInt(
    //     5.0e-1f * glm::length(pDir), csSpeedRange,
    //     llce::geom::anchor1D::mid );
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
            ssn::HIT_DURATION,       // lifetime
            partBasisX,              // basis X
            partBasisY,              // basis Y
            partPos,                 // position
            partSpeed * partDir      // velocity
        ) );
    }
}


void particulator_t::genTrail( const vec2f32_t& pSource, const vec2f32_t& pDir, const float32_t& pSize ) {
    const static uint32_t csMaxPartCount = 12;
    const uint32_t cPartCount = allocParticles( csMaxPartCount );

    const vec2f32_t pTrailU = 3.0f * pSize * glm::normalize( pDir );
    const vec2f32_t pTrailV = pSize * glm::rotate(
        glm::normalize(pTrailU), glm::half_pi<float32_t>() );

    const llce::interval_t cUInt( 0, glm::length(pTrailU) );
    const llce::interval_t cVInt( 0, pSize, llce::geom::anchor1D::mid ); // glm::length(pTrailV) );

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
            ssn::HIT_DURATION,         // lifetime
            partBasisX,                // basis X
            partBasisY,                // basis Y
            partPos,                   // position
            vec2f32_t(0.0f, 0.0f)      // velocity
        ) );
    }
}


uint32_t particulator_t::allocParticles( const uint32_t pParticleCount ) {
    const uint32_t cActualCount = glm::min( pParticleCount,
        particulator_t::MAX_PARTICLE_COUNT - static_cast<uint32_t>(mParticles.size()) );
    LLCE_CHECK_WARNING( pParticleCount == cActualCount,
        "Couldn't generate all requested particles due to insufficient space; " <<
        "using all " << cActualCount << " available particles." );
    return cActualCount;
}


}
