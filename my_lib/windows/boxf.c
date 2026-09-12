/*****************************************************************************
 * WINDOWS/BOXF.C
 *****************************************************************************/

#include <boxf.h>
#include <windows_.h>
#include <stdio.h>
#include <w_mouse.h>
#include <openwork.h>
#include <ro_mem.h>

/*
 * UNICHAR is the type of characters used in Dialog resources.
 * for WIN32 applications, this is always WCHAR, regardless
 * of the UNICODE setting.
 */

#ifdef __WIN32__
#  define UNICHAR WCHAR
#else
#  define UNICHAR char
typedef struct {
	DWORD style;
	BYTE cdit;
	WORD x;
	WORD y;
	WORD cx;
	WORD cy;
} DLGTEMPLATE, *LPDLGTEMPLATE;
#endif


#ifndef DS_3DLOOK
#  define DS_3DLOOK 0x04
#endif
#ifndef IDCLOSE
#  define IDCLOSE   8
#endif
#ifndef IDHELP
#  define IDHELP    9
#endif

#define ID_OFFSET   100


#define ICON_APPLICATION    32512
#define ICON_HAND           32513
#define ICON_QUESTION       32514
#define ICON_EXCLAMATION    32515
#define ICON_ASTERISK       32516
#define ICON_WINLOGO        32517


#define ICON_WIDTH (6 * CHAR_WIDTH)
#define ICON_HEIGHT (3 * CHAR_HEIGHT)

#define LEFT_MARGIN (2 * CHAR_WIDTH)
#define TOP_MARGIN (CHAR_HEIGHT / 2)
#define BUTTON_DIST (2 * CHAR_WIDTH)

typedef struct {
	_UBYTE *ptr;
	_UBYTE *end;
	size_t used_size;
	size_t alloced_size;
	_WORD numitems;
	_WORD width;
	_WORD height;
	HGLOBAL hglbDlg;
} DLG_BUFFER;

#define CHAR_WIDTH 4
#define CHAR_HEIGHT 8

#define FO_ANZ_ZEILEN	10
#define FO_ANZ_BUTTON	5
#define FO_MAX_STR_LENG	80
#define FO_MAX_BUT_LENG	30
#define FO_MAX_SYM      3

#define FO_BUFSIZE	(1 + 4 + FO_ANZ_ZEILEN*FO_MAX_STR_LENG + (FO_ANZ_ZEILEN-1) + 2 + FO_ANZ_BUTTON*FO_MAX_BUT_LENG + (FO_ANZ_BUTTON-1) + 1 + 1)
/***                 |   |   |                               |                   |   |                               |                   |   |
*                    |   |   |                               |                   |   |                               |                   |   +- string terminator
*                    |   |   |                               |                   |   |                               |                   +- last bracket
*                    |   |   |                               |                   |   |                               +- button separators
*                    |   |   |                               |                   |   +- button strings
*                    |   |   |                               |                   +- separators
*                    |   |   |                               +- string separators
*                    |   |   +- strings
*                    |   +- symbol + separators
*                    +- default
***/

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

