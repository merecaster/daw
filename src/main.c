#include <stdio.h>
#include <math.h>
#include <string.h>
#include "assets.h"
#include "platform.h"

#define TWO_PI 6.283185307179586476925286766559

const F64 audioSampleRate = 44100.0;

static F64 noteFrequencies[128] = {
    8.175799,     8.661957,    9.177024,    9.722718,    10.300861,   10.913382,    11.562326,
    12.249857,    12.978272,   13.750000,   14.567618,   15.433853,   16.351598,    17.323914,
    18.354048,    19.445436,   20.601722,   21.826764,   23.124651,   24.499715,    25.956544,
    27.500000,    29.135235,   30.867706,   32.703196,   34.647829,   36.708096,    38.890873,
    41.203445,    43.653529,   46.249303,   48.999429,   51.913087,   55.000000,    58.270470,
    61.735413,    65.406391,   69.295658,   73.416192,   77.781746,   82.406889,    87.307058,
    92.498606,    97.998859,   103.826174,  110.000000,  116.540940,  123.470825,   130.812783,
    138.591315,   146.832384,  155.563492,  164.813778,  174.614116,  184.997211,   195.997718,
    207.652349,   220.000000,  233.081881,  246.941651,  261.625565,  277.182631,   293.664768,
    311.126984,   329.627557,  349.228231,  369.994423,  391.995436,  415.304698,   440.000000,
    466.163762,   493.883301,  523.251131,  554.365262,  587.329536,  622.253967,   659.255114,
    698.456463,   739.988845,  783.990872,  830.609395,  880.000000,  932.327523,   987.766603,
    1046.502261,  1108.730524, 1174.659072, 1244.507935, 1318.510228, 1396.912926,  1479.977691,
    1567.981744,  1661.218790, 1760.000000, 1864.655046, 1975.533205, 2093.004522,  2217.461048,
    2349.318143,  2489.015870, 2637.020455, 2793.825851, 2959.955382, 3135.963488,  3322.437581,
    3520.000000,  3729.310092, 3951.066410, 4186.009045, 4434.922096, 4698.636287,  4978.031740,
    5274.040911,  5587.651703, 5919.910763, 6271.926976, 6644.875161, 7040.000000,  7458.620184,
    7902.132820,  8372.018090, 8869.844191, 9397.272573, 9956.063479, 10548.081821, 11175.303406,
    11839.821527, 12543.853951};

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
    U8 number;    // MIDI note number in [0, 127]
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

static void audio(F32* buffer, U32 frameCount)
{
    // memset(buffer, 0, 2 * frameCount * sizeof(F32));
    // return;
    for (U32 frame = 0; frame < frameCount; frame += 1)
    {
        F64 position = atomicLoadF64(&track.position);
        F32 sample = 0.0f;
        // Skip the past notes.
        while (track.noteIndex < track.noteCount &&
               track.notes[track.noteIndex].start + track.notes[track.noteIndex].duration <
                   position)
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
                F64 phase = fmod(elapsed * TWO_PI * noteFrequencies[note.number], TWO_PI);
                F64 noteSample = sin(phase) * note.amplitude;
                F64 fade = toSeconds(0.002, track.bpm);
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
        position += toBeats(1.0 / track.sampleRate, track.bpm);
        if (position >= track.duration)
        {
            position = fmod(position, track.duration);
            track.noteIndex = 0;
        }
        atomicStoreF64(&track.position, position);
    }
}

void fillRect(Color32* pixels, U32 width, U32 height, U32 rx, U32 ry, U32 rw, U32 rh, Color32 color)
{
    for (U32 x = rx; x < rx + rw; x += 1)
    {
        for (U32 y = ry; y < ry + rh; y += 1)
        {
            U32 index = y * width + x;
            pixels[index] = color;
        }
    }
}

void drawText(Color32* pixels, U32 width, U32 height, U32 tx, U32 ty, char* text, Color32 color)
{
    U32 textureWidth = FONT_ALLOY_CURSES_12X12_PNG_WIDTH;
    U32 textureHeight = FONT_ALLOY_CURSES_12X12_PNG_HEIGHT;
    F32 w = textureWidth / 16;
    F32 h = textureHeight / 16;
    while (*text)
    {
        char ascii = *text++;
        U32 row = ascii / 16;
        U32 col = ascii % 16;
        U32 fx = col * w;
        U32 fy = row * h;
        for (U32 x = 0; x < w; x += 1)
        {
            for (U32 y = 0; y < h; y += 1)
            {
                U32 fIdx = (fy + y) * textureWidth + (fx + x);
                U32 tIdx = (ty + y) * width + (tx + x);
                U8 fontPixel = FONT_ALLOY_CURSES_12X12_PNG_BYTES[fIdx];
                if (fontPixel)
                {
                    pixels[tIdx] = color;
                }
            }
        }
        tx += w;
    }
}

