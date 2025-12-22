#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "merc.h"
#include "olc.h"



bool cledit(CHAR_DATA *ch, char *argument)
{
    int i;
    char arg[MAX_STRING_LENGTH];
    char cmd[MAX_STRING_LENGTH];
    struct class_type *pClass;

    EDIT_CLASS(ch, pClass);
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
	cledit_show(ch, argument);
	return FALSE;
    }

    for (i = 0; cledit_table[i].name != NULL; i++)
	if (!str_prefix(cmd, cledit_table[i].name))
	{
	    if ((*cledit_table[i].olc_fun)(ch, argument))
		pClass->changed = TRUE;
	    return TRUE;
	}

    interpret(ch, arg);
    return FALSE;
}

void do_cledit(CHAR_DATA *ch, char *argument)
{
    int i, numc;
    char arg[MAX_STRING_LENGTH];
    char filename[MAX_STRING_LENGTH];
    struct stat st;
    struct class_type *pClass;

    if (IS_NPC(ch))
	return;

    argument = one_argument(argument, arg);

    if (!str_cmp(arg, "list"))
    {
	send_to_char("Classes list:\n\r",ch);
	for (i = 0; i < max_classes && !IS_NULLSTR(class_table[i].name); i++)
	    printf_to_char("              %s\n\r", ch, class_table[i].name);
	return;
    } else if (!str_cmp(arg, "create"))
    {
	if (IS_NULLSTR(argument))
	{
	    send_to_char("Syntax: cledit create <filename>\n\r", ch);
	    return;
	}

	if (max_classes == MAX_CLASS)
	{
	    send_to_char("You can't create class. Increase MAX_CLASS in merc.h\n\r",ch);
	    return;
	}

	snprintf(filename, sizeof(filename), "%s%s", CLASSES_DIR, argument);
	if (stat(filename, &st) != -1 || errno != ENOENT)
	{
	    printf_to_char("Filename [%s] already exist.\n\r", ch, filename);
	    return;
	}

	numc = max_classes;
	max_classes++;

	if ((pClass = realloc(class_table, sizeof(struct class_type) * (max_classes))) == NULL)
	{
	    printf_to_char("Can't allocate %ld bytes!\n\r", 
			ch, sizeof(struct class_type) * (max_classes));
	    bugf("Can't allocate %d bytes!", sizeof(struct class_type) * (max_classes));
	    return;
	}

	class_table = pClass;
	pClass = &class_table[numc];

	snprintf(filename, sizeof(filename), "%s.class", argument);
	pClass->filename 	= str_dup(filename);
	pClass->name		= str_dup(argument);
	pClass->who_name	= str_dup(argument);
	pClass->attr_prime	= 0;
	pClass->attr_secondary	= 0;
	pClass->weapon      	= 0;
	pClass->skill_adept    	= 75;
	pClass->thac0_00       	= 0;
	pClass->thac0_32       	= 0;
	pClass->hp_min         	= 0;
	pClass->hp_max         	= 0;
	pClass->fMana          	= 0;
	pClass->valid_align    	= 0;
	pClass->base_group     	= str_dup("");
	pClass->default_group  	= str_dup("");
	pClass->changed	  	= TRUE;

	for (i = 0; i < max_skills && !IS_NULLSTR(skill_table[i].name); i++)
	{
	    skill_table[i].skill_level = realloc(skill_table[i].skill_level, sizeof(int16_t) * max_classes);
	    skill_table[i].rating      = realloc(skill_table[i].rating, sizeof(int16_t) * max_classes);
	    skill_table[i].quest       = realloc(skill_table[i].quest, sizeof(int16_t) * max_classes);
	}

	for (i = 0; i < max_groups && !IS_NULLSTR(group_table[i].name); i++)
	    group_table[i].rating = realloc(group_table[i].rating, sizeof(int16_t) * max_classes);

	for (i = 0; i < max_skills; i++)
	{
	    skill_table[i].skill_level[numc] = ANGEL;
	    skill_table[i].rating[numc]      = 0;
	    skill_table[i].quest[numc]	 = FALSE;
	}

	for (i = 0; i < max_groups; i++)
	    group_table[i].rating[numc] = -1;

	for (i = 0; i < MAX_PC_RACE; i++)
	    pClass->guild[i] = 0;

	for (i = 0; i <= MAX_LEVEL; i++)
	{
	    title_table[numc][i][0] = str_dup("");
	    title_table[numc][i][1] = str_dup("");
	}

	if (ch->desc->editor == ED_OBJECT)
	    return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);

	ch->desc->pEdit = (void *)pClass;
	ch->desc->editor = ED_CLASS;
	classes_changed = TRUE;
	cledit_show(ch, "");

	return;
    }

    if ((i = class_lookup(arg)) == -1)
    {
	printf_to_char("Unknown class [%s]\n\r", ch, arg);
	return;
    }

    if (ch->desc->editor == ED_OBJECT)
	return_affects((OBJ_INDEX_DATA *) ch->desc->pEdit, FALSE);

    ch->desc->pEdit  = (void *)&class_table[i];
    ch->desc->editor = ED_CLASS;
    cledit_show(ch, "");
    return;
}

