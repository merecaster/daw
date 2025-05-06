#include "audio.h"
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>

static AudioUnit audioUnit;
static AudioCallback callback;

static OSStatus audioCallback(
    void* state,
    AudioUnitRenderActionFlags* flags,
    const AudioTimeStamp* timeStamp,
    U32 busNumber,
    U32 frameCount,
    AudioBufferList* buffers)
{
    F32* buffer = (F32*)buffers->mBuffers[0].mData;
    callback(buffer, frameCount);
    return noErr;
}

B32 audioInit(AudioConfig config)
{
    callback = config.callback;
    AudioComponent audioComponent = AudioComponentFindNext(
        0,
        &(AudioComponentDescription){
            .componentType = kAudioUnitType_Output,
            .componentSubType = kAudioUnitSubType_DefaultOutput,
            .componentManufacturer = kAudioUnitManufacturer_Apple,
        });
    if (!audioComponent)
    {
        return 0;
    }
    if (AudioComponentInstanceNew(audioComponent, &audioUnit))
    {
        return 0;
    }
    U32 channelsPerFrame = 2;
    U32 bytesPerFrame = channelsPerFrame * sizeof(F32);
    U32 bitsPerChannel = sizeof(F32) * 8;
    U32 framesPerPacket = 1;
    U32 bytesPerPacket = bytesPerFrame * framesPerPacket;
    if (AudioUnitSetProperty(
            audioUnit,
            kAudioUnitProperty_StreamFormat,
            kAudioUnitScope_Input,
            0,
            &(AudioStreamBasicDescription){
                .mSampleRate = config.sampleRate,
                .mFormatID = kAudioFormatLinearPCM,
                .mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked,
                .mBitsPerChannel = bitsPerChannel,
                .mChannelsPerFrame = channelsPerFrame,
                .mFramesPerPacket = framesPerPacket,
                .mBytesPerFrame = bytesPerFrame,
                .mBytesPerPacket = bytesPerPacket,
            },
            sizeof(AudioStreamBasicDescription)))
    {
        return 0;
    }
    if (AudioUnitInitialize(audioUnit))
    {
        return 0;
    }
    AURenderCallbackStruct callback = {
        .inputProc = audioCallback,
        .inputProcRefCon = 0,
    };
    if (AudioUnitSetProperty(
            audioUnit,
            kAudioUnitProperty_SetRenderCallback,
            kAudioUnitScope_Input,
            0,
            &callback,
            sizeof(callback)))
    {
        return 0;
    }
    return 1;
}

B32 audioStart(void)
{
    return AudioOutputUnitStart(audioUnit) == noErr;
}

B32 audioStop(void)
{
    return AudioOutputUnitStop(audioUnit) == noErr;
}

void audioDeinit(void)
{
    AudioUnitUninitialize(audioUnit);
    AudioComponentInstanceDispose(audioUnit);
}
