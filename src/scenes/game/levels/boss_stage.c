#include "../game_levels.h"
#include "../../../include/texture_loader.h"
#include "../../../include/sound_system.h"
#include "../../../include/text_renderer.h"
#include "../../common.h"
#include "level_common.h"
#include <stdlib.h>

#define CHARACTER_WIDTH 64.0f
#define CHARACTER_HEIGHT 64.0f

// for better controllability
#define OBSTACLE_WIDTH 64.0f
#define OBSTACLE_HEIGHT 64.0f
#define OBSTACLE_COLLISION_WIDTH 48.0f
#define OBSTACLE_COLLISION_HEIGHT 48.0f
#define BODY_WIDTH 64.0f
#define BODY_HEIGHT 64.0f

#define GRAVITY 0.25f
#define JUMP_FORCE -4.0f
#define OBSTACLE_SPEED 4.0f

typedef struct Obstacle {
    float x;
    float y;
    float rotation;
    int spriteIndex;
    struct Obstacle* next;
} Obstacle;

typedef struct BossStageData {
    bool initialized;
    bool gameOver;
    bool gameDecided;
    bool success;
    float bgOffset;
    
    // Character
    float characterX;
    float characterY;
    float velocityY;
    bool isJumping;
    bool isHeadingUp;
    
    // Game state
    int timer;
    Obstacle* obstacles;
    bool bodySpawned;
    float bodyX;
    float bodyY;
    bool failureTriggered;
    float gameOverTimer;
} BossStageData;

static void stopLongAudio() {
    // Due to wave being VERY long, we need to stop it manually
    if (ndspChnIsPlaying(0)) {
        ndspChnReset(0);
    }

    if (ndspChnIsPlaying(1)) {
        ndspChnReset(1);
    }

    // Stop audio playback on both channels
    stopAudio();
}

static bool checkCollision(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
    return (x1 < x2 + w2 &&
            x1 + w1 > x2 &&
            y1 < y2 + h2 &&
            y1 + h1 > y2);
}

static void freeObstacles(Obstacle* head) {
    if (head == NULL) return;
    
    Obstacle* current = head;
    while (current != NULL) {
        Obstacle* next = current->next;
        free(current);
        current = next;
    }
}

static void bossStageReset(BossStageData* levelData) {
    if (levelData == NULL) return;

    levelData->gameOver = false;
    levelData->gameDecided = false;
    levelData->success = false;
    levelData->bgOffset = 0.0f;
    
    levelData->characterX = SCREEN_WIDTH - CHARACTER_WIDTH - 50.0f;
    levelData->characterY = SCREEN_HEIGHT / 2;
    levelData->velocityY = 0.0f;
    levelData->isJumping = false;
    levelData->isHeadingUp = false;
    
    // Only free obstacles if they exist
    if (levelData->obstacles != NULL) {
        freeObstacles(levelData->obstacles);
    }
    levelData->timer = 24;
    levelData->obstacles = NULL;
    levelData->bodySpawned = false;
    levelData->bodyX = -300.0f;
    levelData->bodyY = SCREEN_HEIGHT - BODY_HEIGHT;
    levelData->failureTriggered = false;

    levelData->gameOverTimer = -1.0f;
}

static void bossStageSetup(GameSceneData* data) {
    BossStageData* levelData = (BossStageData*)data->currentLevelData;
    if (levelData == NULL) return;

    levelData->timer = 24;
    levelData->obstacles = NULL;
    levelData->bodySpawned = false;
    levelData->bodyX = -300.0f;
    levelData->bodyY = SCREEN_HEIGHT - BODY_HEIGHT;
    levelData->failureTriggered = false;
}

