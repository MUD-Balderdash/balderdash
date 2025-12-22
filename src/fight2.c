/***************************************************************************
 ***************************************************************************/


#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "merc.h"
#include "interp.h"
#include "magic.h"
#include "recycle.h"


char * makehowl (char *string, CHAR_DATA * ch)
{
    struct struckhowl howl[] =
    {
	{0, {"а", "а"} },
	{2, {"ву", "ва", "во"}},
	{0, {"в"}},
	{0, {"г"}},
	{2, {"гу", "уг", "Го"}},
	{3, {"еу", "уе", "еа", "Еэ"}},
	{5, {"х", "хе", "ех", "хХ", "ха", "Х"}},
	{4, {"га", "гу", "хэ", "ваэ", "гэ"}},
	{1, {"оу", "ау",}},
	{3, {"ру", "Руу", "уоу", "оУо"}},
	{6, {"г", "гг", "Ггг", "гг", "гГ", "Гг", "Г"}},
	{3, {"ву", "вуу", "ру", "Руу"}},
	{5, {"м", "м", "М", "ммм", "ммэ", "мама"}},
	{5, {"н", "н", "Н", "ннн", "Нн", "нну"}},
	{0, {"о"}},
	{4, {"рр", "ррэ", "рРр", "ре", "ррр"}},
	{0, {"р"}},
	{5, {"ве", "вв", "вев", "еев", "веее", "ев"}},
	{5, {"хе", "хэ", "ХЭ", "ххэ", "хее", "хуу"}},
	{0, {"у"}},
	{5, {"хр", "ххр", "ххгг", "хгг", "хгв", "вхх"}},
	{0, {"х"}},
	{3, {"вохх", "вхх", "гха", "ахв"}},
	{3, {"рых", "рыг", "рр", "ээв"}},
	{3, {"х", "хх", "ххг", "хгх"}},
	{3, {"х", "хвх", "вв", "хвхв"}},
	{3, {"а", "э", "А", "Э"}},
	{3, {"ы", "ыуыу", "Ыуы", "ыу"}},
	{3, {"у", "о", "У", "О"}},
	{0, {"э"}},
	{3, {"уу", "ууу", "уУ", "Уу"}},
	{5, {"уо", "уа", "ууу", "уауа", "ау", "ауау"}}
    };

    static char buf[5 * MAX_INPUT_LENGTH];
    char temp;
    int pos = 0;
    int randomnum;

    if (IS_NULLSTR(string))
       return "\0";
    do
    {
	temp = UPPER(*string);
	if (temp >= 'А' && temp <= 'Я')
	{
	    randomnum = number_range(0, howl[temp - 'А'].number_of_rep);
	    strcpy(&buf[pos], howl[temp - 'А'].replacement[randomnum]);
	    pos += strlen (howl[temp - 'А'].replacement[randomnum]);
	}
	else
	{
	    if ((temp >= '0') && (temp <= '9'))
	    {
		temp = number_percent() > 50 ? 'о' : 'у';
		buf[pos++] = temp;
	    }
	    else
		buf[pos++] = *string;
	}
    }
    while (*string++)
	;
	    
    buf[pos] = '\0';          /* Mark end of the string... */
    return buf;
}


void set_lycanthrope(CHAR_DATA *ch)
{
    int i;

    if (!ch->in_room || !ch)
	return;

    if (is_lycanthrope(ch))
    {
        for (i = 0; i < DAM_MAX; i++)
        {
	    switch(i)
	    {
		case DAM_CHARM: 	ch->resists[i] +=  100; break;
		case DAM_MENTAL: 	ch->resists[i] +=  100; break;
		case DAM_POISON: 	ch->resists[i] +=   50; break;
		case DAM_DISEASE: 	ch->resists[i] +=   50; break;
		case DAM_SILVER:	ch->resists[i] +=  -40; break;
		case DAM_FIRE: 		ch->resists[i] +=  -90; break;
		case DAM_MAGIC: 	ch->resists[i] +=   30; break;
		default: break;
	    }
	}

	/*Получает следующие аффекты
	*/
	SET_BIT(ch->affected_by, AFF_SNEAK);
	SET_BIT(ch->affected_by, AFF_INFRARED);
	SET_BIT(ch->affected_by, AFF_DETECT_CAMOUFLAGE);
	SET_BIT(ch->affected_by, AFF_DETECT_INVIS);
	SET_BIT(ch->affected_by, AFF_DETECT_HIDDEN);
	return;
    }

    if (is_lycanthrope_spirit(ch))
    {
	for (i = 0; i < DAM_MAX; i++)
	{
	    switch(i)
	    {
		case DAM_CHARM: 	ch->resists[i] +=  40; break;
		case DAM_MENTAL: 	ch->resists[i] +=  40; break;
		case DAM_SILVER:	ch->resists[i] += -20; break;
		case DAM_FIRE:		ch->resists[i] += -40; break;
		case DAM_MAGIC: 	ch->resists[i] +=  15; break;
		default: break;
	    }
        }
    }
}

