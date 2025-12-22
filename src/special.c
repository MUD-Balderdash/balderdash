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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "interp.h"
#include "magic.h"


/*
 * The following special functions are available for mobiles.
 */
DECLARE_SPEC_FUN( spec_breath_any	);
DECLARE_SPEC_FUN( spec_breath_acid	);
DECLARE_SPEC_FUN( spec_breath_fire	);
DECLARE_SPEC_FUN( spec_breath_frost	);
DECLARE_SPEC_FUN( spec_breath_gas	);
DECLARE_SPEC_FUN( spec_breath_lightning	);
DECLARE_SPEC_FUN( spec_cast_adept	);
DECLARE_SPEC_FUN( spec_cast_cleric	);
DECLARE_SPEC_FUN( spec_cast_mage	);
DECLARE_SPEC_FUN( spec_cast_undead	);
DECLARE_SPEC_FUN( spec_executioner	);
DECLARE_SPEC_FUN( spec_templeguard	);
DECLARE_SPEC_FUN( spec_guildguard	);
DECLARE_SPEC_FUN( spec_fido		);
DECLARE_SPEC_FUN( spec_guard		);
DECLARE_SPEC_FUN( spec_janitor		);
DECLARE_SPEC_FUN( spec_mayor		);
DECLARE_SPEC_FUN( spec_poison		);
DECLARE_SPEC_FUN( spec_thief		);
DECLARE_SPEC_FUN( spec_nasty		);
DECLARE_SPEC_FUN( spec_troll_member	);
DECLARE_SPEC_FUN( spec_ogre_member	);
DECLARE_SPEC_FUN( spec_patrolman	);
DECLARE_SPEC_FUN( spec_summoner		);
DECLARE_SPEC_FUN( spec_homunculus_mage	);
DECLARE_SPEC_FUN( spec_homunculus_cleric );
DECLARE_SPEC_FUN( spec_homunculus_damager );


/* the function table */
const   struct  spec_type    spec_table[] =
{
    {	"spec_breath_any",		spec_breath_any,	FALSE	},
    {	"spec_breath_acid",		spec_breath_acid,	FALSE	},
    {	"spec_breath_fire",		spec_breath_fire,	FALSE	},
    {	"spec_breath_frost",		spec_breath_frost,	FALSE	},
    {	"spec_breath_gas",		spec_breath_gas,	FALSE	},
    {	"spec_breath_lightning",	spec_breath_lightning,	FALSE	},	
    {	"spec_cast_adept",		spec_cast_adept,	FALSE	},
    {	"spec_cast_cleric",		spec_cast_cleric,	FALSE	},
    {	"spec_cast_mage",		spec_cast_mage,		FALSE	},
    {	"spec_cast_undead",		spec_cast_undead,	FALSE	},
    {	"spec_executioner",		spec_executioner,	TRUE	},
    {	"spec_templeguard",		spec_templeguard,	TRUE	},    
    {	"spec_guildguard",		spec_guildguard,	TRUE	},
    {	"spec_fido",			spec_fido,		TRUE	},
    {	"spec_guard",			spec_guard,		TRUE	},
    {	"spec_janitor",			spec_janitor,		TRUE	},
    {	"spec_mayor",			spec_mayor,		TRUE	},
    {	"spec_poison",			spec_poison,		FALSE	},
    {	"spec_thief",			spec_thief,		TRUE	},
    {	"spec_nasty",			spec_nasty,		TRUE	},
    {	"spec_troll_member",		spec_troll_member,	TRUE	},
    {	"spec_ogre_member",		spec_ogre_member,	TRUE	},
    {	"spec_patrolman",		spec_patrolman,		TRUE	},
    {   "spec_summoner",                spec_summoner,		FALSE   },
    {   "spec_homunculus_mage",         spec_homunculus_mage,	TRUE    },
    {   "spec_homunculus_cleric",       spec_homunculus_cleric,	TRUE    },
    {   "spec_homunculus_damager",      spec_homunculus_damager,TRUE    },
    {	NULL,				NULL,			FALSE	}
};


char *spec_name(SPEC_FUN *function)
{
    int i;

    for (i = 0; spec_table[i].function != NULL; i++)
    {
	if (function == spec_table[i].function)
	    return spec_table[i].name;
    }

    return NULL;
}

