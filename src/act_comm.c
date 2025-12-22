/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 * Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
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
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "merc.h"
#include "interp.h"
#include "recycle.h"


bool is_compatible_races(void *ch1, void *ch2, bool is_char_data);

bool is_marry(CHAR_DATA *ch1, CHAR_DATA *ch2);

int is_clanmaster(CHAR_DATA *ch);

/* RT code to delete yourself */

void quest_update(CHAR_DATA *ch);


bool check_filter(CHAR_DATA *ch, CHAR_DATA *victim)
{
    if (!ch || !victim)
	return FALSE;

    if (!IS_NPC(victim))
    {
	int i;

	for (i = 0; i < MAX_FILTERS; i++)
	    if (victim->pcdata->filter[i].ch != NULL && !str_cmp(victim->pcdata->filter[i].ch, ch->name))
		return TRUE;
    }

    return FALSE;
}

bool check_channel_level(CHAR_DATA *ch)
{
    if (ch->level < PK_RANGE/2)
    {
	send_to_char("Тебе нужно немного подрасти, чтобы начать пользоваться этим каналом.\n\r", ch);
	return FALSE;
    }

    return TRUE;
}

void do_quote(CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];
    int count, cn = 0;

    for (count = 0; count < MAX_QUOTES && !IS_NULLSTR(quote_table[count]); count++)
	if (number_range(0, count) == 0)
	    cn = count;

    if (!IS_NULLSTR(quote_table[cn]))
    {
	sprintf(buf, "\n\r{DФраза дня:{x {W%s{x\n\r\n\r", quote_table[cn]);
	send_to_char(buf, ch);
    }
    return;
}


/*
 * Extract a char from the world.
 */
void extract_char_for_quit(CHAR_DATA *ch, bool show)
{
    CHAR_DATA *wch;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    if (show)
	nuke_pets(ch);

    ch->pet = NULL; /* just in case */

    die_follower(ch);

    stop_fighting(ch, TRUE);

    for (obj = ch->carrying; obj != NULL; obj = obj_next)
    {
	obj_next = obj->next_content;
	extract_obj(obj, FALSE, FALSE);
    }

    if (ch->mount != NULL && ch->mount->mount == ch)
    {
	ch->mount->mount = NULL;

	if (ch->mount->riding)
	{
	    if (show)
	    {
		act("Ты падаешь с $N1!", ch->mount, NULL, ch, TO_CHAR);
		act("$n падает со спины $N1.", ch->mount, NULL, ch, TO_ROOM);
	    }
	    ch->mount->riding = FALSE;
	    if (!IS_IMMORTAL(ch->mount))
		ch->mount->position = POS_SITTING;
	}
    }

    if (ch->desc != NULL && ch->desc->original != NULL)
    {
	do_function(ch, &do_return, "");
	ch->desc = NULL;
    }

    if (ch->in_room != NULL && show)
	char_from_room(ch);

    LIST_FOREACH(wch, &char_list, link)
    {
	if (wch->reply == ch)
	    wch->reply = NULL;
	if (ch->mprog_target == wch)
	    wch->mprog_target = NULL;
    }

    if (show)
    {
	LIST_REMOVE(ch, link);
    }

    if (!IS_NPC(ch) && ch->pcdata->beacon != NULL)
    {
	extract_obj(ch->pcdata->beacon, FALSE, FALSE);
	ch->pcdata->beacon = NULL;
    }

    if (ch->desc != NULL)
	ch->desc->character = NULL;

    free_char(ch);
    return;
}



void do_delet(CHAR_DATA *ch, char *argument)
{
    send_to_char("Для удаления персонажа надо написать команду ПОЛНОСТЬЮ.\n\r",
		 ch);
}

bool can_quit(CHAR_DATA *ch)
{
    extern CHAR_DATA *who_claim_reboot;
    char buf[MAX_STRING_LENGTH];
    int i;

    if (POS_FIGHT(ch))
    {
	send_to_char("Э, нет! Ты еще сражаешься!\n\r", ch);
	return FALSE;
    }

    if (ch->position <= POS_STUNNED )
    {
	send_to_char("Дождись своей смерти.\n\r", ch);
	return FALSE;
    }

    if (IS_AFFECTED(ch, AFF_CHARM))
    {
	send_to_char("Стоп, стоп. У тебя есть хозяин, дождись, пока он тебя отпустит.\n\r", ch);
	return FALSE;
    }

    if (IS_AFFECTED(ch, AFF_SLEEP))
    {
	send_to_char("Ты не можешь проснуться!\n\r", ch);
	return FALSE;
    }

    if (IS_SET(ch->act, PLR_EXCITED))
    {
	sprintf(buf, "Ты слишком взволнован%s предыдущей дракой!\n\r",
		SEX_ENDING(ch));
	send_to_char(buf, ch);
	return FALSE;
    }

    if (ch == who_claim_reboot)
    {
	send_to_char("Стоп-стоп, надо сначала завершить процесс перезагрузки!\n\r", ch);
	return FALSE;
    }

    if (ch == immquest.who_claim)
    {
	send_to_char("Стоп-стоп, надо сначала завершить квест, данный игрокам!\n\r", ch);
	return FALSE;
    }

    for (i = 0; i < MAX_AUCTION; i++)
    {
	if (auctions[i].valid)
	{
	    if (auctions[i].buyer == ch)
	    {
	        sprintf (buf, "{aПодожди, ты сделал%s ставку на аукционе #%d. Дождись либо окончания аукциона, "
		     "либо момента, когда твоя ставка будет перекрыта кем-нибудь.{x\n\r", SEX_ENDING(ch), i + 1);
	        send_to_char(buf, ch);
	        return FALSE;
	    }

	    if (auctions[i].sender == ch)
	    {
	        sprintf (buf, "{aПодожди, ты поставил%s на аукцион #%d какую-то вещь. "
	                      "Дождись окончания аукциона.{x" , SEX_ENDING(ch), i + 1);
	        send_to_char(buf, ch);
	        return FALSE;
	    }
	}
    }	

    if (IS_SET(ch->act, PLR_CHALLENGER)
	&& IS_SET(ch->in_room->room_flags, ROOM_ARENA))
    {
	send_to_char("Сначала закончи свои дела на арене!\n\r", ch);
	return FALSE;
    }

    if ((ch->pcdata->questmob > 0 || ch->pcdata->questobj > 0)
	&& ch->pcdata->countdown > 0)
    {
	send_to_char("Тебе необходимо завершить квест.\n\r", ch);
	return FALSE;
    } 

    if (ch->desc && ch->desc->editor)
    {
	send_to_char("Тебе необходимо закончить редактирование.\n\r", ch);
	return FALSE;
    } 
    
    REMOVE_BIT(ch->act, PLR_CHALLENGER);
    return TRUE;
}

void do_delete(CHAR_DATA *ch, char *argument)
{
    char strsave[MAX_INPUT_LENGTH];

    if (IS_NPC(ch))
	return;

    if (IS_IMMORTAL(ch))
    {
		send_to_char("Переговори с верховными Богами, возможно, эту проблему "
		     "можно решить по-другому.\n\r", ch);
		return;
    }

    if (is_clanmaster(ch))
    {
		send_to_char("Сначала сними с себя полномочия кланмастера!\n\r", ch);
		return;
    }

    if (IS_AFFECTED(ch, AFF_GODS_SKILL)
	|| is_affected(ch, gsn_gods_curse)
	|| is_affected(ch, gsn_nocast))
    {
		send_to_char("Ты был наказан Богами, сначала до конца отсиди "
		     "положенное наказание.\n\r", ch);
		return;
    }

    if (!can_quit(ch))
	return;

    if (IS_SET(ch->act, PLR_CONFIRM_DELETE))
    {
		if (ch->pcdata->reg_answer != NULL && (strlen(ch->pcdata->reg_answer) > 0))
		{
			if (strcmp(argument, ch->pcdata->reg_answer))
			{
				send_to_char("Статус удаления убран.\n\r", ch);
				REMOVE_BIT(ch->act, PLR_CONFIRM_DELETE);
				return;
			}
			else
			{
				sprintf(strsave, "%s%s", PLAYER_DIR, capitalize(ch->name));
				wiznet("$N удаляет себя.", ch, NULL, 0, 0, 0);
		
				if (is_clan(ch))
					clan_table[ch->clan].count--;
		
				return_limit_from_char(ch, TRUE);
				do_function(ch, &do_quit, "delete");
				unlink(strsave);
				return;
			}
		}
		else
		{
			send_to_char("У тебя отсутствует ответ на регистрационный вопрос.\n\r", ch);
			send_to_char("Обратись к богам для решения этой проблемы.\n\r", ch);
			return;
		}
    }

    if (argument[0] != '\0')
    {
		send_to_char("Просто напиши команду без аргументов.\n\r", ch);
		return;
    }

    send_to_char("Наберите 'удалить <ваш ответ на регистрационный вопрос>' для подтверждения удаления "
		 "персонажа.\n\r", ch);
    send_to_char("{RВНИМАНИЕ: действие этой команды отменить нельзя!{x\n\r",
		 ch);
    send_to_char("(Наберите 'удалить' с аргументом, отличным от вашего ответа на регистрационный вопрос для "
		 "отмены статуса удаления).\n\r", ch);

    SET_BIT(ch->act, PLR_CONFIRM_DELETE);
    wiznet("$N намеревается удалить себя.", ch, NULL, 0, 0, get_trust(ch));
}


/* RT code to display channel status */

void do_channels(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    /* lists all channels and their status */
    send_to_char("Канал        | Статус\n\r", ch);
    send_to_char("-------------+-------\n\r", ch);

    send_to_char("{dБолтовня{x     | ", ch);
    if (!IS_SET(ch->comm, COMM_NOGOSSIP))
	send_to_char("ON\n\r", ch);
    else
	send_to_char("OFF\n\r", ch);

    send_to_char("{DOOC{x          | ", ch);
    if (!IS_SET(ch->comm, COMM_NOOOC))
	send_to_char("ON\n\r", ch);
    else
	send_to_char("OFF\n\r", ch);

    send_to_char("{aАукцион{x      | ", ch);
    if (!IS_SET(ch->comm, COMM_NOAUCTION))
	send_to_char("ON\n\r", ch);
    else
	send_to_char("OFF\n\r", ch);

    send_to_char("{eМузыка{x       | ", ch);
    if (!IS_SET(ch->comm, COMM_NOMUSIC))
	send_to_char("ON\n\r", ch);
    else
	send_to_char("OFF\n\r", ch);


    send_to_char("{eСквернословие{x| ", ch);
    if (!IS_SET(ch->comm, COMM_NOSLANG))
	send_to_char("ON\n\r", ch);
    else
	send_to_char("OFF\n\r", ch);

    send_to_char("{gКлан{x         | ", ch);
    if (!IS_SET(ch->comm, COMM_NOCLAN))
	send_to_char("ON\n\r", ch);
    else
	send_to_char("OFF\n\r", ch);

    send_to_char("{GПоздравления{x | ", ch);
    if (!IS_SET(ch->comm, COMM_NOGRATS))
	send_to_char("ON\n\r", ch);
    else
	send_to_char("OFF\n\r", ch);

    if (IS_IMMORTAL(ch))
    {
	send_to_char("{iGod channel{x  | ", ch);
	if(!IS_SET(ch->comm, COMM_NOWIZ))
	    send_to_char("ON\n\r", ch);
	else
	    send_to_char("OFF\n\r", ch);
    }

    send_to_char("{MОрать{x        | ", ch);
    if (!IS_SET(ch->comm, COMM_SHOUTSOFF))
	send_to_char("ON\n\r", ch);
    else
	send_to_char("OFF\n\r", ch);

    send_to_char("{RПриват{x       | ", ch);
    if (!IS_SET(ch->comm, COMM_DEAF))
	send_to_char("ON\n\r", ch);
    else
	send_to_char("OFF\n\r", ch);

    send_to_char("{BПомощь{x       | ", ch);
    if (IS_SET(ch->comm, COMM_HELPER))
	send_to_char("ON\n\r", ch);
    else
	send_to_char("OFF\n\r", ch);

    send_to_char("\n\rТишина...    | ", ch);
    if (IS_SET(ch->comm, COMM_QUIET))
	send_to_char("ON\n\r", ch);
    else
	send_to_char("OFF\n\r", ch);

    if (IS_SET(ch->comm, COMM_AFK))
	send_to_char("Включен статус ВОК.\n\r", ch);

    if (IS_SET(ch->comm, COMM_SNOOP_PROOF))
	send_to_char("У тебя иммунитет к подглядыванию.\n\r", ch);

    if (ch->lines != PAGELEN)
    {
	if (ch->lines)
	{
	    sprintf(buf, "Вам показывается %d строк.\n\r", ch->lines+2);
	    send_to_char(buf, ch);
	}
	else
	    send_to_char("Буфер прокрутки отключен.\n\r", ch);
    }

    if (IS_SET(ch->comm, COMM_NOSHOUT))
	send_to_char("Ты не можешь орать.\n\r", ch);

    if (IS_SET(ch->comm, COMM_NOTELL))
	send_to_char("Ты не можешь говорить.\n\r", ch);

    if (IS_SET(ch->comm, COMM_NOCHANNELS))
	send_to_char("Ты не можешь использовать каналы.\n\r", ch);

    if (IS_SET(ch->comm, COMM_NOEMOTE))
	send_to_char("Ты не можешь показывать свои эмоции.\n\r", ch);

}

/* RT deaf blocks out all shouts */

void do_deaf(CHAR_DATA *ch, char *argument)
{

    if (IS_SET(ch->comm, COMM_DEAF))
    {
	send_to_char("Теперь ты снова можешь слышать, что тебе говорят.\n\r", ch);
	REMOVE_BIT(ch->comm, COMM_DEAF);
    }
    else
    {
	send_to_char("Теперь ты не сможешь услышать, что тебе говорят.\n\r", ch);
	SET_BIT(ch->comm, COMM_DEAF);
    }
}

/* RT quiet blocks out all communication */

void do_quiet (CHAR_DATA *ch, char *argument)
{
    if (IS_SET(ch->comm, COMM_QUIET))
    {
	send_to_char("Режим тишины выключен.\n\r", ch);
	REMOVE_BIT(ch->comm, COMM_QUIET);
    }
    else
    {
	send_to_char("Теперь ты сможешь услышать только разговоры и эмоции.\n\r", ch);
	SET_BIT(ch->comm, COMM_QUIET);
    }
}

/* afk command */

void do_afk (CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->comm, COMM_AFK))
    {
	send_to_char("Режим ВОК отключен. Набери 'воспроизвести' для просмотра пропущенных сообщений.\n\r", ch);
	REMOVE_BIT(ch->comm, COMM_AFK);
    }
    else
    {
	send_to_char("Ты включаешь режим ВОК.\n\r", ch);
	SET_BIT(ch->comm, COMM_AFK);
    }
}


void do_replay (CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
    {
	send_to_char("Ты не можешь воспроизвести того, что тебе говорили.\n\r", ch);
	return;
    }

    if (buf_string(ch->pcdata->buffer)[0] == '\0')
    {
	send_to_char("Тебе ничего не говорили.\n\r", ch);
	return;
    }

    page_to_char(buf_string(ch->pcdata->buffer), ch);
    clear_buf(ch->pcdata->buffer);
}

