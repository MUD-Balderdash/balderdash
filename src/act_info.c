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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include "merc.h"
#include "interp.h"
#include "magic.h"
#include "recycle.h"



#define MAX_PROMPT_LENGTH	100

struct where_type
{
    char	*name;
    bool	cover;
};

struct where_type	where_name	[] =
{
    {"<свет>               ", TRUE  },
    {"<одето на палец>     ", FALSE },
    {"<одето на палец>     ", FALSE },
    {"<одето на шею>       ", FALSE },
    {"<одето на шею>       ", FALSE },
    {"<одето на тело>      ", FALSE },
    {"<одето на голову>    ", TRUE  },
    {"<одето на ноги>      ", FALSE },
    {"<обуто на ступни>    ", FALSE },
    {"<одето на кисти>     ", FALSE },
    {"<одето на руки>      ", FALSE },
    {"<одето как щит>      ", FALSE },
    {"<одето вокруг тела>  ", TRUE  },
    {"<одето на пояс>      ", FALSE },
    {"<одето на запястье>  ", FALSE },
    {"<одето на запястье>  ", FALSE },
    {"<оружие>             ", FALSE },
    {"<в руке>             ", FALSE },
    {"<второе оружие>      ", FALSE },
    {"<летает около>       ", TRUE  },
    {"\n\r<сидит на плече>     ", TRUE  },
};


/* for  keeping track of the player count */
int max_on   = 0;
int max_ever = 0;

/*
 * Local functions.
 */
char *	format_obj_to_char	args((OBJ_DATA *obj, CHAR_DATA *ch, bool fShort, bool showCond));
void	show_list_to_char	args((OBJ_DATA *list, CHAR_DATA *ch, bool fShort, bool fShowNothing, bool sacrifice, const int iDefaultAction));
void	show_char_to_char_0	args((CHAR_DATA *victim, CHAR_DATA *ch));
void	show_char_to_char_1	args((CHAR_DATA *victim, CHAR_DATA *ch));
void	show_char_to_char	args((CHAR_DATA *list, CHAR_DATA *ch));
char * 	show_condition		args((OBJ_DATA *obj));
bool    can_sacrifice		args((OBJ_DATA *obj, CHAR_DATA *ch, bool show));
char *get_material(OBJ_DATA *obj);


/* from olc.h */
AREA_DATA *get_area_data	args((int vnum));

void do_shortestpath(CHAR_DATA *ch, char *argument)
{
    ROOM_INDEX_DATA *pRoomIndex, *queueIn, *queueOut, *source, *destination = NULL;
    extern ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];
    int iHash;
    AREA_DATA *pArea = NULL;
    EXIT_DATA *pexit, *pexit2;
    int door, door2;
    const char *shortdir_name[] = {"n", "e", "s", "w", "u", "d"};
    int fArea = 0; /* fArea is 0 for no area, nonzero for area vnum */
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];

#ifdef DEBUG
    Debug ("do_shortestpath");
#endif

    /* Use a breadth-first search to find shortest path. */
    if (argument[0] == '\0')
    {
	send_to_char("Синтаксис: путь <игрок, моб или vnum>\n\r", ch);
	send_to_char("           путь зона <area vnum>\n\r", ch);
	return;
    }

    /* First, find source and destination rooms: */
    if ((source = ch->in_room) == NULL)
    {
	send_to_char("Ты должен быть где-то, чтобы пойти куда-то.\n\r", ch);
	return;
    }

    /* Check if we're looking for area vnum */
    if (!str_prefix("area ", argument) || !str_prefix("зона ", argument))
    {
	argument = one_argument(argument, buf); /* strip "area" and discard buf */

	if (is_number(argument))
	{
	    fArea = atoi(argument);

	    if (!(pArea = get_area_data(fArea)))
	    {
		send_to_char("That {Garea vnum{x does not exist.\n\r", ch);
		return;
	    }
	    fArea++; /* since vnums start at 0 this makes it nonzero. */
	}
    }
    else if ((destination = find_location(ch, argument)) == NULL)
    {
	send_to_char("Нет такого пункта назначения.\n\r", ch);
	return;
    }

    if ((fArea && source->area == pArea) || source == destination)
    {
	send_to_char("А зачем куда-то идти?\n\r", ch);
	return;
    }

    /* Initialize: set distance of every room to a very large number, null links and queue */
    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
	for (pRoomIndex  = room_index_hash[iHash];
	     pRoomIndex != NULL;
	     pRoomIndex  = pRoomIndex->next)
	{
	    pRoomIndex->distance_from_source = 32765;
	    pRoomIndex->shortest_from_room = NULL;
	    pRoomIndex->shortest_next_room = NULL;
	}
    }

    /* Now set source distance to 0 and put it on the "search" queue */
    source->distance_from_source = 0;
    queueIn = source;    

    /* Now, set distance for all adjacent rooms to search room + 1 until destination. */
    /* If destination not found, put each unsearched adjacent room on queue and repeat */
    for (queueOut = source; queueOut; queueOut = queueOut->shortest_next_room)
    {
	/* for each exit to search room: */
	for (door = 0; door < MAX_DIR ; door++)
	{
	    if ((pexit = queueOut->exit[door]) != NULL
		&& pexit->u1.to_room != NULL)
	    {
		/*if we haven't looked here, set distance and add to search list */
		if (pexit->u1.to_room->distance_from_source > queueOut->distance_from_source + 1)
		{
		    pexit->u1.to_room->distance_from_source = queueOut->distance_from_source + 1;

		    /* if we've found destination, we're done! */
		    if ((fArea && pexit->u1.to_room->area == pArea)
			|| pexit->u1.to_room == destination)
		    {
			int count = 1;
			char buf3[3];

			/* print the directions in reverse order as we walk back */
			sprintf(buf3, "%s", shortdir_name[door]);
			sprintf(buf2, "%s", buf3);
			for (pRoomIndex = queueOut;
			     pRoomIndex->shortest_from_room;
			     pRoomIndex = pRoomIndex->shortest_from_room)
			{
			    for (door2 = 0; door2 < MAX_DIR ; door2++)
			    {
				if ((pexit2 = pRoomIndex->shortest_from_room->exit[door2]) != NULL
				    && pexit2->u1.to_room == pRoomIndex)
				{
				    if (!str_cmp(shortdir_name[door2], buf3))
				    {
					count++;
				    }
				    else
				    {
					sprintf(buf, "%s", buf2);
					if (count > 1)
					{
					    sprintf(buf2, "%s%d%s", shortdir_name[door2], count, buf);
					}
					else
					{
					    sprintf(buf2, "%s%s", shortdir_name[door2], buf);
					}
					count = 1;
					sprintf(buf3, "%s", shortdir_name[door2]);
				    }
				}
			    }
			}

			if (count > 1)
			{
			    sprintf(buf, "%s", buf2);
			    sprintf(buf2, "%d%s", count, buf);
			}

			if (fArea)
			{
			    sprintf(buf, "Самый короткий путь до %s из %s - %d шагов:\n\r %s.\n\r",
				    pArea->name, source->name,
				    pexit->u1.to_room->distance_from_source, buf2);
			}
			else
			{
			    sprintf(buf, "Самый короткий путь - %d шагов:\n\r %s\n\r",
				    pexit->u1.to_room->distance_from_source, buf2);
			}
			send_to_char(buf, ch);
			return;
		    }
		    /*Didn't find destination, add to queue */
		    pexit->u1.to_room->shortest_from_room = queueOut;
		    queueIn->shortest_next_room = pexit->u1.to_room;
		    queueIn = pexit->u1.to_room;
		}
	    }
	}
    }
    send_to_char("Туда не пройти.\n\r", ch);
    return;
}



char *format_obj_to_char(OBJ_DATA *obj, CHAR_DATA *ch, bool fShort, bool showCond)
{
    static char buf[MAX_STRING_LENGTH];

    buf[0] = '\0';

    if ((fShort && (obj->short_descr == NULL || obj->short_descr[0] == '\0'))
		|| obj->description == NULL	|| obj->description[0] == '\0') {
		return buf;
    }

    if (IS_OBJ_STAT(obj, ITEM_INVIS)){
		strcat(buf, "({DНевид{x) ");
	}

	if (IS_AFFECTED(ch, AFF_DETECT_EVIL) && IS_OBJ_STAT(obj, ITEM_EVIL)) {
    	strcat(buf, "({RКрасная Аура{x) ");
    }
    if (IS_AFFECTED(ch, AFF_DETECT_GOOD) && IS_OBJ_STAT(obj, ITEM_BLESS)){
		strcat(buf,  "({BСиняя аура{x) ");
    }

	if (IS_AFFECTED(ch, AFF_DETECT_MAGIC) && IS_OBJ_STAT(obj, ITEM_MAGIC)){
		strcat(buf, "({GМагическое{x) ");
    }

    if (IS_OBJ_STAT(obj, ITEM_GLOW)){
		strcat(buf, "({MСветится{x) ");
    }

    if (IS_OBJ_STAT(obj, ITEM_HUM)){
		strcat(buf, "({CИздает звук{x) ");
    }

	if (obj->item_type == ITEM_ARTIFACT){
		strcat(buf, "({WАртефакт{x) ");
    }
    if (obj->item_type == ITEM_INGREDIENT){
		strcat(buf, "({YИнгредиент{x) ");
    }

	// Цвет вещей по умолчанию темно зеленый
    strcat(buf, "{g");

    if (fShort){
	if (obj->short_descr != NULL)
	    strcat(buf, obj->short_descr);
    } else {
	if (obj->description != NULL)
	    strcat(buf, obj->description);
    }

    strcat(buf, "{x");

	// Лежит здесь, весит в воздухе и прочее состояния/положения вещи
    if (showCond){
		// Valgrind: Overlapping source and destination blocks
		sprintf(buf, "%s (%s)", buf, show_condition(obj));
	}

    return buf;
}



/*
 * Show a list to a character.
 * Can coalesce duplicated items.
 */
void show_list_to_char(OBJ_DATA *list, CHAR_DATA *ch, bool fShort, bool fShowNothing, bool sacrifice, const int iDefaultAction)
{
    char buf[MAX_STRING_LENGTH];
    BUFFER *output;
    char **prgpstrShow;
    int *prgnShow;
    char *pstrShow;
    OBJ_DATA *obj;
    int nShow;
    int iShow;
    int count;
    bool fCombine;
    char **prgpstrName;        /* for MXP */
    char **prgpstrShortName;   /* for MXP */
    char *pstrName;            /* for MXP */
    char *pstrShortName;       /* for MXP */
    char * pAction = NULL;

    if (ch->desc == NULL)
	return;
    /* work out which MXP tag to use */

    switch (iDefaultAction)
    {
    case eItemGet:  pAction = "Get"; break;   /* item on ground */
    case eItemDrop: pAction = "Drop"; break;   /* item in inventory */
    } /* end of switch on action */


    /*
     * Alloc space for output lines.
     */
    output = new_buf();

    count = 0;
    for (obj = list; obj != NULL; obj = obj->next_content)
	count++;
    prgpstrShow	= alloc_mem(count * sizeof(char *));
    prgnShow    = alloc_mem(count * sizeof(int)   );
    prgpstrName = alloc_mem(count * sizeof(char *));
    prgpstrShortName = alloc_mem(count * sizeof(char *));

    nShow	= 0;

    /*
     * Format the list of objects.
     */
    for (obj = list; obj != NULL; obj = obj->next_content)
    {
	if (obj->wear_loc == WEAR_NONE && can_see_obj(ch, obj)
	    && (!sacrifice || can_sacrifice(obj, ch, FALSE)))
	{
	    pstrShow = format_obj_to_char(obj, ch, fShort, FALSE);

	    pstrName = obj->name;
	    pstrShortName = obj->short_descr;

	    fCombine = FALSE;

	    /*	    if (IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE))  */
	    {
		/*
		 * Look for duplicates, case sensitive.
		 * Matches tend to be near end so run loop backwords.
		 */
		for (iShow = nShow - 1; iShow >= 0; iShow--)
		{
		    if (!strcmp(prgpstrShow[iShow], pstrShow))
		    {
			prgnShow[iShow]++;
			fCombine = TRUE;
			break;
		    }
		}
	    }

	    /*
	     * Couldn't combine, or didn't want to.
	     */
	    if (!fCombine)
	    {
		prgpstrShow [nShow] = str_dup(pstrShow);
		prgpstrName [nShow] = str_dup(pstrName);
		prgpstrShortName [nShow] = str_dup(pstrShortName);
		prgnShow    [nShow] = 1;
		nShow++;
	    }

	    if (nShow > 100)
		break;
	}
    }

    /*
     * Output the formatted list.
     */
    for (iShow = 0; iShow < nShow; iShow++)
    {
	if (prgpstrShow[iShow][0] != '\0')
	{
	    if (prgnShow[iShow] != 1)
	    {
		sprintf(buf, "{x(%2d) ", prgnShow[iShow]);
		add_buf(output, buf);
	    }
	    else
	    {
		add_buf(output, "     ");
	    }
	    if (pAction)
	    {
		int i = 0;
		char * p = prgpstrName[iShow];
		for (; *p && !isspace (*p); p++, i++)
		    ;
		sprintf (buf, MXPTAG ("%s '%.*s' '%s'"), pAction, i, prgpstrName[iShow], prgpstrShortName[iShow]);
		add_buf(output, buf);
	    }

	    add_buf(output, prgpstrShow[iShow]);

	    if (pAction)
	    {
		sprintf (buf, MXPTAG ("/%s"), pAction);
		add_buf(output, buf);
	    }

	    add_buf(output, "\n\r");
	}
	free_string(prgpstrShow[iShow]);
	free_string(prgpstrName[iShow]);
	free_string(prgpstrShortName[iShow]);
    }

    if (fShowNothing && nShow == 0)
	send_to_char("     Ничего.\n\r", ch);

    if (nShow >= 100)
	add_buf(output, "В этой куче не удается разглядеть все...\n\r");

    page_to_char(buf_string(output), ch);

    /*
     * Clean up.
     */
    free_buf(output);
    free_mem(prgpstrShow, count * sizeof(char *));
    free_mem(prgnShow,    count * sizeof(int)   );
    free_mem(prgpstrName, count * sizeof(char *));
    free_mem(prgpstrShortName, count * sizeof(char *));

    return;
}



void show_char_to_char_0(CHAR_DATA *victim, CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH], message[MAX_STRING_LENGTH];
    buf[0] = '\0';

    if (RIDDEN(victim))
    {
	if (ch != RIDDEN(victim))
	    sprintf(buf, "(Оседлан%s) ", SEX_ENDING(victim));
	else
	    strcat(buf, "(Твоя лошадь) ");
    }

    if (IS_SET(victim->comm, COMM_AFK))
	strcat(buf, "[ВОК] ");

    if (IS_AFFECTED(victim, AFF_INVISIBLE))
	strcat(buf, "({DНевид{x) ");

    if (is_affected(victim,gsn_dissolve))
	strcat(buf, "({cРаствор{x) ");
//	sprintf(buf, "({cРастворен%s{x) ", SEX_ENDING(victim));

    if (victim->invis_level >= LEVEL_HERO)
	strcat(buf, "({BWizinvis{x) ");

    if (IS_AFFECTED(victim, AFF_HIDE))
    {
	strcat(buf, "({DСкрыт");
	strcat(buf, SEX_ENDING(victim));
	strcat(buf, "{x) ");
    }

    if (IS_AFFECTED(victim, AFF_CAMOUFLAGE))
	strcat(buf, "({DКамуфляж{x) ");

	if (IS_AFFECTED(victim, AFF_CHARM))
	{
		if (victim->leader == ch)
		{
			strcat(buf, "(Твое очаро) ");
		}
		else
		{
			strcat(buf, "(Очаро) ");
		}
	}

    if (IS_AFFECTED(victim, AFF_PASS_DOOR))
	strcat(buf, "({CПрозр{x) ");

    if (IS_AFFECTED(victim, AFF_FAERIE_FIRE))
	strcat(buf, "({MРозовая Аура{x) ");

    if (IS_EVIL(victim)
	&& IS_AFFECTED(ch, AFF_DETECT_EVIL))
    {
	strcat(buf, "({RКрасная Аура{x) ");
    }

    if (IS_NPC(victim) && IS_SET(victim->act, ACT_UNDEAD)
	&& IS_AFFECTED(ch, AFF_DETECT_UNDEAD))
    {
	strcat(buf, "({rМертв");
	strcat(buf, SEX_ENDING(victim));
	strcat(buf, "{x) ");
    }

    if (IS_GOOD(victim)
	&& IS_AFFECTED(ch, AFF_DETECT_GOOD))
    {
	strcat(buf, "({YЗолотая Аура{x) ");
    }

    if (victim->count_holy_attacks >0)
	strcat(buf, "({MЕретик{x) ");

    if (victim->count_guild_attacks >0)
	strcat(buf, "({MБратоубийца{x) ");

    if (IS_AFFECTED(victim, AFF_SANCTUARY))
	strcat(buf, "({WБелая Аура{x) ");

    if (is_affected(victim, gsn_ghostaura))
	strcat(buf, "({CПризрачная Аура{x) ");

    if (IS_HELPER(victim))
	strcat(buf, "{B(Помощник){x ");

    if (!IS_NPC(victim) && IS_SET(victim->act, PLR_KILLER))
	strcat(buf, "({RУбийца{x) ");

    if (!IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF ))
	strcat(buf, "({RВор{x) ");

    if (is_affected(ch, skill_lookup("detect animal"))
	&& IS_NPC(victim)
	&& is_animal(victim))
    {
	strcat(buf, "({gЖивотное{x) ");
    }

    if (!IS_NPC(victim) && is_affected(victim, gsn_gods_curse))
    {
	strcat(buf, "({rПроклят");
	strcat(buf, SEX_ENDING(victim));
	strcat(buf, "{x) ");
    }

    if (victim->position == victim->start_pos && victim->long_descr[0] != '\0')
    {
	strcat(buf, "{y");
	strcat(buf, victim->long_descr);
	strcat(buf, "{x");
	send_to_char(buf, ch);
	return;
    }

    strcat(buf, "{y");
    if (!IS_NPC(victim) && !IS_NULLSTR(victim->pcdata->pretitle))
    {
	strcat(buf, victim->pcdata->pretitle);
	strcat(buf, " ");
    }

    sprintf(message, "%s", PERS(victim, ch, 0));
    message[0] = UPPER(message[0]);
    strcat(buf, message);


    if (!IS_NPC(victim)
	&& !IS_SET(ch->comm, COMM_BRIEF)
	&& victim->position == POS_STANDING
	&& ch->on == NULL)
    {
	strcat(buf, victim->pcdata->title);
    }

    strcat(buf, "{y");

    switch (victim->position)
    {
    case POS_DEAD:
	sprintf(buf, "%s мертв%s!!", buf, SEX_ENDING(victim));
	break;

    case POS_MORTAL:
	sprintf(buf, "%s смертельно ранен%s.", buf, SEX_ENDING(victim));
	break;

    case POS_INCAP:
	strcat(buf, " не может пошевелиться.");
	break;

    case POS_STUNNED:
	sprintf(buf, "%s оглушен%s.", buf, SEX_ENDING(victim));
	break;

    case POS_SLEEPING:
	if (victim->on != NULL)
	{
	    if (IS_SET(victim->on->value[2], SLEEP_AT))
	    {
		sprintf(message, " спит возле %s.",
			can_see_obj(ch, victim->on) ? cases(victim->on->short_descr, 1) : "чего-то");
		strcat(buf, message);
	    }
	    else if (IS_SET(victim->on->value[2], SLEEP_IN))
	    {
		sprintf(message, " спит в %s.", cases(victim->on->short_descr, 5));
		strcat(buf, message);
	    }
	    else
	    {
		sprintf(message, " спит на %s.",
			can_see_obj(ch, victim->on) ? cases(victim->on->short_descr, 5) : "чем-то");
		strcat(buf, message);
	    }
	}
	else
	    strcat(buf, " спит здесь.");
	break;

    case POS_RESTING:
	if (victim->on != NULL)
	{
	    if (IS_SET(victim->on->value[2], REST_AT))
	    {
		sprintf(message, " отдыхает возле %s.",
			can_see_obj(ch, victim->on) ? cases(victim->on->short_descr, 1) : "чего-то");
		strcat(buf, message);
	    }
	    else if (IS_SET(victim->on->value[2], REST_IN))
	    {
		sprintf(message, " отдыхает в %s.",
			cases(victim->on->short_descr, 5));
		strcat(buf, message);
	    }
	    else
	    {
		sprintf(message, " отдыхает на %s.",
			can_see_obj(ch, victim->on) ? cases(victim->on->short_descr, 5) : "чем-то");
		strcat(buf, message);
	    }
	}
	else
	    strcat(buf, " отдыхает здесь.");
	break;

    case POS_SITTING:
	if (victim->on != NULL)
	{
	    if (IS_SET(victim->on->value[2], SIT_AT))
	    {
		sprintf(message, " сидит возле %s.",
			can_see_obj(ch, victim->on) ? cases(victim->on->short_descr, 1) : "чего-то");
		strcat(buf, message);
	    }
	    else if (IS_SET(victim->on->value[2], SIT_IN))
	    {
		sprintf(message, " сидит в %s.",
			cases(victim->on->short_descr, 5));
		strcat(buf, message);
	    }
	    else
	    {
		sprintf(message, " сидит на %s.",
			can_see_obj(ch, victim->on) ? cases(victim->on->short_descr, 5) : "чем-то");
		strcat(buf, message);
	    }
	}
	else
	    strcat(buf, " сидит здесь.");
	break;

    case POS_STANDING:
	if (victim->on != NULL)
	{
	    if (IS_SET(victim->on->value[2], STAND_AT))
	    {
		sprintf(message, " стоит около %s.",
			can_see_obj(ch, victim->on) ? cases(victim->on->short_descr, 1) : "чего-то");
		strcat(buf, message);
	    }
	    else if (IS_SET(victim->on->value[2], STAND_ON))
	    {
		sprintf(message, " стоит на %s.",
			can_see_obj(ch, victim->on) ? cases(victim->on->short_descr, 5) : "чем-то");
		strcat(buf, message);
	    }
	    else
	    {
		sprintf(message, " стоит в %s.",
			can_see_obj(ch, victim->on) ? cases(victim->on->short_descr, 5) : "чем-то");
		strcat(buf, message);
	    }
	}
	else if (MOUNTED(victim))
	{
	    strcat(buf, " здесь, верхом на ");
	    strcat(buf, PERS(MOUNTED(victim), ch, 5));
	    strcat(buf, ".");
	}
	else if (IS_AFFECTED(victim, AFF_FLYING))
	{
	    strcat(buf, " парит здесь в воздухе.");
	}
	else
	{
	    strcat(buf, " стоит здесь.");
	}
	break;

    case POS_FIGHTING:
	strcat(buf, " сражается здесь с ");

	if (victim->fighting == NULL)
	    strcat(buf, ".... пустотой?");
	else if (victim->fighting == ch)
	    strcat(buf, "ТОБОЙ!");
	else if (victim->in_room == victim->fighting->in_room)
	{
	    strcat(buf, PERS(victim->fighting, ch, 4));
	    strcat(buf, ".");
	}
	else
	    strcat(buf, "кем-то, кто уже ушел?");
	break;

    case POS_BASHED:
	strcat(buf, " сидя сражается с ");

	if (victim->fighting == NULL)
	    strcat(buf, ".... пустотой?");
	else if (victim->fighting == ch)
	    strcat(buf, "ТОБОЙ!");
	else if (victim->in_room == victim->fighting->in_room)
	{
	    strcat(buf, PERS(victim->fighting, ch, 4));
	    strcat(buf, ".");
	}
	else
	    strcat(buf, "кем-то, кто уже ушел?");
	break;
    }

    strcat(buf, "{x\n\r");
    buf[0] = UPPER(buf[0]);
    send_to_char(buf, ch);
    return;
}



