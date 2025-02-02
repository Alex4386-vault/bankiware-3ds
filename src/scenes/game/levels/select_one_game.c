#include "../game_levels.h"
#include "../../../include/texture_loader.h"
#include "../../../include/sound_system.h"
#include "../../../include/text_renderer.h"
#include "../../common.h"
#include "level_common.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct SelectOneGameData {
    bool initialized;
    bool gameOver;
    bool success;
    bool inputReceived;
    
    int correctDirection;    // 0: left, 1: up, 2: right, 3: down
    int selectedDirection;   // -1: none, 0: left, 1: up, 2: right, 3: down
    
    float centerX;          // Center position X for bottom screen
    float centerY;          // Center position Y for bottom screen
    float banki_slide;      // For sliding animation of banki
} SelectOneGameData;

static void selectOneGameReset(SelectOneGameData* levelData) {
    if (levelData == NULL) return;

    levelData->gameOver = false;
    levelData->success = false;
    levelData->inputReceived = false;
    levelData->selectedDirection = -1;
    levelData->banki_slide = SCREEN_WIDTH_BOTTOM;
    
    // Randomly select the correct direction
    levelData->correctDirection = rand() % 4;
    
    // Center coordinates for bottom screen
    levelData->centerX = (SCREEN_WIDTH_BOTTOM - 64) / 2;  // Assuming sprite size is 64x64
    levelData->centerY = (SCREEN_HEIGHT_BOTTOM - 64) / 2;
}

static void selectOneGameInit(GameSceneData* data) {
    SelectOneGameData* levelData = malloc(sizeof(SelectOneGameData));
    if (levelData == NULL) {
        panicEverything("Failed to allocate memory for SelectOneGameData");
        return;
    }

    selectOneGameReset(levelData);
    data->currentLevelData = levelData;
    
    data->gameLeftTime = data->gameSessionTime;
    playWavFromRomfs("romfs:/sounds/bgm_microgame2.wav");
}

static void selectOneGameUpdate(GameSceneData* data, float deltaTime) {
    SelectOneGameData* levelData = (SelectOneGameData*)data->currentLevelData;
    if (levelData == NULL) return;

    // Slide animation for banki with ease-out
    if (levelData->success && levelData->banki_slide > levelData->centerX) {
        float distance = levelData->banki_slide - levelData->centerX;
        float speed = distance * 1.5f; // Speed proportional to distance
        levelData->banki_slide -= speed * deltaTime;
        
        // Prevent overshooting
        if (levelData->banki_slide < levelData->centerX) {
            levelData->banki_slide = levelData->centerX;
        }
    }
    
    // Check win/lose condition for timeout
    if (data->gameLeftTime <= 0 && !levelData->gameOver) {
        levelData->gameOver = true;
        levelData->success = false;
        data->lastGameState = GAME_FAILURE;
        playWavLayered("romfs:/sounds/se_huseikai.wav");
    }
}

static void drawDirectionalOption(const char* texture, float x, float y) {
    // Assume 64x64 sprite size, offset by half to center
    displayImage(texture, x, y);
}

static char* selectOneGameGetTextureForSelection(GameSceneData *data, int thisSelection) {
    SelectOneGameData* levelData = (SelectOneGameData*)data->currentLevelData;
    if (levelData == NULL) return NULL;
    
    if (levelData->correctDirection == thisSelection) {
        if (levelData->selectedDirection == thisSelection) {
            return "romfs:/textures/spr_m1_9_ekisya_1.t3x";
        }

        return "romfs:/textures/spr_m1_9_ekisya_0.t3x";
    } else {
        return "romfs:/textures/spr_m1_9_iwa_0.t3x";
    }
}

