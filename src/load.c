#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <dlfcn.h>


#include "merc.h"
#include "recycle.h"




bool skills_changed = FALSE;
bool groups_changed;
bool races_changed = FALSE;
bool classes_changed = FALSE;

DECLARE_SPELL_FUN(spell_null);

STAILQ_HEAD(SKILL_TAILQ, ttype) s_tailq = STAILQ_HEAD_INITIALIZER(s_tailq);
STAILQ_HEAD(RACE_TAILQ, ttype) r_tailq = STAILQ_HEAD_INITIALIZER(r_tailq);
STAILQ_HEAD(CLASS_TAILQ, ttype) c_tailq = STAILQ_HEAD_INITIALIZER(c_tailq);
STAILQ_HEAD(GROUP_TAILQ, ttype) g_tailq = STAILQ_HEAD_INITIALIZER(g_tailq);

struct ttype
{
    void *data;
    STAILQ_ENTRY(ttype) entries;
};


#define KEY(literal, field, value)		\
		if (!str_cmp(word, literal))	\
		{				\
		    field  = value;		\
		    fMatch = TRUE;		\
		    break;			\
		}

#define SKEY(string, field)			\
		if (!str_cmp(word, string))	\
		{				\
		    free_string(field);		\
		    field = fread_string(fp);	\
		    fMatch = TRUE;		\
		    break;			\
		}

#define FKEY(string, table, field, what)				\
		if (!str_cmp(word, string))				\
		{							\
		    char *sval = fread_string(fp);			\
		    int64_t val;						\
		    fMatch = TRUE;					\
		    if ((val = flag_value(table, sval)) == NO_FLAG)	\
			bugf("Unknown %s '%s'.", what, sval);		\
		    else						\
			field = val;					\
		    free_string(sval);					\
		}

#define BKEY(string, field)						\
		if (!str_cmp(word, string))				\
		{							\
		    char *sval = fread_word(fp);			\
		    fMatch = TRUE;					\
		    if (!str_cmp(sval, "true"))				\
			field = TRUE;					\
		    else if (!str_cmp(sval, "false"))			\
			field = FALSE;					\
		}

int max_races, max_classes, max_skills, max_groups, max_moons;


void load_skills();
void fread_skill(FILE *fp, struct skill_type *st);
void save_skills();
const char *spellname_lookup(SPELL_FUN *fun);
SPELL_FUN *spellfun_lookup(char *name);
void load_races();
static int save_races(CHAR_DATA *ch);
void fread_race(FILE *fp, struct race_type *race);
void fread_class(FILE *fp, struct class_type *classid, int cnum);
void fread_group(FILE *fp, struct group_type *gr);
void fread_moon(FILE *fp, struct moon_type *moon);
int16_t *gsn_lookup(char *name);
char *gsn_name_lookup(int16_t *gsn);

void add_skill_tailq(struct skill_type *skill)
{
    struct ttype *tmp;

    if (STAILQ_EMPTY(&s_tailq))
	STAILQ_INIT(&s_tailq);

    tmp = malloc(sizeof(struct ttype));
    tmp->data = malloc(sizeof(struct skill_type));
    memcpy(tmp->data, skill, sizeof(struct skill_type));
    STAILQ_INSERT_TAIL(&s_tailq, tmp, entries);
}

void free_skill_tailq()
{
    struct ttype *i1, *i2;

    i1 = STAILQ_FIRST(&s_tailq);
    while (i1 != NULL)
    {
	i2 = STAILQ_NEXT(i1, entries);
	free(i1->data);
	free(i1);
	i1 = i2;
    }
}

void add_race_tailq(struct race_type *race)
{
    struct ttype *tmp;

    if (STAILQ_EMPTY(&r_tailq))
	STAILQ_INIT(&r_tailq);

    tmp = malloc(sizeof(struct ttype));
    tmp->data = malloc(sizeof(struct race_type));
    memcpy(tmp->data, race, sizeof(struct race_type));
    STAILQ_INSERT_TAIL(&r_tailq, tmp, entries);
}

void free_race_tailq()
{
    struct ttype *i1, *i2;

    i1 = STAILQ_FIRST(&r_tailq);
    while (i1 != NULL)
    {
	i2 = STAILQ_NEXT(i1, entries);
	free(i1->data);
	free(i1);
	i1 = i2;
    }
}

void add_group_tailq(struct group_type *group)
{
    struct ttype *tmp;

    if (STAILQ_EMPTY(&g_tailq))
	STAILQ_INIT(&g_tailq);

    tmp = malloc(sizeof(struct ttype));
    tmp->data = malloc(sizeof(struct group_type));
    memcpy(tmp->data, group, sizeof(struct group_type));
    STAILQ_INSERT_TAIL(&g_tailq, tmp, entries);
}

void free_group_tailq()
{
    struct ttype *i1, *i2;

    i1 = STAILQ_FIRST(&g_tailq);
    while (i1 != NULL)
    {
	i2 = STAILQ_NEXT(i1, entries);
	free(i1->data);
	free(i1);
	i1 = i2;
    }
}

void load_moons()
{
    FILE *fp;
    char letter;
    char *word;
    struct moon_type *moon;
    struct moon_type *moons = NULL;
    int i = 0;

    if ((fp = fopen(MOONS_FILE, "r")) == NULL)
    {
	_perror(MOONS_FILE);
	bugf("Can not open %s.", MOONS_FILE);
	exit(1);
    }

    for (;;)
    {
	letter = fread_letter(fp);

	if (letter == '*')
	{
	    fread_to_eol(fp);
	    continue;
	}

	if (letter != '#')
	{
	    bugf("Load_moons: '#' not found.");
	    break;
	}

	word = fread_word(fp);
	if (!str_cmp(word, "MOON"))
	{
	    moon = alloc_mem(sizeof(struct moon_type));
	    fread_moon(fp, moon);
	    i++;
	    moons = realloc(moons, sizeof(struct moon_type) * i);
	    memcpy(&moons[i - 1], moon, sizeof(struct moon_type));
	    free_mem(moon, sizeof(struct moon_type)); 
	}
	else if (!str_cmp(word, "End"))
	{
	    break;
	}
	else
	{
	    bugf("Load_moons: bad section name '%s'.", word);
	    continue;
	}
    }

    fclose(fp);
    moons_data = alloc_mem(sizeof(struct moon_type) * i);
    memcpy(moons_data, moons, sizeof(struct moon_type) * i);
    free(moons);
    max_moons = i;
}

struct colour_schema *colour_schemas;
struct colour_schema *colour_schema_default;
struct colour_schema *schema_lookup(char *name)
{
    int i;

    for (i = 0; i < sizeof(colour_schemas) - sizeof(struct colour_schema); i++)
	if (!str_cmp(name, colour_schemas[i].name))
	    return &colour_schemas[i];

    return NULL;
}

