#include <stdio.h>
#include <windows_.h>
#include <path.h>
#include <routine.h>
#include <file_io.h>
#include <filesel.h>

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL _VOID init_path(PATH *path, _UBYTE *select, _UBYTE *text)
{
	get_path(path);
	strBcpy(path->select, select);
	strBcpy(path->text, text);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID set_filename(PATH *p, _UBYTE *str)
{
	strBcpy(p->pathname, p->path);
	F_Path_Append(p->pathname, str);
	strBcpy(p->filename, str);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID set_path(PATH *p, _UBYTE *str)
{
	strBcpy(p->path, str);
	strcpy(p->pathname, p->path);
	F_Path_Append(p->pathname, p->filename);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID get_path(PATH *p)
{
	F_Path_Get(p->path);
   	strcpy(p->pathname, p->path);
   	p->filename[0] = '\0';
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID cpy_path(PATH *d, PATH *s)
{
	strcpy(d->path, s->path);
	strcpy(d->pathname, s->pathname);
	strcpy(d->filename, s->filename);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID set_ext(PATH *p, CONST _UBYTE *s)
{
	_UBYTE *ptr;
	
   	ptr = strrchr(p->filename, '.');
   	if (ptr != NULL)
   	{
   		*ptr = '\0';
   	}
   	if (*s != '.')
   	{
		strBcat(p->filename, ".");
   	}
	strBcat(p->filename, s);
	strcpy(p->pathname, p->path);
	F_Path_Append(p->pathname, p->filename);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID set_select(PATH *p, _UBYTE *s)
{
	strBcpy(p->select, s);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL get_filename(PATH *p)
{
	_UBYTE path[sizeof(p->path)];
	_UBYTE oldPath[PATH_MAX];
	_BOOL retV = FALSE;
	
	F_Path_Get(oldPath);
	strcpy(path, p->path);
	F_Path_Append(path, p->filename);
	if (FileSelect(path, p->select, p->text))
   	{
      	strBcpy(p->path, path);
      	F_Path_Dirname(p->path);
      	strBcpy(p->filename, F_Path_Basename(path));
      	strcpy(p->pathname, p->path);
      	F_Path_Append(p->pathname, p->filename);
      	retV = TRUE;
   	}
   	F_Path_Set(oldPath);
   	return retV;
}
