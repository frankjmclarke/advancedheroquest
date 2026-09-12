/*****************************************************************************
 * WINDOWS/PROFILE.C
 *****************************************************************************/

#include <profile.h>
#include <stdio.h>
#include <string.h>
#include <windows_.h>
#include <ro_mem.h>
#include <termproc.h>
#include <routine.h>
#include <path_max.h>
#include <debug.h>
#ifndef __WIN32__
#include <shellapi.h>
#include <wownt16.h>
#define ERROR_INVALID_FUNCTION 1
#endif
#define NO_KEY ((HKEY)0)

LOCAL _UBYTE *profile_name;
LOCAL HKEY profile_key;

typedef enum keytype {
	T_BOOL,
	T_WORD,
	T_UWORD,
	T_LONG,
	T_ULONG,
	T_STR
} KEYTYPE;

#ifndef HKEY_CURRENT_USER
#  define HKEY_CURRENT_USER ((HKEY) 0x80000001L)
#endif
#ifndef REG_DWORD
#  define REG_DWORD 4
#endif

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

#ifndef __WIN32__

extern DWORD WINAPI CallProc32W(DWORD p1, DWORD p2, DWORD p3, DWORD p4, DWORD p5, DWORD p6, DWORD proc_addr, DWORD addr_conv, DWORD nparams);

