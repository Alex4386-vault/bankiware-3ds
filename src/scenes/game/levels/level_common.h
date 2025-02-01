#include "../game_scene.h"

#ifndef LEVEL_COMMON_H
#define LEVEL_COMMON_H

// Updates background scroll offsets, wrapping at the given texture size
#define UPDATE_BACKGROUND_SCROLL(levelData, textureSize, scrollSpeed) do { \
    levelData->offsetX += (scrollSpeed); \
    levelData->offsetY += (scrollSpeed); \
    if (levelData->offsetX <= -(textureSize)) levelData->offsetX = 0.0f; \
    if (levelData->offsetY <= -(textureSize)) levelData->offsetY = 0.0f; \
    if (levelData->offsetX >= (textureSize)) levelData->offsetX = 0.0f; \
    if (levelData->offsetY >= (textureSize)) levelData->offsetY = 0.0f; \
} while(0)

void levelCommonDraw(GameSceneData* data, float screenWidth, float screenHeight, float offsetX, float offsetY);

#endif // LEVEL_COMMON_H