static void bossStageInit(GameSceneData* data) {
    BossStageData* levelData = malloc(sizeof(BossStageData));
    if (levelData == NULL) {
        panicEverything("Failed to allocate memory for BossStageData");
        return;
    }
    
    // Initialize all fields to 0/NULL
    memset(levelData, 0, sizeof(BossStageData));
    levelData->initialized = true;

    // Setup
    bossStageSetup(data);
    
    // Set initial position
    levelData->characterX = SCREEN_WIDTH - CHARACTER_WIDTH - 50.0f;
    levelData->characterY = SCREEN_HEIGHT / 2;
    data->currentLevelData = levelData;
    data->gameLeftTime = data->gameSessionTime;
    bossStageReset(levelData);

    playWavFromRomfsRange("romfs:/sounds/bgm_bossgame2.wav", 0.0f, SECONDS_TO_SAMPLES(25.0f));
}

static bool checkObstacleOverlap(Obstacle* obstacles, float x, float y) {
    Obstacle* current = obstacles;
    
    int collisionDiffX = (OBSTACLE_WIDTH - OBSTACLE_COLLISION_WIDTH) / 2;
    int collisionDiffY = (OBSTACLE_HEIGHT - OBSTACLE_COLLISION_HEIGHT) / 2;

    while (current != NULL) {
        if (checkCollision(x, y, OBSTACLE_WIDTH, OBSTACLE_HEIGHT,
                         current->x + collisionDiffX, current->y + collisionDiffY, OBSTACLE_COLLISION_WIDTH, OBSTACLE_COLLISION_HEIGHT)) {
            return true;
        }
        current = current->next;
    }
    return false;
}

static void addObstacle(BossStageData* levelData) {
    Obstacle* newObstacle = malloc(sizeof(Obstacle));
    if (newObstacle == NULL) return;
    
    newObstacle->x = -300.0f;
    
    // Define three fixed positions with enough space for player to pass through
    const float positions[] = {
        8.0f,                     // Top position
        88.0f, // Center position
        168.0f     // Bottom position
    };
    
    // Try each position in random order
    bool positionFound = false;
    int availablePositions[] = {0, 1, 2};  // Index of available positions
    int remainingPositions = 3;
    
    while (remainingPositions > 0 && !positionFound) {
        // Pick a random position from remaining ones
        int randomIndex = rand() % remainingPositions;
        float testY = positions[availablePositions[randomIndex]];
        
        // Check if this position is not taken
        if (!checkObstacleOverlap(levelData->obstacles, newObstacle->x, testY)) {
            newObstacle->y = testY;
            positionFound = true;
        }
        
        // Remove this position from available positions
        availablePositions[randomIndex] = availablePositions[remainingPositions - 1];
        remainingPositions--;
    }
    
    // If no position is available, use center position as fallback
    if (!positionFound) {
        newObstacle->y = positions[1];  // Center position
    }
    
    newObstacle->rotation = 0.0f;
    newObstacle->spriteIndex = rand() % 5; // Random sprite 0-4
    newObstacle->next = levelData->obstacles;
    levelData->obstacles = newObstacle;
}

static void bossStageGenerateObstacles(GameSceneData *data) {
    if (data == NULL) return;
    BossStageData* levelData = (BossStageData*)data->currentLevelData;
    if (levelData == NULL) return;

    if (levelData->timer == 120 || levelData->timer == 240 || levelData->timer == 360) {
        addObstacle(levelData);
    } else if (levelData->timer >= 480 && levelData->timer <= 960 && levelData->timer % 120 == 0) {
        addObstacle(levelData);
        addObstacle(levelData);
    } else if (levelData->timer == 1080 && !levelData->bodySpawned) {
        levelData->bodySpawned = true;
        levelData->bodyX = -300.0f;
    }    
}

