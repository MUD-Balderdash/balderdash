/****************************************************************************
 *  Original house code by Key (c) 2005, Russia			   					*
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "recycle.h"
#include "interp.h"



bool is_compatible_races(void *ch1, void *ch2, bool is_char_data);

#define CAN 1

bool is_marry(CHAR_DATA *ch1, CHAR_DATA *ch2)
{
    if (IS_NPC(ch1) || IS_NPC(ch2))
    {
	return FALSE;
    }

    if (IS_NULLSTR(ch1->pcdata->spouse) || IS_NULLSTR(ch2->pcdata->spouse))
    {
	return FALSE;
    }

    if (str_cmp(ch2->name, ch1->pcdata->spouse) || str_cmp(ch1->name, ch2->pcdata->spouse))
    {
	return FALSE;
    }

    return TRUE;
}

void do_marry(CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *victim2;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0')
    {
	send_to_char("Синтаксис: поженить <char1> <char2>\n\r", ch);
	return;
    }
    if ((victim = get_char_world(ch, arg1)) == NULL)
    {
	send_to_char("Первая персона отсутствует.\n\r", ch);
	return;
    }

    if ((victim2 = get_char_world(ch, arg2)) == NULL)
    {
	send_to_char("Вторая персона отсутствует.\n\r", ch);
	return;
    }

    if (victim == victim2)
    {
	send_to_char("Поженить с самим собой? Окстись!.\n\r", ch);
	return;
    }

    if (IS_NPC(victim) || IS_NPC(victim2))
    {
	send_to_char("Поженить с мобом? Ты спятил.\n\r", ch);
	return;
    }

    if (!IS_SET(victim->act, PLR_CONSENT) || !IS_SET(victim2->act, PLR_CONSENT))
    {
	send_to_char("Они не давали своего согласия.\n\r", ch);
	return;
    }

    if (!IS_NULLSTR(victim->pcdata->spouse)
	|| !IS_NULLSTR(victim2->pcdata->spouse))
    {
	send_to_char("Они уже женаты/замужем! \n\r", ch);
	return;
    }


    if (victim->level < 12 || victim2->level < 12)
    {
	send_to_char("Им еще рановато вступать в такие отношения. "
		     "Пусть дождутся 12 уровня.\n\r", ch);
	return;
    }

    send_to_char("Ты объявляешь их МУЖЕМ И ЖЕНОЙ!\n\r", ch);
    send_to_char("Теперь вы муж и жена.\n\r", victim);
    send_to_char("Теперь вы муж и жена.\n\r", victim2);

    REMOVE_BIT(ch->act, PLR_CONSENT);
    REMOVE_BIT(victim->act, PLR_CONSENT);

    free_string(victim->pcdata->spouse);
    free_string(victim2->pcdata->spouse);
    victim->pcdata->spouse = str_dup(victim2->name);
    victim2->pcdata->spouse = str_dup(victim->name);
    return;
}

void do_divorce(CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *victim2;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0')
    {
	send_to_char("Синтаксис: развод <char1> <char2>\n\r", ch);
	send_to_char("Синтаксис: развод <char1> безусловно\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, arg1)) == NULL)
    {
	send_to_char("Первая персона отсутствует.\n\r", ch);
	return;
    }

    if (str_cmp(arg2, "безусловно"))
    {
	if ((victim2 = get_char_world(ch, arg2)) == NULL)
	{
	    send_to_char("Вторая персона отсутствует.\n\r", ch);
	    return;
	}

	if (IS_NPC(victim) || IS_NPC(victim2))
	{
	    send_to_char("Вряд ли мобы могут участвовать в бракоразводных "
			 "процессах...\n\r", ch);
	    return;
	}

	if (!IS_SET(victim->act, PLR_CONSENT) || !IS_SET(victim2->act, PLR_CONSENT))
	{
	    send_to_char("Они не давали своего согласия на развод.\n\r", ch);
	    return;
	}

	if (str_cmp(victim->pcdata->spouse, victim2->name))
	{
	    send_to_char("Но они же не женаты между собой!\n\r", ch);
	    return;
	}

	send_to_char("Ты осуществляешь развод.\n\r", ch);
	send_to_char("Да, развод - это финал этой грустной истории.\n\r", victim);
	send_to_char("Да, развод - это финал этой грустной истории.\n\r", victim2);
	free_string(victim->pcdata->spouse);
	free_string(victim2->pcdata->spouse);
	victim->pcdata->spouse = NULL;
	victim2->pcdata->spouse = NULL;
	return;

    }
    else
    {
	if (IS_NPC(victim))
	{
	    send_to_char("Вряд ли мобы могут участвовать в бракоразводных "
			 "процессах...\n\r", ch);
	    return;
	}

	if (!IS_SET(victim->act, PLR_CONSENT))
	{
	    send_to_char("Он не давал своего согласия на развод.\n\r", ch);
	    return;
	}

	send_to_char("Ты осуществляешь развод.\n\r", ch);
	send_to_char("Да, развод - это финал этой грустной истории.\n\r", victim);
	free_string(victim->pcdata->spouse);
	victim->pcdata->spouse = NULL;
	return;
    }
}

void do_consent(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->act, PLR_CONSENT))
    {
	if (!IS_NULLSTR(ch->pcdata->spouse))
	    send_to_char("Ты больше не даешь согласия на развод.\n\r", ch);
	else
	    send_to_char("Ты больше не даешь согласия на свадьбу.\n\r", ch);

	REMOVE_BIT(ch->act, PLR_CONSENT);
	return;
    }

    if (!IS_NULLSTR(ch->pcdata->spouse))
	send_to_char("Ты даешь свое согласие на развод...\n\r", ch);
    else
	send_to_char("Ты даешь свое согласие на Свадьбу!\n\r", ch);

    SET_BIT(ch->act, PLR_CONSENT);
    return;
}


void do_spousetalk(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (IS_NPC(ch))
	return;

    if (IS_NULLSTR(ch->pcdata->spouse))
    {
	switch (ch->sex)
	{
	case SEX_MALE:
	    send_to_char("Но ты же даже не женат!\n\r", ch);
	    return;
	case SEX_FEMALE:
	    send_to_char("Но ты же даже не замужем!\n\r", ch);
	    return;
	default:
	    send_to_char("Но ты же даже не... Хм. И правильно - "
			 "кому ты такое нужно?!\n\r", ch);
	    return;
	}
    }

    if (argument[0] == '\0')
    {
	sprintf(buf, "Что ты хотел%s сказать своей половине?\n\r",
		SEX_ENDING(ch));
	send_to_char(buf, ch);
	return;
    }

    SLIST_FOREACH(d, &descriptor_list, link)
    {
	CHAR_DATA *victim;

	victim = d->original ? d->original : d->character;

	if (d->connected == CON_PLAYING
	    && victim != ch
	    && !str_cmp(victim->name, ch->pcdata->spouse))
	{
	    act_new("$n говорит тебе: {W$t{x",
		    ch, argument, d->character, TO_VICT, POS_SLEEPING);

	    sprintf(buf, "Ты говоришь %s: {W%s{x\n\r",
		    cases(ch->pcdata->spouse, 2), argument);
	    send_to_char(buf, ch);
	    return;
	}
    }
    send_to_char("К сожалению, твоя половина отсутствует.\n\r", ch);
    return;
}

void do_automarry(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj2;
    CHAR_DATA *sm;
    CHAR_DATA *victim;

    if (IS_NPC(ch))
	return;

    LIST_FOREACH(sm, &ch->in_room->people, room_link)
	if (IS_NPC(sm) && IS_SET(sm->act, ACT_IS_HEALER))
	    break;

    //проверка на наличие в комнате лекаря-жреца
    if (sm == NULL)
    {
	send_to_char("Ты не можешь здесь сыграть свадьбу!\n\r", ch);
	return;
    }

    //проверку на секс - инициатива у мужчины
    if (ch->sex != SEX_MALE)
    {
	sprintf(buf, "%s, инициатива должна происходить от мужчины, не находишь?", ch->name);
	do_say(sm, buf);
	return;
    }
    //проверка на то, женат ли жених
    if (!IS_NULLSTR(ch->pcdata->spouse))
    {
	sprintf(buf, "%s, ты и так уже женат.", ch->name);
	do_say(sm, buf);
	return;
    }

    //проверка на уровень
    if (ch->level < 12)
    {
	sprintf(buf, "%s, тебе рано еще жениться. Дождись 12 уровня.", ch->name);
	do_say(sm, buf);
	return;
    }

    argument = one_argument(argument, arg);

    //проверка - на ком жениться-то?
    if (arg[0] == '\0')
    {
	sprintf(buf, "%s, на ком ты хочешь жениться?", ch->name);
	do_say(sm, buf);
	return;
    }

    //проверку на наличие чара в комнате
    if ((victim = get_char_room(ch, NULL, arg, FALSE)) == NULL)
    {
	sprintf(buf, "%s, таких рядом с тобой нет.", ch->name);
	do_say(sm, buf);
	return;
    }

    if (ch == victim)
    {
	sprintf(buf, "%s, жениться самому на себе?:-)", ch->name);
	do_say(sm, buf);
	return;
    }

    //проверка на то, чар ли избранница :-)
    if (IS_NPC(victim))
    {
	sprintf(buf, "Ну не на мобе же, %s?", ch->name);
	do_say(sm, buf);
	return;
    }

    if (victim->pcdata->flag_can_marry != CAN)
    {
	sprintf(buf, "{R%s, %s пытается на тебе жениться."
		" Если ты не согласна - просто не набирай команду 'согласие'.{x\n\r", victim->name, ch->name);
	send_to_char(buf, victim);
    }

    //проверка на секс партнера
    if (victim->sex != SEX_FEMALE)
    {
	sprintf(buf, "%s, %s же не девушка...", ch->name, victim->name);
	do_say(sm, buf);
	return;
    }

    if (victim->position < POS_STANDING)
    {
	sprintf(buf, "%s, %s же явно не готова выйти замуж. Пусть хотя бы встанет...", ch->name, victim->name);
	do_say(sm, buf);
	return;
    }

    if (!IS_SET(ch->act, PLR_CONSENT)||!IS_SET(victim->act, PLR_CONSENT))
    {
	sprintf(buf, "%s, %s, вы же не согласны жениться.", ch->name, victim->name);
	do_say(sm, buf);
	return;
    }

    //проверка на то, замужем ли невеста
    if (!IS_NULLSTR(victim->pcdata->spouse))
    {
	sprintf(buf, "%s, %s и так замужем. С чем ее и поздравляем!", ch->name, victim->name);
	do_say(sm, buf);
	return;
    }

    //проверка на уровень
    if (victim->level < 12)
    {
	sprintf(buf, "%s, ей еще рано выходить замуж. Дождись 12 уровня.", ch->name);
	do_say(sm, buf);
	return;
    }

    //проверка на расу партнера (если не одной расы - пусть идут к богам)
    /*	if (ch->race != victim->race)
     {
     sprintf(buf, "%s, %s - нет уж, от межрасовых браков боги возмутятся. Обратитесь к ним...", ch->name, victim->name);
     do_say(sm, buf);
     return;
     }*/
    //проверка на разрешение по расе.
    if (!is_compatible_races(ch, victim, TRUE))
    {
	sprintf(buf, "Вы же не уживетесь вдвоем.");
	do_say(sm, buf);
	sprintf(buf, "%s, %s - нет уж, от межрасовых браков боги возмутятся. Обратитесь к ним...", ch->name, victim->name);
	do_say(sm, buf);
	return;
    }

    //проверка на достаточность qp и золота
    if (ch->pcdata->quest_curr < 1000)
    {
	sprintf(buf, "%s, у тебя нет требуемого количества пунктов удачи.", ch->name);
	do_say(sm, buf);

	sprintf(buf, "Свадьба стоит 1000 очков удачи.");
	do_say(sm, buf);
	return;
    }

    if (ch->gold < 1000)
    {
	sprintf(buf, "%s, у тебя нет требуемого количества золота.", ch->name);
	do_say(sm, buf);
	sprintf(buf, "Свадьба стоит 1000 золота.");
	do_say(sm, buf);
	return;
    }

    //проверка на то, опрашивал ли их ранее моб. Если нет, то обнулить им нужные биты и запустить процесс по-новой
    //если да, то продолжить свадьбу. Это - защита от обманов со свадьбами :-)
    if (ch->pcdata->flag_can_marry != CAN || victim->pcdata->flag_can_marry != CAN)
    {
	REMOVE_BIT(ch->act, PLR_CONSENT);
	REMOVE_BIT(victim->act, PLR_CONSENT);

	ch->pcdata->flag_can_marry = CAN;
	victim->pcdata->flag_can_marry = CAN;

	sprintf(buf, "%s, ты пригласил %s замуж. Набери 'согласие', если ты не ошибся.", ch->name, victim->pcdata->cases[2]);
	do_say(sm, buf);
	sprintf(buf, "%s, %s пригласил тебя замуж. Набери согласие, если ты согласна.", victim->name, ch->name);
	do_say(sm, buf);
	return;
    }

    if (!IS_SET(ch->act, PLR_CONSENT))
    {
	sprintf(buf, "%s, ты же не согласен жениться.", ch->name);
	do_say(sm, buf);
	return;
    }

    if (!IS_SET(victim->act, PLR_CONSENT))
    {
	sprintf(buf, "%s, ты же не согласна выйти замуж.", victim->name);
	do_say(sm, buf);
	return;
    }

    //проверка на наличие колец
    if ((obj = get_obj_carry(ch, "обручальное кольцо", ch)) == NULL)
    {
	sprintf(buf, "%s, у тебя нет обручального кольца.", ch->name);
	do_say(sm, buf);
	return;
    }

    if ((obj2 = get_obj_carry(victim, "обручальное кольцо", victim)) == NULL)
    {
	sprintf(buf, "%s, у тебя нет обручального кольца.", victim->name);
	do_say(sm, buf);
	return;
    }

    //делаем им кольца
    //кольца забрать, сделать новые, уже ренеймленные :-)
    extract_obj(obj, TRUE, TRUE);
    extract_obj(obj2, TRUE, TRUE);

    obj = create_object(get_obj_index(OBJ_VNUM_MARRY_RING), 0);
    obj2 = create_object(get_obj_index(OBJ_VNUM_MARRY_RING), 0);

    obj->level	= victim->level;
    obj2->level	= ch->level;

    free_string(obj->owner);
    free_string(obj2->owner);

    obj->owner	= str_dup(victim->name);
    obj2->owner	= str_dup(ch->name);

    sprintf(buf, "обручальное кольцо %s", ch->pcdata->cases[0]);
    free_string(obj->name);
    obj->name = str_dup(buf);

    sprintf(buf, "обручальное кольцо {/%s", ch->pcdata->cases[0]);
    free_string(obj->short_descr);
    obj->short_descr = str_dup(buf);

    sprintf(buf, "Обручальное кольцо %s лежит здесь", ch->pcdata->cases[0]);
    free_string(obj->description);
    obj->description = str_dup(buf);

    sprintf(buf, "обручальное кольцо %s", victim->pcdata->cases[0]);
    free_string(obj2->name);
    obj2->name = str_dup(buf);

    sprintf(buf, "обручальное кольцо {/%s", victim->pcdata->cases[0]);
    free_string(obj2->short_descr);
    obj2->short_descr = str_dup(buf);

    sprintf(buf, "Обручальное кольцо %s лежит здесь", victim->pcdata->cases[0]);
    free_string(obj2->description);
    obj2->description = str_dup(buf);

    obj_to_char(obj, ch);
    obj_to_char(obj2, victim);

    send_to_char("Священник дарит тебе обручальное кольцо.\n\r", ch);
    send_to_char("Священник дарит тебе обручальное кольцо.\n\r", victim);

    //обнуляем все флаги
    REMOVE_BIT(ch->act, PLR_CONSENT);
    REMOVE_BIT(victim->act, PLR_CONSENT);

    ch->pcdata->flag_can_marry = 0;
    victim->pcdata->flag_can_marry = 0;

    sprintf(buf, "%s и %s, теперь вы муж и жена.", ch->name, victim->name);
    do_say(sm, buf);

    sprintf(buf, "Обменяйтесь кольцами и поцелуйте друг друга.");
    do_say(sm, buf);

    //женим
    free_string(ch->pcdata->spouse);
    free_string(victim->pcdata->spouse);

    ch->pcdata->spouse = str_dup(victim->name);
    victim->pcdata->spouse = str_dup(ch->name);

    //берем плату за свадьбу
    ch->gold -= 1000;
    ch->pcdata->quest_curr -= 1000;
    send_to_char("Ты платишь за свадьбу 1000 золота и 1000 очков удачи.\n\r", ch);

    return;
}


/* charset=cp1251 */






