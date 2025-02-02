#include "../game_levels.h"
#include "../../../include/texture_loader.h"
#include "../../../include/sound_system.h"
#include "../../../include/text_renderer.h"
#include "../../common.h"
#include "level_common.h"
#include <stdlib.h>
#include <math.h>

#define BANKI_WIDTH 64.0f
#define BANKI_HEIGHT 64.0f

#define MOVE_SPEED 8.0f
#define GRAVITY 0.3f
#define INITIAL_FALL_SPEED 2.0f
#define BOUNCE_SPEED -12.0f
#define HORIZONTAL_BOUNCE_SPEED 6.0f
#define MAX_BOUNCE_HEIGHT (-OFFSCREEN_HEIGHT)

#define TRAMPOLINE_WIDTH 64.0f
#define TRAMPOLINE_HEIGHT 32.0f

#define SCREEN_TOTAL_HEIGHT (SCREEN_HEIGHT + SCREEN_HEIGHT_BOTTOM)
#define OFFSCREEN_HEIGHT 64.0f

#define ROTATION_SPEED 0.05f
#define MAX_ROTATION_SPEED (M_PI / 8.0f)

#define TEXTURE_PATH_TRAMPOLINE "romfs:/textures/spr_m1_6_tikuwa_0.t3x"
#define TEXTURE_PATH_BANKI_FORMAT "romfs:/textures/spr_m1_6_banki_%d.t3x"

typedef struct BounceCatchData {
    bool initialized;
    bool gameOver;
    bool gameDecided;
    bool success;
    float offsetX;
    float offsetY;

    float lastPressedDirection;
    float playerX;
    bool isHoldingTrampoline;

    float bankiX;
    float bankiY;
    float bankiVelocityY;
    float bankiVelocityX;
    float bankiRotation;
    float bankiRotationVelocity;
    int currentBankiFrame;
    bool isBouncing;
} BounceCatchData;

static void bounceCatchReset(BounceCatchData* levelData) {
    if (levelData == NULL) return;

    levelData->gameOver = false;
    levelData->gameDecided = false;
    levelData->success = false;
    levelData->offsetX = 0.0f;
    levelData->offsetY = 0.0f;

    levelData->lastPressedDirection = 1.0f;
    levelData->isHoldingTrampoline = false;
    levelData->playerX = (SCREEN_WIDTH_BOTTOM - TRAMPOLINE_WIDTH) / 2;
    
    // Start from top screen
    levelData->bankiX = (SCREEN_WIDTH - BANKI_WIDTH) / 2;
    levelData->bankiY = -OFFSCREEN_HEIGHT;
    levelData->bankiVelocityY = INITIAL_FALL_SPEED;
    levelData->bankiVelocityX = 0.0f;
    levelData->bankiRotation = 0.0f;
    levelData->bankiRotationVelocity = 0.0f;
    levelData->currentBankiFrame = 0;
    levelData->isBouncing = false;
}

static void bounceCatchInit(GameSceneData* data) {
    BounceCatchData* levelData = malloc(sizeof(BounceCatchData));
    if (levelData == NULL) {
        panicEverything("Failed to allocate memory for BounceCatchData");
        return;
    }
    bounceCatchReset(levelData);
    data->currentLevelData = levelData;
    
    data->gameLeftTime = data->gameSessionTime;
    data->lastGameState = GAME_SUCCESS;
    playWavFromRomfs("romfs:/sounds/bgm_microgame2.wav");
}

