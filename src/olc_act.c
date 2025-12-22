/***************************************************************************
 *  File: olc_act.c                                                        *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include "merc.h"
#include "olc.h"
#include "recycle.h"
#include "interp.h"



char * prog_type_to_name (int type);

/* Return TRUE if area changed, FALSE if not. */
#define REDIT(fun)		bool fun(CHAR_DATA *ch, char *argument)
#define OEDIT(fun)		bool fun(CHAR_DATA *ch, char *argument)
#define MEDIT(fun)		bool fun(CHAR_DATA *ch, char *argument)
#define AEDIT(fun)		bool fun(CHAR_DATA *ch, char *argument)
#define HEDIT(fun)		bool fun(CHAR_DATA *ch, char *argument)
#define CEDIT(fun)		bool fun(CHAR_DATA *ch, char *argument)



struct olc_help_type
{
    char *command;
    const void *structure;
    char *desc;
};



bool show_version(CHAR_DATA *ch, char *argument)
{
    send_to_char(VERSION, ch);
    send_to_char("\n\r", ch);
    send_to_char(AUTHOR, ch);
    send_to_char("\n\r", ch);
    send_to_char(DATE, ch);
    send_to_char("\n\r", ch);
    send_to_char(CREDITS, ch);
    send_to_char("\n\r", ch);

    return FALSE;
}

/*
 * This table contains help commands and a brief description of each.
 * ------------------------------------------------------------------
 */
struct olc_help_type help_table[] =
{
    {	"area",		area_flags,	 "Area attributes."		 },
    {	"room",		room_flags,	 "Room attributes."		 },
    {	"sector",	sector_flags,	 "Sector types, terrain."	 },
    {	"exit",		exit_flags,	 "Exit types."			 },
    {	"type",		type_flags,	 "Types of objects."		 },
    {	"extra",	extra_flags,	 "Object attributes."		 },
    {	"wear",		wear_flags,	 "Where to wear object."	 },
    {	"spec",		spec_table,	 "Available special programs." 	 },
    {	"sex",		sex_flags,	 "Sexes."			 },
    {	"act",		act_flags,	 "Mobile attributes."		 },
    {	"affect",	affect_flags,	 "Mobile affects."		 },
    {	"wear-loc",	wear_loc_flags,	 "Where mobile wears object."	 },
    {	"spells",	NULL,	 	 "Names of current spells."	 },
    /*    {	"spells",	skill_table,	 "Names of current spells." 	 },*/
    {	"container",	container_flags, "Container status."		 },

    /* ROM specific bits: */

    {	"armor",	ac_type,	 "Ac for different attacks."	 },
    {   "apply",	apply_flags,	 "Apply flags"			 },
    {	"form",		form_flags,	 "Mobile body form."	         },
    {	"part",		part_flags,	 "Mobile body parts."		 },
    {	"imm",		imm_flags,	 "Mobile immunity."		 },
    {	"res",		res_flags,	 "Mobile resistance."	         },
    {	"vuln",		vuln_flags,	 "Mobile vulnerability."	 },
    {	"off",		off_flags,	 "Mobile offensive behaviour."	 },
    {	"size",		size_flags,	 "Mobile size."			 },
    {   "position",     position_flags,  "Mobile positions."             },
    {   "wclass",       weapon_class,    "Weapon class."                 },
    {   "wtype",        weapon_type2,    "Special weapon type."          },
    {	"portal",	portal_flags,	 "Portal types."		 },
    {	"furniture",	furniture_flags, "Furniture types."		 },
    {   "liquid",	liq_table,	 "Liquid types."		 },
    {	"apptype",	apply_types,	 "Apply types."			 },
    {	"weapon",	attack_table,	 "Weapon types."		 },
    {	"mprog",	mprog_flags,	 "MobProgram flags."		 },
    {	"oprog",	oprog_flags,	 "ObjProgram flags."		 },
    {	"rprog",	rprog_flags,	 "RoomProgram flags."		 },
    {	"material",	material_table,	 "Materials."			 },
    {	"stat",		attr_flags,	 "Char stats."			 },

    /* Сундук  */

    {   "chest",	chest_flags,	 "Chest status."		 },
    {	NULL,		NULL,		 NULL				 }
};

void show_triggers(CHAR_DATA *ch, PROG_LIST *progs)
{
	int cnt;
	char buf[MSL];
	PROG_LIST *list;

	for (cnt=0, list=progs; list; list=list->next)
	{
	    if (cnt == 0)
	    {
		send_to_char (" Number Vnum Trigger  When   Phrase\n\r", ch);
		send_to_char (" ------ ---- ------- ------- ------\n\r", ch);
	    }

	    sprintf(buf, "[%5d] %4d %7s %7s %s\n\r", cnt,
		    list->vnum,prog_type_to_name(list->trig_type),
		    flag_string(prog_when_flags, list->trig_flags, FALSE),
		    list->trig_phrase);
	    send_to_char(buf, ch);
	    cnt++;
	}
}

/* Small hack 8)) */
void update_spells_help()
{
    int i;

    for (i = 0; help_table[i].command != NULL; i++)
	if (!str_cmp("spells", help_table[i].command))
	    help_table[i].structure = skill_table;
}

/*****************************************************************************
Name:		show_flag_cmds
Purpose:	Displays settable flags and stats.
Called by:	show_help(olc_act.c).
 ****************************************************************************/
void show_flag_cmds(CHAR_DATA *ch, const struct flag_type *flag_table)
{
    char buf  [ MAX_STRING_LENGTH ];
    char buf1 [ MAX_STRING_LENGTH ];
    int  flag;
    int  col;

    buf1[0] = '\0';
    col = 0;
    for (flag = 0; flag_table[flag].name != NULL; flag++)
    {
	if (flag_table[flag].settable)
	{
	    sprintf(buf, "%-19.18s", flag_table[flag].name);
	    strcat(buf1, buf);
	    if (++col % 4 == 0)
		strcat(buf1, "\n\r");
	}
    }

    if (col % 4 != 0)
	strcat(buf1, "\n\r");

    send_to_char(buf1, ch);
    return;
}


/*****************************************************************************
Name:		show_skill_cmds
Purpose:	Displays all skill functions.
Does remove those damn immortal commands from the list.
Could be improved by:
(1) Adding a check for a particular class.
(2) Adding a check for a level range.
Called by:	show_help(olc_act.c).
 ****************************************************************************/
void show_skill_cmds(CHAR_DATA *ch, int tar)
{
    char buf  [ MAX_STRING_LENGTH ];
    char buf1 [ MAX_STRING_LENGTH*2 ];
    int  sn;
    int  col;

    buf1[0] = '\0';
    col = 0;
    for (sn = 0; sn < max_skills; sn++)
    {
	if (IS_NULLSTR(skill_table[sn].name))
	    break;

	if (!str_cmp(skill_table[sn].name, "reserved")
	    || skill_table[sn].spell_fun == spell_null)
	    continue;

	if (tar == -1 || skill_table[sn].target == tar)
	{
	    sprintf(buf, "%-19.18s", skill_table[sn].name);
	    strcat(buf1, buf);
	    if (++col % 4 == 0)
		strcat(buf1, "\n\r");
	}
    }

    if (col % 4 != 0)
	strcat(buf1, "\n\r");

    send_to_char(buf1, ch);
    return;
}



/*****************************************************************************
Name:		show_spec_cmds
Purpose:	Displays settable special functions.
Called by:	show_help(olc_act.c).
 ****************************************************************************/
void show_spec_cmds(CHAR_DATA *ch)
{
    char buf  [ MAX_STRING_LENGTH ];
    char buf1 [ MAX_STRING_LENGTH ];
    int  spec;
    int  col;

    buf1[0] = '\0';
    col = 0;
    send_to_char("Preceed special functions with 'spec_'\n\r\n\r", ch);
    for (spec = 0; spec_table[spec].function != NULL; spec++)
    {
	sprintf(buf, "%-19.18s", &spec_table[spec].name[5]);
	strcat(buf1, buf);
	if (++col % 4 == 0)
	    strcat(buf1, "\n\r");
    }

    if (col % 4 != 0)
	strcat(buf1, "\n\r");

    send_to_char(buf1, ch);
    return;
}



/*****************************************************************************
Name:		show_help
Purpose:	Displays help for many tables used in OLC.
Called by:	olc interpreters.
 ****************************************************************************/
bool show_help(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char spell[MAX_INPUT_LENGTH];
    int cnt;

    argument = one_argument(argument, arg);
    one_argument(argument, spell);

    /*
     * Display syntax.
     */
    if (arg[0] == '\0')
    {
	send_to_char("Syntax:  ? [command]\n\r\n\r", ch);
	send_to_char("[command]  [description]\n\r", ch);
	for (cnt = 0; help_table[cnt].command != NULL; cnt++)
	{
	    sprintf(buf, "%-10.10s -%s\n\r",
		    capitalize(help_table[cnt].command),
		    help_table[cnt].desc);
	    send_to_char(buf, ch);
	}
	return FALSE;
    }

    /*
     * Find the command, show changeable data.
     * ---------------------------------------
     */
    for (cnt = 0; help_table[cnt].command != NULL; cnt++)
    {
	if (arg[0] == help_table[cnt].command[0]
	    && !str_prefix(arg, help_table[cnt].command))
	{
	    if (help_table[cnt].structure == spec_table)
	    {
		show_spec_cmds(ch);
		return FALSE;
	    }
	    else
		if (help_table[cnt].structure == liq_table)
		{
		    show_liqlist(ch);
		    return FALSE;
		}
		else
		    if (help_table[cnt].structure == attack_table)
		    {
			show_damlist(ch);
			return FALSE;
		    }
		    else
			if (help_table[cnt].structure == NULL && !str_cmp(help_table[cnt].command, "spells"))
			{

			    if (spell[0] == '\0')
			    {
				send_to_char("Syntax:  ? spells "
					     "[ignore/attack/defend/self/object/all]\n\r", ch);
				return FALSE;
			    }

			    if (!str_prefix(spell, "all"))
				show_skill_cmds(ch, -1);
			    else if (!str_prefix(spell, "ignore"))
				show_skill_cmds(ch, TAR_IGNORE);
			    else if (!str_prefix(spell, "attack"))
				show_skill_cmds(ch, TAR_CHAR_OFFENSIVE);
			    else if (!str_prefix(spell, "defend"))
				show_skill_cmds(ch, TAR_CHAR_DEFENSIVE);
			    else if (!str_prefix(spell, "self"))
				show_skill_cmds(ch, TAR_CHAR_SELF);
			    else if (!str_prefix(spell, "object"))
				show_skill_cmds(ch, TAR_OBJ_INV);
			    else
				send_to_char("Syntax:  ? spell "
					     "[ignore/attack/defend/self/object/all]\n\r", ch);

			    return FALSE;
			}
			else
			{
			    show_flag_cmds(ch, help_table[cnt].structure);
			    return FALSE;
			}
	}
    }

    show_help(ch, "");
    return FALSE;
}

REDIT(redit_rlist)
{
    ROOM_INDEX_DATA	*pRoomIndex;
    AREA_DATA		*pArea;
    char		buf  [ MAX_STRING_LENGTH   ];
    BUFFER		*buf1;
    char		arg  [ MAX_INPUT_LENGTH    ];
    bool found, fProgs;
    int vnum;

    pArea = ch->in_room->area;
    buf1 = new_buf();
    found   = FALSE;

    one_argument(argument, arg);
    fProgs  = !str_cmp(arg, "progs");

    for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++)
    {
	if ((pRoomIndex = get_room_index(vnum)))
	{
	    found = TRUE;
	    sprintf(buf, "[%5d][H:%5d%% M:%5d%%] %s\n\r",
		    vnum, pRoomIndex->heal_rate, pRoomIndex->mana_rate, capitalize(pRoomIndex->name));
	    add_buf(buf1, buf);

	    if (fProgs)
	    {
		PROG_LIST *list;
		int cnt;

		for (cnt = 0, list = pRoomIndex->progs; list; list = list->next)
		{
		    if (cnt == 0)
		    {
			add_buf(buf1, "            Number Vnum Trigger Phrase\n\r");
			add_buf(buf1, "            ------ ---- ------- ------\n\r");
		    }
		    sprintf(buf, "           [%5d] %4d %7s %s\n\r", cnt,
			    list->vnum,prog_type_to_name(list->trig_type),
			    list->trig_phrase);
		    add_buf(buf1, buf);
		    cnt++;
		}
	    }

	}
    }

    if (!found)
    {
	send_to_char("Room(s) not found in this area.\n\r", ch);
	free_buf(buf1);
	return FALSE;
    }

    page_to_char(buf_string(buf1), ch);
    free_buf(buf1);
    return FALSE;
}

REDIT(redit_mlist)
{
    MOB_INDEX_DATA	*pMobIndex;
    AREA_DATA		*pArea;
    char		buf  [ MAX_STRING_LENGTH   ];
    BUFFER		*buf1;
    char		arg  [ MAX_INPUT_LENGTH    ];
    bool fAll, found, fProgs;
    int vnum;

    argument = one_argument(argument, arg);
    if (arg[0] == '\0')
    {
	send_to_char("Syntax:  mlist <all/name> [progs]\n\r", ch);
	return FALSE;
    }

    buf1=new_buf();
    pArea = ch->in_room->area;
    /*    buf1[0] = '\0'; */
    fAll    = !str_cmp(arg, "all");
    found   = FALSE;

    one_argument(argument, arg);
    fProgs  = !str_cmp(arg, "progs");

    for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++)
    {
	if ((pMobIndex = get_mob_index(vnum)) != NULL)
	{
	    if (fAll || is_name(arg, pMobIndex->player_name))
	    {
		found = TRUE;
 		sprintf(buf, "[%5d] %-31.30s {D[Level: %d] {R[wealth: %d]{x\n\r",
			pMobIndex->vnum, capitalize(pMobIndex->short_descr),
			pMobIndex->level, (int) (pMobIndex->wealth));
		add_buf(buf1, buf);

		if (fProgs)
		{
		    PROG_LIST *list;
		    int cnt;

		    for (cnt = 0, list = pMobIndex->progs; list; list = list->next)
		    {
			if (cnt == 0)
			{
			    add_buf(buf1, "            Number Vnum Trigger Phrase\n\r");
			    add_buf(buf1, "            ------ ---- ------- ------\n\r");
			}
			sprintf(buf, "           [%5d] %4d %7s %s\n\r", cnt,
				list->vnum,prog_type_to_name(list->trig_type),
				list->trig_phrase);
			add_buf(buf1, buf);
			cnt++;
		    }
		}
	    }
	}
    }

    if (!found)
    {
	send_to_char("Mobile(s) not found in this area.\n\r", ch);
	free_buf(buf1);
	return FALSE;
    }

    page_to_char(buf_string(buf1), ch);
    free_buf(buf1);
    return FALSE;
}

REDIT(redit_olist)
{
    OBJ_INDEX_DATA	*pObjIndex;
    AREA_DATA		*pArea;
    char		buf  [ MAX_STRING_LENGTH   ];
    BUFFER		*buf1;
    char		arg  [ MAX_INPUT_LENGTH    ];
    bool fAll, found, fProgs;
    int vnum;

    argument = one_argument(argument, arg);
    if (arg[0] == '\0')
    {
	send_to_char("Syntax:  olist <all/name/item_type> [progs]\n\r", ch);
	return FALSE;
    }

    pArea = ch->in_room->area;
    buf1 = new_buf();
    fAll    = !str_cmp(arg, "all");
    found   = FALSE;

    one_argument(argument, arg);
    fProgs    = !str_cmp(arg, "progs");

    for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++)
    {
	if ((pObjIndex = get_obj_index(vnum)))
	{
	    if (fAll
		|| is_name(arg, pObjIndex->name)
		|| flag_value(type_flags, arg) == pObjIndex->item_type)
	    {
		found = TRUE;
		sprintf(buf, "[%5d][Lev: %3d] [cost: %5d] %s\n\r",
			pObjIndex->vnum, pObjIndex->level, pObjIndex->cost,
			capitalize(pObjIndex->short_descr));
		add_buf(buf1, buf);

		if (fProgs)
		{
		    PROG_LIST *list;
		    int cnt;

		    for (cnt = 0, list = pObjIndex->progs; list; list = list->next)
		    {
			if (cnt == 0)
			{
			    add_buf(buf1, "            Number Vnum Trigger Phrase\n\r");
			    add_buf(buf1, "            ------ ---- ------- ------\n\r");
			}
			sprintf(buf, "           [%5d] %4d %7s %s\n\r", cnt,
				list->vnum,prog_type_to_name(list->trig_type),
				list->trig_phrase);
			add_buf(buf1, buf);
			cnt++;
		    }
		}

	    }
	}
    }

    if (!found)
    {
	send_to_char("Object(s) not found in this area.\n\r", ch);
	free_buf(buf1);
	return FALSE;
    }

    page_to_char(buf_string(buf1), ch);
    free_buf(buf1);
    return FALSE;
}



REDIT(redit_mshow)
{
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] == '\0')
    {
	send_to_char("Syntax:  mshow <vnum>\n\r", ch);
	return FALSE;
    }

    if (!is_number(argument))
    {
	send_to_char("REdit: Ingresa un numero.\n\r", ch);
	return FALSE;
    }

    if (is_number(argument))
    {
	value = atoi(argument);
	if (!(pMob = get_mob_index(value)))
	{
	    send_to_char("REdit:  That mobile does not exist.\n\r", ch);
	    return FALSE;
	}

	ch->desc->pEdit = (void *)pMob;
    }

    medit_show(ch, argument);
    ch->desc->pEdit = (void *)ch->in_room;
    return FALSE;
}



REDIT(redit_oshow)
{
    OBJ_INDEX_DATA *pObj;
    int value;

    if (argument[0] == '\0')
    {
	send_to_char("Syntax:  oshow <vnum>\n\r", ch);
	return FALSE;
    }

    if (!is_number(argument))
    {
	send_to_char("REdit: Ingresa un numero.\n\r", ch);
	return FALSE;
    }

    if (is_number(argument))
    {
	value = atoi(argument);
	if (!(pObj = get_obj_index(value)))
	{
	    send_to_char("REdit:  That object does not exist.\n\r", ch);
	    return FALSE;
	}

	ch->desc->pEdit = (void *)pObj;
    }

    redit_show(ch, argument);
    ch->desc->pEdit = (void *)ch->in_room;
    return FALSE;
}



/*****************************************************************************
Name:		check_range(lower vnum, upper vnum)
Purpose:	Ensures the range spans only one area.
Called by:	aedit_vnum(olc_act.c).
 ****************************************************************************/
bool check_range(int lower, int upper)
{
    AREA_DATA *pArea;
    int cnt = 0;

    for (pArea = area_first; pArea; pArea = pArea->next)
    {
	/*
	 * lower < area < upper
	 */
	if ((lower <= pArea->min_vnum && pArea->min_vnum <= upper)
	    || (lower <= pArea->max_vnum && pArea->max_vnum <= upper))
	{
	    ++cnt;
	}

	if (cnt > 1)
	    return FALSE;
    }
    return TRUE;
}



AREA_DATA *get_vnum_area(int vnum)
{
    AREA_DATA *pArea;

    for (pArea = area_first; pArea; pArea = pArea->next)
    {
	if (vnum >= pArea->min_vnum
	    && vnum <= pArea->max_vnum)
	{
	    return pArea;
	}
    }

    return 0;
}



/*
 * Area Editor Functions.
 */
AEDIT(aedit_show)
{
    AREA_DATA *pArea;
    char buf[MAX_STRING_LENGTH];

    EDIT_AREA(ch, pArea);

    sprintf(buf, "Name:     [%5d] %s\n\r", pArea->vnum, pArea->name);
    send_to_char(buf, ch);

#if 0  /* ROM OLC */
    sprintf(buf, "Recall:   [%5d] %s\n\r", pArea->recall,
	    get_room_index(pArea->recall)
	    ? get_room_index(pArea->recall)->name : "none");
    send_to_char(buf, ch);
#endif /* ROM */

    sprintf(buf, "File:     %s\n\r", pArea->file_name);
    send_to_char(buf, ch);

    sprintf(buf, "Vnums:    [%d-%d]\n\r", pArea->min_vnum, pArea->max_vnum);
    send_to_char(buf, ch);

    sprintf(buf, "Age:      [%d]\n\r",	pArea->age);
    send_to_char(buf, ch);

    sprintf(buf, "Players:  [%d]\n\r", pArea->nplayer);
    send_to_char(buf, ch);

    sprintf(buf, "Security: [%d]\n\r", pArea->security);
    send_to_char(buf, ch);

    sprintf(buf, "Builders: [%s]\n\r", pArea->builders);
    send_to_char(buf, ch);

    sprintf(buf, "Credits : [%s]\n\r", pArea->credits);
    send_to_char(buf, ch);

    sprintf(buf, "Flags:    [%s]\n\r", flag_string(area_flags, pArea->area_flags, FALSE));
    send_to_char(buf, ch);

    return FALSE;
}



AEDIT(aedit_reset)
{
    AREA_DATA *pArea;

    EDIT_AREA(ch, pArea);

    reset_area(pArea);
    send_to_char("Area reset.\n\r", ch);

    return FALSE;
}



/* AEDIT(aedit_create)
 {
 AREA_DATA *pArea;

 pArea               =   new_area();
 area_last->next     =   pArea;
 area_last		=   pArea;
 ch->desc->pEdit     =   (void *)pArea;

 SET_BIT(pArea->area_flags, AREA_ADDED);
 send_to_char("Area Created.\n\r", ch);
 return FALSE;
 }  */



AEDIT(aedit_name)
{
    AREA_DATA *pArea;

    EDIT_AREA(ch, pArea);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax:   name [$name]\n\r", ch);
	return FALSE;
    }

    free_string(pArea->name);
    pArea->name = str_dup(argument);

    send_to_char("Name set.\n\r", ch);
    return TRUE;
}

AEDIT(aedit_credits)
{
    AREA_DATA *pArea;

    EDIT_AREA(ch, pArea);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax:   credits [$credits]\n\r", ch);
	return FALSE;
    }

    free_string(pArea->credits);
    pArea->credits = str_dup(argument);

    send_to_char("Credits set.\n\r", ch);
    return TRUE;
}

/*
 AEDIT(aedit_file)
 {
 AREA_DATA *pArea;
 char file[MAX_STRING_LENGTH];
 int i, length;

 EDIT_AREA(ch, pArea);

 one_argument(argument, file);

 if (argument[0] == '\0')
 {
 send_to_char("Syntax:  filename [$file]\n\r", ch);
 return FALSE;
 }

 length = strlen(argument);
 if (length > 8)
 {
 send_to_char("No more than eight characters allowed.\n\r", ch);
 return FALSE;
 }

 for (i = 0; i < length; i++)
 {
 if (!isalnum(file[i]))
 {
 send_to_char("Only letters and numbers are valid.\n\r", ch);
 return FALSE;
 }
 }

 free_string(pArea->file_name);
 strcat(file, ".are");
 pArea->file_name = str_dup(file);

 send_to_char("Filename set.\n\r", ch);
 return TRUE;
 }
 */


AEDIT(aedit_age)
{
    AREA_DATA *pArea;
    char age[MAX_STRING_LENGTH];

    EDIT_AREA(ch, pArea);

    one_argument(argument, age);

    if (!is_number(age) || age[0] == '\0')
    {
	send_to_char("Syntax:  age [#xage]\n\r", ch);
	return FALSE;
    }

    pArea->age = atoi(age);

    send_to_char("Age set.\n\r", ch);
    return TRUE;
}


#if 0 /* ROM OLC */
AEDIT(aedit_recall)
{
    AREA_DATA *pArea;
    char room[MAX_STRING_LENGTH];
    int  value;

    EDIT_AREA(ch, pArea);

    one_argument(argument, room);

    if (!is_number(argument) || argument[0] == '\0')
    {
	send_to_char("Syntax:  recall [#xrvnum]\n\r", ch);
	return FALSE;
    }

    value = atoi(room);

    if (!get_room_index(value))
    {
	send_to_char("AEdit:  Room vnum does not exist.\n\r", ch);
	return FALSE;
    }

    pArea->recall = value;

    send_to_char("Recall set.\n\r", ch);
    return TRUE;
}
#endif /* ROM OLC */


