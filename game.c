/* Native party UI, token interaction and durable session files. */
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <commdlg.h>
#include <wincrypt.h>
#include <shlobj.h>
#include <openwork.h>
#include <wind.h>
#include <defs.h>
#include "game.h"
#include "liste.h"
#include "pack.h"
#include "rsh/game.rh"

static MONSTER_STATE monsters,undo_monsters;
static int selected_monster=-1,preview_monster=-1,attack_monster=-1;
static void monsters_reveal(void);
static void focus_prune(void);
static void focus_enter(int side,int index);
static int battle_destination(int side,int index,int *x,int *y);
static int shot_h=-1,shot_m=-1;
static int monster_at(int x,int y);
static HERO heroes[HERO_LIMIT], undo_heroes[HERO_LIMIT];
static int preferred_player_view,view_pause,recovery_checked;
static int hero_count,undo_count,undo_valid,selected=-1,move_mode,active,dirty,ready,autosave_warned,map_ready;
static char session_title[160],save_path[MAX_PATH],recovery_path[MAX_PATH],previous_path[MAX_PATH];
static GAME_DATA *loaded_owner;
static WINDOW_DEF *preview_window;
static int preview_hero=-1,preview_x,preview_y;
extern void game_loaded_title(const char *title);
static void message(const char *text) { MessageBoxA(GlMainHwnd,text,"HQ-Map game",MB_OK|MB_ICONINFORMATION); }
static void remember(void) { memcpy(undo_heroes,heroes,sizeof(heroes)); undo_count=hero_count; undo_monsters=monsters; undo_valid=1; }
static int capture(GAME_DATA *d)
{
    int x,y; PICE *p;
    memset(d,0,sizeof(*d)); if(!map_ready || !Pice || Xsize<1 || Ysize<1) return 0;
    d->pieces=Pice; d->count=MAX_PICE; d->width=Xsize; d->height=Ysize;
    d->visible=game_fog(); if(!d->visible) return 0;
    d->player_view=preferred_player_view;
    d->pack=(CHARACTER_PACK*)pack_current();
    d->monsters=monsters;
    d->hero_count=hero_count; memcpy(d->heroes,heroes,sizeof(heroes)); strcpy(d->title,session_title);
    d->cells=(int*)malloc(sizeof(int)*Xsize*Ysize); if(!d->cells) return 0;
    for(y=0;y<Ysize;y++) for(x=0;x<Xsize;x++) {
        get_square(x,y,&p); d->cells[y*Xsize+x]=p?(int)(p-Pice):-1;
    }
    return 1;
}
/* Durable replacement: write and flush a unique sibling before replacing the
 * destination. The prior autosave is kept as a second recovery generation. */
