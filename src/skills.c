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
*  ROM 2.4 is copyright 1993-1998 Russ Taylor                              *
*  ROM has been brought to you by the ROM consortium                       *
*      Russ Taylor (rtaylor@hypercube.org)                                 *
*      Gabrielle Taylor (gtaylor@hypercube.org)                            *
*      Brian Moore (zump@rom.org)                                          *
*  By using this code, you have agreed to follow the terms of the          *
*  ROM license, in the file Rom24/doc/rom.license                          *
***************************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"
#include "interp.h"
#include "magic.h"
#include "recycle.h"


typedef struct prac_list PRAC_LIST;

struct prac_list
{
    char *rname;
    int  learned;
};

PRAC_LIST *prac_table;

bool check_depend(CHAR_DATA *ch, int sn)
{
    int i;
    char buf[MSL];

    for (i = 0; !IS_NULLSTR(skill_table[sn].depends[i]); i++)
    {
	int sn_d = skill_lookup(skill_table[sn].depends[i]);

	if (sn_d == -1 || (ch->gen_data && ch->gen_data->skill_chosen[sn_d]))
	    continue;

	if (ch->pcdata->learned[sn_d] < 1)
	{
	    sprintf(buf, "Для этого умения необходимо иметь также %s.\n\r", get_skill_name(ch, sn_d, TRUE));
	    send_to_char(buf, ch);
	    return TRUE;
	}
    }

    return FALSE;
}

char *skill_spell(int sn, bool style)
{
    if (skill_table[sn].spell_fun != spell_null)
	return style ? "знании заклинания" : "заклинание";
    else
	return style ? "умении" : "умение";
}


char *get_skill_name(CHAR_DATA *ch, int sn, bool is_skill)
{
    if (IS_TRANSLIT(ch)
	|| IS_NULLSTR(is_skill ? skill_table[sn].rname : group_table[sn].rname))
    {
	return is_skill ? skill_table[sn].name : group_table[sn].name;
    }
    else
	return is_skill ? skill_table[sn].rname : group_table[sn].rname;
}

/* used to get new skills */
void do_gain(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *trainer;
    int gn = 0, sn = 0;
    double fact = 0;

    if (IS_NPC(ch))
	return;

    /* find a trainer */
    LIST_FOREACH(trainer, &ch->in_room->people, room_link)
	if (IS_NPC(trainer) && IS_SET(trainer->act, ACT_GAIN))
	    break;

    if (trainer == NULL || !can_see(ch, trainer))
    {
	send_to_char("Ты не можешь делать этого здесь.\n\r", ch);
	return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	do_function(trainer, &do_say, "Я тебя не понимаю...");
	return;
    }

    if (!str_prefix(arg, "list") || !str_prefix(arg, "список"))
    {
	int col;

	col = 0;

	sprintf(buf, "%-20s %-5s %-20s %-5s %-20s %-5s\n\r",
		"группа", "цена", "группа", "цена", "группа", "цена");
	send_to_char(buf, ch);

	for (gn = 0; gn < max_groups; gn++)
	{
	    if (group_table[gn].name == NULL)
		break;

	    if (!ch->pcdata->group_known[gn]
		&& group_table[gn].rating[ch->classid] > 0)
	    {
		sprintf(buf, "%-20s %-5d ", get_skill_name(ch, gn, FALSE),
			group_table[gn].rating[ch->classid]);
		send_to_char(buf, ch);
		if (++col % 3 == 0)
		    send_to_char("\n\r", ch);
	    }
	}

	if (col % 3 != 0)
	    send_to_char("\n\r", ch);

	send_to_char("\n\r", ch);

	col = 0;

	sprintf(buf, "%-20s %-5s %-20s %-5s %-20s %-5s\n\r",
		"умение", "цена", "умение", "цена", "умение", "цена");
	send_to_char(buf, ch);

	for (sn = 0; sn < max_skills; sn++)
	{
	    if (skill_table[sn].name == NULL)
		break;

	    if (ch->pcdata->learned[sn] <= 0
		&& skill_table[sn].rating[ch->classid] > 0
		&& !skill_table[sn].quest[ch->classid]
		&& skill_table[sn].spell_fun == spell_null
		&& sn != gsn_recall)
	    {
		sprintf(buf, "%-20s %-5d ", get_skill_name(ch, sn, TRUE),
			skill_table[sn].rating[ch->classid]);
		send_to_char(buf, ch);
		if (++col % 3 == 0)
		    send_to_char("\n\r", ch);
	    }
	}

	if (col % 3 != 0)
	    send_to_char("\n\r", ch);
	return;
    }

    if (!str_prefix(arg, "convert") || !str_prefix(arg, "превратить"))
    {
	if (ch->practice < 10)
	{
	    act("$N говорит тебе: {RТы еще не готов$t для этого.{x",
		ch, SEX_ENDING(ch), trainer, TO_CHAR);
	    return;
	}

	act("$N помогает тебе превратить твои практики в тренировки.",
	    ch, NULL, trainer, TO_CHAR);
	ch->practice -= 10;
	ch->train +=1 ;
	return;
    }

    if (!str_prefix(arg, "practice") || !str_prefix(arg, "практики"))
    {
	if (ch->train < 1)
	{
	    act("$N говорит тебе: {RТы еще не готов$t для этого.{x", ch, SEX_ENDING(ch), trainer, TO_CHAR);
	    return;
	}

	act("$N помогает тебе превратить твою тренировку в практики.", ch, NULL, trainer, TO_CHAR);
	ch->train -= 1;
	ch->practice += 10;
	return;
    }

    if (!str_prefix(arg, "hit") || !str_prefix(arg, "жизнь"))
    {
	if (ch->max_hit < 50 || ch->pcdata->perm_hit < 50 || ch->hit < 30)
	{
	    act("$N говорит тебе: {RТы еще не готов$t для этого.{x", ch, SEX_ENDING(ch), trainer, TO_CHAR);
	    return;
	}

	ch->pcdata->perm_hit -= 15;
	ch->max_hit -= 15;
	ch->hit = UMIN(ch->hit, ch->max_hit);
	ch->train += 1;

	act("$N помогает тебе превратить твою жизненную силу в тренировку.", ch, NULL, trainer, TO_CHAR);
	return;
    }

    if (!str_prefix(arg, "points") || !str_prefix(arg, "пункты"))
    {
	if (ch->train < 2)
	{
	    act("$N говорит тебе: {RТы еще не готов$t.{x",
		ch, SEX_ENDING(ch), trainer, TO_CHAR);
	    return;
    }

	if (ch->pcdata->points <= 40)
	{
	    act("$N говорит тебе: {RВ этом нет смысла.", ch, NULL, trainer, TO_CHAR);
	    return;
	}

	act("$N тренирует тебя, и ты чувствуешь себя более умелым.",
	    ch, NULL, trainer, TO_CHAR);

	ch->train -= 2;
	ch->pcdata->points -= 1;

	fact = (double)exp_per_level(ch, ch->pcdata->points) / (double)exp_per_level(ch, ch->pcdata->points + 1);
	bugf("####### %f #########", fact);

	ch->exp = (double)ch->exp * fact;
	return;
    }
    
/*    if (!str_prefix(arg, "продать"))
    {
        
        int i;
        char arg2[MAX_INPUT_LENGTH];

        if (IS_SET(ch->act, PLR_NEWYEAR_2005))
        {
            send_to_char("Ты уже имеешь свой новогодний подарок!\n\r", ch);
            return;
        }
                                                       
        argument = one_argument(argument, arg);
        one_argument(argument, arg2);
        
        sn = skill_lookup(arg2);

        if (sn < 0)
        {        
            act("$N говорит тебе: {RТакого умения нет!{x",
                  ch, NULL, trainer, TO_CHAR);

            return;
        }
        
        if (ch->pcdata->learned[sn] <= 0)
        {
            act("$N говорит тебе: {RТы не знаешь такого умения!{x", ch, NULL, trainer, TO_CHAR);
            return;
        }
    
	for (i = 0; i < 5; i++)
	    if (skill_lookup(pc_race_table[ch->race].skills[i]) == sn)
            {                    
                send_to_char("Ты не можешь продать свое врожденное умение!\n\r", ch);
                return;
            }

        ch->pcdata->learned[sn] = -100;
        
        SET_BIT(ch->act, PLR_NEWYEAR_2005);
        send_to_char("{YТы получаешь свой новогодний подарок. С Новым Годом!{x\n\r", ch);
        act("$N забирает у тебя умение '$t'.", ch, get_skill_name(ch, sn, TRUE), trainer, TO_CHAR);
//новогодний флаг
  
        ch->train += skill_table[sn].rating[ch->class];
        return;
    } */

    /* else add a group/skill */

    gn = group_lookup(argument);
    if (gn > 0)
    {
	if (ch->pcdata->group_known[gn])
	{
	    act("$N говорит тебе: {RТы уже знаешь это!{x",
		ch, NULL, trainer, TO_CHAR);
	    return;
	}

	if (group_table[gn].rating[ch->classid] <= 0)
	{
	    act("$N говорит: {RЭта группа умений тебе недоступна.{x",
		ch, NULL, trainer, TO_CHAR);
	    return;
	}

	if (ch->train < group_table[gn].rating[ch->classid])
	{
	    act("$N говорит тебе: {RТы еще не готов$t для этой группы умений.{x",  ch, SEX_ENDING(ch), trainer, TO_CHAR);
	    return;
	}

	/* add the group */
	gn_add(ch, gn);
	act("$N дает тебе начальные уроки в группе умений '$t'.", ch, get_skill_name(ch, gn, FALSE), trainer, TO_CHAR);
	ch->train -= group_table[gn].rating[ch->classid];
	return;
    }

    sn = skill_lookup(argument);
    if (sn > -1)
    {
	if (skill_table[sn].spell_fun != spell_null)
	{
	    act("$N говорит тебе: {RТебе надо выучить всю группу.{x",
		ch, NULL, trainer, TO_CHAR);
	    return;
	}


	if (ch->pcdata->learned[sn] > 0)
	{
	    act("$N говорит тебе: {RТы уже знаешь это умение!{x",
		ch, NULL, trainer, TO_CHAR);
	    return;
	}

	if (skill_table[sn].rating[ch->classid] <= 0)
	{
	    act("$N говорит тебе: {RЭто умение тебе недоступно.{x",
		ch, NULL, trainer, TO_CHAR);
	    return;
	}

	if (skill_table[sn].quest[ch->classid])
	{
	    act("$N говорит тебе: {RК сожалению, я не могу научить тебя этому "
		"умению. Попробуй поискать учителя получше.{x",
		ch, NULL, trainer, TO_CHAR);
	    return;
	}

	if (ch->train < skill_table[sn].rating[ch->classid] /* && IS_SET(ch->act, PLR_NEWYEAR) */)
	{
	    act("$N говорит тебе: {RТы еще не готов$t для этого умения.{x", ch, SEX_ENDING(ch), trainer, TO_CHAR);
	    return;
	}

	if (sn == gsn_recall)
	{
	    send_to_char("Это бессмысленно.\n\r", ch);
	    return;
	}

	if (check_depend(ch, sn))
	    return;

	/* add the skill */
	ch->pcdata->learned[sn] = 1;
	act("$N дает тебе начальные уроки в умении '$t'.", ch, get_skill_name(ch, sn, TRUE), trainer, TO_CHAR);
    ch->train -= skill_table[sn].rating[ch->classid];
	return;
    }

    act("$N говорит тебе: {RЯ тебя не понимаю...{x", ch, NULL, trainer, TO_CHAR);
}