AEDIT(aedit_security)
{
    AREA_DATA *pArea;
    char sec[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int  value;

    EDIT_AREA(ch, pArea);

    one_argument(argument, sec);

    if (!is_number(sec) || sec[0] == '\0')
    {
	send_to_char("Syntax:  security [#xlevel]\n\r", ch);
	return FALSE;
    }

    value = atoi(sec);

    if (value > ch->pcdata->security || value < 0)
    {
	if (ch->pcdata->security != 0)
	{
	    sprintf(buf, "Security is 0-%d.\n\r", ch->pcdata->security);
	    send_to_char(buf, ch);
	}
	else
	    send_to_char("Security is 0 only.\n\r", ch);
	return FALSE;
    }

    pArea->security = value;

    send_to_char("Security set.\n\r", ch);
    return TRUE;
}



AEDIT(aedit_builder)
{
    AREA_DATA *pArea;
    char name[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];

    EDIT_AREA(ch, pArea);

    one_argument(argument, name);

    if (name[0] == '\0')
    {
	send_to_char("Syntax:  builder [$name]  -toggles builder\n\r", ch);
	send_to_char("Syntax:  builder All      -allows everyone\n\r", ch);
	return FALSE;
    }

    name[0] = UPPER(name[0]);

    if (strstr(pArea->builders, name) != '\0')
    {
	pArea->builders = string_replace(pArea->builders, name, "\0");
	pArea->builders = string_unpad(pArea->builders);

	if (pArea->builders[0] == '\0')
	{
	    free_string(pArea->builders);
	    pArea->builders = str_dup("None");
	}
	send_to_char("Builder removed.\n\r", ch);
	return TRUE;
    }
    else
    {
	buf[0] = '\0';
	if (strstr(pArea->builders, "None") != '\0')
	{
	    pArea->builders = string_replace(pArea->builders, "None", "\0");
	    pArea->builders = string_unpad(pArea->builders);
	}

	if (pArea->builders[0] != '\0')
	{
	    strcat(buf, pArea->builders);
	    strcat(buf, " ");
	}
	strcat(buf, name);
	free_string(pArea->builders);
	pArea->builders = string_proper(str_dup(buf));

	send_to_char("Builder added.\n\r", ch);
	send_to_char(pArea->builders,ch);
	return TRUE;
    }

    return FALSE;
}



AEDIT(aedit_vnum)
{
    AREA_DATA *pArea;
    char lower[MAX_STRING_LENGTH];
    char upper[MAX_STRING_LENGTH];
    int  ilower;
    int  iupper;

    EDIT_AREA(ch, pArea);

    argument = one_argument(argument, lower);
    one_argument(argument, upper);

    if (!is_number(lower) || lower[0] == '\0'
	|| !is_number(upper) || upper[0] == '\0')
    {
	send_to_char("Syntax:  vnum [#xlower] [#xupper]\n\r", ch);
	return FALSE;
    }

    if ((ilower = atoi(lower)) > (iupper = atoi(upper)))
    {
	send_to_char("AEdit:  Upper must be larger then lower.\n\r", ch);
	return FALSE;
    }

    if (!check_range(atoi(lower), atoi(upper)))
    {
	send_to_char("AEdit:  Range must include only this area.\n\r", ch);
	return FALSE;
    }

    if (get_vnum_area(ilower)
	&& get_vnum_area(ilower) != pArea)
    {
	send_to_char("AEdit:  Lower vnum already assigned.\n\r", ch);
	return FALSE;
    }

    if (get_vnum_area(iupper)
	&& get_vnum_area(iupper) != pArea)
    {
	send_to_char("AEdit:  Upper vnum already assigned.\n\r", ch);
	return FALSE;
    }

    pArea->min_vnum = ilower;
    send_to_char("Lower vnum set.\n\r", ch);

    pArea->max_vnum = iupper;
    send_to_char("Upper vnum set.\n\r", ch);

    return TRUE;
}



AEDIT(aedit_lvnum)
{
    AREA_DATA *pArea;
    char lower[MAX_STRING_LENGTH];
    int  ilower;
    int  iupper;

    EDIT_AREA(ch, pArea);

    one_argument(argument, lower);

    if (!is_number(lower) || lower[0] == '\0')
    {
	send_to_char("Syntax:  min_vnum [#xlower]\n\r", ch);
	return FALSE;
    }

    if ((ilower = atoi(lower)) > (iupper = pArea->max_vnum))
    {
	send_to_char("AEdit:  Value must be less than the max_vnum.\n\r", ch);
	return FALSE;
    }

    if (!check_range(ilower, iupper))
    {
	send_to_char("AEdit:  Range must include only this area.\n\r", ch);
	return FALSE;
    }

    if (get_vnum_area(ilower)
	&& get_vnum_area(ilower) != pArea)
    {
	send_to_char("AEdit:  Lower vnum already assigned.\n\r", ch);
	return FALSE;
    }

    pArea->min_vnum = ilower;
    send_to_char("Lower vnum set.\n\r", ch);
    return TRUE;
}



AEDIT(aedit_uvnum)
{
    AREA_DATA *pArea;
    char upper[MAX_STRING_LENGTH];
    int  ilower;
    int  iupper;

    EDIT_AREA(ch, pArea);

    one_argument(argument, upper);

    if (!is_number(upper) || upper[0] == '\0')
    {
	send_to_char("Syntax:  max_vnum [#xupper]\n\r", ch);
	return FALSE;
    }

    if ((ilower = pArea->min_vnum) > (iupper = atoi(upper)))
    {
	send_to_char("AEdit:  Upper must be larger then lower.\n\r", ch);
	return FALSE;
    }

    if (!check_range(ilower, iupper))
    {
	send_to_char("AEdit:  Range must include only this area.\n\r", ch);
	return FALSE;
    }

    if (get_vnum_area(iupper)
	&& get_vnum_area(iupper) != pArea)
    {
	send_to_char("AEdit:  Upper vnum already assigned.\n\r", ch);
	return FALSE;
    }

    pArea->max_vnum = iupper;
    send_to_char("Upper vnum set.\n\r", ch);

    return TRUE;
}

AEDIT(aedit_flags)
{
    AREA_DATA *pArea;
    int value;

    EDIT_AREA(ch, pArea);
    if ((value = flag_value(area_flags, argument)) == NO_FLAG)
    {
	send_to_char("Syntax:   flags <area_flags>\n\r", ch);
	return FALSE;
    }

    TOGGLE_BIT(pArea->area_flags, value);
    send_to_char("Area flags toggled.\n\r", ch);
    return TRUE;
}


/*
 * Room Editor Functions.
 */
REDIT(redit_show)
{
    ROOM_INDEX_DATA	*pRoom;
    char		buf  [MAX_STRING_LENGTH];
    char		buf1 [2*MAX_STRING_LENGTH];
    OBJ_DATA		*obj;
    CHAR_DATA		*rch;

    int			door;
    bool		fcnt;

    EDIT_ROOM(ch, pRoom);

    buf1[0] = '\0';

    sprintf(buf, "Description:\n\r%s", pRoom->description);
    strcat(buf1, buf);

    sprintf(buf, "Name:       [%s ]\n\rArea:       [%5d] %s\n\r",
	    pRoom->name, pRoom->area->vnum, pRoom->area->name);
    strcat(buf1, buf);

    sprintf(buf, "Vnum:       [%5d]\n\rLevels:     [%2d %2d]\n\rSector:     [%s]\n\r",
	    pRoom->vnum, pRoom->min_level, pRoom->max_level, 
	    flag_string(sector_flags, pRoom->sector_type, FALSE));
    strcat(buf1, buf);

    sprintf(buf, "Room flags: [%s]\n\r",
	    flag_string(room_flags, pRoom->room_flags, FALSE));
    strcat(buf1, buf);

    if (pRoom->heal_rate != 100 || pRoom->mana_rate != 100)
    {
	sprintf(buf, "Health rec: [%d]\n\rMana rec  : [%d]\n\r",
		pRoom->heal_rate , pRoom->mana_rate);
	strcat(buf1, buf);
    }

    if (pRoom->clan > 0)
    {
	sprintf(buf, "Clan      : [%d] %s\n\r",
		pRoom->clan,
		clan_table[pRoom->clan].name);
	strcat(buf1, buf);
    }

    if (!IS_NULLSTR(pRoom->owner))
    {
	sprintf(buf, "Owner     : [%s]\n\r", pRoom->owner);
	strcat(buf1, buf);
    }

    if (pRoom->extra_descr)
    {
	EXTRA_DESCR_DATA *ed;

	strcat(buf1, "Desc Kwds:  [");
	for (ed = pRoom->extra_descr; ed; ed = ed->next)
	{
	    strcat(buf1, ed->keyword);
	    if (ed->next)
		strcat(buf1, " ");
	}
	strcat(buf1, "]\n\r");
    }
    sprintf(buf, "Comments:   [%s]\n\r"
	    "Last Edited:[%s]\n\r",
	    pRoom->comments, pRoom->last_edited);
    strcat(buf1, buf);

    strcat(buf1, "Characters: [");
    fcnt = FALSE;
    LIST_FOREACH(rch, &pRoom->people, room_link)
    {
	one_argument(rch->name, buf);
	strcat(buf1, buf);
	strcat(buf1, " ");
	fcnt = TRUE;
    }

    if (fcnt)
    {
	int end;

	end = strlen(buf1) - 1;
	buf1[end] = ']';
	strcat(buf1, "\n\r");
    }
    else
	strcat(buf1, "none]\n\r");

    strcat(buf1, "Objects:    [");
    fcnt = FALSE;
    for (obj = pRoom->contents; obj; obj = obj->next_content)
    {
	one_argument(obj->name, buf);
	strcat(buf1, buf);
	strcat(buf1, " ");
	fcnt = TRUE;
    }

    if (fcnt)
    {
	int end;

	end = strlen(buf1) - 1;
	buf1[end] = ']';
	strcat(buf1, "\n\r");
    }
    else
	strcat(buf1, "none]\n\r");

    for (door = 0; door < MAX_DIR; door++)
    {
	EXIT_DATA *pexit;

	if ((pexit = pRoom->exit[door]))
	{
	    char word[MAX_INPUT_LENGTH];
	    char reset_state[MAX_STRING_LENGTH];
	    char *state;
	    int i, length;

	    sprintf(buf, "-%-5s to [%5d] Key: [%5d] ",
		    capitalize(dir_name[door]),
		    pexit->u1.to_room ? pexit->u1.to_room->vnum : 0,      /* ROM OLC */
		    pexit->key);
	    strcat(buf1, buf);

	    /*
	     * Format up the exit info.
	     * Capitalize all flags that are not part of the reset info.
	     */
	    strcpy(reset_state, flag_string(exit_flags, pexit->rs_flags, FALSE));
	    state = flag_string(exit_flags, pexit->exit_info, FALSE);
	    strcat(buf1, " Exit flags: [");
	    for (; ;)
	    {
		state = one_argument(state, word);

		if (word[0] == '\0')
		{
		    int end;

		    end = strlen(buf1) - 1;
		    buf1[end] = ']';
		    strcat(buf1, "\n\r");
		    break;
		}

		if (str_infix(word, reset_state))
		{
		    length = strlen(word);
		    for (i = 0; i < length; i++)
			word[i] = UPPER(word[i]);
		}
		strcat(buf1, word);
		strcat(buf1, " ");
	    }

	    if (pexit->keyword && pexit->keyword[0] != '\0')
	    {
		sprintf(buf, "Kwds: [%s]\n\r", pexit->keyword);
		strcat(buf1, buf);
	    }
	    if (pexit->description && pexit->description[0] != '\0')
	    {
		sprintf(buf, "%s", pexit->description);
		strcat(buf1, buf);
	    }
	}
    }

    send_to_char(buf1, ch);

    if (pRoom->progs)
    {
	sprintf(buf, "\n\rROOMPrograms for [%5d]:\n\r", pRoom->vnum);
	send_to_char(buf, ch);

	show_triggers(ch, pRoom->progs);
    }


    return FALSE;
}




/* Local function. */
bool change_exit(CHAR_DATA *ch, char *argument, int door)
{
    ROOM_INDEX_DATA *pRoom, *pToRoom = NULL;
    char command[MAX_INPUT_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int64_t value;

    EDIT_ROOM(ch, pRoom);

    /*
     * Set the exit flags, needs full argument.
     * ----------------------------------------
     */
    if ((value = flag_value(exit_flags, argument)) != NO_FLAG)
    {
	int16_t rev;                                    /* ROM OLC */

	if (!pRoom->exit[door])
	{
	    send_to_char("Exit isn't exist.\n\r",ch);
	    return FALSE;
	}
	/*   pRoom->exit[door] = new_exit(); */

	/*
	 * This room.
	 */
	TOGGLE_BIT(pRoom->exit[door]->rs_flags,  value);
	/* Don't toggle exit_info because it can be changed by players. */
	pRoom->exit[door]->exit_info = pRoom->exit[door]->rs_flags;

	/*
	 * Connected room.
	 */
	pToRoom = pRoom->exit[door]->u1.to_room;     /* ROM OLC */
	rev = rev_dir[door];

	if (pToRoom->exit[rev] != NULL)
	{
	    TOGGLE_BIT(pToRoom->exit[rev]->rs_flags,  value);
	    TOGGLE_BIT(pToRoom->exit[rev]->exit_info, value);
	}

	send_to_char("Exit flag toggled.\n\r", ch);
	goto EXIT_TRUE;
    }

    /*
     * Now parse the arguments.
     */
    argument = one_argument(argument, command);
    one_argument(argument, arg);

    if (command[0] == '\0' && argument[0] == '\0')	/* Move command. */
    {
	move_char(ch, door, TRUE, FALSE);                    /* ROM OLC */
	return FALSE;
    }

    if (command[0] == '?')
    {
	do_help(ch, "EXIT");
	return FALSE;
    }

    if (!str_cmp(command, "delete"))
    {
	int16_t rev;                                     /* ROM OLC */

	if (!pRoom->exit[door])
	{
	    send_to_char("REdit:  Cannot delete a null exit.\n\r", ch);
	    return FALSE;
	}

	/*
	 * Remove ToRoom Exit.
	 */
	rev = rev_dir[door];
	pToRoom = pRoom->exit[door]->u1.to_room;       /* ROM OLC */

	if (pToRoom->exit[rev])
	{
	    free_exit(pToRoom->exit[rev]);
	    pToRoom->exit[rev] = NULL;
	}

	/*
	 * Remove this exit.
	 */
	free_exit(pRoom->exit[door]);
	pRoom->exit[door] = NULL;

	send_to_char("Exit unlinked.\n\r", ch);
	goto EXIT_TRUE;
    }

    if (!str_cmp(command, "link"))
    {
	EXIT_DATA *pExit;

	if (arg[0] == '\0' || !is_number(arg))
	{
	    send_to_char("Syntax:  [direction] link [vnum]\n\r", ch);
	    return FALSE;
	}

	value = atoi(arg);

	pToRoom = get_room_index(value);
	if (!pToRoom)
	{
	    send_to_char("REdit:  Cannot link to non-existant room.\n\r", ch);
	    return FALSE;
	}

	if (!IS_BUILDER(ch, pToRoom->area))
	{
	    send_to_char("REdit:  Cannot link to that area.\n\r", ch);
	    return FALSE;
	}

	if (pToRoom->exit[rev_dir[door]])
	{
	    send_to_char("REdit:  Remote side's exit already exists.\n\r", ch);
	    return FALSE;
	}

	if (!pRoom->exit[door])
	{
	    pRoom->exit[door] = new_exit();
	}

	pRoom->exit[door]->u1.to_room = pToRoom;   /* ROM OLC */
	pRoom->exit[door]->orig_door = door;

	/*	pRoom->exit[door]->vnum = value;                Can't set vnum in ROM */

	pRoom                   = pToRoom;
	door                    = rev_dir[door];
	pExit                   = new_exit();
	pExit->u1.to_room       = ch->in_room;
	/*	pExit->vnum             = ch->in_room->vnum;    Can't set vnum in ROM */
	pExit->orig_door	= door;
	pRoom->exit[door]       = pExit;

	SET_BIT(pToRoom->area->area_flags, AREA_CHANGED);
	send_to_char("Two-way link established.\n\r", ch);
	goto EXIT_TRUE;
    }

    if (!str_cmp(command, "dig"))
    {
	char buf[MAX_STRING_LENGTH];

	if (arg[0] == '\0' || !is_number(arg))
	{
	    send_to_char("Syntax: [direction] dig <vnum>\n\r", ch);
	    return FALSE;
	}

	redit_create(ch, arg);
	sprintf(buf, "link %s", arg);
	change_exit(ch, buf, door);
	return TRUE;
    }

    if (!str_cmp(command, "room"))
    {
	if (arg[0] == '\0' || !is_number(arg))
	{
	    send_to_char("Syntax:  [direction] room [vnum]\n\r", ch);
	    return FALSE;
	}

	if (!pRoom->exit[door])
	{
	    pRoom->exit[door] = new_exit();
	}

	value = atoi(arg);

	if (!(pToRoom = get_room_index(value)))
	{
	    send_to_char("REdit:  Cannot link to non-existant room.\n\r", ch);
	    free_exit(pRoom->exit[door]);
	    pRoom->exit[door] = NULL;
	    return FALSE;
	}

	pRoom->exit[door]->u1.to_room = pToRoom;    /* ROM OLC */
	pRoom->exit[door]->orig_door = door;
	/*	pRoom->exit[door]->vnum = value;                 Can't set vnum in ROM */

	send_to_char("One-way link established.\n\r", ch);
	return TRUE;
    }

    if (!str_cmp(command, "key"))
    {
	if (arg[0] == '\0' || !is_number(arg))
	{
	    send_to_char("Syntax:  [direction] key [vnum]\n\r", ch);
	    return FALSE;
	}

	if (!pRoom->exit[door])
	{
	    send_to_char("Exit isn't exist.\n\r",ch);
	    return FALSE;
	}

	/*	if (!pRoom->exit[door])
	 {
	 pRoom->exit[door] = new_exit();
	 } */

	value = atoi(arg);

	if (!get_obj_index(value))
	{
	    send_to_char("REdit:  Item doesn't exist.\n\r", ch);
	    return FALSE;
	}

	if (get_obj_index(atoi(argument))->item_type != ITEM_KEY)
	{
	    send_to_char("REdit:  Key doesn't exist.\n\r", ch);
	    return FALSE;
	}

	pRoom->exit[door]->key = value;

	send_to_char("Exit key set.\n\r", ch);
	return TRUE;
    }

    if (!str_cmp(command, "name"))
    {
	if (arg[0] == '\0')
	{
	    send_to_char("Syntax:  [direction] name [string]\n\r", ch);
	    send_to_char("         [direction] name none\n\r", ch);
	    return FALSE;
	}

	if (!pRoom->exit[door])
	{
	    send_to_char("Exit isn't exist.\n\r",ch);
	    return FALSE;
	}

	/*	if (!pRoom->exit[door])
	 {
	 pRoom->exit[door] = new_exit();
	 } */

	free_string(pRoom->exit[door]->keyword);
	if (str_cmp(arg,"none"))
	    pRoom->exit[door]->keyword = str_dup(arg);
	else
	    pRoom->exit[door]->keyword = str_dup("");

	send_to_char("Exit name set.\n\r", ch);
	return TRUE;
    }

    if (!str_prefix(command, "description"))
    {
	if (arg[0] == '\0')
	{
	    if (!pRoom->exit[door])
	    {
		send_to_char("Exit isn't exist.\n\r",ch);
		return FALSE;
	    }

	    /*	    if (!pRoom->exit[door])
	     {
	     pRoom->exit[door] = new_exit();
	     } */

	    string_append(ch, &pRoom->exit[door]->description);
	    return TRUE;
	}

	send_to_char("Syntax:  [direction] desc\n\r", ch);
	return FALSE;
    }

    return FALSE;

EXIT_TRUE:
    if (pToRoom != NULL)
	SET_BIT(pToRoom->area->area_flags, AREA_CHANGED);

    return TRUE;
}



REDIT(redit_north)
{
    return change_exit(ch, argument, DIR_NORTH);
}



REDIT(redit_south)
{
    return change_exit(ch, argument, DIR_SOUTH);
}



REDIT(redit_east)
{
    return change_exit(ch, argument, DIR_EAST);
}



REDIT(redit_west)
{
    return change_exit(ch, argument, DIR_WEST);
}



REDIT(redit_up)
{
    return change_exit(ch, argument, DIR_UP);
}



REDIT(redit_down)
{
    return change_exit(ch, argument, DIR_DOWN);
}



REDIT(redit_ed)
{
    ROOM_INDEX_DATA *pRoom;
    EXTRA_DESCR_DATA *ed;
    char command[MAX_INPUT_LENGTH];
    char keyword[MAX_INPUT_LENGTH];

    EDIT_ROOM(ch, pRoom);

    argument = one_argument(argument, command);
    one_argument(argument, keyword);

    if (command[0] == '\0' || keyword[0] == '\0')
    {
	send_to_char("Syntax:  ed add [keyword]\n\r", ch);
	send_to_char("         ed edit [keyword]\n\r", ch);
	send_to_char("         ed delete [keyword]\n\r", ch);
	send_to_char("         ed format [keyword]\n\r", ch);
	return FALSE;
    }

    if (!str_cmp(command, "add"))
    {
	if (keyword[0] == '\0')
	{
	    send_to_char("Syntax:  ed add [keyword]\n\r", ch);
	    return FALSE;
	}

	ed			=   new_extra_descr();
	ed->keyword		=   str_dup(keyword);
	ed->description		=   str_dup("");
	ed->next		=   pRoom->extra_descr;
	pRoom->extra_descr	=   ed;

	string_append(ch, &ed->description);

	return TRUE;
    }


    if (!str_cmp(command, "edit"))
    {
	if (keyword[0] == '\0')
	{
	    send_to_char("Syntax:  ed edit [keyword]\n\r", ch);
	    return FALSE;
	}

	for (ed = pRoom->extra_descr; ed; ed = ed->next)
	{
	    if (is_name(keyword, ed->keyword))
		break;
	}

	if (!ed)
	{
	    send_to_char("REdit:  Extra description keyword not found.\n\r", ch);
	    return FALSE;
	}

	string_append(ch, &ed->description);

	return TRUE;
    }


    if (!str_cmp(command, "delete"))
    {
	EXTRA_DESCR_DATA *ped = NULL;

	if (keyword[0] == '\0')
	{
	    send_to_char("Syntax:  ed delete [keyword]\n\r", ch);
	    return FALSE;
	}

	for (ed = pRoom->extra_descr; ed; ed = ed->next)
	{
	    if (is_name(keyword, ed->keyword))
		break;
	    ped = ed;
	}

	if (!ed)
	{
	    send_to_char("REdit:  Extra description keyword not found.\n\r", ch);
	    return FALSE;
	}

	if (!ped)
	    pRoom->extra_descr = ed->next;
	else
	    ped->next = ed->next;

	free_extra_descr(ed);

	send_to_char("Extra description deleted.\n\r", ch);
	return TRUE;
    }


    if (!str_cmp(command, "format"))
    {
	if (keyword[0] == '\0')
	{
	    send_to_char("Syntax:  ed format [keyword]\n\r", ch);
	    return FALSE;
	}

	for (ed = pRoom->extra_descr; ed; ed = ed->next)
	{
	    if (is_name(keyword, ed->keyword))
		break;
	}

	if (!ed)
	{
	    send_to_char("REdit:  Extra description keyword not found.\n\r", ch);
	    return FALSE;
	}

	ed->description = format_string(ed->description);

	send_to_char("Extra description formatted.\n\r", ch);
	return TRUE;
    }

    redit_ed(ch, "");
    return FALSE;
}



REDIT(redit_create)
{
    AREA_DATA *pArea;
    ROOM_INDEX_DATA *pRoom;
    int value;
    int iHash;

    EDIT_ROOM(ch, pRoom);

    value = atoi(argument);

    if (argument[0] == '\0' || value <= 0)
    {
	send_to_char("Syntax:  create [vnum > 0]\n\r", ch);
	return FALSE;
    }

    pArea = get_vnum_area(value);
    if (!pArea)
    {
	send_to_char("REdit:  That vnum is not assigned an area.\n\r", ch);
	return FALSE;
    }

    if (!IS_BUILDER(ch, pArea))
    {
	send_to_char("REdit:  Vnum in an area you cannot build in.\n\r", ch);
	return FALSE;
    }

    if (get_room_index(value))
    {
	send_to_char("REdit:  Room vnum already exists.\n\r", ch);
	return FALSE;
    }

    pRoom			= new_room_index();
    pRoom->area			= pArea;
    pRoom->vnum			= value;

    if (value > top_vnum_room)
	top_vnum_room = value;

    iHash			= value % MAX_KEY_HASH;
    pRoom->next			= room_index_hash[iHash];
    room_index_hash[iHash]	= pRoom;
    ch->desc->pEdit		= (void *)pRoom;

    send_to_char("Room created.\n\r", ch);
    return TRUE;
}



REDIT(redit_name)
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM(ch, pRoom);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax:  name [name]\n\r", ch);
	return FALSE;
    }

    free_string(pRoom->name);
    pRoom->name = str_dup(argument);

    send_to_char("Name set.\n\r", ch);
    return TRUE;
}



REDIT(redit_desc)
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM(ch, pRoom);

    if (argument[0] == '\0')
    {
	string_append(ch, &pRoom->description);
	return TRUE;
    }

    send_to_char("Syntax:  desc\n\r", ch);
    return FALSE;
}

REDIT(redit_heal)
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM(ch, pRoom);

    if (is_number(argument))
    {
	pRoom->heal_rate = atoi (argument);
	send_to_char ("Heal rate set.\n\r", ch);
	return TRUE;
    }

    send_to_char ("Syntax : heal <#xnumber>\n\r", ch);
    return FALSE;
}

REDIT(redit_mana)
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM(ch, pRoom);

    if (is_number(argument))
    {
	pRoom->mana_rate = atoi (argument);
	send_to_char ("Mana rate set.\n\r", ch);
	return TRUE;
    }

    send_to_char ("Syntax : mana <#xnumber>\n\r", ch);
    return FALSE;
}

REDIT(redit_clan)
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM(ch, pRoom);

    pRoom->clan = clan_lookup(argument);

    send_to_char("Clan set.\n\r", ch);
    return TRUE;
}

REDIT(redit_format)
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM(ch, pRoom);

    pRoom->description = format_string(pRoom->description);

    send_to_char("String formatted.\n\r", ch);
    return TRUE;
}



