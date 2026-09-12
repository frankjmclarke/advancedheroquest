/*****************************************************************************
*
* PORTAB.H
*
* Use of this file may make your code compatible with all C compilers
* listed.
*
*
* Folgende Konstanten werden z.Z. benutzt
*
* WINDOWS			Windows 3.1/Win32s Version
* ATARI             Atari (TOS) Version
* XWINDOWS			X Window System
*
* STDC_HEADERS
*	   wenn ANSI-Header-Dateien (stdlib.h, stddef.h, string.h und stdarg.h)
*	   verfuegbar sind.
* HAVE_xxxx_H
*	   wenn die entsprechende Header-Datei verfuegbar ist
* HAVE_xxxx
*	   wenn die entsprechende Funktion verfuegbar ist
* HAVE_DIRENT_H
*	   wenn dirent.h und opendir() etc. verfuegbar sind
*
* FAR
*	   Schluesselwort far
* _HUGE
*	   Schluesselwort huge
* _CDECL
*	   Schluesselwort cdecl
* CONST
*	   Schluesselwort const
* VOLATILE
*	   Schluesselwort volatile
* GLOBAL
*	   fuer globale Daten und Funktionen
* LOCAL
*	   fuer lokale Daten und Funktionen
* RLOCAL
*	   fuer lokale Daten und Funktionen >64 Byte
*	   (normalerweise nur fuer Windows/16Bit, um zu vermeiden dass das
*		Datensegment groesser wird als 64K)
*
* _BOOL
*	   boolean value (TRUE/FALSE)
* _BYTE
*	   signed byte (8 bits)
* _UBYTE
*	   unsigned byte (8 bits)
* _WORD
*	   signed word (16 bits)
* _UWORD
*	   unsigned word (16 bits)
* _LONG
*	   signed long (32 bits)
* _ULONG
*	   unsigned long (32 bits)
* _LLONG
*	   signed long long (64 bits, defined in longlong.h)
* _ULLONG
*	   unsigned long long (64 bits, defined int longlong.h)
* _FLOAT
*	   single precision floating point value
* _DOUBLE
*	   double precision floating point value
*
* PATH_MAX
*	   maximale Laenge von kompletten Pfadangaben
*
* RSC_INLINE
*	   wenn alle RSH-Files eincompiliert werden sollen
*****************************************************************************/

#ifndef __PORTAB_H__
#define __PORTAB_H__

/* ANSI Compiler ist schon Voraussetzung */
#ifndef __STDC__
#  ifndef _AIX /* complains about __STDC__ */
#	 define __STDC__ 1
#  endif
#else
#  if !__STDC__
you loose
#  endif
#endif

#include <stdio.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


/*****************************************************************************/
/* Windows 3.1 / Win32s / Windows 9x / Windows NT / MSDOS					 */
/*****************************************************************************/
#ifdef _MSC_VER
#define __WIN32__
#ifndef WIN32
#define WIN32
#endif
#ifndef __MSDOS__
#define WINDOWS 1
#endif
#endif

#ifdef _Windows
#  define WINDOWS 1
#endif
#ifdef __CYGWIN32__
#  ifndef __WIN32__
#    define __WIN32__ 1
#  endif
#  ifndef __FLAT__
#    define __FLAT__ 1
#  endif
#endif
#ifdef __MINGW32__
#  ifndef __WIN32__
#    define __WIN32__ 1
#  endif
#  ifndef __FLAT__
#    define __FLAT__ 1
#  endif
#  define WINDOWS 1
#endif

#if defined(WINDOWS) || defined(__MSDOS__) || defined(__WIN32__)

#define HOST_BYTE_ORDER BYTE_ORDER_LITTLE_ENDIAN

#define HAVE_STRING_H 1
#define STDC_HEADERS 1
#define HAVE_DIRENT_H 1

#ifdef __BORLANDC__
#define HAVE_FCNTL_H 1
#define HAVE_IO_H 1
#define HAVE_ALLOC_H 1
#define HAVE_STRSTR
#define HAVE_LIMITS_H 1
#define HAVE_SHELLAPI_H 1
#define HAVE_MMSYSTEM_H 1
#define HAVE_DOS_H 1
#endif

