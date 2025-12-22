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
*  ROM 2.4 is copyright 1993-1998 Russ Taylor                              *
*  ROM has been brought to you by the ROM consortium                       *
*       Russ Taylor (rtaylor@hypercube.org)                                 *
*      Gabrielle Taylor (gtaylor@hypercube.org)                            *
*      Brian Moore (zump@rom.org)                                          *
*  By using this code, you have agreed to follow the terms of the          *
*  ROM license, in the file Rom24/doc/rom.license                          *
***************************************************************************/
/* $Id: merc.h,v 1.221.2.125 2014/01/30 17:24:23 mud Exp $ */

#ifndef MERC_H
#define MERC_H



#include "queue.h"

/*
 * Accommodate old non-Ansi compilers.
 */
#if defined(TRADITIONAL)
    #define const
    #define args(list)				()
    #define DECLARE_DO_FUN(fun)			void fun()
    #define DECLARE_SPEC_FUN(fun)		bool fun()
    #define DECLARE_SPELL_FUN(fun)		void fun()
    #define DECLARE_OBJ_FUN(fun)		void fun()
    #define DECLARE_ROOM_FUN(fun)		void fun()
    #define DECLARE_RELIG_FUN(fun)		void fun()
#else
    #define args(list)			list
    #define DECLARE_DO_FUN(fun)		DO_FUN    fun
    #define DECLARE_SPEC_FUN(fun)	SPEC_FUN  fun
    #define DECLARE_SPELL_FUN(fun)	SPELL_FUN fun
    #define DECLARE_OBJ_FUN(fun)	OBJ_FUN	  fun
    #define DECLARE_ROOM_FUN(fun)	ROOM_FUN  fun
#endif

/* damage classes */
#define DAM_NONE		0
#define DAM_BASH		1
#define DAM_PIERCE		2
#define DAM_SLASH		3
#define DAM_FIRE		4
#define DAM_COLD		5
#define DAM_LIGHTNING	6
#define DAM_ACID		7
#define DAM_POISON		8
#define DAM_NEGATIVE	9
#define DAM_HOLY		10
#define DAM_ENERGY		11
#define DAM_MENTAL		12
#define DAM_DISEASE		13
#define DAM_DROWNING	14
#define DAM_LIGHT		15
#define DAM_OTHER		16
#define DAM_HARM		17
#define DAM_CHARM		18
#define DAM_SOUND		19
#define DAM_WOOD		20
#define DAM_SILVER		21
#define DAM_IRON		22
#define DAM_MAGIC		23
#define DAM_SUMMON		24
#define DAM_WEAPON		25
#define DAM_MAX			26

/* максимальное кол-во приглашенных и приглашаемых */
#define  MAX_GUESTS		10

#define MOB_VNUM_ZOMBIE		1
#define MOB_VNUM_NAZGUL_HORSE	2
#define MOB_BONE_SHIELD		8

#define ALIGN_EVIL	1
#define ALIGN_NEUTRAL	2
#define ALIGN_GOOD	4

/* mccp support */

#include <zlib.h>
#define COMPRESS_BUF_SIZE 16384


#define  COMPILE_TIME "Compiled on " __DATE__ " at " __TIME__ ".\n\r\n\r"
#define MSK_OFFSET "+04:00"
#define UTC_OFFSET "+03:00"

/*
 * Short scalar types.
 * Diavolo reports AIX compiler has bugs with short types.
 */
#if  !defined(FALSE)
#define FALSE	0
#endif

#if  !defined(TRUE)
#define TRUE	1
#endif

/* Special return codes */
#define FIGHT_DEAD 16
#define SPEC_STOP  32

#if  defined(_AIX)
#if  !defined(const)
#define const
#endif
typedef int bool;
#else
typedef unsigned char bool;
#endif

#if !defined(__FreeBSD__) || __FreeBSD__ < 4
#  define NO_STRL_FUNCS
#endif

/*
#if !defined(uint16_t)
typedef unsigned short int uint16_t;
#endif
*/

#define SAGE_COST  15  /* для do_identify() и auction identify */

#define CLASS_MAGE	0
#define CLASS_CLERIC	1
#define CLASS_THIEF	2
#define CLASS_WARRIOR	3
#define CLASS_NECROMANT	4
#define CLASS_PALADIN	5
#define CLASS_NAZGUL	6
#define CLASS_DRUID	7
#define CLASS_RANGER	8
#define CLASS_VAMPIRE	9
#define CLASS_ALCHEMIST	10
#define CLASS_LYCANTHROPE 11
#define MAX_CLASS	12

#define CLASS_NO_MAGIC		0
#define CLASS_SLIGHTLY_MAGIC	1
#define CLASS_SEMI_MAGIC	2
#define CLASS_MAGIC		3


#define RACE_HUMAN	1
#define RACE_ELF	2
#define RACE_DWARF	3
#define RACE_GIANT	4
#define RACE_ORC	5
#define RACE_TROLL	6
#define RACE_SNAKE	7
#define RACE_HOBBIT	8
#define RACE_DROW	9
#define RACE_VAMPIRE	10
#define RACE_LYCANTHROPE 11
#define RACE_ZOMBIE 12
#define RACE_TIGGUAN 13
#define MAX_PC_RACE	14

/*
 * Structure types.
 */
typedef struct  track_data		    TRACK_DATA;
typedef struct  affect_data		    AFFECT_DATA;
typedef struct  area_data		    AREA_DATA;
typedef struct  ban_data		    BAN_DATA;
typedef struct  buf_type		    BUFFER;
typedef struct  char_data		    CHAR_DATA;
typedef struct  descriptor_data	    DESCRIPTOR_DATA;
typedef struct  exit_data		    EXIT_DATA;
typedef struct  extra_descr_data	EXTRA_DESCR_DATA;
typedef struct  help_data		    HELP_DATA;
typedef struct  help_area_data		HELP_AREA;
typedef struct  kill_data		    KILL_DATA;
typedef struct  except_data		    EXCEPT_DATA;
typedef struct  mem_data		    MEM_DATA;
typedef struct  mob_index_data		MOB_INDEX_DATA;
typedef struct  vote_data		    VOTE_DATA;
typedef struct  char_votes		    CHAR_VOTES;
typedef struct  note_data		    NOTE_DATA;
typedef struct  obj_data		    OBJ_DATA;
typedef struct  obj_index_data		OBJ_INDEX_DATA;
typedef struct  pc_data			    PC_DATA;
typedef struct  gen_data		    GEN_DATA;
typedef struct  reset_data		    RESET_DATA;
typedef struct  room_index_data		ROOM_INDEX_DATA;
typedef struct  shop_data		    SHOP_DATA;
typedef struct  time_info_data		TIME_INFO_DATA;
typedef struct  weather_data		WEATHER_DATA;
typedef struct  colour_data		    COLOUR_DATA;
typedef struct  prog_list		    PROG_LIST;
typedef struct  prog_code		    PROG_CODE;
typedef struct  auction_data		AUCTION_DATA;
typedef struct  challenger_data		CHALLENGER_DATA;
typedef struct  score_data		    SCORE_DATA;
typedef struct  recipe_data		    RECIPE_DATA;
typedef struct  fight_data		    FIGHT_DATA;
typedef struct  spellaff		    SPELLAFF;
typedef struct  query_data		    QUERY_DATA;
typedef struct  quest_datan		    QUEST_DATAN;

#define COLOUR_GOSSIP		1   /* {9 */
#define COLOUR_GOSSIP_NAME	2   /* {d */
#define COLOUR_SHOUT		3   /* {h */
#define COLOUR_SHOUT_NAME	4   /* {H */
#define COLOUR_YELL		5   /* {8 */
#define COLOUR_YELL_NAME	6   /* {! */
#define COLOUR_TELL		7   /* {k */
#define COLOUR_TELL_NAME	8   /* {K */
#define COLOUR_ANSWER		9   /* {l */
#define COLOUR_ANSWER_NAME	10  /* {L */
#define COLOUR_SAY		11  /* {6 */
#define COLOUR_SAY_NAME		12  /* {7 */
#define COLOUR_EMOTE		13  /* {A */
#define COLOUR_OOC		14  /* {q */
#define COLOUR_OOC_NAME		15  /* {Q */
#define COLOUR_NEWBIES		16  /* {f */
#define COLOUR_NEWBIES_NAME	17  /* {F */
#define COLOUR_WIZNET		18  /* {Z */
#define COLOUR_IMMTALK		19  /* {i */
#define COLOUR_IMMTALK_NAME	20  /* {I */
#define COLOUR_BTALK		21  /* {j */
#define COLOUR_BTALK_NAME	22  /* {J */
#define COLOUR_CLANTALK		23  /* {p */
#define COLOUR_CLANTALK_NAME	24  /* {P */
#define COLOUR_GROUPTALK	25  /* {n */
#define COLOUR_GROUPTALK_NAME	26  /* {N */
#define COLOUR_AUCTION		27  /* {a */
#define COLOUR_MUSIC		28  /* {e */
#define COLOUR_MUSIC_NAME	29  /* {E */
#define COLOUR_GRATS		30  /* {@ */

#define COLOUR_ROOMNAMES	31  /* {s */
#define COLOUR_ROOMDESC		32  /* {S */
#define COLOUR_MOBS		33  /* {t */
#define COLOUR_CHARS		34  /* {u */
#define COLOUR_GROUPCHARS	35  /* {U */
#define COLOUR_OBJS		36  /* {O */
#define COLOUR_EXITS		37  /* {o */

#define COLOUR_AFFON		38  /* {v */
#define COLOUR_AFFOFF		39  /* {V */
#define COLOUR_CAST		40  /* {z */
#define COLOUR_CASTNAME		41  /* {T */
#define COLOUR_AFFECT		42  /* {# */

#define COLOUR_YOURHIT		43  /* {0 */
#define COLOUR_ENEMYHIT		44  /* {2 */
#define COLOUR_OTHERHIT		45  /* {3 */
#define COLOUR_SKILL		46  /* {5 */
#define COLOUR_DEATH		47  /* {1 */
#define COLOUR_CRIT		48  /* {4 */

#define MAX_COLOUR		49

struct colour_data
{
    char *code;      /* actual ANSI escape string */
    int level;      /* level to set */
};

struct colour_schema
{
    char *name;
    char *rname;
    struct colour_schema *parent;
    COLOUR_DATA colours[MAX_COLOUR];
};

extern struct colour_schema *colour_schemas;
extern struct colour_schema *colour_schema_default;


/*
 * Function types.
 */
typedef void DO_FUN  args((CHAR_DATA *ch, char *argument));
typedef bool SPEC_FUN  args((CHAR_DATA *ch));
typedef bool SPELL_FUN  args((int sn, int level, CHAR_DATA *ch, void *vo,
			      int target));

typedef void OBJ_FUN  args((OBJ_DATA *obj, char *argument));
typedef void ROOM_FUN  args((ROOM_INDEX_DATA *room, char *argument));
typedef void RELIG_FUN  args((CHAR_DATA *ch, int16_t react));

typedef void EFFECT_FUN args((void *vo, int level, int dam, int target));


/*
 * String and memory management parameters.
 */
#define MIN_FREE_SPACE		100000  /* Kilobytes */
#define MAX_KEY_HASH		1024

#define MAX_STRING_LENGTH	14000
#define MAX_INPUT_LENGTH	720
#define MAX_LIMITS		1000
#define MAX_FILTERS		50
#define PAGELEN			22
#define MSL			MAX_STRING_LENGTH
#define MIL			MAX_INPUT_LENGTH

#define LIMIT_LIFETIME		60*60*24*21

#define MAX_CYCLES		100
#define MAX_DESCRIPTION_SIZE	4096
#define MAX_NOTEPAD_SIZE	1024

/*
 * Game parameters.
 * Increase the max'es if you add more of something.
 * Adjust the pulse numbers to suit yourself.
 */
#define MAX_SOCIALS		    256
#define MAX_SKILL		    420
#define MAX_GROUP		    54
#define MAX_IN_GROUP		35
#define MAX_ALIAS		    25
#define MAX_DAMAGE_MESSAGE	43

#define MAX_LEVEL		    60
#define MAX_STAT		    32
#define MAX_SAVES           240

#define LEVEL_HERO		    (MAX_LEVEL - 9)
#define LEVEL_IMMORTAL		(MAX_LEVEL - 8)
#define LEVEL_NEWBIE		3
#define TITLE_LEVEL		    35
#define MAX_RECALL_LEVEL	15
#define MAX_VNUM		    0x3fffffff

#define AVG_STAT		    23


#define MAX_SKILL_OVER		150000

#define PULSE_PER_SECOND	4
#define PULSE_VIOLENCE		(3 * PULSE_PER_SECOND)
#define PULSE_MOBILE		(4 * PULSE_PER_SECOND)
#define PULSE_MUSIC		    (6 * PULSE_PER_SECOND)
#define PULSE_TICK		    (60 * PULSE_PER_SECOND)
#define PULSE_AREA		    (120 * PULSE_PER_SECOND)

#define IMPLEMENTOR	        MAX_LEVEL
#define CREATOR		        (MAX_LEVEL - 1)
#define SUPREME		        (MAX_LEVEL - 2)
#define DEITY		        (MAX_LEVEL - 3)
#define GOD		            (MAX_LEVEL - 4)
#define IMMORTAL	        (MAX_LEVEL - 5)
#define DEMI		        (MAX_LEVEL - 6)
#define ANGEL		        (MAX_LEVEL - 7)
#define AVATAR		        (MAX_LEVEL - 8)
#define HERO		        LEVEL_HERO


/*
 * ColoUr stuff v2.0, by Lope.
 */
#define CLEAR		"\e[0m"		/* Resets Colour  */
#define C_RED		"\e[0;31m"	/* Normal Colours  */
#define C_GREEN		"\e[0;32m"
#define C_YELLOW	"\e[0;33m"
#define C_BLUE		"\e[0;34m"
#define C_MAGENTA	"\e[0;35m"
#define C_CYAN		"\e[0;36m"
#define C_WHITE		"\e[0;37m"
#define C_D_GREY	"\e[1;30m"	/* Light Colors    */
#define C_B_RED		"\e[1;31m"
#define C_B_GREEN	"\e[1;32m"
#define C_B_YELLOW	"\e[1;33m"
#define C_B_BLUE	"\e[1;34m"
#define C_B_MAGENTA	"\e[1;35m"
#define C_B_CYAN	"\e[1;36m"
#define C_B_WHITE	"\e[1;37m"

#define COLOUR_NONE	7    /* White, hmm...  */
#define RED		1    /* Normal Colours  */
#define GREEN		2
#define YELLOW		3
#define BLUE		4
#define MAGENTA		5
#define CYAN		6
#define WHITE		7
#define BLACK		0

#define NORMAL_		0    /* Bright/Normal colours */
#define BRIGHT		1

#define ALTER_COLOUR(type)  if(!str_prefix(argument, "red"))      \
        {              \
            ch->pcdata->type[0] = NORMAL_;      \
            ch->pcdata->type[1] = RED;        \
        }              \
        else if(!str_prefix(argument, "hi-red"))    \
        {              \
            ch->pcdata->type[0] = BRIGHT;      \
            ch->pcdata->type[1] = RED;        \
        }              \
        else if(!str_prefix(argument, "green"))      \
        {              \
            ch->pcdata->type[0] = NORMAL_;      \
            ch->pcdata->type[1] = GREEN;      \
        }              \
        else if(!str_prefix(argument, "hi-green"))  \
        {              \
            ch->pcdata->type[0] = BRIGHT;      \
            ch->pcdata->type[1] = GREEN;      \
        }              \
        else if(!str_prefix(argument, "yellow"))    \
        {              \
            ch->pcdata->type[0] = NORMAL_;      \
            ch->pcdata->type[1] = YELLOW;      \
        }              \
        else if(!str_prefix(argument, "hi-yellow")) \
        {              \
            ch->pcdata->type[0] = BRIGHT;      \
            ch->pcdata->type[1] = YELLOW;      \
        }              \
        else if(!str_prefix(argument, "blue"))      \
        {              \
            ch->pcdata->type[0] = NORMAL_;      \
            ch->pcdata->type[1] = BLUE;        \
        }              \
        else if(!str_prefix(argument, "hi-blue"))   \
        {              \
            ch->pcdata->type[0] = BRIGHT;      \
            ch->pcdata->type[1] = BLUE;        \
        }              \
        else if(!str_prefix(argument, "magenta"))   \
        {              \
            ch->pcdata->type[0] = NORMAL_;      \
            ch->pcdata->type[1] = MAGENTA;      \
        }              \
        else if(!str_prefix(argument, "hi-magenta"))\
        {              \
            ch->pcdata->type[0] = BRIGHT;      \
            ch->pcdata->type[1] = MAGENTA;      \
        }              \
        else if(!str_prefix(argument, "cyan"))      \
        {              \
            ch->pcdata->type[0] = NORMAL_;      \
            ch->pcdata->type[1] = CYAN;        \
        }              \
        else if(!str_prefix(argument, "hi-cyan"))   \
        {              \
            ch->pcdata->type[0] = BRIGHT;      \
            ch->pcdata->type[1] = CYAN;        \
        }              \
        else if(!str_prefix(argument, "white"))      \
        {              \
            ch->pcdata->type[0] = NORMAL_;      \
            ch->pcdata->type[1] = WHITE;      \
        }              \
        else if(!str_prefix(argument, "hi-white"))  \
        {              \
            ch->pcdata->type[0] = BRIGHT;      \
            ch->pcdata->type[1] = WHITE;      \
        }              \
        else if(!str_prefix(argument, "grey"))      \
        {              \
            ch->pcdata->type[0] = BRIGHT;      \
            ch->pcdata->type[1] = BLACK;      \
        }              \
        else if(!str_prefix(argument, "beep"))      \
        {              \
            ch->pcdata->type[2] = 1;        \
        }              \
        else if(!str_prefix(argument, "nobeep"))    \
        {              \
            ch->pcdata->type[2] = 0;        \
        }              \
        else              \
        {              \
    send_to_char("Unrecognised colour, unchanged.\n\r", ch); \
            return;            \
        }

#define LOAD_COLOUR(field)  ch->pcdata->field[1] = fread_number(fp);\
        if(ch->pcdata->field[1] > 100)    \
        {          \
            ch->pcdata->field[1] -= 100;  \
            ch->pcdata->field[2] = 1;    \
        }          \
        else          \
        {          \
            ch->pcdata->field[2] = 0;    \
        }          \
        if(ch->pcdata->field[1] > 10)    \
        {          \
            ch->pcdata->field[1] -= 10;    \
            ch->pcdata->field[0] = 1;    \
        }          \
        else          \
        {          \
            ch->pcdata->field[0] = 0;    \
        }



#define IS_DRUNK(ch)		(!IS_NPC(ch) && (ch)->pcdata->condition[COND_DRUNK] > DRUNK_LEVEL)

#define IS_IN_FOREST(ch)	((ch)->in_room && (ch)->in_room->sector_type == SECT_FOREST)

#define IS_IN_WILD(ch)		((ch)->in_room \
				 && ((ch)->in_room->sector_type == SECT_FOREST\
				     || (ch)->in_room->sector_type == SECT_FIELD\
				     || (ch)->in_room->sector_type == SECT_HILLS\
				     || (ch)->in_room->sector_type == SECT_MOUNTAIN\
				     || (ch)->in_room->sector_type == SECT_DESERT))

#define IS_IN_TOWN(ch)		((ch)->in_room \
				 && ((ch)->in_room->sector_type == SECT_INSIDE \
				     || (ch)->in_room->sector_type == SECT_CITY))


#define SECTOR(ch)		((ch)->in_room ? (ch)->in_room->sector_type : 0)

#define IS_VAMPIRE(ch)		(!IS_NPC((ch)) && (ch)->classid == CLASS_VAMPIRE)
#define IS_NOT_SATIETY(ch)	(IS_VAMPIRE((ch)) && !IS_AFFECTED((ch), AFF_SATIETY) && \
				 (ch)->level >= skill_table[gsn_bite].skill_level[(ch)->classid])

#define IS_HELPER(ch)		(!IS_NPC((ch)) && (ch)->level >= (PK_RANGE)*2 && !IS_IMMORTAL(ch) && IS_SET((ch)->comm, COMM_HELPER))
#define IS_UNDEAD(ch)		((IS_NPC(ch) && IS_SET(ch->act, ACT_UNDEAD)) || IS_VAMPIRE(ch))

#define IS_RENEGATE(ch)		(!IS_NPC((ch)) && (((ch)->alignment <= 333 && (ch)->pcdata->orig_align >  333) \
						   || ((ch)->alignment >= -333 && (ch)->pcdata->orig_align < -333) \
						   || (((ch)->alignment > 333 || (ch)->alignment < -333) \
						       && (ch)->pcdata->orig_align <= 333 && (ch)->pcdata->orig_align >= -333)))

