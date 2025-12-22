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
 *	ROM 2.4 is copyright 1993-1995 Russ Taylor			   *
 *	ROM has been brought to you by the ROM consortium		   *
 *	    Russ Taylor (rtaylor@pacinfo.com)				   *
 *	    Gabrielle Taylor (gtaylor@pacinfo.com)			   *
 *	    Brian Moore (rom@rom.efn.org)				   *
 *	By using this code, you have agreed to follow the terms of the	   *
 *	ROM license, in the file Rom24/doc/rom.license			   *
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *  Based on MERC 2.2 MOBprograms by N'Atas-ha.                            *
 *  Written and adapted to ROM 2.4 by                                      *
 *          Markku Nylander (markku.nylander@uta.fi)                       *
 *                                                                         *
 ***************************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"
#include "mob_cmds.h"
#include "interp.h"
#include "olc.h"



DECLARE_DO_FUN(do_look);
extern ROOM_INDEX_DATA *find_location(CHAR_DATA *, char *);
int find_door(CHAR_DATA *ch, char *arg);
int find_door_room(ROOM_INDEX_DATA *room, char *arg);
void show_triggers(CHAR_DATA *ch, PROG_LIST *progs);

extern bool mob_holylight;


/*
 * MOBcommand table.
 */
const	struct	mob_cmd_type	mob_cmd_table	[] =
{
    {	"asound", 	do_mpasound	},
    {	"gecho",	do_mpgecho	},
    {	"zecho",	do_mpzecho	},
    {	"kill",		do_mpkill	},
    {	"assist",	do_mpassist	},
    {	"junk",		do_mpjunk	},
    {	"echo",		do_mpecho	},
    {	"echoaround",	do_mpechoaround	},
    {	"echoat",	do_mpechoat	},
    {	"mload",	do_mpmload	},
    {	"oload",	do_mpoload	},
    {	"purge",	do_mppurge	},
    {	"goto",		do_mpgoto	},
    {	"at",		do_mpat		},
    {	"transfer",	do_mptransfer	},
    {	"gtransfer",	do_mpgtransfer	},
    {	"otransfer",	do_mpotransfer	},
    {	"force",	do_mpforce	},
    {	"gforce",	do_mpgforce	},
    {	"vforce",	do_mpvforce	},
    {	"cast",		do_mpcast	},
    {	"damage",	do_mpdamage	},
    {	"remember",	do_mpremember	},
    {	"forget",	do_mpforget	},
    {	"delay",	do_mpdelay	},
    {	"cancel",	do_mpcancel	},
	{	"peace", 	do_mppeace	},
    {	"call",		do_mpcall	},
    {	"flee",		do_mpflee	},
    {	"remove",	do_mpremove	},
    {	"setskill",	do_mpsetskill	},
    {	"restore",	do_mprestore	},
    {	"door",		do_mpdoor	},
    {	"log",		do_mplog	},
    {	"holylight",	do_mpholylight  },
    {	"addapply",	do_mpaddapply   },
    {	"lag",		do_mplag	},
    {	"",		0		}
};

/*
 * OBJcommand table.
 */
const	struct	obj_cmd_type	obj_cmd_table	[] =
{
    {	"gecho",       	do_opgecho	},
    {	"zecho",	do_opzecho	},
    {	"echo",		do_opecho	},
    {	"echoaround",	do_opechoaround	},
    {	"echoat",	do_opechoat	},
    {	"mload",	do_opmload	},
    {	"oload",	do_opoload	},
    {	"purge",	do_oppurge	},
    {	"goto",		do_opgoto	},
    {	"transfer",	do_optransfer	},
    {	"gtransfer",	do_opgtransfer	},
    {	"otransfer",	do_opotransfer	},
    {	"force",	do_opforce	},
    {	"gforce",	do_opgforce	},
    {	"vforce",	do_opvforce	},
	{	"peace", 	do_oppeace	},
    {	"damage",	do_opdamage	},
    {	"remember",	do_opremember	},
    {	"forget",	do_opforget	},
    {	"delay",	do_opdelay	},
    {	"cancel",	do_opcancel	},
    {	"call",		do_opcall	},
    {	"remove",	do_opremove	},
    {	"attrib",	do_opattrib 	},
    {	"log",		do_oplog	},
    {	"flags",	do_opflags	},
    {	"lag",		do_oplag	},
    {	"",		0		}
};

/*
 * ROOMcommand table.
 */
const	struct  room_cmd_type	room_cmd_table	[] =
{
    {	"asound",	do_rpasound	},
    {	"gecho",	do_rpgecho	},
    {	"zecho",	do_rpzecho	},
    {	"echo",		do_rpecho 	},
    {	"echoaround",	do_rpechoaround },
    {	"echoat",	do_rpechoat 	},
    {	"mload",	do_rpmload 	},
    {	"oload",	do_rpoload 	},
    {	"purge",	do_rppurge 	},
    {	"transfer",	do_rptransfer 	},
    {	"gtransfer",	do_rpgtransfer 	},
    {	"otransfer",	do_rpotransfer 	},
    {	"force",	do_rpforce 	},
    {	"gforce",	do_rpgforce 	},
	{	"peace",	do_rppeace	},
    {	"vforce",	do_rpvforce 	},
    {	"damage",       do_rpdamage 	},
    {	"remember",	do_rpremember 	},
    {	"forget",	do_rpforget 	},
    {	"delay",	do_rpdelay 	},
    {	"cancel",	do_rpcancel 	},
    {	"call",		do_rpcall 	},
    {	"remove",	do_rpremove 	},
    {	"door",		do_rpdoor 	},
    {	"log",		do_rplog	},
    {	"lag",		do_rplag	},
    {	"",		0 		},
};

char *prog_type_to_name(int type)
{
    switch (type)
    {
    case TRIG_ACT:             	return "ACT";
    case TRIG_SPEECH:          	return "SPEECH";
    case TRIG_RANDOM:          	return "RANDOM";
    case TRIG_FIGHT:           	return "FIGHT";
    case TRIG_HPCNT:           	return "HPCNT";
    case TRIG_DEATH:           	return "DEATH";
    case TRIG_ENTRY:           	return "ENTRY";
    case TRIG_GREET:           	return "GREET";
    case TRIG_GRALL:        	return "GRALL";
    case TRIG_GIVE:            	return "GIVE";
    case TRIG_BRIBE:           	return "BRIBE";
    case TRIG_KILL:	      	return "KILL";
    case TRIG_DELAY:           	return "DELAY";
    case TRIG_SURR:	      	return "SURRENDER";
    case TRIG_EXIT:	      	return "EXIT";
    case TRIG_EXALL:	      	return "EXALL";
    case TRIG_GET:		return "GET";
    case TRIG_DROP:		return "DROP";
    case TRIG_SIT:		return "SIT";
    case TRIG_WEAR:		return "WEAR";
    case TRIG_COMMAND:		return "COMMAND";
    case TRIG_NEW_CMD:		return "NEW_CMD";
    case TRIG_POST_CMD:		return "POST_CMD";
    case TRIG_QUIT:		return "QUIT";
    case TRIG_PUT:		return "PUT";
    default:                  	return "ERROR";
    }
}

bool check_imm_cmd(CHAR_DATA *ch, char *argument)
{
    char arg[MIL];
    int cmd;

    ch = IS_SWITCHED(ch) ? ch->desc->original : ch;

    if (IS_NPC(ch) || get_trust(ch) < LEVEL_IMMORTAL)
        return FALSE;

    one_argument(argument, arg);

    for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++)
	if (!str_prefix(arg, cmd_table[cmd].name)
	    && cmd_table[cmd].level > LEVEL_HERO)
	{
	    return TRUE;
	}

    return FALSE;
}

/*
 * MOBprog section
 */

void do_mob(CHAR_DATA *ch, char *argument)
{
    /*
     * Security check!
     */
    if (ch->desc != NULL && get_trust(ch) < MAX_LEVEL)
	return;
    mob_interpret(ch, argument);
}
/*
 * Mob command interpreter. Implemented separately for security and speed
 * reasons. A trivial hack of interpret()
 */
void mob_interpret(CHAR_DATA *ch, char *argument)
{
    char command[MAX_INPUT_LENGTH], buf[MIL];
    int cmd;

    argument = one_argument(argument, command);

    while (!str_cmp(command, "mob"))
	argument = one_argument(argument, command);

    /*
     * Look for command in command table.
     */

    if (IS_NPC(ch) && !IS_SWITCHED(ch))
	for (cmd = 0; mob_cmd_table[cmd].name[0] != '\0'; cmd++)
	{
	    if (command[0] == mob_cmd_table[cmd].name[0]
		&& !str_prefix(command, mob_cmd_table[cmd].name))
	    {
		(*mob_cmd_table[cmd].do_fun) (ch, argument);
		tail_chain();
		return;
	    }
	}
    /*        bugf("Mob_interpret: invalid cmd from mob %d: '%s'",
     IS_NPC(ch) ? ch->pIndexData->vnum : 0, command);  */

    sprintf(buf, "%s %s", command, argument);
    interpret(ch, buf);

    return;
}

/*
 * Displays MOBprogram triggers of a mobile
 *
 * Syntax: mpstat [name]
 */
void do_mpstat(CHAR_DATA *ch, char *argument)
{
    char        arg[ MAX_STRING_LENGTH  ];
    CHAR_DATA   *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Mpstat whom?\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
	send_to_char("No such creature.\n\r", ch);
	return;
    }

    if (!IS_NPC(victim) || !victim->pIndexData)
    {
	send_to_char("That is not a mobile.\n\r", ch);
	return;
    }

    sprintf(arg, "Mobile #%-6d [%s]\n\r",
	    victim->pIndexData->vnum, victim->short_descr);
    send_to_char(arg, ch);

    sprintf(arg, "Delay   %-6d [%s]\n\r",
	    victim->mprog_delay,
	    victim->mprog_target == NULL
	    ? "No target" : victim->mprog_target->name);
    send_to_char(arg, ch);

    if (!victim->pIndexData->progs || !victim->pIndexData->progs->trig_flags)
    {
	send_to_char("[No programs set]\n\r", ch);
	return;
    }

    show_triggers(ch, victim->pIndexData->progs);

    return;

}

/*
 * Displays the source code of a given MOBprogram
 *
 * Syntax: mpdump [vnum]
 */
void do_mpdump(CHAR_DATA *ch, char *argument)
{
    char buf[ MAX_INPUT_LENGTH ];
    PROG_CODE *mprg;

    one_argument(argument, buf);
    if ((mprg = get_prog_index(atoi(buf), PRG_MPROG)) == NULL)
    {
	send_to_char("No such MOBprogram.\n\r", ch);
	return;
    }
    page_to_char(mprg->code, ch);
}

/*
 * Prints the argument to all active players in the game
 *
 * Syntax: mob gecho [string]
 */
void do_mpgecho(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
	bugf("MpGEcho: missing argument from vnum %d",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }

    SLIST_FOREACH(d, &descriptor_list, link)
    {
	if (d->connected == CON_PLAYING)
	{
	    if (IS_IMMORTAL(d->character))
		send_to_char("Mob echo> ", d->character);
	    send_to_char(argument, d->character);
	    send_to_char("\n\r", d->character);
	}
    }
}

/*
 * Prints the argument to all players in the same area as the mob
 *
 * Syntax: mob zecho [string]
 */
void do_mpzecho(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
	bugf("MpZEcho: missing argument from vnum %d",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }

    if (ch->in_room == NULL)
	return;

    SLIST_FOREACH(d, &descriptor_list, link)
    {
	if (d->connected == CON_PLAYING
	    && d->character->in_room != NULL
	    && d->character->in_room->area == ch->in_room->area
	    && d->character->in_room->clan == ch->in_room->clan)
	{
	    if (IS_IMMORTAL(d->character))
		send_to_char("Mob echo> ", d->character);
	    send_to_char(argument, d->character);
	    send_to_char("\n\r", d->character);
	}
    }
}

void do_mprestore(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA *paf, *paf_next;

    for (paf = ch->affected; paf; paf = paf_next)
    {
	paf_next = paf->next;
	affect_remove(ch, paf);
    }
}

/*
 * Prints the argument to all the rooms aroud the mobile
 *
 * Syntax: mob asound [string]
 */
void do_mpasound(CHAR_DATA *ch, char *argument)
{

    ROOM_INDEX_DATA *was_in_room;
    int              door;

    if (argument[0] == '\0')
	return;

    was_in_room = ch->in_room;
    for (door = 0; door < 6; door++)
    {
	EXIT_DATA       *pexit;

	if ((pexit = was_in_room->exit[door]) != NULL
	    && pexit->u1.to_room != NULL
	    && pexit->u1.to_room != was_in_room)
	{
	    ch->in_room = pexit->u1.to_room;
	    MOBtrigger  = FALSE;
	    act(argument, ch, NULL, NULL, TO_ROOM);
	    MOBtrigger  = TRUE;
	}
    }
    ch->in_room = was_in_room;
    return;

}

/*
 * Lets the mobile kill any player or mobile without murder
 *
 * Syntax: mob kill [victim]
 */
