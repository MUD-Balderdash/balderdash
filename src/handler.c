/**************************************************************************r
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/
/***************************************************************************
 *	ROM 2.4 is copyright 1993-1998 Russ Taylor			   *
 *	ROM has been brought to you by the ROM consortium		   *
 *	    Russ Taylor (rtaylor@hypercube.org)				   *
 *	    Gabrielle Taylor (gtaylor@hypercube.org)			   *
 *	    Brian Moore (zump@rom.org)					   *
 *	By using this code, you have agreed to follow the terms of the	   *
 *	ROM license, in the file Rom24/doc/rom.license			   *
 ***************************************************************************/

#if defined(linux)
/* struct statfs is here in linux */
#include <sys/vfs.h>
#endif
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/param.h>
#include "queue.h"
#include <sys/mount.h>


#include "merc.h"
#include "interp.h"
#include "magic.h"
#include "recycle.h"



extern bool mob_holylight;

/* Для get_obj_here, чтобы эмулировать :-) общий список вещей в комнате, инвентаре и экипировке */
int global_obj_count = 0;
bool count_global = FALSE;

void set_resist_flag(int64_t *flag, int dam)
{
    int i;

    for (i = 0; i < DAM_MAX; i++)
	if (damres[i].dam == dam && damres[i].res != -1)
	    SET_BIT(*flag, damres[i].res);
}

int set_resist_num(int64_t res)
{
    int i;

    for (i = 0; i < DAM_MAX; i++)
	if (damres[i].res == res)
	    return damres[i].dam;

    return 0;
}

void set_resist(int16_t *resist, int64_t imm, int64_t res, int64_t vuln)
{
    int i;

    for (i = 0; i < DAM_MAX; i++)
	if (damres[i].res != -1)
	{
	    if (IS_SET(vuln, damres[i].res))
		resist[damres[i].dam] -= 50;
	    if (IS_SET(res, damres[i].res))
		resist[damres[i].dam] += 33;
	    if (IS_SET(imm, damres[i].res))
		resist[damres[i].dam] = 100;
	}
}

/*
 * Local functions.
 */
void affect_modify(CHAR_DATA *ch, AFFECT_DATA *paf, bool fAdd, bool check_wield );
void fix_sex(CHAR_DATA *ch);

int get_moon_phase(int moon)
{
    char phase;
    char period;

    if (moon >= max_moons)
    {
	bugf("get_moon_phase(): moon (%d) >= max_moons (%d).", moon, max_moons);
	return 0;
    }

    phase = moons_data[moon].state;
    period = moons_data[moon].state_period;

    if (phase == 0)
	return MOON_NEW;
    else if (phase > 0 && phase < period / 4)
	return MOON_1QTR;
    else if (phase >= period / 4 && phase < period / 2)
	return MOON_2QTR;
    else if (phase == period / 2)
	return MOON_FULL;
    else if (phase > period / 2 && phase < period * 3 / 4)
	return MOON_3QTR;
    else
	return MOON_4QTR;
}

bool convert_trans(CHAR_DATA *ch, char *arg)
{
    if (IS_TRANSLIT(ch) || !(arg[0] & 0x80))
    {
	recode_trans(arg, FALSE);
	return TRUE;
    }

    return FALSE;
}

ROOM_INDEX_DATA *get_recall(CHAR_DATA *ch)
{
    return (is_clan(ch) && !IS_INDEPEND(ch) && clan_table[ch->clan].recall_vnum > 0)
	? get_room_index(clan_table[ch->clan].recall_vnum)
	: get_room_index(pc_race_table[ch->race].recall);
}

/* friend stuff -- for NPC's mostly */
bool is_friend(CHAR_DATA *ch, CHAR_DATA *victim)
{
    if (is_same_group(ch, victim))
	return TRUE;


    if (!IS_NPC(ch))
	return FALSE;

    if (!IS_NPC(victim))
    {
	if (IS_SET(ch->off_flags, ASSIST_PLAYERS))
	    return TRUE;
	else
	    return FALSE;
    }

    if (IS_AFFECTED(ch, AFF_CHARM))
	return FALSE;

    if (IS_SET(ch->off_flags, ASSIST_ALL))
	return TRUE;

    if (ch->group && ch->group == victim->group)
	return TRUE;

    if (IS_SET(ch->off_flags, ASSIST_VNUM)
	&& ch->pIndexData == victim->pIndexData)
    {
	return TRUE;
    }

    if (IS_SET(ch->off_flags, ASSIST_RACE) && ch->race == victim->race)
	return TRUE;

    if (IS_SET(ch->off_flags, ASSIST_ALIGN)
	&& !IS_SET(ch->act, ACT_NOALIGN)
	&& !IS_SET(victim->act, ACT_NOALIGN)
	&& ((IS_GOOD(ch) && IS_GOOD(victim))
	    || (IS_EVIL(ch) && IS_EVIL(victim))
	    || (IS_NEUTRAL(ch) && IS_NEUTRAL(victim))))
    {
	return TRUE;
    }

    return FALSE;
}

/* returns number of people on an object */
int count_users(OBJ_DATA *obj)
{
    CHAR_DATA *fch;
    int count = 0;

    if (obj->in_room == NULL)
	return 0;

    LIST_FOREACH(fch, &obj->in_room->people, room_link)
    {
	if (fch->on == obj)
	    count++;
    }

    return count;
}

/* returns material number */

/* returns material name -- ROM OLC temp patch */
char *material_name(int16_t num)
{
    return material_table[num].name;
}

int weapon_type (const char *name)
{
    int type;

    for (type = 0; weapon_table[type].name != NULL; type++)
    {
	if (LOWER(name[0]) == LOWER(weapon_table[type].name[0])
	    &&  !str_prefix(name, weapon_table[type].name))
	{
	    return weapon_table[type].type;
	}
    }

    return WEAPON_EXOTIC;
}


char *item_name(int item_type, int rname)
{
    int type;

    for (type = 0; item_table[type].name != NULL; type++)
	if (item_type == item_table[type].type)
	{
	    if (rname == 0)
		return item_table[type].rname;
	    else
		return item_table[type].name;
	}
    return "нет";
}

char *weapon_name(int weapon_type)
{
    int type;

    for (type = 0; weapon_table[type].name != NULL; type++)
	if (weapon_type == weapon_table[type].type)
	    return weapon_table[type].rname;
    return "экзотическое";
}


/* for immunity, vulnerabiltiy, and resistant
 the 'globals' (magic and weapons) may be overriden
 three other cases -- wood, silver, and iron -- are checked in fight.c */
int16_t get_max_resist(CHAR_DATA *ch, int dam_type)
{
    AFFECT_DATA *paf;

    for(paf = ch->affected; paf != NULL; paf = paf->next)
      if (paf->type == gsn_vaccine
         && (paf->bitvector == DAM_POISON || paf->bitvector == DAM_DISEASE)
         && paf->bitvector == dam_type)
	    return 100;

    if (is_lycanthrope(ch)
	&& (dam_type == DAM_CHARM || dam_type == DAM_MENTAL))
	return 100;

    return (IS_NPC(ch) ? 100 : UMAX(75, race_table[ch->race].resists[dam_type]));
}

int16_t get_resist(CHAR_DATA *ch, int dam_type)
{
    return URANGE(-300, ch->resists[dam_type], get_max_resist(ch, dam_type));
}

int16_t check_immune(CHAR_DATA *ch, int dam_type)
{
    int16_t res;

    if (dam_type == DAM_NONE)
	return 0;

    res = ch->resists[dam_type];
    if (dam_type == DAM_BASH
	|| dam_type == DAM_PIERCE
	|| dam_type == DAM_SLASH)
    {
	res += ch->resists[DAM_WEAPON];
    }
    else if (dam_type != DAM_WEAPON
	     && dam_type != DAM_MAGIC
	     && dam_type != DAM_WOOD
	     && dam_type != DAM_SILVER
	     && dam_type != DAM_IRON
	     && dam_type != DAM_OTHER
	     && dam_type != DAM_DROWNING)
    {
	res += ch->resists[DAM_MAGIC];
    }

    return URANGE(-300, res, get_max_resist(ch, dam_type));
}
#if 0
int immune, def;
int bit;

immune = -1;
def = IS_NORMAL;

if (dam_type == DAM_NONE)
    return immune;

if (dam_type <= 3)
{
    if (IS_SET(ch->imm_flags, IMM_WEAPON))
	def = IS_IMMUNE;
    else if (IS_SET(ch->res_flags, RES_WEAPON))
	def = IS_RESISTANT;
    else if (IS_SET(ch->vuln_flags, VULN_WEAPON))
	def = IS_VULNERABLE;
}
else /* magical attack */
{
    if (IS_SET(ch->imm_flags, IMM_MAGIC))
	def = IS_IMMUNE;
    else if (IS_SET(ch->res_flags, RES_MAGIC))
	def = IS_RESISTANT;
    else if (IS_SET(ch->vuln_flags, VULN_MAGIC))
	def = IS_VULNERABLE;
}

/* set bits to check -- VULN etc. must ALL be the same or this will fail */
switch (dam_type)
{
case(DAM_BASH):		bit = IMM_BASH;		break;
case(DAM_PIERCE):	bit = IMM_PIERCE;	break;
case(DAM_SLASH):	bit = IMM_SLASH;	break;
case(DAM_FIRE):		bit = IMM_FIRE;		break;
case(DAM_COLD):		bit = IMM_COLD;		break;
case(DAM_LIGHTNING):	bit = IMM_LIGHTNING;	break;
case(DAM_ACID):		bit = IMM_ACID;		break;
case(DAM_POISON):	bit = IMM_POISON;	break;
case(DAM_NEGATIVE):	bit = IMM_NEGATIVE;	break;
case(DAM_HOLY):		bit = IMM_HOLY;		break;
case(DAM_ENERGY):	bit = IMM_ENERGY;	break;
case(DAM_MENTAL):	bit = IMM_MENTAL;	break;
case(DAM_DISEASE):	bit = IMM_DISEASE;	break;
case(DAM_DROWNING):	bit = IMM_DROWNING;	break;
case(DAM_LIGHT):	bit = IMM_LIGHT;	break;
case(DAM_CHARM):	bit = IMM_CHARM;	break;
case(DAM_SOUND):	bit = IMM_SOUND;	break;
default: 		return def;
}

if (IS_SET(ch->imm_flags, bit))
    immune = IS_IMMUNE;
else if (IS_SET(ch->res_flags, bit) && immune != IS_IMMUNE)
    immune = IS_RESISTANT;
else if (IS_SET(ch->vuln_flags, bit))
{
    if (immune == IS_IMMUNE)
	immune = IS_RESISTANT;
    else if (immune == IS_RESISTANT)
	immune = IS_NORMAL;
    else
	immune = IS_VULNERABLE;
}

if (immune == -1)
    return def;
    else
    return immune;
    }
#endif /* 0 */

/* checks mob format */
bool is_old_mob(CHAR_DATA *ch)
{
    if (ch->pIndexData == NULL)
	return FALSE;
    else if (ch->pIndexData->new_format)
	return FALSE;
    return TRUE;
}

/* for returning skill information */
int get_skill(CHAR_DATA *ch, int sn)
{
    int skill, max_skill = 100;

    if (!ch)
	return 0;

    if (sn == -1) /* shorthand for level based skills */
    {
	skill = ch->level * 5 / 2;
    }
    else if (sn < -1 || sn > max_skills)
    {
	if (sn >= 0)
	    bugf("Bad sn %d in get_skill.", sn);

	skill = 0;
    }
    else if (!IS_NPC(ch))
    {
	if (ch->level < skill_table[sn].skill_level[ch->classid])
	    skill = 0;
	else
	    skill = GET_SKILL(ch, sn);

	skill = UMAX(0, IS_RENEGATE(ch) ? skill - ch->level/2 : skill);
    }
    else /* mobiles */
    {
	if (skill_table[sn].spell_fun != spell_null)
	    skill = 40 + 2 * ch->level;
	else if (sn == gsn_sneak || sn == gsn_hide)
	    skill = ch->level * 2 + 20;
	else if ((sn == gsn_dodge && IS_SET(ch->off_flags, OFF_DODGE))
		 || (sn == gsn_parry && IS_SET(ch->off_flags, OFF_PARRY)))
	{
	    skill = ch->level * 2;
	}
	else if (sn == gsn_shield_block)
	    skill = 10 + 2 * ch->level;
	else if (sn == gsn_second_attack
		 && (IS_SET(ch->act, ACT_WARRIOR)
		     || IS_SET(ch->act, ACT_THIEF)
		     || IS_SET(ch->act, ACT_CLERIC)))
	{
	    skill = 10 + 3 * ch->level;
	}
	else if (sn == gsn_third_attack
		 && (IS_SET(ch->act, ACT_WARRIOR)
		     || IS_SET(ch->act, ACT_THIEF)))
	{
	    skill = 4 * ch->level - 30;
	}
	else if (sn == gsn_fourth_attack && IS_SET(ch->act, ACT_WARRIOR))
	    skill = 3 * ch->level - 40;
	else if (sn == gsn_second_weapon
		 && (IS_SET(ch->act, ACT_WARRIOR)
		     || IS_SET(ch->act, ACT_THIEF)))
	{
	    skill = 3 * ch->level - 20;
	}
	else if (sn == gsn_hand_to_hand)
	    skill = 40 + 2 * ch->level;
	else if (sn == gsn_trip && IS_SET(ch->off_flags, OFF_TRIP))
	    skill = 10 + 3 * ch->level;
	else if (sn == gsn_bash && IS_SET(ch->off_flags, OFF_BASH))
	    skill = 10 + 3 * ch->level;
	else if (sn == gsn_disarm
		 && (IS_SET(ch->off_flags, OFF_DISARM)
		     || IS_SET(ch->act, ACT_WARRIOR)
		     || IS_SET(ch->act, ACT_THIEF)))
	{
	    skill = 20 + 3 * ch->level;
	}
	else if (sn == gsn_berserk && IS_SET(ch->off_flags, OFF_BERSERK))
	    skill = 3 * ch->level;
	else if (sn == gsn_kick && IS_SET(ch->off_flags, OFF_KICK))
	    skill = 10 + 3 * ch->level;
	else if (sn == gsn_rescue)
	    skill = 50 + ch->level;
	else if (sn == gsn_recall)
	    skill = 50 + ch->level;
	else if (sn == gsn_select)
	    skill = 50 + ch->level;
	else if (sn == gsn_sword
		 ||  sn == gsn_dagger
		 ||  sn == gsn_spear
		 ||  sn == gsn_mace
		 ||  sn == gsn_axe
		 ||  sn == gsn_flail
		 ||  sn == gsn_whip
		 ||  sn == gsn_polearm
		 ||  sn == gsn_staff)
	{
	    skill = 40 + 5 * ch->level / 2;
	}
	else if (sn == gsn_cleave && IS_SET(ch->off_flags, OFF_CLEAVE))
	    skill = 100;
	else if (sn == gsn_backstab && IS_SET(ch->off_flags, OFF_BACKSTAB))
	    skill = 40 + 2 * ch->level;
	else if (sn == gsn_dual_backstab && IS_SET(ch->off_flags, OFF_BACKSTAB))
	    skill = 2 * ch->level;
	else if (sn == gsn_run)
	    skill = 50 + ch->level;
	else if (sn == gsn_dirt && IS_SET(ch->off_flags, OFF_KICK_DIRT))
	    skill = 40 + ch->level;
	else if (sn == gsn_tail && IS_SET(ch->off_flags, OFF_TAIL))
	    skill = 50 + ch->level;
	else if ((sn == gsn_ambush || sn == gsn_ambush_move) && IS_SET(ch->off_flags, OFF_AMBUSH))
	    skill = 100;
	else
	    skill = 0;
    }

    if (ch->daze > 0)
    {
	if (skill_table[sn].spell_fun != spell_null)
	    skill /= 2;
	else
	    skill = 2 * skill / 3;
    }

    if (IS_DRUNK(ch))
	skill = 9 * skill / 10;

    if (skill > 1 && is_affected(ch, gsn_animal_dodge) && sn == gsn_dodge)
    {
	skill += ch->level / 3;
	max_skill += ch->level / 3;
    }

    return URANGE(0, skill, max_skill);
}

/* for returning weapon information */
int get_weapon_sn(CHAR_DATA *ch, bool secondary)
{
    OBJ_DATA *wield;
    int sn;

    if (get_eq_char(ch, WEAR_WIELD) == NULL	&& get_eq_char(ch, WEAR_SECONDARY) == NULL) {
		return gsn_hand_to_hand;
    }

    wield = get_eq_char(ch, secondary ? WEAR_SECONDARY : WEAR_WIELD);

    if (wield == NULL || wield->item_type != ITEM_WEAPON) return -2;

    switch (wield->value[0]){
		default :               sn = -1;                break;
		case(WEAPON_SWORD):     sn = gsn_sword;         break;
		case(WEAPON_DAGGER):    sn = gsn_dagger;        break;
		case(WEAPON_SPEAR):     sn = gsn_spear;         break;
		case(WEAPON_MACE):      sn = gsn_mace;          break;
		case(WEAPON_AXE):       sn = gsn_axe;           break;
		case(WEAPON_FLAIL):     sn = gsn_flail;         break;
		case(WEAPON_WHIP):      sn = gsn_whip;          break;
		case(WEAPON_POLEARM):   sn = gsn_polearm;       break;
		case(WEAPON_STAFF):     sn = gsn_staff;         break;
		case(WEAPON_EXOTIC):    sn = gsn_exotic;        break;
    }

    return sn;
}

int get_weapon_skill(CHAR_DATA *ch, int sn)
{
    int skill;

    /* -1 is exotic */
    if (IS_NPC(ch)){
		if (sn == -2) skill = 0;
		else if (sn == -1)
			skill = 3 * ch->level;
		else if (sn == gsn_hand_to_hand)
			skill = 40 + 2 * ch->level;
		else
			skill = 40 + 5 * ch->level / 2;
    } else {
		if (sn == -2)
			skill = 0;
		else if (sn == -1)
			skill = 3 * ch->level;
		else {
			skill = GET_SKILL(ch, sn);
			return UMAX(0, skill);
		}

    }

    return URANGE(0, skill, 100);
}


