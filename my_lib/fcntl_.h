/*****************************************************************************
 * FCNTL_.H
 *****************************************************************************/

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif

#ifndef O_RDONLY
#  define O_RDONLY 0
#endif
#ifndef O_BINARY
#  define O_BINARY 0
#endif