void do_mpkill(CHAR_DATA *ch, char *argument)
{
    char      arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0')
	return;

    if ((victim = get_char_room(ch, NULL, arg, FALSE)) == NULL)
	return;

    if (victim == ch || IS_NPC(victim)
	|| POS_FIGHT(ch))
	return;

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
    {
	bugf("MpKill - Charmed mob attacking master from vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }

    multi_hit(ch, victim, TYPE_UNDEFINED);
    return;
}

/*
 * Lets the mobile assist another mob or player
 *
 * Syntax: mob assist [character]
 */
void do_mpassist(CHAR_DATA *ch, char *argument)
{
    char      arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0')
	return;

    if ((victim = get_char_room(ch, NULL, arg, FALSE)) == NULL)
	return;

    if (victim == ch || ch->fighting != NULL || victim->fighting == NULL)
	return;

    multi_hit(ch, victim->fighting, TYPE_UNDEFINED);
    return;
}


/*
 * Lets the mobile destroy an object in its inventory
 * it can also destroy a worn object and it can destroy
 * items using all.xxxxx or just plain all of them
 *
 * Syntax: mob junk [item]
 */

void do_mpjunk(CHAR_DATA *ch, char *argument)
{
    char      arg[ MAX_INPUT_LENGTH ];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    one_argument(argument, arg);

    if (arg[0] == '\0')
	return;

    if (str_cmp(arg, "all") && str_prefix("all.", arg))
    {
	if (is_number(arg))
	{
	    char buf[MIL];

	    sprintf(buf, "all.%s", arg);
	    do_function(ch, &do_mpjunk, buf);
	    return;
	}

	if ((obj = get_obj_wear(ch, arg, TRUE)) != NULL)
	{
	    unequip_char(ch, obj, TRUE);
	    extract_obj(obj, TRUE, FALSE);
	    return;
	}

	if ((obj = get_obj_carry(ch, arg, ch)) != NULL)
	    extract_obj(obj, TRUE, TRUE);

	return;
    }
    else
    {
	OBJ_INDEX_DATA *pObj = NULL;
	int vnum = -1;

	if (arg[3] != '\0' && is_number(&arg[4]))
	{
	    vnum = atoi(&arg[4]);
	    pObj = get_obj_index(vnum);
	}

	for (obj = ch->carrying; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;

	    if ((pObj && obj->pIndexData == pObj) || arg[3] == '\0' || is_name(&arg[4], obj->name))
	    {
		if (obj->wear_loc != WEAR_NONE)
		    unequip_char(ch, obj, TRUE);
		extract_obj(obj, TRUE, FALSE);
	    }
	}
    }
    return;

}

/*
 * Prints the message to everyone in the room other than the mob and victim
 *
 * Syntax: mob echoaround [victim] [string]
 */

void do_mpechoaround(CHAR_DATA *ch, char *argument)
{
    char       arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
	return;

    if ((victim=get_char_room(ch, NULL, arg, FALSE)) == NULL)
	return;

    act(argument, ch, NULL, victim, TO_NOTVICT);
}

/*
 * Prints the message to only the victim
 *
 * Syntax: mob echoat [victim] [string]
 */
void do_mpechoat(CHAR_DATA *ch, char *argument)
{
    char       arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
	return;

    if ((victim = get_char_room(ch, NULL, arg, FALSE)) == NULL)
	return;

    act(argument, ch, NULL, victim, TO_VICT);
}

/*
 * Prints the message to the room at large
 *
 * Syntax: mpecho [string]
 */
void do_mpecho(CHAR_DATA *ch, char *argument)
{
    if (argument[0] == '\0')
	return;
    act(argument, ch, NULL, NULL, TO_ROOM);
    act(argument, ch, NULL, NULL, TO_CHAR);
}

/*
 * Lets the mobile load another mobile.
 *
 * Syntax: mob mload [<number>.]<vnum> [<vnum2>]
 */
void do_mpmload(CHAR_DATA *ch, char *argument){
    char            arg[ MAX_INPUT_LENGTH ] = "", arg2[MSL];
    MOB_INDEX_DATA *pMobIndex;
    CHAR_DATA      *victim;
    int vnum, vnum1, vnum2, num, i;

    argument = one_argument(argument, arg);
    num = number_argument(arg, arg);
    argument = one_argument(argument, arg2);

    if (ch->in_room == NULL || arg[0] == '\0' || !is_number(arg))
	return;

    vnum1 = atoi(arg);

    if (!IS_NULLSTR(arg2) && is_number(arg2))
	vnum2 = atoi(arg2);
    else
	vnum2 = vnum1;

    vnum = number_range(vnum1, vnum2);

    if ((pMobIndex = get_mob_index(vnum)) == NULL)
    {
	bugf("Mpmload: bad mob index (%d) from mob %d",
	     vnum, IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }

    for(i = 0; i < num; i++)
    {
	victim = create_mobile(pMobIndex);
	char_to_room(victim, ch->in_room, FALSE);
    }

    return;
}

/*
 * Lets the mobile load an object
 *
 * Syntax: mob oload [vnum] [timer] {R}
 */
void do_mpoload(CHAR_DATA *ch, char *argument)
{
    char arg1[ MAX_INPUT_LENGTH ];
    char arg2[ MAX_INPUT_LENGTH ];
    char arg3[ MAX_INPUT_LENGTH ];
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA       *obj;
    int             timer;
    bool            fToroom = FALSE, fWear = FALSE;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    one_argument(argument, arg3);

    if (arg1[0] == '\0' || !is_number(arg1))
    {
	bugf("Mpoload - Bad syntax from vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }

    if (arg2[0] == '\0')
    {
	timer = 0;
    }
    else
    {
	/*
	 * New feature from Alander.
	 */
	if (!is_number(arg2))
	{
	    /* load money */

	    if (!strcmp(arg2,"gold"))
	    {
		ch->gold+=atoi(arg1);
		return;
	    }

	    if (!strcmp(arg2,"silver"))
	    {
		ch->silver+=atoi(arg1);
		return;
	    }

	    bugf("Mpoload - Bad syntax from vnum %d.",
		 IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	    return;
	}
	timer = atoi(arg2);
    }

    /*
     * Added 3rd argument
     * omitted - load to mobile's inventory
     * 'R'     - load to room
     * 'W'     - load to mobile and force wear
     */
    if (UPPER(arg3[0]) == 'R')
	fToroom = TRUE;
    else if (UPPER(arg3[0]) == 'W')
	fWear = TRUE;

    if ((pObjIndex = get_obj_index(atoi(arg1))) == NULL)
    {
	bugf("Mpoload - Bad vnum arg from vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }

    if (check_prog_limit(pObjIndex))
	return;

    obj = create_object(pObjIndex, 0);
    if ((fWear || !fToroom) && CAN_WEAR(obj, ITEM_TAKE))
    {
	obj_to_char(obj, ch);
	if (fWear)
	{
	    Show_output = FALSE;
	    wear_obj(ch, obj, TRUE);
	    Show_output = TRUE;
	}
    }
    else
    {
	obj_to_room(obj, ch->in_room);
    }

    obj->timer = timer;

    return;
}

/*
 * Lets the mobile purge all objects and other npcs in the room,
 * or purge a specified object or mob in the room. The mobile cannot
 * purge itself for safety reasons.
 *
 * syntax mob purge {target}
 */
void do_mppurge(CHAR_DATA *ch, char *argument)
{
    char       arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;
    OBJ_DATA  *obj;

    one_argument(argument, arg);

    if (!ch->in_room)
        return;

    if (arg[0] == '\0')
    {
	/* 'purge' */
	CHAR_DATA *vnext;
	OBJ_DATA  *obj_next;

	LIST_FOREACH_SAFE(victim, &ch->in_room->people, room_link, vnext)
	{
	    if (IS_NPC(victim) && victim != ch
		&& !IS_SET(victim->act, ACT_NOPURGE))
		extract_char(victim, TRUE);
	}

	for (obj = ch->in_room->contents; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;
	    if (!IS_SET(obj->extra_flags, ITEM_NOPURGE))
		extract_obj(obj, TRUE, FALSE);
	}

	return;
    }

    if ((victim = get_char_room(NULL, ch->in_room, arg, FALSE)) == NULL)
    {
	if ((obj = get_obj_here(NULL, ch->in_room, arg)))
	{
	    extract_obj(obj, TRUE, FALSE);
	}
	else
	{
	    bugf("Mppurge - Bad argument from vnum %d.",
		 IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	}
	return;
    }

    if (!IS_NPC(victim))
    {
	bugf("Mppurge - Purging a PC from vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }
    extract_char(victim, TRUE);
    return;
}


/*
 * Lets the mobile goto any location it wishes that is not private.
 *
 * Syntax: mob goto [location]
 */
void do_mpgoto(CHAR_DATA *ch, char *argument)
{
    char             arg[ MAX_INPUT_LENGTH ];
    ROOM_INDEX_DATA *location;

    one_argument(argument, arg);
    if (arg[0] == '\0')
    {
	bugf("Mpgoto - No argument from vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }

    if ((location = find_location(ch, arg)) == NULL)
    {
	bugf("Mpgoto - No such location from vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }

    if (ch->fighting != NULL)
	stop_fighting(ch, TRUE);

    char_from_room(ch);
    char_to_room(ch, location, FALSE);

    return;
}

/*
 * Lets the mobile do a command at another location.
 *
 * Syntax: mob at [location] [commands]
 */
void do_mpat(CHAR_DATA *ch, char *argument)
{
    char             arg[ MAX_INPUT_LENGTH ];
    ROOM_INDEX_DATA *location;
    ROOM_INDEX_DATA *original;
    CHAR_DATA       *wch, *wch_next;
    OBJ_DATA 	    *on;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
	bugf("Mpat - Bad argument from vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }

    if ((location = find_location(ch, arg)) == NULL)
    {
	bugf("Mpat - No such location from vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }

    original = ch->in_room;
    on = ch->on;
    char_from_room(ch);
    char_to_room(ch, location, FALSE);
    interpret(ch, argument);

    /*
     * See if 'ch' still exists before continuing!
     * Handles 'at XXXX quit' case.
     */
    LIST_FOREACH_SAFE(wch, &char_list, link, wch_next)
    {
	if (wch == ch)
	{
	    OBJ_DATA *obj;

	    char_from_room(ch);
	    char_to_room(ch, original, FALSE);

	    /* See if on still exists before setting ch->on back to it. */
	    for (obj = original->contents; obj; obj = obj->next_content)
	    {
		if (obj == on)
		{
		    ch->on = on;
		    break;
		}
	    }
	    break;
	}
    }

    return;
}

/*
 * Lets the mobile transfer people.  The 'all' argument transfers
 *  everyone in the current room to the specified location
 *
 * Syntax: mob transfer [target|'all'] [location]
 */
void do_mptransfer(CHAR_DATA *ch, char *argument)
{
    char             arg1[ MAX_INPUT_LENGTH ];
    char             arg2[ MAX_INPUT_LENGTH ];
    char	     buf[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA *location;
    CHAR_DATA       *victim;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0')
    {
	bugf("Mptransfer - Bad syntax from vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }

    if (!str_cmp(arg1, "all"))
    {
	CHAR_DATA *victim_next;

	LIST_FOREACH_SAFE(victim, &ch->in_room->people, room_link, victim_next)
	{
	    if (!IS_NPC(victim))
	    {
		sprintf(buf, "'%s' %s", victim->name, arg2);
		do_mptransfer(ch, buf);
	    }
	}
	return;
    }

    /*
     * Thanks to Grodyn for the optional location parameter.
     */
    if (arg2[0] == '\0')
    {
	location = ch->in_room;
    }
    else
    {
	if ((location = find_location(ch, arg2)) == NULL)
	{
	    bugf("Mptransfer - No such location from vnum %d.",
		 IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	    return;
	}

	if (room_is_private(location, NULL))
	    return;
    }

    if ((victim = get_char_world(ch, arg1)) == NULL)
	return;

    if (victim->in_room == NULL)
	return;

    if (victim->fighting != NULL)
	stop_fighting(victim, TRUE);

    char_from_room(victim);
    char_to_room(victim, location, TRUE);

    /*    do_look(victim, "auto");*/

    return;
}

/*
 * Lets the mobile transfer all chars in same group as the victim.
 *
 * Syntax: mob gtransfer [victim] [location]
 */
void do_mpgtransfer(CHAR_DATA *ch, char *argument)
{
    char             arg1[ MAX_INPUT_LENGTH ];
    char             arg2[ MAX_INPUT_LENGTH ];
    char	     buf[MAX_STRING_LENGTH];
    CHAR_DATA       *who, *victim, *victim_next;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0')
    {
	bugf("Mpgtransfer - Bad syntax from vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }

    if ((who = get_char_room(ch, NULL, arg1, FALSE)) == NULL)
	return;

    LIST_FOREACH_SAFE(victim, &ch->in_room->people, room_link, victim_next)
    {
	if (is_same_group(who,victim))
	{
	    sprintf(buf, "'%s' %s", victim->name, arg2);
	    do_mptransfer(ch, buf);
	}
    }
    return;
}

/*
 * Lets the mobile force someone to do something. Must be mortal level
 * and the all argument only affects those in the room with the mobile.
 *
 * Syntax: mob force [victim] [commands]
 */
void do_mpforce(CHAR_DATA *ch, char *argument)
{
    char arg[ MAX_INPUT_LENGTH ];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
	bugf("Mpforce - Bad syntax from vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }

    if (!str_cmp(arg, "all"))
    {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;

	LIST_FOREACH_SAFE(vch, &char_list, link, vch_next)
	{
	    if (vch->in_room == ch->in_room
		&& get_trust(vch) < get_trust(ch)
		&& can_see(ch, vch)
		&& !check_imm_cmd(vch, argument))
	    {
		mob_interpret(vch, argument);
	    }
	}

    }
    else
    {
	CHAR_DATA *victim;

	if ((victim = get_char_room(ch, NULL, arg, FALSE)) == NULL)
	    return;

	if (victim == ch || check_imm_cmd(victim, argument))
	    return;

	mob_interpret(victim, argument);
    }

    return;
}

/*
 * Lets the mobile force a group something. Must be mortal level.
 *
 * Syntax: mob gforce [victim] [commands]
 */
void do_mpgforce(CHAR_DATA *ch, char *argument)
{
    char arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim, *vch, *vch_next;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
	bugf("MpGforce - Bad syntax from vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }

    if ((victim = get_char_room(ch, NULL, arg, FALSE)) == NULL)
	return;

    if (victim == ch)
	return;

    LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, vch_next)
    {
	if (is_same_group(victim, vch) && !check_imm_cmd(vch, argument))
	{
	    interpret(vch, argument);
	}
    }
    return;
}

/*
 * Forces all mobiles of certain vnum to do something (except ch)
 *
 * Syntax: mob vforce [vnum] [commands]
 */
void do_mpvforce(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim, *victim_next;
    char arg[ MAX_INPUT_LENGTH ];
    int vnum;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
	bugf("MpVforce - Bad syntax from vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }

    if (!is_number(arg))
    {
	bugf("MpVforce - Non-number argument vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }

    vnum = atoi(arg);

    LIST_FOREACH_SAFE(victim, &char_list, link, victim_next)
    {
	if (IS_NPC(victim) && victim->pIndexData->vnum == vnum
	    && ch != victim && victim->fighting == NULL)
	    mob_interpret(victim, argument);
    }

    return;
}


/*
 * Lets the mobile cast spells --
 * Beware: this does only crude checking on the target validity
 * and does not account for mana etc., so you should do all the
 * necessary checking in your mob program before issuing this cmd!
 *
 * Syntax: mob cast [spell] {target}
 */

void do_mpcast(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *vch;
    OBJ_DATA *obj;
    void *victim;
    char spell[ MAX_INPUT_LENGTH ];
    int sn, target;

    if (!ch->in_room || IS_SET(ch->in_room->room_flags, ROOM_NOMAGIC))
	return;

    argument = one_argument(argument, spell);

    if (spell[0] == '\0')
    {
	bugf("MpCast - Bad syntax from mob %s [%d].",
	     ch->name, IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }

    if ((sn = skill_lookup(spell)) < 0)
    {
	bugf("MpCast - No such spell (%s) from vnum %d.",
	     spell, IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }
    vch = get_char_room(ch, NULL, argument, FALSE);

    if (vch != NULL && is_affected(vch,gsn_protection_sphere))
	return;

    obj = get_obj_here(ch, NULL, argument);
    switch (skill_table[sn].target)
    {
    default:
	return;
    case TAR_IGNORE:
	victim = NULL;
	target = TAR_IGNORE;
	break;
    case TAR_CHAR_OFFENSIVE:
	if (vch == NULL || vch == ch)
	    return;
	victim = (void *) vch;
	target = TARGET_CHAR;
	break;
    case TAR_CHAR_DEFENSIVE:
	victim = vch == NULL ? (void *) ch : (void *) vch;
	target = TARGET_CHAR;
	break;
    case TAR_CHAR_SELF:
	victim = (void *) ch;
	target = TARGET_CHAR;
	break;
    case TAR_OBJ_CHAR_DEF:
    case TAR_OBJ_CHAR_OFF:
	if (vch)
	{
	    victim = (void *) vch;
	    target = TARGET_CHAR;
	    break;
	}
	if (obj)
	{
	    victim = (void *) obj;
	    target = TARGET_OBJ;
	    break;
	}
	return;
    case TAR_OBJ_INV:
	if (obj == NULL)
	    return;
	victim = (void *) obj;
	target = TARGET_OBJ;
	break;
    }

    (*skill_table[sn].spell_fun)(sn, ch->level, ch, victim, target);

    return;
}

/*
 * Lets mob cause unconditional damage to someone. Nasty, use with caution.
 * Also, this is silent, you must show your own damage message...
 *
 * Syntax: mob damage [victim] [min] [max] {kill}
 */
void do_mpdamage(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim = NULL, *victim_next;
    char target[ MAX_INPUT_LENGTH ],
	 min[ MAX_INPUT_LENGTH ],
	 max[ MAX_INPUT_LENGTH ];
    int low, high;
    bool fAll = FALSE, fKill = FALSE;

    argument = one_argument(argument, target);
    argument = one_argument(argument, min);
    argument = one_argument(argument, max);

    if (target[0] == '\0')
    {
	bugf("MpDamage - Bad syntax from vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }
    if (!str_cmp(target, "all"))
	fAll = TRUE;
    else if((victim = get_char_room(ch, NULL, target, FALSE)) == NULL)
	return;

    if (is_number(min))
	low = atoi(min);
    else
    {
	bugf("MpDamage - Bad damage min vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }
    if (is_number(max))
	high = atoi(max);
    else
    {
	bugf("MpDamage - Bad damage max vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }
    one_argument(argument, target);

    /*
     * If kill parameter is omitted, this command is "safe" and will not
     * kill the victim.
     */

    if (target[0] != '\0')
	fKill = TRUE;
    if (fAll)
    {
	LIST_FOREACH_SAFE(victim, &ch->in_room->people, room_link, victim_next)
	{
	    if (victim != ch)
		damage(victim, victim, fKill ? number_range(low,high) : UMIN(victim->hit,number_range(low,high)),
		       TYPE_UNDEFINED, DAM_NONE, FALSE, NULL);
	}
    }
    else
	damage(victim, victim,
	       fKill ?
	       number_range(low,high) : UMIN(victim->hit,number_range(low,high)),
	       TYPE_UNDEFINED, DAM_NONE, FALSE, NULL);
    return;
}

/*
 * Lets the mobile to remember a target. The target can be referred to
 * with $q and $Q codes in MOBprograms. See also "mob forget".
 *
 * Syntax: mob remember [victim]
 */
void do_mpremember(CHAR_DATA *ch, char *argument)
{
    char arg[ MAX_INPUT_LENGTH ];
    one_argument(argument, arg);

    if (arg[0] != '\0')
    {
	CHAR_DATA *victim;

	if ((victim = get_char_world(ch, arg)) != NULL)
	{
	    mob_remember(victim, ch, MEM_MOBPROG);
	    ch->mprog_target = victim;
	}
    }	
    else
	bugf("MpRemember: missing argument from vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
}

/*
 * Reverse of "mob remember".
 *
 * Syntax: mob forget [<victim>]
 */
void do_mpforget(CHAR_DATA *ch, char *argument)
{
    char arg[ MAX_INPUT_LENGTH ];
    one_argument(argument, arg);

    if (arg[0] != '\0')
    {
	CHAR_DATA *victim;

	if ((victim = get_char_world(ch, arg)) != NULL && is_memory(ch, victim, MEM_MOBPROG))
	    mob_forget(ch, victim, MEM_MOBPROG);
    }	
    else
	mob_forget(ch, NULL, MEM_MOBPROG);   

    ch->mprog_target = NULL;
}

/*
 * Sets a delay for MOBprogram execution. When the delay time expires,
 * the mobile is checked for a MObprogram with DELAY trigger, and if
 * one is found, it is executed. Delay is counted in PULSE_MOBILE
 *
 * Syntax: mob delay [pulses]
 */
void do_mpdelay(CHAR_DATA *ch, char *argument)
{
    char arg[ MAX_INPUT_LENGTH ];

    one_argument(argument, arg);
    if (!is_number(arg))
    {
	bugf("MpDelay: invalid arg from vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }
    ch->mprog_delay = atoi(arg);
}

void do_mppeace(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *rch;

    LIST_FOREACH(rch, &ch->in_room->people, room_link)
    {
		if (rch->fighting != NULL)
		    stop_fighting(rch, TRUE);
/*		if (IS_NPC(rch) && IS_SET(rch->act, ACT_AGGRESSIVE))
	    	REMOVE_BIT(rch->act, ACT_AGGRESSIVE);*/
    }
	stop_fighting(ch, TRUE);
    return;
}

/*
 * Reverse of "mob delay", deactivates the timer.
 *
 * Syntax: mob cancel
 */
void do_mpcancel(CHAR_DATA *ch, char *argument)
{
    ch->mprog_delay = -1;
}
/*
 * Lets the mobile to call another MOBprogram withing a MOBprogram.
 * This is a crude way to implement subroutines/functions. Beware of
 * nested loops and unwanted triggerings... Stack usage might be a problem.
 * Characters and objects referred to must be in the same room with the
 * mobile.
 *
 * Syntax: mob call [vnum] [victim|'null'] [object1|'null'] [object2|'null']
 *
 */
void do_mpcall(CHAR_DATA *ch, char *argument)
{
    char arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *vch;
    OBJ_DATA *obj1, *obj2;
    PROG_CODE *prg;
    extern int program_flow(int, char *, CHAR_DATA *, OBJ_DATA *, ROOM_INDEX_DATA *, CHAR_DATA *, const void *, const void *);

    argument = one_argument(argument, arg);
    if (arg[0] == '\0')
    {
	bugf("MpCall: missing arguments from vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }
    if ((prg = get_prog_index(atoi(arg), PRG_MPROG)) == NULL)
    {
	bugf("MpCall: invalid prog from vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }
    vch = NULL;
    obj1 = obj2 = NULL;
    argument = one_argument(argument, arg);
    if (arg[0] != '\0')
	vch = get_char_room(ch, NULL, arg, FALSE);
    argument = one_argument(argument, arg);
    if (arg[0] != '\0')
	obj1 = get_obj_here(ch, NULL, arg);
    argument = one_argument(argument, arg);
    if (arg[0] != '\0')
	obj2 = get_obj_here(ch, NULL, arg);
    program_flow(prg->vnum, prg->code, ch, NULL, NULL, vch, (void *)obj1, (void *)obj2);
}

/*
 * Forces the mobile to flee.
 *
 * Syntax: mob flee
 *
 */
void do_mpflee(CHAR_DATA *ch, char *argument)
{
    ROOM_INDEX_DATA *was_in;
    EXIT_DATA *pexit;
    int door, attempt;

    if (ch->fighting != NULL)
	return;

    if ((was_in = ch->in_room) == NULL)
	return;

    for (attempt = 0; attempt < 6; attempt++)
    {
	door = number_door();
	if ((pexit = was_in->exit[door]) == 0
	    || pexit->u1.to_room == NULL
	    || IS_SET(pexit->exit_info, EX_CLOSED)
	    || (IS_NPC(ch)
		&& IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB)))
	    continue;

	move_char(ch, door, TRUE, FALSE);
	if (ch->in_room != was_in)
	    return;
    }
}

/*
 * Lets the mobile to transfer an object. The object must be in the same
 * room with the mobile.
 *
 * Syntax: mob otransfer [item name] [location]
 */
void do_mpotransfer(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj;
    ROOM_INDEX_DATA *location;
    char arg[ MAX_INPUT_LENGTH ];
    char buf[ MAX_INPUT_LENGTH ];

    argument = one_argument(argument, arg);
    if (arg[0] == '\0')
    {
	bugf("MpOTransfer - Missing argument from vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }
    one_argument(argument, buf);
    if ((location = find_location(ch, buf)) == NULL)
    {
	bugf("MpOTransfer - No such location from vnum %d.",
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
	return;
    }
    if ((obj = get_obj_here(ch, NULL, arg)) == NULL)
	return;
    if (obj->carried_by == NULL)
	obj_from_room(obj);
    else
    {
	if (obj->wear_loc != WEAR_NONE)
	    unequip_char(ch, obj, TRUE);
	obj_from_char(obj, TRUE);
    }
    obj_to_room(obj, location);
}

/*
 * Lets the mobile to strip an object or all objects from the victim.
 * Useful for removing e.g. quest objects from a character.
 *
 * Syntax: mob remove [victim] [[x.]object vnum|'all']
 */
void do_mpremove(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj, *obj_next;
    int vnum = 0, num = 99999, count = 0;
    bool fAll = FALSE, excempt = FALSE;
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);
    if ((victim = get_char_room(ch, NULL, arg, FALSE)) == NULL)
	return;

    argument = one_argument(argument, arg);
    if (!str_cmp(arg, "all"))
	fAll = TRUE;
    else 
    {
	if (!is_number(arg))
	{
	    num = number_argument(arg, arg);

	    if (!is_number(arg))
	    {
		bugf("MpRemove: Invalid object from vnum %d.",
		     IS_NPC(ch) ? ch->pIndexData->vnum : 0);
		return;
	    }
	}

	vnum = atoi(arg);
    }

    one_argument(argument, arg);
    if (!str_cmp(arg,"изъять") || !str_cmp(arg,"excempt"))
	excempt = TRUE;

    for (obj = victim->carrying; obj; obj = obj_next)
    {
	OBJ_DATA *lobj, *lobj_next;

	obj_next = obj->next_content;

	for (lobj = obj->contains; lobj; lobj = lobj_next)
	{
	    lobj_next = lobj->next_content;

	    if ((fAll || lobj->pIndexData->vnum == vnum) && ++count <= num)
	    {
		obj_from_obj(lobj);
		if (excempt)
		    obj_to_char(lobj, ch);
		else
		    extract_obj(lobj, TRUE, FALSE);
	    }
	}

	if ((fAll || obj->pIndexData->vnum == vnum) && ++count <= num)
	{
	    obj_from_char(obj, TRUE);
	    if (excempt)
		obj_to_char(obj, ch);
	    else
		extract_obj(obj, TRUE, FALSE);
	}
    }
}

/*
 * OBJprog section
 */
void do_obj(OBJ_DATA *obj, char *argument)
{
    /*
     * Security check!
     */
    if (obj->level < MAX_LEVEL)
	return;
    obj_interpret(obj, argument);
}
/*
 * Obj command interpreter. Implemented separately for security and speed
 * reasons. A trivial hack of interpret()
 */
void obj_interpret(OBJ_DATA *obj, char *argument)
{
    char command[MAX_INPUT_LENGTH];
    int cmd;

    argument = one_argument(argument, command);

    /*
     * Look for command in command table.
     */
    for (cmd = 0; obj_cmd_table[cmd].name[0] != '\0'; cmd++)
    {
	if (command[0] == obj_cmd_table[cmd].name[0]
	    && !str_prefix(command, obj_cmd_table[cmd].name))
	{
	    (*obj_cmd_table[cmd].obj_fun) (obj, argument);
	    tail_chain();
	    return;
	}
    }
    bugf("Obj_interpret: invalid cmd from obj %d: '%s'",
	 obj->pIndexData->vnum, command);
}


/*
 * Displays OBJprogram triggers of an object
 *
 * Syntax: opstat [name]
 */
void do_opstat(CHAR_DATA *ch, char *argument)
{
    char        arg[ MAX_STRING_LENGTH  ];
    OBJ_DATA   *obj;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Opstat what?\n\r", ch);
	return;
    }

    if ((obj = get_obj_world(ch, arg)) == NULL)
    {
	send_to_char("No such object.\n\r", ch);
	return;
    }

    sprintf(arg, "Object #%-6d [%s]\n\r",
	    obj->pIndexData->vnum, obj->short_descr);
    send_to_char(arg, ch);

    sprintf(arg, "Delay   %-6d [%s]\n\r",
	    obj->oprog_delay,
	    obj->oprog_target == NULL
	    ? "No target" : obj->oprog_target->name);
    send_to_char(arg, ch);

    if (!obj->pIndexData->progs)
    {
	send_to_char("[No programs set]\n\r", ch);
	return;
    }

    show_triggers(ch, obj->pIndexData->progs);

    return;

}

/*
 * Displays the source code of a given OBJprogram
 *
 * Syntax: opdump [vnum]
 */
void do_opdump(CHAR_DATA *ch, char *argument)
{
    char buf[ MAX_INPUT_LENGTH ];
    PROG_CODE *oprg;

    one_argument(argument, buf);
    if ((oprg = get_prog_index(atoi(buf), PRG_OPROG)) == NULL)
    {
	send_to_char("No such OBJprogram.\n\r", ch);
	return;
    }
    page_to_char(oprg->code, ch);
}

/*
 * Prints the argument to all active players in the game
 *
 * Syntax: obj gecho [string]
 */
void do_opgecho(OBJ_DATA *obj, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
	bugf("OpGEcho: missing argument from vnum %d",
	     obj->pIndexData->vnum);
	return;
    }

    SLIST_FOREACH(d, &descriptor_list, link)
    {
	if (d->connected == CON_PLAYING)
	{
	    if (IS_IMMORTAL(d->character))
		send_to_char("Obj echo> ", d->character);
	    send_to_char(argument, d->character);
	    send_to_char("\n\r", d->character);
	}
    }
}

/*
 * Prints the argument to all players in the same area as the mob
 *
 * Syntax: obj zecho [string]
 */
void do_opzecho(OBJ_DATA *obj, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
	bugf("OpZEcho: missing argument from vnum %d",
	     obj->pIndexData->vnum);
	return;
    }

    if (obj->in_room == NULL && (obj->carried_by == NULL || obj->carried_by->in_room == NULL))
	return;

    SLIST_FOREACH(d, &descriptor_list, link)
    {
	if (d->connected == CON_PLAYING
	    && d->character->in_room != NULL
	    && ((obj->in_room && d->character->in_room->area == obj->in_room->area
		 && d->character->in_room->clan == obj->in_room->clan)
		|| (obj->carried_by && d->character->in_room->area == obj->carried_by->in_room->area
		    && d->character->in_room->clan == obj->carried_by->in_room->clan)))
	{
	    if (IS_IMMORTAL(d->character))
		send_to_char("Obj echo> ", d->character);
	    send_to_char(argument, d->character);
	    send_to_char("\n\r", d->character);
	}
    }
}

/*
 * Prints the message to everyone in the room other than the mob and victim
 *
 * Syntax: obj echoaround [victim] [string]
 */

void do_opechoaround(OBJ_DATA *obj, char *argument)
{
    char       arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
	return;

    if (!obj || (!obj->in_room && !obj->carried_by) || 
	(victim = get_char_room(NULL, obj->in_room?obj->in_room:obj->carried_by->in_room, arg, FALSE)) == NULL)
	return;

    act(argument, victim, NULL, NULL, TO_ROOM);
}

/*
 * Prints the message to only the victim
 *
 * Syntax: obj echoat [victim] [string]
 */
void do_opechoat(OBJ_DATA *obj, char *argument)
{
    char       arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
	return;

    if (!obj || (!obj->in_room && !obj->carried_by) || 
	(victim = get_char_room(NULL, obj->in_room?obj->in_room:obj->carried_by->in_room, arg, FALSE)) == NULL)
	return;

    act(argument, victim, NULL, NULL, TO_CHAR);
}

/*
 * Prints the message to the room at large
 *
 * Syntax: obj echo [string]
 */
void do_opecho(OBJ_DATA *obj, char *argument)
{
    if (argument[0] == '\0')
	return;

    if (!obj->carried_by && (!obj->in_room || LIST_EMPTY(&obj->in_room->people)))
	return;

    act(argument, obj->carried_by ? obj->carried_by : LIST_FIRST(&obj->in_room->people),
	NULL, NULL, TO_ROOM);
    act(argument, obj->carried_by ? obj->carried_by : LIST_FIRST(&obj->in_room->people),
	NULL, NULL, TO_CHAR);
} 

/*void do_opecho(OBJ_DATA *obj, char *argument)
{
    if (argument[0] == '\0')
	return;

    if (!obj->carried_by && (!obj->in_room || !obj->in_room->people))
	return;

    act(argument, obj->carried_by?obj->carried_by:obj->in_room->people, NULL, NULL, TO_ROOM);
    act(argument, obj->carried_by?obj->carried_by:obj->in_room->people, NULL, NULL, TO_CHAR);
} */


/*
 * Lets the object load a mobile.
 *
 * Syntax: obj mload [number].[vnum]
 */
void do_opmload(OBJ_DATA *obj, char *argument)
{
    char            arg[ MAX_INPUT_LENGTH ];
    MOB_INDEX_DATA *pMobIndex;
    CHAR_DATA      *victim;
    int vnum, i, num;

    num = number_argument(argument, arg);

    if ((obj->in_room == NULL
	 && (obj->carried_by == NULL || obj->carried_by->in_room == NULL))
	|| arg[0] == '\0' || !is_number(arg))
	return;

    vnum = atoi(arg);
    if ((pMobIndex = get_mob_index(vnum)) == NULL)
    {
	bugf("Opmload: bad mob index (%d) from obj %d",
	     vnum, obj->pIndexData->vnum);
	return;
    }

    for (i = 0; i < num; i++)
    {
	victim = create_mobile(pMobIndex);
	char_to_room(victim, obj->in_room?obj->in_room:obj->carried_by->in_room, FALSE);
    }
    return;
}

/*
 * Lets the object load another object
 *
 * Syntax: obj oload [vnum] [timer]|in [container vnum]
 */
void do_opoload(OBJ_DATA *obj, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA       *nobj;
    OBJ_DATA       *cont = NULL;
    ROOM_INDEX_DATA *room = NULL;
    int             timer = 0;
    int             cvnum;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || !is_number(arg1))
    {
	bugf("Opoload - Bad syntax from vnum %d.",
	     obj->pIndexData->vnum);
	return;
    }

    if (arg2[0] == '\0')
    {
	timer = 0;
    }
    else if (is_number(arg2))
    {
	/*
	 * New feature from Alander.
	 */
	timer = atoi(arg2);
    }
    else if (!str_cmp(arg2, "in"))
    {
	argument = one_argument(argument, arg3);

	if (IS_NULLSTR(arg3) || !is_number(arg3))
	{
	    if (obj->item_type != ITEM_CONTAINER 
	    && obj->item_type != ITEM_CHEST
	    && obj->item_type != ITEM_MORTAR)
	    {
		bugf("obj load %s in %s: unknown container for vnum %d",
		     arg1, arg3, obj->pIndexData->vnum);
		return;
	    }
	    else
		cont = obj;
	}
	else
	{
	    if (obj->in_room)
		room = obj->in_room;
	    else if (obj->carried_by && obj->carried_by->in_room)
		room = obj->carried_by->in_room;
	    else
	    {
		bugf("Unknown room for vnum %d", obj->pIndexData->vnum);
		return;
	    }

	    cvnum = atoi(arg3);

	    for (cont = room->contents; cont; cont = cont->next_content)
		if (cont->pIndexData->vnum == cvnum)
		    break;

	    if (cont == NULL)
	    {
		bugf("obj oload %s in %d: Can't find container %d in this room for vnum %d",
		     arg1, cvnum, cvnum, obj->pIndexData->vnum);
		return;
	    }
	}
    }
    else
    {
	bugf("Bad syntax for vnum %d", obj->pIndexData->vnum);
	return;
    }

    if ((pObjIndex = get_obj_index(atoi(arg1))) == NULL)
    {
	bugf("Opoload - Bad vnum arg from vnum %d.",
	     obj->pIndexData->vnum);
	return;
    }

    if (check_prog_limit(pObjIndex))
	return;

    nobj = create_object(pObjIndex, 0);
    if (cont == NULL)
	obj_to_room(nobj, obj->in_room?obj->in_room:obj->carried_by->in_room);
    else
	obj_to_obj(nobj, cont);

    nobj->timer = timer;	

    return;
}

/*
 * Lets the object purge all other objects and npcs in the room,
 * or purge a specified object or mob in the room. The object cannot
 * purge itself for safety reasons.
 *
 * syntax obj purge {target}
 */
void do_oppurge(OBJ_DATA *obj, char *argument)
{
    char       arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;
    OBJ_DATA  *vobj;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	/* 'purge' */
	CHAR_DATA *vnext;
	OBJ_DATA  *obj_next;
	ROOM_INDEX_DATA *room;

	if (obj->in_room && !LIST_EMPTY(&obj->in_room->people))
	    room = obj->in_room;
	else if (obj->carried_by && obj->carried_by->in_room)
	    room = obj->carried_by->in_room;
	else
	    room = NULL;

	if (room != NULL)
	{
	    LIST_FOREACH_SAFE(victim, &room->people, room_link, vnext)
	    {
		if (IS_NPC(victim)
		    && !IS_SET(victim->act, ACT_NOPURGE))
		{
		    extract_char(victim, TRUE);
		}
	    }
	}

	if (obj->in_room)
	    vobj = obj->in_room->contents;
	else
	    vobj = obj->carried_by->in_room->contents;

	for (; vobj != NULL; vobj = obj_next)
	{
	    obj_next = vobj->next_content;
	    if (!IS_SET(vobj->extra_flags, ITEM_NOPURGE) && vobj != obj)
		extract_obj(vobj, TRUE, FALSE);
	}

	return;
    }

    if (obj->in_room == NULL && obj->carried_by == NULL)
    {
	bugf("oppurge: in_room == NULL && carried_by == NULL %d)",
	     obj->pIndexData->vnum);
	return;
    }

    if ((victim = get_char_room(NULL, obj->in_room?obj->in_room:obj->carried_by ? obj->carried_by->in_room : NULL, arg, FALSE)) == NULL)
    {
	if ((vobj = get_obj_here(NULL, obj->in_room?obj->in_room:obj->carried_by ? obj->carried_by->in_room : NULL, arg)))
	{
	    extract_obj(vobj, TRUE, FALSE);
	} 
	else if (obj->carried_by && (vobj = get_obj_carry(obj->carried_by, arg, NULL)) != NULL)
	{
	    extract_obj(vobj, TRUE, TRUE);
	}
	else if (obj->carried_by && (vobj = get_obj_wear(obj->carried_by, arg, FALSE)) != NULL)
	{
	    unequip_char(vobj->carried_by, vobj, TRUE);
	    extract_obj(vobj, TRUE, FALSE);
	}
	/*	else if (obj->in_obj && (vobj = get_obj_list(NULL, arg, obj->in_obj->contains)) != NULL)
	 {
	 extract_obj(vobj, TRUE, TRUE);
	 } */
	else
	{
	    bugf("Oppurge - Bad argument from vnum %d.",
		 obj->pIndexData->vnum);
	}
	return;
    }

    if (!IS_NPC(victim))
    {
	bugf("Oppurge - Purging a PC from vnum %d.",
	     obj->pIndexData->vnum);
	return;
    }
    extract_char(victim, TRUE);
    return;
}


/*
 * Lets the object goto any location it wishes that is not private.
 *
 * Syntax: obj goto [location]
 */
void do_opgoto(OBJ_DATA *obj, char *argument)
{
    char             arg[ MAX_INPUT_LENGTH ];
    ROOM_INDEX_DATA *location;
    CHAR_DATA *victim;
    OBJ_DATA *dobj;

    one_argument(argument, arg);
    if (arg[0] == '\0')
    {
	bugf("Opgoto - No argument from vnum %d.",
	     obj->pIndexData->vnum);
	return;
    }

    if (is_number(arg))
	location = get_room_index(atoi(arg));
    else if ((victim = get_char_world(NULL, arg)) != NULL)
	location = victim->in_room;
    else if ((dobj = get_obj_world(NULL, arg)) != NULL)
	location = dobj->in_room;
    else
    {
	bugf("Opgoto - No such location from vnum %d.",
	     obj->pIndexData->vnum);
	return;
    }

    if (obj->in_room != NULL)
	obj_from_room(obj);
    else if (obj->carried_by != NULL)
	obj_from_char(obj, TRUE);
    obj_to_room(obj, location);

    return;
}

/*
 * Lets the object transfer people.  The 'all' argument transfers
 *  everyone in the current room to the specified location
 *
 * Syntax: obj transfer [target|'all'] [location]
 */
void do_optransfer(OBJ_DATA *obj, char *argument)
{
    char             arg1[ MAX_INPUT_LENGTH ];
    char             arg2[ MAX_INPUT_LENGTH ];
    char	     buf[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA *location = NULL;
    CHAR_DATA       *victim;
    OBJ_DATA	    *dobj;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0')
    {
	bugf("Optransfer - Bad syntax from vnum %d.",
	     obj->pIndexData->vnum);
	return;
    }

    if (!str_cmp(arg1, "all"))
    {
	CHAR_DATA *victim_next;
	ROOM_INDEX_DATA *room;

	if (obj->in_room && !LIST_EMPTY(&obj->in_room->people))
	    room = obj->in_room;
	else if (obj->carried_by)
	    room = obj->carried_by->in_room;
	else
	    return;

	LIST_FOREACH_SAFE(victim, &room->people, room_link, victim_next)
	{
	    if (!IS_NPC(victim))
	    {
		sprintf(buf, "'%s' %s", victim->name, arg2);
		do_optransfer(obj, buf);
	    }
	}
	return;
    }

    /*
     * Thanks to Grodyn for the optional location parameter.
     */
    if (arg2[0] == '\0')
    {
	location = obj->in_room?obj->in_room:obj->carried_by->in_room;
    }
    else
    {
	if (is_number(arg2))
	    location = get_room_index(atoi(arg2));
	else if ((victim = get_char_world(NULL, arg2)) != NULL)
	    location = victim->in_room;
	else if ((dobj = get_obj_world(NULL, arg2)) != NULL)
	    location = dobj->in_room;

    }

    if (location == NULL)
    {
	bugf("Optransfer - No such location from vnum %d.",
	     obj->pIndexData->vnum);
	return;
    }

    if (room_is_private(location, NULL))
	return;

    if ((victim = get_char_world(NULL, arg1)) == NULL)
	return;

    if (victim->in_room == NULL)
	return;

    if (victim->fighting != NULL)
	stop_fighting(victim, TRUE);
    char_from_room(victim);
    char_to_room(victim, location, TRUE);
    /*    do_look(victim, "auto");*/

    return;
}

/*
 * Lets the object transfer all chars in same group as the victim.
 *
 * Syntax: obj gtransfer [victim] [location]
 */
void do_opgtransfer(OBJ_DATA *obj, char *argument)
{
    char             arg1[ MAX_INPUT_LENGTH ];
    char             arg2[ MAX_INPUT_LENGTH ];
    char	     buf[MAX_STRING_LENGTH];
    CHAR_DATA        *who, *victim, *victim_next;
    ROOM_INDEX_DATA  *room;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0')
    {
	bugf("Opgtransfer - Bad syntax from vnum %d.",
	     obj->pIndexData->vnum);
	return;
    }

    if ((who = get_char_room(NULL, (obj->in_room)?obj->in_room:(obj->carried_by && obj->carried_by->in_room)?obj->carried_by->in_room : NULL, arg1, FALSE)) == NULL)
	return;

    if (obj->in_room && !LIST_EMPTY(&obj->in_room->people))
	room = obj->in_room;
    else if (obj->carried_by && obj->carried_by->in_room)
	room = obj->carried_by->in_room;
    else
	return;

    LIST_FOREACH_SAFE(victim, &room->people, room_link, victim_next)
    {
	if(is_same_group(who,victim))
	{
	    sprintf(buf, "'%s' %s", victim->name, arg2);
	    do_optransfer(obj, buf);
	}
    }
    return;
}

/*
 * Lets the object force someone to do something. Must be mortal level
 * and the all argument only affects those in the room with the object.
 *
 * Syntax: obj force [victim] [commands]
 */
void do_opforce(OBJ_DATA *obj, char *argument)
{
    char arg[ MAX_INPUT_LENGTH ];
    ROOM_INDEX_DATA *room;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
	bugf("Opforce - Bad syntax from vnum %d.",
	     obj->pIndexData->vnum);
	return;
    }

    if (!obj->in_room && !obj->carried_by)
	return;
    if (obj->in_room && LIST_EMPTY(&obj->in_room->people))
	return;
    if (obj->carried_by && !obj->carried_by->in_room)
	return;

    room = obj->in_room ? obj->in_room : obj->carried_by->in_room;

    if (!str_cmp(arg, "all"))
    {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;

	LIST_FOREACH_SAFE(vch, &room->people, room_link, vch_next)
	{
	    if (get_trust(vch) < obj->level && !check_imm_cmd(vch, argument))
	    {
		mob_interpret(vch, argument);
	    }
	}

    }
    else
    {
	CHAR_DATA *victim;

	if ((victim = get_char_room(NULL, room, arg, FALSE)) == NULL)
	    return;

	if (check_imm_cmd(victim, argument))
	    return;

	mob_interpret(victim, argument);
    }

    return;
}

/*
 * Lets the object force a group something. Must be mortal level.
 *
 * Syntax: obj gforce [victim] [commands]
 */
void do_opgforce(OBJ_DATA *obj, char *argument)
{
    char arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim, *vch, *vch_next;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
	bugf("OpGforce - Bad syntax from vnum %d.",
	     obj->pIndexData->vnum);
	return;
    }

    if ((victim = get_char_room(NULL, obj->in_room?obj->in_room:obj->carried_by->in_room, arg, FALSE)) == NULL)
	return;

    LIST_FOREACH_SAFE(vch, &victim->in_room->people, room_link, vch_next)
    {
	if (is_same_group(victim,vch) && !check_imm_cmd(vch, argument))
	{
	    interpret(vch, argument);
	}
    }
    return;
}

/*
 * Forces all mobiles of certain vnum to do something
 *
 * Syntax: obj vforce [vnum] [commands]
 */
void do_opvforce(OBJ_DATA *obj, char *argument)
{
    CHAR_DATA *victim, *victim_next;
    char arg[ MAX_INPUT_LENGTH ];
    int vnum;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
	bugf("OpVforce - Bad syntax from vnum %d.",
	     obj->pIndexData->vnum);
	return;
    }

    if (!is_number(arg))
    {
	bugf("OpVforce - Non-number argument vnum %d.",
	     obj->pIndexData->vnum);
	return;
    }

    vnum = atoi(arg);

    LIST_FOREACH_SAFE(victim, &char_list, link, victim_next)
    {
	if (IS_NPC(victim) && victim->pIndexData->vnum == vnum
	    && victim->fighting == NULL)
	    mob_interpret(victim, argument);
    }

    return;
}

void do_oppeace(OBJ_DATA *obj, char *argument)
{
    CHAR_DATA *rch;

    LIST_FOREACH(rch, &obj->in_room->people, room_link)
    {
		if (rch->fighting != NULL)
	    	stop_fighting(rch, TRUE);
		if (IS_NPC(rch) && IS_SET(rch->act, ACT_AGGRESSIVE))
	    	REMOVE_BIT(rch->act, ACT_AGGRESSIVE);
    }
    
	return;
}

/*
 * Lets obj cause unconditional damage to someone. Nasty, use with caution.
 * Also, this is silent, you must show your own damage message...
 *
 * Syntax: obj damage [victim] [min] [max] {kill}
 */
void do_opdamage(OBJ_DATA *obj, char *argument)
{
    CHAR_DATA *victim = NULL, *victim_next;
    char target[ MAX_INPUT_LENGTH ],
	 min[ MAX_INPUT_LENGTH ],
	 max[ MAX_INPUT_LENGTH ];
    int low, high;
    bool fAll = FALSE, fKill = FALSE;
    ROOM_INDEX_DATA *room;

    argument = one_argument(argument, target);
    argument = one_argument(argument, min);
    argument = one_argument(argument, max);

    if (target[0] == '\0')
    {
	bugf("OpDamage - Bad syntax from vnum %d.",
	     obj->pIndexData->vnum);
	return;
    }
    if(!str_cmp(target, "all"))
	fAll = TRUE;
    else if((victim = get_char_room(NULL, obj->in_room?obj->in_room: 
				    obj->carried_by ? obj->carried_by->in_room : NULL, target, FALSE)) == NULL)
	return;

    if (is_number(min))
	low = atoi(min);
    else
    {
	bugf("OpDamage - Bad damage min vnum %d.",
	     obj->pIndexData->vnum);
	return;
    }
    if (is_number(max))
	high = atoi(max);
    else
    {
	bugf("OpDamage - Bad damage max vnum %d.",
	     obj->pIndexData->vnum);
	return;
    }
    one_argument(argument, target);

    /*
     * If kill parameter is omitted, this command is "safe" and will not
     * kill the victim.
     */

    if (target[0] != '\0')
	fKill = TRUE;
    if (fAll)
    {
	if (obj->in_room)
	    room = obj->in_room;
	else if (obj->carried_by && obj->carried_by->in_room)
	    room = obj->carried_by->in_room;
	else
	    return;

	LIST_FOREACH_SAFE(victim, &room->people, room_link, victim_next)
	{
	    if (obj->carried_by && victim != obj->carried_by)
		damage(victim, victim,
		       fKill ?
		       number_range(low,high) : UMIN(victim->hit,number_range(low,high)),
		       TYPE_UNDEFINED, DAM_NONE, FALSE, NULL);
	}
    }
    else
	damage(victim, victim,
	       fKill ?
	       number_range(low,high) : UMIN(victim->hit,number_range(low,high)),
	       TYPE_UNDEFINED, DAM_NONE, FALSE, NULL);
    return;
}

/*
 * Lets the object to remember a target. The target can be referred to
 * with $q and $Q codes in OBJprograms. See also "obj forget".
 *
 * Syntax: obj remember [victim]
 */
void do_opremember(OBJ_DATA *obj, char *argument)
{
    char arg[ MAX_INPUT_LENGTH ];
    one_argument(argument, arg);
    if (arg[0] != '\0')
	obj->oprog_target = get_char_world(NULL, arg);
    else
	bugf("OpRemember: missing argument from vnum %d.",
	     obj->pIndexData->vnum);
}

/*
 * Reverse of "obj remember".
 *
 * Syntax: obj forget
 */
void do_opforget(OBJ_DATA *obj, char *argument)
{
    obj->oprog_target = NULL;
}

/*
 * Sets a delay for OBJprogram execution. When the delay time expires,
 * the object is checked for a OBJprogram with DELAY trigger, and if
 * one is found, it is executed. Delay is counted in PULSE_TICK
 *
 * Syntax: obj delay [pulses]
 */
void do_opdelay(OBJ_DATA *obj, char *argument)
{
    char arg[ MAX_INPUT_LENGTH ];

    one_argument(argument, arg);
    if (!is_number(arg))
    {
	bugf("OpDelay: invalid arg from vnum %d.",
	     obj->pIndexData->vnum);
	return;
    }
    obj->oprog_delay = atoi(arg);
}

/*
 * Reverse of "obj delay", deactivates the timer.
 *
 * Syntax: obj cancel
 */
void do_opcancel(OBJ_DATA *obj, char *argument)
{
    obj->oprog_delay = -1;
}
/*
 * Lets the object to call another OBJprogram withing a OBJprogram.
 * This is a crude way to implement subroutines/functions. Beware of
 * nested loops and unwanted triggerings... Stack usage might be a problem.
 * Characters and objects referred to must be in the same room with the
 * mobile.
 *
 * Syntax: obj call [vnum] [victim|'null'] [object1|'null'] [object2|'null']
 *
 */
void do_opcall(OBJ_DATA *obj, char *argument)
{
    char arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *vch;
    OBJ_DATA *obj1, *obj2;
    PROG_CODE *prg;
    extern int program_flow(int, char *, CHAR_DATA *, OBJ_DATA *, ROOM_INDEX_DATA *, CHAR_DATA *, const void *, const void *);

    argument = one_argument(argument, arg);
    if (arg[0] == '\0')
    {
	bugf("OpCall: missing arguments from vnum %d.",
	     obj->pIndexData->vnum);
	return;
    }
    if ((prg = get_prog_index(atoi(arg), PRG_OPROG)) == NULL)
    {
	bugf("OpCall: invalid prog from vnum %d.",
	     obj->pIndexData->vnum);
	return;
    }
    vch = NULL;
    obj1 = obj2 = NULL;
    argument = one_argument(argument, arg);
    if (arg[0] != '\0')
	vch = get_char_room(NULL, obj->in_room?obj->in_room:obj->carried_by->in_room, arg, FALSE);
    argument = one_argument(argument, arg);
    if (arg[0] != '\0')
	obj1 = get_obj_here(NULL, obj->in_room?obj->in_room:obj->carried_by->in_room, arg);
    argument = one_argument(argument, arg);
    if (arg[0] != '\0')
	obj2 = get_obj_here(NULL, obj->in_room?obj->in_room:obj->carried_by->in_room, arg);
    program_flow(prg->vnum, prg->code, NULL, obj, NULL, vch, (void *)obj1, (void *)obj2);
}

/*
 * Lets the object to transfer an object. The object must be in the same
 * room with the object.
 *
 * Syntax: obj otransfer [item name] [location]
 */
void do_opotransfer(OBJ_DATA *obj, char *argument)
{
    OBJ_DATA *obj1, *dobj;
    ROOM_INDEX_DATA *location = NULL;
    char arg[ MAX_INPUT_LENGTH ];
    char buf[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;

    argument = one_argument(argument, arg);
    if (arg[0] == '\0')
    {
	bugf("OpOTransfer - Missing argument from vnum %d.",
	     obj->pIndexData->vnum);
	return;
    }
    one_argument(argument, buf);
    if (is_number(buf))
	location = get_room_index(atoi(buf));
    else if ((victim = get_char_world(NULL, buf)) != NULL)
	location = victim->in_room;
    else if ((dobj = get_obj_world(NULL, arg)) != NULL)
	location = dobj->in_room;

    if (!location)
    {
	bugf("OpOTransfer - No such location from vnum %d.",
	     obj->pIndexData->vnum);
	return;
    }

    if ((obj1 = get_obj_here(NULL, obj->in_room?obj->in_room:obj->carried_by->in_room, arg)) == NULL)
	return;
    if (obj1->carried_by == NULL)
	obj_from_room(obj1);
    else
    {
	if (obj1->wear_loc != WEAR_NONE)
	    unequip_char(obj1->carried_by, obj1, TRUE);
	obj_from_char(obj1, TRUE);
    }
    obj_to_room(obj1, location);
}

/*
 * Lets the object to strip an object or all objects from the victim.
 * Useful for removing e.g. quest objects from a character.
 *
 * Syntax: obj remove [victim] [[x.]object vnum|'all']
 */
void do_opremove(OBJ_DATA *obj, char *argument)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj1, *obj_next;
    int vnum = 0, count = 0, num = 99999;
    bool fAll = FALSE;
    char arg[ MAX_INPUT_LENGTH ];

    argument = one_argument(argument, arg);
    if (!obj || (!obj->in_room && !obj->carried_by) || 
	(victim = get_char_room(NULL, obj->in_room?obj->in_room:obj->carried_by ? obj->carried_by->in_room : NULL, arg, FALSE)) == NULL)
	return;

    one_argument(argument, arg);
    if (!str_cmp(arg, "all"))
	fAll = TRUE;
    else 
    {
	if (!is_number(arg))
	{
	    num = number_argument(arg, arg);

	    if (!is_number(arg))
	    {
		bugf("OpRemove: Invalid object from vnum %d.",
		     obj->pIndexData->vnum);
		return;
	    }	
	}

	vnum = atoi(arg);
    }

    for (obj1 = victim->carrying; obj1; obj1 = obj_next)
    {
	OBJ_DATA *lobj, *lobj_next;

	obj_next = obj1->next_content;

	for (lobj = obj1->contains; lobj; lobj = lobj_next)
	{
	    lobj_next = lobj->next_content;

	    if ((fAll || lobj->pIndexData->vnum == vnum) && ++count <= num)
		extract_obj(lobj, TRUE, FALSE);
	}

	if ((fAll || obj1->pIndexData->vnum == vnum) && ++count <= num)
	    extract_obj(obj1, TRUE, FALSE);
    }
}


bool get_attrib(CHAR_DATA *ch, OBJ_DATA *obj, char *arg, int64_t nonearg, int64_t *setarg)
{
    char *p, mod;
    int i;

    if (!strcmp(arg, "none"))
	*setarg = nonearg;
    else if (!isdigit(arg[0]))
    {
	p = arg;
	mod = arg[0];

	for (i = 1; i; i++)
	{
	    if (arg[i] == '\0')
	    {
		*p = '\0';
		break;
	    }

	    *p = arg[i];
	    p++;
	}

	if (!is_number(arg))
	{
	    bugf("OpAttrib: received non-number argument from vnum %d, argument '%s'",
		 obj->pIndexData->vnum, arg);
	    return FALSE;
	}

	switch (mod)
	{
	case '+':
	    *setarg = ch->level + atoi(arg);
	    break;
	case '-':
	    *setarg = ch->level - atoi(arg);
	    break;
	case '*':
	    *setarg = ch->level * atoi(arg);
	    break;
	case '/':
	    *setarg = ch->level / atoi(arg);
	    break;
	default:
	    bugf("OpAttrib: invalid modifier from vnum %d", obj->pIndexData->vnum);
	    return FALSE;
	}
    }
    else if (is_number(arg))
	*setarg = atoi(arg);
    else
    {
	bugf("OpAttrib: received non-number argument from vnum %d, argument '%s'",
	     obj->pIndexData->vnum, arg);
	return FALSE;
    }

    return TRUE;
}

/*
 * Lets the object change its stats, can be related to the
 * target's level by adding a +,-,*,/ to the end of the value.
 *
 * Syntax: obj attrib [target] [level] [condition] [v0] [v1] [v2] [v3] [v4]
 */
void do_opattrib(OBJ_DATA *obj, char *argument)
{
    CHAR_DATA *ch, *victim;
    ROOM_INDEX_DATA *room = obj->in_room ? obj->in_room : obj->carried_by ? obj->carried_by->in_room : NULL;
    char target[MAX_INPUT_LENGTH], arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH], arg4[MAX_INPUT_LENGTH], arg5[MAX_INPUT_LENGTH];
    char arg6[MAX_INPUT_LENGTH], arg7[MAX_INPUT_LENGTH];
    int64_t level, condition, value0, value1, value2, value3, value4;

    argument = one_argument(argument, target);
    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    argument = one_argument(argument, arg3);
    argument = one_argument(argument, arg4);
    argument = one_argument(argument, arg5);
    argument = one_argument(argument, arg6);
    one_argument(argument, arg7);

    if (!strcmp(target, "worn"))
    {
	if (!obj->carried_by)
	    return;
	else
	    ch = obj->carried_by;
    }
    else if ((ch = get_char_room(NULL, room, target, FALSE)) == NULL)
	return;

    if ((victim = get_char_room(NULL, room, arg1, FALSE)) != NULL)	
	level = victim->level;
    else if (!get_attrib(ch, obj, arg1, obj->level, &level))
	return;

    if (!get_attrib(ch, obj, arg2, obj->condition, &condition))
	return;

    if (!get_attrib(ch, obj, arg3, obj->value[0], &value0))
	return;

    if (!get_attrib(ch, obj, arg4, obj->value[1], &value1))
	return;

    if (!get_attrib(ch, obj, arg5, obj->value[2], &value2))
	return;

    if (!get_attrib(ch, obj, arg6, obj->value[3], &value3))
	return;

    if (!get_attrib(ch, obj, arg7, obj->value[4], &value4))
	return;

    obj->level = level;
    obj->condition = condition;
    obj->value[0] = value0;
    obj->value[1] = value1;
    obj->value[2] = value2;
    obj->value[3] = value3;
    obj->value[4] = value4;
}

/*
 * ROOMprog section
 */

void do_room(ROOM_INDEX_DATA *room, char *argument)
{
    room_interpret(room, argument);
}

/*
 * Room command interpreter. Implemented separately for security and speed
 * reasons. A trivial hack of interpret()
 */
void room_interpret(ROOM_INDEX_DATA *room, char *argument)
{
    char command[MAX_INPUT_LENGTH];
    int cmd;

    argument = one_argument(argument, command);

    /*
     * Look for command in command table.
     */
    for (cmd = 0; room_cmd_table[cmd].name[0] != '\0'; cmd++)
    {
	if (command[0] == room_cmd_table[cmd].name[0]
	    && !str_prefix(command, room_cmd_table[cmd].name))
	{
	    (*room_cmd_table[cmd].room_fun) (room, argument);
	    tail_chain();
	    return;
	}
    }
    bugf("Room_interpret: invalid cmd from room %d: '%s'",
	 room->vnum, command);
}

/*
 * Displays ROOMprogram triggers of a room
 *
 * Syntax: rpstat [name]
 */
void do_rpstat(CHAR_DATA *ch, char *argument)
{
    char        arg[ MAX_STRING_LENGTH  ];
    ROOM_INDEX_DATA   *room;

    one_argument(argument, arg);

    if (arg[0] == '\0')
	room = ch->in_room;
    else if (!is_number(arg))
    {
	send_to_char("You must provide a number.\n\r", ch);
	return;
    }
    else if ((room = get_room_index(atoi(arg))) == NULL)
    {
	send_to_char("No such room.\n\r", ch);
	return;
    }

    sprintf(arg, "Room #%-6d [%s]\n\r",
	    room->vnum, room->name);
    send_to_char(arg, ch);

    sprintf(arg, "Delay   %-6d [%s]\n\r",
	    room->rprog_delay,
	    room->rprog_target == NULL
	    ? "No target" : room->rprog_target->name);
    send_to_char(arg, ch);

    if (!room->progs)
    {
	send_to_char("[No programs set]\n\r", ch);
	return;
    }

    show_triggers(ch, room->progs);

    return;

}

/*
 * Displays the source code of a given ROOMprogram
 *
 * Syntax: rpdump [vnum]
 */
void do_rpdump(CHAR_DATA *ch, char *argument)
{
    char buf[ MAX_INPUT_LENGTH ];
    PROG_CODE *rprg;

    one_argument(argument, buf);
    if ((rprg = get_prog_index(atoi(buf), PRG_RPROG)) == NULL)
    {
	send_to_char("No such ROOMprogram.\n\r", ch);
	return;
    }
    page_to_char(rprg->code, ch);
}

/*
 * Prints the argument to all active players in the game
 *
 * Syntax: room gecho [string]
 */
void do_rpgecho(ROOM_INDEX_DATA *room, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
	bugf("RpGEcho: missing argument from vnum %d",
	     room->vnum);
	return;
    }

    SLIST_FOREACH(d, &descriptor_list, link)
    {
	if (d->connected == CON_PLAYING)
	{
	    if (IS_IMMORTAL(d->character))
		send_to_char("Room echo> ", d->character);
	    send_to_char(argument, d->character);
	    send_to_char("\n\r", d->character);
	}
    }
}

/*
 * Prints the argument to all players in the same area as the room
 *
 * Syntax: room zecho [string]
 */
void do_rpzecho(ROOM_INDEX_DATA *room, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
	bugf("RpZEcho: missing argument from vnum %d",
	     room->vnum);
	return;
    }

    SLIST_FOREACH(d, &descriptor_list, link)
    {
	if (d->connected == CON_PLAYING
	    && d->character->in_room != NULL
	    && d->character->in_room->area == room->area
	    && d->character->in_room->clan == room->clan)
	{
	    if (IS_IMMORTAL(d->character))
		send_to_char("Room echo> ", d->character);
	    send_to_char(argument, d->character);
	    send_to_char("\n\r", d->character);
	}
    }
}

/*
 * Prints the argument to all the rooms aroud the room
 *
 * Syntax: room asound [string]
 */
void do_rpasound(ROOM_INDEX_DATA *room, char *argument)
{
    int              door;

    if (argument[0] == '\0')
	return;

    for (door = 0; door < 6; door++)
    {
	EXIT_DATA       *pexit;

	if ((pexit = room->exit[door]) != NULL
	    && pexit->u1.to_room != NULL
	    && pexit->u1.to_room != room)
	{
	    act(argument, LIST_FIRST(&pexit->u1.to_room->people), NULL, NULL, TO_ALL);
	}
    }
    return;

}

/*
 * Prints the message to everyone in the room other than the victim
 *
 * Syntax: room echoaround [victim] [string]
 */

void do_rpechoaround(ROOM_INDEX_DATA *room, char *argument)
{
    char       arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
	return;

    if ((victim=get_char_room(NULL, room, arg, FALSE)) == NULL)
	return;

    act(argument, victim, NULL, NULL, TO_SOUND);
}

/*
 * Prints the message to only the victim
 *
 * Syntax: room echoat [victim] [string]
 */
void do_rpechoat(ROOM_INDEX_DATA *room, char *argument)
{
    char       arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
	return;

    if ((victim = get_char_room(NULL, room, arg, FALSE)) == NULL)
	return;

    act(argument, victim, NULL, NULL, TO_CHAR);
}

/*
 * Prints the message to the room at large
 *
 * Syntax: rpecho [string]
 */
void do_rpecho(ROOM_INDEX_DATA *room, char *argument)
{
    if (argument[0] == '\0')
	return;

    if (LIST_EMPTY(&room->people))
	return;

    act(argument, LIST_FIRST(&room->people), NULL, NULL, TO_ALL);
}

/*
 * Lets the room load a mobile.
 *
 * Syntax: room mload [number].[vnum]
 */
void do_rpmload(ROOM_INDEX_DATA *room, char *argument)
{
    char            arg[ MAX_INPUT_LENGTH ];
    MOB_INDEX_DATA *pMobIndex;
    CHAR_DATA      *victim;
    int vnum, i, num;

    num = number_argument(argument, arg);

    if (arg[0] == '\0' || !is_number(arg))
	return;

    vnum = atoi(arg);
    if ((pMobIndex = get_mob_index(vnum)) == NULL)
    {
	bugf("Rpmload: bad mob index (%d) from room %d",
	     vnum, room->vnum);
	return;
    }

    for (i = 0; i < num; i++)
    {
	victim = create_mobile(pMobIndex);
	char_to_room(victim, room, FALSE);
    }
    return;
}

/*
 * Lets the room load an object
 *
 * Syntax: room oload [vnum] [timer]
 */
void do_rpoload(ROOM_INDEX_DATA *room, char *argument)
{
    char arg1[ MAX_INPUT_LENGTH ];
    char arg2[ MAX_INPUT_LENGTH ];
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA       *obj;
    int             timer;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || !is_number(arg1))    
    {
	bugf("Rpoload - Bad syntax from vnum %d.",
	     room->vnum);
	return;
    }

    if (arg2[0] == '\0' || !is_number(arg2))
	timer = 0;
    else
	timer = atoi(arg2);

    if ((pObjIndex = get_obj_index(atoi(arg1))) == NULL)
    {
	bugf("Rpoload - Bad vnum arg from vnum %d.", room->vnum);
	return;
    }

    if (check_prog_limit(pObjIndex))
	return;

    obj = create_object(pObjIndex, 0);
    obj_to_room(obj, room);

    obj->timer = timer;

    return;
}

/*
 * Lets the room purge all objects npcs in the room,
 * or purge a specified object or mob in the room.
 *
 * syntax room purge {target}
 */
void do_rppurge(ROOM_INDEX_DATA *room, char *argument)
{
    char       arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;
    OBJ_DATA  *obj;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	/* 'purge' */
	CHAR_DATA *vnext;
	OBJ_DATA  *obj_next;

	LIST_FOREACH_SAFE(victim, &room->people, room_link, vnext)
	{
	    if (IS_NPC(victim)
		&& !IS_SET(victim->act, ACT_NOPURGE))
		extract_char(victim, TRUE);
	}

	for (obj = room->contents; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;
	    if (!IS_SET(obj->extra_flags, ITEM_NOPURGE))
		extract_obj(obj, TRUE, FALSE);
	}

	return;
    }

    if ((victim = get_char_room(NULL, room, arg, FALSE)) == NULL)
    {
	if ((obj = get_obj_here(NULL, room, arg)))
	{
	    extract_obj(obj, TRUE, FALSE);
	}
	else
	{
	    bugf("Rppurge - Bad argument from vnum %d.", room->vnum);
	}
	return;
    }

    if (!IS_NPC(victim))
    {
	bugf("Rppurge - Purging a PC from vnum %d.", room->vnum);
	return;
    }
    extract_char(victim, TRUE);
    return;
}

/*
 * Lets the room transfer people.  The 'all' argument transfers
 *  everyone in the room to the specified location
 *
 * Syntax: room transfer [target|'all'] [location]
 */
void do_rptransfer(ROOM_INDEX_DATA *room, char *argument)
{
    char             arg1[ MAX_INPUT_LENGTH ];
    char             arg2[ MAX_INPUT_LENGTH ];
    char	     buf[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA *location = NULL;
    CHAR_DATA       *victim;
    OBJ_DATA	    *tobj;


    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0')
    {
	bugf("Rptransfer - Bad syntax from vnum %d.", room->vnum);
	return;
    }

    if (!str_cmp(arg1, "all"))
    {
	CHAR_DATA *victim_next;

	LIST_FOREACH_SAFE(victim, &room->people, room_link, victim_next)
	{
	    if (!IS_NPC(victim))
	    {
		sprintf(buf, "'%s' %s", victim->name, arg2);
		do_rptransfer(room, buf);
	    }
	}
	return;
    }

    /*
     * Thanks to Grodyn for the optional location parameter.
     */


    if (arg2[0] == '\0')
    {
	location = room;
    }
    else
    {
	if (is_number(arg2))
	    location = get_room_index(atoi(arg2));
	else if ((victim = get_char_world(NULL, arg2)) != NULL)
	    location = victim->in_room;
	else if ((tobj = get_obj_world(NULL, arg2)) != NULL)
	    location = tobj->in_room;
    }

    if (!location)
    {
	bugf("Rptransfer - No such location from vnum %d.", room->vnum);
	return;
    }

    if (room_is_private(location, NULL))
	return;


    if ((victim = get_char_world(NULL, arg1)) == NULL)
	return;

    if (victim->in_room == NULL)
	return;

    if (victim->fighting != NULL)
	stop_fighting(victim, TRUE);
    char_from_room(victim);
    char_to_room(victim, location, TRUE);
    /*    do_look(victim, "auto");*/

    return;
}

/*
 * Lets the room transfer all chars in same group as the victim.
 *
 * Syntax: room gtransfer [victim] [location]
 */
void do_rpgtransfer(ROOM_INDEX_DATA *room, char *argument)
{
    char             arg1[ MAX_INPUT_LENGTH ];
    char             arg2[ MAX_INPUT_LENGTH ];
    char	     buf[MAX_STRING_LENGTH];
    CHAR_DATA       *who, *victim, *victim_next;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0')
    {
	bugf("Rpgtransfer - Bad syntax from vnum %d.", room->vnum);
	return;
    }

    if ((who = get_char_room(NULL, room, arg1, FALSE)) == NULL)
	return;

    LIST_FOREACH_SAFE(victim, &room->people, room_link, victim_next)
    {
	if(is_same_group(who,victim))
	{
	    sprintf(buf, "'%s' %s", victim->name, arg2);
	    do_rptransfer(room, buf);
	}
    }
    return;
}

/*
 * Lets the room force someone to do something. Must be mortal level
 * and the all argument only affects those in the room.
 *
 * Syntax: room force [victim] [commands]
 */
void do_rpforce(ROOM_INDEX_DATA *room, char *argument)
{
    char arg[ MAX_INPUT_LENGTH ];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
	bugf("Rpforce - Bad syntax from vnum %d.", room->vnum);
	return;
    }

    if (!str_cmp(arg, "all"))
    {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;

	LIST_FOREACH_SAFE(vch, &char_list, link, vch_next)
	{
	    if (vch->in_room == room && !check_imm_cmd(vch, argument))
		mob_interpret(vch, argument);
	}

    }
    else
    {
	CHAR_DATA *victim;

	if ((victim = get_char_room(NULL, room, arg, FALSE)) == NULL)
	    return;

	if (check_imm_cmd(victim, argument))
	    return;

	mob_interpret(victim, argument);
    }

    return;
}

/*
 * Lets the room force a group something. Must be mortal level.
 *
 * Syntax: room gforce [victim] [commands]
 */
void do_rpgforce(ROOM_INDEX_DATA *room, char *argument)
{
    char arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim, *vch, *vch_next;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
	bugf("RpGforce - Bad syntax from vnum %d.", room->vnum);
	return;
    }

    if ((victim = get_char_room(NULL, room, arg, FALSE)) == NULL)
	return;

    LIST_FOREACH_SAFE(vch, &victim->in_room->people, room_link, vch_next)
    {
	if (is_same_group(victim,vch) && !check_imm_cmd(vch, argument))
	    interpret(vch, argument);
    }
    return;
}

/*
 * Forces all mobiles of certain vnum to do something
 *
 * Syntax: room vforce [vnum] [commands]
 */
void do_rpvforce(ROOM_INDEX_DATA *room, char *argument)
{
    CHAR_DATA *victim, *victim_next;
    char arg[ MAX_INPUT_LENGTH ];
    int vnum;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
	bugf("RpVforce - Bad syntax from vnum %d.", room->vnum);
	return;
    }

    if (!is_number(arg))
    {
	bugf("RpVforce - Non-number argument vnum %d.", room->vnum);
	return;
    }

    vnum = atoi(arg);

    LIST_FOREACH_SAFE(victim, &char_list, link, victim_next)
    {
	if (IS_NPC(victim) && victim->pIndexData->vnum == vnum
	    && victim->fighting == NULL)
	    mob_interpret(victim, argument);
    }

    return;
}

void do_rppeace(ROOM_INDEX_DATA *room, char *argument)
{
    CHAR_DATA *rch;

    LIST_FOREACH(rch, &room->people, room_link)
    {
		if (rch->fighting != NULL)
	    	stop_fighting(rch, TRUE);
		if (IS_NPC(rch) && IS_SET(rch->act, ACT_AGGRESSIVE))
	    	REMOVE_BIT(rch->act, ACT_AGGRESSIVE);
    }
    return;
}

/*
 * Lets room cause unconditional damage to someone. Nasty, use with caution.
 * Also, this is silent, you must show your own damage message...
 *
 * Syntax: room damage [victim] [min] [max] {kill}
 */
void do_rpdamage(ROOM_INDEX_DATA *room, char *argument)
{
    CHAR_DATA *victim = NULL, *victim_next;
    char target[ MAX_INPUT_LENGTH ],
	 min[ MAX_INPUT_LENGTH ],
	 max[ MAX_INPUT_LENGTH ];
    int low, high;
    bool fAll = FALSE, fKill = FALSE;

    argument = one_argument(argument, target);
    argument = one_argument(argument, min);
    argument = one_argument(argument, max);

    if (target[0] == '\0')
    {
	bugf("RpDamage - Bad syntax from vnum %d.", room->vnum);
	return;
    }
    if(!str_cmp(target, "all"))
	fAll = TRUE;
    else if((victim = get_char_room(NULL, room, target, FALSE)) == NULL)
	return;

    if (is_number(min))
	low = atoi(min);
    else
    {
	bugf("RpDamage - Bad damage min vnum %d.", room->vnum);
	return;
    }
    if (is_number(max))
	high = atoi(max);
    else
    {
	bugf("RpDamage - Bad damage max vnum %d.", room->vnum);
	return;
    }
    one_argument(argument, target);

    /*
     * If kill parameter is omitted, this command is "safe" and will not
     * kill the victim.
     */

    if (target[0] != '\0')
	fKill = TRUE;
    if (fAll)
    {
	LIST_FOREACH_SAFE(victim, &room->people, room_link, victim_next)
	{
	    damage(victim, victim,
		   fKill ?
		   number_range(low,high) : UMIN(victim->hit,number_range(low,high)),
		   TYPE_UNDEFINED, DAM_NONE, FALSE, NULL);
	}
    }
    else
	damage(victim, victim,
	       fKill ?
	       number_range(low,high) : UMIN(victim->hit,number_range(low,high)),
	       TYPE_UNDEFINED, DAM_NONE, FALSE, NULL);
    return;
}

/*
 * Lets the room remember a target. The target can be referred to
 * with $q and $Q codes in ROOMprograms. See also "room forget".
 *
 * Syntax: room remember [victim]
 */
void do_rpremember(ROOM_INDEX_DATA *room, char *argument)
{
    char arg[ MAX_INPUT_LENGTH ];
    one_argument(argument, arg);
    if (arg[0] != '\0')
	room->rprog_target = get_char_world(NULL, arg);
    else
	bugf("RpRemember: missing argument from vnum %d.", room->vnum);
}

/*
 * Reverse of "room remember".
 *
 * Syntax: room forget
 */
void do_rpforget(ROOM_INDEX_DATA *room, char *argument)
{
    room->rprog_target = NULL;
}

/*
 * Sets a delay for ROOMprogram execution. When the delay time expires,
 * the room is checked for a ROOMprogram with DELAY trigger, and if
 * one is found, it is executed. Delay is counted in PULSE_AREA
 *
 * Syntax: room delay [pulses]
 */
void do_rpdelay(ROOM_INDEX_DATA *room, char *argument)
{
    char arg[ MAX_INPUT_LENGTH ];

    one_argument(argument, arg);
    if (!is_number(arg))
    {
	bugf("RpDelay: invalid arg from vnum %d.", room->vnum);
	return;
    }
    room->rprog_delay = atoi(arg);
}

/*
 * Reverse of "room delay", deactivates the timer.
 *
 * Syntax: room cancel
 */
void do_rpcancel(ROOM_INDEX_DATA *room, char *argument)
{
    room->rprog_delay = -1;
}
/*
 * Lets the room call another ROOMprogram within a ROOMprogram.
 * This is a crude way to implement subroutines/functions. Beware of
 * nested loops and unwanted triggerings... Stack usage might be a problem.
 * Characters and objects referred to must be in the room.
 *
 * Syntax: room call [vnum] [victim|'null'] [object1|'null'] [object2|'null']
 *
 */
void do_rpcall(ROOM_INDEX_DATA *room, char *argument)
{
    char arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *vch;
    OBJ_DATA *obj1, *obj2;
    PROG_CODE *prg;
    extern int program_flow(int, char *, CHAR_DATA *, OBJ_DATA *, ROOM_INDEX_DATA *, CHAR_DATA *, const void *, const void *);

    argument = one_argument(argument, arg);
    if (arg[0] == '\0')
    {
	bugf("RpCall: missing arguments from vnum %d.", room->vnum);
	return;
    }
    if ((prg = get_prog_index(atoi(arg), PRG_RPROG)) == NULL)
    {
	bugf("RpCall: invalid prog from vnum %d.", room->vnum);
	return;
    }
    vch = NULL;
    obj1 = obj2 = NULL;
    argument = one_argument(argument, arg);
    if (arg[0] != '\0')
	vch = get_char_room(NULL, room, arg, FALSE);
    argument = one_argument(argument, arg);
    if (arg[0] != '\0')
	obj1 = get_obj_here(NULL, room, arg);
    argument = one_argument(argument, arg);
    if (arg[0] != '\0')
	obj2 = get_obj_here(NULL, room, arg);
    program_flow(prg->vnum, prg->code, NULL, NULL, room, vch, (void *)obj1, (void *)obj2);
}

/*
 * Lets the room transfer an object. The object must be in the room.
 *
 * Syntax: room otransfer [item name] [location]
 */
void do_rpotransfer(ROOM_INDEX_DATA *room, char *argument)
{
    OBJ_DATA *obj, *tobj;
    ROOM_INDEX_DATA *location = NULL;
    char arg[ MAX_INPUT_LENGTH ];
    char buf[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;

    argument = one_argument(argument, arg);
    if (arg[0] == '\0')
    {
	bugf("RpOTransfer - Missing argument from vnum %d.", room->vnum);
	return;
    }
    one_argument(argument, buf);

    if (is_number(buf))
	location = get_room_index(atoi(buf));
    else if ((victim = get_char_world(NULL, buf)) != NULL)
	location = victim->in_room;
    else if ((tobj = get_obj_world(NULL, arg)) != NULL)
	location = tobj->in_room;

    if (!location)
    {
	bugf("RpOTransfer - No such location from vnum %d.", room->vnum);
	return;
    }

    if ((obj = get_obj_here(NULL, room, arg)) == NULL)
	return;

    if (obj->carried_by == NULL)
	obj_from_room(obj);
    else
    {
	if (obj->wear_loc != WEAR_NONE)
	    unequip_char(obj->carried_by, obj, TRUE);
	obj_from_char(obj, TRUE);
    }
    obj_to_room(obj, location);
}

/*
 * Lets the room strip an object or all objects from the victim.
 * Useful for removing e.g. quest objects from a character.
 *
 * Syntax: room remove [victim] [object vnum|'all']
 */
void do_rpremove(ROOM_INDEX_DATA *room, char *argument)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj, *obj_next;
    int vnum = 0, count = 0, num = 99999;
    bool fAll = FALSE;
    char arg[ MAX_INPUT_LENGTH ];

    argument = one_argument(argument, arg);
    if ((victim = get_char_room(NULL, room, arg, FALSE)) == NULL)
	return;

    one_argument(argument, arg);
    if (!str_cmp(arg, "all"))
	fAll = TRUE;
    else 
    {
	if (!is_number(arg))
	{
	    num = number_argument(arg, arg);
	    if (!is_number(arg))
	    {
		bugf("RpRemove: Invalid object from vnum %d.", room->vnum);
		return;
	    }	
	}
	vnum = atoi(arg);
    }

    for (obj = victim->carrying; obj; obj = obj_next)
    {
	OBJ_DATA *lobj, *lobj_next;

	obj_next = obj->next_content;

	for (lobj = obj->contains; lobj; lobj = lobj_next)
	{
	    lobj_next = lobj->next_content;

	    if ((fAll || lobj->pIndexData->vnum == vnum) && ++count <= num)
		extract_obj(lobj, TRUE, FALSE);
	}

	if ((fAll || obj->pIndexData->vnum == vnum) && ++count <= num)
	    extract_obj(obj, TRUE, FALSE);
    }
}

void do_mpsetskill(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char arg[MAX_STRING_LENGTH];
    int sn;

    argument = one_argument(argument, arg);
    if ((victim = get_char_room(ch, NULL, arg, FALSE)) == NULL || IS_NPC(victim))
	return;

    one_argument(argument, arg);

    if ((sn = skill_lookup(arg)) < 0)
    {
	bugf("MProg: setskill - unexistent skill, mob %d", ch->pIndexData->vnum);
	return;
    }

    if (!skill_table[sn].quest[ch->classid])
    {
	bugf("#*#*#*#*# ALERT!!! #*#*#*#*# Attemt to add non-quest "
	     "skill '%s' for class '%s' (player %s) in 'mob setskill'!!!",
	     skill_table[sn].name,
	     class_table[ch->classid].name,
	     ch->name);
    }

    if (victim->pcdata->learned[sn] <= 0)
    {
	victim->pcdata->learned[sn] = 1;
	sprintf(arg, "Ты теперь знаешь, что такое '%s'!\n\r", skill_table[sn].rname);
	send_to_char(arg, victim);
    }
}

/*
 * Syntax: {mob|room} door <direction> '<status>' [<key>]
 *   <direction> is north, east, south, west, up or down
 *   <status> is any exit flags
 *   <key> is vnum of the key
 *
 * Example: mob door north 'close lock pickproof' 30605
 */
void do_mpdoor(CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    int dir, key = 0;
    int64_t flags;
    OBJ_INDEX_DATA *pObjIndex;
    ROOM_INDEX_DATA *room;
    EXIT_DATA *exit, *exit_rev;

    if ((room = ch->in_room) == NULL)
    {
	bugf("[Mob %d] in_room is NULL in 'mob door'", ch->pIndexData->vnum);
	return;
    }

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if ((dir = find_door(ch, arg1)) < 0)
    {
	bugf("[Mob %d] Invalid direction in 'mob door': %s",
	     ch->pIndexData->vnum, arg1);
	return;
    }

    exit = room->exit[dir];
    exit_rev = exit->u1.to_room->exit[rev_dir[dir]];

    if (exit_rev->u1.to_room != room)
	exit_rev = NULL;

    if (!IS_NULLSTR(argument) && is_number(argument))
	key = atoi(argument);

    if ((pObjIndex = get_obj_index(key)) == NULL)
    {
	bugf("[Mob %d] Couldn't find object with vnum %d in 'mob door'",
	     ch->pIndexData->vnum, key);
	return;
    }

    if (pObjIndex->item_type != ITEM_KEY
	&& pObjIndex->item_type != ITEM_ROOM_KEY)
    {
	bugf("[Mob %d] Item %d isn't a key in 'mob door'",
	     ch->pIndexData->vnum, key);
	return;
    }

    if ((flags = flag_value(exit_flags, arg2)) == NO_FLAG)
    {
	bugf("[Mob %d] Wrong exit flags in 'mob door'", ch->pIndexData->vnum);
	return;
    }

    TOGGLE_BIT(exit->exit_info, flags);
    if (exit_rev)
	TOGGLE_BIT(exit_rev->exit_info, flags);

    if (key != 0)
    {
	exit->key = key;
	if (exit_rev)
	    exit_rev->key = key;
    }
}

void do_rpdoor(ROOM_INDEX_DATA *room, char *argument)
{
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    int dir, key = 0;
    int64_t flags;
    OBJ_INDEX_DATA *pObjIndex;
    EXIT_DATA *exit, *exit_rev;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if ((dir = find_door_room(room, arg1)) < 0)
    {
	bugf("[Room %d] Invalid direction in 'room door': %s",
	     room->vnum, arg1);
	return;
    }

    exit = room->exit[dir];
    exit_rev = exit->u1.to_room->exit[rev_dir[dir]];

    if (exit_rev->u1.to_room != room)
	exit_rev = NULL;

    if (!IS_NULLSTR(argument) && is_number(argument))
	key = atoi(argument);

    if ((pObjIndex = get_obj_index(key)) == NULL)
    {
	bugf("[Room %d] Couldn't find object vnum %d in 'mob door'",
	     room->vnum, key);
	return;
    }

    if (pObjIndex->item_type != ITEM_KEY
	&& pObjIndex->item_type != ITEM_ROOM_KEY)
    {
	bugf("[Room %d] Item %d isn't a key in 'mob door'",
	     room->vnum, key);
	return;
    }

    if ((flags = flag_value(exit_flags, arg2)) == NO_FLAG)
    {
	bugf("[Room %d] Wrong exit flags in 'mob door'", room->vnum);
	return;
    }

    TOGGLE_BIT(exit->exit_info, flags);
    if (exit_rev)
	TOGGLE_BIT(exit_rev->exit_info, flags);

    if (key != 0)
    {
	exit->key = key;
	if (exit_rev)
	    exit_rev->key = key;
    }
}

void do_mplog(CHAR_DATA *ch, char *argument)
{
    char buf[MSL];

    sprintf(buf, "[MPLOG] %s(%d)(Holy:%s): %s", ch->name, ch->pIndexData->vnum, 
	    mob_holylight ? "true" : "false", argument);
    log_string(buf);
}

void do_oplog(OBJ_DATA *obj, char *argument)
{
    char buf[MSL];

    sprintf(buf, "[OPLOG] %s(%d): %s", obj->name, obj->pIndexData->vnum, argument);
    log_string(buf);
}

void do_rplog(ROOM_INDEX_DATA *room, char *argument)
{
    char buf[MSL];

    sprintf(buf, "[RPLOG] %s(%d): %s", room->name, room->vnum, argument);
    log_string(buf);
}

void do_mpholylight(CHAR_DATA *ch, char *argument)
{
    char arg[MIL];

    one_argument(argument, arg);

    if (!str_cmp(arg, "yes"))
	mob_holylight = TRUE;
    else
	mob_holylight = FALSE;
}

void do_mpaddapply(CHAR_DATA *ch, char *argument)
{
    do_addapply(ch, argument);
}

void do_mplag(CHAR_DATA *ch, char *argument)
{
    char      arg1[ MAX_INPUT_LENGTH ];
    char      arg2[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;
	int wait = 0;

    argument=one_argument(argument, arg1);

    if (arg1[0] == '\0')
	return;

    if ((victim = get_char_room(ch, NULL, arg1, FALSE)) == NULL)
	return;

    one_argument(argument, arg2);

    if (!is_number(arg2))
    {
		bugf("MpLag: invalid time from vnum %d.", IS_NPC(ch) ? ch->pIndexData->vnum : 0);
		return;
    }

    if (arg2[0] == '\0')
		wait = 1;
	else
		wait = atoi(arg2);

	WAIT_STATE(victim, wait * PULSE_VIOLENCE);
    return;
}

void do_oplag(OBJ_DATA *obj, char *argument)
{
    char      arg1[ MAX_INPUT_LENGTH ];
    char      arg2[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;
	int wait = 0;

    argument=one_argument(argument, arg1);

    if (arg1[0] == '\0')
	return;

	victim = get_char_world(NULL, arg1);
    
	if (victim == NULL)
	return;

    one_argument(argument, arg2);

    if (!is_number(arg2))
    {
		bugf("OpLag: invalid arg from vnum %d.", obj->pIndexData->vnum);
		return;
    }

    if (arg2[0] == '\0')
		wait = 1;
	else
		wait = atoi(arg2);

	WAIT_STATE(victim, wait * PULSE_VIOLENCE);
    return;
}

void do_rplag(ROOM_INDEX_DATA *room, char *argument)
{
    char      arg1[ MAX_INPUT_LENGTH ];
    char      arg2[ MAX_INPUT_LENGTH ];
	CHAR_DATA *victim;
	int wait = 0;

    argument=one_argument(argument, arg1);

    if (arg1[0] == '\0')
	return;

	victim = get_char_world(NULL, arg1);

    if (victim == NULL)
	return;

    one_argument(argument, arg2);

    if (!is_number(arg2))
    {
		bugf("MpLag: invalid time from vnum %d.", room->vnum);
		return;
    }

    if (arg2[0] == '\0')
		wait = 1;
	else
		wait = atoi(arg2);

	WAIT_STATE(victim, wait * PULSE_VIOLENCE);
    return;
}

void do_opflags(OBJ_DATA *obj, char *argument)
{
    int64_t value;
    char arg[MIL];

    argument = one_argument(argument, arg);

    if (!str_cmp(arg, "unusable"))
    {
	if ((value = flag_value(comf_flags, argument)) != NO_FLAG)
	{
	    obj->unusable = 0;
	    SET_BIT(obj->unusable, value);
	}
    }	
    else if (!str_cmp(arg, "uncomf"))
    {
	if ((value = flag_value(comf_flags, argument)) != NO_FLAG)
	{
	    obj->uncomf = 0;
	    SET_BIT(obj->uncomf, value);
	}
    }	
    else if (!str_cmp(arg, "extra"))
    {
	if ((value = flag_value(extra_flags, argument)) != NO_FLAG)
	{
	    obj->extra_flags = 0;
	    SET_BIT(obj->extra_flags, value);
	}
    }	
    else
	bugf("[Obj %d] Wrong argument on obj flags", obj->pIndexData->vnum);

}

/* charset=cp1251 */