#define SUPPRESS_OUTPUT(x)	((((Show_output_old = Show_output) && (Show_output = FALSE)) || TRUE) \
				 && ((Show_result = (x)) || TRUE) && ((Show_output = Show_output_old) || TRUE) && Show_result) 

#define HAS_CORPSE(ch)		(!IS_SET((ch)->act, ACT_UNDEAD)		\
				 && !IS_SET((ch)->form, FORM_MIST)	\
				 && !IS_SET((ch)->form, FORM_INTANGIBLE)\
				 && !IS_SET((ch)->form, FORM_UNDEAD)	\
				 && !IS_SET((ch)->form, FORM_BLOB))

#define POS_FIGHT(ch)		(ch->position == POS_FIGHTING || ch->position == POS_BASHED)

#define END_CHAR_LIST(ch)	(ch == NULL || ch == char_free)
#define END_DESC_LIST(d)	(d == NULL || d == descriptor_free)
#define END_OBJ_LIST(obj)	(obj == NULL || obj == obj_free)

#define IS_WIZINVISMOB(ch)	(ch && IS_NPC(ch) && IS_SET(ch->act, ACT_WIZINVIS))

/* vnums of called charmises...  8))  */
#define MOB_VNUM_DRUIDS_TREE	3  /* дерево */
#define MOB_VNUM_DRUIDS_WOLF	4  /* волк    (31-35 уровни) */
#define MOB_VNUM_DRUIDS_LION	5  /* лев     (36-40 --"---) */
#define MOB_VNUM_DRUIDS_BEAR	6  /* медведь (41-45 --"---) */
#define MOB_VNUM_DRUIDS_DRAGON	7  /* дракон  (46-51 --"---) */
				   /* дракоша кастовать должен уметь */

#define OBJ_VNUM_DRUIDS_SWORD	27
#define OBJ_VNUM_CAMPFIRE	28
#define OBJ_VNUM_MEAT		29
#define OBJ_VNUM_TORCH		9
#define OBJ_VNUM_SKELETON	18
#define OBJ_VNUM_SOUL		8
#define OBJ_VNUM_BAG		30
#define OBJ_VNUM_SLOT		2603


/* Styles */
#define NORM_STYLE    0
#define CARE_STYLE    1
#define AGGR_STYLE    2


/*
 * Site ban structure.
 */

#define BAN_SUFFIX	A
#define BAN_PREFIX	B
#define BAN_NEWBIES	C
#define BAN_ALL		D
#define BAN_PERMIT	E
#define BAN_PERMANENT	F
#define BAN_WHOIS	G

struct ban_data
{
    bool	valid;
    int16_t	ban_flags;
    int16_t	level;
    char	*name;
    char	*reason;
    char	*banner;
    time_t	when;
    SLIST_ENTRY(ban_data) link;
};


struct race_incompatible
{
    int16_t race1;
    int16_t race2;
};

#define MAX_DREAMS    200
char *dream_table[MAX_DREAMS];

#define MAX_QUOTES    200
char *quote_table[MAX_QUOTES];

#define MAX_STOP_WORDS    100
char *stoplist[MAX_STOP_WORDS];

#define MAX_BAD_NAMES    1000
char *badnames[MAX_BAD_NAMES];

#define MAX_WHOIS_ENTRIES	200
char *whiteip[MAX_WHOIS_ENTRIES];
char *whitedomains[MAX_WHOIS_ENTRIES];

/*
 * Religion stuff
 */
struct religion
{
    char	*name;
    int		altar_vnum;
    int16_t	races;
    int16_t	classes;
    int16_t	min_align;
    int16_t	max_align;
    int16_t	pref_item;
    int16_t	pref_weapon;
    int16_t	pref_material;
    RELIG_FUN	*fun;
};


/*
 * Drunk struct
 */
struct struckdrunk
{
  int	min_drunk_level;
  int	number_of_rep;
  char	*replacement[11];
};

/*
 * Lycanthrope struct
 */
struct struckhowl
{
  int	number_of_rep;
  char	*replacement[11];
};

struct buf_type
{
    bool	valid;
    int16_t	state;	 /* error state of the buffer */
    int		size;	 /* size in k */
    char	*string; /* buffer's string */
    SLIST_ENTRY(buf_type) link;
};

/*
 * Auction data
 */
struct auction_data
{
    OBJ_DATA	*obj;
    CHAR_DATA	*sender;
    CHAR_DATA	*buyer;
    int		cost;
    char	time;
    bool	valid;
};
#define MAX_AUCTION  5
AUCTION_DATA auctions[MAX_AUCTION];

/*
 * Time and weather stuff.
 */
#define SUN_DARK	0
#define SUN_RISE	1
#define SUN_LIGHT	2
#define SUN_SET		3

#define SKY_CLOUDLESS	0
#define SKY_CLOUDY	1
#define SKY_RAINING	2
#define SKY_LIGHTNING	3

#define NUM_DAYS	30
#define NUM_MONTHS	12

struct  time_info_data
{
    int    hour;
    int    day;
    int    month;
    int    year;
};

struct  weather_data
{
    int    mmhg;
    int    change;
    int    sky;
    int    sunlight;
};

#define MOON_NEW	0
#define MOON_1QTR	1
#define MOON_2QTR	2
#define MOON_FULL	3
#define MOON_3QTR	4
#define MOON_4QTR	5
#define MOON_MAX_QTRS	6

#define MAX_MOON_MESSAGES 10

struct moon_type
{
    char *name;
    char *msg_rise[MAX_MOON_MESSAGES];
    char *msg_set[MAX_MOON_MESSAGES];
    char state;
    char moonlight;
    char period;
    char state_period;
    char type;    /* TAR_* flags */
    char color;
};

struct moon_type *moons_data;

/*
 * Connected state for a channel.
 */
#define CON_PLAYING			0
#define CON_GET_NAME			1
#define CON_GET_OLD_PASSWORD		2
#define CON_CONFIRM_NEW_NAME		3
#define CON_GET_NEW_PASSWORD		4
#define CON_CONFIRM_NEW_PASSWORD	5
#define CON_GET_NEW_RACE		6
#define CON_GET_NEW_SEX			7
#define CON_GET_NEW_CLASS		8
#define CON_GET_ALIGNMENT		9
#define CON_DEFAULT_CHOICE		10
#define CON_GEN_GROUPS			11
#define CON_PICK_WEAPON			12
#define CON_GET_QUESTION		13
#define CON_READ_IMOTD			14
#define CON_READ_MOTD			15
#define CON_BREAK_CONNECT		16
#define CON_GET_CASES_1			17
#define CON_GET_CASES_2			18
#define CON_GET_CASES_3			19
#define CON_GET_CASES_4			20
#define CON_GET_CASES_5			21
#define CON_GET_COLOR			22
#define CON_GET_CODEPAGE		23
#define CON_GET_EMAIL			24
#define CON_GET_NUM_REGANSWER	25
#define CON_GET_ATHEIST			26

#define OUTBUF_SIZE  3 * MAX_STRING_LENGTH

/*
 * Descriptor (channel) structure.
 */
struct  descriptor_data
{
    DESCRIPTOR_DATA	*snoop_by;
    CHAR_DATA		*character;
    CHAR_DATA		*original;
    bool		valid;
    bool		mxp;
//#if defined(NOTHREAD)
//   char 		*Host;
//#else
   char		Host[4];
//#endif
    char		*ip;
    char		*sock;

    int			descriptor;

    int16_t		connected;
    bool		fcommand;
    char		inbuf[4 * MAX_INPUT_LENGTH];
    char		incomm[MAX_INPUT_LENGTH];
    char		inlast[MAX_INPUT_LENGTH];
    int			repeat;
    char		outbuf[OUTBUF_SIZE];
    int			outtop;
    char		*showstr_head;
    char		*showstr_point;
    void		*pEdit;    /* OLC */
    char		**pString;  /* OLC */
    int			editor;    /* OLC */

    int			codepage;
    int			idle_tics;

    /* mccp: support data */
    z_stream		*out_compress;
    unsigned char	*out_compress_buf;
    int16_t		mccp_version;

    char		grep[MIL];
    char		last_color;

    SLIST_ENTRY(descriptor_data) link;
};




/*
 * Attribute bonus structures.
 */
struct  str_app_type
{
    int16_t  tohit;
    int16_t  todam;
    int16_t  carry;
    int16_t  wield;
};

struct  int_app_type
{
    int16_t  learn;
};

struct  wis_app_type
{
    int16_t  practice;
};

struct  dex_app_type
{
    int16_t	tohit;
    int16_t	defensive;
};

struct  con_app_type
{
    int16_t  hitp;
    int16_t  shock;
};

#define ARTIFACT_MIN_VNUM 	500
#define ARTIFACT_MAX_VNUM 	599

/*
 * TO types for act.
 */
#define TO_ROOM		0
#define TO_NOTVICT	1
#define TO_VICT		2
#define TO_CHAR		3
#define TO_ALL		4
#define TO_SOUND		5

#define STRING_NORM "нормальный"
#define STRING_AGGR "атакующий"
#define STRING_CARE "оборонительный"

/*
 * Help table types.
 */
struct  help_data
{
    HELP_AREA	*help_area;
    int16_t	level;
    char	*keyword;
    char	*text;
    STAILQ_ENTRY(help_data) link;
    STAILQ_ENTRY(help_data) area_link;
};

struct help_area_data
{
  HELP_AREA	*next;
  AREA_DATA	*area;
  char		*filename;
  bool		changed;
  STAILQ_HEAD(, help_data) helps;
};

/*
 * Shop types.
 */
#define MAX_TRADE   5

struct  shop_data
{
    SHOP_DATA	*next;			/* Next shop in list		*/
    int		keeper;			/* Vnum of shop keeper mob	*/
    int16_t	buy_type[MAX_TRADE];	/* Item types shop will buy	*/
    int16_t	profit_buy;		/* Cost multiplier for buying	*/
    int16_t	profit_sell;		/* Cost multiplier for selling	*/
    int16_t	open_hour;		/* First opening hour		*/
    int16_t	close_hour;		/* First closing hour		*/
};



#define QUEST_OBJ	26
#define TO_NEXT_QUEST	5

#define QUEST_GET_OBJ		A
#define QUEST_KILL_MOB		B
#define QUEST_GROUP		C
#define QUEST_GROUP_UNIQUE		D

typedef struct name_list NAME_LIST;
typedef struct group_guest_data GROUP_QUEST_DATA;

struct name_list
{
    char	*name;
    NAME_LIST	*next;
    bool	valid;
};

struct group_guest_data
{
    GROUP_QUEST_DATA	*next;
    NAME_LIST		*names;
    int			time;
    bool		valid;
};

/*
 * Per-class stuff.
 */

#define MAX_STATS	5
#define STAT_STR	0
#define STAT_INT	1
#define STAT_WIS	2
#define STAT_DEX	3
#define STAT_CON	4

struct  class_type
{
    char	*filename;
    char	*name;			/* the full name of the class	*/
    char	*who_name;		/* Three-letter name for 'who'	*/
    int16_t	attr_prime;		/* Prime attribute		*/
    int16_t	attr_secondary;		/* Secondary attribute		*/
    int16_t	weapon;			/* First weapon			*/
    int		guild[MAX_PC_RACE];	/* Vnum of guild rooms		*/
    int16_t	skill_adept;		/* Maximum skill level		*/
    int16_t	thac0_00;		/* Thac0 for level  0		*/
    int16_t	thac0_32;		/* Thac0 for level 32		*/
    int16_t	hp_min;			/* Min hp gained on leveling	*/
    int16_t	hp_max;			/* Max hp gained on leveling	*/
    int16_t	fMana;			/* Class gains mana on level	*/
    char	*base_group;		/* base skills gained		*/
    char	*default_group;		/* default skills gained	*/
    int		valid_align;
    bool	changed;
};

struct flag_type
{
    char	*name;
    int64_t	bit;
    bool	settable;
    char	*rname;
};

struct material
{
    int		bit;
    float	bash;
    float	pierce;
    float	slash;
    float	other;
};


struct position_type
{
    char *name;
    char *short_name;
};

struct sex_type
{
    char *name;
    char *rname;
};

struct size_type
{
    char *name;
};

struct  bit_type
{
  const struct flag_type *table;
  char *help;
};

struct item_type
{
    int		type;
    char	*name;
    char	*rname;
};

struct weapon_type
{
    char	*name;
    char	*rname;
    int		vnum;
    int16_t	type;
    int16_t	*gsn;
    bool	can_select;
};

struct wiznet_type
{
    char	*name;
    int64_t	flag;
    int		level;
};

struct attack_type
{
    char *  name;      /* name */
    char *  noun;      /* message */
    char *  gender;      /* Gender of Damage message     */
    int16_t  obj_case;
    char *  msg_you;
    char *  msg_other;
    char *  msg_miss_you;
    char *  msg_miss_other;
    char *  miss_other_case;
    int     damage;      /* damage class */
};

struct race_type
{
    char *  filename;
    char *  name;      /* call name of the race */
    char *  rname;      /* call russian male name of the race */
    char *  fname;      /* call rus female name of the race */
    bool  pc_race;    /* can be chosen by pcs */
    int64_t  act;      /* act bits for the race */
    int64_t  aff;      /* aff bits for the race */
    int64_t  off;      /* off bits for the race */
#if 0
    int64_t  imm;      /* imm bits for the race */
    int64_t       res;      /* res bits for the race */
    int64_t  vuln;      /* vuln bits for the race */
#endif
    int16_t  resists[DAM_MAX];
    int64_t  form;      /* default form flag for the race */
    int64_t  parts;      /* default parts for the race */
    /* PC race data */
    char   *who_name;
    int16_t  points;      /* cost in points of the race */
    int16_t  *class_mult;    /* exp multiplier for class, * 100 */
    char *  skills[5];    /* bonus skills for the race */
    int16_t   stats[MAX_STATS];  /* starting stats */
    int16_t  max_stats[MAX_STATS];  /* maximum stats */
    int16_t  size;      /* aff bits for the race */
    int    recall;
    int    map_vnum;
    int    valid_align;
    bool  changed;
};


struct spec_type
{
    char *   name;      /* special function name */
    SPEC_FUN *  function;    /* the function */
    bool  can_cast;
};

/*
 * Data structure for notes.
 */

#define NOTE_NOTE	0
#define NOTE_IDEA	1
#define NOTE_PENALTY	2
#define NOTE_NEWS  	3
#define NOTE_CHANGES	4
#define NOTE_VOTES	5


struct char_votes
{
    long	char_id;
    CHAR_VOTES	*next;
    bool	valid;
}; 

struct vote_data
{
    VOTE_DATA	*next;
    char	*text;
    CHAR_VOTES	*votes;
    bool 	valid;
};

struct  note_data
{
    NOTE_DATA *  next;
    bool   valid;
    int16_t  type;
    char *  sender;
    char *  date;
    char *  to_list;
    char *  subject;
    char *  text;
    time_t    date_stamp;
    VOTE_DATA *vote;
};



/*
 * An affect.
 */
struct  affect_data
{
    AFFECT_DATA *  next;
    AFFECT_DATA *  prev;
    bool    valid;
    int16_t    where;
    int16_t    type;
    int16_t    level;
    int16_t    duration;
    int16_t    location;
    int16_t    modifier;
    int64_t    bitvector;
    long     caster_id;
};

/* where definitions */
#define TO_AFFECTS  0
#define TO_OBJECT  1
#define TO_IMMUNE  2
#define TO_RESIST  3
#define TO_VULN    4
#define TO_WEAPON  5
#define TO_ROOM_AFF  6
#define TO_PLR    7

/*
 * A kill structure (indexed by level).
 */
struct  kill_data
{
    int16_t    number;
    int16_t    killed;
};

#define NOUN  1
#define ADJECT  2

struct  except_data
{
    EXCEPT_DATA *next;
    int16_t  type;
    char   *cases[7];
};


/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (Start of section ... start here)                     *
 *                                                                         *
 ***************************************************************************/

/*
 * Well known mob virtual numbers.
 * Defined in #MOBILES.
 */
#define MOB_VNUM_FIDO       3090
#define MOB_VNUM_SAGE          9594

#define MOB_VNUM_PATROLMAN     2106
#define GROUP_VNUM_TROLLS     2100
#define GROUP_VNUM_OGRES     2101


/* RT ASCII conversions -- used so we can have letters in this file */

#define A  0x0000000000000001ULL
#define B  0x0000000000000002ULL
#define C  0x0000000000000004ULL
#define D  0x0000000000000008ULL
#define E  0x0000000000000010ULL
#define F  0x0000000000000020ULL
#define G  0x0000000000000040ULL
#define H  0x0000000000000080ULL
#define I  0x0000000000000100ULL
#define J  0x0000000000000200ULL
#define K  0x0000000000000400ULL
#define L  0x0000000000000800ULL
#define M  0x0000000000001000ULL
#define N  0x0000000000002000ULL
#define O  0x0000000000004000ULL
#define P  0x0000000000008000ULL
#define Q  0x0000000000010000ULL
#define R  0x0000000000020000ULL
#define S  0x0000000000040000ULL
#define T  0x0000000000080000ULL
#define U  0x0000000000100000ULL
#define V  0x0000000000200000ULL
#define W  0x0000000000400000ULL
#define X  0x0000000000800000ULL
#define Y  0x0000000001000000ULL
#define Z  0x0000000002000000ULL
#define aa  0x0000000004000000ULL
#define bb  0x0000000008000000ULL
#define cc  0x0000000010000000ULL
#define dd  0x0000000020000000ULL
#define ee  0x0000000040000000ULL
#define ff  0x0000000080000000ULL
#define gg  0x0000000100000000ULL
#define hh  0x0000000200000000ULL
#define ii  0x0000000400000000ULL
#define jj  0x0000000800000000ULL
#define kk  0x0000001000000000ULL
#define ll  0x0000002000000000ULL
#define mm  0x0000004000000000ULL
#define nn  0x0000008000000000ULL
#define oo  0x0000010000000000ULL
#define pp  0x0000020000000000ULL
#define qq  0x0000040000000000ULL
#define rr  0x0000080000000000ULL
#define ss  0x0000100000000000ULL
#define tt  0x0000200000000000ULL
#define uu  0x0000400000000000ULL
#define vv  0x0000800000000000ULL
#define ww  0x0001000000000000ULL
#define xx  0x0002000000000000ULL
#define yy  0x0004000000000000ULL
#define zz  0x0008000000000000ULL

/*
4503599627370496
9007199254740992
18014398509481984
36028797018963968
72057594037927936
144115188075855872
288230376151711744
576460752303423488
1152921504606846976
2305843009213693952
4611686018427387904
*/


extern bool too_many_victims;

/*
 * ACT bits for mobs.
 * Used in #MOBILES.
 */
#define ACT_IS_NPC    (A)  /* Auto set for mobs  */
#define ACT_SENTINEL        (B)  /* Stays in one room  */
#define ACT_SCAVENGER          (C)  /* Picks up objects  */
#define ACT_SMITHER    (D)
#define ACT_MOUNT    (E)
#define ACT_AGGRESSIVE    (F)      /* Attacks PC's    */
#define ACT_STAY_AREA    (G)  /* Won't leave area  */
#define ACT_WIMPY    (H)
#define ACT_PET      (I)  /* Auto set for pets  */
#define ACT_TRAIN    (J)  /* Can train PC's  */
#define ACT_PRACTICE    (K)  /* Can practice PC's  */
#define ACT_NOEXP    (L)
#define ACT_NOREMEMBER    (M)  /* NPC doesn't remember aggressor */
#define ACT_UNDEAD    (O)
#define ACT_NOLEGEND    (P)
#define ACT_CLERIC    (Q)
#define ACT_MAGE    (R)
#define ACT_THIEF    (S)
#define ACT_WARRIOR    (T)
#define ACT_NOALIGN    (U)
#define ACT_NOPURGE    (V)
#define ACT_OUTDOORS    (W)
#define ACT_HUNTER    (X)
#define ACT_INDOORS    (Y)
#define ACT_DEAD_WARRIOR  (Z)
#define ACT_IS_HEALER    (aa)
#define ACT_GAIN    (bb)
#define ACT_UPDATE_ALWAYS  (cc)
#define ACT_IS_CHANGER    (dd)
#define ACT_BANKER    (ee)
#define ACT_ANIMAL    (ff)
#define ACT_QUESTER    (gg)
#define ACT_AROSH		(hh) /*зодчий - строитель домов*/
#define ACT_EXECUTIONER (ii)
#define ACT_WIZINVIS	(jj)
#define ACT_INLUA	(kk)	/* For internal use only */


