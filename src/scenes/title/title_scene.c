#include "title_scene.h"
#include "../common.h"
#include "../scene_manager.h"
#include "../../include/texture_loader.h"
#include "../../include/text_renderer.h"
#include "../../include/sound_system.h"
#include <stdlib.h>
#include <stdio.h>

#define UNSELECTED_X 128
#define SELECTED_X 64

typedef enum SelectedAction {
    ACTION_START,
    ACTION_EXIT,

    ACTION_COUNT,
} SelectedAction;

typedef struct {
    float offsetX;
    float offsetY;
    float scrollSpeed;
    bool touchActive;
    touchPosition lastTouch;
    bool isDragging;  // To distinguish between dragging and tapping

    bool showDebug;

    SelectedAction selectedAction;
} TitleSceneData;

static void titleInit(Scene* scene) {
    TitleSceneData* data = (TitleSceneData*)scene->data;
    data->offsetX = 0.0f;
    data->offsetY = 0.0f;
    data->scrollSpeed = SCROLL_SPEED;
    data->touchActive = false;
    data->isDragging = false;
    data->showDebug = false;
}

static void doAction(SelectedAction action) {
    playWavFromRomfs("romfs:/sounds/se_decide.wav");
    switch (action) {
        case ACTION_START: {
            changeScene(SCENE_PREGAME_DIALOGUE); 
            break;
        }
        case ACTION_EXIT: {
            svcSleepThread(500000000); // Sleep for 500ms to let sound play
            requestExit();  // Exit is immediate, no transition needed
            break;
        }
        default:
            break;
    }
}

static void titleHandleInput(Scene* scene, const InputState* input) {
    TitleSceneData* data = (TitleSceneData*)scene->data;

    // handle select key
    if (input->kDown & KEY_SELECT) {
        data->showDebug = !data->showDebug;
    }

    // handle touch input
    if (input->kDown & KEY_TOUCH) {
        data->touchActive = true;
        data->lastTouch = input->touch;
        data->isDragging = false;  // Reset drag state on new touch
    } else if (input->kHeld & KEY_TOUCH) {
        if (data->touchActive) {
            // Update drag state
            data->isDragging = true;
        }
    } else if (input->kUp & KEY_TOUCH) {
        data->touchActive = false;
    }

    // handle touch location
    if (!data->showDebug) {
        
        // Handle button inputs first
        if (input->kDown & (KEY_DOWN | KEY_DDOWN)) {
            playWavFromRomfs("romfs:/sounds/se_cursor.wav");
            data->selectedAction = (SelectedAction)((data->selectedAction + 1) % ACTION_COUNT);
        } else if (input->kDown & (KEY_UP | KEY_DUP)) {
            playWavFromRomfs("romfs:/sounds/se_cursor.wav");
            data->selectedAction = (SelectedAction)((data->selectedAction + ACTION_COUNT - 1) % ACTION_COUNT);
        } else if (input->kDown & KEY_A) {
            doAction(data->selectedAction);
        }

        // Then handle touch input
        if (data->touchActive) {
            if (input->touch.px >= (data->selectedAction == ACTION_START ? SELECTED_X : UNSELECTED_X) && input->touch.px <= SCREEN_WIDTH_BOTTOM &&
                input->touch.py >= 64 && input->touch.py <= (64 + 64)) {
                data->selectedAction = ACTION_START;
                doAction(ACTION_START);
            }

            // if exit is pressed:
            if (input->touch.px >= (data->selectedAction == ACTION_EXIT ? SELECTED_X : UNSELECTED_X) && input->touch.px <= SCREEN_WIDTH_BOTTOM &&
                input->touch.py >= 134 && input->touch.py <= (134 + 64)) {
                data->selectedAction = ACTION_EXIT;
                doAction(ACTION_EXIT);
            }
        }

    } else if (data->showDebug) {
        // Debug mode input handling
        if (input->kDown & KEY_A) {
            doAction(ACTION_START);
        } else if (input->kDown & KEY_START) {
            doAction(ACTION_EXIT);
        }
    }
}

static void titleUpdate(Scene* scene, float deltaTime) {
    TitleSceneData* data = (TitleSceneData*)scene->data;

    data->offsetX += data->scrollSpeed;
    data->offsetY += data->scrollSpeed;

    // Wrap offsets to match texture size
    if (data->offsetX <= -128.0f) data->offsetX = 0.0f;
    if (data->offsetY <= -128.0f) data->offsetY = 0.0f;
    if (data->offsetX >= 128.0f) data->offsetX = 0.0f;
    if (data->offsetY >= 128.0f) data->offsetY = 0.0f;
}