static void bossStageUpdate(GameSceneData* data, float deltaTime) {
    BossStageData* levelData = (BossStageData*)data->currentLevelData;
    if (levelData == NULL) return;
    
    levelData->timer++;

    // Spawn obstacles based on timer
    if (!levelData->gameOver) {
        bossStageGenerateObstacles(data);
    }
    
    // Update background scroll
    levelData->bgOffset += SCROLL_SPEED;
    if (levelData->bgOffset >= SCREEN_WIDTH) {
        levelData->bgOffset = 0;
    }

    if (levelData->gameOverTimer > 0) {
        levelData->gameOverTimer -= deltaTime;
        if (levelData->gameOverTimer <= 0) {
            // due to how the implementation works, setting it as 0 won't exit the game.
            data->gameLeftTime = 0.1f;
        }
    }
    
    if (!levelData->gameOver) {
        // Update character physics
        if (!levelData->failureTriggered) {
            levelData->velocityY += GRAVITY;
            levelData->characterY += levelData->velocityY;
        }
        
        // Keep character in bounds
        if (levelData->characterY < 0) {
            levelData->characterY = 0;
            levelData->velocityY = 0;

            // play bounce sound
            playWavLayered("romfs:/sounds/se_boyon2.wav");
        }

        if (levelData->characterY > SCREEN_HEIGHT) {
            if (!levelData->failureTriggered) {
                levelData->failureTriggered = true;
                levelData->gameOver = true;
                levelData->gameDecided = true;
                levelData->success = false;
                data->lastGameState = GAME_FAILURE;
                levelData->gameOverTimer = 2.0f;

                stopLongAudio();
                playWavFromRomfs("romfs:/sounds/bgm_jingleBossFailed.wav");
            }
            return;
        }
        
        levelData->isHeadingUp = levelData->velocityY < 0;
        
        // Update obstacles
        Obstacle** current = &levelData->obstacles;
        while (*current != NULL) {
            (*current)->x += OBSTACLE_SPEED;
            
            // Update rotation
            (*current)->rotation += 0.1f;
            if ((*current)->rotation >= 2 * M_PI) {
                (*current)->rotation -= (2* M_PI);
            }
            
            // Check collision
            
            int collisionDiffX = (OBSTACLE_WIDTH - OBSTACLE_COLLISION_WIDTH) / 2;
            int collisionDiffY = (OBSTACLE_HEIGHT - OBSTACLE_COLLISION_HEIGHT) / 2;
            if (checkCollision(levelData->characterX + collisionDiffX, levelData->characterY + collisionDiffY,
                             OBSTACLE_COLLISION_WIDTH, OBSTACLE_COLLISION_HEIGHT,
                             (*current)->x + collisionDiffX, (*current)->y + collisionDiffY,
                             OBSTACLE_COLLISION_WIDTH, OBSTACLE_COLLISION_HEIGHT)) {
                if (!levelData->failureTriggered) {
                    levelData->failureTriggered = true;
                    levelData->gameOver = true;
                    levelData->gameDecided = true;
                    levelData->success = false;
                    levelData->gameOverTimer = 2.0f;
                    data->lastGameState = GAME_FAILURE;
                    stopLongAudio();
                    playWavFromRomfs("romfs:/sounds/bgm_jingleBossFailed.wav");
                }
                break;
            }
            
            // Remove off-screen obstacles
            if ((*current)->x > SCREEN_WIDTH) {
                Obstacle* temp = *current;
                *current = (*current)->next;
                free(temp);
            } else {
                current = &(*current)->next;
            }
        }
        
        // Update body position if spawned
        if (levelData->bodySpawned) {
            if (levelData->bodyX < SCREEN_WIDTH) {
                levelData->bodyX += OBSTACLE_SPEED;
            } else {
                // you missed the body.
                levelData->failureTriggered = true;
                levelData->gameOver = true;
                levelData->gameDecided = true;
                levelData->success = false;
                levelData->gameOverTimer = 2.0f;
                data->lastGameState = GAME_FAILURE;
                stopLongAudio();
                playWavFromRomfs("romfs:/sounds/bgm_jingleBossFailed.wav");
            }
            
            // Check for successful landing
            if (checkCollision(levelData->characterX, levelData->characterY,
                             CHARACTER_WIDTH, CHARACTER_HEIGHT,
                             levelData->bodyX, levelData->bodyY,
                             BODY_WIDTH, BODY_HEIGHT)) {
                levelData->gameOver = true;
                levelData->gameDecided = true;
                levelData->success = true;
                data->lastGameState = GAME_SUCCESS;
                data->gameLeftTime = 2.0f;
                levelData->gameOverTimer = 2.0f;

                stopLongAudio();
                playWavFromRomfs("romfs:/sounds/bgm_jingleBossClear.wav");
            }
        }
    }
}