static void bounceCatchCheckBounce(GameSceneData* data) {
    BounceCatchData* levelData = (BounceCatchData*)data->currentLevelData;
    if (levelData == NULL || levelData->gameDecided) return;

    float bankiXCenter = levelData->bankiX + (BANKI_WIDTH / 2);
    float playerXCenter = levelData->playerX + (TRAMPOLINE_WIDTH / 2);
    float dx = fabsf(bankiXCenter - playerXCenter);
    
    // Calculate banki's position relative to bottom screen
    float bankiBottomScreenY = levelData->bankiY - (SCREEN_HEIGHT + OFFSCREEN_HEIGHT);
    
    // Check collision with trampoline
    if (levelData->bankiVelocityY > 0 && // Only bounce when falling
        bankiBottomScreenY >= SCREEN_HEIGHT_BOTTOM - TRAMPOLINE_HEIGHT - BANKI_HEIGHT &&
        bankiBottomScreenY <= SCREEN_HEIGHT_BOTTOM - TRAMPOLINE_HEIGHT &&
        dx < (TRAMPOLINE_WIDTH / 2)) {
        
        // Random rotation velocity on bounce
        float randomRotationDir = (frand() > 0.5f) ? 1.0f : -1.0f;
        levelData->bankiRotationVelocity = (frand() * MAX_ROTATION_SPEED + ROTATION_SPEED) * randomRotationDir;

        // Random horizontal velocity on bounce
        float randomDirection = (frand() > 0.5f) ? 1.0f : -1.0f;
        float xVelocity = HORIZONTAL_BOUNCE_SPEED * frand() * randomDirection;
        
        // Check if this velocity would send banki off-screen and adjust if needed
        float predictedX = levelData->bankiX + xVelocity * 10; // Predict future position
        if (predictedX < 0 || predictedX > SCREEN_WIDTH_BOTTOM - BANKI_WIDTH) {
            xVelocity *= -1; // Reverse direction if it would go off-screen
        }
        
        levelData->bankiVelocityX = xVelocity;
        levelData->bankiVelocityY = BOUNCE_SPEED;
        levelData->isBouncing = true;
        levelData->currentBankiFrame = (levelData->currentBankiFrame + 1) % 10;
        playWavLayered("romfs:/sounds/se_poyon2.wav");
    }
}

static void bounceCatchTriggerFail(GameSceneData* data) {
    BounceCatchData* levelData = (BounceCatchData*)data->currentLevelData;
    if (levelData == NULL || levelData->gameDecided) return;

    levelData->gameDecided = true;
    levelData->gameOver = true;
    levelData->success = false;
    data->lastGameState = GAME_FAILURE;
    playWavLayered("romfs:/sounds/se_huseikai.wav");
}

static void bounceCatchUpdate(GameSceneData* data, float deltaTime) {
    BounceCatchData* levelData = (BounceCatchData*)data->currentLevelData;
    if (levelData == NULL) return;
    
    // Update background scroll
    levelData->offsetX += SCROLL_SPEED;
    levelData->offsetY += SCROLL_SPEED;

    if (levelData->offsetX <= -64.0f) levelData->offsetX = 0.0f;
    if (levelData->offsetY <= -64.0f) levelData->offsetY = 0.0f;
    if (levelData->offsetX >= 64.0f) levelData->offsetX = 0.0f;
    if (levelData->offsetY >= 64.0f) levelData->offsetY = 0.0f;

    if (!levelData->gameOver) {
        // Apply gravity
        levelData->bankiVelocityY += GRAVITY;
        levelData->bankiY += levelData->bankiVelocityY;
        
        // Apply horizontal movement
        levelData->bankiX += levelData->bankiVelocityX;
        
        // Apply rotation
        levelData->bankiRotation += levelData->bankiRotationVelocity;
        
        // Keep rotation in bounds
        while (levelData->bankiRotation > M_PI * 2) {
            levelData->bankiRotation -= M_PI * 2;
        }
        while (levelData->bankiRotation < 0) {
            levelData->bankiRotation += M_PI * 2;
        }
        
        // Bounce off screen edges
        if (levelData->bankiX < 0) {
            levelData->bankiX = 0;
            levelData->bankiVelocityX *= -0.8f; // Bounce with some energy loss
            levelData->bankiRotationVelocity *= -1.0f; // Reverse rotation on wall hit
        } else if (levelData->bankiX > SCREEN_WIDTH_BOTTOM - BANKI_WIDTH) {
            levelData->bankiX = SCREEN_WIDTH_BOTTOM - BANKI_WIDTH;
            levelData->bankiVelocityX *= -0.8f; // Bounce with some energy loss
            levelData->bankiRotationVelocity *= -1.0f; // Reverse rotation on wall hit
        }

        // Check for bounce
        bounceCatchCheckBounce(data);

        // Check if banki fell off the bottom screen
        float bankiBottomScreenY = levelData->bankiY - (SCREEN_HEIGHT + OFFSCREEN_HEIGHT);
        if (bankiBottomScreenY > SCREEN_HEIGHT_BOTTOM) {
            bounceCatchTriggerFail(data);
        }
    }
}

