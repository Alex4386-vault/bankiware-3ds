#ifndef GAMEOVER_SCENE_H
#define GAMEOVER_SCENE_H

#include "../scene.h"

// Gameover scene specific data
typedef struct {
    bool isComplete;
    float elapsedTime;
} GameoverSceneData;

// Create a new gameover scene
Scene* createGameoverScene(void);

#endif // GAMEOVER_SCENE_H