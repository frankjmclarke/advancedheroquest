/*****************************************************************************
 * WINDOWS/RO_HELP.C
 *****************************************************************************/

#include <ro_help.h>
#include <windows_.h>
#include <ro_mem.h>
#include <string.h>
#include <openwork.h>
#include <termproc.h>


LOCAL CONST _UBYTE *gl_help_name;

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL help_show(CONST _UBYTE *entry)
{
	if (entry != NULL)
		if (*entry == '$')
			entry++;
	if (help_ok())
	{
		_UBYTE *my_help_name;

		if ((my_help_name = MALLOC(strlen(gl_help_name) + 7, "help_init")) == NULL)
			return FALSE;
		strcpy(my_help_name, gl_help_name);
		strcat(my_help_name, ".hlp");
		if (entry == NULL)
		{
			WinHelp(GlMainHwnd, my_help_name, HELP_PARTIALKEY, (DWORD) "");
		} else if (strcmp(entry, "Contents") == 0)
		{
			entry = "Contents";
			WinHelp(GlMainHwnd, my_help_name, HELP_KEY, (DWORD) entry);
		} else if (strcmp(entry, "Index") == 0)
		{
			WinHelp(GlMainHwnd, my_help_name, HELP_PARTIALKEY, (DWORD) "");
		} else if (strcmp(entry, "Help") == 0)
		{
			WinHelp(GlMainHwnd, my_help_name, HELP_HELPONHELP, 0);
		} else
		{
			WinHelp(GlMainHwnd, my_help_name, HELP_KEY, (DWORD) entry);
		}
		SFREE(my_help_name);
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID help_exit(_VOID)
{
	gl_help_name = NULL;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL help_init(CONST _UBYTE *help_name)
{
	InstallTermproc(help_exit);
	if (help_name != NULL)
		gl_help_name = help_name;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL help_ok(_VOID)
{
	return gl_help_name != NULL;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID help_index(_VOID)
{
	help_show("Index");
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID help_contents(_VOID)
{
	help_show("Contents");
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID help_using_help(_VOID)
{
	WinHelp(GlMainHwnd, NULL, HELP_HELPONHELP, 0);
}
