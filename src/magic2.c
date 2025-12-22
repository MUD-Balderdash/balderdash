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
#include <math.h>
#include <time.h>
#include "merc.h"
#include "interp.h"
#include "magic.h"
#include "recycle.h"


int	find_door	args((CHAR_DATA *ch, char *arg));
bool 	charm(CHAR_DATA *ch, CHAR_DATA *victim, int level, int sn);
bool 	check_disallow(char *command);

void check_undead(CHAR_DATA *ch, OBJ_DATA *portal)
{
    if ((!IS_NPC(ch) && (ch->classid == CLASS_NECROMANT || ch->classid == CLASS_VAMPIRE)) || (IS_NPC(ch) && IS_UNDEAD(ch))){
	    SET_BIT(portal->value[2], GATE_UNDEAD);
	    SET_BIT(portal->extra_flags, ITEM_EVIL|ITEM_DARK);
    }
}

CHAR_DATA *can_transport(CHAR_DATA *ch, int level)
{
    CHAR_DATA *victim;

    if ((victim = get_char_world(ch, target_name)) == NULL
	|| victim == ch
	|| victim->in_room == NULL
	|| IS_SET(victim->in_room->room_flags, ROOM_SAFE)
	|| IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
	|| IS_SET(victim->in_room->room_flags, ROOM_HOUSE)
	|| IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
	|| !can_see_room(ch, victim->in_room)
	|| room_is_private(victim->in_room, ch->mount)
	|| is_affected(ch, gsn_gods_curse)
	|| victim->level >= level + 3
	|| check_immune(victim, DAM_SUMMON) == 100
	|| (IS_NPC(victim)
	    && !IS_NPC(ch)
	    && ch->pcdata->questmob == victim->pIndexData->vnum)
	|| (IS_NPC(victim) && IS_SET(victim->act, ACT_QUESTER))
	|| saves_spell(level, victim, DAM_SUMMON))
    {
	return NULL;
    }

    return victim;
}

CHAR_DATA *can_portal(CHAR_DATA *ch, int level)
{
    CHAR_DATA *victim;

    if ((victim = get_char_world(ch, target_name)) == NULL
	|| victim == ch
	|| victim->in_room == NULL
	|| IS_SET(victim->in_room->room_flags, ROOM_SAFE)
	|| IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
	|| IS_SET(victim->in_room->room_flags, ROOM_HOUSE)
	|| IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
	|| IS_SET(ch->in_room->room_flags, ROOM_SAFE)
	|| !can_see_room(ch, victim->in_room)
	|| room_is_private(victim->in_room, ch->mount)
	|| is_affected(ch, gsn_gods_curse)
	|| victim->level >= level + 3
	|| (IS_NPC(victim)
	    && !IS_NPC(ch)
	    && ch->pcdata->questmob == victim->pIndexData->vnum)
	|| (IS_NPC(victim) && IS_SET(victim->act, ACT_QUESTER)))
    {
	return NULL;
    }

    return victim;
}

bool spell_farsight(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    if (IS_AFFECTED(ch, AFF_BLIND))
    {
	send_to_char("Но ты же вообще ничего не видишь!\n\r", ch);
	return FALSE;
    }

    do_function(ch, &do_scan, target_name);
    return TRUE;
}


bool spell_portal(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *portal, *stone;

    if (IS_NULLSTR(target_name))
    {
	send_to_char("Интересно, на кого ты хотел прыгнуть?\n\r", ch);
	return FALSE;
    }

    stone = get_eq_char(ch, WEAR_HOLD);
    if (!IS_IMMORTAL(ch)
	&& (stone == NULL || stone->item_type != ITEM_WARP_STONE))
    {
	send_to_char("У тебя нет необходимого компонента "
		     "для этого заклинания.\n\r", ch);
	return FALSE;
    }

    if ((victim = can_portal(ch, level)) == NULL
	|| !can_see_room(ch, victim->in_room)
	|| IS_SET(victim->in_room->room_flags, ROOM_NOGATE)
	|| IS_SET(ch->in_room->room_flags, ROOM_NOGATE)
	|| IS_SET(victim->in_room->room_flags, ROOM_KILL)
	|| IS_SET(victim->in_room->room_flags, ROOM_NOFLY_DT)
	|| IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
	|| IS_AFFECTED(ch, AFF_CURSE)
	|| (saves_spell(level, victim, DAM_SUMMON)
	    && number_percent() < 50))
    {
	send_to_char("У тебя ничего не получается.\n\r", ch);
	return TRUE;
    }


    if (stone != NULL && stone->item_type == ITEM_WARP_STONE)
    {
	act("Ты получаешь всю энергию $p1!", ch, stone, NULL, TO_CHAR);
	act("$p ярко вспыхивает и исчезает!", ch, stone, NULL, TO_CHAR);
	extract_obj(stone, TRUE, TRUE);
    }

    portal = create_object(get_obj_index(OBJ_VNUM_PORTAL), 0);
    portal->timer = 2 + level / 25;
    portal->value[3] = victim->in_room->vnum;

    check_undead(ch, portal);

    obj_to_room(portal, ch->in_room);

    act("$p вырастает из земли.", ch, portal, NULL, TO_ROOM);
    act("$p вырастает перед тобой.", ch, portal, NULL, TO_CHAR);

    return TRUE;
}

bool spell_nexus(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *portal, *stone;
    ROOM_INDEX_DATA *to_room, *from_room;

    if (IS_NULLSTR(target_name))
    {
	send_to_char("Интересно, на кого ты хотел прыгнуть?\n\r", ch);
	return FALSE;
    }

    stone = get_eq_char(ch, WEAR_HOLD);
    if (!IS_IMMORTAL(ch)
	&& (stone == NULL || stone->item_type != ITEM_WARP_STONE))
    {
	send_to_char("У тебя нет необходимого компонента "
		     "для этого заклинания..\n\r", ch);
	return FALSE;
    }

    from_room = ch->in_room;

    if ((victim = can_portal(ch, level)) == NULL
	|| !can_see_room(ch, (to_room = victim->in_room))
	|| !can_see_room(ch, from_room)
	|| IS_SET(victim->in_room->room_flags, ROOM_NOGATE)
	|| IS_SET(ch->in_room->room_flags, ROOM_NOGATE)
	|| IS_SET(to_room->room_flags, ROOM_KILL)
	|| IS_SET(to_room->room_flags, ROOM_NOFLY_DT)
	|| IS_SET(to_room->room_flags, ROOM_NO_RECALL)
	|| IS_AFFECTED(ch, AFF_CURSE))
    {
	send_to_char("У тебя ничего не получается.\n\r", ch);
	return TRUE;
    }


    if (stone != NULL && stone->item_type == ITEM_WARP_STONE)
    {
	act("Ты получаешь всю энергию $p1!", ch, stone, NULL, TO_CHAR);
	act("$p ярко вспыхивает и исчезает!", ch, stone, NULL, TO_CHAR);
	extract_obj(stone, TRUE, TRUE);
    }

    /* portal one */
    portal = create_object(get_obj_index(OBJ_VNUM_PORTAL), 0);
    portal->timer = 1 + level / 10;
    portal->value[3] = to_room->vnum;

    check_undead(ch, portal);

    obj_to_room(portal, from_room);

    act("$p вырастает из земли.", ch, portal, NULL, TO_ROOM);
    act("$p вырастает перед тобой.", ch, portal, NULL, TO_CHAR);

    /* no second portal if rooms are the same */
    if (to_room == from_room)
	return TRUE;

    /* portal two */
    portal = create_object(get_obj_index(OBJ_VNUM_PORTAL), 0);
    portal->timer = 1 + level/10;
    portal->value[3] = from_room->vnum;

    check_undead(ch, portal);

    obj_to_room(portal, to_room);

    if (!LIST_EMPTY(&to_room->people))
    {
	act("$p вырастает из земли.", LIST_FIRST(&to_room->people), portal, NULL, TO_ALL);
    }

    return TRUE;
}

bool spell_advance_skill(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    const int maximum = 90;
    int i, chance, selected = 0;
    char bfr[MAX_INPUT_LENGTH];

    if (IS_NPC(ch))
	return FALSE;

    for (i = 1; i < max_skills; i++)
    {
	if (skill_table[i].name == NULL)
	    break;

	if (number_range(1, 100000) < 100000/i && get_skill(ch, i) > 0)
	    selected = i;
    }

    if (selected == 0)
    {
	send_to_char("... И ничего не происходит.\n\r", ch);
	return TRUE;
    }

    i = ch->pcdata->learned[selected];
    if (i == 0)
    {
	sprintf(bfr, "Ты совсем не знаешь, что такое '%s'.\n\r",
		skill_table[selected].rname);
	send_to_char(bfr, ch);
	return TRUE;
    }

    if (i < 5)
    {
	sprintf(bfr, "Ты слишком плохо знаешь, что такое '%s'.\n\r",
		skill_table[selected].rname);
	send_to_char(bfr, ch);
	return TRUE;
    }

    if (i >= maximum)
    {
	sprintf(bfr, "Ты уже довольно неплохо знаешь, что такое '%s'.\n\r"
		"Эта книга уже ничему не сможет тебя научить.\n\r",
		strcmp(skill_table[selected].rname, "")
		? skill_table[selected].rname : skill_table[selected].name);
	send_to_char(bfr, ch);
	return TRUE;
    }
    chance =  int_app[get_curr_stat(ch, STAT_INT)].learn
	+ wis_app[get_curr_stat(ch, STAT_WIS)].practice;
    chance /= UMAX(get_skill_rating(ch, sn), 1);
    chance += ch->level/3;

    if (number_percent() <= chance)
    {
	ch->pcdata->learned[selected] = UMIN(maximum,
					     ch->pcdata->learned[selected]
					     + number_range(1, 10));
	sprintf(bfr,
		"{GТы прочитываешь книгу и повышаешь свой опыт в умении '%s'!{x\n\r",
		skill_table[selected].rname);
	send_to_char(bfr, ch);
    }
    else
	send_to_char("Ты прочитываешь книгу, но ничего не понимаешь...\n\r",
		     ch);

    return TRUE;
}

bool spell_resurrect(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj, *obj2, *next;
    CHAR_DATA *mob;
    int i;
    char buf[MAX_STRING_LENGTH];
    char *arg;
    MOB_INDEX_DATA *pMobIndex;

    if (IS_SET(ch->in_room->room_flags, ROOM_NO_MOB))
    {
	send_to_char("Только не здесь!\n\r", ch);
	return FALSE;
    }

    obj = get_obj_here(NULL, ch->in_room, target_name);

    if (obj == NULL)
    {
	send_to_char("Сделать зомби из чего?\n\r", ch);
	return FALSE;
    }

    /* Nothing but NPC corpses. */

    if (obj->item_type != ITEM_CORPSE_NPC)
    {
	if (obj->item_type == ITEM_CORPSE_PC)
	    send_to_char("Ты не можешь оживлять игроков.\n\r", ch);
	else
	    send_to_char("Гмм... А смысл?\n\r", ch);
	return FALSE;
    }

    if ((pMobIndex = get_mob_index(obj->value[4])) == NULL || IS_SET(pMobIndex->act, ACT_UNDEAD))
    {
	send_to_char("Извини, этот труп оживить уже не получится.\n\r", ch);
	return FALSE;
    }


    if (pMobIndex->resists[DAM_CHARM] >= 100)
    {
	send_to_char("Зомби этого трупа не пойдет за тобой.\n\r", ch);
	return FALSE;
    }

    if (obj->level > (ch->level + 2))
    {
	send_to_char("У тебя не хватит духа, чтобы управлять таким мощным "
		     "зомби.\n\r", ch);
	return FALSE;
    }

    if (max_count_charmed(ch))
	return FALSE;


    /* Chew on the zombie a little bit, recalculate level-dependant stats */

    mob = create_mobile(get_mob_index(MOB_VNUM_ZOMBIE));

    arg = one_argument(obj->short_descr, buf);


    sprintf(buf , mob->name, arg);
    free_string(mob->name);
    mob->name = str_dup(buf);

    sprintf(buf , mob->short_descr, arg);
    free_string(mob->short_descr);
    mob->short_descr = str_dup(buf);

    sprintf(buf , mob->long_descr, arg);
    free_string(mob->long_descr);
    mob->long_descr = str_dup(buf);


    mob->level          = obj->level;
    /*    mob->affected_by	= pMobIndex->affected_by; */
    mob->alignment	= -1000;
    mob->hitroll	= number_range(2 * pMobIndex->hitroll/3,
				       pMobIndex->hitroll);
    mob->damroll	= number_range(2 * pMobIndex->damage[DICE_BONUS]/3,
				       pMobIndex->damage[DICE_BONUS]);
    mob->max_hit	= dice(pMobIndex->hit[DICE_NUMBER],
			       pMobIndex->hit[DICE_TYPE])
	+ pMobIndex->hit[DICE_BONUS];
    mob->max_hit	= number_range(2 * mob->max_hit/3, mob->max_hit);
    mob->hit		= mob->max_hit;
    mob->max_mana       = 100 + dice(mob->level, 10);
    mob->mana           = mob->max_mana;

    mob->damage[DICE_NUMBER] = number_range(2*pMobIndex->damage[DICE_NUMBER]/3,
					    pMobIndex->damage[DICE_NUMBER]);
    mob->damage[DICE_TYPE]   = number_range(2 * pMobIndex->damage[DICE_TYPE]/3,
					    pMobIndex->damage[DICE_TYPE]);
    mob->dam_type	     = pMobIndex->dam_type;

    if (mob->dam_type == 0)
	switch (number_range(1, 3))
	{
	case (1):
	    mob->dam_type = 3;
	    break;  /* slash */
	case (2):
	    mob->dam_type = 7;
	    break;  /* pound */
	case (3):
	    mob->dam_type = 11;
	    break;  /* pierce */
	}

    for (i = 0; i < 4; i++)
    {
	mob->armor[i]	= pMobIndex->ac[i];
	if (mob->armor[i] > 0)
	    mob->armor[i] = UMIN(100, number_range(mob->armor[i],
						   3 * mob->armor[i] / 2));
	else
	    mob->armor[i] = number_range(2 * mob->armor[i] / 3, mob->armor[i]);
    }

    mob->off_flags		= pMobIndex->off_flags;
    for (i = 0; i < DAM_MAX; i++)
	if (pMobIndex->resists[i] < 50)
	    mob->resists[i] = pMobIndex->resists[i];
	else
	    mob->resists[i] = 0;

    mob->resists[DAM_POISON] = 100;
    mob->resists[DAM_DISEASE] = 100;

    mob->parts = pMobIndex->parts;

    for (i = 0; i < MAX_STATS; i++)
	mob->perm_stat[i] = UMAX(MAX_STAT, 11 + mob->level/4);

    /* You rang? */
    char_to_room(mob, ch->in_room, FALSE);
    act("$p поднимается и превращается в ужасного зомби!",
	ch, obj, NULL, TO_ROOM);
    act("$p поднимается и превращается в ужасного зомби!",
	ch, obj, NULL, TO_CHAR);

    for (obj2 = obj->contains;obj2;obj2=next)
    {
	next = obj2->next_content;

	if (SUPPRESS_OUTPUT(!is_have_limit(mob, mob, obj2)))
	{
	    obj_from_obj(obj2);
	    obj_to_char(obj2, mob);
	}
    }

    extract_obj(obj, FALSE, FALSE);

    /* Yessssss, massssssster... */
    SET_BIT(mob->affected_by, AFF_CHARM);
    SET_BIT(mob->act, ACT_UNDEAD);
    SET_BIT(mob->act, ACT_NOEXP);
    REMOVE_BIT(mob->act, ACT_DEAD_WARRIOR);
    mob->comm = COMM_NOTELL|COMM_NOSHOUT|COMM_NOCHANNELS;

    add_follower(mob, ch);
    mob->leader = ch;
    sprintf(buf, "Чем я могу тебе помочь, Повелитель%s?",
	    ch->sex == SEX_MALE ? "" : "ница");
    do_say(mob,  buf);
    return TRUE;
}

bool spell_control_undead(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (!IS_NPC(victim) || !IS_SET(victim->act, ACT_UNDEAD))
    {
	act("$N не похож$t на умершего.", ch, SEX_ENDING(victim), victim, TO_CHAR);
	return FALSE;
    }
    return charm(ch, victim, level, sn);
}


