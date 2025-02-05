#include <stdlib.h>
#include <stdio.h>
#include <3ds.h>
#include <citro2d.h>

#include "../common.h"
#include "../../include/texture_loader.h"
#include "../../include/text_renderer.h"
#include "../../include/sound_system.h"
#include "../scene_manager.h"
#include "game_scene.h"
#include "game_levels.h"

#define ANIMATION_LENGTH 0.1f

static void gameDrawTV(Scene* scene);

static void cleanupLevelData(GameSceneData* data) {
    if (data->currentLevelData != NULL) {
        free(data->currentLevelData);
        data->currentLevelData = NULL;
    }
    data->currentLevelObj = NULL;

    stopAudio();
}

static void gameInit(Scene* scene) {
    GameSceneData* data = (GameSceneData*)scene->data;
    
    // Initialize all data fields
    memset(data, 0, sizeof(GameSceneData));
    
    // Set initial values
    data->remainingLife = 4;
    data->scrollSpeed = SCROLL_SPEED;
    data->bounceState = BOUNCE_ALL;
    data->bounceAnimationInterval = 0.5f;
    data->bounceAnimationTimer = -1.0f;
    data->shouldIncreaseLevelAt = 1.8f + 0.5f;
    data->shouldEnterGameAt = 1.8f + 2.0f;
    data->showSpeedUpAt = -1.0f;
    data->showSpeedUpTimer = -1.0f;
    data->showBossStageAt = -1.0f;
    data->showBossStageTimer = -1.0f;
    data->gameLevelOffset = rand() % 9;
    data->gameSessionTime = 4.0f;
    data->showTimer = true;
    
    playWavFromRomfsRange("romfs:/sounds/bgm_ready.wav", 0, SECONDS_TO_SAMPLES(1.8f));
    queueWavFromRomfsRange("romfs:/sounds/bgm_jingleNext.wav", 0, SECONDS_TO_SAMPLES(2.0f));
}

static GameLevel* getCurrentLevel(GameSceneData* data) {
    if (data->currentLevel == 10) {
        return &BossStageGame;
    }
    return getGameLevel(data->currentLevel + data->gameLevelOffset);
}

static void gameEnterHandler(Scene *scene) {
    GameSceneData* data = (GameSceneData*)scene->data;
    
    // Initialize the current level
    GameLevel* currentLevel = getCurrentLevel(data);
    if (!currentLevel) {
        panicEverything("Failed to load game level");
        return;
    }
    
    data->currentLevelObj = currentLevel;
    if (currentLevel->init) {
        currentLevel->init(data);
    }

    // TODO: When it speeds up, update it accordingly
    data->gameLeftTime = data->gameSessionTime;
    data->elapsedTimeSinceStageScreen = 0.0f;
    
    // Set default values
    data->isInGame = true;
}