void do_clsave(CHAR_DATA *ch, char *argument)
{
    save_classes(ch);
}

bool cledit_show(CHAR_DATA *ch, char *argument)
{
    struct class_type *st;
    char buf[MAX_STRING_LENGTH];
    int i = 0, j;
 
    EDIT_CLASS(ch, st);
    i = class_lookup(st->name);

    sprintf(buf, "Filename: %s\n\r", st->filename);
    send_to_char(buf, ch);

    sprintf(buf, "Name: %s\n\r", st->name);
    send_to_char(buf, ch);

    sprintf(buf, "Wname: %s\n\r", st->who_name);
    send_to_char(buf, ch);
    
    sprintf(buf, "Primary: %s\n\r", fflag_string(attr_flags, st->attr_prime, FALSE, FALSE));
    send_to_char(buf, ch);

    sprintf(buf, "Secondary: %s\n\r", fflag_string(attr_flags, st->attr_secondary, FALSE, FALSE));
    send_to_char(buf, ch);

    sprintf(buf, "Weapon: %s\n\r", fflag_string(weapon_class, st->weapon, FALSE, FALSE));
    send_to_char(buf, ch);

    sprintf(buf, "SkillAdept: %d\n\r", st->skill_adept);
    send_to_char(buf, ch);

    sprintf(buf, "THAC0: %d %d\n\r", st->thac0_00, st->thac0_32);
    send_to_char(buf, ch);

    sprintf(buf, "MinHP: %d\n\r", st->hp_min);
    send_to_char(buf, ch);

    sprintf(buf, "MaxHP: %d\n\r", st->hp_max);
    send_to_char(buf, ch);

    sprintf(buf, "MagicUser: %s\n\r", magic_class_flags[st->fMana].name);
    send_to_char(buf, ch);

    sprintf(buf, "BaseGroup: %s\n\r", st->base_group);
    send_to_char(buf, ch);

    sprintf(buf, "DefaultGroup: %s\n\r", st->default_group);
    send_to_char(buf, ch);

    sprintf(buf, "Align: %s\n\r", fflag_string(align_flags, st->valid_align, FALSE, FALSE));
    send_to_char(buf, ch);

    send_to_char("Guilds: ", ch);
    for (j = 0; j < MAX_PC_RACE; j++)
    {
	sprintf(buf, "%d ", class_table[i].guild[j]);
	send_to_char(buf, ch);
    }
    send_to_char("\n\r", ch);

    if (IS_NULLSTR(argument))
    {
	send_to_char("Groups syntax: 'show groups'\n\r", ch);
	send_to_char("Skills syntax: 'show skills'\n\r", ch);
	send_to_char("Titles syntax: 'show titles'\n\r", ch);
    } else if (!str_prefix(argument,"groups"))
    {
	send_to_char("Groups:\n\r"
		    "* Not shown groups are not available to this class\n\r"
		    "* Group      '<group name>'      <difficulty rating>\n\r",ch);

	for (j = 0; j < max_groups && !IS_NULLSTR(group_table[j].name); j++)
	    if (group_table[j].rating[i] > -1)
	    {
		sprintf(buf, "Group:       %-22.22s %d\n\r", 
			group_table[j].name, 
			group_table[j].rating[i]);
		send_to_char(buf, ch);
	    }
	send_to_char("Skills syntax: 'show skills'\n\r", ch);
	send_to_char("Titles syntax: 'show titles'\n\r", ch);
    } else if (!str_prefix(argument,"skills"))
    {
	send_to_char("Groups syntax: 'show groups'\n\r", ch);
	send_to_char("Skills/spells:\n\r"
		    "* Not shown skills are not available to this class\n\r"
		    "* Skill <level> <difficulty> '<name>'                       <quest>\n\r",ch);
	for (j = 0; j <= max_skills && !IS_NULLSTR(skill_table[j].name); j++)
	    if (skill_table[j].skill_level[i] <= LEVEL_HERO)
	    {
		sprintf(buf, "Skill:  %-2d      %d            %-30.30s %s\n\r",
			skill_table[j].skill_level[i],
			skill_table[j].rating[i],
			skill_table[j].name,
			skill_table[j].quest[i] ? "true" : "false");
		send_to_char(buf, ch);
	    }

	send_to_char("Titles syntax: 'show titles'\n\r", ch);
    } else if (!str_prefix(argument,"titles"))
    {
	send_to_char("Groups syntax: 'show groups'\n\r", ch);
	send_to_char("Skills syntax: 'show skills'\n\r", ch);
	send_to_char("Titles:\n\r", ch);
	for (j = 0; j <= MAX_LEVEL; j++)
	{
	    sprintf(buf, "MTitle: %-2d %s\n\r", j, title_table[i][j][0]);
	    send_to_char(buf, ch);

	    sprintf(buf, "FTitle: %-2d %s\n\r", j, title_table[i][j][1]);
	    send_to_char(buf, ch);
	}
    }
    return FALSE;
}