/* used to get new skills of exp */
void do_buy_add(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *trainer;
    int gn = 0, sn = 0;
//    double fact = 0;

    if (IS_NPC(ch))
		return;

    /* find a trainer */
    LIST_FOREACH(trainer, &ch->in_room->people, room_link)
	if (IS_NPC(trainer) && IS_SET(trainer->act, ACT_GAIN))
	    break;

    if (trainer == NULL || !can_see(ch, trainer))
    {
		send_to_char("У кого докупать желаем-то?\n\r", ch);
		return;
    }

	if (ch->level < 51)
	{
		act("$N говорит тебе: {RТы еще слишком мал$t.{x", ch, SEX_ENDING(ch), trainer, TO_CHAR);	
		return;
	}

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
		do_function(trainer, &do_say, "Чего докупить желаем?");
		return;
    }

    if (!str_prefix(arg, "list") || !str_prefix(arg, "список"))
    {
		int col;

		col = 0;

		sprintf(buf, "%-20s %-5s %-20s %-5s %-20s %-5s\n\r", "группа", "цена", "группа", "цена", "группа", "цена");
		send_to_char(buf, ch);

		for (gn = 0; gn < max_groups; gn++)
		{
	    	if (group_table[gn].name == NULL)
				break;

	    	if (!ch->pcdata->group_known[gn] && group_table[gn].rating[ch->classid] > 0)
	    	{
				sprintf(buf, "%-20s %-5d ", get_skill_name(ch, gn, FALSE), group_table[gn].rating[ch->classid]);
				send_to_char(buf, ch);
				if (++col % 3 == 0)
		    		send_to_char("\n\r", ch);
	    	}
		}

		if (col % 3 != 0)
	    	send_to_char("\n\r", ch);

		send_to_char("\n\r", ch);

		col = 0;

		sprintf(buf, "%-20s %-5s %-20s %-5s %-20s %-5s\n\r", "умение", "цена", "умение", "цена", "умение", "цена");
		send_to_char(buf, ch);

		for (sn = 0; sn < max_skills; sn++)
		{
	    	if (skill_table[sn].name == NULL)
				break;

	    	if (ch->pcdata->learned[sn] <= 0
				&& skill_table[sn].rating[ch->classid] > 0
				&& !skill_table[sn].quest[ch->classid]
				&& skill_table[sn].spell_fun == spell_null
				&& sn != gsn_recall)
	    	{
				sprintf(buf, "%-20s %-5d ", get_skill_name(ch, sn, TRUE),
					skill_table[sn].rating[ch->classid]);
				send_to_char(buf, ch);
				if (++col % 3 == 0)
		    		send_to_char("\n\r", ch);
	    	}
		}

		if (col % 3 != 0)
		    send_to_char("\n\r", ch);
		return;
    }

    if (!str_prefix(arg, "pg") || !str_prefix(arg, "пункты"))
    {
		one_argument(argument, arg);
		
	    if (!is_number(arg))
	    {
			send_to_char("Количество пунктов генерации должно быть числовым.\n\r", ch);
			return;
	    }

		if (atoi(arg)>250 || atoi(arg)<1)
		{
			send_to_char("Это нереальное число.\n\r", ch);
			return;
		}

		sprintf(buf, "На %d добавляемых ПГ кол-во опыта на уровень будет %d exp, суммарное кол-во опыта необходимо будет иметь не менее %d exp.\n\r", 
				atoi(arg), exp_per_level(ch, (1 + (ch->pcdata->points + (int)(1.25*atoi(arg))))), 51*exp_per_level(ch, ( 1 + (ch->pcdata->points + (int)(1.25*atoi(arg)))) ));
		send_to_char(buf, ch);

		return;
	}
		

	if (ch->level > 51)
	{
		send_to_char("Тебе то это зачем?:-)\n\r", ch);
		return;
	}

