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
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "interp.h"



const int16_t rev_dir[] =
{
    DIR_SOUTH, DIR_WEST, DIR_NORTH, DIR_EAST, DIR_DOWN, DIR_UP
};




/*
 SECT_INSIDE		      0
 SECT_CITY		      1
 SECT_FIELD		      2
 SECT_FOREST		      3
 SECT_HILLS		      4
 SECT_MOUNTAIN		      5
 SECT_WATER_SWIM		      6
 SECT_WATER_NOSWIM	      7
 SECT_UNUSED		      8
 SECT_AIR	              9
 SECT_DESERT		     10
 */


const int16_t movement_loss[MAX_CLASS][SECT_MAX]	=
{
    {1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6},  /* Mage */
    {1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6},  /* Cleric */
    {1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6},  /* Thief */
    {1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6},  /* Warrior */
    {1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6},  /* Necromancer */
    {1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6},  /* Paladin */
    {1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6},  /* Nazgul */
    {1, 4, 2, 1, 2, 6, 4, 1, 6, 10, 5},  /* Druid */
    {1, 3, 1, 2, 2, 5, 4, 1, 6, 10, 6},  /* Ranger */
    {1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6},  /* Vampire */
    {1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6},   /* Alchemist */
    {1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6}   /* Lycanthrope */
};

/*
 * Local functions.
 */
int	find_door	args((CHAR_DATA *ch, char *arg));
int find_door_gen(CHAR_DATA *ch, char *arg, bool is_door);
bool	has_key		args((CHAR_DATA *ch, int key));
bool 	is_in_war	args((CHAR_DATA *ch, CHAR_DATA *victim));
void    run(CHAR_DATA *ch, char *argument);

void check_recall(CHAR_DATA *ch, char *message)
{
    if (ch->fighting != NULL)
    {
	int loose = 2 * UMIN(25, UMAX(5, ch->level));
	char buf[MAX_STRING_LENGTH];

	gain_exp(ch, 0 - loose, FALSE);
	sprintf(buf, "%s! Ты теряешь %d %s опыта.\n\r",
		message, loose, hours(loose, TYPE_POINTS));
	send_to_char(buf, ch);
	stop_fighting(ch, TRUE);
	ch->pcdata->flees++;
    }
}

char *direct(unsigned char dir, bool go)
{
    static char drct[15];

    switch(dir)
    {
    case DIR_NORTH:
	strcpy(drct, go ? "на север" : "с севера");
	break;
    case DIR_EAST:
	strcpy(drct, go ? "на восток" : "с востока");
	break;
    case DIR_SOUTH:
	strcpy(drct, go ? "на юг" : "с юга");
	break;
    case DIR_WEST:
	strcpy(drct, go ? "на запад" : "с запада");
	break;
    case DIR_UP:
	strcpy(drct, go ? "вверх" : "сверху");
	break;
    case DIR_DOWN:
	strcpy(drct, go ? "вниз" : "снизу");
	break;
    default:
	drct[0] = '\0';
	break;
    }

    return drct;
}

bool check_sneak(CHAR_DATA *ch, CHAR_DATA *wch)
{
    if (IS_IMMORTAL(wch))
	return FALSE;

    /* sneaking */
    if (IS_AFFECTED(ch, AFF_SNEAK))
    {
	int chance = 0;

	if (!can_see(wch, ch))
	    return TRUE;

	chance = get_skill(ch, gsn_sneak);

	if (chance < 25) chance = 25;

	chance += get_curr_stat(ch, STAT_DEX) * 2;
	chance -= get_curr_stat(wch, STAT_INT) * 2;
	chance += ch->level - wch->level * 3/2;

	if (IS_AFFECTED(wch, AFF_DETECT_HIDDEN))
	    chance -= 10;

	if (wch->fighting != NULL)
	    chance += 30;

	if (number_percent() < chance)
	    return TRUE;
    }

    /* camouflage */
    if (IS_AFFECTED(ch, AFF_CAMOUFLAGE_MOVE) && IS_IN_WILD(ch))
    {
	int chance;

	if (!can_see(wch, ch))
	    return TRUE;

	chance = get_skill(ch, gsn_camouflage_move);
	chance += get_curr_stat(ch, STAT_DEX) * 3/2;
	chance -= get_curr_stat(wch, STAT_INT) * 2;
	chance += ch->level - wch->level * 3/2;

	if (IS_AFFECTED(wch, AFF_DETECT_CAMOUFLAGE))
	    chance -= 40;

	if (wch->fighting != NULL)
	    chance += 30;

	if (number_percent() < chance)
	    return TRUE;
    }
    return FALSE;
}


void show_to_room(char buf[MAX_STRING_LENGTH], CHAR_DATA *ch, ROOM_INDEX_DATA *room)
{
    CHAR_DATA *wch, *safe_wch;

    LIST_FOREACH_SAFE(wch, &(room->people), room_link, safe_wch)
    {
	if (wch != ch
	    && !check_sneak(ch, wch)
	    && ch->invis_level <= wch->invis_level)
	{
	    act(buf, wch, NULL, ch, TO_CHAR);
	}
    }
}



