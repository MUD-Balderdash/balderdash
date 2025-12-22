/**************************************************************************
* Copyright (c) 2002                                                      *
*      The staff of Balderdash MUD.  All rights reserved.                 *
*                                                                         *
* Redistribution and use in source and binary forms, with or without      *
* modification, are permitted provided that the following conditions      *
* are met:                                                                *
* 1. Redistributions of source code must retain the above copyright       *
*    notice, this list of conditions and the following disclaimer.        *
* 2. Redistributions in binary form must reproduce the above copyright    *
*    notice, this list of conditions and the following disclaimer in the  *
*    documentation and/or other materials provided with the distribution. *
* 3. Neither the name of Balderdash MUD the names of its contributors may *
*    be used to endorse or promote products derived from this software    *
*    without specific prior written permission.                           *
*                                                                         *
* THIS SOFTWARE IS PROVIDED BY BALDERDASH MUD AND CONTRIBUTORS ``AS       *
* IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT          *
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS       *
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE         *
* STAFF OF FATAL DIMENSIONS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,     *
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES      *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR      *
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)      *
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,     *
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)           *
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED     *
* OF THE POSSIBILITY OF SUCH DAMAGE.                                      *
***************************************************************************/
/* Alexander Speransky aka Sure                                   */
/* Alexey V. Antipovsky aka Kemm aka Ssart                        */
/* Victor N. Kachulin aka Vicus                                   */


#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include "merc.h"


void do_function (CHAR_DATA *ch, DO_FUN *do_fun, char *argument);
void do_remove(CHAR_DATA *ch, char *argument);
void do_wear(CHAR_DATA *ch, char *argument);
void do_get(CHAR_DATA *ch, char *argument);
int get_aff_value(OBJ_DATA *obj);

int get_obj_value(OBJ_DATA *obj){
    int value = 0, i;
    
    if (obj == NULL)
    {
        bugf("Get_obj_value: NULL obj1 or obj2.");
        return 0;
    }

    switch (obj->item_type)
    {
    case ITEM_WEAPON:
	if (obj->pIndexData->new_format)
	    value = (1 + obj->value[2]) * obj->value[1] / 2;
	else
	    value = (obj->value[1] + obj->value[2]) / 2;

	break;

    case ITEM_ARMOR:
	for (i = 0; i < 4; i++)
	    value += obj->value[i];

	break;

    default:
	break;
    }
    
    value += get_aff_value(obj);

    return value;
}

OBJ_DATA *get_best_object(OBJ_DATA *obj1, OBJ_DATA *obj2){
    int value1, value2;
    
    if (obj1 == NULL || obj2 == NULL)
    {
        bugf("Get_best_object: NULL obj1 or obj2.");
        return NULL;
    }

    if (obj1 == obj2)
        return obj1;
        
    value1 = get_obj_value(obj1);
    value2 = get_obj_value(obj2);
    
    if (value1 >= value2)
        return obj1;
    else
        return obj2;
}

