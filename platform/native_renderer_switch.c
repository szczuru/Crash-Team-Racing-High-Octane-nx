/*
 * Nintendo Switch OpenGL (EGL / switch-mesa nouveau) context layer.
 * Nowy plik - patrz komentarz w native_renderer_switch.h.
 *
 * Wymagane pakiety devkitPro: switch-mesa (ciągnie switch-libdrm_nouveau).
 * Zainstaluj przez: sudo dkp-pacman -S switch-mesa
 */
#ifdef __SWITCH__

#include "platform/native_renderer_switch.h"
#include "platform/native_log.h"

/* libnx <switch.h> defines `typedef struct Thread {...} Thread;` (OS thread
 * type), which clashes with the game's own `struct Thread`
 * (include/namespace_Proc.h) since this whole project is one unity build.
 * This file never names libnx's Thread type, so simply shadowing the macro
 * around the include (undone right after) is enough. */
/* NOTE: <switch.h> is header-guarded, and this whole project is one unity
 * build - whichever *_switch.c file's #include actually runs FIRST (per
 * main.c's #include order) is the only one whose macro rename takes effect;
 * the header guard silently skips the body for every later #include, even
 * with different macros defined. All *_switch.c files that touch <switch.h>
 * must therefore use the SAME alias names (LibnxOsThread/LibnxOsThreadFunc),
 * matching native_sdl_shim_switch.c, regardless of which file wins. */
#define Thread LibnxOsThread
#define ThreadFunc LibnxOsThreadFunc
#include <switch.h>
#undef Thread
#undef ThreadFunc
#include <EGL/egl.h>
#include <EGL/eglext.h>

#define NATIVE_RENDERER_SWITCH_HANDHELD_WIDTH  1280
#define NATIVE_RENDERER_SWITCH_HANDHELD_HEIGHT 720
#define NATIVE_RENDERER_SWITCH_DOCKED_WIDTH    1920
#define NATIVE_RENDERER_SWITCH_DOCKED_HEIGHT   1080

static EGLDisplay s_display = EGL_NO_DISPLAY;
static EGLContext s_context = EGL_NO_CONTEXT;
static EGLSurface s_surface = EGL_NO_SURFACE;
static NWindow *s_window = NULL;

static AppletOperationMode s_lastOperationMode = AppletOperationMode_Handheld;
static int s_operationModeChanged = 0;

static void NativeRendererSwitch_ModeToSize(AppletOperationMode mode, int *outWidth, int *outHeight)
{
	if (mode == AppletOperationMode_Console)
	{
		*outWidth = NATIVE_RENDERER_SWITCH_DOCKED_WIDTH;
		*outHeight = NATIVE_RENDERER_SWITCH_DOCKED_HEIGHT;
	}
	else
	{
		*outWidth = NATIVE_RENDERER_SWITCH_HANDHELD_WIDTH;
		*outHeight = NATIVE_RENDERER_SWITCH_HANDHELD_HEIGHT;
	}
}

