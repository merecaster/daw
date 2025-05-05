#pragma once
#include <stdint.h>

typedef uint32_t U32;
typedef float F32;
typedef double F64;
typedef U32 B32;

typedef void (*AudioCallback)(F32* buffer, U32 frames);

typedef struct AudioConfig
{
    F32 sampleRate;
    AudioCallback callback;
} AudioConfig;

B32 audioInit(AudioConfig config);
B32 audioStart(void);
B32 audioStop(void);
void audioDeinit(void);
