/***************************************************************************
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


#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include "merc.h"
#include "interp.h"
#include "magic.h"
#include "recycle.h"



int vampire_update(CHAR_DATA *ch);
int hmm_update(CHAR_DATA *ch);

/**/
/*
 * Local functions.
 */
void check_assist(CHAR_DATA *ch, CHAR_DATA *victim);
bool check_dodge(CHAR_DATA *ch, CHAR_DATA *victim);
void check_killer(CHAR_DATA *ch, CHAR_DATA *victim);
bool check_parry(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *weapon);
bool check_shield_block(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *weapon);
bool check_fight_off(CHAR_DATA *ch, CHAR_DATA *victim);
bool check_mist(CHAR_DATA *ch, CHAR_DATA *victim);
void dam_message(CHAR_DATA *ch, CHAR_DATA *victim, int dam,
		 int dt, bool immune, OBJ_DATA *wield);
void death_cry(CHAR_DATA *ch);
void group_gain(CHAR_DATA *ch, CHAR_DATA *victim);
int xp_compute(CHAR_DATA *gch, CHAR_DATA *victim,
	       int total_levels, bool use_time, int members, int pc_members);
void make_corpse(CHAR_DATA *ch, CHAR_DATA *killer);
void mob_hit(CHAR_DATA *ch, CHAR_DATA *victim, int dt);
void disarm(CHAR_DATA *ch, CHAR_DATA *victim , bool secondary);
bool one_hit(CHAR_DATA *ch, CHAR_DATA *victim, int dt, bool secondary);
bool check_filter(CHAR_DATA *ch, CHAR_DATA *victim);
float check_material(OBJ_DATA *obj, int16_t dt);
float get_style_multiplier(CHAR_DATA *ch);
bool can_mob_move(CHAR_DATA *ch, int door);
bool check_recovery(CHAR_DATA *victim, CHAR_DATA *ch);


void mob_ai(CHAR_DATA *ch) {}

bool drop_bad_obj(CHAR_DATA *gch, OBJ_DATA *obj, bool check_wield);

int fix_style(CHAR_DATA *ch)
{
    int sn = -1;

    switch(ch->style)
    {
    case AGGR_STYLE:
	sn = gsn_aggr_style;
	break;
    case CARE_STYLE:
	sn = gsn_care_style;
	break;
    case NORM_STYLE:
	break;
    default:
	bugf("Bad style!!!");
	break;
    }

    if (sn == -1 || get_skill(ch, sn) < 1)
	ch->style = NORM_STYLE;

    return ch->style;
}

void wait_state(CHAR_DATA *ch, int npulse)
{
    if (IS_IMMORTAL(ch))
	return;
    else
	ch->wait = UMAX(ch->wait, npulse);
}

void daze_state(CHAR_DATA *ch, int npulse)
{
    if (IS_IMMORTAL(ch))
	return;
    else
	ch->daze = UMAX(ch->daze, npulse);
}

bool check_mirror_image(CHAR_DATA *ch, CHAR_DATA *victim) {
	int chance;
	AFFECT_DATA *paf;

	if (!IS_AWAKE(victim))
		return FALSE;

	if (!is_affected(victim, gsn_mirror_image))
		return FALSE;

	/*Устанавливаем начальный шанс.*/
	chance = 5;

	/*Учитываем прокачку заклинания (умения)
    */
	chance += get_skill(victim, gsn_mirror_image) / 5;

	/*Находим аффект зеркальных отражений*/
	paf = affect_find(victim->affected, gsn_mirror_image);

	/*Вычитаем разницу в уровнях чара и каста
    */
	chance += (paf->level-ch->level);


	/* ЗЕРКАЛКИ ЗАВИСИЯТ ОТ РАЗНИЦЫ СТАТОВ ДЕРУЩИХСЯ. ДЛЯ МОБОВ СТАТЫ 23 по умолчанию взято с средних статов человека. 3% за 1 стат */
	if (!IS_NPC(ch)) {
		chance += 3 * (get_curr_stat(victim, STAT_WIS) + get_curr_stat(victim, STAT_INT) - get_curr_stat(ch, STAT_WIS) - get_curr_stat(ch, STAT_INT));
	} else {
		chance += 3 * (get_curr_stat(victim, STAT_WIS) + get_curr_stat(victim, STAT_INT) - 2 * AVG_STAT - 1);
	}
    
    /* СМЫСЛ ОДЕВАТСЬЯ В СТАТЫ, ЕСЛИ НЕ ДООДЕЛИ СТАТЫ - ИМЕЕМ ШТРАФЫ = 2% за 1 стат */
	/* ПРИ НЕ ДООДЕТОЙ СТАТЕ ШТРАФ ДВОИТСЯ И ЭТО ПРАВИЛЬНО НЕ ДОПОУЛЧАЕм 3% выше и 2% тут*/
    chance -= 2*(get_max_stat(victim, STAT_WIS) - get_curr_stat(victim, STAT_WIS));
    chance -= 2*(get_max_stat(victim, STAT_INT) - get_curr_stat(victim, STAT_INT));

	/* Учитываем стили боя: если мы в оборонке +20% и -20% если в атаке
	 * Если вараг в оборонке +20% и -20% если враг в атаке. Враг NPC без стиля или стль 1 по дефлоту*/
	chance *= get_style_multiplier(victim) + get_style_multiplier(ch) - 1;


	if (number_percent() >= chance)
        return FALSE;

    act("{WАтака $n1 попадает по твоему зеркальному отражению!{x",ch, NULL, victim, TO_VICT);
    act("{WТы попадаешь по зеркальному отражению $N1!{x",ch, NULL, victim, TO_CHAR);
    return TRUE;
}

bool check_gritstorm(CHAR_DATA *ch, CHAR_DATA *victim)
{
    int chance;
    AFFECT_DATA *paf;    

    if (!IS_AWAKE(victim))
        return FALSE;

    if (!is_affected(victim, gsn_gritstorm))
        return FALSE;

    /*Устанавливаем начальный шанс.
    */
    chance = 20;

    /*Учитываем прокачку заклинания (умения)
    */
    chance += get_skill(victim, gsn_gritstorm) / 5;

    /*Находим аффект вихрь осколков
    */
    paf = affect_find(victim->affected, gsn_gritstorm); 

    /*Вычитаем разницу в уровнях чара и каста
    */
    chance -= (ch->level - paf->level) / 5;

    if (number_percent() >= chance)
        return FALSE;

    damage(victim, ch, number_range(40,200), gsn_gritstorm, DAM_SLASH, TRUE, NULL);
    return TRUE;
}


/*
 * Расчет и нанесение повреждений заклинаниями
 * fire_ и ice_shield. Реализовано Викусом.
 */
void shield_damage(CHAR_DATA *ch, CHAR_DATA *victim, int dam, int64_t aff)
{
    int percent = 10, real_dam;
    int dam_type, sn;
    char buf[MAX_STRING_LENGTH], sh[20], dm[20];

    /* На самого себя щиты не должны действовать */
    /* При убегании иногда получалось, что щитом наносился дамаг */
    if (ch == victim || ch->in_room != victim->in_room)
	return;

    if (aff == AFF_FIRE_SHIELD)
    {
	dam_type = DAM_FIRE;
	sn = gsn_fire_shield;
	strcpy(sh, "огненный");
	strcpy(dm, "обжигает");
    }
    else if (aff == AFF_ICE_SHIELD)
    {
	dam_type = DAM_COLD;
	sn = gsn_ice_shield;
	strcpy(sh, "ледяной");
	strcpy(dm, "морозит");
    }
    else if (aff == AFF_CONIFEROUS_SHIELD)
    {
	dam_type = DAM_PIERCE;
	sn = gsn_coniferous_shield;
	strcpy(sh, "хвойный");
	strcpy(dm, "колет иголками");
    }
    else if (aff == AFF_DEATH_AURA)
    {
	dam_type = DAM_NEGATIVE;
	sn = gsn_death_aura;
	strcpy(sh, "неживой");
	strcpy(dm, "обжигает смертью");
    }
    else if (aff == AFF_HOLY_SHIELD)
    {
	dam_type = DAM_HOLY;
	sn = gsn_holy_shield;
	strcpy(sh, "святой");
	strcpy(dm, "бьет светом");
    }
    else
	return;

    if (saves_spell(victim->level, ch, dam_type))
	percent /= 2;

    percent -= check_immune(ch, dam_type) / 10;

    if (percent < 1 || (real_dam = (percent * dam)/100) < 1)
	return;

    if (aff == AFF_DEATH_AURA)
    {
	if ((IS_NPC(ch) && IS_SET(ch->act, ACT_UNDEAD)) || IS_VAMPIRE(ch))
	    return;

	printf_to_char("{WТвоя аура Смерти вытягивает жизнь из %s.{x\n\r",
		victim, PERS(ch, victim, 1));

	act("{WАура Смерти $N1 вытягивает жизнь из $n1.{x", ch, NULL, victim, TO_NOTVICT);
    }
    else
    {
	printf_to_char("{W%s щит %s %s тебя.{x\n\r",
		ch, capitalize(sh), PERS(victim, ch, 1), dm);

	printf_to_char("{WТвой %s щит %s %s.{x\n\r", victim, sh, dm, PERS(ch, victim, 3));

	sprintf(buf, "{W%s щит $N1 %s $n3.{x", capitalize(sh), dm);
	act(buf,  ch, NULL, victim, TO_NOTVICT);
    }

    victim->hit = UMIN(victim->max_hit, victim->hit + real_dam);
    damage(victim, ch, real_dam, sn, dam_type, (aff != AFF_DEATH_AURA), NULL);

    /*        if (!IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL && ch->hit < 1)
     ch->hit = 1; */

    return;
}

int get_leadership(CHAR_DATA *ch, bool hr)
{
    int skill;
    CHAR_DATA *leader, *vch, *vch_next;

    if 	((leader = ch->leader) == NULL)
    {
	LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, vch_next)
	{
	    if (ch != vch && is_same_group(ch, vch))
		leader = ch;
	}
    }

    if (!IS_NPC(ch)
	&& leader != NULL
	&& leader->in_room == ch->in_room
	&& leader->fighting != NULL
	&& ((skill = get_skill(leader, gsn_leadership)) > 0))
    {
	check_improve(leader, ch, gsn_leadership, TRUE, 8);
	return (number_range(skill/2, skill) *
	(hr ? GET_HITROLL(leader) : GET_DAMROLL(leader))) / 380;
    }

    return 0;
}

float get_style_multiplier(CHAR_DATA *ch)
{
    switch(fix_style(ch))
    {
    case NORM_STYLE:
	return 1;
    case AGGR_STYLE:
	return 1 - (float)(get_skill(ch, gsn_aggr_style)) / 500;
    case CARE_STYLE:
	return 1 + (float)(get_skill(ch, gsn_care_style)) / 500;
    default:
	bugf("Bad style!!!"); /*Предположительно для мобов*/
	return 1;
    }
}

static void check_air(CHAR_DATA *ch)
{
    EXIT_DATA *pexit;
    ROOM_INDEX_DATA *room;
    int count = 0, i;
    int vnums[MAX_CYCLES];
    bool cycle;

    for (i = 0; i < MAX_CYCLES; i++)
	vnums[i] = 0;

    while (1)
    {
	if ((pexit = ch->in_room->exit[DIR_DOWN]) == NULL
	    || ((room = pexit->u1.to_room) == NULL)
	    || room == ch->in_room
	    || IS_SET(pexit->exit_info, EX_CLOSED))
	{
	    break;
	}

	vnums[count] = ch->in_room->vnum;

	stop_fighting(ch, TRUE);
	act("$n падает вниз!", ch, NULL, NULL, TO_ROOM);
	send_to_char("{WТы падаешь вниз!{x", ch);
	char_from_room(ch);
	char_to_room(ch, room, TRUE);
	act("$n прилетает сверху!", ch, NULL, NULL, TO_ROOM);

	if (++count > MAX_CYCLES)
	    return;

	cycle = FALSE;

	for (i = 0; i < count; i++)
	    if (vnums[i] == ch->in_room->vnum)
	    {
		cycle = TRUE;
		break;
	    }

	if (check_trap(ch) || cycle)
	    return;

	if (room->sector_type != SECT_AIR)
	    break;

    }

    if (count > 0)
    {
	act("{WТы шмякаешься $x!{x", ch, NULL, NULL, TO_CHAR);
	damage(ch, ch, count * ch->max_hit/5, TYPE_UNDEFINED, DAM_BASH, FALSE, NULL);
    }
}

void for_killed_skills(CHAR_DATA *ch, CHAR_DATA *victim)
{
    OBJ_DATA *corpse;
    CHAR_DATA *ch1;
    char buf[MAX_STRING_LENGTH];

    if (IS_IMMORTAL(victim) && !IS_SWITCHED(victim))
	return;

    sprintf(buf, "%s умирает от рук %s в %s [room %d]",
	    (IS_NPC(victim) ? victim->short_descr : victim->name),
	    (IS_NPC(ch) ? cases(ch->short_descr, 1) : cases(ch->name, 1)),
	    victim->in_room != NULL ? victim->in_room->name : "неизвестно",
	    victim->in_room != NULL ? victim->in_room->vnum : 0);

    convert_dollars(buf);

    if (IS_NPC(victim))
	wiznet(buf, NULL, NULL, WIZ_MOBDEATHS, 0, 0);
    else
	wiznet(buf, NULL, NULL, WIZ_DEATHS, 0, 0);

    ch1 = get_master(ch);

    if (!IS_NPC(ch1))
    {
	if (!IS_NPC(victim)
	    && ch1 != victim)
	{
	    if (ch->in_room && ch->in_room->area)
		ch->in_room->area->popularity[POPUL_PK]++;

	    if (victim->pcdata->bounty > 0)
	    {
		sprintf(buf, "{YТы получаешь %d золота за голову %s!{x "
			"(эти деньги положены на твой счет в банке)\n\r",
			victim->pcdata->bounty, cases(victim->name, 1));
		send_to_char(buf, ch1);
		ch1->pcdata->bank += victim->pcdata->bounty;
		victim->pcdata->bounty = 0;
	    }
	}

	if (IS_NPC(victim))
	{
	    if (ch->in_room && ch->in_room->area)
		ch->in_room->area->popularity[POPUL_KILL]++;

	    if (IS_SET(ch1->act, PLR_QUESTING)
		&& ch1->pcdata->questmob == victim->pIndexData->vnum)
	    {
		CHAR_DATA *fch, *safe_fch;

		LIST_FOREACH_SAFE(fch, &char_list, link, safe_fch)
		{
		    if (fch == ch1
			|| (!IS_NPC(fch)
			    && IS_SET(ch1->pcdata->quest_type, QUEST_GROUP)
			    && is_same_group(fch, ch1)))
		    {
			printf_to_char("Ты полностью закончил%s квест!\n\rБеги быстрее к тому, "
				       "кто тебе давал этот квест, пока не вышло время!\n\r",
				       fch, SEX_ENDING(fch));

			fch->pcdata->questmob = -1;
		    }
		}
	    }
	}
    }
    else if (IS_NPC(ch1) && !IS_NPC(victim))
    {
	if (ch->in_room && ch->in_room->area)
	    ch->in_room->area->popularity[POPUL_DEATH]++;
    }

    /*
     * Death trigger
     */
    if (IS_NPC(victim) && ch != victim && has_trigger(victim->pIndexData->progs, TRIG_DEATH, EXEC_ALL))
    {
	victim->position = POS_STANDING;
	p_percent_trigger(victim, NULL, NULL, ch, NULL, NULL, TRIG_DEATH);
    }

    if (victim->in_room && has_trigger(victim->in_room->progs, TRIG_DEATH, EXEC_ALL))
	p_percent_trigger(NULL, NULL, victim->in_room, victim, NULL, NULL, TRIG_DEATH);

    raw_kill(victim, ch, TRUE);

    /*
     * (c) 2000 TAKA of GhostMud and the Ghost Dancer MUD Project
     * this adds to the counters for deaths and kills
     */

    if (IS_NPC(ch1)) /* is a mob */
    {
	if (!IS_NPC(victim)) /* mob kills character */
	    victim->pcdata->deaths_mob ++;
    }
    else /* is a character */
    {
	if (IS_NPC(victim)) /* character kills a mob */
	    ch1->pcdata->kills_mob ++;
	else if (ch1 != victim)/* character kills a pc */
	{
	    char logname[40], tmp[20], strtime[10], buf[MAX_STRING_LENGTH];

	    victim->pcdata->deaths_pc ++;
	    ch1->pcdata->kills_pc ++;

	    strftime(tmp, 20, "%Y.%m.%d", localtime(&current_time));
	    strftime(strtime, 10, "%T", localtime(&current_time));

	    sprintf(logname, "%s%s-pk.log", LOG_DIR, tmp);

	    sprintf(buf, "%s::Killer: %s [%d %s %s] Victim: %s [%d %s %s] Room: %d\n",
		    strtime, ch1->name, ch1->level,
		    pc_race_table[ch1->race].who_name,
		    is_clan(ch1) ? clan_table[ch1->clan].name : "нет",
		    victim->name, victim->level,
		    pc_race_table[victim->race].who_name,
		    is_clan(victim) ? clan_table[victim->clan].name : "нет",
		    victim->in_room ? victim->in_room->vnum : 0);

	    _log_string(buf, logname);
	}
    }


    /* dump the flags */
    if (ch != victim && !IS_NPC(victim) && (!is_clan(victim) || (is_clan(ch) != victim->clan || IS_INDEPEND(victim))))
    {
		if ((ch->in_room != NULL) && (!IS_SET(ch->in_room->room_flags, ROOM_HOUSE)) && (ch->in_room->clan == 0))
		{
			REMOVE_BIT(victim->act, PLR_KILLER);
			REMOVE_BIT(victim->act, PLR_THIEF);
		}
    }

    /* RT new auto commands */

    if (!IS_NPC(ch) && ch->in_room != NULL
	&& (corpse = get_obj_list(ch, "труп", ch->in_room->contents)) != NULL
	&& corpse->item_type == ITEM_CORPSE_NPC
	&& can_see_obj(ch, corpse))
    {
	if (corpse->contains)
	{
	    if (IS_SET(ch->act, PLR_AUTOLOOT))
	    {
		do_function(ch, &do_get, "все труп");
	    }
	    else if (IS_SET(ch->act, PLR_AUTOMONEY))
	    {
		if (get_obj_list(ch, "gcash", corpse->contains) != NULL)
		    do_function(ch, &do_get, "all.gcash труп");
	    }
	    else if (IS_SET(ch->act, PLR_AUTOGOLD))
	    {
		if (get_obj_list(ch, "goldcash", corpse->contains) != NULL)
		    do_function(ch, &do_get, "all gold труп");
	    }
	}

	if (IS_SET(ch->act, PLR_AUTOSAC))
	{
	    if (IS_SET(ch->act, PLR_AUTOLOOT) && corpse->contains)
		return;  /* leave if corpse has treasure */
	    else
		do_function(ch, &do_sacrifice, "труп");
	}
    }

    return;
}


/*****************************************************************************
Name:		char_in_room_update
Purpose:	Функция нанесения дамаги каждому кто находится вместе с шаровой молнией
		или облаками.
Called by:	fight.c: violence_update
****************************************************************************/
void char_in_room_update(CHAR_DATA *ch)
{
    int dam, level, chance = 0;
    OBJ_DATA *obj;

    static const int16_t dam_each[] =
    {
	0,
	0,  0,  0,  0,  0,  0,  0,  0, 25, 28,
	31, 34, 37, 40, 40, 41, 42, 42, 43, 44,
	44, 45, 46, 46, 47, 48, 48, 49, 50, 50,
	51, 52, 52, 53, 54, 54, 55, 56, 56, 57,
	58, 58, 59, 60, 60, 61, 62, 62, 63, 64,
	70, 70, 70, 70, 70, 70, 70, 70, 70, 70
    };


    if (!ch->in_room)
        return;
    
    for (obj = ch->in_room->contents; obj; obj = obj->next_content)
    {

	level  = UMIN(obj->level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
	level  = UMAX(0, level);

	dam    = number_range(dam_each[level], dam_each[level] * 2);

	chance  = obj->level;

	if (obj->pIndexData->vnum == OBJ_VNUM_LIGHTNING)
	{                   
	    if (weather_info.sky < SKY_RAINING)
	    {
        	extract_obj(obj, FALSE, FALSE);
        	act(skill_table[gsn_lightning_bolt].msg_room, ch, NULL, NULL, TO_CHAR);
		act(skill_table[gsn_lightning_bolt].msg_room, ch, NULL, NULL, TO_ROOM);
		return;
	    }

	    if (obj->level - ch->level > 15 || ch->level < 15)
		return;

	    if (saves_spell(obj->level, ch, DAM_LIGHTNING))
		dam /= 2;

	    damage(ch, ch, dam, gsn_lightning_bolt, DAM_LIGHTNING, FALSE, NULL);
	    send_to_char("Шаровая молния ударяет тебя.\n\r", ch);
	}

	if (obj->pIndexData->vnum == OBJ_VNUM_CLOUD)
	{
	    if (obj->level - ch->level > 15 || ch->level < 15)
		return;

	    /* killing mobiles */
	    if (IS_NPC(ch))
	    {
		if (IS_SWITCHED(ch))
		    return;

		if (ch->pIndexData->pShop != NULL)
		    return;

		/* no killing healers, trainers, etc */
		if (IS_SET(ch->act, ACT_TRAIN)
		    || IS_SET(ch->act, ACT_PRACTICE)
		    || IS_SET(ch->act, ACT_IS_HEALER)
		    || IS_SET(ch->act, ACT_IS_CHANGER)
		    || IS_SET(ch->act, ACT_BANKER)
		    || IS_SET(ch->act, ACT_QUESTER))
		    return;
	    }

	    switch (obj->value[0])
	    {
		AFFECT_DATA af;

	    case CLOUD_POISON:
		if (!saves_spell(obj->level, ch, DAM_POISON))
		{		    
		    chance -= 5*(obj->value[1] - obj->timer);
		    if (number_range(1,1000) <= chance)
			poison_effect(ch, obj->level/4, dam/8, TARGET_CHAR, ch);
		}
		break;
	    case CLOUD_ACID:
		if (!saves_spell(obj->level, ch, DAM_ACID)
		&& !is_affected(ch,gsn_protection_sphere)
		&& damage(ch, ch, dam, gsn_acid_fog, DAM_ACID, FALSE, NULL) > 0)
		{
		    send_to_char("Кислотный туман разъедает твое тело.\n\r", ch);
		    chance -= 5*(obj->value[1] - obj->timer);
		    if (number_range(1,500) <= chance)
			ch->move -= 5*obj->timer;
		    else if (number_range(1,1000) <= chance)
			acid_effect(ch, obj->level/4, dam/8, TARGET_CHAR, ch);
		}
		break;
	    case CLOUD_FIRE:
		if (obj->timer != obj->value[1])
		    if (!saves_spell(obj->level, ch, DAM_FIRE)
		    && !is_affected(ch,gsn_protection_sphere)
		    && damage(ch, ch, dam, gsn_flame_cloud, DAM_FIRE, FALSE, NULL))
		    {		
			send_to_char("Пламя огненного облака поражает тебя.\n\r", ch);
			chance += 10*(obj->value[1] - obj->timer);
			if (!is_affected(ch,gsn_immolation) && number_range(1,1000) <= chance)
			{
			    af.where     = TO_AFFECTS;
			    af.type      = gsn_immolation;
			    af.level     = obj->level;
			    af.duration  = 1 + obj->value[1] - obj->timer;
			    af.location  = APPLY_NONE;
			    af.modifier  = 0;
			    af.bitvector = 0;
			    af.caster_id = get_id(ch);
			    affect_to_char(ch, &af);
			    send_to_char("Тебя охватывают языки пламени.\n\r{x", ch);
			    act("$n1 охватывают языки пламени.{x", ch, NULL, NULL, TO_ROOM);
			}
			else if (number_range(1,1000) <= chance)
			    fire_effect(ch, obj->level/4, dam/8, TARGET_CHAR, ch);
		    }

		break;
	    default:
		break;
	    }
	    return;
	}
    }
}

bool check_parse_name(char *argument, bool newbie);
/*
 * Control the fights going on.
 * Called periodically by update_handler.
 */
void violence_update(void)
{
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    CHAR_DATA *victim;
    OBJ_DATA *obj, *obj_next;
    bool room_trig = FALSE;

    LIST_FOREACH_SAFE(ch, &char_list, link, ch_next)
    {
	if (!IS_VALID(ch))
	{
	    if (!IS_NPC(ch))
		VALIDATE(ch);

	    continue;
	}

	char_in_room_update(ch);

	if (vampire_update(ch) == 0 && ch->in_room)
	    if (hmm_update(ch) == TRUE)
	    {
		/* R.I.P. */
		continue;
	    }

	if (!IS_NPC(ch))
	{
	    /* Fucking hack.... */
/*	    AFFECT_DATA *paf, *paf_next;
	    int i = 0;
	    

	    if (ch->desc && ch->desc->connected == CON_PLAYING && !check_parse_name(ch->name, FALSE))
		bugf("Incorrect name '%s' Id: %ld  Cases: %s %s %s %s %s", ch->name, ch->id, 
		     ch->pcdata->cases[0], ch->pcdata->cases[1], ch->pcdata->cases[2], ch->pcdata->cases[3], ch->pcdata->cases[4]);

	    for (paf = ch->affected; paf != NULL; paf = paf->next)
		i++;

	    if (i > 30)
	    {
		bugf("Too many affects (%d) on character %s, resetting.", i, ch->name);

		send_to_char("На тебе слишком много эффектов! Пожалуйста, сохрани лог предыдущих действий, и вышли Богам!\n\r", ch);

		for (paf = ch->affected; paf != NULL; paf = paf_next)
		{
		    paf_next = paf->next;
		    affect_remove(ch, paf);
		}
	    } */

	    /* End of hacking */

	    if (!IS_IMMORTAL(ch) && ch->in_room && ch->in_room->area)
		ch->in_room->area->popularity[POPUL_VISIT]++;

	    check_clan_war(ch);

	    if (ch->desc && ch->desc->connected == CON_PLAYING && check_parse_name(ch->name, FALSE))
	    {
		int age;

		age = get_age(ch);
		if (ch->pcdata->old_age < age)
		{
		    char buf[MSL];

		    ch->pcdata->old_age = age;

		    sprintf(buf, "{GУ тебя сегодня День Рожденья! Тебе исполнилось сегодня %d!{x\n\r", age);
		    send_to_char(buf, ch);
		    /*    bugf("BirthDay: %s   {gChar: %s  Age: %d  Time: %s", buf, ch->name, age, ctime(&current_time)); */
		}
	    }
	}
	else if (ch->wait > 0)
	    ch->wait--;

	if ((victim = ch->fighting) == NULL && POS_FIGHT(ch))
	    ch->position = POS_STANDING;

	if (weather_info.sky == SKY_RAINING && number_bits(6) < 3 && IS_OUTSIDE(ch) && IS_AWAKE(ch))
	    send_to_char("Идет дождь...\n\r", ch);

	if (ch->in_room == NULL)
	    continue;

	if (ch->in_room->sector_type == SECT_AIR 
	&& !IS_AFFECTED(ch, AFF_FLYING) 
	&& (!MOUNTED(ch) || !IS_AFFECTED(MOUNTED(ch), AFF_FLYING)))
	{
	    check_air(ch);
	}

        if (!IS_VALID(ch))
	{
	    if (!IS_NPC(ch))
		VALIDATE(ch);
	    
	    continue;
	}
	
        if (MOUNTED(ch))
        {
	    CHAR_DATA *gch;

	    LIST_FOREACH(gch, &ch->in_room->people, room_link)
	    {
	        if (gch == MOUNTED(ch))
		    break;
	    }

	    if (gch == NULL)
	        do_dismount(ch, "update");
        }

	if (IS_NPC(ch))
	{
	    int door;
	    
	    if (ch->spec_fun)
	    {
		int i;
		bool flag = FALSE;

		for (i = 0; !IS_NULLSTR(spec_table[i].name); i++)
		{
		    if (ch->spec_fun == spec_table[i].function)
		    {
			if (!spec_table[i].can_cast
			    && IS_SET(ch->in_room->room_flags, ROOM_NOMAGIC))
			{
			    flag = TRUE;
			}
			break;
		    }
		}

		if (!flag)
		    if ((*ch->spec_fun)(ch) == SPEC_STOP)
			continue;
	    }

	    if (ch->pIndexData->pShop != NULL)
	    {
		/* Give him some gold */
		if ((ch->gold * 100 + ch->silver) < ch->pIndexData->wealth)
		{
		    ch->gold += ch->pIndexData->wealth * number_range(1, 20) / 1000000;
		    ch->silver += ch->pIndexData->wealth * number_range(1, 20) / 10000;
		}
	    }

	    /*
	     * Check triggers only if mobile still in default position
	     */
	    if (ch->position == ch->pIndexData->default_pos)
	    {
		/* Delay */
		if (has_trigger(ch->pIndexData->progs, TRIG_DELAY, EXEC_ALL)
		    && ch->mprog_delay > 0)
		{
		    if (--ch->mprog_delay <= 0)
		    {
			p_percent_trigger(ch, NULL, NULL, NULL, NULL, NULL,
					  TRIG_DELAY);
			continue;
		    }
		}

		if (has_trigger(ch->pIndexData->progs, TRIG_RANDOM, EXEC_ALL))
		{
		    if (p_percent_trigger(ch, NULL, NULL, NULL, NULL, NULL,
					  TRIG_RANDOM))
		    {
			continue;
		    }
		}
	    }

	    if (ch->position == POS_STANDING)
	    {
		/* Scavenge */
		if (IS_SET(ch->act, ACT_SCAVENGER)
		    && ch->in_room->contents != NULL
		    && number_bits(4) == 0)
		{
		    OBJ_DATA *obj;
		    OBJ_DATA *obj_best = NULL;
		    int max = 1;

		    for (obj = ch->in_room->contents; obj; obj = obj->next_content)
		    {
			if (CAN_WEAR(obj, ITEM_TAKE) && can_loot(ch, obj)
			    && obj->cost > max
			    && ch->carry_number + get_obj_number(obj) <= can_carry_n(ch)
			    && can_take_weight(ch, get_obj_weight(obj))
			    && count_users(obj) == 0)
			{
			    obj_best = obj;
			    max      = obj->cost;
			}
		    }

		    if (obj_best && !is_have_limit(ch, ch, obj_best))
		    {
			char buf[MIL];

			sprintf(buf, "'%s'", obj_best->name);
			do_get(ch, buf);
		    }
		}

		/* Wander */
		if (IS_SET(ch->act, ACT_HUNTER) && ch->memory != NULL && IS_AWAKE(ch))
		{
		    CHAR_DATA *victim;
		    TRACK_DATA *tr;
		    MEM_DATA *mem;

		    for (mem = ch->memory; mem != NULL; mem = mem->next)
			if (mem->valid)
			    break;

		    if (mem)
		    {
			LIST_FOREACH(victim, &char_list, link)
			    if (!IS_NPC(victim) && victim->id == mem->id)
				break;

			if (victim != NULL
			    && (tr = get_track(ch, victim)) != NULL
			    && can_mob_move(ch, tr->direction))
			{
			    if (number_percent() < 20)
			    {
				act("$n осматривается по сторонам.",
				    ch, NULL, NULL, TO_ROOM);
				do_say(ch, "Я найду тебя!");
				move_char(ch, tr->direction, TRUE, FALSE);
			    }
			    continue;
			}
		    }
		}

		if (number_bits(3) < 2 && can_mob_move(ch, (door = number_bits(5))))
		    move_char(ch, door, TRUE, FALSE);
	    }
	}

	if (!IS_VALID(ch))
	{
	    if (!IS_NPC(ch))
		VALIDATE(ch);

	    continue;
	}

	if (!victim)
	    continue;


	if (IS_AWAKE(ch))
	{
	    if (ch->in_room == victim->in_room)
		multi_hit(ch, victim, TYPE_UNDEFINED);
	    else
		stop_fighting(ch, TRUE);
	}
	else
	    stop_fighting(ch, FALSE);

	if ((victim = ch->fighting) == NULL)
	    continue;

	if (ch->position > POS_STUNNED && !check_recovery(ch, victim))
	    send_to_char("{WТебе лучше встать на ноги!{x\n\r", ch);

	/*
	 * Fun for the whole family!
	 */
	check_assist(ch, victim);

	if (IS_NPC(ch) && IS_VALID(ch))
	{
	    if (has_trigger(ch->pIndexData->progs, TRIG_FIGHT, EXEC_ALL))
		p_percent_trigger(ch, NULL, NULL, victim,
				  NULL, NULL, TRIG_FIGHT);

	    if (IS_VALID(ch) && has_trigger(ch->pIndexData->progs, TRIG_HPCNT, EXEC_ALL))
		p_hprct_trigger(ch, victim);
	}

	for (obj = ch->carrying; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;

	    if (obj->wear_loc != WEAR_NONE
		&& has_trigger(obj->pIndexData->progs, TRIG_FIGHT, EXEC_ALL))
	    {
		p_percent_trigger(NULL, obj, NULL, victim,
				  NULL, NULL, TRIG_FIGHT);
	    }
	}

	if (IS_VALID(ch)
	    && ch->in_room != NULL
	    && has_trigger(ch->in_room->progs, TRIG_FIGHT, EXEC_ALL)
	    && room_trig == FALSE)
	{
	    room_trig = TRUE;
	    p_percent_trigger(NULL, NULL, ch->in_room, victim,
			      NULL, NULL, TRIG_FIGHT);
	}

    }

    return;
}

/* for auto assisting */
void check_assist(CHAR_DATA *ch, CHAR_DATA *victim)
{
    CHAR_DATA *rch, *rch_next;

    if (ch == NULL || ch->in_room == NULL)
	return;

    LIST_FOREACH_SAFE(rch, &ch->in_room->people, room_link, rch_next)
    {
	char buf[MSL];

	if (IS_AWAKE(rch) && rch->fighting == NULL && can_see(rch, victim) && !IS_AFFECTED(rch, AFF_CALM))
	{
	    if ((RIDDEN(rch) == ch
		 || MOUNTED(rch) == ch)
		&& number_percent() < get_skill(ch, gsn_fight_on_mount))
	    {
		multi_hit(rch, victim, TYPE_UNDEFINED);
	    }

	    /* quick check for ASSIST_PLAYER */
	    if (!IS_NPC(ch) && IS_NPC(rch)
		&& IS_SET(rch->off_flags, ASSIST_PLAYERS)
		&& rch->level + 6 > victim->level)
	    {
		sprintf(buf, "броса%sся на помощь!", rch->sex == SEX_MANY ? "ют" : "ет");
		do_emote(rch, buf);

		multi_hit(rch, victim, TYPE_UNDEFINED);
		continue;
	    }

	    /* clan mob check */
	    if (IS_NPC(rch) && IS_SET(rch->off_flags, ASSIST_CLAN) 
		&& is_clan(ch) && ch->clan == rch->clan)
	    {
		multi_hit (rch, victim, TYPE_UNDEFINED);
		continue;
	    }

	    /* PCs next */
	    if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM))
	    {
		if (((!IS_NPC(rch) && IS_SET(rch->act, PLR_AUTOASSIST))
		     || IS_AFFECTED(rch, AFF_CHARM))
		    && is_same_group(ch, rch)
		    && !is_safe(rch, victim))
		{
		    multi_hit (rch, victim, TYPE_UNDEFINED);
		}

		continue;
	    }

	    /* now check the NPC cases */

	    if (IS_NPC(ch) 
		&& !IS_AFFECTED(ch, AFF_CHARM) 
		&& !IS_AFFECTED(rch, AFF_CHARM)
		&& rch->mount == NULL)	/*У всех == NULL, кроме лошадок с хозяином.*/
	    {
		if ((IS_NPC(rch) && IS_SET(rch->off_flags, ASSIST_ALL))
		    || (IS_NPC(rch) && rch->group && rch->group == ch->group)
		    || (IS_NPC(rch) && rch->race == ch->race
			&& IS_SET(rch->off_flags, ASSIST_RACE))
		    || (IS_NPC(rch) && IS_SET(rch->off_flags, ASSIST_ALIGN)
			&& ((IS_GOOD(rch) && IS_GOOD(ch))
			    || (IS_EVIL(rch) && IS_EVIL(ch))
			    || (IS_NEUTRAL(rch) && IS_NEUTRAL(ch))))
		    || (rch->pIndexData == ch->pIndexData
			&& IS_SET(rch->off_flags, ASSIST_VNUM)))
		{
		    CHAR_DATA *vch;
		    CHAR_DATA *target;
		    int number;

		    if (number_bits(1) == 0)
			continue;

		    target = NULL;
		    number = 0;
		    LIST_FOREACH(vch, &ch->in_room->people, room_link)
		    {
			if (can_see(rch, vch)
			    && is_same_group(vch, victim)
			    && number_range(0, number) == 0)
			{
			    target = vch;
			    number++;
			}
		    }

		    if (target != NULL)
		    {
			sprintf(buf, "броса%sся в атаку!", rch->sex == SEX_MANY ? "ют" : "ет");
			do_emote(rch, buf);
			multi_hit(rch, target, TYPE_UNDEFINED);
		    }
		}
	    }
	}
    }
}

/* ОТБИТЬ АТАКУ */
void strip_fight_off(CHAR_DATA *ch, CHAR_DATA *victim)
{
    CHAR_DATA *orig;

    if (is_affected(victim, gsn_fight_off))
    {
	affect_strip(victim, gsn_fight_off);
	act("{WУ тебя не получается отбить атаку $N1...{x",
	    victim, NULL, ch, TO_CHAR);
    }


    orig = IS_SWITCHED(ch) ? ch = CH(ch->desc) : ch;

    if (!IS_NPC(orig) && orig->pcdata)
	orig->pcdata->avg_dam = orig->pcdata->avg_dam > 0 ? 
	    (orig->pcdata->avg_dam + orig->pcdata->dam_tick) / 2 : 
	    orig->pcdata->dam_tick;

    orig->pcdata->dam_tick = 0;
}

/*
 * Do one group of attacks.
 */
