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
#include <math.h>

#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "merc.h"
#include "magic.h"
#include "interp.h"
#include "recycle.h"


ROOM_INDEX_DATA *	find_location	args((CHAR_DATA *ch, char *arg));
/*
 * Local functions.
 */
#define CD CHAR_DATA
#define OD OBJ_DATA

bool	remove_obj	args((CHAR_DATA *ch, int iWear, bool fReplace));
void	wear_obj	args((CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace));
CD *	find_keeper	args((CHAR_DATA *ch));
int	get_cost	args((CHAR_DATA *keeper, OBJ_DATA *obj, bool fBuy));
void 	obj_to_keeper	args((OBJ_DATA *obj, CHAR_DATA *ch));
OD *	get_obj_keeper	args((CHAR_DATA *ch, CHAR_DATA *keeper, char *argument));
char *  item_name(int item_type, int rname);
void	obj_cast_spell	args((int sn, int level, CHAR_DATA *ch,
			      CHAR_DATA *victim, OBJ_DATA *obj, char *tn));

void	show_list_to_char	args((OBJ_DATA *list, CHAR_DATA *ch,
				      bool fShort, bool fShowNothing, bool sacrifice, const int iDefaultAction));

bool check_filter(CHAR_DATA *ch, CHAR_DATA *victim);
int find_exit(CHAR_DATA *ch, char *arg);
void trap_damage(CHAR_DATA *ch, OBJ_DATA *trap);
bool check_primary_secondary(CHAR_DATA *ch, OBJ_DATA *prim, OBJ_DATA *sec);
bool check_auction_obj(CHAR_DATA *ch, OBJ_DATA *obj);

#undef OD
#undef	CD


bool check_obj_spells(OBJ_DATA *obj, int sn)
{
    if (obj == NULL || obj->item_type != ITEM_SCROLL || sn < 0 || sn >= max_skills)
	return FALSE;

    if (!str_cmp(skill_table[obj->value[sn]].name, "locate object")
     || !str_cmp(skill_table[obj->value[sn]].name, "gate")
     || !str_cmp(skill_table[obj->value[sn]].name, "portal")
     || !str_cmp(skill_table[obj->value[sn]].name, "nexus")
     || !str_cmp(skill_table[obj->value[sn]].name, "beacon"))
        return TRUE;

    return FALSE;
}

bool check_mortar_put(OBJ_DATA *mortar){
    OBJ_DATA *pobj;
    int i = 0;

    for (pobj = mortar->contains; pobj != NULL; pobj = pobj->next_content)
	i++;

    return (i < mortar->value[2]);
}

void act_wear(char *txt, CHAR_DATA *ch, OBJ_DATA *obj){
    CHAR_DATA *rch, *safe_rch;

    if (!ch->in_room)
	return;

    LIST_FOREACH_SAFE(rch, &ch->in_room->people, room_link, safe_rch){
		if (ch != rch && SUPPRESS_OUTPUT(check_blind(rch))){
	    char buf[MSL];

	    sprintf(buf, txt, !can_see_obj(rch, obj) ? "о" : "");
	    act(buf, ch, obj, rch, TO_VICT);
		}
    }
}


void show_weapon_level(CHAR_DATA *ch, OBJ_DATA *obj, bool secondary){
    int sn, skill;

    sn = get_weapon_sn(ch, secondary);

    if (sn == gsn_hand_to_hand || sn == -2)
	return;

    skill = get_weapon_skill(ch, sn);

    if (skill >= 100)
	act("$p чувству$T себя как часть тебя!", ch, obj, decompose_end(obj->short_descr), TO_CHAR);
    else if (skill > 85)
	act("Ты чувствуешь себя уверенно с $p4.", ch, obj, NULL, TO_CHAR);
    else if (skill > 70)
	act("Ты неплохо владеешь $p4.", ch, obj, NULL, TO_CHAR);
    else if (skill > 50)
	act("Твой уровень мастерства с $p4 удовлетворителен.",
	    ch, obj, NULL, TO_CHAR);
    else if (skill > 25)
	act("$p чувству$T себя неуверенно в твоих руках.",
	    ch, obj, decompose_end(obj->short_descr), TO_CHAR);
    else if (skill > 1)
	act("Ты едва нащупываешь $p6 и чуть не роняешь.",
	    ch, obj, NULL, TO_CHAR);
    else
	act("Ты даже не знаешь, за какой конец взять $p6.",
	    ch, obj, NULL, TO_CHAR);
    return;
}

bool can_loot(CHAR_DATA *ch, OBJ_DATA *obj){
    CHAR_DATA *owner, *wch;
    int sl = MAX_LEVEL / 2;

    if (IS_NPC(ch))
        ch = get_master(ch);

    if (IS_IMMORTAL(ch) || IS_NULLSTR(obj->owner))
		return TRUE;

    if (!str_cmp(obj->owner, ch->name))
		return TRUE;

	if (obj->item_type == ITEM_CONTAINER && obj->pIndexData->vnum != OBJ_VNUM_ALCHEMIST_PATRONTASH)
		return TRUE;

    if (obj->item_type != ITEM_CORPSE_PC)
		return FALSE;

    if (obj->in_room && IS_SET(obj->in_room->room_flags, ROOM_NOLOOT))
		return FALSE;

    owner = NULL;
    LIST_FOREACH(wch, &char_list, link)
	if (!str_cmp(wch->name, obj->owner))
	    owner = wch;

    if (owner == NULL)
	return TRUE;

    if (is_same_group(ch, owner))
	return TRUE;
    
//— а ўлиҐ 30 га®ў­п ­Ґ ¬®¦Ґв «гвЁвм г з а  ­Ё¦Ґ 30 Ё ­ ®Ў®а®в
    if (ch->level - sl + obj->level < 51)
	return TRUE;

    if (ch->level > PK_RANGE / 2
	&& ((is_clan(ch)
	     && is_clan(owner)) || (!is_clan(ch) && !is_clan(owner))))
    {
	return TRUE;
    }

    return FALSE;
}


bool get_obj(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *container, bool fAll){
    CHAR_DATA *gch, *safe_gch;
    int members;
    char buffer[100];

    if (!CAN_WEAR(obj, ITEM_TAKE))
    {
	send_to_char("Ты не можешь взять это.\n\r", ch);
	return TRUE;
    }

    if (!can_loot(ch, obj))
    {
	act("Извини, ты не можешь взять это.", ch, NULL, NULL, TO_CHAR);
	return TRUE;
    }

    if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch))
    {
	act("$P: ты не можешь нести так много вещей.",
	    ch, NULL, obj, TO_CHAR);
	return FALSE;
    }

    if (obj->in_obj == NULL || obj->in_obj->carried_by != ch)
	if (!can_take_weight(ch, get_obj_weight(obj)))
	{
	    act("$P: ты уже перегружен$t вещами.",
		ch, SEX_ENDING(ch), obj, TO_CHAR);
	    return TRUE;
	}

    if (obj->in_room != NULL)
    {
	LIST_FOREACH_SAFE(gch, &obj->in_room->people, room_link, safe_gch)
	{
	    if (gch->on == obj)
	    {
		if (gch == ch)
		    act("Может тебе для начала освободить $p6?", ch, obj, NULL, TO_CHAR);
		else
		    act("Попроси $N3 освободить $p6.", ch, obj, gch, TO_CHAR);

		return TRUE;
	    }
	}
    }

    if (is_have_limit(ch, ch, obj))
	return TRUE;


    if (container != NULL)
    {
	if (container->pIndexData->vnum == OBJ_VNUM_PIT && get_trust(ch) < obj->level)
	{
	    act("Эта штука слишком мощная для тебя. Подыщи себе в $p5 что-нибудь попроще.",
		    ch, container, NULL, TO_CHAR);
	    return TRUE;
	}

	if (!fAll)
	{
	    act("Ты берешь $p6 из $P1.", ch, obj, container, TO_CHAR);
	    act("$n берет $p6 из $P1.", ch, obj, container, TO_ROOM);
	}

	obj_from_obj(obj);
    }
    else
    {
	if (!fAll)
	{
	    act("Ты берешь $p6.", ch, obj, container, TO_CHAR);
	    act("$n берет $p6.", ch, obj, container, TO_ROOM);
	}
	obj_from_room(obj);

    	if (obj->item_type == ITEM_CHEST && obj->in_room == NULL)
    	{
	    bugf("[room %d]1: %s берет сундук.", ch->in_room->vnum, ch->name);
	    delete_chest(obj->id);
    	}

    }

    if (obj->item_type == ITEM_MONEY)
    {
	ch->silver += obj->value[0];
	ch->gold += obj->value[1];
	if (!IS_NPC(ch))
	{
		ch->pcdata->bronze += obj->value[2];
	}

	if (IS_SET(ch->act, PLR_AUTOSPLIT))
	{ /* AUTOSPLIT code */
	    members = 0;
	    LIST_FOREACH(gch, &ch->in_room->people, room_link)
	    {
		if (!IS_AFFECTED(gch, AFF_CHARM) && is_same_group(gch, ch))
		    members++;
	    }

	    if (members > 1 && (obj->value[0] > 1 || obj->value[1]))
	    {
		sprintf(buffer, "%d %d", obj->value[0], obj->value[1]);
		do_function(ch, &do_split, buffer);
	    }
	}

	extract_obj(obj, FALSE, FALSE);
    }
    else
    {
    	if (obj->item_type == ITEM_CHEST && obj->in_room != NULL)
    	{
	    bugf("[room %d]2: %s берет сундук.", ch->in_room->vnum, ch->name);
	    delete_chest(obj->id);
    	}

	obj_to_char(obj, ch);

	if (has_trigger(obj->pIndexData->progs, TRIG_GET, EXEC_AFTER|EXEC_DEFAULT))
	    p_give_trigger(NULL, obj, NULL, ch, obj, TRIG_GET);
	if (has_trigger(ch->in_room->progs, TRIG_GET, EXEC_AFTER|EXEC_DEFAULT))
	    p_give_trigger(NULL, NULL, ch->in_room, ch, obj, TRIG_GET);
    }

    return TRUE;
}

void get_gold(CHAR_DATA *ch, char *arg1, char *arg2){
    OBJ_DATA *container, *obj, *obj_new;
    int amount, gold, silver, bronze;

    if (arg2[0] == '\0')
    {
	if ((obj = get_obj_here(ch, NULL, "gcash")) == NULL)
	{
	    send_to_char("Ты не видишь этого здесь.\n\r", ch);
	    return;
	}

	if (!is_number(arg1))
	{
	    if (obj->value[1] > 0)
	    {
			amount = obj->value[1];
			silver = obj->value[0];
			bronze = obj->value[2];

			obj_to_room((obj_new = create_money(amount, 0, 0)), ch->in_room);

			extract_obj(obj, FALSE, FALSE);

			if (silver > 0)
		    	obj_to_room(create_money(0, silver, 0), ch->in_room);

				get_obj(ch, obj_new, NULL, FALSE);
	    }
	    else
			send_to_char("Ты не видишь этого здесь.\n\r", ch);
	}
	else
	{
	    amount = atoi(arg1);

	    if (obj->value[1] >= amount)
	    {
		silver = obj->value[0];
		gold = obj->value[1] - amount;
		bronze = obj->value[2];

		obj_to_room((obj_new = create_money(amount, 0, 0)), ch->in_room);

		extract_obj(obj, FALSE, FALSE);

		if (gold > 0 || silver > 0 || bronze > 0)
		    obj_to_room(create_money(gold, silver, bronze), ch->in_room);

		get_obj(ch, obj_new, NULL, FALSE);
	    }
	    else
		send_to_char("Тут нет столько золота.\n\r", ch);
	}
    }
    else
    {
	if ((container = get_obj_here(ch, NULL, arg2)) == NULL)
	{
	    send_to_char("Ты не видишь этого здесь.\n\r", ch);
	    return;
	}

	switch (container->item_type)
	{
	default:
	    send_to_char("Это не контейнер.\n\r", ch);
	    return;

	case ITEM_CONTAINER:
	case ITEM_MORTAR:
	case ITEM_CHEST:
	case ITEM_CORPSE_NPC:
	case ITEM_CORPSE_PC:
	    break;
	}

	if (!can_loot(ch, container))
	{
	    send_to_char("Ты не можешь это сделать.\n\r", ch);
	    return;
	}

	if (IS_SET(container->value[1], CONT_CLOSED))
	{
	    act("$P - закрыто.", ch, NULL, container, TO_CHAR);
	    return;
	}

	if ((obj = get_obj_list(ch, "gcash", container->contains)) == NULL)
	{
	    send_to_char("Ты не видишь там ничего похожего.\n\r", ch);
	    return;
	}

	if (!is_number(arg1))
	{
	    if (obj->value[1] > 0)
	    {
		amount = obj->value[1];
		silver = obj->value[0];
		bronze = obj->value[2];
		extract_obj(obj, FALSE, FALSE);

		obj_to_obj((obj_new = create_money(amount, 0, 0)), container);

		if (silver > 0)
		    obj_to_obj(create_money(0, silver, 0), container);

		if (bronze > 0)
		    obj_to_obj(create_money(0, 0, bronze), container);


		get_obj(ch, obj_new, container, FALSE);

	    }
	    else
		send_to_char("Ты не видишь этого здесь.\n\r", ch);
	}
	else
	{
	    amount = atoi(arg1);
	    if (obj->value[1] >= amount)
	    {
		silver = obj->value[0];
		gold = obj->value[1] - amount;
		bronze = obj->value[2];

		extract_obj(obj, FALSE, FALSE);

		obj_to_obj((obj_new = create_money(amount, 0, 0)), container);

		if (silver > 0 || gold > 0 || bronze >0)
		    obj_to_obj(create_money(gold, silver, bronze), container);

		get_obj(ch, obj_new, container, FALSE);
	    }
	    else
		send_to_char("Тут нет столько золота.\n\r", ch);
	}
    }
}

void do_get(CHAR_DATA *ch, char *argument){
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    OBJ_DATA *container;
    bool found;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    argument = one_argument(argument, arg3);

    if (!str_cmp(arg2, "from") || !str_cmp(arg2, "из"))
	argument = one_argument(argument, arg2);

    /* Get type. */
    if (arg1[0] == '\0')
    {
	send_to_char("Взять что?\n\r", ch);
	return;
    }

    if (((is_number(arg1) && (atoi(arg1)) > 0) || !str_cmp(arg1, "all") || !str_cmp(arg1, "все"))
	&& (!str_cmp(arg2, "gold") || !str_cmp(arg2, "золота")))
    {
	get_gold(ch, arg1, arg3);
	return;
    }

    if (arg2[0] == '\0')
    {
	if (str_cmp(arg1, "все") && str_cmp(arg1, "all") && str_prefix("all.", arg1) && str_prefix("все.", arg1))
	{
	    /* 'get obj' */
	    obj = get_obj_list(ch, arg1, ch->in_room->contents);
	    if (obj == NULL)
	    {
		act("Ты не видишь этого здесь.", ch, NULL, NULL, TO_CHAR);
		return;
	    }

	    get_obj(ch, obj, NULL, FALSE);
	}
	else
	{
	    /* 'get all' or 'get all.obj' */
	    found = FALSE;
	    for (obj = ch->in_room->contents; obj != NULL; obj = obj_next)
	    {
		obj_next = obj->next_content;
		if ((arg1[3] == '\0' || is_name(&arg1[4], obj->name))
		    && can_see_obj(ch, obj))
		{
		   if (!ch->in_room->contents)
			return;
		    found = TRUE;
		    if (!get_obj(ch, obj, NULL, FALSE))
			break;
		}
	    }

	    if (!found)
	    {
		if (arg1[3] == '\0')
		    send_to_char("Ты не видишь здесь ничего.\n\r", ch);
		else
		    act("Ты не видишь этого здесь.", ch, NULL, NULL, TO_CHAR);
	    }
	    /*	    else
	     {
	     if (arg1[3] == '\0')
	     {
	     act("Ты забираешь себе все, что брошено под ногами.", ch, NULL, NULL, TO_CHAR);
	     act("$n забирает все, что брошено под ногами.", ch, NULL, NULL, TO_ROOM);
	     }
	     else
	     {
	     send_to_char("Ты забираешь себе все подобные предметы.\n\r", ch);
	     act("$n забирает себе некоторые предметы.", ch, NULL, NULL, TO_ROOM);
	     }
	     } */
	}
    }
    else
    {
	/* 'get ... container' */
	if (!str_cmp(arg2, "all") || !str_prefix("all.", arg2) || !str_cmp(arg2, "все") || !str_prefix("все.", arg2))
	{
	    send_to_char("Ты не можешь этого сделать.\n\r", ch);
	    return;
	}

	if ((container = get_obj_here(ch, NULL, arg2)) == NULL)
	{
	    act("Ты не видишь этого здесь.", ch, NULL, NULL, TO_CHAR);
	    return;
	}

	switch (container->item_type)
	{
	default:
	    send_to_char("Это не контейнер.\n\r", ch);
	    return;

	case ITEM_CONTAINER:
	case ITEM_CHEST:
	case ITEM_MORTAR:
	case ITEM_CORPSE_NPC:
	    break;

	case ITEM_CORPSE_PC:
	    {

		if (!can_loot(ch, container))
		{
		    send_to_char("Ты не можешь этого сделать.\n\r", ch);
		    return;
		}
	    }
	}

	if (IS_SET(container->value[1], CONT_CLOSED))
	{
	    act("$P - закрыто.", ch, NULL, container, TO_CHAR);
	    return;
	}

	if (str_cmp(arg1, "all") && str_prefix("all.", arg1) && str_cmp(arg1, "все") && str_prefix("все.", arg1))
	{
	    /* 'get obj container' */
	    obj = get_obj_list(ch, arg1, container->contains);
	    if (obj == NULL)
	    {
		send_to_char("Ты не видишь там ничего похожего.\n\r", ch);
		return;
	    }

	    if (container->trap && IS_SET(container->trap_flags, TRAP_ON_GET))
		check_trap_obj(ch, container);

	    get_obj(ch, obj, container, FALSE);

	    if (container->item_type == ITEM_CHEST && container->in_room != NULL)
		save_chest(container->id);	// Сохраняем сундук в файл
	}
	else
	{
	    /* 'get all container' or 'get all.obj container' */
	    found = FALSE;
	    for (obj = container->contains; obj != NULL; obj = obj_next)
	    {
		obj_next = obj->next_content;
		if ((arg1[3] == '\0' || is_name(&arg1[4], obj->name))
		    && can_see_obj(ch, obj))
		{
		    if (!get_obj_here(ch, NULL, arg2))
			return;

		    found = TRUE;
		    if (container->pIndexData->vnum == OBJ_VNUM_PIT
			&& !IS_IMMORTAL(ch))
		    {
			send_to_char("Не будь таким жадным!\n\r", ch);
			return;
		    }
		    if (!get_obj(ch, obj, container, FALSE))
			break;
		}
	    }

	    if (!found)
	    {
		if (arg1[3] == '\0')
		    act("Ты не видишь ничего в '$T'.",	ch, NULL, arg2, TO_CHAR);
		else
		    send_to_char("Ты не видишь там ничего похожего.\n\r", ch);
	    }
	    else
	    {
		if (container->item_type == ITEM_CHEST && container->in_room != NULL)
		    save_chest(container->id);	/* Сохраняем сундук в файл */

		if (container->trap && IS_SET(container->trap_flags, TRAP_ON_GET))
		    check_trap_obj(ch, container);
	    }
	}
    }

    return;
}

int put_obj(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *container){
    if (has_trigger(obj->pIndexData->progs, TRIG_PUT, EXEC_BEFORE|EXEC_DEFAULT))
	p_percent_trigger(NULL, obj, NULL, ch, NULL, NULL, TRIG_PUT);

    if (IS_VALID(obj))
    {
	obj_from_char(obj, TRUE);
	obj_to_obj(obj, container);

	if (IS_SET(container->value[1], CONT_PUT_ON))
	{
	    act("$n кладет $p6 на $P6.", ch, obj, container, TO_ROOM);
	    act("Ты кладешь $p6 на $P6.", ch, obj, container, TO_CHAR);
	}
	else
	{
	    act("$n кладет $p6 в $P6.", ch, obj, container, TO_ROOM);
	    act("Ты кладешь $p6 в $P6.", ch, obj, container, TO_CHAR);
	}

	if (has_trigger(obj->pIndexData->progs, TRIG_PUT, EXEC_AFTER|EXEC_DEFAULT))
            p_percent_trigger(NULL, obj, NULL, ch, container, NULL, TRIG_PUT);

	if (IS_VALID(obj))
	    return 1;
    }

    return 0;
}

void do_put(CHAR_DATA *ch, char *argument){
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *container;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (!str_cmp(arg2, "in") || !str_cmp(arg2, "on") || !str_cmp(arg2, "в") || !str_cmp(arg2, "на"))
	argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0')
    {
	send_to_char("Положить что во что?\n\r", ch);
	return;
    }

    if (!str_cmp(arg2, "all") || !str_prefix("all.", arg2) || !str_cmp(arg2, "все") || !str_prefix("все.", arg2))
    {
	send_to_char("Ты не можешь этого сделать.\n\r", ch);
	return;
    }

    if ((container = get_obj_here(ch, NULL, arg2)) == NULL)
    {
	act("Ты не видишь '$T' здесь.", ch, NULL, arg2, TO_CHAR);
	return;
    }

    if (container->item_type != ITEM_CONTAINER 
    && container->item_type != ITEM_CHEST
    && container->item_type != ITEM_MORTAR)
    {
	send_to_char("Это не контейнер.\n\r", ch);
	return;
    }

    if (container->item_type == ITEM_MORTAR && !check_mortar_put(container))
    {
	send_to_char("В ступу больше нельзя ничего положить.\n\r", ch);
	return;
    }

    if (IS_SET(container->value[1], CONT_CLOSED) 
    && container->item_type != ITEM_MORTAR)
    {
	act("$P - закрыто.", ch, NULL, container, TO_CHAR);
	return;
    }

    if (IS_SET(container->extra_flags, ITEM_AUCTION))
    {
	send_to_char("Ты не можешь ложить что-либо в контейнер, выставленный на аукцион.\n\r", ch);
	return;
    }


    if (str_cmp(arg1, "all") && str_prefix("all.", arg1) && str_cmp(arg1, "все") && str_prefix("все.", arg1))
    {
	/* 'put obj container' */
	if ((obj = get_obj_carry(ch, arg1, ch)) == NULL)
	{
	    send_to_char("У тебя нет этого.\n\r", ch);
	    return;
	}

	if (obj == container)
	{
	    send_to_char("Ты не можешь положить это само в себя.\n\r", ch);
	    return;
	}

	if (obj->item_type == ITEM_CONTAINER || obj->item_type == ITEM_MORTAR)
	{
		if (container->item_type != ITEM_CHEST)
		{
			send_to_char("Ты не можешь положить контейнер в контейнер.\n\r", ch);
			return;
		}
	}

	if (!can_drop_obj(ch, obj)
	|| obj->item_type == ITEM_CORPSE_NPC || obj->item_type == ITEM_CORPSE_PC)
	{
	    send_to_char("Ты не можешь положить это в контейнер.\n\r", ch);
	    return;
	}

	if (WEIGHT_MULT(obj) != 100 && container->item_type != ITEM_CHEST)
	{
	    send_to_char("Ты чувствуешь, что это плохая идея.\n\r", ch);
	    return;
	}

	if (get_obj_weight(obj) + get_true_weight(container) > (container->value[0] * 10)
	    ||  get_obj_weight(obj) > 10 * container->value[3])
	{
	    send_to_char("Это не влезает туда.\n\r", ch);
	    return;
	}

	if (check_container_limit(container, obj))
	{
	    send_to_char("Такая вещь уже есть там.\n\r", ch);
	    return;
	}

	if (container->item_type == ITEM_CHEST 
	&& (is_limit(obj) != -1 || container_have_limit(obj)))
	{
	    send_to_char("Ты не можешь положить лимит или контейнер с лимитом в сундук!\n\r", ch);
	    return;
	}

        
        if (IS_SET(obj->extra_flags, ITEM_NOT_PUTABLE))
        {
	    send_to_char("Эту вещь нельзя класть в контейнеры.\n\r", ch);
	    return;
        }

	put_obj(ch, obj, container);

	if (container->item_type == ITEM_CHEST && container->in_room != NULL)
	    save_chest(container->id);	/* Сохраняем сундук в файл */
    }
    else
    {
	int count = 0;
	/* 'put all container' or 'put all.obj container' */
	for (obj = ch->carrying; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;

	    if (container->item_type == ITEM_MORTAR && !check_mortar_put(container))
	    {
		send_to_char("В ступу больше нельзя ничего положить.\n\r", ch);
		return;
    	    }

	    if ((arg1[3] == '\0' || is_name(&arg1[4], obj->name))
		&&   can_see_obj(ch, obj)
		&&   WEIGHT_MULT(obj) == 100
		&&   obj->wear_loc == WEAR_NONE
		&&   obj != container
		&&   can_drop_obj(ch, obj)
		&&   get_obj_weight(obj) + get_true_weight(container) <= (container->value[0] * 10)
		&&   get_obj_weight(obj) <= 10 * container->value[3]
		&&   obj->item_type != ITEM_CONTAINER
		&&   obj->item_type != ITEM_MORTAR
		&&   obj->item_type != ITEM_CORPSE_NPC
		&&   obj->item_type != ITEM_CORPSE_PC
		&&   obj->item_type != ITEM_CHEST
		&&   !IS_SET(obj->extra_flags, ITEM_NOT_PUTABLE)
		&&   !check_container_limit(container, obj))
	    {
		if (container->item_type == ITEM_CHEST 
		&& (is_limit(obj) != -1 || container_have_limit(obj)))
		    continue;

		count += put_obj(ch, obj, container);
	    }
	}

	if (count == 0)
	{
	    act("Тебе ничего не удалось положить $T $p6.", ch, container, 
	    IS_SET(container->value[1], CONT_PUT_ON) ? "на" : "в", TO_CHAR);
	}
	else
	{
	    if (container->item_type == ITEM_CHEST && container->in_room != NULL)
		 save_chest(container->id);	// Сохраняем сундук в файл
	}
    }

    return;
}

void get_good_objs(CHAR_DATA *ch);

