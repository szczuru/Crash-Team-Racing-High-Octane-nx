/*
 * Nintendo Switch OpenGL (EGL / switch-mesa nouveau) context layer.
 *
 * Ten plik jest CAŁKOWICIE nowy i nie modyfikuje żadnego istniejącego pliku
 * poza kilkoma jednolinijkowymi "hookami" w native_renderer.c / native_glad.c
 * (patrz PATCH_NOTES.md w tym samym katalogu). Celem jest odtworzenie na
 * Switchu dokładnie tego, co na PC robi SDL3 (SDL_CreateWindow +
 * SDL_GL_CreateContext + SDL_GL_SwapWindow), ale surowym libnx + EGL, bo
 * SDL3 nie ma backendu pod Switcha.
 */
#ifndef NATIVE_RENDERER_SWITCH_H
#define NATIVE_RENDERER_SWITCH_H

#ifdef __SWITCH__

#include <stdbool.h>

/* Tworzy okno systemowe (NWindow) + kontekst EGL/OpenGL. Odpowiednik
 * SDL_CreateWindow + SDL_GL_CreateContext z native_renderer.c. */
bool NativeRendererSwitch_InitContext(void);

/* Niszczy kontekst EGL i zwalnia NWindow. Wołane z Platform_Shutdown. */
void NativeRendererSwitch_ShutdownContext(void);

/* Odpowiednik SDL_GL_SwapWindow(g_window). */
void NativeRendererSwitch_SwapBuffers(void);

/* Zwraca aktualną rozdzielczość docelową w zależności od trybu konsoli:
 * handheld/tabletop -> 1280x720, zadokowana -> 1920x1080.
 * Sprawdza appletGetOperationMode() za każdym wywołaniem, więc wynik
 * może się zmienić z klatki na klatkę po (od)dokowaniu. */
void NativeRendererSwitch_GetFramebufferSize(int *outWidth, int *outHeight);

/* true, jeśli tryb konsoli zmienił się od ostatniego wywołania - sygnał dla
 * NativeRenderer żeby przebudować render targety/viewport w nowej rozdzielczości. */
bool NativeRendererSwitch_ConsumeOperationModeChanged(void);

/* Używane przez native_glad.c jako zamiennik dlsym()/SDL_GL_GetProcAddress
 * na platformach desktopowych. */
void *NativeRendererSwitch_GetProcAddress(const char *name);

#endif /* __SWITCH__ */

#endif /* NATIVE_RENDERER_SWITCH_H */
