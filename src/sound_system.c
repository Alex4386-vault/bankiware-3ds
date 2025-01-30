#include "include/sound_system.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SAMPLERATE 22050
#define CHANNELS 2
#define BYTESPERSAMPLE 2
#define MAX_AUDIO_SIZE (1024 * 1024 * 4) // 4MB max audio size

static ndspWaveBuf waveBuf0, waveBuf1;  // One for each channel
static u32* audioBuffer = NULL;
static u32* audioBuffer2 = NULL;  // Buffer for second channel
static u32* queuedBuffer = NULL;  // Buffer for queued audio
static bool soundInitialized = false;
static bool hasQueuedAudio = false;
static size_t queuedSamples = 0;
static size_t queuedSize = 0;
// Forward declarations
static void setupChannel(int channel);
static Result loadWavFile(const char* filename, u32* buffer, size_t* outRead, u16* outBitsPerSample, u16* outNumChannels, u32 startSample, u32 numSamples);

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

    // Create and initialize audio buffers
    audioBuffer = (u32*)linearAlloc(MAX_AUDIO_SIZE);
    audioBuffer2 = (u32*)linearAlloc(MAX_AUDIO_SIZE);
    queuedBuffer = (u32*)linearAlloc(MAX_AUDIO_SIZE);
    if (!audioBuffer || !audioBuffer2 || !queuedBuffer) {
        printf("Failed to allocate audio buffers\n");
        if (audioBuffer) linearFree(audioBuffer);
        if (audioBuffer2) linearFree(audioBuffer2);
        if (queuedBuffer) linearFree(queuedBuffer);
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

    soundInitialized = true;
    hasQueuedAudio = false;
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

    // Load the audio file immediately into queued buffer
    size_t read;
    u16 bits_per_sample, num_channels;
    Result rc = loadWavFile(filename, queuedBuffer, &read, &bits_per_sample, &num_channels, startSample, numSamples);
    if (R_FAILED(rc)) return rc;

    queuedSamples = read / (bits_per_sample >> 3) / num_channels;
    queuedSize = read;
    hasQueuedAudio = true;

    // Pre-flush the cache for the queued buffer
    DSP_FlushDataCache(queuedBuffer, read);

    printf("Queued %lu samples from offset %lu\n", 
           (unsigned long)queuedSamples,
           (unsigned long)startSample);
    return 0;
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

    // Clear queued audio if stopping channel 0
    if (channel == 0) {
        hasQueuedAudio = false;
    }
}

void stopAudio(void) {
    if (!soundInitialized) return;
    
    stopAudioChannel(0);
    stopAudioChannel(1);
}

void soundUpdate(void) {
    if (!soundInitialized || !hasQueuedAudio) return;

    // Check if current audio is near its end or finished
    if (waveBuf0.status == NDSP_WBUF_DONE || !ndspChnIsPlaying(0)) {
        // Play the queued audio immediately using the queued buffer
        waveBuf0.data_vaddr = queuedBuffer;
        waveBuf0.nsamples = queuedSamples;
        waveBuf0.looping = false;
        waveBuf0.status = NDSP_WBUF_FREE;
        ndspChnWaveBufAdd(0, &waveBuf0);
        
        hasQueuedAudio = false;
        printf("Playing queued audio: %lu samples\n", (unsigned long)queuedSamples);
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
    if (queuedBuffer) {
        linearFree(queuedBuffer);
        queuedBuffer = NULL;
    }

    ndspExit();
    soundInitialized = false;
}