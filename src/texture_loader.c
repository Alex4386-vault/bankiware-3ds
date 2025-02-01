#include "include/texture_loader.h"
#include "include/text_renderer.h"
#include <citro2d.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// Initialize global texture store
TextureStore g_textureStore = {0};

Result initGraphics(GraphicsContext* context) {
    if (!context) {
        printf("Invalid context pointer\n");
        return -1;
    }

    // Initialize graphics
    gfxInitDefault();
    printf("Initializing graphics...\n");
    
    // Initialize romfs
    Result rc = romfsInit();
    if (R_FAILED(rc)) {
        printf("romfsInit failed: %08lX\n", rc);
        return rc;
    }
    
    // Initialize citro3d
    if (R_FAILED(rc = C3D_Init(C3D_DEFAULT_CMDBUF_SIZE))) {
        printf("C3D_Init failed: %08lX\n", rc);
        romfsExit();
        return rc;
    }
    
    // Initialize citro2d
    if (R_FAILED(rc = C2D_Init(C2D_DEFAULT_MAX_OBJECTS))) {
        printf("C2D_Init failed: %08lX\n", rc);
        C3D_Fini();
        romfsExit();
        return rc;
    }
    
    C2D_Prepare();
    
    // Create screens
    context->top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    context->bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // Initialize texture store
    if (R_FAILED(rc = initTextureStore())) {
        printf("Failed to initialize texture store: %08lX\n", rc);
        C2D_Fini();
        C3D_Fini();
        romfsExit();
        return rc;
    }

    printf("Graphics initialization complete\n");
    return 0;
}

void exitGraphics(GraphicsContext* context) {
    if (!context) return;
    
    freeTextureStore();
    C2D_Fini();
    C3D_Fini();
    romfsExit();
    gfxExit();
}

Result initTextureStore(void) {
    memset(&g_textureStore, 0, sizeof(TextureStore));
    return 0;
}

Result loadTextureToStore(const char* name, const char* path) {
    if (!name || !path) {
        printf("Invalid parameters for loadTextureToStore\n");
        return -1;
    }

    if (g_textureStore.count >= MAX_TEXTURES) {
        printf("Texture store is full\n");
        return -2;
    }

    // Check if texture with same name already exists
    for (int i = 0; i < g_textureStore.count; i++) {
        if (strcmp(g_textureStore.names[i], name) == 0) {
            printf("Texture with name '%s' already exists\n", name);
            return -3;
        }
    }

    // Load the texture
    GameTexture* tex = &g_textureStore.textures[g_textureStore.count];
    Result rc = loadTextureFromFile(path, tex);
    if (R_FAILED(rc)) {
        return rc;
    }

    // Store the name
    strncpy(g_textureStore.names[g_textureStore.count], name, MAX_TEXTURE_NAME - 1);
    g_textureStore.names[g_textureStore.count][MAX_TEXTURE_NAME - 1] = '\0';
    g_textureStore.count++;

    printf("Added texture '%s' to store (total: %d)\n", name, g_textureStore.count);
    return 0;
}

GameTexture* getTextureFromStore(const char* name) {
    if (!name) return NULL;

    for (int i = 0; i < g_textureStore.count; i++) {
        if (strcmp(g_textureStore.names[i], name) == 0) {
            GameTexture* tex = &g_textureStore.textures[i];
            updateTextureTimestamp(tex);
            return tex;
        }
    }

    printf("Texture '%s' not found in store\n", name);
    return NULL;
}

