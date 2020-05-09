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

typedef bool32_t (*update_f)( ssn::state_t*, ssn::input_t*, const float64_t );
typedef bool32_t (*render_f)( const ssn::state_t*, const ssn::input_t*, const ssn::output_t* );

constexpr static float64_t SCORE_PHASE_DURATIONS[] = { 1.0, 1.0 };

constexpr static char8_t TITLE_ITEM_TEXT[][8] = { "START", "EXIT " };
constexpr static uint32_t TITLE_ITEM_COUNT = ARRAY_LEN( TITLE_ITEM_TEXT );
constexpr static char8_t RESET_ITEM_TEXT[][8] = { "REPLAY", "EXIT  " };
constexpr static uint32_t RESET_ITEM_COUNT = ARRAY_LEN( RESET_ITEM_TEXT );

/// Helper Functions ///

void render_gameboard( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) {
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

    { // Timer Render //
        const static float32_t csSidePadding = 1.0e-2f;
        const static color4u8_t csSideColor = llce::gfx::color::transparentize( ssn::color::INFO, 0.8f );

        float64_t roundProgress = 1.0 - glm::min( pState->rt / ssn::ROUND_DURATION, 1.0 );
        const float32_t cRoundProgress = static_cast<float32_t>( roundProgress );

        glPushAttrib( GL_CURRENT_BIT );
        glColor4ubv( (uint8_t*)&csSideColor );
        glPushMatrix(); {
            mat4f32_t sideSpace( 1.0f );
            sideSpace = glm::translate( vec3f32_t(1.0f, 1.0f, 0.0f) );
            sideSpace *= glm::rotate( glm::half_pi<float32_t>(), vec3f32_t(0.0f, 0.0f, 1.0f) );
            sideSpace *= glm::scale( vec3f32_t(-1.0f, 1.0f, 1.0f) );
            glMultMatrixf( &sideSpace[0][0] );

            for( uint32_t sideIdx = 0; sideIdx < 4 && sideIdx * 0.25f < cRoundProgress; sideIdx++ ) {
                sideSpace = glm::translate( vec3f32_t(0.0f, 1.0f, 0.0f) );
                // NOTE(JRC): Since the "side" coordinate system is left-handed, the
                // rotations need to be inverted relative to expectation.
                sideSpace *= glm::rotate( -glm::half_pi<float32_t>(), vec3f32_t(0.0f, 0.0f, 1.0f) );
                glMultMatrixf( &sideSpace[0][0] );

                const float32_t cSideProgress = glm::min( (cRoundProgress - sideIdx * 0.25f) / 0.25f, 1.0f );
                glBegin( GL_QUADS ); {
                    glVertex2f( 0.0f, 0.0f );
                    glVertex2f( 0.0f, 1.0f * cSideProgress );
                    glVertex2f( csSidePadding, glm::mix(csSidePadding, 1.0f - csSidePadding, cSideProgress) );
                    glVertex2f( csSidePadding, csSidePadding );
                } glEnd();
            }
        } glPopMatrix();
        glPopAttrib();
    }
}

/// 'ssn::mode::game' Functions  ///