void move_char(CHAR_DATA *ch, int door, bool show, bool fRun)
{
    CHAR_DATA *fch;
    CHAR_DATA *fch_next;
    CHAR_DATA *victim2;
    ROOM_INDEX_DATA *in_room;
    ROOM_INDEX_DATA *to_room = NULL;
    EXIT_DATA *pexit;
    AFFECT_DATA *paf;
    char buf[MAX_STRING_LENGTH];
    int i;
    int flag_can_move = 0;
    bool swim = FALSE;
    SHOP_DATA *pShop = NULL;

    if (door < 0 || door > (MAX_DIR - 1))
    {
	bugf("Do_move: bad door %d.", door);
	return;
    }

    if (IS_AFFECTED(ch, AFF_ROOTS))
    {
	send_to_char("Ты не можешь двинуться с места.\n\r", ch);
	return;
    }

//Если висят сети, то движения затруднены
    if ((paf = affect_find(ch->affected, gsn_nets)) != NULL
    && (paf->duration + 1) *25 > number_percent())
    {
	send_to_char("Сети, опутывающие твои ноги, мешают двигаться.\n\r",ch);
	return;
    }

    if ((in_room = ch->in_room) == NULL)
	return;

    if (!IS_NPC(ch))
    {
	/* Uh oh, another drunk Frenchman on the loose! :) */
	if (IS_DRUNK(ch))
	{
	    if (ch->pcdata->condition[COND_DRUNK] > number_percent())
	    {
		act("Ты чувствуешь себя немножко пьяно... "
		    "и не соображаешь, куда идешь.\n\r",
		    ch, NULL, NULL, TO_CHAR);
		act("$n выглядит очень пьяно, и не соображает, куда идет.",
		    ch, NULL, NULL, TO_ROOM);
		door = number_range(0, MAX_DIR - 1);
	    }
	    else
	    {
		act("Ты чувствуешь себя слегка пьяно..\n\r",
		    ch, NULL, NULL, TO_CHAR);
		act("$n выглядит очень пьяно..", ch, NULL, NULL, TO_ROOM);
	    }
	}

    }


    /*
     * Exit trigger, if activated, bail out. Only PCs are triggered.
     */
    if (!IS_NPC(ch)
	&& (p_exit_trigger(ch, door, PRG_MPROG)
	    || p_exit_trigger(ch, door, PRG_OPROG)
	    || p_exit_trigger(ch, door, PRG_RPROG)))
    {
	return;
    }

    if ((pexit = in_room->exit[door]) == NULL
	|| (to_room = pexit->u1.to_room ) == NULL
	|| !can_see_room(ch, pexit->u1.to_room))
    {
	if (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 10)
	{
	    if (number_percent() < (10 + ch->pcdata->condition[COND_DRUNK]))
	    {
		send_to_char("Ты стукаешься головой о преграду. "
			     "Пьянь, да там же нет выхода!\n\r", ch);
		act("$n долбится головой о преграду, думая, что там есть выход.",
		    ch, NULL, NULL, TO_ROOM);

		if (!is_penalty_room(in_room))
		    damage(ch, ch, 3, 0, DAM_BASH, FALSE, NULL);
	    }
	    else
	    {
		send_to_char("Ты пытаешься пройти на несуществующий выход, "
			     "но быстро это осознаешь.\n\r", ch);
		act("$n пытается пройти на несуществующий выход, но "
		    "одумывается.", ch, NULL, NULL, TO_ROOM);
		ch->position = POS_RESTING;
	    }
	}
	else
	{
	    if (to_room != NULL)
		send_to_char("Ты не можешь войти в эту комнату.\n\r", ch);
	    else
		send_to_char("Ты не можешь пойти в этом направлении.\n\r", ch);
	}
	return;
    }

    if (pexit != NULL
	&& IS_SET(pexit->exit_info, EX_RACE)
	&& !IS_NPC(ch))
    {
	/* to_room = get_recall(ch); */
	to_room = get_room_index(pc_race_table[ch->race].recall);
    }


    if (IS_SET(pexit->exit_info, EX_CLOSED)
	&& ((!IS_AFFECTED(ch, AFF_PASS_DOOR) && !is_affected(ch, gsn_dissolve))
	    || IS_SET(pexit->exit_info, EX_NOPASS)
	    || (MOUNTED(ch) && !IS_AFFECTED(MOUNTED(ch), AFF_PASS_DOOR)))
	&& !IS_TRUSTED(ch, ANGEL))
    {
	act("$d - закрыто.", ch, NULL, pexit->keyword, TO_CHAR);
	return;
    }


    if ((fch = get_master(ch)) != NULL && ch != fch && in_room == fch->in_room)
    {
	send_to_char("Что? Ты хочешь покинуть своего любимого повелителя?\n\r",
		     ch);
	return;
    }

    if (((ch->desc && is_affected(CH(ch->desc), gsn_gods_curse)) || is_affected(ch, gsn_gods_curse))
	&& (IS_SET(to_room->room_flags, ROOM_HOLY)
	    || IS_SET(to_room->room_flags, ROOM_GUILD)
	    || IS_SET(to_room->room_flags, ROOM_PRIVATE)
	    || IS_SET(to_room->room_flags, ROOM_SOLITARY)
	    || IS_SET(to_room->room_flags, ROOM_PRIVATE)		
	    || IS_SET(to_room->room_flags, ROOM_NO_RECALL)
	    || IS_SET(to_room->room_flags, ROOM_NOGATE)
	    || IS_SET(to_room->room_flags, ROOM_HOUSE)
	    || (to_room->clan != 0)))
    {
		send_to_char("Боги отобрали у тебя право заходить в такие комнаты.\n\r", ch);
		return;
    }


    LIST_FOREACH(fch, &(to_room->people), room_link)
    {
	if (IS_NPC(fch) && IS_SET(fch->act, ACT_SENTINEL) && (pShop = fch->pIndexData->pShop) != NULL)
	    break;
    }

    if (pShop != NULL && 
    ((pShop->open_hour < pShop->close_hour 
    && pShop->open_hour <= time_info.hour && pShop->close_hour > time_info.hour) == FALSE
    && (pShop->open_hour > pShop->close_hour 
    && (pShop->open_hour <= time_info.hour || pShop->close_hour > time_info.hour)) == FALSE))
    {
	printf_to_char("На дверях магазина висит табличка: \"Магазин закрыт. Время работы %d:00 - %d:00\"\n\r", ch, 
		       pShop->open_hour, pShop->close_hour);
	return;
    }

    if (room_is_private(to_room, MOUNTED(ch)))
    {
	send_to_char("Это место сейчас занято.\n\r", ch);
	return;
    }

    if (IS_NPC(ch) && to_room->owner != NULL && to_room->owner[0] != '\0')
	return;

    if (!IS_NPC(ch))
    {
	//проверка на приглашение
	if (!is_room_owner(ch, to_room))
	{
	    for (i=0; i<MAX_GUESTS; i++)
	    {
		//проверить - есть ли среди ch->pcdata->id_whom_guests хозяин дома
		victim2 = pc_id_lookup(ch->pcdata->id_whom_guest[i]);

		if (victim2 == NULL)
		    continue;

		if (is_room_owner(victim2, to_room))
		{
		    flag_can_move = 1;
		    break;
		}
	    }
	    if (flag_can_move != 1)
	    {
		send_to_char("Это не твой дом.\n\r", ch);
		return;
	    }
	}
    }

    if (MOUNTED(ch))
    {
	if (MOUNTED(ch)->position < POS_FIGHTING)
	{
	    send_to_char("Твоя лошадь должна встать.\n\r", ch);
	    return;
	}

	if (show && MOUNTED(ch)->position == POS_FIGHTING)
	{
	    send_to_char("Но твоя лошадь сейчас сражается!\n\r", ch);
	    return;
	}

	if (!mount_success(ch, MOUNTED(ch), FALSE, FALSE)
	    || IS_SET(to_room->room_flags, ROOM_INDOORS)
	    || IS_SET(to_room->room_flags, ROOM_NO_MOB))
	{
	    send_to_char("Твоя лошадь отказывается идти в эту сторону.\n\r", ch);
	    return;
	}
    }

    if (in_room->sector_type == SECT_AIR
	|| to_room->sector_type == SECT_AIR)
    {
	if (MOUNTED(ch))
	{
	    if (!IS_AFFECTED(MOUNTED(ch), AFF_FLYING))
	    {
		send_to_char("Твоя лошадь не умеет летать.\n\r", ch);
		return;
	    }
	}
	else if (!IS_AFFECTED(ch, AFF_FLYING) && !IS_IMMORTAL(ch))
	{
	    send_to_char("Ты не летаешь.\n\r", ch);
	    return;
	}
    }


    if (in_room->sector_type == SECT_WATER_NOSWIM
	|| to_room->sector_type == SECT_WATER_NOSWIM)
    {
	if (MOUNTED(ch) && !IS_AFFECTED(MOUNTED(ch), AFF_FLYING))
	{
	    send_to_char("Ты не можешь повести свою лошадь туда.\n\r", ch);
	    return;
	}

	if (!IS_AFFECTED(ch, AFF_FLYING) && !IS_IMMORTAL(ch))
	{
	    OBJ_DATA *obj;
	    /*
	     * Look for a boat.
	     */

	    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
		if (obj->item_type == ITEM_BOAT)
		    break;

	    if (!obj)
	    {
		send_to_char("Тебе нужна лодка, чтобы продолжить движение.\n\r", ch);
		return;
	    }
	}

	swim = TRUE;
    }

    if (IS_SET(to_room->room_flags, ROOM_NO_FLY) && IS_AFFECTED(ch, AFF_FLYING))
    {
	send_to_char("Здесь нельзя летать.\n\r", ch);
	do_unfly(ch, NULL);
    }

    //мобы с недостатком мувов не могут ходить, но чтобы избежать багов, просто грубо сделал 1 шаг=3 мува.
    // Временно отключил
    //	if (IS_NPC(ch))
    //	{
    //	    if (ch->move < 3) return;
    //		else ch->move = ch->move - 3;
    //	}

    if (!IS_NPC(ch))
    {
	int move;

	move = (movement_loss[ch->classid][UMIN(SECT_MAX-1, in_room->sector_type)] + movement_loss[ch->classid][UMIN(SECT_MAX-1, to_room->sector_type)]) / 2;

	if (is_affected(ch, skill_lookup("pathfinding")))
	    move /= 2;

	if (is_affected(ch, gsn_silent_step))
	{
	    move /= 2;
	    check_improve(ch, NULL, gsn_silent_step, TRUE, 30);
	}

	/* если аффект слизь, то шаги тратятся быстрей */
	if (is_affected(ch, gsn_slime))		
	    move *= 4;

	if (!MOUNTED(ch))
	{
	    /* conditional effects */ /*сделал, что чар меньше 15-го уровня в 2 раза меньше мувов тратит*/
	    if (IS_AFFECTED(ch, AFF_FLYING) || IS_AFFECTED(ch, AFF_HASTE) || ch->level < 15)
		move /= 2;

	    /* Увеличить шаги, если замедлен, или на ногах ничего нет */
	    if (IS_AFFECTED(ch, AFF_SLOW) || get_eq_char(ch, WEAR_FEET) == NULL)
		move *= 2;

	    if (2 * get_carry_weight(ch) >= can_carry_w(ch))
		move *= (1 + (float)(get_carry_weight(ch)/(2*can_carry_w(ch))));


	    if (ch->move < move)
	    {
		send_to_char("Ты чувствуешь себя слишком устало.\n\r", ch);
		return;
	    }

	    /* Ухудшить обувь */
	    if (!IS_AFFECTED(ch, AFF_FLYING) && number_bits(6) < move)
		make_worse_condition(ch, WEAR_FEET,
				     number_bits(1) + 1, DAM_BASH);

	}
	else //if (!MOUNTED(ch))
	{
	    if (IS_AFFECTED(MOUNTED(ch), AFF_FLYING)
		|| IS_AFFECTED(MOUNTED(ch), AFF_HASTE))
	    {
		move /= 2;
	    }

	    if (IS_AFFECTED(MOUNTED(ch), AFF_SLOW))
		move *= 2;

	    if (ch->size == MOUNTED(ch)->size)
		move *= 1.2;

	    if (MOUNTED(ch)->move < move)
	    {
		send_to_char("Твоя лошадь слишком устала.\n\r", ch);
		return;
	    }
	}

	WAIT_STATE(ch, 1);

	if (!MOUNTED(ch))
	    ch->move -= move;
	else
	    MOUNTED(ch)->move -= move;

    }

    if (RIDDEN(ch) && RIDDEN(ch)->in_room == ch->in_room)
    {
	CHAR_DATA *rch;
	rch = RIDDEN(ch);

	if (!mount_success(rch, ch, FALSE, TRUE))
	{
	    act("$N выходит из под твоего контроля и убегает $t.",
		rch, direct(door, TRUE), ch, TO_CHAR);
	    if (RIDDEN(ch))
		ch = RIDDEN(ch);
	}
	else
	{
	    act("Ты приструняешь $N3.", rch, NULL, ch, TO_CHAR);
	    return;
	}
    }

/*
    //чар в лесу может получить по лбу веткой, если левитирует
    if ((in_room->sector_type == SECT_FOREST)
	&& (IS_AFFECTED(ch, AFF_FLYING))
	&& (!MOUNTED(ch))
	&& number_percent() < 5)
    {
	af.where     = TO_AFFECTS;
	af.type      = gsn_deafen;
	af.level     = ch->level;
	af.duration  = 1;
	af.location  = APPLY_HITROLL;
	af.modifier  = - 1;
	af.bitvector = 0;
	af.caster_id = 0;
	affect_join(ch, &af);

	af.duration  = 1;
	af.location  = APPLY_INT;
	af.modifier  = - 1;
	affect_join(ch, &af);

	send_to_char("{WТы ударяешься головой об ветку! Ох, ой-ой-ой...{x\n\r", ch);
	return;
    }
*/

    if (MOUNTED(ch))
    {
	if(!IS_AFFECTED(MOUNTED(ch), AFF_FLYING))
	    sprintf(buf, "$N %s на %s %s.",
		    show ? "уезжает" : swim ? "уплывает" : "убегает",
		    cases(MOUNTED(ch)->short_descr, 5),
		    direct(door, TRUE));
	else
	    sprintf(buf, "$N %s на %s %s.",
		    show ? "улетает" : swim ? "уплывает" :"убегает",
		    cases(MOUNTED(ch)->short_descr, 5),  direct(door, TRUE));
    }
    else
    {
	if (IS_DRUNK(ch))
	{
	    sprintf(buf, "$N%s %s.",
		    !show
		    ? ", сильно качаясь, убегает"
		    : (IS_AFFECTED(ch, AFF_FLYING))
		    ? ", сильно качаясь, улетает" :
		    swim ? ", сильно качаясь, уплывает"
		    : " нетвердой походкой уходит",
		    direct(door, TRUE));
	}
	else
	{
	    sprintf(buf, "$N %s %s.",
		    !show
		    ? "убегает"
		    : (IS_AFFECTED(ch, AFF_FLYING))
		    ? "улетает" :
		    (IS_NPC(ch) && !IS_NULLSTR(ch->pIndexData->msg_gone)) ?
		    ch->pIndexData->msg_gone : swim ? "уплывает" : "уходит",
		    direct(door, TRUE));
	}
    }

    if (!IS_SET(in_room->room_flags, ROOM_NOTRACK)
	&& !IS_AFFECTED(ch, AFF_CAMOUFLAGE_MOVE))
    {
	add_track(ch, door);
    }


    show_to_room(buf, ch, ch->in_room);

    if (in_room->exit[door]->trap && IS_SET(in_room->exit[door]->trap_flags, TRAP_ON_ENTER))
	check_trap_exit(ch, in_room->exit[door]);

    if (MOUNTED(ch))
    {
	if (in_room->exit[door]->trap && IS_SET(in_room->exit[door]->trap_flags, TRAP_ON_ENTER))
	    check_trap_exit(ch, in_room->exit[door]);

	char_from_room(MOUNTED(ch));
	char_to_room(MOUNTED(ch), to_room, TRUE);

	if (!IS_AFFECTED(MOUNTED(ch), AFF_FLYING))
	    sprintf(buf, "$N приезжает на %s %s.",
		    cases(MOUNTED(ch)->short_descr, 5),
		    direct(rev_dir[door], FALSE));
	else
	    sprintf(buf, "$N прилетает на %s %s.",
		    cases(MOUNTED(ch)->short_descr, 5),
		    direct(rev_dir[door], FALSE));
    }
    else if (IS_DRUNK(ch))
    {
	sprintf(buf, "$N%s %s.",
		!show
		? ", сильно качаясь, прибегает"
		: (IS_AFFECTED(ch, AFF_FLYING))
		? ", сильно качаясь, прилетает" :
		swim ? ", сильно качаясь, приплывает"
		: " нетвердой походкой приходит",
		direct(rev_dir[door], FALSE));
    }
    else
    {
	sprintf(buf, "$N %s %s.",
		!show
		? "прибегает"
		: (IS_AFFECTED(ch, AFF_FLYING))
		? "прилетает" :
		(IS_NPC(ch) && !IS_NULLSTR(ch->pIndexData->msg_arrive)) ?
		ch->pIndexData->msg_arrive : swim ? "приплывает" : "приходит",
		direct(rev_dir[door], FALSE));
    }

    show_to_room(buf, ch, to_room);

    char_from_room(ch);
    char_to_room(ch, to_room, TRUE);

    /*    do_function(ch, &do_look, "auto");*/



    if (check_trap(ch))
	return;

    if (in_room == to_room) /* no circular follows */
	return;

    LIST_FOREACH_SAFE(fch, &(in_room->people), room_link, fch_next)
    {

        if (fch->in_room != in_room)
            continue;

	if ((fch->master == ch && IS_AFFECTED(fch, AFF_CHARM)
	     && fch->position < POS_STANDING)
	    && !(IS_NPC(fch)
		 && IS_SET(to_room->room_flags, ROOM_NO_MOB)))
	{
	    do_function(fch, &do_stand, "");
	}

	if (fch->master == ch
	    && fch->position == POS_STANDING
	    && can_see_room(fch, to_room))
	{
	    if (IS_SET(ch->in_room->room_flags, ROOM_LAW)
		&& (IS_NPC(fch) && IS_SET(fch->act, ACT_AGGRESSIVE)))
	    {
		act("Ты не можешь притащить $N3 в город.",
		    ch, NULL, fch, TO_CHAR);
		act("Тебе не позволено появляться в городе.",
		    fch, NULL, NULL, TO_CHAR);
		continue;
	    }

	    if (IS_NPC(fch) &&
		(IS_SET(to_room->room_flags, ROOM_NO_MOB) || !CHECK_AREAL(fch, to_room)))
	    {
		act("$N не может пойти за тобой сюда.", ch, NULL, fch, TO_CHAR);
		continue;
	    }

	    if (!IS_NPC(fch) && fch->wait > 10)
	    {
		act("$N отстает от тебя.", ch, NULL, fch, TO_CHAR);
		send_to_char("Ты отстаешь от группы.\n\r", fch);
		continue;
	    }

	    if (fRun)
	    {
		sprintf(buf, "%s 1", dir_name[door]);

		if (MOUNTED(fch))
		    do_gallop(fch, buf);
		else
		    do_run(fch, buf);

		continue;
	    }

	    
	    act("Ты следуешь за $N4.", fch, NULL, ch, TO_CHAR);
	    move_char(fch, door, TRUE, fRun);

	    
	}
    }

    /*
     * If someone is following the char, these triggers get activated
     * for the followers before the char, but it's safer this way...
     */
    if (IS_NPC(ch) && has_trigger(ch->pIndexData->progs, TRIG_ENTRY, EXEC_ALL))
	p_percent_trigger(ch, NULL, NULL, NULL, NULL, NULL, TRIG_ENTRY);

    if (!IS_NPC(ch))
    {
	p_greet_trigger(ch, PRG_MPROG);
	p_greet_trigger(ch, PRG_OPROG);
	p_greet_trigger(ch, PRG_RPROG);
    }

    return;
}



void do_north(CHAR_DATA *ch, char *argument)
{
    move_char(ch, DIR_NORTH, TRUE, FALSE);
    return;
}



void do_east(CHAR_DATA *ch, char *argument)
{
    move_char(ch, DIR_EAST, TRUE, FALSE);
    return;
}



void do_south(CHAR_DATA *ch, char *argument)
{
    move_char(ch, DIR_SOUTH, TRUE, FALSE);
    return;
}



void do_west(CHAR_DATA *ch, char *argument)
{
    move_char(ch, DIR_WEST, TRUE, FALSE);
    return;
}



void do_up(CHAR_DATA *ch, char *argument)
{
    move_char(ch, DIR_UP, TRUE, FALSE);
    return;
}



void do_down(CHAR_DATA *ch, char *argument)
{
    move_char(ch, DIR_DOWN, TRUE, FALSE);
    return;
}

int find_exit(CHAR_DATA *ch, char *arg)
{
    return find_door_gen(ch, arg, FALSE);
}

int find_door(CHAR_DATA *ch, char *arg)
{
    return find_door_gen(ch, arg, TRUE);
}

int find_door_gen(CHAR_DATA *ch, char *arg, bool is_door)
{
    EXIT_DATA *pexit;
    int door;

    door = what_door(arg);
    if (door == -1)
    {
	for (door = 0; door <= 5; door++)
	{
	    if ((pexit = ch->in_room->exit[door]) != NULL
		&& IS_SET(pexit->exit_info, EX_ISDOOR)
		&& pexit->keyword != NULL
		&& is_name(arg, pexit->keyword))
	    {
		return door;
	    }
	}
	act("Ты не видишь '$T' здесь.", ch, NULL, arg, TO_CHAR);
	return -1;
    }

    if (!ch->in_room || (pexit = ch->in_room->exit[door]) == NULL)
    {
	act("Ты не видишь двери '$T' здесь.", ch, NULL, arg, TO_CHAR);
	return -1;
    }

    if (!IS_SET(pexit->exit_info, EX_ISDOOR) && is_door)
    {
	send_to_char("Ты не можешь этого сделать.\n\r", ch);
	return -1;
    }

    return door;
}

int find_door_room(ROOM_INDEX_DATA *room, char *arg)
{
    EXIT_DATA *pexit;
    int door;

    door = what_door(arg);
    if (door == -1)
    {
	for (door = 0; door <= 5; door++)
	{
	    if ((pexit = room->exit[door]) != NULL
		&& pexit->keyword != NULL
		&& is_name(arg, pexit->keyword))
	    {
		return door;
	    }
	}
	return -1;
    }

    if ((pexit = room->exit[door]) == NULL)
	return -1;

    return door;
}


