#ifndef SSN_ENTITIES_T_H
#define SSN_ENTITIES_T_H

#include <glm/common.hpp>

#include "ssn_entity_t.h"
#include "circle_t.h"

#include "ssn_data.h"
#include "ssn_consts.h"
#include "consts.h"

namespace ssn {

class team_entity_t : public entity_t {
    public:

    /// Constructors ///

    team_entity_t( const llce::box_t& pBBox, const team::team_e& pTeam );
    team_entity_t( const llce::circle_t& pBounds, const team::team_e& pTeam );

    /// Class Functions ///

    void change( const team::team_e& pTeam );

    /// Class Fields ///

    public:

    uint8_t mTeam;
};


class bounds_t : public entity_t {
    public:

    /// Class Attributes ///

    constexpr static uint32_t AREA_CORNER_COUNT = 3;
    constexpr static uint32_t AREA_MAX_COUNT = 100;

    constexpr static float32_t CORNER_RATIO = 3.0e-2f;   // units: corner size / bound size

    /// Constructors ///

    bounds_t( const llce::box_t& pBBox );

    /// Class Functions ///

    void render() const;

    void claim( const team_entity_t* pSource );

    /// Class Fields ///

    public:

    vec2f32_t mCurrAreaCorners[AREA_CORNER_COUNT];
    uint8_t mCurrAreaTeam;
    uint32_t mCurrAreaCount;
    vec2f32_t mAreaCorners[AREA_MAX_COUNT * AREA_CORNER_COUNT];
    uint8_t mAreaTeams[AREA_MAX_COUNT];
    uint32_t mAreaCount;
};


class paddle_t : public team_entity_t {
    public:

    /// Class Attributes ///

    constexpr static float32_t MOVE_ACCEL = 1.2e0f;     // units: world / second**2
    constexpr static float32_t MOVE_MAX_VEL = 1.0e0f;   // units: world / second

    constexpr static float32_t RUSH_VEL = 3.0e0f;       // units: world / second
    constexpr static float32_t RUSH_DURATION = 5.0e-2f; // units: second
    constexpr static float32_t RUSH_COOLDOWN = 5.0e-1f; // units: second

    /// Constructors ///

    paddle_t( const llce::circle_t& pBounds, const team::team_e& pTeam, const entity_t* pContainer );

    /// Class Functions ///

    void update( const float64_t pDT );
    void render() const;

    void move( const int32_t pDX, const int32_t pDY );
    void rush();

    /// Class Fields ///

    public:

    const entity_t* mContainer;
    vec2f32_t mDI;
    bool32_t mAmRushing;
    vec2f32_t mRushDir;
    float32_t mRushDuration;
    float32_t mRushCooldown;
};


class puck_t : public team_entity_t {
    public:

    /// Class Attributes ///

    constexpr static uint32_t BBOX_COUNT = 4;
    constexpr static uint32_t BBOX_BASE_ID = 0, BBOX_XWRAP_ID = 1, BBOX_YWRAP_ID = 2, BBOX_XYWRAP_ID = 3;

    constexpr static float32_t MAX_VEL = 5.0e0f;        // units: world / second
    constexpr static float32_t MIN_VEL = 5.0e-1f;       // units: world / second
    constexpr static float32_t VEL_MULTIPLIER = 1.1f;   // units: new velocity / old velocity

    constexpr static float32_t CURSOR_RATIO = 5.0e-1f;  // units: cursor radius / puck radius

    /// Constructors ///

    puck_t( const llce::circle_t& pBounds, const team::team_e& pTeam, const entity_t* pContainer );

    /// Class Functions ///

    void update( const float64_t pDT );
    void render() const;

    bool32_t hit( const team_entity_t* pSource );

    vec2i8_t tangible( const vec2i8_t& pWrapCount ) const;

    /// Class Fields ///

    public:

    const entity_t* mContainer;
    vec2i8_t mWrapCount;
    llce::box_t mBBoxes[BBOX_COUNT];
    vec2i8_t mWrapCounts[BBOX_COUNT];
};

}

#endif
