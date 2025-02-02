#include "../game_levels.h"
#include "../../../include/texture_loader.h"
#include "../../../include/sound_system.h"
#include "../../../include/text_renderer.h"
#include "../../common.h"
#include "level_common.h"
#include <stdlib.h>

#define BANKI_SCALE 0.7f
#define BANKI_OFFSET (256 * BANKI_SCALE)

typedef struct DialogueSelectGameData {
    bool initialized;
    bool success;

    int selectedOption;

    float offsetX;
    float offsetY;
} DialogueSelectGameData;

static void dialogueSelectReset(DialogueSelectGameData* levelData) {
    if (levelData == NULL) return;

    levelData->success = false;
    levelData->offsetX = 0.0f;
    levelData->offsetY = 0.0f;
    levelData->selectedOption = 0;
}

static void dialogueSelectInit(GameSceneData* data) {
    DialogueSelectGameData* levelData = malloc(sizeof(DialogueSelectGameData));
    if (levelData == NULL) {
        panicEverything("Failed to allocate memory for DialogueSelectGameData");
        return;
    }

    dialogueSelectReset(levelData);
    data->currentLevelData = levelData;
    
    data->gameLeftTime = data->gameSessionTime;
    playWavFromRomfs("romfs:/sounds/bgm_microgame2.wav");
}

static void dialogueSelectUpdate(GameSceneData* data, float deltaTime) {
    DialogueSelectGameData* levelData = (DialogueSelectGameData*)data->currentLevelData;
    if (levelData == NULL) return;

    UPDATE_BACKGROUND_SCROLL(levelData, 64.0f, SCROLL_SPEED);
}

static char* dialogueSelectGameOption(int option) {
    switch (option) {
        case 0:
            return "かわいい";
        case 1:
            return "kawaii";
        case 2:
            return "KAWAII";
        case 3:
            return "カワイイ";
        default:
            return "Unknown";
    }
}

static void dialogueSelectDraw(GameSceneData* data, const GraphicsContext* context) {
    DialogueSelectGameData* levelData = (DialogueSelectGameData*)data->currentLevelData;
    if (levelData == NULL) return;
    
    // Draw top screen
    C2D_SceneBegin(context->top);
    C2D_TargetClear(context->top, C2D_Color32(0, 0, 0, 255));

    char *texturePath;
    if (data->lastGameState != GAME_SUCCESS) {
        texturePath = "romfs:/textures/bg_5_0.t3x";
    } else {
        texturePath = "romfs:/textures/bg_6_0.t3x";
    }

    // draw a background
    displayTiledImage(texturePath, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, levelData->offsetX, levelData->offsetY);

    if (levelData->success) {
        displayImageWithScaling("romfs:/textures/spr_m1_3_banki_1.t3x", SCREEN_WIDTH / 2 - BANKI_OFFSET, SCREEN_HEIGHT / 2 - BANKI_OFFSET, NULL, BANKI_SCALE, BANKI_SCALE);
        displayImage("romfs:/textures/spr_m1_3_maru_0.t3x", SCREEN_WIDTH / 2 - 128, SCREEN_HEIGHT / 2 - 128);
    } else {
        displayImageWithScaling("romfs:/textures/spr_m1_3_banki_0.t3x", SCREEN_WIDTH / 2 - BANKI_OFFSET, SCREEN_HEIGHT / 2 - BANKI_OFFSET, NULL, BANKI_SCALE, BANKI_SCALE);
    }
    
    // Draw bottom screen
    C2D_SceneBegin(context->bottom);
    C2D_TargetClear(context->bottom, C2D_Color32(0, 0, 0, 255));

    // draw a background for bottom
    displayTiledImage(texturePath, 0, 0, SCREEN_WIDTH_BOTTOM, SCREEN_HEIGHT_BOTTOM, levelData->offsetX, levelData->offsetY);

    // show 4 dialogue options,
    float optionX = (SCREEN_WIDTH_BOTTOM / 2) - 64.0f;
    float optionHeights = (SCREEN_HEIGHT_BOTTOM - 40) / 4;

    float optionY = 20.0f;
    for (int i = 0; i < 4; i++) {
        float availableY = optionHeights - 32.0f;
        optionY += availableY / 2;
        displayImage("romfs:/textures/spr_m1_3_ui_0.t3x", optionX, optionY);
        drawTextWithFlags(optionX + 64, optionY + 8, 0.5f, 0.5f, 0.5f, C2D_Color32(0, 0, 0, 255), C2D_AlignCenter, dialogueSelectGameOption(i));
        optionY += 32.0f;
        optionY += availableY / 2;

        if (levelData->selectedOption == i) {
            // get the hand.
            float targetY = optionY - optionHeights;
            displayImage("romfs:/textures/spr_m1_3_cursor_0.t3x", optionX - 20, targetY + 8);
        }
    }
}

