/***************************************************************************
 *  File: olc.c                                                            *
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
#include "merc.h"
#include "olc.h"
#include "recycle.h"
#include "config.h"



/*
 * Local functions.
 */
AREA_DATA *get_area_data	args((int vnum));

void return_affects(OBJ_INDEX_DATA *pObj, bool add)
{
    OBJ_DATA *obj;
    int i = 0;

    if (pObj == NULL)
	return;             

    for (obj = object_list; obj != NULL; obj = obj->next)
    {
	if (obj->pIndexData->vnum == pObj->vnum)
	{
	    i++;

	    if (!obj->enchanted)
	    {
		AFFECT_DATA *paf, *paf_next;

		if (add)
		{
		    if (pObj->edited == 0)
		        for (paf = pObj->affected; paf != NULL; paf = paf->next)
			    affect_to_obj(obj, paf);
		}
		else
		{
		    if (pObj->edited == 1)
		    {
		        int wl = obj->wear_loc;
		        bool ar = (obj->carried_by != NULL && wl != WEAR_NONE);

		        if (ar)
			    unequip_char(obj->carried_by, obj, FALSE); 

			pObj->edited = 0;

		        for (paf = obj->affected; paf != NULL; paf = paf_next)
		        {
			    paf_next = paf->next; 
			    affect_remove_obj(obj, paf);
		        }
		        obj->affected = NULL;
		        
		        if (ar)
			    equip_char(obj->carried_by, obj, wl); 

			pObj->edited = 1;
		    }
		}
	    }	
	}

	if (i == obj->pIndexData->count)
	    break;
    }

    pObj->edited += add ? 1 : -1;

    return;
}


/* Executed from comm.c.  Minimizes compiling when changes are made. */
bool run_olc_editor(DESCRIPTOR_DATA *d)
{
    bool log = FALSE;
    CHAR_DATA *ch = d->character;

    switch (d->editor)
    {
    case ED_AREA:
	log = aedit(ch, d->incomm);
	break;
    case ED_ROOM:
	log = redit(ch, d->incomm);
	break;
    case ED_OBJECT:
	log = oedit(ch, d->incomm);
	break;
    case ED_MOBILE:
	log = medit(ch, d->incomm);
	break;
    case ED_MPCODE:
    	log = mpedit(ch, d->incomm);
    	break;
    case ED_OPCODE:
	log = opedit(ch, d->incomm);
	break;
    case ED_RPCODE:
	log = rpedit(ch, d->incomm);
	break;
    case ED_HELP:
	log = hedit(ch, d->incomm);
	break;
    case ED_SKILL:
	log = skedit(ch, d->incomm);
	break;
    case ED_CLASS:
	log = cledit(ch, d->incomm);
	break;
    case ED_GROUP:
	log = gredit(ch, d->incomm);
	break;
    case ED_CHAT:
	log = cedit(ch, d->incomm);
	break;
    case ED_RACE:
	log = raedit(ch, d->incomm);
	break;

    default:
	return FALSE;
    }
    
    if (cfg.log_all && log)
    {
        char buf[MAX_STRING_LENGTH];

        sprintf(buf, "[*****] [OLC] Log %s: %s", ch->name, d->incomm);
        log_string(buf);

#if !defined(ONEUSER)
        if (IS_SET(ch->act, PLR_LOG))
        {
    	    char logname[50], *tm;

	    tm = ctime(&current_time);
	    tm[strlen(tm) - 1] = '\0';

	    sprintf(buf, "%s :: [*****] [OLC] %s in room: [%d]\n", tm, d->incomm,
		    ch->in_room != NULL ? ch->in_room->vnum : 0);
	    sprintf(logname, "../log/%s", ch->name);
	    recode(logname, CODEPAGE_ALT, RECODE_OUTPUT|RECODE_NOANTITRIGGER);
	    _log_string(buf, logname);
	}
#endif
    }

    return TRUE;
}



char *olc_ed_name(CHAR_DATA *ch)
{
    static char buf[10];
    
    buf[0] = '\0';
    switch (ch->desc->editor)
    {
    case ED_AREA:
	sprintf(buf, "AEdit");
	break;
    case ED_ROOM:
	sprintf(buf, "REdit");
	break;
    case ED_OBJECT:
	sprintf(buf, "OEdit");
	break;
    case ED_MOBILE:
    	sprintf(buf, "MEdit");
	break;
    case ED_MPCODE:
    	sprintf(buf, "MPEdit");
	break;
    case ED_OPCODE:
	sprintf(buf, "OPEdit");
	break;
    case ED_RPCODE:
	sprintf(buf, "RPEdit");
	break;
    case ED_HELP:
	sprintf(buf, "HEdit");
	break;
    case ED_SKILL:
	sprintf(buf, "SkEdit");
	break;
    case ED_CLASS:
	sprintf(buf, "ClEdit");
	break;
    case ED_GROUP:
	sprintf(buf, "GrEdit");
	break;
    case ED_CHAT:
	sprintf(buf, "CEdit");
	break;
    case ED_RACE:
	sprintf(buf, "RaEdit");
	break;
    default:
	sprintf(buf, " ");
	break;
    }
    return buf;
}



char *olc_ed_vnum(CHAR_DATA *ch)
{
    AREA_DATA *pArea;
    ROOM_INDEX_DATA *pRoom;
    OBJ_INDEX_DATA *pObj;
    MOB_INDEX_DATA *pMob;
    PROG_CODE *pMprog;
    PROG_CODE *pOprog;
    PROG_CODE *pRprog;
    HELP_DATA *pHelp;
    CHAT_KEYS *pCkey;
    struct race_type *race;
    static char buf[10];
	
    buf[0] = '\0';
    switch (ch->desc->editor)
    {
    case ED_AREA:
	pArea = (AREA_DATA *)ch->desc->pEdit;
	snprintf(buf, sizeof(buf), "%d", pArea ? pArea->vnum : 0);
	break;
    case ED_ROOM:
	pRoom = ch->in_room;
	snprintf(buf, sizeof(buf), "%d", pRoom ? pRoom->vnum : 0);
	break;
    case ED_OBJECT:
	pObj = (OBJ_INDEX_DATA *)ch->desc->pEdit;
	snprintf(buf, sizeof(buf), "%d", pObj ? pObj->vnum : 0);
	break;
    case ED_MOBILE:
	pMob = (MOB_INDEX_DATA *)ch->desc->pEdit;
	snprintf(buf, sizeof(buf), "%d", pMob ? pMob->vnum : 0);
	break;
    case ED_MPCODE:
    	pMprog = (PROG_CODE *)ch->desc->pEdit;
    	snprintf(buf, sizeof(buf), "%d", pMprog ? pMprog->vnum : 0);
	break;
    case ED_OPCODE:
	pOprog = (PROG_CODE *)ch->desc->pEdit;
	snprintf(buf, sizeof(buf), "%d", pOprog ? pOprog->vnum : 0);
	break;
    case ED_RPCODE:
	pRprog = (PROG_CODE *)ch->desc->pEdit;
	snprintf(buf, sizeof(buf), "%d", pRprog ? pRprog->vnum : 0);
	break;
    case ED_HELP:
	pHelp = (HELP_DATA *)ch->desc->pEdit;
	snprintf(buf, sizeof(buf), "%s", pHelp->keyword);
	break;
    case ED_CHAT:
	pCkey = (CHAT_KEYS *)ch->desc->pEdit;
	snprintf(buf, sizeof(buf), "%d", pCkey->vnum);
	break;
    case ED_RACE:
	race = (struct race_type *)ch->desc->pEdit;
	snprintf(buf, sizeof(buf), "%s", race->name);

    default:
	snprintf(buf, sizeof(buf), " ");
	break;
    }

    return buf;
}



/*****************************************************************************
 Name:		show_olc_cmds
 Purpose:	Format up the commands from given table.
 Called by:	show_commands(olc_act.c).
 ****************************************************************************/
void show_olc_cmds(CHAR_DATA *ch, const struct olc_cmd_type *olc_table)
{
    char buf  [ MAX_STRING_LENGTH ];
    char buf1 [ MAX_STRING_LENGTH ];
    int  cmd;
    int  col;
 
    buf1[0] = '\0';
    col = 0;
    for (cmd = 0; olc_table[cmd].name != NULL; cmd++)
    {
	sprintf(buf, "%-15.15s", olc_table[cmd].name);
	strcat(buf1, buf);
	if (++col % 5 == 0)
	    strcat(buf1, "\n\r");
    }
 
    if (col % 5 != 0)
	strcat(buf1, "\n\r");

    send_to_char(buf1, ch);
    return;
}



