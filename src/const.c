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
#include <time.h>
#include "merc.h"
#include "magic.h"
#include "interp.h"


char *	const	dir_name	[]		=
{
    "север", "восток", "юг", "запад", "верх", "низ"
};

/* item type list */

const struct item_type item_table[MAX_ITEM_TYPE] =
{
    {	ITEM_LIGHT,	"light",	"свет"			},
    {	ITEM_SCROLL,	"scroll",	"свиток"		},
    {	ITEM_WAND,	"wand",		"волшебная_палочка"	},
    {   ITEM_STAFF,	"staff",	"жезл"			},
    {   ITEM_WEAPON,	"weapon",	"оружие"		},
    {   ITEM_TREASURE,	"treasure",	"драгоценность"		},
    {   ITEM_ARMOR,	"armor",	"броня"			},
    {	ITEM_POTION,	"potion",	"зелье"			},
    {	ITEM_CLOTHING,	"clothing",	"одежда"		},
    {   ITEM_FURNITURE,	"furniture",	"обстановка"		},
    {	ITEM_TRASH,	"trash",	"мусор"			},
    {	ITEM_CONTAINER,	"container",	"контейнер"		},
    {	ITEM_DRINK_CON, "drink",	"выпивка"		},
    {	ITEM_KEY,	"key",		"ключ"			},
    {	ITEM_FOOD,	"food",		"еда"			},
    {	ITEM_MONEY,	"money",	"деньги"		},
    {	ITEM_BOAT,	"boat",		"плавсредство"		},
    {	ITEM_CORPSE_NPC,"npc_corpse",	"труп_моба"		},
    {	ITEM_CORPSE_PC,	"pc_corpse",	"труп_игрока"		},
    {   ITEM_FOUNTAIN,	"fountain",	"источник_воды"		},
    {	ITEM_PILL,	"pill",		"пилюля"		},
    {	ITEM_PROTECT,	"protect",	"protect"		},
    {	ITEM_MAP,	"map",		"карта"			},
    {	ITEM_PORTAL,	"portal",	"портал"		},
    {	ITEM_WARP_STONE,"warp_stone",	"warp_stone"		},
    {	ITEM_ROOM_KEY,	"room_key",	"room_key"		},
    {	ITEM_GEM,	"gem",		"драгоценный_камень"	},
    {	ITEM_JEWELRY,	"jewelry",	"ювелирное_изделие"	},
    {   ITEM_JUKEBOX,	"jukebox",	"музыкальная_шкатулка"	},
    {   ITEM_ROD,	"rod",		"прут"			},
    {   ITEM_TRAP,	"trap",		"ловушка"		},
    {   ITEM_SCALE,	"scales",	"весы"			},
    {   ITEM_ARTIFACT,	"artifact",	"артефакт"		},
    {   ITEM_BOOK,	"book",		"книга"			},
    {   ITEM_CHEST,	"chest",	"сундук"		},
    {   ITEM_MORTAR,	"mortar",	"ступа"			},
    {   ITEM_INGREDIENT,"ingredient",	"ингредиент"		},
    {   0,		NULL,		NULL			}
};


