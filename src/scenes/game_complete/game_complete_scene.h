#ifndef GAME_COMPLETE_SCENE_H
#define GAME_COMPLETE_SCENE_H

#include "../scene.h"

// Game complete scene specific data
typedef struct {
    bool isComplete;
    float elapsedTime;

    float offsetX;
    float offsetY;
} GameCompleteSceneData;

// Create a new game complete scene
Scene* createGameCompleteScene(void);

#endif // GAME_COMPLETE_SCENE_H