bool cledit_name(CHAR_DATA *ch, char *argument)
{
    struct class_type *pclass;

    EDIT_CLASS(ch, pclass);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: name <class name>\n\r", ch);
	return FALSE;
    }

    free_string(pclass->name);
    pclass->name = str_dup(argument);

    send_to_char("Name set.\n\r", ch);
    return TRUE;
}

bool cledit_wname(CHAR_DATA *ch, char *argument)
{
    struct class_type *pclass;

    EDIT_CLASS(ch, pclass);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: wname <class name>\n\r", ch);
	return FALSE;
    }

    free_string(pclass->who_name);
    pclass->who_name = str_dup(argument);

    send_to_char("Russian name set.\n\r", ch);
    return TRUE;
}

bool cledit_primary(CHAR_DATA *ch, char *argument)
{                     
    struct class_type *pclass;

    EDIT_CLASS(ch, pclass);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: primary <str/int/wis/dex/con/none>\n\r", ch);
	return FALSE;
    }

    if (!str_prefix(argument,"none"))
	 pclass->attr_prime = NO_FLAG;
    else
    {
	if (flag_value(attr_flags, argument) == NO_FLAG)
	{
	    send_to_char("Unknown flag.\n\r", ch);
	    return FALSE;
	}

	pclass->attr_prime = flag_value(attr_flags, argument);
    }
    send_to_char("Primary set.\n\r", ch);
    return TRUE;
}

bool cledit_secondary(CHAR_DATA *ch, char *argument)
{                     
    struct class_type *pclass;

    EDIT_CLASS(ch, pclass);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: secondary <str/int/wis/dex/con/none>\n\r", ch);
	return FALSE;
    }

    if (!str_prefix(argument,"none"))
	pclass->attr_secondary = NO_FLAG;
    else
    {
	if (flag_value(attr_flags, argument) == NO_FLAG)
	{
	    send_to_char("Unknown flag.\n\r", ch);
	    return FALSE;
	}
	pclass->attr_secondary = flag_value(attr_flags, argument);
    }

    send_to_char("Secondary set.\n\r", ch);
    return TRUE;
}

bool cledit_weapon(CHAR_DATA *ch, char *argument)
{                     
    struct class_type *pclass;

    EDIT_CLASS(ch, pclass);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: weapon <weapon type/none>\n\r", ch);
	return FALSE;
    }

    if (!str_prefix(argument,"none"))
	pclass->weapon = NO_FLAG;
    {
	if (flag_value(weapon_class, argument) == NO_FLAG)
	{
	    send_to_char("Unknown weapon.\n\r", ch);
	    return TRUE;
	}

	pclass->weapon = flag_value(weapon_class, argument);
    }

    send_to_char("Weapon set.\n\r", ch);
    return TRUE;
}

