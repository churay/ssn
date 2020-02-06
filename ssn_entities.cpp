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

paddle_t::paddle_t( const llce::circle_t& pBounds, const entity_t* pContainer ) :
        entity_t( pBounds, &ssn::color::PADDLE ), mContainer( pContainer ), mDI( 0, 0 ) {
    
}


void paddle_t::update( const float64_t pDT ) {
    mAccel = paddle_t::MOVE_ACCEL * mDI;
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
}

/// 'ssn::puck_t' Functions ///

puck_t::puck_t( const llce::circle_t& pBounds, const entity_t* pContainer ) :
        entity_t( pBounds, &ssn::color::PUCK ), mContainer( pContainer ) {
    mBBoxes[puck_t::BBOX_BASE_ID] = mBBox;
}


void puck_t::update( const float64_t pDT ) {
    entity_t::update( pDT );

    { // Resolve Boundary Wrap //
        const llce::box_t& oBBox = mContainer->mBBox;
        mBBox.mPos.x = oBBox.xbounds().wrap( mBBox.mPos.x );
        mBBox.mPos.y = oBBox.ybounds().wrap( mBBox.mPos.y );
        mBounds.mCenter = mBBox.center();
    }

    { // Initialize Bounding Boxes //
        mBBoxes[puck_t::BBOX_BASE_ID] = mBBox;
        for( uint32_t bboxIdx = puck_t::BBOX_BASE_ID + 1; bboxIdx < puck_t::BBOX_COUNT; bboxIdx++ ) {
            mBBoxes[bboxIdx] = llce::box_t();
        }
    }

    { // Update Bounding Boxes //
        const llce::box_t& oBBox = mContainer->mBBox;
        if( !oBBox.contains(mBBox) ) {
            bool32_t isOutsideX = !oBBox.xbounds().contains(mBBox.xbounds());
            if( isOutsideX ) {
                mBBoxes[puck_t::BBOX_XWRAP_ID] = llce::box_t(
                    mBBox.min().x - 1.0f, // NOTE: see wrap code above
                    mBBoxes[puck_t::BBOX_BASE_ID].mPos.y,
                    mBBoxes[puck_t::BBOX_BASE_ID].mDims.x,
                    mBBoxes[puck_t::BBOX_BASE_ID].mDims.y);
            }

            bool32_t isOutsideY = !oBBox.ybounds().contains(mBBox.ybounds());
            if( isOutsideY ) {
                mBBoxes[puck_t::BBOX_YWRAP_ID] = llce::box_t(
                    mBBoxes[puck_t::BBOX_BASE_ID].mPos.x,
                    mBBox.min().y - 1.0f, // NOTE: see wrap code above
                    mBBoxes[puck_t::BBOX_BASE_ID].mDims.x,
                    mBBoxes[puck_t::BBOX_BASE_ID].mDims.y);
            }

            if( isOutsideX && isOutsideY ) {
                mBBoxes[puck_t::BBOX_XYWRAP_ID] = llce::box_t(
                    mBBoxes[puck_t::BBOX_XWRAP_ID].mPos.x,
                    mBBoxes[puck_t::BBOX_YWRAP_ID].mPos.y,
                    mBBoxes[puck_t::BBOX_BASE_ID].mDims.x,
                    mBBoxes[puck_t::BBOX_BASE_ID].mDims.y);
            }
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
                // FIXME(JRC): The unnecessary multiplication here acts as
                // a workaround to weird interactions between gcc compilation
                // and raw 'constexpr static' variables.
                // See: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=50785
                float32_t hitMag = std::max( (1.0f*puck_t::MIN_VEL), glm::length(pSource->mVel) );

                mBounds.mCenter += hitVec;
                mBBox.mPos += hitVec;
                mVel = hitDir * hitMag;

                break;
            }
        }
    }
}

}
