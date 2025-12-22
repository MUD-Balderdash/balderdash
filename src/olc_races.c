#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "merc.h"
#include "olc.h"



/* Useful macros */
#define CHECK_EDIT_PC(ch, race)					\
    if ((race)->pc_race == TRUE					\
	&& !is_spec_granted((ch), "raedit_pc"))			\
    {								\
	send_to_char("You can not edit PC races!\n\r", (ch));	\
	return FALSE;						\
    }

#define CHECK_PC(ch, race)					\
    if ((race)->pc_race != TRUE)				\
    {								\
	send_to_char("Can't set this to NPC race!\n\r", ch);	\
	return FALSE;						\
    }								\
    if (!is_spec_granted((ch), "raedit_pc"))			\
    {								\
	send_to_char("You can not edit PC races!\n\r", (ch));	\
	return FALSE;						\
    }
    
bool raedit(CHAR_DATA *ch, char *argument)
{
    int i;
    char arg[MAX_STRING_LENGTH];
    char cmd[MAX_STRING_LENGTH];
    struct race_type *race;

    EDIT_RACE(ch, race);
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
	raedit_show(ch, argument);
	return FALSE;
    }

    for (i = 0; raedit_table[i].name != NULL; i++)
	if (!str_prefix(cmd, raedit_table[i].name))
	{
	    if ((*raedit_table[i].olc_fun)(ch, argument))
		race->changed = TRUE;
	    
	    return TRUE;
	}

    interpret(ch, arg);
    return FALSE;
}

void do_raedit(CHAR_DATA *ch, char *argument)
{
    int i;
    char arg[MAX_STRING_LENGTH];
    char filename[MAX_STRING_LENGTH];
    struct stat st;
    struct race_type *race;

    if (IS_NPC(ch))
	return;

    argument = one_argument(argument, arg);

    if (!str_cmp(arg, "create"))
    {
	if (!is_spec_granted(ch, "raedit_create"))
	{
	    send_to_char("Ты не можешь создавать новые расы.\n\r", ch);
	    return;
	}

	if (IS_NULLSTR(argument))
	{
	    send_to_char("Syntax: raedit create <filename>\n\r", ch);
	    return;
	}

	snprintf(filename, sizeof(filename), "%s%s", RACES_DIR, argument);
	if (stat(filename, &st) != -1 || errno != ENOENT)
	{
	    printf_to_char("Filename [%s] already exist.\n\r", ch, filename);
	    return;
	}

	if ((race = realloc(race_table, sizeof(struct race_type) * (max_races + 1))) == NULL)
	{
	    printf_to_char("Can't allocate %ld bytes!\n\r",
			   ch, sizeof(struct race_type) * (max_races + 1));
	    bugf("Can't allocate %d bytes!",
		 sizeof(struct race_type) * (max_races + 1));
	    return;
	}

	race_table = race;
	race = &race_table[max_races];
	max_races++;

	snprintf(filename, sizeof(filename), "%s.race", argument);
	race->filename = str_dup(filename);
	race->name     = str_dup("<undef>");
	race->rname    = str_dup("<undef>");
	race->fname    = str_dup("<undef>");
	race->pc_race  = FALSE;
	race->act      = 0;
	race->aff      = 0;
	race->off      = 0;
	for (i = 0; i < DAM_MAX; i++)
	    race->resists[i] = 0;
	race->form     = 0;
	race->parts    = 0;
	
	race->who_name    = str_dup("");
	race->points      = 0;
	race->size        = SIZE_MEDIUM;
	race->recall      = 0;
	race->map_vnum    = 0;
	race->valid_align = 0;
	race->changed     = TRUE;

	race->class_mult  = alloc_perm(sizeof(int16_t) * max_classes);
	for (i = 0; i < max_classes; i++)
	    race->class_mult[i] = 0;

	for (i = 0; i < 5; i++)
	    race->skills[i] = str_dup("");

	for (i = 0; i < MAX_STATS; i++)
	{
	    race->stats[i]     = 18;
	    race->max_stats[i] = 18;
	}

	if (ch->desc->editor == ED_OBJECT)
	    return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);
	
	ch->desc->pEdit = (void *)race;
	ch->desc->editor = ED_RACE;
	races_changed = TRUE;
	raedit_show(ch, "");

	return;
    }
    
    if ((i = race_lookup(arg)) == -1)
    {
	printf_to_char("Unknown race [%s]\n\r", ch, arg);
	return;
    }

    if (ch->desc->editor == ED_OBJECT)
	return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);

    ch->desc->pEdit  = (void *)&race_table[i];
    ch->desc->editor = ED_RACE;
    raedit_show(ch, "");
    return;
}