void multi_hit(CHAR_DATA *ch, CHAR_DATA *victim, int dt)
{
    int     chance;
    char multiplier = 10;
    int gsn_style = 0;
    int mdam = 0, fom = 0, bk = 0, siz = 0;

    /* decrement the wait */
    if (ch->desc == NULL)
	ch->wait = UMAX(0, ch->wait - PULSE_VIOLENCE);

    if (ch->desc == NULL)
	ch->daze = UMAX(0, ch->daze - PULSE_VIOLENCE);

    /* no attacks for stunnies -- just a check */
    if (ch->position < POS_RESTING)
	return;

    check_gritstorm(ch,victim);

    if (IS_NPC(ch))
    {
	mob_hit(ch, victim, dt);
	return;
    }

    if (dt == gsn_backstab || dt == gsn_dual_backstab)
    {
		siz = ch->size - victim->size;
		bk = dex_app[get_curr_stat(ch, STAT_DEX)].tohit - (siz >= 0 ? siz * 4 : siz * 3);
    }

    switch (fix_style(ch))
    {
    case (NORM_STYLE):	break;
    case (CARE_STYLE):	gsn_style = gsn_care_style;
			multiplier = -1;
			break;
    case (AGGR_STYLE):	gsn_style = gsn_aggr_style;
			multiplier = 1;
			break;
    default:		bugf("Unknown style!!!\n");
			break;
    }


    if (gsn_style != 0)
    {
	multiplier = 10 + multiplier * (get_skill (ch, gsn_style) / 20);
	check_improve (ch, victim, gsn_style, TRUE, 20);
    }

    if (MOUNTED(ch))
    {
	if (!mount_success(ch, MOUNTED(ch), FALSE, TRUE))
	    send_to_char("{WТы падаешь с лошади, когда начинаешь атаку!{x\n\r", ch);
    }

    if (MOUNTED(victim))
    {
	fom = get_skill(victim, gsn_fight_on_mount);
	fom += (victim->mount->size - victim->size) * 5;
    }

    one_hit(ch, victim, dt, FALSE);

    if (ch->fighting != victim)
    {
	strip_fight_off(ch, victim);
	return;
    }

    if (get_eq_char (ch, WEAR_SECONDARY)
	|| get_weapon_sn(ch, TRUE) == gsn_hand_to_hand)
    {
	if (MOUNTED(victim) && UMAX(2, fom/6-mdam*3) > number_percent())
	{
	    one_hit(ch, victim->mount, dt, TRUE);
	    mdam++;
	}
	else
	    one_hit(ch, victim, dt, TRUE);

	if (ch->fighting != victim)
	{
	    strip_fight_off(ch, victim);
	    return;
	}

	if (get_weapon_sn(ch, TRUE) != gsn_hand_to_hand)
	{
	    check_improve(ch, victim, gsn_second_weapon, TRUE, 2);
	    check_improve(ch, NULL, gsn_secondary_mastery, TRUE, 3);
	}
    }

    if (IS_AFFECTED(ch, AFF_HASTE))
    {
	if (MOUNTED(victim) && UMAX(2, fom/6-mdam*3) > number_percent())
	{
	    one_hit(ch, victim->mount, dt, FALSE);
	    mdam++;
	}
	else
	    one_hit(ch, victim, dt, FALSE);

	if (ch->fighting != victim)
	{
	    strip_fight_off(ch, victim);
	    return;
	}
    }

    chance = get_skill(ch, gsn_second_attack) * multiplier / 15;

    if (IS_AFFECTED(ch, AFF_SLOW))
	chance /= 2;

    if (number_percent() < chance + bk)
    {
	if (MOUNTED(victim) && UMAX(2, fom/4-mdam*3) > number_percent())
	{	    
	    one_hit(ch, victim->mount, dt , FALSE);
	    mdam++;
	}
	else
	{
	    one_hit(ch, victim, dt , FALSE);
	    if (ch->fighting != victim)
	    {
		strip_fight_off(ch, victim);
		return;
	    }
	}

	if ((get_eq_char (ch, WEAR_SECONDARY) || get_weapon_sn(ch, TRUE) == gsn_hand_to_hand)
	    && number_percent() < (chance + bk)/2)
	{
	    if (MOUNTED(victim) && UMAX(2, fom/4-mdam*3) > number_percent())
	    {
		one_hit(ch, victim->mount, dt, TRUE);
		mdam++;
	    }
	    else
	    {
		one_hit(ch, victim, dt, TRUE);
		if (ch->fighting != victim)
		{
		    strip_fight_off(ch, victim);
		    return;
		}
	    }

	    check_improve(ch, victim, gsn_second_attack, TRUE, 3);

	    if (get_weapon_sn(ch, TRUE) != gsn_hand_to_hand)
	    {
		check_improve(ch, victim, gsn_second_weapon, TRUE, 3);
		check_improve(ch, NULL, gsn_secondary_mastery, TRUE, 3);
	    }
	}

	check_improve(ch, victim, gsn_second_attack, TRUE, 3);
    }

    chance = get_skill(ch, gsn_third_attack) * multiplier / 20;

    if (IS_AFFECTED(ch, AFF_SLOW))
	chance = chance/3;

    if (number_percent() < chance + bk)
    {
	if (MOUNTED(victim) && UMAX(2, fom/4-mdam*3) > number_percent())
	{
	    one_hit(ch, victim->mount, dt , FALSE);
	}
	else
	{
	    one_hit(ch, victim, dt , FALSE);

	    if (ch->fighting != victim)
	    {
		strip_fight_off(ch, victim);
		return;
	    }
	}

	if ((get_eq_char(ch, WEAR_SECONDARY)
	     || get_weapon_sn(ch, TRUE) == gsn_hand_to_hand)
	    && number_percent() < (chance + bk)/3)
	{
	    if (MOUNTED(victim)	&& UMAX(2, fom/4-mdam*3) > number_percent())
	    {
		one_hit(ch, victim->mount, dt, TRUE);
		mdam++;
	    }
	    else
	    {
		one_hit(ch, victim, dt, TRUE);
		if (ch->fighting != victim)
		{
		    strip_fight_off(ch, victim);
		    return;
		}
	    }
	    check_improve(ch, victim, gsn_third_attack, TRUE, 4);

	    if (get_weapon_sn(ch, TRUE) != gsn_hand_to_hand)
	    {
		check_improve(ch, victim, gsn_second_weapon, TRUE, 4);
		check_improve(ch, NULL, gsn_secondary_mastery, TRUE, 4);
	    }
	}

	check_improve(ch, victim, gsn_third_attack, TRUE, 4);
    }

    chance = get_skill (ch, gsn_fourth_attack) * multiplier / 25;

    if (IS_AFFECTED(ch, AFF_SLOW))
	chance = 0;

    if (number_percent() < chance + bk)
    {
	if (MOUNTED(victim) && UMAX(2, fom/4-mdam*3) > number_percent())
	{
	    one_hit(ch, victim->mount, dt , FALSE);
	    mdam++;
	}
	else
	{
	    one_hit(ch, victim, dt , FALSE);
	}

	if (ch->fighting != victim)
	{
	    strip_fight_off(ch, victim);
	    return;
	}

	if ((get_eq_char (ch, WEAR_SECONDARY)
	     || get_weapon_sn(ch, TRUE) == gsn_hand_to_hand)
	    && number_percent() < (chance + bk)/4)
	{
	    if (MOUNTED(victim) && UMAX(2, fom/4-mdam*3) > number_percent())
		one_hit(ch, victim->mount, dt, TRUE);
	    else
	    {
		one_hit(ch, victim, dt, TRUE);
		if (ch->fighting != victim)
		{
		    strip_fight_off(ch, victim);
		    return;
		}
	    }
	    check_improve(ch, victim, gsn_fourth_attack, TRUE, 5);

	    if (get_weapon_sn(ch, TRUE) == gsn_hand_to_hand)
	    {
		check_improve(ch, victim, gsn_second_weapon, TRUE, 5);
		check_improve(ch, NULL, gsn_secondary_mastery, TRUE, 4);
	    }
	}

	check_improve(ch, victim, gsn_fourth_attack, TRUE, 5);
    }

    strip_fight_off(ch, victim);

    return;
}

/* procedure for all mobile attacks */
void mob_hit(CHAR_DATA *ch, CHAR_DATA *victim, int dt)
{
    int chance, number, fom = 0, mdam = 0;
    CHAR_DATA *vch, *vch_next;    

    if (!IS_VALID(ch) || !IS_VALID(victim))
	return;

    if (!IS_NPC(ch))
	return;

    if (MOUNTED(victim))
    {
	fom = get_skill(victim, gsn_fight_on_mount);
	fom += (victim->mount->size - victim->size) * 5;
    }

    if (ch->position == POS_BASHED && ch->wait == 0)
	do_function(ch, &do_stand, "");

    if (MOUNTED(victim) && UMAX(2, fom/6-mdam*3) > number_percent())
    {
	one_hit(ch, victim->mount, dt, FALSE);
	mdam++;
    }
    else
	one_hit(ch, victim, dt, FALSE);

    if (get_eq_char (ch, WEAR_SECONDARY))
    {
	if (MOUNTED(victim) && UMAX(2, fom/6-mdam*3) > number_percent())
	{
	    one_hit(ch, victim->mount, dt, TRUE);
	    mdam++;
	}
	else
	    one_hit(ch, victim, dt, TRUE);

	if (ch->fighting != victim)
	    return;
    }

    if (ch->fighting != victim)
	return;

    /* Area attack -- BALLS nasty! */

    if (IS_SET(ch->off_flags, OFF_AREA_ATTACK) && ch->in_room != NULL)
    {
	LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, vch_next)
	{
	    if ((vch != victim && vch->fighting == ch))
		one_hit(ch, vch, dt, FALSE);
	}
    }

    if (IS_AFFECTED(ch, AFF_HASTE)
	|| (IS_SET(ch->off_flags, OFF_FAST)
	    && !IS_AFFECTED(ch, AFF_SLOW)))
    {
	if (MOUNTED(victim) && UMAX(2, fom/4-mdam*3) > number_percent())
	{
	    one_hit(ch, victim->mount, dt, FALSE);
	    mdam++;
	}
	else
	    one_hit(ch, victim, dt, FALSE);
	
    }

    if (ch->fighting != victim)
	return;

    chance = get_skill(ch, gsn_second_attack)/2;

    if (IS_AFFECTED(ch, AFF_SLOW) && !IS_SET(ch->off_flags, OFF_FAST))
	chance /= 2;

    if (number_percent() < chance)
    {
	if (MOUNTED(victim) && UMAX(2, fom/4-mdam*3) > number_percent())
	{	    
	    one_hit(ch, victim->mount, dt, FALSE);
	    mdam++;
	}
	else
	    one_hit(ch, victim, dt, FALSE);

	if (get_eq_char (ch, WEAR_SECONDARY))
	{
	    if (MOUNTED(victim) && UMAX(2, fom/4-mdam*3) > number_percent())
	    {
		one_hit(ch, victim->mount, dt, TRUE);
		mdam++;
	    }
	    else
		one_hit(ch, victim, dt, TRUE);

	    if (ch->fighting != victim)
		return;
	}

	if (ch->fighting != victim)
	    return;
    }

    chance = get_skill(ch, gsn_third_attack)/4;

    if (IS_AFFECTED(ch, AFF_SLOW) && !IS_SET(ch->off_flags, OFF_FAST))
	chance /= 3;

    if (number_percent() < chance)
    {
	if (MOUNTED(victim) && UMAX(2, fom/4-mdam*3) > number_percent())
	{
	    one_hit(ch, victim->mount, dt, FALSE);
	    mdam++;
	}
	else
	    one_hit(ch, victim, dt, FALSE);

	if (ch->fighting != victim)
	    return;

	if (get_eq_char (ch, WEAR_SECONDARY))
	{
	    if (MOUNTED(victim) && UMAX(2, fom/4-mdam*3) > number_percent())
	    {
		one_hit(ch, victim->mount, dt, TRUE);
		mdam++;
	    }
	    else
		one_hit(ch, victim, dt, TRUE);

	    if (ch->fighting != victim)
		return;
	}
    }

    chance = get_skill(ch, gsn_fourth_attack)/6;

    if (IS_AFFECTED(ch, AFF_SLOW))
	chance = 0;

    if (number_percent() < chance)
    {
	if (MOUNTED(victim) && UMAX(2, fom/4-mdam*3) > number_percent())
	{
	    one_hit(ch, victim->mount, dt , FALSE);
	    mdam++;
	}
	else
	    one_hit(ch, victim, dt , FALSE);

	if (get_eq_char (ch, WEAR_SECONDARY) && number_percent() < chance/4)
	{
	    if (MOUNTED(victim) && UMAX(2, fom/4-mdam*3) > number_percent())
		one_hit(ch, victim->mount, dt, TRUE);
	    else
		one_hit(ch, victim, dt, TRUE);

	    if (ch->fighting != victim)
		return;
	}

	if (ch->fighting != victim)
	    return;
    }

    if (is_affected(victim, gsn_fight_off))
    {
	affect_strip(victim, gsn_fight_off);
	act("{WУ тебя не получается отбить атаку $N1...{x",
	    victim, NULL, ch, TO_CHAR);
    }


    /* oh boy!  Fun stuff! */

    if (ch->wait > 0)
	return;


    if (ch->pIndexData->vnum == MOB_BONE_SHIELD && ch->leader != NULL)
    {
	do_function(ch, &do_rescue, ch->leader->name);
	return;
    }

    if (ch->pIndexData->vnum == MOB_VNUM_GOMUN && ch->master != NULL)
    {
	if (number_percent() < 70 + (ch->level - ch->leader->level) * 3
	&& IS_SET(ch->off_flags, OFF_RESCUE))
	    do_function(ch, &do_rescue, ch->master->name);

	return;
    }

    number = number_range(0, 8);

    switch(number)
    {
    case (0) :
	if (IS_SET(ch->off_flags, OFF_BASH)
	    && victim->position != POS_BASHED)
	{
	    do_function(ch, &do_bash, "");
	    break;
	}

    case (1) :
	if (IS_SET(ch->off_flags, OFF_BERSERK)
	    && !IS_AFFECTED(ch, AFF_BERSERK))
	{
	    do_function(ch, &do_berserk, "");
	    break;
	}

    case (2) :
	if (IS_SET(ch->off_flags, OFF_DISARM)
	    || (get_weapon_sn(ch, FALSE) != gsn_hand_to_hand
		&& (IS_SET(ch->act, ACT_WARRIOR)
		    ||  IS_SET(ch->act, ACT_THIEF))))
	{
	    do_function(ch, &do_disarm, "");
	    break;
	}

    case (3) :
	if (IS_SET(ch->off_flags, OFF_KICK))
	{
	    do_function(ch, &do_kick, "");
	    break;
	}

    case (4) :
	if (IS_SET(ch->off_flags, OFF_KICK_DIRT)
	    && !IS_AFFECTED(ch, AFF_BLIND))
	{
	    do_function(ch, &do_dirt, "");
	    break;
	}

    case (5) :
	if (IS_SET(ch->off_flags, OFF_TAIL)
	    && victim->position != POS_BASHED)
	{
	    do_function(ch, &do_tail, "");
	    break;
	}

    case (6) :
	if (IS_SET(ch->off_flags, OFF_TRIP)
	    && victim->position != POS_BASHED)
	{
	    do_function(ch, &do_trip, "");
	    break;
	}

    case (7) :
	if (IS_SET(ch->off_flags, OFF_RESCUE)
	    && ch->master != NULL)
	{
	    do_function(ch, &do_rescue, ch->master->name);
	}
	break;

    case (8) :
	if (IS_SET(ch->off_flags, OFF_BACKSTAB))
	{
	    do_function(ch, &do_backstab, "");
	}
    }
}


int check_magic_armor(CHAR_DATA *ch)
{
    AFFECT_DATA *paf;
    int res = 0;

    for (paf = ch->affected; paf != NULL; paf = paf->next)
    {
	if (paf->where == TO_AFFECTS 
	&& paf->location == APPLY_AC)
	    res += paf->modifier;
    }
    return res;
}

int check_weapon_armor(CHAR_DATA *ch)
{
    return (100 +(GET_AC(ch, AC_PIERCE) + GET_AC(ch, AC_BASH) 
		+ GET_AC(ch, AC_EXOTIC) + GET_AC(ch, AC_SLASH)) / 4) 
	- check_magic_armor(ch) 
	- (IS_AWAKE(ch) ? dex_app[get_curr_stat(ch,STAT_DEX)].defensive : 0);
}

/*
 * Hit one guy once.
 */
bool one_hit(CHAR_DATA *ch, CHAR_DATA *victim, int dt, bool secondary)
{
    OBJ_DATA *wield;
    int victim_ac;
    int thac0, thac;
    int thac0_00;
    int thac0_32;
    int dam, siz = 0;
    int diceroll;
    int sn = -1, skill, wn;
    int dam_type;
    bool result;
    char multiplier_ch = 10, multiplier_victim = 10;
    int gsn_style_ch = 0, gsn_style_victim = 0;
    AFFECT_DATA *paf;


    /* just in case */
    if (victim == ch || !IS_VALID(ch) || !IS_VALID(victim))
	return FALSE;

    /*
     * Can't beat a dead char!
     * Guard against weird room-leavings.
     */
    if (victim->position == POS_DEAD || ch->in_room != victim->in_room)
	return FALSE;

    /*
     * Figure out the type of damage message.
     */

    wield = get_eq_char(ch, (wn = secondary ? WEAR_SECONDARY : WEAR_WIELD));

    if (dt == TYPE_UNDEFINED)
    {
	dt = TYPE_HIT;
	if (wield != NULL && wield->item_type == ITEM_WEAPON)	
	    if (is_lycanthrope(ch) 
		&& ((paf = affect_find(ch->affected, gsn_claws)) != NULL)
		&& ch->mana > 5)
		dt += attack_lookup_dam(paf->bitvector);
	    else
		dt += wield->value[3];
	else
	    dt += ch->dam_type;
    }

    if (dt < TYPE_HIT)
	if (wield != NULL)
	    if (is_lycanthrope(ch) 
		&& ((paf = affect_find(ch->affected, gsn_claws)) != NULL)
		&& ch->mana > 5)
		dam_type = paf->bitvector;
	    else
		dam_type = attack_table[wield->value[3]].damage;
	else
	    dam_type = attack_table[ch->dam_type].damage;
    else
	dam_type = attack_table[dt - TYPE_HIT].damage;

    if (dam_type == -1)
	dam_type = DAM_BASH;

    /* get the weapon skill */
    sn = get_weapon_sn(ch, secondary);
    skill = 20 + get_weapon_skill(ch, sn);

/*-Old calculate to-hit-armor-class-0 versus armor.--------------------------

    if (IS_NPC(ch))
    {
	thac0_00 = 20;
	thac0_32 = -4;   

	if (IS_SET(ch->act, ACT_WARRIOR))
	    thac0_32 = -10;
	else if (IS_SET(ch->act, ACT_THIEF))
	    thac0_32 = -4;
	else if (IS_SET(ch->act, ACT_CLERIC))
	    thac0_32 = 2;
	else if (IS_SET(ch->act, ACT_MAGE))
	    thac0_32 = 6;
    }
    else
    {
	thac0_00 = class_table[ch->class].thac0_00;
	thac0_32 = class_table[ch->class].thac0_32;
    }
    thac0  = interpolate(ch->level, thac0_00, thac0_32);

    if (thac0 < 0)
	thac0 = thac0/2;

    if (thac0 < -5)
	thac0 = -5 + (thac0 + 5) / 2;

    thac0 -= (MOUNTED(ch) ? 0.5 + get_skill(ch, gsn_fight_on_mount)/300 : 1)
	* (GET_HITROLL(ch)*2 + get_leadership(ch, TRUE)) * skill/100;
    thac0 += 5 * (100 - skill) / 100;

    if (dt == gsn_backstab)
	thac0 -= get_skill(ch, gsn_backstab) / 10;

    if (dt == gsn_dual_backstab)
    {
	thac0 -= get_skill(ch, gsn_dual_backstab) / 10;
    }

    if (dt == gsn_ambush)
	thac0 -= get_skill(ch, gsn_ambush) / 10;

    if (dt == gsn_ambush_move)
	thac0 -= get_skill(ch, gsn_ambush_move) / 25;


    switch(dam_type)
    {
    case(DAM_PIERCE):
	victim_ac = GET_AC(victim, AC_PIERCE);
	break;
    case(DAM_BASH):
	victim_ac = GET_AC(victim, AC_BASH);
	break;
    case(DAM_SLASH):
	victim_ac = GET_AC(victim, AC_SLASH);
	break;
    default:
	victim_ac = GET_AC(victim, AC_EXOTIC);
	break;
    };

    victim_ac /= 10;

    if (victim_ac < -15)
	victim_ac = (victim_ac + 15) / 5 - 15;

    if ((dam = get_skill(victim, gsn_armor_use)) > 1)
    {
	float add;

	check_improve(victim, ch, gsn_armor_use, TRUE, 25);
	add = (1 + (float) (number_range(1, dam)) / 250);

	victim_ac = victim_ac > 0? rintf(victim_ac/add) : rintf(victim_ac*add);
    }

    if (!can_see(ch, victim))
	victim_ac -= 4;

    if (victim->position < POS_FIGHTING)
	victim_ac += 4;

    if (victim->position < POS_RESTING)
	victim_ac += 6;

    if (victim->position == POS_BASHED)
	victim_ac += 8;


    while ((diceroll = number_bits(5)) >= 20)
	;

    if (MOUNTED(ch) && number_percent() < get_skill(ch, gsn_fight_on_mount))
	stop_fighting(MOUNTED(ch), FALSE);

    if (!IS_NPC(ch) && IS_SET(ch->act, PLR_SHOW_DAMAGE))
	printf_to_char("{B(%d %d %d) {x", ch, diceroll, thac0, victim_ac);

    if (diceroll == 0
	|| (diceroll != 19 && diceroll < thac0 - victim_ac))
    {
	if (!IS_NPC(ch) && IS_SET(ch->act, PLR_SHOW_DAMAGE))
	    send_to_char("{B(Miss) {x", ch);

	if (MOUNTED(ch))
	    check_improve (ch, victim, gsn_fight_on_mount, FALSE, 30);

	damage(ch, victim, 0, dt, dam_type, TRUE, NULL);
	tail_chain();
	return FALSE;
    }
---------------------------------------------------------------------------*/


/*-New calculate to-hit-armor-class-0 versus armor.--------------------------*/

/*Для обеспечения более детального учета значений thac0 и ac, изменим 
*стандартный DND (20), увеличив до 200 (20*10).
*
*Определение начального значения thac0
*/
    if (IS_NPC(ch)) {
	thac0_00 = 150;
	thac0_32 = 0;

	if (IS_SET(ch->act, ACT_WARRIOR))
	    thac0_32 = -50;
	else if (IS_SET(ch->act, ACT_THIEF))
	    thac0_32 = -30;
	else if (IS_SET(ch->act, ACT_CLERIC))
	    thac0_32 = 10;
	else if (IS_SET(ch->act, ACT_MAGE))
	    thac0_32 = 20;
    } else {
		thac0_00 = class_table[ch->classid].thac0_00 * 10;
		thac0_32 = class_table[ch->classid].thac0_32 * 10;
    }

/*Интерполяция по уровню
*/
    thac0  = interpolate(ch->level, thac0_00, thac0_32);

/*Сглаживание 1
*/
    if (thac0 < 0)
	thac0 = thac0/2;

/*Сглаживание 2
*/
    if (thac0 < -25)
	thac0 = -25 + (thac0 + 25) / 2;

/*Учет силы - пробиваемость брони
*/
    if (IS_NPC(ch))
	thac0 -= str_app[get_curr_stat(ch, STAT_STR)].tohit * 2;
    else
	thac0 -= str_app[get_curr_stat(ch, STAT_STR)].tohit * 3;

/*Хитролл и лидерство в зависимости от состояния (верхом и стоя)
*/
    thac0 -= (MOUNTED(ch) ? 25 + get_skill(ch, gsn_fight_on_mount)/6 : 50)
	* (GET_HITROLL(ch)*2 + get_leadership(ch, TRUE)) / 100;

/*Учет умения владеть оружием
*/
    thac0 += 25 * (120 - skill) / 100;

    siz = ch->size - victim->size;

/*Учет специальных умений персонажа
*/
    if (dt == gsn_backstab || dt == gsn_dual_backstab)
    {
	thac0 -= get_skill(ch, gsn_backstab) / 2;
	if (ch->size == SIZE_SMALL)
	{
	    thac0 += str_app[get_curr_stat(ch, STAT_STR)].tohit * 2;
	    thac0 -= dex_app[get_curr_stat(ch, STAT_DEX)].tohit * 9 / 4;
	} if (ch->size == SIZE_MEDIUM)
	{
	    thac0 += str_app[get_curr_stat(ch, STAT_STR)].tohit;
	    thac0 -= dex_app[get_curr_stat(ch, STAT_DEX)].tohit * 10 / 3;
	}
	else
	{
	    thac0 += str_app[get_curr_stat(ch, STAT_STR)].tohit;
	    thac0 -= dex_app[get_curr_stat(ch, STAT_DEX)].tohit;
	}

	if (siz > 0)
	    thac0 = thac0 * (100 - siz*5) / 100;

/*Сглаживание 3
*/
	if (thac0 < -100)
	    thac0 = -100 + (thac0 + 100) / 2;
    }

    if (dt == gsn_ambush)
		thac0 -= get_skill(ch, gsn_ambush) * 2 / 3;

    if (dt == gsn_ambush_move)
		thac0 -= get_skill(ch, gsn_ambush_move) / 2;

/*Определение значения ac
*/
    switch(dam_type)
    {
    case(DAM_PIERCE):
	victim_ac = GET_AC(victim, AC_PIERCE);
	break;
    case(DAM_BASH):
	victim_ac = GET_AC(victim, AC_BASH);
	break;
    case(DAM_SLASH):
	victim_ac = GET_AC(victim, AC_SLASH);
	break;
    default:
	victim_ac = GET_AC(victim, AC_EXOTIC);
	break;
    };

/*Сглаживание 1
*/
    if (victim_ac < -50)
	victim_ac = (victim_ac + 50) / 2 - 50;

/*Сглаживание 2
*/
    if (victim_ac < -150)
	victim_ac = (victim_ac + 150) / 2 - 150;

    if ((dam = get_skill(victim, gsn_armor_use)) > 1)
    {
	float add;

	check_improve(victim, ch, gsn_armor_use, TRUE, 25);
	add = (1 + (float) (number_range(1, dam)) / 250);

	victim_ac = victim_ac > 0 ? rintf(victim_ac/add) : rintf(victim_ac*add);
    }

    if (!can_see(ch, victim))
	victim_ac -= 40;

    if (victim->position < POS_FIGHTING)
	victim_ac += 40;

    if (victim->position < POS_RESTING)
	victim_ac += 60;

    if (victim->position == POS_BASHED)
	victim_ac += 80;

    /*
     * The moment of excitement!
     */
    while ((diceroll = number_bits(15)) >= 200)
	;

    if (MOUNTED(ch) && number_percent() < get_skill(ch, gsn_fight_on_mount))
	stop_fighting(MOUNTED(ch), FALSE);

    thac = URANGE(0 ,thac0 - victim_ac, 100); /* 50% максимум прикрыться броней*/

    if (!IS_NPC(ch) && IS_SET(ch->act, PLR_SHOW_DAMAGE))
	printf_to_char("{B(%d %d %d>%d) {x", ch, thac0, victim_ac, diceroll, thac);

    if (diceroll <= 10
	|| (diceroll <= 190 && diceroll < thac))
    {
	if (!IS_NPC(ch) && IS_SET(ch->act, PLR_SHOW_DAMAGE))
	    send_to_char("{B(Miss) {x", ch);

	/* Miss. */
	if (MOUNTED(ch))
	    check_improve (ch, victim, gsn_fight_on_mount, FALSE, 30);

/*Попортим броню, если попали
*/
	if (diceroll > 10
	&& number_bits(2) == 0 
	&& !is_lycanthrope(victim))
	    make_worse_condition(victim, 0 , number_bits(3) + 1, dam_type);

	damage(ch, victim, 0, dt, dam_type, TRUE, NULL);
	tail_chain();
	return FALSE;
    }
/*---------------------------------------------------------------------------*/

    if (MOUNTED(ch))
	check_improve(ch, victim, gsn_fight_on_mount, TRUE, 30);

    /*
     * Hit.
     * Calc damage.
     */
    if (IS_NPC(ch) && (!ch->pIndexData->new_format || wield == NULL))
    {
	if (!ch->pIndexData->new_format)
	{
	    dam = number_range(ch->level / 2, ch->level * 3 / 2);
	    if (wield != NULL)
		dam += dam / 2;
	}
	else
	    dam = dice(ch->damage[DICE_NUMBER], ch->damage[DICE_TYPE]);
    }
    else
    {
	if (sn > -1)
	    check_improve(ch, victim, sn, TRUE, 2);

	if (wield != NULL)
	{
	    if (wield->pIndexData->new_format)
		dam = dice(wield->value[1], wield->value[2]) * skill/100;
	    else
		dam = number_range(wield->value[1] * skill/100,
				   wield->value[2] * skill/100);

	    if (get_eq_char(ch, WEAR_SHIELD) == NULL)  /* no shield = more */
		dam = dam * 11/10;

	    
	    /* ОРУЖЕНИЕ ЗАТОЧЕНО x2-x4 урона */
	    if (IS_WEAPON_STAT(wield, WEAPON_SHARP))
	    {
		int percent;

		if ((percent = number_percent()) <= (skill / 8))
		    dam = 2 * dam + (dam * 2 * percent / 100);
	/* 2x-4x */ 
	    }
	    /*
	     * WEAPON_SLAY_*
	     * WEAPON_SLAY_GOOD & WEAPON_SLAY_EVIL удваивают повреждения против
	     * соотв. натуры
	     *
	     * WEAPON_SLAY_ANIMAL удваивает повреждения против зверей и друидов
	     * с егерями
	     *
	     * WEAPON_SLAY_UNDEAD удваивает повреждения против нежити и
	     * некромантов с назгулами
	     */
	    if (IS_WEAPON_STAT(wield, WEAPON_SLAY_EVIL) && IS_EVIL(victim))
		dam *= 1.3;

	    if (IS_WEAPON_STAT(wield, WEAPON_SLAY_GOOD) && IS_GOOD(victim))
		dam *= 1.3;

	    if (IS_WEAPON_STAT(wield, WEAPON_SLAY_ANIMAL) 
	      && ((IS_NPC(victim) && is_animal(victim))
		|| (!IS_NPC(victim) && (victim->classid == CLASS_DRUID || victim->classid == CLASS_RANGER))))
	    {
		dam *= 1.3;
	    }

	    if (IS_WEAPON_STAT(wield, WEAPON_SLAY_UNDEAD) 
	     && ((IS_NPC(victim) 
	            && (IS_SET(victim->act, ACT_UNDEAD) 
	             || IS_SET(victim->form, FORM_UNDEAD)))
	      || (!IS_NPC(victim)
		    && (victim->classid == CLASS_NECROMANT
		     || victim->classid == CLASS_NAZGUL
		     || victim->classid == CLASS_VAMPIRE))))
	    {
		dam *= 1.3;
	    }
	}
	else
	    dam = number_range(1 + 4 * skill/100, 2 * ch->level/3 * skill/100);
    }

    /*
     * Bonuses.
     */
    if (get_skill(ch, gsn_enhanced_damage) > 0)
    {
	diceroll = number_percent();
	if (diceroll <= get_skill(ch, gsn_enhanced_damage))
	{
	    check_improve(ch, victim, gsn_enhanced_damage, TRUE, 6);
	    dam += 2 * (dam * diceroll/300);
	}
    }

    if (!IS_AWAKE(victim))
    {
	dam *= 2;
    }
    else if (victim->position == POS_BASHED)
    {
	dam = dam * 7 / 4;
    }
    else if (victim->position < POS_FIGHTING)
    {
	dam = dam * 3 / 2;
    }

    if (wield != NULL)
    {
	if (dt == gsn_backstab || dt == gsn_dual_backstab)
	{
	    dam += ch->level + get_skill(ch, dt)/2;

		if (ch->size == SIZE_SMALL)
		{
		    dam += dam * ((str_app[get_curr_stat(ch, STAT_DEX)].todam - 5 
			+ (siz < -2 ? siz : 0)) * 10)/(130 + 2*(LEVEL_HERO - ch->level));
		}
		else if (ch->size == SIZE_LARGE)
		{
		    dam += dam * ((str_app[get_curr_stat(ch, STAT_STR)].todam - 5 
			+ (siz < 0 ? 7-siz : 0)) * 10)/(100 + 2*(LEVEL_HERO - ch->level));
		}
		else
		{
		    dam += dam * ((str_app[get_curr_stat(ch, STAT_DEX)].todam - 4) * 10) / 
			(120 + 2*(LEVEL_HERO - ch->level));
		    dam += dam * ((str_app[get_curr_stat(ch, STAT_STR)].todam - 4) * 10) / 
			(120 + 2*(LEVEL_HERO - ch->level));
		}
		if (siz > 0)
		    dam = dam * (100 - siz*7) / 100;

		if (wield->value[0] != WEAPON_DAGGER)
		{
		    dam /= 2;
		}
		
		dam =  dam * 4/5;
	}

	if (dt == gsn_ambush)
	{
	    dam += 2 * (ch->level + get_skill(ch, gsn_ambush));
	}

	if (dt == gsn_ambush_move)
	{
	    dam += (ch->level + get_skill(ch, gsn_ambush_move));
	}
    }

    if (dt != gsn_backstab && dt != gsn_dual_backstab)
	dam += dam * ((str_app[get_curr_stat(ch, STAT_STR)].todam - 5) * 10) / 100;

    dam += (GET_DAMROLL(ch)) * UMIN(100, skill)/100;

    dam += (get_leadership(ch, FALSE)) * UMIN(100, skill)/100;

/*Увеличиваем урон у оборотня в обличии зверя при потере хитов
*/
    if (is_lycanthrope(ch) &&
    (100*ch->hit/ch->max_hit < 90))
	dam += dam * UMIN(60, 90 - 100*ch->hit/ch->max_hit) / 80;

    if (dam <= 0)
	dam = 1;

    if (MOUNTED(ch))
	dam += dam * get_skill(ch, gsn_fight_on_mount) / 400;

    if (!IS_NPC(ch))
    {
	switch (fix_style(ch))
	{
	case (NORM_STYLE):
	    break;
	case (CARE_STYLE):
	    gsn_style_ch = gsn_care_style;
	    multiplier_ch = -1;
	    break;
	case (AGGR_STYLE):
	    gsn_style_ch = gsn_aggr_style;
	    multiplier_ch = 1;
	    break;
	default:
	    bugf("Unknown style!!!\n");
	    break;
	}

	if (gsn_style_ch != 0)
	    multiplier_ch = 10 + multiplier_ch
		* (get_skill (ch, gsn_style_ch) / 20);
    }

    if (!IS_NPC(victim))
    {
	switch (fix_style(victim))
	{
	case (NORM_STYLE):
	    break;
	case (CARE_STYLE):
	    gsn_style_victim = gsn_care_style;
	    multiplier_victim = -1;
	    break;
	case (AGGR_STYLE):
	    gsn_style_victim = gsn_aggr_style;
	    multiplier_victim = 1;
	    break;
	default:
	    bugf("Unknown style!!!\n");
	    break;
	}

	if (gsn_style_victim != 0)
	{
	    multiplier_victim = 10 + multiplier_victim
		* (get_skill (ch, gsn_style_victim) / 20);
	    check_improve (victim, ch, gsn_style_victim, TRUE, 26);
	}
    }


    dam *= (float) (multiplier_ch + multiplier_victim)/20;

	/* ВЕСЬ УРОН ПО ГНОМАМ С РУКИ -33% */
	if (victim->race == RACE_DWARF){
		dam *= 2;
		dam /= 3;
	}


    result = damage(ch, victim, dam, dt, dam_type, TRUE, wield);

    /* but do we have a funky weapon? */
    if (result != FALSE && wield != NULL)
    {
	int dam;

	if (ch->fighting == victim && IS_WEAPON_STAT(wield, WEAPON_POISON)
	 && (!IS_NPC(victim) || !IS_SET(victim->act, ACT_UNDEAD)))
	{
	    int level, po = 0, res = 0;
	    AFFECT_DATA *poison, af, *paf;

	    if ((poison = affect_find(wield->affected, gsn_poison)) == NULL)
		level = wield->level;
	    else
		level = poison->level;

	    if ((paf = affect_find(victim->affected, gsn_poison)) != NULL)
	    {
		po  = paf->modifier;
		res = check_immune(victim, DAM_POISON);
	    }

	    if (!saves_spell(level / 2, victim, DAM_POISON) && (number_percent() > (res - po * 10)) && (victim->race != RACE_ZOMBIE))
	    {
		send_to_char("{rТы чувствуешь, как яд проникает в твои вены.{x\n\r",victim);
		act("{r$n отравляется ядом.{x",victim, wield, NULL, TO_ROOM);

		af.where     = TO_AFFECTS;
		af.type      = gsn_poison;
		af.level     = level * 3/4;
		af.duration  = level / 2;
		af.location  = APPLY_STR;
		af.modifier  = -1;
		af.bitvector = AFF_POISON;
		af.caster_id = get_id(ch);
		affect_join(victim, &af);
	    }

	    /* weaken the poison if it's temporary */
	    if (poison != NULL)
	    {
		poison->level = UMAX(0, poison->level - 2);
		poison->duration = UMAX(0, poison->duration - 1);

		if (poison->level == 0 || poison->duration == 0)
		{
		    act("{WДействие яда на $p5 закончилось.{x", ch, wield, NULL, TO_CHAR);
		    affect_remove_obj(wield, poison);
		}
	    }
	}


	if (ch->fighting == victim && IS_WEAPON_STAT(wield, WEAPON_VAMPIRIC)
	    && !IS_VAMPIRE(victim) && (!IS_NPC(victim) || !IS_SET(victim->act, ACT_UNDEAD)))
	{
	    AFFECT_DATA *poison, af;

	    dam = number_range(1, wield->level / 5 + 1);

	    if (dam/2 > 0)
	    {
		act("$p вытягива$r жизнь из $n1.", victim, wield, NULL, TO_ROOM);
		act("Ты чувствуешь, как $p вытягива$r твою жизнь.",
		    victim, wield, NULL, TO_CHAR);
		ch->alignment = UMAX(-1000, ch->alignment - 1);
		ch->hit += dam/2;
	    }

	    damage(ch, victim, dam, 0, DAM_NEGATIVE, FALSE, NULL);
	    if ((poison = affect_find(victim->affected, gsn_poison)) != NULL
		&& !saves_spell(poison->level, ch, DAM_POISON) && (victim->race != RACE_ZOMBIE))
	    {
		send_to_char("{rТы чувствуешь, как яд проникает в твои вены.{x\n\r", ch);
		act("{r$n отравляется ядом.{x", ch, NULL, NULL, TO_ROOM);

		af.where     = TO_AFFECTS;
		af.type      = gsn_poison;
		af.level     = poison->level;
		af.duration  = poison->level/3;
		af.location  = APPLY_STR;
		af.modifier  = (number_percent() < 50) ? -1 : 0;
		af.bitvector = AFF_POISON;
		af.caster_id = get_id(ch);
		affect_join(ch, &af);
	    }
	}

	if (ch->fighting == victim && IS_WEAPON_STAT(wield, WEAPON_MANAPIRIC))
	{
	    dam = number_range(1, wield->level / 10 + 1);
	    damage(ch, victim, dam, 0, DAM_NEGATIVE, FALSE, NULL);
	    dam /= 2;
	    if (dam > 0 && victim->mana > dam)
	    {
		ch->mana += dam;
		victim->mana -= dam;
		act("$p вытягива$r энергию из $n1.", victim, wield, NULL, TO_ROOM);
		act("Ты чувствуешь, как $p вытягива$r твою энергию.",
		    victim, wield, NULL, TO_CHAR);
	    }

	}

	if (ch->fighting == victim && IS_WEAPON_STAT(wield, WEAPON_FLAMING))
	{
	    dam = number_range(1, wield->level / 4 + 1);
	    act("$p обжига$r $n3.", victim, wield, NULL, TO_ROOM);
	    act("$p обжига$r твою плоть.", victim, wield, NULL, TO_CHAR);
	    fire_effect((void *) victim, wield->level/2, dam, TARGET_CHAR, ch);
	    damage(ch, victim, dam, 0, DAM_FIRE, FALSE, NULL);
	}

	if (ch->fighting == victim && IS_WEAPON_STAT(wield, WEAPON_FROST))
	{
	    dam = number_range(1, wield->level / 6 + 2);
	    act("$p заморажива$r $n3.", victim, wield, NULL, TO_ROOM);
	    act("Ледяное касание $p1 замораживает тебя.",victim, wield, NULL, TO_CHAR);
	    cold_effect(victim, wield->level/2, dam, TARGET_CHAR, ch);
	    damage(ch, victim, dam, 0, DAM_COLD, FALSE, NULL);
	}

	if (ch->fighting == victim && IS_WEAPON_STAT(wield, WEAPON_SHOCKING))
	{
	    dam = number_range(1, wield->level/5 + 2);
	    act("$p бьет электрическим разрядом $n3.",victim, wield, NULL, TO_ROOM);
	    act("$p бьет тебя электрическим разрядом.",victim, wield, NULL, TO_CHAR);
	    shock_effect(victim, wield->level/2, dam, TARGET_CHAR, ch);
	    damage(ch, victim, dam, 0, DAM_LIGHTNING, FALSE, NULL);
	}
    }

    /* Немного попортить оружие */
    if (number_bits(5) == 0 && !is_lycanthrope(ch))
	make_worse_condition(ch, wn, number_bits(2) + 1, DAM_BASH);

    tail_chain();
    return result;
}

