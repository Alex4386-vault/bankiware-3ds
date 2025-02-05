#include "include/sound_system.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SAMPLERATE 22050
#define CHANNELS 2
#define BYTESPERSAMPLE 2

static ndspWaveBuf waveBuf0, waveBuf1;  // One for each channel

// Audio queue management
static QueuedAudio audioQueue[MAX_QUEUED_AUDIO];
static u32* currentAudioBuffer = NULL;   // Current audio buffer
static u32* secondaryAudioBuffer = NULL; // Secondary audio buffer
static int queueHead = 0;  // Index of next audio to play
static int queueTail = 0;  // Index where next audio will be added
static int queueCount = 0; // Number of queued items

static bool soundInitialized = false;

// Helper functions for queue management
static bool isQueueEmpty(void) {
    return queueCount == 0;
}

static bool isQueueFull(void) {
    return queueCount >= MAX_QUEUED_AUDIO;
}

static void enqueueAudio(u32* buffer, size_t samples, size_t bufferSize, size_t dataSize) {
    if (isQueueFull()) {
        linearFree(buffer);
        return;
    }
    
    // Set up queue entry with the provided buffer
    audioQueue[queueTail].buffer = buffer;
    audioQueue[queueTail].samples = samples;
    audioQueue[queueTail].size = dataSize;
    audioQueue[queueTail].bufferSize = bufferSize;
    
    queueTail = (queueTail + 1) % MAX_QUEUED_AUDIO;
    queueCount++;
}

static QueuedAudio* peekNextAudio(void) {
    if (isQueueEmpty()) return NULL;
    return &audioQueue[queueHead];
}
static void dequeueAudio(void) {
    if (isQueueEmpty()) return;
    
    // Free the buffer from the dequeued audio
    if (audioQueue[queueHead].buffer) {
        linearFree(audioQueue[queueHead].buffer);
        audioQueue[queueHead].buffer = NULL;
    }
    
    queueHead = (queueHead + 1) % MAX_QUEUED_AUDIO;
    queueCount--;
}

// Forward declarations
static void setupChannel(int channel);
static Result loadWavFile(const char* filename, u32* buffer, size_t* outRead, u16* outBitsPerSample, u16* outNumChannels, u32 startSample, u32 numSamples);

static bool shouldUseDirectPlayback(const char* filename) {
    return isQueueFull();  // Only check if queue is full
}

static void ensureChannelReady(void) {
    if (!ndspChnIsPlaying(0)) {
        // Allocate a small buffer for silence
        u32* silenceBuffer = (u32*)linearAlloc(1024);
        if (!silenceBuffer) return;

        // Initialize with silence to ensure proper state
        memset(silenceBuffer, 0, 1024);
        waveBuf0.data_vaddr = silenceBuffer;
        waveBuf0.nsamples = 256;
        waveBuf0.looping = false;
        waveBuf0.status = NDSP_WBUF_DONE;
        DSP_FlushDataCache(silenceBuffer, 1024);
        ndspChnWaveBufAdd(0, &waveBuf0);

        // Free the silence buffer after adding to wave buf
        linearFree(silenceBuffer);
    }
}

static void setupChannel(int channel) {
    float mix[12];
    memset(mix, 0, sizeof(mix));
    mix[0] = mix[1] = 1.0f; // Both speakers at full volume
    
    ndspChnReset(channel);
    ndspChnSetInterp(channel, NDSP_INTERP_LINEAR);
    ndspChnSetRate(channel, SAMPLERATE);
    ndspChnSetFormat(channel, NDSP_FORMAT_STEREO_PCM16);
    ndspChnSetMix(channel, mix);
    ndspChnSetMix(0, mix);
}

Result soundInit(void) {
    Result ret = 0;

    // Initialize audio system
    ret = ndspInit();
    if (R_FAILED(ret)) {
        printf("ndspInit failed: %08lX\n", ret);
        return ret;
    }

    // Setup NDSP
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspSetOutputCount(2);  // Using 2 channels
    ndspSetClippingMode(NDSP_CLIP_SOFT); // Prevent audio clipping

    // Setup both channels
    setupChannel(0);
    setupChannel(1);

    // Initialize wave buffers
    memset(&waveBuf0, 0, sizeof(ndspWaveBuf));
    memset(&waveBuf1, 0, sizeof(ndspWaveBuf));
    
    waveBuf0.status = NDSP_WBUF_FREE;
    waveBuf1.status = NDSP_WBUF_FREE;

    // Initialize queue state
    queueHead = 0;
    queueTail = 0;
    queueCount = 0;
    currentAudioBuffer = NULL;
    secondaryAudioBuffer = NULL;
    
    soundInitialized = true;
    return 0;
}