/* used to de-screw characters */
void reset_char(CHAR_DATA *ch)
{
    int loc, mod, stat;
    OBJ_DATA *obj;
    AFFECT_DATA *af;
    int i;

    if (IS_NPC(ch))
	return;

    if (ch->pcdata->perm_hit == 0
	|| ch->pcdata->perm_mana == 0
	|| ch->pcdata->perm_move == 0)
	//	|| ch->pcdata->last_level == 0   Вроде отсюда росли ноги у баги с ньюбами.
    {
	/* do a FULL reset */
	for (loc = 0; loc < MAX_WEAR; loc++)
	{
	    obj = get_eq_char(ch, loc);
	    if (obj == NULL)
		continue;

	    if (!obj->enchanted && !obj->pIndexData->edited)
		for (af = obj->pIndexData->affected; af; af = af->next)
		{
		    mod = af->modifier;
		    switch (af->location)
		    {
		    case APPLY_SEX:
			ch->sex -= mod;
			fix_sex(ch);
			break;
		    case APPLY_MANA:
			ch->max_mana -= mod;
			break;
		    case APPLY_HIT:
			ch->max_hit -= mod;
			break;
		    case APPLY_MOVE:
			ch->max_move -= mod;
			break;
		    case APPLY_SKILL:
			if (!IS_NPC(ch)
			    && af->type > 0
			    && af->type < max_skills)
			{
			    ch->pcdata->skill_mod[af->type] -= mod;
			}
			break;
		    }
		}

	    for (af = obj->affected; af != NULL; af = af->next)
	    {
		mod = af->modifier;
		switch (af->location)
		{
		case APPLY_SEX:
		    ch->sex -= mod;
		    break;
		case APPLY_MANA:
		    ch->max_mana -= mod;
		    break;
		case APPLY_HIT:
		    ch->max_hit -= mod;
		    break;
		case APPLY_MOVE:
		    ch->max_move -= mod;
		    break;
		case APPLY_SKILL:
		    if (!IS_NPC(ch)
			&& af->type > 0
			&& af->type < max_skills)
		    {
			ch->pcdata->skill_mod[af->type] -= mod;
		    }
		    break;
		}
	    }
	}

	/* now reset the permanent stats */
	ch->pcdata->perm_hit 	= ch->max_hit;
	ch->pcdata->perm_mana 	= ch->max_mana;
	ch->pcdata->perm_move	= ch->max_move;
	ch->pcdata->last_level	= ch->played/3600;

	if (ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > MAX_PC_SEX)
	{
	    if (ch->sex > 0 && ch->sex <= MAX_PC_SEX)
		ch->pcdata->true_sex	= ch->sex;
	    else
		ch->pcdata->true_sex 	= 0;
	}

    }

    /* now restore the character to his/her true condition */
    for (stat = 0; stat < MAX_STATS; stat++)
	ch->mod_stat[stat] = 0;

    if (ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > MAX_PC_SEX)
	ch->pcdata->true_sex = 0;

    ch->sex		= ch->pcdata->true_sex;
    ch->max_hit 	= ch->pcdata->perm_hit;
    ch->max_mana	= ch->pcdata->perm_mana;
    ch->max_move	= ch->pcdata->perm_move;

    for (i = 0; i < 4; i++)
	ch->armor[i]	= 100;

    ch->hitroll		= 0;
    ch->damroll		= 0;
    ch->saving_throw	= 0;

    /* now start adding back the effects */
    for (loc = 0; loc < MAX_WEAR; loc++)
    {
	obj = get_eq_char(ch, loc);
	if (obj == NULL)
	    continue;
	for (i = 0; i < 4; i++)
	    ch->armor[i] -= apply_ac(obj, loc, i);

	if (!obj->enchanted && !obj->pIndexData->edited)
	    for (af = obj->pIndexData->affected; af != NULL; af = af->next)
	    {
		mod = af->modifier;
		switch (af->location)
		{
		case APPLY_STR:
		    ch->mod_stat[STAT_STR] += mod;
		    break;
		case APPLY_DEX:
		    ch->mod_stat[STAT_DEX] += mod;
		    break;
		case APPLY_INT:
		    ch->mod_stat[STAT_INT] += mod;
		    break;
		case APPLY_WIS:
		    ch->mod_stat[STAT_WIS] += mod;
		    break;
		case APPLY_CON:
		    ch->mod_stat[STAT_CON] += mod;
		    break;
		case APPLY_SEX:
		    ch->sex += mod;
		    break;
		case APPLY_MANA:
		    ch->max_mana += mod;
		    break;
		case APPLY_HIT:
		    ch->max_hit	+= mod;
		    break;
		case APPLY_MOVE:
		    ch->max_move += mod;
		    break;
		case APPLY_AC:
		    for (i = 0; i < 4; i ++)
			ch->armor[i] += mod;
		    break;
		case APPLY_HITROLL:
		    ch->hitroll	+= mod;
		    break;
		case APPLY_DAMROLL:
		    ch->damroll += mod;
		    break;
		case APPLY_SAVES:
		case APPLY_SAVING_ROD:
		case APPLY_SAVING_PETRI:
		case APPLY_SAVING_BREATH:
		case APPLY_SAVING_SPELL:
		    ch->saving_throw += mod;
		    break;
		case APPLY_SKILL:
		    if (!IS_NPC(ch)
			&& af->type > 0
			&& af->type < max_skills)
		    {
			ch->pcdata->skill_mod[af->type] += mod;
		    }
		    break;
		}
	    }

	for (af = obj->affected; af != NULL; af = af->next)
	{
	    mod = af->modifier;
	    switch (af->location)
	    {
	    case APPLY_STR:         ch->mod_stat[STAT_STR]  += mod; break;
	    case APPLY_DEX:         ch->mod_stat[STAT_DEX]  += mod; break;
	    case APPLY_INT:         ch->mod_stat[STAT_INT]  += mod; break;
	    case APPLY_WIS:         ch->mod_stat[STAT_WIS]  += mod; break;
	    case APPLY_CON:         ch->mod_stat[STAT_CON]  += mod; break;
	    case APPLY_SEX:         ch->sex                 += mod; break;
	    case APPLY_MANA:        ch->max_mana            += mod; break;
	    case APPLY_HIT:         ch->max_hit             += mod; break;
	    case APPLY_MOVE:        ch->max_move            += mod; break;
	    case APPLY_AC:
				    for (i = 0; i < 4; i ++)
					ch->armor[i] += mod;
				    break;
	    case APPLY_HITROLL:     ch->hitroll             += mod; break;
	    case APPLY_DAMROLL:     ch->damroll             += mod; break;
	    case APPLY_SAVES:         ch->saving_throw += mod; break;
	    case APPLY_SAVING_ROD:          ch->saving_throw += mod; break;
	    case APPLY_SAVING_PETRI:        ch->saving_throw += mod; break;
	    case APPLY_SAVING_BREATH:       ch->saving_throw += mod; break;
	    case APPLY_SAVING_SPELL:        ch->saving_throw += mod; break;
	    case APPLY_SKILL:
					    if (!IS_NPC(ch)
						&& af->type > 0
						&& af->type < max_skills)
					    {
						ch->pcdata->skill_mod[af->type] += mod;
					    }
					    break;
	    }
	}
    }

    /* now add back spell effects */
    for (af = ch->affected; af != NULL; af = af->next)
    {
	mod = af->modifier;
	switch (af->location)
	{
	case APPLY_STR:         ch->mod_stat[STAT_STR]  += mod; break;
	case APPLY_DEX:         ch->mod_stat[STAT_DEX]  += mod; break;
	case APPLY_INT:         ch->mod_stat[STAT_INT]  += mod; break;
	case APPLY_WIS:         ch->mod_stat[STAT_WIS]  += mod; break;
	case APPLY_CON:         ch->mod_stat[STAT_CON]  += mod; break;

	case APPLY_SEX:         ch->sex                 += mod; break;
	case APPLY_MANA:        ch->max_mana            += mod; break;
	case APPLY_HIT:         ch->max_hit             += mod; break;
	case APPLY_MOVE:        ch->max_move            += mod; break;

	case APPLY_AC:
				for (i = 0; i < 4; i ++)
				    ch->armor[i] += mod;
				break;
	case APPLY_HITROLL:     ch->hitroll             += mod; break;
	case APPLY_DAMROLL:     ch->damroll             += mod; break;

	case APPLY_SAVES:         ch->saving_throw += mod; break;
	case APPLY_SAVING_ROD:          ch->saving_throw += mod; break;
	case APPLY_SAVING_PETRI:        ch->saving_throw += mod; break;
	case APPLY_SAVING_BREATH:       ch->saving_throw += mod; break;
	case APPLY_SAVING_SPELL:        ch->saving_throw += mod; break;
	case APPLY_SKILL:
					if (!IS_NPC(ch)
					    && af->type > 0
					    && af->type < max_skills)
					{
					    ch->pcdata->skill_mod[af->type] += mod;
					}
					break;
	}
    }

    /* make sure sex is RIGHT!!!! */
    fix_sex(ch);
}


/*
 * Retrieve a character's trusted level for permission checking.
 */
int get_trust(CHAR_DATA *ch)
{
    if (ch == NULL)
	return 0;

    if (ch->desc != NULL && ch->desc->original != NULL)
	ch = ch->desc->original;

    if (ch->trust)
	return ch->trust;

    if (IS_NPC(ch) && ch->level >= LEVEL_HERO)
	return LEVEL_HERO - 1;
    else
	return ch->level;
}


/*
 * Retrieve a character's age.
 */
int get_age(CHAR_DATA *ch)
{
    return 17 + (ch->played + (current_time - ch->logon)) / 72000;
}

int get_lycanthrope(CHAR_DATA *ch, int stat)
{
    int add = 0;

    static int add_stats[MAX_STATS][4] =
    {
    /*SW  SB  FW  FB*/
    { 0,  2,  0,  4}, /* STR   */
    {-1, -1, -2, -2}, /* INT  */
    {-3, -3, -6, -6}, /*  WIS   */
    { 2,  0,  4,  0}, /* DEX    */
    { 1,  1,  2,  2} /* CON     */
    };


    if (is_affected(ch,gsn_spirit_wolf))
	add = add_stats[stat][0];

    if (is_affected(ch,gsn_spirit_bear))
	add = add_stats[stat][1];

    if (is_affected(ch,gsn_form_wolf))
	add = add_stats[stat][2];

    if (is_affected(ch,gsn_form_bear))
	add = add_stats[stat][3];

    return add;
}

int get_max_stat(CHAR_DATA *ch, int stat)
{
    int max;

    if (IS_NPC(ch) || ch->level > LEVEL_IMMORTAL)
	max = MAX_STAT;
    else
    {
	max = pc_race_table[ch->race].max_stats[stat] + 4;

	if (class_table[ch->classid].attr_prime == stat)
	    max += 1;

	if (class_table[ch->classid].attr_secondary == stat)
	    max += 1;

	max = UMIN(max+get_lycanthrope(ch, stat), MAX_STAT);
    }

    return max;
}

/* command for retrieving stats */
int get_curr_stat(CHAR_DATA *ch, int stat)
{
    return URANGE(3, ch->perm_stat[stat] + get_lycanthrope(ch, stat) 
	+ UMIN(4, ch->mod_stat[stat]), get_max_stat(ch, stat));
}

/* command for returning max training score */
int get_max_train(CHAR_DATA *ch, int stat)
{
    int max;

    if (IS_NPC(ch) || ch->level > LEVEL_IMMORTAL)
	return MAX_STAT;

    max = pc_race_table[ch->race].max_stats[stat];
    if (class_table[ch->classid].attr_prime == stat)
    {
	/*	if (ch->race == race_lookup("human"))
	 max += 3;
	 else */
	max += 1;
    }

    if (class_table[ch->classid].attr_secondary == stat)
	max += 1;


    return UMIN(max, MAX_STAT);
}


/*
 * Retrieve a character's carry capacity.
 */
int can_carry_n(CHAR_DATA *ch)
{
    if (!IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL)
	return 1000;

    if (IS_NPC(ch) && IS_SET(ch->act, ACT_PET))
	return 0;

    return MAX_WEAR + (6 * get_curr_stat(ch, STAT_STR) + 7 * get_curr_stat(ch, STAT_DEX) + 8 * ch->level) / 10;
}



/*
 * Retrieve a character's carry capacity.
 */
int can_carry_w(CHAR_DATA *ch)
{
    if (!IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL)
	return 999990;

    if (IS_NPC(ch) && IS_SET(ch->act, ACT_PET))
	return 1;

    return str_app[get_curr_stat(ch, STAT_STR)].carry * 10 + ch->level * 25;
}



/*
 * See if a string is one of the names of an object.
 */

bool is_name(char *str, char *namelist)
{
    char name[MAX_INPUT_LENGTH], part[MAX_INPUT_LENGTH];
    char *list, *string;

    /* fix crash on NULL namelist */
    if (namelist == NULL || namelist[0] == '\0')
	return FALSE;

    /* fixed to prevent is_name on "" returning TRUE */
    if (str[0] == '\0')
	return FALSE;

    string = str;
    /* we need ALL parts of string to match part of namelist */
    for (; ;)  /* start parsing string */
    {
	str = one_argument(str, part);

	if (part[0] == '\0')
	    return TRUE;

	/* check to see if this is part of namelist */
	list = namelist;
	for (; ;)  /* start parsing namelist */
	{
	    list = one_argument(list, name);
	    if (name[0] == '\0')  /* this name was not found */
		return FALSE;

	    if (!str_prefix(string, name))
		return TRUE; /* full pattern match */

	    if (!str_prefix(part, name))
		break;
	}
    }
}

bool is_exact_name(char *str, char *namelist)
{
    char name[MAX_INPUT_LENGTH];

    if (namelist == NULL)
	return FALSE;

    for (; ;)
    {
	namelist = one_argument(namelist, name);
	if (name[0] == '\0')
	    return FALSE;
	if (!str_cmp(str, name))
	    return TRUE;
    }
}

void affect_copy(OBJ_DATA *obj, AFFECT_DATA *paf)
{
    AFFECT_DATA *af_new = new_affect();

    af_new->next = obj->affected;
    af_new->prev = NULL;
    if (obj->affected)
	obj->affected->prev = af_new;
    obj->affected = af_new;

    af_new->where	= paf->where;
    af_new->type        = UMAX(0, paf->type);
    af_new->level       = paf->level;
    af_new->duration    = paf->duration;
    af_new->location    = paf->location;
    af_new->modifier    = paf->modifier;
    af_new->bitvector   = paf->bitvector;
}

/* enchanted stuff for eq */
void affect_enchant(OBJ_DATA *obj)
{
    /* okay, move all the old flags into new vectors if we have to */
    if (!obj->enchanted && !obj->pIndexData->edited)
    {
	AFFECT_DATA *paf;
	obj->enchanted = TRUE;

	for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
	    affect_copy(obj, paf);

	REMOVE_BIT(obj->extra_flags, ITEM_HAS_SOCKET);
    }
}

bool drop_obj_to_room(CHAR_DATA *ch, OBJ_DATA *wield, bool check_wield)
{
    if (IS_OBJ_STAT(wield, ITEM_NOREMOVE) && wield->wear_loc != WEAR_NONE)
	return TRUE;

    obj_from_char(wield, check_wield);

    if (IS_OBJ_STAT(wield, ITEM_NODROP))
    {
	obj_to_char(wield, ch);
	return FALSE;
    }

    act("Ты бросаешь $p6.", ch, wield, NULL, TO_CHAR);
    act("$n бросает $p6.", ch, wield, NULL, TO_ROOM);

    if (IS_OBJ_STAT(wield, ITEM_MELT_DROP))
    {
	act("$p исчезает в облачке дыма.", ch, wield, NULL, TO_ALL);
	extract_obj(wield, TRUE, FALSE);
    }
    else
    {
	obj_to_room(wield, ch->in_room);
    }

    return FALSE;
}

/*
 * Apply or remove an affect to a character.
 */