REDIT(redit_mreset)
{
    ROOM_INDEX_DATA	*pRoom;
    MOB_INDEX_DATA	*pMobIndex;
    CHAR_DATA		*newmob;
    char		arg [ MAX_INPUT_LENGTH ];
    char		arg2 [ MAX_INPUT_LENGTH ];

    RESET_DATA		*pReset;
    char		output [ MAX_STRING_LENGTH ];

    EDIT_ROOM(ch, pRoom);

    argument = one_argument(argument, arg);
    argument = one_argument(argument, arg2);

    if (arg[0] == '\0' || !is_number(arg))
    {
	send_to_char ("Syntax:  mreset <vnum> <max #x> <mix #x>\n\r", ch);
	return FALSE;
    }

    if (!(pMobIndex = get_mob_index(atoi(arg))))
    {
	send_to_char("REdit: No mobile has that vnum.\n\r", ch);
	return FALSE;
    }

    if (pMobIndex->area != pRoom->area)
    {
	send_to_char("REdit: No such mobile in this area.\n\r", ch);
	return FALSE;
    }

    /*
     * Create the mobile reset.
     */
    pReset              = new_reset_data();
    pReset->command	= 'M';
    pReset->arg1	= pMobIndex->vnum;
    pReset->arg2	= is_number(arg2) ? atoi(arg2) : MAX_MOB;
    pReset->arg3	= pRoom->vnum;
    pReset->arg4	= is_number(argument) ? atoi (argument) : 1;
    add_reset(pRoom, pReset, 0/* Last slot*/);

    /*
     * Create the mobile.
     */
    newmob = create_mobile(pMobIndex);
    char_to_room(newmob, pRoom, FALSE);

    sprintf(output, "%s (%d) has been loaded and added to resets.\n\r"
	    "There will be a maximum of %d loaded to this room.\n\r",
	    capitalize(pMobIndex->short_descr),
	    pMobIndex->vnum,
	    pReset->arg2);
    send_to_char(output, ch);
    act("$n has created $N!", ch, NULL, newmob, TO_ROOM);
    return TRUE;
}



struct wear_type
{
    int	wear_loc;
    int	wear_bit;
};



const struct wear_type wear_table[] =
{
    {	WEAR_NONE,	ITEM_TAKE		},
    {	WEAR_LIGHT,	ITEM_LIGHT		},
    {	WEAR_FINGER_L,	ITEM_WEAR_FINGER	},
    {	WEAR_FINGER_R,	ITEM_WEAR_FINGER	},
    {	WEAR_NECK_1,	ITEM_WEAR_NECK		},
    {	WEAR_NECK_2,	ITEM_WEAR_NECK		},
    {	WEAR_BODY,	ITEM_WEAR_BODY		},
    {	WEAR_HEAD,	ITEM_WEAR_HEAD		},
    {	WEAR_LEGS,	ITEM_WEAR_LEGS		},
    {	WEAR_FEET,	ITEM_WEAR_FEET		},
    {	WEAR_HANDS,	ITEM_WEAR_HANDS		},
    {	WEAR_ARMS,	ITEM_WEAR_ARMS		},
    {	WEAR_SHIELD,	ITEM_WEAR_SHIELD	},
    {	WEAR_ABOUT,	ITEM_WEAR_ABOUT		},
    {	WEAR_WAIST,	ITEM_WEAR_WAIST		},
    {	WEAR_WRIST_L,	ITEM_WEAR_WRIST		},
    {	WEAR_WRIST_R,	ITEM_WEAR_WRIST		},
    {	WEAR_WIELD,	ITEM_WIELD		},
    {	WEAR_HOLD,	ITEM_HOLD		},
    {	NO_FLAG,	NO_FLAG			}
};



/*****************************************************************************
Name:		wear_loc
Purpose:	Returns the location of the bit that matches the count.
1 = first match, 2 = second match etc.
Called by:	oedit_reset(olc_act.c).
 ****************************************************************************/
int wear_loc(int bits, int count)
{
    int flag;

    for (flag = 0; wear_table[flag].wear_bit != NO_FLAG; flag++)
    {
	if (IS_SET(bits, wear_table[flag].wear_bit) && --count < 1)
	    return wear_table[flag].wear_loc;
    }

    return NO_FLAG;
}



/*****************************************************************************
Name:		wear_bit
Purpose:	Converts a wear_loc into a bit.
Called by:	redit_oreset(olc_act.c).
 ****************************************************************************/
int wear_bit(int loc)
{
    int flag;

    for (flag = 0; wear_table[flag].wear_loc != NO_FLAG; flag++)
    {
	if (loc == wear_table[flag].wear_loc)
	    return wear_table[flag].wear_bit;
    }

    return 0;
}



REDIT(redit_oreset)
{
    ROOM_INDEX_DATA	*pRoom;
    OBJ_INDEX_DATA	*pObjIndex;
    OBJ_DATA		*newobj;
    OBJ_DATA		*to_obj;
    CHAR_DATA		*to_mob;
    char		arg1 [ MAX_INPUT_LENGTH ];
    char		arg2 [ MAX_INPUT_LENGTH ];
    int			olevel = 0;

    RESET_DATA		*pReset;
    char		output [ MAX_STRING_LENGTH ];

    EDIT_ROOM(ch, pRoom);

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || !is_number(arg1))
    {
	send_to_char ("Syntax:  oreset <vnum> <args>\n\r", ch);
	send_to_char ("        -no_args               = into room\n\r", ch);
	send_to_char ("        -<obj_name>            = into obj\n\r", ch);
	send_to_char ("        -<mob_name> <wear_loc> = into mob\n\r", ch);
	return FALSE;
    }

    if (!(pObjIndex = get_obj_index(atoi(arg1))))
    {
	send_to_char("REdit: No object has that vnum.\n\r", ch);
	return FALSE;
    }

    if (pObjIndex->area != pRoom->area)
    {
	send_to_char("REdit: No such object in this area.\n\r", ch);
	return FALSE;
    }

    /*
     * Load into room.
     */
    if (arg2[0] == '\0')
    {
	pReset		= new_reset_data();
	pReset->command	= 'O';
	pReset->arg1	= pObjIndex->vnum;
	pReset->arg2	= 0;
	pReset->arg3	= pRoom->vnum;
	pReset->arg4	= 0;
	add_reset(pRoom, pReset, 0/* Last slot*/);

	newobj = create_object(pObjIndex, number_fuzzy(olevel));
	obj_to_room(newobj, pRoom);

	sprintf(output, "%s (%d) has been loaded and added to resets.\n\r",
		capitalize(pObjIndex->short_descr),
		pObjIndex->vnum);
	send_to_char(output, ch);
    }
    else if (argument[0] == '\0'
	     && ((to_obj = get_obj_list(ch, arg2, pRoom->contents)) != NULL))
    {
	/*
	 * Load into object's inventory.
	 */
	pReset		= new_reset_data();
	pReset->command	= 'P';
	pReset->arg1	= pObjIndex->vnum;
	pReset->arg2	= 0;
	pReset->arg3	= to_obj->pIndexData->vnum;
	pReset->arg4	= 1;
	add_reset(pRoom, pReset, 0/* Last slot*/);

	newobj = create_object(pObjIndex, number_fuzzy(olevel));
	newobj->cost = 0;
	obj_to_obj(newobj, to_obj);

	sprintf(output, "%s (%d) has been loaded into "
		"%s (%d) and added to resets.\n\r",
		capitalize(newobj->short_descr),
		newobj->pIndexData->vnum,
		to_obj->short_descr,
		to_obj->pIndexData->vnum);
	send_to_char(output, ch);
    }
    else if ((to_mob = get_char_room(ch, NULL, arg2, FALSE)) != NULL)
    {
	/*
	 * Load into mobile's inventory.
	 */

	int	wear_loc;

	/*
	 * Make sure the location on mobile is valid.
	 */
	if ((wear_loc = flag_value(wear_loc_flags, argument)) == NO_FLAG)
	{
	    send_to_char("REdit: Invalid wear_loc.  '? wear-loc'\n\r", ch);
	    return FALSE;
	}

	/*
	 * Disallow loading a sword(WEAR_WIELD) into WEAR_HEAD.
	 */
	if (!IS_SET(pObjIndex->wear_flags, wear_bit(wear_loc)))
	{
	    sprintf(output,
		    "%s (%d) has wear flags: [%s]\n\r",
		    capitalize(pObjIndex->short_descr),
		    pObjIndex->vnum,
		    flag_string(wear_flags, pObjIndex->wear_flags, FALSE));
	    send_to_char(output, ch);
	    return FALSE;
	}

	/*
	 * Can't load into same position.
	 */
	if (get_eq_char(to_mob, wear_loc))
	{
	    send_to_char("REdit:  Object already equipped.\n\r", ch);
	    return FALSE;
	}

	pReset		= new_reset_data();
	pReset->arg1	= pObjIndex->vnum;
	pReset->arg2	= wear_loc;
	if (pReset->arg2 == WEAR_NONE)
	    pReset->command = 'G';
	else
	    pReset->command = 'E';
	pReset->arg3	= wear_loc;

	add_reset(pRoom, pReset, 0/* Last slot*/);

	olevel  = URANGE(0, to_mob->level - 2, LEVEL_HERO);
	newobj = create_object(pObjIndex, number_fuzzy(olevel));

	if (to_mob->pIndexData->pShop)	/* Shop-keeper? */
	{
	    switch (pObjIndex->item_type)
	    {
	    default:		olevel = 0;			break;
	    case ITEM_PILL:	olevel = number_range(0, 10);	break;
	    case ITEM_POTION:	olevel = number_range(0, 10);	break;
	    case ITEM_SCROLL:	olevel = number_range(5, 15);	break;
	    case ITEM_WAND:	olevel = number_range(10, 20);	break;
	    case ITEM_STAFF:	olevel = number_range(15, 25);	break;
	    case ITEM_ARMOR:	olevel = number_range(5, 15);	break;
	    case ITEM_WEAPON:	if (pReset->command == 'G')
				    olevel = number_range(5, 15);
				else
				    olevel = number_fuzzy(olevel);
				break;
	    }

	    newobj = create_object(pObjIndex, olevel);
	    if (pReset->arg2 == WEAR_NONE)
		SET_BIT(newobj->extra_flags, ITEM_INVENTORY);
	}
	else
	    newobj = create_object(pObjIndex, number_fuzzy(olevel));

	obj_to_char(newobj, to_mob);
	if (pReset->command == 'E')
	    equip_char(to_mob, newobj, pReset->arg3);

	sprintf(output, "%s (%d) has been loaded "
		"%s of %s (%d) and added to resets.\n\r",
		capitalize(pObjIndex->short_descr),
		pObjIndex->vnum,
		flag_string(wear_loc_strings, pReset->arg3, FALSE),
		to_mob->short_descr,
		to_mob->pIndexData->vnum);
	send_to_char(output, ch);
    }
    else	/* Display Syntax */
    {
	send_to_char("REdit:  That mobile isn't here.\n\r", ch);
	return FALSE;
    }

    act("$n has created $p!", ch, newobj, NULL, TO_ROOM);
    return TRUE;
}



/*
 * Object Editor Functions.
 */
void show_obj_values(CHAR_DATA *ch, OBJ_INDEX_DATA *obj)
{
    char buf[MAX_STRING_LENGTH];
    int time1;

    switch (obj->item_type)
    {
    default:	/* No values. */
	break;

    case ITEM_LIGHT:
	if (obj->value[2] == -1 || obj->value[2] == 999) /* ROM OLC */
	    sprintf(buf, "[v2] Light:  Infinite[-1]\n\r");
	else
	    sprintf(buf, "[v2] Light:  [%d]\n\r", obj->value[2]);
	send_to_char(buf, ch);
	break;

    case ITEM_WAND:
    case ITEM_STAFF:
	sprintf(buf,
		"[v0] Level:          [%d]\n\r"
		"[v1] Charges Total:  [%d]\n\r"
		"[v2] Charges Left:   [%d]\n\r"
		"[v3] Spell:          %s\n\r",
		obj->value[0],
		obj->value[1],
		obj->value[2],
		obj->value[3] != -1 ? skill_table[obj->value[3]].name
		: "none");
	send_to_char(buf, ch);
	break;

    case ITEM_PORTAL:
	sprintf(buf,
		"[v0] Charges:        [%d]\n\r"
		"[v1] Exit Flags:     %s\n\r",
		obj->value[0],
		flag_string(exit_flags, obj->value[1], FALSE));
	send_to_char(buf, ch);

	sprintf(buf,
		"[v2] Portal Flags:   %s\n\r"
		"[v3] Goes to (vnum): [%d]\n\r"
		"[v4] Portal Key    : [%d]\n\r",
		flag_string(portal_flags , obj->value[2], FALSE),
		obj->value[3], obj->value[4]);
	send_to_char(buf, ch);
	break;

    case ITEM_FURNITURE:
	sprintf(buf,
		"[v0] Max people:      [%d]\n\r"
		"[v1] Max weight:      [%d]\n\r"
		"[v2] Furniture Flags: %s\n\r"
		"[v3] Heal bonus:      [%d]\n\r"
		"[v4] Mana bonus:      [%d]\n\r",
		obj->value[0],
		obj->value[1],
		flag_string(furniture_flags, obj->value[2], FALSE),
		obj->value[3],
		obj->value[4]);
	send_to_char(buf, ch);
	break;

    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_PILL:
	sprintf(buf,
		"[v0] Level:  [%d]\n\r"
		"[v1] Spell:  %s\n\r"
		"[v2] Spell:  %s\n\r"
		"[v3] Spell:  %s\n\r"
		"[v4] Spell:  %s\n\r",
		obj->value[0],
		obj->value[1] != -1 ? skill_table[obj->value[1]].name
		: "none",
		obj->value[2] != -1 ? skill_table[obj->value[2]].name
		: "none",
		obj->value[3] != -1 ? skill_table[obj->value[3]].name
		: "none",
		obj->value[4] != -1 ? skill_table[obj->value[4]].name
		: "none");
	send_to_char(buf, ch);
	break;

	/* ARMOR for ROM */

    case ITEM_ARMOR:
	sprintf(buf,
		"[v0] Ac pierce       [%d]\n\r"
		"[v1] Ac bash         [%d]\n\r"
		"[v2] Ac slash        [%d]\n\r"
		"[v3] Ac exotic       [%d]\n\r",
		obj->value[0],
		obj->value[1],
		obj->value[2],
		obj->value[3]);
	send_to_char(buf, ch);
	break;

	/* WEAPON changed in ROM: */
	/* I had to split the output here, I have no idea why, but it helped */
	/* It somehow fixed a bug in showing scroll/pill/potions too ?! */
    case ITEM_WEAPON:
	sprintf(buf, "[v0] Weapon class:   %s\n\r",
		flag_string(weapon_class, obj->value[0], FALSE));
	send_to_char(buf, ch);
	sprintf(buf, "[v1] Number of dice: [%d]\n\r", obj->value[1]);
	send_to_char(buf, ch);
	sprintf(buf, "[v2] Type of dice:   [%d]    (Average: %d)\n\r",
		obj->value[2], (1 + obj->value[2]) * obj->value[1] / 2);
	send_to_char(buf, ch);
	sprintf(buf, "[v3] Type:           %s\n\r",
		attack_table[obj->value[3]].name);
	send_to_char(buf, ch);
	sprintf(buf, "[v4] Special type:   %s\n\r",
		flag_string(weapon_type2,  obj->value[4], FALSE));
	send_to_char(buf, ch);
	break;

    case ITEM_CONTAINER:
	sprintf(buf,
		"[v0] Weight capacity:  [%d kg]\n\r"
		"[v1] Flags:            [%s]\n\r"
		"[v2] Key:         %s   [%d]\n\r"
		"[v3] Max weight        [%d]\n\r"
		"[v4] Weight Mult       [%d]\n\r",
		obj->value[0],
		flag_string(container_flags, obj->value[1], FALSE),
		get_obj_index(obj->value[2])
		? get_obj_index(obj->value[2])->short_descr
		: "none",
		obj->value[2],
		obj->value[3],
		obj->value[4]);
	send_to_char(buf, ch);
	break;

    case ITEM_ROD:
	sprintf(buf,
		"[v0] Level:        [%d]\n\r"
		"[v1] Spell:         %s\n\r"
		"[v2] RechargeTime: [%d]\n\r",
		obj->value[0],
		obj->value[1] > 0 ? skill_table[obj->value[1]].name : "",
		obj->value[2]);
	send_to_char(buf, ch);
	break;

    case ITEM_TRAP:
	sprintf(buf,
		"[v0] Number of dice:  [%d]\n\r"
		"[v1] Type of dice:    [%d]\n\r"
		"[v2] (internal)\n\r"
		"[v3] Damage type:     [%s]\n\r"
		"[v4] Delay:           [%d]\n\r",
		obj->value[0],
		obj->value[1],
		flag_string(dam_flags, obj->value[3], FALSE),
		obj->value[4] / PULSE_VIOLENCE);
	send_to_char(buf, ch);
	break;

    case ITEM_DRINK_CON:
	sprintf(buf,
		"[v0] Liquid Total: [%d]\n\r"
		"[v1] Liquid Left:  [%d]\n\r"
		"[v2] Liquid:       %s\n\r"
		"[v3] Poisoned:     %s (%d)\n\r",
		obj->value[0],
		obj->value[1],
		liq_table[obj->value[2]].liq_name,
		obj->value[3] != 0 ? "Yes" : "No", obj->value[3]);
	send_to_char(buf, ch);
	break;

    case ITEM_FOUNTAIN:
	sprintf(buf,
		"[v0] Liquid Total: [%d]\n\r"
		"[v1] Liquid Left:  [%d]\n\r"
		"[v2] Liquid:	    %s\n\r",
		obj->value[0],
		obj->value[1],
		liq_table[obj->value[2]].liq_name);
	send_to_char(buf,ch);
	break;

    case ITEM_FOOD:
	sprintf(buf,
		"[v0] Food hours: [%d]\n\r"
		"[v1] Full hours: [%d]\n\r"
		"[v3] Poisoned:   %s (%d)\n\r",
		obj->value[0],
		obj->value[1],
		obj->value[3] != 0 ? "Yes" : "No", obj->value[3]);
	send_to_char(buf, ch);
	break;

    case ITEM_MONEY:
	sprintf(buf, "[v0] Silver:   [%d]\n\r", obj->value[0]);
	send_to_char(buf, ch);
	break;
    case ITEM_ARTIFACT:
	sprintf(buf, "[v0] Chance:   [%d]\n\r", obj->value[0]);
	send_to_char(buf, ch);
	break;
    case ITEM_MAP:
	sprintf(buf, "[v1] Recite zone (vnum):   [%d]\n\r", obj->value[1]);
	send_to_char(buf, ch);
	break;

    case ITEM_CHEST:
	sprintf(buf,
		"[v0] Weight capacity:  [%d kg]\n\r"
		"[v1] Flags:            [%s]\n\r"
		"[v2] Key:         %s   [%d]\n\r"
		"[v3] Max weight        [%d]\n\r"
		"[v4] Weight Mult       [%d]\n\r",
		obj->value[0],
		flag_string(chest_flags, obj->value[1], FALSE),
		get_obj_index(obj->value[2])
		? get_obj_index(obj->value[2])->short_descr
		: "none",
		obj->value[2],
		obj->value[3],
		obj->value[4]);
	send_to_char(buf, ch);
	break;

    case ITEM_MORTAR:
	sprintf(buf,
		"[v0] Weight capacity:  [%d kg]\n\r"
		"[v1] Flags:            [%s]\n\r"
		"[v2] Max number:       [%d]\n\r"
		"[v3] Max weight        [%d]\n\r"
		"[v4] Weight Mult       [%d]\n\r",
		obj->value[0],
		flag_string(chest_flags, obj->value[1], FALSE),
		obj->value[2],
		obj->value[3],
		obj->value[4]);
	send_to_char(buf, ch);
	break;

    case ITEM_INGREDIENT:

	time1 = obj->value[0] / 100;

	sprintf(buf,
		"[v0] Search time:      [%d %d]\n\r"	/*время появления*/
		"[v1] Search area:      [%s]\n\r"	/*местность*/
		"[v2] Search race:      [%s]\n\r",	/*раса*/
		time1,(obj->value[0]-time1*100),
		flag_string(ing_sector_flags, obj->value[1], FALSE),
		race_table[obj->value[2]].rname);
	sprintf(buf,"%s"
		"[v3] Search season:    [%s]\n\r"	/*погода*/
		"[v4] Chance find:      [%d]\n\r",	/*шанс найти*/
		buf,
		flag_string(ing_weather_flags, obj->value[3], FALSE),
		obj->value[4]);
	send_to_char(buf, ch);
	break;
    }

    return;
}



