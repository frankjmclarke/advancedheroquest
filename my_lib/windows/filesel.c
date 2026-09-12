/*****************************************************************************
 * WINDOWS/FILESEL.C
 *****************************************************************************/

#include <filesel.h>
#include <windows_.h>
#include <commdlg.h>
#include <string.h>
#include <path_max.h>
#include <routine.h>
#include <file_io.h>

#ifndef OFN_NONETWORKBUTTON
#define OFN_NONETWORKBUTTON			(0x00020000UL)
#endif
#ifndef OFN_NOLONGNAMES
#define OFN_NOLONGNAMES				(0x00040000UL)
#endif
#ifndef OFN_EXPLORER
#define OFN_EXPLORER				(0x00080000UL)
#endif
#ifndef OFN_NODEREFERENCELINKS
#define OFN_NODEREFERENCELINKS		(0x00100000UL)
#endif
#ifndef OFN_LONGNAMES
#define OFN_LONGNAMES				(0x00200000UL)
#endif

#ifdef __MINGW32__
#define STDAPICALLTYPE          __stdcall
#define STDAPI_(type)           EXTERN_C type STDAPICALLTYPE
#define WINOLEAPI_(type) STDAPI_(type)
WINOLEAPI_(LPVOID) CoTaskMemAlloc(ULONG cb);
WINOLEAPI_(LPVOID) CoTaskMemRealloc(LPVOID pv, ULONG cb);
WINOLEAPI_(void)   CoTaskMemFree(LPVOID pv);
#endif

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL _BOOL FileSelect(_UBYTE *path, CONST _UBYTE *suffix, CONST _UBYTE *title)
{
	HWND    hWnd = GetActiveWindow();	/* GlMainHwnd; */
	OPENFILENAME ofnTemp;
	_UBYTE  sufNames[256];
	_UBYTE  dirname[PATH_MAX];
	_UBYTE  titlebuf[80 + 1];
	_UBYTE  szName[PATH_MAX];
	_BOOL retV;
	
	sprintf(sufNames, "Default   (%s)|%s|Text  (*.txt)|*.txt|All    (*.*)|*.*||", suffix, suffix);
	
	{
		_WORD   ii;

		for (ii = 0; ii < sizeof(sufNames); ii++)
		{
			if (sufNames[ii] == '|')
				sufNames[ii] = '\0';
		}
	}
	strBcpy(dirname, path);
	FNAME_TO_WINDOWS(dirname);
	strBcpy(szName, F_Path_Basename(dirname));
	if (*szName != '\0')
		F_Path_Dirname(dirname);

	ofnTemp.lStructSize = sizeof(OPENFILENAME);
	ofnTemp.hwndOwner = hWnd;
	/* An invalid hWnd causes non - modality */
	ofnTemp.hInstance = 0;
	ofnTemp.lpstrFilter = (LPSTR) sufNames;
	/* See previous note concerning string */
	ofnTemp.lpstrCustomFilter = NULL;
	ofnTemp.nMaxCustFilter = 0;
	ofnTemp.nFilterIndex = 1;
	ofnTemp.lpstrFile = (LPSTR) szName;
	/* Stores the result in this variable */
	ofnTemp.nMaxFile = sizeof(szName);
	ofnTemp.lpstrFileTitle = NULL;
	ofnTemp.nMaxFileTitle = 0;
	ofnTemp.lpstrInitialDir = dirname;
	if (title == NULL || *title == '\0')
	{
		title = "File selector:";
	}
	strBcpy(titlebuf, title);
	ofnTemp.lpstrTitle = titlebuf;
	/* Title for dialog */
	ofnTemp.Flags = /* OFN_FILEMUSTEXIST | */ OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
	ofnTemp.nFileOffset = 0;
	ofnTemp.nFileExtension = 0;
	ofnTemp.lpstrDefExt = NULL;
	ofnTemp.lCustData = (LPARAM) NULL;
	ofnTemp.lpfnHook = NULL;
	ofnTemp.lpTemplateName = NULL;
	/* If the call to GetOpenFileName() fails you can call CommDlgExtendedError() to retrieve the type of error that
	   occured. */
	retV = GetOpenFileName(&ofnTemp);
	if (retV == FALSE)
	{
		(void) CommDlgExtendedError();
		return FALSE;
	}
	strcpy(path, szName);
	FNAME_FROM_WINDOWS(path);
	return TRUE;
}
