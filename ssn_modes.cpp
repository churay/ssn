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

typedef bool32_t (*update_f)( ssn::state_t*, ssn::input_t*, const float64_t, const float64_t );
typedef bool32_t (*render_f)( const ssn::state_t*, const ssn::input_t*, const ssn::output_t* );

constexpr static float64_t SCORE_PHASE_DURATIONS[] = { 1.0, 2.0, 1.0 };

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
    //     // const vec2f32_t cTestAreas[2][3] = {
    //     //     {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}},
    //     //     {{0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}} };
    //     // const ssn::team::team_e cTestTeams[] = {
    //     //     ssn::team::left,   // l score: 0.5
    //     //     ssn::team::left }; // l score: 0.5
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
            const llce::interval_t xbounds = bounds->mBBox.xbounds();
            const llce::interval_t ybounds = bounds->mBBox.ybounds();

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
    for( uint32_t phaseIdx = 0; phaseIdx < ARRAY_LEN(SCORE_PHASE_DURATIONS); phaseIdx++ ) {
        phaseMax = phaseMin + SCORE_PHASE_DURATIONS[phaseIdx];
        if( phaseMin <= pState->st && pState->st < phaseMax ) {
            phaseResult = csUpdateFuns[phaseIdx]( pState, pInput, pDT, pState->st - phaseMin );
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
        const static float32_t csTextPadding = 5.0e-2f;
        const static vec2f32_t csTextPos( 0.5f, 0.5f );
        const static vec2f32_t csTextDims( 1.0f - 2.0f * csTextPadding, 1.0f - 2.0f * csTextPadding );

        llce::gfx::text::render( "GAME!", &ssn::color::INFO,
            llce::box_t(csTextPos, csTextDims, llce::geom::anchor2D::mm) );

        return true;
    };
    const static auto csRenderTally = []
            ( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) -> bool32_t  {
        { // Render Tally Regions //
            const static float32_t csTallyWidth = 1.0e-2f, csTallyHeight = 1.0f;

            for( uint32_t tallyIdx = 0; tallyIdx < 2; tallyIdx++ ) {
                llce::gfx::box::render( llce::box_t(
                        pState->tallyPoss[tallyIdx], 0.0f, 1.0f, 1.0f,
                        tallyIdx ? llce::geom::anchor2D::ll : llce::geom::anchor2D::hl),
                    &ssn::color::INFOLL );

                float32_t tallyMidDist = glm::abs( pState->tallyPoss[tallyIdx] - 0.5f );
                if( tallyMidDist > csTallyWidth ) {
                    llce::gfx::box::render( llce::box_t(
                            pState->tallyPoss[tallyIdx], 0.0f,
                            csTallyWidth, csTallyHeight, llce::geom::anchor2D::ml),
                        &ssn::color::INFOL );
                } else {
                    llce::gfx::box::render( llce::box_t(
                            0.5f, 0.0f, tallyMidDist, csTallyHeight,
                            tallyIdx ? llce::geom::anchor2D::ll : llce::geom::anchor2D::hl),
                        &ssn::color::INFOL );
                }
            }
        }

        { // Render Header Text //
            const static float32_t csHeaderPadding = 0.05f;
            const static vec2f32_t csHeaderDims = { 1.0f - 2.0f * csHeaderPadding, 0.25f };
            const static vec2f32_t csHeaderPos = { csHeaderPadding, 1.0f - csHeaderPadding - csHeaderDims.y };

            llce::gfx::text::render( "SCORES!", &ssn::color::INFO,
                llce::box_t(csHeaderPos, csHeaderDims) );
        }

        { // Render Progress Bar //
            const static float32_t csProgressPadding = 0.1f;
            const static vec2f32_t csProgressBarDims = { 1.0f - 2.0f * csProgressPadding, 0.1f };
            const static vec2f32_t csProgressBarPos = { csProgressPadding, csProgressPadding };

            const static float32_t csProgressBarAspect = llce::gfx::aspect( csProgressBarDims );
            const static vec2f32_t csProgressBorderDims = { 1.0e-2f * csProgressBarAspect, 1.0e-2f };

            llce::gfx::render_context_t progressRC(
                llce::box_t(csProgressBarPos, csProgressBarDims), &ssn::color::FOREGROUND );
            progressRC.render();

            for( uint8_t team = ssn::team::left; team <= ssn::team::right; team++ ) {
                llce::box_t teamBox(
                    team == ssn::team::left ? 0.0f : 1.0f, 0.0f,
                    pState->scoreTotals[team], 1.0f,
                    team == ssn::team::left ? llce::geom::anchor2D::ll : llce::geom::anchor2D::hl );
                llce::gfx::box::render( teamBox, &ssn::color::TEAM[team] );

                char8_t teamText[8];
                std::snprintf( &teamText[0], ARRAY_LEN(teamText), "%0.2f%%",
                    glm::clamp(100.0f * pState->scoreTotals[team], 0.0f, 100.0f) );
                llce::gfx::text::render( teamText, &ssn::color::INFO,
                    llce::box_t(teamBox.center(),
                        vec2f32_t(teamBox.mDims.x, 0.30f * teamBox.mDims.y),
                        llce::geom::anchor2D::mm) );
            }

            // TODO(JRC): Factor this code so that it isn't duplicated from the
            // 'game::render' border rendering functionality.
            mat4f32_t sideSpace( 1.0f );
            for( uint32_t sideIdx = 0; sideIdx < 4; sideIdx++ ) {
                sideSpace = glm::translate( vec3f32_t(0.0f, 1.0f, 0.0f) ) *
                    glm::rotate( -glm::half_pi<float32_t>(), vec3f32_t(0.0f, 0.0f, 1.0f) );
                glMultMatrixf( &sideSpace[0][0] );

                // NOTE(JRC): Different widths are needed for the different axes
                // because the context (i.e. the progress bar) has a skewed aspect.
                float32_t sideWidth = *VECTOR_AT( csProgressBorderDims, sideIdx % 2 );
                glBegin( GL_QUADS ); {
                    glVertex2f( 0.0f, 0.0f );
                    glVertex2f( 0.0f, 1.0f );
                    glVertex2f( sideWidth, 1.0f );
                    glVertex2f( sideWidth, 0.0f );
                } glEnd();
            }
        }

        return true;
    };
    const static auto csRenderOutro = csRenderTally;
    const static render_f csRenderFuns[] = { csRenderIntro, csRenderTally, csRenderOutro };

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

    { // Set Render Header Based on Winner //
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
        pState->resetMenu.mTitleColor = headerColor;
    }

    return true;
}


bool32_t reset::render( const ssn::state_t* pState, const ssn::input_t* pInput, const ssn::output_t* pOutput ) {
    pState->resetMenu.render();

    return true;
}

}

}