static void gameLeaveHandler(Scene *scene) {
    GameSceneData* data = (GameSceneData*)scene->data;
    
    // First reset the level if needed
    GameLevel* currentLevel = getCurrentLevel(data);
    if (currentLevel && currentLevel->reset) {
        currentLevel->reset(data);
    }
    
    // Then clean up level data
    cleanupLevelData(data);
    
    // Update game state
    data->isInGame = false;
    data->elapsedTimeSinceStageScreen = 0.0f;

    int savedGameState = data->lastGameState;
    if (data->lastGameState == GAME_UNDEFINED) {
        data->lastGameState = GAME_FAILURE;
    }
    
    // Handle state transitions
    if (data->lastGameState == GAME_SUCCESS) {
        data->bankiState = BANKI_EXCITED;
        playWavFromRomfsRange("romfs:/sounds/bgm_jingle1.wav", 0, SECONDS_TO_SAMPLES(1.8f));
    } else if (data->lastGameState == GAME_FAILURE) {
        data->bankiState = BANKI_SAD;
        playWavFromRomfsRange("romfs:/sounds/bgm_jingle2.wav", 0, SECONDS_TO_SAMPLES(1.8f));
        data->remainingLife--;
    }

    data->lastGameState = GAME_UNDEFINED;
    
    // Check for game over
    if (data->remainingLife <= 0) {
        data->isComplete = true;
        queueWavFromRomfs("romfs:/sounds/bgm_gameover.wav");
    } else {
        data->shouldEnterGameAt = 1.8f + 2.0f;

        if (data->currentLevel < 10) {
            data->shouldIncreaseLevelAt = 1.8f + 0.5f;

            // Set up next level timings
            if (data->currentLevel < 9) {
                // if the levels are 3, 6 we need to speed up
                if (data->currentLevel == 3 || data->currentLevel == 6) {
                    data->gameSessionTime -= 0.25f;

                    float speedUpTime = 3.5f;

                    // also we play speed up sound
                    data->shouldIncreaseLevelAt += speedUpTime;
                    data->shouldEnterGameAt += speedUpTime;

                    data->showSpeedUpAt = 1.8f;
                    data->showSpeedUpTimer = speedUpTime;

                    queueWavFromRomfsRange("romfs:/sounds/bgm_jingleSpeedUp.wav", 0, SECONDS_TO_SAMPLES(speedUpTime));
                }
            } else if (data->currentLevel == 9) {
                // we are now entering the boss stage
                float bossStageTime = 3.5f;

                // also we play speed up sound
                data->shouldIncreaseLevelAt += bossStageTime;
                data->shouldEnterGameAt += bossStageTime;

                data->showBossStageAt = 1.8f;
                data->showBossStageTimer = bossStageTime;

                // entering the boss stage.
                // the boss stage takes 20 seconds.
                data->gameSessionTime = 20.0f;
                data->showTimer = false;

                queueWavFromRomfsRange("romfs:/sounds/bgm_jingleBossstage.wav", 0, SECONDS_TO_SAMPLES(bossStageTime));
            }

            queueWavFromRomfs("romfs:/sounds/bgm_jingleNext.wav");
        } else {
            if (savedGameState == GAME_SUCCESS) {
                data->isComplete = true;

                // handle termination
                queueWavFromRomfs("romfs:/sounds/bgm_gameover.wav");
            } else {
                // re-enter the level
                data->shouldIncreaseLevelAt = -1.0f;
                queueWavFromRomfs("romfs:/sounds/bgm_jingleNext.wav");
            }
        }
    }
}