bool raedit_show(CHAR_DATA *ch, char *argument)
{
    struct race_type *race;
    int i;

    EDIT_RACE(ch, race);

    printf_to_char("Filename: [%s]\n\r", ch, race->filename);
    printf_to_char("Name    : [%s] MName: [%s] FName: [%s] PC: [%s]\n\r",
                   ch, race->name, race->rname, race->fname,
                   race->pc_race == TRUE ? "Yes" : "No");
    printf_to_char("Act     : %s\n\r",
                   ch, flag_string(act_flags, race->act, FALSE));
    printf_to_char("Affects : %s\n\r",
                   ch, flag_string(affect_flags, race->aff, FALSE));
    printf_to_char("Off     : %s\n\r",
                   ch, flag_string(off_flags, race->off, FALSE));

    for (i = 0; i < DAM_MAX; i++)
    {
        if (race->resists[i] != 0)
            printf_to_char("Res     : %-10.10s: %4d%%\n\r",
                           ch, flag_string(dam_flags, i, FALSE),
                           race->resists[i]);
    }

    printf_to_char("Parts   : %s\n\r",
                   ch, flag_string(part_flags, race->parts, FALSE));
    printf_to_char("Form    : %s\n\r",
                   ch, flag_string(form_flags, race->form, FALSE));

    if (race->pc_race == TRUE)
    {
        printf_to_char("WhoName : [%s]\n\r", ch, race->who_name);
        printf_to_char("Points  : [%d] Size: [%s] ",
                       ch, race->points,
                       flag_string(size_flags, race->size, FALSE));
	printf_to_char("Aligns: [%s]\n\r",
		       ch, flag_string(align_flags, race->valid_align, FALSE));
        printf_to_char("Recall  : [%d] Map VNum: [%d]\n\r",
                       ch, race->recall, race->map_vnum);
        printf_to_char("Stats   : [", ch);
        for (i = 0; i < MAX_STATS; i++)
        {
            printf_to_char("%s: %d/%d", ch, flag_string(attr_flags, i, FALSE),
                           race->stats[i], race->max_stats[i]);
            if (i != MAX_STATS - 1)
                printf_to_char(" ", ch);
        }
        printf_to_char("]\n\r", ch);
        printf_to_char("Skills  :", ch);
        for (i = 0; i < 5; i++)
            if (!IS_NULLSTR(race->skills[i]))
                printf_to_char(" '%s'", ch, race->skills[i]);
        printf_to_char("\n\r", ch);

        for (i = 0; i < max_classes; i++)
            printf_to_char("Mult. for class [%s]: %d%%\n\r",
                           ch, class_table[i].name, race->class_mult[i]);
    }

    return FALSE;
}

bool raedit_name(CHAR_DATA *ch, char *argument)
{
    struct race_type *race;

    EDIT_RACE(ch, race);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: name <race name>\n\r", ch);
	return FALSE;
    }

    free_string(race->name);
    race->name = str_dup(argument);

    send_to_char("Name set.\n\r", ch);
    return TRUE;
}

bool raedit_mname(CHAR_DATA *ch, char *argument)
{
    struct race_type *race;

    EDIT_RACE(ch, race);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: mname <male race name>\n\r", ch);
	return FALSE;
    }

    free_string(race->rname);
    race->rname = str_dup(argument);

    send_to_char("Male name set.\n\r", ch);
    return TRUE;
}

