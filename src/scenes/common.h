#ifndef SCENES_COMMON_H
#define SCENES_COMMON_H

#include <3ds.h>
#include <citro2d.h>

// Screen dimensions
#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 240

// Screen dimensions for bottom
#define SCREEN_WIDTH_BOTTOM  320
#define SCREEN_HEIGHT_BOTTOM 240

// Animation constants
#define SCROLL_SPEED  0.5f

// Input handling structure
typedef struct {
    u32 kDown;     // Keys just pressed
    u32 kHeld;     // Keys currently held
    u32 kUp;       // Keys just released
    touchPosition touch;
} InputState;

#endif // SCENES_COMMON_H