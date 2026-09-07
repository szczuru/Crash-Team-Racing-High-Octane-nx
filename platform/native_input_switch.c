/*
 * Nintendo Switch gamepad backend (libnx HID Npad) implementing the subset of
 * the SDL3 SDL_Gamepad / SDL_Joystick API used by native_input.c.
 *
 * Nowy plik - patrz komentarz w native_input_switch.h. Nie modyfikuje SDL ani
 * native_input.c poza dwoma jednolinijkowymi hookami opisanymi w PATCH_NOTES.
 *
 * Wymagane pakiety devkitPro: libnx (-lnx). Brak dodatkowych zależności.
 *
 * Mapowanie przycisków (layout Nintendo, pozycyjnie zgodne z SDL):
 *   SDL_GAMEPAD_BUTTON_SOUTH (dolny)  -> HidNpadButton_B
 *   SDL_GAMEPAD_BUTTON_EAST  (prawy)  -> HidNpadButton_A
 *   SDL_GAMEPAD_BUTTON_WEST  (lewy)   -> HidNpadButton_Y
 *   SDL_GAMEPAD_BUTTON_NORTH (górny)  -> HidNpadButton_X
 * Uwaga: HidNpadButton_A/B/X/Y są nazwane wg ETYKIET Nintendo, więc fizyczna
 * pozycja jest "odwrócona" względem nazw - powyższe mapowanie jest poprawne
 * pozycyjnie (tak jak oczekuje reszta gry, która używa pozycyjnych stałych SDL).
 */
#ifdef __SWITCH__

#include "platform/native_input_switch.h"

#include <switch.h>

/* Prawdziwe nagłówki SDL3 (typy/enumy/stałe są darmowe - nie linkujemy .a). */
#include <SDL3/SDL.h>

#include <stdlib.h>
#include <string.h>

#define NATIVE_INPUT_SWITCH_MAX_PADS 4

/* Reprezentacja "SDL_Gamepad" / "SDL_Joystick" dla jednego gracza Switcha.
 * native_input.c traktuje SDL_Gamepad* i SDL_Joystick* jako nieprzezroczyste
 * uchwyty, więc wystarczy że wskazujemy na nasz własny obiekt. */
struct SwitchPad
{
	int index;                             /* 0..3 */
	int inUse;                             /* czy SDL_OpenGamepad go otworzył */
	PadState pad;                          /* libnx stan pada */
	HidNpadIdType npadId;                  /* HidNpadIdType_No1.. / Handheld */
	HidVibrationDeviceHandle vibe[2];      /* uchwyty wibracji (L/R) */
	int vibeCount;
	int vibeReady;
};

/* SDL_Gamepad i SDL_Joystick to nieprzezroczyste struktury w nagłówku SDL3;
 * w naszym shimie oba wskazują na ten sam obiekt SwitchPad. */
struct SDL_Gamepad
{
	struct SwitchPad *impl;
};
struct SDL_Joystick
{
	struct SwitchPad *impl;
};

static struct SwitchPad s_pads[NATIVE_INPUT_SWITCH_MAX_PADS];
static struct SDL_Gamepad s_gamepadHandles[NATIVE_INPUT_SWITCH_MAX_PADS];
static struct SDL_Joystick s_joystickHandles[NATIVE_INPUT_SWITCH_MAX_PADS];
static int s_initialised = 0;

/* SDL_JoystickID (Uint32) <-> index. native_input.c inicjalizuje instanceId na
 * -1 jako "brak", więc używamy id = index + 1 (nigdy 0). */
#define SWITCH_ID_FROM_INDEX(i) ((SDL_JoystickID)((i) + 1))
#define SWITCH_INDEX_FROM_ID(id) ((int)(id) - 1)

static HidNpadIdType NativeInputSwitch_NpadIdForIndex(int index)
{
	switch (index)
	{
	case 0:
		return HidNpadIdType_No1;
	case 1:
		return HidNpadIdType_No2;
	case 2:
		return HidNpadIdType_No3;
	case 3:
		return HidNpadIdType_No4;
	default:
		return HidNpadIdType_No1;
	}
}

