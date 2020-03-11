#ifndef SSN_PARTICLES_T_H
#define SSN_PARTICLES_T_H

#include "box_t.h"
#include "circle_t.h"

#include <glm/common.hpp>

#include "ssn_data.h"
#include "consts.h"

namespace ssn {

class particle_t {
    public:

    /// Constructors ///

    particle_t();
    particle_t( const vec2f32_t& pPos, const color4u8_t* pColor, const float32_t pLifetime );

    /// Class Functions ///

    void update( const float64_t pDT );
    void render() const;

    bool32_t valid() const;

    /// Class Fields ///

    public:

    vec2f32_t mPos; // units: world
    vec2f32_t mVel; // units: world / second
    vec2f32_t mAccel; // units: world / second**2
    const color4u8_t* mColor; // units: (r,g,b,a)
    float32_t mLifetime; // units: second
};


class particulator_t {
    public:

    /// Class Attributes ///

    constexpr static uint32_t MAX_PARTICLE_COUNT = 32;

    /// Constructors ///

    particulator_t();

    /// Class Functions ///

    void update( const float64_t pDT );
    void render() const;

    void generate( const vec2f32_t& pSource, const vec2f32_t& pDir );

    /// Helper Functions ///

    private:

    void push();
    void pop();
    void peek();

    /// Class Fields ///

    public:

    particle_t mParticles[MAX_PARTICLE_COUNT];
    uint32_t mSize;
};

}

#endif
