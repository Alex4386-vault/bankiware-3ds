#include "../scene.h"
#include "game_scene.h"
#include "game_level_types.h"
#include <stdbool.h>

#ifndef GAME_LEVELS_H
#define GAME_LEVELS_H

// Level declarations
extern const GameLevel SelectOneGame;
extern const GameLevel EatingCakeGame;
extern const GameLevel LaserBeamGame;
extern const GameLevel ExampleStubGame;
extern const GameLevel DialogueSelectGame;
extern const GameLevel CatchMeGame;
extern const GameLevel PizzaSlicingGame;
extern const GameLevel SearchLightGame;
extern const GameLevel BounceCatchGame;
extern const GameLevel CounterGame;
extern const GameLevel BossStageGame;

// Level management functions
GameLevel* getGameLevel(int levelNumber);

#endif // GAME_LEVELS_H