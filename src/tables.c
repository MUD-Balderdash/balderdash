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
*      Russ Taylor (rtaylor@hypercube.org)                                 *
*      Gabrielle Taylor (gtaylor@hypercube.org)                            *
*      Brian Moore (zump@rom.org)                                          *
*  By using this code, you have agreed to follow the terms of the          *
*  ROM license, in the file Rom24/doc/rom.license                          *
***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include "merc.h"


/*****************************************************************************
 Name:    flag_stat_table
 Purpose:  This table catagorizes the tables following the lookup
     functions below into stats and flags.  Flags can be toggled
     but stats can only be assigned.  Update this table when a
     new set of flags is installed.
 ****************************************************************************/
const struct flag_stat_type flag_stat_table[] =
{
/*  { name		structure	 stat	}, */
    { "",		area_flags,	 FALSE	},
    { "",		sex_flags,	 TRUE	},
    { "",		exit_flags,	 FALSE	},
    { "",		door_resets,	 TRUE	},
    { "",		room_flags,	 FALSE  },
    { "",		sector_flags,	 TRUE	},
    { "item_type",	type_flags,	 TRUE	},
    { "extra",		extra_flags,	 FALSE  },
    { "wear",		wear_flags,	 FALSE  },
    { "",		act_flags,	 FALSE  },
    { "affects",	affect_flags,	 FALSE  },
    { "apply",		apply_flags,	 TRUE	},
    { "",		room_apply_flags,TRUE	},
    { "",		wear_loc_flags,	 TRUE	},
    { "",		wear_loc_strings,TRUE	},
    { "container",	container_flags, FALSE	},
    { "",		chest_flags,	 FALSE	},
    { "",		ing_sector_flags, FALSE},
    { "",		ing_weather_flags, FALSE},

    /* ROM specific flags: */
    { "material",	material_table,	 TRUE	},
    { "",		form_flags,	 FALSE	},
    { "",		part_flags,	 FALSE	},
    { "",		ac_type,	 TRUE	},
    { "",		size_flags,	 TRUE	},
    { "",		position_flags,	 TRUE	},
    { "",		off_flags,	 FALSE	},
    { "",		imm_flags,	 FALSE	},
    { "",		res_flags,	 FALSE	},
    { "",		vuln_flags,	 FALSE	},
    { "damages",	dam_flags,	 TRUE	},
    { "weapon_class",	weapon_class,	 TRUE	},
    { "",		target_flags,	 TRUE	},
    { "attributes",	attr_flags,	 TRUE	},
    { "",		saves_flags,	 TRUE	},
    { "",		spell_flags,	 FALSE	},
    { "",		spaff_flags,	 FALSE	},
    { "",		align_flags,	 FALSE	},
    { "aff_where",	where_flags,	 TRUE	},
    { "",		color_flags,	 TRUE	},
    { "confortable",	comf_flags,	 FALSE	},
    { "weapon_flags",	weapon_type2,	 FALSE	},
    { "",		trap_flags,	 FALSE	},
    { "",		color_type_flags,TRUE	},
    { NULL,    		0,      	 0	}
};

struct flag_type weapon_class[MAX_WEAPON_TYPE];
struct flag_type type_flags[MAX_ITEM_TYPE];

/* for position */
const struct position_type position_table[] =
{
    { "dead",			"dead"	},
    { "mortally wounded",	"mort"	},
    { "incapacitated",		"incap"	},
    { "stunned",		"stun"	},
    { "sleeping",		"sleep"	},
    { "resting",		"rest"	},
    { "sitting",		"sit"	},
    { "bashed",			"bash"	},
    { "fighting",		"fight"	},
    { "standing",		"stand"	},
    { NULL,			NULL	}
};

/* for sex */
const struct sex_type sex_table[] =
{
    { "none",	"нет"   	},
    { "male",	"мужской" 	},
    { "female",	"женский"  	},
    { "many",	"множественный" },
    { NULL,	NULL    	}
};

/* for sizes */
const struct size_type size_table[] =
{
    { "tiny"	},
    { "small"	},
    { "medium"	},
    { "large"	},
    { "huge"	},
    { "giant"	},
    { NULL	}
};

const struct flag_type color_type_flags[] =
{
    { "gossip",		COLOUR_GOSSIP,		TRUE,	""  },
    { "gossip_name",	COLOUR_GOSSIP_NAME,	TRUE,	""  },
    { "shout",		COLOUR_SHOUT,		TRUE,	""  },
    { "shout_name",	COLOUR_SHOUT_NAME,	TRUE,	""  },
    { "yell",		COLOUR_YELL,		TRUE,	""  },
    { "yell_name",	COLOUR_YELL_NAME,	TRUE,	""  },
    { "tell",		COLOUR_TELL,		TRUE,	""  },
    { "tell_name",	COLOUR_TELL_NAME,	TRUE,	""  },
    { "answer",		COLOUR_ANSWER,		TRUE,	""  },
    { "answer_name",	COLOUR_ANSWER_NAME,	TRUE,	""  },
    { "say",		COLOUR_SAY,		TRUE,	""  },
    { "say_name",	COLOUR_SAY_NAME,	TRUE,	""  },
    { "emote",		COLOUR_EMOTE,		TRUE,	""  },
    { "ooc",		COLOUR_OOC,		TRUE,	""  },
    { "ooc_name",	COLOUR_OOC_NAME,	TRUE,	""  },
    { "newbies",	COLOUR_NEWBIES,		TRUE,	""  },
    { "newbies_name",	COLOUR_NEWBIES_NAME,	TRUE,	""  },
    { "wiznet",		COLOUR_WIZNET,		TRUE,	""  },
    { "immtalk",	COLOUR_IMMTALK,		TRUE,	""  },
    { "immtalk_name",	COLOUR_IMMTALK_NAME,	TRUE,	""  },
    { "btalk",		COLOUR_BTALK,		TRUE,	""  },
    { "btalk_name",	COLOUR_BTALK_NAME,	TRUE,	""  },
    { "ttalk",		COLOUR_BTALK,		TRUE,	""  },
    { "ttalk_name",	COLOUR_BTALK_NAME,	TRUE,	""  },
    { "clantalk",	COLOUR_CLANTALK,	TRUE,	""  },
    { "clantalk_name",	COLOUR_CLANTALK_NAME,	TRUE,	""  },
    { "gtalk",		COLOUR_GROUPTALK,	TRUE,	""  },
    { "gtalk_name",	COLOUR_GROUPTALK_NAME,	TRUE,	""  },
    { "auction",	COLOUR_AUCTION,		TRUE,	""  },
    { "music",		COLOUR_MUSIC,		TRUE,	""  },
    { "music_name",	COLOUR_MUSIC_NAME,	TRUE,	""  },
    { "grats",		COLOUR_GRATS,		TRUE,	""  },
    { "roomnames",	COLOUR_ROOMNAMES,	TRUE,	""  },
    { "roomdesc",	COLOUR_ROOMDESC,	TRUE,	""  },
    { "mobs",		COLOUR_MOBS,		TRUE,	""  },
    { "chars",		COLOUR_CHARS,		TRUE,	""  },
    { "gchars",		COLOUR_GROUPCHARS,	TRUE,	""  },
    { "objects",	COLOUR_OBJS,		TRUE,	""  },
    { "exits",		COLOUR_EXITS,		TRUE,	""  },
    { "affect_on",	COLOUR_AFFON,		TRUE,	""  },
    { "affect_off",	COLOUR_AFFOFF,		TRUE,	""  },
    { "cast",		COLOUR_CAST,		TRUE,	""  },
    { "cast_name",	COLOUR_CASTNAME,	TRUE,	""  },
    { "affect",		COLOUR_AFFECT,		TRUE,	""  },
    { "your_hit",	COLOUR_YOURHIT,		TRUE,	""  },
    { "enemy_hit",	COLOUR_ENEMYHIT,	TRUE,	""  },
    { "other_hit",	COLOUR_OTHERHIT,	TRUE,	""  },
    { "skill",		COLOUR_SKILL,		TRUE,	""  },
    { "death",		COLOUR_DEATH,		TRUE,	""  },
    { "critical",	COLOUR_CRIT,		TRUE,	""  },
    { NULL,		0,			FALSE,	""  }
};

/* various flag tables */
const struct flag_type act_flags[] =
{
    { "npc",		ACT_IS_NPC,	   FALSE, "моб"		    },
    { "sentinel",	ACT_SENTINEL,	   TRUE,  "стоит_на_месте"  },
    { "scavenger",	ACT_SCAVENGER,	   TRUE,  "дворник"	    },
    { "aggressive",	ACT_AGGRESSIVE,	   TRUE,  "агрессивный"	    },
    { "mount",		ACT_MOUNT,	   TRUE,  "лошадь"	    },
    { "stay_area",	ACT_STAY_AREA,	   TRUE,  "остается_в_зоне" },
    { "wimpy",		ACT_WIMPY,	   TRUE,  "трус"	    },
    { "pet",		ACT_PET,	   TRUE,  "животное"	    },
    { "train",		ACT_TRAIN,	   TRUE,  "тренер"	    },
    { "practice",	ACT_PRACTICE,	   TRUE,  "учитель"	    },
    { "no_remember",	ACT_NOREMEMBER,	   TRUE,  ""		    },
    { "undead",		ACT_UNDEAD,	   TRUE,  "мертвец"	    },
    { "cleric",		ACT_CLERIC,	   TRUE,  "жрец"	    },
    { "mage",		ACT_MAGE,	   TRUE,  "маг"		    },
    { "thief",		ACT_THIEF,	   TRUE,  "вор"		    },
    { "warrior",	ACT_WARRIOR,	   TRUE,  "воин"	    },
    { "noalign",	ACT_NOALIGN,	   TRUE,  "нет_характера"   },
    { "nopurge",	ACT_NOPURGE,	   TRUE,  ""		    },
    { "outdoors",	ACT_OUTDOORS,	   TRUE,  ""		    },
    { "hunter",		ACT_HUNTER,	   TRUE,  "охотник"	    },
    { "indoors",	ACT_INDOORS,	   TRUE,  ""		    },
    { "healer",		ACT_IS_HEALER,	   TRUE,  "лекарь"	    },
    { "gain",		ACT_GAIN,	   TRUE,  "gain"	    },
    { "update_always",	ACT_UPDATE_ALWAYS, TRUE,  "обновлять_всегда"},
    { "changer",	ACT_IS_CHANGER,	   TRUE,  "меняла"	    },
    { "banker",		ACT_BANKER,	   TRUE,  "банкир"	    },
    { "smither",	ACT_SMITHER,	   TRUE,  "ремонтник"	    },
    { "dead_warrior",	ACT_DEAD_WARRIOR,  TRUE,  ""		    },
    { "noexp",		ACT_NOEXP,	   TRUE,  ""		    },
    { "animal",		ACT_ANIMAL,	   TRUE,  "животное"	    },
    { "quester",	ACT_QUESTER,	   TRUE,  ""		    },
    { "arosh",		ACT_AROSH,	   TRUE,  "зодчий"	    },
    { "executioner",	ACT_EXECUTIONER,   TRUE,  "палач"	    },
    { "nolegend",	ACT_NOLEGEND,	   TRUE,  "нет_легенд"	    },
    { "not_visible",	ACT_WIZINVIS,	   TRUE,  "невидим"	    },
    { NULL,		0,		   FALSE, NULL		    }
};