void freeTextureStore(void) {
    for (int i = 0; i < g_textureStore.count; i++) {
        freeTexture(&g_textureStore.textures[i]);
    }
    memset(&g_textureStore, 0, sizeof(TextureStore));
}
Result displayImageWithScalingAndRotation(const char* path, float x, float y, C2D_ImageTint *tint, float scaleX, float scaleY, float rotation) {
    if (!path) {
        printf("Invalid path for displayImage\n");
        return -1;
    }

    // Generate a name from the path for store lookup
    char name[MAX_TEXTURE_NAME];
    const char* lastSlash = strrchr(path, '/');
    if (lastSlash) {
        strncpy(name, lastSlash + 1, MAX_TEXTURE_NAME - 1);
    } else {
        strncpy(name, path, MAX_TEXTURE_NAME - 1);
    }
    name[MAX_TEXTURE_NAME - 1] = '\0';

    // Try to get the texture from store first
    GameTexture* tex = getTextureFromStore(name);
    
    // If texture not found in store, load it
    if (!tex) {
        Result rc = loadTextureToStore(name, path);
        if (R_FAILED(rc)) {
            printf("Failed to load texture: %08lX\n", rc);
            return rc;
        }
        tex = getTextureFromStore(name);
        if (!tex) {
            printf("Failed to retrieve loaded texture\n");
            return -2;
        }
    }

    // Set up subtexture (height and width swapped due to 3DS screen orientation)
    Tex3DS_SubTexture subtex = {
        .width = tex->height,
        .height = tex->width,
        .left = 0.0f,
        .top = 0.0f,
        .right = 1.0f,
        .bottom = 1.0f
    };

    // Create image for rendering
    C2D_Image image = {
        .tex = &tex->texture,
        .subtex = &subtex
    };

    // Draw the image with proper scaling
    if (isnan(rotation)) {
        C2D_DrawImageAt(image, x, y, 0.0f, tint, scaleX, scaleY);
    } else {
        float offsetX = (tex->height * scaleX) / 2.0f;
        float offsetY = (tex->width * scaleY) / 2.0f;
        C2D_DrawImageAtRotated(image, x + offsetX, y + offsetY, 0.0f, rotation, tint, scaleX, scaleY);
    }

    return 0;
}

Result displayImageWithScaling(const char* path, float x, float y, C2D_ImageTint *tint, float scaleX, float scaleY) {
    return displayImageWithScalingAndRotation(path, x, y, tint, scaleX, scaleY, NAN);
}

Result displayImage(const char* path, float x, float y) {
    return displayImageWithScaling(path, x, y, NULL, 1.0f, 1.0f);
}

Result displayTiledImage(const char* path, float x, float y, float width, float height, float offsetX, float offsetY) {
    if (!path) {
        printf("Invalid path for displayTiledImage\n");
        return -1;
    }

    // Generate a name from the path for store lookup
    char name[MAX_TEXTURE_NAME];
    const char* lastSlash = strrchr(path, '/');
    if (lastSlash) {
        strncpy(name, lastSlash + 1, MAX_TEXTURE_NAME - 1);
    } else {
        strncpy(name, path, MAX_TEXTURE_NAME - 1);
    }
    name[MAX_TEXTURE_NAME - 1] = '\0';

    // Try to get the texture from store first
    GameTexture* tex = getTextureFromStore(name);
    
    // If texture not found in store, load it
    if (!tex) {
        Result rc = loadTextureToStore(name, path);
        if (R_FAILED(rc)) {
            printf("Failed to load texture: %08lX\n", rc);
            return rc;
        }
        tex = getTextureFromStore(name);
        if (!tex) {
            printf("Failed to retrieve loaded texture\n");
            return -2;
        }
    }

    // Set up subtexture for tiling
    Tex3DS_SubTexture subtex = {
        .width = tex->width,
        .height = tex->height,
        .left = 0.0f,
        .top = 0.0f,
        .right = 1.0f,
        .bottom = 1.0f
    };

    // Create image for rendering
    C2D_Image image = {
        .tex = &tex->texture,
        .subtex = &subtex
    };

    // Calculate number of tiles needed to cover the area plus extra tiles for seamless scrolling
    int tilesX = (int)ceil(width / (float)tex->width) + 2;  // Add extra columns
    int tilesY = (int)ceil(height / (float)tex->height) + 2; // Add extra rows

    // Draw the tiled pattern with slight overlap to prevent gaps
    const float overlap = 0.5f;  // Half-pixel overlap
    
    // Start drawing one tile before the view area
    float startX = x - tex->width + offsetX;
    float startY = y - tex->height + offsetY;
    
    for (int ty = 0; ty < tilesY; ty++) {
        for (int tx = 0; tx < tilesX; tx++) {
            float tileX = startX + (tx * tex->width) - (tx > 0 ? overlap : 0);
            float tileY = startY + (ty * tex->height) - (ty > 0 ? overlap : 0);
            C2D_DrawImageAt(image, tileX, tileY, 0.0f, NULL, 1.0f, 1.0f);
        }
    }

    return 0;
}

