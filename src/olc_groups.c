#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "olc.h"



bool gredit(CHAR_DATA *ch, char *argument)
{
    int i;
    char arg[MAX_STRING_LENGTH];
    char cmd[MAX_STRING_LENGTH];
    struct group_type *pgroup;

    EDIT_GROUP(ch, pgroup);
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
	gredit_show(ch, argument);
	return FALSE;
    }

    for (i = 0; gredit_table[i].name != NULL; i++)
	if (!str_prefix(cmd, gredit_table[i].name))
	{
	    if ((*gredit_table[i].olc_fun)(ch, argument))
		groups_changed = TRUE;
	    return TRUE;
	}

    interpret(ch, arg);
    return FALSE;
}

void do_gredit(CHAR_DATA *ch, char *argument)
{
    int i;
    char arg[MAX_STRING_LENGTH];
    char filename[MAX_STRING_LENGTH];
    struct group_type *st = NULL;

    if (IS_NPC(ch))
	return;

    argument = one_argument(argument, arg);

    if (!str_cmp(arg, "create"))
    {
	snprintf(filename, sizeof(filename), "%s", GROUPS_FILE);

	if ((st = realloc(group_table, sizeof(struct group_type) * (max_groups + 1))) == NULL)
	{
	    printf_to_char("Can't allocate %ld bytes!\n\r",
			   ch, sizeof(struct group_type) * (max_groups + 1));
	    bugf("Can't allocate %ld bytes!",
		 sizeof(struct group_type) * (max_groups + 1));
	    return;
	}

	group_table = st;
	st = &group_table[max_groups];
	max_groups++;

	st->name		= str_dup("");
	st->rname		= str_dup("");
    	for (i = 0;i < MAX_IN_GROUP; i++)
	    st->spells[i] = str_dup("");

	if (ch->desc->editor == ED_OBJECT)
	    return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);
	
	ch->desc->pEdit = (void *)st;
	ch->desc->editor = ED_GROUP;
	groups_changed = TRUE;
	gredit_show(ch, "");

	return;
    }
    
    if ((i = group_lookup(arg)) == -1)
    {
	printf_to_char("Unknown group [%s]\n\r", ch, arg);
	return;
    }

    if (ch->desc->editor == ED_OBJECT)
	return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);

    ch->desc->pEdit  = (void *)&group_table[i];
    ch->desc->editor = ED_GROUP;
    gredit_show(ch, "");
    return;
}

void do_grsave(CHAR_DATA *ch, char *argument)
{
    if (groups_changed)
    {
	save_groups();
	send_to_char("Groups list saved.\n\r", ch);
    }
    else
	send_to_char("Groups list not edit.\n\r", ch);	
}

bool gredit_show(CHAR_DATA *ch, char *argument)
{
    struct group_type *st;
    char buf[MAX_STRING_LENGTH];
    int i;

    EDIT_GROUP(ch, st);
   
    sprintf(buf, "Name : %s\n\r", st->name);
    send_to_char(buf, ch);

    sprintf(buf, "RName: %s\n\r", st->rname);
    send_to_char(buf, ch);

    for (i = 0;i < MAX_IN_GROUP; i++)
    {
	if (!IS_NULLSTR(st->spells[i]))
	{
	    sprintf(buf, "Spell: %s\n\r", st->spells[i]);
	    send_to_char(buf, ch);
	}
    }

    return FALSE;
}

bool gredit_name(CHAR_DATA *ch, char *argument)
{
    struct group_type *pgroup;

    EDIT_GROUP(ch, pgroup);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: name <group name>\n\r", ch);
	return FALSE;
    }

    free_string(pgroup->name);
    pgroup->name = str_dup(argument);

    send_to_char("Name set.\n\r", ch);
    return TRUE;
}

bool gredit_rname(CHAR_DATA *ch, char *argument)
{
    struct group_type *pgroup;

    EDIT_GROUP(ch, pgroup);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: rname <group russian name>\n\r", ch);
	return FALSE;
    }

    free_string(pgroup->rname);
    pgroup->rname = str_dup(argument);

    send_to_char("Russian name set.\n\r", ch);
    return TRUE;
}

bool gredit_add(CHAR_DATA *ch, char *argument)
{
    struct group_type *pgroup;
    int i, num = -1, sk;
    bool is_skill = FALSE;

    EDIT_GROUP(ch, pgroup);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: add <group/spell name>\n\r", ch);
	return FALSE;
    }

    for (i = 0;i < MAX_IN_GROUP; i++)
	if (IS_NULLSTR(pgroup->spells[i]))
	{
	    num = i;
	    break;
	}

    if (num == -1)
    {
	send_to_char("Groups is full.\n\r", ch);
	return FALSE;	
    }

    if ((sk = group_lookup(argument)) >= 0)
	is_skill = FALSE;
    else if ((sk = skill_lookup(argument)) >= 0)
	is_skill = TRUE;
    else
    {
	send_to_char("Unknown group or spell.\n\r", ch);
	return FALSE;	
    }

    for (i = 0;i < MAX_IN_GROUP; i++)
	if (!IS_NULLSTR(pgroup->spells[i]) && !str_prefix(argument,pgroup->spells[i]))
	{
	    send_to_char("Allready exists group or spell.\n\r", ch);
	    return FALSE;	
	}

    free_string(pgroup->spells[num]);
    pgroup->spells[num] = str_dup(is_skill ? skill_table[sk].name : group_table[sk].name);

    send_to_char("Set group or spell.\n\r", ch);
    return TRUE;
}

bool gredit_delete(CHAR_DATA *ch, char *argument)
{
    struct group_type *pgroup;
    int i;

    EDIT_GROUP(ch, pgroup);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: delete <skill/spell name>\n\r", ch);
	return FALSE;
    }

    for (i = 0;i < MAX_IN_GROUP; i++)
	if (!IS_NULLSTR(pgroup->spells[i]) && !str_prefix(argument,pgroup->spells[i]))
	{
	    free_string(pgroup->spells[i]);
	    pgroup->spells[i] = str_dup("");
	    send_to_char("Delete group or spell.\n\r", ch);
	    return TRUE;
	} 

    send_to_char("Unknown group or spell.\n\r", ch);
    return FALSE;
}

/* charset=cp1251 */