static void gameUpdate(Scene* scene, float deltaTime) {
    GameSceneData* data = (GameSceneData*)scene->data;
    if (data == NULL) return;

    data->elapsedTime += deltaTime;
    data->bounceTimer += deltaTime;

    // Update background scrolling
    data->offsetX += data->scrollSpeed;
    data->offsetY += data->scrollSpeed;

    // Wrap offsets to match texture size
    if (data->offsetX <= -128.0f) data->offsetX = 0.0f;
    if (data->offsetY <= -128.0f) data->offsetY = 0.0f;
    if (data->offsetX >= 128.0f) data->offsetX = 0.0f;
    if (data->offsetY >= 128.0f) data->offsetY = 0.0f;

    if (data->isComplete) {
        // for handling timeout, we need to handle elapsedTimeSinceStageScreen here
        data->elapsedTimeSinceStageScreen += deltaTime;

        // check if it is gameover or not
        if (data->remainingLife <= 0) {
            // game over
            if (data->elapsedTimeSinceStageScreen > 6.0f) {
                stopAudio();
                changeScene(SCENE_GAMEOVER);
            }
        } else {
            // game complete
            if (data->elapsedTimeSinceStageScreen > 6.0f) {
                stopAudio();
                changeScene(SCENE_POSTGAME_DIALOGUE);
            } else if (data->elapsedTimeSinceStageScreen > 5.5f) {
                // stop playing anything
                stopAudio();
            }
        }

        // DO NOT HANDLE ANYTHING ELSE.
        return;
    }

    if (data->isInGame) {
        if (data->gameLeftTime > 0.0f) {
            data->gameLeftTime -= deltaTime;
            if (data->gameLeftTime <= 0.0f) {
                data->gameLeftTime = 0.0f;
                gameLeaveHandler(scene);
                return;
            }
            
            // Update current level
            GameLevel* currentLevel = getCurrentLevel(data);
            if (currentLevel && currentLevel->update) {
                currentLevel->update(data, deltaTime);
            }
                
            // Check for immediate quit request
            if (currentLevel && currentLevel->requestingQuit != NULL) {
                if (currentLevel->requestingQuit(data)) {
                    // Success state should already be set by the level
                    gameLeaveHandler(scene);
                    return;
                }
            }
        }
    } else {
      data->elapsedTimeSinceStageScreen += deltaTime;

    
      if (data->shouldIncreaseLevelAt > 0.0f) {
        if (data->elapsedTimeSinceStageScreen > data->shouldIncreaseLevelAt) {
            data->currentLevel++;
            data->shouldIncreaseLevelAt = -1.0f;
        }
      }

      if (data->shouldEnterGameAt > 0.0f) {
        if (data->elapsedTimeSinceStageScreen > data->shouldEnterGameAt) {
            gameEnterHandler(scene);
            data->shouldEnterGameAt = -1.0f;
        }
      }
    }

    if (data->bounceTimer > (data->bounceAnimationInterval - (ANIMATION_LENGTH / 2))) {
        data->bounceTimer = data->bounceTimer - data->bounceAnimationInterval;
        data->bounceAnimationTimer = 0.0f;
    }

    if (data->bounceAnimationTimer >= ANIMATION_LENGTH) {
        data->bounceAnimationTimer = -1.0f;
    } else if (data->bounceAnimationTimer >= 0.0f) {
        data->bounceAnimationTimer += deltaTime;
    }

    if (data->showSpeedUpTimer > 0.0f && data->showSpeedUpAt <= data->elapsedTimeSinceStageScreen) {
        data->showSpeedUpTimer -= deltaTime;
        if (data->showSpeedUpTimer <= 0.0f) {
            data->showSpeedUpTimer = -1.0f;
        }
    }

    if (data->showBossStageTimer > 0.0f && data->showBossStageAt <= data->elapsedTimeSinceStageScreen) {
        data->showBossStageTimer -= deltaTime;
        if (data->showBossStageTimer <= 0.0f) {
            data->showBossStageTimer = -1.0f;
        }
    }
}

static void gameShowBankiAt(Scene* scene, float x, float y) {
    GameSceneData* data = (GameSceneData*)scene->data;
    BankiState state = data->bankiState;
    const char* textureName = NULL;

    switch (state) {
        case BANKI_IDLE:
            textureName = "romfs:/textures/spr_lifebanki_0.t3x";
            break;
        case BANKI_EXCITED:
            textureName = "romfs:/textures/spr_lifebanki_1.t3x";
            break;
        case BANKI_SAD:
            textureName = "romfs:/textures/spr_lifebanki_2.t3x";
            break;
    }

    if (textureName) {
        float scale = 1.0f;
        float offsetX = 0.0f;
        float offsetY = 0.0f;

        if (data->bounceState == BOUNCE_ALL || data->bounceState == BOUNCE_BANKI) {
            if (data->bounceAnimationTimer >= 0.0f) {
                float progress = data->bounceAnimationTimer / ANIMATION_LENGTH;
                float sinValue = sinf(progress * M_PI);

                scale += 0.1f * sinValue;

                // Offset the image slightly, this is 64x64 image
                offsetX = (-32.0f * 0.1f) * sinValue;
                offsetY = (-32.0f * 0.1f) * sinValue;
            }
        }

        displayImageWithScaling(textureName, x + offsetX, y + offsetY, NULL, scale, scale);
    }
}

static void resetGameTimer(Scene* scene) {
    GameSceneData* data = (GameSceneData*)scene->data;
    data->gameLeftTime = data->gameSessionTime;
}

