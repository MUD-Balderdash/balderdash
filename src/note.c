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
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"
#include "recycle.h"
#include "interp.h"



/* globals from db.c for load_notes */
extern int _filbuf(FILE *);
extern FILE *fpArea;
extern char strArea[MAX_INPUT_LENGTH];

/* local procedures */
void load_thread(char *name, NOTE_DATA **list, int type, time_t free_time);
void parse_note(CHAR_DATA *ch, char *argument, int type);
bool hide_note(CHAR_DATA *ch, NOTE_DATA *pnote);
int is_clanmaster(CHAR_DATA *ch);

NOTE_DATA *note_list;
NOTE_DATA *idea_list;
NOTE_DATA *penalty_list;
NOTE_DATA *news_list;
NOTE_DATA *changes_list;
NOTE_DATA *votes_list;

int count_spool(CHAR_DATA *ch, NOTE_DATA *spool)
{
    int count = 0;
    NOTE_DATA *pnote;

    for (pnote = spool; pnote != NULL; pnote = pnote->next){
        if (!hide_note(ch,pnote)) {
            count++;
        }
    }


    return count;
}

void do_unread(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    int count, cm = 0;
    bool found = FALSE;

    if (!ch || IS_NPC(ch))
	return; 

    if (!IS_NULLSTR(argument) && str_cmp(argument, "quiet"))
	send_to_char("\n\r", ch); 

    if ((count = count_spool(ch, news_list)) > 0)
    {
	found = TRUE;
	sprintf(buf, "Тебя ждет %d %s.\n\r",
    		count, hours(count, TYPE_NEWS)); 
	send_to_char(buf, ch);
    }
    
    if ((count = count_spool(ch, changes_list)) > 0)
    {
	found = TRUE;
	sprintf(buf, "Тебя ждет %d %s.\n\r",
		count, hours(count, TYPE_CHANGES));
        send_to_char(buf, ch);
    }
    
    if ((count = count_spool(ch, note_list)) > 0)
    {
	found = TRUE;
	sprintf(buf, "У тебя %d %s ", count, hours(count, TYPE_UNREAD));
        strcat(buf, hours(count, TYPE_NOTES));  
        strcat(buf, ".\n\r");
	send_to_char(buf, ch);
    }

    if ((count = count_spool(ch, idea_list)) > 0)
    {
	found = TRUE;
	sprintf(buf, "%d %s ", count, hours(count, TYPE_IDEAS)); 
    	strcat(buf, hours(count, TYPE_VISIT));
    	strcat(buf, count == 1 ? " чью-то " : " чьи-то ");
    	strcat(buf, count == 1 ? "светлую " : "светлые ");
    	strcat(buf, count == 1 ? "голову" : "головы");
    	strcat(buf, ".\n\r");
	send_to_char(buf, ch);
    }

    if ((count = count_spool(ch, votes_list)) > 0)
    {
	found = TRUE;
	sprintf(buf, "Тебя ждет %d %s.\n\r", count, hours(count, TYPE_VOTES));
	send_to_char(buf, ch);
    }

    if (IS_TRUSTED(ch, AVATAR) && (count = count_spool(ch, penalty_list)) > 0)
    {
	found = TRUE;
	sprintf(buf, "%d %s было добавлено.\n\r",
		count, hours(count, TYPE_PENALTY));
	send_to_char(buf, ch);
    }

    if (!str_cmp(argument, "entering")
	&& (ch->level == MAX_LEVEL || (cm = is_clanmaster(ch)) > 0))
    {
	bool f = FALSE;

	f = FALSE;
	for (count = 1; count < MAX_CLAN; count++) 
	{
	    long stamp;
	    NOTE_DATA *pnote;

	    if (!clan_table[count].valid
		|| (cm != count && ch->level < MAX_LEVEL)
		|| !str_cmp(clan_table[count].name, CLAN_INDEPEND))
	    {
		continue;
	    }

	    stamp = 0;

	    for (pnote = note_list; pnote != NULL; pnote = pnote->next)
		if (stamp < pnote->date_stamp
		    && is_exact_name(clan_table[count].name, pnote->to_list)
		    && (is_exact_name("клану", pnote->to_list)
			|| is_exact_name("клан", pnote->to_list)
			|| is_exact_name("clan", pnote->to_list)))
		{
		    stamp = pnote->date_stamp;
		}

	    if (stamp < current_time - 60*60*24*7*CLAN_NEWS_EXPIRED)
	    {
		if (ch->level < MAX_LEVEL)
		    sprintf(buf, "\n\rВ твоем клане уже более %d недель"
			    " отсутствуют новости.", CLAN_NEWS_EXPIRED);
		else
		    sprintf(buf, "\n\rВ клане %s уже более %d недель"
			    " отсутствуют новости.",
			    clan_table[count].short_descr, CLAN_NEWS_EXPIRED);

		send_to_char(buf, ch);
		f = TRUE;
	    }
	}
	if (f)
	    send_to_char("\n\r", ch);
    } 

    if (str_cmp(argument, "quiet") && !found)
	send_to_char("У тебя нет непрочитанных сообщений.\n\r", ch);

}

void do_note(CHAR_DATA *ch, char *argument)
{
    parse_note(ch, argument, NOTE_NOTE);
}

void do_idea(CHAR_DATA *ch, char *argument)
{
    parse_note(ch, argument, NOTE_IDEA);
}

void do_penalty(CHAR_DATA *ch, char *argument)
{
    parse_note(ch, argument, NOTE_PENALTY);
}

void do_news(CHAR_DATA *ch,char *argument)
{
    parse_note(ch, argument, NOTE_NEWS);
}

void do_changes(CHAR_DATA *ch, char *argument)
{
    parse_note(ch, argument, NOTE_CHANGES);
}

