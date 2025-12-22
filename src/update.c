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
#include "interp.h"
#include "music.h"
#include "recycle.h"

AUCTION_DATA auctions[MAX_AUCTION];
struct moon_type* moons_data;

void bust_a_prompt(CHAR_DATA *ch);

bool check_obj_sex(CHAR_DATA *ch, OBJ_DATA *obj);
bool is_uncomf(CHAR_DATA *ch, OBJ_DATA *obj);
float check_material(OBJ_DATA *obj, int16_t dt);
bool can_cleave(CHAR_DATA *ch, CHAR_DATA *victim);
void restore(CHAR_DATA *ch, CHAR_DATA *victim);

/*
 * Local functions.
 */
int	hit_gain	args((CHAR_DATA *ch));
int	mana_gain	args((CHAR_DATA *ch));
int	move_gain	args((CHAR_DATA *ch));
void	mobile_update	args((void));
void	char_update	args((void));
void	obj_update	args((void));
void	aggr_update	args((void));
void 	close_quest 	args((void));

void 	quest_update	(CHAR_DATA *ch);
void 	auction_update	args((void));

/* used for saving */

static  int     pulse_area;
static  int     pulse_violence;
static  int     pulse_point;
static  int     pulse_music;

extern int reboot_time;
extern CHAR_DATA *who_claim_reboot;

struct message_ticks
{
    int tick;
};

const struct message_ticks reboot_table[] =
{
    {45},
    {30},
    {20},
    {10},
    { 5},
    { 4},
    { 3},
    { 2},
    { 1},
    { 0},
};

void room_update(void)
{
    ROOM_INDEX_DATA *room;
    TRACK_DATA *tr, *tr_next;
    int iHash;
    AFFECT_DATA *paf, *paf_next;

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
	for (room = room_index_hash[iHash]; room != NULL; room = room->next)
	{
	    LIST_FOREACH_SAFE(tr, &room->tracks, link, tr_next)
	    {
		if (++tr->ago > 23)
		{
		    LIST_REMOVE(tr, link);
		    free_mem(tr, sizeof(TRACK_DATA));
		}
	    }

            for (paf = room->affected; paf != NULL; paf = paf_next)
	    {
	   	paf_next	= paf->next;

	     	if (paf->duration > 0)
	    	    paf->duration--;
	      	else if (paf->duration == 0)
		{
		    if (paf_next == NULL
			|| paf_next->type != paf->type
			|| paf_next->duration > 0)
		    {
			if (paf->type > 0
			    && !IS_NULLSTR(skill_table[paf->type].msg_room))
			{
			    CHAR_DATA *ch, *safe_ch;

			    LIST_FOREACH_SAFE(ch, &room->people, room_link, safe_ch)
			    {
				send_to_char(skill_table[paf->type].msg_room,ch);
				send_to_char("\n\r", ch);
			    }
			}
		    }

		    affect_remove_room(room, paf);
		}
	    }

	    if (room->sector_type == SECT_WATER_NOSWIM
		|| room->sector_type == SECT_WATER_SWIM)
	    {
		OBJ_DATA *obj, *obj_next;

  		for (obj = room->contents; obj; obj = obj_next)
  		{
		    int chance = 10 * ((check_material(obj, DAM_BASH)
					+ check_material(obj, DAM_PIERCE)
					+ check_material(obj, DAM_SLASH)
					+ check_material(obj, DAM_OTHER)) / 4)
				    + get_obj_weight(obj) / 10;

		    obj_next = obj->next_content;

		    if (obj->item_type != ITEM_BOAT
			&& CAN_WEAR(obj, ITEM_TAKE)
			&& chance > number_bits(6))
		    {
			CHAR_DATA *ch;

			if ((ch = LIST_FIRST(&room->people)) != NULL)
			    act("$p исчеза$r под водой!", ch, obj, NULL, TO_ALL);

			extract_obj(obj, TRUE, FALSE);
		    }
		}
	    }

	    if (!room->area->empty)
	    {
	       if (has_trigger(room->progs, TRIG_DELAY, EXEC_ALL)
	        && room->rprog_delay > 0 && --room->rprog_delay <= 0)
		    p_percent_trigger(NULL, NULL, room, NULL, NULL, NULL,
				      TRIG_DELAY);
	       else if (has_trigger(room->progs, TRIG_RANDOM, EXEC_ALL))
		  p_percent_trigger(NULL, NULL, room, NULL, NULL, NULL,
				  TRIG_RANDOM);
	    }

	    /* Затычка для попытки отыскать баг */
	    if (room->light < 0)
	    {
	        bugf("Room %d - light %d", room->vnum, room->light);
	        room->light = 0;
	    }
	}
}

/*
 * Advancement stuff.
 */
void advance_level(CHAR_DATA *ch, bool hide)
{
    char buf[MAX_STRING_LENGTH];
    int add_hp;
    int add_mana;
    int add_move;
    int add_prac;

    if (IS_NPC(ch))
	return;
    
    // Вроде при повторном лвл апе
    if (ch->pcdata->max_level >= ch->level)
    {
	int add;

	add = (ch->pcdata->perm_hit - 20 - ch->pcdata->train_hit) / (ch->level - 1);
	ch->max_hit 	+= add;
	ch->pcdata->perm_hit	+= add;

	add = (ch->pcdata->perm_mana - 100 - ch->pcdata->train_mana) / (ch->level - 1);
	ch->max_mana	+= add;
	ch->pcdata->perm_mana	+= add;

	add = (ch->pcdata->perm_move - 100) / (ch->level - 1);
	ch->max_move	+= add;
	ch->pcdata->perm_move	+= add;


	return;
    }

    // Основнйо лвл ап
    ch->pcdata->last_level =
	(ch->played + (int) (current_time - ch->logon)) / 3600;

    if (IS_SET(ch->act, PLR_AUTOTITLE))
	set_title(ch, title_table[ch->classid][ch->level][ch->pcdata->true_sex == SEX_FEMALE ? 1 : 0]);


    ch->pcdata->max_level = ch->level;

    add_hp	= con_app[get_curr_stat(ch, STAT_CON)].hitp + number_range(
									   class_table[ch->classid].hp_min,
									   class_table[ch->classid].hp_max);
    add_mana 	= number_range(get_curr_stat(ch, STAT_INT)/5, (2*get_curr_stat(ch, STAT_INT) + get_curr_stat(ch, STAT_WIS))/5);

    add_mana    = (add_mana * (15 + 5 * class_table[ch->classid].fMana)/30);

    add_move	= number_range(1, (get_curr_stat(ch, STAT_CON)
				   + get_curr_stat(ch, STAT_DEX))/6);
    add_prac	= wis_app[get_curr_stat(ch, STAT_WIS)].practice;
	
	int temp_prac;
	int temp_wis = get_curr_stat(ch, STAT_WIS);
	if (temp_wis > 25)
	{
		temp_wis -= 25;
	}
	else
	{
		if (temp_wis > 22)
		{
			temp_wis -= 22;
		}
		else
		{
			if (temp_wis > 18)
			{
				temp_wis -= 18;
			}
			else
			{
				if (temp_wis > 22)
				{
					temp_wis -= 22;
				}
				else
				{
					if (temp_wis > 15)
					{
						temp_wis -= 15;
					}
					else
						temp_wis = 0;
				}//15
			}//18
	   }//22
   }//25

	if (temp_wis > 4)
		temp_wis = 4;					
			
	temp_prac = (temp_wis > number_range(0,5)) ? 1 : 0;

    add_hp = add_hp * 9/10;
    /*    add_mana = add_mana * 9/10; */
    add_move = add_move * 9/10;

    add_hp	= UMAX( 2, add_hp  );
    add_mana	= UMAX( 2, add_mana);
    add_move	= UMAX( 6, add_move);


//повышаем вампирам хп на уровень, решая вопрос с малым сложением
    if (ch->race == RACE_VAMPIRE)
    {
     	add_hp += number_range(2,4);
    }

    ch->max_hit 	+= add_hp;
    ch->max_mana	+= add_mana;
    ch->max_move	+= add_move;
    ch->practice	+= add_prac;
    ch->train		+= 1;

    ch->pcdata->perm_hit	+= add_hp;
    ch->pcdata->perm_mana	+= add_mana;
    ch->pcdata->perm_move	+= add_move;

    if (!hide)
    {
		char buf1[MAX_STRING_LENGTH];

		sprintf(buf1, " %d жизни, %d маны, %d шагов и %d %s.\n\r", add_hp, add_mana, add_move, add_prac, hours(add_prac, TYPE_PRACTICES));

		printf_to_char("{xТы получаешь%s", ch, buf1);

		sprintf(buf, "%s получает%s", ch->name, buf1);
		log_string(buf);

		if (temp_prac)
		{
			send_to_char("{xТакже ты получаешь бонусную практику.\n\r", ch);
			sprintf(buf, "%s получает бонусную практику", ch->name);
			log_string(buf);
		    ch->practice += 1;
		}
    }

    if (IS_VAMPIRE(ch) && ch->level == (PK_RANGE/2) + 2)
	affect_strip(ch, gsn_bite);

    if (ch->level > MAX_RECALL_LEVEL)
    {
	ch->pcdata->learned[gsn_recall] = 0;
	if (ch->level == MAX_RECALL_LEVEL + 1)
	    send_to_char("К сожалению, ты забываешь, как пользоваться возвратом.\n\r", ch);
    }

    if (ch->level >= MAX_NOCLAN_LEVEL && !IS_IMMORTAL(ch) && !is_clan(ch))
    {
	ch->clan = clan_lookup(CLAN_INDEPEND);
	clan_table[ch->clan].count++;
	printf_to_char("Вай-вай-вай... Неужели ты все еще не в клане?\n\r"
		       "Ты присоединяешься к клану %s.\n\r",
		       ch, clan_table[ch->clan].short_descr);
    }

    if (ch->level < MIN_CLAN_LEVEL && is_clan(ch))
    {
	printf_to_char("Ты стал слишком мал для клана %s.\n\r",
		       ch, clan_table[ch->clan].short_descr);
	clan_table[ch->clan].count--;
	ch->clan = 0;
    }

    if (ch->level == (PK_RANGE)*2)
	REMOVE_BIT(ch->comm, COMM_HELPER);

    if (ch->level == 15)
	send_to_char("{RНЕ ЗАБУДЬ, ЧТО У ТЕБЯ ТЕПЕРЬ ДОЛЖНО БЫТЬ НОРМАЛЬНОЕ ОПИСАНИЕ!{x\n\r", ch);

	restore(ch, ch);

    return;
}