static void gameDrawWakaKage(Scene* scene) {    
    GameSceneData* data = (GameSceneData*)scene->data;
    BankiState state = data->bankiState;
    const char* textureName = NULL;

    switch (state) {
        case BANKI_IDLE:
            textureName = "romfs:/textures/spr_wakakage1_0.t3x";
            break;
        case BANKI_EXCITED:
            textureName = "romfs:/textures/spr_wakakage1_1.t3x";
            break;
        case BANKI_SAD:
            textureName = "romfs:/textures/spr_wakakage1_2.t3x";
            break;
    }

    if (textureName) {
        float scale = 0.8f;
        float offsetX = 0.0f;
        float offsetY = 0.0f;

        if (data->bounceState == BOUNCE_ALL) {
            if (data->bounceAnimationTimer >= 0.0f) {
                float progress = data->bounceAnimationTimer / ANIMATION_LENGTH;
                float sinValue = sinf(progress * M_PI);

                scale += 0.1f * sinValue;

                // Offset the image slightly, this is 512x512 image
                offsetX = (-256.0f * 0.1f) * sinValue;
                offsetY = (-128.0f * 0.1f) * sinValue;
            }
        }

        displayImageWithScaling(textureName, -12 + offsetX, 40 + offsetY, NULL, scale, scale);
    }
}

static void gameDrawTimer(Scene *scene) {
    GameSceneData* data = (GameSceneData*)scene->data;

    float gameTick = data->gameSessionTime / 8;
    float spentTime = data->gameSessionTime - data->gameLeftTime;
    float progress = spentTime / gameTick;
    if (progress < 0.0f) progress = 0.0f;

    float offsetX = 0.0f, offsetY = 0.0f, scale = 0.9f;
    int imgType = (int) (progress * 4) % 2;

    int fileId = (int) progress;
    if (fileId < 1) fileId = 0;
    if (fileId > 7) fileId = 7;

    char fileName[64];
    if (fileId < 7) {
        snprintf(fileName, sizeof(fileName), "romfs:/textures/spr_bakudan%d_%d.t3x", fileId, imgType);
        if (imgType == 0 && fileId < 6) {
            scale = 1.01f;
        }
    } else {
        imgType = 0;
        strcpy(fileName, "romfs:/textures/spr_bakudan7_0.t3x");
    }


    displayImageWithScaling(fileName, 10, SCREEN_HEIGHT_BOTTOM - 32, NULL, scale, scale);
    if (progress >= 3.0f && fileId < 7) {
        // show three, two, one
        int number = 3 - (int) (progress - 3.0f);
        float currentProgress = progress - (int) progress;
        float diff = ((1.0 - currentProgress) * 0.5f);
        float scale = 0.75f + diff;

        float offsetX = (-16.0 * diff), offsetY = (-16.0 * diff);

        char path[64];
        snprintf(path, sizeof(path), "romfs:/textures/spr_count_%d.t3x", number);

        displayImageWithScaling(path, 10 + offsetX, SCREEN_HEIGHT_BOTTOM - GAME_TIMER_HEIGHT + offsetY, NULL, scale, scale);
    }
}

