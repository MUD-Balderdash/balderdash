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
#include <sys/time.h>

#include <sys/resource.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#define	IN_DB_C
#include "merc.h"
#include "db.h"
#include "recycle.h"
#include "music.h"
#undef IN_DB_C

#include "config.h"




extern	int	_filbuf		args((FILE *));

#if !defined(OLD_RAND)

#if !defined(linux)

long random();

#endif
#endif



/* Lua */
//void load_lua(void);

void dbdump(AREA_DATA *area);
void db_tables();

long get_obj_id();

void init_grants();

/* externals for counting purposes */
extern SLIST_HEAD(, descriptor_data) descriptor_free;
extern	OBJ_DATA	*obj_free;
extern	PC_DATA		*pcdata_free;
extern  AFFECT_DATA	*affect_free;

/*
 * Globals.
 */
STAILQ_HEAD(, help_data) help_first = STAILQ_HEAD_INITIALIZER(help_first);
LIST_HEAD(, char_data) char_list = LIST_HEAD_INITIALIZER(char_list);

HELP_AREA *		had_list;

SHOP_DATA *		shop_first;
SHOP_DATA *		shop_last;

PROG_CODE *		mprog_list;
PROG_CODE *             oprog_list;
PROG_CODE *             rprog_list;



char			bug_buf		[2*MAX_INPUT_LENGTH];
ROOM_INDEX_DATA *	room_list;
char *			help_greeting = NULL;

KILL_DATA		kill_table	[MAX_LEVEL];
OBJ_DATA *		object_list;
TIME_INFO_DATA		time_info;
WEATHER_DATA		weather_info;
EXCEPT_DATA		*exceptions;
int			limits[MAX_LIMITS];

char *dream_table[MAX_DREAMS];
char *quote_table[MAX_QUOTES];
char *stoplist[MAX_STOP_WORDS];
char *badnames[MAX_BAD_NAMES];
char *whiteip[MAX_WHOIS_ENTRIES];
char *whitedomains[MAX_WHOIS_ENTRIES];


/*
 * Locals.
 */
MOB_INDEX_DATA *	mob_index_hash		[MAX_KEY_HASH];
OBJ_INDEX_DATA *	obj_index_hash		[MAX_KEY_HASH];
ROOM_INDEX_DATA *	room_index_hash		[MAX_KEY_HASH];
char *			string_hash		[MAX_KEY_HASH];

AREA_DATA *		area_first;
AREA_DATA *		area_last;
AREA_DATA *		current_area;

char *			string_space;
char *			top_string;
char			str_empty	[1];

int			top_affect;
int			top_area;
int			top_ed;
int			top_exit;
int			top_help;
int			top_mob_index;
int                     top_oprog_index;
int                     top_rprog_index;
int			top_obj_index;
int			top_reset;
int			top_room;
int			top_shop;
int             	top_vnum_room;  /* OLC */
int     	        top_vnum_mob;   /* OLC */
int	                top_vnum_obj;   /* OLC */
int			top_vnum;
int			top_mprog_index;	/* OLC */
int 			mobile_count = 0;
int 			obj_count    = 0;
int			newmobs = 0;
int			newobjs = 0;

int top_map_visit = 0;
int top_map_qd = 0;


/*
 * Memory management.
 * Increase MAX_STRING if you have too.
 * Tune the others only if you understand what you're doing.
 */
#define			MAX_STRING	9999999
#define			MAX_PERM_BLOCK	524000
#define			MAX_MEM_LIST	15

void *			rgFreeList	[MAX_MEM_LIST];
const unsigned int	rgSizeList	[MAX_MEM_LIST]	=
{
    16, 32, 64, 128, 256, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288-64
};

int			nAllocString;
int			sAllocString;
int			nAllocPerm;
int			sAllocPerm;

int area_version;

/*
 * Semi-locals.
 */
bool			fBootDb;
FILE *			fpArea;
char			strArea[MAX_INPUT_LENGTH];



/*
 * Local booting procedures.
 */
void    init_mm         args((void));
void	load_area	args((FILE *fp));
void    new_load_area   args((FILE *fp));   /* OLC */
void	load_helps	args((FILE *fp, char *fname));
void	load_old_mob	args((FILE *fp));
void 	load_mobiles	args((FILE *fp));
void	load_old_obj	args((FILE *fp));
void 	load_objects	args((FILE *fp));
void	load_resets	args((FILE *fp));
void	load_rooms	args((FILE *fp));
void	load_shops	args((FILE *fp));
void 	load_socials	args((FILE *fp));
void	load_specials	args((FILE *fp));
void	load_notes	args((void));
void	load_bans	args((void));
void    load_mobprogs   args((FILE *fp));
void 	load_objprogs	args((FILE *fp));
void 	load_roomprogs	args((FILE *fp));

void 	fix_objprogs 	args((void));
void 	fix_roomprogs  	args((void));

void	load_exceptions	args((void));
void	load_limits	args((void));
void	load_clans	args((void));
void	load_dreams	args((void));
void	load_quotes	args((void));
void	load_stoplist	args((void));
void 	load_badnames();
void 	load_corpses();
void 	load_whois();
void 	load_popularity();

void 	load_chat	args(( FILE *fp ));

void	fix_exits	args((void));
void    fix_mobprogs	args((void));

void	reset_area	args((AREA_DATA * pArea));
int64_t get_prog_when(FILE *fp);


void skip_section(FILE *fp, char *section)
{
    int number;
    char *word;
    char letter;

    if (!str_cmp(section, "AREADATA"))
    {
	for (; ;)
	{
	    word = feof(fp) ? "End" : fread_word(fp);
	    if (!str_cmp(word, "End"))
		return;
	    fread_to_eol(fp);
	}
    }

    if (!str_cmp(section, "HELPS"))
    {
	for (;;)
	{
	    number = fread_number(fp);
	    word = fread_word(fp);
	    if (!str_cmp(word, "$~"))
		return;

	    if (word[strlen(word) - 1] != '~')
		while(fread_letter(fp) != '~')
		    ;

	    while(fread_letter(fp) != '~')
		;
	}
    }

    if (!str_cmp(section, "MOBILES")
	|| !str_cmp(section, "OBJECTS")
	|| !str_cmp(section, "ROOMS")
	|| !str_cmp(section, "MOBPROGS")
	|| !str_cmp(section, "OBJPROGS")
	|| !str_cmp(section, "ROOMPROGS")
	|| !str_cmp(section, "CHAT"))
    {
	for (;;)
	{
	    word = feof(fp) ? "#0" : fread_word(fp);

	    if (word[0] == '#' && word[1] == '0')
		return;
	    fread_to_eol(fp);
	}
    }

    if (!str_cmp(section, "RESETS") || !str_cmp(section, "SPECIALS"))
    {
	for (;;)
	{
	    switch(letter = fread_letter(fp))
	    {
	    case 'S':
		return;
	    case 's':
		fread_to_eol(fp);
		return;
	    }
	    fread_to_eol(fp);
	}
    }

    if (!str_cmp(section, "SHOPS"))
    {
	for (;;)
	{
	    number = fread_number(fp);
	    if (number == 0)
		return;
	    fread_to_eol(fp);
	}
    }

    if (!str_cmp(section, "SOCIALS"))
    {
	for (;;)
	{
	    word = fread_word(fp);
	    if (!strcmp(word, "#0"))
		return;
	    fread_to_eol(fp);
	}
    }
    bugf("skip_section: Invalid section name (%s).", section);
    exit(1);
}

void init_str()
{
    /*
     * Init some data space stuff.
     */
    {
	if ((string_space = calloc(1, MAX_STRING)) == NULL)
	{
	    bugf("Boot_db: can't alloc %d string space.", MAX_STRING);
	    exit(1);
	}
	top_string	= string_space;
	fBootDb		= TRUE;
    }
}

/*
 * Big mama top level function.
 */
void boot_db(void)
{
    int sn;

    /* Set current area version */
    area_version = 4;

    /*
     * Init random number generator.
     */
    {
	init_mm();
    }
    /*
     * Set time and weather.
     */
    {
	long lhour, lday, lmonth;
	int i;

	load_moons();

	log_string("Ok.");

	lhour		= (current_time - 979624455)
	    / (PULSE_TICK / PULSE_PER_SECOND);
	time_info.hour	= lhour  % 24;
	lday		= lhour  / 24;
	time_info.day	= lday   % 31;
	lmonth		= lday   / 31;
	time_info.month	= lmonth % 12;
	time_info.year	= lmonth / 12 + 300;

	/* Init moons */
	for (i = 0; i < max_moons; i++)
	{
	    moons_data[i].moonlight = lhour % moons_data[i].period;
	    moons_data[i].state = lday % moons_data[i].state_period;
	}

	if (time_info.hour <  5)
	    weather_info.sunlight = SUN_DARK;
	else if (time_info.hour < 6)
	    weather_info.sunlight = SUN_RISE;
	else if (time_info.hour < 19)
	    weather_info.sunlight = SUN_LIGHT;
	else if (time_info.hour < 20)
	    weather_info.sunlight = SUN_SET;
	else
	    weather_info.sunlight = SUN_DARK;

	weather_info.change	= 0;
	weather_info.mmhg	= 960;
	if (time_info.month >= 7 && time_info.month <=12)
	    weather_info.mmhg += number_range(1, 50);
	else
	    weather_info.mmhg += number_range(1, 80);

	if (weather_info.mmhg <=  980) weather_info.sky = SKY_LIGHTNING;
	else if (weather_info.mmhg <= 1000) weather_info.sky = SKY_RAINING;
	else if (weather_info.mmhg <= 1020) weather_info.sky = SKY_CLOUDY;
	else                                weather_info.sky = SKY_CLOUDLESS;

    }
    log_string("Init special grants.");
    init_grants();
    log_string("Loading skills.");
    load_skills();
    log_string("Ok.");
    log_string("Loading groups.");
    load_groups();
    log_string("Ok.");
    log_string("Loading classes.");
    load_classes();
    log_string("Ok.");
    log_string("Loading races.");
    load_races();
    log_string("Ok.");

    /*
     * Assign gsn's for skills which have them.
     */

    for (sn = 0; sn < max_skills; sn++)
	if (skill_table[sn].pgsn != NULL)
	    *skill_table[sn].pgsn = sn;

    /*
     * Copy tables
     */

    for (sn = 0;
	 sn < MAX_WEAPON_TYPE && !IS_NULLSTR(weapon_table[sn].name);
	 sn++)
    {
	weapon_class[sn].name  = weapon_table[sn].name;
	weapon_class[sn].rname = weapon_table[sn].rname;
	weapon_class[sn].bit   = weapon_table[sn].type;
	weapon_class[sn].settable = TRUE;
    }

    weapon_class[sn].name  = NULL;
    weapon_class[sn].rname = NULL;
    weapon_class[sn].bit   = 0;
    weapon_class[sn].settable = FALSE;

    for (sn = 0; sn < MAX_ITEM_TYPE && !IS_NULLSTR(item_table[sn].name); sn++)
    {
	type_flags[sn].name  = item_table[sn].name;
	type_flags[sn].rname = item_table[sn].rname;
	type_flags[sn].bit   = item_table[sn].type;

	if (type_flags[sn].bit == ITEM_CORPSE_PC)
	    type_flags[sn].settable = FALSE;
	else
	    type_flags[sn].settable = TRUE;
    }

    type_flags[sn].name  = NULL;
    type_flags[sn].rname = NULL;
    type_flags[sn].bit   = 0;
    type_flags[sn].settable = FALSE;

    /*
     * Read in all the area files.
     */


    load_limits();
    load_clans();

    {
	FILE *fpList;

	if ((fpList = fopen(AREA_LIST, "r")) == NULL)
	{
	    _perror(AREA_LIST);
	    exit(1);
	}
	for (; ;)
	{
	    strcpy(strArea, fread_word(fpList));
	    if (strArea[0] == '$')
		break;

	    if (strArea[0] == '-')
	    {
		fpArea = stdin;
	    }
	    else
	    {
		if ((fpArea = fopen(strArea, "r")) == NULL)
		{
		    _perror(strArea);
		    exit(1);
		}
	    }

	    current_area = NULL;
	    
	    log_string(strArea);
	    
	    for (; ;)
	    {
		char *word;

		if (fread_letter(fpArea) != '#')
		{
		    bugf("Boot_db: # not found.", 0);
		    exit(1);
		}

		word = fread_word(fpArea);
		if (word[0] == '$')
		    break;
		else if (!str_cmp(word, "AREADATA"))
		    new_load_area(fpArea);
		else if (!str_cmp(word, "HELPS"))
		    load_helps(fpArea, strArea);
		else if (!str_cmp(word, "MOBILES"))
		    load_mobiles(fpArea);
		else if (!str_cmp(word, "MOBPROGS"))
		    load_mobprogs(fpArea);
		else if (!str_cmp(word, "OBJPROGS"))
		    load_objprogs(fpArea);
		else if (!str_cmp(word, "ROOMPROGS"))
		    load_roomprogs(fpArea);
		else if (!str_cmp(word, "OBJECTS"))
		    load_objects(fpArea);
		else if (!str_cmp(word, "RESETS"))
		    skip_section(fpArea, word);
		else if (!str_cmp(word, "ROOMS"))
		    load_rooms(fpArea);
		else if (!str_cmp(word, "SHOPS"))
		    skip_section(fpArea, word);
		else if (!str_cmp(word, "SOCIALS"))
		    load_socials(fpArea);
		else if (!str_cmp(word, "SPECIALS"))
		    load_specials(fpArea);
		else if (!str_cmp(word, "CHAT"))
		    load_chat(fpArea);
		else
		{
		    bugf("Boot_db: bad section name.");
		    exit(1);
		}
	    }
	    if (fpArea != stdin)
		fclose(fpArea);
	    fpArea = NULL;
	}
	fclose(fpList);
	/* Load resets and shops only after everything else is loaded in */

	if ((fpList = fopen(AREA_LIST, "r")) == NULL)
	{
	    perror(AREA_LIST);
	    exit(1);
	}

	for (;;)
	{
	    strcpy(strArea, fread_word(fpList));
	    if (strArea[0] == '$')
		break;

	    if (strArea[0] == '-')
	    {
		fpArea = stdin;
	    }
	    else
	    {
		if ((fpArea = fopen(strArea, "r")) == NULL)
		{
		    perror(strArea);
		    exit(1);
		}
	    }

	    for (; ;)
	    {
		char *word;

		if (fread_letter(fpArea) != '#')
		{
		    bugf("Boot_db: # not found.");
		    exit(1);
		}

		word = fread_word(fpArea);

		if (word[0] == '$')
		    break;
		else if (!str_cmp(word, "AREADATA"))
		    skip_section(fpArea, word); /*load_area    (fpArea)*/
		else if (!str_cmp(word, "HELPS"))
		    skip_section(fpArea, word); /*load_helps   (fpArea)*/
		else if (!str_cmp(word, "MOBILES"))
		    skip_section(fpArea, word); /*load_mobiles (fpArea)*/
		else if (!str_cmp(word, "MOBPROGS"))
		    skip_section(fpArea, word); /*load_mobprogs(fpArea)*/
		else if (!str_cmp(word, "OBJPROGS"))
		    skip_section(fpArea, word);
		else if (!str_cmp(word, "ROOMPROGS"))
		    skip_section(fpArea, word);
		else if (!str_cmp(word, "OBJECTS"))
		    skip_section(fpArea, word); /*load_objects (fpArea)*/
		else if (!str_cmp(word, "RESETS"))
		    load_resets(fpArea);
		else if (!str_cmp(word, "ROOMS"))
		    skip_section(fpArea, word); /*load_rooms   (fpArea)*/
		else if (!str_cmp(word, "SHOPS"))
		    load_shops(fpArea);
		else if (!str_cmp(word, "SOCIALS"))
		    skip_section(fpArea, word); /*load_socials (fpArea)*/
		else if (!str_cmp(word, "SPECIALS"))
		    skip_section(fpArea, word); /*load_specials(fpArea)*/
		else if (!str_cmp( word, "CHAT"))
		    skip_section(fpArea, word); /*load_chat(fpArea)*/
		else
		{
		    bugf("Boot_db: bad section name.");
		    exit(1);
		}
	    }

	    if (fpArea != stdin)
		fclose(fpArea);
	    fpArea = NULL;
	}
	fclose(fpList);
    }

    /*
     * Fix up exits.
     * Declare db booting over.
     * Reset all areas once.
     * Load up the songs, notes and ban files.
     */

    load_notes();
    load_bans();
    load_songs();
    load_exceptions();
    load_dreams();
    load_quotes();
   // load_arenastats();
    load_stoplist();
    load_badnames();
    load_whois();
    log_string("Loading recipes.");
    load_recipes();
    log_string("Ok.");
    log_string("Loading query.");
    load_query();
    log_string("Ok.");

    fix_exits();
    fix_mobprogs();
    fix_objprogs();
    fix_roomprogs();
    fBootDb	= FALSE;
    convert_objects();           /* ROM OLC */
    area_update();
    load_corpses();
    load_popularity();

// Сундуки сначала читам, удаляем, затем сохраняем с новыми id
    load_chest();
    save_chest(0);

    if (cfg.use_db)
    {
	dbdump(NULL);
	db_tables();
    }

//    load_lua();

    return;
}

/*
 * Snarf an 'area' header line.
 * OLC modified to read in stock areas (used once)
 */