bool spell_detect_undead(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_replace(sn, level, victim))
	affect_strip(victim, sn);

    if (IS_AFFECTED(victim, AFF_DETECT_UNDEAD))
    {
	if (victim == ch)
	    send_to_char(" Ты уже можешь обнаруживать мертвых.\n\r", ch);
	else
	    act("$N уже может обнаруживать мертвых.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level	 = level;
    af.duration  = (5 + level / 3);
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_UNDEAD;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    send_to_char("Твои глаза начинают светиться красным светом!\n\r", victim);
    if (ch != victim)
	send_to_char("Ok.\n\r", ch);
    return TRUE;
}

OBJ_DATA *get_hold_meat(CHAR_DATA *ch)
{
    OBJ_DATA *obj;

    if ((obj = get_eq_char(ch, WEAR_HOLD)) == NULL)
    {
	send_to_char("Ты ничего не держишь в руке.\n\r", ch);
	return NULL;
    }

    if (obj->pIndexData->vnum != OBJ_VNUM_SEVERED_HEAD
	&& obj->pIndexData->vnum != OBJ_VNUM_TORN_HEART
	&& obj->pIndexData->vnum != OBJ_VNUM_SLICED_ARM
	&& obj->pIndexData->vnum != OBJ_VNUM_SLICED_LEG
	&& obj->pIndexData->vnum != OBJ_VNUM_GUTS
	&& obj->pIndexData->vnum != OBJ_VNUM_BRAINS
	&& obj->pIndexData->vnum != OBJ_VNUM_MEAT
	&& obj->pIndexData->vnum != OBJ_VNUM_TAIL
	&& obj->pIndexData->vnum != OBJ_VNUM_EAR)
    {
	send_to_char("Тебе надо держать в руке кусок мяса!\n\r", ch);
	return NULL;
    }

    return obj;
}
bool spell_power_kill(int sn, int level, CHAR_DATA *ch, void *vo , int target)
{
    OBJ_DATA *obj;
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam, chance;
    bool svs;

    if (IS_VAMPIRE(victim) || is_lycanthrope(victim))
    {
	send_to_char("Твое заклинание не произвело никакого эффекта.\n\r", ch);
	return FALSE;
    }

    if (is_affected(ch, sn))
    {
      	printf_to_char("Ты еще не готов%s применить такое мощное заклинание.\n\r", ch, SEX_ENDING(ch));
       	return FALSE;
    } 

    if ((obj = get_hold_meat(ch)) == NULL)
        return FALSE;

    extract_obj(obj, TRUE, FALSE);

    act("{RТы направляешь свой палец на $N3, и $S окружает {DТЬМА{x!",
	ch, NULL, victim, TO_CHAR);
    act("{RПоток {DТЬМЫ{R окружает $N3!",
	ch, NULL, victim, TO_NOTVICT);
    act("{RПоток {DТЬМЫ{R, вырывающийся из пальца $N1, окружает тебя!{x",
	victim, NULL, ch, TO_CHAR);

    chance  = get_skill(ch, gsn_power_kill);
    chance += level - victim->level;
    chance += 2 * (get_curr_stat(ch, STAT_WIS)
		   - get_curr_stat(victim, STAT_WIS));
    chance += 2 * (get_curr_stat(ch, STAT_INT)
		   - get_curr_stat(victim, STAT_INT));

    if (level + PK_RANGE < victim->level)
	chance = 0;

    svs = saves_spell(level, victim, DAM_MENTAL);

    if (!IS_NPC(victim))
    {
        AFFECT_DATA af; 
        
        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = ch->level;
        af.duration = 5;
        af.modifier = 0;
        af.location = APPLY_NONE;
        af.bitvector = 0;
        af.caster_id = get_id(ch);
        affect_to_char(ch, &af); 
    }

    //повысил шанс прохождения, пределы рандома от 1 до 500 вместо от 1 до 1250
    //ибо прохождение было нулевым
    if (number_range(1, 500) > chance || svs)
    {
	dam = dice(level, 12);

	if (svs)
	    dam /= 2;

	damage(ch, victim, dam, sn, DAM_MENTAL, TRUE, NULL);
	return TRUE;
    }

    check_killer(ch, victim);
    for_killed_skills(ch, victim);

    return TRUE;
}

bool spell_beacon (int sn, int level, CHAR_DATA *ch, void *vo , int target)
{
    OBJ_DATA *obj;

    if (IS_NPC(ch))
	return FALSE;

    if (ch->in_room == NULL
	|| IS_SET(ch->in_room->room_flags, ROOM_SAFE)
	|| IS_SET(ch->in_room->room_flags, ROOM_PRIVATE)
	|| IS_SET(ch->in_room->room_flags, ROOM_SOLITARY)
	|| IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
	|| IS_SET(ch->in_room->room_flags, ROOM_NOGATE))
    {
	send_to_char("Здесь маяк работать не сможет.\n\r", ch);
	return FALSE;
    }

    if (ch->in_room != NULL && ch->in_room->vnum >= ROOM_VNUM_HOUSE && ch->in_room->vnum < ROOM_VNUM_HOUSE + 1000)
    {
	send_to_char("Маяк в доме? Хе-хе...\n\r", ch);
	return FALSE;
    }


    if (ch->pcdata->beacon != NULL)
	extract_obj(ch->pcdata->beacon, FALSE, FALSE);

    obj = create_object(get_obj_index(OBJ_VNUM_BEACON), 0);
    obj->timer = number_range(level * 2 + get_skill(ch, sn),
			      6 * (level + get_skill(ch, sn)));
    obj_to_room(obj, ch->in_room);
    ch->pcdata->beacon = obj;

    send_to_char("Ты создаешь маяк!\n\r", ch);
    act("$n создает маяк!", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool spell_find_beacon (int sn, int level, CHAR_DATA *ch, void *vo , int target)
{
    CHAR_DATA *wch, *wch_next;
    ROOM_INDEX_DATA *in_room, *to_room;

    if (IS_NPC(ch)
	|| ch->pcdata->beacon == NULL
	|| (to_room = ch->pcdata->beacon->in_room) == NULL)
    {
	send_to_char("Сначала установи где-нибудь маяк!\n\r", ch);
	return FALSE;
    }

    if ((in_room = ch->in_room) == NULL
	|| IS_SET(in_room->room_flags, ROOM_SAFE)
	|| IS_SET(in_room->room_flags, ROOM_PRIVATE)
	|| IS_SET(in_room->room_flags, ROOM_SOLITARY)
	|| IS_SET(in_room->room_flags, ROOM_NO_RECALL)
	|| IS_SET(in_room->room_flags, ROOM_NOGATE)
	|| IS_AFFECTED(ch, AFF_CURSE))
    {
	send_to_char("У тебя не получается обнаружить свой маяк...\n\r", ch);
	return FALSE;
    }

    LIST_FOREACH_SAFE(wch, &in_room->people, room_link, wch_next)
    {
	if (ch != wch && is_same_group(wch, ch))
	{
		if (IS_NPC(wch) 
		   && IS_SET(to_room->room_flags, ROOM_NO_MOB) 
		   && wch->pIndexData->vnum != MOB_BONE_SHIELD)
			continue;
		move_for_recall(wch, to_room);
	}
    }

    move_for_recall(ch, to_room);

    return TRUE;
}

int get_beacon_time(CHAR_DATA *ch, OBJ_DATA *obj, int sn)
{
    int sk = number_range(0, (100 - get_skill(ch, sn)));

    if (obj != ch->pcdata->beacon)
	sk *= 2;

    if (number_percent() < 50)
	sk = 0 - sk;

    sk = UMAX(0, obj->timer + sk);

    return sk;
}

bool spell_beacon_identify(int sn, int level, CHAR_DATA *ch, void *vo , int target)
{
    OBJ_DATA *obj;
    CHAR_DATA *wch, *safe_wch;
    int hrs;
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
	return FALSE;

    if (target_name[0] == '\0')
    {
	if ((obj = ch->pcdata->beacon) == NULL)
	{
	    send_to_char("У тебя отсутствует маяк.\n\r", ch);
	    return FALSE;
	}
	hrs = get_beacon_time(ch, obj, sn);
	sprintf(buf, "Твой маяк будет держаться еще около %d %s.\n\r",
		hrs, hours(hrs, TYPE_HOURS));
	send_to_char(buf, ch);
	return TRUE;
    }
    else
	obj = get_obj_here(ch, NULL, target_name);


    if (obj == NULL)
    {
	send_to_char("Ты не видишь этого здесь.\n\r", ch);
	return FALSE;
    }

    if (obj->pIndexData->vnum != OBJ_VNUM_BEACON)
    {
	send_to_char("Это не похоже на маяк.\n\r", ch);
	return FALSE;
    }

    LIST_FOREACH_SAFE(wch, &char_list, link, safe_wch)
    {
	if (!IS_NPC(wch) && wch->pcdata->beacon == obj)
	{
	    hrs = get_beacon_time(wch, obj, sn);
	    if (wch == ch)
		sprintf(buf, "Это твой маяк. Он будет держаться "
			"еще около %d %s.\n\r", hrs, hours(hrs, TYPE_HOURS));
	    else
		sprintf(buf, "Это маяк %s. Он будет держаться "
			"еще около %d %s.\n\r", PERS(wch, ch, 1),
			hrs, hours(hrs, TYPE_HOURS));

	    send_to_char(buf, ch);
	    return TRUE;
	}
    }

    send_to_char("Не удалось опознать маяк.\n\r", ch);
    return TRUE;
}

bool spell_power_word_fear(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (victim == ch)
    {
	send_to_char("Ты и так очень боишься себя.\n\r", ch);
	return FALSE;
    }

    if (IS_VAMPIRE(victim) || is_lycanthrope(victim))
    {
	send_to_char("Твое заклинание не производит никакого эффекта!\n\r", ch);
	return FALSE;
    }

    if (is_affected(victim, sn))
    {
	send_to_char("Это существо уже и так ужасно боится тебя!\n\r", ch);
	return FALSE;
    }

    if (saves_spell(level, victim, DAM_MENTAL) 
	|| number_percent() < (level / (4 * (UMAX(victim->level, 1)))) * 100)
    {
	send_to_char("Неудачно!\n\r", ch);
	return TRUE;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level/10;
    af.location = APPLY_HITROLL;
    af.modifier = -level/7;
    af.bitvector = 0;
    af.caster_id = get_id(ch);

    affect_to_char(victim, &af);

    af.location = APPLY_DAMROLL;
    af.modifier = -level/7;

    affect_to_char(victim, &af);

    af.location = APPLY_STR;
    af.modifier = -level/15;

    affect_to_char(victim, &af);

    af.location = APPLY_SAVES;
    af.modifier = level/10;

    affect_to_char(victim, &af);

    af.where = TO_RESIST;
    af.location = APPLY_NONE;
    af.modifier = -15;
    af.bitvector = DAM_MENTAL;
    affect_to_char(victim, &af);

    DAZE_STATE(victim, 3 * skill_table[sn].beats);

    act("Глаза $N1 в ужасе выкатываются из орбит!", ch, NULL, victim, TO_CHAR);
    act("Ты с ужасом смотришь на $n3!!", ch, NULL, victim, TO_VICT);
    act("Глаза $N1 в ужасе выкатываются из орбит!", ch, NULL, victim, TO_ROOM);

    return TRUE;
}

bool spell_aura_of_fear(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    OBJ_DATA *obj, *floating;

    floating = get_eq_char(ch, WEAR_FLOAT);
    if (floating != NULL && IS_OBJ_STAT(floating, ITEM_NOREMOVE))
    {
	act("Ты не можешь снять $p6!!", ch, floating, NULL, TO_CHAR);
	return FALSE;
    }

    obj = create_object(get_obj_index(OBJ_VNUM_NAZGUL_AURA), 0);

    if (obj == NULL)
    {
	bugf("spell_aura_of_fear: OBJ_VNUM_NAZGUL_AURA is NULL");
	return FALSE;
    }
    obj->level = level;
    obj->timer = level/4;

    SET_BIT(obj->value[4], WEAPON_SHARP);

    obj_to_char(obj, ch);

    af.where = TO_OBJECT;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.location = APPLY_SAVES;
    af.modifier = - level/5;
    af.bitvector = 0;

    affect_to_obj(obj, &af);

    af.location = APPLY_HITROLL;
    af.modifier = level/5;

    affect_to_obj(obj, &af);

    af.location = APPLY_DAMROLL;
    af.modifier = level/5;

    obj->enchanted = TRUE;

    affect_to_obj(obj, &af);

    act("$n создает $p6!", ch, obj, NULL, TO_ROOM);
    act("Ты создаешь $p6!\n\r", ch, obj, NULL, TO_CHAR);

    wear_obj(ch, obj, TRUE);

    return TRUE;
}


bool spell_call_horse(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *horse;
    int i;

    if (!IS_OUTSIDE(ch) || !IS_IN_WILD(ch) || weather_info.sunlight != SUN_DARK
	|| IS_SET(ch->in_room->room_flags, ROOM_NO_MOB))
    {
	send_to_char("Ты можешь позвать скакуна только на дикой природе "
		     "и только в темное время суток.\n\r", ch);
	return FALSE;
    }

    if (ch->mount != NULL)
    {
	send_to_char("У тебя уже есть скакун.\n\r", ch);
	return FALSE;
    }

    if (IS_SET(ch->act, PLR_NOFOLLOW))
    {
	send_to_char("Ты не принимаешь последователей.\n\r", ch);
	return FALSE;
    }

    if (max_count_charmed(ch))
        return FALSE;

    horse = create_mobile(get_mob_index(MOB_VNUM_NAZGUL_HORSE));

    if (horse == NULL)
    {
	bugf("spell_call_horse: MOB_VNUM_NAZGUL_HORSE is NULL");
	return FALSE;
    }

    char_to_room(horse, ch->in_room, FALSE);

    horse->hit = horse->max_hit = ch->max_hit * 5 / 2;
    horse->move = horse->max_move = ch->max_move * 2;
    horse->hitroll = level;
    horse->damroll = level;
    horse->level = ch->level;
    horse->damage[0] = level/10; 
    horse->damage[1] = level/10;
    horse->damage[2] = level/5;
    
    for (i = 0; i < MAX_STATS; i ++)
        horse->perm_stat[i] = UMIN(MAX_STAT, 11 + horse->level/4);

    add_follower(horse, ch);
    horse->leader = ch;
    SET_BIT(horse->act, ACT_NOEXP);

    send_to_char("Ты издаешь жуткий крик, и на призыв прилетает "
		 "твой скакун.\n\r", ch);
    act("$n издает жуткий крик, и на призыв прилетает $N!",
	ch, NULL, horse, TO_ROOM);

    act("Ты выкрикиваешь древнее заклинание, и $N становится "
	"неуязвимым к свету солнца.", ch, NULL, horse, TO_CHAR);
    act("$n выкрикивает древнее заклинание, и $N становится "
	"неуязвимым к свету солнца.", ch, NULL, horse, TO_ROOM);

    ch->mount = horse;

    if (can_see(ch,horse))
	do_function(ch, &do_mount, horse->name);

    return TRUE;
}

bool spell_fire_shield(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_replace(sn, level, victim))
	affect_strip(victim, sn);

    if (IS_AFFECTED(victim, AFF_HOLY_SHIELD)
	|| IS_AFFECTED(victim, AFF_FIRE_SHIELD)
	|| IS_AFFECTED(victim, AFF_ICE_SHIELD)
	|| IS_AFFECTED(victim, AFF_CONIFEROUS_SHIELD))

    {
	if (victim == ch)
	    send_to_char("Ты и так под защитой элементального щита.\n\r", ch);
	else
	    act("$N и так под защитой элементального щита.",
		ch, NULL, victim, TO_CHAR);

	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 5;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_FIRE_SHIELD;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    af.where = TO_RESIST;
    af.location = APPLY_NONE;
    af.modifier = 10;
    af.bitvector = DAM_FIRE;
    affect_to_char(victim, &af);

    act("$n3 окружают едва заметные сполохи огня.",
	victim, NULL, NULL, TO_ROOM);
    send_to_char("Тебя окружают едва заметные сполохи огня.\n\r", victim);
    return TRUE;
}

bool spell_ice_shield(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_replace(sn, level, victim))
	affect_strip(victim, sn);

    if (IS_AFFECTED(victim, AFF_HOLY_SHIELD)
	|| IS_AFFECTED(victim, AFF_FIRE_SHIELD)
	|| IS_AFFECTED(victim, AFF_ICE_SHIELD)
	|| IS_AFFECTED(victim, AFF_CONIFEROUS_SHIELD))
    {
	if (victim == ch)
	    send_to_char("Ты и так под защитой элементального щита.\n\r", ch);
	else
	    act("$N и так под защитой элементального щита.",
		ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 5;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_ICE_SHIELD;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    af.where = TO_RESIST;
    af.location = APPLY_NONE;
    af.modifier = 10;
    af.bitvector = DAM_COLD;
    affect_to_char(victim, &af);

    act("Воздух вокруг $n1 приобретает синеватый оттенок.",
	victim, NULL, NULL, TO_ROOM);
    send_to_char("Воздух вокруг тебя приобретает синеватый оттенок.\n\r",
		 victim);
    return TRUE;
}

bool spell_coniferous_shield(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_replace(sn, level, victim))
	affect_strip(victim, sn);

    if (IS_AFFECTED(victim, AFF_HOLY_SHIELD)
	|| IS_AFFECTED(victim, AFF_FIRE_SHIELD)
	|| IS_AFFECTED(victim, AFF_ICE_SHIELD)
	|| IS_AFFECTED(victim, AFF_CONIFEROUS_SHIELD))
    {
	if (victim == ch)
	    send_to_char("Ты и так под защитой щита.\n\r", ch);
	else
	    act("$N и так под защитой щита.",
		ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 5;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_CONIFEROUS_SHIELD;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    af.where = TO_RESIST;
    af.location = APPLY_NONE;
    af.modifier = 10;
    af.bitvector = DAM_PIERCE;
    affect_to_char(victim, &af);

    act("Воздух вокруг $n1 приобретает зеленоватый оттенок, $n окружается шипами.",
	victim, NULL, NULL, TO_ROOM);
    send_to_char("Воздух вокруг тебя приобретает зеленоватый оттенок, ты окружаешься шипами.\n\r",
		 victim);
    return TRUE;
}


bool spell_puffball(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;
    int dam;

    dam = dice(2, level/2);

    if (!IS_IN_WILD(ch))
    {
	send_to_char("Тебе надо быть на лоне дикой природы "
		     "для этого заклинания!\n\r", ch);
	return FALSE;
    }

    damage(ch, victim, dam, sn, DAM_DISEASE, TRUE, NULL);

    if (is_affected(victim, sn))
    {
	act("$N уже заражен$t чумой.", ch, SEX_ENDING(victim), victim, TO_CHAR);
	return TRUE;
    }

    if (!saves_spell(level, victim, DAM_DISEASE))
    {
	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = level;
	af.duration = level/5;
	af.location = APPLY_STR;
	af.modifier = -5;
	af.bitvector = AFF_PLAGUE;
	af.caster_id = get_id(ch);
	affect_join(victim, &af);

	send_to_char("Ты корчишься в агонии, чумные болячки нарывают "
		     "на твоей коже!\n\r", victim);
	act("$n корчится в агонии, чумные болячки нарывают на $s коже!",
	    victim, NULL, NULL, TO_ROOM);
    }

    return TRUE;
}

bool spell_detect_animal(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;

    if (!is_replace(sn, level, ch))
    {
	send_to_char("Твое внимание уже на максимуме.\n\r", ch);
	return FALSE;
    }

    affect_strip(ch, sn);

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = 0;
    af.caster_id = 0;
    affect_to_char(ch, &af);

    send_to_char("Ты начинаешь отличать животный мир.\n\r", ch);
    return TRUE;
}


/*проверка на то, является ли victim егерем или очарой друида.
 работает всегда, тестировал достаточно долго. С трудом реализовал:-)*/
bool check_druid_spell(CHAR_DATA *ch, CHAR_DATA *victim)
{
    return ((victim != ch) &&
	    ((!IS_NPC(victim) && victim->classid != CLASS_RANGER && victim->classid != CLASS_DRUID) ||
	     (IS_NPC(victim)&&((victim->master == NULL)||(victim->master != ch)))));
}

bool spell_bark_skin(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    CHAR_DATA *victim;
    char *skin, *end, buf[MSL];

    /* character target */
    victim = (CHAR_DATA *) vo;


    if (check_druid_spell(ch, victim))
    {
	send_to_char("Это не лесной житель.\n\r", ch);
	return FALSE;
    }

    if (IS_SET(victim->form, FORM_TREE))
    {
        skin = "Ствол";
        end = "";
    }
    else
    {
        end = "а";
        skin = IS_SET(victim->parts, PART_SKIN) ? "Кожа" : "Внешняя оболочка";
    }

    if (!is_replace(sn, level, victim))
    {
	if (victim == ch)
	    send_to_char("Твоя кожа уже покрыта прочной корой.\n\r", ch);
	else
	{   sprintf(buf, "%s $N1 уже покрыт%s прочной корой.", skin, end);
	    act(buf, ch, NULL, victim, TO_CHAR);
	}
	return FALSE;
    }

    if (!IS_IN_FOREST(ch))
    {
	send_to_char("Но... тут же не лес!\n\r", ch);
	return FALSE;
    }

    affect_strip(victim, sn);

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.modifier = (self_magic(ch,victim) ? -30 : -20) - level/5;
    af.location = APPLY_AC;
    af.bitvector = 0;
    af.caster_id = 0;
    affect_to_char(victim, &af);

    send_to_char("Твоя кожа покрывается прочной корой!\n\r", victim);

    if (ch != victim)
	act("$t $N1 покрывается прочной корой.{x", ch, skin, victim, TO_CHAR);

    act("$t $N1 покрывается прочной корой.", ch, skin, victim, TO_NOTVICT);
    return TRUE;
}

bool spell_pathfinding(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;

    if (!is_replace(sn, level, ch))
    {
	send_to_char("Ты и так выбираешь самый легкий путь.\n\r", ch);
	return FALSE;
    }

    affect_strip(ch, sn);

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = 0;
    af.caster_id = 0;
    affect_to_char(ch, &af);

    send_to_char("Ты начинаешь внимательно выбирать свой путь...\n\r", ch);
    return TRUE;
}

bool spell_detect_camouflage(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;

    if (is_replace(sn, level, ch))
	affect_strip(ch, sn);

    if (IS_AFFECTED(ch, AFF_DETECT_CAMOUFLAGE))
    {
	send_to_char("Твое внимание уже на максимуме.\n\r", ch);
	return FALSE;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = AFF_DETECT_CAMOUFLAGE;
    af.caster_id = 0;
    affect_to_char(ch, &af);

    send_to_char("Твое внимание ко всему окружающему ОЧЕНЬ повысилось.\n\r", ch);
    return TRUE;
}

/*
 * Успокоить животное.
 * Заклинание не должно помогать в убийстве животных, ибо друиду положено по
 * классу о них заботится. Скорее, наоборот - заклинание успокаивает животное
 * и придает ему некоторые душевные силы...  8))
 * Имеет смысл вводить это заклинание, если хочется поддерживать РП в мире.
 * Представь кратинку: идет друид по полю и видит, как какой-то <пиииип> бедного
 * медвежонка убить пытается. Ну друид успокаивает медвежонка (заодно
 * зверененавистник тоже перестает драться - побочный эффект заклинания), утешает
 * его и идет себе дальше... После чего "убийца медвежат" опять бросается в бой и...
 * получает от сильно взбодрившегося медведя по самые помидоры... А если уж друид
 * будет совсем правильный, то медвежонка еще и подлечит...  8))
 */

bool spell_animal_taming(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    CHAR_DATA *victim = (CHAR_DATA *)vo;

    if (!is_animal(victim) && !is_lycanthrope(victim))
    {
	send_to_char("Хм... Это не животное...\n\r", ch);
	return FALSE;
    }

    if (victim->fighting == NULL)
    {
	act("А разве $N с кем-то сражается?", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    if (is_animal(get_master(victim->fighting)))
    {
	send_to_char("Оставь этих животных в покое, "
		     "они сами разберутся между собой.\n\r", ch);
	return FALSE;
    }

    if (is_lycanthrope(victim) && saves_spell(level * 1.7, victim, DAM_OTHER))
    {
	send_to_char("У тебя не получается остановить драку. Это животное сопротивляется.\n\r", ch);
	act("$n пытается остановить драку, но у него ничего не получается...", ch, NULL, NULL, TO_ROOM);
	return FALSE;
    }

    stop_fighting(victim, TRUE);

    if (!is_affected(victim, sn))
    {
	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = level;
	af.duration = (IS_NPC(victim) ? level/4 : 1);
	af.location = APPLY_HITROLL;
	af.modifier = -level/4;
	af.bitvector = AFF_CALM;
	af.caster_id = get_id(ch);
	affect_to_char(victim, &af);

	af.location = APPLY_DAMROLL;
	af.modifier = -level/4;
	affect_to_char(victim, &af);

	af.location = APPLY_SAVING_SPELL;
	af.modifier = -level/10;
	affect_to_char(victim, &af);

	WAIT_STATE(victim, 3 * skill_table[sn].beats);
    }
   
    send_to_char("Ты останавливаешь драку, защищая животное.\n\r", ch);
    act("$n останавливает избиение животного...", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool spell_charm_animal(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;

    if (!is_animal(victim))
    {
	send_to_char("Хм... Это не животное...\n\r", ch);
	return FALSE;
    }

    return charm(ch, victim, level, sn);
}

bool spell_needlestorm(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    CHAR_DATA *vch, *vch_next;
    int dam;

    if (!IS_IN_FOREST(ch))
    {
	send_to_char("Ты можешь использовать это заклинание только в лесу!\n\r",
		     ch);
	return FALSE;
    }

    act("{W$n призывает тучу игл с соседних деревьев!{x",
	ch, NULL, victim, TO_NOTVICT);
    act("{W$n призывает тучу игл с соседних деревьев на тебя!{x",
	ch, NULL, victim, TO_VICT);
    act("{WТы призываешь тучу игл с соседних деревьев!{x",
	ch, NULL, victim, TO_CHAR);

    dam = dice(level*2, 5);

    LIST_FOREACH_SAFE(vch, &victim->in_room->people, room_link, vch_next)
    {
	if (SUPPRESS_OUTPUT(is_safe_spell(ch, vch, TRUE))
	    || (IS_NPC(vch)
		&& IS_NPC(ch)
		&& (ch->fighting != vch || vch->fighting != ch))
	    || is_same_group(ch, vch))
	{
	    continue;
	}

	if (vch == victim)      /* full damage */
	    damage(ch, vch, dam, sn, DAM_PIERCE, TRUE, NULL);
	else                    /* partial damage */
	    damage(ch, vch, dam/3, sn, DAM_PIERCE, TRUE, NULL);

    }
    return TRUE;
}

bool spell_hunger(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;
    int chance;

    if (IS_VAMPIRE(victim))
    {
	send_to_char("Твое заклинание не производит никакого эффекта!\n\r", ch);
	return FALSE;
    }

    if (IS_NPC(victim))
    {
	if (saves_spell(level, victim, DAM_HARM))
	{
	    send_to_char("Не получается!\n\r", ch);
	    return TRUE;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = level;
	af.duration = level/20;
	af.modifier = -UMAX(1, level/15);
	af.location = APPLY_STR;
	af.bitvector = 0;
	af.caster_id = get_id(ch);
	affect_to_char(victim, &af);
    }
    else
    {
	chance = (ch->level - victim->level) * 10;
	switch(SECTOR(ch))
	{
	case SECT_FOREST:
	case SECT_FIELD:
	case SECT_HILLS:
	    chance += 25;
	    break;
	case SECT_MOUNTAIN:
	    chance += 50;
	    break;
	case SECT_DESERT:
	    chance += 75;
	    break;
	case SECT_INSIDE:
	    chance -= 50;
	    break;
	case SECT_CITY:
	    chance -= 40;
	    break;
	case SECT_WATER_SWIM:
	case SECT_WATER_NOSWIM:
	    chance -= 75;
	    break;
	case SECT_AIR:
	    chance += 60;
	    break;
	default:
	    break;
	}

	if (chance < number_percent())
	{
	    send_to_char("Не выходит.\n\r", ch);
	    return TRUE;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = level;
	af.duration = level/4;
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.bitvector = 0;
	af.caster_id = get_id(ch);
	affect_to_char(victim, &af);

	victim->pcdata->condition[COND_HUNGER] = 0;
	victim->pcdata->condition[COND_THIRST] = 0;
    }

    send_to_char("Тебя начинают мучать страшный голод и жажда!\n\r", victim);
    act("$N3 начинают мучать страшный голод и жажда!",
	ch, NULL, victim, TO_CHAR);
    return TRUE;	
}

bool spell_create_scimitar(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    OBJ_DATA *sword;
    OBJ_INDEX_DATA *pIndex;
    int skill;
    int scimitar_level = 0;

    if (IS_NPC(ch))
    {
	send_to_char("У тебя не хватает сноровки, чтобы выращивать скимитары.\n\r", ch);
	return FALSE;
    }

    if (!IS_IN_WILD(ch))
    {
	send_to_char("Ты не можешь тут найти ни клочка земли...\n\r", ch);
	return FALSE;
    }

    if ((pIndex = get_obj_index(OBJ_VNUM_DRUIDS_SWORD)) == NULL)
    {
	bugf("Object with vnum = %d is absent", OBJ_VNUM_DRUIDS_SWORD);
	return FALSE;
    }

    if (is_affected(ch, sn))
    {
	send_to_char("Ты не можешь пока еще достаточно сконцентрироваться.\n\r", ch);
	return FALSE;
    }

    if (!is_number(target_name))
	scimitar_level = level;
    else
    {
	scimitar_level = atoi(target_name);

	if ( scimitar_level < 10 || scimitar_level > level)
	{
	    send_to_char("Уровень должен быть в разумных пределах.\n\r", ch);
	    return FALSE;
	}
    }

    level = scimitar_level;

    sword = create_object(pIndex, level);

    sword->level = level;

    if (level >= 45)
    {
	sword->value[1] = 9;
	sword->value[2] = 4;
    }
    else if (level >= 40)
    {
	sword->value[1] = 5;
	sword->value[2] = 7;
    }
    else if (level >= 35)
    {
	sword->value[1] = 3;
	sword->value[2] = 12;
    }
    else if (level >= 30)
    {
	sword->value[1] = 4;
	sword->value[2] = 8;
    }
    else if (level >= 25)
    {
	sword->value[1] = 4;
	sword->value[2] = 7;
    }
    else
    {
	sword->value[1] = level / 6;
	sword->value[2] = 6;
    }

    af.where = TO_OBJECT;
    af.type = sn;
    af.level = level;
    af.duration = -1;
    af.location = APPLY_HITROLL;
    af.modifier = number_range(level/15, level/10);
    af.bitvector = 0;
    affect_to_obj(sword, &af);

    af.location = APPLY_DAMROLL;
    affect_to_obj(sword, &af);

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 8;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = 0;
    af.caster_id = 0;
    affect_to_char(ch, &af);

    obj_to_char(sword, ch);
    sword->enchanted = TRUE;

    skill = get_skill(ch, sn);


    if (skill/5 > number_percent())
	SET_BIT(sword->value[4], WEAPON_FLAMING);
    else if (skill/5 > number_percent())
	SET_BIT(sword->value[4], WEAPON_FROST);

    if (skill/5 > number_percent())
	SET_BIT(sword->value[4], WEAPON_VAMPIRIC);
    else if (skill/5 > number_percent())
	SET_BIT(sword->value[4], WEAPON_MANAPIRIC);

    if (skill/6 > number_percent())
	SET_BIT(sword->value[4], WEAPON_SLAY_EVIL);
    else if (skill/6 > number_percent())
	SET_BIT(sword->value[4], WEAPON_SLAY_GOOD);

    if (skill/5 > number_percent())
	SET_BIT(sword->value[4], WEAPON_SHOCKING);
    else if (skill/5 > number_percent())
	SET_BIT(sword->value[4], WEAPON_POISON);

    if (skill/4 > number_percent())
	SET_BIT(sword->value[4], WEAPON_SHARP);

    send_to_char("Волшебный скимитар вырастает из-под земли!\n\r", ch);
    act("Волшебный скимитар вырастает из-под земли!", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}

/*
 * Жуткое заклинание, хоть некроманту давай.  8))
 * Из-под (!) кожи жертвы начинают лезть иглы,
 * нанося <level/2>d3
 * Сильное заклинание, и лечится только диспелом или отменой.
 */

bool spell_thornwrack(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    CHAR_DATA *victim = (CHAR_DATA *)vo;

    if (!IS_IN_WILD(ch))
    {
	send_to_char("Только не тут!\n\r", ch);
	return FALSE;
    }

    if (is_affected(victim, sn))
    {
	act("$N и так выглядит как подушечка для иголок.",
	    ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    if (saves_spell(level, victim, DAM_DISEASE))
    {
	act("Твое страшное заклинание не произвело на $N3 никакого эффекта.",
	    ch, NULL, victim, TO_CHAR);
	return TRUE;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level/10;
    af.location = APPLY_DEX;
    af.modifier = -level/8;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    act("{WПо твоему желанию из-под кожи $N1 лезут острые иглы.{x",
	ch, NULL, victim, TO_CHAR);
    act("{W$n указывает на тебя пальцем, и ты неожиданно чувствуешь "
	"острую боль по всему телу.{x", ch, NULL, victim, TO_VICT);
    act("{W$n указывает пальцем на $N3, и $N начинает корчиться от боли.{x",
	ch, NULL, victim, TO_NOTVICT);

    return TRUE;	
}

bool spell_swamp(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    CHAR_DATA *gch, *safe_gch;

    if (!IS_IN_WILD(ch))
    {
	send_to_char("Нет, ну только не тут!\n\r", ch);
	return FALSE;
    }

    if (ch->fighting == NULL)
    {
	send_to_char("Это заклинание применить ты можешь только в бою!\n\r",
		     ch);
	return FALSE;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level/10;
    af.location = APPLY_DEX;
    af.modifier = -level/10;
    af.bitvector = AFF_SLOW;
    af.caster_id = get_id(ch);

    act("{WВокруг разверзлось топкое болото!{x", ch, NULL, NULL, TO_ALL);

    LIST_FOREACH_SAFE(gch, &ch->in_room->people, room_link, safe_gch)
    {
	/* не действует на друида и членов его группы */
	if (is_same_group(ch, gch) || is_safe_spell(ch, gch, TRUE))
	    continue;

	/* Ибо не фиг */
	if (gch->fighting == NULL && gch->position > POS_SLEEPING)
	    set_fighting(gch, ch);

	if (is_affected(gch, sn)
	    || is_affected(gch, skill_lookup("slow"))
	    || IS_AFFECTED(gch, AFF_SLOW))
	{
	    continue;
	}

	if (saves_spell(level, gch, DAM_OTHER))
	    continue;

	if (IS_AFFECTED(gch, AFF_HASTE))
	    check_dispel(level, gch, skill_lookup("haste"));

	act("$n увязает в топком болоте!", gch, NULL, NULL, TO_ROOM);
	send_to_char("Ты увязаешь в топком болоте...\n\r", gch);
	affect_to_char(gch, &af);
    }

    return TRUE;
}

bool spell_forestshield(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    AFFECT_DATA af;

    /* character target */
    victim = (CHAR_DATA *) vo;

    if (check_druid_spell(ch, victim))
    {
	send_to_char("Это не лесной житель.\n\r", ch);
	return FALSE;
    }

    if (!is_replace(sn, level, victim))
    {
	if (victim == ch)
	    send_to_char("Ты хочешь еще больше защиты?!.\n\r", ch);
	else
	    act("$N уже под защитой леса.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    if (!IS_IN_FOREST(ch))
    {
	send_to_char("Но... тут же не лес!\n\r", ch);
	return FALSE;
    }

    affect_strip(victim, sn);

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level/2;
    af.modifier = -level;
    af.location = APPLY_AC;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    af.modifier = -level/10;
    af.location = APPLY_SAVING_SPELL;
    affect_to_char(victim, &af);

    send_to_char("Тебя окружает сияющая зеленая аура.\n\r", victim);

    if (ch != victim)
	act("$N окружается сияющей зеленой аурой.",	ch, NULL, victim, TO_CHAR);
    act("$N окружается сияющей зеленой аурой.", ch, NULL, victim, TO_NOTVICT);


    return TRUE;
}

bool spell_camouflage(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    AFFECT_DATA af;

    /* character target */
    victim = (CHAR_DATA *) vo;

    if (check_druid_spell(ch, victim))
    {
	send_to_char("Это не лесной житель.\n\r", ch);
	return FALSE;
    }

    if (!is_replace(sn, level, victim))
    {
	if (victim == ch)
	    send_to_char("Ты уже в маскировке.\n\r", ch);
	else
	    act("$N уже в маскировке.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    if (IS_AFFECTED(ch, AFF_FAERIE_FIRE) )
    {
	if (victim == ch)
	    send_to_char("Ты не можешь скрыться, пока от тебя исходит свечение.\n\r", ch);
	else
	    act("$N не может скрыться, пока от него исходит свечение.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    if (!IS_IN_WILD(ch))
    {
	send_to_char("Только не тут!\n\r", ch);
	return FALSE;
    }

    send_to_char("Ты сливаешься с окружающей местностью.\n\r", victim);

    if (ch != victim)
	act("$N сливается с окружающей местностью.", ch, NULL, victim, TO_CHAR);
    act("$N сливается с окружающей местностью.", ch, NULL, victim, TO_NOTVICT);

    affect_strip(victim, gsn_camouflage);

    af.where = TO_AFFECTS;
    af.type = gsn_camouflage;
    af.level = ch->level;
    af.duration = ch->level;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_CAMOUFLAGE;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    return TRUE;
}


bool spell_earthmaw(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    int chance, dam;
    AFFECT_DATA af;

    if (IS_NPC(ch))
    {
	send_to_char("У тебя не хватает сноровки, чтобы засыпать землей.\n\r", ch);
	return FALSE;
    }

    if (!IS_IN_WILD(ch))
    {
	send_to_char("Только не тут!\n\r", ch);
	return FALSE;
    }

    if (ch->fighting != NULL)
    {
	send_to_char("Куда?! Ты еще дерешься!\n\r", ch);
	return FALSE;
    }

    if (is_affected(ch, gsn_earthmaw))
    {
      	printf_to_char("Ты еще не готов%s применить такое мощное заклинание.\n\r", ch, SEX_ENDING(ch));
       	return FALSE;
    }


    chance = 4;		   /* Базовая вероятность прохождения - 4% */
    dam = dice(level, 12); /* Базовые повреждения - как у кислотного плевка */

    chance += (ch->level - victim->level)/3;
    chance += get_curr_stat(ch, STAT_DEX) - get_curr_stat(victim, STAT_DEX);

    /* меньше несешь - быстрее выпрыгнешь!  8)) */
    chance += victim->carry_weight / 200;

    /* хоббит - это не только ценная шкурка...  8)) */
    chance += 2 * (SIZE_MEDIUM - victim->size);

    switch(victim->classid)
    {
    case CLASS_WARRIOR:
    case CLASS_THIEF:
	chance -= 2;
	break;

    case CLASS_NAZGUL:
    case CLASS_PALADIN:
	chance -= 1;
	break;

    case CLASS_DRUID:
    case CLASS_RANGER:
	chance -= 3;
	break;

	/* Ну, некросам положено из-под земли уметь выбираться... 8)) */
    case CLASS_NECROMANT:
    case CLASS_VAMPIRE:
	chance -=5;
	break;

    case CLASS_MAGE:
    case CLASS_CLERIC:
	chance += 1;
	break;

    default:
	break;
    }

    if (level + PK_RANGE < victim->level)
	chance = 0;

    if (!IS_NPC(victim))
    {	
        af.where = TO_AFFECTS;
        af.type = gsn_earthmaw;
        af.level = ch->level;
        af.duration = 5;
        af.modifier = 0;
        af.location = APPLY_NONE;
        af.bitvector = 0;
        af.caster_id = get_id(ch);
        affect_to_char(ch, &af);
    }

    act("{W$n засыпает $N3 землей!{x", ch, NULL, victim, TO_NOTVICT);
    act("{W$n засыпает тебя землей!{x", ch, NULL, victim, TO_VICT);

    check_killer(ch, victim);

    if (number_percent() < chance && !saves_spell(level, victim, DAM_BASH))
    {
	/* Убили */
	act("{WТы засыпаешь $N3 землей!{x", ch, NULL, victim, TO_CHAR);
	send_to_char("{RТебе не выбраться из под этой кучи земли! "
		     "Ты задыхаешься...\n\r", victim);
	for_killed_skills(ch, victim);
    }
    else
    {
	/* просто чуть-чуть покоцаем... */
	if (saves_spell(level, victim, DAM_BASH))
	    dam /= 2;

	damage(ch, victim, dam, gsn_earthmaw, DAM_BASH, TRUE, NULL);
    }


    return TRUE;
}

/*
 * "позвать зверя" и "оживить дерево"
 * смысл вот в чем - "позвать зверя" создает одного чармиса, по статсам - дамагера,
 * т.е. со слабой защитой и большими повреждениями, и почти полным отсутствием
 * защитных OFF_* битов. "оживить дерево" создает чармиса-танка, т.е. со слабыми
 * повреждениями, хорошей защитой и полным набором защитный OFF_* битов при
 * отсутствии атакующих.
 * Иными словами, оптимальный набор чармисов для друида (если не использовать
 * "очаровать зверя") - дерево и позванная зверушка. Дерево танкует, зверушка
 * бьет, друид кастит помаленьку.
 */

bool spell_animate_tree(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *tree, *tch;
    MOB_INDEX_DATA *pIndex;
    AFFECT_DATA af;

    if (IS_NPC(ch))
	return FALSE;

    if (IS_SET(ch->in_room->room_flags, ROOM_NO_MOB))
    {
	send_to_char("Только не здесь!\n\r", ch);
	return FALSE;
    }

    if (!IS_IN_FOREST(ch))
    {
	send_to_char("Хм... А тут есть деревья?!\n\r", ch);
	return FALSE;
    }

    /*
     * Проверка аффекта. Объяснение ниже.
     */
    if (is_affected(ch, sn))
    {
	send_to_char("К сожалению, ты еще не набрался сил, чтобы оживить "
		     "еще одно дерево.\n\r", ch);
	return FALSE;
    }

    LIST_FOREACH(tch, &char_list, link)
    {
	if (IS_NPC(tch)
	    && tch->leader == ch
	    && tch->pIndexData->vnum == MOB_VNUM_DRUIDS_TREE)
	{
	    send_to_char("Многовато будет!\n\r", ch);
	    return FALSE;
	}
    }

    if (max_count_charmed(ch))
	return FALSE;

    if ((pIndex = get_mob_index(MOB_VNUM_DRUIDS_TREE)) == NULL)
    {
	bugf("Check limbo.are! Mob with vnum=%d is absent.",
	     MOB_VNUM_DRUIDS_TREE);
	return FALSE;
    }

    tree = create_mobile(pIndex);
    char_to_room(tree, ch->in_room, FALSE);
    add_follower(tree, ch);
    tree->leader = ch;
    SET_BIT(tree->affected_by, AFF_CHARM);

    tree->level = ch->level;
    tree->hit = tree->max_hit = 2 * ch->max_hit;
    tree->mana = tree->max_mana = ch->max_mana * 4 / 3;
    tree->move = tree->max_move = ch->max_move * 4 / 3;
    tree->hitroll = GET_HITROLL(ch) * 3 / 4;
    tree->damroll = GET_DAMROLL(ch) * 3 / 4;
    SET_BIT(tree->off_flags, OFF_DODGE | OFF_PARRY | OFF_BERSERK | OFF_BASH);
    SET_BIT(tree->act, ACT_NOEXP);
    tree->resists[DAM_WEAPON]    += 33;
    tree->resists[DAM_FIRE]      -= 50;
    tree->resists[DAM_NEGATIVE]  -= 10;
    tree->resists[DAM_COLD]      -= 10;
    tree->resists[DAM_LIGHTNING] -= 20;
    tree->resists[DAM_POISON] 	 += 200;
    tree->resists[DAM_DISEASE] 	 += 200;

    /*
     * Добавляем аффект, а то есть абьюз вот такой:
     * все чармисы друида бьют, он "неслед", опять зовет...
     * Дальше понятно?  8))
     */
    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = ch->level;
    af.duration = 10;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(ch, &af);

    return TRUE;
}

int get_druid_animal(CHAR_DATA *ch)
{
    if ((ch->level < 36) && (ch->level > 30))
	return MOB_VNUM_DRUIDS_WOLF;
    else if (ch->level < 41)
	return MOB_VNUM_DRUIDS_LION;
    else if (ch->level < 46)
	return MOB_VNUM_DRUIDS_BEAR;
    else
	return MOB_VNUM_DRUIDS_DRAGON;
}


bool spell_call_animal(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *animal, *tch;
    MOB_INDEX_DATA *pIndex;
    AFFECT_DATA af;
    int vnum;

    if (IS_NPC(ch))
	return FALSE;

    if (!IS_IN_WILD(ch))
    {
	send_to_char("Ты не можешь сделать этого здесь.\n\r", ch);
	return FALSE;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_NO_MOB))
    {
	send_to_char("Только не здесь!\n\r", ch);
	return FALSE;
    }

    if (is_affected(ch, sn))
    {
	send_to_char("К сожалению, у тебя не хватает силы, чтобы позвать "
		     "еще одно животное.\n\r", ch);
	return FALSE;
    }

    LIST_FOREACH(tch, &char_list, link)
    {
	if (IS_NPC(tch) && tch->leader == ch
	    && ((tch->pIndexData->vnum == MOB_VNUM_DRUIDS_WOLF)
		||  (tch->pIndexData->vnum == MOB_VNUM_DRUIDS_LION)
		||  (tch->pIndexData->vnum == MOB_VNUM_DRUIDS_BEAR)
		||  (tch->pIndexData->vnum == MOB_VNUM_DRUIDS_DRAGON)))
	{
	    send_to_char("Многовато будет!\n\r", ch);
	    return FALSE;
	}
    }

    if (max_count_charmed(ch))
	return FALSE;

    vnum = get_druid_animal(ch);
    if ((pIndex = get_mob_index(vnum)) == NULL)
    {
	bugf("Check limbo.are! Mob with vnum=%d is absent.", vnum);
	return FALSE;
    }

    animal = create_mobile(pIndex);
    char_to_room(animal, ch->in_room, FALSE);
    add_follower(animal, ch);
    animal->leader = ch;
    SET_BIT(animal->affected_by, AFF_CHARM);

    animal->level = ch->level;
    animal->hit = animal->max_hit = ch->max_hit * 4/3;
    animal->mana = animal->max_mana = ch->max_mana * 2;
    animal->move = animal->max_move = ch->max_move * 4 / 3;
    animal->hitroll = GET_HITROLL(ch) * 2;
    animal->damroll = GET_DAMROLL(ch) * 6 / 4;
    SET_BIT(animal->off_flags,
	    OFF_FAST
	    | OFF_KICK_DIRT
	    | OFF_BERSERK
	    | OFF_BASH
	    | OFF_DISARM
	    | OFF_CLEAVE);
    SET_BIT(animal->act, ACT_NOEXP);

    animal->resists[DAM_MENTAL] += 10;
    animal->resists[DAM_WEAPON] -= 10;

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = ch->level;
    af.duration = 10;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(ch, &af);

    return TRUE;
}

/*
 * "перекинуться"
 * Ну понятно... Перекинуться в ту зверушку, которую можно позвать.
 * Кстати, надо бы ввести флаг на оружии - ритуальное. В смысле,
 * что его можно использовать в ритуалах. В перекидывании, еще что-нибудь
 * придумаю... Или даже лучше тип новый оружия ввести... Стиля экзотического.
 */
bool spell_shapechange(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *animal;
    MOB_INDEX_DATA *pIndex;
    OBJ_DATA *ritual;
    int vnum;
    AFFECT_DATA af;

    if (IS_NPC(ch) || ch->desc == NULL)
	return FALSE;

    if (ch->desc->original != NULL)
    {
	send_to_char("Ты уже чувствуешь себя зверем...\n\r", ch);
	return FALSE;
    }

    if (!IS_IN_FOREST(ch))
    {
	send_to_char("Ты никак не можешь найти подходящего пенька...\n\r", ch);
	return FALSE;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_NO_MOB))
    {
	send_to_char("Только не здесь!\n\r", ch);
	return FALSE;
    }

    if (MOUNTED(ch))
    {
	send_to_char("Тебе сначала нужно спешиться.\n\r", ch);
	return FALSE;
    }


    if (((ritual = get_eq_char(ch, WEAR_WIELD)) == NULL)
	|| (ritual->item_type != ITEM_WEAPON)
	|| (ritual->value[0] != WEAPON_DAGGER))
    {
	send_to_char("Хм... Ты не находишь подходящего оружия для проведения "
		     "ритуала.\n\r", ch);
	return FALSE;
    }

    vnum = get_druid_animal(ch);

    if ((pIndex = get_mob_index(vnum)) == NULL)
    {
	bugf("Check limbo.are! Mob with vnum=%d is absent.", vnum);
	return FALSE;
    }

    extract_obj(ritual, TRUE, TRUE);

    animal = create_mobile(pIndex);
    char_to_room(animal, ch->in_room, FALSE);
    /*	char_from_room(ch);
     char_to_room(ch, get_room_index(ROOM_VNUM_LIMBO)); */

    animal->hit  = ch->hit;
    animal->mana = ch->mana;
    animal->move = ch->move;
    animal->max_hit = ch->max_hit;
    animal->max_mana = ch->max_mana;
    animal->max_move = ch->max_move;
    animal->hitroll = GET_HITROLL(ch);
    animal->damroll = GET_DAMROLL(ch);

    /*	ch->desc->character = animal;
     ch->desc->original = ch;
     animal->desc = ch->desc;
     ch->desc = NULL;

     if (ch->prompt != NULL)
     animal->prompt = str_dup(ch->prompt);

     animal->comm = COMM_NOTELL|COMM_NOSHOUT|COMM_NOCHANNELS;
     animal->lines = ch->lines;
     animal->level = ch->level; */

    do_switch(ch, animal->name);
    animal->comm |= COMM_NOTELL|COMM_NOSHOUT|COMM_NOCHANNELS;
    SET_BIT(animal->act, ACT_NOEXP);
    REMOVE_BIT(animal->act, ACT_IS_HEALER);

    char_from_room(ch);

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level/2;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(animal, &af);

    send_to_char("Ты становишься животным...\n\r", animal);

    return TRUE;
}

bool spell_detect_landscape(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    int i;
    char buf[MAX_STRING_LENGTH];

    if (ch->in_room == NULL)
	return FALSE;

    for (i = 0; !IS_NULLSTR(sector_flags[i].name); i++)
	if (sector_flags[i].bit == ch->in_room->sector_type)
	    break;

    if (IS_NULLSTR(sector_flags[i].name))
    {
	send_to_char("Невозможно определить, что это за местность.\n\r", ch);
	return FALSE;
    }

    sprintf(buf, "Тебя окружа%s %s.\n\r", decompose_end(sector_flags[i].rname), sector_flags[i].rname);
    send_to_char(buf, ch);

    return TRUE;
}

bool spell_cultivate_forest(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;

    if (ch->in_room == NULL)
	return FALSE;

    switch (ch->in_room->sector_type)
    {
    case SECT_INSIDE:
    case SECT_CITY:
    case SECT_AIR:
    case SECT_WATER_SWIM:
    case SECT_WATER_NOSWIM:
	send_to_char("Здесь у тебя не получится вырастить лес.\n\r", ch);
	return FALSE;
    case SECT_FOREST:
	send_to_char("Тебя и так уже окружают деревья!\n\r", ch);
	return FALSE;
    }

    af.where = TO_ROOM_AFF;
    af.type = sn;
    af.level = level;
    af.duration = level/3;
    af.modifier = SECT_FOREST - ch->in_room->sector_type;
    af.location = ROOM_APPLY_SECTOR;
    af.bitvector = 0;
    affect_to_room(ch->in_room, &af);

    send_to_char("Ты выращиваешь лес!\n\r", ch);
    act("$n бормочет что-то, ходит, приглядывается к земле, и здесь "
	"вырастает лес!", ch, NULL, NULL, TO_ROOM);

    return TRUE;	
}


bool spell_stunning_word(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;
    int dam = dice(level/2, 6);

    if (victim == ch)
    {
	send_to_char("Оглушить самого себя? Да ты в своем уме?\n\r", ch);
	return FALSE;
    }

    if (IS_VAMPIRE(victim))
    {
	send_to_char("Твое заклинание не производит никакого эффекта!\n\r", ch);
	return FALSE;
    }

    if (is_affected(victim, sn))
    {
	send_to_char("Это существо уже и так оглушено!\n\r", ch);
	return FALSE;
    }

    if (saves_spell(level, victim, DAM_OTHER))
    {
	send_to_char("Неудачно!\n\r", ch);
	return TRUE;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level/25;
    af.location = APPLY_HITROLL;
    af.modifier = -level/7;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    af.location = APPLY_DAMROLL;
    affect_to_char(victim, &af);

    af.location = APPLY_DEX;
    af.modifier = -level/25;
    affect_to_char(victim, &af);

    af.location = APPLY_INT;
    af.modifier = -level/15;
    affect_to_char(victim, &af);

    act("{WТы оглушаешь $N3!{x", ch, NULL, victim, TO_CHAR);
    act("{W$n оглушает $N3!{x", ch, NULL, victim, TO_NOTVICT);
    act("{W$n оглушает тебя!{x", ch, NULL, victim, TO_VICT);

    WAIT_STATE(victim, 3 * skill_table[sn].beats);

    if (saves_spell(level, victim, DAM_OTHER))
	dam /= 2;

    damage(ch, victim, dam, sn, DAM_OTHER, TRUE, NULL);
    return TRUE;
}

bool spell_wrath(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam = dice(level, 14);

    if (!IS_GOOD(ch))
    {
	send_to_char("Боги не хотят помогать тебе.\n\r", ch);
	return FALSE;
    }

    act("$n восклицает слова божественного гнева!", ch, NULL, NULL, TO_ROOM);
    send_to_char("Ты восклицаешь слова божественного гнева.\n\r", ch);

    if (IS_GOOD(victim))
    {
	send_to_char("Боги защищают это создание.\n\r", ch);
	return FALSE;
    }
    else if (IS_NEUTRAL(victim))
    {
	dam /= 2;
    }
    else if (IS_EVIL(victim))
    {
	spell_dispel_evil(skill_lookup("dispel evil"),level, ch, victim, TAR_CHAR_OFFENSIVE);
	if (victim == NULL)
	    return TRUE;
    }

    if (saves_spell(level, victim, DAM_HOLY))
	dam /= 2;

    damage(ch, victim, dam, sn, DAM_HOLY, TRUE, NULL);

    return TRUE;
}

bool spell_bone_shield(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *animal, *tch;
    MOB_INDEX_DATA *pIndex;
    OBJ_DATA *obj;
    int i;

    if (IS_NPC(ch))
	return FALSE;

    LIST_FOREACH(tch, &char_list, link)
    {
	if (IS_NPC(tch)
	    && tch->leader == ch
	    && tch->pIndexData->vnum == MOB_BONE_SHIELD)
	{
	    send_to_char("У тебя уже есть костяной щит.\n\r", ch);
	    return FALSE;
	}
    }

    /*	if (max_count_charmed(ch))
     return; */

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
	if (obj->pIndexData->vnum == OBJ_VNUM_SKELETON)
	    break;

    if (obj == NULL)
    {
	send_to_char("У тебя нет скелета.\n\r", ch);
	return FALSE;
    }

    if (obj->level > ch->level + 3)
    {
	send_to_char("У тебя не хватит сил поднять такого скелета!\n\r", ch);
	return FALSE;
    }

    if ((pIndex = get_mob_index(MOB_BONE_SHIELD)) == NULL)
    {
	bugf("Check limbo.are! Mob with vnum=%d is absent.", MOB_BONE_SHIELD);
	return FALSE;
    }

    animal = create_mobile(pIndex);
    char_to_room(animal, ch->in_room, FALSE);
    add_follower(animal, ch);
    animal->leader = ch;
    SET_BIT(animal->affected_by, AFF_CHARM);
    SET_BIT(animal->act, ACT_NOEXP);

    animal->level = obj->level;
    animal->hit = animal->max_hit = number_range(obj->level * 18,
						 (obj->level + 1) * 20);
    animal->mana = animal->max_mana = 100 + (obj->level - 1) * 3;
    animal->move = animal->max_move = 100 + (obj->level - 1) * 10;

    for (i = 0; i < 4; i++)
	animal->armor[i] = ch->armor[i];

    extract_obj(obj, FALSE, FALSE);

    return TRUE;
}

bool spell_make_old(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj;
    CHAR_DATA *victim = NULL;

    if ((obj = get_obj_list(ch, target_name, ch->in_room->contents)) == NULL)
    {
	if ((victim = get_char_room(ch, NULL, target_name, TRUE)) == NULL
	    && (victim = ch->fighting) == NULL)
	{
	    if (!too_many_victims)
		send_to_char("Ты не видишь этого здесь!\n\r", ch);
	    return FALSE;
	}
    }

    if (obj != NULL)
    {
	if ((obj->item_type == ITEM_CORPSE_NPC
	     || obj->item_type == ITEM_CORPSE_PC)
	    && can_loot(ch, obj))
	{
	    OBJ_DATA *skel;

	    if (obj->level > ch->level + 3)
	    {
		send_to_char("У тебя не хватит духа разложить этот труп!\n\r",
			     ch);
		return FALSE;
	    }

	    skel = create_object(get_obj_index(OBJ_VNUM_SKELETON), 0);
	    skel->level = obj->level;
	    skel->value[4] = obj->value[4];
	    obj_to_room(skel, ch->in_room);
	    dump_container(obj);
	    send_to_char("Ты разлагаешь труп в скелет.\n\r", ch);
	    act("$n разлагает труп в скелет.", ch, NULL, NULL, TO_ROOM);
	    return TRUE;
	}
	send_to_char("Ты не можешь это разложить.\n\r", ch);
	return FALSE;
    }

    if (victim != NULL)
    {
	AFFECT_DATA af;
	int dam = dice(level/2, 5);

	if (IS_VAMPIRE(victim))
	{
	    send_to_char("Твое заклинание не производит никакого эффекта!\n\r",
			 ch);
	    return FALSE;
	}

	if (is_safe_spell(ch, victim, TRUE))
	    return TRUE;

	if (saves_spell(level, victim, DAM_NEGATIVE))
	    dam /= 2;
	else if (!is_affected(victim, sn))
	{
	    af.where = TO_AFFECTS;
	    af.type = sn;
	    af.level = level;
	    af.duration = level/15;
	    af.location = APPLY_DEX;
	    af.modifier = -level/15;
	    af.bitvector = 0;
	    af.caster_id = get_id(ch);
	    affect_to_char(victim, &af);

	    af.where = TO_RESIST;
	    af.location = APPLY_NONE;
	    af.modifier = -10;
	    af.bitvector = DAM_SOUND;
	    affect_to_char(victim, &af);

	    send_to_char("Ты чувствуешь себя очень старо...\n\r", victim);
	    act("{W$N чувствует себя очень старо...{x",
		ch, NULL, victim, TO_ROOM);
	    act("{W$N чувствует себя очень старо...{x",
		ch, NULL, victim, TO_CHAR);
	}
	else
	    act("$N уже чуствует себя очень старо...", ch, NULL, victim, TO_CHAR);
	
	damage(ch, victim, dam, sn, DAM_NEGATIVE, TRUE, NULL);
    }
    return TRUE;
}

OBJ_DATA *get_soul(CHAR_DATA *ch, char *arg)
{
    OBJ_DATA *obj;

    if ((obj = get_obj_list(ch, arg, ch->in_room->contents)) == NULL)
    {
	send_to_char("Здесь нет этого.\n\r", ch);
	return NULL;
    }

    if (obj->pIndexData->vnum != OBJ_VNUM_SOUL)
    {
	send_to_char("Это не является душой.\n\r", ch);
	return NULL;
    }

    if (obj->level > ch->level + 5)
    {
	send_to_char("Ты не сможешь справиться со столь мощной душой.\n\r", ch);
	return NULL;
    }

    return obj;
}

bool spell_wind_of_death(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj;
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    target_name = one_argument(target_name, arg);

    if ((obj = get_soul(ch, arg)) == NULL)
	return FALSE;

    if (target_name[0] == '\0')
	victim = ch;
    else
    {
	if ((victim = get_char_room(ch, NULL, target_name, FALSE)) == NULL)
	{
	    send_to_char("Таких здесь нет.\n\r", ch);
	    return FALSE;
	}

	if ((!IS_NPC(victim) && victim->classid != CLASS_NECROMANT)
	    || (IS_NPC(victim) && victim->pIndexData->vnum != MOB_VNUM_ZOMBIE))
	{
	    send_to_char("Ты можешь вдыхать энергию души только в некромантов "
			 "или зомби.\n\r", ch);
	    return FALSE;
	}
    }

    victim->hit = UMIN(victim->max_hit, victim->hit + 3 * obj->level);
    victim->mana = UMIN(victim->max_mana, victim->mana + obj->level);
    extract_obj(obj, FALSE, FALSE);

    send_to_char("Ты пожираешь душу!\n\r", ch);
    act("{W$n пожирает душу!{x", ch, NULL, victim, TO_NOTVICT);

    if (ch != victim)
	act("{W$n пожирает душу и вдыхает ее энергию в тебя!{x",
	    ch, NULL, victim, TO_VICT);

    return TRUE;
}

bool spell_aura_of_dust(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj;
    AFFECT_DATA af;

    if (!is_replace(sn, level, ch))
    {
	char buf[MAX_STRING_LENGTH];

	sprintf(buf, "Ты уже окутан%s тончайшей аурой из праха.\n\r",
		SEX_ENDING(ch));
	send_to_char(buf, ch);
	return FALSE;
    }

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
	if (obj->pIndexData->vnum == OBJ_VNUM_SKELETON)
	    break;

    if (obj == NULL)
    {
	send_to_char("У тебя нет скелета.\n\r", ch);
	return FALSE;
    }

    extract_obj(obj, FALSE, FALSE);

    affect_strip(ch, sn);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/5;
    af.location  = APPLY_AC;
    af.modifier  = -12 * level / 10;
    af.bitvector = 0;
    af.caster_id = 0;
    affect_to_char(ch, &af);

    af.location  = APPLY_SAVES;
    af.modifier  = -level/10;
    affect_to_char(ch, &af);

    af.location  = APPLY_HITROLL;
    af.modifier  = -level/10;
    affect_to_char(ch, &af);

    send_to_char("Ты покрываешься тончайшей аурой праха.\n\r", ch);
    act("$n покрывается тончайшей аурой праха.", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool spell_bone_whirlwind(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj;
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
	if (obj->pIndexData->vnum == OBJ_VNUM_SKELETON)
	    break;

    if (obj == NULL)
    {
	send_to_char("У тебя нет скелета.\n\r", ch);
	return FALSE;
    }

    dam = dice(level, 10) + dice(obj->level, 10);

    extract_obj(obj, FALSE, FALSE);

    if (saves_spell(level, victim, DAM_PIERCE))
	dam /= 2;

    damage(ch, victim, dam, sn, DAM_PIERCE, TRUE, NULL);
    return TRUE;
}

bool spell_blood_signs(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;

    if (!is_replace(sn, level, ch))
    {
	act("Ты уже покрыт$t кровавыми знаками.", ch, SEX_ENDING(ch), NULL, TO_CHAR);
	return FALSE;
    }

    affect_strip(ch, sn);

    af.where     = TO_RESIST;
    af.type      = sn;
    af.level     = level;
    af.duration  = 4;
    af.location  = APPLY_NONE;
    af.modifier  = 20;
    af.bitvector = DAM_WEAPON;
    af.caster_id = get_id(ch);
    affect_to_char(ch, &af);

    af.bitvector = DAM_WOOD;
    affect_to_char(ch, &af);

    af.bitvector = DAM_IRON;
    affect_to_char(ch, &af);

    send_to_char("Ты покрываешься кровавыми знаками...\n\r", ch);
    act("$n покрывает свое тело кровавыми знаками.", ch, NULL, NULL, TO_ROOM);
    return TRUE;
}

bool spell_nets(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj;
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ((obj = get_hold_meat(ch)) == NULL)
        return FALSE;

    if (is_affected(victim, sn))
    {
	send_to_char("Твой противник уже и так окутан "
		     "тончайшими щупальцами.\n\r", ch);
	return FALSE;
    }

    extract_obj(obj, TRUE, FALSE);

    send_to_char("Ты бросаешь кусок мяса, и в полете он превращается "
		 "в клубок тончайших розовых щупалец!\n\r", ch);
    act("$n бросает кусок мяса, и в полете он превращается "
	"в клубок тончайших розовых щупалец!", ch, NULL, NULL, TO_ROOM);

    if (IS_VAMPIRE(victim))
    {
	act("Но... В полете щупальца превращаются "
	    "в тончайший прах и падают $x.", ch, NULL, NULL, TO_CHAR);
	return FALSE;
    }

    if (!saves_spell(level, victim, DAM_OTHER))
    {
	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = level;
	af.bitvector = 0;
	af.duration  = (level + 9) / 30;
	af.caster_id = get_id(ch);
	af.location  = APPLY_DEX;
	af.modifier  = -level/17;
	affect_to_char(victim, &af);

	WAIT_STATE(victim, 2 * skill_table[sn].beats);
	act("$N запутывается в щупальцах...", ch, NULL, victim, TO_ROOM);
	act("$N запутывается в щупальцах...", ch, NULL, victim, TO_CHAR);
	send_to_char("Ты запутываешься в щупальцах!", victim);
    }
    else
	send_to_char("И ничего не происходит...\n\r", ch);

    return TRUE;
}

bool spell_turn_undead(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;

    if (POS_FIGHT(ch))
    {
	send_to_char("В бою? Кхм... Сконцентрируйся получше.\n\r", ch);
	return FALSE;
    }

    if (!IS_UNDEAD(victim))
    {
	send_to_char("Это существо не обращает никакого внимания "
		     "на твое заклинание.\n\r", ch);
	return FALSE;
    }

    send_to_char("Ты пытаешься обратить нежить...\n\r", ch);
    act("$n пытается уничтожить твою неживую сущность!",
	ch, NULL, victim, TO_VICT);

    if (saves_spell(level, victim, DAM_HOLY)
	|| number_percent() > (ch->level - victim->level) * 30
	|| !IS_NPC(victim))
    {
	damage(ch, victim, dice(level, 8), sn, DAM_HOLY, TRUE, NULL);
    }
    else
    {
	act("{WТы УНИЧТОЖАЕШЬ $N3!{x", ch, NULL, victim, TO_CHAR);
	act("{W$n УНИЧТОЖАЕТ $N3!{x", ch, NULL, victim, TO_ROOM);
	for_killed_skills(ch, victim);
    }

    return TRUE;
}

bool spell_soul_frenzy(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj;
    CHAR_DATA *gch, *next;
    int dam;

    if ((obj = get_soul(ch, target_name)) == NULL)
	return FALSE;

    dam = dice((level + obj->level), 16);

    extract_obj(obj, FALSE, FALSE);

    send_to_char("Ты направляешь ярость витающей здесь души на своих "
		 "противников!\n\r", ch);

    LIST_FOREACH_SAFE(gch, &ch->in_room->people, room_link, next)
    {
	/* не действует на некроса и членов его группы */
	if (is_same_group(gch, ch) || is_safe_spell(ch, gch, TRUE))
	    continue;

	/*
	 * Если среди противников оказывается вампир,
	 * то душа в ужасе сваливает на небеса, переставая бить...
	 */
	if (IS_VAMPIRE(gch))
	{
	    act("Душа в ужасе бежит от $N1!!", ch, NULL, gch, TO_NOTVICT);
	    break;
	}

	if (!saves_spell(level, gch, DAM_MENTAL))
	{
	    act("{W$n направляет ярость витающей здесь души на тебя!{x",
		ch, NULL, gch, TO_VICT);
	    damage(ch, gch, dam, sn, DAM_MENTAL, TRUE, NULL);
	}
	check_offensive(sn, TAR_CHAR_OFFENSIVE, ch, gch, TRUE);
    }

    return TRUE;
}

bool spell_fade(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn))
    {
	if (ch == victim)
	    send_to_char("Ты и так выглядишь очень "
		         "устало и болезненно...\n\r", ch);
	else
	    send_to_char("Это существо уже и так выглядит очень "
		         "устало и болезненно...\n\r", ch);
	return FALSE;
    }

    if (IS_VAMPIRE(victim))
    {
	send_to_char("Твое заклинание не произвело никакого эффекта.\n\r", ch);
	return FALSE;
    }

    if (saves_spell(level, victim, DAM_DISEASE)
	|| (IS_NPC(victim) && IS_SET(victim->act, ACT_UNDEAD)))
    {
	if (ch == victim)
	    send_to_char("Ты чувствуешь кратковременную боль, "
			 "но это проходит.\n\r", ch);
	else
	    act("$N выглядит очень болезненно, но это быстро проходит.",
		ch, NULL, victim, TO_CHAR);
	return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type 	 = sn;
    af.level	 = level;
    af.duration  = level/10;
    af.location  = APPLY_STR;
    af.modifier  = - level/10;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    send_to_char("Ты чувствуешь, как процессы жизнедеятельности твоего "
		 "организма останавливаются...\n\r", victim);
    act("$n выглядит так, как будто $s внутренние органы отказываются "
	"работать.", victim, NULL, NULL, TO_NOTVICT);
    return TRUE;	
}

bool spell_dead_warrior(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj, *next_obj;
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];

    target_name = one_argument(target_name, arg);

    if ((victim = get_char_room(ch, NULL, arg, FALSE)) == NULL)
    {
	send_to_char("Таких здесь нет.\n\r", ch);
	return FALSE;
    }

    if (!IS_NPC(victim)
	|| !IS_AFFECTED(victim, AFF_CHARM)
	|| victim->master != ch
	|| victim->pIndexData->vnum != MOB_VNUM_ZOMBIE
	|| IS_SET(victim->act, ACT_DEAD_WARRIOR))
    {
	send_to_char("Это вряд ли целесообразно.\n\r", ch);
	return FALSE;
    }

    one_argument(target_name, arg);

    if ((obj = get_soul(ch, arg)) == NULL)
	return FALSE;

    if (victim->level > obj->level + 5)
    {
	send_to_char("Эта душа слишком слаба для этого зомби.\n\r", ch);
	return FALSE;
    }

    extract_obj(obj, FALSE, FALSE);
    SET_BIT(victim->act, ACT_DEAD_WARRIOR);

    act("Ты вдыхаешь душу в $N3!", ch, NULL, victim, TO_CHAR);
    act("$n вдыхает душу в $N3!", ch, NULL, victim, TO_ROOM);

    for (obj = victim->carrying; obj != NULL; obj = next_obj)
    {
	next_obj = obj->next_content;
	if (number_bits(1) == 0)
	    wear_obj(victim, obj, FALSE);
    }

    return TRUE;
}


bool spell_protection_light(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    CHAR_DATA *victim;

    victim = (vo) ? (CHAR_DATA *) vo : ch;

    if (!is_replace(sn, level, victim))
    {
	send_to_char("Тьма и так защищает тебя.\n\r", ch);
	return FALSE;
    }
 
    affect_strip(victim, sn);

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.location = APPLY_SAVES;
    af.modifier = -(level + 3)/10;
    af.duration = level/7;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    af.where = TO_RESIST;
    af.location = APPLY_NONE;
    af.modifier = 33;
    af.bitvector = DAM_LIGHT;
    affect_to_char(victim, &af);

    send_to_char("Тьма ласково окружает тебя.\n\r", victim);
    act("На $n3 опускается Тьма!", victim, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool spell_vampiric_touch(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int dam;

    if (target == TARGET_CHAR && vo)
    {
	victim = (CHAR_DATA *) vo;

	if (ch == victim)
	{
	    send_to_char("Ты мазохист?\n\r", ch);
	    return FALSE;
	}

	dam = dice(level * 2, 5);

	if (saves_spell(level, victim, DAM_NEGATIVE))
	    dam /= 2;
	else
	{
	    ch->alignment = UMAX(-1000, ch->alignment - 10);
	    if (!is_affected(victim, sn))
	    {
		af.where = TO_AFFECTS;
		af.type = sn;
		af.level = level;
		af.duration = level/20;
		af.location = APPLY_STR;
		af.modifier = -level/9;
		af.bitvector = AFF_WEAKEN;
		af.caster_id = get_id(ch);
		affect_to_char(victim, &af);

		af.location = APPLY_DEX;
		af.bitvector = AFF_SLOW;
		affect_to_char(victim, &af);

		send_to_char("Тьма, какое же удовольствие!!\n\r", ch);
		send_to_char("Ты чувствуешь смертельный холод.\n\r", victim);
	    }
	}

	damage(ch, victim, dam, sn, DAM_NEGATIVE, TRUE, NULL);
	return TRUE;
    }
    else if (target == TARGET_OBJ && vo)
    {
	obj = (OBJ_DATA *) vo;

	if (obj->item_type != ITEM_WEAPON)
	{
	    send_to_char("Ты можешь колдовать это заклинание только на "
			 "оружие.\n\r", ch);
	    return FALSE;
	}

	if (obj->level > ch->level)
	{
	    send_to_char("У тебя не хватает сил.\n\r", ch);
	    return FALSE;
	}

	if (IS_OBJ_STAT(obj, ITEM_BLESS))
	{
	    send_to_char("Это оружие благословлено!\n\r", ch);
	    return FALSE;
	}

	if (IS_WEAPON_STAT(obj, WEAPON_POISON)
	    || IS_WEAPON_STAT(obj, WEAPON_FLAMING)
	    || IS_WEAPON_STAT(obj, WEAPON_SHOCKING))
	{
	    send_to_char("Ничего не выходит.\n\r", ch);
	    return FALSE;
	}

	if (IS_WEAPON_STAT(obj, WEAPON_VAMPIRIC)
	    || IS_WEAPON_STAT(obj, WEAPON_MANAPIRIC))
	{
	    act("Зловещая аура на $p5 и так довольно сильна.",
		ch, obj, NULL, TO_CHAR);
	    return FALSE;
	}

	act("Ты протягиваешь руку, и $p6 окутывает зловещая аура.",
	    ch, obj, NULL, TO_CHAR);
	act("$n протягивает руку, и $p6 окутывает зловещая аура.",
	    ch, obj, NULL, TO_ROOM);

	obj->condition -= 5;
	check_condition(ch, obj);

	af.where = TO_WEAPON;
	af.type = sn;
	af.level = level;
	af.duration = level / 2;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = WEAPON_VAMPIRIC;
	affect_to_obj(obj, &af);

	af.bitvector = WEAPON_MANAPIRIC;
	affect_to_obj(obj, &af);

	return TRUE;
    }
    send_to_char("Ты не видишь ничего (и никого) похожего здесь.\n\r", ch);
    return FALSE;
}

bool spell_mist(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;

    if (!is_replace(sn, level, ch))
    {
	/* Фантазия иссякла... */
	send_to_char("Но ты уже под действием этого заклинания...\n\r", ch);
	return FALSE;
    }

    affect_strip(ch, sn);

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level / 10;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = 0;
    af.caster_id = 0;
    affect_to_char(ch, &af);
    send_to_char("Твои очертания расплываются...\n\r", ch);
    return TRUE;
}

bool spell_darkness(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;

    if (IS_SET(ch->in_room->room_flags, ROOM_DARK))
    {
	send_to_char("Сюда и так не попадают лучи солнца.\n\r", ch);
	return FALSE;
    }

    af.where = TO_ROOM_AFF;
    af.type = sn;
    af.level = level;
    af.duration = level / 2;
    af.modifier = 0;
    af.location = ROOM_APPLY_FLAGS;
    af.bitvector = ROOM_DARK;
    affect_to_room(ch->in_room, &af);

    send_to_char("Благодатная Тьма окружает тебя.\n\r", ch);
    act("Тьма сгущается вокруг!", ch, NULL, NULL, TO_ROOM);
    return TRUE;
}

bool spell_trickle(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    char arg[MAX_INPUT_LENGTH];
    int door;
    ROOM_INDEX_DATA *to_room;
    EXIT_DATA *ex;

    one_argument(target_name, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Через какую дверь ты хочешь просочиться?\n\r", ch);
	return FALSE;
    }

    if ((door = find_door(ch, arg)) >= 0)
    {
	ex = ch->in_room->exit[door];
	to_room = ex->u1.to_room;

	if (to_room == NULL)
	    return FALSE;

	/*	SET_BIT(ch->affected_by, AFF_PASS_DOOR);
	 move_char(ch, door, TRUE, FALSE);
	 REMOVE_BIT(ch->affected_by, AFF_PASS_DOOR);*/

	/* Здесь надо не move_char, а сразу char_from_room, char_to_room,
	 * естественно с проверками типа DOOR_NOPASS и т.п*/
	if (IS_SET(ex->exit_info, EX_NOPASS) || !can_see_room(ch, to_room)
		|| IS_SET(to_room->room_flags, ROOM_PRIVATE)
		|| IS_SET(to_room->room_flags, ROOM_SOLITARY)
		|| IS_SET(to_room->room_flags, ROOM_GUILD)
		|| IS_SET(to_room->room_flags, ROOM_HOUSE)
	        || IS_SET(to_room->area->area_flags, AREA_NA)
	        || IS_SET(to_room->area->area_flags, AREA_TESTING))
	{
	    send_to_char("Сквозь эти двери ты не сможешь просочиться.\n\r", ch);
	    return FALSE;
	}

	act("$n просачивается сквозь $d6!", ch, NULL, ex->keyword, TO_ROOM);
	act("Ты просачиваешься сквозь $d6!", ch, NULL, ex->keyword, TO_CHAR);
	act("$N просачивается сюда сквозь $t6!",
	    LIST_FIRST(&to_room->people), ex->keyword, ch, TO_ALL);

	char_from_room(ch);
	char_to_room(ch, to_room, TRUE);
	return TRUE;
    }
    else
    {
	send_to_char("Ты не видишь здесь этого...\n\r", ch);
	return FALSE;
    }
}

bool check_wait(CHAR_DATA *och, CHAR_DATA *ch);

bool spell_order(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    CHAR_DATA *master;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int64_t affects;

    if (IS_NPC(ch))
	return FALSE;

	if (ch->in_room && IS_SET(ch->in_room->room_flags, ROOM_HOLY) && IS_SET(ch->in_room->room_flags, ROOM_GUILD))
	{
	    send_to_char("Чем ты занимаешься в Святом месте? Оставь эти свои штучки до других мест.\n\r", ch);
	    return FALSE;;
	}

    target_name = one_argument(target_name, arg);

    if ((victim = get_char_room(ch, NULL, arg, TRUE)) == NULL)
    {
	if (!too_many_victims)
	    send_to_char("Таких здесь нет.\n\r", ch);
	return FALSE;
    }

    if (victim == ch)
    {
	send_to_char("Но ты и без того с радоcтью выполняешь свои приказы!\n\r",
		     ch);
	return FALSE;
    }

    if (victim->position == POS_SLEEPING || IS_IMMORTAL(victim))
    {
	send_to_char("Тебе не удается привлечь внимание своей жертвы.\n\r", ch);
	return FALSE;
    }

    if (IS_AFFECTED(victim, AFF_CHARM))
    {
	if (victim->master == ch)
	{
	    act("Но $N и без того подчиняется тебе!",
		ch, NULL, victim, TO_CHAR);
	    return FALSE;
	}
	else
	{
	    act("Но $N слишком любит своего хозяина!",
		ch, NULL, victim, TO_CHAR);
	    return FALSE;
	}
    }

    if (IS_SET(victim->act, ACT_AGGRESSIVE))
    {
	send_to_char("Тебе же хуже будет...\n\r", ch);
	return FALSE;
    }

    if (IS_NULLSTR(target_name))
    {
	send_to_char("Что именно ты хотел приказать?\n\r", ch);
	return FALSE;
    }

    one_argument(target_name, arg);

    if (check_disallow(arg))
    {
	send_to_char("Это не может быть сделано!\n\r", ch);
	return TRUE;
    }

    if (is_safe_spell(ch, victim, FALSE))
    {
	send_to_char("Нельзя приказать этому существу.\n\r", ch);
	return TRUE;
    }

    WAIT_STATE(ch, PULSE_VIOLENCE);

    if (saves_spell(level, victim, DAM_CHARM)
	|| saves_spell(level, victim, DAM_MENTAL))
    {
	sprintf(buf, "У тебя не получается заставить %s сделать что-либо.\n\r",
		PERS(victim, ch, 3));
	send_to_char(buf, ch);
    }
    else
    {
	//	if (check_wait(victim, ch))
	//	   return;
	//      Это на пробу, возможно, это лишнее.
	ch->wait = 0;

	master = victim->master;
	victim->master = ch;
	affects = victim->affected_by;
	SET_BIT(victim->affected_by, AFF_CHARM);
	sprintf(buf, "%s приказывает тебе: '%s'.\n\r", ch->name, target_name);
	send_to_char(buf, victim);
	send_to_char("Ok.\n\r", ch);
	interpret(victim, target_name);
	victim->master = master;
	victim->affected_by = affects;
    }

    check_offensive(sn, TAR_CHAR_OFFENSIVE, ch, victim, TRUE);

    return TRUE;
}

bool spell_death_aura(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;

    if (is_replace(sn, level, ch))
	affect_strip(ch, sn);

    if (IS_AFFECTED(ch, AFF_DEATH_AURA))
    {
	send_to_char("Тебя и так окружает незримая аура Смерти.\n\r", ch);
	return FALSE;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level / 10;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_DEATH_AURA;
    af.caster_id = 0;
    affect_to_char(ch, &af);

    af.where = TO_RESIST;
    af.modifier = 33;
    af.bitvector = DAM_HOLY;
    affect_to_char(ch, &af);

    send_to_char("Тебя окружает незримая аура Смерти!\n\r", ch);
    act("{DОт $n1 внезапно веет могильным холодом.{x", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool spell_acid_rain(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *vch, *vch_next;
    int dam;

    if (weather_info.sky != SKY_RAINING && weather_info.sky != SKY_LIGHTNING)
    {
	send_to_char("Для этого заклинания тебе нужна дождливая погода.\n\r",
		     ch);
	return FALSE;
    }

    if (!IS_OUTSIDE(ch))
    {
	send_to_char("Но ты же в помещении!\n\r", ch);
	return FALSE;
    }

    act("$n превращает капли дождя в кислоту!", ch, NULL, NULL, TO_ROOM);
    send_to_char("Ты превращаешь капли дождя в кислоту, и направляешь их на "
		 "своих противников!\n\r", ch);

    dam = dice(level, 30);

    acid_effect(victim->in_room, level, dam/3, TARGET_ROOM, ch);

    LIST_FOREACH_SAFE(vch, &victim->in_room->people, room_link, vch_next)
    {
	if (is_safe_spell(ch, vch, TRUE) || is_same_group(ch, vch))
	    continue;

	if (vch == victim) /* full damage */
	{
	    if (saves_spell(level, vch, DAM_ACID))
	    {
		acid_effect(vch, level/2, dam/4, TARGET_CHAR, ch);
		damage(ch, vch, dam/2, sn, DAM_ACID, TRUE, NULL);
	    }
	    else
	    {
		acid_effect(vch, level, dam, TARGET_CHAR, ch);
		damage(ch, vch, dam, sn, DAM_ACID, TRUE, NULL);
	    }
	}
	else /* partial damage */
	{
	    if (saves_spell(level - 2, vch, DAM_ACID))
	    {
		acid_effect(vch, level/4, dam/8, TARGET_CHAR, ch);
		damage(ch, vch, dam/4, sn, DAM_ACID, TRUE, NULL);
	    }
	    else
	    {
		acid_effect(vch, level/2, dam/4, TARGET_CHAR, ch);
		damage(ch, vch, dam/2, sn, DAM_ACID, TRUE, NULL);
	    }
	}
	check_offensive(sn, TAR_CHAR_OFFENSIVE, ch, vch, TRUE);
    }

    return TRUE;
}

bool spell_vam_blast(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    int dam;

    if (is_affected(ch, sn))
    {
	send_to_char("Ты не можешь набрать слюны для плевка.\n\r", ch);
	return FALSE;
    }

    dam = dice(level, 14);
    if (saves_spell(level, victim, DAM_NEGATIVE))
	dam /= 2;

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 0;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = 0;
    af.caster_id = 0;
    affect_to_char(ch, &af);

    damage(ch, victim, dam, sn, DAM_NEGATIVE, TRUE, NULL);
    ch->hit = UMIN(ch->max_hit, ch->hit + dam/10);

    return TRUE;
}

bool spell_nightfall(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vch, *safe_vch;
    OBJ_DATA  *light;
    AFFECT_DATA af;
    int count = 0;

    if (is_affected(ch, sn))
    {
	send_to_char("У тебя еще нет сил для управления светом.\n\r", ch);
	return FALSE;
    }

    if (!IS_NULLSTR(target_name))
    {
	if ((light = get_obj_carry(ch, target_name, ch)) == NULL)
	    send_to_char("У тебя нет такого предмета.\n\r", ch);
	else if (!IS_OBJ_STAT(light, ITEM_GLOW) && light->item_type != ITEM_LIGHT)
	    send_to_char("Эта вещь, вроде бы, и так не светится.\n\r", ch);
	else
	{
	    REMOVE_BIT(light->extra_flags, ITEM_GLOW);

	    if (light->item_type == ITEM_LIGHT && light->value[2] > 0)
	    {
		light->value[2] = 0;
		ch->in_room->light--;
	    }	

	    act("$p переста$r испускать мягкое свечение.", ch, light, NULL, TO_ALL);
	    return TRUE;
	}

	return FALSE;
    }

    LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, safe_vch)
    {
	if (!is_safe_spell(ch, vch, TRUE))
	{
	    for (light = vch->carrying;
		 light != NULL;
		 light = light->next_content)
	    {
		if (light->item_type == ITEM_LIGHT
		    && light->value[2] != 0
		    && !is_same_group(ch, vch)
		    && !saves_spell(level, vch, DAM_ENERGY))
		{
		    act("$p в руках у $N1 мигает и потухает!",
			ch, light, vch, TO_CHAR);
		    act("$p в руках у $N1 мигает и потухает!",
			ch, light, vch, TO_ROOM);
	            
		    light->value[2] = 0;

		    if (light->wear_loc == WEAR_LIGHT)
			ch->in_room->light--;

		    count++;
		}
	    }
	    check_offensive(sn, TAR_CHAR_OFFENSIVE, ch, vch, TRUE);
	}
    }

    for (light = ch->in_room->contents; light != NULL; light = light->next_content)
	if (light->item_type == ITEM_LIGHT && light->value[2] != 0)
	{
	    act("$p мигает и потухает!", ch, light, NULL, TO_CHAR);
	    act("$p мигает и потухает!", ch, light, NULL, TO_ROOM);
	    light->value[2] = 0;
	    ch->in_room->light--;
	    count++;
	}

    af.where	 = TO_AFFECTS;
    af.type    	 = sn;
    af.level 	 = level;
    af.duration	 = count;
    af.modifier	 = 0;
    af.location	 = APPLY_NONE;
    af.bitvector = 0;
    af.caster_id = 0;
    affect_to_char(ch, &af);

    if (count == 0)
	send_to_char("Тебе не удается потушить ни одного источника света.\n\r",
		     ch);

    return TRUE;
}

bool spell_sense_life(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;

    if (is_affected(ch, sn) || IS_AFFECTED(ch, AFF_DETECT_LIFE))
    {
	send_to_char("Ты уже и так хорошо различаешь жизненные формы.\n\r", ch);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type 	 = sn;
    af.level 	 = ch->level;
    af.duration  = ch->level;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_DETECT_LIFE;
    af.caster_id = 0;
    affect_to_char(ch, &af);

    act("Ты начинаешь чувствовать жизненные формы вокруг себя!",
	ch, NULL, NULL, TO_CHAR);
    act("$n выглядит более восприимчив$t.", ch, ch->sex == SEX_FEMALE ? "ой" : "ым", NULL, TO_ROOM);

    return TRUE;
}

bool spell_shadow_cloak(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn))
    {
	if (victim == ch)
	    send_to_char("Ты уже под защитой плаща теней.\n\r", ch);
	else
	    act("$N уже под защитой плаща теней.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type      = sn;
    af.level	 = level;
    af.duration  = level/3;
    af.modifier  = -level;
    af.location  = APPLY_AC;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    send_to_char("Ты чувствуешь, как тени защищают тебя!\n\r", victim);

    if (ch != victim)
	act("Плащ из теней защищает $N3.", ch, NULL, victim, TO_CHAR);
    return TRUE;
}

bool spell_roots(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (!IS_IN_WILD(ch))
    {
	send_to_char("Ты не можешь сделать этого здесь.\n\r", ch);
	return FALSE;
    }

    if (is_affected(victim, sn))
    {   
	act("Но ноги $N1 и так опутаны корнями.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    if (is_lycanthrope(victim))
    {
	send_to_char("Корни не удержат это существо.\n\r", ch);
	return FALSE;
    }

    if (saves_spell(level, victim, DAM_OTHER))
    {
	send_to_char("Не получается!\n\r", ch);
	return TRUE;
    }

    af.where = TO_AFFECTS;
    af.level = level;
    af.type = sn;
    af.duration = level / 20;
    af.modifier = -2;
    af.location = APPLY_DEX;
    af.bitvector = AFF_ROOTS;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    send_to_char("Вылезшие из-под земли корни опутывают твои ноги.\n\r", victim);
    act("Вылезшие из-под земли корни опутывают ноги $N2", ch, NULL, victim, TO_NOTVICT);
    act("Вылезшие из-под земли корни опутывают ноги $N2", ch, NULL, victim, TO_CHAR);
    return TRUE;
}

bool spell_cursed_lands(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    CHAR_DATA *vch, *safe_vch;

    if (ch->in_room == NULL)
	return FALSE;

    if (IS_SET(ch->in_room->room_flags, ROOM_LAW))
    {
	send_to_char("Это место защищено богами!\n\r", ch);
	return FALSE;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL))
    {
	send_to_char("Это место и так забыто богами.\n\r", ch);
	return FALSE;
    }

    LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, safe_vch)
    {
	if (!is_same_group(ch, vch))
	{
	    send_to_char("Ты не можешь проклясть это место, пока здесь "
			 "находится кто-то, кто не является твоим другом!\n\r",
			 ch);
	    return FALSE;
	}
    }

    af.where = TO_ROOM_AFF;
    af.type = sn;
    af.level = level;
    af.duration = level / 15;
    af.modifier = 0;
    af.location = ROOM_APPLY_FLAGS;
    af.bitvector = ROOM_NO_RECALL;
    affect_to_room(ch->in_room, &af);

    send_to_char("Боги забывают про это место.\n\r", ch);
    act("Боги забывают это место.\n\r", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool spell_vaccine(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    OBJ_DATA *obj;
    bool is_poison = FALSE, is_disease = FALSE;
    int i;

    if (IS_AFFECTED(victim, AFF_PLAGUE) || IS_AFFECTED(victim, AFF_POISON))
    {
	send_to_char("Поздно, теперь тут уже лечиться надобно.\n\r", ch);
	return FALSE;
    }

    if (ch->level < victim->level - 5)
    {
	send_to_char("Ну нет, этот персонаж слишком велик для твоего "
		     "заклинания.\n\r", ch);
	return FALSE;
    }

    if ((obj = get_eq_char(ch, WEAR_HOLD)) == NULL)
    {
	send_to_char("Ты должен держать в руках зелье лечения "
		     "болезни или яда.\n\r", ch);
	return FALSE;
    }

    for (i = 0; i < 3; i++)
    {
	if (obj->value[i] == gsn_cure_poison)
	    is_poison = TRUE;
	if (obj->value[i] == gsn_cure_disease)
	    is_disease = TRUE;
    }

    if (!is_disease && !is_poison)
    {
	send_to_char("Ну и какую вакцину ты хотел приготовить? "
		     "У тебя же совсем не то зелье.\n\r", ch);
	return FALSE;
    }

    if (is_affected(victim, sn)
	|| check_immune(victim, DAM_DISEASE) == 100
	|| check_immune(victim, DAM_POISON) == 100)
    {
	if (victim == ch)
	    send_to_char("Ты уже находишься под действием вакцины.\n\r", ch);
	else
	    act("$N уже находится под действием вакцины.",
		ch, NULL, victim, TO_CHAR);
	return FALSE;
    }


    af.where     = TO_RESIST;
    af.type      = sn;
    af.level     = (level + obj->level) / 2;

    i = 0;

    if (is_disease)
	i += af.level/5;
    if (is_poison)
	i += af.level/5;

    af.duration  = i;
    af.location  = APPLY_NONE;
    af.modifier  = 100;
    af.caster_id = get_id(ch);

    if (is_disease)
    {
	af.bitvector = DAM_DISEASE;
	affect_to_char(victim, &af);
	send_to_char("Ты чувствуешь, что в твоей крови появляется вакцина "
		     "от болезней.\n\r", victim);
    }

    if (is_poison)
    {
	af.bitvector = DAM_POISON;
	affect_to_char(victim, &af);
	send_to_char("Ты чувствуешь, что в твоей крови появляется вакцина "
		     "от яда.\n\r", victim);
    }

    if (ch != victim)
	act("$N получает от тебя прививку от разных болезней.",
	    ch, NULL, victim, TO_CHAR);

    extract_obj(obj, TRUE, TRUE);
    return TRUE;
}

bool spell_mirror_image(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (!is_replace(sn, level, victim))
    {
	if (victim == ch)
	    act("Ты уже окружен$t зеркальными отражениями.",
		ch, SEX_ENDING(ch), NULL, TO_CHAR);
	else
	    act("$N уже окружен$t зеркальными отражениями.",
		ch, SEX_ENDING(victim), victim, TO_CHAR);
	return FALSE;
    }

    affect_strip(victim, sn);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/5;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    act("$n окружается зеркальными отражениями.", victim, NULL, NULL, TO_ROOM);
    send_to_char("Ты окружаешься зеркальными отражениями.\n\r", victim);
    return TRUE;
}

bool spell_holy_shield(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_replace(sn, level, victim))
	affect_strip(victim, sn);

    if (IS_AFFECTED(victim, AFF_HOLY_SHIELD)
	|| IS_AFFECTED(victim, AFF_FIRE_SHIELD)
	|| IS_AFFECTED(victim, AFF_ICE_SHIELD)
	|| IS_AFFECTED(victim, AFF_CONIFEROUS_SHIELD))
    {
	if (victim == ch)
	    send_to_char("Ты и так под защитой элементального щита.\n\r", ch);
	else
	    act("$N и так под защитой элементального щита.",
		ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 5;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_HOLY_SHIELD;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    af.where = TO_RESIST;
    af.location = APPLY_NONE;
    af.modifier = 10;
    af.bitvector = DAM_HOLY;
    affect_to_char(victim, &af);

    act("$n окружается аурой света.", victim, NULL, NULL, TO_ROOM);
    send_to_char("Ты окружаешься аурой света.\n\r", victim);
    return TRUE;
}

bool spell_evil_aura(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (!is_replace(sn, level, victim)) 
    {
	if (victim == ch)
	    send_to_char("Вокруг тебя уже витает аура зла.\n\r", ch);
	else
	    act("$N и так уже окружен аурой зла.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    if (!IS_EVIL(victim))
    {
	send_to_char("Нет, здесь аура зла не будет держаться.\n\r", ch);
	return FALSE;
    }

    affect_strip(victim, sn);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/4;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_EVIL_AURA;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    af.where = TO_RESIST;
    af.location = APPLY_NONE;
    af.modifier = 10;
    af.bitvector = DAM_HOLY;
    affect_to_char(victim, &af);

    act("$n окружается аурой зла.", victim, NULL, NULL, TO_ROOM);
    send_to_char("Ты окружаешься аурой зла.\n\r", victim);
    return TRUE;
}

bool spell_identify_corpse(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj;
    char buf[MAX_STRING_LENGTH];
    int t;

    if ((obj = get_obj_list(ch, target_name, ch->in_room->contents)) == NULL)
    {
	send_to_char("Ты не видишь этого здесь!\n\r", ch);
	return FALSE;
    }

    if (obj->item_type != ITEM_CORPSE_PC && obj->item_type != ITEM_CORPSE_NPC)
    {
	send_to_char("Но это же не труп!\n\r", ch);
	return FALSE;
    }

    t = obj->when_killed - obj->timer;

    sprintf(buf, "%s\n\rУбийца: %s. Труп лежит здесь %s %d %s.\n\r"
	    "Уровень трупа: %d. Вес трупа: %d.\n\r",
	    obj->description, obj->who_kill,
	    t < 3 ? "всего" : "уже", t, hours(t, TYPE_HOURS),
	    obj->level, get_obj_weight(obj)/10);

    send_to_char(buf, ch);
    return TRUE;
}

bool spell_preserve(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj;

    if ((obj = get_eq_char(ch, WEAR_HOLD)) == NULL)
    {
	send_to_char("Ты ничего не держишь в руке.\n\r", ch);
	return FALSE;
    }

    if (obj->pIndexData->vnum != OBJ_VNUM_SEVERED_HEAD
	&& obj->pIndexData->vnum != OBJ_VNUM_TORN_HEART
	&& obj->pIndexData->vnum != OBJ_VNUM_SLICED_ARM
	&& obj->pIndexData->vnum != OBJ_VNUM_SLICED_LEG
	&& obj->pIndexData->vnum != OBJ_VNUM_GUTS
	&& obj->pIndexData->vnum != OBJ_VNUM_BRAINS
	&& obj->pIndexData->vnum != OBJ_VNUM_MEAT
	&& obj->pIndexData->vnum != OBJ_VNUM_TAIL
	&& obj->pIndexData->vnum != OBJ_VNUM_EAR)
    {
	send_to_char("Ты должен держать в руке кусок мяса!\n\r", ch);
	return FALSE;
    }

    act("Ты консервируешь $p6.", ch, obj, NULL, TO_CHAR);
    obj->timer += level;
    return TRUE;
}

bool spell_restore_mana(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    victim->mana = UMIN(victim->mana + 2 * level, victim->max_mana);
    send_to_char("Ты чувствуешь, как тебя пронизывает тепло.\n\r", victim);
    if (ch != victim)
	send_to_char("Ok.\n\r", ch);
    return TRUE;
}

bool spell_sober(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (IS_NPC(victim))
    {
	send_to_char("А разве мобы могут быть пьяницами?\n\r", victim);
	return FALSE;
    }

    if (!IS_DRUNK(victim))
    {
	act("Ты, вроде, не пьян$t.", victim, SEX_ENDING(victim), NULL, TO_CHAR);

	if (ch != victim)
	    act("$M, вроде, не пьян$t.", ch, SEX_ENDING(victim), victim, TO_CHAR);

	return FALSE;
    }

    gain_condition(victim, COND_DRUNK,  -level);

    if (ch != victim)
	send_to_char("Ok.\n\r", ch);

    return TRUE;
}

bool spell_ask_nature( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    char buf[MAX_INPUT_LENGTH];
    BUFFER *buffer;
    OBJ_DATA *obj;
    OBJ_DATA *in_obj;
    bool found;
    int number = 0, max_found;

    found = FALSE;
    number = 0;
    max_found = 2 * level;

    send_to_char( "Ты вопрошаешь природу...\n\r", ch );

    if (!IS_IN_WILD(ch))
	send_to_char( "Природа не может тебе ответить, ибо ты далек от нее...\n\r", ch);
    else
    {
	buffer = new_buf();

	for ( obj = object_list; obj != NULL; obj = obj->next )
	{
	    if ( !can_see_obj( ch, obj ) || !is_name( target_name, obj->name )
		 ||   IS_OBJ_STAT(obj, ITEM_NOLOCATE) || number_percent() > 2 * level
		 ||   ch->level < obj->level)
		continue;

	    for ( in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj )
		;

	    if ( in_obj->carried_by != NULL && can_see(ch, in_obj->carried_by))
	    {
		if (!IS_IN_WILD(in_obj->carried_by) || is_lycanthrope(in_obj->carried_by))
		    continue;

		sprintf( buf, "Это у %s в месте с названием '%s'\n\r", PERS(in_obj->carried_by, ch, 1),
			 in_obj->carried_by->in_room ? in_obj->carried_by->in_room->name : "где-то");
	    }
	    else if ( in_obj->in_room != NULL)
	    {
		if (in_obj->in_room->sector_type != SECT_FOREST
		 && in_obj->in_room->sector_type != SECT_FIELD
	         && in_obj->in_room->sector_type != SECT_HILLS
		 && in_obj->in_room->sector_type != SECT_MOUNTAIN
		 && in_obj->in_room->sector_type != SECT_DESERT)
		     continue;

		sprintf( buf, "Это в месте с названием '%s'\n\r", in_obj->in_room->name);
	    }
	    else
		continue;

	    buf[0] = UPPER(buf[0]);
	    add_buf(buffer,buf);

	    found = TRUE;
	    number++;

	    if (number >= max_found)
		break;
	}

	if ( !found )
	    send_to_char( "Ничто в природе не слышало об этом...\n\r", ch );
	else
	    page_to_char(buf_string(buffer),ch);

	free_buf(buffer);
    }

    return TRUE;
}

bool spell_forest_tport( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    ROOM_INDEX_DATA *pRoomIndex;
    int i;

    if (ch->in_room == NULL
	|| is_affected(ch, gsn_gods_curse)
	|| IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL))
    {
	send_to_char( "Отсюда не вырваться.\n\r", ch);
	return FALSE;
    }


    for (i = 0; i < level/3; i++)
    {
	if ((pRoomIndex = get_random_room(ch)) == NULL
	    || IS_SET(pRoomIndex->room_flags, ROOM_HOUSE)
	    || (pRoomIndex->clan != 0 && pRoomIndex->clan != is_clan(ch))
	    || (pRoomIndex->sector_type == SECT_CITY || pRoomIndex->sector_type == SECT_INSIDE))
	    continue;

	check_recall(ch, "Ты делаешь переброс из драки");

	act("$n исчезает!", ch, NULL, NULL, TO_ROOM);
	act("$N внезапно возникает здесь из ниоткуда.",
	    LIST_FIRST(&pRoomIndex->people), NULL, ch, TO_ALL);
	char_from_room(ch);
	char_to_room(ch, pRoomIndex, TRUE);
	check_trap(ch);
	return TRUE;
    }

    send_to_char("Природа молчит к твоим взываниям.\n\r",ch);
    return TRUE;
}

bool spell_st_aura(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_replace(sn, level, victim))
	affect_strip(victim, sn);

    if (is_affected(victim, gsn_st_aura))
    {
	send_to_char("Ты уже окружен святой аурой.\n\r", victim);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = gsn_st_aura;
    af.level     = ch->level;
    af.duration  = level / 10;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    act("Боги окружают тебя святой аурой.{x",
	victim, SEX_END_ADJ(victim), NULL, TO_CHAR);
    if (ch != victim)
	act("Боги окружают $N1 святой аурой.{x", ch, NULL, victim, TO_CHAR);
    return TRUE;
}

bool spell_bless_forest(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    AFFECT_DATA af;
    char buf[MAX_STRING_LENGTH];

    /* character target */
    victim = (CHAR_DATA *) vo;

    if (check_druid_spell(ch, victim))
    {
		send_to_char("Ты не сможешь даровать этому существу благословение леса.\n\r", ch);
		return FALSE;
    }

    if (!is_replace(sn, level, victim))
    {
		if (victim == ch)
	    	send_to_char("Ты уже под действием благословения леса.\n\r", ch);
		else
	    	act("$N уже под покровительством леса.", ch, NULL, victim, TO_CHAR);
		return FALSE;
    }

    affect_strip(victim, sn);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level   = level;
    af.duration  = 6+level;
    af.location  = APPLY_HITROLL;
    af.modifier  = level / 8;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level   = level;
    af.duration  = 6+level;
    af.location  = APPLY_DAMROLL;
    af.modifier  = level / 8;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = 0 - level / 8;
    affect_to_char(victim, &af);
    sprintf(buf, "Ты ощущаешь на себе покровительство леса.\n\r{x");
    send_to_char(buf, victim);

    if (ch != victim)
	act("Ты даешь $N2 покровительство леса.{x",
	    ch, NULL, victim, TO_CHAR);
    return TRUE;
}

bool spell_diagnosis(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    AFFECT_DATA *paf, *paf_last = NULL;
    char buf[MAX_STRING_LENGTH];

    /* character target */
    victim = (CHAR_DATA *) vo;

    if (victim == ch)
    {
	send_to_char("Набери {yэффекты{x и узнай свой диагноз.\n\r", ch);
	return FALSE;
    }

    if (victim->affected != NULL)
    {
	send_to_char("На твоей цели следующие эффекты и заклинания:\n\r", ch);
	for (paf = victim->affected; paf != NULL; paf = paf->next)
	{
	    if (paf_last != NULL && paf->type == paf_last->type)
		if (ch->level >= 20)
		    strcpy(buf, "                                ");
		else
		    continue;
	    else
		sprintf(buf, "Заклинания: %-20s", get_skill_name(victim, paf->type, TRUE));

	    send_to_char(buf, ch);

	    if (ch->level >= 20 || paf->type == gsn_gods_curse || paf->type == gsn_nocast)
	    {
		if (paf->where != TO_RESIST)
		{
		    sprintf(buf, ": модифицирует %s на %d ",
			    flag_string(apply_flags, paf->location, TRUE),
			    paf->modifier);
		    send_to_char(buf, ch);
		}
		else
		{
		    sprintf(buf, ": %s сопротивляемость к %s ",
			    paf->modifier > 0 ? "улучшает" : "ухудшает",
			    cases(flag_string(dam_flags, paf->bitvector, TRUE),
				  2));
		    send_to_char(buf, ch);
		}

		if (paf->duration == -1)
		    sprintf(buf, "постоянно");
		else
		    sprintf(buf, "на %d %s.",
			    paf->duration, hours(paf->duration, TYPE_HOURS));
		send_to_char(buf, ch);
	    }

	    send_to_char("\n\r", ch);
	    paf_last = paf;
	}
    }
    else
	send_to_char("На твоей цели нет никаких заклинаний.\n\r", ch);

    return TRUE;
}

bool spell_ghostaura(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;


    if (victim != ch && victim->race != RACE_VAMPIRE)
    {
	send_to_char("Эта аура к лицу только вампирам.\n\r", ch);
	return FALSE;
    }

    if (is_replace(sn, level, victim))
	affect_strip(victim, sn);

    if (is_affected(victim, gsn_ghostaura))
    {
	if (victim == ch)
	    send_to_char("Ты уже под призрачной аурой.\n\r", ch);
	else
	    act("$N уже под призрачной аурой.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = gsn_ghostaura;
    af.level     = level;
    af.duration  = level / 10;
    af.location  = APPLY_NONE;
    af.modifier  = 1;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    act("Призрачная аура окружает $n3.{x", victim, NULL, NULL, TO_ROOM);
    send_to_char("Призрачная аура окружает твое тело.\n\r{x", victim);
    return TRUE;
}

bool spell_manaleak(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn))
    {
	send_to_char("Магическая мощь жертвы уже ослаблена.\n\r", ch);
	return FALSE;
    }

    if (saves_spell(level, victim, DAM_MENTAL))
    {
	send_to_char("Не выходит.\n\r", ch);
	return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 17;
    af.location  = APPLY_WIS;
    af.modifier  = - level / 17;
    af.bitvector = 0;

    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    send_to_char("Твоя магическая энергия улетучивается быстрей.\n\r", victim);
    act("Магическая мощь $n1 слабеет.", victim, NULL, NULL, TO_ROOM);
    return TRUE;
}

bool spell_protection_sphere(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_affected(victim, gsn_protection_sphere))
    {
	send_to_char("Ты уже окружен защитной сферой.\n\r", victim);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = ch->level;
    af.duration  = 1;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    act("Ты окружаешься сферой защитной магии.{x",victim, NULL, NULL, TO_CHAR);
    act("$n окружается сферой защитной магии.{x",victim, NULL, NULL, TO_ROOM);
    return TRUE;
}

bool spell_protection_elements(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_replace(sn, level, victim))
	affect_strip(victim, sn);

    if (is_affected(victim, gsn_protection_elements))
    {
	send_to_char("Ты уже защищен магией стихий.\n\r", victim);
	return FALSE;
    }

    af.where     = TO_RESIST;
    af.type      = sn;
    af.level     = level;
    af.duration  = 2;
    af.location  = APPLY_NONE;
    af.modifier  = 15;
    af.bitvector = DAM_ACID;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    af.bitvector = DAM_FIRE;
    affect_to_char(victim, &af);

    af.bitvector = DAM_LIGHTNING;
    affect_to_char(victim, &af);

    af.bitvector = DAM_COLD;
    affect_to_char(victim, &af);
    act("Стихии защищают тебя.{x",victim, NULL, NULL, TO_CHAR);
    act("$n защищается стихийной магией.{x",victim, NULL, NULL, TO_ROOM);
    return TRUE;
}

bool spell_absorb_thing(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *mortar, *obj;
    AFFECT_DATA *paf, af;
    int chance;
    bool absorb = FALSE;

    if (is_affected(ch,sn))
    {
	send_to_char("Ты не можешь впитать еще одну вещь.\n\r",ch);
	return FALSE;
    }

    if (((mortar = get_eq_char(ch, WEAR_HOLD)) == NULL) || (mortar->item_type != ITEM_MORTAR))
    {
	send_to_char("Тебе же необходимо держать ступу в руке.\n\r",ch);
	return FALSE;
    }

    if ((obj = mortar->contains) == NULL)
    {
	send_to_char("В ступе же нет ничего. Что ты хочешь впитать?\n\r",ch);
	return FALSE;
    }

    if (obj->next_content != NULL)
    {
	send_to_char("Ты не можешь впитать так много вещей.\n\r",ch);
	return FALSE;
    }

    if (obj->level > ch->level)
    {
	send_to_char("У тебя не хватает сил, чтобы получить свойства это вещи.\n\r",ch);
	return FALSE;
    }

    chance = URANGE(75, get_skill(ch,sn), 100);

    for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
    {
	if (paf->where == TO_OBJECT
	&& (paf->location == APPLY_HITROLL
	|| paf->location == APPLY_SAVES
	|| paf->location == APPLY_DAMROLL
	|| paf->location == APPLY_MANA
	|| paf->location == APPLY_HIT
	|| paf->location == APPLY_MOVE
	|| paf->location == APPLY_AC
	|| paf->location == APPLY_STR
	|| paf->location == APPLY_INT
	|| paf->location == APPLY_WIS
	|| paf->location == APPLY_DEX
	|| paf->location == APPLY_CON)
	&& number_percent() <= chance)
	{	
	    chance /= 1.5;

	    af.where     = TO_AFFECTS;
	    af.type      = sn;
	    af.level     = level;
	    af.duration  = level / 5;
	    af.modifier  = paf->modifier;
	    af.location  = paf->location;
	    af.bitvector = 0;
	    af.caster_id = get_id(ch);
	    affect_to_char(ch, &af);
	    
	    absorb = TRUE;
	}
    }  

    extract_obj(obj, TRUE, TRUE);

    if (absorb)
    {
	act("Ты обретаешь некоторые свойства $p1.",ch, obj, NULL, TO_CHAR);
	act("$p вспыхивает магическим свечением, и на $n опускается сияющая аура.",
	    ch, mortar, NULL, TO_ROOM);
    }
    else
    {
	act("Ты не обретаешь никаких свойств $p1.",ch, obj, NULL, TO_CHAR);
	act("$p вспыхивает магическим свечением.",ch, mortar, NULL, TO_ROOM);
    }

    return TRUE;
}

bool spell_acid_fog(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    CHAR_DATA *vch, *safe_vch;
    OBJ_DATA *obj;

    if (ch->in_room == NULL)
	return FALSE;

    if (is_affected(ch, sn))
    {
	send_to_char("Ты пока не можешь вызвать кислотный туман.\n\r", ch);
	return FALSE;
    }	

    if (IS_SET(ch->in_room->room_flags, ROOM_HOLY) || 
	IS_SET(ch->in_room->room_flags, ROOM_GUILD))
    {
	send_to_char("Ты здесь не можешь вызвать кислотный туман.\n\r", ch);
	return FALSE;
    }
    
    for (obj = ch->in_room->contents; obj; obj = obj->next_content)
    {
	if (obj->pIndexData->vnum == OBJ_VNUM_CLOUD)
	{                    
		send_to_char("Это место уже заполнено облаком.\n\r", ch);
		return FALSE;
	}
    }

    obj = create_object(get_obj_index(OBJ_VNUM_CLOUD), 0);
    obj->timer = 1 + SKY_LIGHTNING - (IS_OUTSIDE(ch) ? weather_info.sky : 0);
    obj->value[1] = obj->timer;
    obj->level = level;

    free_string(obj->name);
    obj->name = str_dup("кислотный туман");

    free_string(obj->short_descr);
    obj->short_descr = str_dup("кислотный туман");

    free_string(obj->description);
    obj->description = str_dup("Кислотный туман обволакивает тебя.");

    obj->value[0] = CLOUD_ACID;
    obj_to_room(obj, ch->in_room);

    af.where = TO_AFFECTS;;
    af.type = sn;
    af.level = level;
    af.modifier = 0;
    af.location = APPLY_NONE;;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    af.duration = 0;
    affect_to_char(ch, &af);

    LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, safe_vch)
    {
	if (IS_NPC(vch))
	{
	    if (IS_SWITCHED(vch))
		continue;

	    if (vch->pIndexData->pShop != NULL 
	    || IS_AFFECTED(vch, AFF_CHARM))
		continue;

		/* no killing healers, trainers, etc */
	    if (IS_SET(vch->act, ACT_TRAIN)
	    || IS_SET(vch->act, ACT_PRACTICE)
	    || IS_SET(vch->act, ACT_IS_HEALER)
	    || IS_SET(vch->act, ACT_IS_CHANGER)
	    || IS_SET(vch->act, ACT_BANKER)
	    || IS_SET(vch->act, ACT_QUESTER))
		continue;
	}
	check_killer(vch, ch);
	do_function(vch, &do_kill, ch->name);
    }

    send_to_char("Ты вызываешь кислотный туман.\n\r", ch);
    act("$n вызывает кислотный туман.\n\r", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool spell_flame_cloud(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    CHAR_DATA *vch, *safe_vch;
    OBJ_DATA *obj;

    if (ch->in_room == NULL)
	return FALSE;
    
    if (is_affected(ch, sn))
    {
	send_to_char("Ты пока не можешь вызвать огненное облако.\n\r", ch);
	return FALSE;
    }	

    if (IS_SET(ch->in_room->room_flags, ROOM_HOLY) || 
	IS_SET(ch->in_room->room_flags, ROOM_GUILD))
    {
	send_to_char("Ты здесь не можешь вызвать огненное облако.\n\r", ch);
	return FALSE;
    }
    
    for (obj = ch->in_room->contents; obj; obj = obj->next_content)
    {
	if (obj->pIndexData->vnum == OBJ_VNUM_CLOUD)
	{                    
		send_to_char("Это место уже заполнено облаком.\n\r", ch);
		return FALSE;
	}
    }

    obj = create_object(get_obj_index(OBJ_VNUM_CLOUD), 0);
    obj->timer = 1 + SKY_LIGHTNING - (IS_OUTSIDE(ch) ? weather_info.sky : 0);
    obj->value[1] = obj->timer;
    obj->level = level;

    free_string(obj->name);
    obj->name = str_dup("огненное облако");

    free_string(obj->short_descr);
    obj->short_descr = str_dup("огненное облако");

    free_string(obj->description);
    obj->description = str_dup("Огненное облако окружает тебя.");

    obj->value[0] = CLOUD_FIRE;
    obj_to_room(obj, ch->in_room);

    af.where = TO_AFFECTS;;
    af.type = sn;
    af.level = level;
    af.modifier = 0;
    af.location = APPLY_NONE;;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    af.duration = 0;
    affect_to_char(ch, &af);

    LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, safe_vch)
    {
	if (IS_NPC(vch))
	{
	    if (IS_SWITCHED(vch))
		continue;

	    if (vch->pIndexData->pShop != NULL 
	    || IS_AFFECTED(vch, AFF_CHARM))
		continue;

		/* no killing healers, trainers, etc */
	    if (IS_SET(vch->act, ACT_TRAIN)
	    || IS_SET(vch->act, ACT_PRACTICE)
	    || IS_SET(vch->act, ACT_IS_HEALER)
	    || IS_SET(vch->act, ACT_IS_CHANGER)
	    || IS_SET(vch->act, ACT_BANKER)
	    || IS_SET(vch->act, ACT_QUESTER))
		continue;
	}
	check_killer(vch, ch);
	do_function(vch, &do_kill, ch->name);
    }

    send_to_char("Ты вызываешь огненное облако.\n\r", ch);
    act("$n вызывает огненное облако.\n\r", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool spell_stinking_cloud(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    CHAR_DATA *vch, *safe_vch;
    OBJ_DATA *obj;

    if (ch->in_room == NULL)
	return FALSE;
    
    if (is_affected(ch, sn))
    {
	send_to_char("Ты пока не можешь вызвать зловонное облако.\n\r", ch);
	return FALSE;
    }	

    if (IS_SET(ch->in_room->room_flags, ROOM_HOLY) || 
	IS_SET(ch->in_room->room_flags, ROOM_GUILD))
    {
	send_to_char("Ты здесь не можешь вызвать зловонное облако.\n\r", ch);
	return FALSE;
    }
    
    for (obj = ch->in_room->contents; obj; obj = obj->next_content)
    {
	if (obj->pIndexData->vnum == OBJ_VNUM_CLOUD)
	{                    
		send_to_char("Это место уже заполнено облаком.\n\r", ch);
		return FALSE;
	}
    }

    obj = create_object(get_obj_index(OBJ_VNUM_CLOUD), 0);
    obj->timer = 1 + SKY_LIGHTNING - (IS_OUTSIDE(ch) ? weather_info.sky : 0);
    obj->value[1] = obj->timer;
    obj->level = level;

    free_string(obj->name);
    obj->name = str_dup("зловонное облако");

    free_string(obj->short_descr);
    obj->short_descr = str_dup("зловонное облако");

    free_string(obj->description);
    obj->description = str_dup("Зловонное облако покрывает все вокруг.");

    obj->value[0] = CLOUD_POISON;
    obj_to_room(obj, ch->in_room);

    af.where = TO_AFFECTS;;
    af.type = sn;
    af.level = level;
    af.modifier = 0;
    af.location = APPLY_NONE;;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    af.duration = 0;
    affect_to_char(ch, &af);

    LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, safe_vch)
    {
	if (IS_NPC(vch))
	{
	    if (IS_SWITCHED(vch))
		continue;

	    if (vch->pIndexData->pShop != NULL 
	    || IS_AFFECTED(vch, AFF_CHARM))
		continue;

		/* no killing healers, trainers, etc */
	    if (IS_SET(vch->act, ACT_TRAIN)
	    || IS_SET(vch->act, ACT_PRACTICE)
	    || IS_SET(vch->act, ACT_IS_HEALER)
	    || IS_SET(vch->act, ACT_IS_CHANGER)
	    || IS_SET(vch->act, ACT_BANKER)
	    || IS_SET(vch->act, ACT_QUESTER))
		continue;
	}
	check_killer(vch, ch);
	do_function(vch, &do_kill, ch->name);
    }

    send_to_char("Ты вызываешь зловонное облако.\n\r", ch);
    act("$n вызывает зловонное облако.\n\r", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool spell_slime(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    CHAR_DATA *gch, *safe_gch;

    if (ch->fighting == NULL)
    {
	send_to_char("Это заклинание применить ты можешь только в бою!\n\r",
		     ch);
	return FALSE;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level/10;
    af.location = APPLY_DEX;
    af.modifier = -level/10;
    af.bitvector = 0;
    af.caster_id = get_id(ch);

    act("{WВокруг появилась слизь!{x", ch, NULL, NULL, TO_ALL);

    LIST_FOREACH_SAFE(gch, &ch->in_room->people, room_link, safe_gch)
    {
	/* не действует на алхимика и членов его группы */
	if (is_same_group(ch, gch) || is_safe_spell(ch, gch, TRUE))
	    continue;

	/* Ибо не фиг */
	if (gch->fighting == NULL && gch->position > POS_SLEEPING)
	    set_fighting(gch, ch);

	if (is_affected(gch, sn) || saves_spell(level, gch, DAM_OTHER))
	    continue;

	act("$n увязает в слизи!", gch, NULL, NULL, TO_ROOM);
	send_to_char("Ты увязаешь в слизи...\n\r", gch);
	affect_to_char(gch, &af);
    }

    return TRUE;
}

bool spell_gritstorm(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    OBJ_DATA *obj;

    if (is_affected(victim, gsn_gritstorm))
    {
	if (victim == ch)
	    send_to_char("Вокруг тебя уже летают осколки.\n\r", ch);
	else
	    act("Вокруг $N3 уже летают осколки.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    obj = get_eq_char(ch, WEAR_HOLD);
    if (obj == NULL || obj->item_type != ITEM_POTION)
    {
	send_to_char("У тебя нет необходимого компонента для этого заклинания.\n\r", ch);
	return FALSE;
    }

    if (victim == ch || (IS_NPC(victim) && victim->pIndexData->vnum == MOB_VNUM_GOMUN))
    {
	act("$p разлетается на осколки, которые начинают кружить вокруг тебя.",
		victim, obj, NULL, TO_CHAR);
	if (victim == ch)
	    act("Вихрь осколков окружает $n3.{x", victim, NULL, NULL, TO_ROOM);
	else
	    act("Вихрь осколков окружает гомункула.{x", victim, NULL, NULL, TO_ROOM);
	extract_obj(obj, TRUE, TRUE);

	af.where     = TO_AFFECTS;
	af.type      = gsn_gritstorm;
	af.level     = (obj->level + level) / 2;
	af.duration  = level / 4;
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = 0;
	af.caster_id = get_id(ch);
	affect_to_char(victim, &af); 

	return TRUE;
    }
    send_to_char("Ты можешь окружить осколками только себя и гомункулов.\n\r", ch);
    return FALSE;
}

bool spell_energy_potion(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    CHAR_DATA *victim, *victim2;
    char arg1[MIL];
    int i, skill = get_skill(ch, sn);
    bool fight = FALSE, mes = FALSE;

    if (obj->item_type != ITEM_POTION || obj->pIndexData->vnum != OBJ_VNUM_POTION)
    {
	send_to_char("Ты можешь использовать только зелья, произведенные алхимиками.\n\r", ch);
	return FALSE;
    }

    target_name = one_argument(target_name,arg1);

    if (target_name[0] != '\0')
    {	
	victim = get_char_world(ch, target_name);
	victim2 = get_char_room(ch, NULL, target_name, TRUE);

	if (victim2 == NULL && victim != NULL && IS_IMMORTAL(victim))
	{
	    send_to_char("Определи другую цель.\n\r", ch);
	    return FALSE;
	}

	if (victim2 != NULL)
	    victim = victim2;

	if (victim == NULL)
	{
	    send_to_char("Определи другую цель.\n\r", ch);
	    return FALSE;
	}
    }
    else 
	if (ch->fighting != NULL)
	    victim = ch->fighting;
	else
	    victim = ch;

    for (i = 1; i != 5; i++)
    {
	if (obj->value[i] > 0)
	{
	    if (victim != NULL && is_affected(victim,gsn_protection_sphere))
	    {
		act("Защитная сфера $N1 блокирует твое заклинание.{x", ch, NULL, victim, TO_CHAR);
		return FALSE;
	    }

	    if (ch->position >= skill_table[obj->value[i]].minimum_position)
	    {	    
		if (skill_table[obj->value[i]].target == TAR_CHAR_OFFENSIVE 
		&& is_safe_spell(ch, victim, FALSE))
		    continue;

		if (skill_table[obj->value[i]].target != TAR_IGNORE
		&& skill_table[obj->value[i]].target != TAR_CHAR_SELF
		&& get_char_room(ch, NULL, victim->name, FALSE) == NULL)
		{
		    send_to_char("Таких здесь нет.\n\r", ch);
		    continue;
		}

		if ((*skill_table[obj->value[i]].spell_fun) (obj->value[i], level, ch, victim, TARGET_CHAR))
		    mes = TRUE;
		if (skill_table[obj->value[i]].target == TAR_CHAR_OFFENSIVE)
		    fight = TRUE;
	    }
	}
    }

    if (mes)
    {
	if (victim == ch)
	    act("Ты направляешь силу $p1 в себя.",ch, obj, victim, TO_CHAR);
	else	
	{
	    act("Ты направляешь силу $p1 в $N3.",ch, obj, victim, TO_CHAR);
	    act("$n направляет силу $p1 в тебя.",ch, obj, victim, TO_VICT);
	}	
	act("$n направляет силу $p1 в $N3.",ch, obj, victim, TO_NOTVICT);
    }
    else
    {
	act("И ничего не происходит.",ch, obj, victim, TO_CHAR);
    }

    check_offensive(0, target, ch, victim, fight);

    if (number_percent() > skill / 3)
	extract_obj(obj, TRUE, TRUE);

    return TRUE;
}

bool spell_rename_potion(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    char arg1[MIL];

    if (obj->item_type != ITEM_POTION || obj->pIndexData->vnum != OBJ_VNUM_POTION)
    {
	send_to_char("Ты можешь изменять названия только зельям, которые произвели алхимики.\n\r", ch);
	return FALSE;
    }

    if (obj->pIndexData->edited)
    {
	send_to_char("Подожди немного, попробуй изменить название позже.\n\r", ch);
	return FALSE;
    }

    target_name = one_argument(target_name, arg1);

    if (target_name[0] == '\0')
    {
	send_to_char("Какое название ты хочешь дать зелью?\n\r", ch);
	return FALSE;
    }

    if (strlen(target_name) > 35)
    {
	send_to_char("Слишком длинное название. Выбери другое.\n\r", ch);
	return FALSE;
    }

    if (str_str(target_name,"зелье") == NULL
    && str_str(target_name,"бутыль") == NULL
    && str_str(target_name,"бутылочка") == NULL)
    {
	send_to_char("Необходимо, чтобы в названии было слово зелье или бутыль, или бутылочка.\n\r", ch);
	return FALSE;
    }
    
    if (obj->enchanted)
    {
	send_to_char("Зелью уже меняли название.\n\r", ch);
	return FALSE;
    }

    if (strchr(target_name, '{')) 
    {
        send_to_char("Название не должно содержать цвет.\n\r", ch);
        return FALSE;
    }

    SET_BIT(obj->extra_flags, ITEM_MAGIC);
    act("$p мерца$r, и на ярлычке появляется название...{x", ch, obj, NULL, TO_CHAR);

    free_string(obj->name);
    obj->name = str_dup(target_name);

    free_string(obj->short_descr);
    obj->short_descr = str_dup(target_name);

    free_string(obj->description);
    sprintf(arg1,"%s %s",capitalize(target_name), 
	number_percent() > 50 ? "лежит тут.":"лежит здесь.");
    obj->description = str_dup(arg1);

    obj->enchanted = TRUE;	

    return TRUE;
}

bool spell_create_mortar(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *mortar;

    mortar = create_object(get_obj_index(OBJ_VNUM_MORTAR), 0);
    mortar->level = ch->level;

    mortar->value[0] = 50 + number_percent() * level / 20;
    mortar->value[2] = UMIN(MAX_MORTAR_ING,1 + level * 100 / number_range(700,1000));
    mortar->value[3] = 10 + number_percent() * level / 40;

    mortar->timer = number_range(50,100) * level / 10;

    free_string(mortar->owner);
    mortar->owner = str_dup(ch->name);

    free_string(mortar->person);
    mortar->person = str_dup(ch->name);

    act("Ты создаешь поток магической энергии по форме напоминающий ступу.",
	ch, mortar, NULL, TO_CHAR);
    act("$n создает поток магической энергии по форме напоминающий ступу.",
	ch, mortar, NULL, TO_ROOM);

    if (get_carry_weight(ch) < can_carry_w(ch))
	obj_to_char(mortar, ch);
    else
    {
	send_to_char("Ты не можешь удержать ступу, и она падает.\n\r",ch);
	obj_to_room(mortar, ch->in_room);
    }

    return TRUE;
}

bool spell_create_potion(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *potion, *stone;
    int skill = get_skill(ch, sn);

    potion = create_object(get_obj_index(OBJ_VNUM_POTION), 0);
    potion->level = number_range(ch->level / 2, ch->level);
    potion->value[0] = (ch->level - number_range(1,10)) * skill / 100;

    stone = get_eq_char(ch, WEAR_HOLD);
    if (stone != NULL && stone->item_type == ITEM_WARP_STONE)
    {	
	if (number_percent() < skill - 50)
	    SET_BIT(potion->extra_flags, ITEM_BURN_PROOF);

	act("Ты направляешь магическую энергию в $p3.", ch, stone, NULL, TO_CHAR);
	act("$n направляет магическую энергию в $p3.", ch, stone, NULL, TO_ROOM);
	act("$p ярко вспыхивает и превращается в бутыль.", ch, stone, NULL, TO_ALL);

	extract_obj(stone, TRUE, TRUE);
    }
    else
    {
	if (number_percent() < skill - 5)
	    SET_BIT(potion->extra_flags, ITEM_BURN_PROOF);

	act("Ты создаешь шар магической энергии, который принимает форму бутыли.",
	    ch, potion, NULL, TO_CHAR);
	act("$n создает шар магической энергии, который принимает форму бутыли.",
	    ch, potion, NULL, TO_ROOM);
	potion->timer = 50 + number_range(1, skill);
    } 

    if (get_carry_weight(ch) < can_carry_w(ch))
	obj_to_char(potion, ch);
    else
    {
	send_to_char("Ты не можешь удержать бутыль, и она падает.\n\r",ch);
	obj_to_room(potion, ch->in_room);
    }

    return TRUE;
}

bool spell_create_creature(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *golem;
    MOB_INDEX_DATA *pIndex;
    RECIPE_DATA *recipe;
    OBJ_DATA *mortar, *obj;
    AFFECT_DATA af;
    char buf[MIL];
    int chance = 0, wea = 0, mag = 0; /*переменные брони физика и другая(магия) */
    int lev = 0, s = 0, m = 0, i, r = 0, cost[MAX_MORTAR_ING];
    int skill = get_skill(ch, sn);

    if (IS_NPC(ch))
	return FALSE;

    if (((mortar = get_eq_char(ch, WEAR_HOLD)) == NULL) || (mortar->item_type != ITEM_MORTAR))
    {
	send_to_char("Тебе же необходимо держать ступу в руке.\n\r",ch);
	return FALSE;
    }

    if ((obj = mortar->contains) == NULL)
    {
	send_to_char("В ступе же нет ничего.\n\r",ch);
	return FALSE;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_NO_MOB))
    {
	send_to_char("Только не здесь!\n\r", ch);
	return FALSE;
    }

    if (is_affected(ch, sn))
    {
	send_to_char("Ты еще не набрался сил, чтобы создать еще одно существо.\n\r", ch);
	return FALSE;
    }

    if (max_count_charmed(ch))
	return FALSE;

    for (recipe = recipe_list; recipe != NULL; recipe = recipe->next)
    {
	bool next;
	lev = 0; 
	m   = 0;
	r   = 0;

	if (recipe->type < RECIPE_TANK)
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
		send_to_char("Твой уровень или знания малы, чтобы лепить из ингредиентов существо.\n\r",ch);
		return FALSE;
	    }

	    if ((pIndex = get_mob_index(MOB_VNUM_GOMUN)) == NULL)
	    {
		bugf("Check limbo.are! Mob with vnum=%d is absent.",
		     MOB_VNUM_GOMUN);
		return FALSE;
	    }

	    while (mortar->contains != NULL)
		extract_obj(mortar->contains, TRUE, TRUE);

	    chance = recipe->comp + URANGE(-5, 15 * skill / 100, 15);
	    chance += (get_curr_stat(ch, STAT_WIS) - 20) * 3;

	    if (number_percent() <= chance)
	    {
		act("В ступе появляется миниатюрное существо, ты выкрикиваешь заклинание и переворачиваешь ступу.", 
			ch, SEX_ENDING(ch), NULL, TO_CHAR);
		act("$n выкрикивает заклинание и переворачивает ступу.", 
			ch, SEX_ENDING(ch), NULL, TO_ROOM);
		act("Из ступы что-то вываливается и вырастает в огромное существо!", 
			ch, SEX_ENDING(ch), NULL, TO_ALL);

		golem = create_mobile(pIndex);
		char_to_room(golem, ch->in_room, FALSE);

		free_string(golem->name);
		sprintf(buf, "гомункул %s",recipe->name);
		golem->name = str_dup(buf);

		free_string(golem->short_descr);
		sprintf(buf, "Гомункул %s",recipe->name);
		golem->short_descr = str_dup(buf);

		free_string(golem->long_descr);
		sprintf(buf, "Гомункул %s стоит здесь.\n\r{x",recipe->name);
		golem->long_descr = str_dup(buf);

		add_follower(golem, ch);
		golem->leader = ch;
		SET_BIT(golem->affected_by, AFF_CHARM);
		golem->level   = recipe->level;
		golem->move    = golem->max_move = ch->max_move * 4 / 3;
		SET_BIT(golem->act, ACT_NOEXP);

		switch (recipe->type)
		{
		case RECIPE_TANK:
		    golem->hit     = golem->max_hit 
			= UMIN(golem->level * 80, number_range(ch->max_hit * 1.9, ch->max_hit * 2.7));
		    golem->mana    = golem->max_mana = 0;
		    golem->hitroll = GET_HITROLL(ch) * number_range(50, 70) / 100;
		    golem->damroll = GET_DAMROLL(ch) * number_range(50, 70) / 100;
		    golem->damage[DICE_NUMBER] = 4 + golem->level / 10;
		    golem->damage[DICE_TYPE] = 4 + golem->level / 10;
		    if (golem->level < 20)
			SET_BIT(golem->off_flags, OFF_RESCUE);
		    else if (golem->level < 35)
			SET_BIT(golem->off_flags, OFF_DODGE | OFF_RESCUE);
		    else 
			SET_BIT(golem->off_flags, OFF_DODGE | OFF_PARRY | OFF_RESCUE);
		    golem->resists[DAM_WEAPON]    += 40 * golem->level / LEVEL_HERO;
		    golem->resists[DAM_MAGIC]     += 20 * golem->level / LEVEL_HERO;
		    wea = 10;
		    mag = 5;
		    break;
		case RECIPE_DAM:
		    golem->hit     = golem->max_hit
			= UMIN(recipe->level * 40, number_range(ch->max_hit * 0.8, ch->max_hit * 1.0));
		    golem->mana    = golem->max_mana = ch->max_mana / 4;
		    golem->hitroll = GET_HITROLL(ch) * number_range(100, 180) / 100;
		    golem->damroll = GET_DAMROLL(ch) * number_range(100, 180) / 100;
		    golem->damage[DICE_NUMBER] = 7 + golem->level / 10;
		    golem->damage[DICE_TYPE] = 8 + golem->level / 10;
		    if (golem->level <= 20)
			SET_BIT(golem->off_flags, OFF_KICK_DIRT);
		    else if (golem->level <= 35)
			SET_BIT(golem->off_flags, OFF_KICK_DIRT | OFF_BERSERK);
		    else 
			SET_BIT(golem->off_flags, OFF_KICK_DIRT | OFF_BERSERK | OFF_BASH | OFF_DISARM);
		    golem->spec_fun = spec_lookup("spec_homunculus_damager");
		    golem->resists[DAM_WEAPON]    += 10 * golem->level / LEVEL_HERO;
		    golem->resists[DAM_MAGIC]     += 10 * golem->level / LEVEL_HERO;
		    wea = 5;
		    mag = 2;
		    break;
		case RECIPE_MAGE:
		    golem->hit     = golem->max_hit
			= UMIN(recipe->level * 40, number_range(ch->max_hit * 0.9, ch->max_hit * 1.1));
		    golem->mana    = golem->max_mana = ch->max_mana * 2;
		    golem->hitroll = GET_HITROLL(ch) * number_range(90, 130) / 100;
		    golem->damroll = GET_DAMROLL(ch) * number_range(90, 130) / 100;
		    golem->damage[DICE_NUMBER] = 3 + golem->level / 10;
		    golem->damage[DICE_TYPE] = 5 + golem->level / 10;
		    golem->spec_fun = spec_lookup("spec_homunculus_mage");
		    golem->dam_type = attack_lookup("magic");
		    wea = 5;
		    mag = 10;
		    golem->resists[DAM_MAGIC]     += 40 * golem->level / LEVEL_HERO;
		    break;
		case RECIPE_CLERIC:
		    golem->hit     = golem->max_hit
			= UMIN(recipe->level * 40, number_range(ch->max_hit * 0.9, ch->max_hit * 1.1));
		    golem->mana    = golem->max_mana = ch->max_mana * 1.5;
		    golem->hitroll = GET_HITROLL(ch) * number_range(90, 130) / 100;
		    golem->damroll = GET_DAMROLL(ch) * number_range(90, 130) / 100;
		    golem->damage[DICE_NUMBER] = 3 + golem->level / 10;
		    golem->damage[DICE_TYPE] = 5 + golem->level / 10;
		    golem->spec_fun = spec_lookup("spec_homunculus_cleric");
		    golem->dam_type = (number_percent() > 50 
			? attack_lookup("divine") : attack_lookup("drain"));
		    wea = 7;
		    mag = 7;
		    golem->resists[DAM_MAGIC]     += 20 * golem->level / LEVEL_HERO;
		    golem->resists[DAM_NEGATIVE]  += 40 * golem->level / LEVEL_HERO;
		    golem->resists[DAM_HOLY]      += 40 * golem->level / LEVEL_HERO;
		    break;
		}

		golem->armor[0]    = 100 - golem->level * (wea + recipe->rlevel);
		golem->armor[1]    = 100 - golem->level * (wea + recipe->rlevel);
		golem->armor[2]    = 100 - golem->level * (wea + recipe->rlevel);
		golem->armor[3]    = 100 - golem->level * (mag + recipe->rlevel);

/*
отключаем эффект, не дающий по новой сделать очар
		af.where = TO_AFFECTS;
		af.type = sn;
		af.level = ch->level;
		af.duration = 2;
		af.modifier = 0;
		af.location = APPLY_NONE;
		af.bitvector = 0;
		af.caster_id = get_id(ch);
		affect_to_char(ch, &af);
*/

	    	return TRUE;
	    }
	    act("У тебя не получается слепить в ступе существо.", 
		ch, SEX_ENDING(ch), NULL, TO_CHAR);
	    act("У $n3 не получается слепить в ступе существо.", 
		ch, SEX_ENDING(ch), NULL, TO_ROOM);
	    return FALSE;
	}
    }
    act("Из таких ингредиентов вряд-ли получится что-либо слепить.", 
	ch, SEX_ENDING(ch), NULL, TO_CHAR);
    return FALSE;
}

bool spell_call_wolf(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *animal;
    int i;
    AFFECT_DATA af;

    if (IS_NPC(ch))
	return FALSE;

    if (!IS_IN_WILD(ch))
    {
	send_to_char("Ты не можешь сделать этого здесь.\n\r", ch);
	return FALSE;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_NO_MOB))
    {
	send_to_char("Только не здесь!\n\r", ch);
	return FALSE;
    }

    if (is_affected(ch, sn))
    {
	send_to_char("К сожалению, у тебя не хватает силы, чтобы позвать "
		     "еще одного волка.\n\r", ch);
	return FALSE;
    }

    if (IS_SET(ch->act, PLR_NOFOLLOW))
    {
	send_to_char("Ты не принимаешь последователей.\n\r", ch);
	return FALSE;
    }

    if (max_count_charmed(ch))
        return FALSE;

    animal = create_mobile(get_mob_index(MOB_VNUM_WOLF));

    if (animal == NULL)
    {
	bugf("spell_call_wolf: MOB_VNUM_WOLF is NULL");
	return FALSE;
    }

    char_to_room(animal, ch->in_room, FALSE);

    level = UMIN(40, level);

    animal->hit = animal->max_hit = UMIN(level * 40, ch->max_hit * number_range(90, 140) / 100);
    animal->move = animal->max_move = ch->max_move * 2;
    animal->saving_throw = level * 2;
    animal->hitroll = level * 2;
    animal->damroll = 30 + level * 2.5;
    animal->damage[DICE_NUMBER] = 6 + animal->level / 10;
    animal->damage[DICE_TYPE] = 6 + animal->level / 10;
    animal->level = ch->level;
    
    for (i = 0; i < MAX_STATS; i ++)
        animal->perm_stat[i] = UMIN(MAX_STAT, 11 + animal->level/4);

    animal->perm_stat[STAT_DEX] += 4;

    add_follower(animal, ch);
    animal->leader = ch;
    SET_BIT(animal->act, ACT_NOEXP);
    SET_BIT(animal->affected_by, AFF_CHARM);

    send_to_char("Ты выкрикиваешь заклинание, и на призыв прибегает волк.\n\r", ch);
    act("$n выкрикивает заклинание, и на призыв прибегает $N!", 
	ch, NULL, animal, TO_ROOM);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 4;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(ch, &af);

    return TRUE;
}

bool spell_call_bear(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *animal;
    int i;
    AFFECT_DATA af;    

    if (IS_NPC(ch))
	return FALSE;

    if (!IS_IN_WILD(ch))
    {
	send_to_char("Ты не можешь сделать этого здесь.\n\r", ch);
	return FALSE;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_NO_MOB))
    {
	send_to_char("Только не здесь!\n\r", ch);
	return FALSE;
    }

    if (is_affected(ch, sn))
    {
	send_to_char("К сожалению, у тебя не хватает силы, чтобы позвать "
		     "еще одного медведя.\n\r", ch);
	return FALSE;
    }

    if (IS_SET(ch->act, PLR_NOFOLLOW))
    {
	send_to_char("Ты не принимаешь последователей.\n\r", ch);
	return FALSE;
    }

    if (max_count_charmed(ch))
        return FALSE;

    animal = create_mobile(get_mob_index(MOB_VNUM_BEAR));

    if (animal == NULL)
    {
	bugf("spell_call_wolf: MOB_VNUM_BEAR is NULL");
	return FALSE;
    }

    char_to_room(animal, ch->in_room, FALSE);

    level = UMIN(40, level);

    animal->hit = animal->max_hit = UMIN(level * 60, ch->max_hit * number_range(120, 170) / 100);
    animal->move = animal->max_move = ch->max_move * 2;
    animal->saving_throw = level * 1.5;
    animal->hitroll = level;
    animal->damroll = 30 + level * 1.5;
    animal->damage[DICE_NUMBER] = 4 + animal->level / 10;
    animal->damage[DICE_TYPE] = 4 + animal->level / 10;
    animal->level = ch->level;
    animal->resists[DAM_BASH] = (ch->level >= 40 ? 44+4*(40 - level) : level);
    animal->resists[DAM_COLD] = (ch->level >= 40 ? 22+2*(40 - level) : level /2);
    
    for (i = 0; i < MAX_STATS; i ++)
        animal->perm_stat[i] = UMIN(MAX_STAT, 11 + animal->level/4);

    animal->perm_stat[STAT_STR] += 4;

    add_follower(animal, ch);
    animal->leader = ch;
    SET_BIT(animal->act, ACT_NOEXP);
    SET_BIT(animal->affected_by, AFF_CHARM);

    send_to_char("Ты выкрикиваешь заклинание, и на призыв прибегает медведь.\n\r", ch);
    act("$n выкрикивает заклинание, и на призыв прибегает $N!", 
	ch, NULL, animal, TO_ROOM);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 4;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(ch, &af);

    return TRUE;
}

bool spell_spirit_simbol(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj;
    OBJ_INDEX_DATA *pIndex;

    if ((pIndex = get_obj_index(OBJ_VNUM_SIMBOL)) == NULL)
    {
	bugf("simbol object (vnum = %d) not found! Check your limbo.are!", OBJ_VNUM_SIMBOL);
	return FALSE;
    }

    obj = create_object(pIndex, ch->level);
    obj_to_room(obj, ch->in_room);

    obj->timer = ch->level/5;

    send_to_char("Ты создаешь символ звериного духа!\n\r", ch);
    act("$n создает символ звериного духа!", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool spell_hair(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn))
    {
	send_to_char("Ты уже покрыт шерстью.\n\r", ch);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 2;
    af.location  = APPLY_AC;
    af.modifier  = -10 - level/4;
    af.bitvector = 0;
    af.caster_id = get_id(ch);

    affect_to_char(victim, &af);

    af.where = TO_RESIST;
    af.location = APPLY_NONE;
    af.modifier = level / 5;
    af.bitvector = DAM_COLD;
    affect_to_char(victim, &af);

    act("Тело $n1 покрывается шерстью.{x", victim, NULL, NULL, TO_ROOM);
    send_to_char("Твое тело покрывается шерстью.{x\n\r", victim);

    return TRUE;
}

bool spell_coarse_leather(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn))
    {
	send_to_char("Твоя кожа уже итак груба.\n\r", ch);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 2;
    af.location  = APPLY_AC;
    af.modifier  = -30 - level / 3;
    af.bitvector = 0;
    af.caster_id = get_id(ch);

    affect_to_char(victim, &af);

    af.where = TO_RESIST;
    af.location = APPLY_NONE;
    af.modifier = level / 5;
    af.bitvector = DAM_LIGHTNING;
    affect_to_char(victim, &af);

    act("Кожа $n1 грубеет.{x", victim, NULL, NULL, TO_ROOM);
    send_to_char("Твоя кожа грубеет.{x\n\r", victim);

    return TRUE;
}

bool spell_bear_strong(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn)
	|| get_curr_stat(victim, STAT_STR) >= get_max_stat(victim, STAT_STR)
	|| (IS_NPC(victim) && IS_SET(victim->act, ACT_UNDEAD)))

    {
	send_to_char("Твоя сила уже на максимуме!\n\r", ch);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level   = level;
    af.duration  = level / 2;
    af.location  = APPLY_STR;
    af.modifier  = 1 + (level >= 20) + (level >= 40);
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    send_to_char("Ты получаешь силу медведя!\n\r{x", victim);
    act("$n становится сильным, как медведь.{x", victim, NULL, NULL, TO_ROOM);
    return TRUE;
}

bool spell_animal_dodge(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn))
    {
	send_to_char("Ты итак уже шибко вертишься!\n\r", ch);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level   = level;
    af.duration  = level / 3;
    af.location  = APPLY_DEX;
    af.modifier  = 1 + (level >= 30);
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    send_to_char("Твои движения становятся точнее.\n\r{x", victim);
    act("Движения $n3 становятся точнее.", victim, NULL, NULL, TO_ROOM);
    return TRUE;
}

bool spell_dissolve(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_affected(victim, gsn_dissolve))
    {
	if (ch->sex == SEX_MALE)
	    send_to_char("Ты уже растворен в воздухе.\n\r", victim);
	else
	    send_to_char("Ты уже растворена в воздухе.\n\r", victim);
	return FALSE;
    }

    if (victim == ch || (IS_NPC(victim) && victim->pIndexData->vnum == MOB_VNUM_GOMUN))
    {
	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = ch->level;
	af.duration  = ch->level / 5;
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = 0;
	af.caster_id = get_id(ch);
	affect_to_char(victim, &af);
	act("Твое тело растворяется в воздухе.{x",victim, NULL, NULL, TO_CHAR);
	act("$n медленно растворяется в воздухе.{x",victim, NULL, NULL, TO_ROOM);
	return TRUE;
    }

    send_to_char("Ты можешь растворить только себя и гомункулов.\n\r", ch);
    return FALSE;
}


bool spell_earthen_whirlwind(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice(level, 12);
    dam = (dam*9)/10;

    if (saves_spell(level, victim, DAM_BASH))
	dam /= 2;
    damage(ch, victim, dam, sn, DAM_BASH, TRUE, NULL);

    return TRUE;
}


bool spell_waterfall(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice(level, 12);
    dam = (dam*9)/10;

    if (saves_spell(level, victim, DAM_DROWNING))
	dam /= 2;

    damage(ch, victim, dam, sn, DAM_DROWNING, TRUE, NULL);
    return TRUE;
}


bool spell_ray_of_light(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice(level, 12);
    dam = (dam*9)/10;

    if (saves_spell(level, victim, DAM_LIGHT))
	dam /= 2;
    damage(ch, victim, dam, sn, DAM_LIGHT, TRUE, NULL);
    return TRUE;
}


bool spell_ultrasound(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice(level, 12);
    dam = (dam*9)/10;

    if (saves_spell(level, victim, DAM_SOUND))
	dam /= 2;
    damage(ch, victim, dam, sn, DAM_SOUND, TRUE, NULL);
    return TRUE;
}


bool spell_clarification(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (!is_affected(victim, gsn_faerie_fire))
    {
	if (victim == ch)
	    printf_to_char("Ты не окружен%s розовой аурой.\n\r", ch, SEX_ENDING(ch));
	else
	    act("$N не выглядит окруженн$t розовой аурой.", ch, SEX_END_ADJ(victim), victim, TO_CHAR);
    }
    else
    {
	send_to_char("Розовая аура вокруг тебя тает.\n\r", victim);
	act("$n теряет розовую ауру вокруг себя.{x", victim, NULL, NULL, TO_ROOM);
	affect_strip(victim, gsn_faerie_fire);
    }

    if (victim == ch)
    {
    	if (!is_affected(victim, gsn_slow))
	{
		printf_to_char("Ты не замедлен%s.\n\r", ch, SEX_ENDING(ch));
	}
	else
	{
		send_to_char("Твое замедление проходит.\n\r", victim);
		act("$n снова начинает двигаться быстрее.{x", victim, NULL, NULL, TO_ROOM);
		affect_strip(victim, gsn_slow);
	}
	
    	if (!is_affected(victim, gsn_weaken))
	{
		printf_to_char("Ты не ослаблен%s.\n\r", ch, SEX_ENDING(ch));
	}
	else
	{
		send_to_char("Ты снова чувствуешь себя сильным.\n\r", victim);
		act("$n становится сильнее.{x", victim, NULL, NULL, TO_ROOM);
		affect_strip(victim, gsn_weaken);
	}
     }

    return TRUE;
}

bool spell_die_sun(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice(level, 10);
    if (saves_spell(level, victim, DAM_NEGATIVE))
		dam /= 2;
    damage(ch, victim, dam, sn, DAM_NEGATIVE, TRUE, NULL);
	send_to_char("Внезапно над твоей головой загорается и гаснет {DЧЕРНОЕ {RСОЛНЦЕ{x!\n\r", victim);
    act("$n внезапно залит$t светом вспыхнувшего чёрного солнца.{x", victim, SEX_ENDING(victim), NULL, TO_ROOM);
	
	victim->mana -= number_range(1, (victim->mana)/2);
	
    return TRUE;
}

bool spell_swarm_insect(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice(level, 10);
    if (saves_spell(level, victim, DAM_POISON))
	dam /= 2;
	send_to_char("Вокруг тебя появляется огромный рой ос.\n\r{x", victim);
    act("$n облеплен$t огромным роем ос.{x", victim, SEX_ENDING(victim), NULL, TO_ROOM);
    damage(ch, victim, dam, sn, DAM_POISON, TRUE, NULL);

    if (saves_spell(level, victim, DAM_POISON)
	|| (IS_VAMPIRE(victim))
	|| (victim->race == RACE_ZOMBIE)
	|| (IS_NPC(victim) && IS_SET(victim->act, ACT_UNDEAD)))
    {
		return TRUE;
    }

	poison_effect(victim, level, dam, TARGET_CHAR, ch);
    return TRUE;
}



/* charset=cp1251 */











