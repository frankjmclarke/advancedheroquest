"""Exercise view persistence through the actual menu/load/settings code and Win32 windows.
Profile I/O is isolated in memory so the user's settings are never touched.
Run after build-msvc.bat in an x86 MSVC developer shell.
"""
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
HARNESS = r'''
#include <assert.h>
#include <windows.h>
static int recovery_dialogs;
static int WINAPI recovery_message(HWND hwnd,LPCSTR text,LPCSTR title,UINT flags) {
    recovery_dialogs++; return IDNO;
}
#define MessageBoxA recovery_message
#include "game.c"
#undef MessageBoxA
#include <profile.h>
static _BOOL saved_player;
static void profile_file(const _UBYTE *name,const _UBYTE *author) {}
static _BOOL profile_bool(const _UBYTE *section,const _UBYTE *key,_BOOL def,_BOOL *out) {
    *out=!strcmp(section,"Show") && !strcmp(key,"PlayerView")?saved_player:def; return TRUE;
}
static _BOOL profile_int(const _UBYTE *section,const _UBYTE *key,_WORD def,_WORD *out) { *out=def; return FALSE; }
static _BOOL profile_string(const _UBYTE *section,const _UBYTE *key,const _UBYTE *def,_UBYTE *out,size_t n) {
    if(out!=def) { strncpy(out,def,n-1); out[n-1]=0; } return FALSE;
}
static void profile_write_bool(const _UBYTE *section,const _UBYTE *key,_BOOL value) {
    if(!strcmp(section,"Show") && !strcmp(key,"PlayerView")) saved_player=value;
}
static void profile_write_int(const _UBYTE *section,const _UBYTE *key,_WORD value) {}
static void profile_write_string(const _UBYTE *section,const _UBYTE *key,const _UBYTE *value) {}
static void profile_delete(const _UBYTE *section) {}
#define Profile_UseFile profile_file
#define Profile_ReadBool profile_bool
#define Profile_ReadInt profile_int
#define Profile_ReadString profile_string
#define Profile_WriteBool profile_write_bool
#define Profile_WriteInt profile_write_int
#define Profile_WriteString profile_write_string
#define Profile_DeleteSection profile_delete
#define WindFormMain application_main
#include "main.c"
#undef WindFormMain
#undef assert
#define assert(x) do { if(!(x)) { fprintf(stderr,"Assertion failed: %s, line %d\n",#x,__LINE__); exit(3); } } while(0)
static char *table_path,*save_dir;
static int player_on_top(void) {
    WINDOW_DEF *top=Wind_Top(); MAP_HOOK *hook;
    if(!top) return 0;
    hook=(MAP_HOOK*)GetPropA(W_GetHwnd(top),"HQHeroMap"); return hook && hook->player;
}
static void settings(void) {
    AHQ_para.x=52; AHQ_para.y=32; AHQ_para.von_x=26; AHQ_para.von_y=16; AHQ_para.eingang=North;
    AHQ_para.rnd=1234; AHQ_para.treppe=TRUE; AHQ_para.weiter=TRUE; AHQ_para.ask_exit=FALSE;
    strcpy(AHQ_para.name,"View persistence test");
    Wind_Menu_Check(MGRAFIK,TRUE); karte_ok(TRUE);
}

static void empty_session(void) {
    ready=0; active=0; dirty=0;
    close_all_windows(FALSE); game_discard_dungeon();
    game_data_free(loaded_owner); loaded_owner=NULL;
    memset(&monsters,0,sizeof(monsters)); hero_count=0; recovery_checked=0;
}
static int generate_accepted;
static BOOL CALLBACK accept_generate(HWND hwnd,LPARAM timer) {
    char title[80]; GetWindowTextA(hwnd,title,sizeof(title));
    if(!strcmp(title,"Generate Map")) {
        KillTimer(NULL,(UINT_PTR)timer); generate_accepted=1;
        PostMessage(hwnd,WM_COMMAND,MAKEWPARAM(KOK,BN_CLICKED),0); return FALSE;
    }
    return TRUE;
}
static void CALLBACK generate_timer(HWND hwnd,UINT msg,UINT_PTR timer,DWORD time) {
    EnumThreadWindows(GetCurrentThreadId(),accept_generate,(LPARAM)timer);
}
static void startup_recovery_checks(void) {
    int i,x,y,room=-1,count; FILE *fp; _LONG value=0; PICE *p;
    int stats[9]={5,5,5,5,5,5,5,4,1};
    ready=0;
    for(i=0;i<MAX_PICE;i++) if(Pice[i].type==NORMAL_ROOM) { room=i; break; }
    assert(room>=0); game_fog()[room]=1; monsters_reveal();
    monster_add_reference("recovery beast",stats,1,&room);
    count=monsters.count; assert(count>0); monsters.tokens[count-1].wounds=2;
    strcpy(monsters.dead[monsters.dead_count++],"fallen lord");
    hero_count=1; hero_defaults(&heroes[0],0,1); strcpy(heroes[0].name,"Recovery hero"); heroes[0].wounds=2;
    for(y=0;y<Ysize && heroes[0].x<0;y++) for(x=0;x<Xsize && heroes[0].x<0;x++)
        if(can_move(0,x,y,game_fog())) { heroes[0].x=x; heroes[0].y=y; }
    assert(heroes[0].x>=0); show_player_view(session_title); assert(game_player_view());
    assert(write_game(recovery_path,0)); assert(CopyFileA(recovery_path,previous_path,FALSE));
    empty_session(); startup_started=FALSE;
    strcpy(AHQ_para.tabelle.path,table_path); set_filename(&AHQ_para.tabelle,"sonne2.tab");
    /* Actual startup callback: load exact game before generating/displaying a new map. */
    assert(WindPos_Save_Restore(FALSE,0,&value));
    assert(recovery_dialogs==0 && hero_count==1 && !strcmp(heroes[0].name,"Recovery hero") && heroes[0].wounds==2);
    assert(monsters.count==count && monsters.tokens[count-1].wounds==2 && character_dead("fallen lord"));
    assert(game_fog()[room] && game_player_view() && player_on_top());
    heroes[0].wounds=1;
    assert(WindPos_Save_Restore(FALSE,0,&value)); assert(heroes[0].wounds==1 && monsters.count==count);
    assert(game_recover() && heroes[0].wounds==1 && recovery_dialogs==0);
    recovery_checked=0; assert(game_recover() && heroes[0].wounds==1 && recovery_dialogs==0);
    startup_started=FALSE; recovery_checked=0;
    assert(WindPos_Save_Restore(FALSE,0,&value));
    assert(heroes[0].wounds==1 && Wind_Menu_Enabled(MGAMENEXT) && recovery_dialogs==0);
    /* Generate Map's actual dialog and menu must clear recovered tokens. */
    ready=0; active=0; generate_accepted=0;
    { UINT_PTR timer=SetTimer(NULL,0,20,generate_timer); assert(timer);
      assert(do_menu(MKARTE)); KillTimer(NULL,timer); }
    assert(generate_accepted && monsters.count==0 && hero_count==1 && heroes[0].x==-1 && heroes[0].y==-1);
    assert(player_on_top() && character_dead("fallen lord"));
    game_changed(); game_command(MGAMEUNDO); assert(monsters.count==0 && heroes[0].x==-1);
    {
        GAME_DATA snapshot,*empty; unsigned char *bytes; size_t size;
        assert(capture(&snapshot)); assert(game_encode(&snapshot,&bytes,&size)); free(snapshot.cells);
        empty=game_decode(bytes,size); assert(empty && empty->monsters.count==0 && empty->heroes[0].x==-1);
        game_data_free(empty); free(bytes);
    }
    /* Corrupt latest recovery falls back to the previous generation silently. */
    empty_session(); fp=fopen(recovery_path,"wb"); assert(fp); fputs("broken",fp); fclose(fp);
    assert(game_recover()); assert(heroes[0].wounds==2 && monsters.count==count && recovery_dialogs==0);
    /* Missing/unusable recovery permits normal map generation, with no dialog. */
    empty_session(); fp=fopen(previous_path,"wb"); assert(fp); fputs("broken",fp); fclose(fp);
    assert(!game_recover() && !map_ready && recovery_dialogs==0);
    assert(DeleteFileA(recovery_path)); assert(DeleteFileA(previous_path));
    recovery_checked=0; assert(!game_recover() && recovery_dialogs==0);
    active=0; assert(do_menu(MGAMENEXT)); assert(map_ready);
}

static int party_action,party_accepted;
static BOOL CALLBACK accept_party(HWND hwnd,LPARAM timer) {
    char title[80]; GetWindowTextA(hwnd,title,sizeof(title));
    if(!strcmp(title,"Party - heroes and reserve")) {
        KillTimer(NULL,(UINT_PTR)timer); party_accepted=1;
        PostMessage(hwnd,WM_COMMAND,MAKEWPARAM(party_action,BN_CLICKED),0); return FALSE;
    }
    return TRUE;
}
static void CALLBACK party_timer(HWND hwnd,UINT msg,UINT_PTR timer,DWORD time) {
    EnumThreadWindows(GetCurrentThreadId(),accept_party,(LPARAM)timer);
}
static void open_party_action(int action) {
    UINT_PTR timer; party_action=action; party_accepted=0;
    timer=SetTimer(NULL,0,20,party_timer); assert(timer); game_party(); KillTimer(NULL,timer); assert(party_accepted);
}
static void party_placement_checks(void) {
    int player,x,y,px,py,found,hidden,scale; HWND hwnd; WINDOW_DEF *target; L_GRECT doc; RECT client;
    hero_count=1; hero_defaults(&heroes[0],0,1);
    for(player=0;player<2;player++) {
        if(player) show_player_view(session_title); else Wind_On_Top(Grafik_Karte);
        target=map_view_window(player); hwnd=W_GetHwnd(target);
        open_party_action(GPMOVE);
        assert(game_player_view()==player && Wind_Top()==target && GetFocus()==hwnd && move_mode && selected==0);
        SetWindowPos(hwnd,NULL,0,0,1000,800,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
        Grafik_Zoom_Fit(); Wind_GetDoc(target,&doc); GetClientRect(hwnd,&client); scale=8*Grafik_Zoom_Get();
        hidden=found=0;
        for(y=0;y<Ysize;y++) for(x=0;x<Xsize;x++) {
            px=(x+1)*scale+scale/2-doc.xx; py=(Ysize-y)*scale+scale/2-doc.yy;
            if(px<0 || py<0 || px>=client.right || py>=client.bottom) continue;
            if(player && !hidden && can_move(0,x,y,NULL) && !can_move(0,x,y,game_fog())) {
                SendMessage(hwnd,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(px,py));
                SendMessage(hwnd,WM_LBUTTONUP,0,MAKELPARAM(px,py));
                assert(heroes[0].x==-1 && move_mode); hidden=1;
            }
        }
        if(player) assert(hidden);
        for(y=0;y<Ysize && !found;y++) for(x=0;x<Xsize && !found;x++) {
            px=(x+1)*scale+scale/2-doc.xx; py=(Ysize-y)*scale+scale/2-doc.yy;
            if(px<0 || py<0 || px>=client.right || py>=client.bottom || !can_move(0,x,y,game_fog())) continue;
            SendMessage(hwnd,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(px,py));
            SendMessage(hwnd,WM_LBUTTONUP,0,MAKELPARAM(px,py));
            assert(heroes[0].x==x && heroes[0].y==y && !move_mode); found=1;
        }
        assert(found); heroes[0].x=heroes[0].y=-1;
    }
    /* A player-only layout must not create a GM window when opening Party. */
    Wind_Close(Grafik_Karte); assert(!Grafik_Karte); open_party_action(GPMOVE);
    assert(!Grafik_Karte && game_player_view() && player_on_top());
    move_mode=0; open_party_action(IDOK); assert(!Grafik_Karte && player_on_top());
}
static void view_checks(void) {
    GAME_DATA *d; _LONG value=0; int i;
    assert(init_icons()); assert(mem_alloc(100,100,1000,4096)); heap_clear();
    read_profile(); settings();
    assert(SetCurrentDirectoryA(table_path)); assert(read_table("sonne2.tab",NULL));
    assert(do_menu(MGAMENEXT)); assert(Grafik_Karte && !game_player_view());
    assert(do_menu(MPLAYER)); assert(game_player_view() && player_on_top());
    party_placement_checks();
    /* The actual Next Map path, including mass close and GM bitmap creation. */
    active=0; assert(do_menu(MWEITER));
    assert(game_player_view() && player_on_top());
    for(i=0;i<MAX_PICE;i++) if(Pice[i].type==NORMAL_ROOM) assert(!game_fog()[i]);
    assert(SetCurrentDirectoryA(save_dir));
    GetFullPathNameA("player.hqg",MAX_PATH,save_path,NULL);
    assert(game_save(0)); strcpy(recovery_path,save_path);
    Wind_On_Top(Grafik_Karte); assert(!game_player_view() && Wind_Top()==Grafik_Karte);
    active=0; assert(game_load(1)); assert(game_player_view() && player_on_top());
    /* GM saves likewise restore GM even if Player View was used afterward. */
    Wind_On_Top(Grafik_Karte); assert(!game_player_view());
    GetFullPathNameA("gm.hqg",MAX_PATH,save_path,NULL); assert(game_save(0)); strcpy(recovery_path,save_path);
    assert(do_menu(MPLAYER)); assert(game_player_view());
    active=0; assert(game_load(1)); assert(!game_player_view() && Wind_Top()==Grafik_Karte);
    /* View-only changes update recovery, with no token movement required. */
    GetFullPathNameA("recovery.hqg",MAX_PATH,recovery_path,NULL);
    GetFullPathNameA("previous.hqg",MAX_PATH,previous_path,NULL);
    ready=1; active=1; assert(do_menu(MPLAYER));
    d=read_game(recovery_path); assert(d && d->player_view==1); game_data_free(d);
    Wind_On_Top(Grafik_Karte); d=read_game(recovery_path); assert(d && d->player_view==0); game_data_free(d);
    assert(do_menu(MPLAYER));
    /* Accepted shutdown freezes focus before closing windows. Startup reads
       that setting, and legacy window restoration cannot put GM above it. */
    active=0; ready=0; AHQ_para.ask_exit=FALSE;
    assert(WindPos_Save_Restore(TRUE,0,&value)); close_all_windows(FALSE); write_profile(); assert(saved_player);
    preferred_player_view=0; view_pause=0; read_profile(); settings(); assert(game_player_view());
    active=0; assert(do_menu(MGAMENEXT)); assert(player_on_top());
    assert(WindPos_Save_Restore(FALSE,W_GRAFIK,&value)); assert(game_player_view() && player_on_top());
    assert(WindPos_Save_Restore(FALSE,W_PLAYER,&value)); assert(player_on_top());
    Wind_On_Top(Grafik_Karte); assert(!game_player_view()); active=0; assert(do_menu(MGAMENEXT));
    assert(!game_player_view() && Wind_Top()==Grafik_Karte);
    startup_recovery_checks();
    close_all_windows(FALSE); game_shutdown(); free_icons(); mem_freeall();
    puts("PASS: Player/GM view focus, Next Map, both save/load directions, view-only recovery, shutdown/profile restart, restored-window order, automatic recovery, duplicate startup and corrupt/missing recovery.");
}
static size_t window_settings(_BOOL save,WIND_RESTORE *buf,size_t count) { return 1; }
static _BOOL window_restore(_BOOL save,_WORD group,_LONG *value) {
    if(!save && group==0) { view_checks(); PostQuitMessage(0); } return TRUE;
}
_WORD WindFormMain(_WORD argc,const _UBYTE **argv) {
    GlCmdShow=SW_HIDE; assert(Wind_Hide_Show_Init(window_settings,window_restore)); Evnt_Multi(NULL,MENUE); return 0;
}
int WINAPI WinMain(HINSTANCE,HINSTANCE,LPSTR,int);
int main(int argc,char **argv) {
    table_path=argv[1]; save_dir=argv[2]; SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    return WinMain(GetModuleHandle(NULL),NULL,"",SW_HIDE);
}
'''
with tempfile.TemporaryDirectory(prefix='hq-view-test-') as temp:
    work=Path(temp)
    (work/'check.c').write_text(HARNESS)
    objects=[str(p) for p in (ROOT/'obj/msvc').glob('*.obj') if p.name not in {'main.obj','maindm.obj','winddm.obj','game.obj'}]
    subprocess.run(['cl','/nologo','/D_CRT_SECURE_NO_WARNINGS','/I'+str(ROOT),'/I'+str(ROOT/'my_lib'),
                    '/I'+str(ROOT/'my_lib/windows'),'/I'+str(ROOT/'rsh'),'check.c','/Fe:check.exe',*objects,
                    str(ROOT/'obj/msvc/menu.res'),str(ROOT/'obj/msvc/grafic.res'),
                    'user32.lib','gdi32.lib','shell32.lib','comdlg32.lib','version.lib','winspool.lib','ole32.lib','advapi32.lib'],cwd=work,check=True)
    subprocess.run([str(work/'check.exe'),str(ROOT/'tables'),temp],cwd=work,check=True,timeout=60)
