/*****************************************************************************
 * FILE_IO.H
 *****************************************************************************/

#ifndef __FILE_IO_H__
#define __FILE_IO_H__

#ifndef __PORTAB_H__
#  include <portab.h>
#endif
#ifndef __RO_TIME_H__
#  include <ro_time.h>
#endif
#ifndef __PATH_MAX_H__
#  include <path_max.h>
#endif

/* F_File_Open modes: */

#define	FO_RDONLY		0x00		/* read only */
#define	FO_WRONLY		0x01		/* write only */
#define	FO_RDWR			0x02		/* read/write */
#define FO_ACCMODE		0x03		/* used to mask off file access mode */

/* file sharing modes (not POSIX) */
#define FO_DENYRW		0x0010		/* deny both reads and writes */
#define FO_DENYW		0x0020
#define FO_DENYR		0x0030
#define FO_DENYNONE		0x0040		/* don't deny anything */
#define FO_SHMODE		0x0070		/* mask for file sharing mode */

#define	FO_NDELAY		0x0100		/* Non-blocking I/O */
#define	FO_CREAT		0x0200		/* create new file if needed */
#define	FO_TRUNC		0x0400		/* make file 0 length */
#define	FO_EXCL			0x0800		/* error if file exists */
#define	FO_APPEND		0x1000		/* position at EOF */
#define FO_PIPE			0x2000		/* serial pipe     */
#define FO_NOCTTY		0x4000		/* do not open new controlling tty */
#define FO_SYNC			0x8000		/* sync after writes */

/* error codes */

#define GERR_OK              0
#define GERR_INVFN      (  -10)
#define GERR_NOENT      (  -11)
#define GERR_NOPATH     (  -12)
#define GERR_MFILE      (  -13)
#define GERR_ACCESS     (  -14)
#define GERR_BADF       (  -15)
#define GERR_NOMEM      (  -16)
#define GERR_NOEXEC     (  -17)
#define GERR_FAULT      (  -18)
#define GERR_NODEV      (  -19)
#define GERR_BUSY       (  -20)
#define GERR_INVAL      (  -21)
#define GERR_2BIG       (  -22)
#define GERR_NOTRDY     (  -24)
#define GERR_XDEV       (  -25)
#define GERR_NFILE      (  -26)
#define GERR_NOTTY      (  -27)
#define GERR_NOSPC      (  -28)
#define GERR_SPIPE      (  -29)
#define GERR_ROFS       (  -30)
#define GERR_PIPE       (  -32)
#define GERR_DOM        (  -33)
#define GERR_RANGE      (  -34)
#define GERR_EXIST      (  -35)
#define GERR_DEADLOCK   (  -36)
#define GERR_PERM       (  -37)
#define GERR_IO         (  -40)
#define GERR_NXIO       (  -41)
#define GERR_AGAIN      (  -42)
#define GERR_NOTDIR     (  -45)
#define GERR_ISDIR      (  -46)
#define GERR_INTR       (  -70)
#define GERR_LOCKED     (  -72)
#define GERR_INCOMPLETE ( -100)

typedef _WORD F_ERROR;

#define F_STRICT 0

#if F_STRICT
typedef struct { _WORD dummy; } *FILENO;
#define TO_FILENO(handle) ((FILENO)((_LONG)(handle)))
#define TO_HANDLE(fno)    ((_WORD)(_LONG)(fno))
#else
typedef _WORD FILENO;
#define TO_FILENO(handle) (handle)
#define TO_HANDLE(fno)    (fno)
#endif
#define FILENO_OK(fno) (TO_HANDLE(fno) >= 0)

#define NO_FILE TO_FILENO(-1)


CONST _UBYTE *sys_strerror(_WORD);

_WORD   NetzKnotenNummer(_VOID);

_VOID   F_FlushFiles(_VOID);
F_ERROR F_Flush(FILENO fd);

_VOID   F_File_Lock(CONST _UBYTE *pfad);
_VOID   F_File_Unlock(CONST _UBYTE *pfad);

