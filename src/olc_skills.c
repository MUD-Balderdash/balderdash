#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "olc.h"


bool skedit(CHAR_DATA *ch, char *argument)
{
    int i;
    char arg[MAX_STRING_LENGTH];
    char cmd[MAX_STRING_LENGTH];
    struct skill_type *pSkill;

    EDIT_SKILL(ch, pSkill);
    smash_tilde(argument);
    strcpy(arg, argument);
    argument = one_argument(argument, cmd);

    if (!str_cmp(cmd, "done"))
    {
	edit_done(ch);
	return FALSE;
    }

    if (IS_NULLSTR(cmd))
    {
	skedit_show(ch, argument);
	return FALSE;
    }

    for (i = 0; skedit_table[i].name != NULL; i++)
	if (!str_prefix(cmd, skedit_table[i].name))
	{
	    if ((*skedit_table[i].olc_fun)(ch, argument))
		skills_changed = TRUE;
	    return TRUE;
	}

    interpret(ch, arg);
    return FALSE;
}

void do_skedit(CHAR_DATA *ch, char *argument)
{
    int i;
    char arg[MAX_STRING_LENGTH];
    char filename[MAX_STRING_LENGTH];
    struct skill_type *st = NULL;

    if (IS_NPC(ch))
	return;

    argument = one_argument(argument, arg);

    if (!str_cmp(arg, "create"))
    {
	snprintf(filename, sizeof(filename), "%s", "skills.cfg");

	if ((st = realloc(skill_table, sizeof(struct skill_type) * (max_skills + 1))) == NULL)
	{
	    printf_to_char("Can't allocate %ld bytes!\n\r",
			   ch, sizeof(struct skill_type) * (max_skills + 1));
	    bugf("Can't allocate %ld bytes!",
		 sizeof(struct skill_type) * (max_skills + 1));
	    return;
	}

	skill_table = st;
	st = &skill_table[max_skills];
	max_skills++;

	st->name		= str_dup("");
	st->rname		= str_dup("");
	st->spell_fun	 	= spell_null;
	st->target		= TAR_IGNORE;
	st->minimum_position 	= POS_STANDING;
	st->pgsn		= NULL;
	st->slot		= 0;
	st->min_mana	 	= 0;
	st->beats		= 0;
	st->noun_damage	 	= str_dup("");
	st->gender_damage	= str_dup("");
	st->damage		= str_dup("");
	st->dam_type	 	= 0;
	st->msg_off		= str_dup("");
	st->msg_obj		= str_dup("");
	st->msg_room	 	= str_dup("");
	st->flags		= 0;
	st->saves_mod	 	= 0;
	st->saves_act	 	= 0;
	st->affect		= NULL;

	for (i = 0; i < 3; i++)
	{
	    st->char_msg[i]  = str_dup("");
	    st->char_fail[i] = str_dup("");
	    if (i < 2)
	    {
		st->obj_msg[i]   = str_dup("");
		st->obj_fail[i]  = str_dup("");
		st->room_msg[i]  = str_dup("");
		st->room_fail[i] = str_dup("");
	    }
	}
    
	for (i = 0; i < MAX_SKILL; i++)
	    st->depends[i] = NULL;

	if (ch->desc->editor == ED_OBJECT)
	    return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);
	
	ch->desc->pEdit = (void *)st;
	ch->desc->editor = ED_SKILL;
	skills_changed = TRUE;
	skedit_show(ch, "");

	return;
    }
    
    if ((i = skill_lookup(arg)) == -1)
    {
	printf_to_char("Unknown skill [%s]\n\r", ch, arg);
	return;
    }

    if (ch->desc->editor == ED_OBJECT)
	return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);

    ch->desc->pEdit  = (void *)&skill_table[i];
    ch->desc->editor = ED_SKILL;
    skedit_show(ch, "");
    return;
}

void do_sksave(CHAR_DATA *ch, char *argument)
{
    if (skills_changed)
    {
	save_skills();
	send_to_char("Skill list saved.\n\r", ch);
    }
    else
	send_to_char("Skill list not edit.\n\r", ch);	
}

