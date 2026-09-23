/* Melee drafts never change live tokens until Apply. All dice are D12. */
#define MELEE_ROLL_LIMIT 512
static int melee_reference(const char *name,MELEE_PROFILE *p)
{
    MELEE_PROFILE next={0};
    if(!room_melee(name,&next.dice,next.hit)) return 0;
    strcpy(next.weapon,"Printed melee profile"); *p=next; return 1;
}
typedef struct { MELEE_PROFILE profile; int ws,t,hero_kind; const char *reference; } PROFILE_EDIT;
static void profile_fields(HWND hwnd,const PROFILE_EDIT *p)
{
    int i;
    SetDlgItemTextA(hwnd,MPWEAPON,p->profile.weapon);
    SetDlgItemInt(hwnd,MPDICE,p->profile.dice,FALSE);
    SetDlgItemInt(hwnd,MPWS,p->ws,FALSE); SetDlgItemInt(hwnd,MPT,p->t,FALSE);
    for(i=0;i<12;i++) SetDlgItemInt(hwnd,MPHIT+i,p->profile.hit[i],FALSE);
}
static INT_PTR CALLBACK profile_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
    PROFILE_EDIT *p=(PROFILE_EDIT*)GetWindowLongPtr(hwnd,DWLP_USER),next;
    int i,id=LOWORD(wp),value; BOOL ok;
    if(msg==WM_INITDIALOG) {
        p=(PROFILE_EDIT*)lp; SetWindowLongPtr(hwnd,DWLP_USER,lp);
        SendDlgItemMessage(hwnd,MPWEAPON,EM_LIMITTEXT,39,0);
        next=*p;
        if(p->hero_kind>=0 && p->hero_kind<HERO_CLASS_COUNT) {
            MELEE_PROFILE defaults; hero_melee_defaults(&defaults,p->hero_kind);
            if(!next.profile.weapon[0]) strcpy(next.profile.weapon,defaults.weapon);
            if(!next.profile.dice) next.profile.dice=defaults.dice;
            for(i=0;i<12;i++) if(!next.profile.hit[i]) next.profile.hit[i]=defaults.hit[i];
            SetDlgItemTextA(hwnd,MPREF,"Use class defaults");
            SetDlgItemTextA(hwnd,MPINFO,"Missing entries use class defaults. Edit as needed, then Save profile.");
        }
        EnableWindow(GetDlgItem(hwnd,MPREF),p->reference!=NULL || (p->hero_kind>=0 && p->hero_kind<HERO_CLASS_COUNT));
        profile_fields(hwnd,&next); return TRUE;
    }
    if(msg==WM_CLOSE || (msg==WM_COMMAND && id==IDCANCEL)) { EndDialog(hwnd,0); return TRUE; }
    if(msg!=WM_COMMAND || !p) return FALSE;
    if(id==MPREF) {
        next=*p;
        if(p->hero_kind>=0 && p->hero_kind<HERO_CLASS_COUNT) hero_melee_defaults(&next.profile,p->hero_kind);
        if((p->hero_kind>=0 && p->hero_kind<HERO_CLASS_COUNT) || (p->reference && melee_reference(p->reference,&next.profile))) {
            /* Loading a reference replaces equipment only, not edited WS/T. */
            SetDlgItemTextA(hwnd,MPWEAPON,next.profile.weapon);
            SetDlgItemInt(hwnd,MPDICE,next.profile.dice,FALSE);
            for(i=0;i<12;i++) SetDlgItemInt(hwnd,MPHIT+i,next.profile.hit[i],FALSE);
            SetDlgItemTextA(hwnd,MPINFO,p->hero_kind>=0?"Class weapon and hit defaults loaded. Edit as needed, then Save profile.":"Exact printed melee row loaded. Special rules remain manual.");
        } else SetDlgItemTextA(hwnd,MPINFO,"No single complete reference. Enter the profile manually.");
        return TRUE;
    }
    if(id!=IDOK) return FALSE;
    next=*p; GetDlgItemTextA(hwnd,MPWEAPON,next.profile.weapon,sizeof(next.profile.weapon));
    for(i=0;i<15;i++) {
        value=(int)GetDlgItemInt(hwnd,i<12?MPHIT+i:i==12?MPDICE:i==13?MPWS:MPT,&ok,FALSE);
        if(!ok || value<0 || value>(i<12?12:99)) {
            SetDlgItemTextA(hwnd,MPINFO,"Use whole numbers: hits 0-12; damage, WS and T 0-99."); return TRUE;
        }
        if(i<12) next.profile.hit[i]=value; else if(i==12) next.profile.dice=value;
        else if(i==13) next.ws=value; else next.t=value;
    }
    *p=next; EndDialog(hwnd,1); return TRUE;
}
static int edit_profile(HWND owner,MELEE_PROFILE *profile,int *stats,const char *reference,int hero_kind)
{
    PROFILE_EDIT p; p.profile=*profile; p.ws=stats[0]; p.t=stats[3]; p.reference=reference; p.hero_kind=hero_kind;
    if(DialogBoxParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGMELEE),owner,profile_proc,(LPARAM)&p)!=1) return 0;
    *profile=p.profile; stats[0]=p.ws; stats[3]=p.t; return 1;
}
/* Same-square count is insufficient: reject diagonal drops and room walls.
 * Doors overlay a floor cell; use underlying bounds, then their facing edge. */
