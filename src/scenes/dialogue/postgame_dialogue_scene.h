#ifndef POSTGAME_DIALOGUE_SCENE_H
#define POSTGAME_DIALOGUE_SCENE_H

#include "../scene.h"

// Postgame dialogue scene specific data
typedef struct {
    bool isComplete;
    bool shouldExit;
    int currentIdx;
    float stage0Offset;

    float elapsedTime;
    float elapsedTimeSinceLast;
} PostgameDialogueData;

// Create a new postgame dialogue scene
Scene* createPostgameDialogueScene();

#endif // POSTGAME_DIALOGUE_SCENE_H