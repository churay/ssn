#include <limits>

#include <SDL2/SDL_opengl.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include <glm/ext/matrix_transform.hpp>

#include "box_t.h"
#include "gfx.h"
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
    llce::gfx::render_context_t baseRC( mBBox, mColor );
    baseRC.render();

    glBegin( GL_TRIANGLES );
    for( uint32_t areaIdx = 0; areaIdx < mAreaCount; areaIdx++ ) {
        glColor4ubv( (uint8_t*)&ssn::color::TEAM[mAreaTeams[areaIdx]] );
        for( uint32_t cornerIdx = 0; cornerIdx < AREA_CORNER_COUNT; cornerIdx++ ) {
            uint32_t areaCornerIdx = areaIdx * AREA_CORNER_COUNT + cornerIdx;
            glVertex2fv( (float32_t*)&mAreaCorners[areaCornerIdx] );
        }
    }
    glEnd();

    const float32_t cCornerRadius = mBBox.xbounds().length() * bounds_t::CORNER_RATIO;
    for( uint32_t cornerIdx = 0; cornerIdx < mCurrAreaCount; cornerIdx++ ) {
        llce::circle_t cornerCircle( mCurrAreaCorners[cornerIdx], cCornerRadius );
        llce::gfx::circle::render( cornerCircle, &ssn::color::TEAM[mCurrAreaTeam] );
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
        team_entity_t( pBounds, pTeam ), mContainer( pContainer ), mDI( 0, 0 ) {
    
}


void paddle_t::update( const float64_t pDT ) {
    entity_t::update( pDT );

    const llce::box_t& oBBox = mContainer->mBBox;
    if( !oBBox.contains(mBBox) ) {
        vec2f32_t containVec(
            oBBox.xbounds().contains(mBBox.xbounds()) + 0.0f,
            oBBox.ybounds().contains(mBBox.ybounds()) + 0.0f );
        mVel *= containVec;
        mAccel *= containVec;

        mBBox.embed( oBBox );
        mBounds.mCenter = mBBox.center();
    }
}


void paddle_t::move( const int32_t pDX, const int32_t pDY ) {
    mDI.x = static_cast<float32_t>( glm::clamp(pDX, -1, 1) );
    mDI.y = static_cast<float32_t>( glm::clamp(pDY, -1, 1) );
    mVel = ( pDX || pDY ) ? paddle_t::MAX_VEL * glm::normalize( mDI ) : vec2f32_t( 0.0f, 0.0f );
}

/// 'ssn::puck_t' Functions ///

puck_t::puck_t( const llce::circle_t& pBounds, const team::team_e& pTeam, const entity_t* pContainer ) :
        team_entity_t( pBounds, pTeam ), mWrapCount( 2, 2 ), mContainer( pContainer ) {
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
        mBounds.mCenter = mBBox.center();
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
                boundsBox.min().x, pFocusBox.center().y - cursorRadius / 2.0f,
                boundsBox.xbounds().length(), cursorRadius ) :
            llce::box_t(
                pFocusBox.center().x - cursorRadius / 2.0f, boundsBox.min().y,
                cursorRadius, boundsBox.ybounds().length() );

        llce::gfx::render_context_t cursorRC( cursorBox, &cursorColor );
        cursorRC.render();
    };

    csRenderCursor( this, mBBoxes[puck_t::BBOX_BASE_ID], puck_t::BBOX_XWRAP_ID );
    csRenderCursor( this, mBBoxes[puck_t::BBOX_BASE_ID], puck_t::BBOX_YWRAP_ID );
    if( !mBBoxes[puck_t::BBOX_XWRAP_ID].empty() ) {
        csRenderCursor( this, mBBoxes[puck_t::BBOX_XWRAP_ID], puck_t::BBOX_YWRAP_ID );
    } if( !mBBoxes[puck_t::BBOX_YWRAP_ID].empty() ) {
        csRenderCursor( this, mBBoxes[puck_t::BBOX_YWRAP_ID], puck_t::BBOX_XWRAP_ID );
    }

    for( uint32_t bboxIdx = 0; bboxIdx < puck_t::BBOX_COUNT; bboxIdx++ ) {
        const llce::box_t& puckBBox = mBBoxes[bboxIdx];
        const vec2i8_t& puckWrapCount = mWrapCounts[bboxIdx];
        if( !puckBBox.empty() ) {
            const vec2i8_t cPuckTangible = tangible( puckWrapCount );
            const uint32_t cSegmentCount = 20;
            const color4u8_t cOutlineColor = { 0x00, 0x00, 0x00, 0xFF };
            const float32_t cInnerRadius = 0.875f;

            // TODO(JRC): Abstract out the logic below that's held in common with
            // the 'gfx::circle::render' function.
            {
                llce::gfx::render_context_t entityRC( puckBBox, mColor );

                glPushMatrix();
                glm::mat4 matCircleSpace( 1.0f );
                matCircleSpace *= glm::translate( glm::mat4(1.0f), vec3f32_t(0.5f, 0.5f, 0.0f) );
                matCircleSpace *= glm::scale( glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 1.0f) );
                glMultMatrixf( &matCircleSpace[0][0] );

                glPushAttrib( GL_CURRENT_BIT );
                glColor4ubv( (uint8_t*)&cOutlineColor );
                glBegin( GL_POLYGON );
                for( uint32_t segmentIdx = 0; segmentIdx < cSegmentCount; segmentIdx++ ) {
                    float32_t segmentRadians = 2.0f * M_PI * ( segmentIdx / (cSegmentCount + 0.0f) );
                    glVertex2f( std::cos(segmentRadians), std::sin(segmentRadians) );
                }
                glEnd();

                for( int8_t team = ssn::team::left; team <= ssn::team::right; team++ ) {
                    const color4u8_t* teamColor = *VECTOR_AT( cPuckTangible, team ) ?
                        &ssn::color::TEAM[team] : &ssn::color::TEAM[ssn::team::neutral];
                    const float32_t teamOffset = ( (team == ssn::team::left) ? 1.0f : -1.0f ) * ( M_PI / 2.0f );

                    glColor4ubv( (uint8_t*)teamColor );
                    glBegin( GL_POLYGON );
                    for( uint32_t segmentIdx = 0; segmentIdx <= cSegmentCount; segmentIdx++ ) {
                        float32_t segmentRadians = M_PI * ( segmentIdx / (cSegmentCount + 0.0f) ) + teamOffset;
                        glVertex2f( cInnerRadius * std::cos(segmentRadians), cInnerRadius * std::sin(segmentRadians) );
                    }
                    glEnd();
                }

                glPopAttrib();
                glPopMatrix();
            }
        }
    }
}


bool32_t puck_t::hit( const team_entity_t* pSource ) {
    for( uint32_t bboxIdx = 0; bboxIdx < puck_t::BBOX_COUNT; bboxIdx++ ) {
        const llce::box_t& puckBBox = mBBoxes[bboxIdx];
        const vec2i8_t& puckWrapCount = mWrapCounts[bboxIdx];

        const vec2i8_t puckTangible = tangible( puckWrapCount );
        if( !puckBBox.empty() && *VECTOR_AT(puckTangible, pSource->mTeam) ) {
            llce::circle_t puckBounds( puckBBox.center(), mBounds.mRadius );
            if( pSource->mBounds.overlaps(puckBounds) ) {
                puckBounds.exbed( pSource->mBounds );

                vec2f32_t hitVec = puckBounds.mCenter - puckBBox.center();
                vec2f32_t hitDir = glm::normalize( hitVec );
                // FIXME(JRC): The unnecessary multiplication here acts as
                // a workaround to weird interactions between gcc compilation
                // and raw 'constexpr static' variables.
                // See: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=50785
                float32_t hitMag = std::max( (1.0f*puck_t::MIN_VEL), glm::length(pSource->mVel) );

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