static void gameDraw(Scene* scene, const GraphicsContext* context) {
    GameSceneData* data = (GameSceneData*)scene->data;

    // Let current level draw its content
    if (data->isInGame) {    
        GameLevel* currentLevel = getCurrentLevel(data);
        if (currentLevel && currentLevel->draw) {
            currentLevel->draw(data, context);
        }

        // Draw the timer
        if (context->bottom) {
            C2D_SceneBegin(context->bottom);
            if (data->showTimer) {
                gameDrawTimer(scene);
            }
        }

        return;
    }

    // Clear screens with black background
    if (context->top) {
        C2D_SceneBegin(context->top);
        C2D_TargetClear(context->top, C2D_Color32(0, 0, 0, 255));

        // Display tiled background image with animation
        Result rc = displayTiledImage("romfs:/textures/bg_1_0.t3x", 0, 0,
                              SCREEN_WIDTH, SCREEN_HEIGHT,
                              data->offsetX, data->offsetY);   

                              
        if (R_FAILED(rc)) {
            panicEverything("Failed to display game background");
            return;
        }
        // draw wakakage
        gameDrawWakaKage(scene);

        // show life counter
        int life = data->remainingLife;
        switch(life) {
          case 4: 
            gameShowBankiAt(scene, 296, 10);
          case 3:
            gameShowBankiAt(scene, 211, 10);
          case 2:
            gameShowBankiAt(scene, 126, 10);
          case 1:
            gameShowBankiAt(scene, 40, 10);
          default:
            break;
        }


        gameDrawTV(scene);
    }
    
    if (context->bottom) {
        C2D_SceneBegin(context->bottom);
        C2D_TargetClear(context->bottom, C2D_Color32(0, 0, 0, 255));

        if (data->isDebug) {
            char *bankiState = (data->bankiState == BANKI_IDLE) ? "Idle" : (data->bankiState == BANKI_EXCITED) ? "Excited" : "Sad";
            // Draw some text to show we're in the game scene
            char timeText[256];
            snprintf(timeText, sizeof(timeText), "Game Time: %.2f\n"
            "GameLeftTime: %.2f\n"
            "Life Remaining: %d\n"
            "State: %s\n"
            "StageTime: %.2f\n"
            "Actual Level: %d\n"
            "\n"
            "Press START to exit to TITLE\n"
            "Press A to reset game Timer\n"
            "B: Idle, X: Fail, Y: Success\n"
            "Up, Down: Life count\n"
            "Left, Right: Current Level", data->elapsedTime, data->gameLeftTime, data->remainingLife, bankiState, data->elapsedTimeSinceStageScreen, (data->currentLevel + data->gameLevelOffset));
            drawText(10.0f, 10.0f, 0.5f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255), timeText);
        } else {
            // Draw the tiled background on bottom screen
            Result rc = displayTiledImage("romfs:/textures/bg_1_0.t3x", 0, 0,
                                SCREEN_WIDTH_BOTTOM, SCREEN_HEIGHT_BOTTOM,
                                data->offsetX, data->offsetY);

            if (R_FAILED(rc)) {
                return;
            }

            if (data->showSpeedUpTimer > 0 && data->showSpeedUpAt <= data->elapsedTimeSinceStageScreen) {
                float animProgress = (data->elapsedTimeSinceStageScreen - data->showSpeedUpAt) / 0.5f; // 0.5s animation
                if (animProgress > 1.0f) animProgress = 1.0f;
                
                // Apply ease-out using quadratic formula: 1 - (1-x)^2
                float easeOutProgress = 1.0f - ((1.0f - animProgress) * (1.0f - animProgress));
                
                float startX = SCREEN_WIDTH_BOTTOM;
                float endX = SCREEN_WIDTH_BOTTOM / 2 - 128;
                float currentX = startX + (endX - startX) * easeOutProgress;
                
                displayImage("romfs:/textures/spr_speedup_0.t3x", currentX, SCREEN_HEIGHT_BOTTOM / 2 - 32);
            }

            if (data->showBossStageTimer > 0 && data->showBossStageAt <= data->elapsedTimeSinceStageScreen) {
                float animProgress = (data->elapsedTimeSinceStageScreen - data->showBossStageAt) / 0.5f; // 0.5s animation
                if (animProgress > 1.0f) animProgress = 1.0f;
                
                // Apply ease-out using quadratic formula: 1 - (1-x)^2
                float easeOutProgress = 1.0f - ((1.0f - animProgress) * (1.0f - animProgress));
                
                float startX = SCREEN_WIDTH_BOTTOM;
                float endX = SCREEN_WIDTH_BOTTOM / 2 - 128;
                float currentX = startX + (endX - startX) * easeOutProgress;
                
                displayImage("romfs:/textures/spr_bossstage_0.t3x", currentX, SCREEN_HEIGHT_BOTTOM / 2 - 32);
            }
        }
    }
}

