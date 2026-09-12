/*****************************************************************************
 * WINDOWS/MEMORY.C
 *****************************************************************************/

#include <ro_mem.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows_.h>
#include <routine.h>
#include <file_io.h>
#include <debug.h>

/* for farmalloc */
#ifdef __BORLANDC__
#undef __STDC__
#endif

#ifndef __WIN32__
#include <dos.h>
#include <alloc.h>
#endif

#ifndef GMEM_MODIFY
#  define GMEM_MODIFY 0x0080
#endif

#define MEM_USE_LOCAL_SIZE	65520l		/* 32768L */

LOCAL ERR_MEM_FUNC p_err_memory;
#if SPEC_DEBUG
LOCAL FILE *debug_file;
#endif

#if SPEC_DEBUG
GLOBAL _BOOL is_DEBUGGING;
#endif

LOCAL ERROROUT_FUNC errorOutUseFunk;
LOCAL _LONG memUse;
LOCAL _ULONG memUseSize;
LOCAL _BOOL mem_test_ignored;
#if SPEC_DEBUG
LOCAL _VOID (*p_assert)(CONST _UBYTE *expr, CONST _UBYTE *fname, _LONG line);
#endif

#define MAGIC_SIZE 4

typedef struct _mem_control
{
#if DEBUG_ALLOC
	_LONG	size;
#if DEBUG_ALLOC >= 2
	CONST _UBYTE *who;
	struct _mem_control _HUGE *next;
#endif
	_UBYTE check[MAGIC_SIZE];
#endif
	_LONG magic;
} MEM_CONTROL;

#define MEM_MAGIC_START_SYSTEM 47110815L
#define MEM_MAGIC_START_INTERN 8154711L

#define MEM_MAGIC_NEW_ALLOCATED 0xbb
#define MEM_MAGIC_END			0xaa
#define MEM_MAGIC_FREED         0x99


#if DEBUG_ALLOC
#define EXTRA_MEM (sizeof(MEM_CONTROL) + sizeof(_UBYTE) * MAGIC_SIZE)
#else
#define EXTRA_MEM (sizeof(MEM_CONTROL))
#endif

GLOBAL _BOOL is_MASTER;

#undef mem_free


/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

#if DEBUG_ALLOC >= 2
LOCAL MEM_CONTROL _HUGE *alloc_list;

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL delete_alloc_list(MEM_CONTROL _HUGE *block)
{
	MEM_CONTROL _HUGE *search = alloc_list;

	if (block == search)
	{
		alloc_list = search->next;
	} else
	{
		while (search && search->next != block)
			search = search->next;
		if (search && search->next == block)
		{
			search->next = search->next->next;
		} else
		{
			ErrorOut(EO_ERROR, "Memory Block not found");
			return FALSE;
		}
	}
	return TRUE;
}

LOCAL _BOOL in_alloc_list(MEM_CONTROL _HUGE *block)
{
	MEM_CONTROL _HUGE *search = alloc_list;

	while (search)
	{
		if (search == block)
			return TRUE;
		search = search->next;
	}
	return FALSE;
}

