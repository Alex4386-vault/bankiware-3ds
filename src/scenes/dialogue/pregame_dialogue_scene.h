#ifndef PREGAME_DIALOGUE_SCENE_H
#define PREGAME_DIALOGUE_SCENE_H

#include "../scene.h"

// Pregame dialogue scene specific data
typedef struct {
    bool isComplete;
    bool shouldExit;
    int currentIdx;
    float stage0Offset;

    float elapsedTime;
    float elapsedTimeSinceLast;
} PregameDialogueData;

// Create a new pregame dialogue scene
Scene* createPregameDialogueScene();

#endif // PREGAME_DIALOGUE_SCENE_H