static Result getWavFileSize(const char* filename, size_t* outSize) {
    FILE* file = fopen(filename, "rb");
    if (!file) return -2;

    fseek(file, 0, SEEK_END);
    *outSize = ftell(file);
    fclose(file);
    return 0;
}

static Result loadWavFile(const char* filename, u32* buffer, size_t* outRead, u16* outBitsPerSample, u16* outNumChannels, u32 startSample, u32 numSamples) {
    FILE* file = fopen(filename, "rb");
    if (!file) return -2;

    // Read WAV header
    u32 magic, size, fmt, subchunk1id, subchunk1size;
    u16 audio_format, num_channels;
    u32 sample_rate, byte_rate;
    u16 block_align, bits_per_sample;

    fread(&magic, 4, 1, file);
    fread(&size, 4, 1, file);
    fread(&fmt, 4, 1, file);
    fread(&subchunk1id, 4, 1, file);
    fread(&subchunk1size, 4, 1, file);
    fread(&audio_format, 2, 1, file);
    fread(&num_channels, 2, 1, file);
    fread(&sample_rate, 4, 1, file);
    fread(&byte_rate, 4, 1, file);
    fread(&block_align, 2, 1, file);
    fread(&bits_per_sample, 2, 1, file);

    // Skip any extra params in subchunk1
    if (subchunk1size > 16) {
        fseek(file, subchunk1size - 16, SEEK_CUR);
    }

    // Find data chunk
    u32 chunk_id, chunk_size;
    while (true) {
        if (fread(&chunk_id, 4, 1, file) != 1 ||
            fread(&chunk_size, 4, 1, file) != 1) {
            fclose(file);
            return -3;
        }
        if (chunk_id == 0x61746164) break; // "data"
        fseek(file, chunk_size, SEEK_CUR);
    }

    printf("WAV: %u Hz, %u channels, %u bits\n",
           (unsigned int)sample_rate,
           (unsigned int)num_channels,
           (unsigned int)bits_per_sample);

    // Calculate byte positions
    size_t bytesPerSample = (bits_per_sample >> 3) * num_channels;
    size_t startByte = startSample * bytesPerSample;
    size_t readSize = numSamples > 0 ? numSamples * bytesPerSample : chunk_size;

    // Validate range
    if (startByte >= chunk_size) {
        fclose(file);
        printf("Start sample out of range\n");
        return -6;
    }

    // Adjust read size if it would exceed the file
    if (startByte + readSize > chunk_size) {
        readSize = chunk_size - startByte;
    }

    // Seek to start position
    fseek(file, startByte, SEEK_CUR);

    // Read audio data
    size_t read = fread(buffer, 1, readSize, file);
    fclose(file);

    if (read <= 0) {
        printf("Failed to read audio data\n");
        return -4;
    }

    *outRead = read;
    *outBitsPerSample = bits_per_sample;
    *outNumChannels = num_channels;
    return 0;
}

Result playWavFromRomfs(const char* filename) {
    printf("Attempting to play: %s\n", filename);
    return playWavFromRomfsRange(filename, 0, 0);  // 0 numSamples means play entire file
}

Result playWavFromRomfsRangeWithSpeed(const char* filename, u32 startSample, u32 numSamples, float speedMultiplier) {
    if (!soundInitialized) return -1;
    if (speedMultiplier <= 0.0f) return -1;

    // Get file size first
    size_t fileSize;
    Result rc = getWavFileSize(filename, &fileSize);
    if (R_FAILED(rc)) return rc;

    // Free any existing buffer
    if (currentAudioBuffer) {
        linearFree(currentAudioBuffer);
        currentAudioBuffer = NULL;
    }

    // Allocate new buffer
    currentAudioBuffer = (u32*)linearAlloc(fileSize);
    if (!currentAudioBuffer) return -1;

    // Only stop if something is actually playing
    if (ndspChnIsPlaying(0)) {
        waveBuf0.status = NDSP_WBUF_DONE;
        ndspChnWaveBufClear(0);
    }

    size_t read;
    u16 bits_per_sample, num_channels;
    rc = loadWavFile(filename, currentAudioBuffer, &read, &bits_per_sample, &num_channels, startSample, numSamples);
    if (R_FAILED(rc)) {
        linearFree(currentAudioBuffer);
        currentAudioBuffer = NULL;
        return rc;
    }

    // Setup and play audio with speed adjustment
    waveBuf0.data_vaddr = currentAudioBuffer;
    waveBuf0.nsamples = read / (bits_per_sample >> 3) / num_channels;
    waveBuf0.looping = false;
    waveBuf0.status = NDSP_WBUF_FREE;
    
    // Apply speed multiplier by adjusting the playback rate
    ndspChnSetRate(0, SAMPLERATE * speedMultiplier);
    
    DSP_FlushDataCache(currentAudioBuffer, read);
    ndspChnWaveBufAdd(0, &waveBuf0);

    return 0;
}