void do_drop(CHAR_DATA *ch, char *argument){
    char arg[MAX_INPUT_LENGTH], arg1[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    bool found;
    ROOM_INDEX_DATA *room = ch->in_room, *old_room = ch->in_room;
    bool is_portal = FALSE;
    OBJ_DATA *portal = NULL;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0'){
		send_to_char("Бросить что?\n\r", ch);
		return;
    }

    if (is_number(arg)){
		/* 'drop NNNN coins' */
		int amount, gold = 0, silver = 0, bronze = 0;

		amount   = atoi(arg);
		argument = one_argument(argument, arg);
		if (amount <= 0
			|| (str_cmp(arg, "coins") && str_cmp(arg, "coin")
			&& str_cmp(arg, "gold") && str_cmp(arg, "silver")
			&& str_cmp(arg, "bronze") && str_cmp(arg, "бронзы")
			&& str_cmp(arg, "монеты") && str_cmp(arg, "монет")
			&& str_cmp(arg, "золота") && str_cmp(arg, "серебра"))){

			send_to_char("Извини, ты не можешь этого.\n\r", ch);
			return;
		}

		if ((str_cmp(arg, "bronze") && str_cmp(arg, "бронзы")) && IS_NPC(ch)) return;


		if (!str_cmp(arg, "coins") || !str_cmp(arg, "coin") || !str_cmp(arg, "silver") || !str_cmp(arg, "монет") || !str_cmp(arg, "серебра")){
			if (ch->silver < amount){
				send_to_char("У тебя нет столько серебра.\n\r", ch);
				return;
			}
			ch->silver -= amount;
			silver = amount;
		}

		if (!str_cmp(arg, "gold") || !str_cmp(arg, "золота")){
			if (ch->gold < amount){
				send_to_char("У тебя нет столько золота.\n\r", ch);
				return;
			}
			ch->gold -= amount;
			gold = amount;
		}

		if (!str_cmp(arg, "bronze") || !str_cmp(arg, "бронзы")){
			if (!IS_NPC(ch)) {
				if (ch->pcdata->bronze < amount){
					send_to_char("У тебя нет столько бронзы.\n\r", ch);
					return;
				}
			//	send_to_char("Бросание бронзы временно отключено.\n\r", ch);
			//	return;
				ch->pcdata->bronze -= amount;
				bronze = amount;
				}
			}

		for (obj = ch->in_room->contents; obj != NULL; obj = obj_next){
			obj_next = obj->next_content;

			switch (obj->pIndexData->vnum){
			case OBJ_VNUM_SILVER_ONE:
				if (!str_cmp(arg, "gold") || !str_cmp(arg, "silver") || !str_cmp(arg, "золота") || !str_cmp(arg, "серебра")){
					silver += 1;
					extract_obj(obj, FALSE, FALSE);
				}
				break;

			case OBJ_VNUM_BRONZE_ONE:
				if (!str_cmp(arg, "bronze") || !str_cmp(arg, "бронзы")){
					bronze += 1;
					extract_obj(obj, FALSE, FALSE);
				}
				break;

			case OBJ_VNUM_GOLD_ONE:
				if (!str_cmp(arg, "gold") || !str_cmp(arg, "silver") || !str_cmp(arg, "золота") || !str_cmp(arg, "серебра")){
					gold += 1;
					extract_obj(obj, FALSE, FALSE);
				}
				break;

			case OBJ_VNUM_SILVER_SOME:
				if (!str_cmp(arg, "gold") || !str_cmp(arg, "silver") || !str_cmp(arg, "золота") || !str_cmp(arg, "серебра")){
					silver += obj->value[0];
					extract_obj(obj, FALSE, FALSE);
				}
				break;

			case OBJ_VNUM_BRONZE_SOME:
				if (!str_cmp(arg, "bronze") || !str_cmp(arg, "бронзы")){
					bronze += obj->value[0];
					extract_obj(obj, FALSE, FALSE);
				}
				break;

			case OBJ_VNUM_GOLD_SOME:
				if (!str_cmp(arg, "gold") || !str_cmp(arg, "silver") || !str_cmp(arg, "золота") || !str_cmp(arg, "серебра")){
					gold += obj->value[1];
					extract_obj(obj, FALSE, FALSE);
				}
				break;

			case OBJ_VNUM_COINS:
				if (!str_cmp(arg, "gold") || !str_cmp(arg, "silver") || !str_cmp(arg, "золота") || !str_cmp(arg, "серебра")){
					silver += obj->value[0];
					gold += obj->value[1];
					extract_obj(obj, FALSE, FALSE);
				}
				break;
			}
		}

		obj_to_room(create_money(gold, silver, bronze), room);
		act("$n бросает несколько монет.", ch, NULL, NULL, TO_ROOM);
		send_to_char("OK.\n\r", ch);
		return;
    }

    argument = one_argument(argument, arg1);

    if (!str_cmp(arg1, "в") || !str_cmp(arg1, "in"))
    {
	argument = one_argument(argument, arg1);

	if (arg1[0] == '\0'
	    || (portal = get_obj_list(ch, arg1, ch->in_room->contents)) == NULL)
	{
	    send_to_char("Здесь нет такого портала!\n\r", ch);
	    return;
	}

	if (portal->item_type != ITEM_PORTAL
	    || (IS_SET(portal->value[1], EX_CLOSED) && !IS_TRUSTED(ch, ANGEL)))
	{
	    send_to_char("Ты не видишь способа бросить что-либо туда.\n\r", ch);
	    return;
	}

	if ((IS_SET(portal->value[2], GATE_RANDOM) || portal->value[3] == -1)
	    || (IS_SET(portal->value[2], GATE_BUGGY) && number_percent() < 5))
	{
	    room = get_random_room(ch);
	}
	else
	    room = get_room_index(portal->value[3]);

	if (room == NULL
	    || room == old_room
	    || !can_see_room(ch, room))
	{
	    act("Туда нельзя бросить что-либо.", ch, NULL, NULL, TO_CHAR);
	    return;
	}

	is_portal = TRUE;
    }

    if (str_cmp(arg, "all")
	&& str_prefix("all.", arg)
	&& str_cmp(arg, "все")
	&& str_prefix("все.", arg))
    {
	/* 'drop obj' */
	if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
	{
	    send_to_char("У тебя нет этого.\n\r", ch);
	    return;
	}

	if (!can_drop_obj(ch, obj))
	{
	    send_to_char("Ты не можешь это бросить.\n\r", ch);
	    return;
	}

	obj_from_char(obj, TRUE);
	obj_to_room(obj, room);

	if (is_portal)
	{
	    act("$n бросает $p6 в $P6.", ch, obj, portal, TO_ROOM);
	    act("Ты бросаешь $p6 в $P6.", ch, obj, portal, TO_CHAR);
	    ch->in_room = room;
	    act("$p вываливается из $P1!", ch, obj, portal, TO_ROOM);
	    ch->in_room = old_room;
	}
	else
	{
	    act("$n бросает $p6.", ch, obj, NULL, TO_ROOM);
	    act("Ты бросаешь $p6.", ch, obj, NULL, TO_CHAR);
	}

	if (obj->item_type == ITEM_CHEST && obj->in_room != NULL)
	    save_chest(obj->id);	// Сохраняем сундук в файл

	if (has_trigger(obj->pIndexData->progs, TRIG_DROP, EXEC_AFTER|EXEC_DEFAULT))
	    p_give_trigger(NULL, obj, NULL, ch, obj, TRIG_DROP);

	if (has_trigger(room->progs, TRIG_DROP, EXEC_AFTER|EXEC_DEFAULT))
	    p_give_trigger(NULL, NULL, room, ch, obj, TRIG_DROP);

	if (obj && IS_OBJ_STAT(obj, ITEM_MELT_DROP))
	{
	    act("$p исчеза$r в облачке дыма.", ch, obj, NULL, TO_ALL);
	    extract_obj(obj, TRUE, FALSE);
	}
    }
    else
    {
	/* 'drop all' or 'drop all.obj' */
	found = FALSE;

	for (obj = ch->carrying; obj != NULL; obj = obj_next){
	    obj_next = obj->next_content;

	    if ((arg[3] == '\0' || is_name(&arg[4], obj->name))	&& can_see_obj(ch, obj) && obj->wear_loc == WEAR_NONE && can_drop_obj(ch, obj)){
			found = TRUE;
			obj_from_char(obj, TRUE);
			obj_to_room(obj, room);

			if (arg[3] != '\0'){
				if (is_portal){
					act("$n бросает $p6 в $P6.", ch, obj, portal, TO_ROOM);
					act("Ты бросаешь $p6 в $P6.", ch, obj, portal, TO_CHAR);
					ch->in_room = room;
					act("$p вываливается из $P1!",
					ch, obj, portal, TO_ROOM);
					ch->in_room = old_room;
				} else {
					act("$n бросает $p6.", ch, obj, NULL, TO_ROOM);
					act("Ты бросаешь $p6.", ch, obj, NULL, TO_CHAR);
				}
			}

			if (obj->item_type == ITEM_CHEST && obj->in_room != NULL)
				save_chest(obj->id);	// Сохраняем сундук в файл

			if (has_trigger(obj->pIndexData->progs, TRIG_DROP, EXEC_AFTER|EXEC_DEFAULT))
				p_give_trigger(NULL, obj, NULL, ch, obj, TRIG_DROP);

			if (has_trigger(room->progs, TRIG_DROP, EXEC_AFTER|EXEC_DEFAULT))
				p_give_trigger(NULL, NULL, room, ch, obj, TRIG_DROP);

			if (obj && IS_OBJ_STAT(obj, ITEM_MELT_DROP)){
				act("$p исчеза$r в облачке дыма.", ch, obj, NULL, TO_ALL);
				extract_obj(obj, TRUE, FALSE);
			}
	    }
	}

	if (!found){
	    if (arg[3] == '\0')
		act("Ты ничего не несешь.",
		    ch, NULL, arg, TO_CHAR);
	    else
		act("Ты не несешь ни одной вещи с названием '$T'.",
		    ch, NULL, &arg[4], TO_CHAR);
	} else {
		if (arg[3] == '\0'){
			act("$n бросает $x весь свой инвентарь.",
				ch, NULL, NULL, TO_ROOM);
			act("Ты бросаешь весь свой инвентарь $x.", ch, NULL, NULL, TO_CHAR);
		}
		}
    }

#if 0
    for (vch = ch->in_room->people; vch; vch = vch_next) {
		vch_next = vch->next_in_room;

		if (!IS_NPC(vch) || vch->fighting == NULL)
	    	continue;

		get_good_objs(vch);
    }
#endif

    return;
}



void do_give(CHAR_DATA *ch, char *argument)
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA  *obj;
    bool gift = FALSE;

    argument = one_argument(argument, arg1);

    if (!str_cmp(arg1, "gift"))
    {
	gift = TRUE;
	argument = one_argument(argument, arg1);
    }

    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0')
    {
	sprintf(buf, "%s что кому?\n\r", gift ? "Подарить" : "Дать");
	send_to_char(buf, ch);
	return;
    }

    if (is_number(arg1))
    {
		/* 'give NNNN coins victim' */
		int amount;
		bool silver = 0;

		amount   = atoi(arg1);
		if (amount <= 0
	    	|| (str_cmp(arg2, "coins") && str_cmp(arg2, "coin") &&
			str_cmp(arg2, "gold") && str_cmp(arg2, "silver")&&
			str_cmp(arg2, "монет") &&
			str_cmp(arg2, "золота") && 
			str_cmp(arg2, "серебра") &&
			str_cmp(arg2, "бронзы") &&
			str_cmp(arg2, "bronze")))
		{
	    	send_to_char("Извини, ты не можешь этого.\n\r", ch);
	    	return;
		}

		argument = one_argument(argument, arg3);
		if (arg3[0] == '\0')
		{
	    	sprintf(buf, "%s что кому?\n\r", gift ? "Подарить" : "Дать");
	    	send_to_char(buf, ch);
	    	return;
		}

		if ((victim = get_char_room(ch, NULL, arg3, FALSE)) == NULL)
		{
	    	send_to_char("Таких здесь нет.\n\r", ch);
	    	return;
		}

		if (ch == victim)
		{
	    	send_to_char("Хочешь дать деньги себе?\n\r", ch);
	    	return;
		}

		if (victim->position == POS_SLEEPING)
		{
	    	act("$N спит, поэтому не может взять то, что ты даешь.", ch, NULL, victim, TO_CHAR);
	    	return;
		}

		if (!can_see(victim, ch))
		{
	    	act("$N не видит тебя, и то, что ты хочешь дать.", ch, NULL, victim, TO_CHAR);
	    	return;
		}

		if (!str_cmp(arg2, "bronze") || !str_cmp(arg2, "бронзы"))
		{
			if (!IS_NPC(ch))
			{
				if (ch->pcdata->bronze < amount)
				{
	    			send_to_char("У тебя столько нет.\n\r", ch);
	    			return;
				}
				if (!IS_NPC(victim))
				{
	    			ch->pcdata->bronze		-= amount;
	    			victim->pcdata->bronze	+= amount;
				}
    			else
				{
	    			send_to_char("Мобам бронза ни к чему.\n\r", ch);
	    			return;
				}				
			}

			sprintf(buf, "$n %s тебе %d %s.", gift ? "дарит" : "дает", amount,  "бронзы");
			act(buf, ch, NULL, victim, TO_VICT);
			act("$n дает $N2 несколько монет.",  ch, NULL, victim, TO_NOTVICT);
			sprintf(buf, "Ты %s $N2 %d %s.", gift ? "даришь" : "даешь", amount, "бронзы");
			act(buf, ch, NULL, victim, TO_CHAR);
		}
		else
		{
			silver = (str_cmp(arg2, "gold") && str_cmp(arg2, "золота"));
			if ((!silver && ch->gold < amount) || (silver && ch->silver < amount))
			{
	    		send_to_char("У тебя столько нет.\n\r", ch);
	    		return;
			}

			if (silver)
			{
	    		if (!can_take_weight(victim, get_money_weight(0, amount)))
	    		{
					act("$N не может нести столько тяжести.", ch, NULL, victim, TO_CHAR);
					return;
	    		}
	    		ch->silver		-= amount;
	    		victim->silver 	+= amount;
			}
			else
			{
	    		if (!can_take_weight(victim, get_money_weight(amount, 0)))
	    		{
					act("$N не может нести столько тяжести.", ch, NULL, victim, TO_CHAR);
					return;
	    		}
	    		ch->gold		-= amount;
	    		victim->gold	+= amount;
			}

			sprintf(buf, "$n %s тебе %d %s.", gift ? "дарит" : "дает", amount, silver ? "серебра" : "золота");
			act(buf, ch, NULL, victim, TO_VICT);
			act("$n дает $N2 несколько монет.",  ch, NULL, victim, TO_NOTVICT);
			sprintf(buf, "Ты %s $N2 %d %s.", gift ? "даришь" : "даешь", amount, silver ? "серебра" : "золота");
			act(buf, ch, NULL, victim, TO_CHAR);
		}

		/*
	 	* Bribe trigger
	 	*/
		if (IS_NPC(victim) && has_trigger(victim->pIndexData->progs, TRIG_BRIBE, EXEC_AFTER|EXEC_DEFAULT))
	    	p_bribe_trigger(victim, ch, silver ? amount : amount * 100);

		if (IS_NPC(victim) && IS_SET(victim->act, ACT_IS_CHANGER))
		{
	    	int change;

	    	change = (silver ? 95 * amount / 100 / 100
		      	: 95 * amount);


	    	if (!silver && change > victim->silver)
			victim->silver += change;

	    	if (silver && change > victim->gold)
			victim->gold += change;

	    	if (gift)
	    	{
			do_say(victim, "Спасибо большое!");
			return;
	    	}

	    	if (change < 1 && can_see(victim, ch))
	    	{
			act("$n говорит тебе: Извини, ты даешь мне недостаточно денег для обмена."
		    	, victim, NULL, ch, TO_VICT);
			ch->reply = victim;
			sprintf(buf, "%d %s %s",
				amount, silver ? "серебра" : "золота", ch->name);
			do_function(victim, &do_give, buf);
	    	}
	    	else if (can_see(victim, ch))
	    	{
			sprintf(buf, "%d золота %s",
				change, ch->name);
			do_function(victim, &do_give, buf);
			if (silver)
			{
		    	sprintf(buf, "%d серебра %s",
			    	(95 * amount / 100 - change * 100), ch->name);
		    	do_function(victim, &do_give, buf);
			}
			act("$n говорит тебе: {RСпасибо, приходи еще.{x",
		    	victim, NULL, ch, TO_VICT);
			ch->reply = victim;
	    	}
	    	victim->silver = 0;
	    	victim->gold = 0;
		}
		return;
    }

    if ((obj = get_obj_carry(ch, arg1, ch)) == NULL)
    {
	send_to_char("У тебя этого нет.\n\r", ch);
	return;
    }

    if (obj->wear_loc != WEAR_NONE)
    {
	send_to_char("Надо сначала снять это.\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, NULL, arg2, FALSE)) == NULL)
    {
		send_to_char("Таких здесь нет.\n\r", ch);
		return;
    }

    if (ch == victim)
    {
		send_to_char("Хочешь дать вещь себе?\n\r", ch);
		return;
    }

    if (IS_NPC(victim) && victim->pIndexData->pShop != NULL)
    {
		act("$N говорит тебе: {RИзвини, тебе надо продать это.{x",
	    	ch, NULL, victim, TO_CHAR);
		ch->reply = victim;
		return;
    }

    if (!can_drop_obj(ch, obj))
    {
		send_to_char("Ты не можешь с этим расстаться.\n\r", ch);
		return;
    }

    if (victim->carry_number + get_obj_number(obj) > can_carry_n(victim))
    {
		if (!IS_NPC(victim))
	    	act("У $N1 полные руки.", ch, NULL, victim, TO_CHAR);
		else
	    	act("$N не может взять это.", ch, NULL, victim, TO_CHAR);

		return;
    }

    if (!can_take_weight(victim, get_obj_weight(obj)))
    {
		act("$N не может нести столько тяжести.", ch, NULL, victim, TO_CHAR);
		return;
    }

    if (victim->position == POS_SLEEPING)
    {
		act("$N спит, поэтому не может взять то, что ты даешь.", ch, NULL, victim, TO_CHAR);
		return;
    }

    if (!can_see(victim, ch))
    {
		act("$N не видит тебя, и то, что ты хочешь дать.", ch, NULL, victim, TO_CHAR);
		return;
    }

    if (!can_see_obj(victim, obj))
    {
		act("$N не видит этого.", ch, NULL, victim, TO_CHAR);
		return;
    }

    if (is_have_limit(victim, ch, obj))
	return;

    obj_from_char(obj, TRUE);
    obj_to_char(obj, victim);
    MOBtrigger = FALSE;
    sprintf(buf, "$n %s $p6 $N2.", gift ? "дарит" : "дает");
    act(buf, ch, obj, victim, TO_NOTVICT);
    sprintf(buf, "$n %s тебе $p6.", gift ? "дарит" : "дает");
    act(buf,   ch, obj, victim, TO_VICT);
    sprintf(buf, "Ты %s $p6 $N2.", gift ? "даришь" : "даешь");
    act(buf, ch, obj, victim, TO_CHAR);
    MOBtrigger = TRUE;

    /*
     * Give trigger
     */
    if (has_trigger(obj->pIndexData->progs, TRIG_GIVE, EXEC_AFTER|EXEC_DEFAULT))
	p_give_trigger(NULL, obj, NULL, ch, obj, TRIG_GIVE);
    if (has_trigger(ch->in_room->progs, TRIG_GIVE, EXEC_AFTER|EXEC_DEFAULT))
	p_give_trigger(NULL, NULL, ch->in_room, ch, obj, TRIG_GIVE);

    if (IS_NPC(victim) && has_trigger(victim->pIndexData->progs, TRIG_GIVE, EXEC_AFTER|EXEC_DEFAULT))
	p_give_trigger(victim, NULL, NULL, ch, obj, TRIG_GIVE);

    return;
}

void do_gift(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH + 10];

    sprintf(arg, "gift %s", argument);
    do_give(ch, arg);
}

/* for poisoning weapons and food/drink */
void do_envenom(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj;
    AFFECT_DATA af;
    int percent, skill;

    /* find out what */
    if (argument[0] == '\0')
    {
	send_to_char("Отравить что?\n\r", ch);
	return;
    }

    obj =  get_obj_list(ch, argument, ch->carrying);

    if (obj== NULL)
    {
	send_to_char("У тебя нет этого.\n\r", ch);
	return;
    }

    if ((skill = get_skill(ch, gsn_envenom)) < 1)
    {
	send_to_char("Ты спятил? Ты отравишь себя!\n\r", ch);
	return;
    }

    if (obj->item_type == ITEM_FOOD || obj->item_type == ITEM_DRINK_CON)
    {
	if (IS_OBJ_STAT(obj, ITEM_BLESS) || IS_OBJ_STAT(obj, ITEM_BURN_PROOF))
	{
	    act("У тебя не получается отравить $p6.", ch, obj, NULL, TO_CHAR);
	    return;
	}

	if (number_percent() < skill)  /* success! */
	{
	    act("$n отравляет $p6.", ch, obj, NULL, TO_ROOM);
	    act("Ты подбрасываешь в $p6 смертельный яд.", ch, obj, NULL, TO_CHAR);
	    if (obj->value[3] <= 0)
	    {
		obj->value[3] = ch->level;
		check_improve(ch, NULL, gsn_envenom, TRUE, 4);
	    }
	    WAIT_STATE(ch, skill_table[gsn_envenom].beats);
	    return;
	}

	act("У тебя не получается отравить $p6.", ch, obj, NULL, TO_CHAR);
	if (!obj->value[3])
	    check_improve(ch, NULL, gsn_envenom, FALSE, 4);
	WAIT_STATE(ch, skill_table[gsn_envenom].beats);
	return;
    }

    if (obj->item_type == ITEM_WEAPON)
    {
	if (IS_WEAPON_STAT(obj, WEAPON_FLAMING)
	    ||  IS_WEAPON_STAT(obj, WEAPON_FROST)
	    ||  IS_WEAPON_STAT(obj, WEAPON_VAMPIRIC)
	    ||  IS_WEAPON_STAT(obj, WEAPON_MANAPIRIC)
	    ||  IS_WEAPON_STAT(obj, WEAPON_SHARP)
	    ||  IS_WEAPON_STAT(obj, WEAPON_SHOCKING)
	    ||  IS_OBJ_STAT(obj, ITEM_BLESS) || IS_OBJ_STAT(obj, ITEM_BURN_PROOF))
	{
	    act("Ты не видишь способа отравить $p6.", ch, obj, NULL, TO_CHAR);
	    return;
	}

	if (obj->value[3] < 0
	    ||  attack_table[obj->value[3]].damage == DAM_BASH)
	{
	    send_to_char("Ты можешь отравить только острые оружия.\n\r", ch);
	    return;
	}

	if (IS_WEAPON_STAT(obj, WEAPON_POISON))
	{
	    act("$p - уже отравлено.", ch, obj, NULL, TO_CHAR);
	    return;
	}

	percent = number_percent();
	if (percent < skill)
	{

	    af.where     = TO_WEAPON;
	    af.type      = gsn_poison;
	    af.level     = ch->level * (percent + skill) / 100;
	    af.duration  = ch->level * (percent + skill) / 100;
	    af.location  = 0;
	    af.modifier  = 0;
	    af.bitvector = WEAPON_POISON;
	    affect_to_obj(obj, &af);

	    act("$n покрывает $p6 смертельным ядом.", ch, obj, NULL, TO_ROOM);
	    act("Ты покрываешь $p6 смертельным ядом.", ch, obj, NULL, TO_CHAR);
	    check_improve(ch, NULL, gsn_envenom, TRUE, 3);
	    WAIT_STATE(ch, skill_table[gsn_envenom].beats);
	    return;
	}
	else
	{
	    act("У тебя не получается отравить $p6.", ch, obj, NULL, TO_CHAR);
	    check_improve(ch, NULL, gsn_envenom, FALSE, 3);
	    WAIT_STATE(ch, skill_table[gsn_envenom].beats);
	    return;
	}
    }

    act("Ты не можешь отравить $p6.", ch, obj, NULL, TO_CHAR);
    return;
}

void do_fill(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *fountain = NULL;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Наполнить что?\n\r", ch);
	return;
    }

    if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
    {
	send_to_char("У тебя этого нет.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	for (fountain = ch->in_room->contents; fountain != NULL;
	     fountain = fountain->next_content)
	    if (fountain->item_type == ITEM_FOUNTAIN)
		break;

	if (!fountain)
	{
	    send_to_char("Здесь нет источника жидкости!\n\r", ch);
	    return;
	}

    }
    else if ((fountain = get_obj_list(ch, argument, ch->in_room->contents)) == NULL)
    {
	send_to_char("Здесь нет этого.\n\r", ch);
	return;
    }

    if (fountain->item_type != ITEM_FOUNTAIN)
    {
	send_to_char("Это не является источником жидкости.\n\r", ch);
	return;
    }

    if (obj->item_type != ITEM_DRINK_CON)
    {
	send_to_char("Ты не можешь наполнить это.\n\r", ch);
	return;
    }

    if (obj->value[1] != 0 && obj->value[2] != fountain->value[2])
    {
	send_to_char("Тут уже есть другая жидкость.\n\r", ch);
	return;
    }

    if (obj->value[1] >= obj->value[0])
    {
	send_to_char("Твой контейнер полон.\n\r", ch);
	return;
    }

    sprintf(buf, "Ты наполняешь $p6 из $P1. Там %s.", liq_table[fountain->value[2]].liq_rname);
    act(buf, ch, obj, fountain, TO_CHAR);
    sprintf(buf, "$n наполняет $p6 из $P1.");
    act(buf, ch, obj, fountain, TO_ROOM);
    obj->value[2] = fountain->value[2];
    obj->value[1] = obj->value[0];
    return;
}

void do_pour (CHAR_DATA *ch, char *argument)
{
    char arg[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
    OBJ_DATA *out, *in;
    CHAR_DATA *vch = NULL;
    int amount;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
	send_to_char("Перелить что во что?\n\r", ch);
	return;
    }


    if ((out = get_obj_carry(ch, arg, ch)) == NULL)
    {
	send_to_char("У тебя нет этого.\n\r", ch);
	return;
    }

    if (out->item_type != ITEM_DRINK_CON)
    {
	send_to_char("Этот контейнер не для жидкости.\n\r", ch);
	return;
    }

    if (!str_cmp(argument, "out") || !str_prefix(argument, "наземлю"))
    {
	if (out->value[1] == 0)
	{
	    send_to_char("Этот контейнер уже пуст.\n\r", ch);
	    return;
	}

	out->value[1] = 0;
	out->value[3] = 0;
	act("Ты переворачиваешь $p6, $T выливается $x.",
	    ch, out, liq_table[out->value[2]].liq_rname, TO_CHAR);
	act("$n переворачивает $p6, $T выливается $x.",
	    ch, out, liq_table[out->value[2]].liq_rname, TO_ROOM);
	return;
    }

    if ((in = get_obj_here(ch, NULL, argument)) == NULL)
    {
	vch = get_char_room(ch, NULL, argument, FALSE);

	if (vch == NULL)
	{
	    send_to_char("Перелить во что?\n\r", ch);
	    return;
	}

	in = get_eq_char(vch, WEAR_HOLD);

	if (in == NULL)
	{
	    send_to_char("Кому ты хотел перелить? Он же ничего не держит в руках!", ch);
	    return;
	}
    }

    if (in->item_type != ITEM_DRINK_CON)
    {
	send_to_char("Ты можешь переливать только в другие контейнеры для жидкостей.\n\r", ch);
	return;
    }

    if (in == out)
    {
	send_to_char("Ты хочешь изменить законы физики?\n\r", ch);
	return;
    }

    if (in->value[1] != 0 && in->value[2] != out->value[2])
    {
	send_to_char("Куда ты желаешь перелить? Там же другая жидкость!\n\r", ch);
	return;
    }

    if (out->value[1] == 0)
    {
	act("Здесь пусто, нечего переливать.", ch, out, NULL, TO_CHAR);
	return;
    }

    if (in->value[1] >= in->value[0])
    {
	act("$p уже заполнен доверху.", ch, in, NULL, TO_CHAR);
	return;
    }

    amount = UMIN(out->value[1], in->value[0] - in->value[1]);

    in->value[1] += amount;
    out->value[1] -= amount;
    in->value[2] = out->value[2];

    if (vch == NULL)
    {
	sprintf(buf, "Ты переливаешь %s из $p1 в $P6.",
		cases(liq_table[out->value[2]].liq_rname, 6));
	act(buf, ch, out, in, TO_CHAR);
	sprintf(buf, "$n переливает %s из $p1 в $P6.",
		cases(liq_table[out->value[2]].liq_rname, 6));
	act(buf, ch, out, in, TO_ROOM);
    }
    else
    {
	sprintf(buf, "Ты наливаешь немного %s для $N1.",
		cases(liq_table[out->value[2]].liq_rname, 1));
	act(buf, ch, NULL, vch, TO_CHAR);
	sprintf(buf, "$n наливает тебе немного %s.",
		cases(liq_table[out->value[2]].liq_rname, 1));
	act(buf, ch, NULL, vch, TO_VICT);
	sprintf(buf, "$n наливает немного %s для $N1.",
		cases(liq_table[out->value[2]].liq_rname, 1));
	act(buf, ch, NULL, vch, TO_NOTVICT);

    }
}