/* Magic prof coeff*/
static int mag_class[MAX_CLASS] =
{
    /*MAG CLE  THI  War  NEC  PAL  NAZ  DRU  RAN  VAM  ALH  LYC*/
     180, 180, 100, 100, 180, 150, 150, 170, 100, 170, 160, 160
};

int damage(CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt, int dam_type,
	    bool show, OBJ_DATA *weapon)
{

    int out_dam = 0;

    if (ch && weapon == NULL && dt > 0 && dt < TYPE_HIT)
    {
	if (IS_NPC(ch))
	    dam = dam * 110 / 100;
	else
	{
	    dam = dam * mag_class[ch->classid] / 100;
	    if (ch->race == RACE_VAMPIRE 
	    && (!IS_OUTSIDE(ch) || time_info.hour>=20 || time_info.hour<=6))
		dam = dam * 110 / 100;
	}
    }   

	out_dam = adv_dam(ch, victim, dam, dt, dam_type, show, weapon, FALSE, 0);
	
/*    return adv_dam(ch, victim, dam, dt, dam_type, show, weapon, FALSE, 0);
*/
	return out_dam;
}


/*********************************************************
* УМЕНИЕ ВОССТАНОВЛЕНИЕ (вскочить на ночи после падения) *
**********************************************************/
bool check_recovery(CHAR_DATA *victim, CHAR_DATA *ch)
{
    bool ret = TRUE;

/*    if (!IS_NPC(victim) && victim->position == POS_BASHED)
*/
    if (victim->position == POS_BASHED)
    {
		if (number_percent() < get_skill(victim, gsn_recovery)/2)
		{
	    	victim->position = POS_FIGHTING;
	    	send_to_char("{5Ты оправляешься от падения и вскакиваешь на ноги.{x\n\r", victim);
	    	act("{5$n оправляется от падения и вскакивает на ноги.{x", victim, NULL, NULL, TO_ROOM);
	    	check_improve(victim, ch, gsn_recovery, TRUE, 12);
	/*	    if (number_percent() < get_skill(victim, gsn_recovery))
	*/
	        victim->wait = 0; /*UMAX(0, victim->wait - skill_table[gsn_bash].beats);
*/
		}
		else
		{
	   		check_improve(victim, ch, gsn_recovery, FALSE, 12);
	   		ret = FALSE;
		}
    }
/*    else 
*       victim->position = POS_FIGHTING;
*/
    return ret;
}

/*
 * Inflict damage from a hit.
 */
int adv_dam(CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt, int dam_type,
	     bool show, OBJ_DATA *weapon, bool aff, int gsn)
{
    bool immune;
    char buff[MAX_STRING_LENGTH];
    int mater = 33;
    int dam_new;

    if (victim->position == POS_DEAD)
	return FALSE;

    if (!IS_VALID(ch) || !IS_VALID(victim))
	return FALSE;

    /* damage reduction */
    if (dam > 35)
	dam = (dam - 35)/2 + 35;
    if (dam > 80)
	dam = (dam - 80)/2 + 80;

    /*
     * Stop up any residual loopholes.
     */
    if (dam > 2000 && dt >= TYPE_HIT && !IS_IMMORTAL(ch))
    {
	OBJ_DATA *obj;
	OBJ_DATA *obj2;

	bugf("Damage: %d (%s vs %s): more than 2000 points!", dam, ch->name, victim->name);
	send_to_char("Хмм... Очень похоже на читерство.\n\r", ch);

	if ((obj = get_eq_char(ch, WEAR_WIELD)) != NULL)
	    extract_obj(obj, TRUE, TRUE);

	if ((obj2 = get_eq_char(ch, WEAR_SECONDARY)) != NULL)
	    extract_obj(obj2, TRUE, TRUE);
    }

    if (victim != ch)
    {
	/*
	 * Certain attacks are forbidden.
	 * Most other attacks are returned.
	 */

	if (!aff)
	{
	    /*фикс задравшей баги с двумя вызовами проверки is_safe	в храмах*/
	    if (IS_NPC(ch) || (!IS_SET(ch->in_room->room_flags, ROOM_HOLY) && !IS_SET(ch->in_room->room_flags, ROOM_GUILD)))
	    {		
		if (is_safe(ch, victim))
		    return FALSE;

		check_killer(ch, victim);
	    }

	    if (!IS_NPC(ch))
		SET_BIT(ch->act, PLR_EXCITED);


	    if (!IS_NPC(victim))
		SET_BIT(victim->act, PLR_EXCITED);

	    if (victim->position > POS_STUNNED)
	    {
		if (victim->fighting == NULL)
		{
		    set_fighting(victim, ch);

		    if (IS_NPC(victim) && has_trigger(victim->pIndexData->progs, TRIG_KILL, EXEC_ALL))
		    {
			p_percent_trigger(victim, NULL, NULL, ch,
					  NULL, NULL, TRIG_KILL);
			return TRUE;
		    }
		}
		
		if (victim->timer <= 4)
		    check_recovery(victim, ch);

		if (ch->fighting == NULL)
		    set_fighting(ch, victim);
	    }

	/*
	 * More charm stuff.
	 */
	    if (get_master(ch) == get_master(victim))
	    {       
	        if (ch->master)
		    stop_follower(ch);

	        if (victim->master)
		    stop_follower(victim);
	    }
	}


	/*
	 * Inviso attacks ... not.
	 */
	if (!aff && (IS_AFFECTED(ch, AFF_INVISIBLE) || IS_AFFECTED(ch, AFF_HIDE)
		     || IS_AFFECTED(ch, AFF_CAMOUFLAGE) || is_affected(ch,gsn_dissolve)))
	{
	    do_visible(ch, "fight");
	    act("{W$n проявляется.{x", ch, NULL, NULL, TO_ROOM);
	}
    }

    /*
     * Damage modifiers.
     */

    if (dam > 1 && IS_DRUNK(victim))
    {
	dam = 9 * dam / 10;
    }

    /* САНКА половинит урон */
    if (dam > 1 && IS_AFFECTED(victim, AFF_SANCTUARY))
	dam /= 2;

    /* ПРИЗРАНАЯ АУРА половинит урон */
    if (dam > 1 && is_affected(victim, gsn_ghostaura))
	dam /= 2;

    /* Под молитвой по ЗЛЫМ и МЕРТВЫМ +50% */
    if (dam > 1 && is_affected(ch, gsn_prayer) && (IS_EVIL(victim) || IS_UNDEAD(victim)))
	dam = dam * 3/2;

    /* Под гримуаром по ДОБРЫМ +50% */
    if (dam > 1 && is_affected(ch, gsn_grimuar) && IS_GOOD(victim))
	dam = dam * 3/2;



    // ЗАЩИТА ОТ ЗЛА и ЗАЩИТА ОТ ДОБРА -25% урона
    if (dam > 1 && ((IS_AFFECTED(victim, AFF_PROTECT_EVIL) && IS_EVIL(ch)) || (IS_AFFECTED(victim, AFF_PROTECT_GOOD) && IS_GOOD(ch))))
    {
	dam -= dam / 4;
    }

    immune = FALSE;

    /*
     * Check for parry, and dodge.
     */
    if (dt >= TYPE_HIT && ch != victim){
		if (check_mist(ch, victim))
			return FALSE;

		if (check_mirror_image(ch, victim))
			return FALSE;

		if (check_shield_block(ch, victim, weapon))
			return FALSE;

		if (check_dodge(ch, victim))
			return FALSE;



		if (dt != gsn_cleave){
			if (check_parry(ch, victim, weapon))
			return FALSE;
			if (check_fight_off(ch, victim))
			return FALSE;
		}
    }

    /*
     * Жуткий бред с dam_new введен для того, чтобы можно было поражать
     * иммунных к оружию, но уязвимых к серебру (например), тварей.
     */
    dam_new = dam - dam * check_immune(victim, dam_type) / 200;


    if (weapon != NULL)
    {
	int d = 0;

	mater = material_lookup(weapon->material);

	switch (mater)
	{
	case MAT_SILVER:
	    d = check_immune(victim, DAM_SILVER);
	    break;
	case MAT_WOOD:
	    d = check_immune(victim, DAM_WOOD);
	    break;
	    /*case MAT_STEEL:*/
	case MAT_IRON:
	    d = check_immune(victim, DAM_IRON);
	    break;
	}

	if (dam_type != DAM_PIERCE && dam_type != DAM_BASH && dam_type != DAM_SLASH)
	    d /= 2;

	dam_new -= ((dam_new == 0) ? dam * d : dam_new * d) / 100;
    }

    dam = UMAX(0, dam_new);

    /*понижение урона, ибо бьют больно шибко...
    */
    if (ch->level > 10 && dam > 4)
	dam = dam * 3/4;

    if (show)
	dam_message(ch, victim, dam, dt, immune, weapon);

    {	
	CHAR_DATA *orig;

	orig = IS_SWITCHED(ch) ? ch = CH(ch->desc) : ch;

	if (!IS_NPC(orig) && orig->pcdata)
	    orig->pcdata->dam_tick += dam;
    }    	



    if (dam == 0)
	return FALSE;

    /* Немножечко попортить броню */
    if (number_bits(2) == 0 && !is_lycanthrope(victim))
	make_worse_condition(victim, 0 , number_bits(3) + 1, dam_type);

    /*
     * Hurt the victim.
     * Inform the victim of his new state.
     */

    victim->hit -= dam;

    if (is_lycanthrope(ch)
	&& is_affected(ch,gsn_claws)
	&& ch->mana > 5)
    {
	int man = 1;

	if (!IS_NPC(victim))
	    man = 2;
	ch->mana -= man * (is_affected(ch, gsn_manaleak) ? 2 : 1);
	check_improve(ch, victim, gsn_claws, TRUE, 30);
    }

    if (!aff)
    {
	if (IS_AFFECTED(victim, AFF_FIRE_SHIELD))
	    shield_damage(ch, victim, dam, AFF_FIRE_SHIELD);

	if (!IS_VALID(ch))
	    return TRUE;

	if (IS_AFFECTED(victim, AFF_ICE_SHIELD))
	    shield_damage(ch, victim, dam, AFF_ICE_SHIELD);

	if (!IS_VALID(ch))
	    return TRUE;

	if (IS_AFFECTED(victim, AFF_HOLY_SHIELD))
	    shield_damage(ch, victim, dam, AFF_HOLY_SHIELD);

	if (!IS_VALID(ch))
	    return TRUE;

	if (IS_AFFECTED(victim, AFF_CONIFEROUS_SHIELD))
	    shield_damage(ch, victim, dam, AFF_CONIFEROUS_SHIELD);

	if (!IS_VALID(ch))
	    return TRUE;

	if (IS_AFFECTED(victim, AFF_DEATH_AURA))
	    shield_damage(ch, victim, dam, AFF_DEATH_AURA);

	if (!IS_VALID(ch))
	    return TRUE;

    }

    if (IS_NPC(victim)
	&& victim->pIndexData->vnum == MOB_VNUM_ZOMBIE
	&& !IS_SET(victim->act, ACT_DEAD_WARRIOR)
	&& dam > victim->level
	&& number_bits(4) == 0)
    {
	act("{R$n РАЗВАЛИВАЕТСЯ НА ОТДЕЛЬНЫЕ ЧАСТИ ТЕЛА!{x",
	    victim, 0, 0, TO_ROOM);
	send_to_char("{RТЫ РАЗВАЛИВАЕШЬСЯ НА КУСКИ!{x\n\r", victim);
	for_killed_skills(ch, victim);
	return TRUE;
    }

    if (!IS_NPC(victim)
	&& victim->level >= LEVEL_IMMORTAL
	&& victim->hit < 1)
    {
	victim->hit = 1;
    }

    if (victim->hit < 0)
    {
	victim->hit = -11;
    }

    update_pos(victim);

    switch (victim->position)
    {
    case POS_MORTAL:
	sprintf(buff, "$n смертельно ранен%s, и без помощи скоро умрет.",
		SEX_ENDING(victim));
	act(buff , victim, NULL, NULL, TO_ROOM);

	sprintf(buff, "Ты смертельно ранен%s, и без посторонней помощи "
		"скоро умрешь.\n\r", SEX_ENDING(victim));
	send_to_char(buff  , victim);
	break;

    case POS_INCAP:
	sprintf(buff, "$n выведен%s из строя, и без помощи медленно умрет.",
		SEX_ENDING(victim));
	act(buff , victim, NULL, NULL, TO_ROOM);

	send_to_char("Тебя вывели из строя, и без посторонней помощи ты "
		     "медленно умрешь.\n\r", victim);

	break;

    case POS_STUNNED:
	sprintf(buff, "$n оглушен%s, но может прийти в себя.\n\r",
		SEX_ENDING(victim));
	act(buff , victim, NULL, NULL, TO_ROOM);

	send_to_char("Тебя оглушили, но ты еще можешь прийти в себя.\n\r",
		     victim);
	break;

    case POS_DEAD:
	sprintf(buff, "$n МЕРТВ%s!!", capitalize(SEX_ENDING(victim)));
	act(buff , victim, 0, 0, TO_ROOM);

	if (ch == victim)
	    send_to_char("{RТЫ УМИРАЕШЬ!!{x\n\r\n\r", victim);
	else
	    send_to_char("{RТЕБЯ УБИЛИ!!{x\n\r\n\r", victim);
	break;

    default:
	if (dam > victim->max_hit / 4)
	    send_to_char("{RЭТО БЫЛО БОЛЬНО!{x\n\r", victim);
	if (victim->hit < victim->max_hit / 4)
	    send_to_char("{RТЫ ИСТЕКАЕШЬ КРОВЬЮ!{x\n\r", victim);
	break;
    }

    /*
     * Sleep spells and extremely wounded folks.
     */
    if (!IS_AWAKE(victim))
	stop_fighting(victim, FALSE);

    /*
     * Payoff for killing things.
     */
    if (victim->position == POS_DEAD)
    {
	group_gain(ch, victim);

	if (!IS_NPC(victim))
	{
	    int xp, mult;

	    sprintf(buff, "%s killed by %s at %d",
		    victim->name,
		    (IS_NPC(ch) ? ch->short_descr : ch->name),
		    ch->in_room ? ch->in_room->vnum : 0);
	    log_string(buff);

	    /*
	     * Dying penalty:
	     */
	    if (ch != victim && !is_in_war(ch, victim) && victim->in_room != NULL
		&& !IS_SET(victim->in_room->room_flags, ROOM_NOEXP_LOST))
	    {
		mult = (victim->level-group_level(ch, MAX_GROUP_LEVEL))/2;
		if (mult <= -5)
		    mult=0;
		else if (mult <= -2)
		    mult=1;
		else if (mult<=1)
		    mult=2;

		xp = mult * xp_compute(ch, victim,
				       group_level(ch, MAX_GROUP_LEVEL), FALSE, 1, 1);

		if (xp > 0)
		{
		    sprintf(buff, "{RТы теряешь %d %s опыта!{x\n\r",
			    xp, hours(xp, TYPE_POINTS));
		    send_to_char(buff, victim);
		}

		gain_exp(victim, 0 - xp, FALSE);

		/*Снимаем экспу с чарующего чара */
		if (!IS_NPC(victim) 
		&& IS_AFFECTED(victim, AFF_CHARM) 
		&& victim->master != NULL
		&& !IS_NPC(victim->master))
		{       
		    xp = (victim->master->level - victim->level) * 500;
		    if (xp > 3000) 	/* (разница больше 6 уровеней) 6 * 500 = 3000 */
		    {
			sprintf(buff, "{RМы в ответе за того, кого приручили. Ты теряешь %d %s опыта!{x\n\r",
				xp, hours(xp, TYPE_POINTS));
			send_to_char(buff, victim->master);
			gain_exp(victim->master, 0 - xp, FALSE);
		    }		    
		}
	    }
	}

	for_killed_skills(ch, victim);

	return FIGHT_DEAD;
    }

    if (victim == ch)
	return TRUE;

    /*
     * Take care of link dead people.
     */
    if (!IS_NPC(victim) && victim->desc == NULL)
    {
	if (number_range(0, victim->wait) == 0 && !aff)
	{
	    if (IS_NULLSTR(victim->lost_command))
		do_function(victim, &do_recall, "");
	    else
		interpret(victim, victim->lost_command);

	    return TRUE;
	}
    }

    /*
     * Wimp out?
     */
    if (!aff && victim->position > POS_SITTING)
    {
	if (IS_NPC(victim) && dam > 0 && victim->wait < PULSE_VIOLENCE / 2 &&
	    ((IS_SET(victim->act, ACT_WIMPY)
	      && number_bits(2) == 0
	      && victim->hit < victim->max_hit / 5)
	     || (IS_AFFECTED(victim, AFF_CHARM)
		 && victim->master != NULL
		 && victim->master->in_room != victim->in_room)))
	{
	    do_function(victim, &do_flee, "");
	}

	if (!IS_NPC(victim)
	    && victim->hit > 0
	    && victim->hit <= victim->wimpy
	    && victim->wait < PULSE_VIOLENCE / 2)
	{
	    do_function (victim, &do_flee, "");
	}
    }

    tail_chain();
    return TRUE;
}

bool is_safe(CHAR_DATA *ch, CHAR_DATA *victim)
{
    CHAR_DATA *orig = ch;
    ROOM_INDEX_DATA *room;

    if (victim->in_room == NULL || ch->in_room == NULL)
	return TRUE;

    room = victim->in_room;

    if (IS_SWITCHED(ch))
	ch = ch->desc->original;

    if (IS_SWITCHED(victim) && !IS_IMMORTAL(victim->desc->original))
	victim = victim->desc->original;

    if (IS_IMMORTAL(ch) && ch->level > LEVEL_HERO)
	return FALSE;

    if (victim->fighting == ch || victim == ch)
	return FALSE;

    /*
     if (IS_SET(room->room_flags, ROOM_SAFE))
     {
     send_to_char("Только не здесь.\n\r", ch);
     return TRUE;
     }
     */

    if (IS_SET(ch->comm, COMM_AFK))
    {
	send_to_char("А разве ты за клавиатурой???\n\r", ch);
	return TRUE;
    }

    if (!IS_SET(ch->act, PLR_AUTOATTACK) && !IS_NPC(ch))
    {
		send_to_char("Сначала включи режим атаки.\n\r", ch);
		return TRUE;
    }

    if (is_affected(ch, gsn_timeout) && (!IS_NPC(victim) || (victim->master != NULL && victim->master != ch)))
    {
	send_to_char("Дождись, пока твоя временная миролюбивость исчезнет.\n\r", ch);
	return TRUE;
    }

    if (!IS_NPC(ch) && (ch->pcdata->successed == 0) && (!IS_NPC(victim) || (victim->master != NULL && victim->master != ch)))
    {
		send_to_char("Сначала пройди одобрение у Богов. Набери {y? одобрение{x.\n\r", ch);
		return TRUE;
    }


    if (is_affected(ch, gsn_dissolve) && !IS_NPC(victim) && !IS_NPC(ch))
    {
	send_to_char("Прежде чем напасть на кого-то, тебе нужно собраться в нормальную форму.\n\r", ch);
	return TRUE;
    }

    if (victim->in_room && !IS_SET(victim->in_room->room_flags, ROOM_ARENA))
    {
	if (is_nopk(ch) || (IS_NPC(ch) && ch->master != NULL && is_nopk(ch->master)))
	{
	    if (!IS_NPC(victim) )
	    {
		send_to_char("Ты же не ПКиллер :-)\n\r", ch);
		return TRUE;
	    }

	    if (IS_NPC(victim) && victim->master != NULL && victim->master != ch)
	    {
		send_to_char("Это очара чара. Осторожнее :-)\n\r", ch);
		return TRUE;
	    }
	}
	else if (!IS_NPC(ch) && IS_NPC(victim) && is_nopk(get_master(victim)))
	{
	    send_to_char("Владельца этого моба защищают БОГИ!\n\r", ch);
	    return TRUE;
	}

	if (!IS_NPC(ch) && is_nopk(victim))
	{
	    send_to_char("Этого персонажа защищают БОГИ!\n\r", ch);
	    return TRUE;
	}

	if (is_nopk(victim) && IS_NPC(ch) && (RIDDEN(ch) || ch->mount))
	    return TRUE;	 
    }
    /*конец проверок на ноуПК*/


    if (!IS_NPC(ch))
    {
	if (victim->in_room && IS_SET(victim->in_room->room_flags, ROOM_HOLY) && IS_SET(ch->in_room->room_flags, ROOM_HOLY) && ch->fighting == NULL)
	{
		if (ch->level < 15)
		{
			send_to_char("Тебе еще рано нападать на других существ в храмах.\n\r", ch);
			return TRUE;
		}
		if (victim->level < 15)
		{
			send_to_char("Твоя жертва еще слишком мала, чтобы нападать на нее в храмах.\n\r", ch);
			return TRUE;
		}
	    send_to_char("Чем ты занимаешься в Святом месте??? Боги гневаются на тебя!\n\r", ch);
	    ch->count_holy_attacks++;
	}

	if (victim->in_room && IS_SET(victim->in_room->room_flags, ROOM_GUILD) && IS_SET(ch->in_room->room_flags, ROOM_GUILD) && ch->fighting == NULL)
	{   	
		if (ch->level < 15)
		{
			send_to_char("Тебе еще рано нападать на других существ в гильдиях.\n\r", ch);
			return TRUE;
		}
		if (victim->level < 15)
		{
			send_to_char("Твоя жертва еще слишком мала, чтобы нападать на нее в гильдиях.\n\r", ch);
			return TRUE;
		}
		send_to_char("Чем ты занимаешься в своей Гильдии?? Гильдмастер проклинает тебя!\n\r", ch);
	    ch->count_guild_attacks++;
	}
    }	

    /* killing mobiles */
    if (IS_NPC(victim))
    {
	if (IS_SWITCHED(victim))
	    return FALSE;

	if (victim->pIndexData->pShop != NULL)
	{
	    send_to_char("Торговцу это вряд ли понравится.\n\r", orig);
	    return TRUE;
	}

	/* no killing healers, trainers, etc */
	if (IS_SET(victim->act, ACT_TRAIN)
	    || IS_SET(victim->act, ACT_PRACTICE)
	    || IS_SET(victim->act, ACT_IS_HEALER)
	    || IS_SET(victim->act, ACT_IS_CHANGER)
	    || IS_SET(victim->act, ACT_BANKER)
	    || IS_SET(victim->act, ACT_QUESTER))
	{
	    send_to_char("Боги это не одобряют.\n\r", orig);
	    return TRUE;
	}
    }
    /* killing players */
    else
    {

	/* NPC doing the killing */
	if (IS_NPC(ch))
	{
	    /* charmed mobs and pets cannot attack players while owned */
	    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL
		&& ch->master->fighting != victim)
	    {
		send_to_char("Игроки - твои друзья!\n\r", orig);
		return TRUE;
	    }
	}
	/* player doing the killing */
	else
	{
	    if (ch->level <= PK_RANGE/2)
	    {
		send_to_char("Тебе еще рановато заниматься убийством игроков.\n\r", orig);
		return TRUE;
	    }

	    if (victim->level <= PK_RANGE/2)
	    {
		send_to_char("Дай возможность этому игроку подрасти немножко.\n\r", orig);
		return TRUE;
	    }

	    if (victim->fighting == ch)
		return FALSE;

/*временно поставил комментарий - пока не найдем способ не снимать экспу с того, на кого НАПАЛИ*/
/*	    if (IS_SET(victim->act, PLR_KILLER) || IS_SET(victim->act, PLR_THIEF))
		return FALSE;*/

	    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim
		&& victim->fighting != ch)
	    {
		act("$N - твой хозяин!", orig, NULL, victim, TO_CHAR);
		return TRUE;
	    }

/*запрещаем ПК вне кланов*/
	    if (!is_clan(ch))
	    {
		send_to_char("Присоединяйся к клану, если хочешь убивать других.\n\r", orig);
		return TRUE;
	    }

	    if (!is_clan(victim))
	    {
		act("Оставь этого игрока в покое - $E же не в клане.", orig, NULL, victim, TO_CHAR);
		return TRUE;
	    }



/*
	    if (!is_clan(ch) && is_clan(victim))
	    {
		send_to_char("Присоединяйся к клану, если хочешь убивать тех, кто в кланах.\n\r", orig);
		return TRUE;
	    }

	    if (!is_clan(victim) && is_clan(ch))
	    {
		act("Оставь этого игрока в покое - $E же не в клане.", orig, NULL, victim, TO_CHAR);
		return TRUE;
	    }
*/

	}
    }
    return FALSE;
}

bool is_safe_spell(CHAR_DATA *ch, CHAR_DATA *victim, bool area)
{
    CHAR_DATA *orig = ch;
    ROOM_INDEX_DATA *room;

    if (victim->in_room == NULL || ch->in_room == NULL)
	return TRUE;

    room = victim->in_room;

    if (IS_SWITCHED(ch))
	ch = ch->desc->original;

    if (IS_SWITCHED(victim) && !IS_IMMORTAL(victim->desc->original))
	victim = victim->desc->original;

    if (victim == ch && area)
	return TRUE;

    if (victim->fighting == ch || victim == ch)
	return FALSE;

    if (IS_IMMORTAL(ch) && ch->level > LEVEL_HERO && !area)
	return FALSE;

    /*
     if (IS_SET(room->room_flags, ROOM_SAFE))
     return TRUE;
     */

    if (IS_SET(ch->comm, COMM_AFK))
    {
	send_to_char("А разве ты за клавиатурой???\n\r", ch);
	return TRUE;
    }

    if (!IS_SET(ch->act, PLR_AUTOATTACK) && !IS_NPC(ch))
    {
		send_to_char("Сначала включи режим атаки.\n\r", ch);
		return TRUE;
    }

	if (is_affected(ch, gsn_timeout) && (!IS_NPC(victim) || (victim->master != NULL && victim->master != ch)))
	{
		send_to_char("Дождись, пока твоя временная миролюбивость исчезнет.\n\r", ch);
		return TRUE;
	}

	if (!IS_NPC(ch) && (ch->pcdata->successed == 0) && (!IS_NPC(victim) || (victim->master != NULL && victim->master != ch)))
	{
		send_to_char("Сначала получи одобрение у Богов. Набери {y? одобрение{x.\n\r", ch);
		return TRUE;
	}


    if (victim->in_room && !IS_SET(victim->in_room->room_flags, ROOM_ARENA))
    {
	if (is_nopk(ch)  || (IS_NPC(ch) && ch->master != NULL && is_nopk(ch->master)))
	{
	    if (!IS_NPC(victim) )
	    {
		send_to_char("Ты же не ПКиллер :-)\n\r", ch);
		return TRUE;
	    }

	    if (IS_NPC(victim) && victim->master != NULL && victim->master != ch)
	    {
		send_to_char("Это очара чара. Осторожнее :-)\n\r", ch);
		return TRUE;
	    }
	}
	else if (!IS_NPC(ch) && IS_NPC(victim) && is_nopk(get_master(victim)))
	{
	    send_to_char("Владельца этого моба защищают БОГИ!\n\r", ch);
	    return TRUE;
	}

	if (!IS_NPC(ch) && is_nopk(victim))
	{
	    send_to_char("Этого персонажа защищают БОГИ!\n\r", ch);
	    return TRUE;
	}
	/*конец проверок на ноуПК*/
    }


    if (!IS_NPC(ch))
    {
	if (victim->in_room && IS_SET(victim->in_room->room_flags, ROOM_HOLY) && IS_SET(ch->in_room->room_flags, ROOM_HOLY) && ch->fighting == NULL)
	{
		if (ch->level < 15)
		{
			send_to_char("Тебе еще рано нападать на других существ в храмах.\n\r", ch);
			return TRUE;
		}
		if (victim->level < 15)
		{
			send_to_char("Твоя жертва еще слишком мала, чтобы нападать на нее в храмах.\n\r", ch);
			return TRUE;
		}
		send_to_char("Чем ты занимаешься в Святом месте??? Боги гневаются на тебя!\n\r", ch);
	    ch->count_holy_attacks++;
	}

	if (victim->in_room && IS_SET(victim->in_room->room_flags, ROOM_GUILD) && IS_SET(ch->in_room->room_flags, ROOM_GUILD) && ch->fighting == NULL)
	{
		if (ch->level < 15)
		{
			send_to_char("Тебе еще рано нападать на других существ в гильдиях.\n\r", ch);
			return TRUE;
		}
		if (victim->level < 15)
		{
			send_to_char("Твоя жертва еще слишком мала, чтобы нападать на нее в гильдиях.\n\r", ch);
			return TRUE;
		}
		send_to_char("Чем ты занимаешься в своей Гильдии?? Гильдмастер проклинает тебя!\n\r", ch);
	    ch->count_guild_attacks++;
	}
    }

    /* killing mobiles */
    if (IS_NPC(victim))
    {
	if (IS_SWITCHED(victim))
	    return FALSE;

	/* safe room? */

	if (victim->pIndexData->pShop != NULL)
	    return TRUE;

	if (area && IS_WIZINVISMOB(victim))
	    return TRUE;

	/* no killing healers, trainers, etc */
	if (IS_SET(victim->act, ACT_TRAIN)
	    || IS_SET(victim->act, ACT_PRACTICE)
	    || IS_SET(victim->act, ACT_IS_HEALER)
	    || IS_SET(victim->act, ACT_IS_CHANGER)
	    || IS_SET(victim->act, ACT_BANKER)
	    || IS_SET(victim->act, ACT_QUESTER))
	{
	    return TRUE;
	}

	/* area effect spells do not hit other mobs */
	if (IS_NPC(ch) && area && !is_same_group(victim, ch->fighting))
	    return TRUE;
    }
    /* killing players */
    else
    {
	if (area && IS_IMMORTAL(victim) && victim->invis_level > LEVEL_HERO)
	    return TRUE;

	/* NPC doing the killing */
	if (IS_NPC(ch))
	{
	    /* charmed mobs and pets cannot attack players while owned */
	    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL
		&& ch->master->fighting != victim)
	    {
		return TRUE;
	    }
	}
	/* player doing the killing */
	else
	{
	    if (ch->level <= PK_RANGE/2)
	    {
		send_to_char("Тебе еще рановато заниматься убийством игроков.\n\r", orig);
		return TRUE;
	    }

	    if (victim->level <= PK_RANGE/2)
	    {
		send_to_char("Дай возможность этому игроку подрасти немножко.\n\r", orig);
		return TRUE;
	    }

	    if (victim->fighting == ch)
		return FALSE;

/*временно поставил комментарий - пока не найдем способ не снимать экспу с того, на кого НАПАЛИ*/
/*	    if (IS_SET(victim->act, PLR_KILLER) || IS_SET(victim->act, PLR_THIEF))
		return FALSE;*/

	    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim
		&& victim->fighting != ch)
	    {
		act("$N - твой хозяин!", orig, NULL, victim, TO_CHAR);
		return TRUE;
	    }

/*убираем ПК вне кланов*/

	    if (!is_clan(ch))
	    {
		send_to_char("Присоединяйся к клану, если хочешь убивать.\n\r", orig);
		return TRUE;
	    }

	    if (!is_clan(victim))
	    {
		act("Оставь этого игрока в покое - $E же не в клане.", orig, NULL, victim, TO_CHAR);
		return TRUE;
	    }


/*
	    if (!is_clan(ch) && is_clan(victim))
	    {
		send_to_char("Присоединяйся к клану, если хочешь убивать тех, кто в кланах.\n\r", orig);
		return TRUE;
	    }

	    if (!is_clan(victim) && is_clan(ch))
	    {
		act("Оставь этого игрока в покое - $E же не в клане.", orig, NULL, victim, TO_CHAR);
		return TRUE;
	    }
*/

	}

    }
    return FALSE;
}
/*
 * See if an attack justifies a KILLER flag.
 */
void check_killer(CHAR_DATA *ch, CHAR_DATA *victim)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *master;
    /*
     * Follow charm thread to responsible character.
     * Attacking someone's charmed char is hostile!
     */

    /*на арене киллер не вешается*/
    if (ch->in_room && IS_SET(ch->in_room->room_flags, ROOM_ARENA))
	return;

    /* Fix с перекинутыми друидами, чтобы когда на них нападал чар они убийцу не ловили */
    if (IS_SWITCHED(ch) && ch->fighting != victim)
	ch = CH(ch->desc);

    victim = get_master(victim);


    /*
     * NPC's are fair game.
     * So are killers and thieves.
     */
    if (IS_NPC(victim)
	|| IS_SET(victim->act, PLR_KILLER)
	|| IS_SET(victim->act, PLR_THIEF)
	|| is_in_war(ch, victim))
    {
	return;
    }

    /*
     * Charm-o-rama.
     */

    if ((master = get_master(ch)) != ch)
    {
	if (victim != master
	    && !IS_IMMORTAL(master)
	    && !IS_SET(master->act, PLR_KILLER)
	    && !is_in_war(master, victim))
	{
	    send_to_char("{R*** Ты теперь УБИЙЦА!! ***{x\n\r", ch->master);
	    SET_BIT(master->act, PLR_KILLER);
	    REMOVE_BIT(master->comm, COMM_HELPER);
	    sprintf(buf, "$N is attempting to murder %s", victim->name);
	    wiznet(buf, master, NULL, WIZ_FLAGS, 0, 0);
	    save_char_obj(master, FALSE);
	}

	/*	stop_follower(ch); */
	return;
    }

    /*
     * NPC's are cool of course (as long as not charmed).
     * Hitting yourself is cool too (bleeding).
     * So is being immortal (Alander's idea).
     * And current killers stay as they are.
     */
    if (IS_NPC(ch)
	|| ch == victim
	|| IS_IMMORTAL(ch)
	|| IS_SET(ch->act, PLR_KILLER)
	|| ch->fighting == victim)
    {
	return;
    }

    send_to_char("{R*** Ты теперь УБИЙЦА!! ***{x\n\r", ch);
    SET_BIT(ch->act, PLR_KILLER);
    REMOVE_BIT(ch->comm, COMM_HELPER);
    sprintf(buf, "$N is attempting to murder %s", victim->name);
    wiznet(buf, ch, NULL, WIZ_FLAGS, 0, 0);
    save_char_obj(ch, FALSE);

    return;
}

/* Parry coeff.s */
static float c_parry[MAX_WEAPON_TYPE][MAX_WEAPON_TYPE] =
{
    /*H2H, SWO, DAG, SPE, MAC, AXE, FLA, WHI, POL, STA, EXO */
    { 1.0, 0.1, 0.3, 0.3, 0.1, 0.1, 0.1, 0.5, 0.1, 0.2, 0.1 }, /* Hand2Hand */
    { 1.0, 1.1, 0.8, 0.7, 0.8, 1.0, 0.2, 0.1, 0.7, 0.9, 1.0 }, /* SWOrd     */
    { 1.0, 0.8, 1.1, 0.3, 0.4, 0.3, 0.1, 0.1, 0.1, 0.3, 1.0 }, /* DAGger    */
    { 1.0, 0.6, 0.3, 0.7, 0.4, 0.3, 0.4, 0.3, 0.7, 0.7, 1.0 }, /* SPEar     */
    { 1.0, 1.1, 0.5, 0.3, 1.1, 1.0, 0.1, 0.1, 0.5, 0.5, 1.0 }, /* MACe      */
    { 1.0, 1.0, 0.3, 0.5, 1.0, 1.1, 0.3, 0.1, 0.5, 0.9, 1.0 }, /* AXE       */
    { 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.2 }, /* FLAil     */
    { 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }, /* WHIp      */
    { 1.0, 0.9, 0.3, 0.5, 1.1, 1.0, 0.3, 0.3, 1.1, 1.0, 1.0 }, /* POLearm   */
    { 1.0, 1.0, 0.6, 0.8, 1.0, 0.8, 0.4, 0.4, 0.7, 1.1, 1.0 }, /* STAff     */
    { 1.0, 0.8, 1.1, 0.4, 0.6, 0.5, 0.4, 0.1, 0.8, 0.8, 1.1 }  /* EXOtic    */
};

