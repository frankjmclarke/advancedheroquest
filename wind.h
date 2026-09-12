#ifndef __WIND_H__
#define __WIND_H__

#include <stdio.h>
#ifndef __PORTAB_H__
#include <portab.h>
#endif
#ifndef __WINDOW_H__
#include <window.h>
#endif
#ifndef __W_DIALOG_H__
#include <w_dialog.h>
#endif
#ifndef __PATH__
#include <path.h>
#endif

extern WINDOW_DEF *Grafik_Karte;
extern WINDOW_DEF *Text_Karte;
extern WINDOW_DEF *Monster_Liste;
extern WINDOW_DEF *Statistik;
extern _WORD display_zoom;
extern _WORD print_zoom;

enum { W_DATEI=1, W_TEXT, W_GRAFIK, W_LISTE, W_STATISTIK };

typedef enum _img_type { IMG, PCX, BMP } IMG_TYPE;

_BOOL save_grafic(PATH *p, _BOOL compress, _WORD zoom, IMG_TYPE type);
_BOOL save_text(PATH *p);
_BOOL save_liste(PATH *p);
_BOOL save_statistik(PATH *p);
_BOOL print_grafic(_WORD zoom);
_BOOL print_text(_VOID);
_BOOL print_liste(_VOID);
_BOOL print_statistik(_VOID);
_BOOL print_file(WINDOW_DEF *wr);
_VOID show_grafic(_UBYTE *str);
_VOID show_text(_UBYTE *str);
WINDOW_DEF *show_text_file(_UBYTE *path);
_VOID show_liste(_UBYTE *str);
_VOID show_statistik(_UBYTE *str);
_VOID close_all_windows(_BOOL delete);

DIALOG *show_string(CONST _UBYTE *str);
_VOID hide_string(DIALOG *ptr);

#endif /* __WIND_H__ */
