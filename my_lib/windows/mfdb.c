/*****************************************************************************
 * WINDOWS/MFDB.C
 *****************************************************************************/

#define __MFDB_IMPLEMENTATION__
#include <mfdb.h>
#include <windows_.h>
#include <w_draw.h>
#include <ro_mem.h>
#include <openwork.h>

struct _mfdb {
	HBITMAP bmp;
	BITMAP ic;
	_BOOL is_resource;
	_LONG size;
	HDC hDC;
	DWORD *atlas_pixels;
};

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

/*
 * Ink and paper for the next blit. Defaults reproduce the original
 * black-on-white rendering exactly.
 */
LOCAL COLORREF gl_ink = W_PAL_BLACK;
LOCAL COLORREF gl_paper = W_PAL_WHITE;

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID set_mfdb_colors(_LONG ink, _LONG paper)
{
	gl_ink = (COLORREF)ink;
	gl_paper = (COLORREF)paper;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID reset_mfdb_colors(_VOID)
{
	gl_ink = W_PAL_BLACK;
	gl_paper = W_PAL_WHITE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID draw_mfdb(MFDB *src, _WORD x, _WORD y, _WORD w, _WORD h, MFDB *dest, _WORD dx, _WORD dy, _WORD mode)
{
	COLORREF oldColor;
	COLORREF oldBk;
	HDC hdcMem;
	POINT ptSize, ptOrg;
	DWORD rop;
	
	if ((x + w) > src->ic.bmWidth)
		w = src->ic.bmWidth - x;
	if (w <= 0)
		return;
	if ((y + h) > src->ic.bmHeight)
		h = src->ic.bmHeight - y;
	if (h <= 0)
		return;
	switch (mode)
	{
		case MD_REPLACE: rop = SRCCOPY; break;
		case MD_TRANS: rop = SRCAND; break;
		case MD_ERASE: /* NOT IMPLEMENTED */
		case MD_XOR: /* NOT IMPLEMENTED */
		default: return;
	}
	oldColor = SetTextColor(dest->hDC, gl_ink);
	oldBk = SetBkColor(dest->hDC, gl_paper);
	
	hdcMem = CreateCompatibleDC(dest->hDC);
	SelectObject(hdcMem, src->bmp);
	
	ptSize.x = w;
	ptSize.y = h;
	DPtoLP(dest->hDC, &ptSize, 1);
	ptOrg.x = x;
	ptOrg.y = y;
	DPtoLP(dest->hDC, &ptOrg, 1);
	
	BitBlt(dest->hDC, dx, dy, ptSize.x, ptSize.y, hdcMem, ptOrg.x, ptOrg.y, rop);
	DeleteDC(hdcMem);
	SetTextColor(dest->hDC, oldColor);
	SetBkColor(dest->hDC, oldBk);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID free_mfdb(MFDB *ptr)
{
	if (ptr != NULL)
	{
		if (ptr->hDC != NO_DC)
			DeleteDC(ptr->hDC);
		if (ptr->bmp != NO_BITMAP)
			DeleteObject(ptr->bmp);
		if (ptr->atlas_pixels != NULL)
			FREE(ptr->atlas_pixels, ptr->ic.bmWidth * ptr->ic.bmHeight * 4);
		if (ptr->ic.bmBits != NULL)
			FREE(ptr->ic.bmBits, ptr->size);
		OFREE(ptr);
	}
}

/*** ---------------------------------------------------------------------- ***/

LOCAL MFDB *alloc_mfdb(VOID)
{
	MFDB *ptr;
	
	if ((ptr = NEW(MFDB, "alloc_mfdb")) == NULL)
	{
		return NULL;
	}
	ptr->bmp = NO_BITMAP;
	ptr->ic.bmWidth = 0;
	ptr->ic.bmHeight = 0;
	ptr->ic.bmBits = NULL;
	ptr->is_resource = FALSE;
	ptr->hDC = NO_DC;
	ptr->atlas_pixels = NULL;
	return ptr;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL MFDB *get_mfdb(_WORD w, _WORD h, _WORD planes, _VOID *data)
{
	MFDB *mfdb;
	
	mfdb = alloc_mfdb();
	if (mfdb != NULL)
	{
		mfdb->ic.bmType = 0;
		mfdb->ic.bmWidth = w;
		mfdb->ic.bmHeight = h;
		mfdb->ic.bmWidthBytes = ((w + 15) / 16) * 2;
		mfdb->ic.bmPlanes = 1;
		mfdb->ic.bmBitsPixel = planes;
		mfdb->size = mfdb->ic.bmWidthBytes * mfdb->ic.bmHeight * planes;
		mfdb->ic.bmBits = MALLOC(mfdb->size, "get_mfdb: bits");
		if (mfdb->ic.bmBits == NULL)
		{
			free_mfdb(mfdb);
			mfdb = NULL;
		} else
		{
			if (data != NULL)
				MemCpy(mfdb->ic.bmBits, data, mfdb->size);
			else
				MemSet(mfdb->ic.bmBits, 0xff, mfdb->size);
			if (planes == 1)
			{
				mfdb->bmp = CreateBitmapIndirect(&mfdb->ic);
			} else
			{
				/*
				 * Colour: a screen-compatible bitmap, cleared to white so the
				 * unused margin matches the paper colour of the tiles. The
				 * bmBits buffer is not kept in step for colour bitmaps, which
				 * is why callers that need the pixels (saving, printing)
				 * render into a 1 bit deep bitmap of their own instead.
				 */
				HDC screen = GetDC(HWND_DESKTOP);

				if (screen != NO_DC)
				{
					mfdb->bmp = CreateCompatibleBitmap(screen, w, h);
					if (mfdb->bmp != NO_BITMAP)
					{
						HDC mem = CreateCompatibleDC(screen);

						if (mem != NO_DC)
						{
							HBITMAP old = SelectObject(mem, mfdb->bmp);
							RECT r;

							r.left = 0;
							r.top = 0;
							r.right = w;
							r.bottom = h;
							FillRect(mem, &r, GetStockObject(WHITE_BRUSH));
							SelectObject(mem, old);
							DeleteDC(mem);
						}
					}
					ReleaseDC(HWND_DESKTOP, screen);
				}
			}
			if (mfdb->bmp == NO_BITMAP)
			{
				free_mfdb(mfdb);
				mfdb = NULL;
			}
		}
	}
	return mfdb;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL MFDB *get_mfdb_from_bitmap(_WORD id)
{
	MFDB *mfdb;
	
	mfdb = alloc_mfdb();
	if (mfdb != NULL)
	{
		mfdb->bmp = LoadBitmap(GetInstance(), MAKEINTRESOURCE(id));
		if (mfdb->bmp == NO_BITMAP)
		{
			free_mfdb(mfdb);
			mfdb = NULL;
		} else
		{
			GetObject(mfdb->bmp, sizeof(mfdb->ic), &mfdb->ic);
			mfdb->ic.bmBits = NULL;
			mfdb->is_resource = TRUE;
		}
	}
	return mfdb;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID get_mfdb_info(MFDB *mfdb, _WORD *w, _WORD *h, _VOID **data)
{
	if (w != NULL)
		*w = mfdb->ic.bmWidth;
	if (h != NULL)
		*h = mfdb->ic.bmHeight;
	if (data != NULL)
	{
		if (mfdb->ic.bmBits != NULL)
		{
			if (mfdb->ic.bmBitsPixel == 1)
				GetBitmapBits(mfdb->bmp, mfdb->size, mfdb->ic.bmBits);
			/* else
				GetDiBits(), TODO */
		}
		*data = mfdb->ic.bmBits;
	}
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL mfdb_start_paint(MFDB *mfdb)
{
	if (mfdb->hDC != NO_DC)
		return FALSE;
	mfdb->hDC = CreateCompatibleDC(GetDC(HWND_DESKTOP));
	SetTextColor(mfdb->hDC, W_PAL_BLACK);
	SetBkColor(mfdb->hDC, W_PAL_WHITE);
	SelectObject(mfdb->hDC, mfdb->bmp);
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID mfdb_end_paint(MFDB *mfdb)
{
	DeleteDC(mfdb->hDC);
	mfdb->hDC = NO_DC;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID W_Draw_Bitmap(_VOID /* WINDOW_DEF */ *window, MFDB *src, _WORD x, _WORD y, _WORD w, _WORD h, _WORD dx, _WORD dy, _WORD zoom)
{
	COLORREF oldColor;
	COLORREF oldBk;
	HDC hdcMem;
	POINT ptSize, ptOrg;
	HDC screen;
	
	if ((x + w) > src->ic.bmWidth)
		w = src->ic.bmWidth - x;
	if (w <= 0)
		return;
	if ((y + h) > src->ic.bmHeight)
		h = src->ic.bmHeight - y;
	if (h <= 0)
		return;
	screen = W_GetDC(window);
	if (screen == NO_DC)
		return;
	oldColor = SetTextColor(screen, W_PAL_BLACK);
	oldBk = SetBkColor(screen, W_PAL_WHITE);

	hdcMem = CreateCompatibleDC(screen);
	SelectObject(hdcMem, src->bmp);
	
	ptSize.x = w * zoom;
	ptSize.y = h * zoom;
	DPtoLP(screen, &ptSize, 1);
	ptOrg.x = x;
	ptOrg.y = y;
	DPtoLP(screen, &ptOrg, 1);
	
	StretchBlt(screen, dx, dy, ptSize.x, ptSize.y, hdcMem, ptOrg.x, ptOrg.y, w, h, SRCCOPY);
	DeleteDC(hdcMem);
	SetTextColor(screen, oldColor);
	SetBkColor(screen, oldBk);
}

/*** ---------------------------------------------------------------------- ***/

OS_GLOBAL HBITMAP get_mfdb_bitmap(_VOID /* MFDB */ *mfdb)
{
	return ((MFDB *)mfdb)->bmp;
}


/* Colour board artwork is sampled at display resolution, not ICON_SCALE. */
GLOBAL _VOID W_Draw_Tile(_VOID *window, MFDB *tile, _WORD dx, _WORD dy, _WORD w, _WORD h)
{
    HDC screen = W_GetDC(window);
    HDC source;
    HGDIOBJ previous;
    int saved;
    RECT bounds;
    bounds.left = dx; bounds.top = dy; bounds.right = dx + w; bounds.bottom = dy + h;
    if (screen == NO_DC || tile == NULL || w <= 0 || h <= 0) return;
    if (!RectVisible(screen, &bounds)) return;
    source = CreateCompatibleDC(screen);
    if (source == NULL) return;
    previous = SelectObject(source, tile->bmp);
    saved = SaveDC(screen);
    SetStretchBltMode(screen, HALFTONE);
    SetBrushOrgEx(screen, 0, 0, NULL);
    StretchBlt(screen, dx, dy, w, h, source, 0, 0,
               tile->ic.bmWidth, tile->ic.bmHeight, SRCCOPY);
    RestoreDC(screen, saved);
    SelectObject(source, previous);
    DeleteDC(source);
}

/* Reconstruct pixels with integer quarter turns. GDI parallelogram blits
 * can sample different edge pixels when rotated; explicit copies are exact. */
GLOBAL MFDB *assemble_mfdb(MFDB *atlas, _WORD w, _WORD h, _WORD block,
                          _WORD columns, CONST _UWORD *codes)
{
    MFDB *result;
    HDC screen;
    BITMAPINFO info = {0};
    DWORD *dest;
    int x, y, dx, dy, px, py, sx, sy, patch, turn;
    if (atlas == NULL || codes == NULL || block <= 0 || columns <= 0 ||
        w <= 0 || h <= 0 || w % block || h % block) return NULL;
    screen = GetDC(HWND_DESKTOP);
    if (screen == NULL) return NULL;
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = atlas->ic.bmWidth;
    info.bmiHeader.biHeight = -atlas->ic.bmHeight;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    if (atlas->atlas_pixels == NULL)
    {
        atlas->atlas_pixels = MALLOC(atlas->ic.bmWidth * atlas->ic.bmHeight * 4, "board atlas pixels");
        if (atlas->atlas_pixels == NULL) { ReleaseDC(HWND_DESKTOP, screen); return NULL; }
        if (!GetDIBits(screen, atlas->bmp, 0, atlas->ic.bmHeight, atlas->atlas_pixels, &info, DIB_RGB_COLORS))
        {
            FREE(atlas->atlas_pixels, atlas->ic.bmWidth * atlas->ic.bmHeight * 4);
            atlas->atlas_pixels = NULL; ReleaseDC(HWND_DESKTOP, screen); return NULL;
        }
    }
    result = get_mfdb(w, h, 32, NULL);
    if (result == NULL) { ReleaseDC(HWND_DESKTOP, screen); return NULL; }
    dest = result->ic.bmBits;
    for (y = 0; y < h; y += block)
    {
        for (x = 0; x < w; x += block)
        {
            patch = *codes >> 2; turn = *codes++ & 3;
            sx = (patch % columns) * block; sy = (patch / columns) * block;
            if (sx + block > atlas->ic.bmWidth || sy + block > atlas->ic.bmHeight)
            { free_mfdb(result); ReleaseDC(HWND_DESKTOP, screen); return NULL; }
            for (dy = 0; dy < block; dy++) for (dx = 0; dx < block; dx++)
            {
                switch (turn)
                {
                case 0: px = dx; py = dy; break;
                case 1: px = dy; py = block - 1 - dx; break;
                case 2: px = block - 1 - dx; py = block - 1 - dy; break;
                default: px = block - 1 - dy; py = dx; break;
                }
                dest[(y + dy) * w + x + dx] = atlas->atlas_pixels[(sy + py) * atlas->ic.bmWidth + sx + px];
            }
        }
    }
    info.bmiHeader.biWidth = w; info.bmiHeader.biHeight = -h;
    if (!SetDIBits(screen, result->bmp, 0, h, dest, &info, DIB_RGB_COLORS))
    { free_mfdb(result); result = NULL; }
    ReleaseDC(HWND_DESKTOP, screen);
    return result;
}
