/* Persistent controls beside the MDI map. The panel shares the menu commands
 * and selection state, so every action still goes through the game rules. */
static WNDPROC panel_frame_previous;
static int panel_width;
static HWND toolbar_hwnd;
static HIMAGELIST toolbar_images;
static int toolbar_height=76;
static char panel_last_notice[200]="Select a model on the map to begin.";

static void panel_layout(void)
{
    RECT client; int width,top;
    if(!panel_hwnd || !IsWindow(panel_hwnd) || !IsWindow(GlMainHwnd) || !IsWindow(MdiClientHwnd)) return;
    GetClientRect(GlMainHwnd,&client);
    width=panel_width;
    if(width>client.right/2) width=client.right/2;
    if(width<1) return;
    top=toolbar_hwnd?toolbar_height:0;
    if(toolbar_hwnd) MoveWindow(toolbar_hwnd,0,0,client.right,toolbar_height,TRUE);
    MoveWindow(MdiClientHwnd,0,top,client.right-width,client.bottom-top,TRUE);
    MoveWindow(panel_hwnd,client.right-width,top,width,client.bottom-top,TRUE);
}
static const char *toolbar_tip(int id)
{
    switch(id) {
    case CPUNDO: return "Undo the most recent token or turn change.";
    case CPENDPHASE: return "Finish this side's phase when its models are done.";
    case CPTOOLPARTY: return "Open Heroes and Reserve.";
    case CPTOOLVIEW: return game_player_view()?"Show GM map without fog of war.":"Show Player View with fog of war.";
    default: return "";
    }
}
static LRESULT CALLBACK panel_frame_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
    WNDPROC previous=panel_frame_previous;
    LRESULT result;
    if(msg==WM_COMMAND && spell_map_dialog && LOWORD(wp)!=CPTOOLVIEW &&
       !(LOWORD(wp)>=MZOOMIN && LOWORD(wp)<=MZOOMFIT)) {
        spell_map_resume(); MessageBeep(MB_ICONWARNING); return 0;
    }
    if(msg==WM_NCDESTROY) {
        if(toolbar_images) { ImageList_Destroy(toolbar_images); toolbar_images=NULL; }
        SetWindowLongPtr(hwnd,GWLP_WNDPROC,(LONG_PTR)previous);
        panel_frame_previous=NULL; panel_hwnd=NULL; toolbar_hwnd=NULL;
        return CallWindowProc(previous,hwnd,msg,wp,lp);
    }
    if(msg==WM_COMMAND && (HWND)lp==toolbar_hwnd && combat_action(LOWORD(wp))) return 0;
    if(msg==WM_NOTIFY && toolbar_hwnd && lp) {
        NMHDR *hdr=(NMHDR*)lp;
        if(hdr->hwndFrom==toolbar_hwnd && hdr->code==TBN_GETINFOTIPA) {
            NMTBGETINFOTIPA *tip=(NMTBGETINFOTIPA*)lp;
            if(tip->pszText && tip->cchTextMax>0) lstrcpynA(tip->pszText,toolbar_tip(tip->iItem),tip->cchTextMax);
            return 0;
        }
        if(hdr->code==TTN_GETDISPINFOA && hdr->idFrom>=CPUNDO && hdr->idFrom<=CPTOOLVIEW) {
            NMTTDISPINFOA *tip=(NMTTDISPINFOA*)lp;
            tip->lpszText=(LPSTR)toolbar_tip((int)tip->hdr.idFrom); return 0;
        }
    }
    result=CallWindowProc(previous,hwnd,msg,wp,lp);
    if(msg==WM_SIZE) panel_layout();
    return result;
}
static void panel_update(void)
{
    int side=selected>=0 && selected<hero_count?0:selected_monster>=0 && selected_monster<monsters.count?1:-1;
    int index=side==0?selected:selected_monster,alive=0,remaining=0,attack_used=0,wounds=0,max_wounds=0;
    int i,used=0,total=0;
    const char *name="Select a model on the map",*next; char line[180];
    if(!panel_hwnd || !IsWindow(panel_hwnd)) return;
    if(!map_ready) side=-1;
    if(side==0) {
        HERO *h=&heroes[index]; name=h->name; alive=HERO_ACTIVE(h) && h->x>=0;
        wounds=h->wounds; max_wounds=h->stats[7]; remaining=turn_move_remaining(0,index); attack_used=h->attacked;
    } else if(side==1) {
        MONSTER *m=&monsters.tokens[index]; name=m->name; alive=m->wounds>0 && m->x>=0;
        wounds=m->wounds; max_wounds=m->stats[7]; remaining=turn_move_remaining(1,index); attack_used=m->attacked;
    }
    if(turn_phase==1) for(i=0;i<hero_count;i++) {
        if(heroes[i].x>=0 && HERO_ACTIVE(&heroes[i])) { total++; if(heroes[i].attacked) used++; }
    } else if(turn_phase==2) for(i=0;i<monsters.count;i++) {
        if(monsters.tokens[i].x>=0 && monsters.tokens[i].wounds) { total++; if(monsters.tokens[i].attacked) used++; }
    }
    if(!map_ready) strcpy(line,"No dungeon open");
    else if(turn_phase) sprintf(line,"%s PHASE | %d/%d attacks used%s",turn_phase==1?"HERO":"GM",used,total,gm_override?" | GM override":"");
    else snprintf(line,sizeof(line),"%s | Turn %d",magic.exploration?"EXPLORATION":"FREE PLAY",magic.turn+1);
    SetDlgItemTextA(panel_hwnd,CPPHASE,line);
    sprintf(line,"Selected: %s",name); SetDlgItemTextA(panel_hwnd,CPMODEL,line);
    if(side>=0) {
        const int *stats=side?monsters.tokens[index].stats:heroes[index].stats; MAGIC_MODEL *m=magic_model(side,index);
        snprintf(line,sizeof(line),"Wounds %d/%d | T %d | Br %d",wounds,max_wounds,magic_toughness(stats[3],m),magic_bravery(stats[5],m));
    } else strcpy(line,"Wounds: -");
    SetDlgItemTextA(panel_hwnd,CPWOUNDS,line);
    if(side<0) strcpy(line,"Movement: -");
    else if(!turn_phase && !magic.exploration && !magic_model(side,index)->swift) strcpy(line,"Movement: free play");
    else sprintf(line,"Movement: %d squares remaining",remaining>0?remaining:0);
    SetDlgItemTextA(panel_hwnd,CPMOVELEFT,line);
    if(side<0) strcpy(line,"Normal attack: -");
    else if(!turn_phase) strcpy(line,"Normal attack: free play");
    else sprintf(line,"Normal attack: %s%s",attack_used?"used":"available",gm_override?" (GM override)":"");
    SetDlgItemTextA(panel_hwnd,CPATTACKLEFT,line);
    if(side>=0 && caster_has_book(caster_state(side,index))) {
        CASTER_STATE *s=caster_state(side,index);
        snprintf(line,sizeof(line),"Spell: %s%s",s->setup_pending?"component setup needed":s->cast_used?"used this turn":"ready",s->move_locked?"; no movement":"");
    } else strcpy(line,"Spell: no spellbook");
    SetDlgItemTextA(panel_hwnd,CPSPELLSTATUS,line);
    EnableWindow(GetDlgItem(panel_hwnd,CPSPELLCAST),side>=0 && alive && caster_has_book(caster_state(side,index)));
    EnableWindow(GetDlgItem(panel_hwnd,CPSPELLBOOK),side>=0 && pack_current() && pack_current()->book_count);
    EnableWindow(GetDlgItem(panel_hwnd,CPRUN),turn_phase && side>=0 && alive && !turn_attack_reason(side,side?0:index,side?index:0));
    EnableWindow(GetDlgItem(panel_hwnd,CPENDPHASE),map_ready && (turn_phase || magic.exploration));
    EnableWindow(GetDlgItem(panel_hwnd,CPUNDO),map_ready && undo_valid);
    EnableWindow(GetDlgItem(panel_hwnd,CPGUIDED),map_ready);
    if(toolbar_hwnd) {
        SendMessage(toolbar_hwnd,TB_ENABLEBUTTON,CPUNDO,MAKELONG(map_ready && undo_valid,0));
        SendMessage(toolbar_hwnd,TB_ENABLEBUTTON,CPENDPHASE,MAKELONG(map_ready && (turn_phase || magic.exploration),0));
        SendMessage(toolbar_hwnd,TB_ENABLEBUTTON,CPTOOLPARTY,MAKELONG(map_ready,0));
        SendMessage(toolbar_hwnd,TB_ENABLEBUTTON,CPTOOLVIEW,MAKELONG(map_ready,0));
        SendMessage(toolbar_hwnd,TB_CHECKBUTTON,CPTOOLVIEW,MAKELONG(game_player_view(),0));
    }
    SetDlgItemTextA(panel_hwnd,CPENDPHASE,turn_phase==1?"End Hero phase":turn_phase==2?"End GM phase":magic.exploration?"Next exploration turn":"End phase");
    SetDlgItemTextA(panel_hwnd,CPGUIDED,turn_phase?"Leave guided turns":"Start guided turns");
    if(!map_ready) next="Open or generate a dungeon to begin.";
    else if(side<0) next=turn_phase==2?"Select a monster on the map.":"Select a hero on the map.";
    else if(!alive) next="This model cannot act. Select another model.";
    else if(turn_phase && turn_phase!=(side?2:1) && !gm_override) next=side?"Hero phase: select a hero or end the phase.":"GM phase: select a monster or end the phase.";
    else if(side==1 && !gm_override && magic_stopped(index)) next="Still Air: this monster cannot move or attack until the next full turn.";
    else if(side && monster_move_mode) next="Click an open square to move the monster.";
    else if((side?attack_monster==index:move_mode && selected==index)) next=side?"Click a hero to attack, or drag the monster to move.":"Click an enemy to attack, or click a square to move.";
    else if(side>=0 && caster_has_book(caster_state(side,index)) && !caster_state(side,index)->cast_used && turn_phase && attack_used) next="Weapon attack used. Cast a spell or end the phase.";
    else if(turn_phase && attack_used && remaining<=0 && !gm_override) next="This model is done. Select another or end the phase.";
    else if(turn_phase && attack_used && !gm_override) next="Normal attack used. Move or end the phase.";
    else if(turn_phase && remaining<=0 && !gm_override) next="Movement used. Attack or end the phase.";
    else next="Drag a model to move. Select it, then click an enemy to attack.";
    SetDlgItemTextA(panel_hwnd,CPNEXT,next);
    SetDlgItemTextA(panel_hwnd,CPNOTICE,panel_last_notice);
}
static void panel_note(const char *note)
{
    strncpy(panel_last_notice,note,sizeof(panel_last_notice)-1);
    panel_last_notice[sizeof(panel_last_notice)-1]=0;
    panel_update();
}
static int combat_action(int id)
{
    int side=selected>=0 && selected<hero_count?0:selected_monster>=0 && selected_monster<monsters.count?1:-1;
    int index=side==0?selected:selected_monster;
    if(spell_map_dialog && id!=CPTOOLVIEW) { spell_map_resume(); MessageBeep(MB_ICONWARNING); return 1; }
    if(id==CPSPELLCAST && side>=0) spellcast_open(GlMainHwnd,side,index);
    else if(id==CPSPELLBOOK && side>=0) spellbook_open(GlMainHwnd,side,index);
    else if(id==CPGUIDED) game_command(MGAMEGUIDED);
    else if(id==CPENDPHASE) game_command(MGAMETURN);
    else if(id==CPUNDO) game_command(MGAMEUNDO);
    else if(id==CPRUN && side>=0 && turn_phase &&
            !turn_attack_reason(side,side?0:index,side?index:0)) game_command(MGAMERUN);
    else if(id==CPTOOLPARTY) game_party();
    else if(id==CPTOOLVIEW) game_toggle_view();
    else if(id==CPTOKENDETAILS && side>=0) {
        if(side) game_monsters(); else game_party();
    } else if(id==CPTOKENWOUND && side==1) monster_damage(index);
    else if(id==CPTOKENRESERVE && side==0 && heroes[index].x>=0) {
        remember(); heroes[index].x=heroes[index].y=-1;
        selected=-1; move_mode=0;
        panel_note("Hero moved to reserve. Undo restores the placement."); game_changed();
    } else return 0;
    panel_update(); game_redraw(); return 1;
}
static INT_PTR CALLBACK panel_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
    if(msg==WM_INITDIALOG) return TRUE;
    if(msg==WM_COMMAND) return combat_action(LOWORD(wp));
    return FALSE;
}
static void toolbar_create(void)
{
    static const int commands[]={CPUNDO,CPENDPHASE,CPTOOLPARTY,CPTOOLVIEW};
    static const int icons[]={3,4,5,6};
    static const char labels[]="Undo\0End phase\0Party\0Player view\0\0";
    INITCOMMONCONTROLSEX init; TBBUTTON buttons[4]; HBITMAP strip,mask; int i,base;
    init.dwSize=sizeof(init); init.dwICC=ICC_BAR_CLASSES;
    if(!InitCommonControlsEx(&init)) return;
    toolbar_hwnd=CreateWindowExA(0,TOOLBARCLASSNAMEA,"",WS_CHILD|WS_VISIBLE|TBSTYLE_FLAT|TBSTYLE_TOOLTIPS|CCS_NORESIZE|CCS_NODIVIDER,
        0,0,100,toolbar_height,GlMainHwnd,NULL,GetModuleHandle(NULL),NULL);
    if(!toolbar_hwnd) return;
    SendMessage(toolbar_hwnd,TB_BUTTONSTRUCTSIZE,sizeof(TBBUTTON),0);
    SendMessage(toolbar_hwnd,TB_SETBITMAPSIZE,0,MAKELONG(48,48));
    strip=LoadBitmapA(GetModuleHandle(NULL),MAKEINTRESOURCEA(IDB_TOOLBAR_ICONS));
    mask=LoadImageA(GetModuleHandle(NULL),MAKEINTRESOURCEA(IDB_TOOLBAR_MASK),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
    if(strip && mask) {
        toolbar_images=ImageList_Create(48,48,ILC_COLOR24|ILC_MASK,7,0);
        if(toolbar_images && ImageList_Add(toolbar_images,strip,mask)==0)
            SendMessage(toolbar_hwnd,TB_SETIMAGELIST,0,(LPARAM)toolbar_images);
        else if(toolbar_images) { ImageList_Destroy(toolbar_images); toolbar_images=NULL; }
    }
    if(strip) DeleteObject(strip);
    if(mask) DeleteObject(mask);
    SendMessage(toolbar_hwnd,TB_SETMAXTEXTROWS,1,0);
    base=(int)SendMessageA(toolbar_hwnd,TB_ADDSTRINGA,0,(LPARAM)labels);
    memset(buttons,0,sizeof(buttons));
    for(i=0;i<4;i++) {
        buttons[i].iBitmap=toolbar_images?icons[i]:I_IMAGENONE; buttons[i].idCommand=commands[i];
        buttons[i].fsState=TBSTATE_ENABLED; buttons[i].fsStyle=(commands[i]==CPTOOLVIEW?BTNS_CHECK:BTNS_BUTTON)|BTNS_SHOWTEXT;
        buttons[i].iString=base+i;
    }
    SendMessage(toolbar_hwnd,TB_ADDBUTTONS,4,(LPARAM)buttons);
    SendMessage(toolbar_hwnd,TB_SETBUTTONSIZE,0,MAKELONG(88,70));
    SendMessage(toolbar_hwnd,TB_AUTOSIZE,0,0);
}
static HMENU token_menu_build(int side,int index)
{
    HMENU menu; int alive,can_attack_now;
    if(side==0) {
        if(index<0 || index>=hero_count) return NULL;
        alive=HERO_ACTIVE(&heroes[index]) && heroes[index].x>=0;
    } else {
        if(index<0 || index>=monsters.count) return NULL;
        alive=monsters.tokens[index].wounds>0 && monsters.tokens[index].x>=0;
    }
    can_attack_now=alive && !turn_attack_reason(side,side?0:index,side?index:0);
    menu=CreatePopupMenu(); if(!menu) return NULL;
    AppendMenuA(menu,MF_STRING|(turn_phase && can_attack_now?0:MF_GRAYED),CPRUN,"Run (D12)");
    if(pack_current() && pack_current()->book_count) {
        AppendMenuA(menu,MF_STRING|(alive && caster_has_book(caster_state(side,index))?0:MF_GRAYED),CPSPELLCAST,"Cast spell...");
        AppendMenuA(menu,MF_STRING,CPSPELLBOOK,"Spellbook & components...");
    }
    AppendMenuA(menu,MF_SEPARATOR,0,NULL);
    AppendMenuA(menu,MF_STRING,CPTOKENDETAILS,side?"Monster profile and wounds...":"Hero profile and wounds...");
    if(side) AppendMenuA(menu,MF_STRING,CPTOKENWOUND,"Inflict 1 wound");
    else AppendMenuA(menu,MF_STRING|(!alive?MF_GRAYED:0),CPTOKENRESERVE,"Send to reserve");
    return menu;
}
static void token_context_menu(HWND map,int side,int index,int screen_x,int screen_y)
{
    HMENU menu=token_menu_build(side,index); int command;
    if(!menu) return;
    if(side) { selected=-1; selected_monster=index; }
    else { selected=index; selected_monster=-1; }
    move_mode=monster_move_mode=0; attack_monster=-1;
    game_redraw(); panel_update();
    SetForegroundWindow(GlMainHwnd);
    command=TrackPopupMenuEx(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,screen_x,screen_y,map,NULL);
    DestroyMenu(menu);
    if(command) combat_action(command);
}
static void panel_attach(void)
{
    RECT rect;
    if(panel_hwnd && IsWindow(panel_hwnd)) { panel_update(); return; }
    if(!IsWindow(GlMainHwnd) || !IsWindow(MdiClientHwnd)) return;
    panel_hwnd=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGCOMBATPANEL),GlMainHwnd,panel_proc,0);
    if(!panel_hwnd) return;
    GetWindowRect(panel_hwnd,&rect); panel_width=rect.right-rect.left;
    panel_frame_previous=(WNDPROC)SetWindowLongPtr(GlMainHwnd,GWLP_WNDPROC,(LONG_PTR)panel_frame_proc);
    toolbar_create();
    panel_layout(); panel_update();
}
