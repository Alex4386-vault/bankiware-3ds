#include "../game_levels.h"
#include "../../../include/texture_loader.h"
#include "../../../include/sound_system.h"
#include "../../../include/text_renderer.h"
#include "../../common.h"
#include "level_common.h"
#include <stdlib.h>

#define CAKE_WIDTH 256.0f
#define CAKE_HEIGHT 256.0f

#define BANKI_WIDTH 512.0f
#define BANKI_HEIGHT 256.0f

#define CAKE_TEXTURE_FORMAT "romfs:/textures/spr_m1_8_cake_%d.t3x"
#define BANKI_TEXTURE "romfs:/textures/spr_m1_8_banki_0.t3x"

typedef struct EatingCakeData {
    bool initialized;
    bool gameOver;
    bool gameDecided;
    bool success;
    float offsetX;
    float offsetY;
    
    int cakeState;
    bool isPressed;
    float pressAnimationTimer;
} EatingCakeData;

static void eatingCakeReset(EatingCakeData* levelData) {
    if (levelData == NULL) return;

    levelData->gameOver = false;
    levelData->gameDecided = false;
    levelData->success = false;
    levelData->offsetX = 0.0f;
    levelData->offsetY = 0.0f;
    
    levelData->cakeState = 0;
    levelData->isPressed = false;
    levelData->pressAnimationTimer = 0.0f;
}

static void eatingCakeInit(GameSceneData* data) {
    EatingCakeData* levelData = malloc(sizeof(EatingCakeData));
    if (levelData == NULL) {
        panicEverything("Failed to allocate memory for EatingCakeData");
        return;
    }
    eatingCakeReset(levelData);
    data->currentLevelData = levelData;
    
    data->gameLeftTime = data->gameSessionTime;
    playWavFromRomfs("romfs:/sounds/bgm_microgame2.wav");
}

static void eatingCakeTriggerSuccess(GameSceneData* data) {
    EatingCakeData* levelData = (EatingCakeData*)data->currentLevelData;
    if (levelData == NULL || levelData->gameDecided) return;

    levelData->gameDecided = true;
    levelData->gameOver = true;
    levelData->success = true;
    data->lastGameState = GAME_SUCCESS;
    playWavLayered("romfs:/sounds/se_seikai.wav");
}

static void eatingCakeUpdate(GameSceneData* data, float deltaTime) {
    EatingCakeData* levelData = (EatingCakeData*)data->currentLevelData;
    if (levelData == NULL) return;
    
    // Update background scroll
    levelData->offsetX += SCROLL_SPEED;
    levelData->offsetY += SCROLL_SPEED;

    if (levelData->offsetX <= -64.0f) levelData->offsetX = 0.0f;
    if (levelData->offsetY <= -64.0f) levelData->offsetY = 0.0f;
    if (levelData->offsetX >= 64.0f) levelData->offsetX = 0.0f;
    if (levelData->offsetY >= 64.0f) levelData->offsetY = 0.0f;

    if (levelData->isPressed) {
        levelData->pressAnimationTimer += deltaTime;
            
        // Advance cake state
        if (levelData->cakeState < 6) {
            levelData->cakeState++;
            playWavLayered("romfs:/sounds/se_eat.wav");
            
            // Check for win condition
            if (levelData->cakeState >= 6) {
                eatingCakeTriggerSuccess(data);
            }
        }
    }
}

static void eatingCakeDraw(GameSceneData* data, const GraphicsContext* context) {
    EatingCakeData* levelData = (EatingCakeData*)data->currentLevelData;
    if (levelData == NULL) return;

    // Draw top screen
    C2D_SceneBegin(context->top);
    C2D_TargetClear(context->top, C2D_Color32(255, 255, 255, 255));
    
    // Draw bottom screen
    C2D_SceneBegin(context->bottom);
    C2D_TargetClear(context->bottom, C2D_Color32(255, 255, 255, 255));

    // Draw cake with current state
    char cakeTexturePath[64];
    snprintf(cakeTexturePath, sizeof(cakeTexturePath), CAKE_TEXTURE_FORMAT, levelData->cakeState);
    
    float centerX = (SCREEN_WIDTH_BOTTOM - CAKE_WIDTH) / 2;
    float centerY = (SCREEN_HEIGHT_BOTTOM - CAKE_HEIGHT) / 2;
    
    if (levelData->cakeState > 2) {
        if (levelData->cakeState > 4) {
            centerY += (2 * SCREEN_HEIGHT_BOTTOM / 3);
        } else if (levelData->cakeState > 2) {
            centerY += (SCREEN_HEIGHT_BOTTOM / 3);
        }
        centerY -= 20;
    }
    
    displayImage(cakeTexturePath, centerX, centerY);

    // Draw success banki on top if game is won
    if (levelData->success) {
        displayImage(BANKI_TEXTURE, (SCREEN_WIDTH - BANKI_WIDTH) / 2, (SCREEN_HEIGHT - BANKI_HEIGHT) / 2);
    }
}

static void eatingCakeHandleEat(GameSceneData* data) {
    EatingCakeData* levelData = (EatingCakeData*)data->currentLevelData;
    if (levelData == NULL || levelData->gameDecided) return;
    
    // Play eating sound

    // Advance cake state
    if (levelData->cakeState <= 6) {
        levelData->cakeState++;
        
        // Check for win condition
        if (levelData->cakeState >= 7) {
            eatingCakeTriggerSuccess(data);
        } else {    
            playWavLayered("romfs:/sounds/se_eat.wav");
        }
    }
}

static void eatingCakeHandleInput(GameSceneData* data, const InputState* input) {
    EatingCakeData* levelData = (EatingCakeData*)data->currentLevelData;
    if (levelData == NULL || levelData->gameDecided) return;

    // Handle A button press
    if (input->kDown & KEY_A) {
        // Play eating sound
        eatingCakeHandleEat(data);
    }
    
    // Handle touch input
    if (input->kDown & KEY_TOUCH) {
        float centerX = (SCREEN_WIDTH_BOTTOM - CAKE_WIDTH) / 2;
        float centerY = (SCREEN_HEIGHT_BOTTOM - CAKE_HEIGHT) / 2;
        
        if (input->touch.px >= centerX &&
            input->touch.px <= centerX + CAKE_WIDTH &&
            input->touch.py >= centerY &&
            input->touch.py <= centerY + CAKE_HEIGHT) {
            eatingCakeHandleEat(data);
        }
    }
}

static void eatingCakeResetGame(GameSceneData* data) {
    EatingCakeData* levelData = (EatingCakeData*)data->currentLevelData;
    if (levelData) {
        eatingCakeReset(levelData);
    }
}

const GameLevel EatingCakeGame = {
    .init = eatingCakeInit,
    .update = eatingCakeUpdate,
    .draw = eatingCakeDraw,
    .handleInput = eatingCakeHandleInput,
    .reset = eatingCakeResetGame,
    .requestingQuit = NULL,
};