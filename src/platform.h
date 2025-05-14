#pragma once
#include "core.h"

static F64 atomicLoadF64(const F64* ptr)
{
    F64 value;
    __atomic_load(ptr, &value, __ATOMIC_SEQ_CST);
    return value;
}

static void atomicStoreF64(F64* ptr, F64 value)
{
    __atomic_store(ptr, &value, __ATOMIC_SEQ_CST);
}

typedef struct Color32
{
    U8 r, g, b, a;
} Color32;

typedef struct App
{
    F32 audioSampleRate;
    void (*audio)(F32* buffer, U32 bufferSize);
    void (*draw)(Color32* buffer, U32 width, U32 height);
} App;

void run(App game);
