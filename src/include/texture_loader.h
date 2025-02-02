#ifndef TEXTURE_LOADER_H
#define TEXTURE_LOADER_H

#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>
#include <stdio.h>

// Constants
#define MAX_TEXTURES 96
#define MAX_TEXTURE_NAME 64

typedef struct {
    C3D_Tex texture;
    u16 width;
    u16 height;
    u64 last_used;    // Timestamp of last usage
} GameTexture;

typedef struct {
    GameTexture textures[MAX_TEXTURES];
    char names[MAX_TEXTURES][MAX_TEXTURE_NAME];
    int count;
} TextureStore;

// Graphics context structure
typedef struct {
    C3D_RenderTarget* top;
    C3D_RenderTarget* bottom;
} GraphicsContext;

// Global texture store
extern TextureStore g_textureStore;

// Function declarations
Result initGraphics(GraphicsContext* context);
void exitGraphics(GraphicsContext* context);
Result initTextureStore(void);
Result loadTextureToStore(const char* name, const char* path);
GameTexture* getTextureFromStore(const char* name);
void freeTextureStore(void);

// Display functions
Result displayImage(const char* path, float x, float y);
Result displayTiledImage(const char* path, float x, float y, float width, float height, float offsetX, float offsetY);
Result displayTiledImageWithTint(const char* path, float x, float y, float width, float height, float offsetX, float offsetY, C2D_ImageTint *tint);
Result displayImageWithScaling(const char* path, float x, float y, C2D_ImageTint *tint, float scaleX, float scaleY);
Result displayImageWithScalingAndRotation(const char* path, float x, float y, C2D_ImageTint *tint, float scaleX, float scaleY, float rotation);

// Legacy functions for compatibility
Result loadTextureFromFile(const char* path, GameTexture* tex);
void freeTexture(GameTexture* tex);

// Texture management functions
Result cleanupUnusedTextures(u64 older_than);
void updateTextureTimestamp(GameTexture* tex);

#endif // TEXTURE_LOADER_H