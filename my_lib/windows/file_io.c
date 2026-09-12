/*****************************************************************************
 * WINDOWS/FILE_IO.C
 *****************************************************************************/

#include <file_io.h>
#include <windows_.h>
#include <stdio.h>
#include <errno_.h>
#ifndef __WIN32__
#include <dos.h>
#endif
#include <direct.h>
#include <file_io.h>
#include <ro_mem.h>
#include <termproc.h>
#include <debug.h>
#include <fcntl_.h>
#include <routine.h>
#include <sys/stat.h>
#if GUI_VERSION
#ifndef HFILE_ERROR
#define HFILE_ERROR ((HFILE)-1)
#endif
#else
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif

#define ATR_FILE 0
#define ATR_SUBDIR FA_DIREC


#if GUI_VERSION
typedef HFILE DOSFNO;
#define LEGAL_DOSFNO(handle) ((handle) != HFILE_ERROR)
#else
typedef int DOSFNO;
#define LEGAL_DOSFNO(handle) ((handle) >= 0)
#endif



typedef struct
{
	_BOOL	open;
	_BOOL	used;
	DOSFNO	handle;
	_WORD	mode;
	_LONG	seekPos;
	_UBYTE	path[PATH_MAX];
	_BOOL	free;
} FILE_BUF;


#define VALID_FILENO(fno) (TO_HANDLE(fno) >= 0 && TO_HANDLE(fno) < fi_buf_size)

#ifdef DEBUG
LOCAL _WORD   OpenFiles;
#endif

#define SavePD()
#define RestorePD()

#define NETERR EAGAIN
#define NETWAIT()  EventTimer(200)

LOCAL FILE_BUF *FiBuf = NULL;
LOCAL _WORD fi_buf_size = 0;

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

