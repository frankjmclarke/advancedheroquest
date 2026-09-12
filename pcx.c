#include <pcx.h>

typedef struct pcx_header
{
	_UBYTE manu, hard, encod, bitpx;
	_UWORD x1, y1, x2, y2;
	_UWORD hres, vres;
	_UBYTE palette[48];
	_UBYTE vmode, nplanes;
	_UWORD bytesperline;
} PCX_HEADER;


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

LOCAL _BOOL put_byte(FILE *fd, _UBYTE c, _UWORD w)
{
	_UWORD i;
	
	if (w)
	{
#if ATARI
		c = ~c;
#endif
		if (w > 1 || (c & 0xC0) == 0xC0)
		{
			while (w)
			{
				i = (w >= 0x3F) ? 0x3F : w;
				w -= i;
				
				if ( (fputc((_UWORD)(0xC0|i), fd) == EOF) ||
					 (fputc((_UWORD)c, fd) == EOF) )
				{
					return FALSE;
				}
			}
		} else
		{
			if (fputc((_UWORD)c, fd) == EOF)
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL put_line(FILE *fd, _UBYTE *ptr, _UWORD w, _BOOL compress)
{
	_UWORD i;
	_UBYTE c;

	if (compress)
	{
		c = *ptr;
		i = 0;
		while (w--)
		{
			if (c == *ptr)
			{
				i++;
				ptr++;
			} else
			{
				if (!put_byte(fd, c, i))
				{
					return FALSE;
				}
				c = *ptr++;
				i = 1;
			}
		}
		if (!put_byte(fd, c, i))
		{
			return FALSE;
		}
	} else
	{
		while (w--)
		{
			if (!put_byte(fd, *ptr++, 1))
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL mfdb_to_pcx(FILE *fd, MFDB *mfdb, _BOOL compress, _WORD zoom)
{
	_WORD w, h, i;
	_UBYTE *ptr, *buf;
	_VOID *data;
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
	
#define put_byte(x) \
	if (fputc(x, fd) == EOF) \
		return FALSE
/* output _WORD in little endian order */
#define put_word(w) \
	put_byte((w) & 0xff); \
	put_byte((w) >> 8)

	put_byte(10); /* manu */
	put_byte(3); /* hard */
	put_byte(1); /* encod */
	put_byte(1); /* bitpx */
	put_word(0); /* x1 */
	put_word(0); /* y1 */
	put_word(w * zoom - 1); /* x2 */
	put_word(h * zoom - 1); /* y2 */
	put_word(w * zoom); /* hres */
	put_word(h * zoom); /* vres */
	for (i = 0; i < 48; i++)
	{
		put_byte(0); /* palette */
	}
	put_byte(1); /* vmode */
	put_byte(1); /* nplanes */
	put_word((w * zoom) / 8); /* bytesperline */
	for (i = 68; i < 128; i++)
	{
		put_byte(0);
	}
	
	ptr = data;
	w = w / 8;
	
	while (h--)
	{
		buf = zoom_line(ptr, w, zoom, Buffer);
		for (i = zoom; i > 0; i--)
		{	
			if (!put_line(fd, buf, w * zoom, compress))
			{
				return FALSE;
			}
		}
		ptr += w;
	}
	return TRUE;
}