void show_char_to_char_1(CHAR_DATA *victim, CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    int iWear;
    int percent;
    bool found;

    if (can_see(victim, ch))
    {
	if (ch == victim)
	    act("$n смотрит на себя.", ch, NULL, NULL, TO_ROOM);
	else
	{
	    act("$n смотрит на тебя.", ch, NULL, victim, TO_VICT   );
	    act("$n смотрит на $N3.",  ch, NULL, victim, TO_NOTVICT);
	}
    }

    if (IS_IMMORTAL(ch) && IS_NPC(victim))
    {
	sprintf(buf, "{R{r[{RMob %d{r]{x\n\r", victim->pIndexData->vnum);
	send_to_char(buf, ch);
    }

    if (victim->description[0] != '\0')
    {
	tagline_to_char(victim->description, victim, ch);
    }
    else
    {
	sprintf(buf, "Выглядит как обыкновенн%s %s.\n\r",
		victim->sex == SEX_FEMALE ? "ая" : "ый",
		victim->sex == SEX_FEMALE
		? race_table[victim->race].fname
		: race_table[victim->race].rname);
	send_to_char(buf, ch);
    }

    if (MOUNTED(victim))
    {
	sprintf(buf, "%s верхом на %s.\n\r",
		victim->name, PERS(MOUNTED(victim), ch, 5));
	send_to_char(buf, ch);
    }

    if (RIDDEN(victim))
    {
	sprintf(buf, "%s оседлан%s %s.\n\r",
		capitalize(victim->short_descr),
		SEX_ENDING(victim),
		PERS(RIDDEN(victim), ch, 4));
	send_to_char(buf, ch);
    }

    if (victim->max_hit > 0)
	percent = (100 * victim->hit) / victim->max_hit;
    else
	percent = -1;

    strcpy(buf, PERS(victim, ch, 0));

    if (percent >= 100)
	strcat(buf, " {Gв прекрасном состоянии");
    else if (percent >= 90)
	sprintf(buf, "%s {gслегка поцарапан%s", buf, SEX_ENDING(victim));
    else if (percent >= 75)
	strcat(buf, " {Cимеет несколько ран");
    else if (percent >=  50)
	sprintf(buf, "%s {cранен%s", buf, SEX_ENDING(victim));
    else if (percent >= 30)
	sprintf(buf, "%s {Yсерьезно ранен%s", buf, SEX_ENDING(victim));
    else if (percent >= 15)
	strcat (buf, " {yвыглядит очень плохо");
    else if (percent >= 0)
	strcat (buf, " {rв ужасном состоянии");
    else
	sprintf(buf, "%s {rпрактически мертв%s", buf, SEX_ENDING(victim));

    strcat(buf, "{x.\n\r");

    buf[0] = UPPER(buf[0]);
    send_to_char(buf, ch);

    found = FALSE;
    for (iWear = 0; iWear < MAX_WEAR; iWear++)
    {
	if ((obj = get_eq_char(victim, iWear)) != NULL
	    && can_see_obj(ch, obj))
	{
	    if (!found)
	    {
		OBJ_DATA *about = NULL;

		send_to_char("\n\r", ch);
		if (IS_AFFECTED(victim, AFF_COVER) && (about = get_eq_char(victim, WEAR_ABOUT)) != NULL)
		    sprintf(buf, "$N (укутан%s в $p6) использует:", SEX_ENDING(victim));
		else
		    strcpy(buf, "$N использует:");

		act(buf, ch, about, victim, TO_CHAR);

		found = TRUE;
	    }

	    if (is_lycanthrope(victim) && iWear != WEAR_WIELD && iWear != WEAR_SECONDARY)
		continue;

	    if (!IS_AFFECTED(victim, AFF_COVER)
		|| where_name[iWear].cover
		|| IS_IMMORTAL(ch))
	    {
		send_to_char(where_name[iWear].name, ch);
		send_to_char(format_obj_to_char(obj, ch, TRUE, FALSE), ch);
		send_to_char("\n\r", ch);
	    }
	}
    }

    if (victim != ch
	&& !IS_NPC(ch)
	&& number_percent() < get_skill(ch, gsn_peek)
	&& get_skill(victim, gsn_unpeek) < 100)
    {

	if (get_skill(victim, gsn_unpeek) <= 1
	    || get_skill(victim, gsn_unpeek) + number_range(0, 20)
	    < get_skill(ch, gsn_peek) + number_range(0, 20))
	{
	    send_to_char("\n\rЧто с собой несет:\n\r", ch);
	    check_improve(ch, victim, gsn_peek, TRUE, 5);
	    show_list_to_char(victim->carrying,
			      ch, TRUE, TRUE, FALSE, eItemNothing);

	    if (ch->classid == CLASS_THIEF
		&& number_percent() < get_skill(ch, gsn_peek)/2)
	    {
		int money = victim->gold + victim->silver/100;

		send_to_char("\n\rСодержимое кошелька:\n\r     ", ch);
		if (money > 1000)
		{
		    send_to_char("Очень толстый кошелек, там больше "
				 "1000 золотых монет!\n\r", ch);
		}
		else if (money > 500)
		{
		    send_to_char("Довольно толстый кошелек, там больше "
				 "500 золотых монет.\n\r", ch);
		}
		else if (money > 100)
		{
		    send_to_char("Толстенький кошелек, там больше "
				 "100 золотых монет.\n\r", ch);
		}
		else if (money > 50)
		{
		    send_to_char("Средненький кошелек, там более "
				 "50 золотых монет.\n\r", ch);
		}
		else if (money > 10)
		{
		    send_to_char("Так себе кошелек, там немногим более "
				 "10 золотых монет.\n\r", ch);
		}
		else if (money > 1)
		{
		    send_to_char("Тощенький кошелек, там менее "
				 "10 золотых монет.\n\r", ch);
		}
		else
		    send_to_char("Кошелек практически пуст.\n\r", ch);
	    }
	}
	else
	{
	    check_improve(victim, ch, gsn_unpeek, TRUE, 5);
	}
    }

    return;
}


void show_char_to_char(CHAR_DATA *list, CHAR_DATA *ch)
{
    CHAR_DATA *rch, *rch_next;
    int nShow = 0, life_count = 0;

    for (rch = list; rch != NULL; rch = rch_next)
    {
	rch_next = LIST_NEXT(rch, room_link);

	if (rch == ch
	    || (RIDDEN(rch)
		&& rch->in_room == RIDDEN(rch)->in_room
		&& RIDDEN(rch) != ch))
	{
	    continue;
	}

	if (can_see(ch, rch))
	{
	    show_char_to_char_0(rch, ch);
	    nShow++;

	    if (MOUNTED(rch) && can_see(ch, MOUNTED(rch))
		&& rch->in_room == MOUNTED(rch)->in_room)
	    {
		show_char_to_char_0(MOUNTED(rch), ch);
		nShow++;
	    }
	}
	else
	{
	    if (IS_IMMORTAL(rch) || IS_WIZINVISMOB(rch))
	        continue;

	    life_count++;

	    if (room_is_dark(ch->in_room) && IS_AFFECTED(ch, AFF_INFRARED)
		&& SUPPRESS_OUTPUT(check_blind(ch)))
	    {
		if (IS_AWAKE(rch))
		    send_to_char("{rКрасные мигающие глаза наблюдают за "
				 "тобой!{x\n\r", ch);
		else
		    send_to_char("{rЧто-то или кто-то здесь есть!{x\n\r", ch);

		nShow++;
	    }
	}
	if (nShow > 100)
	{
	    send_to_char("\n\rВ этой толпе невозможно разглядеть всех...\n\r",
			 ch);
	    break;
	}
    }

    if (life_count > 0 && IS_AFFECTED(ch, AFF_DETECT_LIFE))
    {
	char buf[MAX_STRING_LENGTH];

	sprintf(buf, "\n\rТы чувствуешь, что здесь, кроме тебя, есть "
		"еще %d %s",
		life_count, hours(life_count, TYPE_SOMETHING));
	send_to_char(buf, ch);

	sprintf(buf, " %s жизни.\n\r", hours(life_count, TYPE_FORMS));
	send_to_char(buf, ch);
    }

    return;
}



bool check_blind(CHAR_DATA *ch)
{
    bool evil = FALSE;
    CHAR_DATA *rch, *rch_next;

    if (!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT))
	return TRUE;
// Молитва только у добрых, так что проверку rch=ch не ставил
    if (IS_AFFECTED(ch, AFF_BLIND))
    {
    	for (rch = ch; rch != NULL; rch = rch_next)
    	{
		rch_next = LIST_NEXT(rch, room_link);

		if (IS_EVIL(rch) || IS_UNDEAD(rch))
			evil = TRUE;
	}
        	
	if (is_affected(ch, gsn_prayer) && evil)
		send_to_char("Ты не видишь ничего, кроме...\n\r", ch);
	else
		send_to_char("Ты ничего не видишь!\n\r", ch);
	return FALSE;
    }

    return TRUE;
}

/* changes your scroll */
void do_scroll(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    int lines;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	if (ch->lines == 0)
	    send_to_char("Режим Paging выключен.\n\r", ch);
	else
	{
	    sprintf(buf, "Сейчас ты видишь %d строк на страницу.\n\r",
		    ch->lines + 2);
	    send_to_char(buf, ch);
	}
	return;
    }

    if (!is_number(arg))
    {
	send_to_char("Ты должен ввести число.\n\r", ch);
	return;
    }

    lines = atoi(arg);

    if (lines == 0)
    {
	send_to_char("Режим Paging выключен.\n\r", ch);
	ch->lines = 0;
	return;
    }

    if (lines < 10 || lines > 100)
    {
	send_to_char("Ты должен ввести разумное число строк.\n\r", ch);
	return;
    }

    sprintf(buf, "Прокрутка установлена в %d строк.\n\r", lines);
    send_to_char(buf, ch);
    ch->lines = lines - 2;
}


/* RT Commands to replace news, motd, imotd, etc from ROM */

void do_motd(CHAR_DATA *ch, char *argument)
{
    do_function(ch, &do_help, "motd");
}

void do_imotd(CHAR_DATA *ch, char *argument)
{
    do_function(ch, &do_help, "imotd");
}

void do_rules(CHAR_DATA *ch, char *argument)
{
    do_function(ch, &do_help, "rules");
}

void do_story(CHAR_DATA *ch, char *argument)
{
    do_function(ch, &do_help, "story");
}

void do_wizlist(CHAR_DATA *ch, char *argument)
{
    do_function(ch, &do_help, "wizlist");
}

/* RT this following section holds all the auto commands from ROM, as well as
 replacements for config */

void do_autolist(CHAR_DATA *ch, char *argument)
{
    char on[6], off[6];
    /* lists most player flags */
    if (IS_NPC(ch))
	return;

    strcpy(on,  "[+]\n\r");
    strcpy(off, "[ ]\n\r");

    send_to_char("  Действие    Статус\n\r", ch);
    send_to_char("----------------------\n\r", ch);

    send_to_char("Автопомощь     ", ch);
    if (IS_SET(ch->act, PLR_AUTOASSIST))
	send_to_char(on, ch);
    else
	send_to_char(off, ch);

    send_to_char("Автовыходы     ", ch);
    if (IS_SET(ch->act, PLR_AUTOEXIT))
    {
	send_to_char(on, ch);
	send_to_char("  В виде карты ", ch);
	if (IS_SET(ch->act, PLR_MAPEXIT))
	    send_to_char(on, ch);
	else
	    send_to_char(off, ch);
    }
    else
	send_to_char(off, ch);

    send_to_char("Автомонеты     ", ch);
    if (IS_SET(ch->act, PLR_AUTOMONEY))
	send_to_char(on, ch);
    else
	send_to_char(off, ch);

    send_to_char("Автозолото     ", ch);
    if (IS_SET(ch->act, PLR_AUTOGOLD))
	send_to_char(on, ch);
    else
	send_to_char(off, ch);

    send_to_char("Автограбеж     ", ch);
    if (IS_SET(ch->act, PLR_AUTOLOOT))
	send_to_char(on, ch);
    else
	send_to_char(off, ch);

    send_to_char("Автожертва     ", ch);
    if (IS_SET(ch->act, PLR_AUTOSAC))
	send_to_char(on, ch);
    else
	send_to_char(off, ch);

    send_to_char("Автодележ      ", ch);
    if (IS_SET(ch->act, PLR_AUTOSPLIT))
	send_to_char(on, ch);
    else
	send_to_char(off, ch);

    send_to_char("Автотитул      ", ch);
    if (IS_SET(ch->act, PLR_AUTOTITLE))
	send_to_char(on, ch);
    else
	send_to_char(off, ch);

    send_to_char("Сны            ", ch);
    if (!IS_SET(ch->act, PLR_NODREAMS))
	send_to_char(on, ch);
    else
	send_to_char(off, ch);

    send_to_char("Отмена         ", ch);
    if (!IS_SET(ch->act, PLR_NOCANCEL))
	send_to_char(on, ch);
    else
	send_to_char(off, ch);


    send_to_char("Режим Компакт  ", ch);
    if (IS_SET(ch->comm, COMM_COMPACT))
	send_to_char(on, ch);
    else
	send_to_char(off, ch);

    send_to_char("Подсказка      ", ch);
    if (IS_SET(ch->comm, COMM_PROMPT))
	send_to_char(on, ch);
    else
	send_to_char(off, ch);

    send_to_char("Квест          ", ch);
    if (IS_SET(ch->act, PLR_NOIMMQUEST))
	send_to_char(off, ch);
    else
	send_to_char(on, ch);

    send_to_char("Атаки          ", ch);
    if (IS_SET(ch->act, PLR_AUTOATTACK))
	send_to_char(on, ch);
    else
	send_to_char(off, ch);


    /*    send_to_char("combine items  ", ch);
     if (IS_SET(ch->comm, COMM_COMBINE))
     send_to_char(on, ch);
     else
     send_to_char(off, ch);

     if (!IS_SET(ch->act, PLR_CANLOOT))
     send_to_char("Your corpse is safe from thieves.\n\r", ch);
     else
     send_to_char("Your corpse may be looted.\n\r", ch);

     if (IS_SET(ch->act, PLR_NOSUMMON))
     send_to_char("You cannot be summoned.\n\r", ch);
     else
     send_to_char("You can be summoned.\n\r", ch); */

    if (IS_SET(ch->act, PLR_NOFOLLOW))
	send_to_char("Ты не принимаешь последователей.\n\r", ch);
    else
	send_to_char("За тобой могут следовать.\n\r", ch);
}

void do_autoassist(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->act, PLR_AUTOASSIST))
    {
	send_to_char("Автопомощь выключена.\n\r", ch);
	REMOVE_BIT(ch->act, PLR_AUTOASSIST);
    }
    else
    {
	send_to_char("Ты будешь помогать, когда требуется.\n\r", ch);
	SET_BIT(ch->act, PLR_AUTOASSIST);
    }
}

void do_nocancel(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->act, PLR_NOCANCEL))
    {
	send_to_char("Теперь на тебя можно колдовать отмену.\n\r", ch);
	REMOVE_BIT(ch->act, PLR_NOCANCEL);
    }
    else
    {
	send_to_char("Теперь на тебя нельзя будет колдовать отмену.\n\r", ch);
	SET_BIT(ch->act, PLR_NOCANCEL);
    }
}

void do_autoexit(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];

    if (IS_NPC(ch))
	return;

    one_argument(argument, arg);

    if (arg[0] != '\0'
	&& (!str_prefix(arg, "карта") || !str_prefix(arg, "map")))
    {
	if (IS_SET(ch->act, PLR_MAPEXIT))
	{
	    send_to_char("Выходы будут показываться в текстовом виде.\n\r", ch);
	    REMOVE_BIT(ch->act, PLR_MAPEXIT);
	}
	else
	{
	    send_to_char("Теперь выходы будут показываться в виде карты.\n\r",
			 ch);
	    SET_BIT(ch->act, PLR_MAPEXIT);
	}
    }
    else
    {
	if (IS_SET(ch->act, PLR_AUTOEXIT))
	{
	    send_to_char("Выходы не будут показываться.\n\r", ch);
	    REMOVE_BIT(ch->act, PLR_AUTOEXIT);
	}
	else
	{
	    send_to_char("Теперь выходы будут показываться.\n\r", ch);
	    SET_BIT(ch->act, PLR_AUTOEXIT);
	}
    }
}

