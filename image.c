#include <image.h>
#include <ro_mem.h>

typedef struct img_header
{
	_WORD version;
	_WORD headlen;
	_WORD planes;
	_WORD pat_run;
	_WORD pix_width;
	_WORD pix_height;
	_WORD sl_width;
	_WORD sl_height;
} IMG_HEADER;

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

LOCAL _UBYTE *zoom_line(_UBYTE *ptr, _WORD w, _WORD zoom, _UBYTE *Buffer)
{
	_WORD i, j, k;
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

LOCAL _BOOL bit_string(FILE *fd, _UBYTE *ptr, _WORD w)
{
	_UBYTE header[2] = { 0x80, 0x00 };
	
	while (w > 255)
	{
		header[1] = 255;
		if (fwrite(header, sizeof(header), 1, fd) != 1)
		{
			return FALSE;
		}
#if ATARI
		if (fwrite(ptr, sizeof(_UBYTE), 255, fd) != 255)
		{
			return FALSE;
		}
#else
		{
			_WORD i;
			
			for (i = 0; i < 255; i++)
				if (fputc(~ptr[i], fd) == EOF)
					return FALSE;
		}
#endif
		w -= 255;
		ptr += 255;
	}
	
	header[1] = (_UBYTE)w;
	if (fwrite(header, sizeof(header), 1, fd) != 1)
	{
		return FALSE;
	}
#if ATARI
	if (fwrite(ptr, sizeof(_UBYTE), w, fd) != w)
	{
		return FALSE;
	}
#else
	{
		_WORD i;
		
		for (i = 0; i < w; i++)
			if (fputc(~ptr[i], fd) == EOF)
				return FALSE;
	}
#endif
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL solid_run(FILE *fd, _UBYTE *ptr, _WORD w)
{
	_WORD i;
	_UBYTE sr;
	
	while (w)
	{
		i = 0;
		sr = *ptr;
		if (sr == 0x00 || sr == 0xFF)
		{
			while (i < w && i < 0x7F && sr == ptr[i])
			{
				i++;
			}

			sr &= 0x80;
#if !ATARI
			sr ^= 0x80;
#endif
			sr |= (_UBYTE)i;
			if (fputc(sr, fd) == EOF)
			{
				return FALSE;
			}
		} else
		{
			while (i < w && ptr[i] != 0x00 && ptr[i] != 0xFF)
			{
				i++;
			}
			if (!bit_string(fd, ptr, i))
			{
				return FALSE;
			}
		}
		w -= i;
		ptr += i;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL pattern_run(FILE *fd, _UBYTE *ptr, _WORD w)
{
	_WORD i, r;
	_UBYTE pr[4] = { 0x00, 0x00, 0x00, 0x00 };

	while (w)
	{	
		i = 0;
		r = 0;

		pr[2] = ptr[0];
		pr[3] = ptr[1];
		while (i+1 < w && r < 0xFF &&
			pr[2] == ptr[i] && pr[3] == ptr[i+1])
		{
			r++;
			i += 2;
		}
		
		if (r > 1)
		{
			if (!(r == 0xFF && i == w) &&
				((pr[2] == 0x00 && pr[3] == 0x00) ||
				 (pr[2] == 0xFF && pr[3] == 0xFF)))
			{
				while (i < w && ptr[0] == ptr[i])
				{
					i++;
				}
				if (!solid_run(fd, ptr, i))
				{
					return FALSE;
				}
			} else
			{
				pr[1] = (_UBYTE)r;
#if !ATARI
				pr[2] = ~pr[2];
				pr[3] = ~pr[3];
#endif
				if (fwrite(pr, sizeof(pr), 1, fd) != 1)
				{
					return FALSE;
				}
			}
		} else
		{
			i = 1;
			while (i+5 < w &&
				   !(ptr[i] == ptr [i+2] && ptr[i+1] == ptr[i+3] &&
				     ptr[i] == ptr [i+4] && ptr[i+3] == ptr[i+5]))
			{
				i++;
			}
			if (w - i <= 4)
			{
				i = w;
			}
			if (!solid_run(fd, ptr, i))
			{
				return FALSE;
			}
		}
		ptr += i;
		w -= i;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL vertical_replication_count(FILE *fd, _UBYTE *ptr, _WORD w, _WORD h, _WORD zoom, _BOOL compress)
{
	_WORD c;
	_UBYTE vrc[4] = { 0x00, 0x00, 0xFF, 0x00 };
	_UBYTE Buffer[4096]; 
	_UBYTE *buf;
	
	if (compress)
	{
		while (h)
		{
			h--;
			c = zoom; 
			while (h > 0 && c+zoom <= 0xFF && 
				  MemCmp(ptr, ptr+w, (size_t)w) == 0)
			{
				h--;
				c += zoom;
				ptr += w;
			}
			
			if (c > 1)
			{
				vrc[3] = c;
				if (fwrite(vrc, sizeof(vrc), 1, fd) != 1L)
				{
					return FALSE;
				}
			}
	
			if (!pattern_run(fd, zoom_line(ptr, w, zoom, Buffer), w * zoom))
			{
				return FALSE;
			}
			ptr += w;
		}
	} else
	{
		_WORD i;
		
		while (h--)
		{
			buf = zoom_line(ptr, w, zoom, Buffer);
			for (i = zoom; i > 0; i--)
			{	
				if (!bit_string(fd, buf, w * zoom))
				{
					return FALSE;
				}
			}
			ptr += w;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL mfdb_to_img(FILE *fd, MFDB *mfdb, _BOOL compress, _WORD zoom)
{
	_WORD w, h;
	_UBYTE *ptr;
	_VOID *data;
	
	get_mfdb_info(mfdb, &w, &h, &data);
	w = ((w + 15) / 16) * 16;
	
	if (zoom < 1 || zoom > 255 ||
		(_LONG)w * (_LONG)zoom > 0x7FFFL ||
		(_LONG)h * (_LONG)zoom > 0x7FFFL
	   )
	{
		return FALSE;
	}

#define put_byte(x) \
	if (fputc(x, fd) == EOF) \
		return FALSE
/* output _WORD in big endian order */
#define put_word(w) \
	put_byte((w) >> 8); \
	put_byte((w) & 0xff)

	put_word(1); /* version */
	put_word(8); /* headlen */
	put_word(1); /* planes */
	put_word(2); /* pat_run */
	put_word(1); /* pix_width */
	put_word(1); /* pix_height */
	put_word(w * zoom); /* sl_width */
	put_word(h * zoom); /* sl_height */
	
	ptr = data;
	w = w / 8;
	
	if (!vertical_replication_count(fd, ptr, w, h, zoom, compress))
	{
		return FALSE;
	}
	return TRUE;
}