bool spec_homunculus_damager(CHAR_DATA *ch)
{
    CHAR_DATA *vch, *victim = NULL;
    int chance = 100000, i = 1, number;

    if (!IS_AWAKE(ch) || IS_AFFECTED(ch, AFF_CALM) || ch->in_room == NULL 
	|| !IS_AFFECTED(ch, AFF_CHARM) 
	|| ch->leader == NULL || ch->leader->fighting == NULL)
    {
	return FALSE;
    }
                                 
    LIST_FOREACH(vch, &ch->in_room->people, room_link)
    {
	if (ch == vch 
	|| !can_see(ch, vch)
	|| vch == ch->leader)
	    continue;

	if (vch->fighting == ch->leader)
	{	    
	    if (number_range(1,100000) <= chance)
	    {
		victim = vch;
		i++;
		chance /= i;
	    }
	}
    }

    if (victim == NULL)
	return FALSE;

    number = number_range(0,4);

    if (!IS_NPC(victim))
    {
	if (class_table[victim->classid].fMana == CLASS_MAGIC)
	{
	    if (number == 0)
		do_function(ch, &do_bash, victim->name);
	    if (number == 1)
		do_function(ch, &do_dirt, victim->name);
	}
	else
	{
	    if (number == 0)
		do_function(ch, &do_dirt, victim->name);
	    if (number == 1)
		do_function(ch, &do_disarm, victim->name);
	}
    }
    else
    {
	switch (number)
	{
	case (0): do_function(ch, &do_bash, victim->name); break;
	case (1): do_function(ch, &do_dirt, victim->name); break;
	case (2): do_function(ch, &do_disarm, victim->name); break;
	case (3): do_function(ch, &do_kick, victim->name); 
	}
    }

//    multi_hit(ch, ch->leader->fighting, TYPE_UNDEFINED);

    return TRUE;
}

bool spec_homunculus_mage(CHAR_DATA *ch)
{
    CHAR_DATA *victim = NULL;
    char *spell, buf[MIL];

    if (!IS_AWAKE(ch) || IS_AFFECTED(ch, AFF_CALM) || ch->in_room == NULL 
	|| !IS_AFFECTED(ch, AFF_CHARM) || ch->leader == NULL)
    {
	return FALSE;
    }

    if (number_percent() > 20 + ch->level)
	return FALSE;

    if (ch->leader->fighting)
	victim = ch->leader->fighting;

    if (victim == NULL)
	return FALSE;

    for (;;)
    {
	int min_level;

	switch (number_range(0, 12))
	{
	case  0: min_level =  0; spell = "curse";          break;
	case  1: min_level =  3; spell = "weaken";         break;
	case  2: min_level =  9; spell = "blindness";      break;
	case  3: min_level = 15; spell = "energy drain";   break;
	case  4: min_level = 18; spell = "harm";           break;
	case  5: min_level = 20; spell = "plague";	   break;
	case  6: min_level =  1; spell = "faerie fire";	   break;
	case  7: min_level = 16; spell = "fireball";	   break;
	case  8: min_level = 11; spell = "colour spray";   break;
	case  9: min_level =  1; spell = "magic missile";  break;
	default: min_level = 18; spell = "acid blast";     break;
	}

	if (ch->level >= min_level)
	    break;
    }

    if (ch->level > 30 && is_affected(victim, gsn_sanctuary))
	spell = "dispel magic";

    if (skill_lookup(spell) < 1 || is_affected(victim, skill_lookup(spell)))
	return FALSE;

    sprintf(buf,"'%s' %s",spell,victim->name);
   
    do_function(ch, &do_cast, buf);

    return TRUE;
}


struct cast_data
{
    char *spell;
    char *aspell;
};


struct cast_data cast_cleric[] =
{
    { "sleep",     "cancellation" },
    { "blindness", "cure blindness" },
    { "curse",     "cure curse" },
    { "poison",    "cure poison" },
    { "plague",    "cure plague" },
    { NULL, NULL }
};

bool spec_homunculus_cleric(CHAR_DATA *ch)
{
    CHAR_DATA *victim = NULL;    
    AFFECT_DATA *paf;
    char *spell = "", buf[MIL];
    int i;

    if (!IS_AWAKE(ch) || IS_AFFECTED(ch, AFF_CALM) || ch->in_room == NULL 
	|| !IS_AFFECTED(ch, AFF_CHARM) || ch->leader == NULL)
    {
	return FALSE;
    }

    if (number_percent() > 20 + ch->level)
	return FALSE;

    victim = ch->leader;

    for (paf = victim->affected; paf != NULL; paf = paf->next)
    {		
	for (i = 0; cast_cleric[i].spell != NULL ; i++)
	{
	    if (paf->type == skill_lookup(cast_cleric[i].spell))	
		spell = cast_cleric[i].aspell;
	}
    }

    if (victim->hit <= victim->max_hit / 2 && number_bits(2) == 1)
	spell = ( ch->level < 10 ? "лечить легкие раны" 
		: ch->level < 20 ? "лечить серьезные раны"
		: ch->level < 30 ? "лечить критические раны" : "heal");

    if (skill_lookup(spell) < 1)
	return FALSE;

    sprintf(buf,"'%s' %s",spell,victim->name);

    do_function(ch, &do_cast, buf);

    return TRUE;
}

bool spec_summoner(CHAR_DATA *ch)
{
    if (ch->fighting != NULL)
	return spec_cast_mage(ch);
    return FALSE;
}

