#ifndef __PATH_H__
#define __PATH_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif
#ifndef __PATH_MAX_H__
#include <path_max.h>
#endif

typedef struct {
	_UBYTE path[PATH_MAX];		/* Pfad					*/
	_UBYTE pathname[PATH_MAX];	/* Pfad+Filename		*/
	_UBYTE filename[PATH_MAX];	/* Filename pur			*/
	_UBYTE select[PATH_MAX];		/* Selector				*/
	_UBYTE text[80];				/* Text fuer Selector	*/
} PATH;

_BOOL get_filename(PATH *path);
_VOID init_path(PATH *path, _UBYTE *select, _UBYTE *text);
_VOID set_filename(PATH *p, _UBYTE *str);
_VOID set_path(PATH *p, _UBYTE *str);
_VOID get_path(PATH *path);
_VOID cpy_path(PATH *path1, PATH *path2);
_VOID set_ext(PATH *path, CONST _UBYTE *ext);
_VOID set_select(PATH *path, _UBYTE *sel);

#endif /* __PATH_H__ */
