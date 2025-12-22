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
#include "merc.h"
#include "recycle.h"


void acid_effect(void *vo, int level, int dam, int target, CHAR_DATA *caster)
{
    if (target == TARGET_ROOM) /* nail objects on the floor */
    {
	ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *) vo;
	OBJ_DATA *obj, *obj_next;

	for (obj = room->contents; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;
	    acid_effect(obj, level, dam, TARGET_OBJ, caster);
	}
	return;
    }

    if (target == TARGET_CHAR)  /* do the effect on a victim */
    {
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	OBJ_DATA *obj, *obj_next;

	/* let's toast some gear */
	for (obj = victim->carrying; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;

	    //Шмот на оборотне в форме не порится
	    if (is_lycanthrope(victim) && obj->wear_loc >= WEAR_NONE)
		continue;

	    acid_effect(obj, level, dam, TARGET_OBJ, caster);
	}
	return;
    }

    if (target == TARGET_OBJ) /* toast an object */
    {
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	OBJ_DATA *t_obj, *n_obj;
	int chance;
	char msg[MAX_STRING_LENGTH];

	if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF)
	    ||  IS_OBJ_STAT(obj, ITEM_NOPURGE)
	    ||  number_range(0, 4) == 0)
	    return;

	chance = level / 4 + dam / 10;

	if (chance > 25)
	    chance = (chance - 25) / 2 + 25;
	if (chance > 50)
	    chance = (chance - 50) / 2 + 50;

	if (IS_OBJ_STAT(obj, ITEM_BLESS))
	    chance -= 5;

	chance -= obj->level * 2;

	switch (obj->item_type)
	{
	default:
	    return;
	case ITEM_CONTAINER:
	case ITEM_CORPSE_PC:
	case ITEM_CORPSE_NPC:
	    strcpy(msg, "$p та$r.");
	    break;
	case ITEM_ARMOR:
	    strcpy(msg, "$p разлага$rся под действием кислоты.");
	    break;
	case ITEM_CLOTHING:
	    strcpy(msg, "$p разъеда$rся и распада$rся на кусочки.");
	    break;
	case ITEM_STAFF:
	case ITEM_WAND:
	    chance -= 10;
	    strcpy(msg, "$p разъеда$rся кислотой и лома$rся.");
	    break;
	case ITEM_SCROLL:
	    chance += 10;
	    strcpy(msg, "$p вспыхива$r и сгора$r.");
	    break; 
	}

	chance = URANGE(5, chance, 95);

	if (number_percent() > chance)
	    return;

	if (obj->carried_by != NULL)
	    act(msg, obj->carried_by, obj, NULL, TO_ALL);
	else if (obj->in_room != NULL && !LIST_EMPTY(&obj->in_room->people))
	    act(msg, LIST_FIRST(&obj->in_room->people), obj, NULL, TO_ALL);

	if (obj->item_type == ITEM_ARMOR)  /* etch it */
	{
	    int i;


	    for (i = 0; i < 4; i++)
	    {
		obj->value[i] -= 1;

		if (obj->carried_by != NULL && obj->wear_loc != WEAR_NONE)
		    obj->carried_by->armor[i] += 1;
	    }
	    return;
	}

	/* get rid of the object */
	for (t_obj = obj->contains; t_obj != NULL; t_obj = n_obj)
	{
	    n_obj = t_obj->next_content;
	    obj_from_obj(t_obj);
	    if (obj->in_room != NULL)
		obj_to_room(t_obj, obj->in_room);
	    else if (obj->carried_by != NULL)
		obj_to_room(t_obj, obj->carried_by->in_room);
	    else
	    {
		extract_obj(t_obj, TRUE, FALSE);
		continue;
	    }

	    acid_effect(t_obj, level/2, dam/2, TARGET_OBJ, caster);
	}

	extract_obj(obj, TRUE, TRUE);
	return;
    }
}