/* weapon selection table */
const struct weapon_type weapon_table [MAX_WEAPON_TYPE + 1] =
{
   {"sword",   "меч",	   OBJ_VNUM_SCHOOL_SWORD,  WEAPON_SWORD,  &gsn_sword, 	TRUE },
   {"mace",    "булава",   OBJ_VNUM_SCHOOL_MACE,   WEAPON_MACE,   &gsn_mace, 	TRUE },
   {"dagger",  "кинжал",   OBJ_VNUM_SCHOOL_DAGGER, WEAPON_DAGGER, &gsn_dagger,	TRUE },
   {"axe",     "топор",    OBJ_VNUM_SCHOOL_AXE,    WEAPON_AXE,    &gsn_axe,	TRUE },
   {"spear",   "копье",    OBJ_VNUM_SCHOOL_SPEAR,  WEAPON_SPEAR,  &gsn_spear,	TRUE },
   {"flail",   "кистень",  OBJ_VNUM_SCHOOL_FLAIL,  WEAPON_FLAIL,  &gsn_flail,	TRUE },
   {"whip",    "кнут",	   OBJ_VNUM_SCHOOL_WHIP,   WEAPON_WHIP,   &gsn_whip,	TRUE },
   {"polearm", "алебарда", OBJ_VNUM_SCHOOL_POLEARM,WEAPON_POLEARM,&gsn_polearm,	TRUE },
   {"staff",   "посох",    OBJ_VNUM_SCHOOL_STAFF,  WEAPON_STAFF,  &gsn_staff,	TRUE },
   {"exotic",  "экзотическое", 0, 		   WEAPON_EXOTIC, &gsn_exotic,	FALSE},
   {NULL,	NULL,	   0,			   0,	          NULL,		FALSE}
};

/* Таблица комнат наказания */
const struct penalty_rooms_type penalty_rooms_table[MAX_PENALTYROOMS] =
{
    {	"Зал имен",		30696	},
    {	"Зал безликих",		30697	},
    {	"Тюрьма",		30699	},
    {	"Камера пыток",		15170	},
    {	"Царство Смерти",	 1212	}
};

/* wiznet table and prototype for future flag setting */
const struct wiznet_type wiznet_table[] =
{
   {    "on",           WIZ_ON,         IM },
   {    "prefix",	WIZ_PREFIX,	IM },
   {    "ticks",        WIZ_TICKS,      IM },
   {    "logins",       WIZ_LOGINS,     IM },
   {    "sites",        WIZ_SITES,      IM },
   {    "links",        WIZ_LINKS,      IM },
   {	"newbies",	WIZ_NEWBIE,	IM },
   {	"spam",		WIZ_SPAM,	L5 },
   {    "deaths",       WIZ_DEATHS,     IM },
   {    "resets",       WIZ_RESETS,     L4 },
   {    "mobdeaths",    WIZ_MOBDEATHS,  L4 },
   {    "flags",	WIZ_FLAGS,	L5 },
   {	"penalties",	WIZ_PENALTIES,	L5 },
   {	"saccing",	WIZ_SACCING,	L5 },
   {	"levels",	WIZ_LEVELS,	IM },
   {	"load",		WIZ_LOAD,	L2 },
   {	"restore",	WIZ_RESTORE,	L2 },
   {	"snoops",	WIZ_SNOOPS,	L2 },
   {	"switches",	WIZ_SWITCHES,	L2 },
   {	"secure",	WIZ_SECURE,	L1 },
   {	"bugs",		WIZ_BUGS,	IM },
   {	"memcheck",	WIZ_MEMCHECK,	L7 },
   {	"logging",	WIZ_LOG,	L5 },
   {	NULL,		0,		0  }
};

/* attack table  -- not very organized :( */