void load_area(FILE *fp)
{
    AREA_DATA *pArea;

    pArea		= alloc_perm(sizeof(*pArea));
    /*    pArea->reset_first	= NULL;
     pArea->reset_last	= NULL; */
    pArea->file_name	= fread_string(fp);

    pArea->area_flags   = AREA_LOADING;         /* OLC */
    pArea->security     = 9;                    /* OLC */ /* 9 -- Hugin */
    pArea->builders     = str_dup("None");    /* OLC */
    pArea->vnum         = top_area;             /* OLC */


    pArea->name		= fread_string(fp);
    pArea->credits	= fread_string(fp);
    pArea->min_vnum	= fread_number(fp);
    pArea->max_vnum	= fread_number(fp);
    pArea->age		= 15;
    pArea->nplayer	= 0;
    pArea->empty	= FALSE;

    if (!area_first)
	area_first = pArea;
    if ( area_last )
    {
	area_last->next = pArea;
	REMOVE_BIT(area_last->area_flags, AREA_LOADING);        /* OLC */
    }
    area_last   = pArea;
    pArea->next = NULL;
    current_area	= pArea;

    top_area++;
    /*    log_string(pArea->name);    */
    return;
}


/*
 * OLC
 * Use these macros to load any new area formats that you choose to
 * support on your MUD.  See the new_load_area format below for
 * a short example.
 */
#if defined(KEY)
#undef KEY
#endif

#define KEY(literal, field, value)          \
    if (!str_cmp(word, literal))\
{                           \
    field  = value;         \
    fMatch = TRUE;          \
    break;                  \
}

#define SKEY(string, field)                  \
    if (!str_cmp(word, string))  \
{                            \
    free_string(field);      \
    field = fread_string(fp);\
    fMatch = TRUE;           \
    /*          if (!strcmp(string, "Name")) log_string(field); */		    \
    break;                   \
}

/* OLC
 * Snarf an 'area' header line.   Check this format.  MUCH better.  Add fields
 * too.
 *
 * #AREAFILE
 * Name   Newbie School~
 * Builders Locke
 * Security 9
 * VNUMs 100 199
 * End
 */
void new_load_area(FILE *fp)
{
    AREA_DATA *pArea;
    char      *word;
    bool      fMatch;
    int i;

    pArea               = alloc_perm(sizeof(*pArea));
    pArea->age          = 15;
    pArea->nplayer      = 0;
    pArea->file_name    = str_dup(strArea);
    pArea->vnum         = top_area;
    pArea->name         = str_dup("New Area");
    pArea->builders     = str_dup("");
    pArea->security     = 9;                    /* 9 -- Hugin */
    pArea->min_vnum     = 0;
    pArea->max_vnum     = 0;
    pArea->area_flags   = 0;
    /*  pArea->recall       = ROOM_VNUM_TEMPLE;        ROM OLC */
    for (i = 0; i < MAX_POPUL; i++)
	pArea->popularity[i]	= 0;
    pArea->version	= 1;


    for (; ;)
    {
	word   = feof(fp) ? "End" : fread_word(fp);
	fMatch = FALSE;

	switch (UPPER(word[0]))
	{
	case 'C':
	    SKEY("Credits", pArea->credits);
	    break;
	case 'N':
	    SKEY("Name", pArea->name);
	    break;
	case 'S':
	    KEY("Security", pArea->security, fread_number(fp));
	    break;
	case 'V':
	    KEY("Version", pArea->version, fread_number(fp));
	    if (!str_cmp(word, "VNUMs"))
	    {
		pArea->min_vnum = fread_number(fp);
		pArea->max_vnum = fread_number(fp);
	    }
	    break;
	case 'E':
	    if (!str_cmp(word, "End"))
	    {
		fMatch = TRUE;
		if (area_first == NULL)
		    area_first = pArea;
		if (area_last  != NULL)
		    area_last->next = pArea;
		area_last   = pArea;
		pArea->next = NULL;
		top_area++;

		return;
	    }
	    break;
	case 'B':
	    SKEY("Builders", pArea->builders);
	    break;
	case 'F':
	    KEY("Flags", pArea->area_flags, fread_flag(fp));
	    break;
	}
    }
}

/*
 * Sets vnum range for area using OLC protection features.
 */
void assign_area_vnum(int vnum)
{
    if (area_last->min_vnum == 0 || area_last->max_vnum == 0)
	area_last->min_vnum = area_last->max_vnum = vnum;

    if (vnum != URANGE(area_last->min_vnum, vnum, area_last->max_vnum))
    {
	if (vnum < area_last->min_vnum)
	    area_last->min_vnum = vnum;
	else
	    area_last->max_vnum = vnum;
    }

    return;
}


/*
 * Snarf a help section.
 */
void load_helps(FILE *fp, char *fname)
{
    HELP_DATA *pHelp;
    int level;
    char *keyword;

    for (;;)
    {
	HELP_AREA *had;

	level		= fread_number(fp);
	keyword		= fread_string(fp);

	if (keyword[0] == '$')
	{
	    free_string(keyword);
	    break;
	}

	if (!had_list)
	{
	    had				= new_had();
	    had->filename		= str_dup(fname);
	    had->area			= current_area;
	    if (current_area)
		current_area->helps	= had;
	    had->changed		= FALSE;
	    had_list			= had;
	}
	else if (str_cmp(fname, had_list->filename))
	{
	    had				= new_had();
	    had->filename		= str_dup(fname);
	    had->area			= current_area;
	    if (current_area)
		current_area->helps	= had;
	    had->changed		= FALSE;
	    had->next			= had_list;
	    had_list			= had;
	}
	else
	    had				= had_list;

	pHelp		= new_help();
	pHelp->level	= level;
	free_string(pHelp->keyword);
	pHelp->keyword	= keyword;
	pHelp->help_area= had;

	free_string(pHelp->text);
	pHelp->text	= fread_string(fp);

	if (!str_cmp(pHelp->keyword, "greeting"))
	    help_greeting = pHelp->text;

	STAILQ_INSERT_TAIL(&help_first, pHelp, link);
	STAILQ_INSERT_TAIL(&had->helps, pHelp, area_link);

	top_help++;
    }

    return;
}



/*
 * Snarf a mob section.  old style
 */
void load_old_mob(FILE *fp)
{
    MOB_INDEX_DATA *pMobIndex;
    /* for race updating */
    int race;
    char name[MAX_STRING_LENGTH];
    int i;

    if (!area_last)   /* OLC */
    {
	bugf("Load_mobiles: no #AREA seen yet.");
	exit(1);
    }

    for (; ;)
    {
	int vnum;
	char letter;
	int iHash;

	letter				= fread_letter(fp);
	if (letter != '#')
	{
	    bugf("Load_mobiles: # not found.");
	    exit(1);
	}

	vnum				= fread_number(fp);
	if (vnum == 0)
	    break;

	fBootDb = FALSE;
	if (get_mob_index(vnum) != NULL)
	{
	    bugf("Load_mobiles: vnum %d duplicated.", vnum);
	    exit(1);
	}
	fBootDb = TRUE;

	pMobIndex			= alloc_perm(sizeof(*pMobIndex));
	pMobIndex->vnum			= vnum;
	pMobIndex->area                 = area_last;               /* OLC */
	pMobIndex->new_format		= FALSE;
	pMobIndex->player_name		= fread_string(fp);
	pMobIndex->short_descr		= fread_string(fp);
	pMobIndex->long_descr		= fread_string(fp);
	pMobIndex->description		= fread_string(fp);

	pMobIndex->long_descr[0]	= UPPER(pMobIndex->long_descr[0]);
	pMobIndex->description[0]	= UPPER(pMobIndex->description[0]);

	pMobIndex->act			= fread_flag(fp) | ACT_IS_NPC;
	pMobIndex->affected_by		= fread_flag(fp);
	pMobIndex->pShop		= NULL;
	pMobIndex->saving_throw		= fread_number(fp);
	pMobIndex->alignment		= fread_number(fp);
	letter				= fread_letter(fp);
	pMobIndex->level		= fread_number(fp);

	/*
	 * The unused stuff is for imps who want to use the old-style
	 * stats-in-files method.
	 */
	fread_number(fp);	/* Unused */
	fread_number(fp);	/* Unused */
	fread_number(fp);	/* Unused */
	/* 'd'		*/		  fread_letter(fp);	/* Unused */
	fread_number(fp);	/* Unused */
	/* '+'		*/		  fread_letter(fp);	/* Unused */
	fread_number(fp);	/* Unused */
	fread_number(fp);	/* Unused */
	/* 'd'		*/		  fread_letter(fp);	/* Unused */
	fread_number(fp);	/* Unused */
	/* '+'		*/		  fread_letter(fp);	/* Unused */
	fread_number(fp);	/* Unused */
	pMobIndex->wealth               = fread_number(fp)/20;
	/* xp can't be used! */		  fread_number(fp);	/* Unused */
	pMobIndex->start_pos		= fread_number(fp);	/* Unused */
	pMobIndex->default_pos		= fread_number(fp);	/* Unused */

	if (pMobIndex->start_pos < POS_SLEEPING)
	    pMobIndex->start_pos = POS_STANDING;
	if (pMobIndex->default_pos < POS_SLEEPING)
	    pMobIndex->default_pos = POS_STANDING;

	/*
	 * Back to meaningful values.
	 */
	pMobIndex->sex			= fread_number(fp);

	/* compute the race BS */
	one_argument(pMobIndex->player_name, name);

	if (name[0] == '\0' || (race =  race_lookup(name)) == -1)
	{
	    /* fill in with blanks */
	    pMobIndex->race = race_lookup("human");
	    pMobIndex->off_flags = OFF_DODGE|OFF_DISARM|OFF_TRIP|ASSIST_VNUM;

	    for (i = 0; i < DAM_MAX; i++)
		pMobIndex->resists[i] = 0;

	    pMobIndex->form = FORM_EDIBLE
		| FORM_SENTIENT
		| FORM_BIPED
		| FORM_MAMMAL;
	    pMobIndex->parts = PART_HEAD
		| PART_ARMS
		| PART_LEGS
		| PART_HEART
		| PART_BRAINS
		| PART_GUTS
		| PART_SKIN;
	}
	else
	{
	    pMobIndex->race = race;
	    pMobIndex->off_flags = OFF_DODGE
		| OFF_DISARM
		| OFF_TRIP
		| ASSIST_RACE
		| race_table[race].off;

	    for (i = 0; i < DAM_MAX; i++)
		pMobIndex->resists[i] = race_table[race].resists[i];

	    pMobIndex->form = race_table[race].form;
	    pMobIndex->parts = race_table[race].parts;
	}

	if (letter != 'S')
	{
	    bugf("Load_mobiles: vnum %d non-S.", vnum);
	    exit(1);
	}

	convert_mobile(pMobIndex);                           /* ROM OLC */

	iHash			= vnum % MAX_KEY_HASH;
	pMobIndex->next		= mob_index_hash[iHash];
	mob_index_hash[iHash]	= pMobIndex;
	top_mob_index++;
	top_vnum_mob = top_vnum_mob < vnum ? vnum : top_vnum_mob;  /* OLC */

	top_vnum = top_vnum < vnum ? vnum : top_vnum;

	assign_area_vnum(vnum);                                  /* OLC */
	kill_table[URANGE(0, pMobIndex->level, MAX_LEVEL-1)].number++;
    }

    return;
}

/*
 * Snarf an obj section.  old style
 */
void load_old_obj(FILE *fp)
{
    OBJ_INDEX_DATA *pObjIndex;

    if (!area_last)   /* OLC */
    {
	bugf("Load_objects: no #AREA seen yet.");
	exit(1);
    }

    for (; ;)
    {
	int vnum;
	char letter;
	int iHash;

	letter				= fread_letter(fp);
	if (letter != '#')
	{
	    bugf("Load_objects: # not found.");
	    exit(1);
	}

	vnum				= fread_number(fp);
	if (vnum == 0)
	    break;

	fBootDb = FALSE;
	if (get_obj_index(vnum) != NULL)
	{
	    bugf("Load_objects: vnum %d duplicated.", vnum);
	    exit(1);
	}
	fBootDb = TRUE;

	pObjIndex			= alloc_perm(sizeof(*pObjIndex));
	pObjIndex->vnum			= vnum;
	pObjIndex->area                 = area_last;            /* OLC */
	pObjIndex->new_format		= FALSE;
	pObjIndex->reset_num	 	= 0;
	pObjIndex->name			= fread_string(fp);
	pObjIndex->short_descr		= fread_string(fp);
	pObjIndex->description		= fread_string(fp);
	/* Action description */	  fread_string(fp);

	pObjIndex->short_descr[0]	= LOWER(pObjIndex->short_descr[0]);
	pObjIndex->description[0]	= UPPER(pObjIndex->description[0]);
	pObjIndex->material		= str_dup("");

	pObjIndex->item_type		= fread_number(fp);
	pObjIndex->extra_flags		= fread_flag(fp);
	pObjIndex->wear_flags		= fread_flag(fp);
	pObjIndex->value[0]		= fread_number(fp);
	pObjIndex->value[1]		= fread_number(fp);
	pObjIndex->value[2]		= fread_number(fp);
	pObjIndex->value[3]		= fread_number(fp);
	pObjIndex->value[4]		= 0;
	pObjIndex->level		= 0;
	pObjIndex->condition 		= 100;
	pObjIndex->weight		= fread_number(fp);
	pObjIndex->weight		= UMAX(10, pObjIndex->weight);
	pObjIndex->cost			= fread_number(fp);	/* Unused */
	/* Cost per day */		  fread_number(fp);
	pObjIndex->affected		= NULL;


	if (pObjIndex->item_type == ITEM_WEAPON)
	{
	    if (is_name("two", pObjIndex->name)
		|| is_name("two-handed", pObjIndex->name)
		|| is_name("claymore", pObjIndex->name))
	    {
		SET_BIT(pObjIndex->value[4], WEAPON_TWO_HANDS);
	    }
	}

	for (; ;)
	{
	    char letter;

	    letter = fread_letter(fp);

	    if (letter == 'A')
	    {
		AFFECT_DATA *paf;

		paf			= alloc_perm(sizeof(*paf));
		paf->where		= TO_OBJECT;
		paf->type		= -1;
		paf->level		= 20; /* RT temp fix */
		paf->duration		= -1;
		paf->location		= fread_number(fp);
		paf->modifier		= fread_number(fp);
		paf->bitvector		= 0;
		paf->next		= pObjIndex->affected;
		paf->prev		= NULL;
		if (pObjIndex->affected)
		    pObjIndex->affected->prev = paf;
		pObjIndex->affected	= paf;
		top_affect++;
	    }

	    else if (letter == 'E')
	    {
		EXTRA_DESCR_DATA *ed;

		ed			= alloc_perm(sizeof(*ed));
		ed->keyword		= fread_string(fp);
		ed->description		= fread_string(fp);
		ed->next		= pObjIndex->extra_descr;
		pObjIndex->extra_descr	= ed;
		top_ed++;
	    }

	    else
	    {
		ungetc(letter, fp);
		break;
	    }
	}

	/* fix armors */
	if (pObjIndex->item_type == ITEM_ARMOR)
	{
	    pObjIndex->value[1] = pObjIndex->value[0];
	    pObjIndex->value[2] = pObjIndex->value[1];
	}

	/*
	 * Translate spell "slot numbers" to internal "skill numbers."
	 */
	switch (pObjIndex->item_type)
	{
	case ITEM_PILL:
	case ITEM_POTION:
	case ITEM_SCROLL:
	    pObjIndex->value[1] = slot_lookup(pObjIndex->value[1]);
	    pObjIndex->value[2] = slot_lookup(pObjIndex->value[2]);
	    pObjIndex->value[3] = slot_lookup(pObjIndex->value[3]);
	    pObjIndex->value[4] = slot_lookup(pObjIndex->value[4]);
	    break;

	case ITEM_STAFF:
	case ITEM_WAND:
	    pObjIndex->value[3] = slot_lookup(pObjIndex->value[3]);
	    break;
	}

	iHash			= vnum % MAX_KEY_HASH;
	pObjIndex->next		= obj_index_hash[iHash];
	obj_index_hash[iHash]	= pObjIndex;
	top_obj_index++;
	top_vnum_obj = top_vnum_obj < vnum ? vnum : top_vnum_obj;   /* OLC */
	top_vnum = top_vnum < vnum ? vnum : top_vnum;
	assign_area_vnum(vnum);                                   /* OLC */
    }

    return;
}

/*
 * Adds a reset to a room.  OLC
 * Similar to add_reset in olc.c
 */
void new_reset(ROOM_INDEX_DATA *pR, RESET_DATA *pReset)
{
    RESET_DATA *pr;

    if (!pR)
	return;

    pr = pR->reset_last;

    if (!pr)
    {
	pR->reset_first = pReset;
	pR->reset_last  = pReset;
    }
    else
    {
	pR->reset_last->next = pReset;
	pR->reset_last       = pReset;
	pR->reset_last->next = NULL;
    }

    top_reset++;
    return;
}


/*
 * Snarf a room section.
 */