bool set_obj_values(CHAR_DATA *ch, OBJ_INDEX_DATA *pObj, int value_num, char *argument)
{
    switch (pObj->item_type)
    {
    default:
	break;

    case ITEM_LIGHT:
	switch (value_num)
	{
	default:
	    do_help(ch, "ITEM_LIGHT");
	    return FALSE;
	case 2:
	    send_to_char("HOURS OF LIGHT SET.\n\r\n\r", ch);
	    pObj->value[2] = atoi(argument);
	    break;
	}
	break;

    case ITEM_WAND:
    case ITEM_STAFF:
	switch (value_num)
	{
	default:
	    do_help(ch, "ITEM_STAFF_WAND");
	    return FALSE;
	case 0:
	    send_to_char("SPELL LEVEL SET.\n\r\n\r", ch);
	    pObj->value[0] = atoi(argument);
	    break;
	case 1:
	    send_to_char("TOTAL NUMBER OF CHARGES SET.\n\r\n\r", ch);
	    pObj->value[1] = atoi(argument);
	    break;
	case 2:
	    send_to_char("CURRENT NUMBER OF CHARGES SET.\n\r\n\r", ch);
	    pObj->value[2] = atoi(argument);
	    break;
	case 3:
	    send_to_char("SPELL TYPE SET.\n\r", ch);
	    pObj->value[3] = skill_lookup(argument);
	    break;
	}
	break;

    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_PILL:
	switch (value_num)
	{
	default:
	    do_help(ch, "ITEM_SCROLL_POTION_PILL");
	    return FALSE;
	case 0:
	    send_to_char("SPELL LEVEL SET.\n\r\n\r", ch);
	    pObj->value[0] = atoi(argument);
	    break;
	case 1:
	    send_to_char("SPELL TYPE 1 SET.\n\r\n\r", ch);
	    pObj->value[1] = skill_lookup(argument);
	    break;
	case 2:
	    send_to_char("SPELL TYPE 2 SET.\n\r\n\r", ch);
	    pObj->value[2] = skill_lookup(argument);
	    break;
	case 3:
	    send_to_char("SPELL TYPE 3 SET.\n\r\n\r", ch);
	    pObj->value[3] = skill_lookup(argument);
	    break;
	case 4:
	    send_to_char("SPELL TYPE 4 SET.\n\r\n\r", ch);
	    pObj->value[4] = skill_lookup(argument);
	    break;
	}
	break;

	/* ARMOR for ROM: */

    case ITEM_ARMOR:
	switch (value_num)
	{
	default:
	    do_help(ch, "ITEM_ARMOR");
	    return FALSE;
	case 0:
	    send_to_char("AC PIERCE SET.\n\r\n\r", ch);
	    pObj->value[0] = atoi(argument);
	    break;
	case 1:
	    send_to_char("AC BASH SET.\n\r\n\r", ch);
	    pObj->value[1] = atoi(argument);
	    break;
	case 2:
	    send_to_char("AC SLASH SET.\n\r\n\r", ch);
	    pObj->value[2] = atoi(argument);
	    break;
	case 3:
	    send_to_char("AC EXOTIC SET.\n\r\n\r", ch);
	    pObj->value[3] = atoi(argument);
	    break;
	}
	break;

	/* WEAPONS changed in ROM */

    case ITEM_WEAPON:
	switch (value_num)
	{
	default:
	    do_help(ch, "ITEM_WEAPON");
	    return FALSE;
	case 0:
	    send_to_char("WEAPON CLASS SET.\n\r\n\r", ch);
	    pObj->value[0] = flag_value(weapon_class, argument);
	    break;
	case 1:
	    send_to_char("NUMBER OF DICE SET.\n\r\n\r", ch);
	    pObj->value[1] = atoi(argument);
	    break;
	case 2:
	    send_to_char("TYPE OF DICE SET.\n\r\n\r", ch);
	    pObj->value[2] = atoi(argument);
	    break;
	case 3:
	    send_to_char("WEAPON TYPE SET.\n\r\n\r", ch);
	    pObj->value[3] = attack_lookup(argument);
	    break;
	case 4:
	    send_to_char("SPECIAL WEAPON TYPE TOGGLED.\n\r\n\r", ch);
	    pObj->value[4] ^= (flag_value(weapon_type2, argument) != NO_FLAG
			       ? flag_value(weapon_type2, argument) : 0);
	    break;
	}
	break;

    case ITEM_PORTAL:
	switch (value_num)
	{
	default:
	    do_help(ch, "ITEM_PORTAL");
	    return FALSE;

	case 0:
	    send_to_char("CHARGES SET.\n\r\n\r", ch);
	    pObj->value[0] = atoi (argument);
	    break;
	case 1:
	    send_to_char("EXIT FLAGS SET.\n\r\n\r", ch);
	    pObj->value[1] ^= (flag_value(exit_flags, argument) != NO_FLAG
			       ? flag_value(exit_flags, argument) : 0);
	    break;
	case 2:
	    send_to_char("PORTAL FLAGS TOGGLED.\n\r\n\r", ch);
	    pObj->value[2] ^= (flag_value(portal_flags, argument) != NO_FLAG
			       ? flag_value(portal_flags, argument) : 0);
	    break;
	case 3:
	    send_to_char("EXIT VNUM SET.\n\r\n\r", ch);
	    pObj->value[3] = atoi (argument);
	    break;
	case 4:
	    send_to_char("EXIT KEY SET.\n\r\n\r", ch);
	    pObj->value[4] = atoi (argument);
	    break;

	}
	break;

    case ITEM_FURNITURE:
	switch (value_num)
	{
	default:
	    do_help(ch, "ITEM_FURNITURE");
	    return FALSE;

	case 0:
	    send_to_char("NUMBER OF PEOPLE SET.\n\r\n\r", ch);
	    pObj->value[0] = atoi (argument);
	    break;
	case 1:
	    send_to_char("MAX WEIGHT SET.\n\r\n\r", ch);
	    pObj->value[1] = atoi (argument);
	    break;
	case 2:
	    send_to_char("FURNITURE FLAGS TOGGLED.\n\r\n\r", ch);
	    pObj->value[2] ^= (flag_value(furniture_flags, argument) != NO_FLAG
			       ? flag_value(furniture_flags, argument) : 0);
	    break;
	case 3:
	    send_to_char("HEAL BONUS SET.\n\r\n\r", ch);
	    pObj->value[3] = atoi (argument);
	    break;
	case 4:
	    send_to_char("MANA BONUS SET.\n\r\n\r", ch);
	    pObj->value[4] = atoi (argument);
	    break;
	}
	break;

    case ITEM_CONTAINER:
	switch (value_num)
	{
	    int value;

	default:
	    do_help(ch, "ITEM_CONTAINER");
	    return FALSE;
	case 0:
	    send_to_char("WEIGHT CAPACITY SET.\n\r\n\r", ch);
	    pObj->value[0] = atoi(argument);
	    break;
	case 1:
	    if ((value = flag_value(chest_flags, argument))
		!= NO_FLAG)
		TOGGLE_BIT(pObj->value[1], value);
	    else
	    {
		do_help (ch, "ITEM_CONTAINER");
		return FALSE;
	    }
	    send_to_char("CONTAINER TYPE SET.\n\r\n\r", ch);
	    break;
	case 2:
	    if (atoi(argument) != 0)
	    {
		if (!get_obj_index(atoi(argument)))
		{
		    send_to_char("THERE IS NO SUCH ITEM.\n\r\n\r", ch);
		    return FALSE;
		}

		if (get_obj_index(atoi(argument))->item_type != ITEM_KEY)
		{
		    send_to_char("THAT ITEM IS NOT A KEY.\n\r\n\r", ch);
		    return FALSE;
		}
	    }
	    send_to_char("CONTAINER KEY SET.\n\r\n\r", ch);
	    pObj->value[2] = atoi(argument);
	    break;
	case 3:
	    send_to_char("CONTAINER MAX WEIGHT SET.\n\r", ch);
	    pObj->value[3] = atoi(argument);
	    break;
	case 4:
	    send_to_char("WEIGHT MULTIPLIER SET.\n\r\n\r", ch);
	    pObj->value[4] = atoi (argument);
	    break;
	}
	break;

    case ITEM_CHEST:
	switch (value_num)
	{
	    int value;

	default:
	    do_help(ch, "ITEM_CHEST");
	    return FALSE;
	case 0:
	    send_to_char("WEIGHT CAPACITY SET.\n\r\n\r", ch);
	    pObj->value[0] = atoi(argument);
	    break;
	case 1:
	    if ((value = flag_value(chest_flags, argument))
		!= NO_FLAG)
		TOGGLE_BIT(pObj->value[1], value);
	    else
	    {
		do_help (ch, "ITEM_CHEST");
		return FALSE;
	    }
	    send_to_char("CHEST TYPE SET.\n\r\n\r", ch);
	    break;
	case 2:
	    if (atoi(argument) != 0)
	    {
		if (!get_obj_index(atoi(argument)))
		{
		    send_to_char("THERE IS NO SUCH ITEM.\n\r\n\r", ch);
		    return FALSE;
		}

		if (get_obj_index(atoi(argument))->item_type != ITEM_KEY)
		{
		    send_to_char("THAT ITEM IS NOT A KEY.\n\r\n\r", ch);
		    return FALSE;
		}
	    }
	    send_to_char("CHEST KEY SET.\n\r\n\r", ch);
	    pObj->value[2] = atoi(argument);
	    break;
	case 3:
	    send_to_char("CHEST MAX WEIGHT SET.\n\r", ch);
	    pObj->value[3] = atoi(argument);
	    break;
	case 4:
	    send_to_char("WEIGHT MULTIPLIER SET.\n\r\n\r", ch);
	    pObj->value[4] = atoi (argument);
	    break;
	}
	break;

    case ITEM_DRINK_CON:
	switch (value_num)
	{
	default:
	    do_help(ch, "ITEM_DRINK");
	    /* OLC		    do_help(ch, "liquids");    */
	    return FALSE;
	case 0:
	    send_to_char("MAXIMUM AMOUT OF LIQUID HOURS SET.\n\r\n\r", ch);
	    pObj->value[0] = atoi(argument);
	    break;
	case 1:
	    send_to_char("CURRENT AMOUNT OF LIQUID HOURS SET.\n\r\n\r", ch);
	    pObj->value[1] = atoi(argument);
	    break;
	case 2:
	    send_to_char("LIQUID TYPE SET.\n\r\n\r", ch);
	    pObj->value[2] = (liq_lookup(argument) != -1 ?
			      liq_lookup(argument) : 0);
	    break;
	case 3:
	    send_to_char("POISON VALUE SET.\n\r\n\r", ch);
	    pObj->value[3] = atoi(argument);
	    break;
	}
	break;

    case ITEM_ROD:
	switch (value_num)
	{
	default:
	    do_help(ch, "ITEM_ROD");
	    /* OLC		    do_help(ch, "rods");    */
	    return FALSE;
	case 0:
	    send_to_char("LEVEL OF ROD IS SET.\n\r\n\r", ch);
	    pObj->value[0] = atoi(argument);
	    break;
	case 1:
	    send_to_char("SPELL IS SET.\n\r\n\r", ch);
	    pObj->value[1] = skill_lookup(argument);
	    break;
	case 2:
	    send_to_char("RECHARGE TIME IS SET.\n\r\n\r", ch);
	    pObj->value[2] = atoi(argument);
	    break;
	}
	break;

    case ITEM_TRAP:
	switch (value_num)
	{
	default:
	    do_help(ch, "ITEM_TRAP");
	    return FALSE;
	case 0:
	    send_to_char("NUMBER OF DICE IS SET.\n\r\n\r", ch);
	    pObj->value[0] = atoi(argument);
	    break;
	case 1:
	    send_to_char("TYPE OF DICE IS SET.\n\r\n\r", ch);
	    pObj->value[1] = atoi(argument);
	    break;
	case 3:
	    send_to_char("DAMAGE TYPE IS SET.\n\r\n\r", ch);
	    pObj->value[3] = flag_lookup(argument, dam_flags);
	    break;
	case 4:
	    pObj->value[4] = atoi(argument) * PULSE_VIOLENCE;
	    if (pObj->value[4] < 1 * PULSE_VIOLENCE
		|| pObj->value[4] > 10 * PULSE_VIOLENCE)
	    {
		send_to_char("IMPOSSIBLE!!! DELAY IS SET TO 3.\n\r\n\r",
			     ch);
		pObj->value[4] = 3 * PULSE_VIOLENCE;
	    }
	    else
		send_to_char("DELAY IS SET.\n\r\n\r", ch);
	    break;
	}
	break;

    case ITEM_FOUNTAIN:
	switch (value_num)
	{
	default:
	    do_help(ch, "ITEM_FOUNTAIN");
	    /* OLC		    do_help(ch, "liquids");    */
	    return FALSE;
	case 0:
	    send_to_char("MAXIMUM AMOUT OF LIQUID HOURS SET.\n\r\n\r", ch);
	    pObj->value[0] = atoi(argument);
	    break;
	case 1:
	    send_to_char("CURRENT AMOUNT OF LIQUID HOURS SET.\n\r\n\r", ch);
	    pObj->value[1] = atoi(argument);
	    break;
	case 2:
	    send_to_char("LIQUID TYPE SET.\n\r\n\r", ch);
	    pObj->value[2] = (liq_lookup(argument) != -1 ?
			      liq_lookup(argument) : 0);
	    break;
	}
	break;

    case ITEM_FOOD:
	switch (value_num)
	{
	default:
	    do_help(ch, "ITEM_FOOD");
	    return FALSE;
	case 0:
	    send_to_char("HOURS OF FOOD SET.\n\r\n\r", ch);
	    pObj->value[0] = atoi(argument);
	    break;
	case 1:
	    send_to_char("HOURS OF FULL SET.\n\r\n\r", ch);
	    pObj->value[1] = atoi(argument);
	    break;
	case 3:
	    send_to_char("POISON VALUE SET.\n\r\n\r", ch);
	    pObj->value[3] = atoi(argument);
	    break;
	}
	break;

    case ITEM_MONEY:
	switch (value_num)
	{
	default:
	    do_help(ch, "ITEM_MONEY");
	    return FALSE;
	case 0:
	    send_to_char("GOLD AMOUNT SET.\n\r\n\r", ch);
	    pObj->value[0] = atoi(argument);
	    break;
	case 1:
	    send_to_char("SILVER AMOUNT SET.\n\r\n\r", ch);
	    pObj->value[1] = atoi(argument);
	    break;
	}
	break;
    case ITEM_ARTIFACT:
	switch (value_num)
	{
	default:
	    do_help(ch, "ITEM_ARTIFACT");
	    return FALSE;
	case 0:
	    send_to_char("Chance SET.\n\r\n\r", ch);
	    pObj->value[0] = atoi(argument);
	    break;
	}
	break;
    case ITEM_MAP:
	switch (value_num)
	{
	default:
	    do_help(ch, "ITEM_MAP");
	    return FALSE;
	case 1:
	    send_to_char("Recite zone SET.\n\r\n\r", ch);
	    pObj->value[1] = atoi(argument);
	    break;
	}
	break;

    case ITEM_MORTAR:
	switch (value_num)
	{
	    int value;
	default:
	    do_help(ch, "ITEM_MORTAR");
	    return FALSE;
	case 0:
	    send_to_char("WEIGHT CAPACITY SET.\n\r\n\r", ch);
	    pObj->value[0] = atoi(argument);
	    break;
	case 1:
	    if ((value = flag_value(chest_flags, argument))
		!= NO_FLAG)
		TOGGLE_BIT(pObj->value[1], value);
	    else
	    {
		do_help (ch, "ITEM_MORTAR");
		return FALSE;
	    }
	    send_to_char("MORTAR TYPE SET.\n\r\n\r", ch);
	    break;
	case 2:
	    send_to_char("MAX NUMBER SET.\n\r\n\r", ch);
	    pObj->value[2] = atoi(argument);
	    break;
	case 3:
	    send_to_char("MORTAR MAX WEIGHT SET.\n\r\n\r", ch);
	    pObj->value[3] = atoi(argument);
	    break;
	case 4:
	    send_to_char("WEIGHT MULTIPLIER SET.\n\r\n\r", ch);
	    pObj->value[4] = atoi (argument);
	    break;
	}
	break;

    case ITEM_INGREDIENT:
	switch (value_num)
	{
	    int value;
	    char time1[MIL], time2[MIL];

	default:
	    do_help(ch, "ITEM_INGREDIENT");
	    return FALSE;
	case 0:    

	    argument = one_argument(argument, time1);
	    one_argument(argument, time2);

	    if (time1[0] == '\0' || time2[0] =='\0' 
	        || atoi(time1) < 0 || atoi(time1) > 24
	        || atoi(time2) < 0 || atoi(time2) > 24)
	    {
		do_help (ch, "ITEM_INGREDIENT");
		return FALSE;
	    }
	    else
	    {
		pObj->value[0] = atoi(time1)*100+atoi(time2);
	    }		
	    send_to_char("SEARCH TIME.\n\r\n\r", ch);
	    break;
	case 1:
	    if ((value = flag_value(ing_sector_flags, argument)) != NO_FLAG)
		TOGGLE_BIT(pObj->value[1], value);
	    else
	    {
		do_help (ch, "ITEM_INGREDIENT");
		return FALSE;
	    }
	    send_to_char("SEARCH AREA.\n\r\n\r", ch);
	    break;
	case 2:
	    if (!IS_NULLSTR(argument) && (race_lookup(argument) != -1))
	    {
		pObj->value[2] = race_lookup(argument);
	    }
	    else
	    {
		do_help (ch, "ITEM_INGREDIENT");
		return FALSE;
	    }
	    send_to_char("SEARCH RACE.\n\r\n\r", ch);
	    break;
	case 3:
	    if ((value = flag_value(ing_weather_flags, argument)) != NO_FLAG)
		TOGGLE_BIT(pObj->value[3], value);
	    else
	    {
		do_help (ch, "ITEM_INGREDIENT");
		return FALSE;
	    }
	    send_to_char("SEARCH WEATHER.\n\r", ch);
	    break;
	case 4:
	    send_to_char("CHANCE FIND.\n\r", ch);
	    pObj->value[4] = atoi(argument);
	    break;
	}
	break;

    }

    show_obj_values(ch, pObj);

    return TRUE;
}



OEDIT(oedit_show)
{
    OBJ_INDEX_DATA *pObj;
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    AFFECT_DATA *paf;

    int cnt;

    EDIT_OBJ(ch, pObj);

    sprintf(buf, "Name:          [%s]\n\r"
	    "Area:          [%5d] %s\n\r",
	    pObj->name,
	    !pObj->area ? -1        : pObj->area->vnum,
	    !pObj->area ? "No Area" : pObj->area->name);
    send_to_char(buf, ch);


    sprintf(buf, "Vnum:          [%5d]\n\r"
	    "Type:          [%s]\n\r",
	    pObj->vnum,
	    flag_string(type_flags, pObj->item_type, FALSE));
    send_to_char(buf, ch);

    sprintf(buf, "Level:         [%5d]\n\r", pObj->level);
    send_to_char(buf, ch);

    sprintf(buf, "Wear flags:    [%s]\n\r",
	    flag_string(wear_flags, pObj->wear_flags, FALSE));
    send_to_char(buf, ch);

    sprintf(buf, "Extra flags:   [%s]\n\r",
	    flag_string(extra_flags, pObj->extra_flags, FALSE));
    send_to_char(buf, ch);

    sprintf(buf, "Uncomfortable: [%s]\n\r",
	    flag_string(comf_flags, pObj->uncomf, FALSE));
    send_to_char(buf, ch);

    sprintf(buf, "Unusable:      [%s]\n\r",
	    flag_string(comf_flags, pObj->unusable, FALSE));
    send_to_char(buf, ch);

    sprintf(buf, "Material:      [%s]\n\r",                /* ROM */
	    pObj->material);
    send_to_char(buf, ch);

    sprintf(buf, "Condition:     [%5d]\n\r"
                 "Timer:         [%5d]\n\r",               /* ROM */
	    pObj->condition, pObj->timer);
    send_to_char(buf, ch);

    sprintf(buf, "Weight:        [%5d]\n\r"
	    "Cost:          [%5d]\n\r",
	    pObj->weight, pObj->cost);
    send_to_char(buf, ch);

    for (cnt = 0; cnt < MAX_STATS; cnt++)
	if (pObj->require[cnt] > 0)
	{
	    sprintf(buf, "Require:     [%s:%d]\n\r", flag_string(attr_flags, cnt, FALSE), pObj->require[cnt]);
	    send_to_char(buf, ch);
	}

    sprintf(buf, "Comments:      [%s]\n\r"
	    "Last Edited:   [%s]\n\r",
	    pObj->comments, pObj->last_edited);
    send_to_char(buf, ch);

    if (pObj->extra_descr)
    {
	EXTRA_DESCR_DATA *ed;

	send_to_char("Ex desc kwd:   ", ch);

	for (ed = pObj->extra_descr; ed; ed = ed->next)
	{
	    send_to_char("[", ch);
	    send_to_char(ed->keyword, ch);
	    send_to_char("]", ch);
	}

	send_to_char("\n\r", ch);
    }

    sprintf(buf, "Short desc:  %s (%s)\n\rLong desc:\n\r     %s\n\r",
	    pObj->short_descr, color_to_str(pObj->short_descr), pObj->description);
    send_to_char(buf, ch);

    for (cnt = 0, paf = pObj->affected; paf; paf = paf->next)
    {
	if (cnt == 0)
	{
	    send_to_char("Number Modifier Affects\n\r", ch);
	    send_to_char("------ -------- -------\n\r", ch);
	}

	if (paf->location == APPLY_SKILL
	    && paf->type > 0
	    && paf->type < max_skills)
	{
	    sprintf(buf, "[%4d] %-8d %-12s %s{x\n\r",
		    cnt,
		    paf->modifier,
		    flag_string(apply_flags, paf->location, FALSE),
		    skill_table[paf->type].name);
	}
	else if (paf->bitvector)
	{
	    switch (paf->where)
	    {
	    case TO_AFFECTS:
		strcpy(buf2, flag_string(affect_flags, paf->bitvector, FALSE));
		sprintf(buf, "[%4d] %-8d %-12s, affect '%s'\n\r",
			cnt,
			paf->modifier,
			flag_string(apply_flags, paf->location, FALSE),
			buf2);
		break;
	    case TO_WEAPON:
		strcpy(buf2, flag_string(weapon_type2, paf->bitvector, FALSE));
		sprintf(buf, "[%4d] %-8d %-12s, weapon flags '%s'\n\r",
			cnt,
			paf->modifier,
			flag_string(apply_flags, paf->location, FALSE),
			buf2);
		break;
	    case TO_OBJECT:
		strcpy(buf2, flag_string(extra_flags, paf->bitvector, FALSE));
		sprintf(buf, "[%4d] %-8d %-12s, object flags '%s'\n\r",
			cnt,
			paf->modifier,
			flag_string(apply_flags, paf->location, FALSE),
			buf2);
		break;
	    case TO_RESIST:
		sprintf(buf, "[%4d] modifies resistance to %s by %d\n\r",
			cnt,
			flag_string(dam_flags, paf->bitvector, FALSE),
			paf->modifier);
		break;
	    default:
		break;
	    }
	}
	else
	{
	    sprintf(buf, "[%4d] %-8d %s\n\r", cnt,
		    paf->modifier,
		    flag_string(apply_flags, paf->location, FALSE));
	}

	send_to_char(buf, ch);
	cnt++;
    }

    show_obj_values(ch, pObj);
    if (pObj->progs)
    {
	sprintf(buf, "\n\rOBJPrograms for [%5d]:\n\r", pObj->vnum);
	send_to_char(buf, ch);

	show_triggers(ch, pObj->progs);
    }

    for (cnt = 0; limits[cnt] > 0 && cnt < MAX_LIMITS; cnt += 3)
	if (pObj->vnum == limits[cnt])
	{
	    sprintf(buf, "\n\rЭта вещь является лимитной: %2d (%2d)\n\r",
		    limits[cnt + 1], limits[cnt + 2]);
	    send_to_char(buf, ch);
	}

    return FALSE;
}


/*
 * Need to issue warning if flag isn't valid. -- does so now -- Hugin.
 */
OEDIT(oedit_addaffect)
{
    int64_t value;
    OBJ_INDEX_DATA *pObj;
    AFFECT_DATA *pAf;
    char loc[MAX_STRING_LENGTH];
    char mod[MAX_STRING_LENGTH];
    int skill = -1;

    EDIT_OBJ(ch, pObj);

    argument = one_argument(argument, loc);
    argument = one_argument(argument, mod);

    if (loc[0] == '\0' || mod[0] == '\0' || !is_number(mod))
    {
	send_to_char("Syntax:  addaffect [location] [#xmod]\n\r", ch);
	return FALSE;
    }

    if ((value = flag_value(apply_flags, loc)) == NO_FLAG) /* Hugin */
    {
	send_to_char("Valid affects are:\n\r", ch);
	show_help(ch, "apply");
	return FALSE;
    }

    if (value == APPLY_SKILL &&
	(IS_NULLSTR (argument) ||
	 (skill = skill_lookup (argument)) < 0 ||
	 skill >= max_skills))
    {
	send_to_char ("Syntax: addaffect skill <mod> <skill name>\n\r", ch);
	return FALSE;
    }

    pAf             =   new_affect();
    pAf->location   =   value;
    pAf->modifier   =   atoi(mod);
    pAf->where	    =   TO_OBJECT;
    pAf->type       =   skill;
    pAf->duration   =   -1;
    pAf->bitvector  =   0;
    pAf->level      =	pObj->level;
    pAf->next       =   pObj->affected;
    pAf->prev       =	NULL;
    if (pObj->affected)
	pObj->affected->prev = pAf;
    pObj->affected  =   pAf;

    send_to_char("Affect added.\n\r", ch);

    return TRUE;
}

OEDIT(oedit_addapply)
{
    int64_t value,bv = 0,typ, sn = -1;
    OBJ_INDEX_DATA *pObj;
    AFFECT_DATA *pAf;
    char loc[MAX_STRING_LENGTH];
    char mod[MAX_STRING_LENGTH];
    char type[MAX_STRING_LENGTH];
    char bvector[MAX_STRING_LENGTH];

    EDIT_OBJ(ch, pObj);

    argument = one_argument(argument, type);
    argument = one_argument(argument, loc);
    argument = one_argument(argument, mod);
    argument = one_argument(argument, bvector);

    if (type[0] == '\0'
	|| (typ = flag_value(apply_types, type)) == NO_FLAG)
    {
	send_to_char("Invalid apply type. Valid apply types are:\n\r", ch);
	show_help(ch, "apptype");
	return FALSE;
    }

    if (loc[0] == '\0' || (value = flag_value(apply_flags, loc)) == NO_FLAG)
    {
	send_to_char("Valid applys are:\n\r", ch);
	show_help(ch, "apply");
	return FALSE;
    }

    if (is_number(bvector))
	bv = atoi(bvector);

    if (!is_number(bvector) && (bv = flag_value(bitvector_type[typ].table, bvector)) == NO_FLAG)
    {
	send_to_char("Invalid bitvector type.\n\r", ch);
	send_to_char("Valid bitvector types are:\n\r", ch);
	show_help(ch, bitvector_type[typ].help);
	return FALSE;
    }

    if (mod[0] == '\0' || !is_number(mod))
    {
	send_to_char("Syntax: addapply <type> <location> <mod> <bitvector> [<skill>]\n\r",
		     ch);
	return FALSE;
    }

    if (argument[0] != '\0' && (sn = skill_lookup(argument)) == -1)
    {
	send_to_char("Invalid skill.\n\r", ch);
	return FALSE;
    }

    pAf             =   new_affect();
    pAf->location   =   value;
    pAf->modifier   =   atoi(mod);
    pAf->where	    =   apply_types[typ].bit;
    pAf->type	    =	sn;
    pAf->duration   =   -1;
    pAf->bitvector  =   bv;
    pAf->level      =	pObj->level;
    pAf->next       =   pObj->affected;
    pAf->prev       =	NULL;
    if (pObj->affected)
	pObj->affected->prev = pAf;
    pObj->affected  =   pAf;

    send_to_char("Apply added.\n\r", ch);
    return TRUE;
}

/*
 * My thanks to Hans Hvidsten Birkeland and Noam Krendel(Walker)
 * for really teaching me how to manipulate pointers.
 */
OEDIT(oedit_delaffect)
{
    OBJ_INDEX_DATA *pObj;
    AFFECT_DATA *pAf;
    AFFECT_DATA *pAf_next;
    char affect[MAX_STRING_LENGTH];
    int value;

    EDIT_OBJ(ch, pObj);

    one_argument(argument, affect);

    if (!is_number(affect) || affect[0] == '\0')
    {
	send_to_char("Syntax:  delaffect [#xaffect]\n\r", ch);
	return FALSE;
    }

    value = atoi(affect);

    if (value < 0)
    {
	send_to_char("Only non-negative affect-numbers allowed.\n\r", ch);
	return FALSE;
    }

    if (!(pAf = pObj->affected))
    {
	send_to_char("OEdit:  Non-existant affect.\n\r", ch);
	return FALSE;
    }

    if (value == 0)	/* First case: Remove first affect */
    {
	pAf = pObj->affected;
	pObj->affected = pAf->next;
	if (pObj->affected)
	    pObj->affected->prev = NULL;
	free_affect(pAf);
    }
    else		/* Affect to remove is not the first */
    {
	int  cnt = 0;

	while ((pAf_next = pAf->next) && (++cnt < value))
	    pAf = pAf_next;

	if (pAf_next)		/* See if it's the next affect */
	{
	    pAf->next = pAf_next->next;

	    if (pAf_next->next)
		pAf_next->next->prev = pAf;

	    free_affect(pAf_next);
	}
	else                                 /* Doesn't exist */
	{
	    send_to_char("No such affect.\n\r", ch);
	    return FALSE;
	}
    }

    send_to_char("Affect removed.\n\r", ch);
    return TRUE;
}



OEDIT(oedit_name)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax:  name [string]\n\r", ch);
	return FALSE;
    }

    free_string(pObj->name);
    pObj->name = str_dup(argument);

    send_to_char("Name set.\n\r", ch);
    return TRUE;
}



OEDIT(oedit_short)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax:  short [string]\n\r", ch);
	return FALSE;
    }

    free_string(pObj->short_descr);
    pObj->short_descr = str_dup(argument);
    pObj->short_descr[0] = LOWER(pObj->short_descr[0]);

    send_to_char("Short description set.\n\r\n\r", ch);
    do_function(ch, &do_cases, pObj->short_descr);
    return TRUE;
}



OEDIT(oedit_long)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax:  long [string]\n\r", ch);
	return FALSE;
    }

    free_string(pObj->description);
    pObj->description = str_dup(argument);
    pObj->description[0] = UPPER(pObj->description[0]);

    send_to_char("Long description set.\n\r", ch);
    return TRUE;
}



bool set_value(CHAR_DATA *ch, OBJ_INDEX_DATA *pObj, char *argument, int value)
{
    if (argument[0] == '\0')
    {
	set_obj_values(ch, pObj, -1, "");     /* '\0' changed to "" -- Hugin */
	return FALSE;
    }

    if (set_obj_values(ch, pObj, value, argument))
	return TRUE;

    return FALSE;
}



/*****************************************************************************
Name:		oedit_values
Purpose:	Finds the object and sets its value.
Called by:	The four valueX functions below. (now five -- Hugin)
 ****************************************************************************/
bool oedit_values(CHAR_DATA *ch, char *argument, int value)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if (set_value(ch, pObj, argument, value))
	return TRUE;

    return FALSE;
}


OEDIT(oedit_value0)
{
    if (oedit_values(ch, argument, 0))
	return TRUE;

    return FALSE;
}



OEDIT(oedit_value1)
{
    if (oedit_values(ch, argument, 1))
	return TRUE;

    return FALSE;
}



OEDIT(oedit_value2)
{
    if (oedit_values(ch, argument, 2))
	return TRUE;

    return FALSE;
}