static void selectOneGameDraw(GameSceneData* data, const GraphicsContext* context) {
    SelectOneGameData* levelData = (SelectOneGameData*)data->currentLevelData;
    if (levelData == NULL) return;
    
    // Draw top screen - white background
    C2D_SceneBegin(context->top);
    C2D_TargetClear(context->top, C2D_Color32(255, 255, 255, 255));

    // Draw bottom screen
    C2D_SceneBegin(context->bottom);
    C2D_TargetClear(context->bottom, C2D_Color32(255, 255, 255, 255));
    
    float centerX = levelData->centerX;
    float centerY = levelData->centerY;
    // Draw center d-pad (centered)
    displayImage("romfs:/textures/spr_m1_9_allow_0.t3x", centerX, centerY);

    
    // Draw directional options
    // Left
    drawDirectionalOption(
        selectOneGameGetTextureForSelection(data, 0),
        centerX - 96, centerY);
    
    // Up
    drawDirectionalOption(
        selectOneGameGetTextureForSelection(data, 1),
        centerX, centerY - 96);
    
    // Right
    drawDirectionalOption(
        selectOneGameGetTextureForSelection(data, 2),
        centerX + 96, centerY);
    
    // Down
    drawDirectionalOption(
        selectOneGameGetTextureForSelection(data, 3),
        centerX, centerY + 96);
        
    // Draw selected option if any
    if (levelData->selectedDirection >= 0) {
        // If correct direction selected, show ekisya_1 at that position (centered)
        float x = centerX;
        float y = centerY;
        float diff = 96;
        switch (levelData->selectedDirection) {
            case 0: x -= diff; break;  // Left
            case 1: y -= diff; break;  // Up
            case 2: x += diff; break;  // Right
            case 3: y += diff; break;  // Down
        }

        // Draw oonusa at center (centered)
        displayImage("romfs:/textures/spr_m1_9_oonusa_0.t3x", x, y);
    }
    // Draw success banki if applicable (centered)
    if (levelData->success) {
        displayImage("romfs:/textures/spr_m1_9_banki_0.t3x", levelData->banki_slide - 256, 0);
    }
}

static void selectOneGameHandleInput(GameSceneData* data, const InputState* input) {
    SelectOneGameData* levelData = (SelectOneGameData*)data->currentLevelData;
    if (levelData == NULL || levelData->gameOver || levelData->inputReceived) return;
    
    int direction = -1;
    
    // Check for d-pad input
    if (input->kDown & KEY_DLEFT) direction = 0;
    else if (input->kDown & KEY_DUP) direction = 1;
    else if (input->kDown & KEY_DRIGHT) direction = 2;
    else if (input->kDown & KEY_DDOWN) direction = 3;
    
    // Check for circle pad input
    circlePosition pos;
    hidCircleRead(&pos);
    if (direction == -1) {  // Only check circle pad if d-pad wasn't used
        if (pos.dx < -32) direction = 0;        // Left
        else if (pos.dy > 32) direction = 1;    // Up
        else if (pos.dx > 32) direction = 2;    // Right
        else if (pos.dy < -32) direction = 3;   // Down
    }
    
    // Check for touch input
    if (input->kDown & KEY_TOUCH) {
        float touchX = input->touch.px - levelData->centerX;
        float touchY = input->touch.py - levelData->centerY;
        
        // Convert touch position to direction
        if (fabsf(touchX) > fabsf(touchY)) {
            if (touchX < -32) direction = 0;      // Left
            else if (touchX > 32) direction = 2;   // Right
        } else {
            if (touchY < -32) direction = 1;      // Up
            else if (touchY > 32) direction = 3;   // Down
        }
    }
    
    if (direction >= 0) {
        levelData->selectedDirection = direction;
        levelData->inputReceived = true;
        playWavLayered("romfs:/sounds/se_pa3.wav");
        
        // Check if correct direction was selected
        if (direction == levelData->correctDirection) {
            levelData->success = true;
            data->lastGameState = GAME_SUCCESS;
            playWavLayered("romfs:/sounds/se_seikai.wav");
        } else {
            levelData->success = false;
            data->lastGameState = GAME_FAILURE;
            playWavLayered("romfs:/sounds/se_huseikai.wav");
        }
        
        levelData->gameOver = true;
    }
}

static void selectOneGameResetGame(GameSceneData* data) {
    SelectOneGameData* levelData = (SelectOneGameData*)data->currentLevelData;
    if (levelData) {
        selectOneGameReset(levelData);
        levelData->initialized = false;
    }
}

const GameLevel SelectOneGame = {
    .init = selectOneGameInit,
    .update = selectOneGameUpdate,
    .draw = selectOneGameDraw,
    .handleInput = selectOneGameHandleInput,
    .reset = selectOneGameResetGame,
    .requestingQuit = NULL,
};