const struct flag_type plr_flags[] =
{
    { "npc",		PLR_IS_NPC,	      FALSE, "моб"		},
    { "excited",	PLR_EXCITED,	  FALSE, "взволнован"	},
    { "autoassist",	PLR_AUTOASSIST,	  FALSE, "автопомощь"	},
    { "autoexit",	PLR_AUTOEXIT,	  FALSE, "автовыходы"	},
    { "autoloot",	PLR_AUTOLOOT,	  FALSE, "автограбеж"	},
    { "autoattack",	PLR_AUTOATTACK,	  FALSE, "разрешение_атаки"},
    { "autosac",	PLR_AUTOSAC,	  FALSE, "автожертва"	},
    { "automoney",	PLR_AUTOMONEY,	  FALSE, "автомонеты"	},
    { "autosplit",	PLR_AUTOSPLIT,	  FALSE, "автодележ"	},
    { "holylight",	PLR_HOLYLIGHT,	  FALSE, ""		},
    { "autotitle",	PLR_AUTOTITLE,	  FALSE, "автотитул"	},
    { "can_loot",	PLR_CANLOOT,	  FALSE, ""		},
    { "nofollow",	PLR_NOFOLLOW,	  FALSE, "неследовать"	},
    { "nopk",		PLR_NOPK,	  FALSE, "не_убить"	},
    { "full_silence",	PLR_FULL_SILENCE, FALSE, "молчание"	},
 /*   { "compensation",   PLR_COMPENSATION, FALSE, "компенсация"  },
 *  { "compold",   PLR_OLD_COMPENSATION, FALSE, "компold"  },
 *  { "compold2",   PLR_OLD_COMPENSATION2, FALSE, "компold2"  },
 *  { "compold3",   PLR_OLD_COMPENSATION3, FALSE, "компold3"  },
 *  { "new_year2013",   PLR_NEW_YEAR_2013, FALSE, "подарок2013"  },
*/
    { "colour",		PLR_COLOUR,	  FALSE, "цвет"		},
    { "permit",		PLR_PERMIT,	  TRUE,  ""		},
    { "log",		PLR_LOG,	  FALSE, ""		},
    { "deny",		PLR_DENY,	  FALSE, ""		},
    { "freeze",		PLR_FREEZE,	  FALSE, "замороженый"	},
    { "thief",		PLR_THIEF,	  FALSE, "вор"		},
    { "killer",		PLR_KILLER,	  FALSE, "убийца"	},
    { "questing",	PLR_QUESTING,	  FALSE, "в_квесте"	},
    { "consent",	PLR_CONSENT,	  FALSE, "брак/развод"	},
    { "nodreams",	PLR_NODREAMS,	  FALSE, "нет_снов"	},
    { "show_damage",	PLR_SHOW_DAMAGE,  FALSE, ""		},
    { "noimmquest",	PLR_NOIMMQUEST,	  FALSE, ""		},
    { "autogold",	PLR_AUTOGOLD,	  FALSE, "автозолото"	},
    { "nocancel",	PLR_NOCANCEL,	  FALSE, ""		},
    { "nocast",		PLR_NOCAST,	  FALSE, ""		},
    { "mapexit",	PLR_MAPEXIT,	  FALSE, ""		},
    { NULL,		0,		  0,	 NULL		}
};

const struct flag_type affect_flags[] =
{
    {"blind",      AFF_BLIND,     TRUE, "слепоту"        },
    {"invisible",    AFF_INVISIBLE,   TRUE, "невидимость"        },
    {"detect_evil",    AFF_DETECT_EVIL,   TRUE, "обнаружить зло"       },
    {"detect_invis",    AFF_DETECT_INVIS,   TRUE, "обнаружить_невидимое" },
    {"detect_magic",    AFF_DETECT_MAGIC,   TRUE, "обнаружить_магию"     },
    {"detect_hidden",    AFF_DETECT_HIDDEN,   TRUE, "обнаружить_скрытое"   },
    {"detect_good",    AFF_DETECT_GOOD,   TRUE, "обнаружить_добро"     },
    {"sanctuary",    AFF_SANCTUARY,   TRUE, "защита_храма"        },
    {"faerie_fire",    AFF_FAERIE_FIRE,   TRUE, "дьявольский_огонь"    },
    {"infrared",    AFF_INFRARED,     TRUE, "инфразрение"        },
    {"curse",      AFF_CURSE,     TRUE, "проклятье"        },
    {"fire_shield",    AFF_FIRE_SHIELD,   TRUE, "огненный_щит"        },
    {"coniferous_shield",    AFF_CONIFEROUS_SHIELD,   TRUE, "хвойный_щит"        },
    {"poison",      AFF_POISON,     TRUE, "яд"          },
    {"protect_evil",    AFF_PROTECT_EVIL,   TRUE, "защита_от_зла"        },
    {"protect_good",    AFF_PROTECT_GOOD,   TRUE, "защита_от_добра"      },
    {"sneak",      AFF_SNEAK,     TRUE, "подкрадывание"        },
    {"hide",      AFF_HIDE,     TRUE, "скрываться"        },
    {"sleep",      AFF_SLEEP,     TRUE, "сон"          },
    {"charm",      AFF_CHARM,     TRUE, "очарование"        },
    {"flying",      AFF_FLYING,     TRUE, "полет"          },
    {"pass_door",    AFF_PASS_DOOR,   TRUE, "пройти_дверь"        },
    {"haste",      AFF_HASTE,     TRUE, "спешка"          },
    {"calm",      AFF_CALM,     TRUE, "спокойствие"        },
    {"plague",      AFF_PLAGUE,     TRUE, "чума"          },
    {"weaken",      AFF_WEAKEN,     TRUE, "слабость"        },
    {"dark_vision",    AFF_DARK_VISION,   TRUE, "ночное_видение"        },
    {"berserk",         AFF_BERSERK,     TRUE, "берсерк"        },
    {"swim",      AFF_SWIM,     TRUE, "плавание"        },
    {"regeneration",    AFF_REGENERATION,   TRUE, "регенерация"        },
    {"slow",      AFF_SLOW,     TRUE, "замедление"        },
    {"detect_undead",    AFF_DETECT_UNDEAD,   TRUE, "обнаружить_мертвых"   },
    {"ice_shield",    AFF_ICE_SHIELD,   TRUE, "ледяной_щит"        },
    {"camouflage",    AFF_CAMOUFLAGE,   TRUE, "камуфляж"        },
    {"camouflage_move",   AFF_CAMOUFLAGE_MOVE,   TRUE, "камуфляж_в_движении"  },
    {"detect_camouflage", AFF_DETECT_CAMOUFLAGE, TRUE, "обнаружить_камуфляж"  },
    {"satiety",         AFF_SATIETY,     TRUE, "сытость"        },
    {"death_aura",    AFF_DEATH_AURA,   TRUE, "аура_смерти"        },
    {"detect_life",    AFF_DETECT_LIFE,   TRUE, "обнаружить_жизнь"     },
    {"holy_shield",    AFF_HOLY_SHIELD,   TRUE, "святой_щит"        },
    {"evil_aura",    AFF_EVIL_AURA,   TRUE, "аура_зла"        },
    {"roots",             AFF_ROOTS,     TRUE, "опутан корнями"        },
    {"cover",             AFF_COVER,     TRUE, "укутан"        },
    {"gods_skill",    AFF_GODS_SKILL,   FALSE,""          },
    {NULL,      0,       0,  NULL          }
};

const struct flag_type off_flags[] =
{
    {  "area_attack",    OFF_AREA_ATTACK,TRUE,  ""    },
    {  "backstab",    OFF_BACKSTAB,  TRUE,  "удар_в_спину"  },
    {  "bash",      OFF_BASH,  TRUE,  "толчок"  },
    {  "berserk",    OFF_BERSERK,  TRUE,  "берсерк"  },
    {  "disarm",    OFF_DISARM,  TRUE,  "обезоруживание"},
    {  "dodge",    OFF_DODGE,  TRUE,  "увертка"  },
    {  "fast",      OFF_FAST,  TRUE,  "fast"    },
    {  "kick",      OFF_KICK,  TRUE,  "пинок"    },
    {  "dirt_kick",    OFF_KICK_DIRT,  TRUE,  "грязь"    },
    {  "parry",    OFF_PARRY,  TRUE,  "парирование"  },
    {  "rescue",    OFF_RESCUE,  TRUE,  "спасение"  },
    {  "tail",      OFF_TAIL,  TRUE,  "хвост"    },
    {  "trip",      OFF_TRIP,  TRUE,  "подножка"  },
    {  "assist_all",    ASSIST_ALL,  TRUE,  ""    },
    {  "assist_align",    ASSIST_ALIGN,  TRUE,  ""    },
    {  "assist_race",    ASSIST_RACE,  TRUE,  ""    },
    {  "assist_players",  ASSIST_PLAYERS,  TRUE,  ""    },
    {  "assist_vnum",    ASSIST_VNUM,  TRUE,  ""    },
    {  "assist_clan",    ASSIST_CLAN,  TRUE,  ""    },
    {  "off_cleave",    OFF_CLEAVE,  TRUE,  "рассечь"  },
    {  "off_ambush",    OFF_AMBUSH,  TRUE,  "засада"  },
    {  NULL,      0,  0,  NULL      }
};

