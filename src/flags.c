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
#include <time.h>
#include "merc.h"


void do_flag(CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH],arg3[MAX_INPUT_LENGTH];
    char word[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int64_t *flag, old = 0, new_flag = 0, marked = 0, pos;
    char type;
    const struct flag_type *flag_table;

    argument = one_argument(argument,arg1);
    argument = one_argument(argument,arg2);
    argument = one_argument(argument,arg3);

    type = argument[0];

    if (type == '=' || type == '-' || type == '+')
        argument = one_argument(argument,word);

    if (arg1[0] == '\0')
    {
	send_to_char("Syntax:\n\r",ch);
	send_to_char("  flag mob  <name> <field> <flags>\n\r",ch);
	send_to_char("  flag char <name> <field> <flags>\n\r",ch);
	send_to_char("  mob  flags: act,aff,off,imm,res,vuln,form,part\n\r",ch);
	send_to_char("  char flags: plr,comm,aff,imm,res,vuln,\n\r",ch);
	send_to_char("  +: add flag, -: remove flag, = set equal to\n\r",ch);
	send_to_char("  otherwise flag toggles the flags listed.\n\r",ch);
	return;
    }

    if (arg2[0] == '\0')
    {
	send_to_char("What do you wish to set flags on?\n\r",ch);
	return;
    }

    if (arg3[0] == '\0')
    {
	send_to_char("You need to specify a flag to set.\n\r",ch);
	return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("Which flags do you wish to change?\n\r",ch);
	return;
    }

    if (!str_prefix(arg1,"mob") || !str_prefix(arg1,"char"))
    {
	victim = get_char_world(ch,arg2);
	if (victim == NULL)
	{
	    send_to_char("You can't find them.\n\r",ch);
	    return;
	}

        /* select a flag to set */
	if (!str_prefix(arg3,"act"))
	{
	    if (!IS_NPC(victim))
	    {
		send_to_char("Use plr for PCs.\n\r",ch);
		return;
	    }

	    flag = &victim->act;
	    flag_table = act_flags;
	}

	else if (!str_prefix(arg3,"plr"))
	{
	    if (IS_NPC(victim))
	    {
		send_to_char("Use act for NPCs.\n\r",ch);
		return;
	    }

	    flag = &victim->act;
	    flag_table = plr_flags;
	}

 	else if (!str_prefix(arg3,"aff"))
	{
	    flag = &victim->affected_by;
	    flag_table = affect_flags;
	}

/* FIXME
  	else if (!str_prefix(arg3,"immunity"))
	{
	    flag = &victim->imm_flags;
	    flag_table = imm_flags;
	}

	else if (!str_prefix(arg3,"resist"))
	{
	    flag = &victim->res_flags;
	    flag_table = imm_flags;
	}

	else if (!str_prefix(arg3,"vuln"))
	{
	    flag = &victim->vuln_flags;
	    flag_table = imm_flags;
	}
*/
	else if (!str_prefix(arg3,"form"))
	{
	    if (!IS_NPC(victim))
	    {
	 	send_to_char("Form can't be set on PCs.\n\r",ch);
		return;
	    }

	    flag = &victim->form;
	    flag_table = form_flags;
	}

	else if (!str_prefix(arg3,"parts"))
	{
	    if (!IS_NPC(victim))
	    {
		send_to_char("Parts can't be set on PCs.\n\r",ch);
		return;
	    }

	    flag = &victim->parts;
	    flag_table = part_flags;
	}

	else if (!str_prefix(arg3,"comm"))
	{
	    if (IS_NPC(victim))
	    {
		send_to_char("Comm can't be set on NPCs.\n\r",ch);
		return;
	    }

	    flag = &victim->comm;
	    flag_table = comm_flags;
	}

	else 
	{
	    send_to_char("That's not an acceptable flag.\n\r",ch);
	    return;
	}

	old = *flag;
	victim->zone = NULL;

	if (type != '=')
		new_flag = old;

        /* mark the words */
        for (; ;)
        {
	    argument = one_argument(argument,word);

	    if (word[0] == '\0')
		break;

	    pos = flag_lookup(word,flag_table);
	    if (pos == 0)
	    {
		send_to_char("That flag doesn't exist!\n\r",ch);
		return;
	    }
	    else
		SET_BIT(marked,pos);
	}

	for (pos = 0; flag_table[pos].name != NULL; pos++)
	{
	    if (!flag_table[pos].settable && IS_SET(old,flag_table[pos].bit))
	    {
		SET_BIT(new_flag,flag_table[pos].bit);
		continue;
	    }

	    if (IS_SET(marked,flag_table[pos].bit))
	    {
		switch(type)
		{
		    case '=':
		    case '+':
			SET_BIT(new_flag,flag_table[pos].bit);
			break;
		    case '-':
			REMOVE_BIT(new_flag,flag_table[pos].bit);
			break;
		    default:
			if (IS_SET(new_flag,flag_table[pos].bit))
			    REMOVE_BIT(new_flag,flag_table[pos].bit);
			else
			    SET_BIT(new_flag,flag_table[pos].bit);
		}
	    }
	}
	*flag = new_flag;
	return;
    }
}