bool skedit_show(CHAR_DATA *ch, char *argument)
{
    struct skill_type *st;
    char buf[MAX_STRING_LENGTH];

    EDIT_SKILL(ch, st);
    
    sprintf(buf, "Name: %s\n\r", st->name);
    send_to_char(buf, ch);

    sprintf(buf, "RName: %s\n\r", st->rname);
    send_to_char(buf, ch);

    sprintf(buf, "Fun: %s\n\r", spellname_lookup(st->spell_fun));
    send_to_char(buf, ch);
    
    sprintf(buf, "Target: %s\n\r",
	    flag_string(target_flags, st->target, FALSE));
    send_to_char(buf, ch);

    sprintf(buf, "MinPos: %s\n\r",
	    flag_string(position_flags, st->minimum_position, FALSE));
    send_to_char(buf, ch);

    sprintf(buf, "MinMana: %d\n\r", st->min_mana);
    send_to_char(buf, ch);

    sprintf(buf, "Beats: %d\n\r", st->beats);
    send_to_char(buf, ch);

    sprintf(buf, "GSN: %s\n\r", st->pgsn ? gsn_name_lookup(st->pgsn) : "");
    send_to_char(buf, ch);

    sprintf(buf, "DamageMsg: %s\n\r", st->noun_damage);
    send_to_char(buf, ch);

    sprintf(buf, "DamageGender: %s\n\r", st->gender_damage);
    send_to_char(buf, ch);

    sprintf(buf, "MessageOff: %s\n\r", st->msg_off);
    send_to_char(buf, ch);

    sprintf(buf, "MessageObj: %s\n\r", st->msg_obj);
    send_to_char(buf, ch);

    sprintf(buf, "MessageRoom: %s\n\r", st->msg_room);
    send_to_char(buf, ch);

    return FALSE;
}

bool skedit_name(CHAR_DATA *ch, char *argument)
{
    struct skill_type *pskill;

    EDIT_SKILL(ch, pskill);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: name <skill name>\n\r", ch);
	return FALSE;
    }

    free_string(pskill->name);
    pskill->name = str_dup(argument);

    send_to_char("Name set.\n\r", ch);
    return TRUE;
}

bool skedit_rname(CHAR_DATA *ch, char *argument)
{
    struct skill_type *pskill;

    EDIT_SKILL(ch, pskill);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: rname <skill russian name>\n\r", ch);
	return FALSE;
    }

    free_string(pskill->rname);
    pskill->rname = str_dup(argument);

    send_to_char("Russian name set.\n\r", ch);
    return TRUE;
}

bool skedit_fun(CHAR_DATA *ch, char *argument)
{                     
    struct skill_type *pskill;

    EDIT_SKILL(ch, pskill);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: fun <function name>\n\r", ch);
	return FALSE;
    }

    if (!spellfun_lookup(argument))
    {
	send_to_char("Unknown spell_fun.\n\r", ch);
	return FALSE;
    }

    pskill->spell_fun = spellfun_lookup(argument);

    send_to_char("Function set.\n\r", ch);
    return TRUE;
}

bool skedit_target(CHAR_DATA *ch, char *argument)
{                     
    struct skill_type *pskill;
    int i;

    EDIT_SKILL(ch, pskill);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: target <target>\n\r", ch);
	send_to_char("        target - Ignore, CharOffensive, CharDefensive, CharSelf, \n\r", ch);
	send_to_char("                 ObjInventory, ObjCharDefensive, ObjCharOffensive\n\r", ch);
	return FALSE;
    }

    for (i = 0; target_flags[i].name != NULL; i++)
    {
	if (!str_prefix(argument, target_flags[i].name))
	{
	    pskill->target = i;
	    send_to_char("Target set.\n\r", ch);
	    return TRUE;
	}
    }
    send_to_char("Unknown target.\n\r", ch);
    return FALSE;
}

