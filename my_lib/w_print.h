#ifndef __W_PRINT_H__
#define __W_PRINT_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif
#ifndef __MFDB_H__
#include <mfdb.h>
#endif

#ifndef __PRINTER_IMPLEMENTATION__
DUMMY_STRUCT(_printer);
#endif

typedef struct _printer PRINTER;


PRINTER *Printer_Open(CONST _UBYTE *PrintJob, _BOOL grafic_dev);
_VOID Printer_NewPage(PRINTER *printer);
_BOOL Printer_NewLine(PRINTER *printer);
_VOID Printer_Close(PRINTER *printer);
_BOOL Printer_Write(PRINTER *printer, CONST _UBYTE *buf, _LONG size);
_VOID Printer_Bitmap(PRINTER *printer, MFDB *src, _WORD x, _WORD y, _WORD w, _WORD h, _WORD dx, _WORD dy, _WORD zoom);

#endif /* __W_PRINT_H__ */
