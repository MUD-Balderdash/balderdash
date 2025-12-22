/***************************************************************************
 *          This snippet was written by Donut for the Khrooon Mud.         *
 *            Original Coded by Yago Diaz <yago@cerberus.uab.es>           *
 *        	(C) June 1997	                                           *
 *        	(C) Last Modification October 1997			   *
 ***************************************************************************/
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
 **************************************************************************/
/***************************************************************************
 *       ROM 2.4 is copyright 1993-1996 Russ Taylor                         *
 *       ROM has been brought to you by the ROM consortium                  *
 *           Russ Taylor (rtaylor@efn.org)                                  *
 *           Gabrielle Taylor                                               *
 *           Brian Moore (zump@rom.org)                                     *
 *       By using this code, you have agreed to follow the terms of the     *
 *       ROM license, in the file Rom24/doc/rom.license                     *
 ***************************************************************************/

/****************************************************************************
 * 2018 - КААРВАНЩИК АКА ТРАНСПОРТНЫЙ ДРАКОН
 ***************************************************************************/
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "interp.h"


struct location_type
{
    int		vnum;
    int		cost;
    char	*point;
};

const struct location_type location_table [] =
{
    { 10369, 35, "Кания тау" },
    { 4800, 35, "Железные холмы" },
    { 4859, 35, "Лориэн" },
    { 4460, 35, "Змеиный город" },
    { 4579, 35, "Тролльград" },
    { 10145,35, "Убежище Вампиров" },
    { 8540, 35, "Ссин Верин" },
    { 7501, 35, "Риаталь" },
    { 8008, 35, "Райнак" },
    { 9506, 45, "Новый Талос"}, 
    { 1102, 35, "Шир" },
    { 5200, 35, "Талос" },
    { 3014, 45, "Мидгард" },
    { 5513, 35, "Город оборотней" },
    { 31826, 35, "Лаборатория Аозэра (Город зомби)" },
    { 15001, 50, "Ледяной город" },
    { 17301, 50, "Камелот" },
    { 18919, 50, "Город Совершенства" },
    { 20035, 50, "Город Сокола" },
    { 20320, 50, "Белый город" },
    { 2917, 50, "Город скальных троллей" },
    { 2305, 60, "Махнтор" },
    { 0,  0, NULL }
};


CHAR_DATA *find_summoner(CHAR_DATA *ch)
{
    CHAR_DATA *summoner;

    LIST_FOREACH(summoner, &ch->in_room->people, room_link)
    {
	if (!IS_NPC(summoner))
	    continue;

	if (summoner->spec_fun == spec_lookup("spec_summoner"))
	    return summoner;
    }

    if (summoner == NULL || summoner->spec_fun != spec_lookup("spec_summoner"))
    {
	send_to_char("Ты не можешь здесь это сделать.\n\r",ch);
	return NULL;
    }

    if (summoner->fighting != NULL)
    {
	send_to_char("Подожди, пока закончится драка.\n\r",ch);
	return NULL;
    }

    return NULL;
}


void do_travel(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *summoner;
    char  arg[MAX_STRING_LENGTH];
    int i;

    if ((summoner = find_summoner (ch)) == NULL)
	return;


    argument = one_argument(argument, arg);

    if (arg[0] != '\0')
    {
	if (!strcmp(arg, "list") || !str_prefix(arg, "список"))
	{
	    send_to_char("Возможные пункты назначения:\n\r\n\r",ch);
	    for (i=0;location_table[i].point != NULL;i++)
	    {
		sprintf(arg,"\t%-20s  %d золота\n\r",location_table[i].point,location_table[i].cost);
		send_to_char(arg,ch);
	    }
	    return;
	}

	if (!strcmp(arg, "buy") || !str_prefix(arg,"купить"))
	{

	    one_argument(argument, arg);

	    if (arg[0] == '\0')
	    {
		act("$N говорит тебе: {RТы должен сказать мне, куда ты хочешь пройти.{x", ch, NULL, summoner, TO_CHAR);
		return;
	    }

	    for (i=0;location_table[i].point != NULL;i++)
	    {
		if (is_name(arg, location_table[i].point))
		{
		    ROOM_INDEX_DATA *room;

		    if (ch->gold < location_table[i].cost)
		    {
			act("$N говорит тебе: {RУ тебя нет столько золота.{x", ch, NULL, summoner, TO_CHAR);
			return;
		    }

		    room = get_room_index(location_table[i].vnum);

		    if (ch->pet != NULL && ch->pet->in_room == ch->in_room)
		    {
			char_from_room(ch->pet);
			char_to_room(ch->pet, room, TRUE);
			check_trap(ch->pet);
		    }

		    if (MOUNTED(ch))
		    {
			char_from_room(MOUNTED(ch));
			char_to_room(MOUNTED(ch), room, TRUE);
			check_trap(MOUNTED(ch));
		    }

		    sprintf(arg, "%s декламирует 'хасидсиндсад'\n\rТы окружаешься фиолетовым туманом.\n\r\n\r", capitalize(summoner->short_descr));
		    send_to_char(arg, ch);
		    char_from_room(ch);
		    char_to_room(ch, room, TRUE);
		    ch->gold -= location_table[i].cost;
		    summoner->gold += location_table[i].cost;
		    /*	    do_look (ch, "");*/
		    check_trap(ch);
		    return;
		}
	    }

	    act("$N говорит тебе: {RТакого пункта назначения у меня в списке нет.{x", ch, NULL, summoner, TO_CHAR);
	    return;
	}
    }
    act("$N говорит тебе: Ты должен сказать мне, куда ты хочешь пройти.\n\r\tпутешествие список         показывает возможные пункты назначения.\n\r\tпутешествие купить <имя>    для прохода в выбраный пункт назначения.", ch, NULL, summoner, TO_CHAR);
}

/* charset=cp1251 */
