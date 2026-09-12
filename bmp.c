#include <bmp.h>

typedef struct {
	/* file header */
	_UBYTE magic[2];		/* BM */
	_UBYTE filesize[4];
	_UBYTE reserved[4];
	_UBYTE offbits[4];		/* offset to data */

	/* info header */
	union {
		struct {
			_UBYTE hsize[4];		/* size of info header (40) */
			_UBYTE width[4];		/* width in pixels */
			_UBYTE height[4];		/* height in pixels */
			_UBYTE planes[2];		/* always 1 */
			_UBYTE bitcount[2];     /* bits per pixel: 1, 4, 8 or 24 */
			_UBYTE compression[4];	/* compression method */
#define BMP_RGB 0
#define BMP_RLE8 1
#define BMP_RLE4 2
#define BMP_BITFIELDS 24
			_UBYTE sizeImage[4];	/* size of data */
			_UBYTE xPelsPerMeter[4];
			_UBYTE yPelsPerMeter[4];
			_UBYTE clrUsed[4];		/* # of colors used */
			_UBYTE clrImportant[4]; /* # of important colors */
		} bitmapinfoheader;
		struct {
			_UBYTE hsize[4];		/* size of info header (12) */
			_UBYTE width[2];		/* width in pixels */
			_UBYTE height[2];		/* height in pixels */
			_UBYTE planes[2];		/* always 1 */
			_UBYTE bitcount[2];     /* bits per pixel: 1, 4, 8 or 24 */
		} bitmapcoreheader;
	} bmp_info_header;
} BMP_HEADER;

#define put_byte(x) \
	if (fputc(x, fd) == EOF) \
		return FALSE
#define put_word(w) \
	put_byte((_UBYTE)((w)	   )); \
	put_byte((_UBYTE)((w) >>  8))
#define put_long(l) \
	put_byte((_UBYTE)((l)	   )); \
	put_byte((_UBYTE)((l) >>  8)); \
	put_byte((_UBYTE)((l) >> 16)); \
	put_byte((_UBYTE)((l) >> 24))

#define tobyte(pixels) (((pixels) + 7) >> 3)
#define toword(pixels) (((pixels) + 15) >> 4)


/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

LOCAL _UBYTE *zoom_line(_UBYTE *ptr, _UWORD w, _WORD zoom, _UBYTE *Buffer)
{
	_UWORD i, j, k;
	_UBYTE *buf;
	LOCAL CONST _UBYTE bit1[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
	LOCAL CONST _UBYTE bit0[] = { 0x7F, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE };
	
	if (zoom > 1)
	{
		buf = Buffer;
		i = 0;
		while (w--)
		{
			if (*ptr == 0x00 || *ptr == 0xFF)
			{
				for (k = zoom; k > 0; k--)
				{
					*buf++ = *ptr;
				}
			} else
			{
				for (j = 0; j < 8; j++)
				{	
					if (*ptr & bit1[j])
					{
						for (k = zoom; k > 0; k--)
						{
							*buf |= bit1[i++];
							if (i > 7)
							{
								buf++;
								i = 0;
							}
						}
					} else
					{
						for (k = zoom; k > 0; k--)
						{
							*buf &= bit0[i++];
							if (i > 7)
							{
								buf++;
								i = 0;
							}
						}
					}
				}
			}
			ptr++;
		}
		return Buffer;
	}
	return ptr;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL mfdb_to_bmp(FILE *fd, MFDB *mfdb, _BOOL compress, _WORD zoom)
{
	_WORD w, h, i, j, k, wb, planes;
	_UBYTE *src, *buf;
	_VOID *data;
	_ULONG len, headlen;
	_WORD ncolors;
	_ULONG cmapsize;
	_ULONG datasize;
	_ULONG picsize;
	struct _rgb {
		_UBYTE r;
		_UBYTE g;
		_UBYTE b;
	};
	LOCAL struct _rgb win2_palette[2] = {
		{	0,	 0,   0 },
		{ 255, 255, 255 }
	};
	_UBYTE Buffer[8192];
	
	get_mfdb_info(mfdb, &w, &h, &data);
	w = ((w + 15) / 16) * 16;
	
	if (zoom < 1 || zoom > 255 ||
		(_LONG)w * (_LONG)zoom > 0xFFFFL ||
		(_LONG)h * (_LONG)zoom > 0xFFFFL
	   )
	{
		return FALSE;
	}
	
	planes = 1; /* TODO: color formats */
	ncolors = 1 << planes;
	UNUSED(compress); /* monochrom bitmaps always uncompressed */

	cmapsize = 4 * ncolors;

	put_byte('B');
	put_byte('M');

	headlen = 14 + 40 + cmapsize;
	wb = tobyte(w);
	picsize = (_ULONG)((wb + 1) & ~1) * h * planes;
	wb = (tobyte(w * zoom) + 3) & ~3;
	datasize = (_ULONG)wb * h * zoom * planes;
	len = headlen + datasize;
	put_long(len);
	put_long(0l);
	put_long(headlen);

	put_long(40l);
	put_long((_LONG)(w) * zoom);
	put_long((_LONG)(h) * zoom);
	put_word(1);					/* planes */
	put_word(planes);				/* bits per pixel */
	put_long((_LONG)(BMP_RGB));		/* compression */
	put_long((_LONG)(datasize));	/* sizeImage */
	put_long(0l);					/* pix_width */
	put_long(0l);					/* pix_height */
	put_long((_LONG)(ncolors));
	put_long(0l);

	switch (planes)
	{
	case 1:
		for (i = 0; i < ncolors; i++)
		{
			put_byte(win2_palette[i].b);
			put_byte(win2_palette[i].g);
			put_byte(win2_palette[i].r);
			put_byte(0);
		}
		break;
	}
	
	src = (_UBYTE *)data + picsize;
	switch (planes)
	{
	case 1:
		{
			_WORD l;

			j = tobyte(w * zoom);
			k = toword(w) << 1;
			while (h--)
			{
				src -= k;
				buf = zoom_line(src, w, zoom, Buffer);
				for (i = 0; i < zoom; i++)
				{
					for (l = 0; l < j; l++)
#ifdef ATARI
						put_byte(~buf[l]);
#else
						put_byte(buf[l]);
#endif
					switch (j & 3)
					{
					case 1:
						put_byte('\0');
						put_byte('\0');
						put_byte('\0');
						break;
					case 2:
						put_byte('\0');
						put_byte('\0');
						break;
					case 3:
						put_byte('\0');
						break;
					}
				}
			}
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}