OEDIT(oedit_value3)
{
    if (oedit_values(ch, argument, 3))
	return TRUE;

    return FALSE;
}



OEDIT(oedit_value4)
{
    if (oedit_values(ch, argument, 4))
	return TRUE;

    return FALSE;
}



OEDIT(oedit_weight)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if (argument[0] == '\0' || !is_number(argument))
    {
	send_to_char("Syntax:  weight [number]\n\r", ch);
	return FALSE;
    }

    pObj->weight = atoi(argument);

    send_to_char("Weight set.\n\r", ch);
    return TRUE;
}

OEDIT(oedit_cost)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if (argument[0] == '\0' || !is_number(argument))
    {
	send_to_char("Syntax:  cost [number]\n\r", ch);
	return FALSE;
    }

    pObj->cost = atoi(argument);

    send_to_char("Cost set.\n\r", ch);
    return TRUE;
}



OEDIT(oedit_create)
{
    OBJ_INDEX_DATA *pObj;
    AREA_DATA *pArea;
    int  value;
    int  iHash;

    value = atoi(argument);
    if (argument[0] == '\0' || value == 0)
    {
	send_to_char("Syntax:  oedit create [vnum]\n\r", ch);
	return FALSE;
    }

    pArea = get_vnum_area(value);
    if (!pArea)
    {
	send_to_char("OEdit:  That vnum is not assigned an area.\n\r", ch);
	return FALSE;
    }

    if (!IS_BUILDER(ch, pArea))
    {
	send_to_char("OEdit:  Vnum in an area you cannot build in.\n\r", ch);
	return FALSE;
    }

    if (get_obj_index(value))
    {
	send_to_char("OEdit:  Object vnum already exists.\n\r", ch);
	return FALSE;
    }

    pObj			= new_obj_index();
    pObj->vnum			= value;
    pObj->area			= pArea;

    if (value > top_vnum_obj)
	top_vnum_obj = value;

    iHash			= value % MAX_KEY_HASH;
    pObj->next			= obj_index_hash[iHash];
    obj_index_hash[iHash]	= pObj;
    ch->desc->pEdit		= (void *)pObj;

    send_to_char("Object Created.\n\r", ch);
    return TRUE;
}

OEDIT(oedit_clone)
{
    OBJ_INDEX_DATA *pObj;
    OBJ_INDEX_DATA *orig;
    AREA_DATA *pArea;
    EXTRA_DESCR_DATA *extra;
    EXTRA_DESCR_DATA *clone_extra;
    AFFECT_DATA *af;
    AFFECT_DATA *clone_af;
    char arg[MAX_STRING_LENGTH];
    int  value;
    int  clone;
    int  iHash;
    int  i;

    argument = one_argument(argument, arg);
    
    value = atoi(arg);
    clone = atoi(argument);
    if (IS_NULLSTR(arg) || IS_NULLSTR(argument)
	|| value == 0 || clone == 0)
    {
	send_to_char("Syntax:  oedit clone <original obj vnum> <clone vnum>\n\r", ch);
	return FALSE;
    }

    pArea = get_vnum_area(clone);
    if (!pArea)
    {
	send_to_char("OEdit:  That vnum is not assigned an area.\n\r", ch);
	return FALSE;
    }

    if (!IS_BUILDER(ch, pArea))
    {
	send_to_char("OEdit:  Vnum in an area you cannot build in.\n\r", ch);
	return FALSE;
    }

    if (get_obj_index(clone))
    {
	send_to_char("OEdit:  Clone vnum already exists.\n\r", ch);
	return FALSE;
    }

    if ((orig = get_obj_index(value)) == NULL)
    {
	send_to_char("OEdit:  Can't find orig object.\n\r", ch);
	return FALSE;
    }

    pObj = new_obj_index();
    
    /* Clone object */
    free_string(pObj->name);
    free_string(pObj->short_descr);
    free_string(pObj->description);
    free_string(pObj->material);
    pObj->name        = str_dup(orig->name);
    pObj->short_descr = str_dup(orig->short_descr);
    pObj->description = str_dup(orig->description);
    pObj->material    = str_dup(orig->material);

    pObj->item_type   = orig->item_type;
    pObj->extra_flags = orig->extra_flags;
    pObj->uncomf      = orig->uncomf;
    pObj->unusable    = orig->unusable;
    pObj->wear_flags  = orig->wear_flags;
    pObj->level       = orig->level;
    pObj->condition   = orig->condition;
    pObj->weight      = orig->weight;
    pObj->cost        = orig->cost;

    for (i = 0; i < 5; i++)
	pObj->value[i] = orig->value[i];

    for (i = 0; i < MAX_STATS; i++)
	pObj->require[i] = orig->require[i];
    
    /* Clone extra descriptions */
    for (extra = orig->extra_descr; extra; extra = extra->next)
    {
	clone_extra = new_extra_descr();
	free_string(clone_extra->keyword);
	free_string(clone_extra->description);
	clone_extra->keyword     = str_dup(extra->keyword);
	clone_extra->description = str_dup(extra->description);

	clone_extra->next = pObj->extra_descr;
	pObj->extra_descr = clone_extra;
    }

    /* Clone affects */
    for (af = orig->affected; af; af = af->next)
    {
	clone_af = new_affect();
	clone_af->where     = af->where;
	clone_af->type      = af->type;
	clone_af->level     = af->level;
	clone_af->duration  = af->duration;
	clone_af->location  = af->location;
	clone_af->modifier  = af->modifier;
	clone_af->bitvector = af->bitvector;

	clone_af->next = pObj->affected;
	clone_af->prev = NULL;
	if (pObj->affected)
	    pObj->affected->prev = clone_af;
	pObj->affected = clone_af;
    }
    
    pObj->vnum = clone;
    pObj->area = pArea;

    snprintf(arg, sizeof(arg), "Cloned from %d", value);
    pObj->comments = str_dup(arg);

    if (clone > top_vnum_obj)
	top_vnum_obj = value;

    iHash			= clone % MAX_KEY_HASH;
    pObj->next			= obj_index_hash[iHash];
    obj_index_hash[iHash]	= pObj;
    ch->desc->pEdit		= (void *)pObj;

    send_to_char("Object Cloned.\n\r", ch);
    return TRUE;
}



OEDIT(oedit_ed)
{
    OBJ_INDEX_DATA *pObj;
    EXTRA_DESCR_DATA *ed;
    char command[MAX_INPUT_LENGTH];
    char keyword[MAX_INPUT_LENGTH];

    EDIT_OBJ(ch, pObj);

    argument = one_argument(argument, command);
    one_argument(argument, keyword);

    if (command[0] == '\0')
    {
	send_to_char("Syntax:  ed add [keyword]\n\r", ch);
	send_to_char("         ed delete [keyword]\n\r", ch);
	send_to_char("         ed edit [keyword]\n\r", ch);
	send_to_char("         ed format [keyword]\n\r", ch);
	return FALSE;
    }

    if (!str_cmp(command, "add"))
    {
	if (keyword[0] == '\0')
	{
	    send_to_char("Syntax:  ed add [keyword]\n\r", ch);
	    return FALSE;
	}

	ed                  =   new_extra_descr();
	ed->keyword         =   str_dup(keyword);
	ed->next            =   pObj->extra_descr;
	pObj->extra_descr   =   ed;

	string_append(ch, &ed->description);

	return TRUE;
    }

    if (!str_cmp(command, "edit"))
    {
	if (keyword[0] == '\0')
	{
	    send_to_char("Syntax:  ed edit [keyword]\n\r", ch);
	    return FALSE;
	}

	for (ed = pObj->extra_descr; ed; ed = ed->next)
	{
	    if (is_name(keyword, ed->keyword))
		break;
	}

	if (!ed)
	{
	    send_to_char("OEdit:  Extra description keyword not found.\n\r", ch);
	    return FALSE;
	}

	string_append(ch, &ed->description);

	return TRUE;
    }

    if (!str_cmp(command, "delete"))
    {
	EXTRA_DESCR_DATA *ped = NULL;

	if (keyword[0] == '\0')
	{
	    send_to_char("Syntax:  ed delete [keyword]\n\r", ch);
	    return FALSE;
	}

	for (ed = pObj->extra_descr; ed; ed = ed->next)
	{
	    if (is_name(keyword, ed->keyword))
		break;
	    ped = ed;
	}

	if (!ed)
	{
	    send_to_char("OEdit:  Extra description keyword not found.\n\r", ch);
	    return FALSE;
	}

	if (!ped)
	    pObj->extra_descr = ed->next;
	else
	    ped->next = ed->next;

	free_extra_descr(ed);

	send_to_char("Extra description deleted.\n\r", ch);
	return TRUE;
    }


    if (!str_cmp(command, "format"))
    {
	EXTRA_DESCR_DATA *ped = NULL;

	if (keyword[0] == '\0')
	{
	    send_to_char("Syntax:  ed format [keyword]\n\r", ch);
	    return FALSE;
	}

	for (ed = pObj->extra_descr; ed; ed = ed->next)
	{
	    if (is_name(keyword, ed->keyword))
		break;
	    ped = ed;
	}

	if (!ed)
	{
	    send_to_char("OEdit:  Extra description keyword not found.\n\r", ch);
	    return FALSE;
	}

	ed->description = format_string(ed->description);

	send_to_char("Extra description formatted.\n\r", ch);
	return TRUE;
    }

    oedit_ed(ch, "");
    return FALSE;
}





/* ROM object functions : */

OEDIT(oedit_uncomfortable)
{
    OBJ_INDEX_DATA *pObj;
    int64_t value;

    if (!IS_NULLSTR(argument))
    {
	EDIT_OBJ(ch, pObj);

	if ((value = flag_value(comf_flags, argument)) != NO_FLAG)
	{
	    TOGGLE_BIT(pObj->uncomf, value);
	    send_to_char("Ok.\n\r", ch);
	    return TRUE;
	}
    }

    send_to_char("Syntax: uncomfortable <flag>\n\r", ch);
    return FALSE;
}

OEDIT(oedit_unusable)
{
    OBJ_INDEX_DATA *pObj;
    int64_t value;

    if (!IS_NULLSTR(argument))
    {
	EDIT_OBJ(ch, pObj);

	if ((value = flag_value(comf_flags, argument)) != NO_FLAG)
	{
	    TOGGLE_BIT(pObj->unusable, value);
	    send_to_char("Ok.\n\r", ch);
	    return TRUE;
	}
    }

    send_to_char("Syntax: unusable <flag>\n\r", ch);
    return FALSE;
}

OEDIT(oedit_extra)  /* Moved out of oedit() due to naming conflicts -- Hugin */
{
    OBJ_INDEX_DATA *pObj;
    int64_t value;

    if (argument[0] != '\0')
    {
	EDIT_OBJ(ch, pObj);

	if ((value = flag_value(extra_flags, argument)) != NO_FLAG)
	{
	    TOGGLE_BIT(pObj->extra_flags, value);

	    send_to_char("Extra flag toggled.\n\r", ch);
	    return TRUE;
	}
    }

    send_to_char("Syntax:  extra [flag]\n\r"
		 "Type '? extra' for a list of flags.\n\r", ch);
    return FALSE;
}


OEDIT(oedit_wear)      /* Moved out of oedit() due to naming conflicts -- Hugin */
{
    OBJ_INDEX_DATA *pObj;
    int value;

    if (argument[0] != '\0')
    {
	EDIT_OBJ(ch, pObj);

	if ((value = flag_value(wear_flags, argument)) != NO_FLAG)
	{
	    TOGGLE_BIT(pObj->wear_flags, value);

	    send_to_char("Wear flag toggled.\n\r", ch);
	    return TRUE;
	}
    }

    send_to_char("Syntax:  wear [flag]\n\r"
		 "Type '? wear' for a list of flags.\n\r", ch);
    return FALSE;
}


OEDIT(oedit_type)      /* Moved out of oedit() due to naming conflicts -- Hugin */
{
    OBJ_INDEX_DATA *pObj;
    int value;

    if (argument[0] != '\0')
    {
	EDIT_OBJ(ch, pObj);

	if ((value = flag_value(type_flags, argument)) != NO_FLAG)
	{
	    pObj->item_type = value;

	    send_to_char("Type set.\n\r", ch);

	    /*
	     * Clear the values.
	     */
	    pObj->value[0] = 0;
	    pObj->value[1] = 0;
	    pObj->value[2] = 0;
	    pObj->value[3] = 0;
	    pObj->value[4] = 0;     /* ROM */

	    return TRUE;
	}
    }

    send_to_char("Syntax:  type [flag]\n\r"
		 "Type '? type' for a list of flags.\n\r", ch);
    return FALSE;
}

OEDIT(oedit_material)
{
    OBJ_INDEX_DATA *pObj;
    int i;
    char arg[MAX_INPUT_LENGTH], *new_material;

    EDIT_OBJ(ch, pObj);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax:  material [force] <string>\n\r", ch);
	return FALSE;
    }

    argument = one_argument(argument, arg);

    if (!str_cmp(arg, "force"))
    {
		one_argument(argument, arg);
		new_material = arg;
    }
    else if ((i = material_lookup(arg)) == -1)
    {
	send_to_char("Unknown material. Use ? material.\n\rIf you want to set unlisted material, use material force <string>.\n\r", ch);
	return FALSE;
    }
    else
		new_material = material_table[i].name;

    free_string(pObj->material);
    pObj->material = str_dup(new_material);
    send_to_char("Material set.\n\r", ch);
    return TRUE;
}

OEDIT(oedit_level)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if (argument[0] == '\0' || !is_number(argument))
    {
	send_to_char("Syntax:  level [number]\n\r", ch);
	return FALSE;
    }

    pObj->level = atoi(argument);

    send_to_char("Level set.\n\r", ch);
    return TRUE;
}



OEDIT(oedit_condition)
{
    OBJ_INDEX_DATA *pObj;
    int value;

    if (argument[0] != '\0'
	&& (value = atoi (argument)) >= 0
	&& (value <= 100))
    {
	EDIT_OBJ(ch, pObj);

	pObj->condition = value;
	send_to_char("Condition set.\n\r", ch);

	return TRUE;
    }

    send_to_char("Syntax:  condition [number]\n\r"
		 "Where number can range from 0 (ruined) to 100 (perfect).\n\r",
		 ch);
    return FALSE;
}





/*
 * Mobile Editor Functions.
 */
MEDIT(medit_show)
{
    MOB_INDEX_DATA *pMob;
    char buf[MAX_STRING_LENGTH];
    int i;

    EDIT_MOB(ch, pMob);

    sprintf(buf, "Name:        [%s]\n\rArea:        [%5d] %s\n\r",
	    pMob->player_name,
	    !pMob->area ? -1        : pMob->area->vnum,
	    !pMob->area ? "No Area" : pMob->area->name);
    send_to_char(buf, ch);

    sprintf(buf, "Act:         [%s]\n\r",
	    flag_string(act_flags, pMob->act, FALSE));
    send_to_char(buf, ch);

    sprintf(buf, "Vnum:        [%5d] Sex:   [%s]   Race: [%s]\n\r",
	    pMob->vnum,
	    pMob->sex == SEX_MALE    ? "male   " :
	    pMob->sex == SEX_FEMALE  ? "female " :
	    pMob->sex == SEX_MANY    ? "many " : "neutral",
	    race_table[pMob->race].name);
    send_to_char(buf, ch);

    sprintf(buf,
	    "Level:       [%2d]     Saves: [%4d]    Align: [%4d]      Hitroll: [%2d] Dam Type:    [%s]\n\r",
	    pMob->level, 	pMob->saving_throw,	 pMob->alignment,
	    pMob->hitroll,	attack_table[pMob->dam_type].name);
    send_to_char(buf, ch);

    if (pMob->group)
    {
	sprintf(buf, "Group:       [%5d]\n\r", pMob->group);
	send_to_char(buf, ch);
    }

    sprintf(buf, "Hit dice:    [%2dd%-3d+%4d] ",
	    pMob->hit[DICE_NUMBER],
	    pMob->hit[DICE_TYPE],
	    pMob->hit[DICE_BONUS]);
    send_to_char(buf, ch);

    sprintf(buf, "Damage dice: [%2dd%-3d+%4d] ",
	    pMob->damage[DICE_NUMBER],
	    pMob->damage[DICE_TYPE],
	    pMob->damage[DICE_BONUS]);
    send_to_char(buf, ch);

    sprintf(buf, "Mana dice:   [%2dd%-3d+%4d]\n\r",
	    pMob->mana[DICE_NUMBER],
	    pMob->mana[DICE_TYPE],
	    pMob->mana[DICE_BONUS]);
    send_to_char(buf, ch);

    /* ROM values end */

    sprintf(buf, "Affected by: [%s]\n\r",
	    flag_string(affect_flags, pMob->affected_by, FALSE));
    send_to_char(buf, ch);

    /* ROM values: */

    sprintf(buf, "Armor:       [pierce: %d  bash: %d  slash: %d  other: %d]\n\r",
	    pMob->ac[AC_PIERCE], pMob->ac[AC_BASH],
	    pMob->ac[AC_SLASH], pMob->ac[AC_EXOTIC]);
    send_to_char(buf, ch);

    sprintf(buf, "Form:        [%s]\n\r",
	    flag_string(form_flags, pMob->form, FALSE));
    send_to_char(buf, ch);

    sprintf(buf, "Parts:       [%s]\n\r",
	    flag_string(part_flags, pMob->parts, FALSE));
    send_to_char(buf, ch);

    for (i = 0; i < DAM_MAX; i++)
	if (pMob->resists[i] != 0)
	{
	    sprintf(buf, "Res:         [%-10s: %4d%%]\n\r",
		    flag_string(dam_flags, i, FALSE),
		    pMob->resists[i]);
	    send_to_char(buf, ch);
	}

    sprintf(buf, "Off:         [%s]\n\r",
	    flag_string(off_flags,  pMob->off_flags, FALSE));
    send_to_char(buf, ch);

    sprintf(buf, "Size:        [%s]\n\r",
	    flag_string(size_flags, pMob->size, FALSE));
    send_to_char(buf, ch);

    sprintf(buf, "Material:    [%s]\n\r",
	    pMob->material);
    send_to_char(buf, ch);

    sprintf(buf, "Start pos.   [%s]\n\r",
	    flag_string(position_flags, pMob->start_pos, FALSE));
    send_to_char(buf, ch);

    sprintf(buf, "Default pos  [%s]\n\r",
	    flag_string(position_flags, pMob->default_pos, FALSE));
    send_to_char(buf, ch);

    sprintf(buf, "Wealth:      [%5ld]\n\r",
	    pMob->wealth);
    send_to_char(buf, ch);

    send_to_char("Areal:       [ ", ch);

    for (i = 0; i < SECT_MAX; i++)
	if (pMob->areal[i])
	{
	    sprintf(buf, "%s ", flag_string(sector_flags, i, FALSE));
	    send_to_char(buf, ch);
	}

    send_to_char("]\n\r", ch);

    sprintf(buf, "Arrive msg:  [%s]\n\r", pMob->msg_arrive);
    send_to_char(buf, ch);

    sprintf(buf, "Gone msg:    [%s]\n\r", pMob->msg_gone);
    send_to_char(buf, ch);

    if (pMob->chat_vnum[0])
    {
	send_to_char("Chatbooks:   [ ", ch);
	for(i = 0; pMob->chat_vnum[i] && i < MAX_CHATBOOKS; i++)
	{
	    sprintf(buf, "%d ", pMob->chat_vnum[i]);
	    send_to_char(buf, ch);
	}
	send_to_char("]\n\r", ch);
    }

    /* ROM values end */

    if (pMob->spec_fun)
    {
	sprintf(buf, "Spec fun:    [%s]\n\r",  spec_name(pMob->spec_fun));
	send_to_char(buf, ch);
    }

    if (pMob->clan != 0)
    {
	sprintf(buf, "Clan    :    [%s]\n\r",  clan_table[pMob->clan].name);
	send_to_char(buf, ch);
    }

    if (pMob->rating != 0)
    {
	sprintf(buf, "Rating  :    [%d]\n\r",  pMob->rating);
	send_to_char(buf, ch);
    }

    sprintf(buf, "Short descr: %s (%s)\n\rLong descr:\n\r%s",
	    pMob->short_descr, color_to_str(pMob->short_descr),
	    pMob->long_descr);
    send_to_char(buf, ch);

    sprintf(buf, "Description:\n\r%s", pMob->description);
    send_to_char(buf, ch);

    sprintf(buf, "Comments:     [%s]\n\r"
	    "Last Edited:  [%s]\n\r",
	    pMob->comments, pMob->last_edited);
    send_to_char(buf, ch);

    if (pMob->pShop)
    {
	SHOP_DATA *pShop;
	int iTrade;

	pShop = pMob->pShop;

	sprintf(buf,
		"Shop data for [%5d]:\n\r"
		"  Markup for purchaser: %d%%\n\r"
		"  Markdown for seller:  %d%%\n\r",
		pShop->keeper, pShop->profit_buy, pShop->profit_sell);
	send_to_char(buf, ch);
	sprintf(buf, "  Hours: %d to %d.\n\r",
		pShop->open_hour, pShop->close_hour);
	send_to_char(buf, ch);

	for (iTrade = 0; iTrade < MAX_TRADE; iTrade++)
	{
	    if (pShop->buy_type[iTrade] != 0)
	    {
		if (iTrade == 0) {
		    send_to_char("  Number Trades Type\n\r", ch);
		    send_to_char("  ------ -----------\n\r", ch);
		}
		sprintf(buf, "  [%4d] %s\n\r", iTrade,
			flag_string(type_flags, pShop->buy_type[iTrade], FALSE));
		send_to_char(buf, ch);
	    }
	}
    }

    if (pMob->progs)
    {
	sprintf(buf, "\n\rMOBPrograms for [%5d]:\n\r", pMob->vnum);
	send_to_char(buf, ch);

	show_triggers(ch, pMob->progs);
    }

    return FALSE;
}



MEDIT(medit_create)
{
    MOB_INDEX_DATA *pMob;
    AREA_DATA *pArea;
    int  value;
    int  iHash, i;

    value = atoi(argument);
    if (argument[0] == '\0' || value == 0)
    {
	send_to_char("Syntax:  medit create [vnum]\n\r", ch);
	return FALSE;
    }

    pArea = get_vnum_area(value);

    if (!pArea)
    {
	send_to_char("MEdit:  That vnum is not assigned an area.\n\r", ch);
	return FALSE;
    }

    if (!IS_BUILDER(ch, pArea))
    {
	send_to_char("MEdit:  Vnum in an area you cannot build in.\n\r", ch);
	return FALSE;
    }

    if (get_mob_index(value))
    {
	send_to_char("MEdit:  Mobile vnum already exists.\n\r", ch);
	return FALSE;
    }

    pMob			= new_mob_index();
    pMob->vnum			= value;
    pMob->area			= pArea;

    if (value > top_vnum_mob)
	top_vnum_mob = value;

    for (i = 0; i < SECT_MAX; i++)
	pMob->areal[i] = TRUE;

    pMob->act			= ACT_IS_NPC;
    iHash			= value % MAX_KEY_HASH;
    pMob->next			= mob_index_hash[iHash];
    mob_index_hash[iHash]	= pMob;
    ch->desc->pEdit		= (void *)pMob;

    send_to_char("Mobile Created.\n\r", ch);
    return TRUE;
}

MEDIT(medit_clone)
{
    MOB_INDEX_DATA *pMob;
    MOB_INDEX_DATA *orig;
    AREA_DATA *pArea;
    char arg[MAX_STRING_LENGTH];
    int  value;
    int  clone;
    int  iHash, i;

    argument = one_argument(argument, arg);
    
    clone = atoi(argument);
    value = atoi(arg);
    if (IS_NULLSTR(argument) || IS_NULLSTR(arg)
	|| clone == 0 || value == 0)
    {
	send_to_char("Syntax:  medit clone <orig vnum> <clone vnum>\n\r", ch);
	return FALSE;
    }

    pArea = get_vnum_area(clone);

    if (!pArea)
    {
	send_to_char("MEdit:  That vnum is not assigned an area.\n\r", ch);
	return FALSE;
    }

    if (!IS_BUILDER(ch, pArea))
    {
	send_to_char("MEdit:  Vnum in an area you cannot build in.\n\r", ch);
	return FALSE;
    }

    if (get_mob_index(clone))
    {
	send_to_char("MEdit:  Mobile vnum already exists.\n\r", ch);
	return FALSE;
    }

    if ((orig = get_mob_index(value)) == NULL)
    {
	send_to_char("Medit: can't find orig mobile.\n\r", ch);
	return FALSE;
    }

    pMob	= new_mob_index();

    /* Clone mobile */
    free_string(pMob->player_name);
    free_string(pMob->short_descr);
    free_string(pMob->long_descr);
    free_string(pMob->description);
    free_string(pMob->material);
    free_string(pMob->msg_arrive);
    free_string(pMob->msg_gone);

    pMob->player_name = str_dup(orig->player_name);
    pMob->short_descr = str_dup(orig->short_descr);
    pMob->long_descr  = str_dup(orig->long_descr);
    pMob->description = str_dup(orig->description);
    pMob->material    = str_dup(orig->material);
    pMob->msg_arrive  = str_dup(orig->msg_arrive);
    pMob->msg_gone    = str_dup(orig->msg_gone);
    pMob->act         = orig->act;
    pMob->affected_by = orig->affected_by;
	pMob->saving_throw = orig->saving_throw;
    pMob->alignment   = orig->alignment;
    pMob->level       = orig->level;
    pMob->hitroll     = orig->hitroll;
    pMob->dam_type    = orig->dam_type;
    pMob->off_flags   = orig->off_flags;
    pMob->start_pos   = orig->start_pos;
    pMob->default_pos = orig->default_pos;
    pMob->sex         = orig->sex;
    pMob->race        = orig->race;
    pMob->wealth      = orig->wealth;
    pMob->form        = orig->form;
    pMob->parts       = orig->parts;
    pMob->size        = orig->size;
    pMob->clan        = orig->clan;
    pMob->rating      = orig->rating;

    pMob->spec_fun    = orig->spec_fun;

    for (i = 0; i < 3; i++)
	pMob->hit[i] = orig->hit[i];
    
    for (i = 0; i < 3; i++)
	pMob->mana[i] = orig->mana[i];
    
    for (i = 0; i < 3; i++)
	pMob->damage[i] = orig->damage[i];
    
    for (i = 0; i < 4; i++)
	pMob->ac[i] = orig->ac[i];
    
    for (i = 0; i < DAM_MAX; i++)
	pMob->resists[i] = orig->resists[i];
    
    for (i = 0; i < MAX_CHATBOOKS; i++)
	pMob->chat_vnum[i] = orig->chat_vnum[i];
    
    for (i = 0; i < SECT_MAX; i++)
	pMob->areal[i] = orig->areal[i];

    /* Clone shop */
    if (orig->pShop != NULL)
    {
	pMob->pShop = new_shop();
	pMob->pShop->keeper = clone;

	for (i = 0; i < MAX_TRADE; i++)
	    pMob->pShop->buy_type[i] = orig->pShop->buy_type[i];

	pMob->pShop->profit_buy  = orig->pShop->profit_buy;
	pMob->pShop->profit_sell = orig->pShop->profit_sell;
	pMob->pShop->open_hour   = orig->pShop->open_hour;
	pMob->pShop->close_hour  = orig->pShop->close_hour;
    }

    pMob->vnum	= clone;
    pMob->area	= pArea;

    snprintf(arg, sizeof(arg), "Cloned from %d.", value);
    pMob->comments = str_dup(arg);

    if (clone > top_vnum_mob)
	top_vnum_mob = clone;

    iHash			= clone % MAX_KEY_HASH;
    pMob->next			= mob_index_hash[iHash];
    mob_index_hash[iHash]	= pMob;
    ch->desc->pEdit		= (void *)pMob;

    send_to_char("Mobile Cloned.\n\r", ch);
    return TRUE;
}