/*****************************************************************************
 Name:		show_commands
 Purpose:	Display all olc commands.
 Called by:	olc interpreters.
 ****************************************************************************/
bool show_commands(CHAR_DATA *ch, char *argument)
{
    switch (ch->desc->editor)
    {
	case ED_AREA:
	    show_olc_cmds(ch, aedit_table);
	    break;
	case ED_ROOM:
	    show_olc_cmds(ch, redit_table);
	    break;
	case ED_OBJECT:
	    show_olc_cmds(ch, oedit_table);
	    break;
	case ED_MOBILE:
	    show_olc_cmds(ch, medit_table);
	    break;
	case ED_MPCODE:
	    show_olc_cmds(ch, mpedit_table);
	    break;
	case ED_OPCODE:
	    show_olc_cmds(ch, opedit_table);
	    break;
	case ED_RPCODE:
	    show_olc_cmds(ch, rpedit_table);
	    break;
	case ED_HELP:
	    show_olc_cmds(ch, hedit_table);
	    break;
	case ED_SKILL:
	    show_olc_cmds(ch, skedit_table);
	    break;
	case ED_CLASS:
	    show_olc_cmds(ch, cledit_table);
	    break;
	case ED_RACE:
	    show_olc_cmds(ch, raedit_table);
	case ED_GROUP:
	    show_olc_cmds(ch, gredit_table);
	    break;

    }

    return FALSE;
}



/*****************************************************************************
 *                           Interpreter Tables.                             *
 *****************************************************************************/
const struct olc_cmd_type aedit_table[] =
{
/*  {   command		function	}, */

    {   "age",		aedit_age	},
    {   "builder",	aedit_builder	}, /* s removed -- Hugin */
    {   "commands",	show_commands	},
/*    {   "create",	aedit_create	}, */
/*    {   "filename",	aedit_file	}, */
    {   "name",		aedit_name	},
/*  {   "recall",	aedit_recall	},   ROM OLC */
    {	"reset",	aedit_reset	},
    {   "security",	aedit_security	},
    {	"show",		aedit_show	},
    {   "vnum",		aedit_vnum	},
    {   "lvnum",	aedit_lvnum	},
    {   "uvnum",	aedit_uvnum	},
    {   "credits",	aedit_credits	},
    {	"flags",	aedit_flags	},

    {   "?",		show_help	},
    {   "version",	show_version	},

    {	NULL,		0,		}
};



const struct olc_cmd_type redit_table[] =
{
/*  {   command		function	}, */

    {   "commands",	show_commands	},
    {   "create",	redit_create	},
    {   "desc",		redit_desc	},
    {   "ed",		redit_ed	},
    {   "format",	redit_format	},
    {   "name",		redit_name	},
    {	"show",		redit_show	},
    {   "heal",		redit_heal	},
    {	"mana",		redit_mana	},
    {   "clan",		redit_clan	},

    {   "north",	redit_north	},
    {   "south",	redit_south	},
    {   "east",		redit_east	},
    {   "west",		redit_west	},
    {   "up",		redit_up	},
    {   "down",		redit_down	},

    /* New reset commands. */
    {	"mreset",	redit_mreset	},
    {	"oreset",	redit_oreset	},
    {	"mlist",	redit_mlist	},
    {	"rlist",	redit_rlist	},
    {	"olist",	redit_olist	},
    {	"mshow",	redit_mshow	},
    {	"oshow",	redit_oshow	},
    {   "owner",	redit_owner	},
    {	"room",		redit_room	},
    {	"sector",	redit_sector	},

    {   "?",		show_help	},
    {   "version",	show_version	},
    {	"addrprog",	redit_addrprog	},
    {	"delrprog",	redit_delrprog	},
    {	"delete",	redit_delete	},
    {	"comments",	redit_comments	},
    {	"levels",	redit_levels	},

    {	NULL,		0,		}
};



const struct olc_cmd_type oedit_table[] =
{
/*  {   command			function	    }, */

    {   "addaffect",		oedit_addaffect	    },
    {	"addapply",		oedit_addapply	    },
    {   "commands",		show_commands	    },
    {   "cost",			oedit_cost	    },
    {   "create",		oedit_create	    },
    {	"clone",		oedit_clone	    },
    {   "delaffect",		oedit_delaffect	    },
    {   "ed",			oedit_ed	    },
    {   "long",			oedit_long	    },
    {   "name",			oedit_name	    },
    {   "short",		oedit_short	    },
    {	"show",			oedit_show	    },
    {   "v0",			oedit_value0	    },
    {   "v1",			oedit_value1	    },
    {   "v2",			oedit_value2	    },
    {   "v3",			oedit_value3	    },
    {   "v4",			oedit_value4	    },  /* ROM */
    {   "weight",		oedit_weight	    },

    {   "extra",		oedit_extra         },  /* ROM */
    {   "wear",			oedit_wear          },  /* ROM */
    {   "type",			oedit_type          },  /* ROM */
    {   "material",		oedit_material      },  /* ROM */
    {   "level",		oedit_level         },  /* ROM */
    {   "condition",		oedit_condition     },  /* ROM */

    {   "?",			show_help	    },
    {   "version",		show_version	    },
    {	"addoprog",		oedit_addoprog	    },
    {	"deloprog",		oedit_deloprog	    },

    {	"delete",		oedit_delete	    },
    {	"autoweapon",		oedit_autoweapon    },
    {	"autoarmor",		oedit_autoarmor	    },
    {	"limit",		oedit_limit	    },

    {	"uncomfortable",	oedit_uncomfortable },
    {	"unusable",		oedit_unusable	    },
    {	"require",		oedit_require	    },
    {	"comments",		oedit_comments	    },
    {	"timer",		oedit_timer	    },

    {	NULL,			0,		    }
};

const struct olc_cmd_type medit_table[] =
{
/*  {   command		function	}, */

    {   "alignment",	medit_align	},
    {   "commands",	show_commands	},
    {   "create",	medit_create	},
    {	"clone",	medit_clone	},
    {   "desc",		medit_desc	},
    {   "level",	medit_level	},
    {   "long",		medit_long	},
    {   "name",		medit_name	},
    {   "saves",	medit_saves	},
	{   "shop",		medit_shop	},
    {   "short",	medit_short	},
    {	"show",		medit_show	},
    {   "spec",		medit_spec	},

    {   "sex",          medit_sex       },  /* ROM */
    {   "act",          medit_act       },  /* ROM */
    {   "affect",       medit_affect    },  /* ROM */
    {   "armor",        medit_ac        },  /* ROM */
    {   "form",         medit_form      },  /* ROM */
    {   "part",         medit_part      },  /* ROM */
    {   "res",          medit_res       },  /* ROM */
    {   "material",     medit_material  },  /* ROM */
    {   "off",          medit_off       },  /* ROM */
    {   "size",         medit_size      },  /* ROM */
    {   "hitdice",      medit_hitdice   },  /* ROM */
    {   "manadice",     medit_manadice  },  /* ROM */
    {   "damdice",      medit_damdice   },  /* ROM */
    {   "race",         medit_race      },  /* ROM */
    {   "position",     medit_position  },  /* ROM */
    {   "wealth",       medit_gold      },  /* ROM */
    {   "hitroll",      medit_hitroll   },  /* ROM */
    {	"damtype",	medit_damtype	},  /* ROM */
    {   "group",	medit_group	},  /* ROM */
    {   "addmprog",	medit_addmprog  },  /* ROM */
    {	"delmprog",	medit_delmprog	},  /* ROM */
    {	"delete",	medit_delete	},  /* ROM */
    {	"autoset",	medit_autoset	},  /* ROM */
    {	"areal",	medit_sector	},  /* Balderdash :-) */
    {	"message",	medit_message	},
    {	"chat",		medit_chat	},
    {	"clan",		medit_clan	},
    {	"rating",	medit_rating	},
    {	"comments",	medit_comments	},

    {   "?",		show_help	},
    {   "version",	show_version	},

    {	NULL,		0,		}
};

const struct olc_cmd_type hedit_table[] =
{
    {	"create",	hedit_create	},
    {	"delete",	hedit_delete	},
    {	"keywords",	hedit_keywords	},
    {	"text",		hedit_text	},
    {	"show",		hedit_show	},
    {	"level",	hedit_level	},

    {	"?",		show_help	},
    {	"version",	show_version	},
    
    {	NULL,		0		}
};

const struct olc_cmd_type cedit_table[] =
{
/*  {   command		function	}, */

    {   "show",		cedit_show	},
    {   "create",	cedit_create	},
    {   "regexp",	cedit_regexp	},
    {   "reply",	cedit_reply	},
    {   "delete",	cedit_delete	},

    {   "?",		show_help	},
    {   "version",	show_version	},

    {	NULL,		0,		}
};