int get_aff_value(OBJ_DATA *obj){
    AFFECT_DATA *paf;
    int num_affects = 0;
    double value = 0;
    
    if (obj == NULL)
        return 0;
    
    if (!obj->enchanted && !obj->pIndexData->edited)
	for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
	{
	    if (paf->where == TO_OBJECT)
	    {
                if (paf->location == APPLY_AC)
		{
	            value -= paf->modifier / 5;
		}
		else if (paf->location >= APPLY_SAVES
			&& paf->location <= APPLY_SAVING_SPELL)
		{
		    value -= paf->modifier;
		}
	        else if (paf->location >= APPLY_MANA
			&& paf->location <= APPLY_MOVE)
		{
	            value += paf->modifier / 10;
		}
	        else if (paf->location == APPLY_SEX)
	            value += 0;
	        else
	            value += paf->modifier;

	        num_affects++;
	    }
	    if (paf->where == TO_AFFECTS)
	    {
	        if (paf->bitvector == AFF_BLIND
		    || paf->bitvector == AFF_FAERIE_FIRE
		    || paf->bitvector == AFF_CURSE
		    || paf->bitvector == AFF_POISON
		    || paf->bitvector == AFF_SLEEP
		    || paf->bitvector == AFF_PLAGUE
		    || paf->bitvector == AFF_WEAKEN
		    || paf->bitvector == AFF_SLOW
		    || paf->bitvector == AFF_CALM)
		{
	            value -= 15;
		}
	        else if (paf->bitvector == AFF_SANCTUARY
			|| paf->bitvector == AFF_HASTE
	                || paf->bitvector == AFF_REGENERATION)
	        {
	            value += 10;
	            num_affects++;
	        }
	        else
	            num_affects++;
	    }
	    else if (paf->where == TO_RESIST)
	    {
		if (paf->bitvector == DAM_MAGIC || paf->bitvector == DAM_WEAPON)
		    value += paf->modifier / 3;
		else if (paf->bitvector == DAM_SUMMON)
		    value += paf->modifier / 20;
		else if (paf->bitvector == DAM_CHARM)
		    value += paf->modifier / 5;
		else
		    value += paf->modifier / 8;
	    }
	    else
		num_affects++;
	}
    
    for (paf = obj->affected; paf != NULL; paf = paf->next)
    {
        if (paf->where == TO_OBJECT)
        {
	    if (paf->location == APPLY_AC)
    	    {
		value -= paf->modifier / 5;
	    }
	    else if (paf->location >= APPLY_SAVES
		    && paf->location <= APPLY_SAVING_SPELL)
	    {
		value -= paf->modifier;
	    }
            else if (paf->location >= APPLY_MANA
		    && paf->location <= APPLY_MOVE)
	    {
                value += paf->modifier / 10;
	    }
            else if (paf->location == APPLY_SEX)
                value += 0;
            else
                value += paf->modifier;

            num_affects++;
        }

	if (paf->where == TO_AFFECTS)
	{
	    if (paf->bitvector == AFF_BLIND
		|| paf->bitvector == AFF_FAERIE_FIRE
	        || paf->bitvector == AFF_CURSE
		|| paf->bitvector == AFF_POISON
	        || paf->bitvector == AFF_SLEEP
		|| paf->bitvector == AFF_PLAGUE
	        || paf->bitvector == AFF_WEAKEN
		|| paf->bitvector == AFF_SLOW
	        || paf->bitvector == AFF_CALM)
	    {
	        value -= 15;
	    }
	    else if (paf->bitvector == AFF_SANCTUARY
		    || paf->bitvector == AFF_HASTE
	            || paf->bitvector == AFF_REGENERATION)
	    {
	        value += 10;
	        num_affects++;
	    }
	    else
	        num_affects++;
	}
        else if (paf->where == TO_RESIST)
	{
	    if (paf->bitvector == DAM_MAGIC || paf->bitvector == DAM_WEAPON)
		value += paf->modifier / 3;
	    else if (paf->bitvector == DAM_SUMMON)
		value += paf->modifier / 20;
	    else if (paf->bitvector == DAM_CHARM)
		value += paf->modifier / 5;
	    else
		value += paf->modifier / 8;
	}
        else
            num_affects++;
    }

    return value + num_affects;
}

