#include <platform.h>
#include "ctr_scratchpad.h"
#include "platform/native_memory.h"
#if defined(CTR_INTERNAL)
#include "platform/native_checkpoint.h"
#endif

#include <common.h>
#include <macros.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CTR_NATIVE_MEMPACK_RETAIL_PRESSURE
#define CTR_NATIVE_MEMPACK_RETAIL_PRESSURE 1
#endif

// TODO(aalhendi): Re-audit LOAD_ReadFile_ex, LOAD_DramFileCallback, LEV/PTR
// callbacks, hub swapping, MEMPACK size arithmetic + PSX shaped ptr storage before removing the expanded arena escape hatch
#if CTR_NATIVE_MEMPACK_RETAIL_PRESSURE
// NOTE(aalhendi): Retail pressure mode exposes the NTSC-U 926 mempack window
// inside a 2 MiB backing store.
#define CTR_NATIVE_MEMPACK_BUFFER_SIZE  0x200000u
#define CTR_NATIVE_MEMPACK_START_OFFSET 0xba9f0u
#define CTR_NATIVE_MEMPACK_SIZE         0x144e10u
#else
#define CTR_NATIVE_MEMPACK_BUFFER_SIZE  (8u * 1024u * 1024u)
#define CTR_NATIVE_MEMPACK_START_OFFSET 0u
#define CTR_NATIVE_MEMPACK_SIZE         CTR_NATIVE_MEMPACK_BUFFER_SIZE
#endif

union NativeScratchpadStorage
{
	u8 bytes[CTR_SCRATCHPAD_SIZE];
	u32 words[CTR_SCRATCHPAD_SIZE / sizeof(u32)];
};

CTR_STATIC_ASSERT(sizeof(union NativeScratchpadStorage) == CTR_SCRATCHPAD_SIZE);

#if defined(__SWITCH__)
// NOTE(aalhendi): Retail's Load/PtrMap fixups (LOAD_RunPtrMap and friends)
// store every fixed-up "pointer" inside loaded file data (models, levels,
// instances, ...) as a 4-byte "retail 32-bit RAM address" - this is the
// on-disk/in-memory format and cannot be widened without corrupting
// adjacent data or shifting struct layouts relative to the file bytes.
// On 64-bit Switch, those 4-byte slots are computed by truncating a REAL
// host pointer (this arena's base) to its low 32 bits. That truncation is
// exactly correct - and fully reversible - IF this arena is guaranteed to
// never straddle a 4GiB boundary, because then every address inside the
// arena shares the same upper 32 bits, which we cache once here and splice
// back onto any 4-byte slot value to reconstruct the real pointer (see
// NativeMempack_ReconstructPointer below). Guaranteeing no 4GiB-boundary
// crossing only requires the arena to be aligned to its own size (a power
// of two): if the arena's base address is a multiple of its size S, and S
// evenly divides 2^32 (true for any power-of-two S <= 4GiB), then
// (base mod 2^32) is itself a multiple of S, so (base mod 2^32) + S <= 2^32
// - the whole arena's low-32-bits range never wraps. This avoids needing
// any privileged/kernel-level control over where the memory lands (which
// is not available to homebrew - svcMapMemory can only target the Stack
// region on 2.0.0+), and only requires a self-aligned heap allocation.
global_variable u8 *s_mempackMemory;
global_variable void *s_mempackRawAlloc;
global_variable uintptr_t s_mempackArenaHighBits;

static u8 *NativeMemory_AllocAlignedMempackBuffer(size_t size)
{
	void *raw;
	uintptr_t rawAddr;
	uintptr_t alignedAddr;

	// Over-allocate by `size` so an aligned block of `size` bytes is always
	// found inside [raw, raw + size + size). `size` is a power of two
	// (CTR_NATIVE_MEMPACK_BUFFER_SIZE == 0x200000), so a plain mask works.
	raw = malloc(size + size);
	if (raw == NULL)
	{
		return NULL;
	}

	rawAddr = (uintptr_t)raw;
	alignedAddr = (rawAddr + (size - 1)) & ~(uintptr_t)(size - 1);

	// Intentionally never freed: this backing store lives for the entire
	// process lifetime (same as the old static array it replaces).
	s_mempackRawAlloc = raw;
	return (u8 *)alignedAddr;
}

void *NativeMempack_ReconstructPointer(u32 slotValue)
{
	if (slotValue == 0)
	{
		return NULL;
	}

	return (void *)(s_mempackArenaHighBits | (uintptr_t)slotValue);
}

u32 NativeMempack_TruncatePointer(const void *ptr)
{
	// NOTE: intentionally narrows - callers store this into a retail 4-byte
	// slot. Safe as long as `ptr` lives inside the (4GiB-boundary-safe)
	// mempack arena; NativeMempack_ReconstructPointer reverses it exactly.
	return (u32)(uintptr_t)ptr;
}
#else
global_variable char s_mempackMemory[CTR_NATIVE_MEMPACK_BUFFER_SIZE];
#endif
global_variable struct PlatformMempackArena s_mempackArena;
global_variable union NativeScratchpadStorage s_scratchpadMemory;
u8 *gCTRNativeScratchpadBase;