#endif /* DEBUG_ALLOC >= 2 */

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID *sysMemGet(_ULONG size, _UWORD *type)
{
	_VOID *ptr;

	if (size > MEM_USE_LOCAL_SIZE)
		*type |= MEM_SYSTEM;
	if (*type & MEM_SHARABLE)
		*type |= MEM_SYSTEM;

#if GUI_VERSION || CONSOLE_VERSION
	{
		_WORD	runs;
		HANDLE	hnd;
		_UWORD	wtype;

		wtype = 0;
		if (*type & MEM_FIXED)
			wtype |= GMEM_FIXED;
		if (*type & MEM_MOVEABLE)
			wtype |= GMEM_MOVEABLE;
		if (*type & MEM_NOCOMPACT)
			wtype |= GMEM_NOCOMPACT;
		if (*type & MEM_NODISCARD)
			wtype |= GMEM_NODISCARD;
		if (*type & MEM_ZEROINIT)
			wtype |= GMEM_ZEROINIT;
		if (*type & MEM_MODIFY)
			wtype |= GMEM_MODIFY;
		if (*type & MEM_DISCARDABLE)
			wtype |= GMEM_DISCARDABLE;
		if (*type & MEM_NOT_BANKED)
			wtype |= GMEM_NOT_BANKED;
		if (*type & MEM_SHARABLE)
			wtype |= GMEM_SHARE;
		if (*type & MEM_NOTIFY)
			wtype |= GMEM_NOTIFY;

		for (runs = 0;; runs++) /* Fehler kaum zu reproduzieren, klappt vielleicht beim 2. mal */
		{
			hnd = GlobalAlloc(wtype, size);

			if (hnd == NULL)
			{
#define errStr "GlobalAlloc(%d-%ld)."

				GlobalCompact(size);
				if (runs >= 2)
				{
					ErrorOut(EO_FATAL, errStr, runs, size);
					return NULL;
				} else
				{
					ErrorOut(EO_INFO, errStr, runs, size);
				}
			} else
			{
				break;
			}
		}

#ifdef __WIN32__
		if (*type & MEM_FIXED)
			ptr = hnd;
		else
			ptr = GlobalLock(hnd);
#else /* !__WIN32__ */
		ptr = GlobalLock(hnd);
#endif /* __WIN32__ */
#if !DEBUG_ALLOC
		*type &= ~MEM_ZEROINIT;
#endif
	}
#else /* !GUI_VERSION */
	ptr = farmalloc(size);
#endif /* GUI_VERSION */

	return ptr;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID *stdMemGet(_ULONG size, _UWORD type, CONST _UBYTE *who)
{
	MEM_CONTROL _HUGE *cntl;

#if DEBUG_ALLOC
	if (size == 0)
	{
		ErrorOut(EO_ERROR, "MemGet(0) ? (%s)", who ? who : (CONST _UBYTE *) "null");
		return NULL;
	}
#endif

	cntl = sysMemGet(size + EXTRA_MEM, &type);

	if (cntl == NULL)
	{
		if (!(type & MEM_NOERR))
			err_memory(size);
		return NULL;
	}
	
	cntl->magic = (type & MEM_SYSTEM) ? MEM_MAGIC_START_SYSTEM : MEM_MAGIC_START_INTERN;
#if DEBUG_ALLOC
	cntl->size = size;
	{
		size_t i;
		
		for (i = 0; i < MAGIC_SIZE; i++)
			cntl->check[i] = MEM_MAGIC_END;
	}
#if DEBUG_ALLOC >= 2
	cntl->next = alloc_list;
	alloc_list = cntl;
	cntl->who = who;
#endif
#else
	UNUSED(who);
#endif
	cntl++;

	if (type & MEM_ZEROINIT)
		MemSetZero(cntl, size);
#if DEBUG_ALLOC
	else
		MemSet(cntl, MEM_MAGIC_NEW_ALLOCATED, size);
#endif

#if DEBUG_ALLOC
	{
		_UBYTE _HUGE *test;
		size_t i;
		
		test = (_UBYTE _HUGE *) cntl + size;
		for (i = 0; i < MAGIC_SIZE; i++)
			test[i] = MEM_MAGIC_END;
	}
	memUseSize += size;
#endif

	memUse++;

	return (_VOID *) cntl;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID *mem_get(_ULONG size)
{
	return stdMemGet(size, MEM_MOVEABLE, NULL);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID *mem_0get(_ULONG size)
{
	return stdMemGet(size, MEM_MOVEABLE | MEM_ZEROINIT, NULL);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID *mem_get_system(_ULONG size, _UWORD type)
{
	return stdMemGet(size, type | MEM_SYSTEM, NULL);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID *mem_0get_system(_ULONG size, _UWORD type)
{
	return stdMemGet(size, type | MEM_SYSTEM | MEM_ZEROINIT, NULL);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _UBYTE *mem_str_dup(CONST _UBYTE *str)
{
	_UBYTE *p;

	if (str == NULL)
		return NULL;
	if ((p = mem_get(strlen(str) + 1)) != NULL)
		strcpy(p, str);
	return p;
}

/*** ---------------------------------------------------------------------- ***/

#if DEBUG_ALLOC

GLOBAL _VOID *mem_debug_get(_ULONG size, CONST _UBYTE *who)
{
	return stdMemGet(size, MEM_MOVEABLE, who);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID *mem_debug_0get(_ULONG size, CONST _UBYTE *who)
{
	return stdMemGet(size, MEM_MOVEABLE|MEM_ZEROINIT, who);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID *mem_debug_get_system(_ULONG size, _UWORD type, CONST _UBYTE *who)
{
	return stdMemGet(size, type|MEM_SYSTEM, who);
}

/*** ---------------------------------------------------------------------- ***/

_VOID *mem_debug_0get_system(_ULONG size, _UWORD type, CONST _UBYTE *who)
{
	return stdMemGet(size, type|MEM_SYSTEM|MEM_ZEROINIT, who);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _UBYTE *mem_debug_str_dup(CONST _UBYTE *str, CONST _UBYTE *from)
{
	_UBYTE *new;

	if (str == NULL)
		return NULL;
	new = mem_debug_get(strlen(str) + 1, from);
	if (new != NULL)
		strcpy(new, str);
	return new;
}

#endif /* DEBUG_ALLOC */

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

LOCAL _VOID sysMemFree(_VOID *ptr, _UWORD type)
{
	UNUSED(type);
#if GUI_VERSION || CONSOLE_VERSION
	{
#ifdef __WIN32__
		HGLOBAL hnd;

		hnd = GlobalHandle(ptr);

		if (hnd != NULL)
		{
			if (GlobalUnlock(hnd) == FALSE && GetLastError() == NO_ERROR)
				GlobalFree(hnd);
		}
#else
		DWORD hnd;
		HGLOBAL glbl;
		
		hnd = GlobalHandle(FP_SEG(ptr));

		if (hnd != NULL)
		{
			glbl = (HGLOBAL)LOWORD(hnd);
			if (GlobalUnlock(glbl) == 0)
				GlobalFree(glbl);
		}
#endif /* __WIN32__ */
	}
#else /* !GUI_VERSION */
	farfree(ptr);
#endif /* GUI_VERSION */
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID mem_free(_VOID *strPtr)
{
	_WORD type;
	MEM_CONTROL _HUGE *memCntl;

	if (strPtr == NULL)
	{
		ErrorOut(EO_ERROR, "mem_free(NULL) ?");
		return;
	}

	memCntl = (MEM_CONTROL _HUGE *) ((_UBYTE _HUGE *) (strPtr) - sizeof (MEM_CONTROL));

#if DEBUG_ALLOC >= 2
	if (!delete_alloc_list(memCntl))
		return;
#endif
#if DEBUG_ALLOC
	{
		size_t i;
		_BOOL defect;
		
		defect = memCntl->magic != MEM_MAGIC_START_SYSTEM &&
			memCntl->magic != MEM_MAGIC_START_INTERN;
		for (i = 0; i < MAGIC_SIZE; i++)
			if (memCntl->check[i] != MEM_MAGIC_END)
				defect = TRUE;
		if (defect)
		{
#if DEBUG_ALLOC >= 2
			ErrorOut(EO_FATAL, "MemStart defekt (%ld-%ld) (%s).", memUse, memCntl->size, memCntl->who ? memCntl->who : (CONST _UBYTE *) "(null)");
#elif DEBUG_ALLOC
			ErrorOut(EO_FATAL, "MemStart defekt (%ld-%ld).", memUse, memCntl->size);
#else
			ErrorOut(EO_FATAL, "MemStart defekt (%ld %p).", memUse, strPtr);
#endif
			return;
		}
	}
#endif
	type = 0;
	if (memCntl->magic == MEM_MAGIC_START_SYSTEM)
	{
		type |= MEM_SYSTEM;
	}

#if DEBUG_ALLOC
	{
		_UBYTE _HUGE *test = ((_UBYTE _HUGE *) (memCntl)) + memCntl->size + sizeof(MEM_CONTROL);
		size_t i;
		_BOOL defect;
		
		defect = FALSE;
		for (i = 0; i < MAGIC_SIZE; i++)
			if (test[i] != MEM_MAGIC_END)
				defect = TRUE;
		if (defect)
		{
#if DEBUG_ALLOC >= 2
			/* assumes MAGIC_SIZE >= 4 */
			ErrorOut(EO_FATAL, "MemEnd defekt %ld (%s). %02x %02x %02x %02x", memCntl->size, memCntl->who ? memCntl->who : (CONST _UBYTE *) "(null)",
				test[0], test[1], test[2], test[3]);
#else
			ErrorOut(EO_FATAL, "MemEnd defekt %ld.", memCntl->size, memUse);
#endif
		}
	}
#endif

	memUse--;

#if DEBUG_ALLOC
	memUseSize -= memCntl->size;
	MemSet(memCntl, MEM_MAGIC_FREED, memCntl->size + EXTRA_MEM);
#endif
	sysMemFree(memCntl, type);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL mem_debug_check(_VOID *ptr, CONST _UBYTE *where)
{
	MEM_CONTROL _HUGE *memCntl;
	_WORD defect;

	if (ptr == NULL)
	{
		ErrorOut(EO_ERROR, "mem_check(NULL)? (%s)", where);
		return FALSE;
	}

	memCntl = (MEM_CONTROL _HUGE *) ((_UBYTE _HUGE *) (ptr) - sizeof(MEM_CONTROL));

#if DEBUG_ALLOC >= 2
	if (!in_alloc_list(memCntl))
	{
		ErrorOut(EO_ERROR, "%s: Memory Block not found", where);
		return FALSE;
	}
#endif
	defect = memCntl->magic != MEM_MAGIC_START_SYSTEM &&
		memCntl->magic != MEM_MAGIC_START_INTERN;
#if DEBUG_ALLOC
	{
		size_t i;
		
		for (i = 0; i < MAGIC_SIZE; i++)
			if (memCntl->check[i] != MEM_MAGIC_END)
				defect |= 1;
		if (defect & 1)
		{
#if DEBUG_ALLOC >= 2
			ErrorOut(EO_FATAL, "MemStart defekt (%ld-%ld) (%s).", memUse, memCntl->size, memCntl->who ? memCntl->who : (CONST _UBYTE *) "(null)");
#elif DEBUG_ALLOC
			ErrorOut(EO_FATAL, "MemStart defekt (%ld-%ld).", memUse, memCntl->size);
#else
			ErrorOut(EO_FATAL, "MemStart defekt (%ld %p).", memUse, strPtr);
#endif
			return FALSE;
		}
	}

	{
		_UBYTE _HUGE *test = ((_UBYTE _HUGE *) (memCntl)) + memCntl->size + sizeof(MEM_CONTROL);
		size_t i;
		
		for (i = 0; i < MAGIC_SIZE; i++)
			if (test[i] != MEM_MAGIC_END)
				defect |= 2;
		if (defect & 2)
		{
#if DEBUG_ALLOC >= 2
			/* assumes MAGIC_SIZE >= 4 */
			ErrorOut(EO_FATAL, "%s: MemEnd defekt %ld (%s). %02x %02x %02x %02x", where, memCntl->size, memCntl->who ? memCntl->who : (CONST _UBYTE *) "(null)",
				test[0], test[1], test[2], test[3]);
#else
			ErrorOut(EO_FATAL, "%s: MemEnd defekt %ld.", where, memCntl->size, memUse);
#endif
			return FALSE;
		}
	}
#endif /* DEBUG_ALLOC */
	return defect == 0;
}

/*** ---------------------------------------------------------------------- ***/

#if DEBUG_ALLOC >= 2
GLOBAL _VOID mem_debug_check_all(CONST _UBYTE *where)
{
	MEM_CONTROL _HUGE *search = alloc_list;
	_BOOL error;
	
	error = FALSE;
	while (search != NULL)
	{
		if (!mem_debug_check(search + 1, where))
			error = TRUE;
		search = search->next;
	}
	if (!error)
		ErrorOut(EO_DEBUG, "%s: mem ok\n", where);
}
#endif

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID mem_debug_free(_VOID *block)
{
	mem_free(block);
}

LOCAL _ULONG startMemCount;
LOCAL _ULONG startMemSize;

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID sys_mem_init(_BOOL init)
{
	if (init)
	{
#if SPEC_DEBUG
		char *env;
		
		env = getenv("DEBUG");
		if (env)
			is_DEBUGGING = (*env >= '0' && *env <= '9') ? (*env - '0') : 1;
		else
			is_DEBUGGING = FALSE;
		if (is_DEBUGGING)
		{
			debug_file = fopen("debug.out", "w");
		}
#endif
	} else
	{
#if SPEC_DEBUG
		if (debug_file != NULL)
		{
			fclose(debug_file);
			debug_file = NULL;
		}
#endif
	}
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID mem_test_ignore(_VOID)
{
	mem_test_ignored = TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID mem_test_end(_VOID)
{
	if ((memUse != startMemCount || memUseSize != startMemSize) && !mem_test_ignored)
	{
		Pling();
		ErrorOut(EO_DEBUG, "MemTest A:%ld S:%ld", memUse - startMemCount, memUseSize - startMemSize);
#if DEBUG_ALLOC >= 2
		while (alloc_list != NULL)
		{
			ErrorOut(EO_TRACE, "-> %7ld %s", alloc_list->size, alloc_list->who ? alloc_list->who : (CONST _UBYTE *) "(null)");
			alloc_list = alloc_list->next;
		}
#endif
	}
	sys_mem_init(FALSE);
	if (errorOutUseFunk != FUNK_NULL)
		ErrorOut(EO_EXIT, " ");
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID mem_test_start(_VOID)
{
	sys_mem_init(TRUE);
	startMemCount = memUse;
	startMemSize = memUseSize;
	ErrorOut(EO_INIT, " ");
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL _VOID set_assert(_VOID (*f_assert)(CONST _UBYTE *expr, CONST _UBYTE *fname, _LONG line))
{
#if SPEC_DEBUG
	p_assert = f_assert;
#else
	UNUSED(f_assert);
#endif
}

/*** ---------------------------------------------------------------------- ***/

#if SPEC_DEBUG
GLOBAL _BOOL _assert_(CONST _UBYTE *expr, CONST _UBYTE *file, _LONG line)
{
	_UBYTE expr_buf[512];
	_UBYTE fname_buf[512];
	_UBYTE *cp;
	static _WORD been_here;

	if (been_here == 0)
	{
		++been_here;
		cp = expr_buf;
		while (*expr)
		{
			if (*expr == '|' || *expr == '[' || *expr == ']' /* || *expr == '\\' */)
				*cp++ = '\\';
			*cp++ = *expr++;
		}
		*cp = '\0';
		cp = fname_buf;
		while (*file)
		{
			if (*file == '\\')
				*cp++ = '\\';
			*cp++ = *file++;
		}
		*cp = '\0';
		if (p_assert != FUNK_NULL)
		{
			(*p_assert)(expr, fname_buf, line);
		} else
		{
			fprintf(stderr, "assertion failed: %s\nfile %s line %ld\n",
				expr_buf, fname_buf, line);
		}
		--been_here;
	}
	return FALSE;
}
#endif

/*** ---------------------------------------------------------------------- ***/

GLOBAL ERROROUT_FUNC ErrorOut_SetFunk(ERROROUT_FUNC UseFunk)
{
	ERROROUT_FUNC old_func = errorOutUseFunk;

	errorOutUseFunk = UseFunk;
	return old_func;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID ErrorStrOut(CONST _UBYTE *str)
{
#if GUI_VERSION || CONSOLE_VERSION
	_UBYTE buf[128];
	size_t len;
	
	strMcpy(buf, sizeof(buf) - 1, str);
	len = strlen(buf);
	if (len && buf[len-1] == '\n')
	{
		buf[len-1] = '\r';
		buf[len] = '\n';
		buf[len+1] = '\0';
	}
	OutputDebugString(buf);
#else
	fputs(str, stdout);
	fflush(stdout);
#endif /* GUI_VERSION */
#if SPEC_DEBUG
	if (debug_file != NULL)
	{
		fputs(str, debug_file);
		fflush(debug_file);
	}
#endif
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID ErrorOut(EO_MODUS modus, CONST char *str __c_va_alist)
{
	_UBYTE	box[512];
	__c_va_list args;
	static _WORD nr;
	_UBYTE	nrStr[512];
	CONST _UBYTE *modeStr;
	_BOOL alert_error = FALSE;

	if (modus == EO_ERROR || modus == EO_FATAL)
	{
		Pling();
		alert_error = TRUE;
	}

	switch (modus)
	{
	case EO_INFO:
		modeStr = "INFO ";
		break;

	case EO_TRACE:
		modeStr = "TRACE";
		break;

	case EO_ERROR:
		modeStr = "ERROR";
		break;

	case EO_DEBUG:
		modeStr = "DEBUG";
		break;

	case EO_FATAL:
	default:
		modeStr = "FATAL";
		break;
	}

	nr++;

	__c_va_start(args, str);
	vsprintf(box, str, args);
	__c_va_end(args);
	if (errorOutUseFunk != FUNK_NULL)
	{
		alert_error = errorOutUseFunk(ProgramName, modus, nr, modeStr, box);
	}
	if (modus != EO_EXIT && modus != EO_INIT)
	{
		if (errorOutUseFunk == FUNK_NULL)
		{
			sprintf(nrStr, "%-8.8s-%d-%s: %s\n", ProgramName, nr, modeStr, box);
			ErrorStrOut(nrStr);
		}
	} else
	{
		--nr;
	}
	if (alert_error)
	{
#if GUI_VERSION
		MessageBox(NULL, box, modeStr, MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION | MB_DEFBUTTON1);
#endif /* GUI_VERSION */
	}
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL ERR_MEM_FUNC set_err_memory(ERR_MEM_FUNC p_err)
{
	ERR_MEM_FUNC old = p_err_memory;

	p_err_memory = p_err;
	return old;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID err_memory(_ULONG size)
{
	if (p_err_memory != FUNK_NULL)
	{
		(*p_err_memory)(size);
	} else
	{
		ErrorOut(EO_FATAL, "not enough memory for %ld bytes", size);
	}
}

/*** ---------------------------------------------------------------------- ***/

#ifndef __WIN32__
GLOBAL _VOID MemCpy(_VOID *to, CONST _VOID *from, _ULONG size)
{
	_UBYTE _HUGE *toBy = to;
	CONST _UBYTE _HUGE *fromBy = from;

	while (size)
	{
		*toBy++ = *fromBy++;
		size--;
	}
}
#endif /* __WIN32__ */

/*** ---------------------------------------------------------------------- ***/

#ifndef __WIN32__
GLOBAL _VOID MemMove(_VOID *to, CONST _VOID *from, _ULONG size)
{
	_UBYTE _HUGE *toBy = to;
	CONST _UBYTE _HUGE *fromBy = from;

	if (toBy > fromBy)
	{
		toBy += size;
		fromBy += size;

		for (; size > 0; size--)
		{
			toBy--;
			fromBy--;
			*toBy = *fromBy;
		}
	} else
	{
		for (; size > 0; size--)
		{
			*toBy = *fromBy;
			toBy++;
			fromBy++;
		}
	}
}
#endif /* __WIN32__ */

/*** ---------------------------------------------------------------------- ***/

#ifndef __WIN32__
GLOBAL _VOID MemSet(_VOID *buf, _WORD cc, _ULONG size)
{
	_UBYTE _HUGE *toBy = buf;

	for (; size > 0; size--)
		*toBy++ = (_UBYTE) cc;
}
#endif /* __WIN32__ */

/*** ---------------------------------------------------------------------- ***/

#ifndef __WIN32__
GLOBAL _WORD MemCmp(CONST _VOID *to, CONST _VOID *from, _ULONG size)
{
	CONST _UBYTE _HUGE *toBy = to;
	CONST _UBYTE _HUGE *fromBy = from;

	for (; size > 0; size--)
	{
		if (*toBy != *fromBy)
		{
			if (*toBy > *fromBy)
				return 1;
			return -1;
		}
		toBy++;
		fromBy++;
	}
	return 0;
}
#endif

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Pling(_VOID)
{
#if GUI_VERSION || CONSOLE_VERSION
	MessageBeep((UINT) -1);
/*	MessageBeep(MB_ICONEXCLAMATION);*/
#else
	putc(7, stderr);
	fflush(stderr);
#endif /* GUI_VERSION */
}

/*** ---------------------------------------------------------------------- ***/

__c_va_list va_empty_args(_VOID)
{
	return NULL;
}
