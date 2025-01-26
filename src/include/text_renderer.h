#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <citro2d.h>
#include <stdbool.h>

// Initialize text rendering system
Result initTextRenderer(void);

// Clean up text rendering system
void exitTextRenderer(void);

// Draw text at specified position
void drawText(float x, float y, float z, float scaleX, float scaleY, u32 color, const char* text);

// Draw text with specified flags
void drawTextWithFlags(float x, float y, float z, float scaleX, float scaleY, u32 color, u32 flags, const char* text);

// Draw debug info
void drawDebugInfo(float x, float y, float scaleX, float scaleY, 
                  int screenWidth, int screenHeight,
                  float offsetX, float offsetY,
                  bool touchActive, bool isDragging);

#endif // TEXT_RENDERER_H