void do_autogold(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->act, PLR_AUTOGOLD))
    {
	send_to_char("Режим 'автозолото' выключен.\n\r", ch);
	REMOVE_BIT(ch->act, PLR_AUTOGOLD);
    }
    else
    {
	send_to_char("Ты будешь автоматически забирать золото с трупов.\n\r",
		     ch);
	SET_BIT(ch->act, PLR_AUTOGOLD);
    }
}

void do_automoney(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->act, PLR_AUTOMONEY))
    {
	send_to_char("Режим 'автомонеты' выключен.\n\r", ch);
	REMOVE_BIT(ch->act, PLR_AUTOMONEY);
    }
    else
    {
	send_to_char("Ты будешь автоматически забирать деньги с трупов.\n\r",
		     ch);
	SET_BIT(ch->act, PLR_AUTOMONEY);
    }
}

void do_autotitle(CHAR_DATA *ch, char *argument)
{
    char bfr[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->act, PLR_AUTOTITLE))
    {
	send_to_char("Теперь твой титул не будет меняться автоматически.\n\r",
		     ch);
	REMOVE_BIT(ch->act, PLR_AUTOTITLE);
	if (ch->level < TITLE_LEVEL)
	{
	    sprintf(bfr, "Но до %d уровня ты не сможешь воспользоваться "
		    "этой возможностью.\n\r", TITLE_LEVEL);
	    send_to_char(bfr, ch);
	}
    }
    else
    {
	send_to_char("Теперь твой титул будет автоматически изменяться.\n\r",
		     ch);
	SET_BIT(ch->act, PLR_AUTOTITLE);
    }

}


void do_autoloot(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->act, PLR_AUTOLOOT))
    {
	send_to_char("Режим 'автограбеж' выключен.\n\r", ch);
	REMOVE_BIT(ch->act, PLR_AUTOLOOT);
    }
    else
    {
	send_to_char("Ты будешь автоматически обшаривать трупы.\n\r", ch);
	SET_BIT(ch->act, PLR_AUTOLOOT);
    }
}

void do_autoattack(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->act, PLR_AUTOATTACK))
    {
	send_to_char("Режим 'атаки' выключен.\n\r", ch);
	REMOVE_BIT(ch->act, PLR_AUTOATTACK);
    }
    else
    {
	send_to_char("Ты теперь можешь атаковать.\n\r", ch);
	SET_BIT(ch->act, PLR_AUTOATTACK);
    }
}

void do_autodreams(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->act, PLR_NODREAMS))
    {
	send_to_char("Теперь ты будешь видеть сны.\n\r", ch);
	REMOVE_BIT(ch->act, PLR_NODREAMS);
    }
    else
    {
	send_to_char("Ты не будешь видеть сны.\n\r", ch);
	SET_BIT(ch->act, PLR_NODREAMS);
    }
}


void do_autosac(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->act, PLR_AUTOSAC))
    {
	send_to_char("Режим 'автожертва' выключен.\n\r", ch);
	REMOVE_BIT(ch->act, PLR_AUTOSAC);
    }
    else
    {
	send_to_char("Ты будешь автоматически жертвовать трупы.\n\r", ch);
	SET_BIT(ch->act, PLR_AUTOSAC);
    }
}

void do_autosplit(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->act, PLR_AUTOSPLIT))
    {
	send_to_char("Режим 'автодележ' выключен.\n\r", ch);
	REMOVE_BIT(ch->act, PLR_AUTOSPLIT);
    }
    else
    {
	send_to_char("Ты будешь автоматически делить деньги между "
		     "членами группы.\n\r", ch);
	SET_BIT(ch->act, PLR_AUTOSPLIT);
    }
}

void do_brief(CHAR_DATA *ch, char *argument)
{
    if (IS_SET(ch->comm, COMM_BRIEF))
    {
	send_to_char("Будут показываться длинные описания.\n\r", ch);
	REMOVE_BIT(ch->comm, COMM_BRIEF);
    }
    else
    {
	send_to_char("Будут показываться короткие описания.\n\r", ch);
	SET_BIT(ch->comm, COMM_BRIEF);
    }
}

void do_compact(CHAR_DATA *ch, char *argument)
{
    if (IS_SET(ch->comm, COMM_COMPACT))
    {
	send_to_char("Режим 'компактно' выключен.\n\r", ch);
	REMOVE_BIT(ch->comm, COMM_COMPACT);
    }
    else
    {
	send_to_char("Режим 'компактно' включен.\n\r", ch);
	SET_BIT(ch->comm, COMM_COMPACT);
    }
}

void do_show(CHAR_DATA *ch, char *argument)
{
    if (IS_SET(ch->comm, COMM_SHOW_AFFECTS))
    {
	send_to_char("Эффекты не будут показываться в счете.\n\r", ch);
	REMOVE_BIT(ch->comm, COMM_SHOW_AFFECTS);
    }
    else
    {
	send_to_char("Теперь эффекты будут отображаться в счете.\n\r", ch);
	SET_BIT(ch->comm, COMM_SHOW_AFFECTS);
    }
}

void do_prompt(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if (argument[0] == '\0')
    {
	if (IS_SET(ch->comm, COMM_PROMPT))
	{
	    send_to_char("Подсказка не будет показываться.\n\r", ch);
	    REMOVE_BIT(ch->comm, COMM_PROMPT);
	}
	else
	{
	    send_to_char("Теперь ты можешь видеть подсказку.\n\r", ch);
	    SET_BIT(ch->comm, COMM_PROMPT);
	}
	return;
    }

    if (!strcmp(argument, "all") || !strcmp(argument, "все"))
	strcpy(buf, "<%hhp %mm %vmv> ");
    else if (!strcmp(argument, "показ") || !strcmp(argument, "show"))
    {
	if (ch->prompt != NULL)
	{
	    sprintf(buf, "\n\rТекущее приглашение: << %s >>\n\r",
		    color_to_str(ch->prompt));
	    send_to_char(buf, ch);
	}
	else
	{
	    send_to_char("У тебя нет установленного приглашения.\n\r", ch);
	}
	return;
    }
    else if (!strcmp(argument, "ga"))
    {
	if (IS_SET(ch->comm, COMM_TELNET_GA))
	{
	    send_to_char("Посылка GA выключена.\n\r", ch);
	    REMOVE_BIT(ch->comm, COMM_TELNET_GA);
	}
	else
	{
	    send_to_char("Посылка GA включена.\n\r", ch);
	    SET_BIT(ch->comm, COMM_TELNET_GA);
	}

	return;
    }
    else
    {
	if (strlen(argument) > MAX_PROMPT_LENGTH)
	    argument[MAX_PROMPT_LENGTH] = '\0';

	strcpy(buf, argument);
	smash_tilde(buf);

	if (str_suffix("%c", buf))
	    strcat(buf, " ");
    }

    free_string(ch->prompt);
    ch->prompt = str_dup(buf);
    sprintf(buf, "Подсказка установлена в %s\n\r", color_to_str(ch->prompt));
    send_to_char(buf, ch);
    return;
}

/* void do_combine(CHAR_DATA *ch, char *argument)
 {
 if (IS_SET(ch->comm, COMM_COMBINE))
 {
 send_to_char("Выбрано длинное описание инвентаря.\n\r", ch);
 REMOVE_BIT(ch->comm, COMM_COMBINE);
 }
 else
 {
 send_to_char("Выбрано комбинированое описание инвентаря.\n\r", ch);
 SET_BIT(ch->comm, COMM_COMBINE);
 }
 }  */

/*
 void do_noloot(CHAR_DATA *ch, char *argument)
 {
 if (IS_NPC(ch))
 return;

 if (IS_SET(ch->act, PLR_CANLOOT))
 {
 send_to_char("Your corpse is now safe from thieves.\n\r", ch);
 REMOVE_BIT(ch->act, PLR_CANLOOT);
 }
 else
 {
 send_to_char("Your corpse may now be looted.\n\r", ch);
 SET_BIT(ch->act, PLR_CANLOOT);
 }
 }
 */

void pet_gone(CHAR_DATA *pet)
{
    char *msg;

    stop_follower(pet);

    if (pet->position < POS_STANDING)
	do_stand(pet, "");

    if (IS_NPC(pet) && 
	(pet->pIndexData->vnum == MOB_VNUM_ZOMBIE 
	 || pet->pIndexData->vnum == MOB_BONE_SHIELD
	 || pet->pIndexData->vnum == MOB_VNUM_DRUIDS_TREE))
	msg = "$n разваливается на части...";
    else
	msg = "$n $t вдаль...";

    act(msg, pet, IS_AFFECTED(pet, AFF_FLYING) ? "улетает" : "убегает", NULL, TO_ALL);
    extract_char(pet, TRUE);
}

void do_nofollow(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    if (IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM))
	return;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	if (IS_SET(ch->act, PLR_NOFOLLOW))
	{
	    send_to_char("Теперь за тобой могут следовать.\n\r", ch);
	    REMOVE_BIT(ch->act, PLR_NOFOLLOW);
	}
	else
	{
	    send_to_char("Ты не принимаешь последователей.\n\r", ch);
	    SET_BIT(ch->act, PLR_NOFOLLOW);

	    if (ch->pet)
		pet_gone(ch->pet);

	    if (ch->mount != NULL)
	    {
		if (MOUNTED(ch))
		    do_dismount(ch, "");
		pet_gone(ch->mount);
		ch->mount = NULL;
	    }

	    die_follower(ch);
	}
    }
    else
    {
//	if ((victim = get_char_room(ch, NULL, arg, FALSE)) == NULL)
//	{
//	    send_to_char("Таких здесь нет.\n\r", ch);
//	    return;
//	}

	if ((victim = get_char_world(ch, arg)) == NULL)
	{
	    send_to_char("Таких здесь нет.\n\r", ch);
	    return;
	}

	if (victim == ch)
	{
	    send_to_char("Отследовать себя? ну-ну.\n\r", ch);
	    return;
	}

	if (victim->master != ch)
	{
	    act("Он$T и так не следует за тобой.", ch, NULL, SEX_ENDING(victim), TO_CHAR);
	    return;
	}
	else
	{
	    act("Он$T больше не следует за тобой.", ch, NULL, SEX_ENDING(victim), TO_CHAR);
	    act("$n запрещает тебе следовать.", ch, NULL, victim, TO_VICT);

	    if (ch->pet == victim)
	    {
		pet_gone(victim);
		return;
	    }

	    if (ch->mount == victim)
	    {
		if (MOUNTED(ch))
		    do_dismount(ch, "");

		pet_gone(victim);
		ch->mount = NULL;
		return; /*фикс баги stop_follow (вызывалась два раза, из pet_gone() и ниже*/
	    }

	    stop_follower(victim);

	}
    }
}

/*
 void do_nosummon(CHAR_DATA *ch, char *argument)
 {
 if (IS_NPC(ch))
 {
 if (IS_SET(ch->imm_flags, IMM_SUMMON))
 {
 send_to_char("You are no longer immune to summon.\n\r", ch);
 REMOVE_BIT(ch->imm_flags, IMM_SUMMON);
 }
 else
 {
 send_to_char("You are now immune to summoning.\n\r", ch);
 SET_BIT(ch->imm_flags, IMM_SUMMON);
 }
 }
 else
 {
 if (IS_SET(ch->act, PLR_NOSUMMON))
 {
 send_to_char("You are no longer immune to summon.\n\r", ch);
 REMOVE_BIT(ch->act, PLR_NOSUMMON);
 }
 else
 {
 send_to_char("You are now immune to summoning.\n\r", ch);
 SET_BIT(ch->act, PLR_NOSUMMON);
 }
 }
 }
 */



#define MAP_WIDTH 72
#define SHOW_WIDTH MAP_WIDTH/2
#define MAP_HEIGHT 9
/* Should be the string length and number of the constants below.*/
const char * star_map[] =
{
    "   W.N     ' .     :. M, N     :  y:., N    `  ,       B, N      .      .  ",
    " W. :.N .      G, N  :M.: .N  :` y.N    .      :     B:   .N       :     ",
    "    W:N    G.N:       M:., N:.:   y`N      ,    c.N           .:    `    ",
    "   W.`:N       '. G.N  `  : ::.      y.N      c'N      B.N R., , N       ",
    " W:'  `:N .  G. N    `  :    .y.N:.          ,     B.N      :  R:   . .N",
    ":' '.   .    G:.N      .'   '   :::.  ,  c.N   :c.N    `        R`.N    ",
    "      :       `        `        :. ::. :     '  :        ,   , R.`:N    ",
    "  ,       G:.N              `y.N :. ::.c`N      c`.N   '        `      .",
    "     ..        G.:N :           .:   c.N:.    .              .          "
};

/***************************CONSTELLATIONS*******************************
 Lupus     Gigas      Pyx      Enigma   Centaurus    Terken    Raptus
 The       The       The       The       The         The       The
 White Wolf  Giant     Pixie     Sphinx    Centaur      Drow     Raptor
 *************************************************************************/
const char * sun_map[] =
{
    "\\'|'/",
    "- O -",
    "/.|.\\"
};

#define MOON_MAP_WIDTH 3

const char *moon_map[MOON_MAX_QTRS] =
{
    "   "
	"   "
	"   "
	,

    " @ "
	"@  "
	" @ "
	,

    " @ "
	"@@ "
	" @ "
	,

    " @ "
	"@@@"
	" @ "
	,

    " @ "
	" @@"
	" @ "
	,

    " @ "
	"  @"
	" @ "

};

void look_sky(CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];
    char buf2[10];
    int starpos, sunpos, moonpos[max_moons], i, linenum;

    if (!IS_OUTSIDE(ch))
    {
	send_to_char("Ты не можешь поглядеть на небо в помещении.\n\r", ch);
	return;
    }

    send_to_char("Ты поднимаешь голову и видишь на небе следующее:\n\r\n\r", ch);

    sunpos  = (MAP_WIDTH * (24 - time_info.hour) / 24);

    /*    sprintf(buf, "Sunpos: %d \n\r\n\r", sunpos);
     send_to_char(buf, ch);*/

    for (i = 0; i < max_moons;i++)
	moonpos[i] = i % 2 ? abs(sunpos - MAP_WIDTH/2) : MAP_WIDTH - abs(sunpos - MAP_WIDTH/2);

    starpos = (sunpos + MAP_WIDTH * time_info.month / NUM_MONTHS) % MAP_WIDTH;
    /* The left end of the star_map will be straight overhead at midnight during
     month 0 */

    for (linenum = 0; linenum < MAP_HEIGHT; linenum++)
    {
	if ((time_info.hour >= 6 && time_info.hour <= 18)
	    && (linenum < 3 || linenum >= 6))
	{
	    continue;
	}

	sprintf(buf, "{W|{x");
	for (i = MAP_WIDTH/4; i <= 3*MAP_WIDTH/4; i++)
	{
	    bool sunstars = TRUE;

	    if (linenum >= 3 && linenum < 6)
	    {
		int j;

		for (j = 0; j < max_moons;j++)
		{
		    if (moonpos[j] >= MAP_WIDTH/4 - 2
			&& (moonpos[j] <= 3 * MAP_WIDTH/4 + 2) /* in sky? */
			&& (i >= moonpos[j] - MOON_MAP_WIDTH/2) && (i <= moonpos[j] + MOON_MAP_WIDTH/2)
			&& moons_data[j].moonlight >= moons_data[j].period / 2)
		    {
			sprintf(buf2, "{%c%c{x", moons_data[j].color, moon_map[get_moon_phase(j)][(linenum - 3) * MOON_MAP_WIDTH + i + MOON_MAP_WIDTH/2 - moonpos[j]]);
			strcat(buf, buf2);
			sunstars = FALSE;
			break;
		    }
		}
	    }

	    if (sunstars && weather_info.sky > SKY_CLOUDLESS
		&& number_range(0, 10) < weather_info.sky)
	    {
		strcat(buf, "~");
		sunstars = FALSE;
	    }

	    /* plot sun or stars */
	    if (sunstars)
	    {
		if (time_info.hour >= 6 && time_info.hour <= 18) /* daytime */
		{
		    if (weather_info.sky < SKY_RAINING
			&& i >= sunpos - 2 && i <= sunpos + 2)
		    {
			sprintf(buf2, "{y%c{x", sun_map[linenum-3][i+2-sunpos]);
			strcat(buf, buf2);
		    }
		    else
			strcat(buf, " ");
		}
		else
		{
		    switch (star_map[linenum][(MAP_WIDTH+i-starpos)%MAP_WIDTH])
		    {
		    default:
			strcat(buf, " ");
			break;
		    case '.':
			strcat(buf, ".");
			break;
		    case ',':
			strcat(buf, ",");
			break;
		    case ':':
			strcat(buf, ":");
			break;
		    case '`':
			strcat(buf, "`");
			break;
		    case 'R':
			strcat(buf, "{R ");
			break;
		    case 'G':
			strcat(buf, "{G ");
			break;
		    case 'B':
			strcat(buf, "{B ");
			break;
		    case 'W':
			strcat(buf, "{W ");
			break;
		    case 'M':
			strcat(buf, "{M ");
			break;
		    case 'N':
			strcat(buf, "{x ");
			break;
		    case 'y':
			strcat(buf, "{y ");
			break;
		    case 'c':
			strcat(buf, "{c ");
			break;
		    }
		}
	    }
	}
	strcat(buf, "{W|{x\n\r");
	send_to_char(buf, ch);
    }
}

void do_look_sky(CHAR_DATA *ch, char *argument)
{
    do_look(ch, "sky");
}

char *obj_vnum(CHAR_DATA *ch, OBJ_DATA *obj)
{
    static char buf[MAX_INPUT_LENGTH];

    if (obj && IS_IMMORTAL(ch))
    {
	sprintf(buf, "{R{r[{RObject %d{r]{x\n\r", obj->pIndexData->vnum);
	return buf;
    }
    return "";
}

bool show_descr(CHAR_DATA *ch, OBJ_DATA *list, int number, int *count, char *arg3)
{
    OBJ_DATA *obj;
    char buf[MSL], *pdesc, *pdesc1;
    int skill;

    for (obj = list; obj != NULL; obj = obj->next_content)
    {
	if (can_see_obj(ch, obj))
	{  /* player can see object */
	    char ow[MAX_STRING_LENGTH];

	    if (!IS_NULLSTR(obj->owner))
		sprintf(ow, "Владелец - %s.\n\r", obj->owner);
	    else
		ow[0] = '\0';

	    if ((obj->item_type == ITEM_CONTAINER
		 || obj->item_type == ITEM_PORTAL)
		&& obj->trap
		&& (skill = get_skill(ch, gsn_detect_trap)) > 0
		&& number_percent() < skill)
	    {
		strcat(ow, "Ты замечаешь ловушку.\n\r");
	    }


	    if (IS_OBJ_STAT(obj, ITEM_HAS_SOCKET))
		strcat(ow, "Имеется свободное гнездо для артефакта.\n\r");

	    pdesc = get_extra_descr(arg3, obj->pIndexData->extra_descr);
	    pdesc1 = get_extra_descr(arg3, obj->extra_descr);
	    if (pdesc != NULL || pdesc1 != NULL)
	    {
		if (++*count == number)
		{
		    if (pdesc)
		    {
			sprintf(buf, "%s%s\n\r%s", obj_vnum(ch, obj), pdesc, ow);
			tagline_to_char(buf, ch, ch);
		    }

		    if (pdesc1)
		    {
			sprintf(buf, "%s%s\n\r%s", pdesc ? "" : obj_vnum(ch, obj), pdesc1, ow);
			tagline_to_char(buf, ch, ch);
		    }
		    sprintf(buf, "Состояние - %s.\n\r", show_condition(obj));
		    tagline_to_char(buf, ch, ch);
		    return TRUE;
		}
		else
		    continue;
	    }

	    if (is_name(arg3, obj->name) && ++*count == number)
	    {
		sprintf(buf, "%s%s\n\r%sСостояние - %s.\n\r",
			obj_vnum(ch, obj), obj->description,
			ow,
			show_condition(obj));
		tagline_to_char(buf, ch, ch);
		return TRUE;
	    }

	}
    }

    return FALSE;
}