void load_schemas()
{
    FILE *fp;
    char letter;
    char *word;
    int i = -1;
    bool def = FALSE;

    colour_schemas = NULL;

    fp = fopen(COL_SCH_FILE, "r");
    if (!fp)
    {
	bugf("load_schemas(): Can't open %s", COL_SCH_FILE);
	_perror(COL_SCH_FILE);
	exit(1);
    }

    for (;;)
    {
	letter = fread_letter(fp);

	if (letter == '*')
	{
	    fread_to_eol(fp);
	    continue;
	}
	if (letter != '#')
	{
	    bugf("load_schemas(): '#' not found.");
	    break;
	}
	
	word = fread_word(fp);
	if (!str_cmp(word, "SCHEMA"))
	{
	    i++;
	    colour_schemas = realloc(colour_schemas, sizeof(struct colour_schema) * (i + 1));
	    colour_schemas[i].name = str_dup(fread_word(fp));
	    colour_schemas[i].parent = NULL;

	    if (!str_cmp(colour_schemas[i].name, "default"))
	    {
		colour_schema_default = &colour_schemas[i];
		def = TRUE;
	    }

	    for (;;)
	    {
		int col_num, col_val;
		
		word = fread_word(fp);
		if (!str_cmp(word, "Parent"))
		{
		    word = fread_word(fp);
		    colour_schemas[i].parent = schema_lookup(word);
		    if (!colour_schemas[i].parent)
		    {
			bugf("load_schemas(): parent '%s' for schema '%s' not found.",
			     word, colour_schemas[i].name);
		    }
		}
		else if (!str_cmp(word, "RName"))
		{
		    colour_schemas[i].rname = fread_string_eol(fp);
		}
		else if ((col_num = flag_value(color_type_flags, word)) == NO_FLAG)
		{
		    bugf("load_schemas(): unknown field '%s'", word);
		}
		else
		{
		    word = fread_word(fp);
		    if ((col_val = flag_value(color_flags, word)) == NO_FLAG)
		    {
			bugf("load_schemas(): unknown color '%s', use 'white'", word);
			colour_schemas[i].colours[col_num].code = str_dup(C_WHITE);
			colour_schemas[i].colours[col_num].level = 0;
		    }
		    else
		    {
			switch (col_val)
			{
			case 'w':
			    colour_schemas[i].colours[col_num].code = str_dup(C_WHITE);
			    break;
			case 'W':
			    colour_schemas[i].colours[col_num].code = str_dup(C_B_WHITE);
			    break;
			case 'r':
			    colour_schemas[i].colours[col_num].code = str_dup(C_RED);
			    break;
			case 'R':
			    colour_schemas[i].colours[col_num].code = str_dup(C_B_RED);
			    break;
			case 'y':
			    colour_schemas[i].colours[col_num].code = str_dup(C_YELLOW);
			    break;
			case 'Y':
			    colour_schemas[i].colours[col_num].code = str_dup(C_B_YELLOW);
			    break;
			case 'g':
			    colour_schemas[i].colours[col_num].code = str_dup(C_GREEN);
			    break;
			case 'G':
			    colour_schemas[i].colours[col_num].code = str_dup(C_B_GREEN);
			    break;
			case 'c':
			    colour_schemas[i].colours[col_num].code = str_dup(C_CYAN);
			    break;
			case 'C':
			    colour_schemas[i].colours[col_num].code = str_dup(C_B_CYAN);
			    break;
			case 'b':
			    colour_schemas[i].colours[col_num].code = str_dup(C_BLUE);
			    break;
			case 'B':
			    colour_schemas[i].colours[col_num].code = str_dup(C_B_BLUE);
			    break;
			case 'm':
			    colour_schemas[i].colours[col_num].code = str_dup(C_MAGENTA);
			    break;
			case 'M':
			    colour_schemas[i].colours[col_num].code = str_dup(C_B_MAGENTA);
			    break;
			case 'D':
			    colour_schemas[i].colours[col_num].code = str_dup(C_D_GREY);
			    break;
			}
			if (def)
			    colour_schemas[i].colours[col_num].level = fread_number(fp);
		    }
		}
	    }

	    if (!def)
	    {
		int j;
	    
		for (j = 0; j < MAX_COLOUR; j++)
		    if (!colour_schemas[i].colours[j].code)
		    {
			colour_schemas[i].colours[j].code = str_dup(colour_schema_default->colours[j].code);
			colour_schemas[i].colours[j].level = colour_schema_default->colours[j].level;
		    }
	    }

	    def = FALSE;
	}
	else if (!str_cmp(word, "End"))
	{
	    break;
	}
	else
	{
	    bugf("load_schames(): bad section name '%s'.", word);
	    continue;
	}
    }
}

void fread_moon(FILE *fp, struct moon_type *moon)
{
    bool fMatch;
    char *word;
    int16_t last_set = 0, last_rise = 0;

    moon->name         = str_dup("");
    moon->state        = 0;
    moon->moonlight    = 0;
    moon->period       = 0;
    moon->state_period = 0;
    moon->type         = 0;
    moon->color        = 'w';


    log_string("Loading moon...");

    for (;;)
    {
	word = feof(fp) ? "End" : fread_word(fp);
	fMatch = FALSE;

	switch (UPPER(word[0]))
	{
	case '*':
	    fMatch = TRUE;
	    fread_to_eol(fp);
	    break;

	case 'C':
	    FKEY("Color", color_flags, moon->color, "color");
            break;

	case 'E':
	    if (!str_cmp(word, "End"))
	    {
                if (last_set < MAX_MOON_MESSAGES)
  		    moon->msg_set[last_set] = NULL;

    		if (last_rise < MAX_MOON_MESSAGES - 1)
    		    moon->msg_rise[last_rise] = NULL;

		return;
	    }	

	case 'N':
	    SKEY("Name", moon->name);
	    break;

	case 'M':
	    if (!str_cmp(word, "MsgRise"))
	    {
  		fMatch = TRUE;

  		if (last_rise == MAX_MOON_MESSAGES)
  		{
    		    fread_to_eol(fp);
    		    break;
  		}

  		free_string(moon->msg_rise[last_rise]);
  		moon->msg_rise[last_rise] = fread_string(fp);
  		last_rise++;
	    }
	    
	    if (!str_cmp(word, "MsgSet"))
	    {
  		fMatch = TRUE;

  		if (last_set == MAX_MOON_MESSAGES)
  		{
    		    fread_to_eol(fp);
    		    break;
  		}
	      
  		free_string(moon->msg_set[last_set]);
  		moon->msg_set[last_set] = fread_string(fp);
    		last_set++;
	    }
	    break;

	case 'P':
	    KEY("Period", moon->period, fread_number(fp));
	    break;

	case 'S':
	    KEY("StatePeriod", moon->state_period, fread_number(fp));
	    break;

	case 'T':
	    FKEY("Type", target_flags, moon->type, "moon type");
	    break;
	}
	
	if (!fMatch)
	    bugf("Fread_moon: no match '%s'.", word);
    }
}

void save_moons()
{
    FILE *fp;
    int i;

    if ((fp = fopen(MOONS_FILE, "w")) == NULL)
    {
	_perror(MOONS_FILE);
	bugf("Can not open %s for write.", MOONS_FILE);
	exit(1);
    }

    for (i = 0; i < max_moons; i++)
    {
        int j;

	fprintf(fp, "#MOON\n");
	fprintf(fp, "Name        %s~\n", moons_data[i].name);
	for (j = 0; j < MAX_MOON_MESSAGES && !IS_NULLSTR(moons_data[i].msg_rise[j]); j++)
	    fprintf(fp, "MsgRise     %s~\n", moons_data[i].msg_rise[j]);

        for (j = 0; j < MAX_MOON_MESSAGES && !IS_NULLSTR(moons_data[i].msg_set[j]); j++)
	    fprintf(fp, "MsgSet      %s~\n", moons_data[i].msg_set[j]);

	fprintf(fp, "Period      %d\n", moons_data[i].period);
	fprintf(fp, "StatePeriod %d\n", moons_data[i].state_period);
	fprintf(fp, "Type        %s~\n",
		fflag_string(target_flags, moons_data[i].type, FALSE, FALSE));
	fprintf(fp, "End\n\n");
    }

    fprintf(fp, "#End\n");
    fclose(fp);
}

