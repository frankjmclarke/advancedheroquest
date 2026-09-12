/*****************************************************************************
 * PATH_MAX.H
 *****************************************************************************/

#ifndef __PATH_MAX_H__
#define __PATH_MAX_H__

#ifndef __PORTAB_H__
#  include <portab.h>
#endif

#ifndef PATH_MAX
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif
#endif

#endif /* __PATH_MAX_H__ */
