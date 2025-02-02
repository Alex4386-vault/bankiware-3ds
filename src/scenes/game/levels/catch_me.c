#include "../game_levels.h"
#include "../../../include/texture_loader.h"
#include "../../../include/sound_system.h"
#include "../../../include/text_renderer.h"
#include "../../common.h"
#include "level_common.h"
#include <stdlib.h>

#define OFFSCREEN_HEIGHT 64.0f
#define BANKI_WIDTH 64.0f
#define BANKI_HEIGHT 64.0f
#define BANKI_FULLHEIGHT (BANKI_HEIGHT * 2.0f)

#define DROP_SPEED 3.0f
#define MOVE_SPEED 5.0f

#define BANKI_DROPX_MIN ((SCREEN_WIDTH / 2) - (SCREEN_WIDTH_BOTTOM / 2))
#define BANKI_DROPX_MAX (((SCREEN_WIDTH / 2) + (SCREEN_WIDTH_BOTTOM / 2)) - (2 * BANKI_WIDTH))

#define BANKI_DROPY_MAX_BOTTOM_SCREEN (SCREEN_HEIGHT_BOTTOM - BANKI_HEIGHT)
#define BANKI_DROPY_MAX (SCREEN_HEIGHT + OFFSCREEN_HEIGHT + BANKI_DROPY_MAX_BOTTOM_SCREEN)
#define BANKI_DROPY_THRESHOLD (BANKI_DROPY_MAX - BANKI_HEIGHT)

#define TEXTURE_PATH_BANKIHEAD "romfs:/textures/spr_m1_2_bankihead_0.t3x"
#define TEXTURE_PATH_BANKIBODY "romfs:/textures/spr_m1_2_bankibody_0.t3x"
#define TEXTURE_PATH_BANKIFULLBODY "romfs:/textures/spr_m1_2_bankibody_1.t3x"

#define BANKI_FULLBODY_MULTIPLIER 0.675f
#define BANKI_BODY_MULTIPLIER 1.0f

typedef struct CatchMeData {
    bool initialized;
    bool gameOver;
    bool gameDecided;
    bool success;
    float offsetX;
    float offsetY;

    float lastPressedDirection;

    float dropX;
    float dropY;

    float playerX;
    bool isHoldingBanki;
} CatchMeData;

static void catchMeCheckPlayerCatch(GameSceneData *data) {
    CatchMeData* levelData = (CatchMeData*)data->currentLevelData;
    if (levelData == NULL || levelData->gameDecided) {
        return;
    }

    // check if the player have caught (the dropX is within the range of the player)
    float dropXCenter = levelData->dropX + (BANKI_WIDTH / 2);
    float playerXCenter = levelData->playerX + (BANKI_WIDTH / 2);

    float dx = fabsf(dropXCenter - playerXCenter);
    if (dx < (BANKI_WIDTH / 2)) {
        levelData->gameDecided = true;
        levelData->gameOver = true;
        levelData->success = true;
        data->lastGameState = GAME_SUCCESS;

        playWavLayered("romfs:/sounds/se_rappa.wav");
    }
}

static void catchMeTriggerFail(GameSceneData *data) {
    CatchMeData* levelData = (CatchMeData*)data->currentLevelData;
    if (levelData == NULL || levelData->gameDecided) {
        return;
    }

    levelData->success = false;

    if (data->lastGameState == GAME_UNDEFINED) {
        // the game was not decided, but it is now failed
        playWavLayered("romfs:/sounds/se_poyon1.wav");
    }
    data->lastGameState = GAME_FAILURE;
}

static void catchMeReset(CatchMeData* levelData) {
    if (levelData == NULL) return;

    levelData->gameOver = false;
    levelData->gameDecided = false;
    levelData->success = false;
    levelData->offsetX = 0.0f;
    levelData->offsetY = 0.0f;

    levelData->lastPressedDirection = 1.0f;
    levelData->isHoldingBanki = false;
    levelData->playerX = (SCREEN_WIDTH_BOTTOM - BANKI_WIDTH) / 2;
    
    // dropping position
    levelData->dropX = BANKI_DROPX_MIN + (frand() * (BANKI_DROPX_MAX - BANKI_DROPX_MIN));
    levelData->dropY = -OFFSCREEN_HEIGHT;
}

