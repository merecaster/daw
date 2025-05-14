#include "platform.h"
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>

static OSStatus audioCallback(
    void* state,
    AudioUnitRenderActionFlags* flags,
    const AudioTimeStamp* timeStamp,
    U32 busNumber,
    U32 frameCount,
    AudioBufferList* buffers)
{
    F32* buffer = (F32*)buffers->mBuffers[0].mData;
    App* app = (App*)state;
    app->audio(buffer, frameCount);
    return noErr;
}

B32 startAudio(App* app)
{
    AudioUnit audioUnit = 0;
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
                .mSampleRate = app->audioSampleRate,
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
        .inputProcRefCon = app,
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
    if (AudioOutputUnitStart(audioUnit))
    {
        return 0;
    }
    return 1;
}

@interface SoftView : NSView
@end

@implementation SoftView {
    CADisplayLink* displayLink;
    CGColorSpaceRef colorSpace;
    App app;
    U32 width;
    U32 height;
    Color32* pixels;
}

- (instancetype)initWithFrame:(NSRect)frameRect app:(App)aApp {
    self = [super initWithFrame:frameRect];
    if (self) {
        displayLink = [self displayLinkWithTarget:self selector:@selector(render)];
        colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
        app = aApp;
        width = frameRect.size.width;
        height = frameRect.size.height;
        pixels = malloc(width * height * sizeof(Color32));
    }
    return self;
}

- (void)dealloc {
    [self stopRendering];
    CGColorSpaceRelease(colorSpace);
    free(pixels);
    [super dealloc];
}

- (void)startRendering {
    [displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
}

- (void)stopRendering {
    [displayLink removeFromRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
}

- (void)render {
    memset(pixels, 0, width * height * 4);
    app.draw(pixels, width, height);
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
    CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, pixels, width * height * sizeof(Color32), 0);
    CGImageRef image = CGImageCreate(
        width,
        height,
        8,
        32,
        width * 4,
        colorSpace,
        kCGBitmapByteOrder32Big | kCGImageAlphaPremultipliedLast,
        provider,
        NULL,
        false,
        kCGRenderingIntentDefault);
    CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
    CGContextDrawImage(context, CGRectMake(0.5, 0.5, width, height), image);
    CGImageRelease(image);
    CGDataProviderRelease(provider);
}

- (void)setFrameSize:(NSSize)newSize {
    [super setFrameSize:newSize];
    width = newSize.width;
    height = newSize.height;
    pixels = realloc(pixels, width * height * sizeof(Color32));
    [self render];
}
@end

@interface WindowDelegate : NSObject<NSWindowDelegate>
@end

@implementation WindowDelegate
- (void)windowWillClose:(NSNotification*)notification
{
    [NSApp terminate:nil];
}
@end

void run(App app) {
    @autoreleasepool
    {
        NSApplication* application = [NSApplication sharedApplication];
        [application setActivationPolicy:NSApplicationActivationPolicyRegular];
        NSRect frame = NSMakeRect(0, 0, 800, 600);
        NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
        NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
                                                       styleMask:style
                                                         backing:NSBackingStoreBuffered
                                                           defer:NO];
        WindowDelegate* delegate = [[WindowDelegate alloc] init];
        [window setDelegate:delegate];
        [window makeKeyAndOrderFront:nil];
        SoftView* view = [[SoftView alloc] initWithFrame:frame app:app];
        [window setContentView:view];
        [window makeFirstResponder:view];
        [window zoom:nil];
        [view startRendering];
        startAudio(&app);
        [application activate];
        [application run];
    }
}
