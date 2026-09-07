/*
 * Nintendo Switch core SDL3 shim (libnx-backed).
 *
 * SDL3 nie ma backendu pod Switcha, więc zamiast linkować prawdziwą bibliotekę
 * dostarczamy WŁASNE definicje dokładnie tych funkcji SDL, które są realnie
 * wołane w projekcie (poza gamepadem/klawiaturą - te są w native_input_switch.c).
 *
 * Prawdziwe nagłówki SDL3 (externals/SDL/include) są nadal include'owane dla
 * typów/enumów/stałych - one kompilują się bez linkowania. Tutaj definiujemy
 * tylko FUNKCJE.
 *
 * Zakres:
 *   - Wątki / muteksy / zmienne warunkowe  -> libnx (Thread / Mutex / CondVar)
 *   - Atomiki (SDL_AtomicInt)              -> libnx atomic loads/stores
 *   - Czas (Ticks / Performance / Delay)   -> armGetSystemTick / svcSleepThread
 *   - Audio                                -> NO-OP (cisza) na pierwszą wersję;
 *                                             SDL_OpenAudioDeviceStream zwraca NULL,
 *                                             przez co NativeAudio_OpenDevice()
 *                                             gracefully zwraca 0 (SpuInit ignoruje).
 *   - Okno / wideo / kursor / GL swap interval -> bezpieczne no-opy (kontekst
 *                                             tworzy native_renderer_switch.c).
 *   - Reszta (GetError/Hint/BasePath/...)  -> rozsądne stuby.
 *
 * Wymagane pakiety devkitPro: libnx (-lnx). Brak dodatkowych zależności.
 */
#ifdef __SWITCH__

#include <switch.h>

#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================== */
/* Init / Quit (wideo)                                                        */
/* ========================================================================== */
/* Uwaga: SDL_InitSubSystem / SDL_QuitSubSystem są zdefiniowane w
 * native_input_switch.c (inicjalizacja padów). Tu tylko SDL_Init / SDL_Quit. */

bool SDL_Init(SDL_InitFlags flags)
{
	(void)flags;
	/* Wideo na Switchu obsługuje native_renderer_switch.c (EGL). Nic tu nie
	 * inicjalizujemy - zwracamy sukces. */
	return true;
}

void SDL_Quit(void)
{
}

/* ========================================================================== */
/* Błędy / hinty / ścieżki                                                    */
/* ========================================================================== */

static char s_sdlError[256] = {0};

const char *SDL_GetError(void)
{
	return s_sdlError;
}

/* SDL_SetError jest zmienno-argumentowe w SDL3; definiujemy zgodny wariant,
 * gdyby coś je wołało (na wszelki wypadek - tani stub). */
bool SDL_SetError(const char *fmt, ...)
{
	(void)fmt;
	return false;
}

const char *SDL_GetHint(const char *name)
{
	(void)name;
	return NULL;
}

bool SDL_SetHint(const char *name, const char *value)
{
	(void)name;
	(void)value;
	return true;
}

const char *SDL_GetBasePath(void)
{
	/* main.c dla Switcha nie powinien tego wołać (używa hardkodowanej ścieżki
	 * sdmc:/), ale zwracamy sensowną wartość na wszelki wypadek. */
	return "sdmc:/switch/ctr_native/";
}

void SDL_free(void *mem)
{
	free(mem);
}

/* ========================================================================== */
/* Czas                                                                       */
/* ========================================================================== */

Uint64 SDL_GetTicks(void)
{
	/* armGetSystemTick -> nanosekundy -> milisekundy. */
	return (Uint64)(armTicksToNs(armGetSystemTick()) / 1000000ULL);
}

Uint64 SDL_GetPerformanceCounter(void)
{
	return (Uint64)armGetSystemTick();
}

Uint64 SDL_GetPerformanceFrequency(void)
{
	/* Częstotliwość licznika systemowego Switcha (19.2 MHz). armTicksToNs
	 * używa tej samej stałej; zwracamy ją jawnie jako Hz. */
	return (Uint64)armGetSystemTickFreq();
}

void SDL_DelayPrecise(Uint64 ns)
{
	svcSleepThread((s64)ns);
}

void SDL_Delay(Uint32 ms)
{
	svcSleepThread((s64)ms * 1000000LL);
}

/* ========================================================================== */
/* Atomiki                                                                    */
/* ========================================================================== */
/* SDL_AtomicInt to { int value; } (nie-nieprzezroczyste w nagłówku SDL3). */

int SDL_GetAtomicInt(SDL_AtomicInt *a)
{
	if (a == NULL)
	{
		return 0;
	}
	return __atomic_load_n(&a->value, __ATOMIC_SEQ_CST);
}

int SDL_SetAtomicInt(SDL_AtomicInt *a, int v)
{
	int prev;
	if (a == NULL)
	{
		return 0;
	}
	prev = __atomic_exchange_n(&a->value, v, __ATOMIC_SEQ_CST);
	return prev;
}