const struct flag_type imm_flags[] =
{
    {  "summon",    IMM_SUMMON,  TRUE,  ""  },
    {  "charm",    IMM_CHARM,  TRUE,  ""  },
    {  "magic",    IMM_MAGIC,  TRUE,  ""  },
    {  "weapon",    IMM_WEAPON,  TRUE,  ""  },
    {  "bash",      IMM_BASH,  TRUE,  ""  },
    {  "pierce",    IMM_PIERCE,  TRUE,  ""  },
    {  "slash",    IMM_SLASH,  TRUE,  ""  },
    {  "fire",      IMM_FIRE,  TRUE,  ""  },
    {  "cold",      IMM_COLD,  TRUE,  ""  },
    {  "acid",      IMM_ACID,  TRUE,  ""  },
    {  "poison",    IMM_POISON,  TRUE,  ""  },
    {  "negative",    IMM_NEGATIVE,  TRUE,  ""  },
    {  "holy",      IMM_HOLY,  TRUE,  ""  },
    {  "energy",    IMM_ENERGY,  TRUE,  ""  },
    {  "mental",    IMM_MENTAL,  TRUE,  ""  },
    {  "disease",    IMM_DISEASE,  TRUE,  ""  },
    {  "drowning",    IMM_DROWNING,  TRUE,  ""  },
    {  "light",    IMM_LIGHT,  TRUE,  ""  },
    {  "lightning",    IMM_LIGHTNING,  TRUE,  ""  },
    {  "sound",    IMM_SOUND,  TRUE,  ""  },
    {  "wood",      IMM_WOOD,  TRUE,  ""  },
    {  "silver",    IMM_SILVER,  TRUE,  ""  },
    {  "iron",      IMM_IRON,  TRUE,  ""  },
    {  NULL,      0,    0,  NULL  }
};

const struct flag_type dam_flags[] =
{
    {  "none",    DAM_NONE,  FALSE,  ""      },
    {  "bash",    DAM_BASH,  TRUE,  "удар"      },
    {  "pierce",  DAM_PIERCE,  TRUE,  "укол"      },
    {  "slash",  DAM_SLASH,  TRUE,  "разрез"    },
    {  "fire",    DAM_FIRE,  TRUE,  "огонь"      },
    {  "cold",    DAM_COLD,  TRUE,  "холод"      },
    {  "light",  DAM_LIGHT,  TRUE,  "свет"      },
    {  "lightning",  DAM_LIGHTNING,  TRUE,  "молния"    },
    {  "acid",    DAM_ACID,  TRUE,  "кислота"    },
    {  "poison",  DAM_POISON,  TRUE,  "яд"      },
    {  "negative",  DAM_NEGATIVE,  TRUE,  "негативная магия"  },
    {  "holy",    DAM_HOLY,  TRUE,  "святая магия"    },
    {  "energy",  DAM_ENERGY,  TRUE,  "энергия"    },
    {  "mental",  DAM_MENTAL,  TRUE,  "магия разума"    },
    {  "disease",  DAM_DISEASE,  TRUE,  "болезнь"    },
    {  "drowning",  DAM_DROWNING,  TRUE,  "утопление"    },
    {  "other",  DAM_OTHER,  TRUE,  "другие типы"    },
    {  "harm",    DAM_HARM,  TRUE,  "повреждение"    },
    {  "charm",  DAM_CHARM,  TRUE,  "очарование"    },
    {  "sound",  DAM_SOUND,  TRUE,  "звук"      },
    {  "wood",    DAM_WOOD,  TRUE,  "дерево"    },
    {  "silver",  DAM_SILVER,  TRUE,  "серебро"    },
    {  "iron",    DAM_IRON,  TRUE,  "железо"    },
    {  "magic",  DAM_MAGIC,  TRUE,  "магия"      },
    {  "summon",  DAM_SUMMON,  TRUE,  "транспортная магия" },
    {  "weapon",  DAM_WEAPON,  TRUE,  "оружие"    },
    {  NULL,    0,    0,  NULL      }
};

const struct flag_type form_flags[] =
{
    { "edible",		FORM_EDIBLE,		TRUE,  ""	},
    { "poison",		FORM_POISON,		TRUE,  ""	},
    { "magical",	FORM_MAGICAL,		TRUE,  ""	},
    { "instant_decay",	FORM_INSTANT_DECAY,	TRUE,  ""	},
    { "other",		FORM_OTHER,		TRUE,  ""	},
    { "animal",		FORM_ANIMAL,		TRUE,  ""	},
    { "sentient",	FORM_SENTIENT,		TRUE,  ""	},
    { "undead",		FORM_UNDEAD,		TRUE,  ""	},
    { "construct",	FORM_CONSTRUCT,		TRUE,  ""	},
    { "mist",		FORM_MIST,		TRUE,  ""	},
    { "intangible",	FORM_INTANGIBLE,	TRUE,  ""	},
    { "biped",		FORM_BIPED,		TRUE,  ""	},
    { "centaur",	FORM_CENTAUR,		TRUE,  ""	},
    { "insect",		FORM_INSECT,		TRUE,  ""	},
    { "spider",		FORM_SPIDER,		TRUE,  ""	},
    { "crustacean",	FORM_CRUSTACEAN,	TRUE,  ""	},
    { "worm",		FORM_WORM,		TRUE,  ""	},
    { "blob",		FORM_BLOB,		TRUE,  ""	},
    { "mammal",		FORM_MAMMAL,		TRUE,  ""	},
    { "bird",		FORM_BIRD,		TRUE,  ""	},
    { "reptile",	FORM_REPTILE,		TRUE,  ""	},
    { "snake",		FORM_SNAKE,		TRUE,  ""	},
    { "dragon",		FORM_DRAGON,		TRUE,  ""	},
    { "amphibian",	FORM_AMPHIBIAN,		TRUE,  ""	},
    { "fish",		FORM_FISH ,		TRUE,  ""	},
    { "cold_blood",	FORM_COLD_BLOOD,	TRUE,  ""	},
    { "tree",		FORM_TREE,		TRUE,  ""	},
    { "bloodless",	FORM_BLOODLESS,		TRUE,  ""	},
    { "has_blood",	FORM_HAS_BLOOD,		TRUE,  ""	},
    { NULL,		0,			FALSE, ""	}
};

const struct flag_type part_flags[] =
{
    {  "head",       PART_HEAD,    TRUE,  "голова"     },
    {  "arms",       PART_ARMS,    TRUE,  "руки"       },
    {  "legs",       PART_LEGS,    TRUE,  "ноги"       },
    {  "heart",     PART_HEART,    TRUE,  "сердце"     },
    {  "skin",		PART_SKIN,    TRUE,  "кожа"     },
    {  "brains",     PART_BRAINS,    TRUE,  "мозги"      },
    {  "guts",       PART_GUTS,    TRUE,  "кишки"      },
    {  "hands",     PART_HANDS,    TRUE,  "кисти"      },
    {  "feet",       PART_FEET,    TRUE,  "ступни"     },
    {  "fingers",     PART_FINGERS,  TRUE,  "пальцы"     },
    {  "ear",       PART_EAR,    TRUE,  "ухо"       },
    {  "eye",       PART_EYE,    TRUE,  "глаз"       },
    {  "long_tongue",     PART_LONG_TONGUE,  TRUE,  "длинный_язык"  },
    {  "eyestalks",     PART_EYESTALKS,  TRUE,  "ресницы"    },
    {  "tentacles",     PART_TENTACLES,  TRUE,  "щупальца"   },
    {  "fins",       PART_FINS,    TRUE,  "плавники"   },
    {  "wings",     PART_WINGS,    TRUE,  "крылья"     },
    {  "tail",       PART_TAIL,    TRUE,  "хвост"      },
    {  "claws",     PART_CLAWS,    TRUE,  "когти"       },
    {  "fangs",     PART_FANGS,    TRUE,  "клыки"      },
    {  "horns",     PART_HORNS,    TRUE,  "рога"       },
    {  "scales",     PART_SCALES,    TRUE,  "чешуя"       },
    {  "tusks",     PART_TUSKS,    TRUE,  "клыки"      },
    {  NULL,       0,      0,  NULL       }
};

const struct flag_type comm_flags[] =
{
    {  "quiet",    COMM_QUIET,    TRUE,  ""  },
    {   "deaf",      COMM_DEAF,    TRUE,  ""      },
    {   "nowiz",    COMM_NOWIZ,    TRUE,  ""      },
    {   "noclangossip",    COMM_NOAUCTION,    TRUE,  ""      },
    {   "nogossip",    COMM_NOGOSSIP,    TRUE,  ""      },
    {   "noquestion",    COMM_NOQUESTION,  TRUE,  ""      },
    {   "nomusic",    COMM_NOMUSIC,    TRUE,  ""      },
    {   "noclan",    COMM_NOCLAN,    TRUE,  ""  },
    {   "noquote",    COMM_NOQUOTE,    TRUE,  ""  },
    {   "shoutsoff",    COMM_SHOUTSOFF,    TRUE,  ""  },
    {   "compact",    COMM_COMPACT,    TRUE,  ""  },
    {   "brief",    COMM_BRIEF,    TRUE,  ""  },
    {   "prompt",    COMM_PROMPT,    TRUE,  ""  },
    {   "combine",    COMM_COMBINE,    TRUE,  ""  },
    {   "telnet_ga",    COMM_TELNET_GA,    TRUE,  ""  },
    {   "show_affects",    COMM_SHOW_AFFECTS,  TRUE,  ""  },
    {   "nograts",    COMM_NOGRATS,    TRUE,  ""      },
    {   "noemote",    COMM_NOEMOTE,    FALSE,  ""  },
    {   "noshout",    COMM_NOSHOUT,    FALSE,  ""      },
    {   "notell",    COMM_NOTELL,    FALSE,  ""  },
    {   "nochannels",    COMM_NOCHANNELS,  FALSE,  ""  },
    {  "nonotes",    COMM_NONOTES,    FALSE,  ""  },
    {  "notitle",    COMM_NOTITLE,    FALSE,  ""  },
    {   "snoop_proof",    COMM_SNOOP_PROOF,  FALSE,  ""      },
    {   "afk",      COMM_AFK,    TRUE,  ""  },
    {   "noooc",    COMM_NOOOC,    TRUE,  ""  },
    {  "helper",    COMM_HELPER,    FALSE,  ""  },
    {  NULL,      0,      0,  ""  }
};


const struct flag_type mprog_flags[] =
{
    {  "act",      TRIG_ACT,    TRUE,  ""  },
    {  "bribe",    TRIG_BRIBE,    TRUE,  ""   },
    {  "death",    TRIG_DEATH,    TRUE,  ""      },
    {  "entry",    TRIG_ENTRY,    TRUE,  ""  },
    {  "fight",    TRIG_FIGHT,    TRUE,  ""  },
    {  "give",      TRIG_GIVE,    TRUE,  ""  },
    {  "greet",    TRIG_GREET,    TRUE,  ""      },
    {  "grall",    TRIG_GRALL,    TRUE,  ""  },
    {  "kill",      TRIG_KILL,    TRUE,  ""  },
    {  "hpcnt",    TRIG_HPCNT,    TRUE,  ""      },
    {  "random",    TRIG_RANDOM,    TRUE,  ""  },
    {  "speech",    TRIG_SPEECH,    TRUE,  ""  },
    {  "exit",      TRIG_EXIT,    TRUE,  ""      },
    {  "exall",    TRIG_EXALL,    TRUE,  ""      },
    {  "delay",    TRIG_DELAY,    TRUE,  ""      },
    {  "surr",      TRIG_SURR,    TRUE,  ""      },
    {  "wear",      TRIG_WEAR,    TRUE,  ""      },
    {  NULL,      0,      TRUE,  ""  }
};

