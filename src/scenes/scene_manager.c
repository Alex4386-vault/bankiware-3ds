#include "scene_manager.h"
#include "title/title_scene.h"
#include "dialogue/pregame_dialogue_scene.h"
#include "dialogue/postgame_dialogue_scene.h"
#include "game/game_scene.h"
#include "gameover/gameover_scene.h"
#include "game_complete/game_complete_scene.h"
#include "../include/sound_system.h"
#include "../include/text_renderer.h"
#include <stdlib.h>

#define MAX_TOUCH_TRANSITIONS 10
#define FADE_DURATION 0.25f  // Reduced from 0.5f to 0.25f for faster fade
#define MAX_ALPHA 1.0f

// #define SCENE_DEBUG

// Helper function to get scene type name
static const char* getSceneTypeName(SceneType type) {
    switch (type) {
        case SCENE_TITLE:
            return "Title";
        case SCENE_PREGAME_DIALOGUE:
            return "Pregame Dialogue";
        case SCENE_GAME:
            return "Game";
        case SCENE_GAMEOVER:
            return "Game Over";
        case SCENE_POSTGAME_DIALOGUE:
            return "Postgame Dialogue";
        case SCENE_GAME_COMPLETE:
            return "Game Complete";
        default:
            return "Unknown";
    }
}

static Scene* currentScene = NULL;
static Scene* nextScene = NULL;
static TouchTransition touchTransitions[MAX_TOUCH_TRANSITIONS];
static int numTouchTransitions = 0;
bool requestedExit = false;

// Fade-related variables
static FadeState fadeState = FADE_NONE;
static float fadeAlpha = 0.0f;
static float fadeTimer = 0.0f;
static SceneType pendingSceneType = SCENE_TITLE;
static SceneType currentSceneType = SCENE_TITLE;
static Result lastCreationError = 0;  // Store last scene creation error

// Forward declarations
static Result createNewScene(SceneType type, Scene** scene);
static bool isPointInRect(int x, int y, int rectX, int rectY, int rectW, int rectH);
static void handleTouchTransitions(const touchPosition* touch);

Result initSceneManager(void) {
    numTouchTransitions = 0;
    fadeState = FADE_NONE;
    fadeAlpha = 0.0f;
    fadeTimer = 0.0f;
    currentSceneType = SCENE_TITLE;
    return changeSceneImmediate(SCENE_TITLE);
}

void exitSceneManager(void) {
    // Reset fade state
    fadeState = FADE_NONE;
    fadeAlpha = 0.0f;
    fadeTimer = 0.0f;

    // Clean up current scene
    if (currentScene) {
        if (currentScene->destroy) {
            currentScene->destroy(currentScene);
        }
        free(currentScene);
        currentScene = NULL;
    }

    // Clean up next scene if it exists
    if (nextScene) {
        if (nextScene->destroy) {
            nextScene->destroy(nextScene);
        }
        free(nextScene);
        nextScene = NULL;
    }

    clearTouchTransitions();
}

Result changeScene(SceneType type) {
    // If already fading, handle based on current fade state
    if (fadeState != FADE_NONE) {
        // If we're fading out, we can update the pending scene
        pendingSceneType = type;

        // If we're fading in and get a new scene request,
        // wait until fade in completes
        if (fadeState == FADE_IN) {
            return 0;
        }
        
        // Clean up any existing next scene
        if (nextScene) {
            if (nextScene->destroy) {
                nextScene->destroy(nextScene);
            }
            free(nextScene);
            nextScene = NULL;
        }
        return 0;
    }

    // Start new fade out transition
    printf("Starting fade out to scene type %d\n", type);
    fadeState = FADE_OUT;
    fadeTimer = 0.0f;
    fadeAlpha = 0.0f;
    pendingSceneType = type;
    return 0;
}

Result changeSceneImmediate(SceneType type) {
    Scene* newScene = NULL;
    Result rc = createNewScene(type, &newScene);
    
    if (R_FAILED(rc)) {
        printf("Failed to create new scene of type %d: %08lX\n", type, rc);
        return rc;
    }
    if (!newScene) {
        printf("New scene is NULL after creation\n");
        return -1;
    }

    // Clean up current scene
    if (currentScene) {
        if (currentScene->destroy) {
            currentScene->destroy(currentScene);
        }
        free(currentScene);
    }

    // Set new scene
    currentScene = newScene;
    currentSceneType = type;
    clearTouchTransitions();

    return 0;
}