/* ========================================================================== */
/* Muteksy (libnx Mutex na stercie -> nieprzezroczysty SDL_Mutex*)            */
/* ========================================================================== */

SDL_Mutex *SDL_CreateMutex(void)
{
	Mutex *m = (Mutex *)malloc(sizeof(Mutex));
	if (m != NULL)
	{
		mutexInit(m);
	}
	return (SDL_Mutex *)m;
}

void SDL_LockMutex(SDL_Mutex *mutex)
{
	if (mutex != NULL)
	{
		mutexLock((Mutex *)mutex);
	}
}

void SDL_UnlockMutex(SDL_Mutex *mutex)
{
	if (mutex != NULL)
	{
		mutexUnlock((Mutex *)mutex);
	}
}

void SDL_DestroyMutex(SDL_Mutex *mutex)
{
	if (mutex != NULL)
	{
		free(mutex);
	}
}

/* ========================================================================== */
/* Zmienne warunkowe                                                          */
/* ========================================================================== */

SDL_Condition *SDL_CreateCondition(void)
{
	CondVar *c = (CondVar *)malloc(sizeof(CondVar));
	if (c != NULL)
	{
		condvarInit(c);
	}
	return (SDL_Condition *)c;
}

void SDL_WaitCondition(SDL_Condition *cond, SDL_Mutex *mutex)
{
	if ((cond != NULL) && (mutex != NULL))
	{
		condvarWait((CondVar *)cond, (Mutex *)mutex);
	}
}

void SDL_SignalCondition(SDL_Condition *cond)
{
	if (cond != NULL)
	{
		condvarWakeOne((CondVar *)cond);
	}
}

void SDL_BroadcastCondition(SDL_Condition *cond)
{
	if (cond != NULL)
	{
		condvarWakeAll((CondVar *)cond);
	}
}

void SDL_DestroyCondition(SDL_Condition *cond)
{
	if (cond != NULL)
	{
		free(cond);
	}
}

/* ========================================================================== */
/* Wątki                                                                      */
/* ========================================================================== */
/* SDL_CreateThread to makro -> SDL_CreateThreadRuntime, więc definiujemy to.
 * SDL_ThreadFunction zwraca int; libnx ThreadFunc zwraca void -> trampolina. */

#define NATIVE_SWITCH_THREAD_STACK_SIZE (128 * 1024)

struct SDL_Thread
{
	Thread thread;
	SDL_ThreadFunction fn;
	void *data;
	int result;
	int started;
};

static void NativeSwitch_ThreadTrampoline(void *arg)
{
	struct SDL_Thread *t = (struct SDL_Thread *)arg;
	if ((t != NULL) && (t->fn != NULL))
	{
		t->result = t->fn(t->data);
	}
}

SDL_Thread *SDL_CreateThreadRuntime(SDL_ThreadFunction fn, const char *name,
	void *data, SDL_FunctionPointer pfnBeginThread, SDL_FunctionPointer pfnEndThread)
{
	struct SDL_Thread *t;
	Result rc;

	(void)name;
	(void)pfnBeginThread;
	(void)pfnEndThread;

	t = (struct SDL_Thread *)malloc(sizeof(struct SDL_Thread));
	if (t == NULL)
	{
		return NULL;
	}
	memset(t, 0, sizeof(*t));
	t->fn = fn;
	t->data = data;

	/* prio 0x2C = typowy priorytet wątku głównego; cpuid -2 = domyślny rdzeń. */
	rc = threadCreate(&t->thread, NativeSwitch_ThreadTrampoline, t, NULL,
		NATIVE_SWITCH_THREAD_STACK_SIZE, 0x2C, -2);
	if (R_FAILED(rc))
	{
		free(t);
		return NULL;
	}

	rc = threadStart(&t->thread);
	if (R_FAILED(rc))
	{
		threadClose(&t->thread);
		free(t);
		return NULL;
	}

	t->started = 1;
	return (SDL_Thread *)t;
}

void SDL_WaitThread(SDL_Thread *thread, int *status)
{
	struct SDL_Thread *t = (struct SDL_Thread *)thread;
	if (t == NULL)
	{
		if (status != NULL)
		{
			*status = 0;
		}
		return;
	}

	if (t->started != 0)
	{
		threadWaitForExit(&t->thread);
		threadClose(&t->thread);
	}

	if (status != NULL)
	{
		*status = t->result;
	}
	free(t);
}

bool SDL_SetCurrentThreadPriority(SDL_ThreadPriority priority)
{
	(void)priority;
	/* Priorytet ustawiany przy tworzeniu wątku; tu no-op. */
	return true;
}

/* ========================================================================== */
/* Audio - NO-OP (cisza)                                                      */
/* ========================================================================== */