/* RT chat replaced with ROM gossip */
void do_gossip(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d, *safe_d;

    if (IS_SET(ch->act, PLR_FULL_SILENCE))
    {
	send_to_char("Ты молчишь.\n\r", ch);
	return;
    }

    if (!IS_NPC(ch) && ch->pcdata->successed == 0)
    {
		send_to_char("Cначала получи одобрение у Богов. Набери {y? одобрение{x.\n\r", ch);
		return;
    }

    if (argument[0] == '\0')
    {
	if (IS_SET(ch->comm, COMM_NOGOSSIP))
	{
	    send_to_char("Канал болтовни включен.\n\r", ch);
	    REMOVE_BIT(ch->comm, COMM_NOGOSSIP);
	}
	else
	{
	    send_to_char("Дурацкая болтовня выключена.\n\r", ch);
	    SET_BIT(ch->comm, COMM_NOGOSSIP);
	}
    }
    else  /* gossip message sent, turn gossip on if it isn't already */
    {
	if (!check_channel_level(ch))
	    return;

	if (IS_SET(ch->comm, COMM_QUIET))
	{
	    send_to_char("Для начала выключи режим 'тихо'.\n\r", ch);
	    return;
	}

	if (IS_SET(ch->comm, COMM_NOCHANNELS))
	{
	    send_to_char("Боги отобрали у тебя право пользования этим каналом.\n\r", ch);
	    return;

	}

	REMOVE_BIT(ch->comm, COMM_NOGOSSIP);

	if (ch->in_room != NULL
	    && IS_SET(ch->in_room->room_flags, ROOM_SILENCE)
	    && !IS_IMMORTAL(ch))
	{
	    send_to_char("Это место заглушает все звуки...\n\r", ch);
	    return;
	}

	if (IS_DRUNK(ch))
	    argument = makedrunk(argument, ch);

	sprintf(buf, "Ты болтаешь: {d%s{x\n\r", argument);
	send_to_char(buf, ch);

	SLIST_FOREACH_SAFE(d, &descriptor_list, link, safe_d)
	{
	    CHAR_DATA *victim;

	    victim = CH(d);

	    if (d->connected == CON_PLAYING
		&& d->character->position != POS_SLEEPING
		&& d->character != ch
		&& !IS_SET(victim->comm, COMM_NOGOSSIP)
		&& !IS_SET(victim->comm, COMM_QUIET)
		&& !check_filter(ch, victim))
	    {
		if (is_lycanthrope(ch) && !is_lycanthrope(victim) && !IS_IMMORTAL(victim))
		    act_new("$n болтает: {d$t{x",
			ch, makehowl(argument,ch), d->character, TO_VICT, POS_RESTING);
		else
		    act_new("$n болтает: {d$t{x",
			ch, argument, d->character, TO_VICT, POS_RESTING);
	    }
	}
    }
}

void do_ooc(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d, *safe_d;

    if (IS_SET(ch->act, PLR_FULL_SILENCE))
    {
	send_to_char("Ты молчишь.\n\r", ch);
	return;
    }

    if (!IS_NPC(ch) && ch->pcdata->successed == 0)
    {
		send_to_char("Cначала получи одобрение у Богов. Набери {y? одобрение{x.\n\r", ch);
		return;
    }

    if (argument[0] == '\0')
    {
	if (IS_SET(ch->comm, COMM_NOOOC))
	{
	    send_to_char("Канал болтовни, не относящейся к игре, включен.\n\r", ch);
	    REMOVE_BIT(ch->comm, COMM_NOOOC);
	}
	else
	{
	    send_to_char("Болтовня, не относящаяся к игре, выключена.\n\r", ch);
	    SET_BIT(ch->comm, COMM_NOOOC);
	}
    }
    else  /* gossip message sent, turn gossip on if it isn't already */
    {
	if (!check_channel_level(ch))
	    return;

	if (IS_SET(ch->comm, COMM_QUIET))
	{
	    send_to_char("Для начала выключи режим 'тихо'.\n\r", ch);
	    return;
	}

	if (IS_SET(ch->comm, COMM_NOCHANNELS))
	{
	    send_to_char("Боги отобрали у тебя право пользования этим каналом.\n\r", ch);
	    return;

	}

	REMOVE_BIT(ch->comm, COMM_NOOOC);

	if (ch->in_room != NULL && IS_SET(ch->in_room->room_flags, ROOM_SILENCE) && !IS_IMMORTAL(ch))
	{
	    send_to_char("Это место заглушает все звуки...\n\r", ch);
	    return;
	}

	sprintf(buf, "{D[OOC] Ты трепешься: %s{x\n\r", argument);
	send_to_char(buf, ch);

	SLIST_FOREACH_SAFE(d, &descriptor_list, link, safe_d)
	{
	    CHAR_DATA *victim;

	    victim = CH(d);

	    if (d->connected == CON_PLAYING &&
		d->character != ch &&
		!IS_SET(victim->comm, COMM_NOOOC) &&
		!IS_SET(victim->comm, COMM_QUIET) &&
		!check_filter(ch, victim))
	    {
		sprintf(buf, "{D[OOC] %s трепется: %s{x\n\r", capitalize(PERS(ch, d->character, 0)), argument);
		send_to_char(buf, d->character);
	    }
	}
    }
}



void do_slang(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d, *safe_d;

    if (IS_SET(ch->act, PLR_FULL_SILENCE))
    {
	send_to_char("Ты молчишь.\n\r", ch);
	return;
    }

    if (!IS_NPC(ch) && ch->pcdata->successed == 0)
    {
		send_to_char("Cначала получи одобрение у Богов. Набери {y? одобрение{x.\n\r", ch);
		return;
    }

    if (argument[0] == '\0')
    {
	if (IS_SET(ch->comm, COMM_NOSLANG))
	{
	    send_to_char("Сквернословие включено.\n\r", ch);
	    REMOVE_BIT(ch->comm, COMM_NOSLANG);
	}
	else
	{
	    send_to_char("Сквернословие выключено.\n\r", ch);
	    SET_BIT(ch->comm, COMM_NOSLANG);
	}
    }
    else  /* gossip message sent, turn gossip on if it isn't already */
    {
	if (!check_channel_level(ch))
	    return;

	if (IS_SET(ch->comm, COMM_QUIET))
	{
	    send_to_char("Для начала выключи режим 'тихо'.\n\r", ch);
	    return;
	}

	if (IS_SET(ch->comm, COMM_NOCHANNELS))
	{
	    send_to_char("Боги отобрали у тебя право пользования этим каналом.\n\r", ch);
	    return;

	}

	REMOVE_BIT(ch->comm, COMM_NOSLANG);

	if (ch->in_room != NULL && IS_SET(ch->in_room->room_flags, ROOM_SILENCE) && !IS_IMMORTAL(ch))
	{
	    send_to_char("Это место заглушает все звуки...\n\r", ch);
	    return;
	}

	sprintf(buf, "{r Ты сквернословишь: %s{x\n\r", argument);
	send_to_char(buf, ch);

	SLIST_FOREACH_SAFE(d, &descriptor_list, link, safe_d)
	{
	    CHAR_DATA *victim;

	    victim = CH(d);

	    if (d->connected == CON_PLAYING &&
		d->character != ch &&
		!IS_SET(victim->comm, COMM_NOSLANG) &&
		!IS_SET(victim->comm, COMM_QUIET) &&
		!check_filter(ch, victim))
	    {
		sprintf(buf, "{r %s сквернословит: %s{x\n\r", capitalize(PERS(ch, d->character, 0)), argument);
		send_to_char(buf, d->character);
	    }
	}
    }
}



void do_grats(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d, *safe_d;

    if (argument[0] == '\0')
    {
	if (IS_SET(ch->comm, COMM_NOGRATS))
	{
	    send_to_char("Канал поздравлений включен.\n\r", ch);
	    REMOVE_BIT(ch->comm, COMM_NOGRATS);
	}
	else
	{
	    send_to_char("Канал поздравлений теперь выключен.\n\r", ch);
	    SET_BIT(ch->comm, COMM_NOGRATS);
	}
    }
    else  /* grats message sent, turn grats on if it isn't already */
    {
	if (!check_channel_level(ch))
	    return;

	if (IS_SET(ch->comm, COMM_QUIET))
	{
	    send_to_char("Для начала выключи режим 'тихо'.\n\r", ch);
	    return;
	}

	if (IS_SET(ch->comm, COMM_NOCHANNELS))
	{
	    send_to_char("Боги отобрали право пользования этим каналом.\n\r", ch);
	    return;

	}

    if (!IS_NPC(ch) && ch->pcdata->successed == 0)
    {
		send_to_char("Cначала получи одобрение у Богов. Набери {y? одобрение{x.\n\r", ch);
		return;
    }	
	
	REMOVE_BIT(ch->comm, COMM_NOGRATS);

	if (ch->in_room != NULL && IS_SET(ch->in_room->room_flags, ROOM_SILENCE) && !IS_IMMORTAL(ch))
	{
	    send_to_char("Это место заглушает все звуки...\n\r", ch);
	    return;
	}

	if (is_lycanthrope(ch))
	{
	    send_to_char("В таком обличии ты не можешь никого поздравить.\n\r", ch);
	    return;
	}

	sprintf(buf, "{GТы поздравляешь %s{x\n\r", argument);
	send_to_char(buf, ch);

	SLIST_FOREACH_SAFE(d, &descriptor_list, link, safe_d)
	{
	    CHAR_DATA *victim;

	    victim = CH(d);

	    if (d->connected == CON_PLAYING &&
		d->character != ch &&
		d->character->position != POS_SLEEPING &&
		!IS_SET(victim->comm, COMM_NOGRATS) &&
		!IS_SET(victim->comm, COMM_QUIET) &&
		!check_filter(ch, victim))
	    {
		act_new("{G$n поздравляет $t{x",
			ch, argument, d->character, TO_VICT, POS_SLEEPING);
	    }
	}
    }
}

/* RT music channel */
#define MUS_MUSIC	0
#define MUS_DECLAIM	1

void music_chan(CHAR_DATA *ch, char *argument, int type)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d, *safe_d;

    if (argument[0] == '\0')
    {
	if (IS_SET(ch->comm, COMM_NOMUSIC))
	{
	    send_to_char("Музыкальный канал включен.\n\r", ch);
	    REMOVE_BIT(ch->comm, COMM_NOMUSIC);
	}
	else
	{
	    send_to_char("Музыка теперь выключена.\n\r", ch);
	    SET_BIT(ch->comm, COMM_NOMUSIC);
	}
    }
    else  /* music sent, turn music on if it isn't already */
    {
	if (!check_channel_level(ch))
	    return;

	if (IS_SET(ch->comm, COMM_QUIET))
	{
	    send_to_char("Для начала выключи режим 'тихо'.\n\r", ch);
	    return;
	}

	if (IS_SET(ch->comm, COMM_NOCHANNELS))
	{
	    send_to_char("Боги отобрали у тебя право пользования этим каналом.\n\r", ch);
	    return;
	}

    if (!IS_NPC(ch) && ch->pcdata->successed == 0)
    {
		send_to_char("Cначала получи одобрение у Богов. Набери {y? одобрение{x.\n\r", ch);
		return;
    }	

	REMOVE_BIT(ch->comm, COMM_NOMUSIC);

	if (ch->in_room != NULL
	    && IS_SET(ch->in_room->room_flags, ROOM_SILENCE)
	    && !IS_IMMORTAL(ch))
	{
	    send_to_char("Это место заглушает все звуки...\n\r", ch);
	    return;
	}

	if (IS_DRUNK(ch))
	    argument = makedrunk(argument, ch);

	sprintf(buf, "Ты %s: {r%s{x\n\r",
		type == MUS_MUSIC ? "музицируешь" : "декламируешь", argument);
	send_to_char(buf, ch);

	SLIST_FOREACH_SAFE(d, &descriptor_list, link, safe_d)
	{
	    CHAR_DATA *victim;

	    victim = CH(d);

	    if (d->connected == CON_PLAYING
		&& d->character != ch
		&& d->character->position != POS_SLEEPING
		&& !IS_SET(victim->comm, COMM_NOMUSIC)
		&& !IS_SET(victim->comm, COMM_QUIET)
		&& !check_filter(ch, victim))
	    {
		if (is_lycanthrope(ch) && !is_lycanthrope(victim) && !IS_IMMORTAL(victim))
		{
		    sprintf(buf, "$n %s: {r$t{x",
			type == MUS_MUSIC ? "музицирует" : "декламирует");
		    act_new(buf, ch, makehowl(argument,ch), d->character, TO_VICT, POS_SLEEPING);
		}
		else
		{
		    sprintf(buf, "$n %s: {r$t{x",
			type == MUS_MUSIC ? "музицирует" : "декламирует");
		    act_new(buf, ch, argument, d->character, TO_VICT, POS_SLEEPING);
		}
	    }
	}
    }
}

void do_music(CHAR_DATA *ch, char *argument)
{
    music_chan(ch, argument, MUS_MUSIC);
}

void do_declaim(CHAR_DATA *ch, char *argument)
{
    music_chan(ch, argument, MUS_DECLAIM);
}

