"""
Check the quest tables for referential integrity.

Run from the repository root:   python tools/tabcheck.py

Reports, per quest (the .tab files directly in tables/):
  - includes that do not resolve
  - the seven reference tables the engine requires, if any are missing
    (Referenz_Table_Names in table.c; the feature tables in the list below
    it are optional, so they are not required here)
  - Name() references with no matching table definition, which is what the
    program itself reports at load time as "Table not available"

Then lists component .tab files that no quest includes. Those are not errors
- a quest may simply not use one - but an entire folder appearing here means
that content is unreachable.

Conditionals are followed as the program follows them: if/else/endif are
evaluated against the symbols actually defined, so a reference that only
appears in a branch that is never taken is not reported.
"""
import io, os, re, sys, glob

TABLES = 'tables'

REQUIRED_REF = ["Passage-Length","Passage-End","Passage-Feature","Room-Type",
                "Room-Doors","Room-or-Passage","Secret-Doors"]
REQUIRED_FEAT = ["Passage","Dead-End","Corner","T-Junction","Right-Turn","Left-Turn",
                 "Stairs-Down","Stairs-Out","Small","Normal","Hazard","Large","Lair",
                 "Quest","Big","Merscha","Wandering-Monsters"]

NAME = r'[A-Za-z0-9_.\-]+'
CALL = re.compile(r'(' + NAME + r')\s*\(\s*\)')
IDENT = re.compile(r'^(' + NAME + r')\s*$')

def resolve(path):
    """case-insensitive lookup under tables/"""
    if os.path.exists(path):
        return path
    d, b = os.path.split(path)
    if os.path.isdir(d):
        for f in os.listdir(d):
            if f.lower() == b.lower():
                return os.path.join(d, f)
    return None

def read(path):
    return io.open(path, encoding='latin-1', newline='').read().replace('\r\n','\n').split('\n')

def process(path, defines, defs, refs, missing_inc, seen, depth=0):
    real = resolve(path)
    if real is None:
        missing_inc.append(path)
        return
    if depth > 12:
        return
    lines = read(real)
    stack = []          # list of bool: currently emitting?
    i = 0
    while i < len(lines):
        raw = lines[i]
        line = raw.strip()
        i += 1
        low = line.lower()
        # conditionals
        if low.startswith('if '):
            sym = line[3:].strip()
            stack.append(sym in defines)
            continue
        if low == 'else':
            if stack:
                stack[-1] = not stack[-1]
            continue
        if low == 'endif':
            if stack:
                stack.pop()
            continue
        emitting = all(stack)
        if not emitting:
            continue
        if low.startswith('define '):
            defines.add(line[7:].strip())
            continue
        if low.startswith('include '):
            inc = line[8:].strip().replace(chr(92), os.sep)
            process(os.path.join(TABLES, inc), defines, defs, refs,
                    missing_inc, seen, depth+1)
            continue
        if not line or line.startswith(';'):
            continue
        # table definition: bare identifier, next non-blank line opens a paren
        m = IDENT.match(line)
        if m:
            j = i
            while j < len(lines) and not lines[j].strip():
                j += 1
            if j < len(lines) and lines[j].strip().startswith('('):
                defs.setdefault(m.group(1), []).append((os.path.relpath(real), i))
                continue
        # references
        for c in CALL.finditer(line):
            refs.setdefault(c.group(1), []).append((os.path.relpath(real), i))

seen_q = {}
for g in glob.glob(os.path.join(TABLES, '*.tab')) + glob.glob(os.path.join(TABLES, '*.TAB')):
    seen_q[g.lower()] = g
quests = sorted(seen_q.values())
problems = 0
for q in quests:
    defines, defs, refs, missing_inc = set(), {}, {}, []
    process(q, defines, defs, refs, missing_inc, set())
    dl = {k.lower() for k in defs}
    unresolved = sorted({r for r in refs if r.lower() not in dl})
    miss_ref  = [t for t in REQUIRED_REF  if t.lower() not in dl]
    # Feature tables are optional: init_table_names() only validates them
    # when present, so a quest without Merscha rooms simply has no Merscha.
    miss_feat = []
    if unresolved or miss_ref or miss_feat or missing_inc:
        problems += 1
        print('\n%s' % os.path.basename(q))
        for inc in missing_inc:
            print('   MISSING INCLUDE  %s' % inc)
        for t in miss_ref:
            print('   MISSING REQUIRED (reference)  %s' % t)
        for t in miss_feat:
            print('   MISSING REQUIRED (feature)    %s' % t)
        for u in unresolved:
            where = refs[u][0]
            print('   UNDEFINED REF    %-28s used at %s:%d' % (u, where[0], where[1]))
print('\n---- %d of %d quests have problems' % (problems, len(quests)))

print()
def read(p): return io.open(p,encoding='latin-1',newline='').read().replace('\r\n','\n').split('\n')
used=set()
def walk(path, depth=0):
    if depth>12 or not os.path.exists(path): return
    used.add(os.path.normcase(os.path.abspath(path)))
    for line in read(path):
        t=line.strip()
        if t.lower().startswith('include '):
            inc=t[8:].strip().replace(chr(92), os.sep)
            walk(os.path.join(TABLES,inc), depth+1)
q={}
for g in glob.glob(os.path.join(TABLES,'*.tab'))+glob.glob(os.path.join(TABLES,'*.TAB')):
    q[g.lower()]=g
for t in sorted(q.values()):
    walk(t)
allt=set()
for root,dirs,files in os.walk(TABLES):
    for f in files:
        if f.lower().endswith('.tab'):
            allt.add(os.path.normcase(os.path.abspath(os.path.join(root,f))))
orph=sorted(allt-used)
print('Component .tab files no quest includes:')
for o in orph:
    print('   %s' % os.path.relpath(o, os.path.abspath('.')))
print('   (none)' if not orph else '   ---- %d orphan(s)' % len(orph))