void recovery_lycanthrope(CHAR_DATA *ch)
{
    int i;
    int hph = 0;
    OBJ_DATA *obj;

    if (!ch->in_room || !ch)
	return;

    if (is_lycanthrope(ch))
    {
        for (i = 0; i < DAM_MAX; i++)
        {
	    switch(i)
	    {
		case DAM_CHARM: 	ch->resists[i] -=  100; break;
		case DAM_MENTAL: 	ch->resists[i] -=  100; break;
		case DAM_POISON: 	ch->resists[i] -=   50; break;
		case DAM_DISEASE: 	ch->resists[i] -=   50; break;
		case DAM_SILVER:	ch->resists[i] -=  -40; break;
		case DAM_FIRE: 		ch->resists[i] -=  -90; break;
		case DAM_MAGIC: 	ch->resists[i] -=   30; break;
		default: break;
	    }
	}

	REMOVE_BIT(ch->affected_by, AFF_SNEAK);
	REMOVE_BIT(ch->affected_by, AFF_INFRARED);
	REMOVE_BIT(ch->affected_by, AFF_DETECT_CAMOUFLAGE);
	REMOVE_BIT(ch->affected_by, AFF_DETECT_INVIS);
	REMOVE_BIT(ch->affected_by, AFF_DETECT_HIDDEN);

	affect_check(ch,TO_AFFECTS,AFF_DETECT_INVIS);
	affect_check(ch,TO_AFFECTS,AFF_DETECT_HIDDEN);
	affect_check(ch,TO_AFFECTS,AFF_SNEAK);
	affect_check(ch,TO_AFFECTS,AFF_INFRARED);

	/*Убираем вой, рев
	*/
	affect_strip(ch, gsn_howl);
	affect_strip(ch, gsn_growl);

	/*Убираем мягкие шаги
	*/
	affect_strip(ch, gsn_silent_step);

	/*Убираем ауру когтей
	*/
	affect_strip(ch, gsn_claws);

	/*Убираем успокоить животного
	*/
	affect_strip(ch, gsn_animal_taming);

	/*Уничтожаем когти
	*/
	if ((obj = get_eq_char(ch, WEAR_WIELD)) != NULL)
	    extract_obj(obj, FALSE, TRUE);
	if ((obj = get_eq_char(ch, WEAR_SECONDARY)) != NULL)
	    extract_obj(obj, FALSE, TRUE);

	hph = 150;
    }
    if (is_lycanthrope_spirit(ch))
    {
	for (i = 0; i < DAM_MAX; i++)
	{
	    switch(i)
	    {
		case DAM_CHARM: 	ch->resists[i] -=  40; break;
		case DAM_MENTAL: 	ch->resists[i] -=  40; break;
		case DAM_SILVER:	ch->resists[i] -= -20; break;
		case DAM_FIRE:		ch->resists[i] -= -40; break;
		case DAM_MAGIC: 	ch->resists[i] -=  15; break;
		default: break;
	    }
        }

	hph = 70;
    }

/*    if (!IS_IMMORTAL(ch))
*	ch->hit -= hph;
*/
}

