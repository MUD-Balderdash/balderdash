/***************************************************************************
 * 		Original Source Code by John Booth (EoD) 		   *
 * 	   Ported to Rom2.4b4 by Yago Diaz <yago@cerberus.uab.es>	   *
 *	       Bugfixes by Bogardan <jordi@cerberus.uab.es>		   *
 *		     (C) Last Modification February 1998		   *
 ***************************************************************************/
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
 **************************************************************************/
/***************************************************************************
*       ROM 2.4 is copyright 1993-1996 Russ Taylor                         *
*       ROM has been brought to you by the ROM consortium                  *
*           Russ Taylor (rtaylor@efn.org)                                  *
*           Gabrielle Taylor                                               *
*           Brian Moore (zump@rom.org)                                     *
*       By using this code, you have agreed to follow the terms of the     *
*       ROM license, in the file Rom24/doc/rom.license                     *
***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "interp.h"




bool mount_success(CHAR_DATA *ch, CHAR_DATA *mount, bool canattack, bool improve)
{
    int percent, success, skill;

    if (ch == NULL || mount == NULL)
	return FALSE;

    if (IS_NPC(ch))
	return TRUE;

    skill = get_skill(ch, gsn_riding);

    percent = number_percent() + (ch->level < mount->level 
				    ? (mount->level - ch->level) * 3 
				    : (mount->level - ch->level) * 2);

    if (!ch->fighting)
	percent -= 25;

    if (IS_DRUNK(ch))
    {
	percent += skill / 2;
	act("Поскольку ты пьян, езда верхом будет слегка посложнее, "
	    "чем ты думал$t...\n\r", ch, SEX_ENDING(ch), NULL, TO_CHAR);
    }

    success = percent - skill;

    if (success <= 0)
    {
	if (improve)
  	    check_improve(ch, NULL,gsn_riding, TRUE, 1);
	return TRUE;
    }
    else
    {
        if (improve)
  	    check_improve(ch, NULL,gsn_riding, FALSE, 1);

	if (success >= 10 && MOUNTED(ch) == mount)
	{
	    act("Ты теряешь контроль и падаешь со спины $N1.",
		ch, NULL, mount, TO_CHAR);
	    act("$n теряет контроль и падает со спины $N1.",
		ch, NULL, mount, TO_ROOM);
	    act("$n теряет контроль и падает с твоей спины.",
		ch, NULL, mount, TO_VICT);

	    ch->riding = FALSE;
	    mount->riding = FALSE;

	    if (ch->position > POS_STUNNED) 
		ch->position = POS_BASHED;
    
	    damage(ch, ch, number_range(1, 20), TYPE_UNDEFINED, DAM_BASH, FALSE, NULL);
	}

	if ( success >= 20 /* 40 */ && canattack)
	{
	    act("$N2 совершенно не нравится, как ты обходишься с н$t.",
		ch, mount->sex == SEX_FEMALE ? "ей" : "им", mount, TO_CHAR);
	    act("$N2 совершенно не нравиться, как $n обходится с н$t.",
		ch, mount->sex == SEX_FEMALE ? "ей" : "им", mount, TO_ROOM);
	    act("Тебе не нравится, как $n с тобой обошелся.",
		ch, NULL, mount, TO_VICT);

	    act("$N $t и атакует тебя!", ch, 
	          IS_SET(mount->parts, PART_CLAWS) ? "рычит" : "храпит", mount, TO_CHAR);
	    act("$N $t и атакует $n3!", ch, 
	          IS_SET(mount->parts, PART_CLAWS) ? "рычит" : "храпит", mount, TO_ROOM);
	    act("Ты $t и атакуешь $n3!", ch, 
	          IS_SET(mount->parts, PART_CLAWS) ? "рычишь": "храпишь", mount, TO_VICT);  

	    ch->riding = FALSE;
	    mount->riding = FALSE;

	    damage(mount, ch, number_range(1, mount->level),
		   gsn_kick, DAM_BASH, FALSE, NULL);

	}
    }

    return FALSE;
}

