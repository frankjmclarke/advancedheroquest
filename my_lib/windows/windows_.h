/*****************************************************************************
 * WINDOWS/WINDOWS_.H
 *****************************************************************************/

#ifndef __WINDOWS__H__
#define __WINDOWS__H__

#ifndef GUI_VERSION
#  ifdef WINDOWS
#     define GUI_VERSION 1
#  else
#     define GUI_VERSION 0
#  endif
#endif
#ifndef CONSOLE_VERSION
#  ifdef __CYGWIN32__
#    define CONSOLE_VERSION 1
#  else
#    define CONSOLE_VERSION 0
#  endif
#endif

#if GUI_VERSION || CONSOLE_VERSION
#  define WIN32_LEAN_AND_MEAN
#  define NOSERVICE
#  define NOMCX
#  define NOIME
#  define STRICT 1
#  include <windows.h>

#  ifdef __WIN32__
#    define GetInstance() GetModuleHandle(NULL)
#    define DWORD_FROM_HANDLE(hdl) ((DWORD)(hdl))
#    define DWORD_TO_HANDLE(wd) ((HANDLE)(wd))
#    ifndef UnlockResource
#      define UnlockResource(hnd) ((void)0)
#    endif
#    ifndef AnsiToOem
#      define AnsiToOem CharToOemA
#    endif
#    ifndef AnsiToOemBuff
#      define AnsiToOemBuff CharToOemBuffA
#    endif
#    ifndef OemToAnsi
#      define OemToAnsi OemToCharA
#    endif
#    ifndef OemToAnsiBuff
#      define OemToAnsiBuff OemToCharBuffA
#    endif
#  else
extern HINSTANCE GetInstance(void);
#    define DWORD_FROM_HANDLE(hdl) ((DWORD)(UINT)(hdl))
#    define DWORD_TO_HANDLE(wd) ((HANDLE)(UINT)(wd))
#    define GetLastError() 0l
#  endif
extern HINSTANCE GetPrevInstance(void);

#ifndef WS_EX_OVERLAPPEDWINDOW
#  define WS_EX_OVERLAPPEDWINDOW (WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE)
#endif
#ifndef WS_EX_PALETTEWINDOW
#  define WS_EX_PALETTEWINDOW (WS_EX_WINDOWEDGE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST)
#endif
#ifndef WS_EX_MDICHILD
#  define WS_EX_MDICHILD 0x00000040l
#endif
#ifndef WS_EX_TOOLWINDOW
#  define WS_EX_TOOLWINDOW 0x00000080l
#endif
#ifndef WS_EX_WINDOWEDGE
#  define WS_EX_WINDOWEDGE 0x00000100l
#endif
#ifndef WS_EX_CLIENTEDGE
#  define WS_EX_CLIENTEDGE 0x00000200l
#endif
#ifndef WS_EX_CONTEXTHELP
#  define WS_EX_CONTEXTHELP 0x00000400l
#endif
#ifndef WS_EX_RIGHT
#  define WS_EX_RIGHT             0x00001000L
#endif
#ifndef WS_EX_LEFT
#  define WS_EX_LEFT              0x00000000L
#endif
#ifndef WS_EX_RTLREADING
#  define WS_EX_RTLREADING        0x00002000L
#endif
#ifndef WS_EX_LTRREADING
#  define WS_EX_LTRREADING        0x00000000L
#endif
#ifndef WS_EX_LEFTSCROLLBAR
#  define WS_EX_LEFTSCROLLBAR     0x00004000L
#endif
#ifndef WS_EX_RIGHTSCROLLBAR
#  define WS_EX_RIGHTSCROLLBAR    0x00000000L
#endif
#ifndef WS_EX_CONTROLPARENT
#  define WS_EX_CONTROLPARENT     0x00010000L
#endif
#ifndef WS_EX_STATICEDGE
#  define WS_EX_STATICEDGE        0x00020000L
#endif
#ifndef WS_EX_APPWINDOW
#  define WS_EX_APPWINDOW         0x00040000L
#endif

#ifndef WM_PRINT
#  define WM_PRINT 791
#endif
#ifndef WM_PRINTCLIENT
#  define WM_PRINTCLIENT 792
#endif


#ifndef __PORTAB_H__
#  include <portab.h>
#endif

#if GUI_VERSION
#ifdef __WIN32__
#define FNAME_TO_WINDOWS(name) do { if (AreFileApisANSI()) OemToChar(name, name); } while(0)
#define FNAME_FROM_WINDOWS(name) do { if (AreFileApisANSI()) CharToOem(name, name); } while(0)
#endif
#endif

#endif /* GUI_VERSION || CONSOLE_VERSION */

#ifndef FNAME_TO_WINDOWS
#define FNAME_TO_WINDOWS(name)
#define FNAME_FROM_WINDOWS(name)
#endif

#define NO_BITMAP ((HBITMAP)0)
#define NO_DC ((HDC)0)
#define NO_WINDOW ((HWND)0)
#define NO_MENU ((HMENU)0)
#define NO_ACCEL ((HACCEL)0)
#define NO_REGION ((HRGN)0)
#define NO_FONT ((HFONT)0)
#define NO_ACCEL ((HACCEL)0)

#endif /* __WINDOWS__H__ */