bool spec_troll_member(CHAR_DATA *ch)
{
    CHAR_DATA *vch, *victim = NULL;
    int count = 0;
    char *message;

    if (!IS_AWAKE(ch) || IS_AFFECTED(ch, AFF_CALM) || ch->in_room == NULL 
	|| IS_AFFECTED(ch, AFF_CHARM) || ch->fighting != NULL)
    {
	return FALSE;
    }

    /* find an ogre to beat up */
    LIST_FOREACH(vch, &ch->in_room->people, room_link)
    {
	if (!IS_NPC(vch) || ch == vch)
	    continue;

	if (vch->pIndexData->vnum == MOB_VNUM_PATROLMAN)
	    return FALSE;

	if (vch->pIndexData->group == GROUP_VNUM_OGRES
	    && ch->level > vch->level - 2 && !is_safe(ch, vch))
	{
	    if (number_range(0, count) == 0)
		victim = vch;

	    count++;
	}
    }

    if (victim == NULL)
	return FALSE;

    /* say something, then raise hell */
    switch (number_range(0, 6))
    {
    case 0:
	message = "$n вопит: {MЯ нашел тебя, подонок!{x";
	break;
    case 1:
	message = "$n с яростным воплем атакует $N3.";
	break;
    case 2:
	message = "$n говорит тебе: {RЧто такой гнусный урод, как ты, делает здесь?{x";
	break;
    case 3:
	message = "$n щелкает пальцами и произносит: {CТы чувствуешь себя счастливым?{x";
	break;
    case 4:
	message = "$n произносит: {CЗдесь нет никого, кто бы мог спасти тебя на этот раз!{x";
	break;	
    case 5:
	message = "$n произносит: {CПришло время присоединится тебе к своей сестре, картошке.{x";
	break;
    case 6:
	message = "$n произносит: {CПора за дело.{x";
	break;
    default:
	message = NULL;
	break;
    }

    if (message != NULL)
	act(message, ch, NULL, victim, TO_ALL);
    multi_hit(ch, victim, TYPE_UNDEFINED);
    return TRUE;
}

bool spec_ogre_member(CHAR_DATA *ch)
{
    CHAR_DATA *vch, *victim = NULL;
    int count = 0;
    char *message;

    if (!IS_AWAKE(ch) || IS_AFFECTED(ch, AFF_CALM) || ch->in_room == NULL
	||  IS_AFFECTED(ch, AFF_CHARM) || ch->fighting != NULL)
	return FALSE;

    /* find an troll to beat up */
    LIST_FOREACH(vch, &ch->in_room->people, room_link)
    {
	if (!IS_NPC(vch) || ch == vch)
	    continue;

	if (vch->pIndexData->vnum == MOB_VNUM_PATROLMAN)
	    return FALSE;

	if (vch->pIndexData->group == GROUP_VNUM_TROLLS
	    &&  ch->level > vch->level - 2 && !is_safe(ch, vch))
	{
	    if (number_range(0, count) == 0)
		victim = vch;

	    count++;
	}
    }

    if (victim == NULL)
	return FALSE;

    /* say something, then raise hell */
    switch (number_range(0, 6))
    {
    default: message = NULL;	break;
    case 0:	message = "$n вопит: {MЯ нашел тебя, подонок!{x";
		break;
    case 1: message = "$n с яростным воплем атакует $N3.";
	    break;
    case 2: message = 
	    "$n говорит тебе: {RЧто такой урод, как ты, делает здесь?{x";
	    break;
    case 3: message = "$n щелкает пальцами и произносит: {CТы чувствуешь себя счастливым?{x";
	    break;
    case 4: message = "$n произносит: {CЗдесь нет никого, кто бы мог спасти тебя на этот раз!{x";
	    break;	
    case 5: message = "$n произносит: {CПришло время присоединится тебе к своей сестре, картошке.{x";
	    break;
    case 6: message = "$n произносит: {CПора за дело.{x";
	    break;

    }

    if (message != NULL)
	act(message, ch, NULL, victim, TO_ALL);
    multi_hit(ch, victim, TYPE_UNDEFINED);
    return TRUE;
}

