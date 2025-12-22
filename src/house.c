/* For Key testing Смотри, есть строчка? */

/***************************************************************************
 *  Original house code by Key 2006, Russia								   *
 ***************************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "interp.h"


void do_invite( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *victim2;
    char buf[MAX_INPUT_LENGTH];
    int i, j, k;
    int number = -1;
    int flag = -1;

    if (IS_NPC(ch))
	return;

    one_argument(argument,arg);

    if (arg[0] == '\0')
    {
	send_to_char(" ______________________________________________________________________\n\r", ch);
	send_to_char("||    Ты пригласил к себе домой:    || Тебя пригласили к себе домой : ||\n\r", ch);
	send_to_char("||====================================================================||\n\r", ch);
	for (i=0; i<MAX_GUESTS; i++)
	{
	    victim = pc_id_lookup(ch->pcdata->id_who_guest[i]);
	    victim2 = pc_id_lookup(ch->pcdata->id_whom_guest[i]);
	    sprintf(buf, "|| %20s             || %20s           ||\n\r", victim != NULL? victim->name : "*****",
		    victim2 != NULL? victim2->name : "*****");
	    send_to_char(buf, ch);
	}
	send_to_char(" ----------------------------------------------------------------------\n\r", ch);
	return;
    }
    else if ((victim = get_char_room(ch, NULL, arg, FALSE)) == NULL)
    {
	send_to_char("Таких рядом с тобой нет.\n\r",ch);
	return;
    }

    if (ch->pcdata->vnum_house == 0)
    {
	send_to_char("У тебя нет дома.\n\r", ch);
	return;
    }

    if (IS_NPC(victim))
    {
	send_to_char("Только не моба.\n\r", ch);
	return;
    }

    if (ch == victim)
    {
	send_to_char("Приглашать самого себя? Параноик...\n\r", ch);
	return;
    }

    //проверить массив приглашений на повторное приглашение, запомнить номер первой пустой вакансии на приглашение
    for (i = 0; i < MAX_GUESTS; i++)
    {
	if (ch->pcdata->id_who_guest[i] == victim->id)
	{
	    //удалить из обоих списков, прервать функцию.
	    //1.удалить из списка приглашаемого.
	    for (j = 0; j < MAX_GUESTS; j++)
	    {
		if (victim->pcdata->id_whom_guest[j] == ch->id)
		{
		    for (k = j; k < MAX_GUESTS; k++)
		    {
			victim->pcdata->id_whom_guest[k] = (k < (MAX_GUESTS - 1) ? victim->pcdata->id_whom_guest[k + 1] : 0);
		    }
		    break;
		}
	    }
	    //2.удалить из списка приглашающего.
	    for (j = i; j < MAX_GUESTS; j++) //очистка списка отказывающегос
	    {
		ch->pcdata->id_who_guest[j] = (j < (MAX_GUESTS - 1) ?  ch->pcdata->id_who_guest[j + 1] : 0);
	    }
	    sprintf(buf, "Ты отказал%s %s в приглашении.\n\r", SEX_ENDING(ch), victim->pcdata->cases[1]);
	    send_to_char(buf, ch);
	    sprintf(buf, "%s отказал%s тебе в приглашении.\n\r", ch->name, SEX_ENDING(ch));
	    send_to_char(buf, victim);
	    return;
	}

	if (pc_id_lookup(ch->pcdata->id_who_guest[i]) == NULL)
	{
	    if (number == -1) number = i;
	}
    }

    if (number == -1)
    {
	sprintf(buf, "Ты и так пригласил%s слишком много народу в гости.\n\r", SEX_ENDING(ch));
	send_to_char(buf, ch);
	return;
    }

    //проверить приглашаемого на вакансию на приглашение, запомнить номер первой пустой вакансии на приглашение его
    for (j=0; j<MAX_GUESTS; j++)
    {
	if (pc_id_lookup(victim->pcdata->id_whom_guest[j]) == NULL)
	{
	    flag = j;
	    break;
	}
    }
    if (flag == -1)
    {
	sprintf(buf, "У %s и так много приглашений.\n\r", victim->pcdata->cases[0]);
	send_to_char(buf, ch);
	return;
    }

    //приглашаем :-)
    ch->pcdata->id_who_guest[number] = victim->id;
    victim->pcdata->id_whom_guest[flag] = ch->id;

    act("Ты приглашаешь $N3 в гости.", ch, NULL, victim, TO_CHAR);
    act("$n приглашает тебя в гости.", ch, NULL, victim, TO_VICT);

    return;
}

void do_refuse( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    int i, j, k;
    int flag = 0;

    if (IS_NPC(ch))
	return;

    one_argument(argument,arg);

    //без аргументов очищает весь список, с аргументом находит нужного чара и удаляет его из списка
    if (arg[0] == '\0')
    {
	for (i=0; i<MAX_GUESTS; i++)
	{
	    victim = pc_id_lookup(ch->pcdata->id_whom_guest[i]);
	    if (victim != NULL)
	    {
		for (j=0; j<MAX_GUESTS; j++)
		{
		    if (victim->pcdata->id_who_guest[j] == ch->id)
		    {
			for (k=j; k<MAX_GUESTS; k++)
			{
			    victim->pcdata->id_who_guest[k] = (k < (MAX_GUESTS-1) ? victim->pcdata->id_who_guest[k+1] : 0);
			}
			break;
		    }
		}
		flag = 1;

		ch->pcdata->id_whom_guest[i] = 0;
		sprintf(buf, "Ты отказываешься от приглашения %s.\n\r", victim->pcdata->cases[0]);
		send_to_char(buf, ch);
		sprintf(buf, "%s отказывается от твоего приглашения.\n\r", ch->name);
		send_to_char(buf, victim);
	    }
	}
	if (flag == 0)
	{
	    sprintf(buf, "Ты и так никем не был%s приглашен%s.\n\r", SEX_ENDING(ch), SEX_ENDING(ch));
	    send_to_char(buf, ch);
	}
	return;
    }
    else
    {
	if ((victim = get_char_world(ch, arg)) == NULL)
	{
	    sprintf(buf, "Таких в мире нет, или же ты не приглашен%s ими.\n\r", SEX_ENDING(ch));
	    send_to_char(buf, ch);
	    return;
	}

	for (i=0; i<MAX_GUESTS; i++)
	{
	    if (ch->pcdata->id_whom_guest[i] == victim->id)
	    {
		for (j=i; j<MAX_GUESTS; j++)//очистка списка отказывающегос
		{
		    ch->pcdata->id_whom_guest[j] = (j < (MAX_GUESTS-1) ?  ch->pcdata->id_whom_guest[j+1] : 0);
		}
		for (j=0; j<MAX_GUESTS; j++)//очистка списка приглшающего
		{
		    if (victim->pcdata->id_who_guest[j] == ch->id)
		    {
			for (k=j; k<MAX_GUESTS; k++)
			{
			    victim->pcdata->id_who_guest[k] = (k < (MAX_GUESTS-1) ?  victim->pcdata->id_who_guest[k+1] : 0);
			}
			break;
		    }

		}
		flag = 1;
		sprintf(buf, "Ты отказываешься от приглашения %s.\n\r", victim->pcdata->cases[0]);
		send_to_char(buf, ch);
		sprintf(buf, "%s отказывается от твоего приглашения.\n\r", ch->name);
		send_to_char(buf, victim);
		return;
	    }
	}
    }
    sprintf(buf, "Таких в мире нет, или же ты не приглашен%s ими.\n\r", SEX_ENDING(ch));
    send_to_char(buf, ch);

    return;
}

void do_build_room( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *sm;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    int number_exit = -1;

    if (IS_NPC(ch))
	return;

    //проверяем наличие зодчего. Сначала просто в комнате, потом заменить на проверку - следует ли он.
    LIST_FOREACH(sm, &ch->in_room->people, room_link)
    {
	if (IS_NPC(sm) && IS_SET(sm->act, ACT_AROSH))
	    break;
    }

    if (sm == NULL)
    {
	send_to_char("Сначала пригласи тех, кто умеет строить.\n\r", ch);
	return;
    }

    //проверяем первый аргумент.
    argument = one_argument(argument,arg);

    if (arg[0] == '\0')
    {
		send_to_char("Построить что?\n\r", ch);
		return;
    }

    //постройка дома и комнаты идентична, просто 2 проверки немножко разные. Потому и объединил...
    if (!str_cmp(arg, "дом") || !str_cmp(arg, "house") || !str_cmp(arg, "комната") || !str_cmp(arg, "room"))
    {
	//сначала выясняем, может ли чар иметь дом, и может ли он его здесь построить.
	if (!str_cmp(arg, "дом") || !str_cmp(arg, "house"))
	{
	    if(ch->pcdata->vnum_house != 0)
	    {
		send_to_char("У тебя и так уже есть дом. Может, лучше его просто расширить?\n\r", ch);
		return;
	    }

	    if (!IS_SET(ch->in_room->room_flags, ROOM_CAN_BUILD_HOUSE))
	    {
		send_to_char("Здесь нельзя построить дом. Поищи другое место.\n\r", ch);
		return;
	    }
	}
	//теперь выясняем, имеет ли чар вообще дом, чтобы строить комнату.
	if (!str_cmp(arg, "комната") || !str_cmp(arg, "room"))
	{
	    if(ch->pcdata->vnum_house == 0)
	    {
		send_to_char("У тебя вообще еще нет дома. Сначала построй его.\n\r", ch);
		return;
	    }

	    //проверка на то - дом ли тут?
	    if (IS_NULLSTR(ch->in_room->owner))
	    {
		sprintf(buf, "Здесь нет дома.");
		do_say(sm, buf);
		return;
	    }

	    //проверка на владельца комнаты - если комната не его, то запретить строить.
	    if (!is_room_owner(ch, ch->in_room))
	    {
		sprintf(buf, "Здесь не твой дом. Строить в чужом доме... упс....");
		do_say(sm, buf);
		return;
	    }
	}

	//проверка на достаточность qp и золота
	if (ch->pcdata->quest_curr < 500)
	{
	    sprintf(buf, "%s, у тебя нет требуемого количества пунктов удачи.", ch->name);
	    do_say(sm, buf);

	    sprintf(buf, "Построить дом или комнату стоит 500 очков удачи.");
	    do_say(sm, buf);
	    return;
	}

	if (ch->gold < 5000)
	{
	    sprintf(buf, "%s, у тебя нет требуемого количества золота.", ch->name);
	    do_say(sm, buf);
	    sprintf(buf, "Построить дом или комнату стоит 5000 золота.");
	    do_say(sm, buf);
	    return;
	}

	//проверить наличие выхода в нужном направлении, в идеале потом - если не направление - проверить на описание, название комнаты
	argument = one_argument(argument,arg);

	if (arg[0] == '\0')
	{
	    sprintf(buf, "Построить в каком направлении?");
	    do_say(sm, buf);
	    return;
	}

	if (!str_cmp(arg, "север") || !str_cmp(arg, "north"))
	{
	    number_exit = 0;
	}

	if (!str_cmp(arg, "восток") || !str_cmp(arg, "east"))
	{
	    number_exit = 1;
	}

	if (!str_cmp(arg, "юг") || !str_cmp(arg, "south"))
	{
	    number_exit = 2;
	}

	if (!str_cmp(arg, "запад") || !str_cmp(arg, "west"))
	{
	    number_exit = 3;
	}

	if (!str_cmp(arg, "вверх") || !str_cmp(arg, "up"))
	{
	    number_exit = 4;
	}

	if (!str_cmp(arg, "вниз") || !str_cmp(arg, "down"))
	{
	    number_exit = 5;
	}

	if (number_exit == -1)
	{
	    sprintf(buf, "Укажи нормальное направление.");
	    do_say(sm, buf);
	    return;
	}

	if (ch->in_room->exit[number_exit] != NULL)
	{
	    sprintf(buf, "В этом направлении ты не можешь построить дом.");
	    do_say(sm, buf);
	    return;
	}

	//построить комнату в нужном направлении, проставить все флаги и owner-а.

	//берем плату за постройку дома
	ch->gold -= 5000;
	ch->pcdata->quest_curr -= 500;
	send_to_char("Ты платишь за постройку дома 5000 золота и 500 очков удачи.\n\r", ch);

	sprintf(buf, "Зодчий строит тебе дом в направлении %s.\n\r", arg);
	send_to_char(buf, ch);
	return;
    }

    //меняем интерьер (описание)
    if (!str_cmp(arg, "интерьер") || !str_cmp(arg, "description"))
    {

	if(ch->pcdata->vnum_house == 0)
	{
	    send_to_char("У тебя вообще еще нет дома. Сначала построй его.\n\r", ch);
	    return;
	}

	//проверка на то - дом ли тут?
	if (IS_NULLSTR(ch->in_room->owner))
	{
	    sprintf(buf, "Здесь нет дома.");
	    do_say(sm, buf);
	    return;
	}

	//проверка на владельца комнаты - если комната не его, то запретить строить.
	if (!is_room_owner(ch, ch->in_room))
	{
	    sprintf(buf, "Здесь не твой дом. Менять интерьер в чужом доме... упс....");
	    do_say(sm, buf);
	    return;
	}

    }

    //меняем название (name)
    if (!str_cmp(arg, "название") || !str_cmp(arg, "name"))
    {
	if(ch->pcdata->vnum_house == 0)
	{
	    send_to_char("У тебя вообще еще нет дома. Сначала построй его.\n\r", ch);
	    return;
	}

	//проверка на то - дом ли тут?
	if (IS_NULLSTR(ch->in_room->owner))
	{
	    sprintf(buf, "Здесь нет дома.");
	    do_say(sm, buf);
	    return;
	}

	//проверка на владельца комнаты - если комната не его, то запретить строить.
	if (!is_room_owner(ch, ch->in_room))
	{
	    sprintf(buf, "Здесь не твой дом. Менять название комнаты в чужом доме... упс....");
	    do_say(sm, buf);
	    return;
	}

    }

    //меняем выходы (exits)
    if (!str_cmp(arg, "выход") || !str_cmp(arg, "exit"))
    {

    }



    send_to_char("Синтаксис: построить <дом/комната> направление.\n\r", ch);

    return;
}

void do_build_mob( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *sm;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];

    if (IS_NPC(ch))
	return;

    //проверяем наличие зодчего. Сначала просто в комнате, потом заменить на проверку - следует ли он.
    LIST_FOREACH(sm, &ch->in_room->people, room_link)
    {
	if (IS_NPC(sm) && IS_SET(sm->act, ACT_AROSH))
	    break;
    }

    if (sm == NULL)
    {
	send_to_char("Сначала пригласи зодчего.\n\r", ch);
	return;
    }

    //проверяем первый аргумент.

    argument = one_argument(argument,arg);

    if (arg[0] == '\0')
    {
	send_to_char("Заселить что?\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "существо") || !str_cmp(arg, "mob"))
    {
	//теперь выясняем, имеет ли чар вообще дом, чтобы приобретать себе моба.
	if(ch->pcdata->vnum_house == 0)
	{
	    send_to_char("У тебя нет дома. Сначала построй его.\n\r", ch);
	    return;
	}

	//проверка на то - дом ли тут?
	if (IS_NULLSTR(ch->in_room->owner))
	{
	    sprintf(buf, "Здесь нет дома.");
	    do_say(sm, buf);
	    return;
	}

	//проверка на владельца комнаты - если комната не его, то запретить строить.
	if (!is_room_owner(ch, ch->in_room))
	{
	    sprintf(buf, "Здесь не твой дом. Таскать существ в чужие дома... упс....");
	    do_say(sm, buf);
	    return;
	}

	//проверка на достаточность qp и золота
	if (ch->pcdata->quest_curr < 500)
	{
	    sprintf(buf, "%s, у тебя нет требуемого количества пунктов удачи.", ch->name);
	    do_say(sm, buf);

	    sprintf(buf, "Построить существо стоит 500 очков удачи.");
	    do_say(sm, buf);
	    return;
	}

	if (ch->gold < 5000)
	{
	    sprintf(buf, "%s, у тебя нет требуемого количества золота.", ch->name);
	    do_say(sm, buf);
	    sprintf(buf, "Построить существо стоит 5000 золота.");
	    do_say(sm, buf);
	    return;
	}

	//а вот далее думать, какие еще проверки... какие еще параметры.

	//построить моба, проставить все флаги.

	//берем плату за постройку моба
	ch->gold -= 5000;
	ch->pcdata->quest_curr -= 500;
	send_to_char("Ты платишь за покупку существа 5000 золота и 500 очков удачи.\n\r", ch);

	sprintf(buf, "Зодчий приводит тебе существо.\n\r");
	send_to_char(buf, ch);
	return;
    }

    //меняем имя (name)
    if (!str_cmp(arg, "имя") || !str_cmp(arg, "name"))
    {

    }

    //меняем внешность (описание)
    if (!str_cmp(arg, "внешность") || !str_cmp(arg, "description"))
    {

    }

    //меняем позицию (sentiel)
    if (!str_cmp(arg, "стоит") || !str_cmp(arg, "stay"))
    {

    }


    send_to_char("Синтаксис: заселить <существо>.\n\r", ch);

    return;
}


void do_build_obj( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *sm;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];

    if (IS_NPC(ch))
	return;

    //проверяем наличие зодчего. Сначала просто в комнате, потом заменить на проверку - следует ли он.
    LIST_FOREACH(sm, &ch->in_room->people, room_link)
    {
	if (IS_NPC(sm) && IS_SET(sm->act, ACT_AROSH))
	    break;
    }

    if (sm == NULL)
    {
	send_to_char("Сначала найди зодчего.\n\r", ch);
	return;
    }

    //проверяем первый аргумент.

    argument = one_argument(argument,arg);

    if (arg[0] == '\0')
    {
	send_to_char("Мебелировать что?\n\r", ch);
	return;
    }

    //предметы.
    if (!str_cmp(arg, "предмет") || !str_cmp(arg, "object"))
    {
	//теперь выясняем, имеет ли чар вообще дом, чтобы приобретать себе объект.
	if(ch->pcdata->vnum_house == 0)
	{
	    send_to_char("У тебя нет дома. Сначала построй его.\n\r", ch);
	    return;
	}

	//проверка на то - дом ли тут?
	if (IS_NULLSTR(ch->in_room->owner))
	{
	    sprintf(buf, "Здесь нет дома.");
	    do_say(sm, buf);
	    return;
	}

	//проверка на владельца комнаты - если комната не его, то запретить строить.
	if (!is_room_owner(ch, ch->in_room))
	{
	    sprintf(buf, "Здесь не твой дом. Ставить объекты в чужие дома... упс....");
	    do_say(sm, buf);
	    return;
	}

	//проверка на достаточность qp и золота
	if (ch->pcdata->quest_curr < 500)
	{
	    sprintf(buf, "%s, у тебя нет требуемого количества пунктов удачи.", ch->name);
	    do_say(sm, buf);

	    sprintf(buf, "Поставить объект стоит 500 очков удачи.");
	    do_say(sm, buf);
	    return;
	}

	if (ch->gold < 5000)
	{
	    sprintf(buf, "%s, у тебя нет требуемого количества золота.", ch->name);
	    do_say(sm, buf);
	    sprintf(buf, "Поставить объект стоит 5000 золота.");
	    do_say(sm, buf);
	    return;
	}

	//а вот далее думать, какие еще проверки... какие еще параметры.


	//построить объект, проставить все флаги.

	//берем плату за установку объекта.
	ch->gold -= 5000;
	ch->pcdata->quest_curr -= 500;
	send_to_char("Ты платишь за покупку объекта 5000 золота и 500 очков удачи.\n\r", ch);

	sprintf(buf, "Зодчий устанавливает тебе объект.\n\r");
	send_to_char(buf, ch);

	return;
    }

    //меняем тип (type)
    if (!str_cmp(arg, "тип") || !str_cmp(arg, "type"))
    {

    }

    //меняем название (name)
    if (!str_cmp(arg, "название") || !str_cmp(arg, "name"))
    {

    }

    //меняем отделка (desc)
    if (!str_cmp(arg, "отделка") || !str_cmp(arg, "description"))
    {

    }

    //поставить проверки остальные (рест, etc)

    send_to_char("Синтаксис: мебелировать <предмет>.\n\r", ch);

    return;
}

/* charset=cp1251 */