void load_rooms(FILE *fp)
{
    ROOM_INDEX_DATA *pRoomIndex;

    if (area_last == NULL)
    {
	bugf("Load_rooms: no #AREA seen yet.");
	exit(1);
    }

    for (; ;)
    {
	int  vnum;
	int  door;
	int  iHash;
	char letter;
	char *tstr;

	letter				= fread_letter(fp);
	if (letter != '#')
	{
	    bugf("Load_rooms: # not found.");
	    exit(1);
	}

	vnum				= fread_number(fp);

	if (vnum == 0)
	    break;

	fBootDb = FALSE;
	if (get_room_index(vnum) != NULL)
	{
	    bugf("Load_rooms: vnum %d duplicated.", vnum);
	    exit(1);
	}
	fBootDb = TRUE;

	pRoomIndex			= alloc_perm(sizeof(*pRoomIndex));
	LIST_INIT(&pRoomIndex->people);
	pRoomIndex->owner		= str_dup("");
	pRoomIndex->comments		= str_dup("");
	pRoomIndex->last_edited		= str_dup("");
	pRoomIndex->contents		= NULL;
	pRoomIndex->extra_descr		= NULL;
	pRoomIndex->affected		= NULL;
	pRoomIndex->area		= area_last;
	pRoomIndex->vnum		= vnum;
	pRoomIndex->name		= fread_string(fp);
	pRoomIndex->description		= fread_string(fp);
	/* Area number */		  fread_number(fp);
	pRoomIndex->room_flags		= fread_flag(fp);

	/* Room levels hack with old areas */
	pRoomIndex->min_level = 0;
	pRoomIndex->max_level = MAX_LEVEL;
	if (IS_SET(pRoomIndex->room_flags, ROOM_IMP_ONLY))
	{
	    pRoomIndex->min_level = pRoomIndex->max_level = MAX_LEVEL;
	    REMOVE_BIT(pRoomIndex->room_flags, ROOM_IMP_ONLY);
	}

	if (IS_SET(pRoomIndex->room_flags, ROOM_GODS_ONLY))
	{
	    pRoomIndex->min_level = LEVEL_IMMORTAL;
	    pRoomIndex->max_level = MAX_LEVEL;
	    REMOVE_BIT(pRoomIndex->room_flags, ROOM_GODS_ONLY);
	}

	if (IS_SET(pRoomIndex->room_flags, ROOM_HEROES_ONLY))
	{
	    pRoomIndex->min_level = LEVEL_HERO;
	    pRoomIndex->max_level = MAX_LEVEL;
	    REMOVE_BIT(pRoomIndex->room_flags, ROOM_HEROES_ONLY);
	}

	if (IS_SET(pRoomIndex->room_flags, ROOM_NEWBIES_ONLY))
	{
	    pRoomIndex->min_level = 0;
	    pRoomIndex->max_level = LEVEL_NEWBIE;
	    REMOVE_BIT(pRoomIndex->room_flags, ROOM_NEWBIES_ONLY);
	}

	/* horrible hack */
	/*  	if (3000 <= vnum && vnum < 3400)
	 SET_BIT(pRoomIndex->room_flags, ROOM_LAW); */
	pRoomIndex->sector_type		= fread_number(fp);
	pRoomIndex->light		= 0;
	for (door = 0; door <= 5; door++)
	    pRoomIndex->exit[door] = NULL;

	/* defaults */
	pRoomIndex->heal_rate = 100;
	pRoomIndex->mana_rate = 100;
	pRoomIndex->progs    = NULL;

	for (; ;)
	{
	    letter = fread_letter(fp);

	    if (letter == 'S')
		break;

	    if (letter == 'H') /* healing room */
		pRoomIndex->heal_rate = fread_number(fp);
	    else if (letter == 'M') /* mana room */
		pRoomIndex->mana_rate = fread_number(fp);
	    else if (letter == 'C') /* clan */
	    {
		if (pRoomIndex->clan != 0)
		{
		    bugf("Load_rooms: duplicate clan fields.");
		    exit(1);
		}

		tstr = fread_string(fp);
		pRoomIndex->clan = clan_lookup(tstr);
		free_string(tstr);
	    }
	    else if (letter == 'D')
	    {
		EXIT_DATA *pexit;
		int locks, ex_race;

		door = fread_number(fp);
		if (door < 0 || door > 5)
		{
		    bugf("Fread_rooms: vnum %d has bad door number.", vnum);
		    exit(1);
		}

		pexit			= alloc_perm(sizeof(*pexit));
		pexit->description	= fread_string(fp);
		pexit->keyword		= fread_string(fp);

		if (IS_NULLSTR(pexit->keyword))
		{
		    free_string(pexit->keyword);
		    pexit->keyword = str_dup("дверь");
		}

		pexit->exit_info	= 0;
		pexit->rs_flags         = 0;                    /* OLC */
		locks			= fread_number(fp);
		pexit->key		= fread_number(fp);
		pexit->u1.vnum		= fread_number(fp);
		pexit->orig_door        = door;                 /* OLC */

		ex_race = locks & 0xF0;
		locks &= 0x0F;

		switch (locks)
		{
		case 1: pexit->exit_info = EX_ISDOOR;
			pexit->rs_flags  = EX_ISDOOR;		     break;
		case 2: pexit->exit_info = EX_ISDOOR | EX_PICKPROOF;
			pexit->rs_flags  = EX_ISDOOR | EX_PICKPROOF; break;
		case 3: pexit->exit_info = EX_ISDOOR | EX_NOPASS;
			pexit->rs_flags  = EX_ISDOOR | EX_NOPASS;    break;
		case 4: pexit->exit_info = EX_ISDOOR|EX_NOPASS|EX_PICKPROOF;
			pexit->rs_flags  = EX_ISDOOR|EX_NOPASS|EX_PICKPROOF;
			break;
		}

		if (ex_race != 0)
		{
		    pexit->exit_info |= EX_RACE;
		    pexit->rs_flags  |= EX_RACE;
		}

		pRoomIndex->exit[door]	= pexit;
		top_exit++;
	    }
	    else if (letter == 'E')
	    {
		EXTRA_DESCR_DATA *ed;

		ed			= alloc_perm(sizeof(*ed));
		ed->keyword		= fread_string(fp);
		ed->description		= fread_string(fp);
		ed->next		= pRoomIndex->extra_descr;
		pRoomIndex->extra_descr	= ed;
		top_ed++;
	    }
	    else if (letter == 'L')
	    {
		pRoomIndex->min_level = URANGE(0, fread_number(fp), MAX_LEVEL);
		pRoomIndex->max_level = URANGE(0, fread_number(fp), MAX_LEVEL);
	    }
	    else if (letter == 'O')
	    {
		if (pRoomIndex->owner[0] != '\0')
		{
		    bugf("Load_rooms: duplicate owner.");
		    exit(1);
		}

		free_string(pRoomIndex->owner);
		pRoomIndex->owner = fread_string(fp);
	    }
	    else if (letter == 'R')
	    {
		PROG_LIST *pRprog;
		char *word;
		int trigger = 0;

		pRprog		= alloc_perm(sizeof(*pRprog));
		SET_BIT(pRprog->trig_flags, get_prog_when(fp));
		word		= fread_word(fp);
		if (!(trigger = flag_lookup(word, rprog_flags)))
		{
		    bugf("ROOMprogs: invalid trigger.");
		    exit(1);
		}
		SET_BIT(pRprog->trig_flags, trigger);
		pRprog->trig_type	= trigger;
		pRprog->vnum		= fread_number(fp);
		pRprog->trig_phrase	= fread_string(fp);
		pRprog->next		= pRoomIndex->progs;
		pRoomIndex->progs	= pRprog;
	    }
	    else if (letter == '*')
	    {
		free_string(pRoomIndex->comments);
		pRoomIndex->comments = fread_string(fp);
	    }
	    else if (letter == 'Z')
	    {
		free_string(pRoomIndex->last_edited);
		pRoomIndex->last_edited = fread_string(fp);
	    }
	    else
	    {
		bugf("Load_rooms: vnum %d has flag not 'DES'.", vnum);
		exit(1);
	    }
	}

	iHash			= vnum % MAX_KEY_HASH;
	pRoomIndex->next	= room_index_hash[iHash];
	room_index_hash[iHash]	= pRoomIndex;
	top_room++;
	top_vnum_room = top_vnum_room < vnum ? vnum : top_vnum_room; /* OLC */
	top_vnum = top_vnum < vnum ? vnum : top_vnum;
	assign_area_vnum(vnum);                                    /* OLC */

    }

    return;
}



/*
 * Snarf a shop section.
 */
void load_shops(FILE *fp)
{
    SHOP_DATA *pShop;
    int	keeper;

    for (; ;)
    {
	MOB_INDEX_DATA *pMobIndex;
	int iTrade;

	keeper = fread_number(fp);
	if (keeper == 0)
	    break;
	pShop			= alloc_perm(sizeof(*pShop));
	pShop->keeper		= keeper;
	for (iTrade = 0; iTrade < MAX_TRADE; iTrade++)
	    pShop->buy_type[iTrade]	= fread_number(fp);
	pShop->profit_buy	= fread_number(fp);
	pShop->profit_sell	= fread_number(fp);
	pShop->open_hour	= fread_number(fp);
	pShop->close_hour	= fread_number(fp);
	fread_to_eol(fp);
	pMobIndex		= get_mob_index(pShop->keeper);
	pMobIndex->pShop	= pShop;

	//	if (pMobIndex->wealth == 0)
	//	  pMobIndex->wealth = 10000;

	//	pMobIndex->wealth *= 3;

	if (shop_first == NULL)
	    shop_first = pShop;
	if (shop_last  != NULL)
	    shop_last->next = pShop;

	shop_last	= pShop;
	pShop->next	= NULL;
	top_shop++;
    }

    return;
}


/*
 * Snarf spec proc declarations.
 */
void load_specials(FILE *fp)
{
    for (; ;)
    {
	MOB_INDEX_DATA *pMobIndex;
	char letter;

	switch (letter = fread_letter(fp))
	{
	default:
	    bugf("Load_specials: letter '%c' not *MS.", letter);
	    exit(1);

	case 'S':
	    return;

	case '*':
	    break;

	case 'M':
	    pMobIndex		= get_mob_index	(fread_number (fp));
	    pMobIndex->spec_fun	= spec_lookup	(fread_word   (fp));
	    if (pMobIndex->spec_fun == 0)
	    {
		bugf("Load_specials: 'M': vnum %d.", pMobIndex->vnum);
		exit(1);
	    }
	    break;
	}

	fread_to_eol(fp);
    }
}


/*
 * Translate all room exits from virtual to real.
 * Has to be done after all rooms are read in.
 * Check for bad reverse exits.
 */
void fix_exits(void)
{
    extern const int16_t rev_dir[];
    ROOM_INDEX_DATA *pRoomIndex;
    ROOM_INDEX_DATA *to_room;
    EXIT_DATA *pexit;
    EXIT_DATA *pexit_rev;
    int iHash;
    int door;

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
	for (pRoomIndex  = room_index_hash[iHash];
	     pRoomIndex != NULL;
	     pRoomIndex  = pRoomIndex->next)
	{
	    bool fexit;

	    fexit = FALSE;
	    for (door = 0; door <= 5; door++)
	    {
		if ((pexit = pRoomIndex->exit[door]) != NULL)
		{
		    if (pexit->u1.vnum <= 0
			|| get_room_index(pexit->u1.vnum) == NULL)
			pexit->u1.to_room = NULL;
		    else
		    {
			fexit = TRUE;
			pexit->u1.to_room = get_room_index(pexit->u1.vnum);
		    }
		}
	    }
	    if (!fexit)
		SET_BIT(pRoomIndex->room_flags, ROOM_NO_MOB);
	}
    }

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
	for (pRoomIndex  = room_index_hash[iHash];
	     pRoomIndex != NULL;
	     pRoomIndex  = pRoomIndex->next)
	{
	    for (door = 0; door <= 5; door++)
	    {
		if ((pexit     = pRoomIndex->exit[door]      ) != NULL
		    &&   (to_room   = pexit->u1.to_room           ) != NULL
		    &&   (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
		    &&   pexit_rev->u1.to_room != pRoomIndex
		    &&   (pRoomIndex->vnum < 1200 || pRoomIndex->vnum > 1299))
		{
		    /*		    sprintf(buf, "Fix_exits: %d:%d -> %d:%d -> %d.",
		     pRoomIndex->vnum, door,
		     to_room->vnum,    rev_dir[door],
		     (pexit_rev->u1.to_room == NULL)
		     ? 0 : pexit_rev->u1.to_room->vnum);
		     bug(buf, 0);
		     */
		}
	    }
	}
    }

    return;
}

/*
 * Load mobprogs section
 */
void load_mobprogs(FILE *fp)
{
    PROG_CODE *pMprog;

    if (area_last == NULL)
    {
	bugf("Load_mobprogs: no #AREA seen yet.");
	exit(1);
    }

    for (; ;)
    {
	int vnum;
	char letter;

	letter		  = fread_letter(fp);
	if (letter != '#')
	{
	    bugf("Load_mobprogs: # not found.");
	    exit(1);
	}

	vnum		 = fread_number(fp);
	if (vnum == 0)
	    break;

	fBootDb = FALSE;
	if (get_prog_index(vnum, PRG_MPROG) != NULL)
	{
	    bugf("Load_mobprogs: vnum %d duplicated.", vnum);
	    exit(1);
	}
	fBootDb = TRUE;

	pMprog		= alloc_perm(sizeof(*pMprog));
	pMprog->vnum  	= vnum;
	pMprog->code  	= fread_string(fp);
	if (mprog_list == NULL)
	    mprog_list = pMprog;
	else
	{
	    pMprog->next = mprog_list;
	    mprog_list 	= pMprog;
	}
	top_mprog_index++;
    }
    return;
}

/*
 *  Translate mobprog vnums pointers to real code
 */
void fix_mobprogs(void)
{
    MOB_INDEX_DATA *pMobIndex;
    PROG_LIST        *list;
    PROG_CODE        *prog;
    int iHash;

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
	for (pMobIndex   = mob_index_hash[iHash];
	     pMobIndex   != NULL;
	     pMobIndex   = pMobIndex->next)
	{
	    for (list = pMobIndex->progs; list != NULL; list = list->next)
	    {
		if ((prog = get_prog_index(list->vnum, PRG_MPROG)) != NULL)
		    list->code = prog->code;
		else
		{
		    bugf("Fix_mobprogs: code vnum %d not found.", list->vnum);
		    exit(1);
		}
	    }
	}
    }
}


/*
 * Repopulate areas periodically.
 */
void area_update(void)
{
    AREA_DATA *pArea;
    char buf[MAX_STRING_LENGTH];

    for (pArea = area_first; pArea != NULL; pArea = pArea->next)
    {

	if (++pArea->age < 3)
	    continue;

	/*
	 * Check age and reset.
	 * Note: Mud School resets every 3 minutes (not 15).
	 */
	if ((!pArea->empty
	     && (pArea->nplayer == 0 || pArea->age >= 15))
	    || pArea->age >= 31)
	{
	    reset_area(pArea);
	    sprintf(buf, "Зона '%s' обновилась.", pArea->name);
	    convert_dollars(buf);
	    wiznet(buf, NULL, NULL, WIZ_RESETS, 0, 0);

	    pArea->age = number_range(0, 3);

	    if (IS_SET(pArea->area_flags, AREA_FASTREPOP))
		pArea->age = 15 - 2;
	    else if (pArea->nplayer == 0)
		pArea->empty = TRUE;
	}
    }

    return;
}



/*
 * Create an instance of a mobile.
 */
