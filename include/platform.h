#ifndef PLATFORM_H
#define PLATFORM_H

struct PlatformMempackArena
{
	void *base;
	void *start;
	void *endOfMemory;
	int size;
	int backingSize;
};

// NOTE: returns 0 on failure (e.g. renderer/EGL context could not be
// created), 1 on success. Callers MUST check this before proceeding into
// the game loop - on some platforms (e.g. Switch) a failed renderer init
// otherwise goes completely unnoticed (errors only reach an invisible log)
// and the game would spin forever in CTR_Main() drawing through a dead
// GL context, which looks exactly like a black screen with zero indication
// of what went wrong.
int Platform_Init(const char *title, int width, int height);
void Platform_Shutdown(void);
void Platform_InitScratchpad(void);
const struct PlatformMempackArena *Platform_InitMempackArena(void);
const struct PlatformMempackArena *Platform_GetMempackArena(void);
void Platform_BeginFrame(void);
int Platform_BeginScene(void);
void Platform_EndScene(void);
void Platform_EndFrame(void);
void Platform_PresentVRAMDisplay(void);
void Platform_PinVRAMDisplayFrames(int frameCount);
void Platform_PinVRAMDisplayRect(int x, int y, int w, int h, int frameCount);
void Platform_PinTextureDisplay(unsigned int texture, int contentHeight, int displayHeight, int frameCount);
int Platform_GetVBlankCount(void);
void Platform_WaitUntilVBlank(int targetVBlank);
void Platform_PollHostEvents(void);
int Platform_PollInput(void);
int Platform_InputStartPressed(void);
#if defined(CTR_NATIVE) && !defined(__vita__)
void Platform_SetBorderless(int enabled);
#endif

#if defined(CTR_NATIVE)
int NikoGetEnterKey(void);
#endif

#endif
