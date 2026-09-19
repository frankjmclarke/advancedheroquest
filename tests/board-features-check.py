"""Compile the actual feature selection, icon metadata and overlay renderer.

Run in an MSVC developer shell: python tests/board-features-check.py.
The recording backend checks all prepared features, original number placement,
rotations, zoom/scroll coordinates, hidden-room allocation and resource sizes.
"""
from pathlib import Path
import re
import struct
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'icon.c').read_text(encoding='cp1252')

def section(start, end):
    return source[source.index(start):source.index(end)]

import json
catalog = json.loads((root / 'rsh/board-atlas.json').read_text())
descriptors = [f"{{{r['id']},{r['width']},{r['height']}}}" for r in catalog['images'] if 500 <= r['id'] < 700]

prefix = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <map.h>
#include <defs.h>
#include <grafic.rh>
#include <board-features.rh>
#define RAND 3
typedef struct {int id,w,h;} MFDB;
PICE pieces[1], *Pice=pieces;
_WORD MAX_PICE=1,Ysize=40;
MFDB base, digit={0,3,5};
int trap_markers;
int allocations,tiles,labels,old_doors,zoom=3,expected_x,expected_y,expected_w,expected_h;
int error_abort(const char *s, ...) {assert(0);return 0;}
static _BOOL fog_visible(const _UBYTE *visible,_WORD i) {return visible[i];}
static void get_mfdb_info(MFDB *m,_WORD *w,_WORD *h,void *unused) {*w=m->w;*h=m->h;}
static void W_Draw_Tile(void *win,MFDB *m,_WORD x,_WORD y,_WORD w,_WORD h) {
 if(m->id==700) {
  assert(x==((pieces[0].x+1)*8+2)*zoom-11);
  assert(y==((Ysize+1-pieces[0].y-pieces[0].h)*8+2)*zoom-17);
  assert(w<=pieces[0].w*8*zoom && h==8*zoom);trap_markers++;return;
 }
 if(pieces[0].type==DOOR) {
  assert(x==expected_x*zoom-11 && y==expected_y*zoom-17);
  assert(w==8*zoom && h==8*zoom);tiles++;return;
 }
 assert(x==(pieces[0].x+1)*8*zoom-11);
 assert(y==(Ysize+1-pieces[0].y-pieces[0].h)*8*zoom-17);
 assert(w==pieces[0].w*8*zoom && h==pieces[0].h*8*zoom);
 assert(m->w==pieces[0].w*BOARD_PIXELS_PER_SQUARE && m->h==pieces[0].h*BOARD_PIXELS_PER_SQUARE);tiles++;
}
static void W_Draw_Bitmap(void *win,MFDB *m,_WORD x,_WORD y,_WORD w,_WORD h,
                          _WORD dx,_WORD dy,_WORD scale) {
 if(pieces[0].type==DOOR || pieces[0].type==SECRET) {
  assert(!tiles && w==8 && h==8);old_doors++;return;
 }
 assert(tiles==1 && m==&base);assert(scale==zoom);
 assert(x==expected_x && y==expected_y && w==expected_w && h==expected_h);
 assert(dx==x*zoom-11 && dy==y*zoom-17);labels++;
}
'''
backend = 'static MFDB embedded[]={' + ','.join(descriptors) + '};\n' + r'''
static MFDB *get_board_bitmap(_WORD id) {
 static MFDB trap={700,64,32};
 unsigned int i;allocations++;
 if(id==700)return &trap;
 for(i=0;i<sizeof(embedded)/sizeof(embedded[0]);i++)if(embedded[i].id==id)return &embedded[i];
 assert(0);return NULL;
}
'''
main = r'''
static void check_piece(TYPE type,FEATURES feature,int w,int h) {
 int d,before;_UBYTE visible[1]={0};
 _UBYTE *text[]={(_UBYTE*)"R12",NULL};
 ICON *icon;DIRECTION dir;_WORD x,y,axis;
 pieces[0].type=type;pieces[0].feature=feature;
 pieces[0].x=4;pieces[0].y=6;pieces[0].text=text;
 for(d=North;d<=West;d++) {
  pieces[0].pos=(DIRECTION)d;pieces[0].w=d<2?w:h;pieces[0].h=d<2?h:w;
  visible[0]=0;tiles=labels=0;before=allocations;
  draw_board_map(NULL,&base,visible,zoom,11,17);
  assert(!tiles && !labels && allocations==before);
  visible[0]=1;
  icon=pice_icon(&pieces[0]);dir=(DIRECTION)d;
  x=(pieces[0].x+1)*8;y=(pieces[0].y+1)*8;
  if(icon->position)dir=icon->position(dir,&x,&y,pieces[0].w*8,pieces[0].h*8);
  /* Expected badge rectangle follows the original draw_pice algorithm. */
  axis=icon->number(dir,&x,&y,pieces[0].w*8,pieces[0].h*8,6,5,3,10);
  expected_w=axis==DIR_X?6:3;expected_h=axis==DIR_X?5:10;
  expected_x=x;expected_y=(Ysize+2)*8-y-expected_h;
  draw_board_map(NULL,&base,visible,zoom,11,17);
  assert(tiles==1 && labels==1);
  before=allocations;tiles=labels=0;
  draw_board_map(NULL,&base,NULL,zoom,11,17);
  assert(tiles==1 && labels==1 && allocations==before);
 }
}
int main(void) {
 ICON *icon;int i,before;_UBYTE visible[1];_WORD x,y;
 for(icon=Icons;icon->ptr;icon++)*(icon->ptr)=icon;
 for(i=0;i<10;i++)MFDB_digit[i]=&digit;
 /* Plain-room resources are not used by these feature cases. */
 for(zoom=1;zoom<=16;zoom+=5) {
  for(i=Chasm;i<Illegal_Feature;i++) {
   if(i==Wight)continue; /* NPC has the plain-room floor. */
   check_piece(HAZARD,(FEATURES)i,5,5);
  }
  check_piece(LAIR,Chest,5,10);
  check_piece(QUEST,Stairs_and_Chest,5,10);
  check_piece(LARGE_ROOM,Tomb,5,10);
  check_piece(STAIRS_OUT,Nothing,2,2);
  check_piece(STAIRS_DOWN,Nothing,2,2);
  check_piece(BIG,Nothing,10,10);
  check_piece(MERSCHA,Nothing,15,15);
 }
 assert(allocations==sizeof(embedded)/sizeof(embedded[0]));
 /* A malformed/imported size must retain the original rendering. */
 pieces[0].w=7;assert(board_feature_image(&pieces[0])==NULL);
 /* Trap descriptions are GM annotations, never ordinary door skins or
    automatic player reveals. Exercise actual table wording and negatives. */
 {
  _UBYTE *trap_text[]={(_UBYTE*)"R12",(_UBYTE*)"Portcullis (Spot 6, Disarm 11)",NULL};
  _UBYTE *no_trap[]={(_UBYTE*)"R12",(_UBYTE*)"No Portcullis",NULL};
  _UBYTE *multiline[]={(_UBYTE*)"R12",(_UBYTE*)"Notes\n  PORTCULLIS (Spot 6)",NULL};
  check_piece(HAZARD,Rats,5,5);
  pieces[0].text=trap_text;assert(has_portcullis_trap(&pieces[0]));
  tiles=labels=trap_markers=0;before=allocations;
  draw_board_map(NULL,&base,NULL,zoom,11,17);
  assert(tiles==1 && labels==1 && trap_markers==1 && allocations==before+1);
  tiles=labels=trap_markers=0;visible[0]=1;before=allocations;
  draw_board_map(NULL,&base,visible,zoom,11,17);
  assert(tiles==1 && labels==1 && !trap_markers && allocations==before);
  tiles=labels=trap_markers=0;visible[0]=0;
  draw_board_map(NULL,&base,visible,zoom,11,17);
  assert(!tiles && !labels && !trap_markers);
  pieces[0].text=no_trap;assert(!has_portcullis_trap(&pieces[0]));
  pieces[0].text=multiline;assert(has_portcullis_trap(&pieces[0]));
  pieces[0].text=NULL;assert(!has_portcullis_trap(&pieces[0]));
  pieces[0].type=DOOR;pieces[0].w=pieces[0].h=1;tiles=old_doors=0;
  draw_board_map(NULL,&base,NULL,zoom,11,17);assert(!tiles && old_doors==1);
 }
 puts("PASS: all 136 feature sprites, four orientations, original number positions, zoom/scroll, fog and lazy loading.");
 puts("PASS: table-generated portcullis traps, GM-only markers, hidden/revealed Player View rooms and unchanged doors.");
 return 0;
}
'''
with tempfile.TemporaryDirectory() as temp:
    work = Path(temp)
    (work / 'check.c').write_text(prefix + backend
        + section('typedef struct icon', 'LOCAL ICON *rack;') + 'LOCAL ICON *rack;\n'
        + section('LOCAL ICON *pice_icon', '/*\n * Ink and paper')
        + section('LOCAL _VOID center_mfdb', 'typedef struct board_image')
        + section('LOCAL MFDB *board_tiles', 'GLOBAL _BOOL init_icons')
        + source[source.index('LOCAL MFDB *board_room_image'):]
        + main)
    subprocess.run(['cl', '/nologo', '/I'+str(root), '/I'+str(root/'my_lib'),
                    '/I'+str(root/'rsh'), 'check.c', '/Fe:check.exe'], cwd=work, check=True)
    subprocess.run([str(work/'check.exe')], cwd=work, check=True)
