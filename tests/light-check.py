"""Native College of Light effects, combat integration, inventory and v16/v15 saves."""
from pathlib import Path
import ast
import subprocess
import tempfile
import importlib.util
import json
ROOT = Path(__file__).resolve().parents[1]
def constant(file, name):
    tree=ast.parse((ROOT/file).read_text())
    node=next(n.value for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id==name for t in n.targets))
    if isinstance(node,ast.Constant): return node.value
    return max((n.value for n in ast.walk(node) if isinstance(n,ast.Constant) and isinstance(n.value,str)),key=len)
base=constant('tests/game-check.py','HARNESS').split('int main(int argc')[0]
spell=constant('tests/spell-check.py','HARNESS').split('int main(int argc')[0]
monster=(ROOT/'tests/monster-check.py').read_text(); start=monster.index('static void screenshot')
screenshot=monster[start:monster.index('int main(int argc',start)]
HARNESS=base+screenshot+spell+r'''
static void light_fixture(void) {
    const PACK_PROFILE *p; int i;
    magic_fixture(); p=pack_profile("fantasy:light-wizard"); assert(p);
    pack_hero(&heroes[0],p,1); heroes[0].x=heroes[0].y=1; heroes[1].x=2; heroes[1].y=1;
    assert(heroes[0].caster.books[2] && !heroes[0].caster.books[0]);
    for(i=0;i<4;i++) assert(heroes[0].caster.known[15+i]);
    assert(!heroes[0].caster.known[19]);
    heroes[0].caster.setup_pending=0;
    for(i=15;i<27;i++) heroes[0].caster.known[i]=1;
    for(i=0;i<pack_current()->component_count;i++) heroes[0].caster.components[i]=20;
}
static void light_cast(SPELL_CAST *c,const char *name,int target) {
    heroes[0].caster.cast_used=heroes[0].caster.move_locked=0; heroes[0].moved=0;
    if(!strcmp(name,"Sleep of Ages")) heroes[1].y=2;
    choose_spell(c,name,target);
    if(pack_current()->spells[c->spell].target==8) c->count=0;
    if(!cast_prepare(c,0)) { fprintf(stderr,"%s: %s\n",name,c->result); exit(3); } assert(cast_apply(c));
}
int main(int argc,char **argv) {
    SPELL_CAST c; COMBAT melee; GAME_DATA d,*loaded; unsigned char *bytes; size_t size,oldsize; int before,damage; char pool[100];
    assert(mem_alloc(40,40,1000,4096)); heap_clear(); assert(pack_activate(pack_fantasy())); light_fixture();
    /* Native book selection starts with Light, four known spells and five eligible ingredient kinds. */
    {
        BOOK_EDIT e; HWND w;
        memset(&e,0,sizeof(e)); pack_hero(&heroes[0],pack_profile("fantasy:light-wizard"),1); e.draft=heroes[0].caster; e.component=-1;
        w=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGSPELLBOOK),NULL,spellbook_proc,(LPARAM)&e); assert(w);
        assert(SendDlgItemMessage(w,SBBOOK,CB_GETCURSEL,0,0)==2);
        assert(ListView_GetItemCount(GetDlgItem(w,SBSPELLS))==12 && ListView_GetItemCount(GetDlgItem(w,SBCOMPONENTS))==5);
        assert(IsWindowEnabled(GetDlgItem(w,IDOK)) && !IsWindowEnabled(GetDlgItem(w,SBFINISH)));
        assert(e.component>=0); before=e.component; e.draft.components[before]=1;
        book_lists(w,&e); SendMessage(w,WM_COMMAND,IDOK,0);
        assert(e.draft.setup_pending==1 && pack_caster_valid(&e.draft,pack_current()));
        heroes[0].caster=e.draft;
        assert(capture(&d) && game_encode(&d,&bytes,&size)); free(d.cells);
        loaded=game_decode(bytes,size); assert(loaded && loaded->heroes[0].caster.setup_pending==1);
        assert(loaded->heroes[0].caster.components[before]==1);
        game_data_free(loaded); free(bytes);
        screenshot(w,argv[1]); DestroyWindow(w); light_fixture();
    }
    assert(pack_current()->hero_count==10 && pack_current()->spell_count==27);
    /* Friends do not block Light healing; enemies do. Bright healing is unchanged. */
    hero_count=3; hero_defaults(&heroes[2],0,3); heroes[2].x=1; heroes[2].y=2;
    heroes[1].wounds=1; light_cast(&c,"Power of Life",1); assert(heroes[1].wounds==heroes[1].stats[7]);
    heroes[0].caster.cast_used=0; monsters.tokens[0].x=0; monsters.tokens[0].y=1;
    choose_spell(&c,"Power of Life",1); assert(cast_targets_reason(&c));
    light_fixture(); before=heroes[1].stats[0]; light_cast(&c,"Strength of Life",1);
    assert(heroes[1].stats[0]==before && magic_ws(before,&heroes[1].magic)==before+1);
    memset(&melee,0,sizeof(melee)); melee.hero=heroes[1]; melee.monster=monsters.tokens[0];
    assert(combat_dice(&melee)==heroes[1].melee.dice+1);
    light_cast(&c,"Cloak of Protection",-1);
    assert(magic_attack_toughness(4,&heroes[0].magic,0)==5 && magic_attack_toughness(4,&heroes[0].magic,TRAIT_UNDEAD)==6);
    assert(magic_attack_toughness(12,&heroes[0].magic,TRAIT_GREATER_DAEMON)==12);
    light_cast(&c,"Blinding Light",-1); assert(monsters.tokens[0].magic.blind && !heroes[1].magic.blind);
    assert(!turn_can_move(0,0)); assert(magic_hit_modifier(&monsters.tokens[0].magic,&heroes[0].magic)==-1);
    magic_exploration(); assert(!heroes[0].magic.cloak && !heroes[1].magic.strength && !monsters.tokens[0].magic.blind);
    light_fixture(); magic_begin_combat(); heroes[1].wounds=1;
    light_cast(&c,"Regeneration",1); assert(heroes[1].magic.regen==2);
    magic_gm_phase(); assert(heroes[1].wounds==2); magic_end_combat(); assert(heroes[1].magic.regen);
    magic_begin_combat(); magic_gm_phase(); assert(heroes[1].wounds==3); magic_end_combat(); assert(!heroes[1].magic.regen);
    light_fixture(); light_cast(&c,"Banish Fear",-1); assert(heroes[0].magic.fear && heroes[1].magic.fear);
    light_fixture(); light_cast(&c,"Sleep of Ages",HERO_LIMIT);
    assert(monsters.tokens[0].magic.sleep && !turn_can_move(1,0) && turn_attack_reason(1,0,0));
    melee.hero=heroes[0]; melee.monster=monsters.tokens[0]; assert(combat_hit(&melee,3)==7 && combat_dice(&melee)==3);
    strcpy(pool,"4 4 4"); assert(damage_auto(pool,sizeof(pool),3,6-2,&damage) && damage==3);
    /* Wake action requires adjacency and consumes movement, attack and casting. */
    monsters.count=2; monsters.tokens[1]=monsters.tokens[0]; monsters.tokens[1].magic.sleep=0; monsters.tokens[1].x=6;
    assert(light_wake(1,0,1)); assert(!monsters.tokens[0].magic.sleep && !turn_can_move(1,1) && monsters.tokens[1].caster.cast_used);
    light_fixture(); { int i; for(i=0;i<pack_current()->monster_count;i++) if(pack_current()->monsters[i].traits==TRAIT_UNDEAD) break; assert(i<pack_current()->monster_count); strcpy(monsters.tokens[0].profile_id,pack_current()->monsters[i].id); }
    heroes[1].y=2; choose_spell(&c,"Dagger of Banishment",HERO_LIMIT); c.effect_roll=12; assert(cast_prepare(&c,0) && c.damage[0]==8); assert(cast_apply(&c)); assert(!monsters.tokens[0].wounds);
    light_fixture(); magic_begin_combat(); heroes[1].condition=2; heroes[1].wounds=0; magic_hero_death(&heroes[1]);
    choose_spell(&c,"Restore Life",1); assert(cast_reason(0,0,c.spell)); magic_end_combat();
    c.test_roll=1; c.test_ready=1; assert(cast_prepare(&c,0) && cast_apply(&c)); assert(HERO_ACTIVE(&heroes[1]) && heroes[1].x==2);
    light_fixture(); magic_begin_combat(); heroes[1].condition=2; heroes[1].wounds=0; magic_hero_death(&heroes[1]); magic_end_combat();
    choose_spell(&c,"Restore Life",1); c.test_roll=12; c.test_ready=1; heroes[0].stats[6]=1;
    assert(cast_prepare(&c,0) && cast_apply(&c)); assert(HERO_DEAD(&heroes[1]) && !heroes[1].magic.soul_lost);
    light_fixture(); heroes[1].condition=2; heroes[1].wounds=0; magic_hero_death(&heroes[1]);
    choose_spell(&c,"Restore Life",1); c.test_roll=1; c.test_ready=1;
    assert(cast_prepare(&c,0) && cast_apply(&c)); assert(HERO_ACTIVE(&heroes[1]));
    /* Closed-door inspection shows visible features without fog, spawning or hidden details. */
    light_fixture(); {
        PICE pieces[3]; int cells[144],i; unsigned char fog[3]={1,0,0};
        memset(pieces,0,sizeof(pieces)); pieces[0].type=pieces[1].type=NORMAL_ROOM;
        pieces[0].w=pieces[1].w=6; pieces[0].h=pieces[1].h=12; pieces[1].x=6; pieces[1].feature=Chest;
        init_pice(&pieces[2],DOOR,5,1,East);
        for(i=0;i<144;i++) cells[i]=i%12<6?0:1;
        assert(map_restore(pieces,3,12,12,cells)); game_set_fog(fog,3); memset(&monsters,0,sizeof(monsters));
        heroes[0].x=4; heroes[0].y=1; heroes[1].y=2;
        choose_spell(&c,"Light of Learning",-1); c.count=0; c.x=5; c.y=1; c.direction=East;
        assert(cast_prepare(&c,0)); assert(strstr(c.result,"Chest (contents concealed)"));
        assert(cast_apply(&c)); assert(!game_fog()[1] && !game_fog()[2] && !monsters.count && !magic.spied[1]);
        heroes[0].caster.cast_used=0;
        spellcast_open(NULL,0,0); assert(spell_map_dialog);
        SendDlgItemMessage(spell_map_dialog,SCSpell,CB_SETCURSEL,9,0);
        SendMessage(spell_map_dialog,WM_COMMAND,MAKEWPARAM(SCSpell,CBN_SELCHANGE),0);
        assert(IsWindowVisible(GetDlgItem(spell_map_dialog,SCCENTREX)) && IsWindowEnabled(GetDlgItem(spell_map_dialog,SCMAPPICK)));
        assert(spell_map_click(5,1,NULL));
        SendMessage(spell_map_dialog,WM_COMMAND,SCROLL,0);
        assert(IsWindowEnabled(GetDlgItem(spell_map_dialog,IDOK)));
        SendMessage(spell_map_dialog,WM_COMMAND,IDOK,0); assert(spell_map_cast()->committed);
        SendMessage(spell_map_dialog,WM_COMMAND,IDOK,0); assert(!spell_map_dialog);
    }
    light_fixture(); light_cast(&c,"Remove Venom",-1); assert(magic.venom[0]);
    magic_begin_combat(); light_cast(&c,"Escape",-1); assert(magic.escaped[0] && heroes[0].x<0 && heroes[1].x<0 && !magic.combat_active);
    assert(turn_attack_reason(1,0,0)); assert(!turn_can_move(1,0));
    /* State roundtrip and authentic v15 prefix: old fields retained, new fields default zero. */
    light_fixture(); heroes[1].magic.strength=heroes[0].magic.cloak=monsters.tokens[0].magic.sleep=1; magic.venom[0]=1;
    assert(capture(&d) && game_encode(&d,&bytes,&size)); free(d.cells);
    loaded=game_decode(bytes,size); assert(loaded && loaded->heroes[1].magic.strength && loaded->magic.venom[0]); game_data_free(loaded);
    oldsize=size-16-32*(d.hero_count+d.monsters.count)-2*d.count; bytes[8]=15; version8_crc(bytes,oldsize);
    loaded=game_decode(bytes,oldsize); assert(loaded && !loaded->heroes[1].magic.strength && !loaded->magic.venom[0]); game_data_free(loaded); free(bytes);
    /* A real pre-Light snapshot with reordered ingredients extends by ID. */
    { FILE *fp=fopen("legacy5.hqp","rb"); CHARACTER_PACK *old,*extended; long n; unsigned char *resource;
      assert(fp); fseek(fp,0,SEEK_END); n=ftell(fp); rewind(fp); resource=(unsigned char*)malloc(n); assert(resource);
      assert(fread(resource,1,n,fp)==n); fclose(fp); old=pack_decode(resource,n); free(resource); assert(old && old->book_count==2);
      /* Preserved local campaign files must expose Light in the actual party picker. */
      { char error[256],label[100]; HWND party;
        assert(pack_select_campaign("legacy.tab",error));
        assert(pack_current()->hero_count==10 && pack_current()->book_count==3);
        assert(pack_current()->heroes[3].stats[6]==8);
        party=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGPARTY),NULL,party_proc,0); assert(party);
        assert(SendDlgItemMessage(party,GPCLASS,CB_GETCOUNT,0,0)==10);
        assert(SendDlgItemMessageA(party,GPCLASS,CB_GETLBTEXT,9,(LPARAM)label)>=0 && !strcmp(label,"Light Wizard"));
        DestroyWindow(party);
      }
      magic_fixture(); heroes[0].caster.setup_pending=0; heroes[0].caster.components[0]=7;
      assert(capture(&d)); d.pack=old; assert(game_encode(&d,&bytes,&size)); free(d.cells);
      /* Also upgrade v16 saves written with a preserved pre-Light campaign. */
      loaded=game_decode(bytes,size); assert(loaded && loaded->pack->hero_count==10 && loaded->pack->book_count==3);
      assert(loaded->heroes[0].caster.components[0]==7 && !loaded->heroes[0].caster.books[2]); game_data_free(loaded);
      oldsize=size-16-32*(d.hero_count+d.monsters.count)-2*d.count; bytes[8]=15; version8_crc(bytes,oldsize);
      loaded=game_decode(bytes,oldsize); assert(loaded && loaded->pack->book_count==3 && loaded->pack->hero_count==10);
      assert(loaded->heroes[0].caster.components[0]==7 && !loaded->heroes[0].caster.books[2]);
      assert(!strcmp(loaded->pack->components[0].id,old->components[0].id)); assert(loaded->pack->heroes[3].stats[6]==8);
      extended=pack_extend_light(loaded->pack); assert(extended && extended->spell_count==loaded->pack->spell_count); pack_free(extended);
      game_data_free(loaded); free(bytes); pack_free(old);
    }
    game_shutdown(); mem_freeall(); puts("PASS: Light starting spells, inventory, conditional armour, WS/Strength, blindness, regeneration timing, fear, sleep/wake, banishment, Restore Life, Escape, venom and v16/v15 saves."); return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='hq-light-test-') as temp:
    work=Path(temp); (work/'check.c').write_text(HARNESS)
    spec=importlib.util.spec_from_file_location('compile_packs',ROOT/'tools/compile-packs.py')
    compiler=importlib.util.module_from_spec(spec); spec.loader.exec_module(compiler)
    old=json.loads((ROOT/'data/packs/fantasy.json').read_text())
    old['heroes']=old['heroes'][:9]; old['heroes'][3]['stats'][6]=8
    old['spells']=old['spells'][:15]; old['spellbooks']=old['spellbooks'][:2]; old['components']=old['components'][:8][::-1]
    legacy=bytearray(compiler.compile_pack(old)[:-4*(len(old['heroes'])+len(old['monsters']))]); legacy[:8]=b'HQPACK5\n'
    (work/'legacy5.hqp').write_bytes(legacy)
    (work/'legacy.tab').write_text(';character-pack legacy5.hqp\n')
    exclude={'main.obj','maindm.obj','winddm.obj','game.obj'}
    objects=[str(p) for p in (ROOT/'obj/msvc').glob('*.obj') if p.name not in exclude]
    subprocess.run(['cl','/nologo','/D_CRT_SECURE_NO_WARNINGS','/I'+str(ROOT),'/I'+str(ROOT/'my_lib'),'/I'+str(ROOT/'my_lib/windows'),'/I'+str(ROOT/'rsh'),'check.c','/Fe:check.exe',*objects,str(ROOT/'obj/msvc/menu.res'),str(ROOT/'obj/msvc/grafic.res'),'user32.lib','gdi32.lib','shell32.lib','comdlg32.lib','version.lib','winspool.lib','ole32.lib','advapi32.lib','comctl32.lib'],cwd=work,check=True)
    subprocess.run([str(work/'check.exe'),str(ROOT/'obj/light-spellbook-preview.bmp')],cwd=work,check=True,timeout=60)
