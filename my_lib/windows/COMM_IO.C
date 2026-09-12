/*****************************************************************************
 *	WINDOWS/COMM_IO.C
 *****************************************************************************/

#include <portab.h>

#include <file_io.h>
#include <comm_io.h>
#include <_comm_io.h>
#include <_file_io.h>
#if GUI_VERSION
#include <commdlg.h>
#include <w_draw.h>
#include <w_graf.h>
#include <w_print.h>
#endif
#include <debug.h>
#include <event.h>

#define PRINTER_ESCAPE 0


#define TASTEN_ABBRUCH (GetKeyboardStates() == (K_CTRL|K_ALT|K_LSHIFT))

#define COM_BLOCK_SIZE  512
#define PR_BLOCK_SIZE  128

typedef struct _comm_handle {
	union {
#if GUI_VERSION
#  ifdef __WIN32__
		HANDLE fhandle;
#  else
		_WORD chandle;
#  endif
		WIND_OS *os;
#endif
		FILENO fd;
		_WORD port;
	} h;
	_WORD type;
#define CH_NONE    0
#define CH_FHANDLE 1
#define CH_CHANDLE 2
#define CH_OS      3
#define CH_FD      4
#define CH_PORT    5
} COMM_HANDLE;

#define RS_DELAY 1

#define DEV_COM1 1
#define DEV_COM2 2
#define DEV_COM3 3
#define DEV_COM4 4
#define DEV_COM5 5
#define DEV_COM6 6
#define DEV_COM7 7
#define DEV_COM8 8
#define DEV_LPT1 20
#define DEV_LPT2 21
#define DEV_LPT3 22
#define DEV_CON  40
#define DEV_MIDI 41

#define DEFAULT_DEVICE "COM2"
#define DEFAULT_BAUD 19200l

#define PA_FOSSIL  0
#define PA_BIOS    1
#define PA_LPT     2
#define PA_WINDOWS 3
#define PA_MIDI    4

LOCAL struct {
	CONST _UBYTE *printable_name;
	CONST _UBYTE *dev_name;
	_WORD dev;
} devices[MAX_DEVICES];
LOCAL _WORD num_devices;


