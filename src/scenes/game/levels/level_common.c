#include "../game_levels.h"
#include "../../../include/texture_loader.h"
#include "../../../include/sound_system.h"
#include "../../../include/text_renderer.h"
#include "../../common.h"
#include <stdlib.h>

void levelCommonDraw(GameSceneData* data, float screenWidth, float screenHeight, float offsetX, float offsetY) {
    // draw a background
    char targetFile[64];
    int fileId = 2;
    if (data->lastGameState == GAME_UNDEFINED) {
      fileId = 2;
    } else if (data->lastGameState == GAME_SUCCESS) {
      fileId = 3;
    } else if (data->lastGameState == GAME_FAILURE) {
      fileId = 4;
    }
    snprintf(targetFile, sizeof(targetFile), "romfs:/textures/bg_%d_0.t3x", fileId);
    
    displayTiledImage(targetFile, 0, 0, screenWidth, screenHeight, offsetX, offsetY);
}