static void gameDrawTV(Scene* scene) {
    GameSceneData* data = (GameSceneData*)scene->data;

    float offsetX = 0.0f, offsetY = 0.0f, scale = 1.0f;
    if (data->bounceState == BOUNCE_ALL) {
        if (data->bounceAnimationTimer >= 0.0f) {
            float progress = data->bounceAnimationTimer / ANIMATION_LENGTH;
            float sinValue = sinf(progress * M_PI);

            scale += 0.1f * sinValue;

            // Offset the image slightly, this is 64x64 image
            offsetX = (-64.0f * 0.1f) * sinValue;
            offsetY = (-64.0f * 0.1f) * sinValue;
        }
    }

    displayImageWithScaling("romfs:/textures/spr_tv2_0.t3x", SCREEN_WIDTH / 2 - 64 + offsetX, SCREEN_HEIGHT / 2 - 40 + offsetY, NULL, scale, scale);
    displayImageWithScaling("romfs:/textures/spr_tv1_0.t3x", SCREEN_WIDTH / 2 - 64 + offsetX, SCREEN_HEIGHT / 2 - 40 + offsetY, NULL, scale, scale);

    // draw text
    char level[16];
    snprintf(level, sizeof(level), "%d", data->currentLevel);
    drawTextWithFlags(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 12, 0.5f, scale, scale, C2D_Color32(255, 255, 255, 255), C2D_AlignCenter, level);
}

static void gameHandleInput(Scene* scene, const InputState* input) {
    GameSceneData* data = (GameSceneData*)scene->data;

    if (data->isInGame) {    
        GameLevel* currentLevel = getCurrentLevel(data);
        if (currentLevel && currentLevel->handleInput) {
            currentLevel->handleInput(data, input);
        }
    } else {
        // handle debug query if it is not in game
        if (input->kDown & KEY_SELECT) {
            data->isDebug = !data->isDebug;
        }
    }

    if (data->isDebug && !data->isInGame) {
        if (input->kDown & KEY_START) {
            stopAudio();
            changeSceneImmediate(SCENE_TITLE);
        }
        if (input->kDown & KEY_A) {
            resetGameTimer(scene);
        }
        if (input->kDown & KEY_X) {
            data->bankiState = BANKI_SAD;
        }
        if (input->kDown & KEY_Y) {
            data->bankiState = BANKI_EXCITED;
        }
        if (input->kDown & KEY_B) {
            data->bankiState = BANKI_IDLE;
        }
        if (input->kDown & KEY_L) {
            data->bounceState = BOUNCE_ALL;
        }
        if (input->kDown & KEY_R) {
            data->bounceState = BOUNCE_BANKI;
        }
        if (input->kDown & (KEY_DDOWN | KEY_DOWN)) {
            if (data->remainingLife > 0) {
                data->remainingLife--;
            }
        }
        if (input->kDown & (KEY_DUP | KEY_UP)) {
            if (data->remainingLife < 4) {
                data->remainingLife++;
            }
        }
        if (input->kDown & (KEY_DLEFT | KEY_LEFT)) {
            if (data->currentLevel > 1) {
                data->currentLevel--;
            }
        }
        if (input->kDown & (KEY_DRIGHT | KEY_RIGHT)) {
            if (data->currentLevel < 10) {
                data->currentLevel++;
            }
        }
    }
}

static void gameDestroy(Scene* scene) {
    if (scene->data) {
        GameSceneData* data = (GameSceneData*)scene->data;
        // Free any remaining level data
        if (data->currentLevelData != NULL) {
            free(data->currentLevelData);
            data->currentLevelData = NULL;
        }
        free(scene->data);
    }
}

Scene* createGameScene(void) {
    Scene* scene = (Scene*)malloc(sizeof(Scene));
    if (!scene) {
        panicEverything("Failed to allocate memory for game scene");
        return NULL;
    }
    
    GameSceneData* data = (GameSceneData*)malloc(sizeof(GameSceneData));
    if (!data) {
        panicEverything("Failed to allocate memory for game scene data");
        free(scene);
        return NULL;
    }
    
    // Initialize scene function pointers
    scene->init = gameInit;
    scene->update = gameUpdate;
    scene->draw = gameDraw;
    scene->handleInput = gameHandleInput;
    scene->destroy = gameDestroy;
    scene->data = data;
    
    return scene;
}