void do_drink(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MSL];
    OBJ_DATA *obj;
    int amount;
    int liquid;
    AFFECT_DATA af;

    if (ch->fighting)
    {
	send_to_char("Куда? Ведь ты еще сражаешься!\n\r", ch);
	return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	for (obj = ch->in_room->contents; obj; obj = obj->next_content)
	{
	    if (obj->item_type == ITEM_FOUNTAIN && can_see_obj(ch, obj))
		break;
	}

	if (obj == NULL)
	{
	    if (SECTOR(ch) == SECT_WATER_SWIM || SECTOR(ch) == SECT_WATER_NOSWIM)
		send_to_char("Не стоит пить воду из неизвестно каких источников.\n\r", ch);
	    else
		send_to_char("Выпить что?\n\r", ch);
	    return;
	}
    }
    else
    {
	if ((obj = get_obj_here(ch, NULL, arg)) == NULL)
	{
	    send_to_char("Ты не находишь это.\n\r", ch);
	    return;
	}
    }

    if (IS_DRUNK(ch))
    {
	if (CAN_WEAR(obj, ITEM_TAKE))
	    send_to_char("Ты промахиваешься мимо рта.  *Ик*\n\r", ch);
	else
	    send_to_char("Аккуратнее! Можно так и головой об землю стукнуться.  *Ик*\n\r", ch);
	return;
    }

    switch (obj->item_type)
    {
    default:
	send_to_char("Ты не можешь пить из этого.\n\r", ch);
	return;

    case ITEM_FOUNTAIN:
	if ((liquid = obj->value[2])  < 0)
	{
	    bugf("Do_drink: bad liquid number %d.", liquid);
	    liquid = obj->value[2] = 0;
	}
	amount = liq_table[liquid].liq_affect[4] * 3;
	break;

    case ITEM_DRINK_CON:
	if (obj->value[1] <= 0)
	{
	    send_to_char("Здесь уже пусто.\n\r", ch);
	    return;
	}

	if ((liquid = obj->value[2])  < 0)
	{
	    bugf("Do_drink: bad liquid number %d.", liquid);
	    liquid = obj->value[2] = 0;
	}

	amount = liq_table[liquid].liq_affect[4];
	amount = UMIN(amount, obj->value[1]);
	break;
    }
    if (!IS_NPC(ch) && !IS_IMMORTAL(ch)
	&& ch->pcdata->condition[COND_FULL] > 45)
    {
	send_to_char("Тебе не выпить больше ни капли.\n\r", ch);
	return;
    }

    sprintf(buf, "$n пьет %s из $p1.", cases(liq_table[liquid].liq_rname, 6));
    act_wear(buf, ch, obj);
    act("Ты пьешь $T6 из $p1.", ch, obj, liq_table[liquid].liq_rname, TO_CHAR);

    gain_condition(ch, COND_DRUNK,
		   amount * liq_table[liquid].liq_affect[COND_DRUNK] / 36);

    if (!is_affected(ch, skill_lookup("hunger")))
    {
	gain_condition(ch, COND_FULL,
		       amount * liq_table[liquid].liq_affect[COND_FULL] / 4);
	gain_condition(ch, COND_THIRST,
		       amount * liq_table[liquid].liq_affect[COND_THIRST] / 10);
	gain_condition(ch, COND_HUNGER,
		       amount * liq_table[liquid].liq_affect[COND_HUNGER] / 2);

	if (!IS_NPC(ch) && ch->pcdata->condition[COND_FULL]   > 40)
	    send_to_char("Тебе не выпить больше ни капли.\n\r", ch);
	if (!IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] > 40)
	    send_to_char("Ты утоляешь жажду.\n\r", ch);
    }

    if (IS_DRUNK(ch))
	send_to_char("Ты чувствуешь себя пьяно.\n\r", ch);

    if (!IS_VAMPIRE(ch))
    {
	if (obj->value[3] != 0 && !saves_spell(obj->value[3], ch, DAM_POISON))
	{
	    /* The drink was poisoned ! */

	    act("$n давится и задыхается.", ch, NULL, NULL, TO_ROOM);
	    send_to_char("Ты давишься и задыхаешься.\n\r", ch);
	    af.where     = TO_AFFECTS;
	    af.type      = gsn_poison;
	    af.level	 = number_fuzzy(amount);
	    af.duration  = 3 * amount;
	    af.location  = APPLY_NONE;
	    af.modifier  = 0;
	    af.bitvector = AFF_POISON;
	    af.caster_id = 0;
	    affect_join(ch, &af);
	}
    }
    else if (IS_NOT_SATIETY(ch)
	     && !str_cmp(liq_table[liquid].liq_name, "blood"))
    {
	send_to_char("Ты слегка насыщаешься кровью. Но все же охота "
		     "свежатинки...\n\r", ch);

	af.where = TO_AFFECTS;
	af.type = gsn_bite;
	af.level = 0;
	af.location = APPLY_NONE;
	af.duration = 0;
	af.modifier = 0;
	af.bitvector = AFF_SATIETY;
	af.caster_id = 0;
	affect_to_char(ch, &af);
    }

    if (obj->value[0] > 0)
	obj->value[1] -= amount;

    return;
}



void do_eat(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int i;

    one_argument(argument, arg);
    if (arg[0] == '\0')
    {
	send_to_char("Сьесть что?\n\r", ch);
	return;
    }

    if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
    {
	send_to_char("У тебя нет этого.\n\r", ch);
	return;
    }

    if (!IS_IMMORTAL(ch))
    {
	if (obj->item_type != ITEM_FOOD && obj->item_type != ITEM_PILL)
	{
	    send_to_char("Это несъедобно.\n\r", ch);
	    return;
	}

	if (!IS_NPC(ch) && ch->pcdata->condition[COND_FULL] > 40)
	{
	    send_to_char("Тебе не съесть больше ни крошки.\n\r", ch);
	    return;
	}
    }

    if (ch->level < obj->level)
    {
	send_to_char("Тебе нужно подрасти, чтобы проглотить эту вещь.\n\r", ch);
	return;
    }

    if (ch->fighting && obj->item_type == ITEM_FOOD)
    {
	send_to_char("Куда? Ведь ты еще сражаешься!\n\r", ch);
	return;
    }

    act("$n ест $p6.",  ch, obj, NULL, TO_SOUND/*TO_ROOM*/);
    act("Ты ешь $p6.", ch, obj, NULL, TO_CHAR);

    if (ch->classid == CLASS_NECROMANT && obj->pIndexData->vnum == OBJ_VNUM_TORN_HEART)
    {
	send_to_char("Ты чувстуешь себя немножко получше!\n\r", ch);
	ch->hit = UMIN(ch->hit + dice(3, 8) + ch->level, ch->max_hit);
	update_pos(ch);
    }

    switch (obj->item_type)
    {

    case ITEM_FOOD:

	if (!IS_NPC(ch) && !is_affected(ch, skill_lookup("hunger")))
	{
	    int condition;
	    condition = ch->pcdata->condition[COND_HUNGER];
	    gain_condition(ch, COND_FULL, obj->value[0]);
	    gain_condition(ch, COND_HUNGER, obj->value[1]);
	    if (condition == 0 && ch->pcdata->condition[COND_HUNGER] > 0)
		send_to_char("Ты больше не хочешь есть.\n\r", ch);
	    else if (ch->pcdata->condition[COND_FULL] > 40)
		send_to_char("Ты наедаешься до отвала.\n\r", ch);
	}

	if (obj->value[3] > 0
	    && !IS_VAMPIRE(ch)
		&& ch->race != RACE_ZOMBIE
	    && !saves_spell(obj->value[3], ch, DAM_POISON))
	{
	    /* The food was poisoned! */
	    AFFECT_DATA af;

	    act("$n давится и задыхается.", ch, 0, 0, TO_ROOM);
	    send_to_char("Ты давишься и задыхаешься.\n\r", ch);

	    af.where	 = TO_AFFECTS;
	    af.type      = gsn_poison;
	    af.level 	 = number_fuzzy(obj->value[0]);
	    af.duration  = 2 * obj->value[0];
	    af.location  = APPLY_NONE;
	    af.modifier  = 0;
	    af.bitvector = AFF_POISON;
	    af.caster_id = 0;
	    affect_join(ch, &af);
	}
	break;

    case ITEM_PILL:
	for (i = 1; i <= 4; i++)
	    obj_cast_spell(obj->value[i], obj->value[0], ch, ch, NULL, "");
	break;
    }

    extract_obj(obj, TRUE, TRUE);
    return;
}



/*
 * Remove an object.
 */
bool remove_obj(CHAR_DATA *ch, int iWear, bool fReplace)
{
    OBJ_DATA *obj;


    if ((obj = get_eq_char(ch, iWear)) == NULL)
	return TRUE;

    if (!fReplace)
	return FALSE;

    if (IS_SET(obj->extra_flags, ITEM_NOREMOVE))
    {
	act("Ты не можешь снять $p6.", ch, obj, NULL, TO_CHAR);
	return FALSE;
    }

    REMOVE_BIT(ch->affected_by, AFF_COVER);
    unequip_char(ch, obj, TRUE);
    act("$n прекращает использовать $p6.", ch, obj, NULL, TO_ROOM);
    act("Ты прекращаешь использовать $p6.", ch, obj, NULL, TO_CHAR);
    return TRUE;
}

bool check_wear_restrict(CHAR_DATA *ch, OBJ_DATA *obj)
{
    int stat;

    if (ch->level < obj->level)
    {
	send_to_char("Тебе нужно быть повыше уровнем для этой вещи.\n\r", ch);
	act_wear("$n пытается использовать $p6, но у $s1 ничего не получается.",
		 ch, obj);
	return FALSE;
    }

    if ((stat = check_wear_stat(ch, obj)) < MAX_STATS)
    {
	act("Чтобы одеть эту вещь, тебе не хватает $t1.", ch, attr_flags[stat].rname, NULL, TO_CHAR);
	act_wear("$n пытается использовать $p6, но у $s1 ничего не получается.",
		 ch, obj);
	return FALSE;
    }

    if (IS_SET(obj->extra_flags, ITEM_AUCTION))
    {
	act("Но $p уже продается на аукционе!", ch, obj, NULL, TO_CHAR);
	return FALSE;
    }

    if (is_nopk(ch) && is_limit(obj) != -1)
    {
	send_to_char("Твое мировоззрение не позволяет тебе одевать такие вещи.\n\r", ch);
	return FALSE;
    }
    
    REMOVE_BIT(ch->affected_by, AFF_COVER);
    
    return TRUE;
}

/*
 * Wear one object.
 * Optional replacement of existing objects.
 * Big repetitive code, ick.
 */
void wear_obj(CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace)
{

    if (!check_wear_restrict(ch, obj))
	return;

    if (has_trigger(obj->pIndexData->progs, TRIG_WEAR, EXEC_BEFORE))
	p_wear_trigger(obj, ch);

    if (obj->item_type == ITEM_LIGHT)
    {
	if (!remove_obj(ch, WEAR_LIGHT, fReplace))
	    return;

	act_wear("$n зажигает $p6.", ch, obj);
	act("Ты зажигаешь $p6.",  ch, obj, NULL, TO_CHAR);
	equip_char(ch, obj, WEAR_LIGHT);
    }
    else if (CAN_WEAR(obj, ITEM_WEAR_FINGER))
    {
	if (get_eq_char(ch, WEAR_FINGER_L) != NULL
	    && get_eq_char(ch, WEAR_FINGER_R) != NULL
	    && !remove_obj(ch, WEAR_FINGER_L, fReplace)
	    && !remove_obj(ch, WEAR_FINGER_R, fReplace))
	{
	    return;
	}

	if (get_eq_char(ch, WEAR_FINGER_L) == NULL)
	{
	    act_wear("$n одевает $p6 на палец левой руки.", ch, obj);
	    act("Ты одеваешь $p6 на палец левой руки.", ch, obj, NULL, TO_CHAR);
	    equip_char(ch, obj, WEAR_FINGER_L);
	}
	else if (get_eq_char(ch, WEAR_FINGER_R) == NULL)
	{
	    act_wear("$n одевает $p6 на палец правой руки.", ch, obj);
	    act("Ты одеваешь $p6 на палец правой руки.",
		ch, obj, NULL, TO_CHAR);
	    equip_char(ch, obj, WEAR_FINGER_R);
	}
	else
	{
	    bugf("Wear_obj: no free finger.");
	    send_to_char("На тебе уже два кольца.\n\r", ch);
	    return;
	}
    }
    else if (CAN_WEAR(obj, ITEM_WEAR_NECK))
    {
	if (get_eq_char(ch, WEAR_NECK_1) != NULL
	    && get_eq_char(ch, WEAR_NECK_2) != NULL
	    && !remove_obj(ch, WEAR_NECK_1, fReplace)
	    && !remove_obj(ch, WEAR_NECK_2, fReplace))
	{
	    return;
	}

	if (get_eq_char(ch, WEAR_NECK_1) == NULL)
	{
	    act_wear("$n одевает $p6 вокруг шеи.", ch, obj);
	    act("Ты одеваешь $p6 вокруг шеи.", ch, obj, NULL, TO_CHAR);
	    equip_char(ch, obj, WEAR_NECK_1);
	}
	else if (get_eq_char(ch, WEAR_NECK_2) == NULL)
	{
	    act_wear("$n одевает $p6 вокруг шеи.", ch, obj);
	    act("Ты одеваешь $p6 вокруг шеи.", ch, obj, NULL, TO_CHAR);
	    equip_char(ch, obj, WEAR_NECK_2);
	}
	else
	{
	    bugf("Wear_obj: no free neck.");
	    send_to_char("Твоя шея уже занята полностью.\n\r", ch);
	    return;
	}
    }
    else if (CAN_WEAR(obj, ITEM_WEAR_BODY))
    {
	if (!remove_obj(ch, WEAR_BODY, fReplace))
	    return;
	act_wear("$n натягивает $p6 на тело.", ch, obj);
	act("Ты натягиваешь $p6 на тело.", ch, obj, NULL, TO_CHAR);
	equip_char(ch, obj, WEAR_BODY);
    }
    else if (CAN_WEAR(obj, ITEM_WEAR_HEAD))
    {
	if (!remove_obj(ch, WEAR_HEAD, fReplace))
	    return;
	act_wear("$n напяливает $p6 на голову.", ch, obj);
	act("Ты напяливаешь $p6 на голову.", ch, obj, NULL, TO_CHAR);
	equip_char(ch, obj, WEAR_HEAD);
    }
    else if (CAN_WEAR(obj, ITEM_WEAR_LEGS))
    {
	if (!remove_obj(ch, WEAR_LEGS, fReplace))
	    return;
	act_wear("$n надевает $p6 на ноги.", ch, obj);
	act("Ты надеваешь $p6 на свои ноги.", ch, obj, NULL, TO_CHAR);
	equip_char(ch, obj, WEAR_LEGS);
    }
    else if (CAN_WEAR(obj, ITEM_WEAR_FEET))
    {
	if (!remove_obj(ch, WEAR_FEET, fReplace))
	    return;
	act_wear("$n обувается в%s $p6.", ch, obj);
	act("Ты обуваешься в $p6.", ch, obj, NULL, TO_CHAR);
	equip_char(ch, obj, WEAR_FEET);
    }
    else if (CAN_WEAR(obj, ITEM_WEAR_HANDS))
    {
	if (!remove_obj(ch, WEAR_HANDS, fReplace))
	    return;

	act_wear("$n натягивает $p6 на кисти.", ch, obj);
	act("Ты натягиваешь $p6 на кисти.", ch, obj, NULL, TO_CHAR);
	equip_char(ch, obj, WEAR_HANDS);
    }
    else if (CAN_WEAR(obj, ITEM_WEAR_ARMS))
    {
	if (!remove_obj(ch, WEAR_ARMS, fReplace))
	    return;
	act_wear("$n надевает $p6 на руки.", ch, obj);
	act("Ты надеваешь $p6 на руки.", ch, obj, NULL, TO_CHAR);
	equip_char(ch, obj, WEAR_ARMS);
    }
    else if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))
    {
	if (!remove_obj(ch, WEAR_ABOUT, fReplace))
	    return;
	act_wear("$n набрасывает $p6 вокруг тела.", ch, obj);
	act("Ты набрасываешь $p6 вокруг тела.", ch, obj, NULL, TO_CHAR);
	equip_char(ch, obj, WEAR_ABOUT);
    }
    else if (CAN_WEAR(obj, ITEM_WEAR_WAIST))
    {
	if (!remove_obj(ch, WEAR_WAIST, fReplace))
	    return;
	act_wear("$n завязывает $p6 вокруг талии.", ch, obj);
	act("Ты завязываешь $p6 вокруг талии.", ch, obj, NULL, TO_CHAR);
	equip_char(ch, obj, WEAR_WAIST);
    }
    else if (CAN_WEAR(obj, ITEM_WEAR_WRIST))
    {
	if (get_eq_char(ch, WEAR_WRIST_L) != NULL
	    && get_eq_char(ch, WEAR_WRIST_R) != NULL
	    && !remove_obj(ch, WEAR_WRIST_L, fReplace)
	    && !remove_obj(ch, WEAR_WRIST_R, fReplace))
	{
	    return;
	}

	if (get_eq_char(ch, WEAR_WRIST_L) == NULL)
	{
	    act_wear("$n просовывает левую руку в%s $p6.",
		     ch, obj);
	    act("Ты просовываешь левую руку в $p6.",
		ch, obj, NULL, TO_CHAR);
	    equip_char(ch, obj, WEAR_WRIST_L);
	}
	else if (get_eq_char(ch, WEAR_WRIST_R) == NULL)
	{
	    act_wear("$n просовывает правую руку в%s $p6.",
		     ch, obj);
	    act("Ты просовываешь правую руку в $p6.",
		ch, obj, NULL, TO_CHAR);
	    equip_char(ch, obj, WEAR_WRIST_R);
	}
	else
	{
	    bugf("Wear_obj: no free wrist.");
	    send_to_char("Твои запястья уже заняты.\n\r", ch);
	    return;
	}
    }
    else if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))
    {
	OBJ_DATA *weapon;

	if (!remove_obj(ch, WEAR_SHIELD, fReplace))
	    return;

	weapon = get_eq_char(ch, WEAR_WIELD);
	if (get_eq_char (ch, WEAR_SECONDARY) != NULL ||
	    (weapon != NULL && ch->size < SIZE_LARGE &&
	     IS_WEAPON_STAT(weapon, WEAPON_TWO_HANDS)))
	{
	    send_to_char("Тебе надо освободить руку для того, чтобы одеть щит!\n\r", ch);
	    return;
	}

	act_wear("$n одевает $p6 как щит.", ch, obj);
	act("Ты одеваешь $p6 как щит.", ch, obj, NULL, TO_CHAR);
	equip_char(ch, obj, WEAR_SHIELD);
    }
    else if (CAN_WEAR(obj, ITEM_WIELD))
    {

	OBJ_DATA *sec;

	if (IS_WEAPON_STAT(obj, WEAPON_SECONDARY_ONLY))
	{
	    send_to_char("Это можно взять только во вторую руку.\n\r", ch);
	    return;
	}

	if (!check_wield_weight(ch, obj, FALSE))
	{
	    send_to_char("Это для тебя слишком тяжело.\n\r", ch);
	    return;
	}

	if (!check_primary_secondary(ch, obj, (sec = get_eq_char(ch, WEAR_SECONDARY))))
	{
	    send_to_char ("Твое второе оружие должно быть легче первого.\n\r", ch);
	    return;
	}

	if (!remove_obj(ch, WEAR_WIELD, fReplace))
	    return;

	if (!IS_NPC(ch) && ch->size < SIZE_LARGE
	    &&  IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS)
	    &&  (get_eq_char(ch, WEAR_SHIELD) != NULL || get_eq_char(ch, WEAR_HOLD) != NULL || sec != NULL))
	{
	    send_to_char("Тебе нужно иметь обе руки свободными для этого оружия.\n\r", ch);
	    return;
	}

	act_wear("$n вооружается $p4.", ch, obj);
	act("Ты вооружаешься $p4.", ch, obj, NULL, TO_CHAR);
	equip_char(ch, obj, WEAR_WIELD);

	show_weapon_level(ch, obj, FALSE);
    }
    else if (CAN_WEAR(obj, ITEM_HOLD))
    {
	OBJ_DATA *weapon;


	weapon = get_eq_char(ch, WEAR_WIELD);
	if (get_eq_char (ch, WEAR_SECONDARY) != NULL ||
	    (weapon != NULL && ch->size < SIZE_LARGE &&
	     IS_WEAPON_STAT(weapon, WEAPON_TWO_HANDS)))
	{
	    send_to_char ("Освободи сначала левую руку.\n\r", ch);
	    return;
	}

	if (!remove_obj(ch, WEAR_HOLD, fReplace))
	    return;
	act_wear("$n зажимает $p6 в руке.",   ch, obj);
	act("Ты зажимаешь $p6 в своей руке.", ch, obj, NULL, TO_CHAR);
	equip_char(ch, obj, WEAR_HOLD);
    }
    else if (CAN_WEAR(obj, ITEM_WEAR_FLOAT))
    {
	if (!remove_obj(ch, WEAR_FLOAT, fReplace))
	    return;
	act_wear("$n бросает $p6, и это начинает летать вокруг $s1.", ch, obj);
	act("Ты бросаешь $p6, и это начинает летать вокруг тебя.", ch, obj, NULL, TO_CHAR);
	equip_char(ch, obj, WEAR_FLOAT);
    }
    else if (CAN_WEAR(obj, ITEM_WEAR_AT_SHOULDER))
    {
		if (!remove_obj(ch, WEAR_AT_SHOULDER, fReplace))
	    	return;
		act_wear("$n сажает $p6 себе на плечо.", ch, obj);
		act("Ты сажаешь $p6 себе на плечо.", ch, obj, NULL, TO_CHAR);
		equip_char(ch, obj, WEAR_AT_SHOULDER);
    }

    else if (fReplace)
    {
	send_to_char("Ты не можешь одеть, вооружиться или держать это.\n\r", ch);
	return;
    }	
    if (has_trigger(obj->pIndexData->progs, TRIG_WEAR, EXEC_AFTER|EXEC_DEFAULT))
	p_wear_trigger(obj, ch);

    return;
}

void do_wield(CHAR_DATA *ch, char *argument)
{
    char buf[MSL];

    sprintf(buf, "wieldweapon %s", argument);
    do_function(ch, &do_wear, buf);
}