static void NativeInputSwitch_EnsureInit(void)
{
	int i;

	if (s_initialised != 0)
	{
		return;
	}

	memset(s_pads, 0, sizeof(s_pads));

	/* Do 4 graczy + tryb handheld. padConfigureInput wołane raz. */
	padConfigureInput(NATIVE_INPUT_SWITCH_MAX_PADS, HidNpadStyleSet_NpadStandard);

	for (i = 0; i < NATIVE_INPUT_SWITCH_MAX_PADS; i++)
	{
		s_pads[i].index = i;
		s_pads[i].inUse = 0;
		s_pads[i].vibeReady = 0;
		s_pads[i].vibeCount = 0;
		s_gamepadHandles[i].impl = &s_pads[i];
		s_joystickHandles[i].impl = &s_pads[i];

		/* Gracz 0 czyta z No1 ORAZ z trybu handheld (pojedyncza konsola bez
		 * dokowania). Pozostali gracze tylko z odpowiedniego No*. */
		if (i == 0)
		{
			padInitializeWithMask(&s_pads[i].pad,
				(1UL << HidNpadIdType_No1) | (1UL << HidNpadIdType_Handheld));
			s_pads[i].npadId = HidNpadIdType_No1;
		}
		else
		{
			padInitializeWithMask(&s_pads[i].pad, (1UL << (u64)NativeInputSwitch_NpadIdForIndex(i)));
			s_pads[i].npadId = NativeInputSwitch_NpadIdForIndex(i);
		}
	}

	s_initialised = 1;

	/* Pierwszy padUpdate PRZED jakimkolwiek padIsConnected/enumeracją - inaczej
	 * padIsConnected zwróci false i NativeInput_OpenKnownControllers() nie
	 * otworzyłoby żadnego pada podłączonego już przy starcie. */
	for (i = 0; i < NATIVE_INPUT_SWITCH_MAX_PADS; i++)
	{
		padUpdate(&s_pads[i].pad);
	}
}

void NativeInputSwitch_PumpPads(void)
{
	int i;

	if (s_initialised == 0)
	{
		return;
	}

	for (i = 0; i < NATIVE_INPUT_SWITCH_MAX_PADS; i++)
	{
		padUpdate(&s_pads[i].pad);
	}
}

/* -------------------------------------------------------------------------- */
/* SDL subsystem / lifecycle                                                  */
/* -------------------------------------------------------------------------- */

bool SDL_InitSubSystem(SDL_InitFlags flags)
{
	(void)flags;
	NativeInputSwitch_EnsureInit();
	/* SDL3 zwraca true przy sukcesie; native_input.c sprawdza == 0 jako błąd. */
	return true;
}

void SDL_QuitSubSystem(SDL_InitFlags flags)
{
	(void)flags;
	/* PadState nie wymaga jawnego zwolnienia; zostawiamy do reinicjalizacji. */
}

bool SDL_AddGamepadMappingsFromFile(const char *file)
{
	/* Switch nie używa gamecontrollerdb.txt - mapowanie jest natywne (libnx). */
	(void)file;
	return true;
}

/* -------------------------------------------------------------------------- */
/* Enumeracja / otwieranie                                                    */
/* -------------------------------------------------------------------------- */

SDL_JoystickID *SDL_GetGamepads(int *count)
{
	SDL_JoystickID *ids;
	int found = 0;
	int i;

	NativeInputSwitch_EnsureInit();

	/* Alokujemy z zapasem na wszystkie sloty (+1 na zakończenie zerowe, tak jak
	 * robi to SDL3). native_input.c zwalnia to przez SDL_free(). */
	ids = (SDL_JoystickID *)malloc(sizeof(SDL_JoystickID) * (NATIVE_INPUT_SWITCH_MAX_PADS + 1));
	if (ids == NULL)
	{
		if (count != NULL)
		{
			*count = 0;
		}
		return NULL;
	}

	for (i = 0; i < NATIVE_INPUT_SWITCH_MAX_PADS; i++)
	{
		if (padIsConnected(&s_pads[i].pad))
		{
			ids[found++] = SWITCH_ID_FROM_INDEX(i);
		}
	}
	ids[found] = 0;

	if (count != NULL)
	{
		*count = found;
	}
	return ids;
}

