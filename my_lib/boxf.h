#ifndef __BOXF_H__
#define __BOXF_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif
#ifndef __VAR_ARGS_H__
#include <var_args.h>
#endif

_WORD Form_Alert(_WORD def, CONST _UCHAR *str __c_va_alist) __attribute__((format(printf, 2, 3)));
_WORD Form_Ok(CONST _UCHAR *str __c_va_alist) __attribute__((format(printf, 2, 3)));

_VOID ok(CONST _UCHAR *str);
_VOID abbruch(CONST _UCHAR *str);
_VOID ende(CONST _UCHAR *str);
_VOID schreibfehler(_VOID);
_VOID lesefehler(_VOID);
_VOID nichtgefunden(CONST _UCHAR *str);
_VOID nichtangelegt(CONST _UCHAR *str);
_BOOL ok_abbruch(CONST _UCHAR *str);
_BOOL abbruch_ok(CONST _UCHAR *str);
_BOOL weiter_abbruch(CONST _UCHAR *str);
_BOOL abbruch_weiter(CONST _UCHAR *str);
_WORD ok_retry_cancel(CONST _UCHAR *str);
_BOOL W_Keypressed(_WORD *key);

#endif /* __BOXF_H__ */
