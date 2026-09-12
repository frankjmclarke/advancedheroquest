/*****************************************************************************
 * WINDOWS/W_DIALOG.C
 *****************************************************************************/

#define __DIALOG_IMPLEMENTATION__
#include <w_dialog.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows_.h>
#include <openwork.h>
#include <routine.h>
#include <ro_mem.h>
#include <grect.h>

struct _dialog {
	HWND hdlg;
	DIALOG_PROC proc;
	_VOID *para;
	_BOOL retV;
	_WORD edit;
	_BOOL modal;
};

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL _VOID Dialog_SetStr(DIALOG *dialog, _WORD id, _UBYTE *str)
{
	SetDlgItemText(dialog->hdlg, id, str);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Dialog_GetStr(DIALOG *dialog, _WORD id, _UBYTE *str, _UWORD maxlen)
{
	int len;
	
	len = GetDlgItemText(dialog->hdlg, id, str, maxlen - 1);
	str[len] = '\0';
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Dialog_SetLong(DIALOG *dialog, _WORD id, _LONG val, _WORD just)
{
	_UBYTE buf[20];
	
	if (just != 0)
		sprintf(buf, "%*ld", just, val);
	else
		sprintf(buf, "%ld", val);
	Dialog_SetStr(dialog, id, buf);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _LONG Dialog_GetLong(DIALOG *dialog, _WORD id)
{
	_UBYTE buf[20];
	
	Dialog_GetStr(dialog, id, PtrSize(buf));
	return atol(buf);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Dialog_SetBool(DIALOG *dialog, _WORD id, _BOOL val)
{
	CheckDlgButton(dialog->hdlg, id, val);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL Dialog_GetBool(DIALOG *dialog, _WORD id)
{
	return IsDlgButtonChecked(dialog->hdlg, id);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Dialog_SetPopup(DIALOG *dialog, _WORD id, CONST _UBYTE *CONST *strings, _WORD n_strings, _WORD val)
{
	_WORD i;
	
	SendDlgItemMessage(dialog->hdlg, id, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
	for (i = 0; i < n_strings; i++)
		SendDlgItemMessage(dialog->hdlg, id, CB_ADDSTRING, (WPARAM)0, (LPARAM)strings[i]);
	if (val >= 0 && val < n_strings)
		SendDlgItemMessage(dialog->hdlg, id, CB_SETCURSEL, (WPARAM)val, (LPARAM)0);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _WORD Dialog_GetPopup(DIALOG *dialog, _WORD id)
{
	return (_WORD)SendDlgItemMessage(dialog->hdlg, id, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL Dialog_SetCurr(DIALOG *dialog, _WORD id)
{
	HWND hwnd;
	
	hwnd = GetDlgItem(dialog->hdlg, id);
	if (hwnd != NO_WINDOW)
	{
		SetFocus(hwnd);
#ifdef __WIN32__
		SendDlgItemMessage(dialog->hdlg, id, EM_SETSEL, 0, -1);
#else
		SendDlgItemMessage(dialog->hdlg, id, EM_SETSEL, 0, MAKELONG(0, -1));
#endif
		dialog->edit = id;
		return TRUE;
	}
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Dialog_Enable(DIALOG *dialog, _WORD id, _BOOL enable)
{
	HWND hwnd;
	
	hwnd = GetDlgItem(dialog->hdlg, id);
	if (hwnd != NO_WINDOW)
	{
		EnableWindow(hwnd, enable);
	}
}

/*** ---------------------------------------------------------------------- ***/

LOCAL BOOL CALLBACK dialog_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DIALOG *dialog;
	_WORD id;
	_BOOL retV;
	
	switch (message)
	{
	case WM_CREATE:
		break;
	
	case WM_SETFONT:
		return TRUE;
	
	case WM_ACTIVATEAPP:
		break;
		
	case WM_INITDIALOG:
		SetWindowLong(hwnd, DWL_USER, lParam);
		dialog = (DIALOG *)GetWindowLong(hwnd, DWL_USER);
		if (dialog != NULL)
		{
			dialog->hdlg = hwnd;
			if (dialog->proc != FUNK_NULL)
			{
				if (dialog->proc(dialog, DO_INIT, &dialog->retV, dialog->para) == DLG_END)
				{
					dialog->retV = FALSE;
					EndDialog(hwnd, dialog->retV);
					DestroyWindow(hwnd);
				}
			}
			{
				GRECT gr, gr2;
				RECT r, r2;
				
				GetWindowRect(GlMainHwnd, &r);
				RectToGrect(&gr, &r);
				GetWindowRect(hwnd, &r2);
				RectToGrect(&gr2, &r2);
				gr2.g_x = gr.g_x + gr.g_w / 2 - gr2.g_w / 2;
				gr2.g_y = gr.g_y + gr.g_h / 2 - gr2.g_h / 2;
				GrectToRect(&r2, &gr2);
				MoveWindow(hwnd, r2.left, r2.top, r2.right - r2.left, r2.bottom - r2.top, TRUE);
			}
			if (dialog->edit != -1)
				return FALSE;
		}
		return TRUE;
		
	case WM_COMMAND:
		id = LOWORD(wParam);
		dialog = (DIALOG *)GetWindowLong(hwnd, DWL_USER);
		if (dialog != NULL)
		{
			dialog->edit = id;
			if (dialog->proc != FUNK_NULL)
				if (dialog->proc(dialog, id, &dialog->retV, dialog->para) == DLG_END)
				{
					EndDialog(hwnd, dialog->retV);
					DestroyWindow(hwnd);
				}
		}
		break;
	
	case WM_SETFOCUS:
		break;
	
	case WM_KILLFOCUS:
		break;
	
	case WM_CLOSE:
		dialog = (DIALOG *)GetWindowLong(hwnd, DWL_USER);
		if (dialog != NULL)
			if (dialog->proc != FUNK_NULL)
				dialog->proc(dialog, DO_EXIT, &retV, dialog->para);
		EndDialog(hwnd, dialog->retV);
		DestroyWindow(hwnd);
		return TRUE;
		
	case WM_SYSCOLORCHANGE:
		break;

	case WM_DESTROY:
		dialog = (DIALOG *)GetWindowLong(hwnd, DWL_USER);
		if (dialog != NULL)
			dialog->hdlg = NO_WINDOW;
		return FALSE;
	
	case WM_NCDESTROY:
		break;
	
	case WM_TIMER:
		break;
		
	default:
		break;
	}
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL Dialog_Run(DIALOG *dialog)
{
	MSG msg;
	
	if (!dialog->modal)
	{
		while (GetMessage(&msg, NULL, 0, 0) &&
			dialog->hdlg != NO_WINDOW)
		{
			if (!IsDialogMessage(dialog->hdlg, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}	
	return dialog->retV;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL DIALOG *Dialog_Show(_WORD id, DIALOG_PROC proc, _BOOL modal, _VOID *para)
{
	DIALOG *dialog;
	
	dialog = NEW(DIALOG, "Dialog_Show");
	if (dialog != NULL)
	{
		dialog->proc = proc;
		dialog->retV = FALSE;
		dialog->edit = -1;
		dialog->para = para;
		dialog->modal = modal;
		if (modal)
		{
			DialogBoxParam(GetInstance(), MAKEINTRESOURCE(id), GlMainHwnd, dialog_proc, (LPARAM)dialog);
		} else
		{
			dialog->hdlg = CreateDialogParam(GetInstance(), MAKEINTRESOURCE(id), GlMainHwnd, dialog_proc, (LPARAM)dialog);
			if (dialog->hdlg == NO_WINDOW)
			{
				OFREE(dialog);
				dialog = NULL;
			} else
			{
				ShowWindow(dialog->hdlg, SW_SHOW);
			}
		}
	}
	return dialog;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Dialog_Hide(DIALOG *dialog)
{
	if (dialog != NULL)
	{
		if (dialog->hdlg != NO_WINDOW)
			SendMessage(dialog->hdlg, WM_CLOSE, 0, 0);
		OFREE(dialog);
	}
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL Dialog_Select(_WORD id, DIALOG_PROC proc, _VOID *para)
{
	DIALOG *dialog;
	_BOOL retV = FALSE;
	
	dialog = Dialog_Show(id, proc, TRUE, para);
	if (dialog != NULL)
	{
		retV = Dialog_Run(dialog);
		Dialog_Hide(dialog);
	}
	return retV;
}
