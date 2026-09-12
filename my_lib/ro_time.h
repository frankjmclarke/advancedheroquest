/*****************************************************************************
 * RO_TIME.H
 *****************************************************************************/

#ifndef __RO_TIME_H__
#define __RO_TIME_H__

#ifndef __PORTAB_H__
#  include <portab.h>
#endif

typedef _ULONG SYSTIME;

#define BASEYEAR 1900
#define LEAPBASEYEAR ((((BASEYEAR + 1) / 4) * 4) + 1)

/* last year that can be represent by SYSTIME */
#define MAX_YEAR 2035

#define SECS_PER_MIN		((SYSTIME)(60l))
#define SECS_PER_HOUR		((SYSTIME)(SECS_PER_MIN * 60l))
#define SECS_PER_DAY		((SYSTIME)(SECS_PER_HOUR * 24l))
#define SECS_PER_YEAR		((SYSTIME)(SECS_PER_DAY * 365l))
#define SECS_PER_LEAPYEAR	(SECS_PER_YEAR + SECS_PER_DAY)

/* seconds between 1.1.1900 and 1.1.1970 */
#define T_DIFFTIME ((SYSTIME)(((1970 - BASEYEAR) * 365l + (1970 - LEAPBASEYEAR) / 4) * SECS_PER_DAY))
/* #define T_DIFFTIME ((SYSTIME)2208988800l) */


typedef struct {
	_UWORD tm_mday; /* day of month (0-30) */
	_UWORD tm_mon;	/* month (0-11) */
	_UWORD tm_year; /* year (absolute) */
	_UWORD tm_hour; /* hour (0-23) */
	_UWORD tm_min;	/* minute (0-59) */
	_UWORD tm_sec;	/* second (0-59) */
	_UWORD tm_yday; /* day of year (0-365) */
	_UWORD tm_wday; /* day of week (0-6, 0 = sunday) */
} F_TIME;


F_TIME *T_Converttime ( SYSTIME ltime, F_TIME *t );
F_TIME *T_Localtime ( SYSTIME ltime, F_TIME *t );
SYSTIME T_Calctime ( CONST F_TIME *t );
SYSTIME T_Mktime ( CONST F_TIME *t );
SYSTIME T_Systime ( _VOID );
_VOID T_Set_Systime ( SYSTIME t );
SYSTIME T_DosToSystime ( _UWORD dosdate, _UWORD dostime );
_VOID T_SysToDostime ( SYSTIME ltime, _UWORD *date, _UWORD *ptime );
SYSTIME T_Tzadjust ( SYSTIME ltime, _BOOL to_local );
_VOID T_SetZero ( F_TIME *t );
_BOOL T_IsZero ( CONST F_TIME *t );

#define TIME_TS_SECONDS     0x01
#define TIME_TS_PUNCT		0x02
#define TIME_TS_ZERO		0x04
#define TIME_TS_12H         0x08
#define TIME_TS_DEFAULT_MODE	(TIME_TS_ZERO)
#define TIME_MAX_SIZE		12

_UBYTE *T_TimeToString ( _UBYTE *string, _UWORD strSize, _UWORD mode, _UBYTE delimiter, SYSTIME tt );
_UBYTE *T_SystimeToString ( _UBYTE *string, _UWORD strSize, _UWORD mode, SYSTIME tt );
_BOOL T_StringToTime ( CONST _UBYTE *str, SYSTIME *tt );
_BOOL T_TimeOk ( CONST F_TIME *t );
_WORD T_TimeCmp ( SYSTIME t1, SYSTIME t2 );

#define DATE_TS_CENTURY     0x01
#define DATE_TS_PUNCT		0x02
#define DATE_TS_DAY_ZERO	0x04
#define DATE_TS_MON_ZERO	0x08
#define DATE_TS_DMY         0x10
#define DATE_TS_MDY         0x20
#define DATE_TS_YMD         0x30
#define DATE_TS_MASK		0x30
#define DATE_TS_DEFAULT_MODE (DATE_TS_DAY_ZERO | DATE_TS_MON_ZERO | DATE_TS_DMY)
#define DATE_MAX_SIZE		11

_UBYTE *T_DateToString ( _UBYTE *string, _UWORD strSize, _UWORD mode, _UBYTE delimiter, SYSTIME tt );
_UBYTE *T_SysdateToString ( _UBYTE *string, _UWORD strSize, _UWORD mode, SYSTIME tt );
_BOOL T_StringToDate ( CONST _UBYTE *str, SYSTIME *tt );
_BOOL T_DateOk ( CONST F_TIME *t );
_WORD T_DateCmp ( SYSTIME t1, SYSTIME t2 );
_LONG T_DateDiffDays ( SYSTIME t1, SYSTIME t2 );
SYSTIME T_DateAdd ( SYSTIME t, _LONG add );
_UWORD T_Date_AlterJahre ( SYSTIME birthday );

size_t T_Strftime(_UBYTE *str, size_t maxsize, CONST _UBYTE *format, F_TIME *t, ...);
size_t T_Strlftime(_UBYTE *str, size_t maxsize, CONST _UBYTE *format, SYSTIME ltime, ...);

#endif /* __RO_TIME_H__ */