const struct flag_type area_flags[] =
{
    {  "none",      AREA_NONE,    FALSE,  ""  },
    {  "changed",    AREA_CHANGED,    FALSE,  ""  },
    {  "added",    AREA_ADDED,    FALSE,  ""  },
    {  "loading",    AREA_LOADING,    FALSE,  ""  },
    {  "not_available",  AREA_NA,    TRUE,  ""  },
    {  "testing",  AREA_TESTING,    TRUE,  ""  },
    {  "noquest",    AREA_NOQUEST,    TRUE,  ""  },
    {  "nopopularity",  AREA_NOPOPULARITY,  TRUE,  ""  },
    {  "nolegend",  AREA_NOLEGEND,  TRUE,  ""  },
    {  "fastrepop",  AREA_FASTREPOP,  TRUE,  ""  },
    {  "external_edit",  AREA_EXTERN_EDIT,  TRUE,  ""  },
    {  NULL,      0,      0,  ""  }
};



const struct flag_type sex_flags[] =
{
    {  "male",      SEX_MALE,    TRUE,  ""  },
    {  "female",    SEX_FEMALE,    TRUE,  ""  },
    {  "neutral",    SEX_NEUTRAL,    TRUE,  ""  },
    {   "many",                 SEX_MANY,               TRUE,  ""      },
    {  "none",      SEX_NEUTRAL,    TRUE,  ""  },
    {  NULL,      0,      0,  ""  }
};



const struct flag_type exit_flags[] =
{
    {   "door",      EX_ISDOOR,    TRUE,  ""      },
    {  "closed",    EX_CLOSED,    TRUE,  ""  },
    {  "locked",    EX_LOCKED,    TRUE,  ""  },
    {  "pickproof",    EX_PICKPROOF,    TRUE,  ""  },
    {   "nopass",    EX_NOPASS,    TRUE,  ""  },
    {   "easy",      EX_EASY,    TRUE,  ""  },
    {   "hard",      EX_HARD,    TRUE,  ""  },
    {  "infuriating",    EX_INFURIATING,    TRUE,  ""  },
    {  "noclose",    EX_NOCLOSE,    TRUE,  ""  },
    {  "nolock",    EX_NOLOCK,    TRUE,  ""  },
    {   "race",      EX_RACE,    TRUE,  ""  },
    {  NULL,      0,      0,  ""  }
};



const struct flag_type door_resets[] =
{
    {  "open and unlocked",  0,    TRUE,  ""  },
    {  "closed and unlocked",  1,    TRUE,  ""  },
    {  "closed and locked",  2,    TRUE,  ""  },
    {  NULL,      0,    0,  ""  }
};


const struct flag_type room_flags[] =
{
    {  "dark",      ROOM_DARK,    TRUE,  "темно"  },
    {  "no_mob",    ROOM_NO_MOB,    TRUE,  "нет_мобов"  },
    {  "indoors",    ROOM_INDOORS,    TRUE,  "внутри"  },
    {  "private",    ROOM_PRIVATE,    TRUE,  "приватная"      },
    {  "safe",      ROOM_SAFE,    TRUE,  "мирная"  },
    {  "solitary",    ROOM_SOLITARY,    TRUE,  ""  },
    {  "pet_shop",    ROOM_PET_SHOP,    TRUE,  "магазин_любимцев"  },
    {  "no_recall",    ROOM_NO_RECALL,    TRUE,  "нет_возврата"  },
    {  "no_fly",    ROOM_NO_FLY,    TRUE,  "нет_полета"  },
/*    {  "imp_only",    ROOM_IMP_ONLY,    TRUE,  ""      },
    {  "gods_only",          ROOM_GODS_ONLY,    TRUE,  ""      },
    {  "heroes_only",    ROOM_HEROES_ONLY,  TRUE,  "только_герои"  },*/
    {  "arena",    ROOM_ARENA,  TRUE,  "арена"  }, 
    {  "law",      ROOM_LAW,    TRUE,  "охраняется_законом"  },
    {  "holy",      ROOM_HOLY,    TRUE,  "святое место"  },
    {  "prison",      ROOM_PRISON,    TRUE,  "тюрьма"  },
    {  "guild",      ROOM_GUILD,    TRUE,  "гильдия"  },
    {   "nowhere",    ROOM_NOWHERE,    TRUE,  "нигде"  },
    {  "nomagic",     ROOM_NOMAGIC,     TRUE,  "нет_магии"  },
    {  "death_trap",     ROOM_KILL,     TRUE,  "смертельная_ловушка"  },
    {  "mount_shop",     ROOM_MOUNT_SHOP,   TRUE,  "магазин_лошадей"  },
    {  "bank",     ROOM_BANK,     TRUE,  "банк"  },
    {  "bounty_office",   ROOM_BOUNTY_OFFICE,   TRUE,  "заказ_убийств"  },
    {  "slow_deathtrap",   ROOM_SLOW_DT,     TRUE,  "медленная_ловушка"  },
    {  "silence",     	ROOM_SILENCE,     TRUE,  "тишина"  },
    {  "nogate",     	ROOM_NOGATE,     TRUE,  "нет_врат"  },
    {  "home",     	ROOM_HOUSE,     TRUE,  "дом_игрока"  },
    {  "nofly_dt",    	ROOM_NOFLY_DT,    TRUE,  "нужно_лететь"  },
    {  "noexp_lost",    ROOM_NOEXP_LOST,  TRUE,  "опыт_не_теряется"  },
    {  "can_build_house",    ROOM_CAN_BUILD_HOUSE,    TRUE,  "можно_построить_дом"  },
    {  "nolegend",    	ROOM_NOLEGEND,  TRUE,  "нет_легенд"  },
    {  "norun",    	ROOM_NORUN,  TRUE,  "нельзя_побежать"  },
    {  "noauction",    	ROOM_NOAUCTION,  TRUE,  "нет_аукциона"  },
    {  "holy",    	ROOM_HOLY,  TRUE,  "святое"  },
    {  "guild",    	ROOM_GUILD,  TRUE,  "гильдия"  },
    {  "prison",    	ROOM_PRISON,  TRUE,  "тюрьма"  },
    {  "noquest",    	ROOM_NOQUEST,  TRUE,  "нет_квестов"  },
    {  NULL,      0,      0,  ""  }
};



const struct flag_type sector_flags[] =
{
    { "inside",     SECT_INSIDE,    TRUE,  "здание"    },
    { "city",     SECT_CITY,    TRUE,  "город"      },
    { "field",     SECT_FIELD,    TRUE,  "поле"      },
    { "forest",     SECT_FOREST,    TRUE,  "лес"      },
    { "hills",     SECT_HILLS,    TRUE,  "холмы"      },
    { "mountain",  SECT_MOUNTAIN,  TRUE,  "горы"      },
    { "swim",     SECT_WATER_SWIM,  TRUE,  "вода"      },
    { "noswim",     SECT_WATER_NOSWIM,  TRUE,  "вода (нельзя плавать)"  },
    { "unused",     SECT_UNUSED,    TRUE,  "неизвестность"    },
    { "air",     SECT_AIR,    TRUE,  "воздух"    },
    { "desert",     SECT_DESERT,    TRUE,  "пустыня"    },
    { NULL,     0,      0,  ""      }
};

const struct flag_type extra_flags[] =
{
    { "glow",		ITEM_GLOW,	     TRUE,	"светится"           },
    { "hum",		ITEM_HUM,	     TRUE,	"издает_звук"        },
    { "dark",		ITEM_DARK,	     TRUE,	"темное"             },
    { "lock",		ITEM_LOCK,	     TRUE,	"заперто"            },
    { "evil",		ITEM_EVIL,	     TRUE,	"злое"               },
    { "invis",		ITEM_INVIS,	     TRUE,	"невидимое"          },
    { "magic",		ITEM_MAGIC,	     TRUE,	"магическое"         },
    { "nodrop",		ITEM_NODROP,	     TRUE,	"не_бросить"         },
    { "bless",		ITEM_BLESS,	     TRUE,	"благословлено"      },
    { "noremove",	ITEM_NOREMOVE,	     TRUE,	"не_снять"           },
    { "inventory",	ITEM_INVENTORY,	     TRUE,	"инвентарь"          },
    { "nopurge",	ITEM_NOPURGE,	     TRUE,	""		     },
    { "rotdeath",	ITEM_ROT_DEATH,	     TRUE,	""		     },
    { "visdeath",	ITEM_VIS_DEATH,	     TRUE,	""		     },
    { "nonmetal",	ITEM_NONMETAL,	     TRUE,	"неметаллическое"    },
    { "meltdrop",	ITEM_MELT_DROP,	     TRUE,	""    		     },
    { "hadtimer",	ITEM_HAD_TIMER,	     TRUE,	""		     },
    { "sellextract",	ITEM_SELL_EXTRACT,   TRUE,	""		     },
    { "burnproof",	ITEM_BURN_PROOF,     TRUE,	"защищено_от_огня"   },
    { "nouncurse",	ITEM_NOUNCURSE,	     TRUE,	"не_снять_проклятье" },
    { "nolocate",	ITEM_NOLOCATE,	     TRUE,	""		     },
    { "aggravate",	ITEM_AGGRAVATE,	     TRUE,	""		     },
    { "teleport",	ITEM_TELEPORT,	     TRUE,	""		     },
    { "slow_digestion",	ITEM_SLOW_DIGESTION, TRUE,	""		     },
    { "no_identify",	ITEM_NO_IDENTIFY,    TRUE,	""		     },
    { "has_socket",	ITEM_HAS_SOCKET,     TRUE,	"есть_гнездо"	     },
    { "not_digging",	ITEM_NOT_DIGGING,    TRUE,	"нельзя_выкопать"    },
    { "auctioned",	ITEM_AUCTION,	     FALSE,	""		     },
    { "quit_drop",	ITEM_QUITDROP,	     TRUE,	""		     },
    { "nolegend",	ITEM_NO_LEGEND,	     TRUE,	"нет_легенд"	     },
    { "nosac",		ITEM_NO_SAC,	     TRUE,	""		     },
    { "for_quest",	ITEM_FORQUEST,	     TRUE,	" "		     },
    { "no_personalize",	ITEM_NO_PERSONALIZE, TRUE,	"не_персонализируемое"},
    { "not_putable",	ITEM_NOT_PUTABLE,    TRUE,	"нельзя_в_контейнер" },
    { NULL,		0,		     0,		""		     }
};

