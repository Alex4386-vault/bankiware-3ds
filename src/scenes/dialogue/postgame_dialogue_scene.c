#include "postgame_dialogue_scene.h"
#include "../../include/text_renderer.h"
#include "../../include/sound_system.h"
#include "../../include/texture_loader.h"
#include "../scene_manager.h"
#include "../common.h"
#include <stdlib.h>
#include <string.h>

static void postgameDialogueTriggerNext(Scene* scene);

static void postgameDialogueInit(Scene* scene) {
    PostgameDialogueData* data = (PostgameDialogueData*)scene->data;
    data->isComplete = false;
    data->shouldExit = false;
    data->currentIdx = 0;
    data->stage0Offset = 0.0f;
    data->elapsedTime = 0.0f;
    data->elapsedTimeSinceLast = 0.0f;
    
    // Start playing the ending BGM
    playWavFromRomfsLoop("romfs:/sounds/bgm_gameover2.wav");
}

static void postgameDialogueUpdate(Scene* scene, float deltaTime) {
    PostgameDialogueData* data = (PostgameDialogueData*)scene->data;

    data->elapsedTime += deltaTime;
    data->elapsedTimeSinceLast += deltaTime;

    if (data->elapsedTimeSinceLast > 7.0f && data->currentIdx < 8) {
        postgameDialogueTriggerNext(scene);
    }
}

static void postgameDialogueDraw(Scene* scene, const GraphicsContext* context) {
    PostgameDialogueData* data = (PostgameDialogueData*)scene->data;

    if (context->top) {
        C2D_SceneBegin(context->top);
        C2D_TargetClear(context->top, C2D_Color32(255, 255, 255, 255));

        /* background routine */
        switch (data->currentIdx) {
            case 0:
            case 1: {
                displayImageWithScaling("romfs:/textures/spr_outro1_0.t3x", -56, -20, NULL, 1.0f, 1.0f);
                break;
            }
            case 2: {
                displayImageWithScaling("romfs:/textures/spr_outro2_0.t3x", -56, -20, NULL, 1.0f, 1.0f);
                break;
            }
            case 3: {
                displayImageWithScaling("romfs:/textures/spr_outro3_0.t3x", -56, -20, NULL, 1.0f, 1.0f);
                break;
            }
            case 4:
            case 5:
            case 6:
            case 7: {
                displayImage("romfs:/textures/spr_outro4_0.t3x", -56, -12);
                break;
            }
            case 8: {
                C2D_DrawRectSolid(0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, C2D_Color32(0,0,0,255));
            }
        }

        // Draw text section
        C2D_DrawRectSolid(0, SCREEN_HEIGHT - 40, 0, SCREEN_WIDTH, 40, C2D_Color32(0,0,0, 255));
        char *text = "THIS IS FALLBACK! SOMETHING WENT WRONG!";
        switch (data->currentIdx) {
            case 0: {
                text = "今泉 影狼「我ながら、なんてすばらしい出来……」";
                break;
            }
            case 1: {
                text = "わかさぎ姫「ばんきちゃんのかわいさがいっぱいだわ～\nさっそく天狗に頼んで流通を……」";
                break;
            }
            case 2: {
                text = "赤蛮奇「……おい、ふたりとも」";
                break;
            }
            case 3: {
                text = "今泉 影狼＆わかさぎ姫「ギクッ！！！」";
                break;
            }
            case 4: {
                text = "赤蛮奇「わたしにナイショで、なんてものを作ってるんだー！？」";
                break;
            }
            case 5: {
                text = "今泉 影狼「だ、だって……」";
                break;
            }
            case 6: {
                text = "わかさぎ姫「ばんきちゃんはかわいいから……」";
                break;
            }
            case 7: {
                text = "赤蛮奇「その気持ちはうれしいけど！\nこれは没収！！そしてふたりにはおしおき！！」";
                break;
            }
            case 8: {
                text = "今泉 影狼＆わかさぎ姫「ゆ、ゆるして～！！」";
                break;
            }
        }
        float yOffset = 0.0f;

        // check if contains new line
        if (strchr(text, '\n') != NULL) {
            yOffset = -10.0f;
        }

        drawTextWithFlags(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 22 + yOffset, 0.5f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255), C2D_AlignCenter, text);
    }
    
    if (context->bottom) {
        C2D_SceneBegin(context->bottom);
        C2D_TargetClear(context->bottom, C2D_Color32(255, 255, 255, 255));
        C2D_DrawRectSolid(0, 0, 0, SCREEN_WIDTH_BOTTOM, SCREEN_HEIGHT_BOTTOM, C2D_Color32(0,0,0, 255));

        // Draw text on bottom screen
        drawText(10.0f, 10.0f, 0.5f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255), "Aを押して次へ\nPress A to continue\n\nSTARTを押してスキップ\nPress START to skip");
    }
}

static void postgameDialogueTriggerNext(Scene* scene) {
    PostgameDialogueData* data = (PostgameDialogueData*)scene->data;
    data->elapsedTimeSinceLast = 0.0f;
    if (data->currentIdx == 8) {
        data->shouldExit = true;
        // Stop the BGM before exiting
        stopAudio();

        // TODO: Implement switching to SCENE_GAME_COMPLETE after scene implementation
        changeScene(SCENE_GAME_COMPLETE);
    } else {
        data->currentIdx++;
    }
}

static void postgameDialogueHandleInput(Scene* scene, const InputState* input) {
    PostgameDialogueData* data = (PostgameDialogueData*)scene->data;
    
    // If A button is pressed
    if (input->kDown & KEY_A) {
        postgameDialogueTriggerNext(scene);
    } else if (input->kDown & KEY_START) {
        // Skip to the end
        data->currentIdx = 8;
        postgameDialogueTriggerNext(scene);
    }
}

static void postgameDialogueDestroy(Scene* scene) {
    if (scene->data) {
        free(scene->data);
    }
}

Scene* createPostgameDialogueScene() {
    Scene* scene = (Scene*)malloc(sizeof(Scene));
    if (!scene) {
        panicEverything("Failed to allocate memory for postgame dialogue scene");
        return NULL;
    }
    
    PostgameDialogueData* data = (PostgameDialogueData*)malloc(sizeof(PostgameDialogueData));
    if (!data) {
        panicEverything("Failed to allocate memory for postgame dialogue data");
        free(scene);
        return NULL;
    }
    
    // Initialize scene function pointers
    scene->init = postgameDialogueInit;
    scene->update = postgameDialogueUpdate;
    scene->draw = postgameDialogueDraw;
    scene->handleInput = postgameDialogueHandleInput;
    scene->destroy = postgameDialogueDestroy;
    scene->data = data;
    
    return scene;
}