CHAR_DATA *create_mobile(MOB_INDEX_DATA *pMobIndex)
{
    CHAR_DATA *mob;
    int i;
    AFFECT_DATA af;

    mobile_count++;

    if (pMobIndex == NULL)
    {
	bugf("Create_mobile: NULL pMobIndex.");
	safe_exit(1);
    }

    mob = new_char();

    mob->pIndexData	= pMobIndex;

    mob->id		= get_mob_id();
    free_string(mob->name);
    mob->name		= str_dup(pMobIndex->player_name);    /* OLC */
    free_string(mob->short_descr);
    mob->short_descr	= str_dup(pMobIndex->short_descr);    /* OLC */
    free_string(mob->long_descr);
    mob->long_descr	= str_dup(pMobIndex->long_descr);     /* OLC */
    free_string(mob->description);
    mob->description	= str_dup(pMobIndex->description);    /* OLC */
    mob->spec_fun	= pMobIndex->spec_fun;
    free_string(mob->prompt);
    mob->prompt		= NULL;
    mob->mprog_target   = NULL;

    mob->angry		= 0;
    mob->last_regexp	= -1;


    if (pMobIndex->new_format)
	/* load in new style */
    {
	/* read from prototype */
	mob->group		= pMobIndex->group;
	mob->act 		= pMobIndex->act;
	mob->comm		= COMM_NOCHANNELS|COMM_NOSHOUT|COMM_NOTELL;
	mob->affected_by	= pMobIndex->affected_by;
	mob->alignment		= pMobIndex->alignment;
	mob->level		= pMobIndex->level;
	mob->saving_throw	= pMobIndex->saving_throw;
	mob->hitroll		= pMobIndex->hitroll;
	mob->damroll		= pMobIndex->damage[DICE_BONUS];
	mob->max_hit		= dice(pMobIndex->hit[DICE_NUMBER],
				       pMobIndex->hit[DICE_TYPE])
	    + pMobIndex->hit[DICE_BONUS];
	mob->hit		= mob->max_hit;
	mob->max_mana		= dice(pMobIndex->mana[DICE_NUMBER],
				       pMobIndex->mana[DICE_TYPE])
	    + pMobIndex->mana[DICE_BONUS];
	mob->mana		= mob->max_mana;
	mob->damage[DICE_NUMBER]= pMobIndex->damage[DICE_NUMBER];
	mob->damage[DICE_TYPE]	= pMobIndex->damage[DICE_TYPE];
	mob->dam_type		= pMobIndex->dam_type;
	if (mob->dam_type == 0)
	    switch(number_range(1, 3))
	    {
	    case (1): mob->dam_type = 3;        break;  /* slash */
	    case (2): mob->dam_type = 7;        break;  /* pound */
	    case (3): mob->dam_type = 11;       break;  /* pierce */
	    }
	for (i = 0; i < 4; i++)
	    mob->armor[i]	= pMobIndex->ac[i];
	mob->off_flags		= pMobIndex->off_flags;
	for (i = 0; i < DAM_MAX; i++)
	    mob->resists[i] = pMobIndex->resists[i];
	mob->start_pos		= pMobIndex->start_pos;
	mob->default_pos	= pMobIndex->default_pos;
	mob->sex		= pMobIndex->sex;
	mob->race		= pMobIndex->race;
	mob->form		= pMobIndex->form;
	mob->parts		= pMobIndex->parts;
	mob->size		= pMobIndex->size;
	mob->material		= str_dup(pMobIndex->material);

	/* computed on the spot */

	for (i = 0; i < MAX_STATS; i ++)
	    mob->perm_stat[i] = UMIN(MAX_STAT, 11 + mob->level/3);

	if (IS_SET(mob->act, ACT_WARRIOR))
	{
	    mob->perm_stat[STAT_STR] += 3;
	    mob->perm_stat[STAT_INT] -= 1;
	    mob->perm_stat[STAT_CON] += 2;
	}

	if (IS_SET(mob->act, ACT_THIEF))
	{
	    mob->perm_stat[STAT_DEX] += 3;
	    mob->perm_stat[STAT_INT] += 1;
	    mob->perm_stat[STAT_WIS] -= 1;
	}

	if (IS_SET(mob->act, ACT_CLERIC))
	{
	    mob->perm_stat[STAT_WIS] += 3;
	    mob->perm_stat[STAT_DEX] -= 1;
	    mob->perm_stat[STAT_STR] += 1;
	}

	if (IS_SET(mob->act, ACT_MAGE))
	{
	    mob->perm_stat[STAT_INT] += 3;
	    mob->perm_stat[STAT_STR] -= 1;
	    mob->perm_stat[STAT_DEX] += 1;
	}

	if (IS_SET(mob->off_flags, OFF_FAST))
	    mob->perm_stat[STAT_DEX] += 2;

	mob->perm_stat[STAT_STR] += mob->size - SIZE_MEDIUM;
	mob->perm_stat[STAT_CON] += (mob->size - SIZE_MEDIUM) / 2;

	/* let's get some spell action */
	if (IS_AFFECTED(mob, AFF_SANCTUARY))
	{
	    af.where	 = TO_AFFECTS;
	    af.type      = skill_lookup("sanctuary");
	    af.level     = mob->level;
	    af.duration  = -1;
	    af.location  = APPLY_NONE;
	    af.modifier  = 0;
	    af.bitvector = AFF_SANCTUARY;
	    af.caster_id = get_id(mob);
	    affect_to_char(mob, &af);
	}

	if (IS_AFFECTED(mob, AFF_HASTE))
	{
	    af.where	 = TO_AFFECTS;
	    af.type      = skill_lookup("haste");
	    af.level     = mob->level;
	    af.duration  = -1;
	    af.location  = APPLY_DEX;
	    af.modifier  = 1 + (mob->level >= 18) + (mob->level >= 25) +
		(mob->level >= 32);
	    af.bitvector = AFF_HASTE;
	    af.caster_id = get_id(mob);
	    affect_to_char(mob, &af);
	}

	if (IS_AFFECTED(mob, AFF_PROTECT_EVIL))
	{
	    af.where	 = TO_AFFECTS;
	    af.type	 = skill_lookup("protection evil");
	    af.level	 = mob->level;
	    af.duration	 = -1;
	    af.location	 = APPLY_SAVES;
	    af.modifier	 = -1;
	    af.bitvector = AFF_PROTECT_EVIL;
	    af.caster_id = get_id(mob);
	    affect_to_char(mob, &af);
	}

	if (IS_AFFECTED(mob, AFF_PROTECT_GOOD))
	{
	    af.where	 = TO_AFFECTS;
	    af.type      = skill_lookup("protection good");
	    af.level     = mob->level;
	    af.duration  = -1;
	    af.location  = APPLY_SAVES;
	    af.modifier  = -1;
	    af.bitvector = AFF_PROTECT_GOOD;
	    af.caster_id = get_id(mob);
	    affect_to_char(mob, &af);
	}
    }
    else /* read in old format and convert */
    {
		mob->act		= pMobIndex->act;
		mob->affected_by	= pMobIndex->affected_by;
		mob->alignment		= pMobIndex->alignment;
		mob->saving_throw   = pMobIndex->saving_throw;
		mob->level		= pMobIndex->level;
		mob->hitroll		= pMobIndex->hitroll;
		mob->damroll		= 0;
		mob->max_hit		= mob->level * 8 + number_range(
									mob->level * mob->level/4,
									mob->level * mob->level);
		mob->max_hit *= .9;
		mob->hit		= mob->max_hit;
		mob->max_mana		= 100 + dice(mob->level, 10);
		mob->mana		= mob->max_mana;
		switch(number_range(1, 3))
		{
		case (1): mob->dam_type = 3; 	break;  /* slash */
		case (2): mob->dam_type = 7;	break;  /* pound */
		case (3): mob->dam_type = 11;	break;  /* pierce */
		}
		for (i = 0; i < 3; i++)
			mob->armor[i]	= interpolate(mob->level, 100, -100);
		mob->armor[3]		= interpolate(mob->level, 100, 0);
		mob->race		= pMobIndex->race;
		mob->off_flags		= pMobIndex->off_flags;
		for (i = 0; i < DAM_MAX; i++)
			mob->resists[i] = pMobIndex->resists[i];
		mob->start_pos		= pMobIndex->start_pos;
		mob->default_pos	= pMobIndex->default_pos;
		mob->sex		= pMobIndex->sex;
		mob->form		= pMobIndex->form;
		mob->parts		= pMobIndex->parts;
		mob->size		= SIZE_MEDIUM;
		mob->material		= "";

		for (i = 0; i < MAX_STATS; i ++)
			mob->perm_stat[i] = 11 + mob->level/4;
    }

    mob->position = mob->start_pos;

    if (IS_SET(mob->act, ACT_MOUNT))
    {
	mob->max_move += (number_range(5, 10) * mob->level);
	mob->move = mob->max_move;
    }

    if (pMobIndex->wealth == 0 || is_animal(mob))
    {
	mob->silver = 0;
	mob->gold   = 0;
    }
    else
    {
	int64_t wealth;

	wealth = number_range(pMobIndex->wealth/2, 3 * pMobIndex->wealth/2);
	mob->gold = number_range(wealth/200, wealth/100);
	mob->silver = wealth - (mob->gold * 100);
    }

    /* link the mob to the world list */
    LIST_INSERT_HEAD(&char_list, mob, link);

    pMobIndex->count++;
    return mob;
}

/* duplicate a mobile exactly -- except inventory */
void clone_mobile(CHAR_DATA *parent, CHAR_DATA *clone)
{
    int i;
    AFFECT_DATA *paf;

    if (parent == NULL || clone == NULL || !IS_NPC(parent))
	return;

    /* start fixing values */
    free_string(clone->name);
    clone->name 	= str_dup(parent->name);
    clone->version	= parent->version;
    free_string(clone->short_descr);
    clone->short_descr	= str_dup(parent->short_descr);
    free_string(clone->long_descr);
    clone->long_descr	= str_dup(parent->long_descr);
    free_string(clone->description);
    clone->description	= str_dup(parent->description);
    clone->group	= parent->group;
    clone->sex		= parent->sex;
    clone->classid	= parent->classid;
    clone->race		= parent->race;
    clone->level	= parent->level;
    clone->trust	= 0;
    clone->timer	= parent->timer;
    clone->wait		= parent->wait;
    clone->hit		= parent->hit;
    clone->max_hit	= parent->max_hit;
    clone->mana		= parent->mana;
    clone->max_mana	= parent->max_mana;
    clone->move		= parent->move;
    clone->max_move	= parent->max_move;
    clone->gold		= parent->gold;
    clone->silver	= parent->silver;
    clone->exp		= parent->exp;
    clone->act		= parent->act;
    clone->comm		= parent->comm;
    for (i = 0; i < DAM_MAX; i++)
	clone->resists[i] = parent->resists[i];
    clone->invis_level	= parent->invis_level;
    clone->affected_by	= parent->affected_by;
    clone->position	= parent->position;
    clone->practice	= parent->practice;
    clone->train	= parent->train;
    clone->saving_throw	= parent->saving_throw;
    clone->alignment	= parent->alignment;
    clone->hitroll	= parent->hitroll;
    clone->damroll	= parent->damroll;
    clone->wimpy	= parent->wimpy;
    clone->form		= parent->form;
    clone->parts	= parent->parts;
    clone->size		= parent->size;
    clone->material	= str_dup(parent->material);
    clone->off_flags	= parent->off_flags;
    clone->dam_type	= parent->dam_type;
    clone->start_pos	= parent->start_pos;
    clone->default_pos	= parent->default_pos;
    clone->spec_fun	= parent->spec_fun;

    for (i = 0; i < 4; i++)
	clone->armor[i]	= parent->armor[i];

    for (i = 0; i < MAX_STATS; i++)
    {
	clone->perm_stat[i]	= parent->perm_stat[i];
	clone->mod_stat[i]	= parent->mod_stat[i];
    }

    for (i = 0; i < 3; i++)
	clone->damage[i]	= parent->damage[i];

    /* now add the affects */
    for (paf = parent->affected; paf != NULL; paf = paf->next)
	affect_to_char(clone, paf);

}




/*
 * Create an instance of an object.
 */
OBJ_DATA *create_object(OBJ_INDEX_DATA *pObjIndex, int level)
{
    AFFECT_DATA *paf;
    OBJ_DATA *obj;
    int i;

    if (pObjIndex == NULL)
    {
	bugf("Create_object: NULL pObjIndex.");
	safe_exit(1);
    }

    obj_count++;

    obj = new_obj();

    obj->pIndexData	= pObjIndex;
    obj->in_room	= NULL;
    obj->enchanted	= FALSE;
    obj->id		= get_obj_id();

    if (pObjIndex->new_format)
	obj->level = pObjIndex->level;
    else
	obj->level		= UMAX(0, level);
    obj->wear_loc	= -1;

    obj->name		= str_dup(pObjIndex->name);           /* OLC */
    obj->short_descr	= str_dup(pObjIndex->short_descr);    /* OLC */
    obj->description	= str_dup(pObjIndex->description);    /* OLC */
    obj->material	= str_dup(pObjIndex->material);
    obj->item_type	= pObjIndex->item_type;
    obj->extra_flags	= pObjIndex->extra_flags;
    obj->wear_flags	= pObjIndex->wear_flags;

    for (i = 0; i < 5; i++)
	obj->value[i]	= pObjIndex->value[i];

    obj->weight		= pObjIndex->weight;
    obj->condition	= pObjIndex->condition;
    obj->uncomf         = pObjIndex->uncomf;
    obj->unusable       = pObjIndex->unusable;

    if (level == -1 || pObjIndex->new_format)
	obj->cost	= pObjIndex->cost;
    else
	obj->cost	= obj->level;

    /*
     * Mess with object properties.
     */
    switch (obj->item_type)
    {
    default:
	bugf("Read_object: vnum %d bad type.", pObjIndex->vnum);
	break;

    case ITEM_LIGHT:
	if (obj->value[2] == 999)
	    obj->value[2] = -1;
	break;

    case ITEM_FURNITURE:
    case ITEM_TRASH:
    case ITEM_CONTAINER:
    case ITEM_CHEST:
    case ITEM_MORTAR:
    case ITEM_INGREDIENT:
    case ITEM_DRINK_CON:
    case ITEM_KEY:
    case ITEM_FOOD:
    case ITEM_BOAT:
    case ITEM_CORPSE_NPC:
    case ITEM_CORPSE_PC:
    case ITEM_FOUNTAIN:
    case ITEM_MAP:
    case ITEM_CLOTHING:
    case ITEM_PORTAL:
	if (!pObjIndex->new_format)
	    obj->cost /= 5;
	break;

    case ITEM_TREASURE:
    case ITEM_WARP_STONE:
    case ITEM_ROOM_KEY:
    case ITEM_GEM:
    case ITEM_JEWELRY:
	break;

    case ITEM_JUKEBOX:
	for (i = 0; i < 5; i++)
	    obj->value[i] = -1;
	break;

    case ITEM_SCROLL:
	if (level != -1 && !pObjIndex->new_format)
	    obj->value[0]	= number_fuzzy(obj->value[0]);
	break;

    case ITEM_WAND:
    case ITEM_STAFF:
	if (level != -1 && !pObjIndex->new_format)
	{
	    obj->value[0]	= number_fuzzy(obj->value[0]);
	    obj->value[1]	= number_fuzzy(obj->value[1]);
	    obj->value[2]	= number_fuzzy(obj->value[2]);
	}
	if (!pObjIndex->new_format)
	    obj->cost *= 2;
	break;

    case ITEM_ROD:
	obj->value[3] = 0;
	break;

    case ITEM_TRAP:
	obj->value[2] = 0;
	break;

    case ITEM_SCALE:
    case ITEM_ARTIFACT:
    case ITEM_BOOK:
	obj->value[0] = 0;
	obj->value[1] = 0;
	obj->value[2] = 0;
	obj->value[3] = 0;
	obj->value[4] = 0;
	break;

    case ITEM_WEAPON:
	if (level != -1 && !pObjIndex->new_format)
	{
	    obj->value[1] = number_fuzzy(number_fuzzy(1 * level / 4 + 2));
	    obj->value[2] = number_fuzzy(number_fuzzy(3 * level / 4 + 6));
	}
	break;

    case ITEM_ARMOR:
	if (level != -1 && !pObjIndex->new_format)
	{
	    obj->value[0]	= number_fuzzy(level / 5 + 3);
	    obj->value[1]	= number_fuzzy(level / 5 + 3);
	    obj->value[2]	= number_fuzzy(level / 5 + 3);
	}
	break;

    case ITEM_POTION:
    case ITEM_PILL:
	if (level != -1 && !pObjIndex->new_format)
	    obj->value[0] = number_fuzzy(number_fuzzy(obj->value[0]));
	break;

    case ITEM_MONEY:
	if (!pObjIndex->new_format)
	    obj->value[0]	= obj->cost;
	break;
    }

    for (paf = pObjIndex->affected; paf != NULL; paf = paf->next)
	if (paf->location == APPLY_SPELL_AFFECT || pObjIndex->edited)
	    affect_to_obj(obj, paf);

    obj->next		= object_list;
    obj->prev		= NULL;
    if (object_list)
	object_list->prev = obj;
    object_list		= obj;
    pObjIndex->count++;

    if (is_limit(obj) != -1)
	obj->lifetime = current_time + LIMIT_LIFETIME;
    else
	obj->lifetime = 0;

    for (i = 0; i < MAX_STATS; i++)
	obj->require[i] = pObjIndex->require[i];

    return obj;
}

/* duplicate an object exactly -- except contents */
void clone_object(OBJ_DATA *parent, OBJ_DATA *clone)
{
    int i;
    AFFECT_DATA *paf;
    EXTRA_DESCR_DATA *ed, *ed_new;

    if (parent == NULL || clone == NULL)
	return;

    /* start fixing the object */
    free_string(clone->name);
    clone->name 	= str_dup(parent->name);
    free_string(clone->short_descr);
    clone->short_descr 	= str_dup(parent->short_descr);
    free_string(clone->description);
    clone->description	= str_dup(parent->description);
    clone->item_type	= parent->item_type;
    clone->extra_flags	= parent->extra_flags;
    clone->wear_flags	= parent->wear_flags;
    clone->weight	= parent->weight;
    clone->cost		= parent->cost;
    clone->level	= parent->level;
    clone->condition	= parent->condition;
    free_string(clone->material);
    clone->material	= str_dup(parent->material);
    clone->timer	= parent->timer;
    clone->uncomf       = parent->uncomf;
    clone->unusable     = parent->unusable;

    for (i = 0;  i < 5; i ++)
	clone->value[i]	= parent->value[i];

    /* affects */
    clone->enchanted	= parent->enchanted;

    for (paf = parent->affected; paf != NULL; paf = paf->next)
	affect_to_obj(clone, paf);

    /* extended desc */
    for (ed = parent->extra_descr; ed != NULL; ed = ed->next)
    {
	ed_new                  = new_extra_descr();
	ed_new->keyword    	= str_dup(ed->keyword);
	ed_new->description     = str_dup(ed->description);
	ed_new->next           	= clone->extra_descr;
	clone->extra_descr  	= ed_new;
    }

    if ((i = is_limit(clone)) != -1)
	limits[i+1]++;


}



/*
 * Clear a new character.
 */
void clear_char(CHAR_DATA *ch)
{
    static CHAR_DATA ch_zero;
    int i;

    *ch				= ch_zero;
    ch->name			= &str_empty[0];
    ch->short_descr		= &str_empty[0];
    ch->long_descr		= &str_empty[0];
    ch->description		= &str_empty[0];
    ch->notepad 		= &str_empty[0];
    ch->prompt                  = &str_empty[0];
    ch->logon			= current_time;
    ch->lines			= PAGELEN;
    for (i = 0; i < 4; i++)
	ch->armor[i]		= 100;
    ch->position		= POS_STANDING;
    ch->hit			= 20;
    ch->max_hit			= 20;
    ch->mana			= 100;
    ch->max_mana		= 100;
    ch->move			= 100;
    ch->max_move		= 100;
    ch->on			= NULL;
    for (i = 0; i < MAX_STATS; i ++)
    {
	ch->perm_stat[i] = 13;
	ch->mod_stat[i] = 0;
    }
    return;
}

/*
 * Get an extra description from a list.
 */
char *get_extra_descr(const char *name, EXTRA_DESCR_DATA *ed)
{
    for (; ed != NULL; ed = ed->next)
    {
	if (is_name((char *) name, ed->keyword))
	    return ed->description;
    }
    return NULL;
}



/*
 * Translates mob virtual number to its mob index struct.
 * Hash table lookup.
 */
MOB_INDEX_DATA *get_mob_index(int vnum)
{
    MOB_INDEX_DATA *pMobIndex;

    if (!CHECK_VNUM(vnum))
	return NULL;

    for (pMobIndex  = mob_index_hash[vnum % MAX_KEY_HASH];
	 pMobIndex != NULL;
	 pMobIndex  = pMobIndex->next)
    {
	if (pMobIndex->vnum == vnum)
	    return pMobIndex;
    }

    if (fBootDb)
    {
	bugf("Get_mob_index: bad vnum %d.", vnum);
	exit(1);
    }

    return NULL;
}