#ifdef _MSC_VER
#define HAVE_FCNTL_H 1
#define HAVE_IO_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_STRSTR
#define HAVE_SHELLAPI_H 1
#define HAVE_MMSYSTEM_H 1
#define HAVE_DOS_H 1
#define _CDECL
#define fileno _fileno
#define read   _read
#define open   _open
#define close  _close
#define write  _write
#define creat  _creat
#define lseek _lseek
#define isatty _isatty

#pragma warning(disable:4018) /* signed/unsigned comparisons */
#pragma warning(disable:4133) /* signed/unsigned char */
#pragma warning(disable:4761) /* integral size mismatch */
#pragma warning(disable:4068) /* unknown pragma */
#pragma warning(disable:4244) /* possible loss of data */
#pragma warning(disable:4305) /* truncation from long to short */
#endif /* _MSC_VER */

#ifdef __GNUC__
#define HAVE_FCNTL_H 1
#define HAVE_IO_H 1
#ifdef __MINGW32__
#define HAVE_ALLOC_H 1
#define HAVE_DOS_H 1
#endif
#define HAVE_STRSTR
#define HAVE_LIMITS_H 1
#undef HAVE_SHELLAPI_H /* has shellapi.h, but is broken */
#define HAVE_WINBASE_H 1
#define HAVE_UNISTD_H 1
#endif


#ifdef __WIN32__

#ifndef __GNUC__
#define FAR             far
#ifndef _CDECL
#define _CDECL _cdecl
#endif
#endif
/* __cdecl */
/* #define FAR */
#define _HUGE
#define EXP_PROC
#define EXP_PTR

#ifdef _MSC_VER
#define EXPORT
#define IMPORT
#endif

#ifdef __BORLANDC__
#define EXPORT __stdcall _export
#define IMPORT __stdcall _import
#endif

#define _WORD signed short
#define _UWORD unsigned short

#define RLOCAL LOCAL

#ifdef __BORLANDC__
#pragma option -w-sig
#define HAVE_SHLOBJ_H 1
#define HAVE_WINNLS_H 1
#define HAVE_WINBASE_H 1
#endif

#ifdef _MSC_VER
#define HAVE_SHLOBJ_H 1
#define HAVE_WINNLS_H 1
#define HAVE_WINBASE_H 1
#endif

#define HAVE_UNICODE

#else /* !__WIN32__ */

#ifdef __BORLANDC__
#define _CDECL _cdecl
#define FAR _far
#define _HUGE huge
#define EXP_PROC _far
#define EXP_PTR  _far
#define EXPORT _far _pascal _export
#define IMPORT _far _pascal

#define _WORD signed int
#define _UWORD unsigned int

#ifndef RSC_INLINE
#  define RSC_INLINE 0
#endif

#ifdef WINDOWS
#  pragma option -zE_FARDATA
#endif
#endif

#endif /* __WIN32__ */

/* #include <_defs.h> */
/* #include <_nfile.h> */
/* #include <locale.h> */

#ifdef __GNUC__
#include <path_max.h>
#else
#define PATH_MAX		128
#endif
#define ALL_FILE_MASK "*.*"

/* RSC-Dateien aus Windows-Resource laden */
#define USE_WINDOWS_RESOURCE 1

#define UNUSED(x) (void)(x)

#endif /* WINDOWS || __MSDOS__ || __WIN32__ */


/*****************************************************************************/
/* OS/2 Warp																 */
/*****************************************************************************/

#ifdef __OS2__

#define OS2 1

#if 0
#pragma info(all)
#endif
#pragma info(cmp)
#pragma info(cnd)
#pragma info(cns)
#pragma info(cnv)
#pragma info(cpy)
#pragma info(noeff)
#pragma info(enu)
#pragma info(noext)
#pragma info(gnr)
#pragma info(nogot)
#pragma info(noini)
#pragma info(lan)
#pragma info(obs)
#pragma info(par)
#pragma info(nopor)
#pragma info(noppc)
#pragma info(noppt)
#pragma info(pro)
#pragma info(rea)
#pragma info(ret)
#pragma info(notrd)
#pragma info(tru)
#pragma info(und)
#pragma info(nouni)
#pragma info(use)
#pragma info(vft)
#pragma info(dcl)
#pragma info(ord)


#define HOST_BYTE_ORDER BYTE_ORDER_LITTLE_ENDIAN

#define ALL_FILE_MASK "*.*"