LOCAL _BOOL check_size(DLG_BUFFER *buffer, size_t size)
{
	HGLOBAL newDlg;
	_UBYTE *newptr;
	size_t new_size;
	
	if (buffer->used_size + size <= buffer->alloced_size)
		return TRUE;
	new_size = buffer->alloced_size + size + 1024;
	newDlg = GlobalAlloc(GMEM_FIXED, new_size);
	if (newDlg == NULL)
		return FALSE;
#ifdef __WIN32__
	newptr = newDlg;
#else
	newptr = GlobalLock(newDlg);
#endif
	MemCpy(newptr, buffer->ptr, buffer->alloced_size);
	GlobalUnlock(buffer->hglbDlg);
	GlobalFree(buffer->hglbDlg);
	buffer->ptr = newptr;
	buffer->hglbDlg = newDlg;
	buffer->end = newptr + buffer->used_size;
	buffer->alloced_size = new_size;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL add_buffer(DLG_BUFFER *buffer, _VOID *src, size_t size)
{
	if (!check_size(buffer, size))
		return FALSE;
	buffer->used_size += size;
	memcpy(buffer->end, src, size);
	buffer->end += size;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

#ifdef __WIN32__
LOCAL _BOOL align_buffer(DLG_BUFFER *buffer, size_t align)
{
	size_t pad;
	
	pad = buffer->used_size % align;
	if (pad != 0)
	{
		pad = align - pad;
		if (!check_size(buffer, pad))
			return FALSE;
		buffer->used_size += pad;
		while (pad--)
			*buffer->end++ = EOS;
	}
	return TRUE;
}
#endif

/*** ---------------------------------------------------------------------- ***/

LOCAL DLG_BUFFER *alloc_buffer(_VOID)
{
	DLG_BUFFER *buffer;
	
	buffer = MALLOC(sizeof(*buffer), "alloc_dlg_buffer");
	if (buffer == NULL)
		return NULL;
	buffer->alloced_size = 1024;
	buffer->used_size = 0;
	buffer->numitems = 0;
	buffer->hglbDlg = GlobalAlloc(GMEM_FIXED, buffer->alloced_size);
	if (buffer->hglbDlg == NULL)
	{
		OFREE(buffer);
		return NULL;
	}
#ifdef __WIN32__
	buffer->ptr = buffer->hglbDlg;
#else
	buffer->ptr = GlobalLock(buffer->hglbDlg);
#endif
	buffer->end = buffer->ptr;
	return buffer;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID free_buffer(DLG_BUFFER *buffer)
{
	GlobalUnlock(buffer->hglbDlg);
	GlobalFree(buffer->hglbDlg);
	OFREE(buffer);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL add_str(DLG_BUFFER *buffer, CONST _UCHAR *str)
{
	size_t len;
	size_t size;
	
	len = lstrlen(str);
	size = (len + 1) * sizeof(UNICHAR);
	if (!check_size(buffer, size))
		return FALSE;
#ifdef UNICODE
	memcpy(buffer->end, str, size);
#else
#ifdef __WIN32__
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str, len + 1, (_WCHAR *)buffer->end, len + 1);
#else
	lstrcpy(buffer->end, str);
#endif
#endif
	buffer->end += size;
	buffer->used_size += size;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL make_dialog(DLG_BUFFER *buffer, CONST _UCHAR *title, CONST _UCHAR *facename, WORD pointsize)
{
	DWORD style;
#ifdef __WIN32__
	WORD cdit;
	WORD menu;
	WORD class;
#else
	BYTE menu;
	BYTE cdit;
	BYTE class;
#endif
	WORD x, y, cx, cy;
	
	style = DS_MODALFRAME | DS_3DLOOK | WS_VISIBLE | WS_POPUP | WS_CHILD;
	if (title != NULL)
		style |= WS_CAPTION | WS_SYSMENU;
	else
		title = _T("");
	if (facename != NULL)
	{
		if (pointsize == 0)
			pointsize = 8;
		if (*facename == 0)
			facename = _T("Ms Sans Serif");
		style |= DS_SETFONT;
	}
	if (!add_buffer(buffer, &style, sizeof(style)))
		return FALSE;
#ifdef __WIN32__
	{
		DWORD exstyle;
		
		exstyle = WS_EX_DLGMODALFRAME;
		if (!add_buffer(buffer, &exstyle, sizeof(exstyle)))
			return FALSE;
	}
#endif
	cdit = 0;
	if (!add_buffer(buffer, &cdit, sizeof(cdit)))
		return FALSE;
	x = 0;
	if (!add_buffer(buffer, &x, sizeof(x)))
		return FALSE;
	y = 0;
	if (!add_buffer(buffer, &y, sizeof(y)))
		return FALSE;
	cx = 276;
	if (!add_buffer(buffer, &cx, sizeof(cx)))
		return FALSE;
	cy = 148;
	if (!add_buffer(buffer, &cy, sizeof(cy)))
		return FALSE;
	menu = 0;
	if (!add_buffer(buffer, &menu, sizeof(menu)))
		return FALSE;
	class = 0;
	if (!add_buffer(buffer, &class, sizeof(class)))
		return FALSE;
	if (!add_str(buffer, title))
		return FALSE;
	if (style & DS_SETFONT)
	{
		if (!add_buffer(buffer, &pointsize, sizeof(pointsize)))
			return FALSE;
		if (!add_str(buffer, facename))
			return FALSE;
	}
#ifdef __WIN32__
	if (!align_buffer(buffer, sizeof(DWORD)))
		return FALSE;
#endif
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL add_control(DLG_BUFFER *buffer, DWORD style, WORD x, WORD y, WORD w, WORD h, WORD id, UNICHAR type, CONST _UCHAR *text)
{
	UNICHAR idbuf[4];
#ifdef __WIN32__
	WORD class;
	DWORD exStyle;
	
	if (!add_buffer(buffer, &style, sizeof(style)))
		return FALSE;
	exStyle = 0;
	if (!add_buffer(buffer, &exStyle, sizeof(exStyle)))
		return FALSE;
#endif
	
	if (!add_buffer(buffer, &x, sizeof(x)))
		return FALSE;
	if (!add_buffer(buffer, &y, sizeof(y)))
		return FALSE;
	if (!add_buffer(buffer, &w, sizeof(w)))
		return FALSE;
	if (!add_buffer(buffer, &h, sizeof(h)))
		return FALSE;
	if (!add_buffer(buffer, &id, sizeof(id)))
		return FALSE;
#ifdef __WIN32__
	class = 0xffff;
	if (!add_buffer(buffer, &class, sizeof(class)))
		return FALSE;
#else
	if (!add_buffer(buffer, &style, sizeof(style)))
		return FALSE;
#endif
	if (!add_buffer(buffer, &type, sizeof(type)))
		return FALSE;
	if (text == NULL)
	{
		if (!add_str(buffer, _T("")))
			return FALSE;
	} else if (HIWORD(text) == 0)
	{
		idbuf[0] = (UNICHAR)-1;
		*(&idbuf[1]) = LOWORD(text);
		if (!add_buffer(buffer, idbuf, sizeof(UNICHAR) + 2))
			return FALSE;
	} else
	{
		if (!add_str(buffer, text))
			return FALSE;
	}
	if (!add_str(buffer, _T(""))) /* creation data?? */
		return FALSE;
#ifdef __WIN32__
	if (!align_buffer(buffer, sizeof(DWORD)))
		return FALSE;
#endif
	++buffer->numitems;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

#define add_button(buffer, style, x, y, w, h, id, text) \
	add_control(buffer, style | WS_VISIBLE | WS_CHILD | WS_TABSTOP, x, y, w, h, id, 0x0080, text)

#define add_edit(buffer, style, x, y, w, h, id, text) \
	add_control(buffer, style | WS_VISIBLE | WS_CHILD | WS_TABSTOP, x, y, w, h, id, 0x0081, text)

#define add_static(buffer, style, x, y, w, h, id, text) \
	add_control(buffer, style | WS_VISIBLE | WS_CHILD, x, y, w, h, id, 0x0082, text)

#define add_listbox(buffer, style, x, y, w, h, id, text) \
	add_control(buffer, style | WS_VISIBLE | WS_CHILD, x, y, w, h, id, 0x0083, text)

#define add_scrollbar(buffer, style, x, y, w, h, id, text) \
	add_control(buffer, style | WS_VISIBLE | WS_CHILD, x, y, w, h, id, 0x0084, text)

#define add_combobox(buffer, style, x, y, w, h, id, text) \
	add_control(buffer, style | WS_VISIBLE | WS_CHILD | WS_TABSTOP, x, y, w, h, id, 0x0085, text)


#define add_icon(buffer, style, x, y, w, h, id, type) \
	add_static(buffer, style | SS_ICON, x, y, w, h, id, (_UCHAR *)MAKEINTRESOURCE(type))

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID finish_dialog(DLG_BUFFER *buffer)
{
	LPDLGTEMPLATE dlg;
	
	dlg = (LPDLGTEMPLATE)buffer->ptr;
	dlg->cdit = buffer->numitems;
	dlg->cx = buffer->width;
	dlg->cy = buffer->height;
#if 0
	GlobalUnlock(buffer->hglbDlg);
#endif
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

LOCAL _BOOL Form_Alert_is_Str_Ok(CONST _UCHAR *str)
{
	if (str[0] == _T('[') &&
		str[1] >= _T('0') &&
#if 0
		str[1] <= _T('0') + MAX_ALERT_SYM &&
#endif
		str[2] == _T(']') &&
		str[3] == _T('[') &&
#if 0
		lstrlen(str) < FO_BUFSIZE &&
#endif
		(str = strchr(str + 4, _T(']'))) != NULL &&
		str[1] == _T('[') &&
		(str = strchr(str + 2, _T(']'))) != NULL &&
		str[1] == EOS)
		return TRUE;
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _WORD next_alert_string(_UCHAR **dstp, CONST _UCHAR **src, _WORD *maxlen, _BOOL is_button)
{
	_WORD len;
	CONST _UCHAR *cp;
	_UCHAR *dst;
	_UCHAR c;
	
	cp = *src;
	dst = *dstp;
	c = *cp;
	if (c == EOS || c == _T(']'))
	{
		len = -1;
	} else
	{
		len = 0;
		while ((c = *cp) != EOS)
		{
			if (c == _T('\\') &&
				(cp[1] == _T('[') || cp[1] == _T(']') || cp[1] == _T('\\') || cp[1] == _T('|')))
			{
				if ((*maxlen) > 1)
				{
					*dst++ = cp[1];
					(*maxlen)--;
					len++;
				}
				cp += 2;
			} else if (c == _T('|') || c == _T(']') || (c == _T('\n') && !is_button))
			{
				break;
			} else
			{
				if (is_button && c == _T('['))
				{
					c = _T('&');
				} else if (is_button && c == _T('&'))
				{
					if ((*maxlen) > 2)
					{
						*dst++ = c;
						(*maxlen)--;
					}
				}
				if ((*maxlen) > 1)
				{
					*dst++ = c;
					(*maxlen)--;
					len++;
				}
				cp++;
			}
		}
	}
	*dst++ = EOS;
	(*maxlen)--;
	*dstp = dst;
	if (c == _T('|') || c == _T('\n'))
		*src = cp + 1;
	else
		*src = cp;
	return len;
}

/*** ---------------------------------------------------------------------- ***/

typedef struct _alert_string
{
	_WORD   sym;
	_UCHAR *strtext[FO_ANZ_ZEILEN];
	_WORD   str_len[FO_ANZ_ZEILEN];
	_WORD   num_strings;
	_WORD   max_str_len;
	_UCHAR *buttext[FO_ANZ_BUTTON];
	_WORD   but_len[FO_ANZ_BUTTON];
	_WORD   num_buttons;
	_WORD   max_but_len;
	_WORD	default_button;
	_WORD	undo_button;
	_UBYTE  strbuf[FO_BUFSIZE];
} ALERT_STRING;

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL split_alert_string(_WORD def, ALERT_STRING *alert, CONST _UCHAR *str)
{
	_UCHAR *dst;
	_WORD i;
	_WORD len;
	_WORD maxlen;

	alert->sym = 0;
	alert->num_strings = 0;
	alert->max_str_len = 0;
	alert->num_buttons = 0;
	alert->max_but_len = 0;
	alert->default_button = def;
	alert->undo_button = 0;
	
	if (*str++ != '[')
		return FALSE;

	{
		_UBYTE sym = *str++;

		if (sym > FO_MAX_SYM + '0')
			sym = FO_MAX_SYM + '0';
		else if (sym < '0')
			sym = '0';
		alert->sym = sym - '0';
	}
	if (*str++ != ']')
		return FALSE;

	if (*str++ != '[')
		return FALSE;
	dst = alert->strbuf;
	maxlen = FO_BUFSIZE;
	for (i = 0; i < FO_ANZ_ZEILEN; i++)
	{
		alert->strtext[i] = dst;
		len = next_alert_string(&dst, &str, &maxlen, FALSE);
		if (len < 0)
			break;
		alert->str_len[i] = len;
		if (len > alert->max_str_len)
			alert->max_str_len = len;
	}
	alert->num_strings = i;
	if (alert->num_strings == 0)
	{
		alert->strtext[0] = dst;
		*dst++ = '\0';
		alert->str_len[0] = 0;
		alert->num_strings = 1;
	}
	if (*str++ != ']')
		return FALSE;

	if (*str++ != '[')
		return FALSE;
	for (i = 0; i < FO_ANZ_BUTTON; i++)
	{
		alert->buttext[i] = dst;
		if (*str == '.')
		{
			alert->default_button = i + 1;
			str++;
		} else if (*str == ':')
		{
			alert->undo_button = i + 1;
			str++;
		}
		len = next_alert_string(&dst, &str, &maxlen, TRUE);
		if (len < 0)
			break;
		alert->but_len[i] = len;
		if (maxlen > 5)
		{
			dst += 5;			/* leave room for function key */
			maxlen -= 5;
		}
		if (len > alert->max_but_len)
			alert->max_but_len = len;
	}
	alert->num_buttons = i;
	if (alert->num_buttons == 0)
	{
		alert->buttext[0] = dst;
		*dst++ = '\0';
		alert->but_len[0] = 0;
		alert->num_buttons = 1;
	}
	if (*str++ != ']')
		return FALSE;

	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL BOOL CALLBACK dialog_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	_WORD num_buttons;
	_WORD button;
	HWND owner;
	RECT r;
	RECT w;
	_WORD default_button, undo_button;
	
	UNUSED(wParam);
	switch (msg)
	{
	case WM_INITDIALOG:
		SetWindowLong(hwnd, DWL_USER, lParam);
		owner = GetParent(hwnd);
		if (owner == NO_WINDOW)
		{
			r.left = 0;
			r.top = 0;
			r.right = GetSystemMetrics(SM_CXSCREEN);
			r.bottom = GetSystemMetrics(SM_CYSCREEN);
		} else
		{
			GetClientRect(owner, &r);
		}
		GetWindowRect(hwnd, &w);
		SetWindowPos(hwnd, HWND_TOPMOST, (r.right - r.left) / 2 - ((w.right - w.left) / 2), (r.bottom - r.top) / 2 - ((w.bottom - w.top) / 2), w.right - w.left, w.bottom - w.top, SWP_NOSIZE);
		default_button = LOBYTE(HIWORD(lParam));
		if (default_button != 0)
		{
			SetFocus(GetDlgItem(hwnd, default_button + ID_OFFSET - 1));
			return FALSE;
		}
		return TRUE;
	case WM_COMMAND:
		lParam = GetWindowLong(hwnd, DWL_USER);
		num_buttons = LOWORD(lParam);
		default_button = LOBYTE(HIWORD(lParam));
		undo_button = HIBYTE(HIWORD(lParam));
		button = LOWORD(wParam);
		if (button >= ID_OFFSET && button < (ID_OFFSET + num_buttons))
			button -= ID_OFFSET - 1;
		else if (button == IDOK && default_button != 0)
			button = default_button;
		else if (button == IDCANCEL && undo_button != 0)
			button = undo_button;
		else if (num_buttons == 1)
			button = 1;
		else
			button = 0;
		SetWindowLong(hwnd, DWL_MSGRESULT, button);
		EndDialog(hwnd, button);
		return TRUE;
	}
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _WORD _Form_Alert(_WORD def, CONST _UBYTE *str)
{
	ALERT_STRING info;
	_WORD xoff, yoff, left_margin, top_margin;
	_WORD i;
	_WORD minw, minh;
	DWORD style;
	DLG_BUFFER *buffer;
	_UCHAR *title;
	_WORD w;
	HWND owner;
	
	split_alert_string(def, &info, str);
	if (info.default_button > 0)
	{
		if (info.default_button > info.num_buttons)
			info.default_button = 0;
	}
	switch (info.sym)
	{
	case 0:
		title = _T("Info");
		break;
	case 1:
		title = _T("Fehler");
		break;
	case 2:
		title = _T("Frage");
		break;
	case 3:
		title = _T("Halt");
		break;
	default:
		title = _T("");
		break;
	}
	buffer = alloc_buffer();
	if (buffer == NULL)
		return -1;
	if (!make_dialog(buffer, title, NULL, 0))
	{
		free_buffer(buffer);
		return -1;
	}
	
	left_margin = LEFT_MARGIN;
	top_margin = TOP_MARGIN;

	minh = 0;
	xoff = left_margin;
	yoff = top_margin;
	if (info.sym != 0)
	{
		_WORD type;
		
		switch (info.sym)
		{
			case 1: type = ICON_EXCLAMATION; break;
			case 2: type = ICON_QUESTION; break;
			case 3: type = ICON_HAND; break;
			default: type = ICON_ASTERISK; break;
		}
		if (!add_icon(buffer, 0, xoff, yoff, ICON_WIDTH, ICON_HEIGHT, -1, type))
		{
			free_buffer(buffer);
			return -1;
		}
		xoff += ICON_WIDTH + CHAR_WIDTH;
		minh = yoff + ICON_HEIGHT;
	}

	for (i = 0; i < info.num_strings; i++)
	{
		if (!add_static(buffer, SS_LEFT | SS_NOPREFIX, xoff, yoff, info.str_len[i] * CHAR_WIDTH, CHAR_HEIGHT, -1, info.strtext[i]))
		{
			free_buffer(buffer);
			return -1;
		}
		yoff += CHAR_HEIGHT;
	}
	if (yoff < minh)
		yoff = minh;
	
	/*
	 * Leerraum zwischen Text und Buttons
	 */
	yoff += CHAR_HEIGHT;
	
	minw = (info.max_str_len + 1) * CHAR_WIDTH + xoff + left_margin;

	w = (info.max_but_len + 3) * CHAR_WIDTH * info.num_buttons + (BUTTON_DIST * (info.num_buttons - 1)) + 2 * left_margin;
	xoff = left_margin;
	if (minw < w)
	{
		minw = w;
	} else
	{
		xoff = (minw - w) / 2 + left_margin;
	}

	for (i = 0; i < info.num_buttons; i++)
	{
		if (i != 0)
			xoff += BUTTON_DIST;
		style = (i + 1) == info.default_button ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON;
		w = (info.max_but_len + 3) * CHAR_WIDTH;
		if (!add_button(buffer, style, xoff, yoff, w, CHAR_HEIGHT * 2, i + ID_OFFSET, info.buttext[i]))
		{
			free_buffer(buffer);
			return -1;
		}
		xoff += w;
	}
	yoff += CHAR_HEIGHT * 2; /* Platz, den Buttons einnehmen */
	yoff += top_margin;
	
	buffer->width = minw;
	buffer->height = yoff;
	
	owner = GlMainHwnd;
	
	finish_dialog(buffer);

	SetMouse(MOUSE_DIALOG);
#ifdef __WIN32__
	/*
	 * the structure is the same, whether we use UNICODE or not,
	 * but Win9x doesn't support DialogBoxIndirectW
	 */
	def = DialogBoxIndirectParamA(GetInstance(), (LPDLGTEMPLATE)buffer->hglbDlg, owner, dialog_proc, MAKELPARAM(info.num_buttons, MAKEWORD(info.default_button, info.undo_button)));
#else
	def = DialogBoxIndirectParam(GetInstance(), buffer->hglbDlg, owner, dialog_proc, MAKELPARAM(info.num_buttons, MAKEWORD(info.default_button, info.undo_button)));
#endif
	free_buffer(buffer);

	SetMouse(MOUSE_RESTORE);
	return def;
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

LOCAL _WORD Form_Alert_Varargs(_WORD def, CONST _UCHAR *str, __c_va_list args)
{
	_UCHAR buf[FO_BUFSIZE + 1];

	wvsprintf(buf, str, args);
	return _Form_Alert(def, buf);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _WORD Form_Alert(_WORD def, CONST _UCHAR *str __c_va_alist)
{
	__c_va_list args;
	
	__c_va_start(args, str);
	def = Form_Alert_Varargs(def, str, args);
	__c_va_end(args);
	return def;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _WORD Form_Ok(CONST _UCHAR *str __c_va_alist)
{
	__c_va_list args;
	_WORD def;
	
	if (str == NULL)
		return -1;
	
	def = 1;

	if (!Form_Alert_is_Str_Ok(str))
	{
		_UCHAR box[FO_BUFSIZE];
		
		__c_va_start(args, str);
		wvsprintf(box, str, args);
		__c_va_end(args);
		def = Form_Alert(def, _T("[1][%s][.[OK]"), box);
	} else
	{
		__c_va_start(args, str);
		def = Form_Alert_Varargs(def, str, args);
		__c_va_end(args);
	}
	return def;
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL _VOID ok(CONST _UCHAR *str)
{
	Form_Ok(str);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID abbruch(CONST _UCHAR *str)
{
	Form_Alert(1, _T("[1][|%s|][:[Abbruch]"), str);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID ende(CONST _UCHAR *str)
{
	Form_Alert(1, _T("[1][|%s|][:[Abbruch]"), str);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID schreibfehler(_VOID)
{
	abbruch(_T("Schreibfehler!"));
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID lesefehler(_VOID)
{
	abbruch(_T("Lesefehler!"));
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID nichtgefunden(CONST _UCHAR *str)
{
	Form_Alert(1, _T("[1][|Datei '%s' nicht gefunden!|][:[Abbruch]"), str);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID nichtangelegt(CONST _UCHAR *str)
{
	Form_Alert(1, _T("[1][|Kann Datei '%s' nicht anlegen!|][:[Abbruch]"), str);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL ok_abbruch(CONST _UCHAR *str)
{
	return Form_Alert(1, _T("[2][|%s|][.[Ok|:[Abbruch]"), str) == 1;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL abbruch_ok(CONST _UCHAR *str)
{
	return Form_Alert(2, _T("[2][|%s|][[Ok|:[Abbruch]"), str) == 1;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL weiter_abbruch(CONST _UCHAR *str)
{
	return Form_Alert(1, _T("[2][|%s|][.[Weiter|:[Abbruch]"), str) == 1;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL abbruch_weiter(CONST _UCHAR *str)
{
	return Form_Alert(2, _T("[2][|%s|][[Weiter|:[Abbruch]"), str) == 1;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _WORD ok_retry_cancel(CONST _UCHAR *str)
{
	return Form_Alert(1, _T("[1][%s][.[OK|[Nochmal|:[Abbruch]"), str);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL W_Keypressed(_WORD *key)
{
	MSG msg;
	
	UNUSED(key);
	if (PeekMessage(&msg, NO_WINDOW, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE))
		return TRUE;
	return FALSE;
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

#ifdef MAIN
HINSTANCE GlhInstance;
HWND GlMainHwnd;

LOCAL _VOID test(_WORD res)
{
	_UCHAR buf[200];
	
	wsprintf(buf, _T("%d"), res);
	MessageBox(NULL, buf, NULL, MB_TASKMODAL | MB_OK);
}

/*** ---------------------------------------------------------------------- ***/

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
	GlhInstance = hInstance;
	UNUSED(hPrevInstance);
	UNUSED(lpszCmdLine);
	UNUSED(nCmdShow);

	test(Form_Alert(2, _T("[2][Soll der Ausgabepuffer neu erzeugt oder weiterbenutzt|werden ?][:[Neu erzeugen|.[Weiterbenutzen]")));

#if 0
	test(Form_Alert(2, _T("[2][Rezept-Medikamente vorhanden.][aa|.Löschen|:Abbruch]")));
	test(Form_Alert(1, _T("[2][Rezept-Medikamente vorhanden|Bla d  kldk lkds kd sk lka<s kl.][Löschen|Abbruch]")));
	test(Form_Alert(0, _T("[0][Rezept-Medikamente|vorhanden.][Neu erzeugen|Löschen|Weiterbenutzen|Abbruch]")));
	test(Form_Alert(0, _T("[0][Rezept-Medikamente|vorhanden.][Abbruch]")));
	test(Form_Alert(0, _T("[1][Rezept-Medikamente|vorhanden.][Abbruch]")));
	test(Form_Alert(0, _T("[2][Rezept-Medikamente||vorhanden.][Abbruch]")));
	test(Form_Alert(0, _T("[3][Rezept-Medikamente|vorhanden.][Abbruch]")));
	test(Form_Alert(0, _T("[4][Rezept-Medikamente|vorhanden.][Abbruch]")));
	test(Form_Alert(0, _T("[5][Rezept-Medikamente|vorhanden.][Abbruch]")));
	test(Form_Alert(0, _T("[6][Rezept-Medikamente||vorhanden.][Abbruch]")));
	test(Form_Alert(0, _T("[7][Rezept-Medikamente|vorhanden.][Abbruch]")));
	test(Form_Alert(0, _T("[8][Rezept-Medikamente|vorhanden.][Abbruch]")));
	test(Form_Alert(0, _T("[9][Rezept-Medikamente|vorhanden.][Abbruch]")));
	test(Form_Ok(_T("Nach dem Lauf kann die|Patientendatei nicht mehr|für den normalen Betrieb|benutzt werden.")));
#endif

	return 0;
}

#endif /* MAIN */