void do_look(CHAR_DATA *ch, char *argument)
{
    char buf  [MAX_STRING_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    EXIT_DATA *pexit;
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    char *pdesc;
    int door;
    int number, count;
    int skill;
    bool sky = FALSE;

    if (ch->desc == NULL)
	return;

    if (ch->position < POS_SLEEPING)
    {
	send_to_char("Ты ничего не видишь, кроме звезд!\n\r", ch);
	return;
    }

    if (ch->position == POS_SLEEPING)
    {
	send_to_char("Ты ничего не видишь, ты же спишь!\n\r", ch);
	return;
    }

    if (!ch->in_room)
	return;

    if (!check_blind(ch))
    {
	show_char_to_char(LIST_FIRST(&ch->in_room->people), ch);
	check_clan_war(ch);
	return;
    }

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    number = number_argument(arg1, arg3);
    count = 0;

    if (!str_cmp(arg1, "sky") || !str_cmp(arg1, "небо"))
	sky = TRUE;

    if (arg1[0] == '\0' || !str_cmp(arg1, "auto") || sky)
    {
	/* 'look' or 'look auto' */

	if (!IS_NPC(ch)
	    && !IS_SET(ch->act, PLR_HOLYLIGHT)
	    && room_is_dark_new(ch))
	{
	    send_to_char("Тут темно... \n\r", ch);
	    show_list_to_char(ch->in_room->contents,
			      ch, FALSE, FALSE, FALSE, eItemGet);
	    show_char_to_char(LIST_FIRST(&ch->in_room->people), ch);
	    check_clan_war(ch);
	    return;
	}

	if (sky)
	{
	    look_sky(ch);
	    return;
	}

	sprintf(buf, "{C         %s{x", ch->in_room->name);
	send_to_char(buf, ch);

	if ((IS_IMMORTAL(ch) && (IS_NPC(ch) || IS_SET(ch->act, PLR_HOLYLIGHT)))
	    || IS_BUILDER(ch, ch->in_room->area))
	{
	    sprintf(buf, " {r[{RRoom %d{r]", ch->in_room->vnum);
	    send_to_char(buf, ch);
	    if (IS_SET(ch->in_room->room_flags, ROOM_SAFE))
		send_to_char(" {g[{GSafe Room{g]{x", ch);
	}

	send_to_char("{x\n\r", ch);

	if (arg1[0] == '\0'
	    || (!IS_NPC(ch) && !IS_SET(ch->comm, COMM_BRIEF)))
	{
	    char b[MSL], *t;
	    EXTRA_DESCR_DATA *ex;

	    strcpy(b, ch->in_room->description);

	    for(ex = ch->in_room->extra_descr; ex; ex = ex->next)
	    {
		t = b;
		while((t = str_str(t, ex->keyword)))
		{
		    char *m;

		    for(m = t + strlen(b); m >= t; m--)
			*(m + 2) = *m;

		    *t = '{';
		    *++t = 'y';
		    t += strlen(ex->keyword) + 1;

		    for(m = t + strlen(b); m >= t; m--)
			*(m + 2) = *m;

		    *t = '{';
		    *++t = 'x';
		}
	    }

	    sprintf(buf, "{S  %s{x", b);
	    tagline_to_char(buf, ch, ch);
	}

	if ((!IS_NPC(ch) && IS_SET(ch->act, PLR_AUTOEXIT))
	    || (IS_SWITCHED(ch)
		&& IS_SET(ch->desc->original->act, PLR_AUTOEXIT)))
	{
	    /*	    send_to_char("\n\r", ch); */
	    do_function(ch, &do_exits, "auto");
	}

	show_list_to_char(ch->in_room->contents,
			  ch, FALSE, FALSE, FALSE, eItemGet);
	show_char_to_char(LIST_FIRST(&ch->in_room->people), ch);

	check_clan_war(ch);

	return;
    }

    if (!str_cmp(arg1, "i")
	|| !str_cmp(arg1, "in")
	|| !str_cmp(arg1, "on")
	|| !str_cmp(arg1, "в")
	|| !str_cmp(arg1, "на"))
    {
	ROOM_INDEX_DATA *proom;

	/* 'look in' */
	if (arg2[0] == '\0')
	{
	    send_to_char("Посмотреть во что?\n\r", ch);
	    return;
	}

	if ((obj = get_obj_here(ch, NULL, arg2)) == NULL)
	{
	    send_to_char("Ты не видишь этого здесь.\n\r", ch);
	    return;
	}

	switch (obj->item_type)
	{
	default:
	    send_to_char("Это не контейнер.\n\r", ch);
	    break;

	case ITEM_DRINK_CON:
	    if (obj->value[1] <= 0)
	    {
		send_to_char("Там пусто.\n\r", ch);
		break;
	    }

	    sprintf(buf, "Здесь %s жидкости %s цвета.\n\r",
		    obj->value[1] < obj->value[0] / 4
		    ? "меньше половины"
		    : obj->value[1] < 3 * obj->value[0] / 4
		    ? "около половины"
		    : "больше половины",
		    cases(liq_table[obj->value[2]].liq_color, 1));

	    send_to_char(buf, ch);
	    break;

	case ITEM_CONTAINER:
	case ITEM_CHEST:
	case ITEM_MORTAR:
	case ITEM_CORPSE_NPC:
	case ITEM_CORPSE_PC:
	    if (obj->trap
		&& (skill = get_skill(ch, gsn_detect_trap)) > 0
		&& number_percent() < skill)
	    {
		send_to_char("Ты замечаешь ловушку.\n\r", ch);
	    }

	    if (IS_SET(obj->value[1], CONT_CLOSED))
	    {
		send_to_char("Это закрыто.\n\r", ch);
		break;
	    }

	    act("$p содержит:", ch, obj, NULL, TO_CHAR);
	    show_list_to_char(obj->contains,
			      ch, TRUE, TRUE, FALSE, eItemNothing);
	    break;

	case ITEM_PORTAL:
	    if (obj->trap
		&& (skill = get_skill(ch, gsn_detect_trap)) > 0
		&& number_percent() < skill)
	    {
		send_to_char("Ты замечаешь ловушку.\n\r", ch);
	    }

	    if (IS_SET(obj->value[2], GATE_UNDEAD)){
			if (!IS_UNDEAD(ch) && ch->classid != CLASS_NECROMANT && ch->classid != CLASS_NAZGUL && ch->race != RACE_ZOMBIE){
				send_to_char("Тебе лучше туда не смотреть, оттуда тянет мертвым запахом...\n\r", ch);
				break;
			}
	    }

	    if (IS_SET(obj->value[2], GATE_BUGGY)
		|| IS_SET(obj->value[2], GATE_RANDOM)
		|| (proom = get_room_index(obj->value[3])) == NULL
		|| !can_see_room(ch, proom))
	    {
		send_to_char("Ты не замечаешь ничего особенного.\n\r", ch);
	    }
	    else if (IS_SET(obj->value[1], EX_CLOSED))
	    {
		act("$p - закрыто.", ch, obj, NULL, TO_CHAR);
	    }
	    else
	    {
		ROOM_INDEX_DATA *troom = ch->in_room;
		bool cw = clan_war_enabled;

		act("Ты вглядываешься в $p6 и видишь:", ch, obj, NULL, TO_CHAR);
		ch->in_room = proom;
		clan_war_enabled = FALSE;
		do_look(ch, "");
		act("Ты замечаешь во внезапно образовавшемся сгустке воздуха лицо $n1, но оно тут же исчезает!", ch, NULL, NULL, TO_ROOM);
		clan_war_enabled = cw;
		ch->in_room = troom;
	    }
	    break;
	}
	return;
    }

    if ((victim = get_char_room(ch, NULL, arg1, FALSE)) != NULL)
    {
	show_char_to_char_1(victim, ch);
	return;
    }

    count = 0;
    if (show_descr(ch, ch->carrying, number, &count, arg3))
	return;

    if (show_descr(ch, ch->in_room->contents, number, &count, arg3))
	return;


    if ((pdesc = get_extra_descr(arg3, ch->in_room->extra_descr)) != NULL)
    {
	tagline_to_char(pdesc, ch, ch);
	return;
    }

    if (count > 0 && count != number)
    {
	sprintf(buf, "Ты видишь здесь только %d %s ",
		count, hours(count, TYPE_LIKE_THIS));
	strcat(buf, hours(count, TYPE_ITEM));
	strcat(buf, ".\n\r");

	send_to_char(buf, ch);
	return;
    }

    door = what_door(arg1);
    if (door == -1)
    {
	send_to_char("Ты не видишь этого здесь.\n\r", ch);
	return;
    }

    /* 'look direction' */
    if ((pexit = ch->in_room->exit[door]) == NULL)
    {
	send_to_char("Ничего особенного.\n\r", ch);
	return;
    }

    if (pexit->trap
	&& (skill = get_skill(ch, gsn_detect_trap)) > 0
	&& number_percent() < skill)
    {
	send_to_char("Ты замечаешь ловушку.\n\r", ch);
    }

    if (pexit->description != NULL && pexit->description[0] != '\0')
	tagline_to_char(pexit->description, ch, ch);
    else
	send_to_char("Ничего особенного.\n\r", ch);

    if (pexit->keyword != NULL
	&& pexit->keyword[0] != '\0'
	&& pexit->keyword[0] != ' ')
    {
	if (IS_SET(pexit->exit_info, EX_CLOSED))
	{
	    act("$d - закрыто.", ch, NULL, pexit->keyword, TO_CHAR);
	}
	else if (IS_SET(pexit->exit_info, EX_ISDOOR))
	{
	    act("$d - открыто.",   ch, NULL, pexit->keyword, TO_CHAR);
	}
    }

    return;
}

/* RT added back for the hell of it */
void do_read (CHAR_DATA *ch, char *argument)
{
    do_function(ch, &do_look, argument);
}

void do_examine(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int num;

    num = number_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Проверить что?\n\r", ch);
	return;
    }

    do_function(ch, &do_look, arg);

    if ((obj = get_obj_here(ch, NULL, arg)) != NULL)
    {
	switch (obj->item_type)
	{
	default:
	    break;

	case ITEM_JUKEBOX:
	    do_function(ch, &do_play, "list");
	    break;

	case ITEM_MONEY:
	    if (obj->value[0] == 0)
	    {
		if (obj->value[1] == 0)
		    sprintf(buf, "Странно... тут нет денег.\n\r");
		else if (obj->value[1] == 1)
		    sprintf(buf, "Ого! Один золотой.\n\r");
		else
		    sprintf(buf, "Тут %d золотых монет.\n\r",
			    obj->value[1]);
	    }
	    else if (obj->value[1] == 0)
	    {
		if (obj->value[0] == 1)
		    sprintf(buf, "Ого! Целая серебряная монетка.\n\r");
		else
		    sprintf(buf, "Тут %d серебряных монет.\n\r",
			    obj->value[0]);
	    }
	    else
		sprintf(buf, "Тут %d золотых и %d серебряных монет.\n\r",
			obj->value[1], obj->value[0]);
	    send_to_char(buf, ch);
	    break;

	case ITEM_DRINK_CON:
	case ITEM_CONTAINER:
	case ITEM_CHEST:
	case ITEM_MORTAR:
	case ITEM_CORPSE_NPC:
	case ITEM_CORPSE_PC:
	    one_argument(obj->name, arg);
	    sprintf(buf, "в %d.%s", num, arg);
	    do_function(ch, &do_look, buf);
	    break;
	}
    }

    return;
}



/*
 * Thanks to Zrin for auto-exit part.
 */
void do_exits(CHAR_DATA *ch, char *argument)
{
    ROOM_INDEX_DATA *race_room;
    extern char * const dir_name[];
    char buf[MAX_STRING_LENGTH] = "";
    char rose[7][21];
    EXIT_DATA *pexit;
    bool found;
    bool fAuto;
    int door, line, max_line = 3, min_line = 3;

    fAuto  = !str_cmp(argument, "auto");

    if (!check_blind(ch))
	return;

    if (fAuto)
    {
	if (IS_SET(ch->act, PLR_MAPEXIT))
	{
	    int i, j;

	    for (i = 0; i < 7; i++)
		for (j = 0; j < 21; j++)
		    rose[i][j] = ' ';
	}
	else
	    sprintf(buf, "\n\r[Выходы:");
    }
    else if (IS_IMMORTAL(ch))
	sprintf(buf, "Видимые выходы из комнаты %d:\n\r", ch->in_room->vnum);
    else
    {
	if (!IS_NPC(ch)	&& room_is_dark_new(ch))
	{
	    send_to_char("Тут темно... \n\r", ch);
	    return;
	}

	sprintf(buf, "Видимые выходы:\n\r");
    }

    found = FALSE;
    for (door = 0; door <= 5; door++)
    {
	if ((pexit = ch->in_room->exit[door]) != NULL
	    && pexit->u1.to_room != NULL
	    && can_see_room(ch, pexit->u1.to_room)
	    && !IS_SET(pexit->exit_info, EX_CLOSED))
	{
	    found = TRUE;
	    if (fAuto)
	    {
		if (IS_SET(ch->act, PLR_MAPEXIT))
		{
		    switch(door)
		    {
		    case 0:
			memcpy(&rose[0][6], capitalize(dir_name[door]),
			       strlen(dir_name[door]));
			rose[1][8] = '|';
			rose[2][8] = '|';
			min_line = 0;
			break;
		    case 1:
			memcpy(&rose[3][9], "---", 3);
			memcpy(&rose[3][12], capitalize(dir_name[door]),
			       strlen(dir_name[door]));
			break;
		    case 2:
			memcpy(&rose[6][8], capitalize(dir_name[door]),
			       strlen(dir_name[door]));
			rose[4][8] = '|';
			rose[5][8] = '|';
			max_line = 6;
			break;
		    case 3:
			memcpy(&rose[3][5], "---", 3);
			memcpy(&rose[3][0], capitalize(dir_name[door]),
			       strlen(dir_name[door]));
			break;
		    case 4:
			memcpy(&rose[1][10], capitalize(dir_name[door]),
			       strlen(dir_name[door]));
			rose[2][9] = '/';
			min_line = UMIN(1, min_line);
			break;
		    case 5:
			memcpy(&rose[5][4], capitalize(dir_name[door]),
			       strlen(dir_name[door]));
			rose[4][7] = '/';
			max_line = UMAX(5, max_line);
			break;
		    }
		}
		else
		{
		    strcat(buf, " ");
		    strcat (buf, MXPTAG ("Ex"));
		    strcat(buf, dir_name[door]);
		    strcat (buf, MXPTAG ("/Ex"));
		}
	    }
	    else
	    {
		if (!IS_NPC(ch))
		    race_room = get_room_index(pc_race_table[ch->race].recall);
		else
		    race_room = pexit->u1.to_room;
		sprintf(buf + strlen(buf), "%-6s - %s",
			capitalize(dir_name[door]),
			room_is_dark(pexit->u1.to_room)
			? "Ничего не видно"
			: (IS_SET(pexit->exit_info, EX_RACE)
			   ? race_room->name
			   : pexit->u1.to_room->name)
		       );
		if (IS_IMMORTAL(ch))
		    sprintf(buf + strlen(buf), " (room %d)\n\r", pexit->u1.to_room->vnum);
		else
		    sprintf(buf + strlen(buf), "\n\r");
	    }
	}
    }

    if (!found)
	strcat(buf, fAuto ? " отсутствуют" : "Отсутствуют.\n\r");

    if (fAuto)
    {
/*	sprintf(buf, "[ОТЛАДКА_БРУЛЬ1");
*/
    
	strcat(buf, "]\n\r");
    

	if (IS_SET(ch->act, PLR_MAPEXIT))
	{
	    send_to_char("\n\r", ch);
	    if (found)
	    {
		rose[3][8] = '+';
		for (door = 0; door <= max_line; door++)
		{
			rose[door][18] = '\n';
		   	rose[door][19] = '\r'; 
			rose[door][20] = '\0';
		}
		
		for (line = min_line; line <= max_line; line++)
		{
			send_to_char(&rose[line][0], ch);
		}
	    }
	    else
		send_to_char("[Выходы отсутствуют]", ch);

	    send_to_char("\n\r", ch);
	    return;
	}
    }
    send_to_char(buf, ch);
    return;
}

/****/

void do_worth(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
    {
	sprintf(buf, "У тебя %ld золота и %ld серебра.\n\r",
		ch->gold, ch->silver);
	send_to_char(buf, ch);
	return;
    }

    sprintf(buf,
	    "У тебя %ld золота, %ld серебра наличными, и %ld золота в банке.\n\r",
	    ch->gold, ch->silver, ch->pcdata->bank);

    send_to_char(buf, ch);

    return;
}

double get_explore_percent(CHAR_DATA *ch)
{
    int i, j, count = 0;
    for (i = 0; i < ch->pcdata->visited_size; i++)
	if (ch->pcdata->visited[i] > 0)
	    for(j = 0; j < 8; j++)
		if (IS_SET(ch->pcdata->visited[i], 1 << j) && get_room_index(8 * i + j) != NULL) 
		    count++;

    return (double) (100 * (double)count/(double)top_room);
}

