#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h" 



void do_balance(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
	return;

    if (ch->pcdata->bank > 0)
	sprintf(buf, "Ќа твоем счете в банке %ld золота.\n\r",
		ch->pcdata->bank);
    else
	strcpy(buf, "” теб€ нет денег в банке.\n\r");

    send_to_char(buf, ch);
    return;
}

void do_deposit(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *banker;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int amnt;

    if (IS_NPC(ch))
	return;

    if (!IS_SET(ch->in_room->room_flags, ROOM_BANK)) 
    {
	send_to_char("Ќо ты не в банке!\n\r", ch);
	return;
    }

    banker = NULL;
    LIST_FOREACH(banker, &ch->in_room->people, room_link)
    {
	if (IS_NPC(banker) && IS_SET(banker->pIndexData->act, ACT_BANKER))
	    break;
    }

    if (!banker)
    {
	send_to_char("Ѕанкир тут недоступен.\n\r", ch);
	return;
    }
 
    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("—колько золота ты хочешь положить на счет?\n\r", ch);
	return;
    }

    amnt = atoi(arg);

    if (amnt < 1)
    {
	send_to_char("ј головой подумать?\n\r", ch);
	return;
    }

    ch->reply = banker;

    if (amnt > ch->gold)
    {
	sprintf(buf, "%s говорит тебе: {R%s, у теб€ нет столько денег.{x\n\r",
		banker->short_descr, ch->name);
	send_to_char(buf, ch);
	return;
    }

    ch->pcdata->bank += amnt;
    ch->gold -= amnt;
    sprintf(buf,
	    "%s говорит тебе: {R%s, на твоем счете теперь %ld золота.{x\n\r",
	    banker->short_descr, ch->name, ch->pcdata->bank);
    send_to_char(buf, ch);
    return;
}

void do_withdraw (CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *banker;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int amnt, pr;

    if (IS_NPC(ch))
	return;

    if (!IS_SET(ch->in_room->room_flags, ROOM_BANK)) 
    {
    	send_to_char("Ќо ты не в банке!\n\r", ch);
	return;
    }

    banker = NULL;
    LIST_FOREACH(banker, &ch->in_room->people, room_link)
    {
    	if (IS_NPC(banker) && IS_SET(banker->pIndexData->act, ACT_BANKER))
	    break;
    }

    if (!banker)
    {
	send_to_char("Ѕанкир сейчас недоступен.\n\r", ch);
	return;
    }
 
    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("—колько ты хочешь сн€ть со своего счета?\n\r", ch);
	return;
    }

    amnt = atoi(arg);

    if (amnt < 2)
    {
	send_to_char("ј головой подумать?\n\r", ch);
	return;
    }
    
    if (amnt > ch->pcdata->bank) 
    {
        sprintf(buf, "%s говорит тебе: {R%s, у теб€ нет столько денег на "
		     "счете.{x\n\r",
		banker->short_descr, ch->name);
	send_to_char( buf, ch);
        ch->reply = banker;
	return;
    }

    pr = UMAX(1, amnt/10);
    
    if (!can_take_weight(ch, get_money_weight(amnt - pr, 0)))
    {
	send_to_char("“ебе не унести столько т€жести.\n\r", ch);
	return;
    }

    ch->gold += (amnt - pr);
    ch->pcdata->bank -= amnt;
    sprintf(buf, "%s говорит тебе: {R%s, на твоем счете теперь %ld золота. "
		 "«а свои услуги € вз€л себе %d золота.{x\n\r",
	    banker->short_descr, ch->name, ch->pcdata->bank, pr);
    send_to_char(buf, ch);
    ch->reply = banker;
    return;
}

/* charset=cp1251 */