static Color32 white = {0xFF, 0xFF, 0xFF, 0xFF};
static Color32 black = {0x00, 0x00, 0x00, 0xFF};
static Color32 noteColor = {0x77, 0xFF, 0x00, 0xFF};
static Color32 gridColor = {0x66, 0x66, 0x66, 0xFF};
static Color32 positionColor = {0xFF, 0x00, 0x00, 0xFF};

const U32 keyHeight = 24;
const U32 keyWidth = 64;

void drawOctave(Color32* pixels, U32 width, U32 height, U32 x, U32 y, char* cName)
{
    for (U32 i = 0; i < 7; i += 1)
    {
        fillRect(pixels, width, height, x, y + i * keyHeight + 1, keyWidth, keyHeight - 2, white);
    }
    for (U32 i = 0; i < 6; i += 1)
    {
        if (i != 3)
        {
            fillRect(
                pixels,
                width,
                height,
                x,
                y + i * keyHeight + keyHeight / 2 + 2,
                keyWidth / 2,
                keyHeight - 4,
                black);
        }
    }
    drawText(pixels, width, height, x + keyWidth / 2 + 4, y + keyHeight * 6 + 6, cName, black);
}

void draw(Color32* pixels, U32 width, U32 height)
{
    F64 trackPosition = atomicLoadF64(&track.position);
    char positionText[128];
    snprintf(
        positionText,
        sizeof(positionText),
        "%f beats (%f s)",
        trackPosition,
        toSeconds(trackPosition, track.bpm));
    drawText(pixels, width, height, 0, 400, positionText, white);
    drawOctave(pixels, width, height, 0, 0, "C4");
    drawOctave(pixels, width, height, 0, 7 * keyHeight, "C3");
    const U32 gridX = keyWidth + 2;
    const U32 beatWidth = 64;
    const U32 noteHeight = 14;
    for (U32 x = 0; x < track.duration + 1; x += 1)
    {
        fillRect(pixels, width, height, gridX + x * beatWidth, 0, 1, noteHeight * 24, gridColor);
    }
    for (U32 y = 0; y <= 24; y += 1)
    {
        fillRect(
            pixels, width, height, gridX, y * noteHeight, track.duration * beatWidth, 1, gridColor);
    }
    for (U32 noteId = 0; noteId < track.noteCount; noteId += 1)
    {
        Note* note = &track.notes[noteId];
        if (note->number >= 48 && note->number < 72)
        {
            U32 y = noteHeight * (23 - (note->number - 48));
            fillRect(
                pixels,
                width,
                height,
                (gridX + note->start * beatWidth) + 1,
                y + 1,
                (note->duration * beatWidth) - 2,
                noteHeight - 2,
                noteColor);
        }
    }
    fillRect(
        pixels,
        width,
        height,
        gridX + trackPosition * beatWidth,
        0,
        1,
        noteHeight * 24,
        positionColor);
}

int main(void)
{
    track.notes[track.noteCount++] = (Note){
        .number = 69, // A4
        .start = 0.0,
        .duration = 1.0,
        .amplitude = 0.8,
    };
    track.notes[track.noteCount++] = (Note){
        .number = 60, // C4
        .start = 1.0,
        .duration = 0.5,
        .amplitude = 0.8,
    };
    track.notes[track.noteCount++] = (Note){
        .number = 60, // C4
        .start = 1.5,
        .duration = 0.5,
        .amplitude = 0.8,
    };
    track.notes[track.noteCount++] = (Note){
        .number = 48, // C3
        .start = 2.5,
        .duration = 0.5,
        .amplitude = 0.8,
    };
    track.bpm = 70.0;
    track.duration = 4.0;
    track.sampleRate = audioSampleRate;
    run((App){
        .audioSampleRate = audioSampleRate,
        .audio = audio,
        .draw = draw,
    });
    return 0;
}