static void titleDraw(Scene* scene, const GraphicsContext* context) {
    TitleSceneData* data = (TitleSceneData*)scene->data;

    // Clear screens
    C2D_TargetClear(context->top, C2D_Color32(0, 0, 0, 255));
    C2D_TargetClear(context->bottom, C2D_Color32(0, 0, 0, 255));

    Result rc;
    
    // Draw on top screen
    C2D_SceneBegin(context->top);
    
    // Display tiled background image with animation
    rc = displayTiledImage("romfs:/textures/bg_1_0.t3x", 0, 0,
                          SCREEN_WIDTH, SCREEN_HEIGHT,
                          data->offsetX, data->offsetY);

    if (R_FAILED(rc)) {
        panicEverything("Failed to display background image");
        return;
    }

    // Draw the title image on top screen
    rc = displayImage("romfs:/textures/spr_title_0.t3x", 10, 0);
    if (R_FAILED(rc)) {
        panicEverything("Failed to display title image");
        return;
    }

    rc = displayImage("romfs:/textures/spr_titlebanki_0.t3x", 270, 112);
    if (R_FAILED(rc)) {
        panicEverything("Failed to display title banki image");
        return;
    }

    // Draw on bottom screen
    C2D_SceneBegin(context->bottom);
    C2D_TargetClear(context->bottom, C2D_Color32(0, 0, 0, 255));
    
    if (!data->showDebug) {
        // Draw the tiled background on bottom screen
        rc = displayTiledImage("romfs:/textures/bg_1_0.t3x", 0, 0,
                             SCREEN_WIDTH_BOTTOM, SCREEN_HEIGHT_BOTTOM,
                             data->offsetX, data->offsetY);

        if (R_FAILED(rc)) {
            panicEverything("Failed to display bottom screen background");
            return;
        }

        C2D_ImageTint tint;
        for (int i = 0; i < 4; i++) {
            C2D_PlainImageTint(&tint, C2D_Color32(0, 0, 0, 255), 0.5f);
        }

        // Draw the buttons on bottom screen
        rc = displayImageWithScaling("romfs:/textures/spr_ui_0.t3x", (data->selectedAction == ACTION_START ? SELECTED_X : UNSELECTED_X), 64, (data->selectedAction != ACTION_START ? &tint : NULL), 1.0f, 1.0f);
        rc = displayImageWithScaling("romfs:/textures/spr_ui_1.t3x", (data->selectedAction == ACTION_EXIT ? SELECTED_X : UNSELECTED_X), 134, (data->selectedAction != ACTION_EXIT ? &tint : NULL), 1.0f, 1.0f);
    } else {
        // Show debug info
        char debugText[512];
        snprintf(debugText, sizeof(debugText),
            "Title Scene\n"
            "\n"
            "Screen size: %dx%d\n"
            "Offset: %.1f, %.1f\n"
            "Touch Active: %s\n"
            "Is Dragging: %s\n"
            "\nControls:\n"
            "A: Start game\n"
            "START: Exit",
            SCREEN_WIDTH, SCREEN_HEIGHT,
            data->offsetX, data->offsetY,
            data->touchActive ? "Yes" : "No",
            data->isDragging ? "Yes" : "No");

        drawText(10.0f, 10.0f, 0.5f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255), debugText);

        if (R_FAILED(rc)) {
            printf("Failed to display background image on bottom screen: %08lX\n", rc);
            return;
        }
    }
}

static void titleDestroy(Scene* scene) {
    if (scene->data) {
        free(scene->data);
        scene->data = NULL;
    }
}

Scene* createTitleScene(void) {
    Scene* scene = (Scene*)malloc(sizeof(Scene));
    if (!scene) {
        panicEverything("Failed to allocate memory for title scene");
        return NULL;
    }

    TitleSceneData* data = (TitleSceneData*)malloc(sizeof(TitleSceneData));
    if (!data) {
        panicEverything("Failed to allocate memory for title scene data");
        free(scene);
        return NULL;
    }

    // Initialize scene data
    data->offsetX = 0.0f;
    data->offsetY = 0.0f;
    data->scrollSpeed = SCROLL_SPEED;
    data->touchActive = false;
    data->isDragging = false;

    // Set up scene functions and data
    scene->init = titleInit;
    scene->update = titleUpdate;
    scene->draw = titleDraw;
    scene->handleInput = titleHandleInput;
    scene->destroy = titleDestroy;
    scene->data = data;

    return scene;
}