MEDIT(medit_spec)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax:  spec [special function]\n\r", ch);
	return FALSE;
    }


    if (!str_cmp(argument, "none"))
    {
	pMob->spec_fun = NULL;

	send_to_char("Spec removed.\n\r", ch);
	return TRUE;
    }

    if (spec_lookup(argument))
    {
	pMob->spec_fun = spec_lookup(argument);
	send_to_char("Spec set.\n\r", ch);
	return TRUE;
    }

    send_to_char("MEdit: No such special function.\n\r", ch);
    return FALSE;
}

MEDIT(medit_damtype)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax:  damtype [damage message]\n\r", ch);
	send_to_char("Para ver una lista de tipos de mensajes, pon '? weapon'.\n\r", ch);
	return FALSE;
    }

    pMob->dam_type = attack_lookup(argument);
    send_to_char("Damage type set.\n\r", ch);
    return TRUE;
}


MEDIT(medit_align)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0' || !is_number(argument))
    {
	send_to_char("Syntax:  alignment [number]\n\r", ch);
	return FALSE;
    }

    pMob->alignment = atoi(argument);

    send_to_char("Alignment set.\n\r", ch);
    return TRUE;
}

MEDIT(medit_saves)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0' || !is_number(argument))
    {
	send_to_char("Syntax:  saves [number]\n\r", ch);
	return FALSE;
    }

    pMob->saving_throw = atoi(argument);

    send_to_char("Saves set.\n\r", ch);
    return TRUE;
}



MEDIT(medit_level)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0' || !is_number(argument))
    {
	send_to_char("Syntax:  level [number]\n\r", ch);
	return FALSE;
    }

    pMob->level = atoi(argument);

    send_to_char("Level set.\n\r", ch);
    return TRUE;
}



MEDIT(medit_desc)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
	string_append(ch, &pMob->description);
	return TRUE;
    }

    send_to_char("Syntax:  desc    - line edit\n\r", ch);
    return FALSE;
}




MEDIT(medit_long)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax:  long [string]\n\r", ch);
	return FALSE;
    }

    free_string(pMob->long_descr);
    strcat(argument, "\n\r");
    pMob->long_descr = str_dup(argument);
    pMob->long_descr[0] = UPPER(pMob->long_descr[0]);

    send_to_char("Long description set.\n\r", ch);
    return TRUE;
}



MEDIT(medit_short)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax:  short [string]\n\r", ch);
	return FALSE;
    }

    free_string(pMob->short_descr);
    pMob->short_descr = str_dup(argument);

    send_to_char("Short description set.\n\r\n\r", ch);
    do_function(ch, &do_cases, pMob->short_descr);
    return TRUE;
}



MEDIT(medit_name)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax:  name [string]\n\r", ch);
	return FALSE;
    }

    free_string(pMob->player_name);
    pMob->player_name = str_dup(argument);

    send_to_char("Name set.\n\r", ch);
    return TRUE;
}

MEDIT(medit_shop)
{
    MOB_INDEX_DATA *pMob;
    char command[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH];

    argument = one_argument(argument, command);
    argument = one_argument(argument, arg1);

    EDIT_MOB(ch, pMob);

    if (command[0] == '\0')
    {
	send_to_char("Syntax:  shop hours [#xopening] [#xclosing]\n\r", ch);
	send_to_char("         shop profit [#xbuying%] [#xselling%]\n\r", ch);
	send_to_char("         shop type [#x0-4] [item type]\n\r", ch);
	send_to_char("         shop assign\n\r", ch);
	send_to_char("         shop remove\n\r", ch);
	return FALSE;
    }


    if (!str_cmp(command, "hours"))
    {
	if (arg1[0] == '\0' || !is_number(arg1)
	    || argument[0] == '\0' || !is_number(argument))
	{
	    send_to_char("Syntax:  shop hours [#xopening] [#xclosing]\n\r", ch);
	    return FALSE;
	}

	if (!pMob->pShop)
	{
	    send_to_char("MEdit:  Debes crear un shop primero (shop assign).\n\r", ch);
	    return FALSE;
	}

	pMob->pShop->open_hour = atoi(arg1);
	pMob->pShop->close_hour = atoi(argument);

	send_to_char("Shop hours set.\n\r", ch);
	return TRUE;
    }


    if (!str_cmp(command, "profit"))
    {
	if (arg1[0] == '\0' || !is_number(arg1)
	    || argument[0] == '\0' || !is_number(argument))
	{
	    send_to_char("Syntax:  shop profit [#xbuying%] [#xselling%]\n\r", ch);
	    return FALSE;
	}

	if (!pMob->pShop)
	{
	    send_to_char("MEdit:  Debes crear un shop primero (shop assign).\n\r", ch);
	    return FALSE;
	}

	pMob->pShop->profit_buy     = atoi(arg1);
	pMob->pShop->profit_sell    = atoi(argument);

	send_to_char("Shop profit set.\n\r", ch);
	return TRUE;
    }


    if (!str_cmp(command, "type"))
    {
	char buf[MAX_INPUT_LENGTH];
	int value;

	if (arg1[0] == '\0' || !is_number(arg1)
	    || argument[0] == '\0')
	{
	    send_to_char("Syntax:  shop type [#x0-4] [item type]\n\r", ch);
	    return FALSE;
	}

	if (atoi(arg1) >= MAX_TRADE)
	{
	    sprintf(buf, "MEdit:  May sell %d items max.\n\r", MAX_TRADE);
	    send_to_char(buf, ch);
	    return FALSE;
	}

	if (!pMob->pShop)
	{
	    send_to_char("MEdit:  Debes crear un shop primero (shop assign).\n\r", ch);
	    return FALSE;
	}

	if ((value = flag_value(type_flags, argument)) == NO_FLAG)
	{
	    send_to_char("MEdit:  That type of item is not known.\n\r", ch);
	    return FALSE;
	}

	pMob->pShop->buy_type[atoi(arg1)] = value;

	send_to_char("Shop type set.\n\r", ch);
	return TRUE;
    }

    /* shop assign && shop delete by Phoenix */

    if (!str_prefix(command, "assign"))
    {
	if (pMob->pShop)
	{
	    send_to_char("Mob already has a shop assigned to it.\n\r", ch);
	    return FALSE;
	}

	pMob->pShop		= new_shop();
	if (!shop_first)
	    shop_first		= pMob->pShop;
	if (shop_last)
	    shop_last->next	= pMob->pShop;
	shop_last		= pMob->pShop;

	pMob->pShop->keeper	= pMob->vnum;

	send_to_char("New shop assigned to mobile.\n\r", ch);
	return TRUE;
    }

    if (!str_prefix(command, "remove"))
    {
	SHOP_DATA *pShop;

	pShop		= pMob->pShop;
	pMob->pShop	= NULL;

	if (pShop == shop_first)
	{
	    if (!pShop->next)
	    {
		shop_first = NULL;
		shop_last = NULL;
	    }
	    else
		shop_first = pShop->next;
	}
	else
	{
	    SHOP_DATA *ipShop;

	    for (ipShop = shop_first; ipShop; ipShop = ipShop->next)
	    {
		if (ipShop->next == pShop)
		{
		    if (!pShop->next)
		    {
			shop_last = ipShop;
			shop_last->next = NULL;
		    }
		    else
			ipShop->next = pShop->next;
		}
	    }
	}

	free_shop(pShop);

	send_to_char("Mobile is no longer a shopkeeper.\n\r", ch);
	return TRUE;
    }

    medit_shop(ch, "");
    return FALSE;
}


/* ROM medit functions: */


MEDIT(medit_sex)          /* Moved out of medit() due to naming conflicts -- Hugin */
{
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
	EDIT_MOB(ch, pMob);

	if ((value = flag_value(sex_flags, argument)) != NO_FLAG)
	{
	    pMob->sex = value;

	    send_to_char("Sex set.\n\r", ch);
	    return TRUE;
	}
    }

    send_to_char("Syntax: sex [sex]\n\r"
		 "Type '? sex' for a list of flags.\n\r", ch);
    return FALSE;
}


MEDIT(medit_act)          /* Moved out of medit() due to naming conflicts -- Hugin */
{
    MOB_INDEX_DATA *pMob;
    int64_t value;

    if (argument[0] != '\0')
    {
	EDIT_MOB(ch, pMob);

	if ((value = flag_value(act_flags, argument)) != NO_FLAG)
	{
	    pMob->act ^= value;
	    SET_BIT(pMob->act, ACT_IS_NPC);

	    send_to_char("Act flag toggled.\n\r", ch);
	    return TRUE;
	}
    }

    send_to_char("Syntax: act [flag]\n\r"
		 "Type '? act' for a list of flags.\n\r", ch);
    return FALSE;
}


MEDIT(medit_affect)      /* Moved out of medit() due to naming conflicts -- Hugin */
{
    MOB_INDEX_DATA *pMob;
    int64_t value;

    if (argument[0] != '\0')
    {
	EDIT_MOB(ch, pMob);

	if ((value = flag_value(affect_flags, argument)) != NO_FLAG)
	{
	    pMob->affected_by ^= value;

	    send_to_char("Affect flag toggled.\n\r", ch);
	    return TRUE;
	}
    }

    send_to_char("Syntax: affect [flag]\n\r"
		 "Type '? affect' for a list of flags.\n\r", ch);
    return FALSE;
}



MEDIT(medit_ac)
{
    MOB_INDEX_DATA *pMob;
    char arg[MAX_INPUT_LENGTH];
    int pierce, bash, slash, exotic;

    do   /* So that I can use break and send the syntax in one place */
    {
	if (argument[0] == '\0')  break;

	EDIT_MOB(ch, pMob);
	argument = one_argument(argument, arg);

	if (!is_number(arg))  break;
	pierce = atoi(arg);
	argument = one_argument(argument, arg);

	if (arg[0] != '\0')
	{
	    if (!is_number(arg))  break;
	    bash = atoi(arg);
	    argument = one_argument(argument, arg);
	}
	else
	    bash = pMob->ac[AC_BASH];

	if (arg[0] != '\0')
	{
	    if (!is_number(arg))  break;
	    slash = atoi(arg);
	    argument = one_argument(argument, arg);
	}
	else
	    slash = pMob->ac[AC_SLASH];

	if (arg[0] != '\0')
	{
	    if (!is_number(arg))  break;
	    exotic = atoi(arg);
	}
	else
	    exotic = pMob->ac[AC_EXOTIC];

	pMob->ac[AC_PIERCE] = pierce;
	pMob->ac[AC_BASH]   = bash;
	pMob->ac[AC_SLASH]  = slash;
	pMob->ac[AC_EXOTIC] = exotic;

	send_to_char("Ac set.\n\r", ch);
	return TRUE;
    } while (FALSE);    /* Just do it once.. */

    send_to_char("Syntax:  ac [ac-pierce [ac-bash [ac-slash [ac-exotic]]]]\n\r"
		 "help MOB_AC  gives a list of reasonable ac-values.\n\r", ch);
    return FALSE;
}

MEDIT(medit_form)
{
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
	EDIT_MOB(ch, pMob);

	if ((value = flag_value(form_flags, argument)) != NO_FLAG)
	{
	    pMob->form ^= value;
	    send_to_char("Form toggled.\n\r", ch);
	    return TRUE;
	}
    }

    send_to_char("Syntax: form [flags]\n\r"
		 "Type '? form' for a list of flags.\n\r", ch);
    return FALSE;
}

MEDIT(medit_part)
{
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
	EDIT_MOB(ch, pMob);

	if ((value = flag_value(part_flags, argument)) != NO_FLAG)
	{
	    pMob->parts ^= value;
	    send_to_char("Parts toggled.\n\r", ch);
	    return TRUE;
	}
    }

    send_to_char("Syntax: part [flags]\n\r"
		 "Type '? part' for a list of flags.\n\r", ch);
    return FALSE;
}

MEDIT(medit_res)
{
    MOB_INDEX_DATA *pMob;
    int value;
    char dam[MAX_STRING_LENGTH];

    argument = one_argument(argument, dam);
    if (!IS_NULLSTR(argument) && is_number(argument))
    {
	EDIT_MOB(ch, pMob);

	if ((value = flag_value(dam_flags, dam)) != NO_FLAG)
	{
	    pMob->resists[value] = atoi(argument);
	    send_to_char("Resistance toggled.\n\r", ch);
	    return TRUE;
	}
    }

    send_to_char("Syntax: res <damage type> <amount>\n\r"
		 "Type '? res' for a list of flags.\n\r", ch);
    return FALSE;
}

MEDIT(medit_material)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax:  material [string]\n\r", ch);
	return FALSE;
    }

    free_string(pMob->material);
    pMob->material = str_dup(argument);

    send_to_char("Material set.\n\r", ch);
    return TRUE;
}

MEDIT(medit_off)
{
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
	EDIT_MOB(ch, pMob);

	if ((value = flag_value(off_flags, argument)) != NO_FLAG)
	{
	    pMob->off_flags ^= value;
	    send_to_char("Offensive behaviour toggled.\n\r", ch);
	    return TRUE;
	}
    }

    send_to_char("Syntax: off [flags]\n\r"
		 "Type '? off' for a list of flags.\n\r", ch);
    return FALSE;
}

MEDIT(medit_size)
{
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
	EDIT_MOB(ch, pMob);

	if ((value = flag_value(size_flags, argument)) != NO_FLAG)
	{
	    pMob->size = value;
	    send_to_char("Size set.\n\r", ch);
	    return TRUE;
	}
    }

    send_to_char("Syntax: size [size]\n\r"
		 "Type '? size' for a list of sizes.\n\r", ch);
    return FALSE;
}

MEDIT(medit_hitdice)
{
    static char syntax[] = "Syntax:  hitdice <number> d <type> + <bonus>\n\r";
    char *num, *type, *bonus, *cp;
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
	send_to_char(syntax, ch);
	return FALSE;
    }

    num = cp = argument;

    while (isdigit(*cp)) ++cp;
    while (*cp != '\0' && !isdigit(*cp))  *(cp++) = '\0';

    type = cp;

    while (isdigit(*cp)) ++cp;
    while (*cp != '\0' && !isdigit(*cp)) *(cp++) = '\0';

    bonus = cp;

    while (isdigit(*cp)) ++cp;
    if (*cp != '\0') *cp = '\0';

    if ((!is_number(num) || atoi(num) < 1)
	||   (!is_number(type) || atoi(type) < 1)
	||   (!is_number(bonus) || atoi(bonus) < 0))
    {
	send_to_char(syntax, ch);
	return FALSE;
    }

    pMob->hit[DICE_NUMBER] = atoi(num);
    pMob->hit[DICE_TYPE]   = atoi(type);
    pMob->hit[DICE_BONUS]  = atoi(bonus);

    send_to_char("Hitdice set.\n\r", ch);
    return TRUE;
}

MEDIT(medit_manadice)
{
    static char syntax[] = "Syntax:  manadice <number> d <type> + <bonus>\n\r";
    char *num, *type, *bonus, *cp;
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
	send_to_char(syntax, ch);
	return FALSE;
    }

    num = cp = argument;

    while (isdigit(*cp)) ++cp;
    while (*cp != '\0' && !isdigit(*cp))  *(cp++) = '\0';

    type = cp;

    while (isdigit(*cp)) ++cp;
    while (*cp != '\0' && !isdigit(*cp)) *(cp++) = '\0';

    bonus = cp;

    while (isdigit(*cp)) ++cp;
    if (*cp != '\0') *cp = '\0';

    if (!(is_number(num) && is_number(type) && is_number(bonus)))
    {
	send_to_char(syntax, ch);
	return FALSE;
    }

    if ((!is_number(num) || atoi(num) < 1)
	||   (!is_number(type) || atoi(type) < 1)
	||   (!is_number(bonus) || atoi(bonus) < 0))
    {
	send_to_char(syntax, ch);
	return FALSE;
    }

    pMob->mana[DICE_NUMBER] = atoi(num);
    pMob->mana[DICE_TYPE]   = atoi(type);
    pMob->mana[DICE_BONUS]  = atoi(bonus);

    send_to_char("Manadice set.\n\r", ch);
    return TRUE;
}

MEDIT(medit_damdice)
{
    static char syntax[] = "Syntax:  damdice <number> d <type> + <bonus>\n\r";
    char *num, *type, *bonus, *cp;
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
	send_to_char(syntax, ch);
	return FALSE;
    }

    num = cp = argument;

    while (isdigit(*cp)) ++cp;
    while (*cp != '\0' && !isdigit(*cp))  *(cp++) = '\0';

    type = cp;

    while (isdigit(*cp)) ++cp;
    while (*cp != '\0' && !isdigit(*cp)) *(cp++) = '\0';

    bonus = cp;

    while (isdigit(*cp)) ++cp;
    if (*cp != '\0') *cp = '\0';

    if (!(is_number(num) && is_number(type) && is_number(bonus)))
    {
	send_to_char(syntax, ch);
	return FALSE;
    }

    if ((!is_number(num) || atoi(num) < 1)
	||   (!is_number(type) || atoi(type) < 1)
	||   (!is_number(bonus) || atoi(bonus) < 0))
    {
	send_to_char(syntax, ch);
	return FALSE;
    }

    pMob->damage[DICE_NUMBER] = atoi(num);
    pMob->damage[DICE_TYPE]   = atoi(type);
    pMob->damage[DICE_BONUS]  = atoi(bonus);

    send_to_char("Damdice set.\n\r", ch);
    return TRUE;
}

#define ME_RACE_ADD	0
#define ME_RACE_SET	1
#define ME_RACE_NOSET	2

MEDIT(medit_race)
{
    MOB_INDEX_DATA *pMob;
    char buf[MAX_STRING_LENGTH];
    int race;
    int i;
    int	mode = ME_RACE_SET;

    if (argument[0] == '?')
    {

	send_to_char("Syntax: race <race_name> [add|noset]\n\r", ch);
	send_to_char("Available races are:", ch);

	for (race = 0; race < max_races; race++)
	{
	    if ((race % 3) == 0)
		send_to_char("\n\r", ch);
	    printf_to_char(" %-15s", ch, race_table[race].name);
	}

	send_to_char("\n\r", ch);
	return FALSE;
    }

    argument = one_argument(argument, buf);

    if (!IS_NULLSTR(buf)
	&& (race = race_lookup(buf)) != -1)
    {
	EDIT_MOB(ch, pMob);

	if (!IS_NULLSTR(argument))
	{
	    if (!str_cmp(argument, "add"))
		mode = ME_RACE_ADD;
	    else if (!str_cmp(argument, "noset"))
		mode = ME_RACE_NOSET;
	}

	pMob->race = race;

	switch (mode)
	{
	case ME_RACE_ADD:
	    pMob->act	      |= race_table[race].act;
	    pMob->affected_by |= race_table[race].aff;
	    pMob->off_flags   |= race_table[race].off;
	    for (i = 0; i < DAM_MAX; i++)
		pMob->resists[i] += race_table[race].resists[i];
	    pMob->form        |= race_table[race].form;
	    pMob->parts       |= race_table[race].parts;
	    send_to_char("Race params added to mob params.\n\r", ch);
	    break;

	case ME_RACE_SET:
	    pMob->act	      = race_table[race].act;
	    pMob->affected_by = race_table[race].aff;
	    pMob->off_flags   = race_table[race].off;
	    for (i = 0; i < DAM_MAX; i++)
		pMob->resists[i] = race_table[race].resists[i];
	    pMob->form        = race_table[race].form;
	    pMob->parts       = race_table[race].parts;
	    send_to_char("Race params set to mob.\n\r", ch);
	    break;

	default:
	    send_to_char("Mob params unchanged.\n\r", ch);
	    break;
	}
	SET_BIT(pMob->act, ACT_IS_NPC);
	send_to_char("Race set.\n\r", ch);
	return TRUE;
    }

    send_to_char("Syntax:  race [race] [add|noset]\n\r"
		 "Type 'race ?' for a list of races.\n\r", ch);
    return FALSE;
}


MEDIT(medit_position)
{
    MOB_INDEX_DATA *pMob;
    char arg[MAX_INPUT_LENGTH];
    int value;

    argument = one_argument(argument, arg);

    switch (arg[0])
    {
    default:
	break;

    case 'S':
    case 's':
	if (str_prefix(arg, "start"))
	    break;

	if ((value = flag_value(position_flags, argument)) == NO_FLAG)
	    break;

	EDIT_MOB(ch, pMob);

	pMob->start_pos = value;
	send_to_char("Start position set.\n\r", ch);
	return TRUE;

    case 'D':
    case 'd':
	if (str_prefix(arg, "default"))
	    break;

	if ((value = flag_value(position_flags, argument)) == NO_FLAG)
	    break;

	EDIT_MOB(ch, pMob);

	pMob->default_pos = value;
	send_to_char("Default position set.\n\r", ch);
	return TRUE;
    }

    send_to_char("Syntax:  position [start/default] [position]\n\r"
		 "Type '? position' for a list of positions.\n\r", ch);
    return FALSE;
}


MEDIT(medit_gold)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0' || !is_number(argument))
    {
	send_to_char("Syntax:  wealth [number]\n\r", ch);
	return FALSE;
    }

    pMob->wealth = atoi(argument);

    send_to_char("Wealth set.\n\r", ch);
    return TRUE;
}

MEDIT(medit_hitroll)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0' || !is_number(argument))
    {
	send_to_char("Syntax:  hitroll [number]\n\r", ch);
	return FALSE;
    }

    pMob->hitroll = atoi(argument);

    send_to_char("Hitroll set.\n\r", ch);
    return TRUE;
}

void show_liqlist(CHAR_DATA *ch)
{
    int liq;
    BUFFER *buffer;
    char buf[MAX_STRING_LENGTH];

    buffer = new_buf();

    for (liq = 0; liq_table[liq].liq_name != NULL; liq++)
    {
	if ((liq % 21) == 0)
	    add_buf(buffer,"Name                 Color          Proof Full Thirst Food Ssize\n\r");

	sprintf(buf, "%-20s %-14s %5d %4d %6d %4d %5d\n\r",
		liq_table[liq].liq_name,liq_table[liq].liq_color,
		liq_table[liq].liq_affect[0],liq_table[liq].liq_affect[1],
		liq_table[liq].liq_affect[2],liq_table[liq].liq_affect[3],
		liq_table[liq].liq_affect[4]);
	add_buf(buffer,buf);
    }

    page_to_char(buf_string(buffer),ch);
    free_buf(buffer);

    return;
}

void show_damlist(CHAR_DATA *ch)
{
    int att;
    BUFFER *buffer;
    char buf[MAX_STRING_LENGTH];

    buffer = new_buf();

    for (att = 0; attack_table[att].name != NULL; att++)
    {
	if ((att % 21) == 0)
	    add_buf(buffer,"Name                 Noun\n\r");

	sprintf(buf, "%-20s %-20s\n\r",
		attack_table[att].name,attack_table[att].noun);
	add_buf(buffer,buf);
    }

    page_to_char(buf_string(buffer),ch);
    free_buf(buffer);

    return;
}

MEDIT(medit_group)
{
    MOB_INDEX_DATA *pMob;
    MOB_INDEX_DATA *pMTemp;
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int temp;
    BUFFER *buffer;
    bool found = FALSE;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: group [number]\n\r", ch);
	send_to_char("        group show [number]\n\r", ch);
	return FALSE;
    }

    if (is_number(argument))
    {
	pMob->group = atoi(argument);
	send_to_char("Group set.\n\r", ch);
	return TRUE;
    }

    argument = one_argument(argument, arg);

    if (!strcmp(arg, "show") && is_number(argument))
    {
	if (atoi(argument) == 0)
	{
	    send_to_char("Are you crazy?\n\r", ch);
	    return FALSE;
	}

	buffer = new_buf();

	for (temp = 0; temp < MAX_KEY_HASH; temp++)
	{
	    for (pMTemp = mob_index_hash[temp]; pMTemp != NULL; pMTemp = pMTemp->next)
	    {
		if ((pMTemp->group == atoi(argument)))
		{
		    found = TRUE;
		    sprintf(buf, "[%5d] %s\n\r", pMTemp->vnum, pMTemp->player_name);
		    add_buf(buffer, buf);
		}
	    }
	}

	if (found)
	    page_to_char(buf_string(buffer), ch);
	else
	    send_to_char("No mobs in that group.\n\r", ch);

	free_buf(buffer);
	return FALSE;
    }

    return FALSE;
}