bool cledit_skill_adept(CHAR_DATA *ch, char *argument)
{                     
    struct class_type *pclass;

    EDIT_CLASS(ch, pclass);

    if (argument[0] == '\0' || !is_number(argument))
    {
	send_to_char("Syntax: sadept <number>\n\r", ch);
	return FALSE;
    }

    if (atoi(argument) < 0)
    {
	send_to_char("Set skill adept more 0. \n\r", ch);
	return FALSE;
    }

    pclass->skill_adept = atoi(argument);

    send_to_char("Skill adept set.\n\r", ch);
    return TRUE;
}

bool cledit_hp_min(CHAR_DATA *ch, char *argument)
{                     
    struct class_type *pclass;

    EDIT_CLASS(ch, pclass);

    if (argument[0] == '\0' || !is_number(argument))
    {
	send_to_char("Syntax: hpmin <number>\n\r", ch);
	return FALSE;
    }

    if (atoi(argument) < 0 || atoi(argument) > pclass->hp_max)
    {
	send_to_char("Set min hp between 0 and maxhp. \n\r", ch);
	return FALSE;
    }

    pclass->hp_min = atoi(argument);

    send_to_char("Hp min set.\n\r", ch);
    return TRUE;
}

bool cledit_hp_max(CHAR_DATA *ch, char *argument)
{                     
    struct class_type *pclass;

    EDIT_CLASS(ch, pclass);

    if (argument[0] == '\0' || !is_number(argument))
    {
	send_to_char("Syntax: maxhp <number>\n\r", ch);
	return FALSE;
    }

    if (atoi(argument) < 0 || atoi(argument) < pclass->hp_min)
    {
	send_to_char("Set max hp more 0 and minhp. \n\r", ch);
	return FALSE;
    }

    pclass->hp_max = atoi(argument);

    send_to_char("Hp max set.\n\r", ch);
    return TRUE;
}

bool cledit_fmana(CHAR_DATA *ch, char *argument)
{                     
    struct class_type *pclass;

    EDIT_CLASS(ch, pclass);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: magicuser <nomagic/slightly/semimagic/magic>\n\r", ch);
	return FALSE;
    }

    if (flag_value(magic_class_flags, argument) == NO_FLAG)
    {
	send_to_char("Unknown flags.\n\r", ch);
	return FALSE;
    }

    pclass->fMana = flag_value(magic_class_flags, argument);

    send_to_char("Magic user set.\n\r", ch);
    return TRUE;
}

bool cledit_mtitle(CHAR_DATA *ch, char *argument)
{                     
    char lev[MAX_STRING_LENGTH];
    struct class_type *pclass;
    int i,j;

    EDIT_CLASS(ch, pclass);

    i = class_lookup(pclass->name);
    if (i == -1)
    {
	printf_to_char("Unknown class [%s]\n\r", ch, pclass->name);
	return FALSE;
    }

    if (argument[0] == '\0')
    {
	send_to_char("Male titles:\n\r",ch);
	for (j = 0; j <= MAX_LEVEL; j++)
	{
	    sprintf(lev, "MTitle: %-2d %s\n\r", j, title_table[i][j][0]);
	    send_to_char(lev, ch);
	}
	return FALSE;
    } 

    argument = one_argument(argument, lev);

    if (lev[0] == '\0' || argument[0] == '\0')
    {
	send_to_char("Syntax: mtitle <level> <title name>\n\r", ch);
	return FALSE;
    }

    if (!is_number(lev) || atoi(lev) < 0 || atoi(lev) > 60)
    {
	send_to_char("Level must be number between 0 - 60.\n\r", ch);
	return FALSE;
    }

    j = atoi(lev);

    free_string(title_table[i][j][0]);
    title_table[i][j][0] = str_dup(argument);

    send_to_char("Set male title.\n\r", ch);
    return TRUE;
}

