/*
 * Routines for 'generic' spells.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "merc.h"


/* Local functions */

void spell_gen_char(int sn, int level, CHAR_DATA *ch, CHAR_DATA *victim);
void spell_gen_obj(int sn, int level, CHAR_DATA *ch, OBJ_DATA *obj);
void spell_gen_room(int sn, int level, CHAR_DATA *ch, ROOM_INDEX_DATA *room);
void spell_gen_area(int sn, int level, CHAR_DATA *ch);
void spell_gen_aff(int sn, int level, CHAR_DATA *ch, void *vo, int target);
int calc(int sn, char *expr, int level, CHAR_DATA *ch, void *vo, int target);

void spell_generic(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA*)vo;
    OBJ_DATA *obj = (OBJ_DATA *)vo;

/*    if (target == TARGET_NONE
	|| target == TARGET_ROOM)
    {
	bugf("Spell '%s': spell_generic() for TARGET_ROOM and TARGET_NONE "
	     "not implemented yet.", skill_table[sn].name);
	return;
    }*/

    if (IS_SET(skill_table[sn].flags, (SPELL_AREA | SPELL_AREA_SAFE)))
    {
	if (target == TARGET_CHAR)
	    spell_gen_area(sn, level, ch);
	else
	{
	    bugf("Spell_generic: spell '%s' is an area spell, but target != "
		 "TARGET_CHAR.", skill_table[sn].name);
	}
	return;
    }

    switch (target)
    {
    case TARGET_CHAR:
	spell_gen_char(sn, level, ch, victim);
	break;
    case TARGET_OBJ:
	spell_gen_obj(sn, level, ch, obj);
	break;
    case TARGET_ROOM:
	spell_gen_room(sn, level, ch, ch->in_room);
	break;
    case TARGET_NONE:
	bugf("Spell_generic(): '%s' - generic spells for TARGET_NONE not "
	     "implemented yet.", skill_table[sn].name);
	break;
    }
}

void spell_gen_char(int sn, int level, CHAR_DATA *ch, CHAR_DATA *victim)
{
    int dam;
    char *expr;
    bool save = FALSE;

    expr = strdup(skill_table[sn].damage);
    dam = calc(sn, expr, level, ch, victim, TARGET_CHAR);
    free(expr);

    if (skill_table[sn].saves_act == SAVES_IGNORE
	|| !saves_spell(level, victim, skill_table[sn].dam_type))
    {
	if (skill_table[sn].affect)
	    spell_gen_aff(sn, level, ch, victim, TARGET_CHAR);
    }
    else
    {
	save = TRUE;
	
	switch (skill_table[sn].saves_act)
	{
	case SAVES_HALFDAM:
	    dam /= 2;
	    break;
	case SAVES_QUARTERDAM:
	    dam /= 4;
	    break;
	case SAVES_3QTRDAM:
	    dam = dam * 3 / 4;
	    break;
	case SAVES_THIRDDAM:
	    dam /= 3;
	    break;
	case SAVES_2THIRDSDAM:
	    dam = dam * 2 / 3;
	    break;
	case SAVES_ABSORB:
	    victim->hit = UMIN(victim->max_hit, victim->hit + dam/2);
	    act("$N поглощает энергию твоего заклинания.",
		ch, NULL, victim, TO_CHAR);
	    act("Ты поглощаешь энергию заклинания $n1.",
		ch, NULL, victim, TO_VICT);
	    act("$N поглощает энергию заклинания $n1.",
		ch, NULL, victim, TO_NOTVICT);
	    dam = 0;
	    break;
	case SAVES_ABSORB_MANA:
	    victim->mana = UMIN(victim->max_mana, victim->mana + dam/4);
	    act("$N поглощает магическую энергию твоего заклинания.",
		ch, NULL, victim, TO_CHAR);
	    act("Ты поглощаешь магическую энергию заклинания $n1.",
		ch, NULL, victim, TO_VICT);
	    act("$N поглощает магическую энергию заклинания $n1.",
		ch, NULL, victim, TO_NOTVICT);
	    dam = 0;
	    break;
	case SAVES_REFLECT:
	    act("$N отражет твое заклинание обратно на тебя.",
		ch, NULL, victim, TO_CHAR);
	    act("Ты отражешь заклинание обратно на $n3.",
		ch, NULL, victim, TO_VICT);
	    act("$N отражет заклинание обратно на $n3.",
		ch, NULL, victim, TO_NOTVICT);
	    dam = -dam;
	    break;
	case SAVES_REFLECT_HALF:
	    act("$N отражет твое заклинание обратно на тебя.",
		ch, NULL, victim, TO_CHAR);
	    act("Ты отражешь заклинание обратно на $n3.",
		ch, NULL, victim, TO_VICT);
	    act("$N отражет заклинание обратно на $n3.",
		ch, NULL, victim, TO_NOTVICT);
	    dam = -dam;
	    break;
	case SAVES_NONE:
	    act("Твое заклинание не произвело никакого эффекта на $N3.",
		ch, NULL, victim, TO_CHAR);
	    act("Заклинание $n1 не произвело на тебя никакого эффекта.",
		ch, NULL, victim, TO_VICT);
	    act("Заклинание $n1 не произвело никакого эффекта на $N3.",
		ch, NULL, victim, TO_NOTVICT);
	    dam = 0;
	    break;
	}
    }

    if (!save && IS_SET(skill_table[sn].flags, SPELL_INSTANT_KILL))
    {
	for_killed_skills(ch, victim);
	return;
    }
    else if (dam > 0)
    {
	damage(ch, victim, dam, sn, skill_table[sn].dam_type, TRUE, NULL);
    }
    else if (dam < 0)
    {
	damage(victim, ch, -dam, sn, skill_table[sn].dam_type, TRUE, NULL);
    }
}