SDL_AudioStream *SDL_OpenAudioDeviceStream(SDL_AudioDeviceID devid,
	const SDL_AudioSpec *spec, SDL_AudioStreamCallback callback, void *userdata)
{
	(void)devid;
	(void)spec;
	(void)callback;
	(void)userdata;
	/* Zwracamy NULL -> NativeAudio_OpenDevice() zwraca 0 ("SDL audio
	 * unavailable"), a SpuInit() ignoruje wynik. Gra bootuje bez dźwięku. */
	snprintf(s_sdlError, sizeof(s_sdlError), "audio disabled on Switch (silent build)");
	return NULL;
}

bool SDL_ClearAudioStream(SDL_AudioStream *stream)
{
	(void)stream;
	return true;
}

void SDL_DestroyAudioStream(SDL_AudioStream *stream)
{
	(void)stream;
}

bool SDL_GetAudioDeviceFormat(SDL_AudioDeviceID devid, SDL_AudioSpec *spec, int *sample_frames)
{
	(void)devid;
	if (spec != NULL)
	{
		memset(spec, 0, sizeof(*spec));
	}
	if (sample_frames != NULL)
	{
		*sample_frames = 0;
	}
	return false;
}

SDL_AudioDeviceID SDL_GetAudioStreamDevice(SDL_AudioStream *stream)
{
	(void)stream;
	return 0;
}

bool SDL_GetAudioStreamFormat(SDL_AudioStream *stream, SDL_AudioSpec *src_spec, SDL_AudioSpec *dst_spec)
{
	(void)stream;
	if (src_spec != NULL)
	{
		memset(src_spec, 0, sizeof(*src_spec));
	}
	if (dst_spec != NULL)
	{
		memset(dst_spec, 0, sizeof(*dst_spec));
	}
	return false;
}

int SDL_GetAudioStreamQueued(SDL_AudioStream *stream)
{
	(void)stream;
	return 0;
}

bool SDL_LockAudioStream(SDL_AudioStream *stream)
{
	(void)stream;
	return true;
}

bool SDL_UnlockAudioStream(SDL_AudioStream *stream)
{
	(void)stream;
	return true;
}

bool SDL_PutAudioStreamData(SDL_AudioStream *stream, const void *buf, int len)
{
	(void)stream;
	(void)buf;
	(void)len;
	return true;
}

bool SDL_ResumeAudioStreamDevice(SDL_AudioStream *stream)
{
	(void)stream;
	return true;
}

bool SDL_SetAudioStreamGain(SDL_AudioStream *stream, float gain)
{
	(void)stream;
	(void)gain;
	return true;
}

const char *SDL_GetCurrentAudioDriver(void)
{
	return "null";
}

/* ========================================================================== */
/* Okno / wideo / kursor                                                      */
/* ========================================================================== */

void SDL_DestroyWindow(SDL_Window *window)
{
	(void)window;
}

SDL_WindowFlags SDL_GetWindowFlags(SDL_Window *window)
{
	(void)window;
	return 0;
}

bool SDL_SetWindowFullscreen(SDL_Window *window, bool fullscreen)
{
	(void)window;
	(void)fullscreen;
	return true;
}

bool SDL_SetWindowFullscreenMode(SDL_Window *window, const SDL_DisplayMode *mode)
{
	(void)window;
	(void)mode;
	return true;
}

bool SDL_SyncWindow(SDL_Window *window)
{
	(void)window;
	return true;
}

bool SDL_HideCursor(void)
{
	return true;
}

bool SDL_ShowCursor(void)
{
	return true;
}

bool SDL_GL_ExtensionSupported(const char *extension)
{
	/* GLAD ładuje rozszerzenia niezależnie; bezpieczny domyślny false. */
	(void)extension;
	return false;
}

bool SDL_GL_SetSwapInterval(int interval)
{
	/* vsync ustawiany przez eglSwapInterval w native_renderer_switch.c. */
	(void)interval;
	return true;
}

/* ========================================================================== */
/* Zdarzenia (brak kolejki SDL na Switchu)                                    */
/* ========================================================================== */

void SDL_PumpEvents(void)
{
	/* Wejście pompowane przez NativeInputSwitch_PumpPads(); brak kolejki SDL. */
}

bool SDL_PollEvent(SDL_Event *event)
{
	if (event != NULL)
	{
		memset(event, 0, sizeof(*event));
	}
	return false; /* brak zdarzeń */
}

/* ========================================================================== */
/* Powierzchnie / zrzuty (screenshoty)                                        */
/* ========================================================================== */

SDL_Surface *SDL_CreateSurfaceFrom(int width, int height, SDL_PixelFormat format, void *pixels, int pitch)
{
	(void)width;
	(void)height;
	(void)format;
	(void)pixels;
	(void)pitch;
	return NULL; /* screenshoty niedostępne w pierwszej wersji */
}

void SDL_DestroySurface(SDL_Surface *surface)
{
	(void)surface;
}

bool SDL_SaveBMP(SDL_Surface *surface, const char *file)
{
	(void)surface;
	(void)file;
	return false;
}

#endif /* __SWITCH__ */
