#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>
#include "merc.h"
#include "olc.h"
#include "interp.h"
#include "recycle.h"

struct clan_type  clan_table[MAX_CLAN];


extern int errfunc(const char *path, int err);
bool clan_war_enabled = TRUE;
void save_area( AREA_DATA *pArea );

/* Local functions */
static void do_clan_accept(CHAR_DATA *ch, char *argument);
static void do_clan_ally(CHAR_DATA *ch, char *argument);
static void do_clan_command(CHAR_DATA *ch, char *argument);
static void do_clan_master(CHAR_DATA *ch, char *argument);
static void do_clan_master2(CHAR_DATA *ch, char *argument);
static void do_clan_descr(CHAR_DATA *ch, char *argument);
static void do_clan_delete(CHAR_DATA *ch, char *argument);
static void do_clan_war_dis(CHAR_DATA *ch, char *argument);
static void do_clan_enter(CHAR_DATA *ch, char *argument);
static void do_clan_expel(CHAR_DATA *ch, char *argument);
static void do_clan_war_en(CHAR_DATA *ch, char *argument);
static void do_clan_info(CHAR_DATA *ch, char *argument);
static void do_clan_list(CHAR_DATA *ch, char *argument);
static void do_clan_members(CHAR_DATA *ch, char *argument);
static void do_clan_peace(CHAR_DATA *ch, char *argument);
static void do_clan_races(CHAR_DATA *ch, char *argument);
static void do_clan_classes(CHAR_DATA *ch, char *argument);
static void do_clan_short(CHAR_DATA *ch, char *argument);
static void do_clan_talk(CHAR_DATA *ch, char *argument);
static void do_clan_talk_union(CHAR_DATA *ch, char *argument);
static void do_clan_war(CHAR_DATA *ch, char *argument);
static void do_clan_align(CHAR_DATA *ch, char *argument);
static void do_clan_recall(CHAR_DATA *ch, char *argument);

/* Clan commands */
struct clan_cmds
{
    char *cmd;
    DO_FUN *func;
};

static struct clan_cmds clan_commands[] =
{
    { "accept",		do_clan_accept	},
    { "alliance",	do_clan_ally	},
    { "align",		do_clan_align	},
    { "command",	do_clan_command	},
    { "clanmaster",	do_clan_master	},
    { "description",	do_clan_descr	},
    { "delete",		do_clan_delete	},
    { "disable",	do_clan_war_dis	},
    { "enter",		do_clan_enter	},
    { "expel",		do_clan_expel	},
    { "enable",		do_clan_war_en	},
    { "info",		do_clan_info	},
    { "list",		do_clan_list	},
    { "members",	do_clan_members	},
    { "peace",		do_clan_peace	},
    { "races",		do_clan_races	},
    { "recall",		do_clan_recall	},
    { "short",		do_clan_short	},
    { "talk",		do_clan_talk	},
    { "utalk",		do_clan_talk_union	},
    { "war",		do_clan_war		},
    { "вступить",	do_clan_enter	},
    { "выгнать",	do_clan_expel	},
    { "война",		do_clan_war		},
    { "говорить",	do_clan_talk	},
    { "гсоюз",		do_clan_talk_union	},
    { "добавить",	do_clan_accept	},
    { "запретить",	do_clan_war_dis	},
    { "инфо",		do_clan_info	},
    { "команда",	do_clan_command	},
    { "короткое",	do_clan_short	},
    { "кланмастер",	do_clan_master	},
    { "натура",		do_clan_align	},
    { "замкланмастер",	do_clan_master2	},
    { "мир",		do_clan_peace	},
    { "нет",		do_clan_expel	},
    { "описание",	do_clan_descr	},
    { "принять",	do_clan_accept	},
    { "разрешить",	do_clan_war_en	},
    { "расы",		do_clan_races	},
    { "классы",		do_clan_classes	},
    { "список",		do_clan_list	},
    { "союз",		do_clan_ally	},
    { "удалить",	do_clan_delete	},
    { "члены",		do_clan_members	},
	{ NULL,		NULL		}
};

static char *fast_read_string(FILE *fp)
{
    static char plast[MAX_STRING_LENGTH];
    int i = 0;
    char c;

    /*
     * Skip blanks.
     * Read first char.
     */
    do
    {
	c = getc( fp );
    }
    while ( IS_SPACE(c) );

    if ( c == '~' )
    {
	plast[i] = '\0';
	return plast;
    }	

    plast[i] = c;

    for ( ;; )
    {
	/*
	 * Back off the char type lookup,
	 *   it was too dirty for portability.
	 *   -- Furey
	 */
	i++;

	if (i > MAX_STRING_LENGTH - 10)
	{
	    bugf("Fread_string: EOF");
	    plast[0] = '\0';
	    return plast;
	}

	switch ( plast[i] = getc(fp) )
	{
	default:
	    break;

	case EOF:
	    /* temp fix */
	    if (feof(fp))
	    {
		bugf("Fread_string: EOF");
		return NULL;
	    }
	    break; 

	case '\n':
	    i++;
	    plast[i] = '\r';
	    break;

	case '\r':
	    break;

	case '~':
	    plast[i] = '\0';
	    return plast;
	}
    }
}


