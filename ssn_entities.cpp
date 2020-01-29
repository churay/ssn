#include <limits>

#include <SDL2/SDL_opengl.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include "box_t.h"
#include "gfx.h"
#include "ssn_entities.h"

namespace ssn {

/// 'ssn::bounds_t' Functions ///

bounds_t::bounds_t( const llce::box_t& pBBox ) :
        entity_t( pBBox, &ssn::color::BACKGROUND ) {
    
}


void bounds_t::render() const {
    llce::gfx::render_context_t baseRC( mBBox, mColor );
    baseRC.render();
}

/// 'ssn::paddle_t' Functions ///

paddle_t::paddle_t( const llce::circle_t& pBounds ) :
        entity_t( pBounds, &ssn::color::PADDLE ), mDI( 0, 0 ) {
    
}


void paddle_t::update( const float64_t pDT ) {
    mAccel = MOVE_ACCEL * mDI;
    entity_t::update( pDT );
}


void paddle_t::move( const int32_t pDX, const int32_t pDY ) {
    mDI.x = static_cast<float32_t>( glm::clamp(pDX, -1, 1) );
    mDI.y = static_cast<float32_t>( glm::clamp(pDY, -1, 1) );
}


void paddle_t::wrap( const entity_t* pContainer ) {
    const llce::box_t& cBBox = pContainer->mBBox;

    if( !cBBox.contains(mBBox) ) {
        vec2f32_t containVec(
            cBBox.xbounds().contains(mBBox.xbounds()) + 0.0f,
            cBBox.ybounds().contains(mBBox.ybounds()) + 0.0f );
        mVel *= containVec;
        mAccel *= containVec;

        mBBox.embed( cBBox );
        mBounds.mCenter = mBBox.center();
    }
}

/// 'ssn::puck_t' Functions ///

puck_t::puck_t( const llce::circle_t& pBounds ) :
        entity_t( pBounds, &ssn::color::PUCK ) {
    mBBoxes[puck_t::BBOX_BASE_ID] = mBBox;
}


void puck_t::update( const float64_t pDT ) {
    llce::circle_t oldBounds = mBounds;
    entity_t::update( pDT );
    mBBoxes[puck_t::BBOX_BASE_ID] = mBBox;

    const vec2f32_t posDelta = mBounds.mCenter - oldBounds.mCenter;
    for( uint32_t bboxIdx = 0; bboxIdx < puck_t::BBOX_COUNT; bboxIdx++ ) {
        llce::box_t& puckBBox = mBBoxes[bboxIdx];
        if( !puckBBox.empty() ) {
            puckBBox.mPos += posDelta;
        }
    }
}


void puck_t::render() const {
    const static llce::circle_t csRenderCircle( vec2f32_t(0.5f, 0.5f), 1.0f );

    for( uint32_t bboxIdx = 0; bboxIdx < puck_t::BBOX_COUNT; bboxIdx++ ) {
        const llce::box_t& puckBBox = mBBoxes[bboxIdx];
        if( !puckBBox.empty() ) {
            llce::gfx::render_context_t entityRC( puckBBox, mColor );
            llce::gfx::circle::render( csRenderCircle, mColor );
        }
    }
}


void puck_t::hit( const entity_t* pSource ) {
    for( uint32_t bboxIdx = 0; bboxIdx < puck_t::BBOX_COUNT; bboxIdx++ ) {
        llce::box_t& puckBBox = mBBoxes[bboxIdx];
        if( !puckBBox.empty() ) {
            llce::circle_t puckBounds( puckBBox.center(), mBounds.mRadius );
            if( pSource->mBounds.overlaps(puckBounds) ) {
                puckBounds.exbed( pSource->mBounds );

                vec2f32_t hitVec = puckBounds.mCenter - puckBBox.center();
                vec2f32_t hitDir = glm::normalize( hitVec );
                float32_t hitMag = glm::length( pSource->mVel );

                mBounds.mCenter += hitVec;
                mBBox.mPos += hitVec;
                mVel = hitDir * hitMag;
            }
        }
    }
}


void puck_t::wrap( const entity_t* pContainer ) {
    for( uint32_t bboxIdx = 0; bboxIdx < puck_t::BBOX_COUNT; bboxIdx++ ) {
        llce::box_t& puckBBox = mBBoxes[bboxIdx];
        if( !puckBBox.empty() && !pContainer->mBBox.overlaps(puckBBox) ) {
            puckBBox.mPos = vec2f32_t( 0.0f, 0.0f );
            puckBBox.mDims = vec2f32_t( 0.0f, 0.0f );
        }
    }

    if( mBBox.mPos.x < pContainer->mBBox.min().x ) {
        mBBoxes[puck_t::BBOX_XWRAP_ID] = mBBox;
        float32_t wrapDistance = pContainer->mBBox.min().x - mBBox.mPos.x;
        mBBox.mPos.x = pContainer->mBBox.max().x - wrapDistance;
    } else if( mBBox.mPos.x > pContainer->mBBox.max().x ) {
        mBBoxes[puck_t::BBOX_XWRAP_ID] = mBBox;
        float32_t wrapDistance = mBBox.mPos.x - pContainer->mBBox.max().x;
        mBBox.mPos.x = pContainer->mBBox.min().x + wrapDistance;
    } if( mBBox.mPos.y < pContainer->mBBox.min().y ) {
        mBBoxes[puck_t::BBOX_YWRAP_ID] = mBBox;
        float32_t wrapDistance = pContainer->mBBox.min().y - mBBox.mPos.y;
        mBBox.mPos.y = pContainer->mBBox.max().y - wrapDistance;
    } else if( mBBox.mPos.y > pContainer->mBBox.max().y ) {
        mBBoxes[puck_t::BBOX_YWRAP_ID] = mBBox;
        float32_t wrapDistance = mBBox.mPos.y - pContainer->mBBox.max().y;
        mBBox.mPos.y = pContainer->mBBox.min().y + wrapDistance;
    }

    mBBoxes[puck_t::BBOX_BASE_ID] = mBBox;
    mBounds.mCenter = mBBox.center();

    // TODO(JRC): Figure out how to properly activate the XY wrapped puck.
}

}