static void catchMeInit(GameSceneData* data) {
    CatchMeData* levelData = malloc(sizeof(CatchMeData));
    if (levelData == NULL) {
        panicEverything("Failed to allocate memory for CatchMeData");
        return;
    }
    catchMeReset(levelData);
    data->currentLevelData = levelData;
    
    data->gameLeftTime = data->gameSessionTime;
    playWavFromRomfs("romfs:/sounds/bgm_microgame2.wav");
}

static void catchMeUpdate(GameSceneData* data, float deltaTime) {
    CatchMeData* levelData = (CatchMeData*)data->currentLevelData;
    if (levelData == NULL) return;
    
    // Update background scroll
    levelData->offsetX += SCROLL_SPEED;
    levelData->offsetY += SCROLL_SPEED;

    // Wrap offsets to match texture size
    if (levelData->offsetX <= -64.0f) levelData->offsetX = 0.0f;
    if (levelData->offsetY <= -64.0f) levelData->offsetY = 0.0f;
    if (levelData->offsetX >= 64.0f) levelData->offsetX = 0.0f;
    if (levelData->offsetY >= 64.0f) levelData->offsetY = 0.0f;

    if (!levelData->gameOver) {
        float dy = fabs(levelData->dropY - BANKI_DROPY_THRESHOLD);
        if (levelData->dropY >= BANKI_DROPY_THRESHOLD - (BANKI_HEIGHT / 16)) {
            if (levelData->dropY < BANKI_DROPY_THRESHOLD + (BANKI_HEIGHT / 8)) {
                catchMeCheckPlayerCatch(data);
            } else if (levelData->dropY > BANKI_DROPY_THRESHOLD + (BANKI_HEIGHT * 2)) {
                // the drop is entirely out of screen., trigger shutdown
                levelData->gameDecided = true;
                levelData->gameOver = true;
                catchMeTriggerFail(data);
            } else {
                // game has decided. the drop is out of the range of the player.
                levelData->gameDecided = true;
                catchMeTriggerFail(data);
            }
        }

        // Update dropping position
        levelData->dropY += DROP_SPEED;
    }
}

static void catchMeDraw(GameSceneData* data, const GraphicsContext* context) {
    CatchMeData* levelData = (CatchMeData*)data->currentLevelData;

    // Draw top screen
    C2D_SceneBegin(context->top);
    C2D_TargetClear(context->top, C2D_Color32(0, 0, 0, 255));

    // draw a background
    levelCommonDraw(data, SCREEN_WIDTH, SCREEN_HEIGHT, levelData->offsetX, levelData->offsetY);
    if (!levelData->gameDecided) {
        if (levelData->dropY < SCREEN_HEIGHT + BANKI_HEIGHT) {
            // draw the drop. add banki dropX min so the drop is aligned with bottom screen
            displayImage(TEXTURE_PATH_BANKIHEAD, levelData->dropX + BANKI_DROPX_MIN, levelData->dropY);
        }
    }
    
    // Draw bottom screen
    C2D_SceneBegin(context->bottom);
    C2D_TargetClear(context->bottom, C2D_Color32(0, 0, 0, 255));

    // draw a background for bottom
    levelCommonDraw(data, SCREEN_WIDTH_BOTTOM, SCREEN_HEIGHT_BOTTOM, levelData->offsetX, levelData->offsetY);

    if (levelData->gameOver) {
        // draw in direction
        if (levelData->success) {
            displayImageWithScaling(TEXTURE_PATH_BANKIFULLBODY, levelData->playerX, SCREEN_HEIGHT_BOTTOM - (BANKI_FULLHEIGHT * (1.5f * BANKI_FULLBODY_MULTIPLIER)), NULL, levelData->lastPressedDirection * BANKI_FULLBODY_MULTIPLIER, 1.0f * BANKI_FULLBODY_MULTIPLIER);
        } else {
            displayImageWithScaling(TEXTURE_PATH_BANKIBODY, levelData->playerX - ((BANKI_BODY_MULTIPLIER - 1.0f) * BANKI_WIDTH), SCREEN_HEIGHT_BOTTOM - (BANKI_HEIGHT * 0.9), NULL, levelData->lastPressedDirection * BANKI_BODY_MULTIPLIER, 1.0f * BANKI_BODY_MULTIPLIER);
        }
    } else {
        float drawDropY = levelData->dropY - (SCREEN_HEIGHT + OFFSCREEN_HEIGHT);
        if (levelData->dropY > -BANKI_HEIGHT) {
            displayImageWithScaling(TEXTURE_PATH_BANKIHEAD, levelData->dropX, drawDropY, NULL, 1.0f, 1.0f);
        }
        displayImageWithScaling(TEXTURE_PATH_BANKIBODY, levelData->playerX - ((BANKI_BODY_MULTIPLIER - 1.0f) * BANKI_WIDTH), SCREEN_HEIGHT_BOTTOM - (BANKI_HEIGHT * 0.9), NULL, levelData->lastPressedDirection * BANKI_BODY_MULTIPLIER, 1.0f * BANKI_BODY_MULTIPLIER);
    }
}

