"""Load saved games through real Win32 map windows and bitmap resources."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
HARNESS = r'''
#include <assert.h>
#include <stdarg.h>
#include <windows.h>
#include "game.c"
#include <mem.h>
#include <table.h>
#include <makemap.h>
#include <random.h>
#include <icon.h>
_UBYTE ProgramName[]="Saved game rendering test";
static char *table_path,*save_dir;
_BOOL error_abort(const char *fmt,...) { fprintf(stderr,"Map error: %s\n",fmt); exit(2); }
void game_loaded_title(const char *title) {
    show_grafic((char*)title);
    assert(Grafik_Karte);
    Grafik_Zoom_Fit();
    UpdateWindow(W_GetHwnd(Grafik_Karte));
}
static int render_checks(void) {
    int i,mode; PICE *p; MFDB *bitmap;
    fprintf(stderr,"Initialize artwork\n"); assert(init_icons());
    assert(mem_alloc(100,100,1000,4096)); heap_clear();
    assert(SetCurrentDirectoryA(table_path)); assert(read_table("sonne2.tab",NULL));
    new_rand(1234); assert(makemap(52,32,26,16,North));
    game_new_dungeon("Render round trip");
    hero_count=2; hero_defaults(&heroes[0],0,1); hero_defaults(&heroes[1],3,2);
    for(i=0;i<Xsize*Ysize;i++) {
        if(can_move(0,i%Xsize,i/Xsize,NULL)) { heroes[0].x=i%Xsize; heroes[0].y=i/Xsize; break; }
    }
    assert(heroes[0].x>=0);
    {
        static char *encounter[]={"#42","2 Goblins, 1 Orc (20 Gold Crowns)",NULL};
        for(i=0;i<MAX_PICE;i++) if(Pice[i].type==NORMAL_ROOM) break;
        assert(i<MAX_PICE); Pice[i].text=encounter; game_fog()[i]=1; game_changed();
        assert(monsters.seen[i] && monsters.count>=3);
    }
    assert(SetCurrentDirectoryA(save_dir));
    GetFullPathNameA("render.hqg",MAX_PATH,recovery_path,NULL);
    assert(write_game(recovery_path,0));
    fprintf(stderr,"Render generated map\n"); show_grafic(session_title); assert(Grafik_Karte);
    assert(toolbar_hwnd && toolbar_images && ImageList_GetImageCount(toolbar_images)==7);
    {
        TBBUTTON button; RECT rect;
        assert((HIMAGELIST)SendMessage(toolbar_hwnd,TB_GETIMAGELIST,0,0)==toolbar_images);
        assert(SendMessage(toolbar_hwnd,TB_BUTTONCOUNT,0,0)==4);
        assert(SendMessage(toolbar_hwnd,TB_GETBUTTON,0,(LPARAM)&button) && button.iBitmap==3);
        assert(SendMessage(toolbar_hwnd,TB_GETITEMRECT,0,(LPARAM)&rect) && rect.right-rect.left>=48 && rect.bottom-rect.top>=48);
        fprintf(stderr,"Toolbar item width=%ld height=%ld image=%d\n",rect.right-rect.left,rect.bottom-rect.top,button.iBitmap);
    }
    /* Repeated load closes existing windows and reopens the loaded map.
       Exercise both default and enhanced graphics and player fog rendering. */
    for(mode=0;mode<2;mode++) {
        fprintf(stderr,"Load and render saved map, enhanced=%d\n",mode); enhanced_view=mode; active=0; assert(game_load(1));
        assert(monsters.count>=3);
        assert(hero_count==2 && heroes[0].x>=0 && heroes[1].x==-1);
        UpdateWindow(W_GetHwnd(Grafik_Karte));
        show_player_view(session_title);
        game_redraw();
    }
    /* Renderer also tolerates empty lists and blank first lines supplied
       independently of the codec. Neither has a room number to draw. */
    for(i=0;i<MAX_PICE;i++) if(Pice[i].type==NORMAL_ROOM) break;
    assert(i<MAX_PICE); p=&Pice[i];
    {
        char *no_lines[]={NULL},*blank_line[]={"",NULL}; char **original=p->text;
        bitmap=get_mfdb((Xsize+2)*8,(Ysize+2)*8,GetNumPlanes(),NULL); assert(bitmap);
        p->text=no_lines; assert(draw_img_map(bitmap));
        p->text=blank_line; assert(draw_img_map(bitmap));
        p->text=original; free_mfdb(bitmap);
    }
    close_all_windows(FALSE); game_shutdown(); free_icons(); mem_freeall();
    puts("PASS: repeated saved-game load into real GM/player windows, both graphic modes, and empty room text.");
    return 0;
}
static size_t window_settings(_BOOL save,WIND_RESTORE *buf,size_t count) { return 1; }
static _BOOL window_restore(_BOOL save,_WORD group,_LONG *value) {
    if(!save && group==0) { render_checks(); PostQuitMessage(0); }
    return TRUE;
}
_WORD WindFormMain(_WORD argc,const _UBYTE **argv) {
    GlCmdShow=SW_HIDE;
    assert(Wind_Hide_Show_Init(window_settings,window_restore));
    Evnt_Multi(NULL,0);
    return 0;
}
int WINAPI WinMain(HINSTANCE,HINSTANCE,LPSTR,int);
int main(int argc,char **argv) {
    table_path=argv[1]; save_dir=argv[2];
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    return WinMain(GetModuleHandle(NULL),NULL,"",SW_HIDE);
}
'''
with tempfile.TemporaryDirectory(prefix="hq-render-test-") as temp:
    work = Path(temp)
    (work / "check.c").write_text(HARNESS)
    exclude = {"main.obj", "maindm.obj", "winddm.obj", "game.obj"}
    objects = [str(p) for p in (ROOT / "obj/msvc").glob("*.obj") if p.name not in exclude]
    subprocess.run([
        "cl", "/nologo", "/D_CRT_SECURE_NO_WARNINGS", "/I"+str(ROOT),
        "/I"+str(ROOT/"my_lib"), "/I"+str(ROOT/"my_lib/windows"), "/I"+str(ROOT/"rsh"),
        "check.c", "/Fe:check.exe", *objects,
        str(ROOT/"obj/msvc/menu.res"), str(ROOT/"obj/msvc/grafic.res"),
        "user32.lib", "gdi32.lib", "shell32.lib", "comdlg32.lib", "version.lib",
        "winspool.lib", "ole32.lib", "advapi32.lib", "comctl32.lib",
    ], cwd=work, check=True)
    subprocess.run([str(work/"check.exe"), str(ROOT/"tables"), temp],
                   cwd=work, check=True, timeout=60)