void do_votes(CHAR_DATA *ch, char *argument)
{
    parse_note(ch, argument, NOTE_VOTES);
}

void fwrite_note(FILE *fp, NOTE_DATA *pnote)
{
    fprintf(fp, "Отправитель: %s~\n", pnote->sender);
    fprintf(fp, "Дата: %s~\n", pnote->date);
    fprintf(fp, "Stamp: %ld\n", (long)pnote->date_stamp);
    fprintf(fp, "Кому: %s~\n", pnote->to_list);
    fprintf(fp, "Тема: %s~\n", pnote->subject);
    fprintf(fp, "Сообщение:\n%s~\n", pnote->text);

    if (pnote->type == NOTE_VOTES)
    {
        VOTE_DATA *vd;
        CHAR_VOTES *cv;

        for (vd = pnote->vote; vd; vd = vd->next)
        {
            fprintf(fp, "%s\n0", vd->text);

            for (cv = vd->votes; cv; cv = cv->next)
            {
                if (!IS_VALID(cv))
                    continue;

                fprintf(fp, " %ld", cv->char_id);
            }

            fprintf(fp, "\n");
        }
        fprintf(fp, "~\n");
    } 
}

void save_notes(int type)
{
    FILE *fp;
    char *name;
    NOTE_DATA *pnote;

    switch (type)
    {
    default:
	return;
    case NOTE_NOTE:
	name = NOTE_FILE;
	pnote = note_list;
	break;
    case NOTE_IDEA:
	name = IDEA_FILE;
	pnote = idea_list;
	break;
    case NOTE_PENALTY:
	name = PENALTY_FILE;
	pnote = penalty_list;
	break;
    case NOTE_NEWS:
	name = NEWS_FILE;
	pnote = news_list;
	break;
    case NOTE_CHANGES:
	name = CHANGES_FILE;
	pnote = changes_list;
	break;
    case NOTE_VOTES:
	name = VOTES_FILE;
	pnote = votes_list;
	break;
    }

    if ((fp = file_open(name, "w")) == NULL)
    {
	_perror(name);
    }
    else
    {
        for (; pnote != NULL; pnote = pnote->next)
            fwrite_note(fp, pnote);
    }
    file_close(fp);
    return;
}

void load_notes(void)
{
    load_thread(NOTE_FILE,&note_list, NOTE_NOTE, 14*24*60*60);
    load_thread(IDEA_FILE,&idea_list, NOTE_IDEA, 28*24*60*60);
    load_thread(PENALTY_FILE,&penalty_list, NOTE_PENALTY, 0);
    load_thread(NEWS_FILE,&news_list, NOTE_NEWS, 0);
    load_thread(CHANGES_FILE,&changes_list,NOTE_CHANGES, 0);
    load_thread(VOTES_FILE,&votes_list,NOTE_VOTES, 56*24*60*60);
}

void load_thread(char *name, NOTE_DATA **list, int type, time_t free_time)
{
    FILE *fp;
    NOTE_DATA *pnotelast;
    NOTE_DATA *pnote = NULL;
 
    if ((fp = fopen(name, "r")) == NULL)
	return;
	 
    pnotelast = NULL;
    for (;;)
    {
	char letter;
	 
	if (pnote != NULL && pnotelast != pnote)
	    free_note(pnote);
	
	do
	{
	    letter = getc(fp);
            if (feof(fp))
            {
                fclose(fp);
                return;
            }
        }
        while (IS_SPACE(letter))
	    ;
	    
        ungetc(letter, fp);
 
        pnote = new_note();
 
        if (str_cmp(fread_word(fp), "Отправитель:"))
            break;

	free_string(pnote->sender);
        pnote->sender = fread_string(fp);
 
        if (str_cmp(fread_word(fp), "Дата:"))
            break;

	free_string(pnote->date);
        pnote->date = fread_string(fp);
 
        if (str_cmp(fread_word(fp), "Stamp:"))
            break;
        pnote->date_stamp = fread_number(fp);
 
        if (str_cmp(fread_word(fp), "Кому:"))
            break;

	free_string(pnote->to_list);
        pnote->to_list = fread_string(fp);
 
        if (str_cmp(fread_word(fp), "Тема:"))
            break;

	free_string(pnote->subject);
        pnote->subject = fread_string(fp);
 
        if (str_cmp(fread_word(fp), "Сообщение:"))
            break;

	free_string(pnote->text);
        pnote->text = fread_string(fp);

        if (type == NOTE_VOTES)
        {
            char *str, arg[MIL];
            VOTE_DATA *vd;
            CHAR_VOTES *cv;
            long tm;

            for(;;)
            {
                if (feof(fp) || (letter = getc(fp)) == '~')
                    break;

                ungetc(letter, fp);

                vd = new_vote_data();
                vd->next = pnote->vote;
                pnote->vote = vd;

                vd->text = fread_string_eol(fp);

                str = fread_string_eol(fp);

                for(str = one_argument(str, arg); !IS_NULLSTR(arg); str = one_argument(str, arg))
                   if (is_number(arg) && (tm = atoi(arg)) > 0)
                   {
                       cv = new_char_vote();
                       cv->char_id = tm;

                       cv->next = vd->votes;
                       vd->votes = cv;
                   }
            }
        } 

        if (free_time && pnote->date_stamp < current_time - free_time)
        {
	    free_note(pnote);
	    pnote = NULL;
            continue;
        }

	pnote->type = type;
 
        if (*list == NULL)
            *list = pnote;
        else
            pnotelast->next = pnote;
 
        pnotelast = pnote;
    }

    strcpy(strArea, NOTE_FILE);
    fpArea = fp;
    bugf("Load_notes: bad key word.");
    exit(1);
    return;
}