/* Parry race favor weapon */
static int fwr_parry[MAX_WEAPON_TYPE][MAX_PC_RACE] =
{
    /*HUM  ELF  DWA  GiA  ORC  TRO  SNA  HOB  DRO  VAM LYC ZOM TIG*/
    { 0,   5,   0,   0,   3,   0,   0,   0,   0,   0,  0,  2, 0}, /* sword   */
    { 0,   0,   0,   -10, 3,   -10, 0,   5,   5,   0,  0,  3, 0}, /* dagger  */
    { 0,   0,   0,   0,   3,   0,   0,   0,   0,   0,  0,  2, 0}, /* spear   */
    { 0,   0,   0,   5,   3,   5,   0,   0,   0,   0,  0,  3, 0}, /* mace    */
    { 0,   0,   5,   0,   3,   0,   0,   0,   0,   0,  0,  1, 0}, /* axe     */
    { 0,   0,   0,   0,   3,   0,   0,   0,   0,   0,  0,  3, 0}, /* flail   */
    { 0,   0,   0,   0,   3,   0,   0,   0,   0,   0,  0,  2, 0}, /* whip    */
    { 0,   0,   0,   0,   3,   0,   0,   0,   0,   0,  0,  2, 0}, /* polearm */
    { 0,   0,   0,   0,   3,   0,   5,   0,   0,   0,  0,  1, 0}, /* staff   */
    { 0,   0,   0,   0,   3,   0,   0,   0,   0,   5,  5,  3, 0}  /* exotic  */
};

/* Parry prof coeff*/
static float prof_parry[MAX_CLASS] =
{
   /* MAG, CLE,  THI, War, NEC, PAL, NAZ, DRU, RAN, VAM,  ALH,  LYC*/
       0.9, 0.9, 1.1, 1.1, 0.9, 1.0, 1.0, 0.9, 1.1, 0.95, 0.9,  1.0
};

/*
 * Check for parry.
 */
bool check_parry(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *weapon)
{
    OBJ_DATA *obj;
    OBJ_DATA *obj2;
    int chance = 1, chance2 = 1;
    int random_chance;
    char buf[MAX_STRING_LENGTH];

    if (!IS_AWAKE(victim) || (chance = get_skill(victim, gsn_parry)) < 1)
	return FALSE;

    if (!IS_NPC(victim))
        chance *= prof_parry[victim->classid];

    obj = get_eq_char(victim, WEAR_WIELD);              /* Основаная пуха */
    obj2 = get_eq_char(victim, WEAR_SECONDARY);         /* Вторая Пуха */

    if (!IS_NPC(victim) && IS_NPC(ch) && ch->level > 45)
    	chance /= 1.5; /* Some fix */
    else
    	chance /= 2.5; /* Some fix */

    chance *= 0.9;

    chance += (victim->level - ch->level) * 2;
    chance += (get_curr_stat(victim, STAT_DEX) - get_curr_stat(ch, STAT_DEX)) * 2;
    chance += (get_curr_stat(victim, STAT_CON) - get_curr_stat(ch, STAT_CON));
    chance += (get_curr_stat(victim, STAT_STR) - get_curr_stat(ch, STAT_STR));

    chance *= get_style_multiplier(victim);
    chance *= get_style_multiplier(ch);

    chance *= is_lycanthrope(victim) ? 0.7 : 1;
    chance *= is_lycanthrope_spirit(victim) ? 0.9 : 1;

    if (!can_see(victim, ch))
	chance /= 1.6;

    if (weapon == NULL) {           // Нас бьют голыми руками, а мы с оружием
	    /* Парирование голых рук */
	    if ((obj != NULL && obj->item_type == ITEM_WEAPON) || (obj2 != NULL && obj2->item_type == ITEM_WEAPON))
		    return FALSE;
    } else {
	    /* Наше умение владеть оружием */
    	chance = chance * URANGE(90, get_weapon_skill(victim, weapon->value[0]), 100) / 100;

    	/*Если нет мастера второго оружия */
    	chance2 = chance * URANGE(80, get_skill(victim, gsn_secondary_mastery), 100) / 150; /* 0,53 - 0,66 парирование второй рукой */


        /* ПАРИНГ ОСНОВНОЙ ПУХОЙ*/
    	if ((obj != NULL) && (obj->item_type == ITEM_WEAPON)){
            if (obj->value[0] >= MAX_WEAPON_TYPE){
                bugf("victim's [%s] primary weapon [vnum %d] type (%d) >= MAX_WEAPON_TYPE (%d)",
                IS_NPC(victim) ? victim->short_descr : victim->name,
                obj->pIndexData->vnum, obj->value[0], MAX_WEAPON_TYPE);
                return FALSE;
            }

            if (weapon->value[0] >= MAX_WEAPON_TYPE){
                    bugf("aggressor's [%s] weapon [vnum %d] type (%d) >= MAX_WEAPON_TYPE (%d)",
                    IS_NPC(ch) ? ch->short_descr : ch->name,
                    weapon->pIndexData->vnum, weapon->value[0], MAX_WEAPON_TYPE);
                    return FALSE;
            }

            chance *= (float) get_weapon_skill(victim, get_weapon_sn(victim, FALSE)) / 100;	/* ???*/
            chance *= c_parry[obj->value[0]][weapon->value[0]];

            /*Если враг с двуручником, без второй пухи и без щита, а мы не с двуручником, то у нас -10% к парингу основной пухой - хрень полная насчет щита!!! */
            if (IS_WEAPON_STAT(weapon, WEAPON_TWO_HANDS) && get_eq_char(ch, WEAR_SECONDARY) == NULL	&& get_eq_char(ch, WEAR_SHIELD) == NULL	&& !IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS))
                    chance *= 0.9;

            /*Если у нас основаня пуха двуручная и нет втрой пухи и мы без щита*/
            if (IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS) && obj2 == NULL && get_eq_char(victim, WEAR_SHIELD) == NULL)
                    chance *= 1.3*(0.3 + (float) get_curr_stat(victim, STAT_STR) / MAX_STAT);

            chance += (get_obj_weight(obj) - get_obj_weight(weapon)) / 40;

            if (!IS_NPC(victim)){
                chance += fwr_parry[obj->value[0] - 1][victim->race - 1];
            }

                chance = UMIN(90, chance);

            /*	printf_to_char("{B(Паринг1 %d){x",victim,chance);*/

            random_chance = number_percent();
            sprintf(buf, "{B(Паринг: %d/%d) {x", chance, random_chance);

            if (random_chance < chance){
                /* Отладка для тестеров в immdamage*/
                if (!IS_NPC(victim) && IS_SET(victim->act, PLR_SHOW_DAMAGE)) {
                    strcat(buf, "Ты парируешь атаку $n1.");
                    act(buf,  ch, NULL, victim, TO_VICT  );
                } else {
                    act("Ты парируешь атаку $n1.",  ch, NULL, victim, TO_VICT  );
                }

                act("$N парирует твою атаку.", ch, NULL, victim, TO_CHAR  );
                act("$N парирует атаку $n1.", ch, NULL, victim, TO_NOTVICT  );
                check_improve(victim, ch, gsn_parry, TRUE, 5);
                return TRUE;
            } else {
                if (!IS_NPC(victim) && IS_SET(victim->act, PLR_SHOW_DAMAGE)) {
                    strcat(buf, "Неудачна попытка парировать атаку $n1.");
                    act(buf,  ch, NULL, victim, TO_VICT  );
                }
            }
    	}

        /* ПАРИНГ ВТОРОЙ ПУХОЙ*/
    	if ((obj2 != NULL) && (obj2->item_type == ITEM_WEAPON)){
            if (obj2->value[0] >= MAX_WEAPON_TYPE){
                    bugf("victim's [%s] primary weapon [vnum %d] type (%d) >= MAX_WEAPON_TYPE (%d)",
                    IS_NPC(victim) ? victim->short_descr : victim->name,
                    obj2->pIndexData->vnum, obj2->value[0], MAX_WEAPON_TYPE);
                    return FALSE;
            }

            if (weapon->value[0] >= MAX_WEAPON_TYPE){
                    bugf("aggressor's [%s] weapon [vnum %d] type (%d) >= MAX_WEAPON_TYPE (%d)",
                    IS_NPC(ch) ? ch->short_descr : ch->name,
                    weapon->pIndexData->vnum, weapon->value[0], MAX_WEAPON_TYPE);
                    return FALSE;
            }

            chance2 *= (float) get_weapon_skill(victim, get_weapon_sn(victim, TRUE)) / 120;
            chance2 *= c_parry[obj2->value[0]][weapon->value[0]];

            /*Если враг с двуручником, без второй пухи и без щита, а мы не с двуручником, то у нас -20% к парингу второй пухой - хрень полная, насчет щита!!! */
            if (IS_WEAPON_STAT(weapon, WEAPON_TWO_HANDS) && get_eq_char(ch, WEAR_SECONDARY) == NULL	&& get_eq_char(ch, WEAR_SHIELD) == NULL)
                    chance *= 0.8;

            chance2 += (get_obj_weight(obj2) - get_obj_weight(weapon)) / 30;

            if (!IS_NPC(victim)){
                chance2 += fwr_parry[obj2->value[0] - 1][victim->race - 1];
            }


                chance2 = UMIN(90, chance2);

        /*	printf_to_char("{B(Паринг2 %d){x",victim,chance2);*/
            random_chance = number_percent();
            sprintf(buf, "{B(Паринг вторым оружием: %d/%d) {x", chance, random_chance);

            if (random_chance < chance2){
                /* Отладка для тестеров в immdamage*/
                if (!IS_NPC(victim) && IS_SET(victim->act, PLR_SHOW_DAMAGE)) {
                    strcat(buf, "Ты парируешь атаку $n1.");
                    act(buf,  ch, NULL, victim, TO_VICT  );
                } else {
                    act("Ты парируешь атаку $n1.",  ch, NULL, victim, TO_VICT  );
                }

                act("$N парирует твою атаку.", ch, NULL, victim, TO_CHAR  );
                act("$N парирует атаку $n1.", ch, NULL, victim, TO_NOTVICT  );
                check_improve(victim, ch, gsn_parry, TRUE, 5);
                return TRUE;
            } else {
                if (!IS_NPC(victim) && IS_SET(victim->act, PLR_SHOW_DAMAGE)) {
                    strcat(buf, "Неудачна попытка парировать атаку $n1.");
                    act(buf,  ch, NULL, victim, TO_VICT  );
                }
            }
    	}

    }

    /* Эта проверка, только если нет ни основного, ни второго оружия. */
    /* Парирование голыми руками */
    if ((obj == NULL || obj->item_type != ITEM_WEAPON) && (obj2 == NULL || obj2->item_type != ITEM_WEAPON)){
        chance *= (float) get_skill(victim, gsn_hand_to_hand) / 130;
        if (weapon == NULL){
            chance *= c_parry[0][0];
        } else {
            chance *= c_parry[0][weapon->value[0]];
        }

        chance = UMIN(90, chance);

        /*	printf_to_char("{B(Паринг3 %d){x",victim,chance);*/
		random_chance = number_percent();
		sprintf(buf, "{B(Паринг голыми руками: %d/%d) {x", chance, random_chance);
        if (random_chance < chance){
			/* Отладка для тестеров в immdamage*/
			if (!IS_NPC(victim) && IS_SET(victim->act, PLR_SHOW_DAMAGE)) {
				strcat(buf, "Ты парируешь атаку $n1.");
				act(buf,  ch, NULL, victim, TO_VICT  );
			} else {
				act("Ты парируешь атаку $n1.",  ch, NULL, victim, TO_VICT  );
			}

            act("$N парирует твою атаку.", ch, NULL, victim, TO_CHAR  );
            act("$N парирует атаку $n1.", ch, NULL, victim, TO_NOTVICT  );
			check_improve(victim, ch, gsn_parry, TRUE, 5);
			return TRUE;
        }
    }

    return FALSE;
}

/*
 * Check for shield block.
 */

/* Shield block weight coeff.s */
static float w_block[MAX_WEAPON_TYPE][3] =
{
    /*0    1    2*/
    { 0.8, 0.7, 0.6 }, /* Hand   */
    { 0.8, 1.0, 1.0 }, /* SWOrd   */
    { 1.2, 0.8, 0.4 }, /* DAGger  */
    { 0.5, 0.8, 1.2 }, /* SPEar   */
    { 0.4, 0.9, 1.1 }, /* MACe    */
    { 0.4, 0.9, 1.0 }, /* AXE     */
    { 0.1, 0.5, 1.0 }, /* FLAil   */
    { 0.1, 0.4, 0.9 }, /* WHIp    */
    { 0.4, 0.7, 0.9 }, /* POLearm */
    { 0.5, 0.7, 1.1 }, /* STAff   */
    { 0.6, 1.2, 0.8 }  /* EXOtic  */
};

/* Shield block race coeff.s */
static float race_block[MAX_PC_RACE][3] =
{
    /*0    1    2*/
    { 1.0, 1.0, 1.0 }, /* Human   */
    { 1.2, 1.0, 0.8 }, /* Elf   */
    { 1.2, 1.0, 1.2 }, /* Dwarf  */
    { 0.8, 1.0, 1.2 }, /* Giant   */
    { 1.0, 1.0, 1.0 }, /* Orc    */
    { 0.8, 1.0, 1.2 }, /* Troll     */
    { 1.0, 1.0, 1.0 }, /* Snake   */
    { 1.2, 1.0, 0.8 }, /* Hobbit    */
    { 1.2, 1.0, 0.8 }, /* Drow */
    { 1.0, 1.0, 1.0 }, /* Vamp   */
    { 1.0, 1.0, 1.0 },  /* Lycan   */
    { 0.7, 1.0, 1.3 }, /* Zombie  */    
    { 1.0, 1.0, 1.0 } /* Tigguan */        
};


/* Shield block prof coeff*/
static float prof_block[MAX_CLASS] =
{
    /*MAG CLE  THI  War  NEC  PAL  NAZ  DRU  RAN  VAM  ALH  LYC*/
     1.0, 1.1, 0.8, 1.2, 1.0, 1.2, 1.2, 1.0, 0.8, 0.8, 1.0, 1.0
};

bool check_shield_block(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *weapon)
{
    int shn = 0, chance = 0;
    int random_chance;
    float dex_c, str_c;
    OBJ_DATA *shield;
    char buf[MAX_STRING_LENGTH];


    if (!IS_AWAKE(victim) || (chance = get_skill(victim, gsn_shield_block)) < 1)
        return FALSE;

    if ((shield = get_eq_char(victim, WEAR_SHIELD)) == NULL)
        return FALSE;

    if (!IS_NPC(victim))
        chance *= prof_block[victim->classid];

    chance += (victim->level - ch->level) * 2;

    chance /= 2;

/*Вводим коэффициент зависимости массы щита и типа оружия. 
*Разбиваем на 3 части. 0 легкий, 1 средний, 2 тяжелый.
*Ставится ограничение массы щита. Не более 599.
*На легкие щиты влияет больше ловкость, меньше сила
*На средние щиты влияет ловкость и сила одинаково
*На тяжелые щиты влияет меньше ловкость и больше сила
*/
    shn = UMIN(599, shield->weight) / 200;
    dex_c = 0.3 + (float) get_curr_stat(victim, STAT_DEX) / MAX_STAT;
    str_c = 0.3 + (float) get_curr_stat(victim, STAT_STR) / MAX_STAT;

    if (weapon != NULL){
        chance *= w_block[weapon->value[0]][shn];
    } else {
        chance *= w_block[0][shn];
    }

    chance *= race_block[victim->race - 1][shn];

    chance *= (float) (shn == 0 ? (2 * dex_c + str_c) / 3 : (shn == 1 ? (dex_c + str_c) / 2 : (dex_c + 2 * str_c) / 3));

    chance *= get_style_multiplier(victim);
    chance *= get_style_multiplier(ch);

    chance = UMIN(90, chance);

    if (!can_see(victim, ch))
        chance /= 1.5;

    /*	printf_to_char("{R(Блок %d){x",victim,chance);*/
    random_chance = number_percent();
    sprintf(buf, "{B(Блок: %d/%d) {x", chance, random_chance);

    if (random_chance >= chance){
        /* Для тестирования в immdamage*/
        if (!IS_NPC(victim) && IS_SET(victim->act, PLR_SHOW_DAMAGE)) {
            strcat(buf, "Неудачная попытка блокировать щитом атаку $n1.");
            act(buf, ch, NULL, victim, TO_VICT);
        }
        return FALSE;
    }



    
    /***************************************************************************
    * ВЫБИТЬ ЩИТ
    ***************************************************************************/
    if (weapon != NULL && (weapon->value[0] == WEAPON_AXE || weapon->value[0] == WEAPON_POLEARM
                           || weapon->value[0] == WEAPON_MACE || weapon->value[0] == WEAPON_FLAIL)){
        int dt, chance;
        AFFECT_DATA *paf;

        if (is_lycanthrope(ch)  && ((paf = affect_find(ch->affected, gsn_claws)) != NULL)){
            dt = paf->bitvector;
        } else {
            dt = attack_table[weapon->value[3]].damage;
        }

        if (dt == DAM_SLASH || dt == DAM_BASH || dt == DAM_PIERCE){
            chance = (((IS_WEAPON_STAT(weapon, WEAPON_TWO_HANDS) ? 3 : 2) * weapon->weight - shield->weight)/30 + get_curr_stat(ch, STAT_STR)/3)/check_material(shield, dt);
            chance = UMIN(3,chance);
            chance /= 1.5;

            if (number_percent() < chance ){
                if (!IS_OBJ_STAT(shield, ITEM_NOREMOVE)){

                    act("{WТы выбиваешь щит $N1!{x",  ch, NULL, victim, TO_CHAR  );
                    act("{W$n выбивает твой щит!{x", ch, NULL, victim,  TO_VICT  );
                    act("{W$n выбивает щит $N1!{x", ch, NULL, victim,  TO_NOTVICT );

                    obj_from_char(shield, TRUE);
                    if (IS_OBJ_STAT(shield, ITEM_NODROP) || IS_OBJ_STAT(shield, ITEM_INVENTORY)){
                        obj_to_char(shield, victim);
                    } else {
                        obj_to_room(shield, victim->in_room);
                        if (IS_NPC(victim) && victim->wait == 0 && can_see_obj(victim, shield))
                            get_obj(victim, shield, NULL, FALSE);
                    }
                    return FALSE;
                }
            }
        }
    }


    /*********************************************************************************
    * РАЗБИТЬ ЩИТ
    **********************************************************************************
    *
    *отключаем разбивание щита, взамен делаем его дизарм
    *
    *            act("{WТы разбиваешь щит $N1!{x",  ch, NULL, victim, TO_CHAR  );
    *            act("{W$n разбивает твой щит!{x", ch, NULL, victim,  TO_VICT  );
    *            act("{W$n разбивает щит $N1!{x", ch, NULL, victim,  TO_NOTVICT );
    *            extract_obj(shield, TRUE, TRUE);
    *            return FALSE;
    */

    /* Отладку для тестеров в immdamage, что бы видели какой % блока прошел*/
    if (!IS_NPC(victim) && IS_SET(victim->act, PLR_SHOW_DAMAGE)){
        strcat(buf, "Ты блокируешь щитом атаку $n1.");
        act(buf, ch, NULL, victim, TO_VICT);
    } else {
        act("Ты блокируешь щитом атаку $n1.",  ch, NULL, victim, TO_VICT  );
    }


    act("$N блокирует щитом твою атаку.", ch, NULL, victim,  TO_CHAR  );
    act("$N блокирует щитом атаку $n1.",  ch, NULL, victim, TO_NOTVICT  );
    check_improve(victim, ch, gsn_shield_block, TRUE, 5);
    return TRUE;
}


/* #######################################################################################
/*



/* Dodge race-race */
static int race_dodge[MAX_PC_RACE][MAX_PC_RACE] =
{
    /*HUM ELF DWA GiA ORC TRO SNA HOB DRO VAM LYC ZOM TIG*/
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 }, /* Human   */
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 }, /* Elf  */
    { 0,  0,  0,  5,  0,  5,  0,  0,  0,  0,  0,  0, 0 }, /* Dwarf   */
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 }, /* Giant    */
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 }, /* Orc     */
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 }, /* Troll   */
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 }, /* Snake    */
    { 0,  0,  0,  5,  0,  5,  0,  0,  0,  0,  0,  0, 0 }, /* Hobbit */
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 }, /* Drow   */
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },  /* Vampire  */
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },  /* Lycanthrope */
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },  /* Zombie  */
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 }  /* Tigguan */    
};


/* Dodge prof coeff*/
static float prof_dodge[MAX_CLASS] =
{
    /*MAG CLE  THI  War  NEC  PAL  NAZ  DRU  RAN  VAM  ALH  LYC*/
     0.9, 0.9, 1.1, 1.1, 0.9, 1.0, 1.0, 0.9, 1.1, 0.9, 0.9, 0.95
};

/*
 * Check for dodge.
 */
/* Skill Уворот */
bool check_dodge(CHAR_DATA *ch, CHAR_DATA *victim)
{
    int chance = 0;
    OBJ_DATA *shield;
	int random_chance;
	char buf[MAX_STRING_LENGTH];

    if (!IS_AWAKE(victim) || (chance = get_skill(victim, gsn_dodge)) < 1) return FALSE;

    if (MOUNTED(victim)) return FALSE;

    if (!IS_NPC(victim))
        chance *= prof_dodge[victim->classid];

    if (IS_NPC(victim))
    	chance /= 1.9; /* Some fix */
    else
    	if (IS_NPC(ch))
    		chance /= 1.3; /* Some fix */
	else
    		chance /= 2.2; /* Some fix */

/*Учитываем увертку чаров
*/
    chance += (get_curr_stat(victim, STAT_DEX) - get_curr_stat(ch, STAT_DEX)) * 2;   
    chance += (ch->size - victim->size);
    chance += (victim->level - ch->level);

/*Учитываем массу щита. Чем тяжелее, тем шанс меньше. Легкий, средний, тяжелый, сверхтяжелый.
*/
    if ((shield = get_eq_char(victim, WEAR_SHIELD)) != NULL)
        chance -= (1 + shield->weight / 200) * 5;

    chance *= get_style_multiplier(victim);
    chance *= get_style_multiplier(ch);

    chance *= is_lycanthrope(victim) ? 1.2 : 1;
    chance *= is_lycanthrope_spirit(victim) ? 1.1 : 1;

/*Вычитаем "попадание" противника
*/
    chance -= UMIN(20, GET_HITROLL(ch) / 10);

/*Разбиваем загрузку на 4 части. Легкая, средняя, тяжелая, перегруз.
 *Что здесь. Находим коэффициент загруженности и от 1.5 отнимаем его.
 *Коэффициент может равняться 0.3 ... 1.0 и т.д.
 *Соответственно 1.2, 1.1, ... 0.4 и т.д. */
    if (!IS_NPC(victim))
	chance *= (float) (150 - UMAX(30, (float) get_carry_weight(victim) * 100 / UMAX(1, can_carry_w(victim)))) / 100;

    if (IS_AFFECTED(victim, AFF_FLYING))
	chance *= 0.9;

    if (victim->race == RACE_VAMPIRE 
	&& (!IS_OUTSIDE(ch) || time_info.hour>=20 || time_info.hour<=6))
	chance *= 1.1;

/*Бонус при сражении раса-раса
*/
    if (!IS_NPC(victim) && !IS_NPC(ch))
        chance += race_dodge[victim->race - 1][ch->race - 1];

/*Подвязки к погоде
*/
    if (IS_OUTSIDE(ch) && weather_info.sky >= SKY_RAINING)
	chance *= 0.9;

    if (!can_see(victim, ch))
        chance /= 2;

    chance *= 0.9;

    chance = UMIN(90, chance);

/*	printf_to_char("{D(Увертка %d){x",victim,chance);
*/
    if (number_percent() >= chance)
        return FALSE;

    act("Ты уворачиваешься от атаки $n1.", ch, NULL, victim, TO_VICT  );
    act("$N уворачивается от твоей атаки.", ch, NULL, victim, TO_CHAR  );
    act("$N уворачивается от атаки $n1.", ch, NULL, victim, TO_NOTVICT  );
    check_improve(victim, ch, gsn_dodge, TRUE, 4);
    return TRUE;
}

bool check_fight_off(CHAR_DATA *ch, CHAR_DATA *victim)
{
    if (is_affected(victim, gsn_fight_off))
    {
	int wn, wd = 3;

	if (get_eq_char(victim, WEAR_WIELD) != NULL
	    && number_percent() < get_weapon_skill(victim,
						   get_weapon_sn(victim, FALSE)))
	{
	    wd--;
	}

	if (get_eq_char(victim, WEAR_SECONDARY) != NULL
	    && number_percent() < get_weapon_skill(victim,
						   get_weapon_sn(victim, TRUE)))
	{
	    wd--;
	}

	if (get_eq_char(victim, WEAR_SHIELD) != NULL)
	    wd--;

	if (wd == 0)
	    wd = 1;

	wn  = get_skill(victim, gsn_fight_off);

	wn += victim->level - ch->level;
	wn += get_curr_stat(victim, STAT_DEX) - get_curr_stat(ch, STAT_DEX);
	wn /= wd;

	wn *= get_style_multiplier(victim);

	if (number_percent() < wn)
	{
	    act("Ты отбиваешь атаку $N1!", victim, NULL, ch, TO_CHAR);
	    act("$N отбивает твою атаку!", ch, NULL, victim, TO_CHAR);
	    act("$N отбивает атаку $n1!", ch, NULL, victim, TO_NOTVICT);
	    check_improve(victim, ch, gsn_fight_off, TRUE, 2);
	    affect_strip(victim, gsn_fight_off);
	    return TRUE;
	}
	check_improve(victim, ch, gsn_fight_off, FALSE, 2);
    }

    return FALSE;
}


/*
 * Set position of a victim.
 */
void update_pos(CHAR_DATA *victim)
{
    if (victim->position > POS_SITTING)
	REMOVE_BIT(victim->affected_by, AFF_COVER);

    if (victim->hit > 0)
    {
	if (victim->position <= POS_STUNNED)
	    victim->position = POS_STANDING;
	return;
    }

    if (IS_NPC(victim) && victim->hit < 1)
    {
	victim->position = POS_DEAD;
	return;
    }

    if (victim->hit <= -11)
    {
	victim->position = POS_DEAD;
	return;
    }

    if (victim->hit <= -6)
	victim->position = POS_MORTAL;
    else if (victim->hit <= -3)
	victim->position = POS_INCAP;
    else
	victim->position = POS_STUNNED;

    if (MOUNTED(victim) && victim->position == POS_BASHED)
	do_dismount(victim, "update");

    return;
}



/*
 * Start fights.
 */
void set_fighting(CHAR_DATA *ch, CHAR_DATA *victim)
{
    if (IS_NPC(victim)
	&& !IS_NPC(ch)
	&& !IS_SET(victim->act, ACT_NOREMEMBER)
	&& !is_memory(victim, ch, MEM_HOSTILE)
	&& !IS_AFFECTED(victim, AFF_CHARM)
	&& !victim->master
	&& !is_affected(victim, gsn_shapechange))
    {
	mob_remember(ch, victim, MEM_HOSTILE);
    }

    if (ch->fighting != NULL)
	return;

    if (IS_AFFECTED(ch, AFF_SLEEP))
	affect_strip(ch, gsn_sleep);

    ch->fighting = victim;
    ch->position = POS_FIGHTING;

    if (MOUNTED(ch) && get_skill(ch, gsn_fight_on_mount) < 1)
	do_dismount(ch, "update");

    return;
}



/*
 * Stop fights.
 */
void stop_fighting(CHAR_DATA *ch, bool fBoth)
{
    CHAR_DATA *fch, *safe_fch;

    LIST_FOREACH_SAFE(fch, &char_list, link, safe_fch)
    {
	if (fch == ch || (fBoth && fch->fighting == ch))
	{
	    fch->fighting = NULL;

	    if (IS_NPC(fch))
		fch->position = fch->default_pos;
	    else
	    {
		fch->position = POS_STANDING;
		fch->on = NULL;
	    }

	    update_pos(fch);
	}
    }

    return;
}
#if 0
void start_fight(CHAR_DATA *ch, CHAR_DATA *victim)
{
    if (ch->fight == victim->fight)
	return;

    if (ch->fight)
	add_to_fight(ch->fight, victim, FALSE);
    else if (victim->fight)
	add_to_fight(victim->fight, ch, TRUE);
    else
	create_fight(ch, victim);

    update_pos(ch);
    update_pos(victim);
}

void create_fight(CHAR_DATA *ch, CHAR_DATA *victim)
{
    FIGHT_DATA *fd;

    fd = new_fight_data();
    fd->agressors = ch;
    fd->victims = victim;
    ch->fight = fd;
    victim->fight = fd;
}

void add_to_fight(FIGHT_DATA *fd, CHAR_DATA *ch, bool agr)
{
    if (agr)
    {
	ch->next_in_fight = fd->agressors;
	fd->agressors = ch;
    }
    else
    {
	ch->next_in_fight = fd->victims;
	fd->victims = ch;
    }
}

void stop_fight(CHAR_DATA *ch, CHAR_DATA *victim)
{

}
#endif /* 0 */


void create_artifact(CHAR_DATA *ch, OBJ_DATA *corpse)
{
    OBJ_DATA *obj;
    OBJ_INDEX_DATA *pObj;
    int i, j = 0, arf[ARTIFACT_MAX_VNUM - ARTIFACT_MIN_VNUM];

    if (!IS_NPC(ch) || number_bits(5) > 0)
	return;

    for (i = ARTIFACT_MIN_VNUM; i <= ARTIFACT_MAX_VNUM; i++)
	if ((pObj = get_obj_index(i)) != NULL && pObj->level <= ch->level  && (pObj->value[0] == 0 || number_percent() <= (pObj->value[0])*10))
	    arf[j++] = i;

    if (j > 0)
    {
	obj = create_object(get_obj_index(arf[number_range(0, j - 1)]), 0);
	obj_to_obj(obj, corpse);
    }
}

int create_ingredient(CHAR_DATA *ch, OBJ_DATA *target, OBJ_DATA *corpse, char *argument)
{
    OBJ_INDEX_DATA *pObj;
    int i, j = 0, k=0, ing[INGREDIENT_MAX_VNUM - INGREDIENT_MIN_VNUM],
	time = time_info.hour, time1, time2, chance = 0, skill = 0;
    MOB_INDEX_DATA *mob = NULL;
    
    if (target != NULL && corpse != NULL)
	return 0;

    if (ch != NULL)
	skill = get_skill(ch, gsn_collection);

    for (i = INGREDIENT_MIN_VNUM; i <= INGREDIENT_MAX_VNUM; i++)
	if ((pObj = get_obj_index(i)) != NULL)
	{
	    if (str_str(pObj->name,argument) != NULL)
		k++;

	    time1 = pObj->value[0] / 100;
	    time2 = pObj->value[0] - time1 * 100;
/*	    if ((time1 < time2 && time1 <= time && time2 > time) == FALSE
*	    && (time1 > time2 && (time1 <= time || time2 > time)) == FALSE)
*		continue;
*/
	    chance = pObj->value[4];

	    if (corpse == NULL && ch != NULL)
	    {
		if (argument[0] != '\0')
		    chance *= 1.1;
		else
		    chance *= 0.9;		

		chance *= (float)URANGE(70, 30 + skill , 130) / 100;
		chance += (get_curr_stat(ch, STAT_INT) - 20) * 3;

		if (target == NULL
		&& number_range(1, 200) <= chance
		&& IS_SET(pObj->value[1], 1 << ch->in_room->sector_type)
		&& IS_SET(pObj->value[3], 1 << weather_info.sky)
		&& (argument[0] == '\0' || str_str(pObj->name,argument) != NULL))
		    ing[j++] = i;

		if (target != NULL
		&& number_range(1, 200) <= chance * 1.5
		&& target->item_type == ITEM_CORPSE_NPC
		&& (mob = get_mob_index(target->value[4])) != NULL
		&& pObj->value[2] == mob->race
		&& str_str(pObj->name,argument) != NULL
		&& can_see_obj(ch, target)
		&& pObj->value[2] != 0
		&& !target->enchanted)
		     ing[j++] = i;
	    }
	    else
		if (corpse->item_type == ITEM_CORPSE_NPC
		&& number_range(1, 2000) <= chance
		&& (mob = get_mob_index(corpse->value[4])) != NULL
		&& IS_SET(pObj->value[3], 1 << weather_info.sky)
		&& pObj->value[2] != 0
		&& pObj->value[2] == mob->race)
		    ing[j++] = i;
	}    

    if (target != NULL)
	target->enchanted = TRUE;

    if (j > 0)
    {
        OBJ_DATA *obj = NULL;
	int iin;

	iin = number_range(0, j - 1);
	if (corpse != NULL)
	{
	    obj = create_object(get_obj_index(ing[iin]), 0);	    
	    obj->level = corpse->level;
	    obj_to_obj(obj, corpse);
	    return 0;
	}
	if (ch != NULL)
	{
	    chance = skill * (is_affected(ch, gsn_concentrate) ? 14 : 10) / 300;
	    chance = 1 + number_range(0, chance);

	    for (i = 0; i < chance; i++)
	    {
		obj = create_object(get_obj_index(ing[iin]), 0);
		act("$n находит $p6.",ch,obj,NULL,TO_ROOM);
		obj->level = ch->level * skill / 100;
		if (get_carry_weight(ch) < can_carry_w(ch))
		{
		    act("Твои усилия были ненапрасны, ты находишь $p6.",ch,obj,NULL,TO_CHAR);
		    obj_to_char(obj, ch);
		}
		else
		{
		    act("Ты находишь $p6, но бросаешь, так как не можешь нести столько.",
			ch,obj,NULL,TO_CHAR);
		    obj_to_room(obj, ch->in_room);
		}
	    }
	    check_improve(ch, NULL, gsn_collection, TRUE, 3);

	    if (target == NULL)
	    {
		AFFECT_DATA af;

		af.where = TO_ROOM_AFF;
		af.type = gsn_collection;
		af.level = (mob != NULL ? (ch->level + 3 * mob->level) / 4: ch->level);
		af.duration = ch->level / 15;
		af.modifier = 0;
		af.location = APPLY_NONE;
		af.bitvector = 0;
		affect_to_room(ch->in_room, &af);
	    }
	    return 2;
	}	        
    }
    else
    {
	if (corpse != NULL)
	    return 0;
	if (k > 0)
	{
	    send_to_char("У тебя не получается здесь что-то собрать.\n\r",ch);
	    check_improve(ch, NULL, gsn_collection, FALSE, 4);
    	    return 1;
	}
	else
	    send_to_char("Такого ингредиента не существует.\n\r",ch);	
    }    
    return 0;
}

/*
 * Make a corpse out of a character.
 * СОЗДАНИ ТРУПА ПРИ СМЕРТЕ
 */
/*******************
* ЛУТ УБИТОГО ЧАРА *
*******************/
void make_corpse(CHAR_DATA *ch, CHAR_DATA *killer)
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *corpse;
				/* ОБЪЕКТ ТРУП */
    OBJ_DATA *obj, *obj_next;
	
    char *name;
    bool flag;
    int num;

    if (IS_NPC(ch))
    {
	name		= ch->short_descr;
	corpse		= create_object(get_obj_index(OBJ_VNUM_CORPSE_NPC), 0);
	corpse->timer	= number_range(3, 6);
	if (ch->gold > 0 || ch->silver > 0)
	{
	    obj_to_obj(create_money(ch->gold, ch->silver, 0), corpse);
	    ch->gold = 0;
	    ch->silver = 0;
	}
	corpse->cost = 0;
	corpse->value[4] = ch->pIndexData->vnum;
    }
    else
    {
	name		= ch->name;
	corpse		= create_object(get_obj_index(OBJ_VNUM_CORPSE_PC), 0);
	corpse->timer	= IS_SET(ch->in_room->room_flags, ROOM_NOLOOT) ? 0 : number_range(25, 40);
	REMOVE_BIT(ch->act, PLR_CANLOOT);

	free_string(corpse->owner);
	corpse->owner = str_dup(ch->name);

	if (ch->gold > 0 || ch->silver > 0 || ch->pcdata->bronze > 0)
	{
	    obj_to_obj(create_money(ch->gold , ch->silver, 0), corpse);
	    obj_to_obj(create_money(0 , 0, ch->pcdata->bronze), corpse);
	    ch->gold = 0;
	    ch->silver = 0;
	    ch->pcdata->bronze = 0;
	}

	corpse->cost = 0;
    }

    corpse->when_killed = corpse->timer;
    corpse->who_kill  = killer ? str_dup(PERS(killer, killer, 0)) : "неизвестен";

    if (is_affected(ch, gsn_poison)
	|| is_affected(ch, gsn_plague)
	|| IS_AFFECTED(ch, AFF_POISON)
	|| IS_AFFECTED(ch, AFF_PLAGUE)
	|| (IS_NPC(ch) && IS_SET(ch->pIndexData->form, FORM_POISON)))
    {
	corpse->value[3] = ch->level;
    }
    else
	corpse->value[3] = 0;

/*
    if (!IS_UNDEAD(ch) && !IS_SET(ch->form, FORM_UNDEAD) && !is_animal(ch))
    {
	obj_next = create_object(get_obj_index(OBJ_VNUM_SOUL), 0);
	obj_next->timer = UMAX(1, ch->level/5);
	obj_next->level = ch->level;
	obj_next->value[0] = ch->alignment;
	obj_to_room(obj_next, ch->in_room);
    }
*/

    corpse->level = ch->level;

    corpse->value[0] = ch->parts;

    sprintf(buf, corpse->name, name);
    free_string(corpse->name);
    corpse->name = str_dup(buf);

    sprintf(buf, corpse->short_descr, cases(name, 1));
    free_string(corpse->short_descr);
    corpse->short_descr = str_dup(buf);

    sprintf(buf, corpse->description, cases(name, 1));
    free_string(corpse->description);
    corpse->description = str_dup(buf);

    flag = FALSE;


    /* ОДЕТЫЕ ОБЪЕКТЫ ПОДГРУЖАЕМ В ТРУП*/	
    for (obj = ch->carrying; obj != NULL; obj = obj_next)
    {
	OBJ_DATA *t_obj, *n_obj;

	obj_next = obj->next_content;

	flag = TRUE;

	obj_from_char(obj, FALSE);

	if (obj->item_type == ITEM_POTION)
	    obj->timer = number_range(500, 1000);

	if (obj->item_type == ITEM_SCROLL)
	    obj->timer = number_range(1000, 2500);

	REMOVE_BIT(obj->extra_flags, ITEM_VIS_DEATH);

	if (IS_SET(obj->extra_flags, ITEM_INVENTORY))
	    extract_obj(obj, FALSE, FALSE);
	else if (IS_SET(obj->extra_flags, ITEM_ROT_DEATH))
	{
	    for (t_obj = obj->contains; t_obj != NULL; t_obj = n_obj)
	    {
		n_obj = t_obj->next_content;
		obj_from_obj(t_obj);

		if (obj->wear_loc == WEAR_FLOAT)
		{
		    act("$p пада$r $x.", ch, t_obj, NULL, TO_ROOM);
		    obj_to_room(t_obj, ch->in_room);
		}
		else
		    obj_to_obj(t_obj, corpse);
	    }

	    extract_obj(obj, TRUE, TRUE);
	}
	else if (obj->wear_loc == WEAR_FLOAT || !IS_NULLSTR(obj->owner))
	{
	    act("$p пада$r $x.", ch, obj, NULL, TO_ROOM);
	    obj_to_room(obj, ch->in_room);
	}
	else
	{
	    for (t_obj = obj->contains; t_obj != NULL; t_obj = n_obj)
	    {
		n_obj = t_obj->next_content;

		if (!IS_NULLSTR(t_obj->owner))
		{
		    obj_from_obj(t_obj);

		    act("$p пада$r $x.", ch, t_obj, NULL, TO_ROOM);
		    obj_to_room(t_obj, ch->in_room);
		}
	    }

	    obj_to_obj(obj, corpse);
	}
    }

    create_artifact(ch, corpse);

    num = create_ingredient(ch, NULL, corpse, "");

    obj_to_room(corpse, ch->in_room);

    if (IS_NPC(ch) && !HAS_CORPSE(ch))
    {
	dump_container(corpse);
    }

    if (!IS_NPC(ch))
    {
	send_to_char("Беги за своим трупом.\n\r", ch);
    }

    if (!IS_UNDEAD(ch) && !IS_SET(ch->form, FORM_UNDEAD) && !is_animal(ch))
    {
	obj_next = create_object(get_obj_index(OBJ_VNUM_SOUL), 0);
	obj_next->timer = UMAX(1, ch->level/5);
	obj_next->level = ch->level;
	obj_next->value[0] = ch->alignment;
	obj_to_room(obj_next, ch->in_room);
    }


    return;
}



