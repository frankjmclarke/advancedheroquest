static int light_can_wake(int side,int sleeper,int waker)
{
    int x=side?monsters.tokens[sleeper].x:heroes[sleeper].x,y=side?monsters.tokens[sleeper].y:heroes[sleeper].y;
    int wx=side?monsters.tokens[waker].x:heroes[waker].x,wy=side?monsters.tokens[waker].y:heroes[waker].y;
    CASTER_STATE *s=caster_state(side,waker);
    if(waker==sleeper || !magic_model(side,sleeper)->sleep || magic_model(side,waker)->sleep || x<0 || wx<0) return 0;
    if(side?monsters.tokens[waker].wounds<=0:!HERO_ACTIVE(&heroes[waker])) return 0;
    if(abs(x-wx)+abs(y-wy)!=1 || !battle_edge(x,y,wx,wy)) return 0;
    if(!gm_override && ((turn_phase && turn_phase!=(side?2:1)) ||
        (side?monsters.tokens[waker].moved || monsters.tokens[waker].attacked:heroes[waker].moved || heroes[waker].attacked) || s->cast_used)) return 0;
    return 1;
}
static int light_wake(int side,int sleeper,int waker)
{
    char text[256]; int id=side?HERO_LIMIT+waker:waker;
    if(!light_can_wake(side,sleeper,waker)) return 0;
    remember(); magic_model(side,sleeper)->sleep=0;
    if(side) { monsters.tokens[waker].moved=monsters.tokens[waker].attacked=1; monsters.tokens[waker].move_spent=1000000; }
    else { heroes[waker].moved=heroes[waker].attacked=1; heroes[waker].move_spent=1000000; heroes[waker].magic.restore_blocked=1; }
    caster_state(side,waker)->cast_used=caster_state(side,waker)->move_locked=1;
    snprintf(text,sizeof(text),"%s woke %s; its entire turn is used.",spell_target_name(id),spell_target_name(side?HERO_LIMIT+sleeper:sleeper));
    panel_note(text); active=1; game_changed(); return 1;
}
static void light_wake_menu(int side,int sleeper)
{
    HMENU menu=CreatePopupMenu(); POINT at; int i,command,count=0;
    if(!menu) return;
    for(i=0;i<(side?monsters.count:hero_count);i++) if(light_can_wake(side,sleeper,i)) {
        AppendMenuA(menu,MF_STRING,i+1,spell_target_name(side?HERO_LIMIT+i:i)); count++;
    }
    if(count) { GetCursorPos(&at); command=TrackPopupMenuEx(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,at.x,at.y,GlMainHwnd,NULL); if(command) light_wake(side,sleeper,command-1); }
    else message("No adjacent comrade has a whole turn available to wake this model. Move a comrade beside it, then start that side's next phase.");
    DestroyMenu(menu);
}