//	fact = (double)exp_per_level(ch, ch->pcdata->points) / (double)exp_per_level(ch, ch->pcdata->points + 1);
//	bugf("####### %f #########", fact);

//	ch->exp = (double)ch->exp * fact;
//	return;
//   }

    gn = group_lookup(arg);
    if (gn > 0)
    {
		if (ch->pcdata->group_known[gn])
		{
	    	act("$N говорит тебе: {RТы уже знаешь это!{x", ch, NULL, trainer, TO_CHAR);
	    	return;
		}

		if (group_table[gn].rating[ch->classid] <= 0)
		{
		    act("$N говорит: {RЭта группа умений тебе недоступна.{x", ch, NULL, trainer, TO_CHAR);
		    return;
		}

		if (ch->exp < 51*exp_per_level(ch, (1 + (ch->pcdata->points + (int)(1.25*group_table[gn].rating[ch->classid])))))
		{
		    act("$N говорит тебе: {RУ тебя недостаточно опыта для данной покупки.{x",  ch, SEX_ENDING(ch), trainer, TO_CHAR);
		    return;
		}

		/* add the group */
		gn_add(ch, gn);
		act("$N дает тебе начальные уроки в группе умений '$t'.", ch, get_skill_name(ch, gn, FALSE), trainer, TO_CHAR);

		ch->pcdata->points += (1 + (int)(1.25*group_table[gn].rating[ch->classid]));
		return;
    }

    sn = skill_lookup(arg);
    if (sn > -1)
    {
		if (skill_table[sn].spell_fun != spell_null)
		{
		    act("$N говорит тебе: {RТебе надо выучить всю группу.{x", ch, NULL, trainer, TO_CHAR);
		    return;
		}

		if (ch->pcdata->learned[sn] > 0)
		{
		    act("$N говорит тебе: {RТы уже знаешь это умение!{x", ch, NULL, trainer, TO_CHAR);
		    return;
		}

		if (skill_table[sn].rating[ch->classid] <= 0)
		{
		    act("$N говорит тебе: {RЭто умение тебе недоступно.{x", ch, NULL, trainer, TO_CHAR);
		    return;
		}

		if (skill_table[sn].quest[ch->classid])
		{
		    act("$N говорит тебе: {RК сожалению, я не могу научить тебя этому умению. Попробуй поискать учителя получше.{x",
				ch, NULL, trainer, TO_CHAR);
		    return;
		}

		if (ch->exp < 51*exp_per_level(ch, (1 + (ch->pcdata->points + (int)(1.25*skill_table[sn].rating[ch->classid])))))
		{
		    act("$N говорит тебе: {RУ тебя недостаточно опыта для данной покупки.{x",  ch, SEX_ENDING(ch), trainer, TO_CHAR);
		    return;
		}

		if (sn == gsn_recall)
		{
		    send_to_char("Это бессмысленно.\n\r", ch);
		    return;
		}

		if (check_depend(ch, sn))
		    return;

		/* add the skill */
		ch->pcdata->learned[sn] = 1;
		act("$N дает тебе начальные уроки в умении '$t'.", ch, get_skill_name(ch, sn, TRUE), trainer, TO_CHAR);
		ch->pcdata->points += (1 + (int)(1.25*skill_table[sn].rating[ch->classid]));
		return;
    }

    act("$N говорит тебе: {RЯ тебя не понимаю...{x", ch, NULL, trainer, TO_CHAR);
}