void affect_modify(CHAR_DATA *ch, AFFECT_DATA *paf, bool fAdd, bool check_wield)
{
    int mod, i;

    if (paf->where == TO_RESIST && paf->bitvector >= DAM_MAX)
    {
	bugf("Affect_modify: Invalid bitvector %llu in resistance affect.",
	     paf->bitvector);
	return;
    }

    mod = paf->modifier;

    if (fAdd)
    {
	switch (paf->where)
	{
	case TO_AFFECTS:
	    if (paf->bitvector != AFF_BLIND || ch->classid != CLASS_NAZGUL)
	        SET_BIT(ch->affected_by, paf->bitvector);
	    break;
	case TO_RESIST:
	    ch->resists[paf->bitvector] += paf->modifier;
	    break;
	case TO_PLR:
	    SET_BIT(ch->act, paf->bitvector);
	}
    }
    else
    {
	switch (paf->where)
	{
	case TO_AFFECTS:
	    REMOVE_BIT(ch->affected_by, paf->bitvector);
	    break;
	case TO_RESIST:
	    ch->resists[paf->bitvector] -= paf->modifier;
	    break;
	case TO_PLR:
	    REMOVE_BIT(ch->act, paf->bitvector);
	}
	mod = 0 - mod;
    }

    if (paf->where != TO_RESIST)
    {
	switch (paf->location)
	{
	default:
	    bugf("Affect_modify: unknown location %d, name %s.",
		 paf->location, ch->name);
	    return;

	case APPLY_NONE:
	    break;
	case APPLY_STR:
	    ch->mod_stat[STAT_STR] += mod;
	    break;
	case APPLY_DEX:
	    ch->mod_stat[STAT_DEX] += mod;
	    break;
	case APPLY_INT:
	    ch->mod_stat[STAT_INT] += mod;
	    break;
	case APPLY_WIS:
	    ch->mod_stat[STAT_WIS] += mod;
	    break;
	case APPLY_CON:
	    ch->mod_stat[STAT_CON] += mod;
	    break;
	case APPLY_SEX:
	    ch->sex += mod;
	    break;
	case APPLY_MANA:
	    ch->max_mana += mod;
	    break;
	case APPLY_HIT:
	    ch->max_hit += mod;
	    break;
	case APPLY_MOVE:
	    ch->max_move += mod;
	    break;
	case APPLY_AC:
	    for (i = 0; i < 4; i ++)
		ch->armor[i] += mod;
	    break;
	case APPLY_HITROLL:
	    ch->hitroll += mod;
	    break;
	case APPLY_DAMROLL:
	    ch->damroll += mod;
	    break;
	case APPLY_SAVES:
	    ch->saving_throw += mod;
	    break;
	case APPLY_SAVING_ROD:
	    ch->saving_throw += mod;
	    break;
	case APPLY_SAVING_PETRI:
	    ch->saving_throw += mod;
	    break;
	case APPLY_SAVING_BREATH:
	    ch->saving_throw += mod;
	    break;
	case APPLY_SAVING_SPELL:
	    ch->saving_throw += mod;
	    break;
	case APPLY_SPELL_AFFECT:
	    break;
	case APPLY_SKILL:
	    if (!IS_NPC(ch) && paf->type > 0 && paf->type < max_skills)
		ch->pcdata->skill_mod[paf->type] += mod;
	    break;
	}
    }

    /*
     * Check for weapon wielding.
     * Guard against recursion (for weapons with affects).
     */

    if (check_wield && !IS_NPC(ch))
    {
	OBJ_DATA *wield, *obj_next;
	bool ok = FALSE;

	while (!ok)
	{
	    ok = TRUE;

	    if ((wield = get_eq_char(ch, WEAR_WIELD)) != NULL
		&& !check_wield_weight(ch, wield, FALSE))
	    {
		ok = drop_obj_to_room(ch, wield, FALSE);
	    }

	    if ((wield = get_eq_char(ch, WEAR_SECONDARY)) != NULL
		&& !check_wield_weight(ch, wield, TRUE))
	    {
		ok = (drop_obj_to_room(ch, wield, FALSE) && ok);
	    }

	    for (wield = ch->carrying; wield; wield = obj_next)
	    {
		obj_next = wield->next_content;

		if (wield->wear_loc != WEAR_NONE && check_wear_stat(ch, wield) < MAX_STATS)
		    ok = (drop_obj_to_room(ch, wield, FALSE) && ok);
	    }
	}
    }

    return;
}


/* find an effect in an affect list */
AFFECT_DATA  *affect_find(AFFECT_DATA *paf, int sn)
{
    AFFECT_DATA *paf_find;

    for (paf_find = paf; paf_find != NULL; paf_find = paf_find->next)
    {
	if (paf_find->type == sn && paf->bitvector != AFF_GODS_SKILL)
	    return paf_find;
    }

    return NULL;
}

/* fix object affects when removing one */
void affect_check(CHAR_DATA *ch, int where, int vector)
{
    AFFECT_DATA *paf;
    OBJ_DATA *obj;

    if (where == TO_OBJECT || where == TO_WEAPON || vector == 0)
	return;

    for (paf = ch->affected; paf != NULL; paf = paf->next)
	if (paf->where == where
	    && paf->bitvector == vector
	    && where == TO_AFFECTS)
	{
	    SET_BIT(ch->affected_by, vector);
	}

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
	if (obj->wear_loc == -1)
	    continue;

	for (paf = obj->affected; paf != NULL; paf = paf->next)
	    if (paf->where == where
		&& paf->bitvector == vector
		&& where == TO_AFFECTS)
	    {
		SET_BIT(ch->affected_by, vector);
		return;
	    }

	if (obj->enchanted || obj->pIndexData->edited)
	    continue;

	for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
	    if (paf->where == where
		&& paf->bitvector == vector
		&& where == TO_AFFECTS)
	    {
		SET_BIT(ch->affected_by, vector);
		return;
	    }
    }
}

long get_id(CHAR_DATA *ch)
{
    if (ch)
    {
	ch = get_master(ch);
	return ch->id;
    }

    return 0;
}
/*
 * Give an affect to a char.
 */
 /*PAVLOV криво как-то вешается caster_id */
void affect_to_char(CHAR_DATA *ch, AFFECT_DATA *paf)
{
    AFFECT_DATA *paf_new;

    if (paf->type < 1 || paf->type>= max_skills)
    {
	bugf("Affect_to_char: paf->type is out of bounds.");
	return;
    }

/*
    if (paf->caster_id == 0){
	paf->caster_id = get_id(ch);
	bugf("Skill %d, id: %d", paf->type, paf->caster_id);
    }
*/
    paf_new = new_affect();

    *paf_new = *paf;

    VALIDATE(paf_new);	/* in case we missed it when we set up paf */
//    bugf("Skill %d, id: %d", paf_new->type, paf_new->caster_id);

    paf_new->next	= ch->affected;
    paf_new->prev	= NULL;
    if (ch->affected){
    	ch->affected->prev = paf_new;
//        bugf("Skill %d, id: %d", ch->affected->prev->type, ch->affected->prev->caster_id);
    }
    
    ch->affected	= paf_new;
//    bugf("Skill %d, id: %d", ch->affected->type, ch->affected->caster_id);

    affect_modify(ch, paf_new, TRUE, TRUE);

    return;
}

/* give an affect to an object */
void affect_to_obj(OBJ_DATA *obj, AFFECT_DATA *paf)
{
    AFFECT_DATA *paf_new;

    paf_new = new_affect();

    *paf_new		= *paf;

    VALIDATE(paf_new);	/* in case we missed it when we set up paf */
    paf_new->next	= obj->affected;
    paf_new->prev	= NULL;
    if (obj->affected)
	obj->affected->prev = paf_new;
    obj->affected	= paf_new;

    /* apply any affect vectors to the object's extra_flags */
    if (paf->bitvector)
	switch (paf->where)
	{
	case TO_OBJECT:
	    SET_BIT(obj->extra_flags, paf->bitvector);
	    break;
	case TO_WEAPON:
	    if (obj->item_type == ITEM_WEAPON)
		SET_BIT(obj->value[4], paf->bitvector);
	    break;
	}


    return;
}

/*
 * Remove an affect from a char.
 */
void affect_remove(CHAR_DATA *ch, AFFECT_DATA *paf)
{
    int where;
    int vector;

    if (ch->affected == NULL)
    {
	bugf("Affect_remove: no affect.");
	return;
    }

    affect_modify(ch, paf, FALSE, TRUE);
    where = paf->where;
    vector = paf->bitvector;

    if (paf->prev)
	paf->prev->next = paf->next;

    if (paf->next)
	paf->next->prev = paf->prev;

    if (ch->affected == paf)
	ch->affected = paf->next;

    free_affect(paf);

    affect_check(ch, where, vector);
    return;
}

void affect_remove_obj(OBJ_DATA *obj, AFFECT_DATA *paf)
{
    int where, vector;

    if (obj->affected == NULL)
    {
	bugf("Affect_remove_object: no affect.");
	return;
    }

    if (obj->carried_by != NULL && obj->wear_loc != -1)
	affect_modify(obj->carried_by, paf, FALSE, TRUE);

    where = paf->where;
    vector = paf->bitvector;

    /* remove flags from the object if needed */
    if (paf->bitvector)
	switch(paf->where)
	{
	case TO_OBJECT:
	    REMOVE_BIT(obj->extra_flags, paf->bitvector);
	    break;
	case TO_WEAPON:
	    if (obj->item_type == ITEM_WEAPON)
		REMOVE_BIT(obj->value[4], paf->bitvector);
	    break;
	}

    if (paf->prev)
	paf->prev->next = paf->next;

    if (paf->next)
	paf->next->prev = paf->prev;

    if (obj->affected == paf)
	obj->affected = paf->next;

    free_affect(paf);

    if (obj->carried_by != NULL && obj->wear_loc != -1)
	affect_check(obj->carried_by, where, vector);
    return;
}


/*
 * Strip all affects of a given sn.
 */
void affect_strip(CHAR_DATA *ch, int sn)
{
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;

    for (paf = ch->affected; paf != NULL; paf = paf_next)
    {
	paf_next = paf->next;
	if (paf->type == sn && paf->bitvector != AFF_GODS_SKILL)
	    affect_remove(ch, paf);
    }

    return;
}

AFFECT_DATA *get_char_affect(CHAR_DATA *ch, int sn)
{
    AFFECT_DATA *paf;

    for (paf = ch->affected; paf != NULL; paf = paf->next)
	if (paf->type == sn && paf->bitvector != AFF_GODS_SKILL)
	    return paf;

    return NULL;
}

AFFECT_DATA *get_obj_affect(OBJ_DATA *obj, int sn)
{
    AFFECT_DATA *paf;

    for (paf = obj->affected; paf != NULL; paf = paf->next)
	if (paf->type == sn)
	    return paf;

    return NULL;
}

AFFECT_DATA *get_room_affect(ROOM_INDEX_DATA *room, int sn)
{
    AFFECT_DATA *paf;

    for (paf = room->affected; paf != NULL; paf = paf->next)
	if (paf->type == sn)
	    return paf;

    return NULL;
}

bool is_lycanthrope(CHAR_DATA *ch)
{
    if (is_affected(ch,gsn_form_wolf)
	|| is_affected(ch,gsn_form_bear))
	return TRUE;

    return FALSE;
}

bool is_lycanthrope_spirit(CHAR_DATA *ch)
{
    if (is_affected(ch,gsn_spirit_wolf)
	|| is_affected(ch,gsn_spirit_bear))
	return TRUE;

    return FALSE;
}

/*
 * Return true if a char is affected by a spell.
 */
bool is_affected(CHAR_DATA *ch, int sn)
{
    AFFECT_DATA *paf;

    if (!ch)
	return FALSE;

    for (paf = ch->affected; paf != NULL; paf = paf->next)
    {
	if (paf->type == sn && paf->bitvector != AFF_GODS_SKILL)
	    return TRUE;
    }

    return FALSE;
}

/*
 * Add or enhance an affect.
 */
void affect_join(CHAR_DATA *ch, AFFECT_DATA *paf)
{
    AFFECT_DATA *paf_old;

    for (paf_old = ch->affected; paf_old != NULL; paf_old = paf_old->next)
    {
	if (paf_old->type == paf->type
	    && paf_old->bitvector != AFF_GODS_SKILL
	    && paf_old->location == paf->location
	    && paf_old->bitvector == paf->bitvector)
	{
	    paf->level = (paf->level + paf_old->level) / 2;
	    paf->duration += paf_old->duration;
	    paf->modifier += paf_old->modifier;
	    affect_remove(ch, paf_old);
	    break;
	}
    }
    affect_to_char(ch, paf);
    return;
}

void affect_join_obj(OBJ_DATA *obj, AFFECT_DATA *paf)
{
    AFFECT_DATA *paf_old;

    for (paf_old = obj->affected; paf_old != NULL; paf_old = paf_old->next)
    {
	if (paf_old->type == paf->type
	    && paf_old->bitvector != AFF_GODS_SKILL
	    && paf_old->location == paf->location
	    && paf_old->bitvector == paf->bitvector)
	{
	    paf->level = (paf->level += paf_old->level) / 2;
	    paf->duration += paf_old->duration;
	    paf->modifier += paf_old->modifier;
	    affect_remove_obj(obj, paf_old);
	    break;
	}
    }

    affect_to_obj(obj, paf);
    return;
}

void affect_join_room(ROOM_INDEX_DATA *room, AFFECT_DATA *paf)
{
    AFFECT_DATA *paf_old;

    for (paf_old = room->affected; paf_old != NULL; paf_old = paf_old->next)
    {
	if (paf_old->type == paf->type
	    && paf_old->bitvector != AFF_GODS_SKILL
	    && paf_old->location == paf->location
	    && paf_old->bitvector == paf->bitvector)
	{
	    paf->level = (paf->level += paf_old->level) / 2;
	    paf->duration += paf_old->duration;
	    paf->modifier += paf_old->modifier;
	    affect_remove_room(room, paf_old);
	    break;
	}
    }

    affect_to_room(room, paf);
    return;
}

void affect_replace(CHAR_DATA *ch, AFFECT_DATA *paf)
{
    AFFECT_DATA *paf_old;

    for (paf_old = ch->affected; paf_old != NULL; paf_old = paf_old->next)
    {
	if (paf_old->type == paf->type
	    && paf_old->bitvector != AFF_GODS_SKILL
	    && paf_old->location == paf->location
	    && paf_old->bitvector == paf->bitvector)
	{
	    affect_remove(ch, paf_old);
	    break;
	}
    }

    affect_to_char(ch, paf);
    return;
}

void affect_replace_obj(OBJ_DATA *obj, AFFECT_DATA *paf)
{
    AFFECT_DATA *paf_old;

    for (paf_old = obj->affected; paf_old != NULL; paf_old = paf_old->next)
    {
	if (paf_old->type == paf->type
	    && paf_old->bitvector != AFF_GODS_SKILL
	    && paf_old->location == paf->location
	    && paf_old->bitvector == paf->bitvector)
	{
	    affect_remove_obj(obj, paf_old);
	    break;
	}
    }

    affect_to_obj(obj, paf);
    return;
}

void affect_replace_room(ROOM_INDEX_DATA *room, AFFECT_DATA *paf)
{
    AFFECT_DATA *paf_old;

    for (paf_old = room->affected; paf_old != NULL; paf_old = paf_old->next)
    {
	if (paf_old->type == paf->type
	    && paf_old->bitvector != AFF_GODS_SKILL
	    && paf_old->location == paf->location
	    && paf_old->bitvector == paf->bitvector)
	{
	    affect_remove_room(room, paf_old);
	    break;
	}
    }

    affect_to_room(room, paf);
    return;
}


/*
 * Move a char out of a room.
 */
void char_from_room(CHAR_DATA *ch)
{
    OBJ_DATA *obj;

    if (ch->in_room == NULL)
	return;

    if (!IS_NPC(ch))
	--ch->in_room->area->nplayer;

    if ((obj = get_eq_char(ch, WEAR_LIGHT)) != NULL
	&& obj->item_type == ITEM_LIGHT
	&& obj->value[2] != 0
	/* && ch->in_room->light > 0 */)
    {
	--ch->in_room->light;
    }

    LIST_REMOVE(ch, room_link);

    ch->in_room      = NULL;
    ch->on 	     = NULL;  /* sanity check! */
    return;
}

OBJ_DATA *get_trap(ROOM_INDEX_DATA *room)
{
    OBJ_DATA *obj;

    for (obj = room->contents; obj != NULL; obj = obj->next_content)
	if (obj->item_type == ITEM_TRAP && obj->value[2] > 0)
	    break;

    return obj;
}

void trap_damage(CHAR_DATA *ch, OBJ_DATA *trap)
{
    int dam = (trap->level + trap->value[2]) / 2;

    dam += dice(trap->value[0], trap->value[1]);

    if (!IS_NPC(ch) && ch->level <= PK_RANGE && dam > ch->hit)
	dam = ch->hit;

    WAIT_STATE(ch, trap->value[4]);
    damage(ch, ch, dam, TYPE_UNDEFINED, trap->value[3], FALSE, NULL);
}

bool check_trap(CHAR_DATA *ch)
{
    OBJ_DATA *obj;


    /* Смертельная ловушка */
    if ((IS_SET(ch->in_room->room_flags, ROOM_KILL)
	 || (IS_SET(ch->in_room->room_flags, ROOM_NOFLY_DT)
	     && !IS_AFFECTED(ch, AFF_FLYING)))
	&& !IS_IMMORTAL(ch))
    {
	CHAR_DATA *orig = NULL;
	char buf[MSL];

	send_to_char("\n\r{RТЫ УМИРАЕШЬ!!!{x\n\r\n\r", ch);

	if (IS_SET(ch->in_room->room_flags, ROOM_NOFLY_DT))
	    act("$n падает вниз и умирает! К сожалению, ты никак не сможешь "
		"достать $m вещи.", ch, NULL, NULL, TO_ROOM);

	sprintf(buf, "%s умирает в ловушке в %s [room %d]",
		(IS_NPC(ch) ? ch->short_descr : ch->name),
		ch->in_room->name, ch->in_room->vnum);

	convert_dollars(buf);
	if (IS_NPC(ch))
	    wiznet(buf, NULL, NULL, WIZ_MOBDEATHS, 0, 0);
	else
	    wiznet(buf, NULL, NULL, WIZ_DEATHS, 0, 0);

	if (MOUNTED(ch))
	    raw_kill(MOUNTED(ch), NULL, FALSE);

	if (IS_SWITCHED(ch))
	    orig = ch->desc->original;

	raw_kill(ch, NULL, FALSE);

	if (orig)
	    char_to_room(orig, get_recall(orig), TRUE);

//	ch->silver = 0;
//	ch->gold = 0;

	REMOVE_BIT(ch->act, PLR_KILLER);
	REMOVE_BIT(ch->act, PLR_THIEF);

//	VALIDATE(ch);
	save_char_obj(ch, FALSE);

	/*      do_function(ch, &do_look, "auto"); */
	return TRUE;
    }

    if ((obj = get_trap(ch->in_room)) != NULL)
    {
/*	int avg_lev = (obj->level + obj->value[2])/2; */
	CHAR_DATA *victim = MOUNTED(ch) ? MOUNTED(ch) : ch;;

	victim->in_trap = FALSE;

//	if (ch->level >= avg_lev + PK_RANGE)
	{
	if (((IS_IN_WILD(ch) && (ch->classid == CLASS_RANGER || ch->classid == CLASS_DRUID)) || (IS_IN_TOWN(ch) && ch->classid == CLASS_THIEF))
	    && get_skill(ch, gsn_detect_trap) >= number_percent() 
	    && can_see_obj(ch, obj))
	    {
	        send_to_char("Ты замечаешь установленную ловушку!\n\r", ch);
	        check_improve(ch, NULL, gsn_detect_trap, TRUE, 5);
	    }
	    else
	    {
	        act("Ты попадаешь в $p6!", victim, obj, NULL, TO_CHAR);
	        act("$n попадает в $p6!", victim, obj, NULL, TO_ROOM);
	        check_improve(ch, NULL, gsn_detect_trap, FALSE, 5);
	        trap_damage(victim, obj);
	        extract_obj(obj, TRUE, FALSE);
	        victim->in_trap = TRUE;
	    }
	}

	return (victim == NULL || !IS_VALID(victim));
    }

    return (ch == NULL || !IS_VALID(ch));
}