void do_score(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];
    char buf2[50];
    char buf3[50];
    char buf4[50];
    char buf5[50];
    int i;

    if (argument[0] != '\0'
	&& (!str_prefix(argument, "кратко")
	    || !str_prefix(argument, "коротко")
	    || !str_prefix(argument, "brief")))
    {
	sprintf(buf, "{GЖИЗНЬ:{x%d/%d  {GМАНА:{x%d/%d  {GШАГИ:{x%d/%d  {GПРАК:{x%d  "
		"{GТРЕН:{x%d\n\r{GСИЛА:{x%d/%d  {GУМ:{x%d/%d  {GМУДР:{x%d/%d  "
		"{GЛОВК:{x%d/%d  {GСЛОЖ:{x%d/%d",
		ch->hit,  ch->max_hit,
		ch->mana, ch->max_mana,
		ch->move, ch->max_move,
		ch->practice, ch->train,
		ch->perm_stat[STAT_STR],
		get_curr_stat(ch, STAT_STR),
		ch->perm_stat[STAT_INT],
		get_curr_stat(ch, STAT_INT),
		ch->perm_stat[STAT_WIS],
		get_curr_stat(ch, STAT_WIS),
		ch->perm_stat[STAT_DEX],
		get_curr_stat(ch, STAT_DEX),
		ch->perm_stat[STAT_CON],
		get_curr_stat(ch, STAT_CON));
	send_to_char(buf, ch);

	if (ch->level >= 15)
	{
	    sprintf(buf, "  {GПОПАД:{x%d  {GУРОН:{x%d",
		    GET_FULL_HITROLL(ch), GET_FULL_DAMROLL(ch));
	    send_to_char(buf, ch);
	}
	send_to_char("\n\r", ch);

	if (ch->level >= 25)
	{
	    sprintf(buf, "{GБРОНЯ:{x%d/%d/%d/%d  {GСИЛА ВОЛИ:{x%d  ",
		    GET_AC(ch, AC_PIERCE),
		    GET_AC(ch, AC_BASH),
		    GET_AC(ch, AC_SLASH),
		    GET_AC(ch, AC_EXOTIC),
		    ch->saving_throw);
	    send_to_char(buf, ch);
	}

	sprintf(buf, "{GУРОВЕНЬ:{x%d  {GДСУ:{x%d\n\r", ch->level, TNL(ch));
	send_to_char(buf, ch);

	return;
    }


    send_to_char("\n\r     {c/~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/~~\\\n\r", ch);

    if (is_clan(ch))
    {
	sprintf(buf, "{G[{x %s {G]{x", clan_table[ch->clan].short_descr);
	strcpy(buf1, str_color_fmt(buf, 20));
    }
    else
	strcpy(buf1, str_color_fmt(" ", 20));

    sprintf(buf, "    | {W%-10s{x %s%s {c|____|\n\r"
	    "    {c+-------------------------------------------------------------------------+\n\r"
	    "    |{x {cУровень: {C%-2d   {cОчков: {C%-8d{x   ",
	    ch->name,
	    buf1,
	    IS_NPC(ch)
	    ? str_color_fmt(" ", 38) : str_color_fmt(ch->pcdata->title, 40),
	    ch->level, ch->exp
	   );
    send_to_char(buf, ch);

    /* RT shows exp to level */
    if (!IS_NPC(ch))
    {
	sprintf(buf, "  {cДо следующего уровня: {C%6d{c %5s{x    {c|\n\r",
		TNL(ch), hours(TNL(ch), TYPE_POINTS));
	send_to_char(buf, ch);
    }
    else
    {
	send_to_char("                                        {c|\n\r", ch);
    }

    if (ch->level >= 10)
    {
	sprintf(buf, "    | {cНатура: %-13s(%+5d)        {cВозраст:{G%4d%4s/%-5dч.              {c|\n\r", get_align(ch, NULL), ch->alignment,
		get_age(ch), hours(get_age(ch), TYPE_YEARS),
		(ch->played + (int) (current_time - ch->logon)) / 3600);
    }
    else
    {
	sprintf(buf, "    | {cНатура: %-13s{x            {cВозраст:{G%4d%4s/%-5dч.                  {c|\n\r", get_align(ch, NULL),
		get_age(ch), hours(get_age(ch), TYPE_YEARS),
		(ch->played + (int) (current_time - ch->logon)) / 3600);
    }
    send_to_char(buf, ch);

    if (!IS_NPC(ch))
    {
	sprintf(buf, "    | Убито мобов:{x%-5d{c Смертей от игроков:{x%-3d{c мобов:{x%-5d {c Побегов:{x%-6d    {c|\n\r",
		/*	ch->pcdata->kills_pc, */
		ch->pcdata->kills_mob,
		ch->pcdata->deaths_pc,
		ch->pcdata->deaths_mob,
		ch->pcdata->flees
	       );
	send_to_char(buf, ch);
	sprintf(buf, "    | Еретик: {x%-5d{c  Братоубийство: {x%-5d{c {yПлатина: %-5d {rБронза: %-5d{c        |\n\r",
		ch->count_holy_attacks,
		ch->count_guild_attacks,
		ch->pcdata->platinum,
		ch->pcdata->bronze
	       );
	send_to_char(buf, ch);
	if (ch->pcdata->atheist)
	{
		sprintf(buf, "    |                     {RТы атеист.{c                                          |\n\r");
		send_to_char(buf, ch);
	}

	printf_to_char("    | Знание мира:{C%6.2f%%{c",
		       ch, get_explore_percent(ch)); 


	if (get_trust(ch) != ch->level)
	    printf_to_char("       Тебе доверен {C%2d{c уровень                       {c|\n\r", ch, get_trust(ch));
	else
	    send_to_char("                                                     |\n\r", ch);
    }


    sprintf(buf,
	    "    +-------------------------------------------------------------------------+\n\r"
	    "    | {GРаса     : {Y%-9s {c|{x {GСила     {g(STR): {Y%2d{G(%2d) {c| Тренировок: {C%-10d  {c|\n\r"
	    "    | {GПол      : {Y%-9s {c|{x {GУм       {g(INT): {Y%2d{G(%2d) {c| Практик   : {C%-10d  {c|\n\r"
	    "    | {GПрофессия: {Y%-9s {c|{x {GМудрость {g(WIS): {Y%2d{G(%2d) {c|{C Удачи     : {Y%-4d{x({G%-6d{x){c|\n\r"
	    "    | {x           %-9s {c|{x {GЛовкость {g(DEX): {Y%2d{G(%2d) {c|{Y Золота    : %-10ld  {c|\n\r"
	    "    | {xСтиль:%14s {c|{x {GСложение {g(CON): {Y%2d{G(%2d) {c|{W Серебра   : %-10ld  {c|\n\r"
	    "    +-------------------------------------------------------------------------+\n\r"
	    "    | {WЖизнь: {Y%7d{c/{G%-7d  {WМана: {Y%7d{c/{G%-7d  {WШаги: {Y%7d{c/{G%-7d    {c|\n\r"
	    "    +-------------------------------------------------------------------------+\n\r",

	    race_table[ch->race].rname,
	    ch->perm_stat[STAT_STR],
	    get_curr_stat(ch, STAT_STR),
	    ch->train,
	    sex_table[ch->sex].rname,
	    ch->perm_stat[STAT_INT],
	    get_curr_stat(ch, STAT_INT),
	    ch->practice,
	    IS_NPC(ch) ? "моб" : class_table[ch->classid].name,
	    ch->perm_stat[STAT_WIS],
	    get_curr_stat(ch, STAT_WIS),
	    !IS_NPC(ch) ? ch->pcdata->quest_curr : 0,
	    !IS_NPC(ch) ? ch->pcdata->quest_accum : 0, " ",
	    ch->perm_stat[STAT_DEX],
	    get_curr_stat(ch, STAT_DEX),
	    ch->gold, get_style(ch),
	    ch->perm_stat[STAT_CON],
	    get_curr_stat(ch, STAT_CON) ,
	    ch->silver,
	    ch->hit,  ch->max_hit,
	    ch->mana, ch->max_mana,
	    ch->move, ch->max_move
		);
    send_to_char(buf, ch);

    send_to_char("    | {CЗащита от:                                                              {c|\n\r" , ch);
    if (ch->level >= 25)
    {
	sprintf(buf,
		"    |{c    укола: {C%+4d                    {cудара: {C%+4d                           {c|\n\r"
		"    |{c    рубки: {C%+4d               {cостального: {C%+4d                           {c|\n\r",
		GET_AC(ch, AC_PIERCE),
		GET_AC(ch, AC_BASH),
		GET_AC(ch, AC_SLASH),
		GET_AC(ch, AC_EXOTIC));
	send_to_char(buf, ch);
    }
    else
    {
	for (i = 0; i < 4; i++)
	{
	    char * temp;

	    switch(i)
	    {
	    case(AC_PIERCE):	temp = "    |    укола";	break;
	    case(AC_BASH):	temp = "     удара";	break;
	    case(AC_SLASH):	temp = "    |    рубки";	break;
	    case(AC_EXOTIC):	temp = "остального";	break;
	    default:		temp = "ошибка";	break;
	    }

	    if      (GET_AC(ch, i) >=  101)
		sprintf(buf, "{c%s: {Cбезнадежно уязвима    ", temp);
	    else if (GET_AC(ch, i) >= 80)
		sprintf(buf, "{c%s: {Cпрактически нет       ", temp);
	    else if (GET_AC(ch, i) >= 60)
		sprintf(buf, "{c%s: {Cедва                  ", temp);
	    else if (GET_AC(ch, i) >= 40)
		sprintf(buf, "{c%s: {Cслегка                ", temp);
	    else if (GET_AC(ch, i) >= 20)
		sprintf(buf, "{c%s: {Cкое-как               ", temp);
	    else if (GET_AC(ch, i) >= 0)
		sprintf(buf, "{c%s: {Cнеплохо               ", temp);
	    else if (GET_AC(ch, i) >= -20)
		sprintf(buf, "{c%s: {Cхорошо                ", temp);
	    else if (GET_AC(ch, i) >= -40)
		sprintf(buf, "{c%s: {Cочень хорошо          ", temp);
	    else if (GET_AC(ch, i) >= -60)
		sprintf(buf, "{c%s: {Cотлично               ", temp);
	    else if (GET_AC(ch, i) >= -80)
		sprintf(buf, "{c%s: {Cпревосходно           ", temp);
	    else if (GET_AC(ch, i) >= -100)
		sprintf(buf, "{c%s: {Cпрактически неуязвима ", temp);
	    else
		sprintf(buf, "{c%s: {Cбожественная          ", temp);
	    send_to_char(buf, ch);
	    if (i == 1 || i == 3) {send_to_char("      {c|\n\r", ch);}
	}
    }
    send_to_char("    +-------------------------------------------------------------------------+\n\r", ch);

    switch (ch->position)
    {
    case POS_DEAD:
	sprintf(buf1, "Ты МЕРТВ%s!!!", capitalize(SEX_ENDING(ch)));
	break;
    case POS_MORTAL:
	sprintf(buf1, "Ты смертельно ранен%s", SEX_ENDING(ch));
	break;
    case POS_INCAP:
	sprintf(buf1, "Ты не можешь пошевелиться");
	break;
    case POS_STUNNED:
	sprintf(buf1, "Ты оглушен%s.", SEX_ENDING(ch));
	break;
    case POS_SLEEPING:
	sprintf(buf1, "Ты спишь");
	break;
    case POS_RESTING:
	sprintf(buf1, "Ты отдыхаешь");
	break;
    case POS_SITTING:
	sprintf(buf1, "Ты сидишь");
	break;
    case POS_STANDING:
	if(MOUNTED(ch))
	    sprintf(buf1, "Верхом на %s", PERS(MOUNTED(ch), ch, 5));
	else sprintf(buf1, "Ты стоишь");
	break;
    case POS_FIGHTING:
	sprintf(buf1, "Ты сражаешься");
	break;
    case POS_BASHED:
	sprintf(buf1, "Ты сбит%s с ног!", SEX_ENDING(ch));
	break;
    }

    if(RIDDEN(ch))
	sprintf(buf1, "Ты оседлан%s %s.\n\r", SEX_ENDING(ch),
		cases(IS_NPC(RIDDEN(ch)) ? RIDDEN(ch)->short_descr : RIDDEN(ch)->name, 4));

    sprintf(buf2, "%d", GET_FULL_DAMROLL(ch));
    sprintf(buf5, "%d", GET_FULL_HITROLL(ch));
    sprintf(buf3, "%d", ch->saving_throw);
    sprintf(buf4, "Ты взволнован%s", SEX_ENDING(ch));

    sprintf(buf,
	    "    | {cВещи:{Y%4d{c/{G%-4d     {c|{x %-26s {c|{c Осторожность: {C%-4d    {c|\n\r"
	    "    | {cВес :{Y%5ld{c/{G%-5d   {c|{x %-26s {c|{R%-14s {C%-4s    {c|\n\r"
	    "    | {x%-19s{c|{x %-26s {c|{R%-14s {C%-4s    {c|\n\r"
	    "    | {x%-19s{c|{x %-26s {c|{W%-14s {C%-4s    {c|\n\r",
	    ch->carry_number, can_carry_n(ch),
	    buf1,
	    ch->wimpy,
	    get_carry_weight(ch) / 10,
	    can_carry_w(ch) /10,
	    (!IS_NPC(ch) && ch->pcdata->condition[COND_HUNGER] ==  0) ? "Ты хочешь есть" : " ",
	    (ch->level >= 15) ? " Урон        :" : " ",
	    (ch->level >= 15) ? buf2  : " ",
	    (!IS_NPC(ch) && ch->pcdata->condition[COND_FULL] > 40) ? "Твой желудок полон" : " ",
	    (!IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] ==  0) ? "Ты хочешь пить" : " ",
	    (ch->level >= 15) ? " Попадание   :" : " ",
	    (ch->level >= 15) ? buf5 : " ",
	    (!IS_NPC(ch) && IS_SET(ch->act, PLR_EXCITED)) ? buf4 : " ",
	    (IS_DRUNK(ch)) ?
	    ((ch->sex == SEX_MALE) ? "Ты пьян" :
	     (ch->sex == SEX_FEMALE) ? "Ты пьяна" : "Ты пьяно") : " ",
	    (ch->level >= 25) ? " Сила воли   :" : " ",
		(ch->level >= 25) ? buf3 : " "
		    );

    send_to_char(buf, ch);


    /* RT wizinvis and holy light */
    if (IS_IMMORTAL(ch))
    {
	send_to_char("    +-------------------------------------------------------------------------+\n\r", ch);
	send_to_char("    | {xHoly Light: ", ch);
	if (IS_SET(ch->act, PLR_HOLYLIGHT))
	    send_to_char("on     {c|{x", ch);
	else
	    send_to_char("off    {c|{x", ch);

	if (ch->invis_level)
	{
	    sprintf(buf, " Invisible: level %2d        {c|{x", ch->invis_level);
	}
	else
	{
	    sprintf(buf, "                            {c|{x");
	}
	send_to_char(buf, ch);
	if (ch->incog_level)
	{
	    sprintf(buf, " Incognito: level %2d ", ch->incog_level);
	}
	else
	{
	    sprintf(buf, "                     ");
	}
	send_to_char(buf, ch);
	send_to_char("  {c|\n\r", ch);
    }

    send_to_char(" {c/~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/   |\n\r", ch);
    send_to_char(" {c\\________________________________________________________________________\\__/{x\n\r\n\r", ch);

    if (IS_SET(ch->comm, COMM_SHOW_AFFECTS))
	do_function(ch, &do_affects, "");

}


void do_affects(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA *paf, *paf_last = NULL;
    char buf[MAX_STRING_LENGTH];

    if (ch->affected != NULL)
    {
	send_to_char("На тебе следующие эффекты и заклинания:\n\r", ch);
	for (paf = ch->affected; paf != NULL; paf = paf->next)
	{
	    if (paf_last != NULL && paf->type == paf_last->type)
		if (ch->level >= 20)
		    strcpy(buf, "                                ");
		else
		    continue;
	    else
		sprintf(buf, "Эффекты: %-20s",
			get_skill_name(ch, paf->type, TRUE));

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
			    cases(flag_string(dam_flags, paf->bitvector, TRUE), 2));

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
	send_to_char("На тебе нет никаких эффектов.\n\r", ch);

    return;
}



char *	const	day_name	[] =
{
    "понедельник", "вторник", "среда", "четверг", "пятница",
    "суббота", "воскресенье"
};

/*
 char *	const	month_name	[] =
 {
 "январь", "февраль", "март", "апрель",
 "май", "июнь", "июль", "август", "сентябрь",
 "октябрь", "ноябрь", "декабрь"
 };
 */

char *	const	month_name	[] =
{
    "снегварь", "вьюговраль", "кошарт", "капрель",
    "зацветай", "цветюнь", "цветюль", "травгуст", "желтябрь",
    "фруктябрь", "замерзябрь", "хладокабрь"
};

void do_time(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    int day;

    day = time_info.day + 1;
    sprintf(buf, "Сейчас %d %s, %s, %d-е %s, %d год.\n\r",
	    time_info.hour,
	    hours(time_info.hour, TYPE_HOURS),
	    day_name[day % 7],
	    day,
	    cases(month_name[time_info.month], 1),
	    time_info.year);

    send_to_char(buf, ch);
    return;
}

void do_systime(CHAR_DATA *ch, char *argument)
{
    extern time_t boot_time;
    char buf[MAX_STRING_LENGTH];
    int hrs, mnts;

    hrs  = (current_time - boot_time)/3600;
    mnts = ((current_time - boot_time) % 3600)/60;

    sprintf(buf, "Балдердаш был запущен: %s", (char *) c_time(&boot_time));
    send_to_char(buf, ch);

    sprintf(buf, "Системное время: %sUptime: %d %s ",
	    (char *) c_time(&current_time),
	    hrs, hours(hrs, TYPE_HOURS));
    send_to_char(buf, ch);

    sprintf(buf, "%d %s.\n\r", mnts, hours(mnts, TYPE_MINUTES));
    send_to_char(buf, ch);

}

void do_weather(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    int i;

    static char * const sky_look[4] =
    {
	"безоблачно",
	"облачно",
	"собрались тучи",
	"сверкают молнии"
    };

    if (IS_AFFECTED(ch, AFF_BLIND))
    {
	send_to_char("Но ты ничего не видишь вообще!\n\r", ch);
	return;
    }

    if (!IS_OUTSIDE(ch))
    {
	send_to_char("Тебе не видно, что за погода сейчас.\n\r", ch);
	return;
    }

    sprintf(buf, "На небе %s. Дует %s.\n\r",
	    sky_look[weather_info.sky],
	    weather_info.change >= 0
	    ? "теплый южный бриз"
	    : "холодный северный ветер");
    send_to_char(buf, ch);

    for (i = 0; i < max_moons; i++)
    {
	if (moons_data[i].moonlight >= moons_data[i].period / 2)
	{
	    sprintf(buf, "%s находится в фазе ", moons_data[i].name);
	    send_to_char(buf, ch);
	    switch (get_moon_phase(i))
	    {
	    case MOON_NEW:
		send_to_char("новолуния.\n\r", ch);
		break;
	    case MOON_1QTR:
		send_to_char("1ой четверти.\n\r", ch);
		break;
	    case MOON_2QTR:
		send_to_char("2ой четверти.\n\r", ch);
		break;
	    case MOON_FULL:
		send_to_char("полнолуния.\n\r", ch);
		break;
	    case MOON_3QTR:
		send_to_char("3ей четверти.\n\r", ch);
		break;
	    case MOON_4QTR:
		send_to_char("4ой четверти.\n\r", ch);
		break;
	    }
	}
    }

    return;
}

void do_help(CHAR_DATA *ch, char *argument)
{
    HELP_DATA *pHelp;
    BUFFER *output;
    bool found = FALSE;
    char argall[MAX_INPUT_LENGTH], argone[MAX_INPUT_LENGTH];
    int level;

    output = new_buf();

    if (argument[0] == '\0')
	argument = "основные";

    /* this parts handles help a b so that it returns help 'a b' */
    argall[0] = '\0';
    while (argument[0] != '\0')
    {
	argument = one_argument(argument, argone);
	if (argall[0] != '\0')
	    strcat(argall, " ");
	strcat(argall, argone);
    }

    STAILQ_FOREACH(pHelp, &help_first, link)
    {
	level = (pHelp->level < 0) ? -1 * pHelp->level - 1 : pHelp->level;

	if (level > get_trust(ch))
	    continue;

	if (is_name(argall, pHelp->keyword))
	{
	    /* add seperator if found */
	    if (found)
		add_buf(output,
			"\n\r============================================================\n\r\n\r");
	    if (pHelp->level >= 0 && str_cmp(argall, "imotd"))
	    {
		add_buf(output, pHelp->keyword);
		add_buf(output, "\n\r");
	    }

	    /*
	     * Strip leading '.' to allow initial blanks.
	     */
	    if (!IS_NULLSTR(pHelp->text))
	    {
		if (pHelp->text[0] == '.')
		    add_buf(output, pHelp->text+1);
		else
		    add_buf(output, pHelp->text);
	    }

	    found = TRUE;

	    /* small hack :) */
	    if (ch->desc != NULL && ch->desc->connected != CON_PLAYING && ch->desc->connected != CON_GEN_GROUPS)
		break;
	}
    }

    if (!found)
	send_to_char("Подсказка на это слово отсутствует.\n\r", ch);
    else
	page_to_char(buf_string(output), ch);
    free_buf(output);
}

const char *imm_classes[MAX_LEVEL - LEVEL_HERO] =
{
    "Имплементор",
    "Создатель",
    "Величайший",
    "Божество",
    "Бог",
    "Бессмертный",
    "Демиург",
    "Ангел",
    "Аватар"
};

/* whois command */
void do_whois (CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    BUFFER *output;
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    bool found = FALSE;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Ты должен ввести имя.\n\r", ch);
	return;
    }

    output = new_buf();

    SLIST_FOREACH(d, &descriptor_list, link)
    {
	CHAR_DATA *wch;

	if (d->connected != CON_PLAYING)
	    continue;

	wch = CH(d);

	if (!can_see(ch, wch))
	    continue;

	if (!str_prefix(arg, wch->name))
	{
	    char b2[MAX_INPUT_LENGTH], classname[10], b3[5], b4[20];

	    found = TRUE;


	    if (wch->level > LEVEL_HERO)
		sprintf(b4, "%13s", imm_classes[MAX_LEVEL - wch->level]);
	    else
	    {
		strcpy(classname, class_table[wch->classid].who_name);
		classname[5] = '\0';
		sprintf(b4, "%7s %5s",
			wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name
			: "     ", classname);
	    }

	    /*	    strncpy(class, (wch->level > LEVEL_HERO) ?
	     imm_classes[MAX_LEVEL - wch->level] :
	     class_table[wch->class].who_name, 5);

	     class[5] = '\0'; */

	    b4[13] = '\0';

	    sprintf(b3, "%d", wch->level);

	    /* a little formatting */

	    if (!is_clan(wch) || IS_INDEPEND(wch))
		b2[0]='\0';
	    else
		sprintf(b2, " {G[{x %s {G]{x", clan_table[wch->clan].short_descr);

	    sprintf(buf, "[%2s %s] %s%s%s%s%s%s%s%s%s%s%s{x\n\r",
		    (IS_IMMORTAL(wch) || IS_IMMORTAL(ch) || !strcmp(ch->name, wch->name)) ?  b3 : "",
		    b4,
		    /*	    wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name  */
		    /*				    : "     ", */
		    /*	    class, */
		    wch->incog_level >= LEVEL_HERO ? "(Инкогнито) ": "",
		    wch->invis_level >= LEVEL_HERO ? "(Wizi) " : "",
		    IS_SET(wch->comm, COMM_AFK) ? "[ВОК] " : "",
		    IS_HELPER(wch) ? "{B(Помощник){x " : "",
		    IS_SET(wch->act, PLR_KILLER) ? "{R(УБИЙЦА){x " : "",
		    IS_SET(wch->act, PLR_THIEF) ? "{R(ВОР){x " : "",
		    !IS_NULLSTR(wch->pcdata->pretitle) ? wch->pcdata->pretitle : "",
		    !IS_NULLSTR(wch->pcdata->pretitle) ? " " : "",
		    wch->name, b2, IS_NPC(wch) ? "" : wch->pcdata->title);

	    add_buf(output, buf);
	}
    }

    if (!found)
    {
	send_to_char("Таких персонажей в этом мире нет.\n\r", ch);
	free_buf(output);
	return;
    }

    page_to_char(buf_string(output), ch);
    free_buf(output);
}


