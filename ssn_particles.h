#ifndef SSN_PARTICLES_T_H
#define SSN_PARTICLES_T_H

#include <glm/common.hpp>

#include "box_t.h"
#include "circle_t.h"
#include "rng_t.h"

#include "deque.hpp"

#include "ssn_data.h"
#include "consts.h"

namespace ssn {

class particle_t {
    public:

    /// Class Attributes ///

    enum type_e { undefined = 0, hit, trail };

    /// Constructors ///

    particle_t();
    particle_t( const type_e pType, const float32_t pLifetime,
        const vec2f32_t& pBasisX, const vec2f32_t& pBasisY,
        const vec2f32_t& pPos = vec2f32_t(0.0f, 0.0f),
        const vec2f32_t& pVel = vec2f32_t(0.0f, 0.0f),
        const vec2f32_t& mAccel = vec2f32_t(0.0f, 0.0f) );

    /// Class Functions ///

    void update( const float64_t pDT );
    void render() const;

    bool32_t valid() const;

    /// Class Fields ///

    public:

    type_e mType;
    float32_t mLifetime; // units: seconds

    vec2f32_t mBasisX; // units: world
    vec2f32_t mBasisY; // units: world

    vec2f32_t mPos; // units: world
    vec2f32_t mVel; // units: world / second
    vec2f32_t mAccel; // units: world / second**2

    /// Type Functions ///

    // FIXME(JRC): This functions as a reasonable workaround to inheritance
    // w/ different 'render' overrides, but it isn't perfect because 'type_e'
    // and these functions need to be in sync.
    static void update_undefined( particle_t* pParticle, const float64_t pDT );
    static void update_hit( particle_t* pParticle, const float64_t pDT );
    static void update_trail( particle_t* pParticle, const float64_t pDT );

    static void render_undefined( const particle_t* pParticle );
    static void render_hit( const particle_t* pHit );
    static void render_trail( const particle_t* pHit );
};


class particulator_t {
    public:

    /// Class Attributes ///

    constexpr static uint32_t MAX_PARTICLE_COUNT = 32;

    /// Constructors ///

    particulator_t( llce::rng_t* pRNG );

    /// Class Functions ///

    void update( const float64_t pDT );
    void render() const;

    void generate_hit( const vec2f32_t& pSource, const vec2f32_t& pDir, const float32_t& pSize );
    void generate_trail( const vec2f32_t& pSource, const vec2f32_t& pDir, const float32_t& pSize );

    /// Helper Functions ///

    private:

    uint32_t allocate_particles( const uint32_t pParticleCount );

    /// Class Fields ///

    public:

    llce::rng_t* mRNG;
    llce::deque<particle_t, MAX_PARTICLE_COUNT> mParticles;
};

}

#endif