/*
 * Improved Death_cry contributed by Diavolo.
 */
void death_cry(CHAR_DATA *ch)
{
    ROOM_INDEX_DATA *was_in_room;
    char *msg, *msg_und;
    int door;
    int vnum;

    vnum = 0;
    msg = "Ты слышишь предсмертный вопль $n1.";
    msg_und = msg;

    if (!IS_NPC(ch) || HAS_CORPSE(ch))
    {
	switch (number_bits(4))
	{
	case  0:
	    msg     = "$n падает $x ... уже ТРУПОМ!";
	    msg_und = "$n падает $x ... уже просто прахом!";
	    break;
	case  1:
	    if (ch->material == 0)
	    {
		msg      = "Кровь брызгает на твою броню.";
		msg_und  = "Кровь брызгает на твою броню и тут же исчезает...";
		break;
	    }
	case  2 :
	    if (IS_VAMPIRE(ch))
	    {
		msg = "Отломленные клыки $n1 падают $x.";
		msg_und  =  msg;
		vnum = OBJ_VNUM_KLYK;
		break;
	    }
	case  3:
	    if (IS_SET(ch->parts, PART_GUTS))
	    {
		msg = "Кишки $n1 вываливаются $x.";
		msg_und = "Кишки $n1 вываливаются $x и обращаются в прах!";
		vnum = OBJ_VNUM_GUTS;
	    }
	    break;
	case  4:
	    if (IS_SET(ch->parts, PART_HEAD))
	    {
		msg  = "Голова $n1 отделяется от тела и падает $x.";
		msg_und  = "Голова $n1 отделяется от тела, но $x падает только прах...";
		vnum = OBJ_VNUM_SEVERED_HEAD;
	    }
	    break;
	case  5:
	    if (IS_SET(ch->parts, PART_HEART))
	    {
		msg  = "Сердце $n1 вываливается из груди.";
		msg_und  = "Сердце $n1 вываливается из груди... Хотя нет, $x упала только кучка пепла.";
		vnum = OBJ_VNUM_TORN_HEART;
	    }
	    break;
	case  6:
	    if (IS_SET(ch->parts, PART_ARMS))
	    {
		if (is_animal(ch))
		{
		    msg  = "Лапа $n1 отсекается от мертвого тела.";
		    msg_und  = "Лапа $n1 отсекается от мертвого тела и падает горсткой пепла $x.";
		    vnum = OBJ_VNUM_PAD;
		}
		else
		{
		    msg  = "Рука $n1 отсекается от мертвого тела.";
		    msg_und  = "Рука $n1 отсекается от мертвого тела и падает горсткой пепла $x.";
		    vnum = OBJ_VNUM_SLICED_ARM;
		}
	    }
	    break;
	case  7:
	    if (IS_SET(ch->parts, PART_LEGS))
	    {
		if (is_animal(ch))
		{
		    msg  = "Лапа $n1 отсекается от мертвого тела.";
		    msg_und  = "Лапа $n1 отсекается от мертвого тела и падает горсткой пепла $x.";
		    vnum = OBJ_VNUM_PAD;
		}
		else
		{
		    msg  = "Нога $n1 отсекается от мертвого тела.";
		    msg_und  = "Нога $n1 отсекается от мертвого тела и непонятно куда исчезает...";
		    vnum = OBJ_VNUM_SLICED_LEG;
		}
	    }
	    break;
	case 8:
	    if (IS_SET(ch->parts, PART_BRAINS))
	    {
		msg = "Голова $n1 раскалывается, и мозги брызгают на тебя.";
		msg_und = "Голова $n1 раскалывается, и мозги брызгают на тебя... Но, не долетев, растворяются в воздухе.";
		vnum = OBJ_VNUM_BRAINS;
	    }
	    break;
	case 9:
	    if (IS_SET(ch->parts, PART_TAIL))
	    {
		msg = "Отсеченный хвост $n1 падает возле мертвого тела.";
		msg_und = "Отсеченный хвост $n1 падает возле мертвого тела, и как будто просачивается внутрь мира...";
		vnum = OBJ_VNUM_TAIL;
	    }
	    break;
	case 10:
	    if (IS_SET(ch->parts, PART_EAR))
	    {
		msg = "Уши отсекаются от головы $n1.";
		msg_und = "Уши, даже не успев отсечься от головы $n1, изчезают прямо в воздухе.";
		vnum = OBJ_VNUM_EAR;
	    }
	    break;
	}
    }

    if (!IS_NPC(ch)
	|| !IS_SET(ch->act, ACT_UNDEAD)
	|| !IS_SET(ch->form, FORM_UNDEAD))
    {
	act(msg, ch, NULL, NULL, TO_ROOM);
    }
    else
    {
	vnum = 0;
	act(msg_und, ch, NULL, NULL, TO_ROOM);
    }

    if (vnum != 0)
    {
	char buf[MAX_STRING_LENGTH];
	OBJ_DATA *obj;
	char *name;

	name		= IS_NPC(ch) ? ch->short_descr : ch->name;
	obj		= create_object(get_obj_index(vnum), 0);
	obj->timer	= number_range(4, 7);

	sprintf(buf, obj->short_descr, cases(name, 1));
	free_string(obj->short_descr);
	obj->short_descr = str_dup(buf);

	sprintf(buf, obj->description, cases(name, 1));
	free_string(obj->description);
	obj->description = str_dup(buf);

	if (obj->item_type == ITEM_FOOD)
	{
	    if (IS_SET(ch->form, FORM_POISON))
		obj->value[3] = ch->level;
	    else if (!IS_SET(ch->form, FORM_EDIBLE))
		obj->item_type = ITEM_TRASH;
	}
	else if (obj->item_type == ITEM_WEAPON)
	{
	    /* arms, legs & tails */
	    obj->level = ch->level;
	    obj->value[1] = 1 + obj->level/20;
	    obj->value[2] = 1 + obj->level/10;

	    if (ch->size >= SIZE_LARGE)
	    {
		SET_BIT(obj->value[4], WEAPON_TWO_HANDS);
		obj->value[2] = obj->value[2] * 3 / 2;
	    }
	}

	obj_to_room(obj, ch->in_room);
    }

    was_in_room = ch->in_room;

    if (was_in_room != NULL)
    {
	for (door = 0; door <= 5; door++)
	{
	    EXIT_DATA *pexit;

	    if ((pexit = was_in_room->exit[door]) != NULL
		&& pexit->u1.to_room != NULL
		&& pexit->u1.to_room != was_in_room)
	    {
		ch->in_room = pexit->u1.to_room;
		act("Ты слышишь чей-то предсмертный вопль.", ch, NULL, NULL, TO_ROOM);
	    }
	}
	ch->in_room = was_in_room;
    }

    return;
}



void raw_kill(CHAR_DATA *victim, CHAR_DATA *killer, bool is_corpse)
{
	int i;
    CHAR_DATA *vch, *vch_next;
    AFFECT_DATA *paf, *paf_next;

    stop_fighting(victim, TRUE);
    death_cry(victim);

    check_auctions(victim, NULL, "потери его владельцем");
    
    if (victim != killer)
	recovery_lycanthrope(victim);

    if (!IS_NPC(victim) && (victim->pcdata->temp_RIP > 1))		   
/*    if (!IS_NPC(victim))
*/
    {
/*		victim->pcdata->temp_RIP = 0;
*/
		victim->pcdata->temp_RIP = 2;
		send_to_char("Тебя охватывает сияние и ты ощущаешь себя в храме.\n\r", victim);
		if (killer && !IS_NPC(killer))
	    	send_to_char("{WПеред тобой внезапно возникло ОСЛЕПИТЕЛЬНОЕ сияние.{x\n\r", killer);
    }
    else
    {
	if (is_corpse)
		if ((!IS_NPC(victim) && !(victim->pcdata->atheist)) || (IS_NPC(victim)))
		{
	 		make_corpse(victim, killer);	
		}
    }
    if (IS_NPC(victim))
    {
	CHAR_DATA *orig;
        
	victim->pIndexData->killed++;
	kill_table[URANGE(0, victim->level, MAX_LEVEL-1)].killed++;

	if (IS_SWITCHED(victim) && !IS_IMMORTAL((orig = victim->desc->original)))
	{
	    orig->mana = 1;
	    orig->hit	= 1;
	    WAIT_STATE(orig, 2 * PULSE_VIOLENCE);
	}

	extract_char(victim, TRUE);
	return;
    }

    LIST_FOREACH_SAFE(vch, &char_list, link, vch_next)
    {
	if (IS_NPC(vch) && is_memory(vch, victim, MEM_ANY))
	    mob_forget(vch, victim, MEM_ANY);

	if (victim->classid == CLASS_NECROMANT
	    && IS_NPC(vch)
	    && vch->master == victim
	    && IS_AFFECTED(vch, AFF_CHARM)
	    && (vch->pIndexData->vnum == MOB_VNUM_ZOMBIE || vch->pIndexData->vnum == MOB_BONE_SHIELD))
	{
	    act("{R$n РАЗВАЛИВАЕТСЯ НА ОТДЕЛЬНЫЕ ЧАСТИ ТЕЛА!{x",
		vch, 0, 0, TO_ROOM);
	    extract_char(vch, TRUE);
	}
    }
    
    for (paf = victim->affected; paf != NULL; paf = paf_next)
    {
	paf_next = paf->next;

	if (paf->type != gsn_gods_curse
    	&& paf->type != gsn_nocast
    	&& paf->bitvector != AFF_GODS_SKILL)
	{    
	    affect_remove(victim, paf);
	}
    }
    
    extract_char(victim, FALSE);

    victim->affected_by	= race_table[victim->race].aff;
    for (i = 0; i < 4; i++)
	victim->armor[i]= 100;
    victim->position	= POS_RESTING;
    victim->hit		= 1; /* UMAX(1, victim->hit); */
    victim->mana	= UMAX(1, victim->mana);
    victim->move	= UMAX(1, victim->move);
    victim->pcdata->condition[COND_HUNGER] = 30;
    victim->pcdata->condition[COND_THIRST] = 30;
    victim->pcdata->condition[COND_DRUNK] = 0;
    victim->pcdata->condition[COND_FULL] = 0;
    REMOVE_BIT(victim->act, PLR_EXCITED);
    victim->pcdata->quaff	= 0;

    /*    if (!IS_NPC(victim)
     && IS_VAMPIRE(victim)
     && victim->level <= (PK_RANGE/2) + 1)
     {
     AFFECT_DATA af;
     af.where = TO_AFFECTS;
     af.type = gsn_bite;
     af.level = victim->level;
     af.duration = 180;
     af.location = APPLY_NONE;
     af.modifier = 0;
     af.bitvector = AFF_SATIETY;
     affect_to_char(victim, &af);
     } */


    
	if (!IS_NPC(victim) && (victim->pcdata->atheist))
	{
		    char strsave[MAX_INPUT_LENGTH];
			send_to_char("\n\r\n\r{RПрощай...{x\n\r", victim);
	    	
			sprintf(strsave, "%s%s", PLAYER_DIR, capitalize(victim->name));
	    	wiznet("$N удаляется как атеист.", victim, NULL, 0, 0, 0);

	    	if (is_clan(victim))
				clan_table[victim->clan].count--;

	    	return_limit_from_char(victim, TRUE);
	    	do_function(victim, &do_quit, "delete");
	    	unlink(strsave);
	}
	else
	{
	    save_char_obj(victim, TRUE);		
	}			   
	
	return;
}



void group_gain(CHAR_DATA *ch, CHAR_DATA *victim)
{
    CHAR_DATA *gch, *gch_orig, *safe_gch;
    int xp;
    int members;
    int pc_members = 0;
    int group_levels;
    int highestlevel = 0;
    char buf[MAX_STRING_LENGTH];

    /*
     * Monsters don't get kill xp's or alignment changes.
     * P-killing doesn't help either.
     * Dying of mortal wounds or poison doesn't give xp to anyone!
     */
    if (victim == ch || !ch->in_room)
	return;

    members = 0;
    group_levels = 0;
    LIST_FOREACH(gch, &ch->in_room->people, room_link)
    {
	if (is_same_group(gch, ch))
	{
	    members++;
	    if (!IS_NPC(gch) && !IS_AFFECTED(gch, AFF_CHARM))
		pc_members++;

	    group_levels += (IS_NPC(gch) && !IS_SWITCHED(gch))? gch->level / 2 : gch->level;
	}
    }

    if (members == 0)
    {
	bugf("Group_gain: members == 0.");
	members = 1;
	group_levels = ch->level ;
    }

    highestlevel = group_level(ch, MAX_GROUP_LEVEL);

    LIST_FOREACH_SAFE(gch_orig, &ch->in_room->people, room_link, safe_gch)
    {
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;

	gch = gch_orig;

	if (!is_same_group(gch, ch))
	    continue;

	if (IS_NPC(gch))
	{
	    if (IS_SWITCHED(gch))
		gch = gch->desc->original;
	    else
		continue;
	}

	if (highestlevel - gch->level > 5)
	{
	    send_to_char("{RТебе еще рановато ходить в такой группе.{x\n\r", gch_orig);
	    continue;
	}

	xp = 0;

	if (!is_in_war(gch, victim) && !IS_AFFECTED(gch, AFF_CHARM))
	{
	    if (!IS_NPC(victim)
		&& victim->level < gch->level - 2
		&& !(IS_SET(victim->act, PLR_KILLER)
		     || IS_SET(victim->act, PLR_THIEF)))
	    {
		if (!IS_SET(ch->in_room->room_flags, ROOM_NOEXP_LOST))
		{
		    xp = UMIN(exp_per_level(gch, gch->pcdata->points),
			      (gch->level - victim->level)
			      * xp_compute(victim, gch, group_levels, FALSE, members, pc_members)/2 + 10);
		    xp = -xp;
		}
	    }
	    else
	    {
		xp = xp_compute(gch, victim, group_levels, TRUE, members, pc_members);

		if (IS_RENEGATE(gch))
		{
		    xp = 0;
		    sprintf(buf, "{RТы изменяешь своим принципам! Ведь ты же всегда был%s %s%s!{x\n\r",
			    SEX_ENDING(gch),
			    (gch->pcdata->orig_align < -333) ? "зл" :
			    (gch->pcdata->orig_align > 333)  ? "добр" : "нейтральн",
			    SEX_END_ADJ(gch));

		    send_to_char(buf, gch_orig);
		}

	    }
	}

	if (IS_SET(gch->act, PLR_NOEXP) && xp > 0)
	{
	    xp = 0;
	    send_to_char("{WУ ТЕБЯ ВКЛЮЧЕН РЕЖИМ, КОГДА ТЫ НЕ ПОЛУЧАЕШЬ ОПЫТА!{x\n\r", gch_orig);
	}

	if (!IS_NPC(ch) && ch->level >= 29 && ch->pcdata->successed == 0 )
	{
		send_to_char("Ты не получаешь опыта, пока не пройдешь одобрение у Богов. Набери {y? одобрение{x.\n\r", gch_orig);
		xp = 0;
	}

	if (gch_orig->race == victim->race)
	{
	    if (xp > 0)
	    {
		send_to_char("Ты убил своего собрата по расе?!\n\r", gch_orig);
		xp = 0;
	    }
	}
	else 
	{
	    if (!is_compatible_races(victim, gch_orig, TRUE))
	    {
		if (gch_orig->race == RACE_VAMPIRE)
		    xp *= 1.1;
		else
		    xp *= 2;

		if (xp >= 0)
		    send_to_char("Так держать! Смерть врагам расы!\n\r", gch_orig);
		else
		    send_to_char("Смерть врагам расы - хорошо! Но может стоит выбрать кого посильнее?\n\r", gch_orig);
	    }
	    if (gch_orig->race == victim->race)
		send_to_char("Ты позоришь свою расу!\n\r", gch_orig);
	}

	if (!IS_IMMORTAL(gch))
	{
	    if (xp == 0)
		strcpy(buf, "Ты не получаешь опыта.\n\r");
	    else if (xp > 0)
		sprintf(buf, "Ты получаешь %d %s опыта.\n\r", xp, hours(xp, TYPE_POINTS));
	    else
		sprintf(buf, "Ты теряешь %d %s опыта.\n\r", -xp, hours(-xp, TYPE_POINTS));

	    send_to_char(buf, gch_orig);
	    gain_exp(gch, xp, FALSE);
	    sprintf(buf, "{xДо следующего уровня осталось %d %s опыта.\n\r", TNL(gch), hours(TNL(gch), TYPE_POINTS));
	    send_to_char(buf, gch_orig);
	}


	for (obj = gch->carrying; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;

	    if (obj->wear_loc != WEAR_NONE)
		drop_bad_obj(gch, obj, TRUE);
	}

    }

    gch = get_master(ch);

    if (IS_NPC(victim))
    {
	if (gch->clan == victim->clan)
	    clan_table[gch->clan].rating -= victim->level;
	else if (check_clan_group(gch) && group_level(gch, AVG_GROUP_LEVEL) < victim->level + 3)
	    clan_table[gch->clan].rating += victim->pIndexData->rating;
    }
    else if (check_clan_group(gch))
    {
	if (is_in_war(gch, victim))
	{
	    clan_table[gch->clan].rating += victim->level;
	    clan_table[victim->clan].rating -= victim->level;
	}

	if (gch->clan == victim->clan)
	    clan_table[gch->clan].rating -= victim->level;
    }

    return;
}


/*
 * Compute xp for a kill.
 * Also adjust alignment of killer.
 * Edit this function to change xp computations.
 */
int xp_compute(CHAR_DATA *gch, CHAR_DATA *victim, int total_levels,
	       bool use_time, int members, int pc_members)
{
    int xp, base_exp;
    int level_range;
    int change;
    int time_per_level;
    int skill;
    CHAR_DATA *leader;

    total_levels=UMAX(1, total_levels);

    level_range = victim->level - gch->level;

    /* compute the base exp */
    switch (level_range)
    {
    default : 	base_exp =   0;		break;
    case -9 :	base_exp =   1;		break;
    case -8 :	base_exp =   2;		break;
    case -7 :	base_exp =   5;		break;
    case -6 : 	base_exp =   9;		break;
    case -5 :	base_exp =  11;		break;
    case -4 :	base_exp =  22;		break;
    case -3 :	base_exp =  33;		break;
    case -2 :	base_exp =  50;		break;
    case -1 :	base_exp =  66;		break;
    case  0 :	base_exp =  83;		break;
    case  1 :	base_exp =  99;		break;
    case  2 :	base_exp = 121;		break;
    case  3 :	base_exp = 143;		break;
    case  4 :	base_exp = 165;		break;
    }

    if (level_range > 4)
	base_exp = 160 + 20 * (level_range - 4);

    /* do alignment computations */

    if (!IS_SET(victim->act, ACT_NOALIGN) && use_time &&
	!(!IS_NPC(gch) && IS_NPC(victim) && gch->pcdata->questmob == victim->pIndexData->vnum)) /* Не менять алигн за квестовых мобов*/
    {
	int fix, align = gch->alignment - victim->alignment;

	if (align >  500)
	    align = (align-500);
	else if (align < -500)
	    align = (align+500);
	else
	    align = (-victim->alignment - gch->alignment) / 4;

	fix = base_exp * align / 500;

	if (gch->pcdata->orig_align <= 333 && gch->pcdata->orig_align >= -333)
	    fix /= 2;

	gch->alignment += fix;

	gch->alignment = URANGE(-1000, gch->alignment, 1000);

	if (class_table[gch->classid].valid_align == ALIGN_GOOD && gch->alignment < 0)
	    gch->alignment = 0;

	if (class_table[gch->classid].valid_align == ALIGN_EVIL && gch->alignment > 0)
	    gch->alignment = 0;
    }

    if (IS_NPC(victim) && IS_SET(victim->act, ACT_NOEXP))
	return 0;

    /* calculate exp multiplier */
    if (IS_SET(victim->act, ACT_NOALIGN))
	xp = base_exp;
    else if (gch->alignment > 500)  /* for goodie two shoes */
    {
	if (victim->alignment < -750)
	    xp = (base_exp *4)/3;

	else if (victim->alignment < -500)
	    xp = (base_exp * 5)/4;

	else if (victim->alignment > 750)
	    xp = base_exp / 4;

	else if (victim->alignment > 500)
	    xp = base_exp / 2;

	else if (victim->alignment > 250)
	    xp = (base_exp * 3)/4;

	else
	    xp = base_exp;
    }
    else if (gch->alignment < -500) /* for baddies */
    {
	if (victim->alignment > 750)
	    xp = (base_exp * 5)/4;

	else if (victim->alignment > 500)
	    xp = (base_exp * 11)/10;

	else if (victim->alignment < -750)
	    xp = base_exp/2;

	else if (victim->alignment < -500)
	    xp = (base_exp * 3)/4;

	else if (victim->alignment < -250)
	    xp = (base_exp * 9)/10;

	else
	    xp = base_exp;
    }
    else if (gch->alignment > 200)  /* a little good */
    {

	if (victim->alignment < -500)
	    xp = (base_exp * 6)/5;

	else if (victim->alignment > 750)
	    xp = base_exp/2;

	else if (victim->alignment > 0)
	    xp = (base_exp * 3)/4;

	else
	    xp = base_exp;
    }
    else if (gch->alignment < -200) /* a little bad */
    {
	if (victim->alignment > 500)
	    xp = (base_exp * 6)/5;

	else if (victim->alignment < -750)
	    xp = base_exp/2;

	else if (victim->alignment < 0)
	    xp = (base_exp * 3)/4;

	else
	    xp = base_exp;
    }
    else /* neutral */
    {
	if (victim->alignment > 500 || victim->alignment < -500)
	    xp = (base_exp * 4)/3;

	else if (victim->alignment < 200 && victim->alignment > -200)
	    xp = base_exp/2;

	else
	    xp = base_exp;
    }

    /* more exp at the low levels */
    if (gch->level < 6)
	xp = 10 * xp / (gch->level + 4);

    /* less at high */
    if (gch->level > 35)
	xp =  15 * xp / (gch->level - 20);

    /* reduce for playing time */

    /* compute quarter-hours per level */
    if (use_time)
    {
	time_per_level = 4 * (gch->played + (int)(current_time - gch->logon))
	    / (3600 * (gch->level > 0) ? gch->level : 1);

	time_per_level = URANGE(2, time_per_level, 12);
	if (gch->level < 15)  /* make it a curve */
	    time_per_level = UMAX(time_per_level, (15 - gch->level));
	xp = xp * time_per_level / 12;
    }

    /* adjust for grouping */
    change = (pc_members < 2) ? 10 : (15 + 3 * (pc_members - 2));

    xp = change * xp * gch->level/(10 * total_levels);

    if (pc_members > 1
	&& (leader = gch->leader) != NULL
	&& leader->in_room == gch->in_room
	&& (skill = get_skill(leader, gsn_leadership) > 0))
    {
	check_improve(leader, gch, gsn_leadership, TRUE, 5);
	xp += xp * number_range(skill/2, skill) / 400;
    }

    /* randomize the rewards */
    /*xp = number_range (xp * 3/5, xp * 5/3);*/

  xp = number_range (xp * 3/4, xp * 4/3);

/*делаем постоянное утроение
*/
    /*xp *= 2.75;*/

    xp *= 8;

/* утроение от имм квеста - не даем бешенного х27, чуток уменьшаем*/
    if (immquest.is_quest){
		xp *= immquest.double_exp;
		xp = xp*2/3;						/*  11,6      17,5 */
    }

	/*ДЛЯ 51 ЛВЛ СИЛЬНО УСЛОЖНЯЕМ ПРОКАЧКУ ЭКСПЫ*/
	if (gch->level >50){
		xp /= 3;
	};

    return xp;
}


void dam_message(CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt,
		 bool immune, OBJ_DATA *wield)
{
    char buf1[256], buf2[256], buf3[256];
    const char *vs;
    const char *vp;
    const char *vx;
    const char *attack;
    const char *gender;
    char punct;
    bool is_skill = FALSE;

    if (ch == NULL || victim == NULL)
	return;

    if (!IS_NPC(ch) && IS_SET(ch->act, PLR_SHOW_DAMAGE))
    {
	sprintf(buf1, "{W(%d) {x", dam);
	send_to_char(buf1, ch);
    }

    if (!IS_NPC(victim) && IS_SET(victim->act, PLR_SHOW_DAMAGE))
    {
	sprintf(buf1, "{R(%d) {x", dam);
	send_to_char(buf1, victim);
    }

    if (dam ==   0)
    {
	vs = " не задеваешь";
	vp = " не задевает";
	vx = " ";
    }
    else if (dam <=   4)
    {
	vs = " царапаешь";
	vp = " царапает";
	vx = ", едва касаясь,";
    }
    else if (dam <=   8)
    {
	vs = " легко задеваешь";
	vp = " легко задевает";
	vx = ", легко задевая,";
    }
    else if (dam <=  12)
    {
	vs = " поражаешь";
	vp = " поражает";
	vx = ", незначительно поражая,";
    }
    else if (dam <=  16)
    {
	vs = " легко ранишь";
	vp = " легко ранит";
	vx = ", легко раня,";
    }
    else if (dam <=  20)
    {
	vs = " ранишь";
	vp = " ранит";
	vx = ", раня,";
    }
    else if (dam <=  24)
    {
	vs = " сильно бьешь";
	vp = " сильно бьет";
	vx = ", сильно раня,";
    }
    else if (dam <=  28)
    {
	vs = " очень сильно бьешь";
	vp = " очень сильно бьет";
	vx = ", очень сильно раня,";
    }
    else if (dam <=  36)
    {
	vs = " потрошишь";
	vp = " потрошит";
	vx = ", потроша,";
    }
    else if (dam <=  40)
    {
	vs = " калечишь";
	vp = " калечит";
	vx = ", калеча,";
    }
    else if (dam <=  44)
    {
	vs = " изувечиваешь";
	vp = " изувечивает";
	vx = ", изувечивая,";
    }
    else if (dam <=  48)
    {
	vs = " расчленяешь";
	vp = " расчленяет";
	vx = ", расчленяя,";
    }
    else if (dam <=  56)
    {
	vs = " ИСТРЕБЛЯЕШЬ";
	vp = " ИСТРЕБЛЯЕТ";
	vx = ", ИСТРЕБЛЯЯ,";
    }
    else if (dam <=  60)
    {
	vs = " *** СОКРУШАЕШЬ ***";
	vp = " *** СОКРУШАЕТ ***";
	vx = " *** СОКРУШАЯ ***";
    }
    else if (dam <=  75)
    {
	vs = " *** ИСКАЛЕЧИВАЕШЬ ***";
	vp = " *** ИСКАЛЕЧИВАЕТ ***";
	vx = " *** ИСКАЛЕЧИВАЯ ***";
    }
    else if (dam <= 100)
    {
	vs = " === УНИЧТОЖАЕШЬ ===";
	vp = " === УНИЧТОЖАЕТ ===";
	vx = " === УНИЧТОЖАЯ ===";
    }
    else if (dam <= 125)
    {
	vs = " >>> ИЗНИЧТОЖАЕШЬ <<<";
	vp = " >>> ИЗНИЧТОЖАЕТ <<<";
	vx = " >>> ИЗНИЧТОЖАЯ <<<";
    }
    else if (dam <= 150)
    {
	vs = " <<< СТИРАЕШЬ В ПОРОШОК >>>";
	vp = " <<< СТИРАЕТ В ПОРОШОК >>>";
	vx = " <<< СТИРАЯ В ПОРОШОК >>>";
    }
    else if (dam <= 250)
    {
	vs = ", делая НЕМЫСЛИМЫЕ ВЕЩИ, поражаешь";
	vp = ", делая НЕМЫСЛИМЫЕ ВЕЩИ, поражает";
	vx = ", делая НЕМЫСЛИМЫЕ ВЕЩИ, ";
    }
    else if (dam <= 400)
    {
	vs = ", {Yраздирая ПЛОТЬ НА КУСКИ{x, поражаешь";
	vp = ", {Yраздирая ПЛОТЬ НА КУСКИ{x, поражает";
	vx = ", {Yраздирая ПЛОТЬ НА КУСКИ{x, ";
    }
    else if (dam <= 700)
    {
	vs = ", обретая {WСИЛУ {rРАЗРУШИТЕЛЯ{x, поражаешь";
	vp = ", обретая {WСИЛУ {rРАЗРУШИТЕЛЯ{x, поражает";
	vx = ", обретая {WСИЛУ {rРАЗРУШИТЕЛЯ{x, ";
    }
    else if (dam <= 1000)
    {
	vs = ", обретая {WСИЛУ {RАРМАГЕДДОНА{x, поражаешь";
	vp = ", обретая {WСИЛУ {RАРМАГЕДДОНА{x, поражает";
	vx = ", обретая {WСИЛУ {RАРМАГЕДДОНА{x, ";
    }
    else
    {
	vs = ", раздирая {RПЛОТЬ НА КУСКИ{x, поражаешь";
	vp = ", раздирая {RПЛОТЬ НА КУСКИ{x, поражает";
	vx = ", раздирая {RПЛОТЬ НА КУСКИ{x, ";
    }

    punct   = (dam <= 25) ? '.' : '!';

    if (dt >= 0 && dt < max_skills)
    {
	attack	= skill_table[dt].noun_damage;
	gender	= skill_table[dt].gender_damage;
	is_skill = TRUE;
    }
    else if (dt >= TYPE_HIT
	     && dt < TYPE_HIT + MAX_DAMAGE_MESSAGE)
    {
	attack	= attack_table[dt - TYPE_HIT].noun;
	gender	= attack_table[dt - TYPE_HIT].gender;
    }
    else
    {
	bugf("Dam_message: bad dt %d. Char: %s  Victim: %s  Weapon: %d",
	     dt, ch->name, victim->name, wield ? 0 : wield->pIndexData->vnum);
	dt  = TYPE_HIT;
	attack = attack_table[0].name;
	gender = attack_table[0].gender;
    }
    /* Очень много ветвлений 8)) Потом причешу... */
    if (dt == TYPE_HIT)
    {
	if (ch  == victim)
	{
	    sprintf(buf1, "$n%s себя%c{x", vp, punct);
	    sprintf(buf2, "Ты%s себя%c{x", vs, punct);
	}
	else
	{
	    if(dam <= 0 && number_percent() < 65)
	    {
		sprintf(buf1, "$n промахивается%c{x", punct);
		sprintf(buf2, "Ты промахиваешься%c{x", punct);
		sprintf(buf3, "$n не достает тебя%c{x", punct);
	    }
	    else
	    {
		sprintf(buf1, "$n%s $N3%c{x",  vp, punct);
		sprintf(buf2, "Ты%s $N3%c{x", vs, punct);
		sprintf(buf3, "$n%s тебя%c{x", vp, punct);
	    }
	}
    }
    else if (wield == NULL || wield->item_type != ITEM_WEAPON)
    {
	if (immune)
	{
	    if (ch == victim)
	    {
		sprintf(buf1, "$n совсем не задевает себя %s.{x", attack);
		sprintf(buf2, "К счастью, у тебя иммунитет к этому.{x");
	    }
	    else
	    {
		sprintf(buf1, "%s $n1 не причиняет никакого вреда $N2!{x",
			attack);
		sprintf(buf2, "%s %s не причиняет вреда $N2!{x",
			gender, attack);
		sprintf(buf3, "%s $n1 не причиняет тебе никакого вреда.{x",
			attack);
	    }
	}
	else
	{
	    if (ch == victim)
	    {
		sprintf(buf1, "$n%s себя%c{x", vp, punct);
		sprintf(buf2, "%s %s%s тебя%c{x", gender, attack, vp, punct);
	    }
	    else
	    {
		if (dam <= 0 && number_percent() < 75 && !is_skill)
		{
		    char *part = NULL;

		    if (IS_SET(ch->parts, PART_CLAWS))
			part = "когти";

		    else if (IS_SET(ch->parts, PART_FANGS)
			     || IS_SET(ch->parts, PART_TUSKS))
		    {
			part = "клыки";
		    }

		    if (part != NULL)
		    {
			sprintf(buf1, "%s $n1 проносятся мимо $N1%c{x",
				part, punct);
			sprintf(buf2, "Твои %s не достают $N1%c{x",
				part, punct);
			sprintf(buf3, "%s $n1 проносятся мимо тебя%c{x",
				part, punct);
		    }
		    else
		    {
			sprintf(buf1, "$n%s $N3%c{x",  vp, punct);
			sprintf(buf2, "%s %s%s $N3%c{x",
				gender, attack, vp, punct);
			sprintf(buf3, "%s $n1%s тебя%c{x", attack, vp, punct);
		    }
		}
		else
		{
		    sprintf(buf1, "$n%s $N3%c{x",  vp, punct);
		    sprintf(buf2, "%s %s%s $N3%c{x",
			    gender, attack, vp, punct);
		    sprintf(buf3, "%s $n1%s тебя%c{x", attack, vp, punct);
		}
	    }
	}
    }
    else
    {
	AFFECT_DATA *paf;
	int att = 0;
	const char *you, *to, *miss, *oth, *oth_case;
	int16_t  oc;

	if (is_lycanthrope(ch) 
	    && ((paf = affect_find(ch->affected, gsn_claws)) != NULL)
	    && ((att = attack_lookup_dam (paf->bitvector)) != 0)
	    && ch->mana > 5)
	{    
	    you = attack_table[att].msg_you;
	    to  = attack_table[att].msg_other;
	    miss= attack_table[att].msg_miss_you;
	    oth = attack_table[att].msg_miss_other;
	    oth_case = attack_table[att].miss_other_case;
	    oc	= 4;	
	}
	else
	{
	    you = attack_table[wield->value[3]].msg_you;
	    to  = attack_table[wield->value[3]].msg_other;
	    miss= attack_table[wield->value[3]].msg_miss_you;
	    oth = attack_table[wield->value[3]].msg_miss_other;
	    oth_case = attack_table[wield->value[3]].miss_other_case;
	    oc	= is_skill ? 4 : attack_table[wield->value[3]].obj_case;
	}

	if (immune)
	{
	    if (ch == victim)
	    {
		sprintf(buf1, "$n совсем не задевает себя %s.{x", attack);
		sprintf(buf2, "К счастью, у тебя иммунитет к этому.{x");
	    }
	    else
	    {
		sprintf(buf1, "%s $n1 не причиняет никакого вреда $N2!{x",
			attack);
		sprintf(buf2, "%s %s не причиняет вреда $N2!{x",
			gender, attack);
		sprintf(buf3, "%s $n1 не причиняет тебе никакого вреда.{x",
			attack);
	    }
	}
	else if (dam <= 0)
	{
	    if (number_percent() < 65 && oth)
	    {
		/* XXX
		                sprintf(buf1, "$p $n1 %s $N1%c{x", miss, punct);
		 sprintf(buf2, "$p %s $N1%c{x", miss, punct);
		 sprintf(buf3, "$p $n1 %s тебя%c{x", miss, punct);*/
		sprintf(buf1, oth,  oth_case);
		sprintf(buf2, miss);
		sprintf(buf3, oth, "тебя");
	    }
	    else
	    {
		sprintf(buf1, "$n промахивается%c{x", punct);
		sprintf(buf2, "Ты промахиваешься%c{x", punct);
		sprintf(buf3, "$n не достает тебя%c{x", punct);
	    }
	}
	else if (number_percent() < 65 && dam > 0 && you && to)
	{
	    /*	    sprintf(buf1, "$n%s %s $N1 $p4%c{x",  vx, to, punct);
	     sprintf(buf2, "Ты%s %s $N1 $p4%c{x",  vx, you, punct);
	     sprintf(buf3, "$n%s %s тебя $p4%c{x", vx, to, punct);*/
	    sprintf(buf1, to, vx, "$N3", punct);
	    sprintf(buf2, you, vx, punct);
	    sprintf(buf3, to, vx, "тебя", punct);
	}
	else
	{
	    sprintf(buf1, "%s $p%d $n1%s $N3%c{x", attack, oc, vp, punct);
	    sprintf(buf2, "%s %s $p%d%s $N3%c{x",  gender, attack, oc, vp, punct);
	    sprintf(buf3, "%s $p%d $n1%s тебя%c{x", attack, oc, vp, punct);
	}
    }
    if (ch == victim)
    {
	act(buf1, ch, wield, NULL, TO_ROOM);
	act(buf2, ch, wield, NULL, TO_CHAR);
    }
    else
    {
	act(buf1, ch, wield, victim, TO_NOTVICT);
	act(buf2, ch, wield, victim, TO_CHAR);
	act(buf3, ch, wield, victim, TO_VICT);
    }

    return;
}



/*
 * Disarm a creature.
 * Caller must check for successful attack.
 */
 
 /**************
 * ОБЕЗОРУЖИТЬ *
 **************/