char *show_count(int count, int helpers, bool show, int level)
{
    static char buf[MIL];
    char        b[MIL];
    FILE	*fp;

    sprintf(buf, "\n\rНайдено: %d. ", count);

    if (max_on <= count)
    {
	strcat(buf, "Это рекорд cо времени перезагрузки");
	max_on = count;
    }
    else
    {
	sprintf(b, "Максимум с перезагрузки: %d", max_on);
	strcat(buf, b);
    }

    if (count > max_ever)
    {
	max_ever = count;
	if ((fp = file_open(MAX_EVER_FILE, "w")) == NULL)
	    bugf("Unable to open MAX_EVER_FILE file.");
	else
	    fprintf(fp, "%d\n", max_ever);

	file_close(fp);
    }

    sprintf(b, ". Абсолютный рекорд: %d.\n\r", max_ever);
    strcat(buf, b);

    sprintf(b, "В мире находится %d %s.\n\r", helpers, hours(helpers, TYPE_HELPERS));

    if ((fp = file_open(WWW_COUNT_FILE, "w")) == NULL)
	bugf("Unable to open WWW_COUNT_FILE file.");
    else
	fprintf(fp, "%s%s\n", buf, b);

    file_close(fp);

    if (level < PK_RANGE)
	strcat(buf, b);

    if (!show)
	buf[0] = '\0';

    return buf;
}


/*
 * New 'who' command originally by Alander of Rivers of Mud.
 */

void do_who(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_STRING_LENGTH];
    BUFFER *output;
    DESCRIPTOR_DATA *d;
    int iClass;
    int iRace;
    int iClan;
    int iLevelLower = 0;
    int iLevelUpper = MAX_LEVEL;
    int nNumber;
    int nMatch;
    bool rgfClass[MAX_CLASS];
    bool rgfRace[MAX_PC_RACE];
    bool rgfClan[MAX_CLAN];
    bool fClassRestrict = FALSE;
    bool fClanRestrict = FALSE;
    bool fClan = FALSE;
    bool fRaceRestrict = FALSE;
    bool fImmortalOnly = FALSE;
    int i, helpers = 0;

    /*
     * Set default arguments.
     */
    for (iClass = 0; iClass < MAX_CLASS; iClass++)
	rgfClass[iClass] = FALSE;
    for (iRace = 0; iRace < MAX_PC_RACE; iRace++)
	rgfRace[iRace] = FALSE;
    for (iClan = 0; iClan < MAX_CLAN; iClan++)
	rgfClan[iClan] = FALSE;


	if (ch->id == 1337692376)
		return;

    /*
     * Parse arguments.
     */

    if (argument[0] != '\0' && (!str_prefix(argument, "immortals")
				|| !str_prefix(argument, "бессмертные")
				|| !str_prefix(argument, "имморталы")))
	fImmortalOnly = TRUE;

    nNumber = 0;

    if (IS_IMMORTAL(ch))
    {
	for (;;)
	{
	    argument = one_argument(argument, arg);
	    if (arg[0] == '\0')
		break;

	    if (is_number(arg))
	    {
		switch (++nNumber)
		{
		case 1: iLevelLower = atoi(arg); break;
		case 2: iLevelUpper = atoi(arg); break;
		default:
			send_to_char("Разрешено только два параметра.\n\r", ch);
			return;
		}
	    }
	    else
	    {

		/*
		 * Look for classes to turn on.
		 */

		if (!fImmortalOnly)
		{
		    iClass = class_lookup(arg);
		    if (iClass == -1)
		    {
			iRace = race_lookup(arg);

			if (iRace == -1 || iRace >= MAX_PC_RACE)
			{
			    if (!str_prefix(arg, "clan") || !str_prefix(arg, "клан"))
				fClan = TRUE;
			    else
			    {
				iClan = clan_lookup(arg);
				if (iClan)
				{
				    fClanRestrict = TRUE;
				    rgfClan[iClan] = TRUE;
				}
				else
				{
				    send_to_char(
						 "Таких рас, профессий или кланов нет в этом мире.\n\r",
						 ch);
				    return;
				}
			    }
			}
			else
			{
			    fRaceRestrict = TRUE;
			    rgfRace[iRace] = TRUE;
			}
		    }
		    else
		    {
			fClassRestrict = TRUE;
			rgfClass[iClass] = TRUE;
		    }
		}
	    }
	}
    }

    /*
     * Now show matching chars.
     */
    nMatch = 0;
    buf[0] = '\0';
    output = new_buf();
    add_buf(output, "\n\r");

    for (i = 0; i < 2; i++)
    {

	if (i == 1)
	{
	    if (nMatch > 0)
		add_buf(output, "{Y-------------------------------------------------------------------------------{x\n\r");
	    if (fImmortalOnly) break;
	}


	SLIST_FOREACH(d, &descriptor_list, link)
	{
	    CHAR_DATA *wch;
	    char b2[MAX_INPUT_LENGTH],classname[10], b3[5], b4[7], b5[20];
	    bool proom_found = FALSE;

	    /*
	     * Check for match against restrictions.
	     * Don't use trust as that exposes trusted mortals.
	     */
	    if (d->connected != CON_PLAYING)
		continue;

	    wch   = CH(d);

	    if (IS_SET(wch->comm, COMM_HELPER) && i == 1 && wch->level >= PK_RANGE*2 && !IS_IMMORTAL(wch))
		helpers++;

	    if (!can_see(ch, d->character))
		continue;

	    if (!can_see(ch, wch))
		continue;

	    if (wch->level < iLevelLower
		|| wch->level > iLevelUpper
		|| (i == 1 && wch->level >= LEVEL_IMMORTAL)
		|| (i == 0 && wch->level < LEVEL_IMMORTAL)
		|| (fClassRestrict && !rgfClass[wch->classid])
		|| (fRaceRestrict && !rgfRace[wch->race])
		|| (fClan && !is_clan(wch))
		|| (fClanRestrict && !rgfClan[is_clan(wch)]))
	    {
		continue;
	    }

	    nMatch++;

	    if (is_penalty_room(wch->in_room))
	    {
		strcpy(b4, "{YТ{x ");
		proom_found = TRUE;
	    }

	    if (wch->level > LEVEL_HERO)
		sprintf(b5, "%15s", imm_classes[MAX_LEVEL - wch->level]);
	    else
	    {
		strcpy(classname, class_table[wch->classid].who_name);
		classname[5] = '\0';
		sprintf(b5, "%9s %5s",
			wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name
			: "     ", classname);
	    }

	    /*	strncpy(class, (wch->level > LEVEL_HERO) ?
	     imm_classes[MAX_LEVEL - wch->level] :
	     class_table[wch->class].who_name, 5);

	     class[5] = '\0'; */

	    b5[15] = '\0';

	    sprintf(b3, "%d", wch->level);

	    /* a little formatting */

	    if (!is_clan(wch) || IS_INDEPEND(wch))
		b2[0]='\0';
	    else
		sprintf(b2, " {G[{x %s {G]{x", clan_table[wch->clan].short_descr);

	    sprintf(buf, "[%2s %s] %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s{x\n\r",
		    IS_IMMORTAL(wch) || IS_IMMORTAL(ch) || !strcmp(ch->name, wch->name) ? b3 : "",
		    /*	wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name  */
		    /*				    : "     ", */
		    /*	class, */
		    b5,
		    IS_IMMORTAL(ch) && (IS_SET(wch->comm, COMM_NOCHANNELS) || IS_SET(wch->comm, COMM_NOEMOTE) || IS_SET(wch->comm, COMM_EXTRANOCHANNELS) ||
					IS_SET(wch->comm, COMM_NOSHOUT) || IS_SET(wch->comm, COMM_NOTELL) || IS_SET(wch->act, PLR_FULL_SILENCE))
		    ? "{GК{x" : IS_IMMORTAL(ch) ? "-" : "",
		    IS_IMMORTAL(ch) && is_affected(wch, gsn_gods_curse) ? "{DП{x" : IS_IMMORTAL(ch) ? "-" : "",
		    IS_IMMORTAL(ch) && wch->pcdata->atheist ? "{CА{x" : IS_IMMORTAL(ch) ? "-" : "",
		    IS_IMMORTAL(ch) && IS_SET(wch->act, PLR_FREEZE) ? "{RЗ{x" : IS_IMMORTAL(ch) ? "-" : "",
		    IS_IMMORTAL(ch) && is_nopk(wch) ? "{MN{x" : IS_IMMORTAL(ch) ? "-" : "",
		    IS_IMMORTAL(ch) && proom_found ? b4 : IS_IMMORTAL(ch) ? "-" : "",
		    IS_IMMORTAL(ch) && wch->pcdata->successed ? "{yО {x" : IS_IMMORTAL(ch) ? "- " : "",
		    wch->incog_level >= LEVEL_HERO ? "(Инкогнито) ": "",
		    wch->invis_level >= LEVEL_HERO ? "(Wizi) " : "",
		    IS_SET(wch->comm, COMM_AFK) ? "[ВОК] " : "",
		    IS_HELPER(wch) ? "{B(Помощник){x " : "",
		    IS_SET(wch->act, PLR_KILLER) ? "{R(УБИЙЦА){x " : "",
		    IS_SET(wch->act, PLR_THIEF) ? "{R(ВОР){x " : "",
		    !IS_NULLSTR(wch->pcdata->pretitle) ? wch->pcdata->pretitle : "",
		    !IS_NULLSTR(wch->pcdata->pretitle) ? " " : "",
		    wch->name, b2, IS_NPC(wch) ? "" : wch->pcdata->title);

	    add_buf(output, buf);

	}
    }

    add_buf(output, show_count(nMatch, helpers, TRUE, ch->level));

    if (IS_IMMORTAL(ch))
	add_buf(output, "\n\rЛегенда:\n\r"
		"  Нет ({GК{x)анала(ов), ({DП{x)роклят, ({CА{x)теист, ({RЗ{x)аморожен, ({MN{x)oPK, в ({YТ{x)юрьме, ({yО{x)добрен.\n\r");

    page_to_char(buf_string(output), ch);
    free_buf(output);

    return;
}

void do_count (CHAR_DATA *ch, char *argument)
{
    int count = 0, helpers = 0;
    DESCRIPTOR_DATA *d;
    CHAR_DATA *fch;
    bool count_only = !str_cmp(argument, "count");

    SLIST_FOREACH(d, &descriptor_list, link)
	if (d->connected == CON_PLAYING
	    && (fch = CH(d)) != NULL)
	{
	    if (can_see(ch, d->character) || count_only)
		count++;

	    if (IS_SET(fch->comm, COMM_HELPER) && fch->level >= PK_RANGE*2 && !IS_IMMORTAL(fch))
		helpers++;
	}

    send_to_char(show_count(count, helpers, !count_only, ch->level), ch);

}

void do_inventory(CHAR_DATA *ch, char *argument)
{
    send_to_char("Ты несешь:\n\r", ch);
    show_list_to_char(ch->carrying, ch, TRUE, TRUE, FALSE, eItemDrop);
    return;
}




void do_equipment(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj;
    int iWear;
    bool found, full = FALSE;

    if (argument[0] != '\0' && !str_prefix(argument, "полная")) full = TRUE;

    send_to_char("Ты используешь:\n\r", ch);
    found = FALSE;

	if ((obj = get_eq_char(ch, WEAR_AT_SHOULDER)) != NULL)
	{
		send_to_char(where_name[WEAR_AT_SHOULDER].name, ch);			
		if (obj == NULL)
		    send_to_char("(ничего)", ch);
		else if (can_see_obj(ch, obj))
	    	send_to_char(format_obj_to_char(obj, ch, TRUE, TRUE), ch);
			else
	    		send_to_char("что-то.", ch);
		send_to_char("\n\r\n\r", ch);
    }

    for (iWear = 0; iWear < MAX_WEAR-1; iWear++)
    {
		if (((obj = get_eq_char(ch, iWear)) == NULL && !full)
	    	|| (iWear == WEAR_SECONDARY && get_skill(ch, gsn_second_weapon) < 1))
	    	continue;

		if (is_lycanthrope(ch) && iWear != WEAR_WIELD && iWear != WEAR_SECONDARY)
	    	continue;

		send_to_char(where_name[iWear].name, ch);

		if (obj == NULL)
	    	send_to_char("(ничего)", ch);
		else if (can_see_obj(ch, obj))
	    	send_to_char(format_obj_to_char(obj, ch, TRUE, TRUE), ch);
		else
	    	send_to_char("что-то.", ch);

		send_to_char("\n\r", ch);
		found = TRUE;
    }

    if (!found)
	send_to_char("Ничего.\n\r", ch);

    return;
}



/* Проверочная функци
 void do_compare(CHAR_DATA *ch, char *argument)
 {
 char arg1[MAX_INPUT_LENGTH];
 char arg2[MAX_INPUT_LENGTH];
 OBJ_DATA *obj1;
 OBJ_DATA *obj2;
 char *msg;

 argument = one_argument(argument, arg1);
 argument = one_argument(argument, arg2);
 if (arg1[0] == '\0')
 {
 send_to_char("Сравнить что с чем?\n\r", ch);
 return;
 }

 if ((obj1 = get_obj_carry(ch, arg1, ch)) == NULL)
 {
 send_to_char("Ты не несешь этого.\n\r", ch);
 return;
 }

 if (arg2[0] == '\0')
 {
 for (obj2 = ch->carrying; obj2 != NULL; obj2 = obj2->next_content)
 {
 if (obj2->wear_loc != WEAR_NONE
 && can_see_obj(ch, obj2)
 && obj1->item_type == obj2->item_type
 && (obj1->wear_flags & obj2->wear_flags & ~ITEM_TAKE) != 0)
 break;
 }

 if (obj2 == NULL)
 {
 send_to_char("На тебе нет ничего сравнимого.\n\r", ch);
 return;
 }
 }

 else if ((obj2 = get_obj_carry(ch, arg2, ch)) == NULL)
 {
 send_to_char("У тебя нет этого.\n\r", ch);
 return;
 }

 msg		= NULL;

 if (obj1 == obj2)
 {
 msg = "Ты сравниваешь $p с ним же. Выглядит одинаково. Странно, не правда ли?";
 }

 if (msg == NULL)
 {
 if (obj1 == get_best_object(obj1, obj2) && obj2 == get_best_object(obj2, obj1))
 msg = "$p и $P выглядят одинаково.";
 else if (obj1 == get_best_object(obj1, obj2))
 msg = "$p выглядит лучше, чем $P.";
 else
 msg = "$p выглядит хуже, чем $P.";
 }

 act(msg, ch, obj1, obj2, TO_CHAR);
 return;
 }

 */

void do_compare(CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *obj1;
    OBJ_DATA *obj2;
    int value1;
    int value2;
    char *msg;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    if (arg1[0] == '\0')
    {
	send_to_char("Сравнить что с чем?\n\r", ch);
	return;
    }

    if ((obj1 = get_obj_carry(ch, arg1, ch)) == NULL)
    {
	send_to_char("Ты не несешь этого.\n\r", ch);
	return;
    }

    if (arg2[0] == '\0')
    {
	for (obj2 = ch->carrying; obj2 != NULL; obj2 = obj2->next_content)
	{
	    if (obj2->wear_loc != WEAR_NONE
		&& can_see_obj(ch, obj2)
		&& obj1->item_type == obj2->item_type
		&& (obj1->wear_flags & obj2->wear_flags & ~ITEM_TAKE) != 0)
		break;
	}

	if (obj2 == NULL)
	{
	    send_to_char("На тебе нет ничего сравнимого.\n\r", ch);
	    return;
	}
    }

    else if ((obj2 = get_obj_carry(ch, arg2, ch)) == NULL)
    {
	send_to_char("У тебя нет этого.\n\r", ch);
	return;
    }

    msg		= NULL;
    value1	= 0;
    value2	= 0;

    if (obj1 == obj2)
    {
	msg = "Ты сравниваешь $p6 с ним же. Выглядит одинаково. Странно, не правда ли?";
    }
    else if (obj1->item_type != obj2->item_type)
    {
	msg = "Ты не можешь сравнить $p6 и $P6.";
    }
    else
    {
	switch (obj1->item_type)
	{
	default:
	    msg = "Ты не можешь сравнить $p6 и $P6.";
	    break;

	case ITEM_ARMOR:
	    value1 = obj1->value[0] + obj1->value[1] + obj1->value[2];
	    value2 = obj2->value[0] + obj2->value[1] + obj2->value[2];
	    break;

	case ITEM_WEAPON:
	    if (obj1->pIndexData->new_format)
		value1 = (1 + obj1->value[2]) * obj1->value[1];
	    else
		value1 = obj1->value[1] + obj1->value[2];

	    if (obj2->pIndexData->new_format)
		value2 = (1 + obj2->value[2]) * obj2->value[1];
	    else
		value2 = obj2->value[1] + obj2->value[2];
	    break;
	}
    }

    if (msg == NULL)
    {
	if (value1 == value2) msg = "$p и $P выглядят одинаково.";
	else if (value1  > value2) msg = "$p выглядит лучше, чем $P.";
	else                         msg = "$p выглядит хуже, чем $P.";
    }

    act(msg, ch, obj1, obj2, TO_CHAR);
    return;
}


void do_credits(CHAR_DATA *ch, char *argument)
{
    do_function(ch, &do_help, "diku");
    return;
}



void do_where(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim, *safe_victim;
    DESCRIPTOR_DATA *d;
    bool found;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Персонажи, находящиеся рядом с тобой:\n\r", ch);
	found = FALSE;
	SLIST_FOREACH(d, &descriptor_list, link)
	{
	    if (d->connected == CON_PLAYING
		&& (victim = d->character) != NULL
		&& !IS_NPC(victim)
		&& victim->in_room != NULL
		&& !IS_SET(victim->in_room->room_flags, ROOM_NOWHERE)
		&& (is_room_owner(ch, victim->in_room)
		    || !room_is_private(victim->in_room, NULL))
		&& victim->in_room->area == ch->in_room->area
		&& can_see(ch, victim))
	    {
		found = TRUE;
		sprintf(buf, "%-28s %s\n\r",
			victim->name, victim->in_room->name);
		send_to_char(buf, ch);
	    }
	}
	if (!found)
	    send_to_char("Никого\n\r", ch);
    }
    else
    {
	found = FALSE;
	LIST_FOREACH_SAFE(victim, &char_list, link, safe_victim)
	{
	    if (victim->in_room != NULL
		&& victim->in_room->area == ch->in_room->area
		&& !IS_AFFECTED(victim, AFF_HIDE)
		&& !IS_AFFECTED(victim, AFF_SNEAK)
		&& can_see(ch, victim)
		&& is_name(arg, victim->name))
	    {
		found = TRUE;
		sprintf(buf, "%-28s %s\n\r",
			PERS(victim, ch , 0), victim->in_room->name);
		send_to_char(buf, ch);
		break;
	    }
	}

	if (!found)
	    act("Ты не находишь ни одного $T1.", ch, NULL, arg, TO_CHAR);
    }

    return;
}