void do_spirit(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
    char arg[MAX_INPUT_LENGTH];
    int skill, sn;

    if (is_lycanthrope(ch))
    {
	send_to_char("В таком состоянии ты не можешь вызвать дух зверя.\n\r", ch);
	return;
    }

    if (is_lycanthrope_spirit(ch))
    {
	send_to_char("Дух зверя уже присутствует в тебе.\n\r", ch);
	return;
    }

    if (ch->mana < 30)
    {
	send_to_char("У тебя недостаточно энергии.\n\r", ch);
	return;	
    }

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	if (ch->classid != CLASS_LYCANTHROPE)
	    send_to_char("Вызвать дух? Это как?\n\r", ch);
	else
	    send_to_char("Ты можешь вызвать дух волка или медведя.\n\r", ch);
	return;
    }
    else if (!str_prefix(arg, "волк") || !str_prefix(arg, "wolf"))
	sn = gsn_spirit_wolf;
    else 
	if (!str_prefix(arg, "медведя") || !str_prefix(arg, "bear"))
	    sn = gsn_spirit_bear;
	else
	{
	    send_to_char("Дух какого зверя ты хочешь вызвать?\n\r", ch);
	    return;
	}

    if ((skill = get_skill(ch, sn)) < 1)
    {
	send_to_char("Вызвать дух? Это как?\n\r", ch);
	return;
    }

    if (skill < number_percent())
    {
	act("У тебя не получается вызвать дух.", ch, NULL, NULL, TO_CHAR);
/*	act("$n пытается возвать духа, но ничего не получается.", ch, NULL, NULL, TO_ROOM);
*/
	check_improve(ch, NULL, sn, FALSE, 3);
	return;
    }

    af.where = TO_AFFECTS;
    af.level = ch->level;
    af.type = sn;
    af.duration = 4;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(ch, &af);

    af.modifier = 70;
    af.location = APPLY_HIT;
    affect_to_char(ch, &af);

    ch->mana -= 30;
    ch->hit += 70;

    act("Ты вызываешь дух зверя.", ch, NULL, NULL, TO_CHAR);
    act("Ты замечаешь, что в глазах $n3 блеснула звериная ярость.", ch, NULL, NULL, TO_ROOM);
    check_improve(ch, NULL, sn, FALSE, 2);
    WAIT_STATE(ch, skill_table[sn].beats);

    /*Изменяем вульны и резисты
    */
    set_lycanthrope(ch);
}

void do_form(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
    OBJ_DATA *obj;
    char arg[MAX_INPUT_LENGTH];
    int skill, sn;

    if (is_lycanthrope(ch))
    {
	send_to_char("Ты итак уже находишься в обличии зверя.\n\r", ch);
	return;
    }

    if (is_lycanthrope_spirit(ch))
    {
	send_to_char("В таком состоянии ты не можешь изменить обличие.\n\r", ch);
	return;
    }

    if (ch->mana < 50)
    {
	send_to_char("У тебя недостаточно энергии.\n\r", ch);
	return;	
    }

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	if (ch->classid != CLASS_LYCANTHROPE)
	    send_to_char("Изменить обличие? Это как?\n\r", ch);
	else
	    send_to_char("Ты можешь изменить обличие на волка или медведя.\n\r", ch);
	return;
    }
    else if (!str_prefix(arg, "волк") || !str_prefix(arg, "wolf"))
	sn = gsn_form_wolf;
    else 
	if (!str_prefix(arg, "медведя") || !str_prefix(arg, "bear"))
	    sn = gsn_form_bear;
	else
	{
	    send_to_char("Обличие какого зверя ты хочешь принять?\n\r", ch);
	    return;
	}

    if ((skill = get_skill(ch, sn)) < 1)
    {
	send_to_char("Изменить обличие? Это как?\n\r", ch);
	return;
    }

    if (skill < number_percent())
    {
	act("У тебя не получается изменить обличие.", ch, NULL, NULL, TO_CHAR);
/*	act("$n пытается поменять обличие, но ничего не получается.", ch, NULL, NULL, TO_ROOM);
*/
	check_improve(ch, NULL, sn, FALSE, 3);
	return;
    }

    af.where = TO_AFFECTS;
    af.level = ch->level;
    af.type = sn;
    af.duration = 4;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(ch, &af);

    af.modifier = 150;
    af.location = APPLY_HIT;
    affect_to_char(ch, &af);

    ch->mana -= 50;
    ch->hit += 150;

    act("Ты меняешь обличие на звериное.", ch, NULL, NULL, TO_CHAR);
    act("Ты замечаешь, как $n превращатся в зверя.", ch, NULL, NULL, TO_ROOM);
    check_improve(ch, NULL, sn, FALSE, 2);
    WAIT_STATE(ch, skill_table[sn].beats);

    /*Снимаются: щит, оружие, зажато в руке... одеваются 2 когтя
    */
    if ((obj = get_eq_char(ch, WEAR_SHIELD)) != NULL)
	unequip_char(ch, obj, TRUE);

    if ((obj = get_eq_char(ch, WEAR_WIELD)) != NULL)
	unequip_char(ch, obj, TRUE);

    if ((obj = get_eq_char(ch, WEAR_SECONDARY)) != NULL)
	unequip_char(ch, obj, TRUE);

    if ((obj = get_eq_char(ch, WEAR_HOLD)) != NULL)
	unequip_char(ch, obj, TRUE);

    /*Появляются когти
    */
    obj = create_object(get_obj_index(OBJ_VNUM_CLAW), 0);
    obj->level = ch->level;
    obj->weight = ch->level * 1.5;
    obj->value[1] = 2 + ch->level*(20+skill)/number_range(10,14)/100;
    obj->value[2] = number_range(7,8);
    obj_to_char(obj, ch);
    equip_char(ch, obj, WEAR_WIELD);

    obj = create_object(get_obj_index(OBJ_VNUM_CLAW), 0);
    obj->level = ch->level;
    obj->weight = ch->level;
    obj->value[1] = 2 + ch->level*(20+skill)/number_range(10,14)/100;
    obj->value[2] = number_range(7,8);
    obj_to_char(obj, ch);
    equip_char(ch, obj, WEAR_SECONDARY);

    /*Убираем аффект шерсть
    */
    affect_strip(ch, gsn_hair);

    /*Изменяем вульны и резисты
    */
    set_lycanthrope(ch);

    /*Спадают корни, слово страха
    */
    affect_strip(ch, skill_lookup("roots"));
    REMOVE_BIT(ch->affected_by, AFF_ROOTS);

    affect_strip(ch, gsn_power_word_fear);

    /*Очары разбегаются
    */
    REMOVE_BIT(ch->act, PLR_NOFOLLOW);
    do_function(ch, &do_nofollow, "");
}