/* OFF bits for mobiles */
#define OFF_AREA_ATTACK         (A)
#define OFF_BACKSTAB            (B)
#define OFF_BASH                (C)
#define OFF_BERSERK             (D)
#define OFF_DISARM              (E)
#define OFF_DODGE               (F)
#define OFF_FAST                (H)
#define OFF_KICK                (I)
#define OFF_KICK_DIRT           (J)
#define OFF_PARRY               (K)
#define OFF_RESCUE              (L)
#define OFF_TAIL                (M)
#define OFF_TRIP                (N)
#define ASSIST_ALL              (P)
#define ASSIST_ALIGN            (Q)
#define ASSIST_RACE             (R)
#define ASSIST_PLAYERS          (S)
#define ASSIST_CLAN             (T)
#define ASSIST_VNUM             (U)
#define OFF_AMBUSH              (V)
#define OFF_CLEAVE              (W)



/* return values for check_imm */
#define IS_NORMAL    0
#define IS_IMMUNE    1
#define IS_RESISTANT    2
#define IS_VULNERABLE    3

/* IMM bits for mobs */
#define IMM_SUMMON              (A)
#define IMM_CHARM               (B)
#define IMM_MAGIC               (C)
#define IMM_WEAPON              (D)
#define IMM_BASH                (E)
#define IMM_PIERCE              (F)
#define IMM_SLASH               (G)
#define IMM_FIRE                (H)
#define IMM_COLD                (I)
#define IMM_LIGHTNING           (J)
#define IMM_ACID                (K)
#define IMM_POISON              (L)
#define IMM_NEGATIVE            (M)
#define IMM_HOLY                (N)
#define IMM_ENERGY              (O)
#define IMM_MENTAL              (P)
#define IMM_DISEASE             (Q)
#define IMM_DROWNING            (R)
#define IMM_LIGHT    (S)
#define IMM_SOUND    (T)
#define IMM_WOOD                (X)
#define IMM_SILVER              (Y)
#define IMM_IRON                (Z)

/* RES bits for mobs */
#define RES_SUMMON    (A)
#define RES_CHARM    (B)
#define RES_MAGIC               (C)
#define RES_WEAPON              (D)
#define RES_BASH                (E)
#define RES_PIERCE              (F)
#define RES_SLASH               (G)
#define RES_FIRE                (H)
#define RES_COLD                (I)
#define RES_LIGHTNING           (J)
#define RES_ACID                (K)
#define RES_POISON              (L)
#define RES_NEGATIVE            (M)
#define RES_HOLY                (N)
#define RES_ENERGY              (O)
#define RES_MENTAL              (P)
#define RES_DISEASE             (Q)
#define RES_DROWNING            (R)
#define RES_LIGHT    (S)
#define RES_SOUND    (T)
#define RES_WOOD                (X)
#define RES_SILVER              (Y)
#define RES_IRON                (Z)

/* VULN bits for mobs */
#define VULN_SUMMON    (A)
#define VULN_CHARM    (B)
#define VULN_MAGIC              (C)
#define VULN_WEAPON             (D)
#define VULN_BASH               (E)
#define VULN_PIERCE             (F)
#define VULN_SLASH              (G)
#define VULN_FIRE               (H)
#define VULN_COLD               (I)
#define VULN_LIGHTNING          (J)
#define VULN_ACID               (K)
#define VULN_POISON             (L)
#define VULN_NEGATIVE           (M)
#define VULN_HOLY               (N)
#define VULN_ENERGY             (O)
#define VULN_MENTAL             (P)
#define VULN_DISEASE            (Q)
#define VULN_DROWNING           (R)
#define VULN_LIGHT    (S)
#define VULN_SOUND    (T)
#define VULN_WOOD               (X)
#define VULN_SILVER             (Y)
#define VULN_IRON    (Z)

/* body form */
#define FORM_EDIBLE             (A)
#define FORM_POISON             (B)
#define FORM_MAGICAL            (C)
#define FORM_INSTANT_DECAY      (D)
#define FORM_OTHER              (E)  /* defined by material bit */

/* actual form */
#define FORM_ANIMAL             (G)
#define FORM_SENTIENT           (H)
#define FORM_UNDEAD             (I)
#define FORM_CONSTRUCT          (J)
#define FORM_MIST               (K)
#define FORM_INTANGIBLE         (L)

#define FORM_BIPED              (M)
#define FORM_CENTAUR            (N)
#define FORM_INSECT             (O)
#define FORM_SPIDER             (P)
#define FORM_CRUSTACEAN         (Q)
#define FORM_WORM               (R)
#define FORM_BLOB    		(S)

#define FORM_MAMMAL             (V)
#define FORM_BIRD               (W)
#define FORM_REPTILE            (X)
#define FORM_SNAKE              (Y)
#define FORM_DRAGON             (Z)
#define FORM_AMPHIBIAN          (aa)
#define FORM_FISH               (bb)
#define FORM_COLD_BLOOD    	(cc)
#define FORM_TREE	    	(dd)
#define FORM_BLOODLESS		(ee)
#define FORM_HAS_BLOOD		(ff)

/* body parts */
#define PART_HEAD               (A)
#define PART_ARMS               (B)
#define PART_LEGS               (C)
#define PART_HEART              (D)
#define PART_BRAINS             (E)
#define PART_GUTS               (F)
#define PART_HANDS              (G)
#define PART_FEET               (H)
#define PART_FINGERS            (I)
#define PART_EAR                (J)
#define PART_EYE		(K)
#define PART_LONG_TONGUE        (L)
#define PART_EYESTALKS          (M)
#define PART_TENTACLES          (N)
#define PART_FINS               (O)
#define PART_WINGS              (P)
#define PART_TAIL               (Q)
/* for combat */
#define PART_CLAWS              (U)
#define PART_FANGS              (V)
#define PART_HORNS              (W)
#define PART_SCALES             (X)
#define PART_TUSKS    		(Y)
#define PART_SKIN		(Z)


/*
 * Bits for 'affected_by'.
 * Used in #MOBILES.
 */
#define AFF_BLIND    		    (A)
#define AFF_INVISIBLE    	    (B)
#define AFF_DETECT_EVIL    	    (C)
#define AFF_DETECT_INVIS  	    (D)
#define AFF_DETECT_MAGIC  	    (E)
#define AFF_DETECT_HIDDEN  	    (F)
#define AFF_DETECT_GOOD    	    (G)
#define AFF_SANCTUARY    	    (H)
#define AFF_FAERIE_FIRE    	    (I)
#define AFF_INFRARED    	    (J)
#define AFF_CURSE    		    (K)
#define AFF_FIRE_SHIELD    	    (L)
#define AFF_POISON    		    (M)
#define AFF_PROTECT_EVIL  	    (N)
#define AFF_PROTECT_GOOD  	    (O)
#define AFF_SNEAK    		    (P)
#define AFF_HIDE    		    (Q)
#define AFF_SLEEP    		    (R)
#define AFF_CHARM    		    (S)
#define AFF_FLYING    		    (T)
#define AFF_PASS_DOOR    	    (U)
#define AFF_HASTE    		    (V)
#define AFF_CALM    		    (W)
#define AFF_PLAGUE    		    (X)
#define AFF_WEAKEN    		    (Y)
#define AFF_DARK_VISION    	    (Z)
#define AFF_BERSERK    		    (aa)
#define AFF_SWIM    		    (bb)
#define AFF_REGENERATION        (cc)
#define AFF_SLOW    		    (dd)
#define AFF_DETECT_UNDEAD  	    (ee)
#define AFF_ICE_SHIELD    	    (ff)    //0x0000000080000000ULL
#define AFF_CAMOUFLAGE    	    (gg)
#define AFF_CAMOUFLAGE_MOVE  	(hh)
#define AFF_DETECT_CAMOUFLAGE   (ii)
#define AFF_SATIETY    		    (jj)
#define AFF_DEATH_AURA    	    (kk)
#define AFF_COVER               (ll)
#define AFF_DETECT_LIFE    	    (mm)
#define AFF_ROOTS    		    (nn)
#define AFF_GODS_SKILL    	    (oo)
/* #define AFF_INSTANT             (pp) */
#define AFF_HOLY_SHIELD         (rr)
#define AFF_EVIL_AURA           (ss)
#define AFF_CONIFEROUS_SHIELD   (tt)

/*
 * Sex.
 * Used in #MOBILES.
 */
#define SEX_NEUTRAL          0
#define SEX_MALE          1
#define SEX_FEMALE          2
#define SEX_MANY          3

#define MAX_PC_SEX          2
#define MAX_NPC_SEX          3

/* AC types */
#define AC_PIERCE      0
#define AC_BASH        1
#define AC_SLASH      2
#define AC_EXOTIC      3

/* dice */
#define DICE_NUMBER      0
#define DICE_TYPE      1
#define DICE_BONUS      2

/* size */
#define SIZE_TINY      0
#define SIZE_SMALL      1
#define SIZE_MEDIUM      2
#define SIZE_LARGE      3
#define SIZE_HUGE      4
#define SIZE_GIANT      5



/*
 * Well known object virtual numbers.
 * Defined in #OBJECTS.
 */
#define OBJ_VNUM_SILVER_ONE     1
#define OBJ_VNUM_GOLD_ONE       2
#define OBJ_VNUM_GOLD_SOME      3
#define OBJ_VNUM_SILVER_SOME    4
#define OBJ_VNUM_COINS          5

#define OBJ_VNUM_CORPSE_NPC     10
#define OBJ_VNUM_CORPSE_PC      11
#define OBJ_VNUM_SEVERED_HEAD   12
#define OBJ_VNUM_TORN_HEART     13
#define OBJ_VNUM_SLICED_ARM     14
#define OBJ_VNUM_SLICED_LEG     15
#define OBJ_VNUM_GUTS         	16
#define OBJ_VNUM_BRAINS         17
#define OBJ_VNUM_TAIL         	19
#define OBJ_VNUM_EAR			31
#define OBJ_VNUM_KLYK			33
#define OBJ_VNUM_PAD			34
#define OBJ_VNUM_MARRY_RING		36

#define OBJ_VNUM_CARPET              32

#define OBJ_VNUM_MUSHROOM       20
#define OBJ_VNUM_LIGHT_BALL       21
#define OBJ_VNUM_SPRING         22
#define OBJ_VNUM_DISC         23
#define OBJ_VNUM_PORTAL         25
#define OBJ_VNUM_KLYK			33
#define OBJ_VNUM_ROSE       	35
#define OBJ_VNUM_SPRING_RANGER  37

#define OBJ_VNUM_BRONZE_ONE     45
#define OBJ_VNUM_BRONZE_SOME    46

/* #define OBJ_VNUM_ROSE       1001*/

#define OBJ_VNUM_PIT       3010

#define OBJ_VNUM_SCHOOL_MACE     3700
#define OBJ_VNUM_SCHOOL_DAGGER     3701
#define OBJ_VNUM_SCHOOL_SWORD     3702
#define OBJ_VNUM_SCHOOL_SPEAR     3717
#define OBJ_VNUM_SCHOOL_STAFF     3718
#define OBJ_VNUM_SCHOOL_AXE     3719
#define OBJ_VNUM_SCHOOL_FLAIL     3720
#define OBJ_VNUM_SCHOOL_WHIP     3721
#define OBJ_VNUM_SCHOOL_POLEARM    3722

#define OBJ_VNUM_SCHOOL_VEST     3703
#define OBJ_VNUM_SCHOOL_SHIELD     3704
#define OBJ_VNUM_SCHOOL_BANNER     3716
#define OBJ_VNUM_SCHOOL_BOOTS      3708
#define OBJ_VNUM_MAP       3162

#define OBJ_VNUM_WHISTLE     2116
#define OBJ_VNUM_ALCHEMIST_PATRONTASH	1226

#define OBJ_VNUM_BEACON          6
#define OBJ_VNUM_NAZGUL_AURA     7
#define OBJ_VNUM_LIGHTNING       38

#define OBJ_VNUM_POTION		 39
#define OBJ_VNUM_MORTAR		 40
#define OBJ_VNUM_PILL		 41
#define OBJ_VNUM_CLOUD		 42
#define OBJ_VNUM_CLAW            43
#define OBJ_VNUM_SIMBOL		 44

/*
 * CLOUD NAME
 */

#define CLOUD_POISON		 1
#define CLOUD_ACID		 2
#define CLOUD_FIRE		 3


/*
 * Item types.
 * Used in #OBJECTS.
 */
#define ITEM_LIGHT          1
#define ITEM_SCROLL          2
#define ITEM_WAND          3
#define ITEM_STAFF          4
#define ITEM_WEAPON          5
#define ITEM_TREASURE          8
#define ITEM_ARMOR          9
#define ITEM_POTION         10
#define ITEM_CLOTHING         11
#define ITEM_FURNITURE         12
#define ITEM_TRASH         13
#define ITEM_CONTAINER         15
#define ITEM_DRINK_CON         17
#define ITEM_KEY         18
#define ITEM_FOOD         19
#define ITEM_MONEY         20
#define ITEM_BOAT         22
#define ITEM_CORPSE_NPC         23
#define ITEM_CORPSE_PC         24
#define ITEM_FOUNTAIN         25
#define ITEM_PILL         26


#define ITEM_PROTECT         27 /* ?????? */


#define ITEM_MAP         28
#define ITEM_PORTAL         29
#define ITEM_WARP_STONE         30
#define ITEM_ROOM_KEY         31
#define ITEM_GEM         32
#define ITEM_JEWELRY         33
#define ITEM_JUKEBOX         34
#define ITEM_ROD         35
#define ITEM_TRAP         36
#define ITEM_SCALE         37
#define ITEM_ARTIFACT         38
#define ITEM_BOOK         39
#define ITEM_CHEST         40
#define ITEM_MORTAR         41
#define ITEM_INGREDIENT         42
#define MAX_ITEM_TYPE         42

/*
 * Material types
 */
#define  MAT_STEEL  1
#define  MAT_STONE  2
#define  MAT_BRASS  3
#define  MAT_BONE  4
#define  MAT_ENERGY  5
#define  MAT_MITHRIL  6
#define  MAT_COPPER  7
#define  MAT_SILK  8
#define  MAT_MARBLE  9
#define  MAT_GLASS  10
#define  MAT_WATER  11
#define  MAT_FLESH  12
#define  MAT_PLATINUM  13
#define  MAT_GRANITE  14
#define  MAT_LEATHER  15
#define  MAT_CLOTH  16
#define  MAT_GEMSTONE  17
#define  MAT_GOLD  18
#define  MAT_PORCELAIN  19
#define  MAT_OBSIDIAN  20
#define  MAT_DRAGONSCALE  21
#define  MAT_EBONY  22
#define  MAT_BRONZE  23
#define  MAT_WOOD  24
#define  MAT_SILVER  25
#define  MAT_IRON  26
#define  MAT_BLOODSTONE  27
#define  MAT_FOOD  28
#define  MAT_LEAD  29
#define  MAT_WAX    30
#define  MAT_DIAMOND  31
#define  MAT_OLDSTYLE  32
#define  MAT_ICE  33
#define  MAT_LOAM  34

/*
 * Extra flags.
 * Used in #OBJECTS.
 */
#define ITEM_GLOW    (A)
#define ITEM_HUM    (B)

#define ITEM_DARK    (C) /*  ????????? Занакой нужно? */
#define ITEM_LOCK    (D) /*                           */

#define ITEM_EVIL    (E)
#define ITEM_INVIS    (F)
#define ITEM_MAGIC    (G)
#define ITEM_NODROP    (H)
#define ITEM_BLESS    (I)
#define ITEM_ANTI_GOOD          (J) /* to delete */
#define ITEM_ANTI_EVIL          (K) /* to delete */
#define ITEM_ANTI_NEUTRAL       (L) /* to delete */
#define ITEM_NOREMOVE    (M)
#define ITEM_INVENTORY    (N)
#define ITEM_NOPURGE    (O)
#define ITEM_ROT_DEATH    (P)
#define ITEM_VIS_DEATH    (Q)
#define ITEM_FORQUEST       (R)
#define ITEM_NONMETAL    (S)
#define ITEM_NOLOCATE    (T)
#define ITEM_MELT_DROP    (U)
#define ITEM_HAD_TIMER    (V)
#define ITEM_SELL_EXTRACT  (W)
#define ITEM_ANTI_MAGE          (X) /* to delete */
#define ITEM_BURN_PROOF    (Y)
#define ITEM_NOUNCURSE    (Z)
#define ITEM_AGGRAVATE    (aa)
#define ITEM_TELEPORT    (bb)
#define ITEM_SLOW_DIGESTION  (cc)
#define ITEM_ANTI_CLERIC        (dd) /* to delete */
#define ITEM_ANTI_THIEF         (ee) /* to delete */
#define ITEM_ANTI_WARRIOR       (ff) /* to delete */
#define ITEM_ANTI_NECROS        (gg) /* to delete */
#define ITEM_ANTI_PALADIN       (hh) /* to delete */
#define ITEM_ANTI_NAZGUL        (ii) /* to delete */
#define ITEM_ANTI_DRUID         (jj) /* to delete */
#define ITEM_ANTI_RANGER        (kk) /* to delete */
#define ITEM_ANTI_VAMPIRE       (ll) /* to delete */

#define ITEM_NO_PERSONALIZE	(mm)

#define ITEM_AUCTION    	(qq)
#define ITEM_NO_IDENTIFY  	(rr)
#define ITEM_FOR_MAN            (ss) /* to delete */
#define ITEM_FOR_WOMAN          (tt) /* to delete */
#define ITEM_HAS_SOCKET		(uu)
#define ITEM_NO_LEGEND		(vv)
#define ITEM_NOT_DIGGING	(ww)
#define ITEM_NO_SAC		(xx)
#define ITEM_QUITDROP		(yy)
#define ITEM_NOT_PUTABLE	(zz)

/* Uncomfortable and unusable flags */
/* Align */
#define FOR_GOOD    (A)
#define FOR_NEUTRAL    (B)
#define FOR_EVIL    (C)
/* Sex */
#define FOR_MAN      (D)
#define FOR_WOMAN    (E)
/* Class */
#define FOR_MAGE    (F)
#define FOR_CLERIC    (G)
#define FOR_THIEF    (H)
#define FOR_WARRIOR    (I)
#define FOR_NECROMANCER    (J)
#define FOR_PALADIN    (K)
#define FOR_NAZGUL    (L)
#define FOR_DRUID    (M)
#define FOR_RANGER    (N)
#define FOR_VAMPIRE    (O) /* Race & class */
/* (P) - (S) reserved for future use */
/* Race */
#define FOR_HUMAN    (T)
#define FOR_ELF      (U)
#define FOR_DWARF    (V)
#define FOR_GIANT    (W)
#define FOR_ORC      (X)
#define FOR_TROLL    (Y)
#define FOR_SNAKE    (Z)
#define FOR_HOBBIT    (aa)
#define FOR_DROW    (bb)
#define FOR_ALCHEMIST    (cc)
#define FOR_LYCANTHROPE    (dd)
#define FOR_ZOMBIE    (ee)
#define FOR_TIGGUAN    (ff)

/*
 * Wear flags.
 * Used in #OBJECTS.
 */
#define ITEM_TAKE    (A)
#define ITEM_WEAR_FINGER  (B)
#define ITEM_WEAR_NECK    (C)
#define ITEM_WEAR_BODY    (D)
#define ITEM_WEAR_HEAD    (E)
#define ITEM_WEAR_LEGS    (F)
#define ITEM_WEAR_FEET    (G)
#define ITEM_WEAR_HANDS    (H)
#define ITEM_WEAR_ARMS    (I)
#define ITEM_WEAR_SHIELD  (J)
#define ITEM_WEAR_ABOUT    (K)
#define ITEM_WEAR_WAIST    (L)
#define ITEM_WEAR_WRIST    (M)
#define ITEM_WIELD    (N)
#define ITEM_HOLD    (O)
#define ITEM_WEAR_FLOAT    (Q)
#define ITEM_WEAR_AT_SHOULDER	(R)

/* weapon class */
#define WEAPON_SWORD    1
#define WEAPON_DAGGER    2
#define WEAPON_SPEAR    3
#define WEAPON_MACE    4
#define WEAPON_AXE    5
#define WEAPON_FLAIL    6
#define WEAPON_WHIP    7
#define WEAPON_POLEARM    8
#define WEAPON_STAFF    9
#define WEAPON_EXOTIC    10
#define MAX_WEAPON_TYPE    11