void updateTextureTimestamp(GameTexture* tex) {
    if (tex) {
        tex->last_used = osGetTime();
    }
}

Result cleanupUnusedTextures(u64 older_than) {
    u64 current_time = osGetTime();
    int cleaned = 0;

    for (int i = 0; i < g_textureStore.count; i++) {
        if ((current_time - g_textureStore.textures[i].last_used) > older_than) {
            freeTexture(&g_textureStore.textures[i]);
            
            // Move remaining textures up
            for (int j = i; j < g_textureStore.count - 1; j++) {
                g_textureStore.textures[j] = g_textureStore.textures[j + 1];
                strncpy(g_textureStore.names[j], g_textureStore.names[j + 1], MAX_TEXTURE_NAME - 1);
                g_textureStore.names[j][MAX_TEXTURE_NAME - 1] = '\0';
            }
            
            g_textureStore.count--;
            i--; // Recheck this index since we moved a texture here
            cleaned++;
        }
    }

    printf("Cleaned up %d unused textures\n", cleaned);
    return 0;
}

Result loadTextureFromFile(const char* path, GameTexture* tex) {
    if (!path || !tex) {
        printf("Invalid parameters\n");
        return -1;
    }

    FILE* file = fopen(path, "rb");
    if (!file) {
        printf("Failed to open file: %s\n", path);
        return -2;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    if (size == 0) {
        printf("File is empty: %s\n", path);
        fclose(file);
        return -3;
    }

    printf("File size: %zu bytes\n", size);

    // Read entire file to buffer
    void* buffer = malloc(size);
    if (!buffer) {
        fclose(file);
        return -4;
    }
    
    size_t read = fread(buffer, 1, size, file);
    fclose(file);

    if (read != size) {
        printf("Failed to read entire file: %zu of %zu bytes\n", read, size);
        free(buffer);
        return -5;
    }

    // Import the texture directly
    C3D_Tex* texture = &tex->texture;
    memset(texture, 0, sizeof(C3D_Tex));

    printf("Importing texture data...\n");
    Tex3DS_Texture t3x = Tex3DS_TextureImport(buffer, size, texture, NULL, false);
    free(buffer);

    if (!t3x) {
        printf("Failed to import texture\n");
        return -6;
    }

    // Get the first subtexture info
    const Tex3DS_SubTexture* subtex = Tex3DS_GetSubTexture(t3x, 0);
    if (!subtex) {
        printf("Failed to get subtexture info\n");
        Tex3DS_TextureFree(t3x);
        return -7;
    }

    // Store dimensions and initialize timestamp
    tex->width = texture->width;
    tex->height = texture->height;
    updateTextureTimestamp(tex);

    printf("Raw texture dimensions: %dx%d\n", texture->width, texture->height);
    printf("Power-of-2 check: width %s, height %s\n",
           (tex->width & (tex->width - 1)) == 0 ? "yes" : "no",
           (tex->height & (tex->height - 1)) == 0 ? "yes" : "no");

    // Validate dimensions
    if (tex->width == 0 || tex->height == 0 || 
        tex->width > 1024 || tex->height > 1024) {
        printf("Invalid texture dimensions\n");
        Tex3DS_TextureFree(t3x);
        return -8;
    }

    // Set texture parameters for tiling - use nearest neighbor filtering to prevent bleeding
    C3D_TexSetFilter(texture, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(texture, GPU_REPEAT, GPU_REPEAT);

    // Free the t3x when we're done
    Tex3DS_TextureFree(t3x);

    printf("Successfully loaded texture: %ux%u pixels\n", tex->width, tex->height);
    return 0;
}

void freeTexture(GameTexture* tex) {
    if (tex) {
        C3D_TexDelete(&tex->texture);
        memset(tex, 0, sizeof(GameTexture));
    }
}