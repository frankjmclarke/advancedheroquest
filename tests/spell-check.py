"""Native spell registry, resources, casting, UI, Undo and save checks."""
from pathlib import Path
import ast
import subprocess
import tempfile
import importlib.util
import copy
import json
import struct
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('compile_packs',ROOT/'tools/compile-packs.py')
compiler=importlib.util.module_from_spec(spec); spec.loader.exec_module(compiler)
source=json.loads((ROOT/'data/packs/fantasy.json').read_text(encoding='utf-8'))
for change in ('missing_component','missing_book','missing_spell','duplicate','unsupported','starting_outside_book','moving_two_components','empty_template','large_template','wrong_template_target','invalid_template'):
    bad=copy.deepcopy(source)
    if change=='missing_component': bad['spells'][0]['components']={'fantasy:component:missing':1}
    if change=='missing_book': bad['heroes'][3]['caster']['books']=['fantasy:book:missing']
    if change=='missing_spell': bad['spellbooks'][0]['spells'].append('fantasy:spell:missing')
    if change=='duplicate': bad['spellbooks'][0]['id']=bad['spells'][0]['id']
    if change=='unsupported': bad['spells'][0]['effect']='run-arbitrary-script'
    if change=='starting_outside_book': bad['spellbooks'][0]['starting_spells'].append('fantasy:spell:fireball')
    if change=='moving_two_components': bad['spells'][10]['stationary']=0
    if change=='empty_template': bad['spells'][2]['template']={'width':0,'height':2}
    if change=='large_template': bad['spells'][2]['template']={'width':13,'height':2}
    if change=='wrong_template_target': bad['spells'][0]['template']={'width':2,'height':2}
    if change=='invalid_template': bad['spells'][2]['template']=[2,2]
    try: compiler.compile_pack(bad)
    except ValueError: pass
    else: raise AssertionError('Invalid pack accepted: '+change)
# A future book and a component-free spell compile through the same format.
future=copy.deepcopy(source)
future['spells'].append(dict(id='fantasy:spell:future',name='Future spell',text='A future harmless GM effect.',effect='manual',target='manual',components={}))
future['spellbooks'].append(dict(id='fantasy:book:future',name='Future book',spells=['fantasy:spell:future'],starting_spells=['fantasy:spell:future']))
assert compiler.compile_pack(future).startswith(b'HQPACK5\n')
def legacy_resource(version):
    data=compiler.compile_pack(source); pos=8; out=bytearray(f'HQPACK{version}\n'.encode())
    def take(n):
        nonlocal pos
        chunk=data[pos:pos+n]; pos+=n; return chunk
    def string():
        nonlocal pos
        n=struct.unpack_from('<I',data,pos)[0]; out.extend(take(4+n))
    for _ in range(3): string()
    out.extend(take(12))
    for _ in source['heroes']+source['monsters']:
        for _ in range(4): string()
        out.extend(take(48))
        string(); out.extend(take(52))
        thresholds=take(12)
        if version==2:
            critical,fumble,reach=struct.unpack('<III',thresholds)
            out.extend(struct.pack('<III',critical,fumble,reach-1))
        string(); out.extend(take(32)); take(8)
    return out
