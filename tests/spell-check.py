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
for change in ('missing_component','missing_book','missing_spell','duplicate','unsupported','starting_outside_book','moving_two_components'):
    bad=copy.deepcopy(source)
    if change=='missing_component': bad['spells'][0]['components']={'fantasy:component:missing':1}
    if change=='missing_book': bad['heroes'][3]['caster']['books']=['fantasy:book:missing']
    if change=='missing_spell': bad['spellbooks'][0]['spells'].append('fantasy:spell:missing')
    if change=='duplicate': bad['spellbooks'][0]['id']=bad['spells'][0]['id']
    if change=='unsupported': bad['spells'][0]['effect']='run-arbitrary-script'
    if change=='starting_outside_book': bad['spellbooks'][0]['starting_spells'].append('fantasy:spell:fireball')
    if change=='moving_two_components': bad['spells'][10]['stationary']=0
    try: compiler.compile_pack(bad)
    except ValueError: pass
    else: raise AssertionError('Invalid pack accepted: '+change)
# A future book and a component-free spell compile through the same format.
future=copy.deepcopy(source)
future['spells'].append(dict(id='fantasy:spell:future',name='Future spell',text='A future harmless GM effect.',effect='manual',target='manual',components={}))
future['spellbooks'].append(dict(id='fantasy:book:future',name='Future book',spells=['fantasy:spell:future'],starting_spells=['fantasy:spell:future']))
assert compiler.compile_pack(future).startswith(b'HQPACK4\n')
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
    memset(&monsters,0,sizeof(monsters)); monsters.seen[0]=1;
    hero_count=2; hero_defaults(&heroes[0],3,1); heroes[0].x=heroes[0].y=1;
    hero_defaults(&heroes[1],0,2); heroes[1].x=2; heroes[1].y=2;
    monsters.count=1; strcpy(monsters.tokens[0].name,"orc");
    monsters.tokens[0].x=5; monsters.tokens[0].y=1; monsters.tokens[0].stats[0]=5;
    monsters.tokens[0].stats[3]=7; monsters.tokens[0].stats[7]=monsters.tokens[0].wounds=8;
    assert(melee_reference("orc",&monsters.tokens[0].melee));
    strcpy(session_title,"Magic tests"); selected=selected_monster=-1; turn_phase=gm_override=0;
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
    memset(&c,0,sizeof(c)); c.index=0; c.spell=fire; c.x=5; c.y=1; c.count=2; c.targets[0]=1; c.targets[1]=HERO_LIMIT;
    assert(!cast_targets_reason(&c)); assert(cast_prepare(&c,0)); assert(strstr(c.result,heroes[1].name)); assert(strstr(c.result,"orc"));
    c.damage[0]=heroes[1].wounds; c.damage[1]=1; assert(cast_apply(&c));
    assert(HERO_KO(&heroes[1]) && heroes[1].x==2); assert(monsters.tokens[0].wounds==7);
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
    /* Explicit manual effects do not silently change permanent characteristics. */
    memset(&c,0,sizeof(c)); c.index=0; c.spell=spell_index("Dragon Armour");
    heroes[0].caster.components[component_index("Red Dragon Dust")]=1;
    i=heroes[0].stats[3]; assert(cast_prepare(&c,0)); assert(strstr(c.result,"GM resolution required")); assert(cast_apply(&c)); assert(heroes[0].stats[3]==i);
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
    assert(bytes[8]==14); loaded=game_decode(bytes,size); assert(loaded);
    assert(!memcmp(&loaded->heroes[0].caster,&heroes[0].caster,sizeof(CASTER_STATE))); assert(loaded->pack->book_count==2);
    game_data_free(loaded);
    /* Range-check quantities before narrowing them into unsigned short storage. */
    {
        size_t offset=size-464*(hero_count+monsters.count)+4*(SPELL_BOOK_LIMIT+SPELL_LIMIT);
        unsigned char save[4]; memcpy(save,bytes+offset,4);
        bytes[offset]=0; bytes[offset+1]=0; bytes[offset+2]=1; bytes[offset+3]=0; version8_crc(bytes,size);
        assert(!game_decode(bytes,size)); memcpy(bytes+offset,save,4); version8_crc(bytes,size);
    }
    /* A real v13 suffix loads with zero stock and an explicit setup prompt. */
    oldsize=size-464*(hero_count+monsters.count); bytes[8]=13; version8_crc(bytes,oldsize);
    loaded=game_decode(bytes,oldsize); assert(loaded && loaded->heroes[0].caster.setup_pending==2);
    for(i=0;i<SPELL_COMPONENT_LIMIT;i++) assert(!loaded->heroes[0].caster.components[i]); game_data_free(loaded); free(bytes);
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
    game_shutdown(); mem_freeall(); puts("PASS: spell registries, two books, stock, damage/healing, manual effects, turn limits, movement locks, native dialogs, Undo and v14/v13 migration."); return 0;
}

'''
with tempfile.TemporaryDirectory(prefix='hq-spell-test-') as temp:
    work = Path(temp)
    (work / 'check.c').write_text(HARNESS)
    for version in (1,2): (work/f'legacy{version}.hqp').write_bytes(legacy_resource(version))
    exclude = {'main.obj', 'maindm.obj', 'winddm.obj', 'game.obj'}
    objects = [str(p) for p in (ROOT / 'obj/msvc').glob('*.obj') if p.name not in exclude]
    command = ['cl', '/nologo', '/D_CRT_SECURE_NO_WARNINGS', '/I'+str(ROOT),
               '/I'+str(ROOT/'my_lib'), '/I'+str(ROOT/'my_lib/windows'), '/I'+str(ROOT/'rsh'),
               'check.c', '/Fe:check.exe', *objects, str(ROOT/'obj/msvc/menu.res'), str(ROOT/'obj/msvc/grafic.res'),
               'user32.lib', 'gdi32.lib', 'shell32.lib', 'comdlg32.lib', 'version.lib', 'winspool.lib', 'ole32.lib', 'advapi32.lib', 'comctl32.lib']
    subprocess.run(command, cwd=work, check=True)
    subprocess.run([str(work/'check.exe'), str(ROOT/'tables'), str(work), str(ROOT/'obj/spellcast-preview.bmp')], cwd=work, check=True, timeout=60)
