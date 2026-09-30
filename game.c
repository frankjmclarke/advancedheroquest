/* Native party UI, token interaction and durable session files. */
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <wincrypt.h>
#include <shlobj.h>
#include <random.h>
#include <rolldice.h>
#include <makemap.h>
#include <openwork.h>
#include <wind.h>
#include <defs.h>
#include "game.h"
#include "liste.h"
#include "pack.h"
#include "rsh/game.rh"
#include "rsh/grafic.rh"
#include "rsh/menu.rh"

static MONSTER_STATE monsters,undo_monsters;
static int selected_monster=-1,preview_monster=-1,attack_monster=-1;
static int monster_move_mode;
static void monsters_reveal(void);
static void focus_prune(void);
static void focus_enter(int side,int index);
static int battle_destination(int side,int index,int *x,int *y);
static int battle_destination_cost(int side,int index,int *x,int *y,int *cost);
static int shot_h=-1,shot_m=-1;
static int monster_at(int x,int y);
static HERO heroes[HERO_LIMIT], undo_heroes[HERO_LIMIT];
static int turn_phase,gm_override,undo_turn_phase,undo_gm_override;
static int monster_omit_pct,gold_bonus_pct;
static unsigned short room_gold_bonus_pct[ENCOUNTER_LIMIT];
static HWND panel_hwnd;
static HWND spell_map_dialog;
static int spell_map_picking;
static void panel_attach(void);
static void panel_update(void);
static void panel_note(const char *note);
static int combat_action(int id);
static void token_context_menu(HWND map,int side,int index,int screen_x,int screen_y);
static int preferred_player_view,view_pause,recovery_checked;
static int hero_count,undo_count,undo_valid,selected=-1,move_mode,active,dirty,ready,autosave_warned,map_ready;
static char session_title[160],save_path[MAX_PATH],recovery_path[MAX_PATH],previous_path[MAX_PATH];
static GAME_DATA *loaded_owner;
static unsigned char undo_corridor_pieces[ENCOUNTER_LIMIT];
static int hero_route[240*240],hero_route_count;
static int force_explore_path,forced_move_budget;
static MAGIC_DUNGEON magic,undo_magic;
static PICE undo_pieces[ENCOUNTER_LIMIT];
static int undo_cells[240*240];
static unsigned short undo_gold[ENCOUNTER_LIMIT];
static WINDOW_DEF *preview_window;
static int preview_hero=-1,preview_x,preview_y;
extern void game_loaded_title(const char *title);
static void message(const char *text) { MessageBoxA(GlMainHwnd,text,"HQ-Map game",MB_OK|MB_ICONINFORMATION); }
static int corridor_type(int type) { return strchr("PELRTCOD",type)!=NULL; }
static int floor_piece(int x,int y)
{
    PICE *p; int i;
    if(!get_square((_WORD)x,(_WORD)y,&p) || !p) return -1;
    if(p->type!=DOOR && p->type!=SECRET && p->type!=TEST && p->type!=EMPTY && p->type!=INVALID) return (int)(p-Pice);
    for(i=0;i<MAX_PICE;i++) if(Pice[i].type!=DOOR && Pice[i].type!=SECRET && Pice[i].type!=TEST &&
        Pice[i].type!=EMPTY && Pice[i].type!=INVALID &&
        x>=Pice[i].x && x<Pice[i].x+Pice[i].w && y>=Pice[i].y && y<Pice[i].y+Pice[i].h) return i;
    return -1;
}
static int sight_edge(int x,int y,int nx,int ny,const unsigned char *fog)
{
    int a,b,i,dx,dy;
    PICE *from,*to;
    if(abs(x-nx)+abs(y-ny)!=1 || (a=floor_piece(x,y))<0 || (b=floor_piece(nx,ny))<0) return 0;
    get_square((_WORD)x,(_WORD)y,&from); get_square((_WORD)nx,(_WORD)ny,&to);
    if(a==b && from->type!=DOOR && from->type!=SECRET &&
       to->type!=DOOR && to->type!=SECRET) return 1;
    for(i=0;i<MAX_PICE;i++) if(Pice[i].type==DOOR || Pice[i].type==SECRET) {
        dx=Pice[i].pos==East?1:Pice[i].pos==West?-1:0;
        dy=Pice[i].pos==North?1:Pice[i].pos==South?-1:0;
        if((x==Pice[i].x && y==Pice[i].y && nx==x+dx && ny==y+dy) ||
           (nx==Pice[i].x && ny==Pice[i].y && x==nx+dx && y==ny+dy)) return fog[i]!=0;
    }
    return a==b || (corridor_type(Pice[a].type) && corridor_type(Pice[b].type));
}
static int corridor_sight(int x,int y,int tx,int ty,const unsigned char *fog)
{
    int dx=abs(tx-x),dy=abs(ty-y),sx=tx>x?1:-1,sy=ty>y?1:-1,ix=0,iy=0,nx,ny,a,b;
    if(floor_piece(x,y)<0 || floor_piece(tx,ty)<0) return 0;
    while(ix<dx || iy<dy) {
        a=(1+2*ix)*dy; b=(1+2*iy)*dx; nx=x; ny=y;
        if(a==b) {
            nx+=sx; ny+=sy;
            if(!sight_edge(x,y,nx,y,fog) || !sight_edge(x,y,x,ny,fog) ||
               !sight_edge(nx,y,nx,ny,fog) || !sight_edge(x,ny,nx,ny,fog)) return 0;
            ix++; iy++;
        } else if(a<b) { nx+=sx; ix++; if(!sight_edge(x,y,nx,ny,fog)) return 0; }
        else { ny+=sy; iy++; if(!sight_edge(x,y,nx,ny,fog)) return 0; }
        x=nx; y=ny;
    }
    return 1;
}
static void reveal_corridors(void)
{
    unsigned char *fog; int h,x,y,p;
    if(!map_ready || !Pice || !(fog=game_fog())) return;
    for(h=0;h<hero_count;h++) if(HERO_ACTIVE(&heroes[h]) && heroes[h].x>=0) {
        for(y=0;y<Ysize;y++) for(x=0;x<Xsize;x++) {
            if((p=floor_piece(x,y))<0 || !corridor_type(Pice[p].type) || fog[p]) continue;
            if(corridor_sight(heroes[h].x,heroes[h].y,x,y,fog)) {
                fog[p]=1;
            }
        }
    }
}
#include "magic-runtime.inc"
int game_corridor_visible(int x,int y)
{
    int p; unsigned char *fog;
    if(x<0 || y<0 || x>=Xsize || y>=Ysize || (p=floor_piece(x,y))<0) return 0;
    if(!corridor_type(Pice[p].type)) return 1;
    fog=game_fog(); return fog && fog[p]!=0;
}
/* Format from printed text every time. Unrevealed rooms preview the current
 * setting; revealed rooms keep the percentage fixed at first reveal. */