const struct flag_type wear_flags[] =
{
    {  "take",      ITEM_TAKE,    TRUE,  ""  },
    {  "finger",    ITEM_WEAR_FINGER,  TRUE,  ""  },
    {  "neck",      ITEM_WEAR_NECK,    TRUE,  ""  },
    {  "body",      ITEM_WEAR_BODY,    TRUE,  ""  },
    {  "head",      ITEM_WEAR_HEAD,    TRUE,  ""  },
    {  "legs",      ITEM_WEAR_LEGS,    TRUE,  ""  },
    {  "feet",      ITEM_WEAR_FEET,    TRUE,  ""  },
    {  "hands",    ITEM_WEAR_HANDS,  TRUE,  ""  },
    {  "arms",      ITEM_WEAR_ARMS,    TRUE,  ""  },
    {  "shield",    ITEM_WEAR_SHIELD,  TRUE,  ""  },
    {  "about",    ITEM_WEAR_ABOUT,  TRUE,  ""  },
    {  "waist",    ITEM_WEAR_WAIST,  TRUE,  ""  },
    {  "wrist",    ITEM_WEAR_WRIST,  TRUE,  ""  },
    {  "wield",    ITEM_WIELD,    TRUE,  ""  },
    {  "hold",      ITEM_HOLD,    TRUE,  ""  },
    {  "wearfloat",    ITEM_WEAR_FLOAT,  TRUE,  ""  },
    {  "shoulder",    ITEM_WEAR_AT_SHOULDER,  TRUE,  ""  },
    {  NULL,      0,      0,  ""  }
};

/*
 * Used when adding an affect to tell where it goes.
 * See addaffect and delaffect in act_olc.c
 */
const struct flag_type apply_flags[] =
{
    { "none",        APPLY_NONE,      TRUE,    "ничего"                },
    { "strength",     APPLY_STR,      TRUE,    "силу"                  },
    { "dexterity",    APPLY_DEX,      TRUE,    "ловкость"              },
    { "intelligence", APPLY_INT,      TRUE,    "ум"                    },
    { "wisdom",        APPLY_WIS,      TRUE,    "мудрость"              },
    { "constitution", APPLY_CON,      TRUE,    "сложение"              },
    { "sex",        APPLY_SEX,      TRUE,    "пол"                   },
    { "mana",        APPLY_MANA,      TRUE,    "ману"                  },
    { "hp",        APPLY_HIT,      TRUE,    "жизнь"                 },
    { "move",        APPLY_MOVE,      TRUE,    "шаги"                  },
    { "ac",        APPLY_AC,        TRUE,    "броню"                 },
    { "hitroll",      APPLY_HITROLL,      TRUE,    "попадание"             },
    { "damroll",      APPLY_DAMROLL,      TRUE,    "урон"                  },
    { "saves",        APPLY_SAVES,      TRUE,    "силу_воли"                },
    { "savingrod",    APPLY_SAVING_ROD,      TRUE,   "силу_воли"           },
    { "savingpetri",  APPLY_SAVING_PETRI,   TRUE,    "силу_воли" 	},
    { "savingbreath", APPLY_SAVING_BREATH,  TRUE,    "силу_воли"     },
    { "savingspell",  APPLY_SAVING_SPELL,   TRUE,    "силу_воли"       },
    { "spellaffect",  APPLY_SPELL_AFFECT,   FALSE,   "ничего"                },
    { "skill",        APPLY_SKILL,      TRUE,    "умение/заклинание"     },
    { NULL,        0,        0,       ""                      }
};

const struct flag_type room_apply_flags[] =
{
    {	"none",		APPLY_NONE,		TRUE,	"ничего"              },
    {	"sector",	ROOM_APPLY_SECTOR,	TRUE,	"сектор"              },
    {	NULL,		0,			0,	""		      }
};

/*
 * What is seen.
 */
const struct flag_type wear_loc_strings[] =
{
    {  "in the inventory",  WEAR_NONE,  TRUE,  ""  },
    {  "as a light",    WEAR_LIGHT,  TRUE,  ""  },
    {  "on the left finger",  WEAR_FINGER_L,  TRUE,  ""  },
    {  "on the right finger",  WEAR_FINGER_R,  TRUE,  ""  },
    {  "around the neck (1)",  WEAR_NECK_1,  TRUE,  ""  },
    {  "around the neck (2)",  WEAR_NECK_2,  TRUE,  ""  },
    {  "on the body",    WEAR_BODY,  TRUE,  ""  },
    {  "over the head",  WEAR_HEAD,  TRUE,  ""  },
    {  "on the legs",    WEAR_LEGS,  TRUE,  ""  },
    {  "on the feet",    WEAR_FEET,  TRUE,  ""  },
    {  "on the hands",    WEAR_HANDS,  TRUE,  ""  },
    {  "on the arms",    WEAR_ARMS,  TRUE,  ""  },
    {  "as a shield",    WEAR_SHIELD,  TRUE,  ""  },
    {  "about the shoulders",  WEAR_ABOUT,  TRUE,  ""  },
    {  "around the waist",  WEAR_WAIST,  TRUE,  ""  },
    {  "on the left wrist",  WEAR_WRIST_L,  TRUE,  ""  },
    {  "on the right wrist",  WEAR_WRIST_R,  TRUE,  ""  },
    {  "wielded",    WEAR_WIELD,  TRUE,  ""  },
    {  "held in the hands",  WEAR_HOLD,  TRUE,  ""  },
    {  "floating nearby",  WEAR_FLOAT,  TRUE,  ""  },
    {  "secondary weapon",  WEAR_SECONDARY,  TRUE,  ""  },
    {  "at shoulder",  WEAR_AT_SHOULDER,  TRUE,  ""  },
    {  NULL,      0        , 0,  ""  }
};

const struct flag_type wear_loc_flags[] =
{
    {  "none",    WEAR_NONE,  TRUE,  ""  },
    {  "light",  WEAR_LIGHT,  TRUE,  ""  },
    {  "lfinger",  WEAR_FINGER_L,  TRUE,  ""  },
    {  "rfinger",  WEAR_FINGER_R,  TRUE,  ""  },
    {  "neck1",  WEAR_NECK_1,  TRUE,  ""  },
    {  "neck2",  WEAR_NECK_2,  TRUE,  ""  },
    {  "body",    WEAR_BODY,  TRUE,  ""  },
    {  "head",    WEAR_HEAD,  TRUE,  ""  },
    {  "legs",    WEAR_LEGS,  TRUE,  ""  },
    {  "feet",    WEAR_FEET,  TRUE,  ""  },
    {  "hands",  WEAR_HANDS,  TRUE,  ""  },
    {  "arms",    WEAR_ARMS,  TRUE,  ""  },
    {  "shield",  WEAR_SHIELD,  TRUE,  ""  },
    {  "about",  WEAR_ABOUT,  TRUE,  ""  },
    {  "waist",  WEAR_WAIST,  TRUE,  ""  },
    {  "lwrist",  WEAR_WRIST_L,  TRUE,  ""  },
    {  "rwrist",  WEAR_WRIST_R,  TRUE,  ""  },
    {  "wielded",  WEAR_WIELD,  TRUE,  ""  },
    {  "hold",    WEAR_HOLD,  TRUE,  ""  },
    {  "floating",  WEAR_FLOAT,  TRUE,  ""  },
    {  "secondary",  WEAR_SECONDARY,  TRUE,  ""  },
    {  "shoulder",  WEAR_AT_SHOULDER,  TRUE,  ""  },
    {  NULL,    0,    0,  ""  }
};

#define CONT_CLOSEABLE          1
#define CONT_PICKPROOF          2
#define CONT_CLOSED          4
#define CONT_LOCKED          8
#define CONT_PUT_ON         16

const struct flag_type container_flags[] =
{
    {  "closeable",  CONT_CLOSEABLE,    TRUE,  "закрываемое"  },
    {  "pickproof",  CONT_PICKPROOF,    TRUE,  "не_взломать"  },
    {  "closed",  CONT_CLOSED,    TRUE,  "закрыто"      },
    {  "locked",  CONT_LOCKED,    TRUE,  "заперто"      },
    {  "puton",  CONT_PUT_ON,    TRUE,  ""    },
    {  NULL,    0,      0,  ""    }
};

const struct flag_type chest_flags[] =
{
    {  "closeable",  CONT_CLOSEABLE,    TRUE,  "закрываемое"  },
    {  "pickproof",  CONT_PICKPROOF,    TRUE,  "не_взломать"  },
    {  "closed",  CONT_CLOSED,    TRUE,  "закрыто"      },
    {  "locked",  CONT_LOCKED,    TRUE,  "заперто"      },
    {  "puton",  CONT_PUT_ON,    TRUE,  ""    },
    {  NULL,    0,      0,  ""    }
};

/* Ингредиенты */

const struct flag_type ing_sector_flags[] =
{
    { "inside",     ING_SECT_INSIDE,    TRUE,  "здание"    },
    { "city",     ING_SECT_CITY,    TRUE,  "город"      },
    { "field",     ING_SECT_FIELD,    TRUE,  "поле"      },
    { "forest",     ING_SECT_FOREST,    TRUE,  "лес"      },
    { "hills",     ING_SECT_HILLS,    TRUE,  "холмы"      },
    { "mountain",  ING_SECT_MOUNTAIN,  TRUE,  "горы"      },
    { "swim",     ING_SECT_WATER_SWIM,  TRUE,  "вода"      },
    { "noswim",     ING_SECT_WATER_NOSWIM,  TRUE,  "вода (нельзя плавать)"  },
    { "unused",     ING_SECT_UNUSED,    TRUE,  "неизвестность"    },
    { "air",     ING_SECT_AIR,    TRUE,  "воздух"    },
    { "desert",     ING_SECT_DESERT,    TRUE,  "пустыня"    },
    { "in_water",     ING_SECT_IN_WATER,    TRUE,  "в воде"    },
    { NULL,     0,      0,  ""      }
};

const struct flag_type ing_weather_flags[] =
{
    {  "cloudless",  ING_SKY_CLOUDLESS,    TRUE,  "безоблачно"  },
    {  "cloudy",     ING_SKY_CLOUDY,       TRUE,  "облачно"  	},
    {  "raining",    ING_SKY_RAINING,      TRUE,  "дождь"	},
    {  "lightning",  ING_SKY_LIGHTNING,    TRUE,  "гроза"	},
    {  NULL,    0,      0,  ""    }
};