const struct olc_cmd_type raedit_table[] =
{
    {	"show",		raedit_show	},
    {	"name",		raedit_name	},
    {	"mname",	raedit_mname	},
    {	"fname",	raedit_fname	},
    {	"act",		raedit_act	},
    {	"off",		raedit_off	},
    {	"aff",		raedit_aff	},
    {	"form",		raedit_form	},
    {	"parts",	raedit_parts	},
    {	"res",		raedit_res	},
    
    {	"whoname",	raedit_whoname	},
    {	"points",	raedit_points	},
    {	"size",		raedit_size	},
    {	"recall",	raedit_recall	},
    {	"map",		raedit_map	},
    {	"align",	raedit_align	},
    {	"str",		raedit_str	},
    {	"int",		raedit_int	},
    {	"wis",		raedit_wis	},
    {	"dex",		raedit_dex	},
    {	"con",		raedit_con	},
    {	"skill",	raedit_skill	},
    {	"class",	raedit_class	},
    
    {	"?",		show_help	},
    {	"version",	show_version	},
    {	NULL,		0		}
};

const struct olc_cmd_type skedit_table[] =
{
/*  {   command		function	}, */

    {	"show",		skedit_show	},
    {	"name",		skedit_name	},
    {	"rname",	skedit_rname	},
    {	"fun",		skedit_fun	},
    {	"target",	skedit_target	},
    {	"minpos",	skedit_minpos	},
    {	"minmana",	skedit_minmana	},
    {	"beats",	skedit_beats	},
    {	"gsn",		skedit_gsn	},
    {	"damagemsg",	skedit_msgdam	},
    {	"damagegendar",	skedit_genderdam },
    {	"messageoff",	skedit_msgoff	},
    {	"messageobj",	skedit_msgobj	},
    {	"messageroom",	skedit_msgroom	},

    {	NULL,		0,		}
};

const struct olc_cmd_type gredit_table[] =
{
/*  {   command		function	}, */

    {	"show",		gredit_show	},
    {	"name",		gredit_name	},
    {	"rname",	gredit_rname	},
    {	"add",		gredit_add	},
    {	"delete",	gredit_delete	},

    {	NULL,		0,		}
};

const struct olc_cmd_type cledit_table[] =
{
/*  {   command		function	}, */

    {	"show",		cledit_show	},
    {	"name",		cledit_name	},
    {	"wname",	cledit_wname	},
    {	"primary",	cledit_primary	},
    {	"secondary",	cledit_secondary },
    {	"magicuser",	cledit_fmana	},
    {	"sadept",	cledit_skill_adept },
    {	"weapon",	cledit_weapon	},
    {	"minhp",	cledit_hp_min	},
    {	"maxhp",	cledit_hp_max	},
    {	"mtitle",	cledit_mtitle	},
    {	"ftitle",	cledit_ftitle	},
    {	"skills",	cledit_skills	},
    {	"thac0",	cledit_thac0	},
    {	"guilds",	cledit_guilds	},
    {	"align",	cledit_align	},
    {	"basegroup",	cledit_base_group },
    {	"defaultgroup",	cledit_default_group },
    {	"groups",	cledit_groups   },
    {	NULL,		0,		}
};

/*****************************************************************************
 *                          End Interpreter Tables.                          *
 *****************************************************************************/



/*****************************************************************************
 Name:		get_area_data
 Purpose:	Returns pointer to area with given vnum.
 Called by:	do_aedit(olc.c).
 ****************************************************************************/
AREA_DATA *get_area_data(int vnum)
{
    AREA_DATA *pArea;

    for (pArea = area_first; pArea; pArea = pArea->next)
    {
        if (pArea->vnum == vnum)
            return pArea;
    }

    return 0;
}



/*****************************************************************************
 Name:		edit_done
 Purpose:	Resets builder information on completion.
 Called by:	aedit, redit, oedit, medit(olc.c)
 ****************************************************************************/
bool edit_done(CHAR_DATA *ch)
{
    if (ch->desc->editor == ED_OBJECT)
  	return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);

    ch->desc->pEdit = NULL;
    ch->desc->editor = 0;
    return FALSE;
}



/*****************************************************************************
 *                              Interpreters.                                *
 *****************************************************************************/