bool SDL_IsGamepad(SDL_JoystickID instance_id)
{
	int index = SWITCH_INDEX_FROM_ID(instance_id);

	NativeInputSwitch_EnsureInit();

	if ((index < 0) || (index >= NATIVE_INPUT_SWITCH_MAX_PADS))
	{
		return false;
	}
	return padIsConnected(&s_pads[index].pad);
}

SDL_Gamepad *SDL_OpenGamepad(SDL_JoystickID instance_id)
{
	int index = SWITCH_INDEX_FROM_ID(instance_id);

	NativeInputSwitch_EnsureInit();

	if ((index < 0) || (index >= NATIVE_INPUT_SWITCH_MAX_PADS))
	{
		return NULL;
	}

	s_pads[index].inUse = 1;

	/* Inicjalizacja uchwytów wibracji dla tego gracza (leniwie, 2 handles). */
	if (s_pads[index].vibeReady == 0)
	{
		Result rc = hidInitializeVibrationDevices(
			s_pads[index].vibe, 2, s_pads[index].npadId, HidNpadStyleSet_NpadStandard);
		if (R_SUCCEEDED(rc))
		{
			s_pads[index].vibeCount = 2;
			s_pads[index].vibeReady = 1;
		}
		else
		{
			s_pads[index].vibeCount = 0;
			s_pads[index].vibeReady = 1; /* nie próbuj ponownie co klatkę */
		}
	}

	return &s_gamepadHandles[index];
}

void SDL_CloseGamepad(SDL_Gamepad *gamepad)
{
	if ((gamepad != NULL) && (gamepad->impl != NULL))
	{
		gamepad->impl->inUse = 0;
	}
}

bool SDL_GamepadConnected(SDL_Gamepad *gamepad)
{
	if ((gamepad == NULL) || (gamepad->impl == NULL))
	{
		return false;
	}
	return padIsConnected(&gamepad->impl->pad);
}

SDL_Joystick *SDL_GetGamepadJoystick(SDL_Gamepad *gamepad)
{
	if ((gamepad == NULL) || (gamepad->impl == NULL))
	{
		return NULL;
	}
	return &s_joystickHandles[gamepad->impl->index];
}

SDL_JoystickID SDL_GetJoystickID(SDL_Joystick *joystick)
{
	if ((joystick == NULL) || (joystick->impl == NULL))
	{
		return 0;
	}
	return SWITCH_ID_FROM_INDEX(joystick->impl->index);
}

/* -------------------------------------------------------------------------- */
/* Odczyt przycisków / osi                                                    */
/* -------------------------------------------------------------------------- */

bool SDL_GetGamepadButton(SDL_Gamepad *gamepad, SDL_GamepadButton button)
{
	u64 held;

	if ((gamepad == NULL) || (gamepad->impl == NULL))
	{
		return false;
	}

	held = padGetButtons(&gamepad->impl->pad);

	switch (button)
	{
	/* Pozycyjne mapowanie twarzowych przycisków na layout Nintendo. */
	case SDL_GAMEPAD_BUTTON_SOUTH:
		return (held & HidNpadButton_B) != 0;
	case SDL_GAMEPAD_BUTTON_EAST:
		return (held & HidNpadButton_A) != 0;
	case SDL_GAMEPAD_BUTTON_WEST:
		return (held & HidNpadButton_Y) != 0;
	case SDL_GAMEPAD_BUTTON_NORTH:
		return (held & HidNpadButton_X) != 0;

	case SDL_GAMEPAD_BUTTON_BACK:
		return (held & HidNpadButton_Minus) != 0;
	case SDL_GAMEPAD_BUTTON_START:
		return (held & HidNpadButton_Plus) != 0;

	case SDL_GAMEPAD_BUTTON_LEFT_STICK:
		return (held & HidNpadButton_StickL) != 0;
	case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
		return (held & HidNpadButton_StickR) != 0;

	case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
		return (held & HidNpadButton_L) != 0;
	case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
		return (held & HidNpadButton_R) != 0;

	case SDL_GAMEPAD_BUTTON_DPAD_UP:
		return (held & HidNpadButton_Up) != 0;
	case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
		return (held & HidNpadButton_Down) != 0;
	case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
		return (held & HidNpadButton_Left) != 0;
	case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
		return (held & HidNpadButton_Right) != 0;

	default:
		return false;
	}
}

