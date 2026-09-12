/*****************************************************************************
 * ROUTINE.H
 *****************************************************************************/

#ifndef __ROUTINE_H__
#define __ROUTINE_H__

#ifndef __PORTAB_H__
#  include <portab.h>
#endif
#ifndef __VAR_ARGS_H__
#  include <var_args.h>
#endif

#define PtrSize(aa) (aa), (_UWORD)sizeof(aa)
#define PtrAdrSize(aa) ((_VOID*)&aa), (_UWORD)sizeof(aa)

_VOID FlipWord(_WORD *aa);
_VOID FlipUWord(_UWORD *aa);
_VOID FlipLong(_LONG *start);
_VOID FlipULong(_ULONG *start);

_UBYTE *xtrcpy(_UBYTE *dest, CONST _UBYTE *src);
_UBYTE *xtrcat(_UBYTE *dest, CONST _UBYTE *src);
_UBYTE *strMcat(_UBYTE *dest, size_t size, CONST _UBYTE *src);
_UBYTE *vstrcat(_UBYTE *to, size_t size, CONST _UBYTE *first __c_va_alist);
_BOOL replaceChar(_UBYTE *str, _UBYTE from, _UBYTE to);
_BOOL delChar(_UBYTE *str, _UBYTE old);
_BOOL leerStr(CONST _UBYTE *str1);

_UBYTE *LongToStr(_UBYTE *toBuf, size_t size, _LONG wert);
_UBYTE *LongToRstr(_UBYTE *string, size_t size, _LONG zahl);
_UBYTE *LongToRVstr(_UBYTE *string, size_t size, _LONG zahl);
_UBYTE *LongToR0str(_UBYTE *string, size_t size, _LONG zahl);
_UBYTE *make_point(_UBYTE *str, size_t n_komma);

_LONG a_to_l(CONST _UBYTE *str, _WORD len);
_ULONG a_to_ul(CONST _UBYTE *str, _WORD len);
#define a_to_i(str, len) (_WORD)a_to_l(str, len)
_UBYTE *l_to_a(_UBYTE *buf, size_t bufsize, _LONG val, _WORD len);
#define i_to_a(buf, bufsize, val, len) l_to_a(buf, bufsize, (_LONG)(val), len)
_UBYTE *f_to_a(_UBYTE *buf, size_t bufsize, _LONG val, _WORD len, _WORD dez);
_LONG a_to_f(_UBYTE *str, _WORD dez);

_UBYTE *nummer_make(_UBYTE *string, _WORD v_komma, _WORD n_komma);
_UBYTE *LongToWaehrungStr(_UBYTE *string, size_t size, _LONG zahl, _BOOL showWae);

_VOID strdelRightSpace(_UBYTE *str);
_VOID strdelSpace(_UBYTE *str);
_VOID strright(_UBYTE *string, size_t leng);
_VOID strleft(_UBYTE *string);
_VOID strFillRight(_UBYTE *string, size_t strsize, size_t leng);
_VOID strzenter(_UBYTE *string, size_t leng);
_VOID strMove(_UBYTE *desti, _UBYTE *source);
_VOID strnMove(_UBYTE *desti, _UBYTE *source, size_t anzahl);
CONST _UBYTE *strTeilCatOrCpy(_UBYTE *to, size_t toMaxBuf, CONST _UBYTE *run);
_UBYTE ucase(_UBYTE t);
_UBYTE lcase(_UBYTE t);
_UWORD scase(_UBYTE t);
CONST _UBYTE *get_uppercase_table(_VOID);
CONST _UBYTE *get_lowercase_table(_VOID);
_UBYTE hextoc(_UBYTE ch);
_UBYTE *strUpr(_UBYTE *work);
_UBYTE *strUprSort(_UBYTE *work);
_UBYTE *strLwr(_UBYTE *work);

_UBYTE *strMcpy(_UBYTE *dest, size_t maxlen, CONST _UBYTE *src);
_BOOL strReplace(_UBYTE *strTo, size_t sizeTo, CONST _UBYTE *oldstr, CONST _UBYTE *newstr);
_BOOL strReplaceWord(_UBYTE *strTo, size_t sizeTo, CONST _UBYTE *oldstr, CONST _UBYTE *newstr);

_WORD strCmp(CONST _UBYTE *str1, CONST _UBYTE *str2);
_WORD strnCmp(CONST _UBYTE *str1, CONST _UBYTE *str2, size_t leng);
_WORD strCaseCmp(CONST _UBYTE *str1, CONST _UBYTE *str2);
_WORD strSortCmp(CONST _UBYTE *str1, CONST _UBYTE *str2);
_WORD strnCaseCmp(CONST _UBYTE *str1, CONST _UBYTE *str2, size_t leng);
CONST _UBYTE *strCaseStr(CONST _UBYTE _HUGE *s, CONST _UBYTE *wanted);

_VOID strInsertChar(_UBYTE ch, _UBYTE *str, size_t strsize);


#define strBcpy(aa, bb) strMcpy(PtrSize(aa), bb)
#define strBcat(aa, bb) strMcat(PtrSize(aa), bb)
	
_BOOL EleListe(CONST _UBYTE *in, CONST _UBYTE *liste);

_BOOL strmatch(CONST _UBYTE *pattern, CONST _UBYTE *text);

_BOOL is_digit(_UBYTE c);
_BOOL is_alpha(_UBYTE c);
_BOOL is_alnum(_UBYTE c);
_BOOL is_upper(_UBYTE c);
_BOOL is_lower(_UBYTE c);
#define is_cntrl(c) ((c) < 0x20)

size_t Str_ConvertTabs(CONST _UBYTE *source, _UBYTE *dest, size_t destsize, _UWORD TabSize);
/* Expandiert den String zu einem ohne Tabs */

#undef min
#undef max
#undef abs
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define abs(x) ((x) < 0 ? -(x) : (x))

#endif /* __ROUTINE_H__ */