static int write_game(const char *path,int backup)
{
    GAME_DATA d; unsigned char *bytes=NULL; size_t size=0;
    HANDLE file; DWORD written; int ok=0; char temp[MAX_PATH];
    if(!*path || !capture(&d)) return 0;
    ok=game_encode(&d,&bytes,&size); free(d.cells); if(!ok) return 0;
    if(strlen(path)+48>=MAX_PATH) { free(bytes); return 0; }
    sprintf(temp,"%s.%lu.%lu.tmp",path,GetCurrentProcessId(),GetTickCount());
    file=CreateFileA(temp,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE) { free(bytes); return 0; }
    ok=WriteFile(file,bytes,(DWORD)size,&written,NULL) && written==size && FlushFileBuffers(file);
    if(!CloseHandle(file)) ok=0; free(bytes);
    if(ok && backup && GetFileAttributesA(path)!=INVALID_FILE_ATTRIBUTES)
        ok=CopyFileA(path,previous_path,FALSE)!=0;
    if(ok) ok=MoveFileExA(temp,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
    if(!ok) DeleteFileA(temp); return ok;
}
static GAME_DATA *read_game(const char *path)
{
    FILE *fp=fopen(path,"rb"); long length; unsigned char *bytes; GAME_DATA *d=NULL;
    if(!fp) return NULL;
    if(fseek(fp,0,SEEK_END) || (length=ftell(fp))<32 || length>16*1024*1024) { fclose(fp); return NULL; }
    rewind(fp); bytes=(unsigned char*)malloc(length);
    if(bytes && fread(bytes,1,length,fp)==(size_t)length) d=game_decode(bytes,length);
    free(bytes); fclose(fp); return d;
}
void game_initialize(void)
{
    char folder[MAX_PATH]; ready=0; recovery_checked=0;
    if(SHGetFolderPathA(NULL,CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE,NULL,0,folder)==S_OK && strlen(folder)<MAX_PATH-50) {
        strcat(folder,"\\HQ-Map"); CreateDirectoryA(folder,NULL);
        sprintf(recovery_path,"%s\\recovery.hqg",folder);
        sprintf(previous_path,"%s\\recovery-previous.hqg",folder);
    }
}
int game_player_view(void) { return preferred_player_view; }
/* Freeze focus tracking while windows are being replaced or closed together. */
void game_view_pause(int pause)
{
    if(pause) view_pause++; else if(view_pause) view_pause--;
}
void game_view_selected(int player)
{
    player=player!=0;
    if(view_pause || preferred_player_view==player) return;
    preferred_player_view=player;
    /* Include a view-only switch in recovery even without moving a token. */
    if(ready && active && map_ready) game_changed();
}
void game_restore_view(void)
{
    if(!map_ready) return;
    game_view_pause(1);
    if(preferred_player_view) show_player_view(session_title);
    else if(Grafik_Karte) Wind_On_Top(Grafik_Karte);
    game_view_pause(0);
}
void game_changed(void)
{
    monsters_reveal();
    focus_prune();
    if(!active || !map_ready) return;
    dirty=1; game_redraw();
    if(ready && !write_game(recovery_path,1) && !autosave_warned) {
        autosave_warned=1; message("Autosave could not be written. Use File > Save Game to choose a writable location.");
    }
}
static int choose_path(char *path,int save)
{
    OPENFILENAMEA dialog; memset(&dialog,0,sizeof(dialog));
    dialog.lStructSize=sizeof(dialog); dialog.hwndOwner=GlMainHwnd;
    dialog.lpstrFilter="HQ-Map saved games (*.hqg)\0*.hqg\0\0";
    dialog.lpstrFile=path; dialog.nMaxFile=MAX_PATH; dialog.lpstrDefExt="hqg";
    dialog.Flags=OFN_EXPLORER|OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);
    return save?GetSaveFileNameA(&dialog):GetOpenFileNameA(&dialog);
}
int game_save(int save_as)
{
    char path[MAX_PATH];
    if(!map_ready) { message("Generate or load a dungeon first. Your recovery copy is unchanged."); return 0; }
    strcpy(path,save_path);
    if((save_as || !*path) && !choose_path(path,1)) return 0;
    if(!write_game(path,0)) { message("Could not save the game. Your existing save has not been replaced."); return 0; }
    strcpy(save_path,path); active=1; dirty=0;
    if(ready && !write_game(recovery_path,1) && !autosave_warned) { autosave_warned=1; message("Game saved, but the separate recovery copy could not be updated."); }
    return 1;
}
int game_before_replace(void)
{
    int choice;
    if(!active || !dirty) return 1;
    choice=MessageBoxA(GlMainHwnd,"Save your current game before continuing?", "HQ-Map game",MB_YESNOCANCEL|MB_ICONQUESTION);
    if(choice==IDCANCEL) return 0;
    return choice==IDYES?game_save(0):1;
}
int game_load(int recovery)
{
    char path[MAX_PATH]; GAME_DATA *d; CHARACTER_PACK *next_pack;
    if(!recovery) { path[0]=0; if(!choose_path(path,0)) return 0; }
    else strcpy(path,recovery_path);
    d=read_game(path);
    if(!d && recovery) d=read_game(previous_path);
    if(!d) { if(recovery!=2) message("This saved game is damaged, unsupported, or unavailable. The current game has not changed."); return 0; }
    if(recovery!=2 && !game_before_replace()) { game_data_free(d); return 0; }
    next_pack=pack_clone(d->pack);
    if(!next_pack) { game_data_free(d); message("Not enough memory for character profiles."); return 0; }
    /* map_restore allocates first, so even allocation failure preserves play. */
    if(!map_restore(d->pieces,d->count,d->width,d->height,d->cells)) { pack_free(next_pack); game_data_free(d); if(recovery!=2) message("Not enough memory to load the game."); return 0; }
    pack_adopt(next_pack);
    close_all_windows(FALSE);
    game_data_free(loaded_owner); loaded_owner=d;
    if(d->player_view>=0) preferred_player_view=d->player_view;
    monsters=d->monsters; selected_monster=-1; attack_monster=-1;
    hero_count=d->hero_count; memcpy(heroes,d->heroes,sizeof(heroes));
    { int i; for(i=0;i<hero_count;i++) if(heroes[i].kind==4 && !heroes[i].wounds) heroes[i].x=heroes[i].y=-1; }
    strcpy(session_title,d->title); game_set_fog(d->visible,d->count);
    selected=-1; move_mode=0; undo_valid=0; active=1; map_ready=1; dirty=recovery?1:0;
    if(recovery) save_path[0]=0; else strcpy(save_path,path);
    monsters_reveal(); focus_prune();
    game_view_pause(1);
    game_loaded_title(session_title);
    game_view_pause(0);
    game_restore_view();
    return 1;
}
int game_has_loaded_session(void) { return map_ready && loaded_owner!=NULL; }
int game_recover(void)
{
    int restored=0;
    /* Window restoration may be re-entered while a modal dialog pumps messages.
       Restore at most once; existing live sessions always take precedence. */
    if(recovery_checked) return map_ready;
    recovery_checked=1;
    if(game_has_loaded_session()) { ready=1; return 1; }
    if(*recovery_path && (GetFileAttributesA(recovery_path)!=INVALID_FILE_ATTRIBUTES || GetFileAttributesA(previous_path)!=INVALID_FILE_ATTRIBUTES)) {
        restored=game_load(2); /* automatic startup: no prompt, quiet fallback */
    }
    ready=1;
    return restored;
}
void game_discard_dungeon(void)
{
    int i; monsters.count=0; memset(monsters.tokens,0,sizeof(monsters.tokens));
    memset(monsters.seen,0,sizeof(monsters.seen)); selected_monster=-1; preview_monster=-1; attack_monster=-1; map_ready=0; dirty=0; selected=-1; move_mode=0; undo_valid=0; preview_hero=-1;
    for(i=0;i<hero_count;i++) { heroes[i].x=heroes[i].y=-1; heroes[i].fired=heroes[i].focus=heroes[i].moved=0; }
    game_reset_fog();
}
void game_new_dungeon(const char *title)
{
    int i;
    if(!pack_start_dungeon()) { map_ready=0; message("Not enough memory for character profiles."); return; }
    map_ready=1; active=1;
    game_data_free(loaded_owner); loaded_owner=NULL;
    game_reset_fog();
    monsters.count=0; memset(monsters.tokens,0,sizeof(monsters.tokens));
    memset(monsters.seen,0,sizeof(monsters.seen)); selected_monster=-1; preview_monster=-1; attack_monster=-1;
    strncpy(session_title,title,sizeof(session_title)-1); session_title[sizeof(session_title)-1]=0;
    for(i=0;i<hero_count;i++) { heroes[i].x=heroes[i].y=-1; heroes[i].fired=heroes[i].focus=heroes[i].moved=0; }
    selected=-1; move_mode=0; undo_valid=0; preview_hero=-1;
    game_changed();
}
void game_shutdown(void)
{
    game_data_free(loaded_owner); loaded_owner=NULL;
    game_reset_fog();
    pack_shutdown();
}
static void reserve_party(void)
{
    int i; if(!hero_count) return; remember();
    for(i=0;i<hero_count;i++) { heroes[i].x=heroes[i].y=-1; heroes[i].fired=heroes[i].focus=heroes[i].moved=0; }
    move_mode=0; game_changed();
}
/* Keep token locations on occupied map squares, with strictly one hero each. */
static int can_move(int index,int x,int y,const unsigned char *visible)
{
    PICE *piece; int i;
    if(!get_square((_WORD)x,(_WORD)y,&piece) || !piece || piece->type==EMPTY || piece->type==TEST) return 0;
    if(visible && !fog_visible(visible,(_WORD)(piece-Pice))) return 0;
    for(i=0;i<hero_count;i++) if(i!=index && heroes[i].x==x && heroes[i].y==y) return 0;
    if(monster_at(x,y)>=0) return 0;
    if(index>=0 && heroes[index].kind==4 && !heroes[index].wounds) return 0;
    return 1;
}
static int move_hero(int index,int x,int y,const unsigned char *visible)
{
    if(index<0 || index>=hero_count || !can_move(index,x,y,visible)) { MessageBeep(MB_ICONWARNING); return 0; }
    if(heroes[index].x==x && heroes[index].y==y) return 1;
    if(!battle_destination(0,index,&x,&y)) { MessageBeep(MB_ICONWARNING); return 0; }
    remember(); if(heroes[index].x>=0) heroes[index].moved=1;
    heroes[index].x=x; heroes[index].y=y; focus_enter(0,index); game_changed(); return 1;
}
static int hero_at(int x,int y,const unsigned char *visible)
{
    int i; PICE *p;
    if(!get_square((_WORD)x,(_WORD)y,&p) || !p || (visible && !fog_visible(visible,(_WORD)(p-Pice)))) return -1;
    for(i=0;i<hero_count;i++) if(heroes[i].x==x && heroes[i].y==y) return i;
    return -1;
}
/* Small code-drawn class emblems stay sharp without growing the artwork atlas. */
static void marker(HDC dc,int x,int y,int size,int kind,int chosen,int defeated)
{
    static const COLORREF colours[HERO_CLASS_COUNT]={RGB(165,46,36),RGB(145,91,31),RGB(32,117,65),RGB(61,66,162),RGB(86,93,101),RGB(181,137,24),RGB(28,126,132),RGB(126,39,72),RGB(122,58,163)};
    int saved=SaveDC(dc),a=size/4,b=size*3/4,c=size/2;
    HPEN edge=CreatePen(PS_SOLID,chosen?3:1,chosen?RGB(255,220,65):RGB(245,234,203));
    HPEN ink=CreatePen(PS_SOLID,size>40?3:2,RGB(255,245,215));
    HBRUSH fill=CreateSolidBrush(colours[kind]); POINT pts[3];
    SelectObject(dc,edge); SelectObject(dc,fill); Ellipse(dc,x+1,y+1,x+size-1,y+size-1); SelectObject(dc,ink);
    if(kind==5) { /* holy symbol */
        Rectangle(dc,x+c-size/10,y+a,x+c+size/10+1,y+b);
        Rectangle(dc,x+a,y+c-size/10,x+b,y+c+size/10+1);
    } else if(kind==6) { /* dagger */
        MoveToEx(dc,x+a,y+b,NULL); LineTo(dc,x+b,y+a);
        MoveToEx(dc,x+a,y+c,NULL); LineTo(dc,x+c,y+b);
    } else if(kind==7) { /* crossed blades */
        MoveToEx(dc,x+a,y+a,NULL); LineTo(dc,x+b,y+b);
        MoveToEx(dc,x+b,y+a,NULL); LineTo(dc,x+a,y+b);
    } else if(kind==8) { /* staff and orb */
        MoveToEx(dc,x+c,y+c,NULL); LineTo(dc,x+c,y+b);
        Ellipse(dc,x+a,y+a,x+b,y+c+2);
    } else if(kind==3) { pts[0].x=x+c; pts[0].y=y+a; pts[1].x=x+a; pts[1].y=y+b; pts[2].x=x+b; pts[2].y=y+b; Polygon(dc,pts,3); }
    else if(kind==2) { Arc(dc,x+a,y+a,x+b,y+b,x+c,y+a,x+c,y+b); MoveToEx(dc,x+c,y+a,NULL); LineTo(dc,x+c,y+b); }
    else {
        MoveToEx(dc,x+c,y+a,NULL); LineTo(dc,x+c,y+b);
        if(kind==1) Rectangle(dc,x+a,y+a,x+b,y+c);
        else { MoveToEx(dc,x+a,y+c,NULL); LineTo(dc,x+b,y+c); MoveToEx(dc,x+c,y+a,NULL); LineTo(dc,x+c-3,y+a+5); }
    }
    if(defeated) { MoveToEx(dc,x+a,y+a,NULL); LineTo(dc,x+b,y+b); MoveToEx(dc,x+b,y+a,NULL); LineTo(dc,x+a,y+b); }
    RestoreDC(dc,saved); DeleteObject(edge); DeleteObject(ink); DeleteObject(fill);
}
#include "monster.inc"
#include "ranged-rules.inc"
#include "combat.inc"
#include "ranged.inc"