/*****************************************************************************
 Name:		is_stat(table)
 Purpose:	Returns TRUE if the table is a stat table and FALSE if flag.
 Called by:	flag_value and flag_string.
 Note:		This function is local and used only in bit.c.
 ****************************************************************************/
bool is_stat(const struct flag_type *flag_table)
{
    int flag;

    for (flag = 0; flag_stat_table[flag].structure; flag++)
    {
	if (flag_stat_table[flag].structure == flag_table
	    && flag_stat_table[flag].stat)
	{
	    return TRUE;
	}
    }
    return FALSE;
}





/*****************************************************************************
 Name:		flag_value(table, flag)
 Purpose:	Returns the value of the flags entered.  Multi-flags accepted.
 Called by:	olc.c and olc_act.c.
 ****************************************************************************/
int64_t flag_value(const struct flag_type *flag_table, char *argument)
{
    char word[MAX_INPUT_LENGTH];
    int64_t  bit, marked = 0;
    bool found = FALSE;

    if (IS_NULLSTR(argument))
	return 0;

    if (is_stat(flag_table))
    {
	one_argument(argument, word);

	if ((bit = flag_lookup(word, flag_table)) != NO_FLAG)
	    return bit;
	else
	    return NO_FLAG;
    }

    /*
     * Accept multiple flags.
     */
    for (; ;)
    {
        argument = one_argument(argument, word);

        if (word[0] == '\0')
	    break;

        if ((bit = flag_lookup(word, flag_table)) != NO_FLAG)
        {
            SET_BIT(marked, bit);
            found = TRUE;
        }
    }

    if (found)
	return marked;
    else
	return NO_FLAG;
}



/*****************************************************************************
 Name:		flag_string(table, flags/stat)
 Purpose:	Returns string with name(s) of the flags or stat entered.
 Called by:	act_olc.c, olc.c, and olc_save.c.
 ****************************************************************************/
char *flag_string(const struct flag_type *flag_table, int64_t bits, bool rname)
{
    return fflag_string(flag_table, bits, rname, TRUE);
}

char *fflag_string(const struct flag_type *flag_table, int64_t bits, bool rname, bool empty_none)
{
    static char buf[4096];
    int  flag;

    buf[0] = '\0';

    for (flag = 0; flag_table[flag].name != NULL; flag++)
    {

	if (!is_stat(flag_table) && IS_SET(bits, flag_table[flag].bit))
	{
	    char *tmp = NULL;

	    if (!rname || IS_NULLSTR(flag_table[flag].rname))
	      tmp = flag_table[flag].name;
	    else if (strcmp(flag_table[flag].rname, " "))
	      tmp = flag_table[flag].rname;

	    if (tmp)
	    {
	      strcat(buf, " ");
	      strcat(buf, tmp);
	    }
	}
	else if (flag_table[flag].bit == bits)
	{
	    char *tmp = NULL;

	    if (!rname || IS_NULLSTR(flag_table[flag].rname))
	      tmp = flag_table[flag].name;
	    else if (strcmp(flag_table[flag].rname, " "))
	      tmp = flag_table[flag].rname;

	    if (tmp)
	    {
	      strcat(buf, " ");
	      strcat(buf, tmp);
	    }

	    break;
	}
    } 

    return (buf[0] != '\0') ? buf + 1 : (empty_none ? "none" : "");
}

    
/* charset=cp1251 */
