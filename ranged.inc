typedef struct { RANGED_PROFILE profile; int moved,hero_kind; const char *reference; } RANGED_EDIT;
static void ranged_profile_fields(HWND hwnd,const RANGED_EDIT *p)
{
    int i; SetDlgItemTextA(hwnd,RPWEAPON,p->profile.weapon);
    SendDlgItemMessage(hwnd,RPKIND,CB_SETCURSEL,p->profile.kind,0);
    SetDlgItemInt(hwnd,RPRANGE,p->profile.range,FALSE); SetDlgItemInt(hwnd,RPDICE,p->profile.dice,FALSE);
    SetDlgItemInt(hwnd,RPCRIT,p->profile.critical?p->profile.critical:12,FALSE);
    SetDlgItemInt(hwnd,RPFUMBLE,p->profile.fumble?p->profile.fumble:1,FALSE);
    for(i=0;i<5;i++) SetDlgItemInt(hwnd,RPHIT+i,p->profile.hit[i],FALSE);
}
static INT_PTR CALLBACK ranged_profile_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
    RANGED_EDIT *p=(RANGED_EDIT*)GetWindowLongPtr(hwnd,DWLP_USER),next;
    int i,n,id=LOWORD(wp),critical,fumble; BOOL ok;
    static const char *kinds[]={"None equipped","Bow","Crossbow","Thrown weapon","Other (manual movement)"};
    if(msg==WM_INITDIALOG) {
        p=(RANGED_EDIT*)lp; SetWindowLongPtr(hwnd,DWLP_USER,lp);
        for(i=0;i<5;i++) SendDlgItemMessageA(hwnd,RPKIND,CB_ADDSTRING,0,(LPARAM)kinds[i]);
        SendDlgItemMessage(hwnd,RPWEAPON,EM_LIMITTEXT,39,0);
        CheckDlgButton(hwnd,RPMOVED,p->moved?BST_CHECKED:BST_UNCHECKED);
        ranged_profile_fields(hwnd,p);
        SetDlgItemTextA(hwnd,RPINFO,"Bows/crossbows require no movement. Thrown weapons permit movement."); return TRUE;
    }
    if(msg==WM_CLOSE || (msg==WM_COMMAND && id==IDCANCEL)) { EndDialog(hwnd,0); return TRUE; }
    if(msg!=WM_COMMAND || !p) return FALSE;
    if(id==RPDEFAULT) {
        next=*p;
        if(p->hero_kind>=0) hero_ranged_defaults(&next.profile,p->hero_kind);
        else if(!p->reference || !monster_ranged_defaults(&next.profile,p->reference)) {
            SetDlgItemTextA(hwnd,RPINFO,"No unambiguous ranged reference. Enter equipment manually."); return TRUE;
        }
        ranged_profile_fields(hwnd,&next);
        SetDlgItemTextA(hwnd,RPINFO,next.profile.kind?"Reference loaded. Save to keep it.":"No ranged weapon is listed for this class preset. You may equip one manually."); return TRUE;
    }
    if(id!=IDOK) return FALSE;
    next=*p; GetDlgItemTextA(hwnd,RPWEAPON,next.profile.weapon,40);
    next.profile.kind=(int)SendDlgItemMessage(hwnd,RPKIND,CB_GETCURSEL,0,0);
    next.moved=IsDlgButtonChecked(hwnd,RPMOVED)==BST_CHECKED;
    critical=(int)GetDlgItemInt(hwnd,RPCRIT,&ok,FALSE);
    if(!ok || critical<1 || critical>12) { SetDlgItemTextA(hwnd,RPINFO,"Critical threshold must be 1-12."); return TRUE; }
    fumble=(int)GetDlgItemInt(hwnd,RPFUMBLE,&ok,FALSE);
    if(!ok || fumble<1 || fumble>=critical) { SetDlgItemTextA(hwnd,RPINFO,"Fumble must be 1-12 and below the critical threshold."); return TRUE; }
    next.profile.critical=critical; next.profile.fumble=fumble;
    for(i=0;i<7;i++) {
        n=(int)GetDlgItemInt(hwnd,i<5?RPHIT+i:i==5?RPRANGE:RPDICE,&ok,FALSE);
        if(!ok || n<0 || n>(i<5?12:i==5?480:99)) {
            SetDlgItemTextA(hwnd,RPINFO,"Use hit values 0-12, range 0-480 and damage dice 0-99."); return TRUE;
        }
        if(i<5) next.profile.hit[i]=n; else if(i==5) next.profile.range=n; else next.profile.dice=n;
    }
    if(next.profile.kind<0 || next.profile.kind>4) return TRUE;
    *p=next; EndDialog(hwnd,1); return TRUE;
}
static int edit_ranged(HWND owner,RANGED_PROFILE *profile,int *moved,int kind,const char *reference)
{
    RANGED_EDIT p; p.profile=*profile; p.moved=*moved; p.hero_kind=kind; p.reference=reference;
    if(DialogBoxParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGRANGEDPROFILE),owner,ranged_profile_proc,(LPARAM)&p)!=1) return 0;
    *profile=p.profile; *moved=p.moved; return 1;
}
typedef struct {
    HERO hero; MONSTER monster; int h,m,side,resolved,new_wounds,old_wounds,new_condition;
    int fumble,friendly_index,friendly_side,ally_options;
    char result[2600];
} SHOT;
static RANGED_PROFILE *shot_profile(SHOT *s) { return s->side?&s->monster.ranged:&s->hero.ranged; }
static int shot_critical(SHOT *s) { return shot_profile(s)->critical?shot_profile(s)->critical:12; }
static int shot_fumble(SHOT *s) { return shot_profile(s)->fumble?shot_profile(s)->fumble:1; }
static int shot_range(const SHOT *s) { return abs(s->hero.x-s->monster.x)+abs(s->hero.y-s->monster.y); }
static int shot_candidates(const SHOT *s)
{
    int i,n=0,x=s->side?s->hero.x:s->monster.x,y=s->side?s->hero.y:s->monster.y;
    if(s->side) {
        for(i=0;i<hero_count;i++) if(i!=s->h && HERO_ACTIVE(&heroes[i]) && heroes[i].x>=0 &&
            abs(heroes[i].x-x)+abs(heroes[i].y-y)<=2) n++;
    } else {
        for(i=0;i<monsters.count;i++) if(i!=s->m && monsters.tokens[i].wounds && monsters.tokens[i].x>=0 &&
            abs(monsters.tokens[i].x-x)+abs(monsters.tokens[i].y-y)<=2) n++;
    }
    return n;
}
static void shot_ally_options(HWND hwnd,SHOT *s)
{
    int i,x=s->side?s->hero.x:s->monster.x,y=s->side?s->hero.y:s->monster.y;
    char label[96]; HWND combo=GetDlgItem(hwnd,RSALLY);
    SendMessage(combo,CB_RESETCONTENT,0,0);
    if(s->side) {
        for(i=0;i<hero_count;i++) if(i!=s->h && HERO_ACTIVE(&heroes[i]) && heroes[i].x>=0 && abs(heroes[i].x-x)+abs(heroes[i].y-y)<=2) {
            sprintf(label,"%s (%d Wounds)",heroes[i].name,heroes[i].wounds);
            { int at=(int)SendMessageA(combo,CB_ADDSTRING,0,(LPARAM)label); SendMessage(combo,CB_SETITEMDATA,at,(LPARAM)i); }
        }
    } else {
        for(i=0;i<monsters.count;i++) if(i!=s->m && monsters.tokens[i].wounds && monsters.tokens[i].x>=0 && abs(monsters.tokens[i].x-x)+abs(monsters.tokens[i].y-y)<=2) {
            sprintf(label,"%s (%d Wounds)",monsters.tokens[i].name,monsters.tokens[i].wounds);
            { int at=(int)SendMessageA(combo,CB_ADDSTRING,0,(LPARAM)label); SendMessage(combo,CB_SETITEMDATA,at,(LPARAM)i); }
        }
    }
    SendMessage(combo,CB_SETCURSEL,(WPARAM)-1,0); s->ally_options=1;
    ShowWindow(combo,SW_SHOW); ShowWindow(GetDlgItem(hwnd,RSALLYLABEL),SW_SHOW);
}
static void shot_ally_clear(HWND hwnd,SHOT *s)
{
    s->ally_options=0; s->friendly_index=-1;
    SendDlgItemMessage(hwnd,RSALLY,CB_RESETCONTENT,0,0);
    ShowWindow(GetDlgItem(hwnd,RSALLY),SW_HIDE); ShowWindow(GetDlgItem(hwnd,RSALLYLABEL),SW_HIDE);
}
static const char *shot_blocked(SHOT *s)
{
    const char *turn=turn_attack_reason(s->side,s->h,s->m);
    if(turn) return turn;
    if(!s->side && heroes[s->h].fired && !(turn_phase && gm_override)) return "This hero has already fired this turn. Choose Next Turn to fire again.";
    return ranged_eligibility(s->h,s->m,s->side,shot_profile(s),turn_phase && gm_override?0:s->side?s->monster.moved:s->hero.moved);
}
static void shot_fields(HWND hwnd,SHOT *s)
{
    char info[600]; const char *blocked=shot_blocked(s); RANGED_PROFILE *p=shot_profile(s);
    int d=shot_range(s),i; const int inputs[]={RSHIT,RSDAMAGE,RSPROFILE,RSROLL};
    sprintf(info,"%s fires at %s\r\n%s | Distance %d / range %d | Hit %d+ | Damage %d D12\r\nCritical %d+; fumble %d or less. Target T %d; %d Wounds. %s",
        s->side?s->monster.name:s->hero.name,s->side?s->hero.name:s->monster.name,
        p->weapon[0]?p->weapon:"No ranged weapon",d,p->range,p->hit[ranged_band(d)],p->dice,
        p->critical?p->critical:12,p->fumble?p->fumble:1,s->side?s->hero.stats[3]:s->monster.stats[3],s->side?s->hero.wounds:s->monster.wounds,
        s->side?s->monster.moved?"Attacker moved.":"Attacker stationary.":s->hero.moved?"Attacker moved.":"Attacker stationary.");
    SetDlgItemTextA(hwnd,RSINFO,info);
    for(i=0;i<sizeof(inputs)/sizeof(inputs[0]);i++) EnableWindow(GetDlgItem(hwnd,inputs[i]),!s->resolved);
    EnableWindow(GetDlgItem(hwnd,RSROLL),!s->resolved && !blocked);
    EnableWindow(GetDlgItem(hwnd,IDOK),s->resolved && !blocked);
    EnableWindow(GetDlgItem(hwnd,RSNEXTTURN),!s->resolved);
    SetDlgItemTextA(hwnd,RSNEXTTURN,turn_phase?"Next phase":"Next Turn");
    SetDlgItemTextA(hwnd,RSSTATUS,blocked?blocked:s->resolved?"Preview ready. Apply or Cancel.":"Choose Roll attack to preview the result.");
    SetDlgItemTextA(hwnd,RSRESULT,s->result);
}
static void shot_victim(SHOT *s,int *t,int *w,const char **name)
{
    *t=s->side?s->hero.stats[3]:s->monster.stats[3]; *w=s->side?s->hero.wounds:s->monster.wounds;
    *name=s->side?s->hero.name:s->monster.name;
}
static int shot_calculate(HWND hwnd,SHOT *s)
{
    int hit,required=shot_profile(s)->hit[ranged_band(shot_range(s))],t,w,damage=0,needs_damage,n;
    BOOL ok; const char *name,*blocked=shot_blocked(s); char dice[2048];
    if(s->resolved) return 0;
    if(blocked) { SetDlgItemTextA(hwnd,RSSTATUS,blocked); return 0; }
    hit=(int)GetDlgItemInt(hwnd,RSHIT,&ok,FALSE);
    if(!ok || hit<1 || hit>12) { SetDlgItemTextA(hwnd,RSSTATUS,"Enter a hit roll from 1 to 12."); return 0; }
    s->fumble=hit<=shot_fumble(s); s->friendly_index=-1; s->friendly_side=s->side?0:1;
    if(!s->fumble || !shot_candidates(s)) shot_ally_clear(hwnd,s);
    if(s->fumble && shot_candidates(s)) {
        if(!s->ally_options) shot_ally_options(hwnd,s);
        n=(int)SendDlgItemMessage(hwnd,RSALLY,CB_GETCURSEL,0,0);
        if(n<0) { SetDlgItemTextA(hwnd,RSSTATUS,"The intended target's controller must choose a nearby friendly model."); return 0; }
        s->friendly_index=(int)SendDlgItemMessage(hwnd,RSALLY,CB_GETITEMDATA,n,0);
    }
    if(s->fumble && s->friendly_index<0) {
        t=0; w=0; name=shot_candidates(s)?"Defender chose no friendly victim; the missile misses":"No friendly model within two squares; the missile misses";
    } else if(s->fumble && s->friendly_side==0) {
        t=heroes[s->friendly_index].stats[3]; w=heroes[s->friendly_index].wounds; name=heroes[s->friendly_index].name;
    } else if(s->fumble) {
        t=monsters.tokens[s->friendly_index].stats[3]; w=monsters.tokens[s->friendly_index].wounds; name=monsters.tokens[s->friendly_index].name;
    } else shot_victim(s,&t,&w,&name);
    if(!s->fumble && hit>=shot_critical(s)) t=t/2;
    needs_damage=!s->fumble && hit>=required;
    if(s->fumble && s->friendly_index>=0) needs_damage=1;
    GetDlgItemTextA(hwnd,RSDAMAGE,dice,sizeof(dice));
    if(needs_damage && !damage_auto(dice,sizeof(dice),shot_profile(s)->dice,t,&damage)) {
        SetDlgItemTextA(hwnd,RSSTATUS,"Enter the initial damage dice, each from 1 to 12. Bonus dice are automatic. If rolling fails, retry."); return 0;
    }
    SetDlgItemTextA(hwnd,RSDAMAGE,dice);
    s->old_wounds=w; s->new_wounds=damage>w?0:w-damage;
    s->new_condition=(s->fumble && s->friendly_index>=0 && s->friendly_side==0)?heroes[s->friendly_index].condition:s->side?s->hero.condition:0;
    if((s->side || (s->fumble && s->friendly_side==0)) && needs_damage && damage>=w) s->new_condition=damage>w?2:1;
    sprintf(s->result,"Hit %d vs %d: %s. %s\r\n%s: T %d; dice %s; %d wounds (%d -> %d W).",
        hit,required,s->fumble?"Fumble":hit>=required?"Hit":"Miss",!s->fumble && hit>=shot_critical(s)?"Critical: target Toughness halved (rounded down); no free attack.":"",
        name,t,needs_damage?dice:"none",damage,w,s->new_wounds);
    if(s->fumble && s->friendly_index<0) { damage=0; s->new_wounds=0; SetDlgItemTextA(hwnd,RSDAMAGE,""); }
    s->resolved=1; shot_fields(hwnd,s); return 1;
}
static int shot_apply(SHOT *s)
{
    MONSTER *victim=&monsters.tokens[s->m];
    if(!s->resolved || shot_blocked(s)) return 0;
    if((!s->fumble || s->friendly_index>=0) && s->friendly_side==1 && s->fumble) victim=&monsters.tokens[s->friendly_index];
    if((!s->side || (s->fumble && s->friendly_side==1)) && s->old_wounds && !s->new_wounds && victim->unique && !character_dead(monster_identity(victim)) && monsters.dead_count==CHARACTER_LIMIT) return 0;
    remember();
    heroes[s->h].ranged=s->hero.ranged; heroes[s->h].moved=s->hero.moved;
    monsters.tokens[s->m].ranged=s->monster.ranged; monsters.tokens[s->m].moved=s->monster.moved;
    if(s->fumble && s->friendly_index<0) { /* No eligible ally: a clean miss. */ }
    else if(s->fumble && s->friendly_side==0) {
        HERO *h=&heroes[s->friendly_index]; h->wounds=s->new_wounds; h->condition=s->new_condition;
        if(HERO_DEAD(h)) h->x=h->y=-1;
    } else if(s->fumble) {
        victim->wounds=s->new_wounds; if(!s->new_wounds) monster_kill(s->friendly_index);
    } else if(s->side && (s->new_condition!=s->hero.condition || (s->old_wounds>0 && s->new_wounds!=s->old_wounds))) {
        HERO *h=&heroes[s->h]; h->wounds=s->new_wounds; h->condition=s->new_condition;
        if(HERO_DEAD(h)) h->x=h->y=-1;
    } else if(s->old_wounds) {
        victim->wounds=s->new_wounds;
        if(!s->new_wounds) monster_kill(s->m);
    }
    if(!s->side) heroes[s->h].fired=1;
    if(s->side) monsters.tokens[s->m].attacked=1; else heroes[s->h].attacked=1;
    active=1; game_changed(); return 1;
}
static int shot_roll_damage(HWND hwnd,SHOT *s)
{
    HCRYPTPROV provider; int hit,left,count=0,roll,choice; BOOL ok;
    char dice[2048]="",part[8];
    hit=(int)GetDlgItemInt(hwnd,RSHIT,&ok,FALSE);
    if(!ok || hit<1 || hit>12) { SetDlgItemTextA(hwnd,RSSTATUS,"Enter or roll a hit first."); return 0; }
    if(hit<=shot_fumble(s)) {
        if(!shot_candidates(s)) return shot_calculate(hwnd,s);
        if(!s->ally_options) { shot_ally_options(hwnd,s); SetDlgItemTextA(hwnd,RSSTATUS,"The intended target's controller must choose a nearby friendly model."); return 0; }
        choice=(int)SendDlgItemMessage(hwnd,RSALLY,CB_GETCURSEL,0,0);
        if(choice<0) { SetDlgItemTextA(hwnd,RSSTATUS,"The intended target's controller must choose a nearby friendly model."); return 0; }
    } else if(hit<shot_profile(s)->hit[ranged_band(shot_range(s))]) return shot_calculate(hwnd,s);
    if(!CryptAcquireContext(&provider,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT)) { SetDlgItemTextA(hwnd,RSSTATUS,"Dice generator unavailable. Try Roll attack again."); return 0; }
    left=shot_profile(s)->dice;
    while(left && count++<MELEE_ROLL_LIMIT) {
        if(!melee_d12(provider,&roll)) break;
        sprintf(part,"%s%d",dice[0]?" ":"",roll); strcat(dice,part); if(roll!=12) left--;
    }
    CryptReleaseContext(provider,0);
    if(left) { SetDlgItemTextA(hwnd,RSSTATUS,"Dice generation incomplete. Try Roll attack again."); return 0; }
    SetDlgItemTextA(hwnd,RSDAMAGE,dice); return shot_calculate(hwnd,s);
}
static INT_PTR CALLBACK shot_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
    SHOT *s=(SHOT*)GetWindowLongPtr(hwnd,DWLP_USER); int id=LOWORD(wp),n,roll;
    HCRYPTPROV provider;
    if(msg==WM_INITDIALOG) {
        s=(SHOT*)lp; SetWindowLongPtr(hwnd,DWLP_USER,lp);
        SendDlgItemMessage(hwnd,RSHIT,EM_LIMITTEXT,2,0); SendDlgItemMessage(hwnd,RSDAMAGE,EM_LIMITTEXT,2047,0);
        ShowWindow(GetDlgItem(hwnd,RSALLY),SW_HIDE); ShowWindow(GetDlgItem(hwnd,RSALLYLABEL),SW_HIDE);
        shot_fields(hwnd,s); return TRUE;
    }
    if(msg==WM_CLOSE || (msg==WM_COMMAND && id==IDCANCEL)) { EndDialog(hwnd,0); return TRUE; }
    if(msg!=WM_COMMAND || !s) return FALSE;
    if(id==IDOK) {
        if(shot_apply(s)) EndDialog(hwnd,1); else SetDlgItemTextA(hwnd,RSSTATUS,"Cannot apply: roll attack first, check eligibility and character death capacity.");
    } else if(s->resolved) return TRUE;
    else if(id==RSNEXTTURN) {
        game_command(MGAMETURN);
        if(turn_phase) { EndDialog(hwnd,0); return TRUE; }
        s->hero.moved=s->hero.fired=s->monster.moved=0;
        shot_ally_clear(hwnd,s);
        SetDlgItemTextA(hwnd,RSHIT,""); SetDlgItemTextA(hwnd,RSDAMAGE,"");
        shot_fields(hwnd,s);
    }
    else if(id==RSPROFILE) {
        shot_ally_clear(hwnd,s);
        if(s->side) edit_ranged(hwnd,&s->monster.ranged,&s->monster.moved,-1,s->monster.name);
        else edit_ranged(hwnd,&s->hero.ranged,&s->hero.moved,s->hero.kind,NULL);
        SetDlgItemTextA(hwnd,RSHIT,""); SetDlgItemTextA(hwnd,RSDAMAGE,""); shot_fields(hwnd,s);
    } else if(id==RSALLY && HIWORD(wp)==CBN_SELCHANGE) {
        if(shot_candidates(s)) shot_roll_damage(hwnd,s); else shot_calculate(hwnd,s);
    } else if(id==RSROLL) {
        const char *blocked=shot_blocked(s);
        if(blocked) { SetDlgItemTextA(hwnd,RSSTATUS,blocked); return TRUE; }
        if(id==RSROLL) {
            if(!CryptAcquireContext(&provider,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT)) { SetDlgItemTextA(hwnd,RSSTATUS,"Dice generator unavailable. Try Roll attack again."); return TRUE; }
            n=melee_d12(provider,&roll); CryptReleaseContext(provider,0); if(!n) return TRUE;
            SetDlgItemInt(hwnd,RSHIT,roll,FALSE); SetDlgItemTextA(hwnd,RSDAMAGE,"");
        }
        shot_roll_damage(hwnd,s);
    } else return FALSE;
    return TRUE;
}
static void attack_open(HWND owner,int h,int m,int side)
{
    SHOT s; int distance; const char *blocked;
    if(h<0 || h>=hero_count || m<0 || m>=monsters.count || (side?HERO_DEAD(&heroes[h]):!HERO_ACTIVE(&heroes[h])) || !monsters.tokens[m].wounds ||
       heroes[h].x<0 || monsters.tokens[m].x<0 || hero_at(heroes[h].x,heroes[h].y,game_fog())!=h || monster_at(monsters.tokens[m].x,monsters.tokens[m].y)!=m) { MessageBeep(MB_ICONWARNING); return; }
    blocked=turn_attack_reason(side,h,m); if(blocked) { message(blocked); return; }
    distance=abs(heroes[h].x-monsters.tokens[m].x)+abs(heroes[h].y-monsters.tokens[m].y);
    if(distance==1 || melee_reachable(h,m,side)) combat_open(owner,h,m,side);
    else {
        memset(&s,0,sizeof(s)); s.h=h; s.m=m; s.side=side;
        s.hero=heroes[h]; s.monster=monsters.tokens[m]; move_mode=0;
        shot_h=h; shot_m=m; game_redraw(); game_view_pause(1);
        DialogBoxParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGRANGED),GlMainHwnd?GlMainHwnd:owner,shot_proc,(LPARAM)&s);
        shot_h=shot_m=-1; game_redraw(); game_restore_view(); game_view_pause(0);
    }
    selected=-1; selected_monster=-1; attack_monster=-1; move_mode=0;
}