void game_draw(WINDOW_DEF *window,const unsigned char *visible,int zoom,int sx,int sy)
{
    int i,x,y,size=8*zoom,saved; HDC dc=W_GetDC(window); RECT label;
    if(!dc) return; saved=SaveDC(dc); SetBkMode(dc,OPAQUE); SetBkColor(dc,RGB(35,28,22)); SetTextColor(dc,RGB(255,249,221));
    SelectObject(dc,GetStockObject(DEFAULT_GUI_FONT));
    for(i=0;i<hero_count;i++) {
        HERO *h=&heroes[i]; if(h->x<0 || hero_at(h->x,h->y,visible)!=i) continue;
        x=((i==preview_hero && window==preview_window?preview_x:h->x)+1)*size-sx;
        y=(Ysize-(i==preview_hero && window==preview_window?preview_y:h->y))*size-sy;
        marker(dc,x,y,size,h->kind,i==selected,h->wounds==0);
        if(size>=24) { label.left=x-size/2; label.right=x+size+size/2; label.top=y+size; label.bottom=label.top+16;
            DrawTextA(dc,h->name,-1,&label,DT_CENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX); }
    }
    for(i=0;i<monsters.count;i++) {
        MONSTER *m=&monsters.tokens[i]; if(m->x<0 || monster_at(m->x,m->y)!=i) continue;
        x=((i==preview_monster && window==preview_window?preview_x:m->x)+1)*size-sx;
        y=(Ysize-(i==preview_monster && window==preview_window?preview_y:m->y))*size-sy; monster_draw(dc,x,y,size,i);
    }
    if(shot_h>=0 && shot_h<hero_count && shot_m>=0 && shot_m<monsters.count &&
       hero_at(heroes[shot_h].x,heroes[shot_h].y,visible)==shot_h && monster_at(monsters.tokens[shot_m].x,monsters.tokens[shot_m].y)==shot_m) {
        HPEN line=CreatePen(PS_DOT,1,RGB(255,215,80)); HGDIOBJ old=SelectObject(dc,line);
        MoveToEx(dc,(heroes[shot_h].x+1)*size+size/2-sx,(Ysize-heroes[shot_h].y)*size+size/2-sy,NULL);
        LineTo(dc,(monsters.tokens[shot_m].x+1)*size+size/2-sx,(Ysize-monsters.tokens[shot_m].y)*size+size/2-sy);
        SelectObject(dc,old); DeleteObject(line);
    }
    /* Exceptional encounter rosters stay discoverable without a reveal dialog. */
    for(i=0;i<MAX_PICE && i<ENCOUNTER_LIMIT;i++) if(monsters.seen[i] && fog_visible(game_fog(),i)) {
        int j,unplaced=0; char note[80]; PICE *p=&Pice[i];
        for(j=0;j<monsters.count;j++) if(monsters.tokens[j].room==i && monsters.tokens[j].wounds && monsters.tokens[j].x<0) unplaced++;
        if(monsters.seen[i]!=2 && !unplaced) continue;
        sprintf(note,"Monsters: %s%d unplaced",monsters.seen[i]==2?"review; ":"",unplaced);
        label.left=(p->x+1)*size-sx; label.top=(Ysize-p->y-p->h+1)*size-sy;
        label.right=label.left+p->w*size; label.bottom=label.top+16;
        SetTextColor(dc,RGB(255,214,120)); DrawTextA(dc,note,-1,&label,DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
    }
    RestoreDC(dc,saved);
}
typedef struct { WINDOW_DEF *window; WNDPROC previous; int player,drag,start_x,start_y,monster_drag; } MAP_HOOK;
static int map_point(MAP_HOOK *hook,LPARAM lp,int *x,int *y)
{
    L_GRECT doc; int px=(short)LOWORD(lp),py=(short)HIWORD(lp),zoom=Grafik_Zoom_Get(); RECT client;
    GetClientRect(W_GetHwnd(hook->window),&client);
    if(px<0 || py<0 || px>=client.right || py>=client.bottom || zoom<1) return 0;
    Wind_GetDoc(hook->window,&doc); px+=(int)doc.xx; py+=(int)doc.yy;
    *x=px/(8*zoom)-1; *y=Ysize-py/(8*zoom); return px>=0 && py>=0;
}
static LRESULT CALLBACK map_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
    MAP_HOOK *hook=(MAP_HOOK*)GetPropA(hwnd,"HQHeroMap"); int x,y,n,target;
    const unsigned char *visible;
    if(!hook) return DefWindowProc(hwnd,msg,wp,lp);
    /* Preserve activation, but do not let the legacy procedure eat a token
       click when focus returns from the Party dialog or another map window. */
    if(msg==WM_MOUSEACTIVATE && LOWORD(lp)==HTCLIENT && HIWORD(lp)==WM_LBUTTONDOWN && (move_mode || hero_count || monsters.count)) {
        LRESULT activation=CallWindowProc(hook->previous,hwnd,msg,wp,lp);
        if(activation==MA_ACTIVATEANDEAT) return MA_ACTIVATE;
        if(activation==MA_NOACTIVATEANDEAT) return MA_NOACTIVATE;
        return activation;
    }
    visible=hook->player?game_fog():NULL;
    if(msg==WM_SETCURSOR && move_mode && LOWORD(lp)==HTCLIENT) { SetCursor(LoadCursor(NULL,IDC_CROSS)); return TRUE; }
    if((msg==WM_LBUTTONDOWN || msg==WM_LBUTTONDBLCLK) && map_point(hook,lp,&x,&y) && (n=monster_at(x,y))>=0) {
        if(msg==WM_LBUTTONDOWN && selected>=0 && move_mode) { attack_open(hwnd,selected,n,0); return 0; }
        selected_monster=n; selected=-1; move_mode=0;
        if(msg==WM_LBUTTONDBLCLK) { attack_monster=-1; hook->monster_drag=-1; ReleaseCapture(); monster_damage(n); }
        else { attack_monster=n; hook->monster_drag=n; hook->start_x=x; hook->start_y=y; SetCapture(hwnd); game_redraw(); }
        return 0;
    }
    if(msg==WM_MOUSEMOVE && hook->monster_drag>=0) {
        if(map_point(hook,lp,&x,&y) && (target=hero_at(x,y,visible))>=0 && heroes[target].wounds && monsters.tokens[hook->monster_drag].wounds) {
            preview_monster=-1; game_redraw(); SetCursor(LoadCursor(NULL,IDC_CROSS));
        } else if(map_point(hook,lp,&x,&y) && monster_can_move(hook->monster_drag,x,y)) {
            if(preview_monster!=hook->monster_drag || preview_x!=x || preview_y!=y) {
                preview_monster=hook->monster_drag; preview_window=hook->window; preview_x=x; preview_y=y; game_redraw();
            }
            SetCursor(LoadCursor(NULL,IDC_SIZEALL));
        } else SetCursor(LoadCursor(NULL,IDC_NO));
        return 0;
    }
    if(msg==WM_LBUTTONUP && hook->monster_drag>=0) {
        n=hook->monster_drag; hook->monster_drag=-1; ReleaseCapture();
        if(map_point(hook,lp,&x,&y) && (x!=hook->start_x || y!=hook->start_y)) {
            if((target=hero_at(x,y,visible))>=0) attack_open(hwnd,target,n,1);
            else if(monster_can_move(n,x,y) && battle_destination(1,n,&x,&y)) {
                attack_monster=-1; remember(); monsters.tokens[n].moved=1; monsters.tokens[n].x=x; monsters.tokens[n].y=y;
                focus_enter(1,n); active=1; game_changed();
            }
            else MessageBeep(MB_ICONWARNING);
        }
        return 0;
    }
    if((msg==WM_RBUTTONDOWN || msg==WM_RBUTTONDBLCLK) && map_point(hook,lp,&x,&y) && monster_at(x,y)>=0) {
        attack_monster=-1; move_mode=0; ReleaseCapture(); return 0;
    }
    if(msg==WM_RBUTTONUP && map_point(hook,lp,&x,&y) && (n=monster_at(x,y))>=0) {
        selected_monster=n; game_monsters(); return 0;
    }
    if((msg==WM_LBUTTONDOWN || msg==WM_LBUTTONDBLCLK) && map_point(hook,lp,&x,&y)) {
        n=hero_at(x,y,visible);
        if(n>=0 && attack_monster>=0 && msg==WM_LBUTTONDOWN) { attack_open(hwnd,n,attack_monster,1); attack_monster=-1; return 0; }
        if(n>=0) { attack_monster=-1; selected_monster=-1; selected=n; move_mode=1; hook->drag=n; hook->start_x=x; hook->start_y=y; SetCapture(hwnd); game_redraw(); return 0; }
        if(move_mode && selected>=0) {
            if(move_hero(selected,x,y,visible)) { move_mode=0; SetCursor(LoadCursor(NULL,IDC_ARROW)); }
            return 0;
        }
    }
    if(msg==WM_MOUSEMOVE && hook->drag>=0) {
        if(map_point(hook,lp,&x,&y) && (target=monster_at(x,y))>=0 && heroes[hook->drag].wounds && monsters.tokens[target].wounds) {
            preview_hero=-1; game_redraw(); SetCursor(LoadCursor(NULL,IDC_CROSS));
        } else if(map_point(hook,lp,&x,&y) && can_move(hook->drag,x,y,visible)) {
            if(preview_hero!=hook->drag || preview_x!=x || preview_y!=y) {
                preview_window=hook->window; preview_hero=hook->drag; preview_x=x; preview_y=y; game_redraw();
            }
            SetCursor(LoadCursor(NULL,IDC_SIZEALL));
        } else SetCursor(LoadCursor(NULL,IDC_NO));
        return 0;
    }
    if(msg==WM_LBUTTONUP && hook->drag>=0) {
        n=hook->drag; hook->drag=-1; ReleaseCapture();
        if(map_point(hook,lp,&x,&y) && (x!=hook->start_x || y!=hook->start_y)) {
            if((target=monster_at(x,y))>=0) attack_open(hwnd,n,target,0);
            else move_hero(n,x,y,visible);
            move_mode=0;
        }
        return 0;
    }
    if(msg==WM_CAPTURECHANGED) { preview_monster=-1; hook->monster_drag=-1; hook->drag=-1; preview_hero=-1; game_redraw(); }
    if(msg==WM_RBUTTONDOWN && (move_mode || attack_monster>=0)) { attack_monster=-1; move_mode=0; hook->drag=-1; ReleaseCapture(); game_redraw(); return 0; }
    if(msg==WM_NCDESTROY) {
        WNDPROC previous=hook->previous; RemovePropA(hwnd,"HQHeroMap"); free(hook);
        return CallWindowProc(previous,hwnd,msg,wp,lp);
    }
    return CallWindowProc(hook->previous,hwnd,msg,wp,lp);
}
void game_attach(WINDOW_DEF *window,int player)
{
    MAP_HOOK *hook; HWND hwnd;
    if(!window) return; hwnd=W_GetHwnd(window); if(GetPropA(hwnd,"HQHeroMap")) return;
    hook=(MAP_HOOK*)calloc(1,sizeof(*hook)); if(!hook) return;
    hook->window=window; hook->player=player; hook->drag=-1; hook->monster_drag=-1;
    if(!SetPropA(hwnd,"HQHeroMap",hook)) { free(hook); return; }
    hook->previous=(WNDPROC)SetWindowLongPtr(hwnd,GWLP_WNDPROC,(LONG_PTR)map_proc);
}
static int party_selection=-1;
static void party_fields(HWND hwnd)
{
    int i,valid=party_selection>=0 && party_selection<hero_count;
    HERO *h=valid?&heroes[party_selection]:NULL; char status[100];
    SetDlgItemTextA(hwnd,GPNAME,h?h->name:"");
    for(i=0;i<HERO_STATS;i++) { SetDlgItemInt(hwnd,GPSTAT+i,h?h->stats[i]:0,FALSE); EnableWindow(GetDlgItem(hwnd,GPSTAT+i),valid); }
    SetDlgItemInt(hwnd,GPWOUNDS,h?h->wounds:0,FALSE); SetDlgItemInt(hwnd,GPFATE,h?h->fate:0,FALSE);
    EnableWindow(GetDlgItem(hwnd,GPCOMBAT),valid);
    EnableWindow(GetDlgItem(hwnd,GPRANGED),valid);
    EnableWindow(GetDlgItem(hwnd,GPNAME),valid); EnableWindow(GetDlgItem(hwnd,GPWOUNDS),valid); EnableWindow(GetDlgItem(hwnd,GPFATE),valid);
    EnableWindow(GetDlgItem(hwnd,GPMOVE),valid && !(h->kind==4 && !h->wounds)); EnableWindow(GetDlgItem(hwnd,GPRESERVE),valid); EnableWindow(GetDlgItem(hwnd,GPAPPLY),valid);
    if(h && h->kind==4 && !h->wounds) strcpy(status,"Dead - model removed from the map.");
    else if(h && h->x>=0) sprintf(status,"On map: square %d, %d",h->x+1,h->y+1); else strcpy(status,h?"In reserve - choose Place / Move to enter the dungeon.":"Choose a class and Add your first hero.");
    SetDlgItemTextA(hwnd,GPSTATUS,status); InvalidateRect(GetDlgItem(hwnd,GPPORTRAIT),NULL,TRUE);
}
static void party_list(HWND hwnd)
{
    int i; char label[100]; SendDlgItemMessage(hwnd,GPLIST,LB_RESETCONTENT,0,0);
    for(i=0;i<hero_count;i++) { sprintf(label,"%s  [%s]",heroes[i].name,heroes[i].kind==4 && !heroes[i].wounds?"dead":heroes[i].x<0?"reserve":"map"); SendDlgItemMessageA(hwnd,GPLIST,LB_ADDSTRING,0,(LPARAM)label); }
    SendDlgItemMessage(hwnd,GPLIST,LB_SETCURSEL,party_selection,0);
}
static int party_apply(HWND hwnd)
{
    HERO next; int i; BOOL ok;
    if(party_selection<0) return 1;
    next=heroes[party_selection]; GetDlgItemTextA(hwnd,GPNAME,next.name,sizeof(next.name));
    if(!next.name[0]) { message("Give this hero a name."); return 0; }
    for(i=0;i<HERO_STATS+2;i++) {
        int value=(int)GetDlgItemInt(hwnd,i<HERO_STATS?GPSTAT+i:i==HERO_STATS?GPWOUNDS:GPFATE,&ok,FALSE);
        if(!ok || value<0 || value>99) { message("Enter whole numbers from 0 to 99."); return 0; }
        if(i<HERO_STATS) next.stats[i]=value; else if(i==HERO_STATS) next.wounds=value; else next.fate=value;
    }
    if(next.wounds>next.stats[7]) { message("Current Wounds cannot exceed Max W."); return 0; }
    if(next.kind==4 && !next.wounds) next.x=next.y=-1;
    if(memcmp(&next,&heroes[party_selection],sizeof(next))) { remember(); heroes[party_selection]=next; active=1; game_changed(); party_list(hwnd); }
    return 1;
}
static INT_PTR CALLBACK party_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
    int id=LOWORD(wp),n,i;
    if(msg==WM_INITDIALOG) {
        SendDlgItemMessage(hwnd,GPCLASS,CB_SETDROPPEDWIDTH,300,0);
        for(i=0;i<pack_current()->hero_count;i++) SendDlgItemMessageA(hwnd,GPCLASS,CB_ADDSTRING,0,(LPARAM)pack_current()->heroes[i].name);
        SendDlgItemMessage(hwnd,GPCLASS,CB_SETCURSEL,0,0); SendDlgItemMessage(hwnd,GPNAME,EM_LIMITTEXT,39,0);
        party_selection=selected>=0?selected:hero_count?0:-1; party_list(hwnd); party_fields(hwnd); return TRUE;
    }
    if(msg==WM_DRAWITEM && wp==GPPORTRAIT) {
        DRAWITEMSTRUCT *di=(DRAWITEMSTRUCT*)lp;
        FillRect(di->hDC,&di->rcItem,GetSysColorBrush(COLOR_BTNFACE));
        if(party_selection>=0) marker(di->hDC,0,0,di->rcItem.bottom,heroes[party_selection].kind,0,heroes[party_selection].wounds==0);
        return TRUE;
    }
    if(msg==WM_CLOSE) { if(party_apply(hwnd)) EndDialog(hwnd,0); return TRUE; }
    if(msg!=WM_COMMAND) return FALSE;
    if(id==GPLIST && HIWORD(wp)==LBN_SELCHANGE) {
        n=(int)SendDlgItemMessage(hwnd,GPLIST,LB_GETCURSEL,0,0);
        if(!party_apply(hwnd)) { SendDlgItemMessage(hwnd,GPLIST,LB_SETCURSEL,party_selection,0); return TRUE; }
        party_selection=n; SendDlgItemMessage(hwnd,GPLIST,LB_SETCURSEL,n,0); party_fields(hwnd); return TRUE;
    }
    if(id==GPRANGED && party_selection>=0) {
        HERO next; if(!party_apply(hwnd)) return TRUE; next=heroes[party_selection];
        if(edit_ranged(hwnd,&next.ranged,&next.moved,strncmp(next.profile_id,"fantasy:",8)?-1:next.kind,next.profile_id) && memcmp(&next,&heroes[party_selection],sizeof(next))) {
            remember(); heroes[party_selection]=next; active=1; game_changed();
        }
        party_fields(hwnd); return TRUE;
    }
    if(id==GPCOMBAT && party_selection>=0) {
        HERO next;
        if(!party_apply(hwnd)) return TRUE;
        next=heroes[party_selection];
        if(edit_profile(hwnd,&next.melee,next.stats,next.profile_id,strncmp(next.profile_id,"fantasy:",8)?-1:next.kind) && memcmp(&next,&heroes[party_selection],sizeof(next))) {
            remember(); heroes[party_selection]=next; active=1; game_changed();
        }
        party_fields(hwnd); return TRUE;
    }
    if(id==GPRULES) {
        if(party_selection>=0) {
            const HERO *h=&heroes[party_selection];
            MessageBoxA(hwnd,h->class_rules[0]?h->class_rules:hero_class_rules(h->kind),h->class_name[0]?h->class_name:hero_classes[h->kind],MB_OK|MB_ICONINFORMATION);
        } else {
            n=(int)SendDlgItemMessage(hwnd,GPCLASS,CB_GETCURSEL,0,0);
            if(n>=0 && n<pack_current()->hero_count) MessageBoxA(hwnd,pack_current()->heroes[n].text,pack_current()->heroes[n].name,MB_OK|MB_ICONINFORMATION);
        }
        return TRUE;
    }
    if(id!=GPADD && id!=GPAPPLY && id!=GPMOVE && id!=GPRESERVE && id!=IDOK && id!=IDCANCEL) return FALSE;
    if(!party_apply(hwnd)) return TRUE;
    if(id==GPADD) {
        if(hero_count==HERO_LIMIT) { message("This party already has 16 heroes, including reserve."); return TRUE; }
        n=(int)SendDlgItemMessage(hwnd,GPCLASS,CB_GETCURSEL,0,0); if(n<0) return TRUE;
        remember(); pack_hero(&heroes[hero_count],&pack_current()->heroes[n],hero_count+1); party_selection=hero_count++; active=1; game_changed();
    } else if(id==GPRESERVE && party_selection>=0) {
        remember(); heroes[party_selection].x=heroes[party_selection].y=-1; move_mode=0; game_changed();
    } else if(id==GPMOVE && party_selection>=0) {
        selected=party_selection; move_mode=1; game_redraw(); EndDialog(hwnd,1); return TRUE;
    } else if(id==IDOK || id==IDCANCEL) { EndDialog(hwnd,0); return TRUE; }
    party_list(hwnd); party_fields(hwnd); return TRUE;
}
void game_party(void)
{
    WINDOW_DEF *target; INT_PTR result;
    if(!map_ready || !Pice || !Xsize || !Ysize) { message("Generate or load a dungeon first."); return; }
    /* Dialog activation must not change the chosen map or bypass player fog. */
    game_view_pause(1);
    if(preferred_player_view) show_player_view(session_title);
    else if(!Grafik_Karte) show_grafic(session_title);
    result=DialogBoxParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGPARTY),GlMainHwnd,party_proc,0);
    game_restore_view();
    target=map_view_window(preferred_player_view);
    if(result==1 && target) {
        Wind_On_Top(target);
        SetFocus(W_GetHwnd(target));
    }
    game_view_pause(0);
}
void game_command(int command)
{
    if(!map_ready) return;
    if(command==MGAMETURN) {
        int i; remember();
        for(i=0;i<hero_count;i++) heroes[i].moved=heroes[i].fired=0;
        for(i=0;i<monsters.count;i++) monsters.tokens[i].moved=0;
        game_changed();
    }
    else if(command==MGAMELEAVE) reserve_party();
    else if(command==MGAMEUNDO && undo_valid) {
        static MONSTER_STATE monster_swap;
        HERO swap[HERO_LIMIT]; int n=hero_count;
        monster_swap=monsters; monsters=undo_monsters; undo_monsters=monster_swap; selected_monster=-1;
        memcpy(swap,heroes,sizeof(heroes)); memcpy(heroes,undo_heroes,sizeof(heroes)); memcpy(undo_heroes,swap,sizeof(heroes));
        hero_count=undo_count; undo_count=n; attack_monster=-1; selected=-1; move_mode=0; game_changed();
    }
}

#include "monster-ui.inc"
