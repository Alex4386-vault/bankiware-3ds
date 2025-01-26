#include "gameover_scene.h"
#include "../../include/text_renderer.h"
#include "../../include/sound_system.h"
#include "../scene_manager.h"
#include "../common.h"
#include <stdlib.h>
#include <stdio.h>

static void gameoverInit(Scene* scene) {
    GameoverSceneData* data = (GameoverSceneData*)scene->data;
    data->isComplete = false;
    data->elapsedTime = 0.0f;

    playWavFromRomfsLoop("romfs:/sounds/bgm_gameover2.wav");
}

static void gameoverUpdate(Scene* scene, float deltaTime) {
    GameoverSceneData* data = (GameoverSceneData*)scene->data;
    data->elapsedTime += deltaTime;
}

static void gameoverDraw(Scene* scene, const GraphicsContext* context) {
    GameoverSceneData* data = (GameoverSceneData*)scene->data;
    
    if (context->top) {
        C2D_SceneBegin(context->top);
        C2D_TargetClear(context->top, C2D_Color32(0, 0, 0, 255));

        // Draw "GAME OVER" text
        drawTextWithFlags(
            SCREEN_WIDTH / 2, 
            SCREEN_HEIGHT / 2 - 20,
            1.0f, 1.0f, 1.0f,
            C2D_Color32(255, 255, 255, 255),
            C2D_AlignCenter,
            "GAME OVER"
        );
    }
    
    if (context->bottom) {
        C2D_SceneBegin(context->bottom);
        C2D_TargetClear(context->bottom, C2D_Color32(0, 0, 0, 255));

        drawTextWithFlags(
            SCREEN_WIDTH_BOTTOM / 2,
            SCREEN_HEIGHT_BOTTOM / 2 - 40,
            0.5f, 0.5f, 0.5f,
            C2D_Color32(255, 255, 255, 255),
            C2D_AlignCenter,
            "リトライする？\n\n"
            "A：する！\n"
            "B：あきらめる"
        );
    }
}

static void gameoverHandleInput(Scene* scene, const InputState* input) {
    GameoverSceneData* data = (GameoverSceneData*)scene->data;
    
    if (input->kDown & KEY_B) {
        stopAudio();
        changeScene(SCENE_TITLE);
    }

    if (input->kDown & KEY_A) {
        stopAudio();
        changeScene(SCENE_GAME);
    }
}

static void gameoverDestroy(Scene* scene) {
    if (scene->data) {
        free(scene->data);
    }
}

Scene* createGameoverScene(void) {
    Scene* scene = (Scene*)malloc(sizeof(Scene));
    if (!scene) return NULL;
    
    GameoverSceneData* data = (GameoverSceneData*)malloc(sizeof(GameoverSceneData));
    if (!data) {
        free(scene);
        return NULL;
    }
    
    // Initialize scene function pointers
    scene->init = gameoverInit;
    scene->update = gameoverUpdate;
    scene->draw = gameoverDraw;
    scene->handleInput = gameoverHandleInput;
    scene->destroy = gameoverDestroy;
    scene->data = data;
    
    return scene;
}