void cold_effect(void *vo, int level, int dam, int target, CHAR_DATA *caster)
{
    if (target == TARGET_ROOM) /* nail objects on the floor */
    {
	ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *) vo;
	OBJ_DATA *obj, *obj_next;

	for (obj = room->contents; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;
	    cold_effect(obj, level, dam, TARGET_OBJ, caster);
	}
	return;
    }

    if (target == TARGET_CHAR) /* whack a character */
    {
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	OBJ_DATA *obj, *obj_next;
	AFFECT_DATA *paf;
	int sn = skill_lookup("chill touch"), hard;
	/* chill touch effect */

	if ((paf = affect_find(victim->affected, sn)) != NULL)
	    hard = UMAX(1, -paf->modifier);
	else
	    hard = 1;

	if (!saves_spell(level/(4 * hard) + dam / (20 * hard), victim, DAM_COLD))
	{
	    AFFECT_DATA af;

	    act("$n замерзает и дрожит от холода!", victim, NULL, NULL, TO_ROOM);
	    send_to_char("Холод пронизывает тебя насквозь.\n\r", victim);
	    af.where     = TO_AFFECTS;
	    af.type      = sn;
	    af.level     = level;
	    af.duration  = level/10;
	    af.location  = APPLY_STR;
	    af.modifier  = -1;
	    af.bitvector = 0;
	    af.caster_id = get_id(caster);
	    affect_join( victim, &af );
	}

	/* hunger! (warmth sucked out */
	if (!IS_NPC(victim))
	    gain_condition(victim, COND_HUNGER, dam/20);

	/* let's toast some gear */
	for (obj = victim->carrying; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;
	    cold_effect(obj, level, dam, TARGET_OBJ, caster);
	}
	return;
    }

    if (target == TARGET_OBJ) /* toast an object */
    {
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	int chance;
	char msg[MAX_STRING_LENGTH];

	if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF)
	    ||  IS_OBJ_STAT(obj, ITEM_NOPURGE)
	    ||  number_range(0, 4) == 0)
	    return;

	chance = level / 4 + dam / 10;

	if (chance > 25)
	    chance = (chance - 25) / 2 + 25;
	if (chance > 50)
	    chance = (chance - 50) / 2 + 50;

	if (IS_OBJ_STAT(obj, ITEM_BLESS))
	    chance -= 5;

	chance -= obj->level * 2;

	switch(obj->item_type)
	{
	default:
	    return;
	case ITEM_POTION:
	    chance += 25;
	    break;
	case ITEM_DRINK_CON:
	    chance += 5;
	    break;
	}

	strcpy(msg, "$p замерза$r и разбива$rся вдребезги!");

	chance = URANGE(5, chance, 95);

	if (number_percent() > chance)
	    return;

	if (obj->carried_by != NULL)
	    act(msg, obj->carried_by, obj, NULL, TO_ALL);
	else if (obj->in_room != NULL && !LIST_EMPTY(&obj->in_room->people))
	    act(msg, LIST_FIRST(&obj->in_room->people), obj, NULL, TO_ALL);

	extract_obj(obj, TRUE, TRUE);
	return;
    }
}