void load_groups()
{
    FILE *fp;
    char letter;
    char *word;
    struct group_type *gr;
    struct ttype *tmp;
    int i;

    max_groups = 0;
    groups_changed = FALSE;

    if ((fp = fopen(GROUPS_FILE, "r")) == NULL)
    {
	_perror(GROUPS_FILE);
	bugf("Can not open %s.", GROUPS_FILE);
	exit(0);
    }

    for (;;)
    {
	letter = fread_letter(fp);

	if (letter == '*')
	{
	    fread_to_eol(fp);
	    continue;
	}

	if (letter != '#')
	{
	    bugf("Load_groups: '#' not found.");
	    break;
	}

	word = fread_word(fp);
	if (!str_cmp(word, "GROUP"))
	{
	    gr = malloc(sizeof(struct group_type));
	    fread_group(fp, gr);
	    add_group_tailq(gr);
	    free(gr);
	    max_groups++;
	    continue;
	}
	else if (!str_cmp(word, "End"))
	    break;
	else
	{
	    bugf("Load_groups: bad section name '%s'.", word);
	    continue;
	}
    }

    fclose(fp);

    group_table = malloc(sizeof(struct group_type) * max_groups);
    i = 0;
    STAILQ_FOREACH(tmp, &g_tailq, entries)
	memcpy(&group_table[i++], tmp->data, sizeof(struct group_type));
    free_group_tailq();
}


void fread_group(FILE *fp, struct group_type *gr)
{
    char *word;
    bool fMatch;
    int spell_num;

    gr->name   = str_dup("");
    gr->rname  = str_dup("");

    for (spell_num = 0; spell_num < MAX_IN_GROUP; spell_num++)
	gr->spells[spell_num] = str_dup("");

    spell_num = 0;

    for (;;)
    {
	word = feof(fp) ? "End" : fread_word(fp);
	fMatch = FALSE;

	switch (word[0])
	{
	case '*':
	    fMatch = TRUE;
	    fread_to_eol(fp);
	    break;
	case 'E':
	    if (!str_cmp(word, "End"))
		return;

	    break;
	case 'N':
	    SKEY("Name", gr->name);
	    break;
	case 'R':
	    SKEY("RName", gr->rname);
	    break;
	case 'S':
	    if (!str_cmp(word, "Spell"))
	    {
		free_string(gr->spells[spell_num]);
		gr->spells[spell_num] = fread_string(fp);
		spell_num++;
		fMatch = TRUE;
		break;
	    }
	    break;
	}

	if (!fMatch)
	    bugf("Fread_group: no match '%s'.", word);
    }
}

void save_groups()
{
    int i, j;
    FILE *fp;

    if ((fp = fopen(GROUPS_FILE, "w")) == NULL)
    {
	_perror(GROUPS_FILE);
	bugf("Save_groups: can not open %s for write.", GROUPS_FILE);
	return;
    }

    for (i = 0; i < max_groups; i++)
    {
	fprintf(fp, "#GROUP\n");
	fprintf(fp, "Name   %s~\n", group_table[i].name);
	fprintf(fp, "RName  %s~\n", group_table[i].rname);
	
	for (j = 0; j < MAX_IN_GROUP; j++)
	{
	    if (!IS_NULLSTR(group_table[i].spells[j]))
		fprintf(fp, "Spell  %s~\n", group_table[i].spells[j]);
	}
	fprintf(fp, "End\n\n");
    }

    fprintf(fp, "#End\n");
    fclose(fp);
}

void load_skills()
{
    FILE *fp;
    int i = 0;
    char letter;
    char *word;
    struct skill_type *st;
    struct ttype *tmp;

    max_skills = 0;
    skills_changed = FALSE;

    if ((fp = fopen(SKILLS_FILE, "r")) == NULL)
    {
	_perror(SKILLS_FILE);
	bugf("Can not open %s.", SKILLS_FILE);
	exit(0);
    }

    for (;;)
    {
	letter = fread_letter(fp);
	if (letter == '*')
	{
	    /* skip comments */
	    fread_to_eol(fp);
	    continue;
	}

	if (letter != '#')
	{
	    bugf("Load_skills: '#' not found.");
	    break;
	}

	word = fread_word(fp);
	if (!str_cmp(word, "SKILL"))
    	{
	    st = malloc(sizeof(struct skill_type));
	    
	    fread_skill(fp, st);
	    add_skill_tailq(st);
	    free(st);
	    max_skills++;
	    continue;
	}
	else if (!str_cmp(word, "End"))
	{
	    break;
	}
	else
	{
	    bugf("Load_skills: bad section name (%s).", word);
	    continue;
	}
    }

    fclose(fp);
    
    /* Fill skill_table */
    skill_table = malloc(sizeof(struct skill_type) * max_skills);
    i = 0;
    STAILQ_FOREACH(tmp, &s_tailq, entries)
	memcpy(&skill_table[i++], tmp->data, sizeof(struct skill_type));

    free_skill_tailq();
}

void fread_spaffs(FILE *fp, struct skill_type *st)
{
    char *word;
    bool fMatch;
    SPELLAFF *aff, *prev = NULL;
    
    for (aff = st->affect; aff; aff = aff->next)
	prev = aff;
    
    aff = new_spellaff();

    if (prev)
	prev->next = aff;

    aff->next	  = NULL;
    aff->where	  = 0;
    aff->apply	  = 0;
    aff->bit	  = 0;
    aff->mod	  = str_dup("");
    
    for (;;)
    {
	word = feof(fp) ? "[/Affect]" : fread_word(fp);
	fMatch = FALSE;

	switch (UPPER(word[0]))
	{
	case '*':
    	    fread_to_eol(fp);
	    fMatch = TRUE;
	    break;
	case '[':
	    if (str_cmp(word, "[/Affect]"))
	    {
		bugf("Found %s before [/Affect].", word);
	    }
	    else
		return;

	    break;

	case 'A':
	    if (!str_cmp(word, "Apply"))
	    {
		char *sapp = fread_string(fp);

		switch (aff->where)
		{
		case TO_AFFECTS:
		case TO_OBJECT:
		case TO_IMMUNE:
		case TO_RESIST:
		case TO_VULN:
		case TO_WEAPON:
		    aff->apply = flag_value(apply_flags, sapp);
		    if (aff->apply == NO_FLAG)
		    {
			bugf("Fread_spaffs: Unknown affect apply '%s'.",
			     sapp);
			aff->apply = 0;
		    }
		    break;
		    
		case TO_ROOM_AFF:
		    aff->apply = flag_value(room_apply_flags, sapp);
		    if (aff->apply == NO_FLAG)
		    {
			bugf("Fread_spaffs: Unknown room affect apply '%s'.",
			     sapp);
			aff->apply = 0;
		    }
		    break;
		}

		free_string(sapp);
	    }
	    break;

	case 'B':
	    if (!str_cmp(word, "Bit"))
	    {
		char *sbit = fread_string(fp);

		fMatch = TRUE;
		switch (aff->where)
		{
		case TO_AFFECTS:
		case TO_OBJECT:
		case TO_ROOM_AFF:
		    aff->bit = flag_value(affect_flags, sbit);
		    if (aff->bit == NO_FLAG)
		    {
			bugf("Fread_spaffs: Unknown affect bit '%s'.",
			     sbit);
			aff->bit = 0;
		    }
		    break;
		    
		case TO_IMMUNE:
		    aff->bit = flag_value(imm_flags, sbit);
		    if (aff->bit == NO_FLAG)
		    {
			bugf("Fread_spaffs: Unknown immun bit '%s'.",
			     sbit);
			aff->bit = 0;
		    }
		    break;
		    
		case TO_RESIST:
		    aff->bit = flag_value(res_flags, sbit);
		    if (aff->bit == NO_FLAG)
		    {
			bugf("Fread_spaffs: Unknown resist bit '%s'.",
			     sbit);
			aff->bit = 0;
		    }
		    break;
		    
		case TO_VULN:
		    aff->bit = flag_value(vuln_flags, sbit);
		    if (aff->bit == NO_FLAG)
		    {
			bugf("Fread_spaffs: Unknown vuln bit '%s'.",
			     sbit);
			aff->bit = 0;
		    }
		    break;
		    
		case TO_WEAPON:
		    aff->bit = flag_value(weapon_type2, sbit);
		    if (aff->bit == NO_FLAG)
		    {
			bugf("Fread_spaffs: Unknown weapon bit '%s'.",
			     sbit);
			aff->bit = 0;
		    }
		    break;
		}

		free_string(sbit);
	    }
	    break;

	case 'F':
	    FKEY("Flags", spaff_flags, aff->flags, "spell affect flags");
	    break;

	case 'M':
	    SKEY("Mod", aff->mod);
	    break;

	case 'W':
	    FKEY("Where", where_flags, aff->where, "spell affect location");
	    break;
	}

	if (!fMatch)
	    bugf("Fread_spaffs: no match: '%s'.", word);
    }
}

