/*
 * Nintendo Switch gamepad backend (libnx HID Npad) exposed through the exact
 * subset of the SDL3 SDL_Gamepad / SDL_Joystick API that native_input.c calls.
 *
 * Ten plik jest CAŁKOWICIE nowy. native_input.c NIE jest przepisywany - trzyma
 * on `SDL_Gamepad *controller` jako pole struktury NativeInputController i woła
 * SDL_OpenGamepad / SDL_GetGamepadButton / SDL_GetGamepadAxis / SDL_RumbleGamepad
 * itd. Zamiast linkować prawdziwe SDL3 (którego nie ma pod Switcha) dostarczamy
 * WŁASNE definicje tych funkcji SDL, oparte o libnx PadState. Dzięki temu cały
 * ~1200-liniowy native_input.c działa bez zmian layoutu.
 *
 * Jedyne zmiany w native_input.c:
 *   1. #include "platform/native_input_switch.h" (dla NativeInputSwitch_PumpPads()).
 *   2. Gałąź klawiatury w Platform_InputUpdate: #ifdef __vita__ -> 
 *      #if defined(__vita__) || defined(__SWITCH__).
 * (patrz PATCH_NOTES_switch_input.md)
 */
#ifndef NATIVE_INPUT_SWITCH_H
#define NATIVE_INPUT_SWITCH_H

#ifdef __SWITCH__

/* Wołane raz na klatkę z Platform_InputUpdate (w gałęzi __SWITCH__), zamiast
 * SDL_PumpEvents(). Odświeża wszystkie libnx PadState przez padUpdate(). */
void NativeInputSwitch_PumpPads(void);

#endif /* __SWITCH__ */

#endif /* NATIVE_INPUT_SWITCH_H */