/* weapon types */
#define WEAPON_FLAMING    (A)
#define WEAPON_FROST    (B)
#define WEAPON_VAMPIRIC    (C)
#define WEAPON_SHARP    (D)
#define WEAPON_TWO_HANDS  (F)
#define WEAPON_SHOCKING    (G)
#define WEAPON_POISON    (H)
#define WEAPON_MANAPIRIC  (J)
#define WEAPON_SLAY_EVIL  (K)
#define WEAPON_SLAY_GOOD  (L)
#define WEAPON_SLAY_ANIMAL  (M)
#define WEAPON_SLAY_UNDEAD  (N)
#define WEAPON_PRIMARY_ONLY  (O)
#define WEAPON_SECONDARY_ONLY   (P)

/* gate flags */
#define GATE_NORMAL_EXIT  (A)
#define GATE_NOCURSE    (B)
#define GATE_GOWITH    (C)
#define GATE_BUGGY    (D)
#define GATE_RANDOM    (E)
#define GATE_UNDEAD    (F)

/* furniture flags */
#define STAND_AT    (A)
#define STAND_ON    (B)
#define STAND_IN    (C)
#define SIT_AT      (D)
#define SIT_ON      (E)
#define SIT_IN      (F)
#define REST_AT      (G)
#define REST_ON      (H)
#define REST_IN      (I)
#define SLEEP_AT    (J)
#define SLEEP_ON    (K)
#define SLEEP_IN    (L)
#define PUT_AT      (M)
#define PUT_ON      (N)
#define PUT_IN      (O)
#define PUT_INSIDE    (P)




/*
 * Apply types (for affects).
 * Used in #OBJECTS.
 */
#define APPLY_NONE          0
#define APPLY_STR          1
#define APPLY_DEX          2
#define APPLY_INT          3
#define APPLY_WIS          4
#define APPLY_CON          5
#define APPLY_SEX          6
#define APPLY_SKILL           7
#define APPLY_MANA         12
#define APPLY_HIT         13
#define APPLY_MOVE         14
#define APPLY_AC         17
#define APPLY_HITROLL         18
#define APPLY_DAMROLL         19
#define APPLY_SAVES         20
#define APPLY_SAVING_ROD       21
#define APPLY_SAVING_PETRI       22
#define APPLY_SAVING_BREATH       23
#define APPLY_SAVING_SPELL       24
#define APPLY_SPELL_AFFECT       25


#define ROOM_APPLY_SECTOR       1
#define ROOM_APPLY_FLAGS       2

/*
 * Values for containers (value[1]).
 * Used in #OBJECTS.
 */
#define CONT_CLOSEABLE          1
#define CONT_PICKPROOF          2
#define CONT_CLOSED          4
#define CONT_LOCKED          8
#define CONT_PUT_ON         16


/*
 * Values for ingredients (value[1]).
 * Used in #OBJECTS.
 */
#define ING_SECT_INSIDE		(A)
#define ING_SECT_CITY		(B)
#define ING_SECT_FIELD         	(C)
#define ING_SECT_FOREST        	(D)
#define ING_SECT_HILLS         	(E)
#define ING_SECT_MOUNTAIN     	(F)
#define ING_SECT_WATER_SWIM     (G)
#define ING_SECT_WATER_NOSWIM  	(H)
#define ING_SECT_UNUSED        	(I)
#define ING_SECT_AIR          	(J)
#define ING_SECT_DESERT        	(K)
#define ING_SECT_IN_WATER	(L)

/*
 * Values for ingredients (value[3]).
 * Used in #OBJECTS.
 */
#define ING_SKY_CLOUDLESS	(A)
#define ING_SKY_CLOUDY		(B)
#define ING_SKY_RAINING		(C)
#define ING_SKY_LIGHTNING	(D)

/*
 * Well known room virtual numbers.
 * Defined in #ROOMS.
 */
#define ROOM_VNUM_LIMBO          2
#define ROOM_VNUM_CHAT       1200
#define ROOM_VNUM_TEMPLE     3001
#define ROOM_VNUM_ALTAR       3054
#define ROOM_VNUM_ARENA      10
#define ROOM_VNUM_SCHOOL      3700

#define ROOM_VNUM_HOUSE			28000

/*
 * Room flags.
 * Used in #ROOMS.
 */
#define ROOM_DARK    		(A)
#define ROOM_SILENCE    	(B)
#define ROOM_NO_MOB    		(C)
#define ROOM_INDOORS    	(D)

#define ROOM_NOTRACK    	(E)
#define ROOM_NOGATE    		(F)

#define ROOM_PRIVATE    	(J)
#define ROOM_SAFE    		(K)
#define ROOM_SOLITARY    	(L)
#define ROOM_PET_SHOP    	(M)
#define ROOM_NO_RECALL    	(N)

/* Deprecated flags, replaced by room->min_level, room->max->level */
#define ROOM_IMP_ONLY    	(O)
#define ROOM_GODS_ONLY    	(P)
#define ROOM_HEROES_ONLY  	(Q)
#define ROOM_NEWBIES_ONLY  	(R)
/*******************************************************************/

#define ROOM_LAW    		(S)
#define ROOM_NOWHERE    	(T)

#define ROOM_MOUNT_SHOP    	(U)
#define ROOM_SLOW_DT    	(V)
#define ROOM_NOLEGEND    	(W)
#define ROOM_BANK    		(X)
#define ROOM_BOUNTY_OFFICE  	(Y)
#define ROOM_NOMAGIC            (Z)
#define ROOM_NOFLY_DT    	(aa)
#define ROOM_NOLOOT    		(bb)
#define ROOM_NOEXP_LOST    	(cc)
#define ROOM_NO_FLY		(dd) /* флаг "не_летать" */
#define ROOM_KILL    		(ee)
#define ROOM_ARENA              (ff)
#define ROOM_ARENA_MONITOR      (gg)
#define ROOM_HOUSE		(hh) /* флаг "дом чара" */
#define ROOM_NOSLEEP    	(ii)
#define ROOM_NOSIT    		(jj)
#define ROOM_CAN_BUILD_HOUSE	(kk) /* флаг 'можно строить дом' */
#define ROOM_NORUN		(ll)
#define ROOM_NOAUCTION		(mm)
#define ROOM_HOLY    		(nn) /*святые комнаты (храмы)*/
#define ROOM_GUILD    		(oo) /*гильдии*/
#define ROOM_PRISON		(pp) /*тюрьмы*/
#define ROOM_NOQUEST		(qq) /*квесты*/


/*
 * Directions.
 * Used in #ROOMS.
 */
#define DIR_NORTH          0
#define DIR_EAST          1
#define DIR_SOUTH          2
#define DIR_WEST          3
#define DIR_UP            4
#define DIR_DOWN          5
#define MAX_DIR            6



/*
 * Exit flags.
 * Used in #ROOMS.
 */
#define EX_ISDOOR          (A)
#define EX_CLOSED          (B)
#define EX_LOCKED          (C)
#define EX_PICKPROOF          (F)
#define EX_NOPASS          (G)
#define EX_EASY            (H)
#define EX_HARD            (I)
#define EX_INFURIATING          (J)
#define EX_NOCLOSE          (K)
#define EX_NOLOCK          (L)
#define EX_RACE                       (M)



/*
 * Sector types.
 * Used in #ROOMS.
 */
#define SECT_INSIDE          0
#define SECT_CITY          1
#define SECT_FIELD          2
#define SECT_FOREST          3
#define SECT_HILLS          4
#define SECT_MOUNTAIN          5
#define SECT_WATER_SWIM          6
#define SECT_WATER_NOSWIM        7
#define SECT_UNUSED          8
#define SECT_AIR          9
#define SECT_DESERT         10
#define SECT_IN_WATER		11
#define SECT_MAX         12



/*
 * Equpiment wear locations.
 * Used in #RESETS.
 */

#define WEAR_ANYWHERE     -3
#define WEAR_WEAR         -2
#define WEAR_NONE         -1
#define WEAR_LIGHT          0
#define WEAR_FINGER_L          1
#define WEAR_FINGER_R          2
#define WEAR_NECK_1          3
#define WEAR_NECK_2          4
#define WEAR_BODY          5
#define WEAR_HEAD          6
#define WEAR_LEGS          7
#define WEAR_FEET          8
#define WEAR_HANDS          9
#define WEAR_ARMS         10
#define WEAR_SHIELD         11
#define WEAR_ABOUT         12
#define WEAR_WAIST         13
#define WEAR_WRIST_L         14
#define WEAR_WRIST_R         15
#define WEAR_WIELD         16
#define WEAR_HOLD         17
#define WEAR_SECONDARY         18
#define WEAR_FLOAT         19
#define WEAR_AT_SHOULDER         20
#define MAX_WEAR         21



#define MAX_GROUP_LEVEL         1
#define MIN_GROUP_LEVEL         2
#define AVG_GROUP_LEVEL         3
#define PK_RANGE         8

/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (End of this section ... stop here)                   *
 *                                                                         *
 ***************************************************************************/

/*
 * Conditions.
 */
#define COND_DRUNK          0
#define COND_FULL          1
#define COND_THIRST          2
#define COND_HUNGER          3

#define DRUNK_LEVEL	15


/*
 * Positions.
 */
#define POS_DEAD   	0
#define POS_MORTAL      1
#define POS_INCAP       2
#define POS_STUNNED     3
#define POS_SLEEPING    4
#define POS_RESTING     5
#define POS_SITTING     6
#define POS_BASHED      7  /* Position after bash/trip/tail/etc */
#define POS_FIGHTING    8
#define POS_STANDING    9



/*
 * ACT bits for players.
 */
#define PLR_IS_NPC    (A)    /* Don't EVER set.  */

/* RT auto flags */
#define PLR_EXCITED    (B)
#define PLR_AUTOASSIST    (C)
#define PLR_AUTOEXIT    (D)
#define PLR_AUTOLOOT    (E)
#define PLR_AUTOSAC             (F)
#define PLR_AUTOMONEY    (G)
#define PLR_SHOW_DAMAGE    (I)
#define PLR_NOIMMQUEST    (J)
#define PLR_AUTOSPLIT    (H)
#define PLR_NOCANCEL    (K)
#define PLR_AUTOTITLE    (L)
#define PLR_NOCAST    (M)

/* RT personal flags */
#define PLR_HOLYLIGHT    (N)
#define PLR_NOEXP    (O)
#define PLR_CANLOOT    (P)
#define PLR_NOSUMMON    (Q)
#define PLR_NOFOLLOW    (R)
#define PLR_COLOUR    (T)
/* 1 bit reserved, S */

/* penalty flags */
#define PLR_PERMIT    (U)
#define PLR_LOG_OUTPUT          (V)
#define PLR_LOG      (W)
#define PLR_DENY    (X)
#define PLR_FREEZE    (Y)
#define PLR_THIEF    (Z)
#define PLR_KILLER    (aa)
#define PLR_QUESTING     (bb)
#define PLR_CONSENT    (cc)
#define PLR_NODREAMS    (dd)

/* autoflags part N.2 */
#define PLR_AUTOGOLD    (ee)
#define PLR_MAPEXIT    (ff)

/* misc flags */
#define PLR_CHALLENGER          (gg) /* Флаг участия в боях без правил */
#define PLR_FULL_SILENCE		(hh) /* флаг "полное молчание */
#define PLR_CONFIRM_DELETE      (ii)
#define PLR_NOPK				(jj) /* флаг ноу_ПК */
#define PLR_AUTOATTACK			(kk) /*флаг разрешения атаки на чаров, защита от случайных нападений*/

/*
 * #define PLR_OLD_COMPENSATION		(mm)
 * #define PLR_OLD_COMPENSATION2		(ll)
 * #define PLR_OLD_COMPENSATION3		(nn)
 * #define PLR_COMPENSATION		(oo)
 * #define PLR_NEW_YEAR_2013                (pp)
 */



/* RT comm flags -- may be used on both mobs and chars */
#define COMM_QUIET              (A)
#define COMM_DEAF              (B)
#define COMM_NOWIZ              (C)
#define COMM_NOAUCTION          (D)
#define COMM_NOGOSSIP           (E)
#define COMM_NOQUESTION         (F)
#define COMM_NOMUSIC            (G)
#define COMM_NOCLAN    (H)
#define COMM_NOQUOTE    (I)
#define COMM_SHOUTSOFF    (J)
#define COMM_NOOOC    (K)

/* display flags */
#define COMM_COMPACT    (L)
#define COMM_BRIEF    (M)
#define COMM_PROMPT    (N)
#define COMM_COMBINE    (O)
#define COMM_TELNET_GA    (P)
#define COMM_SHOW_AFFECTS  (Q)
#define COMM_NOGRATS    (R)

/* penalties */
#define COMM_NOEMOTE    (T)
#define COMM_NOSHOUT    (U)
#define COMM_NOTELL    (V)
#define COMM_NOCHANNELS    (W)
#define COMM_SNOOP_PROOF  (Y)
#define COMM_AFK    (Z)

#define COMM_NOBUILD    (aa)  /* BTalk */

/* penalties again */
#define COMM_NONOTES    (bb)
#define COMM_NOTITLE    (cc)
#define COMM_HELPER    (dd)

#define COMM_NOTEST    (ee)  /* TTalk */
#define COMM_NORACE		(ff) /*rtalk*/
#define COMM_EXTRANOCHANNELS    (gg)
#define COMM_NOSLANG    (hh)


/* WIZnet flags */
#define WIZ_ON      (A)
#define WIZ_TICKS    (B)
#define WIZ_LOGINS    (C)
#define WIZ_SITES    (D)
#define WIZ_LINKS    (E)
#define WIZ_DEATHS    (F)
#define WIZ_RESETS    (G)
#define WIZ_MOBDEATHS    (H)
#define WIZ_FLAGS    (I)
#define WIZ_PENALTIES    (J)
#define WIZ_SACCING    (K)
#define WIZ_LEVELS    (L)
#define WIZ_SECURE    (M)
#define WIZ_SWITCHES    (N)
#define WIZ_SNOOPS    (O)
#define WIZ_RESTORE    (P)
#define WIZ_LOAD    (Q)
#define WIZ_NEWBIE    (R)
#define WIZ_PREFIX    (S)
#define WIZ_SPAM    (T)
#define WIZ_BUGS    (U)
#define WIZ_MEMCHECK    (V)
#define WIZ_LOG      (W)


struct track_data
{
    CHAR_DATA *ch;
    unsigned char direction;
    unsigned char ago;
    LIST_ENTRY(track_data) link;
};

extern FIGHT_DATA *fights_list;

struct fight_data
{
    FIGHT_DATA *next;
    CHAR_DATA  *agressors;
    CHAR_DATA  *victims;
};

#define MAX_CHATBOOKS   10

/* tail query, not just list */
/*
struct lua_progs
{
    struct lua_progs	*next;
    int32_t		trig;
    char		*func;
    char		*arg;
};
*/

/*
 * Prototype for a mob.
 * This is the in-memory version of #MOBILES.
 */

struct  mob_index_data
{
    AREA_DATA *    area;    /* OLC */
    MOB_INDEX_DATA *  next;
    SPEC_FUN *    spec_fun;
    SHOP_DATA *    pShop;
    PROG_LIST *    progs;
    int      	vnum;
    int      	group;
    bool    	new_format;
    int16_t    count;
    int16_t    killed;
    char *    player_name;
    char *    short_descr;
    char *    long_descr;
    char *    description;
    int64_t    act;
    int64_t    affected_by;
    int16_t	   saving_throw;    
    int16_t    alignment;
    int16_t    level;
    int16_t    hitroll;
    int16_t    hit[3];
    int16_t    mana[3];
    int16_t    damage[3];
    int16_t    ac[4];
    int16_t     dam_type;
    int64_t    off_flags;
    int16_t    resists[DAM_MAX];
    int16_t    start_pos;
    int16_t    default_pos;
    int16_t    sex;
    int16_t    race;
    long    wealth;
    int64_t    form;
    int64_t    parts;
    int16_t    size;
    char *    material;
    int      chat_vnum[MAX_CHATBOOKS];
    bool    areal[SECT_MAX];

    char    *msg_arrive;
    char    *msg_gone;

    int16_t    clan;
    int      rating;

    char	*comments;
    char	*last_edited;

/*
*    struct lua_progs *lprg;
*    struct lua_progs *lprg_last;
*/
};



/* memory settings */
#define MEM_ANY      0
#define MEM_CUSTOMER A
#define MEM_SELLER   B
#define MEM_HOSTILE  C
#define MEM_AFRAID   D
#define MEM_MOBPROG  E

/* memory for mobs */
struct mem_data
{
    MEM_DATA	*next;
    bool	valid;
    int		id;
    int		reaction;
    time_t	when;
};

/*
 * One character (PC or NPC).
 */
struct char_data
{
    CHAR_DATA		*next_in_fight;
    CHAR_DATA		*master;
    CHAR_DATA		*leader;
    CHAR_DATA		*fighting;
    CHAR_DATA		*reply;
    CHAR_DATA		*pet;
    CHAR_DATA		*mprog_target;
    MEM_DATA		*memory;
    SPEC_FUN		*spec_fun;
    MOB_INDEX_DATA	*pIndexData;
    DESCRIPTOR_DATA	*desc;
    AFFECT_DATA		*affected;
    NOTE_DATA		*pnote;
    OBJ_DATA		*carrying;
    OBJ_DATA		*on;
    ROOM_INDEX_DATA	*in_room;
    ROOM_INDEX_DATA	*was_in_room;
    AREA_DATA		*zone;
    PC_DATA		    *pcdata;
    GEN_DATA		*gen_data;
    FIGHT_DATA		*fight;
    bool		    valid;
    char		    *name;
    long		    id;
    int16_t		    version;
    char		    *short_descr;
    char		    *long_descr;
    char		    *description;
    char		    *prompt;
    char		    *prefix;
    int			    group;
    int16_t		    clan;
    int16_t		    clan_ready;
    int16_t		    sex;
    int16_t		    classid;
    int16_t		    race;
    int16_t		    level;
    int16_t		    trust;
    int			    played;
    int			    lines;  /* for the pager */
    time_t		    logon;
    int16_t		    timer;
    int16_t		    wait;
    int16_t		    daze;
    int16_t		    hit;
    int16_t		    max_hit;
    int16_t		    over_hit;
    int16_t		    mana;
    int16_t		    max_mana;
    int16_t		    over_mana;
    int16_t		    move;
    int16_t		    max_move;
    int16_t		    over_move;
    long		    gold;
    long		    silver;
    int			    exp;
    int64_t		    act;
    int64_t		    comm;   /* RT added to pad the vector */
    int64_t		    wiznet; /* wiz stuff */
    int16_t		    resists[DAM_MAX];
    int16_t		    invis_level;
    int16_t		    incog_level;
    int64_t		    affected_by;
    int16_t		    position;
    int16_t		    practice;
    int16_t		    train;
    int16_t		    carry_weight;
    int16_t		    carry_number;
    int16_t		    saving_throw;
    int16_t		    alignment;
    int16_t		    hitroll;
    int16_t		    damroll;
    int16_t		    armor[4];
    int16_t		    wimpy;
    /* stats */
    int16_t		    perm_stat[MAX_STATS];
    int16_t		    mod_stat[MAX_STATS];
    /* parts stuff */
    int64_t		    form;
    int64_t		    parts;
    int16_t		    size;
    char		    *material;
    /* mobile stuff */
    int64_t		    off_flags;
    int16_t		    damage[3];
    int16_t		    dam_type;
    int16_t		    start_pos;
    int16_t		    default_pos;
    int16_t		    count_holy_attacks;
    int16_t		    count_guild_attacks;

    int16_t		    mprog_delay;

    char		    *war_command;
    int16_t		    war_command_on;
    char		    *lost_command;

    CHAR_DATA		*mount;
    bool		    riding;
    int			    style;

    /* for chat */
    int			    angry;
    int			    last_regexp;

    /* for group guests */
    GROUP_QUEST_DATA	*group_quest;
    bool		    in_trap;

    /* List links */
    LIST_ENTRY(char_data) link;
    LIST_ENTRY(char_data) room_link;

    char		*notepad;
};



/*
 * Data which only PC's have.
 */
struct filter_data
{
    char *ch;
};