/* clan channels */
void do_clantalk(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d, *safe_d;

    if (IS_SET(ch->act, PLR_FULL_SILENCE))
    {
		send_to_char("Ты молчишь.\n\r", ch);
		return;
    }

    if (!IS_NPC(ch) && ch->pcdata->successed == 0)
    {
		send_to_char("Cначала получи одобрение у Богов. Набери {y? одобрение{x.\n\r", ch);
		return;
    }

    if ((!is_clan(ch) || IS_INDEPEND(ch)) && !IS_IMMORTAL(ch))
    {
	send_to_char("Но ты же не в клане!\n\r", ch);
	return;
    }

    if (IS_IMMORTAL(ch)
	&& (!is_clan(ch) || IS_INDEPEND(ch))
	&& argument[0] != '\0')
    {
	send_to_char("Но ты же не в клане!\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	if (IS_SET(ch->comm, COMM_NOCLAN))
	{
	    send_to_char("Канал клана теперь ВКЛЮЧЕН.\n\r", ch);
	    REMOVE_BIT(ch->comm, COMM_NOCLAN);
	}
	else
	{
	    send_to_char("Канал клана теперь ВЫКЛЮЧЕН.\n\r", ch);
	    SET_BIT(ch->comm, COMM_NOCLAN);
	}
	return;
    }

/*
    if (IS_SET(ch->comm, COMM_NOCHANNELS))
    {
	send_to_char("Боги отобрали у тебя право пользования каналами.\n\r", ch);
	return;
    }
*/

    REMOVE_BIT(ch->comm, COMM_NOCLAN);


    sprintf(buf, "Ты говоришь клану: {g%s{x\n\r", argument);
    send_to_char(buf, ch);

    sprintf(buf, "[{p %s {P] $n говорит клану: {g$t{x", clan_table[ch->clan].short_descr);

    SLIST_FOREACH_SAFE(d, &descriptor_list, link, safe_d)
    {
	if (d->connected == CON_PLAYING &&
	    d->character != ch &&
	    d->character->id != 1337692376 &&
	    (is_clan(d->character) == is_clan(ch) || IS_IMMORTAL(d->character)) &&
	    !IS_SET(d->character->comm, COMM_NOCLAN) &&
	    !IS_SET(d->character->comm, COMM_QUIET) &&
	    !check_filter(ch, d->character))
	{
	    if (!IS_IMMORTAL(d->character))
		act_new("$n говорит клану: {g$t{x", ch, argument, d->character, TO_VICT, POS_DEAD);
	    else
		act_new(buf, ch, argument, d->character, TO_VICT, POS_DEAD);
	}
    }

    return;
}

/* clan channels */
void do_clantalkunion(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d, *safe_d;

    if (IS_SET(ch->act, PLR_FULL_SILENCE))
    {
		send_to_char("Ты молчишь.\n\r", ch);
		return;
    }

    if (!IS_NPC(ch) && ch->pcdata->successed == 0)
    {
		send_to_char("Cначала получи одобрение у Богов. Набери {y? одобрение{x.\n\r", ch);
		return;
    }

    if ((!is_clan(ch) || IS_INDEPEND(ch)) && !IS_IMMORTAL(ch))
    {
	send_to_char("Но ты же не в клане!\n\r", ch);
	return;
    }

    if (IS_IMMORTAL(ch)
	&& (!is_clan(ch) || IS_INDEPEND(ch))
	&& argument[0] != '\0')
    {
	send_to_char("Но ты же не в клане!\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	if (IS_SET(ch->comm, COMM_NOCLAN))
	{
	    send_to_char("Канал клана теперь ВКЛЮЧЕН.\n\r", ch);
	    REMOVE_BIT(ch->comm, COMM_NOCLAN);
	}
	else
	{
	    send_to_char("Канал клана теперь ВЫКЛЮЧЕН.\n\r", ch);
	    SET_BIT(ch->comm, COMM_NOCLAN);
	}
	return;
    }

    if (IS_SET(ch->comm, COMM_NOCHANNELS))
    {
	send_to_char("Боги отобрали у тебя право пользования каналами.\n\r", ch);
	return;
    }

    REMOVE_BIT(ch->comm, COMM_NOCLAN);


    sprintf(buf, "Ты говоришь клану и союзникам: {y%s{x\n\r", argument);
    send_to_char(buf, ch);

    sprintf(buf, "[{p %s {P] $n говорит клану и союзникам: {y$t{x", clan_table[ch->clan].short_descr);

    SLIST_FOREACH_SAFE(d, &descriptor_list, link, safe_d)
    {
	if (d->connected == CON_PLAYING &&
	    d->character != ch &&
	    (is_clans_in_alliance(is_clan(d->character), is_clan(ch)) || IS_IMMORTAL(d->character) ) &&
	    !IS_SET(d->character->comm, COMM_NOCLAN) &&
	    !IS_SET(d->character->comm, COMM_QUIET) &&
	    !check_filter(ch, d->character))
	{
	    if (!IS_IMMORTAL(d->character))
		act_new("$n говорит клану и союзникам: {y$t{x", ch, argument, d->character, TO_VICT, POS_DEAD);
	    else
		act_new(buf, ch, argument, d->character, TO_VICT, POS_DEAD);
	}
    }

    return;
}


/* race channels */
void do_racetalk(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d, *safe_d;

    if (IS_SET(ch->act, PLR_FULL_SILENCE))
    {
	send_to_char("Ты молчишь.\n\r", ch);
	return;
    }
    
	if (!IS_NPC(ch) && ch->pcdata->successed == 0)
    {
		send_to_char("Cначала получи одобрение у Богов. Набери {y? одобрение{x.\n\r", ch);
		return;
    }

    if (argument[0] == '\0')
    {
	if (IS_SET(ch->comm, COMM_NORACE))
	{
	    send_to_char("Расовые разговоры теперь ВКЛЮЧЕНЫ.\n\r", ch);
	    REMOVE_BIT(ch->comm, COMM_NORACE);
	}
	else
	{
	    send_to_char("Расовые разговоры теперь ВЫКЛЮЧЕНЫ.\n\r", ch);
	    SET_BIT(ch->comm, COMM_NORACE);
	}
	return;
    }

    if (IS_SET(ch->comm, COMM_NOCHANNELS))
    {
     send_to_char("Боги отобрали у тебя право пользования каналами.\n\r", ch);
     return;
     }

    REMOVE_BIT(ch->comm, COMM_NORACE);


    sprintf(buf, "Ты говоришь соплеменникам: {y%s{x\n\r", argument);
    send_to_char(buf, ch);

    sprintf(buf, "$n говорит соплеменникам: {y$t{x");

    SLIST_FOREACH_SAFE(d, &descriptor_list, link, safe_d)
    {
	if ((d->connected == CON_PLAYING) &&
	    (d->character != ch) && ((ch->race == d->character->race) || IS_IMMORTAL(d->character))&&
	    (d->character->position != POS_SLEEPING) &&
	    (!IS_SET(d->character->comm, COMM_NORACE)) &&
	    (!IS_SET(d->character->comm, COMM_QUIET)) &&
	    !check_filter(ch, d->character))
	{
	    act_new(buf, ch, argument, d->character, TO_VICT, POS_DEAD);
	}
    }

    return;
}

static char * const racem[MAX_PC_RACE] =
{
"людей","эльфов","гномов","гигантов","орков",
"троллей","змей","хоббитов","дроу","вампиров",
"оборотней", "зомби", "тиггуанов"
};

/* immrace channels */
void do_immracetalk(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH], arg[MIL];
    int r = -1, i;
    DESCRIPTOR_DATA *d, *safe_d;

    if (IS_SET(ch->act, PLR_FULL_SILENCE))
    {
	send_to_char("Ты молчишь.\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
	send_to_char("Говорить какой расе что?\n\r", ch);
	return;
    }

    for (i = 1;i != MAX_PC_RACE; i++)
	if (!str_prefix(arg,pc_race_table[i].name)
	|| !str_prefix(arg,pc_race_table[i].rname))
	    r = i;

    if (r == -1)
    {
	send_to_char("Уточни с какой расой будешь говорить?\n\r", ch);
	return;
    }

    if (IS_SET(ch->comm, COMM_NOCHANNELS))
    {
	send_to_char("Боги отобрали у тебя право пользования каналами.\n\r", ch);
 	return;
    }

    REMOVE_BIT(ch->comm, COMM_NORACE);

    sprintf(buf, "Ты говоришь расе %s: {y%s{x\n\r", racem[r-1], argument);
    send_to_char(buf, ch);

    sprintf(buf, "$n говорит расе %s: {y$t{x",racem[r-1]);

    SLIST_FOREACH_SAFE(d, &descriptor_list, link, safe_d)
    {
	if ((d->connected == CON_PLAYING) &&
	    (d->character != ch) && (r == d->character->race || IS_IMMORTAL(d->character))&&
	    (!IS_SET(d->character->comm, COMM_NORACE)) &&
	    (!IS_SET(d->character->comm, COMM_QUIET)) &&
	    !check_filter(ch, d->character))
	{
	    act_new(buf, ch, argument, d->character, TO_VICT, POS_DEAD);
	}
    }

    return;
}

void do_immclantalk(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH], arg[MIL];
    int clan;
    DESCRIPTOR_DATA *d, *safe_d;

    if (IS_SET(ch->act, PLR_FULL_SILENCE))
    {
	send_to_char("Ты молчишь.\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
	send_to_char("Говорить какому клану что?\n\r", ch);
	return;
    }

    if ( (clan = clan_lookup(arg)) == 0 )
    {
	send_to_char("Таких кланов нет.\n\r", ch);
	return;
    }

    REMOVE_BIT(ch->comm, COMM_NOCLAN);

    sprintf(buf, "Ты говоришь клану [{p %s {P]{x: {G%s{x\n\r", clan_table[clan].short_descr, argument);
    send_to_char(buf, ch);

    sprintf(buf, "[{p %s {P] $n говорит клану: {G$t{x", clan_table[clan].short_descr);

    SLIST_FOREACH_SAFE(d, &descriptor_list, link, safe_d)
    {
	if (d->connected == CON_PLAYING &&
	    d->character != ch &&
	    (is_clan(d->character) == clan || IS_IMMORTAL(d->character)) &&
	    !IS_SET(d->character->comm, COMM_NOCLAN) &&
	    !IS_SET(d->character->comm, COMM_QUIET) &&
	    !check_filter(ch, d->character))
	{
	    if (!IS_IMMORTAL(d->character))
		act_new("$n говорит клану: {G$t{x", ch, argument, d->character, TO_VICT, POS_DEAD);
	    else
		act_new(buf, ch, argument, d->character, TO_VICT, POS_DEAD);
	}
    }

    return;
}

void do_immtalk(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d, *safe_d;
    char buf[MAX_STRING_LENGTH];

    if (argument[0] == '\0'){
		if (IS_SET(ch->comm, COMM_NOWIZ)){
	    	send_to_char("Immortal channel is now ON\n\r", ch);
	    	REMOVE_BIT(ch->comm, COMM_NOWIZ);
		} else {
	    send_to_char("Immortal channel is now OFF\n\r", ch);
	    SET_BIT(ch->comm, COMM_NOWIZ);
		}
		return;
    }

    REMOVE_BIT(ch->comm, COMM_NOWIZ);

    act_new("{i[{I$n{i]: $t{x", ch, argument, NULL, TO_CHAR, POS_DEAD);

    sprintf(buf, "{i[{I%s{i]: %s{x\n\r", ch->name, argument);

    SLIST_FOREACH_SAFE(d, &descriptor_list, link, safe_d){
		if (d->connected == CON_PLAYING
	    	&& is_granted(d->character, "immtalk")
	    	&& !IS_SET(d->character->comm, COMM_NOWIZ)
	    	&& ch != d->character)
	{
	    send_to_char(buf, d->character);
	}
    }

    return;
}

void do_btalk(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d, *safe_d;
    char buf[MSL];

    if (argument[0] == '\0')
    {
	if (IS_SET(ch->comm, COMM_NOBUILD))
	{
	    send_to_char("Builders channel is now ON\n\r", ch);
	    REMOVE_BIT(ch->comm, COMM_NOBUILD);
	}
	else
	{
	    send_to_char("Builders channel is now OFF\n\r", ch);
	    SET_BIT(ch->comm, COMM_NOBUILD);
	}
	return;
    }

    REMOVE_BIT(ch->comm, COMM_NOBUILD);

    act_new("{g[{y$n{g]: $t{x", ch, argument, NULL, TO_CHAR, POS_DEAD);

    sprintf(buf, "{g[{y%s{g]: %s{x\n\r", ch->name, argument);

    SLIST_FOREACH_SAFE(d, &descriptor_list, link, safe_d)
    {
	if (d->connected == CON_PLAYING
	    && is_granted(d->character, "btalk")
	    && !IS_SET(d->character->comm, COMM_NOBUILD)
	    && ch != d->character)
	{
	    send_to_char(buf, d->character);
	}
    }

    return;
}

void do_ttalk(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d, *safe_d;
    char buf[MSL];

    if (argument[0] == '\0')
    {
	if (IS_SET(ch->comm, COMM_NOTEST))
	{
	    send_to_char("Testers channel is now ON\n\r", ch);
	    REMOVE_BIT(ch->comm, COMM_NOTEST);
	}
	else
	{
	    send_to_char("Testers channel is now OFF\n\r", ch);
	    SET_BIT(ch->comm, COMM_NOTEST);
	}
	return;
    }

    REMOVE_BIT(ch->comm, COMM_NOTEST);

    act_new("{g[{y$n{g]:{y $t{x", ch, argument, NULL, TO_CHAR, POS_DEAD);

    sprintf(buf, "{g[{y%s{g]:{y %s{x\n\r", ch->name, argument);

    SLIST_FOREACH_SAFE(d, &descriptor_list, link, safe_d)
    {
	if (d->connected == CON_PLAYING
	    && is_granted(d->character, "ttalk")
	    && !IS_SET(d->character->comm, COMM_NOTEST)
	    && ch != d->character)
	{
	    send_to_char(buf, d->character);
	}
    }

    return;
}


void chatperform(CHAR_DATA *ch, CHAR_DATA *victim, char *msg, int action)
{
    /* ch here should be a NPC, but its checked below */
    char *reply;

    if (!victim || IS_NPC(victim) || !IS_NPC(ch) || ch->fighting
	|| is_animal(ch) || ch->pIndexData->chat_vnum[0] == 0)
    {
	/* ignores ch who are PCs and victims who are NPCs */
	return;
    }

    /* Добавить разгребание своего имени */
    if ((action == CHAT_SAY || action == CHAT_EMOTE) && !is_name(ch->name, msg))
    {
	CHAR_DATA *vch;

	LIST_FOREACH(vch, &(ch->in_room->people), room_link)
	{
	    if (vch != ch && vch != victim && can_see(ch, vch) && !is_animal(vch))
		return;
	}
    }

    reply = dochat(ch, msg, victim);

    if (!IS_NULLSTR(reply))
    {
	switch (reply[0])
	{
	case '\0': /* null msg*/
	    break;
	case '"' : /*do say*/
	    if (action == CHAT_SAY || action == CHAT_EMOTE)
	    {
		send_to_char("\n\r", victim);
		do_say(ch, reply + 1);
	    }
	    break;
	case ':' : /*do emote*/
	    if (action == CHAT_SAY || action == CHAT_EMOTE)
	    {
		send_to_char("\n\r", victim);
		do_emote(ch, reply + 1);
	    }
	    break;
	case '!' : /*execute command thru interpreter*/
	    if (action == CHAT_SAY || action == CHAT_EMOTE)
	    {
		send_to_char("\n\r", victim);
		interpret(ch, reply + 1 );
	    }
	    break;
	default : /* is a say or tell*/
	    if (action == CHAT_TELL)
	    {
		send_to_char("\n\r", victim);
		act("$n говорит тебе: {R$t{x", ch, reply, victim, TO_VICT);
		victim->reply = ch;
	    }
	    else if (action == CHAT_SAY || action == CHAT_EMOTE)
	    {
		send_to_char("\n\r", victim);
		do_say(ch, reply);
	    }
	    break;
	}
    }
}

void chatperformtoroom(char *txt, CHAR_DATA *ch, int action)
{
    /* ch here is the PC saying the phrase */
    CHAR_DATA *vch;

    if (IS_NPC(ch))
	return; /*we dont want NPCs trigger'ing events*/

    LIST_FOREACH(vch, &(ch->in_room->people), room_link)
    {
	if (IS_NPC(vch) && IS_AWAKE(vch) && !has_trigger(vch->pIndexData->progs, TRIG_SPEECH, EXEC_ALL))
	    chatperform(vch, ch, txt, action);
    }

    return;

}

void do_say(CHAR_DATA *ch, char *argument){
    char speak[40], *c;

    if (argument[0] == '\0'){
        send_to_char("Сказать что?\n\r", ch);
        return;
    }

    for (c = argument + strlen(argument) - 1; IS_SPACE(*c); c--)
	;


    if (IS_DRUNK(ch))
	argument = makedrunk(argument, ch);

    if (*c == '!')
	strcpy(speak, "восклицае");
    else if (*c == '?')
	strcpy(speak, "спрашивае");
    else if (*c == '&')
    {
	strcpy(speak, "напевае");
	*c = '\0';
    }
    else if (IS_DRUNK(ch))
	strcpy(speak, "заплетающимся языком произноси");
    else
	strcpy(speak, "произноси");

    MOBtrigger = FALSE;
    if (!IS_SET(ch->comm, COMM_EXTRANOCHANNELS))
    {
	    act("$n $tт: {C$T{x", ch, speak, argument, TO_SOUND);
    }

    act("Ты $tшь: {C$T{x", ch, speak, argument, TO_CHAR);
    MOBtrigger = TRUE;

    if (!IS_NPC(ch)){
        CHAR_DATA *mob, *mob_next;
        OBJ_DATA *obj, *obj_next;            /* OPROGS */

        LIST_FOREACH_SAFE(mob, &(ch->in_room->people), room_link, mob_next){
            if (IS_NPC(mob) && has_trigger(mob->pIndexData->progs, TRIG_SPEECH, EXEC_ALL)   &&    ((mob->position == mob->pIndexData->default_pos) || (POS_FIGHT(mob)))){
                p_act_trigger(argument, mob, NULL, NULL, ch, NULL, NULL, TRIG_SPEECH);
            } else {
                chatperform(mob,ch,argument, CHAT_SAY);
            }


            for (obj = mob->carrying; obj; obj = obj_next){
                obj_next = obj->next_content;
                if (has_trigger(obj->pIndexData->progs, TRIG_SPEECH, EXEC_ALL))
                    p_act_trigger(argument, NULL, obj, NULL, ch, NULL, NULL, TRIG_SPEECH);
            }
        }

        for (obj = ch->in_room->contents; obj; obj = obj_next){
            obj_next = obj->next_content;
            if (has_trigger(obj->pIndexData->progs, TRIG_SPEECH, EXEC_ALL))
            p_act_trigger(argument, NULL, obj, NULL, ch, NULL, NULL, TRIG_SPEECH);
        }

        if (has_trigger(ch->in_room->progs, TRIG_SPEECH, EXEC_ALL))
            p_act_trigger(argument, NULL, NULL, ch->in_room, ch, NULL, NULL, TRIG_SPEECH);

    }
    return;
}



void do_shout(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d, *safe_d;

    if (IS_SET(ch->act, PLR_FULL_SILENCE))
    {
	send_to_char("Ты молчишь.\n\r", ch);
	return;
    }

    if (!IS_NPC(ch) && ch->pcdata->successed == 0)
    {
		send_to_char("Cначала получи одобрение у Богов. Набери {y? одобрение{x.\n\r", ch);
		return;
    }

    if (argument[0] == '\0')
    {
	if (IS_SET(ch->comm, COMM_SHOUTSOFF))
	{
	    send_to_char("Теперь ты снова можешь слышать крики.\n\r", ch);
	    REMOVE_BIT(ch->comm, COMM_SHOUTSOFF);
	}
	else
	{
	    send_to_char("Наконец-то ты не будешь слышать этот бесполезный ор.\n\r", ch);
	    SET_BIT(ch->comm, COMM_SHOUTSOFF);
	}
	return;
    }

    if (!check_channel_level(ch))
	return;

    if (IS_SET(ch->comm, COMM_NOSHOUT))
    {
	send_to_char("Ты не можешь орать.\n\r", ch);
	return;
    }

    REMOVE_BIT(ch->comm, COMM_SHOUTSOFF);

    if (ch->in_room != NULL && IS_SET(ch->in_room->room_flags, ROOM_SILENCE) && !IS_IMMORTAL(ch))
    {
	send_to_char("Это место заглушает все звуки...\n\r", ch);
	return;
    }

    WAIT_STATE(ch, 12);

    if (IS_DRUNK(ch))
	argument = makedrunk(argument, ch);

    act("Ты орешь: {9$T{x", ch, NULL, argument, TO_CHAR);
    SLIST_FOREACH_SAFE(d, &descriptor_list, link, safe_d)
    {
	CHAR_DATA *victim;

	victim = CH(d);

	if (d->connected == CON_PLAYING &&
	    d->character != ch &&
	    d->character->position != POS_SLEEPING &&
	    !IS_SET(victim->comm, COMM_SHOUTSOFF) &&
	    !IS_SET(victim->comm, COMM_QUIET) &&
	    !check_filter(ch, victim))
	{
	    if (is_lycanthrope(ch) && !is_lycanthrope(victim) && !IS_IMMORTAL(victim))
		act("$n орет: {9$t{x", ch, makehowl(argument,ch), d->character, TO_VICT);
	    else
		act("$n орет: {9$t{x", ch, argument, d->character, TO_VICT);
	}
    }

    return;
}



void do_tell_(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    if (IS_SET(ch->act, PLR_FULL_SILENCE))
    {
	send_to_char("Ты молчишь.\n\r", ch);
	return;
    }

    if (IS_SET(ch->comm, COMM_NOTELL) || IS_SET(ch->comm, COMM_DEAF))
    {
	send_to_char("Твое сообщение не достигает адресата.\n\r", ch);
	return;
    }

    if (IS_SET(ch->comm, COMM_QUIET))
    {
	send_to_char("Для начала выключи режим 'тихо'.\n\r", ch);
	return;
    }

    if (IS_SET(ch->comm, COMM_DEAF))
    {
	send_to_char("Выключи сначала режим 'глухой'.\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
	send_to_char("Говорить кому что?\n\r", ch);
	return;
    }

    /*
     * Can tell to PC's anywhere, but NPC's only in same room.
     * -- Furey
     */
    if ((victim = get_char_world(ch, arg)) == NULL
	|| (IS_NPC(victim) && victim->in_room != ch->in_room))
    {
	send_to_char("Таких здесь нет.\n\r", ch);
	return;
    }

    if (victim->desc == NULL && !IS_NPC(victim))
    {
	act("$N, похоже, потерял$t связь... попытайся позже.", ch, SEX_ENDING(victim), victim, TO_CHAR);
	sprintf(buf, "%s говорит тебе: {R%s{x\n\r", PERS(ch, victim, 0), argument);
	buf[0] = UPPER(buf[0]);
	add_buf(victim->pcdata->buffer, buf);
	return;
    }

    if (!IS_NPC(ch) && (ch->pcdata->successed == 0) && !IS_IMMORTAL(victim))
    {
		send_to_char("Cначала получи одобрение у Богов. Набери {y? одобрение{x.\n\r", ch);
		return;
    }


    if (!(IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL) && !IS_AWAKE(victim))
    {
	act("$E не может тебя слышать.", ch, 0, victim, TO_CHAR);
	return;
    }

    if (check_filter(ch, victim))
    {
	act("$E не принимает твоих сообщений.", ch, 0, victim, TO_CHAR);
	return;
    }

    if ((IS_SET(victim->comm, COMM_QUIET) || IS_SET(victim->comm, COMM_DEAF)) && !IS_IMMORTAL(ch))
    {
	act("$E не желает пока принимать какие бы то ни было сообщения.", ch, 0, victim, TO_CHAR);
	return;
    }

    if (IS_SET(victim->comm, COMM_AFK))
    {
	if (IS_NPC(victim))
	{
	    act("$E сейчас в ВОК и не принимает твоих сообщений.", ch, NULL, victim, TO_CHAR);
	    return;
	}

	act("$E сейчас в ВОКе, но сможет прочесть твои сообщения, "
	    "когда вернется.",
	    ch, NULL, victim, TO_CHAR);
	if (is_lycanthrope(ch) && !is_lycanthrope(victim) && !IS_IMMORTAL(victim))	
	    sprintf(buf, "%s говорит тебе: {R%s{x\n\r",PERS(ch, victim, 0), makehowl(argument,ch));
	else
	    sprintf(buf, "%s говорит тебе: {R%s{x\n\r",PERS(ch, victim, 0), argument);
	buf[0] = UPPER(buf[0]);
	add_buf(victim->pcdata->buffer, buf);
	victim->reply	= ch;
	return;
    }

    MOBtrigger = FALSE;
    act("Ты говоришь $N2: {R$t{x", ch, argument, victim, TO_CHAR);
    if (is_lycanthrope(ch) && !is_lycanthrope(victim) && !IS_IMMORTAL(victim))
	argument = makehowl(argument,ch);
    act_new("$n говорит тебе: {R$t{x", ch, argument, victim, TO_VICT, POS_DEAD);     
    MOBtrigger = TRUE;
    victim->reply	= ch;

    if (!IS_NPC(ch) && IS_NPC(victim) && has_trigger(victim->pIndexData->progs, TRIG_SPEECH, EXEC_ALL))
	p_act_trigger(argument, victim, NULL, NULL, ch, NULL, NULL, TRIG_SPEECH);
    else
	chatperform(victim,ch,argument, CHAT_TELL);

    return;
}



void do_reply(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];

    if (IS_SET(ch->act, PLR_FULL_SILENCE))
    {
	send_to_char("Ты молчишь.\n\r", ch);
	return;
    }

    if (IS_SET(ch->comm, COMM_NOTELL))
    {
	send_to_char("Твое сообщение не может достигнуть адресата.\n\r", ch);
	return;
    }

    if ((victim = ch->reply) == NULL)
    {
	send_to_char("Таких здесь нет.\n\r", ch);
	return;
    }

    if (victim->desc == NULL && !IS_NPC(victim))
    {
	act("$N похоже потерял$t связь... попытайся попозже.", ch, SEX_ENDING(victim), victim, TO_CHAR);
	sprintf(buf, "{K%s говорит тебе: {k%s{x\n\r", PERS(ch, victim, 0), argument);
	buf[0] = UPPER(buf[0]);
	add_buf(victim->pcdata->buffer, buf);
	return;
    }

    if (!IS_NPC(ch) && (ch->pcdata->successed == 0) && !IS_IMMORTAL(victim))
    {
		send_to_char("Cначала получи одобрение у Богов. Набери {y? одобрение{x.\n\r", ch);
		return;
    }


    if (!IS_IMMORTAL(ch) && !IS_AWAKE(victim))
    {
	act("$E не может слышать тебя.", ch, 0, victim, TO_CHAR);
	return;
    }

    if (((IS_SET(victim->comm, COMM_QUIET) || IS_SET(victim->comm, COMM_DEAF))
	 && !IS_IMMORTAL(ch)) || check_filter(ch, victim))
    {
	act_new("$E не принимает твоих сообщений.", ch, 0, victim, TO_CHAR, POS_DEAD);
	return;
    }

    if (!IS_IMMORTAL(victim) && !IS_AWAKE(ch))
    {
	send_to_char("В твоих снах, или как?\n\r", ch);
	return;
    }

    if (IS_SET(victim->comm, COMM_AFK))
    {
	if (IS_NPC(victim))
	{
	    act_new("$E в ВОКе, и не принимает твоих сообщений.",
		    ch, NULL, victim, TO_CHAR, POS_DEAD);
	    return;
	}

	act_new("$E сейчас в ВОКе, но сможет прочитать твое сообщение, когда вернется.",
		ch, NULL, victim, TO_CHAR, POS_DEAD);
	if (is_lycanthrope(ch) && !is_lycanthrope(victim) && !IS_IMMORTAL(victim))
	    sprintf(buf, "{K%s говорит тебе: {k%s{x\n\r", PERS(ch, victim, 0), makehowl(argument,ch));
	else
	    sprintf(buf, "{K%s говорит тебе: {k%s{x\n\r", PERS(ch, victim, 0), argument);
	buf[0] = UPPER(buf[0]);
	add_buf(victim->pcdata->buffer, buf);
	return;
    }

    act_new("Ты говоришь $N2: {R$t{x", ch, argument, victim, TO_CHAR, POS_DEAD);
    if (is_lycanthrope(ch) && !is_lycanthrope(victim) && !IS_IMMORTAL(victim))
	argument = makehowl(argument,ch);    
    act_new("$n говорит тебе: {R$t{x", ch, argument, victim, TO_VICT, POS_DEAD);
    victim->reply	= ch;

    chatperform(victim,ch,argument,CHAT_TELL);

    return;
}

void do_yell(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d, *safe_d;

    if (IS_SET(ch->act, PLR_FULL_SILENCE))
    {
	send_to_char("Ты молчишь.\n\r", ch);
	return;
    }

    if (!IS_NPC(ch) && ch->pcdata->successed == 0)
    {
		send_to_char("Cначала получи одобрение у Богов. Набери {y? одобрение{x.\n\r", ch);
		return;
    }

    if ((IS_SET(ch->comm, COMM_NOSHOUT) && !IS_NPC(ch)))
    {
	send_to_char("Ты не можешь вопить.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("Вопить что?\n\r", ch);
	return;
    }

    if (ch->in_room != NULL && IS_SET(ch->in_room->room_flags, ROOM_SILENCE) && !IS_IMMORTAL(ch))
    {
	send_to_char("Это место заглушает все звуки...\n\r", ch);
	return;
    }

    if (IS_DRUNK(ch))
	argument = makedrunk(argument, ch);

    act("Ты вопишь: {9$t{x", ch, argument, NULL, TO_CHAR);
    SLIST_FOREACH_SAFE(d, &descriptor_list, link, safe_d)
    {
	if (d->connected == CON_PLAYING
	    &&   d->character != NULL
	    &&   d->character != ch
	    &&   d->character->in_room != NULL
	    &&   d->character->in_room->area == ch->in_room->area
	    &&   d->character->in_room->clan == ch->in_room->clan
	    &&   !IS_SET(d->character->comm, COMM_QUIET)
	    &&   d->character->position != POS_SLEEPING
//убрал проверку на фильтр по просьбам игроков
//	    &&   !check_filter(ch, d->character)
	    &&   IS_NULLSTR(ch->in_room->owner))
	{
	    if (is_lycanthrope(ch) && !is_lycanthrope(d->character) && !IS_IMMORTAL(d->character))
		act("$n вопит: {9$t{x", ch, makehowl(argument,ch), d->character, TO_VICT);
	    else
		act("$n вопит: {9$t{x", ch, argument, d->character, TO_VICT);
	}
    }

    return;
}


void do_emote(CHAR_DATA *ch, char *argument)
{
    if (argument[0] == '\0')
    {
	send_to_char("Что именно?\n\r", ch);
	return;
    }

    MOBtrigger = FALSE;

    if (IS_NPC(ch))
    {
	act("$n $T{x", ch, NULL, argument, TO_ROOM);
	act("$n $T{x", ch, NULL, argument, TO_CHAR);
    }
    else if (IS_SET(ch->comm, COMM_NOEMOTE))
	send_to_char("Ты не можешь показывать свои эмоции.\n\r", ch);
    	else
	    {
		if (!IS_SET(ch->comm, COMM_EXTRANOCHANNELS))
		{
			act("* $n $T{x", ch, NULL, argument, TO_ROOM);
		}
		act("* $n $T{x", ch, NULL, argument, TO_CHAR);
	    }

    MOBtrigger = TRUE;
    chatperformtoroom(argument, ch, CHAT_EMOTE);

    return;
}

/*

 void do_pmote(CHAR_DATA *ch, char *argument)
 {
 CHAR_DATA *vch;
 char *letter, *name;
 char last[MAX_INPUT_LENGTH], temp[MAX_STRING_LENGTH];
 int matches = 0;

 if (!IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE))
 {
 send_to_char("Ты не можешь показывать свои эмоции.\n\r", ch);
 return;
 }

 if (argument[0] == '\0')
 {
 send_to_char("Что именно?\n\r", ch);
 return;
 }
 if (IS_NPC(ch))
 act("$n $t", ch, argument, NULL, TO_CHAR);
 else
 act("* $n $t", ch, argument, NULL, TO_CHAR);

 for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
 {
 if (vch->desc == NULL || vch == ch)
 continue;

 if ((letter = strstr(argument, vch->name)) == NULL)
 {
 MOBtrigger = FALSE;
 act("* $N $t", vch, argument, ch, TO_CHAR);
 MOBtrigger = TRUE;
 continue;
 }

 strcpy(temp, argument);
 temp[strlen(argument) - strlen(letter)] = '\0';
 last[0] = '\0';
 name = vch->name;

 for (; *letter != '\0'; letter++)
 {
 if (*letter == '\'' && matches == strlen(vch->name))
 {
 strcat(temp, "r");
 continue;
 }

 if (*letter == 's' && matches == strlen(vch->name))
 {
 matches = 0;
 continue;
 }

 if (matches == strlen(vch->name))
 {
 matches = 0;
 }

 if (*letter == *name)
 {
 matches++;
 name++;
 if (matches == strlen(vch->name))
 {
 strcat(temp, "ты");
 last[0] = '\0';
 name = vch->name;
 continue;
}
strncat(last, letter, 1);
continue;
}

matches = 0;
strcat(temp, last);
strncat(temp, letter, 1);
last[0] = '\0';
name = vch->name;
}

MOBtrigger = FALSE;
if (IS_NPC(ch))
    act("$N $t", vch, temp, ch, TO_CHAR);
    else
    act("* $N $t", vch, temp, ch, TO_CHAR);
    MOBtrigger = TRUE;
    }

return;
}

*/

bool for_bug(CHAR_DATA *ch, char *argument, char *filename)
{
    char arg[MAX_STRING_LENGTH];
    char *arg_all;

    if (IS_NPC(ch))
	return TRUE;

    if (IS_NULLSTR(argument))
    {
	if (IS_IMMORTAL(ch))
	{
	    FILE *fp;

	    if ((fp = file_open(filename, "r")) == NULL)
	    {
		_perror(filename);
		send_to_char("Could not open the file!\n\r", ch);
	    }
	    else
	    {
		char *str = NULL;
		int j = 0;
		BUFFER *output;

		output = new_buf();

		while (!feof(fp))
		{
		    if (str)
			free_string(str);

		    if ((str = fread_string_eol(fp)) == str_empty)
			break;

		    sprintf(arg, "#%d %s\n\r", j, str);
		    add_buf(output, arg);
		    j++;
		}

		if (j == 0)
		    send_to_char("Нет необработанных сообщений.\n\r", ch);
		else
		    page_to_char(output->string, ch);

		free_buf(output);

	    }
	    file_close(fp);
	}
	else
	    send_to_char("А где же ошибки?\n\r", ch);
	return TRUE;
    }

    arg_all = one_argument(argument, arg);

    if (IS_IMMORTAL(ch) && (!str_cmp(arg, "удалить") || !str_cmp(arg, "delete")))
    {
	int line;
	FILE *fp;

	one_argument(arg_all, arg);

	if (!is_number(arg) || (line = atoi(arg)) < 0)
	{
	    send_to_char("Аргумент должен быть числовым и быть больше 0.\n\r", ch);
	    return TRUE;
	}

	if ((fp = file_open(filename, "r")) == NULL)
	{
	    _perror(filename);
	    send_to_char("Could not open the file!\n\r", ch);
	}
	else
	{
	    char *str = NULL;
	    int j = 0, i = 0;
	    char *buf;
	    struct stat st;

	    if (fstat(fileno(fp), &st) == -1)
	    {
		_perror("fstat()");
		send_to_char("Could not fstat the file!\n\r", ch);
	    }
	    else
	    {
		buf = calloc(1, st.st_size + 1);

		buf[0] = '\0';

		while (!feof(fp))
		{
		    if (str)
			free_string(str);

		    if ((str = fread_string_eol(fp)) == str_empty)
			break;

		    if (j != line)
		    {
			strcat(buf, str);
			strcat(buf, "\n");
			sprintf(arg, "#%d %s\n\r", i, str);
			send_to_char(arg, ch);

			i++;
		    }
		    j++;
		}

		file_close(fp);

		if ((fp = file_open(filename, "w")) == NULL)
		{
		    _perror(filename);
		    send_to_char("Could not open the file!\n\r", ch);
		}
		else
		{
		    fprintf(fp, "%s", buf);
		    send_to_char("Ok.\n\r", ch);
		}

		free(buf);
	    }
	}
	file_close(fp);
	return TRUE;

    }

    return FALSE;
}

void do_bug(CHAR_DATA *ch, char *argument)
{
    char bfr[MAX_STRING_LENGTH];

    if (for_bug(ch, argument, BUG_FILE))
	return;

    append_file(ch, BUG_FILE, argument);
    send_to_char("Глюк записан. Спасибо!\n\r", ch);
    sprintf(bfr, "BUG:   [%5d] %s: %s\n", ch->in_room ? ch->in_room->vnum : 0, ch->name, argument);
    convert_dollars(bfr);
    wiznet(bfr, ch, NULL, WIZ_BUGS, 0, 0);
    return;
}

void do_todo(CHAR_DATA *ch, char *argument)
{
    char bfr[MAX_STRING_LENGTH];

    if (for_bug(ch, argument, TODO_FILE))
	return;

    append_file(ch, TODO_FILE, argument);
    send_to_char("TODO записан. Спасибо!\n\r", ch);
    sprintf(bfr, "TODO:   [%5d] %s: %s\n", ch->in_room ? ch->in_room->vnum : 0, ch->name, argument);
    convert_dollars(bfr);
    wiznet(bfr, ch, NULL, WIZ_BUGS, 0, 0);
    return;
}


void do_typo(CHAR_DATA *ch, char *argument)
{
    char bfr[MAX_STRING_LENGTH];

    if (for_bug(ch, argument, TYPO_FILE))
	return;

    append_file(ch, TYPO_FILE, argument);
    send_to_char("Опечатка записана. Спасибо!\n\r", ch);
    sprintf(bfr, "TYPO:   [%5d] %s: %s\n", ch->in_room ? ch->in_room->vnum : 0, ch->name, argument);
    convert_dollars(bfr);
    wiznet(bfr, ch, NULL, WIZ_BUGS, 0, 0);
    return;
}

void do_offense(CHAR_DATA *ch, char *argument)
{
    char bfr[MAX_STRING_LENGTH];

    if (for_bug(ch, argument, VIOLATE_FILE))
	return;

    append_file(ch, VIOLATE_FILE, argument);
    send_to_char("Нарушение записано. Спасибо!\n\r", ch);
    sprintf(bfr, "VIOLATION:   [%5d] %s: %s\n", ch->in_room ? ch->in_room->vnum : 0, ch->name, argument);
    convert_dollars(bfr);
    wiznet(bfr, ch, NULL, WIZ_BUGS, 0, 0);
    return;
}

void do_last_com(CHAR_DATA *ch, char *argument)
{
    for_bug(ch, argument, LAST_COMMAND);
    return;
}


void do_rent(CHAR_DATA *ch, char *argument)
{
    send_to_char("В этом Мире отсутствует рента. Просто сохранись и выйди.\n\r", ch);
    return;
}


void do_qui(CHAR_DATA *ch, char *argument)
{
    send_to_char("Если ты хочешь выйти, набери команду полностью.\n\r", ch);
    return;
}

static void quitdrop(CHAR_DATA *ch, OBJ_DATA *obj)
{
    act("$n бросает $p6.", ch, obj, NULL, TO_ROOM);
    act("Ты выбрасываешь $p6, потому как данный предмет не может находится вне Мира.",
	ch, obj, NULL, TO_CHAR);
    
    obj_from_char(obj, TRUE);
    obj_to_room(obj, ch->in_room);
    
    if (IS_OBJ_STAT(obj, ITEM_MELT_DROP))
    {
	act("$p исчеза$r в облачке дыма.", ch, obj, NULL, TO_ALL);
	extract_obj(obj, TRUE, FALSE);
    }
}

void do_quit(CHAR_DATA *ch, char *argument){
    DESCRIPTOR_DATA *d, *d_next;
    OBJ_DATA *obj, *iobj, *onext, *ionext;
    int id;
    char bfr[MAX_STRING_LENGTH];
    bool silent_quit = FALSE;

    if (IS_NPC(ch)) return;

    if (!strcmp(argument, "_silent_quit")) silent_quit = TRUE;

    if (!can_quit(ch))	return;

/*    if ((ch->pcdata->questmob > 0 || ch->pcdata->questobj > 0)
	&& ch->pcdata->countdown > 0)
    {
	ch->pcdata->countdown = 0;
	quest_update(ch);
    } */

    if (ch->in_room && has_trigger(ch->in_room->progs, TRIG_QUIT, EXEC_ALL))
	p_percent_trigger(NULL, NULL, ch->in_room, ch, NULL, NULL, TRIG_QUIT);

    do_quote(ch);

    if (strcmp(argument, "delete")){
        time_t clock;

        clock = ((int)current_time + 60*60*24*7*ch->level);
        sprintf(bfr, "Твой персонаж будет храниться до %s\n\r", ctime(&clock));
        send_to_char(bfr, ch);
        return_limit_from_char(ch, FALSE);
    //    save_chest(0);
    }

    d = ch->desc;
    id = ch->id;


    
    if (!silent_quit){
        act("$n покидает этот мир.", ch, NULL, NULL, TO_SOUND /*TO_ROOM*/);
    }

    sprintf(bfr, "%s has quit.", ch->name);
    log_string(bfr);

    check_auctions(ch, NULL, "выхода владельца из игры");

    sprintf(bfr, "[R: %d] %s@%s выходит из игры.",
	    ch->in_room ? ch->in_room->vnum : 0,
	    ch->name,
	    d != NULL ? d->ip : "unknown"
	    );

    convert_dollars(bfr);
    wiznet(bfr, ch, NULL, WIZ_LOGINS, 0, get_trust(ch));


    for (obj = ch->carrying; obj; obj = onext){
        onext = obj->next_content;

        if (IS_OBJ_STAT(obj, ITEM_QUITDROP)){
            quitdrop(ch, obj);
            continue;
        }

        for (iobj = obj->contains; iobj; iobj = ionext){
            ionext = iobj->next_content;

            if (IS_SET(iobj->extra_flags, ITEM_QUITDROP))
            quitdrop(ch, iobj);
        }
    }
    
    /*
     * After extract_char the ch is no longer valid!
     */

    save_char_obj(ch, FALSE);
    extract_char_for_quit(ch, TRUE);

    if (d != NULL)
	close_socket(d);

    /* toast evil cheating bastards */
    SLIST_FOREACH_SAFE(d, &descriptor_list, link, d_next){
        CHAR_DATA *tch;

        tch = CH(d);

        if (tch && tch->id == id){
            return_limit_from_char(tch, FALSE);
            extract_char_for_quit(tch, TRUE);
            close_socket(d);
        }
    }

    return;
}

void do_follow(CHAR_DATA *ch, char *argument)
{
    /* RT changed to allow unlimited following and follow the NOFOLLOW rules */
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Следовать за кем?\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, NULL, arg, FALSE)) == NULL)
    {
	send_to_char("Таких здесь нет.\n\r", ch);
	return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL)
    {
	act("Но ты предпочитаешь следовать за $N4!", ch, NULL, ch->master, TO_CHAR);
	return;
    }

    if (victim == ch)
    {
	if (ch->master == NULL)
	{
	    act("Ты теперь сам$T по себе.", ch, NULL, SEX_ENDING(ch), TO_CHAR);
	    return;
	}
	stop_follower(ch);
	return;
    }
    if (!IS_NPC(victim) && IS_SET(victim->act, PLR_NOFOLLOW))
    {
	act("$N не желает, чтобы за $t следовали.\n\r", ch, 
	    victim->sex == SEX_FEMALE ? "ней" : "ним", victim, TO_CHAR);
	return;
    }

    REMOVE_BIT(ch->act, PLR_NOFOLLOW);

    if (ch->master != NULL)
	stop_follower(ch);

    add_follower(ch, victim);
    return;
}


