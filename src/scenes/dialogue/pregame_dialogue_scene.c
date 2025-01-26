#include "pregame_dialogue_scene.h"
#include "../../include/text_renderer.h"
#include "../../include/sound_system.h"
#include "../../include/texture_loader.h"
#include "../scene_manager.h"
#include "../common.h"
#include <stdlib.h>
#include <string.h>

static void pregameDialogueTriggerNext(Scene* scene);

static void pregameDialogueInit(Scene* scene) {
    PregameDialogueData* data = (PregameDialogueData*)scene->data;
    data->isComplete = false;
    data->shouldExit = false;
    data->currentIdx = 0;
    data->stage0Offset = 0.0f;
    data->elapsedTime = 0.0f;
    data->elapsedTimeSinceLast = 0.0f;
    
    // Start playing the looping BGM
    playWavFromRomfsLoop("romfs:/sounds/bgm_gameover2.wav");
}

static void pregameDialogueUpdate(Scene* scene, float deltaTime) {
    PregameDialogueData* data = (PregameDialogueData*)scene->data;

    // No update needed for white screen
    data->elapsedTime += deltaTime;
    data->elapsedTimeSinceLast += deltaTime;

    if (data->elapsedTimeSinceLast > 7.0f && data->currentIdx < 11) {
        pregameDialogueTriggerNext(scene);
    }
}

static void pregameDialogueDraw(Scene* scene, const GraphicsContext* context) {
    PregameDialogueData* data = (PregameDialogueData*)scene->data;

    // Fill both screens with white
    if (context->top) {
        C2D_SceneBegin(context->top);
        C2D_TargetClear(context->top, C2D_Color32(255, 255, 255, 255));

        /* background routine */
        switch (data->currentIdx) {
            case 0: {
                C2D_DrawRectSolid(0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, C2D_Color32(0,127,255,255));
                displayImage("romfs:/textures/bg_sky2_0.t3x", data->stage0Offset, 0);
                displayImage("romfs:/textures/bg_sky2_0.t3x", data->stage0Offset + SCREEN_WIDTH, 0);
                displayImage("romfs:/textures/bg_sky1_0.t3x", 2 * data->stage0Offset, 0);
                displayImage("romfs:/textures/bg_sky1_0.t3x", 2 * data->stage0Offset + SCREEN_WIDTH, 0);
                displayImage("romfs:/textures/bg_sky1_0.t3x", 2 * data->stage0Offset + (2 * SCREEN_WIDTH), 0);

                data->stage0Offset -= SCROLL_SPEED;
                if (data->stage0Offset <= -SCREEN_WIDTH) {
                    data->stage0Offset = 0;
                }
                break;
            }
            case 1: {
                displayImageWithScaling("romfs:/textures/spr_intro1_0.t3x", -56, -20, NULL, 1.0f, 1.0f);
                break;
            }
            case 2:
            case 5: 
            case 6: {
                displayImageWithScaling("romfs:/textures/spr_intro1_0.t3x", -56, -20, NULL, 1.0f, 1.0f);
                displayImageWithScaling("romfs:/textures/spr_intro2_0.t3x", -80, 40, NULL, 1.1f, 1.1f);
                break;
            }
            case 3:
            case 4: {
                displayImage("romfs:/textures/spr_intro3_0.t3x", -56, -12);
                break;
            }
            case 7: {
                C2D_DrawRectSolid(0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, C2D_Color32(0,0,0,255));
                break;
            }
            case 8:
            case 9:
            case 10:
            case 11: {
                displayImage("romfs:/textures/spr_intro4_0.t3x", -56, -12);
                break;
            }

        }

        // Draw text section
        C2D_DrawRectSolid(0, SCREEN_HEIGHT - 40, 0, SCREEN_WIDTH, 40, C2D_Color32(0,0,0, 255));
        char *text = "THIS IS FALLBACK! SOMETHING WENT WRONG!";
        switch (data->currentIdx) {
            case 0: {
                text = "幻想郷——";
                break;
            }
            case 1: {
                text = "幻想郷では、今電子ゲームが大流行中！";
                break;
            }
            case 2: {
                text = "今泉 影狼 「……そこで、私は思ったわけ！」";
                break;
            }
            case 3: {
                text = "今泉 影狼 「草の根妖怪ネットワーク所属の、\n我らが赤蛮奇……もとい、ばんきちゃん」";
                break;
            }
            case 4: {
                text = "今泉 影狼 「ばんきちゃんの圧倒的な\"かわいさ\"は、\nもっと他の妖怪仲間たちにも知られるべきよ！！」";
                break;
            }
            case 5: {
                text = "今泉 影狼 「そこで……ばんきちゃんの『かわいさ』をゲームにして\nもっと知ってもらうのはどうかしら！！」";
                break;
            }
            case 6: {
                text = "わかさぎ姫「すてき~！！大賛成~！！」";
                break;
            }
            case 7: {
                text = "そうして……影狼とわかさぎ姫は赤蛮奇にナイショで\nゲームの開発を急ピッチで進めた";
                break;
            }
            case 8: {
                text = "今泉 影狼 「完成~！！」";
                break;
            }
            case 9: {
                text = "わかさぎ姫「やった~！！」";
                break;
            }
            case 10: {
                text = "今泉 影狼 「さっそく試作品のチェックをしないと！」";
                break;
            }
            case 11: {
                text = "わかさぎ姫「わくわく！！」";
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
        // write Press START to skip in japanese and english
        drawText(10.0f, 10.0f, 0.5f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255), "Aを押して次へ\nPress A to continue\n\nSTARTを押してスキップ\nPress START to skip");
    }
}

static void pregameDialogueTriggerNext(Scene* scene) {
    PregameDialogueData* data = (PregameDialogueData*)scene->data;
    data->elapsedTimeSinceLast = 0.0f;
    if (data->currentIdx == 11) {
        data->shouldExit = true;
        // Stop the BGM before exiting
        stopAudio();

        // switch to next scene "game" with fade
        changeScene(SCENE_GAME);
    } else {
        data->currentIdx++;
    }
}

static void pregameDialogueHandleInput(Scene* scene, const InputState* input) {
    PregameDialogueData* data = (PregameDialogueData*)scene->data;
    
    // If A button is pressed
    if (input->kDown & KEY_A) {
        pregameDialogueTriggerNext(scene);
    } else if (input->kDown & KEY_START) {
        // Stop the BGM before exiting
        data->currentIdx = 11;
        pregameDialogueTriggerNext(scene);
    }
}

static void pregameDialogueDestroy(Scene* scene) {
    if (scene->data) {
        free(scene->data);
    }
}

Scene* createPregameDialogueScene() {
    Scene* scene = (Scene*)malloc(sizeof(Scene));
    if (!scene) return NULL;
    
    PregameDialogueData* data = (PregameDialogueData*)malloc(sizeof(PregameDialogueData));
    if (!data) {
        free(scene);
        return NULL;
    }
    
    // Initialize scene function pointers
    scene->init = pregameDialogueInit;
    scene->update = pregameDialogueUpdate;
    scene->draw = pregameDialogueDraw;
    scene->handleInput = pregameDialogueHandleInput;
    scene->destroy = pregameDialogueDestroy;
    scene->data = data;
    
    return scene;
}