Result playWavFromRomfsRange(const char* filename, u32 startSample, u32 numSamples) {
    return playWavFromRomfsRangeWithSpeed(filename, startSample, numSamples, 1.0f);
}

Result queueWavFromRomfs(const char* filename) {
    return queueWavFromRomfsRange(filename, 0, 0);  // 0 numSamples means play entire file
}

Result queueWavFromRomfsRange(const char* filename, u32 startSample, u32 numSamples) {
    if (!soundInitialized) return -1;

    // Check if we should use direct playback
    if (shouldUseDirectPlayback(filename)) {
        return playWavFromRomfsRange(filename, startSample, numSamples);
    }

    // Get file size first
    size_t fileSize;
    Result rc = getWavFileSize(filename, &fileSize);
    if (R_FAILED(rc)) return rc;

    // Allocate buffer for the audio
    u32* buffer = (u32*)linearAlloc(fileSize);
    if (!buffer) {
        return playWavFromRomfsRange(filename, startSample, numSamples);
    }

    size_t read;
    u16 bits_per_sample, num_channels;
    rc = loadWavFile(filename, buffer, &read, &bits_per_sample, &num_channels, startSample, numSamples);
    
    if (R_SUCCEEDED(rc)) {
        size_t samples = read / (bits_per_sample >> 3) / num_channels;
        enqueueAudio(buffer, samples, fileSize, read);
    } else {
        linearFree(buffer);
    }

    return rc;
}

Result playWavFromRomfsLoop(const char* filename) {
    if (!soundInitialized) return -1;

    // Get file size first
    size_t fileSize;
    Result rc = getWavFileSize(filename, &fileSize);
    if (R_FAILED(rc)) return rc;

    // Free any existing buffer
    if (currentAudioBuffer) {
        linearFree(currentAudioBuffer);
        currentAudioBuffer = NULL;
    }

    // Allocate new buffer
    currentAudioBuffer = (u32*)linearAlloc(fileSize);
    if (!currentAudioBuffer) return -1;

    // Only stop if something is actually playing
    if (ndspChnIsPlaying(0)) {
        waveBuf0.status = NDSP_WBUF_DONE;
        ndspChnWaveBufClear(0);
    }

    size_t read;
    u16 bits_per_sample, num_channels;
    rc = loadWavFile(filename, currentAudioBuffer, &read, &bits_per_sample, &num_channels, 0, 0);
    if (R_FAILED(rc)) {
        linearFree(currentAudioBuffer);
        currentAudioBuffer = NULL;
        return rc;
    }

    // Setup and play audio with looping enabled
    waveBuf0.data_vaddr = currentAudioBuffer;
    waveBuf0.nsamples = read / (bits_per_sample >> 3) / num_channels;
    waveBuf0.looping = true;
    waveBuf0.status = NDSP_WBUF_FREE;
    DSP_FlushDataCache(currentAudioBuffer, read);
    ndspChnWaveBufAdd(0, &waveBuf0);

    return 0;
}

void stopAudioChannel(int channel) {
    if (!soundInitialized || channel < 0 || channel > 1) return;

    // Clear and wait for channel to finish
    if (ndspChnIsPlaying(channel)) {
        if (channel == 0) waveBuf0.status = NDSP_WBUF_DONE;
        else if (channel == 1) waveBuf1.status = NDSP_WBUF_DONE;
        ndspChnWaveBufClear(channel);
    }
    
    // Reset channel
    setupChannel(channel);
    
    // Reset appropriate wave buffer
    ndspWaveBuf* wb = (channel == 0) ? &waveBuf0 : &waveBuf1;
    memset(wb, 0, sizeof(ndspWaveBuf));
    wb->status = NDSP_WBUF_FREE;

    // Clear queue if stopping channel 0
    if (channel == 0) {
        // Clear all queue entries to prevent any pending audio from playing
        for (int i = 0; i < MAX_QUEUED_AUDIO; i++) {
            memset(&audioQueue[i], 0, sizeof(QueuedAudio));
        }
        queueHead = 0;
        queueTail = 0;
        queueCount = 0;
    }
}