void do_mount(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *mount, *mnt;

    argument = one_argument(argument, arg);

    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->act, PLR_NOFOLLOW))
    {
	send_to_char("Ты не принимаешь последователей.\n\r", ch);
	return;
    }

    if (arg[0] == '\0' && ch->mount && ch->mount->in_room == ch->in_room)
    {
	mount = ch->mount;
    }
    else if (!(mount = get_char_room(ch, NULL, arg, FALSE)))
    {
	send_to_char("Оседлать кого?\n\r", ch);
	return;
    }
 
    if (get_skill(ch, gsn_riding) < 1)
    {
	send_to_char("Ты не знаешь, как это делается!\n\r", ch);
	return;
    } 

    if (max_count_charmed(ch))
        return;

    if (!IS_NPC(mount) || !IS_SET(mount->act, ACT_MOUNT)) 
    {
	act("$N3 нельзя оседлать.", ch, NULL, mount, TO_CHAR); 
	return;
    }
  
    if (mount->level > ch->level
       || check_immune(mount, DAM_CHARM) > 35)
    {
	act("$N слишком строптив$t для тебя.",
	    ch, SEX_ENDING(mount), mount, TO_CHAR);
	return;
    }    

    if (mount->size < ch->size)
    {
	act("$N маловат$t для тебя.", ch, SEX_ENDING(mount), mount, TO_CHAR);
	return;
    }

    if (RIDDEN(mount) && (mount->mount != ch))
    {
	act("$N уже оседлан$t.", ch, SEX_ENDING(mount), mount, TO_CHAR);
	return;
    }
    else if (MOUNTED(ch))
    {
	act("Ты уже едешь на $N5!", ch, NULL, MOUNTED(ch), TO_CHAR);
	return;
    }

    if (mount->fighting != NULL && mount != ch->mount)
    {
	act("Подожди, пока $N закончит бой.", ch, NULL, mount, TO_CHAR);
	return;
    }
    
    if (mount->position < POS_FIGHTING)
    {
	act("$N2 необходимо сначала встать на ноги.", ch, NULL, mount, TO_CHAR);
	return;
    }

    if (mount->mount != NULL && !mount->riding && mount->mount != ch)
    {
        if (number_percent() < get_skill(ch, gsn_steal_horse))
        {
            if (is_safe(ch, mount))
              return;

      	    act("{WТы пытаешься украсть $N3!{x", ch, NULL, mount, TO_CHAR);
	    check_improve(ch, mount, gsn_steal_horse, TRUE, 1);
        }
        else        
        {
	    sprintf(buf, "%s принадлежит %s, не тебе.\n\r",
		    capitalize(mount->short_descr), cases(mount->mount->name, 2));
	    send_to_char(buf, ch);
//	    check_improve(ch, mount, gsn_steal_horse, FALSE, 1);
	    return;
        }
    } 

    if (!mount_success(ch, mount, TRUE, TRUE))
    {
	act("У тебя не получается оседлать $N3.", ch, NULL, mount, TO_CHAR);  
	return; 
    }

    act("Ты запрыгиваешь на спину $N1.", ch, NULL, mount, TO_CHAR);
    act("$n запрыгивает на спину $N1.", ch, NULL, mount, TO_NOTVICT);
    act("$n запрыгивает на твою спину!", ch, NULL, mount, TO_VICT);
 
    if ((mnt = ch->mount) != NULL && mnt != mount)
	pet_gone(mnt);

    if (mount->mount != NULL)
	mount->mount->mount = NULL;

    ch->mount = mount;
    ch->riding = TRUE;
    mount->mount = ch;
    mount->riding = TRUE;

    if (mount->master == NULL || mount->master != ch)
    {
	mount->master = NULL;
	add_follower(mount, ch);
    }

    mount->master = ch;
    mount->leader = ch;

    do_visible(ch, "steal");
    
    mob_forget(ch, NULL, MEM_ANY);

}

void do_dismount( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *mount;
    bool update = TRUE;
     
    if (str_cmp(argument, "update"))
      update = FALSE;

    if (MOUNTED(ch))
    {
	mount = MOUNTED(ch);

	if (!update)
	{
	  act("Ты спешиваешься с $N1.", ch, NULL, mount, TO_CHAR);
	  act("$n спешивается с $N1.", ch, NULL, mount, TO_NOTVICT);
	  act("$n спешивается с тебя.", ch, NULL, mount, TO_VICT);
	}

	ch->riding = FALSE;
	mount->riding = FALSE;
    }
    else if (!update)
    {
	send_to_char("Ты, вроде, не верхом.\n\r", ch);
	ch->riding = FALSE;
	if (ch->mount != NULL)
	    ch->mount->riding = FALSE;
	return;
    }
}