void do_howl(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *vch, *safe_vch;
    AFFECT_DATA af;
    int skill, chance, dam;

    if (!IS_IMMORTAL(ch) && !is_affected(ch, gsn_form_wolf))
    {
	send_to_char("Это умение могут использовать только оборотни в обличии волка.\n\r", ch);
	return;
    }

    if ((skill = get_skill(ch, gsn_howl)) < 1)
    {
	send_to_char("Выть, это как?\n\r", ch);
	return;
    }

    if (is_affected(ch,gsn_animal_taming))
    {
	send_to_char("Ты в достаточно спокойном состоянии, и у тебя не получается выть.\n\r", ch);
	return;
    }

    if (ch->mana < 20)
    {
	send_to_char("У тебя недостаточно энергии.\n\r", ch);
	return;	
    }

    WAIT_STATE(ch, skill_table[gsn_howl].beats);

    chance = skill * (ch->hit*2) / UMAX(1,ch->max_hit);

    if (chance < number_percent())
    {
	act("У тебя не получается выть.", ch, NULL, NULL, TO_CHAR);
	act("$n пытается выть, но ничего не получается.", ch, NULL, NULL, TO_ROOM);
	check_improve(ch, NULL, gsn_howl, FALSE, 6);
	return;
    }

    act("Ты начинаешь выть.", ch, NULL, NULL, TO_CHAR);
    act("$n воет, от чего кровь в твоих жилах замирает.", ch, NULL, NULL, TO_ROOM);

    ch->mana -= 20;

    LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, safe_vch)
    {
	if (vch->fighting == ch)
	{
	    chance  = skill / 2;
	    if (!IS_NPC(vch))
	    {
		chance -= 2*(get_curr_stat(vch, STAT_CON) - get_curr_stat(ch, STAT_CON));
		chance -= (get_curr_stat(vch, STAT_INT) - get_curr_stat(ch, STAT_INT));
	    }

	    if (chance > number_percent()
		&& check_immune(ch, DAM_SOUND) < number_percent())
	    {
		dam  = number_range(1, ch->level);
		dam += 3*get_curr_stat(ch, STAT_CON);
		dam += 2*(ch->level - vch->level);
		damage(ch, vch, dam, gsn_howl, DAM_SOUND, TRUE, NULL);

		if (chance > number_percent()
		    && check_immune(vch, DAM_MENTAL) < number_percent()
		    && !is_affected(vch, gsn_howl))
		{
		    act("{W$N начинает сильно бояться тебя!.{x", ch, NULL, vch, TO_CHAR);
		    act("{WТы начинаешь дико бояться!{x", ch, NULL, vch, TO_VICT);
		    af.where = TO_AFFECTS;
		    af.level = ch->level;
		    af.type = gsn_howl;
		    af.duration = number_range(ch->level/15, ch->level/10);
		    af.modifier = -UMAX(1, number_range(0,ch->level/17));
		    af.location = APPLY_STR;
		    af.bitvector = 0;
		    af.caster_id = get_id(ch);
		    affect_to_char(vch, &af);

		    af.modifier = -UMAX(1, number_range(0,ch->level/17));
		    af.location = APPLY_DEX;
		    affect_to_char(vch, &af);

		    af.modifier = -UMAX(1, number_range(0,ch->level/17));
		    af.location = APPLY_INT;
		    affect_to_char(vch, &af);

		    af.modifier = -UMAX(1, number_range(0,ch->level/17));
		    af.location = APPLY_WIS;
		    affect_to_char(vch, &af);
		}
	    }
	}
    }
    check_improve(ch, NULL, gsn_howl, TRUE, 5);
}