/*****************************************************************************
                      ROM - specific tables:
 ****************************************************************************/




const struct flag_type ac_type[] =
{
    {   "pierce",        AC_PIERCE,            TRUE,  ""    },
    {   "bash",          AC_BASH,              TRUE,  ""    },
    {   "slash",         AC_SLASH,             TRUE,  ""    },
    {   "exotic",        AC_EXOTIC,            TRUE,  ""    },
    {   NULL,              0,                    0 ,  ""    }
};


const struct flag_type size_flags[] =
{
    {   "tiny",          SIZE_TINY,            TRUE,  ""    },
    {   "small",         SIZE_SMALL,           TRUE,  ""    },
    {   "medium",        SIZE_MEDIUM,          TRUE,  ""    },
    {   "large",         SIZE_LARGE,           TRUE,  ""    },
    {   "huge",          SIZE_HUGE,            TRUE,  ""    },
    {   "giant",         SIZE_GIANT,           TRUE,  ""    },
    {   NULL,              0,                    0,  ""    },
};

const struct flag_type weapon_type2[] =
{
    { "flaming",      WEAPON_FLAMING,      TRUE,    "огонь"     },
    { "frost",        WEAPON_FROST,        TRUE,    "лед"     },
    { "vampiric",     WEAPON_VAMPIRIC,     TRUE,    "вытягивает_жизнь"  },
    { "sharp",        WEAPON_SHARP,        TRUE,    "заточено"    },
    { "twohands",     WEAPON_TWO_HANDS,    TRUE,    "двуручное"    },
    { "shocking",     WEAPON_SHOCKING,     TRUE,    "электрическое"  },
    { "poison",        WEAPON_POISON,     TRUE,    "отравлено"    },
    { "manapiric",    WEAPON_MANAPIRIC,    TRUE,    "вытягивает_ману"  },
    { "slay_evil",    WEAPON_SLAY_EVIL,    TRUE,    "против_злых"  },
    { "slay_good",    WEAPON_SLAY_GOOD,    TRUE,    "против_добрых"  },
    { "slay_animal",  WEAPON_SLAY_ANIMAL,  TRUE,    "против_животных"  },
    { "slay_undead",  WEAPON_SLAY_UNDEAD,  TRUE,    "против_мертвых"  },
    { "primary_only", WEAPON_PRIMARY_ONLY, TRUE,    "только_основное"  },
    { "second_only",  WEAPON_SECONDARY_ONLY, TRUE,"только_второе"    },
    { NULL,        0,                   0,       ""      }
};


const struct flag_type res_flags[] =
{
    {  "summon",   RES_SUMMON,  TRUE,  ""  },
    {   "charm",         RES_CHARM,     TRUE,  ""      },
    {   "magic",         RES_MAGIC,     TRUE,  ""      },
    {   "weapon",        RES_WEAPON,    TRUE,  ""      },
    {   "bash",          RES_BASH,      TRUE,  ""      },
    {   "pierce",        RES_PIERCE,    TRUE,  ""      },
    {   "slash",         RES_SLASH,     TRUE,  ""      },
    {   "fire",          RES_FIRE,      TRUE,  ""      },
    {   "cold",          RES_COLD,      TRUE,  ""      },
    {   "acid",          RES_ACID,      TRUE,  ""      },
    {   "poison",        RES_POISON,    TRUE,  ""      },
    {   "negative",      RES_NEGATIVE,  TRUE,  ""      },
    {   "holy",          RES_HOLY,      TRUE,  ""      },
    {   "energy",        RES_ENERGY,    TRUE,  ""      },
    {   "mental",        RES_MENTAL,    TRUE,  ""      },
    {   "disease",       RES_DISEASE,   TRUE,  ""      },
    {   "drowning",      RES_DROWNING,  TRUE,  ""      },
    {   "light",         RES_LIGHT,     TRUE,  ""      },
    {   "lightning",     RES_LIGHTNING, TRUE,  ""      },
    {  "sound",   RES_SOUND,  TRUE,  ""  },
    {  "wood",     RES_WOOD,  TRUE,  ""  },
    {  "silver",   RES_SILVER,  TRUE,  ""  },
    {  "iron",     RES_IRON,  TRUE,  ""  },
    {   NULL,            0,             0,  ""  }
};


const struct flag_type vuln_flags[] =
{
    {  "summon",   VULN_SUMMON,    TRUE,  ""  },
    {  "charm",   VULN_CHARM,    TRUE,  ""  },
    {   "magic",         VULN_MAGIC,             TRUE,  ""     },
    {   "weapon",        VULN_WEAPON,            TRUE,  ""    },
    {   "bash",          VULN_BASH,              TRUE,  ""     },
    {   "pierce",        VULN_PIERCE,            TRUE,  ""     },
    {   "slash",         VULN_SLASH,             TRUE,  ""      },
    {   "fire",          VULN_FIRE,              TRUE,  ""      },
    {   "cold",          VULN_COLD,              TRUE,  ""      },
    {   "acid",          VULN_ACID,              TRUE,  ""      },
    {   "poison",        VULN_POISON,            TRUE,  ""      },
    {   "negative",      VULN_NEGATIVE,          TRUE,  ""      },
    {   "holy",          VULN_HOLY,              TRUE,  ""      },
    {   "energy",        VULN_ENERGY,            TRUE,  ""      },
    {   "mental",        VULN_MENTAL,            TRUE,  ""      },
    {   "disease",       VULN_DISEASE,           TRUE,  ""      },
    {   "drowning",      VULN_DROWNING,          TRUE,  ""      },
    {   "light",         VULN_LIGHT,             TRUE,  ""      },
    {   "lightning",     VULN_LIGHTNING,         TRUE,  ""      },
    {  "sound",   VULN_SOUND,           TRUE,  ""  },
    {   "wood",          VULN_WOOD,              TRUE,  ""     },
    {   "silver",        VULN_SILVER,            TRUE,  ""      },
    {   "iron",          VULN_IRON,              TRUE,  ""      },
    {   NULL,              0,                    0,  ""      }
};

const struct flag_type position_flags[] =
{
    {   "dead",           POS_DEAD,            FALSE,  ""   },
    {   "mortal",         POS_MORTAL,          FALSE,  ""   },
    {   "incap",          POS_INCAP,           FALSE,  ""   },
    {   "stunned",        POS_STUNNED,         FALSE,  ""   },
    {   "sleeping",       POS_SLEEPING,        TRUE ,  ""   },
    {   "resting",        POS_RESTING,         TRUE ,  ""   },
    {   "sitting",        POS_SITTING,         TRUE ,  ""   },
    {   "bashed",         POS_BASHED,          FALSE ,  ""   },
    {   "fighting",       POS_FIGHTING,        FALSE,  ""   },
    {   "standing",       POS_STANDING,        TRUE ,  ""   },
    {   NULL,              0,                    0  ,  ""     }
};

const struct flag_type portal_flags[]=
{
    {   "normal_exit",    GATE_NORMAL_EXIT,  TRUE,  ""  },
    {  "no_curse",    GATE_NOCURSE,    TRUE,  ""  },
    {   "go_with",    GATE_GOWITH,    TRUE,  ""  },
    {   "buggy",    GATE_BUGGY,    TRUE,  ""  },
    {  "random",    GATE_RANDOM,    TRUE,  ""  },
    {  "undead",    GATE_UNDEAD,    TRUE,  ""  },
    {   NULL,      0,      0,  ""  }
};

const struct flag_type furniture_flags[]=
{
    {   "stand_at",    STAND_AT,    TRUE,  ""  },
    {  "stand_on",    STAND_ON,    TRUE,  ""  },
    {  "stand_in",    STAND_IN,    TRUE,  ""  },
    {  "sit_at",    SIT_AT,    TRUE,  ""  },
    {  "sit_on",    SIT_ON,    TRUE,  ""  },
    {  "sit_in",    SIT_IN,    TRUE,  ""  },
    {  "rest_at",    REST_AT,    TRUE,  ""  },
    {  "rest_on",    REST_ON,    TRUE,  ""  },
    {  "rest_in",    REST_IN,    TRUE,  ""  },
    {  "sleep_at",    SLEEP_AT,    TRUE,  ""  },
    {  "sleep_on",    SLEEP_ON,    TRUE,  ""  },
    {  "sleep_in",    SLEEP_IN,    TRUE,  ""  },
    {  "put_at",    PUT_AT,    TRUE,  ""  },
    {  "put_on",    PUT_ON,    TRUE,  ""  },
    {  "put_in",    PUT_IN,    TRUE,  ""  },
    {  "put_inside",    PUT_INSIDE,    TRUE,  ""  },
    {  NULL,      0,      0,  ""  }
};

const  struct  flag_type  apply_types  []=
{
  {  "affects",  TO_AFFECTS,  TRUE,  ""  },
  {  "object",  TO_OBJECT,  TRUE,  ""  },
  {  "immune",  TO_IMMUNE,  FALSE,  ""  },
  {  "resist",  TO_RESIST,  TRUE,  ""  },
  {  "vuln",    TO_VULN,  FALSE,  ""  },
  {  "weapon",  TO_WEAPON,  TRUE,  ""  },
  {  "player",  TO_PLR,    FALSE,  ""  },
  {  NULL,    0,    TRUE,  ""  }
};

const  struct  bit_type  bitvector_type  []=
{
  {  affect_flags,  "affect"  },
  {  apply_flags,  "apply"    },
  {  imm_flags,  "imm"    },
  {  dam_flags,  "res"    },
  {  vuln_flags,  "vuln"    },
  {  weapon_type2,  "weapon"  }
};

const struct flag_type oprog_flags[] =
{
    {  "act",      TRIG_ACT,    TRUE,  ""  },
    {  "fight",    TRIG_FIGHT,    TRUE,  ""  },
    {  "give",      TRIG_GIVE,    TRUE,  ""  },
    {   "grall",    TRIG_GRALL,    TRUE,  ""  },
    {  "random",    TRIG_RANDOM,    TRUE,  ""  },
    {   "speech",    TRIG_SPEECH,    TRUE,  ""  },
    {  "exall",    TRIG_EXALL,    TRUE,  ""  },
    {  "delay",    TRIG_DELAY,    TRUE,  ""  },
    {  "drop",      TRIG_DROP,    TRUE,  ""  },
    {  "get",      TRIG_GET,    TRUE,  ""  },
    {  "sit",      TRIG_SIT,    TRUE,  ""  },
    {  "wear",      TRIG_WEAR,    TRUE,  ""  },
    {  "command",    TRIG_COMMAND,    TRUE,  ""  },
    {  "new_cmd",    TRIG_NEW_CMD,    TRUE,  ""  },
    {  "post_cmd",    TRIG_POST_CMD,    TRUE,  ""  },
    {  "put",		TRIG_PUT,    TRUE,  ""  },
    {  NULL,      0,      TRUE,  ""  },
};