void spell_gen_obj(int sn, int level, CHAR_DATA *ch, OBJ_DATA *obj)
{
    if (skill_table[sn].affect)
	spell_gen_aff(sn, level, ch, obj, TARGET_OBJ);
}

void spell_gen_room(int sn, int level, CHAR_DATA *ch, ROOM_INDEX_DATA *room)
{
    if (skill_table[sn].affect)
	spell_gen_aff(sn, level, ch, room, TARGET_ROOM);
}

void spell_gen_area(int sn, int level, CHAR_DATA *ch)
{
    CHAR_DATA *fch, *fch_next;

    LIST_FOREACH_SAFE(fch, &ch->in_room->people, room_link, fch_next)
    {
	if ((skill_table[sn].target == TAR_CHAR_OFFENSIVE
		|| skill_table[sn].target == TAR_OBJ_CHAR_OFF)
	    && (SUPPRESS_OUTPUT(is_safe_spell(ch, fch, TRUE))
		|| (IS_NPC(ch)
		    && IS_NPC(fch)
		    && ch->fighting != fch
		    && fch->fighting != ch)
		|| (is_same_group(ch, fch)
		    && IS_SET(skill_table[sn].flags, SPELL_AREA_SAFE))))
	{
	    continue;
	}
	
	spell_gen_char(sn, level, ch, fch);
    }
}