void do_open_(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Открыть что?\n\r", ch);
	return;
    }

    if ((obj = get_obj_here(ch, NULL, arg)) != NULL)
    {
	/* open portal */
	if (obj->item_type == ITEM_PORTAL)
	{
	    if (!IS_SET(obj->value[1], EX_ISDOOR))
	    {
		send_to_char("Ты не можешь этого сделать.\n\r", ch);
		return;
	    }

	    if (!IS_SET(obj->value[1], EX_CLOSED))
	    {
		send_to_char("Это уже открыто.\n\r", ch);
		return;
	    }

	    if (IS_SET(obj->value[1], EX_LOCKED))
	    {
		send_to_char("Это заперто.\n\r", ch);
		return;
	    }

	    REMOVE_BIT(obj->value[1], EX_CLOSED);
	    act("Ты открываешь $p6.", ch, obj, NULL, TO_CHAR);
	    act("$n открывает $p6.", ch, obj, NULL, TO_ROOM);

	    if (obj->trap && IS_SET(obj->trap_flags, TRAP_ON_OPEN))
		check_trap_obj(ch, obj);

	    return;
	}

	/* 'open object' */
	if (obj->item_type != ITEM_CONTAINER && obj->item_type != ITEM_CHEST)
	{
	    send_to_char("Это не контейнер.\n\r", ch);
	    return;
	}
	if (!IS_SET(obj->value[1], CONT_CLOSED))
	{
	    send_to_char("Это уже открыто.\n\r", ch);
	    return;
	}
	if (!IS_SET(obj->value[1], CONT_CLOSEABLE))
	{
	    send_to_char("Ты не можешь это сделать.\n\r", ch);
	    return;
	}
	if (IS_SET(obj->value[1], CONT_LOCKED))
	{
	    send_to_char("Это заперто.\n\r", ch);
	    return;
	}

	if (obj->trap && IS_SET(obj->trap_flags, TRAP_ON_OPEN))
	    check_trap_obj(ch, obj);

	REMOVE_BIT(obj->value[1], CONT_CLOSED);
	act("Ты открываешь $p6.", ch, obj, NULL, TO_CHAR);
	act("$n открывает $p6.", ch, obj, NULL, TO_ROOM);
	return;
    }

    if ((door = find_door(ch, arg)) >= 0)
    {
	/* 'open door' */
	ROOM_INDEX_DATA *to_room;
	EXIT_DATA *pexit;
	EXIT_DATA *pexit_rev;

	pexit = ch->in_room->exit[door];
	if (!IS_SET(pexit->exit_info, EX_CLOSED))
	{
	    send_to_char("Это уже открыто.\n\r", ch);
	    return;
	}
	if (IS_SET(pexit->exit_info, EX_LOCKED))
	{
	    send_to_char("Это заперто.\n\r", ch);
	    return;
	}

	if (pexit->trap && IS_SET(pexit->trap_flags, TRAP_ON_OPEN))
	    check_trap_exit(ch, pexit);

	REMOVE_BIT(pexit->exit_info, EX_CLOSED);
	act("$n открывает $d6.", ch, NULL, pexit->keyword, TO_ROOM);
	send_to_char("Ok.\n\r", ch);

	/* open the other side */
	if ((to_room   = pexit->u1.to_room          ) != NULL
	    && (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
	    && pexit_rev->u1.to_room == ch->in_room)
	{
	    CHAR_DATA *rch, *safe_rch;

	    REMOVE_BIT(pexit_rev->exit_info, EX_CLOSED);
	    LIST_FOREACH_SAFE(rch, &(to_room->people), room_link, safe_rch)
	    {
		act("Кто-то открывает $d6 с противоположной стороны.",
		    rch, NULL, pexit_rev->keyword, TO_CHAR);
	    }
	}
	return;
    }

    return;
}



void do_close_(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Закрыть что?\n\r", ch);
	return;
    }

    if ((obj = get_obj_here(ch, NULL, arg)) != NULL)
    {
	/* portal stuff */
	if (obj->item_type == ITEM_PORTAL)
	{

	    if (!IS_SET(obj->value[1], EX_ISDOOR)
		|| IS_SET(obj->value[1], EX_NOCLOSE))
	    {
		send_to_char("Ты не можешь это сделать.\n\r", ch);
		return;
	    }

	    if (IS_SET(obj->value[1], EX_CLOSED))
	    {
		send_to_char("Это уже закрыто.\n\r", ch);
		return;
	    }

	    SET_BIT(obj->value[1], EX_CLOSED);
	    act("Ты закрываешь $p6.", ch, obj, NULL, TO_CHAR);
	    act("$n закрывает $p6.", ch, obj, NULL, TO_ROOM);
	    return;
	}

	/* 'close object' */
	if (obj->item_type != ITEM_CONTAINER && obj->item_type != ITEM_CHEST)
	{
	    send_to_char("Это не контейнер.\n\r", ch);
	    return;
	}

	if (IS_SET(obj->value[1], CONT_CLOSED))
	{
	    send_to_char("Это уже закрыто.\n\r", ch);
	    return;
	}

	if (!IS_SET(obj->value[1], CONT_CLOSEABLE))
	{
	    send_to_char("Ты не можешь этого сделать.\n\r", ch);
	    return;
	}

	SET_BIT(obj->value[1], CONT_CLOSED);
	act("Ты закрываешь $p6.", ch, obj, NULL, TO_CHAR);
	act("$n закрывает $p6.", ch, obj, NULL, TO_ROOM);
	return;
    }

    if ((door = find_door(ch, arg)) >= 0)
    {
	/* 'close door' */
	ROOM_INDEX_DATA *to_room;
	EXIT_DATA *pexit;
	EXIT_DATA *pexit_rev;

	pexit	= ch->in_room->exit[door];
	if (IS_SET(pexit->exit_info, EX_CLOSED))
	{
	    send_to_char("Это уже закрыто.\n\r", ch);
	    return;
	}

	SET_BIT(pexit->exit_info, EX_CLOSED);
	act("$n закрывает $d6.", ch, NULL, pexit->keyword, TO_ROOM);
	send_to_char("Ok.\n\r", ch);

	/* close the other side */
	if ((to_room = pexit->u1.to_room) != NULL
	    && (pexit_rev = to_room->exit[rev_dir[door]]) != 0
	    && pexit_rev->u1.to_room == ch->in_room)
	{
	    CHAR_DATA *rch;

	    SET_BIT(pexit_rev->exit_info, EX_CLOSED);
	    if ((rch = LIST_FIRST(&to_room->people)) != NULL)
	    {
		act("Кто-то закрывает $d6 с противоположной стороны.",
		    rch, NULL, pexit_rev->keyword, TO_ALL);
	    }
	}
	return;
    }

    return;
}



bool has_key(CHAR_DATA *ch, int key)
{
    OBJ_DATA *obj;

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
	if (obj->pIndexData->vnum == key)
	    return TRUE;
    }

    return FALSE;
}



void do_lock(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Запереть что?\n\r", ch);
	return;
    }

    if (MOUNTED(ch))
    {
	send_to_char("Ты не достаешь до замка со своей лошади.\n\r", ch);
	return;
    }

    if ((obj = get_obj_here(ch, NULL, arg)) != NULL)
    {
	/* portal stuff */
	if (obj->item_type == ITEM_PORTAL)
	{
	    if (!IS_SET(obj->value[1], EX_ISDOOR)
		||  IS_SET(obj->value[1], EX_NOCLOSE))
	    {
		send_to_char("Ты не можешь этого сделать.\n\r", ch);
		return;
	    }
	    if (!IS_SET(obj->value[1], EX_CLOSED))
	    {
		send_to_char("Это не закрыто.\n\r", ch);
		return;
	    }

	    if (obj->value[4] < 0 || IS_SET(obj->value[1], EX_NOLOCK))
	    {
		send_to_char("Это не может быть отперто.\n\r", ch);
		return;
	    }

	    if (!has_key(ch, obj->value[4]))
	    {
		send_to_char("У тебя нет ключа.\n\r", ch);
		return;
	    }

	    if (IS_SET(obj->value[1], EX_LOCKED))
	    {
		send_to_char("Это уже заперто.\n\r", ch);
		return;
	    }

	    SET_BIT(obj->value[1], EX_LOCKED);
	    act("Ты запираешь $p6.", ch, obj, NULL, TO_CHAR);
	    act("$n запирает $p6.", ch, obj, NULL, TO_ROOM);
	    return;
	}

	/* 'lock object' */
	if (obj->item_type != ITEM_CONTAINER && obj->item_type != ITEM_CHEST)
	{ send_to_char("Это не контейнер.\n\r", ch); return; }
	if (!IS_SET(obj->value[1], CONT_CLOSED))
	{ send_to_char("Это не закрыто.\n\r",        ch); return; }
	if (obj->value[2] < 0)
	{ send_to_char("Это не может быть заперто.\n\r",     ch); return; }
	if (!has_key(ch, obj->value[2]))
	{ send_to_char("У тебя нет ключа.\n\r",       ch); return; }
	if (IS_SET(obj->value[1], CONT_LOCKED))
	{ send_to_char("Это уже заперто.\n\r",    ch); return; }

	SET_BIT(obj->value[1], CONT_LOCKED);
	act("Ты запираешь $p6.", ch, obj, NULL, TO_CHAR);
	act("$n запирает $p6.", ch, obj, NULL, TO_ROOM);
	return;
    }
    if ((door = find_door(ch, arg)) >= 0)
    {
	/* 'lock door' */
	ROOM_INDEX_DATA *to_room;
	EXIT_DATA *pexit;
	EXIT_DATA *pexit_rev;

	pexit	= ch->in_room->exit[door];
	if (!IS_SET(pexit->exit_info, EX_CLOSED))
	{ send_to_char("Это не закрыто.\n\r",        ch); return; }
	if (pexit->key < 0)
	{ send_to_char("Это не может быть заперто.\n\r",     ch); return; }
	if (!has_key(ch, pexit->key))
	{ send_to_char("У тебя нет ключа.\n\r",       ch); return; }
	if (IS_SET(pexit->exit_info, EX_LOCKED))
	{ send_to_char("Это уже заперто.\n\r",    ch); return; }

	SET_BIT(pexit->exit_info, EX_LOCKED);
	send_to_char("*Щелк*\n\r", ch);
	act("$n запирает $d6.", ch, NULL, pexit->keyword, TO_ROOM);

	/* lock the other side */
	if ((to_room   = pexit->u1.to_room          ) != NULL
	    &&   (pexit_rev = to_room->exit[rev_dir[door]]) != 0
	    &&   pexit_rev->u1.to_room == ch->in_room)
	{
	    SET_BIT(pexit_rev->exit_info, EX_LOCKED);
	}
	return;
    }

    return;
}



