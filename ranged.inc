typedef struct { RANGED_PROFILE profile; int moved,hero_kind; const char *reference; } RANGED_EDIT;
static void ranged_profile_fields(HWND hwnd,const RANGED_EDIT *p)
{
    int i; SetDlgItemTextA(hwnd,RPWEAPON,p->profile.weapon);
    SendDlgItemMessage(hwnd,RPKIND,CB_SETCURSEL,p->profile.kind,0);
    SetDlgItemInt(hwnd,RPRANGE,p->profile.range,FALSE); SetDlgItemInt(hwnd,RPDICE,p->profile.dice,FALSE);
    for(i=0;i<5;i++) SetDlgItemInt(hwnd,RPHIT+i,p->profile.hit[i],FALSE);
}
static INT_PTR CALLBACK ranged_profile_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
    RANGED_EDIT *p=(RANGED_EDIT*)GetWindowLongPtr(hwnd,DWLP_USER),next;
    int i,n,id=LOWORD(wp); BOOL ok;
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
    HERO hero; MONSTER monster; int h,m,side,resolved,fumble_notified,new_wounds,old_wounds;
    char result[2600];
} SHOT;
static RANGED_PROFILE *shot_profile(SHOT *s) { return s->side?&s->monster.ranged:&s->hero.ranged; }
static int shot_range(const SHOT *s) { return abs(s->hero.x-s->monster.x)+abs(s->hero.y-s->monster.y); }
static const char *shot_blocked(SHOT *s)
{
    if(!s->side && heroes[s->h].fired) return "This hero has already fired this turn. Choose Next Turn to fire again.";
    return ranged_eligibility(s->h,s->m,s->side,shot_profile(s),s->side?s->monster.moved:s->hero.moved);
}
static void shot_fields(HWND hwnd,SHOT *s)
{
    char info[600]; const char *blocked=shot_blocked(s); RANGED_PROFILE *p=shot_profile(s);
    int d=shot_range(s),i; const int inputs[]={RSHIT,RSDAMAGE,RSPROFILE,RSROLL};
    sprintf(info,"%s fires at %s\r\n%s | Distance %d / range %d | Hit %d+ | Damage %d D12\r\nTarget Toughness %d; %d Wounds. %s",
        s->side?s->monster.name:s->hero.name,s->side?s->hero.name:s->monster.name,
        p->weapon[0]?p->weapon:"No ranged weapon",d,p->range,p->hit[ranged_band(d)],p->dice,
        s->side?s->hero.stats[3]:s->monster.stats[3],s->side?s->hero.wounds:s->monster.wounds,
        s->side?s->monster.moved?"Attacker moved.":"Attacker stationary.":s->hero.moved?"Attacker moved.":"Attacker stationary.");
    SetDlgItemTextA(hwnd,RSINFO,info);
    for(i=0;i<sizeof(inputs)/sizeof(inputs[0]);i++) EnableWindow(GetDlgItem(hwnd,inputs[i]),!s->resolved);
    EnableWindow(GetDlgItem(hwnd,RSROLL),!s->resolved && !blocked);
    EnableWindow(GetDlgItem(hwnd,IDOK),s->resolved && !blocked);
    EnableWindow(GetDlgItem(hwnd,RSNEXTTURN),!s->resolved);
    SetDlgItemTextA(hwnd,RSSTATUS,blocked?blocked:s->resolved?"Preview ready. Apply or Cancel.":"Choose Roll attack to preview the result.");
    SetDlgItemTextA(hwnd,RSRESULT,s->result);
}
static int shot_victim(HWND hwnd,SHOT *s,int hit,int *t,int *w,const char **name)
{
    *t=s->side?s->hero.stats[3]:s->monster.stats[3]; *w=s->side?s->hero.wounds:s->monster.wounds;
    *name=s->side?s->hero.name:s->monster.name;
    if(hit==12) *t=*t/2;
    if(hit==1) {
        *name="Friendly fire: resolve possible friendly-model damage manually"; *w=0;
        if(!s->fumble_notified) {
            s->fumble_notified=1;
            MessageBoxA(hwnd,"Fumble! You may have hit a friendly model. Resolve friendly fire manually.","Friendly fire",MB_OK|MB_ICONINFORMATION);
        }
    }
    return 1;
}
static int shot_calculate(HWND hwnd,SHOT *s)
{
    int hit,required=shot_profile(s)->hit[ranged_band(shot_range(s))],t,w,damage=0,needs_damage;
    BOOL ok; const char *name,*blocked=shot_blocked(s); char dice[2048];
    if(s->resolved) return 0;
    if(blocked) { SetDlgItemTextA(hwnd,RSSTATUS,blocked); return 0; }
    hit=(int)GetDlgItemInt(hwnd,RSHIT,&ok,FALSE);
    if(!ok || hit<1 || hit>12) { SetDlgItemTextA(hwnd,RSSTATUS,"Enter a hit roll from 1 to 12."); return 0; }
    if(!shot_victim(hwnd,s,hit,&t,&w,&name)) return 0;
    needs_damage=hit!=1 && hit>=required;
    GetDlgItemTextA(hwnd,RSDAMAGE,dice,sizeof(dice));
    if(needs_damage && !damage_auto(dice,sizeof(dice),shot_profile(s)->dice,t,&damage)) {
        SetDlgItemTextA(hwnd,RSSTATUS,"Enter the initial damage dice, each from 1 to 12. Bonus dice are automatic. If rolling fails, retry."); return 0;
    }
    SetDlgItemTextA(hwnd,RSDAMAGE,dice);
    s->old_wounds=w; s->new_wounds=damage>w?0:w-damage;
    sprintf(s->result,"Hit %d vs %d: %s. %s\r\n%s: T %d; dice %s; %d wounds (%d -> %d W).",
        hit,required,hit==1?"Friendly fire":hit>=required?"Hit":"Miss",hit==12?"Critical: target Toughness halved (rounded down); no free attack.":"",
        name,t,needs_damage?dice:"none",damage,w,s->new_wounds);
    if(hit==1) {
        strcpy(s->result,"Fumble. Friendly fire may have occurred.\r\nThe intended target was not hit. Resolve friendly fire manually.");
        SetDlgItemTextA(hwnd,RSDAMAGE,"");
    }
    s->resolved=1; shot_fields(hwnd,s); return 1;
}
static int shot_apply(SHOT *s)
{
    MONSTER *victim=&monsters.tokens[s->m];
    if(!s->resolved || shot_blocked(s)) return 0;
    if(!s->side && s->old_wounds && !s->new_wounds && victim->unique && !character_dead(monster_identity(victim)) && monsters.dead_count==CHARACTER_LIMIT) return 0;
    remember();
    heroes[s->h].ranged=s->hero.ranged; heroes[s->h].moved=s->hero.moved;
    monsters.tokens[s->m].ranged=s->monster.ranged; monsters.tokens[s->m].moved=s->monster.moved;
    /* Friendly fire is a notice only: never apply damage to a guessed model. */
    if(s->old_wounds) {
        if(s->side) {
            HERO *h=&heroes[s->h]; h->wounds=s->new_wounds;
            if(!h->wounds && h->kind==4) h->x=h->y=-1;
        } else {
            victim->wounds=s->new_wounds;
            if(!s->new_wounds) monster_kill(s->m);
        }
    }
    if(!s->side) heroes[s->h].fired=1;
    active=1; game_changed(); return 1;
}
static int shot_roll_damage(HWND hwnd,SHOT *s)
{
    HCRYPTPROV provider; int hit,t,w,left,count=0,roll; BOOL ok; const char *name;
    char dice[2048]="",part[8];
    hit=(int)GetDlgItemInt(hwnd,RSHIT,&ok,FALSE);
    if(!ok || hit<1 || hit>12) { SetDlgItemTextA(hwnd,RSSTATUS,"Enter or roll a hit first."); return 0; }
    if(!shot_victim(hwnd,s,hit,&t,&w,&name)) return 0;
    if(hit==1 || (hit!=1 && hit<shot_profile(s)->hit[ranged_band(shot_range(s))])) return shot_calculate(hwnd,s);
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
        shot_fields(hwnd,s); return TRUE;
    }
    if(msg==WM_CLOSE || (msg==WM_COMMAND && id==IDCANCEL)) { EndDialog(hwnd,0); return TRUE; }
    if(msg!=WM_COMMAND || !s) return FALSE;
    if(id==IDOK) {
        if(shot_apply(s)) EndDialog(hwnd,1); else SetDlgItemTextA(hwnd,RSSTATUS,"Cannot apply: roll attack first, check eligibility and character death capacity.");
    } else if(s->resolved) return TRUE;
    else if(id==RSNEXTTURN) {
        game_command(MGAMETURN);
        s->hero.moved=s->hero.fired=s->monster.moved=0;
        SetDlgItemTextA(hwnd,RSHIT,""); SetDlgItemTextA(hwnd,RSDAMAGE,"");
        shot_fields(hwnd,s);
    }
    else if(id==RSPROFILE) {
        if(s->side) edit_ranged(hwnd,&s->monster.ranged,&s->monster.moved,-1,s->monster.name);
        else edit_ranged(hwnd,&s->hero.ranged,&s->hero.moved,s->hero.kind,NULL);
        SetDlgItemTextA(hwnd,RSHIT,""); SetDlgItemTextA(hwnd,RSDAMAGE,""); shot_fields(hwnd,s);
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
    SHOT s; int distance;
    if(h<0 || h>=hero_count || m<0 || m>=monsters.count || !heroes[h].wounds || !monsters.tokens[m].wounds ||
       heroes[h].x<0 || monsters.tokens[m].x<0 || hero_at(heroes[h].x,heroes[h].y,game_fog())!=h || monster_at(monsters.tokens[m].x,monsters.tokens[m].y)!=m) { MessageBeep(MB_ICONWARNING); return; }
    distance=abs(heroes[h].x-monsters.tokens[m].x)+abs(heroes[h].y-monsters.tokens[m].y);
    if(distance==1) combat_open(owner,h,m,side);
    else {
        memset(&s,0,sizeof(s)); s.h=h; s.m=m; s.side=side;
        s.hero=heroes[h]; s.monster=monsters.tokens[m]; move_mode=0;
        shot_h=h; shot_m=m; game_redraw(); game_view_pause(1);
        DialogBoxParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGRANGED),GlMainHwnd?GlMainHwnd:owner,shot_proc,(LPARAM)&s);
        shot_h=shot_m=-1; game_redraw(); game_restore_view(); game_view_pause(0);
    }
    selected=-1; selected_monster=-1; attack_monster=-1; move_mode=0;
}