FILENO  F_File_Create(CONST _UBYTE *fname);
F_ERROR F_File_Delete(CONST _UBYTE *fname);
FILENO  F_File_Open(CONST _UBYTE *datei, _WORD mode);
F_ERROR F_File_Close(FILENO fd);
F_ERROR F_File_Seek(FILENO fd, _LONG offset, _WORD mode);
F_ERROR F_File_Tell(FILENO fd, _ULONG *pos);
F_ERROR F_File_Write(FILENO fd, CONST _VOID *buf, _LONG wtSize, _LONG *written);
F_ERROR F_File_Read(FILENO fd, _VOID *buf, _LONG len, _LONG *readSize);
F_ERROR F_File_Size(CONST _UBYTE *fname, FILENO fd, _ULONG *size);
#define F_File_Exists(fname) (F_File_Size(fname, NO_FILE, NULL) == GERR_OK)

SYSTIME F_File_Gettime(_WORD fno);

F_ERROR F_Path_Create(CONST _UBYTE *dir);
FILENO  F_PathFile_Create(CONST _UBYTE *pfad);
_BOOL   F_Path_Delete(CONST _UBYTE *dir);
_BOOL   F_Path_Get(_UBYTE *pfad);
_BOOL   F_Path_Set(CONST _UBYTE *pfad);
_BOOL   F_Path_Get_Temp(_UBYTE *pfad);
_BOOL   F_Path_Set_Temp(CONST _UBYTE *pfad);
F_ERROR F_Path_Rename(CONST _UBYTE *oldname, CONST _UBYTE *newname);

_UBYTE *F_Path_Basename(CONST _UBYTE *path);
_VOID   F_Path_Dirname(_UBYTE *path);

#ifndef DP_CASESENS
#  define	DP_CASESENS	0		/* case sensitive */
#  define	DP_CASECONV	1		/* case always converted */
#  define	DP_CASEINSENS	2	/* case insensitive, preserved */
#endif
#ifndef DP_DOSTRUNC
#  define	DP_NOTRUNC	0		/* long filenames give an error */
#  define	DP_AUTOTRUNC	1	/* long filenames truncated */
#  define	DP_DOSTRUNC	2		/* DOS truncation rules in effect */
#endif

_WORD   F_Path_CaseMode(CONST _UBYTE *path);
_WORD   F_Path_TruncMode(CONST _UBYTE *path);
_VOID   F_Path_Fullname(_UBYTE *filename);
_VOID   F_Path_Get_Work(_UBYTE *path, CONST _UBYTE *name);
_VOID   F_Path_Get_Program(_UBYTE *path, CONST _UBYTE *name);

/*
 * Call func() once per file matching pattern in dir, skipping subdirectories.
 * func returns FALSE to stop early. Answers the number of files visited.
 */
typedef _BOOL (*F_SCAN_FUNC)(CONST _UBYTE *name, _VOID *para);
_WORD   F_Dir_Scan(CONST _UBYTE *dir, CONST _UBYTE *pattern, F_SCAN_FUNC func, _VOID *para);
_VOID   F_Path_Set_Program(CONST _UBYTE *path);
_BOOL   F_Path_Get_User(_UBYTE *path, CONST _UBYTE *name);
_UBYTE *F_Path_Append(_UBYTE *path, CONST _UBYTE *name);
_BOOL   F_Path_Absolute(CONST _UBYTE *path);
_BOOL	F_Path_Equal(CONST _UBYTE *path1, CONST _UBYTE *path2);
F_ERROR F_Path_Settime(CONST _UBYTE *path, SYSTIME newtime);

_VOID F_GetErrorMsg(_WORD nr, _UBYTE *str, size_t strsize);
_VOID F_SetErrorMsg(_WORD nr, CONST _UBYTE *str);
_VOID F_ErrorAlert(_WORD nr);


_VOID err_fopen(CONST _UBYTE *fname);
_VOID err_fread(CONST _UBYTE *fname);
_VOID err_fwrite(CONST _UBYTE *fname);
_VOID err_fcreate(CONST _UBYTE *fname);
_VOID err_rename(CONST _UBYTE *oldname, CONST _UBYTE *newname);
_VOID err_execute(CONST _UBYTE *fname, _WORD errnum);

_VOID EventTimer(_ULONG millisecs);

#endif /* __FILE_IO_H__ */