void gain_exp(CHAR_DATA *ch, int gain, bool silent)
{
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch) || IS_IMMORTAL(ch) || gain == 0)
	return;

    if (gain > 0 && IS_SET(ch->act, PLR_NOEXP))
        return;
	
    ch->exp = UMAX(exp_per_level(ch, ch->pcdata->points), ch->exp + gain);

    if (gain > 0)
    {
  	while (ch->level < LEVEL_HERO && TNL(ch) <= 0)
    	{
    	    ch->level++;

    	    if (!silent)
    	    {
      		send_to_char("{GТы повышаешь УРОВЕНЬ!!  ", ch);
      		sprintf(buf, "%s gained level %d", ch->name, ch->level);
      		log_string(buf);
      		sprintf(buf, "$N повышает уровень до %d-го!", ch->level);
      		wiznet(buf, ch, NULL, WIZ_LEVELS, 0, 0);
    	    }

    	    advance_level(ch, silent);
    	    save_char_obj(ch, FALSE);
      	}
    }
    else
    {
	int epl = exp_per_level(ch, ch->pcdata->points);;

	while ((ch->level + 1) * epl - ch->exp > 2 * epl)
	{
	    int add;

	    if (!silent)
		send_to_char("{RТы теряешь УРОВЕНЬ!!{x\n\r", ch);


	    add = (ch->pcdata->perm_hit - 20 - ch->pcdata->train_hit) / ch->level;
	    ch->max_hit 		-= add;
	    ch->pcdata->perm_hit	-= add;

	    add = (ch->pcdata->perm_mana - 100 - ch->pcdata->train_mana) / ch->level;
	    ch->max_mana		-= add;
	    ch->pcdata->perm_mana	-= add;

	    add = (ch->pcdata->perm_move - 100) / ch->level;
	    ch->max_move		-= add;
	    ch->pcdata->perm_move	-= add;

	    ch->level--;
	}
    }

    return;
}



int gain_hmm(CHAR_DATA *ch, int gain, bool moves)
{
    if (!IS_NPC(ch) && !IS_IMMORTAL(ch))
    {
      	int i;

	if (ch->pcdata->condition[COND_HUNGER] == 0)
	{
	    i = is_penalty_room(ch->in_room) ? 100 : number_percent() + moves ? 20 : 0;

	    if (i < 30
		|| (IS_AFFECTED(ch, AFF_REGENERATION)
		    && number_percent() < 30))
	    {
		gain /= 2;
	    }
	    else if (i < 60 && !moves)
		gain = -1;
	    else
		gain = 0;
	}

	if (ch->pcdata->condition[COND_THIRST] == 0)
	{
	    i = is_penalty_room(ch->in_room) ? 100 : number_percent() + moves ? 20 : 0;

	    if (i < 30
		|| (IS_AFFECTED(ch, AFF_REGENERATION)
		    && number_percent() < 30))
	    {
		gain = gain > 0 ? gain/2 : gain < 0 ? -1 : 0;
	    }
	    else if (i < 60 && !moves)
		gain = gain < 0 ? -2 : -1;
	    else
		gain = gain < 0 ? -1 : 0;
	}
    }

    if (gain > 0)
    {
      	gain *= (ch->in_room->heal_rate
		+ (IS_SET(ch->in_room->room_flags, ROOM_SAFE) ? 50 : 0)) / 100;

	if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
	    gain = gain * ch->on->value[3] / 100;

	if (IS_AFFECTED(ch, AFF_POISON))
	    gain /= 4;

	if (IS_AFFECTED(ch, AFF_PLAGUE))
	    gain /= 8;

	if (IS_AFFECTED(ch, AFF_HASTE) || IS_AFFECTED(ch, AFF_SLOW))
	    gain /= 2;

	if (IS_NOT_SATIETY(ch))
	    gain /= 4;
    }

    return gain;
}

/*
 * Regeneration stuff.
 */
int hit_gain(CHAR_DATA *ch)
{
    int gain;
    int number;

    if (ch->in_room == NULL)
	return 0;

    if (IS_NPC(ch))
    {
	gain = 5 + ch->level;

	switch(ch->position)
	{
	case POS_SLEEPING:
	    gain  = 3 * gain/2;
	    break;
	case POS_BASHED:
	    gain /= 4;
	    break;
     	case POS_FIGHTING:
	    gain /= 3;
	    break;
	default:
	    gain /= 2;
	    break;
 	}

        if (IS_AFFECTED(ch, AFF_REGENERATION))
	    gain *= 2;

    }
    else
    {
	gain = UMAX(3, get_curr_stat(ch, STAT_CON) - 3 + ch->level/2);
	gain += class_table[ch->classid].hp_max - 10;
 	number = number_percent();
	if (number < get_skill(ch, gsn_fast_healing))
	{
	    gain += number * gain / 100;
	    if (ch->hit < ch->max_hit)
		check_improve(ch, NULL, gsn_fast_healing, TRUE, 4);
	}

	switch (ch->position)
	{
	case POS_SLEEPING:
	    break;
	case POS_RESTING:
	    gain /= 2;
	    break;
	case POS_FIGHTING:
	case POS_BASHED:
	    gain /= 6;
	    break;
	default:
	    gain /= 4;
	    break;
	}

	//увеличен рест
	gain *= 1.5;

	if (IS_AFFECTED(ch, AFF_REGENERATION))
	    gain *= 1.5;
    }

    gain = gain_hmm(ch, gain, FALSE);

    if (IS_SET(ch->in_room->room_flags, ROOM_SLOW_DT))
    {
	number = number_range(ch->in_room->heal_rate / 2,
			      3 * ch->in_room->heal_rate / 2);
	gain = gain < 0 ? gain - number : -number;

	if (number_percent() < 100 - 100 * ch->hit/ch->max_hit)
	    printf_to_char("Ты попал%s в ловушку! Выбирайся скорее отсюда!\n\r", ch, SEX_ENDING(ch));
    }

    if (gain < 0)
    {
	damage(ch, ch, 0 - gain, TYPE_UNDEFINED, DAM_NONE, FALSE, NULL);
	gain = 0;
    }

    return UMIN(gain, ch->max_hit - ch->hit);
}



int mana_gain(CHAR_DATA *ch)
{
    int gain;
    int number;

    if (ch->in_room == NULL)
	return 0;

    if (IS_NPC(ch))
    {
	gain = 5 + ch->level;
	switch (ch->position)
	{
	case POS_SLEEPING:
	    gain  = 3 * gain/2;
	    break;
	case POS_BASHED:
	    gain /= 4;
	    break;
	case POS_FIGHTING:
	    gain /= 3;
	    break;
	default:
	    gain /= 2;
	    break;
    	}

	//увеличен рест
	gain *= 1.5;
	if (IS_AFFECTED(ch, AFF_REGENERATION))
	    gain *= 2;
    }
    else
    {
	gain = (get_curr_stat(ch, STAT_WIS)
		+ get_curr_stat(ch, STAT_INT) + ch->level) / 2;
	number = number_percent();
	if (number < get_skill(ch, gsn_meditation))
	{
	    gain += number * gain / 50;
	    if (ch->mana < ch->max_mana)
	        check_improve(ch, NULL, gsn_meditation, TRUE, 4);
	}

	gain = (gain * (15 + 5 * class_table[ch->classid].fMana)/30);

	switch (ch->position)
	{
	case POS_SLEEPING:
	    break;
    	case POS_RESTING:
	    gain /= 2;
	    break;
	case POS_BASHED:
	case POS_FIGHTING:
	    gain /= 6;
	    break;
	default:
	    gain /= 4;
	    break;
	}

	if (IS_AFFECTED(ch, AFF_REGENERATION))
	    gain *= 1.5;

    }

    return UMIN(gain_hmm(ch, gain, FALSE), ch->max_mana - ch->mana);
}



int move_gain(CHAR_DATA *ch)
{
    int gain;

    if (ch->in_room == NULL)
	return 0;

    if (IS_NPC(ch))
    {
	gain = ch->level;

        if (IS_AFFECTED(ch, AFF_REGENERATION))
	    gain *= 2;

    }
    else
    {
	int stats = (get_curr_stat(ch, STAT_CON) + get_curr_stat(ch, STAT_DEX))/2;

	gain = UMAX(15, ch->level);

	switch (ch->position)
	{
	case POS_SLEEPING:
	    gain += stats;
	    break;
       	case POS_RESTING:
	    gain += stats/2;
	    break;
     	case POS_BASHED:
	case POS_FIGHTING:
	    gain /= 2;
	    break;
	}

	if (IS_AFFECTED(ch, AFF_REGENERATION))
	    gain *= 1.5;
    }

    return UMIN(gain_hmm(ch, gain, TRUE), ch->max_move - ch->move);
}



void gain_condition(CHAR_DATA *ch, int iCond, int value)
{
    int condition;

    if (value == 0 || IS_NPC(ch) || ch->level >= LEVEL_IMMORTAL)
	return;

    condition = ch->pcdata->condition[iCond];

    if (condition == -1)
        return;

    ch->pcdata->condition[iCond] = URANGE(0, condition + value, 48);

    if (IS_VAMPIRE(ch) && iCond != COND_DRUNK
	&& ch->pcdata->condition[iCond] < 2)
    {
	ch->pcdata->condition[iCond] = 2;
    }

    if (ch->pcdata->condition[iCond] == 0)
    {
	switch (iCond)
	{
	case COND_HUNGER:
	    send_to_char("Ты хочешь есть.\n\r",  ch);
	    break;

	case COND_THIRST:
	    send_to_char("Ты хочешь пить.\n\r", ch);
	    break;

	case COND_DRUNK:
	    if (condition != 0)
		send_to_char("Ты трезвеешь.\n\r", ch);
	    break;
	}
    }
    else if (ch->pcdata->condition[iCond] < 2)
    {
	switch (iCond)
	{
	case COND_HUNGER:
	    send_to_char("Ты чувствуешь легкий голод.\n\r",  ch);
	    break;

	case COND_THIRST:
	    send_to_char("Ты чувствуешь легкую жажду.\n\r", ch);
	    break;
	}

    }

    return;
}


