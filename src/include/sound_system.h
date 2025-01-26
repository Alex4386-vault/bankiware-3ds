#ifndef SOUND_SYSTEM_H
#define SOUND_SYSTEM_H

#include <3ds.h>

#define AUDIO_SAMPLERATE 22050
#define SECONDS_TO_SAMPLES(seconds) ((u32)(AUDIO_SAMPLERATE * (seconds)))

// Initialize sound system
Result soundInit(void);

// Load and play WAV file from romfs (immediately stops current audio)
Result playWavFromRomfs(const char* filename);

// Load and play WAV file from romfs with looping (immediately stops current audio)
Result playWavFromRomfsLoop(const char* filename);

// Load and play WAV file from romfs with offset and length control
Result playWavFromRomfsRange(const char* filename, u32 startSample, u32 numSamples);

// Queue WAV file to play after current audio finishes
Result queueWavFromRomfs(const char* filename);

// Queue WAV file with offset and length control
Result queueWavFromRomfsRange(const char* filename, u32 startSample, u32 numSamples);

// Update sound system (check for queued audio)
void soundUpdate(void);

// Stop currently playing audio
void stopAudio(void);

// Clean up sound system
void soundExit(void);

#endif // SOUND_SYSTEM_H