static int melee_adjacent(int h,int m)
{
    if(h<0 || h>=hero_count || m<0 || m>=monsters.count || !heroes[h].wounds || !monsters.tokens[m].wounds || heroes[h].x<0 || monsters.tokens[m].x<0) return 0;
    return battle_edge(heroes[h].x,heroes[h].y,monsters.tokens[m].x,monsters.tokens[m].y);
}
/* Return 1 only for the complete pool, including exploding twelves. */
static int melee_damage(const char *text,int dice,int toughness,int *wounds)
{
    int left=dice,count=0,total=0; long value; char *end; const char *p=text;
    if(dice<1 || dice>99 || toughness<0 || toughness>99) return 0;
    while(*p) {
        while(*p && (isspace((unsigned char)*p) || *p==',')) p++;
        if(!*p) break;
        if(!left || !isdigit((unsigned char)*p) || ++count>MELEE_ROLL_LIMIT) return 0;
        value=strtol(p,&end,10); if(value<1 || value>12 || (*end && !isspace((unsigned char)*end) && *end!=',')) return 0;
        p=end; left--;
        if(value==12) { total++; left++; } else if(value>=toughness) total++;
    }
    if(left) return 0; *wounds=total; return 1;
}
static int melee_d12(HCRYPTPROV provider,int *roll)
{
    DWORD value;
    do { if(!CryptGenRandom(provider,sizeof(value),(BYTE*)&value)) return 0; } while(value>=4294967292UL);
    *roll=(int)(value%12)+1; return 1;
}
/* Physical dice need only the initial weapon pool. Complete any outstanding
 * bonus rolls automatically, accepting already-complete pools without rerolling. */