static void dialogueSelectHandleSelect(GameSceneData* data) {
    DialogueSelectGameData* levelData = (DialogueSelectGameData*)data->currentLevelData;
    if (levelData == NULL) return;

    if (!levelData->success && data->isInGame) {  // Check if game is still running
        levelData->success = true;
        data->lastGameState = GAME_SUCCESS;

        playWavLayered("romfs:/sounds/se_rappa.wav");
    }

}

static void dialogueSelectHandleInput(GameSceneData* data, const InputState* input) {
    DialogueSelectGameData* levelData = (DialogueSelectGameData*)data->currentLevelData;
    if (levelData == NULL) return;

    if (!levelData->success && data->isInGame) {  // Check if game is still running
        // Handle keyboard/button input
        if (input->kDown & (KEY_A)) {
            dialogueSelectHandleSelect(data);
        }

        if (input->kDown & (KEY_DDOWN | KEY_DOWN)) {
            levelData->selectedOption = (levelData->selectedOption + 1) % 4;
            playWavLayered("romfs:/sounds/se_cursor.wav");
        } else if (input->kDown & (KEY_DUP | KEY_UP)) {
            levelData->selectedOption = (levelData->selectedOption + 4 - 1) % 4;
            playWavLayered("romfs:/sounds/se_cursor.wav");
        }

        // Handle touch input
        if (input->kDown & KEY_TOUCH) {
            float optionX = (SCREEN_WIDTH_BOTTOM / 2) - 64.0f;
            float optionHeights = (SCREEN_HEIGHT_BOTTOM - 40) / 4;
            float optionY = 20.0f;

            // Check each option's touch area
            for (int i = 0; i < 4; i++) {
                float availableY = optionHeights - 32.0f;
                optionY += availableY / 2;

                // Check if touch is within option bounds
                if (input->touch.px >= optionX && input->touch.px <= optionX + 128.0f &&
                    input->touch.py >= optionY && input->touch.py <= optionY + 32.0f) {
                    if (levelData->selectedOption != i) {
                        levelData->selectedOption = i;
                        playWavLayered("romfs:/sounds/se_cursor.wav");
                    }
                    dialogueSelectHandleSelect(data);
                    break;
                }

                optionY += 32.0f;
                optionY += availableY / 2;
            }
        }
    }
}

static void dialogueSelectResetGame(GameSceneData* data) {
    DialogueSelectGameData* levelData = (DialogueSelectGameData*)data->currentLevelData;
    if (levelData) {
        dialogueSelectReset(levelData);
        levelData->initialized = false;

        levelData->offsetX = 0.0f;
        levelData->offsetY = 0.0f;
    }

    stopAudio();
}

const GameLevel DialogueSelectGame = {
    .init = dialogueSelectInit,
    .update = dialogueSelectUpdate,
    .draw = dialogueSelectDraw,
    .handleInput = dialogueSelectHandleInput,
    .reset = dialogueSelectResetGame,
    .requestingQuit = NULL,
};