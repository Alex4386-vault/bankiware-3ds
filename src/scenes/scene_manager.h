#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "scene.h"

typedef enum {
    SCENE_TITLE,
    SCENE_PREGAME_DIALOGUE,
    SCENE_GAME,
    SCENE_GAMEOVER,
    SCENE_COUNT
} SceneType;

typedef enum {
    FADE_NONE,
    FADE_OUT,
    FADE_IN,
    FADE_ERROR  // New state for scene creation failures
} FadeState;

// Touch area definition for scene transitions
typedef struct {
    int x;
    int y;
    int width;
    int height;
    SceneType targetScene;
    bool useFade;  // Whether to use fade transition
} TouchTransition;

Result initSceneManager(void);
void exitSceneManager(void);

// Scene transition functions
Result changeScene(SceneType type);  // With fade transition
Result changeSceneImmediate(SceneType type);  // Without fade transition

void updateCurrentScene(float deltaTime);
void handleSceneInput(const InputState* input);
void drawCurrentScene(const GraphicsContext* context);

// Touch transition management
void registerTouchTransition(TouchTransition transition);
void clearTouchTransitions(void);

bool isRequestingExit(void);
void requestExit(void);

#endif // SCENE_MANAGER_H