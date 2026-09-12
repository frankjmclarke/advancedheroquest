/*****************************************************************************
 * WINDOWS/OPENWORK.C
 *****************************************************************************/

#include <openwork.h>
#include <w_draw.h>
#include <windows_.h>
#include <termproc.h>
#include <w_mouse.h>
#include <ro_mem.h>
#include <path_max.h>

GLOBAL int GlCmdShow;

LOCAL HINSTANCE GlhInstance;
LOCAL HINSTANCE GlhPrevInstance;

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID GetMaxScreenSize(_WORD *w, _WORD *h)
{
	*w = (_WORD) GetSystemMetrics(SM_CXSCREEN);
	*h = (_WORD) GetSystemMetrics(SM_CYSCREEN);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _WORD GetNumPlanes(_VOID)
{
	HDC hDC;
	_WORD planes;

	hDC = GetDC(GlMainHwnd);
	planes = GetDeviceCaps(hDC, PLANES);
	planes *= GetDeviceCaps(hDC, BITSPIXEL);
	ReleaseDC(GlMainHwnd, hDC);
	return planes;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _WORD GetNumColors(_VOID)
{
	HDC hDC;
	_WORD colors;
	_WORD planes, bits;

	hDC = GetDC(GlMainHwnd);
	colors = GetDeviceCaps(hDC, NUMCOLORS);
	planes = GetDeviceCaps(hDC, PLANES);
	bits = GetDeviceCaps(hDC, BITSPIXEL);
	if ((planes * bits) >= 16)
		colors = 32766;		/* more than we need */
	ReleaseDC(GlMainHwnd, hDC);
	return colors;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID GetScreenSize(_WORD *breite, _WORD *hoehe)
{
	*breite = (_WORD) GetSystemMetrics(SM_CXSCREEN);
	*hoehe = (_WORD) GetSystemMetrics(SM_CYSCREEN);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL OpenWork(_VOID)
{
#if SPEC_DEBUG
	{
		char *env;
		
		env = getenv("DEBUG");
		if (env)
			is_DEBUGGING = (*env >= '0' && *env <= '9') ? (*env - '0') : 1;
		else
			is_DEBUGGING = FALSE;
	}
#endif

	if (!GetPrevInstance())
		register_classes(TRUE);

#if 0
	if (!W_InitFont(DEV_SCREEN, NO_DC))
		return FALSE;
	if (!w_init_brush())
		return FALSE;
	
	if (!InitCursor())
		return FALSE;
#endif

	SetMouse(MOUSE_RESET);
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID CloseWork(_VOID)
{
	CallTermprocs();
}

/*** ---------------------------------------------------------------------- ***/

#ifdef XXXXXXX
#include <excpt.h>

LOCAL _VOID unexpectedFunc(_VOID)
{
	ErrorOut(EO_ERROR, "unexpectedFunc");
	terminate();
}

#endif

/*** ---------------------------------------------------------------------- ***/
#define is_space(c) ((c) == ' '|| (c) == '\t')

LOCAL _BOOL parseargs(CONST _UBYTE *cmdln, _WORD *p_argc, CONST _UBYTE ***p_argv, _BOOL skip_first)
{
	_LONG count;
	CONST _UBYTE *from;
	_UBYTE *to;
	_UBYTE **arg;
	CONST _UBYTE *arg0 = NULL, *arg1 = NULL;
	_UBYTE quote;
	
	count = 0;

	if (cmdln == NULL || !skip_first)
	{
		_UBYTE path[PATH_MAX + 1];
		
		count = GetModuleFileName(GetInstance(), path, sizeof(path)-1) + 1;
		path[sizeof(path)-1] = '\0';
		arg0 = STRDUP(path, "parseargs: argv0");
	}

	if (skip_first)
		*p_argc = 0;
	else
		*p_argc = 1;
	if (cmdln != NULL)
	{
		from = cmdln;
		while (is_space(*from))
			from++;
		if (arg0 == NULL)
			arg0 = from;
		arg1 = from;

		if (*from != '\0')
		{
			quote = '\0';
			for (;;)
			{
				count++;
				if ((quote == '\0' && is_space(*from)) || *from == '\0')
				{
					while (is_space(*from))
						from++;
					if (skip_first)
						if (*p_argc == 0)
							arg1 = from;
					(*p_argc)++;
					if (*from == '\0')
						break;
				} else
				{
					if (*from == '\'' || *from == '"')
					{
						if (*from == quote)
							quote = '\0';
						else if (quote == '\0')
							quote = *from;
					}
					from++;
				}
			}
		}
	}

	count = ((count + sizeof(CONST _UBYTE *) - 1) / sizeof(CONST _UBYTE *)) * sizeof(CONST _UBYTE *);
	if (*p_argc < 2 || (arg = MALLOC(count + (*p_argc + 1) * sizeof(CONST _UBYTE *), "parseargs")) == NULL)
	{
		static CONST _UBYTE *argv2[2];

		argv2[0] = arg0;
		argv2[1] = NULL;
		*p_argc = 1;
		*p_argv = argv2;
		return FALSE;
	}

	*p_argv = (CONST _UBYTE **) arg;
	to = (_UBYTE *) (arg + *p_argc + 1);

	from = arg0;
	*arg++ = to;
	while (!is_space(*from) && *from != '\0')
		*to++ = *from++;
	*to++ = '\0';
	if (cmdln != NULL)
	{
		from = arg1;
		*arg = to;
		if (*from != '\0')
		{
			quote = '\0';
			for (;;)
			{
				if ((quote == '\0' && is_space(*from)) || *from == '\0')
				{
					while (is_space(*from))
						from++;
					*to++ = '\0';
					arg++;
					*arg = to;
					if (*from == '\0')
						break;
				} else
				{
					if (*from == '\'' || *from == '"')
					{
						if (*from == quote)
							quote = '\0';
						else if (quote == '\0')
							quote = *from;
						else
							*to++ = *from;
						from++;
					} else
					{
						*to++ = *from++;
					}
				}
			}
		}
	}
	*arg = NULL;

	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

#ifndef __WIN32__
GLOBAL HINSTANCE GetInstance(_VOID)
{
	return GlhInstance;
}
#endif

/*** ---------------------------------------------------------------------- ***/

GLOBAL HINSTANCE GetPrevInstance(_VOID)
{
	return GlhPrevInstance;
}

/*** ---------------------------------------------------------------------- ***/

#ifdef __linux__
#  define WinMain main
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int cmdShow)
{
	_WORD argc;
	CONST _UBYTE **argv;
	_BOOL got_mem;
	_WORD exitCode;
	
	GlhInstance = hInstance;
	GlhPrevInstance = hPrevInstance;

#ifdef __WIN32__
	{
		STARTUPINFO si;
		
		UNUSED(cmdShow);
		si.cb = sizeof(si);
		GetStartupInfo(&si);
		if (si.dwFlags & STARTF_USESHOWWINDOW)
			if (si.wShowWindow == SW_SHOWMAXIMIZED)
				GlCmdShow = SW_SHOWMAXIMIZED;
			else
				GlCmdShow = SW_SHOWDEFAULT;
		else
			GlCmdShow = SW_SHOWDEFAULT;
	}
#else
	if (cmdShow == SW_SHOWMAXIMIZED || cmdShow == SW_SHOWMINIMIZED)
		GlCmdShow = cmdShow;
	else
		GlCmdShow = SW_SHOW;
#endif

	/* set_unexpected(unexpectedFunc); */

#if 0
	/* Programm nur einmal starten, sonst aktivieren */
	if (GetPrevInstance())		/* wird mit WIN3.1 nicht erreicht, siehe *.def Datei */
		return FALSE;
#endif

	mem_test_start();

	got_mem = parseargs(lpszCmdLine, &argc, &argv, FALSE);

	if (OpenWork())
	{
		/* NetzKnotenNummer(); */
		exitCode = WindFormMain(argc, argv);
	} else
	{
		exitCode = -1;
	}

	CloseWork();

	if (got_mem)
		SFREE(NO_CONST(argv));
	else
		SFREE(NO_CONST(argv[0]));
	
	mem_test_end();

#if 0
	{
		MSG msg;
		
		while (GetMessage(&msg, NO_WINDOW, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
#endif
	return exitCode;
}
