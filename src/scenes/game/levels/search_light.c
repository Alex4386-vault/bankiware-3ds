#include "../game_levels.h"
#include "../../../include/texture_loader.h"
#include "../../../include/sound_system.h"
#include "../../../include/text_renderer.h"
#include "../../common.h"
#include "level_common.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define SPOTLIGHT_RADIUS 64.0f
#define SPOTLIGHT_SPEED 4.0f

#define SPOTLIGHT_FOUND_TIME 0.5f

#define TEXTURE_PATH_BANKI "romfs:/textures/spr_m1_5_banki_0.t3x"
#define TEXTURE_PATH_BANKI_DETECTED "romfs:/textures/spr_m1_5_banki_1.t3x"
#define TEXTURE_PATH_SPOTLIGHT "romfs:/textures/spr_m1_5_black_0.t3x"

typedef struct SearchLightData {
    bool initialized;
    bool gameOver;
    bool success;

    float spotlightX;
    float spotlightY;
    
    float bankiX;
    float bankiY;

    float foundTime;
} SearchLightData;

static void searchLightReset(SearchLightData* levelData) {
    if (levelData == NULL) return;

    levelData->gameOver = false;
    levelData->success = false;
    
    // Random banki position on top screen
    levelData->bankiX = (float)(rand() % ((int)SCREEN_WIDTH_BOTTOM - 64));  // 64 is sprite width
    levelData->bankiY = (float)(rand() % ((int)SCREEN_HEIGHT_BOTTOM - 64)); // 64 is sprite height
    
    // Start spotlight in center
    levelData->spotlightX = SCREEN_WIDTH_BOTTOM / 2;
    levelData->spotlightY = SCREEN_HEIGHT_BOTTOM / 2;
}

static void searchLightInit(GameSceneData* data) {
    srand(time(NULL));
    
    SearchLightData* levelData = malloc(sizeof(SearchLightData));
    if (levelData == NULL) {
        panicEverything("Failed to allocate memory for SearchLightData");
        return;
    }

    searchLightReset(levelData);
    data->currentLevelData = levelData;
    
    data->gameLeftTime = data->gameSessionTime;
    playWavFromRomfs("romfs:/sounds/bgm_microgame2.wav");
}

static void searchLightUpdate(GameSceneData* data, float deltaTime) {
    SearchLightData* levelData = (SearchLightData*)data->currentLevelData;
    if (levelData == NULL) return;
    
    // Check if spotlight is over banki
    float dx = levelData->spotlightX - levelData->bankiX;
    float dy = levelData->spotlightY - levelData->bankiY;
    float distance = sqrtf(dx * dx + dy * dy);
    
    if (!levelData->gameOver) {
        if (distance < SPOTLIGHT_RADIUS) {
            levelData->foundTime += deltaTime;
        } else {
            levelData->foundTime = 0.0f;
        }
    } 

    // Check for win condition
    if (levelData->foundTime >= SPOTLIGHT_FOUND_TIME && !levelData->gameOver) {
        levelData->success = true;
        levelData->gameOver = true;
        data->lastGameState = GAME_SUCCESS;
        playWavLayered("romfs:/sounds/se_seikai.wav");
    }
}

static void searchLightDraw(GameSceneData* data, const GraphicsContext* context) {
    SearchLightData* levelData = (SearchLightData*)data->currentLevelData;
    if (levelData == NULL) return;
    
    // Draw top screen
    C2D_SceneBegin(context->top);
    if (levelData->success)
        C2D_TargetClear(context->top, C2D_Color32(255, 255, 255, 255));
    else
        C2D_TargetClear(context->top, C2D_Color32(0, 0, 0, 255));
    
    // Draw bottom screen
    C2D_SceneBegin(context->bottom);
    if (levelData->success)
        C2D_TargetClear(context->bottom, C2D_Color32(255, 255, 255, 255));
    else
        C2D_TargetClear(context->bottom, C2D_Color32(20, 20, 20, 255));

    // Draw banki at its position
    if (levelData->success) {
        displayImageWithScaling(TEXTURE_PATH_BANKI_DETECTED, levelData->bankiX, levelData->bankiY, NULL, 1.0f, 1.0f);
    } else {
        displayImageWithScaling(TEXTURE_PATH_BANKI, levelData->bankiX, levelData->bankiY, NULL, 1.0f, 1.0f);
    }

    if (!levelData->success) {
        C2D_ImageTint tint;
        for (int i = 0; i < 4; i++) {
            C2D_PlainImageTint(&tint, C2D_Color32(0, 0, 0, 255), 0.75f);
        }

        displayTiledImageWithTint(TEXTURE_PATH_SPOTLIGHT, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, levelData->spotlightX - 256, levelData->spotlightY - 256, &tint);

        // Draw spotlight overlay centered on cursor position
        displayImageWithScaling(TEXTURE_PATH_SPOTLIGHT, 
                    levelData->spotlightX - 256, 
                    levelData->spotlightY - 256,
                    &tint,
                    1.0f, 1.0f);
    }    
}

static void searchLightHandleInput(GameSceneData* data, const InputState* input) {
    SearchLightData* levelData = (SearchLightData*)data->currentLevelData;
    if (levelData == NULL || levelData->gameOver) return;
    
    // D-Pad controls
    if (input->kHeld & KEY_DRIGHT) levelData->spotlightX += SPOTLIGHT_SPEED;
    if (input->kHeld & KEY_DLEFT) levelData->spotlightX -= SPOTLIGHT_SPEED;
    if (input->kHeld & KEY_DUP) levelData->spotlightY -= SPOTLIGHT_SPEED;
    if (input->kHeld & KEY_DDOWN) levelData->spotlightY += SPOTLIGHT_SPEED;
    
    // Circle Pad controls (using directional buttons)
    if (input->kHeld & KEY_RIGHT) levelData->spotlightX += SPOTLIGHT_SPEED;
    if (input->kHeld & KEY_LEFT) levelData->spotlightX -= SPOTLIGHT_SPEED;
    if (input->kHeld & KEY_UP) levelData->spotlightY -= SPOTLIGHT_SPEED;
    if (input->kHeld & KEY_DOWN) levelData->spotlightY += SPOTLIGHT_SPEED;
    
    // Touch controls
    if (input->kHeld & KEY_TOUCH) {
        levelData->spotlightX = input->touch.px;
        levelData->spotlightY = input->touch.py;
    }
    
    // Clamp spotlight position
    if (levelData->spotlightX < 0) levelData->spotlightX = 0;
    if (levelData->spotlightX > SCREEN_WIDTH) levelData->spotlightX = SCREEN_WIDTH;
    if (levelData->spotlightY < 0) levelData->spotlightY = 0;
    if (levelData->spotlightY > SCREEN_HEIGHT) levelData->spotlightY = SCREEN_HEIGHT;
}

static void searchLightResetGame(GameSceneData* data) {
    SearchLightData* levelData = (SearchLightData*)data->currentLevelData;
    if (levelData) {
        searchLightReset(levelData);
        levelData->initialized = false;
    }
}

const GameLevel SearchLightGame = {
    .init = searchLightInit,
    .update = searchLightUpdate,
    .draw = searchLightDraw,
    .handleInput = searchLightHandleInput,
    .reset = searchLightResetGame,
    .requestingQuit = NULL,
};