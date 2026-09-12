/*****************************************************************************
 * GENERIC/TERMPROC.C
 *****************************************************************************/

#include <termproc.h>
#include <ro_mem.h>


typedef struct _terminfo TermInfo;
struct _terminfo {
	ApplTermProc proc;
	TermInfo *next;
};

LOCAL TermInfo *TermProc;

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID InstallTermproc(ApplTermProc proc)
{
	TermInfo *q;
	
	for (q = TermProc; q != NULL; q = q->next)
		if (q->proc == proc)
			return;
	q = NEW(TermInfo, "InstallTermproc");
	if (q != NULL)
	{
		q->proc = proc;
		q->next = TermProc;
		TermProc = q;
	}
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID CallTermprocs(_VOID)
{
	TermInfo *p, *next;
	
	/* Installierte TermProcedures abarbeiten */
	p = TermProc;
	TermProc = NULL;
	while (p != NULL)
	{
		(*p->proc)();
		next = p->next;
		OFREE(p);
		p = next;
	}
}