static void remove_clanrooms(CHAR_DATA *ch, int clan)
{
    int iHash;
    bool quit = FALSE;

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
	ROOM_INDEX_DATA *room;

	for (room = room_index_hash[iHash]; room != NULL; room = room->next)
	    if (clan == room->clan)
	    {
		char buf[MAX_STRING_LENGTH];
		AREA_DATA *area;
		OBJ_DATA *obj;
		CHAR_DATA *mob;

		send_to_char("\n\rObject in room:\n\r", ch);
		for (obj = room->contents; obj; obj = obj->next_content)
		{
		    sprintf(buf, "[%d] %s\n\r",
			    obj->pIndexData->vnum, obj->name);
		    send_to_char(buf,ch);
		}
		send_to_char("\n\rMobiles in room:\n\r", ch);
		LIST_FOREACH(mob, &room->people, room_link)
		{
		    if (IS_NPC(mob))
		    {
			sprintf(buf, "[%d] %s\n\r",
				mob->pIndexData->vnum, mob->short_descr);
			send_to_char(buf,ch);
		    }
		}
		send_to_char("\n\r", ch);
		char_from_room(ch);
		char_to_room(ch, room, FALSE);

		area = room->area;
		redit_delete(ch, "");	    
		save_area(area);
		quit = TRUE;
		break;
	    }

	if (quit)
	    break;
    }

    if (quit)
	remove_clanrooms(ch, clan);

    return;
}

int is_clan(CHAR_DATA *ch)
{
    return (ch->clan > 0 && clan_table[ch->clan].valid ? ch->clan : 0);
}

static bool correct_clan_command(CHAR_DATA *ch, char *argument)
{
    char *buf, arg1[MAX_STRING_LENGTH];
    int i;
    struct clan_attack
    {
	char *command;
	int16_t gsn;
    } attack[] =
    {
	{ "hit",	0 },
	{ "kill",	0 },
	{ "убить",	0 },
	{ "ambush",	gsn_ambush },
	{ "засада",	gsn_ambush },
	{ "backstab",	gsn_backstab },
	{ "bash",	gsn_bash },
	{ "bs",		gsn_backstab },
	{ "dirt",	gsn_dirt },
	{ "trip",	gsn_trip },
	{ "tail",	gsn_tail },
	{ "lunge",	gsn_lunge },
	{ "deafen",	gsn_deafen },
	{ "вспину",	gsn_backstab },
	{ "пырнуть",	gsn_backstab },
	{ "толчок",	gsn_bash },
	{ "грязь",	gsn_dirt },
	{ "подножка",	gsn_trip },
	{ "подсечь",	gsn_trip },
	{ "выпад",	gsn_lunge },
	{ "оглушить",	gsn_deafen },
	{ "прыжок",	gsn_jump },
//	{ "cleave",	gsn_cleave },
	{ "хвост",	gsn_tail },
	{ "bite",	gsn_bite },
	{ "укусить",	gsn_bite },
	{ "насытиться",	gsn_bite },
//	{ "проткнуть",	gsn_pierce },
//	{ "pierce",	gsn_pierce },
	{ NULL,		0 }
    };


    buf = str_dup(argument);
    buf = one_argument(buf, arg1);

    if (!str_prefix(arg1, "колдовать") || !str_prefix(arg1, "cast"))
    {
	int sn;

	sn = skill_lookup(buf);

	if (sn == -1)
	{
		bugf("Not correct spell.");
	}

	if ((sn != -1) 
	    && ( sn != gsn_power_kill && sn != gsn_earthmaw)
	    && (skill_table[sn].target == TAR_CHAR_OFFENSIVE || skill_table[sn].target == TAR_OBJ_CHAR_OFF)
	    && ch->pcdata->learned[sn] > 0)
	{	    
	    sprintf(argument,"%s%s%s%s",arg1," '",buf,"'");
	    return TRUE;	
	}
	else
	    return FALSE;
    }

    for (i = 0; attack[i].command != NULL; i++)
	if (!str_prefix(arg1, attack[i].command)
	    && (attack[i].gsn == 0
		|| ch->pcdata->learned[attack[i].gsn] > 0))
	    return TRUE;

    return FALSE;
}

void save_clans(void)
{
    FILE *fp;
    int  i,j;

    if ((fp = file_open(CLANS_FILE, "w")) == NULL)
	bugf("Couldn't open clans file for writing...");
    else
    {
	for (i = 0; i < MAX_CLAN; i++)
	    if (clan_table[i].valid)
	    {
		fprintf(fp,"#%s\n", clan_table[i].name);
		fprintf(fp, "%s~\n", clan_table[i].short_descr);
		fprintf(fp, "%s~\n",clan_table[i].long_descr);
		fprintf(fp, "%s~\n", clan_table[i].clanmaster);
		fprintf(fp, "%s~\n", clan_table[i].clanmaster2);

		for (j = 1; j < MAX_PC_RACE; j++)
		    if (clan_table[i].races[j])
			fprintf(fp, "%s ", race_table[j].rname);
		fprintf(fp, "~\n");

		for (j = 0; j < MAX_CLASS; j++)
		    if (clan_table[i].classes[j])
			fprintf(fp, "%s ", class_table[j].name);
		fprintf(fp, "~\n");

		fprintf(fp, "%d\n", clan_table[i].count);
		fprintf(fp, "%d\n", clan_table[i].min_align);
		fprintf(fp, "%d\n", clan_table[i].max_align);
		fprintf(fp, "%d\n", clan_table[i].recall_vnum);

		for (j = 0; j < MAX_CLAN; j++)
		    if (clan_table[i].clan_war[j])
			fprintf(fp, "%s ", clan_table[j].name);
		fprintf(fp, "~\n");

		for (j = 0; j < MAX_CLAN; j++)
		    if (clan_table[i].clan_alliance[j])
			fprintf(fp, "%s ", clan_table[j].name);
		fprintf(fp, "~\n");
		fprintf(fp, "%d\n", clan_table[i].rating);
	    }
	fprintf(fp, "#0\n");
    }
    file_close(fp);
    return;
}