bool can_mob_move(CHAR_DATA *ch, int door)
{
    EXIT_DATA *pexit;
    ROOM_INDEX_DATA *to_room;

    if (!IS_SWITCHED(ch) && !IS_SET(ch->act, ACT_SENTINEL)
	&& ch->fighting == NULL
	&& door < MAX_DIR && door >= 0
	&& ch->in_room
	&& (pexit = ch->in_room->exit[door]) != NULL
	    && (to_room = pexit->u1.to_room) != NULL
	    && !IS_SET(pexit->exit_info, EX_CLOSED)
	&& !IS_SET(to_room->room_flags, ROOM_NO_MOB)
	&& (!IS_SET(ch->act, ACT_STAY_AREA)
	    || to_room->area == ch->in_room->area)
	&& (!IS_SET(ch->act, ACT_OUTDOORS)
	    || !IS_SET(to_room->room_flags, ROOM_INDOORS))
	&& (!IS_SET(ch->act, ACT_INDOORS)
	    || IS_SET(to_room->room_flags, ROOM_INDOORS))
	&& (pexit->u1.to_room->area == ch->in_room->area
	    || (!IS_SET(ch->in_room->area->area_flags, AREA_NA)
		&& !IS_SET(ch->in_room->area->area_flags, AREA_TESTING)))
	&& CHECK_AREAL(ch, to_room))
    {
	return TRUE;
    }
    return FALSE;
}

bool hmm_update(CHAR_DATA *ch)
{
    int pen;
    int hun = 0;
    int ret = FALSE;
    int div;
    int32_t tmana = 0;
    int32_t tmove = 0;
    int32_t thit  = 0;

    if (!IS_NPC(ch))
    {
	pen = is_penalty_room(ch->in_room);
    
	if (ch->pcdata->condition[COND_HUNGER] == 0 && pen != TRUE)
	    hun -= 1;

	if (ch->pcdata->condition[COND_THIRST] == 0 && pen != TRUE)
	    hun -= 1;
    }

    if (hun == 0)
	hun = 10;

    div = 60 + ch->level * 2;
    
    thit  = hun * UMIN(6000, ch->max_hit)  / 20;
    tmana = hun * UMIN(6000, ch->max_mana) / 20;
    tmove = hun * UMIN(1000, ch->max_move) / 20;

    if (IS_SET(ch->in_room->room_flags, ROOM_SLOW_DT))
    {
	int number = number_range(ch->max_hit, ch->max_hit * 2);

	thit = -number;
	printf_to_char("Ты попал%s в ловушку! Выбирайся скорее отсюда!\n\r", ch, SEX_ENDING(ch));
    } 
    else if (hun > 0)
    {
	int number, thit2 = 0;
	double coeff;
	
	thit  = (thit * ch->in_room->heal_rate) / 100;
	tmana = (tmana * ch->in_room->mana_rate) / 100;

	if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
	{
	    thit  = (thit * ch->on->value[3]) / 100;
	    tmana = (tmana * ch->on->value[4]) / 100;
	}

	if (IS_AFFECTED(ch, AFF_REGENERATION))
	    thit2 = thit * 0.5;

	number = number_percent();
	if (number < get_skill(ch, gsn_fast_healing))
	{
	    thit += thit * number / 50;
	    if (ch->hit < ch->max_hit)
	        check_improve(ch, NULL, gsn_fast_healing, TRUE, 80);
	}
	thit += thit2;

	number = number_percent();
	if (number < get_skill(ch, gsn_meditation))
	{
	    tmana += tmana * number / 50;
	    if (ch->mana < ch->max_mana)
	        check_improve(ch, NULL, gsn_meditation, TRUE, 80);
	}

	switch (ch->position)
	{
	case POS_SLEEPING:
	    coeff = 2.0;
	    break;
	case POS_RESTING:
	    coeff = 1.5;
	    break;
	case POS_SITTING:
	    coeff = 1.2;
	    break;
	case POS_BASHED:
	    coeff = 0.1;
	    break;
	case POS_FIGHTING:
	    coeff = 0.5;
	    break;
	default:
	    coeff = 1.0;
	    break;
	}

	thit  *= coeff;
	tmana *= coeff;
	tmove *= coeff;
    }

    if (thit > 0
	&& (IS_AFFECTED(ch, AFF_POISON)
	    || IS_AFFECTED(ch, AFF_PLAGUE)
	    || is_affected(ch, gsn_fade)
	    || is_affected(ch, gsn_clutch_lycanthrope)
	    || is_affected(ch, gsn_bleed)
	    || is_affected(ch, gsn_immolation)
	    || is_affected(ch, gsn_thornwrack)))
    {
	thit = 0;
    }

    if (tmana > 0
	&& is_affected(ch, gsn_manaleak))
    {
	tmana = 0;
    }

    if (ch->mana <= ch->max_mana || tmana < 0)
    {
	pen = tmana >= 0 ? 1 : -1;
	ch->over_mana += tmana % div;
	if (pen * ch->over_mana >= div)
	{
	    tmana += pen * div;
	    ch->over_mana %= div;
	}
    }
    ch->mana = UMIN(ch->max_mana, ch->mana + tmana / div);

    if (ch->move <= ch->max_move || tmove < 0)
    {
	pen = tmove >= 0 ? 1 : -1;
	ch->over_move += tmove % div;
	if (pen * ch->over_move >= div)
	{
	    tmove += pen * div;
	    ch->over_move %= div;
	}
    }
    ch->move = UMIN(ch->max_move, ch->move + tmove / div);

    if (ch->hit <= ch->max_hit || thit < 0)
    {
	pen = thit >= 0 ? 1 : -1;
	ch->over_hit  += thit % div;
	if (pen * ch->over_hit >= div)
	{
	    thit += pen * div;
	    ch->over_hit %= div;
	}
    }

#ifdef HMM_DEBUG
    printf_to_char("{r-{G:{r- {gh: %d.%d/%d.%d, m: %d.%d/%d.%d, v: %d.%d/%d.%d{x\n\r",
		   ch, thit / div, thit % div, ch->max_hit, ch->over_hit,
		   tmana / div, tmana % div, ch->max_mana, ch->over_mana,
		   tmove / div, tmove % div, ch->max_move, ch->over_move);
#endif
    
    if (thit < 0)
    {
	ret = damage(ch, ch, -thit / div, TYPE_UNDEFINED, DAM_NONE, FALSE, NULL);
        /*
          Fix со спящими чарами и всякими болезнями, хотением поесть и т.п.
        */
        if (IS_VALID(ch) && is_affected(ch, gsn_sleep) && ch->position > POS_SLEEPING)
            ch->position = POS_SLEEPING;
    }
    else
	ch->hit += thit / div;

    if (ch->hit > ch->max_hit)
        ch->hit = ch->max_hit;
        	
    if (ret == FIGHT_DEAD)
	ret = TRUE;
    else
	ret = FALSE;

    return ret;
}

int vampire_update(CHAR_DATA *ch)
{
    int vam_dam = 0, pfl = skill_lookup("protection from light");

    if (!IS_VAMPIRE(ch) || !ch || !ch->in_room || is_affected(ch, pfl))
	return 0;

    if (ch->max_hit < ch->hit)
	ch->hit = ch->max_hit;

    if (weather_info.sunlight != SUN_DARK
	&& ch->in_room->sector_type != SECT_INSIDE
	&& !IS_SET(ch->in_room->room_flags, ROOM_DARK)
	&& !IS_SET(ch->in_room->room_flags, ROOM_INDOORS))
    {
	vam_dam = 20;
	if (IS_NOT_SATIETY(ch))
	    vam_dam += 20;

	if ((weather_info.sunlight == SUN_LIGHT)
	    || (weather_info.sunlight == SUN_SET))
	{
	    vam_dam /= 2;
	}

	if (weather_info.sky == SKY_CLOUDY)
	    vam_dam /= 2;

	if (weather_info.sky == SKY_RAINING)
	    vam_dam /= 3;

	if (weather_info.sky == SKY_LIGHTNING)
	    vam_dam /= 5;

	/*
	 * Чем старше вампир, тем бОльшие мучения вызывает солнечный свет
	 * В лучшем случае (т.е. закат/рассвет, молнии, вампир сыт)
	 * повреждения начинают наноситься только с 41го уровня.
	 * В худшем - 6 hp раз в 4 секунды на 1ом левеле, и 40 - на 51ом.
	 * Иными словами, хаевому вампиру надо быть внимательным. 8))
	 */
	vam_dam /= UMAX((61 - ch->level)/10, 1);

	if (vam_dam == 0)
	    return vam_dam;

	send_to_char("{YСолнечный свет поражает тебя!{x\n\r", ch);
	adv_dam(ch, ch, vam_dam, TYPE_UNDEFINED, DAM_NONE, FALSE, NULL, TRUE, 0);

	//если вампир в лосте, то тоже лост_команду выполняет. Иначе повесится.....
	if (!IS_NPC(ch) && ch->desc == NULL)
    	{
	    if (number_range(0, ch->wait) == 0)
	    {
		if (IS_NULLSTR(ch->lost_command))
		    do_function(ch, &do_recall, "");
		else
		    interpret(ch, ch->lost_command);
	    }
    	}
	//конец куска кода, спасающего :-) вампира
    }

    if (ch->level <= 5)
        return 0;
    else
        return vam_dam;
}

#if 0 /* Due to new tickless update of PCs (Kemm) */
/*
 * Mob autonomous action.
 * This function takes 25% to 35% of ALL Merc cpu time.
 * -- Furey
 */
