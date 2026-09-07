# Nintendo Switch port — stan i notatki techniczne

Ten dokument opisuje port CTR: High Octane na Nintendo Switch (devkitA64 /
libnx), z naciskiem na to, KTÓRE pliki są nowe, a które współdzielone zostały
zmienione (i dlaczego minimalnie), żeby ułatwić przyszłe merge z upstreamem
(Rinnegatamante / CTR-tools).

## Zasada naczelna

Cały kod specyficzny dla Switcha żyje w NOWYCH plikach `*_switch.*`. Zmiany w
plikach współdzielonych są ograniczone do pojedynczych gałęzi
`#elif defined(__SWITCH__)` / `#if defined(__SWITCH__)`, żeby zminimalizować
konflikty przy `git pull`/merge.

## Nowe pliki (w 100% owinięte w `#ifdef __SWITCH__`)

| Plik | Rola |
| --- | --- |
| `include/platform/native_renderer_switch.h` / `platform/native_renderer_switch.c` | Kontekst OpenGL przez EGL + `nwindowGetDefault()` (switch-mesa/nouveau). Odpowiednik `SDL_CreateWindow`+`SDL_GL_CreateContext`+`SDL_GL_SwapWindow`. Natywna rozdzielczość 1280x720 handheld / 1920x1080 docked, przełączana w locie przez `appletGetOperationMode()`. vsync = `eglSwapInterval(1)`. |
| `include/platform/native_input_switch.h` / `platform/native_input_switch.c` | Backend gamepada: implementuje podzbiór SDL3 `SDL_Gamepad`/`SDL_Joystick` API na libnx `PadState` (do 4 graczy + handheld). Dzięki temu `native_input.c` (~1200 linii) działa bez zmian layoutu struktur. |
| `platform/native_sdl_shim_switch.c` | Rdzeń shimu SDL3: wątki/muteksy/condvary (libnx `Thread`/`Mutex`/`CondVar`), czas (`armGetSystemTick`/`svcSleepThread`), atomiki, audio jako NO-OP (cisza), okno/wideo/kursor jako bezpieczne stuby. |
| `Makefile.switch` | Build devkitA64/libnx: kompiluje `main.c` (unity build) -> `ctr.nro`. |
| `.github/workflows/build-switch.yml` | CI: obraz `devkitpro/devkita64`, instaluje `switch-mesa`, buduje, wrzuca artefakt `.nro`. |

### Dlaczego shim SDL zamiast prawdziwego SDL3

devkitPro NIE ma paczki `switch-sdl3`, a upstream SDL3 nie ma oficjalnego
backendu pod Switcha (fork `devkitPro/SDL` istnieje TYLKO dla SDL2). Zamiast
portować całe SDL3 dostarczamy własne definicje DOKŁADNIE tych symboli SDL,
których gra realnie używa. Prawdziwe nagłówki SDL3 (`externals/SDL/include`)
są nadal include'owane — typy/enumy/stałe kompilują się bez linkowania biblioteki.

## Zmiany w plikach współdzielonych (minimalne)

### `platform/native_renderer.c`
- **BUGFIX (dotyczył WSZYSTKICH platform):** poprzedni commit dodał
  `#if defined(__SWITCH__) ... #else` w `NativeRenderer_InitialiseGLContext`,
  ale nie zamknął go `#endif`. Dodano brakujące `#endif /* !__SWITCH__ */`
  przed klamrą zamykającą funkcję. Bez tego preprocesor był niezbalansowany
  i psuł kompilację także na PC/Vicie.
- Dodano `#include "platform/native_renderer_switch.h"`.
- Swap buffers: gałąź `#elif defined(__SWITCH__) NativeRendererSwitch_SwapBuffers();`.
- Rozmiar framebuffera: gałąź `#if defined(__SWITCH__) NativeRendererSwitch_GetFramebufferSize(...)`.

### `platform/native_glad.c`
- `get_proc()`: gałąź `#elif defined(__SWITCH__) result = NativeRendererSwitch_GetProcAddress(namez);`
  (podpięcie loadera GLAD pod `eglGetProcAddress`). *(już było w repo)*

### `platform/native_input.c`
- Dodano `#include "platform/native_input_switch.h"`.
- `Platform_InputUpdate()`: gałąź `#elif defined(__SWITCH__)` woła
  `NativeInputSwitch_PumpPads()` (zamiast `SDL_PumpEvents()`), `keyboardButtons = 0xffff`.
- Wywołanie `NativeInput_ApplyKeyboard()`: guard zmieniony z `#ifndef __vita__`
  na `#if !defined(__vita__) && !defined(__SWITCH__)` (Switch nie ma klawiatury,
  zachowuje się jak Vita).

### `main.c`
- Unity build: blok `#if defined(__SWITCH__)` włączający
  `native_renderer_switch.c`, `native_input_switch.c`, `native_sdl_shim_switch.c`.
- `NativeConfig_GetPath()`: gałąź `#elif defined(__SWITCH__)` ->
  `"sdmc:/switch/ctr_native/config.ini"`.
- `sdlBasePath`: gałąź `#elif defined(__SWITCH__)` -> `"sdmc:/switch/ctr_native"`.
- Wrapper `main()`: gałąź `#elif defined(__SWITCH__)` — uruchamia `real_main`
  na wątku pthread z 4 MB stosu (domyślny stos Horizon bywa za mały), tworzy
  katalog danych na karcie SD. Wzorowane na istniejącej gałęzi `__vita__`.