static Result createNewScene(SceneType type, Scene** scene) {
    Scene* newScene = NULL;
    Result rc = 0;

    // Clean up any existing scene at the target pointer
    if (*scene) {
        if ((*scene)->destroy) {
            (*scene)->destroy(*scene);
        }
        free(*scene);
        *scene = NULL;
    }

    switch (type) {
        case SCENE_TITLE:
            newScene = createTitleScene();
            break;
        case SCENE_PREGAME_DIALOGUE:
            newScene = createPregameDialogueScene();
            break;
        case SCENE_GAME:
            newScene = createGameScene();
            break;
        case SCENE_GAMEOVER:
            newScene = createGameoverScene();
            break;
        case SCENE_POSTGAME_DIALOGUE:
            newScene = createPostgameDialogueScene();
            break;
        case SCENE_GAME_COMPLETE:
            newScene = createGameCompleteScene();
            break;
        default:
            rc = -1;
            goto error;
    }

    if (!newScene) {
        rc = -2;  // Different error code for allocation failure
        goto error;
    }

    // Initialize the scene
    if (newScene->init) {
        newScene->init(newScene);
    }

    *scene = newScene;
    return 0;

error:
    if (newScene) {
        if (newScene->destroy) {
            newScene->destroy(newScene);
        }
        free(newScene);
    }
    *scene = NULL;
    return rc;
}

static bool isPointInRect(int x, int y, int rectX, int rectY, int rectW, int rectH) {
    return x >= rectX && x < (rectX + rectW) && 
           y >= rectY && y < (rectY + rectH);
}

static void handleTouchTransitions(const touchPosition* touch) {
    for (int i = 0; i < numTouchTransitions; i++) {
        if (isPointInRect(touch->px, touch->py, 
                         touchTransitions[i].x, touchTransitions[i].y,
                         touchTransitions[i].width, touchTransitions[i].height)) {
            if (touchTransitions[i].useFade) {
                changeScene(touchTransitions[i].targetScene);
            } else {
                changeSceneImmediate(touchTransitions[i].targetScene);
            }
            break;
        }
    }
}

void handleSceneInput(const InputState* input) {
    // Handle error screen input
    if (!currentScene) {
        if (input->kDown & KEY_START) {
            requestExit();
        }
        return;
    }

    // Only handle input if not fading
    if (fadeState == FADE_NONE) {
        // Handle scene-specific input
        if (currentScene->handleInput) {
            currentScene->handleInput(currentScene, input);
        }

        // Handle touch transitions if touch is just pressed
        if (input->kDown & KEY_TOUCH) {
            handleTouchTransitions(&input->touch);
        }
    }
}

void updateCurrentScene(float deltaTime) {
    // Update fade
    if (fadeState != FADE_NONE) {
        fadeTimer += deltaTime;
        
        if (fadeState == FADE_OUT) {
            fadeAlpha = fadeTimer / FADE_DURATION;
            if (fadeAlpha >= MAX_ALPHA) {
                fadeAlpha = MAX_ALPHA;
                
                // Complete fade out before scene switch
                if (!nextScene) {
                    // Create and initialize the next scene
                    Result rc = createNewScene(pendingSceneType, &nextScene);
                    lastCreationError = rc;  // Store error for debugging
                    
                    if (R_FAILED(rc) || !nextScene) {
                        // If scene creation fails, move to error state
                        fadeState = FADE_ERROR;
                        fadeAlpha = MAX_ALPHA;  // Keep screen black
                        return;
                    }
                }
                
                // Next scene is ready, perform the switch
                if (currentScene) {
                    if (currentScene->destroy) {
                        currentScene->destroy(currentScene);
                    }
                    free(currentScene);
                    currentScene = NULL;
                }
                
                if (nextScene) {
                    currentScene = nextScene;
                    currentSceneType = pendingSceneType;
                    nextScene = NULL;
                    fadeState = FADE_IN;
                    fadeTimer = 0.0f;
                    clearTouchTransitions();
                } else {
                    fadeState = FADE_NONE;
                    fadeAlpha = 0.0f;
                    fadeTimer = 0.0f;
                }
            }
        } else if (fadeState == FADE_IN) {
            fadeAlpha = MAX_ALPHA - (fadeTimer / FADE_DURATION);
            if (fadeAlpha <= 0.0f) {
                fadeAlpha = 0.0f;
                fadeState = FADE_NONE;
            }
        }
    }

    // Update current scene
    if (currentScene && currentScene->update) {
        currentScene->update(currentScene, deltaTime);
    }
}

