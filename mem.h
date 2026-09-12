#ifndef __MEM_H__
#define __MEM_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif

_VOID heap_clear(_VOID);
_VOID *heap_store(_VOID *ptr, size_t len);

_VOID stack_clear(_VOID);
_VOID *stack_store(_VOID *ptr, size_t len);

_VOID mem_freeall(_VOID);
_BOOL mem_alloc(_WORD max_x, _WORD max_y, _WORD max_pice, _WORD max_mem);

#endif /* __MEM_H__ */
