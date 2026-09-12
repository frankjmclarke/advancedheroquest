/*****************************************************************************
 * RO_MEM.H
 *****************************************************************************/

#ifndef __RO_MEM_H__
#define __RO_MEM_H__

#ifndef __PORTAB_H__
#  include <portab.h>
#endif
#include <string.h>

#ifndef DEBUG_ALLOC
#  define DEBUG_ALLOC 2
#endif

typedef _VOID (*ERR_MEM_FUNC)(_ULONG size);
ERR_MEM_FUNC set_err_memory(ERR_MEM_FUNC p_err);
_VOID err_memory(_ULONG size);

#define MEM_FIXED		0x0001
#define MEM_MOVEABLE	0x0002
#define MEM_NOCOMPACT	0x0010
#define MEM_NODISCARD	0x0020
#define MEM_ZEROINIT	0x0040
#define MEM_MODIFY		0x0080
#define MEM_DISCARDABLE 0x0100
#define MEM_SYSTEM		0x0200
#define MEM_NOT_BANKED	0x1000
#define MEM_SHARABLE	0x2000
#define MEM_NOTIFY		0x4000
#define MEM_NOERR		0x8000U

_LONG mem_max(_VOID);

_VOID mem_test_start(_VOID);
_VOID mem_test_end(_VOID);
_VOID mem_test_ignore(_VOID);


#undef MALLOC
#undef CALLOC
#undef STRDUP
#undef NEW
#undef FREE
#undef SFREE
#undef OFREE
#undef REALLOC

_VOID *mem_debug_get(_ULONG size, CONST _UBYTE *from);
_VOID *mem_debug_get_system(_ULONG size, _UWORD type, CONST _UBYTE *from);
_VOID mem_debug_free(_VOID *block);
_VOID *mem_debug_0get(_ULONG size, CONST _UBYTE *from);
_VOID *mem_debug_0get_system(_ULONG size, _UWORD type, CONST _UBYTE *from);
_UBYTE *mem_debug_str_dup(CONST _UBYTE *str, CONST _UBYTE *from);
_VOID *mem_debug_realloc(_VOID *block, _ULONG newsize, _ULONG oldsize, CONST _UBYTE *from);
_BOOL mem_debug_check(_VOID *block, CONST _UBYTE *where);

_VOID *mem_get(_ULONG size);
_VOID *mem_get_system(_ULONG size, _UWORD type);
_VOID mem_free(_VOID *p);
_VOID *mem_0get(_ULONG size);
_VOID *mem_0get_system(_ULONG size, _UWORD type);
_UBYTE *mem_str_dup(CONST _UBYTE *str);


#if DEBUG_ALLOC

#define MALLOC(size, from) mem_debug_get((_ULONG)(size), from)
#define SYSMALLOC(size, type, from) mem_debug_get_system((_ULONG)(size), type, from)
#define CALLOC(nitems, size, from) mem_debug_0get((_ULONG)(nitems) * (_ULONG)(size), from)
#define M0ALLOC(size, from) mem_debug_0get((_ULONG)(size), from)
#define STRDUP(str, from) mem_debug_str_dup(str, from)
#define mem_free mem_debug_free
#define FREE(block, size) mem_debug_free(block)
#define SFREE(str) FREE(str, -1)
#define MEMCHECK(ptr, where) mem_debug_check(ptr, where)
#if DEBUG_ALLOC >= 2
_VOID mem_debug_check_all(CONST _UBYTE *where);
#endif

#else /* !DEBUG_ALLOC */

#define MALLOC(size, from) mem_get((_ULONG)(size))
#define SYSMALLOC(size, type, from) mem_get_system((_ULONG)(size), type)
#define CALLOC(nitems, size, from) mem_0get((_ULONG)(nitems) * (_ULONG)(size))
#define M0ALLOC(size, from) mem_0get((_ULONG)(size))
#define STRDUP(str, from) mem_str_dup(str)
#define FREE(block, size) mem_free(block)
#define SFREE(str) mem_free(str)
#define MEMCHECK(ptr, where) /**/

#endif /* DEBUG_ALLOC */

#define SYSFREE(block, size) FREE(block, size)
#define NEW(type, from) (type *)MALLOC(sizeof(type), from)
#define OFREE(obj) FREE(obj, sizeof(*(obj)))


#if ((defined(WINDOWS)&&!defined(__WIN32__)) || defined(__MSDOS__)) && !defined(__FLAT__)

_VOID MemCpy(_VOID *to, CONST _VOID *from, _ULONG size);
_VOID MemMove(_VOID *to, CONST _VOID *from, _ULONG size);
_VOID MemSet(_VOID *buf, _WORD cc, _ULONG size);
_WORD MemCmp(CONST _VOID *to, CONST _VOID *from, _ULONG size);

#else

#ifdef HAVE_BCOPY
#define MemCpy(dst, src, size) bcopy(src, dst, size)
#else
#define MemCpy(dst, src, size) memcpy(dst, src, size)
#endif
#define MemMove(dst, src, size) memmove(dst, src, size)
#define MemSet(dst, val, size) memset(dst, val, size)
#define MemCmp(dst, src, size) memcmp(dst, src, size)

#endif /* WINDOWS || __MSDOS__ */

#define MemCpyStruct(aa, bb) MemCpy(&(aa), &(bb), sizeof(aa))
#define MemCmpStruct(aa, bb) MemCmp(&(aa), &(bb), sizeof(aa))
#ifdef HAVE_BZERO
#define MemSetZero(aa, bb) (_VOID) bzero(aa, bb)
#else
#define MemSetZero(aa, bb) (_VOID) MemSet(aa, 0, bb)
#endif
#define MemSetZeroStruct(aa) MemSetZero(&(aa), sizeof(aa))
#define MemSetZeroArray(aa) MemSetZero(aa, sizeof(aa))

#endif /* __RO_MEM_H__ */