void fread_skill(FILE *fp, struct skill_type *st)
{
    char *word;
    bool fMatch;
    int i, depend = 0;

    st->name		 = str_dup("");
    st->rname		 = str_dup("");
    st->spell_fun	 = spell_null;
    st->target		 = TAR_IGNORE;
    st->minimum_position = POS_STANDING;
    st->pgsn		 = NULL;
    st->slot		 = 0;
    st->min_mana	 = 0;
    st->beats		 = 0;
    st->noun_damage	 = str_dup("");
    st->gender_damage	 = str_dup("");
    st->damage		 = str_dup("");
    st->dam_type	 = 0;
    st->msg_off		 = str_dup("");
    st->msg_obj		 = str_dup("");
    st->msg_room	 = str_dup("");
    st->aff_dur	 	 = str_dup("");
    st->flags		 = 0;
    st->saves_mod	 = 0;
    st->saves_act	 = 0;
    st->affect		 = NULL;

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

    for (;;)
    {
	word = feof(fp) ? "End" : fread_word(fp);
	fMatch = FALSE;

	switch (UPPER(word[0]))
	{
	case '*':
	    /* skip comments */
	    fMatch = TRUE;
	    fread_to_eol(fp);
	    break;

	case '[':
	    if (!str_cmp(word, "[Affect]"))
	    {
		fread_spaffs(fp, st);
		fMatch = TRUE;
	    }
	    break;

	case 'A':
	    SKEY("AffDur", st->aff_dur);
	    break;

	case 'B':
	    KEY("Beats", st->beats, fread_number(fp));
	    break;

	case 'D':
	    SKEY("Damage", st->damage);
	    SKEY("DamageMsg", st->noun_damage);
	    SKEY("DamageGender", st->gender_damage);
	    FKEY("DamType", dam_flags, st->dam_type, "damage type");

	    if (!str_cmp(word, "Depend"))
	    {
	      st->depends[depend++] = fread_string(fp);
	      fMatch = TRUE;
	    }

	    break;

	case 'E':
	    if (!str_cmp(word, "End"))
		return;
	    break;

	case 'F':
	    SKEY("FailCharCaster", st->char_fail[0]);
	    SKEY("FailCharVictim", st->char_fail[1]);
	    SKEY("FailCharRoom", st->char_fail[2]);
	    SKEY("FailObjCaster", st->obj_fail[0]);
	    SKEY("FailObjRoom", st->obj_fail[1]);
	    SKEY("FailRoomCaster", st->room_fail[0]);
	    SKEY("FailRoomRoom", st->room_fail[1]);
	    break;
	    
	case 'G':
	    if (!str_cmp(word, "GSN"))
	    {
		char *gname = fread_word(fp);

		st->pgsn = gsn_lookup(gname);
		fMatch = TRUE;
	    }
	    break;

	case 'M':
	    KEY("MinMana", st->min_mana, fread_number(fp));	
	    SKEY("MessageOff", st->msg_off);
	    SKEY("MessageObj", st->msg_obj);
	    SKEY("MessageRoom", st->msg_room);
	    FKEY("MinPos", position_flags, st->minimum_position, "position");
	    SKEY("MsgCharCaster", st->char_msg[0]);
	    SKEY("MsgCharVictim", st->char_msg[1]);
	    SKEY("MsgCharRoom", st->char_msg[2]);
	    SKEY("MsgObjCaster", st->obj_msg[0]);
	    SKEY("MsgObjRoom", st->obj_msg[1]);
	    SKEY("MsgRoomCaster", st->room_msg[0]);
	    SKEY("MsgRoomRoom", st->room_msg[1]);
	    break;

	case 'N':
	    SKEY("Name", st->name);
	    break;

	case 'R':
	    SKEY("RName", st->rname);
	    break;

	case 'S':
	    FKEY("SavesAct", saves_flags, st->saves_act, "saves action");
	    KEY("SavesMod", st->saves_mod, fread_number(fp));
	    KEY("Slot", st->slot, fread_number(fp));
	    FKEY("SpellFlags", spell_flags, st->flags, "spell flags");
	    
	    if (!str_cmp(word, "SpellFun"))
	    {
		char *sfun;

		sfun = fread_word(fp);
		st->spell_fun = spellfun_lookup(sfun);
		fMatch = TRUE;
	    }
	    break;

	case 'T':
	    FKEY("Target", target_flags, st->target, "target");
	    break;
	}

	if (!fMatch)
	    bugf("Fread_skill: no match: '%s'.", word);
    }
}

void load_races()
{
    FILE *fpList, *fp;
    char race[MAX_STRING_LENGTH];
    struct race_type *rt;
    struct ttype *tmp;
    int i;

    if ((fpList = fopen(RACES_LIST, "r")) == NULL)
    {
	_perror(RACES_LIST);
	bugf("Can not open %s.", RACES_LIST);
	exit(0);
    }

    for (;;)
    {
	char *file;
    
	file = fread_word(fpList);
	if (file[0] == '$')
	    break;

	strlcpy(race, RACES_DIR, MAX_STRING_LENGTH);
	strlcat(race, file, MAX_STRING_LENGTH);

	if ((fp = fopen(race, "r")) == NULL)
	{
	    _perror(race);
	    bugf("Can not open %s.", race);
	    exit(0);
	}

	rt = malloc(sizeof(struct race_type));
	fread_race(fp, rt);
	add_race_tailq(rt);
	max_races++;
	free(rt);
	fclose(fp);
    }
    fclose(fpList);
    
    /* Fill race */
    race_table = malloc(sizeof(struct race_type) * max_races);
    i = 0;
    STAILQ_FOREACH(tmp, &r_tailq, entries)
	memcpy(&race_table[i++], tmp->data, sizeof(struct race_type));
    free_race_tailq();
    pc_race_table = race_table;
}