static void bossStageDrawBackground(const GraphicsContext* context, const BossStageData* levelData) {
    C2D_DrawRectSolid(0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, C2D_Color32(0,127,255,255));
    displayImage("romfs:/textures/bg_sky2_0.t3x", -levelData->bgOffset, 0);
    displayImage("romfs:/textures/bg_sky2_0.t3x", SCREEN_WIDTH - levelData->bgOffset, 0);
    displayImage("romfs:/textures/bg_sky1_0.t3x", -2 * levelData->bgOffset, 0);
    displayImage("romfs:/textures/bg_sky1_0.t3x", SCREEN_WIDTH - 2 * levelData->bgOffset, 0);
}

static void bossStageDraw(GameSceneData* data, const GraphicsContext* context) {
    BossStageData* levelData = (BossStageData*)data->currentLevelData;
    if (levelData == NULL) return;

    // Draw top screen
    if (context->top) {
        C2D_SceneBegin(context->top);
        C2D_TargetClear(context->top, C2D_Color32(255, 255, 255, 255));
        
        // Draw background
        bossStageDrawBackground(context, levelData);
        
        // Draw obstacles
        for (Obstacle* current = levelData->obstacles; current != NULL; current = current->next) {
            char obstaclePath[64];
            snprintf(obstaclePath, sizeof(obstaclePath), "romfs:/textures/spr_m1_boss_enemy_%d.t3x", current->spriteIndex);
            displayImageWithScalingAndRotation(obstaclePath, current->x, current->y, NULL, 1.0f, 1.0f, current->rotation);
        }
        
        // Draw body if spawned
        if (levelData->bodySpawned) {
            if (levelData->success) {
                displayImage("romfs:/textures/spr_m1_boss_bankibody_1.t3x", levelData->bodyX, levelData->bodyY - BODY_HEIGHT + 30);
            } else {
                displayImage("romfs:/textures/spr_m1_boss_bankibody_0.t3x", levelData->bodyX, levelData->bodyY);
            }
        }
        
        // Draw character
        if (!levelData->success) {
            const char* characterSprite = levelData->failureTriggered ? 
                "romfs:/textures/spr_m1_boss_bankihead2_0.t3x" :
                (levelData->isHeadingUp ? 
                    "romfs:/textures/spr_m1_boss_bankihead_0.t3x" : 
                    "romfs:/textures/spr_m1_boss_bankihead_1.t3x");
            displayImage(characterSprite, levelData->characterX, levelData->characterY);
        }
    }
    
    // Draw bottom screen
    if (context->bottom) {
        C2D_SceneBegin(context->bottom);
        C2D_TargetClear(context->bottom, C2D_Color32(0, 0, 0, 255));
        
        // Draw touch instruction
        drawText(10, SCREEN_HEIGHT_BOTTOM - 50, 0.5f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255),
                "Touch or press A to jump!");

        // char text[128];
        // snprintf(text, sizeof(text), "bodyY: %.1f", levelData->bodyY);
        // drawText(10, 10, 0.5f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255), text);
    }
}

static void bossStageHandleInput(GameSceneData* data, const InputState* input) {
    BossStageData* levelData = (BossStageData*)data->currentLevelData;
    if (levelData == NULL || levelData->gameOver) return;
    
    // Handle jump input (A button or touch)
    if ((input->kDown & KEY_A) || (input->kDown & KEY_TOUCH)) {
        levelData->velocityY = JUMP_FORCE;
        levelData->isJumping = true;
        playWavLayered("romfs:/sounds/se_nyu2.wav");
    }
}


static void bossStageResetGame(GameSceneData* data) {
    BossStageData* levelData = (BossStageData*)data->currentLevelData;
    if (levelData) {
        bossStageReset(levelData);
    }
}

const GameLevel BossStageGame = {
    .init = bossStageInit,
    .update = bossStageUpdate,
    .draw = bossStageDraw,
    .handleInput = bossStageHandleInput,
    .reset = bossStageResetGame,
    .requestingQuit = NULL,
};