void append_note(NOTE_DATA *pnote)
{
    FILE *fp;
    char *name;
    NOTE_DATA **list;
    NOTE_DATA *last;

    switch(pnote->type)
    {
    default:
	return;
    case NOTE_NOTE:
	name = NOTE_FILE;
	list = &note_list;
	break;
    case NOTE_IDEA:
	name = IDEA_FILE;
	list = &idea_list;
	break;
    case NOTE_PENALTY:
	name = PENALTY_FILE;
	list = &penalty_list;
	break;
    case NOTE_NEWS:
	name = NEWS_FILE;
	list = &news_list;
	break;
    case NOTE_CHANGES:
	name = CHANGES_FILE;
	list = &changes_list;
	break;
    case NOTE_VOTES:
	name = VOTES_FILE;
	list = &votes_list;
	break;
    }

    if (*list == NULL)
	*list = pnote;
    else
    {
	for (last = *list; last->next != NULL; last = last->next);
	last->next = pnote;
    }

    if ((fp = file_open(name, "a")) == NULL)
    {
        _perror(name);
    }
    else
        fwrite_note(fp, pnote);

    file_close(fp);
}

bool is_note_to(CHAR_DATA *ch, NOTE_DATA *pnote)
{
    if (!str_cmp(ch->name, pnote->sender))
	return TRUE;

    if (is_exact_name("all", pnote->to_list)
	|| is_exact_name("все", pnote->to_list)
	|| is_exact_name("всем", pnote->to_list))
    {
	return TRUE;
    }

    if (IS_IMMORTAL(ch)
	&& (is_exact_name("immortal", pnote->to_list)
	    || is_exact_name("immortals", pnote->to_list)
	    || is_exact_name("богам", pnote->to_list)
	    || is_exact_name("иммам", pnote->to_list)))
    {
	return TRUE;
    }

    if (IS_IMMORTAL(ch)
	|| (is_clan(ch)
	    && is_exact_name(clan_table[ch->clan].name, pnote->to_list)))
    {
	return TRUE;
    }

    if (is_exact_name(ch->name, pnote->to_list))
	return TRUE;

    if (!str_cmp(ch->pcdata->spouse, pnote->sender)
	&& (is_exact_name("семье", pnote->to_list)
	    || is_exact_name("супругу", pnote->to_list)
	    || is_exact_name("супруге", pnote->to_list)
	    || is_exact_name("мужу", pnote->to_list)
	    || is_exact_name("жене", pnote->to_list)
	    || is_exact_name("family", pnote->to_list)
	    || is_exact_name("spouse", pnote->to_list)))
    {
	return TRUE;
    }

    if (((is_clan(ch)
		&& is_exact_name(clan_table[ch->clan].name, pnote->to_list))
	    || IS_IMMORTAL(ch))
        && !is_exact_name(pnote->to_list, CLAN_INDEPEND)
	&& (is_exact_name("клану", pnote->to_list)
	    || is_exact_name("клан", pnote->to_list)
	    || is_exact_name("clan", pnote->to_list)))
    {
	return TRUE;
    }

    return FALSE;
}



void note_attach(CHAR_DATA *ch, int type)
{
    NOTE_DATA *pnote;

    if (ch->pnote != NULL)
	return;

    pnote = new_note();

    pnote->next		= NULL;
    free_string(pnote->sender);
    pnote->sender	= str_dup(ch->name);
    pnote->type		= type;
    pnote->vote		= NULL;
    ch->pnote		= pnote;
    
    return;
}



void note_remove(CHAR_DATA *ch, NOTE_DATA *pnote, bool delete_flag)
{
    char to_new[MAX_INPUT_LENGTH];
    char to_one[MAX_INPUT_LENGTH];
    NOTE_DATA *prev;
    NOTE_DATA **list;
    char *to_list;

    if (!delete_flag)
    {
	/* make a new list */
        to_new[0]	= '\0';
        to_list	= pnote->to_list;
        while (*to_list != '\0')
        {
    	    to_list	= one_argument(to_list, to_one);
    	    if (to_one[0] != '\0' && str_cmp(ch->name, to_one))
	    {
	        strcat(to_new, " ");
	        strcat(to_new, to_one);
	    }
        }
        /* Just a simple recipient removal? */
       if (str_cmp(ch->name, pnote->sender) && to_new[0] != '\0')
       {
	   free_string(pnote->to_list);
	   pnote->to_list = str_dup(to_new + 1);
	   return;
       }
    }
    /* nuke the whole note */

    switch(pnote->type)
    {
    default:
	return;
    case NOTE_NOTE:
	list = &note_list;
	break;
    case NOTE_IDEA:
	list = &idea_list;
	break;
    case NOTE_PENALTY:
	list = &penalty_list;
	break;
    case NOTE_NEWS:
	list = &news_list;
	break;
    case NOTE_CHANGES:
	list = &changes_list;
	break;
    case NOTE_VOTES:
	list = &votes_list;
	break;
    }

    /*
     * Remove note from linked list.
     */
    if (pnote == *list)
    {
	*list = pnote->next;
    }
    else
    {
	for (prev = *list; prev != NULL; prev = prev->next)
	{
	    if (prev->next == pnote)
		break;
	}

	if (prev == NULL)
	{
	    bugf("Note_remove: pnote not found.");
	    return;
	}

	prev->next = pnote->next;
    }

    save_notes(pnote->type);
    free_note(pnote);
    return;
}

