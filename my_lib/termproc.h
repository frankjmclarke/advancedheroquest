/*****************************************************************************
 * TERMPROC.H
 *****************************************************************************/

#ifndef __TERMPROC_H__
#define __TERMPROC_H__

#ifndef __PORTAB_H__
#  include <portab.h>
#endif

typedef _VOID (*ApplTermProc)(_VOID);

_VOID InstallTermproc(ApplTermProc proc);
/* Installiert eine Prozedur, welche bei Aufruf von ApplTerm automatisch
 * abgearbeitet wird.  So koennen Systemresourcen auf einfache Art frei-
 * gegeben werden.
 */

_VOID CallTermprocs(_VOID);
/* Ruft die installierten Termprocs auf
 */

#endif /* __TERMPROC_H__ */
