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
#include "util.hpp"

#include "ssn_modes.h"
#include "ssn_data.h"
#include "ssn_entities.h"

namespace ssn {

namespace mode {

/// Helper Structures ///

// Helper Types //

typedef bool32_t (*update_f)( ssn::state_t*, ssn::input_t*, const float64_t, const float64_t );
typedef bool32_t (*render_f)( const ssn::state_t*, const ssn::input_t*, const ssn::output_t* );

// Input Data //

constexpr static uint32_t TEAM_GO_ACTIONS[] = { ssn::action::lrush, ssn::action::rrush, ssn::action::unbound };
constexpr static uint32_t TEAM_UP_ACTIONS[] = { ssn::action::lup, ssn::action::rup, ssn::action::unbound };
constexpr static uint32_t TEAM_DOWN_ACTIONS[] = { ssn::action::ldown, ssn::action::rdown, ssn::action::unbound };
constexpr static uint32_t TEAM_LEFT_ACTIONS[] = { ssn::action::lleft, ssn::action::rleft, ssn::action::unbound };
constexpr static uint32_t TEAM_RIGHT_ACTIONS[] = { ssn::action::lright, ssn::action::rright, ssn::action::unbound };

constexpr static uint32_t MENU_ACTIONS[] = { ssn::action::unbound, ssn::action::lrush, ssn::action::ldown, ssn::action::lup };

// Per-Mode Data //

constexpr static float64_t SCORE_PHASE_DURATIONS[] = { 1.0, 2.0, 1.0 };

constexpr static char8_t TITLE_ITEM_TEXT[][8] = { "START", "INPUT", "EXIT " };
constexpr static char8_t RESET_ITEM_TEXT[][8] = { "REPLAY", "EXIT  " };

constexpr static vec2u32_t SELECT_ITEM_DIMS = { 4, 2 };
constexpr static uint32_t SELECT_ITEM_COUNT = SELECT_ITEM_DIMS.x * SELECT_ITEM_DIMS.y;

// Helper Assertions //

static_assert( LLCE_ELEM_COUNT(STAGE_SPECS) == ssn::stage_e::_length,
    "Incorrect number of stage specifications; "
    "please add the dimension specifications of all stages in enumeration "
    "'ssn::stage::stage_e' to the array 'STAGE_SPECS' in 'ssn_consts.h'." );
static_assert( LLCE_ELEM_COUNT(STAGE_NAMES) == ssn::stage_e::_length,
    "Incorrect number of stage names; "
    "please add the names of all stages in enumeration "
    "'ssn::stage::stage_e' to the array 'STAGE_NAMES' in 'ssn_consts.h'." );
static_assert( SELECT_ITEM_COUNT >= ssn::stage_e::_length,
    "Insufficient number of selection items; "
    "please add enough selection items to cover all stages in enumeration "
    "'ssn::stage::stage_e' to the array 'SELCT_ITEM_DIMS' in 'ssn_modes.cpp'." );

/// Helper Functions ///

void gameboard_render( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) {
    const ssn::bounds_t* const bounds = &pState->bounds;
    const ssn::puck_t* const puck = &pState->puck;
    const ssn::paddle_t* const paddles = &pState->paddles[0];
    const ssn::particulator_t* const particulator = &pState->particulator;

    { // Game State Render //
        bounds->render();
        puck->render();
        particulator->render();
        paddles[ssn::team::left].render();
        paddles[ssn::team::right].render();

        { // Out-of-Bounds Occlusion //
            const float32_t cBoundsBorderSizes[] = {
                bounds->mBBox.min().y,        // y=ymin
                1.0f - bounds->mBBox.max().x, // x=xmax
                1.0f - bounds->mBBox.max().y, // y=ymax
                bounds->mBBox.min().x,        // x=xmin
            };

            llce::gfx::color_context_t sideCC( &ssn::color::OUTOFBOUND );
            llce::gfx::render::border( cBoundsBorderSizes );
        }
    }

    { // Timer Render //
        const static float32_t csSidePadding = 1.0e-2f;

        float64_t roundProgress = 1.0 - glm::min( pState->rt / ssn::ROUND_DURATION, 1.0 );
        const float32_t cRoundProgress = static_cast<float32_t>( roundProgress );

        glPushAttrib( GL_CURRENT_BIT );
        glColor4ubv( (uint8_t*)&ssn::color::INFOL );
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

bool32_t game::init( ssn::state_t* pState, ssn::input_t* pInput ) {
    pState->rt = 0.0;
    pState->ht = 0.0;

    const vec2f32_t cStageCenter( 0.5f, 0.5f );
    const vec2f32_t cStageDims = ssn::STAGE_SPECS[pState->sid];
    const float32_t cPaddleRadius = 4.0e-2f;
    const float32_t cPuckRadius = cPaddleRadius * 5.0e-1f;

    pState->bounds = ssn::bounds_t( llce::box_t(cStageCenter, cStageDims,
        llce::geom::anchor2D::mm) );

    pState->puck = ssn::puck_t( llce::circle_t(cStageCenter, cPuckRadius),
        ssn::team::neutral, &pState->bounds );

    for( uint8_t team = ssn::team::left; team <= ssn::team::right; team++ ) {
        float32_t paddleXFactor = (team == ssn::team::left) ? 0.25f : 0.75f;
        vec2f32_t paddleCenter(
            cStageCenter.x - (cStageDims.x * 0.5f) + cStageDims.x * paddleXFactor,
            cStageCenter.x - (cStageDims.x * 0.5f) + cStageDims.x * 0.5f );
        pState->paddles[team] = ssn::paddle_t( llce::circle_t(paddleCenter, cPaddleRadius),
            static_cast<ssn::team_e>(team), &pState->bounds );
    }

    pState->particulator = ssn::particulator_t( &pState->rng );

    // { // Testing Score Calculations //
    //     ssn::team_entity_t testEntity( llce::circle_t(0.0f, 0.0f, 0.0f), ssn::team::right );
    //     const llce::box_t& testBBox = pState->bounds.mBBox;

    //     // const vec2f32_t cTestAreas[2][3] = {
    //     //     {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}},
    //     //     {{0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}} };
    //     // const ssn::team::team_e cTestTeams[] = {
    //     //     ssn::team::right,   // r score: 0.25
    //     //     ssn::team::left };  // l score: 0.5
    //     // const vec2f32_t cTestAreas[2][3] = {
    //     //     {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}},
    //     //     {{0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}} };
    //     // const ssn::team::team_e cTestTeams[] = {
    //     //     ssn::team::left,
    //     //     ssn::team::left }; // l score: 1.0
    //     const vec2f32_t cTestAreas[2][3] = { {
    //             testBBox.at(llce::geom::anchor2D::ll),
    //             testBBox.at(llce::geom::anchor2D::hl),
    //             testBBox.at(llce::geom::anchor2D::hh)
    //         }, {
    //             testBBox.at(llce::geom::anchor2D::ll),
    //             testBBox.at(llce::geom::anchor2D::hh),
    //             testBBox.at(llce::geom::anchor2D::lh)
    //         } };
    //     const ssn::team::team_e cTestTeams[] = {
    //         ssn::team::left,    // l score: 0.5
    //         ssn::team::right }; // r score: 0.5
    //     const uint32_t cTestAreaCount = LLCE_ELEM_COUNT( cTestAreas );
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
    vec2i32_t moveInputs[2] = { {0.0f, 0.0f}, {0.0f, 0.0f} };
    bool32_t rushInputs[2] = { false, false };

    { // Input Processing //
        for( uint8_t team = ssn::team::left; team <= ssn::team::right; team++ ) {
            if( pInput->isDownAct(TEAM_UP_ACTIONS[team]) ) {
                moveInputs[team].y += 1;
            } if( pInput->isDownAct(TEAM_DOWN_ACTIONS[team]) ) {
                moveInputs[team].y -= 1;
            } if( pInput->isDownAct(TEAM_LEFT_ACTIONS[team]) ) {
                moveInputs[team].x -= 1;
            } if( pInput->isDownAct(TEAM_RIGHT_ACTIONS[team]) ) {
                moveInputs[team].x += 1;
            }

            if( pInput->isPressedAct(TEAM_GO_ACTIONS[team]) ) {
                rushInputs[team] = true;
            }
        }
    }

    ssn::bounds_t* const bounds = &pState->bounds;
    ssn::puck_t* const puck = &pState->puck;
    ssn::paddle_t* const paddles = &pState->paddles[0];
    ssn::particulator_t* const particulator = &pState->particulator;

    { // Game State Update //
        const bool32_t cPaddleWasRushing[2] = { paddles[0].mAmRushing, paddles[1].mAmRushing };

        // NOTE(JRC): This is handled a bit clumsily so that we recognize round
        // ends the frame they happen instead of a frame late.
        pState->rt += ( pState->ht <= 0.0 ) ? pDT : 0.0;

        if( pState->ht > 0.0 ) {
            pState->ht = ( pState->ht < ssn::HIT_DURATION ) ? pState->ht + pDT : 0.0;
        } else if( pState->rt >= ssn::ROUND_DURATION ) {
            pState->pmode = ssn::mode::score::ID;
        } else {
            for( uint8_t team = ssn::team::left; team <= ssn::team::right; team++ ) {
                paddles[team].move( moveInputs[team].x, moveInputs[team].y );
                if( rushInputs[team] ) { paddles[team].rush(); }
            }

            bounds->update( pDT );
            puck->update( pDT );
            paddles[ssn::team::left].update( pDT );
            paddles[ssn::team::right].update( pDT );

            for( uint8_t team = ssn::team::left; team <= ssn::team::right; team++ ) {
                ssn::paddle_t* const paddle = &paddles[team];
                if( puck->hit(paddle) ) {
                    bounds->claim( paddle );
                    particulator->genHit(
                        puck->mBounds.mCenter, puck->mVel, 2.25f * puck->mBounds.mRadius );
                    pState->ht += pDT;

                    // NOTE(JRC): If an area is claimed as a result of this hit, we
                    // slow down the puck again in preparation for the next rally.
                    if( bounds->mCurrAreaTeam == ssn::team::neutral ) {
                        const float32_t cVolleyCount = glm::max( 1.0f, bounds->mCurrAreaCount + 0.0f );
                        const vec2f32_t cVolleyDir = llce::util::normalize( puck->mVel );
                        puck->mVel = cVolleyDir *
                            ssn::puck_t::MIN_VEL * cVolleyCount * ssn::puck_t::VEL_MULTIPLIER;
                    }
                } if( !cPaddleWasRushing[team] && paddle->mAmRushing ) {
                    particulator->genTrail(
                        paddle->mBounds.mCenter, paddle->mVel, 2.0f * paddle->mBounds.mRadius );
                }
            }
        }

        particulator->update( pDT );
    }

    return true;
}


bool32_t game::render( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) {
    gameboard_render( pState, pInput, pOutput );

    return true;
}

/// 'ssn::mode::select' Functions  ///

bool32_t select::init( ssn::state_t* pState, ssn::input_t* pInput ) {
    pState->selectMenuIndex = 0;

    return true;
}


bool32_t select::update( ssn::state_t* pState, ssn::input_t* pInput, const float64_t pDT ) {
    vec2i32_t menuInput( 0, 0 );
    bool32_t menuSelected = false;

    { // Input Processing //
        if( pInput->isPressedAct(&TEAM_UP_ACTIONS[0]) ) {
            menuInput.y -= 1;
        } if( pInput->isPressedAct(&TEAM_DOWN_ACTIONS[0]) ) {
            menuInput.y += 1;
        } if( pInput->isPressedAct(&TEAM_LEFT_ACTIONS[0]) ) {
            menuInput.x -= 1;
        } if( pInput->isPressedAct(&TEAM_RIGHT_ACTIONS[0]) ) {
            menuInput.x += 1;
        }

        if( pInput->isPressedAct(&TEAM_GO_ACTIONS[0]) ) {
            menuSelected = true;
        }
    }

    int32_t newSelectMenuIndex = pState->selectMenuIndex +
        ( SELECT_ITEM_DIMS.x * menuInput.y ) + menuInput.x;
    if( newSelectMenuIndex >= 0 && newSelectMenuIndex < ssn::stage::_length ) {
        pState->selectMenuIndex = static_cast<int8_t>( newSelectMenuIndex );
    }

    if( menuSelected ) {
        pState->sid = static_cast<ssn::stage_e>( pState->selectMenuIndex );
        pState->pmode = ssn::mode::game::ID;
    }

    return true;
}

bool32_t select::render( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) {
    llce::gfx::color_context_t selectCC( &ssn::color::BACKGROUND );
    llce::gfx::render::box();

    const static float32_t csSectionPadding = 5.0e-2f;
    const static llce::box_t csHeaderArea( 0.0f, 0.5f, 1.0f, 0.5f );
    const static llce::box_t csItemArea( 0.0f, 0.0f, 1.0f, 0.5f );

    const static auto csRenderStagePreview = [] ( const uint32_t pStage ) {
        const static float32_t csRenderPadding = 5.0e-2f;
        const static float32_t csPreviewDim = 1.0f - 2.0f * csRenderPadding;
        const static llce::box_t csPaddedBox(
            0.5f, 0.5f, csPreviewDim, csPreviewDim, llce::geom::anchor2D::mm );

        const vec2f32_t cStageDims = ssn::STAGE_SPECS[pStage];
        const char8_t* cStageName = &ssn::STAGE_NAMES[pStage][0];

        llce::gfx::color_context_t stageCC( &ssn::color::TEAM[ssn::team::neutral] );
        llce::gfx::render::box();

        llce::gfx::render_context_t stageRC( csPaddedBox );
        stageCC.update( &ssn::color::OUTOFBOUND );
        llce::gfx::render::box();

        stageCC.update( &ssn::color::BACKGROUND );
        llce::gfx::render::box( llce::box_t(
            vec2f32_t(0.5f, 0.5f), cStageDims, llce::geom::anchor2D::mm) );

        stageCC.update( &ssn::color::INFOLL );
        llce::gfx::render::text( cStageName, csPaddedBox );
    };

    { // Header //
        llce::gfx::render_context_t headerRC( csHeaderArea );
        llce::gfx::render_context_t previewRC( llce::box_t(
            0.5f, 0.5f, 1.0f - csSectionPadding, 1.0f - csSectionPadding,
            llce::geom::anchor2D::mm), 1.0f );
        csRenderStagePreview( pState->selectMenuIndex );
    }

    { // Items //
        const static vec2f32_t csItemDims(
            csItemArea.mDims.x / SELECT_ITEM_DIMS.x,
            csItemArea.mDims.y / SELECT_ITEM_DIMS.y );
        const static vec2f32_t csItemPaddedDims(
            csItemDims.x - csSectionPadding,
            csItemDims.y - csSectionPadding );

        for( uint32_t yIdx = SELECT_ITEM_DIMS.y, itemIdx = 0; yIdx-- > 0; ) {
            for( uint32_t xIdx = 0; xIdx < SELECT_ITEM_DIMS.x; xIdx++, itemIdx++ ) {
                const vec2f32_t cItemPos( xIdx * csItemDims.x, yIdx * csItemDims.y );
                const llce::box_t cItemBox( cItemPos, csItemDims );

                if( itemIdx == pState->selectMenuIndex ) {
                    selectCC.update( &ssn::color::FOREGROUND );
                    llce::gfx::render::box( cItemBox );
                }

                if( itemIdx < ssn::stage::_length ) {
                    llce::gfx::render_context_t itemRC( llce::box_t(
                        cItemBox.mid(), csItemPaddedDims, llce::geom::anchor2D::mm) );
                    csRenderStagePreview( itemIdx );
                }
            }
        }
    }

    return true;
}

/// 'ssn::mode::title' Functions  ///

bool32_t title::init( ssn::state_t* pState, ssn::input_t* pInput ) {
    auto cTitleItems = llce::util::pointerize( TITLE_ITEM_TEXT );
    pState->titleMenu = llce::gui::menu_t(
        pInput, &MENU_ACTIONS[0],
        "SSN", cTitleItems.data(), cTitleItems.size(),
        &ssn::color::BACKGROUND,
        &ssn::color::FOREGROUND,
        &ssn::color::TEAM[ssn::team::neutral] );

    return true;
}


bool32_t title::update( ssn::state_t* pState, ssn::input_t* pInput, const float64_t pDT ) {
    pState->titleMenu.update( pDT );

    if( pState->titleMenu.changed(llce::gui::event::select) ) {
        if( pState->titleMenu.mItemIndex == 0 ) {
            pState->pmode = ssn::mode::select::ID;
        } else if( pState->titleMenu.mItemIndex == 1 ) {
            pState->pmode = ssn::mode::bind::ID;
        } else if( pState->titleMenu.mItemIndex == 2 ) {
            pState->pmode = ssn::mode::exit::ID;
        }
    }

    return true;
}

bool32_t title::render( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) {
    pState->titleMenu.render();

    return true;
}

/// 'ssn::mode::bind' Functions  ///

bool32_t bind::init( ssn::state_t* pState, ssn::input_t* pInput ) {
    auto cActionTitles = llce::util::pointerize( ssn::ACTION_NAMES );
    pState->bindMenu = llce::gui::bind_menu_t(
        pInput, &MENU_ACTIONS[0],
        cActionTitles.data(), cActionTitles.size(),
        &ssn::color::BACKGROUND,
        &ssn::color::FOREGROUND,
        &ssn::color::TEAM[ssn::team::neutral],
        &ssn::color::OUTOFBOUND );

    return true;
}


bool32_t bind::update( ssn::state_t* pState, ssn::input_t* pInput, const float64_t pDT ) {
    pState->bindMenu.update( pDT );

    if( pState->bindMenu.changed(llce::gui::event::select) ) {
        if( pState->bindMenu.mItemIndex == pState->bindMenu.mItemCount - 1 ) {
            pState->pmode = ssn::mode::title::ID;
        }
    }

    return true;
}

bool32_t bind::render( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) {
    pState->bindMenu.render();

    return true;
}

/// 'ssn::mode::score' Functions  ///

bool32_t score::init( ssn::state_t* pState, ssn::input_t* pInput ) {
    std::memset( &pState->scoreTotals[0], 0, sizeof(pState->scoreTotals) );
    std::memset( &pState->scoreSamples[0], 0, sizeof(pState->scoreSamples) );
    pState->scoreTallied = false;

    return true;
}


bool32_t score::update( ssn::state_t* pState, ssn::input_t* pInput, const float64_t pDT ) {
    const static auto csUpdateIntro = []
            ( ssn::state_t* pState, ssn::input_t* pInput, const float64_t pDT, const float64_t pPT ) -> bool32_t  {
        if( pPT > SCORE_PHASE_DURATIONS[0] / 10.0 && !pState->scoreTallied ) {
            ssn::bounds_t* const bounds = &pState->bounds;
            const uint32_t cAreaLength = bounds_t::AREA_CORNER_COUNT;
            // NOTE(JRC): The global bounds of the game space are used instead of
            // the game space boundaries in order to preserve uniformity of sampling
            // regardless of aspect ratio and to simplify the meaning of 'scoreSamples'.
            const llce::interval_t xbounds( 0.0f, 1.0f ); // = bounds->mBBox.xbounds();
            const llce::interval_t ybounds( 0.0f, 1.0f ); // = bounds->mBBox.ybounds();

            bit8_t* scores = &pState->scoreSamples[0];
            for( uint32_t yIdx = 0, sIdx = 0; yIdx < SCORE_SAMPLE_RES.y; yIdx++ ) {
                for( uint32_t xIdx = 0; xIdx < SCORE_SAMPLE_RES.x; xIdx++, sIdx++ ) {
                    uint32_t sampleIdx = sIdx / SCORE_SAMPLES_PER_BYTE;
                    uint32_t sampleOffset = SCORE_SAMPLE_BITS * ( sIdx % SCORE_SAMPLES_PER_BYTE );
                    vec2f32_t samplePos(
                        xbounds.interp((xIdx + 0.5f) / SCORE_SAMPLE_RES.x),
                        ybounds.interp((yIdx + 0.5f) / SCORE_SAMPLE_RES.y) );

                    for( uint32_t areaIdx = bounds->mAreaCount; areaIdx-- > 0; ) {
                        vec2f32_t* areaPoss = &bounds->mAreaCorners[areaIdx * cAreaLength];
                        uint8_t areaTeam = bounds->mAreaTeams[areaIdx];
                        if( llce::geom::contains(areaPoss, cAreaLength, samplePos) ) {
                            scores[sampleIdx] |= ( 1 << areaTeam ) << sampleOffset;
                            break;
                        }
                    }
                }
            }

            pState->scoreTallied = true;
        }

        return true;
    };
    const static auto csUpdateTally = []
            ( ssn::state_t* pState, ssn::input_t* pInput, const float64_t pDT, const float64_t pPT ) -> bool32_t  {
        const static float64_t csTallyDX = 1.0 / ssn::SCORE_SAMPLE_RES.x;
        const static float64_t csTallyDY = 1.0 / ssn::SCORE_SAMPLE_RES.y;
        const static float64_t csTallyDA = csTallyDX * csTallyDY;

        const float64_t cPrevBasePos = 0.5 * glm::max( (pPT - pDT) / SCORE_PHASE_DURATIONS[1], 0.0 );
        const float64_t cCurrBasePos = 0.5 * glm::min( pPT / SCORE_PHASE_DURATIONS[1], 1.0 );

        for( uint32_t tallyIdx = 0; tallyIdx < 2; tallyIdx++ ) {
            // NOTE(JRC): The initial offsets effectively make the tally fronts
            // count a sample column when it reaches its center rather than either
            // end, which simplifies the process by avoiding corner cases.
            float64_t prevTallyPos = -csTallyDX / 2.0 +
                ( tallyIdx ? 1.0 - cPrevBasePos : cPrevBasePos );
            float64_t currTallyPos = -csTallyDX / 2.0 +
                ( tallyIdx ? 1.0 - cCurrBasePos : cCurrBasePos );
            const float64_t cTallyMin = glm::min( prevTallyPos, currTallyPos );
            const float64_t cTallyMax = glm::max( prevTallyPos, currTallyPos );

            pState->tallyPoss[tallyIdx] = currTallyPos + csTallyDX / 2.0;

            // NOTE(JRC): Consider offsetting these values by the play space
            // boundaries (i.e. bounds->mBBox.{x|y}bounds()) in order to keep
            // the tallying to the relevant area of the game space.
            const int32_t cSampleMinIdx = glm::ceil( cTallyMin / csTallyDX );
            const int32_t cSampleMaxIdx = glm::floor( cTallyMax / csTallyDX );
            for( int32_t xIdx = cSampleMinIdx;
                    xIdx <= cSampleMaxIdx && xIdx >= 0 && xIdx < SCORE_SAMPLE_RES.x;
                    xIdx++ ) {
                for( int32_t yIdx = 0; yIdx < SCORE_SAMPLE_RES.y; yIdx++ ) {
                    uint32_t sIdx = yIdx * SCORE_SAMPLE_RES.x + xIdx;
                    uint32_t sampleIdx = sIdx / SCORE_SAMPLES_PER_BYTE;
                    uint32_t sampleOffset = SCORE_SAMPLE_BITS * ( sIdx % SCORE_SAMPLES_PER_BYTE );

                    uint8_t sampleData = ( pState->scoreSamples[sampleIdx] >> sampleOffset ) & 0b11;
                    for( uint8_t team = ssn::team::left; team <= ssn::team::right; team++ ) {
                        if( sampleData & (1 << team) ) {
                            pState->scoreTotals[team] += csTallyDA;
                        }
                    }
                }
            }
        }

        return true;
    };
    const static auto csUpdateOutro = []
            ( ssn::state_t* pState, ssn::input_t* pInput, const float64_t pDT, const float64_t pPT ) -> bool32_t  {
        return true;
    };
    const static update_f csUpdateFuns[] = { csUpdateIntro, csUpdateTally, csUpdateOutro };

    bool32_t phaseResult = true;

    float64_t phaseMin = 0.0, phaseMax = 0.0;
    for( uint32_t phaseIdx = 0; phaseIdx < LLCE_ELEM_COUNT(SCORE_PHASE_DURATIONS); phaseIdx++ ) {
        phaseMax = phaseMin + SCORE_PHASE_DURATIONS[phaseIdx];
        if( phaseMin <= pState->st && pState->st < phaseMax ) {
            phaseResult = csUpdateFuns[phaseIdx]( pState, pInput, pDT, pState->st - phaseMin );
        }
        phaseMin = phaseMax;
    }

    if( pState->st >= phaseMax ) {
        pState->pmode = ssn::mode::reset::ID;
    }

    return phaseResult;
}


bool32_t score::render( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) {
    const static auto csRenderIntro = []
            ( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) -> bool32_t {
        const static float32_t csTextPadding = 5.0e-2f;
        const static vec2f32_t csTextPos( 0.5f, 0.5f );
        const static vec2f32_t csTextDims( 1.0f - 2.0f * csTextPadding, 1.0f - 2.0f * csTextPadding );

        llce::gfx::color_context_t textCC( &ssn::color::INFO );
        llce::gfx::render::text( "GAME!",
            llce::box_t(csTextPos, csTextDims, llce::geom::anchor2D::mm) );

        return true;
    };
    const static auto csRenderTally = []
            ( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) -> bool32_t  {
        llce::gfx::color_context_t tallyCC( &ssn::color::INFOLL );

        { // Render Tally Regions //
            const static float32_t csTallyWidth = 1.0e-2f, csTallyHeight = 1.0f;

            for( uint32_t tallyIdx = 0; tallyIdx < 2; tallyIdx++ ) {
                tallyCC.update( &ssn::color::INFOLL );
                llce::gfx::render::box( llce::box_t(
                    pState->tallyPoss[tallyIdx], 0.0f, 1.0f, 1.0f,
                    tallyIdx ? llce::geom::anchor2D::ll : llce::geom::anchor2D::hl) );

                tallyCC.update( &ssn::color::INFOL );
                float32_t tallyMidDist = glm::abs( pState->tallyPoss[tallyIdx] - 0.5f );
                if( tallyMidDist > csTallyWidth ) {
                    llce::gfx::render::box( llce::box_t(
                        pState->tallyPoss[tallyIdx], 0.0f,
                        csTallyWidth, csTallyHeight, llce::geom::anchor2D::ml) );
                } else {
                    llce::gfx::render::box( llce::box_t(
                        0.5f, 0.0f, tallyMidDist, csTallyHeight,
                        tallyIdx ? llce::geom::anchor2D::ll : llce::geom::anchor2D::hl) );
                }
            }
        }

        { // Render Header Text //
            const static float32_t csHeaderPadding = 0.05f;
            const static vec2f32_t csHeaderDims = { 1.0f - 2.0f * csHeaderPadding, 0.25f };
            const static vec2f32_t csHeaderPos = { csHeaderPadding, 1.0f - csHeaderPadding - csHeaderDims.y };

            tallyCC.update( &ssn::color::INFO );
            llce::gfx::render::text( "SCORES!", llce::box_t(csHeaderPos, csHeaderDims) );
        }

        { // Render Progress Bar //
            const static float32_t csProgressPadding = 0.1f;
            const static vec2f32_t csProgressBarDims = { 1.0f - 2.0f * csProgressPadding, 0.1f };
            const static vec2f32_t csProgressBarPos = { csProgressPadding, csProgressPadding };

            llce::gfx::render_context_t progressRC( llce::box_t(csProgressBarPos, csProgressBarDims) );
            tallyCC.update( &ssn::color::FOREGROUND );
            llce::gfx::render::box();

            for( uint8_t team = ssn::team::left; team <= ssn::team::right; team++ ) {
                const float32_t cTeamScore = pState->scoreTotals[team] / pState->bounds.mBBox.area();

                llce::box_t teamBox(
                    team == ssn::team::left ? 0.0f : 1.0f, 0.0f, cTeamScore, 1.0f,
                    team == ssn::team::left ? llce::geom::anchor2D::ll : llce::geom::anchor2D::hl );

                tallyCC.update( &ssn::color::TEAM[team] );
                llce::gfx::render::box( teamBox );

                char8_t teamText[8];
                std::snprintf( &teamText[0], LLCE_ELEM_COUNT(teamText), "%0.2f%%",
                    glm::clamp(100.0f * cTeamScore, 0.0f, 100.0f) );

                tallyCC.update( &ssn::color::INFO );
                llce::gfx::render::text( teamText, llce::box_t(
                    teamBox.mid(),
                    vec2f32_t(teamBox.mDims.x, 0.30f * teamBox.mDims.y),
                    llce::geom::anchor2D::mm) );
            }

            tallyCC.update( &ssn::color::INFO );
            llce::gfx::render::border( 1.0e-2f, 0 );
        }

        return true;
    };
    const static auto csRenderOutro = csRenderTally;
    const static render_f csRenderFuns[] = { csRenderIntro, csRenderTally, csRenderOutro };

    bool32_t phaseResult = true;

    gameboard_render( pState, pInput, pOutput );

    float64_t phaseMin = 0.0, phaseMax = 0.0;
    for( uint32_t phaseIdx = 0; phaseIdx < LLCE_ELEM_COUNT(SCORE_PHASE_DURATIONS); phaseIdx++ ) {
        phaseMax = phaseMin + SCORE_PHASE_DURATIONS[phaseIdx];
        if( phaseMin <= pState->st && pState->st < phaseMax ) {
            phaseResult = csRenderFuns[phaseIdx]( pState, pInput, pOutput );
        }
        phaseMin = phaseMax;
    }

    return phaseResult;
}

/// 'ssn::mode::reset' Functions  ///

bool32_t reset::init( ssn::state_t* pState, ssn::input_t* pInput ) {
    auto cResetItems = llce::util::pointerize( RESET_ITEM_TEXT );
    pState->resetMenu = llce::gui::menu_t(
        pInput, &MENU_ACTIONS[0],
        "GAME!", cResetItems.data(), cResetItems.size(),
        &ssn::color::BACKGROUND,
        &ssn::color::FOREGROUND,
        &ssn::color::TEAM[ssn::team::neutral] );
    pState->resetMenuUpdated = false;

    return true;
}


bool32_t reset::update( ssn::state_t* pState, ssn::input_t* pInput, const float64_t pDT ) {
    pState->resetMenu.update( pDT );

    if( pState->resetMenu.changed(llce::gui::event::select) ) {
        if( pState->resetMenu.mItemIndex == 0 ) {
            pState->pmode = ssn::mode::select::ID;
        } else if( pState->resetMenu.mItemIndex == 1 ) {
            pState->pmode = ssn::mode::title::ID;
        }
    }

    if( !pState->resetMenuUpdated ) { // Set Render Header Based on Winner //
        const float32_t* cScores = &pState->scoreTotals[0];
        const char8_t cTeamNames[3][8] = { "LEFT", "RIGHT", "NOBODY" };
        const auto cTeamWinner = (
            (cScores[ssn::team::left] > cScores[ssn::team::right]) ? ssn::team::left : (
            (cScores[ssn::team::left] < cScores[ssn::team::right]) ? ssn::team::right : (
            ssn::team::neutral)) );

        char8_t headerText[16];
        std::snprintf( &headerText[0], sizeof(headerText),
            "%s WINS!", &cTeamNames[cTeamWinner][0] );
        const color4u8_t* headerColor = &ssn::color::TEAM[cTeamWinner];

        std::strcpy( &pState->resetMenu.mTitle[0], &headerText[0] );
        pState->resetMenu.mColorText = headerColor;
        pState->resetMenuUpdated = true;
    }

    return true;
}


bool32_t reset::render( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) {
    pState->resetMenu.render();

    return true;
}

}

}
