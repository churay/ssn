#include <limits>

#include <SDL2/SDL_opengl.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "box_t.h"
#include "gfx.h"
#include "util.hpp"
#include "ssn_entities.h"

namespace ssn {

/// 'ssn::team_entity_t' Functions ///

team_entity_t::team_entity_t( const llce::box_t& pBBox, const team::team_e& pTeam ) :
        entity_t( pBBox, &ssn::color::TEAM[pTeam] ), mTeam( pTeam ) {
    
}


team_entity_t::team_entity_t( const llce::circle_t& pBounds, const team::team_e& pTeam ) :
        entity_t( pBounds, &ssn::color::TEAM[pTeam] ), mTeam( pTeam ) {
    
}


void team_entity_t::change( const team::team_e& pTeam ) {
    mColor = &ssn::color::TEAM[(mTeam = pTeam)];
}

/// 'ssn::bounds_t' Functions ///

bounds_t::bounds_t( const llce::box_t& pBBox ) :
        entity_t( pBBox, &ssn::color::BACKGROUND ) {
    mCurrAreaTeam = ssn::team::neutral;
    mCurrAreaCount = mAreaCount = 0;
}


void bounds_t::render() const {
    // TODO(JRC): The 'bounds_t' annotations aren't rendered relative to the
    // 'bounds_t' bounding space, which means they can interfere with graphics
    // displayed outside of this space. Abstractly speaking, this makes sense
    // because the annotations are relative to the world and not to the 'bounds_t'
    // container object, though it may lead to trouble should the world be changed
    // to have a different reference frame at some point in the future.
    llce::gfx::color_context_t entityCC( mColor );
    llce::gfx::render::box( mBBox );

    glBegin( GL_TRIANGLES );
    for( uint32_t areaIdx = 0; areaIdx < mAreaCount; areaIdx++ ) {
        entityCC.update( &ssn::color::TEAM[mAreaTeams[areaIdx]] );
        for( uint32_t cornerIdx = 0; cornerIdx < AREA_CORNER_COUNT; cornerIdx++ ) {
            uint32_t areaCornerIdx = areaIdx * AREA_CORNER_COUNT + cornerIdx;
            glVertex2fv( (float32_t*)&mAreaCorners[areaCornerIdx] );
        }
    }
    glEnd();

    for( uint32_t cornerIdx = 0; cornerIdx < mCurrAreaCount; cornerIdx++ ) {
        entityCC.update( &ssn::color::INTERFACE );
        llce::gfx::render::circle(
            llce::circle_t(mCurrAreaCorners[cornerIdx], bounds_t::CORNER_RATIO) );

        entityCC.update( &ssn::color::TEAM[mCurrAreaTeam] );
        llce::gfx::render::circle(
            llce::circle_t(mCurrAreaCorners[cornerIdx], 0.75f * bounds_t::CORNER_RATIO) );
    }
}


void bounds_t::claim( const team_entity_t* pSource ) {
    if( pSource->mTeam != mCurrAreaTeam ) {
        mCurrAreaTeam = pSource->mTeam;
        mCurrAreaCount = 0;
    }

    mCurrAreaCorners[mCurrAreaCount++] = pSource->mBounds.mCenter;
    if( mCurrAreaCount == bounds_t::AREA_CORNER_COUNT ) {
        for( uint32_t cornerIdx = 0; cornerIdx < AREA_CORNER_COUNT; cornerIdx++ ) {
            uint32_t areaCornerIdx = mAreaCount * AREA_CORNER_COUNT + cornerIdx;
            mAreaCorners[areaCornerIdx] = mCurrAreaCorners[cornerIdx];
        }
        mAreaTeams[mAreaCount++] = mCurrAreaTeam;

        mCurrAreaTeam = ssn::team::neutral;
        mCurrAreaCount = 0;
    }
}

/// 'ssn::paddle_t' Functions ///

paddle_t::paddle_t( const llce::circle_t& pBounds, const team::team_e& pTeam, const entity_t* pContainer ) :
        team_entity_t( pBounds, pTeam ), mContainer( pContainer ), mDI( 0, 0 ),
        mAmRushing( false ), mRushDuration( 0.0f ), mRushCooldown( 0.0f ) {
    
}


void paddle_t::update( const float64_t pDT ) {
    { // Update Rush State Information //
        mRushDuration = std::min( mRushDuration + (mAmRushing ? pDT : 0.0), 0.0 + paddle_t::RUSH_DURATION );
        mAmRushing &= mRushDuration < paddle_t::RUSH_DURATION;
        mRushCooldown = std::max( mRushCooldown - (mAmRushing ? 0.0 : pDT), 0.0 );

        mVel = mAmRushing ? paddle_t::RUSH_VEL * mRushDir : mVel;
        mAccel = mAmRushing ? vec2f32_t( 0.0f, 0.0f ) : mAccel;
    }

    // FIXME(JRC): It would be best if this function could directly invoke
    // 'entity_t::update' to remove any guesswork (this isn't currently possible
    // since the velocity needs to be intercepted and capped before it's used
    // for position update calculations).
    { // Update Positions/Velocities //
        mVel += static_cast<float32_t>( pDT ) * mAccel;
        const float32_t cVelMag = glm::length( mVel );
        if( !mAmRushing && cVelMag > paddle_t::MOVE_MAX_VEL ) {
            mVel *= paddle_t::MOVE_MAX_VEL / cVelMag;
        }
        mBounds.mCenter += static_cast<float32_t>( pDT ) * mVel;
        mBBox.mPos += static_cast<float32_t>( pDT ) * mVel;
    }

    { // Resolve Container Intersections //
        const llce::box_t& oBBox = mContainer->mBBox;
        if( !oBBox.contains(mBBox) ) {
            vec2f32_t containVec(
                oBBox.xbounds().contains(mBBox.xbounds()) + 0.0f,
                oBBox.ybounds().contains(mBBox.ybounds()) + 0.0f );
            mVel *= containVec;
            mAccel *= containVec;

            mBBox.embed( oBBox );
            mBounds.mCenter = mBBox.mid();
        }
    }
}


void paddle_t::render() const {
    const static llce::circle_t csPaddleBounds( 0.5f, 0.5f, 1.0f );
    const static llce::circle_t csColorBounds( csPaddleBounds.mCenter, 0.90f * csPaddleBounds.mRadius );

    const float32_t cCooldownPercent = glm::min( 1.0f,
        (paddle_t::RUSH_COOLDOWN - mRushCooldown) / paddle_t::RUSH_COOLDOWN );

    const color4f32_t cColorF32 = llce::gfx::color::u82f32( *mColor );
    const color4f32_t cCooldownColorF32 = llce::gfx::color::saturateRGB(
        cColorF32, (cCooldownPercent >= 1.0f) ? 0.0f : -0.5f );
    const color4u8_t cCooldownColor = llce::gfx::color::f322u8( cCooldownColorF32 );

    llce::gfx::render_context_t entityRC( mBBox );
    llce::gfx::color_context_t entityCC( mColor );

    entityCC.update( &ssn::color::INTERFACE );
    llce::gfx::render::circle( csPaddleBounds );

    entityCC.update( &ssn::color::TEAM[ssn::team::neutral] );
    llce::gfx::render::circle( csColorBounds );

    entityCC.update( &cCooldownColor );
    llce::gfx::render::circle( csColorBounds, glm::half_pi<float32_t>(),
        glm::half_pi<float32_t>() + 2.0f * glm::pi<float32_t>() * cCooldownPercent );
}


void paddle_t::move( const int32_t pDX, const int32_t pDY ) {
    mDI.x = static_cast<float32_t>( glm::clamp(pDX, -1, 1) );
    mDI.y = static_cast<float32_t>( glm::clamp(pDY, -1, 1) );
    mAccel = paddle_t::MOVE_ACCEL * llce::util::normalize( mDI );
}


void paddle_t::rush() {
    if( !mAmRushing && mRushCooldown <= 0.0f ) {
        mAmRushing = true;
        mRushDuration = 0.0f;
        mRushCooldown = paddle_t::RUSH_COOLDOWN;
        mRushDir = llce::util::normalize(
            ( glm::length(mVel) > glm::epsilon<float32_t>() ) ? mVel : mAccel );
    }
}

/// 'ssn::puck_t' Functions ///

puck_t::puck_t( const llce::circle_t& pBounds, const team::team_e& pTeam, const entity_t* pContainer ) :
        team_entity_t( pBounds, pTeam ), mContainer( pContainer ), mWrapCount( 2, 2 )  {
    mBBoxes[puck_t::BBOX_BASE_ID] = mBBox;
    mWrapCounts[puck_t::BBOX_BASE_ID] = mWrapCount;
}


void puck_t::update( const float64_t pDT ) {
    const llce::box_t& oBBox = mContainer->mBBox;

    const vec2i8_t isPrevWrap = {
        !oBBox.xbounds().contains( mBBox.xbounds() ),
        !oBBox.ybounds().contains( mBBox.ybounds() ) };
    entity_t::update( pDT );
    const vec2i8_t isCurrWrap = {
        !oBBox.xbounds().contains( mBBox.xbounds() ),
        !oBBox.ybounds().contains( mBBox.ybounds() ) };

    const vec2i8_t newWrapDir = {
        ( !isPrevWrap.x && isCurrWrap.x ) ? ( (mBBox.mPos.x < oBBox.mPos.x) ? -1 : 1 ) : 0,
        ( !isPrevWrap.y && isCurrWrap.y ) ? ( (mBBox.mPos.y < oBBox.mPos.y) ? -1 : 1 ) : 0 };

    { // Resolve Boundary Wrap //
        mBBox.mPos.x = oBBox.xbounds().wrap( mBBox.mPos.x );
        mBBox.mPos.y = oBBox.ybounds().wrap( mBBox.mPos.y );
        mBounds.mCenter = mBBox.mid();
        mWrapCount += newWrapDir;
    }

    { // Initialize Bounding Boxes //
        mBBoxes[puck_t::BBOX_BASE_ID] = mBBox;
        mWrapCounts[puck_t::BBOX_BASE_ID] = mWrapCount;
        for( uint32_t bboxIdx = puck_t::BBOX_BASE_ID + 1; bboxIdx < puck_t::BBOX_COUNT; bboxIdx++ ) {
            mBBoxes[bboxIdx] = llce::box_t();
            mWrapCounts[bboxIdx] = { 0, 0 };
        }
    }

    { // Update Bounding Boxes //
        if( isCurrWrap.x || isCurrWrap.y ) {
            vec2i8_t& baseWrapCount = mWrapCounts[puck_t::BBOX_BASE_ID];

            if( isCurrWrap.x ) {
                mBBoxes[puck_t::BBOX_XWRAP_ID] = llce::box_t(
                    mBBox.min().x - oBBox.xbounds().length(), // NOTE: see wrap code above
                    mBBoxes[puck_t::BBOX_BASE_ID].mPos.y,
                    mBBoxes[puck_t::BBOX_BASE_ID].mDims.x,
                    mBBoxes[puck_t::BBOX_BASE_ID].mDims.y);
                mWrapCounts[puck_t::BBOX_XWRAP_ID] = ( mWrapCount.x > 0 ) ? mWrapCount : vec2i8_t( mWrapCount.x+1, mWrapCount.y );
                mWrapCounts[puck_t::BBOX_BASE_ID] = ( mWrapCount.x < 0 ) ? mWrapCount : vec2i8_t( baseWrapCount.x-1, baseWrapCount.y );
            } if( isCurrWrap.y ) {
                mBBoxes[puck_t::BBOX_YWRAP_ID] = llce::box_t(
                    mBBoxes[puck_t::BBOX_BASE_ID].mPos.x,
                    mBBox.min().y - oBBox.ybounds().length(), // NOTE: see wrap code above
                    mBBoxes[puck_t::BBOX_BASE_ID].mDims.x,
                    mBBoxes[puck_t::BBOX_BASE_ID].mDims.y);
                mWrapCounts[puck_t::BBOX_YWRAP_ID] = ( mWrapCount.y > 0 ) ? mWrapCount : vec2i8_t( mWrapCount.x, mWrapCount.y+1 );
                mWrapCounts[puck_t::BBOX_BASE_ID] = ( mWrapCount.y < 0 ) ? mWrapCount : vec2i8_t( baseWrapCount.x, baseWrapCount.y-1 );
            } if( isCurrWrap.x && isCurrWrap.y ) {
                mBBoxes[puck_t::BBOX_XYWRAP_ID] = llce::box_t(
                    mBBoxes[puck_t::BBOX_XWRAP_ID].mPos.x,
                    mBBoxes[puck_t::BBOX_YWRAP_ID].mPos.y,
                    mBBoxes[puck_t::BBOX_BASE_ID].mDims.x,
                    mBBoxes[puck_t::BBOX_BASE_ID].mDims.y);
                mWrapCounts[puck_t::BBOX_XYWRAP_ID] = vec2i8_t( mWrapCounts[puck_t::BBOX_XWRAP_ID].x, mWrapCounts[puck_t::BBOX_YWRAP_ID].y );
            }
        }
    }
}


void puck_t::render() const {
    const static auto csRenderCursor = []
            ( const ssn::puck_t* pPuck, const llce::box_t& pFocusBox, const uint32_t pDim ) {
        const llce::box_t& boundsBox = pPuck->mContainer->mBBox; 
        const float32_t cursorRadius = puck_t::CURSOR_RATIO * pPuck->mBounds.mRadius;
        const color4u8_t cursorColor = *pPuck->mColor - color4u8_t{ 0x00, 0x00, 0x00, 0xaa };

        const llce::box_t cursorBox = ( pDim == puck_t::BBOX_XWRAP_ID ) ?
            llce::box_t(
                boundsBox.min().x, pFocusBox.mid().y - cursorRadius / 2.0f,
                boundsBox.xbounds().length(), cursorRadius ) :
            llce::box_t(
                pFocusBox.mid().x - cursorRadius / 2.0f, boundsBox.min().y,
                cursorRadius, boundsBox.ybounds().length() );

        llce::gfx::color_context_t cursorCC( &cursorColor );
        llce::gfx::render::box( cursorBox );
    };

    csRenderCursor( this, mBBoxes[puck_t::BBOX_BASE_ID], puck_t::BBOX_XWRAP_ID );
    csRenderCursor( this, mBBoxes[puck_t::BBOX_BASE_ID], puck_t::BBOX_YWRAP_ID );
    if( !mBBoxes[puck_t::BBOX_XWRAP_ID].empty() ) {
        csRenderCursor( this, mBBoxes[puck_t::BBOX_XWRAP_ID], puck_t::BBOX_YWRAP_ID );
    } if( !mBBoxes[puck_t::BBOX_YWRAP_ID].empty() ) {
        csRenderCursor( this, mBBoxes[puck_t::BBOX_YWRAP_ID], puck_t::BBOX_XWRAP_ID );
    }

    const static llce::circle_t csPuckBounds( 0.5f, 0.5f, 1.0f );
    const static llce::circle_t csSideBounds( csPuckBounds.mCenter, 0.875f * csPuckBounds.mRadius );

    llce::gfx::color_context_t entityCC( mColor );
    for( uint32_t bboxIdx = 0; bboxIdx < puck_t::BBOX_COUNT; bboxIdx++ ) {
        const llce::box_t& puckBBox = mBBoxes[bboxIdx];
        const vec2i8_t& puckWrapCount = mWrapCounts[bboxIdx];
        if( !puckBBox.empty() ) {
            const vec2i8_t puckTangible = tangible( puckWrapCount );

            llce::gfx::render_context_t entityRC( puckBBox );
            entityCC.update( &ssn::color::INTERFACE );
            llce::gfx::render::circle( csPuckBounds );
            for( int8_t side = ssn::team::left; side <= ssn::team::right; side++ ) {
                const float32_t sideAngle = ( M_PI / 2.0f ) + ( side + 0.0f ) * M_PI;
                const color4u8_t* sideColor = *LLCE_VECTOR_AT( puckTangible, side ) ?
                    &ssn::color::TEAM[side] : &ssn::color::TEAM[ssn::team::neutral];

                entityCC.update( sideColor );
                llce::gfx::render::circle( csSideBounds, sideAngle, sideAngle + M_PI );
            }
        }
    }
}


bool32_t puck_t::hit( const team_entity_t* pSource ) {
    for( uint32_t bboxIdx = 0; bboxIdx < puck_t::BBOX_COUNT; bboxIdx++ ) {
        const llce::box_t& puckBBox = mBBoxes[bboxIdx];
        const vec2i8_t& puckWrapCount = mWrapCounts[bboxIdx];

        const vec2i8_t puckTangible = tangible( puckWrapCount );
        if( !puckBBox.empty() && *LLCE_VECTOR_AT(puckTangible, pSource->mTeam) ) {
            llce::circle_t puckBounds( puckBBox.mid(), mBounds.mRadius );
            if( pSource->mBounds.overlaps(puckBounds) ) {
                puckBounds.exbed( pSource->mBounds );

                vec2f32_t hitVec = puckBounds.mCenter - puckBBox.mid();
                vec2f32_t hitDir = llce::util::normalize( hitVec );
                // FIXME(JRC): The unnecessary multiplication here acts as
                // a workaround to weird interactions between gcc compilation
                // and raw 'constexpr static' variables.
                // See: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=50785
                float32_t hitMag = glm::clamp( puck_t::VEL_MULTIPLIER * glm::length(mVel),
                    (1.0f * puck_t::MIN_VEL), (1.0f * puck_t::MAX_VEL) );

                mBounds.mCenter += hitVec;
                mBBox.mPos += hitVec;
                mVel = hitDir * hitMag;

                mWrapCount = { 0, 0 };
                team_entity_t::change( static_cast<ssn::team::team_e>(pSource->mTeam) );

                return true;
            }
        }
    }

    return false;
}

vec2i8_t puck_t::tangible( const vec2i8_t& pWrapCount ) const {
    const uint32_t cWrapNumber = std::max( std::abs(pWrapCount.x), std::abs(pWrapCount.y) );
    return vec2i8_t(
        (bool8_t)(cWrapNumber >= (1 + (int8_t)(mTeam == ssn::team::left))),
        (bool8_t)(cWrapNumber >= (1 + (int8_t)(mTeam == ssn::team::right))) );
}


}