void drawCurrentScene(const GraphicsContext* context) {
    if (!currentScene) {
        // Draw error screen
        if (context->top) {
            C2D_SceneBegin(context->top);
            C2D_TargetClear(context->top, C2D_Color32(0, 0, 0, 255));
            drawTextWithFlags(
                SCREEN_WIDTH / 2,
                SCREEN_HEIGHT / 2,
                0.5f, 0.5f, 0.5f,
                C2D_Color32(255, 0, 0, 255),
                C2D_AlignCenter,
                "ERROR: Scene creation failed"
            );
        }
        if (context->bottom) {
            C2D_SceneBegin(context->bottom);
            C2D_TargetClear(context->bottom, C2D_Color32(0, 0, 0, 255));
            drawTextWithFlags(
                SCREEN_WIDTH_BOTTOM / 2,
                SCREEN_HEIGHT_BOTTOM / 2,
                0.5f, 0.5f, 0.5f,
                C2D_Color32(255, 0, 0, 255),
                C2D_AlignCenter,
                "Press START to exit"
            );
        }
        return;
    }

    if (currentScene->draw) {
        // Draw scene first
        currentScene->draw(currentScene, context);
        
        // Then draw fade overlay on top if fading
        if (fadeState != FADE_NONE) {
            u32 fadeColor = C2D_Color32f(0.0f, 0.0f, 0.0f, fadeAlpha);
            
            // Draw fade overlay on both screens
            if (context->top) {
                C2D_SceneBegin(context->top);
                C2D_DrawRectSolid(0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, fadeColor);
            }
            if (context->bottom) {
                C2D_SceneBegin(context->bottom);
                C2D_DrawRectSolid(0, 0, 0, SCREEN_WIDTH_BOTTOM, SCREEN_HEIGHT_BOTTOM, fadeColor);
            }

#ifdef SCENE_DEBUG
            // Draw debug text for fade state
            if (context->top) {
                C2D_SceneBegin(context->top);
                // Draw fade progress
                char fadeText[64];
                if (fadeState == FADE_OUT) {
                    snprintf(fadeText, sizeof(fadeText), "Fade Out: %.0f%%", (fadeAlpha / MAX_ALPHA) * 100);
                } else if (fadeState == FADE_IN) {
                    snprintf(fadeText, sizeof(fadeText), "Fade In: %.0f%%", (1.0f - fadeAlpha / MAX_ALPHA) * 100);
                }
                drawTextWithFlags(
                    10, 10, 0.5f, 0.5f, 0.5f,
                    C2D_Color32(255, 255, 0, 255),
                    C2D_AlignLeft,
                    fadeText
                );

                // Draw current scene info
                char sceneText[64];
                snprintf(sceneText, sizeof(sceneText), "Current: %s", getSceneTypeName(currentSceneType));
                drawTextWithFlags(
                    10, 30, 0.5f, 0.5f, 0.5f,
                    C2D_Color32(255, 255, 0, 255),
                    C2D_AlignLeft,
                    sceneText
                );

                // Draw next scene info if there is a pending scene
                if (fadeState == FADE_OUT || nextScene != NULL) {
                    snprintf(sceneText, sizeof(sceneText), "Next: %s", getSceneTypeName(pendingSceneType));
                    drawTextWithFlags(
                        10, 50, 0.5f, 0.5f, 0.5f,
                        C2D_Color32(255, 255, 0, 255),
                        C2D_AlignLeft,
                        sceneText
                    );

                    // Additional debug info for transition state
                    // Debug info for scene transition state
                    char stateText[128];
                    snprintf(stateText, sizeof(stateText), "nextScene: %s, fadeTimer: %.2f",
                            nextScene ? "exists" : "null",
                            fadeTimer);
                    drawTextWithFlags(
                        10, 70, 0.5f, 0.5f, 0.5f,
                        C2D_Color32(255, 255, 0, 255),
                        C2D_AlignLeft,
                        stateText
                    );

                    // Debug info for scene creation status
                    char createText[128];
                    if (!nextScene && fadeAlpha >= MAX_ALPHA) {
                        snprintf(createText, sizeof(createText), "Attempting scene creation: %s",
                                getSceneTypeName(pendingSceneType));
                    } else if (nextScene) {
                        snprintf(createText, sizeof(createText), "Scene created, waiting for switch");
                    }
                    drawTextWithFlags(
                        10, 90, 0.5f, 0.5f, 0.5f,
                        C2D_Color32(255, 255, 0, 255),
                        C2D_AlignLeft,
                        createText
                    );

                    // Debug info for fade state machine
                    char fadeStateText[128];
                    const char* stateStr =
                        fadeState == FADE_OUT ? "FADE_OUT" :
                        fadeState == FADE_IN ? "FADE_IN" :
                        fadeState == FADE_ERROR ? "FADE_ERROR" : "NONE";
                    
                    if (fadeState == FADE_ERROR) {
                        snprintf(fadeStateText, sizeof(fadeStateText),
                            "State: %s, Error: %08lX",
                            stateStr, lastCreationError);
                    } else {
                        snprintf(fadeStateText, sizeof(fadeStateText),
                            "State: %s, Alpha: %.2f, Timer: %.2f",
                            stateStr, fadeAlpha, fadeTimer);
                    }
                    drawTextWithFlags(
                        10, 110, 0.5f, 0.5f, 0.5f,
                        C2D_Color32(255, 255, 0, 255),
                        C2D_AlignLeft,
                        fadeStateText
                    );
                }
            }
#endif // SCENE_DEBUG
        }
    }
}

void registerTouchTransition(TouchTransition transition) {
    if (numTouchTransitions < MAX_TOUCH_TRANSITIONS) {
        touchTransitions[numTouchTransitions++] = transition;
    }
}

void clearTouchTransitions(void) {
    numTouchTransitions = 0;
}

bool isRequestingExit(void) {
    return requestedExit;
}

void requestExit(void) {
    requestedExit = true;
}

FadeState getCurrentFadeState(void) {
    return fadeState;
}