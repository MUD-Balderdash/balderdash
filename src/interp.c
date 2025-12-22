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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "interp.h"
#include "olc.h"
#include "config.h"


bool	check_social	args((CHAR_DATA *ch, char *command, char *argument));
bool 	convert_trans(CHAR_DATA *ch, char *arg);
/*
 * Command logging types.
 */
#define LOG_NORMAL	0
#define LOG_ALWAYS	1
#define LOG_NEVER	2


char    last_command[2*MAX_INPUT_LENGTH];
int	max_commands;

#ifdef MUD_MEM_DEBUG
extern int nAllocString;
extern int nAllocPerm;
#endif



/*
 * Command table.
 */
const	struct	cmd_type	cmd_table	[] =
{
    /*
     * Common movement commands.
     */
    { "north",		do_north,	POS_STANDING,    0,  LOG_NEVER, 0 },
    { "east",		do_east,	POS_STANDING,	 0,  LOG_NEVER, 0 },
    { "south",		do_south,	POS_STANDING,	 0,  LOG_NEVER, 0 },
    { "west",		do_west,	POS_STANDING,	 0,  LOG_NEVER, 0 },
    { "up",		do_up,		POS_STANDING,	 0,  LOG_NEVER, 0 },
    { "down",		do_down,	POS_STANDING,	 0,  LOG_NEVER, 0 },

    { "север",		do_north,	POS_STANDING,    0,  LOG_NEVER, 0 },
    { "восток",		do_east,	POS_STANDING,	 0,  LOG_NEVER, 0 },
    { "юг",		do_south,	POS_STANDING,	 0,  LOG_NEVER, 0 },
    { "запад",		do_west,	POS_STANDING,	 0,  LOG_NEVER, 0 },
    { "верх",		do_up,		POS_STANDING,	 0,  LOG_NEVER, 0 },
    { "низ",		do_down,	POS_STANDING,	 0,  LOG_NEVER, 0 },
    { "вверх",		do_up,		POS_STANDING,	 0,  LOG_NEVER, 0 },
    { "вниз",		do_down,	POS_STANDING,	 0,  LOG_NEVER, 0 },

    /*
     * Common other commands.
     * Placed here so one and two letter abbreviations work.
     */

    { "cast",		do_cast,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
/*    { "auction",        do_auction,     POS_RESTING,    0,  LOG_NORMAL, 0 },*/
    { "buy",		do_buy,		POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "channels",       do_channels,    POS_DEAD,        0,  LOG_NORMAL, 0 },
    { "exits",		do_exits,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "get",		do_get,		POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "goto",           do_goto,        POS_DEAD,       L8,  LOG_NORMAL, 0 },
    { "group",          do_group,       POS_SLEEPING,    0,  LOG_NORMAL, 0 },
    { "gaffect",        do_gaffect,     POS_RESTING,     0,  LOG_NORMAL, 0 },
    { "hit",		do_kill,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    { "inventory",	do_inventory,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "kill",		do_kill,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    { "look",		do_look,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
	{ "suicid",		do_suicid,	POS_RESTING,	 0,	 LOG_NORMAL, 0 },
    { "suicide",	do_suicide ,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "music",          do_music,   	POS_SLEEPING,    0,  LOG_NORMAL, 0 },
    { "declaim",        do_declaim,   	POS_SLEEPING,    0,  LOG_NORMAL, 0 },
    { "order",		do_order,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "practice",       do_practice,	POS_SLEEPING,    0,  LOG_NORMAL, 0 },
    { "rest",		do_rest,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 },
    { "sit",		do_sit,		POS_SLEEPING,    0,  LOG_NORMAL, 0 },
    { "sockets",        do_sockets,	POS_DEAD,       L3,  LOG_NORMAL, 0 },
    { "stand",		do_stand,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 },
    { "tell",		do_tell_,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "unlock",         do_unlock,      POS_RESTING,     0,  LOG_NORMAL, 0 },
    { "wield",		do_wield,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "second",		do_second,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "wizhelp",	do_wizhelp,	POS_DEAD,	IM,  LOG_NORMAL, 0 },
    { "sky",		do_look_sky,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "run",		do_run,		POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "gallop",		do_gallop,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "scribe",		do_scribe,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "insert",		do_insert,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "cover",		do_cover,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "lostcommand",	do_lost_command,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 },
    { "reganswer",	do_reg_answer,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 },

/*    { "at",             do_at,          POS_DEAD,       L6,  LOG_NORMAL, 1 }, */
    { "колдовать",	do_cast,	POS_FIGHTING,	 0,  LOG_NORMAL, 1 },
    { "аукцион",        do_auction,     POS_RESTING,    0,  LOG_NORMAL, 1 },
    { "купить",		do_buy,		POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "каналы", 	do_channels,    POS_DEAD,        0,  LOG_NORMAL, 1 },
    { "выходы",		do_exits,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "взять",		do_get,		POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "прыгнуть",       do_goto,        POS_DEAD,       L8,  LOG_NORMAL, 1 },
    { "группа",         do_group,       POS_SLEEPING,    0,  LOG_NORMAL, 1 },
    { "гэффекты",       do_gaffect,     POS_RESTING,     0,  LOG_NORMAL, 1 },
/*    { "hit",		do_kill,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 }, */
    { "инвентарь",	do_inventory,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "вещи",		do_inventory,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "убить",		do_kill,	POS_FIGHTING,	 0,  LOG_NORMAL, 1 },
    { "смотреть",	do_look,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "самоубийств",	do_suicid,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "самоубийство",	do_suicide,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "вглядеться",	do_good_look,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "клан",		do_clan,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 },
    { "кг",		do_clantalk,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 },
    { "ксоюзг",		do_clantalkunion,POS_SLEEPING,	 0,  LOG_NORMAL, 1 },
    { "рговорить",	do_racetalk,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "музыка",         do_music,   	POS_RESTING,     0,  LOG_NORMAL, 1 },
    { "декламировать",  do_declaim,   	POS_RESTING,     0,  LOG_NORMAL, 1 },
    { "приказать",	do_order,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "присмотреться",	do_good_look,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "практиковать",   do_practice,	POS_SLEEPING,    0,  LOG_NORMAL, 1 },
    { "отдых",		do_rest,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 },
    { "сесть",		do_sit,		POS_SLEEPING,    0,  LOG_NORMAL, 1 },
    { "присесть",	do_sit,		POS_SLEEPING,    0,  LOG_NORMAL, 0 },
    { "сидеть",		do_sit,		POS_SLEEPING,    0,  LOG_NORMAL, 0 },
    { "сокеты",         do_sockets,	POS_DEAD,       L4,  LOG_NORMAL, 1 },
    { "юзеры",          do_sockets,	POS_DEAD,       L4,  LOG_NORMAL, 0 },
    { "встать",		do_stand,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 },
    { "всмотреться",	do_good_look,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "говорить",	do_tell_,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "отпереть",       do_unlock,      POS_RESTING,     0,  LOG_NORMAL, 1 },
    { "второе",		do_second,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "спать",		do_sleep,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 },
    { "небо",		do_look_sky,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "побежать",	do_run,		POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "начертать",	do_scribe,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "вставить",	do_insert,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "укутаться",	do_cover,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "лосткоманда", do_lost_command,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 },
/*    { "wizhelp",	do_wizhelp,	POS_DEAD,	IM,  LOG_NORMAL, 1 }, */

    /*
     * Informational commands.
     */
    { "affects",	do_affects,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "areas",		do_areas,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "changes",	do_changes,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "commands",	do_commands,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "compare",	do_compare,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "consider",	do_consider,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "count",		do_count,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 },
    { "credits",	do_credits,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "equipment",	do_equipment,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "examine",	do_examine,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
/*  { "groups",		do_groups,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 }, */
    { "help",		do_help,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "idea",		do_idea,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "info",           do_groups,      POS_SLEEPING,    0,  LOG_NORMAL, 0 },
    { "motd",		do_motd,	POS_DEAD,        0,  LOG_NORMAL, 0 },
    { "news",		do_news,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "read",		do_read,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "report",		do_report,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "rules",		do_rules,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "score",		do_score,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "skills",		do_skills,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "socials",	do_socials,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "show",		do_show,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "spells",		do_spells,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "story",		do_story,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "time",		do_time,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "systime",	do_systime,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "weather",	do_weather,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "who",		do_who,		POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "whois",		do_whois,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "wizlist",	do_wizlist,	POS_DEAD,        0,  LOG_NORMAL, 0 },
    { "worth",		do_worth,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 },
    { "repent",		do_repent,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "vote",		do_votes,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "эффекты",	do_affects,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "бессмертие",	do_temp_RIP,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "зоны",		do_areas,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "изменения",	do_changes,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "команды",	do_commands,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "сравнить",	do_compare,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "шансы",		do_consider,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
/*    { "credits",	do_credits,	POS_DEAD,	 0,  LOG_NORMAL, 1 }, */
    { "экипировка",	do_equipment,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "проверить",	do_examine,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
/*  { "groups",		do_groups,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 }, */
    { "помощь",		do_help,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "справка",	do_help,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "?",		do_help,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "идея",		do_idea,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "инфо",           do_groups,      POS_SLEEPING,    0,  LOG_NORMAL, 1 },
/*    { "motd",		do_motd,	POS_DEAD,        0,  LOG_NORMAL, 1 }, */
    { "новости",	do_news,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "читать",		do_read,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "отчет",		do_report,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "правила",	do_rules,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "счет",		do_score,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "счетчик",	do_count,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 },
    { "умения",		do_skills,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "социалы",	do_socials,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "общение",	do_socials,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "показ",		do_show,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "заклинания",	do_spells,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "история",	do_story,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "время",		do_time,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "сисвремя",	do_systime,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "погода",		do_weather,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "кто",		do_who,		POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "ктоесть",	do_whois,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
/*    { "wizlist",	do_wizlist,	POS_DEAD,        0,  LOG_NORMAL, 1 }, */
    { "доход",		do_worth,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 },
    { "боги",		do_wizlist,	POS_DEAD,        0,  LOG_NORMAL, 0 },
    { "покаяться",	do_repent,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "опрос",		do_votes,	POS_DEAD,	 0,  LOG_NORMAL, 1 },

    /*
     * Configuration commands.
     */

/*    { "compress",	do_compress,	POS_DEAD,        0,  LOG_NORMAL, 1 }, */
    { "alia",		do_alia,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "alias",		do_alias,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "autolist",	do_autolist,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "autoassist",	do_autoassist,	POS_DEAD,        0,  LOG_NORMAL, 0 },
    { "autoexit",	do_autoexit,	POS_DEAD,        0,  LOG_NORMAL, 0 },
    { "autogold",	do_autogold,	POS_DEAD,        0,  LOG_NORMAL, 0 },
    { "automoney",	do_automoney,	POS_DEAD,        0,  LOG_NORMAL, 0 },
    { "autoloot",	do_autoloot,	POS_DEAD,        0,  LOG_NORMAL, 0 },
	{ "autosac",	do_autosac,	POS_DEAD,        0,  LOG_NORMAL, 0 },
    { "autotitle",	do_autotitle,	POS_DEAD,        0,  LOG_NORMAL, 0 },
    { "autosplit",	do_autosplit,	POS_DEAD,        0,  LOG_NORMAL, 0 },
    { "autodreams",	do_autodreams,	POS_DEAD,        0,  LOG_NORMAL, 0 },
    { "automarry",	do_automarry,	POS_STANDING,        0,  LOG_NORMAL, 0 },
    { "brief",		do_brief,	POS_DEAD,        0,  LOG_NORMAL, 0 },
    { "colour",		do_colour,	POS_DEAD,        0,  LOG_NORMAL, 0 },
    { "color",		do_colour,	POS_DEAD,        0,  LOG_NORMAL, 0 },
/*    { "combine",	do_combine,	POS_DEAD,        0,  LOG_NORMAL, 0 }, */
    { "compact",	do_compact,	POS_DEAD,        0,  LOG_NORMAL, 0 },
    { "descriptio",	do_descriptio,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "description",	do_description,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "delet",		do_delet,	POS_DEAD,	 0,  LOG_ALWAYS, 0 },
    { "delete",		do_delete,	POS_STANDING,	 0,  LOG_ALWAYS, 0 },
    { "nofollow",	do_nofollow,	POS_DEAD,        0,  LOG_NORMAL, 0 },
    { "nocancel",	do_nocancel,	POS_DEAD,        0,  LOG_NORMAL, 0 },
    { "outfit",		do_outfit,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "password",	do_password,	POS_DEAD,	 0,  LOG_NEVER,  0 },
    { "prompt",		do_prompt,	POS_DEAD,        0,  LOG_NORMAL, 0 },
    { "scroll",		do_scroll,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "titl",		do_titl,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "title",		do_title,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "unalias",	do_unalias,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "wimpy",		do_wimpy,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "email",		do_email,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "translit",	do_translit,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "notepad",	do_char_notepad,POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "псевдони",	do_alia,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "псевдоним",	do_alias,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "авто",		do_autolist,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "автопомощь",	do_autoassist,	POS_DEAD,        0,  LOG_NORMAL, 1 },
    { "автовыходы",	do_autoexit,	POS_DEAD,        0,  LOG_NORMAL, 1 },
    { "автозолото",	do_autogold,	POS_DEAD,        0,  LOG_NORMAL, 1 },
    { "автомонеты",	do_automoney,	POS_DEAD,        0,  LOG_NORMAL, 1 },
    { "автограбеж",	do_autoloot,	POS_DEAD,        0,  LOG_NORMAL, 1 },
    { "автограбёж",	do_autoloot,	POS_DEAD,        0,  LOG_NORMAL, 1 },
    { "автожертва",	do_autosac,	POS_DEAD,        0,  LOG_NORMAL, 1 },
    { "автодележ",	do_autosplit,	POS_DEAD,        0,  LOG_NORMAL, 1 },
    { "автоделёж",	do_autosplit,	POS_DEAD,        0,  LOG_NORMAL, 0 },
    { "автотитул",	do_autotitle,	POS_DEAD,        0,  LOG_NORMAL, 1 },
    { "автосон",	do_autodreams,	POS_DEAD,        0,  LOG_NORMAL, 0 },
    { "свадьба",	do_automarry,	POS_STANDING,        0,  LOG_NORMAL, 0 },
    { "кратко",		do_brief,	POS_DEAD,        0,  LOG_NORMAL, 1 },
    { "атака",		do_autoattack,	POS_STANDING,	 0,  LOG_NORMAL, 1 }, 
    { "цвет",		do_colour,	POS_DEAD,        0,  LOG_NORMAL, 1 },
/*  { "combine",	do_combine,	POS_DEAD,        0,  LOG_NORMAL, 1 }, */
    { "компактно",	do_compact,	POS_DEAD,        0,  LOG_NORMAL, 1 },
    { "описани",	do_descriptio,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "описание",	do_description,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "удалит",		do_delet,	POS_DEAD,	 0,  LOG_ALWAYS, 0 },
    { "удалить",	do_delete,	POS_STANDING,	 0,  LOG_ALWAYS, 1 },
    { "неследовать",	do_nofollow,	POS_DEAD,        0,  LOG_NORMAL, 1 },
    { "неотменить",	do_nocancel,	POS_DEAD,        0,  LOG_NORMAL, 1 },
/*    { "неграбить",		do_noloot,	POS_DEAD,        0,  LOG_NORMAL, 1 }, */
/*    { "непризыв",	do_nosummon,	POS_DEAD,        0,  LOG_NORMAL, 1 }, */
    { "снарядиться",	do_outfit,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "пароль",		do_password,	POS_DEAD,	 0,  LOG_NEVER,  1 },
    { "промпт",		do_prompt,	POS_DEAD,        0,  LOG_NORMAL, 0 },
    { "подсказка",	do_help,	POS_DEAD,        0,  LOG_NORMAL, 1 },
    { "экран",		do_scroll,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "титу",		do_titl,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "титул",		do_title,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
/*    { "unalias",	do_unalias,	POS_DEAD,	 0,  LOG_NORMAL, 1 }, */
    { "осторожность",	do_wimpy,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "почта",		do_email,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "разрешить",	do_setip,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "запис",		do_char_notepa, POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "записи",		do_char_notepad,POS_DEAD,	 0,  LOG_NORMAL, 1 },

    /*
     * Communication commands.
     */
    { "afk",		do_afk,		POS_SLEEPING,	 0,  LOG_NORMAL, 0 },
    { "answer",		do_reply,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 },
/*  { "auction",	do_auction,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 }, */
    { "deaf",		do_deaf,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "emote",		do_emote,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
/*    { "pmote",		do_pmote,	POS_RESTING,	 0,  LOG_NORMAL, 0 }, */
    { ".",		do_gossip,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "gossip",		do_gossip,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "ooc",		do_ooc,		POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "slang",		do_slang,		POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { ", ",		do_emote,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "grats",		do_grats,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "gtell",		do_gtell,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
/*  { "music",		do_music,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 }, */
    { "note",		do_note,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 },
/*    { "question",	do_question,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 }, */
/*    { "quote",		do_quote,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 }, */
    { "quiet",		do_quiet,	POS_SLEEPING, 	 0,  LOG_NORMAL, 0 },
    { "reply",		do_reply,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 },
    { "replay",		do_replay,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 },
    { "say",		do_say,		POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "'",		do_say,		POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "shout",		do_shout,	POS_RESTING,	 3,  LOG_NORMAL, 0 },
    { "unread",		do_unread,	POS_SLEEPING,    0,  LOG_NORMAL, 0 },
    { "yell",		do_yell,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "racetalk",	do_racetalk,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "filter",		do_filter,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 },
    { "beep",		do_beep,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "helper",		do_helper,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 },
    { "noexp",		do_noexp,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 },
    { "camomile",	do_camomile,	POS_STANDING,	 0,  LOG_NORMAL, 0 },

    { "вок",		do_afk,		POS_SLEEPING,	 0,  LOG_NORMAL, 1 },
    { "ответить",	do_reply,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 },
/*  { "auction",	do_auction,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 }, */
    { "глухой",		do_deaf,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "эмоция",		do_emote,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
/*    { "pmote",		do_pmote,	POS_RESTING,	 0,  LOG_NORMAL, 1 }, */
/*    { ".",		do_gossip,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 }, */
    { "болтать",	do_gossip,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "трепаться",	do_ooc,		POS_RESTING,	 0,  LOG_NORMAL, 1 },
/*    { ", ",		do_emote,	POS_RESTING,	 0,  LOG_NORMAL, 0 }, */
    { "поздравить",	do_grats,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "гговорить",	do_gtell,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
/*    { ";",		do_gtell,	POS_DEAD,	 0,  LOG_NORMAL, 0 }, */
/*  { "music",		do_music,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 }, */
    { "письмо",		do_note,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 },
/*    { "спросить",	do_question,	POS_RESTING,	 0,  LOG_NORMAL, 1 }, */
/*    { "quote",	do_quote,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 }, */
    { "тихо",		do_quiet,	POS_SLEEPING, 	 0,  LOG_NORMAL, 1 },
    { "реплика",	do_reply,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 },
    { "вответ",		do_reply,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 },
    { "воспроизвести",	do_replay,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 },
    { "сказать",	do_say,		POS_RESTING,	 0,  LOG_NORMAL, 1 },
/*    { "'",		do_say,		POS_RESTING,	 0,  LOG_NORMAL, 0 }, */
    { "орать",		do_shout,	POS_RESTING,	 3,  LOG_NORMAL, 1 },
    { "сквернословить",	do_slang,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "кричать",	do_shout,	POS_RESTING,	 3,  LOG_NORMAL, 0 },
    { "непрочитанные",	do_unread,	POS_SLEEPING,    0,  LOG_NORMAL, 1 },
    { "вопить",		do_yell,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "фильтр",		do_filter,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 },
    { "пикнуть",	do_beep,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "помощник",	do_helper,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 },
    { "помочь",		do_helper,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 },
    { "безопыта",	do_noexp,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 },
    { "ромашка",	do_camomile,	POS_STANDING,	 0,  LOG_NORMAL, 1 },

    /*
     * Object manipulation commands.
     */
    { "brandish",	do_brandish,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "drink",		do_drink,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "drop",		do_drop,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "eat",		do_eat,		POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "envenom",	do_envenom,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "fill",		do_fill,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "give",		do_give,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "heal",		do_heal,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "hold",		do_wear,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "list",		do_list,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "lock",		do_lock,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "open",		do_open_,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "pick",		do_pick,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "pour",		do_pour,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "put",		do_put,		POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "quaff",		do_quaff,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "recite",		do_recite,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "remove",		do_remove,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "sell",		do_sell,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "take",		do_get,		POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "sacrifice",	do_sacrifice,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "junk",           do_sacrifice,   POS_RESTING,     0,  LOG_NORMAL, 0 },
    { "tap",      	do_sacrifice,   POS_RESTING,     0,  LOG_NORMAL, 0 },
/*  { "unlock",		do_unlock,	POS_RESTING,	 0,  LOG_NORMAL, 0 }, */
    { "value",		do_value,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "wear",		do_wear,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "zap",		do_zap,		POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "sharpen",	do_sharpen,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "csharpen",	do_csharpen,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "bounty",		do_bounty,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "lore",		do_lore,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "reinforce",	do_reinforce,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "activate",	do_activate,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    { "camp",		do_camp,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "herbs",		do_herbs,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "track",		do_track,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "camouflage",	do_camouflage,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "ambush",		do_ambush,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    { "butcher",	do_butcher,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "spring",		do_find_spring,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "trap",		do_trap,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "torch",		do_torch,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "repair",		do_repair,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "fly",		do_levitate,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "weigh",		do_weigh,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "gift",		do_gift,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "cut",		do_make_bag,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "socketing",	do_socketing,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "personalize",	do_renaming,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "ritual",		do_blood_ritual,POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "concentrate",	do_concentrate, POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "prayer",		do_prayer,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "grimuar",	do_grimuar,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    //команды для приглашения чара домой / отказа от приглашени
    { "invite",		do_invite,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "refuse",		do_refuse,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    //команды постройки дома
    { "roombuild",		do_build_room,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "mobbuild",		do_build_mob,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "objbuild",		do_build_obj, 	POS_RESTING,	 0,  LOG_NORMAL, 1 },

    { "взмахнуть",	do_brandish,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "размахивать",	do_brandish,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "закрыть",	do_close_,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "пить",		do_drink,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "выпить",		do_drink,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "бросить",	do_drop,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "выбросить",	do_drop,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "есть",		do_eat,		POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "кушать",		do_eat,		POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "отравить",	do_envenom,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "наполнить",	do_fill,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "дать",		do_give,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "давать",		do_give,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "отдать",		do_give,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "лечить",		do_heal,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "держать",	do_wear,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "список",		do_list,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "запереть",	do_lock,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "открыть",	do_open_,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "взломать",	do_pick,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "налить",		do_pour,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "перелить",	do_pour,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "положить",	do_put,		POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "класть",		do_put,		POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "поместить",	do_put,		POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "засунуть",	do_put,		POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "глотать",	do_quaff,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "зачитать",	do_recite,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "снять",		do_remove,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "продать",	do_sell,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "взять",		do_get,		POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "жертвовать",	do_sacrifice,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
/*  { "unlock",		do_unlock,	POS_RESTING,	 0,  LOG_NORMAL, 1 }, */
    { "цена",		do_value,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "одеть",		do_wear,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "надеть",		do_wear,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "вооружиться",	do_wield,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "тереть",		do_zap,		POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "приглядеться",	do_detect_hidden, POS_STANDING,	 0,  LOG_NORMAL, 1 },

    //команды для приглашения чара домой / отказа от приглашени
    { "пригласить",	do_invite,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "отказаться",	do_refuse,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    //команды постройки дома
    { "построить",		do_build_room,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "заселить",		do_build_mob,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "мебелировать",		do_build_obj, 	POS_RESTING,	 0,  LOG_NORMAL, 1 },

    { "заточить",	do_sharpen,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "наточить",	do_sharpen,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "заказать",	do_bounty,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "легенды",	do_lore,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "активировать",	do_activate,	POS_FIGHTING,	 0,  LOG_NORMAL, 1 },
    { "лагерь",		do_camp,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "травы",		do_herbs,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "выследить",	do_track,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "камуфляж",	do_camouflage,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "внимание",	do_attention,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "засада",		do_ambush,	POS_FIGHTING,	 0,  LOG_NORMAL, 1 },
    { "разделать",	do_butcher,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "родник",		do_find_spring,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "капкан",		do_trap,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "поставить",	do_trap,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "факел",		do_torch,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "чинить",		do_repair,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "отточить",	do_csharpen,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "лететь",		do_levitate,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "взвесить",	do_weigh,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "перевязать",	do_bandage,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "дарить",		do_gift,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "подарить",	do_gift,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "вырезать",	do_make_bag,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "гнездо",		do_socketing,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "персонализировать",do_renaming,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "привлечь",	do_attract_attention,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "ритуал",		do_blood_ritual, POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "концентрация", 	do_concentrate, POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "помолиться", 	do_prayer, POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "гримуар",	do_grimuar,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "сбор",		do_collection,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    { "смешать",	do_mix,		POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "столочь",	do_pound,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "дух",		do_spirit,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    { "обличие",	do_form,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    /*
     * Combat commands.
     */
    { "backstab",	do_backstab,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    { "bash",		do_bash,	POS_FIGHTING,    0,  LOG_NORMAL, 0 },
    { "bs",		do_backstab,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    { "berserk",	do_berserk,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    { "dirt",		do_dirt,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    { "disarm",		do_disarm,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    { "flee",		do_flee,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    { "kick",		do_kick,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
/*    { "murde",		do_murde,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    { "murder",		do_murder,	POS_FIGHTING,	 0,  LOG_ALWAYS, 0 }, */
    { "rescue",		do_rescue,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
/*    { "surrender",	do_surrender,	POS_FIGHTING,    0,  LOG_NORMAL, 0 }, */
    { "trip",		do_trip,	POS_FIGHTING,    0,  LOG_NORMAL, 0 },
    { "tail",	        do_tail,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    { "style",	        do_style,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    { "lunge",		do_lunge,	POS_FIGHTING,    0,  LOG_NORMAL, 0 },
    { "deafen",		do_deafen,	POS_FIGHTING,    0,  LOG_NORMAL, 0 },
    { "select",		do_select,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    { "hoof",		do_hoof,	POS_FIGHTING,    0,  LOG_NORMAL, 0 },


    { "вспину",		do_backstab,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    { "пырнуть",	do_backstab,	POS_FIGHTING,	 0,  LOG_NORMAL, 1 },
    { "толчок",		do_bash,	POS_FIGHTING,    0,  LOG_NORMAL, 1 },
    { "берсерк",	do_berserk,	POS_FIGHTING,	 0,  LOG_NORMAL, 1 },
    { "грязь",		do_dirt,	POS_FIGHTING,	 0,  LOG_NORMAL, 1 },
    { "обезоружить",	do_disarm,	POS_FIGHTING,	 0,  LOG_NORMAL, 1 },
    { "бежать",		do_flee,	POS_FIGHTING,	 0,  LOG_NORMAL, 1 },
    { "убежать",	do_flee,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    { "убегать",	do_flee,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    { "пинок",		do_kick,	POS_FIGHTING,	 0,  LOG_NORMAL, 1 },
/*    { "murde",	do_murde,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 }, */
/*    { "murder",	do_murder,	POS_FIGHTING,	 5,  LOG_ALWAYS, 1 }, */
    { "спасти",		do_rescue,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
/*    { "surrender",	do_surrender,	POS_FIGHTING,    0,  LOG_NORMAL, 1 }, */
    { "подножка",	do_trip,	POS_FIGHTING,    0,  LOG_NORMAL, 1 },
    { "подсечь",	do_trip,	POS_FIGHTING,    0,  LOG_NORMAL, 0 },
    { "выпад",		do_lunge,	POS_FIGHTING,    0,  LOG_NORMAL, 1 },
    { "оглушить",	do_deafen,	POS_FIGHTING,    0,  LOG_NORMAL, 1 },
    { "рассечь",	do_cleave,	POS_STANDING,    0,  LOG_NORMAL, 1 },
    { "хвост",	        do_tail,	POS_FIGHTING,	 0,  LOG_NORMAL, 1 },
    { "отбить",		do_fight_off,	POS_FIGHTING,    0,  LOG_NORMAL, 1 },
    { "стиль",	        do_style,	POS_FIGHTING,	 0,  LOG_NORMAL, 1 },
    { "выбрать",	do_select,	POS_FIGHTING,	 0,  LOG_NORMAL, 1 },
    { "копытом",	do_hoof,	POS_FIGHTING,    0,  LOG_NORMAL, 1 },
    { "проткнуть",	do_pierce,	POS_FIGHTING,    0,  LOG_NORMAL, 1 },

    { "прыжок",		do_jump,	POS_STANDING,    0,  LOG_NORMAL, 1 },
    { "вой",		do_howl,	POS_FIGHTING,    0,  LOG_NORMAL, 1 },
    { "рык",		do_growl,	POS_FIGHTING,    0,  LOG_NORMAL, 1 },
    { "рычание",	do_growl,	POS_FIGHTING,    0,  LOG_NORMAL, 1 },
    { "охота",		do_find_victim,	POS_STANDING,    0,  LOG_NORMAL, 1 },
    { "когти",		do_claws, 	POS_FIGHTING,    0,  LOG_NORMAL, 1 },
    { "лапа",		do_clutch_lycanthrope, 	POS_FIGHTING,    0,  LOG_NORMAL, 1 },
    { "мягкость",	do_silent_step,	POS_STANDING,    0,  LOG_NORMAL, 1 },

    /*
     * Mob command interpreter (placed here for faster scan...)
     */
    { "mob",		do_mob,		POS_DEAD,	 0,  LOG_NEVER,  0 },

    { "disarmtrap",	do_disarm_trap,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "обезвредить",	do_disarm_trap, POS_STANDING,    0,  LOG_NORMAL, 1 },

    /*
     * Miscellaneous commands.
     */
    { "enter", 		do_enter, 	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "follow",		do_follow,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "gain",		do_gain,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "addbuy",		do_buy_add,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "go",			do_enter,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
/*  { "group",		do_group,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 }, */
    { "groups",		do_groups,	POS_SLEEPING,    0,  LOG_NORMAL, 0 },
    { "hide",		do_hide,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "play",		do_play,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
/*  { "practice",	do_practice,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 }, */
    { "qui",		do_qui,		POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "quit",		do_quit,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "recall",		do_recall,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    { "/",		do_recall,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    { "rent",		do_rent,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "save",		do_save,	POS_DEAD,	 IM, LOG_NORMAL, 0 },
    { "sleep",		do_sleep,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 },
    { "sneak",		do_sneak,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "split",		do_split,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "steal",		do_steal,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "train",		do_train,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "visible",	do_visible,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "unfly",		do_unfly,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "wake",		do_wake,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 },
/*    { "where",		do_where,	POS_RESTING,	 0,  LOG_NORMAL, 0 }, */
    { "ident",    	do_ident,	POS_DEAD,       ML,  LOG_ALWAYS, 1 },

    { "identify",	do_identify,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "travel",         do_travel,      POS_STANDING,    0,  LOG_NORMAL, 0 },
    { "quest",		do_quest,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 },
    { "detecthidden",	do_detect_hidden, POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "mount",		do_mount,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },
    { "dismount",	do_dismount,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 },

    { "войти", 		do_enter, 	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "идти", 		do_enter, 	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "следовать",	do_follow,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "приобрести",	do_gain,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "приобретение",	do_gain,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "докупить",	do_buy_add,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
/*  { "go",		do_enter,	POS_STANDING,	 0,  LOG_NORMAL, 0 }, */
/*  { "group",		do_group,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 }, */
/*  { "groups",		do_groups,	POS_SLEEPING,    0,  LOG_NORMAL, 1 }, */
    { "скрываться",	do_hide,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "скрыться",	do_hide,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "играть",		do_play,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
/*  { "practice",	do_practice,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 }, */
    { "коне",		do_qui,		POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "конец",		do_quit,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "выйти",		do_quit,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "возврат",	do_recall,	POS_FIGHTING,	 0,  LOG_NORMAL, 1 },
/*  { "/",		do_recall,	POS_FIGHTING,	 0,  LOG_NORMAL, 0 }, */
    { "рента",		do_rent,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "сохранить",	do_save,	POS_DEAD,	 IM, LOG_NORMAL, 1 },
    { "записать",	do_save,	POS_DEAD,	 IM, LOG_NORMAL, 0 },
    { "красться",	do_sneak,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "подкрасться",	do_sneak,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "подкрадывание",	do_sneak,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "разделить",	do_split,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "поделить",	do_split,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "украсть",	do_steal,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "укрепить",	do_reinforce,	POS_STANDING,	 0,  LOG_NORMAL, 1 },
    { "тренироваться",	do_train,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "видимый",	do_visible,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "видимость",	do_visible,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "нелетать",	do_unfly,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "проснуться",	do_wake,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 },
    { "разбудить",	do_wake,	POS_SLEEPING,	 0,  LOG_NORMAL, 1 },
/*    { "где",		do_where,	POS_RESTING,	 0,  LOG_NORMAL, 1 }, */
    { "опознать",	do_identify,	POS_RESTING,	 0,  LOG_NORMAL, 1 },
    { "путешествие",    do_travel,      POS_STANDING,    0,  LOG_NORMAL, 1 },
    { "квест",		do_quest,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 },
    { "оседлать",	do_mount,	POS_FIGHTING,	 0,  LOG_NORMAL, 1 },
    { "спешиться",	do_dismount,	POS_FIGHTING,	 0,  LOG_NORMAL, 1 },

    { "bite",		do_bite,	POS_STANDING,	 5,  LOG_NORMAL, 0 },
    { "укусить",	do_bite,	POS_STANDING,	 5,  LOG_NORMAL, 1 },
    { "насытиться",	do_bite,	POS_STANDING,	 5,  LOG_NORMAL, 1 },

    /*
     * Immortal commands.
     */
    { "advance",	do_advance,	POS_DEAD,	ML,  LOG_ALWAYS, 1 },
    { "dump",		do_dump,	POS_DEAD,	ML,  LOG_ALWAYS, 0 },
    { "trust",		do_trust,	POS_DEAD,	ML,  LOG_ALWAYS, 1 },
    { "violate",	do_violate,	POS_DEAD,	ML,  LOG_ALWAYS, 1 },

    { "allow",		do_allow,	POS_DEAD,	L2,  LOG_ALWAYS, 1 },
    { "ban",		do_ban,		POS_DEAD,	L2,  LOG_ALWAYS, 1 },
	{ "cemail",		do_cemail,	POS_DEAD,	L1,  LOG_ALWAYS, 1 },
    { "deny",		do_deny,	POS_DEAD,	L1,  LOG_ALWAYS, 1 },
    { "disconnect",	do_disconnect,	POS_DEAD,	L3,  LOG_ALWAYS, 1 },
    { "flag",		do_flag,	POS_DEAD,	L4,  LOG_ALWAYS, 1 },
    { "freeze",		do_freeze,	POS_DEAD,	L4,  LOG_ALWAYS, 1 },
    { "house",		do_house,	POS_DEAD,	L4,  LOG_ALWAYS, 1 },
    { "popularity",	do_popularity,	POS_DEAD,	ML,  LOG_NORMAL, 1 },
    { "permban",	do_permban,	POS_DEAD,	L1,  LOG_ALWAYS, 1 },
    { "protect",	do_protect,	POS_DEAD,	L1,  LOG_ALWAYS, 1 },
    { "reboo",		do_reboo,	POS_DEAD,	L1,  LOG_NORMAL, 0 },
    { "reboot",		do_reboot,	POS_DEAD,	L1,  LOG_ALWAYS, 1 },
    { "set",		do_set,		POS_DEAD,	L4,  LOG_ALWAYS, 1 },
    { "shutdow",	do_shutdow,	POS_DEAD,	L1,  LOG_NORMAL, 0 },
    { "shutdown",	do_shutdown,	POS_DEAD,	L1,  LOG_ALWAYS, 1 },
/*  { "sockets",	do_sockets,	POS_DEAD,	L4,  LOG_NORMAL, 1 }, */
    { "wizlock",	do_wizlock,	POS_DEAD,	L2,  LOG_ALWAYS, 1 },

    { "force",		do_force,	POS_DEAD,	L7,  LOG_ALWAYS, 1 },
    { "load",		do_load,	POS_DEAD,	IM,  LOG_ALWAYS, 1 },
    { "pload",		do_pload,	POS_DEAD,	L4,  LOG_ALWAYS, 1 },
    { "punload",	do_punload,	POS_DEAD,	L4,  LOG_ALWAYS, 1 },
    { "newlock",	do_newlock,	POS_DEAD,	L4,  LOG_ALWAYS, 1 },
    { "nochannels",	do_nochannels,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },
    { "extranochannels",	do_extranochannels,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },
    { "noemote",	do_noemote,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },
    { "noshout",	do_noshout,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },
    { "notell",		do_notell,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },
    { "nonotes",	do_nonotes,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },
    { "notitle",	do_notitle,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },
    { "pecho",		do_pecho,	POS_DEAD,	L4,  LOG_ALWAYS, 1 },
    { "pardon",		do_pardon,	POS_DEAD,	L3,  LOG_ALWAYS, 1 },
    { "purge",		do_purge,	POS_DEAD,	L4,  LOG_ALWAYS, 1 },
    { "restore",	do_restore,	POS_DEAD,	L4,  LOG_ALWAYS, 1 },
    { "sla",		do_sla,		POS_DEAD,	L3,  LOG_NORMAL, 0 },
    { "slay",		do_slay,	POS_DEAD,	L3,  LOG_ALWAYS, 1 },
    { "teleport",	do_transfer,    POS_DEAD,	IM,  LOG_ALWAYS, 1 },
    { "transfer",	do_transfer,	POS_DEAD,	IM,  LOG_ALWAYS, 1 },

    { "at",			do_at,		POS_DEAD,	L6,  LOG_NORMAL, 1 },
    { "absolutely",	do_absolutely, 	POS_DEAD,	ML,  LOG_NORMAL, 1 },
    { "choice",		do_choice, 	POS_DEAD,	L8,  LOG_NORMAL, 1 },
    { "poofin",		do_bamfin,	POS_DEAD,	L8,  LOG_NORMAL, 1 },
    { "poofout",	do_bamfout,	POS_DEAD,	L8,  LOG_NORMAL, 1 },
    { "gecho",		do_echo,	POS_DEAD,	IM,  LOG_ALWAYS, 1 },
/*  { "goto",		do_goto,	POS_DEAD,	L8,  LOG_NORMAL, 1 }, */
    { "holylight",	do_holylight,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "incognito",	do_incognito,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "invis",		do_invis,	POS_DEAD,	IM,  LOG_NORMAL, 0 },
    { "log",		do_log,		POS_DEAD,	L1,  LOG_ALWAYS, 1 },
    { "memory",		do_memory,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "mwhere",		do_mwhere,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "owhere",		do_owhere,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "rwhere",		do_rwhere,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "peace",		do_peace,	POS_DEAD,	L5,  LOG_NORMAL, 1 },
    { "penalty",	do_penalty,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "echo",		do_recho,	POS_DEAD,	L6,  LOG_ALWAYS, 1 },
    { "return",         do_return,      POS_DEAD,       L7,  LOG_NORMAL, 1 },
    { "snoop",		do_snoop,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },
    { "stat",		do_stat,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "string",		do_string,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },
    { "switch",		do_switch,	POS_DEAD,	L7,  LOG_ALWAYS, 1 },
    { "wizinvis",	do_invis,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "vnum",		do_vnum,	POS_DEAD,	L7,  LOG_NORMAL, 1 },
    { "zecho",		do_zecho,	POS_DEAD,	L4,  LOG_ALWAYS, 1 },

    { "clone",		do_clone,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },

    { "wiznet",		do_wiznet,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "immtalk",	do_immtalk,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "imotd",         do_imotd,       POS_DEAD,       IM,  LOG_NORMAL, 1 },
    { ":",		do_immtalk,	POS_DEAD,	IM,  LOG_NORMAL, 0 },
    { "btalk",		do_btalk,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
	{ "ttalk",		do_ttalk,	POS_DEAD,	IM,	 LOG_NORMAL,	1 },
    { "\"",		do_btalk,	POS_DEAD,	IM,  LOG_NORMAL, 0 },
    { "iptalk",		do_iptalk,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
/*    { "smote",		do_smote,	POS_DEAD,	IM,  LOG_NORMAL, 1 }, */
    { "prefi",		do_prefi,	POS_DEAD,	IM,  LOG_NORMAL, 0 },
    { "prefix",		do_prefix,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "mpdump",		do_mpdump,	POS_DEAD,	IM,  LOG_NEVER,  1 },
    { "mpstat",		do_mpstat,	POS_DEAD,	IM,  LOG_NEVER,  1 },
    { "rename",		do_rename,	POS_DEAD,	IM,  LOG_ALWAYS, 1 },
    { "nopk",		do_nopk,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },
    { "success",	do_success,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },
    { "fullsilence",	do_full_silence,POS_DEAD,	L1,  LOG_NORMAL, 1 },
    { "addapply",	do_addapply,	POS_DEAD,	IM,  LOG_ALWAYS, 1 },
/*    { "limits",		do_limits,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
 */
    { "immquest",	do_immquest,	POS_DEAD,	IM,  LOG_ALWAYS, 0 },
    { "squest",		do_charquest,	POS_DEAD,	0,   LOG_ALWAYS, 0 },
    { "immnotepad",	do_immnotepad,	POS_DEAD,	IM,  LOG_ALWAYS, 1 },
    { "setalign",	do_setalign,	POS_DEAD,	IM,  LOG_ALWAYS, 0 },
    { "olevel",		do_olevel,	POS_DEAD,	IM,  LOG_ALWAYS, 0 },
    { "mlevel",		do_mlevel,	POS_DEAD,	IM,  LOG_ALWAYS, 0 },
    { "otype",		do_otype,	POS_DEAD,	IM,  LOG_ALWAYS, 0 },
    { "immtitle",	do_immtitle,	POS_DEAD,	IM,  LOG_ALWAYS, 0 },
    { "immpass",	do_pass,	POS_DEAD,	L4,  LOG_NORMAL, 0 },
    { "immskills",	do_immskills,	POS_DEAD,	IM,  LOG_NORMAL, 0 },
    { "immspells",	do_immspells,	POS_DEAD,	IM,  LOG_NORMAL, 0 },
    { "immprompt",	do_immprompt,	POS_DEAD,	IM,  LOG_NORMAL, 0 },
    { "immcompare",	do_immcompare,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "excempt",	do_excempt,	POS_DEAD,	IM,  LOG_ALWAYS, 1 },
    { "arealinks",  	do_arealinks,   POS_DEAD,       ML,  LOG_NORMAL, 1 },
    { "races",  	do_races,       POS_DEAD,       0,   LOG_NORMAL, 0 },
    { "check",  	do_check,       POS_DEAD,       L7,  LOG_NORMAL, 1 },
    { "doas",  		do_doas,        POS_DEAD,       L1,  LOG_NORMAL, 0 },
    { "security",	do_security,    POS_DEAD,       ML,  LOG_NORMAL, 0 },
    { "whowas",	        do_whowas,   	POS_DEAD,       L2,  LOG_NORMAL, 1 },
    { "host",	        do_host,   	POS_DEAD,       L3,  LOG_NORMAL, 0 },
    { "fileident",      do_fileident,  	POS_DEAD,       ML,  LOG_NORMAL, 1 },
    { "loadquester",    do_loadquester,	POS_DEAD,       IM,  LOG_NORMAL, 0 },
    { "noreply",    	do_noreply,	POS_DEAD,       IM,  LOG_NORMAL, 1 },
    { "immdamage",    	do_immdamage,	POS_DEAD,       ML,  LOG_NORMAL, 1 },
    { "tick",    	do_tick,	POS_DEAD,       ML,  LOG_NORMAL, 0 },
    { "nocast",    	do_nocast,	POS_DEAD,       IM,  LOG_NORMAL, 1 },
    { "godscurse",    	do_gods_curse,	POS_DEAD,       IM,  LOG_NORMAL, 1 },
    { "skillaff",    	do_skillaffect,	POS_DEAD,       IM,  LOG_NORMAL, 1 },
    { "last",    	do_last_com,	POS_DEAD,       IM,  LOG_NORMAL, 1 },
    { "grant",    	do_grant,	POS_DEAD,       ML,  LOG_ALWAYS, 1 },
    { "gdb",    	do_gdb,		POS_DEAD,       ML,  LOG_ALWAYS, 1 },
    { "sysident",    	do_ident,	POS_DEAD,       ML,  LOG_ALWAYS, 1 },
    { "players",    	do_players,	POS_DEAD,       ML,  LOG_ALWAYS, 1 },
    { "emailpass",	do_email_pass,	POS_DEAD,	IM,  LOG_ALWAYS, 1 },
    { "whoislist",	do_whoislist,	POS_DEAD,	IM,  LOG_NEVER, 1 },
    { "attack",		do_autoattack,	POS_STANDING,	 0,  LOG_NORMAL, 0 }, 

    { "абсолютный",	do_absolutely,	POS_DEAD,	ML,  LOG_NORMAL, 1 },
    { "выбор",		do_choice, 	POS_DEAD,	L8,  LOG_NORMAL, 1 },
    { "левел",		do_advance,	POS_DEAD,	ML,  LOG_ALWAYS, 1 },
    { "дамп",		do_dump,	POS_DEAD,	ML,  LOG_ALWAYS, 0 },
    { "доверить",	do_trust,	POS_DEAD,	ML,  LOG_ALWAYS, 1 },
    { "вприватную",	do_violate,	POS_DEAD,	ML,  LOG_ALWAYS, 1 },

    { "позволить",	do_allow,	POS_DEAD,	L2,  LOG_ALWAYS, 1 },
    { "бан",		do_ban,		POS_DEAD,	L2,  LOG_ALWAYS, 1 },
    { "запретить",	do_deny,	POS_DEAD,	L1,  LOG_ALWAYS, 1 },
	{ "сменпочту",	do_cemail,	POS_DEAD,	L1,  LOG_ALWAYS, 1 },
    { "отключить",	do_disconnect,	POS_DEAD,	L3,  LOG_ALWAYS, 1 },
    { "флаг",		do_flag,	POS_DEAD,	L4,  LOG_ALWAYS, 1 },
    { "заморозить",	do_freeze,	POS_DEAD,	L4,  LOG_ALWAYS, 1 },
    { "дом",		do_house,	POS_DEAD,	L4,  LOG_ALWAYS, 1 },
    { "популярность",	do_popularity,	POS_DEAD,	ML,  LOG_NORMAL, 1 },
    { "пермбан",	do_permban,	POS_DEAD,	L1,  LOG_ALWAYS, 1 },
    { "протект",	do_protect,	POS_DEAD,	L1,  LOG_ALWAYS, 1 },
    { "ребу",		do_reboo,	POS_DEAD,	L1,  LOG_NORMAL, 0 },
    { "ребут",		do_reboot,	POS_DEAD,	L1,  LOG_ALWAYS, 1 },
    { "установить",	do_set,		POS_DEAD,	L2,  LOG_ALWAYS, 1 },
    { "выключит",	do_shutdow,	POS_DEAD,	L1,  LOG_NORMAL, 0 },
    { "выключить",	do_shutdown,	POS_DEAD,	L1,  LOG_ALWAYS, 1 },
    { "сокеты",		do_sockets,	POS_DEAD,	L3,  LOG_NORMAL, 1 },
    { "визлок",		do_wizlock,	POS_DEAD,	L2,  LOG_ALWAYS, 1 },

    { "заставить",	do_force,	POS_DEAD,	L7,  LOG_ALWAYS, 1 },
    { "принудить",	do_force,	POS_DEAD,	L7,  LOG_ALWAYS, 1 },
    { "загрузить",	do_load,	POS_DEAD,	IM,  LOG_ALWAYS, 1 },

    { "ньюлок",		do_newlock,	POS_DEAD,	L4,  LOG_ALWAYS, 1 },
    { "нетканалов",	do_nochannels,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },
    { "экстранетканалов",	do_extranochannels,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },
    { "нетэмоций",	do_noemote,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },
    { "неторать",	do_noshout,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },
    { "нетпривата",	do_notell,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },
    { "нетписем",	do_nonotes,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },
    { "неттитул",	do_notitle,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },
    { "неубить",	do_nopk,	POS_DEAD,   L5,  LOG_ALWAYS, 1 },
    { "одобрить",	do_success,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },
    { "молчание",	do_full_silence,	POS_DEAD,   L1,  LOG_NORMAL, 1 },
    { "пэхо",		do_pecho,	POS_DEAD,	L4,  LOG_ALWAYS, 1 },

    { "простить",	do_pardon,	POS_DEAD,	L3,  LOG_ALWAYS, 1 },
    { "очистить",	do_purge,	POS_DEAD,	L4,  LOG_ALWAYS, 1 },
    { "восстановить",	do_restore,	POS_DEAD,	L4,  LOG_ALWAYS, 1 },
    { "уничтожит",	do_sla,		POS_DEAD,	L3,  LOG_NORMAL, 0 },
    { "уничтожить",	do_slay,	POS_DEAD,	L3,  LOG_ALWAYS, 1 },
    { "переместить",	do_transfer,    POS_DEAD,	IM,  LOG_ALWAYS, 1 },

    { "появление",	do_bamfin,	POS_DEAD,	L8,  LOG_NORMAL, 1 },
    { "исчезание",	do_bamfout,	POS_DEAD,	L8,  LOG_NORMAL, 1 },
    { "гэхо",		do_echo,	POS_DEAD,	IM,  LOG_ALWAYS, 1 },
    { "свет",		do_holylight,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "инкогнито",	do_incognito,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "инвиз",		do_invis,	POS_DEAD,	IM,  LOG_NORMAL, 0 },
    { "лог",		do_log,		POS_DEAD,	L1,  LOG_ALWAYS, 1 },
    { "память",		do_memory,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "гдемоб",		do_mwhere,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "гдеобъект",	do_owhere,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "гдересет",	do_rwhere,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "мир",		do_peace,	POS_DEAD,	L5,  LOG_NORMAL, 1 },
    { "наказания",	do_penalty,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "эхо",		do_recho,	POS_DEAD,	L6,  LOG_ALWAYS, 1 },
    { "вернуться",      do_return,      POS_DEAD,       L7,  LOG_NORMAL, 1 },
    { "подглядеть",	do_snoop,	POS_DEAD,	L5,  LOG_ALWAYS, 0 },
    { "подсмотреть",	do_snoop,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },
    { "статистика",	do_stat,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "строка",		do_string,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },
    { "переключиться",	do_switch,	POS_DEAD,	L7,  LOG_ALWAYS, 1 },
    { "внум",		do_vnum,	POS_DEAD,	L7,  LOG_NORMAL, 1 },
    { "зэхо",		do_zecho,	POS_DEAD,	L4,  LOG_ALWAYS, 1 },
    { "переименовать",	do_rename,	POS_DEAD,	IM,  LOG_ALWAYS, 1 },

    { "клонировать",	do_clone,	POS_DEAD,	L5,  LOG_ALWAYS, 1 },

    { "визнет",		do_wiznet,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "иммразговор",	do_immtalk,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "бразговор",	do_btalk,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "тразговор",	do_ttalk,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "иммкг",  	do_immclantalk,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "immclantalk",  	do_immclantalk,	POS_DEAD,	IM,  LOG_NORMAL, 1 },

    { "иммрг",  	do_immracetalk,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "immracetalk",  	do_immracetalk,	POS_DEAD,	IM,  LOG_NORMAL, 1 },

    { "адресу",		do_iptalk,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "попочте",	do_email_pass,	POS_DEAD,	IM,  LOG_ALWAYS, 1 },

    { "имотд",          do_imotd,       POS_DEAD,       IM,  LOG_NORMAL, 1 },
/*    { "смоте",		do_smote,	POS_DEAD,	IM,  LOG_NORMAL, 1 }, */
    { "префи",		do_prefi,	POS_DEAD,	IM,  LOG_NORMAL, 0 },
    { "префикс",	do_prefix,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "мпдамп",		do_mpdump,	POS_DEAD,	IM,  LOG_NEVER,  1 },
    { "мпстат",		do_mpstat,	POS_DEAD,	IM,  LOG_NEVER,  1 },
#if defined(ONEUSER)
    { "падеж",		do_cases,	POS_DEAD,	 1,  LOG_NEVER, 1 },
    { "вбоги",		do_promote,	POS_DEAD,	 1,  LOG_NEVER, 1 },
#else
    { "падеж",		do_cases,	POS_DEAD,	IM,  LOG_NEVER, 1 },
#endif

    { "иммзагрузить",	do_pload,	POS_DEAD,	L4,  LOG_ALWAYS, 1 },
    { "иммвыгрузить",	do_punload,	POS_DEAD,	L4,  LOG_ALWAYS, 1 },
    { "иммсравнить",	do_immcompare,	POS_DEAD,	IM,  LOG_NORMAL, 1 },

    { "порт",		do_port,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
/*    { "лимиты",		do_limits,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
*/
    { "добавитьэфф",	do_addapply,	POS_DEAD,	IM,  LOG_ALWAYS, 1 },
    { "омир",		do_wpeace,	POS_DEAD,	L4,  LOG_ALWAYS, 1 },
    { "путь",		do_shortestpath, POS_DEAD,	L4,  LOG_ALWAYS, 1 },
    { "иммквест",	do_immquest,	POS_DEAD,	IM,  LOG_ALWAYS, 1 },
    { "сквест",		do_charquest,	POS_DEAD,	0,   LOG_ALWAYS, 0 },
    { "натура",		do_setalign,	POS_DEAD,	IM,  LOG_ALWAYS, 1 },
    { "олевел",		do_olevel,	POS_DEAD,	IM,  LOG_ALWAYS, 1 },
    { "млевел",		do_mlevel,	POS_DEAD,	IM,  LOG_ALWAYS, 1 },
    { "отип",		do_otype,	POS_DEAD,	IM,  LOG_ALWAYS, 1 },
    { "иммтитул",	do_immtitle,	POS_DEAD,	IM,  LOG_ALWAYS, 1 },
    { "иммпароль",	do_pass,	POS_DEAD,	L4,  LOG_NORMAL, 1 },
    { "иммумения",	do_immskills,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "иммзаклинания",	do_immspells,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "иммпромпт",	do_immprompt,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    { "изъять",		do_excempt,	POS_DEAD,	IM,  LOG_ALWAYS, 1 },
    { "расы",	  	do_races,       POS_DEAD,       0,   LOG_NORMAL, 1 },
    { "сделатьза",	do_doas,        POS_DEAD,       L1,  LOG_NORMAL, 1 },
    { "секретность",	do_security,    POS_DEAD,       ML,  LOG_NORMAL, 1 },
    { "хост",	        do_host,   	POS_DEAD,       L3,  LOG_NORMAL, 1 },
    { "квестер",        do_loadquester,	POS_DEAD,       IM,  LOG_NORMAL, 1 },
    { "тик",    	do_tick,	POS_DEAD,       ML,  LOG_NORMAL, 1 },
    { "проклясть",    	do_gods_curse,	POS_DEAD,       IM,  LOG_NORMAL, 1 },
    { "ловушки",    	do_deathtraps,	POS_DEAD,       L5,  LOG_NORMAL, 1 },
    { "иммзаписи",	do_immnotepad,	POS_DEAD,	IM,  LOG_ALWAYS, 1 },
    /* 2018 */
    { "immaffects",	do_immaffects,	POS_DEAD,	IM,  LOG_NORMAL, 0 },
    { "иммэффекты",	do_immaffects,	POS_DEAD,	IM,  LOG_NORMAL, 1 },
    
    
    /*
    Bank
    */
    { "balance",	do_balance,	POS_SLEEPING,    0,  LOG_NORMAL, 0 },
    { "deposit",	do_deposit,	POS_STANDING,    0,  LOG_NORMAL, 0 },
    { "withdraw",	do_withdraw,	POS_STANDING,    0,  LOG_NORMAL, 0 },
    { "баланс",		do_balance,	POS_SLEEPING,    0,  LOG_NORMAL, 1 },
    { "насчет",		do_deposit,	POS_STANDING,    0,  LOG_NORMAL, 1 },
    { "сосчета",	do_withdraw,	POS_STANDING,    0,  LOG_NORMAL, 1 },



    /*
     * OLC
     */
    { "edit",		do_olc,		POS_DEAD,    IM,  LOG_NORMAL, 1 },
    { "aedit",		do_aedit,	POS_DEAD,    IM,  LOG_NORMAL, 1 },
    { "redit",		do_redit,	POS_DEAD,    IM,  LOG_NORMAL, 1 },
    { "medit",		do_medit,	POS_DEAD,    IM,  LOG_NORMAL, 1 },
    { "oedit",		do_oedit,	POS_DEAD,    IM,  LOG_NORMAL, 1 },
    { "asave",          do_asave,	POS_DEAD,    IM,  LOG_NORMAL, 1 },
    { "alist",		do_alist,	POS_DEAD,    IM,  LOG_NORMAL, 1 },
    { "resets",		do_resets,	POS_DEAD,    IM,  LOG_NORMAL, 1 },
    { "mpedit",		do_mpedit,	POS_DEAD,    IM,  LOG_NORMAL, 1 },
    { "opedit",         do_opedit,      POS_DEAD,    IM,  LOG_ALWAYS, 1 },
    { "rpedit",         do_rpedit,      POS_DEAD,    IM,  LOG_ALWAYS, 1 },
    { "hedit",		do_hedit,	POS_DEAD,    IM,  LOG_ALWAYS, 1 },
    { "hsave",		do_hsave,	POS_DEAD,    IM,  LOG_ALWAYS, 1 },
    { "raedit",		do_raedit,	POS_DEAD,    L5,  LOG_ALWAYS, 1 },
    { "rasave",		do_rasave,	POS_DEAD,    L5,  LOG_ALWAYS, 1 },
    { "opdump",		do_opdump,	POS_DEAD,    IM,  LOG_NEVER,  1 },
    { "opstat",		do_opstat,	POS_DEAD,    IM,  LOG_NEVER,  1 },
    { "rpdump",		do_rpdump,	POS_DEAD,    IM,  LOG_NEVER,  1 },
    { "rpstat",		do_rpstat,	POS_DEAD,    IM,  LOG_NEVER,  1 },
    { "cedit",		do_cedit,	POS_DEAD,    IM,  LOG_NEVER,  1 },
    { "skedit",		do_skedit,	POS_DEAD,    IM,  LOG_ALWAYS, 1 },
    { "sksave",		do_sksave,	POS_DEAD,    IM,  LOG_ALWAYS, 1 },
    { "cledit",		do_cledit,	POS_DEAD,    IM,  LOG_ALWAYS, 1 },
    { "clsave",		do_clsave,	POS_DEAD,    IM,  LOG_ALWAYS, 1 },
    { "gredit",		do_gredit,	POS_DEAD,    IM,  LOG_ALWAYS, 1 },
    { "grsave",		do_grsave,	POS_DEAD,    IM,  LOG_ALWAYS, 1 },

    { "resetlist",	do_resetlist,	POS_DEAD,    IM,  LOG_NORMAL, 1 },

    { "attention",	do_attention,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "attract",	do_attract_attention,	POS_STANDING,	 0,  LOG_NORMAL, 0 },
    { "clan",		do_clan,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 },
    { "close",		do_close_,	POS_RESTING,	 0,  LOG_NORMAL, 0 },
    { "cleave",		do_cleave,	POS_FIGHTING,    0,  LOG_NORMAL, 0 },
    { "pierce",		do_pierce,	POS_FIGHTING,    0,  LOG_NORMAL, 0 },

    { "marry",	  	do_marry,	POS_DEAD,	IM,	LOG_ALWAYS, 0 },
    { "divorce",	do_divorce,	POS_DEAD,	IM,	LOG_ALWAYS, 0 },
    { "spousetalk", 	do_spousetalk, 	POS_SLEEPING,	1,	LOG_NORMAL, 0 },
    { "consent",	do_consent,	POS_RESTING,	1,	LOG_NORMAL, 0 },
    { "bandage",	do_bandage,	POS_RESTING,	0,      LOG_NORMAL, 0 },

    { "поженить",	do_marry,	POS_DEAD,	IM,	LOG_ALWAYS, 1 },
    { "развод",		do_divorce,	POS_DEAD,	IM,	LOG_ALWAYS, 1 },
    { "говсемье", 	do_spousetalk, 	POS_SLEEPING,	1,	LOG_NORMAL, 1 },
    { "согласие",	do_consent,	POS_RESTING,	1,	LOG_NORMAL, 1 },

    { "lookmap",	do_lookmap,	POS_STANDING,	1,	LOG_NORMAL, 0 },
    { "карта",		do_lookmap,	POS_STANDING,	1,	LOG_NORMAL, 1 },
 //   { "arena",  	do_arena,	POS_DEAD,	IM,	LOG_NORMAL, 1 },
 //  { "арена",  	do_arena,	POS_DEAD,	IM,	LOG_NORMAL, 1 },
    { "recipe",  	do_recipe,	POS_DEAD,	L5,	LOG_NORMAL, 1 },
    { "рецепт",  	do_recipe,	POS_DEAD,	L5,	LOG_NORMAL, 1 },

    { "slot",  		do_slot,	POS_SITTING,	 0,	LOG_NORMAL, 0 },
    { "слот",  		do_slot,	POS_SITTING,	 0,	LOG_NORMAL, 1 },
    { "поскакать",	do_gallop,	POS_STANDING,	 0,  	LOG_NORMAL, 1 },

    { "dig",		do_dig,		POS_STANDING,	 ML,  	LOG_NORMAL, 0 },
    { "sayspells",	do_sayspells,	POS_STANDING,	 IM,  	LOG_NORMAL, 0 },
    { "setip",		do_setip,	POS_DEAD,	 0,  	LOG_NORMAL, 0 },
    { "регвопрос",	do_reg_answer,	POS_SLEEPING,	 0,  LOG_NORMAL, 0 },
    { "map",		do_map,		POS_DEAD,	 IM,  	LOG_NORMAL, 0 },
    { "bug",		do_bug,		POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "todo",		do_todo,	POS_DEAD,	 IM,  LOG_NORMAL, 0 },
    { "глюк",		do_bug,		POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "туду",		do_todo,	POS_DEAD,	 IM,  LOG_NORMAL, 0 },    
	{ "ошибка",		do_bug,		POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "баг",		do_bug,		POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "typo",		do_typo,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "опечатка",	do_typo,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
    { "offense",	do_offense,	POS_DEAD,	 0,  LOG_NORMAL, 0 },
    { "нарушение",	do_offense,	POS_DEAD,	 0,  LOG_NORMAL, 1 },
     /*
     * End of list.
     */
    { "",		0,		POS_DEAD,	 0,  LOG_NORMAL, 0 }
};

#ifdef MUD_MEM_DEBUG
int string_count;
int perm_count;
char cmd_copy[MAX_INPUT_LENGTH];
char buf[MAX_STRING_LENGTH];
#endif

char *is_granted(CHAR_DATA *ch, char *command)
{
    int cmd, i;
    bool found = FALSE;

    if (ch->desc != NULL && ch->desc->original != NULL)
	ch = ch->desc->original;

    for (i = 0; cmd_table[i].name[0] != '\0'; i++)
	if (!str_cmp(cmd_table[i].name, command))
	{
	    found = TRUE;
	    break;
	}

    if (!found)
	return NULL;

    for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++)
	if (cmd_table[cmd].do_fun == cmd_table[i].do_fun
	    && is_exact_name(cmd_table[cmd].name, ch->pcdata->grants))
	{
	    return cmd_table[cmd].name;
	}

    return NULL;
}

bool is_immcmd(char *command)
{
    int cmd;

    for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++)
	if (!str_cmp(cmd_table[cmd].name, command)
	    && cmd_table[cmd].level > LEVEL_HERO)
	{
	    return TRUE;
	}

    return FALSE;
}

void grant_all(CHAR_DATA *ch)
{
    int cmd;
    char grants[4*MAX_STRING_LENGTH];

    grants[0] = '\0';
    free_string(ch->pcdata->grants);

    for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++)
	if (cmd_table[cmd].level > LEVEL_HERO
	    && !is_granted(ch, cmd_table[cmd].name))
	{
	    if (grants[0] != '\0')
		strlcat(grants, " ", sizeof(grants));

	    strlcat(grants, cmd_table[cmd].name, sizeof(grants));
	    ch->pcdata->grants = str_dup(grants);
	}
}

void grant_level(CHAR_DATA *ch, int level)
{
    int cmd;
    char grants[4*MAX_STRING_LENGTH];

    grants[0] = '\0';
    free_string(ch->pcdata->grants);

    for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++)
	if (cmd_table[cmd].level > LEVEL_HERO
	    && cmd_table[cmd].level <= level
	    && !is_granted(ch, cmd_table[cmd].name))
	{
	    if (grants[0] != '\0')
		strcat(grants, " ");

	    strcat (grants, cmd_table[cmd].name);
	    ch->pcdata->grants = str_dup(grants);
	}

}


int cmd_lookup(CHAR_DATA *ch, char *command)
{
    int trust, cmd;
    CHAR_DATA *real;
    
    trust = get_trust(ch);
    if (ch->desc && ch->desc->original)
	real = ch->desc->original;
    else
	real = ch;
    for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++)
	if (command[0] == cmd_table[cmd].name[0]
    	    && !str_prefix(command, cmd_table[cmd].name)
    	    && (cmd_table[cmd].level <= UMIN(trust, LEVEL_HERO)
		|| (cmd_table[cmd].level > LEVEL_HERO
		    && !IS_NPC(real)
		    && (real->level == MAX_LEVEL || is_granted(real, cmd_table[cmd].name)))))
             return cmd;

    return -1;
}

/*
 * The main entry point for executing commands.
 * Can be recursively called from 'at', 'order', 'force'.
 */
void interpret(CHAR_DATA *ch, char *argument)
{
    char command[MAX_INPUT_LENGTH];
    char logline[MAX_INPUT_LENGTH];
    int cmd;
    bool found, prg = FALSE;

    if (!IS_VALID(ch))
    {
	if (!IS_NPC(ch))
	    VALIDATE(ch);
	else
	{
	    bugf("Invalidated character [%d/'%s']!", ch->pIndexData->vnum, ch->name);
	    return;
	}
    }

    for (max_commands = 0; cmd_table[max_commands].name[0] != '\0'; max_commands++)
        ;

    if (IS_NPC(ch) && ch->wait > 0)
       	return;

#ifdef MUD_MEM_DEBUG
    string_count = nAllocString;
    perm_count = nAllocPerm;
    strcpy(cmd_copy, argument) ;
#endif
    /*
     * Strip leading spaces.
     */

    while (IS_SPACE(*argument))
	argument++;

    if (argument[0] == '\0')
	return;

    /*
     * Implement freeze command.
     */
    if (!IS_NPC(ch)
	&& IS_SET(ch->act, PLR_FREEZE)
	&& str_cmp(argument, "quit")
	&& str_cmp(argument, "конец")
	&& str_cmp(argument, "выйти"))
    {
	send_to_char("Вы заморожены!\n\r", ch);
	return;
    }

    /*
     * Grab the command word.
     * Special parsing so ' can be a command,
     *   also no spaces needed after punctuation.
     */

    strcpy(logline, argument);

    /*Lets see who is doing what? -Ferric*/
    sprintf(last_command, "%s in room[%d]: '%s'.",
	    ch->name, ch->in_room != NULL ? ch->in_room->vnum : 0, argument);

    if (!IS_ALPHA(argument[0]) && !isdigit(argument[0]) )
    {

	command[0] = argument[0];
	command[1] = '\0';
	argument++;

	while (IS_SPACE(*argument))
	    argument++;
    }
    else
    {
	argument = one_argument(argument, command);
    }

    /*
     * Look for command in command table.
     */

    

    cmd = cmd_lookup(ch, command);
    
    found = cmd < 0 ? FALSE : TRUE;

/*    trust = get_trust(ch);
    if (ch->desc && ch->desc->original)
	real = ch->desc->original;
    else
	real = ch;
    for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++)
    {
	if (command[0] == cmd_table[cmd].name[0]
    	    && !str_prefix(command, cmd_table[cmd].name)
    	    && (cmd_table[cmd].level <= UMIN(trust, LEVEL_HERO)
		|| (cmd_table[cmd].level > LEVEL_HERO
		    && !IS_NPC(real)
		    && is_granted(real, cmd_table[cmd].name))))
	{
	    found = TRUE;
	    break;
	}
    } */

    /*
     * Log and snoop.
     */
    if (cmd_table[cmd].log == LOG_NEVER || IS_NPC(ch))
	strcpy(logline, "");

    if (logline[0] != '\0')
    {
	char bfr[MAX_STRING_LENGTH];

  	if (cfg.log_all || cmd_table[cmd].log == LOG_ALWAYS)
    	{
    	    sprintf(bfr, "Log %s: %s in room: [%d]", ch->name, logline,
    		    ch->in_room != NULL ? ch->in_room->vnum : 0);
    	    log_string(bfr);
    	    convert_dollars(bfr);
    	    wiznet(bfr, ch, NULL, WIZ_SECURE, 0, get_trust(ch));
      	}

#if !defined(ONEUSER)
	if (IS_SET(ch->act, PLR_LOG))
	{
    	    char logname[50], *tm;

	    tm = ctime(&current_time);
	    tm[strlen(tm) - 1] = '\0';

	    sprintf(bfr, "%s :: %s in room: [%d]\n", tm, logline,
		    ch->in_room != NULL ? ch->in_room->vnum : 0);
	    sprintf(logname, "%s%s", LOG_DIR, ch->name);
	    /*recode(logname, CODEPAGE_ALT, RECODE_OUTPUT|RECODE_NOANTITRIGGER);
*/
	    _log_string(bfr, logname);
	}
#endif
    }

    if (ch->desc != NULL && ch->desc->snoop_by != NULL)
    {
	write_to_buffer(ch->desc->snoop_by, "% ",    2);
	write_to_buffer(ch->desc->snoop_by, logline, 0);
	write_to_buffer(ch->desc->snoop_by, "\n\r",  2);
    }

    if (IS_SET(ch->comm, COMM_AFK))
    {
       	do_afk(ch, "");
	if (found && cmd_table[cmd].do_fun == do_afk)
	    return;
    }

    if (!found && !IS_NPC(ch))
    {
	prg = p_command_trigger(ch, command, argument, TRIG_NEW_CMD);
	if (!IS_VALID(ch))
	{
	    VALIDATE(ch);
	    return;
	}
    }

    if (!found)
    {
	/*
	 * Look for command in socials table.
	 */
	if (!prg)
	    if (!check_social(ch, command, argument))
		send_to_char("Чего?\n\r", ch);
	return;
    }

    /*
     * Character not in position for command?
     */
    if (ch->position < cmd_table[cmd].position)
    {
	switch(ch->position)
	{
	case POS_DEAD:
	    send_to_char("Бесполезно - тебя же убили.\n\r", ch);
	    break;

	case POS_MORTAL:
	case POS_INCAP:
	    send_to_char("У тебя нет сил сделать это.\n\r", ch);
	    break;

	case POS_STUNNED:
	    send_to_char("Тебя оглушили, и ты не можешь пошевелиться.\n\r", ch);
	    break;

	case POS_SLEEPING:
	    send_to_char("В твоих снах, или как?\n\r", ch);
	    break;

	case POS_RESTING:
	    send_to_char("Ахх... Ведь ты расслабляешься...\n\r", ch);
	    break;

	case POS_SITTING:
	    send_to_char("Для этого тебе необходимо сначала встать.\n\r", ch);
	    break;

	case POS_FIGHTING:
	    send_to_char("Куда? Ведь ты еще сражаешься!\n\r", ch);
	    break;
	case POS_BASHED:
	    act("Тебя же cтолкнули $x!", ch, NULL, NULL, TO_CHAR);
	    break;

	}
	return;
    }

    /*
     * Dispatch the command.
     */
    if (found && !IS_NPC(ch))
    {
	prg = p_command_trigger(ch, cmd_table[cmd].name,
				argument, TRIG_COMMAND);
	if (!IS_VALID(ch))
	{
	    VALIDATE(ch);
	    return;
	}
    }

    if (prg || IS_NPC(ch))
	(*cmd_table[cmd].do_fun)(ch, argument);

    if (!IS_VALID(ch))
    {
	if (!IS_NPC(ch))
	    VALIDATE(ch);

	return;
    }
    
    if (found && !IS_NPC(ch))
	prg = p_command_trigger(ch, cmd_table[cmd].name,
				argument, TRIG_POST_CMD);

    if (!IS_NPC(ch))
	VALIDATE(ch);
    
#ifdef MUD_MEM_DEBUG
    if (string_count < nAllocString)
    {
	sprintf(buf,
	    "Memcheck : Increase in strings :: %s : %s", ch->name, cmd_copy);
	convert_dollars(buf);
	wiznet(buf, NULL, NULL, WIZ_MEMCHECK, 0, 0);
    }

    if (perm_count < nAllocPerm)
    {
	sprintf(buf,
	    "Increase in perms :: %s : %s", ch->name, cmd_copy);
	convert_dollars(buf);
	wiznet(buf, NULL, NULL, WIZ_MEMCHECK, 0, 0);
    }
#endif

    /* Record that the command was the last done, but it is finished */

    tail_chain();
    return;
}

/* function to keep argument safe in all commands -- no static strings */
void do_function (CHAR_DATA *ch, DO_FUN *do_fun, char *argument)
{
    char *command_string;

    /* copy the string */
    command_string = str_dup(argument);

    /* dispatch the command */
    (*do_fun) (ch, command_string);

    /* free the string */
    free_string(command_string);
}

bool check_social(CHAR_DATA *ch, char *command, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int cmd;
    bool found;

    if (!ch)
      return FALSE;

    found  = FALSE;
    for (cmd = 0; social_table[cmd].name[0] != '\0'; cmd++)
    {
	  if ( command[0] == social_table[cmd].name[0]
	     && !str_prefix(command, social_table[cmd].name))
	  {
	      found = TRUE;
	      break;
	  }
    }

    if (!found && convert_trans(ch, command))
    {
        for (cmd = 0; social_table[cmd].name[0] != '\0'; cmd++)
        {
	    if ( command[0] == social_table[cmd].name[0]
	     && !str_prefix(command, social_table[cmd].name))
	    {
	        found = TRUE;
	        break;
	    }
	}
    }

    if (!found)
	return FALSE;

    if (!IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE))
    {
	send_to_char("Ты бесполезен для общества!\n\r", ch);
	return TRUE;
    }

    switch (ch->position)
    {
    case POS_DEAD:
	send_to_char("Бесполезно - тебя убили.\n\r", ch);
	return TRUE;

    case POS_INCAP:
    case POS_MORTAL:
	send_to_char("У тебя нет сил сделать это.\n\r", ch);
	return TRUE;

    case POS_STUNNED:
	sprintf(arg, "Ты оглушен%s и не можешь пошевелиться.\n\r", SEX_ENDING(ch));
	send_to_char(arg, ch);
	return TRUE;

    case POS_SLEEPING:
	/*
	 * I just know this is the path to a 12" 'if' statement.  :(
	 * But two players asked for it already!  -- Furey
	 */
	if (!str_cmp(social_table[cmd].name, "храпеть"))
	    break;
	send_to_char("В твоих снах, или как?\n\r", ch);
	return TRUE;

    }

    one_argument(argument, arg);
    victim = NULL;
    if (arg[0] == '\0')
    {
	act(social_table[cmd].others_no_arg, ch, NULL, victim, TO_ROOM   );
	act(social_table[cmd].char_no_arg,   ch, NULL, victim, TO_CHAR   );
    }
    else if ((victim = get_char_room(ch, NULL, arg, FALSE)) == NULL)
    {
	send_to_char("Таких здесь нет.\n\r", ch);
    }
    else if (victim == ch)
    {
	act(social_table[cmd].others_auto,   ch, NULL, victim, TO_ROOM   );
	act(social_table[cmd].char_auto,     ch, NULL, victim, TO_CHAR   );
    }
    else
    {
	act(social_table[cmd].others_found,  ch, NULL, victim, TO_NOTVICT);
	act(social_table[cmd].char_found,    ch, NULL, victim, TO_CHAR   );
	act(social_table[cmd].vict_found,    ch, NULL, victim, TO_VICT   );

	if (!IS_NPC(ch) && IS_NPC(victim) && !is_animal(victim)
	    && !IS_AFFECTED(victim, AFF_CHARM)
	    && IS_AWAKE(victim)
	    && can_see(victim,ch)
	    && victim->desc == NULL)
	{
	    switch (number_bits(4))
	    {
	    case 0:

	    case 1: case 2: case 3: case 4:
	    case 5: case 6: case 7: case 8:
		act(social_table[cmd].others_found,
		    victim, NULL, ch, TO_NOTVICT);
		act(social_table[cmd].char_found,
		    victim, NULL, ch, TO_CHAR   );
		act(social_table[cmd].vict_found,
		    victim, NULL, ch, TO_VICT   );
		break;

	    case 9: case 10: case 11: case 12:
		act("$n шлепает $N3.",  victim, NULL, ch, TO_NOTVICT);
		act("Ты шлепаешь $N3.",  victim, NULL, ch, TO_CHAR   );
		act("$n шлепает тебя.", victim, NULL, ch, TO_VICT   );
		break;
	    }
	}
    }

    return TRUE;
}



/*
 * Return true if an argument is completely numeric.
 */
bool is_number (char *arg)
{

    if (*arg == '\0')
        return FALSE;

    if (*arg == '+' || *arg == '-')
        arg++;

    for (; *arg != '\0'; arg++)
    {
        if (!isdigit(*arg))
            return FALSE;
    }

    return TRUE;
}



/*
 * DEBUG001
 * Given a string like 14.foo, return 14 and 'foo'
 */
int number_argument(char *argument, char *arg)
{
    char *pdot;
    int number;

    for (pdot = argument; *pdot != '\0'; pdot++){
        if (*pdot == '.'){
            *pdot = '\0';
            number = atoi(argument);
            *pdot = '.';
            strcpy(arg, pdot+1);
            return number;
        }
    }

    strcpy(arg, argument);
    return 1;
}

/*
 * Given a string like 14*foo, return 14 and 'foo'
*/
int mult_argument(char *argument, char *arg)
{
    char *pdot;
    int number;

    for (pdot = argument; *pdot != '\0'; pdot++)
    {
        if (*pdot == '*')
        {
            *pdot = '\0';
            number = atoi(argument);
            *pdot = '*';
            strcpy(arg, pdot+1);
            return number;
        }
    }

    strcpy(arg, argument);
    return 1;
}



/*
 * Выберите один аргумент из строки и верните остальные.
 * Understands quotes.
 */
char *one_argument(char *argument, char *arg_first)
{
    char cEnd;

    if (argument == NULL || argument[0] == '\0'){
  	*arg_first = '\0';
    	return argument;
    }


    while (IS_SPACE(*argument))
	argument++;

    cEnd = ' ';
    if (*argument == '\'' || *argument == '"')
	cEnd = *argument++;

    while (*argument != '\0'){
		if (*argument == cEnd){
			argument++;
			break;
		}
		*arg_first = LOWER(*argument);
		arg_first++;
		argument++;
    }
    *arg_first = '\0';

    while (IS_SPACE(*argument))
	argument++;

    return argument;
}


int compare_strings (const void *v1, const void *v2)
{
    STRING_DATA a1 = *(STRING_DATA*) v1;
    STRING_DATA a2 = *(STRING_DATA*) v2;
    int i;

    i = strcmp(a1.name , a2.name);

    if (i < 0)
	return -1;

    if (i > 0)
	return +1;
    else
	return 0;
}


void do_commands(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    int cmd;
    int col;
    STRING_DATA sorted_commands_list[max_commands];

    col = 0;

    for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++)
    {
        if (cmd_table[cmd].level <  LEVEL_HERO
	    && cmd_table[cmd].level <= get_trust(ch)
	    && ((IS_TRANSLIT(ch)
		&& !(cmd_table[cmd].name[0] & 0x80))
		    || (!IS_TRANSLIT(ch) && cmd_table[cmd].show)))
	      sorted_commands_list[col++].name = cmd_table[cmd].name;
    }

    sorted_commands_list[col].name = NULL;

    qsort(sorted_commands_list, col, sizeof(sorted_commands_list[0]), compare_strings);

    col = 0;

    for (cmd = 0; !IS_NULLSTR(sorted_commands_list[cmd].name); cmd++)
    {
		sprintf(buf, "%-16s", sorted_commands_list[cmd].name);
		send_to_char(buf, ch);

		if (++col % 5 == 0)
		    send_to_char("\n\r", ch);
    }

    if (col % 5 != 0)
	send_to_char("\n\r", ch);
    return;
}

void do_wizhelp(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    int cmd;
    int col;
    STRING_DATA sorted_commands_list[max_commands];

    col = 0;

    for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++)
    {
        if (cmd_table[cmd].level >= LEVEL_HERO
	    && is_granted(ch, cmd_table[cmd].name)
	    && cmd_table[cmd].show)
              sorted_commands_list[col++].name = cmd_table[cmd].name;
    }

    sorted_commands_list[col].name = NULL;

    qsort(sorted_commands_list, col, sizeof(sorted_commands_list[0]), compare_strings);

    col = 0;

    for (cmd = 0; !IS_NULLSTR(sorted_commands_list[cmd].name); cmd++)
    {
	    sprintf(buf, "%-16s", sorted_commands_list[cmd].name);
	    send_to_char(buf, ch);

	    if (++col % 5 == 0)
		send_to_char("\n\r", ch);
    }

    if (col % 5 != 0)
	send_to_char("\n\r", ch);
    return;
}

void do_socials(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    int iSocial;
    int col;
    extern int social_count;
    STRING_DATA sorted_commands_list[social_count];

    col = 0;

    for (iSocial = 0; social_table[iSocial].name[0] != '\0'; iSocial++)
	sorted_commands_list[col++].name = social_table[iSocial].name;

    sorted_commands_list[col].name = NULL;

    qsort(sorted_commands_list, col, sizeof(sorted_commands_list[0]), compare_strings);

    col = 0;

    for (iSocial = 0; !IS_NULLSTR(sorted_commands_list[iSocial].name); iSocial++)
    {
	sprintf(buf, "%-16s", sorted_commands_list[iSocial].name);
	send_to_char(buf, ch);
	if (++col % 5 == 0)
	    send_to_char("\n\r", ch);
    }

    if (col % 5 != 0)
	send_to_char("\n\r", ch);
    return;
}


/* charset=cp1251 */