REDIT(redit_owner)
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM(ch, pRoom);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax:  owner [owner]\n\r", ch);
	send_to_char("         owner none\n\r", ch);
	return FALSE;
    }

    free_string(pRoom->owner);
    if (!str_cmp(argument, "none"))
	pRoom->owner = str_dup("");
    else
	pRoom->owner = str_dup(argument);

    send_to_char("Owner set.\n\r", ch);
    return TRUE;
}

MEDIT (medit_addmprog)
{
    int64_t value, value1;
    MOB_INDEX_DATA *pMob;
    PROG_LIST *list;
    PROG_CODE *code;
    char trigger[MAX_STRING_LENGTH];
    char num[MAX_STRING_LENGTH];
    char when[MIL];

    EDIT_MOB(ch, pMob);
    argument=one_argument(argument, num);
    argument=one_argument(argument, trigger);
    argument=one_argument(argument, when);

    if (!is_number(num) || trigger[0] =='\0' || argument[0] =='\0')
    {
	send_to_char("Syntax:   addmprog <vnum> <trigger> [before|after|default] <phrase>\n\r",ch);
	return FALSE;
    }

    if ((value = flag_value (mprog_flags, trigger)) == NO_FLAG)
    {
	send_to_char("Valid flags are:\n\r",ch);
	show_help(ch, "mprog");
	return FALSE;
    }

    if ((code =get_prog_index (atoi(num), PRG_MPROG)) == NULL)
    {
	send_to_char("No such MOBProgram.\n\r",ch);
	return FALSE;
    }

    if ((value1 = flag_value (prog_when_flags, when)) == NO_FLAG)
        value1 = EXEC_DEFAULT;

    list                  = new_mprog();
    list->vnum            = atoi(num);
    SET_BIT(list->trig_flags, value1);
    list->trig_type       = value;
    list->trig_phrase     = str_dup(argument);
    list->code            = code->code;
    list->next            = pMob->progs;
    pMob->progs          = list;

    send_to_char("Mprog Added.\n\r",ch);
    return TRUE;
}

MEDIT (medit_delmprog)
{
    MOB_INDEX_DATA *pMob;
    PROG_LIST *list;
    PROG_LIST *list_next;
    char mprog[MAX_STRING_LENGTH];
    int value;
    int cnt = 0;

    EDIT_MOB(ch, pMob);

    one_argument(argument, mprog);
    if (!is_number(mprog) || mprog[0] == '\0')
    {
	send_to_char("Syntax:  delmprog [#mprog]\n\r",ch);
	return FALSE;
    }

    value = atoi (mprog);

    if (value < 0)
    {
	send_to_char("Only non-negative mprog-numbers allowed.\n\r",ch);
	return FALSE;
    }

    if (!(list = pMob->progs))
    {
	send_to_char("MEdit:  Non existant mprog.\n\r",ch);
	return FALSE;
    }

    if (value == 0)
    {
//	REMOVE_BIT(pMob->progs->trig_flags, pMob->progs->trig_type);
	list = pMob->progs;
	pMob->progs = list->next;
	//        free_mprog(list);
    }
    else
    {
	while ((list_next = list->next) && (++cnt < value))
	    list = list_next;

	if (list_next)
	{
//	    REMOVE_BIT(pMob->mprog_flags, list_next->trig_type);
	    list->next = list_next->next;
	    //                free_mprog(list_next);
	}
	else
	{
	    send_to_char("No such mprog.\n\r",ch);
	    return FALSE;
	}
    }

    send_to_char("Mprog removed.\n\r", ch);
    return TRUE;
}

REDIT(redit_room)
{
    ROOM_INDEX_DATA *room;
    int64_t value;

    EDIT_ROOM(ch, room);

    if ((value = flag_value(room_flags, argument)) == NO_FLAG)
    {
	send_to_char("Sintaxis: room [flags]\n\r", ch);
	return FALSE;
    }

    TOGGLE_BIT(room->room_flags, value);
    send_to_char("Room flags toggled.\n\r", ch);
    return TRUE;
}

REDIT(redit_sector)
{
    ROOM_INDEX_DATA *room;
    int value;

    EDIT_ROOM(ch, room);

    if ((value = flag_value(sector_flags, argument)) == NO_FLAG)
    {
	send_to_char("Sintaxis: sector [tipo]\n\r", ch);
	return FALSE;
    }

    room->sector_type = value;
    send_to_char("Sector type set.\n\r", ch);

    return TRUE;
}

OEDIT (oedit_addoprog)
{
    int64_t value, value1;
    OBJ_INDEX_DATA *pObj;
    PROG_LIST *list;
    PROG_CODE *code;
    char trigger[MAX_STRING_LENGTH];
    char num[MAX_STRING_LENGTH];
    char when[MIL];

    EDIT_OBJ(ch, pObj);
    argument=one_argument(argument, num);
    argument=one_argument(argument, trigger);
    argument=one_argument(argument, when);

    if (!is_number(num) || trigger[0] =='\0' || argument[0] =='\0')
    {
	send_to_char("Syntax:   addoprog [vnum] [trigger] [before|after|default] [phrase]\n\r",ch);
	return FALSE;
    }

    if ((value = flag_value (oprog_flags, trigger)) == NO_FLAG)
    {
	send_to_char("Valid flags are:\n\r",ch);
	show_help(ch, "oprog");
	return FALSE;
    }

    if ((code =get_prog_index (atoi(num), PRG_OPROG)) == NULL)
    {
	send_to_char("No such OBJProgram.\n\r",ch);
	return FALSE;
    }

    if ((value1 = flag_value (prog_when_flags, when)) == NO_FLAG)
        value1 = EXEC_DEFAULT;

    list                  = new_oprog();
    list->vnum            = atoi(num);
    list->trig_type       = value;
    SET_BIT(list->trig_flags, value1);
    list->trig_phrase     = str_dup(argument);
    list->code            = code->code;
    list->next            = pObj->progs;
    pObj->progs          = list;

    send_to_char("Oprog Added.\n\r",ch);
    return TRUE;
}

OEDIT (oedit_deloprog)
{
    OBJ_INDEX_DATA *pObj;
    PROG_LIST *list;
    PROG_LIST *list_next;
    char oprog[MAX_STRING_LENGTH];
    int value;
    int cnt = 0;

    EDIT_OBJ(ch, pObj);

    one_argument(argument, oprog);
    if (!is_number(oprog) || oprog[0] == '\0')
    {
	send_to_char("Syntax:  deloprog [#oprog]\n\r",ch);
	return FALSE;
    }

    value = atoi (oprog);

    if (value < 0)
    {
	send_to_char("Only non-negative oprog-numbers allowed.\n\r",ch);
	return FALSE;
    }

    if (!(list= pObj->progs))
    {
	send_to_char("OEdit:  Non existant oprog.\n\r",ch);
	return FALSE;
    }

    if (value == 0)
    {
//	REMOVE_BIT(pObj->oprog_flags, pObj->progs->trig_type);
	list = pObj->progs;
	pObj->progs = list->next;
	//        free_oprog(list);
    }
    else
    {
	while ((list_next = list->next) && (++cnt < value))
	    list = list_next;

	if (list_next)
	{
//	    REMOVE_BIT(pObj->oprog_flags, list_next->trig_type);
	    list->next = list_next->next;
	    //                free_oprog(list_next);
	}
	else
	{
	    send_to_char("No such oprog.\n\r",ch);
	    return FALSE;
	}
    }

    send_to_char("Oprog removed.\n\r", ch);
    return TRUE;
}

REDIT (redit_addrprog)
{
    int64_t value, value1;
    ROOM_INDEX_DATA *pRoom;
    PROG_LIST *list;
    PROG_CODE *code;
    char trigger[MAX_STRING_LENGTH];
    char num[MAX_STRING_LENGTH];
    char when[MIL];

    EDIT_ROOM(ch, pRoom);
    argument=one_argument(argument, num);
    argument=one_argument(argument, trigger);
    argument=one_argument(argument, when);

    if (!is_number(num) || trigger[0] =='\0' || argument[0] =='\0')
    {
	send_to_char("Syntax:   addrprog [vnum] [trigger] [before|after|default] [phrase]\n\r",ch);
	return FALSE;
    }

    if ((value = flag_value (rprog_flags, trigger)) == NO_FLAG)
    {
	send_to_char("Valid flags are:\n\r",ch);
	show_help(ch, "rprog");
	return FALSE;
    }

    if ((code =get_prog_index (atoi(num), PRG_RPROG)) == NULL)
    {
	send_to_char("No such ROOMProgram.\n\r",ch);
	return FALSE;
    }

    if ((value1 = flag_value (prog_when_flags, when)) == NO_FLAG)
        value1 = EXEC_DEFAULT;

    list                  = new_rprog();
    list->vnum            = atoi(num);
    list->trig_type       = value;
    SET_BIT(list->trig_flags, value1);
    list->trig_phrase     = str_dup(argument);
    list->code            = code->code;
    list->next            = pRoom->progs;
    pRoom->progs          = list;

    send_to_char("Rprog Added.\n\r",ch);
    return TRUE;
}

REDIT (redit_delrprog)
{
    ROOM_INDEX_DATA *pRoom;
    PROG_LIST *list;
    PROG_LIST *list_next;
    char rprog[MAX_STRING_LENGTH];
    int value;
    int cnt = 0;

    EDIT_ROOM(ch, pRoom);

    one_argument(argument, rprog);
    if (!is_number(rprog) || rprog[0] == '\0')
    {
	send_to_char("Syntax:  delrprog [#rprog]\n\r",ch);
	return FALSE;
    }

    value = atoi (rprog);

    if (value < 0)
    {
	send_to_char("Only non-negative rprog-numbers allowed.\n\r",ch);
	return FALSE;
    }

    if (!(list= pRoom->progs))
    {
	send_to_char("REdit:  Non existant rprog.\n\r",ch);
	return FALSE;
    }

    if (value == 0)
    {
//	REMOVE_BIT(pRoom->rprog_flags, pRoom->progs->trig_type);
	list = pRoom->progs;
	pRoom->progs = list->next;
	//        free_rprog(list);
    }
    else
    {
	while ((list_next = list->next) && (++cnt < value))
	    list = list_next;

	if (list_next)
	{
//	    REMOVE_BIT(pRoom->rprog_flags, list_next->trig_type);
	    list->next = list_next->next;
	    //                free_rprog(list_next);
	}
	else
	{
	    send_to_char("No such rprog.\n\r",ch);
	    return FALSE;
	}
    }

    send_to_char("Rprog removed.\n\r", ch);
    return TRUE;
}

HEDIT(hedit_create)
{
    HELP_DATA *pHelp;
    HELP_AREA *pArea;

    if (argument[0] != '\0'
	&& (pArea = get_help_area(argument)))
    {
	pHelp = new_help();
	pHelp->help_area = pArea;
	STAILQ_INSERT_HEAD(&pArea->helps, pHelp, area_link);
	STAILQ_INSERT_HEAD(&help_first, pHelp, link);

	ch->desc->pEdit = (void *)pHelp;
	pArea->changed = TRUE;
	send_to_char("Help created.\n\r", ch);

	return TRUE;
    }
    else
    {
	send_to_char("Check your brains, guy!\n\r", ch);
	return FALSE;
    }
}

HEDIT(hedit_delete)
{
    HELP_DATA *pHelp;

    EDIT_HELP(ch, pHelp);

    STAILQ_REMOVE(&help_first, pHelp, help_data, link);
    
    free_help(pHelp);
    send_to_char("Help deleted.\n\r", ch);
    edit_done(ch);

    return TRUE;
}

HEDIT(hedit_show)
{
    HELP_DATA *pHelp;
    char buf[2*MAX_STRING_LENGTH];

    EDIT_HELP(ch, pHelp);

    sprintf(buf, "Keywords : [%s]\n\r", pHelp->keyword);
    send_to_char(buf, ch);
    sprintf(buf, "Filename : %s\n\r", pHelp->help_area->filename);
    send_to_char(buf, ch);
    sprintf(buf, "Min Level: %d\n\r", pHelp->level);
    send_to_char(buf, ch);
    sprintf(buf, "Text     :\n\r%s\n\r", pHelp->text);
    send_to_char(buf, ch);

    return FALSE;
}

HEDIT(hedit_level)
{
    HELP_DATA *pHelp;

    EDIT_HELP(ch, pHelp);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax:   level <level>\n\r", ch);
	return FALSE;
    }

    if (!is_number(argument))
    {
	send_to_char("Level must be a number!\n\r", ch);
	return FALSE;
    }

    pHelp->level = atoi(argument);
    send_to_char("Min level set.\n\r", ch);
    return TRUE;
}

HEDIT(hedit_keywords)
{
    HELP_DATA *pHelp;
    char buf[MSL];
    int i;

    if (argument[0] == '\0')
    {
	send_to_char("Syntax:   keywords <keyword> [<keyword>] ...\n\r", ch);
	return FALSE;
    }

    for (i = 0; argument[i] != '\0'; i++)
	argument[i] = UPPER(argument[i]);

    EDIT_HELP(ch, pHelp);

    if (pHelp->keyword)
	free_string(pHelp->keyword);

    pHelp->keyword = str_dup(argument);

    sprintf(buf, "Keywords: %s\n\r", pHelp->keyword);
    send_to_char(buf, ch);
    return TRUE;
}

HEDIT(hedit_text)
{
    HELP_DATA *pHelp;

    EDIT_HELP(ch, pHelp);
    string_append(ch, &pHelp->text);
    return TRUE;
}


/* unlink a given reset from a given room */
void unlink_reset(ROOM_INDEX_DATA *pRoom, RESET_DATA *pReset)
{
    RESET_DATA *prev, *wReset;

    prev = pRoom->reset_first;
    for (wReset = pRoom->reset_first; wReset; wReset = wReset->next)
    {
	if (wReset == pReset)
	{
	    if (pRoom->reset_first == pReset)
	    {
		pRoom->reset_first = pReset->next;
		if (!pRoom->reset_first)
		    pRoom->reset_last = NULL;
	    }
	    else if (pRoom->reset_last == pReset)
	    {
		pRoom->reset_last = prev;
		prev->next = NULL;
	    }
	    else
		prev->next = prev->next->next;

	    if (pRoom->area->reset_first == pReset)
		pRoom->area->reset_first = pReset->next;

	    if (!pRoom->area->reset_first)
		pRoom->area->reset_last = NULL;
	}

	prev = wReset;
    }
}


void unlink_obj_index(OBJ_INDEX_DATA *pObj)
{
    int iHash;
    OBJ_INDEX_DATA *iObj, *sObj;

    iHash = pObj->vnum % MAX_KEY_HASH;

    sObj = obj_index_hash[iHash];

    if (sObj->next == NULL) /* only entry */
	obj_index_hash[iHash] = NULL;
    else if (sObj == pObj) /* first entry */
	obj_index_hash[iHash] = pObj->next;
    else /* everything else */
    {
	for (iObj = sObj; iObj != NULL; iObj = iObj->next)
	{
	    if (iObj == pObj)
	    {
		sObj->next = pObj->next;
		break;
	    }
	    sObj = iObj;
	}
    }
}

void unlink_room_index(ROOM_INDEX_DATA *pRoom)
{
    int iHash;
    ROOM_INDEX_DATA *iRoom, *sRoom;

    iHash = pRoom->vnum % MAX_KEY_HASH;

    sRoom = room_index_hash[iHash];

    if (sRoom->next == NULL) /* only entry */
	room_index_hash[iHash] = NULL;
    else if (sRoom == pRoom) /* first entry */
	room_index_hash[iHash] = pRoom->next;
    else /* everything else */
    {
	for (iRoom = sRoom; iRoom != NULL; iRoom = iRoom->next)
	{
	    if (iRoom == pRoom)
	    {
		sRoom->next = pRoom->next;
		break;
	    }
	    sRoom = iRoom;
	}
    }
}


void unlink_mob_index(MOB_INDEX_DATA *pMob)
{
    int iHash;
    MOB_INDEX_DATA *iMob, *sMob;

    iHash = pMob->vnum % MAX_KEY_HASH;

    sMob = mob_index_hash[iHash];

    if (sMob->next == NULL) /* only entry */
	mob_index_hash[iHash] = NULL;
    else if (sMob == pMob) /* first entry */
	mob_index_hash[iHash] = pMob->next;
    else /* everything else */
    {
	for (iMob = sMob; iMob != NULL; iMob = iMob->next)
	{
	    if (iMob == pMob)
	    {
		sMob->next = pMob->next;
		break;
	    }
	    sMob = iMob;
	}
    }
}


OEDIT(oedit_delete)
{
    OBJ_DATA *obj, *obj_next;
    OBJ_INDEX_DATA *pObj;
    RESET_DATA *pReset, *wReset;
    ROOM_INDEX_DATA *pRoom;
    char buf[MAX_STRING_LENGTH];
    int index, rcount, ocount, i, iHash;


    EDIT_OBJ(ch, pObj);

    if (!pObj)
    {
	send_to_char("No such object.\n\r", ch);
	return FALSE;
    }

    index = pObj->vnum;

    SET_BIT(pObj->area->area_flags, AREA_CHANGED);

    if (top_vnum_obj == index)
	for (i = 1; i < index; i++)
	    if (get_obj_index(i))
		top_vnum_obj = i;


    top_obj_index--;

    /* remove objects */
    ocount = 0;
    for (obj = object_list; obj; obj = obj_next)
    {
	obj_next = obj->next;

	if (obj->pIndexData == pObj)
	{
	    extract_obj(obj, TRUE, TRUE);
	    ocount++;
	}
    }

    /* crush resets */
    rcount = 0;
    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
	for (pRoom = room_index_hash[iHash]; pRoom; pRoom = pRoom->next)
	{
	    for (pReset = pRoom->reset_first; pReset; pReset = wReset)
	    {
		wReset = pReset->next;
		switch (pReset->command)
		{
		case 'O':
		case 'E':
		case 'P':
		case 'G':
		    if ((pReset->arg1 == index)
			|| ((pReset->command == 'P')
			    && (pReset->arg3 == index)))
		    {
			unlink_reset(pRoom, pReset);
			free_reset_data(pReset);

			rcount++;
			SET_BIT(pRoom->area->area_flags, AREA_CHANGED);
		    }
		}
	    }
	}
    }

    oedit_limit(ch, "0");

    unlink_obj_index(pObj);

    pObj->area = NULL;
    pObj->vnum = 0;

    free_obj_index(pObj);

    sprintf(buf, "Removed object vnum {C%d{x and{C%d{x resets.\n\r",
	    index, rcount);

    send_to_char(buf, ch);

    sprintf(buf, "{C%d{x occurences of the object"
	    " were extracted from the mud.\n\r", ocount);

    send_to_char(buf, ch);

    edit_done(ch);

    return TRUE;
}


MEDIT(medit_delete)
{
    CHAR_DATA *wch, *wnext;
    MOB_INDEX_DATA *pMob;
    RESET_DATA *pReset, *wReset;
    ROOM_INDEX_DATA *pRoom;
    char buf[MAX_STRING_LENGTH];
    int index, mcount, rcount, iHash, i;
    bool foundmob = FALSE;
    bool foundobj = FALSE;


    EDIT_MOB(ch, pMob);

    if (!pMob)
    {
	send_to_char("No such mobile.\n\r", ch);
	return FALSE;
    }

    index = pMob->vnum;

    SET_BIT(pMob->area->area_flags, AREA_CHANGED);

    if (top_vnum_mob == index)
	for (i = 1; i < index; i++)
	    if (get_mob_index(i))
		top_vnum_mob = i;

    top_mob_index--;

    /* Now crush all resets and take out mobs while were at it */
    rcount = 0;
    mcount = 0;

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
	for (pRoom = room_index_hash[iHash]; pRoom; pRoom = pRoom->next)
	{
	    LIST_FOREACH_SAFE(wch, &pRoom->people, room_link, wnext)
	    {
		if (wch->pIndexData == pMob)
		{
		    extract_char(wch, TRUE);
		    mcount++;
		}
	    }

	    for (pReset = pRoom->reset_first; pReset; pReset = wReset)
	    {
		wReset = pReset->next;
		switch (pReset->command)
		{
		case 'M':
		    if (pReset->arg1 == index)
		    {
			foundmob = TRUE;

			unlink_reset(pRoom, pReset);
			free_reset_data(pReset);

			rcount++;
			SET_BIT(pRoom->area->area_flags, AREA_CHANGED);
		    }
		    else
			foundmob = FALSE;

		    break;
		case 'E':
		case 'G':
		    if (foundmob)
		    {
			foundobj = TRUE;

			unlink_reset(pRoom, pReset);
			free_reset_data(pReset);

			rcount++;
			SET_BIT(pRoom->area->area_flags, AREA_CHANGED);
		    }
		    else
			foundobj = FALSE;

		    break;
		case 'O':
		    foundobj = FALSE;
		    break;
		case 'P':
		    if (foundobj && foundmob)
		    {
			unlink_reset(pRoom, pReset);
			free_reset_data(pReset);

			rcount++;
			SET_BIT(pRoom->area->area_flags, AREA_CHANGED);
		    }
		}
	    }
	}
    }

    unlink_mob_index(pMob);

    pMob->area = NULL;
    pMob->vnum = 0;

    free_mob_index(pMob);
    edit_done(ch);

    sprintf(buf, "Removed mobile vnum {C%d{x and {C%d{x resets.\n\r",
	    index, rcount);
    send_to_char(buf, ch);
    sprintf(buf, "{C%d{x mobiles were extracted from the mud.\n\r", mcount);
    send_to_char(buf, ch);

    return TRUE;
}

REDIT(redit_delete)
{
    ROOM_INDEX_DATA *pRoom, *pRoom2;
    RESET_DATA *pReset;
    EXIT_DATA *ex;
    OBJ_DATA *Obj, *obj_next;
    CHAR_DATA *wch, *wnext;
    EXTRA_DESCR_DATA *pExtra;
    char buf[MAX_STRING_LENGTH];
    int index, i, iHash, rcount, ecount, mcount, ocount, edcount;

    EDIT_ROOM(ch, pRoom);

    if (!pRoom)
    {
	send_to_char("No such room.\n\r", ch);
	return FALSE;
    }

    index = pRoom->vnum;

    /* Move the player out of the room. */
    if (ch->in_room->vnum == index)
    {
	send_to_char("Moving you out of the room you are deleting.\n\r", ch);
	if (ch->fighting != NULL)
	    stop_fighting(ch, TRUE);

	char_from_room(ch);
	char_to_room(ch, get_room_index(ROOM_VNUM_LIMBO), FALSE); /* limbo */
    }

    SET_BIT(pRoom->area->area_flags, AREA_CHANGED);

    /* Count resets. They are freed by free_room_index. */
    rcount = 0;

    for (pReset = pRoom->reset_first; pReset; pReset = pReset->next)
    {
	rcount++;
    }

    /* Now contents */
    ocount = 0;
    for (Obj = pRoom->contents; Obj; Obj = obj_next)
    {
	obj_next = Obj->next_content;

	extract_obj(Obj, TRUE, TRUE);
	ocount++;
    }

    /* Now PCs and Mobs */
    mcount = 0;
    LIST_FOREACH_SAFE(wch, &pRoom->people, room_link, wnext)
    {
	if (IS_NPC(wch))
	{
	    extract_char(wch, TRUE);
	    mcount++;
	}
	else
	{
	    send_to_char("Эта комната удаляется. Перемещаемся...\n\r", ch);
	    if (wch->fighting != NULL)
		stop_fighting(wch, TRUE);

	    char_from_room(wch);

	    /* Midgaard Temple */
	    char_to_room(wch, get_recall(wch) , TRUE); 
	}
    }

    /* unlink all exits to the room. */
    ecount = 0;
    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
	for (pRoom2 = room_index_hash[iHash]; pRoom2; pRoom2 = pRoom2->next)
	{
	    for (i = 0; i <= MAX_DIR; i++)
	    {
		if (!(ex = pRoom2->exit[i]))
		    continue;

		if (pRoom2 == pRoom)
		{
		    /* these are freed by free_room_index */
		    ecount++;
		    continue;
		}

		if (ex->u1.to_room == pRoom)
		{
		    free_exit(pRoom2->exit[i]);
		    pRoom2->exit[i] = NULL;
		    SET_BIT(pRoom2->area->area_flags, AREA_CHANGED);
		    ecount++;
		}
	    }
	}
    }

    /* count extra descs. they are freed by free_room_index */
    edcount = 0;
    for (pExtra = pRoom->extra_descr; pExtra; pExtra = pExtra->next)
	edcount++;

    if (top_vnum_room == index)
	for (i = 1; i < index; i++)
	    if (get_room_index(i))
		top_vnum_room = i;

    top_room--;

    unlink_room_index(pRoom);

    pRoom->area = NULL;
    pRoom->vnum = 0;

    free_room_index(pRoom);

    edit_done(ch);
    /* Na na na na! Hey Hey Hey, Good Bye! */

    sprintf(buf, "Removed room vnum {C%d{x, %d resets, %d extra "
	    "descriptions and %d exits.\n\r",
	    index, rcount, edcount, ecount);
    send_to_char(buf, ch);
    sprintf(buf, "{C%d{x objects and {C%d{x mobiles were extracted "
	    "from the room.\n\r", ocount, mcount);
    send_to_char(buf, ch);

    return TRUE;
}