void Platform_InitScratchpad(void)
{
#if defined(CTR_NATIVE)
	gCTRNativeScratchpadBase = &s_scratchpadMemory.bytes[0];
	memset(&s_scratchpadMemory, 0, sizeof(s_scratchpadMemory));
#endif
}

void Platform_ConfigureMempackArena(void)
{
	s_mempackArena.base = &s_mempackMemory[0];
	s_mempackArena.start = &s_mempackMemory[CTR_NATIVE_MEMPACK_START_OFFSET];
	s_mempackArena.endOfMemory = &s_mempackMemory[CTR_NATIVE_MEMPACK_BUFFER_SIZE];
	s_mempackArena.size = CTR_NATIVE_MEMPACK_SIZE;
	s_mempackArena.backingSize = CTR_NATIVE_MEMPACK_BUFFER_SIZE;
}

const struct PlatformMempackArena *Platform_InitMempackArena(void)
{
#if defined(__SWITCH__)
	if (s_mempackMemory == NULL)
	{
		s_mempackMemory = NativeMemory_AllocAlignedMempackBuffer(CTR_NATIVE_MEMPACK_BUFFER_SIZE);
		if (s_mempackMemory == NULL)
		{
			fprintf(stderr, "[CTR Native] FATAL: failed to allocate %u-byte aligned mempack arena\n", (unsigned)CTR_NATIVE_MEMPACK_BUFFER_SIZE);
			abort();
		}

		// Cache the arena's upper address bits now, once, while we still
		// have the real pointer. See the NOTE above s_mempackMemory for why
		// this alignment guarantees a single, unchanging high-bits value
		// for every address inside the arena.
		s_mempackArenaHighBits = (uintptr_t)s_mempackMemory & ~(uintptr_t)0xffffffffu;
		printf("[CTR Native] MEMPACK arena allocated at %p (aligned to 0x%x, high bits 0x%llx)\n", (void *)s_mempackMemory,
		       (unsigned)CTR_NATIVE_MEMPACK_BUFFER_SIZE, (unsigned long long)s_mempackArenaHighBits);
	}
#endif
	memset(s_mempackMemory, 0, CTR_NATIVE_MEMPACK_BUFFER_SIZE);
	Platform_ConfigureMempackArena();
#if defined(CTR_INTERNAL)
	NativeCheckpoint_OnMempackArenaReset();
#endif

	return &s_mempackArena;
}

const struct PlatformMempackArena *Platform_GetMempackArena(void)
{
	return &s_mempackArena;
}

void *Platform_GetMempackBacking(void)
{
	return &s_mempackMemory[0];
}

int Platform_GetMempackBackingSize(void)
{
	return (int)CTR_NATIVE_MEMPACK_BUFFER_SIZE;
}

void Platform_RepairResidentPointers(s32 activeMempackIndex)
{
	u32 voiceSetIndex;

	if ((activeMempackIndex < 0) || (activeMempackIndex >= 4))
	{
		activeMempackIndex = 0;
	}

	// NOTE(aalhendi): Native keeps retail-shaped global data, but pointer aliases
	// must target this process's static storage. This also moves GCC's
	// initializer-only memcard helper global out of the live state graph so
	// checkpoints capture the actual memcard buffer.
	sdata = &sdata_static;
	sdata_static.gGT = &sdata_static.gameTracker;
	sdata_static.gGamepads = &sdata_static.gamepadSystem;
	sdata_static.PtrMempack = &sdata_static.mempack[activeMempackIndex];
	sdata_static.ptrToMemcardBuffer1 = &sdata_static.memcardBytes[0];
	sdata_static.ptrToMemcardBuffer2 = &sdata_static.memcardBytes[0];

	// NOTE: data.voiceSetPtr[] used to be a compile-time static initializer
	// (address of each voiceData[i].voiceSet[0] cast to (int)), which is not
	// a valid constant expression once `data` is addressed with real 64-bit
	// pointers (Switch/AArch64). Repair it here instead, at the same point
	// the other resident-pointer aliases above are repaired. Consumed only by
	// the checkpoint/relocation system (platform/native_checkpoint.c); stored
	// as a 32-bit slot (matches retail layout / checkpoint region sizing) via
	// NativeCheckpoint_WriteU32Slot-equivalent truncation, which is safe here
	// because this field is itself relocated (not dereferenced) by that code.
	for (voiceSetIndex = 0; voiceSetIndex < len(data.voiceSetPtr); voiceSetIndex++)
	{
		data.voiceSetPtr[voiceSetIndex] = (int)(u32)(uintptr_t)&data.voiceData[voiceSetIndex].voiceSet[0];
	}
}