void add_follower(CHAR_DATA *ch, CHAR_DATA *master)
{
    if (ch->master != NULL)
    {
	bugf("Add_follower: non-null master for '%s'.", ch->name);
	return;
    }

    ch->master        = master;
    ch->leader        = NULL;

    if (can_see(master, ch))
	act("$n теперь следует за тобой.", ch, NULL, master, TO_VICT);

    act("Теперь ты следуешь за $N4.",  ch, NULL, master, TO_CHAR);

    return;
}



void stop_follower(CHAR_DATA *ch)
{
    if (ch->master == NULL)
    {
	bugf("Stop_follower: null master for '%s'.", ch->name);
	return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM))
    {
	REMOVE_BIT(ch->affected_by, AFF_CHARM);
	affect_strip(ch, gsn_charm_person);
    }

    if (can_see(ch->master, ch) && ch->in_room != NULL)
    {
	act("$n прекращает следовать за тобой.",     ch, NULL, ch->master, TO_VICT   );
    	act("Ты прекращаешь следовать за $N4.",      ch, NULL, ch->master, TO_CHAR   );
    }
    
    if (ch->master->pet == ch)
	ch->master->pet = NULL;

    if (ch->master->mount == ch)
	ch->master->mount = NULL;

    ch->master = NULL;
    ch->leader = NULL;
    ch->mount = NULL;
    return;
}