bool cledit_ftitle(CHAR_DATA *ch, char *argument)
{                     
    char lev[MAX_STRING_LENGTH];
    struct class_type *pclass;
    int i,j;

    EDIT_CLASS(ch, pclass);

    i = class_lookup(pclass->name);
    if (i == -1)
    {
	printf_to_char("Unknown class [%s]\n\r", ch, pclass->name);
	return FALSE;
    }

    if (argument[0] == '\0')
    {
	send_to_char("Female titles:\n\r",ch);
	for (j = 0; j <= MAX_LEVEL; j++)
	{
	    sprintf(lev, "FTitle: %-2d %s\n\r", j, title_table[i][j][1]);
	    send_to_char(lev, ch);
	}
	return FALSE;
    } 

    argument = one_argument(argument, lev);

    if (lev[0] == '\0' || argument[0] == '\0')
    {
	send_to_char("Syntax: ftitle <level> <title name>\n\r", ch);
	return FALSE;
    }

    if (!is_number(lev) || atoi(lev) < 0 || atoi(lev) > 60)
    {
	send_to_char("Level must be number between 0 - 60.\n\r", ch);
	return FALSE;
    }

    j = atoi(lev);

    free_string(title_table[i][j][1]);
    title_table[i][j][1] = str_dup(argument);

    send_to_char("Set female title.\n\r", ch);
    return TRUE;
}