void do_unlock(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument(argument, arg);

    if (IS_NULLSTR(arg))
    {
	send_to_char("Отпереть что?\n\r", ch);
	return;
    }

    if (MOUNTED(ch))
    {
	send_to_char("Ты не достаешь до замка со своей лошади.\n\r", ch);
	return;
    }

    if ((obj = get_obj_here(ch, NULL, arg)) != NULL)
    {
	/* portal stuff */
	if (obj->item_type == ITEM_PORTAL)
	{
	    if (!IS_SET(obj->value[1], EX_ISDOOR))
	    {
		send_to_char("Ты не можешь этого сделать.\n\r", ch);
		return;
	    }

	    if (!IS_SET(obj->value[1], EX_CLOSED))
	    {
		send_to_char("Это не закрыто.\n\r", ch);
		return;
	    }

	    if (obj->value[4] < 0)
	    {
		send_to_char("Это не может быть отперто.\n\r", ch);
		return;
	    }

	    if (!has_key(ch, obj->value[4]))
	    {
		send_to_char("У тебя нет ключа.\n\r", ch);
		return;
	    }

	    if (!IS_SET(obj->value[1], EX_LOCKED))
	    {
		send_to_char("Это уже отперто.\n\r", ch);
		return;
	    }

	    if (obj->trap && IS_SET(obj->trap_flags, TRAP_ON_UNLOCK))
		check_trap_obj(ch, obj);

	    REMOVE_BIT(obj->value[1], EX_LOCKED);
	    act("Ты отпираешь $p6.", ch, obj, NULL, TO_CHAR);
	    act("$n отпирает $p6.", ch, obj, NULL, TO_ROOM);
	    return;
	}

	/* 'unlock object' */
	if (obj->item_type != ITEM_CONTAINER && obj->item_type != ITEM_CHEST)
	{
	    send_to_char("Это не контейнер.\n\r", ch);
	    return;
	}

	if (!IS_SET(obj->value[1], CONT_CLOSED))
	{
	    send_to_char("Это не закрыто.\n\r", ch);
	    return;
	}

	if (obj->value[2] < 0)
	{
	    send_to_char("Это нельзя отпереть.\n\r", ch);
	    return;
	}

	if (!has_key(ch, obj->value[2]))
	{
	    send_to_char("У тебя нет ключа.\n\r", ch);
	    return;
	}

	if (!IS_SET(obj->value[1], CONT_LOCKED))
	{
	    send_to_char("Это уже отперто.\n\r", ch);
	    return;
	}

	if (obj->trap && IS_SET(obj->trap_flags, TRAP_ON_UNLOCK))
	    check_trap_obj(ch, obj);

	REMOVE_BIT(obj->value[1], CONT_LOCKED);
	act("Ты отпираешь $p6.", ch, obj, NULL, TO_CHAR);
	act("$n отпирает $p6.", ch, obj, NULL, TO_ROOM);
	return;
    }

    if ((door = find_door(ch, arg)) >= 0)
    {
	/* 'unlock door' */
	ROOM_INDEX_DATA *to_room;
	EXIT_DATA *pexit;
	EXIT_DATA *pexit_rev;

	pexit = ch->in_room->exit[door];
	if (!IS_SET(pexit->exit_info, EX_CLOSED))
	{
	    send_to_char("Это не закрыто.\n\r", ch);
	    return;
	}

	if (pexit->key < 0)
	{
	    send_to_char("Это нельзя отпереть.\n\r", ch);
	    return;
	}

	if (!has_key(ch, pexit->key))
	{
	    send_to_char("У тебя нет ключа.\n\r", ch);
	    return;
	}

	if (!IS_SET(pexit->exit_info, EX_LOCKED))
	{
	    send_to_char("Это уже отперто.\n\r", ch);
	    return;
	}

	if (pexit->trap && IS_SET(pexit->trap_flags, TRAP_ON_UNLOCK))
	    check_trap_exit(ch, pexit);

	REMOVE_BIT(pexit->exit_info, EX_LOCKED);
	send_to_char("*Щелк*\n\r", ch);
	act("$n отпирает $d6.", ch, NULL, pexit->keyword, TO_ROOM);

	/* unlock the other side */
	if ((to_room   = pexit->u1.to_room) != NULL
	    && (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
	    && pexit_rev->u1.to_room == ch->in_room)
	{
	    REMOVE_BIT(pexit_rev->exit_info, EX_LOCKED);
	}
	return;
    }

    return;
}



void do_pick(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *gch, *safe_gch;
    OBJ_DATA *obj;
    int door;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Взломать что?\n\r", ch);
	return;
    }

    if (MOUNTED(ch))
    {
	send_to_char("Ты не можешь взламывать замки, пока находишься в седле.\n\r", ch);
	return;
    }

    WAIT_STATE(ch, skill_table[gsn_pick_lock].beats);

    /* look for guards */
    LIST_FOREACH_SAFE(gch, &(ch->in_room->people), room_link, safe_gch)
    {
	if (IS_NPC(gch) && IS_AWAKE(gch) && ch->level + 5 < gch->level)
	{
	    act("$N стоит слишком близко к замку.",
		ch, NULL, gch, TO_CHAR);
	    return;
	}
    }

    if (!IS_NPC(ch) && number_percent() > get_skill(ch, gsn_pick_lock))
    {
	send_to_char("У тебя не получается.\n\r", ch);
	check_improve(ch, NULL, gsn_pick_lock, FALSE, 2);
	return;
    }

    if ((obj = get_obj_here(ch, NULL, arg)) != NULL)
    {
	/* portal stuff */
	if (obj->item_type == ITEM_PORTAL)
	{
	    if (!IS_SET(obj->value[1], EX_ISDOOR))
	    {
		send_to_char("Ты не можешь сделать это.\n\r", ch);
		return;
	    }

	    if (!IS_SET(obj->value[1], EX_CLOSED))
	    {
		send_to_char("Это не закрыто.\n\r", ch);
		return;
	    }

	    if (obj->value[4] < 0)
	    {
		send_to_char("Это вообще нельзя отпереть.\n\r", ch);
		return;
	    }

	    if (IS_SET(obj->value[1], EX_PICKPROOF))
	    {
		send_to_char("У тебя не получается.\n\r", ch);
		return;
	    }

	    if (obj->trap && IS_SET(obj->trap_flags, TRAP_ON_PICK))
		check_trap_obj(ch, obj);

	    REMOVE_BIT(obj->value[1], EX_LOCKED);
	    act("Ты взламываешь замок на $p5.", ch, obj, NULL, TO_CHAR);
	    act("$n взламывает замок на $p5.", ch, obj, NULL, TO_ROOM);
	    check_improve(ch, NULL, gsn_pick_lock, TRUE, 2);
	    return;
	}

	/* 'pick object' */
	if (obj->item_type != ITEM_CONTAINER && obj->item_type != ITEM_CHEST)
	{
	    send_to_char("Это не контейнер.\n\r", ch);
	    return;
	}
	if (!IS_SET(obj->value[1], CONT_CLOSED))
	{
	    send_to_char("Это не закрыто.\n\r", ch);
	    return;
	}
	if (obj->value[2] < 0)
	{
	    send_to_char("Это вообще нельзя отпереть.\n\r", ch);
	    return;
	}
	if (!IS_SET(obj->value[1], CONT_LOCKED))
	{
	    send_to_char("Это уже отперто.\n\r", ch);
	    return;
	}
	if (IS_SET(obj->value[1], CONT_PICKPROOF))
	{
	    send_to_char("Не получилось.\n\r", ch);
	    return;
	}

	if (obj->trap && IS_SET(obj->trap_flags, TRAP_ON_PICK))
	    check_trap_obj(ch, obj);

	REMOVE_BIT(obj->value[1], CONT_LOCKED);
	act("Ты взламываешь замок на $p5.", ch, obj, NULL, TO_CHAR);
	act("$n взламывает замок на $p5.", ch, obj, NULL, TO_ROOM);
	check_improve(ch, NULL, gsn_pick_lock, TRUE, 2);
	return;
    }

    if ((door = find_door(ch, arg)) >= 0)
    {
	/* 'pick door' */
	ROOM_INDEX_DATA *to_room;
	EXIT_DATA *pexit;
	EXIT_DATA *pexit_rev;

	pexit = ch->in_room->exit[door];
	if (!IS_SET(pexit->exit_info, EX_CLOSED) && !IS_IMMORTAL(ch))
	{
	    send_to_char("Это не закрыто.\n\r", ch);
	    return;
	}
	if (pexit->key < 0 && !IS_IMMORTAL(ch))
	{
	    send_to_char("Это нельзя взломать.\n\r", ch);
	    return;
	}
	if (!IS_SET(pexit->exit_info, EX_LOCKED))
	{
	    send_to_char("Это уже отперто.\n\r", ch);
	    return;
	}
	if (IS_SET(pexit->exit_info, EX_PICKPROOF) && !IS_IMMORTAL(ch))
	{
	    send_to_char("Не получается.\n\r", ch);
	    return;
	}

	if (pexit->trap && IS_SET(pexit->trap_flags, TRAP_ON_PICK))
	    check_trap_exit(ch, pexit);

	REMOVE_BIT(pexit->exit_info, EX_LOCKED);
	send_to_char("*Щелк*\n\r", ch);
	act("$n взламывает $d6.", ch, NULL, pexit->keyword, TO_ROOM);
	check_improve(ch, NULL, gsn_pick_lock, TRUE, 2);

	/* pick the other side */
	if ((to_room   = pexit->u1.to_room          ) != NULL
	    && (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
	    && pexit_rev->u1.to_room == ch->in_room)
	{
	    REMOVE_BIT(pexit_rev->exit_info, EX_LOCKED);
	}
    }

    return;
}


void do_stand(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj = NULL;

    REMOVE_BIT(ch->affected_by, AFF_COVER);

    if (argument[0] != '\0')
    {
	if (ch->position == POS_FIGHTING)
	{
	    send_to_char("Может быть сначала закончить драку?\n\r", ch);
	    return;
	}
	obj = get_obj_list(ch, argument, ch->in_room->contents);
	if (obj == NULL)
	{
	    send_to_char("Ты не видишь этого здесь.\n\r", ch);
	    return;
	}
	if (obj->item_type != ITEM_FURNITURE
	    ||  (!IS_SET(obj->value[2], STAND_AT)
		 &&   !IS_SET(obj->value[2], STAND_ON)
		 &&   !IS_SET(obj->value[2], STAND_IN)))
	{
	    send_to_char("Ты не видишь места, куда встать.\n\r", ch);
	    return;
	}
	if (ch->on != obj && count_users(obj) >= obj->value[0])
	{
	    act_new("Некуда встать, на $p3, не хватает места.",
		    ch, obj, NULL, TO_CHAR, POS_DEAD);
	    return;
	}
	ch->on = obj;
	if (has_trigger(obj->pIndexData->progs, TRIG_SIT, EXEC_AFTER|EXEC_DEFAULT))
	    p_percent_trigger(NULL, obj, NULL, ch, NULL, NULL, TRIG_SIT);

    }

    switch (ch->position)
    {
    case POS_SLEEPING:
	if (IS_AFFECTED(ch, AFF_SLEEP))
	{ send_to_char("Ты не можешь проснуться!\n\r", ch); return; }

	if (obj == NULL)
	{
	    send_to_char("Ты просыпаешься и встаешь.\n\r", ch);
	    act("$n просыпается и встает.", ch, NULL, NULL, TO_ROOM);
	    ch->on = NULL;
	}
	else if (IS_SET(obj->value[2], STAND_AT))
	{
	    act_new("Ты просыпаешься и встаешь у $p1.", ch, obj, NULL, TO_CHAR, POS_DEAD);
	    act("$n просыпается и встает у $p1.", ch, obj, NULL, TO_ROOM);
	}
	else if (IS_SET(obj->value[2], STAND_ON))
	{
	    act_new("Ты просыпаешься и встаешь на $p5.", ch, obj, NULL, TO_CHAR, POS_DEAD);
	    act("$n просыпается и встает на $p5.", ch, obj, NULL, TO_ROOM);
	}
	else
	{
	    act_new("Ты просыпаешься и встаешь в $p6", ch, obj, NULL, TO_CHAR, POS_DEAD);
	    act("$n просыпается и встает в $p6", ch, obj, NULL, TO_ROOM);
	}
	ch->position = POS_STANDING;
	do_function(ch, &do_look, "auto");
	break;

    case POS_RESTING:
    case POS_SITTING:
    case POS_BASHED:
	if (obj == NULL)
	{
	    send_to_char("Ты встаешь.\n\r", ch);
	    act("$n встает.", ch, NULL, NULL, TO_ROOM);
	    ch->on = NULL;
	}
	else if (IS_SET(obj->value[2], STAND_AT))
	{
	    act("Ты стоишь у $p1", ch, obj, NULL, TO_CHAR);
	    act("$n стоит у $p1.", ch, obj, NULL, TO_ROOM);
	}
	else if (IS_SET(obj->value[2], STAND_ON))
	{
	    act("Ты стоишь на $p5.", ch, obj, NULL, TO_CHAR);
	    act("$n стоит на $p5.", ch, obj, NULL, TO_ROOM);
	}
	else
	{
	    act("Ты стоишь в $p5.", ch, obj, NULL, TO_CHAR);
	    act("$n стоит в $p5.", ch, obj, NULL, TO_ROOM);
	}

	if (ch->fighting != NULL)
	    ch->position = POS_FIGHTING;
	else
	    ch->position = POS_STANDING;
	break;

    case POS_STANDING:
	send_to_char("Ты уже стоишь.\n\r", ch);
	break;

    case POS_FIGHTING:
	send_to_char("Ты уже сражаешься!\n\r", ch);
	break;
    }

    return;
}



void do_rest(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj = NULL;

    if (MOUNTED(ch))
    {
	send_to_char("Ты не можешь отдыхать, пока находишься в седле.\n\r", ch);
	return;
    }

    if (RIDDEN(ch))
    {
	send_to_char("Ты не можешь отдохнуть, пока на тебе кто-то сидит.\n\r", ch);
	return;
    }

    if (!is_room_owner(ch, ch->in_room))
    {
	send_to_char("Ты сюда не отдыхать пришел.\n\r", ch);
	return;
    }

    if (POS_FIGHT(ch))
    {
	send_to_char("Ты уже сражаешься!\n\r", ch);
	return;
    }

    if (is_affected(ch, gsn_levitate) || IS_AFFECTED(ch, AFF_FLYING))
    {
	send_to_char("Ты не можешь сесть в полете.\n\r", ch);
	return;
    }

    /* okay, now that we know we can rest, find an object to rest on */
    if (argument[0] != '\0')
    {
	/*	    if (ch->position == POS_SLEEPING)
	 {
	 send_to_char("Ты же спишь!\n\r", ch);
	 return;
	 } */
	obj = get_obj_list(ch, argument, ch->in_room->contents);
	if (obj == NULL)
	{
	    send_to_char("Ты не видишь здесь этого.\n\r", ch);
	    return;
	}
    }
    else obj = ch->on;

    if (obj != NULL)
    {
	if (obj->item_type != ITEM_FURNITURE
	    ||  (!IS_SET(obj->value[2], REST_ON)
		 &&   !IS_SET(obj->value[2], REST_IN)
		 &&   !IS_SET(obj->value[2], REST_AT)))
	{
	    send_to_char("Ты не можешь на этом отдыхать.\n\r", ch);
	    return;
	}

	if (obj != NULL && ch->on != obj && count_users(obj) >= obj->value[0])
	{
	    act_new("Не хватает места на $p5.", ch, obj, NULL, TO_CHAR, POS_DEAD);
	    return;
	}

	ch->on = obj;
	if (has_trigger(obj->pIndexData->progs, TRIG_SIT, EXEC_AFTER|EXEC_DEFAULT))
	    p_percent_trigger(NULL, obj, NULL, ch, NULL, NULL, TRIG_SIT);

    }

    switch (ch->position)
    {
    case POS_SLEEPING:
	if (IS_AFFECTED(ch, AFF_SLEEP))
	{
	    send_to_char("Ты не можешь проснуться!\n\r", ch);
	    return;
	}

	if (obj == NULL)
	{
	    send_to_char("Ты просыпаешься и отдыхаешь.\n\r", ch);
	    act ("$n просыпается и отдыхает.", ch, NULL, NULL, TO_ROOM);
	}
	else if (IS_SET(obj->value[2], REST_AT))
	{
	    act_new("Ты просыпаешься и отдыхаешь у $p1.",
		    ch, obj, NULL, TO_CHAR, POS_SLEEPING);
	    act("$n просыпается и отдыхает у $p1.", ch, obj, NULL, TO_ROOM);
	}
	else if (IS_SET(obj->value[2], REST_ON))
	{
	    act_new("Ты просыпаешься и отдыхаешь на $p5.",
		    ch, obj, NULL, TO_CHAR, POS_SLEEPING);
	    act("$n просыпается и отдыхает на $p5.", ch, obj, NULL, TO_ROOM);
	}
	else
	{
	    act_new("Ты просыпаешься и отдыхаешь в $p5.",
		    ch, obj, NULL, TO_CHAR, POS_SLEEPING);
	    act("$n просыпается и отдыхает в $p5.", ch, obj, NULL, TO_ROOM);
	}
	ch->position = POS_RESTING;
	do_function(ch, &do_look, "auto");
	break;

    case POS_RESTING:
	send_to_char("Ты уже отдыхаешь.\n\r", ch);
	break;

    case POS_STANDING:
	if (obj == NULL)
	{
	    send_to_char("Ты садишься отдыхать.\n\r", ch);
	    act("$n садится отдыхать.", ch, NULL, NULL, TO_ROOM);
	}
	else if (IS_SET(obj->value[2], REST_AT))
	{
	    act("Ты садишься у $p1 и отдыхаешь.", ch, obj, NULL, TO_CHAR);
	    act("$n садится у $p1 и отдыхает.", ch, obj, NULL, TO_ROOM);
	}
	else if (IS_SET(obj->value[2], REST_ON))
	{
	    act("Ты садишься на $p6 и отдыхаешь.", ch, obj, NULL, TO_CHAR);
	    act("$n садится на $p6 и отдыхает.", ch, obj, NULL, TO_ROOM);
	}
	else
	{
	    act("Ты отдыхаешь в $p5.", ch, obj, NULL, TO_CHAR);
	    act("$n отдыхает в $p5.", ch, obj, NULL, TO_ROOM);
	}
	ch->position = POS_RESTING;
	break;

    case POS_SITTING:
	if (obj == NULL)
	{
	    send_to_char("Ты отдыхаешь.\n\r", ch);
	    act("$n отдыхает.", ch, NULL, NULL, TO_ROOM);
	}
	else if (IS_SET(obj->value[2], REST_AT))
	{
	    act("Ты отдыхаешь у $p1.", ch, obj, NULL, TO_CHAR);
	    act("$n отдыхает у  $p1.", ch, obj, NULL, TO_ROOM);
	}
	else if (IS_SET(obj->value[2], REST_ON))
	{
	    act("Ты отдыхаешь на $p5.", ch, obj, NULL, TO_CHAR);
	    act("$n отдыхает на $p5.", ch, obj, NULL, TO_ROOM);
	}
	else
	{
	    act("Ты отдыхаешь в $p5.", ch, obj, NULL, TO_CHAR);
	    act("$n отдыхает в $p5.", ch, obj, NULL, TO_ROOM);
	}
	ch->position = POS_RESTING;
	break;
    }


    return;
}


void do_sit (CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj = NULL;

    if (MOUNTED(ch))
    {
	send_to_char("Ты не можешь сесть, пока находишься на лошади.\n\r", ch);
	return;
    }

    if (RIDDEN(ch))
    {
	send_to_char("Ты не можешь сесть, пока на тебе кто-то сидит.\n\r", ch);
	return;
    }

    if (POS_FIGHT(ch))
    {
	send_to_char("Сначала закончи драку.\n\r", ch);
	return;
    }

    if (ch->in_room && IS_SET(ch->in_room->room_flags, ROOM_NOSIT))
    {
	send_to_char("В этом месте лучше не садиться.\n\r", ch);
	return;
    }

    if (is_affected(ch, gsn_levitate) || IS_AFFECTED(ch, AFF_FLYING))
    {
	send_to_char("Ты не можешь сесть в полете.\n\r", ch);
	return;
    }

    /* okay, now that we know we can sit, find an object to sit on */
    if (argument[0] != '\0')
    {
	obj = get_obj_list(ch, argument, ch->in_room->contents);
	if (obj == NULL)
	{
	    send_to_char("Ты не видишь этого здесь.\n\r", ch);
	    return;
	}
    }
    else obj = ch->on;

    if (obj != NULL)
    {
	if (obj->item_type != ITEM_FURNITURE
	    ||  (!IS_SET(obj->value[2], SIT_ON)
		 &&   !IS_SET(obj->value[2], SIT_IN)
		 &&   !IS_SET(obj->value[2], SIT_AT)))
	{
	    send_to_char("Ты не можешь сесть на это.\n\r", ch);
	    return;
	}

	if (obj != NULL && ch->on != obj && count_users(obj) >= obj->value[0])
	{
	    act_new("Не хватает места на $p5.", ch, obj, NULL, TO_CHAR, POS_DEAD);
	    return;
	}

	ch->on = obj;
	if (has_trigger(obj->pIndexData->progs, TRIG_SIT, EXEC_DEFAULT|EXEC_AFTER))
	    p_percent_trigger(NULL, obj, NULL, ch, NULL, NULL, TRIG_SIT);

    }
    switch (ch->position)
    {
    case POS_SLEEPING:
	if (IS_AFFECTED(ch, AFF_SLEEP))
	{
	    send_to_char("Ты не можешь проснуться!\n\r", ch);
	    return;
	}

	if (obj == NULL)
	{
	    send_to_char("Ты просыпаешься и садишься.\n\r", ch);
	    act("$n просыпается и садится.", ch, NULL, NULL, TO_ROOM);
	}
	else if (IS_SET(obj->value[2], SIT_AT))
	{
	    act_new("Ты просыпаешься и садишься у $p1.", ch, obj, NULL, TO_CHAR, POS_DEAD);
	    act("$n просыпается и садится у $p1.", ch, obj, NULL, TO_ROOM);
	}
	else if (IS_SET(obj->value[2], SIT_ON))
	{
	    act_new("Ты просыпаешься и садишься на $p6.", ch, obj, NULL, TO_CHAR, POS_DEAD);
	    act("$n просыпается и садится на $p6.", ch, obj, NULL, TO_ROOM);
	}
	else
	{
	    act_new("Ты просыпаешься и садишься в $p6.", ch, obj, NULL, TO_CHAR, POS_DEAD);
	    act("$n просыпается и садится в $p6.", ch, obj, NULL, TO_ROOM);
	}

	ch->position = POS_SITTING;

	do_function(ch, &do_look, "auto");

	break;
    case POS_RESTING:
	if (obj == NULL)
	    send_to_char("Ты перестаешь отдыхать.\n\r", ch);
	else if (IS_SET(obj->value[2], SIT_AT))
	{
	    act("Ты садишься у $p1.", ch, obj, NULL, TO_CHAR);
	    act("$n садится у $p1.", ch, obj, NULL, TO_ROOM);
	}

	else if (IS_SET(obj->value[2], SIT_ON))
	{
	    act("Ты садишься на $p6.", ch, obj, NULL, TO_CHAR);
	    act("$n садится на $p6.", ch, obj, NULL, TO_ROOM);
	}
	ch->position = POS_SITTING;
	break;
    case POS_SITTING:
	send_to_char("Ты уже сидишь.\n\r", ch);
	break;
    case POS_STANDING:
	if (obj == NULL)
	{
	    send_to_char("Ты присаживаешься.\n\r", ch);
	    act("$n садится $x.", ch, NULL, NULL, TO_ROOM);
	}
	else if (IS_SET(obj->value[2], SIT_AT))
	{
	    act("Ты садишься у $p1.", ch, obj, NULL, TO_CHAR);
	    act("$n садится у $p1.", ch, obj, NULL, TO_ROOM);
	}
	else if (IS_SET(obj->value[2], SIT_ON))
	{
	    act("Ты садишься на $p6.", ch, obj, NULL, TO_CHAR);
	    act("$n садится на $p6.", ch, obj, NULL, TO_ROOM);
	}
	else
	{
	    act("Ты садишься в $p6.", ch, obj, NULL, TO_CHAR);
	    act("$n садится в $p6.", ch, obj, NULL, TO_ROOM);
	}
	ch->position = POS_SITTING;
	break;
    }
    return;
}


void do_sleep(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj = NULL;

    if (MOUNTED(ch))
    {
	send_to_char("Ты не можешь уснуть на лошади.\n\r", ch);
	return;
    }

    if (RIDDEN(ch))
    {
	send_to_char("Ты не можешь спать, пока на тебе кто-то сидит.\n\r", ch);
	return;
    }

    if (!is_room_owner(ch, ch->in_room))
    {
	send_to_char("Ты сюда не спать пришел.\n\r", ch);
	return;
    }

    if (ch->fighting != NULL)
    {
	send_to_char("Спать во время боя?!\n\r", ch);
	return;
    }
    
    if (!IS_NPC(ch) && IS_SET(ch->act, PLR_EXCITED))
    {
	char buf[MAX_INPUT_LENGTH];

	sprintf(buf, "Ты слишком взволнован%s предыдущей дракой!\n\r", SEX_ENDING(ch));
	send_to_char(buf, ch);
	return;
    }

    if (is_affected(ch, gsn_levitate) || IS_AFFECTED(ch, AFF_FLYING))
    {
	send_to_char("Ты не можешь уснуть в полете.\n\r", ch);
	return;
    }

    if (ch->in_room && IS_SET(ch->in_room->room_flags, ROOM_NOSLEEP))
    {
	send_to_char("Не стоит спать в этом месте.\n\r", ch);
	return;
    }

    switch (ch->position)
    {
    case POS_SLEEPING:
	send_to_char("Ты уже спишь.\n\r", ch);
	break;

    case POS_RESTING:
    case POS_SITTING:
    case POS_STANDING:
	if (argument[0] == '\0' && ch->on == NULL)
	{
	    send_to_char("Ты ложишься спать.\n\r", ch);
	    act("$n ложится спать.", ch, NULL, NULL, TO_ROOM);
	    ch->position = POS_SLEEPING;
	}
	else  /* find an object and sleep on it */
	{
	    if (argument[0] == '\0')
		obj = ch->on;
	    else
		obj = get_obj_list(ch, argument,  ch->in_room->contents);

	    if (obj == NULL)
	    {
		send_to_char("Ты не видишь этого здесь.\n\r", ch);
		return;
	    }
	    if (obj->item_type != ITEM_FURNITURE
		||  (!IS_SET(obj->value[2], SLEEP_ON)
		     &&   !IS_SET(obj->value[2], SLEEP_IN)
		     &&	 !IS_SET(obj->value[2], SLEEP_AT)))
	    {
		send_to_char("Ты не можешь на этом спать!\n\r", ch);
		return;
	    }

	    if (ch->on != obj && count_users(obj) >= obj->value[0])
	    {
		act_new("Для тебя не хватает места на $p5.",
			ch, obj, NULL, TO_CHAR, POS_DEAD);
		return;
	    }

	    ch->on = obj;
	    if (has_trigger(obj->pIndexData->progs, TRIG_SIT, EXEC_AFTER|EXEC_DEFAULT))
		p_percent_trigger(NULL, obj, NULL, ch, NULL, NULL, TRIG_SIT);

	    if (IS_SET(obj->value[2], SLEEP_AT))
	    {
		act("Ты ложишься спать у $p1.", ch, obj, NULL, TO_CHAR);
		act("$n ложится спать у $p1.", ch, obj, NULL, TO_ROOM);
	    }
	    else if (IS_SET(obj->value[2], SLEEP_ON))
	    {
		act("Ты ложишься спать на $p6.", ch, obj, NULL, TO_CHAR);
		act("$n ложится спать на $p6.", ch, obj, NULL, TO_ROOM);
	    }
	    else
	    {
		act("Ты ложишься спать в $p6.", ch, obj, NULL, TO_CHAR);
		act("$n ложится спать в $p6.", ch, obj, NULL, TO_ROOM);
	    }
	    ch->position = POS_SLEEPING;
	}
	break;

    case POS_FIGHTING:
    case POS_BASHED:
	send_to_char("Но ты же сражаешься!\n\r", ch);
	break;
    }

    return;
}



void do_wake(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    one_argument(argument, arg);
    if (arg[0] == '\0')
    { do_function(ch, &do_stand, ""); return; }

    if (!IS_AWAKE(ch))
    { send_to_char("Ты что там, спишь?!\n\r",       ch); return; }

    if ((victim = get_char_room(ch, NULL, arg, FALSE)) == NULL)
    { send_to_char("Таких здесь нет.\n\r",              ch); return; }

    if (IS_AWAKE(victim))
    { act("$N уже не спит.", ch, NULL, victim, TO_CHAR); return; }

    if (IS_AFFECTED(victim, AFF_SLEEP))
    { act("Ты не можешь разбудить $S!",   ch, NULL, victim, TO_CHAR);  return; }

    act_new("$n будит тебя.", ch, NULL, victim, TO_VICT, POS_SLEEPING);

    /* Фикс по поводу ковриков */
    if ((obj = victim->on) != NULL)
    {
	DO_FUN *fun;

	if (IS_SET(obj->value[2], STAND_ON)
	    || IS_SET(obj->value[2], STAND_AT)
	    || IS_SET(obj->value[2], STAND_IN))
	    fun = do_stand;
	else if (IS_SET(obj->value[2], REST_ON)
		 || IS_SET(obj->value[2], REST_AT)
		 || IS_SET(obj->value[2], REST_IN))
	    fun = do_rest;
	else if (IS_SET(obj->value[2], SIT_ON)
		 || IS_SET(obj->value[2], SIT_AT)
		 || IS_SET(obj->value[2], SIT_IN))
	    fun = do_sit;
	else
	    fun = NULL;

	if (fun)
	{
	    do_function(victim, fun, obj->name);
	    return;
	}
    }

    do_function(victim, &do_stand, "");
    return;
}



void do_sneak(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;

    if (MOUNTED(ch))
    {
	send_to_char("Ты не можешь красться на лошади.\n\r", ch);
	return;
    }

    //    affect_strip(ch, gsn_sneak);

    if (IS_AFFECTED(ch, AFF_SNEAK) || is_affected(ch, gsn_sneak))
    {
	send_to_char("Ты уже и так тихонько крадешься.\n\r", ch);
	return;
    }

    if (number_percent() <= get_skill(ch, gsn_sneak))
    {
	check_improve(ch, NULL, gsn_sneak, TRUE, 3);
	af.where     = TO_AFFECTS;
	af.type      = gsn_sneak;
	af.level     = ch->level;
	af.duration  = number_range(ch->level/3, ch->level);
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = AFF_SNEAK;
	af.caster_id = 0;
	affect_to_char(ch, &af);
	send_to_char("Ты начинаешь двигаться тихо-тихо.\n\r", ch);
    }
    else
    {
	send_to_char("Ты пытаешься двигаться очень тихо.\n\r", ch);
	check_improve(ch, NULL, gsn_sneak, FALSE, 3);
    }

    return;
}



void do_hide(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;

    if (MOUNTED(ch))
    {
	send_to_char("Ты не можешь скрыться на лошади.\n\r", ch);
	return;
    }

    if (RIDDEN(ch))
    {
	send_to_char("Ты не можешь скрыться, когда тебя кто-то оседлал.\n\r", ch);
	return;
    }

    if (IS_AFFECTED(ch, AFF_FAERIE_FIRE))
    {
	send_to_char("Ты не можешь скрыться, пока от тебя исходит свечение.\n\r", ch);
	return;
    }

    if (IS_AFFECTED(ch, AFF_HIDE) || is_affected(ch, gsn_hide))
    {
	send_to_char("Ты уже и так превосходно прячешься от чужих глаз.\n\r", ch);
	return;
    }

    //    if (!IS_NPC(ch) && IS_SET(ch->act, PLR_EXCITED))
    //    {
    //        act("Ты слишком взволнован$t, чтобы скрыться.", ch, SEX_ENDING(ch), NULL, TO_CHAR);
    //	return;
    //    }

    if (number_percent() <= get_skill(ch, gsn_hide))
    {
	af.where     = TO_AFFECTS;
	af.type      = gsn_hide;
	af.level     = ch->level;
	af.duration  = number_range(ch->level/3, ch->level);
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = AFF_HIDE;
	af.caster_id = 0;
	affect_to_char(ch, &af);
	check_improve(ch, NULL, gsn_hide, TRUE, 3);
	act("Ты уходишь в тень.",ch, NULL, NULL, TO_CHAR);
	act("$n уходит в тень.", ch, NULL, NULL, TO_NOTVICT);
    }
    else
    {
	send_to_char("Ты пытаешься скрыться.\n\r", ch);
	check_improve(ch, NULL, gsn_hide, FALSE, 3);
    }

    return;
}



/*
 * Contributed by Alander.
 */
void do_visible(CHAR_DATA *ch, char *argument)
{
    affect_strip (ch, gsn_invis			);
    affect_strip (ch, gsn_mass_invis			);
    affect_strip (ch, gsn_sneak			);
    affect_strip (ch, gsn_hide				);
    affect_strip (ch, gsn_camouflage			);
    REMOVE_BIT   (ch->affected_by, AFF_HIDE		);
    REMOVE_BIT   (ch->affected_by, AFF_INVISIBLE	);
    REMOVE_BIT   (ch->affected_by, AFF_SNEAK		);
    REMOVE_BIT   (ch->affected_by, AFF_CAMOUFLAGE 	);

    if (is_affected(ch,gsn_dissolve))
    {
	affect_strip (ch, gsn_dissolve);
	WAIT_STATE(ch, skill_table[gsn_dissolve].beats * 2);
    }
    if (strcmp(argument, "fight"))
    {
        REMOVE_BIT   (ch->affected_by, AFF_CAMOUFLAGE_MOVE	);
        affect_strip (ch, gsn_camouflage_move		);

        if (strcmp(argument, "steal"))
        {
            if (is_affected(ch, gsn_shapechange))
            {
	        affect_strip(ch, gsn_shapechange);
	        extract_char(ch, TRUE);
	    }
	    printf_to_char("Ты становишься видим%s для всех.\n\r", ch, SEX_END_ADJ(ch));
	}
    }

    return;
}

void do_unfly(CHAR_DATA *ch, char *argument)
{
    if (!is_affected(ch, gsn_fly) && !is_affected(ch, gsn_levitate) && !IS_AFFECTED(ch, AFF_FLYING))
    {
	send_to_char("Ты и так не летаешь.\n\r", ch);
	return;
    }

    affect_strip(ch, gsn_fly);
    affect_strip(ch, gsn_levitate);
    REMOVE_BIT(ch->affected_by, AFF_FLYING);
    send_to_char("Ты медленно опускаешься на землю.\n\r", ch);

    return;
}


void kick_pet(CHAR_DATA *pet)
{
    ROOM_INDEX_DATA *in_room = pet->in_room, *to_room;

    if (in_room && IS_SET(in_room->room_flags, ROOM_NO_MOB))
    {
	int i;
	EXIT_DATA *pexit;

	if (pet->position == POS_SLEEPING)
	    do_stand(pet, "");

	if (pet->mount)
	    do_dismount(pet->mount, "");

	for (i = 0; i < MAX_DIR;i++)
	    if ((pexit = in_room->exit[i]) != NULL
		&& (to_room = pexit->u1.to_room) != NULL
		&& can_see_room(pet, to_room)
		&& !IS_SET(to_room->room_flags, ROOM_NO_MOB))
	    {
		act("$n уходит $t.", pet, direct(i, TRUE), NULL, TO_ROOM);
		char_from_room(pet);
		char_to_room(pet, to_room, TRUE);
		act("$n приходит $t.", pet, direct(i, FALSE), NULL, TO_ROOM);
		break;
	    }
    }
}

bool move_for_recall(CHAR_DATA *victim, ROOM_INDEX_DATA *location)
{
    if (victim->position < POS_RESTING)
	return FALSE;

    act("$n исчезает.", victim, NULL, NULL, TO_ROOM);

    char_from_room(victim);

    if (LIST_FIRST(&location->people))
	act("$N внезапно появляется.",
	    LIST_FIRST(&location->people), NULL, victim, TO_ALL);

    char_to_room(victim, location, TRUE);

    return (check_trap(victim));
}


void do_recall(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA *location;
    int skill;

    if (IS_NPC(ch))
    {
	send_to_char("Ты не можешь пользоваться возвратом.\n\r", ch);
	return;
    }

    act("$n молится для отзыва!", ch, 0, 0, TO_ROOM);

    if ((location = get_recall(ch)) == NULL)
    {
	send_to_char("Ты окончательно потерялся...\n\r", ch);
	return;
    }

    if (ch->in_room == location)
	return;

    if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
	||   IS_AFFECTED(ch, AFF_CURSE)
	||   is_affected(ch, gsn_gods_curse))
    {
	send_to_char("Боги забыли про тебя.\n\r", ch);
	return;
    }

    if ((skill = get_skill(ch, gsn_recall)) < 1 || (ch->level > MAX_RECALL_LEVEL && !IS_IMMORTAL(ch)))
    {
	sprintf(buf, "Ты уже слишком больш%s, чтобы пользоваться возвратом.\n\r",
		ch->sex == SEX_MALE ? "ой" : ch->sex == SEX_FEMALE ? "ая" : "ое");
	send_to_char(buf, ch);
	return;
    }

    if (number_percent() > 80 * skill / 100)
    {
	WAIT_STATE(ch, 4);
	send_to_char("Не получается!\n\r", ch);
	return;
    }

    check_recall(ch, "Ты делаешь возврат из драки");

    ch->move -= abs(ch->move/2);

    if (move_for_recall(ch, location))
	return;

    if (!IS_NPC(ch) && ch->pet != NULL)
    {
	move_for_recall(ch->pet, location);
	kick_pet(ch->pet);
    }

    if (!IS_NPC(ch) && ch->mount != NULL)
    {
	move_for_recall(ch->mount, location);
	kick_pet(ch->mount);
    }

    return;
}