struct pc_data
{
    PC_DATA	*next;
    BUFFER	*buffer;
    COLOUR_DATA	codes[MAX_COLOUR];	/* Data for coloUr configuration  */
    bool	        valid;
    char	        *pwd;
    char	        *bamfin;
    char	        *bamfout;
    char	        *title;
    time_t	        last_note;
    time_t	        last_idea;
    time_t	        last_penalty;
    time_t	        last_news;
    time_t	        last_changes;
    time_t	        last_votes;
    int16_t	        perm_hit;
    int16_t	        perm_mana;
    int16_t	        perm_move;
    int16_t	        true_sex;
    int		        last_level;
	unsigned char   num_char;
	unsigned char   count_entries_errors;
	int             platinum;
	int             bronze;
    int16_t	        condition[4];
    int16_t	        *learned;
    int16_t	        *skill_mod;
    int32_t	        *over_skill;
    bool	        group_known[MAX_GROUP];
    int16_t	        points;
    char	        *alias[MAX_ALIAS];
    char	        *alias_sub[MAX_ALIAS];
    int		        security;		/* OLC */ /* Builder security */
	int		        temp_RIP;                   /* 0 - none, 1- no enable, 2 - enable, 3 - not enabled */
#if 1
    /*
     *Colour data stuff for config.
     */
    int		text[3];		/* {t (*) */
    int		auction[3];		/* {a */
    int		auction_text[3];	/* {A (*) */
    int		gossip[3];		/* {d */
    int		gossip_text[3];		/* {9 (*) */
    int		music[3];		/* {e */
    int		music_text[3];		/* {E */
    int		question[3];		/* {q (*) */
    int		question_text[3];	/* {Q (*) */
    int		answer[3];		/* {f (*) */
    int		answer_text[3];		/* {F (*) */
    int		quote[3];		/* {h (*) */
    int		quote_text[3];		/* {H (*) */
    int		immtalk_text[3];	/* {i */
    int		immtalk_type[3];	/* {I */
    int		info[3];		/* {j (*) */
    int		say[3];			/* {6 (*) */
    int		say_text[3];		/* {7 (*) */
    int		tell[3];		/* {k (*) */
    int		tell_text[3];		/* {K (*) */
    int		reply[3];		/* {l (*) */
    int		reply_text[3];		/* {L (*) */
    int		gtell_text[3];		/* {n (*) */
    int		gtell_type[3];		/* {N (*) */
    int		wiznet[3];		/* {B */
    int		room_title[3];		/* {s */
    int		room_text[3];		/* {S */
    int		room_exits[3];		/* {o */
    int		room_things[3];		/* {O */
    int		prompt[3];		/* {p */
    int		fight_death[3];		/* {1 */
    int		fight_yhit[3];		/* {2 */
    int		fight_ohit[3];		/* {3 */
    int		fight_thit[3];		/* {4 */
    int		fight_skill[3];		/* {5 */
#endif
    int		bounty;

    int		kills_mob;
    int		kills_pc;
    int		deaths_mob;
    int		deaths_pc;
    int		flees;

    char	*cases[5];
    long	bank;
    char	*lastip;
    char	*lasthost;
    time_t	lastlogof;


    int		quest_accum;		/* Общее количество купе за всю игру чаром */
    CHAR_DATA	*questgiver;		/* who gave the quest quest */
    int		quest_curr;         /* Текущее количество купе у чара */
    int		nextquest;
    int		qtime;
    int		countdown;
    int		questobj;
    char	*questobjname;
    int		questmob;
    int16_t	quest_type;

    char	*spouse;
    int		max_level;
    int		quaff;
    int		orig_align;
    struct	filter_data filter[MAX_FILTERS];
    time_t	freeze;
    char	*freeze_who;

    time_t	nochan;
    time_t	nonotes;
    time_t	notitle;

    OBJ_DATA	*beacon;
    char	*email;
    char	email_key[16];

    char	*grants;		/* Granted wiz commands */
    int		old_age;
    char	*pretitle;
    char	*old_names;
    char	*arena_bet_name;
    int		arena_victories;
    int		arena_losses;

    NAME_LIST	*ips;
    char	*genip;
    int		train_hit;
    int		train_mana;

    /*внум основной комнаты его дома*/
    int		vnum_house;
    /*массив - кого пригласил*/
    long	id_who_guest[MAX_GUESTS];
    /*массив - кем приглашен*/
    long	id_whom_guest[MAX_GUESTS];
    /*флаг продолжения свадьбы у моба*/
    int		flag_can_marry;
    /*контрольный вопрос/ответ*/
    char	*reg_answer;

    Bytef	*visited;
    int		visited_size;
    int		avg_dam;
    int		dam_tick;
	int		atheist;
	char	successed;
};

/* Data for generating characters -- only used during generation */
struct gen_data
{
    GEN_DATA	*next;
    bool	valid;
    bool	*skill_chosen;
    bool	group_chosen[MAX_GROUP];
    int		points_chosen;
};



/*
 * Liquids.
 */
#define LIQ_WATER        0

struct liq_type
{
    char	*liq_name;
    char *liq_rname;
    char *liq_color;
    int16_t  liq_affect[5];
};



/*
 * Extra description data for a room or object.
 */
struct  extra_descr_data
{
    EXTRA_DESCR_DATA *next;  /* Next in list                     */
    bool valid;
    char *keyword;              /* Keyword in look/examine          */
    char *description;          /* What to see                      */
};

/*
 * Структура участника боев.
 */
struct  challenger_data
{
    CHALLENGER_DATA  *next;
    char             *name;
    SCORE_DATA       *score;
    int		     ball;
    int		     game;
    bool	     sort;
    bool             valid;
};

/*
 * Структура отдельно взятого боя.
 */
struct  score_data
{
    SCORE_DATA       *next;
    time_t           time;
    char             *opponent;
    int              won;
    bool             valid;
};


#define INGREDIENT_MIN_VNUM 	1900
#define INGREDIENT_MAX_VNUM 	1999

#define MAX_ING			8
#define MAX_MORTAR_ING		7

#define MOB_VNUM_GOMUN		9
#define MOB_VNUM_WOLF		10
#define MOB_VNUM_BEAR		11

#define RECIPE_NONE		0
#define RECIPE_POTION		1
#define RECIPE_PILL		2
#define RECIPE_TANK		3
#define RECIPE_DAM		4
#define RECIPE_MAGE		5
#define RECIPE_CLERIC		6

/*
 * Структура рецепта.
 */
struct  recipe_data
{
    RECIPE_DATA      *next;
    char             *name;
    int		     level;
    int		     rlevel;
    int		     comp;
    int		     type;
    int		     reserve;
    int              value[MAX_ING];
    bool             valid;
};

struct  query_data
{
    QUERY_DATA       *next;
    char             *name;
    char	     *text;
    bool             valid;
};
/*
 * Prototype for an object.
 */
struct  obj_index_data
{
    AREA_DATA		    *area;  /* OLC */
    OBJ_INDEX_DATA	    *next;
    EXTRA_DESCR_DATA	*extra_descr;
    AFFECT_DATA		    *affected;
    bool		        new_format;
    char		        *name;
    char		        *short_descr;
    char		        *description;
    int			        vnum;
    int16_t		        reset_num;
    char		        *material;
    int16_t		        item_type;
    int64_t		        extra_flags;
    int64_t		        uncomf;
    int64_t		        unusable;
    int			        wear_flags;
    int16_t		        level;
    int16_t		        condition;
    int16_t		        count;
    int16_t		        weight;
    int			        cost;
    int			        value[5];
    PROG_LIST		    *progs;
    int			        edited;
    bool		        valid;
    int16_t		        require[MAX_STATS];
    char		        *comments;
    char		        *last_edited;
    int16_t		        timer;
};

#define TRAP_ON_ENTER	(A)
#define TRAP_ON_OPEN	(B)
#define TRAP_ON_UNLOCK	(C)
#define TRAP_ON_GET	(D)
#define TRAP_ON_PICK	(E)
#define TRAP_NO_DISARM	(F)


/*
 * One object.
 */
struct  obj_data
{
    OBJ_DATA		*next;
    OBJ_DATA		*prev;
    OBJ_DATA		*next_content;
    OBJ_DATA		*prev_content;
    OBJ_DATA		*contains;
    OBJ_DATA		*in_obj;
    OBJ_DATA		*on;
    CHAR_DATA		*carried_by;
    EXTRA_DESCR_DATA	*extra_descr;
    AFFECT_DATA		*affected;
    OBJ_INDEX_DATA	*pIndexData;
    ROOM_INDEX_DATA	*in_room;
    bool		valid;
    long		id;
    bool		enchanted;
    char		*name;
    char		*short_descr;
    char		*description;
    char		*owner;
    char		*person;
    int16_t		item_type;
    int64_t		extra_flags;
    int64_t		uncomf;
    int64_t		unusable;
    int			wear_flags;
    int16_t		wear_loc;
    int16_t		weight;
    int			cost;
    int16_t		level;
    int16_t		condition;
    char		*material;
    int			timer;
    int			value[5];
    CHAR_DATA		*oprog_target;
    int16_t		oprog_delay;
    time_t		lifetime;

    OBJ_DATA		*trap;
    int16_t		trap_flags;

    char		*who_kill;
    time_t		when_killed;
    int16_t		require[MAX_STATS];
};



/*
 * Exit data.
 */
struct  exit_data
{
    EXIT_DATA *    next;    /* OLC */
    int      rs_flags;  /* OLC */
    int      orig_door;  /* OLC */

    union
    {
  		ROOM_INDEX_DATA *  to_room;
  		int      vnum;
    } u1;
    int16_t    exit_info;
    int16_t    key;
    char *    keyword;
    char *    description;
    OBJ_DATA *    trap;
    int16_t    trap_flags;
};



/*
 * Reset commands:
 *   '*': comment
 *   'M': read a mobile
 *   'H': put mobile on mobile 8)) (chivalry)
 *   'O': read an object
 *   'P': put object in object
 *   'G': give object to mobile
 *   'E': equip object to mobile
 *   'D': set state of door
 *   'R': randomize room exits
 *   'T': put trap in room
 *   'C': put trap on container ir portal
 *   'X': put trap on exit in room
 *   'S': stop (end of list)
 */

/*
 * Area-reset definition.
 */
struct  reset_data
{
    RESET_DATA *  next;
    char    command;
    int      arg1;
    int      arg2;
    int      arg3;
    int      arg4;
};

#define POPUL_VISIT  0
#define POPUL_KILL  1
#define POPUL_PK  2
#define POPUL_DEATH	3
#define MAX_POPUL  4

/* File version */
#define POPUL_VERS 2


/* New area_data for OLC */
struct  area_data
{
    AREA_DATA	*next;
    RESET_DATA	*reset_first;
    RESET_DATA	*reset_last;
    HELP_AREA	*helps;
    char	*file_name;
    char	*name;
    char	*credits;
    int16_t	age;
    int16_t	nplayer;
    int16_t	low_range;
    int16_t	high_range;
    int		min_vnum;
    int		max_vnum;
    bool	empty;
    char	*builders;	/* OLC */ /* Listing of */
    int		vnum;		/* OLC */ /* Area vnum  */
    int		area_flags;	/* OLC */
    int		security;	/* OLC */ /* Value 1-9  */
    int		popularity[MAX_POPUL];
    int16_t	version;
};

/*
 * Room type.
 */
struct  room_index_data
{
    RESET_DATA		*reset_first;  /* OLC */
    RESET_DATA		*reset_last;  /* OLC */
    ROOM_INDEX_DATA	*next;
    OBJ_DATA		*contents;
    EXTRA_DESCR_DATA	*extra_descr;
    AREA_DATA		*area;
    EXIT_DATA		*exit[MAX_DIR];
    char		*name;
    char		*description;
    char		*owner;
    int			vnum;
    int64_t		room_flags;
    int16_t		light;
    int16_t		sector_type;
    int16_t		heal_rate;
    int16_t		mana_rate;
    int			clan;

    int			distance_from_source;
    ROOM_INDEX_DATA	*shortest_from_room;
    ROOM_INDEX_DATA	*shortest_next_room;
    PROG_LIST		*progs;
    CHAR_DATA		*rprog_target;
    int16_t		rprog_delay;
    AFFECT_DATA		*affected;

    char		*comments;
    char		*last_edited;
    int16_t		min_level;
    int16_t		max_level;
/*
*    struct lua_progs	*lprg;
*    struct lua_progs	*lprg_last;
*/
    LIST_HEAD(, char_data) people;
    LIST_HEAD(, track_data) tracks;
};



/*
 * Types of attacks.
 * Must be non-overlapping with spell/skill types,
 * but may be arbitrary beyond that.
 */
#define TYPE_UNDEFINED               -1
#define TYPE_HIT                     1000



/*
 *  Target types.
 */
#define TAR_IGNORE        0
#define TAR_CHAR_OFFENSIVE      1
#define TAR_CHAR_DEFENSIVE      2
#define TAR_CHAR_SELF        3
#define TAR_OBJ_INV        4
#define TAR_OBJ_CHAR_DEF      5
#define TAR_OBJ_CHAR_OFF      6

#define TARGET_CHAR        0
#define TARGET_OBJ        1
#define TARGET_ROOM        2
#define TARGET_NONE        3



/*
 * Skills include spells as a particular case.
 */
struct  skill_type
{
    char *  name;               /* Name of skill                */
    char *  rname;              /* Russian name of skill        */
    int16_t  *skill_level;      /* Level needed by class        */
    int16_t  *rating;           /* How hard it is to learn      */
    bool  *quest;               /* If TRUE, this skill can be   */
                                /* gained only in the quest     */
    SPELL_FUN *  spell_fun;     /* Spell pointer (for spells)   */
    int16_t  target;            /* Legal targets                */
    int16_t  minimum_position;  /* Position for caster / user   */
    int16_t *  pgsn;            /* Pointer to associated gsn    */
    int16_t  slot;              /* Slot for #OBJECT loading     */
    int16_t  min_mana;          /* Minimum mana used            */
    int16_t  beats;             /* Waiting time after use       */
    char *  noun_damage;        /* Damage message               */
    char *  gender_damage;      /* Gender of Damage message     */
    char *  msg_off;            /* Wear off message             */
    char *  msg_obj;            /* Wear off message for obects  */
    char *  msg_room;           /* Wear off message for rooms   */

              /* All fields below concern ONLY   */
          /* spells on a basis spell_generic */
    char *  damage;      /* Damage expression for spells    */
              /* (look on calc() func in magic.c */
    int16_t  dam_type;    /* Damage type (fire, acid, etc.)  */
    int16_t  flags;      /* Spells/skills flags (see below) */
    int16_t  saves_mod;    /* Saves throw modifier       */
    int16_t  saves_act;    /* Saves flags (see below)     */
    SPELLAFF *  affect;      /* See below         */
    char *  aff_dur;    /* Affect duration       */
    char *  char_msg[3];    /* Message to caster, victim, room */
    char *  obj_msg[2];    /* Message to caster, room     */
    char *  room_msg[2];    /* Message to caster, room     */
    char *  char_fail[3];    /* Message to caster, victim, room */
    char *  obj_fail[2];    /* Message to caster, room     */
    char *  room_fail[2];    /* Message to caster, room     */
    char *  depends[MAX_SKILL];
};

struct spellaff
{
    SPELLAFF *  next;      /* Next in list         */
    int16_t  where;      /* TO_AFFECTS, TO_ROOMAFF, etc...  */
    int16_t  apply;      /* APPLY_what         */
    int64_t  bit;      /* Affect bitvector (AFF_what)     */
    int16_t  flags;      /* See SPAFF_*         */
    char *  mod;      /* Affect modifier expression     */
};

#define SPELL_AREA    (A)  /* Area attack/affect      */
#define SPELL_AREA_SAFE    (B)  /* Same, but not caster's group    */
#define SPELL_BUGGY    (C)  /* Spell is buggy      */
#define SPELL_NOSCRIBE    (D)  /* Spell can't be scribed    */
#define SPELL_NOBREW    (E)  /* Spell can't be brewed    */
#define SPELL_NOWAND    (F)  /* Spell can't be used for create */
          /* staves/wands/rods      */
#define SPELL_RANDOM    (G)  /* Spell get random target    */
#define SPELL_INSTANT_KILL  (H)  /* pwk, earthmaw...      */
#define SPELL_RECASTABLE  (I)  /* Spell affect can be refreshed  */
#define SPELL_ACCUM    (J)  /* Spell affect is accumulative   */

#define SAVES_IGNORE     0  /* Spell ignores saves       */
#define SAVES_HALFDAM     1  /* Spell's damage 1/2       */
#define SAVES_QUARTERDAM   2  /* Spell's damage 1/4       */
#define SAVES_3QTRDAM     3  /* Spell's damage 3/4       */
#define SAVES_THIRDDAM     4  /* Spell's damage 1/3       */
#define SAVES_2THIRDSDAM   5  /* Spell's damage 2/3       */
#define SAVES_ABSORB     6  /* Spell's damage absorbs to     */
          /* victims hp         */
#define SAVES_ABSORB_MANA   7  /* ---"--- victims mana       */
#define SAVES_REFLECT     8  /* Spell's damage returns to caster*/
#define SAVES_REFLECT_HALF   9  /* 1/2 ---"---         */
#define SAVES_NONE    10  /* Spell does not damage victim    */
#define SAVES_FULL    11  /* Spell does full damage     */

#define SPELLAFF_CASTER    (A)  /* Spell affects on caster    */
#define SPELLAFF_VICTIM    (B)  /* Spell affects on victim    */
#define SPELLAFF_OBJ    (C)  /* Spell affects on object    */
#define SPELLAFF_ROOM    (D)  /* Spell affects on room    */
#define SPELLAFF_GROUP    (E)  /* TAR_IGNORE only, affects group */

struct  group_type
{
    char *  name;
    char *  rname;
    int16_t  *rating;
    char *  spells[MAX_IN_GROUP];
};

/*
 * MOBprog definitions
 */
#define TRIG_ACT  	(A)
#define TRIG_BRIBE  	(B)
#define TRIG_DEATH  	(C)
#define TRIG_ENTRY  	(D)
#define TRIG_FIGHT  	(E)
#define TRIG_GIVE  	(F)
#define TRIG_GREET  	(G)
#define TRIG_GRALL  	(H)
#define TRIG_KILL  	(I)
#define TRIG_HPCNT  	(J)
#define TRIG_RANDOM  	(K)
#define TRIG_SPEECH  	(L)
#define TRIG_EXIT  	(M)
#define TRIG_EXALL  	(N)
#define TRIG_DELAY  	(O)
#define TRIG_SURR  	(P)
#define TRIG_GET  	(Q)
#define TRIG_DROP  	(R)
#define TRIG_SIT  	(S)
#define TRIG_WEAR  	(T)
#define TRIG_COMMAND  	(U)
#define TRIG_NEW_CMD  	(V)
#define TRIG_POST_CMD  	(W)
#define TRIG_QUIT  	(X)
#define TRIG_PUT       	(Y)

#define LTRIG_ACT	 0
#define LTRIG_BRIBE	 1
#define LTRIG_DEATH	 2
#define LTRIG_ENTRY	 3
#define LTRIG_FIGHT	 4
#define LTRIG_GIVE	 5
#define LTRIG_GREET	 6
#define LTRIG_GRALL	 7
#define LTRIG_KILL	 8
#define LTRIG_HPCNT	 9
#define LTRIG_RANDOM	10
#define LTRIG_SPEECH	11
#define LTRIG_EXIT	12
#define LTRIG_EXALL	13
#define LTRIG_DELAY	14
#define LTRIG_SURR	15
#define LTRIG_GET	16
#define LTRIG_DROP	17
#define LTRIG_SIT	18
#define LTRIG_WEAR	19
#define LTRIG_COMMAND	20
#define LTRIG_NEW_CMD	21
#define LTRIG_POST_CMD	22


#define EXEC_DEFAULT	(aa)
#define EXEC_BEFORE	(bb)
#define EXEC_AFTER	(cc)
#define EXEC_ALL	(EXEC_DEFAULT|EXEC_AFTER|EXEC_BEFORE)

/*
 * Prog types
 */
#define PRG_MPROG  0
#define PRG_OPROG  1
#define PRG_RPROG  2



struct prog_list
{
    int	                trig_type;
    char *              trig_phrase;
    int           	vnum;
    char *              code;
    PROG_LIST *         next;
    bool                valid;
    int64_t		trig_flags;
};

struct prog_code
{
    int      		vnum;
    char *              code;
    PROG_CODE *         next;
};

/*
 * Utility macros.
 */

#define IS_VALID(data)    ((data) != NULL && (data)->valid)
#define VALIDATE(data)    do { if ((data) != NULL) (data)->valid = TRUE; } while (0)
#define INVALIDATE(data)  ((data)->valid = FALSE)

#define LOWER(c)    ((c) >= 'А' && (c) <= 'Я'\
        ? (c)+'а'-'А'\
        : ((c) >= 'A' && (c) <= 'Z'? (c)+'a'-'A' : (c)))

#define UPPER(c)    ((c) >= 'а' && (c) <= 'я'\
        ? (c)+'А'-'а'\
        : ((c) >= 'a' && (c) <= 'z'? (c)+'A'-'a' : (c)))

#define IS_RUSSIAN(c)    (((c) >= 'А' && (c) <= 'Я')\
        || ((c) >= 'а' && (c) <= 'я'))