const struct flag_type rprog_flags[] =
{
    {  "act",      TRIG_ACT,    TRUE,  ""  },
    {  "fight",    TRIG_FIGHT,    TRUE,  ""  },
    {  "drop",      TRIG_DROP,    TRUE,  ""  },
    {  "grall",    TRIG_GRALL,    TRUE,  ""  },
    {  "random",    TRIG_RANDOM,    TRUE,  ""  },
    {  "speech",    TRIG_SPEECH,    TRUE,  ""  },
    {  "exall",    TRIG_EXALL,    TRUE,  ""  },
    {  "delay",    TRIG_DELAY,    TRUE,  ""  },
    {  "command",    TRIG_COMMAND,    TRUE,  ""  },
    {  "new_cmd",    TRIG_NEW_CMD,    TRUE,  ""  },
    {  "post_cmd",    TRIG_POST_CMD,    TRUE,  ""  },
    {  "death",    TRIG_DEATH,    TRUE,  ""  },
    {  "quit",      TRIG_QUIT,    TRUE,  ""      },
    {  NULL,      0,      TRUE,  ""  },
};

const struct flag_type material_table[] =
{
    {   "none",         0,        TRUE,  "нет"       },
    {   "steel",        MAT_STEEL,      TRUE,  "сталь"      },
    {   "stone",  MAT_STONE,  TRUE,  "камень"     },
    {   "brass",  MAT_BRASS,  TRUE,  "латунь"     },
    {   "bone",     MAT_BONE,  TRUE,  "кость"     },
    {   "energy",  MAT_ENERGY,  TRUE,  "энергия"     },
    {  "mithril",  MAT_MITHRIL,  TRUE,  "мифрил"     },
    {  "copper",  MAT_COPPER,  TRUE,  "медь"       },
    {  "silk",    MAT_SILK,  TRUE,  "шелк"       },
    {  "marble",  MAT_MARBLE,  TRUE,  "мрамор"     },
    {  "glass",  MAT_GLASS,  TRUE,  "стекло"     },
    {  "water",  MAT_WATER,  TRUE,  "вода"       },
    {  "flesh",  MAT_FLESH,  TRUE,  "плоть"     },
    {  "platinum",  MAT_PLATINUM,  TRUE,  "платина"     },
    {  "granite",  MAT_GRANITE,  TRUE,  "гранит"     },
    {  "leather",  MAT_LEATHER,  TRUE,  "кожа"       },
    {  "cloth",  MAT_CLOTH,  TRUE,  "ткань"     },
    {  "gemstone",  MAT_GEMSTONE,  TRUE,  "драгоценный камень"   },
    {  "gold",    MAT_GOLD,  TRUE,  "золото"     },
    {  "porcelain",  MAT_PORCELAIN,  TRUE,  "фарфор"     },
    {  "obsidian",  MAT_OBSIDIAN,  TRUE,  "обсидиан"     },
    {  "dragonscal",  MAT_DRAGONSCALE,TRUE,  "драконья чешуя"  },
    {  "ebony",  MAT_EBONY,  TRUE,  "черное дерево"    },
    {  "bronze",  MAT_BRONZE,  TRUE,  "бронза"     },
    {  "wood",    MAT_WOOD,  TRUE,  "дерево"     },
    {  "silver",  MAT_SILVER,  TRUE,  "серебро"     },
    {  "iron",    MAT_IRON,  TRUE,  "железо"     },
    {  "bloodstone",  MAT_BLOODSTONE,  TRUE,  "кровавый камень"   },
    {  "food",    MAT_FOOD,  TRUE,  "еда"       },
    {  "lead",    MAT_LEAD,  TRUE,  "свинец"     },
    {  "wax",    MAT_WAX,  TRUE,   "воск"      },
    {  "diamond",  MAT_DIAMOND,  TRUE,  "алмаз"      },
    {  "oldstyle",  MAT_OLDSTYLE,  TRUE,   "древний материал"  },
    {  "ice",  MAT_ICE,  TRUE,   "лед"  },
    {  "loam",  MAT_LOAM,  TRUE,   "глина"  },
    {   NULL,            0,             0,      ""      }
};

const struct flag_type target_flags[] =
{
    {  "Ignore",    TAR_IGNORE,    TRUE,  ""  },
    {  "CharOffensive",  TAR_CHAR_OFFENSIVE,  TRUE,  ""  },
    {  "CharDefensive",  TAR_CHAR_DEFENSIVE,  TRUE,  ""  },
    {  "CharSelf",    TAR_CHAR_SELF,    TRUE,  ""  },
    {  "ObjInventory",    TAR_OBJ_INV,    TRUE,  ""  },
    {  "ObjCharDefensive",  TAR_OBJ_CHAR_DEF,  TRUE,  ""  },
    {  "ObjCharOffensive",  TAR_OBJ_CHAR_OFF,  TRUE,  ""  },
    {  NULL,      0,      0,  ""  }
};

const struct flag_type attr_flags[] =
{
    {  "Str",      STAT_STR,    TRUE,  "сила"    },
    {  "Int",      STAT_INT,    TRUE,  "ум"    },
    {  "Wis",      STAT_WIS,    TRUE,  "мудрость"  },
    {  "Dex",      STAT_DEX,    TRUE,  "ловкость"  },
    {  "Con",      STAT_CON,    TRUE,  "сложение"  },
    {  NULL,      0,      0,  ""  }
};

const struct flag_type saves_flags[] =
{
    {  "ignore",    SAVES_IGNORE,    TRUE,  ""  },
    {  "half_dam",    SAVES_HALFDAM,    TRUE,  ""  },
    {  "quarter_dam",    SAVES_QUARTERDAM,  TRUE,  ""  },
    {  "3quarters_dam",  SAVES_3QTRDAM,    TRUE,  ""  },
    {  "third_dam",    SAVES_THIRDDAM,    TRUE,  ""  },
    {  "2thirds_dam",    SAVES_2THIRDSDAM,  TRUE,  ""  },
    {  "absorb_hp",    SAVES_ABSORB,    TRUE,  ""  },
    {  "absorb_mana",    SAVES_ABSORB_MANA,  TRUE,  ""  },
    {  "reflect",    SAVES_REFLECT,    TRUE,  ""  },
    {  "reflect_half",    SAVES_REFLECT_HALF,  TRUE,  ""  },
    {  "none_dam",    SAVES_NONE,    TRUE,  ""  },
    {  "full_dam",    SAVES_FULL,    TRUE,  ""  },
    {  NULL,      0,      0,  ""  }
};

const struct flag_type spell_flags[] =
{
    {  "area",      SPELL_AREA,    TRUE,  ""  },
    {  "area_safe",    SPELL_AREA_SAFE,  TRUE,  ""  },
    {  "buggy",    SPELL_BUGGY,    TRUE,  ""  },
    {  "noscribe",    SPELL_NOSCRIBE,    TRUE,  ""  },
    {  "nobrew",    SPELL_NOBREW,    TRUE,  ""  },
    {  "nowand",    SPELL_NOWAND,    TRUE,  ""  },
    {  "random",    SPELL_RANDOM,    TRUE,  ""  },
    {  "instant_kill",    SPELL_INSTANT_KILL,  TRUE,  ""  },
    {  "recastable",    SPELL_RECASTABLE,  TRUE,  ""  },
    {  "accum",    SPELL_ACCUM,    TRUE,  ""  },
    {  NULL,      0,      0,  ""  }
};

const struct flag_type spaff_flags[] =
{
    {  "caster_affect",  SPELLAFF_CASTER,  TRUE,  ""  },
    {  "victim_affect",  SPELLAFF_VICTIM,  TRUE,  ""  },
    {  "obj_affect",    SPELLAFF_OBJ,    TRUE,  ""  },
    {  "room_affect",    SPELLAFF_ROOM,    TRUE,  ""  },
    {  "group_affect",    SPELLAFF_GROUP,    TRUE,  ""  },
    {  NULL,      0,      0,  ""  }
};

const struct flag_type align_flags[] =
{
    {  "evil",      ALIGN_EVIL,    TRUE,  ""  },
    {  "neutral",    ALIGN_NEUTRAL,    TRUE,  ""  },
    {  "good",      ALIGN_GOOD,    TRUE,  ""  },
    {  NULL,      0,      0,  ""  }
};

const struct flag_type where_flags[] =
{
    {  "affects",    TO_AFFECTS,    TRUE,  ""  },
    {  "object",    TO_OBJECT,    TRUE,  ""  },
    {  "immun",    TO_IMMUNE,    TRUE,  ""  },
    {  "resist",    TO_RESIST,    TRUE,  ""  },
    {  "vuln",      TO_VULN,    TRUE,  ""  },
    {  "weapon",    TO_WEAPON,    TRUE,  ""  },
    {  "room",      TO_ROOM_AFF,    TRUE,  ""  },
    {  "player",    TO_PLR,      FALSE,  ""  },
    {  NULL,      0,      0,  ""  }
};

const struct flag_type color_flags[] =
{
    {  "red",      'r',      TRUE,  ""  },
    {  "green",    'g',      TRUE,  ""  },
    {  "brown",    'y',      TRUE,  ""  },
    {  "blue",      'b',      TRUE,  ""  },
    {  "magenta",    'm',      TRUE,  ""  },
    {  "cyan",      'c',      TRUE,  ""  },
    {  "gray",      'w',      TRUE,  ""  },
    {  "darkgray",    'D',      TRUE,  ""  },
    {  "bright_red",    'R',      TRUE,  ""  },
    {  "bright_green",    'G',      TRUE,  ""  },
    {  "yellow",    'Y',      TRUE,  ""  },
    {  "bright_blue",    'B',      TRUE,  ""  },
    {  "bright_magenta",  'M',      TRUE,  ""  },
    {  "bright_cyan",    'C',      TRUE,  ""  },
    {  "white",    'W',      TRUE,  ""  },
    {  NULL,      0,      0,  ""  }
};