bool32_t game::init( ssn::state_t* pState ) {
    pState->rt = 0.0;
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
    vec2i32_t paddleInput( 0, 0 );
    bool32_t rushInput = false;

    { // Input Processing //
        if( llce::input::isKeyDown(pInput->keyboard(), SDL_SCANCODE_W) ) {
            paddleInput.y += 1;
        } if( llce::input::isKeyDown(pInput->keyboard(), SDL_SCANCODE_S) ) {
            paddleInput.y -= 1;
        } if( llce::input::isKeyDown(pInput->keyboard(), SDL_SCANCODE_A) ) {
            paddleInput.x -= 1;
        } if( llce::input::isKeyDown(pInput->keyboard(), SDL_SCANCODE_D) ) {
            paddleInput.x += 1;
        }

        if( llce::input::isKeyDown(pInput->keyboard(), SDL_SCANCODE_E) ) {
            rushInput = true;
        }
    }

    ssn::bounds_t* const bounds = &pState->bounds;
    ssn::puck_t* const puck = &pState->puck;
    ssn::paddle_t* const paddle = &pState->paddle;
    ssn::particulator_t* const particulator = &pState->particulator;

    { // Game State Update //
        const bool32_t cPaddleWasRushing = paddle->mAmRushing;

        // NOTE(JRC): This is handled a bit clumsily so that we recognize round
        // ends the frame they happen instead of a frame late.
        pState->rt += ( pState->ht <= 0.0 ) ? pDT : 0.0;

        if( pState->ht > 0.0 ) {
            pState->ht = ( pState->ht < ssn::HIT_DURATION ) ? pState->ht + pDT : 0.0;
        } else if( pState->rt >= ssn::ROUND_DURATION ) {
            pState->pmid = ssn::mode::score_id;
        } else {
            paddle->move( paddleInput.x, paddleInput.y );
            if( rushInput ) { paddle->rush(); }

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

    // TODO(JRC): Remove this shortcut function once debugging is over.
    if( llce::input::isKeyDown(pInput->keyboard(), SDL_SCANCODE_T) ) { // Game Scoring //
        pState->pmid = ssn::mode::score_id;
    }

    return true;
}


bool32_t game::render( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) {
    render_gameboard( pState, pInput, pOutput );

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
    const auto cMenuEvent = pState->titleMenu.update( pInput->keyboard(), pDT );
    const uint32_t cMenuIndex = pState->titleMenu.mSelectIndex;

    if( cMenuEvent == llce::gui::event_e::select ) {
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
    pState->scores[ssn::team::left] = pState->scores[ssn::team::right] = -1.0f;

    return true;
}


bool32_t score::update( ssn::state_t* pState, ssn::input_t* pInput, const float64_t pDT ) {
    const static auto csUpdateIntro = []
            ( ssn::state_t* pState, ssn::input_t* pInput, const float64_t pPT ) -> bool32_t  {
        if( pPT > SCORE_PHASE_DURATIONS[0] / 10.0 && pState->scores[0] < 0.0f ) {
            // TODO(JRC): Make this value more properly informed by the current
            // aspect ratio being used by the simulation.
            const static vec2u32_t csPixelRes( 512, 512 );
            const static uint32_t csPixelTotal = csPixelRes.x * csPixelRes.y;

            ssn::bounds_t* const bounds = &pState->bounds;
            const uint32_t cAreaLength = bounds_t::AREA_CORNER_COUNT;
            const llce::interval_t& xbounds = bounds->mBBox.xbounds();
            const llce::interval_t& ybounds = bounds->mBBox.ybounds();

            int32_t scores[2] = { 0, 0 };
            for( uint32_t yPixelIdx = 0; yPixelIdx < csPixelRes.y; yPixelIdx++ ) {
                for( uint32_t xPixelIdx = 0; xPixelIdx < csPixelRes.x; xPixelIdx++ ) {
                    vec2f32_t pixelPos(
                        xbounds.interp((xPixelIdx + 0.5f) / csPixelRes.x),
                        ybounds.interp((yPixelIdx + 0.5f) / csPixelRes.y) );

                    for( uint32_t areaIdx = bounds->mAreaCount; areaIdx-- > 0; ) {
                        vec2f32_t* areaPoss = &bounds->mAreaCorners[areaIdx * cAreaLength];
                        if( llce::geom::contains(areaPoss, cAreaLength, pixelPos) ) {
                            scores[bounds->mAreaTeams[areaIdx]]++;
                            break;
                        }
                    }
                }
            } for( uint32_t teamIdx = ssn::team::left; teamIdx <= ssn::team::right; teamIdx++ ) {
                pState->scores[teamIdx] = scores[teamIdx] / ( csPixelTotal + 0.0f );
            }
        }

        return true;
    };
    const static auto csUpdateTally = []
            ( ssn::state_t* pState, ssn::input_t* pInput, const float64_t pPT ) -> bool32_t  {
        std::cout << "Scores:" << std::endl;
        std::cout << "  Left: " << pState->scores[ssn::team::left] << std::endl;
        std::cout << "  Right: " << pState->scores[ssn::team::right] << std::endl;

        return true;
    };
    const static update_f csUpdateFuns[] = { csUpdateIntro, csUpdateTally };

    bool32_t phaseResult = true;

    float64_t phaseMin = 0.0, phaseMax = 0.0;
    for( uint32_t phaseIdx = 0; phaseIdx < ARRAY_LEN(SCORE_PHASE_DURATIONS); phaseIdx++ ) {
        phaseMax = phaseMin + SCORE_PHASE_DURATIONS[phaseIdx];
        if( phaseMin <= pState->st && pState->st < phaseMax ) {
            phaseResult = csUpdateFuns[phaseIdx]( pState, pInput, pState->st - phaseMin );
        }
        phaseMin = phaseMax;
    }

    if( pState->st >= phaseMax ) {
        pState->pmid = ssn::mode::reset_id;
    }

    return phaseResult;
}


bool32_t score::render( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) {
    const static auto csRenderIntro = []
            ( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) -> bool32_t {
        const float32_t cHeaderPadding = 0.05f;
        const vec2f32_t cHeaderDims = { 1.0f - 2.0f * cHeaderPadding, 0.25f };
        const vec2f32_t cHeaderPos = { cHeaderPadding, 1.0f - cHeaderPadding - cHeaderDims.y };
        llce::gfx::text::render( "GAME!", &ssn::color::INFO, llce::box_t(cHeaderPos, cHeaderDims) );

        return true;
    };
    const static auto csRenderTally = []
            ( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) -> bool32_t  {
        const float32_t cHeaderPadding = 0.05f;
        const vec2f32_t cHeaderDims = { 1.0f - 2.0f * cHeaderPadding, 0.25f };
        const vec2f32_t cHeaderPos = { cHeaderPadding, 1.0f - cHeaderPadding - cHeaderDims.y };
        llce::gfx::text::render( "SCORES!", &ssn::color::INFO, llce::box_t(cHeaderPos, cHeaderDims) );

        // notes:
        // - advancing fronts will show both team scores in a bar at the bottom of the
        //   screen that fills in from either end
        // - advancing fronts will just start as vertical lines
        // - calculate the score values as aggregates so that subsequent frames need to
        //   do less work (they can just add to the aggregate with the current marginal)
        // - a velocity for the advancing fronts should be chosen so that an appropriate
        //   number of cells are calculated each frame to fit in that frame's time slice

        return true;
    };
    const static render_f csRenderFuns[] = { csRenderIntro, csRenderTally };

    bool32_t phaseResult = true;

    render_gameboard( pState, pInput, pOutput );

    float64_t phaseMin = 0.0, phaseMax = 0.0;
    for( uint32_t phaseIdx = 0; phaseIdx < ARRAY_LEN(SCORE_PHASE_DURATIONS); phaseIdx++ ) {
        phaseMax = phaseMin + SCORE_PHASE_DURATIONS[phaseIdx];
        if( phaseMin <= pState->st && pState->st < phaseMax ) {
            phaseResult = csRenderFuns[phaseIdx]( pState, pInput, pOutput );
        }
        phaseMin = phaseMax;
    }

    return phaseResult;
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
    const auto cMenuEvent = pState->resetMenu.update( pInput->keyboard(), pDT );
    const uint32_t cMenuIndex = pState->resetMenu.mSelectIndex;

    if( cMenuEvent == llce::gui::event_e::select ) {
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