/* Area Interpreter, called by do_aedit. */
bool aedit(CHAR_DATA *ch, char *argument)
{
    AREA_DATA *pArea;
    char command[MAX_INPUT_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int  cmd;
    int  value;

    EDIT_AREA(ch, pArea);
    smash_tilde(argument);
    strcpy(arg, argument);
    argument = one_argument(argument, command);

    if (!IS_BUILDER(ch, pArea))
    {
	send_to_char("AEdit:  Insufficient security to modify area.\n\r", ch);
	edit_done(ch);
	return FALSE;
    }

    if (!str_cmp(command, "done"))
    {
	edit_done(ch);
	return FALSE;
    }

    if (command[0] == '\0')
    {
	aedit_show(ch, argument);
	return FALSE;
    }

    if ((value = flag_value(area_flags, command)) != NO_FLAG)
    {
	TOGGLE_BIT(pArea->area_flags, value);

	send_to_char("Flag toggled.\n\r", ch);
	return TRUE;
    }

    /* Search Table and Dispatch Command. */
    for (cmd = 0; aedit_table[cmd].name != NULL; cmd++)
    {
	if (!str_prefix(command, aedit_table[cmd].name))
	{
	    if ((*aedit_table[cmd].olc_fun) (ch, argument))
		SET_BIT(pArea->area_flags, AREA_CHANGED);
	    
	    return TRUE;
	}
    }

    /* Default to Standard Interpreter. */
    interpret(ch, arg);
    return FALSE;
}



/* Room Interpreter, called by do_redit. */
bool redit(CHAR_DATA *ch, char *argument)
{
    AREA_DATA *pArea;
    ROOM_INDEX_DATA *pRoom;
    char arg[MAX_STRING_LENGTH];
    char command[MAX_INPUT_LENGTH];
    int  cmd;

    EDIT_ROOM(ch, pRoom);
    pArea = pRoom->area;

    smash_tilde(argument);
    strcpy(arg, argument);
    argument = one_argument(argument, command);

    if (!IS_BUILDER(ch, pArea))
    {
        send_to_char("REdit:  Insufficient security to modify room.\n\r", ch);
	edit_done(ch);
	return FALSE;
    }

    if (!str_cmp(command, "done"))
    {
	edit_done(ch);
	return FALSE;
    }

    if (command[0] == '\0')
    {
	redit_show(ch, argument);
	return FALSE;
    }

    /* Search Table and Dispatch Command. */
    for (cmd = 0; redit_table[cmd].name != NULL; cmd++)
    {
	if (!str_prefix(command, redit_table[cmd].name))
	{
	    if ((*redit_table[cmd].olc_fun) (ch, argument))
	    {

    		char *strtime;
    		char buf[MSL];
		
		SET_BIT(pArea->area_flags, AREA_CHANGED);
		free_string(pRoom->last_edited);

    		strtime = ctime(&current_time);
    		strtime[strlen(strtime)-1] = '\0';

    		sprintf(buf, "%s at %s", ch->name, strtime);
		pRoom->last_edited = str_dup(buf);
	    }	
	    
	    return TRUE;
	}
    }

    /* Default to Standard Interpreter. */
    interpret(ch, arg);
    return FALSE;
}



/* Object Interpreter, called by do_oedit. */
bool oedit(CHAR_DATA *ch, char *argument)
{
    AREA_DATA *pArea;
    OBJ_INDEX_DATA *pObj;
    char arg[MAX_STRING_LENGTH];
    char command[MAX_INPUT_LENGTH];
    int  cmd;

    smash_tilde(argument);
    strcpy(arg, argument);
    argument = one_argument(argument, command);

    EDIT_OBJ(ch, pObj);
    pArea = pObj->area;

    if (!IS_BUILDER(ch, pArea))
    {
	send_to_char("OEdit: Insufficient security to modify area.\n\r", ch);
	edit_done(ch);
	return FALSE;
    }

    if (!str_cmp(command, "done"))
    {
	edit_done(ch);
	return FALSE;
    }

    if (command[0] == '\0')
    {
	oedit_show(ch, argument);
	return FALSE;
    }

    /* Search Table and Dispatch Command. */
    for (cmd = 0; oedit_table[cmd].name != NULL; cmd++)
    {
	if (!str_prefix(command, oedit_table[cmd].name))
	{
	    if ((*oedit_table[cmd].olc_fun) (ch, argument))
	    {
    		char *strtime;
    		char buf[MSL];
		
		SET_BIT(pArea->area_flags, AREA_CHANGED);
		free_string(pObj->last_edited);

    		strtime = ctime(&current_time);
    		strtime[strlen(strtime)-1] = '\0';

    		sprintf(buf, "%s at %s", ch->name, strtime);
		pObj->last_edited = str_dup(buf);
	    }	
	    return TRUE;
	}
    }

    /* Default to Standard Interpreter. */
    interpret(ch, arg);
    return FALSE;
}


/* Mobile Interpreter, called by do_medit. */
bool medit(CHAR_DATA *ch, char *argument)
{
    AREA_DATA *pArea;
    MOB_INDEX_DATA *pMob;
    char command[MAX_INPUT_LENGTH];
    char arg[MAX_STRING_LENGTH];
    int  cmd;

    smash_tilde(argument);
    strcpy(arg, argument);
    argument = one_argument(argument, command);

    EDIT_MOB(ch, pMob);
    pArea = pMob->area;

    if (!IS_BUILDER(ch, pArea))
    {
	send_to_char("MEdit: Insufficient security to modify area.\n\r", ch);
	edit_done(ch);
	return FALSE;
    }

    if (!str_cmp(command, "done"))
    {
	edit_done(ch);
	return FALSE;
    }

    if (command[0] == '\0')
    {
        medit_show(ch, argument);
        return FALSE;
    }

    /* Search Table and Dispatch Command. */
    for (cmd = 0; medit_table[cmd].name != NULL; cmd++)
    {
	if (!str_prefix(command, medit_table[cmd].name))
	{
	    if ((*medit_table[cmd].olc_fun) (ch, argument))
	    {
    		char *strtime;
    		char buf[MSL];
		
		SET_BIT(pArea->area_flags, AREA_CHANGED);
		free_string(pMob->last_edited);

    		strtime = ctime(&current_time);
    		strtime[strlen(strtime)-1] = '\0';

    		sprintf(buf, "%s at %s", ch->name, strtime);
		pMob->last_edited = str_dup(buf);
	    }	
	    
	    return TRUE;
	}
    }

    /* Default to Standard Interpreter. */
    interpret(ch, arg);
    return FALSE;
}

/* Help Interpreter, called by do_hedit. */
bool hedit(CHAR_DATA *ch, char *argument)
{
    AREA_DATA *pArea;
    HELP_DATA *pHelp;
    HELP_AREA *pHArea;
    char command[MAX_INPUT_LENGTH];
    char arg[MAX_STRING_LENGTH];
    int  cmd;

    smash_tilde(argument);
    strcpy(arg, argument);
    argument = one_argument(argument, command);

    EDIT_HELP(ch, pHelp);
    pHArea = pHelp->help_area;
    pArea = pHelp->help_area->area;

    if (pArea && !IS_BUILDER(ch, pArea))
    {
	send_to_char("HEdit: Insufficient security to modify help.\n\r", ch);
	edit_done(ch);
	return FALSE;
    }

    if (!str_cmp(command, "done"))
    {
	edit_done(ch);
	return FALSE;
    }

    if (command[0] == '\0')
    {
        hedit_show(ch, argument);
        return FALSE;
    }

    /* Search Table and Dispatch Command. */
    for (cmd = 0; hedit_table[cmd].name != NULL; cmd++)
    {
	if (!str_prefix(command, hedit_table[cmd].name))
	{
	    if ((*hedit_table[cmd].olc_fun) (ch, argument))
		pHArea->changed = TRUE;
	    
	    return TRUE;
	}
    }

    /* Default to Standard Interpreter. */
    interpret(ch, arg);
    return FALSE;
}

const struct editor_cmd_type editor_table[] =
{
/*  {   command		function	}, */

    {   "area",		do_aedit	},
    {   "room",		do_redit	},
    {   "object",	do_oedit	},
    {   "mobile",	do_medit	},
    {	"mpcode",	do_mpedit	},
    {	"opcode",	do_opedit	},
    {	"rpcode",	do_rpedit	},
    {	"help",		do_hedit	},
    {	"skill",	do_skedit	},
    {	"race",		do_raedit	},
    {	"class",	do_cledit	},
    {	"group",	do_gredit	},

    {	NULL,		0,		}
};


/* Entry point for all editors. */
void do_olc(CHAR_DATA *ch, char *argument)
{
    char command[MAX_INPUT_LENGTH];
    int  cmd;

    if (IS_NPC(ch))
	return;

    argument = one_argument(argument, command);

    if (command[0] != '\0')
    {
	/* Search Table and Dispatch Command. */
	for (cmd = 0; editor_table[cmd].name != NULL; cmd++)
	{
	    if (!str_prefix(command, editor_table[cmd].name))
	    {
		(*editor_table[cmd].do_fun) (ch, argument);
		return;
	    }
	}
    }

    /* Invalid command, send help. */
    do_help(ch, "olc");
    return;
}


bool check_area(CHAR_DATA *ch, AREA_DATA *area)
{
  if (IS_SET(area->area_flags, AREA_EXTERN_EDIT))
  {
      send_to_char("Эта зона заблокирована для редактирования.\n\r", ch);
      return TRUE;
  }   
  else
      return FALSE;
}

/* Entry point for editing area_data. */
void do_aedit(CHAR_DATA *ch, char *argument)
{
    AREA_DATA *pArea;
    int value;
    char arg[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
    	return;

    pArea	= ch->in_room->area;

    if (check_area(ch, pArea))
        return;

    argument	= one_argument(argument,arg);

    if (is_number(arg))
    {
	value = atoi(arg);
	if (!(pArea = get_area_data(value)))
	{
	    send_to_char("That area vnum does not exist.\n\r", ch);
	    return;
	}
        
    }
/*    else
    if (!str_cmp(arg, "create"))
    {
	if (ch->pcdata->security < 9)
	{
		send_to_char("AEdit : Insufficient security for edit this area.\n\r", ch);
		return;
	}

	aedit_create(ch, ""); 

        if (ch->desc->editor == ED_OBJECT)
          return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);

	ch->desc->editor = ED_AREA;
	return;
    } */

    if (!IS_BUILDER(ch,pArea))
    {
	send_to_char("Insufficient security for edit this area.\n\r",ch);
	return;
    }

    if (ch->desc->editor == ED_OBJECT)
          return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);

    ch->desc->pEdit = (void *)pArea;
    ch->desc->editor = ED_AREA;
    aedit_show(ch, "");
    return;
}

/* Entry point for editing room_index_data. */
void do_redit(CHAR_DATA *ch, char *argument)
{
    ROOM_INDEX_DATA *pRoom;
    char arg1[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
    	return;

    argument = one_argument(argument, arg1);

    pRoom = ch->in_room;

    if (pRoom->area && check_area(ch, pRoom->area))
        return;

    if (!str_cmp(arg1, "reset"))	/* redit reset */
    {
	if (!IS_BUILDER(ch, pRoom->area))
	{
		send_to_char("Insufficient security for edit this room.\n\r" , ch);
        	return;
	}

	reset_room(pRoom);
	send_to_char("Room reset.\n\r", ch);

	return;
    }
    else
    if (!str_cmp(arg1, "create"))	/* redit create <vnum> */
    {
	if (argument[0] == '\0' || atoi(argument) == 0)
	{
	    send_to_char("Syntax:  edit room create [vnum]\n\r", ch);
	    return;
	}

	if (redit_create(ch, argument)) /* pEdit == nuevo cuarto */
	{
            if (ch->desc->editor == ED_OBJECT)
              return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);

            ch->desc->editor = ED_ROOM;
	    char_from_room(ch);
	    char_to_room(ch, ch->desc->pEdit, TRUE);
	    SET_BIT(((ROOM_INDEX_DATA *)ch->desc->pEdit)->area->area_flags, AREA_CHANGED);
	}

	return;
    }
    else if (!IS_NULLSTR(arg1))	/* redit <vnum> */
    {
	pRoom = get_room_index(atoi(arg1));

	if (!pRoom)
	{
		send_to_char("REdit : this room is not exist.\n\r", ch);
		return;
	}

	if (!IS_BUILDER(ch, pRoom->area))
	{
		send_to_char("REdit : Insufficient security for edit this room.\n\r", ch);
		return;
	}

	char_from_room(ch);
	char_to_room(ch, pRoom, TRUE);
    }

    if (!IS_BUILDER(ch, pRoom->area))
    {
    	send_to_char("REdit : Insufficient security for edit this room.\n\r", ch);
    	return;
    }

    if (ch->desc->editor == ED_OBJECT)
      return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);

    ch->desc->pEdit	= (void *) pRoom;
    ch->desc->editor	= ED_ROOM;
    redit_show(ch, "");
    return;
}



