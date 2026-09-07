#include <common.h>


// NOTE(aalhendi): ASM-verified NTSC-U 926 PS1 path 0x8003e740-0x8003e80c; CTR_NATIVE uses host RAM.
void MEMPACK_Init(int ramSize)
{
	(void)ramSize;

#if defined(CTR_NATIVE)

	s32 packSize;
	const struct PlatformMempackArena *arena = Platform_InitMempackArena();

	// NOTE: keep the arena start as a real pointer end-to-end instead of
	// round-tripping through (u32) - that cast truncated real 64-bit
	// pointers on Switch (AArch64/LP64). On 32-bit hosts (PC/Vita) this is a
	// no-op change (the pointer already fit in 32 bits).
	void *startAddr = (void *)arena->start;
	packSize = arena->size;

	printf("[CTR] MEMPACK native backing: base=%p\n", arena->base);

	MEMPACK_NewPack(startAddr, packSize);
	sdata->PtrMempack->endOfAllocator = (u8 *)startAddr + packSize;
	sdata->PtrMempack->endOfMemory = arena->endOfMemory;

	printf("[CTR] MEMPACK native arena: start=%p size=%08x end=%p\n", startAddr, packSize, sdata->PtrMempack->endOfAllocator);

#else

	u32 startPtr;
	s32 packSize;

	maxOverlayEnd = (u32)AH_EndOfFile;
	if (maxOverlayEnd < (u32)RB_EndOfFile)
		maxOverlayEnd = (u32)RB_EndOfFile;
	if (maxOverlayEnd < (u32)MM_EndOfFile)
		maxOverlayEnd = (u32)MM_EndOfFile;
	if (maxOverlayEnd < (u32)CS_EndOfFile)
		maxOverlayEnd = (u32)CS_EndOfFile;

	startPtr = (u32)OVR_Region3 + (((maxOverlayEnd - (u32)OVR_Region3) + MEMPACK_PS1_OVERLAY_ALIGNMENT_MASK) & ~MEMPACK_PS1_OVERLAY_ALIGNMENT_MASK);
	packSize = ramSize - (int)(startPtr & MEMPACK_PS1_RAM_ADDRESS_MASK) - MEMPACK_PS1_END_GUARD_SIZE;

	ptrMempack = sdata->PtrMempack;
	ptrMempack->start = (void *)startPtr;
	ptrMempack->endOfAllocator = (void *)(startPtr + packSize);
	ptrMempack->lastFreeByte = (void *)(startPtr + packSize);
	ptrMempack->packSize = packSize;
	ptrMempack->numBookmarks = 0;
	ptrMempack->endOfMemory = (void *)MEMPACK_PS1_END_OF_MEMORY;
	ptrMempack->firstFreeByte = (void *)startPtr;
#endif
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003e80c-0x8003e830.
void MEMPACK_SwapPacks(int index)
{
	sdata->PtrMempack = &sdata->mempack[index];
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003e830-0x8003e85c.
void MEMPACK_NewPack(void *start, int size)
{
	struct Mempack *ptrMempack = sdata->PtrMempack;
	// NOTE: (u32)start truncates real pointers on 64-bit (Switch/AArch64);
	// route the offset through a byte pointer instead (identical addresses on
	// 32-bit platforms).
	void *end = (void *)((u8 *)start + size);

	ptrMempack->packSize = size;
	ptrMempack->start = start;
	ptrMempack->lastFreeByte = end;
	ptrMempack->endOfMemory = end;
	ptrMempack->firstFreeByte = start;
	ptrMempack->numBookmarks = 0;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003e85c-0x8003e874.
int MEMPACK_GetFreeBytes()
{
	struct Mempack *ptrMempack = sdata->PtrMempack;

	// NOTE: (u32) casts here happened to be safe even on 64-bit (both pointers
	// share the same upper bits within the same backing buffer), but real
	// pointer subtraction is clearer and portable to any address.
	return (int)((u8 *)ptrMempack->lastFreeByte - (u8 *)ptrMempack->firstFreeByte);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003e874-0x8003e8e8.
void *MEMPACK_AllocMem(int allocSize)
{
	struct Mempack *ptrMempack = sdata->PtrMempack;

	if (MEMPACK_GetFreeBytes() < allocSize)
	{
		CTR_ErrorScreen(0xFF, 0, 0);
		for (;;)
		{
		}
	}

	s32 newAllocSize = MEMPACK_ALIGN_SIZE(allocSize);
	ptrMempack->sizeOfPrevAllocation = newAllocSize;

	// NOTE: (s32) truncates real pointers on 64-bit (Switch/AArch64); keep the
	// bump-allocator pointer arithmetic in pointer form instead.
	void *firstFreeByte = ptrMempack->firstFreeByte;
	ptrMempack->firstFreeByte = (u8 *)firstFreeByte + newAllocSize;

	return firstFreeByte;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003e8e8-0x8003e938.
void *MEMPACK_AllocHighMem(int allocSize)
{
	while (MEMPACK_GetFreeBytes() < allocSize)
	{
	}

	allocSize = MEMPACK_ALIGN_SIZE(allocSize);
	sdata->PtrMempack->sizeOfPrevAllocation = allocSize;

	// NOTE: (s32) truncates real pointers on 64-bit (Switch/AArch64); keep the
	// bump-allocator pointer arithmetic in pointer form instead.
	void *newLastFreeByte = (u8 *)sdata->PtrMempack->lastFreeByte - allocSize;
	sdata->PtrMempack->lastFreeByte = newLastFreeByte;

	return newLastFreeByte;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003e938-0x8003e94c.
void MEMPACK_ClearHighMem()
{
	sdata->PtrMempack->lastFreeByte = sdata->PtrMempack->endOfAllocator;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003e94c-0x8003e978.
void *MEMPACK_ReallocMem(int allocSize)
{
	struct Mempack *ptrMempack = sdata->PtrMempack;

	s32 newAllocSize = MEMPACK_ALIGN_SIZE(allocSize);
	// NOTE: (s32) truncates real pointers on 64-bit (Switch/AArch64); keep the
	// bump-allocator pointer arithmetic in pointer form instead.
	ptrMempack->firstFreeByte = (u8 *)ptrMempack->firstFreeByte - ptrMempack->sizeOfPrevAllocation + newAllocSize;
	ptrMempack->sizeOfPrevAllocation = newAllocSize;

	return ptrMempack->firstFreeByte;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003e978-0x8003e9b8.
int MEMPACK_PushState()
{
	struct Mempack *ptrMempack = sdata->PtrMempack;
	s32 numBookmarks = ptrMempack->numBookmarks;
	if (numBookmarks < MEMPACK_BOOKMARK_COUNT)
	{
		ptrMempack->bookmarks[numBookmarks] = ptrMempack->firstFreeByte;
		ptrMempack->numBookmarks++;
	}

	return numBookmarks;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003e9b8-0x8003e9d0.
void MEMPACK_ClearLowMem()
{
	struct Mempack *ptrMempack = sdata->PtrMempack;

	ptrMempack->numBookmarks = 0;
	ptrMempack->firstFreeByte = ptrMempack->start;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003e9d0-0x8003ea08.
void MEMPACK_PopState()
{
	struct Mempack *ptrMempack = sdata->PtrMempack;
	s32 numBookmarks = ptrMempack->numBookmarks;
	if (numBookmarks > 0)
	{
		numBookmarks--;
		ptrMempack->firstFreeByte = ptrMempack->bookmarks[numBookmarks];
		ptrMempack->numBookmarks = numBookmarks;
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003ea08-0x8003ea28.
void MEMPACK_PopToState(int id)
{
	struct Mempack *ptrMempack = sdata->PtrMempack;

	ptrMempack->numBookmarks = id;
	ptrMempack->firstFreeByte = ptrMempack->bookmarks[id];
}