void do_consider(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    char *msg;
    int diff;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Прикинуть шансы убить кого?\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, NULL, arg, FALSE)) == NULL)
    {
	send_to_char("Таких здесь нет.\n\r", ch);
	return;
    }

    if (ch == victim)
    {
	send_to_char("Нуу... Ты нашел себе наконец-то достойного соперника!\n\r", ch);
	return;
    }

    /*    if (is_safe(ch, victim))
     {
     send_to_char("Даже и не думай об этом.\n\r", ch);
     return;
     }*/

    diff = victim->level - ch->level;

    if (diff <= -10) msg = "Ты можешь убить $N3 голыми руками.";
    else if (diff <=  -5) msg = "$N не соперник тебе.";
    else if (diff <=  -2) msg = "$N3 вроде бы легко убить.";
    else if (diff <=   1) msg = "Прекрасный поединок!";
    else if (diff <=   4) msg = "У тебя сегодня счастливый день?";
    else if (diff <=   9) msg = "$N просто посмеётся над тобой.";
    else                  msg = "Смерть охотно примет тебя.";

    act(msg, ch, NULL, victim, TO_CHAR);
    act("$n прикидывает шансы на бой с $N4.", ch, NULL, victim, TO_NOTVICT);
    act("$n прикидывает шансы на бой с тобой.", ch, NULL, victim, TO_VICT);
    return;
}



void set_title(CHAR_DATA *ch, char *title)
{
    char buf[MAX_INPUT_LENGTH];
    int i;

    if (IS_NPC(ch))
    {
	bugf("Set_title: NPC.");
	return;
    }

    if (title[0] != '.'
	&& title[0] != ','
	&& title[0] != '!'
	&& title[0] != '?')
    {
	buf[0] = ' ';
	strcpy(buf + 1, title);
    }
    else
    {
	strcpy(buf, title);
    }

    for (i = 0;buf[i] != '\0'; i++)
    {
	if (buf[i] == '[' || buf[i] == ']')
	    buf[i] = ' ';

	if (buf[i] == '{' && buf[++i] == '*')
	    buf[i] = '/';
    }

    if (buf[0] != '\0')
	strcat(buf, "{x");

    free_string(ch->pcdata->title);
    ch->pcdata->title = str_dup(buf);
    return;
}

void do_titl(CHAR_DATA *ch, char *argument)
{
    send_to_char("Эту команду надо написать целиком.\n\r", ch);
}

void do_title(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->comm, COMM_NOTITLE))
    {
	send_to_char("Боги отобрали у тебя право выбирать себе титул.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	if (!strcmp(ch->pcdata->title, " "))
	    set_title(ch, title_table[ch->classid][ch->level][ch->pcdata->true_sex == SEX_FEMALE ? 1 : 0]);
	else
	    set_title (ch, argument);
	send_to_char("Ok.\n\r", ch);
	return;
    }


    if (ch->level < TITLE_LEVEL)
    {
	send_to_char("Тебе еще рановато пользоваться титулами.\n\r", ch);
	return;
    }

    if (strlen(argument) > 30)
	argument[30] = '\0';

    smash_tilde(argument);
    set_title(ch, argument);
    send_to_char("Ok.\n\r", ch);
}


void do_descriptio(CHAR_DATA *ch, char *argument)
{
    send_to_char("Эту команду надо написать целиком.\n\r", ch);
}


void do_description(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
	return;

    if (IS_NULLSTR(argument))
    {
	send_to_char("Твое описание:\n\r", ch);
	send_to_char(!IS_NULLSTR(ch->description)
		     ? ch->description
		     : "(Описание отсутствует).\n\r", ch);
	return;
    }

    if (!IS_NPC(ch) && ch->pcdata->successed == 0)
    {
        send_to_char("Cначала получи одобрение у Богов. Набери {y? одобрение{x.\n\r", ch);
        return;
    }

    if (!str_cmp(argument, "написать") || !str_cmp(argument, "edit"))
    {
	string_append(ch, &ch->description);
	return;
    }

    if (argument[0] != '\0')
    {
	buf[0] = '\0';
	smash_tilde(argument);

	if (argument[0] == '-')
	{
	    int len;
	    bool found = FALSE;

	    if (ch->description == NULL || ch->description[0] == '\0')
	    {
		send_to_char("Больше нечего удалять.\n\r", ch);
		return;
	    }

	    strcpy(buf, ch->description);

	    for (len = strlen(buf); len > 0; len--)
	    {
		if (buf[len] == '\r')
		{
		    if (!found)  /* back it up */
		    {
			if (len > 0)
			    len--;
			found = TRUE;
		    }
		    else /* found the second one */
		    {
			buf[len + 1] = '\0';
			free_string(ch->description);
			ch->description = str_dup(buf);
			send_to_char("Твое описание:\n\r", ch);
			send_to_char(ch->description ? ch->description
				     : "(None).\n\r", ch);
			return;
		    }
		}
	    }
	    buf[0] = '\0';
	    free_string(ch->description);
	    ch->description = str_dup(buf);
	    send_to_char("Описание стерто.\n\r", ch);
	    return;
	}

	if (argument[0] == '+')
	{
	    if (ch->description != NULL)
		strlcat(buf, ch->description, MAX_STRING_LENGTH);

	    argument++;
	    while (IS_SPACE(*argument))
		argument++;
	}

	if (strlen(buf) >= MAX_DESCRIPTION_SIZE)
	{
	    send_to_char("Слишком длинное описание.\n\r", ch);
	    return;
	}

	strlcat(buf, argument, MAX_STRING_LENGTH);
	strlcat(buf, "{x\n\r", MAX_STRING_LENGTH);
	free_string(ch->description);
	ch->description = str_dup(buf);
    }
}

void do_char_notepa(CHAR_DATA *ch, char *argument)
{
    send_to_char("Эту команду надо написать целиком.\n\r", ch);
}

void do_char_notepad(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
	return;

    if (IS_NULLSTR(argument))
    {
	send_to_char("Твои записи:\n\r", ch);
	send_to_char(!IS_NULLSTR(ch->notepad)
		     ? ch->notepad
		     : "(Записи отсутствуют).\n\r", ch);
	return;
    }

    if (!str_cmp(argument, "написать") || !str_cmp(argument, "edit"))
    {
	string_append(ch, &ch->notepad);
	return;
    }

    if (argument[0] != '\0')
    {
	buf[0] = '\0';
	smash_tilde(argument);

	if (argument[0] == '-')
	{
	    int len;
	    bool found = FALSE;

	    if (ch->notepad == NULL || ch->notepad[0] == '\0')
	    {
		send_to_char("Больше нечего удалять.\n\r", ch);
		return;
	    }

	    strcpy(buf, ch->notepad);

	    for (len = strlen(buf); len > 0; len--)
	    {
		if (buf[len] == '\r')
		{
		    if (!found)  /* back it up */
		    {
			if (len > 0)
			    len--;
			found = TRUE;
		    }
		    else /* found the second one */
		    {
			buf[len + 1] = '\0';
			free_string(ch->notepad);
			ch->notepad = str_dup(buf);
			send_to_char("Твои записи:\n\r", ch);
			send_to_char(ch->notepad ? ch->notepad
				     : "(None).\n\r", ch);
			return;
		    }
		}
	    }
	    buf[0] = '\0';
	    free_string(ch->notepad);
	    ch->notepad = str_dup(buf);
	    send_to_char("Записи стерты.\n\r", ch);
	    return;
	}
	else if (argument[0] == '+')
	{
	    if (ch->notepad != NULL)
		strlcat(buf, ch->notepad, MAX_STRING_LENGTH);

	    argument++;
	    while (IS_SPACE(*argument))
		argument++;

	    if (strlen(buf) >= MAX_NOTEPAD_SIZE)
	    {
		send_to_char("Слишком много записей.\n\r", ch);
		return;
	    }

	    strlcat(buf, argument, MAX_STRING_LENGTH);
	    strlcat(buf, "{x\n\r", MAX_STRING_LENGTH);
	    free_string(ch->notepad);
	    ch->notepad = str_dup(buf);
	}
	else 
	    send_to_char("Применение: ЗАПИСИ\n\r"
			 "Применение: ЗАПИСИ + <строка>\n\r"
			 "Применение: ЗАПИСИ - \n\r"
			 "Применение: ЗАПИСИ НАПИСАТЬ\n\r", ch);
    }
}
 

void do_report(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_INPUT_LENGTH];

    sprintf(buf,
	    "Ты произносишь: {CУ меня %d/%d жизни %d/%d маны %d/%d шагов %d %s опыта{x.\n\r",
	    ch->hit,  ch->max_hit,
	    ch->mana, ch->max_mana,
	    ch->move, ch->max_move,
	    ch->exp, hours(ch->exp, TYPE_POINTS));

    send_to_char(buf, ch);

    sprintf(buf, "$n произносит: {CУ меня %d/%d жизни %d/%d маны %d/%d шагов %d %s опыта{x.",
	    ch->hit,  ch->max_hit,
	    ch->mana, ch->max_mana,
	    ch->move, ch->max_move,
	    ch->exp, hours(ch->exp, TYPE_POINTS));

    act(buf, ch, NULL, NULL, TO_ROOM);

    return;
}


/*
 * 'Wimpy' originally by Dionysos.
 */
void do_wimpy(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int wimpy;

    one_argument(argument, arg);

    if (arg[0] == '\0')
	wimpy = ch->max_hit / 5;
    else
	wimpy = atoi(arg);

    if (wimpy < 0)
    {
	send_to_char("Твой кураж превышает твою мудрость.\n\r", ch);
	return;
    }

    if (wimpy > ch->max_hit/5)
    {
	send_to_char("Такая трусость тебе не к лицу.\n\r", ch);
	return;
    }

    ch->wimpy	= wimpy;
    sprintf(buf, "Осторожность установлена в %d %s жизни.\n\r",
	    wimpy, hours(wimpy, TYPE_POINTS));
    send_to_char(buf, ch);
    return;
}



void do_password(CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char *pArg;
    char cEnd;

    if (IS_NPC(ch) || ch->desc == NULL)
	return;

    /*
     * Can't use one_argument here because it smashes case.
     * So we just steal all its code.  Bleagh.
     */
    pArg = arg1;
    while (IS_SPACE(*argument))
	argument++;

    cEnd = ' ';
    if (*argument == '\'' || *argument == '"')
	cEnd = *argument++;

    while (*argument != '\0')
    {
	if (*argument == cEnd)
	{
	    argument++;
	    break;
	}
	*pArg++ = *argument++;
    }
    *pArg = '\0';

    pArg = arg2;
    while (IS_SPACE(*argument))
	argument++;

    cEnd = ' ';
    if (*argument == '\'' || *argument == '"')
	cEnd = *argument++;

    while (*argument != '\0')
    {
	if (*argument == cEnd)
	{
	    argument++;
	    break;
	}
	*pArg++ = *argument++;
    }
    *pArg = '\0';

    if (arg1[0] == '\0' || arg2[0] == '\0')
    {
	send_to_char("Синтаксис: пароль <старый> <новый>.\n\r", ch);
	return;
    }

    if (strcmp(arg1, ch->pcdata->pwd))
    {
	WAIT_STATE(ch, 40);
	send_to_char("Неправильный пароль.  Подождите 10 секунд.\n\r", ch);
	return;
    }


    if (!check_password(ch->desc, arg2))
	return;

    free_string(ch->pcdata->pwd);
    ch->pcdata->pwd = str_dup(arg2);
    save_char_obj(ch, FALSE);
    send_to_char("Ok.\n\r", ch);
    return;
}

char * show_condition(OBJ_DATA *obj)
{
    if (obj->condition > 95) return "превосходно";
    else if (obj->condition > 85) return "отлично";
    else if (obj->condition > 70) return "очень хорошо";
    else if (obj->condition > 45) return "хорошо";
    else if (obj->condition > 20) return "удовлетворительно";
    else if (obj->condition >  5) return "плохо";
    else                          return "распадается";
}

bool is_compatible_races(void *ch1, void *ch2, bool is_char_data)
{
    int i, race1, race2;

    if (is_char_data)
    {
	race1 = ((CHAR_DATA *) ch1)->race;
	race2 = ((CHAR_DATA *) ch2)->race;
    }
    else
    {
	race1 = *((int *) ch1);
	race2 = *((int *) ch2);
    }

    for(i = 0; race_incompatible_table[i].race1 > 0; i++)
	if ((race1 == race_incompatible_table[i].race1
	     && race2 == race_incompatible_table[i].race2)
	    || (race1 == race_incompatible_table[i].race2
		&& race2 == race_incompatible_table[i].race1))
	{
	    return FALSE;
	}

    return TRUE;
}

void do_races(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char bfr[MAX_STRING_LENGTH];
    int r[MAX_PC_RACE];
    int i, j, k;

    if (argument[0] == '\0')
    {
	send_to_char("Этой командой можно проверить совместимость рас.\n\r\n\r"
		     "Синтаксис: расы <раса> <раса>\n\r", ch);
	return;
    }

    for (i = 0, argument = one_argument(argument, arg);
	 arg[0] != '\0' && i < MAX_PC_RACE;
	 argument = one_argument(argument, arg))
    {
	if ((j = race_lookup(arg)) == -1)
	{
	    sprintf(bfr, "Расы '%s' не существует!\n\r", arg);
	    send_to_char(bfr, ch);
	    return;
	}
	r[i++] = j;
    }

    if (i < 2)
    {
	send_to_char("Нужно указать не менее 2 рас!\n\r", ch);
	return;
    }

    for (j = 0; j < i; j++)
    {
	for (k = j + 1; k < i; k++)
	    if (!is_compatible_races(&r[j], &r[k], FALSE))
	    {
		sprintf(bfr, "%s и %s враждуют между собой.\n\r",
			pc_race_table[r[j]].who_name,
			pc_race_table[r[k]].who_name);
		send_to_char(bfr, ch);
		return;
	    }
    }

    send_to_char("Эти расы совместимы.\n\r", ch);

    return;
}

void do_gaffect(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA *paf, *paf_last = NULL;
    char buf[MAX_STRING_LENGTH];
    BUFFER *output;

    output = new_buf();

    if (ch->affected != NULL)
    {
	add_buf(output, "\n\rНа мне:");
	for (paf = ch->affected; paf != NULL; paf = paf->next)
	{
	    if (paf_last != NULL && paf->type == paf_last->type)
		continue;

	    sprintf(buf, "\n\r%-20s", get_skill_name(ch, paf->type, TRUE));
	    add_buf(output, buf);

	    if (ch->level >= 20)
	    {
		if (paf->duration == -1)
		    sprintf(buf, " : постоянно");
		else
		    sprintf(buf, " : %d %s.",
			    paf->duration, hours(paf->duration, TYPE_HOURS));
		add_buf(output, buf);
	    }

	    paf_last = paf;
	}
    }
    else
	add_buf(output, "На мне нет никаких заклинаний.");


    do_function(ch, &do_gtell, output->string);

    free_buf(output);

    return;
}

void do_identify(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj;
    CHAR_DATA *rch;

    if ((obj = get_obj_carry(ch, argument, ch)) == NULL)
    {
	send_to_char("У тебя нет этого!\n\r", ch);
	return;
    }

    LIST_FOREACH(rch, &(ch->in_room->people), room_link)
    {
	if (IS_NPC(rch) && rch->pIndexData->vnum == MOB_VNUM_SAGE)
	    break;
    }

    if (rch == NULL || !can_see(ch, rch))
    {
	send_to_char("Здесь нет никого, кто бы мог тебе помочь.\n\r", ch);
	return;
    }

    if (IS_OBJ_STAT(obj, ITEM_NO_IDENTIFY))
    {
	send_to_char("{aЭтот предмет невозможно опознать.{x\n\r", ch);
	return;
    }

    if (IS_IMMORTAL(ch))
	act("$n хмыкает и как-то странно на тебя смотрит.",
	    rch, NULL, ch, TO_VICT);
    else if (ch->gold < SAGE_COST)
    {
	act("$n продолжает перебирать четки, даже не взглянув на $p6.",
	    rch, obj, 0, TO_ROOM);
	return;
    }
    else
    {
	ch->gold -= SAGE_COST;
	send_to_char("Твой кошелек стал чуть-чуть полегче...\n\r", ch);
    }

    act("$n исследует $p6 и бросает несколько куриных лапок $x.\n\r"
	"Потом он произносит какое-то заклинание.", rch, obj, 0, TO_ROOM);
    spell_identify(0, 0, ch, obj, 0);
}