void do_train(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *mob;
    int16_t stat = -1;
    char *pOutput = NULL;
    char *pOutput1 = NULL;
    char *pOutput2 = NULL;

    if (IS_NPC(ch))
	return;

    /*
     * Check for trainer.
     */
    LIST_FOREACH(mob, &ch->in_room->people, room_link)
	if (IS_NPC(mob) && IS_SET(mob->act, ACT_TRAIN))
	    break;

    if (mob == NULL || !can_see(ch, mob))
    {
	send_to_char("Ты не можешь тренироваться здесь.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	sprintf(buf, "У тебя %d %s.\n\r", ch->train, hours(ch->train, TYPE_TRAIN));
	send_to_char(buf, ch);
	argument = "foo";
    }

    if (!str_prefix(argument, "силу"))
    {
	stat        = STAT_STR;
	pOutput     = "сила";
	pOutput1    = "Твоя";
	pOutput2    = "ась";
    }

    else if (!str_prefix(argument, "ум"))
    {
	stat      = STAT_INT;
	pOutput     = "ум";
	pOutput1    = "Твой";
	pOutput2    = "ся";
    }

    else if (!str_prefix(argument, "мудрость"))
    {
	stat      = STAT_WIS;
	pOutput     = "мудрость";
	pOutput1    = "Твоя";
	pOutput2    = "ась";
    }

    else if (!str_prefix(argument, "ловкость"))
    {
	stat        = STAT_DEX;
	pOutput     = "ловкость";
	pOutput1    = "Твоя";
	pOutput2    = "ась";
    }

    else if (!str_prefix(argument, "сложение"))
    {
	stat      = STAT_CON;
	pOutput     = "сложение";
	pOutput1    = "Твое";
	pOutput2    = "ось";
    }
    else if (!str_prefix(argument, "жизнь"))
	;

    else if (!str_prefix(argument, "энергию"))
	;

    else
    {
	strcpy(buf, "Ты можешь тренировать:\n\r");
	if (ch->perm_stat[STAT_STR] < get_max_train(ch, STAT_STR))
	    strcat(buf, " силу");
	if (ch->perm_stat[STAT_INT] < get_max_train(ch, STAT_INT))
	    strcat(buf, " ум");
	if (ch->perm_stat[STAT_WIS] < get_max_train(ch, STAT_WIS))
	    strcat(buf, " мудрость");
	if (ch->perm_stat[STAT_DEX] < get_max_train(ch, STAT_DEX))
	    strcat(buf, " ловкость");
	if (ch->perm_stat[STAT_CON] < get_max_train(ch, STAT_CON))
	    strcat(buf, " сложение");
	strcat(buf, " жизнь энергию");

	if (buf[strlen(buf)-1] != ':')
	{
	    strcat(buf, ".\n\r");
	    send_to_char(buf, ch);
	}
	else
	{
	    /*
	     * This message dedicated to Jordan ... you big stud!
	     */
	    act("Тебе нечего больше тренировать, $T!",
		ch, NULL,
		ch->sex == SEX_MALE   ? "большой жеребец" :
		ch->sex == SEX_FEMALE ? "милашка" :
		"дикарь",
		TO_CHAR);
	}

	return;
    }

    if (ch->train < 1)
    {
	send_to_char("У тебя не хватает тренировок.\n\r", ch);
	return;
    }

    if (!str_prefix(argument, "жизнь"))
    {
	ch->train--;
	ch->pcdata->perm_hit += 10;
	ch->max_hit += 10;
	ch->hit +=10;
	ch->pcdata->train_hit += 10;
	act("Твоя жизненная стойкость выросла!", ch, NULL, NULL, TO_CHAR);
	act("Жизненная стойкость $n1 выросла!", ch, NULL, NULL, TO_ROOM);
	return;
    }

    if (!str_prefix(argument, "энергию"))
    {
	ch->train--;
	ch->pcdata->perm_mana += 10;
	ch->max_mana += 10;
	ch->mana += 10;
	ch->pcdata->train_mana += 10;
	act("Твоя энергия увеличилась!", ch, NULL, NULL, TO_CHAR);
	act("Энергия $n1 увеличилась!", ch, NULL, NULL, TO_ROOM);
	return;
    }

    if (ch->perm_stat[stat]  >= get_max_train(ch, stat))
    {
	sprintf(buf, "%s %s уже на максимуме.\n\r", pOutput1, pOutput);
	send_to_char(buf, ch);
	return;
    }

    ch->train--;

    ch->perm_stat[stat]  ++;
    sprintf(buf, "%s %s увеличил%s.", pOutput1, pOutput, pOutput2);
    act(buf, ch, NULL, pOutput, TO_CHAR);
    sprintf(buf, "%s $n1 увеличил%s.", pOutput, pOutput2);
    act(buf, ch, NULL, pOutput, TO_ROOM);
    return;
}

int compare_pract (const void *v1, const void *v2)
{
    PRAC_LIST a1 = *(PRAC_LIST*) v1;
    PRAC_LIST a2 = *(PRAC_LIST*) v2;
    int i;

    i = strcmp(a1.rname , a2.rname);

    if (i < 0)
	return -1;

    if (i > 0)
	return +1;
    else
	return 0;
}

void do_practice(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    int sn;

    if (IS_NPC(ch))
	return;

    if (!prac_table)
	prac_table = alloc_perm(sizeof(PRAC_LIST) * max_skills);

    if (argument[0] == '\0')
    {
	int col = 0, i = 0;

	for (sn = 0; sn < max_skills; sn++)
	{
	    if (skill_table[sn].name == NULL)
		break;

	    if (ch->level < skill_table[sn].skill_level[ch->classid]
		|| ch->pcdata->learned[sn] < 1 /* skill is not known */)
	    {
		continue;
	    }
	    prac_table[i].rname = get_skill_name(ch, sn, TRUE);
	    prac_table[i].learned = get_skill(ch, sn);
	    i++;
	}

	qsort(prac_table, i, sizeof(prac_table[0]), compare_pract);

	for (sn = 0; sn < i; sn++)
	{
	    if (prac_table[sn].rname[0] == '\0')
		continue;

	    sprintf(buf, "%-20s %3d%%  ",
		    prac_table[sn].rname, prac_table[sn].learned);
	    send_to_char(buf, ch);
	    if (++col % 3 == 0)
		send_to_char("\n\r", ch);
	}

	if (col % 3 != 0)
	    send_to_char("\n\r", ch);

	sprintf(buf, "У тебя есть %d %s.\n\r",
		ch->practice, hours(ch->practice, TYPE_PRACTICE));
	send_to_char(buf, ch);
    }
    else
    {
	CHAR_DATA *mob;
	int adept, rating = 1;

	if (!IS_AWAKE(ch))
	{
	    send_to_char("В твоих снах, или как?\n\r", ch);
	    return;
	}

	LIST_FOREACH(mob, &ch->in_room->people, room_link)
	    if (IS_NPC(mob) && IS_SET(mob->act, ACT_PRACTICE))
		break;

	if (mob == NULL || !can_see(ch, mob))
	{
	    send_to_char("Ты не можешь этого здесь.\n\r", ch);
	    return;
	}

	if (ch->practice <= 0)
	{
	    send_to_char("У тебя не осталось практик.\n\r", ch);
	    return;
	}

	if ((sn = find_spell(ch, argument)) < 0)
	{
	    send_to_char("Такого умения нет.\n\r", ch);
	    return;
	}


	if ( (ch->level < skill_table[sn].skill_level[ch->classid]
		|| ch->pcdata->learned[sn] < 1   /* skill is not known */
		|| (sn == gsn_recall && ch->level > MAX_RECALL_LEVEL)
		|| (rating = get_skill_rating(ch, sn)) <= 0))
	{
	    send_to_char("Ты не можешь практиковаться в этом.\n\r", ch);
	    return;
	}

	adept = class_table[ch->classid].skill_adept;

	if (ch->pcdata->learned[sn] >= adept)
	{
	    sprintf(buf, "Ты уже знаешь %s %s.\n\r",
		    skill_spell(sn, FALSE), get_skill_name(ch, sn, TRUE));
	    send_to_char(buf, ch);
	}
	else
	{
	    ch->practice--;
	    ch->pcdata->learned[sn] += int_app[get_curr_stat(ch, STAT_INT)].learn / rating;

	    if (ch->pcdata->learned[sn] < adept)
	    {
		sprintf(buf, "Ты практикуешься в %s %s, и твой уровень знаний достигает %d%%.\n\r",
			skill_spell(sn, TRUE), get_skill_name(ch, sn, TRUE), ch->pcdata->learned[sn]);
		send_to_char(buf, ch);

		act("$n практикует $t $T.",
		    ch, skill_spell(sn, FALSE), get_skill_name(ch, sn, TRUE), TO_ROOM);
	    }
	    else
	    {
		ch->pcdata->learned[sn] = adept;
		act("Ты теперь знаешь $t $T.",
		    ch, skill_spell(sn, FALSE), get_skill_name(ch, sn, TRUE), TO_CHAR);
		act("$n теперь знает $t $T.",
		    ch, skill_spell(sn, FALSE), get_skill_name(ch, sn, TRUE), TO_ROOM);
	    }
	}
    }
    return;
}


/* RT spells and skills show the players spells (or skills) */

void show_spells(CHAR_DATA *ch, CHAR_DATA *victim, char *argument)
{
    BUFFER *buffer;
    char arg[MAX_INPUT_LENGTH];
    char spell_list[LEVEL_HERO + 1][2 * MIL];
    char spell_columns[LEVEL_HERO + 1];
    int sn, level, min_lev = 1, max_lev = LEVEL_HERO, mana;
    bool fAll = FALSE, found = FALSE;
    char buf[MAX_STRING_LENGTH], bs = '>';
    int percent, pn = -1;

    if (IS_NPC(victim))
	return;

    if (argument[0] != '\0')
    {
	fAll = TRUE;

	if (str_prefix(argument, "all") && str_prefix(argument, "все"))
	{
	    argument = one_argument(argument, arg);
	        
	    if (arg[strlen(arg) - 1] == '%')
	    {
	        char newarg[MIL];

	        if (arg[0] != '>' && arg[0] != '<')
	        {
	            send_to_char("Необходимо указывать знаки > или < перед числом процентов.\n\r", ch);
	            return;
	        }

	        bs = arg[0];

	        strlcpy(newarg, arg + 1, strlen(arg) - 1);
                if (!is_number(newarg))
	        {
	            send_to_char("Аргумент должен быть числовым.\n\r", ch);
	            return;
	        }

	        pn = atoi(newarg);

	        if (pn < 1 || pn > 100)
	        {
	            send_to_char("Процент должен быть в пределах от 1 до 100.\n\r", ch);
	            return;
	        }
	    }
	    else
	    {
	        if (!is_number(arg))
	        {
		    send_to_char("Аргументы должны быть числовыми или 'все'.\n\r", ch);
		    return;
	        }
	        max_lev = atoi(arg);

	        if (max_lev < 1 || max_lev > LEVEL_HERO)
	        {
		    sprintf(buf, "Уровни должны быть между 1 и %d.\n\r", LEVEL_HERO);
		    send_to_char(buf, ch);
		    return;
	        }

	        if (argument[0] != '\0')
	        {
		    argument = one_argument(argument, arg);

		    if (!is_number(arg))
		    {
		        send_to_char("Аргументы должны быть числовыми или 'все'.\n\r", ch);
		        return;
		    }
		    min_lev = max_lev;
		    max_lev = atoi(arg);

		    if (max_lev < 1 || max_lev > LEVEL_HERO)
		    {
		        sprintf(buf, "Уровни должны быть между 1 и %d.\n\r", LEVEL_HERO);
		        send_to_char(buf, ch);
		        return;
		    }

		    if (min_lev > max_lev)
		    {
		        level = min_lev;
		        min_lev = max_lev;
		        max_lev = level;
		    }
	        }
	    }
	}
    }

    /* initialize data */
    for (level = 0; level < LEVEL_HERO + 1; level++)
    {
	spell_columns[level] = 0;
	spell_list[level][0] = '\0';
    }

    for (sn = 0; sn < max_skills; sn++)
    {
	if (skill_table[sn].name == NULL)
	    break;

	if ((level = skill_table[sn].skill_level[victim->classid]) <= LEVEL_HERO
	    &&  (fAll || level <= victim->level)
	    &&  level >= min_lev && level <= max_lev
	    &&  skill_table[sn].spell_fun != spell_null
	    &&  victim->pcdata->learned[sn] > 0
	    &&  (pn < 0 || (bs == '>' && get_skill(victim, sn) > pn) || (bs == '<' && get_skill(victim, sn) < pn)))
	{
	    found = TRUE;
	    level = skill_table[sn].skill_level[victim->classid];
	    if (victim->level < level)
		sprintf(buf, "%-20s пока нет         ", get_skill_name(ch, sn, TRUE));
	    else
	    {
		char c;
		char over[MIL];
		mana = UMAX(skill_table[sn].min_mana, 100/(2 + victim->level - level));

		percent = get_skill(victim, sn);
		if (percent >= 75 && percent < 100)
		    c = 'y';
		else if (percent == 100)
		    c = 'g';
		else if (percent >= 100)
		    c = 'G';
		else if (percent < 30)
		    c = 'r';
		else
		    c = 'x';

		if (IS_IMMORTAL(ch))
		    sprintf(over, "%7.3f%%", (float)victim->pcdata->over_skill[sn] / MAX_SKILL_OVER * 100);
		else
		    strcpy(over, "");

		sprintf(buf, "%-20s [{%c%3d%%{x] %s %3d маны  ", get_skill_name(ch, sn, TRUE), c, percent, over, mana);

	    }

	    if (IS_NULLSTR(spell_list[level]))
		sprintf(spell_list[level], "\n\rУровень %2d: %s", level, buf);
	    else /* append */
	    {
		if (++spell_columns[level] % 2 == 0)
		    strlcat(spell_list[level], "\n\r            ", 2 * MIL);
		strlcat(spell_list[level], buf, 2 * MIL);
	    }
	}
    }

    /* return results */

    if (!found)
    {
	send_to_char("Заклинаний не найдено.\n\r", ch);
	return;
    }

    buffer = new_buf();

    for (level = 0; level < LEVEL_HERO + 1; level++)
	if (!IS_NULLSTR(spell_list[level]))
	    add_buf(buffer, spell_list[level]);

    add_buf(buffer, "\n\r");

    page_to_char(buf_string(buffer), ch);
    free_buf(buffer);
}

void show_skills(CHAR_DATA *ch, CHAR_DATA *victim, char *argument)
{
    BUFFER *buffer;
    char arg[MAX_INPUT_LENGTH];
    char skill_list[LEVEL_HERO + 1][2 * MIL];
    char skill_columns[LEVEL_HERO + 1];
    int sn, level, min_lev = 1, max_lev = LEVEL_HERO;
    bool fAll = FALSE, found = FALSE;
    char buf[MAX_STRING_LENGTH];
    int percent;

    if (IS_NPC(victim))
	return;

    if (argument[0] != '\0')
    {
	fAll = TRUE;

	if (str_prefix(argument, "all") && str_prefix(argument, "все"))
	{
	    argument = one_argument(argument, arg);
	    if (!is_number(arg))
	    {
		send_to_char("Аргументы должны быть числовыми или 'все'.\n\r", ch);
		return;
	    }
	    max_lev = atoi(arg);

	    if (max_lev < 1 || max_lev > LEVEL_HERO)
	    {
		sprintf(buf, "Уровень должен быть между 1 и %d.\n\r", LEVEL_HERO);
		send_to_char(buf, ch);
		return;
	    }

	    if (argument[0] != '\0')
	    {
		argument = one_argument(argument, arg);
		if (!is_number(arg))
		{
		    send_to_char("Аргументы должны быть числовыми или 'все'.\n\r", ch);
		    return;
		}
		min_lev = max_lev;
		max_lev = atoi(arg);

		if (max_lev < 1 || max_lev > LEVEL_HERO)
		{
		    sprintf(buf,
			    "Уровень должен быть между 1 и %d.\n\r", LEVEL_HERO);
		    send_to_char(buf, ch);
		    return;
		}

		if (min_lev > max_lev)
		{
		    level = min_lev;
		    min_lev = max_lev;
		    max_lev = level;
		}
	    }
	}
    }


    /* initialize data */
    for (level = 0; level < LEVEL_HERO + 1; level++)
    {
	skill_columns[level] = 0;
	skill_list[level][0] = '\0';
    }

    for (sn = 0; sn < max_skills; sn++)
    {
	if (IS_NULLSTR(skill_table[sn].name))
	    break;

	if ((level = skill_table[sn].skill_level[victim->classid]) <= LEVEL_HERO
	    && (fAll || level <= victim->level)
	    && level >= min_lev && level <= max_lev
	    && skill_table[sn].spell_fun == spell_null
	    && victim->pcdata->learned[sn] > 0)
	{
	    char b[MIL];
	    char over[MIL];

	    found = TRUE;
	    level = skill_table[sn].skill_level[victim->classid];
	    if (victim->level < level)
		strcpy(b, "пока нет");
	    else
	    {
		percent = get_skill(victim, sn);
		if (percent >= 75 && percent < 100)
		    sprintf(b, "{y%3d%%{x", percent);
		else if (percent == 100)
		    sprintf(b, "{g%3d%%{x", percent);
		else if (percent >= 100)
		    sprintf(b, "{G%3d%%{x", percent);
		else if (percent < 30)
		    sprintf(b, "{r%3d%%{x", percent);
		else
		    sprintf(b, "%3d%%", percent);

		if (IS_IMMORTAL(ch))
		    sprintf(over, "%7.3f%%", (float)victim->pcdata->over_skill[sn] / MAX_SKILL_OVER * 100);
		else
		    strcpy(over, "");
	    }
	    
	    sprintf(buf, "%-20s %s %s     ", get_skill_name(ch, sn, TRUE), b, over);

	    if (IS_NULLSTR(skill_list[level]))
		sprintf(skill_list[level], "\n\rУровень %2d: %s", level, buf);
	    else /* append */
	    {
		if (++skill_columns[level] % 2 == 0)
		    strlcat(skill_list[level], "\n\r            ", 2 * MIL);
		strlcat(skill_list[level], buf, 2 * MIL);
	    }
	}
    }

    /* return results */

    if (!found)
    {
	send_to_char("Умений не найдено.\n\r", ch);
	return;
    }

    buffer = new_buf();
    for (level = 0; level < LEVEL_HERO + 1; level++)
	if (!IS_NULLSTR(skill_list[level]))
	    add_buf(buffer, skill_list[level]);
    add_buf(buffer, "\n\r");
    page_to_char(buf_string(buffer), ch);
    free_buf(buffer);
}

void do_skills(CHAR_DATA *ch, char *argument)
{
    show_skills(ch, ch, argument);
}

void do_spells(CHAR_DATA *ch, char *argument)
{
    show_spells(ch, ch, argument);
}

/* shows skills, groups and costs (only if not bought) */
void list_group_costs(CHAR_DATA *ch)
{
    char buf[100];
    int gn, sn, col;

    if (IS_NPC(ch))
	return;

    col = 0;

    sprintf(buf, "%-20s %-5s %-20s %-5s %-20s %-5s\n\r", "Группа", "Цена", "Группа", "Цена", "Группа", "Цена");
    send_to_char(buf, ch);

    for (gn = 0; gn < max_groups; gn++)
    {
	if (group_table[gn].name == NULL)
	    break;

	if (!ch->gen_data->group_chosen[gn]
	    && !ch->pcdata->group_known[gn]
	    && group_table[gn].rating[ch->classid] > 0)
	{
	    sprintf(buf, "%-20s %-5d ", get_skill_name(ch, gn, FALSE), group_table[gn].rating[ch->classid]);
	    send_to_char(buf, ch);
	    if (++col % 3 == 0)
		send_to_char("\n\r", ch);
	}
    }
    if (col % 3 != 0)
	send_to_char("\n\r", ch);
    send_to_char("\n\r", ch);

    col = 0;

    sprintf(buf, "%-20s %-5s %-20s %-5s %-20s %-5s\n\r",
	    "Умение", "Цена", "Умение", "Цена", "Умение", "Цена");
    send_to_char(buf, ch);

    for (sn = 0; sn < max_skills; sn++)
    {
	if (skill_table[sn].name == NULL)
	    break;

	if (!ch->gen_data->skill_chosen[sn]
	    && ch->pcdata->learned[sn] <= 0
	    && skill_table[sn].spell_fun == spell_null
	    && skill_table[sn].rating[ch->classid] > 0
	    && !skill_table[sn].quest[ch->classid])
	{
	    sprintf(buf, "%-20s %-5d ",
		    get_skill_name(ch, sn, TRUE),
		    skill_table[sn].rating[ch->classid]);
	    send_to_char(buf, ch);
	    if (++col % 3 == 0)
		send_to_char("\n\r", ch);
	}
    }
    if (col % 3 != 0)
	send_to_char("\n\r", ch);
    send_to_char("\n\r", ch);

    sprintf(buf, "Пункты генерации: %d\n\r",
	    ch->pcdata->points);
    send_to_char(buf, ch);
    sprintf(buf, "Очков на уровень: %d\n\r",
	    exp_per_level(ch, ch->gen_data->points_chosen));
    send_to_char(buf, ch);
    return;
}


void list_group_chosen(CHAR_DATA *ch)
{
    char buf[100];
    int gn, sn, col;

    if (IS_NPC(ch))
	return;

    col = 0;

    sprintf(buf, "%-20s %-5s %-20s %-5s %-20s %-5s",
	    "Группа", "Цена", "Группа", "Цена", "Группа", "Цена\n\r");
    send_to_char(buf, ch);

    for (gn = 0; gn < max_groups; gn++)
    {
	if (group_table[gn].name == NULL)
	    break;

	if (ch->gen_data->group_chosen[gn]
	    && group_table[gn].rating[ch->classid] > 0)
	{
	    sprintf(buf, "%-20s %-5d ",
		    get_skill_name(ch, gn, FALSE),
		    group_table[gn].rating[ch->classid]);
	    send_to_char(buf, ch);
	    if (++col % 3 == 0)
		send_to_char("\n\r", ch);
	}
    }
    if (col % 3 != 0)
	send_to_char("\n\r", ch);
    send_to_char("\n\r", ch);

    col = 0;

    sprintf(buf, "%-20s %-5s %-20s %-5s %-20s %-5s",
	    "Умение", "Цена", "Умение", "Цена", "Умение", "Цена\n\r");
    send_to_char(buf, ch);

    for (sn = 0; sn < max_skills; sn++)
    {
	if (skill_table[sn].name == NULL)
	    break;

	if (ch->gen_data->skill_chosen[sn]
	    && skill_table[sn].rating[ch->classid] > 0)
	{
	    sprintf(buf, "%-20s %-5d ",
		    get_skill_name(ch, sn, TRUE),
		    skill_table[sn].rating[ch->classid]);
	    send_to_char(buf, ch);
	    if (++col % 3 == 0)
		send_to_char("\n\r", ch);
	}
    }
    if (col % 3 != 0)
	send_to_char("\n\r", ch);
    send_to_char("\n\r", ch);

    sprintf(buf, "Пункты генерации: %d\n\r", ch->gen_data->points_chosen);
    send_to_char(buf, ch);
    sprintf(buf, "Очков на уровень: %d\n\r",
	    exp_per_level(ch, ch->gen_data->points_chosen));
    send_to_char(buf, ch);
    return;
}

int exp_per_level(CHAR_DATA *ch, int points)
{
    int expl, inc;

    if (IS_NPC(ch))
	return 1000;

    expl = 1000;
    inc = 500;

    if (pc_race_table[ch->race].class_mult[ch->classid] == 0)
    {
	bugf("%s - запрещенное сочетание: %s %s",
	     ch->name,
	     pc_race_table[ch->race].name,
	     class_table[ch->classid].name);
    }

    if (points < 40)
	return 1000 * (pc_race_table[ch->race].class_mult[ch->classid]
		       ? pc_race_table[ch->race].class_mult[ch->classid]/100 : 1);

    /* processing */
    points -= 40;

    while (points > 9)
    {
	expl += inc;
	points -= 10;
	if (points > 9)
	{
	    expl += inc;
	    inc *= 2;
	    points -= 10;
	}
    }

    expl += points * inc / 10;

    return expl * (pc_race_table[ch->race].class_mult[ch->classid]
		   ? pc_race_table[ch->race].class_mult[ch->classid]/100 : 1);
}

/* this procedure handles the input parsing for the skill generator */
bool parse_gen_groups(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MSL];
    int gn, sn, i;

    if (argument[0] == '\0')
	return FALSE;

    argument = one_argument(argument, arg);

    if (!str_prefix(arg, "help") || !str_prefix(arg, "помощь"))
    {
	if (argument[0] == '\0')
	{
	    do_function(ch, &do_help, "group help");
	    return TRUE;
	}

	do_function(ch, &do_help, argument);
	return TRUE;
    }

    if (!str_prefix(arg, "add") || !str_prefix(arg, "добавить"))
    {
	if (argument[0] == '\0')
	{
	    send_to_char("Ты должен ввести название умения.\n\r", ch);
	    return TRUE;
	}

	gn = group_lookup(argument);
	if (gn != -1)
	{
	    if (ch->gen_data->group_chosen[gn]
		|| ch->pcdata->group_known[gn])
	    {
		send_to_char("Ты уже знаешь эту группу умений!\n\r", ch);
		return TRUE;
	    }

	    if (group_table[gn].rating[ch->classid] < 1)
	    {
		send_to_char("Эта группа умений тебе недоступна.\n\r", ch);
		return TRUE;
	    }


/*
	    if (ch->gen_data->points_chosen + group_table[gn].rating[ch->class]	> 100)
	    {
		send_to_char("Ты не можешь набрать более 100 "
			     "пунктов генерации.\n\r", ch);
		return TRUE;
	    }
*/

	    sprintf(buf, "Группа умений '%s' добавлена\n\r",
		    get_skill_name(ch, gn, FALSE));
	    send_to_char(buf, ch);
	    ch->gen_data->group_chosen[gn] = TRUE;
	    ch->gen_data->points_chosen += group_table[gn].rating[ch->classid];
	    gn_add(ch, gn);
	    ch->pcdata->points += group_table[gn].rating[ch->classid];
	    return TRUE;
	}

	sn = skill_lookup(argument);
	if (sn != -1)
	{
	    if (ch->gen_data->skill_chosen[sn]
		|| ch->pcdata->learned[sn] > 0)
	    {
		send_to_char("Ты уже знаешь это умение!\n\r", ch);
		return TRUE;
	    }

	    if (skill_table[sn].rating[ch->classid] < 1
		|| skill_table[sn].spell_fun != spell_null
		|| skill_table[sn].quest[ch->classid])
	    {
		send_to_char("Это умение тебе недоступно.\n\r", ch);
		return TRUE;
	    }

	    if (check_depend(ch, sn))
		return TRUE;

/*
	    if (ch->gen_data->points_chosen + skill_table[sn].rating[ch->class]
		> 100)
	    {
		send_to_char("Ты не можешь набрать более 100 "
			     "пунктов генерации..\n\r", ch);
		return TRUE;
	    }
*/
	    sprintf(buf, "Умение '%s' добавлено\n\r",
		    get_skill_name(ch, sn, TRUE));
	    send_to_char(buf, ch);
	    ch->gen_data->skill_chosen[sn] = TRUE;
	    ch->gen_data->points_chosen += skill_table[sn].rating[ch->classid];
	    ch->pcdata->learned[sn] = 1;
	    ch->pcdata->points += skill_table[sn].rating[ch->classid];
	    return TRUE;
	}

	send_to_char("Такие умения или группы умений отсутствуют...\n\r", ch);
	return TRUE;
    }

    if (!strcmp(arg, "drop") || !strcmp(arg, "сбросить"))
    {
	if (argument[0] == '\0')
	{
	    send_to_char("Для сброса надо ввести название умения.\n\r", ch);
	    return TRUE;
	}

	gn = group_lookup(argument);
	if (gn != -1 && ch->gen_data->group_chosen[gn])
	{
	    send_to_char("Группа умений сброшена.\n\r", ch);
	    ch->gen_data->group_chosen[gn] = FALSE;
	    ch->gen_data->points_chosen -= group_table[gn].rating[ch->classid];
	    gn_remove(ch, gn);
	    
            group_add(ch, "rom basics", FALSE);
            group_add(ch, class_table[ch->classid].base_group, FALSE);
	    group_add(ch, class_table[ch->classid].default_group, FALSE);

	    for (i = 0; i < max_groups; i++)
		if (ch->gen_data->group_chosen[gn])
		    gn_add(ch, gn);

	    ch->pcdata->points -= group_table[gn].rating[ch->classid];
	    return TRUE;
	}

	sn = skill_lookup(argument);
	if (sn != -1 && ch->gen_data->skill_chosen[sn])
	{
	    for (i = 0; i < max_skills; i++)
	    {
		int j;

		if (i == sn || (!ch->gen_data->skill_chosen[i] && ch->pcdata->learned[i] < 1))
		    continue;

		for (j = 0; !IS_NULLSTR(skill_table[i].depends[j]); j++)
		{
		    if (skill_lookup(skill_table[i].depends[j]) == sn)
		    {
			sprintf(buf, "Нельзя сбросить это умение, поскольку от него зависит %s.\n\r", get_skill_name(ch, i, TRUE));
			send_to_char(buf, ch);
			return TRUE;
		    }
		}
	    }

	    send_to_char("Умение сброшено.\n\r", ch);
	    ch->gen_data->skill_chosen[sn] = FALSE;
	    ch->gen_data->points_chosen -= skill_table[sn].rating[ch->classid];
	    ch->pcdata->learned[sn] = 0;
	    ch->pcdata->points -= skill_table[sn].rating[ch->classid];
	    return TRUE;
	}

	send_to_char("Таких умений или групп умений еще не приобретено.\n\r",
		     ch);
	return TRUE;
    }

    if (!str_prefix(arg, "list") || !str_prefix(arg, "список"))
    {
	list_group_costs(ch);
	return TRUE;
    }

    if (!str_prefix(arg, "learned") || !str_prefix(arg, "приобретенные"))
    {
	list_group_chosen(ch);
	return TRUE;
    }

    if (!str_prefix(arg, "info") || !str_prefix(arg, "инфо"))
    {
	do_function(ch, &do_groups, argument);
	return TRUE;
    }

    return FALSE;
}