void spell_gen_aff(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    OBJ_DATA *obj = (OBJ_DATA *)vo;
    ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *)room;
    SPELLAFF *saff;
    AFFECT_DATA af;
    char *dur;

    if ((saff = skill_table[sn].affect) == NULL)
	return;

    if ((target == TARGET_CHAR
	    && is_affected(victim, sn)
	    && !IS_SET(skill_table[sn].flags, SPELL_ACCUM)
	    && !IS_SET(skill_table[sn].flags, SPELL_RECASTABLE))
	|| target != TARGET_CHAR)
    {
	return;
    }

    dur = strdup(skill_table[sn].aff_dur);
    af.duration = calc(sn, dur, level, ch, vo, target);
    free(dur);

    for (; saff != NULL; saff = saff->next)
    {
	char *mod;

	mod = strdup(saff->mod);
	af.modifier = calc(sn, mod, level, ch, vo, target);
	free(mod);

	af.where = saff->where;
	af.location = saff->apply;
	af.type = sn;
	af.level = level;
	af.bitvector = saff->bit;
	af.caster_id = 0;

	switch (target)
	{
	case TARGET_CHAR:
	    if (IS_SET(skill_table[sn].flags, SPELL_ACCUM))
		affect_join(victim, &af);
	    else if (IS_SET(skill_table[sn].flags, SPELL_RECASTABLE))
		affect_replace(victim, &af);
	    else
		affect_to_char(victim, &af);
	    break;
	case TARGET_OBJ:
	    if (IS_SET(skill_table[sn].flags, SPELL_ACCUM))
		affect_join_obj(obj, &af);
	    else if (IS_SET(skill_table[sn].flags, SPELL_RECASTABLE))
		affect_replace_obj(obj, &af);
	    else
		affect_to_obj(obj, &af);
	    break;
	case TARGET_ROOM:
	    if (IS_SET(skill_table[sn].flags, SPELL_ACCUM))
		affect_join_room(room, &af);
	    else if (IS_SET(skill_table[sn].flags, SPELL_RECASTABLE))
		affect_replace_room(room, &af);
	    else
		affect_to_room(room, &af);
	    break;
	}
    }
}

/*
 * Calculate damage/duration/modifier from 'expr'
 * Expr varialbes:
 * p - level of spell	    P - percentage
 * l - level of victim/obj  L - level of ch (caster)
 * s - victim's str	    S - caster's str
 * i - ----"--- int	    I - ----"--- int
 * w - ----"--- wis	    W - ----"--- wis
 * x - ----"--- dex	    X - ----"--- dex (x/X due to conflict with 'd'ice)
 * c - ----"--- con	    C - ----"--- con
 * h - ----"--- HP	    H - ----"--- HP
 * m - ----"--- mana	    M - ----"--- mana
 * v - ----"--- moves	    V - ----"--- moves
 * a - ----"--- age	    A - ----"--- age
 *
 * Possible operators and priority from highest to lowest
 * ( )
 * < > = {(min) }(max) (a<b =1 if a < b, =0 if a >= b; a{b =a if a<b, =b if b<a)
 * d(dice)
 * ^(pow)
 * * / %(remainder)
 * + -
 */