LOCAL LONG RegSetValueEx(HKEY hkey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType, CONST BYTE *lpData, DWORD cbData)
{
	DWORD shell32;
	HINSTANCE kernel;
	UINT old_error_mode;
	FARPROC loadlibraryex32w;
	DWORD addr;
	DWORD key;
	DWORD error;
	
	old_error_mode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
		
	shell32 = (DWORD) -1;
	kernel = LoadLibrary("KERNEL");
	if (kernel >= HINSTANCE_ERROR)
	{
		loadlibraryex32w = GetProcAddress(kernel, "LoadLibraryEx32W");
		if (loadlibraryex32w != FUNK_NULL)
		{
			shell32 = (*((DWORD (WINAPI *)(LPCSTR, DWORD, DWORD)) loadlibraryex32w))("SHELL32", NULL, 0l);
			if (shell32 == 0)
				shell32 = (DWORD) -1;
		}
		FreeLibrary(kernel);
	}

	SetErrorMode(old_error_mode);

	if (shell32 == (DWORD)-1)
		return ERROR_INVALID_FUNCTION;
	addr = GetProcAddress32W(shell32, "RegSetValueExA");
	if (addr == 0)
		return ERROR_INVALID_FUNCTION;
	key = DWORD_FROM_HANDLE(hkey);
	error = CallProc32W(key, (DWORD)lpValueName, (DWORD)Reserved, (DWORD)dwType, (DWORD)lpData, (DWORD)cbData, addr, 0x12, 6);
	FreeLibrary32W(shell32);
	return error;
}
#endif /* __WIN32__ */

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID WriteKey(CONST _UBYTE *section, CONST _UBYTE *key, CONST _VOID *pval, KEYTYPE type)
{
	_UBYTE *subkeyname;
	HKEY subkey;
	LONG error;
	DWORD val;
	
	if (profile_key != NO_KEY)
	{
		subkeyname = MALLOC(strlen(section) + 1 + strlen(key) + 1, "WriteKey");
		if (subkeyname != NULL)
		{
			strcpy(subkeyname, section);
			/* strcat(subkeyname, "\\");
			strcat(subkeyname, key); */
			error = RegCreateKey(profile_key, subkeyname, &subkey);
			if (error == ERROR_SUCCESS)
			{
				switch (type)
				{
				case T_BOOL:
					val = *((_BOOL *)pval);
					error = RegSetValueEx(subkey, key, 0, REG_DWORD, (CONST BYTE *)&val, sizeof(val));
					break;
				case T_WORD:
					val = (*(_WORD *)pval);
					error = RegSetValueEx(subkey, key, 0, REG_DWORD, (CONST BYTE *)&val, sizeof(val));
					break;
				case T_UWORD:
					val = (*(_UWORD *)pval);
					error = RegSetValueEx(subkey, key, 0, REG_DWORD, (CONST BYTE *)&val, sizeof(val));
					break;
				case T_LONG:
					val = (*(_LONG *)pval);
					error = RegSetValueEx(subkey, key, 0, REG_DWORD, (CONST BYTE *)&val, sizeof(val));
					break;
				case T_ULONG:
					val = (*(_ULONG *)pval);
					error = RegSetValueEx(subkey, key, 0, REG_DWORD, (CONST BYTE *)&val, sizeof(val));
					break;
				case T_STR:
					error = RegSetValueEx(subkey, key, 0, REG_SZ, (CONST BYTE *)pval, strlen(pval));
					break;
				}
				RegCloseKey(subkey);
			}
			SFREE(subkeyname);
#if 0
			if (error != ERROR_SUCCESS)
				ErrorOut(EO_DEBUG, "RegKey error: %ld", error);
#else
			UNUSED(error);
#endif
		}
	}
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID DeleteKey(CONST _UBYTE *section, CONST _UBYTE *key)
{
	_UBYTE *subkeyname;
	LONG error;
	
	if (profile_key != NO_KEY)
	{
		subkeyname = MALLOC(strlen(section) + 1 + strlen(key) + 1, "DeleteKey");
		if (subkeyname != NULL)
		{
			strcpy(subkeyname, section);
			if (*key != '\0')
			{
				strcat(subkeyname, "\\");
				strcat(subkeyname, key);
			}
			error = RegDeleteKey(profile_key, subkeyname);
			SFREE(subkeyname);
#if 0
			if (error != ERROR_SUCCESS)
				ErrorOut(EO_DEBUG, "RegKey error: %ld", error);
#else
			UNUSED(error);
#endif
		}
	}
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL _VOID Profile_WriteString(CONST _UBYTE *section, CONST _UBYTE *key, CONST _UBYTE *str)
{
	if (profile_name != NULL)
		WritePrivateProfileString(section, key, str, profile_name);
	WriteKey(section, key, str, T_STR);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Profile_WriteInt(CONST _UBYTE *section, CONST _UBYTE *key, _WORD val)
{
	_UBYTE buf[20];
	
	sprintf(buf, "%d", val);
	if (profile_name != NULL)
		WritePrivateProfileString(section, key, buf, profile_name);
	WriteKey(section, key, &val, T_WORD);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Profile_WriteCard(CONST _UBYTE *section, CONST _UBYTE *key, _UWORD val)
{
	_UBYTE buf[20];
	
	sprintf(buf, "%u", val);
	if (profile_name != NULL)
		WritePrivateProfileString(section, key, buf, profile_name);
	WriteKey(section, key, &val, T_UWORD);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Profile_WriteLong(CONST _UBYTE *section, CONST _UBYTE *key, _LONG val)
{
	_UBYTE buf[20];
	
	sprintf(buf, "%ld", val);
	if (profile_name != NULL)
		WritePrivateProfileString(section, key, buf, profile_name);
	WriteKey(section, key, &val, T_LONG);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Profile_WriteLongcard(CONST _UBYTE *section, CONST _UBYTE *key, _ULONG val)
{
	_UBYTE buf[20];
	
	sprintf(buf, "%lu", val);
	if (profile_name != NULL)
		WritePrivateProfileString(section, key, buf, profile_name);
	WriteKey(section, key, &val, T_ULONG);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Profile_WriteBool(CONST _UBYTE *section, CONST _UBYTE *key, _BOOL val)
{
	_UBYTE buf[20];
	
	sprintf(buf, "%d", val ? 1 : 0);
	if (profile_name != NULL)
		WritePrivateProfileString(section, key, buf, profile_name);
	WriteKey(section, key, &val, T_BOOL);
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL _BOOL Profile_ReadString(CONST _UBYTE *section, CONST _UBYTE *key, CONST _UBYTE *def, _UBYTE *str, size_t len)
{
	if (profile_name != NULL)
	{
		GetPrivateProfileString(section, key, def ? def : (CONST _UBYTE *)"", str, len - 1, profile_name);
		str[len - 1] = '\0';
		return TRUE;
	}
	strMcpy(str, len, def);
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL Profile_ReadInt(CONST _UBYTE *section, CONST _UBYTE *key, _WORD def, _WORD *pval)
{
	_UBYTE buf[20];
	int val;
	_BOOL found;
	
	val = def;
	found = FALSE;
	if (profile_name != NULL)
	{
		GetPrivateProfileString(section, key, "", buf, sizeof(buf), profile_name);
		if (*buf != '\0')
		{
			sscanf(buf, "%d", &val);
			found = TRUE;
		}
	}
	*pval = val;
	return found;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL Profile_ReadLong(CONST _UBYTE *section, CONST _UBYTE *key, _LONG def, _LONG *pval)
{
	_UBYTE buf[20];
	long val;
	_BOOL found;
	
	val = def;
	found = FALSE;
	if (profile_name != NULL)
	{
		GetPrivateProfileString(section, key, "", buf, sizeof(buf), profile_name);
		if (*buf != '\0')
		{
			sscanf(buf, "%ld", &val);
			found = TRUE;
		}
	}
	*pval = val;
	return found;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL Profile_ReadCard(CONST _UBYTE *section, CONST _UBYTE *key, _UWORD def, _UWORD *pval)
{
	_UBYTE buf[20];
	unsigned int val;
	_BOOL found;
	
	val = def;
	found = FALSE;
	if (profile_name != NULL)
	{
		GetPrivateProfileString(section, key, "", buf, sizeof(buf), profile_name);
		if (*buf != '\0')
		{
			sscanf(buf, "%u", &val);
			found = TRUE;
		}
	}
	*pval = val;
	return found;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL Profile_ReadLongCard(CONST _UBYTE *section, CONST _UBYTE *key, _ULONG def, _ULONG *pval)
{
	_UBYTE buf[20];
	unsigned long val;
	_BOOL found;
	
	val = def;
	found = FALSE;
	if (profile_name != NULL)
	{
		GetPrivateProfileString(section, key, "", buf, sizeof(buf), profile_name);
		if (*buf != '\0')
		{
			sscanf(buf, "%lu", &val);
			found = TRUE;
		}
	}
	*pval = val;
	return found;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL Profile_ReadBool(CONST _UBYTE *section, CONST _UBYTE *key, _BOOL def, _BOOL *pval)
{
	_UBYTE buf[20];
	int val;
	_BOOL found;
	
	val = def;
	found = FALSE;
	if (profile_name != NULL)
	{
		GetPrivateProfileString(section, key, "", buf, sizeof(buf), profile_name);
		if (*buf != '\0')
		{
			sscanf(buf, "%d", &val);
			found = TRUE;
		}
	}
	*pval = val != 0;
	return found;
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL _VOID Profile_DeleteKey(CONST _UBYTE *section, CONST _UBYTE *key)
{
	if (profile_name != NULL)
		WritePrivateProfileString(section, key, NULL, profile_name);
	DeleteKey(section, key);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Profile_DeleteSection(CONST _UBYTE *section)
{
	if (profile_name != NULL)
		WritePrivateProfileString(section, NULL, NULL, profile_name);
	DeleteKey(section, "");
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

LOCAL _VOID Profile_Exit(_VOID)
{
	if (profile_name != NULL)
	{
		SFREE(profile_name);
		profile_name = NULL;
	}
	if (profile_key != NO_KEY)
	{
		RegCloseKey(profile_key);
		profile_key = NO_KEY;
	}
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Profile_UseFile(CONST _UBYTE *filename, CONST _UBYTE *author)
{
	size_t len;
	_UBYTE path[PATH_MAX];
	_UBYTE *p;
	_UBYTE *profile_keyname;
	
	InstallTermproc(Profile_Exit);
	Profile_Exit();
	GetModuleFileName(GetInstance(), path, sizeof(path));
	if ((p = strrchr(path, '\\')) != NULL)
		p[1] = '\0';
	else
		*path = '\0';
	strBcat(path, filename);
	strBcat(path, ".ini");
	len = strlen(path) + 1;
	profile_name = MALLOC(len, "Profile_UseFile: filename");
	if (profile_name != NULL)
	{
		strcpy(profile_name, path);
	}
	len = 8 + 1 + strlen(author) + 1 + strlen(filename) + 1;
	profile_keyname = MALLOC(len, "Profile_UseFile: keyname");
	if (profile_keyname != NULL)
	{
		strcpy(profile_keyname, "Software");
		strcat(profile_keyname, "\\");
		strcat(profile_keyname, author);
		strcat(profile_keyname, "\\");
		strcat(profile_keyname, filename);
		RegCreateKey(HKEY_CURRENT_USER, profile_keyname, &profile_key);
		SFREE(profile_keyname);
	}
}