static bool is_clans_in_war(int c1, int c2)
{
    int i;

    if (clan_table[c1].clan_war[c2] || clan_table[c2].clan_war[c1])
	return TRUE;

    for (i = 0; i < MAX_CLAN; i++)
	if ((clan_table[i].clan_war[c1]
	     && clan_table[c2].clan_alliance[i]
	     && clan_table[i].clan_alliance[c2])
	    || (clan_table[i].clan_war[c2]
		&& clan_table[c1].clan_alliance[i]
		&& clan_table[i].clan_alliance[c1]))
	{
	    return TRUE;
	}

    return FALSE;
}

bool is_clans_in_alliance(int c1, int c2)
{
   if (c1 == c2)
	return TRUE;

    if (clan_table[c1].clan_alliance[c2] && clan_table[c2].clan_alliance[c1])
	return TRUE;

    return FALSE;
}


bool is_in_war(CHAR_DATA *ch, CHAR_DATA *victim)
{
    if (!is_clan(ch) || !is_clan(victim))
	return FALSE;

    return is_clans_in_war(ch->clan, victim->clan);
}

void clan_command(CHAR_DATA *fch, CHAR_DATA *ch)
{
    char buf[MSL];

    if (IS_SET(fch->in_room->room_flags, ROOM_HOLY) || IS_SET(fch->in_room->room_flags, ROOM_GUILD))
	return;
	
    if(fch->war_command_on == 0)
        return;

    sprintf(buf, "Я ненавижу тебя, %s!", ch->name);
    do_function(fch, &do_yell, buf);
    if (IS_NULLSTR(fch->war_command))
	do_function(fch, &do_kill, ch->name);
    else
    {
	sprintf(buf, "%s %s", fch->war_command, ch->name);
	interpret(fch, buf);
	if (ch->fighting)
	    do_function(fch, &do_kill, ch->name);
    }
}

void check_clan_war(CHAR_DATA *ch)
{
    CHAR_DATA *fch, *safe_fch;

    if (!clan_war_enabled || !ch->in_room || IS_AFFECTED(ch, AFF_CHARM) 
	|| !ch->in_room || IS_SET(ch->in_room->room_flags, ROOM_SAFE))
	return;

    LIST_FOREACH_SAFE(fch, &ch->in_room->people, room_link, safe_fch)
    {    
	if (!IS_NPC(fch) && !IS_AFFECTED(fch, AFF_CHARM) && is_in_war(ch, fch))
	{
	    if (fch->position > POS_SLEEPING && can_see(fch, ch) && fch->wait == 0 
		&& !is_affected(ch, gsn_timeout) && !is_affected(fch, gsn_timeout)
		&& SUPPRESS_OUTPUT(!is_safe(fch, ch)) && !fch->fighting
		&& (number_percent() > 50 || !can_see(ch, fch) || ch->wait > 0 
		    || ch->fighting || ch->position <= POS_SLEEPING)
		&& !is_affected(ch, gsn_sleep))
	    {
		clan_command(fch, ch);
		return;
	    }
	    else if (ch->position > POS_SLEEPING && !is_affected(ch, gsn_timeout) && !is_affected(fch, gsn_timeout) 
			&& can_see(ch, fch) && ch->wait == 0 && SUPPRESS_OUTPUT(!is_safe(ch, fch)) && !ch->fighting 
			&& !is_affected(fch, gsn_sleep))
	    {
		clan_command(ch, fch);
		return;
	    }
	}
    }
}


int is_clanmaster(CHAR_DATA *ch)
{
    if (is_clan(ch) && !str_cmp(ch->name, clan_table[ch->clan].clanmaster))
	return ch->clan;
    return 0;
}

int is_clanmaster2(CHAR_DATA *ch)
{
    if (get_char_world(ch, clan_table[ch->clan].clanmaster))
    {
	send_to_char("Глава клана здесь. Пусть он принимает решения.\n\r",ch);
	return 0;
    }

    if (is_clan(ch) && !str_cmp(ch->name, clan_table[ch->clan].clanmaster2))
	return ch->clan;
    return 0;
}