/* Entry point for editing obj_index_data. */
void do_oedit(CHAR_DATA *ch, char *argument)
{
    OBJ_INDEX_DATA *pObj;
    AREA_DATA *pArea;
    char arg1[MAX_STRING_LENGTH];
    int value;

    if (IS_NPC(ch))
	return;

    argument = one_argument(argument, arg1);

    if (is_number(arg1))
    {
	value = atoi(arg1);
	if (!(pObj = get_obj_index(value)))
	{
	    send_to_char("OEdit:  That vnum does not exist.\n\r", ch);
	    return;
	}

	if (!IS_BUILDER(ch, pObj->area))
	{
	    send_to_char("Insufficient security for modifying this object.\n\r" , ch);
	    return;
	}


        if (pObj->area && check_area(ch, pObj->area))
            return;

        if (ch->desc->editor == ED_OBJECT)
	    return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);

	ch->desc->pEdit = (void *)pObj;
	ch->desc->editor = ED_OBJECT;
	oedit_show(ch, "");
	return_affects(pObj, TRUE);
	return;
    }
    else
    {
	if (!str_cmp(arg1, "create"))
	{
	    value = atoi(argument);
	    if (argument[0] == '\0' || value == 0)
	    {
		send_to_char("Syntax:  oedit create [vnum]\n\r", ch);
		return;
	    }

	    pArea = get_vnum_area(value);

	    if (!pArea)
	    {
		send_to_char("OEdit:  That vnum is not assigned an area.\n\r", ch);
		return;
	    }

            if (check_area(ch, pArea))
                return;

	    if (!IS_BUILDER(ch, pArea))
	    {
		send_to_char("Insufficient security for modifying this object.\n\r" , ch);
	        return;
	    }

	    if (oedit_create(ch, argument))
	    {
		if (ch->desc->editor == ED_OBJECT)
		    return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);

		SET_BIT(pArea->area_flags, AREA_CHANGED);
		ch->desc->editor = ED_OBJECT;
		oedit_show(ch, "");
	    }
	    return;
	}
	else if (!str_cmp(arg1, "clone"))
	{
	    char arg2[MAX_STRING_LENGTH];
	    char *arg3;
	    int vnum;

	    arg3 = one_argument(argument, arg2);

	    if (IS_NULLSTR(arg2) || IS_NULLSTR(arg3)
		|| !is_number(arg2) || !is_number(arg3))
	    {
		send_to_char("Syntax: oedit clone <original obj vnum> <clone vnum>.\n\r", ch);
		return;
	    }
	    
	    vnum = atoi(arg3);
	    pArea = get_vnum_area(vnum);

	    if (!pArea)
	    {
		send_to_char("OEdit:  That vnum is not assigned an area.\n\r", ch);
		return;
	    }

            if (check_area(ch, pArea))
                return;

	    if (!IS_BUILDER(ch, pArea))
	    {
		send_to_char("Insufficient security for modifying this object.\n\r" , ch);
	        return;
	    }

	    if (oedit_clone(ch, argument))
	    {
		if (ch->desc->editor == ED_OBJECT)
		    return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);

		SET_BIT(pArea->area_flags, AREA_CHANGED);
		ch->desc->editor = ED_OBJECT;
		oedit_show(ch, "");
	    }
	    return;
	}
    }

    send_to_char("OEdit:  There is no default object to edit.\n\r", ch);
    return;
}

/* Entry point for editing mob_index_data. */
void do_medit(CHAR_DATA *ch, char *argument)
{
    MOB_INDEX_DATA *pMob;
    AREA_DATA *pArea;
    int value;
    char arg1[MAX_STRING_LENGTH];

    argument = one_argument(argument, arg1);

    if (IS_NPC(ch))
    	return;

    if (is_number(arg1))
    {
	value = atoi(arg1);
	if (!(pMob = get_mob_index(value)))
	{
	    send_to_char("MEdit:  That vnum does not exist.\n\r", ch);
	    return;
	}

	if (!IS_BUILDER(ch, pMob->area))
	{
		send_to_char("Insufficient security for modifying mobs.\n\r" , ch);
	        return;
	}


        if (pMob->area && check_area(ch, pMob->area))
            return;

        if (ch->desc->editor == ED_OBJECT)
          return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);

	ch->desc->pEdit = (void *)pMob;
	ch->desc->editor = ED_MOBILE;
	medit_show(ch, "");
	return;
    }
    else
    {
	if (!str_cmp(arg1, "create"))
	{
	    value = atoi(argument);
	    if (arg1[0] == '\0' || value == 0)
	    {
		send_to_char("Syntax:  edit mobile create [vnum]\n\r", ch);
		return;
	    }

	    pArea = get_vnum_area(value);

	    if (!pArea)
	    {
		send_to_char("OEdit:  That vnum is not assigned an area.\n\r", ch);
		return;
	    }


            if (check_area(ch, pArea))
                return;

	    if (!IS_BUILDER(ch, pArea))
	    {
		send_to_char("Insufficient security for modifying mobs.\n\r" , ch);
	        return;
	    }

	    if (medit_create(ch, argument))
	    {
                if (ch->desc->editor == ED_OBJECT)
          	  return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);

		SET_BIT(pArea->area_flags, AREA_CHANGED);
		ch->desc->editor = ED_MOBILE;
		medit_show(ch, "");
	    }
	}
	else if (!str_cmp(arg1, "clone"))
	{
	    char arg2[MAX_STRING_LENGTH];
	    char *arg3;
	    int  clone;

	    arg3 = one_argument(argument, arg2);
	    
	    if (IS_NULLSTR(arg2) || IS_NULLSTR(arg3)
		|| !is_number(arg2) || !is_number(arg3))
	    {
		send_to_char("Syntax:  medit clone <orig vnum> <clone vnum>\n\r", ch);
		return;
	    }

	    value = atoi(arg2);
	    clone = atoi(arg3);

	    pArea = get_vnum_area(clone);

	    if (!pArea)
	    {
		send_to_char("OEdit:  That vnum is not assigned an area.\n\r", ch);
		return;
	    }


            if (check_area(ch, pArea))
                return;

	    if (!IS_BUILDER(ch, pArea))
	    {
		send_to_char("Insufficient security for modifying mobs.\n\r" , ch);
	        return;
	    }

	    if (medit_clone(ch, argument))
	    {
		if (ch->desc->editor == ED_OBJECT)
		    return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);

		SET_BIT(pArea->area_flags, AREA_CHANGED);
		ch->desc->editor = ED_MOBILE;
		medit_show(ch, "");
	    }
	    return;
	}
    }

    send_to_char("MEdit:  There is no default mobile to edit.\n\r", ch);
    return;
}


/* Entry point for editing help_data. */
void do_hedit(CHAR_DATA *ch, char *argument)
{
    HELP_DATA *pHelp;
    HELP_AREA *pArea;
    char arg1[MAX_STRING_LENGTH];

    argument = one_argument(argument, arg1);

    if (IS_NPC(ch))
    	return;

    if (!str_cmp(arg1, "create"))
    {
	if (argument[0] == '\0')
	{
	    send_to_char("Syntax:  edit help create <filename>\n\r", ch);
	    return;
	}

	pArea = get_help_area(argument);

	if (!pArea)
	{
	    send_to_char("HEdit:  That filename is not assigned an area.\n\r", ch);
	    return;
	}

	if (hedit_create(ch, argument))
	{
   	    if (ch->desc->editor == ED_OBJECT)
		return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);

	    pArea->changed = TRUE;
	    ch->desc->editor = ED_HELP;
	    hedit_show(ch, "");
	}
	return;
    }
    else
    {
	strcat(arg1, " ");
	strcat(arg1, argument);

	if ((pHelp = get_help(arg1)) == NULL)
	{
	    send_to_char("HEdit:  That help does not exist.\n\r", ch);
	    return;
	}

        if (ch->desc->editor == ED_OBJECT)
          return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);

	ch->desc->pEdit = (void *)pHelp;
	ch->desc->editor = ED_HELP;
	hedit_show(ch, "");
	return;
    }

    send_to_char("HEdit:  There is no default help to edit.\n\r", ch);
    return;
}