static int damage_auto(char *text,size_t capacity,int dice,int toughness,int *wounds)
{
    const char *p=text; char *end,completed[2048],part[8]; long value;
    int left=dice,count=0,roll; HCRYPTPROV provider;
    if(dice<1 || dice>99 || toughness<0 || toughness>99 || strlen(text)>=sizeof(completed)) return 0;
    while(*p) {
        while(*p && (isspace((unsigned char)*p) || *p==',')) p++;
        if(!*p) break;
        if(!left || !isdigit((unsigned char)*p) || ++count>MELEE_ROLL_LIMIT) return 0;
        value=strtol(p,&end,10);
        if(value<1 || value>12 || (*end && !isspace((unsigned char)*end) && *end!=',')) return 0;
        p=end; if(value!=12) left--;
    }
    if(count<dice) return 0;
    if(!left) return melee_damage(text,dice,toughness,wounds);
    strcpy(completed,text);
    if(!CryptAcquireContext(&provider,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT)) return 0;
    while(left && count<MELEE_ROLL_LIMIT) {
        if(!melee_d12(provider,&roll)) break;
        sprintf(part," %d",roll);
        if(strlen(completed)+strlen(part)>=sizeof(completed) || strlen(completed)+strlen(part)>=capacity) break;
        strcat(completed,part); count++; if(roll!=12) left--;
    }
    CryptReleaseContext(provider,0);
    if(left || !melee_damage(completed,dice,toughness,wounds)) return 0;
    strcpy(text,completed); return 1;
}
typedef struct {
    HERO hero; MONSTER monster;
    int h,m,side,resolved,pending,steps; /* side: 0 hero; 1 monster. pending: -1 none. */
    char log[12000];
} COMBAT;
static int combat_required(const COMBAT *c)
{
    const MELEE_PROFILE *p=c->side?&c->monster.melee:&c->hero.melee;
    int ws=c->side?c->hero.stats[0]:c->monster.stats[0];
    return ws>=1 && ws<=12 && p->dice>0?p->hit[ws-1]:0;
}
static void combat_fields(HWND hwnd,COMBAT *c)
{
    char text[500]; const char *attacker=c->side?c->monster.name:c->hero.name;
    const char *defender=c->side?c->hero.name:c->monster.name;
    const MELEE_PROFILE *p=c->side?&c->monster.melee:&c->hero.melee;
    int ws=c->side?c->hero.stats[0]:c->monster.stats[0],t=c->side?c->hero.stats[3]:c->monster.stats[3];
    int required=combat_required(c),can_roll=!c->resolved && required>0;
    sprintf(text,"%s attacks %s\r\nWeapon: %s | Damage: %d D12 | Required hit: %d\r\nTarget WS %d, T %d | Remaining Wounds: %s %d; %s %d",
        attacker,defender,p->weapon[0]?p->weapon:"not configured",p->dice,required,ws,t,c->hero.name,c->hero.wounds,c->monster.name,c->monster.wounds);
    SetDlgItemTextA(hwnd,MCINFO,text);
    EnableWindow(GetDlgItem(hwnd,MCHIT),!c->resolved); EnableWindow(GetDlgItem(hwnd,MCDAMAGE),!c->resolved);
    EnableWindow(GetDlgItem(hwnd,MCROLLHIT),can_roll);
    EnableWindow(GetDlgItem(hwnd,IDOK),c->steps>0);
    EnableWindow(GetDlgItem(hwnd,MCNEXT),c->resolved && c->pending>=0 && c->hero.wounds && c->monster.wounds && c->steps<32);
    EnableWindow(GetDlgItem(hwnd,MCHERO),!c->resolved); EnableWindow(GetDlgItem(hwnd,MCMONSTER),!c->resolved);
    SetDlgItemTextA(hwnd,MCLOG,c->log);
    SendDlgItemMessage(hwnd,MCLOG,EM_SETSEL,(WPARAM)-1,(LPARAM)-1);
    SendDlgItemMessage(hwnd,MCLOG,EM_SCROLLCARET,0,0);
    SetDlgItemTextA(hwnd,MCSTATUS,!required?"Configure damage and the target WS hit entry first.":
        !c->resolved?(c->steps?"Roll the free attack, or Apply the results already previewed.":"Choose Roll attack to preview the result."):
        c->pending>=0 && c->hero.wounds && c->monster.wounds?"Free attack available. Take it here, or apply and adjudicate it separately.":"Preview ready. Apply or Cancel.");
}
static int combat_calculate(HWND hwnd,COMBAT *c)
{
    int hit,required=combat_required(c),damage=0,*wounds,old; BOOL ok;
    char dice[2048],line[2350];
    if(c->resolved || !required || c->steps>=32) return 0;
    hit=(int)GetDlgItemInt(hwnd,MCHIT,&ok,FALSE);
    if(!ok || hit<1 || hit>12) { SetDlgItemTextA(hwnd,MCSTATUS,"Enter a hit die from 1 to 12."); return 0; }
    GetDlgItemTextA(hwnd,MCDAMAGE,dice,sizeof(dice));
    if(hit!=1 && hit>=required && !damage_auto(dice,sizeof(dice),c->side?c->monster.melee.dice:c->hero.melee.dice,c->side?c->hero.stats[3]:c->monster.stats[3],&damage)) {
        SetDlgItemTextA(hwnd,MCSTATUS,"Enter the initial damage dice, each from 1 to 12. Bonus dice are automatic. If rolling fails, retry."); return 0;
    }
    SetDlgItemTextA(hwnd,MCDAMAGE,dice);
    wounds=c->side?&c->hero.wounds:&c->monster.wounds; old=*wounds;
    sprintf(line,"%s: hit %d vs %d. %s%s%s\r\nDamage dice: %s\r\n%d wounds; target %d -> %d W.\r\n\r\n",
        c->side?c->monster.name:c->hero.name,hit,required,hit==1?"Fumble.":hit>=required?"Hit.":"Miss.",
        hit==12?" Attacker gains a free attack.":"",hit==1?" Defender gains an immediate free attack.":"",
        hit!=1 && hit>=required?dice:"none",damage,old,damage>old?0:old-damage);
    if(strlen(c->log)+strlen(line)>=sizeof(c->log)) { SetDlgItemTextA(hwnd,MCSTATUS,"Combat log full. Apply this preview; resolve further free attacks separately."); return 0; }
    *wounds=damage>old?0:old-damage; strcat(c->log,line);
    c->pending=hit==12?c->side:hit==1?!c->side:-1; c->resolved=1; c->steps++;
    combat_fields(hwnd,c); return 1;
}
static int combat_apply(COMBAT *c)
{
    if(!c->steps || !melee_adjacent(c->h,c->m)) return 0;
    if(!c->monster.wounds && c->monster.unique && !character_dead(c->monster.name) && monsters.dead_count==CHARACTER_LIMIT) return 0;
    remember(); heroes[c->h]=c->hero; monsters.tokens[c->m]=c->monster;
    if(!c->hero.wounds && c->hero.kind==4) heroes[c->h].x=heroes[c->h].y=-1;
    if(!c->monster.wounds) monster_kill(c->m);
    active=1; game_changed(); return 1;
}
static INT_PTR CALLBACK combat_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
    COMBAT *c=(COMBAT*)GetWindowLongPtr(hwnd,DWLP_USER); int id=LOWORD(wp),roll,left,count;
    HCRYPTPROV provider; char dice[2048],part[8];
    if(msg==WM_INITDIALOG) {
        c=(COMBAT*)lp; SetWindowLongPtr(hwnd,DWLP_USER,lp);
        SendDlgItemMessage(hwnd,MCHIT,EM_LIMITTEXT,2,0); SendDlgItemMessage(hwnd,MCDAMAGE,EM_LIMITTEXT,2047,0);
        combat_fields(hwnd,c); return TRUE;
    }
    if(msg==WM_CLOSE || (msg==WM_COMMAND && id==IDCANCEL)) { EndDialog(hwnd,0); return TRUE; }
    if(msg!=WM_COMMAND || !c) return FALSE;
    if(id==MCHERO || id==MCMONSTER) {
        if(c->resolved) return TRUE;
        if(id==MCHERO) edit_profile(hwnd,&c->hero.melee,c->hero.stats,NULL,c->hero.kind);
        else edit_profile(hwnd,&c->monster.melee,c->monster.stats,c->monster.name,-1);
        SetDlgItemTextA(hwnd,MCHIT,""); SetDlgItemTextA(hwnd,MCDAMAGE,""); combat_fields(hwnd,c);
    } else if(id==MCROLLHIT) {
        if(c->resolved || !combat_required(c)) return TRUE;
        if(!CryptAcquireContext(&provider,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT)) { SetDlgItemTextA(hwnd,MCSTATUS,"Dice generator unavailable. Try Roll attack again."); return TRUE; }
        if(!melee_d12(provider,&roll)) {
            CryptReleaseContext(provider,0);
            SetDlgItemTextA(hwnd,MCSTATUS,"Dice generation failed. Try Roll attack again."); return TRUE;
        }
        SetDlgItemInt(hwnd,MCHIT,roll,FALSE); SetDlgItemTextA(hwnd,MCDAMAGE,"");
        dice[0]=0; count=0;
        left=roll!=1 && roll>=combat_required(c)?(c->side?c->monster.melee.dice:c->hero.melee.dice):0;
        while(left && count++<MELEE_ROLL_LIMIT) {
            if(!melee_d12(provider,&roll)) break;
            sprintf(part,"%s%d",dice[0]?" ":"",roll); strcat(dice,part);
            if(roll!=12) left--;
        }
        CryptReleaseContext(provider,0);
        if(left) SetDlgItemTextA(hwnd,MCSTATUS,"Dice generation incomplete. Try Roll attack again.");
        else {
            SetDlgItemTextA(hwnd,MCDAMAGE,dice);
            combat_calculate(hwnd,c);
        }
    }
    else if(id==MCNEXT && c->resolved && c->pending>=0 && c->hero.wounds && c->monster.wounds && c->steps<32) {
        c->side=c->pending; c->pending=-1; c->resolved=0;
        SetDlgItemTextA(hwnd,MCHIT,""); SetDlgItemTextA(hwnd,MCDAMAGE,""); combat_fields(hwnd,c);
    } else if(id==IDOK) {
        if(combat_apply(c)) EndDialog(hwnd,1);
        else SetDlgItemTextA(hwnd,MCSTATUS,"Roll attack first. Cannot apply if positions changed or the character death record is full.");
    } else return FALSE;
    return TRUE;
}
static void combat_open(HWND owner,int h,int m,int monster_attacks)
{
    COMBAT c;
    if(!melee_adjacent(h,m)) { MessageBeep(MB_ICONWARNING); return; }
    memset(&c,0,sizeof(c)); c.h=h; c.m=m; c.hero=heroes[h]; c.monster=monsters.tokens[m];
    c.side=monster_attacks; c.pending=-1;
    /* Legacy saves have no profile. Hydrate only the draft; Cancel is inert. */
    if(!c.monster.melee.dice && !c.monster.melee.weapon[0]) melee_reference(c.monster.name,&c.monster.melee);
    move_mode=0; game_view_pause(1);
    DialogBoxParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGCOMBAT),owner,combat_proc,(LPARAM)&c);
    game_restore_view(); game_view_pause(0);
}