/* nukes charmed monsters and pets */
void nuke_pets(CHAR_DATA *ch)
{
    CHAR_DATA *pet;

    if (IS_NPC(ch)) return;

    if ((pet = ch->pet) != NULL)
    {
	stop_follower(pet);
	if (pet->in_room != NULL)
	    act("$n медленно изчезает.", pet, NULL, NULL, TO_ROOM);
	extract_char(pet, TRUE);
	ch->pet = NULL;
    }

    if ((pet = ch->mount) != NULL)
    {
	do_dismount(ch, "");
	if (pet->in_room != NULL)
	    act("$n медленно исчезает...", pet, NULL, NULL, TO_ROOM);
	else
	    log_string("void nuke_pets: Extracting null pet");

	ch->mount = NULL;
	ch->riding = FALSE;
	extract_char(pet, TRUE);
    }

    return;
}



void die_follower(CHAR_DATA *ch)
{
    CHAR_DATA *fch, *fch_next;

    if (ch->master != NULL)
    {
	if (ch->master->pet == ch)
	    ch->master->pet = NULL;
	stop_follower(ch);
    }

    ch->leader = NULL;

    LIST_FOREACH_SAFE(fch, &char_list, link, fch_next)
    {
	if (fch->master == ch)
	{
	    if (!IS_NPC(ch) && IS_NPC(fch) && !IS_SWITCHED(fch) && !fch->fighting
		&& fch->pIndexData && fch->in_room && fch->pIndexData->area != fch->in_room->area)
	    {
	        pet_gone(fch); 
	        continue;
	    }
	    else 
	        stop_follower(fch);
	}
	if (fch->leader == ch)
	    fch->leader = fch;

    }

    return;
}

#define ORDER_DISALLOW 	"delete mob wear remove hold wield drop second quit follow hide sneak cast steal afk sacrifice description attack suicide"

bool check_disallow(char *command)
{
    int cmd, i;

    for (i = 0; cmd_table[i].name[0] != '\0'; i++)
	if (!str_prefix(command, cmd_table[i].name))
	    break;

    if (cmd_table[i].name[0] == '\0')
	return FALSE;

    for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++)
	if (cmd_table[cmd].do_fun == cmd_table[i].do_fun
	    && is_exact_name(cmd_table[cmd].name, ORDER_DISALLOW))
	{
	    return TRUE;
	}

    return FALSE;
}

bool check_wait(CHAR_DATA *och, CHAR_DATA *ch)
{
    if (och->wait > 0)
    {
	act("$N пока не в состоянии выполнить твой приказ.", ch, NULL, och, TO_CHAR);
	return TRUE;
    }

    return FALSE;
}