/*
 * Translates mob virtual number to its obj index struct.
 * Hash table lookup.
 */
OBJ_INDEX_DATA *get_obj_index(int vnum)
{
    OBJ_INDEX_DATA *pObjIndex;

    if (!CHECK_VNUM(vnum))
	return NULL;

    for (pObjIndex  = obj_index_hash[vnum % MAX_KEY_HASH];
	 pObjIndex != NULL;
	 pObjIndex  = pObjIndex->next)
    {
	if (pObjIndex->vnum == vnum)
	{
	    if (pObjIndex->item_type == ITEM_ARTIFACT && 
		(pObjIndex->vnum < ARTIFACT_MIN_VNUM || pObjIndex->vnum > ARTIFACT_MAX_VNUM))
	    {
		bugf("Artifact not in artifact.are zone, vnum %d", pObjIndex->vnum);
		return NULL;
	    }

	    return pObjIndex;
	}
    }

    if (fBootDb)
    {
	bugf("Get_obj_index: bad vnum %d.", vnum);
	exit(1);
    }

    return NULL;
}


PROG_CODE *get_prog_index(int vnum, int type)
{
    PROG_CODE *prg;

    if (!CHECK_VNUM(vnum))
	return NULL;

    switch (type)
    {
    case PRG_MPROG:
	prg = mprog_list;
	break;
    case PRG_OPROG:
	prg = oprog_list;
	break;
    case PRG_RPROG:
	prg = rprog_list;
	break;
    default:
	return NULL;
    }

    for (; prg; prg = prg->next)
    {
	if (prg->vnum == vnum)
	    return(prg);
    }
    return NULL;
}

/*
 * Translates room virtual number to its room index struct.
 * Hash table lookup.
 */
ROOM_INDEX_DATA *get_room_index(int vnum)
{
    ROOM_INDEX_DATA *pRoomIndex;

    if (!CHECK_VNUM(vnum))
	return NULL;

    for (pRoomIndex  = room_index_hash[vnum % MAX_KEY_HASH];
	 pRoomIndex != NULL;
	 pRoomIndex  = pRoomIndex->next)
    {
	if (pRoomIndex->vnum == vnum)
	    return pRoomIndex;
    }

    if (fBootDb)
    {
	bugf("Get_room_index: bad vnum %d.", vnum);
	exit(1);
    }

    return NULL;
}

HELP_DATA *get_help(char *keyword)
{
    HELP_DATA *pHelp;
    char argall[MAX_STRING_LENGTH], argone[MAX_STRING_LENGTH];
    int num, i = 0;

    num = number_argument(keyword, argone);
    keyword = argone;

    argall[0] = '\0';
    while (keyword[0] != '\0')
    {
	keyword = one_argument(keyword, argone);
	if (argall[0] != '\0')
	    strcat(argall, " ");

	strcat(argall, argone);
    }

    STAILQ_FOREACH(pHelp, &help_first, link)
    {
	if (is_name(argall, pHelp->keyword))
	    if (++i == num)
		break;
    }

    return pHelp;
}

HELP_AREA *get_help_area(char *filename)
{
    HELP_AREA *pArea;

    for (pArea = had_list; pArea; pArea = pArea->next)
	if (!str_cmp(pArea->filename, filename))
	    break;

    return pArea;
}

/*
 * Read a letter from a file.
 */
char fread_letter(FILE *fp)
{
    char c;

    do
    {
	c = getc(fp);
    }
    while (IS_SPACE(c));

    return c;
}



/*
 * Read a number from a file.
 */
int64_t fread_number(FILE *fp)
{
    int64_t number;
    bool sign;
    char c;

    do
    {
	c = getc(fp);
    }
    while (IS_SPACE(c));

    number = 0;

    sign   = FALSE;
    if (c == '+')
    {
	c = getc(fp);
    }
    else if (c == '-')
    {
	sign = TRUE;
	c = getc(fp);
    }

    if (!isdigit(c))
    {
	bugf("Fread_number: bad format.");
	safe_exit(1);
    }

    while (isdigit(c))
    {
	number = number * 10 + c - '0';
	c      = getc(fp);
    }

    if (sign)
	number = 0 - number;

    if (c == '|')
	number += fread_number(fp);
    else if (c != ' ')
	ungetc(c, fp);

    return number;
}

int64_t fread_flag(FILE *fp)
{
    int64_t number;
    char c;
    bool negative = FALSE;

    do
    {
	c = getc(fp);
    }
    while (IS_SPACE(c));

    if (c == '-')
    {
	negative = TRUE;
	c = getc(fp);
    }

    number = 0;

    if (!isdigit(c) && c != '-')  /* ROM OLC */
    {
	while (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'))
	{
	    number += flag_convert(c);
	    c = getc(fp);
	}
    }

    if (c == '-')      /* ROM OLC */
    {
	number = fread_number(fp);
	return -number;
    }



    while (isdigit(c))
    {
	number = number * 10 + c - '0';
	c = getc(fp);
    }

    if (c == '|')
	number += fread_flag(fp);

    else if  (c != ' ')
	ungetc(c, fp);

    if (negative)
	return -1 * number;

    return number;
}

int64_t flag_convert(char letter)
{
    int64_t bitsum = 0;
    char i;

    if ('A' <= letter && letter <= 'Z')
    {
	bitsum = A;
	for (i = letter; i > 'A'; i--)
	    bitsum <<= 1;
    }
    else if ('a' <= letter && letter <= 'z')
    {
	bitsum = aa; /* 2^26 */
	for (i = letter; i > 'a'; i --)
	    bitsum <<= 1;
    }

    return bitsum;
}




/*
 * Read and allocate space for a string from a file.
 * These strings are read-only and shared.
 * Strings are hashed:
 *   each string prepended with hash pointer to prev string,
 *   hash code is simply the string length.
 *   this function takes 40% to 50% of boot-up time.
 */
#ifdef MEMDEBUG
char *fread_string(FILE *fp)
{
    char str[65535];
    char c;
    char *s;
    int  i = 1;

    do
    {
	c = getc(fp);
    }
    while (IS_SPACE(c));

    memset(str, '\0', sizeof(str));
    s = str;

    while (c != '~')
    {
	if (feof(fp))
	    return str_dup(str);
	if (i++ >= sizeof(str))
	    return str_dup(str);
	*s++ = c;

	c = getc(fp);
    }

    return str_dup(str);
}
#else
char *fread_string(FILE *fp)
{
    char *plast;
    char c;

    plast = top_string + sizeof(char *);

    if (plast > &string_space[MAX_STRING - MAX_STRING_LENGTH])
    {
	bugf("Fread_string: MAX_STRING %d exceeded.", MAX_STRING);
	safe_exit(1);
    }

    /*
     * Skip blanks.
     * Read first char.
     */
    do
    {
	c = getc(fp);
    }
    while (IS_SPACE(c));

    if ((*plast++ = c) == '~')
	return &str_empty[0];

    for (;;)
    {
	/*
	 * Back off the char type lookup,
	 *   it was too dirty for portability.
	 *   -- Furey
	 */

	switch (*plast = getc(fp))
	{
	default:
	    plast++;
	    break;

	case EOF:
	    /* temp fix */
	    if (feof(fp))
	    {
		bugf("Fread_string: EOF");
		return NULL;
		/* exit(1); */
	    }
	    else plast++;
	    break;

	case '\n':
	    plast++;
	    *plast++ = '\r';
	    break;

	case '\r':
	    break;

	case '~':
	    plast++;
	    {
		union
		{
		    char *	pc;
		    char	rgc[sizeof(char *)];
		} u1;
		int ic;
		int iHash;
		char *pHash;
		char *pHashPrev;
		char *pString;

		plast[-1] = '\0';
		iHash     = UMIN(MAX_KEY_HASH - 1, plast - 1 - top_string);
		for (pHash = string_hash[iHash]; pHash; pHash = pHashPrev)
		{
		    for (ic = 0; ic < sizeof(char *); ic++)
			u1.rgc[ic] = pHash[ic];
		    pHashPrev = u1.pc;
		    pHash    += sizeof(char *);

		    if (top_string[sizeof(char *)] == pHash[0]
			&&   !strcmp(top_string+sizeof(char *)+1, pHash+1))
			return pHash;
		}

		if (fBootDb)
		{
		    pString		= top_string;
		    top_string		= plast;
		    u1.pc		= string_hash[iHash];
		    for (ic = 0; ic < sizeof(char *); ic++)
			pString[ic] = u1.rgc[ic];
		    string_hash[iHash]	= pString;

		    nAllocString += 1;
		    sAllocString += top_string - pString;
		    return pString + sizeof(char *);
		}
		else
		{
		    return str_dup(top_string + sizeof(char *));
		}
	    }
	}
    }
}
#endif

#ifdef MEMDEBUG
char *fread_string_eol(FILE *fp)
{
    char str[65535];
    char c;
    char *s;
    int  i = 1;

    do
    {
	c = getc(fp);
    }
    while (IS_SPACE(c));

    memset(str, '\0', sizeof(str));
    s = str;

    while (c != '\n' && c != '\r')
    {
	if (feof(fp))
	    return str_dup(str);
	if (i++ >= sizeof(str))
	    return str_dup(str);
	*s++ = c;

	c = getc(fp);
    }

    return str_dup(str);
}
#else
char *fread_string_eol(FILE *fp)
{
    static bool char_special[256-EOF];
    char *plast;
    char c;

    if (char_special[EOF-EOF] != TRUE)
    {
	char_special[EOF -  EOF] = TRUE;
	char_special['\n' - EOF] = TRUE;
	char_special['\r' - EOF] = TRUE;
    }

    plast = top_string + sizeof(char *);
    if (plast > &string_space[MAX_STRING - MAX_STRING_LENGTH])
    {
	bugf("Fread_string: MAX_STRING %d exceeded.", MAX_STRING);
	safe_exit(1);
    }

    /*
     * Skip blanks.
     * Read first char.
     */
    do
    {
	c = getc(fp);
    }
    while (IS_SPACE(c));

    if ((*plast++ = c) == '\n')
	return &str_empty[0];

    for (;;)
    {
	if (!char_special[ (*plast++ = getc(fp)) - EOF ])
	    continue;

	if (feof(fp))
	    return &str_empty[0];

	switch (plast[-1])
	{
	default:
	    break;

	case '\n':  case '\r':
	    {
		union
		{
		    char *      pc;
		    char        rgc[sizeof(char *)];
		} u1;
		int ic;
		int iHash;
		char *pHash;
		char *pHashPrev;
		char *pString;

		plast[-1] = '\0';
		iHash     = UMIN(MAX_KEY_HASH - 1, plast - 1 - top_string);
		for (pHash = string_hash[iHash]; pHash; pHash = pHashPrev)
		{
		    for (ic = 0; ic < sizeof(char *); ic++)
			u1.rgc[ic] = pHash[ic];
		    pHashPrev = u1.pc;
		    pHash    += sizeof(char *);

		    if (top_string[sizeof(char *)] == pHash[0]
			&&   !strcmp(top_string+sizeof(char *)+1, pHash+1))
			return pHash;
		}

		if (fBootDb)
		{
		    pString             = top_string;
		    top_string          = plast;
		    u1.pc               = string_hash[iHash];
		    for (ic = 0; ic < sizeof(char *); ic++)
			pString[ic] = u1.rgc[ic];
		    string_hash[iHash]  = pString;

		    nAllocString += 1;
		    sAllocString += top_string - pString;
		    return pString + sizeof(char *);
		}
		else
		{
		    return str_dup(top_string + sizeof(char *));
		}
	    }
	}
    }
}
#endif


/*
 * Read to end of line (for comments).
 */
void fread_to_eol(FILE *fp)
{
    char c;

    do
    {
	c = getc(fp);
    }
    while (c != '\n' && c != '\r');

    do
    {
	c = getc(fp);
    }
    while (c == '\n' || c == '\r');

    ungetc(c, fp);
    return;
}



/*
 * Read one word (into static buffer).
 */
char *fread_word(FILE *fp)
{
    static char word[MAX_INPUT_LENGTH];
    char *pword;
    char cEnd;

    do
    {
	cEnd = getc(fp);
    }
    while (IS_SPACE(cEnd));

    if (cEnd == '\'' || cEnd == '"')
    {
	pword   = word;
    }
    else
    {
	word[0] = cEnd;
	pword   = word + 1;
	cEnd    = ' ';
    }

    for (; pword < word + MAX_INPUT_LENGTH-1; pword++)
    {
	*pword = getc(fp);
	if (cEnd == ' ' ? IS_SPACE(*pword) : *pword == cEnd)
	{
	    if (cEnd == ' ')
		ungetc(*pword, fp);
	    *pword = '\0';
	    return word;
	}
    }
    *pword = '\0';
    return word;
}

/*
 * Allocate some ordinary memory,
 *   with the expectation of freeing it someday.
 */
void *alloc_mem(unsigned int sMem)
{
#ifdef MEMDEBUG
    return calloc(sMem, 1);
#else
    void *pMem;
    int *magic;
    int iList;

    sMem += sizeof(*magic);

    for (iList = 0; iList < MAX_MEM_LIST; iList++)
    {
	if (sMem <= rgSizeList[iList])
	    break;
    }

    if (iList == MAX_MEM_LIST)
    {
	bugf("Alloc_mem: size %u too large.", sMem);
	safe_exit(1);
    }

    if (rgFreeList[iList] == NULL)
    {
	pMem              = alloc_perm(rgSizeList[iList]);
    }
    else
    {
	pMem              = rgFreeList[iList];
	rgFreeList[iList] = * ((void **) rgFreeList[iList]);
    }

    magic = (int *) pMem;
    *magic = MAGIC_NUM;
    pMem += sizeof(*magic);

    return pMem;
#endif
}



/*
 * Free some memory.
 * Recycle it back onto the free list for blocks of that size.
 */
void free_mem(void *pMem, int sMem)
{
#ifdef MEMDEBUG
    free(pMem);
    return;
#else
    int iList;
    int *magic;

    pMem -= sizeof(*magic);
    magic = (int *)pMem;

    if (*magic != MAGIC_NUM)
    {
	bugf("Attempt to recyle invalid memory of size %d (0x%x).", sMem, pMem);
	bugf("%s", (char*) pMem + sizeof(*magic));
	return;
    }

    *magic = 0;
    sMem += sizeof(*magic);

    for (iList = 0; iList < MAX_MEM_LIST; iList++)
    {
	if (sMem <= rgSizeList[iList])
	    break;
    }

    if (iList == MAX_MEM_LIST)
    {
	bugf("Free_mem: size %d too large.", sMem);
	safe_exit(1);
    }

    *((void **)pMem) = rgFreeList[iList];
    rgFreeList[iList]  = pMem;

    return;
#endif
}


/*
 * Allocate some permanent memory.
 * Permanent memory is never freed,
 *   pointers into it may be copied safely.
 */
void *alloc_perm(int sMem)
{
#ifdef MEMDEBUG
    return calloc(sMem, 1);
#else
    static char *pMemPerm;
    static int iMemPerm;
    void *pMem;

    while (sMem % sizeof(long) != 0)
	sMem++;

    if (sMem > MAX_PERM_BLOCK)
    {
	bugf("Alloc_perm: %d too large.", sMem);
	safe_exit(1);
    }

    if (pMemPerm == NULL || iMemPerm + sMem > MAX_PERM_BLOCK)
    {
	iMemPerm = 0;
	if ((pMemPerm = calloc(1, MAX_PERM_BLOCK)) == NULL)
	{
	    _perror("Alloc_perm");
	    safe_exit(1);
	}
    }

    pMem        = pMemPerm + iMemPerm;
    iMemPerm   += sMem;
    nAllocPerm += 1;
    sAllocPerm += sMem;
    return pMem;
#endif
}



/*
 * Duplicate a string into dynamic memory.
 * Fread_strings are read-only and shared.
 */
char *str_dup(const char *str)
{
#ifdef MEMDEBUG
    if (str == NULL)
	return NULL;
    else
	return strdup(str);
#else
    char *str_new;

    if (str == NULL)
	return NULL;
    
    if (str[0] == '\0')
	return &str_empty[0];

    if (str >= string_space && str < top_string)
	return (char *) str;

    str_new = alloc_mem(strlen(str) + 1);
    strcpy(str_new, str);

    return str_new;
#endif
}



/*
 * Free a string.
 * Null is legal here to simplify callers.
 * Read-only shared strings are not touched.
 */
void free_string(char *pstr)
{
#ifdef MEMDEBUG
    if (pstr != NULL)
	free(pstr);

    return;
#else
    if (!pstr
	|| pstr == &str_empty[0]
	|| (pstr >= string_space && pstr < top_string))
    {
	return;
    }

    free_mem(pstr, strlen(pstr) + 1);

    return;
#endif
}

void do_areas(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    AREA_DATA *pArea1;
    int counter = 0;
    BUFFER *output;
    int level = 0;
    bool islevel = TRUE;

    if (!IS_NULLSTR(argument))
    {
	char *arg;

	if (is_number(argument))
	    level = atoi(argument);
	else
	{
	    islevel = FALSE;

	    for(arg = argument; *arg != '\0'; arg++)
		*arg = UPPER(*arg);
	}

	if (islevel && (level < 1 || level > LEVEL_HERO))
	{
	    send_to_char("Уровень должен быть в разумных пределах.\n\r", ch);
	    return;
	}
    }

    output = new_buf();

    for (pArea1 = area_first; pArea1 != NULL; pArea1 = pArea1->next)
    {
	int level1, level2;

	if (IS_NULLSTR(pArea1->credits)
	    || IS_SET(pArea1->area_flags, AREA_NA)	 /* Don't show n/a and testing areas */
	    || IS_SET(pArea1->area_flags, AREA_TESTING)) /* to mortals.                      */
	{
	    continue;
	}

	level1=level2=-1;

	if (!IS_NULLSTR(argument))
	{
	    int tmp;

	    if (islevel)
	    {
		buf[0] = *(pArea1->credits + 1);
		buf[1] = '\0';

		if (!is_number(buf)) tmp=0; else tmp=1;
		buf[tmp] = *(pArea1->credits + 2);
		buf[++tmp] = '\0';

		if (is_number(buf))
		{
		    level1 = atoi(buf);
		    buf[0] = *(pArea1->credits + 4);
		    buf[1] = '\0';
		    if (!is_number(buf)) tmp=0; else tmp=1;
		    buf[tmp] = *(pArea1->credits + 5);
		    buf[++tmp] = '\0';
		    if (is_number(buf))
			level2 = atoi(buf);
		}
	    }
	}

	if ((islevel && (level1 == -1 || level2 == -1 || (level >= level1 && level <= level2)))
	    || (!islevel && !str_infix(argument, pArea1->credits)))
	{
	    sprintf(buf, "%-39s", pArea1->credits);
	    add_buf(output, buf);
	    counter++;
	    if ((counter & 1) == 0)
		add_buf(output, "\n\r");
	}
    }

    sprintf(buf, "%s\n\r%s зон: %d\n\r", (counter & 1) == 1 ? "\n\r" : "",
	    argument[0] != '\0' ? "Найдено" : "Всего", counter);

    add_buf(output, buf);
    page_to_char(buf_string(output), ch);
    free_buf(output);
    return;
}



void do_memory(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    sprintf(buf, "Affects %5d\n\r", top_affect   ); send_to_char(buf, ch);
    sprintf(buf, "Areas   %5d\n\r", top_area     ); send_to_char(buf, ch);
    sprintf(buf, "ExDes   %5d\n\r", top_ed       ); send_to_char(buf, ch);
    sprintf(buf, "Exits   %5d\n\r", top_exit     ); send_to_char(buf, ch);
    sprintf(buf, "Helps   %5d\n\r", top_help     ); send_to_char(buf, ch);
    sprintf(buf, "Socials %5d\n\r", social_count ); send_to_char(buf, ch);
    sprintf(buf, "Mobs    %5d (%d new format)\n\r", top_mob_index, newmobs);
    send_to_char(buf, ch);
    sprintf(buf, "(in use)%5d\n\r", mobile_count ); send_to_char(buf, ch);
    sprintf(buf, "Objs    %5d (%d new format)\n\r", top_obj_index, newobjs);
    send_to_char(buf, ch);
    sprintf(buf, "(in use)%5d\n\r", obj_count    ); send_to_char(buf, ch);
    sprintf(buf, "Resets  %5d\n\r", top_reset    ); send_to_char(buf, ch);
    sprintf(buf, "Rooms   %5d\n\r", top_room     ); send_to_char(buf, ch);
    sprintf(buf, "Shops   %5d\n\r", top_shop     ); send_to_char(buf, ch);
    sprintf(buf, "MProgs  %5d\n\r", top_mprog_index); send_to_char(buf, ch);
    sprintf(buf, "OProgs  %5d\n\r", top_oprog_index); send_to_char(buf, ch);
    sprintf(buf, "RProgs  %5d\n\r", top_rprog_index); send_to_char(buf, ch);

    sprintf(buf, "Strings %5d strings of %7d bytes (max %d).\n\r",
	    nAllocString, sAllocString, MAX_STRING);
    send_to_char(buf, ch);

    sprintf(buf, "Perms   %5d blocks  of %7d bytes.\n\r",
	    nAllocPerm, sAllocPerm);
    send_to_char(buf, ch);

    return;
}

void do_dump(CHAR_DATA *ch, char *argument)
{
    int count, count2, num_pcs, aff_count;
    CHAR_DATA *fch;
    MOB_INDEX_DATA *pMobIndex;
    PC_DATA *pc;
    OBJ_DATA *obj;
    OBJ_INDEX_DATA *pObjIndex;
    ROOM_INDEX_DATA *room;
    EXIT_DATA *exit;
    DESCRIPTOR_DATA *d;
    AFFECT_DATA *af;
    FILE *fp;
    int vnum, nMatch = 0;

    /* open file */
    if ((fp = file_open("mem.dmp", "w")) != NULL)
    {

	/* report use of data structures */

	num_pcs = 0;
	aff_count = 0;

	/* mobile prototypes */
	fprintf(fp, "MobProt	%4d (%8ld bytes)\n", top_mob_index, top_mob_index * (sizeof(*pMobIndex)));

	/* mobs */
	count = 0;  count2 = 0;
	LIST_FOREACH(fch, &char_list, link)
	{
	    count++;
	    if (fch->pcdata != NULL)
		num_pcs++;
	    for (af = fch->affected; af != NULL; af = af->next)
		aff_count++;
	}


	LIST_FOREACH(fch, &char_free, link)
	    count2++;

	fprintf(fp, "Mobs	%4d (%8ld bytes), %2d free (%ld bytes)\n",
		count, count * (sizeof(*fch)), count2, count2 * (sizeof(*fch)));

	/* pcdata */
	count = 0;
	for (pc = pcdata_free; pc != NULL; pc = pc->next)
	    count++;

	fprintf(fp, "Pcdata	%4d (%8ld bytes), %2d free (%ld bytes)\n",
		num_pcs, num_pcs * (sizeof(*pc)), count, count * (sizeof(*pc)));

	/* descriptors */
	count = 0; count2 = 0;
	SLIST_FOREACH(d, &descriptor_list, link)
	    count++;
	SLIST_FOREACH(d, &descriptor_free, link)
	    count2++;

	fprintf(fp, "Descs	%4d (%8ld bytes), %2d free (%ld bytes)\n",
		count, count * (sizeof(*d)), count2, count2 * (sizeof(*d)));

	/* object prototypes */
	for (vnum = 0; nMatch < top_obj_index; vnum++)
	    if ((pObjIndex = get_obj_index(vnum)) != NULL)
	    {
		for (af = pObjIndex->affected; af != NULL; af = af->next)
		    aff_count++;
		nMatch++;
	    }
	fprintf(fp, "ObjProt	%4d (%8ld bytes)\n",
		top_obj_index, top_obj_index * (sizeof(*pObjIndex)));


	/* objects */
	count = 0;  count2 = 0;
	for (obj = object_list; obj != NULL; obj = obj->next)
	{
	    count++;
	    for (af = obj->affected; af != NULL; af = af->next)
		aff_count++;
	}
	for (obj = obj_free; obj != NULL; obj = obj->next)
	    count2++;

	fprintf(fp, "Objs	%4d (%8ld bytes), %2d free (%ld bytes)\n",
		count, count * (sizeof(*obj)), count2, count2 * (sizeof(*obj)));

	/* affects */
	count = 0;
	for (af = affect_free; af != NULL; af = af->next)
	    count++;

	fprintf(fp, "Affects	%4d (%8ld bytes), %2d free (%ld bytes)\n",
		aff_count, aff_count * (sizeof(*af)), count, count * (sizeof(*af)));

	/* rooms */
	fprintf(fp, "Rooms	%4d (%8ld bytes)\n",
		top_room, top_room * (sizeof(*room)));

	/* exits */
	fprintf(fp, "Exits	%4d (%8ld bytes)\n",
		top_exit, top_exit * (sizeof(*exit)));
    }
    file_close(fp);

    /* start printing out mobile data */
    if ((fp = file_open("mob.dmp", "w")) != NULL)
    {

	fprintf(fp, "\nMobile Analysis\n");
	fprintf(fp,  "---------------\n");
	nMatch = 0;
	for (vnum = 0; nMatch < top_mob_index; vnum++)
	    if ((pMobIndex = get_mob_index(vnum)) != NULL)
	    {
		nMatch++;
		fprintf(fp, "#%-4d %3d active %3d killed     %s\n",
			pMobIndex->vnum, pMobIndex->count,
			pMobIndex->killed, pMobIndex->short_descr);
	    }
    }
    file_close(fp);

    /* start printing out object data */
    if ((fp = file_open("obj.dmp", "w")) != NULL)
    {
	fprintf(fp, "\nObject Analysis\n");
	fprintf(fp,  "---------------\n");
	nMatch = 0;
	for (vnum = 0; nMatch < top_obj_index; vnum++)
	    if ((pObjIndex = get_obj_index(vnum)) != NULL)
	    {
		nMatch++;
		fprintf(fp, "#%-4d %3d active %3d reset      %s\n",
			pObjIndex->vnum, pObjIndex->count,
			pObjIndex->reset_num, pObjIndex->short_descr);
	    }
    }
    file_close(fp);
}



/*
 * Stick a little fuzz on a number.
 */
int number_fuzzy(int number)
{
    switch (number_bits(2))
    {
    case 0:  number -= 1; break;
    case 3:  number += 1; break;
    }

    return UMAX(1, number);
}



/*
 * Generate a random number.
 */
int number_range(int from, int to)
{
    int power;
    int number;

    if (from == 0 && to == 0)
	return 0;

    if ((to = to - from + 1) <= 1)
	return from;

    for (power = 2; power < to; power <<= 1)
	;

    while ((number = number_mm() & (power -1)) >= to)
	;

    return from + number;
}



/*
 * Generate a percentile roll.
 */
int number_percent(void)
{
    int percent;

    while ((percent = number_mm() & (128-1)) > 99)
	;

    return 1 + percent;
}



/*
 * Generate a random door.
 */
int number_door(void)
{
    int door;

    while ((door = number_mm() & (8-1)) >= MAX_DIR)
	;

    return door;
}

int number_bits(int width)
{
    return number_mm() & ((1 << width) - 1);
}




/*
 * I've gotten too many bad reports on OS-supplied random number generators.
 * This is the Mitchell-Moore algorithm from Knuth Volume II.
 * Best to leave the constants alone unless you've read Knuth.
 * -- Furey
 */

/* I noticed streaking with this random number generator, so I switched
 back to the system srandom call.  If this doesn't work for you,
 define OLD_RAND to use the old system -- Alander */

#if defined (OLD_RAND)
static  int     rgiState[2+55];
#endif

void init_mm()
{
#if defined (OLD_RAND)
    int *piState;
    int iState;

    piState     = &rgiState[2];

    piState[-2] = 55 - 55;
    piState[-1] = 55 - 24;

    piState[0]  = ((int) current_time) & ((1 << 30) - 1);
    piState[1]  = 1;
    for (iState = 2; iState < 55; iState++)
    {
	piState[iState] = (piState[iState-1] + piState[iState-2])
	    & ((1 << 30) - 1);
    }
#else
    srandom(time(NULL)^getpid());
#endif
    return;
}



long number_mm(void)
{
#if defined (OLD_RAND)
    int *piState;
    int iState1;
    int iState2;
    int iRand;

    piState             = &rgiState[2];
    iState1             = piState[-2];
    iState2             = piState[-1];
    iRand               = (piState[iState1] + piState[iState2])
	& ((1 << 30) - 1);
    piState[iState1]    = iRand;
    if (++iState1 == 55)
	iState1 = 0;
    if (++iState2 == 55)
	iState2 = 0;
    piState[-2]         = iState1;
    piState[-1]         = iState2;
    return iRand >> 6;
#else
    return random(); // >> 6;
#endif
}


/*
 * Roll some dice.
 */
int dice(int number, int size)
{
    int idice;
    int sum;

    switch (size)
    {
    case 0: return 0;
    case 1: return number;
    }

    for (idice = 0, sum = 0; idice < number; idice++)
	sum += number_range(1, size);

    return sum;
}



/*
 * Simple linear interpolation.
 */
int interpolate(int level, int value_00, int value_32)
{
    return value_00 + level * (value_32 - value_00) / 32;
}



/*
 * Removes the tildes from a string.
 * Used for player-entered strings that go into disk files.
 */
void smash_tilde(char *str)
{
    if (str)
	for (; *str != '\0'; str++)
	    if (*str == '~')
		*str = '-';

    return;
}


#define CHECK_E_YO(a, b)	((LOWER(a) == 'е' && LOWER(b) == 'ё') || (LOWER(a) == 'ё' && LOWER(b) == 'е'))


/*
 * Проверка что строки разные:
 * Для оличающихся строк вернет true
 * Если строки идентичны вернет false
 */
bool str_cmp(const char *astr, const char *bstr){
    if (astr == NULL || bstr == NULL)
	return TRUE;

    for (; *astr || *bstr; astr++, bstr++){
		if ((*astr == '\0' && *bstr != '\0') || (*astr != '\0' && *bstr == '\0')){
			return TRUE;
		}

		if (LOWER(*astr) != LOWER(*bstr) && !CHECK_E_YO(*astr, *bstr))
			return TRUE;
    }

    return FALSE;
}



/*
 * Compare strings, case insensitive, for prefix matching.
 * Return TRUE if astr not a prefix of bstr
 *   (compatibility with historical functions).
 */
bool str_prefix(const char *astr, const char *bstr)
{
    if (astr == NULL || bstr == NULL)
	return TRUE;

    for (; *astr; astr++, bstr++)
    {
	if (LOWER(*astr) != LOWER(*bstr) && !CHECK_E_YO(*astr, *bstr))
	    return TRUE;
    }

    return FALSE;
}



/*
 * Compare strings, case insensitive, for match anywhere.
 * Returns TRUE is astr not part of bstr.
 *   (compatibility with historical functions).
 */
bool str_infix(const char *astr, const char *bstr)
{
    int sstr1;
    int sstr2;
    int ichar;
    char c0;

    if ((c0 = astr[0]) == '\0')
	return FALSE;

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);

    for (ichar = 0; ichar <= sstr2 - sstr1; ichar++)
    {
	if (!str_prefix(astr, bstr + ichar))
	    return FALSE;
    }

    return TRUE;
}