void fread_race(FILE *fp, struct race_type *race)
{
    char *word;
    bool fMatch;
    int i;
   
    race->filename = str_dup("");
    race->name     = str_dup("");
    race->rname    = str_dup("");
    race->fname    = str_dup("");
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
    race->changed     = FALSE;
    
    race->class_mult  = alloc_perm(sizeof(int16_t) * max_classes);
    
    for (i = 0; i < max_classes; i++)
	race->class_mult[i] = 100;

    for (i = 0; i < 5; i++)
	race->skills[i] = str_dup("");

    for (i = 0; i < MAX_STATS; i++)
    {
	race->stats[i]     = 18;
	race->max_stats[i] = 18;
    }

    for (;;)
    {
	word = feof(fp) ? "End" : fread_word(fp);
	fMatch = FALSE;

	switch (UPPER(word[0]))
	{
	case '*':
	    /* skip comments */
	    fMatch = TRUE;
	    fread_to_eol(fp);
	    break;

	case 'A':
	    FKEY("Aff", affect_flags, race->aff, "affect");
	    FKEY("Act", act_flags, race->act, "act flags");
	    FKEY("Align", align_flags, race->valid_align, "align");
	    break;

	case 'C':
	    if (!str_cmp(word, "ClassMult"))
	    {
		char *sclass = fread_word(fp);
		int mult = fread_number(fp);
		int classid;

		fMatch = TRUE;
		if ((classid = class_lookup(sclass)) == -1)
		    bugf("Unknown class '%s'.", sclass);
		else
		    race->class_mult[classid] = mult;
	    }
	    break;

	case 'E':
	    if (!str_cmp(word, "End"))
		return;
	    break;

	case 'F':
	    SKEY("Filename", race->filename);
	    SKEY("FName", race->fname);
	    FKEY("Form", form_flags, race->form, "form");
	    break;

	case 'M':
	    KEY("Map", race->map_vnum, fread_number(fp));
	    SKEY("MName", race->rname);
	    if (!str_cmp(word, "MStats"))
	    {
		int i;

		fMatch = TRUE;
		for (i = 0; i < MAX_STATS; i++)
		    race->max_stats[i] = fread_number(fp);
	    }
	    break;

	case 'N':
	    SKEY("Name", race->name);
	    break;

	case 'O':
	    FKEY("Off", off_flags, race->off, "offense flags");
	    break;

	case 'P':
	    BKEY("PC", race->pc_race);
	    FKEY("Parts", part_flags, race->parts, "part");
	    KEY("Points", race->points, fread_number(fp));
	    break;

	case 'R':
	    KEY("Recall", race->recall, fread_number(fp));
	    if (!str_cmp(word, "Resist"))
	    {
		int res;
		char *sres = fread_word(fp);
		int percent = fread_number(fp);

		fMatch = TRUE;
		res = flag_value(dam_flags, sres);
		if (res == NO_FLAG)
		    bugf("Unknown resist type '%s'.", sres);
	       	else
		    race->resists[res] = percent;
	    }
	    break;

	case 'S':
	    if (!str_cmp(word, "Stats"))
	    {
		int i;

		fMatch = TRUE;
		for (i = 0; i < MAX_STATS; i++)
		    race->stats[i] = fread_number(fp);
	    }
	    if (!str_cmp(word, "Skills"))
	    {
		char *skill = fread_string(fp);
		char *line = skill;
		char arg[MAX_STRING_LENGTH];
		int i;

		fMatch = TRUE;
		for (i = 0; i < 5 && !IS_NULLSTR(line); i++)
		{
		    line = one_argument(line, arg);
		    free_string(race->skills[i]);
		    race->skills[i] = str_dup(arg);
		}

		free_string(skill);
	    }
	    if (!str_cmp(word, "Size"))
	    {
		char *ssize = fread_word(fp);
		int size;

		fMatch = TRUE;
		if ((size = flag_value(size_flags, ssize)) == NO_FLAG)
		    bugf("Unknown size '%s'.", ssize);
		else
		    race->size = size;
	    }
	    break;

	case 'W':
	    SKEY("WName", race->who_name);
	    break;
	}

	if (!fMatch)
	    bugf("Fread_race: no match: '%s'.", word);
    }
}

void load_classes()
{
    FILE *fpList, *fp;
    char classid[MAX_STRING_LENGTH];
    struct class_type *ct;
    int i;

    if ((fpList = fopen(CLASSES_LIST, "r")) == NULL)
    {
	_perror(CLASSES_LIST);
	bugf("Can not open %s.", CLASSES_LIST);
	exit(0);
    }

    max_classes = fread_number(fpList);
    class_table = calloc(sizeof(struct class_type), max_classes);

    for (i = 0; i < max_skills && !IS_NULLSTR(skill_table[i].name); i++)
    {
	skill_table[i].skill_level = calloc(sizeof(int16_t), max_classes);
	skill_table[i].rating      = calloc(sizeof(int16_t), max_classes);
	skill_table[i].quest       = calloc(sizeof(int16_t), max_classes);
    }

    for (i = 0; i < max_groups && !IS_NULLSTR(group_table[i].name); i++)
	group_table[i].rating = calloc(sizeof(int16_t), max_classes);

    for (i = 0; i < max_classes; i++)
    {
	strlcpy(classid, CLASSES_DIR, MAX_STRING_LENGTH);
	strlcat(classid, fread_word(fpList), MAX_STRING_LENGTH);

	if ((fp = fopen(classid, "r")) == NULL)
	{
	    _perror(classid);
	    bugf("Can not open %s.", classid);
	    exit(0);
	}

	ct = malloc(sizeof(struct class_type));
	fread_class(fp, ct, i);
	memcpy(&class_table[i], ct, sizeof(struct class_type));
	free(ct);
	fclose(fp);
    }
}