void do_wear(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Одеть или держать что?\n\r", ch);
	return;
    }

    if ((IS_NPC(ch) && IS_SWITCHED(ch)) || is_lycanthrope(ch))
    {
	send_to_char("Ты не можешь ничего одеть.\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "all") || !str_cmp(arg, "все"))
    {
	OBJ_DATA *obj_next;

	for (obj = ch->carrying; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;
	    if (obj->wear_loc == WEAR_NONE && can_see_obj(ch, obj))
		wear_obj(ch, obj, FALSE);
	    
	    if (!IS_VALID(ch))
	        break;
	}
	return;
    }
    else
    {
	bool wield = FALSE;

	if (!str_cmp(arg, "wieldweapon"))
	{
	    argument = one_argument(argument, arg);  
	    wield = TRUE;
	}

	obj = get_obj_carry(ch, arg, ch);

	if (obj == NULL)
	{
	    send_to_char("У тебя нет этого.\n\r", ch);
	    return;
	}

	if (wield && !CAN_WEAR(obj, ITEM_WIELD))
	{
	    send_to_char("Ты не можешь вооружиться этим.\n\r", ch);
	    return;
	}

	wear_obj(ch, obj, TRUE);
    }

    return;
}



void do_remove(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;


    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Снять что?\n\r", ch);
	return;
    }

    if (is_lycanthrope(ch))
    {
	send_to_char("На тебе же ничего не одето.\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "все") || !str_cmp(arg, "all"))
    {
	OBJ_DATA *obj_next, *light = NULL;

	for (obj = ch->carrying; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;

	    if (can_see_obj(ch, obj))
	    {
		if (obj->wear_loc == WEAR_LIGHT)
		    light = obj;
		else if (obj->wear_loc != WEAR_NONE)
		    remove_obj(ch, obj->wear_loc, TRUE);
	    }
	}

	if (light)
	    remove_obj(ch, light->wear_loc, TRUE);  

	return;
    }

    if (!str_cmp(arg, "weapon") || !str_cmp(arg, "оружие"))
    {
        if (get_eq_char(ch, WEAR_WIELD))
            remove_obj(ch, WEAR_WIELD, TRUE);
        if (get_eq_char(ch, WEAR_SECONDARY))
            remove_obj(ch, WEAR_SECONDARY, TRUE);
        return;
    }

    if ((obj = get_obj_wear(ch, arg, TRUE)) == NULL)
    {
	send_to_char("У тебя нет этого.\n\r", ch);
	return;
    }

    remove_obj(ch, obj->wear_loc, TRUE);
    return;
}

bool can_sacrifice(OBJ_DATA *obj, CHAR_DATA *ch, bool show)
{
    CHAR_DATA *gch, *safe_gch;
    OBJ_DATA *obj_in;

	if (!IS_NPC(ch) && ch->pcdata->atheist)
	{
	    send_to_char("Ты же атеист.\n\r", ch);
		return FALSE;
	}

    if (obj == NULL)
    {
	if (show)
	    send_to_char("Ты не находишь это.\n\r", ch);
		return FALSE;
    }

    if (obj->item_type == ITEM_CORPSE_PC)
    {
	if (obj->contains)
	{
	    if (show)
		send_to_char("Богам это не понравится.\n\r", ch);
	    return FALSE;
	}
    }

    if (!CAN_WEAR(obj, ITEM_TAKE) || IS_OBJ_STAT(obj, ITEM_NO_SAC))
    {
	if (show)
	    act("$p не является хорошей жертвой.", ch, obj, 0, TO_CHAR);
	return FALSE;
    }

    if (!IS_NULLSTR(obj->owner) && str_cmp(obj->owner, ch->name))
    {
	if (show)
	    send_to_char("Ты не владелец этой вещи, не трогай ее.\n\r", ch);
	return FALSE;
    }

    for (obj_in = obj->contains; obj_in; obj_in = obj_in->next_content)
	if (!IS_NULLSTR(obj_in->owner) && str_cmp(obj_in->owner, ch->name))
	{
	    if (show)
		act("Ты не являешься владельцем $p1! (Это находится в $P5)", ch, obj_in, obj, TO_CHAR);
	    return FALSE;
	}

    if (obj->in_room != NULL)
    {
	LIST_FOREACH_SAFE(gch, &obj->in_room->people, room_link, safe_gch)
	{
	    if (gch->on == obj)
	    {
		if (show)
		    act("$N использует $p6.", ch, obj, gch, TO_CHAR);
		return FALSE;
	    }
	}
    }

    return TRUE;
}

int get_sacrifice_cost(OBJ_DATA *obj)
{
    int silver;

    silver = UMAX(1, number_fuzzy(obj->level));

    if (obj->item_type != ITEM_CORPSE_NPC && obj->item_type != ITEM_CORPSE_PC)
	silver = UMIN(silver, obj->cost);

    return silver;
}


void show_and_autosplit(CHAR_DATA *ch, int silver)
{
    CHAR_DATA *gch;
    int members;
    char buffer[100];

    if (silver <= 0 || IS_VAMPIRE(ch))
	return;

    if (!can_take_weight(ch, get_money_weight(0, silver)))
    {
	obj_to_room(create_money(0, silver, 0), ch->in_room);
	send_to_char("Ты не можешь нести столько тяжестей, монеты остались лежать на земле.\n\r", ch);
	act("$n не может нести столько тяжести, монеты остались лежать на земле.", ch, NULL, NULL, TO_ROOM);
	return;
    }

    ch->silver += silver;

    if (silver == 1)
	send_to_char("Боги дают тебе одну серебряную монетку за твою жертву.\n\r", ch);
    else
    {
	sprintf(buffer, "Боги дают тебе %d серебра за твою жертву.\n\r", silver);
	send_to_char(buffer, ch);
    }

    if (IS_SET(ch->act, PLR_AUTOSPLIT) && silver > 1)
    { /* AUTOSPLIT code */
	members = 0;
	LIST_FOREACH(gch, &ch->in_room->people, room_link)
	{
	    if (is_same_group(gch, ch))
		members++;
	}

	if (members > 1)
	{
	    sprintf(buffer, "%d", silver);
	    do_function(ch, &do_split, buffer);
	}
    }

}


void do_sacrifice(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int silver = 0;

    if (POS_FIGHT(ch))
    {
	send_to_char("Ты не можешь жертвовать что-либо, пока дерешься.\n\r", ch);
	return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0' || !str_cmp(arg, ch->name))
    {
	act("$n предлагает себя богам, но они грациозно отказываются.", ch, NULL, NULL, TO_ROOM);
	send_to_char("Боги отклонили твое предложение, но могут принять его в будущем.\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "all") || !str_cmp(arg, "все"))
    {
	OBJ_DATA *obj_next;
	int count = 0;

	/* Показать, что жертвуется */

	send_to_char("Ты жертвуешь следующие предметы:\n\r", ch);
	show_list_to_char(ch->in_room->contents, ch, TRUE, FALSE, TRUE, eItemNothing);

	for (obj = ch->in_room->contents; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;
	    if (can_sacrifice(obj, ch, FALSE) && can_see_obj(ch, obj))
	    {
		silver += get_sacrifice_cost(obj);
		count++;
		extract_obj(obj, TRUE, FALSE);
	    }
	}
	if (count > 0)
	{
	    act("$n жертвует Богам все, что брошено $x.", ch, obj, NULL, TO_ROOM);
	    wiznet("$N жертвует Богам все, что брошено $x.", ch, obj, WIZ_SACCING, 0, 0);
	}
	show_and_autosplit(ch, silver);
    }
    else
    {
	obj = get_obj_list(ch, arg, ch->in_room->contents);
	if (can_sacrifice(obj, ch, TRUE))
	{
	    act("$n жертвует $p6 Богам.", ch, obj, NULL, TO_ROOM);
	    wiznet("$N жертвует $p6 Богам.", ch, obj, WIZ_SACCING, 0, 0);
	    silver = get_sacrifice_cost(obj);
	    show_and_autosplit(ch, silver);
	    extract_obj(obj, TRUE, FALSE);
	}
    }

    return;
}

void do_quaff(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	if ((obj = get_eq_char(ch, WEAR_HOLD)) == NULL)
	{
	    send_to_char("Глотать что?\n\r", ch);
	    return;
	}
    }
    else if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
    {
	send_to_char("У тебя нет этого зелья.\n\r", ch);
	return;
    }

    if (obj->item_type != ITEM_POTION)
    {
	send_to_char("Ты можешь глотать только зелья.\n\r", ch);
	return;
    }

    if (ch->level < obj->level)
    {
	send_to_char("Это зелье слишком мощное для тебя.\n\r", ch);
	return;
    }

    if (!IS_NPC(ch) && ch->pcdata->condition[COND_FULL] > 45)
    {
	send_to_char("Твой желудок уже полон!\n\r", ch);
	return;
    }

    if (check_auction_obj(ch, obj))
	return;

    act("$n глотает $p6.", ch, obj, NULL, TO_ROOM);
    act("Ты глотаешь $p6.", ch, obj, NULL , TO_CHAR);

    if (!IS_NPC(ch) && ch->pcdata->quaff > 3)
	send_to_char("Тебя выворачивает наружу!\n\r", ch);
    else
    {
	int i;

	for (i = 1; i <= 4; i++)
	    obj_cast_spell(obj->value[i], obj->value[0], ch, ch, NULL, "");

	if (!IS_NPC(ch))
	    ch->pcdata->quaff++;
    }

    extract_obj(obj, TRUE, TRUE);
    return;
}



void do_recite(CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *scroll;
    OBJ_DATA *obj = NULL;
    int i;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if ((scroll = get_obj_carry(ch, arg1, ch)) == NULL)
    {
	send_to_char("У тебя нет этого свитка.\n\r", ch);
	return;
    }

    if (scroll->item_type != ITEM_SCROLL && scroll->item_type != ITEM_MAP)
    {
	send_to_char("Ты можешь зачитывать только свитки или карты.\n\r", ch);
	return;
    }

    if (is_lycanthrope(ch))
    {
	send_to_char("Ты не можешь понять смысл написанного.\n\r",ch);
	return;
    }

    if (ch->level < scroll->level)
    {
	send_to_char(
		     "Этот свиток слишком сложен для твоего понимания.\n\r", ch);
	return;
    }

    for (i = 1; i <= 4; i++)
	if (scroll->value[i] > 0)   
	    break;
    if (i > 4)
    {
	send_to_char("Но этот свиток пуст!\n\r", ch);
	return;
    }

    if (arg2[0] == '\0')
    {
	victim = ch;
    }
    else
    {
	if ((victim = get_char_room(ch, NULL, arg2, TRUE)) == NULL
	    && (obj = get_obj_here(ch, NULL, arg2)) == NULL
	    && !check_obj_spells(scroll, 1))
	{
	    if (!too_many_victims)
		send_to_char("Ты не находишь этого.\n\r", ch);
	    return;
	}
    }

    if (check_auction_obj(ch, scroll))
	return;

    act("$n зачитывает $p6.", ch, scroll, NULL, TO_ROOM);
    act("Ты зачитываешь $p6.", ch, scroll, NULL, TO_CHAR);
    if (number_percent() >= 20 + get_skill(ch, gsn_scrolls) * 4/5)
    {
	send_to_char("Ты неправильно зачитываешь свиток, путая слоги.\n\r", ch);
	check_improve(ch, NULL, gsn_scrolls, FALSE, 2);
    }
    else
    {
	check_improve(ch, NULL, gsn_scrolls, TRUE, 2);
	if (scroll->item_type == ITEM_SCROLL)
	{
	    for (i = 1; i <= 4; i++)
		obj_cast_spell(scroll->value[i], scroll->value[0],
			       ch, victim, obj, arg2);
	}
	else
	{
	    bool found = FALSE;

	    if (scroll->value[1] != 0
		&& !IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
		&& !IS_AFFECTED(ch, AFF_CURSE)
		&& !is_affected(ch, gsn_gods_curse)
		&& ch->count_holy_attacks == 0)
	    {
		AREA_DATA *pArea;

		for (pArea = area_first; pArea != NULL; pArea = pArea->next)
		    if (pArea->min_vnum <= scroll->value[1]
			&& pArea->max_vnum >= scroll->value[1])
		    {
			break;
		    }

		if (pArea == NULL)
		{
		    bugf("Map vnum %d has bad v1.", scroll->pIndexData->vnum);
		}
		else
		{
		    int i;
		    ROOM_INDEX_DATA *location = NULL;

		    for (i = 0; i < 100; i++)
		    {
			location = get_room_index(number_range(pArea->min_vnum,
							       pArea->max_vnum));

			if (location != NULL
			    && !IS_SET(location->room_flags, ROOM_KILL)
			    && !IS_SET(location->room_flags, ROOM_NOFLY_DT)
			    && !IS_SET(location->room_flags, ROOM_NO_RECALL)
			    && !IS_SET(location->room_flags, ROOM_SAFE)
			    && can_see_room(ch, location))
			{
			    found = TRUE;
			    check_recall(ch, "Ты делаешь возврат из драки");
			    move_for_recall(ch, location);
			    ch->move = UMAX(0, ch->move /= 2);
			    check_trap(ch);
			    break;
			}
		    }
		}
	    }

	    if (!found)
		send_to_char("... И ничего не происходит.\n\r", ch);
	}
    }

    extract_obj(scroll, TRUE, TRUE);
    WAIT_STATE(ch, 24);
    return;
}



void do_brandish(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    OBJ_DATA *staff;
    int sn;

    if ((staff = get_eq_char(ch, WEAR_HOLD)) == NULL)
    {
	send_to_char("Ты ничего не держишь в руке.\n\r", ch);
	return;
    }

    if (staff->item_type != ITEM_STAFF)
    {
	send_to_char("Ты можешь взмахивать только жезлами.\n\r", ch);
	return;
    }

    if (check_auction_obj(ch, staff))
	return;

    if ((sn = staff->value[3]) < 0
	|| sn >= max_skills
	|| skill_table[sn].spell_fun == 0)
    {
	bugf("Do_brandish: bad sn %d.", sn);
	return;
    }

    WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

    if (staff->value[2] > 0)
    {
	act("$n взмахивает $p4.", ch, staff, NULL, TO_ROOM);
	act("Ты взмахиваешь $p4.",  ch, staff, NULL, TO_CHAR);

	if (ch->level < staff->level
	    || number_percent() >= 5 + get_skill(ch, gsn_staves) * 19/20)
	{
	    act ("Ты неудачно заклинаешь $p4.", ch, staff, NULL, TO_CHAR);
	    act ("...и ничего не происходит.", ch, NULL, NULL, TO_ROOM);
	    check_improve(ch, NULL, gsn_staves, FALSE, 2);
	}
	else
	{
	    LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, vch_next)
	    {
		if (!can_see(ch, vch))
		    continue;

		switch (skill_table[sn].target)
		{
		default:
		    bugf("Do_brandish: bad target for sn %d.", sn);
		    return;

		case TAR_IGNORE:
		    if (vch != ch)
			continue;
		    break;

		case TAR_CHAR_OFFENSIVE:
		case TAR_OBJ_CHAR_OFF:
		    if ((IS_NPC(ch) && IS_NPC(vch)) || (!IS_NPC(ch) && !IS_NPC(vch)))
			continue;
		    break;

		case TAR_CHAR_DEFENSIVE:
		case TAR_OBJ_CHAR_DEF:
		    if ((IS_NPC(ch) && !IS_NPC(vch)) || (!IS_NPC(ch) && IS_NPC(vch)))
			continue;
		    break;

		case TAR_CHAR_SELF:
		    if (vch != ch)
			continue;
		    break;
		}

		obj_cast_spell(staff->value[3], staff->value[0], ch, vch, NULL, argument);
		check_improve(ch, NULL, gsn_staves, TRUE, 2);
	    }
	}
    }

    if (--staff->value[2] <= 0)
    {
	act("$p ярко вспыхива$r и исчеза$r из рук $n1.", ch, staff, NULL, TO_ROOM);
	act("$p ярко вспыхива$r и исчеза$r в твоих руках.", ch, staff, NULL, TO_CHAR);
	extract_obj(staff, TRUE, TRUE);
    }

    return;
}



void do_zap(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim = NULL;
    OBJ_DATA *wand;
    OBJ_DATA *obj = NULL;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	if ((wand = get_eq_char(ch, WEAR_HOLD)) == NULL)
	{
	    send_to_char("Тереть что?\n\r", ch);
	    return;
	}

	if (ch->fighting != NULL)
	{
	    if (skill_table[wand->value[3]].target == TAR_CHAR_SELF
	    || skill_table[wand->value[3]].target == TAR_CHAR_DEFENSIVE)
		victim = ch;
	    else
		victim = ch->fighting;
	}
	else
	    victim = ch;
    }
    else
    {
	if ((wand = get_obj_wear(ch, arg, TRUE)) == NULL)
	{
	    send_to_char("У тебя нет этого.\n\r", ch);
	    return;
	}

	if (wand->wear_loc == WEAR_NONE)
	{
	    send_to_char("Ты должен это одеть.\n\r", ch);
	    return;
	}

	if (argument[0] == '\0')
	{
	    if (ch->fighting != NULL)
	    {
		if (skill_table[wand->value[3]].target == TAR_CHAR_SELF
		|| skill_table[wand->value[3]].target == TAR_CHAR_DEFENSIVE)
		    victim = ch;
		else
		    victim = ch->fighting;
	    }
	    else
		victim = ch;
	}
	else
	{
	    victim = get_char_room (ch, NULL, argument, TRUE);
	    obj    = get_obj_here  (ch, NULL, argument);
	}
    }

    if (wand->item_type != ITEM_WAND)
    {
	send_to_char("Ты можешь тереть только волшебные палочки.\n\r", ch);
	return;
    }

    if (check_auction_obj(ch, wand))
	return;

    if (check_obj_spells(wand, 3))
    {
	victim = NULL;
	obj    = NULL;
    }
    else if (!victim && !obj)
    {
	send_to_char("Тереть на что или на кого?\n\r", ch);
	return;
    }

    WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

    if (wand->value[2] > 0)
    {
	if (victim != NULL)
	{
	    if (victim != ch)
	    {
		act("$n потирает $p6 на $N3.",  ch, wand, victim, TO_NOTVICT);
		act("Ты потираешь $p6 на $N3.", ch, wand, victim, TO_CHAR);
		act("$n потирает $p6 на тебя.", ch, wand, victim, TO_VICT);
	    }
	    else
	    {
		act("Ты потираешь $p6 на себя.", ch, wand, NULL, TO_CHAR);
		act("$n потирает $p6 на себя.", ch, wand, NULL, TO_ROOM);
	    }
	}
	else if (obj != NULL)
	{
	    act("$n потирает $p6 на $P6.", ch, wand, obj, TO_ROOM);
	    act("Ты трешь $p6 на $P6.", ch, wand, obj, TO_CHAR);
	}
	else
	{
	    act("$n трет $p6.", ch, wand, NULL, TO_ROOM);
	    act("Ты трешь $p6.", ch, wand, NULL, TO_CHAR);
	}


	if (ch->level < wand->level
	    ||  number_percent() >= 5 + get_skill(ch, gsn_wands) * 19/20)
	{
	    act("Твои попытки потереть $p6 вызвали только дым и искры.",
		ch, wand, NULL, TO_CHAR);
	    act("Попытки $n1 потереть $p6 вызвали только дым и искры.",
		ch, wand, NULL, TO_ROOM);
	    check_improve(ch, NULL, gsn_wands, FALSE, 2);
	}
	else
	{
	    obj_cast_spell(wand->value[3], wand->value[0], ch, victim, obj, argument);
	    check_improve(ch, NULL, gsn_wands, TRUE, 2);
	}
    }

    if (--wand->value[2] <= 0)
    {
	act("$p распада$rся на кусочки в руках $n1.", ch, wand, NULL, TO_ROOM);
	act("$p распада$rся на кусочки в твоих руках.", ch, wand, NULL, TO_CHAR);
	extract_obj(wand, TRUE, TRUE);
    }

    return;
}



void do_steal(CHAR_DATA *ch, char *argument)
{
    char buf  [MAX_STRING_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int percent;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0')
    {
	send_to_char("Украсть что у кого?\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, NULL, arg2, FALSE)) == NULL)
    {
	send_to_char("Таких здесь нет.\n\r", ch);
	return;
    }

    if (victim == ch || IS_IMMORTAL(victim))
    {
	send_to_char("Это бессмысленно.\n\r", ch);
	return;
    }

    if (is_safe(ch, victim))
	return;

    if (POS_FIGHT(ch))
    {
	send_to_char("Ты же сражаешься!.\n\r", ch);
	return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
    {
	act("Но $N - твой друг!", ch, NULL, victim, TO_CHAR);
	return;
    }

    WAIT_STATE(ch, skill_table[gsn_steal].beats);
    percent  = number_percent();

    if (!IS_AWAKE(victim))
	percent -= 10;
    else if (!can_see(victim, ch))
	percent += 20;
    else
	percent += 40;

    percent -= (ch->level - victim->level)/3;

    percent = UMAX(1, percent);

    if ((ch->level + PK_RANGE < victim->level && !IS_NPC(victim) && !IS_NPC(ch))
	|| percent >= get_skill(ch, gsn_steal))
    {
	/*
	 * Failure.
	 */

	send_to_char("Оп-па.\n\r", ch);
	do_function(ch, &do_visible, "steal");

	act("$n пытается обокрасть тебя.", ch, NULL, victim, TO_VICT);
	act("$n пытается обокрасть $N3.",  ch, NULL, victim, TO_NOTVICT);

	switch(number_range(0, 3))
	{
	case 0 :
	    sprintf(buf, "%s, вшивое ворье!", PERS(ch, victim, 0));
	    break;
	case 1 :
	    sprintf(buf, "%s, ты попал%s!",
		    PERS(ch, victim, 0), (ch->sex == SEX_MALE) ? "ся" : (ch->sex == SEX_FEMALE) ? "ась" : "ось");
	    break;
	case 2 :
	    sprintf(buf, "%s пытается обокрасть меня!", PERS(ch, victim, 0));
	    break;
	case 3 :
	    sprintf(buf, "Держи свои грязные руки при себе, %s!", PERS(ch, victim, 0));
	    break;
	}
	if (!IS_AWAKE(victim))
	    do_function(victim, &do_wake, "");
	if (IS_AWAKE(victim))
	{
	    if (!is_animal(victim))
	        do_function(victim, &do_yell, buf);
	    else
	    {
	        check_social(victim, "рыч", ch->name);
	    }
	}
	if (!IS_NPC(ch))
	{
	    if (IS_NPC(victim))
	    {
	        if (get_master(victim) == victim)
	        {
		    if (has_trigger(victim->pIndexData->progs, TRIG_KILL, EXEC_AFTER|EXEC_DEFAULT))
		    {
			p_percent_trigger(victim, NULL, NULL, ch,
					  NULL, NULL, TRIG_KILL);
			return;
		    }

		    multi_hit(victim, ch, TYPE_UNDEFINED);
		}
	    }
	    else
	    {
		sprintf(buf, "$N пытается обокрасть %s.", cases(victim->name, 1));
		wiznet(buf, ch, NULL, WIZ_FLAGS, 0, 0);
		if (!IS_IMMORTAL(ch) && !IS_SET(ch->act, PLR_THIEF))
		{
		    SET_BIT(ch->act, PLR_THIEF);
		    REMOVE_BIT(ch->comm, COMM_HELPER);
		    send_to_char("*** Ты  теперь ВОР!! ***\n\r", ch);
		    save_char_obj(ch, FALSE);

		}
	    }
	    check_improve(ch, victim, gsn_steal, FALSE, 2);
	}

	return;
    }

    if (!str_cmp(arg1, "coin")
	|| !str_cmp(arg1, "coins")
	|| !str_cmp(arg1, "gold")
	|| !str_cmp(arg1, "silver")
	|| !str_cmp(arg1, "монеты")
	|| !str_cmp(arg1, "золото")
	|| !str_cmp(arg1, "серебро"))
    {
	int gold, silver, bronze;

	gold = victim->gold * number_range(1, ch->level) / MAX_LEVEL;
	silver = victim->silver * number_range(1, ch->level) / MAX_LEVEL;
	if (!IS_NPC(victim) && !IS_NPC(ch))
	{
		bronze = victim->pcdata->bronze * number_range(1, ch->level) / MAX_LEVEL;
	}
	if (gold <= 0 && silver <= 0 && bronze <= 0)
	{
	    send_to_char("Ты не можешь взять ни монетки.\n\r", ch);
	    return;
	}

	ch->gold     	+= gold;
	ch->silver   	+= silver;
	victim->silver 	-= silver;
	victim->gold 	-= gold;
	if (!IS_NPC(victim) && !IS_NPC(ch))
	{
		ch->pcdata->bronze   	+= bronze;
		victim->pcdata->bronze 	-= bronze;
	}


	strcpy(buf, "Вау! Ты крадешь, может быть, немножечко бронзы, и ");

	if (silver <= 0)
	{
	    sprintf(arg1, "%d %s ", gold, hours(gold, TYPE_GOLD1));
	    strcat(buf, arg1);
	    sprintf(arg1, "%s.\n\r", hours(gold, TYPE_COINS1));
	    strcat(buf, arg1);
	}
	else if (gold <= 0)
	{
	    sprintf(arg1, "%d %s ", silver, hours(silver, TYPE_SILVER1));
	    strcat(buf, arg1);
	    sprintf(arg1, "%s.\n\r", hours(silver, TYPE_COINS1));
	    strcat(buf, arg1);
	}
	else
	{
	    sprintf(arg1, "%d %s и %d", silver, hours(silver, TYPE_SILVER1), gold);
	    strcat(buf, arg1);
	    sprintf(arg1, " %s ", hours(gold, TYPE_GOLD1));
	    strcat(buf, arg1);
	    sprintf(arg1, "%s.\n\r", hours(gold, TYPE_COINS1));
	    strcat(buf, arg1);
	}

	send_to_char(buf, ch);
	check_improve(ch, victim, gsn_steal, TRUE, 2);
	return;
    }

    if ((obj = get_obj_carry(victim, arg1, ch)) == NULL)
    {
	send_to_char("Ты не находишь этого.\n\r", ch);
	return;
    }

    if (!can_drop_obj(ch, obj)
	|| IS_SET(obj->extra_flags, ITEM_INVENTORY)
	|| obj->level > ch->level
	|| !can_loot(ch, obj))
    {
	send_to_char("Спрячь свое любопытство.\n\r", ch);
	return;
    }

    if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch))
    {
	send_to_char("У тебя все руки заняты.\n\r", ch);
	return;
    }

    if (!can_take_weight(ch, get_obj_weight(obj)))
    {
	send_to_char("Ты не можешь нести такую тяжесть.\n\r", ch);
	return;
    }

    if (is_have_limit(ch, ch, obj))
	return;

    obj_from_char(obj, TRUE);
    obj_to_char(obj, ch);
    act("Ты прикарманиваешь $p6.", ch, obj, NULL, TO_CHAR);
    check_improve(ch, victim, gsn_steal, TRUE, 2);
    send_to_char("Получилось!\n\r", ch);
    return;
}



/*
 * Shopping commands.
 */
CHAR_DATA *find_keeper(CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *keeper;
    SHOP_DATA *pShop;

    pShop = NULL;
    LIST_FOREACH(keeper, &ch->in_room->people, room_link)
    {
	if (IS_NPC(keeper) && (pShop = keeper->pIndexData->pShop) != NULL)
	    break;
    }

    if (pShop == NULL)
    {
	send_to_char("Ты не можешь здесь этого сделать.\n\r", ch);
	return NULL;
    }

    /*
     * Shop hours.
     */
    if (time_info.hour < pShop->open_hour)
    {
	do_function(keeper, &do_say, 
		    IS_SET(ch->act, ACT_SENTINEL) ? "Извини, мы еще закрыты. Приходи попозже." : 
		    "Извини, я еще не торгую. Приходи попозже.");
	return NULL;
    }

    if (time_info.hour > pShop->close_hour)
    {
	do_function(keeper, &do_say, 
		    IS_SET(ch->act, ACT_SENTINEL) ? "Извини, мы уже закрылись. Приходи завтра."
		    : "Извини, я уже не торгую. Приходи завтра.");
	return NULL;
    }

    /*
     * Invisible or hidden people.
     */
    if (!can_see(keeper, ch))
    {
	do_function(keeper, &do_say, "Я не могу торговать с тем, кого не вижу.");
	return NULL;
    }

    /*
     * Undesirables.
     */
    if (!IS_NPC(ch) && IS_SET(ch->act, PLR_KILLER))
    {
	do_function(keeper, &do_say, "Убийц у нас не жалуют!");
	sprintf(buf, "%s, подл%s убийца, здесь!\n\r", ch->name, 
	     ch->sex == SEX_FEMALE ? "ая" : "ый");
	do_function(keeper, &do_yell, buf);
	return NULL;
    }

    if (!IS_NPC(ch) && IS_SET(ch->act, PLR_THIEF))
    {
	do_function(keeper, &do_say, "Воров у нас не жалуют!");
	sprintf(buf, "%s, подлый вор, здесь!\n\r", ch->name);
	do_function(keeper, &do_yell, buf);
	return NULL;
    }

    return keeper;
}

/* insert an object at the right spot for the keeper */
void obj_to_keeper(OBJ_DATA *obj, CHAR_DATA *ch)
{
    OBJ_DATA *t_obj;

    /* see if any duplicates are found */
    for (t_obj = ch->carrying; t_obj != NULL; t_obj = t_obj->next_content)
    {
	if (obj->pIndexData == t_obj->pIndexData
	    && !str_cmp(obj->short_descr, t_obj->short_descr))
	{
	    /* if this is an unlimited item, destroy the new one */
	    if (IS_OBJ_STAT(t_obj, ITEM_INVENTORY))
	    {
		extract_obj(obj, TRUE, TRUE);
		return;
	    }
	    obj->cost = t_obj->cost; /* keep it standard */
	    break;
	}

    }

    obj_from_char(obj, TRUE);
    obj_to_char_gen(obj, ch, FALSE, t_obj);
}

/* get an object from a shopkeeper's list */
OBJ_DATA *get_obj_keeper(CHAR_DATA *ch, CHAR_DATA *keeper, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number = 0;
    int count = 0;

    if (argument[0] == '#' || (unsigned char) argument[0] == 185)
    {
	int cnt=0;

	argument++;
	number=atoi(argument);
	for (obj = keeper->carrying; obj != NULL; obj = obj->next_content)
	{
	    if (obj->wear_loc == WEAR_NONE
		&& can_see_obj(keeper, obj)
		&& can_see_obj(ch, obj)
		&& get_cost(keeper, obj, TRUE) > 0)
	    {
		if (!IS_OBJ_STAT(obj, ITEM_INVENTORY))
		{
		    while (obj->next_content != NULL
			   && obj->pIndexData == obj->next_content->pIndexData
			   && !str_cmp(obj->short_descr,
				       obj->next_content->short_descr))
		    {
			obj = obj->next_content;
		    }
		}
		if (++cnt == number)
		    return obj;
	    }
	}
    }
    else
    {
	number = number_argument(argument, arg);
	count  = 0;
	for (obj = keeper->carrying; obj != NULL; obj = obj->next_content)
	{
	    if (obj->wear_loc == WEAR_NONE
		&& can_see_obj(keeper, obj)
		&& can_see_obj(ch, obj)
		&& is_name(arg, obj->name))
	    {
		if (++count == number)
		    return obj;

		/* skip other objects of the same name */
		while (obj->next_content != NULL
		       && obj->pIndexData == obj->next_content->pIndexData
		       && !str_cmp(obj->short_descr,
				   obj->next_content->short_descr))
		{
		    obj = obj->next_content;
		}
	    }
	}
    }
    return NULL;
}

int get_cost(CHAR_DATA *keeper, OBJ_DATA *obj, bool fBuy)
{
    SHOP_DATA *pShop;
    int cost;

    if (obj == NULL || (pShop = keeper->pIndexData->pShop) == NULL)
	return 0;

    if (fBuy)
    {
	cost = obj->cost * pShop->profit_buy  / 100;
    }
    else
    {
	OBJ_DATA *obj2;
	int itype;

	cost = 0;
	for (itype = 0; itype < MAX_TRADE; itype++)
	{
	    if (obj->item_type == pShop->buy_type[itype])
	    {
		cost = obj->cost * pShop->profit_sell / 100;
		break;
	    }
	}

	if (!IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT))
	    for (obj2 = keeper->carrying; obj2; obj2 = obj2->next_content)
	    {
		if (obj->pIndexData == obj2->pIndexData
		    &&   !str_cmp(obj->short_descr, obj2->short_descr))
		{
		    if (IS_OBJ_STAT(obj2, ITEM_INVENTORY))
			cost /= 2;
		    else
			cost = cost * 3 / 4;
		}
	    }
    }

    if (obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND)
    {
	if (obj->value[1] == 0)
	    cost /= 4;
	else
	    cost = cost * obj->value[2] / obj->value[1];
    }
    cost= (int) sqrt(obj->condition)*cost/10;
    return cost;
}



void do_buy(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    int cost, roll;

    if (argument[0] == '\0')
    {
	send_to_char("Что ты хотел купить?\n\r", ch);
	return;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_MOUNT_SHOP))
    {
	do_buy_mount(ch, argument);
	return;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP))
    {
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *pet;
	ROOM_INDEX_DATA *pRoomIndexNext;
	ROOM_INDEX_DATA *in_room;

	smash_tilde(argument);

	if (IS_NPC(ch))
	    return;

	if (IS_SET(ch->act, PLR_NOFOLLOW))
	{
	    send_to_char("Ты не принимаешь последователей.\n\r", ch);
	    return;
	}

	argument = one_argument(argument, arg);

	/* hack to make new thalos pets work */
	if (ch->in_room->vnum == 9621)
	    pRoomIndexNext = get_room_index(9706);
	else
	    pRoomIndexNext = get_room_index(ch->in_room->vnum + 1);
	if (pRoomIndexNext == NULL)
	{
	    bugf("Do_buy: bad pet shop at vnum %d.", ch->in_room->vnum);
	    send_to_char("Извини, ты не можешь купить это здесь.\n\r", ch);
	    return;
	}

	in_room     = ch->in_room;
	ch->in_room = pRoomIndexNext;
	pet         = get_char_room(ch, NULL, arg, FALSE);
	ch->in_room = in_room;

	if (pet == NULL || !IS_NPC(pet) || !IS_SET(pet->act, ACT_PET))
	{
	    send_to_char("Извини, ты не можешь купить это здесь.\n\r", ch);
	    return;
	}

	if (ch->pet != NULL)
	{
	    send_to_char("У тебя уже есть любимец.\n\r", ch);
	    return;
	}

	if (max_count_charmed(ch))
	    return;

	cost = 10 * pet->level * pet->level;

	if ((ch->silver + 100 * ch->gold) < cost)
	{
	    send_to_char("Ты не можешь позволить себе купить это.\n\r", ch);
	    return;
	}

	if (ch->level < pet->level)
	{
	    send_to_char("Ты недостаточно опытен, чтобы управлять этим любимцем.\n\r", ch);
	    return;
	}

	/* haggle */
	roll = number_percent();
	if (roll < get_skill(ch, gsn_haggle))
	{
	    cost -= cost / 2 * roll / 100;
	    sprintf(buf, "Ты торгуешься и снижаешь цену до %d монет.\n\r", cost);
	    send_to_char(buf, ch);
	    check_improve(ch, NULL, gsn_haggle, TRUE, 4);

	}

	deduct_cost(ch, cost);
	pet			= create_mobile(pet->pIndexData);
	SET_BIT(pet->act, ACT_PET);
	SET_BIT(pet->affected_by, AFF_CHARM);
	SET_BIT(pet->act, ACT_NOEXP);
	pet->comm = COMM_NOTELL|COMM_NOSHOUT|COMM_NOCHANNELS;
	pet->gold = 0;
	pet->silver = 0;

	if (ch->classid == CLASS_RANGER)
	{
	    pet->hit += (pet->hit * 3 * ch->level / 200);
	    pet->max_hit = pet->hit;
	}

	/*	argument = one_argument(argument, arg);
	 if (arg[0] != '\0')
	 {
	 sprintf(buf, "%s %s", pet->name, arg);
	 free_string(pet->name);
	 pet->name = str_dup(buf);
	 } */

	sprintf(buf, "%s\n\rТабличка на шее гласит: 'Принадлежит %s'.\n\r", pet->description, ch->pcdata->cases[1]);
	free_string(pet->description);
	pet->description = str_dup(buf);

	char_to_room(pet, ch->in_room, FALSE);
	add_follower(pet, ch);
	pet->leader = ch;
	ch->pet = pet;
	send_to_char("Получи своего любимца.\n\r", ch);
	act("$n покупает $N3.", ch, NULL, pet, TO_ROOM);
	return;
    }
    else
    {
	CHAR_DATA *keeper;
	OBJ_DATA *obj, *t_obj;
	char arg[MAX_INPUT_LENGTH];
	int  number, count = 1;

	if ((keeper = find_keeper(ch)) == NULL)
	    return;

	number = mult_argument(argument, arg);
	obj  = get_obj_keeper(ch, keeper, arg);
	cost = get_cost(keeper, obj, TRUE);

	if (number < 1 || number > 99)
	{
	    act("$n говорит тебе: {RБудь благоразумен!{x", keeper, NULL, ch, TO_VICT);
	    return;
	}

	if (cost <= 0 || !can_see_obj(ch, obj))
	{
	    act("$n говорит тебе: {Rя не продаю этого, набери 'список' для просмотра моих товаров.{x",
		keeper, NULL, ch, TO_VICT);
	    ch->reply = keeper;
	    return;
	}

	if (!IS_OBJ_STAT(obj, ITEM_INVENTORY))
	{
	    for (t_obj = obj->next_content;
		 count < number && t_obj != NULL;
		 t_obj = t_obj->next_content)
	    {
		if (t_obj->pIndexData == obj->pIndexData
		    &&  !str_cmp(t_obj->short_descr, obj->short_descr))
		    count++;
		else
		    break;
	    }

	    if (count < number)
	    {
		act("$n говорит тебе: {RУ меня нет такого количества в запасе.{x",
		    keeper, NULL, ch, TO_VICT);
		ch->reply = keeper;
		return;
	    }
	}

	if (is_have_limit(ch, ch, obj))
	    return;

	if (is_limit(obj) != -1 && number > 1)
	{
	    act("$n говорит тебе: {RИзвини, эту вещь я могу продать только в единственном экземпляре.{x",
		keeper, NULL, ch, TO_VICT);
	    ch->reply = keeper;
	    return;
	}


	if ((ch->silver + ch->gold * 100) < cost * number)
	{
	    if (number > 1)
		act("$n говорит тебе: {RТы не можешь позволить себе купить столько.{x",
		    keeper, obj, ch, TO_VICT);
	    else
		act("$n говорит тебе: {RТы не можешь позволить себе купить $p6.{x",
		    keeper, obj, ch, TO_VICT);
	    ch->reply = keeper;
	    return;
	}

	if (obj->level > ch->level)
	{
	    act("$n говорит тебе: {RТы пока не сможешь использовать $p6.{x", keeper, obj, ch, TO_VICT);
	    ch->reply = keeper;
	    return;
	}

	if (ch->carry_number + number * get_obj_number(obj) > can_carry_n(ch))
	{
	    send_to_char("Ты не сможешь нести так много вещей.\n\r", ch);
	    return;
	}

	if (!can_take_weight(ch, number * get_obj_weight(obj)))
	{
	    send_to_char("Тебе не утащить столько тяжестей.\n\r", ch);
	    return;
	}

	/* haggle */
	roll = number_percent();
	if (!IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT)
	    && roll < get_skill(ch, gsn_haggle))
	{
	    cost -= obj->cost / 2 * roll / 100;
	    act("Ты торгуешься с $N4.", ch, NULL, keeper, TO_CHAR);
	    check_improve(ch, NULL, gsn_haggle, TRUE, 4);
	}

	if (number > 1)
	{
	    sprintf(buf, "$n покупает $p6[%d].", number);
	    act(buf, ch, obj, NULL, TO_ROOM);
	    sprintf(buf, "Ты покупаешь $p6[%d] за %d серебра.", number, cost * number);
	    act(buf, ch, obj, NULL, TO_CHAR);
	}
	else
	{
	    act("$n покупает $p6.", ch, obj, NULL, TO_ROOM);
	    sprintf(buf, "Ты покупаешь $p6 за %d серебра.", cost);
	    act(buf, ch, obj, NULL, TO_CHAR);
	}
	deduct_cost(ch, cost * number);
	keeper->gold += cost * number/100;
	keeper->silver += cost * number - (cost * number/100) * 100;

	for (count = 0; count < number; count++)
	{
	    if (IS_SET(obj->extra_flags, ITEM_INVENTORY))
		t_obj = create_object(obj->pIndexData, obj->level);
	    else
	    {
		t_obj = obj;
		obj = obj->next_content;
		obj_from_char(t_obj, TRUE);
	    }

	    obj_to_char(t_obj, ch);
	    if (cost < t_obj->cost)
		t_obj->cost = cost;
	}
    }
}



