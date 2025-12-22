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
 *  ROM 2.4 is copyright 1993-1998 Russ Taylor				   *
 *  ROM has been brought to you by the ROM consortium			   *
 *      Russ Taylor (rtaylor@hypercube.org)				   *
 *      Gabrielle Taylor (gtaylor@hypercube.org)			   *
 *      Brian Moore (zump@rom.org)					   *
 *  By using this code, you have agreed to follow the terms of the	   *
 *  ROM license, in the file Rom24/doc/rom.license			   *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "interp.h"
#include "magic.h"
#include "recycle.h"


/*
 * Local functions.
 */
void  say_spell  args((CHAR_DATA *ch, int sn, bool check, bool success));

/* imported functions */
bool    remove_obj      args((CHAR_DATA *ch, int iWear, bool fReplace));
void   wear_obj  args((CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace));
CHAR_DATA *can_transport(CHAR_DATA *ch, int level);



struct cancel_data {
    char *spell;
    char *msg;
    int  hard;
};


struct cancel_data cancel_spell[] =
{
    { "armor", "", 1 },
    { "bless", "", 1},
    { "blindness", "$n теперь видит.{x", 1},
    { "calm", "$n больше не выглядит так мирно...{x", 1},
    { "change sex", "$n снова выглядит собой.{x", 1},
    { "charm person", "$n теперь ни от кого не зависит.{x", 1},
    { "charm animal", "$n теперь ни от кого не зависит.{x", 1},
    { "control undead", "$n теперь ни от кого не зависит.{x", 1},
    { "chill touch", "$n согревается.{x", 1},
    { "curse", "", 1},
    { "detect evil", "", 1},
    { "detect good", "", 1},
    { "detect invis", "", 1},
    { "detect hidden", "", 1},
    { "detect magic", "", 1},
    { "faerie fire", "Розовая аура вокруг $n1 тает.{x", 1},
    { "fly", "$n падает $x!{x", 1},
    { "frenzy", "$n не выглядит дикарем.{x", 1},
    { "giant strength", "$n слабеет.{x", 1},
    { "haste", "$n двигается медленнее.{x", 1},
    { "infravision", "", 1},
    { "invis", "$n проявляется.{x", 1},
    { "mass invis", "$n проявляется.{x", 1},
    { "pass door", "", 1},
    { "protection evil", "", 1},
    { "protection good", "", 1},
    { "sanctuary", "Белая аура вокруг $n1 тает.{x", 2},
    { "shield", "Защитный щит вокруг $n1 тает.{x", 1},
    { "sleep", "", 1},
    { "slow", "$n двигается быстрее.{x", 1},
    { "stone skin", "Кожа $n1 снова мягкая.{x", 1},
    { "weaken", "$n выглядит сильнее.{x", 1},
    { "forestshield", "$n уже не так защищен лесом.{x", 1},
    { "bark skin", "Кожа $n1 уже не такая прочная.{x", 1},
    { "power word fear", "$n перестает дрожать от страха.{x", 1},
    { "fire shield", "$n перестает окружаться аурой огня.{x", 1},
    { "ice shield", "$n перестает окружаться синеватой аурой.{x", 1},
    { "holy shield", "$n перестает окружаться аурой света.{x", 1},
    { "coniferous shield", "$n перестает окружаться зеленоватой аурой.{x", 1},
    { "shadow cloak", "$n перестает защищаться тенями.{x", 1},
    { "death aura", "Аура смерти больше не защищает $n3.{x", 1},
    { "mist", "$n прекращает трансформироваться в туманное облако.{x", 2},
/*    { "vaccine", "Действие вакцины в крови $n1 прекращается.{x", 2}, */
    { "st aura", "Боги прекращают защищать $n3.{x",1},
    { "bless forest", "Лес прекращает защищать $n3.{x",1},
    { "ghost aura", "Призрачная аура, окружающая $n3, испаряется.{x",1},
    { "coarse leather", "Кожа $n1 становится гладкой.{x", 1},
    { "hair", "Шерсть, покрывающая кожу $n1, исчезает.{x", 1},
    { "bear strong", "Сила медведя покидает $n3.{x", 1},
    { "dissolve", "$n1 проявляется, принимая свое нормальное состояние.{x", 1},
    { NULL, NULL, 0 }
};

bool OBJcast = FALSE;

// КОЛИЧЕСТВО ОЧАР
bool max_count_charmed(CHAR_DATA *ch)
{
    CHAR_DATA *gch;
    int i = 0;

    LIST_FOREACH(gch, &char_list, link){
	    if (gch->master == ch && IS_AFFECTED(gch, AFF_CHARM) && (!IS_NPC(gch) || gch->pIndexData->vnum != MOB_BONE_SHIELD)){
	        i++;
	    }
    }

    if ((i >= 1 && UMAX(get_curr_stat(ch, STAT_INT), get_curr_stat(ch, STAT_WIS))  < 16)
	|| (i >= 2 && UMAX(get_curr_stat(ch, STAT_INT), get_curr_stat(ch, STAT_WIS)) < 26)
	|| (i >= 3))
    {
	send_to_char("Справься сначала с остальными своими очарованными!\n\r",ch);
	return TRUE;
    }

    return FALSE;
}

bool self_magic(CHAR_DATA *ch, CHAR_DATA *victim)
{
    if (ch == victim 
	&& class_table[ch->classid].fMana == CLASS_MAGIC
	&& !IS_VAMPIRE(victim))
	return TRUE;
    else
	return FALSE;
}

/*
 * Убираем эффект, если по силе (уровню) заклинателя он меньше
 */

bool is_replace(int sn, int level, CHAR_DATA *victim)
{
	AFFECT_DATA *paf;
	if (((paf = affect_find(victim->affected, sn)) != NULL) && (level <= paf->level) && (paf->caster_id))
	    return FALSE;	
           
	return TRUE;
}


/*
 * Lookup a skill by name.
 */

int find_spell(CHAR_DATA *ch, const char *name)
{
    /* finds a spell the character can cast if possible */
    int sn, found = -1;

    if (IS_NPC(ch))
	return skill_lookup(name);

    for (sn = 0; sn < max_skills; sn++)
    {
	if (IS_NULLSTR(skill_table[sn].name))
	    break;

	if ((LOWER(name[0]) == LOWER(skill_table[sn].name[0])
	     && !str_prefix(name, skill_table[sn].name))
	    || (LOWER(name[0]) == LOWER(skill_table[sn].rname[0])
		&& !str_prefix(name, skill_table[sn].rname)))
	{
	    if (found == -1)
		found = sn;
	    if (ch->level >= skill_table[sn].skill_level[ch->classid]
		&& ch->pcdata->learned[sn] > 0)
	    {
		return sn;
	    }
	}
    }
    return found;
}



/*
 * Utter mystical words for an sn.
 */
void say_spell(CHAR_DATA *ch, int sn, bool check, bool success)
{
    char buf  [MAX_STRING_LENGTH];
    char buf2 [MAX_STRING_LENGTH];
    char tmp [MAX_STRING_LENGTH];
    CHAR_DATA *rch, *safe_rch;
    char *pName;
    int iSyl;
    int length, i;

    struct syl_type
    {
	char *  old;
	char *  new_type;
    };

    static const struct syl_type syl_table[] =
    {
	{ " ",    " "    },
	{ "ар",    "абра"    },
	{ "ау",    "када"    },
	{ "блесс",  "фидо"    },
	{ "блинд",  "носе"    },
	{ "бур",  "моса"    },
	{ "цу",    "джуди"    },
	{ "де",    "окуло"    },
	{ "ен",    "унсо"    },
	{ "лайт",  "диес"    },
	{ "ло",    "хай"    },
	{ "мор",  "зак"    },
	{ "мув",  "сидо"    },
	{ "несс",  "лакри"    },
	{ "нинг",  "илла"    },
	{ "пер",  "дуда"    },
	{ "ра",    "гру"    },
	{ "фреш",  "има"    },
	{ "ре",    "кандус"  },
	{ "сон",  "сабру"    },
	{ "тект",  "инфра"    },
	{ "три",  "цула"    },
	{ "вен",  "нофо"    },
	{ "а",    "а"    },
	{ "б",    "б"    },
	{ "ц",    "ку"    },
	{ "д",    "е"    },
	{ "е",    "з"    },
	{ "ф",    "и"    },
	{ "г",    "о"    },
	{ "х",    "п"    },
	{ "и",    "у"    },
	{ "ж",    "и"    },
	{ "к",    "т"    },
	{ "л",    "р"    },
	{ "м",    "вв"    },
	{ "н",    "и"    },
	{ "о",    "а"    },
	{ "п",    "с"    },
	{ "ку",    "д"    },
	{ "р",    "ф"    },
	{ "с",    "г"    },
	{ "т",    "х"    },
	{ "у",    "ж"    },
	{ "в",    "з"    },
	{ "вв",    "кс"    },
	{ "кс",    "н"    },
	{ "и",    "л"    },
	{ "з",    "к"    },
	{ "",    ""    }
    };

    buf[0]  = '\0';
    for (pName = skill_table[sn].rname; *pName != '\0'; pName += length)
    {
	for (iSyl = 0; (length = strlen(syl_table[iSyl].old)) != 0; iSyl++)
	{
	    if (!str_prefix(syl_table[iSyl].old, pName))
	    {
		strcat(buf, syl_table[iSyl].new_type);
		break;
	    }
	}

	if (length == 0)
	    length = 1;
    }

    if (!success)
    {
	length = strlen(buf);
	iSyl = UMAX(1, number_range(1, length - 1));
	for (i = iSyl; i < length; i++)
	    buf[i] = '.';
    }

    sprintf(buf2, "$n восклицает '%s'.{x", buf);

    strcpy(tmp, skill_table[sn].name);

    if (!success)
    {
	length = strlen(tmp);
	iSyl = UMAX(1, number_range(1, length - 1));
	for (i = iSyl; i < length; i++)
	    tmp[i] = '.';
    }

    sprintf(buf,  "$n восклицает '%s'.{x", tmp);

    if (check)
	act(buf2, ch, NULL, NULL, TO_CHAR);
    else
	LIST_FOREACH_SAFE(rch, &ch->in_room->people, room_link, safe_rch)
	{
	    act((!IS_NPC(rch) && ch->classid == rch->classid) ? buf : buf2,
		ch, NULL, rch, TO_VICT);
	}
    return;
}



/* * *
 * ЗАЩИТА ОТ МАГИИ ЧЕРЕЗ СИЛУ ВОЛИ (SAVES)
 *  tag:    #saves #сила воли
 *  last:   brullek@gmail.com   2018-10-29
 * * */
bool saves_spell(int level, CHAR_DATA *victim, int dam_type){
    int saves;
    int res;

    if ((res = check_immune(victim, dam_type)) > 99) return TRUE;

	if (IS_NPC(victim)){
		/* В мире сейчас у мобов болье -130 почти не встречаются савы, что бы не менять - удвоим мобам их савы, быстро и эффективно */
        saves = -2*victim->saving_throw;
	} else {
        saves = -1*victim->saving_throw;
	}

    /* */
    saves += victim->level - level;

    /* Бонус или штраф до 25% от резистов и вульнов */
    saves += saves * res / 300;

    /* 1% = 2.4 савеса. 5% с обоих сторон. 5% что каст не пройдет, даже без савесов, и 5% что каст пройдет даже при полной защите */
    saves = URANGE(12, saves, MAX_SAVES - 12);
    return number_range(1, MAX_SAVES) < saves;
}


/* RT save for dispels */
/* * *
 * ЗАЩИТА ОТ МАГИИ ЧЕРЕЗ СИЛУ ВОЛИ ДЛЯ СОГНАТЬ (SAVES)
 *  tag:    #saves #сила воли
 *  last:   brullek@gmail.com   2018-11-21
 * * */
bool saves_dispel(int dis_level, int spell_level, int duration, CHAR_DATA *victim){
    int save = 0;


    if (victim && (save = check_immune(victim, DAM_MAGIC)) > 99)
	return TRUE;

    if (duration == -1)
	spell_level += 5;
    /* very hard to dispel permanent effects */

    save += 20 + (spell_level - dis_level) * 5;
    save = URANGE(2, save, 98);
    return number_percent() < save;

	// %резиста + 20 + 5*уровень_сгоняемого_каста - 5*уровень_согнать
}

/* co-routine for dispel magic and cancellation */

bool check_dispel(int dis_level, CHAR_DATA *victim, int sn){
    AFFECT_DATA *af;

    if (is_affected(victim, sn)){
        for (af = victim->affected; af != NULL; af = af->next){
            if (af->type == sn){
                if (!saves_dispel(dis_level, af->level, af->duration, victim)){
                    if (af->type == gsn_shapechange){
                        extract_char(victim, TRUE);
                        return TRUE;
                    } else {
                        if (af->bitvector == AFF_CHARM && victim->master){
                            act("$N прекращает следовать за тобой.", victim->master, NULL, victim, TO_CHAR);
                            victim->leader = NULL;
                            victim->master = NULL;
                        }
                    }

                    affect_strip(victim, sn);
                    if (skill_table[sn].msg_off){
                        act(skill_table[sn].msg_off, victim, NULL, NULL, TO_CHAR);
                    }
                    return TRUE;
                } else {
                    af->level--;
                }
            }
        }
    }
    return FALSE;
}

/* for finding mana costs -- temporary version */
int mana_cost(CHAR_DATA *ch, int min_mana, int level)
{
    if (ch->level + 2 == level)
	return 1000;
    return UMAX(min_mana, (100/(2 + ch->level - level)));
}

void check_offensive(int sn, int target, CHAR_DATA *ch, CHAR_DATA *victim, bool tar)
{
    if (victim == NULL || ch == NULL || ch->in_room == NULL)
	return;

    if ((tar || skill_table[sn].target == TAR_CHAR_OFFENSIVE
	 || (skill_table[sn].target == TAR_OBJ_CHAR_OFF
	     && target == TARGET_CHAR))
	&& victim != ch)
	//  && victim->master != ch)
    {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;

	LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, vch_next)
	{
	    if (victim == vch && (victim->fighting == NULL || ch->fighting == NULL))
	    {
		check_killer(ch, victim);

		/*    				if (sn != gsn_sleep && IS_AFFECTED(victim, AFF_SLEEP))
		 {
		 affect_strip(victim, gsn_sleep);
		 do_stand(victim, "");
		 } */

		if (victim->position == POS_SLEEPING && !IS_AFFECTED(victim, AFF_SLEEP))
		    do_stand(victim, "");

		if (IS_NPC(victim) && has_trigger(victim->pIndexData->progs, TRIG_KILL, EXEC_ALL))
		    p_percent_trigger(victim, NULL, NULL, ch, NULL, NULL, TRIG_KILL);

		if (victim->fighting == NULL)
		    multi_hit(victim, ch, TYPE_UNDEFINED);

		break;
	    }
	}
    }
}


/*
 * The kludgy global is for spells who want more stuff from command line.
 */
char *target_name;

void do_cast(CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim, *orig, *check, *vch;
    OBJ_DATA *obj, *negobj;
    void *vo;
    int mana;
    int sn;
    int target;
    int chance;
    int level;
    int i;
    int dam = 0;
    bool success;

    /*
     * Switched NPC's can cast spells, but others can't.
     */
    if (ch == NULL || (IS_NPC(ch) && ch->pIndexData->vnum != MOB_VNUM_GOMUN && ch->desc == NULL && !IS_SET(ch->act, ACT_INLUA)))
	return;

    if (ch->in_room == NULL){
		bugf("Do_cast: char '%s', NULL room.", ch->name);
		return;
    }

    if (is_lycanthrope(ch) && !IS_IMMORTAL(ch)){
		send_to_char("Из твоего горла вырывается только невнятное рычание.\n\r", ch);
		return;
    }

    target_name = one_argument(argument, arg1);
    one_argument(target_name, arg2);

    if (arg1[0] == '\0'){
		send_to_char("Колдовать что на кого?\n\r", ch);
		return;
    }

    if (ch->desc != NULL && ch->desc->original != NULL){
		orig = ch->desc->original;
	} else {
		orig = ch;
	}


    if ((sn = find_spell(orig, arg1)) < 1 || skill_table[sn].spell_fun == spell_null || (!IS_NPC(orig)
		&& (orig->level < skill_table[sn].skill_level[orig->classid] || orig->pcdata->learned[sn] < 1))){
		send_to_char("Ты не знаешь ни одного заклинания с таким именем.\n\r", ch);
		return;
    }

    if (ch->position < skill_table[sn].minimum_position){
		send_to_char("Ты не можешь сконцентрироваться.\n\r", ch);
		return;
    }

    if (IS_SET (ch->in_room->room_flags, ROOM_NOMAGIC)){
		send_to_char("Ты декламируешь заклинание ... "
		"И ничего не происходит.\n\r", ch);
		return;
    }

    if (!IS_NPC(ch) && IS_SET(ch->act, PLR_NOCAST)){
		send_to_char("Ты пытаешься продекламировать слова, "
		"но они не хотят выходить изо рта.\n\r", ch);
		return;
    }

    if (is_affected(ch,gsn_protection_sphere)){
		send_to_char("Пока на тебе защитная сфера, "
		"ты не можешь колдовать заклинания.\n\r", ch);
		return;
    }

    if (ch->level + 2 == skill_table[sn].skill_level[ch->classid]){
		mana = 50;
	} else {
		mana = UMAX(skill_table[sn].min_mana, 100/(2+ch->level - skill_table[sn].skill_level[ch->classid]));
	}


	/* ПОД ЭФФЕКТОМ УТЕЧКА МАНЫ РАСХОД В 2 РАЗ БОЛЬШЕ НА ВСЕ КАСТЫ*/
    if (is_affected(ch, gsn_manaleak)) mana *= 2;
         
    /*
     * Locate targets.
     */
    victim  = NULL;
    obj    = NULL;
    vo    = NULL;
    check  = NULL;
    target  = TARGET_NONE;

    switch (skill_table[sn].target){
		default:
		bugf("Do_cast: bad target for sn %d.", sn);
		return;

		case TAR_IGNORE:
		break;

		case TAR_CHAR_OFFENSIVE:

		if (arg2[0] == '\0'){
			if ((victim = ch->fighting) == NULL){
			send_to_char("Колдовать заклинание на кого?\n\r", ch);
			return;
			}
		} else {
			if ((victim = get_char_room(ch, NULL, target_name, TRUE)) == NULL){
				/*код, отвечающий за каст по одному из тех, кто тебя бьет, выбираемому случайно*/
				if (!str_prefix(target_name, "случайно") || !str_prefix(target_name, "random")){
					int count = 0;

					//проверить комнату на кол-во персонажей, с которым дерешься, выбрать цель
					LIST_FOREACH(vch, &ch->in_room->people, room_link){
					if (vch->fighting == ch && number_range(1, 100000) < 100000/(++count))
						victim = vch;
					}

					if (count == 0){
						send_to_char("Ты ни с кем не сражаешься.\n\r", ch);
						return;
					}
					send_to_char("Ты направляешь заклинание без цели.\n\r", ch);
				} else {
					if (!too_many_victims)
					send_to_char("Таких здесь нет.\n\r", ch);
					return;
				}
				/*конец кода, отвечающего за случайный каст. При удалении не забыть оставить else {...} перед этой строкой комментария.*/
			}
		}

		if (ch == victim){
			printf_to_char("Ты мазохист%s?\n\r", ch, ch->sex == SEX_FEMALE ? "ка" : "");
			return;
		}

		if (is_safe_spell(ch, victim, FALSE)){
			send_to_char("Только не здесь и не на это!\n\r", ch);
			return;
		}

		if (check_charmees(ch, victim))
			return;

		vo = (void *) victim;
		target = TARGET_CHAR;
		check = victim;
		break;

		case TAR_CHAR_DEFENSIVE:
		if (arg2[0] == '\0'){
			victim = ch;
		} else{
			if ((victim = get_char_room(ch, NULL, target_name, FALSE)) == NULL){
				send_to_char("Таких здесь нет.\n\r", ch);
				return;
			}
		}

		vo = (void *) victim;
		target = TARGET_CHAR;
		break;

		case TAR_CHAR_SELF:
		if (arg2[0] != '\0' && !is_name(target_name, ch->name)){
			send_to_char("Ты не можешь колдовать это заклинание на других.\n\r", ch);
			return;
		}

		vo = (void *) ch;
		target = TARGET_CHAR;
		break;

		case TAR_OBJ_INV:
		if (arg2[0] == '\0')
		{
			send_to_char("На что ты колдуешь это заклинание?\n\r", ch);
			return;
		}

		if ((obj = get_obj_carry(ch, arg2, ch)) == NULL)
		{
			send_to_char("Ты не несешь этого.\n\r", ch);
			return;
		}

		vo = (void *) obj;
		target = TARGET_OBJ;
		break;

		case TAR_OBJ_CHAR_OFF:

		if (arg2[0] == '\0')
		{
			if ((victim = ch->fighting) == NULL)
			{
			send_to_char("Колдовать это заклинание на кого?\n\r",
					 ch);
			return;
			}

			target = TARGET_CHAR;
		}
		else if ((victim = get_char_room(ch, NULL, target_name, TRUE)) != NULL)
		{
			target = TARGET_CHAR;
		}
		else if (too_many_victims)
			return;

		if (target == TARGET_CHAR) /* check the sanity of the attack */
		{
			if(is_safe_spell(ch, victim, FALSE) && victim != ch)
			{
			send_to_char("Только не на это.\n\r", ch);
			return;
			}

			if (check_charmees(ch, victim))
			return;

			vo = (void *) victim;
		}
		else if ((obj = get_obj_here(ch, NULL, target_name)) != NULL)
		{
			vo = (void *) obj;
			target = TARGET_OBJ;
		}
		else
		{
			send_to_char("Ты не видишь этого здесь.\n\r", ch);
			return;
		}
		check = victim;
		break;

		case TAR_OBJ_CHAR_DEF:
		if (arg2[0] == '\0')
		{
			vo = (void *) ch;
			target = TARGET_CHAR;
		}
		else if ((victim = get_char_room(ch, NULL, target_name, FALSE)) != NULL)
		{
			vo = (void *) victim;
			target = TARGET_CHAR;
		}
		else if ((obj = get_obj_carry(ch, target_name, ch)) != NULL)
		{
			vo = (void *) obj;
			target = TARGET_OBJ;
		}
		else
		{
			send_to_char("Ты не видишь этого здесь.\n\r", ch);
			return;
		}
		break;
    }
    
    
    //сделал, что мобы при нехватке маны тоже не могут кастить. (Кей)

    if (ch->mana < mana 
	/*&& (!IS_NPC(ch) || IS_SWITCHED(ch))*/)
    {
	if ( (!IS_NPC(ch) || IS_SWITCHED(ch)) )
	{
	    sprintf(arg1, "У тебя не хватает энергии для заклинания '%s'.\n\r", skill_table[sn].rname);
	    send_to_char(arg1, ch);
	}
	return;
    }

    WAIT_STATE(ch, skill_table[sn].beats);

    chance = get_skill(orig, sn);

    if (IS_NPC(ch))
	level = ch->level;
    else
	{
		if (IS_VAMPIRE(ch)) {
			level = (int) ((9 + class_table[ch->classid].fMana)*ch->level / 12.35);
		} else {
			level = (9 + class_table[ch->classid].fMana) * ch->level / 12;
		}
	}

    for (i = 0; i < max_moons; i++)
    {
	if ((((skill_table[sn].target == TAR_CHAR_OFFENSIVE
	       || skill_table[sn].target == TAR_OBJ_CHAR_OFF)
	      && moons_data[i].type == TAR_CHAR_OFFENSIVE)
	     || ((skill_table[sn].target == TAR_CHAR_DEFENSIVE
		  || skill_table[sn].target == TAR_CHAR_SELF
		  || skill_table[sn].target == TAR_OBJ_CHAR_DEF)
		 && moons_data[i].type == TAR_CHAR_DEFENSIVE))
	    && moons_data[i].moonlight >= moons_data[i].period / 2)
	{
	    switch (get_moon_phase(i))
	    {
	    case MOON_NEW:
		level = UMAX(1, level - 2);
		if (chance > 0)
		    chance = chance - 5;
		break;
	    case MOON_1QTR:
	    case MOON_4QTR:
		level = UMAX(1, level - 1);
		if (chance > 0)
		    chance = chance - 2;
		break;
	    case MOON_2QTR:
	    case MOON_3QTR:
		level += 1;
		if (chance > 0)
		    chance += 5;
		break;
	    case MOON_FULL:
		level += 2;
		if (chance > 0)
		    chance += 10;
		break;
	    }
	}
    }

    /*
     * Вампиры штрафуются только с 6 до 18 
     */
    if (!IS_NPC(ch))
    {
	switch (ch->classid)
	{
	case CLASS_MAGE:
	case CLASS_NECROMANT:
	case CLASS_LYCANTHROPE:
    	chance -= (MAX_STAT - UMAX(get_curr_stat(ch, STAT_INT), get_curr_stat(ch, STAT_WIS))  ) * chance / 100;
	    break;
	case CLASS_NAZGUL:
    	chance -= (MAX_STAT - UMAX(get_curr_stat(ch, STAT_INT), get_curr_stat(ch, STAT_WIS))  ) * chance / 100;
	    break;
	case CLASS_CLERIC:
	case CLASS_PALADIN:
    	chance -= (MAX_STAT - UMAX(get_curr_stat(ch, STAT_INT), get_curr_stat(ch, STAT_WIS))  ) * chance / 100;
	    break;
	case CLASS_DRUID:
	    chance -= (MAX_STAT - get_curr_stat(ch, STAT_INT)) * chance / 200;
	    chance -= (MAX_STAT - get_curr_stat(ch, STAT_WIS)) * chance / 200;
	    break;
	case CLASS_VAMPIRE:
	    if (time_info.hour >= 6 && time_info.hour <= 18)
	    {
	    	chance -= (MAX_STAT - get_curr_stat(ch, STAT_INT)) * chance / 200;
	    	chance -= (MAX_STAT - get_curr_stat(ch, STAT_WIS)) * chance / 200;
	    }
	    break;
	default:
	    break;
	};
    }

    if (ch->position == POS_BASHED)
	chance /= 2;

    if (is_affected(ch, gsn_concentrate))
	chance *= 1.5;

    //Оборотень с духом кастит хуже
    if (is_lycanthrope_spirit(ch))
	chance /= 2;	

    if (ch->in_room)
	for (negobj = ch->in_room->contents; negobj; negobj = negobj->next_content)
	{
	    if (negobj->pIndexData->vnum == OBJ_VNUM_CLOUD
	    && negobj->value[0] == CLOUD_POISON)
	    {
		chance /= 1.5;

		if (!IS_VAMPIRE(ch) && !saves_spell(negobj->level, ch, DAM_POISON))
		{	
		    chance /= 1.5;
		    dam = UMAX(0, negobj->level);
		    poison_effect(ch, negobj->level/4, dam/8, TARGET_CHAR, ch);
		}
	    }
	}

    if (number_percent() > chance)
	success = FALSE;
    else
	success = TRUE;

    if (sn != skill_lookup("ventriloquate"))
	say_spell(ch, sn, FALSE, success);
    
    if (!success)
    {
	send_to_char("Ты теряешь концентрацию.\n\r", ch);
	act("$n сбивается в середине колдуемого заклинания.{x", ch, NULL, NULL, TO_ROOM);
	check_improve(ch, check, sn, FALSE, 1);
	ch->mana -= mana / 2;
    }
    else
    {
	if (victim != NULL && is_affected(victim,gsn_protection_sphere))
	{
	    act("Защитная сфера $N1 блокирует твое заклинание.{x", ch, NULL, victim, TO_CHAR);
	    ch->mana -=mana;
	    check_offensive(sn, target, ch, victim, FALSE);
	    return;
        }

	if (is_affected(ch,gsn_dissolve))
	{
	    affect_strip (ch, gsn_dissolve);
	    WAIT_STATE(ch, skill_table[gsn_dissolve].beats * 2);
	    printf_to_char("Ты становишься видим%s для всех.\n\r", ch, SEX_END_ADJ(ch));
	}

	if ((*skill_table[sn].spell_fun) (sn, level, ch, vo, target))
	{
	    check_improve(ch, check, sn, TRUE, 1);
	    ch->mana -= mana;
	}
	else
	    ch->mana -= mana / 2;
    }

    check_offensive(sn, target, ch, victim, FALSE);

    return;
}



