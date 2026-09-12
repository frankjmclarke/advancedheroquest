/*****************************************************************************
 * WINDOWS/W_DRAW.C
 *****************************************************************************/

#include <w_draw.h>
#include <windows_.h>
#include <grect.h>
#include <openwork.h>
#include <routine.h>
#include <ro_mem.h>

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL _VOID W_Clear_Rect(WINDOW_DEF *wr, CONST GRECT *gr)
/* fuellt ein Rechteck WHITE */
{
	if (gr->g_w >= 0 && gr->g_h >= 0)
		PatBlt(W_GetDC(wr), gr->g_x, gr->g_y, gr->g_w, gr->g_h, WHITENESS);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID W_ScrollGrect(WINDOW_DEF *wr, _WORD diffX, _WORD diffY, CONST GRECT *to, REDRAW_FUNC func, _VOID *para)
{
	RECT re, clip;
	GRECT togr, area;
	HWND hwnd;

	hwnd = W_GetHwnd(wr);
	UpdateWindow(hwnd);
	GetClientRect(hwnd, &re);
	RectToGrect(&togr, &re);
	if (rc_intersect(to, &togr))
	{
		_WORD fromX, fromY;
		_WORD absX, absY;

		rc_copy(&togr, &area);
		GrectToRect(&clip, &togr);
		fromX = togr.g_x;
		fromY = togr.g_y;
		if (diffX > 0)
			togr.g_x += (absX = diffX);
		else
			fromX += (absX = -diffX);
		if (diffY > 0)
			togr.g_y += (absY = diffY);
		else
			fromY += (absY = -diffY);

		if (togr.g_w > absX && togr.g_h > absY)
		{
#if 0
			togr.g_x = fromX;
			togr.g_y = fromY;
			togr.g_w -= absX;
			togr.g_h -= absY;
			GrectToRect(&re, &togr);
			ScrollWindowEx(hwnd, diffX, diffY, &re, &clip, NULL, NULL, 0);
			togr.g_x = to->g_x;
			if (diffX < 0)
				togr.g_x = to->g_x + to->g_w - absX;
			togr.g_y = to->g_y;
			if (diffY < 0)
				togr.g_y = to->g_y + to->g_h - absY;
			togr.g_w = diffX == 0 ? to->g_w : absX;
			togr.g_h = diffY == 0 ? to->g_h : absY;
			W_Clip_Rect(os, TRUE, &togr);
			func(para, &togr);
			W_Clip_Rect(os, FALSE, &togr);
#else
			{
				RECT update;

				UNUSED(fromX);
				UNUSED(fromY);
				GrectToRect(&re, &area);
				ScrollWindowEx(hwnd, diffX, diffY, &re, &clip, NO_REGION, &update, SW_INVALIDATE);
				UpdateWindow(hwnd);
			}
#endif
		} else
		{
			func(para, &area);
		}
	} else
	{
		rc_copy(to, &togr);
		func(para, &togr);
	}
}

/* -------------------------------------------------------------------------- */

LOCAL _BOOL w_font_found(LOGFONT *lf, HDC hDC, _WORD *w, _WORD *h, HFONT *handle, _WORD *points)
{
	_BOOL result = TRUE;
	HFONT oldFont;
	TEXTMETRIC tm;
	_WORD newh;
	_LONG resy;
	int oldMapMode;
	
	resy = GetDeviceCaps(hDC, LOGPIXELSY);
	lf->lfHeight = -(int)((((_LONG)lf->lfHeight * resy / 72l) + 5) / 10);

	lf->lfQuality = DRAFT_QUALITY;
	lf->lfOutPrecision = OUT_DEFAULT_PRECIS;
	*handle = CreateFontIndirect(lf);
	if (*handle == NO_FONT || GetObject(*handle, sizeof(*lf), lf) == 0)
	{
		return FALSE;
	}
	oldMapMode = SetMapMode(hDC, MM_TEXT);
	oldFont = SelectObject(hDC, *handle);
	if (GetTextMetrics(hDC, &tm))
	{
		_UBYTE facename[LF_FACESIZE];
		_BOOL ok;

		newh = tm.tmHeight + tm.tmExternalLeading;
		ok = GetTextFace(hDC, sizeof(facename), facename) != 0;
		if (!ok || strnCaseCmp(facename, lf->lfFaceName, strlen(lf->lfFaceName)) != 0)
		{
			HFONT font;

			lf->lfQuality = DRAFT_QUALITY;
			lf->lfOutPrecision = OUT_TT_PRECIS;
			font = CreateFontIndirect(lf);
			if (font != NO_FONT)
			{
				SelectObject(hDC, font);
				if (GetObject(font, sizeof(*lf), lf) != 0 &&
					GetTextMetrics(hDC, &tm))
				{
					DeleteObject(*handle);
					*handle = font;
					newh = tm.tmHeight + tm.tmExternalLeading;
					ok = GetTextFace(hDC, sizeof(facename), facename) != 0;
					if (!ok || strnCaseCmp(facename, lf->lfFaceName, strlen(lf->lfFaceName)) != 0)
					{
						lf->lfQuality = DRAFT_QUALITY;
						lf->lfOutPrecision = OUT_DEFAULT_PRECIS;
						font = CreateFontIndirect(lf);
						if (font != NO_FONT)
						{
							SelectObject(hDC, font);
							if (GetObject(font, sizeof(*lf), lf) != 0 &&
								GetTextMetrics(hDC, &tm))
							{
								DeleteObject(*handle);
								*handle = font;
								newh = tm.tmHeight + tm.tmExternalLeading;
								ok = GetTextFace(hDC, sizeof(facename), facename) != 0;
								if (!ok || strnCaseCmp(facename, lf->lfFaceName, strlen(lf->lfFaceName)) != 0)
								{
									if (oldFont != NO_FONT)
										SelectObject(hDC, oldFont);
									oldFont = NO_FONT;
									DeleteObject(font);
									*handle = NO_FONT;
									result = FALSE;
								}
							} else
							{
								SelectObject(hDC, *handle);
								DeleteObject(font);
							}
						}
					}
				} else
				{
					SelectObject(hDC, *handle);
					DeleteObject(font);
				}
			}
		}
		*w = tm.tmAveCharWidth;
		*h = newh;
	} else
	{
		result = FALSE;
	}
	SetMapMode(hDC, oldMapMode);
	if (oldFont != NO_FONT)
		SelectObject(hDC, oldFont);
	newh = lf->lfHeight;
	if (newh < 0)
		newh = -newh;
	newh = (_WORD)(((((_LONG)newh * 720l / resy) + 5) / 10) * 10);
	*points = newh;
	if (tm.tmPitchAndFamily & TMPF_FIXED_PITCH)
		lf->lfPitchAndFamily = VARIABLE_PITCH;
	else
		lf->lfPitchAndFamily = FIXED_PITCH;
	return result;
}

/* -------------------------------------------------------------------------- */

OS_GLOBAL HFONT W_GetFont(HDC hdc, _WORD height, _WORD *w, _WORD *h)
{
	LOGFONT lf;
	HFONT hfont;
	_WORD found;
	_WORD points;
	
	MemSetZeroStruct(lf);
	lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	lf.lfCharSet = ANSI_CHARSET;
	*w = 8;
	if (height >= FONT_SIZE_NORMAL)
	{
		lf.lfWidth = 0;
		lf.lfHeight = FONT_SIZE_NORMAL;
		*h = 16;
	} else if (height >= FONT_SIZE_SMALL)
	{
		lf.lfWidth = 0;
		lf.lfHeight = FONT_SIZE_SMALL;
		*h = 8;
	} else
	{
		lf.lfWidth = 0;
		lf.lfHeight = FONT_SIZE_ICON;
		*h = 6;
	}
	strcpy(lf.lfFaceName, "Courier New");
	lf.lfWeight = FW_NORMAL;
	lf.lfUnderline = FALSE;
	lf.lfItalic = FALSE;
	
	if (!w_font_found(&lf, hdc, w, h, &hfont, &points))
	{
		found = 2;
		if (height <= FONT_SIZE_SMALL)
			strcpy(lf.lfFaceName, "Small Fonts");
		else
			strcpy(lf.lfFaceName, "Courier New");
		/* lf.lfQuality = DRAFT_QUALITY; */
		lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
		lf.lfCharSet = ANSI_CHARSET;
		if (!w_font_found(&lf, hdc, w, h, &hfont, &points))
		{
			strcpy(lf.lfFaceName, "Fixedsys");
			lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
			lf.lfCharSet = ANSI_CHARSET;
			if (!w_font_found(&lf, hdc, w, h, &hfont, &points))
			{
				strcpy(lf.lfFaceName, "System");
				lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
				lf.lfCharSet = ANSI_CHARSET;
				if (!w_font_found(&lf, hdc, w, h, &hfont, &points))
				{
					found = 0;
				}
			}
		}
	} else
	{
		found = 1;
	}

	if (found == 0)
	{
		hfont = GetStockObject(SYSTEM_FIXED_FONT);
		GetObject(hfont, sizeof(lf), &lf);
		points = FONT_SIZE_NORMAL;
	}
	return hfont;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID W_GetFontSize(WINDOW_DEF *window, _WORD size, _WORD *w, _WORD *h)
{
	HDC hdc;
	HFONT font;
	_BOOL release_dc;
	
	hdc = W_GetDC(window);
	if (hdc == NO_DC)
	{
		hdc = GetDC(HWND_DESKTOP);
		release_dc = TRUE;
	} else
	{
		release_dc = FALSE;
	}
	font = W_GetFont(hdc, size, w, h);
	if (font != NO_FONT)
		DeleteObject(font);
	if (release_dc)
		ReleaseDC(HWND_DESKTOP, hdc);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID W_Text(WINDOW_DEF *window, _WORD x, _WORD y, _WORD size, CONST _UBYTE *text, size_t len)
{
	HDC hdc;
	HFONT font, oldFont;
	_WORD w, h;
	int oldMapMode;
	
	hdc = W_GetDC(window);
	if (hdc == NO_DC)
		return;
	font = W_GetFont(hdc, size, &w, &h);
	if (font != NO_FONT)
	{
		oldMapMode = SetMapMode(hdc, MM_TEXT);
		oldFont = SelectObject(hdc, font);
		SetTextColor(hdc, W_PAL_BLACK);
		SetBkMode(hdc, OPAQUE);
		TextOut(hdc, x, y, text, len);
		SelectObject(hdc, oldFont);
		DeleteObject(font);
		SetMapMode(hdc, oldMapMode);
	}
}
