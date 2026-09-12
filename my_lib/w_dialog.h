#ifndef __W_DIALOG_H__
#define __W_DIALOG_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif

#ifndef __DIALOG_IMPLEMENTATION__
DUMMY_STRUCT(_dialog);
#endif

typedef struct _dialog DIALOG;

typedef enum _dlg_return {
	DLG_CONTINUE,
	DLG_END,
	DLG_IGNORE
} DLG_RETURN;

typedef DLG_RETURN (*DIALOG_PROC)(DIALOG *ptr, _WORD button, _BOOL *ret, _VOID *para);

#define DO_INIT -1
#define DO_EXIT -2

DIALOG *Dialog_Show(_WORD id, DIALOG_PROC proc, _BOOL modal, _VOID *para);
_VOID Dialog_Hide(DIALOG *dialog);
_BOOL Dialog_Run(DIALOG *ptr);
_BOOL Dialog_Select(_WORD id, DIALOG_PROC proc, _VOID *para);

_VOID Dialog_SetStr(DIALOG *dialog, _WORD id, _UBYTE *str);
_VOID Dialog_SetLong(DIALOG *dialog, _WORD id, _LONG val, _WORD just);
#define Dialog_SetInt(dialog, id, val, just) Dialog_SetLong(dialog, id, (_LONG)(val), just)
_VOID Dialog_SetBool(DIALOG *dialog, _WORD id, _BOOL val);
_VOID Dialog_SetPopup(DIALOG *dialog, _WORD id, CONST _UBYTE *CONST *strings, _WORD n_strings, _WORD val);

_LONG Dialog_GetLong(DIALOG *dialog, _WORD id);
#define Dialog_GetInt(dialog, id) (_WORD)Dialog_GetLong(dialog, id)
_BOOL Dialog_GetBool(DIALOG *dialog, _WORD id);
_VOID Dialog_GetStr(DIALOG *dialog, _WORD id, _UBYTE *str, _UWORD maxstr);
_WORD Dialog_GetPopup(DIALOG *dialog, _WORD id);

_BOOL Dialog_SetCurr(DIALOG *dialog, _WORD id);
_VOID Dialog_Enable(DIALOG *dialog, _WORD id, _BOOL enable);

#endif /* __W_DIALOG_H__ */