void do_jump(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance, skill = get_skill(ch, gsn_jump);

    one_argument(argument, arg);

    if (!IS_IMMORTAL(ch) && (!is_affected(ch,gsn_form_bear) || skill < 1))
    {
	send_to_char("Это умение могут использовать только оборотни в обличии медведя.\n\r", ch);
	return;
    }

    if (arg[0] == '\0')
    {
	send_to_char("Прыжок на кого?\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, NULL, arg, TRUE)) == NULL)
    {
	if (!too_many_victims)
	    send_to_char("Таких здесь нет.\n\r", ch);
	return;
    }

    if (victim == ch)
    {
	send_to_char("Ты пытаешься сделать прыжок на самого себя?\n\r", ch);
	return;
    }

    if (is_affected(ch,gsn_jump))
    {
	send_to_char("Ты пока не можешь совершить прыжок.\n\r", ch);
	return;
    }

    if (is_safe(ch, victim))
	return;

    if (check_charmees(ch, victim))
	return;

    if (IS_NPC(victim))
	chance = skill / 1.8;
    else
	chance = skill / 2.5;

    if (ch->size < victim->size)
	chance += (ch->size - victim->size) * 10;
    else
	chance += (ch->size - victim->size) * 5;

    chance += 2 * (ch->level - victim->level);

    chance += 2 * (get_curr_stat(ch, STAT_DEX) - get_curr_stat(victim, STAT_DEX));
    chance += 2 * (get_curr_stat(ch, STAT_CON) - get_curr_stat(victim, STAT_CON));
    chance -= get_skill(victim, gsn_dodge)/5;
    chance += GET_HITROLL(ch) / 5;

    /* speed  */
    if (IS_SET(ch->off_flags, OFF_FAST) || IS_AFFECTED(ch, AFF_HASTE))
	chance += 10;
    if (IS_SET(victim->off_flags, OFF_FAST) || IS_AFFECTED(victim, AFF_HASTE))
	chance -= 10;

    check_killer(ch, victim);  
    WAIT_STATE(ch, skill_table[gsn_jump].beats);

    chance = URANGE(5, chance, 90);

    if (number_percent() < chance)
    {
	int wait, dam;
	AFFECT_DATA af;

	dam  = number_range(ch->level * 2, ch->level * 10);
	dam += 5 * GET_DAMROLL(ch);
	if (!IS_NPC(victim))
	    dam += 50 * (get_curr_stat(ch, STAT_DEX) - get_curr_stat(victim, STAT_CON));
	dam += 3 * URANGE(25, get_carry_weight(ch) / 3, 250);
	dam  = (dam * (30 + skill))/100;

	check_improve(ch, victim, gsn_jump, TRUE, 3);
	damage(ch, victim, dam, gsn_jump, DAM_BASH, TRUE, NULL);

	if (victim->position == POS_DEAD)
	    return;

	if (!IS_SET(victim->parts, PART_LEGS))
	{
	    send_to_char("Существо нельзя сбить с ног, так как у него нет ног!\n\r", ch);
	    return;
	}

	wait = number_range(1,2);

	act("{5Ты сбиваешь с ног $N3 !{x", ch, NULL, victim, TO_CHAR);
	act("{5$n прыжком сбивает $N3 с ног!{x", ch, NULL, victim, TO_NOTVICT);
	act("{5$n прыгает на тебя, и сбивает с ног!{x", ch, NULL, victim, TO_VICT);

	if (!IS_NPC(victim))
	{
	    af.where = TO_AFFECTS;
	    af.type = gsn_jump;
	    af.level = ch->level;
	    af.duration = 0;
	    af.modifier = 0;
	    af.location = APPLY_NONE;
	    af.bitvector = 0;
	    af.caster_id = get_id(ch);
	    affect_to_char(ch, &af);
	}

	WAIT_STATE(victim, wait * PULSE_VIOLENCE);
	victim->position = POS_BASHED;	
    }
    else
    {
	check_improve(ch, victim, gsn_jump, FALSE, 3);
	damage(ch, victim, 0, gsn_jump, DAM_BASH, TRUE, NULL);
    }
}

void do_growl(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *vch, *safe_vch;
    AFFECT_DATA af;
    int chance, dam, skill = get_skill(ch, gsn_growl);

    if (!IS_IMMORTAL(ch) && !is_affected(ch, gsn_form_bear))
    {
	send_to_char("Это умение могут использовать только оборотни в обличии медведя.\n\r", ch);
	return;
    }

    if (skill < 1)
    {
	send_to_char("Рычать, это как?\n\r", ch);
	return;
    }

    if (is_affected(ch,gsn_animal_taming))
    {
	send_to_char("Ты в достаточно спокойном состоянии, и у тебя не получается рычать.\n\r", ch);
	return;
    }

    if (ch->mana < 20)
    {
	send_to_char("У тебя недостаточно энергии.\n\r", ch);
	return;	
    }

    WAIT_STATE(ch, skill_table[gsn_growl].beats);

    chance = skill * (ch->hit*2) / UMAX(1,ch->max_hit);

    if (chance < number_percent())
    {
	act("У тебя не получается рычать.", ch, NULL, NULL, TO_CHAR);
	act("$n пытается рычать, но ничего не получается.", ch, NULL, NULL, TO_ROOM);
	check_improve(ch, NULL, gsn_growl, FALSE, 6);
	return;
    }

    act("Ты начинаешь рычать.", ch, NULL, NULL, TO_CHAR);
    act("$n рычит. Это пугает тебя.", ch, NULL, NULL, TO_ROOM);

    ch->mana -= 20;

    if (!is_affected(ch, gsn_growl))
    {
	af.where = TO_AFFECTS;
	af.type = gsn_growl;
	af.level = ch->level;
	af.duration = 5;
	af.modifier = ch->level / 5;
	af.location = APPLY_DAMROLL;
	af.bitvector = AFF_BERSERK;
	af.caster_id = get_id(ch);
	affect_to_char(ch, &af);

	af.location = APPLY_HITROLL;
	affect_to_char(ch, &af);

	af.modifier = -ch->level / 5;
	af.location = APPLY_SAVES;
	affect_to_char(ch, &af);
    }

    LIST_FOREACH_SAFE(vch, &ch->in_room->people, room_link, safe_vch)
    {
	if (vch->fighting == ch)
	{
	    chance  = skill / 2;
	    if (!IS_NPC(vch))
	    {
		chance -= 2*(get_curr_stat(vch, STAT_CON) - get_curr_stat(ch, STAT_CON));
		chance -= (get_curr_stat(vch, STAT_INT) - get_curr_stat(ch, STAT_INT));
	    }

	    if (chance > number_percent() 
		&& check_immune(ch, DAM_SOUND) < number_percent())
	    {
		dam  = number_range(1, ch->level);
		dam += 3*get_curr_stat(ch, STAT_CON);
		dam += 2*(ch->level - vch->level);
		damage(ch, vch, dam, gsn_growl, DAM_SOUND, TRUE, NULL);
	    }
	}
    }
    check_improve(ch, NULL, gsn_growl, TRUE, 6);
}

void do_find_victim(CHAR_DATA *ch, char *argument)
{
    TRACK_DATA *tr;
    CHAR_DATA *victim;
    int pexit, skill = 0, i;
    char arg[MIL];
    bool find_vict = FALSE;

    skill = get_skill(ch, gsn_find_victim);

    if (!IS_IMMORTAL(ch) && (!is_affected(ch, gsn_form_wolf) || skill < 1))
    {
	send_to_char("Это умение могут использовать только оборотни в обличии волка.\n\r", ch);
	return;
    }

    if (ch->mana < 70)
    {
	send_to_char("У тебя недостаточно энергии.\n\r", ch);
	return;	
    }

    one_argument(argument, arg);

    if (IS_NULLSTR(arg))
    {
	send_to_char("На кого ты собираешься охотиться?\n\r", ch);
	return;
    }

    act("$n прислушивается и внюхивается в запахи.",ch, NULL, NULL, TO_ROOM);

    if ((victim = get_char_world(ch, arg)) == NULL
	|| ch->in_room == NULL
	|| IS_SET(ch->in_room->room_flags, ROOM_NOTRACK)
	|| number_percent() >= skill)
    {
	send_to_char("Ты не замечаешь никаких признаков присутствия жертвы.\n\r", ch);
	return;
    }

    for (i = 0; i < 3;i++)
    {
	if ((tr = get_track(ch, victim)) != NULL)
	{
	    if (ch->fighting != NULL && !IS_NPC(ch->fighting))
	    {
		act("Ты останавливаешься и принимаешь бой.", ch, NULL, NULL, TO_CHAR);
		break;
	    }

	    if (!ch->in_room || IS_SET(ch->in_room->room_flags, ROOM_NORUN))
	    {
		send_to_char("Тут невозможно быстро двигаться.\n\r", ch);
		break;
	    }

	    if ((pexit = tr->direction) >= 0)
		move_char(ch, pexit, TRUE, TRUE);

	    if (ch->in_trap)
	    {
		ch->in_trap = FALSE;
		break;
	    }

	    if (!ch->in_room)
		break;

	    if (get_char_room(ch, NULL, victim->name, FALSE) != NULL)
	    {	
		if (is_safe(ch, victim))
		    break;
		act("{WТы догоняешь $N3 и с диким воплем набрасываешься...{x", ch, NULL, victim, TO_CHAR);
		act("{W$n догоняет тебя и с диким воплем набрасывается...{x", ch, NULL, victim, TO_VICT);
		do_function(ch, &do_kill, victim->name);
		check_killer(ch, victim);
		find_vict = TRUE;
		break;
	    }	    
	    check_improve(ch, victim, gsn_find_victim, TRUE, 3);	
	}
	else
	{
	    send_to_char("Ты не замечаешь здесь следов объекта охоты.\n\r", ch);
	    check_improve(ch, victim, gsn_find_victim, FALSE, 5);
	}
    }

    if (find_vict)
    {
	WAIT_STATE(ch, skill_table[gsn_find_victim].beats * 2);
	ch->mana -= 70;
    }
    else
	WAIT_STATE(ch, skill_table[gsn_find_victim].beats);

}

void do_clutch_lycanthrope(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    int skill = get_skill(ch, gsn_clutch_lycanthrope);
    char arg[MIL];

    if (!IS_IMMORTAL(ch) && (!is_lycanthrope(ch) || skill < 1))
    {
	send_to_char("Это умение могут использовать только оборотни в обличии зверя.\n\r", ch);
	return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	victim = ch->fighting;
	if (victim == NULL || victim->position == POS_DEAD)
	{
	    send_to_char("Кого ты хочешь ударить?\n\r", ch);
	    return;
	}
    }
    else if ((victim = get_char_room(ch, NULL, arg, TRUE)) == NULL)
    {
	if (!too_many_victims)
	    send_to_char("Здесь таких нет.\n\r", ch);
	return;
    }

    if (victim == ch)
    {
	send_to_char("Ты пытаешься ударить себя?\n\r", ch);
	return;
    }

    if (is_safe(ch, victim))
	return;

    if (check_charmees(ch, victim))
	return;


    WAIT_STATE(ch, skill_table[gsn_clutch_lycanthrope].beats);
    check_killer(ch, victim);

    if (skill > number_percent())
    {
	AFFECT_DATA af;
	int chance, dam;

	dam = number_range(1, ch->level);
	dam += 4*get_curr_stat(ch, STAT_STR);

	act("$n бьет $N3 лапой.", ch, NULL, victim, TO_NOTVICT);
	damage(ch, victim, dam, gsn_clutch_lycanthrope, DAM_PIERCE, TRUE, NULL);

	chance  = skill / 4;
	chance -= 2*(get_curr_stat(victim, STAT_DEX) - get_curr_stat(ch, STAT_DEX));
	
	if (check_immune(victim, DAM_POISON) == 100)
	    chance = 0;

	if (chance > number_percent() 
	    && !is_affected(victim, gsn_clutch_lycanthrope)
	    && victim->position != POS_DEAD)
	{
	    af.where = TO_AFFECTS;
	    af.type = gsn_clutch_lycanthrope;
	    af.level = ch->level;
	    af.duration = number_range(ch->level/25, ch->level/15);
	    af.modifier = 0;
	    af.location = APPLY_NONE;
	    af.bitvector = 0;
	    af.caster_id = get_id(ch);
	    affect_to_char(victim, &af);

	    act("Яд с когтей $n3 проникает в кровь $N3.", ch, NULL, victim, TO_NOTVICT);
	    act("Яд с твоих когтей отравляет $N3.", ch, NULL, victim, TO_CHAR);
	    act("Яд с когтей $n3 проникает в твою кровь.", ch, NULL, victim, TO_VICT);
	}

	check_improve(ch, victim, gsn_clutch_lycanthrope, TRUE, 3);
    }
    else
    {
	act("$n безуспешно пытается ударить $N3 лапой.", ch, NULL, victim, TO_NOTVICT);

	damage(ch, victim, 0, gsn_clutch_lycanthrope, DAM_PIERCE, TRUE, NULL);
	check_improve(ch, victim, gsn_clutch_lycanthrope, FALSE, 3);
    }
}

void do_silent_step(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
    int skill = get_skill(ch, gsn_silent_step);

    if (!IS_IMMORTAL(ch) && (!is_lycanthrope(ch) || skill < 1))
    {
	send_to_char("Это умение могут использовать только оборотни в обличии зверя.\n\r", ch);
	return;
    }

    if (ch->mana < 30)
    {
	send_to_char("У тебя недостаточно энергии, чтобы начать мягко двигаться.\n\r", ch);
	return;	
    }

    if (is_affected(ch, gsn_silent_step))
    {
	send_to_char("Ты уже и так мягко двигаешься.\n\r", ch);
	return;
    }

    if (skill < number_percent())
    {
	send_to_char("Ты пытаешься мягко двигаться.\n\r", ch);
	check_improve(ch, NULL, gsn_silent_step, FALSE, 5);
	return;	
    }

    af.where = TO_AFFECTS;
    af.type = gsn_silent_step;
    af.level = ch->level;
    af.duration = number_range(ch->level/3, ch->level);
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = 0;
    af.caster_id = get_id(ch);
    affect_to_char(ch, &af);
    send_to_char("Ты начинаешь мягко двигаться.\n\r", ch);

    ch->mana -= 30;

    WAIT_STATE(ch, skill_table[gsn_silent_step].beats);
    check_improve(ch, NULL, gsn_silent_step, TRUE, 4);
}

void do_claws(CHAR_DATA *ch, char *argument)
{
    int skill = get_skill(ch, gsn_claws);
    char arg[MIL];

    if (!IS_IMMORTAL(ch) && (!is_lycanthrope(ch) || skill < 1))
    {
	send_to_char("Это умение могут использовать только оборотни в обличии зверя.\n\r", ch);
	return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("К какой стихии ты хочешь воззывать?\n\r", ch);
	return;
    }

    if (!str_prefix(arg,"вернуть") || !str_prefix(arg,"back"))
    {
	if (is_affected(ch,gsn_claws))
	{
	    affect_strip(ch, gsn_claws);
	    send_to_char("Ты отказываешься от стихий, и аура спадает.\n\r",ch);
	}
	else
	    send_to_char("Вернуть? Стихии не были призваны!\n\r", ch);
	return;
    }

    if (ch->mana < 50)
    {
	send_to_char("У тебя недостаточно энергии, чтобы взывать к стихиям.\n\r", ch);
	return;	
    }

    WAIT_STATE(ch, skill_table[gsn_claws].beats);

    if (skill < number_percent())
    {
	send_to_char("У тебя не получается воззвать стихию.\n\r", ch);
	ch->mana -= 25;
	check_improve(ch, NULL, gsn_claws, FALSE, 4);
    }
    else
    {
	AFFECT_DATA af;

	if (!str_prefix(arg,"вода") || !str_prefix(arg,"water") || !str_prefix(arg,"воды"))
	{
	    af.bitvector = DAM_ACID;
	    send_to_char("Ты призываешь стихию воды на свои когти.\n\r",ch);
	}
	else if (!str_prefix(arg,"грозы") || !str_prefix(arg,"thander") || !str_prefix(arg,"гроза"))
	{
	    af.bitvector = DAM_LIGHTNING;
	    send_to_char("Ты призываешь стихию грозы на свои когти.\n\r",ch);
	}
	else if (!str_prefix(arg,"холода") || !str_prefix(arg,"cold"))
	{
	    af.bitvector = DAM_COLD;
	    send_to_char("Ты призываешь стихию холода на свои когти.\n\r",ch);
	}
	else
	{
	    send_to_char("К чему ты хочешь воззвать?\n\r", ch);
	    return;
	}

	affect_strip(ch, gsn_claws);

	af.where = TO_RESIST;
	af.type = gsn_claws;
	af.level = ch->level;
	af.duration = 12;
	af.modifier = 1;
	af.location = APPLY_NONE;	
	af.caster_id = get_id(ch);
	affect_to_char(ch, &af);

	ch->mana -= 50;

	check_improve(ch, NULL, gsn_claws, TRUE, 3);
    }
}

/* charset=cp1251 */

