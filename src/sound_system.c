#include "include/sound_system.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SAMPLERATE 22050
#define CHANNELS 2
#define BYTESPERSAMPLE 2
#define MAX_QUEUE_SIZE (1024 * 512) // 512KB max size for queued audio
#define MAX_AUDIO_SIZE (1024 * 1024 * 4) // 4MB max size for direct playback

static ndspWaveBuf waveBuf0, waveBuf1;  // One for each channel
static u32* audioBuffer = NULL;
static u32* audioBuffer2 = NULL;  // Buffer for second channel

// Audio queue management
static QueuedAudio audioQueue[MAX_QUEUED_AUDIO];
static u32* queueBuffers[MAX_QUEUED_AUDIO];  // Pre-allocated buffers for queued audio
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

static void enqueueAudio(const u32* buffer, size_t samples, size_t size) {
    if (isQueueFull()) {
        printf("Audio queue full, dropping audio (%lu samples)\n", (unsigned long)samples);
        return;
    }
    
    // Copy audio data to queue buffer
    memcpy(queueBuffers[queueTail], buffer, size);
    
    // Set up queue entry
    audioQueue[queueTail].buffer = queueBuffers[queueTail];
    audioQueue[queueTail].samples = samples;
    audioQueue[queueTail].size = size;
    
    queueTail = (queueTail + 1) % MAX_QUEUED_AUDIO;
    queueCount++;
    
    printf("Enqueued audio at position %d, count now %d (%lu samples)\n",
           (queueTail - 1 + MAX_QUEUED_AUDIO) % MAX_QUEUED_AUDIO,
           queueCount,
           (unsigned long)samples);
}

static QueuedAudio* peekNextAudio(void) {
    if (isQueueEmpty()) return NULL;
    return &audioQueue[queueHead];
}

static void dequeueAudio(void) {
    if (isQueueEmpty()) return;
    queueHead = (queueHead + 1) % MAX_QUEUED_AUDIO;
    queueCount--;
}
// Forward declarations
static void setupChannel(int channel);
static Result loadWavFile(const char* filename, u32* buffer, size_t* outRead, u16* outBitsPerSample, u16* outNumChannels, u32 startSample, u32 numSamples);

static bool shouldUseDirectPlayback(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) return true;  // If we can't check size, use direct playback to be safe
    
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fclose(file);
    
    return fileSize > MAX_QUEUE_SIZE || isQueueFull();
}

static void ensureChannelReady(void) {
    if (!ndspChnIsPlaying(0)) {
        // Initialize with silence to ensure proper state
        memset(audioBuffer, 0, 1024);
        waveBuf0.data_vaddr = audioBuffer;
        waveBuf0.nsamples = 256;
        waveBuf0.looping = false;
        waveBuf0.status = NDSP_WBUF_DONE;
        DSP_FlushDataCache(audioBuffer, 1024);
        ndspChnWaveBufAdd(0, &waveBuf0);
        printf("Channel initialized with silence\n");
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

    // Create and initialize main audio buffers (4MB for direct playback)
    audioBuffer = (u32*)linearAlloc(MAX_AUDIO_SIZE);
    audioBuffer2 = (u32*)linearAlloc(MAX_AUDIO_SIZE);
    
    // Initialize queue buffers (512KB each for jingles)
    for (int i = 0; i < MAX_QUEUED_AUDIO; i++) {
        queueBuffers[i] = (u32*)linearAlloc(MAX_QUEUE_SIZE);
        if (!queueBuffers[i]) {
            // Cleanup on failure
            printf("Failed to allocate queue buffer %d\n", i);
            if (audioBuffer) linearFree(audioBuffer);
            if (audioBuffer2) linearFree(audioBuffer2);
            for (int j = 0; j < i; j++) {
                linearFree(queueBuffers[j]);
            }
            ndspExit();
            return -1;
        }
    }

    if (!audioBuffer || !audioBuffer2) {
        printf("Failed to allocate main audio buffers\n");
        if (audioBuffer) linearFree(audioBuffer);
        if (audioBuffer2) linearFree(audioBuffer2);
        for (int i = 0; i < MAX_QUEUED_AUDIO; i++) {
            linearFree(queueBuffers[i]);
        }
        ndspExit();
        return -1;
    }

    // Setup NDSP
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspSetOutputCount(2);  // Using 2 channels
    ndspSetClippingMode(NDSP_CLIP_SOFT); // Prevent audio clipping

    // Setup both channels
    setupChannel(0);
    setupChannel(1);

    // Initialize wave buffers with proper validation
    memset(&waveBuf0, 0, sizeof(ndspWaveBuf));
    memset(&waveBuf1, 0, sizeof(ndspWaveBuf));
    
    if (audioBuffer) {
        waveBuf0.data_vaddr = audioBuffer;
        waveBuf0.status = NDSP_WBUF_FREE;
    }
    
    if (audioBuffer2) {
        waveBuf1.data_vaddr = audioBuffer2;
        waveBuf1.status = NDSP_WBUF_FREE;
    }

    // Initialize queue state
    queueHead = 0;
    queueTail = 0;
    queueCount = 0;
    
    soundInitialized = true;
    printf("Sound system initialized\n");
    return 0;
}

static Result loadWavFile(const char* filename, u32* buffer, size_t* outRead, u16* outBitsPerSample, u16* outNumChannels, u32 startSample, u32 numSamples) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Failed to open file: %s\n", filename);
        return -2;
    }

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

    // Check buffer size
    if (readSize > MAX_AUDIO_SIZE) {
        printf("Audio data too large: %lu bytes\n", (unsigned long)readSize);
        fclose(file);
        return -5;
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
    return playWavFromRomfsRange(filename, 0, 0);  // 0 numSamples means play entire file
}