bool check_trap_obj(CHAR_DATA *ch, OBJ_DATA *obj)
{
    int avg_lev;

    ch->in_trap = FALSE;

    if (obj->trap)
    {
	avg_lev = (obj->trap->level + obj->trap->value[2]) / 2;

	if (get_skill(ch, gsn_detect_trap) <= number_percent()
	    && ch->level < avg_lev + PK_RANGE)
	{
	    act("Ты задеваешь незаметный рычажок, и $p срабатывает!",
		ch, obj->trap, NULL, TO_CHAR);
	    act("$n задевает незаметный рычажок, и $p срабатывает!",
		ch, obj->trap, NULL, TO_ROOM);
	    check_improve(ch, NULL, gsn_detect_trap, FALSE, 5);
	    trap_damage(ch, obj->trap);

	    obj->trap->in_obj = NULL;
	    extract_obj(obj->trap, TRUE, FALSE);
	    obj->trap = NULL;
	    obj->trap_flags = 0;
	    ch->in_trap = TRUE;
	}
	else
	{
	    send_to_char("Ты замечаешь установленную ловушку!\n\r", ch);
	    check_improve(ch, NULL, gsn_detect_trap, TRUE, 5);
	}
    }

    return (ch == NULL);
}

bool check_trap_exit(CHAR_DATA *ch, EXIT_DATA *exit)
{
    int avg_lev;

    ch->in_trap = FALSE;

    if (exit->trap)
    {
	avg_lev = (exit->trap->level + exit->trap->value[2]) / 2;

	if (get_skill(ch, gsn_detect_trap) <= number_percent()
	    && ch->level < avg_lev + PK_RANGE)
	{
	    act("Ты задеваешь незаметный рычажок, и $p срабатывает!",
		ch, exit->trap, NULL, TO_CHAR);
	    act("$n задевает незаметный рычажок, и $p срабатывает!",
		ch, exit->trap, NULL, TO_ROOM);
	    check_improve(ch, NULL, gsn_detect_trap, FALSE, 5);
	    trap_damage(ch, exit->trap);

	    exit->trap->in_room = NULL;
	    extract_obj(exit->trap, TRUE, FALSE);
	    exit->trap = NULL;
	    exit->trap_flags = 0;
	    ch->in_trap = TRUE;
	}
	else
	{
	    send_to_char("Ты замечаешь установленную ловушку!\n\r", ch);
	    check_improve(ch, NULL, gsn_detect_trap, TRUE, 5);
	}
    }

    return (ch == NULL);
}

bool is_memory(CHAR_DATA *ch, CHAR_DATA *victim, int type)
{
    MEM_DATA *mem;

    if (!ch)
	return FALSE;

    if (!IS_NPC(ch) || (victim && IS_NPC(victim)))
	return FALSE;

    for (mem = ch->memory; mem; mem = mem->next)
	if ((!victim || mem->id == victim->id) && (type == 0 || type == mem->reaction))
	    return TRUE;

    return FALSE;
}

void mob_remember(CHAR_DATA *ch, CHAR_DATA *victim, int type)
{
    MEM_DATA *mem;

    if (!victim->memory)
    {
	mem = new_mem_data();
	mem->next = NULL;
	victim->memory = mem;
    }
    else
    {
	mem = new_mem_data();
	mem->next = victim->memory;
	victim->memory = mem;
    }

    mem->id = ch->id;
    mem->reaction = type;
    mem->when = current_time;
}

void mob_forget(CHAR_DATA *ch, CHAR_DATA *victim, int type)
{
    MEM_DATA *mem, *mem_next, *mem_prev = NULL;

    for (mem = ch->memory; mem; mem = mem_next)
    {
	mem_next = mem->next;

	if ((!victim || mem->id == victim->id) && (type == 0 || type == mem->reaction))
	{
	    if (ch->memory == mem)
	    {
		ch->memory = mem_next;
		free_mem_data(mem);
	    }
	    else
	    {
		mem_prev->next = mem_next;
		free_mem_data(mem);
	    }
	}
	else
	{
	    mem_prev = mem;
	}
    }
}

bool can_cleave(CHAR_DATA *ch, CHAR_DATA *victim)
{
    OBJ_DATA *prim;

    if (get_skill(ch, gsn_cleave) == 0)
	return FALSE;

    if (ch->fighting != NULL)
	return FALSE;

    if (is_safe(ch, victim))
	return FALSE;

    if (victim->hit < 5 * victim->max_hit / 6)
	return FALSE;

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
	return FALSE;

    if ((prim = get_eq_char (ch, WEAR_WIELD)) == NULL)
	return FALSE;

    if (!IS_WEAPON_STAT(prim, WEAPON_TWO_HANDS)
	|| get_eq_char (ch, WEAR_SECONDARY) != NULL
	|| get_eq_char (ch, WEAR_SHIELD) != NULL
	|| get_eq_char (ch, WEAR_HOLD) != NULL)
    {
	return FALSE;
    }

    if (prim->value[0] != WEAPON_SWORD
	&& prim->value[0] != WEAPON_AXE
	&& prim->value[0] != WEAPON_POLEARM)
    {
	return FALSE;
    }

    return TRUE;
}

bool is_visited(CHAR_DATA *ch, int vnum)
{
    return IS_NPC(ch) ? FALSE : IS_SET(ch->pcdata->visited[(vnum / 8)], 1 << (vnum % 8));
}


void set_visited(CHAR_DATA *ch, int vnum)
{
    int byte = vnum / 8;
    int bit  = vnum % 8;

    if (IS_NPC(ch))
	return;

    if (ch->pcdata->visited_size < byte + 1)
    {
	// Increase bit size
	void *newbuf;

	newbuf = alloc_mem(byte + 1);
	bzero(newbuf, byte + 1);

	if (ch->pcdata->visited_size > 0)
	{
	    memcpy(newbuf, ch->pcdata->visited, ch->pcdata->visited_size);
	    free_mem(ch->pcdata->visited, ch->pcdata->visited_size);
	}
	ch->pcdata->visited = newbuf;
	ch->pcdata->visited_size = byte + 1;
    }
    SET_BIT(ch->pcdata->visited[byte], 1 << bit);
}

/*
 * Move a char into a room.
 */
void char_to_room(CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex, bool show)
{
    OBJ_DATA *obj;
    AFFECT_DATA *af;
    CHAR_DATA *gch;
    AFFECT_DATA *paf;


    if (pRoomIndex == NULL)
    {
	ROOM_INDEX_DATA *room;

	bugf("Char_to_room: location is NULL.");

	if ((room = get_room_index(ROOM_VNUM_TEMPLE)) != NULL)
	    char_to_room(ch, room, show);

	return;
    }

    if ((is_affected(ch, gsn_gods_curse) || ch->count_holy_attacks > 0 || ch->count_guild_attacks > 0)
     && (IS_SET(pRoomIndex->room_flags, ROOM_SAFE)
      || IS_SET(pRoomIndex->room_flags, ROOM_PRIVATE)
      || IS_SET(pRoomIndex->room_flags, ROOM_SOLITARY)))
    {
        ROOM_INDEX_DATA *room; 

        while ((room = get_random_room(ch)) == NULL || !can_see_room(ch, room))
   	    ;
	    
        char_to_room(ch, room, show);
        return;
    }

    if (MOUNTED(ch))
    {
	LIST_FOREACH(gch, &pRoomIndex->people, room_link)
	{
	    if (gch == MOUNTED(ch))
		break;
	}

	if (gch == NULL)
	    do_dismount(ch, "update");
    }

    ch->in_room		= pRoomIndex;
    LIST_INSERT_HEAD(&pRoomIndex->people, ch, room_link);

    set_visited(ch, pRoomIndex->vnum);

    if (!IS_NPC(ch))
    {
	if (ch->in_room->area->empty)
	{
	    ch->in_room->area->empty = FALSE;
	    ch->in_room->area->age = 0;
	}
	++ch->in_room->area->nplayer;
    }

    if ((obj = get_eq_char(ch, WEAR_LIGHT)) != NULL
	&& obj->item_type == ITEM_LIGHT
	&& obj->value[2] != 0)
    {
	++ch->in_room->light;
    }

    if (show)
    {
	if (IS_SET (ch->in_room->room_flags, ROOM_NOMAGIC)
	&& !is_affected(ch,gsn_protection_sphere))
	{
	    int i;

	    for (i = 0; i < 4; i++)
		spell_dispel_magic(0, 2 * MAX_LEVEL, ch, ch, TARGET_CHAR);
	}

	if ((paf = affect_find(ch->in_room->affected, gsn_cursed_lands)))
	    spell_curse(gsn_cursed_lands, paf->level, ch, ch, TARGET_CHAR);

	if (IS_AFFECTED(ch, AFF_PLAGUE))
	{
	    AFFECT_DATA plague;
	    CHAR_DATA *vch, *safe_vch;

	    if ((af = affect_find(ch->affected, gsn_plague)) != NULL && af->level > 1)
	    {

		plague.where		= TO_AFFECTS;
		plague.type 		= gsn_plague;
		plague.level 		= af->level - 1;
		plague.duration 	= number_range(1, 2 * plague.level);
		plague.location	= APPLY_STR;
		plague.modifier 	= -5;
		plague.bitvector 	= AFF_PLAGUE;
		plague.caster_id 	= 0;

		LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, safe_vch)
		{
		    if (!saves_spell(plague.level - 2, vch, DAM_DISEASE)
			&& !IS_IMMORTAL(vch)
			&& !IS_AFFECTED(vch, AFF_PLAGUE)
			&& (!IS_NPC(vch) || !IS_SET(vch->act, ACT_UNDEAD))
			&& !IS_VAMPIRE(vch)
			&& vch->race != RACE_ZOMBIE
			&& number_bits(6) == 0)
		    {
			send_to_char("Ты чувствуешь жар и лихорадку.\n\r", vch);
			act("$n трясется и выглядит очень больным.",
			    vch, NULL, NULL, TO_ROOM);

			affect_join(vch, &plague);

		    }
		}
	    }
	}
	do_function(ch, &do_look, "auto");
    }

    return;
}



/*
 * Give an obj to a char.
 * If top is TRUE - obj added to the head of the list,
 * else it added after vobj or (if vobj == NULL) to the tail of the list
 */
void obj_to_char_gen(OBJ_DATA *obj, CHAR_DATA *ch, bool top, OBJ_DATA *vobj)
{
    int temp_obj;
	
    if (ch == NULL || obj == NULL)
	return;

    temp_obj = obj->id;

    if (obj->item_type == ITEM_MONEY)
    {
	ch->silver += obj->value[0];
	ch->gold += obj->value[1];
	extract_obj(obj, TRUE, FALSE);
	return;
    }


    if (top || !ch->carrying)
    {
	obj->next_content	 = ch->carrying;
	obj->prev_content	 = NULL;
	if (ch->carrying)
	    ch->carrying->prev_content = obj;
	ch->carrying		 = obj;
    }
    else
    {
	if (!vobj)
	{
	    for (vobj = ch->carrying; vobj->next_content; vobj = vobj->next_content)
		; /* nothing to do, just find last object */
	}

	obj->next_content = vobj->next_content;
	obj->prev_content = vobj;
	if (vobj->next_content)
	    vobj->next_content->prev_content = obj;
	vobj->next_content = obj;
    }

    obj->carried_by	 = ch;
    obj->in_room	 = NULL;
    obj->in_obj		 = NULL;
    ch->carry_number	+= get_obj_number(obj);
    ch->carry_weight	+= get_obj_weight(obj);

    if (!IS_NPC(ch) && obj->timer == 0)
    {
        if (obj->pIndexData 
        && IS_SET(obj->extra_flags, ITEM_HAD_TIMER) 
        && obj->pIndexData->timer > 0)
        {
            obj->timer = obj->pIndexData->timer;
            REMOVE_BIT(obj->extra_flags, ITEM_HAD_TIMER);
        }
        else if (obj->item_type == ITEM_ROOM_KEY || obj->item_type == ITEM_KEY)
	    obj->timer = number_range(10, 20);
    }	

    if (top)
    {
	if (temp_obj != ch->carrying->id)
	    bugf("Не совпадает выданная шмотка с тем, что таскает чар/моб. Вещь: %s Чар/моб %s", obj->name, ch->name);
    }
    return;
}



/*
 * Take an obj from its character.
 */
void obj_from_char(OBJ_DATA *obj, bool check_wield)
{
    CHAR_DATA *ch;

    if ((ch = obj->carried_by) == NULL)
    {
	bugf("Obj_from_char: null ch.");
	return;
    }

    if (obj->wear_loc != WEAR_NONE)
	unequip_char(ch, obj, check_wield);

    if (obj->prev_content)
	obj->prev_content->next_content = obj->next_content;

    if (obj->next_content)
	obj->next_content->prev_content = obj->prev_content;

    if (ch->carrying == obj)
	ch->carrying = obj->next_content;

    obj->carried_by	 = NULL;
    obj->next_content	 = NULL;
    obj->prev_content    = NULL;
    ch->carry_number	-= get_obj_number(obj);
    ch->carry_weight	-= get_obj_weight(obj);
    return;
}



/*
 * Find the ac value of an obj, including position effect.
 */
int apply_ac(OBJ_DATA *obj, int iWear, int type)
{
    if (obj->item_type != ITEM_ARMOR || type < 0)
	return 0;

    switch (iWear)
    {
    case WEAR_BODY:		return 3 * obj->value[type];
    case WEAR_HEAD:		return 2 * obj->value[type];
    case WEAR_LEGS:		return 2 * obj->value[type];
    case WEAR_FEET:		return     obj->value[type];
    case WEAR_HANDS:		return     obj->value[type];
    case WEAR_ARMS:		return     obj->value[type];
    case WEAR_SHIELD:		return 2 * obj->value[type];
    case WEAR_NECK_1:		return     obj->value[type]/2;
    case WEAR_NECK_2:		return     obj->value[type]/2;
    case WEAR_ABOUT:		return 2 * obj->value[type];
    case WEAR_WAIST:		return     obj->value[type];
    case WEAR_WRIST_L:	return     obj->value[type]/2;
    case WEAR_WRIST_R:	return     obj->value[type]/2;
    case WEAR_HOLD:		return     obj->value[type]/5;
    case WEAR_FINGER_L:	return     obj->value[type]/4;
    case WEAR_FINGER_R:	return     obj->value[type]/4;
    }

    return 0;
}

/*
 * Find a piece of eq on a character.
 */
OBJ_DATA *get_eq_char(CHAR_DATA *ch, int iWear)
{
    OBJ_DATA *obj;

    if (ch == NULL)
	return NULL;

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
	if (obj->wear_loc == iWear)
	    return obj;

    return NULL;
}

bool check_obj_sex(CHAR_DATA *ch, OBJ_DATA *obj)
{
    if ((IS_SET(obj->uncomf, FOR_WOMAN) && ch->sex == SEX_FEMALE)
	|| (IS_SET(obj->uncomf, FOR_MAN) && ch->sex == SEX_MALE))
    {
	return TRUE;
    }

    return FALSE;
}

bool is_uncomf(CHAR_DATA *ch, OBJ_DATA *obj)
{
    if ((IS_SET(obj->uncomf, FOR_EVIL) && IS_EVIL(ch))
	|| (IS_SET(obj->uncomf, FOR_GOOD) && IS_GOOD(ch))
	|| (IS_SET(obj->uncomf, FOR_NEUTRAL) && IS_NEUTRAL(ch))
	|| (IS_SET(obj->uncomf, FOR_MAN) && ch->sex == SEX_MALE)
	|| (IS_SET(obj->uncomf, FOR_WOMAN) && ch->sex == SEX_FEMALE)
	|| (!IS_NPC(ch)
	    && ((IS_SET(obj->uncomf, FOR_MAGE)              && ch->classid == CLASS_MAGE)
            || (IS_SET(obj->uncomf, FOR_CLERIC)         && ch->classid == CLASS_CLERIC)
            || (IS_SET(obj->uncomf, FOR_THIEF)          && ch->classid == CLASS_THIEF)
            || (IS_SET(obj->uncomf, FOR_WARRIOR)        && ch->classid == CLASS_WARRIOR)
            || (IS_SET(obj->uncomf, FOR_NECROMANCER)    && ch->classid == CLASS_NECROMANT)
            || (IS_SET(obj->uncomf, FOR_PALADIN)        && ch->classid == CLASS_PALADIN)
            || (IS_SET(obj->uncomf, FOR_NAZGUL)         && ch->classid == CLASS_NAZGUL)
            || (IS_SET(obj->uncomf, FOR_DRUID)          && ch->classid == CLASS_DRUID)
            || (IS_SET(obj->uncomf, FOR_RANGER)         && ch->classid == CLASS_RANGER)
            || (IS_SET(obj->uncomf, FOR_ALCHEMIST)      && (ch->classid == CLASS_ALCHEMIST)
            || (IS_SET(obj->uncomf, FOR_LYCANTHROPE)    && ch->classid == CLASS_LYCANTHROPE)
            || (IS_SET(obj->uncomf, FOR_VAMPIRE)        && ch->classid == CLASS_VAMPIRE)
            || (IS_SET(obj->uncomf, FOR_HUMAN)          && ch->race == RACE_HUMAN)
            || (IS_SET(obj->uncomf, FOR_ELF)            && ch->race == RACE_ELF)
            || (IS_SET(obj->uncomf, FOR_DWARF)          && ch->race == RACE_DWARF)
            || (IS_SET(obj->uncomf, FOR_GIANT)          && ch->race == RACE_GIANT)
            || (IS_SET(obj->uncomf, FOR_ORC)            && ch->race == RACE_ORC)
            || (IS_SET(obj->uncomf, FOR_TROLL)          && ch->race == RACE_TROLL)
            || (IS_SET(obj->uncomf, FOR_SNAKE)          && ch->race == RACE_SNAKE)
            || (IS_SET(obj->uncomf, FOR_HOBBIT)         && ch->race == RACE_HOBBIT)
            || (IS_SET(obj->uncomf, FOR_LYCANTHROPE)    && ch->race == RACE_LYCANTHROPE)
            || (IS_SET(obj->uncomf, FOR_DROW)           && ch->race == RACE_DROW)
            || (IS_SET(obj->uncomf, FOR_ZOMBIE)         && ch->race == RACE_ZOMBIE))
			)
		)){
		return TRUE;
	}

    return FALSE;
}