void display_resets(CHAR_DATA *ch)
{
    ROOM_INDEX_DATA	*pRoom;
    RESET_DATA		*pReset;
    MOB_INDEX_DATA	*pMob = NULL;
    char 		buf   [ MAX_STRING_LENGTH ];
    char 		final [ MAX_STRING_LENGTH ];
    int 		iReset = 0;

    EDIT_ROOM(ch, pRoom);
    final[0]  = '\0';
    
    send_to_char (
  " No.  Loads    Description       Location         Vnum   Mx Mn Description"
  "\n\r"
  "==== ======== ============= =================== ======== ===== ==========="
  "\n\r", ch);

    for (pReset = pRoom->reset_first; pReset; pReset = pReset->next)
    {
	OBJ_INDEX_DATA  *pObj;
	MOB_INDEX_DATA  *pMobIndex;
	OBJ_INDEX_DATA  *pObjIndex;
	OBJ_INDEX_DATA  *pObjToIndex;
	ROOM_INDEX_DATA *pRoomIndex;

	final[0] = '\0';
	sprintf(final, "[%2d] ", ++iReset);

	switch (pReset->command)
	{
	default:
	    sprintf(buf, "Bad reset command: %c.", pReset->command);
	    strcat(final, buf);
	    break;

	case 'M':
	case 'H':
	    if (!(pMobIndex = get_mob_index(pReset->arg1)))
	    {
                sprintf(buf, "Load Mobile - Bad Mob %d\n\r", pReset->arg1);
                strcat(final, buf);
                continue;
	    }

	    if (!(pRoomIndex = get_room_index(pReset->arg3)))
	    {
                sprintf(buf, "Load Mobile - Bad Room %d\n\r", pReset->arg3);
                strcat(final, buf);
                continue;
	    }

	    if (pReset->command == 'H' && pMob == NULL)
	    {
		snprintf(buf, sizeof(buf), "{RLoad Horseman -- no prev. mobile.{x\n\r");
		strlcat(final, buf, sizeof(final));
		continue;
	    }

            sprintf(buf, "M[%5d] %-13.13s %-18.18s  %c[%5d] %2d %2d "
		    "%-15.15s\n\r",
		    pReset->arg1, pMobIndex->short_descr,
		    pReset->command == 'H' ? "on horse" : "in room",
		    pReset->command == 'H' ? 'M' : 'R',
		    pReset->arg3,
	       	    pReset->arg2, pReset->arg4,
		    pReset->command == 'H' ? pMob->short_descr
					   : pRoomIndex->name);
            strcat(final, buf);

	    /*
	     * Check for pet shop.
	     * -------------------
	     */
	    {
		ROOM_INDEX_DATA *pRoomIndexPrev;

		pRoomIndexPrev = get_room_index(pRoomIndex->vnum - 1);
		if (pRoomIndexPrev
		    && IS_SET(pRoomIndexPrev->room_flags, ROOM_PET_SHOP))
		{
                    final[5] = 'P';
		}
	    }

            pMob = pMobIndex;
	    break;

	case 'O':
	    if (!(pObjIndex = get_obj_index(pReset->arg1)))
	    {
                sprintf(buf, "Load Object - Bad Object %d\n\r",
		    pReset->arg1);
                strcat(final, buf);
                continue;
	    }

            pObj       = pObjIndex;

	    if (!(pRoomIndex = get_room_index(pReset->arg3)))
	    {
                sprintf(buf, "Load Object - Bad Room %d\n\r", pReset->arg3);
                strcat(final, buf);
                continue;
	    }

	    if (!(pRoomIndex = get_room_index(pReset->arg3)))
                bugf("Reset_room [%d]: 'O' 3 : bad vnum %d.", pRoom->vnum, pReset->arg1);

            sprintf(buf, "O[%5d] %-13.13s in room             "
                         "R[%5d]         %-15.15s\n\r",
		    pReset->arg1, pObj->short_descr,
	     	    pReset->arg3, 
	     	    pRoomIndex->name);
            strcat(final, buf);

	    break;

	case 'P':
	    if (!(pObjIndex = get_obj_index(pReset->arg1)))
	    {
                sprintf(buf, "Put Object - Bad Object %d\n\r",
                    pReset->arg1);
                strcat(final, buf);
                continue;
	    }

            pObj       = pObjIndex;

	    if (!(pObjToIndex = get_obj_index(pReset->arg3)))
	    {
                sprintf(buf, "Put Object - Bad To Object %d\n\r",
                    pReset->arg3);
                strcat(final, buf);
                continue;
	    }

	    sprintf(buf,
		"O[%5d] %-13.13s inside              O[%5d] %2d %2d %-15.15s\n\r",
		pReset->arg1,
		pObj->short_descr,
		pReset->arg3,
		pReset->arg2,
		pReset->arg4,
		pObjToIndex->short_descr);
            strcat(final, buf);

	    break;

	case 'G':
	case 'E':
	    if (!(pObjIndex = get_obj_index(pReset->arg1)))
	    {
                sprintf(buf, "Give/Equip Object - Bad Object %d\n\r",
                    pReset->arg1);
                strcat(final, buf);
                continue;
	    }

            pObj       = pObjIndex;

	    if (!pMob)
	    {
                sprintf(buf, "Give/Equip Object - No Previous Mobile\n\r");
                strcat(final, buf);
                break;
	    }

	    if (pMob->pShop)
	    {
	    sprintf(buf,
		"O[%5d] %-13.13s in the inventory of S[%5d]       %-15.15s\n\r",
		pReset->arg1,
		pObj->short_descr,                           
		pMob->vnum,
		pMob->short_descr );
	    }
	    else
	    sprintf(buf,
		"O[%5d] %-13.13s %-19.19s M[%5d] %2d     %-15.15s\n\r",
		pReset->arg1,
		pObj->short_descr,
		(pReset->command == 'G') ?
		    flag_string(wear_loc_strings, WEAR_NONE, FALSE)
		  : flag_string(wear_loc_strings, pReset->arg3, FALSE),
		  pMob->vnum,
		  pReset->arg2,
		  pMob->short_descr);
	    strcat(final, buf);

	    break;

	/*
	 * Doors are set in rs_flags don't need to be displayed.
	 * If you want to display them then uncomment the new_reset
	 * line in the case 'D' in load_resets in db.c and here.
	 */
	case 'D':
	    pRoomIndex = get_room_index(pReset->arg1);
	    sprintf(buf, "R[%5d] %s door of %-19.19s reset to %s\n\r",
		pReset->arg1,
		capitalize(dir_name[ pReset->arg2 ]),
		pRoomIndex->name,
		flag_string(door_resets, pReset->arg3, FALSE));
	    strcat(final, buf);

	    break;
	/*
	 * End Doors Comment.
	 */
	case 'R':
	    if (!(pRoomIndex = get_room_index(pReset->arg1)))
	    {
		sprintf(buf, "Randomize Exits - Bad Room %d\n\r",
		    pReset->arg1);
		strcat(final, buf);
		continue;
	    }

	    sprintf(buf, "R[%5d] Exits are randomized in %s\n\r",
		pReset->arg1, pRoomIndex->name);
	    strcat(final, buf);

	    break;

	case 'T':
	case 'C':
	case 'X':
	    if ((pObjIndex = get_obj_index(pReset->arg1)) == NULL)
	    {
		sprintf(buf, "Trap - bad vnum %d\n\r", pReset->arg1);
		strcat(final, buf);
		continue;
	    }
	
            sprintf(buf, "T[%5d] %-13.13s %-18.18s  %c[%5d]       "
		    "%-15.15s\n\r",
		    pReset->arg1, pObjIndex->short_descr,
		    pReset->command == 'T'
			? "in room"
			: (pReset->command == 'C'
			    ? "on object"
			    : "on exit"),
		    pReset->command == 'T'
			? 'R'
			: (pReset->command == 'C'
			    ? 'O'
			    : 'E'),
		    pReset->arg2,
		    flag_string(trap_flags, pReset->arg3, FALSE));
	    strcat(final, buf);
	    break;

	}
	send_to_char(final, ch);
    }

    return;
}



/*****************************************************************************
 Name:		add_reset
 Purpose:	Inserts a new reset in the given index slot.
 Called by:	do_resets(olc.c).
 ****************************************************************************/
void add_reset(ROOM_INDEX_DATA *room, RESET_DATA *pReset, int index)
{
    RESET_DATA *reset;
    int iReset = 0;

    if (!room->reset_first)
    {
	room->reset_first	= pReset;
	room->reset_last	= pReset;
	pReset->next		= NULL;
	return;
    }

    index--;

    if (index == 0)	/* First slot (1) selected. */
    {
	pReset->next = room->reset_first;
	room->reset_first = pReset;
	return;
    }

    /*
     * If negative slot(<= 0 selected) then this will find the last.
     */
    for (reset = room->reset_first; reset->next; reset = reset->next)
    {
	if (++iReset == index)
	    break;
    }

    pReset->next	= reset->next;
    reset->next		= pReset;
    if (!pReset->next)
	room->reset_last = pReset;
    return;
}



