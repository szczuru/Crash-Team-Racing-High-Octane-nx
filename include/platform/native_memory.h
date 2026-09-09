#ifndef PLATFORM_NATIVE_MEMORY_H
#define PLATFORM_NATIVE_MEMORY_H

#include <macros.h>

void Platform_ConfigureMempackArena(void);
void Platform_RepairResidentPointers(s32 activeMempackIndex);
void *Platform_GetMempackBacking(void);
int Platform_GetMempackBackingSize(void);

#if defined(__SWITCH__)
// NOTE(aalhendi): See the long comment above s_mempackMemory in
// native_memory.c. The mempack arena is allocated self-aligned to its own
// (power-of-two) size, which guarantees it never straddles a 4GiB boundary,
// which in turn guarantees every address inside it shares the same upper
// 32 bits. That makes it safe to truncate any pointer into the arena down
// to its low 32 bits (matching retail's 4-byte "RAM address" slot format
// used throughout loaded file data - models, levels, instances, pointer
// maps, etc.) and reconstruct the exact original pointer later by splicing
// the cached upper bits back on. Use these two functions at any site that
// currently does a raw (u32)/(int) truncation or a raw (uintptr_t)/(u64)
// zero-extension of a pointer that is known to live inside the mempack
// arena, instead of the raw cast.
void *NativeMempack_ReconstructPointer(u32 slotValue);
u32 NativeMempack_TruncatePointer(const void *ptr);
#endif

#endif
