#include "../game_levels.h"
#include "../../../include/texture_loader.h"
#include "../../../include/sound_system.h"
#include "../../../include/text_renderer.h"
#include "../../common.h"
#include "level_common.h"
#include <stdlib.h>
#include <math.h>

#define ROTATION_SPEED 0.02f
#define ROTATION_TOLERANCE 0.2f
#define PIZZA_SCALE 0.8f
#define PI 3.14159265358979323846f

typedef struct PizzaSlicingData {
    bool initialized;
    bool gameOver;
    bool success;

    float offsetX;
    float offsetY;

    float currentRotation;  // Current pizza rotation in degrees
    float targetRotation;   // Target rotation for successful cut
    bool hasCut;           // Whether pizza has been cut
    bool touchActive;      // Whether touch is currently being held
    float pizzaCenterX;   // X coordinate of pizza center
    float pizzaCenterY;   // Y coordinate of pizza center
    float lastTouchAngle; // Last calculated touch angle
    float lastTouchReleaseTime;  // Time when last touch was released
} PizzaSlicingData;

static void pizzaSlicingReset(PizzaSlicingData* levelData) {
    if (levelData == NULL) return;

    levelData->gameOver = false;
    levelData->success = false;
    levelData->currentRotation = frand() * M_PI;
    levelData->targetRotation = 0.0f;
    levelData->hasCut = false;
    levelData->touchActive = false;
    levelData->pizzaCenterX = SCREEN_WIDTH_BOTTOM / 2;
    levelData->pizzaCenterY = SCREEN_HEIGHT_BOTTOM - 10 - (128 * PIZZA_SCALE);
    levelData->lastTouchAngle = 0.0f;
    levelData->lastTouchReleaseTime = 0.0f;
}

static void pizzaSlicingInit(GameSceneData* data) {
    PizzaSlicingData* levelData = malloc(sizeof(PizzaSlicingData));
    if (levelData == NULL) {
        panicEverything("Failed to allocate memory for PizzaSlicingData");
        return;
    }

    pizzaSlicingReset(levelData);
    data->currentLevelData = levelData;
    data->gameLeftTime = data->gameSessionTime;
    
    playWavFromRomfs("romfs:/sounds/bgm_microgame2.wav");
}

static void pizzaSlicingUpdate(GameSceneData* data, float deltaTime) {
    PizzaSlicingData* levelData = (PizzaSlicingData*)data->currentLevelData;
    if (levelData == NULL) return;

    UPDATE_BACKGROUND_SCROLL(levelData, 64.0f, SCROLL_SPEED);
}

static void pizzaSlicingDraw(GameSceneData* data, const GraphicsContext* context) {
    PizzaSlicingData* levelData = (PizzaSlicingData*)data->currentLevelData;
    if (levelData == NULL) return;

    // Draw top screen (empty or UI elements)
    C2D_SceneBegin(context->top);
    C2D_TargetClear(context->top, C2D_Color32(0, 0, 0, 255));

    displayTiledImage("romfs:/textures/bg_5_0.t3x", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, levelData->offsetX, levelData->offsetY);

    float rotationDiff = fmodf(levelData->currentRotation + M_PI, M_PI) - levelData->targetRotation;
    if (rotationDiff > M_PI / 2) rotationDiff = M_PI - rotationDiff;

#ifdef DEBUG
    char rotationText[64];
    snprintf(rotationText, sizeof(rotationText), "RotationDiff: %.2f, Threshold: %.2f", rotationDiff, ROTATION_TOLERANCE);
    drawText(10.0f, 10.0f, 0.5f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255), rotationText);
#endif

    // Draw bottom screen (pizza game)
    C2D_SceneBegin(context->bottom);
    C2D_TargetClear(context->bottom, C2D_Color32(0, 0, 0, 255));

    displayTiledImage("romfs:/textures/bg_5_0.t3x", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, levelData->offsetX, levelData->offsetY);

    // Draw pizza with current rotation
    float y = (SCREEN_HEIGHT_BOTTOM - 10) - (256 * PIZZA_SCALE);
    if (levelData->hasCut) {
        displayImageWithScalingAndRotation("romfs:/textures/spr_m1_4_pizza_1.t3x", 
            SCREEN_WIDTH_BOTTOM / 2 - (128 * PIZZA_SCALE), y, NULL,
            PIZZA_SCALE, PIZZA_SCALE, levelData->currentRotation);
    } else {
        displayImageWithScalingAndRotation("romfs:/textures/spr_m1_4_pizza_0.t3x", 
            SCREEN_WIDTH_BOTTOM / 2 - (128 * PIZZA_SCALE), y, NULL,
            PIZZA_SCALE, PIZZA_SCALE, levelData->currentRotation);
    }

    // Draw cutter at the center top
    displayImage("romfs:/textures/spr_m1_4_cutter_0.t3x", 
        SCREEN_WIDTH_BOTTOM / 2, SCREEN_HEIGHT_BOTTOM / 4 - 64);

    if (levelData->hasCut) {
        C2D_DrawLine(SCREEN_WIDTH_BOTTOM / 2, SCREEN_HEIGHT_BOTTOM / 2 - (128 * PIZZA_SCALE) + 24, C2D_Color32(0, 0, 0, 255),
            SCREEN_WIDTH_BOTTOM / 2, SCREEN_HEIGHT_BOTTOM / 2 + (128 * PIZZA_SCALE), C2D_Color32(0, 0, 0, 255), 1.0f, 0.5f);

        if (data->lastGameState == GAME_SUCCESS) {
            float scale = 0.8f;
            displayImageWithScaling("romfs:/textures/spr_m1_4_clear_0.t3x", SCREEN_WIDTH_BOTTOM / 2 - (256 * scale), SCREEN_HEIGHT_BOTTOM / 2 - (128 * scale), NULL, scale, scale);
        }
    }
}

