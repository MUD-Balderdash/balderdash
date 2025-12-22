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



char *const distance[4]=
{
    "пр€мо здесь.", "р€дом %s%s.", "недалеко %s%s.", "вдалеке %s%s."
};

void scan_list           args((ROOM_INDEX_DATA *scan_room, CHAR_DATA *ch, int16_t depth, int16_t door));
void scan_char           args((CHAR_DATA *victim, CHAR_DATA *ch, int16_t depth, int16_t door));


void show_scan(CHAR_DATA *ch, int door, int depth_max)
{
    char arg1[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *scan_room;
    EXIT_DATA *pExit;
    int depth;

    sprintf(arg1,"%s%s", (door==DIR_UP || door==DIR_DOWN) ? "в" : "на ",
	    dir_name[door]);

    sprintf(buf,"“ы всматриваешьс€ %s.\n\r",arg1);
    send_to_char(buf, ch);

    sprintf(buf,"$n всматриваетс€ %s.",arg1);
    act(buf, ch, NULL, NULL, TO_ROOM);

    sprintf(buf, "√л€д€ %s, ты видишь:\n\r"
	    "-------------------------------------------\n\r", arg1);
    send_to_char(buf,ch);

    scan_room = ch->in_room;

    for (depth = 1; depth <= depth_max; depth++)
    {
	if ((pExit = scan_room->exit[door]) != NULL
	    && !IS_SET(pExit->exit_info, EX_CLOSED))
	{
	    if (!IS_NPC(ch) && IS_SET(pExit->exit_info, EX_RACE))
		scan_room = get_room_index(pc_race_table[ch->race].recall);
	    else
		scan_room = pExit->u1.to_room;
	    scan_list(scan_room, ch, depth, door);
	}
	else
	{
	    send_to_char("“ам ничего не видно.\n\r",ch);
	    return;
	}
    }
}

void do_scan(CHAR_DATA *ch, char *argument)
{
    ROOM_INDEX_DATA *scan_room;
    int16_t door;
    char arg1[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg1);

    if (arg1[0] == '\0')
    {
	EXIT_DATA *pExit;

	act("$n осматриваетс€ вокруг.", ch, NULL, NULL, TO_ROOM);
	send_to_char("ќсмотревшись вокруг, ты видишь:\n\r-------------------------------------------\n\r", ch);
	scan_list(ch->in_room, ch, 0, -1);

	for (door=0;door<6;door++)
	{
	    if ((pExit = ch->in_room->exit[door]) != NULL
		&& !IS_SET(pExit->exit_info, EX_CLOSED))
	    {
		if (!IS_NPC(ch) && IS_SET(pExit->exit_info, EX_RACE))
		    scan_room = get_room_index(pc_race_table[ch->race].recall);
		else
		    scan_room = pExit->u1.to_room;

		scan_list(scan_room, ch, 1, door);
	    }
	}
	return;
    }
    else 
	if ((door = what_door(arg1)) == -1)
	{
	    send_to_char("¬ какую сторону ты хотел посмотреть?\n\r", ch);
	    return;
	}

    show_scan(ch, door, 3);

    return;
}

void do_good_look(CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    int16_t door;

    argument = one_argument(argument, arg1);

    if (arg1[0] == '\0' || (door = what_door(arg1)) == -1)
    {
	act("¬ какую сторону ты хочешь посмотреть?", ch, NULL, NULL, TO_CHAR);
	return;
    }

    show_scan(ch, door, 1);

    return;
}


void scan_list(ROOM_INDEX_DATA *scan_room, CHAR_DATA *ch, int16_t depth, int16_t door)
{
    CHAR_DATA *rch, *safe_rch;

    if (scan_room == NULL || door < 0 
	|| scan_room->exit[rev_dir[door]] == NULL 
	|| IS_SET(scan_room->exit[rev_dir[door]]->exit_info,EX_CLOSED) 
	|| !can_see_room(ch, scan_room))
	return;

    LIST_FOREACH_SAFE(rch, &scan_room->people, room_link, safe_rch)
    {
	ROOM_INDEX_DATA *old_room;

	if (rch == ch) 
	    continue;

	if (!IS_NPC(rch) && rch->invis_level > get_trust(ch)) 
	    continue;

	old_room = ch->in_room;

	ch->in_room = rch->in_room;

	if (can_see(ch, rch))
	{ 
	    ch->in_room = old_room;
	    scan_char(rch, ch, depth, door);
	}
	else
	    ch->in_room = old_room;
    }
    return;
}

void scan_char(CHAR_DATA *victim, CHAR_DATA *ch, int16_t depth, int16_t door)
{
    extern char *const distance[];
    char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

    buf[0] = '\0';

    strcat(buf, PERS(victim, ch, 0));
    strcat(buf, ", ");
    sprintf(buf2, distance[depth], (door==DIR_UP || door==DIR_DOWN) ? "в" : "на ", dir_name[door]);
    strcat(buf, buf2);

    act(buf, ch, NULL, NULL, TO_CHAR);
    return;
}
/* charset=cp1251 */
