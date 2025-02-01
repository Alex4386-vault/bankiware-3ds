#ifndef GAME_SCENE_H
#define GAME_SCENE_H

#include "../scene.h"

#define GAME_TIMER_HEIGHT 64.0f

typedef enum BankiState {
    BANKI_IDLE,
    BANKI_EXCITED,
    BANKI_SAD,
} BankiState;

typedef enum GameState {
    GAME_UNDEFINED,
    GAME_SUCCESS,
    GAME_FAILURE,
} GameState;

typedef enum BounceState {
    BOUNCE_NONE,
    BOUNCE_BANKI,
    BOUNCE_ALL,
} BounceState;

// Game scene specific data
typedef struct GameSceneData {
    bool isComplete;
    float elapsedTime;

    int currentLevel;
    int remainingLife;
    bool isInGame;

    float offsetX;
    float offsetY;
    float scrollSpeed;

    BankiState bankiState;
    GameState lastGameState;

    float elapsedTimeSinceStageScreen;

    BounceState bounceState;
    float bounceTimer;
    float bounceAnimationTimer;
    float bounceAnimationInterval;

    bool isDebug;
    float gameLeftTime;
    float gameSessionTime;
    float shouldIncreaseLevelAt;
    float shouldEnterGameAt;

    float showSpeedUpAt;
    float showSpeedUpTimer;

    void *currentLevelData;
    void *currentLevelObj;
} GameSceneData;

// Create a new game scene
Scene* createGameScene(void);

#endif // GAME_SCENE_H