void do_list(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if (IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP)
	|| IS_SET(ch->in_room->room_flags, ROOM_MOUNT_SHOP))
    {
	ROOM_INDEX_DATA *pRoomIndexNext;
	CHAR_DATA *pet;
	bool found;

	/* hack to make new thalos pets work */
	if (ch->in_room->vnum == 9621)
	    pRoomIndexNext = get_room_index(9706);
	else
	    pRoomIndexNext = get_room_index(ch->in_room->vnum + 1);

	if (pRoomIndexNext == NULL)
	{
	    bugf("Do_list: bad pet shop at vnum %d.", ch->in_room->vnum);
	    send_to_char("Ты не можешь этого здесь.\n\r", ch);
	    return;
	}

	found = FALSE;
	LIST_FOREACH(pet, &pRoomIndexNext->people, room_link)
	{
	    if (IS_SET(pet->act, ACT_PET) || IS_SET(pet->act, ACT_MOUNT))
	    {
		if (!found)
		{
		    found = TRUE;
		    if (IS_SET(pet->act, ACT_PET))
			send_to_char("Ручные животные на продажу:\n\r", ch);
		    else if (IS_SET(pet->act, ACT_MOUNT))
			send_to_char("Лошади на продажу:\n\r", ch);
		}
		sprintf(buf, "[%2d] %8d - %s\n\r",
			pet->level,
			10 * pet->level * pet->level,
			pet->short_descr);
		send_to_char(buf, ch);
	    }
	}
	if (!found)
	{
	    if (IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP))
		send_to_char("Извини, но все животные кончились.\n\r", ch);
	    else
		send_to_char("Извини, но все лошади закончились.\n\r", ch);
	}
	return;
    }
    else
    {
	CHAR_DATA *keeper;
	OBJ_DATA *obj;
	int cost, count, cnt=0;
	bool found;
	char arg[MAX_INPUT_LENGTH];

	if ((keeper = find_keeper(ch)) == NULL)
	    return;
	one_argument(argument, arg);

	found = FALSE;
	for (obj = keeper->carrying; obj; obj = obj->next_content)
	{
	    int i = 0;
	    char * p = obj->name;

	    for (; *p && !IS_SPACE(*p); p++, i++)
		;

	    if (obj->wear_loc == WEAR_NONE
		&&   can_see_obj(ch, obj)
		&&   can_see_obj(keeper, obj)
		&&   (cost = get_cost(keeper, obj, TRUE)) > 0
		&&   (arg[0] == '\0'
		      ||  is_name(arg, obj->name)))
	    {
		if (!found)
		{
		    found = TRUE;
		    send_to_char("[#  Ур Цена  Кол] Вещь\n\r", ch);
		}
		cnt++;
		if (IS_OBJ_STAT(obj, ITEM_INVENTORY))
		    sprintf(buf, "[%-2d %2d %5d -- ] "
			    MXPTAG ("list '%.*s' '%s'")
			    "%s"
			    MXPTAG ("/list")
			    "\n\r",
			    cnt,
			    obj->level, cost,
			    i, obj->name, obj->short_descr,
			    obj->short_descr);
		else
		{
		    count = 1;

		    while (obj->next_content != NULL
			   && obj->pIndexData == obj->next_content->pIndexData
			   && !str_cmp(obj->short_descr,
				       obj->next_content->short_descr))
		    {
			obj = obj->next_content;
			count++;
		    }
		    sprintf(buf, "[%-2d %2d %5d %2d ] "
			    MXPTAG ("list '%.*s' '%s'")
			    "%s"
			    MXPTAG ("/list")
			    "\n\r",
			    cnt, obj->level, cost, count,
			    i, obj->name, obj->short_descr,
			    obj->short_descr);
		}
		send_to_char(buf, ch);
	    }
	}

	if (!found)
	    send_to_char("Ты ничего не можешь здесь купить.\n\r", ch);
	return;
    }
}



void do_sell(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int cost, roll;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Продать что?\n\r", ch);
	return;
    }

    if ((keeper = find_keeper(ch)) == NULL)
	return;

    if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
    {
	act("$n говорит тебе: {RНе морочь мне голову, у тебя нет этого.{x",
	    keeper, NULL, ch, TO_VICT);
	ch->reply = keeper;
	return;
    }

    if (!can_drop_obj(ch, obj))
    {
	send_to_char("Ты не можешь с этим расстаться.\n\r", ch);
	return;
    }

    if (!can_see_obj(keeper, obj))
    {
	act("$n не видит того, что ты $m предлагаешь.", keeper, NULL, ch, TO_VICT);
	return;
    }

    if ((cost = get_cost(keeper, obj, FALSE)) <= 0)
    {
	act("$n даже не смотрит на $p6.", keeper, obj, ch, TO_VICT);
	return;
    }
    if (cost > (keeper-> silver + 100 * keeper->gold))
    {
	act("$n говорит тебе: {RИзвини, у меня закончились деньги.{x",
	    keeper, obj, ch, TO_VICT);
	return;
    }

    act("$n продает $p6.", ch, obj, NULL, TO_ROOM);
    /* haggle */
    roll = number_percent();
    if (!IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT) && roll < get_skill(ch, gsn_haggle))
    {
	send_to_char("Ты торгуешься с продавцом.\n\r", ch);
	cost += obj->cost / 3 * roll / 100;
	//        cost = UMIN(cost, 95 * get_cost(keeper, obj, TRUE) / 100);
	cost = UMIN(cost, (keeper->silver + 100 * keeper->gold));
	check_improve(ch, NULL, gsn_haggle, TRUE, 4);
    }
    sprintf(buf, "Ты продаешь $p6 за %d серебра %d золота.",
	    cost - (cost/100) * 100, cost/100);
    act(buf, ch, obj, NULL, TO_CHAR);
    ch->gold     += cost/100;
    ch->silver 	 += cost - (cost/100) * 100;
    deduct_cost(keeper, cost);
    if (keeper->gold < 0)
	keeper->gold = 0;
    if (keeper->silver< 0)
	keeper->silver = 0;

    if (obj->item_type == ITEM_TRASH || IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT))
	extract_obj(obj, TRUE, TRUE);
    else
	obj_to_keeper(obj, keeper);

    return;
}



void do_value(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int cost;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Оценить что?\n\r", ch);
	return;
    }

    if ((keeper = find_keeper(ch)) == NULL)
	return;

    if (!str_cmp(arg, "list") || !str_cmp(arg, "список"))
    {
	OBJ_DATA *iobj;
	int count = 0;

	if (number_percent() < get_skill(ch, gsn_haggle))
	{
	    for(iobj = ch->carrying; iobj; iobj = iobj->next_content)
		if (iobj->wear_loc == WEAR_NONE)
		{
		    count++;
		    do_value(ch, iobj->name);
		}

	    if (count == 0)
	    {
		act("$N говорит тебе: {RУ тебя ничего и нету.{x", ch, NULL, keeper, TO_CHAR);
		ch->reply = keeper;
	    }
	    else
	    {
		send_to_char("\n\r", ch);
		sprintf(buf, "$N говорит тебе: {RУ меня сейчас есть %ld золота и %ld серебра.{x",
			keeper->gold, keeper->silver);
		act(buf, ch, NULL, keeper, TO_CHAR);
		ch->reply = keeper;
		check_improve(ch, NULL, gsn_haggle, TRUE, 8);
	    }
	}
	else
	    send_to_char("У тебя не получается привлечь внимание торговца для того, чтобы он рассмотрел твой инвентарь.\n\r", ch);

	return;
    }

    if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
    {
	act("$n говорит тебе: {RУ тебя нет этого.{x",
	    keeper, NULL, ch, TO_VICT);
	ch->reply = keeper;
	return;
    }

    if (!can_see_obj(keeper, obj))
    {
	act("$n не видит того, что ты ему предлагаешь.", keeper, NULL, ch, TO_VICT);
	return;
    }

    if (!can_drop_obj(ch, obj))
    {
	send_to_char("Ты не сможешь с этим расстаться.\n\r", ch);
	return;
    }

    if ((cost = get_cost(keeper, obj, FALSE)) <= 0)
    {
	act("$n даже не смотрит на $p6.", keeper, obj, ch, TO_VICT);
	return;
    }

    sprintf(buf,
	    "$n говорит тебе: {RЯ дам тебе %d серебра и %d золота за $p6.{x",
	    cost - (cost/100) * 100, cost/100);
    act(buf, keeper, obj, ch, TO_VICT);
    ch->reply = keeper;

    return;
}

/* wear object as a secondary weapon */
void do_second(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj, *prim;
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if (get_skill(ch, gsn_second_weapon) < 1)
    {
	send_to_char("Ты же всю жизнь воюешь одним оружием!\n\r", ch);
	return;
    }
    if (arg[0] == '\0') /* empty */
    {
	send_to_char("Какое оружие ты хочешь взять во вторую руку?\n\r", ch);
	return;
    }

    if ((obj = get_obj_carry (ch, arg, ch)) == NULL)
    {
	send_to_char("У тебя нет этого.\n\r", ch);
	return;
    }

    if (obj->item_type != ITEM_WEAPON)
    {
	send_to_char("Хе-хе. Это чем ты интересно хочешь вооружиться?\n\r", ch);
	return;
    }

    /* check if the char is using a shield or a held weapon */
    if (!remove_obj(ch, WEAR_SHIELD, TRUE) || !remove_obj(ch, WEAR_HOLD, TRUE))
    {
	send_to_char("Ты не можешь освободить левую руку.\n\r", ch);
	return;
    }

    if (IS_WEAPON_STAT(obj, WEAPON_PRIMARY_ONLY))
    {
	send_to_char("Этим можно вооружиться только как основным оружием..\n\r",
		     ch);
	return;
    }

    /* check that the character is using a first weapon at all */
    if ((prim = get_eq_char (ch, WEAR_WIELD)) == NULL)
    {
	send_to_char ("Возьми сначала оружие в правую руку!\n\r", ch);
	return;
    }

    /* check for str - secondary weapons have to be lighter */
    if (!check_wield_weight(ch, obj, TRUE))
    {
	send_to_char("Это оружие тяжеловато для левой руки.\n\r", ch);
	return;
    }

    /* check if the secondary weapon is at least half as light as
     the primary weapon */
    if (!check_primary_secondary(ch, prim, obj))
    {
	send_to_char ("Твое второе оружие должно быть легче первого.\n\r", ch);
	return;
    }

    if (IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS) ||
	(ch->size < SIZE_LARGE && IS_WEAPON_STAT(prim, WEAPON_TWO_HANDS)))
    {
	send_to_char ("Тебе надо иметь обе руки свободными.\n\r", ch);
	return;
    }

    if (!check_wear_restrict(ch, obj))
	return;

    /* at last - the char uses the weapon */
    /* remove the current weapon if any */
    if (!remove_obj(ch, WEAR_SECONDARY, TRUE))
	return;     /* remove obj tells about any no_remove */

    /* char CAN use the item! that didn't take long at aaall */
    act ("$n берет $p6 в левую руку.", ch, obj, NULL, TO_ROOM);
    act ("Ты берешь $p6 в левую руку.", ch, obj, NULL, TO_CHAR);

    equip_char (ch, obj, WEAR_SECONDARY);
    show_weapon_level(ch, obj, TRUE);
    //    check_improve(ch, NULL, gsn_secondary_mastery, TRUE, 4);
    return;
}

void do_sharpen(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj, *obj1;
    AFFECT_DATA af;
    int percent, skill, mat;

    if ((skill = get_skill(ch, gsn_sharpen)) < 1)
    {
	send_to_char("Ты не знаешь, как затачивать оружие.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	send_to_char ("И чего точить?\n\r", ch);
	return;
    }

    if ((obj = get_obj_list(ch, argument, ch->carrying)) == NULL)
    {
	send_to_char ("У тебя нет этого.\n\r", ch);
	return;
    }

    if ((obj1 = get_eq_char(ch, WEAR_HOLD)) == NULL
	|| ((mat = material_lookup(obj1->material)) != MAT_STONE && mat != MAT_GEMSTONE &&
	    mat != MAT_BLOODSTONE && mat != MAT_DIAMOND))
    {
	send_to_char ("Возьми камень в руку.\n\r", ch);
	return;
    }

    if (obj->item_type == ITEM_WEAPON
	&& (obj->value[0] == WEAPON_SWORD
	    || obj->value[0] == WEAPON_AXE
	    || obj->value[0] == WEAPON_POLEARM
	    || obj->value[0] == WEAPON_DAGGER
	    || obj->value[0] == WEAPON_SPEAR))
    {
	if (IS_WEAPON_STAT(obj, WEAPON_SHARP))
	{
	    act("$p - уже заточено.", ch, obj, NULL, TO_CHAR);
	    return;
	}

	percent = number_percent();

	if (ch->mana < percent/2)
	{
	    send_to_char("Не хватает маны.\n\r", ch);
	    return;
	}

	if (percent < 2*skill/3)
	{

	    af.where     = TO_WEAPON;
	    af.type      = gsn_sharpen;
	    af.level     = ch->level;
	    af.duration  = ch->level * (percent + skill) / 50;
	    af.location  = 0;
	    af.modifier  = 0;
	    af.bitvector = WEAPON_SHARP;
	    affect_to_obj(obj, &af);

	    act("$n берет камень и затачивает $p6.", ch, obj, NULL, TO_ROOM);
	    act("Ты затачиваешь $p6.", ch, obj, NULL, TO_CHAR);
	    check_improve(ch, NULL, gsn_sharpen, TRUE, 3);
	    ch->mana -= percent/2;
	    make_worse_condition(ch, WEAR_HOLD, 50, DAM_SLASH);
	    obj->condition -= 2;
	    check_condition(ch, obj);
	}
	else
	{
	    act("$n безуспешно пытается наточить $p6.", ch, obj, NULL, TO_ROOM);
	    act("У тебя не получается наточить $p6.", ch, obj, NULL, TO_CHAR);
	    ch->mana-=percent/4;
	    check_improve(ch, NULL, gsn_sharpen, FALSE, 3);
	    make_worse_condition(ch, WEAR_HOLD, 25, DAM_SLASH);
	    obj->condition--;
	    check_condition(ch, obj);
	}
	WAIT_STATE(ch, skill_table[gsn_sharpen].beats);
	return;
    }
    send_to_char("Ты можешь затачивать только мечи, топоры, кинжалы, копья или алебарды.\n\r", ch);
    return;
}

char *get_material(OBJ_DATA *obj)
{
    int i = material_lookup(obj->material);

    return (i == -1) ? obj->material :
	!IS_NULLSTR(material_table[i].rname) ? material_table[i].rname :
	material_table[i].name;
}


float check_material(OBJ_DATA *obj, int16_t dt)
{
    int i;

    for (i = 0; material_waste[i].bash != 0; i++)
	if (material_table[material_lookup(obj->material)].bit == (int64_t)material_waste[i].bit)
	    switch (dt)
	    {
	    case DAM_BASH:	return material_waste[i].bash;
	    case DAM_PIERCE:	return material_waste[i].pierce;
	    case DAM_SLASH:	return material_waste[i].slash;
	    default:		return material_waste[i].other;
	    }

    return 1;
}

void check_condition(CHAR_DATA *ch, OBJ_DATA *obj)
{

    if (obj->condition <= 0)
    {
	act("{W$p развалива$rся на кусочки!{x",
	    ch, obj, NULL, TO_ALL);

	/* dump contents */
	dump_container(obj);
    }
}

/* Ухудшить шмотку */
void make_worse_condition(CHAR_DATA *ch, int thing, int dam, int16_t dt)
{
    OBJ_DATA *obj, *obj_next;

    if (dt == DAM_NONE)
	return;

    if (thing > 0 && thing < MAX_WEAR)
    {
	if ((obj = get_eq_char(ch, thing)) == NULL)
	    return;
	dam /= 2*(check_material(obj, dt));
	obj->condition -= dam;
	check_condition(ch, obj);
    }
    else
    {
	int prob;
	/* пройтись по всему инвентарю и экипировке */
	for (obj = ch->carrying; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;
	    prob = 0;

	    switch(obj->wear_loc)
	    {
	    case WEAR_NONE:	prob =  2; break;
	    case WEAR_LIGHT:	prob =  4; break;
	    case WEAR_FINGER_L:	prob =  5; break;
	    case WEAR_FINGER_R:	prob =  5; break;
	    case WEAR_NECK_1:	prob =  5; break;
	    case WEAR_NECK_2:	prob =  5; break;
	    case WEAR_BODY:	prob = 25; break;
	    case WEAR_HEAD:	prob = 19; break;
	    case WEAR_LEGS:	prob = 17; break;
	    case WEAR_FEET:	prob =  5; break;
	    case WEAR_HANDS:	prob = 18; break;
	    case WEAR_ARMS:	prob = 16; break;
	    case WEAR_SHIELD:	prob = 26; break;
	    case WEAR_ABOUT:	prob = 20; break;
	    case WEAR_WAIST:	prob =  8; break;
	    case WEAR_WRIST_L:	prob =  8; break;
	    case WEAR_WRIST_R:	prob =  8; break;
	    case WEAR_HOLD:	prob =  4; break;
	    case WEAR_FLOAT:	prob =  5; break;
	    default: 		break;
	    }

	    if (number_range(0, 500) < prob)
	    {
		obj->condition = obj->condition - dam/(check_material(obj, dt)*12);
		check_condition(ch, obj);
	    }
	}
    }
}

void do_reinforce(CHAR_DATA *ch, char *argument)
{
    int chance, percent;
    OBJ_DATA *obj1, *obj2;


    if ((chance = get_skill(ch, gsn_reinforce)) < 1)
    {
	send_to_char("Ты не умеешь укреплять броню.\n\r", ch);
	return;
    }

    if ((obj1 = get_obj_carry(ch, argument, ch)) == NULL)
    {
	send_to_char("У тебя нет такой вещи.\n\r", ch);
	return;
    }

    if (obj1->item_type != ITEM_ARMOR)
    {
	send_to_char("Это не броня.\n\r", ch);
	return;
    }

    if (obj1->level > ch->level)
    {
	send_to_char("Ты не сможешь укрепить такую мощную вещь.\n\r", ch);
	return;
    }

    if (obj1->enchanted)
    {
	send_to_char("Эту вещь ты не сможешь укрепить.\n\r", ch);
	return;
    }

    if (obj1->pIndexData->edited)
    {
	send_to_char("Подожди немного, попробуй улучшить эту вещь минут через "
		     "пять. Возможно, у тебя это получится.\n\r", ch);
	return;
    }

    for (obj2 = ch->carrying; obj2 != NULL; obj2 = obj2->next_content)
	if (obj2 != obj1 && obj2->pIndexData == obj1->pIndexData)
	    break;

    if (obj2 == NULL)
    {
	send_to_char("У тебя нет второго экземпляра такой брони.\n\r", ch);
	return;
    }

    if (check_auction_obj(ch, obj2))
	return;

    percent = number_percent();

    if (ch->mana < percent)
    {
	send_to_char("Не хватает маны.\n\r", ch);
	return;
    }

    if (chance > percent)
    {
	AFFECT_DATA *paf;
	const int mult = 500;

	act("$n укрепляет $p6!", ch, obj1, NULL, TO_ROOM);
	act("Ты укрепляешь $p6!", ch, obj1, NULL, TO_CHAR);
	ch->mana -= percent;
	check_improve(ch, NULL, gsn_reinforce, TRUE, 3);

	obj1->enchanted = TRUE;
	obj1->condition = UMIN(100, obj1->condition + obj2->condition/(400/percent));


	obj1->value[0] += UMAX(1, obj1->value[0] * (chance + percent)/mult);
	obj1->value[1] += UMAX(1, obj1->value[1] * (chance + percent)/mult);
	obj1->value[2] += UMAX(1, obj1->value[2] * (chance + percent)/mult);
	obj1->value[3] += UMAX(1, obj1->value[3] * (chance + percent)/mult);

	for (paf = obj1->pIndexData->affected; paf != NULL; paf = paf->next)
	    if (chance > number_percent())
		affect_copy(obj1, paf);
    }
    else
    {
	act("$n безуспешно пытается укрепить $p6.", ch, obj1, NULL, TO_ROOM);
	act("У тебя не получается укрепить $p6.", ch, obj1, NULL, TO_CHAR);
	ch->mana-= percent/4;
	check_improve(ch, NULL, gsn_reinforce, FALSE, 3);
    }

    extract_obj(obj2, TRUE, TRUE);

    WAIT_STATE(ch, skill_table[gsn_reinforce].beats);
}

void do_activate(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *rod;
    OBJ_DATA *obj = NULL;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	if ((rod = get_eq_char(ch, WEAR_HOLD)) == NULL)
	{
	    send_to_char("Тереть что?\n\r", ch);
	    return;
	}

	if (ch->fighting != NULL)
	    victim = ch->fighting;
	else
	    victim = ch;
    }
    else
    {
	if ((rod = get_obj_wear(ch, arg, TRUE)) == NULL)
	{
	    send_to_char("У тебя нет этого.\n\r", ch);
	    return;
	}

	if (rod->wear_loc == WEAR_NONE)
	{
	    send_to_char("Ты должен это одеть.\n\r", ch);
	    return;
	}

	if (argument[0] == '\0')
	{
	    if (ch->fighting != NULL)
		victim = ch->fighting;
	    else
		victim = ch;
	}
	else
	{
	    victim = get_char_room (ch, NULL, argument, TRUE);
	    obj    = get_obj_here  (ch, NULL, argument);
	}
    }

    if (rod->item_type != ITEM_ROD)
    {
	send_to_char("Ты можешь активировать только пруты.\n\r", ch);
	return;
    }

    if (check_auction_obj(ch, rod))
	return;

    if (check_obj_spells(rod, 1))
    {
	victim = NULL;
	obj    = NULL;
    }
    else if (!victim && !obj)
    {
	send_to_char("Тереть на что или на кого?\n\r", ch);
	return;
    }

    WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

    if (rod->value[3] == 0)
    {
	int chance = 0;

	if (victim != NULL && victim != ch)
	{
	    act("$n активирует $p6 на $N3.", ch, rod,  victim, TO_NOTVICT);
	    act("Ты активируешь $p6 на $N3.", ch, rod, victim, TO_CHAR);
	    act("$n активирует $p6 на тебя.", ch, rod, victim, TO_VICT);
	}
	else if (obj != NULL)
	{
	    act("$n активирует $p6 на $P6.", ch, rod, obj, TO_ROOM);
	    act("Ты активируешь $p6 на $P6.", ch, rod, obj, TO_CHAR);
	}
	else
	{
	    act("$n активирует $p6.", ch, rod, NULL, TO_ROOM);
	    act("Ты активируешь $p6.", ch, rod, NULL, TO_CHAR);
	}

	if (ch->level < rod->level
	    || number_percent() >= 20 + get_skill(ch, gsn_rods) * 4/5)
	{
	    act("Твои попытки активировать $p6 вызвали только дым и искры.",
		ch, rod, NULL, TO_CHAR);
	    act("Попытки $n1 активировать $p6 вызвали только дым и искры.",
		ch, rod, NULL, TO_ROOM);
	    check_improve(ch, NULL, gsn_rods, FALSE, 2);
	    chance = 30;
	}
	else
	{
	    obj_cast_spell(rod->value[1], rod->value[0], ch, victim,
			   obj, argument);
	    check_improve(ch, NULL, gsn_rods, TRUE, 2);
	    chance = 10;
	}

	rod->value[3] = rod->value[2];

	if (number_percent() <= chance)	/* Шанс развалиться */
	{
	    act("$p распада$rся на кусочки!", ch, rod, NULL, TO_ROOM);
	    act("$p распада$rся на кусочки!", ch, rod, NULL, TO_CHAR);
	    extract_obj(rod, TRUE, TRUE);
	}
    }
    else
	send_to_char("Подожди еще немного, этот прут еще не готов к "
		     "активации.\n\r", ch);

    return;
}

