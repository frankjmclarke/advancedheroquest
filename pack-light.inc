/* Append Light definitions to older fantasy campaigns and save snapshots by ID. Existing slots,
   printed profiles, spells and inventories retain their meaning. */
static CHARACTER_PACK *pack_write_snapshot(const CHARACTER_PACK *p)
{
    PACK_WRITER w; CHARACTER_PACK *result; int i,j,total=p->hero_count+p->monster_count;
    w.size=PACK_BYTES_LIMIT; w.bytes=(unsigned char*)malloc(w.size); if(!w.bytes) return NULL;
    memcpy(w.bytes,"HQPACK6\n",8); w.pos=8; w.ok=1;
    pack_write_string(&w,p->id); pack_write_string(&w,p->name); pack_write_string(&w,p->nonmonsters);
    pack_write_number(&w,p->legacy_references); pack_write_number(&w,p->hero_count); pack_write_number(&w,p->monster_count);
    for(i=0;i<total;i++) {
        const PACK_PROFILE *q=i<p->hero_count?&p->heroes[i]:&p->monsters[i-p->hero_count];
        pack_write_string(&w,q->id); pack_write_string(&w,q->name); pack_write_string(&w,q->aliases); pack_write_string(&w,q->text);
        for(j=0;j<HERO_STATS;j++) pack_write_number(&w,q->stats[j]);
        pack_write_number(&w,q->kind); pack_write_number(&w,q->fate); pack_write_number(&w,q->unique);
        pack_write_string(&w,q->melee.weapon); pack_write_number(&w,q->melee.dice);
        for(j=0;j<12;j++) pack_write_number(&w,q->melee.hit[j]);
        pack_write_number(&w,q->melee.critical); pack_write_number(&w,q->melee.fumble); pack_write_number(&w,q->melee.reach?q->melee.reach:1);
        pack_write_string(&w,q->ranged.weapon); pack_write_number(&w,q->ranged.kind); pack_write_number(&w,q->ranged.range); pack_write_number(&w,q->ranged.dice);
        for(j=0;j<5;j++) pack_write_number(&w,q->ranged.hit[j]);
        pack_write_number(&w,q->ranged.critical); pack_write_number(&w,q->ranged.fumble);
    }
    pack_write_number(&w,p->component_count); pack_write_number(&w,p->spell_count); pack_write_number(&w,p->book_count);
    for(i=0;i<p->component_count;i++) { pack_write_string(&w,p->components[i].id); pack_write_string(&w,p->components[i].name); pack_write_number(&w,p->components[i].price); }
    for(i=0;i<p->spell_count;i++) {
        const PACK_SPELL *s=&p->spells[i];
        pack_write_string(&w,s->id); pack_write_string(&w,s->name); pack_write_string(&w,s->text);
        pack_write_number(&w,s->effect); pack_write_number(&w,s->target); pack_write_number(&w,s->range); pack_write_number(&w,s->dice);
        pack_write_number(&w,s->test); pack_write_number(&w,s->failed_dice); pack_write_number(&w,s->stationary); pack_write_number(&w,s->cost);
        for(j=0;j<p->component_count;j++) pack_write_number(&w,s->components[j]);
    }
    for(i=0;i<p->book_count;i++) {
        pack_write_string(&w,p->books[i].id); pack_write_string(&w,p->books[i].name);
        for(j=0;j<p->spell_count;j++) pack_write_number(&w,p->books[i].spells[j]);
        for(j=0;j<p->spell_count;j++) pack_write_number(&w,p->books[i].starting[j]);
    }
    for(i=0;i<total;i++) {
        const PACK_PROFILE *q=i<p->hero_count?&p->heroes[i]:&p->monsters[i-p->hero_count];
        for(j=0;j<p->book_count;j++) pack_write_number(&w,q->books[j]);
        pack_write_number(&w,q->starting_components);
        for(j=0;j<p->component_count;j++) pack_write_number(&w,q->components[j]);
    }
    for(i=0;i<p->spell_count;i++) { pack_write_number(&w,p->spells[i].template_width); pack_write_number(&w,p->spells[i].template_height); }
    for(i=0;i<total;i++) pack_write_number(&w,i<p->hero_count?p->heroes[i].traits:p->monsters[i-p->hero_count].traits);
    result=w.ok?pack_decode(w.bytes,w.pos):NULL; free(w.bytes); return result;
}
CHARACTER_PACK *pack_extend_light(const CHARACTER_PACK *old)
{
    const CHARACTER_PACK *f=pack_fantasy(); CHARACTER_PACK *p,*result; int i,j,k,book=-1,ci[SPELL_COMPONENT_LIMIT],si[SPELL_LIMIT];
    PACK_PROFILE *heroes; PACK_COMPONENT *components; PACK_SPELL *spells; PACK_SPELLBOOK *books;
    if(!old || strcmp(old->id,"fantasy") || !f) return pack_clone(old);
    for(i=0;i<old->book_count;i++) if(!strcmp(old->books[i].id,"fantasy:book:light")) return pack_clone(old);
    for(i=0;i<f->book_count;i++) if(!strcmp(f->books[i].id,"fantasy:book:light")) book=i;
    if(book<0 || old->hero_count>=PACK_LIMIT || old->book_count>=SPELL_BOOK_LIMIT || old->component_count+13>SPELL_COMPONENT_LIMIT || old->spell_count+12>SPELL_LIMIT) return pack_clone(old);
    p=pack_clone(old); if(!p) return NULL;
    heroes=(PACK_PROFILE*)realloc(p->heroes,(p->hero_count+1)*sizeof(*heroes)); if(!heroes) goto bad; p->heroes=heroes;
    components=(PACK_COMPONENT*)realloc(p->components,SPELL_COMPONENT_LIMIT*sizeof(*components)); if(!components) goto bad; p->components=components;
    spells=(PACK_SPELL*)realloc(p->spells,SPELL_LIMIT*sizeof(*spells)); if(!spells) goto bad; p->spells=spells;
    books=(PACK_SPELLBOOK*)realloc(p->books,SPELL_BOOK_LIMIT*sizeof(*books)); if(!books) goto bad; p->books=books;
    for(i=0;i<f->component_count;i++) {
        int needed=0; ci[i]=-1;
        for(j=0;j<f->spell_count;j++) if(f->books[book].spells[j] && f->spells[j].components[i]) needed=1;
        if(!needed) continue;
        for(j=0;j<p->component_count;j++) if(!strcmp(p->components[j].id,f->components[i].id)) break;
        if(j==p->component_count) p->components[p->component_count++]=f->components[i]; ci[i]=j;
    }
    for(i=0;i<f->spell_count;i++) {
        si[i]=-1; if(!f->books[book].spells[i]) continue;
        for(j=0;j<p->spell_count;j++) if(!strcmp(p->spells[j].id,f->spells[i].id)) break;
        si[i]=j;
        if(j==p->spell_count) {
            p->spells[p->spell_count++]=f->spells[i]; memset(p->spells[j].components,0,sizeof(p->spells[j].components));
            for(k=0;k<f->component_count;k++) if(ci[k]>=0) p->spells[j].components[ci[k]]=f->spells[i].components[k];
        }
    }
    j=p->book_count++; memset(&p->books[j],0,sizeof(p->books[j])); strcpy(p->books[j].id,f->books[book].id); strcpy(p->books[j].name,f->books[book].name);
    for(i=0;i<f->spell_count;i++) if(si[i]>=0) { p->books[j].spells[si[i]]=1; p->books[j].starting[si[i]]=f->books[book].starting[i]; }
    for(i=0;i<f->hero_count;i++) if(!strcmp(f->heroes[i].id,"fantasy:light-wizard")) {
        for(k=0;k<p->hero_count;k++) if(!strcmp(p->heroes[k].id,f->heroes[i].id)) break;
        if(k==p->hero_count) { p->heroes[p->hero_count++]=f->heroes[i]; memset(p->heroes[k].books,0,sizeof(p->heroes[k].books)); memset(p->heroes[k].components,0,sizeof(p->heroes[k].components)); p->heroes[k].books[j]=1; }
    }
    if(memcmp(old->bytes,"HQPACK6\n",8)) for(i=0;i<p->monster_count;i++) for(j=0;j<f->monster_count;j++) if(!strcmp(p->monsters[i].id,f->monsters[j].id)) { p->monsters[i].traits=f->monsters[j].traits; break; }
    result=pack_write_snapshot(p); pack_free(p); return result;
bad: pack_free(p); return NULL;
}