void get_best_wear(CHAR_DATA *ch, OBJ_DATA *obj){
    OBJ_DATA *old_obj = NULL, *new_obj = obj;
    bool sec = FALSE;
    bool rem = FALSE;
    char sobj[MAX_INPUT_LENGTH];

    /* I don't like cursed stuff */
    if (IS_SET(obj->extra_flags, ITEM_NOREMOVE))
	return;

    if (obj->item_type == ITEM_LIGHT)
    {
	if ((old_obj = get_eq_char(ch, WEAR_LIGHT)) == NULL)
	    new_obj = obj;
	else
	    new_obj = get_best_object(obj, old_obj);
    }
    else if (obj->item_type == ITEM_WEAPON)
    {
	if ((old_obj = get_eq_char(ch, WEAR_WIELD)) == NULL)
	{
	    new_obj = obj;
	    
    	    if ((old_obj = get_eq_char(ch, WEAR_SECONDARY)) != NULL
		&& old_obj->weight > obj->weight)
	    {
	    	sec = TRUE;
	    }
	}
	else
	    new_obj = get_best_object(obj, old_obj);
    }
    else
    {
	if (CAN_WEAR(obj, ITEM_WEAR_FINGER))
	{
	    if ((old_obj = get_eq_char(ch, WEAR_FINGER_L)) == NULL)
		new_obj = obj;
	    else
	    {
		if ((new_obj = get_eq_char(ch, WEAR_FINGER_R)) == NULL)
		    new_obj = obj;
		else
		{
		    old_obj = get_best_object(old_obj, new_obj) == old_obj ? new_obj : old_obj;
		    new_obj = get_best_object(old_obj, obj);
		    rem = TRUE;
		}
	    }
	}
	else if (CAN_WEAR(obj, ITEM_WEAR_NECK))
	{
	    if ((old_obj = get_eq_char(ch, WEAR_NECK_1)) == NULL)
		new_obj = obj;
	    else
	    {
		if ((new_obj = get_eq_char(ch, WEAR_NECK_2)) == NULL)
		    new_obj = obj;
		else
		{
		    old_obj = get_best_object(old_obj, new_obj) == old_obj ? new_obj : old_obj;
		    new_obj = get_best_object(old_obj, obj);
		    rem = TRUE;
		}
	    }
	}
	else if (CAN_WEAR(obj, ITEM_WEAR_WRIST))
	{
	    if ((old_obj = get_eq_char(ch, WEAR_WRIST_L)) == NULL)
		new_obj = obj;
	    else
	    {
		if ((new_obj = get_eq_char(ch, WEAR_WRIST_R)) == NULL)
		    new_obj = obj;
		else
		{
		    old_obj = get_best_object(old_obj, new_obj) == old_obj ? new_obj : old_obj;
		    new_obj = get_best_object(old_obj, obj);
		    rem = TRUE;
		}
	    }
	}
	else if (CAN_WEAR(obj, ITEM_WEAR_BODY))
	{
	    if ((old_obj = get_eq_char(ch, WEAR_BODY)) == NULL)
		new_obj = obj;
	    else
		new_obj = get_best_object(old_obj, obj);
	}
	else if (CAN_WEAR(obj, ITEM_WEAR_HEAD))
	{
	    if ((old_obj = get_eq_char(ch, WEAR_HEAD)) == NULL)
		new_obj = obj;
	    else
		new_obj = get_best_object(old_obj, obj);
	}
	else if (CAN_WEAR(obj, ITEM_WEAR_LEGS))
	{
	    if ((old_obj = get_eq_char(ch, WEAR_LEGS)) == NULL)
		new_obj = obj;
	    else
		new_obj = get_best_object(old_obj, obj);
	}
	else if (CAN_WEAR(obj, ITEM_WEAR_FEET))
	{
	    if ((old_obj = get_eq_char(ch, WEAR_FEET)) == NULL)
		new_obj = obj;
	    else
		new_obj = get_best_object(old_obj, obj);
	}
	else if (CAN_WEAR(obj, ITEM_WEAR_HANDS))
	{
	    if ((old_obj = get_eq_char(ch, WEAR_HANDS)) == NULL)
		new_obj = obj;
	    else
		new_obj = get_best_object(old_obj, obj);
	}
	else if (CAN_WEAR(obj, ITEM_WEAR_ARMS))
	{
	    if ((old_obj = get_eq_char(ch, WEAR_ARMS)) == NULL)
		new_obj = obj;
	    else
		new_obj = get_best_object(old_obj, obj);
	}
	else if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))
	{
	    if ((old_obj = get_eq_char(ch, WEAR_SHIELD)) == NULL)
		new_obj = obj;
	    else
		new_obj = get_best_object(old_obj, obj);
	}
	else if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))
	{
	    if ((old_obj = get_eq_char(ch, WEAR_ABOUT)) == NULL)
		new_obj = obj;
	    else
		new_obj = get_best_object(old_obj, obj);
	}
	else if (CAN_WEAR(obj, ITEM_WEAR_FLOAT))
	{
	    if ((old_obj = get_eq_char(ch, WEAR_FLOAT)) == NULL)
		new_obj = obj;
	    else
		new_obj = get_best_object(old_obj, obj);
	}
	else if (CAN_WEAR(obj, ITEM_HOLD))
	{
	    if ((old_obj = get_eq_char(ch, WEAR_HOLD)) == NULL)
		new_obj = obj;
	    else
		new_obj = get_best_object(old_obj, obj);
	}
    }

    /* Ffuhhh... */
    if (new_obj != obj)
	return;

    sprintf(sobj, "'%s'", new_obj->name);

    if (sec)
    {
	char ssec[MAX_STRING_LENGTH];
    
	sprintf(ssec, "'%s'", old_obj->name);
	
	do_function(ch, &do_remove, ssec);
	do_function(ch, &do_wear, ssec);
    }

    if (rem)
    {
	char srem[MAX_STRING_LENGTH];

	sprintf(srem, "'%s'", old_obj->name);
	do_function(ch, &do_remove, srem);
    }

    do_function(ch, &do_get, sobj);
    do_function(ch, &do_wear, sobj);
}

