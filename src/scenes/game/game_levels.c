#include "game_levels.h"
#include "game_level_types.h"

// Array of game levels
static GameLevel* gameLevels[] = {
    (GameLevel*)&LaserBeamGame,   // Cast to GameLevel* to match array type
    (GameLevel*)&CatchMeGame,     // Cast to GameLevel* to match array type
    (GameLevel*)&DialogueSelectGame,  // Cast to GameLevel* to match array type
    (GameLevel*)&ExampleStubGame,  // Cast to GameLevel* to match array type
    NULL  // Sentinel value
};

GameLevel* getGameLevel(int levelNumber) {
    if (levelNumber < 0) return NULL;
    
    // Count number of available levels
    int maxLevels = 0;
    while (gameLevels[maxLevels] != NULL) maxLevels++;
    
    if (maxLevels == 0) {
        return NULL;  // No levels available
    }
    
    // Use modulo to cycle through available levels if level number exceeds available levels
    int index = levelNumber % maxLevels;
    
    GameLevel* level = gameLevels[index];
    if (level == NULL) {
        // Fallback to first level if something goes wrong
        return gameLevels[0];
    }
    
    return level;
}