#ifndef __WIN32__
extern int X00(union REGS *, union REGS *);
extern int X00X(union REGS *, union REGS *, struct SREGS *);
#endif

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL printer_retry(_VOID)
{
#if GUI_VERSION
	return Form_Alert(1, MsgString(MSG_PRINTER_RETRY)) == 1;
#else
	return FALSE;
#endif
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL PRN_HANDLE Printer_Open(_WORD nr)
{
	COMM_HANDLE *handle;
	_UBYTE name[PATH_MAX];

	handle = NEW(COMM_HANDLE, "Printer_Open");
	if (handle == NULL)
		return -1;
#if GUI_VERSION
	if (nr == 0)
	{
		WIND_OS *os;
#if PRINTER_ESCAPE
		GRECT gr;
		
		xywh2rect(0, 0, 0, 0, &gr);
		os = W_G_Open_Printer(&gr, "Druckausgabe");
		if (os == NULL)
		{
			OFREE(handle);
			return -1;
		}
		handle->type = CH_OS;
		handle->h.os = os;
		return (PRN_HANDLE) handle;
#else
		os = W_G_Open_Printer((GRECT *)name, "SelectPrinter");
		if (os == NULL)
		{
			OFREE(handle);
			return -1;
		}
		W_G_Close_Printer(os);
#endif
	} else
#else
	if (nr == 0)
	{
		strcpy(name, "PRN");
	} else
#endif
	{
		sprintf(name, "LPT%d:", nr);
	}
	
	do
	{
#if GUI_VERSION
#ifdef __WIN32__
		handle->h.fhandle = CreateFile(name, GENERIC_WRITE,
			0,	/* exclusive access */
			NULL,		/* no security attrs */
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		handle->type = CH_FHANDLE;
		if (handle->h.fhandle >= 0)
			return (PRN_HANDLE) handle;
#else
		handle->h.chandle = OpenComm((LPSTR) name, 0, PR_BLOCK_SIZE);
		handle->type = CH_CHANDLE;
		if (handle->h.chandle >= 0)
			return (PRN_HANDLE) handle;
#endif
#else /* !GUI_VERSION */
		handle->h.fd = F_File_Open(name, FO_WRONLY);
		handle->type = CH_FHANDLE;
		if (FILENO_OK(handle->h.fd))
			return (PRN_HANDLE) handle;
#endif
	} while (printer_retry());
	OFREE(handle);
	return -1;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL F_ERROR Printer_Close(PRN_HANDLE handle)
{
	COMM_HANDLE *hnd = (COMM_HANDLE *) handle;
	F_ERROR result;
	
#if GUI_VERSION
	if (hnd->type == CH_OS)
	{
		W_G_Close_Printer(hnd->h.os);
		result = GERR_OK;
	} else
	{
#ifdef __WIN32__
		ASSERT(hnd->type == CH_FHANDLE);
		FlushFileBuffers(hnd->h.fhandle);
		result = CloseHandle(hnd) ? GERR_OK : GERR_BADF;
#else
		ASSERT(hnd->type == CH_CHANDLE);
		result = CloseComm(hnd->h.chandle) == 0 ? GERR_OK : GERR_BADF;
#endif
	}
#else /* !GUI_VERSION */
	ASSERT(hnd->type == CH_FD);
	result = F_File_Close(hnd->h.fd);
#endif /* GUI_VERSION */
	OFREE(hnd);
	return result;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _LONG Printer_Write(PRN_HANDLE hnd, CONST _UBYTE *buf, _LONG size)
{
	COMM_HANDLE *handle = (COMM_HANDLE *) hnd;
#if GUI_VERSION
	_LONG toDo = size;

	if (handle->type == CH_OS)
	{
		_UBYTE *pass;
		
		pass = MALLOC(size + sizeof(_WORD), "Printer_Write");
		if (pass == NULL)
			return 0;
		*(_WORD *)pass = (_WORD)size;
		MemCpy(pass + sizeof(_WORD), buf, size);
		if (Escape(handle->h.os->hDC, PASSTHROUGH, 0, pass, NULL) > 0)
			toDo = 0;
		FREE(pass, size + sizeof(_WORD));
	} else
	{
		DWORD retWrite;
		_LONG outSize = PR_BLOCK_SIZE;
		CONST _UBYTE _HUGE *out = buf;

		while (toDo > 0)
		{
			if (toDo < PR_BLOCK_SIZE)
				outSize = toDo;
	
#ifdef __WIN32__
			ASSERT(handle->type == CH_FHANDLE);
			retWrite = 0;
			if (WriteFile(handle->h.fhandle, out, outSize, &retWrite, NULL) == FALSE)
			{
			}
#else
			ASSERT(handle->type == CH_CHANDLE);
			retWrite = WriteComm(handle->h.chandle, out, (int) outSize);
			if ((_LONG)retWrite < 0)
				retWrite = -retWrite;
#endif
			toDo -= retWrite;
			out += retWrite;
			if (retWrite != outSize)
			{
#ifndef __WIN32__
				_LONG err;
	
				err = GetCommError(handle->h.chandle, NULL);
				if (err != 0)
					if (!(err & CE_TXFULL))
#endif
					{
						if (!printer_retry())
						{
							return size - toDo;
						}
					}
			}
	
			if (toDo > 0)
			{
				/* Rechenzeit fuer Server freigeben */
				if (retWrite == 0)	/* nicht 1 Byte geschrieben */
					EventTimer(500);
				else
					EventTimer(1);
			}
		}
	}	
	return size - toDo;
#else
	ASSERT(handle->type == CH_FD);
	F_File_Write(handle->h.fd, buf, size, &size);
	return size;
#endif
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID get_devices(_VOID)
{
	if (num_devices > 0)
		return;
	num_devices = 0;
	
	devices[0].dev = DEV_LPT1;
	devices[0].printable_name = "Parallel";
	devices[0].dev_name = "lpt1";
	num_devices++;
	
	devices[1].dev = DEV_COM1;
	devices[1].printable_name = "COM1";
	devices[1].dev_name = "COM1";
	num_devices++;
	
	devices[2].dev = DEV_COM2;
	devices[2].printable_name = "COM2";
	devices[2].dev_name = "COM2";
	num_devices++;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _WORD comm_devices(_WORD *dev)
{
	_WORD i;
	
	get_devices();
	for (i = 0; i < num_devices; i++)
		*dev++ = devices[i].dev;
	return num_devices;
}

/*** ---------------------------------------------------------------------- ***/

#ifndef __WIN32__
LOCAL _UWORD comm_port(_WORD dev)
{
	switch (dev)
	{
		case DEV_COM1: return *((unsigned short FAR *)(0x00400000l)); /* 0x03f8 */
		case DEV_COM2: return *((unsigned short FAR *)(0x00400002l)); /* 0x02f8 */
		case DEV_COM3: return *((unsigned short FAR *)(0x00400004l)); /* 0x03e8 0x3220 */
		case DEV_COM4: return *((unsigned short FAR *)(0x00400006l)); /* 0x02e8 0x3228 */
		case DEV_COM5: return 0x4220;
		case DEV_COM6: return 0x4228;
		case DEV_COM7: return 0x5220;
		case DEV_COM8: return 0x5228;
	}
	return 0;
}
#endif

/*** ---------------------------------------------------------------------- ***/

#ifndef __WIN32__
LOCAL _UWORD baud_val(_LONG baud)
{
	if (baud ==    110l) return 0x00;
	if (baud ==    150l) return 0x01;
	if (baud ==    300l) return 0x02;
	if (baud ==    600l) return 0x03;
	if (baud ==   1200l) return 0x04;
	if (baud ==   2400l) return 0x05;
	if (baud ==   4800l) return 0x06;
	if (baud ==   9600l) return 0x07;
	if (baud ==  19200l) return 0x08;
	if (baud ==  28800l) return 0x80;
	if (baud ==  38400l) return 0x81;
	if (baud ==  57600l) return 0x82;
	if (baud ==  76800l) return 0x83;
	if (baud == 115200l) return 0x84;
	return baud_val(DEFAULT_BAUD);
}

LOCAL _UWORD baud_time(_LONG *baud)
{
	if (*baud ==     50l) return 0x900;
	if (*baud ==     75l) return 0x600;
	if (*baud ==    110l) return 0x417;
	if (*baud ==    134l) return 0x359;
	if (*baud ==    150l) return 0x300;
	if (*baud ==    200l) return 0x240;
	if (*baud ==    300l) return 0x180;
	if (*baud ==    600l) return 0x0c0;
	if (*baud ==   1200l) return 0x060;
	if (*baud ==   1800l) return 0x040;
	if (*baud ==   2000l) return 0x03a;
	if (*baud ==   2400l) return 0x030;
	if (*baud ==   3600l) return 0x020;
	if (*baud ==   4800l) return 0x018;
	if (*baud ==   7200l) return 0x010;
	if (*baud ==   9600l) return 0x00c;
	if (*baud ==  19200l) return 0x006;
	if (*baud ==  28800l) return 0x004;
	if (*baud ==  38400l) return 0x003;
	if (*baud ==  57600l) return 0x002;
	if (*baud == 115200l) return 0x001;
	*baud = DEFAULT_BAUD;
	return baud_time(baud);
}

#if GUI_VERSION
LOCAL UINT dcb_baud(_LONG baud)
{
	if (baud == 110) return CBR_110;
	if (baud == 300) return CBR_300;
	if (baud == 600) return CBR_600;
	if (baud == 1200) return CBR_1200;
	if (baud == 2400) return CBR_2400;
	if (baud == 4800) return CBR_4800;
	if (baud == 9600) return CBR_9600;
	if (baud == 14400) return CBR_14400;
	if (baud == 19200) return CBR_19200;
	if (baud == 38400l) return CBR_38400;
	if (baud == 56000l) return CBR_56000;
	if (baud == 57600l) return CBR_56000;
	if (baud == 128000l) return CBR_128000;
	if (baud == 115200l) return CBR_128000;
	if (baud == 256000l) return CBR_256000;
	if (baud == 230400l) return CBR_256000;
	return (UINT)baud;
}
#endif
#endif

/*** ---------------------------------------------------------------------- ***/

GLOBAL CONST _UBYTE *comm_device_name(_WORD dev)
{
	_WORD i;
	
	get_devices();
	if (dev == -1)
		dev = comm_default();
	for (i = 0; i < num_devices; i++)
		if (devices[i].dev == dev)
			return devices[i].printable_name;
	return NULL;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL CONST _UBYTE *comm_dev_name(_WORD dev)
{
	_WORD i;
	
	get_devices();
	for (i = 0; i < num_devices; i++)
		if (devices[i].dev == dev)
			return devices[i].dev_name;
	return NULL;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _WORD try_comm(_WORD dev)
{
	_WORD i;
	
	for (i = 0; i < num_devices; i++)
		if (devices[i].dev == dev)
			break;
	if (i >= num_devices)
		return -1;
	
	return dev;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _WORD comm_speeds(_WORD dev, _LONG *speeds)
{
	_WORD num = 0;
	_WORD handle;
	
	get_devices();
	switch (dev)
	{
	case DEV_LPT1:
	case DEV_LPT2:
	case DEV_LPT3:
	case DEV_CON:
		*speeds = 0, num++;
		break;
	case DEV_MIDI:
		*speeds = 31250l, num++;
		break;
	default:
		handle = try_comm(dev);
		if (handle >= 0)
		{
			*speeds++ = 50l, num++;
			*speeds++ = 75l, num++;
			*speeds++ = 110, num++;
			*speeds++ = 134, num++;
			*speeds++ = 150, num++;
			*speeds++ = 200l, num++;
			*speeds++ = 300l, num++;
			*speeds++ = 600l, num++;
			*speeds++ = 1200l, num++;
			*speeds++ = 1800l, num++;
			*speeds++ = 2000l, num++;
			*speeds++ = 2400l, num++;
			*speeds++ = 3600l, num++;
			*speeds++ = 4800l, num++;
			*speeds++ = 7200l, num++;
			*speeds++ = 9600l, num++;
			*speeds++ = 19200l, num++;
			*speeds++ = 28800l, num++;
			*speeds++ = 38400l, num++;
			*speeds++ = 57600l, num++;
			*speeds = 115200l, num++;
		}
		break;
	}
	return num;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _WORD comm_default(_VOID)
{
	_WORD i;

	get_devices();
	for (i = 0; i < num_devices; i++)
	{
		if (strCaseCmp(devices[i].printable_name, DEFAULT_DEVICE) == 0)
			return devices[i].dev;
	}
	return DEV_COM1;
}

/*** ---------------------------------------------------------------------- ***/

#ifndef __WIN32__
LOCAL _LONG comm_available_bios(COM_PARA *pa)
{
	union REGS rg;
	
	rg.h.ah = 0x03;
	rg.w.dx = ((COMM_HANDLE *)pa->handle)->h.port;
	int86(0x14, &rg, &rg);
	if (rg.h.ah & 0x01)
		return 1;
	return 0;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _UBYTE comm_readc_bios(COM_PARA *pa)
{
	union REGS rg;
	
	rg.h.ah = 0x02;
	rg.w.dx = ((COMM_HANDLE *)pa->handle)->h.port;
	int86(0x14, &rg, &rg);
	return rg.h.al;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _LONG comm_read_bios(COM_PARA *pa, _UBYTE *buf, _LONG size)
{
	_LONG count;
	_LONG timeout;
	
	count = 0;
	timeout = pa->timeout / RS_DELAY;
	while (size && timeout >= 0)
	{
		if ((*pa->comm_available)(pa) > 0)
		{
			*buf++ = (*pa->comm_readc)(pa);
			count++;
			size--;
		} else
		{
			EventTimer(RS_DELAY);
			timeout--;
		}
	}
	return count;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _LONG comm_write_bios(COM_PARA *pa, CONST _UBYTE *buf, _LONG size)
{
	_LONG count;
	
	count = 0;
	while (size)
	{
		(*pa->comm_writec)(pa, *buf);
		buf++;
		size--;
		count++;
	}
	return count;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _LONG comm_writec_bios(COM_PARA *pa, _UBYTE c)
{
	union REGS rg;
	
	rg.h.ah = 0x01;
	rg.h.al = c;
	rg.w.dx = ((COMM_HANDLE *)pa->handle)->h.port;
	int86(0x14, &rg, &rg);
	return 1;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL comm_flush_bios(COM_PARA *para, _WORD which)
{
	if (which & COMM_FLUSH_READ)
	{
		while (comm_available(para))
			comm_readc(para);
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/
/*** ---------------------------------------------------------------------- ***/

LOCAL _LONG comm_available_lpt(COM_PARA *pa)
{
	UNUSED(pa);
	return 0;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _UBYTE comm_readc_lpt(COM_PARA *pa)
{
	UNUSED(pa);
	return 0;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _LONG comm_read_lpt(COM_PARA *pa, _UBYTE *buf, _LONG size)
{
	_LONG count;
	_LONG timeout;
	
	count = 0;
	timeout = pa->timeout / RS_DELAY;
	while (size && timeout >= 0)
	{
		if ((*pa->comm_available)(pa) > 0)
		{
			*buf++ = (*pa->comm_readc)(pa);
			count++;
			size--;
		} else
		{
			EventTimer(RS_DELAY);
			timeout--;
		}
	}
	return count;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _LONG comm_write_lpt(COM_PARA *pa, CONST _UBYTE *buf, _LONG size)
{
	_LONG count;
	
	count = 0;
	while (size)
	{
		(*pa->comm_writec)(pa, *buf);
		buf++;
		size--;
		count++;
	}
	return count;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _LONG comm_writec_lpt(COM_PARA *pa, _UBYTE c)
{
	union REGS rg;
	
	rg.h.ah = 0x00;
	rg.h.al = c;
	rg.w.dx = ((COMM_HANDLE *)pa->handle)->h.port;
	int86(0x17, &rg, &rg);
	return 1;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL comm_flush_lpt(COM_PARA *para, _WORD which)
{
	if (which & COMM_FLUSH_READ)
	{
		while (comm_available(para))
			comm_readc(para);
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/
/*** ---------------------------------------------------------------------- ***/

LOCAL _LONG comm_available_x00(COM_PARA *pa)
{
	union REGS rg;
	
	rg.h.ah = 0x03;
	rg.w.dx = ((COMM_HANDLE *)pa->handle)->h.port;
	X00(&rg, &rg);
	if (rg.h.ah & 0x01)
		return 1;
	return 0;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _UBYTE comm_readc_x00(COM_PARA *pa)
{
	union REGS rg;
	
	rg.h.ah = 0x02;
	rg.w.dx = ((COMM_HANDLE *)pa->handle)->h.port;
	X00(&rg, &rg);
	return rg.h.al;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _LONG comm_read_x00(COM_PARA *pa, _UBYTE *buf, _LONG size)
{
	_LONG count;
	union REGS rg;
	struct SREGS sg;
	_UWORD rsize;
	
	count = 0;
	while (size)
	{
		if (size >= 0x2000l)
			rsize = 0x2000;
		else
			rsize = (_UWORD)size;
		rg.w.cx = rsize;
		rg.w.dx = ((COMM_HANDLE *)pa->handle)->h.port;
		rg.w.di = FP_OFF(buf);
		sg.es = FP_SEG(buf);
		rg.h.ah = 0x18;
		X00X(&rg, &rg, &sg);
		rsize = rg.w.ax;
		if (rsize <= 0)
			break;
		size -= rsize;
		buf += rsize;
		count += rsize;
	}
	return count;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _LONG comm_write_x00(COM_PARA *pa, CONST _UBYTE *buf, _LONG size)
{
	_LONG count;
	_UWORD wsize;
	union REGS rg;
	struct SREGS sg;
	
	count = 0;
	while (size)
	{
		if (size >= 0x2000)
			wsize = 0x2000;
		else
			wsize = (_UWORD)size;
		rg.w.cx = wsize;
		rg.w.dx = ((COMM_HANDLE *)pa->handle)->h.port;
		rg.w.di = FP_OFF(buf);
		sg.es = FP_SEG(buf);
		rg.h.ah = 0x19;
		X00X(&rg, &rg, &sg);
		wsize = rg.w.ax;
		if (wsize <= 0)
			break;
		buf += wsize;
		size -= wsize;
		count += wsize;
	}
	return count;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _LONG comm_writec_x00(COM_PARA *pa, _UBYTE c)
{
	union REGS rg;
	
	rg.h.ah = 0x01;
	rg.h.al = c;
	rg.w.dx = ((COMM_HANDLE *)pa->handle)->h.port;
	X00(&rg, &rg);
	return 1;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL comm_flush_x00(COM_PARA *para, _WORD which)
{
	if (which & COMM_FLUSH_READ)
	{
		while (comm_available(para))
			comm_readc(para);
	}
	return TRUE;
}
#endif /* !__WIN32__ */

/*** ---------------------------------------------------------------------- ***/
/*** ---------------------------------------------------------------------- ***/

#if GUI_VERSION
LOCAL _LONG comm_available_windows(COM_PARA *pa)
{
#if 0
#ifdef __WIN32__
	DWORD evmask;
	
	evmask = 0;
	if (GetCommMask(((COMM_HANDLE *)pa->handle)->h.fhandle, &evmask) &&
		(evmask & EV_RXCHAR))
		return 1;
#else
	if (GetCommEventMask(((COMM_HANDLE *)pa->handle)->h.chandle, 0) & EV_RXCHAR)
		return 1;
#endif
#else
	UNUSED(pa);
#endif
	return 0;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _UBYTE comm_readc_windows(COM_PARA *pa)
{
	COMM_HANDLE *handle = (COMM_HANDLE *) pa->handle;
	_LONG timeout = pa->timeout / RS_DELAY;
	_UBYTE c;
	
	while (timeout >= 0)
	{
#ifdef __WIN32__
		{
			DWORD   dwBytesRead;

			ReadFile(handle->h.fhandle, &c, 1, &dwBytesRead, NULL /* &ov */);
			if (dwBytesRead == 1)
				return c;
		}
#else
		if (ReadComm(handle->h.chandle, &c, 1) == 1)
			return c;
#endif /* __WIN32__ */
		EventTimer(RS_DELAY);		/* Rechenzeit fuer Server freigeben */
		timeout--;
	}
	return 0; /* TODO */
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _LONG comm_read_windows(COM_PARA *pa, _UBYTE *buf, _LONG size)
{
#ifdef __WIN32__
	COMM_HANDLE *handle = (COMM_HANDLE *) pa->handle;
	_LONG   retRead, outSize = COM_BLOCK_SIZE;
	_LONG   toDo = size;
	_UBYTE _HUGE *out = buf;

	do
	{
		if (toDo < COM_BLOCK_SIZE)
			outSize = (_WORD) toDo;

		{
			DWORD   dwBytesRead;

			ReadFile(handle->h.fhandle, out, outSize, &dwBytesRead, NULL /* &ov */);
			retRead = dwBytesRead;
		}
		toDo -= retRead;
		out += retRead;
		if (retRead != outSize)
		{
			_LONG   err;

			err = GetLastError();
			if (err != 0)
			{
				if (err != CE_OVERRUN)
					ErrorOut(EO_ERROR, "RS_Read %lx %ld", err, retRead);
				return size - toDo;
			}
		}

		if (toDo > 0)
		{
			/* Rechenzeit fuer Server freigeben */
			if (retRead == 0)	/* nicht 1 Byte geschrieben */
				EventTimer(500);
			else
				EventTimer(1);
		}
	} while (toDo > 0);

	return size - toDo;
#else
	COMM_HANDLE *handle = (COMM_HANDLE *) pa->handle;
	_LONG timeout = pa->timeout / RS_DELAY;
	_UBYTE *useBuf = buf;
	_LONG count = 0;
	
	while (timeout >= 0)
	{
		while (ReadComm(handle->h.chandle, useBuf, 1) == 1)
		{
			count++;
			useBuf++;
			size--;
			if (size == 0)
				return count;
		}
		if ((timeout & 0x0f) == 0)
			if (TASTEN_ABBRUCH)
				return count;
		EventTimer(RS_DELAY);		/* Rechenzeit fuer Server freigeben */
		timeout--;
	}
	return count;
#endif /* __WIN32__ */
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _LONG comm_write_windows(COM_PARA *pa, CONST _UBYTE *buf, _LONG size)
{
#ifdef __WIN32__
	COMM_HANDLE *handle = (COMM_HANDLE *) pa->handle;
	_LONG   retWrite, outSize = COM_BLOCK_SIZE;
	_LONG   toDo = size;
	CONST _UBYTE _HUGE *out = buf;

	do
	{
		if (toDo < COM_BLOCK_SIZE)
			outSize = (_WORD) toDo;

		{
			DWORD   dwBytesWrite;

			WriteFile(handle->h.fhandle, out, outSize, &dwBytesWrite, NULL /* &ov */);
			retWrite = dwBytesWrite;
		}
		toDo -= retWrite;
		out += retWrite;
		if (retWrite != outSize)
		{
			_LONG   err;

			err = GetLastError();
			if (err != 0)
				Pling();
		}

		if (toDo > 0)
		{
			/* Rechenzeit fuer Server freigeben */
			if (retWrite == 0)	/* nicht 1 Byte geschrieben */
				EventTimer(500);
			else
				EventTimer(1);
		}
	} while (toDo > 0);

	return size - toDo;
#else
	COMM_HANDLE *handle = (COMM_HANDLE *) pa->handle;
	_LONG timeout = pa->timeout / RS_DELAY;
	_LONG count = 0;
	CONST _UBYTE *useBuf = buf;
	
	while (timeout >= 0 && size)
	{
		while (WriteComm(handle->h.chandle, useBuf, 1) == 1)
		{
			count++;
			useBuf++;
			size--;
			if (size == 0)
				return count;
		}
		if ((timeout & 0x0f) == 0)
			if (TASTEN_ABBRUCH)
				break;
		EventTimer(RS_DELAY);		/* Rechenzeit fuer Server freigeben */
		timeout--;
	}
	return count;
#endif /* __WIN32__ */
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _LONG comm_writec_windows(COM_PARA *pa, _UBYTE c)
{
	COMM_HANDLE *handle = (COMM_HANDLE *) pa->handle;
	_LONG timeout = pa->timeout / RS_DELAY;
	
	while (timeout >= 0)
	{
#ifdef __WIN32__
		{
			DWORD   dwBytesWrite;

			WriteFile(handle->h.fhandle, &c, 1, &dwBytesWrite, NULL /* &ov */);
			if (dwBytesWrite == 1)
				return 1;
		}
#else
		if (WriteComm(handle->h.chandle, &c, 1) == 1)
			return 1;
#endif /* __WIN32__ */
		EventTimer(RS_DELAY);		/* Rechenzeit fuer Server freigeben */
		timeout--;
	}
	return 0;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL comm_flush_windows(COM_PARA *pa, _WORD which)
{
	COMM_HANDLE *handle = (COMM_HANDLE *) pa->handle;

#ifdef __WIN32__
	DWORD action;
	
	action = 0;
	if (which & COMM_FLUSH_READ)
		action |= PURGE_RXABORT|PURGE_RXCLEAR;
	if (which & COMM_FLUSH_WRITE)
		action |= PURGE_TXABORT|PURGE_TXCLEAR;
	if (action)
		PurgeComm(handle->h.fhandle, action);
#else
	if (which & COMM_FLUSH_WRITE)
		FlushComm(handle->h.chandle, 0);
	if (which & COMM_FLUSH_READ)
		FlushComm(handle->h.chandle, 1);
#endif /* __WIN32__ */
	return TRUE;
}

#endif /* GUI_VERSION */

/*** ---------------------------------------------------------------------- ***/
/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL comm_init(COM_PARA *pa, _WORD portnum)
{
	COMM_HANDLE *handle;

	handle = NEW(COMM_HANDLE, "comm_init");
	if (handle == NULL)
		return FALSE;
	pa->handle = handle;
	handle->type = CH_NONE;
#if GUI_VERSION
	UNUSED(portnum);
	{
		CONST _UBYTE *name = comm_dev_name(pa->dev);
		
		pa->mode = PA_WINDOWS;

		pa->comm_available = comm_available_windows;
		pa->comm_readc = comm_readc_windows;
		pa->comm_read = comm_read_windows;
		pa->comm_write = comm_write_windows;
		pa->comm_writec = comm_writec_windows;
		pa->comm_flush = comm_flush_windows;

#ifdef __WIN32__
		handle->h.fhandle = CreateFile(name, GENERIC_READ | GENERIC_WRITE,
							 0,		/* exclusive access */
							 NULL,	/* no security attrs */
							 OPEN_EXISTING,
							 FILE_ATTRIBUTE_NORMAL /*| FILE_FLAG_OVERLAPPED */,
							 NULL);
		if (handle->h.fhandle >= 0)
		{
			COMMTIMEOUTS CommTimeOuts;
	
			handle->type = CH_FHANDLE;
			/* SetCommMask(handle->h.fhandle, EV_RXCHAR) ; */
			comm_flush(pa, COMM_FLUSH_ALL);

			if (!SetupComm(handle->h.fhandle, COM_BLOCK_SIZE, COM_BLOCK_SIZE))
				ErrorOut(EO_TRACE, "SetupComm %ld %lx", (long)DWORD_FROM_HANDLE(handle->h.fhandle), (unsigned long)GetLastError());
	
			/* set up for overlapped non - blocking I / O */
	
			CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFFUL;
			CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
			CommTimeOuts.ReadTotalTimeoutConstant = pa->timeout;
			CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
			CommTimeOuts.WriteTotalTimeoutConstant = pa->timeout;
			if (!SetCommTimeouts(handle->h.fhandle, &CommTimeOuts))
				ErrorOut(EO_TRACE, "SetCommTimeouts %ld %lx", (long)DWORD_FROM_HANDLE(handle->h.fhandle), (unsigned long)GetLastError());
		}
#else
		handle->h.chandle = OpenComm((LPSTR) name, COM_BLOCK_SIZE, COM_BLOCK_SIZE);

		if (handle->h.chandle >= 0)
		{
			handle->type = CH_CHANDLE;
			comm_flush(pa, COMM_FLUSH_ALL);
		}
		
#endif /* __WIN32__ */

		if (handle->type != CH_NONE)
		{
			DCB     dcb;
	
#ifdef __WIN32__
			if (!GetCommState(handle->h.fhandle, &dcb))
				ErrorOut(EO_FATAL, "GetCommState %lx", (unsigned long)GetLastError());
			else
#else
			if (GetCommState(handle->h.chandle, &dcb) < 0)
				ErrorOut(EO_FATAL, "GetCommState");
			else
#endif /* __WIN32__ */
			{
				_BOOL   hardProt = FALSE;
				_BOOL   softProt = FALSE;
	
				dcb.fBinary = TRUE;
				dcb.fParity = TRUE;
#ifdef __WIN32__
				dcb.BaudRate = pa->baud;
#else
				dcb.BaudRate = dcb_baud(pa->baud);
#endif
				dcb.ByteSize = (BYTE)pa->bits;
	
				switch (pa->stopbits)
				{
				default:
					dcb.StopBits = ONE5STOPBITS;
					break;
				case 2:
					dcb.StopBits = TWOSTOPBITS;
					break;
				case 1:
					dcb.StopBits = ONESTOPBIT;
					break;
				}
	
				switch (pa->parity)
				{
				case PA_EVEN:
					dcb.Parity = EVENPARITY;
					break;
				case PA_ODD:
					dcb.Parity = ODDPARITY;
					break;
				default:
					dcb.Parity = NOPARITY;
					break;
				}
	
				switch (pa->protokoll)
				{
				case PR_NONE:
					break;
	
				case PR_XON:
					softProt = TRUE;
					break;
	
				case PR_RTS:
					hardProt = TRUE;
					break;
	
				case PR_ALL:
				default:
					hardProt = TRUE;
					softProt = TRUE;
					break;
				}
	
				/* setup hardware flow control */
#ifdef __WIN32__
				dcb.fOutxDsrFlow = hardProt;
				if (hardProt)
					dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
				else
					dcb.fDtrControl = DTR_CONTROL_ENABLE;
				dcb.fOutxCtsFlow = hardProt;
				if (hardProt)
					dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
				else
					dcb.fRtsControl = RTS_CONTROL_ENABLE;
#else
				/* dcb.fOutxDsrFlow = dcb.fDtrflow = hardProt ; */
				/* dcb.DsrTimeout = (hardProt) ? 30 : 0; */
				if (hardProt)
					dcb.fRtsDisable = FALSE;
				else
					dcb.fRtsDisable = TRUE;
	
				dcb.fOutxCtsFlow = dcb.fRtsflow = hardProt;
				/* dcb.CtsTimeout = (hardProt) ? 30 : 0;  */
#endif /* __WIN32__ */
	
				/* setup software flow control */
	
				dcb.fInX = dcb.fOutX = softProt;
				dcb.XonChar = 0x11;
				dcb.XoffChar = 0x13;
				dcb.XonLim = 100;
				dcb.XoffLim = 100;
	
#ifdef __WIN32__
				if (SetCommState(handle->h.fhandle, &dcb))
#else
				if (SetCommState(&dcb) == 0)
#endif
				{
#if 0
					EscapeCommFunction(handle->h.chandle, CLRDTR);
					EscapeCommFunction(handle->h.chandle, CLRRTS);
					EscapeCommFunction(handle->h.chandle, SETDTR);
					EscapeCommFunction(handle->h.chandle, SETRTS);
#endif
#ifdef __WIN32__
					EscapeCommFunction(handle->h.fhandle, SETDTR);
					SetCommMask(handle->h.fhandle, EV_RXCHAR);
#else
					EscapeCommFunction(handle->h.chandle, SETDTR);
					SetCommEventMask(handle->h.chandle, EV_RXCHAR);
#endif
					comm_flush(pa, COMM_FLUSH_ALL);
					return TRUE;
				}
#ifdef __WIN32__
				ErrorOut(EO_FATAL, "SetCommState %lx", (unsigned long)GetLastError());
#else
				ErrorOut(EO_FATAL, "SetCommState");
#endif
				OFREE(handle);
				return FALSE;
			}
			
		}
	}

#else /* !GUI_VERSION */

	if (pa->mode != PA_LPT && pa->mode != PA_MIDI)
		pa->mode = PA_FOSSIL;
	
#endif

#ifndef __WIN32__
	if (pa->mode == PA_FOSSIL)
	{
		union REGS rg;

		rg.h.ah = 0x1c;
		rg.w.dx = portnum;
		X00(&rg, &rg);
		if (rg.w.ax != 0x1954)
		{
			pa->mode = PA_BIOS;
		}
	}
	
	if (pa->mode == PA_FOSSIL)
	{
		union REGS rg;
		_UWORD baud;
		
		switch (pa->bits)
		{
			case 5: rg.h.ch = 0; break;
			case 6: rg.h.ch = 1; break;
			case 7: rg.h.ch = 2; break;
			case 8: rg.h.ch = 3; break;
		}
		switch (pa->parity)
		{
			case PA_NONE: rg.h.bh = 0; break;
			case PA_ODD: rg.h.bh = 1; break;
			case PA_EVEN: rg.h.bh = 2; break;
		}
		switch (pa->stopbits)
		{
			case 1: rg.h.bl = 0; break;
			case 2: rg.h.bl = 1; break;
		}
		
		baud = baud_val(pa->baud);
		rg.h.cl = baud;
		rg.w.dx = portnum;
		rg.h.ah = 0x1e;
		rg.h.al = 0;
		X00(&rg, &rg);
		if (rg.w.cflag)
		{
			pa->mode = PA_BIOS;
		}
		
		switch (pa->protokoll)
		{
			case PR_NONE: rg.h.al = 0x00; break;
			case PR_XON:  rg.h.al = 0x09; break;
			case PR_RTS:  rg.h.al = 0x02; break;
			case PR_ALL:  rg.h.al = 0x0b; break;
		}

		rg.w.dx = portnum;
		rg.h.ah = 0x0f;
		X00(&rg, &rg);
		
		if (pa->mode == PA_BIOS)
		{
			rg.h.ah = 0x1d;
			rg.w.dx = portnum;
			X00(&rg, &rg);
		}

		if (pa->mode == PA_FOSSIL)
		{
			pa->comm_available = comm_available_x00;
			pa->comm_readc = comm_readc_x00;
			pa->comm_read = comm_read_x00;
			pa->comm_write = comm_write_x00;
			pa->comm_writec = comm_writec_x00;
			pa->comm_flush = comm_flush_x00;
		}
	}
	
	if (pa->mode == PA_BIOS)
	{
		union REGS rg;
		_UWORD baud;
		_UBYTE param = 0;
		
		pa->mode = PA_BIOS;

		pa->comm_available = comm_available_bios;
		pa->comm_readc = comm_readc_bios;
		pa->comm_read = comm_read_bios;
		pa->comm_write = comm_write_bios;
		pa->comm_writec = comm_writec_bios;
		pa->comm_flush = comm_flush_bios;

		switch (pa->bits)
		{
			case 8: param |= 0x03; break;
			case 7: param |= 0x02; break;
			case 6: param |= 0x01; break;
			case 5: param |= 0x00; break;
		}
		switch (pa->parity)
		{
			case PA_EVEN: param |= 0x18; break;
			case PA_ODD: param |= 0x08; break;
			case PA_NONE: break;
		}
		switch (pa->stopbits)
		{
			case 1: break;
			case 2: param |= 0x04; break;
		}
		baud = baud_val(pa->baud) << 5;
		param |= baud;
		
		rg.h.ah = 0;
		rg.h.al = param;
		rg.w.dx = portnum;
		
		int86(0x14, &rg, &rg);
	}
	
	if (pa->mode == PA_BIOS || pa->mode == PA_FOSSIL)
	{
		_UBYTE line_ctrl;
		_UWORD baud;
		_UWORD port;
		
		port = comm_port(pa->dev);
		
		baud = baud_time(&pa->baud);
	
		line_ctrl = inp(port+3);
		outp(port+3, line_ctrl|0x80);
		outpw(port, baud);
		outp(port+3, line_ctrl);
		handle->type = CH_PORT;
		handle->h.port = portnum;
	}
	
	if (pa->mode == PA_LPT)
	{
		union REGS rg;
		
		pa->comm_available = comm_available_lpt;
		pa->comm_readc = comm_readc_lpt;
		pa->comm_read = comm_read_lpt;
		pa->comm_write = comm_write_lpt;
		pa->comm_writec = comm_writec_lpt;
		pa->comm_flush = comm_flush_lpt;
	
		rg.h.ah = 0x02;
		rg.w.dx = portnum;
		int86(0x17, &rg, &rg);
		if ((rg.h.ah & 0x09) != 0x09)
		{
			OFREE(handle);
			return FALSE;
		}
		handle->type = CH_PORT;
		handle->h.port = portnum;
	}
#endif /* !__WIN32__ */
	
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL comm_open(COM_PARA *pa)
{
	_WORD num;
	
	get_devices();
	if (try_comm(pa->dev) < 0)
	{
		ErrorOut(EO_FATAL, "Rs_OpenDev");
		return FALSE;
	}
	
	switch (pa->dev)
	{
	case DEV_COM1:
	case DEV_COM2:
	case DEV_COM3:
	case DEV_COM4:
	case DEV_COM5:
	case DEV_COM6:
	case DEV_COM7:
	case DEV_COM8:
		num = pa->dev - DEV_COM1;
		pa->mode = PA_WINDOWS;
		if (comm_init(pa, num))
			return TRUE;
		break;
		
	case DEV_LPT1:
	case DEV_LPT2:
	case DEV_LPT3:
		num = pa->dev - DEV_LPT1;
		pa->mode = PA_LPT;
		if (comm_init(pa, num))
			return TRUE;
		break;
	}
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL comm_set_dtr(COM_PARA *para, _BOOL on)
{
#if GUI_VERSION
	COMM_HANDLE *hnd = (COMM_HANDLE *) para->handle;

	switch (para->mode)
	{
	case PA_WINDOWS:
		if (on)
		{
#ifdef __WIN32__
			EscapeCommFunction(hnd->h.fhandle, CLRDTR);
			EscapeCommFunction(hnd->h.fhandle, SETDTR);
#else
			EscapeCommFunction(hnd->h.chandle, CLRDTR);
			EscapeCommFunction(hnd->h.chandle, SETDTR);
#endif
		} else
		{
#ifdef __WIN32__
			EscapeCommFunction(hnd->h.fhandle, CLRDTR);
#else
			EscapeCommFunction(hnd->h.chandle, CLRDTR);
#endif
		}
		break;
	}
#else
	UNUSED(para);
	UNUSED(on);
#endif
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID comm_close(COM_PARA *para)
{
	COMM_HANDLE *hnd = (COMM_HANDLE *) para->handle;

	if (hnd == NULL)
		return;
	
	switch (para->mode)
	{
#ifndef __WIN32__
	case PA_FOSSIL:
		{
			union REGS rg;
	
			rg.h.ah = 0x1d;
			rg.w.dx = hnd->h.port;
			X00(&rg, &rg);
		}
		break;
#endif

#if GUI_VERSION
	case PA_WINDOWS:
		comm_flush(para, COMM_FLUSH_ALL);
#ifdef __WIN32__
		EscapeCommFunction(hnd->h.fhandle, CLRDTR);
		CloseHandle(hnd->h.fhandle);
#else
		EscapeCommFunction(hnd->h.chandle, CLRDTR);
		CloseComm(hnd->h.chandle);
#endif
		break;
#endif /* GUI_VERSION */
	}
	OFREE(hnd);
	para->handle = NULL;
}

