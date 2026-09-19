"""Test actual Win32 atlas assembly for every image and every pixel.

Run in an MSVC developer shell after building: python tests/board-atlas-check.py.
Also checks the generator's lossless RGB hashes without source scans or Pillow.
"""
from pathlib import Path
import hashlib
import json
import struct
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
catalog = json.loads((root / 'rsh/board-atlas.json').read_text())
data = (root / 'rsh/board/atlas.bmp').read_bytes()
offset = struct.unpack_from('<I', data, 10)[0]
width, height = struct.unpack_from('<ii', data, 18)
assert struct.unpack_from('<H', data, 28)[0] == 24
stride = (width * 3 + 3) & ~3
pixels = [data[offset + y * stride:offset + y * stride + width * 3] for y in range(height)][::-1]
block, columns = catalog['block'], catalog['columns']
used = set()
for entry in catalog['images']:
    image = bytearray(entry['width'] * entry['height'] * 3)
    n = entry['offset']
    for y in range(0, entry['height'], block):
        for x in range(0, entry['width'], block):
            code = catalog['codes'][n]; n += 1
            patch, turn = code >> 2, code & 3
            used.add(patch)
            for dy in range(block):
                for dx in range(block):
                    sx, sy = ((dx, dy), (dy, block - 1 - dx),
                              (block - 1 - dx, block - 1 - dy), (block - 1 - dy, dx))[turn]
                    sx += (patch % columns) * block
                    sy += (patch // columns) * block
                    start = ((y + dy) * entry['width'] + x + dx) * 3
                    image[start:start + 3] = pixels[sy][sx * 3:sx * 3 + 3][::-1]
    assert hashlib.sha256(image).hexdigest() == entry['sha256'], entry['file']
assert used == set(range(catalog['patches'])), 'Unreferenced patches in the atlas'

source = (root / 'my_lib/windows/mfdb.c').read_text(encoding='cp1252')
implementation = source[source.index('GLOBAL MFDB *assemble_mfdb'):]
prefix = r'''
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#undef assert
#define assert(x) do { if(!(x)) {fprintf(stderr,"Assertion failed at line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
#define GLOBAL
#define CONST const
typedef short _WORD;
typedef unsigned short _UWORD;
typedef unsigned long _ULONG;
typedef int _BOOL;
#include <board-atlas.h>
typedef struct {HBITMAP bmp;BITMAP ic;DWORD *atlas_pixels;} MFDB;
#define MALLOC(n,label) malloc(n)
#define FREE(p,n) free(p)
static MFDB *get_mfdb(_WORD w,_WORD h,_WORD planes,void *unused) {
 MFDB *m=calloc(1,sizeof(*m));BITMAPINFO info={0};void *bits;
 assert(m);info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
 info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-h;
 info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
 m->bmp=CreateDIBSection(NULL,&info,DIB_RGB_COLORS,&bits,NULL,0);
 assert(m->bmp);GetObject(m->bmp,sizeof(m->ic),&m->ic);return m;
}
static void free_mfdb(MFDB *m) {if(m){DeleteObject(m->bmp);free(m);}}
static DWORD *read_pixels(MFDB *m) {
 BITMAPINFO info={0};HDC dc=GetDC(NULL);
 DWORD *p=malloc(m->ic.bmWidth*m->ic.bmHeight*4);assert(p);
 info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
 info.bmiHeader.biWidth=m->ic.bmWidth;info.bmiHeader.biHeight=-m->ic.bmHeight;
 info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
 assert(GetDIBits(dc,m->bmp,0,m->ic.bmHeight,p,&info,DIB_RGB_COLORS));
 ReleaseDC(NULL,dc);return p;
}
'''
main = r'''
typedef struct {int id,w,h;unsigned long offset;} IMAGE;
static IMAGE images[]=BOARD_IMAGE_ENTRIES;
int main(int argc,char **argv) {
 HMODULE module;MFDB atlas={0},*image;DWORD *a,*b;
 unsigned int i,n;int x,y,dx,dy,sx,sy,code,patch,turn;DWORD before;
 assert(argc==2);module=LoadLibraryExA(argv[1],NULL,LOAD_LIBRARY_AS_DATAFILE);assert(module);
 atlas.bmp=LoadBitmap(module,MAKEINTRESOURCE(BOARD_ATLAS_RESOURCE));assert(atlas.bmp);
 GetObject(atlas.bmp,sizeof(atlas.ic),&atlas.ic);a=read_pixels(&atlas);
 before=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
 for(i=0;i<sizeof(images)/sizeof(images[0]);i++) {
  image=assemble_mfdb(&atlas,images[i].w,images[i].h,BOARD_ATLAS_BLOCK,BOARD_ATLAS_COLUMNS,
                      board_patch_codes+images[i].offset);assert(image);b=read_pixels(image);
  n=images[i].offset;
  for(y=0;y<images[i].h;y+=BOARD_ATLAS_BLOCK)for(x=0;x<images[i].w;x+=BOARD_ATLAS_BLOCK) {
   code=board_patch_codes[n++];patch=code>>2;turn=code&3;
   for(dy=0;dy<BOARD_ATLAS_BLOCK;dy++)for(dx=0;dx<BOARD_ATLAS_BLOCK;dx++) {
    switch(turn) {
    case 0:sx=dx;sy=dy;break;
    case 1:sx=dy;sy=BOARD_ATLAS_BLOCK-1-dx;break;
    case 2:sx=BOARD_ATLAS_BLOCK-1-dx;sy=BOARD_ATLAS_BLOCK-1-dy;break;
    default:sx=BOARD_ATLAS_BLOCK-1-dy;sy=dx;break;
    }
    sx+=(patch%BOARD_ATLAS_COLUMNS)*BOARD_ATLAS_BLOCK;
    sy+=(patch/BOARD_ATLAS_COLUMNS)*BOARD_ATLAS_BLOCK;
    assert((a[sy*atlas.ic.bmWidth+sx]&0xffffff)==(b[(y+dy)*images[i].w+x+dx]&0xffffff));
   }
  }
  free(b);free_mfdb(image);
 }
 assert(GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS)==before);
 free(a);free(atlas.atlas_pixels);DeleteObject(atlas.bmp);FreeLibrary(module);
 puts("PASS: all 185 images assembled pixel-for-pixel by Win32, including rotations; no GDI leaks.");
 return 0;
}
'''
with tempfile.TemporaryDirectory() as temp:
    work = Path(temp)
    (work / 'check.c').write_text(prefix + implementation + main)
    subprocess.run(['cl', '/nologo', '/I'+str(root/'rsh'), 'check.c',
                    '/Fe:check.exe', 'gdi32.lib', 'user32.lib'], cwd=work, check=True)
    subprocess.run([str(work/'check.exe'), str(root/'bin/hq_map.exe')], cwd=work, check=True)
print('PASS: all recipe hashes match the full-colour originals; every shared patch is used.')