bool spec_patrolman(CHAR_DATA *ch)
{
    CHAR_DATA *vch, *safe_vch, *victim = NULL;
    OBJ_DATA *obj;
    char *message;
    int count = 0;

    if (!IS_AWAKE(ch) || IS_AFFECTED(ch, AFF_CALM) || ch->in_room == NULL
	||  IS_AFFECTED(ch, AFF_CHARM) || ch->fighting != NULL)
	return FALSE;

    /* look for a fight in the room */
    LIST_FOREACH(vch, &ch->in_room->people, room_link)
    {
	if (vch == ch)
	    continue;

	if (vch->fighting != NULL)  /* break it up! */
	{
	    if (number_range(0, count) == 0)
		victim = (vch->level > vch->fighting->level) 
		    ? vch : vch->fighting;
	    count++;
	}
    }

    if (victim == NULL || (IS_NPC(victim) && victim->spec_fun == ch->spec_fun))
	return FALSE;

    if (((obj = get_eq_char(ch, WEAR_NECK_1)) != NULL 
	 && obj->pIndexData->vnum == OBJ_VNUM_WHISTLE)
	|| ((obj = get_eq_char(ch, WEAR_NECK_2)) != NULL
	    && obj->pIndexData->vnum == OBJ_VNUM_WHISTLE))
    {
	act("Ты тяжело ударяешь по $p2.", ch, obj, NULL, TO_CHAR);
	act("$n бьет по $p2, ***WHEEEEEEEEEEEET***", ch, obj, NULL, TO_ROOM);

	LIST_FOREACH_SAFE(vch, &char_list, link, safe_vch)
	{
	    if (vch->in_room == NULL)
		continue;

	    if (vch->in_room != ch->in_room 
		&& vch->in_room->area == ch->in_room->area)
		send_to_char("Ты слышишь пронзительный свист.\n\r", vch);
	}

    }

    switch (number_range(0, 6))
    {
    default:	message = NULL;		break;
    case 0:	message = "$n вопит: {MВнимание всем! Внимание всем! Прекратить!{x";
		break;
    case 1: message = 
	    "$n произносит: {CОбщество порочно, но что я могу сделать?{x";
	    break;
    case 2: message = 
	    "$n бормочет: {cдети крови будут нашей смертью.{x";
	    break;
    case 3: message = "$n атакует с воплем: {MПрекратить это!{x";
	    break;
    case 4: message = "$n сует за пояс свою дубинку и идет работать.";
	    break;
    case 5: message = 
	    "$n тоскует по своей отставке и продолжает разнимать дерущихся.";
	    break;
    case 6: message = "$n произносит: {CПрисядь, отдохни, хулиган!{x";
	    break;
    }

    if (message != NULL)
	act(message, ch, NULL, NULL, TO_ALL);

    multi_hit(ch, victim, TYPE_UNDEFINED);

    return TRUE;
}


bool spec_nasty(CHAR_DATA *ch)
{
    CHAR_DATA *victim, *v_next;
    long gold;

    if (!IS_AWAKE(ch)) {
	return FALSE;
    }

    if (!POS_FIGHT(ch)) 
    {
	LIST_FOREACH_SAFE(victim, &ch->in_room->people, room_link, v_next)
	{
	    if (!IS_NPC(victim)
		&& (victim->level > ch->level)
		&& (victim->level < ch->level + 10))
	    {
		do_function(ch, &do_backstab, victim->name);
		if (!POS_FIGHT(ch))
		{
		    do_function(ch, &do_kill, victim->name);
		}

		/* should steal some coins right away? :) */
		return TRUE;
	    }
	}
	return FALSE;    /*  No one to attack */
    }

    /* okay, we must be fighting.... steal some coins and flee */
    if ((victim = ch->fighting) == NULL)
	return FALSE;   /* let's be paranoid.... */

    switch (number_bits(2))
    {
    case 0:  act("$n разрезает твой кошелек, и монеты высыпаются $x!",
		 ch, NULL, victim, TO_VICT);
	     act("Ты разрезаешь кошелек $N1 и собираешь упавшие монеты!",
		 ch, NULL, victim, TO_CHAR);
	     act("Кошелек $N1 оказывается разрезанным!",
		 ch, NULL, victim, TO_NOTVICT);
	     gold = victim->gold / 10;  /* steal 10% of his gold */
	     victim->gold -= gold;
	     ch->gold     += gold;
	     return TRUE;

    case 1:  do_function(ch, &do_flee, "");
	     return TRUE;

    default: return FALSE;
    }
}

/*
 * Core procedure for dragons.
 */
bool dragon(CHAR_DATA *ch, char *spell_name)
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    int sn;

    if (!POS_FIGHT(ch))
	return FALSE;

    LIST_FOREACH_SAFE(victim, &ch->in_room->people, room_link, v_next)
    {
	if (victim->fighting == ch && number_bits(3) == 0)
	    break;
    }

    if (victim == NULL)
	return FALSE;

    if ((sn = skill_lookup(spell_name)) < 0 || is_affected(victim,gsn_protection_sphere))
	return FALSE;
    (*skill_table[sn].spell_fun) (sn, ch->level, ch, victim, TARGET_CHAR);
    return TRUE;
}



/*
 * Special procedures for mobiles.
 */
bool spec_breath_any(CHAR_DATA *ch)
{
    if (!POS_FIGHT(ch))
	return FALSE;

    switch (number_bits(3))
    {
    case 0: return spec_breath_fire		(ch);
    case 1:
    case 2: return spec_breath_lightning	(ch);
    case 3: return spec_breath_gas		(ch);
    case 4: return spec_breath_acid		(ch);
    case 5:
    case 6:
    case 7: return spec_breath_frost		(ch);
    }

    return FALSE;
}



bool spec_breath_acid(CHAR_DATA *ch)
{
    return dragon(ch, "acid breath");
}



bool spec_breath_fire(CHAR_DATA *ch)
{
    return dragon(ch, "fire breath");
}



bool spec_breath_frost(CHAR_DATA *ch)
{
    return dragon(ch, "frost breath");
}



bool spec_breath_gas(CHAR_DATA *ch)
{
    int sn;

    if (!POS_FIGHT(ch))
	return FALSE;

    if ((sn = skill_lookup("gas breath")) < 0)
	return FALSE;
    (*skill_table[sn].spell_fun) (sn, ch->level, ch, NULL, TARGET_CHAR);
    return TRUE;
}



bool spec_breath_lightning(CHAR_DATA *ch)
{
    return dragon(ch, "lightning breath");
}