#define IS_ALPHA(c)    (IS_RUSSIAN(c) || ((c) >= 'A' && (c) <= 'Z')\
        || ((c) >= 'a' && (c) <= 'z'))


#define IS_SET(flag, bit)  ((flag) & (bit))
#define SET_BIT(var, bit)  ((var) |= (bit))
#define REMOVE_BIT(var, bit)  ((var) &= ~(bit))

#define IS_NULLSTR(str)    ((str) == NULL || (str)[0] == '\0')
#define ENTRE(min,num,max)  (((min) < (num)) && ((num) < (max)))

#define CHECK_POS(a, b, c)  {                \
          (a) = (b);            \
          if ((a) < 0)            \
          bugf("CHECK_POS : " c " == %d < 0", a);\
        }                \


/*
 * Character macros.
 */
#define IS_NPC(ch)    (IS_SET((ch)->act, ACT_IS_NPC))
#define IS_IMMORTAL(ch)    (get_trust(ch) >= LEVEL_IMMORTAL)
#define IS_HERO(ch)    (get_trust(ch) >= LEVEL_HERO)
#define IS_TRUSTED(ch,level)  (get_trust((ch)) >= (level))
#define IS_AFFECTED(ch, sn)  (IS_SET((ch)->affected_by, (sn)))

#define GET_AGE(ch)    ((int) (17 + ((ch)->played + current_time - (ch)->logon)/72000))

#define IS_GOOD(ch)    (ch->alignment >= 333)
#define IS_EVIL(ch)    (ch->alignment <= -333)
#define IS_NEUTRAL(ch)    (!IS_GOOD(ch) && !IS_EVIL(ch))

#define IS_AWAKE(ch)    (ch->position > POS_SLEEPING)
#define GET_AC(ch,type)    ((ch)->armor[type] + (IS_AWAKE(ch) ? dex_app[get_curr_stat(ch,STAT_DEX)].defensive : 0))

#define GET_HITROLL(ch)    ((IS_NPC(ch)) ? ((ch)->hitroll+dex_app[get_curr_stat(ch,STAT_DEX)].tohit) : ((1-((((ch)->hitroll+dex_app[get_curr_stat(ch,STAT_DEX)].tohit)/2) * 0.005 )) * ((ch)->hitroll+dex_app[get_curr_stat(ch,STAT_DEX)].tohit)))
#define GET_DAMROLL(ch)    ((IS_NPC(ch)) ? ((ch)->damroll+str_app[get_curr_stat(ch,STAT_STR)].todam) : ((1-((((ch)->damroll+str_app[get_curr_stat(ch,STAT_STR)].todam)/2) * 0.005 )) * ((ch)->damroll+str_app[get_curr_stat(ch,STAT_STR)].todam)))

#define GET_FULL_HITROLL(ch)   ((ch)->hitroll+dex_app[get_curr_stat(ch,STAT_DEX)].tohit)
#define GET_FULL_DAMROLL(ch)   ((ch)->damroll+str_app[get_curr_stat(ch,STAT_STR)].todam)

#define IS_OUTSIDE(ch)    ((ch)->in_room && !IS_SET((ch)->in_room->room_flags, ROOM_INDOORS) && (ch)->in_room->sector_type != SECT_INSIDE)

#define WAIT_STATE(ch, npulse)    wait_state((ch), (npulse))
#define DAZE_STATE(ch, npulse)    daze_state((ch), (npulse))
#define get_money_weight(gold, silver)  (2*(gold)/5 + (silver)/10)
#define get_carry_weight(ch)    ((ch)->carry_weight + get_money_weight((ch)->gold, (ch)->silver))

#define MOUNTED(ch)    mounted(ch)
#define RIDDEN(ch)     ridden(ch)

#define IS_SWITCHED(ch)       (ch->desc && ch->desc->original)
#define IS_BUILDER(ch, Area)  (ch && Area && !IS_NPC(ch) && !IS_SWITCHED(ch) &&   \
        (ch->pcdata->security >= Area->security  \
        || strstr(Area->builders, ch->name)   \
        || strstr(Area->builders, "All")))


#define SEX_ENDING(ch)    ((ch->sex == SEX_FEMALE) ? "а" : (ch->sex == SEX_MALE) ? "" : (ch->sex == SEX_MANY) ? "ы" : "о")
#define SEX_END_ADJ(victim)  ((victim->sex == SEX_FEMALE) ? "ой" : (ch->sex == SEX_MANY) ? "ыми" : "ым")
#define GET_MAX_SEX(ch)    (IS_NPC(ch) ? MAX_NPC_SEX : MAX_PC_SEX)

#define act(format,ch,arg1,arg2,type)  act_new((format),(ch),(arg1),(arg2),(type),POS_RESTING)

/*
 * Object macros.
 */
#define CAN_WEAR(obj, part)  (IS_SET((obj)->wear_flags,  (part)))
#define IS_OBJ_STAT(obj, stat)  (IS_SET((obj)->extra_flags, (stat)))
#define IS_WEAPON_STAT(obj,stat)(IS_SET((obj)->value[4],(stat)))
#define WEIGHT_MULT(obj)  ((obj)->item_type == ITEM_CONTAINER || (obj)->item_type == ITEM_CHEST ? (obj)->value[4] : 100)


#define CHECK_EMAIL(argument)  (strstr(argument, "@") && strstr(argument, ".") && !strstr(argument, "~"))

/*
 * Description macros.
 */
#define PERS(ch, looker, numcase)  (can_see(looker, (ch)) ? (IS_NPC(ch) ? cases(ch->short_descr, numcase) : cases(ch->name, numcase)) : (IS_IMMORTAL(ch) && !IS_SWITCHED(ch) ? cases(ch->sex == SEX_FEMALE ? "Бессмертная" : "Бессмертный", numcase) : cases("кто-то",numcase)))

#define PERS_OBJ(looker, obj, numcase)  (can_see_obj(looker, obj) ? cases(obj->short_descr, numcase) : cases("что-то",numcase))

#define IS_SPACE(arg)    (arg == ' ' || arg == '\n' || arg == '\r' || arg == '\t')

#define GET_SKILL(ch, sn)  ((IS_NPC((ch)) || ((ch)->pcdata->learned[(sn)]) <= 0) ? 0 : (((ch)->pcdata->learned[(sn)]) < 101 ? \
                            (ch)->pcdata->learned[(sn)] : 100) + (ch)->pcdata->skill_mod[(sn)])
#define CH(d)  (d->original ? d->original : d->character)

#define CHECK_AREAL(ch, to_room)  (!IS_NPC(ch) || ch->pIndexData->areal[to_room->sector_type])
#define CHECK_VNUM(vnum)    ((vnum) > 0 && (vnum) < MAX_VNUM)






/*
 * Structure for a social in the socials table.
 */
struct  social_type
{
    char      name[20];
    char *    char_no_arg;
    char *    others_no_arg;
    char *    char_found;
    char *    others_found;
    char *    vict_found;
    char *    char_not_found;
    char *    char_auto;
    char *    others_auto;
};


/* For quest */

#define MAX_IN_QUEST       200

struct player_quest_type
{
    int    score;
    char   name[15];
};

/*type*/
#define SINGLE_QUEST	0
#define GROUPS_QUEST	1
#define GLOBAL_QUEST	2

/*actiontype*/
#define QUEST_FINDOBJECT	0

/*
 * New Quest struct
 */
struct quest_datan
{
    QUEST_DATAN      *next;
    char	     *name;
    int16_t	     minlevel;
    int16_t	     maxlevel;
    int16_t	     type;
    int16_t	     actiontype;
    int64_t	     race;
    int64_t	     classid;
    char	     *mobquester;
    char	     *rewardlist;
    char	     *objlist;
    char	     *moblist;
    char	     *roomlist;
    char	     *castlist;
    char	     *text;
    bool             valid;
};


struct quest_type
{
    char        desc[MAX_INPUT_LENGTH];
    int        timer;
    int        max_score;
    bool      is_quest;
    CHAR_DATA *who_claim;
    struct player_quest_type  player[MAX_IN_QUEST];
    bool                        nowar;
    int      double_qp;
    int      double_exp;
    int      double_skill;
    char      *last_quest;
};

extern struct quest_type immquest;

#define TYPE_HOURS  	0
#define TYPE_POINTS  	1
#define TYPE_UNREAD  	2
#define TYPE_NEWS  	3
#define TYPE_CHANGES  	4
#define TYPE_NOTES  	5
#define TYPE_IDEAS  	6
#define TYPE_VISIT  	7
#define TYPE_PENALTY  	11
#define TYPE_MINUTES  	12
#define TYPE_DAYS  	13
#define TYPE_CHARGES  	14
#define TYPE_YEARS  	15
#define TYPE_PRACTICE  	16
#define TYPE_TRAIN  	17
#define TYPE_PIECE  	18
#define TYPE_ROUND  	19
#define TYPE_LIKE_THIS  20
#define TYPE_ITEM  	21
#define TYPE_SILVER  	22
#define TYPE_GOLD  	23
#define TYPE_COINS  	24
#define TYPE_MINUTE  	25
#define TYPE_HELPERS  	26
#define TYPE_SILVER1  	27
#define TYPE_GOLD1  	28
#define TYPE_COINS1  	29
#define TYPE_ERRORS  	30
#define TYPE_ROOMS  	31
#define TYPE_PRACTICES  32
#define TYPE_SOMETHING  33
#define TYPE_FORMS      34
#define TYPE_VOTES      35
#define TYPE_PLAYERS    36
#define TYPE_BRONZE  	37

/* For clans */
#define MAX_CLAN   20
#define MAX_CLAN_PLAYERS 150
#define MIN_CLAN_LEVEL   20
#define CLAN_INDEPEND   "независимые"
#define CLAN_NEWS_EXPIRED 2
#define MAX_NOCLAN_LEVEL 30

#define IS_INDEPEND(ch)  (is_clan(ch) && !str_cmp(clan_table[(ch)->clan].name, CLAN_INDEPEND))

struct clan_type
{
    char   *name;
    char   *short_descr;
    char   *long_descr;
    char   *clanmaster;
    char   *clanmaster2;
    bool  races[MAX_PC_RACE];
    bool  classes[MAX_CLASS];
    int    count;
    int    min_align;
    int    max_align;
    int    recall_vnum;
    bool   clan_war[MAX_CLAN];
    bool   clan_alliance[MAX_CLAN];
    bool  valid;
    int    rating;
};

struct clan_type  clan_table[MAX_CLAN];

extern bool clan_war_enabled;

#define MAX_PENALTYROOMS 5
struct penalty_rooms_type
{
    char  *name;
    int    vnum;
};

extern const struct penalty_rooms_type penalty_rooms_table[MAX_PENALTYROOMS];

/*extern  const   struct  weapon_msg_type weapon_msg_table[MAX_WEAPON_TYPE];*/

extern int max_races;
extern int max_classes;
extern int max_skills;
extern int max_groups;
extern int max_moons;

/*
 * Global constants.
 */
extern  const  struct  str_app_type  str_app    [MAX_STAT + 1];
extern  const  struct  int_app_type  int_app    [MAX_STAT + 1];
extern  const  struct  wis_app_type  wis_app    [MAX_STAT + 1];
extern  const  struct  dex_app_type  dex_app    [MAX_STAT + 1];
extern  const  struct  con_app_type  con_app    [MAX_STAT + 1];


extern  struct class_type  *class_table;
extern  const  struct  weapon_type  weapon_table  [];
extern  const  struct  item_type  item_table  [];
extern  const  struct  wiznet_type  wiznet_table  [];
extern  const  struct  attack_type  attack_table  [];
extern  struct race_type  *race_table;
extern  struct race_type  *pc_race_table;
extern  const  struct  spec_type  spec_table  [];
extern  const  struct  liq_type  liq_table  [];
extern  struct skill_type *skill_table;
extern  struct group_type *group_table;
extern          struct social_type      social_table  [MAX_SOCIALS];
extern  char *  title_table  [MAX_CLASS]
              [MAX_LEVEL+1]
              [2];

extern  const struct race_incompatible    race_incompatible_table  [];

/*
 * Global variables.
 */
#ifndef IN_RECYCLE_C
extern LIST_HEAD(, char_data) char_free;
#endif

#ifndef IN_COMM_C
extern SLIST_HEAD(, descriptor_data) descriptor_list;
#endif

#ifndef IN_DB_C
extern STAILQ_HEAD(, help_data) help_first;
extern LIST_HEAD(, char_data) char_list;
#endif

extern    SHOP_DATA    *  shop_first;
extern          AREA_DATA         *     area_first;

extern    ROOM_INDEX_DATA   *  room_list;
extern    OBJ_DATA    *  object_list;
extern    OBJ_DATA    *  obj_free;
extern          CHALLENGER_DATA   *     challenger_list;
extern          PROG_CODE         *     mprog_list;
extern          PROG_CODE         *     rprog_list;
extern          PROG_CODE         *     oprog_list;
extern          RECIPE_DATA       *     recipe_list;
extern          QUERY_DATA        *     query_list;


extern    char      bug_buf    [];
extern    time_t      current_time;
extern    FILE *      fpReserve;
extern    KILL_DATA    kill_table  [];
extern    TIME_INFO_DATA    time_info;
extern    WEATHER_DATA    weather_info;
extern          ROOM_INDEX_DATA   *  room_index_hash[MAX_KEY_HASH];
extern    bool      MOBtrigger;
extern    EXCEPT_DATA    *exceptions;
extern    int      limits[MAX_LIMITS];

/*
 * OS-dependent declarations.
 * These are all very standard library functions,
 *   but some systems have incomplete or non-ansi header files.
 */
#if  defined(_AIX)
char *  crypt    args((const char *key, const char *salt));
#endif

#if  defined(apollo)
int  atoi    args((const char *string));
void *  calloc    args((unsigned nelem, size_t size));
char *  crypt    args((const char *key, const char *salt));
#endif

#if  defined(hpux)
char *  crypt    args((const char *key, const char *salt));
#endif

#if  defined(linux)
char *  crypt    args((const char *key, const char *salt));
#endif

#if  defined(MIPS_OS)
char *  crypt    args((const char *key, const char *salt));
#endif

#if  defined(NeXT)
char *  crypt    args((const char *key, const char *salt));
#endif

#if  defined(sequent)
char *  crypt    args((const char *key, const char *salt));
int  fclose    args((FILE *stream));
int  fprintf    args((FILE *stream, const char *format, ...));
int  fread    args((void *ptr, int size, int n, FILE *stream));
int  fseek    args((FILE *stream, long offset, int ptrname));
void  perror    args((const char *s));
int  ungetc    args((int c, FILE *stream));
#endif

#if  defined(sun)
char *  crypt    args((const char *key, const char *salt));
int  fclose    args((FILE *stream));
int  fprintf    args((FILE *stream, const char *format, ...));
#if  defined(SYSV)
size_t  fread    args((void *ptr, size_t size, size_t n,
          FILE *stream));
#elif !defined(__SVR4)
int  fread    args((void *ptr, int size, int n, FILE *stream));
#endif
int  fseek    args((FILE *stream, long offset, int ptrname));
void  perror    args((const char *s));
int  ungetc    args((int c, FILE *stream));
#endif

#if  defined(ultrix)
char *  crypt    args((const char *key, const char *salt));
#endif



/*
 * The crypt(3) function is not available on some operating systems.
 * In particular, the U.S. Government prohibits its export from the
 *   United States to foreign countries.
 * Turn on NOCRYPT to keep passwords in plain text.
 */
#if  defined(NOCRYPT)
#define crypt(s1, s2)  (s1)
#endif



/*
 * Data files used by the server.
 *
 * AREA_LIST contains a list of areas to boot.
 * All files are read in completely at bootup.
 * Most output files (bug, idea, typo, shutdown) are append-only.
 *
 * The NULL_FILE is held open so that we have a stream handle in reserve,
 *   so players can go ahead and telnet to all the other descriptors.
 * Then we close it whenever we need to open a file (e.g. a save file).
 */
#define PLAYER_DIR      "../player/"          /* Player files */
#define GOD_DIR         "../gods/"      /* list of gods */
#define LOG_DIR         "../log/"      /* log dir */
#define TEMP_FILE  "../player/romtmp"

#define NULL_FILE  "/dev/null"    /* To reserve one stream */


#define AREA_LIST       "area.lst"  /* List of areas*/
#define BUG_FILE        "bugs.txt" /* For 'bug' and bug()*/
#define TODO_FILE		"todo.txt" /*For todo*/

/*для списка всех операций с домами*/
/*#define HOUSE_FILE		"houses.txt" */

#define TYPO_FILE       "typos.txt" /* For 'typo'*/
#define NOTE_FILE       "notes.not"/* For 'notes'*/
#define IDEA_FILE  "ideas.not"
#define PENALTY_FILE  "penal.not"
#define NEWS_FILE  "news.not"
#define CHANGES_FILE  "chang.not"
#define VOTES_FILE  "votes.not"
#define SHUTDOWN_FILE   "shutdown.txt"/* For 'shutdown'*/
#define BAN_FILE  "ban.txt"
#define MUSIC_FILE  "music.txt"
#define EXCEPT_FILE  "except.txt"
#define LIMITS_FILE  "limits.txt"
#define CLANS_FILE  "clans.txt"
#define DREAM_FILE  "dream.txt"
#define LAST_COMMAND  "last_command.txt"
#define QUOTES_FILE  "quotes.txt"
#define STOPLIST_FILE  "stoplist.txt"
#define MAX_EVER_FILE  "max_ever.txt"
#define WHOIS_FILE  "whois.txt"
#define ASPELL_FILE  "aspell.txt"
#define SKILLS_FILE  "skills.cfg"
#define GROUPS_FILE  "groups.cfg"
#define MOONS_FILE  "moons.cfg"
#define RACES_LIST  "../races/races.lst"
#define RACES_DIR  "../races/"
#define CLASSES_LIST  "../classes/classes.lst"
#define CLASSES_DIR  "../classes/"
#define CHALLENGER_LIST "../scores/challenger.lst"
#define CHALLENGER_DIR  "../scores/"
#define BADNAMES_FILE  "badnames.cfg"
#define CHAT_LOG  "chat.log"
#define VIOLATE_FILE  "violation.txt"
#define CORPSES_FILE  "corpses.txt"
#define COL_SCH_FILE  "colours.cfg"
#define WWW_COUNT_FILE  "wwwcount.txt"
#define POPULARITY_FILE  "popularity.txt"
#define QUEST_LOG  "quest.log"
#define CHEST_LIST  "../chest/chest.lst"
#define CHEST_DIR  "../chest/"
#define RECIPE_FILE  "recipe.txt"
#define QUERY_FILE  "query.txt"

#define DEBUG_FILE  "../log/debug.txt"




/*
 * Our function prototypes.
 * One big lump ... this is every function in Merc.
 */
#define CD  CHAR_DATA
#define MID  MOB_INDEX_DATA
#define OD  OBJ_DATA
#define OID  OBJ_INDEX_DATA
#define RID  ROOM_INDEX_DATA
#define AD  AFFECT_DATA
#define PC  PROG_CODE


/* act_comm.c */
void check_sex(CHAR_DATA *ch);
void add_follower(CHAR_DATA *ch, CHAR_DATA *master);
void stop_follower(CHAR_DATA *ch);
void nuke_pets(CHAR_DATA *ch);
void die_follower(CHAR_DATA *ch);
bool is_same_group(CHAR_DATA *ach, CHAR_DATA *bch);
bool check_password(DESCRIPTOR_DATA *d, char *argument);
char *dns_gethostname(char *s);
void check_proxy(unsigned int address);
int cp_get_state(unsigned int address);
int recode_trans(char *argument,bool outp);
void safe_exit(int flag);
void mailing(char *to, char *subj, char *body);

/* act_info.c */
void set_title(CHAR_DATA *ch, char *title);
bool check_blind(CHAR_DATA *ch);
void pet_gone(CHAR_DATA *pet);
bool is_compatible_races(void *ch1, void *ch2, bool is_char_data);

/* act_move.c */
void move_char(CHAR_DATA *ch, int door, bool show, bool fRun);
int what_door(char *arg1);
void check_recall(CHAR_DATA *ch, char *message);

/* act_obj.c */
bool can_loot(CHAR_DATA *ch, OBJ_DATA *obj);
void wear_obj(CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace);
bool get_obj(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *container, bool fAll );
void do_second(CHAR_DATA *ch, char *argument);
void make_worse_condition(CHAR_DATA *ch, int thing, int dam, int16_t dt);
void check_auctions(CHAR_DATA *victim, OBJ_DATA *obj, char *msg);
void check_condition(CHAR_DATA *ch, OBJ_DATA *obj);