char *game_gold_text(const PICE *piece,const char *source)
{
    char *result,*out;
    const char *p; int index,pct;
    size_t length;
    if(!source) return NULL;
    index=piece && Pice && piece>=Pice && piece<Pice+MAX_PICE?(int)(piece-Pice):-1;
    pct=index>=0 && index<ENCOUNTER_LIMIT && monsters.seen[index]?room_gold_bonus_pct[index]:piece?gold_bonus_pct:0;
    length=strlen(source);
    if(length>((size_t)-1-64)/2) return NULL;
    result=(char*)malloc(length*2+64);
    if(!result) return NULL;
    p=source; out=result;
    while(*p) {
        if(isdigit((unsigned char)*p) && (p==source || !isalnum((unsigned char)p[-1]))) {
            const char *q=p,*after; long value=0; int digits=0,too_large=0;
            while(isdigit((unsigned char)*q) || (*q==',' && isdigit((unsigned char)q[1]))) {
                if(isdigit((unsigned char)*q)) {
                    int digit=*q-'0'; digits++;
                    if(!too_large) {
                        if(value>(1000000L-digit)/10) too_large=1;
                        else value=value*10+digit;
                    }
                }
                q++;
            }
            after=q; while(*after==' ') after++;
            if(digits && !too_large && !_strnicmp(after,"Gold Crown",10) &&
               (after[10]=='s' || !isalpha((unsigned char)after[10]))) {
                out+=sprintf(out,"%ld",value+(value*pct+50)/100);
                p=q; continue;
            }
        }
        *out++=*p++;
    }
    *out=0; return result;
}
char *game_room_contents(const PICE *piece)
{
    char *source=(char*)room_contents_text(piece?piece->text:NULL),*result;
    if(!source) return NULL;
    result=game_gold_text(piece,source); free(source); return result;
}
static INT_PTR CALLBACK difficulty_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
    int id=LOWORD(wp),omit,gold; BOOL ok1,ok2;
    (void)lp;
    if(msg==WM_INITDIALOG) {
        int chosen=monster_omit_pct==0 && gold_bonus_pct==0?GDSTANDARD:
            monster_omit_pct==10 && gold_bonus_pct==10?GDEASIER:
            monster_omit_pct==25 && gold_bonus_pct==25?GDEASY:GDCUSTOM;
        SetDlgItemInt(hwnd,GDOMIT,monster_omit_pct,FALSE);
        SetDlgItemInt(hwnd,GDGOLD,gold_bonus_pct,FALSE);
        CheckRadioButton(hwnd,GDSTANDARD,GDCUSTOM,chosen); return TRUE;
    }
    if(msg!=WM_COMMAND) return FALSE;
    if(id==IDCANCEL) { EndDialog(hwnd,0); return TRUE; }
    if(id==GDSTANDARD || id==GDEASIER || id==GDEASY) {
        omit=gold=id==GDSTANDARD?0:id==GDEASIER?10:25;
        SetDlgItemInt(hwnd,GDOMIT,omit,FALSE); SetDlgItemInt(hwnd,GDGOLD,gold,FALSE);
        CheckRadioButton(hwnd,GDSTANDARD,GDCUSTOM,id); return TRUE;
    }
    if((id==GDOMIT || id==GDGOLD) && HIWORD(wp)==EN_CHANGE) {
        CheckRadioButton(hwnd,GDSTANDARD,GDCUSTOM,GDCUSTOM); return TRUE;
    }
    if(id!=IDOK) return FALSE;
    omit=(int)GetDlgItemInt(hwnd,GDOMIT,&ok1,FALSE);
    gold=(int)GetDlgItemInt(hwnd,GDGOLD,&ok2,FALSE);
    if(!ok1 || !ok2 || omit<0 || omit>100 || gold<0 || gold>500) {
        MessageBoxA(hwnd,"Enter a monster omission chance from 0 to 100% and a gold bonus from 0 to 500%.","Adventure difficulty",MB_OK|MB_ICONWARNING);
        return TRUE;
    }
    monster_omit_pct=omit; gold_bonus_pct=gold;
    if(map_ready) { active=1; game_changed(); }
    EndDialog(hwnd,1); return TRUE;
}
void game_difficulty_dialog(void)
{
    if(DialogBoxParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGDIFFICULTY),GlMainHwnd,difficulty_proc,0)==1 && Monster_Liste) {
        Wind_Close(Monster_Liste); show_liste(session_title);
    }
}
static void remember(void) {
    int i; unsigned char *fog=game_fog();
    memcpy(undo_heroes,heroes,sizeof(heroes)); undo_count=hero_count; undo_monsters=monsters;
    undo_turn_phase=turn_phase; undo_gm_override=gm_override; undo_magic=magic;
    memcpy(undo_gold,room_gold_bonus_pct,sizeof(undo_gold));
    if(Pice && MAX_PICE<=ENCOUNTER_LIMIT) memcpy(undo_pieces,Pice,sizeof(PICE)*MAX_PICE);
    for(i=0;i<Xsize*Ysize;i++) { PICE *p=NULL; get_square(i%Xsize,i/Xsize,&p); undo_cells[i]=p?(int)(p-Pice):-1; }
    for(i=0;fog && i<MAX_PICE && i<ENCOUNTER_LIMIT;i++) undo_corridor_pieces[i]=fog[i];
    undo_valid=1;
}
static const char *turn_attack_reason(int side,int h,int m)
{
    if(!gm_override && (side?monsters.tokens[m].move_spent:heroes[h].move_spent)==1000000) return "This model's movement and normal attack were used by a spell this turn.";
    if(side && !gm_override && magic_stopped(m)) return "Still Air: this monster cannot move or attack this turn.";
    if(!turn_phase || gm_override) return NULL;
    if(turn_phase!=(side?2:1)) return side?"It is the Hero phase.":"It is the GM phase.";
    if(side?monsters.tokens[m].attacked:heroes[h].attacked) return "This model has already made its normal attack or run this phase.";
    return NULL;
}
static int turn_move_remaining(int side,int index)
{
    int speed=side?monsters.tokens[index].stats[4]:heroes[index].stats[4];
    int bonus=side?monsters.tokens[index].run_bonus:heroes[index].run_bonus;
    int spent=side?monsters.tokens[index].move_spent:heroes[index].move_spent;
    if(magic_model(side,index)->swift) speed=magic.exploration?18:2*speed;
    else if(magic.exploration) speed=12;
    return speed+bonus>spent?speed+bonus-spent:0;
}
static int turn_can_move(int side,int index)
{
    if(!gm_override && (side?monsters.tokens[index].move_spent:heroes[index].move_spent)==1000000) return 0;
    if(side && !gm_override && magic_stopped(index)) return 0;
    if((side?monsters.tokens[index].caster.move_locked:heroes[index].caster.move_locked) && !gm_override) return 0;
    if((!turn_phase && !magic.exploration && !magic_model(side,index)->swift) || gm_override) return 1;
    if(turn_phase && turn_phase!=(side?2:1)) return 0;
    if((side?monsters.tokens[index].x:heroes[index].x)<0) return 1; /* setup placement */
    return turn_move_remaining(side,index)>0;
}
static void turn_reset_side(int side)
{
    int i;
    if(side) for(i=0;i<monsters.count;i++) {
        MONSTER *m=&monsters.tokens[i]; m->move_spent=m->attacked=m->run_bonus=m->moved=0; m->caster.cast_used=m->caster.move_locked=0;
    } else for(i=0;i<hero_count;i++) {
        HERO *h=&heroes[i]; h->move_spent=h->attacked=h->run_bonus=h->moved=h->fired=0; h->caster.cast_used=h->caster.move_locked=0;
    }
}
static int turn_run_apply(int side,int index,int roll)
{
    if(!turn_phase || roll<1 || roll>12 || index<0 ||
       (side?index>=monsters.count:index>=hero_count) ||
       (!gm_override && turn_attack_reason(side,side?0:index,side?index:0))) return 0;
    remember();
    if(side) { monsters.tokens[index].attacked=1; monsters.tokens[index].run_bonus=roll==1?0:roll; }
    else { heroes[index].attacked=1; heroes[index].run_bonus=roll==1?0:roll; }
    game_changed(); return 1;
}
static void turn_menu_update(void)
{
    HMENU menu=GlMainHwnd?GetMenu(GlMainHwnd):NULL;
    int i,used=0,total=0; char status[100];
    if(!menu) return;
    if(turn_phase==1) for(i=0;i<hero_count;i++) {
        if(heroes[i].x<0 || !HERO_ACTIVE(&heroes[i])) continue;
        total++; if(heroes[i].attacked) used++;
    } else if(turn_phase==2) for(i=0;i<monsters.count;i++) {
        if(monsters.tokens[i].x<0 || !monsters.tokens[i].wounds) continue;
        total++; if(monsters.tokens[i].attacked) used++;
    }
    /* A maximized MDI child inserts its system menu before the app menus. */
    for(i=0;i<GetMenuItemCount(menu);i++) {
        HMENU submenu=GetSubMenu(menu,i);
        if(submenu && GetMenuState(submenu,MGAMEPARTY,MF_BYCOMMAND)!=(UINT)-1) {
            ModifyMenuA(menu,i,MF_BYPOSITION|MF_POPUP,(UINT_PTR)submenu,
                turn_phase==1?"&Party - Hero phase":turn_phase==2?"&Party - GM phase":"&Party - Free play");
            break;
        }
    }
    if(turn_phase) sprintf(status,"%s phase: %d/%d normal attacks used%s",turn_phase==1?"Hero":"GM",used,total,gm_override?" (GM override)":"");
    else snprintf(status,sizeof(status),"%s | Turn %d",magic.exploration?"Exploration: 12 squares per model":"Free play",magic.turn+1);
    ModifyMenuA(menu,MGAMEPHASESTATUS,MF_BYCOMMAND|MF_STRING|MF_GRAYED,MGAMEPHASESTATUS,status);
    CheckMenuItem(menu,MGAMEGUIDED,MF_BYCOMMAND|(turn_phase?MF_CHECKED:MF_UNCHECKED));
    CheckMenuItem(menu,MGAMEOVERRIDE,MF_BYCOMMAND|(gm_override?MF_CHECKED:MF_UNCHECKED));
    EnableMenuItem(menu,MGAMEOVERRIDE,MF_BYCOMMAND|(turn_phase?MF_ENABLED:MF_GRAYED));
    EnableMenuItem(menu,MGAMERUN,MF_BYCOMMAND|(turn_phase?MF_ENABLED:MF_GRAYED));
    ModifyMenuA(menu,MGAMETURN,MF_BYCOMMAND|MF_STRING,MGAMETURN,
        turn_phase==1?"End Hero phase / Start GM phase":turn_phase==2?"End GM phase / Start Hero phase":magic.exploration?"Next exploration turn":"Next Turn (reset movement)");
    DrawMenuBar(GlMainHwnd);
}
static int capture(GAME_DATA *d)
{
    int x,y; PICE *p;
    memset(d,0,sizeof(*d)); if(!map_ready || !Pice || Xsize<1 || Ysize<1) return 0;
    d->pieces=Pice; d->count=MAX_PICE; d->width=Xsize; d->height=Ysize;
    d->visible=game_fog(); if(!d->visible) return 0;
    d->player_view=preferred_player_view;
    d->turn_phase=turn_phase; d->gm_override=gm_override; d->magic=magic;
    d->monster_omit_pct=monster_omit_pct; d->gold_bonus_pct=gold_bonus_pct;
    memcpy(d->room_gold_bonus_pct,room_gold_bonus_pct,sizeof(room_gold_bonus_pct));
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
void game_toggle_view(void)
{
    if(!map_ready) return;
    if(preferred_player_view) {
        if(!Grafik_Karte) show_grafic(session_title);
        if(Grafik_Karte) Wind_On_Top(Grafik_Karte);
    } else show_player_view(session_title);
}
/* Freeze focus tracking while windows are being replaced or closed together. */
void game_view_pause(int pause)
{
    if(pause) view_pause++; else if(view_pause) view_pause--;
}
void game_view_selected(int player)
{
    HMENU menu;
    player=player!=0;
    if(view_pause || preferred_player_view==player) return;
    preferred_player_view=player;
    menu=GlMainHwnd?GetMenu(GlMainHwnd):NULL;
    if(menu) {
        ModifyMenuA(menu,MPLAYER,MF_BYCOMMAND|MF_STRING,MPLAYER,
            player?"&GM Map (no fog)\tAlt+W":"&Player View (fog of war)\tAlt+W");
        CheckMenuItem(menu,MPLAYER,MF_BYCOMMAND|(player?MF_CHECKED:MF_UNCHECKED));
        DrawMenuBar(GlMainHwnd);
    }
    /* Include a view-only switch in recovery even without moving a token. */
    if(ready && active && map_ready) game_changed();
    else panel_update();
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
    reveal_corridors();
    monsters_reveal();
    focus_prune();
    turn_menu_update();
    panel_update();
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
    unsigned char *prepared_fog; int fog_capacity;
    if(!recovery) { path[0]=0; if(!choose_path(path,0)) return 0; }
    else strcpy(path,recovery_path);
    d=read_game(path);
    if(!d && recovery) d=read_game(previous_path);
    if(!d) { if(recovery!=2) message("This saved game is damaged, unsupported, or unavailable. The current game has not changed."); return 0; }
    if(recovery!=2 && !game_before_replace()) { game_data_free(d); return 0; }
    next_pack=pack_clone(d->pack);
    if(!next_pack) { game_data_free(d); message("Not enough memory for character profiles."); return 0; }
    fog_capacity=d->count>MAX_PICE?d->count:MAX_PICE;
    prepared_fog=game_prepare_fog(d->visible,d->count,fog_capacity);
    if(!prepared_fog) { pack_free(next_pack); game_data_free(d); if(recovery!=2) message("Not enough memory to restore fog of war."); return 0; }
    /* map_restore allocates first, so even allocation failure preserves play. */
    if(!map_restore(d->pieces,d->count,d->width,d->height,d->cells)) { game_discard_prepared_fog(prepared_fog,fog_capacity); pack_free(next_pack); game_data_free(d); if(recovery!=2) message("Not enough memory to load the game."); return 0; }
    game_adopt_fog(prepared_fog,fog_capacity);
    pack_adopt(next_pack);
    close_all_windows(FALSE);
    game_data_free(loaded_owner); loaded_owner=d;
    if(d->player_view>=0) preferred_player_view=d->player_view;
    monsters=d->monsters; selected_monster=-1; attack_monster=-1; monster_move_mode=0;
    hero_count=d->hero_count; memcpy(heroes,d->heroes,sizeof(heroes));
    turn_phase=d->turn_phase; gm_override=d->gm_override; magic=d->magic;
    monster_omit_pct=d->monster_omit_pct; gold_bonus_pct=d->gold_bonus_pct;
    memcpy(room_gold_bonus_pct,d->room_gold_bonus_pct,sizeof(room_gold_bonus_pct));
    { int i; for(i=0;i<hero_count;i++) if(HERO_DEAD(&heroes[i])) heroes[i].x=heroes[i].y=-1; }
    strcpy(session_title,d->title);
    selected=-1; move_mode=0; undo_valid=0; active=1; map_ready=1; dirty=recovery?1:0;
    if(recovery) save_path[0]=0; else strcpy(save_path,path);
    monsters_reveal(); focus_prune();
    turn_menu_update();
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
    memset(monsters.seen,0,sizeof(monsters.seen)); selected_monster=-1; preview_monster=-1; attack_monster=-1; monster_move_mode=0; map_ready=0; dirty=0; selected=-1; move_mode=0; undo_valid=0; preview_hero=-1;
    memset(room_gold_bonus_pct,0,sizeof(room_gold_bonus_pct)); memset(&magic,0,sizeof(magic));
    for(i=0;i<hero_count;i++) { heroes[i].x=heroes[i].y=-1; heroes[i].fired=heroes[i].focus=heroes[i].moved=0; heroes[i].move_spent=heroes[i].attacked=heroes[i].run_bonus=0; heroes[i].caster.cast_used=heroes[i].caster.move_locked=0; memset(&heroes[i].magic,0,sizeof(heroes[i].magic)); }
    turn_phase=gm_override=0; turn_menu_update(); panel_update();
    game_reset_fog();
}
void game_new_dungeon(const char *title)
{
    int i;
    if(!pack_start_dungeon()) { map_ready=0; message("Not enough memory for character profiles."); return; }
    map_ready=1; active=1;
    game_data_free(loaded_owner); loaded_owner=NULL;
    game_reset_fog();
    {
        unsigned char *fog=game_fog();
        if(fog) for(i=0;i<MAX_PICE;i++) if(Pice[i].type==STAIRS_OUT) fog[i]=1;
    }
    monsters.count=0; memset(monsters.tokens,0,sizeof(monsters.tokens));
    memset(monsters.seen,0,sizeof(monsters.seen)); selected_monster=-1; preview_monster=-1; attack_monster=-1; monster_move_mode=0;
    memset(room_gold_bonus_pct,0,sizeof(room_gold_bonus_pct)); memset(&magic,0,sizeof(magic));
    strncpy(session_title,title,sizeof(session_title)-1); session_title[sizeof(session_title)-1]=0;
    for(i=0;i<hero_count;i++) { heroes[i].x=heroes[i].y=-1; heroes[i].fired=heroes[i].focus=heroes[i].moved=0; heroes[i].move_spent=heroes[i].attacked=heroes[i].run_bonus=0; heroes[i].caster.cast_used=heroes[i].caster.move_locked=0; memset(&heroes[i].magic,0,sizeof(heroes[i].magic)); }
    turn_phase=gm_override=0; turn_menu_update(); panel_update();
    selected=-1; move_mode=0; undo_valid=0; preview_hero=-1;
    game_changed();
}
void game_shutdown(void)
{
    if(spell_map_dialog) DestroyWindow(spell_map_dialog);
    game_data_free(loaded_owner); loaded_owner=NULL;
    game_reset_fog();
    pack_shutdown();
}
static void reserve_party(void)
{
    int i; if(!hero_count) return; remember();
    for(i=0;i<hero_count;i++) { heroes[i].x=heroes[i].y=-1; heroes[i].fired=heroes[i].focus=heroes[i].moved=0; heroes[i].move_spent=heroes[i].attacked=heroes[i].run_bonus=0; heroes[i].caster.cast_used=heroes[i].caster.move_locked=0; memset(&heroes[i].magic,0,sizeof(heroes[i].magic)); }
    move_mode=0; game_changed();
}
/* Keep token locations on occupied map squares, with strictly one hero each. */
static int can_move(int index,int x,int y,const unsigned char *visible)
{
    PICE *piece; int i;
    if(!get_square((_WORD)x,(_WORD)y,&piece) || !piece || piece->type==EMPTY || piece->type==TEST) return 0;
    if(visible && (!fog_visible(visible,(_WORD)(piece-Pice)) || !game_corridor_visible(x,y)) &&
       !(index>=0 && heroes[index].x>=0 && abs(x-heroes[index].x)+abs(y-heroes[index].y)==1 &&
         sight_edge(heroes[index].x,heroes[index].y,x,y,visible))) return 0;
    for(i=0;i<hero_count;i++) if(i!=index && heroes[i].x==x && heroes[i].y==y) return 0;
    if(monster_at(x,y)>=0) return 0;
    if(index>=0 && !HERO_ACTIVE(&heroes[index])) return 0;
    return 1;
}
static int move_hero(int index,int x,int y,const unsigned char *visible)
{
    int cost=0,i,allowed; char note[140];
    if(index<0 || index>=hero_count || !can_move(index,x,y,visible)) { MessageBeep(MB_ICONWARNING); return 0; }
    if(heroes[index].x==x && heroes[index].y==y) return 1;
    force_explore_path=visible!=NULL;
    allowed=turn_can_move(0,index) && battle_destination_cost(0,index,&x,&y,&cost);
    force_explore_path=0;
    if(!allowed) { MessageBeep(MB_ICONWARNING); return 0; }
    remember(); if(heroes[index].x>=0) heroes[index].moved=1;
    if(turn_phase || magic.exploration || heroes[index].magic.swift) heroes[index].move_spent+=cost;
    for(i=0;i<hero_route_count;i++) {
        heroes[index].x=hero_route[i]%Xsize; heroes[index].y=hero_route[i]/Xsize;
        reveal_corridors();
    }
    heroes[index].x=x; heroes[index].y=y; focus_enter(0,index);
    if(turn_phase) sprintf(note,"%s moved %d square%s; %d remaining.",heroes[index].name,cost,cost==1?"":"s",turn_move_remaining(0,index));
    else sprintf(note,"%s moved to square %d, %d.",heroes[index].name,x+1,y+1);
    panel_note(note); game_changed(); return 1;
}
static int hero_at(int x,int y,const unsigned char *visible)
{
    int i; PICE *p;
    if(!get_square((_WORD)x,(_WORD)y,&p) || !p ||
       (visible && (!fog_visible(visible,(_WORD)(p-Pice)) || !game_corridor_visible(x,y)))) return -1;
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
#include "magic-map.inc"
#include "spellcasting.inc"
#include "spell-map.inc"

void game_draw(WINDOW_DEF *window,const unsigned char *visible,int zoom,int sx,int sy)
{
    int i,x,y,size=8*zoom,saved; HDC dc=W_GetDC(window); RECT label;
    if(!dc) return; saved=SaveDC(dc); SetBkMode(dc,OPAQUE); SetBkColor(dc,RGB(35,28,22)); SetTextColor(dc,RGB(255,249,221));
    SelectObject(dc,GetStockObject(DEFAULT_GUI_FONT));
    for(i=0;i<hero_count;i++) {
        HERO *h=&heroes[i]; if(h->x<0 || hero_at(h->x,h->y,visible)!=i) continue;
        x=((i==preview_hero && window==preview_window?preview_x:h->x)+1)*size-sx;
        y=(Ysize-(i==preview_hero && window==preview_window?preview_y:h->y))*size-sy;
        marker(dc,x,y,size,h->kind,i==selected,!HERO_ACTIVE(h));
        if(size>=24) { label.left=x-size/2; label.right=x+size+size/2; label.top=y+size; label.bottom=label.top+16;
            DrawTextA(dc,h->name,-1,&label,DT_CENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX); }
    }
    for(i=0;i<monsters.count;i++) {
        MONSTER *m=&monsters.tokens[i]; PICE *p;
        if(m->x<0 || monster_at(m->x,m->y)!=i) continue;
        if(visible && (!get_square((_WORD)m->x,(_WORD)m->y,&p) || !p ||
           !fog_visible(visible,(_WORD)(p-Pice)) || !game_corridor_visible(m->x,m->y))) continue;
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
    spell_map_draw(dc,visible,size,sx,sy);
    RestoreDC(dc,saved);
}
typedef struct { WINDOW_DEF *window; WNDPROC previous; int player,drag,start_x,start_y,monster_drag; } MAP_HOOK;
static int move_monster(int index,int x,int y)
{
    int cost=0; char note[160]; MONSTER *m;
    if(index<0 || index>=monsters.count || !monster_can_move(index,x,y) || !turn_can_move(1,index) ||
       !battle_destination_cost(1,index,&x,&y,&cost)) { MessageBeep(MB_ICONWARNING); return 0; }
    remember(); m=&monsters.tokens[index]; m->moved=1; m->x=x; m->y=y;
    if(turn_phase || magic.exploration || m->magic.swift) m->move_spent+=cost;
    focus_enter(1,index);
    if(turn_phase) sprintf(note,"%s moved %d square%s; %d remaining.",m->name,cost,cost==1?"":"s",turn_move_remaining(1,index));
    else sprintf(note,"%s moved to square %d, %d.",m->name,x+1,y+1);
    panel_note(note); active=1; game_changed(); return 1;
}
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
    if(spell_map_dialog) {
        if(msg==WM_SETCURSOR && LOWORD(lp)==HTCLIENT) { SetCursor(LoadCursor(NULL,IDC_CROSS)); return TRUE; }
        if(msg==WM_KEYDOWN && wp==VK_ESCAPE) { spell_map_resume(); return 0; }
        if((msg==WM_LBUTTONDOWN || msg==WM_LBUTTONDBLCLK) && map_point(hook,lp,&x,&y)) {
            spell_map_click(x,y,visible); return 0;
        }
        if(msg==WM_MOUSEMOVE && map_point(hook,lp,&x,&y)) { spell_map_hover(x,y); return 0; }
        if(msg==WM_LBUTTONUP || msg==WM_RBUTTONDOWN || msg==WM_RBUTTONUP || msg==WM_RBUTTONDBLCLK || msg==WM_CONTEXTMENU) return 0;
    }
    if(msg==WM_SETCURSOR && move_mode && LOWORD(lp)==HTCLIENT) { SetCursor(LoadCursor(NULL,IDC_CROSS)); return TRUE; }
    if((msg==WM_LBUTTONDOWN || msg==WM_LBUTTONDBLCLK) && map_point(hook,lp,&x,&y) &&
       (!visible || game_corridor_visible(x,y)) && (n=monster_at(x,y))>=0) {
        if(msg==WM_LBUTTONDOWN && selected>=0 && move_mode) { attack_open(hwnd,selected,n,0); return 0; }
        selected_monster=n; selected=-1; move_mode=0; monster_move_mode=0; panel_update();
        if(msg==WM_LBUTTONDBLCLK) { attack_monster=-1; hook->monster_drag=-1; ReleaseCapture(); monster_damage(n); }
        else { attack_monster=n; hook->monster_drag=n; hook->start_x=x; hook->start_y=y; SetCapture(hwnd); game_redraw(); }
        return 0;
    }
    if(msg==WM_MOUSEMOVE && hook->monster_drag>=0) {
        if(map_point(hook,lp,&x,&y) && (target=hero_at(x,y,visible))>=0 && !HERO_DEAD(&heroes[target]) && monsters.tokens[hook->monster_drag].wounds) {
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
            else if(monster_can_move(n,x,y) && turn_can_move(1,n)) { attack_monster=-1; move_monster(n,x,y); }
            else MessageBeep(MB_ICONWARNING);
        }
        return 0;
    }
    if(msg==WM_RBUTTONDOWN || msg==WM_RBUTTONDBLCLK) return 0;
    if(msg==WM_RBUTTONUP) {
        POINT point; int side=-1,index=-1;
        if(GetCapture()==hwnd) ReleaseCapture();
        hook->drag=hook->monster_drag=-1;
        if(map_point(hook,lp,&x,&y)) {
            index=(!visible || game_corridor_visible(x,y))?monster_at(x,y):-1; if(index>=0) side=1;
            else { index=hero_at(x,y,visible); if(index>=0) side=0; }
        }
        if(side>=0) {
            point.x=(short)LOWORD(lp); point.y=(short)HIWORD(lp); ClientToScreen(hwnd,&point);
            token_context_menu(hwnd,side,index,point.x,point.y);
        } else {
            attack_monster=-1; monster_move_mode=move_mode=0;
            panel_note("Action cancelled. Select a model or choose another action."); game_redraw();
        }
        return 0;
    }
    if(msg==WM_CONTEXTMENU) {
        POINT point; RECT bounds; int side=-1,index=-1;
        if((short)LOWORD(lp)==-1 && (short)HIWORD(lp)==-1) {
            L_GRECT doc; int square_x=-1,square_y=-1,zoom=Grafik_Zoom_Get();
            if(selected>=0 && selected<hero_count) { side=0; index=selected; }
            else if(selected_monster>=0 && selected_monster<monsters.count) { side=1; index=selected_monster; }
            if(side==0) { square_x=heroes[index].x; square_y=heroes[index].y; }
            else if(side==1) { square_x=monsters.tokens[index].x; square_y=monsters.tokens[index].y; }
            if(square_x>=0 && zoom>0) {
                Wind_GetDoc(hook->window,&doc);
                point.x=(square_x+1)*8*zoom+4*zoom-(int)doc.xx;
                point.y=(Ysize-square_y)*8*zoom+4*zoom-(int)doc.yy;
                ClientToScreen(hwnd,&point);
            } else {
                GetWindowRect(hwnd,&bounds);
                point.x=bounds.left+24; point.y=bounds.top+48;
            }
        } else {
            point.x=(short)LOWORD(lp); point.y=(short)HIWORD(lp);
            { POINT client=point; ScreenToClient(hwnd,&client);
              if(map_point(hook,MAKELPARAM(client.x,client.y),&x,&y)) {
                  index=(!visible || game_corridor_visible(x,y))?monster_at(x,y):-1; if(index>=0) side=1;
                  else { index=hero_at(x,y,visible); if(index>=0) side=0; }
              }
            }
        }
        if(side>=0) token_context_menu(hwnd,side,index,point.x,point.y);
        return 0;
    }
    if((msg==WM_LBUTTONDOWN || msg==WM_LBUTTONDBLCLK) && map_point(hook,lp,&x,&y)) {
        n=hero_at(x,y,visible);
        if(n>=0 && attack_monster>=0 && msg==WM_LBUTTONDOWN) { attack_open(hwnd,n,attack_monster,1); attack_monster=-1; return 0; }
        if(n>=0) { attack_monster=-1; monster_move_mode=0; selected_monster=-1; selected=n; move_mode=1; hook->drag=n; hook->start_x=x; hook->start_y=y; SetCapture(hwnd); game_redraw(); panel_update(); return 0; }
        if(monster_move_mode && selected_monster>=0) {
            if(move_monster(selected_monster,x,y)) monster_move_mode=0;
            panel_update(); return 0;
        }
        if(move_mode && selected>=0) {
            if(move_hero(selected,x,y,visible)) { move_mode=0; SetCursor(LoadCursor(NULL,IDC_ARROW)); }
            return 0;
        }
    }
    if(msg==WM_MOUSEMOVE && hook->drag>=0) {
        if(map_point(hook,lp,&x,&y) && (!visible || game_corridor_visible(x,y)) &&
           (target=monster_at(x,y))>=0 && HERO_ACTIVE(&heroes[hook->drag]) && monsters.tokens[target].wounds) {
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
            if((!visible || game_corridor_visible(x,y)) && (target=monster_at(x,y))>=0) attack_open(hwnd,n,target,0);
            else move_hero(n,x,y,visible);
            move_mode=0;
        }
        return 0;
    }
    if(msg==WM_CAPTURECHANGED) { preview_monster=-1; hook->monster_drag=-1; hook->drag=-1; preview_hero=-1; game_redraw(); }
    if(msg==WM_KEYDOWN && wp==VK_ESCAPE) {
        attack_monster=-1; monster_move_mode=move_mode=0; hook->drag=hook->monster_drag=-1;
        if(GetCapture()==hwnd) ReleaseCapture();
        panel_note("Action cancelled. Select a model or choose another action."); game_redraw(); return 0;
    }
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
    panel_attach();
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
    EnableWindow(GetDlgItem(hwnd,GPSPELLBOOK),valid && pack_current() && pack_current()->book_count);
    EnableWindow(GetDlgItem(hwnd,GPMOVE),valid && HERO_ACTIVE(h)); EnableWindow(GetDlgItem(hwnd,GPRESERVE),valid); EnableWindow(GetDlgItem(hwnd,GPDELETE),valid); EnableWindow(GetDlgItem(hwnd,GPAPPLY),valid);
    if(h && HERO_DEAD(h)) strcpy(status,"Dead - model removed from the map.");
    else if(h && HERO_KO(h)) strcpy(status,"KO'd at zero Wounds. Another wound is fatal.");
    else if(h && h->x>=0) sprintf(status,"On map: square %d, %d",h->x+1,h->y+1); else strcpy(status,h?"In reserve - choose Place / Move to enter the dungeon.":"Choose a class and Add your first hero.");
    if(h && turn_phase && !HERO_DEAD(h)) sprintf(status,"%s phase | Move %d/%d | %s%s",turn_phase==1?"Hero":"GM",h->move_spent,h->stats[4]+h->run_bonus,h->attacked?"Attack used":"Attack ready",gm_override?" | GM override":"");
    SetDlgItemTextA(hwnd,GPSTATUS,status); InvalidateRect(GetDlgItem(hwnd,GPPORTRAIT),NULL,TRUE);
    SetDlgItemTextA(hwnd,GPGUIDE,turn_phase==1?"Hero phase: move and attack in either order. End phase in Party menu when ready.":turn_phase==2?"GM phase: move and attack with monsters, then end phase in Party menu.":"Free play. Enable Guided combat turns in the Party menu to use phases.");
}
static void party_list(HWND hwnd)
{
    int i; char label[100]; SendDlgItemMessage(hwnd,GPLIST,LB_RESETCONTENT,0,0);
    for(i=0;i<hero_count;i++) { sprintf(label,"%s  [%s]",heroes[i].name,HERO_DEAD(&heroes[i])?"dead":HERO_KO(&heroes[i])?"KO'd":heroes[i].x<0?"reserve":"map"); SendDlgItemMessageA(hwnd,GPLIST,LB_ADDSTRING,0,(LPARAM)label); }
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
    next.condition=next.wounds?0:next.kind==4?2:1;
    magic_hero_death(&next);
    if(memcmp(&next,&heroes[party_selection],sizeof(next))) { remember(); heroes[party_selection]=next; active=1; game_changed(); party_list(hwnd); }
    return 1;
}
static INT_PTR CALLBACK party_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
    int id=LOWORD(wp),n,i;
    if(msg==WM_INITDIALOG) {
        const CHARACTER_PACK *pack=pack_current();
        if(!pack) { message("Not enough memory for character profiles."); EndDialog(hwnd,0); return TRUE; }
        SendDlgItemMessage(hwnd,GPCLASS,CB_SETDROPPEDWIDTH,300,0);
        for(i=0;i<pack->hero_count;i++) SendDlgItemMessageA(hwnd,GPCLASS,CB_ADDSTRING,0,(LPARAM)pack->heroes[i].name);
        SendDlgItemMessage(hwnd,GPCLASS,CB_SETCURSEL,0,0); SendDlgItemMessage(hwnd,GPNAME,EM_LIMITTEXT,39,0);
        party_selection=selected>=0?selected:hero_count?0:-1; party_list(hwnd); party_fields(hwnd); return TRUE;
    }
    if(msg==WM_DRAWITEM && wp==GPPORTRAIT) {
        DRAWITEMSTRUCT *di=(DRAWITEMSTRUCT*)lp;
        FillRect(di->hDC,&di->rcItem,GetSysColorBrush(COLOR_BTNFACE));
        if(party_selection>=0) marker(di->hDC,0,0,di->rcItem.bottom,heroes[party_selection].kind,0,!HERO_ACTIVE(&heroes[party_selection]));
        return TRUE;
    }
    if(msg==WM_CLOSE) { if(party_apply(hwnd)) EndDialog(hwnd,0); return TRUE; }
    if(msg!=WM_COMMAND) return FALSE;
    if(id==GPLIST && HIWORD(wp)==LBN_SELCHANGE) {
        n=(int)SendDlgItemMessage(hwnd,GPLIST,LB_GETCURSEL,0,0);
        if(!party_apply(hwnd)) { SendDlgItemMessage(hwnd,GPLIST,LB_SETCURSEL,party_selection,0); return TRUE; }
        party_selection=n; SendDlgItemMessage(hwnd,GPLIST,LB_SETCURSEL,n,0); party_fields(hwnd); return TRUE;
    }
    if(id==GPSPELLBOOK && party_selection>=0) {
        if(!party_apply(hwnd)) return TRUE;
        spellbook_open(hwnd,0,party_selection); party_fields(hwnd); return TRUE;
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
            const CHARACTER_PACK *pack=pack_current();
            n=(int)SendDlgItemMessage(hwnd,GPCLASS,CB_GETCURSEL,0,0);
            if(pack && n>=0 && n<pack->hero_count) MessageBoxA(hwnd,pack->heroes[n].text,pack->heroes[n].name,MB_OK|MB_ICONINFORMATION);
        }
        return TRUE;
    }
    if(id!=GPADD && id!=GPAPPLY && id!=GPMOVE && id!=GPRESERVE && id!=GPDELETE && id!=IDOK && id!=IDCANCEL) return FALSE;
    if(!party_apply(hwnd)) return TRUE;
    if(id==GPADD) {
        const CHARACTER_PACK *pack=pack_current();
        if(hero_count==HERO_LIMIT) { message("This party already has 16 heroes, including reserve."); return TRUE; }
        n=(int)SendDlgItemMessage(hwnd,GPCLASS,CB_GETCURSEL,0,0); if(!pack || n<0 || n>=pack->hero_count) return TRUE;
        remember(); pack_hero(&heroes[hero_count],&pack->heroes[n],hero_count+1); party_selection=hero_count++; active=1; game_changed();
    } else if(id==GPRESERVE && party_selection>=0) {
        remember(); heroes[party_selection].x=heroes[party_selection].y=-1; move_mode=0; game_changed();
    } else if(id==GPDELETE && party_selection>=0) {
        n=party_selection; remember();
        memmove(&heroes[n],&heroes[n+1],(size_t)(hero_count-n-1)*sizeof(heroes[0]));
        memset(&heroes[--hero_count],0,sizeof(heroes[0]));
        if(selected==n) selected=-1; else if(selected>n) selected--;
        if(party_selection>=hero_count) party_selection=hero_count-1;
        move_mode=0; active=1; game_changed();
    } else if(id==GPMOVE && party_selection>=0) {
        selected=party_selection; move_mode=1; game_redraw(); panel_update(); EndDialog(hwnd,1); return TRUE;
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
    if(spell_map_dialog) { spell_map_resume(); MessageBeep(MB_ICONWARNING); return; }
    if(!map_ready) return;
    if(command==MGAMEHELP) {
        message("Guided combat turns: select Party > Guided combat turns to begin the Hero phase. Move each hero up to Speed by dragging it on the map; attack by selecting or dragging onto a target. Each hero gets one normal attack and may move before or after it. To run instead, select a model and choose Party > Run selected model; the D12 roll adds movement and uses its attack. When the heroes are done, choose Party > End Hero phase / Start GM phase. Do the same for monsters, then end the GM phase. The phase ends only when you choose that menu command. The Party menu shows how many normal attacks have been used. GM override bypasses limits when needed.");
    }
    else if(command==MGAMEGUIDED) {
        remember(); turn_phase=turn_phase?0:1; gm_override=0;
        if(turn_phase) { magic_next_turn(); magic.exploration=0; turn_reset_side(0); turn_reset_side(1); }
        panel_note(turn_phase?"Hero phase started. Select a hero to move or attack.":"Guided turns ended. Free play is active.");
        turn_menu_update(); game_changed();
    }
    else if(command==MGAMESURPRISE) {
        int i,roll; char text[300];
        for(i=0;i<MAX_PICE && i<ENCOUNTER_LIMIT && !magic.surprise[i];i++);
        if(i==MAX_PICE || i==ENCOUNTER_LIMIT) { message("There is no pending surprise bonus from Open Window."); return; }
        if(!magic_die(&roll)) { message("Dice generator unavailable. The bonus remains pending."); return; }
        snprintf(text,sizeof(text),"Section at %d, %d: Leader D12 %d + 3 (Open Window) = %d. Apply this result? Other surprise modifiers are adjudicated by the GM.",Pice[i].x+1,Pice[i].y+1,roll,roll+3);
        if(MessageBoxA(GlMainHwnd,text,"Spied-area surprise",MB_OKCANCEL|MB_ICONINFORMATION)==IDOK) { remember(); magic.surprise[i]=0; magic.spied[i]=0; panel_note(text); game_changed(); }
    }
    else if(command==MGAMEEXPLORATION) {
        remember(); turn_phase=gm_override=0; magic_exploration(); turn_reset_side(0); turn_reset_side(1);
        panel_note("Exploration turn started: 12 squares per model. Dragon Armour, Courage and Flaming Hand have expired.");
        turn_menu_update(); game_changed();
    }
    else if(command==MGAMEOVERRIDE) {
        if(!turn_phase) return;
        remember(); gm_override=!gm_override; turn_menu_update(); game_changed();
    }
    else if(command==MGAMERUN) {
        int side=selected>=0?0:1,index=side?selected_monster:selected,roll;
        HCRYPTPROV provider; char note[120];
        if(!turn_phase) { message("Enable Guided combat turns before running."); return; }
        if(index<0 || (side?index>=monsters.count:index>=hero_count)) { message("Select a Hero or monster on the map first."); return; }
        if(!gm_override && (turn_phase!=(side?2:1) || (side?monsters.tokens[index].attacked:heroes[index].attacked))) {
            message("This model cannot run in the current phase after using its attack."); return;
        }
        if(!CryptAcquireContext(&provider,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT)) { message("Dice generator unavailable."); return; }
        if(!melee_d12(provider,&roll)) { CryptReleaseContext(provider,0); message("Dice generation failed."); return; }
        CryptReleaseContext(provider,0);
        if(!turn_run_apply(side,index,roll)) return;
        sprintf(note,"Run roll: %d. Extra movement: %d squares. This model cannot make a normal attack this phase.",roll,roll==1?0:roll);
        if(panel_hwnd) panel_note(note); else message(note);
    }
    else if(command==MGAMETURN) {
        remember();
        if(turn_phase==1) { turn_phase=2; turn_reset_side(1); }
        else if(turn_phase==2) { magic_next_turn(); turn_phase=1; turn_reset_side(0); }
        else { if(magic.exploration) magic_exploration(); else magic_next_turn(); turn_reset_side(0); turn_reset_side(1); }
        panel_note(turn_phase==1?"Hero phase started. Choose a hero to move or attack.":turn_phase==2?"GM phase started. Choose a monster to move or attack.":"Movement and ranged shots reset.");
        turn_menu_update(); game_changed();
    }
    else if(command==MGAMELEAVE) reserve_party();
    else if(command==MGAMEUNDO && undo_valid) {
        static MONSTER_STATE monster_swap;
        HERO swap[HERO_LIMIT]; int n=hero_count,i; unsigned char *fog=game_fog();
        for(i=0;i<Xsize*Ysize;i++) { PICE *p=NULL; int cell; get_square(i%Xsize,i/Xsize,&p); cell=p?(int)(p-Pice):-1; set_square(i%Xsize,i/Xsize,undo_cells[i]<0?NULL:&Pice[undo_cells[i]]); undo_cells[i]=cell; }
        for(i=0;i<MAX_PICE && i<ENCOUNTER_LIMIT;i++) { PICE p=Pice[i]; Pice[i]=undo_pieces[i]; undo_pieces[i]=p; }
        { static MAGIC_DUNGEON swap_magic; swap_magic=magic; magic=undo_magic; undo_magic=swap_magic; }
        for(i=0;i<ENCOUNTER_LIMIT;i++) { unsigned short pct=room_gold_bonus_pct[i]; room_gold_bonus_pct[i]=undo_gold[i]; undo_gold[i]=pct; }
        for(i=0;fog && i<MAX_PICE && i<ENCOUNTER_LIMIT;i++) {
            unsigned char state=fog[i]; fog[i]=undo_corridor_pieces[i]; undo_corridor_pieces[i]=state;
        }
        monster_swap=monsters; monsters=undo_monsters; undo_monsters=monster_swap; selected_monster=-1;
        memcpy(swap,heroes,sizeof(heroes)); memcpy(heroes,undo_heroes,sizeof(heroes)); memcpy(undo_heroes,swap,sizeof(heroes));
        { int state=turn_phase; turn_phase=undo_turn_phase; undo_turn_phase=state;
          state=gm_override; gm_override=undo_gm_override; undo_gm_override=state; }
        hero_count=undo_count; undo_count=n; attack_monster=-1; monster_move_mode=0; selected=-1; move_mode=0; game_changed();
        turn_menu_update();
    }
}

#include "monster-ui.inc"
#include "combat-panel.inc"