bool spec_cast_adept(CHAR_DATA *ch)
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;

    if (!IS_AWAKE(ch))
	return FALSE;

    LIST_FOREACH_SAFE(victim, &ch->in_room->people, room_link, v_next)
    {
	if (victim != ch && !IS_NPC(victim) && victim->level <= MAX_RECALL_LEVEL 
	    && can_see(ch, victim) && number_bits(UMIN(1, victim->level/5)) == 0)
	    break;
    }

    if (victim == NULL)
	return FALSE;

    switch (number_bits(4))
    {
    case 0:
	if (ch->race == RACE_VAMPIRE && IS_VAMPIRE(victim))
	{
	    act("$n восклицает 'сра хунсоз'.", ch, NULL, NULL, TO_ROOM);
	    spell_shadow_cloak(skill_lookup("shadow cloak"), ch->level, ch, victim, TARGET_CHAR);
	}
	else
	{
	    act("$n восклицает 'абразак'.", ch, NULL, NULL, TO_ROOM);
	    spell_armor(skill_lookup("armor"), ch->level, ch, victim, TARGET_CHAR);
	}

	return TRUE;

    case 1:
	if (!IS_VAMPIRE(victim))
	{
	    act("$n восклицает 'фидо'.", ch, NULL, NULL, TO_ROOM);
	    spell_bless(skill_lookup("bless"), ch->level, ch, victim, TARGET_CHAR);
	    return TRUE;
	}
	break;
    case 2:
	act("$n восклицает 'джудикандус носелакри'.", ch, NULL, NULL, TO_ROOM);
	spell_cure_blindness(skill_lookup("cure blindness"),
			     ch->level, ch, victim, TARGET_CHAR);
	return TRUE;

    case 3:
	act("$n восклицает 'джудикандус диес'.", ch, NULL, NULL, TO_ROOM);
	spell_cure_light(skill_lookup("cure light"),
			 ch->level, ch, victim, TARGET_CHAR);
	return TRUE;

    case 4:
	act("$n восклицает 'джудикандус саусабру'.", ch, NULL, NULL, TO_ROOM);
	spell_cure_poison(skill_lookup("cure poison"),
			  ch->level, ch, victim, TARGET_CHAR);
	return TRUE;

    case 5:
	act("$n восклицает 'кандусима'.", ch, NULL, NULL, TO_ROOM);
	spell_refresh(skill_lookup("refresh"), ch->level, ch, victim, TARGET_CHAR);
	return TRUE;

    case 6:
	act("$n восклицает 'джудикандус еугзагс'.", ch, NULL, NULL, TO_ROOM);
	spell_cure_disease(skill_lookup("cure disease"),
			   ch->level, ch, victim, TARGET_CHAR);
	return TRUE;
    case 7:
	act("$n восклицает 'энерджайзер'.", ch, NULL, NULL, TO_ROOM);
	if (victim->mana < victim->max_mana)
	{
	    victim->mana += dice(2, 5) + ch->level / 5;
	    victim->mana = UMIN(victim->mana, victim->max_mana);
	    send_to_char("Тебя пронизывает тепло.\n\r", victim);
	}
	return TRUE;
    case 8:
	if (ch->race == RACE_VAMPIRE && IS_VAMPIRE(victim))
	{
	    act("$n восклицает 'кауха ах гззха'.", ch, NULL, NULL, TO_ROOM);
	    spell_protection_light(skill_lookup("protection from light"),
				   ch->level, ch, victim, TARGET_CHAR);
	    return TRUE;
	}
	break;
    case 9:
	if (ch->race == RACE_VAMPIRE && IS_VAMPIRE(victim))
	{
	    act("$n восклицает 'кауха ах еабгру'.", ch, NULL, NULL, TO_ROOM);
	    spell_protection_good(skill_lookup("protection good"),
				  ch->level, ch, victim, TARGET_CHAR);
	    return TRUE;
	}
	break;
    }

    return FALSE;
}



bool spec_cast_cleric(CHAR_DATA *ch)
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    char *spell;
    int sn;

    if (!POS_FIGHT(ch))
	return FALSE;

    LIST_FOREACH_SAFE(victim, &ch->in_room->people, room_link, v_next)
    {
	if (victim->fighting == ch && number_bits(2) == 0)
	    break;
    }

    if (victim == NULL)
	return FALSE;

    for (;;)
    {
	int min_level;

	switch (number_bits(4))
	{
	case  0: min_level =  0; spell = "blindness";      break;
	case  1: min_level =  3; spell = "cause serious";  break;
	case  2: min_level =  7; spell = "earthquake";     break;
	case  3: min_level =  9; spell = "cause critical"; break;
	case  4: min_level = 10; spell = "dispel evil";    break;
	case  5: min_level = 12; spell = "curse";          break;
	case  6: min_level = 12; spell = "change sex";     break;
	case  7: min_level = 13; spell = "flamestrike";    break;
	case  8:
	case  9:
	case 10: min_level = 15; spell = "harm";           break;
	case 11: min_level = 15; spell = "plague";	   break;
	default: min_level = 16; spell = "dispel magic";   break;
	}

	/*
	 * Жрецы сами себя подлечивают... Чтоб жизнь медом не казалась.
	 * 50% вероятность
	 */
	if ((ch->hit <= ch->max_hit / 2) && (number_bits(2) == 1))
	{
	    min_level = 21;
	    spell = "heal";
	    victim = ch;
	}

	if (ch->level >= min_level)
	    break;
    }

    if ((sn = skill_lookup(spell)) < 0 || is_affected(victim,gsn_protection_sphere))
	return FALSE;
    (*skill_table[sn].spell_fun) (sn, ch->level, ch, victim, TARGET_CHAR);
    return TRUE;
}

