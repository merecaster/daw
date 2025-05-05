#include <stdio.h>
#include <math.h>
#include "audio.h"

typedef struct SineWave
{
    F64 phase;
    F64 frequency;
    F64 sampleRate;
    F64 amplitude;
} SineWave;

SineWave wave;

void sineWave(F32* buffer, U32 frameCount)
{
    F64 phaseIncrement = 2.0 * M_PI * wave.frequency / wave.sampleRate;
    for (U32 frame = 0; frame < frameCount; frame += 1)
    {
        F32 sine = sin(wave.phase);
        F32 sample = sine * wave.amplitude;
        // Left channel
        buffer[2 * frame + 0] = sample;
        // Right channel
        buffer[2 * frame + 1] = sample;
        wave.phase += phaseIncrement;
        if (wave.phase >= 2.0 * M_PI)
        {
            wave.phase -= 2.0 * M_PI;
        }
    }
}

int main(void)
{
    F64 sampleRate = 44100.0;
    wave.phase = 0.0f;
    wave.frequency = 440.0; // A4
    wave.sampleRate = sampleRate;
    wave.amplitude = 0.8;
    audioInit((AudioConfig){
        .callback = sineWave,
        .sampleRate = sampleRate,
    });
    audioStart();
    getchar();
    audioStop();
    audioDeinit();
    return 0;
}
