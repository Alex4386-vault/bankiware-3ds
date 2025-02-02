#include "game_complete_scene.h"
#include "../../include/text_renderer.h"
#include "../../include/sound_system.h"
#include "../scene_manager.h"
#include "../common.h"
#include <stdlib.h>
#include <stdio.h>

static void gameCompleteInit(Scene* scene) {
    GameCompleteSceneData* data = (GameCompleteSceneData*)scene->data;
    data->isComplete = false;
    data->elapsedTime = 0.0f;

    data->offsetX = 0.0f;
    data->offsetY = 0.0f;

    playWavFromRomfsLoop("romfs:/sounds/bgm_end.wav");
}

static void gameCompleteUpdate(Scene* scene, float deltaTime) {
    GameCompleteSceneData* data = (GameCompleteSceneData*)scene->data;
    data->elapsedTime += deltaTime;

    UPDATE_BACKGROUND_SCROLL(data, 128.0f, SCROLL_SPEED);
}

static void gameCompleteDraw(Scene* scene, const GraphicsContext* context) {
    GameCompleteSceneData* data = (GameCompleteSceneData*)scene->data;
    
    if (context->top) {
        C2D_SceneBegin(context->top);
        C2D_TargetClear(context->top, C2D_Color32(0, 0, 0, 255));

        // Display tiled background image with animation
        Result rc = displayTiledImage("romfs:/textures/bg_1_0.t3x", 0, 0,
                              SCREEN_WIDTH, SCREEN_HEIGHT,
                              data->offsetX, data->offsetY);

        if (R_FAILED(rc)) {
            panicEverything("Failed to display game complete background");
            return;
        }

        displayImageWithScaling("romfs:/textures/spr_end_0.t3x", 0, 15, NULL, 0.8f, 0.8f);
    }
    
    if (context->bottom) {
        C2D_SceneBegin(context->bottom);
        C2D_TargetClear(context->bottom, C2D_Color32(0, 0, 0, 255));

        // Display tiled background image with animation
        Result rc = displayTiledImage("romfs:/textures/bg_1_0.t3x", 0, 0,
                              SCREEN_WIDTH, SCREEN_HEIGHT,
                              data->offsetX, data->offsetY);
              
        if (R_FAILED(rc)) {
            panicEverything("Failed to display game complete background");
            return;
        }

        drawTextWithFlags(
            SCREEN_WIDTH_BOTTOM / 2,
            SCREEN_HEIGHT_BOTTOM / 2 - 40,
            0.5f, 0.5f, 0.5f,
            C2D_Color32(255, 255, 255, 255),
            C2D_AlignCenter,
            "A:タイトルに戻る"
        );
    }
}

static void gameCompleteHandleInput(Scene* scene, const InputState* input) {
    GameCompleteSceneData* data = (GameCompleteSceneData*)scene->data;
    
    if (input->kDown & KEY_A) {
        stopAudio();
        changeScene(SCENE_TITLE);
    }
}

static void gameCompleteDestroy(Scene* scene) {
    if (scene->data) {
        free(scene->data);
    }
}

Scene* createGameCompleteScene(void) {
    Scene* scene = (Scene*)malloc(sizeof(Scene));
    if (!scene) {
        panicEverything("Failed to allocate memory for game complete scene");
        return NULL;
    }
    
    GameCompleteSceneData* data = (GameCompleteSceneData*)malloc(sizeof(GameCompleteSceneData));
    if (!data) {
        panicEverything("Failed to allocate memory for game complete scene data");
        free(scene);
        return NULL;
    }
    
    // Initialize scene function pointers
    scene->init = gameCompleteInit;
    scene->update = gameCompleteUpdate;
    scene->draw = gameCompleteDraw;
    scene->handleInput = gameCompleteHandleInput;
    scene->destroy = gameCompleteDestroy;
    scene->data = data;
    
    return scene;
}