bool spec_cast_mage(CHAR_DATA *ch)
{
    CHAR_DATA *victim;
    char *spell;
    int sn;

    if (!POS_FIGHT(ch))
	return FALSE;

    LIST_FOREACH(victim, &ch->in_room->people, room_link)
	if (victim->fighting == ch && number_bits(2) == 0)
	    break;

    if (victim == NULL)
	return FALSE;

    for (;;)
    {
	int min_level;

	switch (number_bits(4)){
		case  0: min_level =  0; spell = "blindness";      break;
		case  1: min_level =  3; spell = "chill touch";    break;
		case  2: min_level =  7; spell = "weaken";         break;
		case  3: min_level =  8; spell = "teleport";       break;
		case  4: min_level = 11; spell = "colour spray";   break;
		case  5: min_level = 12; spell = "change sex";     break;
		case  6: min_level = 13; spell = "energy drain";   break;
		case  7:
		case  8:
		case  9: min_level = 15; spell = "fireball";       break;
		case 10: min_level = 20; spell = "plague";	   break;
		default: min_level = 20; spell = "acid blast";     break;
	}

	if ((number_bits(2) == 2 && ch->level >= 18)
	    && (IS_AFFECTED(ch, AFF_BLIND)
		|| IS_AFFECTED(ch, AFF_CURSE)
		|| IS_AFFECTED(ch, AFF_SLOW)))
	{
	    min_level = 18;
	    spell = "cancellation";
	    victim = ch;
	}

	if (ch->level >= min_level)
	    break;
    }

    if ((sn = skill_lookup(spell)) < 0 || is_affected(victim,gsn_protection_sphere))
	return FALSE;
    (*skill_table[sn].spell_fun) (sn, ch->level, ch, victim, TARGET_CHAR);
    return TRUE;
}



bool spec_cast_undead(CHAR_DATA *ch)
{
    CHAR_DATA *victim;
    char *spell;
    int sn;

    if (!POS_FIGHT(ch))
	return FALSE;

    LIST_FOREACH(victim, &ch->in_room->people, room_link)
    {
	if (victim->fighting == ch && number_bits(2) == 0)
	    break;
    }

    if (victim == NULL)
	return FALSE;

    for (;;)
    {
	int min_level;

	switch (number_bits(4))
	{
	case  0: min_level =  0; spell = "curse";          break;
	case  1: min_level =  3; spell = "weaken";         break;
	case  2: min_level =  6; spell = "chill touch";    break;
	case  3: min_level =  9; spell = "blindness";      break;
	case  4: min_level = 12; spell = "poison";         break;
	case  5: min_level = 15; spell = "energy drain";   break;
	case  6: min_level = 18; spell = "harm";           break;
	case  7: min_level = 21; spell = "teleport";       break;
	case  8: min_level = 20; spell = "plague";	   break;
	default: min_level = 18; spell = "harm";           break;
	}

	if (ch->level >= min_level)
	    break;
    }

    if ((sn = skill_lookup(spell)) < 0 || is_affected(victim,gsn_protection_sphere))
	return FALSE;
    (*skill_table[sn].spell_fun) (sn, ch->level, ch, victim, TARGET_CHAR);
    return TRUE;
}


bool spec_executioner(CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    char *crime;

    if (is_animal(ch) || !IS_AWAKE(ch) || ch->fighting != NULL)
	return FALSE;

    crime = "";
    LIST_FOREACH(victim, &ch->in_room->people, room_link)
    {
	if (!can_see(ch, victim))
	    continue;

	if (!IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF)) 
	{
	    crime = "ВОР";
	    break;
	}

	if (!IS_NPC(victim) && IS_SET(victim->act, PLR_KILLER)) 
	{
	    crime = "УБИЙЦА";
	    break;
	}
    }

    if (victim == NULL)
	return FALSE;

    sprintf(buf, "%s - %s! Теперь ты в моих руках! Крови твоей жажду!!",
	    victim->name, crime);
    REMOVE_BIT(ch->comm, COMM_NOSHOUT);
    do_function(ch, &do_yell, buf);
    multi_hit(ch, victim, TYPE_UNDEFINED);
    return TRUE;
}