void mobile_update()
{
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    int door;

    /* Examine all mobs. */
    for (ch = char_list; !END_CHAR_LIST(ch); ch = ch_next)
    {
	ch_next = ch->next;

	if (vampire_update(ch) == 0 && ch->in_room)
	    if (hmm_update(ch) == TRUE)
	    {
		/* char is dead */
		continue;
	    }
	
	if (!IS_NPC(ch))
	    continue;

	if (ch->pIndexData && ch->pIndexData->area && ch->in_room
	    && (IS_SET(ch->pIndexData->area->area_flags, AREA_NA)
		|| IS_SET(ch->pIndexData->area->area_flags, AREA_TESTING))
	    && ch->in_room->area != ch->pIndexData->area)
	{
      	    extract_char(ch, TRUE);
	}

	if (ch->in_room == NULL || IS_AFFECTED(ch, AFF_CHARM))
	    continue;

	if (ch->in_room->area->empty && !IS_SET(ch->act, ACT_UPDATE_ALWAYS))
	    continue;

	/* Examine call for special procedure */
	if (ch->spec_fun && ch->in_room)
	{
	    int i;
	    bool flag = FALSE;

	    for (i = 0; !IS_NULLSTR(spec_table[i].name); i++)
		if (ch->spec_fun == spec_table[i].function)
		{
		    if (!spec_table[i].can_cast
			&& IS_SET(ch->in_room->room_flags, ROOM_NOMAGIC))
		    {
			flag = TRUE;
		    }

		    break;
		}

	    if (!flag)
	    {
		if ((*ch->spec_fun)(ch))
		    continue;
	    }
	}

	if (ch->pIndexData->pShop != NULL) /* give him some gold */
	    if ((ch->gold * 100 + ch->silver) < ch->pIndexData->wealth)
	    {
		ch->gold += ch->pIndexData->wealth * number_range(1, 20)/5000000;
		ch->silver += ch->pIndexData->wealth * number_range(1, 20)/50000;
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

	/* That's all for sleeping / busy monster, and empty zones */
	if (ch->position != POS_STANDING)
	    continue;

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
	    	    obj_best    = obj;
    		    max         = obj->cost;
		}
	    }

	    if (obj_best && SUPPRESS_OUTPUT(!is_have_limit(ch, ch, obj_best)))
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
		for (victim = char_list; !END_CHAR_LIST(victim); victim = victim->next)
		    if (!IS_NPC(victim) && victim->id == mem->id)
			break;

		if (victim != NULL
		    && (tr = get_track(ch, victim)) != NULL
		    && can_mob_move(ch, tr->direction))
		{
		    if (number_percent() < 20)
		    {
			if (!is_animal(ch))
			{
			    act("$n осматривается по сторонам.",
				ch, NULL, NULL, TO_ROOM);
			    do_say(ch, "Я найду тебя!");
			}
			else
			{
			    act("$n шумно принюхивается к следам.",
				ch, NULL, NULL, TO_ROOM);
			}
			move_char(ch, tr->direction, TRUE, FALSE);
		    }
		    continue;
		}
	    }
        }

	if (number_bits(3) < 2 && can_mob_move(ch, (door = number_bits(5))))
	    move_char(ch, door, TRUE, FALSE);

    }

    return;
}
#endif


/*
 * Update the weather.
 */
void weather_update(bool moons)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    int diff;
    bool can_feel = FALSE;
    int i;

    buf[0] = '\0';

    if (moons)
    {
	switch (++time_info.hour)
	{
	case 5:
	    weather_info.sunlight = SUN_LIGHT;
	    strlcat(buf, "Начинается день.\n\r", sizeof(buf));
	    break;

	case 6:
	    weather_info.sunlight = SUN_RISE;
	    strlcat(buf, "Солнце встает на востоке.\n\r", sizeof(buf));
	    break;

	case 19:
	    weather_info.sunlight = SUN_SET;
	    strcat(buf, "Солнце медленно закатывается на запад.\n\r");
	    break;

	case 20:
	    weather_info.sunlight = SUN_DARK;
	    strcat(buf, "Начинается ночь.\n\r");
	    break;

	case 24:
	    time_info.hour = 0;
	    time_info.day++;
	    /* Moons states update */
	    for (i = 0; i < max_moons; i++)
	    {
		if (++moons_data[i].state >= moons_data[i].state_period)
		    moons_data[i].state = 0;
	    }
	    break;
	}

	if (time_info.day >= NUM_DAYS)
	{
	    time_info.day = 0;
	    time_info.month++;
	}

	if (time_info.month >= NUM_MONTHS)
	{
	    time_info.month = 0;
	    time_info.year++;
	}
    }

    /*
     * Weather change.
     */
    if (time_info.month >= 9 && time_info.month <= 16)
	diff = weather_info.mmhg >  985 ? -2 : 2;
    else
	diff = weather_info.mmhg > 1015 ? -2 : 2;

    weather_info.change   += diff * dice(1, 4) + dice(2, 6) - dice(2, 6);
    weather_info.change    = UMAX(weather_info.change, -12);
    weather_info.change    = UMIN(weather_info.change,  12);

    weather_info.mmhg += weather_info.change;
    weather_info.mmhg  = URANGE(960, weather_info.mmhg,  1040);

    switch (weather_info.sky)
    {
    default:
	bugf("Weather_update: bad sky %d.", weather_info.sky);
	weather_info.sky = SKY_CLOUDLESS;
	break;

    case SKY_CLOUDLESS:
	if (weather_info.mmhg <  990
	    || (weather_info.mmhg < 1010 && number_bits(2) == 0))
	{
	    strcat(buf, "На небе появляются облака.\n\r");
	    weather_info.sky = SKY_CLOUDY;
	}
	break;

    case SKY_CLOUDY:
	if (weather_info.mmhg <  970
	    || (weather_info.mmhg <  990 && number_bits(2) == 0))
	{
	    strcat(buf, "Начинается дождь.\n\r");
	    //	    strcat(buf, "Начинается снегопад.\n\r");
	    weather_info.sky = SKY_RAINING;
	    can_feel = TRUE;
	}

	if (weather_info.mmhg > 1030 && number_bits(2) == 0)
	{
	    strcat(buf, "Облака рассеиваются.\n\r");
	    weather_info.sky = SKY_CLOUDLESS;
	}
	break;

    case SKY_RAINING:
	if (weather_info.mmhg <  970 && number_bits(2) == 0)
	{
	    strcat(buf, "Молнии сверкают на небе.\n\r");
	    weather_info.sky = SKY_LIGHTNING;
	}

	if (weather_info.mmhg > 1030
	    || (weather_info.mmhg > 1010 && number_bits(2) == 0))
	{
	    strcat(buf, "Дождь заканчивается.\n\r");
	    weather_info.sky = SKY_CLOUDY;
	    can_feel = TRUE;
	}
	break;

    case SKY_LIGHTNING:
	if (weather_info.mmhg > 1010
	    || (weather_info.mmhg >  990 && number_bits(2) == 0))
	{
	    strcat(buf, "Вспышки молний затихают.\n\r");
	    weather_info.sky = SKY_RAINING;
	    break;
	}
	break;
    }

    /* Moons update */
    if (moons)
	for (i = 0; i < max_moons; i++)
	{
	    int j, k = 0;

	    if (++moons_data[i].moonlight >= moons_data[i].period)
	    {
		moons_data[i].moonlight = 0;

		for (j = 0; j < MAX_MOON_MESSAGES && !IS_NULLSTR(moons_data[i].msg_set[j]); j++)
		    if (number_range(0, j) == 0)
			k = j;

		strlcat(buf, moons_data[i].msg_set[k], sizeof(buf));
		strlcat(buf, "\n\r", sizeof(buf));
	    }
	    else if (moons_data[i].moonlight == moons_data[i].period / 2)
	    {
		for (j = 0; j < MAX_MOON_MESSAGES && !IS_NULLSTR(moons_data[i].msg_rise[j]); j++)
		    if (number_range(0, j) == 0)
			k = j;

		strlcat(buf, moons_data[i].msg_rise[k], sizeof(buf));
		strlcat(buf, "\n\r", sizeof(buf));
	    }
	}

    if (buf[0] != '\0')
    {
	SLIST_FOREACH(d, &descriptor_list, link)
	{
	    if (d->connected == CON_PLAYING
		&& IS_OUTSIDE(d->character)
		&& IS_AWAKE(d->character)
		&& (!IS_AFFECTED(d->character, AFF_BLIND) || can_feel))
	    {
		send_to_char(buf, d->character);
	    }
	}
    }

    return;
}


/*
 * Update all chars, including mobs.
 */
