"""Run in an MSVC developer shell: python tests/monster-reference-check.py.

Compile the real liste.c formatter and test source fidelity, encounter parsing,
ambiguous variants, missing names, and large room descriptions.
"""
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from ahq_monsters import PAGES, ROOT, c_header, reference

assert (ROOT / 'ahq-reference-data.h').read_text(encoding='ascii') == c_header()
assert PAGES[6][2]['damage'] == ''
assert PAGES[5][3]['hits'][-2:] == ['11', '12']
assert 'blank in source' in reference(PAGES[6][2], 7, 3)

HARNESS = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pice.h>
#include <liste.h>
PICE *Pice = NULL;
_WORD MAX_PICE = 0;
FEATURE *Titel = NULL;
void abbruch(const char *message) { assert(!message); }
static char *expand(const char *line) {
    _UBYTE *lines[2];
    lines[0] = (_UBYTE *)line; lines[1] = NULL;
    return (char *)room_contents_text(lines);
}
static int occurrences(const char *text, const char *needle) {
    int count = 0;
    while ((text = strstr(text, needle)) != NULL) { count++; text += strlen(needle); }
    return count;
}
int main(void) {
    char *text;
    int i;
    _UBYTE *many[301];
    text = expand("14 Zombies (115 Gold Crowns)\"");
    assert(strstr(text,"14 Zombies (115 Gold Crowns)\"\r\n") == text);
    assert(strstr(text,"ZOMBIE\r\n"));
    assert(strstr(text,"HAND-TO-HAND COMBAT") && strstr(text,"RANGED COMBAT"));
    assert(strstr(text,"Cleaver (counts as a sword). Fearsome monster."));
    assert(!strstr(text,".pdf") && !strstr(text,"page 5"));
    free(text);
    text = expand("1 Orc Champion, 2 Orcs AND 4 Goblins (50 Gold Crowns)");
    assert(occurrences(text,"ORC CHAMPION\r\n") == 1);
    assert(occurrences(text,"ORC\r\n") == 1);
    assert(occurrences(text,"GOBLIN\r\n") == 1);
    free(text);
    text = expand("1 Orc Champion (30 Gold Crowns)");
    assert(!strstr(text,"ORC\r\n")); free(text);
    text = expand("2 Ghouls, 1 Wight, 3 Skaven Gutter Runners");
    assert(occurrences(text,"Multiple source profiles") == 3);
    assert(occurrences(text,"GHOUL (variant") == 2);
    assert(occurrences(text,"WIGHT (variant") == 2);
    assert(occurrences(text,"SKAVEN GUTTER RUNNER (variant") == 2);
    assert(strstr(text,"GHOUL (variant 1 of 2)") && strstr(text,"GHOUL (variant 2 of 2)"));
    assert(!strstr(text,".pdf"));
    free(text);
    text = expand("2 Zombies, 3 Zombie");
    assert(occurrences(text,"ZOMBIE\r\n") == 1); free(text);
    text = expand("3 zOmBiEs"); assert(strstr(text,"ZOMBIE\r\n")); free(text);
    text = expand("1 Goblin Archer, 1 White Skaven Sorcerer");
    assert(strstr(text,"11*") && strstr(text,"Damage dice: (blank in source)")); free(text);
    text = expand("2 Zombies, 1 Zombie Dragon (150 Gold Crowns)");
    assert(occurrences(text,"ZOMBIE\r\n") == 1);
    assert(strstr(text,"No matching monster reference found: Zombie Dragon.")); free(text);
    text = expand("1 Champion");
    assert(strstr(text,"No matching monster reference found: Champion.")); free(text);
    text = expand("Hidden treasure: 115 Gold Crowns");
    assert(strcmp(text,"Hidden treasure: 115 Gold Crowns\r\n")==0); free(text);
    text = expand("115 Gold Crowns");
    assert(strcmp(text,"115 Gold Crowns\r\n")==0); free(text);
    text = expand("A locked chest (trapped)");
    assert(strcmp(text,"A locked chest (trapped)\r\n")==0); free(text);
    text = (char *)room_contents_text(NULL);
    assert(strcmp(text,"No contents recorded for this room.")==0); free(text);
    for(i=0;i<300;i++) many[i]=(_UBYTE *)"14 Zombies (115 Gold Crowns)";
    many[300]=NULL;
    text=(char *)room_contents_text(many);
    assert(text && strlen(text)>65535 && occurrences(text,"ZOMBIE\r\n")==300);
    free(text);
    puts("PASS: real C formatter, plural/case/mixed encounters, exact names, all variants, source blanks, no false subtype matches, unchanged notes, and >64KB output.");
    return 0;
}
'''

with tempfile.TemporaryDirectory() as tmp:
    work = Path(tmp)
    (work / 'check.c').write_text(HARNESS)
    subprocess.run(['cl', '/nologo', '/I' + str(ROOT), '/I' + str(ROOT / 'my_lib'),
                    str(ROOT / 'liste.c'), 'check.c', '/Fe:check.exe'], cwd=work, check=True)
    subprocess.run([str(work / 'check.exe')], cwd=work, check=True)