tree=ast.parse((ROOT/'tests/game-check.py').read_text())
base=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='HARNESS' for t in n.targets))
monster=(ROOT/'tests/monster-check.py').read_text()
start=monster.index('static void screenshot')
screenshot=monster[start:monster.index('int main(int argc',start)]
HARNESS=base[:base.index('int main(int argc')]+screenshot+r'''

static int spell_index(const char *name) { int i; for(i=0;i<pack_current()->spell_count;i++) if(!strcmp(pack_current()->spells[i].name,name)) return i; return -1; }
static int component_index(const char *name) { int i; for(i=0;i<pack_current()->component_count;i++) if(!strcmp(pack_current()->components[i].name,name)) return i; return -1; }
static void magic_fixture(void) {
    PICE pieces[3]; int cells[144],i; unsigned char fog[3]={1,0,0};
    memset(pieces,0,sizeof(pieces)); pieces[0].type=NORMAL_ROOM; pieces[0].w=pieces[0].h=12; pieces[1].type=pieces[2].type=EMPTY;
    for(i=0;i<144;i++) cells[i]=0;
    assert(map_restore(pieces,3,12,12,cells)); map_ready=active=1; ready=0; game_set_fog(fog,3);
    memset(&monsters,0,sizeof(monsters)); memset(&magic,0,sizeof(magic)); monsters.seen[0]=1;
    hero_count=2; hero_defaults(&heroes[0],3,1); heroes[0].x=heroes[0].y=1;
    hero_defaults(&heroes[1],0,2); heroes[1].x=2; heroes[1].y=2;
    monsters.count=1; strcpy(monsters.tokens[0].name,"orc");
    monsters.tokens[0].x=5; monsters.tokens[0].y=1; monsters.tokens[0].stats[0]=5;
    monsters.tokens[0].stats[3]=7; monsters.tokens[0].stats[7]=monsters.tokens[0].wounds=8;
    assert(melee_reference("orc",&monsters.tokens[0].melee));
    strcpy(session_title,"Magic tests"); selected=selected_monster=-1; turn_phase=gm_override=0;
}

static void all_bright_ready(void) {
    int i; heroes[0].caster.setup_pending=0;
    for(i=0;i<12;i++) heroes[0].caster.known[i]=1;
    for(i=0;i<pack_current()->component_count;i++) heroes[0].caster.components[i]=20;
}
static void choose_spell(SPELL_CAST *c,const char *name,int target) {
    memset(c,0,sizeof(*c)); c->spell=spell_index(name);
    if(target>=0) { c->count=1; c->targets[0]=target; }
    assert(c->spell>=0);
}
static void map_spell_choose(const char *name) {
    int i; HWND w=spell_map_dialog;
    for(i=0;i<SendDlgItemMessage(w,SCSpell,CB_GETCOUNT,0,0);i++)
        if(SendDlgItemMessage(w,SCSpell,CB_GETITEMDATA,i,0)==spell_index(name)) break;
    assert(i<SendDlgItemMessage(w,SCSpell,CB_GETCOUNT,0,0));
    SendDlgItemMessage(w,SCSpell,CB_SETCURSEL,i,0);
    SendMessage(w,WM_COMMAND,MAKEWPARAM(SCSpell,CBN_SELCHANGE),0);
}
static void spell_pump_messages(void) {
    MSG msg;
    while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
        if(!Wind_FilterMessage(&msg)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    }
}
static void spell_mouse_click(HWND control,int x,int y) {
    assert(PostMessage(control,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(x,y)));
    assert(PostMessage(control,WM_LBUTTONUP,0,MAKELPARAM(x,y)));
    spell_pump_messages();
}
static LRESULT CALLBACK magic_board_proc(HWND w,UINT msg,WPARAM wp,LPARAM lp) {
    if(msg==WM_PAINT || msg==WM_PRINT || msg==WM_PRINTCLIENT) {
        PAINTSTRUCT ps; HDC dc=msg==WM_PAINT?BeginPaint(w,&ps):(HDC)wp; RECT r; int x,y;
        GetClientRect(w,&r); FillRect(dc,&r,(HBRUSH)GetStockObject(DKGRAY_BRUSH));
        SelectObject(dc,GetStockObject(DEFAULT_GUI_FONT));
        for(y=0;y<12;y++) for(x=0;x<12;x++) { r.left=(x+1)*40; r.top=(12-y)*40; r.right=r.left+40; r.bottom=r.top+40;
            FrameRect(dc,&r,(HBRUSH)GetStockObject(GRAY_BRUSH)); }
        marker(dc,80,440,40,3,0,0);
        spell_map_draw(dc,NULL,40,0,0);
        if(msg==WM_PAINT) EndPaint(w,&ps); return 0;
    }
    return DefWindowProcA(w,msg,wp,lp);
}
static void map_spell_checks(const char *preview_path) {
    SPELL_CAST *c; int stock,turn,i; HWND board; HDC dc; RECT bounds; char text[512],path[MAX_PATH]; DWORD handles;
    CHARACTER_PACK *old; unsigned char *bytes; const CHARACTER_PACK *p=pack_current(); size_t n=p->size-8*p->spell_count;
    /* A real HQPACK4 snapshot receives a bounded 2x2 legacy footprint. */
    bytes=(unsigned char*)malloc(p->size); assert(bytes); memcpy(bytes,p->bytes,p->size); memcpy(bytes,"HQPACK4\n",8);
    old=pack_decode(bytes,n); assert(old && old->spells[2].template_width==2 && old->spells[2].template_height==2); pack_free(old);
    memcpy(bytes,p->bytes,p->size); bytes[n+8*2]=13; assert(!pack_decode(bytes,p->size)); free(bytes);
    magic_fixture(); all_bright_ready(); heroes[1].x=6; heroes[1].y=2;
    stock=heroes[0].caster.components[component_index("Fire Dust")]; turn=magic.turn;
    spellcast_open(NULL,0,0); assert(spell_map_dialog); c=spell_map_cast(); assert(c && c->map_owned);
    /* Queue actual mouse down/up through the same preprocessing used by the
       application's event loop. Direct WM_COMMAND tests cannot catch a hook
       swallowing mouse-up while a native control is tracking a click. */
    spell_mouse_click(GetDlgItem(spell_map_dialog,SCTARGETS),8,6);
    assert(c->count==1 && IsWindowEnabled(GetDlgItem(spell_map_dialog,SCROLL)));
    spell_mouse_click(GetDlgItem(spell_map_dialog,SCMAPPICK),8,8);
    assert(spell_map_picking && !IsWindowVisible(spell_map_dialog)); spell_map_resume();
    map_spell_choose("Flames of Death");
    SendMessage(spell_map_dialog,WM_COMMAND,SCMAPPICK,0); assert(spell_map_picking && !IsWindowVisible(spell_map_dialog));
    spell_map_hover(5,1); assert(c->count==2 && !c->resolved && heroes[0].caster.components[component_index("Fire Dust")]==stock);
    assert(spell_map_click(5,1,NULL)); assert(!spell_map_picking && IsWindowVisible(spell_map_dialog));
    assert(c->count==2 && c->targets[0]==1 && c->targets[1]==HERO_LIMIT);
    assert(!IsWindowEnabled(GetDlgItem(spell_map_dialog,SCTARGETS)));
    assert(cast_prepare(c,0)); c->count=1; assert(!cast_apply(c)); /* friendly fire cannot be omitted */
    spell_template_collect(c); assert(c->count==2);
    game_command(MGAMETURN); assert(magic.turn==turn); /* unrelated changes cannot stale a preview */
    spell_mouse_click(GetDlgItem(spell_map_dialog,IDCANCEL),8,8); assert(!spell_map_dialog);
    assert(heroes[0].caster.components[component_index("Fire Dust")]==stock && !heroes[0].caster.cast_used);
    /* Flight needs two distinct clicks and cannot accidentally move a token. */
    spellcast_open(NULL,0,0); c=spell_map_cast(); map_spell_choose("Flight");
    assert(spell_map_click(6,2,NULL) && c->flight_destination && c->targets[0]==1);
    assert(spell_map_click(9,2,NULL) && !c->flight_destination && c->x==9 && heroes[1].x==6);
    { MSG msg; PostMessage(spell_map_dialog,WM_KEYDOWN,VK_ESCAPE,0);
      spell_pump_messages(); assert(!spell_map_dialog);
      memset(&msg,0,sizeof(msg)); assert(!Wind_FilterMessage(&msg)); }
    /* Applying leaves a visible outcome and an inert Close action. */
    stock=heroes[0].caster.components[component_index("Red Dragon Dust")];
    spellcast_open(NULL,0,0); c=spell_map_cast(); map_spell_choose("Dragon Armour");
    assert(spell_map_click(1,1,NULL)); SendMessage(spell_map_dialog,WM_COMMAND,SCROLL,0); assert(c->resolved);
    SendMessage(spell_map_dialog,WM_COMMAND,IDOK,0);
    assert(spell_map_dialog && c->committed && !c->resolved);
    assert(strstr(c->result,"Toughness is now") && strstr(c->result,"until the next exploration turn"));
    assert(strstr(c->result,"consumed;") && strstr(c->result,"remaining."));
    GetDlgItemTextA(spell_map_dialog,IDOK,text,sizeof(text)); assert(!strcmp(text,"Close"));
    assert(!IsWindowVisible(GetDlgItem(spell_map_dialog,SCTARGETS)) && !IsWindowVisible(GetDlgItem(spell_map_dialog,SCROLL)));
    assert(heroes[0].caster.components[component_index("Red Dragon Dust")]==stock-1);
    SendMessage(spell_map_dialog,WM_COMMAND,SCROLL,0); assert(heroes[0].caster.components[component_index("Red Dragon Dust")]==stock-1);
    snprintf(path,sizeof(path),"%s.applied.bmp",preview_path); screenshot(spell_map_dialog,path);
    spell_mouse_click(GetDlgItem(spell_map_dialog,IDOK),8,8); assert(!spell_map_dialog);
    game_command(MGAMEUNDO); assert(heroes[0].caster.components[component_index("Red Dragon Dust")]==stock && !heroes[0].magic.armour);
    /* The renderer labels timing and corpse eligibility without leaking fog. */
    magic.turn=1; heroes[0].magic.armour=heroes[0].magic.courage=heroes[0].magic.hand=heroes[0].magic.swift=1;
    magic.still_until[0]=2; magic_effect_text(0,0,text,sizeof(text));
    assert(strstr(text,"until exploration") && strstr(text,"until Turn 3"));
    heroes[1].wounds=0; heroes[1].condition=2; magic_hero_death(&heroes[1]);
    assert(!strcmp(magic_corpse_status(&heroes[1].magic),"Resurrect next turn"));
    magic.turn=2; assert(!strcmp(magic_corpse_status(&heroes[1].magic),"Resurrect now"));
    assert(spell_corpse_at(6,2,game_fog())==1);
    game_fog()[0]=0; assert(spell_corpse_at(6,2,game_fog())<0); game_fog()[0]=1;
    heroes[0].x=6; heroes[0].y=2; assert(strstr(magic_corpse_status(&heroes[1].magic),"blocked")); heroes[0].x=heroes[0].y=1;
    heroes[1].magic.soul_lost=1; assert(!strcmp(magic_corpse_status(&heroes[1].magic),"Soul lost")); heroes[1].magic.soul_lost=0;
    { WNDCLASSA cls; memset(&cls,0,sizeof(cls)); cls.hInstance=GetModuleHandle(NULL); cls.lpfnWndProc=magic_board_proc; cls.lpszClassName="HQMagicBoardTest"; assert(RegisterClassA(&cls)); }
    magic.still_until[0]=3;
    board=CreateWindowExA(0,"HQMagicBoardTest","Spell annotations",WS_POPUP|WS_VISIBLE,0,0,900,620,NULL,NULL,GetModuleHandle(NULL),NULL); assert(board);
    dc=GetDC(board); GetClientRect(board,&bounds); FillRect(dc,&bounds,(HBRUSH)GetStockObject(DKGRAY_BRUSH));
    SelectObject(dc,GetStockObject(DEFAULT_GUI_FONT)); handles=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
    for(i=0;i<30;i++) spell_map_draw(dc,NULL,40,0,0);
    assert(GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS)<=handles+1);
    snprintf(path,sizeof(path),"%s.magic-board.bmp",preview_path); screenshot(board,path); ReleaseDC(board,dc); DestroyWindow(board);
    magic.turn=3; assert(!strcmp(magic_corpse_status(&heroes[1].magic),"Resurrection expired"));
    /* Empty footprints are valid; board-edge placement is not. */
    magic_fixture(); all_bright_ready();
    { SPELL_CAST empty; choose_spell(&empty,"Flames of Death",-1); empty.x=1; empty.y=8;
      assert(cast_prepare(&empty,0) && !empty.count); empty.x=11; heroes[0].x=10; heroes[0].y=8; assert(!cast_prepare(&empty,0)); }
}
static void bright_effect_checks(const char *table_path,const char *preview_path) {
    SPELL_CAST c; COMBAT melee; HWND w; int base,damage,slot,before; char pool[32];
    static GAME_DATA d; GAME_DATA *loaded; unsigned char *bytes; size_t size;
    magic_fixture(); all_bright_ready();
    /* Named monster deaths retain a corpse; resurrection removes its death-ledger entry. */
    monsters.tokens[0].unique=1; strcpy(monsters.tokens[0].character_id,"test:orc-leader"); monsters.tokens[0].wounds=1;
    assert(monster_damage(0)); assert(monsters.tokens[0].magic.death_turn==1 && monsters.tokens[0].x==-1 && monsters.dead_count==1);
    game_command(MGAMETURN); choose_spell(&c,"Power of the Phoenix",HERO_LIMIT); c.test_ready=1; c.test_roll=1;
    assert(cast_prepare(&c,0) && cast_apply(&c)); assert(monsters.tokens[0].wounds==8 && monsters.tokens[0].x==5 && !monsters.dead_count);
    assert(!turn_can_move(1,0) && turn_attack_reason(1,0,0) && turn_move_remaining(1,0)==0);
    game_command(MGAMEUNDO); assert(!monsters.tokens[0].wounds && monsters.dead_count==1);
    magic_fixture(); all_bright_ready();
    /* Armour is consulted by damage, survives combat turns, and expires at exploration. */
    choose_spell(&c,"Dragon Armour",0); base=heroes[0].stats[3];
    assert(cast_prepare(&c,0) && cast_apply(&c)); assert(heroes[0].stats[3]==base);
    assert(magic_toughness(12,&heroes[0].magic)==12);
    strcpy(pool,"8"); assert(damage_auto(pool,sizeof(pool),1,8,&damage) && damage==1);
    strcpy(pool,"8"); assert(damage_auto(pool,sizeof(pool),1,magic_toughness(8,&heroes[0].magic),&damage) && !damage);
    game_command(MGAMETURN); assert(heroes[0].magic.armour);
    choose_spell(&c,"Courage",0); base=heroes[0].stats[5]; assert(cast_prepare(&c,0) && cast_apply(&c));
    assert(heroes[0].stats[5]==base && magic_bravery(base,&heroes[0].magic)==12);
    game_command(MGAMEEXPLORATION); assert(!heroes[0].magic.armour && !heroes[0].magic.courage && magic.exploration);
    assert(turn_move_remaining(0,0)==12);
    /* Swift Wind retains its roll when the target selection exceeds the limit. */
    choose_spell(&c,"Swift Wind",0); c.count=2; c.targets[1]=1; c.effect_roll=2;
    before=heroes[0].caster.components[component_index("Fire Dust")];
    assert(!cast_prepare(&c,0) && c.effect_roll==2); assert(heroes[0].caster.components[component_index("Fire Dust")]==before);
    c.count=1; assert(cast_prepare(&c,0) && cast_apply(&c)); assert(turn_move_remaining(0,0)==18);
    game_command(MGAMEUNDO); assert(!heroes[0].magic.swift && turn_move_remaining(0,0)==12);
    game_command(MGAMEGUIDED); choose_spell(&c,"Swift Wind",0); c.effect_roll=12; assert(cast_prepare(&c,0) && cast_apply(&c));
    assert(turn_move_remaining(0,0)==2*heroes[0].stats[4]); game_command(MGAMETURN); game_command(MGAMETURN); assert(!heroes[0].magic.swift);
    /* Flaming Hand starts in the following combat turn; a twelve causes twelve Wounds without bonus dice. */
    choose_spell(&c,"Flaming Hand of Destruction",-1); assert(cast_prepare(&c,0) && cast_apply(&c)); assert(!magic_hand(&heroes[0].magic));
    game_command(MGAMETURN); game_command(MGAMETURN); assert(magic_hand(&heroes[0].magic));
    monsters.tokens[0].x=2; monsters.tokens[0].y=1; monsters.tokens[0].wounds=monsters.tokens[0].stats[7]=20; monsters.tokens[0].stats[3]=99;
    memset(&melee,0,sizeof(melee)); melee.hero=heroes[0]; melee.monster=monsters.tokens[0]; melee.pending=-1;
    w=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGCOMBAT),NULL,combat_proc,(LPARAM)&melee); assert(w);
    SetDlgItemInt(w,MCHIT,12,FALSE); SetDlgItemTextA(w,MCDAMAGE,"12"); assert(combat_calculate(w,&melee)); assert(melee.monster.wounds==8);
    assert(combat_apply(&melee)); DestroyWindow(w); game_command(MGAMEUNDO); assert(monsters.tokens[0].wounds==20);
    game_command(MGAMEEXPLORATION); assert(!magic_hand(&heroes[0].magic));
    /* Still Air blocks movement, attacks and free attacks throughout the GM phase. */
    game_command(MGAMEGUIDED); choose_spell(&c,"Still Air",-1); c.x=3; c.y=1; assert(cast_prepare(&c,0) && cast_apply(&c));
    game_command(MGAMETURN); assert(magic_stopped(0) && !turn_can_move(1,0) && turn_attack_reason(1,0,0));
    memset(&melee,0,sizeof(melee)); melee.hero=heroes[0]; melee.monster=monsters.tokens[0]; melee.resolved=1; melee.pending=1;
    assert(!combat_can_next(&melee)); gm_override=1; assert(turn_can_move(1,0)); gm_override=0;
    game_command(MGAMETURN); assert(!magic_stopped(0));
    /* Forced runs obey walls/occupancy/death zones and spend the target's action. Undo restores it. */
    choose_spell(&c,"Flight",1); c.effect_roll=12; c.x=10; c.y=2; monsters.tokens[0].x=5;
    heroes[1].attacked=1; assert(!cast_targets_reason(&c)); /* printed eligibility checks movement */
    heroes[1].moved=1; assert(cast_targets_reason(&c)); heroes[1].moved=0;
    heroes[1].attacked=0;
    assert(cast_prepare(&c,0)); assert(c.dest_x<10); assert(cast_apply(&c)); assert(heroes[1].moved && heroes[1].attacked && !turn_can_move(0,1));
    assert(turn_attack_reason(0,1,0)); game_command(MGAMEUNDO); assert(heroes[1].x==2 && heroes[1].y==2 && !heroes[1].moved);
    /* KO is not a death. Death records preserve location and allow only the next turn. */
    choose_spell(&c,"Power of the Phoenix",1); assert(cast_targets_reason(&c));
    heroes[1].wounds=0; heroes[1].condition=2; magic_hero_death(&heroes[1]); assert(heroes[1].x==-1);
    assert(cast_targets_reason(&c)); game_command(MGAMETURN); game_command(MGAMETURN); assert(!cast_targets_reason(&c));
    c.test_ready=1; c.test_roll=12; heroes[0].stats[6]=9; heroes[0].fate=2;
    assert(cast_prepare(&c,0) && cast_apply(&c)); assert(heroes[1].magic.soul_lost && HERO_DEAD(&heroes[1]));
    game_command(MGAMEUNDO); assert(!heroes[1].magic.soul_lost);
    heroes[1].magic.armour=heroes[1].magic.courage=heroes[1].magic.swift=1;
    heroes[1].run_bonus=5;
    assert(cast_prepare(&c,1) && cast_apply(&c)); assert(HERO_ACTIVE(&heroes[1]) && heroes[1].x==2 && heroes[0].fate==1);
    assert(!heroes[1].magic.armour && !heroes[1].magic.courage && !heroes[1].magic.swift && !heroes[1].run_bonus);
    assert(!turn_can_move(0,1) && turn_attack_reason(0,1,0) && turn_move_remaining(0,1)==0);
    game_command(MGAMEUNDO);
    game_command(MGAMETURN); game_command(MGAMETURN); assert(cast_targets_reason(&c));
    /* Pack snapshot + temporary effects + corpse/clock metadata round-trip; malformed timers are refused. */
    heroes[0].magic.armour=heroes[0].magic.courage=1;
    assert(capture(&d) && game_encode(&d,&bytes,&size)); free(d.cells); loaded=game_decode(bytes,size); assert(loaded);
    assert(!memcmp(&loaded->magic,&magic,sizeof(magic))); assert(!memcmp(&loaded->heroes[1].magic,&heroes[1].magic,sizeof(MAGIC_MODEL)));
    loaded->magic.still_until[0]=loaded->magic.turn+2; { unsigned char *bad; size_t n; assert(!game_encode(loaded,&bad,&n)); }
    game_data_free(loaded);
    /* v14 stock/known spells are retained; absent timing is not fabricated. */
    size-=magic_extension(&d); bytes[8]=14; version8_crc(bytes,size); loaded=game_decode(bytes,size); assert(loaded);
    assert(!loaded->magic.turn && !loaded->heroes[1].magic.death_turn); assert(loaded->heroes[0].caster.components[0]==heroes[0].caster.components[0]);
    game_data_free(loaded); free(bytes);
    /* Two rooms touching along a wall: spy does not reveal/spawn; a permanent door opens that edge. */
    {
        PICE pieces[4]; int cells[100],i; unsigned char fog[4]={1,0,0,0};
        memset(pieces,0,sizeof(pieces)); pieces[0].type=pieces[1].type=NORMAL_ROOM; pieces[0].w=pieces[0].h=pieces[1].w=pieces[1].h=5; pieces[1].x=5; pieces[2].type=pieces[3].type=EMPTY;
        for(i=0;i<100;i++) cells[i]=i/10<5?(i%10<5?0:1):-1;
        assert(map_restore(pieces,4,10,10,cells)); game_set_fog(fog,4); memset(&magic,0,sizeof(magic)); memset(&monsters,0,sizeof(monsters)); monsters.seen[0]=1;
        memset(&heroes[0].magic,0,sizeof(MAGIC_MODEL)); memset(&heroes[1].magic,0,sizeof(MAGIC_MODEL)); heroes[0].x=heroes[0].y=1; heroes[1].x=2; heroes[1].y=2; turn_phase=gm_override=0; turn_reset_side(0);
        choose_spell(&c,"Open Window",-1); c.x=6; c.y=1; assert(cast_prepare(&c,0) && cast_apply(&c));
        assert(magic.spied[1] && !game_fog()[1] && !monsters.seen[1] && !monsters.count);
        game_command(MGAMETURN);
        /* Native wall targeting explains the coordinate/direction controls and hides the manual checkbox. */
        memset(&c,0,sizeof(c)); c.x=4; c.y=2;
        w=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGSPELLCAST),NULL,spellcast_proc,(LPARAM)&c); assert(w);
        for(i=0;i<SendDlgItemMessage(w,SCSpell,CB_GETCOUNT,0,0);i++) if(SendDlgItemMessage(w,SCSpell,CB_GETITEMDATA,i,0)==spell_index("The Bright Key")) break;
        SendDlgItemMessage(w,SCSpell,CB_SETCURSEL,i,0); SendMessage(w,WM_COMMAND,MAKEWPARAM(SCSpell,CBN_SELCHANGE),0);
        SendDlgItemMessage(w,SCDIRECTION,CB_SETCURSEL,East,0); SendMessage(w,WM_COMMAND,MAKEWPARAM(SCDIRECTION,CBN_SELCHANGE),0);
        assert(!IsWindowEnabled(GetDlgItem(w,SCTARGETS))); assert((GetWindowLong(GetDlgItem(w,SCDIRECTION),GWL_STYLE)&WS_VISIBLE) && !(GetWindowLong(GetDlgItem(w,SCMANUAL),GWL_STYLE)&WS_VISIBLE));
        assert(cast_ui_preview(w,&c,0) && c.resolved); { char path[MAX_PATH]; snprintf(path,sizeof(path),"%s.key.bmp",preview_path); screenshot(w,path); } DestroyWindow(w);
        choose_spell(&c,"The Bright Key",-1); c.x=4; c.y=2; c.direction=East;
        assert(cast_prepare(&c,0) && cast_apply(&c)); slot=magic_door_edge(4,2,East); assert(slot>=0 && magic.doors[slot]==1);
        assert(!battle_edge(4,2,5,2)); assert(magic_open_index(slot,4)); assert(magic.doors[slot]==3 && !game_fog()[1]);
        assert(magic_open_index(slot,12) && magic.doors[slot]==3); /* no repeated rock roll */
        game_command(MGAMEUNDO); assert(magic.doors[slot]==1);
        assert(magic_open_index(slot,5)); game_changed(); assert(game_fog()[1] && battle_edge(4,2,5,2) && magic.surprise[1]);
        before=monsters.count; game_changed(); assert(monsters.count==before);
        game_command(MGAMEUNDO); assert(!game_fog()[1] && magic.spied[1] && !magic.surprise[1]);
        game_command(MGAMEUNDO); assert(game_fog()[1] && magic.surprise[1]);
        assert(capture(&d) && game_encode(&d,&bytes,&size)); free(d.cells); loaded=game_decode(bytes,size); assert(loaded && loaded->magic.doors[slot]==2 && loaded->magic.surprise[1]); game_data_free(loaded); free(bytes);
    }
    /* New space uses the campaign's ordinary tables and continuation generator. */
    assert(SetCurrentDirectoryA(table_path) && read_table("sonne2.tab",NULL));
    {
        PICE pieces[1000]; int cells[40*40],i; unsigned char fog[1000]={0};
        memset(pieces,0,sizeof(pieces)); for(i=0;i<1000;i++) pieces[i].type=EMPTY;
        pieces[0].type=NORMAL_ROOM; pieces[0].x=pieces[0].y=15; pieces[0].w=pieces[0].h=5;
        for(i=0;i<1600;i++) cells[i]=(i%40>=15 && i%40<20 && i/40>=15 && i/40<20)?0:-1; fog[0]=1;
        assert(map_restore(pieces,1000,40,40,cells)); game_set_fog(fog,1000); memset(&magic,0,sizeof(magic)); memset(&monsters,0,sizeof(monsters)); monsters.seen[0]=1;
        memset(&heroes[0].magic,0,sizeof(MAGIC_MODEL)); memset(&heroes[1].magic,0,sizeof(MAGIC_MODEL)); heroes[0].x=16; heroes[0].y=16; heroes[1].x=17; heroes[1].y=16; turn_phase=gm_override=0; turn_reset_side(0);
        choose_spell(&c,"The Bright Key",-1); c.x=19; c.y=17; c.direction=East; assert(cast_prepare(&c,0) && cast_apply(&c)); slot=magic_door_edge(19,17,East); assert(slot>=0);
        new_rand(1234); assert(magic_open_index(slot,5)); assert(game_fog()[slot] && floor_piece(20,17)>=0);
        game_changed(); assert(capture(&d) && game_encode(&d,&bytes,&size)); free(d.cells); loaded=game_decode(bytes,size); assert(loaded); game_data_free(loaded); free(bytes);
        game_command(MGAMEUNDO); assert(floor_piece(20,17)<0 && !game_fog()[slot]);
    }
}

int main(int argc,char **argv) {
    const CHARACTER_PACK *p; int i,fire,phoenix,dust,tooth,heal,inferno; SPELL_CAST c; BOOK_EDIT e; HWND w;
    static GAME_DATA d; GAME_DATA *loaded; unsigned char *bytes; size_t size,oldsize; CHARACTER_PACK *legacy,*upgraded; unsigned char *oldpack;
    INITCOMMONCONTROLSEX controls={sizeof(controls),ICC_LISTVIEW_CLASSES|ICC_TAB_CLASSES}; InitCommonControlsEx(&controls);
    assert(mem_alloc(40,40,1000,4096)); heap_clear(); magic_fixture(); p=pack_current();
    assert(p && p->book_count==2 && p->spell_count==15 && p->component_count==8);
    assert(caster_has_book(&heroes[0].caster)); assert(heroes[0].caster.setup_pending && heroes[0].caster.starting_allowance==4);
    fire=spell_index("Flames of Death"); heal=spell_index("Flames of the Phoenix"); inferno=spell_index("Inferno of Doom");
    dust=component_index("Fire Dust"); phoenix=component_index("Phoenix Feather"); tooth=component_index("Dragon Tooth");
    assert(fire>=0 && heal>=0 && inferno>=0); assert(cast_reason(0,0,fire));
    /* Starting components are player choices, not free stock per spell. */
    memset(&e,0,sizeof(e)); e.draft=heroes[0].caster; e.component=-1;
    w=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGSPELLBOOK),NULL,spellbook_proc,(LPARAM)&e); assert(w);
    assert(ListView_GetItemCount(GetDlgItem(w,SBCOMPONENTS))==4);
    assert(ListView_GetItemCount(GetDlgItem(w,SBSPELLS))==12); assert(IsWindowEnabled(GetDlgItem(w,SBSPELLS)));
    assert(!IsWindowEnabled(GetDlgItem(w,SBFINISH)) && !IsWindowEnabled(GetDlgItem(w,IDOK)));
    ListView_SetItemState(GetDlgItem(w,SBSPELLS),2,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
    assert(!memcmp(&e.draft,&heroes[0].caster,sizeof(CASTER_STATE))); /* selecting does not learn */
    e.draft.components[dust]=4; book_lists(w,&e); assert(IsWindowEnabled(GetDlgItem(w,SBFINISH)));
    SendMessage(w,WM_COMMAND,SBFINISH,0); assert(!e.draft.setup_pending); assert(heroes[0].caster.setup_pending);
    assert(IsWindowEnabled(GetDlgItem(w,SBGMADJUST)));
    { char path[MAX_PATH]; snprintf(path,sizeof(path),"%s.book.bmp",argv[3]); screenshot(w,path); }
    DestroyWindow(w);
    heroes[0].caster.components[dust]=3; heroes[0].caster.components[phoenix]=1; heroes[0].caster.setup_pending=0;
    assert(!cast_reason(0,0,fire));
    heroes[1].x=5; heroes[1].y=2;
    memset(&c,0,sizeof(c)); c.index=0; c.spell=fire; c.x=5; c.y=1; c.count=2; c.targets[0]=1; c.targets[1]=HERO_LIMIT;
    assert(!cast_targets_reason(&c)); assert(cast_prepare(&c,0)); assert(strstr(c.result,heroes[1].name)); assert(strstr(c.result,"orc"));
    c.damage[0]=heroes[1].wounds; c.damage[1]=1; assert(cast_apply(&c));
    assert(HERO_KO(&heroes[1]) && heroes[1].x==5); assert(monsters.tokens[0].wounds==7);
    assert(strstr(c.result,"knocked out") && strstr(c.result,"7 remaining") && strstr(c.result,"Resources spent"));
    assert(heroes[0].caster.components[dust]==2 && heroes[0].caster.cast_used); assert(cast_reason(0,0,fire));
    game_command(MGAMEUNDO); assert(heroes[0].caster.components[dust]==3 && !heroes[0].caster.cast_used); assert(HERO_ACTIVE(&heroes[1]));
    /* Healing at zero Wounds; another adjacent figure blocks casting. */
    heroes[1].x=2; heroes[1].y=1; heroes[1].wounds=0; heroes[1].condition=1;
    memset(&c,0,sizeof(c)); c.index=0; c.spell=heal; c.count=1; c.targets[0]=1;
    monsters.tokens[0].x=1; monsters.tokens[0].y=2; assert(cast_targets_reason(&c));
    monsters.tokens[0].x=5; monsters.tokens[0].y=1;
    assert(cast_prepare(&c,0)); assert(cast_apply(&c)); assert(HERO_ACTIVE(&heroes[1])); assert(!heroes[0].caster.components[phoenix]);
    game_command(MGAMETURN); assert(!heroes[0].caster.cast_used);
    /* Two components: a prior move blocks casting; a cast blocks later movement. */
    heroes[0].caster.known[inferno]=1; heroes[1].y=2; heroes[0].caster.components[tooth]=1; heroes[0].moved=1;
    assert(cast_reason(0,0,inferno)); heroes[0].moved=0;
    memset(&c,0,sizeof(c)); c.index=0; c.spell=inferno; c.x=5; c.y=1; c.count=1; c.targets[0]=HERO_LIMIT;
    assert(cast_prepare(&c,1)); c.damage[0]=0; assert(cast_apply(&c));
    assert(!turn_can_move(0,0)); assert(heroes[0].caster.components[tooth]==0); assert(heroes[0].caster.components[dust]==2);
    game_command(MGAMETURN); assert(turn_can_move(0,0));
    /* Armour changes effective Toughness, never the printed characteristic. */
    memset(&c,0,sizeof(c)); c.index=0; c.spell=spell_index("Dragon Armour"); c.count=1; c.targets[0]=0;
    heroes[0].caster.components[component_index("Red Dragon Dust")]=1;
    i=heroes[0].stats[3]; assert(cast_prepare(&c,0)); assert(strstr(c.result,"+1 Toughness")); assert(cast_apply(&c)); assert(heroes[0].stats[3]==i && magic_toughness(i,&heroes[0].magic)==i+1);
    game_command(MGAMETURN);
    /* Native casting dialog lists learned spells and has no weapon hit roll. */
    memset(&c,0,sizeof(c)); c.index=0; c.x=c.y=1;
    w=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGSPELLCAST),NULL,spellcast_proc,(LPARAM)&c); assert(w);
    assert(SendDlgItemMessage(w,SCSpell,CB_GETCOUNT,0,0)==5); assert(GetDlgItem(w,SCROLL)); assert(!IsWindowEnabled(GetDlgItem(w,IDOK))); screenshot(w,argv[3]); DestroyWindow(w);
    /* Failed Intelligence test offers Fate after rolling and retains that roll. */
    heroes[0].caster.components[dust]=2; heroes[0].caster.components[tooth]=1;
    memset(&c,0,sizeof(c)); c.index=0; c.x=5; c.y=1;
    w=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGSPELLCAST),NULL,spellcast_proc,(LPARAM)&c); assert(w);
    SendDlgItemMessage(w,SCSpell,CB_SETCURSEL,4,0); SendMessage(w,WM_COMMAND,MAKEWPARAM(SCSpell,CBN_SELCHANGE),0);
    { int n; for(n=0;n<SendDlgItemMessage(w,SCTARGETS,LB_GETCOUNT,0,0);n++) if(SendDlgItemMessage(w,SCTARGETS,LB_GETITEMDATA,n,0)==HERO_LIMIT) SendDlgItemMessage(w,SCTARGETS,LB_SETSEL,TRUE,n); }
    c.test_ready=1; c.test_roll=12; c.fate_offer=1; heroes[0].stats[6]=9; heroes[0].fate=2;
    cast_dialog_status(w,&c); assert(IsWindowEnabled(GetDlgItem(w,SCFATE)));
    SendMessage(w,WM_COMMAND,MAKEWPARAM(SCTARGETS,LBN_SELCHANGE),0);
    assert(c.test_ready && c.test_roll==12 && c.fate_offer && !c.resolved);
    assert(IsWindowEnabled(GetDlgItem(w,SCFATE)));
    SendMessage(w,WM_COMMAND,SCFATE,0); assert(c.test_roll==12 && c.fate_used && c.resolved);
    assert(heroes[0].fate==2 && heroes[0].caster.components[tooth]==1); assert(IsWindowEnabled(GetDlgItem(w,IDOK)));
    screenshot(w,argv[3]); DestroyWindow(w);
    /* GM edits are isolated from the main draft until accepted. */
    { BOOK_EDIT gm; CASTER_STATE before=heroes[0].caster; QUANTITY_EDIT qty;
      memset(&gm,0,sizeof(gm)); gm.draft=before;
      w=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGSPELLADJUST),NULL,gm_book_proc,(LPARAM)&gm); assert(w);
      SendDlgItemMessage(w,SBBOOK,CB_SETCURSEL,1,0); SendMessage(w,WM_COMMAND,MAKEWPARAM(SBBOOK,CBN_SELCHANGE),0);
      SendMessage(w,WM_COMMAND,SBToggleBook,0); assert(gm.draft.books[1]); assert(!memcmp(&before,&heroes[0].caster,sizeof(before))); DestroyWindow(w);
      qty.component=dust; qty.value=2;
      w=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGSPELLQUANTITY),NULL,quantity_proc,(LPARAM)&qty); assert(w);
      SetDlgItemTextA(w,SBQUANTITY,"10000"); SendMessage(w,WM_COMMAND,IDOK,0); assert(qty.value==2);
      SetDlgItemTextA(w,SBQUANTITY,"3"); SendMessage(w,WM_COMMAND,IDOK,0); assert(qty.value==3); DestroyWindow(w);
    }
    /* Remaining-stock migration has no four-component requirement. */
    memset(&e,0,sizeof(e)); e.draft=heroes[0].caster; e.draft.setup_pending=2; e.component=-1;
    w=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGSPELLBOOK),NULL,spellbook_proc,(LPARAM)&e); assert(w);
    assert(IsWindowEnabled(GetDlgItem(w,SBFINISH)) && IsWindowEnabled(GetDlgItem(w,IDOK)));
    DestroyWindow(w);
    /* New save snapshots round-trip personal state exactly. */
    assert(capture(&d)); assert(game_encode(&d,&bytes,&size)); free(d.cells);
    assert(bytes[8]==15); loaded=game_decode(bytes,size); assert(loaded);
    assert(!memcmp(&loaded->heroes[0].caster,&heroes[0].caster,sizeof(CASTER_STATE))); assert(loaded->pack->book_count==2);
    game_data_free(loaded);
    /* Range-check quantities before narrowing them into unsigned short storage. */
    {
        size_t offset=size-magic_extension(&d)-464*(hero_count+monsters.count)+4*(SPELL_BOOK_LIMIT+SPELL_LIMIT);
        unsigned char save[4]; memcpy(save,bytes+offset,4);
        bytes[offset]=0; bytes[offset+1]=0; bytes[offset+2]=1; bytes[offset+3]=0; version8_crc(bytes,size);
        assert(!game_decode(bytes,size)); memcpy(bytes+offset,save,4); version8_crc(bytes,size);
    }
    /* A real v13 suffix loads with zero stock and an explicit setup prompt. */
    oldsize=size-magic_extension(&d)-464*(hero_count+monsters.count); bytes[8]=13; version8_crc(bytes,oldsize);
    loaded=game_decode(bytes,oldsize); assert(loaded && loaded->heroes[0].caster.setup_pending==2);
    for(i=0;i<SPELL_COMPONENT_LIMIT;i++) assert(!loaded->heroes[0].caster.components[i]); game_data_free(loaded); free(bytes);
    /* A genuine pre-effect v14 snapshot upgrades manual handlers and descriptions,
       preserving caster stock, learned slots and profile definitions. */
    {
        FILE *fp=fopen("legacy4.hqp","rb"); long length; assert(fp);
        fseek(fp,0,SEEK_END); length=ftell(fp); rewind(fp); oldpack=(unsigned char*)malloc(length); assert(oldpack);
        assert(fread(oldpack,1,length,fp)==length); fclose(fp); legacy=pack_decode(oldpack,length); free(oldpack); assert(legacy);
        assert(legacy->spells[0].effect==SPELL_MANUAL);
        assert(capture(&d)); d.pack=legacy; assert(game_encode(&d,&bytes,&size)); free(d.cells);
        size-=magic_extension(&d); bytes[8]=14; version8_crc(bytes,size); loaded=game_decode(bytes,size); assert(loaded);
        assert(loaded->pack->spells[0].effect==SPELL_ARMOUR && loaded->pack->spells[0].target==4);
        assert(!strncmp(loaded->pack->spells[0].text,"A model",7));
        assert(!memcmp(&loaded->heroes[0].caster,&heroes[0].caster,sizeof(CASTER_STATE)));
        assert(!strcmp(loaded->pack->heroes[3].text,legacy->heroes[3].text));
        game_data_free(loaded); free(bytes); pack_free(legacy);
    }
    /* HQPACK3 snapshot upgrade preserves profile definitions. */
    oldpack=(unsigned char*)malloc(p->magic_offset); assert(oldpack); memcpy(oldpack,p->bytes,p->magic_offset); memcpy(oldpack,"HQPACK3\n",8);
    legacy=pack_decode(oldpack,p->magic_offset); assert(legacy && !legacy->book_count); upgraded=pack_upgrade_magic(legacy); assert(upgraded && upgraded->book_count==2);
    assert(!strcmp(legacy->heroes[3].text,upgraded->heroes[3].text)); pack_free(legacy); pack_free(upgraded); free(oldpack);
    for(i=1;i<=2;i++) {
        char filename[30]; FILE *fp; long length;
        sprintf(filename,"legacy%d.hqp",i); fp=fopen(filename,"rb"); assert(fp);
        fseek(fp,0,SEEK_END); length=ftell(fp); rewind(fp); oldpack=(unsigned char*)malloc(length); assert(oldpack);
        assert(fread(oldpack,1,length,fp)==length); fclose(fp);
        legacy=pack_decode(oldpack,length); assert(legacy); upgraded=pack_upgrade_magic(legacy); assert(upgraded && upgraded->book_count==2);
        assert(!strcmp(upgraded->heroes[3].text,legacy->heroes[3].text)); assert(upgraded->heroes[3].stats[6]==legacy->heroes[3].stats[6]);
        pack_free(legacy); pack_free(upgraded); free(oldpack);
    }
    /* Per-caster integrity: unknown learned slots, invalid stock, duplicate target. */
    heroes[0].caster.known[63]=1; assert(!pack_caster_valid(&heroes[0].caster,p)); heroes[0].caster.known[63]=0;
    memset(&c,0,sizeof(c)); c.spell=fire; c.count=2; c.targets[0]=c.targets[1]=HERO_LIMIT; c.x=5; c.y=1; assert(cast_targets_reason(&c));
    /* A second book uses the same engine and independent ingredients. */
    memset(&heroes[0].caster,0,sizeof(CASTER_STATE)); heroes[0].caster.books[1]=1; heroes[0].caster.known[spell_index("Fireball")]=1;
    heroes[0].caster.components[component_index("Pinch of Warpstone")]=1;
    assert(pack_caster_valid(&heroes[0].caster,p)); assert(!cast_reason(0,0,spell_index("Fireball"))); assert(cast_reason(0,0,fire));
    bright_effect_checks(argv[1],argv[3]); map_spell_checks(argv[3]);
    game_shutdown(); mem_freeall(); puts("PASS: spell registries, two books, stock, damage/healing, all Bright handlers, turn timing, forced runs, magic doors/generation, spying, resurrection, native dialogs, Undo and v15/v14/v13 migration."); return 0;
}

'''
with tempfile.TemporaryDirectory(prefix='hq-spell-test-') as temp:
    work = Path(temp)
    (work / 'check.c').write_text(HARNESS)
    for version in (1,2): (work/f'legacy{version}.hqp').write_bytes(legacy_resource(version))
    old_magic=copy.deepcopy(source)
    for sp in old_magic['spells']:
        if sp['effect'] not in ('manual','damage','heal'):
            sp['effect']='manual'; sp['target']='manual'; sp['range']=0; sp['text']='GM resolves this effect.'
    legacy4=bytearray(compiler.compile_pack(old_magic)[:-8*len(old_magic['spells'])])
    legacy4[:8]=b'HQPACK4\n'
    (work/'legacy4.hqp').write_bytes(legacy4)
    exclude = {'main.obj', 'maindm.obj', 'winddm.obj', 'game.obj'}
    objects = [str(p) for p in (ROOT / 'obj/msvc').glob('*.obj') if p.name not in exclude]
    command = ['cl', '/nologo', '/D_CRT_SECURE_NO_WARNINGS', '/I'+str(ROOT),
               '/I'+str(ROOT/'my_lib'), '/I'+str(ROOT/'my_lib/windows'), '/I'+str(ROOT/'rsh'),
               'check.c', '/Fe:check.exe', *objects, str(ROOT/'obj/msvc/menu.res'), str(ROOT/'obj/msvc/grafic.res'),
               'user32.lib', 'gdi32.lib', 'shell32.lib', 'comdlg32.lib', 'version.lib', 'winspool.lib', 'ole32.lib', 'advapi32.lib', 'comctl32.lib']
    subprocess.run(command, cwd=work, check=True)
    subprocess.run([str(work/'check.exe'), str(ROOT/'tables'), str(work), str(ROOT/'obj/spellcast-preview.bmp')], cwd=work, check=True, timeout=60)