void fread_class(FILE *fp, struct class_type *classid, int cnum)
{
    char *word;
    bool fMatch;
    int i;

	classid->filename	  = str_dup("");
	classid->name           = str_dup("");
	classid->who_name       = str_dup("");
	classid->attr_prime     = 0;
	classid->attr_secondary = 0;
	classid->weapon         = 0;
	classid->skill_adept    = 75;
	classid->thac0_00       = 0;
	classid->thac0_32       = 0;
	classid->hp_min         = 0;
	classid->hp_max         = 0;
	classid->fMana          = 0;
	classid->valid_align    = 0;
	classid->base_group     = str_dup("");
	classid->default_group  = str_dup("");
	classid->changed	  = FALSE;

    for (i = 0; i < max_skills; i++)
    {
	skill_table[i].skill_level[cnum] = ANGEL;
	skill_table[i].rating[cnum]      = 0;
	skill_table[i].quest[cnum]	 = FALSE;
    }

    for (i = 0; i < max_groups; i++)
	group_table[i].rating[cnum] = -1;

    for (i = 0; i < MAX_PC_RACE; i++)
		classid->guild[i] = 0;

    for (;;)
    {
	word = feof(fp) ? "End" : fread_word(fp);
	fMatch = FALSE;

	switch(UPPER(word[0]))
	{
	case '*':
	    /* skip comments */
	    fMatch = TRUE;
	    fread_to_eol(fp);
	    break;

	case 'A':
	    FKEY("Align", align_flags, classid->valid_align, "align");
	    break;

	case 'B':
	    SKEY("BaseGroup", classid->base_group);
	    break;

	case 'D':
	    SKEY("DefaultGroup", classid->default_group);
	    break;

	case 'E':
	    if (!str_cmp(word, "End"))
		return;
	    break;

	case 'F':
	    SKEY("Filename", classid->filename);
	    if (!str_cmp(word, "FTitle"))
	    {
		int level = fread_number(fp);
		char *title = fread_string(fp);

		fMatch = TRUE;
		if (level < 0 || level > MAX_LEVEL)
		    bugf("FTitle level %d: must be between %d and %d.",
			 level, 0, MAX_LEVEL);
		else
		    title_table[cnum][level][1] = title;
	    }
	    break;

	case 'G':
	    if (!str_cmp(word, "Group"))
	    {
		char *group = fread_word(fp);
		int rating = fread_number(fp);
		int grnum;
		
		fMatch = TRUE;
		if ((grnum = group_lookup(group)) >= 0)
		    group_table[grnum].rating[cnum] = rating;
		else
		    bugf("Fread_class '%s': unknown group '%s'.",
				 classid->filename, group);
	    }
	    if (!str_cmp(word, "Guilds"))
	    {
		int i;

		fMatch = TRUE;
		for (i = 0; i < MAX_PC_RACE; i++)
			classid->guild[i] = fread_number(fp);
	    }
	    break;

	case 'M':
	    FKEY("MagicUser", magic_class_flags, classid->fMana, "attribute");
	    KEY("MaxHP", classid->hp_max, fread_number(fp));
	    KEY("MinHP", classid->hp_min, fread_number(fp));
	    if (!str_cmp(word, "MTitle"))
	    {
		int level = fread_number(fp);
		char *title = fread_string(fp);

		fMatch = TRUE;
		if (level < 0 || level > MAX_LEVEL)
		    bugf("MTitle level %d: must be between %d and %d.",
			 level, 0, MAX_LEVEL);
		else
		    title_table[cnum][level][0] = title;
	    }
	    break;

	case 'N':
	    SKEY("Name", classid->name);
	    break;

	case 'P':
	    FKEY("Primary", attr_flags, classid->attr_prime, "attribute");
	    break;

	case 'S':
	    FKEY("Secondary", attr_flags, classid->attr_secondary, "attribute");
	    KEY("SkillAdept", classid->skill_adept, fread_number(fp));
	    if (!str_cmp(word, "Skill"))
	    {
		int level = fread_number(fp);
		int diff = fread_number(fp);
		char *name = fread_word(fp);
		char *squest = fread_string_eol(fp);
		bool quest;
		int sn;
		
		fMatch = TRUE;
		
		quest = !str_cmp(squest, "true");
		free_string(squest);
		if ((sn = skill_lookup(name)) >= 0)
		{
		    skill_table[sn].skill_level[cnum] = level;
		    skill_table[sn].rating[cnum] = diff;
		    skill_table[sn].quest[cnum] = quest;
		}
		else
		    bugf("Unknown skill '%s'.", name);
	    }
	    break;
	case 'T':
	    if (!str_cmp(word, "THAC0"))
	    {
		fMatch = TRUE;
			classid->thac0_00 = fread_number(fp);
			classid->thac0_32 = fread_number(fp);
	    }
	    break;

	case 'W':
	    SKEY("WName", classid->who_name);
	    FKEY("Weapon", weapon_class, classid->weapon, "weapon");
	    break;
	}

	if (!fMatch)
	    bugf("Unknown keyword '%s'.", word);
    }
}

static int save_races(CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];
    int i, j;
    FILE *fp, *fpList;
    bool saved = FALSE;

    if (races_changed == TRUE)
    {
	if ((fpList = file_open(RACES_LIST, "w")) == NULL)
	{
	    _perror(RACES_LIST);
	    bugf("Can not open %s for write.", RACES_LIST);
	    return -1;
	}

	for (i = 0; i < max_races; i++)
	    fprintf(fpList, "%s\n", race_table[i].filename);
	fprintf(fpList, "$\n");
	file_close(fpList);

	send_to_char("Races list saved.\n\r", ch);
	races_changed = FALSE;
    }

    send_to_char("Saved races:\n\r", ch);
    
    for (i = 0; i < max_races; i++)
    {
	if (race_table[i].changed == FALSE)
	    continue;

	sprintf(buf, "%s%s", RACES_DIR, race_table[i].filename);

	if ((fp = file_open(buf, "w")) == NULL)
	{
	    _perror(buf);
	    bugf("Can not open %s for write.", buf);
	    return -1;
	}

	fprintf(fp, "Filename  %s~\n", race_table[i].filename);
	fprintf(fp, "Name      %s~\n", race_table[i].name);
	fprintf(fp, "MName     %s~\n", race_table[i].rname);
	fprintf(fp, "FName     %s~\n", race_table[i].fname);
	fprintf(fp, "PC        %s\n", race_table[i].pc_race ? "true" : "false");
	fprintf(fp, "Act       %s~\n",
		fflag_string(act_flags, race_table[i].act, FALSE, FALSE));
	fprintf(fp, "Aff       %s~\n",
		fflag_string(affect_flags, race_table[i].aff, FALSE, FALSE));
	fprintf(fp, "Off       %s~\n",
		fflag_string(off_flags, race_table[i].off, FALSE, FALSE));
	for (j = 0; j < DAM_MAX; j++)
	    if (race_table[i].resists[j] != 0)
	    {
		fprintf(fp, "Resist    %s %d\n",
			fflag_string(dam_flags, j, FALSE, FALSE),
			race_table[i].resists[j]);
	    }
	fprintf(fp, "Form      %s~\n",
		fflag_string(form_flags, race_table[i].form, FALSE, FALSE));
	fprintf(fp, "Parts     %s~\n",
		fflag_string(part_flags, race_table[i].parts, FALSE, FALSE));

	if (race_table[i].pc_race)
	{
	    fprintf(fp, "WName     %s~\n", race_table[i].who_name);
	    fprintf(fp, "Points    %d\n", race_table[i].points);
	    fprintf(fp, "Skills    '%s' '%s' '%s' '%s' '%s'~\n\n",
		    race_table[i].skills[0],
		    race_table[i].skills[1],
		    race_table[i].skills[2],
		    race_table[i].skills[3],
		    race_table[i].skills[4]);
	    fprintf(fp, "*         Str Int Wis Dex Con\n");
	    fprintf(fp, "Stats     %-3d %-3d %-3d %-3d %-3d\n",
		    race_table[i].stats[0],
		    race_table[i].stats[1],
		    race_table[i].stats[2],
		    race_table[i].stats[3],
		    race_table[i].stats[4]);
	    fprintf(fp, "MStats    %-3d %-3d %-3d %-3d %-3d\n",
		    race_table[i].max_stats[0],
		    race_table[i].max_stats[1],
		    race_table[i].max_stats[2],
		    race_table[i].max_stats[3],
		    race_table[i].max_stats[4]);
		    
	    for (j = 0; j < MAX_CLASS; j++)
	    {
		fprintf(fp, "ClassMult %-15.15s %d\n",
			class_table[j].name, race_table[i].class_mult[j]);
	    }
	    
	    fprintf(fp, "Size      %s\n",
		    fflag_string(size_flags, race_table[i].size, FALSE, FALSE));
	    fprintf(fp, "Recall    %d\n", race_table[i].recall);
	    fprintf(fp, "Map       %d\n", race_table[i].map_vnum);
	    fprintf(fp, "Align     %s~\n",
		    fflag_string(align_flags, race_table[i].valid_align,
				 FALSE, FALSE));
	}
	fprintf(fp, "End\n\r* charset=cp1251");
	race_table[i].changed = FALSE;

	file_close(fp);

	printf_to_char("\t%s\n\r", ch, race_table[i].filename);
	saved = TRUE;
    }

    if (!saved)
	send_to_char("\tNone.\n\r", ch);
    
    return 0;
}