const struct flag_type comf_flags[] =
{
    {  "good",    FOR_GOOD,    TRUE,  "добрых"  },
    {  "neutral",  FOR_NEUTRAL,    TRUE,  "нейтральных"  },
    {  "evil",    FOR_EVIL,    TRUE,  "злых"    },
    {  "man",    FOR_MAN,    TRUE,  "мужчин"  },
    {  "woman",  FOR_WOMAN,    TRUE,  "женщин"  },
    {  "mage",    FOR_MAGE,    TRUE,  "магов"    },
    {  "cleric",  FOR_CLERIC,    TRUE,  "жрецов"  },
    {  "thief",  FOR_THIEF,    TRUE,  "воров"    },
    {  "warrior",  FOR_WARRIOR,    TRUE,  "воинов"  },
    {  "necromancer",  FOR_NECROMANCER,  TRUE,  "некромантов"  },
    {  "paladin",  FOR_PALADIN,    TRUE,  "паладинов"  },
    {  "nazgul",  FOR_NAZGUL,    TRUE,  "назгулов"  },
    {  "druid",  FOR_DRUID,    TRUE,  "друидов"  },
    {  "ranger",  FOR_RANGER,    TRUE,  "егерей"  },
    {  "vampire",  FOR_VAMPIRE,    TRUE,  "вампиров"  },
    {  "human",  FOR_HUMAN,    TRUE,  "людей"    },
    {  "elf",    FOR_ELF,    TRUE,  "эльфов"  },
    {  "dwarf",  FOR_DWARF,    TRUE,  "гномов"  },
    {  "giant",  FOR_GIANT,    TRUE,  "гигантов"  },
    {  "orc",    FOR_ORC,    TRUE,  "орков"    },
    {  "troll",  FOR_TROLL,    TRUE,  "троллей"  },
    {  "snake",  FOR_SNAKE,    TRUE,  "змеев"    },
    {  "hobbit",  FOR_HOBBIT,    TRUE,  "хоббитов"  },
    {  "drow",    FOR_DROW,    TRUE,  "дроу"    },
    {  "alchemist", FOR_ALCHEMIST,    TRUE,  "алхимиков"    },
    {  "lycanthrope", FOR_LYCANTHROPE,    TRUE,  "оборотней"    },
    {  "zombie",  FOR_ZOMBIE,    TRUE,  "зомби"  },
    {  "tigguan",  FOR_TIGGUAN,    TRUE,  "тиггуанов"  },
    {  NULL,    0,      0,  ""    },
};

const struct flag_type trap_flags[] =
{
    {	"enter",	TRAP_ON_ENTER,	TRUE,	"вход"		},
    {	"open",		TRAP_ON_OPEN,	TRUE,	"открыть"	},
    {	"unlock",	TRAP_ON_UNLOCK,	TRUE,	"отпереть"	},
    {	"get",		TRAP_ON_GET,	TRUE,	"взять"		},
    {	"pick",		TRAP_ON_PICK,	TRUE,	"взломать"	},
    {	"no_disarm",	TRAP_NO_DISARM,	TRUE,	"не_обезвредить"},
    {	NULL,		0,		0,	""		}
};

const struct flag_type exit_dirs[] =
{
    {	"north",	DIR_NORTH,	TRUE,	"север"	},
    {	"south",	DIR_SOUTH,	TRUE,	"юг"	},
    {	"west",		DIR_WEST,	TRUE,	"запад"	},
    {	"east",		DIR_EAST,	TRUE,	"Восток"},
    {	"up",		DIR_UP,		TRUE,	"вверх"	},
    {	"down",		DIR_DOWN,	TRUE,	"вниз"	},
    {	NULL,		0,		0,	""	}
};
    
const struct material material_waste[] =
{                    	/*    BASH  PIERCE  SLASH  OTHER  */
    {  0,      			1.0,  1.0,  1.0,  1.0  },
    {  MAT_STEEL,    		2.2,  1.9,  2.2,  2.0  },
    {  MAT_STONE,    		1.4,  2.5,  2.8,  2.0  },
    {  MAT_BRASS,    		1.2,  0.8,  1.0,  1.1  },
    {  MAT_BONE,    		1.0,  2.0,  1.5,  1.5  },
    {  MAT_ENERGY,    		1.3,  1.3,  1.3,  1.3  },
    {  MAT_MITHRIL,    		2.5,  2.5,  2.5,  2.5  },
    {  MAT_COPPER,   		1.5,  1.0,  1.2,  1.2  },
    {  MAT_SILK,    		0.9,  0.7,  0.7,  0.8  },
    {  MAT_MARBLE,    		1.0,  2.0,  1.9,  1.5  },
    {  MAT_GLASS,    		0.7,  1.2,  1.0,  1.0  },
    {  MAT_WATER,    		0.7,  0.7,  0.7,  0.7  },
    {  MAT_FLESH,    		0.8,  0.7,  0.7,  0.6  },
    {  MAT_PLATINUM,    	2.1,  2.1,  2.3,  2.7  },
    {  MAT_GRANITE,    		1.2,  1.4,  1.4,  1.3  },
    {  MAT_LEATHER,    		1.1,  1.0,  1.0,  1.0  },
    {  MAT_CLOTH,    		1.0,  0.9,  0.9,  0.9  },
    {  MAT_GEMSTONE,    	1.0,  1.0,  1.0,  1.0  },
    {  MAT_GOLD,    		1.2,  1.0,  1.1,  1.3  },
    {  MAT_PORCELAIN,    	0.9,  1.2,  1.2,  1.2  },
    {  MAT_OBSIDIAN,    	2.4,  2.6,  2.6,  2.7  },
    {  MAT_DRAGONSCALE,  	1.9,  1.5,  2.2,  2.3  },
    {  MAT_EBONY,    		1.4,  1.4,  1.4,  1.4  },
    {  MAT_BRONZE,    		1.6,  1.3,  1.5,  1.3  },
    {  MAT_WOOD,    		0.9,  0.9,  0.9,  0.8  },
    {  MAT_SILVER,    		1.9,  1.6,  1.8,  1.7  },
    {  MAT_IRON,    		2.0,  1.8,  1.9,  1.9  },
    {  MAT_BLOODSTONE,    	1.1,  1.1,  1.1,  1.1  },
    {  MAT_FOOD,    		0.8,  0.8,  0.8,  0.8  },
    {  MAT_LEAD,    		1.0,  0.7,  0.8,  1.1  },
    {  MAT_WAX,    		0.7,  0.5,  0.6,  0.7  },
    {  MAT_DIAMOND,    		1.5,  4.0,  4.0,  4.0  },
    {  MAT_OLDSTYLE,    	1.2,  1.2,  1.2,  1.2  },
    {  MAT_ICE,    		1.4,  0.7,  0.8,  0.8  },
    {  MAT_LOAM,    		0.8,  0.8,  0.8,  1  },
    {  0,      0,  0,  0,  0  }
};

const struct damres_t damres[DAM_MAX] =
{
    {  DAM_NONE,  -1    },
    {  DAM_BASH,  RES_BASH  },
    {  DAM_PIERCE,  RES_PIERCE  },
    {  DAM_SLASH,  RES_SLASH  },
    {  DAM_FIRE,  RES_FIRE  },
    {  DAM_COLD,  RES_COLD  },
    {  DAM_LIGHTNING,  RES_LIGHTNING  },
    {  DAM_ACID,  RES_ACID  },
    {  DAM_POISON,  RES_POISON  },
    {  DAM_NEGATIVE,  RES_NEGATIVE  },
    {  DAM_HOLY,  RES_HOLY  },
    {  DAM_ENERGY,  RES_ENERGY  },
    {  DAM_MENTAL,  RES_MENTAL  },
    {  DAM_DISEASE,  RES_DISEASE  },
    {  DAM_DROWNING,  RES_DROWNING  },
    {  DAM_LIGHT,  RES_LIGHT  },
    {  DAM_OTHER,  -1    },
    {  DAM_HARM,  -1    },
    {  DAM_CHARM,  RES_CHARM  },
    {  DAM_SOUND,  RES_SOUND  },
    {  DAM_WOOD,  RES_WOOD  },
    {  DAM_SILVER,  RES_SILVER  },
    {  DAM_IRON,  RES_IRON  },
    {  DAM_MAGIC,  RES_MAGIC  },
    {  DAM_SUMMON,  RES_SUMMON  },
    {  DAM_WEAPON,  RES_WEAPON  }
};

const struct flag_type magic_class_flags[] =
{
    {  "nomagic",  CLASS_NO_MAGIC      ,    FALSE,  "немагический"    },
    {  "slightly",  CLASS_SLIGHTLY_MAGIC,    FALSE,  "слегка_магический"  },
    {  "semimagic",  CLASS_SEMI_MAGIC    ,    FALSE,  "полумагический"  },
    {  "magic",  CLASS_MAGIC         ,    FALSE,  "магический"    },
    {  NULL,    0,        0,  ""      }
};

const struct flag_type trig_flags[] =
{
    {  "act",          LTRIG_ACT,      TRUE,   ""      },
    {  "bribe",        LTRIG_BRIBE,    TRUE,   ""      },
    {  "death",        LTRIG_DEATH,    TRUE,   ""      },
    {  "entry",        LTRIG_ENTRY,    TRUE,   ""      },
    {  "fight",        LTRIG_FIGHT,    TRUE,   ""      },
    {  "give",         LTRIG_GIVE,     TRUE,   ""      },
    {  "greet",        LTRIG_GREET,    TRUE,   ""      },
    {  "grall",        LTRIG_GRALL,    TRUE,   ""      },
    {  "kill",         LTRIG_KILL,     TRUE,   ""      },
    {  "hpcnt",        LTRIG_HPCNT,    TRUE,   ""      },
    {  "random",       LTRIG_RANDOM,   TRUE,   ""      },
    {  "speech",       LTRIG_SPEECH,   TRUE,   ""      },
    {  "exit",         LTRIG_EXIT,     TRUE,   ""      },
    {  "exall",        LTRIG_EXALL,    TRUE,   ""      },
    {  "delay",        LTRIG_DELAY,    TRUE,   ""      },
    {  "surr",         LTRIG_SURR,     TRUE,   ""      },
    {  "get",          LTRIG_GET,      TRUE,   ""      },
    {  "drop",         LTRIG_DROP,     TRUE,   ""      },
    {  "sit",          LTRIG_SIT,      TRUE,   ""      },
    {  "wear",         LTRIG_WEAR,     TRUE,   ""      },
    {  "command",      LTRIG_COMMAND,  TRUE,   ""      },
    {  "new_cmd",      LTRIG_NEW_CMD,  TRUE,   ""      },
    {  "post_cmd",     LTRIG_POST_CMD, TRUE,   ""      },
    {  NULL,           0,              0,      ""      }
};

const struct flag_type prog_when_flags[] =
{
    {  "default",  	EXEC_DEFAULT,   TRUE,  "по_умолчанию"	},
    {  "before",   	EXEC_BEFORE,    TRUE,  "до"  		},
    {  "after",  	EXEC_AFTER,    	TRUE,  "после"  	},
    {  NULL,    	0,        	0,  	""      	}
};

/* charset=cp1251 */