#define HAVE_STRING_H 1
#define STDC_HEADERS 1
#define HAVE_FCNTL_H 1
#define HAVE_IO_H 1
#define HAVE_DIRENT_H 1
#define HAVE_STRSTR
#define HAVE_LIMITS_H

#ifndef _CDECL
/* #define _CDECL _Cdecl */
#endif

#define UNUSED(x) x = x

#endif /* __OS2__ */


/*****************************************************************************/
/* Atari TOS/GEM															 */
/*****************************************************************************/
#if defined(__TOS__) || defined(atarist)

#define HAVE_STRING_H
#define STDC_HEADERS
#define HAVE_STRSTR

#define ATARI	   1			/* fuer noch zu erledigende Sachen */

#define HOST_BYTE_ORDER BYTE_ORDER_BIG_ENDIAN

/* #define _(params) params */

#define ALL_FILE_MASK "*.*"

#ifdef __PUREC__
#define PATH_MAX		128
#define _WORD	  signed int	/* A machine dependent int	   */
#define _UWORD	  unsigned int	/* A machine dependent uint    */
#define _CDECL	   cdecl
#define NEEDS_DUMMY_STRUCT
#endif

#ifdef __SOZOBONX__
#define CDECL    
#define _CDECL	 
#define __CDECL  
#endif

#ifdef __GNUC__
#define _WORD	  signed short
#define _UWORD	  unsigned short
#define HAVE_UNISTD_H 1
#define HAVE_FCNTL_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_DIRENT_H 1
#define HAVE_LIMITS_H 1
#endif

#endif /* __TOS__ */


/*****************************************************************************/
/* X Window System / Unix generell											 */
/*****************************************************************************/
#ifdef _AIX
#  define UNIX 1
#endif
#ifdef unix
#  define XWINDOWS 1
#endif
#ifdef __unix__
#  define XWINDOWS 1
#endif
#ifdef __unix
#  define XWINDOWS 1
#endif
#ifdef UNIX
#  define XWINDOWS 1
#endif
#ifdef hpux
#  define XWINDOWS 1
#endif
#ifdef XWINDOWS
#ifndef __WIN32__

#ifdef HAVE_LIMITS_H
#  include <limits.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#define ALL_FILE_MASK "*"

#endif
#endif /* XWINDOWS */


/*****************************************************************************/
/* DEFAULTS                                                                  */
/*****************************************************************************/

#ifdef STDC_HEADERS
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef STDC_HEADERS
#include <string.h>
#else
#include <strings.h>
#define strchr(s,c) index(s, c)
#define strrchr(s,c) rindex(s, c)
#endif
#endif
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stddef.h>
#endif
#include <var_args.h>


#ifndef EXPORT
#define EXPORT
#endif

#ifndef _HUGE
#define _HUGE
#endif
#ifndef FAR
#define FAR
#endif

#ifndef _CDECL
#define _CDECL /* */
#endif

#ifndef EXP_PROC
#define EXP_PROC
#endif
#ifndef EXP_PTR
#define EXP_PTR
#endif

#ifndef _VOID
#define _VOID void
#endif

#ifndef _BOOL
#  ifdef LINT
#	 define _BOOL bool
#  else
#	 define _BOOL int			/* 2 valued (true/false)	   */
#  endif
#endif

#ifndef _BYTE
#define _BYTE	signed char
#endif
#ifndef _UBYTE
#define _UBYTE	unsigned char	/* Unsigned byte			   */
#endif

#ifdef UNICODE
#  ifndef HAVE_UNICODE
#    undef UNICODE
#  endif
#endif

#ifndef _UCHAR
#  ifdef UNICODE
#    define _UCHAR unsigned short
#    define _CHAR unsigned short
#    define EOS 0x0000
#  else
#    define _UCHAR unsigned char
#    define _CHAR char
#    define EOS '\0'
#  endif
#endif
#ifndef _WCHAR
#  define _WCHAR unsigned short
#endif

#ifndef _WORD
#define _WORD	signed short	/* signed word (16 bits)	   */
#endif
#ifndef _UWORD
#define _UWORD	unsigned short	/* unsigned word (16 bits)	   */
#endif

#ifndef _LONG
#define _LONG	signed long     /* Signed long (32 bits)	   */
#endif
#ifndef _ULONG
#define _ULONG	unsigned long	/* Unsigned long			   */
#endif