void save_classes(CHAR_DATA *ch)
{
    FILE *fpList = NULL, *fp;
    int i, j;
    char buf[MAX_STRING_LENGTH];

    if (classes_changed == TRUE)
    {
	if ((fpList = file_open(CLASSES_LIST, "w")) == NULL)
	{
	    _perror(CLASSES_LIST);
	    bugf("Can not open %s for write.", CLASSES_LIST);
	    return;
	}

	fprintf(fpList, "%d\n",max_classes);

	for (i = 0; i < max_classes; i++)
	   fprintf(fpList, "%s\n", class_table[i].filename);
	fprintf(fpList, "$\n");
	file_close(fpList);

	send_to_char("Class list saved.\n\r", ch);
	classes_changed = FALSE;
    }

    send_to_char("Saved classes:\n\r", ch);

    for (i = 0; i < max_classes; i++)
    {
	if (class_table[i].changed == FALSE)
	    continue;
	sprintf(buf, "%s%s", CLASSES_DIR, class_table[i].filename);

	if ((fp = fopen(buf, "w")) == NULL)
	{
	    _perror(buf);
	    bugf("Can not open %s for write.", buf);
	    exit(0);
	}

	fprintf(fp, "Filename     %s~\n", class_table[i].filename);
	fprintf(fp, "Name         %s~\n", class_table[i].name);
	fprintf(fp, "WName        %s~\n", class_table[i].who_name);
	fprintf(fp, "Primary      %s~\n",
		fflag_string(attr_flags, class_table[i].attr_prime,
			     FALSE, FALSE));
	fprintf(fp, "Secondary    %s~\n",
		fflag_string(attr_flags, class_table[i].attr_secondary,
			     FALSE, FALSE));
	fprintf(fp, "Weapon       %s~\n",
		fflag_string(weapon_class, class_table[i].weapon,
			     FALSE, FALSE));
	fprintf(fp, "SkillAdept   %d\n", class_table[i].skill_adept);
	fprintf(fp, "THAC0        %-2d %-2d\n",
		class_table[i].thac0_00, class_table[i].thac0_32);
	fprintf(fp, "MinHP        %d\n", class_table[i].hp_min);
	fprintf(fp, "MaxHP        %d\n", class_table[i].hp_max);

	fprintf(fp, "MagicUser    %s~\n", magic_class_flags[class_table[i].fMana].name);
	fprintf(fp, "BaseGroup    %s~\n", class_table[i].base_group);
	fprintf(fp, "DefaultGroup %s~\n", class_table[i].default_group);
	fprintf(fp, "Align        %s~\n",
		fflag_string(align_flags, class_table[i].valid_align,
			     FALSE, FALSE));

	fprintf(fp, "Guilds      ");
	for (j = 0; j < MAX_PC_RACE; j++)
	    fprintf(fp, " %d", class_table[i].guild[j]);
	fprintf(fp, "\n\n");

	fprintf(fp, "* Groups section\n"
		    "* Not shown groups are not available to this class\n"
		    "* Format: Group '<group name>' <difficulty rating>\n");

	for (j = 0; j < max_groups && !IS_NULLSTR(group_table[j].name); j++)
	    if (group_table[j].rating[i] > -1)
	    {
		sprintf(buf, "'%s'", group_table[j].name);
		fprintf(fp, "Group        %-22.22s %d\n",
			buf, group_table[j].rating[i]);
	    }

	fprintf(fp, "\n* Skills/spells section\n"
		    "* Not shown skills are not available to this class\n"
		    "* Format: Skill <level> <difficulty> '<name>' <quest>\n");

	for (j = 0; j <= max_skills && !IS_NULLSTR(skill_table[j].name); j++)
	    if (skill_table[j].skill_level[i] <= LEVEL_HERO)
	    {
		sprintf(buf, "'%s'", skill_table[j].name);
		fprintf(fp, "Skill        %-2d %d %-30.30s %s\n",
			skill_table[j].skill_level[i],
			skill_table[j].rating[i],
			buf,
			skill_table[j].quest[i] ? "true" : "false");
	    }

	fprintf(fp, "\n* Titles section.\n");

	for (j = 0; j <= MAX_LEVEL; j++)
	{
	    fprintf(fp, "MTitle       %-2d %s~\n", j, title_table[i][j][0]);
	    fprintf(fp, "FTitle       %-2d %s~\n", j, title_table[i][j][1]);
	}

	fprintf(fp, "End\n\n* charset=cp1251\n");

	printf_to_char("\t%s\n\r", ch, class_table[i].filename);	
	class_table[i].changed = FALSE;

	fclose(fp);
    }
}

void save_spaffs(FILE *fp, const struct skill_type *st)
{
    SPELLAFF *aff;

    for (aff = st->affect; aff; aff = aff->next)
    {
	fprintf(fp, "[Affect]");
	fprintf(fp, "Where        %s~\n",
		fflag_string(where_flags, aff->where, FALSE, FALSE));

	switch (aff->where)
	{
	case TO_AFFECTS:
	case TO_OBJECT:
	    fprintf(fp, "Apply        %s~\n",
	    	    fflag_string(apply_flags, aff->apply, FALSE, FALSE));
	    fprintf(fp, "Bit          %s~\n",
		    fflag_string(affect_flags, aff->bit, FALSE, FALSE));
	    break;
	case TO_IMMUNE:
	    fprintf(fp, "Apply        %s~\n",
	    	    fflag_string(apply_flags, aff->apply, FALSE, FALSE));
	    fprintf(fp, "Bit          %s~\n",
		    fflag_string(imm_flags, aff->bit, FALSE, FALSE));
	    break;
	case TO_RESIST:
	    fprintf(fp, "Apply        %s~\n",
	    	    fflag_string(apply_flags, aff->apply, FALSE, FALSE));
	    fprintf(fp, "Bit          %s~\n",
		    fflag_string(res_flags, aff->bit, FALSE, FALSE));
	    break;
	case TO_VULN:
	    fprintf(fp, "Apply        %s~\n",
	    	    fflag_string(apply_flags, aff->apply, FALSE, FALSE));
	    fprintf(fp, "Bit          %s~\n",
		    fflag_string(vuln_flags, aff->bit, FALSE, FALSE));
	    break;
	case TO_WEAPON:
	    fprintf(fp, "Apply        %s~\n",
	    	    fflag_string(apply_flags, aff->apply, FALSE, FALSE));
	    fprintf(fp, "Bit          %s~\n",
		    fflag_string(weapon_type2, aff->bit, FALSE, FALSE));
	    break;
	case TO_ROOM_AFF:
	    fprintf(fp, "Apply        %s~\n",
	    	    fflag_string(room_apply_flags, aff->apply, FALSE, FALSE));
	    fprintf(fp, "Bit          %s~\n",
		    fflag_string(affect_flags, aff->bit, FALSE, FALSE));
	    break;
	default:
	    bugf("Save_spaffs: Unknown 'where' field: %d.", aff->where);
	    break;
	}
	if (aff->bit != 0)
	    fprintf(fp, "Flags        %s~\n",
		    fflag_string(spaff_flags, aff->bit, FALSE, FALSE));

	if (!IS_NULLSTR(aff->mod))
	    fprintf(fp, "Mod          %s~\n", aff->mod);
	    
	fprintf(fp, "[/Affect]\n");
    }
}

