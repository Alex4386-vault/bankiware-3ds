#ifndef SCENE_H
#define SCENE_H

#include "common.h"
#include "../include/texture_loader.h"

typedef struct Scene Scene;

struct Scene {
    void (*init)(Scene* scene);
    void (*update)(Scene* scene, float deltaTime);
    void (*draw)(Scene* scene, const GraphicsContext* context);
    void (*handleInput)(Scene* scene, const InputState* input);
    void (*destroy)(Scene* scene);
    void* data;  // Scene-specific data
};

#endif // SCENE_H