const struct attack_type attack_table[MAX_DAMAGE_MESSAGE] =
{
    {
	"none",	  	"удар",			"твой", 4,
	"Ты%s ударяешь $N3 $p4%c",
	"$n%s ударяет %s $p4%c",
	"Твой удар не достает $N3.",
	"Удар $n1 не достает %s.",
	"$N3",
	-1
    },
    {
	"slice",	"режущий удар",		"твой", 4,
	"Ты%s режешь $N3 $p4%c",
	"$n%s режет %s $p4%c",
	"Ты полосуешь воздух возле $N1.",
	"$n полосует воздух возле %s.",
        "$N1",
	DAM_SLASH
    },
    {
	"stab",		"укол",			"твой", 4,
	"Ты%s колешь $N3 $p4%c",
	"$n%s колет %s $p4%c",
	"Ты пронзаешь воздух возле $N1.",
	"$n пронзает воздух возле %s.",
        "$N1",
	DAM_PIERCE
    },
    {
	"slash",	"рубящий удар",		"твой", 4,
	"Ты%s рубишь $N3 $p4%c",
	"$n%s рубит %s $p4%c",
	"Ты со свистом рубишь воздух возле $N1.",
	"$n со свистом рубит воздух возле %s.",
        "$N1",
	DAM_SLASH
    },
    {
	"whip",		"хлесткий удар",	"твой", 4,
	"Ты%s хлещешь $N3 $p4%c",
	"$n%s хлещет %s $p4%c",
	"Ты хлещешь землю около $N1.",
	"$n хлещет землю около %s.",
        "$N1",
	DAM_SLASH
    },
    {
	"claw",		"рвущий удар",		"твой", 4,
	"Ты%s раздираешь $N3 $p4%c",
	"$n%s раздирает %s $p4%c",
	"Твой удар $p4 не достигает $N3.",
	"Удар $n1 $p4 не достигает %s.",
        "$N3",
	DAM_SLASH
    },  /*  5 */
    {
	"blast",	"взрыв",		"твой", 1,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	DAM_BASH
    },
    {
	"pound",	"тяжелый удар",		"твой", 4,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	DAM_BASH
    },
    {
	"crush",	"сдавливание",		"твое", 1,
	"Ты%s сдавливаешь $N3 $p4%c",
	"$n%s сдавливает %s $p4%c",
	"Ты сдавлишаешь лишь пустоту около $N1.",
	"$n сдавливает лишь пустоту около %s.",
        "$N1",
	DAM_BASH
    },
    {
	"grep",		"секущий удар",		"твой", 4,
	"Ты%s сечешь $N3 $p4%c",
	"$n%s сечет %s $p4%c",
	"Ты впустую рассекаешь воздух возле $N1.",
	"$n впустую рассекает воздух возле %s.",
        "$N1",
	DAM_SLASH
    },
    {
	"bite",		"укус",			"твой", 4,
	"Ты%s кусаешь $N3 $p4%c",
	"$n%s кусает %s $p4%c",
	"Твой укус $p4 промахивается мимо $N1.",
	"$n промахивается мимо %s.",
        "$N1",
	DAM_PIERCE
    },  /* 10 */
    {
	"pierce",	"выпад",		"твой", 4,
	"Ты%s колешь $N3 $p4%c",
	"$n%s колет %s $p4%c",
	"Ты пронзаешь воздух возле $N1.",
	"$n пронзает воздух возле %s.",
        "$N1",
	DAM_PIERCE
    },
    {
	"suction",	"засос",		"твой", 4,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	DAM_BASH
    },
    {
	"beating",	"удар",			"твой", 4,
	"Ты%s ударяешь $N3 $p4%c",
	"$n%s ударяет %s $p4%c",
	"Твой удар не достает $N3.",
	"Удар $n1 не достает %s.",
        "$N3",
	DAM_BASH
    },
    {
	"digestion",	"пищеварение",		"твое", 1,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	DAM_ACID
    },
    {
	"charge",	"толчок",		"твой", 4,
	"Ты%s толкаешь $N3 $p4%c",
	"$n%s толкает %s $p4%c",
	"Твой толчок $p4 промахивается мимо $N1.",
	"Толчок $n3 проходит мимо %s.",
        "$N1",
	DAM_BASH
    },  /* 15 */
    {
	"slap",		"шлепок",		"твой", 4,
	"Ты%s шлепаешь $N3 $p4%c",
	"$n%s шлепает %s $p4%c",
	"Ты шлепаешь воздух рядом с $N4.",
	"$n шлепает воздух возле %s.",
        "$N1",
	DAM_BASH
    },
    {
	"punch",	"кулак",		"твой", 1,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	DAM_BASH
    },
    {
	"wrath",	"гнев",			"твой", 1,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	DAM_ENERGY
    },
    {
	"magic",	"магия",		"твоя", 1,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	DAM_ENERGY
    },
    {
	"divine",	"божественная сила",	"твоя", 1,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	DAM_HOLY
    },  /* 20 */
    {
	"cleave",	"рассекающий удар",	"твой", 4,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	DAM_SLASH
    },
    {
	"scratch",	"царапающий удар",	"твой", 4,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	DAM_PIERCE
    },
    {
	"peck",		"удар клювом",		"твой", 4,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	DAM_PIERCE
    },
    {
	"peckb",	"удар клювом",		"твой", 4,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	DAM_BASH
    },
    {
	"chop",		"взмах",		"твой", 4,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	DAM_SLASH
    },  /* 25 */
    {
	"sting",	"жало",			"твое", 1,
	"Ты%s жалишь $N3 $p4%c",
	"$n%s жалит %s $p4%c",
	"Жало $p1 промахивается мимо $N1.",
	"Жало $n1 промахивается мимо %s.",
        "$N1",
	DAM_PIERCE
    },
    {
	"smash",	"сокрушительный удар",	"твой", 4,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	DAM_BASH
    },
    {
	"shbite",	"электрический удар",	"твой", 1,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	DAM_LIGHTNING
    },
    {
	"flbite",	"огненный удар",	"твой", 4,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	DAM_FIRE
    },
    {
	"frbite",	"ледяной удар",		"твой", 1,
	NULL,
	NULL,
	NULL,
	NULL,
        NULL,
	DAM_COLD
    },  /* 30 */
    {
	"acbite",	"кислота",		"твоя", 1,
	NULL,
	NULL,
	NULL,
	NULL,
        NULL,
	DAM_ACID
    },
    {
	"chomp",	"чавканье",		"твое", 1,
	NULL,
	NULL,
	NULL,
	NULL,
        NULL,
	DAM_PIERCE
    },
    {
	"drain",	"утечка энергии",	"твоя", 1,
	NULL,
	NULL,
	NULL,
	NULL,
        NULL,
	DAM_NEGATIVE
    },
    {
	"thrust",	"колющий удар",		"твой", 4,
	NULL,
	NULL,
	NULL,
	NULL,
        NULL,
	DAM_PIERCE
    },
    {
	"slime",	"слизь",		"твоя", 1,
	NULL,
	NULL,
	NULL,
	NULL,
        NULL,
	DAM_ACID
    },
    {
	"shock",	"шок",			"твой", 1,
	NULL,
	NULL,
	NULL,
	NULL,
        NULL,
	DAM_LIGHTNING
    },
    {
	"thwack",	"сильный удар",		"твой", 4,
	NULL,
	NULL,
	NULL,
	NULL,
        NULL,
	DAM_BASH
    },
    {
	"flame",	"пламя",		"твое", 1,
	"Ты%s обжигаешь $N3 $p4%c",
	"$n%s обжигает %s $p4%c",
	"Твое пламя $p1 проходит мимо $N1.",
	"Струя пламени $n3 проходит мимо %s.",
        "$N1",
	DAM_FIRE
    },
    {
	"chill",	"холод",		"твой", 1,
	NULL,
	NULL,
	NULL,
	NULL,
        NULL,
	DAM_COLD
    },
    {
	"light",	"вспышка света",	"твоя", 1,
	NULL,
	NULL,
	NULL,
	NULL,
        NULL,
	DAM_LIGHT
    },
    {
	"sound",	"звенящий звук",	"твой", 1,
	NULL,
	NULL,
	NULL,
	NULL,
        NULL,
	DAM_SOUND
    },
    {
	NULL,		NULL,			NULL, 0,
	NULL,
	NULL,
	NULL,
	NULL,
        NULL,
	0
    }
};

