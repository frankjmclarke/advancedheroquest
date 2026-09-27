/* Persistent controls beside the MDI map. The panel shares the menu commands
 * and selection state, so every action still goes through the game rules. */
static WNDPROC panel_frame_previous;
static int panel_width;
static char panel_last_notice[200]="Select a model on the map to begin.";

static void panel_layout(void)
{
    RECT client; int width;
    if(!panel_hwnd || !IsWindow(panel_hwnd) || !IsWindow(GlMainHwnd) || !IsWindow(MdiClientHwnd)) return;
    GetClientRect(GlMainHwnd,&client);
    width=panel_width;
    if(width>client.right/2) width=client.right/2;
    if(width<1) return;
    MoveWindow(MdiClientHwnd,0,0,client.right-width,client.bottom,TRUE);
    MoveWindow(panel_hwnd,client.right-width,0,width,client.bottom,TRUE);
}
static LRESULT CALLBACK panel_frame_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
    WNDPROC previous=panel_frame_previous;
    LRESULT result;
    if(msg==WM_NCDESTROY) {
        SetWindowLongPtr(hwnd,GWLP_WNDPROC,(LONG_PTR)previous);
        panel_frame_previous=NULL; panel_hwnd=NULL;
        return CallWindowProc(previous,hwnd,msg,wp,lp);
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
    else strcpy(line,"FREE PLAY");
    SetDlgItemTextA(panel_hwnd,CPPHASE,line);
    sprintf(line,"Selected: %s",name); SetDlgItemTextA(panel_hwnd,CPMODEL,line);
    if(side>=0) sprintf(line,"Wounds: %d / %d",wounds,max_wounds); else strcpy(line,"Wounds: -");
    SetDlgItemTextA(panel_hwnd,CPWOUNDS,line);
    if(side<0) strcpy(line,"Movement: -");
    else if(!turn_phase) strcpy(line,"Movement: free play");
    else sprintf(line,"Movement: %d squares remaining",remaining>0?remaining:0);
    SetDlgItemTextA(panel_hwnd,CPMOVELEFT,line);
    if(side<0) strcpy(line,"Normal attack: -");
    else if(!turn_phase) strcpy(line,"Normal attack: free play");
    else sprintf(line,"Normal attack: %s%s",attack_used?"used":"available",gm_override?" (GM override)":"");
    SetDlgItemTextA(panel_hwnd,CPATTACKLEFT,line);
    EnableWindow(GetDlgItem(panel_hwnd,CPMOVE),side>=0 && alive && turn_can_move(side,index));
    EnableWindow(GetDlgItem(panel_hwnd,CPATTACK),side>=0 && alive && !turn_attack_reason(side,side?0:index,side?index:0));
    EnableWindow(GetDlgItem(panel_hwnd,CPRUN),turn_phase && side>=0 && alive && !turn_attack_reason(side,side?0:index,side?index:0));
    EnableWindow(GetDlgItem(panel_hwnd,CPENDPHASE),map_ready && turn_phase);
    EnableWindow(GetDlgItem(panel_hwnd,CPUNDO),map_ready && undo_valid);
    EnableWindow(GetDlgItem(panel_hwnd,CPGUIDED),map_ready);
    SetDlgItemTextA(panel_hwnd,CPENDPHASE,turn_phase==1?"End Hero phase":turn_phase==2?"End GM phase":"End phase");
    SetDlgItemTextA(panel_hwnd,CPGUIDED,turn_phase?"Leave guided turns":"Start guided turns");
    if(!map_ready) next="Open or generate a dungeon to begin.";
    else if(side<0) next=turn_phase==2?"Select a monster on the map.":"Select a hero on the map.";
    else if(!alive) next="This model cannot act. Select another model.";
    else if(turn_phase && turn_phase!=(side?2:1) && !gm_override) next=side?"Hero phase: select a hero or end the phase.":"GM phase: select a monster or end the phase.";
    else if(side && monster_move_mode) next="Click an open square to move the monster.";
    else if((side?attack_monster==index:move_mode && selected==index)) next=side?"Click a hero to attack, or drag the monster to move.":"Click an enemy to attack, or click a square to move.";
    else if(turn_phase && attack_used && remaining<=0 && !gm_override) next="This model is done. Select another or end the phase.";
    else if(turn_phase && attack_used && !gm_override) next="Normal attack used. Move or end the phase.";
    else if(turn_phase && remaining<=0 && !gm_override) next="Movement used. Attack or end the phase.";
    else next=turn_phase?"Choose Move, Attack, or Run. Then use the map.":"Choose Move or Attack. Then use the map.";
    SetDlgItemTextA(panel_hwnd,CPNEXT,next);
    SetDlgItemTextA(panel_hwnd,CPNOTICE,panel_last_notice);
}
static void panel_note(const char *note)
{
    strncpy(panel_last_notice,note,sizeof(panel_last_notice)-1);
    panel_last_notice[sizeof(panel_last_notice)-1]=0;
    panel_update();
}
static INT_PTR CALLBACK panel_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
    int id=LOWORD(wp),side=selected>=0 && selected<hero_count?0:selected_monster>=0 && selected_monster<monsters.count?1:-1;
    if(msg==WM_INITDIALOG) return TRUE;
    if(msg!=WM_COMMAND) return FALSE;
    if(id==CPGUIDED) game_command(MGAMEGUIDED);
    else if(id==CPENDPHASE) game_command(MGAMETURN);
    else if(id==CPUNDO) game_command(MGAMEUNDO);
    else if(id==CPRUN) game_command(MGAMERUN);
    else if(id==CPMOVE && side>=0) {
        if(!turn_can_move(side,side?selected_monster:selected)) return TRUE;
        attack_monster=-1; move_mode=side?0:1; monster_move_mode=side?1:0;
        panel_note(side?"Click an open square or drag the selected monster.":"Click a destination square or drag the selected hero.");
    } else if(id==CPATTACK && side>=0) {
        int index=side?selected_monster:selected;
        if(turn_attack_reason(side,side?0:index,side?index:0)) return TRUE;
        monster_move_mode=0;
        if(side) { attack_monster=index; move_mode=0; }
        else { attack_monster=-1; move_mode=1; }
        panel_note(side?"Click a hero on the map to attack.":"Click an enemy on the map to attack.");
    } else return FALSE;
    panel_update(); game_redraw(); return TRUE;
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
    panel_layout(); panel_update();
}