/* act_wiz.c */
void wiznet(char *string, CHAR_DATA *ch, OBJ_DATA *obj, int64_t flag, int64_t flag_skip, int min_level);
void save_popularity();
void do_ostat(CHAR_DATA *ch, OBJ_DATA *obj);
void do_mstat(CHAR_DATA *ch, CHAR_DATA *victim);
void do_rstat(CHAR_DATA *ch, ROOM_INDEX_DATA *location);

/* alias.c */
void substitute_alias(DESCRIPTOR_DATA *d, char *input);

/* ban.c */
bool check_ban(char *site, int type);

/* clan.c */
bool is_in_war(CHAR_DATA *ch, CHAR_DATA *victim);
void check_clan_war(CHAR_DATA *ch);
int is_clan(CHAR_DATA *ch);
bool is_clans_in_alliance(int c1, int c2);
bool check_clan_group(CHAR_DATA *ch);

/* comm.c */
void show_string(struct descriptor_data *d, char *input);
void close_socket(DESCRIPTOR_DATA *dclose);
void write_to_buffer(DESCRIPTOR_DATA *d, const char *txt, int length);
void send_to_char(const char *txt, CHAR_DATA *ch);
void printf_to_char(const char *fmt, CHAR_DATA *ch, ...)
		    __attribute__ ((format (printf, 1, 3)));
void printf_to_buffer(BUFFER *buffer, const char *fmt, ...)
                      __attribute__((format (printf, 2, 3)));
void page_to_char(const char *txt, CHAR_DATA *ch);
void act_new(const char *format, CHAR_DATA *ch, void *arg1, void *arg2, int type, int min_pos);
void tagline_to_char(const char *text, CHAR_DATA *ch, CHAR_DATA *looker);

#define  CODEPAGE_WIN  1
#define  CODEPAGE_KOI  2
#define  CODEPAGE_ALT  3
#define CODEPAGE_WINIAC 4
#define CODEPAGE_MAC    5
#define CODEPAGE_TRANS  6

#define RECODE_INPUT    1
#define RECODE_OUTPUT    2
#define RECODE_NOANTITRIGGER  4

#define IS_TRANSLIT(ch) (ch && !IS_NPC(ch) && ch->desc && ch->desc->codepage == CODEPAGE_TRANS)

int recode(char *argument,int codepage,int16_t outp) ;

extern bool Show_output;
extern bool Show_output_old;
extern bool Show_result;
/*
 * Colour stuff by Lope
 */
int colour(char type, CHAR_DATA *ch, char *string);
void colourconv(char *buffer, const char *txt, CHAR_DATA *ch);

/* load.c */
char *gsn_name_lookup(int16_t *gsn);
int16_t *gsn_lookup(char *name);
void load_skills();
void load_groups();
void save_groups();
void fread_skill(FILE *fp, struct skill_type *st);
void save_skills();
const char *spellname_lookup(SPELL_FUN *fun);
SPELL_FUN *spellfun_lookup(char *name);
void load_races();
void fread_race(FILE *fp, struct race_type *race);
void load_classes();
void save_classes(CHAR_DATA *ch);
void load_moons();

/* save.c */
void    save_chest args((long id));
void    save_one_chest args((OBJ_DATA *chest));
void    load_chest args((void));
void    delete_chest args((long id));

/* arena.c */
/*
void    load_arenastats args((void));
void    save_arenastats args((void));
void    load_scores     args((FILE *fp, CHALLENGER_DATA *challenger));
void    delete_score    args((CHALLENGER_DATA *challenger, time_t time));
void    delete_challenger               args((CHALLENGER_DATA *challenger));
CHALLENGER_DATA *add_challenger         args((char *name));
SCORE_DATA      *add_score              args((CHALLENGER_DATA *challenger, time_t time, char *opponent, int won));
CHALLENGER_DATA *find_challenger        args((char *name));
SCORE_DATA      *find_score_by_time     args((CHALLENGER_DATA *challenger, time_t time));
SCORE_DATA      *find_score_by_opponent args((CHALLENGER_DATA *challenger, char *name));
SCORE_DATA      *find_score_by_won      args((CHALLENGER_DATA *challenger, bool won));
*/

/* recipe.c */
void    	load_recipes args((void));
void    	save_recipes args((void));
void    	list_recipe               	args((CHAR_DATA *ch));
void 		show_recipe        		args((CHAR_DATA *ch, RECIPE_DATA *recipe));
void 		edit_recipe        		args((CHAR_DATA *ch, char *argument));
void    	delete_recipe               	args((RECIPE_DATA *recipe));
void    	help_recipe               	args((CHAR_DATA *ch));
RECIPE_DATA 	*add_recipe         		args((char *name));
RECIPE_DATA 	*find_recipe        		args((char *name));
RECIPE_DATA 	*get_recipe        		args((int num));
char 		*get_name_ing			args((int num));

/* query.c */
void    	load_query args((void));
void    	save_query args((void));
void    	list_query               	args((CHAR_DATA *ch));
void 		show_query        		args((CHAR_DATA *ch, QUERY_DATA *query));
void 		edit_query        		args((CHAR_DATA *ch, char *argument));
void    	delete_query               	args((QUERY_DATA *query));
QUERY_DATA 	*add_query         		args((char *name));
QUERY_DATA 	*get_query        		args((int num));
bool 		check_query_obj			args((QUERY_DATA *query, OBJ_INDEX_DATA *obj));
bool 		check_query_mob			args((QUERY_DATA *query, MOB_INDEX_DATA *mob));

/* db.c */
void reset_area(AREA_DATA * pArea); /* OLC */
void reset_room(ROOM_INDEX_DATA *pRoom); /* OLC */
void boot_db(void);
void area_update(void);
CD * create_mobile(MOB_INDEX_DATA *pMobIndex);
void clone_mobile(CHAR_DATA *parent, CHAR_DATA *clone);
OD * create_object(OBJ_INDEX_DATA *pObjIndex, int level);
void clone_object(OBJ_DATA *parent, OBJ_DATA *clone);
void clear_char(CHAR_DATA *ch);
char * get_extra_descr(const char *name, EXTRA_DESCR_DATA *ed);
MID * get_mob_index(int vnum);
OID * get_obj_index(int vnum);
RID * get_room_index(int vnum);
PC * get_mprog_index(int vnum, int type);
HELP_DATA *get_help(char *keyword);
HELP_AREA *get_help_area(char *filename);
char fread_letter(FILE *fp);
int64_t fread_number(FILE *fp);
int64_t fread_flag(FILE *fp);
char * fread_string(FILE *fp);
char * fread_string_eol(FILE *fp);
void fread_to_eol(FILE *fp);
char * fread_word(FILE *fp);
int64_t flag_convert(char letter);
void * alloc_mem(unsigned int sMem);
void * alloc_perm(int sMem);
void free_mem(void *pMem, int sMem);

char * str_dup(const char *str);
void free_string(char *pstr);
int number_fuzzy(int number);
int number_range(int from, int to);
int number_percent(void);
int number_door(void);
int number_bits(int width);
long number_mm(void);
int dice(int number, int size);
int interpolate(int level, int value_00, int value_32);
void smash_tilde(char *str);
bool str_cmp(const char *astr, const char *bstr);
bool str_prefix(const char *astr, const char *bstr);
bool str_infix(const char *astr, const char *bstr);
bool str_suffix(const char *astr, const char *bstr);
char * capitalize(const char *str);
void append_file(CHAR_DATA *ch, char *file, char *str);
void bugf_internal(char *format, ...);
void log_string(const char *str);
void _log_string(const char *str, char *logname);
void tail_chain(void);
PROG_CODE *get_prog_index(int vnum, int type);
void _perror(const char *str);
char *str_str(const char *bstr, const char *astr);
void convert_dollars(char *s);

/* db2.c */
FILE *file_open(char *file, const char *mode);
bool file_close(FILE * fp);

/* effect.c */
void acid_effect(void *vo, int level, int dam, int target, CHAR_DATA *caster);
void cold_effect(void *vo, int level, int dam, int target, CHAR_DATA *caster);
void fire_effect(void *vo, int level, int dam, int target, CHAR_DATA *caster);
void poison_effect(void *vo, int level, int dam, int target, CHAR_DATA *caster);
void shock_effect(void *vo, int level, int dam, int target, CHAR_DATA *caster);

/* fight2.c */
char *makehowl(char *string, CHAR_DATA *ch);
void set_lycanthrope(CHAR_DATA *ch);
void recovery_lycanthrope(CHAR_DATA *ch);

/* fight.c */
bool is_safe(CHAR_DATA *ch, CHAR_DATA *victim);
bool is_safe_spell(CHAR_DATA *ch, CHAR_DATA *victim, bool area);
void violence_update(void);
void multi_hit(CHAR_DATA *ch, CHAR_DATA *victim, int dt);
int damage(CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt, int classid, bool show, OBJ_DATA *weapon);
int adv_dam(CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt, int classid, bool show, OBJ_DATA *weapon, bool aff, int gsn);
void update_pos(CHAR_DATA *victim);
void stop_fighting(CHAR_DATA *ch, bool fBoth);
void check_killer(CHAR_DATA *ch, CHAR_DATA *victim);
int group_level(CHAR_DATA *ch, int whatlevel);
void for_killed_skills(CHAR_DATA *ch, CHAR_DATA *victim);
void set_fighting(CHAR_DATA *ch, CHAR_DATA *victim);
char *get_style(CHAR_DATA *ch);
bool can_backstab(CHAR_DATA *ch, CHAR_DATA *victim);
void raw_kill(CHAR_DATA *victim, CHAR_DATA *killer, bool is_corpse);

void wait_state(CHAR_DATA *ch, int npulse);
void daze_state(CHAR_DATA *ch, int npulse);

/* handler.c */
int get_moon_phase(int moon);
void dump_container(OBJ_DATA *obj);
bool is_memory(CHAR_DATA *ch, CHAR_DATA *victim, int type);
void mob_forget(CHAR_DATA *ch, CHAR_DATA *victim, int type);
void mob_remember(CHAR_DATA *ch, CHAR_DATA *victim, int type);
char * material_name(int16_t num); /* OLC */
AD * affect_find(AFFECT_DATA *paf, int sn);
void affect_check(CHAR_DATA *ch, int where, int vector);
int count_users(OBJ_DATA *obj);
void deduct_cost(CHAR_DATA *ch, int cost);
void affect_enchant(OBJ_DATA *obj);
int16_t check_immune(CHAR_DATA *ch, int dam_type);
int weapon_type(const char *name);
char *weapon_name(int weapon_Type);
char *item_name(int item_type, int rname);
bool is_old_mob args ((CHAR_DATA *ch));
int get_skill(CHAR_DATA *ch, int sn);
int get_weapon_sn(CHAR_DATA *ch ,bool secondary);
int get_weapon_skill(CHAR_DATA *ch, int sn);
int get_age(CHAR_DATA *ch);
void reset_char(CHAR_DATA *ch) ;
int get_trust(CHAR_DATA *ch);
int get_curr_stat(CHAR_DATA *ch, int stat);
int get_max_train(CHAR_DATA *ch, int stat);
int can_carry_n(CHAR_DATA *ch);
int can_carry_w(CHAR_DATA *ch);
bool is_name(char *str, char *namelist);
bool is_exact_name(char *str, char *namelist);
void affect_to_char(CHAR_DATA *ch, AFFECT_DATA *paf);
void affect_to_obj(OBJ_DATA *obj, AFFECT_DATA *paf);
void affect_remove(CHAR_DATA *ch, AFFECT_DATA *paf);
void affect_remove_obj(OBJ_DATA *obj, AFFECT_DATA *paf);
void affect_strip(CHAR_DATA *ch, int sn);
bool is_lycanthrope(CHAR_DATA *ch);
bool is_lycanthrope_spirit(CHAR_DATA *ch);
bool is_affected(CHAR_DATA *ch, int sn);
void affect_join(CHAR_DATA *ch, AFFECT_DATA *paf);
void affect_join_obj(OBJ_DATA *obj, AFFECT_DATA *paf);
void affect_join_room(ROOM_INDEX_DATA *room, AFFECT_DATA *paf);
void affect_replace(CHAR_DATA *ch, AFFECT_DATA *paf);
void affect_replace_obj(OBJ_DATA *obj, AFFECT_DATA *paf);
void affect_replace_room(ROOM_INDEX_DATA *room, AFFECT_DATA *paf);
AFFECT_DATA *get_char_affect(CHAR_DATA *ch, int sn);
AFFECT_DATA *get_obj_affect(OBJ_DATA *obj, int sn);
AFFECT_DATA *get_room_affect(ROOM_INDEX_DATA *room, int sn);
void char_from_room(CHAR_DATA *ch);
void char_to_room(CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex, bool show);
#define obj_to_char(obj, ch)  obj_to_char_gen((obj), (ch), TRUE, NULL)
void obj_to_char_gen(OBJ_DATA *obj, CHAR_DATA *ch, bool top, OBJ_DATA *vobj);
void obj_from_char(OBJ_DATA *obj, bool check_wield);
int apply_ac(OBJ_DATA *obj, int iWear, int type);
OD * get_eq_char(CHAR_DATA *ch, int iWear);
void equip_char(CHAR_DATA *ch, OBJ_DATA *obj, int iWear);
void unequip_char(CHAR_DATA *ch, OBJ_DATA *obj, bool check_wield );
int count_obj_list(OBJ_INDEX_DATA *obj, OBJ_DATA *list);
void obj_from_room(OBJ_DATA *obj);
void obj_to_room(OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex);
void obj_to_obj(OBJ_DATA *obj, OBJ_DATA *obj_to);
void obj_from_obj(OBJ_DATA *obj);
void extract_obj(OBJ_DATA *obj, bool limit, bool check_wield);
void extract_char(CHAR_DATA *ch, bool fPull);
void extract_char_for_quit( CHAR_DATA *ch, bool show );
CD * get_char_room(CHAR_DATA *ch, ROOM_INDEX_DATA *room, char *argument, bool check);
CD * get_char_world(CHAR_DATA *ch, char *argument);
OD * get_obj_type(OBJ_INDEX_DATA *pObjIndexData);
OD * get_obj_list(CHAR_DATA *ch, char *argument, OBJ_DATA *list);
OD * get_obj_carry(CHAR_DATA *ch, char *argument, CHAR_DATA *viewer);
OD * get_obj_wear(CHAR_DATA *ch, char *argument, bool character);
OD * get_obj_here(CHAR_DATA *ch, ROOM_INDEX_DATA *room, char *argument);

OD * get_obj_world(CHAR_DATA *ch, char *argument);
OD * create_money(int gold, int silver, int bronze);
int get_obj_number(OBJ_DATA *obj);
int get_obj_weight(OBJ_DATA *obj);
int get_true_weight(OBJ_DATA *obj);
bool room_is_dark(ROOM_INDEX_DATA *pRoomIndex);
bool is_room_owner(CHAR_DATA *ch, ROOM_INDEX_DATA *room);
bool room_is_private(ROOM_INDEX_DATA *pRoomIndex, CHAR_DATA *mount);
bool can_see(CHAR_DATA *ch, CHAR_DATA *victim);
bool can_see_obj(CHAR_DATA *ch, OBJ_DATA *obj);
bool can_see_room(CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex);
bool can_drop_obj(CHAR_DATA *ch, OBJ_DATA *obj);
void convert_name(char *argument);
bool check_channels(CHAR_DATA *ch);
bool check_freeze(CHAR_DATA *ch);
RID * get_recall(CHAR_DATA *ch);
TRACK_DATA *get_track(CHAR_DATA *ch, CHAR_DATA *victim);
void add_track(CHAR_DATA *ch, int direction);
bool is_animal(CHAR_DATA *victim);

bool is_wear_aggravate(CHAR_DATA *ch);
bool is_wear_teleport(CHAR_DATA *ch);
bool is_wear_slow_digestion(CHAR_DATA *ch);
void teleport_curse(CHAR_DATA *ch);

char * hours(unsigned int num, int flag);
long free_space(void);
CD * get_master(CHAR_DATA *ch);
DESCRIPTOR_DATA *get_original(CHAR_DATA *ch);
void affect_to_room(ROOM_INDEX_DATA *room, AFFECT_DATA *paf);
void affect_remove_room(ROOM_INDEX_DATA *room, AFFECT_DATA *paf);

OBJ_DATA *get_trap(ROOM_INDEX_DATA *room);
bool check_trap(CHAR_DATA *ch);
bool check_trap_obj(CHAR_DATA *ch, OBJ_DATA *obj);
bool check_trap_exit(CHAR_DATA *ch, EXIT_DATA *exit);
RID  *  get_random_room args ((CHAR_DATA *ch));

int  TNL(CHAR_DATA *ch);
bool is_room_affected(ROOM_INDEX_DATA *room, int sn);
int  get_max_stat(CHAR_DATA *ch, int stat);

void affect_copy(OBJ_DATA *obj, AFFECT_DATA *paf);

int check_wear_stat(CHAR_DATA *ch, OBJ_DATA *obj);

bool is_penalty_room(ROOM_INDEX_DATA *room);

int UMIN(int a, int b);
int UMAX(int a, int b);
int URANGE(int a, int b, int c);

bool check_wield_weight(CHAR_DATA *ch, OBJ_DATA *wield, bool secondary);
bool can_take_weight(CHAR_DATA *ch, int weight);

char *get_align(CHAR_DATA *gch, CHAR_DATA *ch);
int16_t get_resist(CHAR_DATA *ch, int dam_type);
int16_t get_max_resist(CHAR_DATA *ch, int dam_type);

long get_id(CHAR_DATA *ch);

bool is_nopk(CHAR_DATA *ch);
bool is_unusable(CHAR_DATA *ch, OBJ_DATA *obj);
bool is_visited(CHAR_DATA *ch, int vnum);
bool check_charmees(CHAR_DATA *ch, CHAR_DATA *victim);
void check_light_status(CHAR_DATA *ch);
bool has_trigger(PROG_LIST *target, int trig, int64_t when);

/*
 * Colour Config
 */
void default_colour(CHAR_DATA *ch);
void all_colour(CHAR_DATA *ch, char *argument);

/* time.c */
char    *c_time         args((const time_t *t));
char    *asc_time       args((const struct tm *tp));

/* interp.c */
void interpret(CHAR_DATA *ch, char *argument);
bool is_number(char *arg);
int number_argument(char *argument, char *arg);
int mult_argument(char *argument, char *arg);
char * one_argument(char *argument, char *arg_first);
char * is_granted(CHAR_DATA *ch, char *command);
bool is_spec_granted(CHAR_DATA *ch, char *spec);
bool check_social(CHAR_DATA *ch, char *command, char *argument);
int compare_strings (const void *v1, const void *v2);

/* magic.c */
int find_spell(CHAR_DATA *ch, const char *name);
bool self_magic(CHAR_DATA *ch, CHAR_DATA *victim);
int mana_cost(CHAR_DATA *ch, int min_mana, int level);
bool saves_spell(int level, CHAR_DATA *victim, int dam_type);
bool max_count_charmed(CHAR_DATA *ch);

extern char *target_name;

/* mob_prog.c */

static char * const he_she  [] = { "оно",  "он",  "она", "они" };
static char * const him_her [] = { "ему",  "ему", "ей", "им" };
static char * const his_her [] = { "его", "его", "ее", "их" };


int program_flow(int vnum, char *source, CHAR_DATA *mob,
     OBJ_DATA *obj, ROOM_INDEX_DATA *room,
     CHAR_DATA *ch, const void *arg1,
     const void *arg2);
void p_act_trigger(char *argument, CHAR_DATA *mob,
           OBJ_DATA *obj, ROOM_INDEX_DATA *room,
       CHAR_DATA *ch, const void *arg1,
       const void *arg2, int type);
bool p_percent_trigger(CHAR_DATA *mob, OBJ_DATA *obj,
           ROOM_INDEX_DATA *room, CHAR_DATA *ch,
           const void *arg1, const void *arg2, int type);
void p_bribe_trigger(CHAR_DATA *mob, CHAR_DATA *ch, int amount);
bool p_exit_trigger(CHAR_DATA *ch, int dir, int type);
void p_give_trigger(CHAR_DATA *mob, OBJ_DATA *obj,
        ROOM_INDEX_DATA *room, CHAR_DATA *ch,
        OBJ_DATA *dropped, int type);
void p_greet_trigger(CHAR_DATA *ch, int type);
void p_hprct_trigger(CHAR_DATA *mob, CHAR_DATA *ch);
void p_wear_trigger(OBJ_DATA *obj, CHAR_DATA *ch);
int p_command_trigger(CHAR_DATA *ch, const char *cmd, char *args, int type);


/* mob_cmds.c */
void mob_interpret(CHAR_DATA *ch, char *argument);
void obj_interpret(OBJ_DATA *obj, char *argument);
void room_interpret(ROOM_INDEX_DATA *room, char *argument);