bool hide_note (CHAR_DATA *ch, NOTE_DATA *pnote)
{
    time_t last_read;

    if (IS_NPC(ch))
	return TRUE;

    switch (pnote->type)
    {
    default:
	return TRUE;
    case NOTE_NOTE:
	last_read = ch->pcdata->last_note;
	break;
    case NOTE_IDEA:
    	last_read = ch->pcdata->last_idea;
	break;
    case NOTE_PENALTY:
	last_read = ch->pcdata->last_penalty;
	break;
    case NOTE_NEWS:
	last_read = ch->pcdata->last_news;
	break;
    case NOTE_CHANGES:
	last_read = ch->pcdata->last_changes;
	break;
    case NOTE_VOTES:
	last_read = ch->pcdata->last_votes;
	break;
    }
    
    if (pnote->date_stamp <= last_read)
	return TRUE;

    if (!str_cmp(ch->name, pnote->sender))
	return TRUE;

    if (!is_note_to(ch, pnote))
	return TRUE;

    return FALSE;
}

void update_read(CHAR_DATA *ch, NOTE_DATA *pnote)
{
    time_t stamp;

    if (IS_NPC(ch))
	return;

    stamp = pnote->date_stamp;

    switch (pnote->type)
    {
    default:
	return;
    case NOTE_NOTE:
	ch->pcdata->last_note = UMAX(ch->pcdata->last_note, stamp);
	break;
    case NOTE_IDEA:
	ch->pcdata->last_idea = UMAX(ch->pcdata->last_idea, stamp);
	break;
    case NOTE_PENALTY:
	ch->pcdata->last_penalty = UMAX(ch->pcdata->last_penalty, stamp);
	break;
    case NOTE_NEWS:
	ch->pcdata->last_news = UMAX(ch->pcdata->last_news, stamp);
	break;
    case NOTE_CHANGES:
	ch->pcdata->last_changes = UMAX(ch->pcdata->last_changes, stamp);
	break;
    case NOTE_VOTES:
	ch->pcdata->last_votes = UMAX(ch->pcdata->last_votes, stamp);
	break;
    }
}

void show_note(NOTE_DATA *pnote, CHAR_DATA *ch, int vnum)
{
    char buf[MSL];

    if (vnum < 0)
        sprintf(buf, "%s: %s\n\rКому: %s\n\r",
	   	    pnote->sender,
	       	    pnote->subject,
		    pnote->to_list);
    else
        sprintf(buf, "[%3d] %s: %s\n\r%s\n\rКому: %s\n\r",
  		    vnum,
	   	    pnote->sender,
	       	    pnote->subject,
		    pnote->date,
		    pnote->to_list);

    send_to_char(buf, ch);
    page_to_char(pnote->text, ch);
     
    if (pnote->vote)
    {
        VOTE_DATA *vd;
        CHAR_VOTES *cv;
        int count_variants = 0, all_votes = 0;

        send_to_char("----------------------------------------------------------------\n\r", ch);

        for (vd = pnote->vote; vd; vd = vd->next)
            for(cv = vd->votes; cv; cv = cv->next)
                all_votes++;
            
        for (vd = pnote->vote; vd; vd = vd->next)
        {
            int count = 0, i, c;
            

            for(cv = vd->votes; cv; cv = cv->next)
                count++;
             
            printf_to_char("(%d) %s\n\r", ch, ++count_variants, vd->text);

            c = all_votes > 0 ? 50 * count / all_votes : 0;

            buf[0] = '\0';
            for (i = 0; i < c; i++)
               strcat(buf, "#");

            if (!IS_NULLSTR(buf))
                strcat(buf, " ");

            printf_to_char("%s%.2f%%(%d)\n\r\n\r", ch, buf, all_votes > 0 ? 100 * (float) count / all_votes : 0, count);
        }
    }

    update_read(ch, pnote);
}

