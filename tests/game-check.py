"""Native integration checks: real tables/maps, session codec, recovery and party UI.
Run after build-msvc.bat in an x86 MSVC developer shell.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
HARNESS = r'''
#include <assert.h>
#include <stdarg.h>
#include <windows.h>
#include <game.h>
#include <openwork.h>
static HWND test_map_hwnd;
static L_GRECT test_document;
static HWND test_window(_VOID *window) { return test_map_hwnd; }
static void test_doc(WINDOW_DEF *window,L_GRECT *doc) { *doc=test_document; }
#define W_GetHwnd test_window
#define Wind_GetDoc test_doc
#include "game.c"
#undef W_GetHwnd
#undef Wind_GetDoc
#include <mem.h>
#include <table.h>
#include <makemap.h>
#include <random.h>
_UBYTE ProgramName[]="Game tests";
_WORD WindFormMain(_WORD argc,const _UBYTE **argv) { return 0; }
_BOOL error_abort(const char *fmt,...) { fprintf(stderr,"Map error: %s\n",fmt); exit(2); }
void game_loaded_title(const char *title) { }
#undef assert
#define assert(x) do { if(!(x)) { fprintf(stderr,"Assertion failed: %s, line %d\n",#x,__LINE__); exit(3); } } while(0)
static void generate(void) {
    int i,x,y; PICE *p;
    new_rand(1234); assert(makemap(52,32,26,16,North));
    game_new_dungeon("Test dungeon");
    for(i=0;i<5;i++) hero_defaults(&heroes[i],i,i+1);
    hero_count=5;
    i=0;
    for(y=0;y<Ysize && i<5;y++) for(x=0;x<Xsize && i<5;x++) {
        if(get_square(x,y,&p) && p && p->type!=TEST) { heroes[i].x=x; heroes[i].y=y; ++i; }
    }
    assert(i==5);
}
static void write_bytes(const char *path,const unsigned char *p,size_t n) {
    FILE *fp=fopen(path,"wb"); assert(fp); assert(fwrite(p,1,n,fp)==n); fclose(fp);
}
static LPARAM mouse_at(int x,int y) {
    int zoom=Grafik_Zoom_Get();
    return MAKELPARAM((x+1)*8*zoom+4*zoom-test_document.xx,(Ysize-y)*8*zoom+4*zoom-test_document.yy);
}
/* The legacy map procedure activates the map but consumes the first click. */
static LRESULT CALLBACK inactive_map_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    if(msg==WM_MOUSEACTIVATE) return MA_ACTIVATEANDEAT;
    return DefWindowProc(hwnd,msg,wp,lp);
}
static void mouse_checks(void) {
    int z,x,y,found,from_x=heroes[0].x,from_y=heroes[0].y;
    int dest_x=0,dest_y=0;
    test_map_hwnd=CreateWindowExA(0,"STATIC","Test map",WS_POPUP,0,0,800,800,NULL,NULL,GetModuleHandle(NULL),NULL);
    assert(test_map_hwnd);
    SetWindowLongPtr(test_map_hwnd,GWLP_WNDPROC,(LONG_PTR)inactive_map_proc);
    game_attach((WINDOW_DEF*)1,0);
    for(z=1;z<=16;z+=5) {
        display_zoom=z;
        test_document.xx=(from_x+1)*8*z-150;
        test_document.yy=(Ysize-from_y)*8*z-150;
        found=0;
        for(y=0;y<Ysize && !found;y++) for(x=0;x<Xsize && !found;x++) {
            LPARAM point=mouse_at(x,y);
            if(can_move(0,x,y,NULL) && (x!=from_x || y!=from_y) &&
                (short)LOWORD(point)>20 && (short)LOWORD(point)<780 && (short)HIWORD(point)>20 && (short)HIWORD(point)<780) {
                dest_x=x;dest_y=y;found=1;
            }
        }
        assert(found);
        /* Place a reserve hero with the first click after leaving the dialog. */
        heroes[0].x=heroes[0].y=-1; selected=0; move_mode=1;
        assert(SendMessage(test_map_hwnd,WM_MOUSEACTIVATE,0,MAKELPARAM(HTCLIENT,WM_LBUTTONDOWN))==MA_ACTIVATE);
        SendMessage(test_map_hwnd,WM_LBUTTONDOWN,MK_LBUTTON,mouse_at(from_x,from_y));
        SendMessage(test_map_hwnd,WM_LBUTTONUP,0,mouse_at(from_x,from_y));
        assert(heroes[0].x==from_x && heroes[0].y==from_y && !move_mode);
        SendMessage(test_map_hwnd,WM_LBUTTONDOWN,MK_LBUTTON,mouse_at(from_x,from_y));
        SendMessage(test_map_hwnd,WM_MOUSEMOVE,MK_LBUTTON,mouse_at(dest_x,dest_y));
        assert(preview_hero==0 && heroes[0].x==from_x && heroes[0].y==from_y);
        SendMessage(test_map_hwnd,WM_LBUTTONUP,0,mouse_at(dest_x,dest_y));
        assert(heroes[0].x==dest_x && heroes[0].y==dest_y);
        /* Select, release without moving, then click a destination. */
        SendMessage(test_map_hwnd,WM_LBUTTONDOWN,MK_LBUTTON,mouse_at(dest_x,dest_y));
        SendMessage(test_map_hwnd,WM_LBUTTONUP,0,mouse_at(dest_x,dest_y));
        assert(move_mode);
        SendMessage(test_map_hwnd,WM_LBUTTONDOWN,MK_LBUTTON,mouse_at(from_x,from_y));
        assert(heroes[0].x==from_x && heroes[0].y==from_y && !move_mode);
        /* An occupied square rejects a drop; no stats or positions change. */
        SendMessage(test_map_hwnd,WM_LBUTTONDOWN,MK_LBUTTON,mouse_at(from_x,from_y));
        SendMessage(test_map_hwnd,WM_LBUTTONUP,0,mouse_at(heroes[1].x,heroes[1].y));
        assert(heroes[0].x==from_x && heroes[0].y==from_y);
    }
    DestroyWindow(test_map_hwnd); test_map_hwnd=NULL; selected=-1; move_mode=0;
}
int main(int argc,char **argv) {
    GAME_DATA snapshot,*decoded,*again; unsigned char *bytes,*other; size_t size,other_size;
    int i,j,x,y,old_count; HWND dialog; PICE *p; char path[MAX_PATH];
    assert(mem_alloc(100,100,1000,4096)); heap_clear();
    assert(SetCurrentDirectoryA(argv[1]));
    assert(read_table("sonne2.tab",NULL));
    generate();
    /* Snapshot exact grid and text. Mark a room explored and modify stats. */
    for(i=0;i<MAX_PICE;i++) if(Pice[i].type==NORMAL_ROOM || Pice[i].type==HAZARD) { game_fog()[i]=1; break; }
    heroes[1].wounds=1; heroes[3].stats[6]=11; strcpy(heroes[0].name,"Aldric");
    assert(capture(&snapshot));
    assert(game_encode(&snapshot,&bytes,&size)); free(snapshot.cells);
    decoded=game_decode(bytes,size); assert(decoded);
    /* Missing room text stays NULL, rather than an allocated empty list. */
    for(i=0;i<MAX_PICE;i++) if(Pice[i].type!=EMPTY && !Pice[i].text) assert(!decoded->pieces[i].text);
    assert(game_encode(decoded,&other,&other_size)); assert(size==other_size && !memcmp(bytes,other,size)); free(other);
    for(i=0;i<5;i++) { assert(!strcmp(heroes[i].name,decoded->heroes[i].name)); assert(!memcmp(&heroes[i].kind,&decoded->heroes[i].kind,sizeof(HERO)-40)); }
    /* Truncation, corruption, future version, overlap and malformed positions. */
    for(i=0;i<64;i++) assert(!game_decode(bytes,i));
    assert(!game_decode(bytes,size-1)); bytes[size-1]^=1; assert(!game_decode(bytes,size)); bytes[size-1]^=1;
    bytes[8]=2; assert(!game_decode(bytes,size)); bytes[8]=1;
    x=decoded->heroes[1].x; y=decoded->heroes[1].y;
    decoded->heroes[1].x=decoded->heroes[0].x; decoded->heroes[1].y=decoded->heroes[0].y;
    assert(!game_encode(decoded,&other,&other_size)); decoded->heroes[1].x=x; decoded->heroes[1].y=y;
    assert(!can_move(1,heroes[0].x,heroes[0].y,NULL)); assert(!can_move(0,-1,0,NULL)); assert(!can_move(0,999,999,NULL));
    for(y=0;y<Ysize;y++) for(x=0;x<Xsize;x++) {
        get_square(x,y,&p);
        if(!p) assert(!can_move(0,x,y,NULL));
        else if(!fog_visible(game_fog(),(_WORD)(p-Pice))) assert(!can_move(0,x,y,game_fog()));
    }
    mouse_checks();
    assert(SetCurrentDirectoryA(argv[2]));
    GetFullPathNameA("recovery.hqg",MAX_PATH,recovery_path,NULL);
    GetFullPathNameA("previous.hqg",MAX_PATH,previous_path,NULL);
    /* Same atomic writer is used by Save Game and automatic recovery. */
    assert(write_game(recovery_path,1)); again=read_game(recovery_path); assert(again); game_data_free(again);
    heroes[0].wounds=2; assert(write_game(recovery_path,1));
    again=read_game(previous_path); assert(again && again->heroes[0].wounds==4); game_data_free(again);
    /* Corrupt latest recovery; load falls back to the previous complete save. */
    write_bytes(recovery_path,(const unsigned char*)"bad",3);
    active=0; assert(game_load(1)); assert(heroes[0].wounds==4);
    assert(capture(&snapshot)); assert(game_encode(&snapshot,&other,&other_size)); free(snapshot.cells);
    assert(size==other_size && !memcmp(bytes,other,size)); free(other);
    /* Restore works with different allocated map sizes and without table files. */
    map_exit(); pice_exit(); assert(map_init(4,4)); assert(pice_init(5));
    assert(map_restore(decoded->pieces,decoded->count,decoded->width,decoded->height,decoded->cells));
    game_set_fog(decoded->visible,decoded->count);
    for(i=0;i<decoded->width*decoded->height;i++) {
        get_square(i%decoded->width,i/decoded->width,&p);
        assert((p?(int)(p-Pice):-1)==decoded->cells[i]);
    }
    /* Keep decoded text ownership alive while drawing/editing. */
    game_data_free(loaded_owner); loaded_owner=decoded;
    dialog=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGPARTY),NULL,party_proc,0); assert(dialog);
    assert(SendDlgItemMessage(dialog,GPLIST,LB_GETCOUNT,0,0)==5);
    SetDlgItemTextA(dialog,GPNAME,"Renamed hero"); SetDlgItemInt(dialog,GPWOUNDS,2,FALSE);
    assert(party_apply(dialog)); assert(!strcmp(heroes[0].name,"Renamed hero") && heroes[0].wounds==2);
    SendMessage(dialog,WM_COMMAND,GPRESERVE,0); assert(heroes[0].x==-1);
    game_command(MGAMEUNDO); assert(heroes[0].x>=0);
    /* Preserve old class IDs, and add all four new classes through the UI. */
    assert(SendDlgItemMessage(dialog,GPCLASS,CB_GETCOUNT,0,0)==HERO_CLASS_COUNT);
    for(i=5;i<HERO_CLASS_COUNT;i++) {
        SendDlgItemMessage(dialog,GPCLASS,CB_SETCURSEL,i,0);
        SendMessage(dialog,WM_COMMAND,GPADD,0);
        assert(hero_count==i+1 && heroes[i].kind==i && heroes[i].x==-1);
        assert(strstr(heroes[i].name,hero_classes[i]) && strlen(hero_class_rules(i))>100);
    }
    assert(heroes[5].stats[2]==5 && heroes[5].stats[5]==9);
    assert(heroes[6].stats[0]==6 && heroes[6].stats[1]==7 && heroes[6].stats[4]==9 && heroes[6].stats[5]==7);
    assert(heroes[7].stats[0]==7 && heroes[7].stats[2]==6);
    assert(heroes[8].stats[0]==6 && heroes[8].stats[6]==7);
    assert(capture(&snapshot)); assert(game_encode(&snapshot,&other,&other_size)); free(snapshot.cells);
    again=game_decode(other,other_size); assert(again && again->hero_count==9);
    for(i=0;i<HERO_CLASS_COUNT;i++) assert(again->heroes[i].kind==i);
    again->heroes[8].kind=HERO_CLASS_COUNT;
    { unsigned char *invalid; size_t invalid_size; assert(!game_encode(again,&invalid,&invalid_size)); }
    game_data_free(again); free(other);

    /* Render the actual native dialog for visual inspection. */
    {
        HDC screen=GetDC(NULL),dc=CreateCompatibleDC(screen); HBITMAP bitmap,old;
        BITMAPINFO info; BITMAPFILEHEADER file; void *pixels; FILE *fp; RECT rect; int w,h;
        SetWindowPos(dialog,NULL,-10000,-10000,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
        ShowWindow(dialog,SW_SHOWNOACTIVATE); UpdateWindow(dialog); GetWindowRect(dialog,&rect);
        w=rect.right-rect.left;h=rect.bottom-rect.top;memset(&info,0,sizeof(info));
        info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-h;
        info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
        bitmap=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&pixels,NULL,0); assert(bitmap);
        old=(HBITMAP)SelectObject(dc,bitmap); PrintWindow(dialog,dc,0);
        memset(&file,0,sizeof(file));file.bfType=0x4d42;file.bfOffBits=sizeof(file)+sizeof(info.bmiHeader);file.bfSize=file.bfOffBits+w*h*4;
        fp=fopen(argv[3],"wb");assert(fp);fwrite(&file,sizeof(file),1,fp);fwrite(&info.bmiHeader,sizeof(info.bmiHeader),1,fp);fwrite(pixels,w*h*4,1,fp);fclose(fp);
        SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);ReleaseDC(NULL,screen);
    }
    DestroyWindow(dialog);
    reserve_party(); for(i=0;i<hero_count;i++) assert(heroes[i].x==-1 && heroes[i].y==-1);
    old_count=hero_count; new_rand(5678); assert(makemap(52,32,26,16,North)); game_new_dungeon("Next dungeon"); assert(hero_count==old_count && heroes[0].wounds==2);
    for(i=0;i<MAX_PICE;i++) if(Pice[i].type==NORMAL_ROOM) assert(!game_fog()[i]);
    /* A discarded/failed dungeon cannot replace the valid recovery file. */
    game_discard_dungeon(); assert(!map_ready && !capture(&snapshot));
    assert(!write_game(recovery_path,1));
    again=read_game(previous_path); assert(again); game_data_free(again);
    for(i=0;i<hero_count;i++) assert(heroes[i].x==-1 && heroes[i].y==-1);
    free(bytes); game_shutdown(); mem_freeall();
    puts("PASS: exact dungeon/stat/fog round trip, portable restore, corrupt-file rejection,");
    puts("atomic saves, previous recovery, occupancy, reserve, undo, party UI and next-dungeon preservation.");
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='hq-game-test-') as temp:
    work = Path(temp)
    (work / 'check.c').write_text(HARNESS)
    exclude = {'main.obj', 'maindm.obj', 'winddm.obj', 'game.obj'}
    objects = [str(p) for p in (ROOT / 'obj/msvc').glob('*.obj') if p.name not in exclude]
    command = ['cl', '/nologo', '/D_CRT_SECURE_NO_WARNINGS', '/I'+str(ROOT),
               '/I'+str(ROOT/'my_lib'), '/I'+str(ROOT/'my_lib/windows'), '/I'+str(ROOT/'rsh'),
               'check.c', '/Fe:check.exe', *objects, str(ROOT/'obj/msvc/menu.res'), str(ROOT/'obj/msvc/grafic.res'),
               'user32.lib', 'gdi32.lib', 'shell32.lib', 'comdlg32.lib', 'version.lib', 'winspool.lib', 'ole32.lib', 'advapi32.lib']
    subprocess.run(command, cwd=work, check=True)
    subprocess.run([str(work/'check.exe'), str(ROOT/'tables'), str(work), str(ROOT/'obj/party-preview.bmp')], cwd=work, check=True, timeout=60)