bool cledit_skills(CHAR_DATA *ch, char *argument)
{                     
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    char arg3[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    struct class_type *pclass;
    int cl,j;

    EDIT_CLASS(ch, pclass);

    cl = class_lookup(pclass->name);
    if (cl == -1)
    {
	printf_to_char("Unknown class [%s]\n\r", ch, pclass->name);
	return FALSE;
    }

    if (argument[0] == '\0')
    {
	send_to_char("Skills/spells:\n\r"
		    "* Not shown skills are not available to this class\n\r"
		    "* Skill <level> <difficulty> '<name>'                       <quest>\n\r",ch);
	for (j = 0; j <= max_skills && !IS_NULLSTR(skill_table[j].name); j++)
	    if (skill_table[j].skill_level[cl] <= LEVEL_HERO)
	    {
		sprintf(buf, "Skill:  %-2d      %d            %-30.30s %s\n\r",
			skill_table[j].skill_level[cl],
			skill_table[j].rating[cl],
			skill_table[j].name,
			skill_table[j].quest[cl] ? "true" : "false");
		send_to_char(buf, ch);
	    }
	return FALSE;
    }

    argument = one_argument(argument, arg1);

    if (!str_prefix(arg1,"list") || !str_prefix(arg1,"список"))
    {	
	int col;
	STRING_DATA sorted_list[max_skills];

	col = 0;

	argument = one_argument(argument, arg2);

	for (j = 0; j < max_skills; j++)
	    if (arg2[0] == '\0')
		sorted_list[col++].name = skill_table[j].name;
	    else
		sorted_list[col++].name = skill_table[j].rname;

	sorted_list[col].name = NULL;

	qsort(sorted_list, col, sizeof(sorted_list[0]), compare_strings);

	col = 0;

	send_to_char("List all skills and spells:\n\r",ch);
	for (j = 0; !IS_NULLSTR(sorted_list[j].name); j++)
	{
	    sprintf(buf, "%-25s", sorted_list[j].name);
	    send_to_char(buf, ch);
	    if (++col % 3 == 0)
	        send_to_char("\n\r", ch);
	}

	if (col % 3 != 0)
	    send_to_char("\n\r", ch);
	return FALSE;
    } 
    else if (!str_prefix(arg1,"add") || !str_prefix(arg1,"добавить"))
    {
	bool quest = FALSE;

	argument = one_argument(argument, arg2);
	argument = one_argument(argument, arg3);

	if (arg2[0] == '\0')
	{
	    send_to_char("skills <add> <level> <difficalty> [quest] <skill name>\n\r",ch);
	    return FALSE;
	}

	if (!is_number(arg2) || atoi(arg2) > 60 || atoi(arg2) < 0)
	{
	    send_to_char("Level must be number between 0 - 60.\n\r",ch);
	    return FALSE;
	}
	if (!is_number(arg3) || atoi(arg3) > 100 || atoi(arg3) < 1)
	{                                                       
	    send_to_char("Difficulty must be number between 1 - 100.\n\r",ch);
	    return FALSE;
	}

	one_argument(argument, arg1);

	if (!str_prefix(arg1,"quest"))
	{
	    argument = one_argument(argument, arg1);
	    quest = TRUE;
	} 

	j = skill_lookup(argument);
	if (j == -1)
	{
	    printf_to_char("Unknown skill/spell [%s].\n\r", ch, argument);
	    return FALSE;
	}

	skill_table[j].quest[cl]	= quest;
	skill_table[j].skill_level[cl] 	= atoi(arg2);
	skill_table[j].rating[cl]      	= atoi(arg3);
	
	send_to_char("Add skill.\n\r", ch);
	return TRUE;
    }
    else if (!str_prefix(arg1,"delete") || !str_prefix(arg1,"удалить"))
    {
	j = skill_lookup(argument);
	if (j == -1)
	{
	    printf_to_char("Unknown skill/spell [%s].\n\r", ch, argument);
	    return FALSE;
	}

	skill_table[j].quest[cl]	= FALSE;
	skill_table[j].skill_level[cl] 	= ANGEL;
	skill_table[j].rating[cl]      	= 0;

	send_to_char("Delete skill.\n\r", ch);
	return TRUE;
    }	
    else if (!str_prefix(arg1,"help") || !str_prefix(arg1,"помощь"))
    {
	send_to_char("Syntax: skills <list> [russian] - показывает список всех умений и заклинаний.\n\r"
		     "        skills <add> <level> <difficalty> [quest] <skill name>\n\r"
		     "               добавляет умение/заклинание данной профессии\n\r"
		     "               level      - уровень появления\n\r"
		     "               difficalty - сложность в обучении\n\r"
		     "               quest      - дается по квесту, по умолчанию дается всегда\n\r"
		     "               skill name - название умения или заклинания (выбирается из skills list)\n\r"
		     "        skills <delete> <skill/spell name> - удаляет умение или заклинание.\n\r"
		     "        skills <help> - помощь.\n\r",ch);
    }
    return FALSE;    
}

bool cledit_thac0(CHAR_DATA *ch, char *argument)
{                     
    char arg1[MAX_STRING_LENGTH];
    struct class_type *pclass;

    EDIT_CLASS(ch, pclass);

    argument = one_argument(argument, arg1);

    if (arg1[0] == '\0' 
	|| !is_number(arg1) 
	|| argument[0] == '\0' 
	|| !is_number(argument))
    {
	send_to_char("Syntax: thac0 <number> <number>\n\r", ch);
	return FALSE;
    }

    if (atoi(arg1) > 20 || atoi(arg1) < -20)
    {
	send_to_char("THAC0_00 must be between -20 - 20\n\r", ch);
	return FALSE;
    }
    if (atoi(argument) > 20 || atoi(argument) < -20)
    {
	send_to_char("THAC0_32 must be between -20 - 20\n\r", ch);
	return FALSE;
    }

    pclass->thac0_00 = atoi(arg1);
    pclass->thac0_32 = atoi(argument);

    send_to_char("THAC0 set.\n\r", ch);
    return TRUE;
}

bool cledit_guilds(CHAR_DATA *ch, char *argument)
{
    struct class_type *pclass;
    char arg1[MAX_STRING_LENGTH];
    int i;
    bool gui = FALSE;

    EDIT_CLASS(ch, pclass);

    for (i = 0; i < MAX_PC_RACE; i++)
    {  
	argument = one_argument(argument, arg1);
	if (arg1[0] == '\0' || !is_number(arg1))
	{
	    printf_to_char("Syntax: guilds <number>...<number>  (count <= %d)\n\r", ch, MAX_PC_RACE);
	    break;
	}
	pclass->guild[i] = atoi(arg1);
	gui = TRUE;
    }
    if (gui)
	send_to_char("Guilds set.\n\r", ch);
    return gui;
}

bool cledit_align(CHAR_DATA *ch, char *argument)
{                     
    struct class_type *pclass;

    EDIT_CLASS(ch, pclass);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: align <evil/neutral/good>\n\r", ch);
	return FALSE;
    }

    pclass->valid_align = flag_value(align_flags, argument);

    send_to_char("Align set.\n\r", ch);
    return TRUE;
}

bool cledit_base_group(CHAR_DATA *ch, char *argument)
{                     
    struct class_type *pclass;
    int i;

    EDIT_CLASS(ch, pclass);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: basegroup <group name>\n\r", ch);
	return FALSE;
    }

    i = group_lookup(argument);

    if (i == -1)
    {
	printf_to_char("Unknown group [%s]\n\r", ch, argument);
	return FALSE;
    }

    free_string(pclass->base_group);
    pclass->base_group = str_dup(group_table[i].name);

    send_to_char("Base group set.\n\r", ch);
    return TRUE;
}

