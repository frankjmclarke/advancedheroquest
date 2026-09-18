"""Run in an MSVC developer shell: python tests/fog-check.py."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
source = (root / "map.c").read_text(encoding="cp1252")
logic = source[source.index("/* Fog geometry"):]
render_source = (root / "icon.c").read_text(encoding="cp1252")
render = render_source[render_source.index("LOCAL _BOOL draw_map_visible"):render_source.index("GLOBAL _BOOL draw_img_map")]
harness = r'''#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <map.h>
#include <defs.h>
PICE pieces[8];
PICE *Pice = pieces;
_WORD MAX_PICE = 8, Ysize = 30;
static _UBYTE visible[8];
static void piece(int i, TYPE type, int x, int y, int w, int h, DIRECTION dir) {
 memset(&pieces[i],0,sizeof(PICE));
 pieces[i].type=type; pieces[i].x=x; pieces[i].y=y;
 pieces[i].w=w; pieces[i].h=h; pieces[i].pos=dir;
}
typedef int MFDB;
static int drawn[8];
static int mfdb_start_paint(MFDB *p) { return 1; }
static void mfdb_end_paint(MFDB *p) {}
static void draw_border(MFDB *p) {}
static int draw_pice(MFDB *p, PICE *piece) { drawn[piece-Pice]++; return 1; }
int error_abort(const char *s, ...) { assert(0); return 0; }
static void click(int i) {
 PICE *p=&pieces[i];
 int x=(p->x+1)*ICON_SCALE, y=(Ysize-p->y)*ICON_SCALE;
 if(p->pos==North) y-=ICON_SCALE/2;
 if(p->pos==South) y+=ICON_SCALE/2;
 if(p->pos==East) x+=ICON_SCALE/2;
 if(p->pos==West) x-=ICON_SCALE/2;
 assert(fog_open_door(visible,x+ICON_SCALE/2,y+ICON_SCALE/2));
}
int main(void) {
 int dir; MFDB image;
 piece(0,PASSAGE,0,0,2,10,North);
 piece(1,NORMAL_ROOM,2,1,5,5,North);
 piece(2,DOOR,2,2,1,1,West);
 piece(3,QUEST,7,1,5,5,North);
 piece(4,SECRET,7,2,1,1,West);
 piece(5,EMPTY,0,0,0,0,North);
 piece(6,EMPTY,0,0,0,0,North);
 piece(7,EMPTY,0,0,0,0,North);
 fog_init(visible);
 assert(visible[0] && !visible[1] && !visible[3]);
 assert(fog_visible(visible,2) && !fog_visible(visible,4));
 draw_map_visible(&image,visible);
 assert(drawn[0]==1 && drawn[2]==1 && drawn[1]==0 && drawn[3]==0 && drawn[4]==0);
 assert(!fog_open_door(visible,8*8,28*8)); /* hidden second door */
 assert(!visible[3]);
 assert(!fog_open_door(visible,0,0));
 click(2);
 memset(drawn,0,sizeof(drawn)); draw_map_visible(&image,visible);
 assert(drawn[0]==1 && drawn[1]==1 && drawn[2]==1 && drawn[4]==1 && drawn[3]==0);
 assert(visible[1] && !visible[3] && fog_visible(visible,4));
 click(4); assert(visible[3]);
 assert(!fog_open_door(visible,8*8,28*8)); /* already open */
 fog_init(visible); assert(!visible[1] && !visible[3]);
 for(dir=North;dir<=West;dir++) {
  int dx=dir==East ? 1 : dir==West ? -1 : 0;
  int dy=dir==North ? 1 : dir==South ? -1 : 0;
  piece(0,PASSAGE,10+dx,10+dy,1,1,North);
  piece(1,NORMAL_ROOM,10,10,1,1,North);
  piece(2,DOOR,10,10,1,1,(DIRECTION)dir);
  piece(3,EMPTY,0,0,0,0,North);
  piece(4,EMPTY,0,0,0,0,North);
  fog_init(visible); assert(fog_visible(visible,2));
  click(2); assert(visible[1]);
 }
 puts("PASS: initial corridors, hidden rooms/doors, chained reveals, secret doors, four door orientations, repeat clicks and reset.");
 return 0;
}
'''
with tempfile.TemporaryDirectory() as temp:
    work = Path(temp)
    # Includes/globals must precede the implementation; main follows it.
    split = harness.index("static void click")
    (work / "check.c").write_text(harness[:split] + logic + render + harness[split:])
    subprocess.run(["cl", "/nologo", "/I" + str(root), "/I" + str(root / "my_lib"), "check.c", "/Fe:check.exe"], cwd=work, check=True)
    subprocess.run([str(work / "check.exe")], cwd=work, check=True)