bool is_unusable(CHAR_DATA *ch, OBJ_DATA *obj)
{
    if ((IS_SET(obj->unusable, FOR_EVIL) && IS_EVIL(ch))
	|| (IS_SET(obj->unusable, FOR_GOOD) && IS_GOOD(ch))
	|| (IS_SET(obj->unusable, FOR_NEUTRAL) && IS_NEUTRAL(ch))
	|| (IS_SET(obj->unusable, FOR_MAN) && ch->sex == SEX_MALE)
	|| (IS_SET(obj->unusable, FOR_WOMAN) && ch->sex == SEX_FEMALE)
	|| (!IS_NPC(ch)
	    && (
            (IS_SET(obj->unusable, FOR_MAGE)            && ch->classid == CLASS_MAGE)
            || (IS_SET(obj->unusable, FOR_CLERIC)       && ch->classid == CLASS_CLERIC)
            || (IS_SET(obj->unusable, FOR_THIEF)        && ch->classid == CLASS_THIEF)
            || (IS_SET(obj->unusable, FOR_WARRIOR)      && ch->classid == CLASS_WARRIOR)
            || (IS_SET(obj->unusable, FOR_NECROMANCER)  && ch->classid == CLASS_NECROMANT)
            || (IS_SET(obj->unusable, FOR_PALADIN)      && ch->classid == CLASS_PALADIN)
            || (IS_SET(obj->unusable, FOR_NAZGUL)       && ch->classid == CLASS_NAZGUL)
            || (IS_SET(obj->unusable, FOR_DRUID)        && ch->classid == CLASS_DRUID)
            || (IS_SET(obj->unusable, FOR_RANGER)       && ch->classid == CLASS_RANGER)
            || (IS_SET(obj->unusable, FOR_ALCHEMIST)    && ch->classid == CLASS_ALCHEMIST)
            || (IS_SET(obj->unusable, FOR_LYCANTHROPE)  && ch->classid == CLASS_LYCANTHROPE)
            || (IS_SET(obj->unusable, FOR_VAMPIRE)      && ch->classid == CLASS_VAMPIRE)
            || (IS_SET(obj->unusable, FOR_HUMAN)        && ch->race == RACE_HUMAN)
            || (IS_SET(obj->unusable, FOR_ELF)          && ch->race == RACE_ELF)
            || (IS_SET(obj->unusable, FOR_DWARF)        && ch->race == RACE_DWARF)
            || (IS_SET(obj->unusable, FOR_GIANT)        && ch->race == RACE_GIANT)
            || (IS_SET(obj->unusable, FOR_ORC)  	    && ch->race == RACE_ORC)
            || (IS_SET(obj->unusable, FOR_TROLL)        && ch->race == RACE_TROLL)
            || (IS_SET(obj->unusable, FOR_SNAKE)        && ch->race == RACE_SNAKE)
            || (IS_SET(obj->unusable, FOR_HOBBIT)	    && ch->race == RACE_HOBBIT)
            || (IS_SET(obj->unusable, FOR_LYCANTHROPE)  && ch->race == RACE_LYCANTHROPE)
            || (IS_SET(obj->unusable, FOR_DROW) 	    && ch->race == RACE_DROW)
            || (IS_SET(obj->uncomf, FOR_ZOMBIE) 	    && ch->race == RACE_ZOMBIE)
            || (IS_OBJ_STAT(obj, ITEM_BLESS)    	    && (ch->race == RACE_VAMPIRE || ch->race == RACE_ZOMBIE) )
            )
        )
		    || (is_nopk(ch) && is_limit(obj) != -1))
		    return TRUE;
    else
	return FALSE;               
}

bool drop_bad_obj(CHAR_DATA *ch, OBJ_DATA *obj)
{
    if (is_unusable(ch, obj)
    && (obj->wear_loc <= WEAR_NONE || !IS_OBJ_STAT(obj, ITEM_NOREMOVE)))
    {
	act("Ты с отвращением отбрасываешь $p6.", ch, obj, NULL, TO_CHAR);
	act("$n с отвращением отбрасывает от себя $p6.",
	    ch, obj, NULL, TO_ROOM);

	drop_obj_to_room(ch, obj, TRUE);

	return TRUE;
    }

    return FALSE;
}


/*
 * Equip a char with an obj.
 */
void equip_char(CHAR_DATA *ch, OBJ_DATA *obj, int iWear)
{
    AFFECT_DATA *paf;
    int i;
    char bfr[MAX_STRING_LENGTH];

    if (get_eq_char(ch, iWear) != NULL)
    {
	bugf("Equip_char: mob %s already equipped (%d).", ch->name, iWear);
	return;
    }

    if (drop_bad_obj(ch, obj))
	return;

    if (ch->desc && ch->desc->connected == CON_PLAYING)
    {
	if (check_obj_sex(ch, obj))
	{
	    send_to_char("Тебе не слишком удобно в таком одеянии.\n\r", ch);
	    act("$n чувствует себя не очень комфортно в таком одеянии.",
		ch, NULL, NULL, TO_ROOM);

	    sprintf(bfr, "Кажися, я выгляжу как идиот%s, кажися...",
		    ch->sex == SEX_FEMALE ? "ка" : "");
	    do_say(ch, bfr);
	}
	else if (is_uncomf(ch, obj))
	{
	    send_to_char("Тебе не слишком удобно в таком одеянии.\n\r", ch);
	    act("$n чувствует себя не очень удобно в таком одеянии.",
		ch, NULL, NULL, TO_ROOM);
	}
	else if ((IS_SET(obj->uncomf, FOR_WOMAN)
		  || IS_SET(obj->unusable, FOR_WOMAN))
		 && ch->sex == SEX_MALE)
	{
	    sprintf(bfr, "Во, вот %s как раз по мне.", obj->short_descr);
	    do_say(ch, bfr);
	}
	else if ((IS_SET(obj->uncomf, FOR_MAN)
		  || IS_SET(obj->unusable, FOR_MAN))
		 && ch->sex == SEX_FEMALE)
	{
	    sprintf(bfr, "Ой, вы посмотрите, какая замечательная вещичка - %s! "
		    "Давно такую искала!", obj->short_descr);
	    do_say(ch, bfr);
	}
    }

    for (i = 0; i < 4; i++)
	ch->armor[i]      	-= apply_ac(obj, iWear, i);
    obj->wear_loc	 = iWear;

    if (!obj->enchanted && !obj->pIndexData->edited)
	for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
	    if (paf->location != APPLY_SPELL_AFFECT)
		affect_modify(ch, paf, TRUE, TRUE);

    for (paf = obj->affected; paf != NULL; paf = paf->next)
	if (paf->location == APPLY_SPELL_AFFECT)
	    affect_to_char (ch, paf);
	else
	    affect_modify(ch, paf, TRUE, TRUE);

    if (obj->item_type == ITEM_LIGHT
	&& obj->value[2] != 0
	&& ch->in_room != NULL)
    {
	++ch->in_room->light;
    }

    if (ch->position <= POS_SITTING && (is_affected(ch, gsn_levitate) || IS_AFFECTED(ch, AFF_FLYING)))
	do_stand(ch, "");

    return;
}



/*
 * Unequip a char with an obj.
 */
void unequip_char(CHAR_DATA *ch, OBJ_DATA *obj, bool check_wield)
{
    AFFECT_DATA *paf = NULL;
    AFFECT_DATA *lpaf = NULL;
    AFFECT_DATA *lpaf_next = NULL;
    int i;

    if (obj->wear_loc == WEAR_NONE)
    {
	bugf("Unequip_char: already unequipped.");
	return;
    }

    if (obj->wear_loc == WEAR_ABOUT)
	REMOVE_BIT(ch->affected_by, AFF_COVER);

    for (i = 0; i < 4; i++)
	ch->armor[i]	+= apply_ac(obj, obj->wear_loc, i);
    obj->wear_loc	 = -1;

    if (!obj->enchanted && !obj->pIndexData->edited)
    {
	for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
	    if (paf->location == APPLY_SPELL_AFFECT)
	    {
		for (lpaf = ch->affected; lpaf != NULL; lpaf = lpaf_next)
		{
		    lpaf_next = lpaf->next;
		    if ((lpaf->type == paf->type) &&
			(lpaf->level == paf->level) &&
			(lpaf->location == APPLY_SPELL_AFFECT))
		    {
			affect_remove(ch, lpaf);
			lpaf_next = NULL;
		    }
		}
	    }
	    else
	    {
		affect_modify(ch, paf, FALSE, check_wield);
		affect_check(ch, paf->where, paf->bitvector);
	    }
    }
    for (paf = obj->affected; paf != NULL; paf = paf->next)
	if (paf->location == APPLY_SPELL_AFFECT)
	{
	    for (lpaf = ch->affected; lpaf != NULL; lpaf = lpaf_next)
	    {
		lpaf_next = lpaf->next;
		if ((lpaf->type == paf->type) &&
		    (lpaf->level == paf->level) &&
		    (lpaf->location == APPLY_SPELL_AFFECT))
		{
		    affect_remove(ch, lpaf);
		    lpaf_next = NULL;
		}
	    }
	}
	else
	{
	    affect_modify(ch, paf, FALSE, check_wield);
	    affect_check(ch, paf->where, paf->bitvector);
	}

    if (obj->item_type == ITEM_LIGHT
	&& obj->value[2] != 0
	&& ch->in_room != NULL
	/* && ch->in_room->light > 0 */)
    {
	--ch->in_room->light;
    }

    return;
}



/*
 * Count occurrences of an obj in a list.
 */
int count_obj_list(OBJ_INDEX_DATA *pObjIndex, OBJ_DATA *list)
{
    OBJ_DATA *obj;
    int nMatch;

    nMatch = 0;
    for (obj = list; obj != NULL; obj = obj->next_content)
    {
	if (obj->pIndexData == pObjIndex)
	    nMatch++;
    }

    return nMatch;
}



/*
 * Move an obj out of a room.
 */
void obj_from_room(OBJ_DATA *obj)
{
    ROOM_INDEX_DATA *in_room;
    CHAR_DATA *ch;

    if ((in_room = obj->in_room) == NULL)
    {
	bugf("obj_from_room: NULL.");
	return;
    }

    LIST_FOREACH(ch, &in_room->people, room_link)
    {
	if (ch->on == obj)
	    ch->on = NULL;
    }

    if (obj->prev_content)
	obj->prev_content->next_content = obj->next_content;

    if (obj->next_content)
	obj->next_content->prev_content = obj->prev_content;

    if (obj == in_room->contents)
	in_room->contents = obj->next_content;

    if ( /* (obj->item_type == ITEM_LIGHT && obj->value[2] != 0) || */
	 obj->pIndexData->vnum == OBJ_VNUM_CAMPFIRE)
	--obj->in_room->light;

    obj->in_room      = NULL;
    obj->next_content = NULL;
    obj->prev_content = NULL;
    return;
}



/*
 * Move an obj into a room.
 */
void obj_to_room(OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex)
{
    if (obj != NULL && pRoomIndex != NULL)
    {
	obj->next_content	= pRoomIndex->contents;
	obj->prev_content	= NULL;
	if (pRoomIndex->contents)
	    pRoomIndex->contents->prev_content = obj;
	pRoomIndex->contents	= obj;
	obj->in_room		= pRoomIndex;
	obj->carried_by		= NULL;
	obj->in_obj		= NULL;

	if (/* (obj->item_type == ITEM_LIGHT && obj->value[2] != 0) || */
	    obj->pIndexData->vnum == OBJ_VNUM_CAMPFIRE)
	    ++pRoomIndex->light;

    }
    return;
}



/*
 * Move an object into an object.
 */
void obj_to_obj(OBJ_DATA *obj, OBJ_DATA *obj_to)
{
    obj->next_content		= obj_to->contains;
    obj->prev_content		= NULL;
    if (obj_to->contains)
	obj_to->contains->prev_content = obj;
    obj_to->contains		= obj;
    obj->in_obj			= obj_to;
    obj->in_room		= NULL;
    obj->carried_by		= NULL;

    if (obj_to != NULL && obj_to->pIndexData != NULL && obj_to->pIndexData->vnum == OBJ_VNUM_PIT)
	obj->cost = 0;

    for (; obj_to != NULL; obj_to = obj_to->in_obj)
    {
	if (obj_to->carried_by != NULL)
	{
	    /*	    obj_to->carried_by->carry_number += get_obj_number(obj);  */
	    obj_to->carried_by->carry_weight += get_obj_weight(obj)
		* WEIGHT_MULT(obj_to) / 100;
	}
    }

    return;
}



/*
 * Move an object out of an object.
 */
void obj_from_obj(OBJ_DATA *obj)
{
    OBJ_DATA *obj_from;

    if ((obj_from = obj->in_obj) == NULL)
    {
	bugf("Obj_from_obj: null obj_from.");
	log_string(obj->name);
	if (obj->carried_by != NULL) log_string(obj->carried_by->name);
	return;
    }

    if (obj->prev_content)
	obj->prev_content->next_content = obj->next_content;

    if (obj->next_content)
	obj->next_content->prev_content = obj->prev_content;

    if (obj == obj_from->contains)
	obj_from->contains = obj->next_content;

    obj->next_content = NULL;
    obj->prev_content = NULL;
    obj->in_obj       = NULL;

    for (; obj_from != NULL; obj_from = obj_from->in_obj)
    {
	if (obj_from->carried_by != NULL)
	{
	    /*	    obj_from->carried_by->carry_number -= get_obj_number(obj); */
	    obj_from->carried_by->carry_weight -= get_obj_weight(obj)
		* WEIGHT_MULT(obj_from) / 100;
	}
    }

    return;
}



/*
 * Extract an obj from the world.
 */
void extract_obj(OBJ_DATA *obj, bool limit, bool check_wield)
{
    OBJ_DATA *obj_content;
    OBJ_DATA *obj_next;
    int indx;
    CHAR_DATA *ch;

    /* На всякий случай. Как с мобами. */
    if (!IS_VALID(obj))
	return;

    if (obj->item_type == ITEM_CHEST && obj->in_room != NULL)
	delete_chest(obj->id);	/* Удаляем файл сундука */

    if (obj->in_room != NULL)
	obj_from_room(obj);
    else if (obj->carried_by != NULL)
	obj_from_char(obj, check_wield);
    else if (obj->in_obj != NULL)
	obj_from_obj(obj);


    for (obj_content = obj->contains; obj_content; obj_content = obj_next)
    {
	obj_next = obj_content->next_content;
	extract_obj(obj_content, limit, FALSE);
    }

    if (obj->prev)
	obj->prev->next = obj->next;

    if (obj->next)
	obj->next->prev = obj->prev;

    if (object_list == obj)
	object_list = obj->next;


    if (obj->pIndexData != NULL)
	--obj->pIndexData->count;

    if (limit && (indx = is_limit(obj)) != -1 && limits[indx + 1] > 0
	&& !IS_SET(obj->extra_flags, ITEM_INVENTORY))
    {
	limits[indx+1]--;
    }

    /* Beacon fix */
    LIST_FOREACH(ch, &char_list, link)
	if (!IS_NPC(ch) && ch->pcdata->beacon == obj)
	{
	    ch->pcdata->beacon = NULL;
	    break;
	}

    /* Auction fix */
    check_auctions(NULL, obj, "его исчезновения");

    free_obj(obj);
    return;
}



/*
 * Extract a char from the world.
 */
void extract_char(CHAR_DATA *ch, bool fPull)
{
    CHAR_DATA *wch;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    MEM_DATA *memory, *memory_next;

    if (!IS_VALID(ch))
	return;

    nuke_pets(ch);
    ch->pet = NULL; /* just in case */

    /*    if (fPull) */
    die_follower(ch);

    stop_fighting(ch, TRUE);

    for (memory = ch->memory; memory; memory = memory_next)
    {
	memory_next = memory->next;
	free_mem_data(memory);
    }


	if (!IS_NPC(ch) && (ch->pcdata->temp_RIP > 1))		   
	{
		ch->pcdata->temp_RIP = 0;
	}
	else
	{
	    for (obj = ch->carrying; obj != NULL; obj = obj_next)
    	{
			obj_next = obj->next_content;
			extract_obj(obj, TRUE, FALSE);
    	}
	}


    /* Death room is set in the clan tabe now */
    if (!fPull && !IS_NPC(ch))
    {
	ch->hit = 1;
	ch->position = POS_RESTING;
	update_pos(ch);
	
	if (ch->in_room != NULL)
	    char_from_room(ch);

	char_to_room(ch, get_recall(ch), TRUE);

	INVALIDATE(ch);
	return;
    }

    if (IS_NPC(ch))
	--ch->pIndexData->count;

    if (ch->mount && ch->mount->mount == ch)
    {
	ch->mount->mount = NULL;
	
	if (ch->mount->riding)
	{
	    act("Ты падаешь с $N1!", ch->mount, NULL, ch, TO_CHAR);
	    act("$n падает с $N1.", ch->mount, NULL, ch, TO_ROOM);
	    ch->mount->riding = FALSE;
	    if (!IS_IMMORTAL(ch->mount))
		ch->mount->position = POS_SITTING;
	}
    }

    if (ch->desc != NULL && ch->desc->original != NULL)
    {
	do_function(ch, &do_return, "");
	ch->desc = NULL;
    }

    if (ch->in_room != NULL)
	char_from_room(ch);

    LIST_FOREACH(wch, &char_list, link)
    {
	if (wch->reply == ch)
	    wch->reply = NULL;
	if (wch->mprog_target == ch)
	    wch->mprog_target = NULL;
	if (IS_AFFECTED(wch, AFF_CHARM) && wch->master == ch)
	{
	    wch->master = NULL;
	    wch->leader = NULL;
	    affect_strip(wch, gsn_charm_person);
	}
    }

    LIST_REMOVE(ch, link);

    if (!IS_NPC(ch) && ch->pcdata->beacon != NULL)
	extract_obj(ch->pcdata->beacon, FALSE, FALSE);

    if (ch->desc != NULL)
	ch->desc->character = NULL;

    free_char(ch);
    return;
}