struct race_type *race_table;
struct race_type *pc_race_table;

const struct race_incompatible race_incompatible_table[] =
{
  { RACE_HUMAN,			RACE_SNAKE	},
  { RACE_HUMAN, 		RACE_VAMPIRE	},
  { RACE_LYCANTHROPE, 	RACE_VAMPIRE	},
  { RACE_ELF,			RACE_ORC	},
  { RACE_ELF,			RACE_TROLL	},
  { RACE_ELF,			RACE_DROW	},
  { RACE_ELF,			RACE_VAMPIRE	},
  { RACE_DWARF,			RACE_TROLL	},
  { RACE_DWARF,			RACE_ORC	},
  { RACE_DWARF, 		RACE_VAMPIRE	},
  { RACE_GIANT, 			RACE_VAMPIRE	},
  { RACE_ORC,			RACE_DROW	},
  { RACE_ORC,			RACE_VAMPIRE	},
  { RACE_TROLL, 			RACE_VAMPIRE	},
  { RACE_SNAKE, 		RACE_VAMPIRE	},
  { RACE_HOBBIT,		RACE_VAMPIRE	},
  { RACE_DROW,			RACE_VAMPIRE	},
  { RACE_ZOMBIE,		RACE_HOBBIT	},
  { RACE_ZOMBIE,		RACE_HUMAN	},
  { RACE_TIGGUAN,		RACE_VAMPIRE	},
  { RACE_TIGGUAN,		RACE_SNAKE	},  
  { 0,			0		}
};