bool spec_templeguard(CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    char *crime;

    if (is_animal(ch) || !IS_AWAKE(ch)/* || ch->fighting != NULL*/)
	return FALSE;

    crime = "";
    LIST_FOREACH(victim, &ch->in_room->people, room_link)
    {
	if (!can_see(ch, victim))
	    continue;

	if (!IS_NPC(victim) && victim->count_holy_attacks > 0) 
	{
	    crime = "Еретик";
	    break;
	}
/*
	if (!IS_NPC(victim) && victim->count_guild_attacks > 0) 
	{
	    crime = "Братоубийца";
	    break;
	}
*/
    }

    if (victim == NULL)
	return FALSE;

    sprintf(buf, "%s - %s! Как ты посмел%s войти в храм?!",
	    victim->name, crime, SEX_ENDING(victim));
    REMOVE_BIT(ch->comm, COMM_NOSHOUT);
    do_function(ch, &do_say, buf);
    multi_hit(ch, victim, TYPE_UNDEFINED);
    return TRUE;
}

bool spec_guildguard(CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    char *crime;

    if (is_animal(ch) || !IS_AWAKE(ch)/* || ch->fighting != NULL*/)
	return FALSE;

    crime = "";
    LIST_FOREACH(victim, &ch->in_room->people, room_link)
    {
	if (!can_see(ch, victim))
	    continue;

	if (!IS_NPC(victim) && victim->count_guild_attacks > 0) 
	{
	    crime = "Братоубийца";
	    break;
	}
/*    
	if (!IS_NPC(victim) && victim->count_holy_attacks > 0) 
	{
	    crime = "Еретик";
	    break;
	}
 */
    }

    if (victim == NULL)
	return FALSE;

    sprintf(buf, "%s - %s! Как ты посмел%s прийти в гильдию?!",
	    victim->name, crime, SEX_ENDING(victim));
    REMOVE_BIT(ch->comm, COMM_NOSHOUT);
    do_function(ch, &do_say, buf);
    multi_hit(ch, victim, TYPE_UNDEFINED);
    return TRUE;
}




bool spec_fido(CHAR_DATA *ch)
{
    OBJ_DATA *corpse;
    OBJ_DATA *c_next;

    if (!IS_AWAKE(ch) || !ch->in_room)
	return FALSE;

    for (corpse = ch->in_room->contents; corpse != NULL; corpse = c_next)
    {
	c_next = corpse->next_content;
	if (corpse->item_type != ITEM_CORPSE_NPC)
	    continue;

	act("$n жадно пожирает труп.", ch, NULL, NULL, TO_ROOM);
	dump_container(corpse);
	return TRUE;
    }

    return FALSE;
}



bool spec_guard(CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *ech;
    char *crime;
    int max_evil;

    if (is_animal(ch) || !IS_AWAKE(ch) || ch->fighting != NULL)
	return FALSE;

    max_evil = 300;
    ech      = NULL;
    crime    = "";

    LIST_FOREACH(victim, &ch->in_room->people, room_link)
    {
	if (can_see(ch, victim))
	{
	    if (!IS_NPC(victim) && IS_SET(victim->act, PLR_KILLER))
	    {
		crime = "УБИЙЦА";
		break;
	    }

	    if (!IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF)) 
	    {
		crime = "ВОР";
		break;
	    }
	}

	if (!IS_AFFECTED(ch, AFF_BLIND)
	    && victim->fighting != NULL
	    && victim->fighting != ch
	    && victim->alignment < max_evil 
	    && victim->spec_fun != spec_lookup("spec_guard"))
	{
	    max_evil = victim->alignment;
	    ech      = victim;
	}
    }

    if (victim != NULL && SUPPRESS_OUTPUT(!is_safe(ch, victim)))
    {
	sprintf(buf, "%s - %s! Теперь ты от меня не уйдешь!!",
		victim->name, crime);
	REMOVE_BIT(ch->comm, COMM_NOSHOUT);
	do_function(ch, &do_yell, buf);
	multi_hit(ch, victim, TYPE_UNDEFINED);
	return TRUE;
    }

    if (ech != NULL && SUPPRESS_OUTPUT(!is_safe(ch, ech)))
    {
	act("$n вопит: {MНА ЗАЩИТУ НЕВИННЫХ!!!{x", ch, NULL, NULL, TO_ROOM);
	multi_hit(ch, ech, TYPE_UNDEFINED);
	return TRUE;
    }

    return FALSE;
}


bool spec_janitor(CHAR_DATA *ch)
{
    OBJ_DATA *trash;
    OBJ_DATA *trash_next;

    if (!IS_AWAKE(ch))
	return FALSE;

    for (trash = ch->in_room->contents; trash != NULL; trash = trash_next)
    {
	trash_next = trash->next_content;

	if (!IS_SET(trash->wear_flags, ITEM_TAKE) || !can_loot(ch, trash))
	    continue;

	if ((trash->item_type == ITEM_DRINK_CON
	     || trash->item_type == ITEM_TRASH
	     || trash->cost < 100)
	    && count_users(trash) == 0
	    && SUPPRESS_OUTPUT(!is_have_limit(ch, ch, trash)))
	{
	    act("$n собирает кое-какой мусор.", ch, NULL, NULL, TO_ROOM);
	    obj_from_room(trash);
	    obj_to_char(trash, ch);
	    return TRUE;
	}
    }
    return FALSE;
}