void save_skills()
{
    FILE *fp;
    int i;

    if ((fp = fopen(SKILLS_FILE, "w")) == NULL)
    {
	_perror(SKILLS_FILE);
	bugf("Can not open %s for write.", SKILLS_FILE);
	return;
    }

    for (i = 0; i < max_skills && !IS_NULLSTR(skill_table[i].name); i++)
    {
	fprintf(fp, "#SKILL\n");
	fprintf(fp, "Name           %s~\n", skill_table[i].name);
	fprintf(fp, "RName          %s~\n", skill_table[i].rname);
	fprintf(fp, "SpellFun       %s\n",
		spellname_lookup(skill_table[i].spell_fun));
	fprintf(fp, "Target         %s~\n",
		fflag_string(target_flags, skill_table[i].target,
			     FALSE, FALSE));
	fprintf(fp, "MinPos         %s~\n",
		fflag_string(position_flags,
			    skill_table[i].minimum_position,
			    FALSE, FALSE));
	if (skill_table[i].slot != 0)
	    fprintf(fp, "Slot           %d\n", skill_table[i].slot);

	if (skill_table[i].min_mana != 0)
	    fprintf(fp, "MinMana        %d\n", skill_table[i].min_mana);
	    
	fprintf(fp, "Beats          %d\n", skill_table[i].beats);

	if (skill_table[i].pgsn)
	    fprintf(fp, "GSN            %s\n", gsn_name_lookup(skill_table[i].pgsn));

	if (!IS_NULLSTR(skill_table[i].noun_damage))
	    fprintf(fp, "DamageMsg      %s~\n", skill_table[i].noun_damage);

	if (!IS_NULLSTR(skill_table[i].gender_damage))
	    fprintf(fp, "DamageGender   %s~\n", skill_table[i].gender_damage);

	if (!IS_NULLSTR(skill_table[i].msg_off))
	    fprintf(fp, "MessageOff     %s~\n", skill_table[i].msg_off);

	if (!IS_NULLSTR(skill_table[i].msg_obj))
	    fprintf(fp, "MessageObj     %s~\n", skill_table[i].msg_obj);

	if (!IS_NULLSTR(skill_table[i].msg_room))
	    fprintf(fp, "MessageRoom    %s~\n", skill_table[i].msg_room);

	if (!IS_NULLSTR(skill_table[i].damage))
	    fprintf(fp, "Damage         %s~\n", skill_table[i].damage);

	if (skill_table[i].dam_type != 0)
	    fprintf(fp, "DamType        %s~\n",
		    fflag_string(dam_flags, skill_table[i].dam_type,
				 FALSE, FALSE));

	if (skill_table[i].flags != 0)
	{
	    fprintf(fp, "SpellFlags     %s~\n",
		    fflag_string(spell_flags, skill_table[i].flags,
				 FALSE, FALSE));
	}

	if (skill_table[i].saves_mod != 0)
	    fprintf(fp, "SavesMod       %d\n", skill_table[i].saves_mod);

	if (skill_table[i].saves_act != 0)
	{
	    fprintf(fp, "SavesAct       %s~\n",
		    fflag_string(saves_flags, skill_table[i].saves_act,
				 FALSE, FALSE));
	}

	if (!IS_NULLSTR(skill_table[i].char_msg[0]))
	    fprintf(fp, "MsgCharCaster  %s~\n", skill_table[i].char_msg[0]);

	if (!IS_NULLSTR(skill_table[i].char_msg[1]))
	    fprintf(fp, "MsgCharVictim  %s~\n", skill_table[i].char_msg[1]);

	if (!IS_NULLSTR(skill_table[i].char_msg[2]))
	    fprintf(fp, "MsgCharRoom    %s~\n", skill_table[i].char_msg[2]);

	if (!IS_NULLSTR(skill_table[i].obj_msg[0]))
	    fprintf(fp, "MsgObjCaster   %s~\n", skill_table[i].obj_msg[0]);

	if (!IS_NULLSTR(skill_table[i].obj_msg[1]))
	    fprintf(fp, "MsgObjRoom     %s~\n", skill_table[i].obj_msg[1]);

	if (!IS_NULLSTR(skill_table[i].room_msg[0]))
	    fprintf(fp, "MsgRoomCaster  %s~\n", skill_table[i].room_msg[0]);

	if (!IS_NULLSTR(skill_table[i].room_msg[1]))
	    fprintf(fp, "MsgRoomRoom    %s~\n", skill_table[i].room_msg[1]);

	if (!IS_NULLSTR(skill_table[i].char_msg[0]))
	    fprintf(fp, "FailCharCaster %s~\n", skill_table[i].char_fail[0]);

	if (!IS_NULLSTR(skill_table[i].char_msg[1]))
	    fprintf(fp, "FailCharVictim %s~\n", skill_table[i].char_fail[1]);

	if (!IS_NULLSTR(skill_table[i].char_msg[2]))
	    fprintf(fp, "FailCharRoom   %s~\n", skill_table[i].char_fail[2]);

	if (!IS_NULLSTR(skill_table[i].obj_msg[0]))
	    fprintf(fp, "FailObjCaster  %s~\n", skill_table[i].obj_fail[0]);

	if (!IS_NULLSTR(skill_table[i].obj_msg[1]))
	    fprintf(fp, "FailObjRoom    %s~\n", skill_table[i].obj_fail[1]);

	if (!IS_NULLSTR(skill_table[i].room_msg[0]))
	    fprintf(fp, "FailRoomCaster %s~\n", skill_table[i].room_fail[0]);

	if (!IS_NULLSTR(skill_table[i].room_msg[1]))
	    fprintf(fp, "FailRoomRoom   %s~\n", skill_table[i].room_fail[1]);

/*	if (!IS_NULLSTR(skill_table[i].aff_dur))
	    fprintf(fp, "AffDur         %s~\n", skill_table[i].aff_dur); */

	if (skill_table[i].affect != NULL)
	    save_spaffs(fp, &skill_table[i]);

	fprintf(fp, "End\n\n");
    }

    fprintf(fp, "#END\n");
    fclose(fp);
    skills_changed = FALSE;
}

const char *spellname_lookup(SPELL_FUN *fun)
{

    Dl_info info;

    if (dladdr(fun, &info) == 0)
    {
	bugf("spellname_lookup(): unknown fun.");
	return NULL;
    }

    if (info.dli_saddr != fun)
    {
	/* WTF? */
	bugf("spellname_lookup(): strange thing happens.");
	return NULL;
    }

    return info.dli_sname;

}

SPELL_FUN *spellfun_lookup(char *name)
{

    SPELL_FUN *fun;

    fun = (SPELL_FUN *)dlsym(NULL, name);
    if (fun == NULL)
	bugf("spellfun_lookup(): spellfun '%s' unknown.", name);

    return fun;

}

int16_t *gsn_lookup(char *name)
{

    int16_t *gsn;

    gsn = (int16_t *)dlsym(NULL, name);
    if (gsn == NULL)
	bugf("gsn_lookup(): gsn '%s' unknown.", name);

    return gsn;

}

char *gsn_name_lookup(int16_t *gsn)
{

    Dl_info info;

    if (dladdr(gsn, &info) == 0)
    {
	bugf("gsn_name_lookup(): unknown gsn.");
	return NULL;
    }

    if (info.dli_saddr != gsn)
    {
	/* WTF? */
	bugf("gsn_name_lookup(): strange thing happens.");
	return NULL;
    }

    return (char *)info.dli_sname;

}

void do_rasave(CHAR_DATA *ch, char *argument)
{
    save_races(ch);
}

/* charset=cp1251 */
