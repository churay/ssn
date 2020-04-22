#include <cstring>
#include <sstream>

#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/ext/scalar_constants.hpp>

#include "gui.h"
#include "gfx.h"
#include "sfx.h"
#include "geom.h"

#include "ssn_modes.h"
#include "ssn_data.h"
#include "ssn_entities.h"

namespace ssn {

namespace mode {

/// Helper Structures ///

constexpr static char8_t TITLE_ITEM_TEXT[][8] = { "START", "EXIT " };
constexpr static uint32_t TITLE_ITEM_COUNT = ARRAY_LEN( TITLE_ITEM_TEXT );
constexpr static char8_t RESET_ITEM_TEXT[][8] = { "REPLAY", "EXIT  " };
constexpr static uint32_t RESET_ITEM_COUNT = ARRAY_LEN( RESET_ITEM_TEXT );

/// 'ssn::mode::game' Functions  ///

bool32_t game::init( ssn::state_t* pState ) {
    pState->ht = 0.0;

    const float32_t cPaddleBaseRadius = 5.0e-2f;

    const vec2f32_t boundsBasePos( 0.0f, 0.0f );
    const vec2f32_t boundsDims( 1.0f, 1.0f );
    pState->bounds = ssn::bounds_t( llce::box_t(boundsBasePos, boundsDims) );

    const vec2f32_t puckCenterPos( 0.25f, 0.5f );
    const float32_t puckRadius( cPaddleBaseRadius * 0.6f );
    pState->puck = ssn::puck_t( llce::circle_t(puckCenterPos, puckRadius),
        ssn::team::neutral, &pState->bounds );

    const vec2f32_t paddleCenterPos( 0.5f, 0.5f );
    const float32_t paddleRadius( cPaddleBaseRadius );
    pState->paddle = ssn::paddle_t( llce::circle_t(paddleCenterPos, paddleRadius),
        ssn::team::left, &pState->bounds );

    pState->particulator = ssn::particulator_t( &pState->rng );

    // { // Testing Score Calculations //
    //     ssn::team_entity_t testEntity( llce::circle_t(0.0f, 0.0f, 0.0f), ssn::team::right );

    //     const vec2f32_t cTestAreas[2][3] = {
    //         {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}},
    //         {{0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}} };
    //     const ssn::team::team_e cTestTeams[] = {
    //         ssn::team::right,   // r score: 0.25
    //         ssn::team::left };  // l score: 0.5
    //     const uint32_t cTestAreaCount = ARRAY_LEN( cTestAreas );
    //     for( uint32_t areaIdx = 0; areaIdx < cTestAreaCount; areaIdx++ ) {
    //         testEntity.change( cTestTeams[areaIdx] );
    //         for( uint32_t cornerIdx = 0; cornerIdx < ssn::bounds_t::AREA_CORNER_COUNT; cornerIdx++ ) {
    //             vec2f32_t prevCenter = testEntity.mBounds.mCenter;
    //             testEntity.mBounds.mCenter = cTestAreas[areaIdx][cornerIdx];
    //             testEntity.mBBox.mPos += ( testEntity.mBounds.mCenter - prevCenter );
    //             pState->bounds.claim( &testEntity );
    //         }
    //     }
    // }

    return true;
}


bool32_t game::update( ssn::state_t* pState, ssn::input_t* pInput, const float64_t pDT ) {
    vec2i32_t di = { 0, 0 };
    bool32_t de = false;

    { // Input Processing //
        if( llce::input::isKeyDown(pInput->keyboard(), SDL_SCANCODE_W) ) {
            di.y += 1;
        } if( llce::input::isKeyDown(pInput->keyboard(), SDL_SCANCODE_S) ) {
            di.y -= 1;
        } if( llce::input::isKeyDown(pInput->keyboard(), SDL_SCANCODE_A) ) {
            di.x -= 1;
        } if( llce::input::isKeyDown(pInput->keyboard(), SDL_SCANCODE_D) ) {
            di.x += 1;
        }

        if( llce::input::isKeyDown(pInput->keyboard(), SDL_SCANCODE_E) ) {
            de = true;
        }
    }

    ssn::bounds_t* const bounds = &pState->bounds;
    ssn::puck_t* const puck = &pState->puck;
    ssn::paddle_t* const paddle = &pState->paddle;
    ssn::particulator_t* const particulator = &pState->particulator;

    { // Game State Update //
        pState->dt = pDT;
        pState->tt += pDT;

        const bool32_t cPaddleWasRushing = paddle->mAmRushing;

        if( pState->ht > 0.0 ) {
            pState->ht = ( pState->ht < ssn::MAX_HIT_TIME ) ? pState->ht + pDT : 0.0;
        } else {
            paddle->move( di.x, di.y );
            if( de ) { paddle->rush(); }

            bounds->update( pDT );
            puck->update( pDT );
            paddle->update( pDT );

            if( puck->hit(paddle) ) {
                bounds->claim( paddle );
                particulator->genHit(
                    puck->mBounds.mCenter, puck->mVel, 2.25f * puck->mBounds.mRadius );
                pState->ht += pDT;
            } if( !cPaddleWasRushing && paddle->mAmRushing ) {
                particulator->genTrail(
                    paddle->mBounds.mCenter, paddle->mVel, 2.0f * paddle->mBounds.mRadius );
            }
        }

        particulator->update( pDT );
    }

    if( llce::input::isKeyDown(pInput->keyboard(), SDL_SCANCODE_T) ) { // Game Scoring //
        vec2i32_t scores = { 0, 0 };

        // NOTE(JRC): The source for this algorithm was derived from here:
        // http://geomalgorithms.com/a01-_area.html
        const static auto csTriDeterminant = []
                ( const vec2f32_t& pP0, const vec2f32_t& pP1, const vec2f32_t& pP2 ) {
            return ( (pP1.x - pP0.x) * (pP2.y - pP0.y) - (pP2.x - pP0.x) * (pP1.y - pP0.y) );
        };

        // NOTE(JRC): The source for this algorithm was derived from here:
        // http://geomalgorithms.com/a03-_inclusion.html
        const static auto csPointInArea = [] ( const vec2f32_t& pPoint,  const vec2f32_t* pArea ) {
            int32_t windCount = 0;

            for( uint32_t cornerIdx = 0; cornerIdx < bounds_t::AREA_CORNER_COUNT; cornerIdx++ ) {
                const vec2f32_t& cornerStart = pArea[cornerIdx];
                const vec2f32_t& cornerEnd = pArea[(cornerIdx + 1) % bounds_t::AREA_CORNER_COUNT];

                // if the corner edge intersects the point x-axis ray upward...
                if( cornerStart.y < pPoint.y && cornerEnd.y > pPoint.y ) {
                    // and the point is strictly left of the edge (s->e->p is ccw)
                    if( csTriDeterminant(cornerStart, cornerEnd, pPoint) > 0.0f ) {
                        windCount++;
                    }
                // if the corner edge intersects the point x-axis ray downward...
                } else if( cornerStart.y > pPoint.y && cornerEnd.y < pPoint.y ) {
                    // and the point is strictly right of the edge (s->e->p is cw)
                    if( csTriDeterminant(cornerStart, cornerEnd, pPoint) < 0.0f ) {
                        windCount--;
                    }
                }
            }

            return windCount != 0;
        };

        // TODO(JRC): Make this value more properly informed by the current
        // aspect ratio being used by the simulation.
        const vec2u32_t cPixelRes( 512, 512 );
        for( uint32_t yPixelIdx = 0; yPixelIdx < cPixelRes.y; yPixelIdx++ ) {
            for( uint32_t xPixelIdx = 0; xPixelIdx < cPixelRes.x; xPixelIdx++ ) {
                vec2f32_t pixelPos(
                    bounds->mBBox.xbounds().interp( (xPixelIdx + 0.5f) / cPixelRes.x ),
                    bounds->mBBox.ybounds().interp( (yPixelIdx + 0.5f) / cPixelRes.y ) );

                for( uint32_t areaIdx = bounds->mAreaCount; areaIdx-- > 0; ) {
                    vec2f32_t* areaPoss = &bounds->mAreaCorners[areaIdx * bounds_t::AREA_CORNER_COUNT];
                    if( csPointInArea(pixelPos, areaPoss) ) {
                        (*VECTOR_AT(scores, bounds->mAreaTeams[areaIdx]))++;
                        break;
                    }
                }
            }
        }

        const uint32_t cPixelTotal = cPixelRes.x * cPixelRes.y;
        std::cout << "Scores:" << std::endl;
        std::cout << "  Left: " << (*VECTOR_AT(scores, ssn::team::left)) / ( cPixelTotal + 0.0f ) << std::endl;
        std::cout << "  Right: " << (*VECTOR_AT(scores, ssn::team::right)) / ( cPixelTotal + 0.0f ) << std::endl;

        pState->pmid = ssn::mode::reset_id;
    }

    return true;
}


bool32_t game::render( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) {
    const ssn::bounds_t* const bounds = &pState->bounds;
    const ssn::puck_t* const puck = &pState->puck;
    const ssn::paddle_t* const paddle = &pState->paddle;
    const ssn::particulator_t* const particulator = &pState->particulator;

    { // Game State Render //
        bounds->render();
        puck->render();
        particulator->render();
        paddle->render();
    }

    { // Annotation Render //
        llce::gfx::text::render( "YOU", &ssn::color::INFO, 20.0f,
            paddle->mBBox.mPos + llce::geom::anchor( paddle->mBBox.mDims, llce::geom::anchor2D::mh ),
            llce::geom::anchor2D::ml );
    }

    return true;
}

/// 'ssn::mode::title' Functions  ///

bool32_t title::init( ssn::state_t* pState ) {
    const char8_t* cTitleItems[] = { &TITLE_ITEM_TEXT[0][0], &TITLE_ITEM_TEXT[1][0] };
    pState->titleMenu = llce::gui::menu_t( "SSN",
        cTitleItems, TITLE_ITEM_COUNT,
        &ssn::color::BACKGROUND, &ssn::color::FOREGROUND,
        &ssn::color::TEAM[ssn::team::neutral], &ssn::color::FOREGROUND );

    return true;
}


bool32_t title::update( ssn::state_t* pState, ssn::input_t* pInput, const float64_t pDT ) {
    const auto cMenuAction = pState->titleMenu.update( pInput->keyboard(), pDT );
    const uint32_t cMenuIndex = pState->titleMenu.mSelectIndex;

    if( cMenuAction == llce::gui::menu_t::action_e::select ) {
        if( cMenuIndex == 0 ) {
            pState->pmid = ssn::mode::game_id;
        } else if( cMenuIndex == 1 ) {
            pState->pmid = ssn::mode::exit_id;
        }
    }

    return true;
}

bool32_t title::render( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) {
    pState->titleMenu.render();

    return true;
}

/// 'ssn::mode::score' Functions  ///

bool32_t score::init( ssn::state_t* pState ) {
    // TODO(JRC): Implement this function.
    return true;
}


bool32_t score::update( ssn::state_t* pState, ssn::input_t* pInput, const float64_t pDT ) {
    // TODO(JRC): Implement this function.
    return true;
}

bool32_t score::render( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) {
    // TODO(JRC): Implement this function.
    return true;
}

/// 'ssn::mode::reset' Functions  ///

bool32_t reset::init( ssn::state_t* pState ) {
    const char8_t* cResetItems[] = { &RESET_ITEM_TEXT[0][0], &RESET_ITEM_TEXT[1][0] };
    pState->resetMenu = llce::gui::menu_t( "GAME!",
        cResetItems, RESET_ITEM_COUNT,
        &ssn::color::BACKGROUND, &ssn::color::FOREGROUND,
        &ssn::color::TEAM[ssn::team::neutral], &ssn::color::FOREGROUND );

    return true;
}


bool32_t reset::update( ssn::state_t* pState, ssn::input_t* pInput, const float64_t pDT ) {
    const auto cMenuAction = pState->resetMenu.update( pInput->keyboard(), pDT );
    const uint32_t cMenuIndex = pState->resetMenu.mSelectIndex;

    if( cMenuAction == llce::gui::menu_t::action_e::select ) {
        if( cMenuIndex == 0 ) {
            pState->pmid = ssn::mode::game_id;
        } else if( cMenuIndex == 1 ) {
            pState->pmid = ssn::mode::title_id;
        }
    }

    return true;
}


bool32_t reset::render( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) {
    pState->resetMenu.render();

    return true;
}

}

}
