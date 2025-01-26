#include "include/text_renderer.h"
#include <stdio.h>
#include <string.h>

static C2D_TextBuf g_textBuf = NULL;
static bool g_initialized = false;

Result initTextRenderer(void) {
    if (g_initialized) return 0;

    g_textBuf = C2D_TextBufNew(4096);
    if (!g_textBuf) return -1;

    g_initialized = true;
    return 0;
}

void exitTextRenderer(void) {
    if (g_textBuf) {
        C2D_TextBufDelete(g_textBuf);
        g_textBuf = NULL;
    }
    g_initialized = false;
}

void drawText(float x, float y, float z, float scaleX, float scaleY, u32 color, const char* text) {
    drawTextWithFlags(x, y, z, scaleX, scaleY, color, C2D_AlignLeft, text);
}

void drawTextWithFlags(float x, float y, float z, float scaleX, float scaleY, u32 color, u32 flags, const char* text) {
    if (!g_initialized || !text) return;

    C2D_TextBufClear(g_textBuf);
    float currentY = y;
    const float lineHeight = 25.0f;
    char lineBuf[256];
    size_t textLen = strlen(text);
    size_t pos = 0;

    while (pos < textLen) {
        // Find end of line
        size_t lineEnd = pos;
        while (lineEnd < textLen && text[lineEnd] != '\n') {
            lineEnd++;
        }

        // Copy line to buffer
        size_t lineLen = lineEnd - pos;
        if (lineLen >= sizeof(lineBuf)) lineLen = sizeof(lineBuf) - 1;
        memcpy(lineBuf, text + pos, lineLen);
        lineBuf[lineLen] = '\0';

        // Draw line
        C2D_Text lineText;
        C2D_TextParse(&lineText, g_textBuf, lineBuf);
        C2D_TextOptimize(&lineText);
        C2D_DrawText(&lineText, flags | C2D_WithColor, x, currentY, z, scaleX, scaleY, color);

        // Move to next line
        currentY += lineHeight * scaleY;
        pos = lineEnd + 1;
        if (lineEnd >= textLen) break;
    }
}