void char_update(void)
{
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    bool show = TRUE;

    LIST_FOREACH_SAFE(ch, &char_list, link, ch_next)
    {
	AFFECT_DATA *paf;
	AFFECT_DATA *paf_next;
        OBJ_DATA *obj;

	if (!IS_VALID(ch))
	{
       	    bugf("Trying to work with an invalidated character (%s).", ch->name);
	    continue;
	}

	if (!IS_NPC(ch))
        {
	    ch->timer++;

	    if (check_freeze(ch))
	    {
		long diff = ch->pcdata->freeze - current_time;

   		if (diff > 0)
		{
     		    char buf[MAX_STRING_LENGTH];
    		    int days, hrs, minutes;
    		    long s;

    		    days = diff / (24 * 60 * 60);
    		    s = diff - days * 24 * 60 * 60;
    		    hrs = s / (60 * 60);
    		    s -= hrs * 60 * 60;
    		    minutes = s/60;

    		    sprintf(buf, "До окончания срока заморозки осталось %d %s, %d ",
			    days, hours(days, TYPE_DAYS), hrs);
		    strcat(buf, hours(hrs, TYPE_HOURS));
                    send_to_char(buf, ch);
                    sprintf(buf, ", %d %s.\n\r", minutes, hours(minutes, TYPE_MINUTES));
		    send_to_char(buf, ch);
		    show = FALSE;
		}
		else
		{
		    REMOVE_BIT(ch->act, PLR_FREEZE);
		    send_to_char("Ты сможешь опять играть.\n\r"
				 "Флаг FREEZE снят.\n\r", ch);
		    ch->pcdata->freeze = 0;
		    show = FALSE;
		}
	    }

	    if (check_channels(ch) && ch->pcdata->nochan < current_time)
	    {
		REMOVE_BIT(ch->comm, COMM_NOCHANNELS);
		REMOVE_BIT(ch->comm, COMM_NOSHOUT);
		REMOVE_BIT(ch->comm, COMM_NOTELL);
		REMOVE_BIT(ch->comm, COMM_NOEMOTE);
		send_to_char("Ты сможешь вновь пользоваться каналами.\n\r", ch);
		ch->pcdata->nochan = 0;
		show = FALSE;
	    }

	    if (IS_SET(ch->comm, COMM_NONOTES)
		&& ch->pcdata->nonotes > 0
		&& ch->pcdata->nonotes < current_time)
	    {
		ch->pcdata->nonotes = 0;
		REMOVE_BIT(ch->comm, COMM_NONOTES);
		send_to_char("Теперь ты снова можешь писать сообщения.\n\r",
			    ch);
		show = FALSE;
	    }

	    if (IS_SET(ch->comm, COMM_NOTITLE)
		&& ch->pcdata->notitle > 0
		&& ch->pcdata->notitle < current_time)
	    {
		ch->pcdata->notitle = 0;
		REMOVE_BIT(ch->comm, COMM_NOTITLE);
		send_to_char("Боги вернули тебе право выбирать титул.\n\r",
			    ch);
		show = FALSE;
	    }

	    if (IS_SET(ch->act, PLR_EXCITED)
		&& !POS_FIGHT(ch) && ch->fighting == NULL)
	    {
		REMOVE_BIT(ch->act, PLR_EXCITED);
		send_to_char("Ты понемногу успокаиваешься.\n\r", ch);
		show = FALSE;
	    }

	    if (ch->pcdata->quaff > 0)
		ch->pcdata->quaff--;

	    if (IS_VAMPIRE(ch))
	    {
  		switch (time_info.hour)
		{
		case 5:
    		    send_to_char("Ты чувствуешь, как с восходящим солнцем "
				 "улетучивается твоя сила...\n\r", ch);
		    break;

	     	case 20:
    		    send_to_char("Твои глаза вспыхивают красным светом! ", ch);

    		    if (ch->level >= skill_table[gsn_bite].skill_level[CLASS_VAMPIRE])
		      	send_to_char("Тело наливается силой, клыки становятся "
		   		     "длиннее...", ch);

		    send_to_char("\n\r", ch);
		    break;

	     	default:
		    break;
	  	}
	    }
	}
	else
	{
	    if (IS_SET(ch->act, ACT_MOUNT)
	      	&& ch->fighting == NULL
	  	&& ch->position < POS_STANDING
	    	&& ch->master == NULL)
	    {
	        do_stand(ch, "");
	    }

	    if (ch->angry > 0)
		ch->angry--;
	}

        if (ch->position >= POS_STUNNED)
        {
#if 0
            ch->hit  = UMIN(ch->max_hit,  ch->hit  + hit_gain(ch));
	    ch->mana = UMIN(ch->max_mana, ch->mana + mana_gain(ch));
	    ch->move = UMIN(ch->max_move, ch->move + move_gain(ch));
#endif

	    if (IS_NOT_SATIETY(ch))
	        printf_to_char("%s\n\r", ch, skill_table[gsn_bite].msg_off);
	}

	update_pos(ch);

	if (ch->position <= POS_STUNNED)
	{
	    damage(ch, ch, 0 - number_bits(4), TYPE_UNDEFINED, DAM_NONE, FALSE, NULL);
	    show = FALSE;
	}

	if ((obj = get_eq_char(ch, WEAR_LIGHT)) != NULL
	    && obj->item_type == ITEM_LIGHT
	    && obj->value[2] > 0)
	{
    	    if (--obj->value[2] == 0)
	    {
		if (ch->in_room)
		{
		    --ch->in_room->light;
		    act("$p исчеза$r.", ch, obj, NULL, TO_ROOM);
		    act("$p мерца$r и исчеза$r.", ch, obj, NULL, TO_CHAR);
		    show = FALSE;
		}
		extract_obj(obj, TRUE, TRUE);
	    }
	    else if (obj->value[2] <= 5)
	    {
		act("$p мерца$r.", ch, obj, NULL, TO_CHAR);
		show = FALSE;
	    }
	}

	if (ch->level < LEVEL_IMMORTAL)
	{
	    if (ch->timer >= 12)
	    {
		if (ch->was_in_room == NULL && ch->in_room != NULL)
		{
		    ch->was_in_room = ch->in_room;

		    if (ch->fighting != NULL)
			stop_fighting(ch, TRUE);

		    act("$n исчезает в никуда.",
			ch, NULL, NULL, TO_ROOM);
		    send_to_char("Ты исчезаешь в никуда.\n\r", ch);
		    show = FALSE;

		    VALIDATE(ch);

		    if (ch->mount
	    	     && ch->mount->pIndexData
	   	     && ch->mount->pIndexData->count < 2)
        	        pet_gone(ch->mount);
		    
		    if (ch->level > 1)
		        save_char_obj(ch, FALSE);
		    
		    char_from_room(ch);
		    char_to_room(ch, get_room_index(ROOM_VNUM_LIMBO), FALSE);

		}
	    }
	    if (is_wear_slow_digestion(ch))
	    {
	    	gain_condition(ch, COND_DRUNK,  -1);
	    	gain_condition(ch, COND_FULL, ch->size > SIZE_MEDIUM ? -2 : -1);
	    	gain_condition(ch, COND_THIRST, -number_range(0, 1));
	    	gain_condition(ch, COND_HUNGER, ch->size > SIZE_MEDIUM
						? -1 : -number_range(0, 1));
	    }
	    else
	    {
	    	gain_condition(ch, COND_DRUNK,  -1);
	    	gain_condition(ch, COND_FULL, ch->size > SIZE_MEDIUM ? -4 : -2);
	    	gain_condition(ch, COND_THIRST, -1);
	    	gain_condition(ch, COND_HUNGER, ch->size > SIZE_MEDIUM
						? -2 : -1);
	    }
	}

        if (((ch->desc == NULL && ch->timer >= 15)
		|| (ch->timer > 30 && !IS_IMMORTAL(ch)))
	    && get_original(ch) == NULL)
        {
	    do_quit(ch, "");
     	    continue;
        }

      	if (!IS_NPC(ch) && ch->position == POS_SLEEPING)
      	{
       	    if (!IS_SET(ch->act, PLR_NODREAMS) && number_percent() <= 10)
	    {
	    	int count, cn = 0;

		for (count = 0;
		     count < MAX_DREAMS && dream_table[count] != NULL;
		     count++)
		{
		    if (number_range(0, count) == 0)
			cn = count;
		}

		if (dream_table[cn] != NULL)
		{
		    send_to_char("{BТебе снится сон.....{x\n\r\n\r", ch);
		    page_to_char(dream_table[cn], ch);
		    show = FALSE;
		}
	    }

	    if (!IS_IMMORTAL(ch) && number_percent() < get_age(ch))
	    {
		if (IS_VAMPIRE(ch))
		{
		    act(number_bits(2) == 0
			    ? "$n довольно скалится во сне."
			    : "$n щелкает во сне клыками.",
			ch, NULL, NULL, TO_ROOM);
		}
		else if (ch->sex != SEX_FEMALE)
		{
		    act("$n храпит во сне: {CХрр... хррррр... хррр...{x",
			ch, NULL, NULL, TO_ROOM);
		}
		else
		{
		    act("$n посапывает во сне: {CПсс... пссссс... пссс...{x",
			ch, NULL, NULL, TO_ROOM);
		}
		show = FALSE;
	    }
      	}

	if (is_affected(ch, gsn_form_bear))
	    check_improve(ch, NULL, gsn_form_bear, TRUE, 2);
	if (is_affected(ch, gsn_form_wolf))
	    check_improve(ch, NULL, gsn_form_wolf, TRUE, 2);
	if (is_affected(ch, gsn_spirit_bear))
	    check_improve(ch, NULL, gsn_spirit_bear, TRUE, 2);
	if (is_affected(ch, gsn_spirit_wolf))
	    check_improve(ch, NULL, gsn_spirit_wolf, TRUE, 2);

	for (paf = ch->affected; paf != NULL; paf = paf_next)
	{
	    paf_next	= paf->next;
	    if (paf->duration > 0)
	    {
		paf->duration--;
		if (number_range(0, 4) == 0 && paf->level > 0)
		    paf->level--;  /* spell strength fades with time */
            }
	    else if (paf->duration < 0)
		;
	    else
	    {
		if (paf_next == NULL
		    || paf_next->type != paf->type
		    || paf_next->duration > 0)
		{
		    if (paf->type > 0 && skill_table[paf->type].msg_off)
		    {
			act(skill_table[paf->type].msg_off, ch, NULL, NULL, TO_CHAR);
			show = FALSE;
		    }
		}

	        if (paf->type == gsn_shapechange)
		    extract_char(ch, TRUE);
	        else
	        {
		    if (paf->bitvector == AFF_CHARM && ch->master)
		    {
		        act("$N прекращает следовать за тобой.", ch->master, NULL, ch, TO_CHAR);
		        ch->leader = NULL;   
		        ch->master = NULL;		        
		    }		    
		    affect_remove(ch, paf);
		    if (paf->type == gsn_form_bear
			|| paf->type == gsn_form_wolf
			|| paf->type == gsn_spirit_wolf
			|| paf->type == gsn_spirit_bear)
			recovery_lycanthrope(ch);
		}
	    }
	}

	/*
	 * Careful with the damages here,
	 *   MUST NOT refer to ch after damage taken,
	 *   as it may be lethal damage (on NPC).
	 */
	if (IS_VALID(ch) && is_affected(ch, gsn_thornwrack))
	{
	    AFFECT_DATA *tw;

	    if ((tw = affect_find(ch->affected, gsn_thornwrack)) != NULL)
	    {
	        CHAR_DATA *caster;

		send_to_char("Ты чувствуешь страшную боль по всему телу!\n\r",
			     ch);
		act("Из-под кожи $n1 лезут страшные иглы и шипы!",
		    ch, NULL, NULL, TO_ROOM);
		show = FALSE;

		caster = pc_id_lookup(tw->caster_id); 

		adv_dam(caster ? caster : ch, ch, dice(tw->level/2, 3),
		        gsn_thornwrack, DAM_PIERCE, FALSE, NULL, TRUE, gsn_thornwrack);
	    }
	}

	if (IS_VALID(ch) && is_affected(ch, gsn_fade))
	{
	    AFFECT_DATA *tw;

	    if ((tw = affect_find(ch->affected, gsn_fade)) != NULL)
	    {
	        CHAR_DATA *caster;

		send_to_char("Твое тело отказывается работать...\n\r", ch);
		show = FALSE;

		caster = pc_id_lookup(tw->caster_id); 
		adv_dam(caster ? caster : ch, ch, dice(tw->level, 10),
		        gsn_fade, DAM_DISEASE, FALSE, NULL, TRUE, gsn_fade);
	    }
	}

	if (IS_VALID(ch) && is_affected(ch, gsn_clutch_lycanthrope))
	{
	    AFFECT_DATA *tw;

	    if ((tw = affect_find(ch->affected,gsn_clutch_lycanthrope)) != NULL)
	    {
	        CHAR_DATA *caster;

		send_to_char("Раны от когтей воспаляются и кровоточат...\n\r", ch);
		show = FALSE;

		caster = pc_id_lookup(tw->caster_id); 
		adv_dam(caster ? caster : ch, ch, dice(tw->level, 10),
		        gsn_clutch_lycanthrope, DAM_POISON, FALSE, NULL, TRUE, gsn_clutch_lycanthrope);
	    }
	}

	if (IS_VALID(ch) && is_affected(ch, gsn_bleed))
	{
	    AFFECT_DATA *tw;

	    if ((tw = affect_find(ch->affected, gsn_bleed)) != NULL)
	    {
	        CHAR_DATA *caster;

		send_to_char("Твои раны кровоточат...\n\r", ch);
		show = FALSE;

		caster = pc_id_lookup(tw->caster_id); 
		adv_dam(caster ? caster : ch, ch, dice(tw->level, 6),
		        gsn_bleed, DAM_SLASH, FALSE, NULL, TRUE, gsn_bleed);
	    }
	}

	if (IS_VALID(ch) && is_affected(ch, gsn_immolation))
	{
	    AFFECT_DATA *tw;

	    if ((tw = affect_find(ch->affected, gsn_immolation)) != NULL)
	    {
	        CHAR_DATA *caster;

		send_to_char("Языки пламени, окружающие тебя, причиняют тебе боль.\n\r", ch);
		show = FALSE;

		caster = pc_id_lookup(tw->caster_id); 
		adv_dam(caster ? caster : ch, ch, dice(tw->level, 10),
		        gsn_immolation, DAM_FIRE, FALSE, NULL, TRUE, gsn_immolation);
	    }
	}

	if (IS_VALID(ch) && IS_AFFECTED(ch, AFF_PLAGUE))
	{
	    AFFECT_DATA *af, plague;

	    if ((af = affect_find(ch->affected, gsn_plague)) != NULL)
	    {	
		CHAR_DATA *vch, *caster, *safe_vch;
		int dam;

		show = FALSE;

		act("$n корчится от боли, чумные болячки нарывают на $s3 коже.",
		    ch, NULL, NULL, TO_ROOM);

		send_to_char("Ты корчишься от боли.\n\r", ch);

		if (IS_AFFECTED(ch, AFF_SLEEP))
		{
			affect_strip(ch, gsn_sleep);
		}

		caster = pc_id_lookup(af->caster_id); 

		if (af->level > 1 && ch->in_room != NULL)
		{
		    plague.where	= TO_AFFECTS;
		    plague.type 	= gsn_plague;
		    plague.level 	= af->level - 1;
		    plague.duration 	= number_range(1, 2 * plague.level);
		    plague.location	= APPLY_STR;
		    plague.modifier 	= -5;
		    plague.bitvector 	= AFF_PLAGUE;
		    plague.caster_id 	= af->caster_id;

		    LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, safe_vch)
		    {
			if (!saves_spell(plague.level - 2, vch, DAM_DISEASE)
			    && !IS_IMMORTAL(vch)
			    && !IS_AFFECTED(vch, AFF_PLAGUE)
			    && (!IS_NPC(vch) || !IS_SET(vch->act, ACT_UNDEAD))
			    && !IS_VAMPIRE(vch)
			    && number_bits(4) == 0)
			{
			    send_to_char("Тебя кидает в жар и лихорадит.\n\r", vch);
			    act("$n дрожит и выглядит очень больн$t.",
				vch, SEX_END_ADJ(vch), NULL, TO_ROOM);
			    show = FALSE;
			    affect_join(vch, &plague);
			}
		    }
		}
		dam = UMIN(ch->level, af->level/5+1);
		ch->mana -= dam;
		ch->move -= dam;

		adv_dam(caster ? caster : ch, ch, dam, gsn_plague, DAM_DISEASE,
			FALSE, NULL, TRUE, gsn_plague);
	    }
	}

        if (IS_VALID(ch)
	    && IS_AFFECTED(ch, AFF_POISON)
	    && !IS_AFFECTED(ch, AFF_SLOW))
	{
	    AFFECT_DATA *poison;

	    poison = affect_find(ch->affected, gsn_poison);

	    if (poison != NULL)
	    {
	        CHAR_DATA *caster;

	        act("$n дрожит и трясется.", ch, NULL, NULL, TO_ROOM);
	        send_to_char("Ты дрожишь и трясешься.\n\r", ch);
		show = FALSE;
		caster = pc_id_lookup(poison->caster_id);
	        adv_dam(caster ? caster : ch, ch, poison->level/10 + 1, gsn_poison, DAM_POISON,
		       FALSE, NULL, TRUE, gsn_poison);
		if (IS_AFFECTED(ch, AFF_SLEEP))
		{
			affect_strip(ch, gsn_sleep);
		}
	    }
	}

      	if (ch != NULL && !IS_NPC(ch) && is_wear_teleport(ch))
	{
	    teleport_curse(ch);
	    show = FALSE;
	}

        if (IS_VALID(ch) && ch->in_room && !IS_IMMORTAL(ch)
	    && ch->in_room->sector_type == SECT_WATER_NOSWIM
	    && !IS_AFFECTED(ch, AFF_FLYING)
	    && !IS_AFFECTED(ch, AFF_SWIM))
        {
	    OBJ_DATA *obj;

	    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
		if (obj->item_type == ITEM_BOAT)
		    break;

            if (!obj)
            {
	      	send_to_char("Ты тонешь!!!\n\r", ch);
		damage(ch, ch, ch->max_hit, TYPE_UNDEFINED,
	    	       DAM_DROWNING, FALSE, NULL);
            }
        }

	if (IS_VALID(ch))
	{
	    if (((current_time - ch->logon) % 3600)/60 == 59 && !IS_IMMORTAL(ch))
		send_to_char("Ты уже довольно долго играешь, может, тебе стоит отдохнуть?\n\r", ch);
	    else if (show && ch->desc)
		send_to_char("{/", ch);
	}
        /*
           Fix со спящими чарами и всякими болезнями, хотением поесть и т.п.
           Должно быть последним в цикле.
        */
        if (IS_VALID(ch) && is_affected(ch, gsn_sleep) && ch->position > POS_SLEEPING)
  	    ch->position = POS_SLEEPING;

	/* Validate position */
	update_pos(ch);
    }

    /*
     * Autosave
     */
    LIST_FOREACH(ch, &char_list, link)
	if (!IS_NPC(ch) || ch->desc != NULL)
	{
	    VALIDATE(ch);
	    save_char_obj(ch, FALSE);
	}

    return;
}