bool too_many_victims; // Для определения, что выбрано слишком много жертв

CHAR_DATA *get_char_room(CHAR_DATA *ch, ROOM_INDEX_DATA *room, char *argument, bool check)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *rch, *safe_rch, *vch;
    int number;
    int count;

    too_many_victims = FALSE;

    if (isdigit(*argument))
	check = FALSE;

    number = number_argument(argument, arg);

    if (!str_cmp(arg, "self"))
	return ch;

    if (ch && room)
    {
	bugf("get_char_room received multiple types (ch/room)");
	return NULL;
    }

    if (ch)
    {
	if (ch->in_room == NULL)
	{
	    //	  bugf("get_char_room: ch->in_room is NULL");
	    return NULL;
	}
	room = ch->in_room;
    }
    else
    {
	if (room == NULL)
	{
	    //  	    bugf("get_char_room: room is NULL");
	    return NULL;
	}
    }

    /* Mob progs fix 
     if (ch && IS_NPC(ch) && mob_holylight)
     ch = NULL;     */

    count = 0;
    vch = NULL;

    LIST_FOREACH_SAFE(rch, &room->people, room_link, safe_rch)
    {
	if (!can_see(ch, rch) || !is_name(arg, rch->name))
	    continue;

	count++;

	if (check && vch)
	{
	    if (!IS_NPC(vch) || !IS_NPC(rch) ||
		(vch->pIndexData && rch->pIndexData && vch->pIndexData->vnum != rch->pIndexData->vnum))
	    {
		if (ch)
		    send_to_char("Слишком много целей. Определись конкретнее.\n\r", ch);

		too_many_victims = TRUE;
		return NULL;
	    }
	}

	if (count == number)
	{
	    if (!check)
		return rch;
	    else
		vch = rch;
	}
    }

    if (check && vch)
	return vch;

    if (convert_trans(ch, arg))
    {
	count = 0;
	vch = NULL;

	LIST_FOREACH_SAFE(rch, &room->people, room_link, safe_rch)
	{
	    if (!can_see(ch, rch) || !is_name(arg, rch->name))
		continue;

	    count++;
	    if (check && vch)
	    {
		if (!IS_NPC(vch) || !IS_NPC(rch) ||
		    (vch->pIndexData && rch->pIndexData && vch->pIndexData->vnum != rch->pIndexData->vnum))
		{
		    if (ch)
			send_to_char("Слишком много целей. Определись конкретнее.\n\r", ch);

		    too_many_victims = TRUE;
		    return NULL;
		}
	    }

	    if (count == number)
	    {
		if (!check)
		    return rch;
		else
		    vch = rch;
	    }
	}

	if (check && vch)
	    return vch;
    }

    return NULL;
}


/*
 * Find a char in the room.
 */

CHAR_DATA *get_char_world(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *wch, *rch = NULL;
    int number;
    int count, count1;

    if (ch && ch->in_room && (wch = get_char_room(ch, NULL, argument, FALSE)) != NULL)
	return wch;

    number = number_argument(argument, arg);
    count = count1 = 0;

    /* Mob progs fix 
     if (ch && IS_NPC(ch) && mob_holylight)
     ch = NULL;     */

    LIST_FOREACH(wch, &char_list, link)
    {
	if (wch->in_room != NULL && can_see(ch, wch))
	{
	    if (is_exact_name(arg, wch->name) && ++count == number)
		return wch;

	    if (is_name(arg, wch->name) && ++count1 == number )
		rch = wch;
	}
    }

    if (ch == NULL)
	return NULL;

    count = count1 = 0;

    if (!rch && convert_trans(ch, arg))
	LIST_FOREACH(wch, &char_list, link)
	{
	    if (wch->in_room != NULL && can_see(ch, wch))
	    {
		if (is_exact_name(arg, wch->name) && ++count == number)
		    return wch;

		if (is_name(arg, wch->name) && ++count1 == number )
		    rch = wch;
	    }
	}

    return rch;
}




/*
 * Find some object with a given index data.
 * Used by area-reset 'P' command.
 */
OBJ_DATA *get_obj_type(OBJ_INDEX_DATA *pObjIndex)
{
    OBJ_DATA *obj;

    for (obj = object_list; obj != NULL; obj = obj->next)
    {
	if (obj->pIndexData == pObjIndex)
	    return obj;
    }

    return NULL;
}


/*
 * Find an obj in a list.
 */
OBJ_DATA *get_obj_list(CHAR_DATA *ch, char *argument, OBJ_DATA *list)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument(argument, arg);

    if (count_global)
	number -= global_obj_count;

    count  = 0;

    for (obj = list; !END_OBJ_LIST(obj); obj = obj->next_content)
    {
	if (can_see_obj(ch, obj) && is_name(arg, obj->name))
	{
	    if (count_global)
		++global_obj_count;

	    if (++count == number)
		return obj;
	}
    }

    count  =  0;

    if (convert_trans(ch, arg))
	for (obj = list; !END_OBJ_LIST(obj); obj = obj->next_content)
	{
	    if (can_see_obj(ch, obj) && is_name(arg, obj->name))
	    {
		if (count_global)
		    ++global_obj_count;

		if (++count == number)
		    return obj;
	    }
	}


    return NULL;
}



/*
 * Find an obj in player's inventory.
 */
OBJ_DATA *get_obj_carry(CHAR_DATA *ch, char *argument, CHAR_DATA *viewer)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument(argument, arg);
    if (count_global)
	number -= global_obj_count;

    count  = 0;

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
	if (obj->wear_loc == WEAR_NONE
	    && (viewer ? can_see_obj(viewer, obj) : TRUE)
	    && is_name(arg, obj->name))
	{
	    if (count_global)
		++global_obj_count;

	    if (++count == number)
		return obj;
	}
    }

    count  = 0;

    if (convert_trans(ch, arg))
	for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
	{
	    if (obj->wear_loc == WEAR_NONE
		&& (viewer ? can_see_obj(viewer, obj) : TRUE)
		&& is_name(arg, obj->name))
	    {
		if (count_global)
		    ++global_obj_count;

		if (++count == number)
		    return obj;
	    }
	}


    return NULL;
}



/*
 * Find an obj in player's equipment.
 */

OBJ_DATA *get_obj_wear(CHAR_DATA *ch, char *argument, bool character)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument(argument, arg);
    if (count_global)
	number -= global_obj_count;

    count  = 0;

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
	if (obj->wear_loc != WEAR_NONE
	    && (character ? can_see_obj(ch, obj) : TRUE)
	    && is_name(arg, obj->name))
	{
	    if (count_global)
		++global_obj_count;

	    if (++count == number)
		return obj;
	}
    }

    count  = 0;

    if (convert_trans(ch, arg))
	for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
	{
	    if (obj->wear_loc != WEAR_NONE
		&&  (character ? can_see_obj(ch, obj) : TRUE)
		&&   is_name(arg, obj->name))
	    {
		if (count_global)
		    ++global_obj_count;

		if (++count == number)
		    return obj;
	    }
	}

    return NULL;
}



/*
 * Find an obj in the room or in inventory.
 */
OBJ_DATA *get_obj_here(CHAR_DATA *ch, ROOM_INDEX_DATA *room, char *argument)
{
    OBJ_DATA *obj;
    int number, count;
    char arg[MAX_INPUT_LENGTH];

    if (ch && room)
    {
	bugf("get_obj_here received a ch and a room");
	return NULL;
    }

    number = number_argument(argument, arg);
    count = 0;

    if (ch)
    {
	global_obj_count = 0;
	count_global = TRUE;
	if (ch->in_room)
	{
	    obj = get_obj_list(ch, argument, ch->in_room->contents);
	    if (obj != NULL)
	    {
		count_global = FALSE;
		return obj;
	    }	
	}
	if ((obj = get_obj_carry(ch, argument, ch)) != NULL)
	{
	    count_global = FALSE;
	    return obj;
	}

	if ((obj = get_obj_wear(ch, argument, TRUE)) != NULL)
	{
	    count_global = FALSE;
	    return obj;
	}
	count_global = FALSE;
    }
    else
    {
	if (room == NULL)
	{
	    bugf("get_obj_here(): room == NULL");
	    return NULL;
	}

	for (obj = room->contents; obj; obj = obj->next_content)
	{
	    if (!is_name(arg, obj->name))
		continue;
	    if (++count == number)
		return obj;
	}

	count  = 0;

	if (convert_trans(ch, arg))
	    for (obj = room->contents; obj; obj = obj->next_content)
	    {
		if (!is_name(arg, obj->name))
		    continue;
		if (++count == number)
		    return obj;
	    }
    }

    return NULL;
}




/*
 * Find an obj in the world.
 */
OBJ_DATA *get_obj_world(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    if (ch && (obj = get_obj_here(ch, NULL, argument)) != NULL)
	return obj;

    number = number_argument(argument, arg);
    count  = 0;

    for (obj = object_list; obj != NULL; obj = obj->next)
    {
	if ((ch && !can_see_obj(ch, obj))
	    || !is_name(arg, obj->name))
	{
	    continue;
	}
	if (++count == number)
	    return obj;
    }

    count  = 0;

    if (convert_trans(ch, arg))
	for (obj = object_list; obj != NULL; obj = obj->next)
	{
	    if ((ch && !can_see_obj(ch, obj))
		|| !is_name(arg, obj->name))
	    {
		continue;
	    }
	    if (++count == number)
		return obj;
	}

    return NULL;
}

/* deduct cost from a character */

void deduct_cost(CHAR_DATA *ch, int cost)
{
    int silver = 0, gold = 0;

    silver = UMIN(ch->silver, cost);

    if (silver < cost)
    {
	gold = ((cost - silver + 99) / 100);
	silver = cost - 100 * gold;
    }

    ch->gold -= gold;
    ch->silver -= silver;

    if (ch->gold < 0)
    {
	bugf("deduct costs: gold %d < 0 cost %d", ch->gold, cost);
	ch->gold = 0;
    }
    if (ch->silver < 0)
    {
	bugf("deduct costs: silver %d < 0 cost %d", ch->silver, cost);
	ch->silver = 0;
    }
}
/*
 * Create a 'money' obj.
 */
OBJ_DATA *create_money(int gold, int silver, int bronze)
{
    char buf[MSL], tmp[MSL];
    OBJ_DATA *obj;

    if (gold < 0 || silver < 0 || bronze < 0 || (gold == 0 && silver == 0 && bronze == 0))
    {
		gold = UMAX(1, gold);
		silver = UMAX(1, silver);
		bronze = UMAX(1, bronze);
    }

    if (gold == 0 && bronze == 0 && silver == 1)
    {
		obj = create_object(get_obj_index(OBJ_VNUM_SILVER_ONE), 0);
    }
    else if (gold == 1 && bronze == 0 && silver == 0)
    {
		obj = create_object(get_obj_index(OBJ_VNUM_GOLD_ONE), 0);
    }
	else if (gold == 0 && silver == 0 && bronze == 1)
	{
		obj = create_object(get_obj_index(OBJ_VNUM_BRONZE_ONE), 0);
		obj->value[2] = 1;
		obj->cost =  bronze;
		obj->weight		= get_money_weight(0, 0);
	}
    else if (silver == 0 && bronze == 0)
    {
		obj = create_object(get_obj_index(OBJ_VNUM_GOLD_SOME), 0);

		strcpy(tmp, hours(gold, TYPE_COINS));

		sprintf(buf, obj->short_descr, gold, hours(gold, TYPE_GOLD), tmp);
		free_string(obj->short_descr);
		obj->short_descr        = str_dup(buf);
		obj->value[1]           = gold;
		obj->cost               = gold;
		obj->weight		= get_money_weight(gold, 0);
    }
    else if (gold == 0 && bronze == 0)
    {
		obj = create_object(get_obj_index(OBJ_VNUM_SILVER_SOME), 0);

		strcpy(tmp, hours(silver, TYPE_COINS));

		sprintf(buf, obj->short_descr, silver, hours(silver, TYPE_SILVER), tmp);
		free_string(obj->short_descr);
		obj->short_descr        = str_dup(buf);
		obj->value[0]           = silver;
		obj->cost               = silver;
		obj->weight		= get_money_weight(0, silver);
    }
	else if (gold == 0 && silver == 0)
	{
		obj = create_object(get_obj_index(OBJ_VNUM_BRONZE_SOME), 0);

		strcpy(tmp, hours(bronze, TYPE_COINS));

		sprintf(buf, obj->short_descr, bronze, hours(bronze, TYPE_BRONZE), tmp);
		free_string(obj->short_descr);
		obj->short_descr        = str_dup(buf);
		obj->value[2]           = bronze;
		obj->cost               = bronze;
		obj->weight		= get_money_weight(0, 0);
	}
    else
    {
		char tmp1[MSL];

		obj = create_object(get_obj_index(OBJ_VNUM_COINS), 0);

		strcpy(tmp, hours(gold, TYPE_GOLD));
		strcpy(tmp1, hours(gold, TYPE_COINS));

		sprintf(buf, obj->short_descr, silver, hours(silver, TYPE_SILVER),
			gold, tmp, tmp1);

		free_string(obj->short_descr);
		obj->short_descr	= str_dup(buf);
		obj->value[0]		= silver;
		obj->value[1]		= gold;
		obj->cost		= 100 * gold + silver;
		obj->weight		= get_money_weight(gold, silver);
    }

    return obj;
}



/*
 * Return # of objects which an object counts as.
 * Thanks to Tony Chamberlain for the correct recursive code here.
 */
int get_obj_number(OBJ_DATA *obj)
{
    int number;

    /*    if (obj->item_type == ITEM_CONTAINER || obj->item_type == ITEM_MONEY
     ||  obj->item_type == ITEM_GEM || obj->item_type == ITEM_JEWELRY)
     number = 0;
     else */
    number = 1;

    /*    for (obj = obj->contains; obj != NULL; obj = obj->next_content)
     number += get_obj_number(obj); */

    return number;
}


/*
 * Return weight of an object, including weight of contents.
 */
int get_obj_weight(OBJ_DATA *obj)
{
    int weight;
    OBJ_DATA *tobj;

    if (!obj)
        return 0;

    weight = obj->weight;
    for (tobj = obj->contains; tobj != NULL; tobj = tobj->next_content)
	weight += get_obj_weight(tobj) * WEIGHT_MULT(obj) / 100;

    return weight;
}

int get_true_weight(OBJ_DATA *obj)
{
    int weight;
    OBJ_DATA *tobj;

    if (!obj)
        return 0;

    weight = obj->weight;
    for (tobj = obj->contains; tobj != NULL; tobj = tobj->next_content)
	weight += get_obj_weight(tobj);

    return weight;
}

/*
 * True if room is dark.
 */
bool room_is_dark(ROOM_INDEX_DATA *pRoomIndex)
{
    if (pRoomIndex == NULL) return TRUE;

    if (pRoomIndex->light > 0)
	return FALSE;

    if (IS_SET(pRoomIndex->room_flags, ROOM_DARK))
	return TRUE;

    if (pRoomIndex->sector_type == SECT_INSIDE
	||   pRoomIndex->sector_type == SECT_CITY)
	return FALSE;

    if (weather_info.sunlight == SUN_SET
	||   weather_info.sunlight == SUN_DARK)
	return TRUE;

    return FALSE;
}

bool room_is_dark_new(CHAR_DATA *ch)
{
    if (IS_VAMPIRE(ch))
	return FALSE;
    else
	return room_is_dark(ch->in_room);
}


bool is_room_owner(CHAR_DATA *ch, ROOM_INDEX_DATA *room)
{
	char buf[MAX_STRING_LENGTH];
    if (room->owner == NULL || room->owner[0] == '\0')
	return TRUE;

    if ( IS_IMMORTAL(ch))
	return TRUE;

    if (ch->desc != NULL && ch->desc->original != NULL)
	return FALSE;

	sprintf(buf, "%lu", ch->id);
    return is_name(buf, room->owner);

}

/*
 * True if room is private.
 */
bool room_is_private(ROOM_INDEX_DATA *pRoomIndex, CHAR_DATA *mount)
{
    CHAR_DATA *rch;
    int count;


    /*    if (pRoomIndex->owner != NULL && pRoomIndex->owner[0] != '\0')
     return TRUE;*/

    count = 0;
    LIST_FOREACH(rch, &pRoomIndex->people, room_link)
	count++;

    if (IS_SET(pRoomIndex->room_flags, ROOM_PRIVATE)  && (count >= 2 || (count == 1 && mount)))
	return TRUE;

    if (IS_SET(pRoomIndex->room_flags, ROOM_SOLITARY) && (count >= 1 || mount))
	return TRUE;

    if (pRoomIndex->min_level == MAX_LEVEL && pRoomIndex->max_level == MAX_LEVEL)
	return TRUE;

    return FALSE;
}

bool check_guild(CHAR_DATA *ch, ROOM_INDEX_DATA *to_room)
{
    if (!IS_NPC(ch) && !IS_IMMORTAL(ch))
    {
	bool can = TRUE, flag = FALSE;
	int iClass, iGuild;

	for (iClass = 0; iClass < MAX_CLASS; iClass++)
	{
	    for (iGuild = 0;
		 iGuild < MAX_PC_RACE
		 && class_table[iClass].guild[iGuild] != 0;
		 iGuild ++)
	    {
		if (to_room->vnum == class_table[iClass].guild[iGuild])
		{
		    if (iClass != ch->classid)
			can = FALSE;
		    else
		    {
			can = TRUE;
			flag = TRUE;
			break;
		    }
		}
	    }
	    if (flag)
		break;
	}

	if (!can)
	    return TRUE;
    }

    return FALSE;
}

