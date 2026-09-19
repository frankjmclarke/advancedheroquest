"""Exercise the actual enhanced overlay with a recording drawing backend.
Run in an MSVC developer shell: python tests/board-tiles-check.py.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'icon.c').read_text(encoding='cp1252')
def section(start, end):
    return source[source.index(start):source.index(end)]

prefix = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <map.h>
#include <defs.h>
typedef int MFDB;
typedef struct {
 DIRECTION (*position)(DIRECTION,_WORD*,_WORD*,_WORD,_WORD);
 _WORD (*number)(DIRECTION,_WORD*,_WORD*,_WORD,_WORD,_WORD,_WORD,_WORD,_WORD);
} ICON;
#define DIR_X 1
#define DIR_Y 2
#define DIR_NONE 0
#define RAND 3
PICE pieces[4], *Pice=pieces;
_WORD MAX_PICE=4, Ysize=30;
MFDB resources[24], digits[10], base;
MFDB *board_tiles[24], *MFDB_digit[10];
MFDB room_resources[24], *board_rooms[24], *expected_tile;
int tiles, badges, door_drawn, last_x, last_y;
int zoom=3;
int error_abort(const char *message, ...) {assert(0);return 0;}
static int fog_visible(const _UBYTE *v,_WORD i) {return v[i];}
static void get_mfdb_info(MFDB *m,_WORD *w,_WORD *h,void *unused) {*w=3;*h=5;}
static void W_Draw_Tile(void *win,MFDB *m,_WORD x,_WORD y,_WORD w,_WORD h) {
 assert(!door_drawn); assert(m==expected_tile);
 assert(w==pieces[0].w*8*zoom && h==pieces[0].h*8*zoom);
 last_x=x;last_y=y;tiles++;
}
static void W_Draw_Bitmap(void *win,MFDB *m,_WORD x,_WORD y,_WORD w,_WORD h,
                          _WORD dx,_WORD dy,_WORD scale) {
 assert(m==&base);assert(dx==x*zoom-11 && dy==y*zoom-17);assert(scale==zoom);
 if(w==8 && h==8) {assert(tiles==1);door_drawn++;}
 else {assert(!door_drawn);assert(w==6 && h==5);badges++;}
}
static ICON door_icon;
static ICON *pice_icon(PICE *p) {return &door_icon;}
static MFDB *board_feature_image(PICE *p) {return NULL;}
static void draw_board_trap(void *win,PICE *p,_WORD zoom,_WORD sx,_WORD sy) {}
'''
main = r'''
int main(void) {
 int i,dir; _UBYTE visible[4]={1,0,1,0};
 _UBYTE *label[]={(_UBYTE*)"P12",NULL};
 door_icon.number=center_nr;
 for(i=0;i<24;i++) {board_tiles[i]=resources+i;board_rooms[i]=room_resources+i;}
 for(i=0;i<10;i++)MFDB_digit[i]=digits+i;
 pieces[0].type=PASSAGE;pieces[0].x=4;pieces[0].y=7;pieces[0].text=label;
 pieces[1].type=NORMAL_ROOM;pieces[1].w=5;pieces[1].h=5;pieces[1].feature=Chest;
 pieces[2].type=DOOR;pieces[2].x=6;pieces[2].y=8;pieces[2].w=pieces[2].h=1;
 pieces[3].type=SECRET;pieces[3].w=pieces[3].h=1;
 for(dir=North;dir<=West;dir++) {
  pieces[0].pos=(DIRECTION)dir;pieces[0].w=dir<2?2:5;pieces[0].h=dir<2?5:2;
  expected_tile=board_tiles[dir];
  tiles=badges=door_drawn=0;
  draw_board_map(NULL,&base,visible,3,11,17);
  assert(tiles==1 && badges==1 && door_drawn==1);
  assert(last_x==109 && last_y==(31-7-pieces[0].h)*24-17);
 }
 tiles=badges=door_drawn=0;
 memset(visible,0,sizeof(visible));
 draw_board_map(NULL,&base,visible,3,11,17);
 assert(!tiles && !badges && !door_drawn);
 tiles=badges=door_drawn=0;
 draw_board_map(NULL,&base,NULL,3,11,17);
 assert(tiles==1 && badges==1 && door_drawn==2);
 /* Plain small rooms use one stable floor; each orientation has its own
    resource. Revealing a room changes visibility, not its chosen artwork. */
 pieces[0].type=NORMAL_ROOM;pieces[0].w=pieces[0].h=5;
 visible[0]=visible[2]=1;
 for(zoom=1;zoom<=16;zoom+=5) {
  for(dir=North;dir<=West;dir++) {
   pieces[0].pos=(DIRECTION)dir;expected_tile=board_rooms[4+dir];
   tiles=badges=door_drawn=0;
   draw_board_map(NULL,&base,visible,zoom,11,17);
   assert(tiles==1 && badges==1 && door_drawn==1);
   assert(last_x==40*zoom-11 && last_y==152*zoom-17);
  }
 }
 zoom=3;
 for(dir=North;dir<=West;dir++) {
  pieces[0].type=LARGE_ROOM;pieces[0].pos=(DIRECTION)dir;
  pieces[0].w=dir<2?5:10;pieces[0].h=dir<2?10:5;
  expected_tile=board_rooms[20+dir];tiles=badges=door_drawn=0;
  draw_board_map(NULL,&base,visible,zoom,11,17);
  assert(tiles==1 && badges==1 && door_drawn==1);
 }
 pieces[0].pos=North; /* Imported room with orientation differing from shape. */
 assert(board_room_image(&pieces[0])==board_rooms[22]);
 pieces[0].feature=Chest;assert(board_room_image(&pieces[0])==NULL);
 pieces[0].type=NORMAL_ROOM;pieces[0].w=pieces[0].h=5;
 for(i=Nothing;i<Illegal_Feature;i++) {
  pieces[0].feature=(FEATURES)i;
  if(i==Nothing || i==Wandering_Monsters || i==Maiden || i==Witch ||
     i==Man_at_Arms || i==Rogue || i==Wight) assert(board_room_image(&pieces[0])!=NULL);
  else assert(board_room_image(&pieces[0])==NULL);
 }
 pieces[0].feature=Nothing;pieces[0].w=10;
 assert(board_room_image(&pieces[0])==NULL); /* Unsupported room sizes. */
 pieces[0].type=BIG;pieces[0].h=10;assert(board_room_image(&pieces[0])==NULL);
 pieces[0].type=NORMAL_ROOM;pieces[0].w=pieces[0].h=5;
 memset(visible,0,sizeof(visible));tiles=badges=door_drawn=0;
 draw_board_map(NULL,&base,visible,zoom,11,17);
 assert(!tiles && !badges && !door_drawn); /* No enhanced hidden-room leak. */
 puts("PASS: corridor/room overlays, orientations, zoom/scroll, labels, door layering, fog and special-feature fallback.");
 return 0;
}
'''
with tempfile.TemporaryDirectory() as temp:
    work = Path(temp)
    (work / 'check.c').write_text(prefix
        + section('LOCAL _VOID center_mfdb', 'LOCAL _WORD west_mfdb')
        + section('LOCAL _WORD center_nr', 'LOCAL _WORD south_nr')
        + section('LOCAL DIRECTION room_pos', 'LOCAL DIRECTION door_pos')
        + section('LOCAL MFDB *board_room_image', '/* Feature tiles are loaded on demand:')
        + source[source.index('GLOBAL _VOID draw_board_map'):]
        + main)
    subprocess.run(['cl', '/nologo', '/I'+str(root), '/I'+str(root/'my_lib'),
                    'check.c', '/Fe:check.exe'], cwd=work, check=True)
    subprocess.run([str(work/'check.exe')], cwd=work, check=True)
