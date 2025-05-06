#include <stdio.h>
#include <math.h>
#include "audio.h"

#define TWO_PI 6.283185307179586476925286766559

static F64 toBeats(F64 seconds, F64 bpm)
{
    return seconds * (bpm / 60.0);
}

static F64 toSeconds(F64 beats, F64 bpm)
{
    return beats * (60.0 / bpm);
}

typedef struct
{
    F64 start;    // Beats
    F64 duration; // Beats
    F64 frequency;
    F64 amplitude;
} Note;

#define MAX_TRACK_LENGTH 256

typedef struct Track
{
    F64 sampleRate;
    Note notes[MAX_TRACK_LENGTH];
    U32 noteCount;
    F64 bpm;
    F64 duration; // Beats
    U32 noteIndex;
    F64 position; // Beats
} Track;

static Track track;

static void callback(F32* buffer, U32 frameCount)
{
    for (U32 frame = 0; frame < frameCount; frame += 1)
    {
        F32 sample = 0.0f;
        if (track.position >= track.duration)
        {
            track.position = fmod(track.position, track.duration);
            track.noteIndex = 0;
        }
        // Skip the past notes.
        while (track.noteIndex < track.noteCount &&
               track.notes[track.noteIndex].start + track.notes[track.noteIndex].duration <
                   track.position)
        {
            track.noteIndex += 1;
        }
        if (track.noteIndex < track.noteCount)
        {
            Note note = track.notes[track.noteIndex];
            // Have we started yet?
            if (note.start < track.position)
            {
                F64 elapsed = toSeconds(track.position - note.start, track.bpm);
                F64 remaining = toSeconds(note.duration, track.bpm) - elapsed;
                F64 phase = fmod(elapsed * TWO_PI * note.frequency, TWO_PI);
                F64 noteSample = sin(phase) * note.amplitude;
                F64 fade = toSeconds(0.1, track.bpm);
                F64 envelope = 1.0;
                if (elapsed < fade)
                {
                    envelope = elapsed / fade;
                }
                else if (remaining < fade)
                {
                    envelope = remaining / fade;
                }
                sample += noteSample * envelope;
            }
        }
        // Left channel
        buffer[2 * frame + 0] = sample;
        // Right channel
        buffer[2 * frame + 1] = sample;
        track.position += toBeats(1.0 / track.sampleRate, track.bpm);
    }
}

int main(void)
{
    F64 sampleRate = 44100.0;
    track.notes[track.noteCount++] = (Note){
        .frequency = 440.0, // A4
        .start = 0.0,
        .duration = 1.0,
        .amplitude = 0.8,
    };
    track.notes[track.noteCount++] = (Note){
        .frequency = 261.63, // C4
        .start = 1.0,
        .duration = 0.5,
        .amplitude = 0.8,
    };
    track.notes[track.noteCount++] = (Note){
        .frequency = 261.63, // C4
        .start = 1.5,
        .duration = 0.5,
        .amplitude = 0.8,
    };
    track.notes[track.noteCount++] = (Note){
        .frequency = 261.63 * 2, // C5
        .start = 2.5,
        .duration = 0.5,
        .amplitude = 0.8,
    };
    track.bpm = 90.0;
    track.duration = 4.0;
    track.sampleRate = sampleRate;
    audioInit((AudioConfig){
        .callback = callback,
        .sampleRate = sampleRate,
    });
    audioStart();
    getchar();
    audioStop();
    audioDeinit();
    return 0;
}
