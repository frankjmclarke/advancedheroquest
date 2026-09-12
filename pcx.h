#ifndef __PCX_H__
#define __PCX_H__

#include <stdio.h>
#ifndef __PORTAB_H__
#include <portab.h>
#endif
#ifndef __MFDB_H__
#include <mfdb.h>
#endif

_BOOL mfdb_to_pcx(FILE *fd, MFDB *mfdb, _BOOL compress, _WORD zoom);

#endif /* __PCX_H__ */