int what_door(char *arg1)
{
    if (!str_prefix(arg1, "north")
	|| !str_prefix(arg1, "север"))
    {
	return DIR_NORTH;
    }
    else if (!str_prefix(arg1, "east")
	     || !str_prefix(arg1, "восток"))
    {
	return DIR_EAST;
    }
    else if (!str_prefix(arg1, "south")
	     || !str_prefix(arg1, "юг"))
    {
	return DIR_SOUTH;
    }
    else if (!str_prefix(arg1, "west")
	     || !str_prefix(arg1, "запад"))
    {
	return DIR_WEST;
    }
    else if (!str_prefix(arg1, "up")
	     || !str_prefix(arg1, "верх")
	     || !str_prefix(arg1, "вверх"))
    {
	return DIR_UP;
    }
    else if (!str_prefix(arg1, "down")
	     || !str_prefix(arg1, "низ")
	     || !str_prefix(arg1, "вниз"))
    {
	return DIR_DOWN;
    }
    else
	return -1;
}

void do_detect_hidden(CHAR_DATA *ch, char *argument)
{
    int skill;
    AFFECT_DATA af;

    if ((skill = get_skill(ch, gsn_detect_hidden)) < 1)
    {
	send_to_char("Ты не знаешь, как это делается.\n\r", ch);
	return;
    }

    if (IS_AFFECTED(ch, AFF_DETECT_HIDDEN))
    {
	send_to_char("Твое внимание на максимуме. \n\r", ch);
	return;
    }

    WAIT_STATE(ch, skill_table[gsn_detect_hidden].beats);

    if (skill <= number_percent())
    {
	send_to_char("Ты пытаешься разглядеть незаметные движения в тени.\n\r",
		     ch);
	check_improve(ch, NULL, gsn_detect_hidden, FALSE, 4);
	return;
    }

    af.where     = TO_AFFECTS;
    af.type      = gsn_detect_hidden;
    af.level     = ch->level;
    af.duration  = ch->level;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_DETECT_HIDDEN;
    af.caster_id = 0;
    affect_to_char(ch, &af);
    send_to_char("Твоя бдительность повышается.\n\r", ch);
    check_improve(ch, NULL, gsn_detect_hidden, TRUE, 4);
    return;
}

