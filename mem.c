#include <mem.h>
#include <pice.h>
#include <map.h>
#include <ro_mem.h>

#define ODD(x) ((x)&1)
#define EVEN(x) (!ODD(x))

LOCAL _UBYTE *Mem = NULL;
LOCAL size_t Mem_size = 0;

LOCAL _UBYTE *Heap_ptr;
LOCAL _UBYTE *Stack_ptr;


/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL _VOID heap_clear(_VOID)
{
	Heap_ptr = Mem;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID *heap_store(_VOID *ptr, size_t len)
{
	_VOID *mem;
	
	if (Heap_ptr+len < Stack_ptr)
	{
		if (ODD((_ULONG)Heap_ptr))
		{
			Heap_ptr++;
		}
		mem = Heap_ptr;
		MemMove(mem, ptr, len);
		Heap_ptr += len;
		return mem;
	}
	return NULL;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID stack_clear(_VOID)
{
	Stack_ptr = Mem + Mem_size;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID *stack_store(_VOID *ptr, size_t len)
{
	if (Stack_ptr - len > Heap_ptr)
	{
		Stack_ptr -= len;
		
		if (ODD((_ULONG)Stack_ptr))
		{
			Stack_ptr--;
		}
		memmove(Stack_ptr, ptr, len);
		return Stack_ptr;
	}
	return NULL;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID mem_freeall(_VOID)
{
	map_exit();
	pice_exit();
	
	if (Mem != NULL)
	{
		SFREE(Mem);
		Mem = NULL;
	}
	Mem_size = 0;
	
	heap_clear();
	stack_clear();
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL mem_alloc(_WORD max_x, _WORD max_y, _WORD max_pice, _WORD max_mem)
{
	_ULONG size;
	
	size = sizeof(_UBYTE) * (size_t)max_mem * 1024L;
	mem_freeall();
	if (map_init(max_x, max_y) == FALSE ||
		pice_init(max_pice) == FALSE ||
		(size_t)size != size ||
		(Mem = MALLOC(size, "mem_alloc: mem")) == NULL
	   )
	{
		mem_freeall();
		return FALSE;
	}
	Mem_size = (size_t)size;
	return TRUE;
}
