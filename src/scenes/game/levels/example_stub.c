#include "../game_levels.h"
#include "../../../include/texture_loader.h"
#include "../../../include/sound_system.h"
#include "../../../include/text_renderer.h"
#include "../../common.h"
#include "level_common.h"
#include <stdlib.h>

typedef struct ExampleStubData {
    bool initialized;
    bool gameOver;
    bool success;

    float offsetX;
    float offsetY;
} ExampleStubData;

static void exampleStubReset(ExampleStubData* levelData) {
    if (levelData == NULL) return;

    levelData->gameOver = false;
    levelData->success = false;
    levelData->offsetX = 0.0f;
    levelData->offsetY = 0.0f;
}

static void exampleStubInit(GameSceneData* data) {
    ExampleStubData* levelData = malloc(sizeof(ExampleStubData));
    if (levelData == NULL) {
        panicEverything("Failed to allocate memory for ExampleStubData");
        return;
    }

    exampleStubReset(levelData);
    data->currentLevelData = levelData;
    
    data->gameLeftTime = data->gameSessionTime;
    playWavFromRomfs("romfs:/sounds/bgm_microgame1.wav");
}

static void exampleStubUpdate(GameSceneData* data, float deltaTime) {
    ExampleStubData* levelData = (ExampleStubData*)data->currentLevelData;
    if (levelData == NULL) return;

    UPDATE_BACKGROUND_SCROLL(levelData, 64.0f, SCROLL_SPEED);
}

static void exampleStubDraw(GameSceneData* data, const GraphicsContext* context) {
    ExampleStubData* levelData = (ExampleStubData*)data->currentLevelData;
    
    // Draw top screen
    C2D_SceneBegin(context->top);
    C2D_TargetClear(context->top, C2D_Color32(0, 0, 0, 255));

    // draw a background
    levelCommonDraw(data, SCREEN_WIDTH, SCREEN_HEIGHT, levelData->offsetX, levelData->offsetY);
    
    // Draw bottom screen
    C2D_SceneBegin(context->bottom);
    C2D_TargetClear(context->bottom, C2D_Color32(0, 0, 0, 255));

    // draw a background for bottom
    levelCommonDraw(data, SCREEN_WIDTH_BOTTOM, SCREEN_HEIGHT_BOTTOM, levelData->offsetX, levelData->offsetY);
}

static void exampleStubHandleInput(GameSceneData* data, const InputState* input) {
    ExampleStubData* levelData = (ExampleStubData*)data->currentLevelData;
    if (levelData == NULL) return;
    
    if (!levelData->gameOver) {
        if (input->kDown & (KEY_A)) {
            levelData->gameOver = true;
            levelData->success = true;
            data->lastGameState = GAME_SUCCESS;

            playWavLayered("romfs:/sounds/se_seikai.wav");
        } else if (input->kDown & KEY_B) {
            levelData->gameOver = true;
            levelData->success = false;
            data->lastGameState = GAME_FAILURE;

            playWavLayered("romfs:/sounds/se_huseikai.wav");
        }
    }
}

static void exampleStubResetGame(GameSceneData* data) {
    ExampleStubData* levelData = (ExampleStubData*)data->currentLevelData;
    if (levelData) {
        exampleStubReset(levelData);
        levelData->initialized = false;

        levelData->offsetX = 0.0f;
        levelData->offsetY = 0.0f;
    }
}

const GameLevel ExampleStubGame = {
    .init = exampleStubInit,
    .update = exampleStubUpdate,
    .draw = exampleStubDraw,
    .handleInput = exampleStubHandleInput,
    .reset = exampleStubResetGame,
    .requestingQuit = NULL,
};