#ifndef _FLOAT
#define _FLOAT	float			/* Single precision float	   */
#endif
#ifndef _DOUBLE
#define _DOUBLE double			/* Double precision float	   */
#endif

#ifndef EXTERN
#define EXTERN	extern			/* External variable		   */
#endif
#ifndef LOCAL
#define LOCAL	static			/* Local to module			   */
#endif
#ifndef RLOCAL
#define RLOCAL LOCAL FAR
#endif
#ifndef GLOBAL
#define GLOBAL					/* Global variable			   */
#endif

#ifndef CONST
#define CONST	 const
#endif
#ifndef VOLATILE
#define VOLATILE volatile
#endif

#ifndef INLINE
#  ifdef __GNUC__
#	 define INLINE __inline__
#  endif
#endif
#ifndef INLINE
#  define INLINE /**/
#endif


#ifndef RSC_INLINE
#  define RSC_INLINE 1
#endif

#ifndef ALL_FILE_MASK
#  define ALL_FILE_MASK "*"
#endif

/* global object for library, not intended to be used by application */
#define LIB_GLOBAL GLOBAL
/* global object for os depended stuff */
#define OS_GLOBAL GLOBAL


/*****************************************************************************/
/* MISCELLANEOUS DEFINITIONS												 */
/*****************************************************************************/

#define OffsetOf(type,ident)	 ((_ULONG)&(((type *)0)->ident))
#define SizeOf(type,ident)		 (_UWORD)sizeof(((type *)0)->ident)
#define NelemOf(type)			 (sizeof(type)/sizeof(type[0]))

#undef FALSE
#undef TRUE
#define FALSE	0				/* Function FALSE value        */
#define TRUE	1				/* Function TRUE  value        */

#ifndef NULL
#define NULL	( ( _VOID * ) 0L )		/* Null pointer value			  */
#endif

#ifndef UNUSED
#  define UNUSED(x) if(x!=0){}						 /* indicate unused variable */
#endif

#ifndef NO_CONST
#  ifdef __GNUC__
#	 define NO_CONST(p) __extension__({ union { CONST _VOID *cs; _VOID *s; } x; x.cs = p; x.s; })
#  else
#	 define NO_CONST(p) ((_VOID *)(p))
#  endif
#endif

/* (void *)0 als Funktionspointer, hier ohne cast,
   da die meisten Compiler dann Warnungen generieren */
#define FUNK_NULL 0L

#ifndef EOF
#define EOF     (-1)			/* EOF value				   */
#endif

#if __GNUC__ >= 2
#  ifndef LINT
#	 define __FORMAT_ATTRIBUTE__(x) __attribute__ (x)
#  endif
#endif
#ifndef __GNUC__
#	define __attribute__(x) /**/
#else
#  if (__GNUC__ >= 2) && (__GNUC__ >= 3 || __GNUC_MINOR__ >= 7)
#	 ifndef LINT
#      ifndef __cplusplus
#        ifndef __osf__ /* doesn't work */
#          define PACKED __attribute__((packed))
#        endif
#      endif
#	 endif
#  endif
#endif
#ifndef __FORMAT_ATTRIBUTE__
#  define __FORMAT_ATTRIBUTE__(x) /**/
#endif

#ifndef PACKED
#  define PACKED /**/
#endif

#ifdef c_plusplus
#  ifndef __cplusplus
#    define __cplusplus
#  endif
#endif

#ifdef __cplusplus
#  define EXTERN_C_BEG extern "C" {
#  define EXTERN_C_END }
#  define EXTERN_C extern "C"
#else
#  define EXTERN_C_BEG
#  define EXTERN_C_END
#  define EXTERN_C extern
#endif

#ifndef DUMMY_STRUCT
#  ifdef NEEDS_DUMMY_STRUCT
#    define DUMMY_STRUCT(name) struct name { int dummy; }
#  else
#    define DUMMY_STRUCT(name) struct name
#  endif
#endif

#define T(str) ((CONST _UCHAR *)(str))
#define TC(str) ((CONST _CHAR *)(str))

#ifdef UNICODE
#define _T(x) L ## x
#else
#define _T(x) x
#endif

extern _UBYTE ProgramName[];
_WORD WindFormMain(_WORD argc, CONST _UBYTE **argv);

#endif /* __PORTAB_H__ */