/*
 * Class table.
 */
struct class_type *class_table;

/*
 * Titles.
 */
char *title_table[MAX_CLASS][MAX_LEVEL+1][2];

/*
 * Attribute bonus tables.
 */
const struct str_app_type str_app[MAX_STAT + 1] =
{
    { -5, -4,   0,  0 },  /* 0  */
    { -5, -4,   3,  1 },  /* 1  */
    { -3, -2,   3,  2 },
    { -3, -1,  10,  3 },  /* 3  */
    { -2, -1,  25,  4 },
    { -2, -1,  55,  5 },  /* 5  */
    { -1,  0,  80,  6 },
    { -1,  0,  90,  7 },
    {  0,  0, 100,  8 },
    {  0,  0, 100,  9 },
    {  0,  0, 115, 10 }, /* 10  */
    {  0,  0, 115, 11 },
    {  0,  0, 130, 12 },
    {  0,  0, 130, 13 }, /* 13  */
    {  0,  1, 140, 14 },
    {  1,  1, 150, 15 }, /* 15  */
    {  1,  2, 165, 16 },
    {  2,  3, 180, 22 },
    {  2,  3, 200, 25 }, /* 18  */
    {  3,  4, 225, 30 },
    {  3,  5, 250, 35 }, /* 20  */
    {  4,  6, 300, 40 },
    {  4,  7, 350, 45 },
    {  5,  8, 400, 50 }, /*23 - начинаем экспоненту роллов*/
    {  5,  10, 450, 55 },
    {  6,  13, 500, 60 }, /* 25   */
    {  7, 16, 550, 65 },
    {  8, 19, 600, 70 },
    {  9, 23, 650, 75 },
    { 10, 28, 700, 80 },
    { 11, 34, 750, 85 },  /* 30 */
    { 12, 40, 800, 90 },  /* 31 */
    { 14, 46, 850, 95 }  /* 32 */
};

const struct int_app_type int_app[MAX_STAT + 1] =
{
    {  0 },	/*  0 */
    {  0 },	/*  1 */
    {  0 },
    {  0 },	/*  3 */
    {  0 },
    {  3 },	/*  5 */
    {  5 },
    {  7 },
    {  8 },
    {  9 },
    { 10 },	/* 10 */
    { 11 },
    { 12 },
    { 13 },
    { 15 },
    { 17 },	/* 15 */
    { 19 },
    { 22 },
    { 25 },	/* 18 */
    { 28 },
    { 31 },	/* 20 */
    { 34 },
    { 37 },
    { 40 },
    { 44 },
    { 49 },	/* 25 */
    { 54 },
    { 59 },
    { 65 },
    { 71 },
    { 77 }, 	/* 30 */
    { 83 },     /* 31 */
    { 89 }      /* 32 */
};