void do_lookmap(CHAR_DATA *ch, char *argument)
{
    ROOM_INDEX_DATA *location;
    int skill, x, MapDiameter;
    char arg[MAX_INPUT_LENGTH];

    if ((skill = get_skill(ch, gsn_maps)) < 1)
    {
	send_to_char("Ты не силен в картографии.\n\r", ch);
	return;
    }

    if ((location = ch->in_room) == NULL
	|| IS_SET(location->room_flags, ROOM_NO_RECALL)
	|| IS_SET(location->room_flags, ROOM_SAFE)
	|| IS_SET(location->room_flags, ROOM_PRIVATE)
	|| IS_SET(location->room_flags, ROOM_SOLITARY)
	|| affect_find(location->affected, gsn_cursed_lands) != NULL)
    {
	send_to_char("Отсюда не получится исследовать местность.\n\r", ch);
	return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0' || !is_number(arg))
    {
	send_to_char("Ты должен задать размер исследуемого пространства.\n\r",
		     ch);
	return;
    }

    MapDiameter = atoi(arg);

    x = number_range(skill/2, skill);
    x = 2 + x/20;

    MapDiameter = URANGE(1, MapDiameter, x);

    if (ch->mana < MapDiameter * 4 || ch->move < MapDiameter * 4)
    {
	send_to_char("У тебя не хватает энергии.\n\r", ch);
	return;
    }

    act("$n пытается исследовать близлежащую местность.",
	ch, NULL, NULL, TO_ROOM);
    send_to_char("Ты пытаешься исследовать близлежащую местность.\n\r", ch);

    WAIT_STATE(ch, skill_table[gsn_maps].beats);

    if (skill - number_fuzzy(1) < number_percent())
    {
	ch->mana -= MapDiameter * 2;
	ch->move -= MapDiameter * 2;
	send_to_char("Не получается.\n\r", ch);
	check_improve(ch, NULL, gsn_maps, FALSE, UMAX(1, 8 - MapDiameter));
	return;
    }

    sprintf(arg, "Тебе удалось ранее изучить такую местность в пределах %dх%d"
	    " %s вокруг:\n\r\n\r", MapDiameter, MapDiameter, hours(MapDiameter, TYPE_ROOMS));
    send_to_char(arg, ch);

    sprintf(arg, "%d world terrain visited doors", -MapDiameter);
    do_function(ch, &do_map, arg);

    check_improve(ch, NULL, gsn_maps, TRUE, UMAX(1, 8 - MapDiameter));
    ch->mana -= MapDiameter * 4;
    ch->move -= MapDiameter * 4;

    return;
}

char *resist_amount(int res)
{
    if (res < 0)
	res = -res;

    if (res < 10)
	return "Незначительно";
    else if (res < 20)
	return "Едва";
    else if (res < 30)
	return "Слегка";
    else if (res < 40)
	return "Заметно";
    else if (res < 50)
	return "Значительно";
    else if (res < 60)
	return "Сильно";
    else if (res < 75)
	return "Очень сильно";
    else
	return "Колоссально";
}

void obj_lore(CHAR_DATA *ch, OBJ_DATA *obj)
{
    int chance = get_skill(ch, gsn_lore);
    AFFECT_DATA *paf;
    int i, j;
    int v[5];

    if (IS_SET(obj->extra_flags, ITEM_NO_LEGEND))
    {
	send_to_char("Об этом предмете нет никаких сказаний и легенд...\n\r", ch);
	return;
    }

    if (ch->mana < 30)
    {
	send_to_char("У тебя не хватает энергии.\n\r", ch);
	return;
    }

    ch->mana -= 30;

    if (chance < 10)
    {
	send_to_char("Извини, но ты слишком пренебрегал домыслами старых идиотов, "
		     "чтобы сейчас узнать хоть что-то об этом предмете.\n\r", ch);
	return;
    }

    if (number_percent() > chance)
    {
	send_to_char("Ты не можешь вспомнить ничего из услышанного, что могло "
		     "бы иметь отношение к этому предмету.\n\r", ch);
	check_improve(ch, NULL, gsn_lore, FALSE, 6);
	WAIT_STATE(ch, skill_table[gsn_lore].beats * 2);
	return;
    }

    chance = number_range(2 * chance/ 3, chance);

    printf_to_char("Объект: '%s' Вес: %d. Цена: %d.\n\r",
		   ch, obj->short_descr,
		   number_range(obj->weight * chance, obj->weight * (200 - chance)) / 100,
		   number_range(obj->cost * chance, obj->cost * (200 - chance)) / 100);

    if (chance > 20)
    {
	printf_to_char("Уровень: %d. ",
		       ch, number_range(obj->level * chance, obj->level * (200 - chance)) / 100);
    }

    if (chance > 30)
	printf_to_char("Тип: %s.\n\r", ch, item_name(obj->item_type, 0));

    if (chance > 40)
	printf_to_char("Материал: %s\n\r", ch, get_material(obj));

    if (chance > 50)
    {
	int bash, pierce, slash, other;

	i = material_lookup(obj->material);
	if (i != -1)
	{
	    i = material_table[i].bit;
	    for (j = 0; material_waste[j].bash != 0; j++)
		if (material_waste[j].bit == i)
		    break;
	}
	if (i == -1 || material_waste[j].bash == 0)
	    j = 0;

	bash = material_waste[j].bash * 100;
	pierce = material_waste[j].pierce * 100;
	slash = material_waste[j].slash * 100;
	other = material_waste[j].other * 100;

	printf_to_char("Прочность материала к удару: %d\n\r"
		       "                      уколу: %d\n\r"
		       "                    разрубу: %d\n\r"
		       "                 остальному: %d\n\r",
		       ch,
		       number_range(bash * chance, bash * (200 - chance)) / 100,
		       number_range(pierce * chance, pierce * (200 - chance)) / 100,
		       number_range(slash * chance, slash * (200 - chance)) / 100,
		       number_range(other * chance, other * (200 - chance)) / 100);
    }

    if (chance > 60)
	printf_to_char("Флаги: %s.\n\r", ch, flag_string(extra_flags, obj->extra_flags, TRUE));

    if (chance > 70)
    {
	printf_to_char("Неудобно для: %s.\n\r", ch, flag_string(comf_flags, obj->uncomf, TRUE));
	printf_to_char("Запрещено для: %s.\n\r", ch, flag_string(comf_flags, obj->unusable, TRUE));
    }

    if (chance > 80)
    {
	for (i = 0; i < 5; i++)
	    v[i] = number_range(obj->value[i] * chance, obj->value[i] * (200 - chance)) / 100;

	switch (obj->item_type)
	{
	case ITEM_SCROLL:
	case ITEM_PILL:
	case ITEM_POTION:
	    printf_to_char("Заклинания %d уровня:", ch, v[0]);
	    for (i = 1; i < 5; i++)
		if (v[i] > 0 && v[i] < max_skills)
		    printf_to_char(" '%s'", ch, skill_table[v[i]].name);
	    break;
	case ITEM_WAND:
	case ITEM_STAFF:
	    printf_to_char("Имеет %d из %d %s %d уровня",
			   ch, v[2], v[1], hours(v[1], TYPE_CHARGES), v[0]);
	    if (v[3] > 0 && v[3] < max_skills)
		printf_to_char(" '%s'", ch, skill_table[v[3]].name);
	    printf_to_char(".\n\r", ch);
	    break;
	case ITEM_ROD:
	    printf_to_char("Имеет заряд %d уровня с заклинанием '%s'.\n\r",
			   ch, v[0],
			   v[1] > 0 && v[1] < max_skills
			   ? skill_table[v[1]]. name
			   : "unknown");
	    printf_to_char("Время перезарядки: %d(%d) %s.\n\r",
			   ch, v[3], v[2], hours(v[3], TYPE_HOURS));
	    break;
	case ITEM_TRAP:
	    printf_to_char("Урон %dd%d (средний %d), тип повреждений: %s\n\r",
			   ch, v[0], v[1], (1 + v[1]) * v[0] / 2,
			   flag_string(dam_flags, v[3], TRUE));
	    printf_to_char("Задерживает на %d %s.\n\r",
			   ch, v[4] / PULSE_VIOLENCE, hours(v[4] / PULSE_VIOLENCE, TYPE_ROUND));
	    break;
	case ITEM_DRINK_CON:
	    printf_to_char("Содержит %s. Цвет - %s. %sтравлено. Осталось %d из %d.\n\r",
			   ch, liq_table[v[2]].liq_rname, liq_table[v[2]].liq_color,
			   number_percent() < chance
			   ? (number_bits(1) == 1 ? "О" : "Не о")
			   : (obj->value[3] != 0 ? "О" : "Не о"),
			   v[1], v[0]);
	    break;
	case ITEM_CONTAINER:
	    printf_to_char("Вмещаемый вес: %d#   Макс. вес одного предмета: %d#   Флаги: %s\n\r",
			   ch, v[0], v[3], flag_string(container_flags, obj->value[1], TRUE));
	    printf_to_char("Весовой модификатор: %d%%\n\r",
			   ch, v[4]);
	    break;
	case ITEM_CHEST:
	    printf_to_char("Вмещаемый вес: %d#   Макс. вес одного предмета: %d#   Флаги: %s\n\r",
			   ch, v[0], v[3], flag_string(chest_flags, obj->value[1], TRUE));
	    printf_to_char("Весовой модификатор: %d%%\n\r",
			   ch, v[4]);
	    break;
	case ITEM_MORTAR:
	    printf_to_char("Макс. количество предметов: %d\n\r",ch, v[2]);
	    break;
	case ITEM_INGREDIENT:
	    break;
	case ITEM_WEAPON:
	    printf_to_char("Тип оружия: %s. ",
			   ch, flag_string(weapon_class, obj->value[0], TRUE));
	    printf_to_char("Урон: %dd%d (средний %d).\n\r",
			   ch, v[1], v[2], (1 + v[2]) * v[1] / 2);
	    printf_to_char("Тип повреждений: %s. ",
			   ch, flag_string(dam_flags, attack_table[obj->value[3]].damage, TRUE));

	    printf_to_char("Флаги оружия: %s.\n\r",
			   ch, flag_string(weapon_type2, obj->value[4], TRUE));
	    break;
	case ITEM_ARMOR:
	    printf_to_char("Броня: %d от укола, %d от удара, %d от рубки, %d от остального.\n\r",
			   ch, v[0], v[1], v[2], v[3]);
	    break;
	}

	if (is_limit(obj) != -1)
	    send_to_char("\n\rВ этом мире количество этих вещей ограничено.\n\r", ch);

    }

    if (chance > 90)
    {
	int mod, dur, qp = 0;
	BUFFER *output;
	char buf[MSL];

	output = new_buf();
	/* Подумать насчет отнимания qp за эту инфу... */

	if (!obj->enchanted && !obj->pIndexData->edited)
	{
	    for (paf = obj->pIndexData->affected; paf; paf = paf->next)
	    {
		mod = number_range(paf->modifier * chance, paf->modifier * (200 - chance)) / 100;

		if ((paf->location != APPLY_NONE && paf->modifier != 0) || chance > 95)
		{
		    if (paf->location == APPLY_SKILL
			&& paf->type > 0
			&& paf->type < max_skills)
		    {
			sprintf(buf, "Модифицирует умение/заклинание {W%s{x на {W%d%%{x\n\r",
				skill_table[paf->type].name, mod);

			add_buf(output, buf);
		    }
		    else if (paf->location != APPLY_NONE)
		    {
			sprintf(buf, "Модифицирует %s на %d.\n\r",
				flag_string(apply_flags, paf->location, TRUE),
				mod);
			add_buf(output, buf);
		    }

		    if (paf->bitvector && 3 * chance / 4 > number_percent())
			switch (paf->where)
			{
			case TO_AFFECTS:
			    sprintf(buf, "Добавляет эффект: %s.\n\r",
				    flag_string(affect_flags, paf->bitvector, TRUE));
			    add_buf(output, buf);
			    qp += 30;
			    break;
			case TO_OBJECT:
			    sprintf(buf, "Добавляет флаг: %s.\n\r",
				    flag_string(extra_flags, paf->bitvector, TRUE));
			    add_buf(output, buf);
			    qp += 20;
			    break;
			case TO_RESIST:
			    sprintf(buf, "%s %s сопротивление к %s.\n\r",
				    resist_amount(mod),
				    mod < 0 ? "ухудшает" : "улучшает",
				    cases(flag_string(dam_flags, paf->bitvector, TRUE), 2));
			    add_buf(output, buf);
			    qp += 50;
			    break;
			}
		}
	    }
	}

	for (paf = obj->affected; paf; paf = paf->next)
	{
	    mod = number_range(paf->modifier * chance, paf->modifier * (200 - chance)) / 100;
	    dur = number_range(paf->duration * chance, paf->duration * (200 - chance)) / 100;

	    if ((paf->location != APPLY_NONE && paf->modifier != 0) || chance > 95)
	    {
		if (paf->location == APPLY_SKILL
		    && paf->type > 0
		    && paf->type < max_skills)
		{
		    sprintf(buf, "Модифицирует умение/заклинание {W%s{x на {W%d%%{x",
			    skill_table[paf->type].name, mod);
		    add_buf(output, buf);
		}
		else if (paf->location != APPLY_NONE)
		{
		    sprintf(buf, "Модифицирует %s на %d",
			    flag_string(apply_flags, paf->location, TRUE),
			    mod);
		    add_buf(output, buf);
		}

		if (paf->location != APPLY_NONE)
		{
		    if (dur > -1)
		    {
			sprintf(buf, ", %d %s", dur, hours(dur, TYPE_HOURS));
			add_buf(output, buf);
		    }

		    add_buf(output, ".\n\r");
		}

		if (paf->bitvector)
		    switch (paf->where)
		    {
		    case TO_AFFECTS:
			sprintf(buf, "Добавляет эффект: %s.\n\r",
				flag_string(affect_flags, paf->bitvector, TRUE));
			add_buf(output, buf);
			qp += 30;
			break;
		    case TO_OBJECT:
			sprintf(buf, "Добавляет флаг: %s.\n\r",
				flag_string(extra_flags, paf->bitvector, TRUE));
			add_buf(output, buf);
			qp += 20;
			break;
		    case TO_RESIST:
			sprintf(buf, "%s %s сопротивление к %s.\n\r",
				resist_amount(mod),
				mod < 0 ? "ухудшает" : "улучшает",
				flag_string(dam_flags, paf->bitvector, TRUE));
			add_buf(output, buf);
			qp += 50;
			break;
		    case TO_WEAPON:
			sprintf(buf, "Добавляет флаг оружия: %s.\n\r",
				flag_string(weapon_type2, paf->bitvector, TRUE));
			add_buf(output, buf);
			qp += 5;
			break;
		    }
	    }
	}

	if (qp > 0 && qp > ch->pcdata->quest_curr)
	    send_to_char("У тебя не хватает удачи для того, чтобы узнать дополнительную информацию.\n\r", ch);
	else
	{
	    if (qp > 0)
	    {
		ch->pcdata->quest_curr -= qp;
		sprintf(buf, "За дополнительные знания ты теряешь %d %s удачи.\n\r",
			qp, hours(qp, TYPE_POINTS));
		add_buf(output, buf);
	    }
	    page_to_char(buf_string(output), ch);
	}

	free_buf(output);
    }

    j = number_range(obj->condition * chance, obj->condition * (200 - chance)) / 100;

    if (chance > 50)
	printf_to_char("Состояние: %d%%.\n\r", ch, j);
    else
    {
	i = obj->condition;
	obj->condition = j;
	printf_to_char("Состояние: %s.\n\r", ch, show_condition(obj));
	obj->condition = i;
    }

    check_improve(ch, NULL, gsn_lore, TRUE, 6);
    WAIT_STATE(ch, skill_table[gsn_lore].beats);
}

void char_lore(CHAR_DATA *ch, CHAR_DATA *victim)
{
    //	send_to_char("Будет реализовано в ближайшее время.\n\r", ch);
    int chance = get_skill(ch, gsn_lore);

    if (IS_SET(victim->act, ACT_NOLEGEND))
    {
	send_to_char("Об этом существе нет никаких сказаний и легенд...\n\r", ch);
	return;
    }

    if (ch->mana < 30)
    {
	send_to_char("У тебя не хватает энергии.\n\r", ch);
	return;
    }

    ch->mana -= 30;

    if (chance < 15)
    {
	send_to_char("Извини, но ты слишком пренебрегал домыслами старых идиотов, "
		     "чтобы сейчас узнать хоть что-то об этом мобе.\n\r", ch);
	return;
    }

    if (number_percent() > chance)
    {
	send_to_char("Ты не можешь вспомнить ничего из услышанного, что могло "
		     "бы иметь отношение к этому мобу.\n\r", ch);
	check_improve(ch, NULL, gsn_lore, FALSE, 6);
	WAIT_STATE(ch, skill_table[gsn_lore].beats * 2);
	return;
    }

    chance = number_range(2 * chance/ 3, chance);

    printf_to_char("Моб: %s, пол: %s.\n\r",
		   ch, victim->short_descr, sex_table[victim->sex].rname);

    if (chance > 30)
    {
        int sign, align;

        align = victim->alignment;
        sign = align < 0 ? -1 : 1;

        align = sign * number_range(9 * sign * align / 10 - 10, 11 * sign * align / 10 + 10);

        align = URANGE(-1000, align, 1000);
	printf_to_char("Раса: %s. Натура: %d.\n\r", ch, race_table[victim->race].rname, align);
    }

    if (chance > 45)
    {
	printf_to_char("Уровень: %d.\n\r",
		       ch, number_range(victim->level * chance, victim->level * (200 - chance)) / 100);
    }

    if (chance > 52)
    {
	printf_to_char("Золота: %d, Серебра: %d.\n\r",
		       ch, number_fuzzy(victim->gold), number_fuzzy(victim->silver));
    }

    if (chance > 60)
    {
	printf_to_char("Жизни: %d, Маны: %d, Шагов: %d.\n\r",
		       ch, number_fuzzy(victim->hit), number_fuzzy(victim->mana), number_fuzzy(victim->move));
	printf_to_char("Попадание: %d, Урон: %d.\n\r",
		       ch, number_fuzzy(GET_FULL_HITROLL(victim)), number_fuzzy(GET_FULL_DAMROLL(victim)));
    }

    if (chance > 75 && victim->affected_by)
    {
	printf_to_char("Эффекты: %s.\n\r",
		       ch, flag_string(affect_flags, victim->affected_by, TRUE));
    }

    check_improve(ch, NULL, gsn_lore, TRUE, 6);
    WAIT_STATE(ch, skill_table[gsn_lore].beats);
    //подумать - стоит ли вводить вывод информации о резистах/вульнах моба?
    return;
}

void area_lore(CHAR_DATA *ch, AREA_DATA *area)
{
    //    send_to_char("Будет реализовано в ближайшее время.\n\r", ch);
    int chance = get_skill(ch, gsn_lore);

    if (IS_SET(area->area_flags, AREA_NOLEGEND))
    {
	send_to_char("Об этой зоне нет никаких сказаний и легенд...\n\r", ch);
	return;
    }

    if (ch->mana < 30)
    {
	send_to_char("У тебя не хватает энергии.\n\r", ch);
	return;
    }

    ch->mana -= 30;

    if (number_percent() > chance)
    {
	send_to_char("Ты не можешь вспомнить ничего из услышанного, что могло "
		     "бы иметь отношение к этой зоне.\n\r", ch);
	check_improve(ch, NULL, gsn_lore, FALSE, 6);
	WAIT_STATE(ch, skill_table[gsn_lore].beats * 2);
	return;
    }

    chance = number_range(2 * chance/ 3, chance);

    if (chance < 45)
    {
	send_to_char("Извини, но ты слишком пренебрегал домыслами старых идиотов, "
		     "чтобы сейчас узнать хоть что-то об этой зоне.\n\r", ch);
	return;
    }

    printf_to_char("Зона: %s.\n\r", ch, area->name);

    if ( chance > 95)
    {
	printf_to_char("В зоне в данный момент %d %s.\n\r", ch, 
	            area->nplayer, hours(area->nplayer, TYPE_PLAYERS));
    }

    check_improve(ch, NULL, gsn_lore, TRUE, 6);
    WAIT_STATE(ch, skill_table[gsn_lore].beats);
    return;
}

void room_lore(CHAR_DATA *ch, ROOM_INDEX_DATA *room)
{
    int chance = get_skill(ch, gsn_lore);

    if (IS_SET(room->room_flags, ROOM_NOLEGEND))
    {
	send_to_char("Об этом месте нет никаких сказаний и легенд...\n\r", ch);
	return;
    }

    if (ch->mana < 30)
    {
	send_to_char("У тебя не хватает энергии.\n\r", ch);
	return;
    }

    ch->mana -= 30;

    if (chance < 15)
    {
	send_to_char("Извини, но ты слишком пренебрегал домыслами старых идиотов, "
		     "чтобы сейчас узнать хоть что-то об этой комнате.\n\r", ch);
	return;
    }

    if (number_percent() > chance)
    {
	send_to_char("Ты не можешь вспомнить ничего из услышанного, что могло "
		     "бы иметь отношение к этому месту.\n\r", ch);
	check_improve(ch, NULL, gsn_lore, FALSE, 6);
	WAIT_STATE(ch, skill_table[gsn_lore].beats * 2);
	return;
    }

    chance = number_range(2 * chance/ 3, chance);

    printf_to_char("Местечко: '%s'\n\r",
		   ch, room->name);

    if (chance > 50)
	printf_to_char("Тип местности: %s.\n\r", ch, flag_string(sector_flags, room->sector_type, TRUE));

    if (chance > 80)
	printf_to_char("Дополнительная информация: %s\n\r", ch, flag_string(room_flags, room->room_flags, TRUE));

    if (chance > 95)
	printf_to_char("Восстановление: %d%%\n\r", ch, room->heal_rate);

    check_improve(ch, NULL, gsn_lore, TRUE, 6);
    WAIT_STATE(ch, skill_table[gsn_lore].beats);
    return;
}

void do_lore(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    CHAR_DATA *mob;

    if (IS_NPC(ch))
    {
	send_to_char("Мобы не знают легенд.\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);
    if (!str_prefix(arg, "object")
	|| !str_prefix(arg, "объект"))
    {
	if ((obj = get_obj_carry(ch, argument, ch)) == NULL)
	{
	    send_to_char("У тебя нет ничего похожего.\n\r", ch);
	    return;
	}
	obj_lore(ch, obj);
    }
    else if (!str_prefix(arg, "mobile")
	     || !str_prefix(arg, "моб")
	     || !str_prefix(arg, "монстр"))
    {
	if ((mob = get_char_room(ch, NULL, argument, FALSE)) == NULL)
	{
	    send_to_char("Ты не видишь тут никого похожего.\n\r", ch);
	    return;
	}

	if (!IS_NPC(mob))
	{
	    send_to_char("Это - не моб.\n\r", ch);
	    return;
	}

	char_lore(ch, mob);
    }
    else if (!str_prefix(arg, "area")
	     || !str_prefix(arg, "ария")
	     || !str_prefix(arg, "зона"))
    {
	if (!ch->in_room || !ch->in_room->area)
	    return;

	area_lore(ch, ch->in_room->area);
    }
    else if (!str_prefix(arg, "room")
	     || !str_prefix(arg, "комната"))
    {
	if (!ch->in_room)
	    return;

	room_lore(ch, ch->in_room);
    }
    else
	do_function(ch, &do_help, "lore");
}

/* charset=cp1251 */