/* visibility on a room -- for entering and exits */
bool can_see_room(CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex)
{
    if (!pRoomIndex || !ch)
	return FALSE;

    if (IS_SWITCHED(ch) && !can_see_room(ch->desc->original, pRoomIndex))
        return FALSE;

    if (pRoomIndex->min_level >= LEVEL_IMMORTAL && get_trust(ch) < pRoomIndex->min_level)	
	return FALSE;

    if (IS_IMMORTAL(ch))
	return TRUE;

    if (check_guild(ch, pRoomIndex))
        return FALSE;

    if (pRoomIndex->clan != 0 && is_clan(ch) != pRoomIndex->clan)
	return FALSE;

    if (IS_SET(pRoomIndex->area->area_flags, AREA_NA))
	return FALSE;

    if (pRoomIndex->min_level > ch->level || pRoomIndex->max_level < ch->level)
	return FALSE;

    return TRUE;
}



/*
 * True if char can see victim.
 */
bool can_see(CHAR_DATA *ch, CHAR_DATA *victim)
{
    /* RT changed so that WIZ_INVIS has levels */
    if (victim == NULL)
	return FALSE;

    if (ch == NULL)
    {
       if (victim->invis_level >= LEVEL_IMMORTAL && victim->invis_level == victim->level)
           return FALSE;
       else
           return TRUE;
    }	

    if (ch == victim)
	return TRUE;

    if (get_trust(ch) < victim->invis_level)
	return FALSE;

    if (get_trust(ch) < victim->incog_level && ch->in_room != victim->in_room)
	return FALSE;

    if (!IS_IMMORTAL(ch) && victim->desc == NULL && get_original(victim) != NULL)
	return FALSE;

    if ((!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT))
	||   (IS_NPC(ch) && mob_holylight))
	return TRUE;

    if (IS_WIZINVISMOB(victim))
	return FALSE;

    if (is_affected(victim, gsn_dissolve)
	&& victim->fighting == NULL
	&& victim->master != ch)
	return FALSE;

    if(is_affected(ch, gsn_prayer) && (victim->classid == CLASS_VAMPIRE || victim->classid == CLASS_NAZGUL))
	return TRUE;

    if (IS_AFFECTED(ch, AFF_BLIND))
    {
	return FALSE;
    }

    if (room_is_dark(ch->in_room) && !IS_AFFECTED(ch, AFF_INFRARED))
	return FALSE;

    if (IS_AFFECTED(victim, AFF_INVISIBLE)
	&&   !IS_AFFECTED(ch, AFF_DETECT_INVIS))
	return FALSE;

    if (IS_AFFECTED(victim, AFF_HIDE)
	&&   !IS_AFFECTED(ch, AFF_DETECT_HIDDEN)
	&&   victim->fighting == NULL)
	return FALSE;

    if (IS_AFFECTED(victim, AFF_CAMOUFLAGE)
	&&   IS_IN_WILD(victim)
	&&   !IS_AFFECTED(ch, AFF_DETECT_CAMOUFLAGE)
	&&   victim->fighting == NULL)
	return FALSE;

    return TRUE;
}



/*
 * True if char can see obj.
 */
bool can_see_obj(CHAR_DATA *ch, OBJ_DATA *obj)
{
    if (!ch)
	return TRUE;

    if (!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT))
	return TRUE;

    if (IS_SET(obj->extra_flags, ITEM_VIS_DEATH))
	return FALSE;

    if (IS_AFFECTED(ch, AFF_BLIND)
	&& obj->item_type != ITEM_POTION
	&& obj->item_type != ITEM_PILL)
    {
	return FALSE;
    }

    if (obj->item_type == ITEM_LIGHT && obj->value[2] != 0)
	return TRUE;

    if (IS_SET(obj->extra_flags, ITEM_INVIS)
	&& !IS_AFFECTED(ch, AFF_DETECT_INVIS))
    {
	return FALSE;
    }

    if (obj->pIndexData->vnum == OBJ_VNUM_SOUL && ch->classid != CLASS_NECROMANT)
	return FALSE;


    if (IS_OBJ_STAT(obj, ITEM_GLOW))
	return TRUE;

    if (room_is_dark(ch->in_room) && !IS_AFFECTED(ch, AFF_DARK_VISION))
	return FALSE;

    return TRUE;
}



/*
 * True if char can drop obj.
 */
bool can_drop_obj(CHAR_DATA *ch, OBJ_DATA *obj)
{
    if (!IS_SET(obj->extra_flags, ITEM_NODROP)
	&& !IS_SET(obj->extra_flags, ITEM_AUCTION)
	&& !IS_SET(obj->extra_flags, ITEM_INVENTORY))
    {
	return TRUE;
    }

    if (!IS_NPC(ch) && get_trust(ch) >= LEVEL_IMMORTAL)
	return TRUE;

    return FALSE;
}


/*
 * Config Colour stuff
 */

void default_colour( CHAR_DATA *ch )
{
    if( IS_NPC( ch ) )
	return;

    if( !ch->pcdata )
	return;

    ch->pcdata->text[1]		= ( WHITE );
    ch->pcdata->auction[1]	= ( YELLOW );
    ch->pcdata->auction_text[1]	= ( WHITE );
    ch->pcdata->gossip[1]	= ( MAGENTA );
    ch->pcdata->gossip_text[1]	= ( MAGENTA );
    ch->pcdata->music[1]	= ( RED );
    ch->pcdata->music_text[1]	= ( RED );
    ch->pcdata->question[1]	= ( YELLOW );
    ch->pcdata->question_text[1] = ( WHITE );
    ch->pcdata->answer[1]	= ( YELLOW );
    ch->pcdata->answer_text[1]	= ( WHITE );
    ch->pcdata->quote[1]	= ( GREEN );
    ch->pcdata->quote_text[1]	= ( GREEN );
    ch->pcdata->immtalk_text[1]	= ( CYAN );
    ch->pcdata->immtalk_type[1]	= ( YELLOW );
    ch->pcdata->info[1]		= ( YELLOW );
    ch->pcdata->tell[1]		= ( GREEN );
    ch->pcdata->tell_text[1]	= ( GREEN );
    ch->pcdata->say[1]		= ( GREEN );
    ch->pcdata->say_text[1]	= ( GREEN );
    ch->pcdata->reply[1]	= ( GREEN );
    ch->pcdata->reply_text[1]	= ( GREEN );
    ch->pcdata->gtell_text[1]	= ( GREEN );
    ch->pcdata->gtell_type[1]	= ( RED );
    ch->pcdata->wiznet[1]	= ( GREEN );
    ch->pcdata->room_title[1]	= ( CYAN );
    ch->pcdata->room_text[1]	= ( WHITE );
    ch->pcdata->room_exits[1]	= ( GREEN );
    ch->pcdata->room_things[1]	= ( CYAN );
    ch->pcdata->prompt[1]	= ( CYAN );
    ch->pcdata->fight_death[1]	= ( RED );
    ch->pcdata->fight_yhit[1]	= ( GREEN );
    ch->pcdata->fight_ohit[1]	= ( YELLOW );
    ch->pcdata->fight_thit[1]	= ( RED );
    ch->pcdata->fight_skill[1]	= ( WHITE );
    ch->pcdata->text[0]		= ( NORMAL_ );
    ch->pcdata->auction[0]	= ( BRIGHT );
    ch->pcdata->auction_text[0]	= ( BRIGHT );
    ch->pcdata->gossip[0]	= ( NORMAL_ );
    ch->pcdata->gossip_text[0]	= ( BRIGHT );
    ch->pcdata->music[0]	= ( NORMAL_ );
    ch->pcdata->music_text[0]	= ( BRIGHT );
    ch->pcdata->question[0]	= ( BRIGHT );
    ch->pcdata->question_text[0] = ( BRIGHT );
    ch->pcdata->answer[0]	= ( BRIGHT );
    ch->pcdata->answer_text[0]	= ( BRIGHT );
    ch->pcdata->quote[0]	= ( NORMAL_ );
    ch->pcdata->quote_text[0]	= ( BRIGHT );
    ch->pcdata->immtalk_text[0]	= ( NORMAL_ );
    ch->pcdata->immtalk_type[0]	= ( NORMAL_ );
    ch->pcdata->info[0]		= ( NORMAL_ );
    ch->pcdata->say[0]		= ( NORMAL_ );
    ch->pcdata->say_text[0]	= ( BRIGHT );
    ch->pcdata->tell[0]		= ( NORMAL_ );
    ch->pcdata->tell_text[0]	= ( BRIGHT );
    ch->pcdata->reply[0]	= ( NORMAL_ );
    ch->pcdata->reply_text[0]	= ( BRIGHT );
    ch->pcdata->gtell_text[0]	= ( NORMAL_ );
    ch->pcdata->gtell_type[0]	= ( NORMAL_ );
    ch->pcdata->wiznet[0]	= ( NORMAL_ );
    ch->pcdata->room_title[0]	= ( NORMAL_ );
    ch->pcdata->room_text[0]	= ( NORMAL_ );
    ch->pcdata->room_exits[0]	= ( NORMAL_ );
    ch->pcdata->room_things[0]	= ( NORMAL_ );
    ch->pcdata->prompt[0]	= ( NORMAL_ );
    ch->pcdata->fight_death[0]	= ( NORMAL_ );
    ch->pcdata->fight_yhit[0]	= ( NORMAL_ );
    ch->pcdata->fight_ohit[0]	= ( NORMAL_ );
    ch->pcdata->fight_thit[0]	= ( NORMAL_ );
    ch->pcdata->fight_skill[0]	= ( NORMAL_ );
    ch->pcdata->text[2]		= 0;
    ch->pcdata->auction[2]	= 0;
    ch->pcdata->auction_text[2]	= 0;
    ch->pcdata->gossip[2]	= 0;
    ch->pcdata->gossip_text[2]	= 0;
    ch->pcdata->music[2]	= 0;
    ch->pcdata->music_text[2]	= 0;
    ch->pcdata->question[2]	= 0;
    ch->pcdata->question_text[2] = 0;
    ch->pcdata->answer[2]	= 0;
    ch->pcdata->answer_text[2]	= 0;
    ch->pcdata->quote[2]	= 0;
    ch->pcdata->quote_text[2]	= 0;
    ch->pcdata->immtalk_text[2]	= 0;
    ch->pcdata->immtalk_type[2]	= 0;
    ch->pcdata->info[2]		= 1;
    ch->pcdata->say[2]		= 0;
    ch->pcdata->say_text[2]	= 0;
    ch->pcdata->tell[2]		= 0;
    ch->pcdata->tell_text[2]	= 0;
    ch->pcdata->reply[2]	= 0;
    ch->pcdata->reply_text[2]	= 0;
    ch->pcdata->gtell_text[2]	= 0;
    ch->pcdata->gtell_type[2]	= 0;
    ch->pcdata->wiznet[2]	= 0;
    ch->pcdata->room_title[2]	= 0;
    ch->pcdata->room_text[2]	= 0;
    ch->pcdata->room_exits[2]	= 0;
    ch->pcdata->room_things[2]	= 0;
    ch->pcdata->prompt[2]	= 0;
    ch->pcdata->fight_death[2]	= 0;
    ch->pcdata->fight_yhit[2]	= 0;
    ch->pcdata->fight_ohit[2]	= 0;
    ch->pcdata->fight_thit[2]	= 0;
    ch->pcdata->fight_skill[2]	= 0;

    return;
}

#if 0
void default_colour(CHAR_DATA *ch)
{
    int i;

    if (IS_NPC(ch))
	return;

    if (!ch->pcdata)
	return;

    for (i = 0; i < MAX_COLOUR; i++)
    {
	free_string(ch->pcdata->codes[i].code);
	ch->pcdata->codes[i].code = str_dup(colour_schema_default->colours[i].code);
	ch->pcdata->codes[i].level = colour_schema_default->colours[i].level;
    }

    return;
}
#endif
void all_colour(CHAR_DATA *ch, char *argument)
{
    char	buf[  100 ];
    char	buf2[ 100 ];
    int		colour;
    int		bright;

    if(IS_NPC(ch) || !ch->pcdata)
	return;

    if(!*argument)
	return;

#if 1
    if(!str_prefix(argument, "red"))
    {
	colour = (RED);
	bright = NORMAL_;
	sprintf(buf2, "Red");
    }
    if(!str_prefix(argument, "hi-red"))
    {
	colour = (RED);
	bright = BRIGHT;
	sprintf(buf2, "Red");
    }
    else if(!str_prefix(argument, "green"))
    {
	colour = (GREEN);
	bright = NORMAL_;
	sprintf(buf2, "Green");
    }
    else if(!str_prefix(argument, "hi-green"))
    {
	colour = (GREEN);
	bright = BRIGHT;
	sprintf(buf2, "Green");
    }
    else if(!str_prefix(argument, "yellow"))
    {
	colour = (YELLOW);
	bright = NORMAL_;
	sprintf(buf2, "Yellow");
    }
    else if(!str_prefix(argument, "hi-yellow"))
    {
	colour = (YELLOW);
	bright = BRIGHT;
	sprintf(buf2, "Yellow");
    }
    else if(!str_prefix(argument, "blue"))
    {
	colour = (BLUE);
	bright = NORMAL_;
	sprintf(buf2, "Blue");
    }
    else if(!str_prefix(argument, "hi-blue"))
    {
	colour = (BLUE);
	bright = BRIGHT;
	sprintf(buf2, "Blue");
    }
    else if(!str_prefix(argument, "magenta"))
    {
	colour = (MAGENTA);
	bright = NORMAL_;
	sprintf(buf2, "Magenta");
    }
    else if(!str_prefix(argument, "hi-magenta"))
    {
	colour = (MAGENTA);
	bright = BRIGHT;
	sprintf(buf2, "Magenta");
    }
    else if(!str_prefix(argument, "cyan"))
    {
	colour = (CYAN);
	bright = NORMAL_;
	sprintf(buf2, "Cyan");
    }
    else if(!str_prefix(argument, "hi-cyan"))
    {
	colour = (CYAN);
	bright = BRIGHT;
	sprintf(buf2, "Cyan");
    }
    else if(!str_prefix(argument, "white"))
    {
	colour = (WHITE);
	bright = NORMAL_;
	sprintf(buf2, "White");
    }
    else if(!str_prefix(argument, "hi-white"))
    {
	colour = (WHITE);
	bright = BRIGHT;
	sprintf(buf2, "White");
    }
    else if(!str_prefix(argument, "grey"))
    {
	colour = (BLACK);
	bright = BRIGHT;
	sprintf(buf2, "White");
    }
    else
    {
	send_to_char("Unrecognised colour, unchanged.\n\r", ch);
	return;
    }

    ch->pcdata->text[1]		= colour;
    ch->pcdata->auction[1]	= colour;
    ch->pcdata->gossip[1]	= colour;
    ch->pcdata->music[1]	= colour;
    ch->pcdata->question[1]	= colour;
    ch->pcdata->answer[1]	= colour;
    ch->pcdata->quote[1]	= colour;
    ch->pcdata->quote_text[1]	= colour;
    ch->pcdata->immtalk_text[1]	= colour;
    ch->pcdata->immtalk_type[1]	= colour;
    ch->pcdata->info[1]		= colour;
    ch->pcdata->say[1]		= colour;
    ch->pcdata->say_text[1]	= colour;
    ch->pcdata->tell[1]		= colour;
    ch->pcdata->tell_text[1]	= colour;
    ch->pcdata->reply[1]	= colour;
    ch->pcdata->reply_text[1]	= colour;
    ch->pcdata->gtell_text[1]	= colour;
    ch->pcdata->gtell_type[1]	= colour;
    ch->pcdata->wiznet[1]	= colour;
    ch->pcdata->room_title[1]	= colour;
    ch->pcdata->room_text[1]	= colour;
    ch->pcdata->room_exits[1]	= colour;
    ch->pcdata->room_things[1]	= colour;
    ch->pcdata->prompt[1]	= colour;
    ch->pcdata->fight_death[1]	= colour;
    ch->pcdata->fight_yhit[1]	= colour;
    ch->pcdata->fight_ohit[1]	= colour;
    ch->pcdata->fight_thit[1]	= colour;
    ch->pcdata->fight_skill[1]	= colour;
    ch->pcdata->text[0]		= bright;
    ch->pcdata->auction[0]	= bright;
    ch->pcdata->gossip[0]	= bright;
    ch->pcdata->music[0]	= bright;
    ch->pcdata->question[0]	= bright;
    ch->pcdata->answer[0]	= bright;
    ch->pcdata->quote[0]	= bright;
    ch->pcdata->quote_text[0]	= bright;
    ch->pcdata->immtalk_text[0]	= bright;
    ch->pcdata->immtalk_type[0]	= bright;
    ch->pcdata->info[0]		= bright;
    ch->pcdata->say[0]		= bright;
    ch->pcdata->say_text[0]	= bright;
    ch->pcdata->tell[0]		= bright;
    ch->pcdata->tell_text[0]	= bright;
    ch->pcdata->reply[0]	= bright;
    ch->pcdata->reply_text[0]	= bright;
    ch->pcdata->gtell_text[0]	= bright;
    ch->pcdata->gtell_type[0]	= bright;
    ch->pcdata->wiznet[0]	= bright;
    ch->pcdata->room_title[0]	= bright;
    ch->pcdata->room_text[0]	= bright;
    ch->pcdata->room_exits[0]	= bright;
    ch->pcdata->room_things[0]	= bright;
    ch->pcdata->prompt[0]	= bright;
    ch->pcdata->fight_death[0]	= bright;
    ch->pcdata->fight_yhit[0]	= bright;
    ch->pcdata->fight_ohit[0]	= bright;
    ch->pcdata->fight_thit[0]	= bright;
    ch->pcdata->fight_skill[0]	= bright;

    sprintf(buf, "Все цветовые параметры установлены в %s.\n\r", buf2);
    send_to_char(buf, ch);
#endif

    return;
}

