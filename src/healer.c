/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,	   *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *									   *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael	   *
 *  Chastain, Michael Quan, and Mitchell Tse.				   *
 *									   *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc	   *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.						   *
 *									   *
 *  Much time and thought has gone into this software and you are	   *
 *  benefitting.  We hope that you share your changes too.  What goes	   *
 *  around, comes around.						   *
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
#include "magic.h"



void do_heal(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *mob, *victim;
    char arg[MAX_INPUT_LENGTH],arg1[MAX_INPUT_LENGTH];
    int sn;
    unsigned int cost;
    SPELL_FUN *spell;
    char *words;	
    bool fMana = FALSE;

    /* check for healer */
    LIST_FOREACH(mob, &ch->in_room->people, room_link)
    {
        if (IS_NPC(mob) && !IS_SWITCHED(mob) && IS_SET(mob->act, ACT_IS_HEALER))
            break;
    }
 
    if (mob == NULL)
    {
        send_to_char("Ты не можешь этого здесь.\n\r", ch);
        return;
    }

	if ((ch->count_holy_attacks > 0)||(ch->count_guild_attacks > 0))
	{
		send_to_char("Еретики и Братоубийцы не могут лечиться в храмах.\n\r", ch);
		return;
	}			   
    
	argument=one_argument(argument,arg);

    if (arg[0] == '\0')
    {
        /* display price list */
	act("$N говорит тебе: {RЯ предлагаю следующие услуги:{x",ch,NULL,mob,TO_CHAR);
	send_to_char("  диагноз:     что нужно полечить      бесплатно\n\r",ch);
	send_to_char("  легкие:      лечить легкие раны      10 золота\n\r",ch);
	send_to_char("  серьезные:   лечить серьезные раны   15 золота\n\r",ch);
	send_to_char("  критические: лечить критические раны 25 золота\n\r",ch);
	send_to_char("  здоровье:    излечение               50 золота\n\r",ch);
	send_to_char("  слепоту:     лечить слепоту          20 золота\n\r",ch);
	send_to_char("  чуму:        лечить чуму             15 золота\n\r",ch);
	send_to_char("  яд:          лечить яд               25 золота\n\r",ch); 
	send_to_char("  проклятье:   снять проклятье         30 золота\n\r",ch);
	send_to_char("  шаги:        восстановить шаги       5 золота\n\r",ch);
	send_to_char("  энергию:     восстановить ману       Сколько дашь, на столько и вылечу\n\r",ch);
	send_to_char("  отмена:      отменяет заклинания     50 золота{x\n\r",ch);
	send_to_char(" Набери 'лечить <тип> [<кого лечить>] [<кол-во золота>]' для лечения.\n\r",ch);
	
        ch->reply = mob;
	return;
    }

    argument = one_argument(argument, arg1);

    if (arg1[0] == '\0')
    {
      victim = ch;
    }
    else if ((victim = get_char_room(ch, NULL, arg1, FALSE)) == NULL)
    {
      send_to_char("Таких здесь нет.\n\r", ch);
      return;
    }

    if (!str_prefix(arg,"diagnose") || !str_prefix(arg,"диагноз"))
    {
	char buf[MAX_STRING_LENGTH];

	buf[0] = '\0';
	act("$N внимательно изучает тело $n1.", ch, NULL, mob, TO_ROOM);
        act("$N внимательно изучает твое тело.", ch, NULL, mob, TO_CHAR);
        
        if (is_affected(ch, gsn_blindness))
          strcat(buf, "слепоту, ");
        if (is_affected(ch, gsn_curse))
          strcat(buf, "проклятье, ");
        if (is_affected(ch, gsn_poison))
          strcat(buf, "яд, ");
        if (is_affected(ch, gsn_plague))
          strcat(buf, "чуму, ");

        if (buf[0] != '\0')
          act("$N говорит тебе: {RТебе нужно полечить $t, и все будет нормально.{x",ch,buf,mob,TO_CHAR);
        else
          act("$N говорит тебе: {RУ тебя вроде ничего не болит.{x",ch,NULL,mob,TO_CHAR);

        buf[0] = '\0';
        if (ch->hit < ch->max_hit/3)
          strcat(buf, "здоровье, ");
        if (ch->mana < ch->max_mana/3)
          strcat(buf, "энергию, ");
        if (ch->move < ch->max_move/3)
          strcat(buf, "шаги, ");

        if (buf[0] != '\0')
            act("$N говорит тебе: {RЯ рекомендую полечить тебе $t, чтобы полностью восстановиться.{x",ch,buf,mob,TO_CHAR);

	ch->reply = mob;
	return;
    }

    if (!str_prefix(arg,"light") || !str_prefix(arg,"легкие"))
    {
        spell = spell_cure_light;
	sn    = skill_lookup("cure light");
	words = "джудикандус диес";
	cost  = 1000;
    }

    else if (!str_prefix(arg,"serious") || !str_prefix(arg,"серьезные"))
    {
	spell = spell_cure_serious;
	sn    = skill_lookup("cure serious");
	words = "джудикандус гзфуажг";
	cost  = 1600;
    }

    else if (!str_prefix(arg,"critical") || !str_prefix(arg,"критические"))
    {
	spell = spell_cure_critical;
	sn    = skill_lookup("cure critical");
	words = "джудикандус кфухукар";
	cost  = 2500;
    }

    else if (!str_prefix(arg,"heal") || !str_prefix(arg,"здоровье"))
    {
	spell = spell_heal;
	sn = skill_lookup("heal");
	words = "пзар";
	cost  = 5000;
    }

    else if (!str_prefix(arg,"blindness") || !str_prefix(arg,"слепоту"))
    {
	spell = spell_cure_blindness;
	sn    = skill_lookup("cure blindness");
      	words = "джудикандус носелакри";		
        cost  = 2000;
    }

    else if (!str_prefix(arg,"disease") || !str_prefix(arg,"болезнь") || !str_prefix(arg,"чуму"))
    {
	spell = spell_cure_disease;
	sn    = skill_lookup("cure disease");
	words = "джудикандус еугзагз";
	cost = 1500;
    }

    else if (!str_prefix(arg,"poison") || !str_prefix(arg,"яд"))
    {
	spell = spell_cure_poison;
	sn    = skill_lookup("cure poison");
	words = "джудикандус саусабру";
	cost  = 2500;
    }
	
    else if (!str_prefix(arg,"uncurse") || !str_prefix(arg,"curse") || !str_prefix(arg,"проклятье"))
    {
	spell = spell_remove_curse; 
	sn    = skill_lookup("remove curse");
	words = "кандуссидо джудигфз";
	cost  = 3000;
    }

    else if (!str_prefix(arg,"mana") || !str_prefix(arg,"energize")  || !str_prefix(arg,"энергию"))
    {
        spell = spell_restore_mana;
        sn = skill_lookup("restore mana");
        words = "энерджайзер";
        one_argument(argument, arg1);

        fMana = TRUE;

        if (arg1[0] == '\0' || !is_number(arg1))
           cost = 1000;
        else
        {
           cost = atoi(arg1) * 100;
           if (cost <= 0)
           {
	     act("$N говорит тебе: {RЭто что, шутка???{x", ch,NULL,mob,TO_CHAR);
             ch->reply = mob;
             return;
           }
        }
    }
    else if (!str_prefix(arg,"refresh") || !str_prefix(arg,"moves")  || !str_prefix(arg,"шаги"))
    {
	spell =  spell_refresh;
	sn    = skill_lookup("refresh");
	words = "кандусима"; 
	cost  = 500;
    }
    else if (!str_prefix(arg,"cancel") || !str_prefix(arg,"отмена"))
    {
        spell = spell_cancellation;
	sn    = gsn_cancellation;
	words = "cancel";
	cost  = 5000;
    }
    else 
    {
	act("$N говорит тебе: {RНабери 'лечить' для списка того, что я предлагаю.{x",
	    ch,NULL,mob,TO_CHAR);
        ch->reply = mob;
	return;
    }

    if (cost > (ch->gold * 100 + ch->silver) && !IS_IMMORTAL(ch))
    {
	act("$N говорит тебе: {RУ тебя не хватает денег на мои услуги.{x",
	    ch,NULL,mob,TO_CHAR);
        ch->reply = mob;
	return;
    }

    WAIT_STATE(ch,PULSE_VIOLENCE);

    act("$n восклицает '$T'.",mob,NULL,words,TO_ROOM);
  
    if (!spell(sn, fMana ? cost*(dice(2,8) + mob->level/3)/2000 : mob->level, mob, victim,TARGET_CHAR))
    {
	act("$N говорит тебе: {RИзвини, я не могу тебе в этом помочь.{x",
	    ch,NULL,mob,TO_CHAR);
        ch->reply = mob;
	return;
    }
    
    if (!IS_IMMORTAL(ch))
    {
      deduct_cost(ch,cost);
      mob->gold += cost / 100;
      mob->silver += cost % 100;
    }

}
/* charset=cp1251 */
