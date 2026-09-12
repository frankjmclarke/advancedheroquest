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
};

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

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
	oldColor = SetTextColor(dest->hDC, W_PAL_BLACK);
	oldBk = SetBkColor(dest->hDC, W_PAL_WHITE);
	
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
		{
			if (ptr->is_resource)
				FreeResource((HGLOBAL) ptr->bmp);
			else
				DeleteObject(ptr->bmp);
		}
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
				mfdb->bmp = CreateBitmapIndirect(&mfdb->ic);
			/* else
				mfdb->bmp = CreateDiBitmap() TODO */
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