void do_resets(CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    char arg4[MAX_INPUT_LENGTH];
    char arg5[MAX_INPUT_LENGTH];
    char arg6[MAX_INPUT_LENGTH];
    char arg7[MAX_INPUT_LENGTH];
    RESET_DATA *pReset = NULL;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    argument = one_argument(argument, arg3);
    argument = one_argument(argument, arg4);
    argument = one_argument(argument, arg5);
    argument = one_argument(argument, arg6);
    argument = one_argument(argument, arg7);

    if (!IS_BUILDER(ch, ch->in_room->area))
    {
	send_to_char("Resets: Invalid security for editing this area.\n\r",
                      ch);
	return;
    }

    /*
     * Display resets in current room.
     * -------------------------------
     */
    if (arg1[0] == '\0')
    {
	if (ch->in_room->reset_first)
	{
	    send_to_char(
		"Resets:\n\rM = mobile, H = Horseman, R = room, O = object, "
		"P = pet, S = shopkeeper, T = trap\n\r"
		"Mx - in world, Mn - in container (room, or mob, or obj), if <= 0 - no limit\n\r\n\r", ch);
	    display_resets(ch);
	}
	else
	    send_to_char("No resets in this room.\n\r", ch);

	return;
    }


    /*
     * Take index number and search for commands.
     * ------------------------------------------
     */
    if (is_number(arg1))
    {
	ROOM_INDEX_DATA *pRoom = ch->in_room;

	/*
	 * Delete a reset.
	 * ---------------
	 */
	if (!str_cmp(arg2, "delete"))
	{
	    int insert_loc = atoi(arg1);

	    if (!ch->in_room->reset_first)
	    {
		send_to_char("No resets in this area.\n\r", ch);
		return;
	    }

	    if (insert_loc-1 <= 0)
	    {
		pReset = pRoom->reset_first;
		pRoom->reset_first = pRoom->reset_first->next;
		if (!pRoom->reset_first)
		    pRoom->reset_last = NULL;
	    }
	    else
	    {
		int iReset = 0;
		RESET_DATA *prev = NULL;

		for (pReset = pRoom->reset_first;
		     pReset;
		     pReset = pReset->next)
		{
		    if (++iReset == insert_loc)
			break;
		    prev = pReset;
		}

		if (!pReset)
		{
		    send_to_char("Reset not found.\n\r", ch);
		    return;
		}

		if (prev)
		    prev->next = prev->next->next;
		else
		    pRoom->reset_first = pRoom->reset_first->next;

		for (pRoom->reset_last = pRoom->reset_first;
		     pRoom->reset_last->next;
		     pRoom->reset_last = pRoom->reset_last->next)
		    ;
	    }

	    free_reset_data(pReset);
	    send_to_char("Reset deleted.\n\r", ch);
	}
	else
	/*
	 * Add a reset.
	 * ------------
	 */
	if ((!str_cmp(arg2, "mob") && is_number(arg3))
	    || (!str_cmp(arg2, "obj") && is_number(arg3))
	    || (!str_cmp(arg2, "horseman") && is_number(arg3)))
	{
	    /*
	     * Check for Mobile reset.
	     * -----------------------
	     */
	    if (!str_cmp(arg2, "mob"))
	    {
		if (get_mob_index(is_number(arg3) ? atoi(arg3) : 1) == NULL)
		{
		    send_to_char("Mob doesn't exist.\n\r",ch);
		    return;
	       	}
		pReset = new_reset_data();

    		pReset->command = 'M';
		pReset->arg1 = atoi(arg3);
		pReset->arg2 = is_number(arg4) ? atoi(arg4) : 1; /* Max # */
		pReset->arg3 = ch->in_room->vnum;
		pReset->arg4 = is_number(arg5) ? atoi(arg5) : 1; /* Min # */
	    }
	    else if (!str_cmp(arg2, "horseman"))
	    {
		if (get_mob_index(is_number(arg3) ? atoi(arg3) : 1) == NULL)
		{
		    send_to_char("Mob doesn't exist.\n\r",ch);
		    return;
	       	}
		pReset = new_reset_data();
		pReset->command = 'H';
		pReset->arg1 = atoi(arg3);
		pReset->arg2 = 1;
		pReset->arg3 = ch->in_room->vnum;
		pReset->arg4 = 1;
	    }
	    else
	    /*
	     * Check for Object reset.
	     * -----------------------
	     */
	    if (!str_cmp(arg2, "obj"))
	    {
		pReset = new_reset_data();
		pReset->arg1    = atoi(arg3);
		
		/*
		 * Inside another object.
		 * ----------------------
		 */
		if (!str_prefix(arg4, "inside"))
		{                    
		    OBJ_INDEX_DATA *temp;

		    if ((temp = get_obj_index(is_number(arg5) ? atoi(arg5) : 1)) == NULL) /* fix by kalahn@pobox.com */
		    {
			send_to_char("Couldn't find Object 2.\n\r",ch);
			free_reset_data(pReset);
			return;     
		    }

		    if ((temp->item_type != ITEM_CONTAINER)
			&& (temp->item_type != ITEM_CORPSE_NPC))
		    {
			send_to_char("Object 2 is not a container.\n\r", ch);
			free_reset_data(pReset);
			return;
		    }
		    pReset->command = 'P';
		    pReset->arg2    = is_number(arg6) ? atoi(arg6) : 1;
		    pReset->arg3    = is_number(arg5) ? atoi(arg5) : 1;
		    pReset->arg4    = is_number(arg7) ? atoi(arg7) : 1;
		}
		else
		/*
		 * Inside the room.
		 * ----------------
		 */
		if (!str_cmp(arg4, "room"))
		{
		    if (get_obj_index(atoi(arg3)) == NULL)
		    {
		 	send_to_char("Vnum does not exist.\n\r", ch);
			free_reset_data(pReset);
			return;
	      	    }
		    pReset->command  = 'O';
		    pReset->arg2     = 0;
		    pReset->arg3     = ch->in_room->vnum;
		    pReset->arg4     = 0;
		}
		else
		/*
		 * Into a Mobile's inventory.
		 * --------------------------
		 */
		{
		    if (flag_value(wear_loc_flags, arg4) == NO_FLAG)
		    {
			send_to_char("Resets: '? wear-loc'\n\r", ch);
			free_reset_data(pReset);
			return;
		    }
		    if (get_obj_index(atoi(arg3)) == NULL)
		    {
			send_to_char("Vnum doesn't exist.\n\r",ch);
			free_reset_data(pReset);
			return;
		    }
		    pReset->arg1 = atoi(arg3);
		    pReset->arg3 = flag_value(wear_loc_flags, arg4);
		    if (pReset->arg3 == WEAR_NONE)
			pReset->command = 'G';
		    else
			pReset->command = 'E';
		}
	    }
	    add_reset(ch->in_room, pReset, atoi(arg1));
	    SET_BIT(ch->in_room->area->area_flags, AREA_CHANGED);
	    send_to_char("Reset added.\n\r", ch);
	}
	else if (!str_cmp(arg2, "trap") && is_number(arg3))
	{
	    int tvnum = atoi(arg3);
	    OBJ_INDEX_DATA *temp;
	    int16_t tflags;
	
	    if (get_obj_index(tvnum) == NULL)
	    {
		send_to_char("Vnum doesn't exist.\n\r", ch);
		return;
	    }
	
	    pReset = new_reset_data();
	    pReset->arg1 = tvnum;

	    if (!str_cmp(arg4, "obj"))
	    {
		int ovnum;
	    
		if (!is_number(arg5))
		{
		    send_to_char("Incorrect vnum.\n\r", ch);
		    free_reset_data(pReset);
		    return;
		}

		ovnum = atoi(arg5);

		if ((temp = get_obj_index(ovnum)) == NULL)
		{
		    send_to_char("Object 2 does not exist.\n\r", ch);
		    free_reset_data(pReset);
		    return;
		}

		if (temp->item_type != ITEM_CONTAINER
		    && temp->item_type != ITEM_PORTAL)
		{
		    send_to_char("Object 2 is not a container or a portal.\n\r", ch);
		    free_reset_data(pReset);
		    return;
		}

		if (IS_NULLSTR(arg6)
		    || (tflags = flag_value(trap_flags, arg6)) == NO_FLAG)
		{
		    send_to_char("Incorrect trap flags.\n\r", ch);
		    free_reset_data(pReset);
		    return;
		}

		pReset->command = 'C';
		pReset->arg2 = ovnum;
		pReset->arg3 = tflags;
		pReset->arg4 = 0;
	    }
	    else if (!str_cmp(arg4, "exit"))
	    {
		int tdoor;
	    
		if (!is_number(arg5))
		{
		    send_to_char("Incorrect door number.\n\r", ch);
		    free_reset_data(pReset);
		    return;
		}

		tdoor = atoi(arg5);
		if (tdoor < 0 || tdoor > 5)
		{
		    send_to_char("Incorrect door number.\n\r", ch);
		    free_reset_data(pReset);
		    return;
		}
		
		if (IS_NULLSTR(arg6)
		    || (tflags = flag_value(trap_flags, arg6)) == NO_FLAG)
		{
		    send_to_char("Incorrect trap flags.\n\r", ch);
		    free_reset_data(pReset);
		    return;
		}

		if (!ch->in_room->exit[tdoor])
		{
		    send_to_char("Unexistent door.\n\r", ch);
		    free_reset_data(pReset);
		    return;
		}

		pReset->command = 'X';
		pReset->arg2 = tdoor;
		pReset->arg3 = tflags;
		pReset->arg4 = ch->in_room->vnum;
	    }
	    else
	    {
		pReset->command = 'T';
		pReset->arg2 = ch->in_room->vnum;
		pReset->arg3 = 0;
		pReset->arg4 = 0;
	    }
	    add_reset(ch->in_room, pReset, atoi(arg1));
	    SET_BIT(ch->in_room->area->area_flags, AREA_CHANGED);
	    send_to_char("Reset added.\n\r", ch);
	}
	else
	if (!str_cmp(arg2, "random") && is_number(arg3))
	{
		if (atoi(arg3) < 1 || atoi(arg3) > 6)
		{
		    send_to_char("Invalid argument.\n\r", ch);
		    return;
		}
		pReset = new_reset_data();
		pReset->command = 'R';
		pReset->arg1 = ch->in_room->vnum;
		pReset->arg2 = atoi(arg3);
		add_reset(ch->in_room, pReset, atoi(arg1));
		SET_BIT(ch->in_room->area->area_flags, AREA_CHANGED);
		send_to_char("Random exits reset added.\n\r", ch);
	}
	else
	{
	    send_to_char("Syntax: RESET <number> OBJ <vnum> <wear_loc> [limit]\n\r",
			 ch);
	    send_to_char("        RESET <number> OBJ <vnum> inside <vnum> "
			 "[limit] [count]\n\r", ch);
	    send_to_char("        RESET <number> OBJ <vnum> room\n\r", ch);
	    send_to_char("        RESET <number> MOB <vnum> [max #x world] "
			 "[max #x room]\n\r", ch);
	    send_to_char("        RESET <number> HORSEMAN <vnum>\n\r", ch);
	    send_to_char("        RESET <number> DELETE\n\r", ch);
	    send_to_char("        RESET <number> RANDOM [#x exits]\n\r", ch);
	}
    }

    return;
}

