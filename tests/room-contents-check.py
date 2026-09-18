"""Run in an MSVC developer shell: python tests/room-contents-check.py.
Compile the actual room-click handler against a small fake map/dialog backend.
"""
from pathlib import Path
import subprocess
import tempfile

repo = Path(__file__).resolve().parents[1]
source = (repo / "wind.c").read_text(encoding="cp1252")
handler = source[source.index("LOCAL DIALOG *Room_Contents"):source.index("LOCAL _BOOL player_proc")]
prefix = r'''#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define LOCAL static
#define GLOBAL
#define CONST const
#define _VOID void
#define _BOOL int
#define _WORD short
#define _UBYTE char
#define TRUE 1
#define FALSE 0
#define UNUSED(x) (void)(x)
#define MALLOC(n,label) malloc(n)
#define FREE(p,n) free(p)
#define ICON_SCALE 8
#define FROOMCONTENTS 11
#define ROOMCONTENTSTEXT 170
#define DO_EXIT -2
#define DO_INIT -1
#define IDCANCEL 2
#define IDOK 1
#define DLG_END 1
#define DLG_CONTINUE 0
typedef int DLG_RETURN;
typedef int DIALOG;
typedef struct { short xx, yy, clicks; } WIPR_HIT;
enum { SMALL_ROOM, NORMAL_ROOM, HAZARD, LARGE_ROOM, LAIR, QUEST, BIG, MERSCHA, PASSAGE };
typedef struct { int type; char **text; } PICE;
static int display_zoom, Ysize = 20, opened, writes, last_x, last_y;
static DIALOG dialog;
static char result[65536];
static char *lines[] = { "#12", "14 Zombies (115 Gold Crowns)", "Treasure: 20 gold", NULL };
static PICE room = { NORMAL_ROOM, lines };
static int get_square(int x, int y, PICE **p) {
 last_x=x; last_y=y;
 *p = (x == 3 && y == 4) ? &room : NULL;
 return x >= 0 && x < 20 && y >= 0 && y < 20;
}
static void Dialog_SetMonospace(DIALOG *d, int id) { (void)d; (void)id; }
static void Dialog_Hide(DIALOG *d) { (void)d; }
static DIALOG *Dialog_Show(int id, DLG_RETURN (*proc)(DIALOG*,short,int*,void*), int modal, void *p) {
 (void)id; (void)proc; (void)modal; (void)p; opened++; return &dialog;
}
static void Dialog_SetStr(DIALOG *d, int id, char *text) {
 (void)d; (void)id; strcpy(result,text); writes++;
}
'''
suffix = r'''int main(void) {
 WIPR_HIT hit; int zoom, ret, before;
 for (zoom=1; zoom<=16; zoom++) {
  display_zoom=zoom;
  hit.xx=(3+1)*8*zoom + 4*zoom;
  hit.yy=(20-4)*8*zoom + 4*zoom;
  show_room_contents(&hit);
  assert(last_x==3 && last_y==4);
  assert(strstr(result,"#12\r\n14 Zombies (115 Gold Crowns)\r\n") == result);
  assert(strstr(result,"ZOMBIE\r\n") && strstr(result,"Treasure: 20 gold\r\n"));
 }
 assert(opened==1);
 before=writes;
 hit.xx=0; hit.yy=0; show_room_contents(&hit);
 assert(writes==before);
 hit.xx=-1; show_room_contents(&hit); assert(writes==before);
 display_zoom=1; hit.xx=36; hit.yy=132;
 room.type=PASSAGE; show_room_contents(&hit); assert(writes==before);
 room.type=NORMAL_ROOM; room.text=NULL; show_room_contents(&hit);
 assert(strcmp(result,"No contents recorded for this room.")==0);
 room.text=lines;
 room_contents_proc(&dialog,IDCANCEL,&ret,NULL);
 show_room_contents(&hit); assert(opened==2);
 close_room_contents(); assert(Room_Contents==NULL && !room_contents_closed);
 puts("PASS: all zoom levels, inverted coordinates, border/blank clicks, room filtering, empty contents, close/reopen.");
 return 0;
}
'''
with tempfile.TemporaryDirectory() as temp:
    root = Path(temp)
    logic = (repo / "liste.c").read_text(encoding="cp1252")
    logic = logic[logic.index("/* Room reference expansion"):]
    generated = (repo / "ahq-reference-data.h").read_text(encoding="ascii")
    (root / "check.c").write_text(prefix + generated + logic + handler + suffix)
    subprocess.run(["cl", "/nologo", "check.c", "/Fe:check.exe"], cwd=root, check=True)
    subprocess.run([str(root / "check.exe")], cwd=root, check=True)