/* save.c */
void save_char_obj(CHAR_DATA *ch, bool inval);
bool load_char_obj(DESCRIPTOR_DATA *d, char *name, bool beac, bool equip);

/* skills.c */
bool parse_gen_groups(CHAR_DATA *ch,char *argument);
void list_group_costs(CHAR_DATA *ch);
void list_group_known(CHAR_DATA *ch);
int exp_per_level(CHAR_DATA *ch, int points);
void check_improve(CHAR_DATA *ch, CHAR_DATA *victim, int sn, bool success,
       int multiplier);
void gn_add(CHAR_DATA *ch, int gn);
void gn_remove(CHAR_DATA *ch, int gn);
void group_add(CHAR_DATA *ch, const char *name, bool deduct);
void group_remove(CHAR_DATA *ch, const char *name);
int get_skill_rating(CHAR_DATA *ch, int skill);
char *get_skill_name(CHAR_DATA *ch, int sn, bool is_skill);

/* special.c */
char * spec_name(SPEC_FUN *function);

/* teleport.c */
RID * room_by_name(char *target, int level, bool error);

/* update.c */
void advance_level(CHAR_DATA *ch, bool hide);
void gain_exp(CHAR_DATA *ch, int gain, bool silent);
void gain_condition(CHAR_DATA *ch, int iCond, int value);
void update_handler(void);
void	weather_update(bool moons);

/* string.c */
void string_edit(CHAR_DATA *ch, char **pString);
void string_append(CHAR_DATA *ch, char **pString);
char * string_replace(char * orig, char * old, char * newstr);
void string_add(CHAR_DATA *ch, char *argument);
char * format_string(char *oldstring /*, bool fSpace */);
char * first_arg(char *argument, char *arg_first, bool fCase);
char * string_unpad(char * argument);
char * string_remspaces(char * argument);
char * string_proper(char * argument);
char * decompose_end(char *name);

/* olc.c */
bool run_olc_editor(DESCRIPTOR_DATA *d);
char *olc_ed_name(CHAR_DATA *ch);
char *olc_ed_vnum(CHAR_DATA *ch);


char *_fwrite_flag(int64_t flags, char *file, int line);

#define fwrite_flag(flags)     _fwrite_flag(flags, __FILE__, __LINE__ )

/*char *  fwrite_flag  args((int64_t flags)); */

/*
 * Improved mobile AI... Heh-heh! 8))
 */
void mob_ai(CHAR_DATA *ch);

#undef  CD
#undef  MID
#undef  OD
#undef  OID
#undef  RID
#undef AD


/*****************************************************************************
 *                                    OLC                                    *
 *****************************************************************************/

/*
 * Object defined in limbo.are
 * Used in save.c to load objects that don't exist.
 */
#define OBJ_VNUM_DUMMY  30

/*
 * Area flags.
 */
#define         AREA_NONE         0x00
#define         AREA_CHANGED      A  /* Area has been modified.          */
#define         AREA_ADDED        B  /* Area has been added to.       */
#define         AREA_LOADING      C  /* Used for counting in db.c       */
#define   	AREA_NA           D  /* Area is under construction,      */
            /* no access for mortals      */
#define    	AREA_TESTING      E  /* Area is under testing, no stuff  */
            /* can be carried outside this area */
#define    	AREA_NOQUEST      F
#define    	AREA_NOPOPULARITY G
#define    	AREA_NOLEGEND     H
#define    	AREA_FASTREPOP    I
#define    	AREA_EXTERN_EDIT  J


#define NO_FLAG -99  /* Must not be used in flags or stats. */

/*
 * Global Constants
 */

extern int area_version; /* db.c */

extern char * const dir_name[];
extern const int16_t rev_dir[];          /* int16_t - ROM OLC */
extern const struct spec_type spec_table[];

/*
 * Global variables
 */
extern AREA_DATA *area_first;
extern AREA_DATA *area_last;
extern SHOP_DATA *shop_last;

extern int top_affect;
extern int top_area;
extern int top_ed;
extern int top_exit;
extern int top_help;
extern int top_mob_index;
extern int top_obj_index;
extern int top_reset;
extern int top_room;
extern int top_shop;

extern int top_vnum_mob;
extern int top_vnum_obj;
extern int top_vnum_room;
extern int top_vnum;

extern char str_empty[1];

extern MOB_INDEX_DATA  *mob_index_hash[MAX_KEY_HASH];
extern OBJ_INDEX_DATA  *obj_index_hash[MAX_KEY_HASH];
extern ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];

extern bool skills_changed;
extern bool groups_changed;
extern bool races_changed;
extern bool classes_changed;

/* act_wiz.c */
ROOM_INDEX_DATA *find_location(CHAR_DATA *ch, char *arg);


/* db.c */
void reset_area(AREA_DATA * pArea);
void reset_room(ROOM_INDEX_DATA *pRoom);

/* string.c */
void string_edit(CHAR_DATA *ch, char **pString);
void string_append(CHAR_DATA *ch, char **pString);
char * string_replace(char * orig, char * old, char * new_str);
void string_add(CHAR_DATA *ch, char *argument);
char * format_string(char *oldstring /*, bool fSpace */);
char * first_arg(char *argument, char *arg_first, bool fCase);
char * string_unpad(char * argument);
char * string_proper(char * argument);
char * cases(char *phrase, int num);
int strlen_color(char *argument);
char * str_color_fmt(char *argument, int fmt);
char * color_to_str(char *argument);

/* olc.c */
bool run_olc_editor(DESCRIPTOR_DATA *d);
char *olc_ed_name(CHAR_DATA *ch);
char *olc_ed_vnum(CHAR_DATA *ch);

/* special.c */
char * spec_string(SPEC_FUN *fun); /* OLC */

/* tables.c */

struct flag_stat_type
{
    char *name;
    const struct flag_type *structure;
    bool stat;
};

extern const struct flag_stat_type flag_stat_table[];

/*
 * Contributed by Alander.
 */


typedef struct string_data STRING_DATA;

struct string_data
{
  char *name;
};


/* game tables */
extern const struct position_type position_table[];
extern const struct sex_type      sex_table[];
extern const struct size_type     size_table[];

/* flag tables */
extern const struct flag_type  act_flags[];
extern const struct flag_type  plr_flags[];
extern const struct flag_type  affect_flags[];
extern const struct flag_type  off_flags[];
extern const struct flag_type  imm_flags[];
extern const struct flag_type  form_flags[];
extern const struct flag_type  part_flags[];
extern const struct flag_type  comm_flags[];
extern const struct flag_type  extra_flags[];
extern const struct flag_type  wear_flags[];
extern const struct flag_type  weapon_flags[];
extern const struct flag_type  container_flags[];
extern const struct flag_type  chest_flags[];
extern const struct flag_type  ing_sector_flags[];	/*ингредиенты - сектор*/
extern const struct flag_type  ing_weather_flags[];	/*ингредиенты - погода*/
extern const struct flag_type  portal_flags[];
extern const struct flag_type  room_flags[];
extern const struct flag_type  exit_flags[];
extern const struct flag_type  mprog_flags[];
extern const struct flag_type  area_flags[];
extern const struct flag_type  sector_flags[];
extern const struct flag_type  door_resets[];
extern const struct flag_type  wear_loc_strings[];
extern const struct flag_type  wear_loc_flags[];
extern const struct flag_type  res_flags[];
extern const struct flag_type  imm_flags[];
extern const struct flag_type  vuln_flags[];
extern       struct flag_type  type_flags[];
extern const struct flag_type  apply_flags[];
extern const struct flag_type  room_apply_flags[];
extern const struct flag_type  sex_flags[];
extern const struct flag_type  furniture_flags[];
extern       struct flag_type  weapon_class[];
extern const struct flag_type  apply_types[];
extern const struct flag_type  weapon_type2[];
extern const struct flag_type  apply_types[];
extern const struct flag_type  size_flags[];
extern const struct flag_type  position_flags[];
extern const struct flag_type  ac_type[];
extern const struct bit_type  bitvector_type[];
extern const struct flag_type  oprog_flags[];
extern const struct flag_type  rprog_flags[];
extern const struct material     material_waste[];
extern const struct flag_type    material_table[];
extern const struct flag_type  target_flags[];
extern const struct flag_type  attr_flags[];
extern const struct flag_type   dam_flags[];
extern const struct flag_type   saves_flags[];
extern const struct flag_type   spell_flags[];
extern const struct flag_type   spaff_flags[];
extern const struct flag_type   align_flags[];
extern const struct flag_type   where_flags[];
extern const struct flag_type   color_flags[];
extern const struct flag_type	comf_flags[];
extern const struct flag_type	trap_flags[];
extern const struct flag_type	exit_dirs[];
extern const struct flag_type	trig_flags[];
extern const struct flag_type	color_type_flags[];
extern const struct flag_type	magic_class_flags[];
extern const struct flag_type   prog_when_flags[];

char *flag_string(const struct flag_type *flag_table, int64_t bits, bool rname);
char *fflag_string(const struct flag_type *flag_table, int64_t bits, bool rname, bool empty_none);


/*****************************************************************************
 *                                 OLC END                                   *
 *****************************************************************************/

bool room_is_dark_new(CHAR_DATA *ch);

/* drunk.c */
char *makedrunk(char *string, CHAR_DATA *ch);

/* mount.c */
bool mount_success(CHAR_DATA *ch, CHAR_DATA *mount, bool canattack, bool improve);
void do_mount(CHAR_DATA *ch, char *argument);
void do_dismount(CHAR_DATA *ch, char *argument);
void do_buy_mount(CHAR_DATA *ch, char *argument);
CHAR_DATA *mounted(CHAR_DATA *ch);
CHAR_DATA *ridden(CHAR_DATA *ch);

/* mob_ai.c */
OBJ_DATA *get_best_object(OBJ_DATA *obj1, OBJ_DATA *obj2);

/* limit.c */
int is_limit(OBJ_DATA *obj);
void return_limit_from_char(CHAR_DATA *ch, bool extract);
bool is_have_limit(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj);
bool container_have_limit(OBJ_DATA *container);
bool check_container_limit(OBJ_DATA *container, OBJ_DATA *obj);
bool check_reset_for_limit(OBJ_DATA *obj, bool clone);
bool check_prog_limit(OBJ_INDEX_DATA *pObjIndex);

/* mccp.c */
bool compressStart(DESCRIPTOR_DATA *desc, bool v2);
bool compressEnd(DESCRIPTOR_DATA *desc);
bool processCompressed(DESCRIPTOR_DATA *desc);
bool writeCompressed(DESCRIPTOR_DATA *desc, char *txt, int length);

/* lookup.c */
CHAR_DATA *pc_id_lookup(long id);
int clan_lookup(const char *name);
int position_lookup(const char *name);
int sex_lookup(const char *name);
int size_lookup(const char *name);
int64_t flag_lookup(const char *name, const struct flag_type *flag_table);
int material_lookup(const char *name);
int weapon_lookup(const char *name);
int attack_lookup_dam  (int dt);
int attack_lookup(const char *name);
int wiznet_lookup(const char *name);
int class_lookup(const char *name);
int skill_lookup(const char *name);
int slot_lookup(int slot);
int group_lookup(const char *name);
SPEC_FUN *spec_lookup(const char *name);
int race_lookup(const char *name);
int item_lookup(const char *name);
int liq_lookup(const char *name);
int wear_loc_lookup(const char *name);

/* flags.c */
int64_t flag_value(const struct flag_type *flag_table, char *argument);


#define rdns_cache

void rdns_cache_set_ttl(int new_ttl);

char * gethostname_cached(const unsigned char *addr, int len, int ttl_refresh);





#if !defined(GSN_H)
#define GSN_H

#if defined(IN_DB_C)
#if defined(GSN)
#undef GSN
#endif
#define  GSN(gsn) int16_t gsn

#else
#if defined(GSN)
#undef GSN
#endif
#define  GSN(gsn) extern int16_t gsn
#endif
#endif
/*
 * These are skill_lookup return values for common skills and spells.
 */

GSN(gsn_fade);
GSN(gsn_unpeek);
GSN(gsn_hoof);
GSN(gsn_steal_horse);
GSN(gsn_ambush_move);
GSN(gsn_torch);
GSN(gsn_camouflaging);
GSN(gsn_detect_trap);
GSN(gsn_trap);
GSN(gsn_find_spring);
GSN(gsn_butcher);
GSN(gsn_cancellation);
GSN(gsn_thornwrack);
GSN(gsn_herbs);
GSN(gsn_track);
GSN(gsn_ambush);
GSN(gsn_camp);
GSN(gsn_camouflage_move);
GSN(gsn_camouflage);
GSN(gsn_rods);
GSN(gsn_ice_shield);
GSN(gsn_fire_shield);
GSN(gsn_coniferous_shield);
GSN(gsn_select);
GSN(gsn_leadership);
GSN(gsn_power_word_fear);
GSN(gsn_reinforce);
GSN(gsn_deafen);
GSN(gsn_care_style);
GSN(gsn_aggr_style);
GSN(gsn_fight_on_mount);
GSN(gsn_riding);
GSN(gsn_fight_off);
GSN(gsn_detect_hidden);
GSN(gsn_tail);
GSN(gsn_dual_backstab);
GSN(gsn_backstab);
GSN(gsn_dodge);
GSN(gsn_envenom);
GSN(gsn_hide);
GSN(gsn_peek);
GSN(gsn_pick_lock);
GSN(gsn_sneak);
GSN(gsn_steal);
GSN(gsn_disarm);
GSN(gsn_enhanced_damage);
GSN(gsn_kick);
GSN(gsn_parry);
GSN(gsn_rescue);
GSN(gsn_second_attack);
GSN(gsn_third_attack);
GSN(gsn_fourth_attack);
GSN(gsn_second_weapon);
GSN(gsn_lunge);
GSN(gsn_cleave);
GSN(gsn_blindness);
GSN(gsn_charm_person);
GSN(gsn_curse);
GSN(gsn_invis);
GSN(gsn_mass_invis);
GSN(gsn_plague);
GSN(gsn_poison);
GSN(gsn_sleep);
GSN(gsn_fly);
GSN(gsn_sanctuary);
GSN(gsn_power_kill);
GSN(gsn_axe);
GSN(gsn_dagger);
GSN(gsn_flail);
GSN(gsn_mace);
GSN(gsn_polearm);
GSN(gsn_shield_block);
GSN(gsn_spear);
GSN(gsn_sword);
GSN(gsn_whip);
GSN(gsn_staff);
GSN(gsn_bash);
GSN(gsn_berserk);
GSN(gsn_dirt);
GSN(gsn_hand_to_hand);
GSN(gsn_trip);
GSN(gsn_fast_healing);
GSN(gsn_haggle);
GSN(gsn_lore);
GSN(gsn_meditation);
GSN(gsn_scrolls);
GSN(gsn_staves);
GSN(gsn_wands);
GSN(gsn_recall);
GSN(gsn_sharpen);
GSN(gsn_levitate);
GSN(gsn_bite);
GSN(gsn_death_aura);
GSN(gsn_cursed_lands);
GSN(gsn_armor_use);
GSN(gsn_nocast);
GSN(gsn_gods_curse);
GSN(gsn_prayer);
GSN(gsn_grimuar);
GSN(gsn_timeout);

GSN(gsn_mist);
GSN(gsn_recovery);
GSN(gsn_hunger);
GSN(gsn_mirror_image);
GSN(gsn_cure_disease);
GSN(gsn_cure_poison);

GSN(gsn_run);
GSN(gsn_scribe);

GSN(gsn_holy_shield);
GSN(gsn_bandage);
GSN(gsn_maps);

GSN(gsn_shapechange);
GSN(gsn_exotic);
GSN(gsn_make_bag);

GSN(gsn_restore_mana);
GSN(gsn_secondary_mastery);

GSN(gsn_disarm_trap);
GSN(gsn_attract_attention);
GSN(gsn_dig);
GSN(gsn_sober);
GSN(gsn_blood_ritual);
GSN(gsn_concentrate);
/*GSNs of HARM*/
GSN(gsn_lighting_harm);
GSN(gsn_serious_harm);
GSN(gsn_critical_harm);
GSN(gsn_harm);
GSN(gsn_st_aura);
GSN(gsn_bless_forest);
GSN(gsn_diagnosis);
GSN(gsn_ghostaura);

GSN(gsn_pierce);

GSN(gsn_manaleak);
GSN(gsn_vaccine);
GSN(gsn_nets);
GSN(gsn_bleed);
GSN(gsn_deafenstrike);
GSN(gsn_lightning_bolt);
GSN(gsn_collection);
GSN(gsn_mix);
GSN(gsn_pound);
GSN(gsn_gritstorm);
GSN(gsn_protection_sphere);
GSN(gsn_protection_elements);
GSN(gsn_flame_cloud);
GSN(gsn_stinking_cloud);
GSN(gsn_acid_fog);
GSN(gsn_immolation);
GSN(gsn_slime);
GSN(gsn_dissolve);
GSN(gsn_earthmaw);

GSN(gsn_spirit_wolf);
GSN(gsn_spirit_bear);
GSN(gsn_form_wolf);
GSN(gsn_form_bear);
GSN(gsn_howl);
GSN(gsn_jump);
GSN(gsn_claws);
GSN(gsn_silent_step);
GSN(gsn_find_victim);
GSN(gsn_animal_dodge);
GSN(gsn_hair);
GSN(gsn_growl);
GSN(gsn_clutch_lycanthrope);

GSN(gsn_animal_taming);


GSN(gsn_faerie_fire);
GSN(gsn_weaken);
GSN(gsn_slow);

/* mxp stuff - added by Nick Gammon - 18 June 2001 */

/*
To simply using MXP we'll use special tags where we want to use MXP tags
and then change them to <, > and & at the last moment.

  eg. MXP_BEG "send" MXP_END    becomes: <send>
      MXP_AMP "version;"        becomes: &version;

*/

/* strings */
#define MXP_BEG "\x03"    /* becomes < */
#define MXP_END "\x04"    /* becomes > */
#define MXP_AMP "\x05"    /* becomes & */

/* characters */
#define MXP_BEGc '\x03'    /* becomes < */
#define MXP_ENDc '\x04'    /* becomes > */
#define MXP_AMPc '\x05'    /* becomes & */

/* constructs an MXP tag with < and > around it */
#define MXPTAG(arg) MXP_BEG arg MXP_END

#define ESC "\x1B"  /* esc character */

#define MXPMODE(arg) ESC "[" #arg "z"

/* flags for show_list_to_char */

enum {
  eItemNothing,   /* item is not readily accessible */
  eItemGet,     /* item on ground */
  eItemDrop,    /* item in inventory */
  eItemBid     /* auction item */
};

#define MXP_open 0   /* only MXP commands in the "open" category are allowed.  */
#define MXP_secure 1 /* all tags and commands in MXP are allowed within the line.  */
#define MXP_locked 2 /* no MXP or HTML commands are allowed in the line.  The line is not parsed for any tags at all.   */
#define MXP_reset 3  /* close all open tags */
#define MXP_secure_once 4  /* next tag is secure only */
#define MXP_perm_open 5   /* open mode until mode change  */
#define MXP_perm_secure 6 /* secure mode until mode change */
#define MXP_perm_locked 7 /* locked mode until mode change */

/* Chat stuff */
typedef struct chat_key    CHAT_KEY;
typedef struct chat_keys  CHAT_KEYS;
typedef struct reply    REPLY;

struct reply
{
    int   weight;
    char   *sent;
    int    race;
    int    sex;
    REPLY   *next;
    int    tmp;
};

struct chat_key
{
    char   *regexp;
    REPLY   *replys;
    CHAT_KEY  *next;
};

struct chat_keys
{
    CHAT_KEY  *list;
    CHAT_KEYS  *next;
    int    vnum;
    AREA_DATA  *area;
};

#define CHAT_TELL  1
#define CHAT_SAY  2
#define CHAT_EMOTE  3

char *dochat(CHAR_DATA *ch, char *msg, CHAR_DATA *victim);
CHAT_KEYS *get_chat_index(int vnum);

CHAT_KEYS  *chatkeys_list;


#if defined (NO_STRL_FUNCS)
/* implementation in string.c */
size_t strlcpy(char *dst, const char *src, size_t siz);
size_t strlcat(char *dst, const char *src, size_t siz);
#endif

struct damres_t
{
    int dam;
    int64_t res;
};

const struct damres_t damres[DAM_MAX];

#define bugf(fmt, args...)	bugf_internal("%s:%d:%s():: " fmt, __FILE__, __LINE__, __FUNCTION__ , ## args)

#endif /* MERC_H */

/* charset=cp1251 */