void disarm(CHAR_DATA *ch, CHAR_DATA *victim, bool secondary)
{
    OBJ_DATA *obj, *obj1;

    obj = secondary
	? get_eq_char(victim, WEAR_SECONDARY)
	: get_eq_char(victim, WEAR_WIELD);
    obj1 = get_eq_char(victim, WEAR_SECONDARY);

    if (obj != NULL)
    {
	if (obj1 != NULL && number_range(1, 10) > 5)
	    obj = obj1;
    }
    else
    {
	if (obj1 != NULL)
	    obj = obj1;
	else
	{
	    send_to_char("Твой противник без оружия.", ch);
	    return;
	}
    }

    if (IS_OBJ_STAT(obj, ITEM_NOREMOVE))
    {
	act("{5$S оружие зажато намертво!{x", ch, NULL, victim, TO_CHAR);
	act("{5$n пытается обезоружить тебя, но твое оружие зажато намертво!{x",
	    ch, NULL, victim, TO_VICT);
	act("{5$n неудачно пытается обезоружить $N3.{x",
	    ch, NULL, victim, TO_NOTVICT);
	return;
    }

    act("{5$n ОБЕЗОРУЖИВАЕТ тебя, и $p вылета$r из твоих рук!{x",
	ch, obj, victim, TO_VICT  );
    act("{5Ты обезоруживаешь $N3!{x",  ch, NULL, victim, TO_CHAR  );
    act("{5$n обезоруживает $N3!{x",  ch, NULL, victim, TO_NOTVICT);

    obj_from_char(obj, TRUE);
    if (IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT(obj, ITEM_INVENTORY))
	obj_to_char(obj, victim);
    else
    {
	obj_to_room(obj, victim->in_room);
	if (IS_NPC(victim) && victim->wait == 0 && can_see_obj(victim, obj))
	    get_obj(victim, obj, NULL, FALSE);
    }

    return;
}


void do_berserk(CHAR_DATA *ch, char *argument)
{
    int chance, chance2, hp_percent;

    if ((chance = get_skill(ch, gsn_berserk)) < 1
		|| (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_BERSERK))
		|| (!IS_NPC(ch)
    	&&  ch->level < skill_table[gsn_berserk].skill_level[ch->classid]))
    {
		send_to_char("Твое лицо наливается кровью, но ничего не происходит.\n\r", ch);
		return;
    }  
    
    if (IS_AFFECTED(ch, AFF_BERSERK) || is_affected(ch, gsn_berserk))
    {
		send_to_char("Ты уже чувствуешь себя как бык, увидевший красную тряпку.\n\r", ch);
		return;
    }

    if (IS_AFFECTED(ch, AFF_CALM))
    {
		send_to_char("Ты чувствуешь себя слишком спокойно для этого.\n\r", ch);
		return;
    }

    if (ch->mana < 50)
    {
		send_to_char("У тебя не хватает энергии.\n\r", ch);
		return;
    }

    /*Новое. Сохраняем значение прокачки умения
    */
    chance2 = chance;

    /* modifiers */
    /* fighting */
    if (ch->position == POS_FIGHTING)
		chance += 10;

    if (ch->position == POS_BASHED)
    {
		chance += 15;
		ch->position = (ch->fighting == NULL) ? POS_STANDING : POS_FIGHTING;
    }

    /* damage -- below 50% of hp helps, above hurts */
    hp_percent = 100 * ch->hit/ch->max_hit;
    chance += 25 - hp_percent/2;

    chance /= get_style_multiplier(ch);

    if (number_percent() < chance)
    {
		AFFECT_DATA af;

		WAIT_STATE(ch, PULSE_VIOLENCE);
		ch->mana -= 50;
		ch->move -= abs(ch->move/2);

/* heal a little damage */
		ch->hit += ch->level * 2;
		ch->hit = UMIN(ch->hit, ch->max_hit);

		if (IS_SET(ch->parts, PART_EYE))
		{
    		    send_to_char("{5Твои глаза наливаются кровью!{x\n\r", ch);
    		    act("{5Глаза $n1 светятся диким огнем!{x", ch, NULL, NULL, TO_ROOM);
		}
		else
		{
    		    send_to_char("{5Ты впадаешь в бешенство!{x\n\r", ch);
    		    act("{5$n впадает в бешенство!{x", ch, NULL, NULL, TO_ROOM);
		}
		
		check_improve(ch, NULL, gsn_berserk, TRUE, 2);

		af.where = TO_AFFECTS;
		af.type = gsn_berserk;
		af.level = ch->level;
		af.duration = number_fuzzy(ch->level / 8);
		af.modifier = UMAX(1, ch->level/5);
		af.bitvector = AFF_BERSERK;
		af.caster_id = get_id(ch);

		af.location = APPLY_HITROLL;
		affect_to_char(ch, &af);

		af.location = APPLY_DAMROLL;
		affect_to_char(ch, &af);

		af.location = APPLY_SAVES;
		af.modifier = 0 - UMAX(1, ch->level/10);
		affect_to_char(ch, &af);

		af.modifier = UMAX(10, 2 * ch->level);
		af.location = APPLY_AC;
		affect_to_char(ch, &af);

/*Новое. С вероятностью половины прокаченного умения убираем эффекты
*корни, очарование 
*/
		if (number_percent() < chance2 / 2)
		{
         	    if  (!IS_NPC(ch)&& IS_AFFECTED(ch, AFF_CHARM))
    		    {
  			send_to_char("{WВ порыве ярости твое сознание проясняется!{x\n\r", ch);
  			act("{W$n взбесился и больше не подчиняется тебе!{x", ch, NULL, ch->leader, TO_VICT);
        		REMOVE_BIT(ch->affected_by, AFF_CHARM);
			affect_strip(ch, gsn_charm_person);
			ch->master = NULL;
    			ch->leader = NULL;
		    }
			
		    if  (IS_AFFECTED(ch, AFF_ROOTS)) 
          	    {
  			send_to_char("{WВ порыве дикой ярости, ты разрываешь корни, опутывающие твои ноги!{x\n\r", ch);
        		act("{W$n в дикой ярости разрывает корни, опутывающие $s ноги!{x", ch, NULL, NULL, TO_ROOM);
           		affect_strip(ch, skill_lookup("roots"));
			REMOVE_BIT(ch->affected_by, AFF_ROOTS);
		    }
 		}
    }
    else
    {
		WAIT_STATE(ch, 3 * PULSE_VIOLENCE);
		ch->mana -= 25;
		ch->move -= abs(ch->move/4);

		send_to_char("Твой пульс учащается, но ничего не происходит.\n\r", ch);
		check_improve(ch, NULL, gsn_berserk, FALSE, 2);
    }
}


/********
* ТОЛЧОК
*********/
void do_bash(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim, *ride;
    OBJ_DATA *obj;
    int dam;
    int chance;

    one_argument(argument, arg);

    if ((chance = get_skill(ch, gsn_bash)) == 0
	|| (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_BASH)))
    {
	send_to_char("Толкать? Это как?\n\r", ch);
	return;
    }


    if (MOUNTED(ch))
    {
	send_to_char("Ты не можешь толкаться верхом!\n\r", ch);
	return;
    }

    if (arg[0] == '\0')
    {
	victim = ch->fighting;
	if (victim == NULL || victim->position == POS_DEAD)
	{
	    send_to_char("Но ты ни с кем не сражаешься!\n\r", ch);
	    return;
	}
    }
    else if ((victim = get_char_room(ch, NULL, arg, TRUE)) == NULL)
    {
	if (!too_many_victims)
	    send_to_char("Здесь таких нет.\n\r", ch);
	return;
    }

    if (victim->race == RACE_DWARF)
    {
	send_to_char("По гному-то? Толкать? А скалу толкать не попробовать?\n\r", ch);
	return;
    }


    if (victim->position < POS_FIGHTING)
    {
	act("Позволь $M сначала встать.", ch, NULL, victim, TO_CHAR);
	return;
    }

    if (MOUNTED(victim))
    {
	send_to_char("Ты не можешь толкнуть того, кто сидит на лошади.\n\r", ch);
	return;
    }

    if (!IS_SET(victim->parts, PART_LEGS))
    {
	send_to_char("Как можно сбить с ног существо, их не имеющее?!\n\r", ch);
	return;
    }

    ride = RIDDEN(victim);

    if (victim == ch)
    {
		send_to_char("Ты пытаешься пнуть свою задницу, но у тебя не получается.\n\r", ch);
		return;
    }

    if (is_safe(ch, victim))
	return;

    /*    if (IS_NPC(victim) &&
     victim->fighting != NULL &&
     !is_same_group(ch, victim->fighting))
     {
     send_to_char("Kill stealing is not permitted.\n\r", ch);
     return;
     }  */

    if (check_charmees(ch, victim))
	return;

    /* modifiers */

    chance -= get_skill(victim, gsn_bash) / 3;

    /* size  and weight */
    chance += get_carry_weight(ch) / 500;
    chance -= (get_carry_weight(victim) + (ride ? get_carry_weight(ride) : 0))/ 200;

    if (ch->size < victim->size)
	chance += (ch->size - victim->size) * 20;
    else
	chance += (ch->size - victim->size) * 10;

    /* stats */
    chance += 2 * (get_curr_stat(ch, STAT_DEX) - get_curr_stat(victim, STAT_DEX));
    chance += GET_AC(victim, AC_BASH)/20;

    dam = GET_DAMROLL(ch) / 4;

    if ((obj = get_eq_char(ch, WEAR_SHIELD)) != NULL)
    {
	chance += 15;
	dam += obj->weight / 10;
    }

    /* speed  */
    if (IS_SET(ch->off_flags, OFF_FAST) || IS_AFFECTED(ch, AFF_HASTE))
	chance += 10;
    if (IS_SET(victim->off_flags, OFF_FAST) || IS_AFFECTED(victim, AFF_HASTE))
	chance -= 20;

    chance -= get_skill(victim, gsn_dodge)/3;

    /* level */
    chance += 3 * (ch->level - (ride ? ride->level : victim->level)) / 2;

    /* race SNAKE */
    if (victim->race == RACE_SNAKE)
	chance *= 0.8;

    /* Blindness */
    if (IS_AFFECTED(ch, AFF_BLIND))
	chance /= 2;

    if (!IS_SET(victim->form, FORM_BIPED))
	chance /= 2;

	/*ослабляем шанс баша на треть*/
	if (!IS_NPC(victim))
	{
		chance *= 0.9;
	}
			
    chance = URANGE(0, chance, 99);

    dam += number_range(4, 4 + 4 * ch->size + chance/10);

    check_killer(ch, victim);

    /* now the attack */

    if (number_percent() < chance)
    {
	int wait;

	act("{5$n мощно толкает тебя!{x", ch, NULL, victim, TO_VICT);
	act("{5Ты мощным толчком отправляешь $N3 $x!{x",
	    ch, NULL, victim, TO_CHAR);
	act("{5$n мощным толчком отправляет $N3 $x.{x",
	    ch, NULL, victim, TO_NOTVICT);

	check_improve(ch, victim, gsn_bash, TRUE, 1);

	switch(number_bits(2)) {
	case 0: wait = 0; break;
	case 1: wait = 1; break;
	case 2: wait = 2; break;
	case 3: wait = 3; break;
	default: wait= 2; break;
	}

	WAIT_STATE(victim, (wait * PULSE_VIOLENCE));
	WAIT_STATE(ch, skill_table[gsn_bash].beats);
	damage(ch, victim, dam, gsn_bash, DAM_BASH, TRUE, NULL);
	victim->position = POS_BASHED;

	if (ride)
	{
	    act("{5$n падает с $N1 $x!{x", ride, NULL, victim, TO_ROOM);
	    act("{5Ты падаешь с $N1 $x!{x", ride, NULL, victim, TO_CHAR);
	    damage(ch, ride, dam/2, gsn_bash, DAM_BASH, TRUE, NULL);

	    if (number_percent() < get_skill(ride, gsn_recovery))
		mount_success(ride, victim, FALSE, TRUE);
	    else
		ride->position = POS_BASHED;
	}
    }
    else
    {
	damage(ch, victim, 0, gsn_bash, DAM_BASH, FALSE, NULL);
	act("{5Ты промахиваешься и падаешь!{x", ch, NULL, victim, TO_CHAR);
	act("{5$n промахивается и падает!{x", ch, NULL, victim, TO_NOTVICT);
	act("{5Ты избегаешь толчка $n1, и $e падает!{x",
	    ch, NULL, victim, TO_VICT);
	check_improve(ch, victim, gsn_bash, FALSE, 1);
	ch->position = POS_BASHED;
	WAIT_STATE(ch, skill_table[gsn_bash].beats * 3/2);
    }
}


/****************
* БРОСОК ГРЯЗЬЮ *
****************/
void do_dirt(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;

    one_argument(argument, arg);

    if ((chance = get_skill(ch, gsn_dirt)) < 1
	|| (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_KICK_DIRT)))
    {
	send_to_char("Ты загребаешь грязь ногой.\n\r", ch);
	return;
    }

    if (MOUNTED(ch))
    {
	send_to_char("Ты не можешь бросать грязь, пока сидишь на лошади!\n\r", ch);
	return;
    }

    if (arg[0] == '\0')
    {
	victim = ch->fighting;
	if (victim == NULL)
	{
	    send_to_char("Но ты ни с кем не сражаешься!\n\r", ch);
	    return;
	}
    }
    else if ((victim = get_char_room(ch, NULL, arg, TRUE)) == NULL)
    {
	if (!too_many_victims)
	    send_to_char("Таких здесь нет.\n\r", ch);
	return;
    }

    if (IS_AFFECTED(victim, AFF_BLIND))
    {
	sprintf(arg, "$N выглядит уже ослепленн%s!", SEX_END_ADJ(victim));
	act(arg, ch, NULL, victim, TO_CHAR);
	return;
    }

    if (victim == ch)
    {
	printf_to_char("Очень смешно. Хорошо, что не попал%s.\n\r", ch, SEX_ENDING(ch));
	return;
    }

    if (MOUNTED(victim))
    {
	send_to_char("Грязью не достать того, кто сидит на лошади.\n\r", ch);
	return;
    }

    if (!IS_SET(victim->parts, PART_EYE) || victim->classid == CLASS_NAZGUL)
    {
	send_to_char("Но у этого существа же нет глаз!\n\r", ch);
	return;
    }

    if (is_safe(ch, victim))
	return;

    /*    if (IS_NPC(victim) &&
     victim->fighting != NULL &&
     !is_same_group(ch, victim->fighting))
     {
     send_to_char("Kill stealing is not permitted.\n\r", ch);
     return;
     } */

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
    {
	act("Но $N - такой хороший друг!", ch, NULL, victim, TO_CHAR);
	return;
    }

    /* modifiers */

    chance -= get_skill(victim, gsn_dirt)/4;

    /* dexterity */
    chance += get_curr_stat(ch, STAT_DEX);
    chance -= 2 * get_curr_stat(victim, STAT_DEX);

    /* speed  */
    if (IS_SET(ch->off_flags, OFF_FAST) || IS_AFFECTED(ch, AFF_HASTE))
	chance += 10;
    if (IS_SET(victim->off_flags, OFF_FAST) || IS_AFFECTED(victim, AFF_HASTE))
	chance -= 25;

    /* level */
    chance += (ch->level - victim->level) * 2;

	if (ch->classid == CLASS_DRUID)
	{
		chance -= 5;
	}

    /* sloppy hack to prevent false zeroes */
    if (chance % 5 == 0)
	chance += 1;

    /* terrain */

    switch(ch->in_room->sector_type)
    {
    case(SECT_INSIDE):		chance -= 20;	break;
    case(SECT_CITY):		chance -= 10;	break;
    case(SECT_FIELD):		chance +=  5;	break;
    case(SECT_FOREST):				break;
    case(SECT_HILLS):				break;
    case(SECT_MOUNTAIN):	chance -= 10;	break;
    case(SECT_WATER_SWIM):	chance  =  0;	break;
    case(SECT_WATER_NOSWIM):	chance  =  0;	break;
    case(SECT_AIR):		chance  =  0;  	break;
    case(SECT_DESERT):		chance += 10;   break;
    }

    if (chance == 0)
    {
	send_to_char("Здесь совсем нет грязи.\n\r", ch);
	return;
    }

    check_killer(ch, victim);

    /* now the attack */
    if (number_percent() < chance)
    {
	AFFECT_DATA af;

	act("{5$n жмурится от грязи, попавшей в $s2 глаза!{x",
	    victim, NULL, NULL, TO_ROOM);
	act("{5$n бросает грязь тебе в лицо!{x", ch, NULL, victim, TO_VICT);
	damage(ch, victim, number_range(2, 5), gsn_dirt, DAM_NONE, FALSE, NULL);
	send_to_char("{5Ты ничего не видишь!{x\n\r", victim);
	check_improve(ch, victim, gsn_dirt, TRUE, 1);
	WAIT_STATE(ch, skill_table[gsn_dirt].beats);

	af.where	= TO_AFFECTS;
	af.type 	= gsn_dirt;
	af.level 	= ch->level;
	af.duration	= 0;
	af.location	= APPLY_HITROLL;
	af.modifier	= -4;
	af.bitvector 	= AFF_BLIND;
	af.caster_id 	= get_id(ch);
	affect_to_char(victim, &af);
    }
    else
    {
	damage(ch, victim, 0, gsn_dirt, DAM_NONE, TRUE, NULL);
	check_improve(ch, victim, gsn_dirt, FALSE, 1);
	WAIT_STATE(ch, skill_table[gsn_dirt].beats);
    }
}

/**********
* ПОДНОЖКА 
***********/
void do_trip(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;

    one_argument(argument, arg);

    if ((chance = get_skill(ch, gsn_trip)) < 1
	|| (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_TRIP)))
    {
	send_to_char("Подсечь? Это как?\n\r", ch);
	return;
    }

    if (MOUNTED(ch))
    {
	send_to_char("Как ты собираешься подсечь ноги противника с лошади?\n\r",
		     ch);
	return;
    }

    if (arg[0] == '\0')
    {
	victim = ch->fighting;
	if (victim == NULL)
	{
	    send_to_char("Но ты ни с кем не сражаешься!\n\r", ch);
	    return;
	}
    }
    else if ((victim = get_char_room(ch, NULL, arg, TRUE)) == NULL)
    {
	if (!too_many_victims)
	    send_to_char("Таких здесь нет.\n\r", ch);
	return;
    }

    if (MOUNTED(victim))
    {
	send_to_char("Ты не можешь подсечь того, кто сидит на лошади.\n\r", ch);
	return;
    }

    if (!IS_SET(victim->parts, PART_LEGS))
    {
	send_to_char("И как это ты, интересно, собираешься поставить подножку "
		     "существу без ног?\n\r", ch);
	return;
    }

    if (!IS_SET(ch->parts, PART_LEGS))
    {
	send_to_char("И как это ты, интересно, собираешься поставить подножку, не имея ног?\n\r", ch);
	return;
    }

    if (is_safe(ch, victim))
	return;

    if (IS_AFFECTED(victim, AFF_FLYING))
    {
	act("Ноги $N1 не стоят на земле.", ch, NULL, victim, TO_CHAR);
	return;
    }

    if (victim->position < POS_FIGHTING)
    {
	act("$N уже на земле.", ch, NULL, victim, TO_CHAR);
	return;
    }

    if (victim == ch)
    {
	act("{5Ты падаешь $x!{x", ch, NULL, NULL, TO_CHAR);
	WAIT_STATE(ch, 2 * skill_table[gsn_trip].beats);
	act("{5$n подсекает свои ноги!{x", ch, NULL, NULL, TO_ROOM);
	return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
    {
	act("$N - твой любимый хозяин.", ch, NULL, victim, TO_CHAR);
	return;
    }

    /* modifiers */

    chance -= get_skill(victim, gsn_trip)/4;

    /* size */
    if (ch->size < victim->size)
	chance += (ch->size - victim->size) * 10;  /* bigger = harder to trip */

    /* dex */
    chance += get_curr_stat(ch, STAT_DEX);
    chance -= get_curr_stat(victim, STAT_DEX) * 3 / 2;

    /* speed */
    if (IS_SET(ch->off_flags, OFF_FAST) || IS_AFFECTED(ch, AFF_HASTE))
	chance += 10;
    if (IS_SET(victim->off_flags, OFF_FAST) || IS_AFFECTED(victim, AFF_HASTE))
	chance -= 20;

    /* level */
    chance += (ch->level - victim->level) * 2;

    /* race SNAKE */
    if (victim->race == RACE_SNAKE)
	chance *= 0.9;

    /* If victim is not biped, reduce chance */
    if (!IS_SET(victim->form, FORM_BIPED))
	chance /= 2;

    check_killer(ch, victim);

    /* now the attack */
    if (number_percent() < chance)
    {
	CHAR_DATA *ride;
	int dam;

	act("{5$n ставит тебе подножку и ты падаешь!{x",
	    ch, NULL, victim, TO_VICT);
	act("{5Ты ставишь подножку, и $N падает!{x", ch, NULL, victim, TO_CHAR);
	act("{5$n ставит подножку, и $N падает $x.{x",
	    ch, NULL, victim, TO_NOTVICT);
	check_improve(ch, victim, gsn_trip, TRUE, 1);

	WAIT_STATE(victim, (number_bits(1) + 1) * skill_table[gsn_trip].beats);
	WAIT_STATE(ch, skill_table[gsn_trip].beats);
	victim->position = POS_BASHED;
	damage(ch, victim, (dam = number_range(2, 2 +  2 * victim->size)),
	       gsn_trip, DAM_BASH, TRUE, NULL);

	if ((ride = RIDDEN(victim)) != NULL)
	{
	    act("{5$n падает с $N1 $x!{x", ride, NULL, victim, TO_ROOM);
	    act("{5Ты падаешь с $N1 $x!{x", ride, NULL, victim, TO_CHAR);
	    damage(ch, ride, dam/2, gsn_trip, DAM_BASH, TRUE, NULL);

	    if (number_percent() < get_skill(ride, gsn_recovery))
		mount_success(ride, victim, FALSE, TRUE);
	    else
		ride->position = POS_BASHED;
	}
    }
    else
    {
	damage(ch, victim, 0, gsn_trip, DAM_BASH, TRUE, NULL);
	WAIT_STATE(ch, skill_table[gsn_trip].beats*2/3);
	check_improve(ch, victim, gsn_trip, FALSE, 1);
    }
}



void do_kill(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *victim_next;
    bool blinded_attack = FALSE;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Убить кого?\n\r", ch);
	return;
    }

    if (ch->in_room == NULL)
    {
	send_to_char("Чтобы убить кого-то, нужно быть где-то.\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "вслепую") || !str_cmp(arg, "blind"))
    {
	if (!IS_AFFECTED(ch, AFF_BLIND) && !room_is_dark(ch->in_room))
	{
	    send_to_char("Ты прекрасно все видишь.\n\r", ch);
	    return;
	}
	blinded_attack = TRUE;
    }

    if (blinded_attack)
    {
	bool found = FALSE;

	LIST_FOREACH_SAFE(victim, &ch->in_room->people, room_link, victim_next)
	{
	    int chance = 50;

	    if (victim == ch || IS_WIZINVISMOB(victim) || IS_IMMORTAL(victim))
		continue;

	    if (number_percent() < chance)
	    {
		found = TRUE;
		break;
	    }
	}

	if (!found)
	{
	    send_to_char("Твоя попытка атаковать кого-нибудь "
			 "вслепую не удалась!\n\r", ch);
	    return;
	}
    }
    else if ((victim = get_char_room(ch, NULL, arg, TRUE)) == NULL)
    {
	if (!too_many_victims)
	    send_to_char("Таких здесь нет.\n\r", ch);
	return;
    }

    /*  Allow player killing
     if (!IS_NPC(victim))
     {
     if (!IS_SET(victim->act, PLR_KILLER)
     &&   !IS_SET(victim->act, PLR_THIEF))
     {
     send_to_char("You must MURDER a player.\n\r", ch);
     return;
     }
     }
     */
    if (victim == ch)
    {
	send_to_char("Ты ударяешь себя. Интересно, за что?\n\r", ch);
	multi_hit(ch, ch, TYPE_UNDEFINED);
	return;
    }

    if (is_safe(ch, victim))
	return;

    /*    if (victim->fighting != NULL &&
     !is_same_group(ch, victim->fighting))
     {
     send_to_char("Kill stealing is not permitted.\n\r", ch);
     return;
     } */

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
    {
	act("$N - твой любимый хозяин.", ch, NULL, victim, TO_CHAR);
	return;
    }

    if (POS_FIGHT(ch))
    {
	send_to_char("Ты уже изо всех сил пытаешься сражаться!\n\r", ch);
	return;
    }
    WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
    check_killer(ch, victim);
    multi_hit(ch, victim, TYPE_UNDEFINED);
}

bool can_backstab(CHAR_DATA *ch, CHAR_DATA *victim)
{
    char arg[MAX_STRING_LENGTH];
    bool ret = TRUE;

    arg[0] = 0;

    if (get_skill(ch, gsn_backstab) < 1)
    {
	sprintf(arg, "Ты не знаешь, как это делается.\n\r");
	ret = FALSE;
    }

    if (ch->fighting != NULL)
    {
	sprintf(arg, "Ты стоишь лицом не в ту сторону.\n\r");
	ret = FALSE;
    }

    if (MOUNTED(ch))
    {
	sprintf(arg, "Ты не можешь пырнуть в спину, пока сидишь верхом!\n\r");
	ret = FALSE;
    }

    if (victim == ch)
    {
	sprintf(arg, "Каким образом ты хочешь подкрасться к себе сзади?\n\r");
	ret = FALSE;
    }

    if (!IS_AFFECTED(ch, AFF_HIDE))
    {
	sprintf(arg, "Для того, чтобы пырнуть, нужно быть скрытым.\n\r");
	ret = FALSE;
    }

    if (is_safe(ch, victim))
	ret = FALSE;

    if (get_eq_char(ch, WEAR_WIELD) == NULL)
    {
	sprintf(arg, "Для начала одень оружие.\n\r");
	ret = FALSE;
    }

    if (victim->hit < 3 * victim->max_hit / 4)
    {
	sprintf(arg, "$N ранен%s и осторож%s... ты не можешь подкрасться.",
		SEX_ENDING(victim),
		victim->sex == SEX_FEMALE ? "на" :
		victim->sex == SEX_MALE   ? "ен" :
		victim->sex == SEX_MANY   ? "ны" : "но");
	ret = FALSE;
    }

    /*    if (IS_NPC(victim) &&
     victim->fighting != NULL &&
     !is_same_group(ch, victim->fighting))
     {
     send_to_char("Kill stealing is not permitted.\n\r", ch);
     ret = FALSE;
     } */

    if (!IS_NULLSTR(arg))
	act(arg, ch, NULL, victim, TO_CHAR);

    return ret;
}

void do_backstab(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Пырнуть кого?\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, NULL, arg, TRUE)) == NULL)
    {
	if (!too_many_victims)
	    send_to_char("Таких здесь нет.\n\r", ch);
	return;
    }

    if (!can_backstab(ch, victim))
	return;

    check_killer(ch, victim);
    WAIT_STATE(ch, skill_table[gsn_backstab].beats);
    if (number_percent() < get_skill(ch, gsn_backstab)
	|| (get_skill(ch, gsn_backstab) > 1 && !IS_AWAKE(victim)))
    {
	check_improve(ch, victim, gsn_backstab, TRUE, 1);
	multi_hit(ch, victim, gsn_backstab);
	
	if (number_percent() < (get_skill(ch, gsn_dual_backstab) / 10) * 8)
	{
	    check_improve(ch, victim, gsn_dual_backstab, TRUE, 1);
	    multi_hit(ch, victim, gsn_dual_backstab);
	}
	else
	    check_improve(ch, victim, gsn_dual_backstab, FALSE, 1);

    }
    else
    {
	check_improve(ch, victim, gsn_backstab, FALSE, 1);
	damage(ch, victim, 0, gsn_backstab, DAM_NONE, TRUE, NULL);
    }

    return;
}




/****************
* СБЕЖАТЬ ИЗ БОЯ
*****************/
void do_flee(CHAR_DATA *ch, char *argument)
{
    ROOM_INDEX_DATA *was_in;
    ROOM_INDEX_DATA *now_in;
    CHAR_DATA *victim;
    int attempt, max_attempts;

    if (ch->position < POS_SLEEPING)
	return;

    if ((victim = ch->fighting) == NULL)
    {
	if (POS_FIGHT(ch))
	    ch->position = POS_STANDING;

	send_to_char("Но ты ни с кем не сражаешься.\n\r", ch);
	return;
    }

    if (IS_AFFECTED(ch, AFF_ROOTS))
    {
	send_to_char("{RТы не можешь двинуться с места.{x\n\r", ch);
	return;
    }

    if (ch->position == POS_BASHED)
    {
	send_to_char("{RТебе надо встать на ноги!{x\n\r", ch);
	return;
    }

    max_attempts = is_affected(victim, gsn_power_word_fear) ? 3 : 6;

    /*Оборотень в форме убегает из драки хуже
    */
    max_attempts = is_lycanthrope(victim) ? 3 : 6;

    /*Оборотень с духом  убегает из драки хуже
    */
    
    max_attempts = is_lycanthrope_spirit(victim) ? 4 : 6;

    max_attempts += is_affected(victim, gsn_howl) ? 2 : 0;

    was_in = ch->in_room;
    for (attempt = 0; attempt < max_attempts; attempt++)
    {
	EXIT_DATA *pexit;
	int door;

	door = number_door();
	if ((pexit = was_in->exit[door]) == 0
	    || pexit->u1.to_room == NULL
	    || (IS_SET(pexit->exit_info, EX_CLOSED)
		&& ((!IS_AFFECTED(ch, AFF_PASS_DOOR) && !is_affected(ch, gsn_dissolve))
		    || IS_SET(pexit->exit_info, EX_NOPASS)))
	    || number_range(0, ch->daze) != 0
	    || (IS_NPC(ch)
		&& IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB)))
	{
	    continue;
	}

	move_char(ch, door, FALSE, FALSE);

	if ((now_in = ch->in_room) == was_in)
	    continue;

	if (IS_NPC(ch) && ch->fighting)
	    mob_forget(ch, ch->fighting, MEM_HOSTILE);

	/*	ch->in_room = was_in;
	 act("$n убегает!", ch, NULL, NULL, TO_ROOM);
	 ch->in_room = now_in; */

	if (!IS_NPC(ch))
	{
	    send_to_char("Ты убегаешь из драки!\n\r", ch);
	    ch->pcdata->flees++;

	    if (ch->classid == CLASS_THIEF
		&& number_percent() < 2 * ch->level / 3)
	    {
		send_to_char("Ты не теряешь опыта.\n\r", ch);
	    }
	    else
	    {
		int loose = UMIN(25, UMAX(5, ch->level));
		char buf[MSL];

		sprintf(buf, "Ты теряешь %d %s опыта.\n\r", loose, hours(loose, TYPE_POINTS));
		send_to_char(buf, ch);
/*		do_yell(ch, "Спасите, убивают!");
*/
    		DESCRIPTOR_DATA *d, *safe_d;

		act("Ты вопишь: {9$t{x", ch, "Спасите, убивают!", NULL, TO_CHAR);
		SLIST_FOREACH_SAFE(d, &descriptor_list, link, safe_d)
		{
			if (d->connected == CON_PLAYING
			&&   d->character != NULL
			&&   d->character != ch
			&&   d->character->in_room != NULL
			&&   d->character->in_room->area == ch->in_room->area
			&&   d->character->in_room->clan == ch->in_room->clan
			&&   !IS_SET(d->character->comm, COMM_QUIET)
			&&   d->character->position != POS_SLEEPING
			&&   !check_filter(ch, d->character)
			&&   IS_NULLSTR(ch->in_room->owner))
			{
			if (is_lycanthrope(ch) && !is_lycanthrope(d->character) && !IS_IMMORTAL(d->character))
				act("$n вопит: {9$t{x", ch, makehowl("Спасите, убивают!",ch), d->character, TO_VICT);
			else
				act("$n вопит: {9$t{x", ch, "Спасите, убивают!", d->character, TO_VICT);
			}
		}

		gain_exp(ch, -loose, FALSE);
	    }
	}

	stop_fighting(ch, TRUE);
	return;
    }

    send_to_char("КАРАУЛ! Ты не находишь выхода!\n\r", ch);
    return;
}



void do_rescue(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *fch;

    one_argument(argument, arg);
    if (arg[0] == '\0')
    {
	send_to_char("Спасти кого?\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, NULL, arg, TRUE)) == NULL)
    {
	if (!too_many_victims)
	    send_to_char("Таких здесь нет.\n\r", ch);
	return;
    }

    if (victim == ch)
    {
	send_to_char("Как насчет убежать?\n\r", ch);
	return;
    }


    if (ch->fighting == victim)
    {
	send_to_char("Слишком поздно.\n\r", ch);
	return;
    }


    if (!can_see(ch, victim))
    {
	return;
    }

    if ((fch = victim->fighting) == NULL)
    {
	send_to_char("Этот персонаж не сражается в данное время.\n\r", ch);
	return;
    }

    if ((!IS_NPC(ch) && IS_NPC(victim) && !is_same_group(ch, victim)) || is_safe(ch, fch))
    {
	send_to_char("Твоя помощь не нужна!\n\r", ch);
	return;
    }

    /*    if (IS_NPC(fch) && !is_same_group(ch, victim))
     {
     send_to_char("Kill stealing is not permitted.\n\r", ch);
     return;
     }  */

    WAIT_STATE(ch, skill_table[gsn_rescue].beats);
    if (number_percent() > get_skill(ch, gsn_rescue))
    {
	send_to_char("У тебя не получается.\n\r", ch);
	check_improve(ch, victim, gsn_rescue, FALSE, 1);
	return;
    }

    act("{5Ты заслоняешь $N3!{x",  ch, NULL, victim, TO_CHAR  );
    act("{5$n заслоняет тебя $t!{x", ch,
	!IS_NPC(ch) || ch->pIndexData->vnum != MOB_VNUM_DRUIDS_TREE ?"от атаки" : "своими ветвями",
	victim, TO_VICT  );
    act("{5$n спасает $N3!{x",  ch, NULL, victim, TO_NOTVICT);
    check_improve(ch, victim, gsn_rescue, TRUE, 1);

    stop_fighting(fch, FALSE);
    stop_fighting(victim, FALSE);

    check_killer(ch, fch);

    set_fighting(ch, fch);
    set_fighting(fch, ch);
    set_fighting(victim, fch);
    set_fighting(fch, victim);

    return;
}


/* Skill Пинок*/
void do_kick(CHAR_DATA *ch, char *argument){
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int dam, skill;
    char arg[MIL];

    if ((skill = get_skill(ch, gsn_kick)) < 1){
		send_to_char("Оставь боевые искусства воинам.\n\r", ch);
		return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0'){
		victim = ch->fighting;
		if (victim == NULL || victim->position == POS_DEAD){
			send_to_char("Кого пнуть-то?\n\r", ch);
			return;
		}
    } else {
		if ((victim = get_char_room(ch, NULL, arg, TRUE)) == NULL){
			if (!too_many_victims) send_to_char("Здесь таких нет.\n\r", ch);
			return;
		}
	}

    if (victim == ch) {
		send_to_char("Ты пытаешься пнуть свою задницу.\n\r", ch);
		return;
    }

    if (is_safe(ch, victim)) return;

    if (MOUNTED(victim)){
		send_to_char("Ты не можешь пнуть того, кто сидит на лошади.\n\r", ch);
		return;
    }

    if (check_charmees(ch, victim))	return;

    obj = get_eq_char(ch, WEAR_FEET);

    dam = number_range(1, ch->level);
	dam *= 4; /*усиливаем пинок в 4 раза. Ну на деле - менее, чем в 2 раза, но все ж. Мб, юзабельнее станет.*/

    if (obj != NULL) {
		dam += (obj->weight) / 5 + UMIN(get_carry_weight(ch), 600) / 40;
	} else {
		dam += UMIN(get_carry_weight(ch), 600) / 50;
	}

    if (obj != NULL) dam += obj->weight / 10;

    WAIT_STATE(ch, skill_table[gsn_kick].beats);
    check_killer(ch, victim);

    if (skill > number_percent()){
		act("$n пинает $N3.", ch, NULL, victim, TO_NOTVICT);
		damage(ch, victim, dam, gsn_kick, DAM_BASH, TRUE, NULL);
		check_improve(ch, victim, gsn_kick, TRUE, 1);
    } else {
		act("$n безуспешно пытается пнуть $N3.", ch, NULL, victim, TO_NOTVICT);
		damage(ch, victim, 0, gsn_kick, DAM_BASH, TRUE, NULL);
		check_improve(ch, victim, gsn_kick, FALSE, 1);
    }
}


/* Disarm coeff */
static float c_disarm[MAX_WEAPON_TYPE][MAX_WEAPON_TYPE] =
{
    /*H2H, SWO, DAG, SPE, MAC, AXE, FLA, WHI, POL, STA, EXO */
    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }, /* Hand2Hand */
    { 0.8, 1.0, 0.8, 0.8, 1.0, 0.8, 1.2, 1.1, 0.9, 1.0, 0.9 }, /* SWOrd     */
    { 1.2, 1.0, 1.0, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 1.0 }, /* DAGger    */
    { 0.9, 0.9, 0.8, 1.0, 0.8, 0.8, 1.0, 0.9, 1.1, 1.1, 1.0 }, /* SPEar     */
    { 0.9, 1.0, 0.8, 0.8, 1.0, 0.8, 0.9, 1.0, 0.8, 0.9, 1.1 }, /* MACe      */
    { 0.8, 1.0, 0.8, 0.8, 1.1, 1.0, 1.1, 0.8, 0.9, 0.8, 0.9 }, /* AXE       */
    { 0.8, 0.8, 0.8, 1.1, 0.8, 0.8, 1.0, 0.8, 1.1, 1.1, 0.9 }, /* FLAil     */
    { 0.9, 0.8, 0.8, 1.2, 0.8, 0.8, 0.8, 1.0, 1.2, 1.2, 1.0 }, /* WHIp      */
    { 0.8, 0.8, 0.8, 0.8, 0.9, 0.9, 0.9, 0.8, 1.0, 0.9, 0.9 }, /* POLearm   */
    { 0.8, 0.8, 0.8, 0.8, 0.9, 0.8, 0.8, 0.8, 1.0, 1.0, 0.9 }, /* STAff     */
    { 0.9, 1.0, 0.9, 0.9, 1.0, 0.9, 1.0, 0.9, 0.9, 0.9, 1.0 }  /* EXOtic    */
};

/* Disarm prof coeff*/
static float prof_disarm[MAX_CLASS] =
{
    /*MAG  CLE  THI  War  NEC  PAL   NAZ  DRU  RAN   VAM  ALH  LYC*/
     0.9, 0.95, 1.0, 1.0, 0.9, 0.95, 0.95, 0.9, 1.0, 0.9, 0.9, 0.95
};

void do_disarm(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj, *obj1, *ch_obj;
    int chance, hth, ch_weapon, vict_weapon, ch_vict_weapon;
    bool secondary=FALSE;

    hth = 0;

    if ((chance = get_skill(ch, gsn_disarm)) < 1)
    {
		send_to_char("Ты не знаешь, как обезоружить противника.\n\r", ch);
		return;
    }

    if ((get_eq_char(ch, WEAR_WIELD) == NULL
		&& get_eq_char(ch, WEAR_SECONDARY) == NULL)
		&& ((hth = get_skill(ch, gsn_hand_to_hand)) < 1
    	|| (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_DISARM))))
    {
		send_to_char("Ты должен сначала вооружиться.\n\r", ch);
		return;
    }

    if ((victim = ch->fighting) == NULL)
    {
		send_to_char("Но ты ни с кем не сражаешься!\n\r", ch);
		return;
    }

    obj  = get_eq_char(victim, WEAR_WIELD);
    obj1 = get_eq_char(victim, WEAR_SECONDARY);

    if (obj != NULL)
    {
		if (obj1 != NULL && number_range(1, 10) > 5)
    	secondary = TRUE;
    }
    else
    {
		if (obj1 != NULL)
    		secondary = TRUE;
		else
		{
    		send_to_char("Твой противник без оружия.\n\r", ch);
    		return;
		}
    }

    /* find weapon skills */
    ch_weapon = get_weapon_skill(ch, get_weapon_sn(ch, FALSE));    /* Умение нападающего владеть своим оружием */
    vict_weapon = get_weapon_skill(victim, get_weapon_sn(victim, secondary)); /* Умение жертвы владеть своим оружием */
    ch_vict_weapon = get_weapon_skill(ch, get_weapon_sn(victim, secondary)); /* Умение нападающего владеть оружием жертвы */

 /* modifiers 
* Влияют: мастерство владения оружием - агрессор_своим, жертва_своим, агрессор_жертвы. (ch, victim, ch_victim)
*   мастерство жертвы владеть умением обезоружить
*   ловкость
*   сила
*   спешка
*   разница в уровнях
*   соотношение возможности обезоружить по типу оружия
*   флаг оружия - двуручное
    */

    ch_obj = get_eq_char(ch, WEAR_WIELD);

    /* Учитываем умение агрессора владеть оружием */
    if (!ch_obj)
		chance = chance * hth/150;
    else
		chance = chance * ch_weapon/100;

    /* Учитываем способность данного класса пользоваться умением */
    if (!IS_NPC(ch))
        chance *= prof_disarm[ch->classid];

    chance -= get_skill(victim, gsn_disarm) / 4;
    chance += (ch_vict_weapon/2 - vict_weapon) / 2;

    /* dex vs. strength */
    chance += get_curr_stat(ch, STAT_DEX);        
    chance -= 2 * get_curr_stat(victim, STAT_STR);
  
    /*Спешка*/
    if (IS_SET(ch->off_flags, OFF_FAST) || IS_AFFECTED(ch, AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags, OFF_FAST) || IS_AFFECTED(victim, AFF_HASTE))
        chance -= 10;

    /* Разница в уровнях*/
    chance += (ch->level - victim->level) * 2;

    /* Учитываем типы оружия. Если противник без оружия, проверка выше */
    if (ch_obj != NULL)
          chance *= (secondary ? c_disarm[ch_obj->value[0]][obj1->value[0]] : c_disarm[ch_obj->value[0]][obj->value[0]]);
    else
          chance *= (secondary ? c_disarm[0][obj1->value[0]] : c_disarm[0][obj->value[0]]);

    /*Двуручное оружие труднее*/
    if (!secondary && IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS))
    {
          chance /= 2;
    }

    /* and now the attack */
    if (number_percent() < chance) 
    {
		WAIT_STATE(ch, skill_table[gsn_disarm].beats);
		disarm(ch, victim, secondary);
		check_improve(ch, victim, gsn_disarm, TRUE, 1);
    }
    else
    {
		WAIT_STATE(ch, skill_table[gsn_disarm].beats);
		act("{5У тебя не получается обезоружить $N3.{x", ch, NULL, victim, TO_CHAR);
		act("{5$n безуспешно пытается тебя обезоружить.{x", ch, NULL, victim, TO_VICT);
		act("{5$n безуспешно пытается обезоружить $N3.{x", ch, NULL, victim, TO_NOTVICT);
		check_improve(ch, victim, gsn_disarm, FALSE, 1);
    }

    check_killer(ch, victim);
}