void convert_name(char *argument)
{
    char *tm;

    if (*argument == 'я' && *(argument+1) == 'ь')
	argument += 2;

    for (tm = argument; *tm != '\0'; tm++)
	if (tm == argument)
	    *tm = UPPER(*tm);
	else
	    *tm = LOWER(*tm);
}

bool check_channels(CHAR_DATA *ch)
{
    if (ch->pcdata->nochan > 0 &&
	(IS_SET(ch->comm, COMM_NOCHANNELS) || IS_SET(ch->comm, COMM_NOSHOUT)
	 || IS_SET(ch->comm, COMM_NOTELL) || IS_SET(ch->comm, COMM_NOEMOTE)))
    {
	return TRUE;
    }

    return FALSE;
}

bool check_freeze(CHAR_DATA *ch)
{
    if (IS_SET(ch->act, PLR_FREEZE) && ch->pcdata->freeze > 0)
	return TRUE;

    return FALSE;
}

bool is_animal(CHAR_DATA *victim)
{
    if (victim == NULL || !IS_NPC(victim))
	return FALSE;

    if (IS_SET(victim->act, ACT_ANIMAL))
	return TRUE;

    switch (victim->race)
    {
    case 12:				/* bat		*/
    case 13:				/* bear		*/
    case 14:				/* cat		*/
    case 15:				/* centipede	*/
    case 16:				/* dog		*/
    case 19:				/* fido		*/
    case 20:				/* fox		*/
    case 23:				/* lizard	*/
    case 25:				/* pig		*/
    case 26:				/* rabbit	*/
    case 28:				/* song bird	*/
    case 29:				/* water fowl	*/
    case 30:				/* wolf		*/
    case 31:                            /* wyvern 	*/
    case 36:                            /* horse 	*/
    case 55:				/* bird		*/
    case 57:				/* griffon	*/
	return TRUE;
    }

    return FALSE;
}

struct
{
    char *word;
    char *end[3];
    int  type;
} hours_table[] =
{
    {"час", 		{"", 	"а", 	"ов"	}, 	TYPE_HOURS	},
    {"очк", 		{"о", 	"а", 	"ов"	}, 	TYPE_POINTS	},
    {"непрочитанн",	{"ое", 	"ых", 	"ых"	}, 	TYPE_UNREAD	},
    {"новост", 		{"ь", 	"и", 	"ей"	}, 	TYPE_NEWS	},
    {"изменени", 		{"е", 	"я", 	"й"	}, 	TYPE_CHANGES	},
    {"пис", 		{"ьмо", "ьма",	"ем", 	},	TYPE_NOTES	},
    {"иде", 		{"я", 	"и", 	"й"	}, 	TYPE_IDEAS	},
    {"посетил", 		{"а", 	"и", 	"и"	}, 	TYPE_VISIT	},
    {"наказани", 		{"е", 	"я", 	"й"	}, 	TYPE_PENALTY	},
    {"минут", 		{"а", 	"ы", 	""	}, 	TYPE_MINUTES	},
    {"минут", 		{"у", 	"ы", 	""	}, 	TYPE_MINUTE	},
    {"сут", 		{"ки", 	"ок", 	"ок"	}, 	TYPE_DAYS	},
    {"заряд", 		{"", 	"а", 	"ов"	}, 	TYPE_CHARGES	},
    {"практик", 		{"а", 	"и", 	""	}, 	TYPE_PRACTICE	},
    {"практик", 		{"у", 	"и", 	""	}, 	TYPE_PRACTICES	},
    {"трениров", 		{"ка", 	"ки", 	"ок"	}, 	TYPE_TRAIN	},
    {"", 			{"год",	"года", "лет"	}, 	TYPE_YEARS	},
    {"кус", 		{"ок", 	"ка", 	"ков"	}, 	TYPE_PIECE	},
    {"раунд",		{"",	"а",	"ов"	},	TYPE_ROUND	},
    {"так",		{"ой",	"их",	"их"	},	TYPE_LIKE_THIS	},
    {"предмет",		{"",	"а",	"ов"	},	TYPE_ITEM	},
    {"серебрян",		{"ая",	"ые",	"ых"	},	TYPE_SILVER	},
    {"золот",		{"ая",	"ые",	"ых"	},	TYPE_GOLD	},
    {"бронзов",		{"ая",	"ые",	"ых"	},	TYPE_BRONZE	},
    {"монет",		{"а",	"ы",	""	},	TYPE_COINS	},
    {"серебрян",		{"ую",	"ые",	"ых"	},	TYPE_SILVER1	},
    {"золот",		{"ую",	"ые",	"ых"	},	TYPE_GOLD1	},
    {"монет",		{"у",	"ы",	""	},	TYPE_COINS1	},
    {"помощник",		{"",	"а",	"ов"	},	TYPE_HELPERS	},
    {"ошиб",		{"ка",	"ки",	"ок"	},	TYPE_ERRORS	},
    {"комнат",		{"а",	"ы",	""	},	TYPE_ROOMS	},
    {"как",		{"ая-то","ие-то","их-то"},	TYPE_SOMETHING	},
    {"форм",		{"а",	"ы",	""	},	TYPE_FORMS	},
    {"опрос",		{"",	"а",	"ов"	},	TYPE_VOTES	},
    {"игрок",		{"",	"а",	"ов"	},	TYPE_PLAYERS	},
    {NULL,		{NULL,	NULL,	NULL	},	0		}
};

char *hours(unsigned int num, int flag)
{
    static char str[100];
    int i, j;

    for(i = 0; hours_table[i].word != NULL; i++)
	if (hours_table[i].type == flag)
	    break;

    if (!hours_table[i].word)
    {
	bugf("Hours: unknown flag %d", flag);
	return "";
    }

    for (; num >= 100; num -= 100)
	;

    if (num < 20 && num > 10)
    {
	j = 2;
	j = 2;
    }
    else
	switch (num % 10)
	{
	case 1:
	    j = 0;
	    break;
	case 2:
	case 3:
	case 4:
	    j = 1;
	    break;
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 0:
	    j = 2;
	    break;
	default:
	    j = 0;
	    break;
	}

    sprintf(str, "%s%s", hours_table[i].word, hours_table[i].end[j]);
    return str;
}

/*
 * Some funny stuff!!  8))
 * extra-флаги на вещах:
 * "aggravate monsters" - все мобы аггры к носителю шмотки.
 * "teleportation curse" - раз в тик телепортит носителя.
 * "slow digestion" - медленное пищеварение. Жрать и пить меньше надо, короче...
 *
 */
bool is_wear_aggravate(CHAR_DATA *ch)
{
    OBJ_DATA *obj;

    if (ch == NULL)
	return FALSE;

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
	if (obj->wear_loc != WEAR_NONE && IS_SET(obj->extra_flags, ITEM_AGGRAVATE))
	    return TRUE;

    return FALSE;
}

bool is_wear_teleport(CHAR_DATA *ch)
{
    OBJ_DATA *obj;

    if (ch == NULL)
	return FALSE;

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
	if (obj->wear_loc != WEAR_NONE && IS_SET(obj->extra_flags, ITEM_TELEPORT))
	    return TRUE;

    return FALSE;
}

bool is_wear_slow_digestion(CHAR_DATA *ch)
{
    OBJ_DATA *obj;

    if (ch == NULL)
	return FALSE;

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
	if (obj->wear_loc != WEAR_NONE && IS_SET(obj->extra_flags, ITEM_SLOW_DIGESTION))
	    return TRUE;

    return FALSE;
}

void teleport_curse(CHAR_DATA *ch)
{
    ROOM_INDEX_DATA *room;

    if (ch == NULL)
	return;

    if (ch->fighting != NULL)
	return;			/* Из боя так слинять не выйдет */

    if (ch->in_room != NULL && IS_SET(ch->in_room->room_flags, ROOM_NOGATE))
	return;

    room = get_random_room(ch);

    send_to_char("Тебя телепортировали!\n\r", ch);
    act("$n внезапно возникает из пустоты.", LIST_FIRST(&room->people), NULL, NULL, TO_ALL);

    char_from_room(ch);
    char_to_room(ch, room, TRUE);

    /*	do_function(ch, &do_look, "auto");*/
    check_trap(ch);
}

/* utility function */

TRACK_DATA *get_track(CHAR_DATA *ch, CHAR_DATA *victim)
{
    TRACK_DATA *tr;

    if (ch->in_room == NULL)
	return NULL;

    LIST_FOREACH(tr, &ch->in_room->tracks, link)
	if (victim == tr->ch || !strcmp(victim->name, tr->ch->name))
	    return tr;

    return NULL;
}

void add_track(CHAR_DATA *ch, int direction)
{
    TRACK_DATA *track;

    if (ch->in_room == NULL)
	return;

    track = (TRACK_DATA *)alloc_mem(sizeof(TRACK_DATA));
    track->ch = ch;
    track->ago = 0;
    track->direction = direction;
    LIST_INSERT_HEAD(&ch->in_room->tracks, track, link);
}

long free_space()
{

    struct statfs fs;

    if (statfs(".", &fs) != 0)
	return -1;

    return fs.f_bavail;

}

CHAR_DATA *get_master(CHAR_DATA *ch)
{
    CHAR_DATA *victim = ch;

    while (victim->master != NULL &&
	   (IS_AFFECTED(victim, AFF_CHARM) ||
	    (IS_NPC(victim) && victim->mount && victim->master == victim->mount)))
    {
	victim = victim->master;
    }

    return victim;
}

DESCRIPTOR_DATA *get_original(CHAR_DATA *ch)
{
    DESCRIPTOR_DATA *d;

    SLIST_FOREACH(d, &descriptor_list, link)
	if (d->original == ch)
	    return d;

    return NULL;
}

void affect_modify_room(ROOM_INDEX_DATA *room, AFFECT_DATA *paf, bool fAdd)
{
    int mod = paf->modifier;

    if (!fAdd)
	mod = 0 - mod;

    switch (paf->location)
    {
    case ROOM_APPLY_SECTOR:
	room->sector_type += mod;
	break;
    case ROOM_APPLY_FLAGS:
	room->room_flags ^= paf->bitvector;
	break;
    default:
	break;
    }
}

/* give an affect to room */
void affect_to_room(ROOM_INDEX_DATA *room, AFFECT_DATA *paf)
{
    AFFECT_DATA *paf_new;

    paf_new = new_affect();

    *paf_new		= *paf;

    VALIDATE(paf_new);	/* in case we missed it when we set up paf */
    paf_new->next	= room->affected;
    paf_new->prev	= NULL;
    if (room->affected)
	room->affected->prev = paf_new;
    room->affected	= paf_new;

    /* apply all room parameters */
    affect_modify_room(room, paf, TRUE);

    return;
}

void affect_remove_room(ROOM_INDEX_DATA *room, AFFECT_DATA *paf)
{
    if (room->affected == NULL)
    {
	bugf("Affect_remove_room: no affect.");
	return;
    }

    if (paf->prev)
	paf->prev->next = paf->next;

    if (paf->next)
	paf->next->prev = paf->prev;

    if (room->affected == paf)
	room->affected = paf->next;

    affect_modify_room(room, paf, FALSE);

    free_affect(paf);
    return;
}

/* random room generation procedure */
ROOM_INDEX_DATA  *get_random_room(CHAR_DATA *ch)
{
    ROOM_INDEX_DATA *room;

    for (; ;)
    {
	room = get_room_index(number_range(0, MAX_VNUM));
	if (room)
	{
	    if (can_see_room(ch, room)
		&& !room_is_private(room, MOUNTED(ch))
		&& !IS_SET(room->room_flags, ROOM_PRIVATE)
		&& !IS_SET(room->room_flags, ROOM_SOLITARY)
		&& !IS_SET(room->room_flags, ROOM_SAFE)
		&& !IS_SET(room->room_flags, ROOM_KILL)
		&& !IS_SET(room->room_flags, ROOM_NOFLY_DT)
		&& (IS_NPC(ch) || IS_SET(ch->act, ACT_AGGRESSIVE)
		    || !IS_SET(room->room_flags, ROOM_LAW)))
	    {
		break;
	    }
	}
    }

    return room;
}

bool is_room_affected(ROOM_INDEX_DATA *room, int sn)
{
    AFFECT_DATA *paf;

    for (paf = room->affected; paf != NULL; paf = paf->next)
	if (paf->type == sn)
	    return TRUE;

    return FALSE;
}

int TNL(CHAR_DATA *ch)
{
    return (IS_NPC(ch) || IS_IMMORTAL(ch)) ? 0 : (ch->level + 1) * exp_per_level(ch, ch->pcdata->points) - ch->exp;
}

void dump_container(OBJ_DATA *obj)
{
    OBJ_DATA *t_obj, *n_obj;

    if (!obj)
	return;

    for (t_obj = obj->contains; t_obj != NULL; t_obj = n_obj)
    {
	n_obj = t_obj->next_content;
	obj_from_obj(t_obj);
	if (obj->in_room != NULL)
	    obj_to_room(t_obj, obj->in_room);
	else if (obj->carried_by != NULL)
	    obj_to_room(t_obj, obj->carried_by->in_room);
	else
	    extract_obj(t_obj, TRUE, FALSE);
    }

    extract_obj(obj, TRUE, TRUE);
}

int check_wear_stat(CHAR_DATA *ch, OBJ_DATA *obj)
{
    int i;

    for (i = 0; i < MAX_STATS; i++)
	if (get_curr_stat(ch, i) < obj->require[i])
	    break;

    return i;
}

bool is_penalty_room(ROOM_INDEX_DATA *room)
{
    int proom;

    if (!room)
	return FALSE;

    for (proom = 0; proom < MAX_PENALTYROOMS; proom++)
	if (room->vnum == penalty_rooms_table[proom].vnum)
	    return TRUE;

    return FALSE;
}

int UMIN(int a, int b)
{
    return a < b ? a : b;
}

int UMAX(int a, int b)
{
    return a > b ? a : b;
}

int URANGE(int a, int b, int c)
{
    return b < a ? a : (b > c ? c : b);
}

bool check_class_weapon(CHAR_DATA *ch, OBJ_DATA *prim, OBJ_DATA *sec)
{
    if (ch->classid == CLASS_WARRIOR)
	return TRUE;

    if (prim->value[0] != sec->value[0])
	return FALSE;

    switch (ch->classid)
    {
    case CLASS_THIEF:
	return (prim->value[0] == WEAPON_DAGGER);
	break;
    case CLASS_PALADIN:
	return (prim->value[0] == WEAPON_SPEAR);
	break;
    case CLASS_RANGER:
	return (prim->value[0] == WEAPON_AXE);
     	break;
    case CLASS_VAMPIRE:
	return (prim->value[0] == WEAPON_EXOTIC);
	break;
    case CLASS_LYCANTHROPE:
	return (prim->value[0] == WEAPON_EXOTIC);
	break;
    default:
	return FALSE;
	break;
    }
}

bool check_wield_weight(CHAR_DATA *ch, OBJ_DATA *wield, bool secondary)
{
    int skill, mult;

    if (IS_NPC(ch))
	return TRUE;

    if (secondary)
    {
	mult = ch->classid == CLASS_WARRIOR ? 5 : 1;

	if ((skill = get_skill(ch, gsn_secondary_mastery)) > 0)
	    mult += skill/20;
    }
    else
	mult = 10;

    return (get_obj_weight(wield) <= mult*str_app[get_curr_stat(ch, STAT_STR)].wield);
}

bool check_primary_secondary(CHAR_DATA *ch, OBJ_DATA *prim, OBJ_DATA *sec)
{
    int pw, sw;

    if (IS_NPC(ch) || !sec
	|| (pw = get_obj_weight(prim)) > (sw = get_obj_weight(sec))
	|| (pw == sw  && check_class_weapon(ch, prim, sec)
	    && get_skill(ch, gsn_secondary_mastery) >= 100))
    {
	return TRUE;
    }

    return FALSE;
}

bool can_take_weight(CHAR_DATA *ch, int weight)
{
    int aw = 0;
/*
    for (i = 0; i < MAX_AUCTION; i++)
	if (ch == auctions[i].buyer)
	    aw += get_money_weight(auctions[i].cost, 0);
 */

    return (get_carry_weight(ch) + weight + aw <= can_carry_w(ch));
}

char *get_align(CHAR_DATA *gch, CHAR_DATA *ch)
{
    if (!ch || ch->level < 10)
    {
	if (gch->alignment >  900)      return "{Yангел";
	else if (gch->alignment >  700) return "{yсвятая";
	else if (gch->alignment >  350) return "{Gхорошая";
	else if (gch->alignment >  100) return "{gлюбезная";
	else if (gch->alignment > -100) return "{xнейтральная";
	else if (gch->alignment > -350) return "{mподлая";
	else if (gch->alignment > -700) return "{Mзлая";
	else if (gch->alignment > -900) return "{rдемон";
	else                            return "{Rсатана";
    }
    else
    {
	static char buf[10];

	sprintf(buf, "%d", gch->alignment);
	return buf;
    }
}

bool is_nopk(CHAR_DATA *ch){
    if (!ch)
	return FALSE;

    if (ch->desc)
	ch = CH(ch->desc);

    if (IS_NPC(ch))
	return FALSE;

    if (IS_SET(ch->act, PLR_NOPK))
	return TRUE;
    else 
	return FALSE;
}

bool check_charmees(CHAR_DATA *ch, CHAR_DATA *victim)
{
    if ((IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) || ch->mount == victim)
    {
	act("Но $N - твой друг!", ch, NULL, victim, TO_CHAR);
	return TRUE;
    }
    return FALSE;
}

void check_light_status(CHAR_DATA *ch){
    OBJ_DATA *obj;

    if (ch->in_room  && (obj = get_eq_char(ch, WEAR_LIGHT)) != NULL  && obj->item_type == ITEM_LIGHT && obj->value[2] != 0){
		++ch->in_room->light;
    }
}

bool has_trigger(PROG_LIST *target, int trig, int64_t when)
{
    PROG_LIST *list;

    for(list = target; list != NULL; list = list->next)
        if (list->trig_type == trig && IS_SET(list->trig_flags, when))
            return TRUE;    

    return FALSE;
}

/* charset=cp1251 */

