#include "../game_levels.h"
#include "../../../include/texture_loader.h"
#include "../../../include/sound_system.h"
#include "../../../include/text_renderer.h"
#include "../../common.h"
#include <stdlib.h>
#include <math.h>

#define APPLE_MOVE_SPEED 100.0f
#define APPLE_MIN_Y 25.0f
#define APPLE_MAX_Y (SCREEN_HEIGHT - 25.0f - 64.0f)  // 64 is the height of the apple sprite
#define CHARACTER_X 240.0f  // Character on right side
#define CHARACTER_Y 0.0f
#define APPLE_X 75.0f      // Apple on left side
#define BUTTON_X (SCREEN_WIDTH_BOTTOM / 2)
#define BUTTON_Y (SCREEN_HEIGHT_BOTTOM / 2)
#define BUTTON_WIDTH 200.0f
#define BUTTON_HEIGHT 140.0f
#define LASER_Y (CHARACTER_Y + 82.0f)

#define TEXTURE_PATH_BANKI "romfs:/textures/spr_m1_1_banki_0.t3x"
#define TEXTURE_PATH_APPLE "romfs:/textures/spr_m1_1_enemy1_0.t3x"
#define TEXTURE_PATH_BEAM1 "romfs:/textures/spr_m1_1_beam1_0.t3x"
#define TEXTURE_PATH_BEAM2 "romfs:/textures/spr_m1_1_beam1_1.t3x"

typedef struct LaserBeamGameData {
    float appleY;
    float appleDirection;  // 1 for down, -1 for up
    
    bool laserFired;
    float laserAnimTimer;
    int laserFrame;
    
    bool gameOver;
    bool success;
    bool initialized;
} LaserBeamGameData;

static void resetLaserBeamGame(LaserBeamGameData* levelData) {
    levelData->appleY = APPLE_MIN_Y;
    levelData->appleDirection = 1.0f;
    levelData->laserFired = false;
    levelData->gameOver = false;
    levelData->success = false;
    levelData->laserAnimTimer = 0.0f;
    levelData->laserFrame = 0;
}

static void laserBeamGameInit(GameSceneData* data) {
    LaserBeamGameData* levelData = malloc(sizeof(LaserBeamGameData));
    resetLaserBeamGame(levelData);
    data->currentLevelData = levelData;
    
    // Set game session time
    data->gameSessionTime = 4.0f;  // 4 seconds to complete the level
    data->gameLeftTime = data->gameSessionTime;
    
    playWavFromRomfs("romfs:/sounds/bgm_test.wav");
}

static void laserBeamGameUpdate(GameSceneData* data, float deltaTime) {
    LaserBeamGameData* levelData = (LaserBeamGameData*)data->currentLevelData;
    
    if (!levelData->gameOver) {
        // Update apple movement
        levelData->appleY += levelData->appleDirection * APPLE_MOVE_SPEED * deltaTime;
        
        // Bounce apple at boundaries
        if (levelData->appleY >= APPLE_MAX_Y) {
            levelData->appleY = APPLE_MAX_Y;
            levelData->appleDirection = -1.0f;
        } else if (levelData->appleY <= APPLE_MIN_Y) {
            levelData->appleY = APPLE_MIN_Y;
            levelData->appleDirection = 1.0f;
        }
        
        // Update laser animation and timing if fired
        if (levelData->laserFired) {
            levelData->laserAnimTimer += deltaTime;
            
            // Update laser animation
            if (levelData->laserAnimTimer >= 0.5f) {
                levelData->laserAnimTimer = 0.0f;
                levelData->laserFrame = !levelData->laserFrame;
            }
        }
    }
}

static void laserBeamGameDraw(GameSceneData* data, const GraphicsContext* context) {
    LaserBeamGameData* levelData = (LaserBeamGameData*)data->currentLevelData;
    
    // Draw on top screen
    C2D_SceneBegin(context->top);

    // Draw background
    C2D_TargetClear(context->top, C2D_Color32(0, 0, 0, 255));
    
    // Draw character (Banki) on right side
    displayImageWithScaling(TEXTURE_PATH_BANKI, CHARACTER_X, CHARACTER_Y, NULL, 1.0f, 1.0f);  // No flip needed
    
    // Draw apple on left side
    if (!levelData->success) {
        displayImage(TEXTURE_PATH_APPLE, APPLE_X, levelData->appleY);
    }
    
    // Draw laser if fired
    if (levelData->laserFired) {
        const char* laserSprite = levelData->laserFrame ? TEXTURE_PATH_BEAM2 : TEXTURE_PATH_BEAM1;
        // Draw laser from character to apple (right to left)
        float laserY = LASER_Y; // Adjust to match character's "mouth"
        float laserX = 0;
        displayImageWithScaling(laserSprite, laserX, laserY, NULL, 1.2f, 1.2f);
    }
    
    // Draw fire button on bottom screen
    C2D_SceneBegin(context->bottom);
    C2D_TargetClear(context->bottom, C2D_Color32(0, 0, 0, 255));

    // Draw a simple rectangle for the button

    C2D_DrawRectSolid(BUTTON_X - BUTTON_WIDTH/2, BUTTON_Y - BUTTON_HEIGHT/2, 
                      0, BUTTON_WIDTH, BUTTON_HEIGHT, levelData->laserFired ? C2D_Color32(50, 25, 0, 255) : C2D_Color32(190, 75, 0, 255));

    // Draw text on button
    drawTextWithFlags(BUTTON_X, BUTTON_Y - 16, 0.5f, 1.0f, 1.0f, C2D_Color32(255, 255, 255, 255), C2D_AlignCenter, "FIRE");
}

static void handleLaserBeamGameHit(GameSceneData* data) {
    LaserBeamGameData* levelData = (LaserBeamGameData*)data->currentLevelData;

    // Check for hit - if laser Y position matches apple Y position within a margin
    float hitMargin = 32.0f;
    float dy = fabsf(LASER_Y - levelData->appleY);
    
    if (dy < hitMargin) {
        levelData->gameOver = true;
        levelData->success = true;
        data->lastGameState = GAME_SUCCESS;
    } else {
        levelData->gameOver = true;
        levelData->success = false;
        data->lastGameState = GAME_FAILURE;
    }
}

static void laserBeamGameHandleInput(GameSceneData* data, const InputState* input) {
    LaserBeamGameData* levelData = (LaserBeamGameData*)data->currentLevelData;
    
    if (!levelData->laserFired && !levelData->gameOver) {
        // Check for A button or touch in button area
        if (input->kDown & KEY_A || 
            (input->touch.px >= BUTTON_X - BUTTON_WIDTH/2 && 
             input->touch.px <= BUTTON_X + BUTTON_WIDTH/2 &&
             input->touch.py >= BUTTON_Y - BUTTON_HEIGHT/2 &&
             input->touch.py <= BUTTON_Y + BUTTON_HEIGHT/2)) {
            
            levelData->laserFired = true;
            handleLaserBeamGameHit(data);
        }
    }
}

static void laserBeamGameReset(GameSceneData* data) {
    LaserBeamGameData* levelData = (LaserBeamGameData*)data->currentLevelData;
    if (levelData) {
        resetLaserBeamGame(levelData);
        // Don't free here as the game scene handles memory management
        levelData->initialized = false;
    }
}

// Export level functions
const GameLevel LaserBeamGame = {
    .init = laserBeamGameInit,
    .update = laserBeamGameUpdate,
    .draw = laserBeamGameDraw,
    .handleInput = laserBeamGameHandleInput,
    .reset = laserBeamGameReset,
    .requestingQuit = NULL,  // No immediate quit
};