void do_camp(CHAR_DATA *ch, char *argument)
{
    int hard = 1, mana;
    OBJ_INDEX_DATA *pIndex;
    OBJ_DATA *obj;


    if (MOUNTED(ch))
    {
	send_to_char("Ты не можешь разжечь костер с лошади.\n\r", ch);
	return;
    }

    switch (ch->in_room->sector_type)
    {
    case SECT_FOREST:
	hard = 1;
	break;
    case SECT_HILLS:
    case SECT_FIELD:
	hard = 2;
	break;
    case SECT_MOUNTAIN:
	hard = 4;
	break;
    case SECT_DESERT:
	hard = 8;
	break;
    default:
	send_to_char("Только не тут!\n\r", ch);
	return;
	break;
    }

    if (IS_OUTSIDE(ch))
    {
	switch (weather_info.sky)
	{
	case SKY_CLOUDLESS:
	    hard /= 2;
	    break;
	case SKY_CLOUDY:
	    break;
	case SKY_RAINING:
	    hard *= 2;
	    break;
	case SKY_LIGHTNING:
	    hard *= 3;
	    break;
	default:
	    break;
	}
    }

    mana = number_range(3, 8);

    if (mana > ch->mana || mana > ch->move)
    {
	send_to_char("У тебя не хватает энергии для разжигания костра.\n\r", ch);
	return;
    }

    if (hard <= 0)
	hard = 1;

    if (number_percent() >= get_skill(ch, gsn_camp)/hard)
    {
	send_to_char("Твой костер никак не хочет разгораться.\n\r", ch);
	check_improve(ch, NULL, gsn_camp, FALSE, 2);
	ch->mana -= mana/2;
	ch->move -= mana/2;
	return;

    }

    check_improve(ch, NULL, gsn_camp, TRUE, 2);

    if ((pIndex = get_obj_index(OBJ_VNUM_CAMPFIRE)) == NULL)
    {
	bugf("campfire object (vnum = %d) not found! Check your limbo.are!", OBJ_VNUM_CAMPFIRE);
	return;
    }

    /*
     * Собственно, объект "костер" (campfire) должен быть,
     * естественно, фурнитуриной, у которой можно стоять,
     * спать, сидеть, отдыхать...  8)) Модификатор должен
     * быть процентов 150 имхо...
     * В отличии от лагеря у егерей в Аладоне, это умение
     * вполне груповое. Идея спионерена с Былин...  8))
     */

    obj = create_object(pIndex, ch->level);
    obj_to_room(obj, ch->in_room);

    obj->timer = ch->level/5;

    ch->mana -= mana;
    ch->move -= mana;

    /*
     * И сообщения, сообщения, сообщения...
     */
    send_to_char("Ты разжигаешь костер, и он начинает весело потрескивать ветками!\n\r", ch);
    act("$n разжигает костер, и он начинает весело потрескивать ветками!", ch, NULL, NULL, TO_ROOM);

    WAIT_STATE(ch, skill_table[gsn_camp].beats);
}

