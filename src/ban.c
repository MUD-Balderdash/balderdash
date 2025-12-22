/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik Strfeldt, Tom Madsen, and Katja Nyboe.    *
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
 *   Updated BAN.C written for Taron for Mark of the Beast.                *
 *    coral.wolfpaw.net 4800                                               *
 *    http://coral.wolfpaw.net/                                            *
 *   Send bugs to taron0@yahoo.com.                                        *
 **************************************************************************/
/***************************************************************************
 *   ROM 2.4 is copyright 1993-1998 Russ Taylor                            *
 *   ROM has been brought to you by the ROM consortium                     *
 *       Russ Taylor (rtaylor@hypercube.org)                               *
 *       Gabrielle Taylor (gtaylor@hypercube.org)                          *
 *       Brian Moore (zump@rom.org)                                        *
 *   By using this code, you have agreed to follow the terms of the        *
 *   ROM license, in the file Rom24/doc/rom.license                        *
 **************************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "merc.h"
#include "recycle.h"


SLIST_HEAD(, ban_data) ban_list;

void save_bans(void)
{
    BAN_DATA *pban;
    FILE *fp;
    bool found = FALSE;

    if ((fp = file_open(BAN_FILE, "w")) == NULL)
    {
	_perror(BAN_FILE);
	found = TRUE;
    }
    else
    {
	SLIST_FOREACH(pban, &ban_list, link)
	{
	    if (IS_SET(pban->ban_flags, BAN_PERMANENT))
	    {
		found = TRUE;
		fprintf(fp, "%s~\n%s~\n%d\n%s\n%s~\n%ld\n\n",
			pban->name, pban->banner, pban->level,
			fwrite_flag (pban->ban_flags), pban->reason,
			(long)pban->when);
	    }
	}
    }
   
    file_close(fp);
    if (!found)
        unlink(BAN_FILE);
}

void load_bans(void)
{
    FILE *fp;
    BAN_DATA *ban_last = NULL;

    SLIST_INIT(&ban_list);
	
    if ((fp = file_open(BAN_FILE, "r")) != NULL)
    {
	for (;;)
	{
	    BAN_DATA *pban;

	    if (feof(fp))
		break;

	    pban = new_ban();

	    pban->name = fread_string(fp);
	    pban->banner = fread_string(fp);
	    pban->level = fread_number(fp);
	    pban->ban_flags = fread_flag(fp);
	    pban->reason = fread_string(fp);
	    pban->when = fread_number(fp);
	    fread_to_eol(fp);

	    if (SLIST_EMPTY(&ban_list))
		SLIST_INSERT_HEAD(&ban_list, pban, link);
	    else
		SLIST_INSERT_AFTER(ban_last, pban, link);
	    
	    ban_last = pban;
	}
	
	file_close(fp);
    }
    else
    {
	bugf("Can't open bans file [%s]", BAN_FILE);
	_perror("fopen()");
    }
}

bool check_ban(char *site, int type)
{
    BAN_DATA *pban;
    char host[MAX_STRING_LENGTH];

    strcpy(host, capitalize(site));
    host[0] = LOWER (host[0]);

    SLIST_FOREACH(pban, &ban_list, link)
    {
        if (!IS_SET(pban->ban_flags, type))
            continue;

        if (IS_SET(pban->ban_flags, BAN_PREFIX)
            && IS_SET(pban->ban_flags, BAN_SUFFIX)
            && strstr(pban->name, host) != NULL)
	{
            return TRUE;
	}

        if (IS_SET(pban->ban_flags, BAN_PREFIX)
            && !str_suffix(pban->name, host))
	{
            return TRUE;
	}

        if (IS_SET(pban->ban_flags, BAN_SUFFIX)
            && !str_prefix(pban->name, host))
	{
            return TRUE;
	}
    }

    return FALSE;
}


void ban_site(CHAR_DATA * ch, char *argument, bool fPerm)
{
    char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    char *name;
    BUFFER *buffer;
    BAN_DATA *pban, *next, *prev = NULL;
    bool prefix = FALSE, suffix = FALSE;
    int type;

    smash_tilde(argument);

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0')
    {
        bool found = FALSE;

        if (SLIST_EMPTY(&ban_list))
        {
            send_to_char("No sites banned at this time.\n\r", ch);
            return;
        }
        buffer = new_buf();

        add_buf(buffer, "{WSite                       Banned By    Level Type     Status  When{w\n\r");
        add_buf(buffer, "{W----------------------------------------------------------------------------------------{w\n\r");
	SLIST_FOREACH(pban, &ban_list, link)
        {
            if (arg1[0] == '\0' || strstr(pban->name, arg1))
            {
		sprintf(buf2, "{c%s%s%s{w",
			IS_SET (pban->ban_flags, BAN_PREFIX) ? "*" : "",
			pban->name,
			IS_SET(pban->ban_flags, BAN_SUFFIX) ? "*" : "");

		sprintf(buf, "{c%-30s %-12s %-5d %-7s  %s    %s{w",
			buf2, pban->banner, pban->level,
			IS_SET(pban->ban_flags, BAN_NEWBIES) ? "newbies" :
			IS_SET(pban->ban_flags, BAN_PERMIT) ? "permit" :
			IS_SET(pban->ban_flags, BAN_ALL) ? "all" : 
			IS_SET(pban->ban_flags, BAN_WHOIS) ? "whois" : "",
			IS_SET(pban->ban_flags, BAN_PERMANENT)
				    ? "perm" : "temp",
			ctime(&pban->when));
		add_buf(buffer, buf);
		sprintf(buf,"{YReason: {y%s\n\r{w", pban->reason);
		add_buf(buffer, buf);
		found = TRUE;
            }
        }

        if (found)
	    page_to_char(buf_string (buffer), ch);
        else
	    send_to_char("Cannot find any matched bans.\n\r", ch);

        free_buf(buffer);
        return;
    }

    if (!str_cmp(arg1, "permit"))
    {
	CHAR_DATA *wch;

	if ((wch = get_char_world(ch, arg2)) == NULL || IS_NPC(wch))
	{
	    send_to_char("Таких нет в этом мире.\n\r", ch);
	    return;
	}

	if (IS_SET(wch->act, PLR_PERMIT))
	{
	    send_to_char("Ban permit cleared.\n\r", ch);
	    REMOVE_BIT(wch->act, PLR_PERMIT);
	}
	else
	{
	    send_to_char("Ban permit set.\n\r", ch);
	    SET_BIT(wch->act, PLR_PERMIT);
	}

	return;
    }

    if (argument[0] == '\0')
    {
	  send_to_char("Syntax: ban|permban <site> <type> <reason>\n\r\n\r"
		       "Acceptable ban types are all, newbies, permit and whois.\n\r\n\r"
		       "Yet another syntax: ban <site> - shows information about <site> ban.\n\r\n\r"
		       "Yet yet another syntax: ban permit <char name> - permits certain player plays regardless of ban.\n\r", ch);
	  return;
    }

    /* find out what type of ban */

    if (arg2[0] == '\0' || !str_prefix(arg2, "all"))
        type = BAN_ALL;
    else if (!str_prefix(arg2, "newbies"))
        type = BAN_NEWBIES;
    else if (!str_prefix(arg2, "permit"))
        type = BAN_PERMIT;
    else if (!str_prefix(arg2, "whois"))
        type = BAN_WHOIS;
    else
    {
        send_to_char("Acceptable ban types are all, newbies, permit and whois.\n\r", ch);
        return;
    }

    name = arg1;

    if (name[0] == '*')
    {
        prefix = TRUE;
        name++;
    }

    if (name[strlen(name) - 1] == '*')
    {
        suffix = TRUE;
        name[strlen(name) - 1] = '\0';
    }

    if (strlen(name) == 0)
    {
        send_to_char("You have to ban SOMETHING.\n\r", ch);
        return;
    }

    prev = NULL;
    for (pban = SLIST_FIRST(&ban_list); pban != NULL; pban = next)
    {
	next = SLIST_NEXT(pban, link);
        if (!str_cmp(name, pban->name))
        {
            if (pban->level > get_trust(ch))
            {
                send_to_char("That ban was set by a higher power.\n\r", ch);
                return;
            }
            else
            {
                if (prev == NULL)
		    SLIST_REMOVE_HEAD(&ban_list, link);
                else
		    SLIST_NEXT(prev, link) = SLIST_NEXT(pban, link);
                free_ban(pban);
		continue;
            }
        }
	prev = pban;
    }

    pban = new_ban();
    pban->banner = str_dup(ch->name);
    pban->name = str_dup(name);
    pban->level = get_trust(ch);
    pban->reason = str_dup(argument);
    pban->when = current_time;

    /* set ban type */
    pban->ban_flags = type;

    if (prefix)
        SET_BIT(pban->ban_flags, BAN_PREFIX);
    if (suffix)
        SET_BIT(pban->ban_flags, BAN_SUFFIX);
    if (fPerm)
        SET_BIT(pban->ban_flags, BAN_PERMANENT);

    SLIST_INSERT_HEAD(&ban_list, pban, link);
    save_bans();
    sprintf(buf, "You have banned the site %s for reason:\n\r%s\n\r", pban->name, pban->reason);
    send_to_char(buf, ch);
    return;
}

void do_ban(CHAR_DATA * ch, char *argument)
{
    ban_site(ch, argument, FALSE);
}

void do_permban(CHAR_DATA * ch, char *argument)
{
    ban_site(ch, argument, TRUE);
}

void do_allow(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    BAN_DATA *prev = NULL;
    BAN_DATA *curr;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Remove which site from the ban list?\n\r", ch);
        return;
    }

    prev = NULL;
    for (curr = SLIST_FIRST(&ban_list); curr != NULL; curr = SLIST_NEXT(curr, link))
    {
        if (!str_cmp(arg, curr->name))
        {
            if (curr->level > get_trust(ch))
            {
                send_to_char("You are not powerful enough to lift that ban.\n\r", ch);
                return;
            }
            if (prev == NULL)
                SLIST_REMOVE_HEAD(&ban_list, link);
            else
		SLIST_NEXT(prev, link) = SLIST_NEXT(curr, link);

            free_ban(curr);
            sprintf(buf, "Ban on %s lifted.\n\r", arg);
            send_to_char(buf, ch);
            save_bans();
            return;
        }
	prev = curr;
    }

    send_to_char("Site is not banned.\n\r", ch);
    return;
}
/* charset=cp1251 */
