/*****************************************************************************
 * DEBUG.H
 *****************************************************************************/

#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifndef __PORTAB_H__
#  include <portab.h>
#endif

#ifndef SPEC_DEBUG
#  define SPEC_DEBUG 1
#endif

extern _BOOL is_MASTER;

_VOID Pling(_VOID);

#if SPEC_DEBUG
extern _BOOL is_DEBUGGING;
#  define ASSERT( expr )\
	 ((_VOID)((expr) || _assert_(#expr, T(__FILE__), (_LONG)__LINE__)))
#  define SPEC_DEBUG_OUT(x) ((_VOID)(!is_DEBUGGING || (ErrorOut x, 0)))
#else
#  define SPEC_DEBUG_OUT(x)
#  define ASSERT(expr)			/* nothing */
#endif

typedef enum
{
	EO_INFO,
	EO_TRACE,
	EO_ERROR,
	EO_FATAL,
	EO_DEBUG,
	EO_INIT,
	EO_EXIT
} EO_MODUS;

typedef _BOOL (*ERROROUT_FUNC)(
	CONST _UBYTE *prgName,
	EO_MODUS modus,
	_WORD nr,
	CONST _UBYTE *modeStr,
	CONST _UBYTE *errStr
);

/* eigentlich CONST _UBYTE *str, aber dann kann GNU-C das format nicht pruefen */
_VOID ErrorOut(EO_MODUS modus, CONST char *str, ...) __attribute__((format(printf, 2, 3)));

ERROROUT_FUNC ErrorOut_SetFunk(ERROROUT_FUNC UseFunk);

_BOOL _assert_ ( CONST _UBYTE *expr, CONST _UBYTE *file, _LONG line );
_VOID set_assert ( _VOID (*p_assert) ( CONST _UBYTE *expr, CONST _UBYTE *fname, _LONG line ) );

#endif /* __DEBUG_H__ */