int calc(int sn, char *expr, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    OBJ_DATA *obj = (OBJ_DATA *)vo;
    char op;
    int res, res1;
    int i;
    int len = strlen(expr);
    int rop = 0, dop = 0, eop = 0, mop = 0, aop = 0;
    char *hexpr[2];
    int braces_level = 0;
    
    if (!expr || !strlen(expr))
	return 0;

    if (expr[0] == '(' && expr[len - 1] == ')' && !strchr(&expr[1], '('))
    {
	expr[len - 1] = 0;
	expr++;
	len -= 2;
    }

    if (len == 1 && isalpha(expr[0]))
    {
	if (target == TARGET_ROOM
	    || target == TARGET_NONE)
	{
	    bugf("Spell '%s': spell_generic() for TARGET_NONE "
	         "not implemented yet.", skill_table[sn].name);
	    return 0;
	}
	
	switch (expr[0])
	{
	case 'p':
	    return level;
	case 'P':
	    return get_skill(ch, sn);
	case 'L':
	    return ch->level;
	case 'l':
	    if (target == TARGET_CHAR)
		return victim->level;
	    else if (target == TARGET_OBJ)
		return obj->level;
	    else
		return -1;
	    break;
		
	case 'S':
	    return get_curr_stat(ch, STAT_STR);
	case 'I':
	    return get_curr_stat(ch, STAT_INT);
	case 'W':
	    return get_curr_stat(ch, STAT_WIS);
	case 'X':
	    return get_curr_stat(ch, STAT_DEX);
	case 'C':
	    return get_curr_stat(ch, STAT_CON);
	case 'H':
	    return ch->hit;
	case 'M':
	    return ch->mana;
	case 'V':
	    return ch->move;
	case 'A':
	    return get_age(ch);

	case 's':
	    if (target == TARGET_CHAR)
		return get_curr_stat(victim, STAT_STR);
	    else
		return -1;
	    break;
	case 'i':
	    if (target == TARGET_CHAR)
		return get_curr_stat(victim, STAT_INT);
	    else
		return -1;
	    break;
	case 'w':
	    if (target == TARGET_CHAR)
		return get_curr_stat(victim, STAT_WIS);
	    else
		return -1;
	    break;
	case 'x':
	    if (target == TARGET_CHAR)
		return get_curr_stat(victim, STAT_DEX);
	    else
		return -1;
	    break;
	case 'c':
	    if (target == TARGET_CHAR)
		return get_curr_stat(victim, STAT_CON);
	    else
		return -1;
	    break;
	case 'h':
	    if (target == TARGET_CHAR)
		return victim->hit;
	    else
		return -1;
	    break;
	case 'm':
	    if (target == TARGET_CHAR)
		return victim->mana;
	    else
		return -1;
	    break;
	case 'v':
	    if (target == TARGET_CHAR)
		return victim->move;
	    else
		return -1;
	    break;
	case 'a':
	    if (target == TARGET_CHAR)
		return get_age(victim);
	    else
		return -1;
	    break;
	}
    }

    for (i = 0; i < len; i++)
	if (!isdigit(expr[i]))
	    break;

    if (i == len)
	return atoi(expr);

    for (i = 0; i < len; i++)
	switch (expr[i])
	{
	case '<':
	case '>':
    	case '=':
	case '{':
	case '}':
	    if (!braces_level)
		rop = i;
	    break;
	case 'd':
	case 'D':
	    if (!braces_level)
		dop = i;
	    break;
	case '^':
	    if (!braces_level)
		eop = i;
	    break;
	case '*':
	case '/':
	case '%':
	    if (!braces_level)
		mop = i;
	    break;
	case '+':
	case '-':
	    if (!braces_level)
		aop = i;
	    break;
	case '(':
	    braces_level++;
	    break;
	case ')':
	    braces_level--;
	    break;
	}

    if (aop)
	i = aop;
    else if (mop)
	i = mop;
    else if (eop)
	i = eop;
    else if (dop)
	i = dop;
    else
	i = rop;

    op = expr[i];
    expr[i] = 0;
    hexpr[0] = expr;
    hexpr[1] = &expr[i + 1];

    res = calc(sn, hexpr[0], level, ch, vo, target);
    res1 = calc(sn, hexpr[1], level, ch, vo, target);
    switch (op)
    {
    case '+':
	res += res1;
	break;
    case '-':
	res -= res1;
	break;
    case '*':
	res *= res1;
	break;
    case '/':
	res /= res1;
	break;
    case '%':
	res %= res1;
	break;
    case '<':
	res = (res < res1) ? 1 : 0;
	break;
    case '>':
	res = (res > res1) ? 1 : 0;
	break;
    case '=':
	res = (res == res1) ? 1 : 0;
	break;
    case '{':
	res = UMIN(res, res1);
	break;
    case '}':
	res = UMAX(res, res1);
	break;
    case 'd':
    case 'D':
	res = dice(res, res1);
	break;
    case '^':
	if (res1 != 0)
	{
	    int x;
	    int z = res;

	    for (x = 1; x < res1; ++x)
		z *= res;

	    res = z;
	}
	else
	    res = 1;
	break;
    }

    return res;
}

/* charset=cp1251 */
