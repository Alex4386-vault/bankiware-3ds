#include "../scene.h"
#include "game_scene.h"
#include "game_level_types.h"
#include <stdbool.h>

#ifndef GAME_LEVELS_H
#define GAME_LEVELS_H

// Level declarations
extern const GameLevel LaserBeamGame;
extern const GameLevel ExampleStubGame;
extern const GameLevel DialogueSelectGame;
extern const GameLevel CatchMeGame;
extern const GameLevel PizzaSlicingGame;

// Level management functions
GameLevel* getGameLevel(int levelNumber);

#endif // GAME_LEVELS_H