char *str_str(const char *bstr, const char *astr)
{
    int sstr1;
    int sstr2;
    int ichar;
    char c0;

    if ((c0 = astr[0]) == '\0')
	return (char *) bstr;

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);

    for (ichar = 0; ichar <= sstr2 - sstr1; ichar++)
    {
	if (!str_prefix(astr, bstr + ichar))
	    return (char *) bstr + ichar;
    }

    return NULL;
}



/*
 * Compare strings, case insensitive, for suffix matching.
 * Return TRUE if astr not a suffix of bstr
 *   (compatibility with historical functions).
 */
bool str_suffix(const char *astr, const char *bstr)
{
    int sstr1;
    int sstr2;

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);
    if (sstr1 <= sstr2 && !str_cmp(astr, bstr + sstr2 - sstr1))
	return FALSE;
    else
	return TRUE;
}



/*
 * Returns an initial-capped string.
 */
char *capitalize(const char *str)
{
    static char strcap[MAX_STRING_LENGTH];
    int i;

    for (i = 0; str[i] != '\0'; i++)
	strcap[i] = LOWER(str[i]);
    strcap[i] = '\0';
    strcap[0] = UPPER(strcap[0]);
    return strcap;
}


/*
 * Append a string to a file.
 */
void append_file(CHAR_DATA *ch, char *file, char *str)
{
    FILE *fp;

    if (IS_NPC(ch) || str[0] == '\0')
	return;

    if ((fp = file_open(file, "a")) == NULL)
	_perror(file);
    else
    {
	char tmp[50];

	strftime(tmp, 50, "%d.%m %H:%M", localtime(&current_time));
	fprintf(fp, "[%s] [%d] %s: %s\n", tmp, ch->in_room ? ch->in_room->vnum : 0, ch->name, str);
    }
    file_close(fp);
    return;
}



void convert_dollars(char *s)
{
    char *p;

    for(p = s; *p != '\0'; p++)
	if (*p == '$')
	    *p = 'S';
}
/*
 * Reports a bug.
 */
void bugf_internal(char *format, ...)
{
    va_list va;
    char buf[MAX_STRING_LENGTH];

    if (fpArea != NULL)
    {
	int iLine;
	int iChar;

	if (fpArea == stdin)
	{
	    iLine = 0;
	}
	else
	{
	    iChar = ftell(fpArea);
	    fseek(fpArea, 0, 0);
	    for (iLine = 0; ftell(fpArea) < iChar; iLine++)
	    {
		while (getc(fpArea) != '\n')
		    ;
	    }
	    fseek(fpArea, iChar, 0);
	}

	snprintf(buf, MAX_STRING_LENGTH,
		 "[*****] FILE: %s LINE: %d", strArea, iLine);
	log_string(buf);
    }

    strlcpy(buf, "[*****] BUG: ", MAX_STRING_LENGTH);

    va_start(va, format);
    vsnprintf(buf + strlen(buf), MAX_STRING_LENGTH, format, va);
    va_end(va);

    log_string(buf);
    convert_dollars(buf);
    wiznet(buf, NULL, NULL, WIZ_BUGS, 0, 0);

    return;
}

/*
 * Writes a string to the log.
 */
void _log_string(const char *str, char *logname)
{
    FILE *fp;

    if (free_space() < MIN_FREE_SPACE)
	return;

    if ((fp = file_open(logname, "a")) != NULL)
	fprintf(fp, "%s", str);

    file_close(fp);

    return;
}


void log_string(const char *str)
{
    char buf[MAX_STRING_LENGTH], logname[40], tmp[20], strtime[20];

    strftime(tmp, 20, "%Y.%m.%d", localtime(&current_time));
    sprintf(logname, "%s%s.log", LOG_DIR, tmp);

    strftime(strtime, 10, "%T", localtime(&current_time));

    snprintf(buf, MSL, "%s :: %s\n", strtime, str);

    _log_string(buf, logname);

    snprintf(buf, MSL, "[LOG] %s", str);
    convert_dollars(buf);
    wiznet(buf, NULL, NULL, WIZ_LOG, 0, 0);
}

void _perror(const char *str)
{
    extern int errno;
    char buf[MAX_STRING_LENGTH];

    snprintf(buf, MSL, "%s: %s", str, strerror(errno));
    log_string(buf);
}