bool raedit_fname(CHAR_DATA *ch, char *argument)
{
    struct race_type *race;

    EDIT_RACE(ch, race);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: fname <female race name>\n\r", ch);
	return FALSE;
    }

    free_string(race->fname);
    race->fname = str_dup(argument);

    send_to_char("Female name set.\n\r", ch);
    return TRUE;
}

bool raedit_act(CHAR_DATA *ch, char *argument)
{
    int64_t bit;
    struct race_type *race;

    EDIT_RACE(ch, race);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: act <act flags>\n\r", ch);
	return FALSE;
    }

    if ((bit = flag_value(act_flags, argument)) == NO_FLAG)
    {
	send_to_char("Unknown act.\n\rType '? act' for a list of flags.\n\r", ch);
	return FALSE;
    }

    TOGGLE_BIT(race->act, bit);

    send_to_char("Act flags toggle.\n\r", ch);
    return TRUE;
}

bool raedit_off(CHAR_DATA *ch, char *argument)
{
    int64_t bit;
    struct race_type *race;

    EDIT_RACE(ch, race);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: off <off flags>\n\r", ch);
	return FALSE;
    }

    if ((bit = flag_value(off_flags, argument)) == NO_FLAG)
    {
	send_to_char("Unknown off.\n\rType '? off' for a list of flags.\n\r", ch);
	return FALSE;
    }

    TOGGLE_BIT(race->off, bit);

    send_to_char("Off flags toggle.\n\r", ch);
    return TRUE;
}

bool raedit_aff(CHAR_DATA *ch, char *argument)
{
    int64_t bit;
    struct race_type *race;

    EDIT_RACE(ch, race);
    CHECK_EDIT_PC(ch, race);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: aff <aff flags>\n\r", ch);
	return FALSE;
    }

    if ((bit = flag_value(affect_flags, argument)) == NO_FLAG)
    {
	send_to_char("Unknown affect.\n\rType '? aff' for a list of flags.\n\r", ch);
	return FALSE;
    }

    TOGGLE_BIT(race->aff, bit);

    send_to_char("Affect flags toggle.\n\r", ch);
    return TRUE;
}

bool raedit_form(CHAR_DATA *ch, char *argument)
{
    int64_t bit;
    struct race_type *race;

    EDIT_RACE(ch, race);
    CHECK_EDIT_PC(ch, race);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: form <form flags>\n\r", ch);
	return FALSE;
    }

    if ((bit = flag_value(form_flags, argument)) == NO_FLAG)
    {
	send_to_char("Unknown form.\n\rType '? form' for a list of flags.\n\r", ch);
	return FALSE;
    }

    TOGGLE_BIT(race->form, bit);

    send_to_char("Form flags toggle.\n\r", ch);
    return TRUE;
}

bool raedit_parts(CHAR_DATA *ch, char *argument)
{
    int64_t bit;
    struct race_type *race;

    EDIT_RACE(ch, race);
    CHECK_EDIT_PC(ch, race);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: parts <parts flags>\n\r", ch);
	return FALSE;
    }

    if ((bit = flag_value(part_flags, argument)) == NO_FLAG)
    {
	send_to_char("Unknown parts.\n\rType '? parts' for a list of flags.\n\r", ch);
	return FALSE;
    }

    TOGGLE_BIT(race->parts, bit);

    send_to_char("Parts flags toggle.\n\r", ch);
    return TRUE;
}

bool raedit_res(CHAR_DATA *ch, char *argument)
{
    int64_t res;
    char arg[MAX_STRING_LENGTH];
    struct race_type *race;

    EDIT_RACE(ch, race);
    CHECK_EDIT_PC(ch, race);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: res <res type> <percent>\n\r", ch);
	return FALSE;
    }

    argument = one_argument(argument, arg);
    
    if ((res = flag_value(dam_flags, arg)) == NO_FLAG
	|| !is_number(argument))
    {
	send_to_char("Unknown resist.\n\rType '? res' for a list of flags.\n\r", ch);
	return FALSE;
    }

    race->resists[res] = atoi(argument);
    
    printf_to_char("Resist from %s set to %d.\n\r",
		   ch, arg, race->resists[res]);
    return TRUE;
}