/*
 * Cast spells at targets using a magical object.
 */
void obj_cast_spell(int sn, int level, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj, char *tn)
{
    void *vo;
    int target = TARGET_NONE;

    if (sn <= 0)
	return;

    if (sn >= max_skills || skill_table[sn].spell_fun == 0)
    {
	bugf("Obj_cast_spell: bad sn %d.", sn);
	return;
    }

    if (IS_SET (ch->in_room->room_flags, ROOM_NOMAGIC))
	return;

    switch (skill_table[sn].target)
    {
    default:
	bugf("Obj_cast_spell: bad target for sn %d.", sn);
	return;

    case TAR_IGNORE:
	vo = NULL;
	break;

    case TAR_CHAR_OFFENSIVE:
	if (victim == NULL)
	    victim = ch->fighting;
	if (victim == NULL)
	{
	    send_to_char("Ты не можешь сделать это.\n\r", ch);
	    return;
	}
	if (is_safe(ch, victim))
	{
	    send_to_char("Что-то не так...\n\r", ch);
	    return;
	}
	vo = (void *) victim;
	target = TARGET_CHAR;
	break;


    case TAR_CHAR_SELF:
	vo = (void *) ch;
	target = TARGET_CHAR;
	break;

    case TAR_CHAR_DEFENSIVE:
	vo = (void *) (victim ? victim : ch);
	target = TARGET_CHAR;
	break;

    case TAR_OBJ_INV:
	if (obj == NULL)
	{
	    send_to_char("Ты не можешь этого сделать.\n\r", ch);
	    return;
	}
	vo = (void *) obj;
	target = TARGET_OBJ;
	break;

    case TAR_OBJ_CHAR_OFF:
	if (victim == NULL && obj == NULL)
	{
	    if (ch->fighting != NULL)
		victim = ch->fighting;
	    else
	    {
		send_to_char("Ты не можешь этого сделать.\n\r", ch);
		return;
	    }
	}

	if (victim != NULL)
	{
	    if (is_safe_spell(ch, victim, FALSE))
	    {
		send_to_char("Что-то не так...\n\r", ch);
		return;
	    }

	    vo = (void *) victim;
	    target = TARGET_CHAR;
	}
	else
	{
	    vo = (void *) obj;
	    target = TARGET_OBJ;
	}
	break;


    case TAR_OBJ_CHAR_DEF:
	if (victim == NULL && obj == NULL)
	{
	    vo = (void *) ch;
	    target = TARGET_CHAR;
	}
	else if (victim != NULL)
	{
	    vo = (void *) victim;
	    target = TARGET_CHAR;
	}
	else
	{
	    vo = (void *) obj;
	    target = TARGET_OBJ;
	}

	break;
    }

    target_name = tn;

    OBJcast = TRUE;
    (*skill_table[sn].spell_fun) (sn, level, ch, vo, target);
    OBJcast = FALSE;

    check_offensive(sn, target, ch, victim, FALSE);

    return;
}

/*
 * Spell functions.
 */
bool spell_acid_blast(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice(level, 12);
    if (saves_spell(level, victim, DAM_ACID))
	dam /= 2;
    damage(ch, victim, dam, sn, DAM_ACID, TRUE, NULL);
    return TRUE;
}