void do_herbs(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    AFFECT_DATA af, *paf;
    int hard = 1, mana;

    if (!check_blind(ch))
	return;

    if (!IS_IN_WILD(ch))
    {
	printf_to_char("А где ты тут увидел%s травы?\n\r", ch, SEX_ENDING(ch));
	return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0')
	victim = ch;
    else if ((victim = get_char_room(ch, NULL, arg, FALSE)) == NULL)
    {
	send_to_char("Таких тут нет.\n\r", ch);
	return;
    }

    if (is_affected(victim, gsn_herbs))
    {
	if (ch == victim)
	    send_to_char("Ты пока не можешь есть лечебные травы.\n\r", ch);
	else
	    act("$N пока не может есть лечебные травы.", ch, NULL, victim, TO_CHAR);
	return;
    }

    if (!IS_AWAKE(victim)
	|| POS_FIGHT(victim))
    {
	send_to_char("У тебя не получается привлечь внимание этого "
		     "существа!\n\r", ch);
	return;	/* Давний аладонский глюк - можно травой кормить спящего чара */
    }

    mana = number_range(5, 10);

    if (mana > ch->mana || mana > ch->move)
    {
	send_to_char("У тебя не хватает энергии для собирания трав.\n\r", ch);
	return;
    }

    switch(SECTOR(ch))
    {
    case SECT_FIELD:
    case SECT_HILLS:
    case SECT_FOREST:
	hard = 1;
	break;
    case SECT_MOUNTAIN:
	hard = 5;
	break;
    case SECT_DESERT:
	/* В пустыне тоже что-то можно найти, но крайне редко... */
	hard = 20;
	break;
    default:
	break;
    }

    if (number_percent() > get_skill(ch, gsn_herbs) / hard)
    {
	send_to_char("Тебе не удалось найти лечебных трав.\n\r", ch);
	check_improve(ch, NULL, gsn_herbs, FALSE, 2);
	ch->mana -= mana/2;
	ch->move -= mana/2;

	return;
    }

    ch->mana -= mana;
    ch->move -= mana;

    if (IS_UNDEAD(victim))
    {
	act("Ты кормишь $N3 целебными травами, но они не производят на $S1 никакого действия.", ch, NULL, victim, TO_CHAR);
	act("$n кормит тебя целебными травами, но они не производят никакого действия.", ch, NULL, victim, TO_VICT);
	act("$n кормит $N3 целебными травами, но они не производят на $S1 никакого действия.", ch, NULL, victim, TO_NOTVICT);
	return;
    }

    WAIT_STATE(ch, skill_table[gsn_herbs].beats);
    check_improve(ch, NULL, gsn_herbs, TRUE, 2);

    victim->hit += number_range(ch->level, 5 * ch->level / 2) * (is_animal(victim) ? 2 : 1);

    af.where = TO_AFFECTS;
    af.type = gsn_herbs;
    af.level = ch->level;
    af.duration = ch->level/8;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    if (victim != ch)
    {
	act("Ты кормишь $N3 целебными травами.", ch, NULL, victim, TO_CHAR);
	act("$n кормит тебя целебными травами.", ch, NULL, victim, TO_VICT);
	act("$n кормит $N3 целебными травами.", ch, NULL, victim, TO_NOTVICT);
	//	affect_to_char(ch, &af);
    }
    else
    {
	send_to_char("Ты находишь целебные травы и ешь их.\n\r", ch);
	act("$n ест целебные травы.", ch, NULL, victim, TO_NOTVICT);
    }

    if ((paf = affect_find(victim->affected, gsn_poison)) != NULL
	&& number_percent() < 3 * ch->level / 2 - paf->level)
    {
	affect_strip(victim, gsn_poison);
	send_to_char("Действие яда в твоем теле проходит!\n\r", victim);
    }

    if ((paf = affect_find(victim->affected, gsn_plague)) != NULL
	&& number_percent() < 3 * ch->level / 2 - paf->level)
    {
	affect_strip(victim, gsn_plague);
	send_to_char("Твои болячки проходят!\n\r", victim);
    }
}


void do_butcher(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj, *meat, *wield1, *wield2;
    int skill, num;
    MOB_INDEX_DATA *mob = NULL;

    if ((skill = get_skill(ch, gsn_butcher)) < 1)
    {
	send_to_char("Разделать? А как это?\n\r", ch);
	return;
    }

    one_argument(argument, arg);
    if (arg[0] == '\0')
    {
	send_to_char("Что разделать?\n\r", ch);
	return;
    }

    if ((obj = get_obj_list(ch, arg, ch->in_room->contents)) == NULL)
    {
	send_to_char("Ты не видишь этого здесь!\n\r", ch);
	return;
    }

    wield1 = get_eq_char(ch, WEAR_WIELD);
    wield2 = get_eq_char(ch, WEAR_SECONDARY);

    if ((wield1 != NULL && wield1->value[0] == WEAPON_DAGGER)
	|| (wield2 != NULL && wield2->value[0] == WEAPON_DAGGER))
    {
    }
    else
    {
	send_to_char("Ты не можешь это разделать без кинжала...\n\r", ch);
	return;
    }

    if ((obj->item_type != ITEM_CORPSE_NPC
	 && obj->item_type != ITEM_CORPSE_PC)
	|| !can_loot(ch, obj))
    {
	send_to_char("Ты не можешь это разделать...\n\r", ch);
	return;
    }

    if (obj->item_type == ITEM_CORPSE_NPC)
    {
	if ((mob = get_mob_index(obj->value[4])) == NULL
	    || !IS_SET(mob->form, FORM_EDIBLE)
	    || IS_SET(mob->form, FORM_MAGICAL)     /* магическое создание */
	    || IS_SET(mob->form, FORM_CONSTRUCT)   /* голем, статуя       */
	    || IS_SET(mob->form, FORM_MIST)        /* туман               */
	    || IS_SET(mob->form, FORM_INTANGIBLE)) /* нечто неосязаемое   */
	{
	    send_to_char("Ты не можешь найти на этом трупе ни одного "
			 "съедобного кусочка.\n\r", ch);
	    return;
	}
    }

    if (number_percent() < skill)
    {
	num = UMAX(1, number_fuzzy(skill / 25));
	sprintf(buf, "Ты разделываешь $T6 и вырезаешь из него %d %s мяса.",
		num, hours(num, TYPE_PIECE));
	act(buf, ch, NULL, obj->short_descr, TO_CHAR);
	act("$n разделывает $T6.", ch, NULL, obj->short_descr, TO_ROOM);

	for(;num > 0; num--)
	{
	    meat = create_object(get_obj_index(OBJ_VNUM_MEAT), 0);

	    meat->value[3] = obj->value[3];

	    strcpy(arg, cases(obj->short_descr, 1));

	    strcpy(arg, arg + 6);

	    sprintf(buf , meat->name, arg);
	    free_string(meat->name);
	    meat->name = str_dup(buf);

	    sprintf(buf , meat->short_descr, arg);
	    free_string(meat->short_descr);
	    meat->short_descr = str_dup(buf);

	    sprintf(buf , meat->description, arg);
	    free_string(meat->description);
	    meat->description = str_dup(buf);

	    obj_to_room(meat, ch->in_room);
	}
	check_improve(ch, NULL, gsn_butcher, TRUE, 3);
    }
    else
    {
	send_to_char("Неудачно!\n\r", ch);
	act("$n пытается разделать $T6, но неудачно.",
	    ch, NULL, obj->short_descr, TO_ROOM);
	check_improve(ch, NULL, gsn_butcher, FALSE, 3);
    }

    dump_container(obj);
    WAIT_STATE(ch, skill_table[gsn_butcher].beats);
}

void do_find_spring(CHAR_DATA *ch, char *argument)
{
    int skill;

    if ((skill = get_skill(ch, gsn_find_spring)) < 1)
    {
	send_to_char("Ты не знаешь даже, где ищут родники...\n\r", ch);
	return;
    }

    if (!can_see_room(ch, ch->in_room) || IS_AFFECTED(ch, AFF_BLIND))
    {
	send_to_char("Да ты даже не видишь, где искать!\n\r", ch);
	return;
    }

    if (!IS_IN_WILD(ch))
    {
	send_to_char("Здесь ты вряд ли найдешь воду...\n\r", ch);
	return;
    }

    if (ch->mana < 10)
    {
	send_to_char("У тебя не хватает энергии для того, чтобы отыскать здесь родник.\n\r", ch);
	return;
    }

    if (number_percent() < skill)
    {
	OBJ_DATA *spring = create_object(get_obj_index(OBJ_VNUM_SPRING_RANGER), 0);

	spring->timer = ch->level/2;
	obj_to_room(spring, ch->in_room);
	act("$n внимательно изучает землю, выкапывает ямку, "
	    "и оттуда начинает бить ключ!", ch, NULL, NULL, TO_ROOM);
	send_to_char("Родник начинает бить из-под земли!\n\r", ch);
	check_improve(ch, NULL, gsn_find_spring, TRUE, 3);
	ch->mana -= 10;
    }
    else
    {
	send_to_char("У тебя не получается найти родник.\n\r", ch);
	check_improve(ch, NULL, gsn_find_spring, FALSE, 3);
	ch->mana -= 5;
    }

    WAIT_STATE(ch, skill_table[gsn_find_spring].beats);
    return;
}

int16_t do_trap_flags(CHAR_DATA *ch, char *argument)
{
    int16_t flag;

    if (!IS_NULLSTR(argument)
	&& ((flag = flag_value(trap_flags, argument)) != NO_FLAG))
    {
	return flag;
    }
    else
	return 0;
}


void do_disarm_trap(CHAR_DATA *ch, char *argument)
{
    int chance;
    OBJ_DATA *obj;
    OBJ_DATA *trap;
    EXIT_DATA *texit;
    int door;

    if (IS_NPC(ch))
    {
	send_to_char("Твои конечности не предназначены для этого.\n\r", ch);
	return;
    }

    if ((chance = get_skill(ch, gsn_disarm_trap)) < 1)
    {
	send_to_char("Чего?\n\r", ch);
	return;
    }

    if (IS_NULLSTR(argument)
	|| !str_prefix(argument, "room")
	|| !str_prefix(argument, "комната"))
    {
	trap = get_trap(ch->in_room);
	if (!trap
	    || number_percent() > get_skill(ch, gsn_detect_trap))
	{
	    send_to_char("Ты не видишь тут ни одной ловушки.\n\r", ch);
	    return;
	}

	chance += (get_curr_stat(ch, STAT_INT) - MAX_STAT) * 2;
	chance += (get_curr_stat(ch, STAT_DEX) - MAX_STAT) * 2;
	chance += ch->level - trap->level;
	chance += (ch->level - trap->value[2]) * 3;

	if (number_percent() < chance)
	{
	    act("Ты удачно обезвреживаешь $p6!", ch, trap, NULL, TO_CHAR);
	    act("$n удачно обезвреживает $p6!", ch, trap, NULL, TO_ROOM);
	    SET_BIT(trap->wear_flags, ITEM_TAKE);
	    trap->value[2] = 0;
	    obj_from_room(trap);
	    obj_to_char(trap, ch);
	    check_improve(ch, NULL, gsn_disarm_trap, TRUE, 4);
	}
	else
	{
	    act("Неудачно! Ты задеваешь незаметный рычажок, и $p срабатывает!",
		ch, trap, NULL, TO_CHAR);
	    act("$n задевает незаметный рычажок, и $p срабатывает!",
		ch, trap, NULL, TO_ROOM);
	    trap_damage(ch, trap);
	    extract_obj(trap, TRUE, FALSE);
	    check_improve(ch, NULL, gsn_disarm_trap, FALSE, 4);
	}
    }
    else if ((obj = get_obj_list(ch, argument, ch->in_room->contents)) != NULL)
    {
	trap = obj->trap;
	if (!trap
	    || number_percent() > get_skill(ch, gsn_detect_trap))
	{
	    send_to_char("Ты не видишь тут ни одной ловушки.\n\r", ch);
	    return;
	}

	chance += (get_curr_stat(ch, STAT_INT) - MAX_STAT) * 2;
	chance += (get_curr_stat(ch, STAT_DEX) - MAX_STAT) * 2;
	chance += ch->level - trap->level;
	chance += (ch->level - trap->value[2]) * 3;

	if (number_percent() < chance)
	{
	    act("Ты удачно обезвреживаешь $p6!", ch, trap, NULL, TO_CHAR);
	    act("$n удачно обезвреживает $p6!", ch, trap, NULL, TO_ROOM);
	    SET_BIT(trap->wear_flags, ITEM_TAKE);
	    trap->value[2] = 0;
	    obj->trap = NULL;
	    obj->trap_flags = 0;
	    obj_to_char(trap, ch);
	    check_improve(ch, NULL, gsn_disarm_trap, TRUE, 4);
	}
	else
	{
	    act("Неудачно! Ты задеваешь незаметный рычажок, и $p срабатывает!",
		ch, trap, NULL, TO_CHAR);
	    act("$n задевает незаметный рычажок, и $p срабатывает!",
		ch, trap, NULL, TO_ROOM);
	    trap_damage(ch, trap);
	    obj->trap = NULL;
	    obj->trap_flags = 0;
	    extract_obj(trap, TRUE, FALSE);
	    check_improve(ch, NULL, gsn_disarm_trap, FALSE, 4);
	}
    }
    else if ((door = find_exit(ch, argument)) != -1)
    {
	texit = ch->in_room->exit[door];
	trap = texit->trap;
	if (!trap
	    || number_percent() > get_skill(ch, gsn_detect_trap))
	{
	    send_to_char("Ты не видишь тут ни одной ловушки.\n\r", ch);
	    return;
	}

	chance += (get_curr_stat(ch, STAT_INT) - MAX_STAT) * 2;
	chance += (get_curr_stat(ch, STAT_DEX) - MAX_STAT) * 2;
	chance += ch->level - trap->level;
	chance += (ch->level - trap->value[2]) * 3;

	if (number_percent() < chance)
	{
	    act("Ты удачно обезвреживаешь $p6!", ch, trap, NULL, TO_CHAR);
	    act("$n удачно обезвреживает $p6!", ch, trap, NULL, TO_ROOM);
	    SET_BIT(trap->wear_flags, ITEM_TAKE);
	    trap->value[2] = 0;
	    texit->trap = NULL;
	    texit->trap_flags = 0;
	    obj_to_char(trap, ch);
	    check_improve(ch, NULL, gsn_disarm_trap, TRUE, 4);
	}
	else
	{
	    act("Неудачно! Ты задеваешь незаметный рычажок, и $p срабатывает!",
		ch, trap, NULL, TO_CHAR);
	    act("$n задевает незаметный рычажок, и $p срабатывает!",
		ch, trap, NULL, TO_ROOM);
	    trap_damage(ch, trap);
	    texit->trap = NULL;
	    texit->trap_flags = 0;
	    extract_obj(trap, TRUE, FALSE);
	    check_improve(ch, NULL, gsn_disarm_trap, FALSE, 4);
	}
    }

    WAIT_STATE(ch, skill_table[gsn_disarm_trap].beats);
}

void do_trap(CHAR_DATA *ch, char *argument)
{
    int skill;
    OBJ_DATA *trap;
    OBJ_DATA *obj;
    char arg1[MAX_STRING_LENGTH];
    int door;

    if (IS_NPC(ch))
    {
	send_to_char("Твои конечности не предназначены для этого.\n\r", ch);
	return;
    }

    if ((skill = get_skill(ch, gsn_trap)) < 1)
    {
	printf_to_char("Ты не знаешь, как ставить %s!\n\r",
		       ch,
		       ch->classid == CLASS_RANGER
		       ? "капканы"
		       : "ловушки");
	return;
    }

    if (!IS_IN_WILD(ch) && ch->classid == CLASS_RANGER)
    {
	send_to_char("К сожалению, здесь твой капкан будет слишком заметен...\n\r", ch);
	return;
    }
    else if (!IS_IN_TOWN(ch) && ch->classid == CLASS_THIEF)
    {
	send_to_char("Извини, но ты же не егерь...\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg1);

    if ((trap = get_obj_list(ch, arg1, ch->carrying)) == NULL)
    {
	send_to_char("У тебя нет этого.\n\r", ch);
	return;
    }

    if (trap->item_type != ITEM_TRAP)
    {
	send_to_char("Это не ловушка.\n\r", ch);
	return;
    }

    if (trap->level > ch->level)
    {
	send_to_char("Ты не сможешь управиться с этой ловушкой.\n\r", ch);
	return;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_SAFE))
    {
	send_to_char("Здесь нельзя ставить ловушки.\n\r", ch);
	return;
    }

    if (IS_NULLSTR(argument))
    {
	if (get_trap(ch->in_room) != NULL)
	{
	    send_to_char("Здесь уже стоит ловушка.\n\r", ch);
	    return;
	}

	if (number_percent() < skill)
	{
	    trap->timer = 2 * ch->level;
	    REMOVE_BIT(trap->wear_flags, ITEM_TAKE);
	    trap->value[2] = ch->level;
	    obj_from_char(trap, TRUE);
	    obj_to_room(trap, ch->in_room);
	    act("$n незаметно для всех устанавливает ловушку.",
		ch, NULL, NULL, TO_ROOM);
	    send_to_char("Ты ставишь ловушку!\n\r", ch);
	    check_improve(ch, NULL, gsn_trap, TRUE, 3);
	}
	else
	{
	    send_to_char("У тебя не получается поставить ловушку.\n\r", ch);
	    check_improve(ch, NULL, gsn_trap, FALSE, 3);
	    extract_obj(trap, TRUE, TRUE);
	}
    }
    else
    {
	argument = one_argument(argument, arg1);

	if ((obj = get_obj_here(ch, NULL, arg1)) != NULL
	    && (obj->item_type == ITEM_PORTAL
		|| obj->item_type == ITEM_CONTAINER))
	{
	    if (obj->trap)
	    {
		send_to_char("Здесь уже стоит ловушка.\n\r", ch);
		return;
	    }

	    if (number_percent() < skill)
	    {
		trap->timer = 2 * ch->level;
		REMOVE_BIT(trap->wear_flags, ITEM_TAKE);
		trap->value[2] = ch->level;
		obj_from_char(trap, TRUE);
		obj->trap = trap;
		trap->in_obj = obj;

		obj->trap_flags = do_trap_flags(ch, argument);

		if (obj->trap_flags == 0)
		{
		    if (obj->item_type == ITEM_PORTAL)
			obj->trap_flags = TRAP_ON_ENTER;
		    else
			obj->trap_flags = TRAP_ON_OPEN;
		}

		act("$n незаметно для всех устанавливает ловушку.",
		    ch, NULL, NULL, TO_ROOM);
		send_to_char("Ты ставишь ловушку!\n\r", ch);
		check_improve(ch, NULL, gsn_trap, TRUE, 3);
	    }
	    else
	    {
		send_to_char("У тебя не получается поставить ловушку.\n\r", ch);
		check_improve(ch, NULL, gsn_trap, FALSE, 3);
		extract_obj(trap, TRUE, TRUE);
	    }
	}
	else if ((door = find_exit(ch, arg1)) >= 0)
	{
	    EXIT_DATA *exit;

	    exit = ch->in_room->exit[door];

	    if (exit->trap)
	    {
		send_to_char("Здесь уже стоит ловушка.\n\r", ch);
		return;
	    }

	    if (number_percent() < skill)
	    {
		trap->timer = 2 * ch->level;
		REMOVE_BIT(trap->wear_flags, ITEM_TAKE);
		trap->value[2] = ch->level;
		obj_from_char(trap, TRUE);
		exit->trap = trap;
		trap->in_room = ch->in_room;

		exit->trap_flags = do_trap_flags(ch, argument);

		if (exit->trap_flags == 0)
		    exit->trap_flags = TRAP_ON_ENTER;

		act("$n незаметно для всех устанавливает ловушку.",
		    ch, NULL, NULL, TO_ROOM);
		send_to_char("Ты ставишь ловушку!\n\r", ch);
		check_improve(ch, NULL, gsn_trap, TRUE, 3);
	    }
	    else
	    {
		send_to_char("У тебя не получается поставить ловушку.\n\r", ch);
		check_improve(ch, NULL, gsn_trap, FALSE, 3);
		extract_obj(trap, TRUE, TRUE);
	    }

	}
	else
	{
	    send_to_char("Ты не видишь этого здесь.\n\r", ch);
	    return;
	}
    }

    WAIT_STATE(ch, skill_table[gsn_trap].beats);
    return;
}

void do_torch(CHAR_DATA *ch, char *argument)
{
    int skill;

    if ((skill = get_skill(ch, gsn_torch)) < 1)
    {
	send_to_char("Ты не знаешь даже, как зажечь факел...\n\r", ch);
	return;
    }

    if (!IS_IN_FOREST(ch))
    {
	send_to_char("Здесь ты вряд ли найдешь сухую палку для факела...\n\r",
		     ch);
	return;
    }

    if (ch->mana < 10)
    {
	send_to_char("У тебя не хватает энергии для того, чтобы найти подходящую палку и зажечь факел.\n\r", ch);
	return;
    }

    if (number_percent() < skill)
    {
	OBJ_DATA *torch = create_object(get_obj_index(OBJ_VNUM_TORCH), 0);

	torch->timer = ch->level;
	obj_to_char(torch, ch);
	act("$n подбирает сухую палку и зажигает факел!",
	    ch, NULL, NULL, TO_ROOM);
	send_to_char("Ты подбираешь сухую палку, и зажигаешь факел!\n\r", ch);
	check_improve(ch, NULL, gsn_torch, TRUE, 3);
	ch->mana -= 10;
    }
    else
    {
	send_to_char("У тебя не получается зажечь факел.\n\r", ch);
	check_improve(ch, NULL, gsn_torch, FALSE, 3);
	ch->mana -= 5;
    }

    WAIT_STATE(ch, skill_table[gsn_torch].beats);
    return;
}

void do_repair(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    CHAR_DATA *sm;
    int qp;
    bool fCost = FALSE;

    if (IS_NPC(ch))
	return;

    LIST_FOREACH(sm, &ch->in_room->people, room_link)
    {
	if (IS_NPC(sm) && IS_SET(sm->act, ACT_SMITHER))
	    break;
    }

    if (sm == NULL)
    {
	send_to_char("Ты не можешь здесь ничего починить!\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Что ты хочешь починить?\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "цена") || !str_cmp(arg, "cost"))
    {
	one_argument(argument, arg);
	if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
	{
	    send_to_char("Но у тебя этого нет!\n\r", ch);
	    return;
	}

	qp = (UMAX(obj->level, 1) * (is_limit(obj) != -1 ? 3 : 1));
	sprintf(arg, "Починить эту вещь будет стоить тебе %d %s удачи.",
		qp, hours(qp, TYPE_POINTS));
	do_say(sm, arg);
	return;
    }

    if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
    {
	send_to_char("Но у тебя этого нет!\n\r", ch);
	return;
    }

    if (obj->condition >= 100)
    {
	send_to_char("Эта вещь не нуждается в починке.\n\r", ch);
	return;
    }

    if (obj->pIndexData->edited)
    {
	send_to_char("Эту вещь в данное время починить нельзя. Попытайся через несколько минут.\n\r", ch);
	return;
    }

    if (IS_SET(obj->extra_flags, ITEM_NOREMOVE) && obj->wear_loc != WEAR_NONE)
    {
	send_to_char("Ты не можешь с этим расстаться!\n\r", ch);
	return;
    }

    qp = UMAX(obj->level, 1);

    if (is_limit(obj) != -1)
	qp *= 3;

    if (fCost)
    {
	sprintf(arg, "Починить эту вещь будет стоить тебе %d %s удачи.",
		qp, hours(qp, TYPE_POINTS));
	do_say(sm, arg);
	return;
    }

    if (ch->pcdata->quest_curr < qp)
    {
	send_to_char("У тебя нет требуемого количества пунктов удачи.\n\r", ch);
	return;
    }

    ch->pcdata->quest_curr -= qp;
    obj->condition = UMIN(100, obj->condition + number_range(25, 75));

    act("$N починил $p6.", ch, obj, sm, TO_ALL);
}

void do_csharpen(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    CHAR_DATA *sm;
    AFFECT_DATA af;
    int qp;
    bool price = FALSE;

    if (IS_NPC(ch))
	return;

    LIST_FOREACH(sm, &ch->in_room->people, room_link)
    {
	if (IS_NPC(sm) && IS_SET(sm->act, ACT_SMITHER))
	    break;
    }

    if (sm == NULL)
    {
	send_to_char("Ты не можешь здесь ничего отточить!\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Что ты хочешь отточить?\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "цена") || !str_cmp(arg, "cost"))
    {
	one_argument(argument, arg);
	price = TRUE;
    }

    if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
    {
	send_to_char("Но у тебя этого нет!\n\r", ch);
	return;
    }

    if (obj->item_type != ITEM_WEAPON)
    {
	send_to_char("Это не оружие.\n\r",ch);
	return;
    }

    if (IS_SET(obj->extra_flags, ITEM_NOREMOVE) && obj->wear_loc != WEAR_NONE)
    {
	send_to_char("Ты не можешь с этим расстаться!\n\r", ch);
	return;
    }

    if (obj->value[0] != WEAPON_SWORD
	&& obj->value[0] != WEAPON_AXE
	&& obj->value[0] != WEAPON_POLEARM
	&& obj->value[0] != WEAPON_DAGGER
	&& obj->value[0] != WEAPON_SPEAR)
    {
	send_to_char("Можно затачивать только мечи, топоры, кинжалы, копья или алебарды.\n\r", ch);
	return;
    }

    if (IS_WEAPON_STAT(obj, WEAPON_SHARP))
    {
	act("$p - уже заточено.", ch, obj, NULL, TO_CHAR);
	return;
    }

    qp = obj->level * (is_limit(obj) != -1 ? 3 : 1) / 2;

    if (price)
    {
	sprintf(arg, "Отточить эту вещь будет стоить тебе %d %s удачи.",
		qp, hours(qp, TYPE_POINTS));
	do_say(sm, arg);
	return;
    }

    if (ch->pcdata->quest_curr < qp)
    {
	send_to_char("У тебя нет требуемого количества пунктов удачи.\n\r", ch);
	return;
    }

    af.where     = TO_WEAPON;
    af.type      = gsn_sharpen;
    af.level     = sm->level;
    af.duration  = sm->level/2;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = WEAPON_SHARP;
    affect_to_obj(obj, &af);

    act("$n берет камень и затачивает $p6.", sm, obj, NULL, TO_ROOM);
    obj->condition -= 2;
    check_condition(ch, obj);
    ch->pcdata->quest_curr -= qp;
}


bool can_create_socket(OBJ_DATA *obj);


// #Гнездо у кузнеца
void do_socketing(CHAR_DATA *ch, char *argument){
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    CHAR_DATA *sm;
    int qp;
    bool fCost = FALSE;

    if (IS_NPC(ch))	return;

    LIST_FOREACH(sm, &ch->in_room->people, room_link){
		if (IS_NPC(sm) && IS_SET(sm->act, ACT_SMITHER)) break;
    }

    if (sm == NULL){
		send_to_char("Здесь тебе никто не поможет сделать гнездо в "
				 "твоей вещи!\n\r", ch);
		return;
    }

    argument = one_argument(argument, arg);

    if (arg[0] == '\0'){
		send_to_char("В какой вещи ты хотел сделать гнездо?\n\r", ch);
		return;
    }

    if (!str_cmp(arg, "цена") || !str_cmp(arg, "cost")){
		one_argument(argument, arg);
		fCost = TRUE;
    }

    if ((obj = get_obj_carry(ch, arg, ch)) == NULL){
		send_to_char("Но у тебя этого нет!\n\r", ch);
		return;
    }

    if (IS_OBJ_STAT(obj, ITEM_HAS_SOCKET)){
		send_to_char("Но тут уже есть гнездо!\n\r", ch);
		return;
    }

    if (obj->enchanted || !can_create_socket(obj) || is_limit(obj) != -1){
		send_to_char("В этой вещи нельзя сделать гнездо. Возможно, она была либо улучшена, либо является лимитной.\n\r", ch);
		return;
    }

    if (obj->pIndexData->edited){
		send_to_char("В этой вещи в данное время нельзя сделать гнездо. Попытайся через несколько минут.\n\r", ch);
		return;
    }

    if (IS_SET(obj->extra_flags, ITEM_NODROP)){
		send_to_char("Ты не можешь с этим расстаться!\n\r", ch);
		return;
    }

    qp = 25 + 2 * UMAX(obj->level, 1);

    if (fCost){
		sprintf(arg, "Сделать гнездо в этой вещи будет стоить тебе %d %s удачи.",
			qp, hours(qp, TYPE_POINTS)
			);

		do_say(sm, arg);
		return;
    }

    if (ch->pcdata->quest_curr < qp){
		send_to_char("У тебя нет требуемого количества пунктов удачи.\n\r", ch);
		return;
    }

    ch->pcdata->quest_curr -= qp;

    SET_BIT(obj->extra_flags, ITEM_HAS_SOCKET);
    /*    affect_enchant(obj); */

    act("$N делает гнездо в $p5.", ch, obj, sm, TO_ALL);
}

void do_renaming(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    CHAR_DATA *sm;
    int qp;
    bool fCost = FALSE;

    if (IS_NPC(ch))
	return;

    LIST_FOREACH(sm, &ch->in_room->people, room_link)
    {
	if (IS_NPC(sm) && IS_SET(sm->act, ACT_SMITHER))
	    break;
    }

    if (sm == NULL)
    {
	send_to_char("Здесь тебе никто не поможет сделать личную вещь.\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Какую вещь ты хотел переименовать?\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "цена") || !str_cmp(arg, "cost"))
    {
	one_argument(argument, arg);
	fCost = TRUE;
    }

    if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
    {
	send_to_char("Но у тебя этого нет!\n\r", ch);
	return;
    }

    if (obj->enchanted || is_limit(obj) != -1)
    {
	send_to_char("Эту вещь уже нельзя переименовать.\n\r", ch);
	return;
    }

    if (!IS_NULLSTR(obj->owner))
    {
	send_to_char("У этого уже есть владелец.\n\r", ch);
	return;
    }

	// PERSON
    if (obj->item_type == ITEM_ARMOR || obj->item_type == ITEM_WEAPON || obj->item_type == ITEM_CONTAINER
	|| IS_SET(obj->extra_flags, ITEM_NO_PERSONALIZE))
    {
	send_to_char("Извини, вещи такого типа переименовать нельзя.\n\r", ch);
	return;
    }

    if (IS_SET(obj->extra_flags, ITEM_NODROP) || 
	(IS_SET(obj->extra_flags, ITEM_NOREMOVE) && obj->wear_loc != WEAR_NONE))
    {
	send_to_char("Ты не можешь с этим расстаться!\n\r", ch);
	return;
    }

    qp = (20 + 3 * UMAX(obj->level, 1))*10;

    if (fCost)
    {
	sprintf(arg, "Переименовать эту вещь будет стоить тебе %d %s удачи.",
		qp, hours(qp, TYPE_POINTS));
	do_say(sm, arg);
	return;
    }

    if (ch->pcdata->quest_curr < qp)
    {
	send_to_char("У тебя нет требуемого количества пунктов удачи.\n\r", ch);
	return;
    }

    ch->pcdata->quest_curr -= qp;
    act("$N переименовывает $p6!", ch, obj, sm, TO_ALL);

    obj->owner = str_dup(ch->name);

    sprintf(arg, "Любимая вещь %s - %s - лежит на земле.", cases(ch->name, 1), obj->short_descr);
    free_string(obj->description);
    obj->description = str_dup(arg);

    sprintf(arg, "%s {/%s{x", obj->short_descr, cases(ch->name, 1));
    free_string(obj->short_descr);
    obj->short_descr = str_dup(arg);
}

/*
 * Аукцион
 * Разнесено по разным функциям, потому как не люблю
 * гигантских листингов одной функции.
 *
 * Английский вариант дан от фонаря - плохо у меня с буржуйским
 */

static void auction_list(CHAR_DATA *ch)
{
    int i;
    char buf[MAX_STRING_LENGTH];

    send_to_char("{aСписок идущих аукционов:\n\r", ch);
    for (i = 0; i < MAX_AUCTION; i++)
    {
	if (!auctions[i].valid)
	    continue;

	sprintf(buf, "#%d: Предмет '%s{a' (уровень %d), владелец '%s', "
		"текущая ставка %d золота, время до окончания %d %s\n\r",
		i + 1,
		PERS_OBJ(ch, auctions[i].obj, 0),
		auctions[i].obj->level,
		PERS(auctions[i].sender, ch, 0),
		auctions[i].cost, 3 - auctions[i].time,
		hours(3 - auctions[i].time, TYPE_HOURS));
	send_to_char(buf, ch);
    }
    send_to_char("{x", ch);
}

static void auction_put(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj;
    bool found = FALSE;
    int i;
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;


    if (argument[0] == '\0')
    {
	send_to_char("{aФормат: аукцион поставить <предмет> [начальная цена]\n\r{x", ch);
	return;
    }

    argument = one_argument(argument, arg1);
    one_argument(argument, arg2);

    if (!is_number(arg2) && (arg2[0] != '\0'))
    {
	send_to_char("{aНачальная цена должна быть числом.{x\n\r", ch);
	return;
    }

    if ((obj = get_obj_carry(ch, arg1, ch)) == NULL)
    {
	send_to_char("У тебя этого нет.\n\r", ch);
	return;
    }

    if ((obj->item_type == ITEM_CORPSE_NPC)
	|| (obj->item_type == ITEM_CORPSE_PC)
	|| (obj->item_type == ITEM_FOOD)
	|| (obj->item_type == ITEM_MORTAR)
	|| (obj->item_type == ITEM_MAP))
    {
	send_to_char("{aПравила аукциона запрещают продажу подобных предметов с торгов!{x\n\r", ch);
	return;
    }

    if (obj->wear_loc != -1)
    {
	send_to_char("{aНо... Ты же это используешь!{x\n\r", ch);
	return;
    }

    if (IS_SET(obj->extra_flags, ITEM_NODROP))
    {
	send_to_char("{aТы не можешь с этим расстаться!{x\n\r", ch);
	return;
    }

    if ((IS_SET(obj->item_type, ITEM_CONTAINER)
    || IS_SET(obj->item_type, ITEM_CHEST)
    || IS_SET(obj->item_type, ITEM_MORTAR))
    && obj->contains != NULL
    && !IS_IMMORTAL(ch))
    {
	send_to_char("{aТы не можешь продавать контейнеры с содержимым. Сначала выложи оттуда все.{x\n\r", ch);
	return;
    }

    if (is_limit(obj) != -1)
	for (i = 0; i < MAX_AUCTION; i++)
	    if (auctions[i].valid && auctions[i].obj->pIndexData == obj->pIndexData)
	    {
		send_to_char("{aНа аукционе уже есть такой предмет.{x\n\r", ch);
		return;
	    }

    if (!IS_NULLSTR(obj->owner))
    {
	send_to_char("{aИменные предметы нельзя выставлять на аукцион.{x\n\r", ch);
	return;
    }

    if (IS_SET(obj->extra_flags, ITEM_AUCTION))
    {
	send_to_char("{aТы уже продаешь этот предмет на аукционе.{x\n\r", ch);
	return;
    }

    if (obj->pIndexData && (IS_SET(obj->pIndexData->area->area_flags, AREA_NA) 
			    || IS_SET(obj->pIndexData->area->area_flags, AREA_TESTING)))
    {
	send_to_char("{aПредметы из таких зон нельзя выставлять на аукционе.{x\n\r", ch);
	return;
    }

    for (i = 0; i < MAX_AUCTION; i++)
	if (!auctions[i].valid)
	{
	    found = TRUE;
	    break;
	}

    if (!found)
    {
	send_to_char("{aК сожалению, свободных распределителей аукциона нет. Подожди немного.{x\n\r", ch);
	return;
    }

    SET_BIT(obj->extra_flags, ITEM_AUCTION);

    auctions[i].obj = obj;
    auctions[i].sender = ch;
    auctions[i].buyer = NULL;
    auctions[i].cost = (arg2[0] == '\0') ? UMAX(obj->cost/100, 1) : URANGE(1, atoi(arg2), 150000);
    auctions[i].time = 0;
    auctions[i].valid = TRUE;

    /* obj_from_char(obj); */

    REMOVE_BIT(ch->comm, COMM_NOAUCTION);
    sprintf(buf, "{aТы выставляешь предмет '%s{a' на аукцион.{x\n\r", obj->short_descr);
    send_to_char(buf, ch);

    SLIST_FOREACH(d, &descriptor_list, link)
    {
	CHAR_DATA *victim;

	if (d->connected == CON_PLAYING
	    && (victim = CH(d)) != ch
	    && victim->position > POS_SLEEPING
	    && !IS_SET(victim->comm, COMM_NOAUCTION)
	    && !IS_SET(victim->comm, COMM_QUIET))
	{
	    sprintf(buf, "{aНа аукцион поступил новый предмет: '%s{a' (уровень %d)! Начальная ставка %d золота!\n\r{x",
		    PERS_OBJ(victim, obj, 0),
		    auctions[i].obj->level,
		    auctions[i].cost);

	    send_to_char(buf, victim);
	}
    }
}

static void auction_stake(CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int number, cost;
    /*    int silver, gold; */
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    argument = one_argument(argument, arg1);
    one_argument(argument, arg2);
    if ((arg1[0] == '\0') || (arg2[0] == '\0')
	|| (!is_number(arg1)) || (!is_number(arg2)))
    {
	send_to_char("{aФормат: аукцион ставка <# аукциона> <ставка>{x\n\r", ch);
	return;
    }

    number = atoi(arg1) - 1;

    if (number >= MAX_AUCTION || number < 0)
    {
	send_to_char("{aТакого аукциона нет.{x\n\r", ch);
	return;
    }

    if (!auctions[number].valid)
    {
	send_to_char("{aДанный аукцион неактивен.{x\n\r", ch);
	return;
    }

    cost = atoi(arg2);
    if (cost < 0)
    {
	send_to_char("{aЭто как, простите?{x\n\r", ch);
	return;
    }

    if (cost <= auctions[number].cost || cost < auctions[number].cost * 21 / 20)
    {
	send_to_char("{aНовая ставка должна превышать предыдущую как минимум на 5%.{x\n\r", ch);
	return;
    }

    if (auctions[number].sender == ch)
    {
	act("{aТихо сам$t с собою, я веду торговлю...{x", ch, SEX_ENDING(ch), NULL, TO_CHAR);
	return;
    }

    if (auctions[number].obj->level > ch->level)
    {
	send_to_char("{aСначала подрасти немного...{x\n\r", ch);
	return;
    }

    if (ch->pcdata->bank < (cost * 1.1) )
    {
	send_to_char("{aУ тебя нет такого количества золота!{x\n\r", ch);
	return;
    }

    if (!can_see_obj(ch, auctions[number].obj))
    {
	send_to_char("{aНо ты же не видишь, что там продается!{x\n\r", ch);
	return;
    }

    if (is_have_limit(ch, ch, auctions[number].obj))
	return;

    send_to_char("{aПринято.{x\n\r", ch);
    sprintf(buf, "{aАукцион #%d: принята новая ставка от $n1 в размере %d золота.{x", number + 1, cost);

    SLIST_FOREACH(d, &descriptor_list, link)
    {
	CHAR_DATA *victim;

	victim = CH(d);

	if (d->connected == CON_PLAYING
	    && victim != ch
	    && victim->position > POS_SLEEPING
	    && !IS_SET(victim->comm, COMM_NOAUCTION)
	    && !IS_SET(victim->comm, COMM_QUIET))
	{
	    act(buf, ch, NULL, d->character, TO_VICT);
	}
    }

    if (auctions[number].buyer != NULL)
	auctions[number].buyer->pcdata->bank += (auctions[number].cost * 1.1); 

    auctions[number].buyer = ch;
    auctions[number].cost = cost;
    auctions[number].time = 0;

    ch->pcdata->bank -= (cost * 1.1);
}

static void auction_struck(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int number, penal;
    DESCRIPTOR_DATA *d;

    one_argument(argument, arg);
    if ((arg[0] == '\0') || !is_number(arg))
    {
	send_to_char("{aФормат: аукцион снять <# аукциона>{x\n\r", ch);
	return;
    }

    number = atoi(arg) - 1;

    if (number >= MAX_AUCTION || number < 0)
    {
	send_to_char("{aТакого аукциона нет.{x\n\r", ch);
	return;
    }

    if (!auctions[number].valid)
    {
	send_to_char("{aДанный аукцион неактивен.{x\n\r", ch);
	return;
    }

    if (auctions[number].sender != ch)
    {
	send_to_char("{aТы же не выставлял на этот аукцион ничего!{x\n\r", ch);
	return;
    }

    penal = UMAX(1, auctions[number].cost / 3);

    if (ch->gold < penal)
    {

	sprintf(buf, "{aТебе запрещено снимать эту вещь с аукциона, так как у тебя недостаточно золота для оплаты штрафа (%d золота).\n\r", penal);
	send_to_char(buf, ch);
	return;
    }

    ch->gold -= penal;

    auctions[number].valid = FALSE;
    send_to_char("{aПринято.{x\n\r", ch);

    /* obj_to_char(auctions[number].obj, auctions[number].sender); */
    REMOVE_BIT(auctions[number].obj->extra_flags, ITEM_AUCTION);

    if (auctions[number].buyer != NULL)
	auctions[number].buyer->pcdata->bank += (auctions[number].cost * 1.1);


    SLIST_FOREACH(d, &descriptor_list, link)
    {
	CHAR_DATA *victim;

	victim = CH(d);

	if (d->connected == CON_PLAYING
	    && victim != ch
	    && victim->position > POS_SLEEPING
	    && !IS_SET(victim->comm, COMM_NOAUCTION)
	    && !IS_SET(victim->comm, COMM_QUIET))
	{
	    sprintf(buf, "{aАукцион #%d: предмет '%s{a' снят владельцем.{x\n\r",
		    number + 1,
		    PERS_OBJ(victim, auctions[number].obj, 0));
	    send_to_char(buf, victim);
	}
    }
}

static void auction_identify(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    int number;

    one_argument(argument, arg);
    if ((arg[0] == '\0') || !is_number(arg))
    {
	send_to_char("{aФормат: аукцион опознать <# аукциона>{x\n\r", ch);
	return;
    }

    number = atoi(arg) - 1;

    if (number >= MAX_AUCTION || number < 0)
    {
	send_to_char("{aТакого аукциона нет.{x\n\r", ch);
	return;
    }

    if (!auctions[number].valid)
    {
	send_to_char("{aДанный аукцион неактивен.{x\n\r", ch);
	return;
    }

    if (ch->gold < 2 * SAGE_COST)
    {
	send_to_char("{aУ тебя нет такого количества золота.{x\n\r", ch);
	return;
    }

    if (ch == auctions[number].sender)
    {
	send_to_char("{aЭто же твой предмет, опознай его самостоятельно.{x\n\r", ch);
	return;
    }

    if (IS_OBJ_STAT(auctions[number].obj, ITEM_NO_IDENTIFY))
    {
	send_to_char("{aЭтот предмет невозможно опознать.{x\n\r", ch);
	return;
    }

    ch->gold -= 2 * SAGE_COST;

    if (IS_IMMORTAL(ch))
    {
	sprintf(arg, "{r[{RObject %d{r]{x\n\r",
		auctions[number].obj->pIndexData->vnum);
	send_to_char(arg, ch);
    }
    spell_identify(0, 0, ch, auctions[number].obj, 0);
}

static void auction_stat(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    int number;

    one_argument(argument, arg);
    if ((arg[0] == '\0') || !is_number(arg))
    {
	send_to_char("{aФормат: аукцион статистика <# аукциона>{x\n\r", ch);
	return;
    }

    number = atoi(arg) - 1;

    if (number >= MAX_AUCTION || number < 0)
    {
	send_to_char("{aТакого аукциона нет.{x\n\r", ch);
	return;
    }

    if (!auctions[number].valid)
    {
	send_to_char("{aДанный аукцион неактивен.{x\n\r", ch);
	return;
    }

    do_ostat(ch, auctions[number].obj);
}

void do_auction(CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];

    if (IS_NPC(ch))
	return;

    argument = one_argument(argument, arg1);

    if (arg1[0] == '\0')
    {
	if (IS_SET(ch->comm, COMM_NOAUCTION))
	{
	    send_to_char("{aКанал Аукциона включен.{x\n\r", ch);
	    REMOVE_BIT(ch->comm, COMM_NOAUCTION);
	}
	else
	{
	    send_to_char("{aКанал Аукциона выключен.{x\n\r", ch);
	    SET_BIT(ch->comm, COMM_NOAUCTION);
	}
	return;
    }

    if (ch->in_room && IS_SET(ch->in_room->room_flags, ROOM_NOAUCTION))
    {
	send_to_char("В этом месте невозможно воспользоваться аукционом.\n\r", ch);
	return;
    }

    if (is_lycanthrope(ch))
    {
	send_to_char("В таком состоянии ты не можешь пользоваться аукционом.\n\r", ch);
	return;
    }

    if (!str_prefix(arg1, "список") || !str_prefix(arg1, "list"))
    {
	auction_list(ch);
    }
    else if (!str_prefix(arg1, "поставить") || !str_prefix(arg1, "put"))
    {
	auction_put(ch, argument);
    }
    else if (!str_prefix(arg1, "ставка") || !str_prefix(arg1, "bet"))
    {
	auction_stake(ch, argument);
    }
    else if (!str_prefix(arg1, "снять") || !str_prefix(arg1, "struck"))
    {
	auction_struck(ch, argument);
    }
    else if (!str_prefix(arg1, "опознать") || !str_prefix(arg1, "identify"))
    {
	auction_identify(ch, argument);
    }
    else if (is_spec_granted(ch, "auction_stat")
	     && (!str_prefix(arg1, "статистика") || !str_prefix(arg1, "stat")))
    {
	auction_stat(ch, argument);
    }
    else
    {
	printf_to_char("{aДоступные команды: список, поставить, снять, ставка, опознать%s.{x\n\r",
		     ch, is_spec_granted(ch, "auction_stat") ? ", статистика" : "");
    }

    return;
}

void check_auctions(CHAR_DATA *victim, OBJ_DATA *obj, char *msg)
{
    int i;

    if ((victim && !IS_NPC(victim)) || obj)
    {
	for (i = 0; i < MAX_AUCTION; i++)
	    if (auctions[i].valid
		&& ((victim && auctions[i].sender == victim)
		    || (obj && auctions[i].obj == obj)))
	    {
		CHAR_DATA *ch;
		DESCRIPTOR_DATA *d;
		char buf[MAX_STRING_LENGTH];

		auctions[i].valid = FALSE;
		REMOVE_BIT(auctions[i].obj->extra_flags, ITEM_AUCTION);

		if (auctions[i].buyer != NULL)
		    auctions[i].buyer->pcdata->bank += (auctions[i].cost * 1.1);

		SLIST_FOREACH(d, &descriptor_list, link)
		    if (d->connected == CON_PLAYING
			&& (ch = CH(d)) != victim
			&& ch != NULL
			&& ch->position != POS_SLEEPING
			&& !IS_SET(ch->comm, COMM_NOAUCTION)
			&& !IS_SET(ch->comm, COMM_QUIET)
			&& !check_filter(victim, ch))
		    {
			sprintf(buf, "{aАукцион #%d: предмет '%s{a' снят с торгов по причине %s.\n\r{x",
				i + 1,
				PERS_OBJ(ch, auctions[i].obj, 0),
				msg);

			send_to_char(buf, ch);
		    }
	    }
    }
}


void do_weigh(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj;
    char buf[MAX_STRING_LENGTH];

    if ((obj = get_eq_char(ch, WEAR_HOLD)) == NULL
	|| obj->item_type != ITEM_SCALE)
    {
	send_to_char("Тебе надо держать весы в руке.\n\r", ch);
	return;
    }

    if ((obj = get_obj_carry(ch, argument, ch)) == NULL)
    {
	send_to_char("У тебя нет этого.\n\r", ch);
	return;
    }

    sprintf(buf, "Вес %s составляет %d ед.\n\r",
	    cases(obj->short_descr, 1), obj->weight / 10);
    send_to_char(buf, ch);

    make_worse_condition(ch, WEAR_HOLD, obj->weight / 10, DAM_OTHER);
}

/*
 * Ok this is a scribe skill for writing various spells
 * onto a scroll for personal use or for a friend. Certain
 * spells can't be scribed, and you will fail with a low
 * intelligence or wisdom.
 */

char *denied_spells[] =
{
    "power word kill"	,
    "order"		,
    "shapechange"	,
    "earthmaw"		,
    "call animal"	,
    "animate tree"	,
    "call horse"	,
    "resurrect" 	,
    "detect camouflage" ,
    "charm person"	,
    "camouflage"	,
    "roots"		,
    NULL
};

void do_scribe(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *object;
    int sn;
    int mana;
    int i, skill;

    argument = one_argument(argument, arg);

    if (IS_NPC(ch) || (skill = get_skill(ch, gsn_scribe)) < 1)
    {
	send_to_char("Ты ведь не умеешь этого, не так ли?\n\r", ch);
	return;
    }

    if (arg[0] == '\0')
    {
	send_to_char("Начертать какое заклинание?\n\r", ch);
	return;
    }

    if ((sn = find_spell(ch, arg)) <= 0
	|| ch->level < skill_table[sn].skill_level[ch->classid]
	|| ch->pcdata->learned[sn] < 1)
    {
	send_to_char("Ты не можешь начертать на свиток заклинание, "
		     " которого ты не знаешь.\n\r", ch);
	return;
    }

    if ((skill_table[sn].target == TAR_IGNORE))
    {
	send_to_char("Ты не можешь начертать это заклинание на свиток.\n\r",
		     ch);
	return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Начертать заклинание на какой свиток?\n\r", ch);
	return;
    }

    if ((object = get_obj_carry(ch, arg, ch)) == NULL
	|| object->item_type != ITEM_SCROLL)
    {
	send_to_char("У тебя должен быть какой-нибудь свиток.\n\r", ch);
	return;
    }

    if (object->enchanted)
    {
	send_to_char("На этот свиток уже вряд ли получиться что-нибудь записать.\n\r", ch);
	return;
    }

    if (check_auction_obj(ch, object))
	return;

    /*
     * This is specialized for my mud, these are spells
     * that can't be scribed to prevent abuse. Feel free
     * to remove this or change them as you see fit.
     */

    for (i = 0; denied_spells[i] != NULL; i++)
	if (!strcmp(denied_spells[i], skill_table[sn].name))
	{
	    send_to_char("Ты не можешь начертать это заклинание на свиток.\n\r",
			 ch);
	    return;
	}

    for (i = 1; i < 5;i++)
	if (object->value[i] < 1)
	    break;

    if (i == 5)
    {
	send_to_char("На этом свитке нет свободного места.\n\r", ch);
	return;
    }

    mana = UMAX(skill_table[sn].min_mana,
		100 / (2 + ch->level - skill_table[sn].skill_level[ch->classid]));

    if (ch->mana < mana)
    {
	send_to_char("У тебя не хватает энергии для того, чтобы начертать такое заклинание на свиток.\n\r", ch);
	return;
    }

    ch->mana -= mana;

    act("$n начинает писать что-то на $p5.", ch, object, NULL, TO_ROOM);
    WAIT_STATE(ch, skill_table[gsn_scribe].beats);

    if (number_percent() > skill
	|| number_percent() > ((get_curr_stat(ch, STAT_INT) - 13) * 5
			       + (get_curr_stat(ch, STAT_WIS) - 13) * 3))
    {
	send_to_char("Свиток взрывается в твоих руках!\n\r", ch);
	act("Свиток взрывается в руках $n1!", ch, NULL, NULL, TO_ROOM);
	check_improve(ch, NULL, gsn_scribe, FALSE, 3);
	extract_obj(object, TRUE, TRUE);
	return;
    }

    object->value[0] = UMIN(object->value[0], ch->level);

    object->value[i] = sn;
    object->level = UMAX(1, ch->level - number_fuzzy(5));

    sprintf(buf, "%s %s", object->name, get_skill_name(ch, sn, TRUE));
    free_string(object->name);
    object->name = str_dup(buf);

    affect_enchant(object);

    act("$n дописывает на свиток с заклинанием '$t'!",
	ch, get_skill_name(ch, sn, TRUE), NULL, TO_ROOM);
    sprintf(buf, "Ты дописываешь на свиток заклинание '%s'!\n\r",
	    get_skill_name(ch, sn, TRUE));
    send_to_char(buf, ch);
    check_improve(ch, NULL, gsn_scribe, TRUE, 3);
    WAIT_STATE(ch, skill_table[gsn_scribe].beats);
}

void do_make_bag(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj, *bag;
    char name[MAX_INPUT_LENGTH], *arg;
    int skill;
    bool is_bag = FALSE;
    bool is_load = FALSE;
    int vnum = 0;
    char *msg_to, *msg_all;
    int64_t part = 0;

    msg_to = "";
    msg_all = "";

    if ((((obj = get_eq_char(ch, WEAR_WIELD)) == NULL)
	 || (obj->item_type != ITEM_WEAPON) || (obj->value[0] != WEAPON_DAGGER))
	&& (((obj = get_eq_char(ch, WEAR_SECONDARY)) == NULL)
	    || (obj->item_type != ITEM_WEAPON) || (obj->value[0] != WEAPON_DAGGER)))
    {
	send_to_char("Сначала одень нож или кинжал.\n\r",ch);
	return;
    }

    if (IS_NPC(ch) || (skill = get_skill(ch, gsn_make_bag)) < 1)
    {
	send_to_char("Ты ведь не умеешь этого, не так ли?\n\r", ch);
	return;
    }

    argument = one_argument(argument, name);

    if (!str_prefix(name, "сумка")
	|| !str_prefix(name, "сумку")
	|| !str_prefix(name, "bag"))
    {
	is_bag = TRUE;
    }
    else if (!str_prefix(name, "коврик") || !str_prefix(name, "carpet"))
    {
	is_bag = FALSE;
    }
    else
    {
	if (!str_prefix(name, "кишки"))
	{
	    vnum = OBJ_VNUM_GUTS;
	    part = PART_GUTS;
	    msg_to = "Ты вырезаешь кишки из $p1.";
	    msg_all = "$n вырезает кишки из $p1.";
	}
	else if (!str_prefix(name, "голова")
		 || !str_prefix(name, "голову"))
	{
	    vnum = OBJ_VNUM_SEVERED_HEAD;
	    part = PART_HEAD;
	    msg_to = "Ты отрезаешь голову от $p1.";
	    msg_all = "$n отрезает голову от $p1.";
	}
	else if (!str_prefix(name, "сердце"))
	{
	    vnum = OBJ_VNUM_TORN_HEART;
	    part = PART_HEART;
	    msg_to = "Ты вырезаешь сердце из $p1.";
	    msg_all = "$n вырезает сердце из $p1.";
	}
	else if (!str_prefix(name, "рука")
		 || !str_prefix(name, "руку"))
	{
	    vnum = OBJ_VNUM_SLICED_ARM;
	    part = PART_ARMS;
	    msg_to = "Ты отрезаешь руку от $p1.";
	    msg_all = "$n отрезает руку от $p1.";
	}
	else if (!str_prefix(name, "нога")
		 || !str_prefix(name, "ногу"))
	{
	    vnum = OBJ_VNUM_SLICED_LEG;
	    part = PART_LEGS;
	    msg_to = "Ты отрезаешь ногу от $p1.";
	    msg_all = "$n отрезает ногу от $p1.";
	}
	else if (!str_prefix(name, "мозги"))
	{
	    vnum = OBJ_VNUM_BRAINS;
	    part = PART_BRAINS;
	    msg_to = "Ты вырезаешь мозги из $p1.";
	    msg_all = "$n вырезает мозги из $p1.";
	}
	else if (!str_prefix(name, "уши"))
	{
	    vnum = OBJ_VNUM_EAR;
	    part = PART_EAR;
	    msg_to = "Ты отрезаешь уши от $p1.";
	    msg_all = "$n отрезает уши от $p1.";
	}
	else
	{
	    send_to_char("Что ты хотел вырезать из трупа?\n\r", ch);
	    return;
	}
    }

    if ((obj = get_obj_list(ch, argument, ch->in_room->contents)) == NULL)
    {
	send_to_char("Здесь нет такого трупа.\n\r", ch);
	return;
    }


    if ((obj->item_type != ITEM_CORPSE_NPC
	 && obj->item_type != ITEM_CORPSE_PC)
	|| !can_loot(ch, obj))
    {
	send_to_char("Ты не можешь из этого вырезать что-либо...\n\r", ch);
	return;
    }

    /* проверка на кожу у моба, из которого труп. */
    if (obj->item_type == ITEM_CORPSE_NPC && vnum == 0)
    {
	if (!IS_SET(obj->value[0], PART_SKIN))
	{
	    send_to_char("У таких трупов нету кожи.\n\r", ch);
	    return;
	}
    }

    //создание ковриков и сумок оставил, как есть, только проверка - точно ли их хотели вырезать, а не часть тела?
    if (vnum == 0)
    {
	if (skill > number_percent())
	{
	    if (is_bag)
	    {
		bag = create_object(get_obj_index(OBJ_VNUM_BAG), 0);
		bag->wear_flags = ITEM_HOLD|ITEM_TAKE;
		bag->value[0] = obj->level;                 /* Weight capacity */
		bag->value[1] = CONT_CLOSEABLE;             /* Closeable */
		bag->value[2] = -1;                         /* No key needed */
		bag->value[3] = obj->level/5;
		bag->value[4] = 100;
	    }
	    else
	    {
		bag = create_object(get_obj_index(OBJ_VNUM_CARPET), 0);
		bag->wear_flags = ITEM_TAKE;
		bag->value[0] = 1;
		bag->value[1] = 0;
		bag->value[2] = SIT_ON | REST_ON | SLEEP_ON;
		bag->value[3] = 100 + obj->level/2;
		bag->value[4] = 100 + obj->level/2;
	    };

	    bag->timer = 0;
	    bag->weight = 50;
	    bag->level = ch->level;
	    bag->cost = obj->level;

	    arg = one_argument(obj->short_descr, name);

	    sprintf(name, bag->name, arg);
	    free_string(bag->name);
	    bag->name = str_dup(name);

	    sprintf(name, bag->short_descr, arg);
	    free_string(bag->short_descr);
	    bag->short_descr = str_dup(name);

	    sprintf(name, bag->description, arg);
	    free_string(bag->description);
	    bag->description = str_dup(name);

	    if (can_take_weight(ch, get_obj_weight(bag)) && 
		ch->carry_number + get_obj_number(bag) <= can_carry_n(ch))
		obj_to_char(bag, ch);
	    else
		obj_to_room(bag, ch->in_room);

	    act("Ты вырезаешь себе $p6. Выглядит очень красиво.",
		ch, bag, NULL, TO_CHAR);
	    act("$n вырезает себе $p6. Выглядит очень красиво.",
		ch, bag, NULL, TO_ROOM);
	    check_improve(ch, NULL, gsn_make_bag, TRUE, 3);
	}
	else
	{
	    act("Ты безуспешно пытаешься снять кожу с $p1.",
		ch, obj, NULL, TO_CHAR);
	    act("$n безуспешно пытается снять кожу с $p1.",
		ch, obj, NULL, TO_ROOM);
	    check_improve(ch, NULL, gsn_make_bag, FALSE, 3);
	};
    }
    //далее идет вырезаничие части тела
    else
    {
	if (obj->item_type != ITEM_CORPSE_PC)
	{
	    if (IS_SET(obj->value[0], part))
		is_load = TRUE;
	    else
		is_load = FALSE;
	}
	else is_load = TRUE;

	//приступить к созданию объекта
	if (is_load)
	{
	    if (skill > number_percent())
	    {
		bag = create_object(get_obj_index(vnum), 0);

		if (bag->item_type == ITEM_WEAPON)
		{
		    /* arms, legs & tails */
		    bag->level = ch->level;
		    bag->value[1] = 1 + obj->level/20;
		    bag->value[2] = 1 + obj->level/10;
		}

		bag->timer = ch->level;
		bag->weight = 10;
		bag->level = ch->level;
		bag->cost = obj->level;

		arg = one_argument(obj->short_descr, name);

		sprintf(name, bag->name, arg);
		free_string(bag->name);
		bag->name = str_dup(name);

		sprintf(name, bag->short_descr, arg);
		free_string(bag->short_descr);
		bag->short_descr = str_dup(name);

		sprintf(name, bag->description, arg);
		free_string(bag->description);
		bag->description = str_dup(name);

		if (can_take_weight(ch, get_obj_weight(bag)) && 
		    ch->carry_number + get_obj_number(bag) <= can_carry_n(ch))
		    obj_to_char(bag, ch);
		else
		    obj_to_room(bag, ch->in_room);

		act(msg_to,	ch, obj, NULL, TO_CHAR);
		act(msg_all, ch, obj, NULL, TO_ROOM);
		check_improve(ch, NULL, gsn_make_bag, TRUE, 3);
	    }
	    else
	    {
		act("Ты безуспешно пытаешься разделать $p6.",
		    ch, obj, NULL, TO_CHAR);
		act("$n безуспешно пытается разделать $p6.",
		    ch, obj, NULL, TO_ROOM);
		check_improve(ch, NULL, gsn_make_bag, FALSE, 3);
	    }
	}
	else
	{
	    send_to_char("Из этого трупа это вырезать невозможно.\n\r", ch);
	    return;
	}

    }
    WAIT_STATE(ch, skill_table[gsn_make_bag].beats);
    dump_container(obj);
    return;
}

void do_insert(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *artifact, *obj;
    char arg[MAX_STRING_LENGTH];
    AFFECT_DATA *paf;
    EXTRA_DESCR_DATA *ed;

    argument = one_argument(argument, arg);

    if ((artifact = get_obj_carry(ch, arg, ch)) == NULL
	|| artifact->item_type != ITEM_ARTIFACT)
    {
	send_to_char("У тебя должен быть артефакт!\n\r", ch);
	return;
    }

    if (artifact->level > ch->level)
    {
	send_to_char("Ты не сможешь совладать с силой этого артефакта.\n\r",
		     ch);
	return;
    }

    one_argument(argument, arg);

    if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
    {
	act("Во что ты хотел$T вставить $p6?", ch, artifact, SEX_ENDING(ch), TO_CHAR);
	return;
    }

    if (obj->level > ch->level)
    {
	send_to_char("Ты не сможешь справиться с силой вещи, которая "
		     "получится в итоге.\n\r", ch);
	return;
    }

    if (!IS_OBJ_STAT(obj, ITEM_HAS_SOCKET))
    {
	send_to_char("Тут нет гнезда для артефактов.\n\r", ch);
	return;
    }

    if (obj->enchanted)
    {
	send_to_char("Эта вещь была либо улучшена, либо переименована, либо персонализирована, в нее уже нельзя вставить артефакт.\n\r", ch);
	return;
    }

    if (obj->pIndexData->edited)
    {
	send_to_char("Попробуй вставить артефакт в эту вещь попозже.\n\r", ch);
	return;
    }

    if (artifact->level > obj->level)
    {
	send_to_char("Этот артефакт слишком мощный для этой вещи.\n\r", ch);
	return;
    }

    if (check_auction_obj(ch, artifact))
	return;


    obj->extra_flags |= artifact->extra_flags;
    obj->uncomf |= artifact->uncomf;
    obj->unusable |= artifact->unusable;

    obj->weight += artifact->weight;

    REMOVE_BIT(obj->extra_flags, ITEM_HAS_SOCKET);
    SET_BIT(obj->extra_flags, ITEM_MAGIC);

    affect_enchant(obj);

    for (paf = artifact->pIndexData->affected; paf; paf = paf->next)
	affect_copy(obj, paf);

    act("{WТы вставляешь $p6 в $P6!{x", ch, artifact, obj, TO_CHAR);
    act("$n вставляет $p6 в $P6!", ch, artifact, obj, TO_ROOM);

    ed = new_extra_descr();

    sprintf(arg, "Вставлено в гнездо: %s.", artifact->short_descr);
    ed->description = str_dup(arg);
    ed->keyword = str_dup(obj->name);
    ed->next		= obj->extra_descr;
    obj->extra_descr	= ed;
    top_ed++;

    extract_obj(artifact, TRUE, TRUE);
}


/**************************************************************
 **************************************************************
 ****                                                      ****
 ****   Casino Slots by Nikola Sarcevic from Manga nation  ****
 ****  (manga.thedigitalrealities.com port 6969).  If you  ****
 ****  find any bugs or loopholes in the code, contact me  ****
 ****  at fuck@servicememartha.com, you will receive full  ****
 ****  credit for finding the error and correcting it.  I  ****
 ****  require the same respect, being the writer of this  ****
 ****  snippet,  it is required that  you leave a visible  ****
 ****  creditation somewhere on the MUD  using this code,  ****
 ****  one stating that it was written by  'Nikola',  no-  ****
 ****  thing more is required.   Thank you for your time,  ****
 ****  enjoy.                                              ****
 ****                                                      ****
 **************************************************************
 **************************************************************/

//Note:  If you like this, I have also written a craps game that
//       I may make available on Kyndig soon.

/*
 HELP FILE!  This the helpfile I wrote for casino slots, post this
 on your MUD with whatever altercations needed.

 The Casino Slots on Manga Nation have ten possible results in each
roll:  cherry, plum, grape, watermelon, orange, lemon, bar, double bar,
triple bar, or seven.  Getting three of the same results in a row(this
counts diagonally, horizontally, or vertically) wins.  The amount you
win from that particular result depends on what it is, the result calcu-
lations are as follows:

CHERRY (any) (any)  - Win 1/4 of what you bet(horizontally only).
CHERRY CHERRY (any) - Win 1/2 of what you bet(horizontally only).
CHERRY x3           - Win the amount you bet.
PLUM x3             - Win double the amount you bet.
GRAPE x3            - Win triple the amount you bet.
WATERMELON x3       - Win the amount you bet x 4.
ORANGE x3           - Win the amount you bet x 5.
LEMON x3            - Win the amount you bet x 6.
BAR x3              - Win the amount you bet x 7.
2BAR x3             - Win the amount you bet x 8.
3BAR x3             - Win the amount you bet x 9.
SEVEN x3            - Win the amount you bet x 10.
SEVEN x6            - Jackpot, varies(must be aligned diagonally, both
ways making an 'X').



One of two arguments may be given when the slots command is entered
into the prompt, the first one being play.  Entering 'play' as an arg-
with the amount you wish to bet as a second argument rolls the slots.
You may bet 25, 50, 100, 200, 400, 800, or 1000(amounts in cents).

Syntax:
slot PLAY <amount>

The second of the arguments is 'ascii'.  This selection was added for
players with odd fonts/clients.  Toggling it 'off' will remove the
ascii imaging surrounding the slots, leaving only the results to be
seen.

Syntax:
slot ASCII <on/off>

The Casino Slots can be located at the Vixion Casino in Neon-Genesis.

(Casino Slots written by Nikola Sarcevic of Manga Nation)
 */

/*
 *  You will need to define this following bits in either merc.h
 *  or at the beginning of the file this code is being inserted in.
 *
 *  #define OBJ_VNUM_SLOT (the vnum of the item you selected for the slot machine)
 *
 *  This could also be done another way, you could make a 'slot'
 *  item type that is settable in olc, I didn't write it this way
 *  due to lack of obj_types remaining on Manga.  If you do write
 *  it this way, the code will need altered some, I'll leave that
 *  altering up to you.
 *
 *  You will also need to define one variable in merc.h, the
 *  slascii variable.  Under char_data, insert this line:
 *
 *  int16_t              slascii;
 *
 *  Then in save.c you will need to insert code in two different
 *  places.
 *
 *  In void fwrite_char:
 *  fprintf(fp, "SLAS %d\n",ch->slascii);
 *
 *  And in void fread_char under case 'S':
 *  KEY("SLAS", ch->slascii, fread_number(fp));
 *
 *  That should take care of that variable.
 */

/*
 *  This short function is for showing the result after it is
 *  selected.
 */

void show_slot(CHAR_DATA *ch, int num)
{
    switch(num)
    {
    case 1:
	send_to_char("   {Rcherry{X   ",ch);
	break;
    case 2:
	send_to_char("    {mplum{X    ",ch);
	break;
    case 3:
	send_to_char("   {Mgrape{X    ",ch);
	break;
    case 4:
	send_to_char(" {Gwatermelon{X ",ch);
	break;
    case 5:
	send_to_char("   {yorange{X   ",ch);
	break;
    case 6:
	send_to_char("   {Ylemon{X    ",ch);
	break;
    case 7:
	send_to_char("    {Dbar{X     ",ch);
	break;
    case 8:
	send_to_char("    {Y2{Dbar{X    ",ch);
	break;
    case 9:
	send_to_char("    {R3{Dbar{X    ",ch);
	break;
    case 10:
	send_to_char("   {Bseven{X    ",ch);
	break;
    }
    return;
}

/*
 *  Rolls the slots.
 */

int roll(void)
{
    return number_range(1,10);
}

/*
 *  The two functions just written will need defined in
 *  in either manga.h or at the top of the file the code
 *  is being inserted into.  Define them as shown:
 *
 *  int roll            args((void));
 *  int show_slotargs((CHAR_DATA *ch, int num));
 */

/*
 *  The main function... prepare for confusion.
 */

void do_slot(CHAR_DATA *ch, char *argument) // - Nikola(manga.thedigitalrealities.com port: 6969)
{
    OBJ_DATA *obj;
    char arg1[MAX_INPUT_LENGTH];
    char buf[MSL];
    bool CanPlay = TRUE;
    bool winner = FALSE;
    int winnings=0, basebet=0;
    int sa=0, sb=0, sc=0, sd=0, se=0, sf=0, sg=0, sh=0, si=0;

    if(IS_NPC(ch))
    {
	send_to_char("Мобы не могут играть.\n\r",ch);
	return;
    }

    //Check the room for the object.

    for (obj = ch->in_room->contents; obj != NULL; obj = obj->next_content)
	if (obj->pIndexData->vnum == OBJ_VNUM_SLOT)
	    break;

    if (obj == NULL)
    {
	send_to_char("Здесь нет автомата.\n\r",ch);
	return;
    }

    argument = one_argument(argument, arg1);

    if (arg1[0] == '\0')
    {
	send_to_char("слот <ставка>\n\rДля подробной информации набери {W'{XHELP SLOTS{W'{X.\n\r",ch);
	return;
    }

    basebet = atoi(arg1);

    //I made it so that only certain amounts can be wagered, it would be
    //entirely possible to remove this and go with however much they want
    //to wager, however I added this to give the slots a touch of reality.

    switch(basebet)
    {
    case 4:
    case 8:
    case 16:
    case 32:
    case 64:
    case 128:
    case 256:
	break;
    default:
	CanPlay = FALSE;
	break;
    }

    if(CanPlay == FALSE)
    {
	send_to_char("Ты можешь поставить 4, 8, 16, 32, 64, 128, 256 золота.\n\r",ch);
	return;
    }

    if(basebet > ch->gold)
    {
	send_to_char("У тебя нет столько золота.\n\r",ch);
	return;
    }

    ch->gold -= basebet;
    obj->value[0] += basebet;

    sprintf(buf,"Ты бросаешь %d золота в автомат.\n\r",basebet);
    send_to_char(buf,ch);

    act("$n кидает несколько монет в автомат.",ch,NULL,NULL,TO_ROOM);

    //Roll the slots!

    sa = roll();
    sb = roll();
    sc = roll();
    sd = roll();
    se = roll();
    sf = roll();
    sg = roll();
    sh = roll();
    si = roll();

    //The following section reveals the slots to the player.

    send_to_char("\n\r{D||||||||||||||||||||||||||||||||||||||||||||{X\n\r{D|||||||||||||||{R*{yC{YA{WS{wI{YN{yO {DSL{wO{WTS{R*{D|||||||||||||||{X\n\r{D||||||||||||||||||||||||||||||||||||||||||||{X\n\r{D||            ||            ||            ||{X\n\r",ch);
    send_to_char("{D||{X",ch);
    show_slot(ch,sa);
    send_to_char("{D||{X",ch);
    show_slot(ch,sb);
    send_to_char("{D||{X",ch);
    show_slot(ch,sc);
    send_to_char("{D||{X",ch);
    send_to_char("\n\r{D||            ||            ||            ||{X\n\r{D||||||||||||||||||||||||||||||||||||||||||||{X\n\r{D||            ||            ||            ||{X\n\r",ch);
    send_to_char("{D||{X",ch);
    show_slot(ch,sd);
    send_to_char("{D||{X",ch);
    show_slot(ch,se);
    send_to_char("{D||{X",ch);
    show_slot(ch,sf);
    send_to_char("{D||{X",ch);
    send_to_char("\n\r{D||            ||            ||            ||{X\n\r{D||||||||||||||||||||||||||||||||||||||||||||{X\n\r{D||            ||            ||            ||{X\n\r",ch);
    send_to_char("{D||{X",ch);
    show_slot(ch,sg);
    send_to_char("{D||{X",ch);
    show_slot(ch,sh);
    send_to_char("{D||{X",ch);
    show_slot(ch,si);
    send_to_char("{D||{X",ch);
    send_to_char("\n\r{D||            ||            ||            ||{X\n\r{D||||||||||||||||||||||||||||||||||||||||||||{X\n\r{D|||||||||||||||{R*{yC{YA{WS{wI{YN{yO {DSL{wO{WTS{R*{D|||||||||||||||{X\n\r{D||||||||||||||||||||||||||||||||||||||||||||{X\n\r",ch);

    //Decide whether or not they won.

    if(sa == 1 && sb != 1)
    {
	winner = TRUE;
	winnings += basebet/4;
    }
    else if(sa == 1 && sb == 1 && sc != 1)
    {
	winner = TRUE;
	winnings += basebet/2;
    }

    if(sd == 1 && se != 1)
    {
	winner = TRUE;
	winnings += basebet/4;
    }
    else if(sd == 1 && se == 1 && sf != 1)
    {
	winner = TRUE;
	winnings += basebet/2;
    }

    if(sg == 1 && sh != 1)
    {
	winner = TRUE;
	winnings += basebet/4;
    }
    else if(sg == 1 && sh == 1 && si != 1)
    {
	winner = TRUE;
	winnings += basebet/2;
    }

    if(sa == sb && sb == sc)
    {
	winner = TRUE;
	winnings += basebet * sb;
    }

    if(sd == se && se == sf)
    {
	winner = TRUE;
	winnings += basebet * se;
    }

    if(sg == sh && sh == si)
    {
	winner = TRUE;
	winnings += basebet * sh;
    }

    if(sa == se && se == si)
    {
	winner = TRUE;
	winnings += basebet * se;
    }

    if(sg == se && se == sc)
    {
	winner = TRUE;
	winnings += basebet * se;
    }

    if(sa == sd && sd == sg)
    {
	winner = TRUE;
	winnings += basebet * sd;
    }

    if(sb == se && se == sh)
    {
	winner = TRUE;
	winnings += basebet * se;
    }

    if(sc == sf && sf == si)
    {
	winner = TRUE;
	winnings += basebet * sf;
    }

    //This is the JACKPOT!  It gives a special message and is therefore
    //separated from the regular winning check.  I have never hit the
    //jackpot, so I don't really know if it works right. :-p

    if(sa == 10 && se == 10 && si == 10 && sg == 10 && sc == 10)
    {
	send_to_char("\n\r{WБоже мой!!! {RДЖЕКПОТ{W!!!{X\n\r",ch); //Potty word, if you're a pussy pg13 MUD you'll probably to change it.
	act("Красная лампочка над автоматом начинает мигать, раздается сигнал. {RДЖЕКПОТ{X!!!",ch,NULL,NULL,TO_ROOM);
	winnings = obj->value[0];
	sprintf(buf,"{WТы получаешь %d золота!{X\n\r",winnings);
	send_to_char(buf,ch);
	ch->gold += winnings;
	obj->value[0] = 0;
    }

    //Regular winner.

    else if(winner == TRUE)
    {
	send_to_char("\n\r{YВыигрыш!{X\n\r",ch);
	act("Автомат начинает мигать и издавать звуки.",ch,NULL,NULL,TO_ROOM);
	winnings = UMIN(winnings, obj->value[0]);
	sprintf(buf,"{WТы получаешь %d золота!{X\n\r",winnings);
	send_to_char(buf,ch);
	ch->gold += winnings;
	obj->value[0] -= winnings;

    }
    //Loser. :(

    else
    {
	send_to_char("\n\r{RАххх... Ничего, повезет в другой раз.{X\n\r",ch); //Another potty word.
    }
    return;
}


void do_dig(CHAR_DATA *ch, char *argument){
    OBJ_INDEX_DATA *pObj;
    int hard = 0, i, skill;
    bool found = FALSE;
    AFFECT_DATA af;
    OBJ_DATA *obj;

    if (!ch->in_room)
	return;

    if ((skill = get_skill(ch, gsn_dig)) < 1)
    {
	send_to_char("Ты не умеешь это делать.\n\r", ch);
	return;
    }

    if (MOUNTED(ch))
    {
	send_to_char("Ты не можешь заниматься распкопками с лошади.\n\r", ch);
	return;
    }

    if (is_room_affected(ch->in_room, gsn_dig))
    {
	send_to_char("Здесь до тебя уже постарались...\n\r", ch);
	return;
    }

    if (is_room_affected(ch->in_room, skill_lookup("cultivate forest")))
    {
	send_to_char("Здесь что-то не так...\n\r", ch);
	return;
    }

    if (ch->mana < 30 || ch->move < 10)
    {
	send_to_char("У тебя не хватает энергии на раскопки...\n\r", ch);
	return;
    }

    switch (ch->in_room->sector_type)
    {
    case SECT_FOREST:
    case SECT_HILLS:
    case SECT_FIELD:
	hard = 2;
	break;
    case SECT_MOUNTAIN:
	hard = 1;
	break;
    case SECT_DESERT:
	hard = 3;
	break;
    default:
	send_to_char("Здесь не стоит заниматься раскопками!\n\r", ch);
	return;
	break;
    }

    af.where = TO_ROOM_AFF;
    af.type = gsn_dig;
    af.level = ch->level;
    af.duration = ch->level;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = 0;
    affect_to_room(ch->in_room, &af);

    send_to_char("Ты начинаешь раскопки в поисках интересных вещей...\n\r", ch);
    act("$n начинает копаться здесь в поисках чего-нибудь ценного...", ch, NULL, NULL, TO_ROOM);

    for (i = 0; i < skill;)
    {
	if ((pObj = get_obj_index(number_range(100, top_vnum_obj - 1))) != NULL 
	    && CAN_WEAR(pObj, ITEM_TAKE) && pObj->level <= ch->level 
	    && !IS_OBJ_STAT(pObj, ITEM_NOT_DIGGING)
	    && !IS_SET(pObj->area->area_flags, AREA_NA)
	    && !IS_SET(pObj->area->area_flags, AREA_TESTING))
	{
	    int chance;

	    i++;

	    switch(pObj->item_type)
	    {
	    case ITEM_SCROLL:
	    case ITEM_WAND:
	    case ITEM_STAFF:	
	    case ITEM_PILL:
	    case ITEM_ROD:
	    case ITEM_MAP:
	    case ITEM_KEY:	
	    case ITEM_BOAT:
		chance = 6;
		break;

	    case ITEM_WEAPON:	
	    case ITEM_ARMOR:
		chance = 3;
		break;

	    case ITEM_GEM:
	    case ITEM_TREASURE:
	    case ITEM_JEWELRY:
		chance = 5;
		break;

	    case ITEM_MONEY:
		chance = 40;
		break;

	    case ITEM_ARTIFACT:	
		chance = 4;
		break;

	    default: 
		chance = 0; 
		break;
	    }

	    if (chance/hard >= number_percent())
	    {
		obj = create_object(pObj, 0);

		if (check_reset_for_limit(obj, TRUE))
		{
		    extract_obj(obj, FALSE, FALSE);
		    continue;
		}

		obj_to_room(obj, ch->in_room);

		obj->condition = number_range(1, obj->condition);

		act("При проведении раскопок ты обнаруживаешь $p6!", ch, obj, NULL, TO_CHAR);
		found = TRUE;
	    }
	}
    }

    if (!found)
    {
	if (number_percent() < skill/4)
	{
	    obj = create_money(number_range(0, skill/4), number_range(1, skill * 2), number_range(0, skill/8));
	    obj_to_room(obj, ch->in_room);
	    act("При проведении раскопок ты обнаруживаешь $p6!", ch, obj, NULL, TO_CHAR);
	}
	else
	{
	    send_to_char("Ничего не удалось раскопать...\n\r", ch);
	    check_improve(ch, NULL, gsn_dig, FALSE, 2);
	}

	ch->mana -= 15;
	ch->move -= 5;
    }
    else
    {
	check_improve(ch, NULL, gsn_dig, TRUE, 2);
	ch->mana -= 30;
	ch->move -= 10;
    }

    WAIT_STATE(ch, skill_table[gsn_dig].beats); 
}

bool check_auction_obj(CHAR_DATA *ch, OBJ_DATA *obj) {
	if (IS_SET(obj->extra_flags, ITEM_AUCTION)) {
		send_to_char("Эта вещь выставлена на аукцион! Ты ничего не можешь с ней пока сделать...\n\r", ch);
		return TRUE;
	}

	return FALSE;

}



/* charset=cp1251 */