/*-----------------24.10.2006 15:33-----------------
 * - old version
 * --------------------------------------------------*/
/*
void do_disarm(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj, *obj1;
    int chance, hth, ch_weapon, vict_weapon, ch_vict_weapon;
    bool secondary=FALSE;

    hth = 0;

    if ((chance = get_skill(ch, gsn_disarm)) < 1)
    {
	send_to_char("Ты не знаешь, как обезоружить противника.\n\r", ch);
	return;
    }


    if ((get_eq_char(ch, WEAR_WIELD) == NULL
	 && get_eq_char(ch, WEAR_SECONDARY) == NULL)
	&& ((hth = get_skill(ch, gsn_hand_to_hand)) < 1
	    || (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_DISARM))))
    {
	send_to_char("Ты должен сначала вооружиться.\n\r", ch);
	return;
    }

    if ((victim = ch->fighting) == NULL)
    {
	send_to_char("Но ты ни с кем не сражаешься!\n\r", ch);
	return;
    }

    obj  = get_eq_char(victim, WEAR_WIELD);
    obj1 = get_eq_char(victim, WEAR_SECONDARY);


    if (obj != NULL)
    {
	if (obj1 != NULL && number_range(1, 10) > 5)
	    secondary = TRUE;
    }
    else
    {
	if (obj1 != NULL)
	    secondary = TRUE;
	else
	{
	    send_to_char("Твой противник без оружия.\n\r", ch);
	    return;
	}
    }


    ch_weapon = get_weapon_skill(ch, get_weapon_sn(ch, FALSE));
    vict_weapon = get_weapon_skill(victim, get_weapon_sn(victim, secondary));
    ch_vict_weapon = get_weapon_skill(ch, get_weapon_sn(victim, secondary));

    if (get_eq_char(ch, WEAR_WIELD) == NULL)
	chance = chance * hth/150;
    else
	chance = chance * ch_weapon/100;

    chance -= get_skill(victim, gsn_disarm)/4;

    chance += (ch_vict_weapon/2 - vict_weapon) / 2;


    chance += get_curr_stat(ch, STAT_DEX);
    chance -= 2 * get_curr_stat(victim, STAT_STR);


    chance += (ch->level - victim->level) * 2;


    if (number_percent() < chance)
    {
	WAIT_STATE(ch, skill_table[gsn_disarm].beats);
	disarm(ch, victim, secondary);
	check_improve(ch, victim, gsn_disarm, TRUE, 1);
    }
    else
    {
	WAIT_STATE(ch, skill_table[gsn_disarm].beats);
	act("{5У тебя не получается обезоружить $N3.{x",
	    ch, NULL, victim, TO_CHAR);
	act("{5$n безуспешно пытается тебя обезоружить.{x",
	    ch, NULL, victim, TO_VICT);
	act("{5$n безуспешно пытается обезоружить $N3.{x",
	    ch, NULL, victim, TO_NOTVICT);
	check_improve(ch, victim, gsn_disarm, FALSE, 1);
    }

    check_killer(ch, victim);
}
*/

/*
 void do_surrender(CHAR_DATA *ch, char *argument)
 {
 CHAR_DATA *mob;
 if ((mob = ch->fighting) == NULL)
 {
 send_to_char("Но ты ни с кем не дерешься!\n\r", ch);
 return;
 }
 act("Ты сдаешься $N2!", ch, NULL, mob, TO_CHAR);
 act("$n сдается тебе!", ch, NULL, mob, TO_VICT);
 act("$n пытается сдасться $N2!", ch, NULL, mob, TO_NOTVICT);
 stop_fighting(ch, TRUE);

 if (!IS_NPC(ch) && IS_NPC(mob)
 &&   (!HAS_TRIGGER_MOB(mob, TRIG_SURR)
 || !p_percent_trigger(mob, NULL, NULL, ch, NULL, NULL, TRIG_SURR)))
 {
 act("$N игнорирует твою попытку сдасться!", ch, NULL, mob, TO_CHAR);
 multi_hit(mob, ch, TYPE_UNDEFINED);
 }
 }
 */

void do_sla(CHAR_DATA *ch, char *argument)
{
    send_to_char("Если хочешь уничтожить игрока, набери команду полностью.\n\r",
		 ch);
    return;
}


void do_suicid(CHAR_DATA *ch, char *argument)
{
    send_to_char("Если хочешь покончить жизнь самоубийством, набери команду полностью.\n\r", ch);
    return;
}

void do_suicide(CHAR_DATA *ch, char *argument)
{
    if (is_affected(ch, gsn_timeout))
   	{
		send_to_char("Дождись, пока спадет временная миролюбивость.\n\r", ch);
		return;
	}
	
	if (IS_SET(ch->in_room->room_flags, ROOM_PRISON))
	{
		send_to_char("Отсиди сначала свое в тюрьме.\n\r", ch);
		return;
	}

	if (POS_FIGHT(ch))
	{
		send_to_char("Сначала закончи бой.\n\r", ch);
		return;
	}

	if (ch->hit < ch->max_hit/3 )
	{
		send_to_char("Сначала отдохни, подумай...\n\r", ch);
		return;
	}


	act("{1Ты убиваешь себя!{x",  ch, NULL, NULL, TO_CHAR  );
    act("{1$n убивает себя!{x",  ch, NULL, NULL, TO_NOTVICT);
    raw_kill(ch, ch, TRUE);
	return;
}


void do_slay(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);
    if (arg[0] == '\0')
    {
	send_to_char("Уничтожить кого?\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, NULL, arg, TRUE)) == NULL)
    {
	if (!too_many_victims)
	    send_to_char("Таких здесь нет.\n\r", ch);
	return;
    }

    if (ch == victim)
    {
	send_to_char("Хочешь заняться суицидом?\n\r", ch);
	return;
    }

    if (!IS_NPC(victim) && victim->level >= get_trust(ch))
    {
	send_to_char("У тебя не получается.\n\r", ch);
	return;
    }

    act("{1Ты уничтожаешь $N3!{x",  ch, NULL, victim, TO_CHAR  );
    act("{1$n уничтожает тебя!{x", ch, NULL, victim, TO_VICT  );
    act("{1$n уничтожает $N3!{x",  ch, NULL, victim, TO_NOTVICT);
    raw_kill(victim, ch, TRUE);
}



/********
* ВЫПАД *
********/
void do_lunge(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance, bleed;
    OBJ_DATA *prim;
    OBJ_DATA *sec;
    bool found = FALSE;
    AFFECT_DATA af;

    if ((chance = get_skill(ch, gsn_lunge)) < 1)
    {
        send_to_char("Выпад? Это как?\n\r", ch);
        return;
    }

    bleed = chance;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        victim = ch->fighting;
        if (victim == NULL || victim->position == POS_DEAD)
        {
            send_to_char("Но ты ни с кем не сражаешься!\n\r", ch);
            return;
        }
    }
    else if ((victim = get_char_room(ch, NULL, arg, TRUE)) == NULL)
    {
        if (!too_many_victims)
            send_to_char("Здесь таких нет.\n\r", ch);
        return;
    }

    if (victim == ch)
    {
        send_to_char("Ты это серьезно?\n\r", ch);
        return;
    }

    if (is_safe(ch, victim))
        return;

    if (check_charmees(ch, victim))
        return;

    prim=get_eq_char (ch, WEAR_WIELD);
    sec=get_eq_char (ch, WEAR_SECONDARY);

    /* modifiers */

    chance -= get_skill(victim, gsn_lunge) / 4;
    chance -= (victim->size - ch->size) * 3;

    /* stats */
    chance -= (get_curr_stat(victim, STAT_DEX) - get_curr_stat(ch, STAT_DEX)) * 2.5;
    chance += GET_AC(victim, AC_PIERCE) / 25;

    chance *= get_style_multiplier(ch);
    chance *= get_style_multiplier(victim);
    
    /* speed */
    /* Шанс равнозначно увеличивается и уменьшается если есть спешка */
    if (IS_SET(ch->off_flags, OFF_FAST) || IS_AFFECTED(ch, AFF_HASTE))
        chance += 15;
    if (IS_SET(victim->off_flags, OFF_FAST) || IS_AFFECTED(victim, AFF_HASTE))
        chance -= 15;

    /* level */
    chance += (ch->level - victim->level) * 2;

    /* skills */
/*    chance -= get_skill(victim, gsn_parry)/20;*/

/* Так же как и паринг, только вычитаем в зависимости от увертки */
/*    chance -= get_skill(victim, gsn_dodge)/20;*/

    chance /= 1.4;  /* Some Fix */

    if (victim->race == RACE_VAMPIRE)
    {
    	bleed = 0;
    }
    else
    {
     	bleed /= 6;
    	bleed += (ch->size - victim->size) * 3;
    	bleed += (get_curr_stat(ch,STAT_DEX) - get_curr_stat(victim, STAT_DEX));
    	bleed += (MAX_STAT - get_curr_stat(victim, STAT_CON));
	bleed *= (is_affected(victim, skill_lookup("stone skin")) ? 0.5 : 1);
    }

    check_killer(ch, victim);

    /* now the attack */
    if (prim != NULL)
    {
        if (prim->value[0] == WEAPON_SWORD || prim->value[0] == WEAPON_SPEAR ||
            prim->value[0] == WEAPON_POLEARM || prim->value[0] == WEAPON_DAGGER)
        {
            found=TRUE;

            if (number_percent() < chance)
            {
                send_to_char("{5Ты наносишь молниеносный выпад!{x\n\r", ch);
                check_improve(ch, victim, gsn_lunge, TRUE, 1);
                if (one_hit(ch, victim, /* TYPE_HIT+11 */ TYPE_UNDEFINED, FALSE)
                    && !is_affected(victim, gsn_bleed)
		    && victim->position != POS_DEAD
	            && number_percent() < bleed)
		{
		    if (IS_NPC(victim) && !IS_SET(victim->act, ACT_UNDEAD))
		    {
			af.where = TO_AFFECTS;
		        af.type = gsn_bleed;
		        af.level = ch->level;
		        af.duration = 2;
		        af.modifier = 0;
		        af.location = APPLY_NONE;
		        af.bitvector = 0;
		        af.caster_id = get_id(ch);
		        affect_to_char(victim, &af);

                	act("{5Ты наносишь $N2 кровоточащую рану!{x", ch, NULL, victim, TO_CHAR);
                	act("{5Ты получаешь кровоточащую рану!{x", ch, NULL, victim, TO_VICT);
                	act("{5$N получает кровоточащую рану!{x", ch, NULL, victim, TO_NOTVICT);
		    }
		}	

            }
            else
            {
                damage(ch, victim, 0, gsn_lunge, DAM_BASH, FALSE, NULL);
                act("{0Твой выпад не достает $N3!{x", ch, NULL, victim, TO_CHAR);
                act("{3Выпад $n1 не достает $N3!{x", ch, NULL, victim, TO_NOTVICT);
                act("{2Выпад $n1 не достигает тебя!{x", ch, NULL, victim, TO_VICT);
                check_improve(ch, victim, gsn_lunge, FALSE, 1);
            }
        }
    }

    if (sec != NULL)
    {
        if (sec->value[0] == WEAPON_SWORD || sec->value[0] == WEAPON_SPEAR ||
            sec->value[0] == WEAPON_POLEARM || sec->value[0] == WEAPON_DAGGER)
        {
            found=TRUE;

            if (victim != NULL && victim->position != POS_DEAD)
            {
                chance -= 100-get_skill(ch, gsn_lunge);
                if (number_percent() < chance)
                {
                    send_to_char("{5Ты наносишь молниеносный выпад!\n\r{x", ch);

                	if (one_hit(ch, victim, /* TYPE_HIT+11 */ TYPE_UNDEFINED, TRUE)
                    	    && !is_affected(victim, gsn_bleed)
			    && victim->position != POS_DEAD
	            	    && number_percent() < bleed)
			{
			    if (IS_NPC(victim) && !IS_SET(victim->act, ACT_UNDEAD))
			    {
				af.where = TO_AFFECTS;
		        	af.type = gsn_bleed;
		        	af.level = ch->level;
		        	af.duration = 2;
		        	af.modifier = 0;
		        	af.location = APPLY_NONE;
		        	af.bitvector = 0;
		        	af.caster_id = get_id(ch);
		        	affect_to_char(victim, &af);

                 		act("{5Ты наносишь $N2 кровоточащую рану!{x", ch, NULL, victim, TO_CHAR);
                		act("{5Ты получаешь кровоточащую рану!{x", ch, NULL, victim, TO_VICT);
                		act("{5$N получает кровоточащую рану!{x", ch, NULL, victim, TO_NOTVICT);
			    }
			}			    
                    	check_improve(ch, victim, gsn_lunge, TRUE, 1);
                }
                else
                {
                    damage(ch, victim, 0, gsn_lunge, DAM_BASH, FALSE, NULL);
                    act("{0Твой выпад не достает $N3!{x", ch, NULL, victim, TO_CHAR);
                    act("{3Выпад $n1 не достает $N3!{x", ch, NULL, victim, TO_NOTVICT);
                    act("{2Выпад $n1 не достигает тебя!{x", ch, NULL, victim, TO_VICT);
                    check_improve(ch, victim, gsn_lunge, FALSE, 1);		   
                }
            }
        }
    }

    if (!found)
        send_to_char("Для этого тебе надо вооружиться мечом, копьем, алебардой или кинжалом.\n\r", ch);
    else
        WAIT_STATE(ch, skill_table[gsn_lunge].beats);
}

int group_level(CHAR_DATA *ch, int whatlevel)
{
    CHAR_DATA *lch;
    int level, members=0;

    if (whatlevel == MIN_GROUP_LEVEL)
	level=MAX_LEVEL;
    else
	level=0;

    LIST_FOREACH(lch, &char_list, link)
    {
	if (ch->in_room != lch->in_room || !is_same_group(lch, ch) || IS_NPC(lch))
	    continue;

	if ((lch->level > level && whatlevel == MAX_GROUP_LEVEL)
	    || (lch->level < level && whatlevel == MIN_GROUP_LEVEL))
	{
	    level = lch->level;
	}
	else if (whatlevel == AVG_GROUP_LEVEL)
	{
	    level += lch->level;
	    members++;
	}
    }

    if (level == 0)
	return ch->level;

    if (whatlevel == AVG_GROUP_LEVEL)
    {
	if (members > 0)
	    level /= members;
	else
	    return ch->level;
    }

    return level;
}


/**********
* РАССЕЧЬ *
**********/
void do_cleave(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance = 0, skill = 0;
    OBJ_DATA *prim;

    if ((skill = get_skill(ch, gsn_cleave)) < 1
	|| (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_CLEAVE)))
    {
	send_to_char("Рассечь? Это как?\n\r", ch);
	return;
    }

    if (ch->fighting != NULL)
    {
	send_to_char("Ты не можешь как следует собраться для того,"
		     " чтобы рассечь противника.\n\r", ch);
	return;
    }


    one_argument(argument, arg);

    if ((victim = get_char_room(ch, NULL, arg, TRUE)) == NULL)
    {
	if (!too_many_victims)
	    send_to_char("Здесь таких нет.\n\r", ch);
	return;
    }

    if (victim == ch)
    {
	send_to_char("Ты это серьезно?\n\r", ch);
	return;
    }

    if (check_charmees(ch, victim))
	return;

    if ((prim = get_eq_char(ch, WEAR_WIELD)) == NULL)
    {
	send_to_char("Вооружиcь сначала.\n\r", ch);
	return;
    }

    /* now the attack */
    if (!IS_WEAPON_STAT(prim, WEAPON_TWO_HANDS)
	|| get_eq_char (ch, WEAR_SECONDARY) != NULL
	|| get_eq_char (ch, WEAR_SHIELD) != NULL
	|| get_eq_char (ch, WEAR_HOLD) != NULL)
    {
	send_to_char("Чтобы рассечь, надо держать оружие двумя руками!\n\r", ch);
	return;
    }

    if (is_affected(ch, gsn_cleave))
    {
	printf_to_char("Ты еще не готов%s применить такой мощный удар.\n\r", ch, SEX_ENDING(ch));
	return;
    }

    if (victim->hit < 5 * victim->max_hit / 6)
    {
	sprintf(arg, "$N ранен%s и очень осторож%s...",
		SEX_ENDING(victim),
		victim->sex == SEX_FEMALE ? "на" :
		victim->sex == SEX_MALE   ? "ен" :
		victim->sex == SEX_MANY   ? "ны" : "но");

	act(arg, ch, NULL, victim, TO_CHAR);
	return;
    }

    if (victim->fighting)
    {
	act("$N слишком быстро передвигается в бою.", ch, NULL, victim, TO_CHAR);
	return;
    }

    if (is_safe(ch, victim))
	return;

    /*    if (IS_NPC(victim) &&
     victim->fighting != NULL &&
     !is_same_group(ch, victim->fighting))
     {
     send_to_char("Kill stealing is not permitted.\n\r", ch);
     return;
     } */

    /* modifiers */
    chance -= get_skill(victim, gsn_cleave)/4;

    /* size  and weight */
    chance += ch->carry_weight / 50;
    chance += 5*(SIZE_MEDIUM - victim->size);

    /* stats */
    chance += get_curr_stat(ch, STAT_DEX) - get_curr_stat(victim, STAT_DEX);
    chance += get_curr_stat(ch, STAT_STR) - 15;

    chance += GET_AC(victim, AC_SLASH) / 20;

    /* speed */
    if (IS_SET(ch->off_flags, OFF_FAST) || IS_AFFECTED(ch, AFF_HASTE))
	chance += 10;
    if (IS_SET(victim->off_flags, OFF_FAST) || IS_AFFECTED(victim, AFF_HASTE))
	chance -= 20;

    /* skills */
    /*    chance += get_skill(victim, gsn_parry)/20; */


    if (!IS_NPC(victim) && chance < get_skill(victim, gsn_dodge))
	chance -= (get_skill(victim, gsn_dodge) - chance);

    chance /= 2;   /* Some Fix */

    chance += (int) sqrt(UMAX(0 , prim->weight));

    /* level */
    chance += 2 * (ch->level - victim->level);

    chance += (int) sqrt(20 * skill);


    if (ch->level + PK_RANGE < victim->level
	|| check_immune(victim, DAM_SLASH) > 99)
    {
	chance = 0;
    }


    check_killer(ch, victim);

    if (prim->value[0] == WEAPON_SWORD
	|| prim->value[0] == WEAPON_AXE
	|| prim->value[0] == WEAPON_POLEARM)
    {
	AFFECT_DATA af;

	act("{5$n замахивается, пытаясь рассечь тебя!{x",
	    ch, NULL, victim, TO_VICT);
	act("{5$n замахивается, пытаясь рассечь $N3!{x",
	    ch, NULL, victim, TO_NOTVICT);
	act("{5Ты замахиваешься, пытаясь рассечь $N3!{x",
	    ch, NULL, victim, TO_CHAR);

	if (!IS_NPC(victim))
	{
	    af.where = TO_AFFECTS;
	    af.type = gsn_cleave;
  	    af.level = ch->level;
	    af.duration = 5;
	    af.modifier = 0;
	    af.location = APPLY_NONE;
	    af.bitvector = 0;
	    af.caster_id = get_id(ch);
	    affect_to_char(ch, &af);
	}

	if (number_range(1, 1000) < chance)
	{
	    act("{2$n разрубает тебя!{x", ch, NULL, victim, TO_VICT);
	    act("{3$n разрубает $N3!{x", ch, NULL, victim, TO_NOTVICT);
	    act("{0Ты разрубаешь $N3 на две половинки!{x",
		ch, NULL, victim, TO_CHAR);
	    check_improve(ch, victim, gsn_cleave, TRUE, 1);

	    make_worse_condition(victim, WEAR_BODY,
				 number_range(5, 30), DAM_SLASH);
	    make_worse_condition(victim, WEAR_HEAD,
				 number_range(5, 30), DAM_SLASH);
	    make_worse_condition(victim, WEAR_NECK_1,
				 number_range(5, 10), DAM_SLASH);
	    make_worse_condition(victim, WEAR_NECK_2,
				 number_range(5, 10), DAM_SLASH);
	    make_worse_condition(victim, WEAR_ABOUT,
				 number_range(5, 30), DAM_SLASH);

	    if (!IS_NPC(victim))
	    {
		sprintf(arg, "%s cleaved by %s at %d",
			victim->name,
			(IS_NPC(ch) ? ch->short_descr : ch->name),
			ch->in_room->vnum);
		log_string(arg);
	    }
	    for_killed_skills(ch, victim);
	}
	else
	{
	    check_improve(ch, victim, gsn_cleave, FALSE, 1);
	    adv_dam(ch, victim, dice(skill, 3), TYPE_HIT + 21, DAM_SLASH,
		    TRUE, NULL, FALSE, gsn_cleave);
	}
	WAIT_STATE(ch, skill_table[gsn_cleave].beats);

    }
    else
	send_to_char("Для этого тебе надо вооружиться мечом, "
		     "топором или алебардой.\n\r", ch);

}


/*******
* ХВОСТ
********/
void do_tail(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;

    one_argument(argument, arg);

    if ((chance = get_skill(ch, gsn_tail)) < 1 || (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_TAIL)))
    {
	if (!IS_NPC(ch) && IS_SET(race_table[ch->race].parts, PART_TAIL))
	    send_to_char("Ты еще не можешь управляться со своим хвостом.\n\r", ch);
	else
	    send_to_char("А разве у тебя есть хвост? Странно...\n\r", ch);

	return;
    }

    if (MOUNTED(ch))
    {
	send_to_char("Ты не можешь воспользоваться своим хвостом с лошади!\n\r", ch);
	return;
    }

    if (arg[0] == '\0')
    {
	victim = ch->fighting;
	if (victim == NULL)
	{
	    send_to_char("Но ты ни с кем не сражаешься!\n\r", ch);
	    return;
	}
    }
    else if ((victim = get_char_room(ch, NULL, arg, TRUE)) == NULL)
    {
	if (!too_many_victims)
	    send_to_char("Таких здесь нет.\n\r", ch);
	return;
    }

    if (MOUNTED(victim))
    {
	send_to_char("Ты не можешь сбить своим хвостом того, кто сидит на лошади.\n\r", ch);
	return;
    }

    if (is_safe(ch, victim))
	return;

    /*    if (IS_NPC(victim) &&
     victim->fighting != NULL &&
     !is_same_group(ch, victim->fighting))
     {
     send_to_char("Kill stealing is not permitted.\n\r", ch);
     return;
     }  */

    /*
     if (IS_AFFECTED(victim, AFF_FLYING))
     {
     act("Увы, ноги $N1 не стоят на земле.", ch, NULL, victim, TO_CHAR);
     return;
     }
     */
    if (victim->position < POS_FIGHTING)
    {
	act("$N уже лежит на земле.", ch, NULL, victim, TO_CHAR);
	return;
    }

    if (victim == ch)
    {
	send_to_char("Как ты себе это представляешь?\n\r", ch);
	return;
    }

    if (!IS_SET(victim->parts, PART_LEGS))
    {
	send_to_char("И как это ты, интересно, собираешься сбить хвостом с ног "
		     "существо без оных?\n\r", ch);
	return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
    {
	act("$N - твой любимый хозяин.", ch, NULL, victim, TO_CHAR);
	return;
    }

    /* modifiers */

    chance /= 2;

    chance -= get_skill(victim, gsn_tail)/4;

    /* size */
    if (ch->size < victim->size)
	chance += (ch->size - victim->size) * 10;  /* bigger = harder to trip */

    /* dex */
    chance += get_curr_stat(ch, STAT_DEX);
    chance -= get_curr_stat(victim, STAT_DEX) * 3 / 2;

    /* speed */
    if (IS_SET(ch->off_flags, OFF_FAST) || IS_AFFECTED(ch, AFF_HASTE))
	chance += 10;
    if (IS_SET(victim->off_flags, OFF_FAST) || IS_AFFECTED(victim, AFF_HASTE))
	chance -= 20;

    /* level */
    chance += (ch->level - victim->level) * 2;

    /* race SNAKE */
    if (victim->race == RACE_SNAKE)
	chance *= 0.9;

    if (!IS_SET(victim->form, FORM_BIPED))
	chance = chance * 2 / 3;

    check_killer(ch, victim);

    /* now the attack */
    if (number_percent() < chance)
    {
	CHAR_DATA *ride;
	int dam;

	act("{5$n оборачивается вокруг себя и сбивает тебя с ног своим хвостом!{x", ch, NULL, victim, TO_VICT);
	act("{5Ты оборачиваешься вокруг себя и сбиваешь с ног $N3 своим хвостом!{x", ch, NULL, victim, TO_CHAR);
	act("{5$n оборачивается вокруг себя и сбивает с ног $N3 своим хвостом!{x", ch, NULL, victim, TO_NOTVICT);
	check_improve(ch, victim, gsn_tail, TRUE, 1);

	DAZE_STATE(victim, 2 * PULSE_VIOLENCE);
	WAIT_STATE(victim, skill_table[gsn_tail].beats);
	WAIT_STATE(ch, skill_table[gsn_tail].beats);

	victim->position = POS_BASHED;
	damage(ch, victim, (dam = number_range(2, 2 +  2 * victim->size)),
	       gsn_tail, DAM_BASH, TRUE, NULL);

	if ((ride = RIDDEN(victim)) != NULL)
	{
	    act("{5$n падает с $N1 $x!{x", ride, NULL, victim, TO_ROOM);
	    act("{5Ты падаешь с $N1 $x!{x", ride, NULL, victim, TO_CHAR);
	    damage(ch, ride, dam/2, gsn_tail, DAM_BASH, TRUE, NULL);

	    if (number_percent() < get_skill(ride, gsn_recovery))
		mount_success(ride, victim, FALSE, TRUE);
	    else
		ride->position = POS_BASHED;
	}

    }
    else
    {
	damage(ch, victim, 0, gsn_tail, DAM_BASH, TRUE, NULL);
	WAIT_STATE(ch, skill_table[gsn_tail].beats*2/3);
	check_improve(ch, victim, gsn_tail, FALSE, 1);
    }
}

void do_fight_off(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
    int skill;

    if ((skill = get_skill(ch, gsn_fight_off)) < 1)
    {
	send_to_char("Отбить? Это как?\n\r", ch);
	return;
    }

    if (!POS_FIGHT(ch))
    {
	send_to_char("Но ты же ни с кем не сражаешься!\n\r", ch);
	return;
    }

    if (is_affected(ch, gsn_fight_off) || number_percent() > skill)
	return;

    af.where     = TO_AFFECTS;
    af.type      = gsn_fight_off;
    af.level     = ch->level;
    af.duration  = 1;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(ch, &af);

    WAIT_STATE(ch, skill_table[gsn_fight_off].beats);

    return;
}

char * get_style(CHAR_DATA *ch)
{
    switch (fix_style(ch))
    {
    case NORM_STYLE: return STRING_NORM;
    case AGGR_STYLE: return STRING_AGGR;
    case CARE_STYLE: return STRING_CARE;
    default: return "неизвестен";
    }
}

void do_style(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (arg[0] == '\0' || !str_prefix(arg, "показать"))
    {
	sprintf(arg, "Твой текущий стиль ведения боя - %s.\n\r", get_style(ch));
	send_to_char(arg, ch);
	return;
    }

    if (!str_prefix(arg, STRING_NORM))
    {
	ch->style = NORM_STYLE;
	send_to_char("Ты выбираешь нормальный стиль боя.\n\r", ch);
	return;
    }

    if (!str_prefix(arg, STRING_AGGR))
    {
	if (get_skill(ch, gsn_aggr_style) < 1)
	{
	    send_to_char("У тебя абсолютно отсутствует опыт в этом стиле боя.\n\r", ch);
	    return;
	}

	ch->style = AGGR_STYLE;
	send_to_char("Ты выбираешь агрессивный стиль боя.\n\r", ch);
	WAIT_STATE(ch, skill_table[gsn_aggr_style].beats);
	return;
    }

    if (!str_prefix(arg, STRING_CARE))
    {
	if (get_skill(ch, gsn_care_style) < 1)
	{
	    send_to_char("У тебя абсолютно отсутствует опыт в этом стиле боя.\n\r", ch);
	    return;
	}

	ch->style = CARE_STYLE;
	send_to_char("Ты выбираешь оборонительный стиль боя.\n\r", ch);
	WAIT_STATE(ch, skill_table[gsn_care_style].beats);
	return;
    }

    send_to_char("Такого стиля ведения боя не существует!\n\r", ch);
    return;
}

void do_deafen(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;
    OBJ_DATA *prim;
    OBJ_DATA *sec;
    bool found = FALSE;
    AFFECT_DATA af;

    if ((chance = get_skill(ch, gsn_deafen)) < 1)
    {
        send_to_char("Оглушить? Это как?\n\r", ch);
        return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        victim = ch->fighting;
        if (victim == NULL || victim->position == POS_DEAD)
        {
            send_to_char("Но ты ни с кем не сражаешься!\n\r", ch);
            return;
        }
    }
    else if ((victim = get_char_room(ch, NULL, arg, TRUE)) == NULL)
    {
        if (!too_many_victims)
            send_to_char("Здесь таких нет.\n\r", ch);
        return;
    }

    if (victim == ch)
    {
        send_to_char("Ты это серьезно?\n\r", ch);
        return;
    }
	
    if (is_safe(ch, victim))
        return;

    if (check_charmees(ch, victim))
        return;

    prim = get_eq_char (ch, WEAR_WIELD);
    sec = get_eq_char (ch, WEAR_SECONDARY);

    /* modifiers */

    chance -= get_skill(victim, gsn_deafen) / 4;

    /* size  and weight */
    /*    chance += ch->carry_weight / 250;
    chance -= victim->carry_weight / 200; */

    chance += 3 * (ch->size - victim->size);

    /* stats */
    chance += (get_curr_stat(ch, STAT_DEX) - get_curr_stat(victim, STAT_DEX)) * 4/3;

    chance += 2 * (get_curr_stat(ch, STAT_STR)-get_curr_stat(victim, STAT_STR));

    chance += GET_AC(victim, AC_BASH) / 25;

    /* speed */
/*Изменен параметр шанса с 10 -20, на 15, -15*/
    if (IS_SET(ch->off_flags, OFF_FAST) || IS_AFFECTED(ch, AFF_HASTE))
        chance += 15;
    if (IS_SET(victim->off_flags, OFF_FAST) || IS_AFFECTED(victim, AFF_HASTE))
        chance -= 15;

    /* level */
    chance += 2 * (ch->level - victim->level);

    /* skills */
/*    chance -= get_skill(victim, gsn_dodge) / 20;*/

    chance /= 1.5;  /* Some Fix */

    check_killer(ch, victim);

    /* now the attack */
    if (prim != NULL
        && (prim->value[0] == WEAPON_STAFF
            || prim->value[0] == WEAPON_MACE
            || prim->value[0] == WEAPON_FLAIL))
    {
        found = TRUE;

        if (number_percent() < chance)
        {
            send_to_char("{5Ты наносишь оглушающий удар!{x\n\r", ch);
            check_improve(ch, victim, gsn_deafen, TRUE, 1);
            if (one_hit(ch, victim, TYPE_HIT + 7, FALSE)
                && victim != NULL
                && 2 * chance / 5 + get_obj_weight(prim)/20 > number_percent()
		&& victim->position != POS_DEAD
                && !is_affected(victim, gsn_deafenstrike))
            {
                af.where     = TO_AFFECTS;
                af.type      = gsn_deafenstrike;
                af.level     = ch->level;
                af.duration  = get_obj_weight(prim)/75;
                af.location  = APPLY_HITROLL;
                af.modifier  = - (ch->level/10 + get_obj_weight(prim)/50 + 1);

                af.bitvector = 0;
                af.caster_id = get_id(ch);
                affect_to_char(victim, &af);

/*Увеличили снимаемую инту c 2 до 5 на 51 уровне*/
                if (ch->level >= 12)
                {
                    af.location  = APPLY_INT;
                    af.modifier  = -ch->level/12;
                    affect_to_char(victim, &af);
                }

		WAIT_STATE(victim, 2 * skill_table[gsn_deafenstrike].beats);
                act("{5Ты оглушаешь $N3!{x", ch, NULL, victim, TO_CHAR);
                act("{5$n оглушает $N3!{x", ch, NULL, victim, TO_NOTVICT);
                act("{5$n оглушает тебя! У тебя из глаз сыпятся искры...{x",
                    ch, NULL, victim, TO_VICT);
            }
        }
        else
        {
            act("Твой оглушающий удар не достает $N3!",
                ch, NULL, victim, TO_CHAR);
            act("Оглушающий удар $n1 не достает $N3!",
                ch, NULL, victim, TO_NOTVICT);
            act("Оглушающий удар $n1 не достает тебя!",
                ch, NULL, victim, TO_VICT);
            damage(ch, victim, 0, gsn_deafen, DAM_BASH, FALSE, NULL);
            check_improve(ch, victim, gsn_deafen, FALSE, 1);
        }
    }

    if (IS_VALID(victim) && victim->position != POS_DEAD && sec != NULL
        && (sec->value[0] == WEAPON_STAFF
            || sec->value[0] == WEAPON_MACE
            || sec->value[0] == WEAPON_FLAIL))
    {
        found=TRUE;

        if (victim != NULL && victim->position != POS_DEAD)
        {
            chance -= 100-get_skill(ch, gsn_deafen);
            if (number_percent() < chance)
            {
                send_to_char("{5Ты наносишь оглушающий удар!{x\n\r", ch);
                check_improve(ch, victim, gsn_deafen, TRUE, 1);
                if (one_hit(ch, victim, TYPE_HIT + 7, TRUE)
                    && 2 * chance / 5 + get_obj_weight(sec)/20 > number_percent()
		    && victim->position != POS_DEAD
                    && !is_affected(victim, gsn_deafenstrike))
                {
                    af.where     = TO_AFFECTS;
                    af.type      = gsn_deafenstrike;
                    af.level     = ch->level;
                    af.duration  = get_obj_weight(sec)/75;
                    af.location  = APPLY_HITROLL;
                    af.modifier  = -(ch->level/10 + get_obj_weight(sec)/50 + 1);

                    af.bitvector = 0;
                    af.caster_id = get_id(ch);
                    affect_to_char(victim, &af);

/* На второе оружие шанс снять инту не было*/
                    if (ch->level >= 12)
                    {
                        af.location  = APPLY_INT;
                        af.modifier  = -ch->level/12;
                        affect_to_char(victim, &af);
                    }

		    WAIT_STATE(victim, 2 * skill_table[gsn_deafenstrike].beats);
                    act("{5Ты оглушаешь $N3!{x", ch, NULL, victim, TO_CHAR);
                    act("{5$n оглушает $N3!{x", ch, NULL, victim, TO_NOTVICT);
                    act("{5$n оглушает тебя! У тебя из глаз сыпятся искры...{x",
                        ch, NULL, victim, TO_VICT);
                }
            }
            else
            {
                act("Твой оглушающий удар не достает $N3!",
                    ch, NULL, victim, TO_CHAR);
                act("Оглушающий удар $n1 не достает $N3!",
                    ch, NULL, victim, TO_NOTVICT);
                act("Оглушающий удар $n1 не достает тебя!",
                    ch, NULL, victim, TO_VICT);
                damage(ch, victim, 0, gsn_deafen, DAM_BASH, FALSE, NULL);
                check_improve(ch, victim, gsn_deafen, FALSE, 1);
            }
        }
    }

    if (!found)
        send_to_char("Для этого тебе надо вооружиться булавой, кистенем "
                     "или посохом.\n\r", ch);
    else
        WAIT_STATE(ch, skill_table[gsn_deafen].beats);
}