static float calculateTouchAngle(float touchX, float touchY, float centerX, float centerY) {
    float dx = touchX - centerX;
    float dy = touchY - centerY;
    return atan2f(dy, dx);
}

static void performCut(PizzaSlicingData* levelData, GameSceneData* data) {
    levelData->hasCut = true;
    
    float rotationDiff = fmodf(levelData->currentRotation + M_PI, M_PI) - levelData->targetRotation;
    if (rotationDiff > M_PI / 2) rotationDiff = M_PI - rotationDiff;

    if (rotationDiff <= ROTATION_TOLERANCE) {
        levelData->gameOver = true;
        levelData->success = true;
        data->lastGameState = GAME_SUCCESS;
        playWavLayered("romfs:/sounds/se_seikai.wav");
    } else {
        levelData->gameOver = true;
        levelData->success = false;
        data->lastGameState = GAME_FAILURE;
        playWavLayered("romfs:/sounds/se_huseikai.wav");
    }
}

static void pizzaSlicingHandleInput(GameSceneData* data, const InputState* input) {
    PizzaSlicingData* levelData = (PizzaSlicingData*)data->currentLevelData;
    if (levelData == NULL) return;

    if (!levelData->gameOver) {
        // Handle keyboard controls
        if (input->kHeld & KEY_DLEFT || input->kHeld & KEY_LEFT) {
            levelData->currentRotation -= ROTATION_SPEED;
        }
        if (input->kHeld & KEY_DRIGHT || input->kHeld & KEY_RIGHT) {
            levelData->currentRotation += ROTATION_SPEED;
        }
        if (input->kDown & KEY_A && !levelData->hasCut) {
            performCut(levelData, data);
        }

        // Get current time in seconds
        float currentTime = osGetTime() / 1000.0f;

        // Handle touch input
        if (input->touch.px > 0 || input->touch.py > 0) {  // Touch is active
            if (!levelData->touchActive) {
                // New touch - check for double tap
                if (!levelData->hasCut &&
                    (currentTime - levelData->lastTouchReleaseTime) <= 0.5f) {
                    // Double tap within 0.5 seconds - perform cut
                    performCut(levelData, data);
                } else {
                    // Single tap - start rotation
                    levelData->touchActive = true;
                    levelData->lastTouchAngle = calculateTouchAngle(
                        input->touch.px,
                        input->touch.py,
                        levelData->pizzaCenterX,
                        levelData->pizzaCenterY
                    );
                }
            } else {
                // Continuing touch - calculate rotation
                float currentAngle = calculateTouchAngle(
                    input->touch.px,
                    input->touch.py,
                    levelData->pizzaCenterX,
                    levelData->pizzaCenterY
                );
                
                float angleDiff = currentAngle - levelData->lastTouchAngle;
                // Handle angle wrapping
                if (angleDiff > M_PI) angleDiff -= 2 * M_PI;
                if (angleDiff < -M_PI) angleDiff += 2 * M_PI;
                
                levelData->currentRotation += angleDiff;
                levelData->lastTouchAngle = currentAngle;
            }
        } else if (levelData->touchActive) {  // Touch was released
            levelData->touchActive = false;
            levelData->lastTouchReleaseTime = currentTime;
        }

        // Normalize rotation to 0-2*pi
        while (levelData->currentRotation < 0) levelData->currentRotation += (2 * M_PI);
        while (levelData->currentRotation >= (2 * M_PI)) levelData->currentRotation -= (2 * M_PI);
    }
}

static void pizzaSlicingResetGame(GameSceneData* data) {
    PizzaSlicingData* levelData = (PizzaSlicingData*)data->currentLevelData;
    if (levelData) {
        pizzaSlicingReset(levelData);
    }
}

const GameLevel PizzaSlicingGame = {
    .init = pizzaSlicingInit,
    .update = pizzaSlicingUpdate,
    .draw = pizzaSlicingDraw,
    .handleInput = pizzaSlicingHandleInput,
    .reset = pizzaSlicingResetGame,
    .requestingQuit = NULL,
};