const struct wis_app_type wis_app[MAX_STAT + 1] =
{
    { 0 },	/*  0 */
    { 0 },	/*  1 */
    { 0 },
    { 0 },	/*  3 */
    { 0 },
    { 1 },	/*  5 */
    { 1 },
    { 1 },
    { 1 },
    { 1 },
    { 1 },	/* 10 */
    { 1 },
    { 1 },
    { 1 },
    { 1 },
    { 2 },	/* 15 */
    { 2 },
    { 2 },
    { 3 },	/* 18 */
    { 3 },
    { 3 },	/* 20 */
    { 3 },
    { 4 },
    { 4 },
    { 4 },
    { 5 }, 	/* 25 */
    { 5 },
    { 5 },
    { 6 },
    { 6 },
    { 7 },  	/* 30 */
    { 7 },      /* 31 */
    { 8 }       /* 32 */
};



const struct dex_app_type dex_app[MAX_STAT + 1]	=
{
    { -5,  90 },   /* 0 */
    { -5,  80 },   /* 1 */
    { -3,  70 },
    { -3,  60 },
    { -2,  50 },
    { -2,  40 },   /* 5 */
    { -1,  30 },
    { -1,  20 },
    {  0,  10 },
    {  0,   0 },
    {  0,   0 },   /* 10 */
    {  0,   0 },
    {  0,   0 },
    {  0,   0 },
    {  0,   0 },
    {  1,   0 },   /* 15 */
    {  1,   0 },
    {  2,   0 },
    {  2, - 10 },
    {  3, - 20 },
    {  3, - 30 },   /* 20 */
    {  4, - 40 },
    {  5, - 50 },
    {  6, - 65 },  /*23 - начинаем экспоненту роллов*/
    {  8, - 80 },
    {  11, - 95 },   /* 25 */
    {  16, -110 },
    {  21, -125 },
    {  24, -140 },
    {  27, -155 },
    {  30, -170 },    /* 30 */
    {  33, -195 },    /* 31 */
    {  36, -230 }    /* 32 */
};


const struct con_app_type con_app[MAX_STAT + 1]	=
{
    { -6, 10 },   /*  0 */
    { -5, 13 },   /*  1 */
    { -4, 16 },
    { -4, 19 },	  /*  3 */
    { -3, 22 },
    { -3, 25 },   /*  5 */
    { -3, 28 },
    { -2, 31 },
    { -2, 34 },
    { -2, 37 },
    { -1, 40 },   /* 10 */
    { -1, 43 },
    { -1, 46 },
    {  0, 49 },
    {  0, 52 },
    {  0, 55 },   /* 15 */
    {  0, 58 },
    {  0, 61 },
    {  1, 64 },   /* 18 */
    {  2, 67 },
    {  3, 70 },   /* 20 */
    {  4, 73 },
    {  5, 76 },
    {  6, 79 },
    {  7, 82 },
    {  8, 85 },   /* 25 */
    {  9, 88 },
    { 10, 91 },
    { 11, 94 },
    { 12, 97 },
    { 13, 99 },    /* 30 */
    { 14, 99 },    /* 31 */
    { 15, 99 }    /* 32 */
};



/*
 * Liquid properties.
 * Used in world.obj.
 */