void do_order(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *och;
    CHAR_DATA *och_next;
    bool found;
    bool fAll;

    argument = one_argument(argument, arg);
    one_argument(argument, arg2);


    if (arg[0] == '\0' || argument[0] == '\0')
    {
	send_to_char("Приказать кому что?\n\r", ch);
	return;
    }


    if (IS_SET(ch->in_room->room_flags, ROOM_HOLY))
    {
	send_to_char("Не в Божьем доме.\n\r", ch);
	return;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_GUILD))
    {
	send_to_char("Не в братской гильдии.\n\r", ch);
	return;
    }


    if (IS_AFFECTED(ch, AFF_CHARM))
    {
	send_to_char("Ты чувствуешь себя готовым исполнять, а не отдавать приказы.\n\r", ch);
	return;
    }

    one_argument(arg2, buf);

    if (check_disallow(buf))
    {
	send_to_char("Это не может быть сделано!\n\r", ch);
	return;
    }


    if (!str_cmp(arg, "all") || !str_cmp(arg, "все"))
    {
	fAll   = TRUE;
	victim = NULL;
    }
    else
    {
	fAll   = FALSE;
	if (!str_cmp(arg, "mount") || !str_cmp(arg, "лошадь"))
	{
	    if (!ch->mount)
	    {
		send_to_char("У тебя нет лошади.\n\r", ch);
		return;
	    }

	    if (ch->mount->in_room != ch->in_room)
	    {
		send_to_char("Здесь нет твоей лошади!\n\r", ch);
		return;
	    }
	    else
	    {
		victim = ch->mount;
	    }
	}
	else if ((victim = get_char_room(ch, NULL, arg, FALSE)) == NULL)
	{
	    send_to_char("Таких здесь нет.\n\r", ch);
	    return;
	}

	if (victim == ch)
	{
	    send_to_char("Да, да, сейчас же...\n\r", ch);
	    return;
	}

	if (victim->mount == ch)
	{
	    if (!mount_success(ch, victim, FALSE, FALSE))
	    {
		act("$N игнорирует твои приказы.", ch, NULL, victim, TO_CHAR);
		return;
	    }
	    else
	    {
		if (check_wait(victim, ch))
		    return;

		if (!IS_NPC(victim))
		{
			sprintf(buf, "%s приказывает тебе '%s'.", ch->name, argument);
			send_to_char(buf, victim);
		}
		interpret(victim, argument);
		return;
	    }
	}
	else if (!IS_AFFECTED(victim, AFF_CHARM) || victim->master != ch
		 || (IS_IMMORTAL(victim) && victim->trust >= ch->trust))
	{
	    printf_to_char("Делай это сам%s!\n\r", ch, SEX_ENDING(ch));
	    return;
	}
    }

    found = FALSE;
    LIST_FOREACH_SAFE(och, &(ch->in_room->people), room_link, och_next)
    {
	if (IS_AFFECTED(och, AFF_CHARM)
	    && get_master(och) == ch
	    && can_see(ch, och)
	    && (fAll || och == victim)
	    && (!IS_NPC(och)
		|| (och->pIndexData->vnum != MOB_BONE_SHIELD
		    && (och->pIndexData->vnum != MOB_VNUM_ZOMBIE
			|| IS_SET(och->act, ACT_DEAD_WARRIOR))))
	   && !IS_SWITCHED(och))
	{
	    found = TRUE;

	    if (check_wait(och, ch))
		continue;

	    sprintf(buf, "$n приказывает тебе '%s'.", argument);
	    act(buf, ch, NULL, och, TO_VICT);
	    interpret(och, argument);
	}
    }

    if (found)
    {
	WAIT_STATE(ch, PULSE_VIOLENCE);
	send_to_char("Ok.\n\r", ch);
    }
    else
	send_to_char("Ты не видишь никого, кто бы следовал за тобой или понимал бы твои приказы.\n\r", ch);

    return;
}