bool raedit_whoname(CHAR_DATA *ch, char *argument)
{
    struct race_type *race;

    EDIT_RACE(ch, race);
    CHECK_PC(ch, race);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: whoname <race name>\n\r", ch);
	return FALSE;
    }

    free_string(race->who_name);
    race->who_name = str_dup(argument);

    send_to_char("Whoname set.\n\r", ch);
    return TRUE;
}

bool raedit_points(CHAR_DATA *ch, char *argument)
{
    struct race_type *race;

    EDIT_RACE(ch, race);
    CHECK_PC(ch, race);

    if (argument[0] == '\0' || !is_number(argument))
    {
	send_to_char("Syntax: points <points>\n\r", ch);
	return FALSE;
    }

    race->points = atoi(argument);

    send_to_char("Points set.\n\r", ch);
    return TRUE;
}

bool raedit_size(CHAR_DATA *ch, char *argument)
{
    int64_t size;
    struct race_type *race;

    EDIT_RACE(ch, race);
    CHECK_PC(ch, race);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: size <size>\n\r", ch);
	return FALSE;
    }

    if ((size = flag_value(size_flags, argument)) == NO_FLAG)
    {
	printf_to_char("Unknown size [%s].\n\r"
		       "Type '? size' for a list of sizes.\n\r",
		       ch, argument);
	return FALSE;
    }

    race->size = size;

    send_to_char("Size set.\n\r", ch);
    return TRUE;
}

bool raedit_recall(CHAR_DATA *ch, char *argument)
{
    int vnum;
    struct race_type *race;
    ROOM_INDEX_DATA *room;

    EDIT_RACE(ch, race);
    CHECK_PC(ch, race);

    if (argument[0] == '\0' || !is_number(argument))
    {
	send_to_char("Syntax: recall <recall VNum>\n\r", ch);
	return FALSE;
    }

    vnum = atoi(argument);
    
    if ((room = get_room_index(vnum)) == NULL)
    {
	printf_to_char("Room [%d] does not exist!\n\r", ch, vnum);
	return FALSE;
    }
    
    race->recall = vnum;

    printf_to_char("Recall set to [%s].\n\r", ch, room->name);
    return TRUE;
}

bool raedit_map(CHAR_DATA *ch, char *argument)
{
    int vnum;
    struct race_type *race;
    OBJ_INDEX_DATA *obj;

    EDIT_RACE(ch, race);
    CHECK_PC(ch, race);

    if (argument[0] == '\0' || !is_number(argument))
    {
	send_to_char("Syntax: map <map VNum>\n\r", ch);
	return FALSE;
    }

    vnum = atoi(argument);
    
    if ((obj = get_obj_index(vnum)) == NULL)
    {
	printf_to_char("Obj [%d] does not exist!\n\r", ch, vnum);
	return FALSE;
    }

    if (obj->item_type != ITEM_MAP)
    {
	printf_to_char("Obj [%s] is not a map!\n\r", ch, obj->short_descr);
	return FALSE;
    }
    
    race->map_vnum = vnum;

    printf_to_char("Map set to [%s].\n\r", ch, obj->short_descr);
    return TRUE;
}

bool raedit_align(CHAR_DATA *ch, char *argument)
{
    int64_t align;
    int i;
    struct race_type *race;

    EDIT_RACE(ch, race);
    CHECK_PC(ch, race);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: align <aligns>\n\r", ch);
	return FALSE;
    }

    if (argument[0] == '?')
    {
	for (i = 0; !IS_NULLSTR(align_flags[i].name); i++)
	{
	    if (i % 3 == 0)
		send_to_char("\n\r", ch);
	    printf_to_char("%-10.10s ", ch, align_flags[i].name);
	}
	send_to_char("\n\r", ch);
	return FALSE;
    }

    if ((align = flag_value(align_flags, argument)) == NO_FLAG)
    {
	printf_to_char("Unknown align [%s].\n\r"
		       "Type 'align ?' for a list of aligns.\n\r",
		       ch, argument);
	return FALSE;
    }

    TOGGLE_BIT(race->valid_align, align);

    send_to_char("Aligns toggled.\n\r", ch);
    return TRUE;
}