void fire_effect(void *vo, int level, int dam, int target, CHAR_DATA *caster)
{
    char msg[MAX_STRING_LENGTH];

    if (target == TARGET_ROOM)  /* nail objects on the floor */
    {
	ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *) vo;
	OBJ_DATA *obj, *obj_next;

	for (obj = room->contents; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;
	    fire_effect(obj, level, dam, TARGET_OBJ, caster);
	}
	return;
    }

    if (target == TARGET_CHAR)   /* do the effect on a victim */
    {
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	OBJ_DATA *obj, *obj_next;

	/* getting thirsty */
	if (!IS_NPC(victim))
	    gain_condition(victim, COND_THIRST, dam/20);

	/* let's toast some gear! */
	for (obj = victim->carrying; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;

	    fire_effect(obj, level, dam, TARGET_OBJ, caster);
	}

	/* chance of blindness */
	if (!IS_AFFECTED(victim, AFF_BLIND) && victim->classid != CLASS_NAZGUL
	    &&  !saves_spell(level / 4 + dam / 20, victim, DAM_FIRE) && IS_SET(victim->parts, PART_EYE))
	{
	    AFFECT_DATA af;

	    sprintf(msg, "$n ослеплен%s дымом!", SEX_ENDING(victim));
	    act(msg, victim, NULL, NULL, TO_ROOM);
	    send_to_char("Твои глаза застилает дым...Ты ничего не видишь!\n\r", victim);

	    af.where        = TO_AFFECTS;
	    af.type         = skill_lookup("fire breath");
	    af.level        = level;
	    af.duration     = 0;
	    af.location     = APPLY_HITROLL;
	    af.modifier     = -4;
	    af.bitvector    = AFF_BLIND;
	    af.caster_id    = get_id(caster);
	    affect_to_char(victim, &af);
	}

	return;
    }

    if (target == TARGET_OBJ)  /* toast an object */
    {
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	OBJ_DATA *t_obj, *n_obj;
	int chance;

	if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF)
	    ||  IS_OBJ_STAT(obj, ITEM_NOPURGE)
	    ||  number_range(0, 4) == 0)
	    return;

	chance = level / 4 + dam / 10;

	if (chance > 25)
	    chance = (chance - 25) / 2 + 25;
	if (chance > 50)
	    chance = (chance - 50) / 2 + 50;

	if (IS_OBJ_STAT(obj, ITEM_BLESS))
	    chance -= 5;
	chance -= obj->level * 2;

	switch ( obj->item_type )
	{
	default:             
	    return;
	case ITEM_CONTAINER:
	    strcpy(msg, "$p воспламеня$rся и сгора$r!");
	    break;
	case ITEM_POTION:
	    chance += 25;
	    strcpy(msg, "$p очень сильно нагрева$rся!");
	    break;
	case ITEM_SCROLL:
	    chance += 50;
	    strcpy(msg, "$p сгора$r!");
	    break;
	case ITEM_STAFF:
	    chance += 10;
	    strcpy(msg, "$p обуглива$rся!");
	    break;
	case ITEM_WAND:
	    strcpy(msg, "$p взрыва$rся, обдавая тебя снопом искр!");
	    break;
	case ITEM_FOOD:
	    strcpy(msg, "$p черне$r и обуглива$rся!");
	    break;
	case ITEM_PILL:
	    strcpy(msg, "$p капа$r!");
	    break;
	}

	chance = URANGE(5, chance, 95);

	if (number_percent() > chance)
	    return;

	if (obj->carried_by != NULL)
	    act( msg, obj->carried_by, obj, NULL, TO_ALL );
	else if (obj->in_room != NULL && !LIST_EMPTY(&obj->in_room->people))
	    act(msg, LIST_FIRST(&obj->in_room->people), obj, NULL, TO_ALL);

	if (obj->contains)
	{
	    /* dump the contents */

	    for (t_obj = obj->contains; t_obj != NULL; t_obj = n_obj)
	    {
		n_obj = t_obj->next_content;
		obj_from_obj(t_obj);
		if (obj->in_room != NULL)
		    obj_to_room(t_obj, obj->in_room);
		else if (obj->carried_by != NULL)
		    obj_to_room(t_obj, obj->carried_by->in_room);
		else
		{
		    extract_obj(t_obj, TRUE, FALSE);
		    continue;
		}
		fire_effect(t_obj, level/2, dam/2, TARGET_OBJ, caster);
	    }
	}

	extract_obj( obj, TRUE, TRUE );
	return;
    }
}