/*
 * Update all objs.
 * This function is performance sensitive.
 */
void obj_update(void)
{
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    OBJ_DATA *t_obj, *next_obj;
    AFFECT_DATA *paf, *paf_next;
    AREA_DATA *pArea = NULL;
    long id = -1;

    for (obj = object_list; obj != NULL; obj = obj_next)
    {
	CHAR_DATA *rch;
	char message[MAX_STRING_LENGTH];

	obj_next = obj->next;

	if (obj->in_room)
	{
	    pArea = obj->in_room->area;
	}
	else if (obj->in_obj)
	{
	    if (obj->in_obj->in_room)
		pArea = obj->in_obj->in_room->area;
	    else if (obj->in_obj->carried_by && obj->in_obj->carried_by->in_room)
		pArea = obj->in_obj->carried_by->in_room->area;
	}
	else if (obj->carried_by && obj->carried_by->in_room)
	    pArea = obj->carried_by->in_room->area;

	if (pArea && obj->pIndexData
	    && obj->pIndexData->area != pArea
	    && (IS_SET(obj->pIndexData->area->area_flags, AREA_NA)
		|| IS_SET(obj->pIndexData->area->area_flags, AREA_TESTING)))
	{
		extract_obj(obj, TRUE, TRUE);
		continue;
	}

	if (obj->item_type == ITEM_ROD)
	{
	    if (obj->value[3] == 1 && obj->carried_by != NULL)
	    {
		printf_to_char("Теперь %s можно использовать.\n\r",
			       obj->carried_by, cases(obj->short_descr, 6));
	    }

            if (obj->value[3] > 0)
	        obj->value[3]--;
	}

	if (obj->pIndexData != NULL
	    && obj->pIndexData->vnum == OBJ_VNUM_SKELETON
	    && (obj->carried_by == NULL
		&& (obj->in_obj == NULL || obj->in_obj->carried_by == NULL))
	    && number_range(0, obj->level) <= 3)
	{
	    obj->timer = 1;
	}

	if (obj->wear_loc != WEAR_NONE && (rch = obj->carried_by) != NULL
	    && !IS_OBJ_STAT(obj, ITEM_NOREMOVE)
	    && is_uncomf(rch, obj)
	    && number_bits(4) == 0)
	{
            act("$p сполза$r со своего места. Да, собственно, тебе и не очень "
		"удобно было.", rch, obj, NULL, TO_CHAR);

	    if (obj->wear_loc == WEAR_WIELD || obj->wear_loc == WEAR_SECONDARY
	       || obj->wear_loc == WEAR_HOLD)	
	        act("$p выскальзыва$r из рук $n1.", rch, obj, NULL, TO_ROOM);
	    else     
                act("$p сполза$r с тела $n1.", rch, obj, NULL, TO_ROOM);
            unequip_char(rch, obj, TRUE);
	}


	/* go through affects and decrement */
        for (paf = obj->affected; paf != NULL; paf = paf_next)
        {
            paf_next    = paf->next;
            if (paf->duration > 0)
            {
                paf->duration--;
                if (number_range(0, 4) == 0 && paf->level > 0)
                  paf->level--;  /* spell strength fades with time */
            }
            else if (paf->duration < 0)
                ;
            else
            {
                if (paf_next == NULL
		    || paf_next->type != paf->type
		    || paf_next->duration > 0)
                {
                    if (paf->type > 0 && skill_table[paf->type].msg_obj)
                    {
			if (obj->carried_by != NULL)
			{
			    rch = obj->carried_by;
			    act(skill_table[paf->type].msg_obj,
				rch, obj, NULL, TO_CHAR);
			}
			if (obj->in_room != NULL
			    && !LIST_EMPTY(&obj->in_room->people))
			{
			    rch = LIST_FIRST(&obj->in_room->people);
			    act(skill_table[paf->type].msg_obj,
				rch, obj, NULL, TO_ALL);
			}
                    }
                }

                affect_remove_obj(obj, paf);
            }
        }
	/*
	 * Oprog triggers!
	 */
	if (obj->in_room || (obj->carried_by && obj->carried_by->in_room))
	{
	    if (has_trigger(obj->pIndexData->progs, TRIG_DELAY, EXEC_ALL)
		&& obj->oprog_delay > 0)
	    {
	        if (--obj->oprog_delay <= 0)
		    p_percent_trigger(NULL, obj, NULL, NULL, NULL, NULL, TRIG_DELAY);
	    }
	    else if (((obj->in_room && !obj->in_room->area->empty)
	    	|| obj->carried_by) && has_trigger(obj->pIndexData->progs, TRIG_RANDOM, EXEC_ALL))
		    p_percent_trigger(NULL, obj, NULL, NULL, NULL, NULL, TRIG_RANDOM);
	 }
	/* Make sure the object is still there before proceeding */
	if (!IS_VALID(obj))
	    continue;

	if (obj->timer <= 0 || --obj->timer > 0)
	{
            if (obj->in_room &&
                obj->in_room->sector_type == SECT_AIR &&
                (obj->wear_flags & ITEM_TAKE) &&
                obj->in_room->exit[DIR_DOWN] &&
                obj->in_room->exit[DIR_DOWN]->u1.to_room)
            {
		ROOM_INDEX_DATA *new_room = obj->in_room->exit[DIR_DOWN]->u1.to_room;

		if ((rch = LIST_FIRST(&obj->in_room->people)) != NULL)
		{
		    act("$p пада$r вниз..... Бумс!", rch, obj, NULL, TO_ALL);
		}
		obj_from_room(obj);
		obj_to_room(obj, new_room);

		if ((rch = LIST_FIRST(&obj->in_room->people)) != NULL)
		{
		    act("$p со свистом прилета$r сверху. Бумс! "
			"Хорошо, что ты вовремя от этого увернулся!",
			rch, obj, NULL, TO_ALL);
		}
            }

            if (obj->pIndexData->vnum == OBJ_VNUM_DISC
		&& obj->timer > 0 && obj->timer <= 5
		&& obj->carried_by)
	    {
	       	act("$p изда$r странное гудение...", obj->carried_by, obj, NULL, TO_ALL);
	    }

            if (is_limit(obj) == -1 || obj->lifetime == 0 || obj->lifetime > current_time)
		continue;
	}

	if (obj->pIndexData->vnum == OBJ_VNUM_SOUL)
	{
	    if (obj->value[0] < -333)
	       	strcpy(message, "$p провалива$rся глубоко-глубоко вниз...");
	    else if (obj->value[0] >= -333 && obj->value[0] <= 333)
		strcpy(message, "$p растворя$rся в пустоте...");
	    else
		strcpy(message, "$p улета$r далеко-далеко на небеса...");
	}
	else if (obj->pIndexData->vnum == OBJ_VNUM_TAIL
		 || obj->pIndexData->vnum == OBJ_VNUM_SLICED_ARM
		 || obj->pIndexData->vnum == OBJ_VNUM_SLICED_LEG)
	{
	    strcpy(message, "$p разлага$rся.");
	}
	else
	{
	    switch (obj->item_type)
	    {
	    default:
	        if (obj->pIndexData->vnum == OBJ_VNUM_CAMPFIRE)
	  	    strcpy(message, "Приятное тепло $p1 больше не греет тебя.");
	     	else if (obj->pIndexData->vnum == OBJ_VNUM_LIGHTNING)
	    	    	strcpy(message, "$p взрыва$rся.");
		     else
			if (obj->pIndexData->vnum == OBJ_VNUM_CLOUD)
			    strcpy(message, "$p рассеивается.");
			else
			    strcpy(message, "$p распада$rся в пыль.");
		break;
	    case ITEM_FOUNTAIN:
		strcpy(message, "$p высыха$r.");
		break;
	    case ITEM_CORPSE_NPC:
		strcpy(message, "$p распада$rся в пыль.");
		break;
	    case ITEM_CORPSE_PC:
		strcpy(message, "$p распада$rся в пыль.");
		break;
	    case ITEM_FOOD:
		strcpy(message, "$p разлага$rся.");
		break;
	    case ITEM_POTION:
		if (obj->pIndexData->vnum == OBJ_VNUM_POTION)
		    strcpy(message, "$p исчеза$r.");
		else
		    strcpy(message, "$p испаря$rся.");
		break;
	    case ITEM_MORTAR:
		strcpy(message, "$p распада$rся в пыль.");
		break;
	    case ITEM_PORTAL:
		strcpy(message, "$p исчеза$r.");
		break;
	    case ITEM_CONTAINER:
		if (CAN_WEAR(obj, ITEM_WEAR_FLOAT))
		    if (obj->contains)
		    {
			strcpy(message, "$p мерца$r и исчеза$r, "
					 "содержимое вываливается $x.");
		    }
		    else
		    {
			strcpy(message, "$p мерца$r и исчеза$r.");
		    }
		else
		    strcpy(message, "$p распада$rся в пыль.");
		break;
	    }
	}


	if (obj->carried_by != NULL)
	{
	    if (IS_NPC(obj->carried_by)
		&& obj->carried_by->pIndexData->pShop != NULL)
	    {
		obj->carried_by->silver += obj->cost/5;
	    }
	    else
	    {
	    	act(message, obj->carried_by, obj, NULL, TO_CHAR);
		if (obj->wear_loc == WEAR_FLOAT)
		    act(message, obj->carried_by, obj, NULL, TO_ROOM);
	    }
	}
	else if (obj->in_room != NULL
		&& (rch = LIST_FIRST(&obj->in_room->people)) != NULL)
	{
	    if (!(obj->in_obj && obj->in_obj->pIndexData->vnum == OBJ_VNUM_PIT
	           && !CAN_WEAR(obj->in_obj, ITEM_TAKE)))
	    {
	    	act_new(message, rch, obj, NULL, TO_ALL, 
			obj->pIndexData->vnum == OBJ_VNUM_CAMPFIRE ? POS_SLEEPING : POS_RESTING);
	    }
	}
    
	for (t_obj = obj->in_obj; t_obj != NULL; t_obj = next_obj)
        {
	    next_obj = t_obj->in_obj;

	    if (t_obj->item_type == ITEM_CHEST && t_obj->in_room != NULL)
		id = t_obj->id;	    
	}

        if ((obj->item_type == ITEM_CORPSE_NPC
	     || obj->item_type == ITEM_CHEST
	     || obj->item_type == ITEM_CONTAINER
	     || obj->item_type == ITEM_MORTAR
	     || obj->item_type == ITEM_CORPSE_PC
	     || obj->wear_loc == WEAR_FLOAT)
	    && obj->contains)
	{   /* save the contents */
	    for (t_obj = obj->contains; t_obj != NULL; t_obj = next_obj)
	    {
		next_obj = t_obj->next_content;

		obj_from_obj(t_obj);

		if (obj->in_obj) /* in another object */
		    obj_to_obj(t_obj, obj->in_obj);
		else if (obj->carried_by)  /* carried */
		{
		    if (obj->wear_loc == WEAR_FLOAT || !can_loot(obj->carried_by, t_obj))
			if (obj->carried_by->in_room == NULL)
			{
				extract_obj(t_obj, TRUE, TRUE);
			}
			else
			    obj_to_room(t_obj, obj->carried_by->in_room);
		    else
		    	obj_to_char(t_obj, obj->carried_by);
		}
		else if (obj->in_room == NULL)  /* destroy it */
			{
				extract_obj(t_obj, TRUE, TRUE);
			}
		else /* to a room */
		    obj_to_room(t_obj, obj->in_room);
	    }
	}

	if (!IS_VALID(obj))
	    return;

	if ((obj->item_type == ITEM_CONTAINER
		|| obj->item_type == ITEM_PORTAL)
	    && obj->trap != NULL)
	{
	    extract_obj(obj->trap, TRUE, TRUE);
	}

	if (obj->item_type == ITEM_TRAP)
	{
	    if (obj->in_obj
		&& obj->in_obj->trap == obj)
	    {
	     	obj->in_obj->trap = NULL;
		obj->in_obj->trap_flags = 0;
		obj->in_obj = NULL;
	    }
	    else if (obj->in_room)
	    {
		int t;

		for (t = 0; t < 6; t++)
		    if (obj->in_room->exit[t] && obj->in_room->exit[t]->trap == obj)
		    {
			obj->in_room->exit[t]->trap = NULL;
			obj->in_room->exit[t]->trap_flags = 0;
			obj->in_room = NULL;
			break;
		    }
	    }
	}

	extract_obj(obj, TRUE, TRUE);
	if (id > 0)
	    save_chest(id);
    }

    return;
}