static bool ra_stat(CHAR_DATA *ch, char *argument, int stat)
{
    char buf[MAX_STRING_LENGTH];
    struct race_type *race;

    EDIT_RACE(ch, race);
    CHECK_PC(ch, race);

    argument = one_argument(argument, buf);
    
    if (IS_NULLSTR(argument) || IS_NULLSTR(buf)
	|| !is_number(argument) || !is_number(buf))
    {
	send_to_char("Syntax: <stat> <min> <max>\n\r", ch);
	return FALSE;
    }

    race->stats[stat] = URANGE(3, atoi(buf), 30);
    race->max_stats[stat] = URANGE(3, atoi(argument), 30);

    send_to_char("Stats set.\n\r", ch);
    return TRUE;
}

bool raedit_str(CHAR_DATA *ch, char *argument)
{
    return ra_stat(ch, argument, STAT_STR);
}

bool raedit_int(CHAR_DATA *ch, char *argument)
{
    return ra_stat(ch, argument, STAT_INT);
}

bool raedit_wis(CHAR_DATA *ch, char *argument)
{
    return ra_stat(ch, argument, STAT_WIS);
}

bool raedit_dex(CHAR_DATA *ch, char *argument)
{
    return ra_stat(ch, argument, STAT_DEX);
}

bool raedit_con(CHAR_DATA *ch, char *argument)
{
    return ra_stat(ch, argument, STAT_CON);
}

bool raedit_skill(CHAR_DATA *ch, char *argument)
{
    int i, gsn;
    int free_slot = -1;
    struct race_type *race;

    EDIT_RACE(ch, race);
    CHECK_PC(ch, race);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: skill <skill>\n\r", ch);
	return FALSE;
    }

    if ((gsn = skill_lookup(argument)) == -1)
    {
	printf_to_char("Unknown skill [%s].\n\r", ch, argument);
	return FALSE;
    }

    for (i = 0; i < 5; i++)
    {
	if (IS_NULLSTR(race->skills[i]) && free_slot == -1)
	    free_slot = i;
	else if (gsn == skill_lookup(race->skills[i]))
	{
	    free_string(race->skills[i]);
	    race->skills[i] = str_dup("");
	    printf_to_char("Skill [%s] removed.\n\r",
			   ch, skill_table[gsn].name);
	    return TRUE;
	}
    }
    
    if (free_slot == -1)
    {
	send_to_char("Can't find empty slot.\n\r", ch);
	return FALSE;
    }
    
    free_string(race->skills[free_slot]);
    race->skills[free_slot] = str_dup(argument);

    printf_to_char("Add skill [%s].\n\r", ch, skill_table[gsn].name);
    return TRUE;
}

bool raedit_class(CHAR_DATA *ch, char *argument)
{
    int classid;
    char buf[MAX_STRING_LENGTH];
    struct race_type *race;

    EDIT_RACE(ch, race);
    CHECK_PC(ch, race);

    argument = one_argument(argument, buf);
    
    if (IS_NULLSTR(argument) || IS_NULLSTR(buf) || !is_number(argument)
	|| (classid = class_lookup(buf)) == -1)
    {
	send_to_char("Syntax: class <class name> <exp multiplier>\n\r", ch);
	return FALSE;
    }

    race->class_mult[classid] = atoi(argument);

    printf_to_char("Class [%s]: multiplier set to %d%%.\n\r",
		   ch, class_table[classid].name, atoi(argument));
    return TRUE;
}

/* charset=cp1251 */
