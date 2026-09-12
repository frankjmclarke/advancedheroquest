#ifndef __BMP_H__
#define __BMP_H__

#include <stdio.h>
#ifndef __PORTAB_H__
#include <portab.h>
#endif
#ifndef __MFDB_H__
#include <mfdb.h>
#endif

_BOOL mfdb_to_bmp(FILE *fd, MFDB *mfdb, _BOOL compress, _WORD zoom);

#endif /* __BMP_H__ */