static void catchMeHandleInput(GameSceneData* data, const InputState* input) {
    CatchMeData* levelData = (CatchMeData*)data->currentLevelData;
    if (levelData == NULL) return;
    
    if (!levelData->gameDecided) {
        // handle dpad/circlepad input
        float targetX = levelData->playerX;

        if (input->kHeld & (KEY_DRIGHT | KEY_RIGHT)) {
            targetX += MOVE_SPEED;
            levelData->lastPressedDirection = -1.0f;
        } else if (input->kHeld & (KEY_DLEFT | KEY_LEFT)) {
            targetX -= MOVE_SPEED;
            levelData->lastPressedDirection = 1.0f;
        }

        // check if the touchscreen is pressed
        if (!levelData->isHoldingBanki) {
            if (input->kDown & KEY_TOUCH) {
                // check if the touch is within the player's range
                if (input->touch.px >= levelData->playerX && input->touch.px <= levelData->playerX + BANKI_WIDTH) {
                    // check if the touch is within the player's range
                    if (input->touch.py >= SCREEN_HEIGHT_BOTTOM - BANKI_HEIGHT && input->touch.py <= SCREEN_HEIGHT_BOTTOM) {
                        levelData->isHoldingBanki = true;
                    }
                }
            }
        } else {
            // it is holding

            if (input->kUp & KEY_TOUCH) {
                levelData->isHoldingBanki = false;
            } else {
                if (input->touch.px > 0 && input->touch.py > 0) {
                    targetX = input->touch.px - (BANKI_WIDTH / 2);
                }
            }
        }

        // do the range limitation
        if (targetX < 0) {
            targetX = 0;
        } else if (targetX > SCREEN_WIDTH_BOTTOM - BANKI_WIDTH) {
            targetX = SCREEN_WIDTH_BOTTOM - BANKI_WIDTH;
        }

        levelData->playerX = targetX;
    }
}

static void catchMeResetGame(GameSceneData* data) {
    CatchMeData* levelData = (CatchMeData*)data->currentLevelData;
    if (levelData) {
        catchMeReset(levelData);
        levelData->initialized = false;

        levelData->offsetX = 0.0f;
        levelData->offsetY = 0.0f;
    }
}

const GameLevel CatchMeGame = {
    .init = catchMeInit,
    .update = catchMeUpdate,
    .draw = catchMeDraw,
    .handleInput = catchMeHandleInput,
    .reset = catchMeResetGame,
    .requestingQuit = NULL,
};