/*
 * This function is here to aid in debugging.
 * If the last expression in a function is another function call,
 *   gcc likes to generate a JMP instead of a CALL.
 * This is called "tail chaining."
 * It hoses the debugger call stack for that call.
 * So I make this the last call in certain critical functions,
 *   where I really need the call stack to be right for debugging!
 *
 * If you don't understand this, then LEAVE IT ALONE.
 * Don't remove any calls to tail_chain anywhere.
 *
 * -- Furey
 */
void tail_chain(void)
{
    return;
}


/*
 * Snarf a reset section.       Changed for OLC.
 */
void load_resets(FILE *fp)
{
    RESET_DATA  *pReset;
    int         iLastRoom = 0;
    int         iLastObj  = 0;


    if (!area_last)
    {
	bugf("Load_resets: no #AREA seen yet.");
	exit(1);
    }

    for (; ;)
    {
	EXIT_DATA       *pexit;
	ROOM_INDEX_DATA *pRoomIndex;
	char             letter;

	if ((letter = fread_letter(fp)) == 'S')
	    break;

	if (letter == '*') /* comment - skip to new line */
	{
	    fread_to_eol(fp);
	    continue;
	}

	pReset          = alloc_perm(sizeof(*pReset));
	pReset->command = letter;
	/* if_flag */     fread_number(fp);
	pReset->arg1    = fread_number(fp);
	pReset->arg2    = fread_number(fp);
	pReset->arg3    = (letter == 'G' || letter == 'R' || letter == 'T')
	    ? 0 : fread_number(fp);
	pReset->arg4    = (letter == 'P' || letter == 'M' || letter == 'H' || letter == 'X')
	    ? fread_number(fp) : 0;

	fread_to_eol(fp);

	/*
	 * Validate parameters.
	 * We're calling the index functions for the side effect.
	 */
	switch (letter)
	{
	default:
	    bugf("Load_resets: bad command '%c'.", letter);
	    exit(1);
	    break;

	case 'M':
	    get_mob_index(pReset->arg1);
	    if ((pRoomIndex = get_room_index(pReset->arg3)))
	    {
		new_reset(pRoomIndex, pReset);
		iLastRoom = pReset->arg3;
	    }
	    break;

	case 'H':
	    get_mob_index(pReset->arg1);
	    if ((pRoomIndex = get_room_index(iLastRoom)))
		new_reset(pRoomIndex, pReset);

	    break;

	case 'O':
	    get_obj_index(pReset->arg1);
	    if ((pRoomIndex = get_room_index(pReset->arg3)))
	    {
		new_reset(pRoomIndex, pReset);
		iLastObj = pReset->arg3;
	    }
	    break;

	case 'P':
	    get_obj_index(pReset->arg1);
	    if ((pRoomIndex = get_room_index(iLastObj)))
	    {
		new_reset(pRoomIndex, pReset);
	    }
	    break;

	case 'G':
	case 'E':
	    get_obj_index(pReset->arg1);
	    if ((pRoomIndex = get_room_index(iLastRoom)))
	    {
		new_reset(pRoomIndex, pReset);
		iLastObj = iLastRoom;
	    }
	    break;

	case 'D':
	    if ((pRoomIndex = get_room_index(pReset->arg1)) == NULL)
	    {
		bugf("Bad room!");
		exit(1);
	    }

	    if (pReset->arg2 < 0
		|| pReset->arg2 > (MAX_DIR - 1)
		|| !pRoomIndex
		|| !(pexit = pRoomIndex->exit[pReset->arg2])
		|| !IS_SET(pexit->rs_flags, EX_ISDOOR))
	    {
		bugf("Load_resets: 'D': exit %d not door.", pReset->arg2);
		exit(1);
	    }

	    switch (pReset->arg3)
	    {
	    default:
		bugf("Load_resets: 'D': bad 'locks': %d." , pReset->arg3);
		exit(1);
		break;
	    case 0:
		break;
	    case 1:
		SET_BIT(pexit->rs_flags, EX_CLOSED);
		SET_BIT(pexit->exit_info, EX_CLOSED);
		break;
	    case 2:
		SET_BIT(pexit->rs_flags, EX_CLOSED | EX_LOCKED);
		SET_BIT(pexit->exit_info, EX_CLOSED | EX_LOCKED);
		break;
	    }
	    new_reset(pRoomIndex, pReset);

	    break;

	case 'R':
	    pRoomIndex = get_room_index(pReset->arg1);

	    if (pReset->arg2 < 0 || pReset->arg2 > MAX_DIR)
	    {
		bugf("Load_resets: 'R': bad exit %d.", pReset->arg2);
		exit(1);
	    }
	    if (pRoomIndex)
		new_reset(pRoomIndex, pReset);

	    break;

	case 'T':
	    get_obj_index(pReset->arg1);
	    if ((pRoomIndex = get_room_index(pReset->arg2)) == NULL)
	    {
		bugf("Load_resets: 'T': bad room %d.", pReset->arg2);
		exit(1);
	    }

	    new_reset(pRoomIndex, pReset);
	    break;

	case 'C':
	    get_obj_index(pReset->arg1);
	    get_obj_index(pReset->arg2);
	    if ((pRoomIndex = get_room_index(iLastObj)))
	    {
		new_reset(pRoomIndex, pReset);
	    }
	    break;

	case 'X':
	    get_obj_index(pReset->arg1);
	    if ((pRoomIndex = get_room_index(pReset->arg4)) != NULL)
		new_reset(pRoomIndex, pReset);
	    break;
	}
    }

    return;
}

bool can_create_socket(OBJ_DATA *obj)
{
    return (!IS_OBJ_STAT(obj, ITEM_HAS_SOCKET) &&
	    (obj->item_type == ITEM_ARMOR || obj->item_type == ITEM_WEAPON ||
	     obj->item_type == ITEM_LIGHT || obj->item_type == ITEM_JEWELRY));
}

void create_socket(OBJ_DATA *obj)
{
    if (can_create_socket(obj) && number_bits(5) < 3)
	SET_BIT(obj->extra_flags, ITEM_HAS_SOCKET);
}


/* OLC
 * Reset one room.  Called by reset_area and olc.
 */
void reset_room(ROOM_INDEX_DATA *pRoom)
{
    RESET_DATA  *pReset;
    CHAR_DATA   *pMob;
    /*    CHAR_DATA   *mob; */
    OBJ_DATA    *pObj;
    CHAR_DATA   *LastMob = NULL;
    OBJ_DATA    *LastObj = NULL;
    int iExit;
    int level = 0;
    bool last;

    if (!pRoom)
	return;

    pMob        = NULL;
    last        = FALSE;

    for (iExit = 0;  iExit < MAX_DIR;  iExit++)
    {
	EXIT_DATA *pExit;
	if ((pExit = pRoom->exit[iExit]))
	{
	    pExit->exit_info = pExit->rs_flags;
	    if ((pExit->u1.to_room != NULL)
		&& ((pExit = pExit->u1.to_room->exit[rev_dir[iExit]])))
	    {
		/* nail the other side */
		pExit->exit_info = pExit->rs_flags;
	    }
	}
    }

    for (pReset = pRoom->reset_first; pReset != NULL; pReset = pReset->next)
    {
	MOB_INDEX_DATA  *pMobIndex;
	MOB_INDEX_DATA  *pHorsemanInd;
	CHAR_DATA 	*pHorseman;
	OBJ_INDEX_DATA  *pObjIndex;
	OBJ_INDEX_DATA  *pObjToIndex;
	ROOM_INDEX_DATA *pRoomIndex;
	CHAR_DATA *mob;
	int count, limit=0;

	switch (pReset->command)
	{
	default:
	    bugf("Reset_room [%d]: bad command %c.", pRoom->vnum, pReset->command);
	    break;

	case 'M':
	    if (!(pMobIndex = get_mob_index(pReset->arg1)))
	    {
		bugf("Reset_room [%d]: 'M': bad vnum %d.", pRoom->vnum, pReset->arg1);
		continue;
	    }

	    if (!(pRoomIndex = get_room_index(pReset->arg3)))
	    {
		bugf("Reset_room [%d]: 'M': bad room vnum %d.", pRoom->vnum, pReset->arg3);
		continue;
	    }

	    /* max world */
	    if (pMobIndex->count >= pReset->arg2)
	    {
		last = FALSE;
		break;
	    }

	    /* max room */
	    count = 0;
	    LIST_FOREACH(mob, &pRoomIndex->people, room_link)
	    {
		if (mob->pIndexData == pMobIndex)
		{
		    count++;
		    if (count >= pReset->arg4)
		    {
			last = FALSE;
			break;
		    }
		}
	    }

	    if (count >= pReset->arg4)
		break;

	    pMob = create_mobile(pMobIndex);

	    /*
	     * Some more hard coding.
	     */
	    if (room_is_dark(pRoom))
		SET_BIT(pMob->affected_by, AFF_INFRARED);
	    /*
	     * Pet shop mobiles get ACT_PET set.
	     */
	    {
		ROOM_INDEX_DATA *pRoomIndexPrev;

		pRoomIndexPrev = get_room_index(pRoom->vnum - 1);
		if (pRoomIndexPrev
		    && IS_SET(pRoomIndexPrev->room_flags, ROOM_PET_SHOP))
		{
		    SET_BIT(pMob->act, ACT_PET);
		}
	    }

	    char_to_room(pMob, pRoom, FALSE);

	    LastMob = pMob;
	    level  = URANGE(0, pMob->level - 2, LEVEL_HERO - 1); /* -1 ROM */
	    last = TRUE;
	    break;

	case 'H':
	    if (!(pHorsemanInd = get_mob_index(pReset->arg1)))
	    {
		bugf("Reset_room [%d]: 'H' 1 : bad vnum %d", pRoom->vnum, pReset->arg1);
		bugf("%d %d %d %d", pReset->arg1, pReset->arg2,
		     pReset->arg3, pReset->arg4);
		continue;
	    }

	    if (!last)
		break;

	    if (!LastMob)
	    {
		bugf("Reset_room [%d]: 'H': null mob(horse) for vnum %d",
		     pRoom->vnum, pReset->arg1);
		last = FALSE;
		break;
	    }

	    if (!IS_SET(LastMob->act, ACT_MOUNT))
	    {
		bugf("Reset_room [%d]: 'H': mob isn't a horse for vnum %d",
		     pRoom->vnum, pReset->arg1);
		last = FALSE;
		break;
	    }

	    pHorseman = create_mobile(pHorsemanInd);

	    if (room_is_dark(pRoom))
		SET_BIT(pHorseman->affected_by, AFF_INFRARED);

	    char_to_room(pHorseman, pRoom, FALSE);

	    pHorseman->mount = LastMob;
	    pHorseman->riding = TRUE;
	    LastMob->mount = pHorseman;
	    LastMob->riding = TRUE;
	    add_follower(LastMob, pHorseman);
	    LastMob->leader = pHorseman;

	    LastMob = pHorseman;
	    last = TRUE;
	    break;

	case 'O':
	    if (!(pObjIndex = get_obj_index(pReset->arg1)))
	    {
		bugf("Reset_room [%d]: 'O' 1 : bad vnum %d", pRoom->vnum, pReset->arg1);
		bugf("%d %d %d %d", pReset->arg1, pReset->arg2,
		     pReset->arg3, pReset->arg4);
		continue;
	    }

	    if (!(pRoomIndex = get_room_index(pReset->arg3)))
	    {
		bugf("Reset_room [%d]: 'O' 2 : bad vnum %d.", pRoom->vnum, pReset->arg3);
		bugf("%d %d %d %d", pReset->arg1, pReset->arg2,
		     pReset->arg3, pReset->arg4);
		continue;
	    }

	    if (pRoom->area->nplayer > 0
		|| count_obj_list(pObjIndex, pRoom->contents) > 0)
	    {
		last = FALSE;
		break;
	    }

	    pObj = create_object(pObjIndex,              /* UMIN - ROM OLC */
				 UMIN(number_fuzzy(level), LEVEL_HERO -1));

	    if (check_reset_for_limit(pObj, FALSE))
	    {
		extract_obj(pObj, FALSE, FALSE);
		break;
	    }

	    if (pObj->item_type == ITEM_CHEST)
	    {
		bugf("Reset_room [%d]: bad OBJ vnum %d.", pRoom->vnum, pReset->arg1);
		extract_obj(pObj, FALSE, FALSE);
		continue;
	    }

	    //            pObj->cost = 0;
	    obj_to_room(pObj, pRoom);
	    last = TRUE;
	    break;

	case 'P':
	    if (!(pObjIndex = get_obj_index(pReset->arg1)))
	    {
		bugf("Reset_room [%d]: 'P': bad vnum %d.", pRoom->vnum, pReset->arg1);
		continue;
	    }

	    if (!(pObjToIndex = get_obj_index(pReset->arg3)))
	    {
		bugf("Reset_room [%d]: 'P': bad vnum %d.", pRoom->vnum, pReset->arg3);
		continue;
	    }

	    if (pReset->arg2 > 50) /* old format */
		limit = 6;
	    else if (pReset->arg2 <= 0) /* no limit */
		limit = 999;
	    else
		limit = pReset->arg2;

	    if (pRoom->area->nplayer > 0
		|| (LastObj = get_obj_type(pObjToIndex)) == NULL
		|| (LastObj->in_room == NULL && !last)
		|| (pObjIndex->count >= limit /* && number_range(0, 4) != 0 */)
		|| (count = count_obj_list(pObjIndex, LastObj->contains))
		> pReset->arg4 )
	    {
		last = FALSE;
		break;
	    }
	    /* lastObj->level  -  ROM */

	    while (count < pReset->arg4)
	    {
		pObj = create_object(pObjIndex, number_fuzzy(LastObj->level));

		if (check_reset_for_limit(pObj, FALSE))
		{
		    extract_obj(pObj, FALSE, FALSE);
		    break;
		}

		create_socket(pObj);

		obj_to_obj(pObj, LastObj);
		count++;

		if (pObjIndex->count >= limit)
		    break;
	    }
	    /* fix object lock state! */
	    LastObj->value[1] = LastObj->pIndexData->value[1];
	    last = TRUE;
	    break;

	case 'T':
	    if (!(pObjIndex = get_obj_index(pReset->arg1)))
	    {
		bugf("Reset_room [%d]: 'T': bad trap vnum %d.", pRoom->vnum, pReset->arg1);
		continue;
	    }

	    if (!(pRoomIndex = get_room_index(pReset->arg2)))
	    {
		bugf("Reset_room [%d]: 'T': bad room vnum %d.", pRoom->vnum, pReset->arg2);
		continue;
	    }

	    if (get_trap(pRoomIndex) != NULL)
		continue;

	    pObj = create_object(pObjIndex, pObjIndex->level);

	    if (check_reset_for_limit(pObj, FALSE))
	    {
		extract_obj(pObj, FALSE, FALSE);
		break;
	    }

	    REMOVE_BIT(pObj->wear_flags, ITEM_TAKE);
	    pObj->value[2] = pObj->level;
	    obj_to_room(pObj, pRoomIndex);
	    break;

	case 'C':
	    if (!(pObjIndex = get_obj_index(pReset->arg1)))
	    {
		bugf("Reset_room [%d]: 'C': bad trap vnum %d.", pRoom->vnum, pReset->arg1);
		continue;
	    }

	    if (!(pObjToIndex = get_obj_index(pReset->arg2)))
	    {
		bugf("Reset_room [%d]: 'C': bad trap container vnum %d.",
		     pRoom->vnum, pReset->arg2);
		continue;
	    }

	    if ((LastObj = get_obj_type(pObjToIndex)) == NULL
		|| (LastObj->in_room == NULL && !last))
	    {
		last = FALSE;
		break;
	    }

	    if (LastObj->item_type != ITEM_CONTAINER
		&& LastObj->item_type != ITEM_PORTAL)
	    {
		bugf("Reset_room [%d]: 'C': bad last object type.", pRoom->vnum);
		continue;
	    }

	    if (LastObj->trap != NULL)
		continue;

	    pObj = create_object(pObjIndex, pObjIndex->level);

	    if (check_reset_for_limit(pObj, FALSE))
	    {
		extract_obj(pObj, FALSE, FALSE);
		break;
	    }

	    REMOVE_BIT(pObj->wear_flags, ITEM_TAKE);
	    pObj->value[2] = pObj->level;
	    LastObj->trap = pObj;
	    LastObj->trap_flags = pReset->arg3;
	    last = TRUE;
	    break;

	case 'X':
	    if (!(pObjIndex = get_obj_index(pReset->arg1)))
	    {
		bugf("Reset_room [%d]: 'X': bad trap vnum %d.", pRoom->vnum, pReset->arg1);
		continue;
	    }

	    if (!(pRoomIndex = get_room_index(pReset->arg4)))
	    {
		bugf("Reset_room [%d]: 'X': bad room vnum %d.", pRoom->vnum, pReset->arg4);
		continue;
	    }

	    if (pReset->arg2 < 0 || pReset->arg2 >= MAX_DIR)
	    {
		bugf("Reset_room [%d]: 'X': bad door number %d: must be "
		     "in [0; %d).", pRoom->vnum, pReset->arg2, MAX_DIR);
		continue;
	    }

	    if (!pRoomIndex->exit[pReset->arg2])
		continue;

	    if (pRoomIndex->exit[pReset->arg2]->trap != NULL)
		continue;

	    pObj = create_object(pObjIndex, pObjIndex->level);

	    if (check_reset_for_limit(pObj, FALSE))
	    {
		extract_obj(pObj, FALSE, FALSE);
		break;
	    }

	    REMOVE_BIT(pObj->wear_flags, ITEM_TAKE);
	    pObj->value[2] = pObj->level;
	    pRoomIndex->exit[pReset->arg2]->trap = pObj;
	    pRoomIndex->exit[pReset->arg2]->trap_flags = pReset->arg3;
	    last = TRUE;
	    break;

	case 'G':
	case 'E':

	    if (!(pObjIndex = get_obj_index(pReset->arg1)))
	    {
		bugf("Reset_room [%d]: 'E' or 'G': bad vnum %d.", pRoom->vnum, pReset->arg1);
		continue;
	    }

	    if (!last)
		break;

	    if (!LastMob)
	    {
		bugf("Reset_room [%d]: 'E' or 'G': null mob for vnum %d.",
		     pRoom->vnum, pReset->arg1);
		last = FALSE;
		break;
	    }

	    if (LastMob->pIndexData->pShop)   /* Shop-keeper? */
	    {
		int olevel=0, i, j;

		if (!pObjIndex->new_format)
		    switch (pObjIndex->item_type)
		    {
		    default:
			olevel = 0;
			break;
		    case ITEM_PILL:
		    case ITEM_POTION:
		    case ITEM_SCROLL:
			olevel = 53;
			for (i = 1; i < 5; i++)
			{
			    if (pObjIndex->value[i] > 0)
			    {
				for (j = 0; j < MAX_CLASS; j++)
				{
				    olevel = UMIN(olevel,
						  skill_table[pObjIndex->value[i]].
						  skill_level[j]);
				}
			    }
			}

			olevel = UMAX(0, (olevel * 3 / 4) - 2);
			break;

		    case ITEM_WAND:
			olevel = number_range(10, 20);
			break;
		    case ITEM_STAFF:
			olevel = number_range(15, 25);
			break;
		    case ITEM_ARMOR:
			olevel = number_range( 5, 15);
			break;
			/* ROM patch weapon, treasure */
		    case ITEM_WEAPON:
			olevel = number_range( 5, 15);
			break;
		    case ITEM_TREASURE:
			olevel = number_range(10, 20);
			break;

		    }

		pObj = create_object(pObjIndex, olevel);
		SET_BIT(pObj->extra_flags, ITEM_INVENTORY);  /* ROM OLC */

	    }
	    else   /* ROM OLC else version */
	    {
		int limit;
		if (pReset->arg2 > 50)  /* old format */
		    limit = 6;
		else if (pReset->arg2 <= 0)  /* no limit */
		    limit = 999;
		else
		    limit = pReset->arg2;

		if (pObjIndex->count < limit /* || number_range(0, 4) == 0 */)
		{
		    pObj = create_object(pObjIndex,
					 UMIN(number_fuzzy(level), LEVEL_HERO));

		    /* error message if it is too high */
		    if (pObj->level > LastMob->level + 3)
		    {
			char buf[MAX_STRING_LENGTH];

			sprintf(buf, "Err: obj %s(%d) -- %d, mob %s(%d) -- %d",
				pObj->short_descr, pObj->pIndexData->vnum,
				pObj->level, LastMob->short_descr,
				LastMob->pIndexData->vnum, LastMob->level);

			log_string(buf);
		    }
		}
		else
		    break;
	    }

	    (void) check_reset_for_limit(pObj, FALSE);

	    create_socket(pObj);

	    obj_to_char(pObj, LastMob);
	    if (pReset->command == 'E')
		equip_char(LastMob, pObj, pReset->arg3);
	    last = TRUE;
	    break;

	case 'D':
	    break;

	case 'R':
	    if (!(pRoomIndex = get_room_index(pReset->arg1)))
	    {
		bugf("Reset_room [%d]: 'R': bad vnum %d.", pRoom->vnum, pReset->arg1);
		continue;
	    }

	    {
		EXIT_DATA *pExit;
		int d0;
		int d1;

		if (pReset->arg2 != 0)
		{
		    for (d0 = 0; d0 < pReset->arg2 - 1; d0++)
		    {
			d1                   = number_range(d0, pReset->arg2-1);
			pExit                = pRoomIndex->exit[d0];
			pRoomIndex->exit[d0] = pRoomIndex->exit[d1];
			pRoomIndex->exit[d1] = pExit;
		    }
		}
	    }
	    break;
	}
    }

    return;
}