/* shows all groups, or the sub-members of a group */
void do_groups(CHAR_DATA *ch, char *argument)
{
    char buf[100];
    int gn, sn, col;

    if (IS_NPC(ch))
	return;

    col = 0;

    if (argument[0] == '\0')
    {   /* show all groups */

	for (gn = 0; gn < max_groups; gn++)
	{
	    if (group_table[gn].name == NULL)
		break;
	    if (ch->pcdata->group_known[gn])
	    {
		sprintf(buf, "%-20s ", get_skill_name(ch, gn, FALSE));
		send_to_char(buf, ch);
		if (++col % 3 == 0)
		    send_to_char("\n\r", ch);
	    }
	}
	if (col % 3 != 0)
	    send_to_char("\n\r", ch);
	sprintf(buf, "Пункты генерации: %d\n\r", ch->pcdata->points);
	send_to_char(buf, ch);
	return;
    }

    if (!str_cmp(argument, "all") || !str_cmp(argument, "все"))
    {
	for (gn = 0; gn < max_groups; gn++)
	{
	    if (group_table[gn].name == NULL)
		break;
	    sprintf(buf, "%-20s ", get_skill_name(ch, gn, FALSE));
	    send_to_char(buf, ch);
	    if (++col % 3 == 0)
		send_to_char("\n\r", ch);
	}
	if (col % 3 != 0)
	    send_to_char("\n\r", ch);
	return;
    }


    /* show the sub-members of a group */
    gn = group_lookup(argument);
    if (gn == -1)
    {
	send_to_char("Нет таких групп умений.\n\r"
		     "Набери 'инфо все' для просмотра полного списка.\n\r", ch);
	return;
    }

    for (sn = 0; sn < MAX_IN_GROUP; sn++)
    {
	char *tmp = group_table[gn].spells[sn];
	int i;
	bool is_skill;

	if (IS_NULLSTR(tmp))
	    continue;

	if ((i = group_lookup(tmp)) >= 0)
	    is_skill = FALSE;
	else if ((i = skill_lookup(tmp)) >= 0)
	    is_skill = TRUE;
	else
	    continue;

	sprintf(buf, "%-20s ", get_skill_name(ch, i, is_skill));
	send_to_char(buf, ch);
	if (++col % 3 == 0)
	    send_to_char("\n\r", ch);
    }
    if (col % 3 != 0)
	send_to_char("\n\r", ch);
}

