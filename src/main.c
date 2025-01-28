#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>
#include "scenes/scene_manager.h"
#include "include/texture_loader.h"
#include "include/sound_system.h"
#include "include/text_renderer.h"

int main(int argc, char* argv[]) {
    // Initialize random
    srand(time(NULL));

    // Initialize graphics
    GraphicsContext context = {0};
    Result rc = initGraphics(&context);
    if (R_FAILED(rc)) {
        printf("Failed to initialize graphics: %08lX\n", rc);
        return rc;
    }

    // Initialize text renderer
    rc = initTextRenderer();
    if (R_FAILED(rc)) {
        printf("Failed to initialize text renderer: %08lX\n", rc);
        exitGraphics(&context);
        return rc;
    }

    // Initialize sound system
    rc = soundInit();
    if (R_FAILED(rc)) {
        printf("Failed to initialize sound system: %08lX\n", rc);
        exitTextRenderer();
        exitGraphics(&context);
        return rc;
    }

    rc = initSceneManager();
    if (R_FAILED(rc)) {
        printf("Failed to initialize scene manager: %08lX\n", rc);
        soundExit();
        exitTextRenderer();
        exitGraphics(&context);
        return rc;
    }

    // For tracking frame time
    float deltaTime = 1.0f / 60.0f;

    // Main loop
    while (aptMainLoop()) {
        // Scan all input
        hidScanInput();
        
        // Create input state
        InputState input = {
            .kDown = hidKeysDown(),
            .kHeld = hidKeysHeld(),
            .kUp = hidKeysUp()
        };
        
        // Get touch position if touched
        if (input.kDown & KEY_TOUCH || input.kHeld & KEY_TOUCH) {
            hidTouchRead(&input.touch);
        }

        // Handle input for current scene
        handleSceneInput(&input);

        // Update current scene
        updateCurrentScene(deltaTime);

        // Update sound system to check for queued audio
        soundUpdate();

        // Start frame
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // Draw current scene
        drawCurrentScene(&context);

        if (isRequestingExit()) {
            break;
        }

        // End frame
        C3D_FrameEnd(0);
    }

    exitSceneManager();
    soundExit();
    exitTextRenderer();
    exitGraphics(&context);
    return 0;
}