LOCAL _WORD sys_errno(_WORD err)
{
	switch (err)
	{
		case  0: return GERR_OK;
		case  1: return GERR_INVFN;
		case  2: return GERR_NOENT;
		case  3: return GERR_NOPATH;
		case  4: return GERR_MFILE;
		case  5: return GERR_ACCESS;
		case  6: return GERR_BADF;
		case  7: return GERR_FAULT;
		case  8: return GERR_NOMEM;
		case  9: return GERR_FAULT;
		case 10: return GERR_FAULT;
		case 11: return GERR_FAULT;
		case 12: return GERR_INVAL;
		case 13: return GERR_INVAL;
		case 14: return GERR_FAULT;
		case 15: return GERR_NODEV;
		case 16: return GERR_BUSY;
		case 17: return GERR_XDEV;
		case 18: return GERR_NOENT;
		case 19: return GERR_ROFS;
		case 20: return GERR_NXIO;
		case 21: return GERR_NOTRDY;
		case 22: return GERR_INVAL;
		case 23: return GERR_IO;
		case 24: return GERR_INVAL;
		case 25: return GERR_SPIPE;
		case 26: return GERR_IO;
		case 27: return GERR_IO;
		case 28: return GERR_NOSPC;
		case 29: return GERR_IO;
		case 30: return GERR_IO;
		case 31: return GERR_IO;
		case 32: return GERR_PIPE;
		case 33: return GERR_DEADLOCK;
		case 34: return GERR_RANGE;
		case 35: return GERR_BADF;
		case 36: return GERR_NOMEM;
		case 51: return GERR_NOTRDY;
		case 52: return GERR_EXIST;
		case 53: return GERR_NOPATH;
		case 54: return GERR_BUSY;
		case 55: return GERR_NXIO;
		case 56: return GERR_2BIG;
		case 57: return GERR_IO;
		case 58: return GERR_IO;
		case 59: return GERR_FAULT;
		case 60: return GERR_NXIO;
		case 61: return GERR_NOTRDY;
		case 62: return GERR_NOSPC;
		case 63: return GERR_INTR;
		case 64: return GERR_NOENT; 
		case 65: return GERR_ACCESS;
		case 66: return GERR_NODEV;
		case 67: return GERR_NOENT;
		case 68: return GERR_RANGE;
		case 69: return GERR_NOTRDY;
		case 70: return GERR_INTR;
		case 71: return GERR_ACCESS;
		case 72: return GERR_INTR;
		case 80: return GERR_EXIST;
		case 82: return GERR_ACCESS;
		case 86: return GERR_PERM;
		case 87: return GERR_INVAL;
		case 88: return GERR_IO;
	}
	return GERR_IO;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID FExit(_VOID)
{
	if (FiBuf != NULL)
	{
		SFREE(FiBuf);
		FiBuf = NULL;
	}
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL FInit(_VOID)
{
	_WORD ii;
	
	if (FiBuf == NULL)
	{
		FiBuf = CALLOC(36, sizeof(FILE_BUF), "Finit");
		if (FiBuf == NULL)
			return FALSE;
			
		fi_buf_size = 36;
		InstallTermproc(FExit);
		
		for (ii = 0; ii < fi_buf_size; ii++)
		{
			FiBuf[ii].open = FALSE;
			FiBuf[ii].used = FALSE;
			FiBuf[ii].seekPos = 0;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL F_ERROR doFopen(CONST _UBYTE *filename, _WORD mode, DOSFNO *handle)
{
#if GUI_VERSION || CONSOLE_VERSION
	_UBYTE	dumStr[PATH_MAX];
	UINT wiMode;
	OFSTRUCT of;

	switch (mode & FO_ACCMODE)
	{
	case FO_RDONLY:
		wiMode = OF_READ | OF_SHARE_DENY_NONE;
		break;
	case FO_WRONLY:
		wiMode = OF_WRITE | OF_SHARE_DENY_READ;
		break;
	default:
		wiMode = OF_READWRITE | OF_SHARE_EXCLUSIVE;
		break;
	}

	strBcpy(dumStr, filename);
	FNAME_TO_WINDOWS(dumStr);
	filename = dumStr;

	*handle = OpenFile(filename, &of, OF_READ|OF_EXIST|OF_SHARE_DENY_NONE);
	if (*handle != HFILE_ERROR)
	{
		if ((mode & (FO_CREAT | FO_EXCL)) == (FO_CREAT | FO_EXCL))
		{
			return GERR_EXIST;
		}
		*handle = OpenFile(filename, &of, wiMode);
		if ((mode & FO_TRUNC) && (*handle != HFILE_ERROR))
		{
			/* Give up if the mode flags conflict */
			if ((mode & FO_ACCMODE) == FO_RDONLY)
			{
				_lclose(*handle);
				return GERR_ACCESS;
			}
			_lclose(*handle);
			*handle = OpenFile(filename, &of, wiMode | OF_CREATE);
			if (*handle < 0)
			{
#ifdef __WIN32__
				return sys_errno(GetLastError());
#else
				return GERR_ACCESS;
#endif
			}
		}
	} else					/* file doesn't exist */
	{
		if (mode & FO_CREAT)
		{
			*handle = OpenFile(filename, &of, wiMode | OF_CREATE);
			if (*handle < 0)
			{
#ifdef __WIN32__
				return sys_errno(GetLastError());
#else
				return GERR_ACCESS;
#endif
			}
		} else
		{
#ifdef __WIN32__
			return sys_errno(GetLastError());
#else
			return GERR_NOENT;
#endif
		}
	}

	if (*handle == HFILE_ERROR)
	{
#ifdef __WIN32__
		return sys_errno(GetLastError());
#else
		return GERR_NOENT;
#endif
	}
	if (mode & FO_APPEND)
		(void)_llseek(*handle, 0L, SEEK_END);

	return GERR_OK;
#else /* !GUI_VERSION */
	int wiMode;

	switch (mode & FO_ACCMODE)
	{
	case FO_RDONLY:
		wiMode = O_RDONLY|O_BINARY;
		break;
	case FO_WRONLY:
		wiMode = O_WRONLY|O_BINARY;
		break;
	default:
		wiMode = O_RDWR|O_BINARY;
		break;
	}
	if (mode & FO_APPEND)
		wiMode |= O_APPEND;
	if (mode & FO_CREAT)
		wiMode |= O_CREAT;
	if (mode & FO_TRUNC)
		wiMode |= O_TRUNC;
	if (mode & FO_SYNC)
		{}
	if (mode & FO_EXCL)
		wiMode |= O_EXCL;
	if (mode & FO_NDELAY)
		{}
	if ((*handle = (int)open(filename, FO_RDONLY, 0666)) >= 0)		/* file exists */
	{
		close(*handle);
		if ((mode & (FO_CREAT | FO_EXCL)) == (FO_CREAT | FO_EXCL))
		{
			return GERR_EXIST;
		}
		*handle = (int)open(filename, wiMode, 0666);
		if ((mode & FO_TRUNC) && (*handle >= 0))
		{
			/* Give up if the mode flags conflict */
			if (mode & O_RDONLY)
			{
				close(*handle);
				return GERR_ACCESS;
			}
			(void)close(*handle);
			*handle = (int)creat(filename, 0644);
			if (*handle < 0)
			{
				return sys_errno(errno);
			}
		}
	} else					/* file doesn't exist */
	{
		if (mode & FO_CREAT)
		{
			*handle = (int)creat(filename, 0644);
			if (*handle >= 0)
			{
				(void)close(*handle);
				*handle = (int)open(filename, wiMode);
			}
			if (*handle < 0)
				return sys_errno(errno);
		} else
		{
			return GERR_NOENT;
		}
	}

	if (!LEGAL_DOSFNO(*handle))
		return sys_errno(errno);
	
	if (mode & FO_APPEND)
		(void)lseek(*handle, 0L, SEEK_END);

	return GERR_OK;
#endif /* GUI_VERSION */

}

/*** ---------------------------------------------------------------------- ***/

LOCAL F_ERROR FiBufOpen(CONST _UBYTE *datei, _WORD mode, _LONG offset, DOSFNO *handle)
{
	F_ERROR err;
	
	if ((err = doFopen(datei, mode, handle)) == GERR_OK)
	{
		if (offset != 0)
		{
#if GUI_VERSION
			_llseek(*handle, offset, 0);
#else
			lseek(*handle, offset, 0);
#endif
		}
	}
	return err;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL F_ERROR sysFclose(FILE_BUF *ff)
{
	_WORD retV = 0;

	if (ff->open)
	{
		SavePD();
#if GUI_VERSION
		retV = _lclose(ff->handle);
#else
		retV = close(ff->handle);
#endif
		if (retV != 0)
			retV = errno;
		RestorePD();
		ff->open = FALSE;
		
#ifdef DEBUG
		OpenFiles--;
		ErrorOut(EO_TRACE, "***C %d       ", OpenFiles);
#endif
	}
	return sys_errno(retV);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL F_ERROR FiBufClose(FILENO fd)
{
	F_ERROR retV;
	FILE_BUF *ff;

	if (!VALID_FILENO(fd))
	{
		ErrorOut(EO_ERROR, "FiBufClose: invalid file number %ld", (_LONG) TO_HANDLE(fd));
		return GERR_BADF;
	}
	
	if (FiBuf == NULL)
	{
		ErrorOut(EO_ERROR, "FiBufClose: not initialized");
		return GERR_INVAL;
	}
	
	ff = &FiBuf[TO_HANDLE(fd)];

	retV = sysFclose(ff);
	ff->used = FALSE;
	return retV;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL FILENO FiBufSave(DOSFNO handle, CONST _UBYTE *path, _WORD mode)
{
	_WORD   ii;
	FILE_BUF *ff;
	
	if (FiBuf == NULL)
		if (!FInit())
		{
			return GERR_NOMEM;
		}
	
	for (ii = 0; ii < fi_buf_size; ii++)
	{
		if (!((ff = &FiBuf[ii])->used))
		{
			ff->open = TRUE;
			ff->used = TRUE;
			ff->handle = handle;
			ff->mode = mode & (FO_ACCMODE | FO_SHMODE);
			ff->seekPos = 0;
			strcpy(ff->path, path);

			return TO_FILENO(ii);
		}
	}
	return GERR_NFILE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL F_ERROR FiBufHandle(FILENO fd, DOSFNO *handle)
{
	FILE_BUF *ff;
	F_ERROR err;
	
	if (!VALID_FILENO(fd))
	{
		ErrorOut(EO_ERROR, "FiBufHandle: invalid file number %ld", (_LONG) TO_HANDLE(fd));
		return GERR_BADF;
	}
	
	if (FiBuf == NULL)
	{
		ErrorOut(EO_ERROR, "FiBufHandle: not initialized");
		return GERR_INVAL;
	}
	
	ff = &FiBuf[TO_HANDLE(fd)];

	if (!ff->used)
		return GERR_BADF;

	if (!ff->open)
	{
		if ((err = FiBufOpen(ff->path, ff->mode, ff->seekPos, handle)) == GERR_OK)
		{
			ff = &FiBuf[TO_HANDLE(fd)]; /* FiBuf may have changed */
			ff->open = TRUE;
			ff->handle = *handle;
			return GERR_OK;
		}
		return err;
	}
	*handle = ff->handle;
	return GERR_OK;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL FILENO F_File_Open(CONST _UBYTE *datei, _WORD mode)
{
	FILENO retV;
	F_ERROR err;
	DOSFNO handle;
	
	if ((err = FiBufOpen(datei, mode, 0, &handle)) == GERR_OK)
		retV = FiBufSave(handle, datei, mode);
	else
		retV = TO_FILENO(err);
#ifdef DEBUG_TXT
	ErrorOut(EO_TRACE, "F_File_Open %s: %ld", datei, (_LONG)TO_HANDLE(retV));
#endif
	return retV;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL F_ERROR F_File_Close(FILENO fd)
{
	return FiBufClose(fd);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL F_ERROR F_File_Delete(CONST _UBYTE *fname)
{
#if GUI_VERSION || CONSOLE_VERSION
#ifdef __WIN32__
	_UBYTE name[PATH_MAX];
	
	strBcpy(name, fname);
	FNAME_TO_WINDOWS(name);
	if (DeleteFile(name))
		return GERR_OK;
	return sys_errno(GetLastError());
#else
	_UBYTE name[PATH_MAX];
	OFSTRUCT s;
	
	strBcpy(name, fname);
	FNAME_TO_WINDOWS(name);
	s.cBytes = sizeof(s);
	if (OpenFile(name, &s, OF_DELETE) != HFILE_ERROR)
		return GERR_OK;
	return sys_errno(errno);
#endif /* __WIN32__ */
#else
	if (remove(fname) == 0)
		return GERR_OK;
	return sys_errno(errno);
#endif /* GUI_VERSION */
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _UBYTE *F_Path_Append(_UBYTE *path, CONST _UBYTE *name)
{
	if (*path != '\0' && path[strlen (path) - 1] != '\\')
		strMcat(path, PATH_MAX, "\\");
	return strMcat(path, PATH_MAX, name);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL F_Path_Get(_UBYTE *pfad)
{
#if defined(__WIN32__) || defined(__CYGWIN32__)
	*pfad = '\0';
	if (GetCurrentDirectory(PATH_MAX - 2, pfad) == 0)
		return FALSE;
	F_Path_Append(pfad, "");
	FNAME_FROM_WINDOWS(pfad);
	return TRUE;
#else
	_BOOL retV;
	_UBYTE pp[PATH_MAX];
	unsigned drNr;

	*pfad = '\0';

	_dos_getdrive(&drNr);

	retV = _getdcwd(drNr, pp, sizeof(pp)) != NULL;
	if (retV)
	{
		if (pp[1] != ':')
		{
			pfad[0] = 'A' + drNr - 1;
			pfad[1] = ':';
			pfad[2] = '\\';
			pfad[3] = '\0';
		}
		strcat(pfad, pp);
		F_Path_Append(pfad, "");
	}
	return retV;
#endif
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL F_Path_Set(CONST _UBYTE *pfad)
{
	_UBYTE  buf[PATH_MAX];

	strBcpy(buf, pfad);
	{
		_UBYTE *end;

		end = strrchr(buf, '\\');
		if (end != NULL)
			if (*(end + 1) == '\0')
				*end = '\0';
	}
	if (*buf == '\0')
		return TRUE;

#if defined(__WIN32__) || defined(__CYGWIN32__)
	FNAME_TO_WINDOWS(buf);
	if (!SetCurrentDirectory(buf))
		return FALSE;
	return TRUE;
#else
	{
		_BOOL retV;

		if (buf[1] == ':')
		{
			unsigned anzDrive;

			_dos_setdrive (buf[0] - 'A' + 1, &anzDrive);
			if (buf[2] == '\0')
				strcat (buf, "\\");
		}

		retV = chdir(buf) == 0;
		return retV;
	}
#endif
}
/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID F_Path_Dirname(_UBYTE *path)
{
	_UBYTE *sp;

	sp = strrchr(path, '\\');
	if (sp == NULL)
		sp = strrchr(path, '/');
	if (sp != NULL)
	{
		if (sp[1] == '\0')
		{
			if (sp == path)
			{
				++sp;
			} else
			{
				*sp = '\0';
				sp = strrchr(path, '\\');
				if (sp == NULL)
					sp = strrchr(path, '/');
			}
		}
	}
	if (sp == NULL)
		sp = path;
	*sp = '\0';
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL F_ERROR F_File_Size(CONST _UBYTE *fname, FILENO fd, _ULONG *size)
{
	FILENO handle = fd;
	
	if (size != NULL)
		*size = 0;
	if (!FILENO_OK(fd))
	{
		if (fname == NULL)
			return GERR_INVAL;
		handle = F_File_Open(fname, FO_RDONLY);
	}
	if (!FILENO_OK(handle))
	{
		return TO_HANDLE(handle);
	}
#ifdef __WIN32__
	if (size != NULL)
	{
		DOSFNO h;
		
		FiBufHandle(handle, &h);
		*size = GetFileSize((HANDLE)h, NULL);
	}
	if (!FILENO_OK(fd))
		F_File_Close(handle);
#else
	if (FILENO_OK(fd))
	{
		if (size != NULL)
		{
			_ULONG pos;
			
			F_File_Tell(fd, &pos);
			F_File_Seek(fd, 0L, SEEK_END);
			F_File_Tell(fd, size);
			F_File_Seek(fd, pos, SEEK_SET);
		}
	} else
	{
		if (size != NULL)
		{
			F_File_Seek(handle, 0L, SEEK_END);
			F_File_Tell(handle, size);
		}
		F_File_Close(handle);
	}
#endif
	return GERR_OK;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL F_ERROR F_File_Read(FILENO fd, _VOID *buf, _LONG wtSize, _LONG *readSize)
{
	_LONG size;
	F_ERROR err;
	DOSFNO handle;
	
	if ((err = FiBufHandle(fd, &handle)) != GERR_OK)
		return err;
	
	size = _hread(handle, buf, wtSize);
	if (size > 0)
		FiBuf[TO_HANDLE(fd)].seekPos += size;
	if (readSize != NULL)
		*readSize = size;
	return err;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL F_ERROR F_File_Write(FILENO fd, CONST _VOID *buf, _LONG wtSize, _LONG *written)
{
	_LONG size;
	F_ERROR err;
	DOSFNO handle;
	
	if ((err = FiBufHandle(fd, &handle)) == GERR_OK)
	{
		size = _hwrite(handle, buf, wtSize);
		if (size != wtSize)
			err = GERR_INCOMPLETE;
	} else
	{
		size = 0;
	}
	if (size > 0)
		FiBuf[TO_HANDLE(fd)].seekPos += size;
	if (written != NULL)
		*written = size;
	return err;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL F_ERROR F_File_Tell(FILENO fd, _ULONG *pos)
{
	DOSFNO handle;
	F_ERROR error;
	
	SavePD();
	if ((error = FiBufHandle(fd, &handle)) != GERR_OK)
	{
		*pos = 0;
		return error;
	}
	RestorePD();
	*pos = FiBuf[TO_HANDLE(fd)].seekPos;
	return GERR_OK;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL F_ERROR F_File_Seek(FILENO fd, _LONG offset, _WORD mode)
{
	_ULONG seekPos;
	_WORD ii;
	DOSFNO handle;
	F_ERROR err;
	
	SavePD();
	ii = 0;
	for (;;)
	{
		err = FiBufHandle(fd, &handle);
		if (err != GERR_OK)
		{
			seekPos = -1;
			break;
		}
#if GUI_VERSION
		seekPos = _llseek(handle, offset, mode);
		if (seekPos == (_ULONG)(HFILE_ERROR))
#else
		seekPos = lseek(handle, offset, mode);
		if (seekPos == (_ULONG)(-1))
#endif
		{
			if (errno == NETERR)
			{
				err = GERR_ACCESS;
				ii++;
				if (ii > 5)
					break;
			} else
			{
				err = sys_errno(errno);
				break;
			}
		} else
		{
			err = GERR_OK;
			break;
		}
#ifdef __WIN32__
		Sleep(200);
#endif
	}
	RestorePD();
	if (err == GERR_OK)
		FiBuf[TO_HANDLE(fd)].seekPos = seekPos;
	return err;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _UBYTE *F_Path_Basename(CONST _UBYTE *filename)
{
	_UBYTE *sp;

	if ((sp = strrchr(filename, '\\')) != NULL)
		sp++;
	else if ((sp = strrchr(filename, '/')) != NULL)
		sp++;
	else
		sp = NO_CONST(filename);
	return sp;
}