/* checks for skill improvement */
/* ПРОКАЧКА УМЕНИЙ И ЗАКЛИНАНИЙ*/
void check_improve(CHAR_DATA *ch, CHAR_DATA *victim, int sn, bool success, int multiplier)
{
    int chance, rating;
    int lev_diff;
    int over = 0;
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA *paf;

    if (!ch || IS_NPC(ch) || ch == victim)
	return;

    //на арене скиллы не качаем, в безопасных местах, гильдиях, храмах, домах и кланах
    if (ch->in_room != NULL && (IS_SET(ch->in_room->room_flags, ROOM_ARENA) 
								|| IS_SET(ch->in_room->room_flags, ROOM_SAFE)
								|| IS_SET(ch->in_room->room_flags, ROOM_GUILD)
								|| IS_SET(ch->in_room->room_flags, ROOM_HOLY)
								|| IS_SET(ch->in_room->room_flags, ROOM_HOUSE)
								|| (ch->in_room->clan>0)
							   ))
	return;

    //безопыта тоже скиллы не качаем
    if (IS_SET(ch->act, PLR_NOEXP))
	return;

    if (victim	&& IS_NPC(victim) && (IS_SET(victim->act, ACT_PET) || IS_SET(victim->act, ACT_NOEXP))){
		return;  /* Ибо не хер на кошках умения качать. */
    }

    if (victim && !IS_NPC(victim) && !IS_NPC(ch) && (ch->level + 3 < victim->level)){
		return;  /* Ибо не хер на чарах хаях качать. */
    }


    if (ch->level < skill_table[sn].skill_level[ch->classid]
	|| (rating = get_skill_rating(ch, sn)) == 0
	|| ch->pcdata->learned[sn] <= 1
	|| ch->pcdata->learned[sn] >= 100
	|| sn < 0)
    {
	return;  /* skill is not known */
    }

    /*  Skillaff fix  */
    for  (paf = ch->affected; paf; paf = paf->next)
	if (paf->type == sn
	    && paf->bitvector == AFF_GODS_SKILL
	    && paf->modifier < 0)
	{
	    return;
	}

    /* check to see if the character has a chance to learn */
    chance = 10 * int_app[get_curr_stat(ch, STAT_INT)].learn;
    if (victim && victim->fighting == ch)
	lev_diff = URANGE(0, victim->level - ch->level + 3, 7) * 20;
    else
	lev_diff = ch->level;

    over = (2 * chance / multiplier + lev_diff) / (2 * rating);
    if (over < 0)
	over = 0;

    /* now that the character has a CHANCE to learn, see if they really have */

    if (success)
	over *= URANGE(5, 100 - ch->pcdata->learned[sn], 90);
    else
	over *= URANGE(5, ch->pcdata->learned[sn]/2, 25);


//делаем постоянные утроения
/*    over *= 2.75;*/
    over *= 9;

    /*over *= immquest.double_skill;
*/

    if (immquest.is_quest){
		over *= immquest.double_skill;
		over = over*2/3;
    }


    ch->pcdata->over_skill[sn] += over;
    if (ch->pcdata->over_skill[sn] >= MAX_SKILL_OVER)
    {
	if (ch->pcdata->learned[sn] == 99)
	{
	    sprintf(buf, "{GТы достигаешь мастерства в %s '%s'!{x\n\r",
		    skill_spell(sn, TRUE), get_skill_name(ch, sn, TRUE));
	}
	else if (success)
	{
	    sprintf(buf, "{GТы повышаешь свой опыт в %s '%s'!{x\n\r",
		    skill_spell(sn, TRUE), get_skill_name(ch, sn, TRUE));
	}
	else
	{
	    sprintf(buf, "{GТы учишься на своих ошибках, и твое мастерство "
		    "в %s '%s' растет.{x\n\r",
		    skill_spell(sn, TRUE), get_skill_name(ch, sn, TRUE));
	}
	send_to_char(buf, ch);
	ch->pcdata->learned[sn]++;
	if (!IS_RENEGATE(ch))
	    gain_exp(ch, 2 * rating, FALSE);
	ch->pcdata->over_skill[sn] %= MAX_SKILL_OVER;
    }
}