bool is_good_spp(OBJ_DATA *obj){
    return TRUE;
}

void get_good_objs(CHAR_DATA *ch)
{
#if 0 /* Infinite recurse, if obj has progs... 8(( Fix later */
    OBJ_DATA *obj, *obj_next;
    char buf[MAX_STRING_LENGTH];

    if (!IS_NPC(ch)
	|| !ch->in_room
	|| IS_AFFECTED(ch, AFF_CHARM)
	|| IS_SET(ch->act, ACT_PET))
    {
	return;
    }
    
    for (obj = ch->in_room->contents; obj; obj = obj_next)
    {
	obj_next = obj->next;

	/* Ough, shit! This stuff is TOO cool for me... 8)) */
	if (!CAN_WEAR(obj, ITEM_TAKE) || obj->level > ch->level)
	    continue;

	switch (obj->item_type)
	{
	default:
	    return;
	case ITEM_LIGHT:
	case ITEM_WEAPON:
	case ITEM_ARMOR:
	case ITEM_CLOTHING:
	case ITEM_FURNITURE:
	case ITEM_TRASH:
	case ITEM_CONTAINER:
	case ITEM_CHEST:
	case ITEM_MORTAR:
	case ITEM_INGREDIENT:
	case ITEM_BOAT:
	case ITEM_FOUNTAIN:
	case ITEM_PROTECT:
	case ITEM_PORTAL:
	case ITEM_WARP_STONE:
	case ITEM_GEM:
	case ITEM_JEWELRY:
	case ITEM_JUKEBOX:
	case ITEM_ARTIFACT:
	    get_best_wear(ch, obj);
	    break;
	case ITEM_WAND:
	case ITEM_STAFF:
	case ITEM_ROD:
	    break;
#if 0
	    if ((IS_SET(ch->act, ACT_MAGE)
		    || IS_SET(ch->act, ACT_CLERIC))
		&& is_good_device(ch, obj))
	    {
		sprintf(buf, "'%s'", obj->name);
		do_function(ch, &do_get, buf);
	    }
	    break;
#endif
	case ITEM_SCROLL:
	case ITEM_PILL:
	case ITEM_POTION:
	    if (is_good_spp(obj))
	    {
		sprintf(buf, "'%s'", obj->name);
		do_function(ch, &do_get, buf);
	    }
	    break;
	}
    }
#endif //0
}
	    
/* charset=cp1251 */