/*
 * Aggress.
 *
 * for each mortal PC
 *     for each mob in room
 *         aggress on some random PC
 *
 * This function takes 25% to 35% of ALL Merc cpu time.
 * Unfortunately, checking on each PC move is too tricky,
 *   because we don't the mob to just attack the first PC
 *   who leads the party into the room.
 *
 * -- Furey
 */
void aggr_update(void)
{
    CHAR_DATA *wch;
    CHAR_DATA *wch_next;
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    CHAR_DATA *victim;

    LIST_FOREACH_SAFE(wch, &char_list, link, wch_next)
    {
	if (IS_NPC(wch)
/*	    || wch->level >= LEVEL_IMMORTAL */
	    || wch->in_room == NULL
	    || wch->in_room->area->empty)
	{
	    continue;
	}

	LIST_FOREACH_SAFE(ch, &wch->in_room->people, room_link, ch_next)
	{
	    int count;

	    if (IS_NPC(ch)
		&& !is_animal(ch)
		&& number_bits(10) == 0
		&& can_see(ch, wch)
		&& IS_AWAKE(ch))
	    {
  		OBJ_DATA *obj;

  		for (obj = wch->carrying;obj; obj = obj->next_content)
		    if (obj->wear_loc != WEAR_NONE && can_see_obj(ch, obj))
		    {
      			char buf[MAX_STRING_LENGTH];

	    		if (IS_SET(obj->uncomf, FOR_WOMAN)
			    && wch->sex == SEX_FEMALE)
			{
			    switch (number_bits(2))
			    {
			    default:
				sprintf(buf, "%s, сними скорее %s, не "
					"позорься, ты же женщина!",
					PERS(wch, ch, 0),
					PERS_OBJ(ch, obj, 6));
				break;
			    case 2:
				sprintf(buf, "Гмм... То ли женщина, то ли "
					"мужчина... %s, ты чего на себя "
					"напялила?", PERS(wch, ch, 0));
				break;
			    case 3:
				sprintf(buf, "%s, с чего ты взяла, что тебе "
					"идет мужская одежда?",
					PERS(wch, ch, 0));
				break;
			    }
			    do_say(ch, buf);
			}

			if (IS_SET(obj->uncomf, FOR_MAN)
			    && wch->sex == SEX_MALE)
			{
			    switch (number_bits(2))
			    {
			    default:
				sprintf(buf, "%s, сними скорее %s, не "
					"позорься, ты же мужчина!",
					PERS(wch, ch, 0),
					PERS_OBJ(ch, obj, 6));
				break;
			    case 2:
				sprintf(buf, "Нет, ну вы посмотрите на %s - "
					"оделся как баба!", PERS(wch, ch, 3));
				break;
			    case 3:
				sprintf(buf, "Эй, %s, ты чего оделся как "
					"женщина?", PERS(wch, ch, 0));
				break;
			}

			do_say(ch, buf);
		    }

		    break;
		}
	    }

	    if (!IS_NPC(ch)
		|| ch->in_room == NULL
		|| IS_SET(ch->in_room->room_flags, ROOM_SAFE)
		|| IS_AFFECTED(ch, AFF_CALM)
		|| ch->fighting != NULL
		|| IS_AFFECTED(ch, AFF_CHARM)
		|| !IS_AWAKE(ch)
		|| (IS_SET(ch->act, ACT_WIMPY) && IS_AWAKE(wch))
		|| !can_see(ch, wch)
		|| number_bits(1) == 0)
	    {
		continue;
	    }


	    if (!IS_SET(ch->act, ACT_NOREMEMBER))
	    {
		  MEM_DATA *mem;
		  bool found = FALSE;

		  for (mem = ch->memory; mem; mem = mem->next)
		  {
	    	    if (is_memory(ch, wch, MEM_HOSTILE)
			&& can_see(ch, wch)
			&& SUPPRESS_OUTPUT(!is_safe(ch, wch))
			&& !ch->fighting
			&& wch->fighting != ch)
	    	    {
	    		char buf[MAX_STRING_LENGTH];

	    		if (!is_animal(ch))
	    		{
			    sprintf(buf, "Наконец-то ты вернул%s!!! "
			    	    "Сейчас ты за все ответишь!",
			    	    wch->sex == SEX_MALE ? "ся"
			    		: (wch->sex == SEX_FEMALE ? "ась" : "ось"));
			    do_yell(ch, buf);
	    		}

	    		
	    		if (IS_SET(ch->off_flags, OFF_BACKSTAB) 
	    		        && SUPPRESS_OUTPUT(can_backstab(ch, wch)))
	    		{
	    		    do_backstab(ch, wch->name);
			}
	    		else if (IS_SET(ch->off_flags, OFF_CLEAVE)
				&& can_cleave(ch, wch))
			{
			    do_cleave(ch, wch->name);
			}
			else
	    		    do_kill(ch, wch->name);

	    		found = TRUE;
	    		break;
	    	    }
	    	  }
	    	  
	    	  if (found)
	    	    continue;
	    }
	    /*
	     * Ok we have a 'wch' player character and a 'ch' npc aggressor.
	     * Now make the aggressor fight a RANDOM pc victim in the room,
	     *   giving each 'vch' an equal chance of selection.
	     */
	    count	= 0;
	    victim	= NULL;
	    LIST_FOREACH_SAFE(vch, &wch->in_room->people, room_link, vch_next)
	    {
		if (get_master(vch) != vch)
		    continue;

		if (!IS_NPC(vch) && is_wear_aggravate(vch) && SUPPRESS_OUTPUT(!is_safe(vch, ch)))
		{
		    victim = vch;
		    break;
		}

		if ((!IS_NPC(vch) || IS_SWITCHED(vch))
		    && vch->level < LEVEL_IMMORTAL
		    && ch->level >= vch->level - 5
		    && (!IS_SET(ch->act, ACT_WIMPY) || !IS_AWAKE(vch))
		    && IS_SET(ch->act, ACT_AGGRESSIVE)
		    && can_see(ch, vch)
		    && (!IS_AFFECTED(vch, AFF_EVIL_AURA) || !IS_EVIL(ch)))
		{
		    if (number_range(0, count) == 0)
			victim = vch;
		    count++;
		}
	    }

	    if (!IS_VALID(victim))
	    {
		CHAR_DATA *mount;

		if ((mount = MOUNTED(wch)) != NULL
		    && number_bits(4) == 0
		    && !mount_success(wch, mount, TRUE, FALSE))
		{
		    send_to_char("Твоя лошадь выходит из под твоего контроля!\n\r", wch);
		}
	    }
	    else
		multi_hit(ch, victim, TYPE_UNDEFINED);
	}
    }

    return;
}