bool skedit_minpos(CHAR_DATA *ch, char *argument)
{                     
    struct skill_type *pskill;
    int i;

    EDIT_SKILL(ch, pskill);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: minpos <pos>\n\r", ch);
	send_to_char("        pos - dead, mortal, incap, stunned, sleeping, resting\n\r", ch);
	send_to_char("              resting, sitting, bashed, fighting, standing\n\r", ch);
	return FALSE;
    }

    for (i = 0; position_flags[i].name != NULL; i++)
    {
	if (!str_prefix(argument, position_flags[i].name))
	{
	    pskill->minimum_position = i;
	    send_to_char("Minpos set.\n\r", ch);
	    return TRUE;
	}
    }
    send_to_char("Unknown minpos.\n\r", ch);
    return FALSE;
}

bool skedit_minmana(CHAR_DATA *ch, char *argument)
{                     
    struct skill_type *pskill;

    EDIT_SKILL(ch, pskill);

    if (argument[0] == '\0' || !is_number(argument))
    {
	send_to_char("Syntax: minmana <number>\n\r", ch);
	return FALSE;
    }

    if (atoi(argument) < 0)
    {
	send_to_char("Set mana >= 0. \n\r", ch);
	return FALSE;
    }

    pskill->min_mana = atoi(argument);

    send_to_char("Minmana set.\n\r", ch);
    return TRUE;
}

bool skedit_beats(CHAR_DATA *ch, char *argument)
{                     
    struct skill_type *pskill;

    EDIT_SKILL(ch, pskill);
                                           
    if (argument[0] == '\0' || !is_number(argument))
    {
	send_to_char("Syntax: beats <number>\n\r", ch);
	return FALSE;
    }

    if (atoi(argument) < 0)
    {
	send_to_char("Set beats >= 0. \n\r", ch);
	return FALSE;
    }

    pskill->beats = atoi(argument);

    send_to_char("Beats set.\n\r", ch);
    return TRUE;
}

bool skedit_gsn(CHAR_DATA *ch, char *argument)
{                     
    struct skill_type *pskill;

    EDIT_SKILL(ch, pskill);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: gsn <gsn name>\n\r", ch);
	return FALSE;
    }

    if (gsn_lookup(argument) <= 0)
    {
	send_to_char("Unknown GSN name.\n\r", ch);
	return FALSE;
    }

    pskill->pgsn = gsn_lookup(argument);

    send_to_char("GSN set.\n\r", ch);
    return TRUE;
}

bool skedit_msgdam(CHAR_DATA *ch, char *argument)
{
    struct skill_type *pskill;

    EDIT_SKILL(ch, pskill);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: damegemsg <string>\n\r", ch);
	return FALSE;
    }

    free_string(pskill->noun_damage);
    pskill->noun_damage = str_dup(argument);

    send_to_char("Msgdam set.\n\r", ch);
    return TRUE;
}

bool skedit_genderdam(CHAR_DATA *ch, char *argument)
{
    struct skill_type *pskill;

    EDIT_SKILL(ch, pskill);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: damegegender <string>\n\r", ch);
	return FALSE;
    }

    free_string(pskill->gender_damage);
    pskill->gender_damage = str_dup(argument);

    send_to_char("Genderdam set.\n\r", ch);
    return TRUE;
}



bool skedit_msgoff(CHAR_DATA *ch, char *argument)
{
    struct skill_type *pskill;

    EDIT_SKILL(ch, pskill);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: messageoff <string>\n\r", ch);
	return FALSE;
    }

    free_string(pskill->msg_off);
    pskill->msg_off = str_dup(argument);

    send_to_char("Msgoff set.\n\r", ch);
    return TRUE;
}

bool skedit_msgobj(CHAR_DATA *ch, char *argument)
{
    struct skill_type *pskill;

    EDIT_SKILL(ch, pskill);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: messageobj <string>\n\r", ch);
	return FALSE;
    }

    free_string(pskill->msg_obj);
    pskill->msg_obj = str_dup(argument);

    send_to_char("Msgobj set.\n\r", ch);
    return TRUE;
}

bool skedit_msgroom(CHAR_DATA *ch, char *argument)
{
    struct skill_type *pskill;

    EDIT_SKILL(ch, pskill);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: messageroom <string>\n\r", ch);
	return FALSE;
    }

    free_string(pskill->msg_room);
    pskill->msg_room = str_dup(argument);

    send_to_char("Msgroom set.\n\r", ch);
    return TRUE;
}

/* charset=cp1251 */