void do_group(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim, *gch, *safe_gch;
    int avg;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	CHAR_DATA *gch;
	CHAR_DATA *leader;

	leader = (ch->leader != NULL) ? ch->leader : ch;
	sprintf(buf, "Группа %s:\n\r\n\r", PERS(leader, ch, 1));
	send_to_char(buf, ch);


	send_to_char("           Имя                 Жизнь       Мана        Шаги     TNL   Натура\n\r", ch);
	send_to_char("-------------------------------------------------------------------------------\n\r", ch);

	LIST_FOREACH_SAFE(gch, &char_list, link, safe_gch)
	{
	    if (is_same_group(gch, ch))
	    {
		strcpy(arg, class_table[gch->classid].who_name);
		arg[5] = '\0';
		sprintf(buf,
			"[%2d %5s] %-16s %5d/%-5d %5d/%-5d %5d/%-5d %4d  %s{x\n\r",
			gch->level,
			IS_NPC(gch) ? "Моб" : arg,
			capitalize(PERS(gch, ch, 0)),
			gch->hit,   gch->max_hit,
			gch->mana,  gch->max_mana,
			gch->move,  gch->max_move,
			TNL(gch), get_align(gch, ch));
		send_to_char(buf, ch);
	    }
	}
	return;
    }

    if ((victim = get_char_room(ch, NULL, arg, FALSE)) == NULL)
    {
	send_to_char("Таких здесь нет.\n\r", ch);
	return;
    }

    if (ch->master != NULL || (ch->leader != NULL && ch->leader != ch))
    {
	send_to_char("Но ты уже следуешь за кем-то!\n\r", ch);
	return;
    }

    if (IS_NPC(victim))
    {
	act_new("Ты не можешь принять $N3 в группу.", ch, NULL, victim, TO_CHAR, POS_SLEEPING);
	return;
    }

    if (victim == ch)
    {
	send_to_char("Хмм... Решил сам себя в группу взять?\n\r", ch);
	return;
    }

    if (is_lycanthrope(victim))
    {
	act_new("Ты не можешь принять $N3 в группу.", ch, NULL, victim, TO_CHAR, POS_SLEEPING);
	return;	
    }

    if (victim->master != ch && ch != victim)
    {
	act_new("$N не следует за тобой.", ch, NULL, victim, TO_CHAR, POS_SLEEPING);
	return;
    }

    if (IS_AFFECTED(victim, AFF_CHARM))
    {
	send_to_char("Ты не можешь выгнать из группы очарованных.\n\r", ch);
	return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM))
    {
	act_new("Тебе слишком нравится следовать за $N4!",
		ch, NULL, victim, TO_VICT, POS_SLEEPING);
	return;
    }

    if (is_same_group(victim, ch) && ch != victim)
    {
	victim->leader = NULL;
	act_new("$n выгоняет $N3 из группы.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
	act_new("$n выгоняет тебя из группы.", ch, NULL, victim, TO_VICT, POS_SLEEPING);
	act_new("Ты выгоняешь $N3 из своей группы.", ch, NULL, victim, TO_CHAR, POS_SLEEPING);
	return;
    }

    avg = group_level(ch, AVG_GROUP_LEVEL);

    if (avg + PK_RANGE < victim->level)
    {
	act_new("Ты слишком сил$t для группы $n1.", ch,
		victim->sex == SEX_MALE ? "ен" :
		victim->sex == SEX_FEMALE ? "ьна" :
		victim->sex == SEX_MANY ? "ьны" : "ьно",
		victim, TO_VICT, POS_SLEEPING);
	act_new("$N слишком сил$t для твоей группы.", ch,
		victim->sex == SEX_MALE ? "ен" :
		victim->sex == SEX_FEMALE ? "ьна" :
		victim->sex == SEX_MANY ? "ьны" : "ьно",
		victim, TO_CHAR, POS_SLEEPING);
	return;
    }

    if (avg - PK_RANGE > victim->level)
    {
	act_new("Ты маловат$t для группы $n1.", ch, SEX_ENDING(victim), victim, TO_VICT, POS_SLEEPING);
	act_new("$N маловат$t для твоей группы.", ch, SEX_ENDING(victim), victim, TO_CHAR, POS_SLEEPING);
	return;
    }

    if (!is_marry(ch, victim))
    {
	LIST_FOREACH_SAFE(gch, &char_list, link, safe_gch)
	{
	    if (!IS_NPC(gch) && is_same_group(ch, gch))
	    {
		if (!is_compatible_races(victim, gch, TRUE)
		    && (is_clan(gch) != is_clan(victim)
			|| IS_INDEPEND(victim)))
		{
		    if (gch == ch)
			sprintf(buf, "Ты не уживешься в одной группе с %s.\n\r", cases(victim->name, 4));
		    else
			sprintf(buf, "%s не уживется в одной группе с %s.\n\r", gch->name, cases(victim->name, 4));

		    send_to_char(buf, ch);
		    return;
		}
	    }
	}
    }

    victim->leader = ch;
    act_new("$N присоединяется к группе $n1.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
    act_new("Ты присоединяешься к группе $n1.", ch, NULL, victim, TO_VICT, POS_SLEEPING);
    act_new("$N присоединяется к твоей группе.", ch, NULL, victim, TO_CHAR, POS_SLEEPING);

    return;
}



/*
 * 'Split' originally by Gnort, God of Chaos.
 */
void do_split(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *gch;
    int members;
    int amount_gold = 0, amount_silver = 0;
    int share_gold, share_silver;
    int extra_gold, extra_silver;

    argument = one_argument(argument, arg1);
    one_argument(argument, arg2);

    if (arg1[0] == '\0')
    {
	send_to_char("Разделить сколько?\n\r", ch);
	return;
    }

    amount_silver = atoi(arg1);

    if (arg2[0] != '\0')
	amount_gold = atoi(arg2);

    if (amount_gold < 0 || amount_silver < 0)
    {
	send_to_char("Твоей группе это вряд ли понравится.\n\r", ch);
	return;
    }

    if (amount_gold == 0 && amount_silver == 0)
    {
	send_to_char("У тебя нет ни одной монеты.\n\r", ch);
	return;
    }

    if (ch->gold <  amount_gold || ch->silver < amount_silver)
    {
	send_to_char("У тебя нет такого количества денег.\n\r", ch);
	return;
    }

    members = 0;
    
    LIST_FOREACH(gch, &(ch->in_room->people), room_link)
    {
	if (is_same_group(gch, ch) && (!IS_NPC(gch) || IS_SWITCHED(gch)))
	    members++;
    }

    if (members < 2)
    {
	send_to_char("Просто оставь себе.\n\r", ch);
	return;
    }

    share_silver = amount_silver / members;
    extra_silver = amount_silver % members;

    share_gold   = amount_gold / members;
    extra_gold   = amount_gold % members;

    if (share_gold == 0 && share_silver == 0)
    {
	send_to_char("Даже не суетись, скряга.\n\r", ch);
	return;
    }

    ch->silver	-= amount_silver;
    ch->silver	+= share_silver + extra_silver;
    ch->gold 	-= amount_gold;
    ch->gold 	+= share_gold + extra_gold;

    if (share_silver > 0)
    {
	sprintf(buf,
		"Ты делишь %d серебряных монет. Твоя доля - %d серебра.\n\r",
		amount_silver, share_silver + extra_silver);
	send_to_char(buf, ch);
    }

    if (share_gold > 0)
    {
	sprintf(buf,
		"Ты делишь %d золотых монет. Твоя доля - %d золота.\n\r",
		amount_gold, share_gold + extra_gold);
	send_to_char(buf, ch);
    }

    if (share_gold == 0)
    {
	sprintf(buf, "$n делит %d серебряных монет. Твоя доля - %d серебра.",
		amount_silver, share_silver);
    }
    else if (share_silver == 0)
    {
	sprintf(buf, "$n делит %d золотых монет. Твоя доля - %d золота.",
		amount_gold, share_gold);
    }
    else
    {
	sprintf(buf,
		"$n делит %d серебряных и %d золотых монет, давая тебе %d серебра и %d золота.\n\r",
		amount_silver, amount_gold, share_silver, share_gold);
    }

    LIST_FOREACH(gch, &(ch->in_room->people), room_link)
    {
	if (gch != ch && is_same_group(gch, ch) && (!IS_NPC(gch) || IS_SWITCHED(gch)))
	{
	    if (can_carry_w(gch) <= get_carry_weight(gch))
	    {
	        act("$n хочет поделиться с тобой монетами, но ты не можешь нести столько.\n\r", 
		    ch, NULL, gch, TO_VICT);
	        act("$N не может нести столько, поэтому ты забираешь $S долю.\n\r", 
		    ch, NULL, gch, TO_CHAR);
	        ch->gold += share_gold;
	        ch->silver += share_silver;
	    }
	    else
	    {
		act(buf, ch, NULL, gch, TO_VICT);
	        gch->gold += share_gold;
	        gch->silver += share_silver;
	    }	    
	}
    }

    return;
}



void do_gtell(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *gch, *safe_gch;

    if (argument[0] == '\0')
    {
	send_to_char("Сказать группе что?\n\r", ch);
	return;
    }

    if (!IS_NPC(ch) && IS_SET(ch->comm, COMM_NOTELL))
    {
	send_to_char("Твое сообщение не достигает адресата!\n\r", ch);
	return;
    }

    LIST_FOREACH_SAFE(gch, &char_list, link, safe_gch)
    {
	if (is_same_group(gch, ch))
	    act_new("$n говорит группе: {G$t{x",
		    ch, argument, gch, TO_VICT, POS_SLEEPING);
    }
    act_new("Ты говоришь группе: {G$t{x",
	    ch, argument, NULL, TO_CHAR, POS_SLEEPING);

    return;
}



/*
 * It is very important that this be an equivalence relation:
 * (1) A ~ A
 * (2) if A ~ B then B ~ A
 * (3) if A ~ B  and B ~ C, then A ~ C
 */
bool is_same_group(CHAR_DATA *ach, CHAR_DATA *bch)
{
    int count;

    if (ach == NULL || bch == NULL)
	return FALSE;

    count = 0;
    while(ach->leader != NULL && ++count < 10)
	ach = ach->leader;

    count = 0;
    while(bch->leader != NULL && ++count < 10)
	bch = bch->leader;

    return ach == bch;
}

#if 0
void show_colours(CHAR_DATA *ch)
{
}

void show_schemas(CHAR_DATA *ch)
{
}

void set_colour(CHAR_DATA *ch, char *argument)
{
}
#endif

/*
 * ColoUr setting and unsetting, way cool, Ant Oct 94
 *        revised to include config colour, Ant Feb 95
 */

void do_colour( CHAR_DATA *ch, char *argument )
{
    char 	arg[ MAX_STRING_LENGTH ];

    if( IS_NPC( ch ) )
    {
	send_to_char( "Зачем тебе цвет?\n\r", ch );
	return;
    }

    argument = one_argument( argument, arg );

    if( arg[0] == '\0' || !strcmp(arg,"noprint") )
    {
	if( !IS_SET( ch->act, PLR_COLOUR ) )
	{
	    SET_BIT( ch->act, PLR_COLOUR );
	    if (strcmp(arg,"noprint"))
		send_to_char( "Цвет включен.\n\r"
			      "Инструкции:\n\r   colour {c<{xfield{c> <{xcolour{c>{x\n\r"
			      "   colour {c<{xfield{c>{x {cbeep{x|{cnobeep{x\n\r"
			      "Наберите help {ccolour{x для детальной информации.\n\r"
			      "ColoUr is brought to you by Lope, ant@@solace.mh.se.\n\r", ch );
	}
	else
	{
	    send_to_char( "Цвет теперь будет выключен.\n\r", ch );
	    REMOVE_BIT( ch->act, PLR_COLOUR );
	}
	return;
    }

    if( !str_cmp( arg, "default" ) || !str_cmp( arg, "поумолчанию" ))
    {
	default_colour( ch );
	send_to_char( "Установлен цвет по умолчанию.\n\r", ch );
	return;
    }

    if( !str_cmp( arg, "all" ) || !str_cmp( arg, "все" ) )
    {
	all_colour( ch, argument );
	return;
    }

    /*
     * Yes, I know this is ugly and unnessessary repetition, but its old
     * and I can't justify the time to make it pretty. -Lope
     */
    if( !str_cmp( arg, "text" ) )
    {
	ALTER_COLOUR( text )
    }
    else if( !str_cmp( arg, "auction" ) )
    {
	ALTER_COLOUR( auction )
    }
    else if( !str_cmp( arg, "auction_text" ) )
    {
	ALTER_COLOUR( auction_text )
    }
    else if( !str_cmp( arg, "gossip" ) )
    {
	ALTER_COLOUR( gossip )
    }
    else if( !str_cmp( arg, "gossip_text" ) )
    {
	ALTER_COLOUR( gossip_text )
    }
    else if( !str_cmp( arg, "music" ) )
    {
	ALTER_COLOUR( music )
    }
    else if( !str_cmp( arg, "music_text" ) )
    {
	ALTER_COLOUR( music_text )
    }
    else if( !str_cmp( arg, "question" ) )
    {
	ALTER_COLOUR( question )
    }
    else if( !str_cmp( arg, "question_text" ) )
    {
	ALTER_COLOUR( question_text )
    }
    else if( !str_cmp( arg, "answer" ) )
    {
	ALTER_COLOUR( answer )
    }
    else if( !str_cmp( arg, "answer_text" ) )
    {
	ALTER_COLOUR( answer_text )
    }
    else if( !str_cmp( arg, "quote" ) )
    {
	ALTER_COLOUR( quote )
    }
    else if( !str_cmp( arg, "quote_text" ) )
    {
	ALTER_COLOUR( quote_text )
    }
    else if( !str_cmp( arg, "immtalk_text" ) )
    {
	ALTER_COLOUR( immtalk_text )
    }
    else if( !str_cmp( arg, "immtalk_type" ) )
    {
	ALTER_COLOUR( immtalk_type )
    }
    else if( !str_cmp( arg, "info" ) )
    {
	ALTER_COLOUR( info )
    }
    else if( !str_cmp( arg, "say" ) )
    {
	ALTER_COLOUR( say )
    }
    else if( !str_cmp( arg, "say_text" ) )
    {
	ALTER_COLOUR( say_text )
    }
    else if( !str_cmp( arg, "tell" ) )
    {
	ALTER_COLOUR( tell )
    }
    else if( !str_cmp( arg, "tell_text" ) )
    {
	ALTER_COLOUR( tell_text )
    }
    else if( !str_cmp( arg, "reply" ) )
    {
	ALTER_COLOUR( reply )
    }
    else if( !str_cmp( arg, "reply_text" ) )
    {
	ALTER_COLOUR( reply_text )
    }
    else if( !str_cmp( arg, "gtell_text" ) )
    {
	ALTER_COLOUR( gtell_text )
    }
    else if( !str_cmp( arg, "gtell_type" ) )
    {
	ALTER_COLOUR( gtell_type )
    }
    else if( !str_cmp( arg, "wiznet" ) )
    {
	ALTER_COLOUR( wiznet )
    }
    else if( !str_cmp( arg, "room_title" ) )
    {
	ALTER_COLOUR( room_title )
    }
    else if( !str_cmp( arg, "room_text" ) )
    {
	ALTER_COLOUR( room_text )
    }
    else if( !str_cmp( arg, "room_exits" ) )
    {
	ALTER_COLOUR( room_exits )
    }
    else if( !str_cmp( arg, "room_things" ) )
    {
	ALTER_COLOUR( room_things )
    }
    else if( !str_cmp( arg, "prompt" ) )
    {
	ALTER_COLOUR( prompt )
    }
    else if( !str_cmp( arg, "fight_death" ) )
    {
	ALTER_COLOUR( fight_death )
    }
    else if( !str_cmp( arg, "fight_yhit" ) )
    {
	ALTER_COLOUR( fight_yhit )
    }
    else if( !str_cmp( arg, "fight_ohit" ) )
    {
	ALTER_COLOUR( fight_ohit )
    }
    else if( !str_cmp( arg, "fight_thit" ) )
    {
	ALTER_COLOUR( fight_thit )
    }
    else if( !str_cmp( arg, "fight_skill" ) )
    {
	ALTER_COLOUR( fight_skill )
    }
    else
    {
	send_to_char( "Незвестный параметр.\n\r", ch );
	return;
    }

    send_to_char( "Установлены новые цветовые параметры.\n\r", ch );
    return;
}

#if 0
void do_colour(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
    {
	send_to_char("Зачем тебе цвет?\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || !strcmp(arg, "noprint"))
    {
	if (!IS_SET(ch->act, PLR_COLOUR))
	{
	    SET_BIT(ch->act, PLR_COLOUR);
	    if (strcmp(arg, "noprint"))
		send_to_char("Цвет включен.\n\r"
			     "Наберите help {ccolour{x для детальной информации.\n\r"
			     "ColoUr is brought to you by Lope, ant@solace.mh.se.\n\r"
			     "Colour configuration by Balderdash Imms\n\r", ch);
	}
	else
	{
	    send_to_char("Цвет теперь будет выключен.\n\r", ch);
	    REMOVE_BIT(ch->act, PLR_COLOUR);
	}
	return;
    }

    if (!str_cmp(arg, "default") || !str_cmp(arg, "поумолчанию"))
    {
	default_colour(ch);
	send_to_char("Установлен цвет по умолчанию.\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "schema") || !str_cmp(arg, "схема"))
    {
	argument = one_argument(argument, arg);

	if (IS_NULLSTR(arg))
	    show_colours(ch);
	else if (!str_cmp(arg, "список") || !str_cmp(arg, "list"))
	    show_schemas(ch);
	else
	    set_colour(ch, argument);
	return;
    }
    /*
     * Yes, I know this is ugly and unnessessary repetition, but its old
     * and I can't justify the time to make it pretty. -Lope
     */
    if (!str_cmp(arg, "text"))
    {
	ALTER_COLOUR(text)
    }
    else if (!str_cmp(arg, "auction"))
    {
	ALTER_COLOUR(auction)
    }
    else if (!str_cmp(arg, "auction_text"))
    {
	ALTER_COLOUR(auction_text)
    }
    else if (!str_cmp(arg, "gossip"))
    {
	ALTER_COLOUR(gossip)
    }
    else if (!str_cmp(arg, "gossip_text"))
    {
	ALTER_COLOUR(gossip_text)
    }
    else if (!str_cmp(arg, "music"))
    {
	ALTER_COLOUR(music)
    }
    else if (!str_cmp(arg, "music_text"))
    {
	ALTER_COLOUR(music_text)
    }
    else if (!str_cmp(arg, "question"))
    {
	ALTER_COLOUR(question)
    }
    else if (!str_cmp(arg, "question_text"))
    {
	ALTER_COLOUR(question_text)
    }
    else if (!str_cmp(arg, "answer"))
    {
	ALTER_COLOUR(answer)
    }
    else if (!str_cmp(arg, "answer_text"))
    {
	ALTER_COLOUR(answer_text)
    }
    else if (!str_cmp(arg, "quote"))
    {
	ALTER_COLOUR(quote)
    }
    else if (!str_cmp(arg, "quote_text"))
    {
	ALTER_COLOUR(quote_text)
    }
    else if (!str_cmp(arg, "immtalk_text"))
    {
	ALTER_COLOUR(immtalk_text)
    }
    else if (!str_cmp(arg, "immtalk_type"))
    {
	ALTER_COLOUR(immtalk_type)
    }
    else if (!str_cmp(arg, "info"))
    {
	ALTER_COLOUR(info)
    }
    else if (!str_cmp(arg, "say"))
    {
	ALTER_COLOUR(say)
    }
    else if (!str_cmp(arg, "say_text"))
    {
	ALTER_COLOUR(say_text)
    }
    else if (!str_cmp(arg, "tell"))
    {
	ALTER_COLOUR(tell)
    }
    else if (!str_cmp(arg, "tell_text"))
    {
	ALTER_COLOUR(tell_text)
    }
    else if (!str_cmp(arg, "reply"))
    {
	ALTER_COLOUR(reply)
    }
    else if (!str_cmp(arg, "reply_text"))
    {
	ALTER_COLOUR(reply_text)
    }
    else if (!str_cmp(arg, "gtell_text"))
    {
	ALTER_COLOUR(gtell_text)
    }
    else if (!str_cmp(arg, "gtell_type"))
    {
	ALTER_COLOUR(gtell_type)
    }
    else if (!str_cmp(arg, "wiznet"))
    {
	ALTER_COLOUR(wiznet)
    }
    else if (!str_cmp(arg, "room_title"))
    {
	ALTER_COLOUR(room_title)
    }
    else if (!str_cmp(arg, "room_text"))
    {
	ALTER_COLOUR(room_text)
    }
    else if (!str_cmp(arg, "room_exits"))
    {
	ALTER_COLOUR(room_exits)
    }
    else if (!str_cmp(arg, "room_things"))
    {
	ALTER_COLOUR(room_things)
    }
    else if (!str_cmp(arg, "prompt"))
    {
	ALTER_COLOUR(prompt)
    }
    else if (!str_cmp(arg, "fight_death"))
    {
	ALTER_COLOUR(fight_death)
    }
    else if (!str_cmp(arg, "fight_yhit"))
    {
	ALTER_COLOUR(fight_yhit)
    }
    else if (!str_cmp(arg, "fight_ohit"))
    {
	ALTER_COLOUR(fight_ohit)
    }
    else if (!str_cmp(arg, "fight_thit"))
    {
	ALTER_COLOUR(fight_thit)
    }
    else if (!str_cmp(arg, "fight_skill"))
    {
	ALTER_COLOUR(fight_skill)
    }
    else
    {
	send_to_char("Незвестный параметр.\n\r", ch);
	return;
    }

    send_to_char("Установлены новые цветовые параметры.\n\r", ch);
    return;
}
#endif

void do_bounty(CHAR_DATA *ch, char *argument)
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    DESCRIPTOR_DATA *d, *safe_d;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (!IS_SET(ch->in_room->room_flags, ROOM_BOUNTY_OFFICE))
    {
	send_to_char("Здесь нельзя никого заказать. Ищи место с насильниками и убийцами.\n\r", ch);
	return;
    }

    if (arg1[0] == '\0')
    {
	send_to_char("Заказать какого персонажа?\n\rСинтаксис: заказать <жертва> <количество золота>\n\r           заказать список\n\r", ch);
	return;
    }

    if (!strcmp(arg1, "list") || !strcmp(arg1, "список"))
    {
	DESCRIPTOR_DATA *d;

	send_to_char("\n\rПерсонаж    Награда(золота)\n\r = == = == = == = == = == = == = == = == = == = \n\r", ch);
	SLIST_FOREACH_SAFE(d, &descriptor_list, link, safe_d)
	{
	    CHAR_DATA *wch;

	    if (d->connected != CON_PLAYING)
		continue;

	    wch = CH(d);

	    if (wch->pcdata->bounty > 0)
	    {
		sprintf(arg2, "%-10s  %d\n\r", wch->name, wch->pcdata->bounty);
		send_to_char(arg2, ch);
	    }
	}
	return;
    }

    if (arg2[0] == '\0')
    {
	send_to_char("За сколько золота ты хотел заказать этого персонажа?\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, arg1)) == NULL)
    {
	send_to_char("Таких персонажей сейчас в игре нет!\n\r", ch);
	return;
    }

    if (IS_NPC(victim))
    {
	send_to_char("Ты не можешь заказывать мобов!\n\r", ch);
	return;
    }

    if (IS_IMMORTAL(victim))
    {
	send_to_char("Ты в этом уверен? Боги будут очень недовольны...\n\r", ch);
	return;
    }

    if (ch == victim)
    {
	send_to_char("Ты хочешь заказать самого себя???\n\r", ch);
	return;
    }


    if (is_number(arg2))
    {
	int amount;
	amount   = atoi(arg2);
	if (amount < 2*victim->level)
	{
	    sprintf(buf, "Что?? Делай это сам%s за эти деньги! Голова %s стоит дороже.\n\r", SEX_ENDING(ch), cases(victim->name, 1));
	    send_to_char(buf, ch);
	    return;
	}
	if (ch->gold < amount)
	{
	    send_to_char("У тебя нет такого количества золота!\n\r", ch);
	    return;
	}
	ch->gold -= amount;
	victim->pcdata->bounty += amount;

	sprintf(buf, "Ты платишь %d золота за голову %s{x.\n\r", amount, cases(victim->name, 1));
	send_to_char(buf, ch);

	sprintf(buf, "{gЗа твою голову теперь дают %d золота.{x\n\r", victim->pcdata->bounty);
	send_to_char(buf, victim);

	sprintf(buf, "{gЗа голову %s теперь дают %d золота.{x\n\r", cases(victim->name, 1), victim->pcdata->bounty);

	SLIST_FOREACH_SAFE(d, &descriptor_list, link, safe_d)
	    if (d->connected == CON_PLAYING && d->character != victim)
		send_to_char(buf, d->character);

	return;
    }
}


void do_filter(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char bfr[MAX_STRING_LENGTH], b[MIL];
    int i, pos = -1;

    if (IS_NPC(ch))
	return;

    if (argument[0] == '\0'
	|| !str_cmp(argument, "показ")
	|| !str_cmp(argument, "show")
	|| !str_cmp(argument, "список"))
    {
	int count = 1;

	bfr[0] = '\0';

	for (i = 0; i < MAX_FILTERS; i++)
	    if (!IS_NULLSTR(ch->pcdata->filter[i].ch))
	    {
		sprintf(b, "%2d) %s\n\r",
			count++, capitalize(ch->pcdata->filter[i].ch));
		strcat(bfr, b);
	    }

	if (bfr[0] != '\0')
	{
	    send_to_char("Список игнорируемых игроков:\n\r", ch);
	    send_to_char(bfr, ch);
	}
	else
	    send_to_char("Ты никого не игнорируешь.\n\r", ch);

	return;
    }

    if ((victim = get_char_world(ch, argument)) != NULL)
	argument = victim->name;

    for (i = 0; i < MAX_FILTERS; i++)
	if (!IS_NULLSTR(ch->pcdata->filter[i].ch))
	{
	    if (!str_cmp(argument, ch->pcdata->filter[i].ch))
	    {
		ch->pcdata->filter[i].ch = NULL;

		sprintf(bfr,
			"Ты удаляешь %s из списка игнорируемых игроков.\n\r",
			capitalize(cases(argument, 3)));
		send_to_char(bfr, ch);
		return;
	    }
	}
	else if (pos < 0)
	    pos = i;

    if (!victim)
    {
	send_to_char("Таких нет в этом мире.\n\r", ch);
	return;
    }

    if (IS_NPC(victim))
    {
	send_to_char("Ты можешь игнорировать только игроков.\n\r", ch);
	return;
    }

    if (victim == ch)
    {
	printf_to_char("Ты так сильно устал%s от себя?\n\r", ch, SEX_ENDING(ch));
	return;
    }

    if (IS_IMMORTAL(victim))
    {
	send_to_char("Ты не можешь игнорировать Бессмертных.\n\r", ch);
	return;
    }

    if (pos < 0)
    {
	send_to_char("Ты уже и так много кого игнорируешь...\n\r", ch);
	return;
    }

    free_string(ch->pcdata->filter[pos].ch);
    ch->pcdata->filter[pos].ch = str_dup(victim->name);
    sprintf(bfr, "Ты вносишь %s в список игнорируемых игроков.\n\r",
	    cases(victim->name, 3));
    send_to_char(bfr, ch);

    return;
}

void p_openw(char *command, char *argument);

void do_email(CHAR_DATA *ch, char *argument)
{
    char arg[MIL], email[MIL], buf[MSL];

    if (IS_NPC(ch))
	return;

    argument = one_argument(argument, arg);

    if (IS_NULLSTR(arg) || !str_prefix(arg, "show") || !str_prefix(arg, "показ"))
    {
	sprintf(buf, "Твой e-mail: %s\n\r", ch->pcdata->email);
	send_to_char(buf, ch);
	return;
    }

    if (!str_prefix(arg, "выслать") || !str_prefix(arg, "ключ") ||
	!str_prefix(arg, "send") || !str_prefix(arg, "key"))
    {
	int i;

	if (IS_NULLSTR(ch->pcdata->email))
	{
	    send_to_char("У тебя не указан адрес электронной почты. "
			 "Просто смени его командой 'почта сменить <адрес>'.\n\r", ch);
	    return;
	}

	for(i = 0; i < sizeof(ch->pcdata->email_key) - 1; i++)
	    arg[i] = (char) (number_bits(1) == 0 ? number_range('A', 'Z') : number_range('a', 'z'));

	arg[i] = '\0';

	sprintf(buf, "На адрес %s выслан ключ для смены адреса.\n\r", ch->pcdata->email);
	send_to_char(buf, ch);

	strlcpy(ch->pcdata->email_key, arg, sizeof(ch->pcdata->email_key));
	sprintf(buf, "Change mail key: %s", arg);
	
	mailing(ch->pcdata->email, "BalderDash", buf);
	return;
    }

    if (!str_prefix(arg, "сменить") || !str_prefix(arg, "change"))
    {
	argument = one_argument(argument, email);

	if (!CHECK_EMAIL(email))
	{
	    send_to_char("Таких адресов не бывает.\n\r", ch);
	    return;
	}

	if (!IS_NULLSTR(ch->pcdata->email) &&
	    (IS_NULLSTR(argument) || IS_NULLSTR(ch->pcdata->email_key)
	     || strcmp(ch->pcdata->email_key, argument)))
	{
	    send_to_char("Неправильный ключ.\n\r", ch);
	    return;
	}

	free_string(ch->pcdata->email);
	ch->pcdata->email_key[0] = '\0';
	ch->pcdata->email = str_dup(email);
	send_to_char("Адрес успешно изменен.\n\r", ch);
	return;
    }

    send_to_char("Доступные команды: показ, выслать, сменить.\n\r", ch);
}

void do_beep(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Привлечь внимание кого?\n\r", ch);
	return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL || IS_NPC(victim))
    {
	send_to_char ("Таких здесь нет.\n\r", ch);
	return;
    }

    if (victim == ch)
    {
	send_to_char("Хочешь себя разбудить?\n\r", ch);
	return;
    }
    act("{*$n пытается привлечь твое внимание.", ch, NULL, victim, TO_VICT);
    act_new("Ты пытаешься привлечь внимание $N1.", ch, argument, victim,
	    TO_CHAR, POS_RESTING);

    WAIT_STATE(ch, 3 * PULSE_VIOLENCE);
    return;
}


void do_iptalk(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d, *safe_d;
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument(argument, arg);

    if ((victim = get_char_world(ch, arg)) == NULL || victim->desc == NULL)
    {
	send_to_char("Таких здесь нет.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	act("Адрес играющего: {c$t{x", ch, victim->desc->ip, ch, TO_CHAR);
	return;
    }

    act("Ты говоришь играющим с $T: {c$t{x", ch, argument, victim->desc->ip, TO_CHAR);

    SLIST_FOREACH_SAFE(d, &descriptor_list, link, safe_d)
	if (d->connected == CON_PLAYING
	    && ch != d->character
	    && !str_cmp(victim->desc->ip, d->ip))
	    act("$N говорит всем играющим с твоего адреса: {c$t{x", d->character, argument, ch, TO_CHAR);

    return;
}

void do_attract_attention(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int skill;

    if ((skill = get_skill(ch, gsn_attract_attention)) < 1)
    {
	send_to_char("Ты не знаешь, как это делать.\n\r", ch);
	return;
    }

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Привлечь внимание кого?\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, NULL, arg, FALSE)) == NULL)
    {
	send_to_char ("Таких здесь нет.\n\r", ch);
	return;
    }

    if (victim == ch)
    {
	send_to_char("Хочешь привлечь внимание к своей персоне?\n\r", ch);
	return;
    }

    if (victim->fighting)
    {
	act("$N2 пока не до тебя.", ch, NULL, victim, TO_CHAR);
	return;
    }

    if (!IS_AFFECTED(victim, AFF_CHARM) || victim->master != ch)
    {
	act("$N даже не смотрит в твою сторону.", ch, NULL, victim, TO_CHAR);
	return;
    }

    if (victim->wait == 0)
    {
	act("$N уже и так полностью весь во внимании.", ch, NULL, victim, TO_CHAR);
	return;
    }

    act("$n пытается привлечь твое внимание.", ch, NULL, victim, TO_VICT);
    act("Ты пытаешься привлечь внимание $N1.", ch, NULL, victim, TO_CHAR);

    if (number_percent() > skill)
    {
	check_improve(ch, victim, gsn_attract_attention, FALSE, 2);
	ch->mana -= victim->wait/4;
	send_to_char("Не получается.\n\r", ch);
	return;
    }

    ch->mana -= victim->wait/2;
    check_improve(ch, victim, gsn_attract_attention, TRUE, 2);
    WAIT_STATE(ch, skill_table[gsn_attract_attention].beats);
    victim->wait = 0;
    affect_strip(victim, skill_lookup("calm"));
    affect_strip(victim, skill_lookup("animal taming"));
    send_to_char("Получилось!\n\r", ch);
    return;
}


void do_helper(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d, *safe_d;

    if (IS_NPC(ch))
	return;

    if (argument[0] == '\0')
    {
	if (IS_SET(ch->comm, COMM_HELPER))
	{
	    send_to_char("Теперь ты не будешь слышать бесполезный крик новичков.\n\r", ch);
	    REMOVE_BIT(ch->comm, COMM_HELPER);
	}
	else
	{
	    if (IS_SET(ch->act, PLR_KILLER) || IS_SET(ch->act, PLR_THIEF))
	    {
		send_to_char("Неужели ты думаешь, что можешь подавать пример молодым?\n\r", ch);
		return;
	    }

	    if (ch->level >= (PK_RANGE)*2)
		send_to_char("Теперь ты сможешь помогать новичкам и отвечать на их вопросы.\n\r", ch);
	    else
		send_to_char("Теперь ты снова сможешь задавать свои вопросы.\n\r", ch);
	    SET_BIT(ch->comm, COMM_HELPER);
	}
    }
    else  /* gossip message sent, turn gossip on if it isn't already */
    {
	if (ch->level >= (PK_RANGE)*2 && !IS_SET(ch->comm, COMM_HELPER) && !IS_IMMORTAL(ch))
	{
	    send_to_char("Тебе не разрешено пользоваться этим каналом. Сначала набери команду 'помочь' без аргументов.\n\r", ch);
	    return;
	}

	if (IS_SET(ch->comm, COMM_QUIET))
	{
	    send_to_char("Для начала выключи режим 'тихо'.\n\r", ch);
	    return;
	}

	if (IS_SET(ch->comm, COMM_NOCHANNELS)) //  || get_age(ch) < 19 )
	{
	    send_to_char("Боги отобрали у тебя право пользования этим каналом.\n\r", ch);
	    return;
	}

//	SET_BIT(ch->comm, COMM_HELPER);

	sprintf(buf, "Ты говоришь новичкам и помощникам: {B%s{x\n\r", argument);
	send_to_char(buf, ch);

	if (IS_SET(ch->comm, COMM_EXTRANOCHANNELS))
    	{
		SLIST_FOREACH_SAFE(d, &descriptor_list, link, safe_d)
		{
	    	CHAR_DATA *victim;

		    victim = CH(d);

		    if (d->connected == CON_PLAYING
			&& d->character->position != POS_SLEEPING
			&& d->character != ch
			&& IS_IMMORTAL(victim)
			&& !IS_SET(victim->comm, COMM_QUIET)
			&& !check_filter(ch, victim))
		    {
			sprintf(buf, "%s говорит новичкам и помощникам: {B%s{x\n\r", ch->name, argument);
			send_to_char(buf, victim);
		    }
		}
    	}
	else
	{
		SLIST_FOREACH_SAFE(d, &descriptor_list, link, safe_d)
		{
		    CHAR_DATA *victim;
	
		    victim = CH(d);

		    if (d->connected == CON_PLAYING
			&& d->character->position != POS_SLEEPING
			&& d->character != ch
			&& (IS_SET(victim->comm, COMM_HELPER) || IS_IMMORTAL(victim))
			&& !IS_SET(victim->comm, COMM_QUIET)
			&& !check_filter(ch, victim))
		    {
			sprintf(buf, "%s говорит новичкам и помощникам: {B%s{x\n\r", ch->name, argument);
			send_to_char(buf, victim);
		    }
		}
	}
    }
}


void do_setip(CHAR_DATA *ch, char *argument)
{
    NAME_LIST *ip;
    char b[MSL], arg[MIL], *c;

    if (IS_NPC(ch))
	return;

    one_argument(argument, arg);

    if (arg[0] == '\0'
	|| !str_cmp(arg, "показ")
	|| !str_cmp(arg, "show")
	|| !str_cmp(arg, "список"))
    {
	int count = 0;
	BUFFER *bfr;

	if (!ch->pcdata->ips)
	    send_to_char("Ты играешь отовсюду.\n\r", ch);
	else
	{
	    bfr = new_buf();

	    add_buf(bfr, "Список разрешенных адресов:\n\r");

	    for (ip = ch->pcdata->ips; ip; ip = ip->next)
	    {
		sprintf(b, "%2d) %s\n\r",++count, ip->name);
		add_buf(bfr, b);
	    }

	    page_to_char(bfr->string, ch);
	    free_buf(bfr);

	}

	return;
    }

    if (!str_cmp(arg, "flush")
	|| !str_cmp(arg, "стереть")
	|| !str_cmp(arg, "удалить"))
    {
	NAME_LIST *ip_next;

	for (ip = ch->pcdata->ips; ip; ip = ip_next)
	{
	    ip_next = ip->next;
	    free_name_list(ip);
	}

	ch->pcdata->ips = NULL;

	send_to_char("Список адресов очищен.\n\r", ch);
	return;
    }

    for (c = arg; *c != '\0'; c++)
	if (*c != '.' && (*c < '0' || *c > '9'))
	{
	    send_to_char("Недопустимые символы в адресе.\n\r", ch);
	    return;
	}

    for (ip = ch->pcdata->ips; ip; ip = ip->next)
    {
	if (!IS_NULLSTR(ip->name) && !str_prefix(arg, ip->name))
	{
	    if (ip == ch->pcdata->ips)
		ch->pcdata->ips = ip->next;
	    else
	    {
		NAME_LIST *tmp;

		for(tmp = ch->pcdata->ips; tmp; tmp = tmp->next)
		    if (tmp->next == ip)
		    {
			tmp->next = ip->next;
			break;
		    }
	    }
	    sprintf(b, "Ты удаляешь адрес %s из списка разрешенных адресов.\n\r", ip->name);
	    send_to_char(b, ch);

	    free_name_list(ip);

	    return;
	}
    }

    ip = new_name_list();
    ip->name = str_dup(arg);
    ip->next = ch->pcdata->ips;
    ch->pcdata->ips = ip;

    sprintf(b, "Ты вносишь %s в список разрешенных адресов.\n\r", arg);
    send_to_char(b, ch);

    return;
}

void do_noexp(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
	return;

    if (ch->fighting || POS_FIGHT(ch))
    {
	send_to_char("Сначала закончи бой.\n\r", ch);
	return;
    }

    if (IS_SET(ch->act, PLR_NOEXP))
    {
	send_to_char("Теперь ты будешь получать опыт.\n\r", ch);
	REMOVE_BIT(ch->act, PLR_NOEXP);
    }
    else
    {
	send_to_char("Теперь ты не будешь получать опыт за убийства мобов.\n\r", ch);
	SET_BIT(ch->act, PLR_NOEXP);
    }
}

void do_reg_answer(CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    if (IS_NPC(ch))
	return;

    if (!IS_NULLSTR(ch->pcdata->reg_answer) && !IS_IMMORTAL(ch))
    {
	send_to_char("Ты уже ввел ответ на регистрационный вопрос.\n\r", ch);
	return;
    }

    if (IS_NULLSTR(argument))
    {
	send_to_char("Введите, пожалуйста, девичью фамилию вашей матери:\n\r", ch);
	send_to_char("Синтаксис: регвопрос <девичья фамилия вашей матери>. \n\r", ch);
	send_to_char("Например: регвопрос Иванова.\n\r", ch);
	if (IS_IMMORTAL(ch))
	{
	    send_to_char("Синтаксис: регвопрос clear <имя чара, которому надо очистить поле регистрационного вопроса.\n\r", ch);
	}
	return;
    }

    argument = one_argument(argument, arg1);

    if (!str_cmp(arg1, "clear") && IS_IMMORTAL(ch))
    {
	one_argument(argument, arg2);

	if (IS_NULLSTR(arg2))
	{
	    send_to_char("Введи имя чара, которому надо очистить поле регистрационного вопроса.\n\r", ch);
	    return;
	}

	if ((victim = get_char_world(ch, arg2)) == NULL
	    || (IS_NPC(victim)))
	{
	    send_to_char("Таких в мире нет.\n\r", ch);
	    return;
	}

	free_string(victim->pcdata->reg_answer);
	victim->pcdata->reg_answer = NULL;
	send_to_char("Поле его регистрационного вопроса очищено.\n\r", ch);
	return;
    }

    free_string(ch->pcdata->reg_answer);
    smash_tilde(arg1);
    ch->pcdata->reg_answer = str_dup(arg1);

    send_to_char("Ответ на регистрационный вопрос введен. Больше вы его изменить не сможете.\n\r", ch);
    return;
}



char *camomile[] =
{
    "Любит",
    "Не любит",
    "Плюнет",
    "Поцелует",
    "К сердцу прижмет",
    "К черту пошлет",
    NULL
};

void do_camomile(CHAR_DATA *ch, char *argument)
{
    int i;

    send_to_char("Ты срываешь ромашку, отрываешь лепестки, приговаривая:\n\r", ch);
    act("$n срывает ромашку, отрывает лепестки, приговаривая:\n\r", ch, NULL, NULL, TO_ROOM);

    for (i = 0; camomile[i] != NULL; i++)
	act("$t...", ch, camomile[i], NULL, TO_ALL);

    act("\n\r$t!", ch, camomile[number_range(0, i - 1)], NULL, TO_CHAR);
}

void do_temp_RIP(CHAR_DATA *ch, char *argument)
{
	if (IS_NPC(ch))
		return;

    if (IS_NULLSTR(argument))
    {
		if (!ch->pcdata->temp_RIP)
		{
			send_to_char("Ты не имеешь права на временное бессмертие. Тебе нечего желать.\n\r", ch);
			return;
		}

		if (ch->pcdata->temp_RIP > 2)
		{
			send_to_char("Ты имеешь право не на временное бессмертие, а на посмертие. Тебе нечего желать.\n\r", ch);
			return;
		}

		if (ch->pcdata->temp_RIP == 1)
		{
			send_to_char("Теперь ты желаешь временное бессмертие.\n\r", ch);
			ch->pcdata->temp_RIP = 2;
		}
		else
		{
			send_to_char("Теперь ты не желаешь временное бессмертие.\n\r", ch);
			ch->pcdata->temp_RIP = 1;
		}
	}
	else
	{
		if (!str_cmp(argument, "статус"))
		{
			switch (ch->pcdata->temp_RIP)
			{
				case 0 : send_to_char("У тебя нет права ни на временное бессмертие, ни на посмертие.\n\r", ch); break;
				case 1 : send_to_char("У тебя есть право на временное бессмертие, но сейчас ты не желаешь его.\n\r", ch); break;
				case 2 : send_to_char("У тебя есть право на временное бессмертие, и сейчас ты желаешь его.\n\r", ch); break;
				case 3 : send_to_char("У тебя есть право на посмертие.\n\r", ch);
			}
		}
		else
		{
			send_to_char("Синтаксис: бессмертие <статус>\n\r", ch);
		}
	}
				
	return;
}



/* charset=cp1251 */