bool is_reboot_message(int t)
{
    int i;

    for (i = 0; reboot_table[i].tick != 0; i++)
	if (reboot_table[i].tick == t)
	    return TRUE;

    return FALSE;
}

void tick_update()
{
    char buf[MAX_STRING_LENGTH];

//    pulse_point = number_range(2 * PULSE_TICK / 3, 4 * PULSE_TICK / 3);
    pulse_point = PULSE_TICK;

    weather_update(TRUE);
    char_update();
    obj_update();
    room_update();
    quest_update(NULL);
    auction_update();
    save_popularity();

    if (reboot_time > 0)
    {
	if (--reboot_time == 0)
	    do_function(who_claim_reboot, &do_reboot, "");
	else
	{
	    if (is_reboot_message(reboot_time))
	    {
		sprintf(buf, "{*{gДо запланированной перезагрузки "
			"осталось: %d %s.{x",
			reboot_time, hours(reboot_time, TYPE_MINUTES));
		do_function(who_claim_reboot, &do_echo, buf);
	    }
	}
    }

    if (immquest.is_quest)
    {
	DESCRIPTOR_DATA *d;
	char buf1[MAX_STRING_LENGTH/2];
	bool cq = FALSE;

	snprintf(buf, sizeof(buf), "{GВнимание, %s объявил%s квест! %s{g\n\r",
		immquest.who_claim->name,
		SEX_ENDING(immquest.who_claim),
		immquest.desc);

	if (immquest.timer > 0)
	{
	    if (--immquest.timer == 0)
	    {
    		snprintf(buf, sizeof(buf),
			    "{gВремя квеста '%s', к сожалению, вышло.{x\n\r",
			immquest.desc);
		cq = TRUE;
	    }
    	    else
	    {
		snprintf(buf1, sizeof(buf1), "До конца квеста осталось: %d %s.\n\r",
			immquest.timer, hours(immquest.timer, TYPE_HOURS));
		strcat(buf, buf1);
	    }
	}

     	if (immquest.max_score > 0)
 	{
	    int i;
	    char buf2[MAX_STRING_LENGTH];

	    buf2[0] = '\0';

	    for (i = 0; i < MAX_IN_QUEST; i++)
	    {
		if (immquest.player[i].name[0] != '\0')
		{
		    snprintf(buf1, sizeof(buf1), "%-12s %d\n\r",
			    immquest.player[i].name,
			    immquest.player[i].score);
		    strlcat(buf2, buf1, sizeof(buf2));
		}
	    }
	    if (i > 0)
	    {
    		strlcat(buf, "Текущий счет:\n\r", sizeof(buf));
		strlcat(buf, buf2, sizeof(buf));
     	    }
	}
	strlcat(buf, "{x", sizeof(buf));
	SLIST_FOREACH(d, &descriptor_list, link)
	    if (d->connected == CON_PLAYING
		&& !IS_SET(d->character->act, PLR_NOIMMQUEST))
	    {
		send_to_char(buf, d->character);
	    }

	if (cq)
	    close_quest();
    }
}

/*
 * Handle all kinds of updates.
 * Called once per pulse from game loop.
 * Random times to defeat tick-timing clients and players.
 */
void update_handler(void)
{
    char buf[MAX_STRING_LENGTH];

    if (--pulse_area <= 0)
    {
	pulse_area = PULSE_AREA;
	/* number_range(PULSE_AREA / 2, 3 * PULSE_AREA / 2); */
	area_update();
    }

    if (--pulse_music <= 0)
    {
	pulse_music = PULSE_MUSIC;
	song_update();
    }

    if (--pulse_violence <= 0)
    {
	pulse_violence = PULSE_VIOLENCE;
	violence_update();
    }

    if (--pulse_point <= 0)
    {
	wiznet("TICK!", NULL, NULL, WIZ_TICKS, 0, 0);
	tick_update();
    }

    if (reboot_time == 1
	&& is_reboot_message(pulse_point/PULSE_PER_SECOND)
	&& (int)(pulse_point/PULSE_PER_SECOND)*PULSE_PER_SECOND == pulse_point)
    {
        sprintf(buf, "{gДо запланированной перезагрузки осталось %d сек.{x",
		pulse_point/PULSE_PER_SECOND);
    	do_function(who_claim_reboot, &do_echo,  buf);
    }

    aggr_update();
    tail_chain();
    return;
}

void auction_update()
{
    int i;
    DESCRIPTOR_DATA *d;
    CHAR_DATA *ch;
    char buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH];

    for (i = 0; i < MAX_AUCTION; i++)
    {
	if (auctions[i].valid)
	{
	    if (++auctions[i].time > 3)
	    {
	    	if (auctions[i].buyer == NULL)
		{
		    sprintf(buf, "{aАукцион #%d: предмет '%%s{a' снят с "
				 "торгов по причине отсутствия спроса.{x\n\r",
			    i + 1);
		    auctions[i].valid = FALSE;
		    /* obj_to_char(auctions[i].obj, auctions[i].sender); */
		    REMOVE_BIT(auctions[i].obj->extra_flags, ITEM_AUCTION);
		}
		else
		{
		    CHAR_DATA *ch = auctions[i].buyer;
		    OBJ_DATA *obj = auctions[i].obj;

		    sprintf(buf, "{aАукцион #%d: предмет '%%s{a' продан с "
				 "торгов.{x\n\r",
			    i + 1);

                    /* 5% - богам на пиво.  8)) */
	    	    auctions[i].sender->gold += auctions[i].cost * 20 / 21;
		    obj_from_char(obj, TRUE);

		    if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch)
			|| !can_take_weight(ch, get_obj_weight(obj)))
                    {
			act("Ты не можешь нести так много вещей, ищи $P6 рядом с собой.",
			    ch, NULL, obj, TO_CHAR);
			obj_to_room(obj, ch->in_room);
                    }
                    else
			obj_to_char(obj, ch);

		    auctions[i].valid = FALSE;
		    REMOVE_BIT(auctions[i].obj->extra_flags, ITEM_AUCTION);
		}
	    }
	    else
	    {
                sprintf(buf, "{aАукцион #%d: предмет '%%s{a' (уровень %d), "
			     "ставка %d золота - ",
			i + 1, auctions[i].obj->level, auctions[i].cost);
	    	switch (auctions[i].time)
		{
	    	case 1:
    		    strcat(buf, "ОДИН!");
		    break;
		case 2:
		    strcat(buf, "ДВА!!");
		    break;
		case 3:
		    strcat(buf, "ТРИ!!!");
		    break;
		default:
		    break;
		}
		strcat(buf, "{x\n\r");
	    }

	    SLIST_FOREACH(d, &descriptor_list, link)
	    {
	    	ch = CH(d);

		if ((d->connected == CON_PLAYING)
		    && d->character
		    && d->character->position > POS_SLEEPING
		    && ch
		    && !IS_SET(ch->comm, COMM_NOAUCTION)
		    && !IS_SET(ch->comm, COMM_QUIET)
		    && ch->in_room && !IS_SET(ch->in_room->room_flags, ROOM_NOAUCTION))
		{
		    sprintf(buf1, buf, PERS_OBJ(ch, auctions[i].obj, 0));
		    send_to_char(buf1, ch);
		}
	    }
	}
    }
}
/* charset=cp1251 */