bool cledit_default_group(CHAR_DATA *ch, char *argument)
{                     
    struct class_type *pclass;
    int i;

    EDIT_CLASS(ch, pclass);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: defaultgroup <group name>\n\r", ch);
	return FALSE;
    }

    i = group_lookup(argument);

    if (i == -1)
    {
	printf_to_char("Unknown group [%s]\n\r", ch, argument);
	return FALSE;
    }

    free_string(pclass->default_group);
    pclass->default_group = str_dup(group_table[i].name);

    send_to_char("Default group set.\n\r", ch);
    return TRUE;
} 

bool cledit_groups(CHAR_DATA *ch, char *argument)
{                     
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    struct class_type *pclass;
    int cl,j;

    EDIT_CLASS(ch, pclass);

    cl = class_lookup(pclass->name);
    if (cl == -1)
    {
	printf_to_char("Unknown class [%s]\n\r", ch, pclass->name);
	return FALSE;
    }

    if (argument[0] == '\0')
    {
	send_to_char("Groups:\n\r"
		    "* Not shown groups are not available to this class\n\r"
		    "* Group      '<group name>'      <difficulty rating>\n\r",ch);

	for (j = 0; j < max_groups && !IS_NULLSTR(group_table[j].name); j++)
	    if (group_table[j].rating[cl] > -1)
	    {
		sprintf(buf, "Group:       %-22.22s %d\n\r", 
			group_table[j].name, 
			group_table[j].rating[cl]);
		send_to_char(buf, ch);
	    }
	return FALSE;
    }

    argument = one_argument(argument, arg1);

    if (!str_prefix(arg1,"list") || !str_prefix(arg1,"список"))
    {	
	int col;
	STRING_DATA sorted_list[max_groups];

	col = 0;

	argument = one_argument(argument, arg2);

	for (j = 0; j < max_groups; j++)
	    if (arg2[0] == '\0')
		sorted_list[col++].name = group_table[j].name;
	    else
		sorted_list[col++].name = group_table[j].rname;

	sorted_list[col].name = NULL;

	qsort(sorted_list, col, sizeof(sorted_list[0]), compare_strings);

	col = 0;

	send_to_char("List all skills and spells:\n\r",ch);
	for (j = 0; !IS_NULLSTR(sorted_list[j].name); j++)
	{
	    sprintf(buf, "%-25s", sorted_list[j].name);
	    send_to_char(buf, ch);
	    if (++col % 3 == 0)
	        send_to_char("\n\r", ch);
	}

	if (col % 3 != 0)
	    send_to_char("\n\r", ch);
	return FALSE;
    } 
    else if (!str_prefix(arg1,"add") || !str_prefix(arg1,"добавить"))
    {
	argument = one_argument(argument, arg2);

	if (arg2[0] == '\0')
	{
	    send_to_char("groups <add> <difficalty> <group name>\n\r",ch);
	    return FALSE;
	}

	if (!is_number(arg2) || atoi(arg2) > 100 || atoi(arg2) < 0)
	{
	    send_to_char("Difficulty must be number between 1 - 100.\n\r",ch);
	    return FALSE;
	}

	j = group_lookup(argument);
	if (j == -1)
	{
	    printf_to_char("Unknown group [%s].\n\r", ch, argument);
	    return FALSE;
	}

	group_table[j].rating[cl] = atoi(arg2);
	
	send_to_char("Add groups.\n\r", ch);
	return TRUE;
    }
    else if (!str_prefix(arg1,"delete") || !str_prefix(arg1,"удалить"))
    {
	j = group_lookup(argument);
	if (j == -1)
	{
	    printf_to_char("Unknown group [%s].\n\r", ch, argument);
	    return FALSE;
	}

	group_table[j].rating[cl] = -1;

	send_to_char("Delete group.\n\r", ch);
	return TRUE;
    }	
    else if (!str_prefix(arg1,"help") || !str_prefix(arg1,"помощь"))
    {
	send_to_char("Syntax: groups <list>     - показывает список групп.\n\r"
		     "        groups <add> <difficalty> <group name>\n\r"
		     "               добавляет группу данной профессии\n\r"
		     "               difficalty - сложность в обучении\n\r"
		     "               group name - название группы (выбирается из groups list)\n\r"
		     "        groups <delete> <group name> - удаляет умение или заклинание.\n\r"
		     "        groups <help> - помощь.\n\r",ch);
    }
    return FALSE;    
}

/* charset=cp1251 */