Result playWavFromRomfsRange(const char* filename, u32 startSample, u32 numSamples) {
    if (!soundInitialized) return -1;

    // Only stop if something is actually playing
    if (ndspChnIsPlaying(0)) {
        ndspChnWaveBufClear(0);
        while (ndspChnIsPlaying(0)) {
            svcSleepThread(100000); // 0.1ms
        }
    }

    size_t read;
    u16 bits_per_sample, num_channels;
    Result rc = loadWavFile(filename, audioBuffer, &read, &bits_per_sample, &num_channels, startSample, numSamples);
    if (R_FAILED(rc)) return rc;

    // Setup and play audio with validation
    if (audioBuffer && read > 0) {
        waveBuf0.data_vaddr = audioBuffer;
        waveBuf0.nsamples = read / (bits_per_sample >> 3) / num_channels;
        waveBuf0.looping = false;
        waveBuf0.status = NDSP_WBUF_FREE;
        DSP_FlushDataCache(audioBuffer, read);
        ndspChnWaveBufAdd(0, &waveBuf0);
    }

    printf("Playing %lu samples from offset %lu\n", 
           (unsigned long)waveBuf0.nsamples, 
           (unsigned long)startSample);
    return 0;
}

Result queueWavFromRomfs(const char* filename) {
    return queueWavFromRomfsRange(filename, 0, 0);  // 0 numSamples means play entire file
}

Result queueWavFromRomfsRange(const char* filename, u32 startSample, u32 numSamples) {
    if (!soundInitialized) return -1;

    // Check if we should use direct playback
    if (shouldUseDirectPlayback(filename)) {
        printf("Using direct playback for %s\n", filename);
        return playWavFromRomfsRange(filename, startSample, numSamples);
    }

    // Try to queue the audio
    u32* tempBuffer = (u32*)linearAlloc(MAX_QUEUE_SIZE);
    if (!tempBuffer) {
        printf("Failed to allocate temp buffer, falling back to direct playback\n");
        return playWavFromRomfsRange(filename, startSample, numSamples);
    }

    size_t read;
    u16 bits_per_sample, num_channels;
    Result rc = loadWavFile(filename, tempBuffer, &read, &bits_per_sample, &num_channels, startSample, numSamples);
    
    if (R_SUCCEEDED(rc)) {
        size_t samples = read / (bits_per_sample >> 3) / num_channels;
        
        // Copy to queue buffer and enqueue
        memcpy(queueBuffers[queueTail], tempBuffer, read);
        enqueueAudio(queueBuffers[queueTail], samples, read);
        
        printf("Queued %s: %lu samples (queue count: %d)\n",
               filename, (unsigned long)samples, queueCount);
    } else {
        printf("Failed to load audio for queueing: %d\n", rc);
    }

    linearFree(tempBuffer);
    return rc;
}