bool NativeRendererSwitch_InitContext(void)
{
	s_window = nwindowGetDefault();
	if (s_window == NULL)
	{
		Platform_LogError("[CTR Native/Switch] nwindowGetDefault() failed\n");
		return false;
	}

	s_lastOperationMode = appletGetOperationMode();
	s_operationModeChanged = 0;

	int width, height;
	NativeRendererSwitch_ModeToSize(s_lastOperationMode, &width, &height);
	/* Ustawia natywną rozdzielczość bufora prezentacji - to ona steruje
	 * 720p handheld / 1080p docked, niezależnie od tego w jakiej
	 * rozdzielczości renderujemy wewnętrznie. */
	nwindowSetDimensions(s_window, width, height);

	s_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (s_display == EGL_NO_DISPLAY)
	{
		Platform_LogError("[CTR Native/Switch] eglGetDisplay failed\n");
		return false;
	}

	if (!eglInitialize(s_display, NULL, NULL))
	{
		Platform_LogError("[CTR Native/Switch] eglInitialize failed\n");
		return false;
	}

	eglBindAPI(EGL_OPENGL_API);

	static const EGLint framebufferAttributeList[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		// NOTE: no EGL_ALPHA_SIZE requested for the window/NWindow surface
		// itself - unlike a desktop window compositor (which ignores an
		// application window's own framebuffer alpha), the Switch NWindow
		// surface honours it, and the game's final presented frame carries
		// PSX draw-mask bits in its alpha channel that are ~0 for most
		// ordinary draws. An alpha-enabled surface would composite that as
		// (near-)transparent - visually a black screen - even though the
		// present shader now also forces alpha=1.0 on its own (see
		// ctr_present_rgba_shader in native_renderer.c). Belt-and-suspenders:
		// don't give the compositor an alpha channel to honour in the first
		// place.
		EGL_DEPTH_SIZE, 24,
		EGL_STENCIL_SIZE, 8,
		EGL_NONE
	};

	EGLConfig config;
	EGLint numConfigs;
	if (!eglChooseConfig(s_display, framebufferAttributeList, &config, 1, &numConfigs) || numConfigs == 0)
	{
		Platform_LogError("[CTR Native/Switch] eglChooseConfig failed\n");
		return false;
	}

	s_surface = eglCreateWindowSurface(s_display, config, s_window, NULL);
	if (s_surface == EGL_NO_SURFACE)
	{
		Platform_LogError("[CTR Native/Switch] eglCreateWindowSurface failed: 0x%x\n", eglGetError());
		return false;
	}

	/* Prosimy o 4.3 core i schodzimy w dół, tak samo jak robi to
	 * NativeRenderer_InitialiseGLContext dla PC (min. OpenGL 3.x). */
	int majorVersion = 4;
	int minorVersion = 3;
	do
	{
		const EGLint contextAttributeList[] = {
			EGL_CONTEXT_MAJOR_VERSION_KHR, majorVersion,
			EGL_CONTEXT_MINOR_VERSION_KHR, minorVersion,
			EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
			EGL_NONE
		};

		s_context = eglCreateContext(s_display, config, EGL_NO_CONTEXT, contextAttributeList);
		if (s_context != EGL_NO_CONTEXT)
		{
			break;
		}

		if (minorVersion > 0)
		{
			minorVersion--;
		}
		else if (majorVersion > 3)
		{
			majorVersion--;
			minorVersion = 3;
		}
		else
		{
			break;
		}
	} while (1);

	if (s_context == EGL_NO_CONTEXT)
	{
		Platform_LogError("[CTR Native/Switch] eglCreateContext failed: 0x%x\n", eglGetError());
		return false;
	}

	if (!eglMakeCurrent(s_display, s_surface, s_surface, s_context))
	{
		Platform_LogError("[CTR Native/Switch] eglMakeCurrent failed: 0x%x\n", eglGetError());
		return false;
	}

	/* vsync: 1 = zablokowane do odświeżania ekranu (60Hz), spójne z opcją
	 * "60 FPS" z wersji Vity. */
	eglSwapInterval(s_display, 1);

	Platform_Log("[CTR Native/Switch] OpenGL %d.%d context ready (%dx%d)\n", majorVersion, minorVersion, width, height);

	return true;
}

void NativeRendererSwitch_ShutdownContext(void)
{
	if (s_display != EGL_NO_DISPLAY)
	{
		eglMakeCurrent(s_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

		if (s_context != EGL_NO_CONTEXT)
		{
			eglDestroyContext(s_display, s_context);
			s_context = EGL_NO_CONTEXT;
		}

		if (s_surface != EGL_NO_SURFACE)
		{
			eglDestroySurface(s_display, s_surface);
			s_surface = EGL_NO_SURFACE;
		}

		eglTerminate(s_display);
		s_display = EGL_NO_DISPLAY;
	}

	s_window = NULL;
}

void NativeRendererSwitch_SwapBuffers(void)
{
	/* KRYTYCZNE: libnx wymaga regularnego wywoływania appletMainLoop(), żeby
	 * przetwarzać komunikaty systemowe applet (focus/sleep/powiadomienia) -
	 * każdy oficjalny przykład graficzny libnx (np.
	 * switch-examples/graphics/opengl/simple_triangle) woła to w pętli
	 * głównej razem z eglSwapBuffers. Bez tego bufor prezentacji (nvnflinger)
	 * może nigdy nie odebrać zaprezentowanej klatki - eglSwapBuffers() wisi
	 * w nieskończoność, co objawia się jako trwały czarny ekran, mimo że
	 * kontekst EGL utworzył się poprawnie i cała gra działa dalej w tle.
	 * Ten kod nigdzie wcześniej nie wołał appletMainLoop(). */
	appletMainLoop();

	/* Sprawdzamy zmianę trybu konsoli raz na klatkę - najtańszy i
	 * najbardziej niezawodny moment, zaraz po prezentacji poprzedniej. */
	AppletOperationMode mode = appletGetOperationMode();
	if (mode != s_lastOperationMode)
	{
		s_lastOperationMode = mode;
		s_operationModeChanged = 1;

		int width, height;
		NativeRendererSwitch_ModeToSize(mode, &width, &height);
		nwindowSetDimensions(s_window, width, height);
		Platform_Log("[CTR Native/Switch] Operation mode changed -> %dx%d\n", width, height);
	}

	eglSwapBuffers(s_display, s_surface);
}

void NativeRendererSwitch_GetFramebufferSize(int *outWidth, int *outHeight)
{
	NativeRendererSwitch_ModeToSize(appletGetOperationMode(), outWidth, outHeight);
}

bool NativeRendererSwitch_ConsumeOperationModeChanged(void)
{
	int changed = s_operationModeChanged;
	s_operationModeChanged = 0;
	return changed != 0;
}

void *NativeRendererSwitch_GetProcAddress(const char *name)
{
	return (void *)eglGetProcAddress(name);
}

#endif /* __SWITCH__ */