static void do_clan_accept(CHAR_DATA *ch, char *argument)
{
    int	clan;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    if ((clan = is_clanmaster(ch)) == 0 
	&& (clan = is_clanmaster2(ch)) == 0 
	&& !IS_IMMORTAL(ch))
    {
	send_to_char( "Но ты же не бессмертный и не (зам) кланмастер!\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || (arg2[0] == '\0' && IS_IMMORTAL(ch)))
    {
	printf_to_char("Синтаксис: клан добавить <игрок>%s\n\r",
		       ch, IS_IMMORTAL(ch) ? " <имя клана>" : "");
	return;
    }

    if ((victim = get_char_world(ch, arg1)) == NULL || IS_NPC(victim))
    {
	send_to_char("Таких игроков сейчас нет в этом мире.\n\r", ch);
	return;
    }

    if (is_clanmaster(victim))
    {
	send_to_char("Этот игрок является кланмастером этого клана.\n\r", ch);
	return;
    }

    if (victim->level < MIN_CLAN_LEVEL)
    {
	send_to_char("Этому игроку рановато еще думать о кланах.\n\r", ch);
	return;
    }

    if (IS_IMMORTAL(ch))
    {
	if ((clan = clan_lookup(arg2)) == 0)
	{
	    send_to_char("Таких кланов нет.\n\r", ch);
	    return;
	}
    }

    if (victim->clan_ready != clan)
    {
	send_to_char("Этот игрок не давал своего согласия на вступление "
		     "в этот клан.\n\r",ch);
	return;
    }

    if (clan_table[clan].count >= MAX_CLAN_PLAYERS
	&& str_cmp(clan_table[clan].name, CLAN_INDEPEND))
    {
	send_to_char("В этом клане уже нет свободного места.\n\r", ch);
	return;
    }

    if (clan_table[clan].min_align > victim->alignment
	|| clan_table[clan].max_align < victim->alignment)
    {
	send_to_char("Этот игрок не соответствует характеру клана.\n\r", ch);
	return;
    }

    if (!clan_table[clan].races[victim->race])
    {
	send_to_char("Раса этого игрока не соответствует характеру клана.\n\r", ch);
	return;
    }

    if (!clan_table[clan].classes[victim->classid])
    {
	send_to_char("Профессия этого игрока не соответствует характеру клана.\n\r", ch);
	return;
    }

    printf_to_char("Теперь этот игрок член клана %s.\n\r", ch, clan_table[clan].short_descr);
    printf_to_char("Ты теперь член клана %s.\n\r", victim, clan_table[clan].short_descr);
    victim->clan = clan;
    victim->clan_ready = 0;
    victim->war_command_on = 1;
    clan_table[clan].count++;

    return;
}

static void do_clan_ally(CHAR_DATA *ch, char *argument)
{
    int clan, wclan;
    char arg1[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

    if ((clan = is_clanmaster(ch)) == 0 && (clan = is_clanmaster2(ch)) == 0)
    {
	send_to_char("Но ты же не (зам) кланмастер!\n\r", ch);
	return;
    }

    one_argument(argument, arg1);
    if ((wclan = clan_lookup(arg1)) == 0)
    {
	send_to_char("Таких кланов нет в этом мире.\n\r", ch);
	return;
    }

    if (clan == wclan)
    {
	send_to_char("Смысл заключать союз со своим кланом?\n\r", ch);
	return;
    }

    if (!strcmp(clan_table[wclan].name, CLAN_INDEPEND))
    {
	send_to_char("С ними нельзя заключить союз.\n\r", ch);
	return;
    }

    if (clan_table[clan].clan_alliance[wclan])
    {
	clan_table[clan].clan_alliance[wclan] = FALSE;
	sprintf(buf, "Клан %s разрывает союз с кланом %s",
		clan_table[clan].short_descr, clan_table[wclan].short_descr);
    }
    else
    {
	if (is_clans_in_war(clan, wclan))
	{
	    send_to_char("Вы в состоянии войны с этим кланом (напрямую, "
			 "либо через союзные кланы).\n\r", ch);
	    return;
	}

	clan_table[clan].clan_alliance[wclan] = TRUE;
	sprintf(buf, "Клан %s объявляет союз с кланом %s",
		clan_table[clan].short_descr, clan_table[wclan].short_descr);
    }
    do_function(ch, &do_echo, buf);
    save_clans();

    return;
}

static void do_clan_command(CHAR_DATA *ch, char *argument)
{
    if (!is_clan(ch))
    {
	send_to_char("Но ты же не в клане!\n\r", ch);
	return;
    }
    
	if (IS_NULLSTR(argument) || argument[0] == '\0')
	{
		if (ch->war_command_on)
			send_to_char("Твоя клан-команда включена.\n\r", ch);
		else
			send_to_char("Твоя клан-команда выключена.\n\r", ch);
			
		if (!IS_NULLSTR(ch->war_command) && ch->war_command != '\0')
		{
			send_to_char("Твоя клан-команда: ", ch);
			send_to_char(ch->war_command, ch);
			send_to_char("\n\r", ch);
		}
		else
		{
			send_to_char("Ты до сих пор не установил клан-команду.\n\r", ch);
		}
		return;
	}
				
    
    if(!str_cmp(argument, "on") || !str_cmp(argument, "включить"))
    {
	    if (ch->war_command_on)
	    {
		send_to_char("Ты выключаешь клан-команду.\n\r", ch);
		ch->war_command_on = 0;
	    }
	    else
	    {
		send_to_char("Ты включаешь клан-команду.\n\r", ch);
		ch->war_command_on = 1;
	    }
	    return;
    }

    if (!correct_clan_command(ch, argument))
    {
	send_to_char("Команда должна быть атакующим действием либо "
		     "заклинанием.\n\r", ch);
	return;
    }

    free_string(ch->war_command);
    ch->war_command = str_dup(argument);

    send_to_char("Ok.\n\r", ch);

    return;
}

static void do_clan_master(CHAR_DATA *ch, char *argument)
{
    int clan;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    if ((clan = is_clanmaster(ch)) == 0 && !IS_IMMORTAL(ch))
    {
	send_to_char("Но ты же не бессмертный и не кланмастер!\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || (arg2[0] == '\0' && IS_IMMORTAL(ch)))
    {
	printf_to_char("Синтаксис: клан кланмастер <игрок>%s\n\r",
		       ch, IS_IMMORTAL(ch) ? " [<имя клана>]" : "");
	return;
    }

    if ((victim = get_char_world(ch, arg1)) == NULL || IS_NPC(victim))
    {
	send_to_char("Таких игроков сейчас нет в этом мире.\n\r", ch);
	return;
    }

    if (IS_IMMORTAL(ch) && (clan = clan_lookup(arg2)) == 0)
    {
	send_to_char("Таких кланов нет.\n\r", ch);
	return;
    }

    if (!is_clan(victim))
	send_to_char("Этот игрок не в клане!\n\r", ch);
    else if (victim == ch && !IS_IMMORTAL(ch))
	send_to_char("Гмм... Ты и так глава своего клана.\n\r", ch);
    else if (!IS_IMMORTAL(ch) && ch->clan != victim->clan)
	send_to_char("Не трогай этого игрока - он не в твоем клане.\n\r", ch);
    else
    {
	free_string(clan_table[clan].clanmaster);
	clan_table[clan].clanmaster = str_dup(victim->name);
	send_to_char("Ты теперь глава клана.\n\r", victim);
	send_to_char("Выбранный тобой игрок теперь глава твоего клана.\n\r", ch);
    }

    save_clans();
    return;
}

static void do_clan_master2(CHAR_DATA *ch, char *argument)
{
    int clan;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    if ((clan = is_clanmaster(ch)) == 0 && !IS_IMMORTAL(ch))
    {
	send_to_char("Но ты же не бессмертный и не кланмастер!\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || (arg2[0] == '\0' && IS_IMMORTAL(ch)))
    {
	printf_to_char("Синтаксис: клан замкланмастер <игрок>%s\n\r",
		       ch, IS_IMMORTAL(ch) ? " [<имя клана>]" : "");
	printf_to_char("Синтаксис: клан замкланмастер убрать%s\n\r",
		       ch, IS_IMMORTAL(ch) ? " [<имя клана>]" : "");
	return;
    }

    if (IS_IMMORTAL(ch) && (clan = clan_lookup(arg2)) == 0)
    {
	send_to_char("Таких кланов нет.\n\r", ch);
	return;
    }

    if (!str_prefix(arg1,"убрать"))
    {
	free_string(clan_table[clan].clanmaster2);
	clan_table[clan].clanmaster2 = str_dup("");
	send_to_char("Ты отказываешься от заместителя главы клана.\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, arg1)) == NULL || IS_NPC(victim))
    {
	send_to_char("Таких игроков сейчас нет в этом мире.\n\r", ch);
	return;
    }

    if (!is_clan(victim))
	send_to_char("Этот игрок не в клане!\n\r", ch);
    else if (victim == ch && !IS_IMMORTAL(ch))
	send_to_char("Гмм... Ты глава своего клана.\n\r", ch);
    else if (!IS_IMMORTAL(ch) && ch->clan != victim->clan)
	send_to_char("Не трогай этого игрока - он не в твоем клане.\n\r", ch);
    else
    {
	free_string(clan_table[clan].clanmaster2);
	clan_table[clan].clanmaster2 = str_dup(victim->name);
	send_to_char("Ты теперь заместитель главы клана.\n\r", victim);
	send_to_char("Выбранный тобой игрок теперь заместитель твоего клана.\n\r", ch);
    }

    save_clans();
    return;
}

static void do_clan_descr(CHAR_DATA *ch, char *argument)
{
    int clan = is_clanmaster(ch);

    if (clan == 0)
    {
	send_to_char("Но ты же не кланмастер!\n\r", ch);
	return;
    }

    if (ch->desc)
        string_append(ch, &clan_table[clan].long_descr);
}

static void do_clan_delete(CHAR_DATA *ch, char *argument)
{
    ROOM_INDEX_DATA *curr;
    char arg1[MAX_INPUT_LENGTH];
    int i;

    if (get_trust(ch) < MAX_LEVEL)
    {
	send_to_char("Тебе не положено этим заниматься.\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg1);

    if ((i = clan_lookup(arg1)) == 0)
    {
	send_to_char("Таких кланов нет в этом мире.\n\r", ch);
	return;
    }

    clan_table[i].valid = FALSE;

    /* Удаление кланрумов */

    curr = ch->in_room;
    remove_clanrooms(ch, i);
    char_from_room(ch);
    char_to_room(ch, curr, FALSE);
    send_to_char("Ok.\n\r", ch);
    save_clans();
    return;
}

static void do_clan_war_dis(CHAR_DATA *ch, char *argument)
{
    if (!IS_IMMORTAL(ch))
    {
	send_to_char("Тебе не положено этим заниматься.\n\r", ch);
	return;
    }

    clan_war_enabled = FALSE;
    do_echo(ch, "{RКлановые войны временно приостановлены.{x");
    return;
}

static void do_clan_align(CHAR_DATA *ch, char *argument)
{

	int i;
	int min_align = 0, max_align = 0;
    char arg1[MAX_INPUT_LENGTH];

	if (!IS_IMMORTAL(ch))
    {
		send_to_char("Тебе не положено этим заниматься.\n\r", ch);
		return;
    }

    argument = one_argument(argument, arg1);

    if ((i = clan_lookup(arg1)) == 0)
    {
		send_to_char("Таких кланов нет в этом мире.\n\r", ch);
		return;
    }

    argument = one_argument(argument, arg1);

    if (is_number(arg1))
    {
		min_align=atoi(arg1);
		if (min_align < -1000 || min_align > 1000)
		{
			send_to_char("Ты должен ввести разумный параметр минимальной натуры (от -1000 до 1000).\n\r", ch);
			return;
		}
	}

    argument = one_argument(argument, arg1);

    if (is_number(argument))
    {
		max_align=atoi(argument);
		if (max_align < -1000 || max_align > 1000)
		{
			send_to_char("Ты должен ввести разумный параметр максимальной натуры (от -1000 до 1000).\n\r", ch);
			return;
		}
	}


	if (min_align == 0 || max_align == 0)
	{
		send_to_char("Введите корректное числое значение натур.\n\r", ch);
		return;
	}

	clan_table[i].min_align = min_align;
	clan_table[i].max_align = max_align;
	save_clans();

	send_to_char("Ok.\n\r", ch);
	return;
}

static void do_clan_recall(CHAR_DATA *ch, char *argument)
{
    int i;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;

	if (!IS_IMMORTAL(ch))
    {
		send_to_char("Тебе не положено этим заниматься.\n\r", ch);
		return;
    }

    argument = one_argument(argument, arg1);

    if ((i = clan_lookup(arg1)) == 0)
    {
		send_to_char("Таких кланов нет в этом мире.\n\r", ch);
		return;
    }

    argument = one_argument(argument, arg2);

    if ((location = find_location(ch, arg2)) == NULL)
    {
		send_to_char("Нет такой комнаты.\n\r", ch);
		return;
    }

    clan_table[i].recall_vnum = location->vnum;
    send_to_char("Ok.\n\r", ch);
    save_clans();

	return;
}


static void do_clan_enter(CHAR_DATA *ch, char *argument)
{
    int clan;
    char arg1[MAX_INPUT_LENGTH];

    if (ch->level < MIN_CLAN_LEVEL)
    {
	send_to_char("Тебе рановато еще думать о кланах.\n\r", ch);
	return;
    }

    if (is_clan(ch) && !IS_INDEPEND(ch))
    {
	send_to_char("Сначала выйди из своего клана.\n\r", ch);
	return;
    }

    one_argument(argument, arg1);

    if ((clan = clan_lookup(arg1)) == 0)
    {
	send_to_char("Таких кланов нет в этом мире.\n\r", ch);
	return;
    }
    ch->clan_ready = clan;
    send_to_char("Ok.\n\r", ch);
    return;
}

static void do_clan_expel(CHAR_DATA *ch, char *argument)
{
    int tm_time = current_time;
    DESCRIPTOR_DATA *d = NULL;
    bool fFree = FALSE;
    char arg1[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    if (is_clanmaster(ch) == 0 && is_clanmaster2(ch) == 0 && !IS_IMMORTAL(ch))  
    {
	send_to_char("Но ты же не бессмертный и не (зам) кланмастер!\n\r", ch);
	return;
    }

    one_argument(argument, arg1);

    if (arg1[0] == '\0')
    {
	send_to_char("Синтаксис: клан выгнать <игрок>\n\r", ch);
	return;
    }

    if ((victim = get_char_world(NULL, arg1)) == NULL || IS_NPC(victim))
    {
	d = new_descriptor();

	if (!load_char_obj(d, arg1, FALSE, FALSE))
	{
	    send_to_char("Таких нет в этом мире.\n\r", ch);
	    free_descriptor(d);
	    return;
	}

	victim = d->character;
	fFree = TRUE;
	current_time = victim->pcdata->lastlogof;
	check_light_status(victim);
    }

    if (is_clanmaster(victim))
    {
	send_to_char("Игрок является кланмастером этого клана. Кого ты хочешь выгнать?\n\r", ch);
	return;
    }

    if (!is_clan(victim))
	send_to_char("Этот игрок не в клане!\n\r", ch);
    else if (victim == ch)
	send_to_char("Гмм... Самого себя ты выгнать не сможешь.\n\r", ch);
    else if (!IS_IMMORTAL(ch) && ch->clan != victim->clan)
	send_to_char("Не трогай этого игрока - он не в твоем клане.\n\r",
		     ch);
    else
    {
	send_to_char("Теперь это игрок не участвует в делах клана.\n\r",
		     ch);

	if (!fFree)
	    send_to_char("Ты теперь не являешься членом клана!\n\r",
			 victim);

	clan_table[victim->clan].count--;
	if (!IS_INDEPEND(victim))
	    victim->clan = clan_lookup(CLAN_INDEPEND);
	else
	    victim->clan = 0;
    }

    if (fFree)
    {
	VALIDATE(victim);
	save_char_obj(victim, FALSE);
	extract_char_for_quit(victim, FALSE);
	free_descriptor(d);
	current_time = tm_time;
    }

    return;
}

static void do_clan_war_en(CHAR_DATA *ch, char *argument)
{
    if (!IS_IMMORTAL(ch))
    {
	send_to_char("Тебе не положено этим заниматься.\n\r", ch);
	return;
    }

    clan_war_enabled = TRUE;
    do_echo(ch, "{RКлановые войны вступают в силу.{x");
    return;
}

static void do_clan_list(CHAR_DATA *ch, char *argument)
{
    BUFFER *output;
    int i;

    output = new_buf();
    add_buf(output, "В настоящий момент в Мире действуют следующие кланы:\n\r\n\r");

    for (i = 1; i < MAX_CLAN; i++)
	if (clan_table[i].valid)
	{
	    printf_to_buffer(output, "%2d) %s (%-15s)\n\r",
			     i, str_color_fmt(clan_table[i].short_descr, 30),
			     clan_table[i].name);
	}

    page_to_char(buf_string(output), ch);
    free_buf(output);

    return;
}

static void do_clan_members(CHAR_DATA *ch, char *argument)
{
    BUFFER *output;
    int count = 0;
    int clan;
    char arg1[MAX_INPUT_LENGTH];
    DIR *dp;
    struct dirent *ent;

    if ((clan = is_clanmaster(ch)) == 0 
	&& (clan = is_clanmaster2(ch)) == 0 
	&& !IS_IMMORTAL(ch))  
    {
	send_to_char("Но ты же не Бессмертный и не (зам) кланмастер!\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg1);

    if (clan == 0 && (clan = clan_lookup(arg1)) == 0)
    {
	send_to_char("Таких кланов нет в этом мире.\n\r", ch);
	return;
    }

    if ((dp = opendir(PLAYER_DIR)) == NULL)
    {
	bugf("Can't open players dir %s!", PLAYER_DIR);
	return;
    }

    output = new_buf();

    printf_to_buffer(output, "Члены клана %s:\n\r\n\r", clan_table[clan].short_descr);

    while ((ent = readdir(dp)) != NULL)
    {
	char *name;
	char fname[MAX_STRING_LENGTH];
	bool bad;
	FILE *fp = NULL;

	bad = FALSE;

	for (name = ent->d_name; *name != '\0'; name++)
	{
	    if (!IS_RUSSIAN(*name))
	    {
		bad = TRUE;
		break;
	    }
	}

	if (bad)
	    continue;

	snprintf(fname, sizeof(fname), "%s%s", PLAYER_DIR, ent->d_name);

	if ((fp = file_open(fname, "r")) != NULL)
	{
	    char *word;
	    for (;;)
	    {
		char *str;

		if (feof(fp))
		    break;

		word = fread_word(fp);

		/* fix with bad names */
		if (!str_cmp(word, "Name"))
		{
		    str = fast_read_string(fp);
		    if (UPPER(*str) != *str)
			break;
		}

		if (!str_cmp(word, "Clan"))
		{
		    str = fast_read_string(fp);

		    if (clan == clan_lookup(str))
		    {
			add_buf(output, ent->d_name);
			add_buf(output, "\n\r");
			count++;
			break;
		    }
		    fread_to_eol(fp);
		}
	    }
	}
	file_close(fp);
    }
    printf_to_buffer(output, "\n\rНайдено: %d\n\r", count);
    page_to_char(output->string, ch);
    free_buf(output);
    closedir(dp);
    return;
}

static void do_clan_peace(CHAR_DATA *ch, char *argument)
{
    int i;
    int clan;
    char arg1[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

    if ((i = is_clanmaster(ch)) == 0 && (i = is_clanmaster2(ch)) == 0)  
    {
	send_to_char("Но ты же не кланмастер!\n\r", ch);
	return;
    }

    one_argument(argument, arg1);

    if ((clan = clan_lookup(arg1)) == 0)
    {
	send_to_char("Таких кланов нет в этом мире.\n\r", ch);
	return;
    }

    if (!clan_table[i].clan_war[clan])
    {
	send_to_char("Ты не объявлял войны этому клану.\n\r", ch);
	return;
    }

    clan_table[i].clan_war[clan] = FALSE;

    sprintf(buf, "Клан %s заключает мир с кланом %s",
	    clan_table[i].short_descr, clan_table[clan].short_descr);

    do_function(ch, &do_echo, buf);

    save_clans();

    return; 
}

static void do_clan_races(CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    int race;
    int i;

    if (!IS_IMMORTAL(ch))  
    {
	send_to_char("Но ты же не бессмертный!\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg1);

    if ((i = clan_lookup(arg1)) == 0)
    {
	send_to_char("Таких кланов нет в этом мире.\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg1);

    if ((race = race_lookup(arg1)) == -1)
    {
	send_to_char("Таких рас нет в этом мире.\n\r", ch);
	return;
    }

    clan_table[i].races[race] = !clan_table[i].races[race];
    send_to_char("Ok.\n\r", ch);
    save_clans();
    return;
}

static void do_clan_classes(CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    int classid;
    int i;

    if (!IS_IMMORTAL(ch))  
    {
	send_to_char("Но ты же не бессмертный!\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg1);

    if ((i = clan_lookup(arg1)) == 0)
    {
	send_to_char("Таких кланов нет в этом мире.\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg1);

    if ((classid = class_lookup(arg1)) == -1)
    {
	send_to_char("Таких классов нет в этом мире.\n\r", ch);
	return;
    }

    clan_table[i].classes[classid] = !clan_table[i].classes[classid];
    send_to_char("Ok.\n\r", ch);
    save_clans();
    return;
}
    
static void do_clan_short(CHAR_DATA *ch, char *argument)
{
    int i;
    char arg1[MAX_INPUT_LENGTH];

    if (!IS_IMMORTAL(ch))  
    {
	send_to_char("Но ты же не бессмертный!\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg1);

    if ((i = clan_lookup(arg1)) == 0)
    {
	send_to_char("Таких кланов нет в этом мире.\n\r", ch);
	return;
    }
    free_string(clan_table[i].short_descr);
    clan_table[i].short_descr = str_dup(argument);
    send_to_char("Ok.\n\r", ch);
    save_clans();
    return;
}
    
static void do_clan_talk(CHAR_DATA *ch, char *argument)
{
    do_function(ch, &do_clantalk, argument);
    return;
}

static void do_clan_talk_union(CHAR_DATA *ch, char *argument)
{
    do_function(ch, &do_clantalkunion, argument);
    return;
}

    
static void do_clan_war(CHAR_DATA *ch, char *argument)
{
    int i, j;
    int clan;
    char arg1[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

    if ((i = is_clanmaster(ch)) == 0 && (i = is_clanmaster2(ch)) == 0)
    {
	send_to_char("Но ты же не кланмастер!\n\r", ch);
	return;
    }

    one_argument(argument, arg1);

    if ((clan=clan_lookup(arg1)) == 0)
    {
	send_to_char("Таких кланов нет в этом мире.\n\r", ch);
	return;
    }

    if (i == clan)
    {
	send_to_char("Тебе что-то не нравится в твоем клане?\n\r", ch);
	return;
    }

    if (!strcmp(clan_table[clan].name, CLAN_INDEPEND))
    {
	send_to_char("Им нельзя объявить войну.\n\r", ch);
	return;
    }

    if (clan_table[i].clan_war[clan])
    {
	send_to_char("Вы уже воюете с этим кланом.\n\r", ch);
	return;
    }

    if (clan_table[i].clan_alliance[clan])
    {
	send_to_char("У Вас союз с этим кланом.\n\r", ch);
	return;
    }

    for (j = 1; j < MAX_CLAN; j++)
	if (clan_table[j].clan_alliance[i]
	    && clan_table[i].clan_alliance[j]
	    && clan_table[j].clan_alliance[clan]
	    && clan_table[clan].clan_alliance[j])
	{
	    send_to_char("Они же твои союзники! (через другие кланы)\n\r",
			 ch);
	    return;
	}

    clan_table[i].clan_war[clan] = TRUE;

    sprintf(buf, "Клан %s объявляет войну клану %s",
	    clan_table[i].short_descr, clan_table[clan].short_descr);

    do_function(ch, &do_echo, buf);

    save_clans();

    return; 
}

static void do_clan_info(CHAR_DATA *ch, char *argument)
{
    int found = 0;
    int i, j;
    bool fAll = 0;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    BUFFER *output;

    output = new_buf();

    one_argument(argument, arg);

    j = 0;

    if (arg[0] == '\0')
    {
	if ((j = is_clan(ch)) == 0)
	{
	    send_to_char("Но ведь ты же не в клане! Укажи, информацию о "
			 "каком клане ты хочешь посмотреть, или 'все' для "
			 "списка всех кланов.\n\r", ch);
	    free_buf(output);
	    return;
	}
    }
    else if (!str_cmp(arg,"все") || !str_cmp(arg,"all"))
	fAll = TRUE;
    else
	j = clan_lookup(arg);

    for (i = 1; i < MAX_CLAN ;i++)
    {
	if (clan_table[i].valid && (i == j || fAll))
	{
	    found++;
	    sprintf(buf,"\n\r"
		    "---------------------------------------------------------------------------\n\r"
		    "%s   (%s)\n\r"
		    "---------------------------------------------------------------------------\n\r"
		    "Кланмастер                   : %s\n\r"
		    "Заместитель кланмастера      : %s\n\r"
		    "Описание                     :\n\r%s\n\r\n\r"
		    "Расы, принимаемые в клан     : ",
		    clan_table[i].short_descr,
		    clan_table[i].name,
		    clan_table[i].clanmaster,
		    clan_table[i].clanmaster2,
		    clan_table[i].long_descr);
	    add_buf(output, buf);

	    for (j = 1; j < MAX_PC_RACE; j++)
		if (clan_table[i].races[j])
		{
		    sprintf(buf, "%s ", race_table[j].rname);
		    add_buf(output, buf);
		}

	    add_buf(output, "\n\rПрофессии, принимаемые в клан: ");

	    for (j = 0; j < MAX_CLASS; j++)
		if (clan_table[i].classes[j])
		{
		    sprintf(buf, "%s ", class_table[j].name);
		    add_buf(output, buf);
		}

	    add_buf(output, "\n\r");

	    sprintf(buf, "Разрешенная натура в клане   : от %+d до %+d\n\r"
		    "Количество игроков в клане   : %d\n\r"
		    "С кем враждуют               : ",
		    clan_table[i].min_align,
		    clan_table[i].max_align,
		    clan_table[i].count);

	    add_buf(output, buf);

	    for (j = 0; j < MAX_CLAN; j++)
		if (clan_table[i].clan_war[j])
		{
		    sprintf(buf, "%s ", clan_table[j].name);
		    add_buf(output, buf);
		}

	    add_buf(output, "\n\rС кем в союзе                : ");

	    for (j = 0; j < MAX_CLAN; j++)
		if (clan_table[i].clan_alliance[j])
		{
		    sprintf(buf, "%s ", clan_table[j].name);
		    add_buf(output, buf);
		}

	    add_buf(output, "\n\r");
	    sprintf(buf, "Рейтинг клана                : %d\n\r", clan_table[i].rating);

	    add_buf(output, buf);

	    if (IS_IMMORTAL(ch))
	    {
		sprintf(buf, "Комната возврата (vnum)      : %d\n\r",
			clan_table[i].recall_vnum);
		add_buf(output, buf);
	    }
	}
    }

    if (found == 0)
	add_buf(output, "Кланов не найдено.\n\r");
    else
    {
	sprintf(buf,"\n\rНайдено кланов: %d\n\r",found);
	add_buf(output,buf);
    }

    if (!clan_war_enabled)
	add_buf(output, "\n\rКлановые войны временно приостановлены.\n\r");

    page_to_char(buf_string(output), ch);
    free_buf(output);

    return;
}

void do_clan(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    int i;


    if (IS_NPC(ch))
	return;

    smash_tilde(argument);

    argument = one_argument(argument, buf);

    for (i = 0; !IS_NULLSTR(clan_commands[i].cmd); i++)
    {
	if (!str_prefix(buf, clan_commands[i].cmd))
	{
	    clan_commands[i].func(ch, argument);
	    return;
	}
    }

    send_to_char("Синтаксис: клан <команда> [<аргумент1>] [<аргумент2>]\n\r", ch);
    send_to_char("Список доступных команд:", ch);

    for (i = 0; !IS_NULLSTR(clan_commands[i].cmd); i++)
	printf_to_char(" %s", ch, clan_commands[i].cmd);

    send_to_char("\n\r", ch);
    
    return;
}

bool check_clan_group(CHAR_DATA *ch)
{
    CHAR_DATA *fch;

    if (!is_clan(ch) || IS_INDEPEND(ch))
	return FALSE;

    LIST_FOREACH(fch, &ch->in_room->people, room_link)
	if (!IS_NPC(fch) && is_same_group(ch, fch) && ch->clan != fch->clan)
	    return FALSE;

    return TRUE;
}

/* charset=cp1251 */
