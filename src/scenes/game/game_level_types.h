#ifndef GAME_LEVEL_TYPES_H
#define GAME_LEVEL_TYPES_H

#include <stdbool.h>
#include <3ds.h>
#include "common.h"
#include "game_scene.h"

// Game level definition
typedef struct {
    void (*init)(GameSceneData* data);
    void (*update)(GameSceneData* data, float deltaTime);
    void (*draw)(GameSceneData* data, const GraphicsContext* context);
    void (*handleInput)(GameSceneData* data, const InputState* input);
    void (*reset)(GameSceneData* data);  // Reset handler for game level
    bool (*requestingQuit)(GameSceneData* data);  // Check if level requests immediate quit
} GameLevel;

#endif // GAME_LEVEL_TYPES_H