void do_track(CHAR_DATA *ch, char *argument)
{
    TRACK_DATA *tr;
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    int mana, skill;

    if ((skill = get_skill(ch, gsn_track)) < 1)
    {
	send_to_char("Ты не знаешь как это делается.\n\r", ch);
	return;
    }

    mana = number_range(5, 25);
    if (mana > ch->mana)
    {
	send_to_char("У тебя не хватает энергии для выслеживания.\n\r", ch);
	return;
    }

    //добавил: не на дикой местности могут выслеживать только воры, на дикой только егеря и друиды
    if (!IS_IN_WILD(ch) && ch->classid != CLASS_THIEF)
    {
	send_to_char("Искать следы в городе? А ну-ка, беги в свой лес!\n\r", ch);
	return;
    }

    if (IS_IN_WILD(ch) && (ch->classid != CLASS_RANGER && ch->classid != CLASS_DRUID && ch->classid != CLASS_LYCANTHROPE))
    {
	send_to_char("Искать следы в лесу? А ну-ка, беги в свой город!\n\r", ch);
	return;
    }

    one_argument(argument, arg);

    if (IS_NULLSTR(arg))
    {
	send_to_char("Кого ты хотел выследить?\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, NULL, arg, FALSE)) != NULL)
    {
	if (victim == ch)
	    send_to_char("У тебя раздвоение личности?\n\r", ch);
	else
	    send_to_char("Объект, который ты преследуешь, "
			 "находится прямо здесь!\n\r", ch);

	return;
    }

    act("$n внимательно приглядывается к следам на земле...",
	ch, NULL, NULL, TO_ROOM);

    if ((victim = get_char_world(ch, arg)) == NULL
	|| ch->in_room == NULL
	|| IS_SET(ch->in_room->room_flags, ROOM_NOTRACK)
	|| number_percent() >= skill)
    {
	send_to_char("Ты не замечаешь следов...\n\r", ch);
	ch->mana -= mana/2;
	return;
    }

    if ((tr = get_track(ch, victim)) != NULL)
    {
	sprintf(arg, "Объект был здесь %d %s тому назад и отправился %s.\n\r",
		tr->ago, hours(tr->ago, TYPE_HOURS),
		direct(tr->direction, TRUE));
	send_to_char(arg, ch);
	check_improve(ch, victim, gsn_track, TRUE, 3);
    }
    else
    {
	send_to_char("Ты не замечаешь здесь следов "
		     "преследуемого объекта...\n\r", ch);
	check_improve(ch, victim, gsn_track, FALSE, 3);
    }	

    ch->mana -= mana;

    WAIT_STATE(ch, skill_table[gsn_track].beats);
}

void do_camouflage(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
    char arg[MAX_INPUT_LENGTH];
    int sn, skill;
    int64_t bv;

    if (!IS_IN_WILD(ch))
    {
	send_to_char("Только не тут!\n\r", ch);
	return;
    }

    if (MOUNTED(ch))
    {
	send_to_char("На лошади - не выйдет!\n\r", ch);
	return;
    }

    if (IS_AFFECTED(ch, AFF_FAERIE_FIRE))
    {
	send_to_char("Ты не можешь замаскироваться, пока от тебя исходит свечение.\n\r", ch);
	return;
    }


    //    if (!IS_NPC(ch) && IS_SET(ch->act, PLR_EXCITED))
    //    {
    //        act("Ты слишком взволнован$t, чтобы слиться с местностью.", ch, SEX_ENDING(ch), NULL, TO_CHAR);
    //	return;
    //    }

    one_argument(argument, arg);

    if (arg[0] != '\0' && (!str_prefix(arg, "move") || !str_prefix(arg, "движение")))
    {
	sn = gsn_camouflage_move;
	skill = sn;
	bv = AFF_CAMOUFLAGE_MOVE;
    }
    else
    {
	sn = gsn_camouflage;
	skill = gsn_camouflaging;
	bv = AFF_CAMOUFLAGE;
    }

    if (IS_AFFECTED(ch, bv) || is_affected(ch, sn))
    {
	send_to_char("Тебя и так уже очень трудно заметить на фоне окружающей тебя местности.\n\r", ch);
	return;
    }

    if (number_percent() <= get_skill(ch, skill))
    {
	check_improve(ch, NULL, skill, TRUE, 3);
	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = ch->level;
	af.duration = ch->level;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = bv;
	af.caster_id = 0;
	affect_to_char(ch, &af);

	send_to_char("Ты сливаешься с окружающей местностью.\n\r", ch);
    }
    else
    {
	check_improve(ch, NULL, skill, FALSE, 3);
	send_to_char("Ты пытаешься слиться с окружающей тебя местностью.\n\r", ch);
    }

    WAIT_STATE(ch, (!IS_NPC(ch) && IS_SET(ch->act, PLR_EXCITED)) ? 4 : 1 * skill_table[skill].beats);
}

void do_attention(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
    int sn = skill_lookup("attention");
    int skill;

    if ((skill = get_skill(ch, sn)) < 1)
    {
	send_to_char("Ты не знаешь, как это делается...\n\r", ch);
	return;
    }

    if (IS_AFFECTED(ch, AFF_DETECT_CAMOUFLAGE))
    {
	send_to_char("Твое внимание уже на максимуме.\n\r", ch);
	return;
    }

    if (number_percent() >= skill)
    {
	check_improve(ch, NULL, sn, FALSE, 6);
	send_to_char("Ничего не получается...\n\r", ch);
	return;
    }

    check_improve(ch, NULL, sn, TRUE, 6);
    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = ch->level;
    af.duration = ch->level;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = AFF_DETECT_CAMOUFLAGE;
    af.caster_id = 0;
    affect_to_char(ch, &af);

    printf_to_char("Ты становишься ОЧЕНЬ внимательн%s!\n\r", ch, SEX_END_ADJ(ch));
}

void do_levitate(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
    int skill;

    if ((skill = get_skill(ch, gsn_levitate)) < 1)
    {
	send_to_char("Но... Ты же не умеешь летать!\n\r", ch);
	return;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_NO_FLY))
    {
	send_to_char("Здесь не взлететь.\n\r", ch);
	return;
    }

    if (IS_AFFECTED(ch, AFF_FLYING))
	send_to_char("Ты уже летаешь.\n\r", ch);
    else
    {
	if (ch->mana < 20)
	{
	    send_to_char("У тебя не хватает энергии для того, "
			 "чтобы взлететь.\n\r", ch);
	    return;
	}

	if (skill <= number_percent())
	{
	    send_to_char("Ничего не получается...\n\r", ch);
	    check_improve(ch, NULL, gsn_levitate, FALSE, 4);
	    ch->mana -= 10;
	    return;
	}
	af.where = TO_AFFECTS;
	af.type = gsn_levitate;
	af.level = ch->level;
	af.duration = ch->level / 2;
	af.modifier = 0;
	af.location = 0;
	af.bitvector = AFF_FLYING;
	af.caster_id = 0;
	affect_to_char(ch, &af);
	act("Ты начинаешь парить $X.", ch, NULL, NULL, TO_CHAR);
	act("$n начинает парить $X.", ch, NULL, NULL, TO_ROOM);
	check_improve(ch, NULL, gsn_levitate, TRUE, 6);
	ch->mana -= 20;
    }
    return;
}

void do_enter(CHAR_DATA *ch, char *argument)
{
    ROOM_INDEX_DATA *location;

    if (ch->fighting != NULL)
    {
	send_to_char("Не получается...\n\r", ch);
	return;
    }

    /* nifty portal stuff */
    if (argument[0] != '\0')
    {
	ROOM_INDEX_DATA *old_room;
	OBJ_DATA *portal;
	CHAR_DATA *fch, *fch_next;

	old_room = ch->in_room;

	portal = get_obj_list(ch, argument,  ch->in_room->contents);

	if (portal == NULL)
	{
	    send_to_char("Ты не видишь этого здесь.\n\r", ch);
	    return;
	}

	if (IS_SET(old_room->room_flags, ROOM_NOGATE))
	{
	    send_to_char("Какие-то силы блокируют тебя...\n\r", ch);
	    return;
	}

	if (portal->item_type != ITEM_PORTAL
	    ||  (IS_SET(portal->value[1], EX_CLOSED) && !IS_TRUSTED(ch, ANGEL)))
	{
	    send_to_char("Ты не видишь способа войти сюда.\n\r", ch);
	    return;
	}

	if (!IS_TRUSTED(ch, ANGEL) && !IS_SET(portal->value[2], GATE_NOCURSE)
	    &&  (IS_AFFECTED(ch, AFF_CURSE) || IS_SET(old_room->room_flags, ROOM_NOGATE)))
	{
	    send_to_char("Какие-то силы блокируют тебя...\n\r", ch);
	    return;
	}

	if (IS_SET(portal->value[2], GATE_UNDEAD))
	{
	    if (!IS_UNDEAD(ch) && ch->classid != CLASS_NECROMANT && ch->classid != CLASS_NAZGUL && ch->race != RACE_ZOMBIE)
	    {
		send_to_char("Тебе лучше туда не входить, оттуда пахнет мертвым запахом...\n\r", ch);
		return;
	    }
	}

	if (IS_SET(portal->value[2], GATE_RANDOM) || portal->value[3] == -1)
	{
	    location = get_random_room(ch);
	    portal->value[3] = location->vnum; /* for record keeping :) */
	}
	else if (IS_SET(portal->value[2], GATE_BUGGY) && (number_percent() < 5))
	    location = get_random_room(ch);
	else
	    location = get_room_index(portal->value[3]);

	if (location == NULL
	    ||  location == old_room
	    ||  (room_is_private(location, MOUNTED(ch)) && !IS_TRUSTED(ch, IMPLEMENTOR))
	    || !can_see_room(ch, location))
	{
	    act("В $p6 нельзя войти, так как ты не сможешь попасть в то место, куда ведет этот портал.", ch, portal, NULL, TO_CHAR);
	    return;
	}

	if (IS_NPC(ch) && IS_SET(ch->act, ACT_AGGRESSIVE)
	    &&  IS_SET(location->room_flags, ROOM_LAW))
	{
	    send_to_char("Какие-то силы препятствуют тебе...\n\r", ch);
	    return;
	}

	if (IS_NPC(ch) && IS_SET(location->room_flags, ROOM_NO_MOB))
	{
	    if (ch->master != NULL)
		act("$n не сможешь попасть в то место, куда ведет этот портал.", 
			ch, NULL, ch->master, TO_VICT);
	    return;
	}

	act("$n входит в $p6.", ch, portal, NULL, TO_ROOM);

	if (IS_SET(portal->value[2], GATE_NORMAL_EXIT))
	    act("Ты входишь в $p6.", ch, portal, NULL, TO_CHAR);
	else
	    act("Ты проходишь через $p6 и находишь себя в другом месте...",
		ch, portal, NULL, TO_CHAR);

	if (IS_SET(portal->value[2], GATE_GOWITH)) /* take the gate along */
	{
	    obj_from_room(portal);
	    obj_to_room(portal, location);
	}

	char_from_room(ch);
	char_to_room(ch, location, TRUE);

	if (!LIST_EMPTY(&location->people))
	{
	    if (IS_SET(portal->value[2], GATE_NORMAL_EXIT))
		act("$n прибывает.", ch, NULL, LIST_FIRST(&location->people), TO_ROOM);
	    else
		act("$n прибывает через $p6.", ch, portal, LIST_FIRST(&location->people), TO_ROOM);
	}

	/*	do_function(ch, &do_look, "auto");*/

	if (check_trap(ch))
	    return;

	/* charges */
	if (portal->value[0] > 0)
	{
	    portal->value[0]--;
	    if (portal->value[0] == 0)
		portal->value[0] = -1;
	}

	/* protect against circular follows */
	if (old_room == location)
	    return;

	LIST_FOREACH_SAFE(fch, &old_room->people, room_link, fch_next)
	{
	    if (portal == NULL || portal->value[0] == -1)
		/* no following through dead portals */
		continue;

	    if (fch->master == ch && IS_AFFECTED(fch, AFF_CHARM)
		&&   fch->position < POS_STANDING)
		do_function(fch, &do_stand, "");

	    if (fch->master == ch && fch->position == POS_STANDING)
	    {

		if (IS_SET(ch->in_room->room_flags, ROOM_LAW)
		    &&  (IS_NPC(fch) && IS_SET(fch->act, ACT_AGGRESSIVE)))
		{
		    act("Ты не можешь притащить $N3 в город.", 	ch, NULL, fch, TO_CHAR);
		    act("Тебе не разрешено появляться в городе.", fch, NULL, NULL, TO_CHAR);
		    continue;
		}

		if (IS_NPC(fch) && IS_SET(ch->in_room->room_flags, ROOM_NO_MOB))
		    continue;

		act("Ты следуешь за $N4.", fch, NULL, ch, TO_CHAR);
		do_function(fch, &do_enter, argument);
	    }
	}

	if (portal != NULL && portal->value[0] == -1)
	{
	    act("$p исчезает в никуда.", ch, portal, NULL, TO_CHAR);
	    if (ch->in_room == old_room)
		act("$p исчезает в никуда.", ch, portal, NULL, TO_ROOM);
	    else if (!LIST_EMPTY(&old_room->people))
	    {
		act("$p исчезает в никуда.",
		    LIST_FIRST(&old_room->people), portal, NULL, TO_ALL);
	    }
	    extract_obj(portal, FALSE, FALSE);
	}

	/*
	 * If someone is following the char, these triggers get activated
	 * for the followers before the char, but it's safer this way...
	 */
	if (IS_NPC(ch) && has_trigger(ch->pIndexData->progs, TRIG_ENTRY, EXEC_ALL))
	    p_percent_trigger(ch, NULL, NULL, NULL, NULL, NULL, TRIG_ENTRY);
	if (!IS_NPC(ch))
	{
	    p_greet_trigger(ch, PRG_MPROG);
	    p_greet_trigger(ch, PRG_OPROG);
	    p_greet_trigger(ch, PRG_RPROG);
	}

	return;
    }

    send_to_char("Нет, ты не можешь этого сделать.\n\r", ch);
    return;
}

void run(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    int pexit, count = 0, max_count, skill, i;
    bool fRun, cycle;
    int vnums[MAX_CYCLES];

    if ((skill = get_skill(ch, gsn_run)) < 1)
    {
	send_to_char("Ты не знаешь, как это делается.\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || (pexit = what_door(arg)) < 0)
    {
	send_to_char("В какую сторону?\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
	max_count = ch->move + 1;
    else
    {
	one_argument(argument, arg);
	if (!is_number(arg) || (max_count = atoi(arg)) < 0 || max_count > ch->move)
	{
	    send_to_char("Второй аргумент должен быть числовым и в разумных пределах.\n\r", ch);
	    return;
	}
    }

    if (ch->in_trap)
    {
	ch->in_trap = FALSE;
	return;
    }

    for (i = 0; i < MAX_CYCLES; i++)
	vnums[i] = 0;

    while (1)
    {
	if (MOUNTED(ch))
	{
	    fRun = FALSE;

	    if (MOUNTED(ch)->move < 1)
		break;
	}
	else
	    fRun = TRUE;

	if (ch->move < 1)
	{
	    act("Ты слишком устал$t, чтобы бежать еще.", ch, SEX_ENDING(ch), NULL, TO_CHAR);
	    ch->move = 0;
	    break;
	}

	/*фикс баги:
#1 [01.02 02:39] [214] Врейн: Умень бег. Бегу по лесу никого не трогаю-встречаю Рию. Бего пробегаю дальше но батлтик один
всегда проходит. Добегаю до наемников - у меня в промте строчка ее жизней и считается что бой еще не закончен, хотя ее нету
со мной в одной клетке. В Итоге наемники вопят на защиту невинных и склеивают меня - итог Делев %)*/

	/*Останавливаю бег, если чар в бою с чаром. Если что, то этот кусок убрать.*/
	if (ch->fighting != NULL && !IS_NPC(ch->fighting))
	{
	    act("Ты останавливаешься и принимаешь бой.", ch, NULL, NULL, TO_CHAR);
	    break;
	}

	if (!ch->in_room || IS_SET(ch->in_room->room_flags, ROOM_NORUN))
	{
	    send_to_char("Тут невозможно быстро двигаться.\n\r", ch);
	    break;
	}

	if (skill < number_percent())
	    break;
	
	vnums[count] = ch->in_room->vnum;

	move_char(ch, pexit, (!fRun), TRUE);

	if (!ch->in_room)
	    break;

	cycle = FALSE;

	for (i = 0; i <= count; i++)
	    if (vnums[i] == ch->in_room->vnum)
	    {
		cycle = TRUE;
		break;
	    }

	if (cycle)
	    break;

	send_to_char("\n\r", ch);

	if (++count >= max_count || count >= MAX_CYCLES)
	    break;

	if (ch->in_trap)
	{
	    ch->in_trap = FALSE;
	    break;
	}

	if (MOUNTED(ch) && MOUNTED(ch)->in_trap)
	{
	    MOUNTED(ch)->in_trap = FALSE;
	    break;
	}


    }

    if (count > 0)
    {
	if (!MOUNTED(ch))
	{
	    send_to_char("Ты замедляешь ход и переводишь дух.\n\r", ch);
	    act("$n замедляет ход и переводит дух.", ch, NULL, NULL, TO_ROOM);
	    WAIT_STATE(ch, skill_table[gsn_run].beats / (is_lycanthrope(ch) ? 2 : 1));
	}
	else
	{
	    act("Ты останавливаешь $N3 и переводишь дух.", ch, NULL, MOUNTED(ch), TO_CHAR);
	    act("$n останавливает $N3 и переводит дух.", ch, NULL, MOUNTED(ch), TO_ROOM);
	    WAIT_STATE(ch, skill_table[gsn_run].beats / (is_lycanthrope(ch) ? 2 : 1));
	}
	check_improve(ch, NULL, gsn_run, TRUE, 2);
    }
    else
    {
	send_to_char(fRun ? "У тебя не получается начать бег.\n\r" :
		     "Твоя лошадь не смогла разогнаться.\n\r", ch);
    }
}

void do_gallop(CHAR_DATA *ch, char *argument)
{
    if (!MOUNTED(ch))
    {
	send_to_char("Ты не на лошади.\n\r", ch);
	return;
    }

    if (IS_AFFECTED(MOUNTED(ch), AFF_FLYING))
    {
	send_to_char("Твоя лошадь не касается ногами земли...\n\r", ch);
	return;
    }

    run(ch, argument);
}

void do_run(CHAR_DATA *ch, char *argument)
{
    if (MOUNTED(ch))
    {
	send_to_char("Ты же на лошади!\n\r", ch);
	return;
    }

    if (IS_AFFECTED(ch, AFF_FLYING))
    {
	send_to_char("Ты же не касаешься ногами земли...\n\r", ch);
	return;
    }

    run(ch, argument);
}

void do_blood_ritual(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int e;
    AFFECT_DATA af;
    int chance;

    //проверка - знает ли чар умение, и доступно ли оно ему вообще?

    if (IS_NPC(ch) || (chance = get_skill(ch, gsn_blood_ritual)) < 1)
    {
	send_to_char("Ритуал крови? Что это такое?\n\r",ch);
	return;
    }

    //Проверка, висит ли на чаре флаг магии крови?

    if (is_affected(ch, gsn_blood_ritual))
    {
	send_to_char("Накопи сначала сил для ритуала, пусть кровь восстановится.\n\r",ch);
	return;
    }

    //Проверка - есть ли на чаре кинжал?
    if (((obj = get_eq_char(ch, WEAR_WIELD)) == NULL)
	|| (obj->item_type != ITEM_WEAPON) || (obj->value[0] != WEAPON_DAGGER))
    {
	send_to_char("Тебе нечем получить свою кровь.\n\r",ch);
	return;
    }

    //Проверка - не слишком ли мал для чара кинжал, который на нем.
    if (obj->level + 10 < ch->level)
    {
	send_to_char("Этот кинжал не справится с мощью твоей крови.\n\r",ch);
	return;
    }

    //Проверка - кому чар хочет добавить маны?
    one_argument(argument,arg);

    if (arg[0] == '\0')
    {
	victim = ch;
    }
    else if ((victim = get_char_room(ch, NULL, arg, FALSE)) == NULL)
    {
	send_to_char("Такого рядом с тобой нет.\n\r",ch);
	return;
    }

    //Проверка - хватит ли чару жизни для совершения ритуала?
    e = ch->max_hit / 3;

    if (ch->hit <= e)
    {
	send_to_char("У тебя слишком мало крови.\n\r",ch);
	return;
    }

    if (victim->mana >= victim->max_mana)
    {
	if (victim != ch)
	    act("$N не нуждается в твоей крови.", ch, NULL, victim, TO_CHAR);
	else
	    send_to_char("Оно тебе надо?\n\r", ch);
	return;
    }

    //Отнимание 1/3 от максимальной жизни из хп чара.
    ch->hit -= e;

    send_to_char("Ты начинаешь обряд крови.\n\r",ch);

    //Проверка - получилось ли у чара в зависимости от знания умения?
    if (number_percent() >= get_skill(ch, gsn_blood_ritual))
    {
	send_to_char("Кровь слишком сильно плеснула из твоих вен...\n\rТы не можешь справиться с обрядом.\n\r",ch);
	check_improve(ch, NULL, gsn_blood_ritual, FALSE, 2);
	return;
    }

    check_improve(ch, NULL, gsn_blood_ritual, TRUE, 2);

    af.where	= TO_AFFECTS;
    af.type 	= gsn_blood_ritual;
    af.level 	= ch->level;
    af.duration	= 24;
    af.location	= APPLY_NONE;
    af.modifier	= 0;
    af.bitvector= 0;
    af.caster_id = 0;
    affect_to_char(ch, &af);

    victim->mana = UMIN(victim->mana + e, victim->max_mana);

    make_worse_condition(ch, WEAR_WIELD, 10, DAM_PIERCE);

    send_to_char("Ты отдаешь энергию КРОВИ!\n\r", ch);
    send_to_char("Внезапно тебя наполняет энергия КРОВИ!\n\r",victim);
    WAIT_STATE(ch, skill_table[gsn_blood_ritual].beats);
    return;
}

void do_concentrate(CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
    int chance;

    //проверка - знает ли чар умение, и доступно ли оно ему вообще?
    if (IS_NPC(ch) || (chance = get_skill(ch, gsn_concentrate)) < 1)
    {
	send_to_char("А зачем это тебе нужно? Ты и так неплохо машешь оружием.\n\r",ch);
	return;
    }

    //Проверка, висит ли на чаре флаг концентрации?
    if (is_affected(ch, gsn_concentrate))
    {
	send_to_char("Твоя концентрация и так уже на пределе.\n\r",ch);
	return;
    }

    if (ch->move < 50)
    {
	printf_to_char("Ты слишком устал%s.\n\r",ch, SEX_ENDING(ch));
	return;
    }

    send_to_char("Ты пытаешься сконцентрироваться.\n\r",ch);

    //Проверка - получилось ли у чара в зависимости от знания умения?
    if (number_percent() >= get_skill(ch, gsn_concentrate))
    {
	send_to_char("Тебя что-то отвлекло.\n\r",ch);
	check_improve(ch, NULL, gsn_concentrate, FALSE, 2);
	return;
    }

    check_improve(ch, NULL, gsn_concentrate, TRUE, 2);

    af.where	= TO_AFFECTS;
    af.type 	= gsn_concentrate;
    af.level 	= ch->level;
    af.duration	= 2;
    af.location	= APPLY_INT;
    af.modifier	= 1;
    af.bitvector= 0;
    af.caster_id = 0;
    affect_to_char(ch, &af);

    ch->move -= 50;

    send_to_char("Ты достигаешь просветления!\n\r", ch);
    WAIT_STATE(ch, skill_table[gsn_concentrate].beats);
    return;
}

void do_prayer(CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
    int chance;

    //проверка - знает ли чар умение, и доступно ли оно ему вообще?
    if (IS_NPC(ch) || (chance = get_skill(ch, gsn_prayer)) < 1)
    {
	send_to_char("Научись сначала молиться.\n\r",ch);
	return;
    }

    //Проверка, висит ли на чаре флаг молитвы?
    if (is_affected(ch, gsn_prayer))
    {
	printf_to_char("Ты уже взывал%s к богам.\n\r",ch, SEX_ENDING(ch));
	return;
    }

    if (ch->move < 50)
    {
	printf_to_char("Ты слишком устал%s.\n\r",ch, SEX_ENDING(ch));
	return;
    }

    send_to_char("Ты молишься богам.\n\r",ch);

    //Проверка - получилось ли у чара в зависимости от знания умения?
    if (number_percent() >= get_skill(ch, gsn_prayer))
    {
	send_to_char("Тебя что-то отвлекло.\n\r",ch);
	check_improve(ch, NULL, gsn_prayer, FALSE, 2);
	return;
    }

    check_improve(ch, NULL, gsn_prayer, TRUE, 2);

    af.where	= TO_AFFECTS;
    af.type 	= gsn_prayer;
    af.level 	= ch->level;
    af.duration	= (ch->level)/10 +1;
    af.location	= 0;
    af.modifier	= 0;
    af.bitvector= 0;
    af.caster_id= 0;
    affect_to_char(ch, &af);

    ch->move -= 50;

    act("Боги даруют тебе силу!", ch, NULL, NULL, TO_CHAR);
    act("$n сотворяет молитву светлым богам.", ch, NULL, NULL, TO_ROOM);
    WAIT_STATE(ch, skill_table[gsn_prayer].beats);
    return;
}


void do_cover(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj;

    if (ch->position > POS_SITTING)
    {
	send_to_char("Ты можешь укутаться только на отдыхе.\n\r", ch);
	return;
    }

    if ((obj = get_eq_char(ch, WEAR_ABOUT)) == NULL || is_lycanthrope(ch))
    {
	send_to_char("На тебе нет плаща.\n\r", ch);
	return;
    }

    if (IS_OBJ_STAT(obj, ITEM_INVIS))
    {
	send_to_char("Ты не сможешь укутаться в невидимый плащ.\n\r", ch);
	return;
    }

    if (!can_see_obj(ch, obj))
    {
	send_to_char("Ты же не видишь этого!\n\r", ch);
	return;
    }

    if (obj->item_type == ITEM_CONTAINER || obj->item_type == ITEM_DRINK_CON)
    {
	act("Ты не сможешь укутаться в $p6.", ch, obj, NULL, TO_CHAR);
	return;
    }

    SET_BIT(ch->affected_by, AFF_COVER);

    act("Ты укутываешься в $p6.", ch, obj, NULL, TO_CHAR);
    act("$n укутывается в $p6.", ch, obj, NULL, TO_ROOM);
}

void do_lost_command(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_INPUT_LENGTH];
    if (IS_NPC(ch))
	return;

    if (argument[0] == '\0')
    {
	sprintf(buf, "Твоя текущая лост-команда: %s\n\r", ch->lost_command);
	send_to_char(buf, ch);
	return;
    }

    free_string(ch->lost_command);
    smash_tilde(argument);
    ch->lost_command = str_dup(argument);

    send_to_char("Команда успешно записана.\n\r", ch);
    return;
}


void do_grimuar(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
    int chance;
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int liquid;

    /* проверка - знает ли чар умение, и доступно ли оно ему вообще? */
    if (IS_NPC(ch) || (chance = get_skill(ch, gsn_grimuar)) < 1)
    {
	send_to_char("Научись сначала гримуару.\n\r",ch);
	return;
    }

    if (ch->fighting)
    {
	send_to_char("Куда? Ведь ты еще сражаешься!\n\r", ch);
	return;
    }

    /* Проверка, висит ли на чаре флаг гримуара? */
    if (is_affected(ch, gsn_grimuar))
    {
	printf_to_char("Ты уже исполнил%s гримуар.\n\r",ch, SEX_ENDING(ch));
	return;
    }

    if (ch->move < 50)
    {
	printf_to_char("Ты слишком устал%s.\n\r",ch, SEX_ENDING(ch));
	return;
    }

    /* проверка на наличие контейнера с кровью, etc */
    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Для совершения гримуара тебе нужен сосуд с кровью.\n\r", ch);
	return;
    }

    if ((obj = get_obj_here(ch, NULL, arg)) == NULL)
    {
	send_to_char("У тебя нет такого сосуда.\n\r", ch);
	return;
    }

    if (obj->item_type != ITEM_DRINK_CON
	&& obj->item_type != ITEM_FOUNTAIN)
    {
	send_to_char("Это не сосуд.\n\r", ch);
	return;
    }

    if (obj->item_type == ITEM_DRINK_CON && obj->value[1] <= 0)
    {
	send_to_char("Здесь уже пусто.\n\r", ch);
	return;
    }

    if ((liquid = obj->value[2])  < 0)
    {
	bugf("Do_drink: bad liquid number %d.", liquid);
	liquid = obj->value[2] = 0;
    }

    if (str_cmp(liq_table[liquid].liq_name, "blood"))
    {
	send_to_char("В сосуде нет крови.\n\r", ch);
	return;
    }

    if (obj->item_type == ITEM_DRINK_CON && obj->value[1] < 10)
    {
	send_to_char("Здесь слишком мало крови.\n\r", ch);
	return;
    }

    send_to_char("Проведя жуткий ритуал на крови, ты просишь помощи у демонов Тьмы.\n\r",ch);

    if (obj->value[0] > 0)
	obj->value[1] -= 10;


    /* Проверка - получилось ли у чара в зависимости от знания умения? */
    if (number_percent() >= get_skill(ch, gsn_grimuar))
    {
	send_to_char("Тебя что-то отвлекло.\n\r",ch);
	check_improve(ch, NULL, gsn_grimuar, FALSE, 2);
	return;
    }

    check_improve(ch, NULL, gsn_grimuar, TRUE, 2);

    af.where	= TO_AFFECTS;
    af.type 	= gsn_grimuar;
    af.level 	= ch->level;
    af.duration	= ch->level / 10 + 1;
    af.location	= 0;
    af.modifier	= 0;
    af.bitvector= 0;
    af.caster_id= 0;
    affect_to_char(ch, &af);

    ch->move -= 50;

    send_to_char("Демоны тьмы даруют тебе силу!\n\r", ch);
    WAIT_STATE(ch, skill_table[gsn_grimuar].beats);

    return;
}

void do_repent(CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *sm;
    char buf[MAX_INPUT_LENGTH];

    if (IS_NPC(ch))
	return;

    if (ch->count_holy_attacks == 0 && ch->count_guild_attacks == 0)
    {
	send_to_char("Тебе не в чем каяться. Иди на волю.\n\r", ch);
	return;
    }

    // проверяем наличие палача.
    LIST_FOREACH(sm, &ch->in_room->people, room_link)
    {
	if (IS_NPC(sm) && IS_SET(sm->act, ACT_EXECUTIONER))
	    break;
    }

    if (sm == NULL)
    {
	send_to_char("Сначала найди палача.\n\r", ch);
	return;
    }

    //проверка на достаточность qp и золота
    if (ch->pcdata->quest_curr < 500)
    {
	sprintf(buf, "%s, у тебя нет требуемого количества пунктов удачи.", ch->name);
	do_say(sm, buf);

	sprintf(buf, "Покаяться и получить прощение стоит 500 очков удачи.");
	do_say(sm, buf);
	return;
    }

    if (ch->silver + 100 * ch->gold < 100000)
    {
	sprintf(buf, "%s, у тебя нет требуемого количества золота.", ch->name);
	do_say(sm, buf);
	sprintf(buf, "Покаяться и получить прощение стоит 1000 золота.");
	do_say(sm, buf);
	return;
    }

    deduct_cost(ch, 100000);
    ch->pcdata->quest_curr -= 500;
    send_to_char("Ты платишь палачу 1000 золота и 500 очков удачи.\n\r", ch);

    if (ch->count_holy_attacks > 0)
	ch->count_holy_attacks-- ;
    else
	ch->count_guild_attacks-- ;

    sprintf(buf, "Получи же кару за свои прегрешения!");
    do_say(sm, buf);

    act("{W$N отсекает твою голову!{x", ch, NULL, sm, TO_CHAR);
    act("{W$N отсекает голову $n1!{x", ch, NULL, sm, TO_ROOM);

    raw_kill(ch, sm, TRUE);		   

    return;
}


/* charset=cp1251 */