void do_buy_mount( CHAR_DATA *ch, char *argument )
{
    int cost,roll;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char color[80], name[30], size[30];
    CHAR_DATA *mount;
    ROOM_INDEX_DATA *pRoomIndexNext;
    ROOM_INDEX_DATA *in_room;
    
    name[0] = '\0';
    size[0] = '\0';
    color[0] = '\0'; 

    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->act, PLR_NOFOLLOW))
    {
	send_to_char("Ты не принимаешь последователей.\n\r", ch);
	return;
    }

    if (get_skill(ch, gsn_riding) < 1)
    {
	send_to_char("Зачем ты собираешься купить лошадь, если ты даже "
		     "не знаешь, как ее оседлать?\n\r", ch);
        return;
    }

    argument = one_argument(argument, arg);

    pRoomIndexNext = get_room_index(ch->in_room->vnum + 1);

    if (pRoomIndexNext == NULL)
    {
	bugf("Do_buy: bad mount shop at vnum %d.", ch->in_room->vnum);
        send_to_char("Я боюсь, что моего загона для лошадей уже больше "
		     "не существует.\n\r", ch);
        return;
    }

    in_room     = ch->in_room;
    ch->in_room = pRoomIndexNext;
    mount       = get_char_room(ch, NULL, arg, FALSE);
    ch->in_room = in_room;

    if (mount == NULL || !IS_NPC(mount) || !IS_SET(mount->act, ACT_MOUNT))
    {
	send_to_char("Извини, здесь не продают такого.\n\r", ch);
	return;
    }

    if (ch->mount != NULL)
    {
        send_to_char("У тебя уже есть лошадь.\n\r",ch);
        return;
    }

    cost = 10 * mount->level * mount->level;

    if ((ch->silver + 100 * ch->gold) < cost)
    {
    	send_to_char( "Ты не можешь позволить себе купить это.\n\r", ch );
	return;
    }

    if (ch->level < mount->level)
    {
	send_to_char("Ты недостаточно опытен, чтобы управлять "
		     "этой лошадью.\n\r", ch );
	return;
    }

    if (mount->size < ch->size)
    {
	send_to_char("Эта лошадка маловата для тебя...\n\r", ch);
	return;
    }

    roll = number_percent();

    if (!IS_NPC(ch) && roll < get_skill(ch, gsn_haggle))
    {
        cost -= cost / 2 * roll / 100;
        sprintf(buf, "Ты торгуешься и снижаешь цену до %d монет.\n\r",
		cost);
        send_to_char(buf,ch);
        check_improve(ch, NULL, gsn_haggle, TRUE, 4);
    }

    deduct_cost(ch,cost);

    mount = create_mobile( mount->pIndexData );
        
    mount->comm = COMM_NOTELL | COMM_NOSHOUT | COMM_NOCHANNELS;
    SET_BIT(mount->act, ACT_NOEXP);
    mount->gold = 0;
    mount->silver = 0;

    if (ch->classid == CLASS_RANGER)
    {
	mount->hit += (mount->hit * 3 * ch->level / 200);
	mount->max_hit = mount->hit;
    }

    sprintf(buf, "%sИмя '%s' выжжено на задней ноге.\n\r",
	    mount->description, ch->name );
    free_string(mount->description);
    mount->description = str_dup(buf);

    char_to_room(mount, ch->in_room, FALSE);

    act("$n покупает $N3.", ch, NULL, mount, TO_ROOM);
    
    sprintf(buf, "Теперь у тебя есть %s.\n\r", mount->name);
    send_to_char(buf, ch);
    
    do_mount(ch, mount->name);

    return;
}

CHAR_DATA *mounted(CHAR_DATA *ch)
{
    CHAR_DATA *mount;

//    if (!(mount = ch->mount) || !ch->riding)
//	return NULL;

    if ((mount = ch->mount) != NULL
	&& ch->riding
	&& mount->leader == ch
	&& mount->master == ch
	&& mount->mount == ch)
    {
	return mount;
    }

    return NULL;
}

CHAR_DATA *ridden(CHAR_DATA *ch)
{
    if (IS_NPC(ch)
	&& ch->riding
	&& ch->mount
	&& ch->master == ch->mount)
    {
	return ch->mount;
    }

    return NULL;
}

/* charset=cp1251 */