void do_select(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int skill;

    one_argument(argument, arg);

    if ((skill = get_skill(ch, gsn_select)) == 0)
    {
	send_to_char("Чего?\n\r", ch);
	return;
    }

    if (ch->fighting == NULL)
    {
	send_to_char("Но ты же ни с кем не сражаешься!\n\r", ch);
	return;
    }

    if (arg[0] == '\0')
    {
	send_to_char("Выбрать кого?\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, NULL, arg, TRUE)) == NULL)
    {
	if (!too_many_victims)
	    send_to_char("Таких здесь нет.\n\r", ch);
	return;
    }

    if (victim == ch)
    {
	send_to_char("Бить себя?\n\r", ch);
	return;
    }

    if (is_safe(ch, victim))
	return;

    if (victim == ch->fighting)
    {
	send_to_char("Ты и так бьешь его!\n\r", ch);
	return;
    }
    if (is_same_group(ch, victim))
    {
	act("Но $N ведь заодно с тобой!{x", ch, NULL, victim, TO_CHAR);
	return;
    }


    skill = 2 * skill/3 - (ch->fighting->level - ch->level);

    if (number_percent() < skill)
    {
	act("{5$n переключает свое внимание на тебя!{x",
	    ch, NULL, victim, TO_VICT);
	act("{5Ты переключаешь свое внимание на $N3!{x",
	    ch, NULL, victim, TO_CHAR);
	act("{5$n переключает свое внимание на $N3!{x",
	    ch, NULL, victim, TO_NOTVICT);
	ch->fighting = victim;
	check_killer(ch, victim);

	if (victim->fighting == NULL)
	    multi_hit(victim, ch, TYPE_UNDEFINED);

	check_improve(ch, victim, gsn_select, TRUE, 1);
    }
    else
    {
	act("{5$n пытается переключить свое внимание на тебя, но у $s1 "
	    "не получается!{x", ch, NULL, victim, TO_VICT);
	act("{5Ты пытаешься переключить свое внимание на $N3, но что-то "
	    "не выходит!{x", ch, NULL, victim, TO_CHAR);
	act("{5$n пытается переключить свое внимание на $N3, но что-то "
	    "не выходит!{x", ch, NULL, victim, TO_NOTVICT);

	check_improve(ch, victim, gsn_select, FALSE, 1);
    }
    WAIT_STATE(ch, skill_table[gsn_select].beats);

    return;
}

void do_ambush(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim = NULL;
    bool move = FALSE;
    int sn = gsn_ambush;

    if (ch->fighting != NULL && get_skill(ch, gsn_ambush_move) > 0)
    {
	move = TRUE;
	sn = gsn_ambush_move;
    }

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	if (!move)
	{
	    send_to_char("На кого засаду готовить будем?\n\r", ch);
	    return;
	}
	else if ((victim = ch->fighting) == NULL)
	{
	    send_to_char("Но ты ни с кем не сражаешься!\n\r", ch);
	    return;
	}
    }

    if (victim == NULL)
	victim = get_char_room(ch, NULL, arg, TRUE);

    if (victim == NULL)
    {
	if (!too_many_victims)
	    send_to_char("Таких тут нет!\n\r", ch);
	return;
    }

    if (ch->fighting != NULL && !move)
    {
	send_to_char("Каким образом?! Ты же дерешься!..\n\r", ch);
	return;
    }

    if (victim == ch)
    {
	send_to_char("От себя не спрячешься!!!\n\r", ch);
	return;
    }

    if ((sn == gsn_ambush && !IS_AFFECTED(ch, AFF_CAMOUFLAGE))
	|| (sn == gsn_ambush_move && !IS_AFFECTED(ch, AFF_CAMOUFLAGE_MOVE)))
    {
	printf_to_char("Интересно, как ты хотел%s устроить засаду без "
		       "камуфляжа?\n\r", ch, SEX_ENDING(ch));
	return;
    }

    if (!IS_IN_WILD(ch))
    {
	send_to_char("Только не тут!\n\r", ch);
	return;
    }

    if (is_safe(ch, victim))
	return;

    if (victim->hit < victim->max_hit * 3/4)
    {
	sprintf(arg, "$N ранен%s и слишком осторож%s...",
		SEX_ENDING(victim),
		victim->sex == SEX_FEMALE ? "на" :
		victim->sex == SEX_MALE   ? "ен" :
		victim->sex == SEX_MANY   ? "ны" : "но");

	act(arg, ch, NULL, victim, TO_CHAR);
	return;
    }

    if (get_eq_char(ch, WEAR_WIELD) == NULL 
     && get_eq_char(ch, WEAR_SECONDARY) == NULL
     && get_skill(ch, gsn_hand_to_hand) < 1)
    {
        send_to_char("Голыми руками? Ты так не умеешь.\n\r", ch);
        return;
    }

    check_killer(ch, victim);
    WAIT_STATE(ch, skill_table[sn].beats);

    if (number_percent() < get_skill(ch, sn)
	|| (!IS_AWAKE(victim) && get_skill(ch, sn) > 1))
    {
		check_improve(ch, victim, sn, TRUE, 2);
		multi_hit(ch, victim, sn);
    }
    else
    {
		check_improve(ch, victim, sn, FALSE, 2);
		damage(ch, victim, 0, sn, DAM_NONE, TRUE, NULL);
    }
}

void do_hoof(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim, *mount;
    int chance;
    AFFECT_DATA af;

    if ((mount = MOUNTED(ch)) == NULL)
    {
	send_to_char("Так ты ж не на лошади!\n\r", ch);
	return;
    }

    if ((chance = get_skill(ch, gsn_hoof)) <= 0)
    {
	send_to_char("Ты не умеешь заставлять вставать на дыбы свою лошадь.\n\r", ch);
	return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	victim = ch->fighting;
	if (victim == NULL || victim->position == POS_DEAD)
	{
	    send_to_char("Но ты ни с кем не сражаешься!\n\r", ch);
	    return;
	}
    }
    else if ((victim = get_char_room(ch, NULL, arg, TRUE)) == NULL)
    {
	if (!too_many_victims)
	    send_to_char("Здесь таких нет.\n\r", ch);
	return;
    }

    if (victim == ch)
    {
	send_to_char("Ты это серьезно?\n\r", ch);
	return;
    }

    if (is_safe(ch, victim))
	return;

    if (check_charmees(ch, victim))
	return;

    /* modifiers */

    chance -= get_skill(victim, gsn_hoof)/5;

    /* size  and weight */
    /*    chance += ch->carry_weight / 250;
     chance -= victim->carry_weight / 200; */

    chance += (mount->size - victim->size) / 2;

    /* stats */
    chance += (get_curr_stat(mount, STAT_DEX) - get_curr_stat(victim, STAT_DEX))/3;

    chance += GET_AC(victim, AC_BASH)/25;

    /* speed */
    if (IS_SET(mount->off_flags, OFF_FAST) || IS_AFFECTED(mount, AFF_HASTE))
	chance += 10;
    if (IS_SET(victim->off_flags, OFF_FAST) || IS_AFFECTED(victim, AFF_HASTE))
	chance -= 15;

    /* level */
    chance += ((ch->level + mount->level)/2 - victim->level);

    /* skills */
    chance -= get_skill(victim, gsn_dodge) / 20;

    /* now the attack */

    check_killer(ch, victim);

    if (number_percent() < chance)
    {
	act("{5$n поднимается на дыбы и $t!{x", mount,
	    IS_SET(mount->parts, PART_CLAWS)
	    ? "полосует тебя когтями"
	    : "сокрушительно бьет тебя копытами",
	    victim, TO_VICT);
	act("{5$n поднимается на дыбы и $t $N3!{x", mount,
	    IS_SET(mount->parts, PART_CLAWS)
	    ? "полосует когтями"
	    : "сокрушительно бьет копытами",
	    victim, TO_NOTVICT);

	check_improve(ch, victim, gsn_hoof, TRUE, 1);
	damage(mount, victim, dice(mount->level, 5), gsn_hoof,
	       IS_SET(mount->parts, PART_CLAWS) ? DAM_SLASH : DAM_BASH,
	       TRUE, NULL);

	if (!IS_SET(mount->parts, PART_CLAWS)
	    && victim != NULL
	    && chance / 4 > number_percent()
	    && !is_affected(ch, gsn_deafen))
	{
	    af.where     = TO_AFFECTS;
	    af.type      = gsn_deafen;
	    af.level     = mount->level;
	    af.duration  = 0;
	    af.location  = APPLY_HITROLL;
	    af.modifier  = - ((ch->level + mount->level)/20 + 1);

	    af.bitvector = 0;
	    af.caster_id = get_id(ch);
	    affect_to_char(victim, &af);

	    act("{5$n оглушает тебя!{x", mount, NULL, victim, TO_VICT);
	    act("{5$n оглушает $N3!{x", mount, NULL, victim, TO_NOTVICT);
	}
    }
    else
    {
	damage(ch, victim, 0, gsn_hoof, DAM_BASH, FALSE, NULL);
	act("{5$n поднимается на дыбы, но не достает тебя своими $t.{x",
	    mount, IS_SET(mount->parts, PART_CLAWS) ? "когтями" : "копытами",
	    victim, TO_VICT);
	act("{5$n поднимается на дыбы, но не достает своими $t $N3!{x",
	    mount, IS_SET(mount->parts, PART_CLAWS) ? "когтями" : "копытами",
	    victim, TO_NOTVICT);
	check_improve(ch, victim, gsn_hoof, FALSE, 1);
    }
    WAIT_STATE(ch, skill_table[gsn_hoof].beats);
}

void do_bite(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    AFFECT_DATA af, *paf;
    int chance = 0, dam = 0, add = 0;

    one_argument(argument, arg);

    if ((chance = get_skill(ch, gsn_bite)) == 0)
    {
	send_to_char("Укусить? Это как?!\n\r", ch);
	return;
    }

    if (arg[0] == '\0')
    {
	send_to_char("Кого укусить?!\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, NULL, arg, TRUE)) == NULL)
    {
	if (!too_many_victims)
	    send_to_char("Таких здесь нет.\n\r", ch);
	return;
    }

    if (victim == ch)
    {
	send_to_char("Ты долго и безуспешно пытаешься дотянуться клыками до своей шеи...\n\r", ch);
	return;
    }

    if (IS_VAMPIRE(victim))
    {
	send_to_char("Ты хочешь укусить вампира? Сородичи тебе этого не простят!\n\r", ch);
	return;
    }

    if (IS_NPC(victim) && IS_SET(victim->act, ACT_UNDEAD))
    {
	send_to_char("Ты думаешь, что у мертвых есть кровь?\n\r", ch);
	return;
    }

    if (!IS_SET(victim->form, FORM_HAS_BLOOD)
	&& (IS_SET(victim->form, FORM_MIST)
	    || IS_SET(victim->form, FORM_INTANGIBLE)
	    || IS_SET(victim->form, FORM_UNDEAD)
	    || IS_SET(victim->form, FORM_CONSTRUCT)
	    || IS_SET(victim->form, FORM_INSECT)
	    || IS_SET(victim->form, FORM_MAGICAL)
	    || IS_SET(victim->form, FORM_BLOB)
	    || IS_SET(victim->form, FORM_BLOODLESS)
	    || IS_SET(victim->form, FORM_TREE)
	    || IS_SET(victim->form, FORM_INSECT)
	    || IS_SET(victim->form, FORM_SPIDER)
	    || IS_SET(victim->form, FORM_CRUSTACEAN)
	    || IS_SET(victim->form, FORM_WORM)))
    {
	send_to_char("У таких существ нет крови.\n\r", ch);
	return;
    }

    if (check_charmees(ch, victim))
	return;

    if (is_safe(ch, victim))
	return;

    if (can_see(victim, ch) && IS_AWAKE(victim))
    {
	act("Тебе не удается незаметно подкрасться к $N2.", ch, NULL, victim, TO_CHAR);
	return;
    }

    if (victim->max_hit < ch->max_hit / 4)
    {
	act("$E слишком слаб$t для тебя.", ch, SEX_ENDING(victim), victim, TO_CHAR);
	return;
    }

    if (is_affected(victim, gsn_bite))
    {
	send_to_char("У этого существа ты не сможешь выпить сколько-нибудь крови - до тебя уже постарались.\n\r", ch);
	return;
    }

    chance += (get_curr_stat(ch, STAT_DEX) - get_curr_stat(victim, STAT_DEX));
    chance += (ch->level - victim->level) * 2;

    if (IS_AFFECTED(ch, AFF_HASTE))
	chance += 10;
    if (IS_AFFECTED(victim, AFF_HASTE))
	chance -= 15;
    if (IS_AFFECTED(ch, AFF_SLOW))
	chance -= 15;
    if (IS_AFFECTED(victim, AFF_SLOW))
	chance += 10;

    if (IS_AFFECTED(victim, AFF_SANCTUARY))
	chance -= 30;

    if (!IS_AWAKE(victim))
	chance += 20;
    else
	chance -= 5;

    if (chance < number_percent())
    {
	send_to_char("Неудачно!\n\r", ch);
	damage(ch, victim, 0, gsn_bite, DAM_NEGATIVE, FALSE, NULL);
	check_improve(ch, victim, gsn_bite, FALSE, 2);
	WAIT_STATE(ch, skill_table[gsn_bite].beats);
	return;
    }

    act("{5$n кусает $N3 в шею...{x", ch, NULL, victim, TO_NOTVICT);

    WAIT_STATE(ch, skill_table[gsn_bite].beats);
    WAIT_STATE(victim, skill_table[gsn_bite].beats);

    check_killer(ch, victim);

    dam = dice(ch->level, UMAX(1, (ch->level - victim->level) / 2));

    if (!damage(ch, victim, dam, gsn_bite, DAM_NEGATIVE, TRUE, NULL))
	return;

    sprintf(arg, "{5Ты уже не так голод%s.{x\n\r", ch->sex == SEX_MALE ? "ен" : ch->sex == SEX_FEMALE ? "на" : "но");
    send_to_char(arg, ch);
    send_to_char("{5Ты чувствуешь сильную слабость.{x\n\r", victim);

    check_improve(ch, victim, gsn_bite, TRUE, 2);

    if (IS_AFFECTED(victim, AFF_SANCTUARY))
	dam /= 3;

    ch->hit += 2 * dam / 3;
    ch->mana+= dam / 3;

    /* Добавил, чтобы аффект и на жертву вешался, чтобы одного и того же не кусали 10 раз */
    add = UMAX(0, (2 + victim->level - ch->level) / 2);
    dam = (3 + victim->level - ch->level);

    af.where = TO_AFFECTS;
    af.type = gsn_bite;
    af.level = ch->level;
    af.location = APPLY_STR;
    af.duration = UMAX(0, dam);
    af.modifier = -(ch->level/15);
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);


    af.location = APPLY_DAMROLL;

    /* af.modifier = add; */
    /*
     * Временная мера против возможности получить кучу роллов.
     * Надо бы что-то иное придумать, ИМХО.
     * Хотя бы возможность набирать роллы до определенного
     * момента (как вариант, до level/4)
     */
    if ((paf = affect_find(ch->affected, gsn_bite)) != NULL)
    {
	add = paf->modifier + add;
	af.duration = paf->duration + UMAX(0, dam);
	affect_strip(ch, gsn_bite);
    }
    else
	af.duration = UMAX(0, 2 * dam);

    af.modifier = UMIN(add, ch->level/5);
    af.bitvector = AFF_SATIETY;
    af.caster_id = get_id(ch);
    affect_to_char(ch, &af);

}

bool check_mist_old(CHAR_DATA *ch, CHAR_DATA *victim)
{
    int percent = get_skill(ch, gsn_mist)/1.5;

    percent += (get_curr_stat(victim, STAT_WIS) - get_max_stat(victim, STAT_WIS)) * 5;
    percent += (get_curr_stat(victim, STAT_CON) - get_max_stat(victim, STAT_CON)) * 5;
    percent += (victim->level - ch->level);

    percent *= get_style_multiplier(victim);

    if (number_percent() < percent)
    {
	check_improve(victim, ch, gsn_mist, TRUE, 8);
	return TRUE;
    }

    check_improve(victim, ch, gsn_mist, FALSE, 12);

    return FALSE;
}

bool check_mist(CHAR_DATA *ch, CHAR_DATA *victim) {
    int chance;
    AFFECT_DATA *paf;
	char buf[MIL];

    if (!is_affected(victim, gsn_mist)){
        return FALSE;
    }

    /* Устанавливаем начальный шанс. */
    chance = 5;

    /*Учитываем прокачку заклинания (умения)*/
    chance += get_skill(victim, gsn_mist) / 10;

    /* Находим аффект тумана на себе, для получения левела каста */
    paf = affect_find(victim->affected, gsn_mist);              //15%

    /*Учитываем разницы в уровне врага и нашего каста*/
    chance += paf->level - ch->level; //-10-12% против мобов 60 лвл      //12%



	/* ТУМАН ЗАВИСИЯТ ОТ РАЗНИЦЫ СТАТОВ ДЕРУЩИХСЯ. ДЛЯ МОБОВ СТАТЫ 23 по умолчанию взято с средних статов человека. 2% за 1 стат */
    if (!IS_NPC(ch)) {
        chance += 2 * (get_curr_stat(victim, STAT_WIS) + get_curr_stat(victim, STAT_INT) -
                       get_curr_stat(ch, STAT_WIS) - get_curr_stat(ch, STAT_INT)); // Против тролля воина + 18%; Эльфа мага -8%
    } else {
        chance += 2 * (get_max_stat(victim, STAT_WIS) + get_max_stat(victim, STAT_INT) - 2 * AVG_STAT - 1); //2*3%=6%
    }


	/* СМЫСЛ ОДЕВАТСЬЯ В СТАТЫ, ЕСЛИ НЕ ДООДЕЛИ СТАТЫ - ИМЕЕМ ШТРАФЫ = 3% за 1 стат */
    chance -= 3 * (get_max_stat(victim, STAT_WIS) - get_curr_stat(victim, STAT_WIS));
    chance -= 3 * (get_max_stat(victim, STAT_INT) - get_curr_stat(victim, STAT_INT));


    /*Если агрессор в атакующем стиле - процент понижается (попадает в чара)
    *Если жертва в атакующем стиле - процент понижается (попадает в чара)
    */

    /* Учитываем стили боя: если мы в оборонке +20% и -20% если в атаке
     * Если вараг в оборонке +20% и -20% если враг в атаке. Враг NPC без стиля или стль 1 по дефлоту*/
    chance *= get_style_multiplier(victim) + get_style_multiplier(ch) - 1;

	    /* Если противник больше нас, шанс снижаем, что бы гиганты паладины и тролли воины могли навалять вампиру */
    if (victim->size < ch->size)
        chance -= 5;

	    /*  Слепота половинит шанс*/
    if (IS_AFFECTED(victim, AFF_BLIND)){
        chance /= 2;
    }


    if (number_percent() <= chance) {
		check_improve(victim, ch, gsn_mist, TRUE, 8);
		if (!IS_NPC(victim) && IS_SET(victim->act, PLR_SHOW_DAMAGE)){
			sprintf(buf, "{B(Mist %d){x$n теряет тебя из виду из-за сильного {Wтумана!{x", chance);
			act(buf, ch, NULL, victim, TO_VICT);
		} else {
			act("$n теряет тебя из виду из-за сильного {Wтумана!{x",ch, NULL, victim, TO_VICT);
		}

		act("Ты теряешь $N1 из виду  из-за сильного {Wтумана!{x",ch, NULL, victim, TO_CHAR);

		return TRUE;

    }

    check_improve(victim, ch, gsn_mist, FALSE, 24);

    return FALSE;
}


void do_bandage(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
    int skill;

    if (POS_FIGHT(ch))
    {
	send_to_char("Ты не можешь отвлечься от драки!\n\r", ch);
	return;
    }

    if ((skill = get_skill(ch, gsn_bandage)) < 1)
    {
	send_to_char("Да ты не умеешь как следует перевязывать раны!\n\r", ch);
	return;
    }

    if (is_affected(ch, gsn_bandage))
    {
	act("Ты уже и так обмотан$t бинтами.", ch, SEX_ENDING(ch), NULL, TO_CHAR);
	return;
    }

    if (ch->hit > 2*ch->max_hit/3)
    {
	send_to_char("Да ты, вроде, не так уж и плохо себя чувствуешь.\n\r", ch);
	return;
    }

    if (ch->mana < 30)
    {
	send_to_char("У тебя не хватает энергии.\n\r", ch);
	return;
    }

    send_to_char("{5Ты пытаешься наложить повязки на свои раны.\n\r{x", ch);

    if (number_percent() < skill)
    {
	af.where = TO_AFFECTS;
	af.type = gsn_bandage;
	af.level = ch->level;
	af.location = APPLY_NONE;
	af.duration = ch->level/10;
	af.modifier = 0;
	af.bitvector = IS_AFFECTED(ch, AFF_REGENERATION) ? 0 : AFF_REGENERATION;
	af.caster_id = get_id(ch);
	affect_to_char(ch, &af);

	affect_strip(ch, gsn_bleed);

	send_to_char("{5Ты чувствуешь, что твои раны перестают кровоточить.\n\r{x", ch);

	if (number_percent() < skill)
	{
	    if (skill < 20)
		spell_cure_light(0, ch->level, ch, ch, TARGET_CHAR);
	    else if (skill < 40)
		spell_cure_serious(0, ch->level, ch, ch, TARGET_CHAR);
	    else if (skill < 60)
		spell_cure_critical(0, ch->level, ch, ch, TARGET_CHAR);
	    else
		spell_heal(0, ch->level, ch, ch, TARGET_CHAR);

	    ch->mana -= 30;
	}
	else
	    ch->mana -= 20;

	check_improve(ch, NULL, gsn_bandage, TRUE, 4);
	return;
    }

    check_improve(ch, NULL, gsn_bandage, FALSE, 10);
    ch->mana -= 10;
}

void do_pierce(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim, *mount;
    int chance = 0, skill = 0;
    OBJ_DATA *prim;
    AFFECT_DATA af;

    /*Пусть для защиты от протыкания будет тот же флаг, что и для рассечь: OFF_CLEAVE
    */
    if ((skill = get_skill(ch, gsn_pierce)) < 1
	|| (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_CLEAVE)))
    {
	send_to_char("Проткнуть? Это как?\n\r", ch);
	return;
    }

    /*Проверка, сидит ли чар на коне?
    */
    if ((mount = MOUNTED(ch)) == NULL) 
    {
	send_to_char("Пешком? Без коня? Хе-хе...\n\r", ch);
	return;
    }

    if (ch->fighting != NULL || mount->fighting != NULL)
    {
	send_to_char("Для таранного удара надо разогнаться.\n\r", ch);
	return;
    }

    one_argument(argument, arg);

    if ((victim = get_char_room(ch, NULL, arg, TRUE)) == NULL)
    {
	if (!too_many_victims)
	    send_to_char("Здесь таких нет.\n\r", ch);
	return;
    }

    if (victim == ch)
    {
	send_to_char("Cамого себя? Копьем?\n\r", ch);
	return;
    }

    if (is_safe(ch, victim))
	return;

    if (is_affected(ch, gsn_pierce))
    {
	printf_to_char("Ты еще не готов%s применить такой мощный удар.\n\r", ch, SEX_ENDING(ch));
	return;
    }


    if (victim->hit < 5 * victim->max_hit / 6)
    {
	sprintf(arg, "$N ранен%s и очень осторож%s...",
		SEX_ENDING(victim),
		victim->sex == SEX_FEMALE ? "на" :
		victim->sex == SEX_MALE   ? "ен" :
		victim->sex == SEX_MANY   ? "ны" : "но");

	act(arg, ch, NULL, victim, TO_CHAR);
	return;
    }

    if (check_charmees(ch, victim))
	return;

    /*Проверка - с оружием ли агрессор?

*/
    if ((prim = get_eq_char (ch, WEAR_WIELD)) == NULL || prim->item_type != ITEM_WEAPON)
    {
	send_to_char("Вооружиcь сначала.\n\r", ch);
	return;
    }


    /*Проверка - копье ли у чара? Двуручное ли оно?

*/
    if (prim->value[0] != WEAPON_SPEAR || !IS_WEAPON_STAT(prim, WEAPON_TWO_HANDS))
    {
	send_to_char("Тебе нужно хорошее копье.\n\r",ch);
	return;
    };

    /* modifiers */

    chance -= get_skill(victim, gsn_pierce)/4;

    /* size  and weight */
    chance += ch->carry_weight / 50;

    /* stats */
    chance += get_curr_stat(ch, STAT_DEX) - get_curr_stat(victim, STAT_DEX);
    chance += get_curr_stat(ch, STAT_STR) - 18;

    chance += GET_AC(victim, AC_PIERCE) / 10;

    /* speed */
    if (IS_SET(ch->off_flags, OFF_FAST) || IS_AFFECTED(ch, AFF_HASTE))
	chance += 10;
    if (IS_SET(victim->off_flags, OFF_FAST) || IS_AFFECTED(victim, AFF_HASTE))
	chance -= 20;

    /* skills */

    if (!IS_NPC(victim) && chance < get_skill(victim, gsn_dodge))
	chance -= get_skill(victim, gsn_dodge)/5;

    chance /= 2;   /* Some Fix */

    chance += (int) sqrt(UMAX(0 , prim->weight));

    /* level */
    chance += 2 * (ch->level - victim->level);

    chance += (int) sqrt(20 * skill);


    if (ch->level + PK_RANGE < victim->level || check_immune(victim, DAM_PIERCE) > 99)
	chance = 0;


    /* now the attack */

    check_killer(ch, victim);

    act("{W$n пытается проткнуть тебя таранным ударом копья!{x",
	ch, NULL, victim, TO_VICT);
    act("{W$n пытается проткнуть таранным ударом копья $N3!{x",
	ch, NULL, victim, TO_NOTVICT);
    act("{WТы пытаешься проткнуть таранным ударом копья $N3!{x",
	ch, NULL, victim, TO_CHAR);

    if (!IS_NPC(victim))
    {	
        af.where = TO_AFFECTS;
        af.type = gsn_pierce;
        af.level = ch->level;
        af.duration = 5;
        af.modifier = 0;
        af.location = APPLY_NONE;
        af.bitvector = 0;
        af.caster_id = get_id(ch);
        affect_to_char(ch, &af);
    }

    if (number_range(1, 1000) < chance)
    {
	act("$n протыкает тебя!", ch, NULL, victim, TO_VICT);
	act("$n протыкает $N3!", ch, NULL, victim, TO_NOTVICT);
	act("Ты протыкаешь $N3!", ch, NULL, victim, TO_CHAR);
	check_improve(ch, victim, gsn_pierce, TRUE, 1);

	make_worse_condition(victim, WEAR_BODY,
			     number_range(5, 30), DAM_PIERCE);
	make_worse_condition(victim, WEAR_ABOUT,
			     number_range(5, 30), DAM_PIERCE);

	if (!IS_NPC(victim))
	{
	    sprintf(arg, "%s pierced by %s at %d",
		    victim->name,
		    (IS_NPC(ch) ? ch->short_descr : ch->name),
		    ch->in_room->vnum);
	    log_string(arg);
	}
	for_killed_skills(ch, victim);
    }
    else
    {
	check_improve(ch, victim, gsn_pierce, FALSE, 1);
	adv_dam(ch, victim, dice(skill, 3), TYPE_HIT + 2, DAM_PIERCE,
		TRUE, NULL, FALSE, gsn_pierce);
    }
    WAIT_STATE(ch, skill_table[gsn_pierce].beats);

    return;
}



/*
*сбор - позволяет собрать все, что возможно
*/

void do_collection(CHAR_DATA *ch, char *argument)
{
    int skill, num = 0, t;
    char arg1[MIL], *p;
    OBJ_DATA *getobj;

    if ((skill = get_skill(ch, gsn_collection)) < 1)
    {
	send_to_char("Ты не силен в собирательстве!\n\r", ch);
	return;
    }

    t = URANGE(20, 100 - skill, 40);

    if (ch->move < 2 || ch->mana < t*1.5)
    {
	act("Ты еще не отдохнул$t, чтобы что-то заново собирать.", ch, SEX_ENDING(ch), NULL, TO_CHAR);
	return;
    }

    p = one_argument(argument, arg1);
    if ((getobj = get_obj_here(ch, NULL, arg1)) != NULL
    && getobj->carried_by != ch)
    {
	num = create_ingredient(ch,getobj,NULL,p);
    }
    else
    {
	if (get_room_affect(ch->in_room,gsn_collection) != NULL)
	{
	    send_to_char("Здесь уже все собрали до тебя!\n\r", ch);
	    return;	
	}
	num = create_ingredient(ch,NULL,NULL,argument);
    }

    ch->mana -= t*1.5*num/4;
/*    ch->move -= t*num/4;
*/
    ch->move -= 1;

    if (num == 2)
	WAIT_STATE(ch, skill_table[gsn_collection].beats / 4);
    else
	WAIT_STATE(ch, skill_table[gsn_collection].beats / 8);

    return;
}

void do_mix(CHAR_DATA *ch, char *argument)
{
    RECIPE_DATA *recipe;
    OBJ_DATA *mortar, *obj, *potion;
    int skill, chance = 0;
    char buf[MIL];
    int lev = 0, s = 0, m = 0, i, r = 0, cost[MAX_MORTAR_ING];

    if ((skill = get_skill(ch, gsn_mix)) < 1)
    {
	send_to_char("Смешивать! Это как?\n\r", ch);
	return;
    }

    if (((mortar = get_eq_char(ch, WEAR_HOLD)) == NULL) || (mortar->item_type != ITEM_MORTAR))
    {
	send_to_char("Тебе же необходимо держать ступу в руке.\n\r",ch);
	return;
    }

    if ((obj = mortar->contains) == NULL)
    {
	send_to_char("В ступе же нет ничего.\n\r",ch);
	return;
    }

    if ((potion = get_obj_carry(ch, argument, ch)) == NULL)
    {
	send_to_char("Определи бутылочку, куда ты будешь замешивать?\n\r",ch);
	return;
    }

    if (potion->pIndexData->vnum != OBJ_VNUM_POTION)
    {
	send_to_char("Ты можешь переливать только в произведенных тобой бутылочки.\n\r",ch);
	return;
    }

    if (potion->value[4] > 1)
    {
	send_to_char("Ты больше не можешь добавлять в бутылочку экстракты.\n\r",ch);
	return;
    }
   
    for (recipe = recipe_list; recipe != NULL; recipe = recipe->next)
    {
	bool next;
	lev = 0; 
	m   = 0;
	r   = 0;

	if (recipe->type != RECIPE_POTION)
	    continue;

	for (i = 0; i != MAX_ING; i++)
	{
	    if (recipe->value[i] != 0)
		r++;			/*количество ингредиентов в рецепте */
	    else
		continue;

	    s = 0;
	    next = FALSE; 
	    for (obj = mortar->contains; obj != NULL; obj = obj->next_content)
	    {
		s++;			/*количество ингредиентов в ступе */
		if (!next && obj->cost != -1 && recipe->value[i] == obj->pIndexData->vnum)
		{
		    lev += obj->level;
		    cost[m++] = obj->cost;
		    obj->cost = -1;
		    next = TRUE;
		}
	    }
	}
	i = 0;
	for (obj = mortar->contains; obj != NULL; obj = obj->next_content)
	    if (obj->cost == -1) 
		obj->cost = cost[i++];
	if (r == m && r == s && r != 0)
	{
	    if ((ch->level < recipe->level || skill < 100 - UMAX(5, 15 * (5 - recipe->rlevel)))
		&& number_range(1,1000) > 5)
	    {
		send_to_char("Твой уровень или знания малы, чтобы смешивать ингредиенты.\n\r",ch);
		return;
	    }    

	    for (i=1; i != 5; i++)
	    {
		if (potion->value[i] <= 0)
		{
		    send_to_char("Ты смешиваешь ингредиенты.\n\r",ch);
		    
		    WAIT_STATE(ch, skill_table[gsn_mix].beats);

		    while (mortar->contains != NULL)
			extract_obj(mortar->contains, TRUE, TRUE);

		    chance  = recipe->comp + URANGE(-5, 15 * skill / 100, 15);
		    chance += (2 - i) * 10;
		    chance += (get_curr_stat(ch, STAT_WIS) - 20) * 3;
		    if (is_affected(ch, gsn_concentrate))
			chance *= 1.4;

		    if (number_percent() <= chance)
		    {
			potion->value[i] = skill_lookup(recipe->name);
			potion->value[0] = (potion->value[0] + 2 * ch->level + lev/m) / 4 +
			    UMAX(0, skill - 95);
			potion->level    = UMIN(ch->level, potion->level+3);

			free_string(potion->name);
			sprintf(buf, "зелье %s{x", recipe->name);
			potion->name = str_dup(buf);

			free_string(potion->short_descr);
			sprintf(buf, "зелье с надписью %s{x", recipe->name);
			potion->short_descr = str_dup(buf);

			free_string(potion->description);
			sprintf(buf, "Зелье с надписью %s лежит здесь.{x", recipe->name);
			potion->description = str_dup(buf);

			send_to_char("Ты добавляешь полученный экстракт в бутылочку.\n\r",ch);
			check_improve(ch, NULL, gsn_mix, TRUE, 1 + recipe->rlevel / 3);
		    }
		    else
		    {
			send_to_char("У тебя не получается добавить экстракт в бутылочку, и она взрывается.\n\r",ch);
			check_improve(ch, NULL, gsn_mix, FALSE, UMAX(1,6 - recipe->rlevel));
			extract_obj(potion, TRUE, TRUE);
		    }
		    return;
		}
	    }
	    send_to_char("У тебя не получится добавить в бутылочку еще один экстракт.\n\r",ch);	    
	    return;
	}
    }
    send_to_char("Из таких ингредиентов вряд-ли что получится смешать.\n\r",ch);
    return;
}

void do_pound(CHAR_DATA *ch, char *argument)
{
    RECIPE_DATA *recipe;
    OBJ_DATA *mortar, *obj, *pill;
    int skill, chance = 0;
    char buf[MIL];
    int lev = 0, s = 0, m = 0, i, r = 0, cost[MAX_MORTAR_ING];

    if ((skill = get_skill(ch, gsn_mix)) < 1)
    {
	send_to_char("Столочь. Это как?\n\r", ch);
	return;
    }

    if (((mortar = get_eq_char(ch, WEAR_HOLD)) == NULL) || (mortar->item_type != ITEM_MORTAR))
    {
	send_to_char("Тебе же необходимо держать ступу в руке.\n\r",ch);
	return;
    }

    if ((obj = mortar->contains) == NULL)
    {
	send_to_char("В ступе же нет ничего.\n\r",ch);
	return;
    } 

    for (recipe = recipe_list; recipe != NULL; recipe = recipe->next)
    {
	bool next;
	lev = 0; 
	m   = 0;
	r   = 0;

	if (recipe->type != RECIPE_PILL)
	    continue;
	for (i = 0; i != MAX_ING; i++)
	{
	    if (recipe->value[i] != 0)
		r++;			/*количество ингредиентов в рецепте */
	    else
		continue;

	    s = 0;
	    next = FALSE; 
	    for (obj = mortar->contains; obj != NULL; obj = obj->next_content)
	    {
		s++;			/*количество ингредиентов в ступе */
		if (!next && obj->cost != -1 && recipe->value[i] == obj->pIndexData->vnum)
		{
		    lev += obj->level;
		    cost[m++] = obj->cost;
		    obj->cost = -1;
		    next = TRUE;
		}
	    }
	}
	i = 0;
	for (obj = mortar->contains; obj != NULL; obj = obj->next_content)
	    if (obj->cost == -1) 
		obj->cost = cost[i++];
	if (r == m && r == s && r != 0)
	{
	    if (ch->level < recipe->level || skill < 100 - UMAX(5, 15 * (5 - recipe->rlevel)))
	    {
		send_to_char("Твой уровень или знания малы, чтобы смешивать ингредиенты.\n\r",ch);
		return;
	    }

	    send_to_char("Ты сдавливаешь ингредиенты.\n\r",ch);

	    WAIT_STATE(ch, skill_table[gsn_pound].beats);

	    while (mortar->contains != NULL)
		extract_obj(mortar->contains, TRUE, TRUE);

	    chance  = recipe->comp + URANGE(-5, 15 * skill / 100, 15);
	    chance += (get_curr_stat(ch, STAT_WIS) - 20) * 3;

	    if (number_percent() <= chance)
	    {	
		pill = create_object(get_obj_index(OBJ_VNUM_PILL), 0);

		free_string(pill->name);
		sprintf(buf, "пилюля %s{x", recipe->name);
		pill->name = str_dup(buf);

		free_string(pill->short_descr);
		sprintf(buf, "пилюля '%s'{x", recipe->name);
		pill->short_descr = str_dup(buf);

		pill->value[0] = (2 * ch->level + lev/m) / 3 + UMAX(0, skill - 95);
		pill->value[1] = skill_lookup(recipe->name);
		if (number_percent() <= 75)
		{
		 	pill->value[2] = skill_lookup(recipe->name);
			if (number_percent() <= 50)
			{
				pill->value[3] = skill_lookup(recipe->name);
				if (number_percent() <= 15)
				{
					pill->value[4] = skill_lookup(recipe->name);
				}
			}
		}
		pill->level    = ch->level;
		send_to_char("Ты получаешь пилюлю.\n\r",ch);
		obj_to_char(pill, ch);
		check_improve(ch, NULL, gsn_pound, TRUE, 1 + recipe->rlevel / 2);
	    }
	    else
	    {		
		send_to_char("У тебя не получается что-либо столочь.\n\r",ch);
		check_improve(ch, NULL, gsn_pound, FALSE, UMAX(1, 6 - recipe->rlevel));
	    }	    
	    return;
	}
    }
    send_to_char("Из таких ингредиентов вряд-ли что получится столочь.\n\r",ch);
    return;
}





/* charset=cp1251 */