Widescreen 1280x720 trafia automatycznie w istniejącą gałąź
`#elif CTR_NATIVE_WIDESCREEN` w `main()` — `Makefile.switch` definiuje
`-DCTR_NATIVE_WIDESCREEN`, więc NIE trzeba było dodawać nowej gałęzi.

## Co działa w tej wersji (pierwszy bootujący build)

- Renderer OpenGL przez switch-mesa, natywna rozdzielczość, przełączanie
  handheld/docked w locie.
- 60 FPS: `CTR_NATIVE_60FPS` włącza się automatycznie dla `CTR_NATIVE`
  (macros.h), a vsync zapewnia `eglSwapInterval(1)`.
- Sterowanie: do 4 padów + handheld, analogi, wibracje. **Ograniczenie:** pady
  wykrywane są przy starcie (`NativeInput_OpenKnownControllers`); hot-plug w
  trakcie gry nie jest jeszcze obsługiwany (shim `SDL_PollEvent` nie generuje
  zdarzeń `SDL_EVENT_GAMEPAD_ADDED/REMOVED`). Do dopracowania: syntezowanie tych
  zdarzeń w `SDL_PollEvent` na podstawie zmiany `padIsConnected` per klatkę.
- **Audio: CISZA** (świadomie, na pierwszą wersję — patrz niżej).
- Online/leaderboard/duchy: **wyłączone** (kompilują się jako no-opy — patrz niżej).

## Zamierzone ograniczenia / TODO

### Audio (na razie cisza)
`native_sdl_shim_switch.c` ma `SDL_OpenAudioDeviceStream` zwracające `NULL`,
przez co `NativeAudio_OpenDevice()` gracefully zwraca 0, a `SpuInit()` ignoruje
wynik — gra bootuje bez dźwięku. Prawdziwe audio wymaga backendu libnx
(`audren`/`audout`) i podmiany no-opów w sekcji AUDIO tego pliku na realny
strumień PCM (kontrakt: 16-bit signed, `NATIVE_AUDIO_SAMPLE_RATE`,
`NATIVE_AUDIO_CHANNELS`).

### Leaderboard i duchy (ghosts) przez internet — "prawie za darmo"
Cały online stack jest wyłączony przez `CTR_NATIVE_HAS_LEADERBOARD == 0`
(`include/macros.h`, gałąź `#else`). Żeby włączyć na Switchu:
1. `include/macros.h`: dopisać przypadek dla Switcha ustawiający
   `CTR_NATIVE_HAS_LEADERBOARD 1` (np. `#elif defined(CTR_NATIVE) && defined(__SWITCH__)`).
2. `platform/native_leaderboard.c`: rozszerzyć gałęzie `#if defined(__vita__)`
   (transport `HttpGet`/hash, `curl_global_init/cleanup`, multipart upload)
   o `|| defined(__SWITCH__)`, żeby użyć `switch-curl` zamiast wpadać w gałąź
   `#else` (WinHTTP, którego NIE ma na libnx). Potrzebny provider SHA-256
   (mbedtls z devkitPro albo openssl).
3. `Makefile.switch` + workflow: dołączyć `switch-curl switch-mbedtls switch-zlib`
   i linkować `-lcurl -lmbedtls -lmbedcrypto -lmbedx509 -lz`.
Endpointy (te same co Vita/PC): `https://www.rinnegatamante.eu/ctr/...`
(`get_leaderboard.php`, `upload_record.php`, `api/me.php`). To zwykłe HTTPS
przez internet — nie ma tu problemu ad-hoc opisanego niżej.

### Lokalny multiplayer / netcode (adhoc -> internet UDP) — DUŻY, osobny temat
Lokalny multiplayer P2P na Vicie (`native_adhoc.c`) używa `SceNet adhoc` —
zamkniętego protokołu radiowego Sony (WiFi P2P bez routera), Vita-only. Switch
ma zupełnie inny, też zamknięty protokół (`LDN`/local wireless). **Nie da się
ich fizycznie zmostkować.** Na Switchu `native_adhoc.c` wpada automatycznie w
gałąź `#else` (non-vita) z no-opami, więc lokalny multiplayer jest po prostu
NIEDOSTĘPNY.

Aby uzyskać crossplay Switch<->Switch i Switch<->Vita, trzeba napisać OD ZERA
nową warstwę transportową UDP przez zwykły internet/wspólny router (a nie
prawdziwy ad-hoc), oraz dodać jej odpowiednik po stronie Vity obok istniejącego
adhoc. To osobny protokół sieciowy dla trybu wyścigu (input delay/rollback,
synchronizacja klatek symulacji), niezależny od leaderboardu/duchów. **To NIE
jest prosta podmiana ifdefów.**

> UWAGA dla użytkownika: jeśli oczekujesz literalnego "ad hoc" Switch<->Vita
> (jak dwie Vity obok siebie bez internetu) — to fizycznie niemożliwe.
> Realny crossplay musi iść przez internet/router.

## Budowanie

```sh
export DEVKITPRO=/opt/devkitpro
sudo dkp-pacman -S switch-mesa      # OpenGL/EGL (nouveau)
make -f Makefile.switch             # -> ctr.nro
```

Assety gry umieść na karcie SD w `sdmc:/switch/ctr_native/` (obok `.nro`).