const struct liq_type liq_table[] =
{
/*    name			color	proof, full, thirst, food, ssize */
    { "water",		"вода",		"прозрачный",	{  0, 1, 10, 0, 16 } },
    { "beer",		"пиво",		"янтарный",	{  8, 1,  8, 1, 12 } },
    { "red wine",	"красное вино",	"красный",	{ 24, 1,  8, 1,  5 } },
    { "ale",		"эль",		"коричневый",	{ 10, 1,  8, 1, 12 } },
    { "dark ale",	"темный эль",	"темный",	{ 14, 1,  8, 1, 12 } },

    { "whisky",		"виски",	"золотистый",	{100, 1,  5, 0,  2 } },
    { "lemonade",	"лимонад",	"розовый",	{  0, 1,  9, 2, 12 } },
    { "firebreather",	"огненная вода","горячий",	{160, 0,  4, 0,  2 } },
    { "local specialty","местный самогон","прозрачный",	{130, 1,  3, 0,  2 } },
    { "slime mold juice","склизкая жидкость","зеленый",	{  0, 2, -8, 1,  2 } },

    { "milk",		"молоко",	"белый",	{  0, 2,  9, 3, 12 } },
    { "tea",		"чай",		"чайный",	{  0, 1,  8, 0,  6 } },
    { "coffee",		"кофе",		"черный",	{  0, 1,  8, 0,  6 } },
    { "blood",		"кровь",	"красный",	{  0, 2, -1, 2,  6 } },
    { "salt water",	"подсоленая вода","прозрачный",	{  0, 1, -2, 0,  1 } },

    { "coke",		"кока-кола",	"коричневый",	{  0, 2,  9, 2, 12 } },
    { "root beer",	"старое пиво",	"коричневый",	{  0, 2,  9, 2, 12 } },
    { "elvish wine",	"эльфийское вино","зеленый",	{ 30, 2,  8, 1,  5 } },
    { "white wine",	"белое вино",	"золотистый",	{ 24, 1,  8, 1,  5 } },
    { "champagne",	"шампанское",	"золотистый",	{ 28, 1,  8, 1,  5 } },

    { "mead",		"мед",		"медовый",	{ 30, 2,  8, 2, 12 } },
    { "rose wine",	"розовое вино",	"розовый",	{ 22, 1,  8, 1,  5 } },
    { "benedictine wine","вино",	"красный",	{ 36, 1,  8, 1,  5 } },
    { "vodka",		"водка",	"прозрачный",	{110, 1,  5, 0,  2 } },
    { "cranberry juice","клюквенный сок","красный",	{  0, 1,  9, 2, 12 } },

    { "orange juice",	"апельсиновый сок","оранжевый",	{  0, 2,  9, 3, 12 } },
    { "absinthe",	"абсент",	"зеленый",	{170, 1,  4, 0,  2 } },
    { "brandy",		"коньяк",	"золотистый",	{ 70, 1,  5, 0,  4 } },
    { "aquavit",	"самогон",	"прозрачный",	{125, 1,  5, 0,  2 } },
    { "schnapps",	"шнапс",	"прозрачный",	{ 76, 1,  5, 0,  2 } },

    { "icewine",	"вино",		"пурпурный",	{ 36, 2,  6, 1,  5 } },
    { "amontillado",	"амонтиладо",	"красный",	{ 32, 2,  8, 1,  5 } },
    { "sherry",		"шерри",	"красный",	{ 32, 2,  7, 1,  5 } },
    { "framboise",	"фрамбойз",	"красный",	{ 42, 1,  7, 1,  5 } },
    { "rum",		"ром",		"янтарный",	{136, 1,  4, 0,  2 } },

    { "cordial",	"крепкий напиток","прозрачный",	{100, 1,  5, 0,  2 } },
    { "nicotin",	"никотин",	"коричневый",	{130, 1,  5, 0,  2 } },
    { NULL,			NULL,	  NULL,		{  0, 0,  0, 0,  0 } }
};



/*
 * The skill and spell table.
 * Slot numbers must never be changed as they appear in #OBJECTS sections.
 */
#define SLOT(n)	n

struct skill_type *skill_table;

struct group_type *group_table;

/* charset=cp1251 */