int compare_vnums (const void *v1, const void *v2)
{
    AREA_DATA *a1 = *(AREA_DATA**)v1;
    AREA_DATA *a2 = *(AREA_DATA**)v2;

    if (a1->min_vnum < a2->min_vnum)
	return -1;
    else if (a1->min_vnum > a2->min_vnum)
	return +1;
    else
	return 0;
}


/*****************************************************************************
 Name:		do_alist
 Purpose:	Normal command to list areas and display area information.
 Called by:	interpreter(interp.c)
 ****************************************************************************/
void do_alist(CHAR_DATA *ch, char *argument)
{
    char buf    [ MAX_STRING_LENGTH ];
    char colour[2];
    BUFFER *output = new_buf();
    AREA_DATA *pArea;
    AREA_DATA *AreaTable[top_area];
    int i;
    bool na = FALSE;

    sprintf(buf, "[%3s] [%-27s] (%-5s-%5s) [%-10s] %3s [%-10s]\n\r",
	    "Num", "Area Name", "lvnum", "uvnum", "Filename", "Sec", "Builders");
    add_buf(output, buf);

    if (!str_cmp(argument, "n/a"))
	na = TRUE;

    for (pArea = area_first, i = 0; pArea; pArea = pArea->next, i++)
	AreaTable[i] = pArea;

    qsort(AreaTable, i , sizeof(AreaTable[0]), compare_vnums);

    for (i = 0; i < top_area; i++)
    {
	pArea = AreaTable[i];

	if (IS_NULLSTR(argument)
	    || !str_infix(argument, pArea->name)
	    || na)
	{
	    bool is_na = FALSE;
	
	    colour[0] = '\0';
	
	    if (IS_SET(pArea->area_flags, AREA_NA))
	    {
		sprintf(colour, "{r");
		is_na = TRUE;
	    }

	    if (IS_SET(pArea->area_flags, AREA_TESTING))
	    {
		sprintf(colour, "{y");
		is_na = TRUE;
	    }

	    if (!na || is_na)
	    {
		sprintf(buf, "%s[%3d] %-29.29s (%-5d-%5d) %-12.12s [%d] "
			     "[%-10.10s]{x\n\r",
			colour,
			pArea->vnum,
			IS_NULLSTR(pArea->credits)
				    ? pArea->name
				    : pArea->credits,
			pArea->min_vnum,
			pArea->max_vnum,
			pArea->file_name,
			pArea->security,
			pArea->builders);
		add_buf(output, buf);
	    }
	}	
    }

    sprintf(buf, "\n\rNote:\n\r    N/A areas are shown {rred{x,\n\r"
		 "    Areas under testing are shown {yyellow{x.\n\r");
    add_buf(output, buf);

    page_to_char(buf_string(output), ch);
    free_buf(output);
    return;
}

bool cedit(CHAR_DATA *ch, char *argument)
{
    AREA_DATA *pArea;
    CHAT_KEYS *pCkey;
    char command[MAX_INPUT_LENGTH];
    char arg[MAX_STRING_LENGTH];
    int  cmd;

    smash_tilde(argument);
    strcpy(arg, argument);
    argument = one_argument(argument, command);

    EDIT_CKEY(ch, pCkey);
    pArea = pCkey->area;

    if (pArea && !IS_BUILDER(ch, pArea))
    {
	send_to_char("CEdit: Insufficient security to modify chatbook.\n\r", ch);
	edit_done(ch);
	return FALSE;
    }

    if (!str_cmp(command, "done"))
    {
	edit_done(ch);
	return FALSE;
    }

    if (command[0] == '\0')
    {
        cedit_show(ch, argument);
        return FALSE;
    }

    /* Search Table and Dispatch Command. */
    for (cmd = 0; cedit_table[cmd].name != NULL; cmd++)
    {
	if (!str_prefix(command, cedit_table[cmd].name))
	{
	    if ((*cedit_table[cmd].olc_fun) (ch, argument))
		SET_BIT(pArea->area_flags, AREA_CHANGED);
	    
	    return TRUE;
	}
    }

    /* Default to Standard Interpreter. */
    interpret(ch, arg);
    return FALSE;
}

/* Entry point for editing chat_data */
void do_cedit(CHAR_DATA *ch, char *argument)
{
    CHAT_KEYS *pCkey;
    AREA_DATA *pArea;
    int value;
    char arg1[MAX_STRING_LENGTH];

    argument = one_argument(argument, arg1);

    if (IS_NPC(ch))
    	return;

    if (is_number(arg1))
    {
	value = atoi(arg1);
	if (!(pCkey = get_chat_index(value)))
	{
	    send_to_char("CEdit:  That vnum does not exist.\n\r", ch);
	    return;
	}

	if (!IS_BUILDER(ch, pCkey->area))
	{
	    send_to_char("Insufficient security for modifying mobs.\n\r" , ch);
	    return;
	}

	if (ch->desc->editor == ED_OBJECT)
	    return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);

	ch->desc->pEdit = (void *) pCkey;
	ch->desc->editor = ED_CHAT;
	cedit_show(ch, "");
	return;
    }
    else
    {
	if (!str_cmp(arg1, "create"))
	{
	    value = atoi(argument);
	    if (arg1[0] == '\0' || value == 0)
	    {
		send_to_char("Syntax:  edit chat create [vnum]\n\r", ch);
		return;
	    }

	    pArea = get_vnum_area(value);

	    if (!pArea)
	    {
		send_to_char("CEdit:  That vnum is not assigned an area.\n\r", ch);
		return;
	    }

	    if (!IS_BUILDER(ch, pArea))
	    {
		send_to_char("Insufficient security for modifying chatbooks.\n\r" , ch);
	        return;
	    }

	    if (cedit_create(ch, argument))
	    {
              if (ch->desc->editor == ED_OBJECT)
          	  return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);

		SET_BIT(pArea->area_flags, AREA_CHANGED);
		ch->desc->editor = ED_CHAT;
		cedit_show(ch, "");
	    }
	    return;
	}
    }

    send_to_char("CEdit:  There is no default chatbook to edit.\n\r", ch);
    return;
}

/* charset=cp1251 */