/* returns a group index number given the name */

/* recursively adds a group given its number -- uses group_add */
void gn_add(CHAR_DATA *ch, int gn)
{
    int i;

    ch->pcdata->group_known[gn] = TRUE;
    for (i = 0; i < MAX_IN_GROUP; i++)
    {
	if (group_table[gn].spells[i] == NULL)
	    continue;
	group_add(ch, group_table[gn].spells[i], FALSE);
    }
}

/* recusively removes a group given its number -- uses group_remove */
void gn_remove(CHAR_DATA *ch, int gn)
{
    int i;

    ch->pcdata->group_known[gn] = FALSE;

    for (i = 0; i < MAX_IN_GROUP; i ++)
    {
	if (group_table[gn].spells[i] == NULL)
	    continue;
	group_remove(ch, group_table[gn].spells[i]);
    }
}

/* use for processing a skill or group for addition  */
void group_add(CHAR_DATA *ch, const char *name, bool deduct)
{
    int sn, gn;

    if (IS_NPC(ch)) /* NPCs do not have skills */
	return;

    sn = skill_lookup(name);

    if (sn != -1)
    {
	if (ch->pcdata->learned[sn] == 0) /* i.e. not known */
	{
	    ch->pcdata->learned[sn] = 1;
	    if (deduct)
		ch->pcdata->points += skill_table[sn].rating[ch->classid];
	}
	return;
    }

    /* now check groups */

    gn = group_lookup(name);

    if (gn != -1)
    {
	if (ch->pcdata->group_known[gn] == FALSE)
	{
	    ch->pcdata->group_known[gn] = TRUE;
	    if (deduct)
		ch->pcdata->points += group_table[gn].rating[ch->classid];
	}
	gn_add(ch, gn); /* make sure all skills in the group are known */
    }
}

/* used for processing a skill or group for deletion -- no points back! */

void group_remove(CHAR_DATA *ch, const char *name)
{
    int sn, gn;

    sn = skill_lookup(name);

    if (sn != -1)
    {
	ch->pcdata->learned[sn] = 0;
	return;
    }

    /* now check groups */

    gn = group_lookup(name);

    if (gn != -1 && ch->pcdata->group_known[gn] == TRUE)
    {
	ch->pcdata->group_known[gn] = FALSE;
	gn_remove(ch, gn);  /* be sure to call gn_add on all remaining groups */
    }
}

int get_skill_rating(CHAR_DATA *ch, int skill)
{
    int i;

    for (i = 0; i < 5 && pc_race_table[ch->race].skills[i] != NULL; i++)
	if (skill_lookup(pc_race_table[ch->race].skills[i]) == skill)
	    return (skill_table[skill].rating[ch->classid] > 0)
		? skill_table[skill].rating[ch->classid] : number_bits(2) + 1;

    return skill_table[skill].rating[ch->classid];
}
/* charset=cp1251 */
