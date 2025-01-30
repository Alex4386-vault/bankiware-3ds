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
    playWavFromRomfs("romfs:/sounds/bgm_test.wav");
}

static void dialogueSelectUpdate(GameSceneData* data, float deltaTime) {
    DialogueSelectGameData* levelData = (DialogueSelectGameData*)data->currentLevelData;
    if (levelData == NULL) return;

    // Update background scroll
    levelData->offsetX += SCROLL_SPEED;
    levelData->offsetY += SCROLL_SPEED;

    // Wrap offsets to match texture size
    if (levelData->offsetX <= -64.0f) levelData->offsetX = 0.0f;
    if (levelData->offsetY <= -64.0f) levelData->offsetY = 0.0f;
    if (levelData->offsetX >= 64.0f) levelData->offsetX = 0.0f;
    if (levelData->offsetY >= 64.0f) levelData->offsetY = 0.0f;
}

// static char* dialogueSelectGameOption(int option) {
//     switch (option) {
//         case 0:
//             return "Option 1";
//         case 1:
//             return "Option 2";
//         case 2:
//             return "Option 3";
//         case 3:
//             return "Option 4";
//         default:
//             return "Unknown";
//     }
// }

static void dialogueSelectDraw(GameSceneData* data, const GraphicsContext* context) {
    DialogueSelectGameData* levelData = (DialogueSelectGameData*)data->currentLevelData;
    if (levelData == NULL) return;
    
    // Draw top screen
    C2D_SceneBegin(context->top);
    C2D_TargetClear(context->top, C2D_Color32(0, 0, 0, 255));

    // draw a background
    displayTiledImage("romfs:/textures/bg_5_0.t3x", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, levelData->offsetX, levelData->offsetY);

    if (levelData->success) {
        displayImageWithScaling("romfs:/textures/spr_m1_3_banki_1.t3x", SCREEN_WIDTH / 2 - BANKI_OFFSET, SCREEN_HEIGHT / 2 - BANKI_OFFSET, NULL, BANKI_SCALE, BANKI_SCALE);
        displayImage("romfs:/textures/spr_m1_3_maru_0.t3x", SCREEN_WIDTH / 2 - 128, SCREEN_HEIGHT / 2 - 128);
    } else {
        displayImageWithScaling("romfs:/textures/spr_m1_3_banki_0.t3x", SCREEN_WIDTH / 2 - BANKI_OFFSET, SCREEN_HEIGHT / 2 - BANKI_OFFSET, NULL, BANKI_SCALE, BANKI_SCALE);
    }
    
    // Draw bottom screen
    C2D_SceneBegin(context->bottom);
    C2D_TargetClear(context->bottom, C2D_Color32(0, 0, 0, 255));

    // show 4 dialogue options,
    float optionX = (SCREEN_WIDTH_BOTTOM / 2) - 64.0f;
    float optionHeights = (SCREEN_HEIGHT_BOTTOM - 40) / 4;

    float optionY = 20.0f;
    for (int i = 0; i < 4; i++) {
        float availableY = optionHeights - 32.0f;
        optionY += availableY / 2;
        displayImage("romfs:/textures/spr_m1_3_ui_0.t3x", optionX, optionY);
        drawTextWithFlags(optionX + 64, optionY + 8, 0.5f, 1.0f, 1.0f, C2D_Color32(0, 0, 0, 255), C2D_AlignCenter, "bruh");
        optionY += 32.0f;
        optionY += availableY / 2;

        if (levelData->selectedOption == i) {
            // get the hand.
            float targetY = optionY - optionHeights;
            displayImage("romfs:/textures/spr_m1_3_cursor_0.t3x", optionX - 32, targetY);
        }
    }

    // draw a background for bottom
    displayTiledImage("romfs:/textures/bg_5_0.t3x", 0, 0, SCREEN_WIDTH_BOTTOM, SCREEN_HEIGHT_BOTTOM, levelData->offsetX, levelData->offsetY);
}

static void dialogueSelectHandleSelect(GameSceneData* data) {
    DialogueSelectGameData* levelData = (DialogueSelectGameData*)data->currentLevelData;
    if (levelData == NULL) return;

    if (!levelData->success && data->isInGame) {  // Check if game is still running
        levelData->success = true;
        data->lastGameState = GAME_SUCCESS;

        playWavLayered("romfs:/sounds/se_seikai.wav");
    }

}

static void dialogueSelectHandleInput(GameSceneData* data, const InputState* input) {
    DialogueSelectGameData* levelData = (DialogueSelectGameData*)data->currentLevelData;
    if (levelData == NULL) return;

    if (!levelData->success && data->isInGame) {  // Check if game is still running
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