void parse_note(CHAR_DATA *ch, char *argument, int type)
{
    DESCRIPTOR_DATA *d;
    BUFFER *buffer;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    NOTE_DATA *pnote;
    NOTE_DATA **list;
    char *list_name, *tm;
    int vnum;
    int anum;
    bool found = FALSE;

    if (IS_NPC(ch))
	return;

    if (IS_AFFECTED(ch, AFF_SLEEP))
    {
	send_to_char("Тебя усыпили, ты не можешь в таком состоянии работать с сообщениями.\n\r", ch);
	return;
    } 

    switch (type)
    {
    default:
      	return;
    case NOTE_NOTE:
	list = &note_list;
	list_name = "писем";
	break;
    case NOTE_IDEA:
	list = &idea_list;
	list_name = "идей";
	break;
    case NOTE_PENALTY:
	list = &penalty_list;
	list_name = "наказаний";
	break;
    case NOTE_NEWS:
	list = &news_list;
	list_name = "новостей";
	break;
    case NOTE_CHANGES:
	list = &changes_list;
	list_name = "изменений";
	break;
    case NOTE_VOTES:
	list = &votes_list;
	list_name = "опросов";
	break;
    }

    argument = one_argument(argument, arg);
    smash_tilde(argument);

    for (tm = argument; *tm != '\0'; tm++)
    {
	if (*tm == '{')
	{
	    found = TRUE;
	    break;
	}
    }

    if (found)
	strcat(argument,"{x");

    if (arg[0] == '\0'
	|| !str_prefix(arg, "read")
	|| !str_prefix(arg, "прочитать")
	|| !str_prefix(arg, "читать"))
    {
        bool fAll;
 
        if (!str_cmp(argument, "all") || !str_cmp(argument, "все"))
        {
            fAll = TRUE;
            anum = 0;
        }
        else if (argument[0] == '\0'
		|| !str_prefix(argument, "next")
		|| !str_prefix(argument, "следующее"))
        {
	    /* read next unread note */
            vnum = 0;
            for (pnote = *list; pnote != NULL; pnote = pnote->next)
            {
                if (!hide_note(ch, pnote))
                {
                    show_note(pnote, ch, vnum);
                    update_read(ch, pnote);
                    return;
                }
                else if (is_note_to(ch, pnote))
                    vnum++;
            }

	    sprintf(buf, "У тебя больше нет непрочитанных %s.\n\r", list_name);
	    send_to_char(buf, ch);
            return;
        }
        else if (is_number(argument))
        {
            fAll = FALSE;
            anum = atoi(argument);
        }
        else
        {
            send_to_char("Читать какой номер?\n\r", ch);
            return;
        }
 
        vnum = 0;
        for (pnote = *list; pnote != NULL; pnote = pnote->next)
        {
            if (is_note_to(ch, pnote) && (vnum++ == anum || fAll))
            {
                show_note(pnote, ch, anum);
                return;
            }
        }
 
	sprintf(buf, "Тебе еще не написали столько %s.\n\r", list_name);
	send_to_char(buf, ch);
        return;
    }

    if (!str_prefix(arg, "list") || !str_prefix(arg, "список"))
    {
        char *str;
        bool found = FALSE;
        char last_stamp[MSL];

        last_stamp[0] = '\0';

	vnum = 0;
	one_argument(argument, arg);

	buffer = new_buf();
	
	for (str = arg; !IS_NULLSTR(str); str++)
	    *str = UPPER(*str);

	for (pnote = *list; pnote != NULL; pnote = pnote->next)
	{
	    if (is_note_to(ch, pnote))
	    {
	        if (IS_NULLSTR(arg) || !str_infix(arg, pnote->subject) || !str_infix(arg, pnote->text))
	        {
	            char tmp[MSL], vts[MSL];

	            strftime(tmp, sizeof(tmp), "%Y-%m-%d", localtime(&pnote->date_stamp));

	            if (IS_NULLSTR(last_stamp) || strcmp(tmp, last_stamp))
	            {
	                strlcpy(last_stamp, tmp, sizeof(last_stamp));
	                printf_to_buffer(buffer, "{W%s:{x\n\r", last_stamp);
	            }

	            sprintf(tmp, "%s -> %s", pnote->sender, pnote->to_list);

	            if (pnote->type == NOTE_VOTES)
	            {
                        VOTE_DATA *vd;
                        CHAR_VOTES *cv;
                        int all_votes = 0;

                        for (vd = pnote->vote; vd; vd = vd->next)
                            for (cv = vd->votes; cv; cv = cv->next)
                                all_votes++;

                        sprintf(vts, " (%d)", all_votes);

	            }
	            else
	                vts[0] = '\0';

	  	    printf_to_buffer(buffer, "  [%3d%s] %-40s : %-40s%s\n\r",
				     vnum, hide_note(ch, pnote) ? " " : "N", 
				     tmp, pnote->subject, vts);
	  	    found = TRUE;
		}
		vnum++;
	    }
	}

	page_to_char(buffer->string, ch);
	free_buf(buffer);
	
	if (!found)
	{
	    switch(type)
	    {
	    case NOTE_NOTE:	
		send_to_char("Для тебя нет писем.\n\r", ch);
		break;
	    case NOTE_IDEA:
	      	send_to_char("Для тебя нет идей.\n\r", ch);
    		break;
	    case NOTE_PENALTY:
		send_to_char("Для тебя нет непрочитаных наказаний.\n\r", ch);
		break;
	    case NOTE_NEWS:
		send_to_char("Для тебя нет новостей.\n\r", ch);
		break;
	    case NOTE_CHANGES:
	      	send_to_char("Для тебя нет изменений.\n\r", ch);
    		break;
	    case NOTE_VOTES:
	      	send_to_char("Для тебя нет новых опросов.\n\r", ch);
    		break;
	    }
	}
	return;
    }

    if (!str_prefix(arg, "autor") || !str_prefix(arg, "автор"))
    {
        char *str;
        bool found = FALSE;
        char last_stamp[MSL];

        last_stamp[0] = '\0';

		vnum = 0;
		one_argument(argument, arg);

		buffer = new_buf();
	
		for (str = arg; !IS_NULLSTR(str); str++)
	    	*str = UPPER(*str);
		
		if (IS_NULLSTR(arg))
		{
			send_to_char("Введи имя автора.\n\r", ch);
			return;
		}

		for (pnote = *list; pnote != NULL; pnote = pnote->next)
		{
	    	if (is_note_to(ch, pnote))
	    	{
	        	if (!str_infix(arg, pnote->sender))
	        	{
	            	char tmp[MSL], vts[MSL];

	            	strftime(tmp, sizeof(tmp), "%Y-%m-%d", localtime(&pnote->date_stamp));

	            	if (IS_NULLSTR(last_stamp) || strcmp(tmp, last_stamp))
	            	{
	                	strlcpy(last_stamp, tmp, sizeof(last_stamp));
	                	printf_to_buffer(buffer, "{W%s:{x\n\r", last_stamp);
	            	}

	            	sprintf(tmp, "%s -> %s", pnote->sender, pnote->to_list);

	            	if (pnote->type == NOTE_VOTES)
	            	{
                        VOTE_DATA *vd;
                        CHAR_VOTES *cv;
                        int all_votes = 0;

                        for (vd = pnote->vote; vd; vd = vd->next)
                            for (cv = vd->votes; cv; cv = cv->next)
                                all_votes++;

                        sprintf(vts, " (%d)", all_votes);

	            	}
	            	else
	                	vts[0] = '\0';

	  	    		printf_to_buffer(buffer, "  [%3d%s] %-40s : %-40s%s\n\r", vnum, hide_note(ch, pnote) ? " " : "N", tmp, pnote->subject, vts);
	  	    		found = TRUE;
				}
				vnum++;
	    	}
		}

		page_to_char(buffer->string, ch);
		free_buf(buffer);
	
		if (!found)
		{
	    	switch(type)
	    	{
	    		case NOTE_NOTE:	
					send_to_char("Для тебя нет писем.\n\r", ch);
					break;
	    		case NOTE_IDEA:
	      			send_to_char("Для тебя нет идей.\n\r", ch);
    				break;
	    		case NOTE_PENALTY:
					send_to_char("Для тебя нет непрочитаных наказаний.\n\r", ch);
					break;
	    		case NOTE_NEWS:
					send_to_char("Для тебя нет новостей.\n\r", ch);
					break;
	    		case NOTE_CHANGES:
	      			send_to_char("Для тебя нет изменений.\n\r", ch);
    				break;
	    		case NOTE_VOTES:
	      			send_to_char("Для тебя нет новых опросов.\n\r", ch);
    				break;
	    	}
		}
		return;
    }

    if (!str_prefix(arg, "theme") || !str_prefix(arg, "заголовок"))
    {
        char *str;
        bool found = FALSE;
        char last_stamp[MSL];

        last_stamp[0] = '\0';

		vnum = 0;
		one_argument(argument, arg);

		buffer = new_buf();
	
		for (str = arg; !IS_NULLSTR(str); str++)
	    	*str = UPPER(*str);
		
		if (IS_NULLSTR(arg))
		{
			send_to_char("Введи слова для поиска в темах писем.\n\r", ch);
			return;
		}

		for (pnote = *list; pnote != NULL; pnote = pnote->next)
		{
	    	if (is_note_to(ch, pnote))
	    	{
	        	if (!str_infix(arg, pnote->subject))
	        	{
	            	char tmp[MSL], vts[MSL];

	            	strftime(tmp, sizeof(tmp), "%Y-%m-%d", localtime(&pnote->date_stamp));

	            	if (IS_NULLSTR(last_stamp) || strcmp(tmp, last_stamp))
	            	{
	                	strlcpy(last_stamp, tmp, sizeof(last_stamp));
	                	printf_to_buffer(buffer, "{W%s:{x\n\r", last_stamp);
	            	}

	            	sprintf(tmp, "%s -> %s", pnote->sender, pnote->to_list);

	            	if (pnote->type == NOTE_VOTES)
	            	{
                        VOTE_DATA *vd;
                        CHAR_VOTES *cv;
                        int all_votes = 0;

                        for (vd = pnote->vote; vd; vd = vd->next)
                            for (cv = vd->votes; cv; cv = cv->next)
                                all_votes++;

                        sprintf(vts, " (%d)", all_votes);

	            	}
	            	else
	                	vts[0] = '\0';

	  	    		printf_to_buffer(buffer, "  [%3d%s] %-40s : %-40s%s\n\r", vnum, hide_note(ch, pnote) ? " " : "N", tmp, pnote->subject, vts);
	  	    		found = TRUE;
				}
				vnum++;
	    	}
		}

		page_to_char(buffer->string, ch);
		free_buf(buffer);
	
		if (!found)
		{
	    	switch(type)
	    	{
	    		case NOTE_NOTE:	
					send_to_char("Для тебя нет писем.\n\r", ch);
					break;
	    		case NOTE_IDEA:
	      			send_to_char("Для тебя нет идей.\n\r", ch);
    				break;
	    		case NOTE_PENALTY:
					send_to_char("Для тебя нет непрочитаных наказаний.\n\r", ch);
					break;
	    		case NOTE_NEWS:
					send_to_char("Для тебя нет новостей.\n\r", ch);
					break;
	    		case NOTE_CHANGES:
	      			send_to_char("Для тебя нет изменений.\n\r", ch);
    				break;
	    		case NOTE_VOTES:
	      			send_to_char("Для тебя нет новых опросов.\n\r", ch);
    				break;
	    	}
		}
		return;
    }


    if (!str_prefix(arg, "remove") || !str_prefix(arg, "удалить"))
    {
        if (!is_number(argument))
        {
            send_to_char("Удалить сообщение под каким номером?\n\r", ch);
            return;
        }
 
        anum = atoi(argument);
        vnum = 0;
        for (pnote = *list; pnote != NULL; pnote = pnote->next)
        {
            if (is_note_to(ch, pnote) && vnum++ == anum)
            {
                note_remove(ch, pnote, FALSE);
                send_to_char("Ok.\n\r", ch);
                return;
            }
        }
 
	sprintf(buf, "Тебе еще не написали столько %s.\n\r", list_name);
	send_to_char(buf, ch);
        return;
    }
 
    if ((!str_prefix(arg, "delete") || !str_prefix(arg, "уничтожить"))
        && (get_trust(ch) >= MAX_LEVEL - 3 || is_spec_granted(ch, "post_delete")))
    {
        if (!is_number(argument))
        {
            send_to_char("Удалить сообщение под каким номером?\n\r", ch);
            return;
        }
 
        anum = atoi(argument);
        vnum = 0;
        for (pnote = *list; pnote != NULL; pnote = pnote->next)
        {
            if (is_note_to(ch, pnote) && vnum++ == anum)
            {
                note_remove(ch, pnote, TRUE);
                send_to_char("Ok.\n\r", ch);
                return;
            }
        }

 	sprintf(buf, "Тебе еще не написали столько %s.\n\r", list_name);
	send_to_char(buf, ch);
        return;
    }

    if (!str_prefix(arg, "catchup") || !str_prefix(arg, "пометить"))
    {
	switch(type)
	{
	case NOTE_NOTE:
	    ch->pcdata->last_note = current_time;
	    break;
	case NOTE_IDEA:
	    ch->pcdata->last_idea = current_time;
	    break;
	case NOTE_PENALTY:
	    ch->pcdata->last_penalty = current_time;
	    break;
	case NOTE_NEWS:
	    ch->pcdata->last_news = current_time;
	    break;
	case NOTE_CHANGES:
    	    ch->pcdata->last_changes = current_time;
	    break;
	case NOTE_VOTES:
    	    ch->pcdata->last_votes = current_time;
	    break;
	}

	send_to_char("Ok.\n\r", ch);
	return;
    }

    /* below this point only certain people can edit notes */
    if ((type == NOTE_NEWS && !IS_TRUSTED(ch, AVATAR))
	|| (type == NOTE_CHANGES && !IS_TRUSTED(ch, IMMORTAL)))
    {
	sprintf(buf, "У тебя недостаточный уровнь для редактирования %s.\n\r",
		list_name);
	send_to_char(buf, ch);
	return;
    }

    if (!str_cmp(arg, "+"))
    {
	if (IS_SET(ch->comm, COMM_NONOTES))
	{
	    send_to_char("Боги запретили тебе писать сообщения.\n\r", ch);
	    return;
	}
    
	note_attach(ch, type);
	if (ch->pnote->type != type)
	{
	    send_to_char("Ты уже занимаешься другим сообщением.\n\r", ch);
	    return;
	}

	if (strlen(ch->pnote->text) + strlen(argument) >= 4096)
	{
	    send_to_char("Сообщение слишком длинное.\n\r", ch);
	    return;
	}

 	buffer = new_buf();

	add_buf(buffer, ch->pnote->text);
	add_buf(buffer, argument);
	add_buf(buffer, "\n\r");
	free_string(ch->pnote->text);
	ch->pnote->text = str_dup(buf_string(buffer));
	free_buf(buffer);
	send_to_char("Ok.\n\r", ch);
	return;
    }

    if (!str_cmp(arg,"-"))
    {
 	int len;
	bool found = FALSE;

	if (IS_SET(ch->comm, COMM_NONOTES))
	{
	    send_to_char("Боги запретили тебе писать сообщения.\n\r", ch);
	    return;
	}
    
	note_attach(ch, type);
        if (ch->pnote->type != type)
        {
            send_to_char("Ты уже занимаешься другим сообщением.\n\r", ch);
            return;
        }

	if (ch->pnote->text == NULL || ch->pnote->text[0] == '\0')
	{
	    send_to_char("Больше нет строк для удаления.\n\r",ch);
	    return;
	}

	strcpy(buf, ch->pnote->text);

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
		    free_string(ch->pnote->text);
		    ch->pnote->text = str_dup(buf);
		    return;
		}
	    }
	}
	buf[0] = '\0';
	free_string(ch->pnote->text);
	ch->pnote->text = str_dup(buf);
	return;
    }

    if (!str_cmp(arg, "write")
	|| !str_cmp(arg, "написать")
	|| !str_cmp(arg, "edit"))
    {
	if (IS_SET(ch->comm, COMM_NONOTES))
	{
	    send_to_char("Боги запретили тебе писать сообщения.\n\r", ch);
	    return;
	}
    
        note_attach(ch, type);
        if (argument[0] == '\0')
	    string_append(ch, &ch->pnote->text);
        return;
    }

    if (!str_prefix(arg, "subject") || !str_prefix(arg, "тема"))
    {
	if (IS_SET(ch->comm, COMM_NONOTES))
	{
	    send_to_char("Боги запретили тебе писать сообщения.\n\r", ch);
	    return;
	}
    
	note_attach(ch, type);
        if (ch->pnote->type != type)
        {
            send_to_char("Ты уже занимаешься другой запиской.\n\r", ch);
            return;
        }

	free_string(ch->pnote->subject);
   	ch->pnote->subject = str_dup(argument);
	send_to_char("Ok.\n\r", ch);
	return;
    }

    if (!str_prefix(arg, "to") || !str_prefix(arg, "кому"))
    {
	char buf[MAX_INPUT_LENGTH];

	if (IS_SET(ch->comm, COMM_NONOTES))
	{
	    send_to_char("Боги запретили тебе писать сообщения.\n\r", ch);
	    return;
	}
    
	note_attach(ch, type);
        if (ch->pnote->type != type)
        {
            send_to_char("Ты уже занимаешься другой запиской.\n\r", ch);
            return;
        }

        if (!str_cmp(argument, "клану")
	    || !str_cmp(argument, "клан")
	    || !str_cmp(argument, "clan"))
        {
	    if (!is_clan(ch) || IS_INDEPEND(ch))
	    {
		send_to_char("Но ведь ты же не в клане!\n\r", ch);
		return;
	    }	    
		
		sprintf(buf, "%s %s", argument, clan_table[ch->clan].name);
        }
        else
		{		
        	if ((ch->pcdata->successed)
			|| (!str_cmp(argument, "боги")
	    	|| !str_cmp(argument, "богам")
	    	|| !str_cmp(argument, "immortal")))
        	{
			}
			else
			{
				send_to_char("Пока ты не прошел одобрение, ты можешь писать лишь богам. Набери ? {yодобрение{x.\n\r", ch);
				return;
	    	}
		    strcpy(buf, argument);
		}


	free_string(ch->pnote->to_list);
	ch->pnote->to_list = str_dup(buf);
	send_to_char("Ok.\n\r", ch);
	return;
    }

    if (!str_prefix(arg, "clear") || !str_prefix(arg, "стереть"))
    {
	if (ch->pnote != NULL)
	{
	    free_note(ch->pnote);
	    ch->pnote = NULL;
	}

	send_to_char("Ok.\n\r", ch);
	return;
    }

    if (!str_prefix(arg, "show") || !str_prefix(arg, "показать"))
    {
	if (ch->pnote == NULL)
	{
	    send_to_char("Ты еще пока не пишешь никаких сообщений.\n\r", ch);
	    return;
	}

	if (ch->pnote->type != type)
	{
	    send_to_char("Ты работаешь над другим типом сообщения.\n\r", ch);
	    return;
	}

	show_note(ch->pnote, ch, -1);
/*	sprintf(buf, "%s: %s\n\rКому: %s\n\r",
		ch->pnote->sender,
		ch->pnote->subject,
		ch->pnote->to_list);

	send_to_char(buf, ch);
	send_to_char(ch->pnote->text, ch); */
	return;
    }

    if (!str_prefix(arg, "post")
	|| !str_prefix(arg, "send")
	|| !str_prefix(arg, "послать"))
    {
	char *strtime;

	if (IS_SET(ch->comm, COMM_NONOTES))
	{
	    send_to_char("Боги запретили тебе писать сообщения.\n\r", ch);
	    return;
	}
    
	if (ch->pnote == NULL)
	{
	    printf_to_char("Ты же еще ничего не написал%s.\n\r", ch, SEX_ENDING(ch));
	    return;
	}

        if (ch->pnote->type != type)
        {
            send_to_char("Ты работаешь над другим типом записки.\n\r", ch);
            return;
        }

	if (!str_cmp(ch->pnote->to_list, ""))
	{
	    send_to_char("Надо указать получателя (имя, всем, Богам, "
			 "семье, клану).\n\r", ch);
	    return;
	}

	if (!str_cmp(ch->pnote->subject, ""))
	{
	    send_to_char("Необходимо указать тему письма.\n\r", ch);
	    return;
	}

	if (ch->pnote->type == NOTE_VOTES && ch->pnote->vote == NULL)
	{
	    send_to_char("Необходимо создать хотя бы один вариант ответа.\n\r", ch);
	    return;
	}

	ch->pnote->next			= NULL;
	strtime				= ctime(&current_time);
	strtime[strlen(strtime) - 1]	= '\0';
	free_string(ch->pnote->date);
	ch->pnote->date			= str_dup(strtime);
	ch->pnote->date_stamp		= current_time;

	append_note(ch->pnote);
	ch->pnote = NULL;

	send_to_char("Твое сообщение послано адресатам.\n\r", ch);

	SLIST_FOREACH(d, &descriptor_list, link)
	{
	    CHAR_DATA *wch = CH(d);
	    
	    if (d->connected == CON_PLAYING && wch && wch != ch)
		do_unread(wch, "quiet");
	}
	
	return;
    }
    
    if (type == NOTE_VOTES)
    {
        if (!str_prefix(arg, "вариант") || !str_prefix(arg, "variant"))
        {
	    char buf[MIL];
	    VOTE_DATA *vd;

	    if (IS_SET(ch->comm, COMM_NONOTES))
	    {
	        send_to_char("Боги запретили тебе писать сообщения.\n\r", ch);
	        return;
	    }
    
	    note_attach(ch, type);
            if (ch->pnote->type != type)
            {
                send_to_char("Ты уже занимаешься другой запиской.\n\r", ch);
                return;
            }

            argument = one_argument(argument, buf);

            if (!str_cmp(buf, "+"))
            {
                vd = new_vote_data();

                smash_tilde(argument);
                vd->text = str_dup(argument);

                vd->next = ch->pnote->vote;
                ch->pnote->vote = vd;

                send_to_char("Ok.\n\r", ch);
                return;
            }
            if (!str_cmp(buf, "-"))
            {
                if ((vd = ch->pnote->vote) != NULL)
                {
                    ch->pnote->vote = vd->next;
                    free_vote_data(vd);
                    
                    send_to_char("Ok.\n\r", ch);
                }
                else
                    send_to_char("Не создано еще ни одного варианта.\n\r", ch);

                return;
            }
        }

        if (!str_prefix(arg, "проголосовать") || !str_prefix(arg, "vote") || !str_prefix(arg, "голосовать"))
        {
            int variant, num, vnum;

            argument = one_argument(argument, arg);
            if (!is_number(arg))
            {
                send_to_char("Аргументы должны быть числовыми.\n\r", ch);
                return;
            }
            num = atoi(arg);
            
            argument = one_argument(argument, arg);                     
            if (!is_number(arg))
            {
                send_to_char("Аргументы должны быть числовыми.\n\r", ch);
                return;
            }
            variant = atoi(arg);
            vnum = 0;

            for (pnote = *list; pnote != NULL; pnote = pnote->next)
            {
                if (is_note_to(ch, pnote) && vnum++ == num)
                {
                    VOTE_DATA *vd, *vd_curr = NULL;
                    CHAR_VOTES *cv;
                    int count_variants = 0;

                    for (vd = pnote->vote; vd; vd = vd->next)
                    {
                        if (++count_variants == variant)
                            vd_curr = vd;
                    
                        for (cv = vd->votes; cv; cv = cv->next)
                            if (cv->char_id == ch->id)
                            {
                                printf_to_char("Ты уже участвовал%s в этом опросе.\n\r", ch, SEX_ENDING(ch));
                                return;
                            }
                    }

                    if (!vd_curr)
                    {
                        send_to_char("Нет такого варианта ответа.\n\r", ch);
                        return;
                    }
                    
                    cv = new_char_vote();

                    cv->char_id = ch->id;

                    cv->next = vd_curr->votes;
                    vd_curr->votes = cv;
                    
                    show_note(pnote, ch, num);
                    save_notes(NOTE_VOTES);
                    return;
                }
            }

	    sprintf(buf, "Тебе еще не написали столько %s.\n\r", list_name);
	    send_to_char(buf, ch);
           
            return;
        }
    }
     
    send_to_char("Ты не можешь этого сделать.\n\r", ch);
    return;
}

/* charset=cp1251 */