Sint16 SDL_GetGamepadAxis(SDL_Gamepad *gamepad, SDL_GamepadAxis axis)
{
	HidAnalogStickState stick;
	u64 held;

	if ((gamepad == NULL) || (gamepad->impl == NULL))
	{
		return 0;
	}

	switch (axis)
	{
	case SDL_GAMEPAD_AXIS_LEFTX:
		stick = padGetStickPos(&gamepad->impl->pad, 0);
		return (Sint16)stick.x;
	case SDL_GAMEPAD_AXIS_LEFTY:
		/* libnx: +Y = w górę; SDL: +Y = w dół -> negujemy. */
		stick = padGetStickPos(&gamepad->impl->pad, 0);
		return (Sint16)(-stick.y);
	case SDL_GAMEPAD_AXIS_RIGHTX:
		stick = padGetStickPos(&gamepad->impl->pad, 1);
		return (Sint16)stick.x;
	case SDL_GAMEPAD_AXIS_RIGHTY:
		stick = padGetStickPos(&gamepad->impl->pad, 1);
		return (Sint16)(-stick.y);

	/* Switch nie ma analogowych ZL/ZR - cyfrowe: 0 albo max. */
	case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
		held = padGetButtons(&gamepad->impl->pad);
		return (held & HidNpadButton_ZL) != 0 ? (Sint16)32767 : (Sint16)0;
	case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
		held = padGetButtons(&gamepad->impl->pad);
		return (held & HidNpadButton_ZR) != 0 ? (Sint16)32767 : (Sint16)0;

	default:
		return 0;
	}
}

/* -------------------------------------------------------------------------- */
/* Wibracje                                                                   */
/* -------------------------------------------------------------------------- */

bool SDL_RumbleGamepad(SDL_Gamepad *gamepad, Uint16 low_frequency_rumble,
	Uint16 high_frequency_rumble, Uint32 duration_ms)
{
	HidVibrationValue values[2];
	float ampLow;
	float ampHigh;
	int i;

	(void)duration_ms; /* libnx wibruje aż do kolejnej wartości; brak timera. */

	if ((gamepad == NULL) || (gamepad->impl == NULL))
	{
		return false;
	}
	if ((gamepad->impl->vibeReady == 0) || (gamepad->impl->vibeCount <= 0))
	{
		return false;
	}

	ampLow = (float)low_frequency_rumble / 65535.0f;
	ampHigh = (float)high_frequency_rumble / 65535.0f;

	for (i = 0; i < gamepad->impl->vibeCount; i++)
	{
		values[i].amp_low = ampLow;
		values[i].freq_low = 160.0f;   /* typowe wartości bazowe Joy-Con */
		values[i].amp_high = ampHigh;
		values[i].freq_high = 320.0f;
	}

	hidSendVibrationValues(gamepad->impl->vibe, values, gamepad->impl->vibeCount);
	return true;
}

/* -------------------------------------------------------------------------- */
/* Klawiatura (brak na Switchu) - native_input.c woła to raz w Platform_InputInit */
/* -------------------------------------------------------------------------- */

const bool *SDL_GetKeyboardState(int *numkeys)
{
	/* Zwracamy stabilny, wyzerowany bufor - żaden klawisz nigdy nie wciśnięty.
	 * SDL_SCANCODE_COUNT jest zdefiniowane w SDL3 (SDL_scancode.h). */
	static bool s_keys[SDL_SCANCODE_COUNT];

	if (numkeys != NULL)
	{
		*numkeys = SDL_SCANCODE_COUNT;
	}
	return s_keys;
}

#endif /* __SWITCH__ */