static void bounceCatchDraw(GameSceneData* data, const GraphicsContext* context) {
    BounceCatchData* levelData = (BounceCatchData*)data->currentLevelData;
    if (levelData == NULL) return;

    // Draw top screen
    C2D_SceneBegin(context->top);
    C2D_TargetClear(context->top, C2D_Color32(0, 0, 0, 255));

    // Draw background on top screen
    char targetFile[64];
    int fileId = 2;
    if (data->lastGameState == GAME_FAILURE) {
        fileId = 4;
    }
    snprintf(targetFile, sizeof(targetFile), "romfs:/textures/bg_%d_0.t3x", fileId);
    displayTiledImage(targetFile, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, levelData->offsetX, levelData->offsetY);

    // Draw banki on top screen if in range
    if (levelData->bankiY >= -OFFSCREEN_HEIGHT && levelData->bankiY <= SCREEN_HEIGHT) {
        char bankiTexturePath[64];
        snprintf(bankiTexturePath, sizeof(bankiTexturePath), TEXTURE_PATH_BANKI_FORMAT, levelData->currentBankiFrame);
        C2D_ImageTint tint;
        C2D_PlainImageTint(&tint, 0xFFFFFFFF, 1.0f);
        displayImageWithScalingAndRotation(bankiTexturePath, levelData->bankiX, levelData->bankiY, NULL, 1.0f, 1.0f, levelData->bankiRotation);
    }
    
    // Draw bottom screen
    C2D_SceneBegin(context->bottom);
    C2D_TargetClear(context->bottom, C2D_Color32(0, 0, 0, 255));

    // Draw background for bottom
    displayTiledImage(targetFile, 0, 0, SCREEN_WIDTH_BOTTOM, SCREEN_HEIGHT_BOTTOM, levelData->offsetX, levelData->offsetY);

    // Draw trampoline
    displayImage(TEXTURE_PATH_TRAMPOLINE, levelData->playerX, SCREEN_HEIGHT_BOTTOM - TRAMPOLINE_HEIGHT);

    // Draw banki on bottom screen if in range
    float bankiBottomScreenY = levelData->bankiY - (SCREEN_HEIGHT + OFFSCREEN_HEIGHT);
    if (bankiBottomScreenY >= -BANKI_HEIGHT && bankiBottomScreenY <= SCREEN_HEIGHT_BOTTOM) {
        char bankiTexturePath[64];
        snprintf(bankiTexturePath, sizeof(bankiTexturePath), TEXTURE_PATH_BANKI_FORMAT, levelData->currentBankiFrame);
        displayImageWithScalingAndRotation(bankiTexturePath, levelData->bankiX, bankiBottomScreenY, NULL, 1.0f, 1.0f, levelData->bankiRotation);
    }
}

static void bounceCatchHandleInput(GameSceneData* data, const InputState* input) {
    BounceCatchData* levelData = (BounceCatchData*)data->currentLevelData;
    if (levelData == NULL || levelData->gameDecided) return;
    
    float targetX = levelData->playerX;

    // Handle D-pad and Circle Pad movement
    if (input->kHeld & (KEY_DRIGHT | KEY_RIGHT)) {
        targetX += MOVE_SPEED;
        levelData->lastPressedDirection = -1.0f;
    } else if (input->kHeld & (KEY_DLEFT | KEY_LEFT)) {
        targetX -= MOVE_SPEED;
        levelData->lastPressedDirection = 1.0f;
    }

    // Handle touch screen input
    if (!levelData->isHoldingTrampoline) {
        if (input->kDown & KEY_TOUCH) {
            if (input->touch.px >= levelData->playerX && 
                input->touch.px <= levelData->playerX + TRAMPOLINE_WIDTH &&
                input->touch.py >= SCREEN_HEIGHT_BOTTOM - TRAMPOLINE_HEIGHT &&
                input->touch.py <= SCREEN_HEIGHT_BOTTOM) {
                levelData->isHoldingTrampoline = true;
            }
        }
    } else {
        if (input->kUp & KEY_TOUCH) {
            levelData->isHoldingTrampoline = false;
        } else if (input->touch.px > 0 && input->touch.py > 0) {
            targetX = input->touch.px - (TRAMPOLINE_WIDTH / 2);
        }
    }

    // Limit movement to screen bounds
    if (targetX < 0) {
        targetX = 0;
    } else if (targetX > SCREEN_WIDTH_BOTTOM - TRAMPOLINE_WIDTH) {
        targetX = SCREEN_WIDTH_BOTTOM - TRAMPOLINE_WIDTH;
    }

    levelData->playerX = targetX;
}

static void bounceCatchResetGame(GameSceneData* data) {
    BounceCatchData* levelData = (BounceCatchData*)data->currentLevelData;
    if (levelData) {
        bounceCatchReset(levelData);
    }
}

const GameLevel BounceCatchGame = {
    .init = bounceCatchInit,
    .update = bounceCatchUpdate,
    .draw = bounceCatchDraw,
    .handleInput = bounceCatchHandleInput,
    .reset = bounceCatchResetGame,
    .requestingQuit = NULL,
};