bool spec_mayor(CHAR_DATA *ch)
{
    static const char open_path[] =
	"W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";

    static const char close_path[] =
	"W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

    static const char *path;
    static int pos;
    static bool move;

    if (!move)
    {
	if (time_info.hour ==  6)
	{
	    path = open_path;
	    move = TRUE;
	    pos  = 0;
	}

	if (time_info.hour == 20)
	{
	    path = close_path;
	    move = TRUE;
	    pos  = 0;
	}
    }

    if (ch->fighting != NULL)
	return spec_cast_mage(ch);
    if (!move || ch->position < POS_SLEEPING)
	return FALSE;

    switch (path[pos])
    {
    case '0':
    case '1':
    case '2':
    case '3':
	move_char(ch, path[pos] - '0', TRUE, FALSE);
	break;

    case 'W':
	ch->position = POS_STANDING;
	act("$n просыпается и широко зевает.", ch, NULL, NULL, TO_ROOM);
	break;

    case 'S':
	ch->position = POS_SLEEPING;
	act("$n ложится спать.", ch, NULL, NULL, TO_ROOM);
	break;

    case 'a':
	act("$n восклицает: {CПривет!{x", ch, NULL, NULL, TO_ROOM);
	break;

    case 'b':
	act("$n восклицает: {CКакой вид! Я должен это зарисовать!{x",
	    ch, NULL, NULL, TO_ROOM);
	break;

    case 'c':
	act("$n восклицает: {CВандалы! Юнцы ни к чему не имеют никакого уважения!{x",
	    ch, NULL, NULL, TO_ROOM);
	break;

    case 'd':
	act("$n восклицает: {CЗдравствуйте, граждане!{x", ch, NULL, NULL, TO_ROOM);
	break;

    case 'e':
	act("$n восклицает: {CЗа сим я объявляю Мидгард открытым!{x",
	    ch, NULL, NULL, TO_ROOM);
	break;

    case 'E':
	act("$n восклицает: {CЗа сим я объявляю Мидгард закрытым!{x",
	    ch, NULL, NULL, TO_ROOM);
	break;

    case 'O':
	/*	do_function(ch, &do_unlock, "gate"); */
	do_function(ch, &do_open_, "ворота");
	break;

    case 'C':
	do_function(ch, &do_close_, "ворота");
	/*	do_function(ch, &do_lock, "gate"); */
	break;

    case '.' :
	move = FALSE;
	break;
    }

    pos++;
    return FALSE;
}



bool spec_poison(CHAR_DATA *ch)
{
    CHAR_DATA *victim;

    if (!POS_FIGHT(ch)
	|| (victim = ch->fighting) == NULL
	|| number_percent() > 2 * ch->level)
    {
	return FALSE;
    }

    act("Ты кусаешь $N3!",  ch, NULL, victim, TO_CHAR);
    act("$n кусает $N3!",  ch, NULL, victim, TO_NOTVICT);
    act("$n кусает тебя!", ch, NULL, victim, TO_VICT);
    spell_poison(gsn_poison, ch->level, ch, victim, TARGET_CHAR);
    return TRUE;
}



bool spec_thief(CHAR_DATA *ch)
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    long gold, silver;

    if (ch->position != POS_STANDING || is_animal(ch))
	return FALSE;

    LIST_FOREACH_SAFE(victim, &ch->in_room->people, room_link, v_next)
    {
	if (IS_NPC(victim)
	    || victim->level >= LEVEL_IMMORTAL
	    || number_bits(5) != 0 
	    || !can_see(ch, victim)
	    || SUPPRESS_OUTPUT(is_safe(victim, ch)))
	{
	    continue;
	}

	if (IS_AWAKE(victim) && number_range(0, ch->level) == 0)
	{
	    act("Ты обнаруживаешь грязные лапы $n1 в своем кармане!",
		ch, NULL, victim, TO_VICT);
	    act("$N обнаруживает грязные лапы $n1 в своем кармане!",
		ch, NULL, victim, TO_NOTVICT);
	    return TRUE;
	}
	else
	{
	    gold = victim->gold * UMIN(number_range(1, 20), ch->level / 2) / 100;
	    gold = UMIN(gold, ch->level * ch->level * 10);
	    ch->gold     += gold;
	    victim->gold -= gold;
	    silver = victim->silver * UMIN(number_range(10, 40), ch->level / 2) / 100;
	    silver = UMIN(silver, ch->level*ch->level * 25);
	    ch->silver	+= silver;
	    victim->silver -= silver;
	    return TRUE;
	}
    }

    return FALSE;
}

/* OLC Inserted */
/*****************************************************************************
Name:		spec_string
Purpose:	Given a function, return the appropriate name.
Called by:	<???>
 ****************************************************************************/
char *spec_string(SPEC_FUN *fun)	/* OLC */
{
    int cmd;

    for (cmd = 0; spec_table[cmd].function != NULL; cmd++)
	if (fun == spec_table[cmd].function)
	    return spec_table[cmd].name;

    return 0;
}

/* charset=cp1251 */