void stopAudio(void) {
    if (!soundInitialized) return;
    
    stopAudioChannel(0);
    stopAudioChannel(1);
}

void soundUpdate(void) {
    if (!soundInitialized) {
        printf("Sound not initialized\n");
        return;
    }

    if (isQueueEmpty()) {
        return;
    }

    // Verify queue entry is valid before playing
    QueuedAudio* nextAudio = peekNextAudio();
    if (!nextAudio || !nextAudio->buffer || nextAudio->samples == 0) {
        // Invalid queue entry, clear it and return
        if (nextAudio) dequeueAudio();
        return;
    }

    // If nothing is playing, start playing from queue immediately
    if (!ndspChnIsPlaying(0)) {
        // Play the next queued audio
        waveBuf0.data_vaddr = nextAudio->buffer;
        waveBuf0.nsamples = nextAudio->samples;
        waveBuf0.looping = false;
        waveBuf0.status = NDSP_WBUF_FREE;
        DSP_FlushDataCache(nextAudio->buffer, nextAudio->size);
        ndspChnWaveBufAdd(0, &waveBuf0);
        
        printf("Starting queued audio: %lu samples (queue count: %d)\n",
               (unsigned long)nextAudio->samples, queueCount);
        dequeueAudio();
        return;
    }

    // Check if current audio is finished
    if (waveBuf0.status == NDSP_WBUF_DONE) {
        // Verify next queue entry again as it might have changed
        nextAudio = peekNextAudio();
        if (!nextAudio || !nextAudio->buffer || nextAudio->samples == 0) {
            if (nextAudio) dequeueAudio();
            return;
        }

        // Play the next queued audio
        waveBuf0.data_vaddr = nextAudio->buffer;
        waveBuf0.nsamples = nextAudio->samples;
        waveBuf0.looping = false;
        waveBuf0.status = NDSP_WBUF_FREE;
        DSP_FlushDataCache(nextAudio->buffer, nextAudio->size);
        ndspChnWaveBufAdd(0, &waveBuf0);
        
        printf("Playing next queued audio: %lu samples (queue count: %d)\n",
               (unsigned long)nextAudio->samples, queueCount);
        dequeueAudio();
    }
}

Result playWavLayered(const char* filename) {
    if (!soundInitialized) return -1;

    // Stop previous layered sound and clean up
    stopAudioChannel(1);

    // Get file size first
    size_t fileSize;
    Result rc = getWavFileSize(filename, &fileSize);
    if (R_FAILED(rc)) return rc;

    // Free any existing secondary buffer
    if (secondaryAudioBuffer) {
        linearFree(secondaryAudioBuffer);
        secondaryAudioBuffer = NULL;
    }

    // Allocate new buffer
    secondaryAudioBuffer = (u32*)linearAlloc(fileSize);
    if (!secondaryAudioBuffer) return -1;

    size_t read;
    u16 bits_per_sample, num_channels;
    rc = loadWavFile(filename, secondaryAudioBuffer, &read, &bits_per_sample, &num_channels, 0, 0);
    if (R_FAILED(rc)) {
        linearFree(secondaryAudioBuffer);
        secondaryAudioBuffer = NULL;
        return rc;
    }

    // Setup and play audio on channel 1
    waveBuf1.data_vaddr = secondaryAudioBuffer;
    waveBuf1.nsamples = read / (bits_per_sample >> 3) / num_channels;
    waveBuf1.looping = false;
    waveBuf1.status = NDSP_WBUF_FREE;
    DSP_FlushDataCache(secondaryAudioBuffer, read);
    ndspChnWaveBufAdd(1, &waveBuf1);

    return 0;
}

void soundExit(void) {
    if (!soundInitialized) return;

    // Stop audio playback on both channels
    stopAudio();
    ndspChnWaveBufClear(1);

    // Free resources
    if (currentAudioBuffer) {
        linearFree(currentAudioBuffer);
        currentAudioBuffer = NULL;
    }
    if (secondaryAudioBuffer) {
        linearFree(secondaryAudioBuffer);
        secondaryAudioBuffer = NULL;
    }
    
    // Free queue buffers
    for (int i = 0; i < MAX_QUEUED_AUDIO; i++) {
        if (audioQueue[i].buffer) {
            linearFree(audioQueue[i].buffer);
            audioQueue[i].buffer = NULL;
        }
    }
    
    // Reset queue state
    queueHead = 0;
    queueTail = 0;
    queueCount = 0;

    ndspExit();
    soundInitialized = false;
}