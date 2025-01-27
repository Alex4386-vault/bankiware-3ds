#include "../game_levels.h"
#include "../../../include/texture_loader.h"
#include "../../../include/sound_system.h"
#include "../../../include/text_renderer.h"
#include "../../common.h"
#include <stdlib.h>

typedef struct ExampleStubData {
    bool initialized;
    bool gameOver;
    bool success;
    float offsetX;
    float offsetY;
} ExampleStubData;

static void exampleStubReset(ExampleStubData* levelData) {
    levelData->gameOver = false;
    levelData->success = false;
    levelData->offsetX = 0.0f;
    levelData->offsetY = 0.0f;
}

static void exampleStubInit(GameSceneData* data) {
    ExampleStubData* levelData = malloc(sizeof(ExampleStubData));
    exampleStubReset(levelData);
    data->currentLevelData = levelData;
    
    data->gameSessionTime = 3.0f;
    data->gameLeftTime = data->gameSessionTime;
}

static void exampleStubUpdate(GameSceneData* data, float deltaTime) {
    ExampleStubData* levelData = (ExampleStubData*)data->currentLevelData;
    
    // Update background scroll
    levelData->offsetX += SCROLL_SPEED;
    levelData->offsetY += SCROLL_SPEED;

    // Wrap offsets to match texture size
    if (levelData->offsetX <= -64.0f) levelData->offsetX = 0.0f;
    if (levelData->offsetY <= -64.0f) levelData->offsetY = 0.0f;
    if (levelData->offsetX >= 64.0f) levelData->offsetX = 0.0f;
    if (levelData->offsetY >= 64.0f) levelData->offsetY = 0.0f;
}

static void exampleStubDraw(GameSceneData* data, const GraphicsContext* context) {
    ExampleStubData* levelData = (ExampleStubData*)data->currentLevelData;
    
    // Draw top screen
    C2D_SceneBegin(context->top);
    C2D_TargetClear(context->top, C2D_Color32(0, 0, 0, 255));

    // draw a background
    char targetFile[64];
    int fileId = 2;
    if (data->lastGameState == GAME_UNDEFINED) {
      fileId = 2;
    } else if (data->lastGameState == GAME_SUCCESS) {
      fileId = 3;
    } else if (data->lastGameState == GAME_FAILURE) {
      fileId = 4;
    }
    snprintf(targetFile, sizeof(targetFile), "romfs:/textures/bg_%d_0.t3x", fileId);
    
    displayTiledImage(targetFile, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                     levelData->offsetX, levelData->offsetY);
    
    drawTextWithFlags(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 16, 
                     0.5f, 1.0f, 1.0f, C2D_Color32(0, 0, 0, 255),
                     C2D_AlignCenter, "Stub Level");
    drawTextWithFlags(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 16, 
                     0.5f, 0.5f, 0.5f, C2D_Color32(0, 0, 0, 255),
                     C2D_AlignCenter, "A: Success, B: Failure");
    
    // Draw bottom screen
    C2D_SceneBegin(context->bottom);
    C2D_TargetClear(context->bottom, C2D_Color32(0, 0, 0, 255));

    displayTiledImage(targetFile, 0, 0, SCREEN_WIDTH_BOTTOM, SCREEN_HEIGHT_BOTTOM,
                     levelData->offsetX, levelData->offsetY);
    
}

static void exampleStubHandleInput(GameSceneData* data, const InputState* input) {
    ExampleStubData* levelData = (ExampleStubData*)data->currentLevelData;
    
    if (!levelData->gameOver) {
        if (input->kDown & KEY_A) {
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