bool spell_armor(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (!is_replace(sn, level, victim))
    {
	if (victim == ch)
	    send_to_char("Ты уже под действием брони.\n\r", ch);
	else
	    act("$N уже под действием брони.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    affect_strip(victim, sn);

    af.where   = TO_AFFECTS;
    af.type      = sn;
    af.level   = level;
    af.duration  = 10 + level/2;
    af.modifier  = (self_magic(ch,victim) ? -25 : -10) - level/3;
    af.location  = APPLY_AC;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    send_to_char("Ты чувствуешь, как твоя броня улучшилась!\n\r{x", victim);

    if (ch != victim)
	act("$N защищается твоей магией.{x", ch, NULL, victim, TO_CHAR);

    return TRUE;
}

bool spell_bless(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA af;
    char buf[MAX_STRING_LENGTH];

    /* deal with the object case first */
    if (target == TARGET_OBJ)
    {
	obj = (OBJ_DATA *) vo;
	if (IS_OBJ_STAT(obj, ITEM_BLESS))
	{
	    act("$p - уже благословлено.", ch, obj, NULL, TO_CHAR);
	    return FALSE;
	}

	if (IS_OBJ_STAT(obj, ITEM_EVIL))
	{
	    AFFECT_DATA *paf;

	    paf = affect_find(obj->affected, gsn_curse);
	    if (!saves_dispel(level, paf != NULL ? paf->level : obj->level, 0, NULL))
	    {
		if (paf != NULL)
		    affect_remove_obj(obj, paf);
		act("$p освеща$rся чистым синим цветом!{x",
		    ch, obj, NULL, TO_ALL);
		REMOVE_BIT(obj->extra_flags, ITEM_EVIL);
		return TRUE;
	    }
	    else
	    {
		act("Сила зла на $p5 слишком сильна.",
		    ch, obj, NULL, TO_CHAR);
		return FALSE;
	    }
	}

	af.where  = TO_OBJECT;
	af.type    = sn;
	af.level  = level;
	af.duration  = 6 + level;
	af.location  = APPLY_SAVES;
	af.modifier  = -1;
	af.bitvector  = ITEM_BLESS;
	affect_to_obj(obj, &af);

	act("$p загора$rся святой аурой.{x",
	    ch, obj, NULL, TO_ALL);

	if (obj->wear_loc != WEAR_NONE)
	    ch->saving_throw -= 1;
	return TRUE;
    }

    /* character target */
    victim = (CHAR_DATA *) vo;

    if (IS_VAMPIRE(victim) && (!IS_IMMORTAL(ch)))
    {
	if (ch == victim)
	    send_to_char("Ни один Бог не благоволит тебе!\n\r", ch);
	else
	    send_to_char("Это существо противно твоему Богу!\n\r", ch);

	return FALSE;
    }

    if (((IS_GOOD(ch) && IS_EVIL(victim))
	|| (IS_EVIL(ch) && IS_GOOD(victim)))
	&& !IS_IMMORTAL(ch)
	&& !IS_NPC(ch))
    {
	send_to_char("Это существо противно твоему Богу!\n\r", ch);
	return FALSE;
    }


    if (!is_replace(sn, level, victim))
    {
	if (victim == ch)
	    send_to_char("Ты уже под действием благословения.\n\r", ch);
	else
	    act("$N уже под покровительством богов.",
		ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    affect_strip(victim, sn);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level   = level;
    af.duration  = 6+level;
    af.location  = APPLY_HITROLL;
    af.modifier  = level / 8;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = 0 - level / 8;
    affect_to_char(victim, &af);

    af.where = TO_RESIST;
    af.location = APPLY_NONE;
    af.modifier = level / 10;
    af.bitvector = DAM_NEGATIVE;
    affect_to_char(victim, &af);

    sprintf(buf, "Ты ощущаешь на себе покровительство богов.\n\r{x");
    send_to_char(buf, victim);

    if (ch != victim)
	act("Ты даешь $N2 покровительство своего бога.{x",
	    ch, NULL, victim, TO_CHAR);
    return TRUE;
}



bool spell_blindness(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    char buf[50];

    if (is_affected(victim, gsn_st_aura)){
		sprintf(buf, "Святая аура $N1 мешает тебе ослепить его.");
		act(buf, ch, NULL, victim, TO_CHAR);
		return FALSE;
    }

    if (IS_AFFECTED(victim, AFF_BLIND)){
		sprintf(buf, "$N уже слеп%s.", SEX_ENDING(victim));
		act(buf, ch, NULL, victim, TO_CHAR);
		return FALSE;
    }

    if (saves_spell(level, victim, DAM_OTHER) || victim->classid == CLASS_NAZGUL){
		send_to_char("Не выходит!\n\r", ch);
		return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.location  = APPLY_HITROLL;
    af.modifier  = -4;
    af.duration  = 1 + level / 10;
    af.bitvector = AFF_BLIND;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    send_to_char("Тебя ослепили!\n\r{x", victim);
    act("$n выглядит ослепленн$t!{x", victim, SEX_END_ADJ(victim), NULL, TO_ROOM);
    return TRUE;
}



bool spell_burning_hands(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    static const int16_t dam_each[] =
    {
	0,
	0,  0,  0,  0,  14,  17, 20, 23, 26, 29,
	29, 29, 30, 30,  31,  31, 32, 32, 33, 33,
	34, 34, 35, 35,  36,  36, 37, 37, 38, 38,
	39, 39, 40, 40,  41,  41, 42, 42, 43, 43,
	44, 44, 45, 45,  46,  46, 47, 47, 48, 48
    };
    int dam;

    if (target == TARGET_CHAR && vo != NULL)
    {
	level  = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
	level  = UMAX(0, level);
	dam    = number_range(dam_each[level] / 2, dam_each[level] * 2);
	if (saves_spell(level, victim, DAM_FIRE))
	    dam /= 2;
	damage(ch, victim, dam, sn, DAM_FIRE, TRUE, NULL);
    }
    else if (target == TARGET_OBJ && vo != NULL)
    {
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	AFFECT_DATA af;

	if (obj->item_type != ITEM_WEAPON)
	{
	    send_to_char("Ты можешь колдовать это заклинание только на "
			 "оружие.\n\r", ch);
	    return FALSE;
	}

	if (obj->level > ch->level)
	{
	    send_to_char("У тебя не хватает сил.\n\r", ch);
	    return FALSE;
	}

	if (IS_WEAPON_STAT(obj, WEAPON_POISON)
	    || IS_WEAPON_STAT(obj, WEAPON_FROST)
	    || IS_WEAPON_STAT(obj, WEAPON_SHOCKING)
	    || material_lookup(obj->material) == MAT_ICE)
	{
	    send_to_char("Ничего не выходит.\n\r", ch);
	    return FALSE;
	}

	if (IS_WEAPON_STAT(obj, WEAPON_FLAMING))
	{
	    act("По $p2 и так уже гуляют сполохи огня.",
		ch, obj, NULL, TO_CHAR);
	    return FALSE;
	}

	act("Ты прикасаешься рукой к своему оружию, и на $p5 начинают плясать язычки пламени.{x",
	    ch, obj, NULL, TO_CHAR);
	act("$n прикасается рукой к своему оружию, и на $p5 начинают плясать язычки пламени.{x",
	    ch, obj, NULL, TO_ROOM);

	obj->condition -= 5;
	check_condition(ch, obj);

	af.where = TO_WEAPON;
	af.type = sn;
	af.level = level;
	af.duration = level / 2;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = WEAPON_FLAMING;
	affect_to_obj(obj, &af);
    }
    else
	send_to_char("Ты не видишь ничего (и никого) похожего здесь.\n\r", ch);

    return TRUE;
}



bool spell_call_lightning(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam;

    if (!IS_OUTSIDE(ch))
    {
	send_to_char("Ты должен выйти из помещения.\n\r", ch);
	return FALSE;
    }

    if (weather_info.sky < SKY_RAINING)
    {
	send_to_char("Для этого нужна плохая погода.\n\r", ch);
	return FALSE;
    }

    dam = dice(level, 8);

    send_to_char("Молния бьет твоего противника!\n\r", ch);
    act("$n вызывает молнию, и она бьет $s противника!",
	ch, NULL, NULL, TO_ROOM);

    LIST_FOREACH_SAFE(vch, &char_list, link, vch_next)
    {
	if (vch->in_room == NULL)
	    continue;

	if (vch->in_room == ch->in_room)
	{
	    if (is_safe_spell(ch, vch, TRUE)
		|| (IS_NPC(vch) && IS_NPC(ch)
		    && (ch->fighting != vch || vch->fighting != ch))
		|| get_master(ch) == get_master(vch))
		continue;

	    damage(ch, vch, saves_spell(level, vch, DAM_LIGHTNING)
		   ? dam / 2 : dam, sn, DAM_LIGHTNING, TRUE, NULL);
	}
	else if (vch->in_room->area == ch->in_room->area
		 && IS_OUTSIDE(vch)
		 && IS_AWAKE(vch))
	{
	    send_to_char("Молнии сверкают вдали.\n\r", vch);
	}
    }

    return TRUE;
}

/* RT calm spell stops all fighting in the room */

bool spell_calm(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vch, *safe_vch;
    int mlevel = 0;
    int count = 0;
    int high_level = 0;
    int chance;
    AFFECT_DATA af;

    if (IS_SET(ch->in_room->room_flags, ROOM_SAFE))
    {
	send_to_char("Только не здесь.\n\r", ch);
	return FALSE;
    }

    /* get sum of all mobile levels in the room */
    LIST_FOREACH(vch, &ch->in_room->people, room_link)
    {
	if (POS_FIGHT(vch))
	{
	    count++;
	    if (IS_NPC(vch))
		mlevel += vch->level;
	    else
		mlevel += vch->level/2;
	    high_level = UMAX(high_level, vch->level);
	}
    }

    /* compute chance of stopping combat */
    chance = 4 * level - high_level + 2 * count;

    if (IS_IMMORTAL(ch)) /* always works */
	mlevel = 0;

    if (number_range(0, chance) >= mlevel)  /* hard to stop large fights */
    {
	LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, safe_vch)
	{
	    if (!POS_FIGHT(vch))
	        continue;
	    
	    if (IS_NPC(vch)
		&& (check_immune(vch, DAM_MAGIC) == 100
		    || IS_SET(vch->act, ACT_UNDEAD)))
		continue;

	    if (IS_AFFECTED(vch, AFF_CALM) || IS_AFFECTED(vch, AFF_BERSERK)
		|| is_affected(vch, skill_lookup("frenzy")))
		continue;

	    send_to_char("Волна спокойствия накатывает на тебя.\n\r{x", vch);

	    if (vch->fighting || POS_FIGHT(vch))
		stop_fighting(vch, FALSE);

	    //добавил, что после каста спокойствия все в комнате становятся спокойными, т.е. волнение проходит
            if (!IS_NPC(vch))
	    	REMOVE_BIT(vch->act, PLR_EXCITED);

	    af.where = TO_AFFECTS;
	    af.type = sn;
	    af.level = level;
	    af.duration = level/4;
	    af.location = APPLY_HITROLL;

	    if (!IS_NPC(vch))
		af.modifier = -4;
	    else
		af.modifier = -2;

	    af.bitvector = AFF_CALM;
	    af.caster_id = get_id(ch);
	    affect_to_char(vch, &af);

	    af.location = APPLY_DAMROLL;
	    affect_to_char(vch, &af);
	}

	send_to_char("Ты останавливаешь драку!\n\r", ch);
    }
    else
        send_to_char("Не получается...\n\r", ch);

    return TRUE;
}

bool spell_cancellation(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    bool found = FALSE;
    int i;

    if ( victim->master == NULL || victim->master != ch)
	/*не стал разбираться со след.if-ом, проверки летят к черту.
	 Если жертва ни за кем не следует, или же если ее мастер - не кастер,
	 то делаем уже проверку на допустимость отмены :-) */
    {
	if ((!IS_NPC(ch)
	     && IS_NPC(victim)
	     && !(IS_AFFECTED(ch, AFF_CHARM)
		  && ch->master == victim
		  && victim->master != ch))//условие 1
	    || (IS_NPC(ch)
		&& !IS_NPC(victim)
		&& !IS_SET(ch->act, ACT_IS_HEALER))//условие 2
	    || (ch != victim
		&& !IS_NPC(victim)
		&& IS_SET(victim->act, PLR_NOCANCEL)))//условие 3
	{
	    send_to_char("Не получается, попробуй изгнать магию.\n\r", ch);
	    return FALSE;
	}
    }


    /* unlike dispel magic, the victim gets NO save */

    /* begin running through the spells */

    for (i = 0; cancel_spell[i].spell != NULL; i++)
	if (check_dispel(level/cancel_spell[i].hard, victim,
			 skill_lookup(cancel_spell[i].spell)))
	{
	    found = TRUE;

	    if (!IS_NULLSTR(cancel_spell[i].msg))
		act(cancel_spell[i].msg, victim, NULL, NULL, TO_ROOM);
	}

    if (found)
	send_to_char("Ok.\n\r", ch);
    else
	send_to_char("Заклинание не удалось.\n\r", ch);

    return TRUE;
}

bool spell_cause_light(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_affected(victim, gsn_st_aura))
    {
	act("Святая аура $N1 мешает тебе нанести $M вред.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    if (!IS_NPC(ch))
    {
	if (is_affected(victim, gsn_lighting_harm)||
	    is_affected(victim, gsn_serious_harm)||
	    is_affected(victim, gsn_critical_harm)||
	    is_affected(victim, gsn_harm) )
	{
	    send_to_char("Это существо уже и так не может лечиться.\n\r",ch);
	}
	else
	    if (saves_spell(level, victim, DAM_HARM)|| is_affected(victim, gsn_bandage))
	    {
		act("Лишить $S способности к лечению не вышло.\n\r", ch, NULL, victim, TO_CHAR);
	    }
	    else
	    {
		af.where  = TO_AFFECTS;
		af.type   = gsn_lighting_harm;
		af.level   = ch->level;
		af.duration  = 3;
		af.location  = APPLY_NONE;
		af.modifier  = 0;
		af.bitvector = 0;
		af.caster_id = get_id(ch);
		affect_to_char(victim, &af);

		act("Ты лишаешь $N3 способности к лечению!",  ch, NULL, victim, TO_CHAR);
		act("{R$n лишает тебя способности к лечению!{x",  ch, NULL, victim, TO_VICT);
		act("$N больше не может лечиться.", ch, NULL, victim, TO_ROOM);
	    }
    }
    damage(ch, (CHAR_DATA *)vo, dice(2, 8) + level / 2, sn, DAM_HARM, TRUE, NULL);
    return TRUE;
}



bool spell_cause_critical(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_affected(victim, gsn_st_aura))
    {
	act("Святая аура $N1 мешает тебе нанести $M вред.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    if (!IS_NPC(ch))
    {
	if (is_affected(victim, gsn_critical_harm)||
	    is_affected(victim, gsn_harm) )
	{
	    act("$E уже и так не может лечиться.",ch, NULL, victim, TO_CHAR);
	}
	else
	    if (saves_spell(level, victim, DAM_HARM)|| is_affected(victim, gsn_bandage))
	    {
		act("Лишить $S способности к лечению не вышло.", ch, NULL, victim, TO_CHAR);
	    }
	    else
	    {
		af.where  = TO_AFFECTS;
		af.type   = gsn_critical_harm;
		af.level   = ch->level;
		af.duration  = 3;
		af.location  = APPLY_NONE;
		af.modifier  = 0;
		af.bitvector = 0;
		af.caster_id = get_id(ch);
		affect_to_char(victim, &af);

		act("Ты лишаешь $N3 способности к лечению!",  ch, NULL, victim, TO_CHAR);
		act("{R$n лишает тебя способности к лечению!{x",  ch, NULL, victim, TO_VICT);
		act("$N больше не может лечиться.", ch, NULL, victim, TO_ROOM);
	    }
    }
    damage(ch, (CHAR_DATA *) vo, dice(4, 8) + level - 2, sn, DAM_HARM, TRUE, NULL);
    return TRUE;
}



bool spell_cause_serious(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_affected(victim, gsn_st_aura))
    {
	act("Святая аура $N1 мешает тебе нанести $M вред.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    if (!IS_NPC(ch))
    {
	if (is_affected(victim, gsn_serious_harm)||
	    is_affected(victim, gsn_critical_harm)||
	    is_affected(victim, gsn_harm) )
	{
	    act("$E уже и так не может лечиться.",ch, NULL, victim, TO_CHAR);
	}
	else
	    if (saves_spell(level, victim, DAM_HARM)|| is_affected(victim, gsn_bandage))
	    {
		act("Лишить $S способности к лечению не вышло.", ch, NULL, victim, TO_CHAR);
	    }
	    else
	    {
		af.where  = TO_AFFECTS;
		af.type   = gsn_serious_harm;
		af.level   = ch->level;
		af.duration  = 3;
		af.location  = APPLY_NONE;
		af.modifier  = 0;
		af.bitvector = 0;
		af.caster_id = get_id(ch);
		affect_to_char(victim, &af);

		act("Ты лишаешь $N3 способности к лечению!",  ch, NULL, victim, TO_CHAR);
		act("{R$n лишает тебя способности к лечению!{x",  ch, NULL, victim, TO_VICT);
		act("$N больше не может лечиться.", ch, NULL, victim, TO_ROOM);
	    }

    }
    damage(ch, (CHAR_DATA *) vo, dice(3, 8) + level / 2, sn, DAM_HARM, TRUE, NULL);
    return TRUE;
}

bool spell_chain_lightning(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *tmp_vict, *last_vict, *next_vict;
    bool found;
    int dam;

    /* first strike */

    act("Световой шар вылетает из рук $n1 и бьет $N3.",
	ch, NULL, victim, TO_ROOM);
    act("Световой шар вылетает из твоих рук и бьет $N3.",
	ch, NULL, victim, TO_CHAR);
    act("Световой шар вылетает из рук $n1 и бьет тебя!",
	ch, NULL, victim, TO_VICT);

    dam = dice(level, 6);
    if (saves_spell(level, victim, DAM_LIGHTNING))
	dam /= 3;
    damage(ch, victim, dam, sn, DAM_LIGHTNING, TRUE, NULL);
    last_vict = victim;
    level -= 4;   /* decrement damage */

    /* new targets */
    while (level > 0)
    {
	found = FALSE;
	LIST_FOREACH_SAFE(tmp_vict, &ch->in_room->people, room_link, next_vict)
	{
	    if (!is_safe_spell(ch, tmp_vict, TRUE) && tmp_vict != last_vict)
	    {
		found = TRUE;
		last_vict = tmp_vict;
		act("Шар бьет $n3!", tmp_vict, NULL, NULL, TO_ROOM);
		act("Шар бьет тебя!", tmp_vict, NULL, NULL, TO_CHAR);
		dam = dice(level,6);
		if (saves_spell(level, tmp_vict, DAM_LIGHTNING))
		    dam /= 3;
		damage(ch, tmp_vict, dam, sn, DAM_LIGHTNING, TRUE, NULL);
		level -= 4;  /* decrement damage */
	    }
	}   /* end target searching loop */

	if (!found) /* no target found, hit the caster */
	{
	    if (ch == NULL || ch->position == POS_DEAD)
		return TRUE;

	    if (last_vict == ch) /* no double hits */
	    {
		act("Шар опускается $x.", ch, NULL, NULL, TO_ROOM);
		act("Шар проходит сквозь твое тело и опускается $x.",
		    ch, NULL, NULL, TO_CHAR);
		return TRUE;
	    }

	    last_vict = ch;
	    act("Шар бьет $n3...упс!", ch, NULL, NULL, TO_ROOM);
	    send_to_char("Тебя ударяет твой собственный световой шар!\n\r", ch);
	    dam = dice(level, 6);
	    if (saves_spell(level, ch, DAM_LIGHTNING))
		dam /= 3;
	    damage(ch, ch, dam, sn, DAM_LIGHTNING, TRUE, NULL);
	    level -= 4;  /* decrement damage */
	    if (ch == NULL || ch->position == POS_DEAD)
		return TRUE;
	}
	/* now go back and find more targets */
    }

    return TRUE;
}


bool spell_change_sex(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn))
    {
	if (victim == ch)
	    send_to_char("У тебя уже изменен пол.\n\r", ch);
	else
	    act("Пол $N1 уже изменен.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    if (saves_spell(level , victim, DAM_OTHER))
    {
	send_to_char("Не получается.\n\r", ch);
	return TRUE;
    }	

    send_to_char("Ты чувствуешь себя как-то по другому.\n\r{x", victim);
    act("$n выглядит как-то по другому...{x", victim, NULL, NULL, TO_ROOM);
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 2 * level/3;
    af.location  = APPLY_SEX;
    do
    {
	af.modifier  = number_range(0, 2) - victim->sex;
    }
    while (af.modifier == 0);

    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    af.location  = APPLY_CON;
    af.modifier  = -(level + 9)/15;
    affect_to_char(victim, &af);

    return TRUE;
}

bool charm(CHAR_DATA *ch, CHAR_DATA *victim, int level, int sn)
{
    AFFECT_DATA af;

    /**
     * ch       - кастер
     * victiom  - цель, кого чармим
     */

    if (victim == ch){
	    send_to_char("Так ты нравишься себе даже лучше!\n\r", ch);
	    return FALSE;
    }

    if (IS_SET(victim->in_room->room_flags, ROOM_LAW)){
		send_to_char("Городской начальник не позволяет очаровывать в пределах города.\n\r", ch);
		return FALSE;
    }

    if (victim->position == POS_SLEEPING){
		send_to_char("Ты не можешь привлечь внимание жертвы.\n\r", ch);
		send_to_char("Твой сон прерывается на какое-то короткое время...\n\r", victim);
		return FALSE;
    }

    if (IS_AFFECTED(victim, AFF_CHARM) || IS_AFFECTED(ch, AFF_CHARM) ||	(IS_NPC(victim) && IS_SET(victim->act, ACT_MOUNT))){
		if (IS_SET(ch->act, PLR_SHOW_DAMAGE)) {
			send_to_char("Не получается. {D(#debug 1854){x\n\r", ch);
		} else {
			send_to_char("Не получается.\n\r", ch);
		}

		return FALSE;
    }


    /**
     * ЧАРОВ НЕ ОЧАРОВАТЬ???? ДА ЛАДНО!
     */
    if (!IS_NPC(victim)){
		if (IS_SET(ch->act, PLR_SHOW_DAMAGE)) {
			send_to_char("Не получается! {D(#debug 1868){x\n\r", ch);
		}else {
			send_to_char("Не получается!\n\r", ch);
		}
		return FALSE;
    }


    if (level < victim->level || check_immune(victim, DAM_CHARM) == 100
        || saves_spell(level, victim, DAM_CHARM)
	    || max_count_charmed(ch)
	    || victim->fighting != NULL
	    || victim->spec_fun == spec_lookup("spec_guard")
	    || victim->max_hit > 6000){
		if (IS_SET(ch->act, PLR_SHOW_DAMAGE)){
			send_to_char("Не получается! {D(#debug 1883){x\n\r", ch);
		} else {
			send_to_char("Не получается!\n\r", ch);
		}

		return TRUE;
    }



    if (victim->master) stop_follower(victim);

    add_follower(victim, ch);
    victim->leader = ch;
    af.where        = TO_AFFECTS;
    af.type         = sn;
    af.level        = level;
    af.duration     = number_fuzzy(IS_NPC(victim) ? level : level / 4);
    af.location     = 0;
    af.modifier     = 0;
    af.bitvector    = AFF_CHARM;
    af.caster_id    = get_id(ch);
    affect_to_char(victim, &af);
    act("Ты смотришь на $n3 с обожанием!{x", ch, NULL, victim, TO_VICT);

    if (ch != victim) act("$N преданно смотрит тебе в глаза.{x", ch, NULL, victim, TO_CHAR);

    if (IS_NPC(victim) && is_memory(victim, ch, MEM_HOSTILE)) mob_forget(victim, ch, MEM_HOSTILE);

    REMOVE_BIT(victim->act, ACT_AGGRESSIVE);

    check_killer(ch, victim);

    return TRUE;
}

bool spell_charm_person(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (is_lycanthrope(victim) || (IS_NPC(victim) && (IS_SET(victim->act, ACT_UNDEAD) || is_animal(victim)))){
		send_to_char("Нет, это существо очаровать не получится.\n\r", ch);
		return FALSE;
    }

    return charm(ch, victim, level, sn);
}



bool spell_chill_touch(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    static const int16_t dam_each[] =
    {
	0,
	0,  0,  6,  7,  8,   9, 12, 13, 13, 13,
	14, 14, 14, 15, 15,  15, 16, 16, 16, 17,
	17, 17, 18, 18, 18,  19, 19, 19, 20, 20,
	20, 21, 21, 21, 22,  22, 22, 23, 23, 23,
	24, 24, 24, 25, 25,  25, 26, 26, 26, 27
    };
    AFFECT_DATA af;
    int dam;

    if (target == TARGET_CHAR && vo != NULL)
    {
	victim = (CHAR_DATA *) vo;

	if (ch == victim)
	{
	    send_to_char("Ты мазохист?\n\r", ch);
	    return FALSE;
	}

	level  = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
	level  = UMAX(0, level);
	dam  = number_range(dam_each[level] / 2, dam_each[level] * 2);
	if (!saves_spell(level, victim, DAM_COLD))
	{
	    act("$n дрожит и синеет.{x", victim, NULL, NULL, TO_ROOM);
	    af.where     = TO_AFFECTS;
	    af.type      = sn;
	    af.level     = level;
	    af.duration  = 6;
	    af.location  = APPLY_STR;
	    af.modifier  = -1;
	    af.bitvector = 0;
	    af.caster_id = get_id(ch);
	    affect_join(victim, &af);
	}
	else
	{
	    dam /= 2;
	}

	damage(ch, victim, dam, sn, DAM_COLD, TRUE, NULL);
	return TRUE;
    }
    else if (target == TARGET_OBJ && vo != NULL)
    {
	obj = (OBJ_DATA *) vo;

	if (obj->item_type != ITEM_WEAPON)
	{
	    send_to_char("Ты можешь колдовать это заклинание только на "
			 "оружие.\n\r", ch);
	    return FALSE;
	}

	if (obj->level > ch->level)
	{
	    send_to_char("У тебя не хватает сил.\n\r", ch);
	    return FALSE;
	}

	if (IS_WEAPON_STAT(obj, WEAPON_POISON)
	    || IS_WEAPON_STAT(obj, WEAPON_FLAMING)
	    || IS_WEAPON_STAT(obj, WEAPON_SHOCKING))
	{
	    send_to_char("Ничего не выходит.\n\r", ch);
	    return FALSE;
	}

	if (IS_WEAPON_STAT(obj, WEAPON_FROST))
	{
	    act("На $p5 уже есть тончайшая корочка льда.",
		ch, obj, NULL, TO_CHAR);
	    return FALSE;
	}

	act("Ты прикасаешься рукой, и $p покрыва$rся тоненькой корочкой льда.{x",
	    ch, obj, NULL, TO_CHAR);
	act("$n прикасается рукой, и $p покрыва$rся тоненькой корочкой льда.{x",
	    ch, obj, NULL, TO_ROOM);

	obj->condition -= 5;
	check_condition(ch, obj);

	af.where = TO_WEAPON;
	af.type = sn;
	af.level = level;
	af.duration = level / 2;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = WEAPON_FROST;
	affect_to_obj(obj, &af);

	return TRUE;
    }
    else
    {
	send_to_char("Ты не видишь ничего (и никого) похожего здесь.\n\r", ch);
	return FALSE;
    }
}



bool spell_colour_spray(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    static const int16_t dam_each[] =
    {
	0,
	0,  0,  0,  0,  0,   0,  0,  0,  0,  0,
	30, 35, 40, 45, 50,  55, 55, 55, 56, 57,
	58, 58, 59, 60, 61,  61, 62, 63, 64, 64,
	65, 66, 67, 67, 68,  69, 70, 70, 71, 72,
	73, 73, 74, 75, 76,  76, 77, 78, 79, 79
    };
    int dam;

    level  = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
    level  = UMAX(0, level);
    dam    = number_range(dam_each[level] / 2,  dam_each[level] * 3);
    if (saves_spell(level, victim, DAM_LIGHT))
	dam /= 2;
    else
	spell_blindness(skill_lookup("blindness"),
			level/2, ch, (void *) victim, TARGET_CHAR);

    damage(ch, victim, dam, sn, DAM_LIGHT, TRUE, NULL);
    return TRUE;
}

char *light_table[] =
{
    "яркий светящийся шар",
    "светящийся череп",
    "светящийся зеленым светом кристалл",
    "яркое солнышко",
    NULL
};


bool spell_continual_light(int sn,
			   int level, CHAR_DATA *ch,
			   void *vo, int target)
{
    OBJ_DATA *light;
    char buf[MAX_STRING_LENGTH];
    int num_class; //номер класса кастера

    if (target_name[0] != '\0')  /* do a glow on some object */
    {
	light = get_obj_carry(ch, target_name, ch);

	if (light == NULL)
	{
	    send_to_char("Ты не видишь этого здесь.\n\r", ch);
	    return FALSE;
	}

	if (IS_OBJ_STAT(light, ITEM_GLOW))
	{
	    act("$p уже и так излуча$r мягкий свет.",
		ch, light, NULL, TO_CHAR);
	    return FALSE;
	}

	SET_BIT(light->extra_flags, ITEM_GLOW);
	act("$p загора$rся белым светом.{x",
	    ch, light, NULL, TO_ALL);
	return TRUE;
    }

    light = create_object(get_obj_index(OBJ_VNUM_LIGHT_BALL), 0);

    /*выбор по классу названия для создаваемого источника света, a la create_food()*/
    //проверка на класс num_class=?
    if (ch->classid == CLASS_MAGE) num_class = 0;
    else
	if (ch->classid == CLASS_NECROMANT) num_class = 1 ;
	else
	    if (ch->classid == CLASS_DRUID) num_class = 2;
	    else num_class = 3;

	    sprintf(buf, light->name, light_table[num_class]);
	    free_string(light->name);
	    light->name = str_dup(buf);

	    sprintf(buf, light->short_descr, light_table[num_class]);
	    free_string(light->short_descr);
	    light->short_descr = str_dup(buf);

	    sprintf(buf, light->description, capitalize(light_table[num_class]));
	    free_string(light->description);
	    light->description = str_dup(buf);

	    obj_to_room(light, ch->in_room);
	    act("$n крутит пальцами, и в воздухе перед н$T появляется $p.",
		ch, light, ch->sex == SEX_FEMALE ? "ей" : "им", TO_ROOM);
	    act("Ты крутишь пальцами, и в воздухе перед тобой появляется $p.",
		ch, light, NULL, TO_CHAR);
	    return TRUE;
}

bool spell_control_weather(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    int change = dice(level/2, 1);

    if (!str_cmp(target_name, "better") || !str_cmp(target_name, "лучше"))
    {
	weather_info.change += change/2;
	weather_info.mmhg += change;
	weather_update(FALSE);
    }
    else if (!str_cmp(target_name, "worse") || !str_cmp(target_name, "хуже"))
    {
	weather_info.change -= change/2;
	weather_info.mmhg -= change;
	weather_update(FALSE);
    }
    else
	send_to_char ("Ты хочешь улучшить или ухудшить погоду?\n\r", ch);

    send_to_char("Ok.\n\r", ch);
    do_weather(ch, "");
    return TRUE;
}

struct food_type
{
    char 		*name;
    int16_t 	class_food[MAX_CLASS + 1];
};

struct food_type food_table[] =
{
    {"магический гриб", 	{CLASS_MAGE, CLASS_CLERIC, CLASS_THIEF, CLASS_WARRIOR, CLASS_NECROMANT, CLASS_PALADIN, CLASS_NAZGUL, CLASS_DRUID, CLASS_RANGER, CLASS_VAMPIRE, CLASS_ALCHEMIST, -1}},
    {"магический пирог", 	{CLASS_MAGE, CLASS_CLERIC, CLASS_THIEF, CLASS_WARRIOR, CLASS_NECROMANT, CLASS_PALADIN, CLASS_NAZGUL, CLASS_VAMPIRE, CLASS_ALCHEMIST, -1}},
    {"магический орех", 	{CLASS_MAGE, CLASS_CLERIC, CLASS_DRUID, CLASS_RANGER, CLASS_ALCHEMIST, -1}},
    {"волшебная ягода", 	{CLASS_MAGE, CLASS_CLERIC, CLASS_DRUID, CLASS_RANGER, CLASS_ALCHEMIST, -1}},
    {"магическое пирожное", 	{CLASS_MAGE, CLASS_CLERIC, CLASS_THIEF, CLASS_WARRIOR, CLASS_NECROMANT, CLASS_PALADIN, CLASS_NAZGUL, CLASS_VAMPIRE, CLASS_ALCHEMIST, -1}},
    {"мистическое печенье", 	{CLASS_MAGE, CLASS_CLERIC, CLASS_THIEF, CLASS_WARRIOR, CLASS_NECROMANT, CLASS_PALADIN, CLASS_NAZGUL, CLASS_VAMPIRE, CLASS_ALCHEMIST, -1}},
    {"волшебное мороженое", 	{CLASS_MAGE, CLASS_CLERIC, CLASS_THIEF, CLASS_WARRIOR, CLASS_NECROMANT, CLASS_PALADIN, CLASS_NAZGUL, CLASS_VAMPIRE, CLASS_ALCHEMIST, -1}},
    {"волшебная котлета", 	{CLASS_MAGE, CLASS_CLERIC, CLASS_THIEF, CLASS_WARRIOR, CLASS_NECROMANT, CLASS_PALADIN, CLASS_NAZGUL, CLASS_VAMPIRE, CLASS_ALCHEMIST, -1}},
    {"кольцо магической колбасы",{CLASS_MAGE, CLASS_CLERIC, CLASS_THIEF, CLASS_WARRIOR, CLASS_NECROMANT, CLASS_PALADIN, CLASS_NAZGUL, CLASS_VAMPIRE, CLASS_ALCHEMIST, -1}},
    {"колдовской бутерброд", 	{CLASS_MAGE, CLASS_CLERIC, CLASS_THIEF, CLASS_WARRIOR, CLASS_NECROMANT, CLASS_PALADIN, CLASS_NAZGUL, CLASS_VAMPIRE, CLASS_ALCHEMIST, -1}},
    {"магический кусок сала", 	{CLASS_MAGE, CLASS_CLERIC, CLASS_THIEF, CLASS_WARRIOR, CLASS_NECROMANT, CLASS_PALADIN, CLASS_NAZGUL, CLASS_VAMPIRE, CLASS_ALCHEMIST, -1}},
    {"магическое яблоко", 	{CLASS_DRUID, CLASS_RANGER, -1}},
    {"волшебная груша", 	{CLASS_DRUID, CLASS_RANGER, -1}},
    {"сказочный лист салата",	{CLASS_DRUID, CLASS_RANGER, -1}},
    {NULL, {-1}}
};


bool spell_create_food(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *mushroom;
    char buf[MAX_STRING_LENGTH];
    int i, j, count, sel = 0;

    mushroom = create_object(get_obj_index(OBJ_VNUM_MUSHROOM), 0);
    mushroom->value[0] = level / 2;
    mushroom->value[1] = level;

    for (i = 0, count = 0; food_table[i].name; i++)
    {
	if (!IS_NPC(ch))
	{
	    bool flag = FALSE;

	    for(j = 0; j < MAX_CLASS && food_table[i].class_food[j] >=0; j++)
		if (ch->classid == food_table[i].class_food[j])
		{
		    flag = TRUE;
		    break;
		}

	    if (!flag)
		continue ;
	}

	if (number_range(0, count++) == 0)
	    sel = i;
    }

    sprintf(buf, mushroom->name, food_table[sel].name);
    free_string(mushroom->name);
    mushroom->name = str_dup(buf);

    sprintf(buf, mushroom->short_descr, food_table[sel].name);
    free_string(mushroom->short_descr);
    mushroom->short_descr = str_dup(buf);

    sprintf(buf, mushroom->description, capitalize(food_table[sel].name));
    free_string(mushroom->description);
    mushroom->description = str_dup(buf);

    obj_to_room(mushroom, ch->in_room);

    act("$T появляется $p.",
	ch, mushroom, number_bits(1) == 0 ? "Неожиданно" : "Внезапно", TO_ALL);

    if (!OBJcast && number_range(0, get_skill(ch, sn)) == 0)
	mushroom->value[3] = number_range(level/5, level);

    return TRUE;
}

bool spell_create_rose(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *rose;
    rose = create_object(get_obj_index(OBJ_VNUM_ROSE), 0);
    act("Едва заметное движение рук $n1, и перед тобой из земли появляется росток... Роза... Блеск клинка...",
	ch, rose, NULL, TO_ROOM);
    act("И над землей появляется прелестный цветок... Роза....", ch, rose, NULL, TO_ROOM);

    send_to_char("Едва заметное движение твоих рук, и перед тобой из земли появляется росток... Роза... Блеск клинка...\n\r", ch);
    send_to_char("И над землей появляется прелестный цветок... Роза...\n\r", ch);

    obj_to_room(rose, ch->in_room);
    return TRUE;
}

bool spell_create_spring(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *spring;

    if (SECTOR(ch) == SECT_WATER_SWIM || SECTOR(ch) == SECT_WATER_NOSWIM)
    {
	send_to_char("Здесь и так полно воды.\n\r", ch);
	return FALSE;
    }
    else
    {
	spring = create_object(get_obj_index(OBJ_VNUM_SPRING), 0);
	spring->timer = level;
	obj_to_room(spring, ch->in_room);
	act("$p пробива$rся из-под земли.",  ch, spring, NULL, TO_ALL);
    }
    return TRUE;
}



bool spell_create_water(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    int water, i, sel = LIQ_WATER;

    if (obj->item_type != ITEM_DRINK_CON)
    {
	send_to_char("Это не контейнер для жидкостей.\n\r", ch);
	return FALSE;
    }

    if (obj->value[1] != 0)
    {
	send_to_char("Здесь уже есть какая-то жидкость.\n\r", ch);
	return FALSE;
    }

    if (number_bits(3) > 4)
    {
	for (i = 0; liq_table[i].liq_name; i++)
	    if (number_range(0, i) == 0)
		sel = i;
    }

    water = URANGE(1,
		   level * (weather_info.sky >= SKY_RAINING ? 4 : 2),
		   obj->value[0]
		  );

    obj->value[2] = sel;

    obj->value[1] += water;
    act("В $p5 начинает плескаться $T.", ch, obj, liq_table[sel].liq_rname, TO_CHAR);

    return TRUE;
}



bool spell_cure_blindness(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (!is_affected(victim, gsn_blindness))
    {
	if (victim == ch)
	    act("Ты не слеп$t.", ch, SEX_ENDING(ch), NULL, TO_CHAR);
	else
	    act("$N не выглядит слеп$t.",
		ch, SEX_END_ADJ(victim), victim, TO_CHAR);
	return FALSE;
    }

    if (check_dispel(level, victim, gsn_blindness))
    {
	send_to_char("Зрение к тебе вернулось!\n\r{x", victim);
	act("$n теперь не слеп$t.{x", victim, SEX_ENDING(victim), NULL, TO_ROOM);
    }
    else
	send_to_char("Не получилось.\n\r", ch);

    return TRUE;
}



bool spell_cure_critical(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int heal;

    if (is_affected(victim, gsn_harm)
	|| is_affected(victim, gsn_critical_harm))
    {
	if (ch == victim)
	    send_to_char("Ты не можешь вылечиться.\n\r", ch);
	else
	    send_to_char("Ты не можешь его вылечить.\n\r", ch);
	return FALSE;
    }

    if (IS_VAMPIRE(victim))
    {
	send_to_char("Твое заклинание не произвело никакого эффекта.\n\r", ch);
	return FALSE;
    }
    heal = dice(3, 8) + level;
    victim->hit = UMIN(victim->hit + heal, victim->max_hit);
    update_pos(victim);
    send_to_char("Ты чувствуешь себя лучше!\n\r", victim);
    if (ch != victim)
	send_to_char("Ok.\n\r", ch);
    return TRUE;
}

/* RT added to cure plague */
bool spell_cure_disease(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (!is_affected(victim, gsn_plague))
    {
	if (victim == ch)
	    send_to_char("Ты не выглядишь болезненно.\n\r", ch);
	else
	    act("$N не выглядит болезненно.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    if (check_dispel(level, victim, gsn_plague))
    {
		send_to_char("Ты больше не болен чумой.\n\r{x", victim);
		act("$n перестает мучаться от болезней.{x", victim, NULL, NULL, TO_ROOM);
    }
    else
	send_to_char("Не получается.\n\r", ch);

    return TRUE;
}



bool spell_cure_light(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int heal;

    if (is_affected(victim, gsn_harm)||
	is_affected(victim, gsn_critical_harm)||
	is_affected(victim, gsn_serious_harm)||
	is_affected(victim, gsn_lighting_harm) )
    {
	if (ch==victim)
	    send_to_char("Ты не можешь вылечиться.\n\r", ch);
	else
	    send_to_char("Ты не можешь его вылечить.\n\r", ch);
	return FALSE;
    }

    if (IS_VAMPIRE(victim))
    {
	send_to_char("Твое заклинание не произвело никакого эффекта.\n\r", ch);
	return FALSE;
    }
    heal = dice(2, 8) + level / 2;
    victim->hit = UMIN(victim->hit + heal, victim->max_hit);
    update_pos(victim);
    send_to_char("Ты чувствуешь себя получше!\n\r", victim);
    if (ch != victim)
	send_to_char("Ok.\n\r", ch);
    return TRUE;
}



bool spell_cure_poison(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (!is_affected(victim, gsn_poison))
    {
	if (victim == ch)
	    printf_to_char("Ты не отравлен%s.\n\r", ch, SEX_ENDING(ch));
	else
	    act("$N не выглядит отравленн$t.", ch, SEX_END_ADJ(victim), victim, TO_CHAR);
	return FALSE;
    }

    if (check_dispel(level, victim, gsn_poison))
    {
		send_to_char("Тепло пронизывает твое тело.\n\r{x", victim);
		act("$n выглядит намного лучше.{x", victim, NULL, NULL, TO_ROOM);
    }
    else
		send_to_char("Не получилось.\n\r", ch);

    return TRUE;
}

bool spell_cure_serious(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int heal;

    if (is_affected(victim, gsn_harm)||
	is_affected(victim, gsn_critical_harm)||
	is_affected(victim, gsn_serious_harm))
    {
	if (ch==victim)
	    send_to_char("Ты не можешь вылечиться.\n\r", ch);
	else
	    send_to_char("Ты не можешь его вылечить.\n\r", ch);
	return FALSE;
    }

    if (IS_VAMPIRE(victim))
    {
	send_to_char("Твое заклинание не произвело никакого эффекта.\n\r", ch);
	return FALSE;
    }
    heal = dice(3, 8) + level/2 ;
    victim->hit = UMIN(victim->hit + heal, victim->max_hit);
    update_pos(victim);
    send_to_char("Ты чувствуешь себя лучше!\n\r", victim);
    if (ch != victim)
	send_to_char("Ok.\n\r", ch);
    return TRUE;
}



bool spell_curse(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA af;
    char buf[MAX_STRING_LENGTH];

    /* deal with the object case first */
    if (target == TARGET_OBJ)
    {
		obj = (OBJ_DATA *) vo;
		if (IS_OBJ_STAT(obj, ITEM_EVIL))
		{
	    	act("В $p5 уже и так достаточно злобы.", ch, obj, NULL, TO_CHAR);
	    	return FALSE;
		}

		if (IS_OBJ_STAT(obj, ITEM_BLESS))
		{
	    	AFFECT_DATA *paf;

	    	paf = affect_find(obj->affected, skill_lookup("bless"));
	    	if (!saves_dispel(level, paf != NULL ? paf->level : obj->level, 0, NULL))
	    	{
				if (paf != NULL)
		    		affect_remove_obj(obj, paf);
				act("$p окружа$rся красной аурой.{x",
		    		ch, obj, NULL, TO_ALL);
				REMOVE_BIT(obj->extra_flags, ITEM_BLESS);
				return TRUE;
	    	}
	    	else
	    	{
				act("Святая аура на $p4 слишком мощна для тебя.",
		    		ch, obj, NULL, TO_CHAR);
				return FALSE;
	    	}
		}

		af.where        = TO_OBJECT;
		af.type         = sn;
		af.level        = level;
		af.duration     = 2 * level;
		af.location     = APPLY_SAVES;
		af.modifier     = +1;
		af.bitvector    = ITEM_EVIL;
		affect_to_obj(obj, &af);

		act("$p окружа$rся злой аурой.{x",
	    	ch, obj, NULL, TO_ALL);

		if (obj->wear_loc != WEAR_NONE)
	    	ch->saving_throw += 1;
		return TRUE;
    	}

    /* character curses */
    victim = (CHAR_DATA *) vo;

    if (is_affected(victim, gsn_st_aura))
    {
		sprintf(buf, "Святая аура $N1 мешает тебе проклясть его.");
		act(buf, ch, NULL, victim, TO_CHAR);
		return FALSE;
    }

    if (IS_VAMPIRE(victim))
    {
		send_to_char("Твое заклинание не произвело никакого эффекта.\n\r", ch);
		return FALSE;
    }

    if (IS_AFFECTED(victim, AFF_CURSE))
    {
		if (ch != victim)
		{
	    	sprintf(buf, "$N уже проклят%s.", SEX_ENDING(victim));
	    	act(buf, ch, NULL, victim, TO_CHAR);
		}
		return FALSE;
    }

    if (saves_spell(level, victim, DAM_NEGATIVE))
    {
		send_to_char("Не выходит!\n\r", ch);
		return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 1 + level / 8;
    af.location  = APPLY_HITROLL;
    af.modifier  = -1 * (level / 8);
    af.bitvector = AFF_CURSE;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = level / 8;
    affect_to_char(victim, &af);

    af.where = TO_RESIST;
    af.location = APPLY_NONE;
    af.modifier = -level / 10;
    af.bitvector = DAM_NEGATIVE;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    act("Ты чувствуешь себя нечист$t.{x", victim, SEX_END_ADJ(victim), NULL, TO_CHAR);
    if (ch != victim)
		act("$N выглядит очень неприятно.{x", ch, NULL, victim, TO_CHAR);
    return TRUE;
}

/* RT replacement demonfire spell */

bool spell_demonfire(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    if (!IS_NPC(ch) && IS_GOOD(ch))
    {
	victim = ch;
	send_to_char("Демоны кружатся вокруг тебя!\n\r", ch);
    }

    ch->alignment = UMAX(-1000, ch->alignment - 50);

    if (victim != ch)
    {
	act("$n вызывает четырех демонов Ада над $N4!",
	    ch, NULL, victim, TO_NOTVICT);
	act("$n вызывает над твоей головой четырех демонов Ада!",
	    ch, NULL, victim, TO_VICT);
	send_to_char("Ты вызываешь четырех демонов Ада!\n\r", ch);
    }
    dam = dice(level, 10);
    if (saves_spell(level, victim, DAM_NEGATIVE))
	dam /= 2;

    spell_curse(gsn_curse, 3 * level / 4, ch, (void *) victim, TARGET_CHAR);
    damage(ch, victim, dam, sn, DAM_NEGATIVE , TRUE, NULL);

    return TRUE;
}

bool spell_detect_evil(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_replace(sn, level, victim))
	affect_strip(victim, sn);

    if (IS_AFFECTED(victim, AFF_DETECT_EVIL))
    {
	if (victim == ch)
	    send_to_char("Ты уже чувствуешь зло.\n\r", ch);
	else
	    act("$N уже чувствует зло.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level   = level;
    af.duration  = level/2;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_EVIL;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    send_to_char("Ты чувствуешь покалывание в глазах.\n\r{x", victim);
    if (ch != victim)
	send_to_char("Ok.\n\r", ch);
    return TRUE;
}


bool spell_detect_good(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_replace(sn, level, victim))
	affect_strip(victim, sn);

    if (IS_AFFECTED(victim, AFF_DETECT_GOOD))
    {
	if (victim == ch)
	    send_to_char("Ты уже чувствуешь добро.\n\r", ch);
	else
	    act("$N уже чувствует добро.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/2;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_GOOD;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    send_to_char("Ты чувствуешь покалывание в глазах.\n\r{x", victim);
    if (ch != victim)
	send_to_char("Ok.\n\r", ch);
    return TRUE;
}



bool spell_detect_hidden(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_replace(sn, level, victim))
	affect_strip(victim, sn);

    if (IS_AFFECTED(victim, AFF_DETECT_HIDDEN))
    {
	if (victim == ch)
	    send_to_char("Твое внимание на максимуме. \n\r", ch);
	else
	    act("$N уже замечает скрытые формы жизни.",
		ch, NULL, victim, TO_CHAR);
	return FALSE;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/3;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_DETECT_HIDDEN;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    send_to_char("Твоя бдительность повышается.\n\r{x", victim);
    if (ch != victim)
	send_to_char("Ok.\n\r", ch);
    return TRUE;
}



bool spell_detect_invis(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_replace(sn, level, victim))
	affect_strip(victim, sn);

    if (IS_AFFECTED(victim, AFF_DETECT_INVIS))
    {
	if (victim == ch)
	    send_to_char("Ты уже видишь невидимое.\n\r", ch);
	else
	    act("$N уже может видеть невидимые вещи.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/3;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_INVIS;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    send_to_char("Ты чувствуешь покалывание в глазах.\n\r{x", victim);
    if (ch != victim)
	send_to_char("Ok.\n\r", ch);
    return TRUE;
}



bool spell_detect_magic(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_replace(sn, level, victim))
	affect_strip(victim, sn);

    if (IS_AFFECTED(victim, AFF_DETECT_MAGIC))
    {
	if (victim == ch)
	    send_to_char("Ты уже можешь видеть магические ауры.\n\r", ch);
	else
	    act("$N уже может обнаруживать магию.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level   = level;
    af.duration  = level/2;
    af.modifier  = -1;
    af.location  = APPLY_SAVES;
    af.bitvector = AFF_DETECT_MAGIC;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    send_to_char("Ты чувствуешь покалывание в глазах.\n\r{x", victim);
    if (ch != victim)
	send_to_char("Ok.\n\r", ch);
    return TRUE;
}



bool spell_detect_poison(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;

    if (obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_FOOD)
    {
	if (obj->value[3] != 0)
	    send_to_char("Ты чувствуешь пары яда.\n\r", ch);
	else
	    send_to_char("Это выглядит восхитительно.\n\r", ch);
    }
    else
    {
	send_to_char("Это не выглядит отравленным.\n\r", ch);
    }

    return TRUE;
}



bool spell_dispel_evil(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    if (!IS_NPC(ch) && IS_EVIL(ch))
	victim = ch;

    if (IS_GOOD(victim))
    {
	act("Боги защищают $N3.", ch, NULL, victim, TO_ROOM);
	return FALSE;
    }

    if (IS_NEUTRAL(victim))
    {
	act("$N плевать хотел$t на твое заклинание.", ch, SEX_ENDING(victim), victim, TO_CHAR);
	return FALSE;
    }

    if (victim->hit > (ch->level * 4))
	dam = dice(level, 4);
    else
	dam = UMAX(victim->hit, dice(level, 4));

    if (saves_spell(level, victim, DAM_HOLY))
	dam /= 2;
    damage(ch, victim, dam, sn, DAM_HOLY , TRUE, NULL);
    victim->alignment += 2*(ch->level);
    return TRUE;
}


bool spell_dispel_good(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    if (!IS_NPC(ch) && IS_GOOD(ch))
	victim = ch;

    if (IS_EVIL(victim))
    {
	act("$N защищается злом.", ch, NULL, victim, TO_ROOM);
	return FALSE;
    }

    if (IS_NEUTRAL(victim))
    {
	act("$N плевать хотел$t на твое заклинание.", ch, SEX_ENDING(victim), victim, TO_CHAR);
	return FALSE;
    }

    if (victim->hit > (ch->level * 4))
	dam = dice(level, 4);
    else
	dam = UMAX(victim->hit, dice(level, 4));
    if (saves_spell(level, victim, DAM_NEGATIVE))
	dam /= 2;
    damage(ch, victim, dam, sn, DAM_NEGATIVE , TRUE, NULL);
    victim->alignment -= 2*(ch->level);
    return TRUE;
}


/* modified for enhanced use */
/* КАСТ ИЗГНАТЬ МАГИЮ */
bool spell_dispel_magic(int sn, int level, CHAR_DATA *ch, void *vo, int target){
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    bool found = FALSE;
    int i;

    if (saves_spell(level, victim, DAM_OTHER)){
        if (sn > 0){
            send_to_char("Ты чувствуешь кратковременное покалывание.\n\r", victim);
            send_to_char("Не выходит.\n\r", ch);
        }
        return TRUE;
    }

    /* begin running through the spells */
    for (i = 0; cancel_spell[i].spell != NULL; i++)
	if (check_dispel(level, victim, skill_lookup(cancel_spell[i].spell))){
	    found = TRUE;
	    if (!IS_NULLSTR(cancel_spell[i].msg))
		act(cancel_spell[i].msg, victim, NULL, NULL, TO_ROOM);
	}


    if (sn > 0){
        if (found){
            send_to_char("Ok.\n\r", ch);
        } else {
            send_to_char("Не получается.\n\r", ch);
        }
    }

    return TRUE;
}

bool spell_earthquake(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;

    send_to_char("Земля дрожит под твоими ногами!\n\r", ch);
    act("$n заставляет землю дрожать и трястись.", ch, NULL, NULL, TO_ROOM);

    LIST_FOREACH_SAFE(vch, &char_list, link, vch_next)
    {
	if (vch->in_room == NULL || is_same_group(ch, vch))
	    continue;
	if (vch->in_room == ch->in_room)
	{
	    if (!is_safe_spell(ch, vch, TRUE))
	    {
		if (IS_AFFECTED(vch, AFF_FLYING))
		{
//		    damage(ch, vch, 0, sn, DAM_BASH, TRUE, NULL);
		}
		else
		    damage(ch, vch, level + dice(2, 8), sn, DAM_BASH, TRUE, NULL);
	    }
	    continue;
	}

	if (vch->in_room->area == ch->in_room->area)
	    send_to_char("Земля дрожит и трясется.\n\r", vch);
    }
    return TRUE;
}

#define MAX_ENARMOR    6

static int enarmor[MAX_ENARMOR][2] =
{
{APPLY_MANA,    5}, {APPLY_HIT,     5}, {APPLY_MOVE,    5},
{APPLY_AC,     -2}, {APPLY_HITROLL, 1}, {APPLY_DAMROLL, 1}
};

bool spell_enchant_armor(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    AFFECT_DATA *paf;
    int result, fail;
    int bonus, i, added;
    int found = 0;
    bool new_spell = TRUE;

    if (obj->item_type != ITEM_ARMOR)
    {
	send_to_char("Это не броня.\n\r", ch);
	return FALSE;
    }

    if (obj->wear_loc != -1)
    {
	send_to_char("Ты должен это сначала взять.\n\r", ch);
	return FALSE;
    }

    if (obj->pIndexData->edited)
    {
	send_to_char("Подожди немного, попробуй улучшить эту вещь минут через пять, возможно у тебя это получится.\n\r", ch);
	return FALSE;
    }

    if (obj->owner != NULL)
    {
	send_to_char("Эта вещь персонализована. Ты не можешь ее улучшить.\n\r", ch);
	return FALSE;
    }

    /* this means they have no bonus */
    fail = 33;  /* base 33% chance of failure */
    bonus = number_range(0, MAX_ENARMOR - 1);

    /* find the bonuses */
    for (paf = obj->affected; paf != NULL; paf = paf->next)
    {
	for  (i = 0; i != MAX_ENARMOR; i++)
	{
	    if (paf->location == enarmor[i][0] && paf->type != 0)
		found += (paf->modifier / enarmor[i][1]);
	    if (paf->location == enarmor[bonus][0] && paf->type == sn)
			new_spell = FALSE;
	}
    }

    fail += found * 20;

    /* apply other modifiers */
    fail -= (level + get_skill(ch,sn) / 5) / 1.5;

    if (IS_OBJ_STAT(obj, ITEM_BLESS))
	fail -= 15;
    if (IS_OBJ_STAT(obj, ITEM_GLOW))
	fail -= 5;

    fail = URANGE(5, fail, 85);

    result = number_percent();

    /* the moment of truth */
    if (result < (fail / 5))  /* item destroyed */
    {
	act("$p вспыхива$r... и испаря$rся!{x",
	    ch, obj, NULL, TO_ALL);
	extract_obj(obj, TRUE, TRUE);
	return TRUE;
    }

    if (result < (fail / 3)) /* item disenchanted */
    {
	AFFECT_DATA *paf_next;

	act("$p мерца$r, затем исчеза$r... Упс. Но снова появля$rся.{x",
	    ch, obj, NULL, TO_ALL);
	obj->enchanted = TRUE;

	/* remove all affects */
	for (paf = obj->affected; paf != NULL; paf = paf_next)
	{
	    paf_next = paf->next;
	    affect_remove_obj(obj, paf);
	}

	if (obj->affected)
	{
	    obj->affected->next = NULL;
	    obj->affected = NULL;
	}

	/* clear all flags */
	REMOVE_BIT(obj->extra_flags, ITEM_HAS_SOCKET);
	REMOVE_BIT(obj->extra_flags, ITEM_GLOW);
	REMOVE_BIT(obj->extra_flags, ITEM_MAGIC);
	REMOVE_BIT(obj->extra_flags, ITEM_BURN_PROOF);
	REMOVE_BIT(obj->extra_flags, ITEM_BLESS);
	REMOVE_BIT(obj->extra_flags, ITEM_HUM);
	REMOVE_BIT(obj->extra_flags, ITEM_INVIS);
	return TRUE;
    }

    if (result <= fail)  /* failed, no bad result */
    {
	send_to_char("Ничего не произошло.\n\r", ch);
	return TRUE;
    }

    /* okay, move all the old flags into new vectors if we have to */
    affect_enchant(obj);

    REMOVE_BIT(obj->extra_flags, ITEM_HAS_SOCKET);

    if (result <= (95 - level/4))  /* success! */
    {
	act("$p окружа$rся золотой аурой.{x", ch, obj, NULL, TO_ALL);
	SET_BIT(obj->extra_flags, ITEM_MAGIC);
	added = 1;
    }

    else  /* exceptional enchant */
    {
	act("$p мерца$r чистым золотом!{x", ch, obj, NULL, TO_ALL);
	SET_BIT(obj->extra_flags, ITEM_MAGIC);
	SET_BIT(obj->extra_flags, ITEM_GLOW);
	added = 2;
    }

    /* now add the enchantments */

    obj->level = UMIN(MAX_LEVEL, obj->level + 1);

    if (!new_spell)
    {
	for (paf = obj->affected; paf != NULL; paf = paf->next)
	{
	    if (paf->location == enarmor[bonus][0] && paf->type != 0)
	    {
		paf->type = sn;
		paf->modifier += added * enarmor[bonus][1];
		paf->level = UMAX(paf->level, level);		
	    }
	}
    }
    else /* add a new affect */
    {
	paf = new_affect();

	paf->where  = TO_OBJECT;
	paf->type  = sn;
	paf->level  = level;
	paf->duration  = -1;
	paf->location  = enarmor[bonus][0];
	paf->modifier  =  added * enarmor[bonus][1];
	paf->bitvector  = 0;
	affect_to_obj(obj, paf);

	free_affect(paf);
    }

    return TRUE;
}

bool spell_enchant_weapon(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    AFFECT_DATA *paf;
    int result, fail;
    int hit_bonus, dam_bonus, added;
    bool hit_found = FALSE, dam_found = FALSE;

    if (obj->item_type != ITEM_WEAPON)
    {
	send_to_char("Это не оружие.\n\r", ch);
	return FALSE;
    }

    if (obj->wear_loc != -1)
    {
	send_to_char("Ты должен это нести.\n\r", ch);
	return FALSE;
    }

    if (obj->pIndexData->edited)
    {
	send_to_char("Подожди немного, попробуй улучшить эту вещь минут через пять, возможно у тебя это получится.\n\r", ch);
	return FALSE;
    }

    /* this means they have no bonus */
    hit_bonus = 0;
    dam_bonus = 0;
    fail = 25;  /* base 25% chance of failure */

    /* find the bonuses */

    if (!obj->enchanted)
	for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
	{
	    if (paf->location == APPLY_HITROLL)
	    {
		hit_bonus = paf->modifier;
		hit_found = TRUE;
		fail += 2 * (hit_bonus * hit_bonus);
	    }

	    else if (paf->location == APPLY_DAMROLL)
	    {
		dam_bonus = paf->modifier;
		dam_found = TRUE;
		fail += 2 * (dam_bonus * dam_bonus);
	    }

	    else  /* things get a little harder */
		fail += 25;
	}

    for (paf = obj->affected; paf != NULL; paf = paf->next)
    {
	if (paf->location == APPLY_HITROLL)
	{
	    hit_bonus = paf->modifier;
	    hit_found = TRUE;
	    fail += 2 * (hit_bonus * hit_bonus);
	}

	else if (paf->location == APPLY_DAMROLL)
	{
	    dam_bonus = paf->modifier;
	    dam_found = TRUE;
	    fail += 2 * (dam_bonus * dam_bonus);
	}

	else /* things get a little harder */
	    fail += 25;
    }

    /* apply other modifiers */
    fail -= 3 * level/2;

    if (IS_OBJ_STAT(obj, ITEM_BLESS))
	fail -= 15;
    if (IS_OBJ_STAT(obj, ITEM_GLOW))
	fail -= 5;

    fail = URANGE(5, fail, 95);

    if (obj->enchanted)
	fail *= 4;

    result = number_percent();

    /* the moment of truth */
    if (result < (fail / 4))  /* item destroyed */
    {
	act("$p вспыхива$r и взрыва$rся!{x",
	    ch, obj, NULL, TO_ALL);
	extract_obj(obj, TRUE, TRUE);
	return TRUE;
    }

    if (result < (fail / 2)) /* item disenchanted */
    {
	AFFECT_DATA *paf_next;

	act("$p вспыхива$r и исчеза$r... Упс. Но снова появля$rся.{x",
	    ch, obj, NULL, TO_CHAR);
	act("$p вспыхива$r и исчеза$r. Но снова появля$rся.{x",
	    ch, obj, NULL, TO_ROOM);
	obj->enchanted = TRUE;

	/* remove all affects */
	for (paf = obj->affected; paf != NULL; paf = paf_next)
	{
	    paf_next = paf->next;
	    affect_remove_obj(obj, paf);
	}

	if (obj->affected)
	{
	    obj->affected->next = NULL;
	    obj->affected = NULL;
	}

	/* clear all flags */
	REMOVE_BIT(obj->extra_flags, ITEM_HAS_SOCKET);
	REMOVE_BIT(obj->extra_flags, ITEM_GLOW);
	REMOVE_BIT(obj->extra_flags, ITEM_MAGIC);
	REMOVE_BIT(obj->extra_flags, ITEM_BURN_PROOF);
	REMOVE_BIT(obj->extra_flags, ITEM_BLESS);
	REMOVE_BIT(obj->extra_flags, ITEM_HUM);
	REMOVE_BIT(obj->extra_flags, ITEM_INVIS);
	return TRUE;
    }

    if (result <= fail)  /* failed, no bad result */
    {
	send_to_char("Ничего не произошло.\n\r", ch);
	return TRUE;
    }

    /* okay, move all the old flags into new vectors if we have to */
    affect_enchant(obj);

    REMOVE_BIT(obj->extra_flags, ITEM_HAS_SOCKET);

    if (result <= (100 - level/5))  /* success! */
    {
	act("$p загора$rся синим цветом.{x",
	    ch, obj, NULL, TO_ALL);
	SET_BIT(obj->extra_flags, ITEM_MAGIC);
	added = 1;
    }

    else  /* exceptional enchant */
    {
	act("$p загора$rся кристально чистым синим цветом!{x",
	    ch, obj, NULL, TO_ALL);
	SET_BIT(obj->extra_flags, ITEM_MAGIC);
	SET_BIT(obj->extra_flags, ITEM_GLOW);
	added = 2;
    }

    /* now add the enchantments */

    obj->level = UMIN(MAX_LEVEL, obj->level + 1);

    if (dam_found)
    {
	for (paf = obj->affected; paf != NULL; paf = paf->next)
	{
	    if (paf->location == APPLY_DAMROLL)
	    {
		paf->type = sn;
		paf->modifier += added;
		paf->level = UMAX(paf->level, level);
		if (paf->modifier > 4)
		    SET_BIT(obj->extra_flags, ITEM_HUM);
	    }
	}
    }
    else /* add a new affect */
    {
	paf = new_affect();

	paf->where  = TO_OBJECT;
	paf->type  = sn;
	paf->level  = level;
	paf->duration  = -1;
	paf->location  = APPLY_DAMROLL;
	paf->modifier  =  added;
	paf->bitvector  = 0;

	affect_to_obj(obj, paf);

	free_affect(paf);
    }

    if (hit_found)
    {
	for (paf = obj->affected; paf != NULL; paf = paf->next)
	{
	    if (paf->location == APPLY_HITROLL)
	    {
		paf->type = sn;
		paf->modifier += added;
		paf->level = UMAX(paf->level, level);
		if (paf->modifier > 4)
		    SET_BIT(obj->extra_flags, ITEM_HUM);
	    }
	}
    }
    else /* add a new affect */
    {
	paf = new_affect();

	paf->type       = sn;
	paf->level      = level;
	paf->duration   = -1;
	paf->location   = APPLY_HITROLL;
	paf->modifier   =  added;
	paf->bitvector  = 0;

	affect_to_obj(obj, paf);

	free_affect(paf);
    }

    return TRUE;
}



/*
 * Drain XP, MANA, HP.
 * Caster gains HP.
 */
bool spell_energy_drain(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    if (IS_VAMPIRE(victim))
    {
	send_to_char("Твое заклинание не произвело никакого эффекта.\n\r", ch);
	return FALSE;
    }
    if (saves_spell(level, victim, DAM_NEGATIVE))
    {
	send_to_char("Ты чувствуешь кратковременный холод.\n\r", victim);
	send_to_char("Твое заклинание не произвело никакого эффекта.\n\r", ch);
	return TRUE;
    }

    if (victim != ch)
	ch->alignment = UMAX(-1000, ch->alignment - 1);

    victim->mana  /= 2;
    victim->move  /= 2;
    dam       = dice(1, level);
    ch->hit    += dam;

    send_to_char("Ты чувствуешь, как твоя жизнь утекает!\n\r", victim);
    send_to_char("Вау....что за удовольствие!\n\r", ch);
    damage(ch, victim, dam, sn, DAM_NEGATIVE , TRUE, NULL);

    return TRUE;
}



bool spell_fireball(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    static const int16_t dam_each[] =
    {
	0,
	0,   0,   0,   0,   0,    0,   0,   0,   0,   0,
	0,   0,   0,   0,  30,   35,  40,  45,  50,  55,
	60,  65,  70,  75,  80,   82,  84,  86,  88,  90,
	92,  94,  96,  98, 100,  102, 104, 106, 108, 110,
	112, 114, 116, 118, 120,  122, 124, 126, 128, 130
    };
    int dam;

    level  = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
    level  = UMAX(0, level);
    dam    = number_range(dam_each[level] / 2, dam_each[level] * 3);
    if (saves_spell(level, victim, DAM_FIRE))
	dam /= 2;
    damage(ch, victim, dam, sn, DAM_FIRE , TRUE, NULL);
    return TRUE;
}


bool spell_fireproof(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;

    if (target == TARGET_OBJ && vo != NULL)
    {
	OBJ_DATA *obj = (OBJ_DATA *) vo;

	if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF))
	{
	    act("$p - уже защищено.", ch, obj, NULL, TO_CHAR);
	    return FALSE;
	}

	af.where     = TO_OBJECT;
	af.type      = sn;
	af.level     = level;
	af.duration  = number_fuzzy(level / 4);
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = ITEM_BURN_PROOF;

	affect_to_obj(obj, &af);


	act("Ты защищаешь $p6 от огня, кислоты и электричества.{x", ch, obj, NULL, TO_CHAR);
	act("$p окружа$rся защитной аурой.{x", ch, obj, NULL, TO_ROOM);
    }
    else if (target == TARGET_CHAR && vo != NULL)
    {
	CHAR_DATA *victim = (CHAR_DATA *) vo;


	if (!is_replace(sn, level, victim))
	{
	    if (victim == ch)
		printf_to_char("Ты уже защищен%s от огня, кислоты и электричества.\n\r", ch, SEX_ENDING(ch));
	    else
		act("$N и так уже защищен$t от огня, кислоты и электричества.", ch, SEX_ENDING(victim), victim, TO_CHAR);
	    return FALSE;
	}

	if (victim == ch)
	    send_to_char("Ты защищаешься от огня, кислоты и электричества!\n\r", ch);
	else
	{
	    act("Ты защищаешь $N3 от огня, кислоты и электричества.{x", ch, NULL, victim, TO_CHAR);
	    act("Ты окружаешься защитной аурой.{x", ch, NULL, victim, TO_VICT);
	}
	act("$N окружается защитной аурой.{x", ch, NULL, victim, TO_NOTVICT);

	affect_strip(victim, sn);

	af.where = TO_RESIST;
	af.type      = sn;
	af.level     = level;
	af.duration  = level/4;
	af.location  = APPLY_NONE;
	af.modifier  = level/5;
	af.bitvector = DAM_FIRE;
	af.caster_id = get_id(ch);
	affect_to_char(victim, &af);

	af.bitvector = DAM_LIGHTNING;
	affect_to_char(victim, &af);
	
	af.bitvector = DAM_ACID;
	affect_to_char(victim, &af);




    }

    return TRUE;
}



bool spell_flamestrike(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice(20 + level / 2, 10);
    if (saves_spell(level, victim, DAM_FIRE))
	dam /= 2;
    damage(ch, victim, dam, sn, DAM_FIRE , TRUE, NULL);
    return TRUE;
}



bool spell_faerie_fire(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_FAERIE_FIRE))
    {
	act("$N и так уже окружен$t розовой аурой.", ch, SEX_ENDING(victim), victim, TO_CHAR);
	return FALSE;
    }	

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level   = level;
    af.duration  = level / 2;
    af.location  = APPLY_AC;
    af.modifier  = 2 * level;
    af.bitvector = AFF_FAERIE_FIRE;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    send_to_char("Ты окружаешься розовой аурой.\n\r{x", victim);
    act("$n окружается розовой аурой.{x", victim, NULL, NULL, TO_ROOM);
    return TRUE;
}



bool spell_faerie_fog(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *ich, *safe_ich;

    act("$n вызывает облако розового тумана.", ch, NULL, NULL, TO_ROOM);
    send_to_char("Ты вызываешь облако розового тумана.\n\r", ch);

    LIST_FOREACH_SAFE(ich, &ch->in_room->people, room_link, safe_ich)
    {
	if (ich->invis_level > 0 || IS_WIZINVISMOB(ich))
	    continue;

	if (ich == ch || saves_spell(level, ich, DAM_OTHER))
	    continue;

	affect_strip (ich, gsn_invis      );
	affect_strip (ich, gsn_mass_invis    );
	affect_strip (ich, gsn_sneak      );
	affect_strip (ich, gsn_hide      );
	REMOVE_BIT   (ich->affected_by, AFF_HIDE  );
	REMOVE_BIT   (ich->affected_by, AFF_INVISIBLE  );
	REMOVE_BIT   (ich->affected_by, AFF_SNEAK  );

	affect_strip (ich, gsn_camouflage    );
	affect_strip (ich, gsn_camouflage_move    );
	REMOVE_BIT   (ich->affected_by, AFF_CAMOUFLAGE  );
	REMOVE_BIT   (ich->affected_by, AFF_CAMOUFLAGE_MOVE);

	affect_strip (ich, gsn_dissolve      );

	act("$n проявляется!{x", ich, NULL, NULL, TO_ROOM);
	send_to_char("Тебя обнаружили!\n\r{x", ich);
    }

    return TRUE;
}

bool spell_floating_disc(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *disc;
    int apply, mod, to, bit;
    AFFECT_DATA af;

    if ((disc = get_eq_char(ch, WEAR_FLOAT)) != NULL
	&& IS_OBJ_STAT(disc, ITEM_NOREMOVE))
    {
	act("Ты не можешь снять $p.", ch, disc, NULL, TO_CHAR);
	return FALSE;
    }

    disc = create_object(get_obj_index(OBJ_VNUM_DISC), 0);
    disc->value[0]  = ch->level * 10; /* 10 pounds per level capacity */
    disc->value[3]  = ch->level * 5; /* 5 pounds per level max per item */
    disc->timer    = ch->level * 4 - number_range(0, level / 2);

    apply = APPLY_NONE; mod = 0; to = 0; bit = 0;

    switch (number_bits(4))
    {
    case 0: apply = APPLY_STR;
	    mod = 1;
	    to = TO_OBJECT;
	    break;
    case 1: apply = APPLY_DEX;
	    mod = 1;
	    to = TO_OBJECT;
	    break;
    case 2: apply = APPLY_INT;
	    mod = 1;
	    to = TO_OBJECT;
	    break;
    case 3: apply = APPLY_WIS;
	    mod = 1;
	    to = TO_OBJECT;
	    break;
    case 4: apply = APPLY_CON;
	    mod = 1;
	    to = TO_OBJECT;
	    break;
    case 5: apply = APPLY_MANA;
	    mod = number_range(1, level);
	    to = TO_OBJECT;
	    break;
    case 6: apply = APPLY_HIT;
	    mod = number_range(1, level);
	    to = TO_OBJECT;
	    break;
    case 7: apply = APPLY_MOVE;
	    mod = number_range(1, level);
	    to = TO_OBJECT;
	    break;
    case 8: apply = APPLY_AC;
	    mod = -number_range(1, level/2);
	    to = TO_OBJECT;
	    break;
    case 9: apply = APPLY_SAVES;
	    mod = -number_range(1, level/5);
	    to = TO_OBJECT;
	    break;
    case 10: apply = APPLY_HITROLL;
	     mod = number_range(1, level/10);
	     to = TO_OBJECT;
	     break;
    case 11: apply = APPLY_DAMROLL;
	     mod = number_range(1, level/10);
	     to = TO_OBJECT;
	     break;
    case 12:
	     mod = number_range(1, level);
	     to = TO_RESIST;
	     bit = DAM_FIRE;
	     break;
    case 13:
	     mod = number_range(1, level);
	     to = TO_RESIST;
	     bit = DAM_COLD;
	     break;
    case 14:
	     mod = number_range(1, level);
	     to = TO_RESIST;
	     bit = DAM_ACID;
	     break;
    case 15:
	     mod = number_range(1, level);
	     to = TO_RESIST;
	     bit = DAM_POISON;
	     break;

    default:
	     break;
    }

    if ((apply != APPLY_NONE || bit != 0) && mod != 0)
    {
	af.where  = to;
	af.type  = sn;
	af.level  = level;
	af.duration  = -1;
	af.location  = apply;
	af.modifier  = mod;
	af.bitvector = bit;
	affect_to_obj(disc, &af);
    }

    act("$n создает черный летающий диск.", ch, NULL, NULL, TO_ROOM);
    send_to_char("Ты создаешь летающий диск.\n\r", ch);

    spell_identify(0, level, ch, disc, TARGET_OBJ);

    send_to_char("\n\r", ch);
    obj_to_char(disc, ch);
    wear_obj(ch, disc, TRUE);
    return TRUE;
}


bool spell_fly(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (IS_SET(victim->in_room->room_flags, ROOM_NO_FLY))
    {
	send_to_char("Здесь не взлететь.\n\r", ch);
	return FALSE;
    }

    if (is_replace(sn, level, victim))
	affect_strip(victim, sn);

    if (IS_AFFECTED(victim, AFF_FLYING))
    {
	if (victim == ch)
	    send_to_char("Ты уже и так летаешь.\n\r", ch);
	else
	    act("$N не нуждается в твоей помощи.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    if (victim->position < POS_STANDING)
    {
	send_to_char("Но ведь надо стоять на ногах, чтобы полететь!\n\r", ch);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level   = level;
    af.duration  = level/5;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_FLYING;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    act("Ты начинаешь парить $X.", victim, NULL, NULL, TO_CHAR);
    act("$n начинает парить $X.", victim, NULL, NULL, TO_ROOM);
    return TRUE;
}

/* RT clerical berserking spell */

bool spell_frenzy(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (!is_replace(sn, level, victim))
    {
	if (victim == ch)
	    send_to_char("Ты уже в ярости.\n\r", ch);
	else
	    act("$N уже в ярости.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    if (is_affected(victim, skill_lookup("calm")))
    {
	if (victim == ch)
	    send_to_char("Почему бы тебе просто не присесть отдохнуть?\n\r", ch);
	else
	    act("$N не желает сражаться.",
		ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    if (((IS_GOOD(ch) && !IS_GOOD(victim))
	|| (IS_NEUTRAL(ch) && !IS_NEUTRAL(victim))
	|| (IS_EVIL(ch) && !IS_EVIL(victim)))
	&& !IS_IMMORTAL(ch)
	&& !IS_NPC(ch))
    {
		act("Твой бог не жалует $N3.\n\r", ch, NULL, victim, TO_CHAR);
		return FALSE;
    }

    affect_strip(victim, sn);

    af.where     = TO_AFFECTS;
    af.type    = sn;
    af.level   = level;
    af.duration   = level / 3;
    af.modifier  = level / 6;
    af.bitvector = 0;
    af.caster_id = get_id(ch);

    af.location  = APPLY_HITROLL;
    affect_to_char(victim, &af);

    af.location  = APPLY_DAMROLL;
    affect_to_char(victim, &af);

    af.modifier  = 10 * (level / 12);
    af.location  = APPLY_AC;
    affect_to_char(victim, &af);

    send_to_char("Тебя переполняет ярость!\n\r{x", victim);
    act("Дикий огонь загорается в глазах $n1!{x", victim, NULL, NULL, TO_ROOM);

    return TRUE;
}

/* RT ROM-style gate */

bool spell_gate(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;

    if (IS_NULLSTR(target_name))
    {
	send_to_char("Интересно, на кого ты хотел прыгнуть?\n\r", ch);
	return FALSE;
    }

    if ((victim = can_transport(ch, level)) == NULL
	|| IS_SET(victim->in_room->room_flags, ROOM_NOGATE)
	|| IS_SET(ch->in_room->room_flags, ROOM_NOGATE)
	|| !can_see_room(ch, victim->in_room)
	|| IS_SET(victim->in_room->room_flags, ROOM_KILL)
	|| IS_SET(victim->in_room->room_flags, ROOM_NOFLY_DT)
	|| IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
	|| IS_AFFECTED(ch, AFF_CURSE))
    {
	send_to_char("У тебя ничего не получается.\n\r", ch);
	return TRUE;
    }


    if (ch->pet != NULL && IS_AWAKE(ch->pet) && ch->in_room == ch->pet->in_room)
    {
	act("$n шагает в магические врата.",
	    ch->pet, NULL, NULL, TO_ROOM);
	move_for_recall(ch->pet, victim->in_room);
	act("$n прибывает через магические врата.",
	    ch->pet, NULL, NULL, TO_ROOM);
    }

    if (MOUNTED(ch) && IS_AWAKE(MOUNTED(ch)) && MOUNTED(ch)->in_room == ch->in_room)
    {
	act("$n шагает в магические врата.",
	    MOUNTED(ch), NULL, NULL, TO_ROOM);
	move_for_recall(MOUNTED(ch), victim->in_room);
	act("$n прибывает через магические врата.",
	    MOUNTED(ch), NULL, NULL, TO_ROOM);
    }

    act("$n шагает в магические врата и исчезает.", ch, NULL, NULL, TO_ROOM);
    send_to_char("Ты шагаешь в магические врата и исчезаешь.\n\r", ch);

    check_recall(ch, "Ты делаешь врата из драки");    
    char_from_room(ch);
    char_to_room(ch, victim->in_room, TRUE);

    act("$n прибывает через магические врата.", ch, NULL, NULL, TO_ROOM);

    check_trap(ch);
    return TRUE;
}



bool spell_giant_strength(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (!is_replace(sn, level, victim)
	|| get_curr_stat(victim, STAT_STR) >= get_max_stat(victim, STAT_STR)
	|| (IS_NPC(victim) && IS_SET(victim->act, ACT_UNDEAD)))

    {
	if (victim == ch)
	    send_to_char("Твоя сила уже на максимуме!\n\r", ch);
	else
	    act("Сила $N1 уже на пределе.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    affect_strip(victim, sn);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level   = level;
    af.duration  = level;
    af.location  = APPLY_STR;
    af.modifier  = 1 + (level >= 18) + (level >= 25) + (level >= 32);
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    send_to_char("Твои мускулы накачиваются силой!\n\r{x", victim);
    act("Мускулы $n1 накачиваются силой.{x", victim, NULL, NULL, TO_ROOM);
    return TRUE;
}



bool spell_harm(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    int dam;

    if (is_affected(victim, gsn_st_aura))
    {
	act("Святая аура $N1 мешает тебе нанести $M вред.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }


    if (!IS_NPC(ch))
    {
	if (is_affected(victim, gsn_harm))
	{
	    act("$E уже и так не может лечиться.",ch, NULL, victim, TO_CHAR);
	}
	else if (saves_spell(level, victim, DAM_HARM)|| is_affected(victim, gsn_bandage))
	{
	    act("Лишить $S способности к лечению не вышло.", ch, NULL, victim, TO_CHAR);
	}
	else
	{
	    af.where  = TO_AFFECTS;
	    af.type   = gsn_harm;
	    af.level  = ch->level;
	    af.duration  = 3;
	    af.location  = APPLY_NONE;
	    af.modifier  = 0;
	    af.bitvector = 0;
	    af.caster_id = get_id(ch);
	    affect_to_char(victim, &af);

	    act("Ты лишаешь $N3 способности к лечению!",  ch, NULL, victim, TO_CHAR);
	    act("{R$n лишает тебя способности к лечению!{x",  ch, NULL, victim, TO_VICT);
	    act("$N больше не может лечиться.", ch, NULL, victim, TO_NOTVICT);
	}
    }

    dam = UMAX( 20, victim->hit - dice(1, 4));
    if (saves_spell(level, victim, DAM_HARM))
	dam = UMIN(50, dam);
    dam = UMIN(100, dam);
    damage(ch, victim, dam, sn, DAM_HARM , TRUE, NULL);
    return TRUE;
}

/* RT haste spell */

bool spell_haste(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_replace(sn, level, victim))
	affect_strip(victim, sn);

    if (IS_AFFECTED(victim, AFF_HASTE)
	|| IS_SET(victim->off_flags, OFF_FAST))
    {
	if (victim == ch)
	    send_to_char("Ты не можешь двигаться еще быстрее!\n\r", ch);
	else
	    act("$N не сможет двигаться еще быстрее.",
		ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    if (IS_AFFECTED(victim, AFF_SLOW))
    {
	if (!check_dispel(level, victim, skill_lookup("slow")))
	{
	    if (victim != ch)
		send_to_char("Не получается.\n\r", ch);
	    send_to_char("Ты чувствуешь на короткое время, что ты можешь двигаться очень быстро.\n\r", victim);
	    return TRUE;
	}
	act("$n двигается теперь не так медленно.{x", victim, NULL, NULL, TO_ROOM);
	return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    if (victim == ch)
	af.duration  = level/2;
    else
	af.duration  = level/4;
    af.location  = APPLY_DEX;
    af.modifier  = 1 + (level >= 18) + (level >= 25) + (level >= 32);
    af.bitvector = AFF_HASTE;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    send_to_char("Ты начинаешь двигаться быстрее.\n\r{x", victim);
    act("$n начинает двигаться быстрее.{x", victim, NULL, NULL, TO_ROOM);
/*    if (ch != victim)
	send_to_char("Ok.\n\r", ch);
 */
    return TRUE;
}



bool spell_heal(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (IS_VAMPIRE(victim))
    {
	send_to_char("Твое заклинание не произвело никакого эффекта.\n\r", ch);
	return FALSE;
    }

    if (is_affected(victim, gsn_harm))
    {
	if (ch==victim)
	    send_to_char("Ты не можешь вылечиться.\n\r", ch);
	else
	    send_to_char("Ты не можешь его вылечить.\n\r", ch);
	return FALSE;
    }

    victim->hit = UMIN(victim->hit + 100, victim->max_hit);
    update_pos(victim);
    send_to_char("Тепло наполняет твое тело.\n\r", victim);
    if (ch != victim)
	send_to_char("Ok.\n\r", ch);
    return TRUE;
}

bool spell_heat_metal(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    OBJ_DATA *obj_lose, *obj_next;
    int dam = 0;
    bool fail = TRUE;

    if (is_lycanthrope(victim))
    {
	fail = TRUE;
    }
    else
    if (!saves_spell(level + 2, victim, DAM_FIRE))
    {
	for (obj_lose = victim->carrying;
	     obj_lose != NULL;
	     obj_lose = obj_next)
	{
	    int mat = material_lookup(obj_lose->material);;

	    obj_next = obj_lose->next_content;

	    if (number_range(1, 2 * level) > obj_lose->level
		&& !saves_spell(level, victim, DAM_FIRE)
		&& !IS_OBJ_STAT(obj_lose, ITEM_NONMETAL)
		&& !IS_OBJ_STAT(obj_lose, ITEM_BURN_PROOF)
		&& (mat == MAT_STEEL || mat == MAT_MITHRIL
		    || mat == MAT_COPPER || mat == MAT_PLATINUM
		    || mat == MAT_GOLD || mat == MAT_BRONZE
		    || mat == MAT_SILVER || mat == MAT_IRON))
	    {
		switch (obj_lose->item_type)
		{
		default:
		    if (obj_lose->wear_loc != WEAR_NONE) /* remove the item */
		    {
			if (can_drop_obj(victim, obj_lose)
			    &&  (obj_lose->weight / 10) <
			    number_range(1, 2 * get_curr_stat(victim, STAT_DEX))
			    &&  remove_obj(victim, obj_lose->wear_loc, TRUE))
			{
			    act("$n визжит и отбрасывает от себя $p6!",
				victim, obj_lose, NULL, TO_ROOM);
			    act("Ты стаскиваешь с себя и бросаешь $p6!",
				victim, obj_lose, NULL, TO_CHAR);
			    dam += (number_range(1, obj_lose->level) / 3);

			    if (IS_OBJ_STAT(obj_lose, ITEM_INVENTORY))
				unequip_char(victim, obj_lose, TRUE);
			    else
			    {
				obj_from_char(obj_lose, TRUE);
				if (IS_OBJ_STAT(obj_lose, ITEM_MELT_DROP))
				{
				    act("$p исчезает в облачке дыма.", victim, obj_lose, NULL, TO_ALL);
				    extract_obj(obj_lose, TRUE, FALSE);
				}
				else
				    obj_to_room(obj_lose, victim->in_room);
			    }

			    fail = FALSE;
			}
			else /* stuck on the body! ouch! */
			{
			    act("Ты обжигаешься $p4!",
				victim, obj_lose, NULL, TO_CHAR);
			    dam += (number_range(1, obj_lose->level));
			    fail = FALSE;
			}

		    }
		    else /* drop it if we can */
		    {
			if (can_drop_obj(victim, obj_lose))
			{
			    act("$n визжит и отбрасывает от себя $p6!",
				victim, obj_lose, NULL, TO_ROOM);
			    act("Ты снимаешь $p6 и отбрасываешь от себя!",
				victim, obj_lose, NULL, TO_CHAR);
			    dam += (number_range(1, obj_lose->level) / 6);

			    if (!IS_OBJ_STAT(obj_lose, ITEM_INVENTORY))
			    {
				obj_from_char(obj_lose, TRUE);
				if (IS_OBJ_STAT(obj_lose, ITEM_MELT_DROP))
				{
				    act("$p исчезает в облачке дыма.", victim, obj_lose, NULL, TO_ALL);
				    extract_obj(obj_lose, TRUE, FALSE);
				}
				else
				    obj_to_room(obj_lose, victim->in_room);
			    }

			    fail = FALSE;
			}
			else /* cannot drop */
			{
			    act("Ты обжигаешься $p4!",
				victim, obj_lose, NULL, TO_CHAR);
			    dam += (number_range(1, obj_lose->level) / 2);
			    fail = FALSE;
			}
		    }
		    break;

		case ITEM_WEAPON:
		    if (obj_lose->wear_loc != WEAR_NONE) /* try to drop it */
		    {
			if (IS_WEAPON_STAT(obj_lose, WEAPON_FLAMING))
			    continue;

			if (can_drop_obj(victim, obj_lose)
			    &&  remove_obj(victim, obj_lose->wear_loc, TRUE))
			{
			    act("$n обжигается $p4 и отбрасывает это от себя!",
				victim, obj_lose, NULL, TO_ROOM);
			    send_to_char("Ты отбрасываешь от себя горячее оружие!\n\r", victim);
			    dam += 1;

			    if (IS_OBJ_STAT(obj_lose, ITEM_INVENTORY))
				unequip_char(victim, obj_lose, TRUE);
			    else
			    {
				obj_from_char(obj_lose, TRUE);
				if (IS_OBJ_STAT(obj_lose, ITEM_MELT_DROP))
				{
				    act("$p исчезает в облачке дыма.", victim, obj_lose, NULL, TO_ALL);
				    extract_obj(obj_lose, TRUE, FALSE);
				}
				else
				    obj_to_room(obj_lose, victim->in_room);
			    }

			    fail = FALSE;
			}
			else /* YOWCH! */
			{
			    send_to_char("Твое оружие обжигает твою плоть!\n\r", victim);
			    dam += number_range(1, obj_lose->level);
			    fail = FALSE;
			}
		    }
		    else /* drop it if we can */
		    {
			if (can_drop_obj(victim, obj_lose))
			{
			    act("$n обжигается $p4 и отбрасывает это от себя!",
				victim, obj_lose, NULL, TO_ROOM);
			    act("Ты снимаешь и отбрасываешь $p6 от себя!",
				victim, obj_lose, NULL, TO_CHAR);
			    dam += (number_range(1, obj_lose->level) / 6);

			    if (!IS_OBJ_STAT(obj_lose, ITEM_INVENTORY))
			    {
				obj_from_char(obj_lose, TRUE);
				if (IS_OBJ_STAT(obj_lose, ITEM_MELT_DROP))
				{
				    act("$p исчезает в облачке дыма.", victim, obj_lose, NULL, TO_ALL);
				    extract_obj(obj_lose, TRUE, FALSE);
				}
				else
				    obj_to_room(obj_lose, victim->in_room);
			    }

			    fail = FALSE;
			}
			else /* cannot drop */
			{
			    act("Ты обжигаешься $p4!",
				victim, obj_lose, NULL, TO_CHAR);
			    dam += (number_range(1, obj_lose->level) / 2);
			    fail = FALSE;
			}
		    }
		    break;
		}
	    }
	}
    }

    if (fail)
    {
	send_to_char("Ничего не получилось.\n\r", ch);
	send_to_char("Ты чувствуешь кратковременное тепло.\n\r", victim);
    }
    else /* damage! */
    {
	if (saves_spell(level, victim, DAM_FIRE))
	    dam = 2 * dam / 3;
	damage(ch, victim, dam, sn, DAM_FIRE, TRUE, NULL);
    }

    return TRUE;
}

/* RT really nasty high-level attack spell */
bool spell_holy_word(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam;
    int bless_num, curse_num, frenzy_num, count = 0;

    bless_num = skill_lookup("bless");
    curse_num = skill_lookup("curse");
    frenzy_num = skill_lookup("frenzy");

    act("$n восклицает слова божественной силы!", ch, NULL, NULL, TO_ROOM);
    send_to_char("Ты восклицаешь слова божественной силы.\n\r", ch);

    LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, vch_next)
    {
	if ((IS_GOOD(ch) && IS_GOOD(vch)) ||
	    (IS_EVIL(ch) && IS_EVIL(vch)) ||
	    (IS_NEUTRAL(ch) && IS_NEUTRAL(vch)))
	{
	    send_to_char("Ты чувствуешь себя более мощно.\n\r", vch);
	    spell_frenzy(frenzy_num, level, ch, (void *) vch, TARGET_CHAR);
	    spell_bless(bless_num, level, ch, (void *) vch, TARGET_CHAR);
	    count++;
	}

	else if ((IS_GOOD(ch) && IS_EVIL(vch)) ||
		 (IS_EVIL(ch) && IS_GOOD(vch)))
	{
	    if (!is_safe_spell(ch, vch, TRUE))
	    {
		spell_curse(curse_num, level, ch, (void *) vch, TARGET_CHAR);
		send_to_char("Тебя сильно ударяют!\n\r", vch);
		dam = dice(level, 6);
		damage(ch, vch, dam, sn, DAM_ENERGY, TRUE, NULL);
		count++;
	    }
	}

	else if (IS_NEUTRAL(ch))
	{
	    if (!is_safe_spell(ch, vch, TRUE))
	    {
		spell_curse(curse_num, level/2, ch, (void *) vch, TARGET_CHAR);
		send_to_char("Тебя сильно ударяют!\n\r", vch);
		dam = dice(level, 4);
		damage(ch, vch, dam, sn, DAM_ENERGY, TRUE, NULL);
		count++;
	    }
	}
    }

    send_to_char("Ты чувствуешь себя опустошенно.\n\r", ch);


    ch->move /= 1 + count;
    ch->hit = UMAX(1, ch->hit - 50 * count);

    return TRUE;
}

char *   show_condition    args((OBJ_DATA *obj));

bool spell_identify(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA *paf;

    if (IS_SET(obj->extra_flags, ITEM_NO_IDENTIFY))
    {
	send_to_char("К сожалению, опознать этот предмет невозможно.\n\r", ch);
	return FALSE;
    }


    sprintf(buf,
	    "Объект: %s   Тип: %s   \n\rФлаги: %s.\n\rВес: %d   Цена: %d   Уровень: %d   Таймер: %d.\n\r",
	    obj->name,
	    item_name(obj->item_type, 0),
	    flag_string(extra_flags, obj->extra_flags, TRUE),
	    obj->weight / 10,
	    obj->cost,
	    obj->level,
		obj->timer
	   );
    send_to_char(buf, ch);

    if (obj->uncomf != 0)
    {
	sprintf(buf, "Неудобен для: %s\n\r",
		flag_string(comf_flags, obj->uncomf, TRUE));
	send_to_char(buf, ch);
    }

    if (obj->unusable != 0)
    {
	sprintf(buf, "Запрещен для: %s\n\r",
		flag_string(comf_flags, obj->unusable, TRUE));
	send_to_char(buf, ch);
    }

    switch (obj->item_type)
    {
    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_PILL:
	sprintf(buf, "Заклинания %d уровня:", obj->value[0]);
	send_to_char(buf, ch);

	if (obj->value[1] >= 0 && obj->value[1] < max_skills)
	{
	    send_to_char(" '", ch);
	    send_to_char(strcmp(skill_table[obj->value[1]].rname, "")?skill_table[obj->value[1]].rname:skill_table[obj->value[1]].name, ch);
	    send_to_char("'", ch);
	}

	if (obj->value[2] >= 0 && obj->value[2] < max_skills)
	{
	    send_to_char(" '", ch);
	    send_to_char(strcmp(skill_table[obj->value[2]].rname, "")?skill_table[obj->value[2]].rname:skill_table[obj->value[2]].name, ch);
	    send_to_char("'", ch);
	}

	if (obj->value[3] >= 0 && obj->value[3] < max_skills)
	{
	    send_to_char(" '", ch);
	    send_to_char(strcmp(skill_table[obj->value[3]].rname, "")?skill_table[obj->value[3]].rname:skill_table[obj->value[3]].name, ch);
	    send_to_char("'", ch);
	}

	if (obj->value[4] >= 0 && obj->value[4] < max_skills)
	{
	    send_to_char(" '", ch);
	    send_to_char(strcmp(skill_table[obj->value[4]].rname, "")?skill_table[obj->value[4]].rname:skill_table[obj->value[4]].name, ch);
	    send_to_char("'", ch);
	}

	send_to_char(".\n\r", ch);
	break;

    case ITEM_WAND:
    case ITEM_STAFF:
	sprintf(buf, "Имеет %d %s %d уровня",
		obj->value[2], hours(obj->value[2], TYPE_CHARGES),
		obj->value[0]);
	send_to_char(buf, ch);

	if (obj->value[3] >= 0 && obj->value[3] < max_skills)
	{
	    send_to_char(" '", ch);
	    send_to_char(strcmp(skill_table[obj->value[3]].rname, "")
			 ? skill_table[obj->value[3]].rname
			 : skill_table[obj->value[3]].name, ch);
	    send_to_char("'", ch);
	}

	send_to_char(".\n\r", ch);
	break;
    case ITEM_ROD:
	sprintf(buf, "Имеет заряд %d уровня с заклинанием '%s'.\n\rВремя перезарядки %d(%d) %s.\n\r",
		obj->value[0], skill_table[obj->value[1]].name, obj->value[3], obj->value[2], hours(obj->value[3], TYPE_HOURS));

	send_to_char(buf, ch);
	break;
    case ITEM_TRAP:
	sprintf(buf, "Урон %dd%d (средний %d).\n\rУровень активирования: %d.\n\r",
		obj->value[0], obj->value[1],
		(1 + obj->value[1]) * obj->value[0] / 2,
		obj->value[2]);
	send_to_char(buf, ch);
	break;

    case ITEM_DRINK_CON:
	sprintf(buf, "Содержит %s. Цвет - %s.\n\r",
		liq_table[obj->value[2]].liq_rname,
		liq_table[obj->value[2]].liq_color);
	send_to_char(buf, ch);
	break;

    case ITEM_CONTAINER:
	sprintf(buf, "Вмещаемый вес: %d#   Макс. вес одного предмета: %d#   Флаги: %s\n\r",
		obj->value[0], obj->value[3], flag_string(container_flags, obj->value[1], TRUE));
	send_to_char(buf, ch);
	if (obj->value[4] != 100)
	{
	    sprintf(buf, "Весовой модификатор: %d%%\n\r",
		    obj->value[4]);
	    send_to_char(buf, ch);
	}
	break;

    case ITEM_CHEST:
	sprintf(buf, "Вмещаемый вес: %d#   Макс. вес одного предмета: %d#   Флаги: %s\n\r",
		obj->value[0], obj->value[3], flag_string(chest_flags, obj->value[1], TRUE));
	send_to_char(buf, ch);
	if (obj->value[4] != 100)
	{
	    sprintf(buf, "Весовой модификатор: %d%%\n\r",
		    obj->value[4]);
	    send_to_char(buf, ch);
	}
	break;

    case ITEM_WEAPON:
	sprintf(buf, "Тип оружия: %s\n\r", flag_string(weapon_class, obj->value[0], TRUE));
	send_to_char(buf, ch);

	if (obj->pIndexData->new_format)
	    sprintf(buf, "Урон: %dd%d (средний %d).\n\r",
		    obj->value[1], obj->value[2],
		    (1 + obj->value[2]) * obj->value[1] / 2);
	else
	    sprintf(buf, "Урон: %d - %d (средний %d).\n\r",
		    obj->value[1], obj->value[2],
		    (obj->value[1] + obj->value[2]) / 2);
	send_to_char(buf, ch);
	if (obj->value[4])  /* weapon flags */
	{
	    sprintf(buf, "Флаги оружия: %s\n\r", flag_string(weapon_type2, obj->value[4], TRUE));
	    send_to_char(buf, ch);
	}
	break;

    case ITEM_ARMOR:
	sprintf(buf,
		"Броня: %d от укола, %d от удара, %d от рубки, %d от остального.\n\r",
		obj->value[0], obj->value[1], obj->value[2], obj->value[3]);
	send_to_char(buf, ch);
	break;

    case ITEM_MORTAR:
	sprintf(buf, "Вмещаемый вес: %d#   Макс. вес одного предмета: %d#   Кол-во предметов: %d\n\r",
		obj->value[0], obj->value[3], obj->value[2]);
	send_to_char(buf, ch);
	if (obj->value[4] != 100)
	{
	    sprintf(buf, "Весовой модификатор: %d%%\n\r", obj->value[4]);
	    send_to_char(buf, ch);
	}

	sprintf(buf, "Время существования: %d \n\r",obj->timer);
	send_to_char(buf, ch);
	break;

    case ITEM_INGREDIENT:
	break;
    }

    if (!obj->enchanted && !obj->pIndexData->edited)
	for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
	{
	    if (paf->location != APPLY_NONE && paf->modifier != 0)
	    {
		if (paf->location == APPLY_SKILL
		    && paf->type > 0
		    && paf->type < max_skills)
		{
		    sprintf(buf, "Модифицирует умение/заклинание {W%s{x "
			    "на {W%d%%{x\n\r",
			    skill_table[paf->type].rname, paf->modifier);
		}
		else
		    sprintf(buf, "Модифицирует %s на %d.\n\r",
			    flag_string(apply_flags, paf->location, TRUE),
			    paf->modifier);

		send_to_char(buf, ch);
		if (paf->bitvector) /* Не работает */
		{
		    switch(paf->where)
		    {
		    case TO_AFFECTS:
			sprintf(buf, "Добавляет эффект: %s.\n",
				flag_string(affect_flags, paf->bitvector, TRUE));
			break;
		    case TO_OBJECT:
			sprintf(buf, "Добавляет флаг: %s.\n",
				flag_string(extra_flags, paf->bitvector, TRUE));
			break;
		    case TO_RESIST:
			sprintf(buf, "Изменяет сопротивление к %s.\n\r",
				flag_string(dam_flags, paf->bitvector, TRUE));
			break;
		    default:
			sprintf(buf, "Неизвестный бит: %d: %lu\n\r",
				paf->where, paf->bitvector);
			break;
		    }
		    send_to_char(buf, ch);
		}
	    }
	}

    for (paf = obj->affected; paf != NULL; paf = paf->next)
    {
	if (paf->location != APPLY_NONE && paf->modifier != 0)
	{

	    if (paf->location == APPLY_SKILL && paf->type > 0 && paf->type < max_skills)
		sprintf (buf, "Модифицирует умение/заклинание {W%s{x на {W%d%%{x\n\r",
			 skill_table[paf->type].rname, paf->modifier);
	    else
		sprintf(buf, "Модифицирует %s на %d",
			flag_string(apply_flags, paf->location, TRUE), paf->modifier);
	    send_to_char(buf, ch);
	    if (paf->duration > -1)
	    {
		sprintf(buf, ", %d %s",
			paf->duration, hours(paf->duration, TYPE_HOURS));
		send_to_char(buf,ch);
	    }

	    send_to_char(".\n\r", ch);

	    if (paf->bitvector) /* Не работает */
	    {
		switch(paf->where)
		{
		case TO_AFFECTS:
		    sprintf(buf, "Добавляет эффект: %s.\n",
			    flag_string(affect_flags, paf->bitvector, TRUE));
		    break;
		case TO_OBJECT:
		    sprintf(buf, "Добавляет флаг: %s.\n",
			    flag_string(extra_flags, paf->bitvector, TRUE));
		    break;
		case TO_WEAPON:
		    sprintf(buf, "Добавляет флаг оружия: %s.\n",
			    flag_string(weapon_type2, paf->bitvector, TRUE));
		    break;
		case TO_RESIST:
		    sprintf(buf, "Изменяет сопротивление к %s.\n\r",
			    flag_string(dam_flags, paf->bitvector, TRUE));
		    break;
		default:
		    sprintf(buf, "Неизвестный бит: %d: %lu\n\r",
			    paf->where, paf->bitvector);
		    break;
		}
		send_to_char(buf, ch);
	    }
	}
    }

    sprintf(buf, "Состояние: %s\n\r", show_condition(obj));
    send_to_char(buf, ch);

    if (is_limit(obj) != -1)
	send_to_char("\n\rВ этом мире количество этих вещей ограничено.\n\r", ch);

    return TRUE;
}



bool spell_infravision(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_replace(sn, level, victim))
	affect_strip(victim, sn);

    if (IS_AFFECTED(victim, AFF_INFRARED))
    {
	if (victim == ch)
	    send_to_char("Ты уже можешь видеть в темноте.\n\r", ch);
	else
	    act("$N уже видит в темноте.\n\r", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }
    act("Глаза $n1 становятся красными.\n\r{x", ch, NULL, NULL, TO_ROOM);

    af.where   = TO_AFFECTS;
    af.type      = sn;
    af.level   = level;
    af.duration  = 2 * level;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_INFRARED;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    send_to_char("Твои глаза становятся красными.\n\r{x", victim);
    return TRUE;
}



bool spell_invis(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA af;

    /* object invisibility */
    if (target == TARGET_OBJ)
    {
	obj = (OBJ_DATA *) vo;

	if (IS_OBJ_STAT(obj, ITEM_INVIS))
	{
	    act("$p - уже невидимо.", ch, obj, NULL, TO_CHAR);
	    return FALSE;
	}

	af.where  = TO_OBJECT;
	af.type    = sn;
	af.level  = level;
	af.duration  = level + 12;
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector  = ITEM_INVIS;
	affect_to_obj(obj, &af);

	act("$p исчезает из вида.{x", ch, obj, NULL, TO_ALL);
	return TRUE;
    }

    /* character invisibility */
    victim = (CHAR_DATA *) vo;

    if (IS_AFFECTED(victim, AFF_FAERIE_FIRE) )
    {
	if ( ch == victim)
	{
	    send_to_char("Ты не можешь стать невидимым, пока от тебя исходит свечение.\n\r", ch);
	}
	else
	{
	    send_to_char("Это существо не может стать невидимым, пока от него исходит свечение.\n\r", ch);
	}
	return FALSE;
    }

    if (is_replace(sn, level, victim))
	affect_strip(victim, sn);

    if (IS_AFFECTED(victim, AFF_INVISIBLE))
    {
	if (victim == ch)
	    send_to_char("Ты уже пытаешься быть невидимкой для всех.\n\r", ch);
	else
	    act("$N уже пытается быть невидимкой для всех!", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }
    act("$n исчезает.{x", victim, NULL, NULL, TO_ROOM);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level + 12;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_INVISIBLE;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    send_to_char("Ты исчезаешь для окружающих.\n\r{x", victim);
    return TRUE;
}



bool spell_know_alignment(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    char *msg;
    int ap;

    ap = victim->alignment;

    if (ap >  700) msg = "У $N1 чистая золотая аура.";
    else if (ap >  350) msg = "$N - весьма нравственный персонаж.";
    else if (ap >  100) msg = "$N часто заботлив и чуток.";
    else if (ap > -100) msg = "$N не имеет жестких моральных устоев.";
    else if (ap > -350) msg = "$N лжет своим друзьям.";
    else if (ap > -700) msg = "$N - черствый убийца.";
    else msg = "$N - само зло!";

    act(msg, ch, NULL, victim, TO_CHAR);
    return TRUE;
}



bool spell_lightning_bolt_old(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    static const int16_t dam_each[] =
    {
	0,
	0,  0,  0,  0,  0,   0,  0,  0, 25, 28,
	31, 34, 37, 40, 40,  41, 42, 42, 43, 44,
	44, 45, 46, 46, 47,  48, 48, 49, 50, 50,
	51, 52, 52, 53, 54,  54, 55, 56, 56, 57,
	58, 58, 59, 60, 60,  61, 62, 62, 63, 64
    };
    int dam;

    level  = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
    level  = UMAX(0, level);
    dam    = number_range(dam_each[level] / 2, dam_each[level] * 2);
    if (saves_spell(level, victim, DAM_LIGHTNING))
	dam /= 2;
    damage(ch, victim, dam, sn, DAM_LIGHTNING , TRUE, NULL);
    return TRUE;
}

bool spell_lightning_bolt(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
//    CHAR_DATA *vch, *safe_vch;
    OBJ_DATA *obj;

    if (ch->in_room == NULL)
	return FALSE;
    
    if (is_affected(ch, sn))
    {
	send_to_char("Ты пока не можешь вызвать еще одну шаровую молнию.\n\r", ch);
	return FALSE;
    }	

    if (IS_SET(ch->in_room->room_flags, ROOM_HOLY) || 
	IS_SET(ch->in_room->room_flags, ROOM_GUILD))
    {
	send_to_char("Ты здесь не можешь создать шаровую молнию.\n\r", ch);
	return FALSE;
    }

    if (!IS_OUTSIDE(ch))
    {
	send_to_char("Ты должен выйти из помещения.\n\r", ch);
	return FALSE;
    }

    if (weather_info.sky < SKY_RAINING)
    {
	send_to_char("Для этого нужна плохая погода.\n\r", ch);
	return FALSE;
    }
    
    for (obj = ch->in_room->contents; obj; obj = obj->next_content)
    {
	if (obj->pIndexData->vnum == OBJ_VNUM_LIGHTNING)
	{                    
		send_to_char("Здесь уже есть шаровая молния.\n\r", ch);
		return FALSE;
	}
    }

/*    LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, safe_vch)
    {
	if (can_see(ch, vch) && !is_same_group(ch, vch))
	{
	    send_to_char("Ты не можешь вызвать шаровую молнию, пока здесь "
			 "находится кто-то, кто не является твоим другом!\n\r",
			 ch);
	    return FALSE;
	}
    }
*/
    obj = create_object(get_obj_index(OBJ_VNUM_LIGHTNING), 0);
    obj->timer = 3;
    obj->level = level;
//    free_string(obj->owner);
//    obj->owner = str_dup(ch->name);
    obj_to_room(obj, ch->in_room);

    af.where = TO_AFFECTS;;
    af.type = sn;
    af.level = level;
    af.modifier = 0;
    af.location = APPLY_NONE;;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    af.duration = 0;
    affect_to_char(ch, &af);

    send_to_char("Ты вызываешь шаровую молнию.\n\r", ch);
    act("$n выпускает шаровую молнию.\n\r", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}



bool spell_locate_object(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    char buf[MAX_INPUT_LENGTH];
    BUFFER *buffer;
    OBJ_DATA *obj;
    OBJ_DATA *in_obj;
    bool found;
    int number = 0, max_found;

    found = FALSE;
    number = 0;
    max_found = 2 * level;

    buffer = new_buf();

    for (obj = object_list; obj != NULL; obj = obj->next)
    {
	if (!can_see_obj(ch, obj) 
	    || !is_name(target_name, obj->name)
	    || IS_OBJ_STAT(obj, ITEM_NOLOCATE) || number_percent() > 2 * level
	    || ch->level < obj->level 
	    || IS_SET(obj->pIndexData->area->area_flags, AREA_NA)
	    || (obj->in_room && IS_SET(obj->in_room->room_flags, ROOM_HOUSE))
	    || (obj->in_obj && obj->in_obj->item_type == ITEM_CHEST)
	    || (obj->in_obj && obj->in_obj->in_obj && obj->in_obj->in_obj->item_type == ITEM_CHEST)
	    || (obj->in_room && obj->in_room->clan != 0))
	{
	    continue;
	}


	for (in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj)
	    ;

	if (in_obj->carried_by != NULL && can_see(ch, in_obj->carried_by))
	{
	    sprintf(buf, "Имеется у %s\n\r",
		    PERS(in_obj->carried_by, ch, 1));
	}
	else
	{
	    if (IS_IMMORTAL(ch) && in_obj->in_room != NULL)
		sprintf(buf, "Находится: %s [Room %d]\n\r",
			in_obj->in_room->name, in_obj->in_room->vnum);
	    else
	    {
		if (in_obj->carried_by && IS_IMMORTAL(in_obj->carried_by))
		    continue;
		else
		    sprintf(buf, "Находится: %s\n\r",
			    in_obj->in_room == NULL
			    ? "где-то" : in_obj->in_room->name);
	    }
	}

	found = TRUE;
	number++;

	buf[0] = UPPER(buf[0]);
	add_buf(buffer, buf);

	if (number >= max_found)
	    break;
    }

    if (!found)
	send_to_char("Ничего подобного в этом мире нет.\n\r", ch);
    else
	page_to_char(buf_string(buffer), ch);

    free_buf(buffer);

    return TRUE;
}


/*
 bool spell_magic_missile(int sn, int level, CHAR_DATA *ch, void *vo, int target)
 {
 CHAR_DATA *victim = (CHAR_DATA *) vo;
 static const int16_t dam_each[] =
 {
 0,
 3,  3,  4,  4,  5,   6,  6,  6,  6,  6,
 7,  7,  7,  7,  7,   8,  8,  8,  8,  8,
 9,  9,  9,  9,  9,  10, 10, 10, 10, 10,
 11, 11, 11, 11, 11,  12, 12, 12, 12, 12,
 13, 13, 13, 13, 13,  14, 14, 14, 14, 14
 };
 int dam;

 level  = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
 level  = UMAX(0, level);
 dam    = number_range(dam_each[level] / 2, dam_each[level] * 2);
 if (saves_spell(level, victim, DAM_ENERGY))
 dam /= 2;
 damage(ch, victim, dam, sn, DAM_ENERGY , TRUE, NULL);
 return TRUE;
 }
 */

bool spell_magic_missile(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    static const int16_t dam_each[] =
    {
	0,
	3,  3,  4,  4,  5,   6,  6,  6,  6,  6,
	7,  7,  7,  7,  7,   8,  8,  8,  8,  8,
	9,  9,  9,  9,  9,  10, 10, 10, 10, 10,
	11, 11, 11, 11, 11,  12, 12, 12, 12, 12,
	13, 13, 13, 13, 13,  14, 14, 14, 14, 14
    };

    static const struct
    {
	int16_t  damtype;
	char	*message;
    }  MISSILE_DAM[] =
    {
	{DAM_ENERGY, 	""},
	{DAM_ACID,  	"кислотная "},
	{DAM_FIRE,  	"огненная "},
	{DAM_COLD,  	"ледяная "},
	{DAM_LIGHTNING, "электрическая "},
	{DAM_LIGHT,  	"световая "},
	{DAM_MENTAL,  	""},
	{DAM_OTHER, 	""} 
    };
    int dam, i, lvl;
    char *back;
    bool is_mage = FALSE;

    lvl  = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
    lvl  = UMAX(0, lvl);

    back = str_dup(skill_table[sn].noun_damage);

    for(i = 0; i < (level / 10) + 1; i++)
    {
	int dt;
	char buf[MSL];

	if (ch->classid == CLASS_MAGE)
	{ 
	    is_mage = TRUE;
	    dt = MISSILE_DAM[i].damtype;

	    free_string(skill_table[sn].noun_damage);

	    sprintf(buf, "%s%s", MISSILE_DAM[i].message, back);
	    skill_table[sn].noun_damage = str_dup(buf);
	}
	else
	    dt = MISSILE_DAM[0].damtype;

	dam    = number_range(dam_each[lvl] / 2, dam_each[lvl] * 2);

	if (saves_spell(level, victim, dt))
	    dam /= 2;

	damage(ch, victim, dam, sn, dt, TRUE, NULL);
    }
    if (is_mage)
    {
	free_string(skill_table[sn].noun_damage);
	skill_table[sn].noun_damage = str_dup(back);
    }
    free_string(back);
    return TRUE;
}

bool spell_mass_healing(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *gch, *safe_gch;
    int heal_num, refresh_num;

    heal_num = skill_lookup("heal");
    refresh_num = skill_lookup("refresh");

    LIST_FOREACH_SAFE(gch, &ch->in_room->people, room_link, safe_gch)
    {
	if (IS_VAMPIRE(gch))
	    continue;

	if ((IS_NPC(ch) && IS_NPC(gch)) ||
	    (!IS_NPC(ch) && !IS_NPC(gch)) ||
	    is_same_group(ch, gch))
	{
	    spell_heal(heal_num, level, ch, (void *) gch, TARGET_CHAR);
	    spell_refresh(refresh_num, level, ch, (void *) gch, TARGET_CHAR);
	}
    }

    return TRUE;
}


bool spell_mass_invis(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    CHAR_DATA *gch, *safe_gch;

    LIST_FOREACH_SAFE(gch, &ch->in_room->people, room_link, safe_gch)
    {
	if (!is_same_group(gch, ch) || IS_AFFECTED(gch, AFF_INVISIBLE))
	    continue;
	act("$n медленно исчезает из вида.{x", gch, NULL, NULL, TO_ROOM);
	send_to_char("Ты медленно исчезаешь из вида.\n\r{x", gch);

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = level/2;
	af.duration  = 24;
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = AFF_INVISIBLE;
	af.caster_id = get_id(ch);
	affect_to_char(gch, &af);
    }
    send_to_char("Ok.\n\r", ch);

    return TRUE;
}



bool spell_null(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    send_to_char("Это не заклинание!\n\r", ch);
    return TRUE;
}



bool spell_pass_door(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_replace(sn, level, victim))
	affect_strip(victim, sn);

    if (IS_AFFECTED(victim, AFF_PASS_DOOR))
    {
	if (victim == ch)
	    send_to_char("Ты уже можешь проходить сквозь двери.\n\r", ch);
	else
	    act("$N уже может проходить сквозь двери.",
		ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = number_fuzzy(level / 4);
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_PASS_DOOR;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);

    af.where = TO_RESIST;
    af.modifier = 1 + level / 8;
    af.bitvector = DAM_WEAPON;
    affect_to_char(victim, &af);

    af.modifier = -1 - level / 8;
    af.bitvector = DAM_MAGIC;
    affect_to_char(victim, &af);

    act("$n становится полупрозрачн$t.{x", victim, SEX_END_ADJ(victim), NULL, TO_ROOM);
    act("Ты становишься полупрозрачн$t.{x", victim, SEX_END_ADJ(victim), NULL, TO_CHAR);
    return TRUE;
}

/* RT plague spell, very nasty */

bool spell_plague(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn))
    {
	act("$N уже заражен$t чумой.", ch, SEX_ENDING(victim), victim, TO_CHAR);
	return FALSE;
    }

    if (saves_spell(level, victim, DAM_DISEASE)
	|| (IS_NPC(victim) && IS_SET(victim->act, ACT_UNDEAD))
	|| (IS_VAMPIRE(victim))
	|| (victim->race == RACE_ZOMBIE))
    {
	if (ch == victim)
	    send_to_char("Ты чувствуешь кратковременную боль, но это проходит.\n\r", ch);
	else
	    act("$N выглядит очень болезненно, но это быстро проходит.", ch, NULL, victim, TO_CHAR);
	return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type     = sn;
    af.level    = level * 3/4;
    af.duration  = level / 5;
    af.location  = APPLY_STR;
    af.modifier  = -5;
    af.bitvector = AFF_PLAGUE;
    af.caster_id = get_id(ch);
    affect_join(victim, &af);

    send_to_char("Ты корчишься в агонии, чумные болячки нарывают на твоей коже.\n\r{x", victim);
    act("$n корчится в агонии, чумные болячки нарывают на $s3 коже.{x",
	victim, NULL, NULL, TO_ROOM);
    return TRUE;
}

bool spell_poison(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA af;
    char buf[MAX_STRING_LENGTH];

    if (target == TARGET_OBJ)
    {
	obj = (OBJ_DATA *) vo;

	if (obj->item_type == ITEM_FOOD || obj->item_type == ITEM_DRINK_CON)
	{
	    if (IS_OBJ_STAT(obj, ITEM_BLESS) || IS_OBJ_STAT(obj, ITEM_BURN_PROOF))
	    {
		act("У тебя не получается отравить $p3.", ch, obj, NULL, TO_CHAR);
		return FALSE;
	    }
	    obj->value[3] = level;
	    act("$p отравля$rся смертельным ядом.{x",
		ch, obj, NULL, TO_ALL);
	    return TRUE;
	}

	if (obj->item_type == ITEM_WEAPON)
	{
	    if (IS_WEAPON_STAT(obj, WEAPON_FLAMING)
		|| IS_WEAPON_STAT(obj, WEAPON_FROST)
		|| IS_WEAPON_STAT(obj, WEAPON_VAMPIRIC)
		|| IS_WEAPON_STAT(obj, WEAPON_MANAPIRIC)
		|| IS_WEAPON_STAT(obj, WEAPON_SHARP)
		|| IS_WEAPON_STAT(obj, WEAPON_SHOCKING)
		|| IS_OBJ_STAT(obj, ITEM_BLESS)
		|| IS_OBJ_STAT(obj, ITEM_BURN_PROOF))
	    {
		act("Ты не видишь способа отравить $p6.", ch, obj, NULL, TO_CHAR);
		return FALSE;
	    }

	    if (IS_WEAPON_STAT(obj, WEAPON_POISON))
	    {
		act("$p - уже отравлено.", ch, obj, NULL, TO_CHAR);
		return FALSE;
	    }

	    af.where   = TO_WEAPON;
	    af.type   = sn;
	    af.level   = level / 2;
	    af.duration   = level/4;
	    af.location   = 0;
	    af.modifier   = 0;
	    af.bitvector = WEAPON_POISON;
	    affect_to_obj(obj, &af);

	    act("$p покрыва$rся смертельным ядом.{x",
		ch, obj, NULL, TO_ALL);
	    return TRUE;
	}

	act("Ты не можешь отравить $p6.", ch, obj, NULL, TO_CHAR);
	return FALSE;
    }

    victim = (CHAR_DATA *) vo;

    if (saves_spell(level, victim, DAM_POISON)
	|| (IS_VAMPIRE(victim))
	|| (victim->race == RACE_ZOMBIE)
	|| (IS_NPC(victim) && IS_SET(victim->act, ACT_UNDEAD)))
    {
	act("$n слегка зеленеет, но это проходит.",
	    victim, NULL, NULL, TO_ROOM);
	send_to_char("Ты чувствуешь кратковременную боль, но это проходит.\n\r",
		     victim);
	return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/8 + 4;
    af.location  = APPLY_STR;
    af.modifier  = -2;
    af.bitvector = AFF_POISON;
    af.caster_id = get_id(ch);
    affect_join(victim, &af);
    sprintf(buf, "Ты чувствуешь себя очень больн%s.\n\r{x", SEX_END_ADJ(victim));
    send_to_char(buf, victim);
    act("$n выглядит очень больн$t.{x", victim, SEX_END_ADJ(victim), NULL, TO_ROOM);
    return TRUE;
}



bool spell_protection_evil(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_replace(sn, level, victim))
	affect_strip(victim, sn);

   /* if (IS_AFFECTED(victim, AFF_PROTECT_EVIL) || IS_AFFECTED(victim, AFF_PROTECT_GOOD)) {*/
	if IS_AFFECTED(victim, AFF_PROTECT_EVIL) {
		if (victim == ch)
	    	send_to_char("Ты уже под защитой.\n\r", ch);
		else
	  	  act("$N уже защищен.", ch, NULL, victim, TO_CHAR);
		return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 6;
    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = -1;
    af.bitvector = AFF_PROTECT_EVIL;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    act("Ты чувcтвуешь себя чист$t и свят$t.{x",
	victim, SEX_END_ADJ(victim), NULL, TO_CHAR);
    if (ch != victim)
	act("$N защищается от зла.{x", ch, NULL, victim, TO_CHAR);
    return TRUE;
}

bool spell_protection_good(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_replace(sn, level, victim))
	affect_strip(victim, sn);

    /*if (IS_AFFECTED(victim, AFF_PROTECT_GOOD) ||   IS_AFFECTED(victim, AFF_PROTECT_EVIL)) { */
    if IS_AFFECTED(victim, AFF_PROTECT_GOOD){
		if (victim == ch)
		    send_to_char("Ты уже под защитой.\n\r", ch);
		else
	 	   act("$N уже защищен.", ch, NULL, victim, TO_CHAR);
		return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 6;
    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = -1;
    af.bitvector = AFF_PROTECT_GOOD;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    act("Ты чувствуешь себя защищенн$t тьмой.{x", victim, SEX_END_ADJ(victim), NULL, TO_CHAR);
    if (ch != victim)
	act("$N защищается от добра.{x", ch, NULL, victim, TO_CHAR);
    return TRUE;
}


bool spell_ray_of_truth (int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam, align;

    if (IS_EVIL(ch))
    {
	victim = ch;
	send_to_char("Энергия взрывается внутри тебя!\n\r", ch);
    }

    if (victim != ch)
    {
	act("$n поднимает руки, и слепящий луч правды вылетает из ладоней!", ch, NULL, NULL, TO_ROOM);
	send_to_char("Ты поднимаешь руки, и слепящий луч правды вылетает из ладоней!\n\r",  ch);
    }

    if (IS_GOOD(victim))
    {
	act("$n плевал$t на луч правды.", victim, SEX_ENDING(victim), NULL, TO_ROOM);
	send_to_char("Луч правды слишком слаб, чтобы задеть тебя.\n\r", victim);
	return FALSE;
    }

    dam = dice(level, 10);
    if (saves_spell(level, victim, DAM_HOLY))
	dam /= 2;

    align = victim->alignment;
    align -= 350;

    if (align < -1000)
	align = -1000 + (align + 1000) / 3;

    dam = (dam * align * align) / 1000000;

    spell_blindness(gsn_blindness, level, ch, (void *) victim, TARGET_CHAR);
    damage(ch, victim, dam, sn, DAM_HOLY , TRUE, NULL);

    return TRUE;
}


bool spell_recharge(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    int chance, percent;
    bool rod;

    switch (obj->item_type)
    {
    case ITEM_WAND:
    case ITEM_STAFF:
	rod = FALSE;
	break;
    case ITEM_ROD:
	rod = TRUE;
	break;
    default:
	send_to_char("Этот предмет не заряжается.\n\r", ch);
	return FALSE;
    }

    if (obj->value[0] >= 3 * level / 2)
    {
	send_to_char("Твой уровень мастерства маловат для этого.\n\r", ch);
	return FALSE;
    }

    if (!rod && obj->value[1] == 0)
    {
	send_to_char("Этот предмет уже перезаряжался.\n\r", ch);
	return FALSE;
    }

    chance = 40 + 2 * level;

    chance -= obj->value[0]; /* harder to do high-level spells */

    if (rod)
    {
	chance -= (obj->value[1] - obj->value[2]) *
	    (obj->value[1] - obj->value[2]);
    }
    else
    {
	chance -= (obj->value[2] - obj->value[3]) *
	    (obj->value[2] - obj->value[3]);
    }

    chance = UMAX(level/2, chance);

    percent = number_percent();

    if (percent < chance / 2)
    {
	act("$p мягко светится.", ch, obj, NULL, TO_CHAR);
	act("$p мягко светится.", ch, obj, NULL, TO_ROOM);

	if (rod)
	    obj->value[3] = 0;
	else
	{
	    obj->value[2] = UMAX(obj->value[1], obj->value[2]);
	    obj->value[1] = 0;
	}
	return TRUE;
    }
    else if (percent <= chance)
    {
	int chargeback, chargemax;

	act("$p мягко светится.", ch, obj, NULL, TO_CHAR);
	act("$p мягко светится.", ch, obj, NULL, TO_CHAR);

	if (rod)
	    chargemax = obj->value[2] - obj->value[3];
	else
	    chargemax = obj->value[1] - obj->value[2];

	if (chargemax > 0)
	    chargeback = UMAX(1, chargemax * percent / 100);
	else
	    chargeback = 0;

	if (rod)
	    obj->value[3] += chargeback;
	else
	{
	    obj->value[2] += chargeback;
	    obj->value[1] = 0;
	}

    }

    else if (percent <= UMIN(95, 3 * chance / 2))
    {
	send_to_char("Ничего не произошло.\n\r", ch);

	if (rod)
	    obj->value[2]++;
	else if (obj->value[1] > 1)
	    obj->value[1]--;
    }
    else /* whoops! */
    {
	act("$p вспыхива$r и взрыва$rся!",
	    ch, obj, NULL, TO_ALL);
	extract_obj(obj, TRUE, TRUE);
    }

    return TRUE;
}

bool spell_refresh(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    char buf[MAX_STRING_LENGTH];

    victim->move = UMIN(victim->move + level, victim->max_move);

    if (victim->max_move == victim->move)
	sprintf(buf, "Ты чувствуешь себя полностью освеженн%s!\n\r", SEX_END_ADJ(victim));
    else
	sprintf(buf, "Ты чувствуешь себя менее устал%s.\n\r", SEX_END_ADJ(victim));

    send_to_char(buf, victim);

    if (ch != victim)
	send_to_char("Ok.\n\r", ch);
    return TRUE;
}

bool spell_remove_curse(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    bool found = FALSE;
    bool flag = FALSE;

    /* do object cases first */
    if (target == TARGET_OBJ)
    {
	obj = (OBJ_DATA *) vo;

	if (IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT(obj, ITEM_NOREMOVE))
	{
	    if (!IS_OBJ_STAT(obj, ITEM_NOUNCURSE)
		&&  !saves_dispel(level + 2, obj->level, 0, NULL))
	    {
		REMOVE_BIT(obj->extra_flags, ITEM_NODROP);
		REMOVE_BIT(obj->extra_flags, ITEM_NOREMOVE);
		act("$p светится синим цветом.{x", ch, obj, NULL, TO_ALL);
		return TRUE;
	    }

	    act("Проклятье на $p5 слишком сильно для тебя.", ch, obj, NULL, TO_CHAR);
	    return FALSE;
	}
	act("На $p5 вроде нет проклятия.", ch, obj, NULL, TO_CHAR);
	return FALSE;
    }

    /* characters */
    victim = (CHAR_DATA *) vo;

    if (check_dispel(level, victim, gsn_curse))
    {
	send_to_char("Ты чувствуешь себя лучше.\n\r{x", victim);
	act("$n выглядит более расслабленно.{x", victim, NULL, NULL, TO_ROOM);
	flag = TRUE;
    }

    for (obj = victim->carrying; (obj && !found); obj = obj->next_content)
    {
	if ((IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT(obj, ITEM_NOREMOVE))
	    && !IS_OBJ_STAT(obj, ITEM_NOUNCURSE))
	{   /* attempt to remove curse */
	    if (!saves_dispel(level, obj->level, 0, NULL))
	    {
		found = TRUE;
		REMOVE_BIT(obj->extra_flags, ITEM_NODROP);
		REMOVE_BIT(obj->extra_flags, ITEM_NOREMOVE);
		REMOVE_BIT(obj->extra_flags,ITEM_NODROP);
		REMOVE_BIT(obj->extra_flags,ITEM_NOREMOVE);
		act("$p загора$rся синим цветом.{x",
		    victim, obj, NULL, TO_CHAR);
		act("$p у $n1 загора$rся синим цветом.{x",
		    victim, obj, NULL, TO_ROOM);
		flag = TRUE;
	    }
	}
    }

    if (!flag)
	send_to_char("Ничего не получилось.\n\r" ,ch);

    return TRUE;
}

bool spell_sanctuary(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;


    if (victim != ch && (ch->classid != CLASS_CLERIC && !IS_IMMORTAL(ch)) )
    {		
	send_to_char("Ты не можешь колдовать это заклинание на других.\n\r", ch);
	return FALSE;
    }

    if (victim->race == RACE_VAMPIRE)
    {
	send_to_char("Твое заклинание не производит никакого эффекта.\n\r", ch);
	return FALSE;
    }

    if ((is_replace(sn, level, victim)) && !IS_NPC(victim))
	affect_strip(victim, sn);
 

    if (IS_AFFECTED(victim, AFF_SANCTUARY))
    {
	if (victim == ch)
	    send_to_char("Ты уже под защитой храма.\n\r", ch);
	else
	    act("$N уже под защитой храма.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 10;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_SANCTUARY;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    act("$n окружается белой аурой.{x", victim, NULL, NULL, TO_ROOM);
    send_to_char("Тебя окружает белая аура.\n\r{x", victim);
    return TRUE;
}

										 

bool spell_shield(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (!is_replace(sn, level, victim))
    {
	if (victim == ch)
	    send_to_char("Ты уже под действием силового щита.\n\r", ch);
	else
	    act("$N уже под действием силового щита.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    affect_strip(victim, sn);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.location  = APPLY_AC;
    af.modifier  = (self_magic(ch,victim) ? -25 : -10) - level/4;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    act("$n окружается силовым щитом.{x", victim, NULL, NULL, TO_ROOM);
    send_to_char("Ты окружаешься силовым щитом.\n\r{x", victim);
    return TRUE;
}



bool spell_shocking_grasp(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    static const int dam_each[] =
    {
	0,
	0,  0,  0,  0,  0,   0, 20, 25, 29, 33,
	36, 39, 39, 39, 40,  40, 41, 41, 42, 42,
	43, 43, 44, 44, 45,  45, 46, 46, 47, 47,
	48, 48, 49, 49, 50,  50, 51, 51, 52, 52,
	53, 53, 54, 54, 55,  55, 56, 56, 57, 57
    };
    int dam;

    if (target == TARGET_CHAR && vo != NULL)
    {
	level  = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
	level  = UMAX(0, level);
	dam    = number_range(dam_each[level] / 2, dam_each[level] * 2);
	if (saves_spell(level, victim, DAM_LIGHTNING))
	    dam /= 2;
	damage(ch, victim, dam, sn, DAM_LIGHTNING , TRUE, NULL);
    }
    else if (target == TARGET_OBJ && vo != NULL)
    {
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	AFFECT_DATA af;

	if (obj->item_type != ITEM_WEAPON)
	{
	    send_to_char("Ты можешь колдовать это заклинание только на "
			 "оружие.\n\r", ch);
	    return FALSE;
	}

	if (obj->level > ch->level)
	{
	    send_to_char("У тебя не хватает сил.\n\r", ch);
	    return FALSE;
	}

	if (IS_WEAPON_STAT(obj, WEAPON_POISON)
	    || IS_WEAPON_STAT(obj, WEAPON_FROST)
	    || IS_WEAPON_STAT(obj, WEAPON_FLAMING))
	{
	    send_to_char("Ничего не выходит.\n\r", ch);
	    return FALSE;
	}

	if (IS_WEAPON_STAT(obj, WEAPON_SHOCKING))
	{
	    act("На $p5 итак уже заметна мельчайшая сеть электрических разрядов.",
		ch, obj, NULL, TO_CHAR);
	    return FALSE;
	}

	act("Ты прикасаешься рукой к своему оружию, и на $p5 появляется мельчайшая сеть электрических разрядов.{x",
	    ch, obj, NULL, TO_CHAR);
	act("$n прикасается рукой к своему оружию, и на $p5 появляется мельчайшая сеть электрических разрядов.{x",
	    ch, obj, NULL, TO_ROOM);

	obj->condition -= 5;
	check_condition(ch, obj);

	af.where = TO_WEAPON;
	af.type = sn;
	af.level = level;
	af.duration = level / 2;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = WEAPON_SHOCKING;
	affect_to_obj(obj, &af);
    }
    else
	send_to_char("Ты не видишь ничего (и никого) похожего здесь.\n\r", ch);

    return TRUE;
}



bool spell_sleep(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (IS_IMMORTAL(victim)
	|| IS_AFFECTED(victim, AFF_SLEEP)
	|| (IS_NPC(victim) && IS_SET(victim->act, ACT_UNDEAD))
	|| (level + 2) < victim->level
	|| victim->fighting != NULL)
	return FALSE;

    if (saves_spell(level - 2, victim, DAM_CHARM))
        return TRUE;

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 1 + level/(IS_VAMPIRE(ch) ? 40 : 20);
    af.location  = APPLY_SAVES;
    af.modifier  = -level;
    af.bitvector = AFF_SLEEP;
    af.caster_id = get_id(ch);
    affect_join(victim, &af);

    if (IS_AWAKE(victim))
    {
	act("Ты чувствуешь себя очень сонн$t... zzzzzz.{x",
	    victim, SEX_END_ADJ(victim), NULL, TO_CHAR);
	act("$n ложится спать.{x", victim, NULL, NULL, TO_ROOM );
	victim->position = POS_SLEEPING;
    }
    return TRUE;
}

bool spell_slow(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn) || IS_AFFECTED(victim, AFF_SLOW))
    {
	if (victim == ch)
	    send_to_char("Ты не можешь двигаться еще медленнее!\n\r", ch);
	else
	    act("$N не может двигаться медленнее, чем сейчас.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    if (saves_spell(level, victim, DAM_OTHER)
	|| check_immune(victim, DAM_MAGIC) == 100)
    {
	if (victim != ch)
	    send_to_char("Ничего не произошло.\n\r", ch);
	send_to_char("Ты чувствуешь себя очень вяло.\n\r", victim);
	return TRUE;
    }

    if (IS_AFFECTED(victim, AFF_HASTE))
    {
	if (!check_dispel(level, victim, skill_lookup("haste")))
	{
	    if (victim != ch)
		send_to_char("Заклинание не получается.\n\r", ch);
	    send_to_char("Ты чувствуешь кратковременное замедление.\n\r", victim);
	    return TRUE;
	}

	act("$n двигается медленнее.{x", victim, NULL, NULL, TO_ROOM);
	return TRUE;
    }


    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/5;
    af.location  = APPLY_DEX;
    af.modifier  = -1 - (level >= 18) - (level >= 25) - (level >= 32);
    af.bitvector = AFF_SLOW;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    send_to_char("Ты чувствуешь, как все твои движения з а м е д л я ю т с я...\n\r{x", victim);
    act("$n начинает двигаться медленнее.{x", victim, NULL, NULL, TO_ROOM);
    return TRUE;
}


bool spell_stone_skin(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (!is_replace(sn, level, victim))
    {
	if (victim == ch)
	    send_to_char("Твоя кожа уже тверда как скала.\n\r", ch);
	else
	    act("Кожа $N1 уже тверда как камень.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    affect_strip(victim, sn);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.location  = APPLY_AC;
    af.modifier  = (self_magic(ch,victim) ? -50 : -20);
    af.bitvector = 0;
    af.caster_id = get_id(ch);

    affect_to_char(victim, &af);

    af.where = TO_RESIST;
    af.location = APPLY_NONE;
    af.modifier = level / 5;
    af.bitvector = DAM_BASH;
    affect_to_char(victim, &af);

    act("Кожа $n1 становится как камень.{x", victim, NULL, NULL, TO_ROOM);
    send_to_char("Твоя кожа становится как камень.{x\n\r", victim);
    return TRUE;
}

bool spell_summon(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim, *lch, *safe_lch;

    /*    send_to_char("Призыв временно отключен.\n\r", ch);
     return; */

    if (number_percent() > get_skill(ch, sn) && number_bits(1) == 0)
    {
	int mcounter, mob_vnum;
	MOB_INDEX_DATA *vsearch = NULL;
	CHAR_DATA *v = NULL;

	for (mcounter = 0; mcounter < 2500; mcounter ++)
	{
	    mob_vnum = number_range(100, MAX_VNUM);

	    if ((vsearch = get_mob_index(mob_vnum)) != NULL)
	    {
		if ((v = get_char_world(ch, vsearch->player_name)) != NULL
		    && ch->level >= v->level
		    && number_percent() < 40)
		    break;
		else
		    vsearch = NULL;
	    }
	}

	if (vsearch && v)
	    strcpy(target_name, v->name);
    }


    if ((victim = can_transport(ch, level)) == NULL
	|| IS_SET(ch->in_room->room_flags, ROOM_SAFE)
	|| IS_SET(ch->in_room->room_flags, ROOM_PRIVATE)
	|| IS_SET(ch->in_room->room_flags, ROOM_SOLITARY)
	|| (IS_NPC(victim) && IS_SET(victim->act, ACT_AGGRESSIVE))
	|| (!IS_NPC(victim) && victim->level >= LEVEL_IMMORTAL)
	|| victim->fighting != NULL
	|| (IS_NPC(victim) && victim->pIndexData->pShop != NULL)
	|| (ch->in_room->clan != 0 && is_clan(victim) != ch->in_room->clan)
	|| victim->spec_fun == spec_lookup("spec_guard"))
    {
	send_to_char("Не получается.\n\r", ch);
	return TRUE;
    }

    LIST_FOREACH_SAFE(lch, &ch->in_room->people, room_link, safe_lch)
    {
	if (IS_NPC(lch) && IS_SET(lch->act, ACT_AGGRESSIVE) && lch->level + 5 > victim->level)
	{
	    send_to_char("Не стоит призывать сюда этого персонажа - тебе же выйдет дороже...\n\r", ch);
	    return TRUE;
	}
    }

    act("$n внезапно исчезает.", victim, NULL, NULL, TO_ROOM);
    if (MOUNTED(victim))
    {
	act("$N внезапно появляется.", ch, NULL, victim->mount, TO_ROOM);
	char_from_room(victim->mount);
	char_to_room(victim->mount, ch->in_room, TRUE);

    }
    act("$n призывает тебя!", ch, NULL, victim,   TO_VICT);
    act("$N внезапно появляется.", ch, NULL, victim, TO_ALL);
    char_from_room(victim);
    char_to_room(victim, ch->in_room, TRUE);

    check_trap(victim);
    return TRUE;
}



bool spell_teleport(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    ROOM_INDEX_DATA *pRoomIndex;

    if (victim->in_room == NULL
	|| is_affected(victim, gsn_gods_curse)
	|| IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
	|| (victim != ch
	    && (check_immune(victim, DAM_SUMMON) == 100
		|| saves_spell(level - 5, victim, DAM_OTHER)))
	|| (!IS_NPC(ch) && victim->fighting != NULL)
	|| (pRoomIndex = get_random_room(victim)) == NULL
	|| (pRoomIndex->clan != 0 && pRoomIndex->clan != is_clan(victim))
	|| IS_SET(pRoomIndex->room_flags, ROOM_HOUSE)
	|| IS_SET(pRoomIndex->room_flags, ROOM_GUILD)
	|| IS_SET(pRoomIndex->room_flags, ROOM_PRISON))
    {
	send_to_char("Не получается.\n\r", ch);
	return TRUE;
    }

    if (victim != ch)
	send_to_char("Тебя телепортировали!\n\r", victim);
    else
	check_recall(ch, "Ты делаешь переброс из драки");

    act("$n исчезает!", victim, NULL, NULL, TO_ROOM);
    act("$N медленно проявляется.", LIST_FIRST(&pRoomIndex->people), NULL, victim, TO_ALL);
    char_from_room(victim);
    char_to_room(victim, pRoomIndex, TRUE);
    /*    do_function(victim, &do_look, "auto");*/
    check_trap(victim);
    return TRUE;
}



bool spell_ventriloquate(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    char buf[MAX_STRING_LENGTH];
    char speaker[MAX_INPUT_LENGTH];
    CHAR_DATA *vch, *safe_vch;
    CHAR_DATA *victim;

    target_name = one_argument(target_name, speaker);
    if ((victim = get_char_room(ch, NULL, speaker, FALSE)) == NULL)
    {
	send_to_char("Ты не видишь тут никого похожего.\n\r", ch);
	return FALSE;
    }

    LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, safe_vch)
    {
        if (victim != vch && IS_AWAKE(vch))
        {
            if (!saves_spell(level, vch, DAM_MAGIC))
                sprintf(buf, "%s произносит: {C%s{x\n\r", capitalize(PERS(victim, vch, 0)), target_name);
            else
                sprintf(buf, "По чьему-то принуждению %s произносит: {C%s{x\n\r", 
                     PERS(victim, vch, 0), target_name);
	    
	    send_to_char(buf, vch);
	}
    }

    return TRUE;
}



bool spell_weaken(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn))
    {
	send_to_char("Твоя жертва уже слаба.\n\r", ch);
	return FALSE;
    }

    if (saves_spell(level, victim, DAM_OTHER))
    {
	send_to_char("Не выходит.\n\r", ch);
	return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 2;
    af.location  = APPLY_STR;
    af.modifier  = -1 * (level / 8);
    af.bitvector = AFF_WEAKEN;
    af.caster_id = get_id(ch);
    affect_to_char(victim, &af);
    send_to_char("Твоя сила улетучивается.\n\r{x", victim);
    act("$n выглядит слаб$t и устал$t.{x", victim, SEX_END_ADJ(victim), NULL, TO_ROOM);
    return TRUE;
}

/* RT recall spell is back */

void kick_pet(CHAR_DATA *pet);

bool spell_word_of_recall(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    ROOM_INDEX_DATA *location;

    if (IS_NPC(ch))
	return FALSE;

    location = get_recall(ch);

    if (location == NULL)
    {
	send_to_char("Ты окончательно заблудился.\n\r", ch);
	return FALSE;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
	|| IS_AFFECTED(ch, AFF_CURSE)
	|| is_affected(ch, gsn_gods_curse)
	|| ch->count_holy_attacks > 0
	|| ch->pcdata->atheist)
    {
	send_to_char("Не получается.\n\r", ch);
	return FALSE;
    }

    check_recall(ch, "Ты делаешь возврат из драки");

    ch->move -= abs(ch->move/2);

    if (move_for_recall(ch, location))
	return TRUE;

    if (!IS_NPC(ch) && ch->mount != NULL && !ch->mount->fighting)
    {
	move_for_recall(ch->mount, location);
	kick_pet(ch->mount);
    }

    if (!IS_NPC(ch) && ch->pet != NULL && !ch->pet->fighting)
    {
	move_for_recall(ch->pet, location);
	kick_pet(ch->pet);
    }

    return TRUE;
}

/*
 * NPC spells.
 */
bool spell_acid_breath(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vch, *vch_next;
    int dam, hp_dam, dice_dam, hpch;

    act("$n выдыхает кислоту.", ch, NULL, NULL, TO_ROOM);
    act("Ты выдыхаешь кислоту.", ch, NULL, NULL, TO_CHAR);

    hpch = UMAX(12, ch->hit);
    hp_dam = number_range(hpch/11 + 1, hpch/6);
    dice_dam = dice(level, 26);

    dam = UMAX(hp_dam + dice_dam/10, dice_dam + hp_dam/10);
    acid_effect(ch->in_room, level, dam/2, TARGET_ROOM, ch);

    LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, vch_next)
    {
	if (is_safe_spell(ch, vch, TRUE)
	    || (IS_NPC(vch) && IS_NPC(ch)
		&& (ch->fighting != vch || vch->fighting != ch))
	    || (is_same_group(ch, vch)))
	{
	    continue;
	}

	    if (saves_spell(level, vch, DAM_ACID))
	    {
		if (damage(ch, vch, dam/2, sn, DAM_ACID, TRUE, NULL) && IS_VALID(vch))
		    acid_effect(vch, level/2, dam/4, TARGET_CHAR, ch);
	    }
	    else
	    {
		if (damage(ch, vch, dam, sn, DAM_ACID, TRUE, NULL) && IS_VALID(vch))
		    acid_effect(vch, level, dam, TARGET_CHAR, ch);
	    }
    }

    return TRUE;
}



bool spell_fire_breath(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vch, *vch_next;
    int dam, hp_dam, dice_dam;
    int hpch;

    act("$n выдыхает большой шар огня.", ch, NULL, NULL, TO_ROOM);
    act("Ты выдыхаешь большой шар огня.", ch, NULL, NULL, TO_CHAR);

    hpch = UMAX(10, ch->hit);
    hp_dam  = number_range(hpch/9+1, hpch/5);
    dice_dam = dice(level, 35);

    dam = UMAX(hp_dam + dice_dam /10, dice_dam + hp_dam / 10);
    fire_effect(ch->in_room, level, dam/2, TARGET_ROOM, ch);

    LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, vch_next)
    {
	if (is_safe_spell(ch, vch, TRUE)
	    || (IS_NPC(vch) && IS_NPC(ch)
		&& (ch->fighting != vch || vch->fighting != ch))
	    || (is_same_group(ch, vch)))
	{
	    continue;
	}

	    if (saves_spell(level, vch, DAM_FIRE))
	    {
		if (damage(ch, vch, dam/2, sn, DAM_FIRE, TRUE, NULL) && IS_VALID(vch))
		    fire_effect(vch, level/2, dam/4, TARGET_CHAR, ch);
	    }
	    else
	    {
		if (damage(ch, vch, dam, sn, DAM_FIRE, TRUE, NULL) && IS_VALID(vch))
		    fire_effect(vch, level, dam, TARGET_CHAR, ch);
		
	    }
    }

    return TRUE;
}

bool spell_frost_breath(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vch, *vch_next;
    int dam, hp_dam, dice_dam, hpch;

    act("$n выдыхает струю холода!", ch, NULL, NULL, TO_ROOM);
    act("Ты выдыхаешь струю холода.", ch, NULL, NULL, TO_CHAR);

    hpch = UMAX(12, ch->hit);
    hp_dam = number_range(hpch/11 + 1, hpch/6);
    dice_dam = dice(level, 26);

    dam = UMAX(hp_dam + dice_dam/10, dice_dam + hp_dam/10);
    cold_effect(ch->in_room, level, dam/2, TARGET_ROOM, ch);

    LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, vch_next)
    {
	if (is_safe_spell(ch, vch, TRUE)
	    || (IS_NPC(vch) && IS_NPC(ch)
		&& (ch->fighting != vch || vch->fighting != ch))
	    || (is_same_group(ch, vch)))
	{
	    continue;
	}

	    if (saves_spell(level, vch, DAM_COLD))
	    {
		if (damage(ch, vch, dam/2, sn, DAM_COLD, TRUE, NULL) && IS_VALID(vch))
		    cold_effect(vch, level/2, dam/4, TARGET_CHAR, ch);
	    }
	    else
	    {
		if (damage(ch, vch, dam, sn, DAM_COLD, TRUE, NULL) && IS_VALID(vch))
		    cold_effect(vch, level, dam, TARGET_CHAR, ch);
	    }
    }

    return TRUE;
}


bool spell_gas_breath(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam, hp_dam, dice_dam, hpch;

    act("$n выдыхает облако ОЧЕНЬ ядовитого газа!", ch, NULL, NULL, TO_ROOM);
    act("Ты выдыхаешь облако ядовитого газа.", ch, NULL, NULL, TO_CHAR);

    hpch = UMAX(16, ch->hit);
    hp_dam = number_range(hpch/15+1, 8);
    dice_dam = dice(level, 20);

    dam = UMAX(hp_dam + dice_dam/10, dice_dam + hp_dam/10);
    poison_effect(ch->in_room, level, dam, TARGET_ROOM, ch);

    LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, vch_next)
    {
	if (is_safe_spell(ch, vch, TRUE)
	    || (IS_NPC(vch) && IS_NPC(ch)
		&& (ch->fighting != vch || vch->fighting != ch))
	    || (is_same_group(ch, vch)))
	{
	    continue;
	}

	if (saves_spell(level, vch, DAM_POISON))
	{
	    if (damage(ch, vch, dam/2, sn, DAM_POISON, TRUE, NULL) && IS_VALID(vch))
	        poison_effect(vch, level/2, dam/4, TARGET_CHAR, ch);
	}
	else
	{
	    if (damage(ch, vch, dam, sn, DAM_POISON, TRUE, NULL) && IS_VALID(vch))
	        poison_effect(vch, level, dam, TARGET_CHAR, ch);
	}
    }
    return TRUE;
}

bool spell_lightning_breath(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vch, *vch_next;
    int dam, hp_dam, dice_dam, hpch;

    act("$n выдыхает шаровую молнию.", ch, NULL, NULL, TO_ROOM);
    act("Ты выдыхаешь шаровую молнию.", ch, NULL, NULL, TO_CHAR);

    hpch = UMAX(10, ch->hit);
    hp_dam = number_range(hpch/9+1, hpch/5);
    dice_dam = dice(level, 35);

    dam = UMAX(hp_dam + dice_dam/10, dice_dam + hp_dam/10);
    shock_effect(ch->in_room, level, dam, TARGET_ROOM, ch);

    LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, vch_next)
    {
	if (is_safe_spell(ch, vch, TRUE)
	    || (IS_NPC(vch) && IS_NPC(ch)
		&& (ch->fighting != vch || vch->fighting != ch))
	    || (is_same_group(ch, vch)))
	{
	    continue;
	}

	    if (saves_spell(level, vch, DAM_LIGHTNING))
	    {
		if (damage(ch, vch, dam/2, sn, DAM_LIGHTNING, TRUE, NULL) && IS_VALID(vch))
		    shock_effect(vch, level/2, dam/4, TARGET_CHAR, ch);
	    }
	    else
	    {
		if (damage(ch, vch, dam, sn, DAM_LIGHTNING, TRUE, NULL) && IS_VALID(vch))
		    shock_effect(vch, level, dam, TARGET_CHAR, ch);
	    }
    }
    return TRUE;
}

/* charset=cp1251 */

