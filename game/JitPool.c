#include <common.h>


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80030fdc-0x8003105c.
void JitPool_Clear(struct JitPool *AP)
{
	uintptr_t currSlot = (uintptr_t)AP->ptrPoolData;

	// clear list of free and taken
	LIST_Clear(&AP->free);
	LIST_Clear(&AP->taken);

	for (s32 loopIndex = 0; loopIndex < AP->maxItems; loopIndex++)
	{
		// add all pool items to the free list
		LIST_AddFront(&AP->free, (struct Item *)currSlot);

		currSlot += JITPOOL_ALIGN_ITEM_STRIDE(AP->itemSize);
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003105c-0x800310d4.
void JitPool_Init(struct JitPool *AP, int maxItems, int itemSize, char *name)
{
	(void)name;

	memset(AP, 0, sizeof(struct JitPool));
	AP->maxItems = maxItems;
	AP->itemSize = itemSize;
	AP->poolSize = maxItems * itemSize;
	AP->ptrPoolData = MEMPACK_AllocMem(AP->poolSize);
	JitPool_Clear(AP);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800310d4-0x8003112c.
// NOTE: Retail returns a 32-bit RAM address here (return (s32)item), which
// callers immediately cast back to a struct pointer. That round-trip truncates
// real 64-bit pointers on Switch (AArch64/LP64). All three call sites already
// cast the result to the destination pointer type, so returning the pointer
// directly (instead of an (s32) address) is a no-op change for every existing
// platform and fixes the truncation on Switch.
struct Item *JitPool_Add(struct JitPool *AP)
{
	struct Item *item = AP->free.first;

	if (item != 0)
	{
		LIST_RemoveMember(&AP->free, item);
		LIST_AddFront(&AP->taken, item);
	}

	return item;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003112c-0x8003116c.
void JitPool_Remove(struct JitPool *AP, struct Item *item)
{
	LIST_RemoveMember(&AP->taken, item);
	LIST_AddFront(&AP->free, item);
}