MEDIT(medit_autoset)
{
    MOB_INDEX_DATA *pMob;
    char temp[500];
    int dice, size, bonus;
    int ac_n, ac_x;
    char arg[MAX_INPUT_LENGTH];
    int hard;

    EDIT_MOB(ch, pMob);

    if (pMob->level < 1)
    {
	send_to_char("Set a level on the mob first!!!\n\r", ch);
	return FALSE;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0' || !str_cmp(arg, "normal"))
	hard = 1;
    else if (!str_cmp(arg, "easy"))
	hard = 0;
    else if (!str_cmp(arg, "hard"))
	hard = 2;
    else
    {
	send_to_char("Syntax: autoset [normal|easy|hard]\n\r", ch);
	return FALSE;
    }

    /* adjust these next 2 lines to affect ACs */
    switch (hard)
    {
    case 0:
	ac_n = 99 - (pMob->level * 6.37) - ((pMob->level/10)^2);
	ac_x = 99 - (pMob->level * 4.27) - ((pMob->level/10)^2);
	break;
    default: 
	ac_n = 95 - (pMob->level * 6.67) - ((pMob->level/10)^2);
	ac_x = 95 - (pMob->level * 4.57) - ((pMob->level/10)^2);
	break;
    case 2:
	ac_n = 88 - (pMob->level * 7.05) - ((pMob->level/10)^2);
	ac_x = 88 - (pMob->level * 5.02) - ((pMob->level/10)^2);
	break;
    }
    pMob->ac[AC_PIERCE] = ac_n;
    pMob->ac[AC_BASH]   = ac_n;
    pMob->ac[AC_SLASH]  = ac_n;
    pMob->ac[AC_EXOTIC] = ac_x;
    send_to_char("AC Values set.\n\r", ch);

    dice = pMob->level;

    switch (hard)
    {
    case 0:
	size = 9;
	bonus = 60;
	break;
    default:
	size = 10;
	bonus = 85;
	break;
    case 2:
	size = 11;
	bonus = 95;
	break;
    }
    sprintf(temp, "%dd%d + %d", dice, size, bonus);
    medit_manadice(ch, temp);


    switch (hard)
    {
    default:
	dice = pMob->level/3;
	if (dice < 1)
	    dice = 1;
	size = (.87 + pMob->level/dice);
	if (size < 2)
	    size = 2;
	bonus = (5.5 + pMob->level/2);
	break;
    case 2:
	dice = pMob->level/3;
	if (dice < 1)
	    dice = 1;
	dice++;
	size = (.87 + pMob->level/dice);
	if (size < 2)
	    size = 2;
	bonus = (7.5 + pMob->level/1.5);
	break;
    case 0:
	dice = pMob->level/3 * .95;
	if (dice < 1)
	    dice = 1;
	size = (.87 + pMob->level/dice) * .95;
	if (size < 2)
	    size = 2;
	bonus = (2.5 + pMob->level/2.1);
	break;
    }

    sprintf(temp, "%dd%d + %d", dice, size, bonus);
    medit_damdice(ch, temp);

    switch (hard)
    {
    default:
	bonus = (pMob->level/53 + 1) * ((pMob->level * 10)+(pMob->level/10))+1; 
	size = pMob->level/3;
	if (size < 2)
	    size = 2;
	dice = (pMob->level + .5) * 2 + .2;
	if (dice < 1)
	    dice = 1;
	break;
    case 2:
	bonus = (pMob->level/53 + 1) * ((pMob->level * 10)+(pMob->level/10))+1;
	size = pMob->level/3;
	if (size < 2)
	    size = 2;
	size++;
	dice = (pMob->level + .6) * 2.05 + .5;
	if (dice < 1)
	    dice = 1;
	break;
    case 0:
	bonus = (pMob->level/59 + 1) * ((pMob->level * 9) + (pMob->level/11))+1;
	size = pMob->level/3;
	if (size < 2)
	    size = 2;
	dice = (pMob->level + .5) * 2 + .2;
	if (dice < 1)
	    dice = 1;
	break;
    }

    sprintf(temp, "%dD%d + %d", dice, size, bonus);
    medit_hitdice(ch, temp);

    sprintf(temp, "%d", pMob->level);
    medit_hitroll(ch, temp);

    send_to_char(" Values set, check for accuracy.\n\r", ch);
    return TRUE;
}

OEDIT(oedit_autoweapon)
{
    OBJ_INDEX_DATA *pObj;
    AFFECT_DATA *pAf;
    int dice, size, bonus;
    double avg;

    EDIT_OBJ(ch, pObj);
    if (pObj->item_type != ITEM_WEAPON)
    {
	send_to_char("Autoweapn only works on weapons...\n\r", ch);
	return FALSE;
    }

    if (pObj->level < 1)
    {
	send_to_char("Autoweapon requires a level to be set "
		     "on the weapon first.\n\r", ch);
	return FALSE;
    }

    bonus = UMAX(0, pObj->level/10 - 1);
    /* adjust this next line to change the avg dmg your weapons will get! */
    avg = (pObj->level * .55);
    dice = (pObj->level/10 + 1);
    size = dice/2;

    /* loop through dice sizes until we find that the Next dice size's avg
     will be too high... ie, find the "best fit" */
    for (size=dice/2 ; dice * (size +2)/2 < avg ; size++)
	;

    dice = UMAX(1, dice);
    size = UMAX(2, size);

    switch (pObj->value[0])
    {
    default:
    case WEAPON_EXOTIC:
    case WEAPON_SWORD:
	break;
    case WEAPON_DAGGER:
	dice = UMAX(1, dice - 1);
	size = UMAX(2, size - 1);
	break;
    case WEAPON_SPEAR:
    case WEAPON_POLEARM:
	size++;
	break;
    case WEAPON_MACE:
    case WEAPON_AXE:
	size = UMAX(2,size - 1);
	break;
    case WEAPON_FLAIL:
    case WEAPON_WHIP:
	dice = UMAX(1, dice - 1);
	break;
    }
    dice = UMAX(1, dice);
    size = UMAX(2, size);


    pObj->cost = 25 * (size * (dice + 1)) + 20 * bonus + 20 * pObj->level;
    pObj->weight = pObj->level + 1;
    pObj->value[1] = dice;
    pObj->value[2] = size;

    if (bonus > 0) 
    {
	pAf             =   new_affect();
	pAf->location   =   APPLY_DAMROLL;
	pAf->modifier   =   bonus;
	pAf->where	=   TO_OBJECT;
	pAf->type       =   -1;
	pAf->duration   =   -1;
	pAf->bitvector  =   0;
	pAf->level      =   pObj->level;
	pAf->next       =   pObj->affected;
	pAf->prev	=   NULL;
	if (pObj->affected)
	    pObj->affected->prev = pAf;
	pObj->affected  =   pAf;

	pAf             =   new_affect();
	pAf->location   =   APPLY_HITROLL;
	pAf->modifier   =   bonus;
	pAf->where	=   TO_OBJECT;
	pAf->type       =   -1;
	pAf->duration   =   -1;
	pAf->bitvector  =   0;
	pAf->level      =   pObj->level;
	pAf->next       =   pObj->affected;
	pAf->prev	=   NULL;
	if (pObj->affected)
	    pObj->affected->prev = pAf;
	pObj->affected  =   pAf;
    }

    send_to_char("Experimental values set on weapon...\n\r", ch);
    return TRUE;
}

OEDIT(oedit_autoarmor)
{
    OBJ_INDEX_DATA *pObj;
    int size;

    EDIT_OBJ(ch, pObj);
    if (pObj->item_type != ITEM_ARMOR)
    {
	send_to_char("AutoArmor only works on Armor ...\n\r", ch);
	return FALSE;
    }

    if (pObj->level < 1)
    {
	send_to_char("AutoArmor requires a level to be set on "
		     "the armor first.\n\r", ch);
	return FALSE;
    }

    size = UMAX(1, pObj->level/2.8 + 1);
    pObj->weight = pObj->level + 1;
    pObj->cost = pObj->level^2 * 2;	
    pObj->value[0] = size;
    pObj->value[1] = size;
    pObj->value[2] = size;
    pObj->value[3] = (size - 1);

    send_to_char("AutoArmor has set experimental values for AC.\n\r", ch);
    return TRUE;
}

void save_limits(void);

OEDIT(oedit_limit)
{
    OBJ_INDEX_DATA *pObj;
    char arg[MAX_INPUT_LENGTH];
    int n, i;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	do_limits(ch, "");
	return FALSE;
    }

    EDIT_OBJ(ch, pObj);

    if (!is_number(arg))
    {
	send_to_char("Необходимо указать максимальное количество этих вещей в мире.\n\r", ch);
	return TRUE;
    }

    n = atoi(arg);

    if (n <= 0)
    {
	for (i=0; (i<MAX_LIMITS && limits[i]>=0); i+=3)
	    if (limits[i] == pObj->vnum)
	    {
		for (n = i; (n<MAX_LIMITS && limits[n]>=0); n+=3)
		{
		    limits[n] = limits[n + 3];
		    limits[n + 1] = limits[n + 4];
		    limits[n + 2] = limits[n + 5];
		}
		send_to_char("Вещь удалена из списка лимитов.\n\r", ch);
		save_limits();
		return TRUE;
	    }

	send_to_char("Это и так не лимитная вещь.\n\r", ch);
	return TRUE;
    }

    argument = one_argument(argument, arg);

    if (IS_SET(pObj->extra_flags, ITEM_FORQUEST) && str_cmp(arg, "set"))
    {
	send_to_char("Эта вещь участвует в квесте! Для принудительной установки лимита введите limit <max> set.\n\r", ch);
	return TRUE;
    }

    for (i=0; (i < MAX_LIMITS && limits[i]>=0); i+=3)
	if (limits[i] == pObj->vnum)
	{
	    limits[i+2] = n;
	    send_to_char("Ok.\n\r", ch);
	    save_limits();
	    return TRUE;
	}

    if (i >= MAX_LIMITS)
    {
	send_to_char("Таблица лимитов заполнена. Требуется либо увеличить параметр MAX_LIMITS, либо удалить какой-нибудь лимит из списка.\n\r", ch);
	return TRUE;
    }

    limits[i] = pObj->vnum;
    limits[i+1] = pObj->count;
    limits[i+2] = n;
    limits[i+3] = -1;
    save_limits();
    send_to_char("Вещь добавлена в список лимитов.\n\r", ch);
    return TRUE;
}


MEDIT(medit_sector)
{
    MOB_INDEX_DATA *pMob;
    int value = -1, i;
    bool fAll = FALSE, fNone = FALSE;

    EDIT_MOB(ch, pMob);

    if (!str_cmp(argument, "all"))
	fAll = TRUE;
    else if (!str_cmp(argument, "none"))
	fNone = TRUE;
    else if (argument[0] == '\0'
	     || (value = flag_value(sector_flags, argument)) == NO_FLAG)
    {
	send_to_char("Syntax: areal [sector]\n\r"
		     "        areal all\n\r"
		     "        areal none\n\r"    
		     "Try ? sector\n\r", ch);
	return FALSE;
    }

    for (i = 0; i < SECT_MAX; i++)
	if (fAll)
	    pMob->areal[i] = TRUE;
	else if (fNone)
	    pMob->areal[i] = FALSE;
	else if (i == value)
	{
	    pMob->areal[i] = !pMob->areal[i];
	    break;
	}

    send_to_char("Areal changed.\n\r", ch);

    return TRUE;
}

OEDIT(oedit_require)
{
    OBJ_INDEX_DATA *pObj;
    char arg[MAX_INPUT_LENGTH];
    int value, n;

    EDIT_OBJ(ch, pObj);

    argument = one_argument(argument, arg);

    if ((value = flag_value(attr_flags, arg)) == NO_FLAG)
    {
	send_to_char("Unknown stat.\n\r"
		     "Try ? stat\n\r", ch);
	return FALSE;
    }

    one_argument(argument, arg);

    if (!is_number(arg) || (n = atoi(arg)) < 0 || n > MAX_STAT)
    {
	send_to_char("Необходим числовой аргумент, лежащий в разумных пределах.\n\r", ch);
	return FALSE;
    }

    pObj->require[value] = n;
    send_to_char("Ok.\n\r", ch);

    return TRUE;
}

MEDIT(medit_message)
{
    MOB_INDEX_DATA *pMob;
    char arg[MIL];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || (str_prefix(arg, "arrive") && str_prefix(arg, "gone")))
    {
	send_to_char("message [arrive|gone] <message>\n\r", ch);
	return FALSE;
    }

    EDIT_MOB(ch, pMob);

    if (!str_prefix(arg, "arrive"))
    {
	free_string(pMob->msg_arrive);
	pMob->msg_arrive = str_dup(argument);
    }
    else
    {
	free_string(pMob->msg_gone);
	pMob->msg_gone = str_dup(argument);
    }

    send_to_char("Ok.\n\r", ch);

    return TRUE;
}

MEDIT(medit_chat)
{
    MOB_INDEX_DATA *pMob;
    char arg[MIL], buf[MSL];
    int vnum;
    int i, j;

    if (argument[0] == '\0')
    {
	send_to_char("Usage: chat [<chatbook vnum> [[<chatbook vnum>] ......] | 0]\n\r", ch);
	return FALSE;
    }

    EDIT_MOB(ch, pMob);

    for(i = 0, argument = one_argument(argument, arg); i < MAX_CHATBOOKS && arg[0] != '\0';
	argument = one_argument(argument, arg))
    {

	if (!is_number(arg))
	    continue;

	if ((vnum = atoi(arg)) != 0 && get_chat_index(vnum) == NULL)
	{
	    sprintf(buf, "Cannot find any chatbook with vnum %d.\n\r", vnum);
	    send_to_char(buf, ch);
	    continue;
	}

	pMob->chat_vnum[i++] = vnum;
    }

    for(j = i; j < MAX_CHATBOOKS; j++)
	pMob->chat_vnum[j] = 0;

    send_to_char("Ok.\n\r", ch);
    return TRUE;
}

MEDIT(medit_clan)
{
    MOB_INDEX_DATA *pMob;
    int i;

    if (argument[0] == '\0')
    {
	send_to_char("Usage: clan <name>\n\r"
		     "       clan clear\n\r" , ch);
	return FALSE;
    }

    if (!str_cmp(argument, "clear") || !str_cmp(argument, "очистить"))
    {
	i = 0;
	send_to_char("Clan cleared.\n\r", ch);
    }
    else if ((i = clan_lookup(argument)) == 0)
    {
	send_to_char("Нет таких кланов.\n\r", ch);
	return FALSE;
    }
    else
	send_to_char("Clan set.\n\r", ch);

    EDIT_MOB(ch, pMob);

    pMob->clan = i;

    return TRUE;
}

MEDIT(medit_rating)
{
    MOB_INDEX_DATA *pMob;

    if (argument[0] == '\0' || !is_number(argument))
    {
	send_to_char("Usage: rating <number>\n\r", ch);
	return FALSE;
    }

    EDIT_MOB(ch, pMob);

    pMob->rating = atoi(argument);
    send_to_char("Rating set.\n\r", ch);

    return TRUE;
}

MEDIT(medit_comments)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    free_string(pMob->comments);
    pMob->comments = str_dup(argument);

    send_to_char("Ok.\n\r", ch);

    return TRUE;
}

OEDIT(oedit_comments)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    free_string(pObj->comments);
    pObj->comments = str_dup(argument);

    send_to_char("Ok.\n\r", ch);

    return TRUE;
}

REDIT(redit_comments)
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM(ch, pRoom);

    free_string(pRoom->comments);
    pRoom->comments = str_dup(argument);

    send_to_char("Ok.\n\r", ch);

    return TRUE;
}

CEDIT(cedit_show)
{
    CHAT_KEYS *pCkey;
    CHAT_KEY *ck;
    char  buf[MSL];
    BUFFER *output = new_buf();
    int rx = 0, rp, num = -1;

    one_argument(argument, buf);

    if (buf[0] != '\0')
    {
	if (!is_number(buf))
	{
	    send_to_char("Параметр должен быть числовым.\n\r", ch);
	    free_buf(output);
	    return FALSE;
	}

	num = atoi(buf);
    }

    EDIT_CKEY(ch, pCkey);

    for (ck = pCkey->list; ck != NULL; ck = ck->next)
    {
	if (num == rx || num == -1)
	{
	    REPLY *r;

	    sprintf(buf, "REGEXP #%-3d: %s\n\r", rx, ck->regexp);

	    add_buf(output, buf);

	    add_buf(output, "#  W Race     Sex     Reply\n\r\
		    ----------------------------------------------------------------------------------------\n\r");

	    rp = 0;

	    for(r = ck->replys; r != NULL; r = r->next)
	    {
		sprintf(buf, 
			"%-2d %d %-8s %-7s %s\n",
			rp++,
			r->weight, 
			r->race > 0 ? race_table[r->race].name : "", 
			r->sex >= 0 ? sex_table[r->sex].name : "", 
			r->sent);
		add_buf(output, buf);
	    }
	    add_buf(output, "\n\r");
	}
	rx++;
    }

    page_to_char(output->string, ch);
    free_buf(output);
    return TRUE;
}



CEDIT(cedit_create)
{
    CHAT_KEYS *k;
    int  value;
    AREA_DATA *pArea;

    value = atoi(argument);
    if (argument[0] == '\0' || value == 0)
    {
	send_to_char("Syntax:  cedit create [vnum]\n\r", ch);
	return FALSE;
    }

    pArea = get_vnum_area(value);

    if (!pArea)
    {
	send_to_char("CEdit:  That vnum is not assigned an area.\n\r", ch);
	return FALSE;
    }

    if (!IS_BUILDER(ch, pArea))
    {
	send_to_char("CEdit:  Vnum in an area you cannot build in.\n\r", ch);
	return FALSE;
    }

    if (get_chat_index(value))
    {
	send_to_char("CEdit:  Chatbook vnum already exists.\n\r", ch);
	return FALSE;
    }

    k			= new_chatkeys();
    k->vnum		= value;
    k->area		= pArea;

    k->next		= chatkeys_list;
    chatkeys_list	= k;

    ch->desc->pEdit	= (void *) k;

    send_to_char("ChatBook Created.\n\r", ch); 
    return TRUE;
}



CEDIT(cedit_regexp)
{
    CHAT_KEYS *pCkey;
    CHAT_KEY *ck, *k;
    char  buf[MIL];
    int rx = 0, num;
    bool new_flag = FALSE, delete_flag = FALSE;

    argument = one_argument(argument, buf);

    if (buf[0] == '\0')
    {
	send_to_char("Usage: regexp [create|new|delete] <regexp #> <regexp text>\n\rSee also: ? chat\n\r", ch);
	return FALSE;
    }

    if (!str_cmp(buf, "create") || !str_cmp(buf, "new"))
    {
	new_flag = TRUE;
	argument = one_argument(argument, buf);
    }

    if (!str_cmp(buf, "delete"))
    {
	delete_flag = TRUE;
	argument = one_argument(argument, buf);
    }

    if (!is_number(buf) || (num = atoi(buf)) < 0)
    {
	send_to_char("Номер регулярного выражения должен быть числовым и быть больше 0!\n\r", ch);
	return FALSE;
    }

    EDIT_CKEY(ch, pCkey);

    if (new_flag || delete_flag)
	num--;

    if (num == -1)
    {
	if (new_flag)
	{
	    k = new_chatkey();

	    k->next = pCkey->list;
	    pCkey->list = k;
	    k->regexp = str_dup(argument);
	}
	else
	{
	    CHAT_KEY *tmp;

	    if ((tmp = pCkey->list) != NULL)
	    {
		pCkey->list = pCkey->list->next;

		free_chatkey(tmp);
		send_to_char("Ok.\n\r", ch);
		return TRUE;
	    }
	    else
	    {
		send_to_char("Nothing to delete.\n\r", ch);
		return FALSE;
	    }
	}
    }
    else
    {
	for (ck = pCkey->list; ck != NULL; ck = ck->next)
	    if (num == rx++)
	    {
		if (new_flag)
		{
		    k = new_chatkey();

		    k->next = ck->next;
		    ck->next = k;

		    k->regexp = str_dup(argument);
		}
		else if (delete_flag)
		{
		    if (ck->next)
		    {
			CHAT_KEY *tmp = ck->next;

			ck->next = ck->next->next;

			free_chatkey(tmp);
			send_to_char("Ok.\n\r", ch);
			return TRUE;
		    }
		    else
			ck = NULL;
		}
		else
		{
		    free_string(ck->regexp);
		    ck->regexp = str_dup(argument);
		}

		break;
	    }

	if (ck == NULL)
	{
	    send_to_char("Такого номера регулярного выражения нет!\n\r", ch);
	    return FALSE;
	}
    }

    sprintf(buf, "%d", new_flag ? num + 1 : num);
    cedit_show(ch, buf);

    return TRUE;
}

CEDIT(cedit_reply)
{
    CHAT_KEYS *pCkey;
    CHAT_KEY *ck;
    char  buf[MIL];
    int rx = 0, rp = 0, rx_num, rp_num;
    bool delete_flag = FALSE;

    argument = one_argument(argument, buf);

    if (buf[0] == '\0')
    {
	send_to_char("Usage: reply <regexp #> [<reply #>|new|create|delete] | [weight|sex|race|text] <value>\n\rSee also: ? chat\n\r", ch);
	return FALSE;
    }

    if (!is_number(buf))
    {
	send_to_char("Номер регулярного выражения должен быть числовым!\n\r", ch);
	return FALSE;
    }

    rx_num = atoi(buf);

    argument = one_argument(argument, buf);

    if (!str_cmp(buf, "new") || !str_cmp(buf, "create"))
    {
	rp_num = -1;
    }
    else 
    {
	if (!str_cmp(buf, "delete"))
	{
	    delete_flag = TRUE;
	    argument = one_argument(argument, buf);
	}

	if (!is_number(buf))
	{
	    send_to_char("Номер ответа должен быть числовым!\n\r", ch);
	    return FALSE;
	}

	rp_num = atoi(buf);
    }

    EDIT_CKEY(ch, pCkey);

    for (ck = pCkey->list; ck != NULL; ck = ck->next)
	if (rx_num == rx++)
	{
	    REPLY *r;

	    if (rp_num == -1)
	    {
		r = new_reply();

		r->next = ck->replys;
		ck->replys = r;
	    }
	    else
	    {
		for(r = ck->replys; r != NULL; r = r->next)
		    if (rp++ == rp_num)
			break;

		if (r == NULL)
		{
		    send_to_char("Такого номера ответа нет!\n\r", ch);
		    return FALSE;
		}

		if (delete_flag)
		{
		    REPLY *tmp;

		    if (r == ck->replys)
		    {
			ck->replys = ck->replys->next;
		    }
		    else
			for(tmp = ck->replys; tmp != NULL; tmp = tmp->next)
			    if (tmp->next == r)
			    {
				tmp->next = r->next;
				break;
			    }

		    free_reply(r);
		}
		else
		{
		    argument = one_argument(argument, buf);

		    if (!str_prefix(buf, "weight"))
		    {
			if (!is_number(argument) || (rp = atoi(argument)) < 0 || rp > 9)
			{
			    send_to_char("Вес должен быть числовым и быть от 0 до 9!\n\r", ch);
			    return FALSE;
			}

			r->weight = rp;
		    }
		    else if (!str_prefix(buf, "race"))
			r->race = race_lookup(argument);
		    else if (!str_prefix(buf, "sex"))
			r->sex = sex_lookup(argument);
		    else if (!str_prefix(buf, "text"))
			r->sent = str_dup(argument);
		    else
		    {
			cedit_reply(ch, "");
			return FALSE;
		    }
		}
	    }

	    break;
	}

    if (ck == NULL)
    {
	send_to_char("Такого номера регулярного выражения нет!\n\r", ch);
	return FALSE;
    }

    sprintf(buf, "%d", rx_num);
    cedit_show(ch, buf);

    return TRUE;
}

CEDIT(cedit_delete)
{
    CHAT_KEYS *pCkey, *tmp;

    EDIT_CKEY(ch, pCkey);

    if (pCkey == chatkeys_list)
    {
	chatkeys_list = chatkeys_list->next;
    }
    else
	for(tmp = chatkeys_list; tmp != NULL; tmp = tmp->next)
	    if (tmp->next == pCkey)
	    {
		tmp->next = pCkey->next;
		break;
	    }

    free_chatkeys(pCkey);
    edit_done(ch);
    send_to_char("Ok.\n\r", ch);
    return TRUE;
}

REDIT(redit_levels)
{
    ROOM_INDEX_DATA *pRoom;
    int16_t min, max;
    char arg1[MIL], arg2[MIL];

    EDIT_ROOM(ch, pRoom);

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (!is_number(arg1) || (min = atoi(arg1)) < 0 || min > MAX_LEVEL
	|| !is_number(arg2) || (max = atoi(arg2)) < 0 || max > MAX_LEVEL)
    {
	printf_to_char("Аргументы дожны быть числовыми и в пределах от 0 до %d.\n\r"
		       "Использование: levels <min_level> <max_level>\n\r",
		       ch, MAX_LEVEL);
	return FALSE;
    }

    if (min > max)
    {
	send_to_char("Минимальный уровень не может быть больше максимального!\n\r", ch);
	return FALSE;
    }

    pRoom->min_level = min;
    pRoom->max_level = max;
    send_to_char("Ok.\n\r", ch);

    return TRUE;
}

OEDIT(oedit_timer)
{
    OBJ_INDEX_DATA *pObj;
    char arg[MIL];
    int16_t tm;

    argument = one_argument(argument, arg);

    if (!is_number(arg) || (tm = atoi(arg)) < 0)
    {
	send_to_char("Аргумент должен быть числовым и больше нуля!\n\r", ch);
	return FALSE;
    }

    EDIT_OBJ(ch, pObj);
    
    pObj->timer = tm;

    send_to_char("Ok.\n\r", ch);

    return TRUE;
}
/* charset=cp1251 */