/* OLC
 * Reset one area.
 */
void reset_area(AREA_DATA *pArea)
{
    ROOM_INDEX_DATA *pRoom;
    int  vnum;

    for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++)
    {
	if ((pRoom = get_room_index(vnum)))
	    reset_room(pRoom);
    }

    return;
}

void load_exceptions(void)
{
    FILE *fp;
    int type = 0, count;
    char *word;
    EXCEPT_DATA *new_except = NULL;

    if ((fp = fopen(EXCEPT_FILE, "r")) == NULL)
    {
	bugf("Couldn't open exceptions file, no exceptions available.");
	return;
    }

    for (count = 0; ;)
    {
	word = fread_word(fp);

	if (!strcmp(word, "#0"))
	    break;

	if (!strcmp(word, "#NOUNS"))
	{
	    type = NOUN;
	    continue;
	}

	if (!strcmp(word, "#ADJECTIVES"))
	{
	    type = ADJECT;
	    continue;
	}

	if (type == 0)
	{
	    bugf("Load_exceptions: Cannot set type in exceptions.");
	    return;
	}

	if (count == 0)
	{
		new_except  = alloc_perm(sizeof(*new_except));
		new_except->next = exceptions;
	    exceptions = new_except;
		new_except->type = type;
	}

	new_except->cases[count] = str_dup(word);

	if (++count == 7)
	    count = 0;
    }
    fclose(fp);
    return;
}

void load_limits(void)
{
    FILE *fp;
    int count = 0, num;

    if ((fp = fopen(LIMITS_FILE, "r")) == NULL)
    {
	bugf("Couldn't open limits file, no limits available.");
	return;
    }

    log_string("Loading limits...");

    for (count = 0; (count < MAX_LIMITS && feof(fp) == 0);count++)
    {
	num = fread_number(fp);
	limits[count]=num;
	if (num < 0) break;
    }
    fclose(fp);
    return;
}

void load_clans(void)
{
    FILE *fp;
    char *p;
    int count, clan, i;
    char arg[MAX_STRING_LENGTH];
    char *argument = NULL;
    char *tmp_table_war[MAX_CLAN], *tmp_table_all[MAX_CLAN];

    if ((fp = fopen(CLANS_FILE, "r")) == NULL)
    {
	bugf("Couldn't open clans file, no clans available.");
	return;
    }

    for (count = 0; count < MAX_CLAN; count++)
	clan_table[count].valid = FALSE;

    log_string("Loading clans...");

    bzero(tmp_table_all, sizeof(tmp_table_all));
    bzero(tmp_table_war, sizeof(tmp_table_war));

    for (count = 0; (count < MAX_CLAN && feof(fp) == 0); count++)
    {
	p = fread_string_eol(fp);

	if (*p != '#')
	{
	    bugf("Bug in clans file.");
	    fclose(fp);
	    exit(1);
	}

	if (*(p + 1) == '0')
	{
	    free_string(p);
	    break;
	}

	clan_table[count].name = str_dup(p + 1);
	free_string(p);
	clan_table[count].short_descr = fread_string(fp);

	clan_table[count].long_descr = fread_string(fp);
	clan_table[count].clanmaster = fread_string(fp);
	clan_table[count].clanmaster2 = fread_string(fp);

	argument = fread_string(fp);

	for (clan = 1; clan < MAX_PC_RACE; clan++)
	    if (argument[0] == '\0')
		clan_table[count].races[clan] = TRUE;
	    else
		clan_table[count].races[clan] = FALSE;

	for (p = one_argument(argument, arg);
	     arg[0] != '\0';
	     p = one_argument(p, arg))
	{
	    if ((clan = race_lookup(arg)) != -1)
		clan_table[count].races[clan] = TRUE;
	}

	free_string(argument);
	argument = fread_string(fp);

	for (clan = 0; clan < MAX_CLASS; clan++)
	    if (argument[0] == '\0')
		clan_table[count].classes[clan] = TRUE;
	    else
		clan_table[count].classes[clan] = FALSE;

	for (p = one_argument(argument, arg);
	     arg[0] != '\0';
	     p = one_argument(p, arg))
	{
	    if ((clan = class_lookup(arg)) != -1)
		clan_table[count].classes[clan] = TRUE;
	}

	free_string(argument);

	clan_table[count].count = fread_number(fp);
	clan_table[count].min_align = fread_number(fp);
	clan_table[count].max_align = fread_number(fp);
	clan_table[count].recall_vnum = fread_number(fp);

	fBootDb = FALSE;
	tmp_table_war[count] = fread_string(fp);
	tmp_table_all[count] = fread_string(fp);
	fBootDb = TRUE;

	clan_table[count].rating = fread_number(fp);
	clan_table[count].valid = TRUE;
    }
    fclose(fp);

    for (i = 0; i < count; i++)
    {
	for (clan = 0; clan < MAX_CLAN; clan++)
	    clan_table[i].clan_war[clan] = FALSE;

	for (argument = one_argument(tmp_table_war[i], arg);
	     arg[0] != '\0';
	     argument = one_argument(argument, arg))
	{
	    if ((clan = clan_lookup(arg)) != 0)
		clan_table[i].clan_war[clan] = TRUE;
	}
    }

    for (i = 0; i < count; i++)
    {
	for (clan = 0; clan < MAX_CLAN; clan++)
	    clan_table[i].clan_alliance[clan] = FALSE;

	for (argument = one_argument(tmp_table_all[i], arg);
	     arg[0] != '\0';
	     argument = one_argument(argument, arg))
	{
	    if ((clan = clan_lookup(arg)) != 0)
		clan_table[i].clan_alliance[clan] = TRUE;
	}
    }

    for (i = 0; i < MAX_CLAN; i++)
    {
	if (tmp_table_all[i] != NULL)
	    free_string(tmp_table_all[i]);

	if (tmp_table_war[i] != NULL)
	    free_string(tmp_table_war[i]);
    } 

    log_string("Ok.");

    return;
}

void load_objprogs(FILE *fp)
{
    PROG_CODE *pOprog;

    if (area_last == NULL)
    {
	bugf("Load_objprogs: no #AREA seen yet.");
	exit(1);
    }

    for (; ;)
    {
	int vnum;
	char letter;

	letter		  = fread_letter(fp);
	if (letter != '#')
	{
	    bugf("Load_objprogs: # not found.");
	    exit(1);
	}

	vnum		 = fread_number(fp);
	if (vnum == 0)
	    break;

	fBootDb = FALSE;
	if (get_prog_index(vnum, PRG_OPROG) != NULL)
	{
	    bugf("Load_objprogs: vnum %d duplicated.", vnum);
	    exit(1);
	}
	fBootDb = TRUE;

	pOprog		= alloc_perm(sizeof(*pOprog));
	pOprog->vnum  	= vnum;
	pOprog->code  	= fread_string(fp);
	if (oprog_list == NULL)
	{
	    oprog_list = pOprog;
	    pOprog->next = NULL;
	}
	else
	{
	    pOprog->next = oprog_list;
	    oprog_list 	= pOprog;
	}
	top_oprog_index++;
    }
    return;
}

void load_roomprogs(FILE *fp)
{
    PROG_CODE *pRprog;

    if (area_last == NULL)
    {
	bugf("Load_roomprogs: no #AREA seen yet.");
	exit(1);
    }

    for (; ;)
    {
	int vnum;
	char letter;

	letter		  = fread_letter(fp);
	if (letter != '#')
	{
	    bugf("Load_roomprogs: # not found.");
	    exit(1);
	}

	vnum		 = fread_number(fp);
	if (vnum == 0)
	    break;

	fBootDb = FALSE;
	if (get_prog_index(vnum, PRG_RPROG) != NULL)
	{
	    bugf("Load_roomprogs: vnum %d duplicated.", vnum);
	    exit(1);
	}
	fBootDb = TRUE;

	pRprog		  = alloc_perm(sizeof(*pRprog));
	pRprog->vnum  	  = vnum;
	pRprog->code  	  = fread_string(fp);
	if (rprog_list == NULL)
	    rprog_list = pRprog;
	else
	{
	    pRprog->next = rprog_list;
	    rprog_list 	= pRprog;
	}
	top_rprog_index++;
    }
    return;
}

void fix_objprogs(void)
{
    OBJ_INDEX_DATA *pObjIndex;
    PROG_LIST        *list;
    PROG_CODE        *prog;
    int iHash;

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
	for (pObjIndex = obj_index_hash[iHash];
	     pObjIndex != NULL;
	     pObjIndex = pObjIndex->next)
	{
	    for (list = pObjIndex->progs; list; list = list->next)
	    {
		if ((prog = get_prog_index(list->vnum, PRG_OPROG)) != NULL)
		    list->code = prog->code;
		else
		{
		    bugf("Fix_objprogs: code vnum %d not found from object %d.", list->vnum, pObjIndex->vnum);
		    exit(1);
		}
	    }
	}
    }
}

void fix_roomprogs(void)
{
    ROOM_INDEX_DATA *pRoomIndex;
    PROG_LIST        *list;
    PROG_CODE        *prog;
    int iHash;

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
	for (pRoomIndex = room_index_hash[iHash];
	     pRoomIndex != NULL;
	     pRoomIndex = pRoomIndex->next)
	{
	    for (list = pRoomIndex->progs; list != NULL; list = list->next)
	    {
		if ((prog = get_prog_index(list->vnum, PRG_RPROG)) != NULL)
		{
		    list->code = prog->code;
		}
		else
		{
		    bugf("Fix_roomprogs: code vnum %d not found.", list->vnum);
		    exit(1);
		}
	    }
	}
    }
}

void load_dreams(void)
{
    FILE *fp;
    int count = 0;
    char letter;

    if ((fp = fopen(DREAM_FILE, "r")) == NULL)
    {
	bugf("Couldn't open dreams file, no dreams available.");
	return;
    }

    for (count = 0; count < MAX_DREAMS; count++)
    {
	letter = fread_letter(fp);
	if (letter == '#')
	{
	    if (count < MAX_DREAMS)
		dream_table[count] = NULL;
/*	    fclose(fp); */
	    return;
	}
	else
	    ungetc(letter, fp);

	dream_table[count] = fread_string(fp);
    }

}

void load_quotes(void)
{
    FILE *fp;
    int count;
    char letter;

    if ((fp = fopen(QUOTES_FILE, "r")) == NULL)
    {
	bugf("Couldn't open quotes file, no quotes available.");
	return;
    }

    for (count = 0; count < MAX_QUOTES && !feof(fp); count++)
    {
	letter = fread_letter(fp);
	if (letter == '#')
	{
	    quote_table[count] = NULL;
/*	    fclose(fp); */
	    break;
	}
	else
	    ungetc(letter, fp);

	quote_table[count] = fread_string_eol(fp);
    }
    fclose(fp);
    return;
}

void load_badnames()
{
    FILE *fp;
    int i;
    char let;

    if ((fp = fopen(BADNAMES_FILE, "r")) == NULL)
    {
	bugf("Couldn't open bad names file.");
	return;
    }

    for (i = 0; i < MAX_BAD_NAMES && !feof(fp);)
    {
	let = fread_letter(fp);
	if (let == '#')
	{
	    badnames[i] = NULL;
/*	    fclose(fp);*/
	    break;
	}
	else if (let == '*' || let == '\n' || let == '\r')
	    fread_to_eol(fp);
	else
	{
	    ungetc(let, fp);
	    badnames[i++] = fread_string_eol(fp);
	}
    }
    fclose(fp);
}

void load_stoplist(void)
{
    FILE *fp;
    int count;
    char letter;

    if ((fp = fopen(STOPLIST_FILE, "r")) == NULL)
    {
	bugf("Couldn't open stoplist file, no filters available.");
	return;
    }

    for (count = 0; count < MAX_STOP_WORDS && !feof(fp); count++)
    {
	letter = fread_letter(fp);
	if (letter == '#')
	{
	    stoplist[count] = NULL;
/*	    fclose(fp);*/
	    break;
	}
	else
	    ungetc(letter, fp);

	stoplist[count] = fread_string_eol(fp);
    }
    fclose(fp);
    return;
}

void load_whois(void)
{
    FILE *fp;
    int count = 0;
    char letter, *word, **p = NULL;

    if ((fp = fopen(WHOIS_FILE, "r")) == NULL)
    {
	return;
    }

    while(1)
    {
	if (feof(fp))
	    break;

	letter = fread_letter(fp);
	if (letter == '#')
	{
	    word = fread_word(fp);
	    if (!str_cmp(word, "WHITELIST"))
	    {
		p = whiteip;
		count = 0;
	    }
	    else if (!str_cmp(word, "DOMAINS"))
	    {
		p = whitedomains;
		count = 0;
	    }
	    else if (!str_cmp(word, "END"))
		break;
	}
	else if (letter == '*')
	{
	    fread_to_eol(fp);
	    continue;
	}
	else
	    ungetc(letter, fp);

	if (count < MAX_WHOIS_ENTRIES)
	{
	    p[count] = fread_string_eol(fp);
	    p[++count] = NULL;

	}
    }

    fclose(fp);
    return;
}

void load_popularity(void)
{
    FILE *fp;
    int  vers = 0;
    char letter;

    if ((fp = fopen(POPULARITY_FILE, "r")) == NULL)
    {
	bugf("Couldn't open popularity file, no old popularity available.");
	return;
    }

    if ((letter = fread_letter(fp)) != '#')
    {
	ungetc(letter, fp);
    }
    else
    {
	vers = fread_number(fp);
    }

    while (!feof(fp))
    {
	char *name;
	int64_t p;
	int i;
	AREA_DATA *area;

	name = fread_word(fp);

	for (area = area_first; area; area = area->next)
	    if (!str_cmp(area->file_name, name))
		break;

	for (i = 0; i < MAX_POPUL; i++)
	{
	    if (vers == 0 && i >= POPUL_DEATH)
		continue;

	    p = fread_number(fp);

	    if (area)
		area->popularity[i] = p;
	}

	fread_to_eol(fp);
    }
    fclose(fp);
    return;
}

/* charset=cp1251 */