Result playWavFromRomfsLoop(const char* filename) {
    if (!soundInitialized) return -1;

    // Only stop if something is actually playing
    if (ndspChnIsPlaying(0)) {
        ndspChnWaveBufClear(0);
        while (ndspChnIsPlaying(0)) {
            svcSleepThread(100000); // 0.1ms
        }
    }

    size_t read;
    u16 bits_per_sample, num_channels;
    Result rc = loadWavFile(filename, audioBuffer, &read, &bits_per_sample, &num_channels, 0, 0);
    if (R_FAILED(rc)) return rc;

    // Setup and play audio with looping enabled
    waveBuf0.data_vaddr = audioBuffer;
    waveBuf0.nsamples = read / (bits_per_sample >> 3) / num_channels;
    waveBuf0.looping = true;
    waveBuf0.status = NDSP_WBUF_FREE;
    DSP_FlushDataCache(audioBuffer, read);
    ndspChnWaveBufAdd(0, &waveBuf0);

    printf("Playing %lu samples (Looping)\n", (unsigned long)waveBuf0.nsamples);
    return 0;
}

void stopAudioChannel(int channel) {
    if (!soundInitialized || channel < 0 || channel > 1) return;

    // Clear and wait for channel to finish
    if (ndspChnIsPlaying(channel)) {
        ndspChnWaveBufClear(channel);
        while (ndspChnIsPlaying(channel)) {
            svcSleepThread(100000); // 0.1ms
        }
    }
    
    // Reset channel
    setupChannel(channel);
    
    // Reset appropriate wave buffer
    ndspWaveBuf* wb = (channel == 0) ? &waveBuf0 : &waveBuf1;
    memset(wb, 0, sizeof(ndspWaveBuf));
    wb->status = NDSP_WBUF_FREE;

    // Clear queue if stopping channel 0
    if (channel == 0) {
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

    // If nothing is playing, start playing from queue immediately
    if (!ndspChnIsPlaying(0)) {
        QueuedAudio* nextAudio = peekNextAudio();
        if (nextAudio) {
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
    }

    // Check if current audio is finished
    if (waveBuf0.status == NDSP_WBUF_DONE) {
        QueuedAudio* nextAudio = peekNextAudio();
        if (nextAudio) {
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
}

Result playWavLayered(const char* filename) {
    if (!soundInitialized) return -1;

    // Stop previous layered sound and clean up
    stopAudioChannel(1);

    size_t read;
    u16 bits_per_sample, num_channels;
    Result rc = loadWavFile(filename, audioBuffer2, &read, &bits_per_sample, &num_channels, 0, 0);
    if (R_FAILED(rc)) return rc;

    // Setup and play audio on channel 1
    if (audioBuffer2) {
        waveBuf1.data_vaddr = audioBuffer2;
        waveBuf1.nsamples = read / (bits_per_sample >> 3) / num_channels;
        waveBuf1.looping = false;
        waveBuf1.status = NDSP_WBUF_FREE;
        DSP_FlushDataCache(audioBuffer2, read);
        ndspChnWaveBufAdd(1, &waveBuf1);
    }

    printf("Playing layered sound: %lu samples\n", (unsigned long)waveBuf1.nsamples);
    return 0;
}

void soundExit(void) {
    if (!soundInitialized) return;

    // Stop audio playback on both channels
    stopAudio();
    ndspChnWaveBufClear(1);
    while (ndspChnIsPlaying(1)) {
        svcSleepThread(100000); // 0.1ms
    }

    // Free resources
    if (audioBuffer) {
        linearFree(audioBuffer);
        audioBuffer = NULL;
    }
    if (audioBuffer2) {
        linearFree(audioBuffer2);
        audioBuffer2 = NULL;
    }
    
    // Free queue buffers
    for (int i = 0; i < MAX_QUEUED_AUDIO; i++) {
        if (queueBuffers[i]) {
            linearFree(queueBuffers[i]);
            queueBuffers[i] = NULL;
        }
    }
    
    // Reset queue state
    queueHead = 0;
    queueTail = 0;
    queueCount = 0;

    ndspExit();
    soundInitialized = false;
}