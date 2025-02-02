#include "../game_levels.h"
#include "../../../include/texture_loader.h"
#include "../../../include/sound_system.h"
#include "../../../include/text_renderer.h"
#include "../../common.h"
#include "level_common.h"
#include <stdlib.h>
#include <stdio.h>

#define MAX_BANKIS 9
#define BANKI_SIZE 64
#define BANKI_PADDING 10

typedef struct BankiPosition {
    float x;
    float y;
} BankiPosition;

typedef struct CounterGameData {
    bool initialized;
    bool gameOver;
    bool success;

    int totalBankis;         // Random number of bankis to display (1-9)
    int currentInput;        // Player's current input
    BankiPosition bankiPositions[MAX_BANKIS];  // Store positions of each banki
    
    float counterX;          // Counter position X
    float counterY;          // Counter position Y
} CounterGameData;

static void generateRandomBankiPositions(CounterGameData* levelData) {
    int maxWidth = SCREEN_WIDTH - BANKI_SIZE;
    int maxHeight = SCREEN_HEIGHT - BANKI_SIZE;
    
    for (int i = 0; i < levelData->totalBankis; i++) {
        // Keep trying until we find a non-overlapping position
        bool validPosition = false;
        while (!validPosition) {
            float newX = (float)(rand() % maxWidth);
            float newY = (float)(rand() % maxHeight);
            
            // Check if this position overlaps with any existing banki
            validPosition = true;
            for (int j = 0; j < i; j++) {
                float dx = fabsf(newX - levelData->bankiPositions[j].x);
                float dy = fabsf(newY - levelData->bankiPositions[j].y);
                if (dx < (BANKI_SIZE + BANKI_PADDING) && dy < (BANKI_SIZE + BANKI_PADDING)) {
                    validPosition = false;
                    break;
                }
            }
            
            if (validPosition) {
                levelData->bankiPositions[i].x = newX;
                levelData->bankiPositions[i].y = newY;
            }
        }
    }
}

static void counterGameReset(CounterGameData* levelData) {
    if (levelData == NULL) return;

    levelData->gameOver = false;
    levelData->success = false;
    levelData->currentInput = 0;
    levelData->totalBankis = (rand() % 9) + 1;  // Random number between 1-9
    
    // Center the counter
    levelData->counterX = (SCREEN_WIDTH_BOTTOM - BANKI_SIZE) / 2;
    levelData->counterY = (SCREEN_HEIGHT_BOTTOM - BANKI_SIZE) / 2;
    
    // Generate random positions for bankis
    generateRandomBankiPositions(levelData);
}

static void counterGameInit(GameSceneData* data) {
    CounterGameData* levelData = malloc(sizeof(CounterGameData));
    if (levelData == NULL) {
        panicEverything("Failed to allocate memory for CounterGameData");
        return;
    }

    counterGameReset(levelData);
    data->currentLevelData = levelData;
    
    data->gameLeftTime = data->gameSessionTime;
    playWavFromRomfs("romfs:/sounds/bgm_microgame1.wav");
}

static void counterGameUpdate(GameSceneData* data, float deltaTime) {
    CounterGameData* levelData = (CounterGameData*)data->currentLevelData;
    if (levelData == NULL) return;
    
    // Check win/lose condition
    if (data->gameLeftTime <= 0 && !levelData->gameOver) {
        levelData->gameOver = true;
        levelData->success = false;
        data->lastGameState = GAME_FAILURE;
        playWavLayered("romfs:/sounds/se_huseikai.wav");
    }
}

static void counterGameDraw(GameSceneData* data, const GraphicsContext* context) {
    CounterGameData* levelData = (CounterGameData*)data->currentLevelData;
    if (levelData == NULL) return;
    
    // Draw top screen - white background with bankis
    C2D_SceneBegin(context->top);
    C2D_TargetClear(context->top, C2D_Color32(255, 255, 255, 255));

    // Draw all bankis at their positions
    for (int i = 0; i < levelData->totalBankis; i++) {
        char bankiPath[64];
        snprintf(bankiPath, sizeof(bankiPath), "romfs:/textures/spr_m1_6_banki_0.t3x");
        displayImage(bankiPath, levelData->bankiPositions[i].x, levelData->bankiPositions[i].y);
    }
    
    // Draw bottom screen - counter
    C2D_SceneBegin(context->bottom);
    C2D_TargetClear(context->bottom, C2D_Color32(255, 255, 255, 255));
    
    // Draw counter
    displayImage("romfs:/textures/spr_m1_7_counter_0.t3x", levelData->counterX, levelData->counterY);
    
    // Draw current input
    char inputText[8];
    snprintf(inputText, sizeof(inputText), "%d", levelData->currentInput);
    float textX = levelData->counterX + 20;
    float textY = levelData->counterY + 20;
    drawText(textX, textY, 0.0f, 1.0f, 1.0f, C2D_Color32(0, 0, 0, 255), inputText);
}

static void counterGameHandleInput(GameSceneData* data, const InputState* input) {
    CounterGameData* levelData = (CounterGameData*)data->currentLevelData;
    if (levelData == NULL || levelData->gameOver) return;
    
    // Handle A button or touch screen input
    bool inputActivated = (input->kDown & KEY_A) || 
                         (input->touch.px >= levelData->counterX && 
                          input->touch.px <= levelData->counterX + BANKI_SIZE &&
                          input->touch.py >= levelData->counterY &&
                          input->touch.py <= levelData->counterY + BANKI_SIZE);
    
    if (inputActivated) {
        levelData->currentInput++;
        
        // Check if player counted correctly
        if (levelData->currentInput == levelData->totalBankis) {
            levelData->gameOver = true;
            levelData->success = true;
            data->lastGameState = GAME_SUCCESS;
            playWavLayered("romfs:/sounds/se_seikai.wav");
        } else if (levelData->currentInput > levelData->totalBankis) {
            levelData->gameOver = true;
            levelData->success = false;
            data->lastGameState = GAME_FAILURE;
            playWavLayered("romfs:/sounds/se_huseikai.wav");
        }
    }
}

static void counterGameResetGame(GameSceneData* data) {
    CounterGameData* levelData = (CounterGameData*)data->currentLevelData;
    if (levelData) {
        counterGameReset(levelData);
        levelData->initialized = false;
    }
}

const GameLevel CounterGame = {
    .init = counterGameInit,
    .update = counterGameUpdate,
    .draw = counterGameDraw,
    .handleInput = counterGameHandleInput,
    .reset = counterGameResetGame,
    .requestingQuit = NULL,
};