void poison_effect(void *vo, int level, int dam, int target, CHAR_DATA *caster)
{
    if (target == TARGET_ROOM)  /* nail objects on the floor */
    {
	ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *) vo;
	OBJ_DATA *obj, *obj_next;

	for (obj = room->contents; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;
	    poison_effect(obj, level, dam, TARGET_OBJ, caster);
	}
	return;
    }

    if (target == TARGET_CHAR)   /* do the effect on a victim */
    {
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	OBJ_DATA *obj, *obj_next;
	AFFECT_DATA *paf;
	int hard;

	if ((paf = affect_find(victim->affected, gsn_poison)) != NULL)
	    hard = UMAX(1, -2 * paf->modifier);
	else
	    hard = 1;

	/* chance of poisoning */
	if (!saves_spell(level / (4 * hard) + dam / (20 * hard), victim, DAM_POISON))
	{
	    AFFECT_DATA af;

	    send_to_char("Ты чувствуешь, как яд проникает в твои вены.\n\r", victim);
	    act("$n выглядит очень больным.", victim, NULL, NULL, TO_ROOM);

	    af.where     = TO_AFFECTS;
	    af.type      = gsn_poison;
	    af.level     = level;
	    af.duration  = level / 10;
	    af.location  = APPLY_STR;
	    af.modifier  = -1;
	    af.bitvector = AFF_POISON;
	    af.caster_id = get_id(caster);
	    affect_join( victim, &af );
	}

	/* equipment */
	for (obj = victim->carrying; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;
	    poison_effect(obj, level, dam, TARGET_OBJ, caster);
	}
	return;
    }

    if (target == TARGET_OBJ)  /* do some poisoning */
    {
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	int chance;


	if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF)
	    ||  IS_OBJ_STAT(obj, ITEM_BLESS)
	    ||  number_range(0, 4) == 0)
	    return;

	chance = level / 4 + dam / 10;
	if (chance > 25)
	    chance = (chance - 25) / 2 + 25;
	if (chance > 50)
	    chance = (chance - 50) / 2 + 50;

	chance -= obj->level * 2;

	switch (obj->item_type)
	{
	default:
	    return;
	case ITEM_FOOD:
	    break;
	case ITEM_DRINK_CON:
	    if (obj->value[0] == obj->value[1])
		return;
	    break;
	}

	chance = URANGE(5, chance, 95);

	if (number_percent() > chance)
	    return;

	obj->value[3] = level;
	return;
    }
}


void shock_effect(void *vo, int level, int dam, int target, CHAR_DATA *caster)
{
    if (target == TARGET_ROOM)
    {
	ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *) vo;
	OBJ_DATA *obj, *obj_next;

	for (obj = room->contents; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;
	    shock_effect(obj, level, dam, TARGET_OBJ, caster);
	}
	return;
    }

    if (target == TARGET_CHAR)
    {
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	OBJ_DATA *obj, *obj_next;

	/* daze and confused? */
	if (!saves_spell(level/4 + dam/20, victim, DAM_LIGHTNING))
	{
	    send_to_char("Твои мускулы перестают слушаться.\n\r", victim);
	    DAZE_STATE(victim, UMAX(12, level/4 + dam/20));
	}

	/* toast some gear */
	for (obj = victim->carrying; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;
	    shock_effect(obj, level, dam, TARGET_OBJ, caster);
	}
	return;
    }

    if (target == TARGET_OBJ)
    {
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	int chance;
	char msg[MAX_STRING_LENGTH];

	if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF)
	    ||  IS_OBJ_STAT(obj, ITEM_NOPURGE)
	    ||  number_range(0, 4) == 0)
	    return;

	chance = level / 4 + dam / 10;

	if (chance > 25)
	    chance = (chance - 25) / 2 + 25;
	if (chance > 50)
	    chance = (chance - 50) /2 + 50;

	if (IS_OBJ_STAT(obj, ITEM_BLESS))
	    chance -= 5;

	chance -= obj->level * 2;

	switch(obj->item_type)
	{
	default:
	    return;
	case ITEM_WAND:
	case ITEM_STAFF:
	    chance += 10;
	    strcpy(msg , "$p перегрева$rся и взрыва$rся!");
	    break;
	case ITEM_JEWELRY:
	    chance -= 10;
	    strcpy(msg, "$p превраща$rся в бесполезный комок.");
	}

	chance = URANGE(5, chance, 95);

	if (number_percent() > chance)
	    return;

	if (obj->carried_by != NULL)
	    act(msg, obj->carried_by, obj, NULL, TO_ALL);
	else if (obj->in_room != NULL && !LIST_EMPTY(&obj->in_room->people))
	    act(msg, LIST_FIRST(&obj->in_room->people), obj, NULL, TO_ALL);

	extract_obj(obj, TRUE, TRUE);
	return;
    }
}

/* charset=cp1251 */
