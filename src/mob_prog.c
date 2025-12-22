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
 *	ROM 2.4 is copyright 1993-1995 Russ Taylor			   *
 *	ROM has been brought to you by the ROM consortium		   *
 *	    Russ Taylor (rtaylor@pacinfo.com)				   *
 *	    Gabrielle Taylor (gtaylor@pacinfo.com)			   *
 *	    Brian Moore (rom@rom.efn.org)				   *
 *	By using this code, you have agreed to follow the terms of the	   *
 *	ROM license, in the file Rom24/doc/rom.license			   *
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *  MOBprograms for ROM 2.4 v0.98g (C) M.Nylander 1996                     *
 *  Based on MERC 2.2 MOBprograms concept by N'Atas-ha.                    *
 *  Written and adapted to ROM 2.4 by                                      *
 *          Markku Nylander (markku.nylander@uta.fi)                       *
 *  This code may be copied and distributed as per the ROM license.        *
 *                                                                         *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <regex.h>


//#include <lua.h>

#include "merc.h"
//#include "lua/glua.h"


bool mob_holylight = FALSE;

/*
 * These defines correspond to the entries in fn_keyword[] table.
 * If you add a new if_check, you must also add a #define here.
 */
#define CHK_RAND   	(0)
#define CHK_MOBHERE     (1)
#define CHK_OBJHERE     (2)
#define CHK_MOBEXISTS   (3)
#define CHK_OBJEXISTS   (4)
#define CHK_PEOPLE      (5)
#define CHK_PLAYERS     (6)
#define CHK_MOBS        (7)
#define CHK_CLONES      (8)
#define CHK_ORDER       (9)
#define CHK_HOUR        (10)
#define CHK_ISPC        (11)
#define CHK_ISNPC       (12)
#define CHK_ISGOOD      (13)
#define CHK_ISEVIL      (14)
#define CHK_ISNEUTRAL   (15)
#define CHK_ISIMMORT    (16)
#define CHK_ISCHARM     (17)
#define CHK_ISFOLLOW    (18)
#define CHK_ISACTIVE    (19)
#define CHK_ISDELAY     (20)
#define CHK_ISVISIBLE   (21)
#define CHK_HASTARGET   (22)
#define CHK_ISTARGET    (23)
#define CHK_EXISTS      (24)
#define CHK_AFFECTED    (25)
#define CHK_ACT         (26)
#define CHK_OFF         (27)
#define CHK_IMM         (28)
#define CHK_CARRIES     (29)
#define CHK_WEARS       (30)
#define CHK_HAS         (31)
#define CHK_USES        (32)
#define CHK_NAME        (33)
#define CHK_POS         (34)
#define CHK_CLAN        (35)
#define CHK_RACE        (36)
#define CHK_CLASS       (37)
#define CHK_OBJTYPE     (38)
#define CHK_VNUM        (39)
#define CHK_HPCNT       (40)
#define CHK_ROOM        (41)
#define CHK_SEX         (42)
#define CHK_LEVEL       (43)
#define CHK_ALIGN       (44)
#define CHK_MONEY       (45)
#define CHK_OBJVAL0     (46)
#define CHK_OBJVAL1     (47)
#define CHK_OBJVAL2     (48)
#define CHK_OBJVAL3     (49)
#define CHK_OBJVAL4     (50)
#define CHK_GRPSIZE     (51)
#define CHK_ISKILLER	(52)
#define CHK_ISTHIEF	(53)
#define CHK_STRCMP	(54)
#define CHK_STRPREFIX	(55)
#define CHK_HASSKILL	(56)
#define CHK_ISSTR		(57)
#define CHK_ISINT		(58)
#define CHK_ISWIS		(59)
#define CHK_ISDEX		(60)
#define CHK_ISCON		(61)
#define CHK_CANUSEOBJ		(62)
#define CHK_ISNOPK		(63)

/*
 * These defines correspond to the entries in fn_evals[] table.
 */
#define EVAL_EQ            0
#define EVAL_GE            1
#define EVAL_LE            2
#define EVAL_GT            3
#define EVAL_LT            4
#define EVAL_NE            5


char *someone = "Кто-то";
char *something = "Что-то";
char *someones = "Чье-то";

#define MAX_REGEX_MATCHES 9
char match[MAX_REGEX_MATCHES][MAX_INPUT_LENGTH];

/*
 * if-check keywords:
 */
const char * fn_keyword[] =
{
    "rand",		/* if rand 30		- if random number < 30 */
    "mobhere",		/* if mobhere fido	- is there a 'fido' here */
    "objhere",		/* if objhere bottle	- is there a 'bottle' here */
    /* if mobhere 1233	- is there mob vnum 1233 here */
    /* if objhere 1233	- is there obj vnum 1233 here */
    "mobexists",	/* if mobexists fido	- is there a fido somewhere */
    "objexists",	/* if objexists sword	- is there a sword somewhere */

    "people",		/* if people > 4	- does room contain>4 people */
    "players",		/* if players > 1	- does room contain>1 pcs */
    "mobs",		/* if mobs > 2		- does room contain>2 mobiles */
    "clones",		/* if clones > 3	- are there > 3 mobs */
    /*			  of same vnum here  */
    "order",		/* if order == 0	- is mob the first in room */
    "hour",		/* if hour > 11		- is the time > 11 o'clock */


    "ispc",		/* if ispc $n 		- is $n a pc */
    "isnpc",		/* if isnpc $n 		- is $n a mobile */
    "isgood",		/* if isgood $n 	- is $n good */
    "isevil",		/* if isevil $n 	- is $n evil */
    "isneutral",	/* if isneutral $n 	- is $n neutral */
    "isimmort",		/* if isimmort $n	- is $n immortal */
    "ischarm",		/* if ischarm $n	- is $n charmed */
    "isfollow",		/* if isfollow $n	- is $n following someone */
    "isactive",		/* if isactive $n	- is $n's position > SLEEPING */
    "isdelay",		/* if isdelay $i	- does $i have mprog pending */
    "isvisible",	/* if isvisible $n	- can mob see $n */
    "hastarget",	/* if hastarget $i	- does $i have a valid target */
    "istarget",		/* if istarget $n	- is $n mob's target */
    "exists",		/* if exists $n		- does $n exist somewhere */

    "affected",		/* if affected $n blind - is $n affected by blind */
    "act",		/* if act $i sentinel	- is $i flagged sentinel */
    "off",              /* if off $i berserk	- is $i flagged berserk */
    "imm",              /* if imm $i fire	- is $i immune to fire */
    "carries",		/* if carries $n sword	- does $n have a 'sword' */
    /* if carries $n 1233	- does $n have obj vnum 1233 */
    "wears",		/* if wears $n lantern	- is $n wearing a 'lantern' */
    /* if wears $n 1233	- is $n wearing obj vnum 1233 */
    "has",    		/* if has $n weapon	- does $n have obj */
    /*			  of type weapon */
    "uses",		/* if uses $n armor	- is $n wearing obj */
    /*			  of type armor */
    "name",		/* if name $n puff	- is $n's name 'puff' */
    "pos",		/* if pos $n standing	- is $n standing */
    "clan",		/* if clan $n 'whatever'- does $n belong to */
    /*			  clan 'whatever' */
    "race",		/* if race $n dragon	- is $n of 'dragon' race */
    "class",		/* if class $n mage	- is $n's class 'mage' */
    "objtype",		/* if objtype $p scroll	- is $p a scroll */

    "vnum",		/* if vnum $i == 1233  	- virtual number check */
    "hpcnt",		/* if hpcnt $i > 30	- hit point percent check */
    "room",		/* if room $i == 1233	- room virtual number */
    "sex",		/* if sex $i == 0	- sex check */
    "level",		/* if level $n < 5	- level check */
    "align",		/* if align $n < -1000	- alignment check */
    "money",		/* if money $n */
    "objval0",		/* if objval0 > 1000 	- object value[] checks 0..4 */
    "objval1",
    "objval2",
    "objval3",
    "objval4",
    "grpsize",		/* if grpsize $n > 6	- group size check */
    "iskiller",		/* if iskiller $n	- is $n killer	*/
    "isthief",		/* if isthief $n	- is $n thief	*/
    "str_cmp",		/* if str_cmp $a door	- is $a == "door" */
    "str_prefix",	/* if str_prefix $a foo - is $a == "foo" or "fo" */
    /*			  or even "f"		 */
    "hasskill",		/* if hasskill $n 'axe' - is $n has skilled in axes */
    //статсы чара
    "str",		/* if str $n > 20 - is str of $n > 20 */
    "int",		/* if int $n > 20 - is int of $n > 20 */
    "wis",		/* if wis $n > 20 - is wis of $n > 20 */
    "dex",		/* if dex $n > 20 - is dex of $n > 20 */
    "con",		/* if con $n > 20 - is con of $n > 20 */

    "canuseobj",	/* if canuseobj $n 1233 - if $n can wear 1233 obj */
    "isnopk",	        /* if isnopk $n - if $n no PK char */

    "\n"		/* Table terminator */
};

const char *fn_evals[] =
{
    "==",
    ">=",
    "<=",
    ">",
    "<",
    "!=",
    "\n"
};

bool can_use_obj(CHAR_DATA *ch, int vnum)
{
    OBJ_INDEX_DATA *pTmp; 
    OBJ_DATA *tmp;
    bool flag = FALSE;

    if ((pTmp = get_obj_index(vnum)) == NULL)
    {
	bugf("Can_use_obj: NULL index from vnum %d", vnum);
	return FALSE;
    }

    if (check_prog_limit(pTmp))
	return FALSE;

    if ((tmp = create_object(pTmp, 0)) != NULL)
    {
	if (ch && !is_unusable(ch, tmp) && SUPPRESS_OUTPUT(!is_have_limit(ch, ch, tmp)))
	    flag = TRUE;

	extract_obj(tmp, FALSE, FALSE);
    }

    return flag;
}

/* regexp compare */
bool regex_compare(char *arg, char *tpl)
{
    regex_t expr;
    regmatch_t m[MAX_REGEX_MATCHES + 1];
    int i;
    int err, regerr;
    bool status = FALSE;


    for (i = 0; arg[i] != '\0'; i++);
    arg[i] = UPPER(arg[i]);

    for (i = 0; tpl[i] != '\0'; i++);
    tpl[i] = UPPER(tpl[i]);

    err = regcomp(&expr, tpl, REG_EXTENDED | REG_ICASE);
    
    if (err != 0) {
        char buff[512];
        regerror(err, &expr, buff, sizeof(buff));
	bugf("Кривой REX_EXP шаблон! Err: %s", buff);
    } else {
	regerr = regexec(&expr, arg, MAX_REGEX_MATCHES, m, 0);
	if (regerr == 0 ){
    	    for (i = 0; i < MAX_REGEX_MATCHES && m[i + 1].rm_so > -1; i++){
		int sz = m[i + 1].rm_eo - m[i + 1].rm_so;
		memcpy(match[i], arg + m[i + 1].rm_so, sz);
		match[i][sz] = '\0';
	    }
	    
	    status = TRUE;
	}
    }

    regfree(&expr);

    return status;
}

/*
 * Return a valid keyword from a keyword table
 */
int keyword_lookup(const char **table, char *keyword)
{
    register int i;

    for (i = 0; table[i][0] != '\n'; i++)
	if (!str_cmp(table[i], keyword))
	    return (i);

    return -1;
}

/*
 * Perform numeric evaluation.
 * Called by cmd_eval()
 */
int num_eval(int lval, int oper, int rval)
{
    switch (oper)
    {
    case EVAL_EQ:
	return (lval == rval);
    case EVAL_GE:
	return (lval >= rval);
    case EVAL_LE:
	return (lval <= rval);
    case EVAL_NE:
	return (lval != rval);
    case EVAL_GT:
	return (lval > rval);
    case EVAL_LT:
	return (lval < rval);
    default:
	bugf("num_eval: invalid oper");
	return 0;
    }
}

/*
 * ---------------------------------------------------------------------
 * UTILITY FUNCTIONS USED BY CMD_EVAL()
 * ----------------------------------------------------------------------
 */

/*
 * Get a random PC in the room (for $r parameter)
 */

#define PC_ONLY   1
#define NPC_ONLY  2


CHAR_DATA *get_random_char(CHAR_DATA *mob, OBJ_DATA *obj, ROOM_INDEX_DATA *room, int sel)
{
    CHAR_DATA *vch, *victim = NULL;
    int now = 0, highest = 0;

    if ((mob && obj) || (mob && room) || (obj && room))
    {
	bugf("get_random_char received multiple prog types");
	return NULL;
    }

    if (mob)
    {
	if (mob->in_room)
	    room = mob->in_room;
	else
	    return NULL;
    }
    else if (obj)
    {
	if (obj->in_room)
	    room = obj->in_room;
	else if (obj->carried_by && obj->carried_by->in_room)
	    room = obj->carried_by->in_room;
	else
	    return NULL;
    }
    else if (room == NULL)
	return NULL;

    LIST_FOREACH(vch, &room->people, room_link)
    {
	now = number_percent();

	if (now > highest && 
	    ((IS_SET(sel, PC_ONLY) && !IS_NPC(vch)) || (IS_SET(sel, NPC_ONLY) && IS_NPC(vch))))
	{
	    if (mob)
	    {
		if (mob != vch
		    && can_see(mob, vch))
		{
		    victim = vch;
		    highest = now;
		}
	    }
	    else if (obj)
	    {
		if (obj->carried_by != vch)
		{
		    victim = vch;
		    highest = now;
		}
	    }
	    else 
	    {
		victim = vch;
		highest = now;
	    }
	}
    }

    return victim;
}

/*
 * How many other players / mobs are there in the room
 * iFlag: 0: all, 1: players, 2: mobiles 3: mobs w/ same vnum 4: same group
 */
int count_people_room(CHAR_DATA *mob, OBJ_DATA *obj, ROOM_INDEX_DATA *room, int iFlag)
{
    CHAR_DATA *vch;
    int count = 0;

    if ((mob && obj) || (mob && room) || (obj && room))
    {
	bugf("count_people_room received multiple prog types");
	return 0;
    }

    if (mob)
	room = mob->in_room;
    else if (obj)
    {
	if (obj->in_room)
	    room = obj->in_room;
	else
	    room = obj->carried_by->in_room;
    }
    else if (!room)
    {
	bugf("count_people_room: NULL mob, obj and room");
	return 0;
    }

    LIST_FOREACH(vch, &room->people, room_link)
    {
	if (mob)
	{
	    if (mob != vch
		&& (iFlag == 0
		    || (iFlag == 1 && !IS_NPC(vch))
		    || (iFlag == 2 && IS_NPC(vch))
		    || (iFlag == 3 && IS_NPC(mob) && IS_NPC(vch)
			&& mob->pIndexData->vnum == vch->pIndexData->vnum)
		    || (iFlag == 4 && is_same_group(mob, vch)))
		&& can_see(mob, vch))
	    {
		count++;
	    }
	}
	else if (obj || room)
	{
	    if (iFlag == 0
		|| (iFlag == 1 && !IS_NPC(vch))
		|| (iFlag == 2 && IS_NPC(vch)))
	    {
		count++;
	    }
	}
    }

    return (count);
}

/*
 * Get the order of a mob in the room. Useful when several mobs in
 * a room have the same trigger and you want only the first of them
 * to act
 */
int get_order(CHAR_DATA *ch, OBJ_DATA *obj)
{
    CHAR_DATA *vch;
    OBJ_DATA *vobj;
    ROOM_INDEX_DATA *room = NULL;
    int i = 0;

    if (ch && obj)
    {
	bugf("get_order received multiple prog types");
	return 0;
    }

    if (ch && !IS_NPC(ch))
	return 0;

    if (ch)
    {
	room = ch->in_room;
	vobj = NULL;
    }
    else
    {
	vch = NULL;
	if (obj->in_room)
	    vobj = obj->in_room->contents;
	else if (obj->carried_by->in_room->contents)
	    vobj = obj->carried_by->in_room->contents;
	else
	    vobj = NULL;
    }

    if (ch)
	LIST_FOREACH(vch, &room->people, room_link)
	{
	    if (vch == ch)
		return i;

	    if (IS_NPC(vch)
		&& vch->pIndexData->vnum == ch->pIndexData->vnum)
	    {
		i++;
	    }
	}
    else
	for (i = 0; vobj; vobj = vobj->next_content)
	{
	    if (vobj == obj)
		return i;

	    if (vobj->pIndexData->vnum == obj->pIndexData->vnum)
		i++;
	}

    return 0;
}

/*
 * Check if ch has a given item or item type
 * vnum: item vnum or -1
 * item_type: item type or -1
 * fWear: TRUE: item must be worn, FALSE: don't care
 */
bool has_item(CHAR_DATA *ch, int vnum, int16_t item_type, int16_t wear)
{
    OBJ_DATA *obj, *obj2;

    for (obj = ch->carrying; obj; obj = obj->next_content)
    {
	if ((vnum < 0 || obj->pIndexData->vnum == vnum)
	    && (item_type < 0 || obj->pIndexData->item_type == item_type)
	    && (wear == WEAR_ANYWHERE
		|| obj->wear_loc == wear
		|| (wear == WEAR_WEAR && obj->wear_loc != WEAR_NONE)))
	{
	    return TRUE;
	}

	if (obj->pIndexData->item_type == ITEM_CONTAINER 
	|| obj->pIndexData->item_type == ITEM_CHEST
	|| obj->pIndexData->item_type == ITEM_MORTAR)
	{
	    for (obj2 = obj->contains; obj2; obj2 = obj2->next_content)
	    {
		if ((vnum < 0 || obj2->pIndexData->vnum == vnum)
		    && (item_type < 0 || obj2->pIndexData->item_type == item_type)
		    && (wear == WEAR_ANYWHERE
			|| obj->wear_loc == wear
			|| (wear == WEAR_WEAR && obj->wear_loc != WEAR_NONE)))
		{
		    return TRUE;
		}
	    }
	}
    }
    return FALSE;
}

/*
 * Check if there's a mob with given vnum in the room
 */
bool get_mob_vnum_room(CHAR_DATA *ch, OBJ_DATA *obj, ROOM_INDEX_DATA *room, int vnum)
{
    CHAR_DATA *mob;

    if ((ch && obj) || (ch && room) || (obj && room))
    {
	bugf("get_mob_vnum_room received multiple prog types");
	return FALSE;
    }

    if (ch)
    {
	if (ch->in_room)
	    room = ch->in_room;
    }
    else if (obj)
    {
	if (obj->in_room)
	    room = obj->in_room;
	else
	    room = obj->carried_by->in_room;
    }
    else if (room == NULL)
	return FALSE;

    LIST_FOREACH(mob, &room->people, room_link)
	if (IS_NPC(mob) && mob->pIndexData->vnum == vnum)
	    return TRUE;

    return FALSE;
}

/*
 * Check if there's an object with given vnum in the room
 */
bool get_obj_vnum_room(CHAR_DATA *ch, OBJ_DATA *obj, ROOM_INDEX_DATA *room, int vnum)
{
    OBJ_DATA *vobj;

    if ((ch && obj) || (ch && room) || (obj && room))
    {
	bugf("get_obj_vnum_room received multiple prog types");
	return FALSE;
    }

    if (ch)
	vobj = ch->in_room->contents;
    else if (obj)
    {
	if (obj->in_room)
	    vobj = obj->in_room->contents;
	else
	    vobj = obj->carried_by->in_room->contents;
    }
    else
	vobj = room->contents;

    for (; vobj; vobj = vobj->next_content)
	if (vobj->pIndexData->vnum == vnum)
	    return TRUE;
    return FALSE;
}

/* ---------------------------------------------------------------------
 * CMD_EVAL
 * This monster evaluates an if/or/and statement
 * There are five kinds of statement:
 * 1) keyword and value (no $-code)	    if random 30
 * 2) keyword, comparison and value	    if people > 2
 * 3) keyword and actor		    	    if isnpc $n
 * 4) keyword, actor and value		    if carries $n sword
 * 5) keyword, actor, comparison and value  if level $n >= 10
 *
 *----------------------------------------------------------------------
 */
int cmd_eval_mob(int vnum, char *line, int check,
		 CHAR_DATA *mob, CHAR_DATA *ch,
		 const void *arg1, const void *arg2, CHAR_DATA *rch)
{
    CHAR_DATA *lval_char = mob;
    CHAR_DATA *vch = (CHAR_DATA *) arg2;
    OBJ_DATA *obj1 = (OBJ_DATA  *) arg1;
    OBJ_DATA *obj2 = (OBJ_DATA  *) arg2;
    OBJ_DATA  *lval_obj = NULL;
    ROOM_INDEX_DATA *room;

    char *original, buf[MAX_INPUT_LENGTH], buf1[MIL], code;
    int lval = 0, oper = 0, rval = -1;
    bool rf;

    original = line;
    line = one_argument(line, buf);

    if (!strcmp(buf, "$a"))
	strcpy(buf, (char *) arg1);

    if (!strcmp(buf, "$A"))
	strcpy(buf, (char *) arg2); 

    if (buf[0] == '\0' || mob == NULL)
	return FALSE;

    /*
     * If this mobile has no target, let's assume our victim is the one
     */
    if (mob->mprog_target == NULL)
	mob->mprog_target = ch;

    switch (check)
    {
	/*
	 * Case 1: keyword and value
	 */
    case CHK_RAND:
	return (atoi(buf) < number_percent());
	break;

    case CHK_MOBHERE:
	if (is_number(buf))
	    return (get_mob_vnum_room(mob, NULL, NULL, atoi(buf)));
	else
	    return ((bool) (get_char_room(mob, NULL, buf, FALSE) != NULL));
	break;

    case CHK_OBJHERE:
	if (is_number(buf))
	    return (get_obj_vnum_room(mob, NULL, NULL, atoi(buf)));
	else
	    return ((bool) (get_obj_here(mob, NULL, buf) != NULL));
	break;

    case CHK_MOBEXISTS:
	return ((bool) (get_char_world(mob, buf) != NULL));

    case CHK_OBJEXISTS:
	return ((bool) (get_obj_world(mob, buf) != NULL));

    case CHK_STRCMP:
	one_argument(line, buf1);
	return (!str_cmp(buf, buf1));

    case CHK_STRPREFIX:
	one_argument(line, buf1);
	return (!str_prefix(buf, buf1));

	/*
	 * Case 2 begins here: We sneakily use rval to indicate need
	 * 		       for numeric eval...
	 */
    case CHK_PEOPLE:
	rval = count_people_room(mob, NULL, NULL, 0);
	break;

    case CHK_PLAYERS:
	rval = count_people_room(mob, NULL, NULL, 1);
	break;

    case CHK_MOBS:
	rval = count_people_room(mob, NULL, NULL, 2);
	break;

    case CHK_CLONES:
	rval = count_people_room(mob, NULL, NULL, 3);
	break;

    case CHK_ORDER:
	rval = get_order(mob, NULL);
	break;

    case CHK_HOUR:
	rval = time_info.hour;
	break;

    default:
	break;
    }

    /*
     * Case 2 continued: evaluate expression
     */
    if (rval >= 0)
    {
	if ((oper = keyword_lookup(fn_evals, buf)) < 0)
	{
	    bugf("Cmd_eval_mob: prog %d syntax error(2) '%s'",
		 vnum, original);
	    return FALSE;
	}

	one_argument(line, buf);
	lval = rval;
	rval = atoi(buf);
	return (num_eval(lval, oper, rval));
    }

    /*
     * Case 3,4,5: Grab actors from $* codes
     */
    if (buf[0] != '$' || buf[1] == '\0')
    {
	bugf("Cmd_eval_mob: prog %d syntax error(3) '%s'",
	     vnum, original);
	return FALSE;
    }
    else
	code = buf[1];

    switch (code)
    {
    case 'i':
	lval_char = mob;
	break;
    case 'n':
	lval_char = ch;
	break;
    case 't':
	lval_char = vch;
	break;
    case 'r':
	lval_char = (rch == NULL) ? get_random_char(mob, NULL, NULL, PC_ONLY|NPC_ONLY) : rch;
	break;
    case 'b':
	lval_char = (rch == NULL) ? get_random_char(mob, NULL, NULL, PC_ONLY) : rch;
	break;
    case 'c':
	lval_char = (rch == NULL) ? get_random_char(mob, NULL, NULL, NPC_ONLY) : rch;
	break;
    case 'o':
	lval_obj = obj1;
	break;
    case 'p':
	lval_obj = obj2;
	break;
    case 'q':
	lval_char = mob->mprog_target;
	break;
    default:
	bugf("Cmd_eval_mob: prog %d syntax error(4) '%s'",
	     vnum, original);
	return FALSE;
    }
    /*
     * From now on, we need an actor, so if none was found, bail out
     */
    if (lval_char == NULL && lval_obj == NULL)
	return FALSE;

    /*
     * Case 3: Keyword, comparison and value
     */
    switch (check)
    {
    case CHK_ISPC:
	return (lval_char != NULL && !IS_NPC(lval_char));
	break;

    case CHK_ISNPC:
	return (lval_char != NULL && IS_NPC(lval_char));
	break;

    case CHK_ISGOOD:
	return (lval_char != NULL && IS_GOOD(lval_char));
	break;

    case CHK_ISEVIL:
	return (lval_char != NULL && IS_EVIL(lval_char));
	break;

    case CHK_ISNEUTRAL:
	return (lval_char != NULL && IS_NEUTRAL(lval_char));
	break;

    case CHK_ISIMMORT:
	return (lval_char != NULL && IS_IMMORTAL(lval_char));
	break;

    case CHK_ISCHARM: /* A relic from MERC 2.2 MOBprograms */
	return (lval_char != NULL && IS_AFFECTED(lval_char, AFF_CHARM));
	break;

    case CHK_ISFOLLOW:
	return (lval_char != NULL && lval_char->master != NULL
		&& lval_char->master->in_room == lval_char->in_room);
	break;

    case CHK_ISACTIVE:
	return (lval_char != NULL && lval_char->position > POS_SLEEPING);
	break;

    case CHK_ISDELAY:
	return (lval_char != NULL && lval_char->mprog_delay > 0);
	break;

    case CHK_ISVISIBLE:
	switch (code)
	{
	default :
	case 'i':
	case 'n':
	case 't':
	case 'r':
	case 'b':
	case 'c':
	case 'q':
	    return (lval_char != NULL && can_see(mob, lval_char));
	case 'o':
	case 'p':
	    return (lval_obj != NULL && can_see_obj(mob, lval_obj));
	}
	break;

    case CHK_HASTARGET:
	return (lval_char != NULL && lval_char->mprog_target != NULL
		&&  lval_char->in_room == lval_char->mprog_target->in_room); 
	/*	    return (lval_char != NULL && is_memory(lval_char, NULL, MEM_MOBPROG)); */

	break;

    case CHK_ISTARGET:
	/*	    return (lval_char != NULL && mob->mprog_target == lval_char); */
	if (lval_char != NULL && is_memory(mob, lval_char, MEM_MOBPROG))
	{
	    mob->mprog_target = lval_char;
	    return TRUE;
	}
	else
	    return FALSE;
	break;

    case CHK_ISKILLER:
	return (lval_char && !IS_NPC(lval_char)
		&& IS_SET(lval_char->act, PLR_KILLER));
	break;

    case CHK_ISTHIEF:
	return (lval_char && !IS_NPC(lval_char)
		&& IS_SET(lval_char->act, PLR_THIEF));
	break;

    case CHK_ISNOPK:
	return (lval_char && is_nopk(lval_char));
	break;

    default:
	break;
    }

    /*
     * Case 4: Keyword, actor and value
     */
    line = one_argument(line, buf);
    switch (check)
    {
    case CHK_AFFECTED:
	return (lval_char != NULL
		&& IS_SET(lval_char->affected_by,
			  flag_lookup(buf, affect_flags)));
	break;

    case CHK_ACT:
	return (lval_char != NULL
		&& IS_SET(lval_char->act, flag_lookup(buf, act_flags)));
	break;

    case CHK_IMM:
	return (lval_char != NULL
		&& check_immune(lval_char,
				flag_lookup(buf, dam_flags)) == 100);
	break;

    case CHK_OFF:
	return (lval_char != NULL
		&& IS_SET(lval_char->off_flags,
			  flag_lookup(buf, off_flags)));
	break;

    case CHK_CARRIES:
	if (is_number(buf))
	{
	    return (lval_char != NULL && can_see(mob, lval_char)
		    && has_item(lval_char, atoi(buf), -1, WEAR_ANYWHERE));
	}
	else
	{
	    return (lval_char != NULL && can_see(mob, lval_char)
		    && (get_obj_carry(lval_char, buf, lval_char) != NULL));
	}
	break;

    case CHK_WEARS:
	if (is_number(buf))
	{
	    return (lval_char != NULL
		    && has_item(lval_char, atoi(buf), -1, WEAR_WEAR));
	}
	else
	{
	    return (lval_char != NULL
		    && (get_obj_wear(lval_char, buf, TRUE) != NULL));
	}
	break;

    case CHK_HAS:
	return (lval_char != NULL
		&& has_item(lval_char, -1, item_lookup(buf), WEAR_ANYWHERE));
	break;

    case CHK_USES:

	return (lval_char != NULL
		&& has_item(lval_char, -1, item_lookup(buf), wear_loc_lookup(buf)));
	break;

    case CHK_NAME:
	switch (code)
	{
	default :
	case 'i':
	case 'n':
	case 't':
	case 'r':
	case 'q':
	case 'b':
	case 'c':
	    return (lval_char != NULL && is_name(buf, lval_char->name));
	case 'o':
	case 'p':
	    return (lval_obj != NULL && is_name(buf, lval_obj->name));
	}
	break;

    case CHK_POS:
	return (lval_char != NULL
		&& lval_char->position == position_lookup(buf));
	break;

    case CHK_CLAN:
	return (lval_char != NULL
		&& is_clan(lval_char) == clan_lookup(buf));
	break;

    case CHK_RACE:
	return (lval_char != NULL && lval_char->race == race_lookup(buf));
	break;

    case CHK_CLASS:
	return (lval_char != NULL && lval_char->classid == class_lookup(buf));
	break;
    case CHK_OBJTYPE:
	return (lval_obj != NULL
		&& lval_obj->item_type == item_lookup(buf));
	break;

    case CHK_HASSKILL:
	return (lval_char && get_skill(lval_char, skill_lookup(buf)) > 0);
	break;

    case CHK_CANUSEOBJ:

	if (!is_number(buf))
	{
	    bugf("Cmd_eval_mob: prog %d canuseobj argument is not vnum",
		 vnum);
	    return FALSE;
	}

	return (can_use_obj(lval_char, atoi(buf)));

    case CHK_ROOM:

	if (keyword_lookup(fn_evals, buf) >= 0)
	    break;

	if ((oper = flag_lookup(buf, room_flags)) != NO_FLAG)
	    rf = TRUE;
	else if ((oper = flag_lookup(buf, sector_flags)) != NO_FLAG)
	    rf = FALSE;
	else
	{
	    bugf("Cmd_eval_mob: prog %d unknown room flag '%s'",
		 vnum, original);
	    return FALSE;
	}


	room = lval_char && lval_char->in_room ? lval_char->in_room : 
	    lval_obj && lval_obj->in_room ? lval_obj->in_room :
	    lval_obj && lval_obj->carried_by && lval_obj->carried_by->in_room ? lval_obj->carried_by->in_room :
	    NULL;

	if (room && ((rf && IS_SET(room->room_flags, oper))
		     || (!rf && room->sector_type == oper)))
	    return TRUE;
	else
	    return FALSE;

    default:
	break;
    }

    /*
     * Case 5: Keyword, actor, comparison and value
     */
    if ((oper = keyword_lookup(fn_evals, buf)) < 0)
    {
	bugf("Cmd_eval_mob: prog %d syntax error(5): '%s'",
	     vnum, original);

	return FALSE;
    }
    one_argument(line, buf);
    rval = atoi(buf);

    switch (check)
    {
    case CHK_VNUM:
	switch (code)
	{
	default :
	case 'i':
	case 'n':
	case 't':
	case 'r':
	case 'q':
	case 'b':
	case 'c':
	    if(lval_char != NULL && IS_NPC(lval_char))
		lval = lval_char->pIndexData->vnum;
	    break;
	case 'o':
	case 'p':
	    if (lval_obj != NULL)
		lval = lval_obj->pIndexData->vnum;
	}
	break;

    case CHK_HPCNT:
	if (lval_char != NULL)
	    lval = (lval_char->hit * 100) / (UMAX(1, lval_char->max_hit));
	break;

    case CHK_ROOM:
	if (lval_char != NULL && lval_char->in_room != NULL)
	    lval = lval_char->in_room->vnum;
	break;

    case CHK_SEX:
	if (lval_char != NULL)
	    lval = lval_char->sex;
	break;

    case CHK_LEVEL:
	if (lval_char != NULL)
	    lval = lval_char->level;
	break;

    case CHK_ALIGN:
	if (lval_char != NULL)
	    lval = lval_char->alignment;
	break;

    case CHK_MONEY:  /* Money is converted to silver... */
	if (lval_char != NULL)
	    lval = lval_char->silver + (lval_char->gold * 100);
	break;

    case CHK_OBJVAL0:
	if (lval_obj != NULL)
	    lval = lval_obj->value[0];
	break;

    case CHK_OBJVAL1:
	if (lval_obj != NULL)
	    lval = lval_obj->value[1];
	break;

    case CHK_OBJVAL2:
	if (lval_obj != NULL)
	    lval = lval_obj->value[2];
	break;

    case CHK_OBJVAL3:
	if (lval_obj != NULL)
	    lval = lval_obj->value[3];
	break;

    case CHK_OBJVAL4:
	if (lval_obj != NULL)
	    lval = lval_obj->value[4];
	break;

    case CHK_GRPSIZE:
	if (lval_char != NULL)
	    lval = count_people_room(lval_char, NULL, NULL, 4);
	break;

	//проверки на статсы
    case CHK_ISSTR:
	if (lval_char != NULL)
	    lval = get_curr_stat(lval_char, STAT_STR);
	break;

    case CHK_ISINT:
	if (lval_char != NULL)
	    lval = get_curr_stat(lval_char, STAT_INT);
	break;

    case CHK_ISWIS:
	if (lval_char != NULL)
	    lval = get_curr_stat(lval_char, STAT_WIS);
	break;

    case CHK_ISDEX:
	if (lval_char != NULL)
	    lval = get_curr_stat(lval_char, STAT_DEX);
	break;

    case CHK_ISCON:
	if (lval_char != NULL)
	    lval = get_curr_stat(lval_char, STAT_CON);
	break;

    default:
	return FALSE;
    }
    return (num_eval(lval, oper, rval));
}

int cmd_eval_obj(int vnum, char *line, int check,
		 OBJ_DATA *obj, CHAR_DATA *ch,
		 const void *arg1, const void *arg2, CHAR_DATA *rch)
{
    CHAR_DATA *lval_char = NULL;
    CHAR_DATA *vch = (CHAR_DATA *) arg2;
    OBJ_DATA *obj1 = (OBJ_DATA  *) arg1;
    OBJ_DATA *obj2 = (OBJ_DATA  *) arg2;
    OBJ_DATA *lval_obj = obj;
    ROOM_INDEX_DATA *room;

    char *lval_arg1 = (char *)arg1;
    char *lval_arg2 = (char *)arg2;

    char *original, buf[MAX_INPUT_LENGTH], buf1[MSL], code;
    int lval = 0, oper = 0, rval = -1;
    bool rf;

    original = line;

    line = one_argument(line, buf);

    if (!strcmp(buf, "$a"))
	strcpy(buf, lval_arg1);

    if (!strcmp(buf, "$A"))
	strcpy(buf, lval_arg2); 

    if (buf[0] == '\0' || obj == NULL)
	return FALSE;

    /*
     * If this object has no target, let's assume our victim is the one
     */
    if (obj->oprog_target == NULL)
	obj->oprog_target = ch;

    switch (check)
    {
	/*
	 * Case 1: keyword and value
	 */
    case CHK_RAND:
	return (atoi(buf) < number_percent());
	break;

    case CHK_MOBHERE:
	if (is_number(buf))
	    return (get_mob_vnum_room(NULL, obj, NULL,  atoi(buf)));
	else
	    return (get_char_room(NULL,
				  (obj->in_room ? obj->in_room
				   : obj->carried_by->in_room),
				  buf, FALSE) != NULL);
	break;

    case CHK_OBJHERE:
	if (is_number(buf))
	    return (get_obj_vnum_room(NULL, obj, NULL,  atoi(buf)));
	else
	    return (get_obj_here(NULL,
				 (obj->in_room ? obj->in_room
				  : obj->carried_by->in_room),
				 buf) != NULL);
	break;

    case CHK_MOBEXISTS:
        return ((bool) (get_char_world(NULL, buf) != NULL)); 

    case CHK_OBJEXISTS:
	return (get_obj_world(NULL, buf) != NULL);

    case CHK_STRCMP:
	one_argument(line, buf1);
	return (!str_cmp(buf, buf1));

    case CHK_STRPREFIX:
	one_argument(line, buf1);
	return (!str_prefix(buf, buf1));

	/*
	 * Case 2 begins here: We sneakily use rval to indicate need
	 * 		       for numeric eval...
	 */
    case CHK_PEOPLE:
	rval = count_people_room(NULL, obj, NULL, 0);
	break;

    case CHK_PLAYERS:
	rval = count_people_room(NULL, obj, NULL, 1);
	break;

    case CHK_MOBS:
	rval = count_people_room(NULL, obj, NULL, 2);
	break;

    case CHK_CLONES:
	bugf("cmd_eval_obj: received CHK_CLONES.");
	break;

    case CHK_ORDER:
	rval = get_order(NULL, obj);
	break;

    case CHK_HOUR:
	rval = time_info.hour;
	break;

    default:
	break;
    }

    /*
     * Case 2 continued: evaluate expression
     */
    if (rval >= 0)
    {
	if ((oper = keyword_lookup(fn_evals, buf)) < 0)
	{
	    bugf("Cmd_eval_obj: prog %d syntax error(2) '%s'",
		 vnum, original);
	    return FALSE;
	}

	one_argument(line, buf);
	lval = rval;
	rval = atoi(buf);
	return (num_eval(lval, oper, rval));
    }

    /*
     * Case 3,4,5: Grab actors from $* codes
     */
    if (buf[0] != '$' || buf[1] == '\0')
    {
	bugf("Cmd_eval_obj: prog %d syntax error(3) '%s'",
	     vnum, original);
	return FALSE;
    }
    else
	code = buf[1];
    switch (code)
    {
    case 'i':
	lval_obj = obj;
	break;
    case 'n':
	lval_char = ch;
	break;
    case 't':
	lval_char = vch;
	break;
    case 'r':
	lval_char = rch == NULL ? get_random_char(NULL, obj, NULL, PC_ONLY|NPC_ONLY) : rch ;
	break;
    case 'b':
	lval_char = (rch == NULL) ? get_random_char(NULL, obj, NULL, PC_ONLY) : rch;
	break;
    case 'c':
	lval_char = (rch == NULL) ? get_random_char(NULL, obj, NULL, NPC_ONLY) : rch;
	break;
    case 'w':
	lval_char = obj->carried_by;
	break;
    case 'o':
	lval_obj = obj1;
	break;
    case 'p':
	lval_obj = obj2;
	break;
    case 'q':
	lval_char = obj->oprog_target;
	break;
    default:
	bugf("Cmd_eval_obj: prog %d syntax error(4) '%s'",
	     vnum, original);
	return FALSE;
    }
    /*
     * From now on, we need an actor, so if none was found, bail out
     */
    if (lval_char == NULL && lval_obj == NULL)
	return FALSE;

    /*
     * Case 3: Keyword, comparison and value
     */
    switch (check)
    {
    case CHK_ISPC:
	return (lval_char != NULL && !IS_NPC(lval_char));
	break;

    case CHK_ISNPC:
	return (lval_char != NULL && IS_NPC(lval_char));
	break;

    case CHK_ISGOOD:
	return (lval_char != NULL && IS_GOOD(lval_char));
	break;

    case CHK_ISEVIL:
	return (lval_char != NULL && IS_EVIL(lval_char));
	break;

    case CHK_ISNEUTRAL:
	return (lval_char != NULL && IS_NEUTRAL(lval_char));
	break;

    case CHK_ISIMMORT:
	return (lval_char != NULL && IS_IMMORTAL(lval_char));
	break;

    case CHK_ISCHARM: /* A relic from MERC 2.2 MOBprograms */
	return (lval_char != NULL && IS_AFFECTED(lval_char, AFF_CHARM));
	break;

    case CHK_ISFOLLOW:
	return (lval_char != NULL && lval_char->master != NULL
		&& lval_char->master->in_room == lval_char->in_room);
	break;

    case CHK_ISACTIVE:
	return (lval_char != NULL && lval_char->position > POS_SLEEPING);
	break;

    case CHK_ISDELAY:
	return (lval_char != NULL && lval_char->mprog_delay > 0);
	break;

    case CHK_HASTARGET:
	return (lval_char != NULL && lval_char->mprog_target != NULL
		&& lval_char->in_room == lval_char->mprog_target->in_room);
	break;

    case CHK_ISTARGET:
	return (lval_char != NULL && obj->oprog_target == lval_char);
	break;

    case CHK_ISKILLER:
	return (lval_char && !IS_NPC(lval_char)
		&& IS_SET(lval_char->act, PLR_KILLER));
	break;

    case CHK_ISTHIEF:
	return (lval_char && !IS_NPC(lval_char)
		&& IS_SET(lval_char->act, PLR_THIEF));
	break;

    case CHK_ISNOPK:
	return (lval_char && is_nopk(lval_char));
	break;

    default:
	break;
    }

    /*
     * Case 4: Keyword, actor and value
     */
    line = one_argument(line, buf);

    switch (check)
    {
    case CHK_AFFECTED:
	return (lval_char != NULL
		&& IS_SET(lval_char->affected_by,
			  flag_lookup(buf, affect_flags)));
	break;

    case CHK_ACT:
	return (lval_char != NULL
		&& IS_SET(lval_char->act, flag_lookup(buf, act_flags)));
	break;

    case CHK_IMM:
	return (lval_char != NULL
		&& check_immune(lval_char,
				flag_lookup(buf, dam_flags)) == 100);
	break;

    case CHK_OFF:
	return (lval_char != NULL
		&& IS_SET(lval_char->off_flags,
			  flag_lookup(buf, off_flags)));
	break;

    case CHK_CARRIES:
	if (is_number(buf))
	    return (lval_char != NULL
		    && has_item(lval_char, atoi(buf), -1, WEAR_ANYWHERE));
	else
	    return (lval_char != NULL
		    && (get_obj_carry(lval_char, buf, lval_char) != NULL));

	break;

    case CHK_WEARS:
	if (is_number(buf))
	    return (lval_char != NULL
		    && has_item(lval_char, atoi(buf), -1, WEAR_WEAR));
	else
	    return (lval_char != NULL
		    && (get_obj_wear(lval_char, buf, FALSE) != NULL));
	break;

    case CHK_HAS:
	return (lval_char != NULL
		&& has_item(lval_char, -1, item_lookup(buf), WEAR_ANYWHERE));
	break;

    case CHK_USES:
	return (lval_char != NULL
		&& has_item(lval_char, -1, item_lookup(buf), wear_loc_lookup(buf)));
	break;

    case CHK_NAME:
	switch (code)
	{
	default :
	case 'n':
	case 't':
	case 'r':
	case 'q':
	case 'b':
	case 'c':
	    return (lval_char != NULL && is_name(buf, lval_char->name));
	case 'i':
	case 'o':
	case 'p':
	    return (lval_obj != NULL && is_name(buf, lval_obj->name));
	}
	break;

    case CHK_POS:
	return (lval_char != NULL
		&& lval_char->position == position_lookup(buf));
	break;

    case CHK_CLAN:
	return (lval_char != NULL
		&& is_clan(lval_char) == clan_lookup(buf));
	break;

    case CHK_RACE:
	return (lval_char != NULL && lval_char->race == race_lookup(buf));
	break;

    case CHK_CLASS:
	return (lval_char != NULL && lval_char->classid == class_lookup(buf));
	break;

    case CHK_OBJTYPE:
	return (lval_obj != NULL
		&& lval_obj->item_type == item_lookup(buf));
	break;

    case CHK_CANUSEOBJ:

	if (!is_number(buf))
	{
	    bugf("Cmd_eval_obj: prog %d canuseobj argument is not vnum",
		 vnum);
	    return FALSE;
	}

	return (can_use_obj(lval_char, atoi(buf)));

    case CHK_ROOM:

	if (keyword_lookup(fn_evals, buf) >= 0)
	    break;

	if ((oper = flag_lookup(buf, room_flags)) != NO_FLAG)
	    rf = TRUE;
	else if ((oper = flag_lookup(buf, sector_flags)) != NO_FLAG)
	    rf = FALSE;
	else
	{
	    bugf("Cmd_eval_obj: prog %d unknown room flag '%s'",
		 vnum, original);
	    return FALSE;
	}

	room = lval_char && lval_char->in_room ? lval_char->in_room : 
	    lval_obj && lval_obj->in_room ? lval_obj->in_room :
	    lval_obj && lval_obj->carried_by && lval_obj->carried_by->in_room ? lval_obj->carried_by->in_room :
	    NULL;

	if (room && ((rf && IS_SET(room->room_flags, oper))
		     || (!rf && room->sector_type == oper)))
	    return TRUE;
	else
	    return FALSE;

    default:
	break;
    }

    /*
     * Case 5: Keyword, actor, comparison and value
     */
    if ((oper = keyword_lookup(fn_evals, buf)) < 0)
    {
	bugf("Cmd_eval_obj: prog %d syntax error(5): '%s'",
	     vnum, original);
	return FALSE;
    }

    one_argument(line, buf);
    rval = atoi(buf);

    switch (check)
    {
    case CHK_VNUM:
	switch (code)
	{
	default :
	case 'n':
	case 't':
	case 'r':
	case 'q':
	case 'b':
	case 'c':
	    if (lval_char != NULL && IS_NPC(lval_char))
		lval = lval_char->pIndexData->vnum;
	    break;
	case 'i':
	case 'o':
	case 'p':
	    if (lval_obj != NULL)
		lval = lval_obj->pIndexData->vnum;
	}
	break;
    case CHK_HPCNT:
	if (lval_char != NULL)
	    lval = (lval_char->hit * 100) / (UMAX(1, lval_char->max_hit));
	break;

    case CHK_ROOM:
	if (lval_char != NULL && lval_char->in_room != NULL)
	    lval = lval_char->in_room->vnum;
	else if (lval_obj && (lval_obj->in_room || lval_obj->carried_by ))
	    lval = lval_obj->in_room ? lval_obj->in_room->vnum
		: lval_obj->carried_by->in_room->vnum;
	break;

    case CHK_SEX:
	if (lval_char != NULL)
	    lval = lval_char->sex;
	break;

    case CHK_LEVEL:
	if (lval_char != NULL)
	    lval = lval_char->level;
	break;

    case CHK_ALIGN:
	if (lval_char != NULL)
	    lval = lval_char->alignment;
	break;

    case CHK_MONEY:  /* Money is converted to silver... */
	if (lval_char != NULL)
	    lval = lval_char->gold + (lval_char->silver * 100);
	break;

    case CHK_OBJVAL0:
	if (lval_obj != NULL)
	    lval = lval_obj->value[0];
	break;

    case CHK_OBJVAL1:
	if (lval_obj != NULL)
	    lval = lval_obj->value[1];
	break;

    case CHK_OBJVAL2:
	if (lval_obj != NULL)
	    lval = lval_obj->value[2];
	break;

    case CHK_OBJVAL3:
	if (lval_obj != NULL)
	    lval = lval_obj->value[3];
	break;

    case CHK_OBJVAL4:
	if (lval_obj != NULL)
	    lval = lval_obj->value[4];
	break;

    case CHK_GRPSIZE:
	if(lval_char != NULL)
	    lval = count_people_room(lval_char, NULL, NULL, 4);
	break;

	/* проверки на статсы */
    case CHK_ISSTR:
	if (lval_char != NULL)
	    lval = get_curr_stat(lval_char, STAT_STR);
	break;

    case CHK_ISINT:
	if (lval_char != NULL)
	    lval = get_curr_stat(lval_char, STAT_INT);
	break;

    case CHK_ISWIS:
	if (lval_char != NULL)
	    lval = get_curr_stat(lval_char, STAT_WIS);
	break;

    case CHK_ISDEX:
	if (lval_char != NULL)
	    lval = get_curr_stat(lval_char, STAT_DEX);
	break;

    case CHK_ISCON:
	if (lval_char != NULL)
	    lval = get_curr_stat(lval_char, STAT_CON);
	break;


    default:
	return FALSE;
    }
    return (num_eval(lval, oper, rval));
}

int cmd_eval_room(int vnum, char *line, int check,
		  ROOM_INDEX_DATA *room, CHAR_DATA *ch,
		  const void *arg1, const void *arg2, CHAR_DATA *rch)
{
    CHAR_DATA *lval_char = NULL;
    CHAR_DATA *vch = (CHAR_DATA *) arg2;
    OBJ_DATA *obj1 = (OBJ_DATA  *) arg1;
    OBJ_DATA *obj2 = (OBJ_DATA  *) arg2;
    OBJ_DATA *lval_obj = NULL;
    ROOM_INDEX_DATA *in_room;

    char *lval_arg1 = (char *) arg1;
    char *lval_arg2 = (char *) arg2;

    char *original, buf[MAX_INPUT_LENGTH], buf1[MIL], code;
    int lval = 0, oper = 0, rval = -1;
    bool rf;

    original = line;
    line = one_argument(line, buf);

    if (!strcmp(buf, "$a"))
	strcpy(buf, lval_arg1);

    if (!strcmp(buf, "$A"))
	strcpy(buf, lval_arg2);

    if (buf[0] == '\0' || room == NULL)
	return FALSE;

    /*
     * If this room has no target, let's assume our victim is the one
     */
    if (room->rprog_target == NULL)
	room->rprog_target = ch;

    switch (check)
    {
	/*
	 * Case 1: keyword and value
	 */
    case CHK_RAND:
	return (atoi(buf) < number_percent());

    case CHK_MOBHERE:
	if (is_number(buf))
	    return (get_mob_vnum_room(NULL, NULL, room,  atoi(buf)));
	else
	    return ((bool) (get_char_room(NULL, room, buf, FALSE) != NULL));

    case CHK_OBJHERE:
	if (is_number(buf))
	    return (get_obj_vnum_room(NULL, NULL, room,  atoi(buf)));
	else
	    return ((bool) (get_obj_here(NULL, room, buf) != NULL));

    case CHK_MOBEXISTS:
        return ((bool) (get_char_world(NULL, buf) != NULL));

    case CHK_OBJEXISTS:
	return ((bool) (get_obj_world(NULL, buf) != NULL));

	/*
	 * Case 2 begins here: We sneakily use rval to indicate need
	 * 		       for numeric eval...
	 */
    case CHK_PEOPLE:
	rval = count_people_room(NULL, NULL, room, 0); break;

    case CHK_PLAYERS:
	rval = count_people_room(NULL, NULL, room, 1); break;

    case CHK_MOBS:
	rval = count_people_room(NULL, NULL, room, 2); break;

    case CHK_CLONES:
	bugf("Cmd_eval_room: received CHK_CLONES."); break;

    case CHK_ORDER:
	bugf("Cmd_eval_room: received CHK_ORDER."); break;

    case CHK_HOUR:
	rval = time_info.hour;
	break;
    case CHK_STRCMP:
	one_argument(line, buf1);
	return (!str_cmp(buf, buf1));
	break;

    case CHK_STRPREFIX:
	one_argument(line, buf1);
	return (!str_prefix(buf, buf1));
	break;

    default:
	break;
    }

    /*
     * Case 2 continued: evaluate expression
     */
    if (rval >= 0)
    {
	if ((oper = keyword_lookup(fn_evals, buf)) < 0)
	{
	    bugf("Cmd_eval_room: prog %d syntax error(2) '%s'",
		 vnum, original);
	    return FALSE;
	}
	one_argument(line, buf);
	lval = rval;
	rval = atoi(buf);
	return (num_eval(lval, oper, rval));
    }

    /*
     * Case 3,4,5: Grab actors from $* codes
     */
    if (buf[0] != '$' || buf[1] == '\0')
    {
	bugf("Cmd_eval_room: prog %d syntax error(3) '%s'",
	     vnum, original);
	return FALSE;
    }
    else
	code = buf[1];
    switch (code)
    {
    case 'i':
	bugf("Cmd_eval_room: received code case 'i'.");
	break;
    case 'n':
	lval_char = ch;
	break;
    case 't':
	lval_char = vch;
	break;
    case 'r':
	lval_char = rch == NULL ? get_random_char(NULL, NULL, room, PC_ONLY|NPC_ONLY) : rch;
	break;
    case 'b':
	lval_char = (rch == NULL) ? get_random_char(NULL, NULL, room, PC_ONLY) : rch;
	break;
    case 'c':
	lval_char = (rch == NULL) ? get_random_char(NULL, NULL, room, NPC_ONLY) : rch;
	break;

    case 'o':
	lval_obj = obj1;
	break;
    case 'p':
	lval_obj = obj2;
	break;
    case 'q':
	lval_char = room->rprog_target;
	break;
    default:
	bugf("Cmd_eval_room: prog %d syntax error(4) '%s'",
	     vnum, original);
	return FALSE;
    }
    /*
     * From now on, we need an actor, so if none was found, bail out
     */
    if (lval_char == NULL && lval_obj == NULL)
	return FALSE;

    /*
     * Case 3: Keyword, comparison and value
     */
    switch (check)
    {
    case CHK_ISPC:
	return (lval_char != NULL && !IS_NPC(lval_char));

    case CHK_ISNPC:
	return (lval_char != NULL && IS_NPC(lval_char));

    case CHK_ISGOOD:
	return (lval_char != NULL && IS_GOOD(lval_char));

    case CHK_ISEVIL:
	return (lval_char != NULL && IS_EVIL(lval_char));

    case CHK_ISNEUTRAL:
	return (lval_char != NULL && IS_NEUTRAL(lval_char));

    case CHK_ISIMMORT:
	return (lval_char != NULL && IS_IMMORTAL(lval_char));

    case CHK_ISCHARM: /* A relic from MERC 2.2 MOBprograms */
	return (lval_char != NULL && IS_AFFECTED(lval_char, AFF_CHARM));

    case CHK_ISFOLLOW:
	return (lval_char != NULL && lval_char->master != NULL
		&& lval_char->master->in_room == lval_char->in_room);

    case CHK_ISACTIVE:
	return (lval_char != NULL && lval_char->position > POS_SLEEPING);

    case CHK_ISDELAY:
	return (lval_char != NULL && lval_char->mprog_delay > 0);

    case CHK_HASTARGET:
	return (lval_char != NULL && lval_char->mprog_target != NULL
		&& lval_char->in_room == lval_char->mprog_target->in_room);

    case CHK_ISTARGET:
	return (lval_char != NULL && room->rprog_target == lval_char);

    case CHK_ISKILLER:
	return (lval_char && !IS_NPC(lval_char)
		&& IS_SET(lval_char->act, PLR_KILLER));
	break;

    case CHK_ISTHIEF:
	return (lval_char && !IS_NPC(lval_char)
		&& IS_SET(lval_char->act, PLR_THIEF));
	break;

    case CHK_ISNOPK:
	return (lval_char && is_nopk(lval_char));
	break;

    default:;
    }

    /*
     * Case 4: Keyword, actor and value
     */
    line = one_argument(line, buf);
    switch (check)
    {
    case CHK_AFFECTED:
	return (lval_char != NULL
		&& IS_SET(lval_char->affected_by,
			  flag_lookup(buf, affect_flags)));

    case CHK_ACT:
	return (lval_char != NULL
		&& IS_SET(lval_char->act, flag_lookup(buf, act_flags)));

    case CHK_IMM:
	return (lval_char != NULL
		&& check_immune(lval_char, flag_lookup(buf, dam_flags)) == 100);

    case CHK_OFF:
	return (lval_char != NULL
		&& IS_SET(lval_char->off_flags, flag_lookup(buf, off_flags)));

    case CHK_CARRIES:
	if (is_number(buf))
	    return (lval_char != NULL
		    && has_item(lval_char, atoi(buf), -1, WEAR_ANYWHERE));
	else
	    return (lval_char != NULL
		    && (get_obj_carry(lval_char, buf, lval_char) != NULL));

    case CHK_WEARS:
	if (is_number(buf))
	    return (lval_char != NULL
		    && has_item(lval_char, atoi(buf), -1, WEAR_WEAR));
	else
	    return (lval_char != NULL
		    && (get_obj_wear(lval_char, buf, FALSE) != NULL));

    case CHK_HAS:
	return (lval_char != NULL
		&& has_item(lval_char, -1, item_lookup(buf), WEAR_ANYWHERE));

    case CHK_USES:
	return (lval_char != NULL
		&& has_item(lval_char, -1, item_lookup(buf), wear_loc_lookup(buf)));

    case CHK_NAME:
	switch (code)
	{
	default :
	case 'n':
	case 't':
	case 'r':
	case 'q':
	case 'b':
	case 'c':
	    return (lval_char != NULL && is_name(buf, lval_char->name));
	case 'i':
	    return FALSE;
	case 'o':
	case 'p':
	    return (lval_obj != NULL && is_name(buf, lval_obj->name));
	}
	break;

    case CHK_POS:
	return (lval_char != NULL
		&& lval_char->position == position_lookup(buf));

    case CHK_CLAN:
	return (lval_char != NULL
		&& is_clan(lval_char) == clan_lookup(buf));

    case CHK_RACE:
	return (lval_char != NULL
		&& lval_char->race == race_lookup(buf));

    case CHK_CLASS:
	return (lval_char != NULL
		&& lval_char->classid == class_lookup(buf));

    case CHK_OBJTYPE:
	return (lval_obj != NULL
		&& lval_obj->item_type == item_lookup(buf));

    case CHK_CANUSEOBJ:

	if (!is_number(buf))
	{
	    bugf("Cmd_eval_room: prog %d canuseobj argument is not vnum",
		 vnum);
	    return FALSE;
	}

	return (can_use_obj(lval_char, atoi(buf)));

    case CHK_ROOM:

	if (keyword_lookup(fn_evals, buf) >= 0)
	    break;

	if ((oper = flag_lookup(buf, room_flags)) != NO_FLAG)
	    rf = TRUE;
	else if ((oper = flag_lookup(buf, sector_flags)) != NO_FLAG)
	    rf = FALSE;
	else
	{
	    bugf("Cmd_eval_room: prog %d unknown room flag '%s'",
		 vnum, original);
	    return FALSE;
	}

	in_room = lval_char && lval_char->in_room ? lval_char->in_room : 
	    lval_obj && lval_obj->in_room ? lval_obj->in_room :
	    lval_obj && lval_obj->carried_by && lval_obj->carried_by->in_room ? lval_obj->carried_by->in_room :
	    NULL;

	if (room && ((rf && IS_SET(room->room_flags, oper))
		     || (!rf && room->sector_type == oper)))
	    return TRUE;
	else
	    return FALSE;

    default:;
    }

    /*
     * Case 5: Keyword, actor, comparison and value
     */
    if ((oper = keyword_lookup(fn_evals, buf)) < 0)
    {
	bugf("Cmd_eval_room: prog %d syntax error(5): '%s'",
	     vnum, original);
	return FALSE;
    }
    one_argument(line, buf);
    rval = atoi(buf);

    switch (check)
    {
    case CHK_VNUM:
	switch (code)
	{
	default :
	case 'n':
	case 't':
	case 'r':
	case 'q':
	case 'b':
	case 'c':
	    if(lval_char != NULL && IS_NPC(lval_char))
		lval = lval_char->pIndexData->vnum;
	    break;
	case 'i':
	    return FALSE;
	case 'o':
	case 'p':
	    if (lval_obj != NULL)
		lval = lval_obj->pIndexData->vnum;
	}
	break;

    case CHK_HPCNT:
	if (lval_char != NULL)
	    lval = (lval_char->hit * 100) / (UMAX(1, lval_char->max_hit));
	break;

    case CHK_ROOM:
	if (lval_char != NULL && lval_char->in_room != NULL)
	    lval = lval_char->in_room->vnum;
	else if (lval_obj && (lval_obj->in_room || lval_obj->carried_by))
	    lval = lval_obj->in_room ? lval_obj->in_room->vnum
		: lval_obj->carried_by->in_room->vnum;
	break;

    case CHK_SEX:
	if (lval_char != NULL)
	    lval = lval_char->sex;
	break;

    case CHK_LEVEL:
	if (lval_char != NULL)
	    lval = lval_char->level;
	break;

    case CHK_ALIGN:
	if (lval_char != NULL)
	    lval = lval_char->alignment;
	break;

    case CHK_MONEY:  /* Money is converted to silver... */
	if (lval_char != NULL)
	    lval = lval_char->gold + (lval_char->silver * 100);
	break;

    case CHK_OBJVAL0:
	if (lval_obj != NULL)
	    lval = lval_obj->value[0];
	break;

    case CHK_OBJVAL1:
	if (lval_obj != NULL)
	    lval = lval_obj->value[1];
	break;

    case CHK_OBJVAL2:
	if (lval_obj != NULL)
	    lval = lval_obj->value[2];
	break;

    case CHK_OBJVAL3:
	if (lval_obj != NULL)
	    lval = lval_obj->value[3];
	break;

    case CHK_OBJVAL4:
	if (lval_obj != NULL)
	    lval = lval_obj->value[4];
	break;

    case CHK_GRPSIZE:
	if(lval_char != NULL)
	    lval = count_people_room(lval_char, NULL, NULL, 4);
	break;

	/* проверки на статсы */
    case CHK_ISSTR:
	if (lval_char != NULL)
	    lval = get_curr_stat(lval_char, STAT_STR);
	break;

    case CHK_ISINT:
	if (lval_char != NULL)
	    lval = get_curr_stat(lval_char, STAT_INT);
	break;

    case CHK_ISWIS:
	if (lval_char != NULL)
	    lval = get_curr_stat(lval_char, STAT_WIS);
	break;

    case CHK_ISDEX:
	if (lval_char != NULL)
	    lval = get_curr_stat(lval_char, STAT_DEX);
	break;

    case CHK_ISCON:
	if (lval_char != NULL)
	    lval = get_curr_stat(lval_char, STAT_CON);
	break;

    default:
	return FALSE;
    }
    return (num_eval(lval, oper, rval));
}

/*
 * ------------------------------------------------------------------------
 * EXPAND_ARG
 * These are hacks of act() in comm.c. I've added some safety guards,
 * so that missing or invalid $-codes do not crash the server
 * ------------------------------------------------------------------------
 */
void expand_arg_mob(char *buf,
		    const char *format,
		    CHAR_DATA *mob, CHAR_DATA *ch,
		    const void *arg1, const void *arg2, CHAR_DATA *rch)
{
    char fname[MAX_INPUT_LENGTH];
    CHAR_DATA *vch = (CHAR_DATA *) arg2;
    OBJ_DATA *obj1 = (OBJ_DATA  *) arg1;
    OBJ_DATA *obj2 = (OBJ_DATA  *) arg2;
    const char *str;
    const char *i;
    char *point;

    /*
     * Discard null and zero-length messages.
     */
    if (format == NULL || format[0] == '\0')
	return;

    point   = buf;
    str     = format;
    while (*str != '\0')
    {
	int cs, delta;

	if (*str != '$')
	{
	    *point++ = *str++;
	    continue;
	}
	++str;

	cs = *(str+1)-'0';
	if (cs < 0 || cs > 6)
	{
	    cs = 0;
	    delta = 1;
	}
	else
	    delta = 2;

	switch (*str)
	{
	default:
	    bugf("Expand_arg_mob: bad code %d from mob [%d].", *str, mob->pIndexData->vnum);
	    i = " <@@@> ";
	    break;

	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	    i = match[*str - '1'];
	    break;

	case 'a':
	    i = arg1 ? (char *) arg1 : "";
	    break;

	case 'A':
	    i = arg2 ? (char *) arg2 : "";
	    break;

	case 'i':
	    one_argument(mob->name, fname);
	    i = fname;
	    break;

	case 'I':
	    i = cases(mob->short_descr, cs);
	    break;

	case 'n':
	    i = someone;
	    if (ch != NULL && can_see(mob, ch))
	    {
		one_argument(ch->name, fname);
		i = capitalize(cases(fname, cs));
	    }
	    break;

	case 'N':
	    i = ch ? PERS(ch, mob, cs) : cases(someone, cs);
	    break;

	case 't':
	    i = someone;
	    if (vch != NULL && can_see(mob, vch))
	    {
		one_argument(vch->name, fname);
		i = capitalize(cases(fname, cs));
	    }
	    break;

	case 'T':
	    i = vch ? PERS(vch, mob, cs) : cases(someone, cs);
	    break;

	case 'r':
	    if (rch == NULL)
		rch = get_random_char(mob, NULL, NULL, PC_ONLY|NPC_ONLY);

	    i = someone;

	    if (rch != NULL && can_see(mob, rch))
	    {
		one_argument(rch->name, fname);
		i = capitalize(cases(fname, cs));
	    }
	    break;

	case 'R':
	    if (rch == NULL)
		rch = get_random_char(mob, NULL, NULL, PC_ONLY|NPC_ONLY);

	    i  = rch ? PERS(rch, mob, cs) : cases(someone, cs);
	    break;
	case 'b':
	    if (rch == NULL)
		rch = get_random_char(mob, NULL, NULL, PC_ONLY);

	    i = someone;

	    if (rch != NULL && can_see(mob, rch))
	    {
		one_argument(rch->name, fname);
		i = capitalize(cases(fname, cs));
	    }
	    break;

	case 'B':
	    if (rch == NULL)
		rch = get_random_char(mob, NULL, NULL, PC_ONLY);

	    i  = rch ? PERS(rch, mob, cs) : cases(someone, cs);
	    break;
	case 'c':
	    if (rch == NULL)
		rch = get_random_char(mob, NULL, NULL, NPC_ONLY);

	    i = someone;

	    if (rch != NULL && can_see(mob, rch))
	    {
		one_argument(rch->name, fname);
		i = capitalize(cases(fname, cs));
	    }
	    break;

	case 'C':
	    if (rch == NULL)
		rch = get_random_char(mob, NULL, NULL, NPC_ONLY);

	    i  = rch ? PERS(rch, mob, cs) : cases(someone, cs);
	    break;

	case 'q':
	    i = someone;
	    if (mob->mprog_target != NULL
		&& can_see(mob, mob->mprog_target))
	    {
		one_argument(mob->mprog_target->name, fname);
		i = capitalize(cases(fname, cs));
	    }
	    break;

	case 'Q':
	    i = mob->mprog_target ? PERS(mob->mprog_target, mob, cs)
		: cases(someone,cs);
	    break;

	case 'j':
	    i = he_she  [URANGE(0, mob->sex, GET_MAX_SEX(mob))];
	    break;

	case 'e':
	    i = (ch != NULL && can_see(mob, ch))
		? he_she  [URANGE(0, ch->sex, GET_MAX_SEX(ch))]
		: someone;
	    break;

	case 'E':
	    i = (vch != NULL && can_see(mob, vch))
		? he_she  [URANGE(0, vch->sex, GET_MAX_SEX(vch))]
		: someone;
	    break;

	case 'J':
	    i = (rch != NULL && can_see(mob, rch))
		? he_she  [URANGE(0, rch->sex, GET_MAX_SEX(rch))]
		: someone;
	    break;

	case 'X':
	    i = (mob->mprog_target && can_see(mob, mob->mprog_target))
		? he_she  [URANGE(0, mob->mprog_target->sex, GET_MAX_SEX(mob->mprog_target))]
		: someone;
	    break;

	case 'k':
	    i = him_her [URANGE(0, mob->sex, GET_MAX_SEX(mob))];
	    break;

	case 'm':
	    i = (ch != NULL && can_see(mob, ch))
		? him_her [URANGE(0, ch->sex, GET_MAX_SEX(ch))]
		: someone;
	    break;

	case 'M':
	    i = (vch != NULL && can_see(mob, vch))
		? him_her [URANGE(0, vch->sex, GET_MAX_SEX(vch))]
		: someone;
	    break;

	case 'K':
	    if (rch == NULL)
		rch = get_random_char(mob, NULL, NULL, PC_ONLY|NPC_ONLY);

	    i = (rch != NULL && can_see(mob, rch))
		? him_her [URANGE(0, rch->sex, 2)]
		: someone;
	    break;

	case 'Y':
	    i = (mob->mprog_target && can_see(mob, mob->mprog_target))
		? him_her [URANGE(0, mob->mprog_target->sex, GET_MAX_SEX(mob->mprog_target))]
		: someone;
	    break;

	case 'l':
	    i = his_her [URANGE(0, mob->sex, GET_MAX_SEX(mob))];
	    break;

	case 's':
	    i = (ch != NULL && can_see(mob, ch))
		? his_her [URANGE(0, ch->sex, GET_MAX_SEX(ch))]
		: someones;
	    break;

	case 'S':
	    i = (vch != NULL && can_see(mob, vch))
		? his_her [URANGE(0, vch->sex, GET_MAX_SEX(vch))]
		: someones;
	    break;

	case 'L':
	    if (rch == NULL)
		rch = get_random_char(mob, NULL, NULL, PC_ONLY|NPC_ONLY);

	    i = (rch != NULL && can_see(mob, rch))
		? his_her [URANGE(0, rch->sex, GET_MAX_SEX(rch))]
		: someones;
	    break;

	case 'Z':
	    i = (mob->mprog_target && can_see(mob, mob->mprog_target))
		? his_her [URANGE(0, mob->mprog_target->sex, GET_MAX_SEX(mob->mprog_target))]
		: someones;
	    break;

	case 'o':
	    i = something;
	    if (obj1 != NULL && can_see_obj(mob, obj1))
	    {
		one_argument(obj1->name, fname);
		i = cases(fname, cs);
	    }
	    break;

	case 'O':
	    i = cases((obj1 != NULL && can_see_obj(mob, obj1))
		      ? obj1->short_descr
		      : something, cs);
	    break;

	case 'p':
	    i = something;
	    if (obj2 != NULL && can_see_obj(mob, obj2))
	    {
		one_argument(obj2->name, fname);
		i = cases(fname, cs);
	    }
	    break;

	case 'P':
	    i = cases((obj2 != NULL && can_see_obj(mob, obj2))
		      ? obj2->short_descr
		      : something, cs);
	    break;
	}

	str += delta;

	while ((*point = *i) != '\0')
	    ++point, ++i;

    }
    *point = '\0';

    return;
}

void expand_arg_other(char *buf,
		      const char *format,
		      OBJ_DATA *obj, ROOM_INDEX_DATA *room, CHAR_DATA *ch,
		      const void *arg1, const void *arg2, CHAR_DATA *rch)
{
    char fname[MAX_INPUT_LENGTH];
    CHAR_DATA *vch = (CHAR_DATA *) arg2;
    OBJ_DATA *obj1 = (OBJ_DATA  *) arg1;
    OBJ_DATA *obj2 = (OBJ_DATA  *) arg2;
    const char *str;
    const char *i;
    char *point;

    if (obj && room)
    {
	bugf("expand_arg_other received a obj and a room");
	return;
    }

    /*
     * Discard null and zero-length messages.
     */
    if (format == NULL || format[0] == '\0')
	return;

    point   = buf;
    str     = format;
    while (*str != '\0')
    {
	int delta, cs;

	if (*str != '$')
	{
	    *point++ = *str++;
	    continue;
	}
	++str;

	cs = *(str+1)-'0';
	if (cs < 0 || cs > 6)
	{
	    cs = 0;
	    delta = 1;
	}
	else
	    delta = 2;

	switch (*str)
	{
	case 'a':
	    i = arg1 ? (char *) arg1 : "";
	    break;

	case 'A':
	    i = arg2 ? (char *) arg2 : "";
	    break;

	case 'i':
	    if (obj)
	    {
		one_argument(obj->name, fname);
		i = fname;
	    }
	    else
	    {
		bugf("Expand_arg_other: room had an \"i\" case.");
		i = " <@@@> ";
	    }						break;
	case 'I':
	    if (obj)
		i = cases(obj->short_descr, cs);
	    else
	    {
		bugf("Expand_arg_other: room had an \"I\" case.");
		i = " <@@@> ";
	    }						break;
	case 'n':
	    i = someone;
	    if (ch != NULL)
	    {
		one_argument(ch->name, fname);
		i = capitalize(cases(fname, cs));
	    }						break;
	case 'N':
	    i = ch ? PERS(ch, ch, cs) : cases(someone, cs);
	    break;
	case 't':
	    i = someone;
	    if (vch != NULL)
	    {
		one_argument(vch->name, fname);
		i = capitalize(fname);
	    }						break;
	case 'T':
	    i = vch != NULL	? PERS(vch, vch, cs)
		: cases(someone, cs);                         		break;
	case 'r':
	    if (rch == NULL && obj)
		rch = get_random_char(NULL, obj, NULL, PC_ONLY|NPC_ONLY);
	    else if (rch == NULL && room)
		rch = get_random_char(NULL, NULL, room, PC_ONLY|NPC_ONLY);
	    i = someone;
	    if(rch != NULL)
	    {
		one_argument(rch->name, fname);
		i = capitalize(fname);
	    } 						break;
	case 'R':
	    if (rch == NULL && obj)
		rch = get_random_char(NULL, obj, NULL, PC_ONLY|NPC_ONLY);
	    else if (rch == NULL && room)
		rch = get_random_char(NULL, NULL, room, PC_ONLY|NPC_ONLY);
	    i  = rch ? PERS(rch, rch, cs)
		: cases(someone, cs);				break;
	case 'b':
	    if (rch == NULL && obj)
		rch = get_random_char(NULL, obj, NULL, PC_ONLY);
	    else if (rch == NULL && room)
		rch = get_random_char(NULL, NULL, room, PC_ONLY);
	    i = someone;
	    if(rch != NULL)
	    {
		one_argument(rch->name, fname);
		i = capitalize(fname);
	    } 						break;
	case 'B':
	    if (rch == NULL && obj)
		rch = get_random_char(NULL, obj, NULL, NPC_ONLY);
	    else if (rch == NULL && room)
		rch = get_random_char(NULL, NULL, room, NPC_ONLY);
	    i  = rch ? PERS(rch, rch, cs)
		: cases(someone, cs);				break;
	case 'c':
	    if (rch == NULL && obj)
		rch = get_random_char(NULL, obj, NULL, NPC_ONLY);
	    else if (rch == NULL && room)
		rch = get_random_char(NULL, NULL, room, NPC_ONLY);
	    i = someone;
	    if(rch != NULL)
	    {
		one_argument(rch->name, fname);
		i = capitalize(fname);
	    } 						break;
	case 'C':
	    if (rch == NULL && obj)
		rch = get_random_char(NULL, obj, NULL, NPC_ONLY);
	    else if (rch == NULL && room)
		rch = get_random_char(NULL, NULL, room, NPC_ONLY);
	    i  = rch ? PERS(rch, rch, cs)
		: cases(someone, cs);				break;
	case 'q':
	    i = someone;
	    if (obj && obj->oprog_target != NULL)
	    {
		one_argument(obj->oprog_target->name, fname);
		i = capitalize(fname);
	    }
	    else if (room && room->rprog_target != NULL)
	    {
		one_argument(room->rprog_target->name, fname);
		i = capitalize(fname);
	    } 						break;
	case 'Q':
	    i = cases((obj && obj->oprog_target != NULL)
		      ? (IS_NPC(obj->oprog_target) ?
			 obj->oprog_target->short_descr : obj->oprog_target->name)
		      : (room && room->rprog_target != NULL)
		      ? (IS_NPC(room->rprog_target) ?
			 room->rprog_target->short_descr : room->rprog_target->name)
		      : someone, cs);					break;
	case 'j':
	    bugf("Expand_arg_other: Obj/room received case 'j'");
	    i = " <@@@> ";     break;
	case 'e':
	    i = (ch != NULL)
		? he_she  [URANGE(0, ch->sex, GET_MAX_SEX(ch))]
		: someone;					break;
	case 'E':
	    i = (vch != NULL)
		? he_she  [URANGE(0, vch->sex, GET_MAX_SEX(vch))]
		: someone;					break;
	case 'J':
	    i = (rch != NULL)
		? he_she  [URANGE(0, rch->sex, GET_MAX_SEX(rch))]
		: someone;					break;
	case 'X':
	    i = (obj && obj->oprog_target != NULL)
		? he_she  [URANGE(0, obj->oprog_target->sex, GET_MAX_SEX(obj->oprog_target))]
		: (room && room->rprog_target != NULL)
		? he_she  [URANGE(0, room->rprog_target->sex, GET_MAX_SEX(room->rprog_target))]
		: someone;
	    break;
	case 'k':
	    bugf("Expand_arg_other: received case 'k'.");
	    i = " <@@@> ";					break;
	case 'm':
	    i = (ch != NULL)
		? him_her [URANGE(0, ch->sex, GET_MAX_SEX(ch))]
		: someone;        				break;
	case 'M':
	    i = (vch != NULL)
		? him_her [URANGE(0, vch->sex, GET_MAX_SEX(vch))]
		: someone;					break;
	case 'K':
	    if (obj && rch == NULL)
		rch = get_random_char(NULL, obj, NULL, PC_ONLY|NPC_ONLY);
	    else if (room && rch == NULL)
		rch = get_random_char(NULL, NULL, room, PC_ONLY|NPC_ONLY);
	    i = (rch != NULL)
		? him_her [URANGE(0, rch->sex, GET_MAX_SEX(rch))]
		: someone;					break;
	case 'Y':
	    i = (obj && obj->oprog_target != NULL)
		? him_her [URANGE(0, obj->oprog_target->sex, GET_MAX_SEX(obj->oprog_target))]
		: (room && room->rprog_target != NULL)
		? him_her [URANGE(0, room->rprog_target->sex, GET_MAX_SEX(room->rprog_target))]
		: someone;					break;
	case 'l':
	    bugf("Expand_arg_other: received case 'l'.");
	    i = " <@@@> ";					break;
	case 's':
	    i = (ch != NULL)
		? his_her [URANGE(0, ch->sex, GET_MAX_SEX(ch))]
		: someones;					break;
	case 'S':
	    i = (vch != NULL)
		? his_her [URANGE(0, vch->sex, GET_MAX_SEX(vch))]
		: someones;					break;
	case 'L':
	    if (obj && rch == NULL)
		rch = get_random_char(NULL, obj, NULL, PC_ONLY|NPC_ONLY);
	    else if (room && rch == NULL)
		rch = get_random_char(NULL, NULL, room, PC_ONLY|NPC_ONLY);
	    i = (rch != NULL)
		? his_her [URANGE(0, rch->sex, GET_MAX_SEX(rch))]
		: someones;					break;
	case 'Z':
	    i = (obj && obj->oprog_target != NULL)
		? his_her [URANGE(0, obj->oprog_target->sex, GET_MAX_SEX(obj->oprog_target))]
		: (room && room->rprog_target != NULL)
		? his_her [URANGE(0, room->rprog_target->sex, GET_MAX_SEX(room->rprog_target))]
		: someones;					break;
	case 'o':
	    i = something;
	    if (obj1 != NULL)
	    {
		one_argument(obj1->name, fname);
		i = fname;
	    } 						break;
	case 'O':
	    i = cases((obj1 != NULL)
		      ? obj1->short_descr
		      : something, cs);				break;
	case 'p':
	    i = something;
	    if (obj2 != NULL)
	    {
		one_argument(obj2->name, fname);
		i = fname;
	    } 						break;
	case 'P':
	    i = cases((obj2 != NULL)
		      ? obj2->short_descr
		      : something, cs);				break;

	case 'w':
	    if(obj->carried_by != NULL)
	    {
		one_argument(obj->carried_by->name, fname);
		i = capitalize(fname);
	    }
	    else
		i = " <@@@> ";
	    break;

	default:
	    bugf("Expand_arg: bad code %d.", *str);
	    i = " <@@@> ";
	    break;
	}

	str += delta;
	while ((*point = *i) != '\0')
	    ++point, ++i;

    }
    *point = '\0';

    return;
}

/*
 * ------------------------------------------------------------------------
 *  PROGRAM_FLOW
 *  This is the program driver. It parses the mob program code lines
 *  and passes "executable" commands to interpret()
 *  Lines beginning with 'mob' are passed to mob_interpret() to handle
 *  special mob commands (in mob_cmds.c)
 *-------------------------------------------------------------------------
 */

#define MAX_NESTED_LEVEL 15 /* Maximum nested if-else-endif's (stack size) */
#define BEGIN_BLOCK       0 /* Flag: Begin of if-else-endif block */
#define IN_BLOCK         -1 /* Flag: Executable statements */
#define END_BLOCK        -2 /* Flag: End of if-else-endif block */
#define MAX_CALL_LEVEL    20 /* Maximum nested calls */

int program_flow(
		 int pvnum,  /* For diagnostic purposes */
		 char *source,  /* the actual MOBprog code */
		 CHAR_DATA *mob,
		 OBJ_DATA *obj,
		 ROOM_INDEX_DATA *room,
		 CHAR_DATA *ch, const void *arg1, const void *arg2)

{
    CHAR_DATA *rch = NULL;
    char *code, *line;
    char buf[MAX_STRING_LENGTH];
    char control[MAX_INPUT_LENGTH], data[MAX_STRING_LENGTH];

    static int call_level; /* Keep track of nested "mpcall"s */

    int level, eval, check;
    int state[MAX_NESTED_LEVEL], /* Block state (BEGIN,IN,END) */
	cond[MAX_NESTED_LEVEL];  /* Boolean value based on the last if-check */

    int mvnum = 0, ovnum = 0, rvnum = 0;

    if ((mob && obj) || (mob && room) || (obj && room))
    {
	bugf("PROGs: program_flow received multiple prog types.");
	return -1;
    }

    if (mob)
	mvnum = mob->pIndexData->vnum;
    else if (obj)
	ovnum = obj->pIndexData->vnum;
    else if (room)
	rvnum = room->vnum;
    else
    {
	bugf("PROGs: program_flow did not receive a prog type.");
	return -1;
    }

    if (++call_level > MAX_CALL_LEVEL)
    {
	if (mob)
	{
	    bugf("Progs: MAX_CALL_LEVEL exceeded, vnum %d, mprog vnum %d",
		 mvnum, pvnum);
	}
	else if (obj)
	{
	    bugf("Progs: MAX_CALL_LEVEL exceeded, vnum %d oprog vnum %d.",
		 ovnum, pvnum);
	}
	else
	{
	    bugf("Progs: MAX_CALL_LEVEL exceeded, vnum %d rprog vnum %d.",
		 rvnum, pvnum);
	}

	call_level--;
	return -1;
    }

    /*
     * Reset "stack"
     */
    for (level = 0; level < MAX_NESTED_LEVEL; level++)
    {
	state[level] = IN_BLOCK;
	cond[level]  = TRUE;
    }
    level = 0;

    code = source;
    /*
     * Parse the Prog code
     */
    while (*code)
    {
	bool first_arg = TRUE;
	char *b = buf, *c = control, *d = data;
	/*
	 * Get a command line. We sneakily get both the control word
	 * (if/and/or) and the rest of the line in one pass.
	 */
	while(IS_SPACE(*code) && *code)
	    code++;

	while (*code)
	{
	    if (*code == '\n' || *code == '\r')
		break;
	    else if (IS_SPACE(*code))
	    {
		if (first_arg)
		    first_arg = FALSE;
		else
		    *d++ = *code;
	    }
	    else
	    {
		if (first_arg)
		    *c++ = *code;
		else
		    *d++ = *code;
	    }
	    *b++ = *code++;
	}
	*b = *c = *d = '\0';

	if (buf[0] == '\0')
	    break;
	if (buf[0] == '*') /* Comment */
	    continue;

	line = data;
	/*
	 * Match control words
	 */
	if (!str_cmp(control, "return") && (!level || cond[level] == TRUE))
	{
	    int retval;

	    if (!str_cmp(data, "true"))
		retval = TRUE;
	    else if (!str_cmp(data, "false"))
		retval = FALSE;
	    else
	    {
		if (mob)
		{
		    bugf("MProg %d, mob %d: invalid return value '%s'",
			 pvnum, mvnum, data);
		}
		else if (obj)
		{
		    bugf("OProg %d, obj %d: invalid return value '%s'",
			 pvnum, ovnum, data);
		}
		else
		{
		    bugf("RProg %d, room %d: invalid return value '%s'",
			 pvnum, rvnum, data);
		}

		retval = -1;
	    }

	    call_level--;
	    return retval;
	}
	else if (!str_cmp(control, "if"))
	{
	    if (state[level] == BEGIN_BLOCK)
	    {
		if (mob)
		{
		    bugf("Mobprog: misplaced if statement, mob %d prog %d",
			 mvnum, pvnum);
		}
		else if (obj)
		{
		    bugf("Objprog: misplaced if statement, obj %d prog %d",
			 ovnum, pvnum);
		}
		else
		{
		    bugf("Roomprog: misplaced if statement, room %d prog %d",
			 rvnum, pvnum);
		}

		call_level--;
		return -1;
	    }

	    state[level] = BEGIN_BLOCK;
	    if (++level >= MAX_NESTED_LEVEL)
	    {
		if (mob)
		{
		    bugf("Mprog: Max nested level exceeded, mob %d prog %d",
			 mvnum, pvnum);
		}
		else if (obj)
		{
		    bugf("Oprog: Max nested level exceeded, obj %d prog %d",
			 ovnum, pvnum);
		}
		else
		{
		    bugf("Rprog: Max nested level exceeded, room %d prog %d",
			 rvnum, pvnum);
		}

		return -1;
	    }

	    if (level && cond[level-1] == FALSE)
	    {
		cond[level] = FALSE;
		continue;
	    }

	    line = one_argument(line, control);
	    if (mob && (check = keyword_lookup(fn_keyword, control)) >= 0)
	    {
		cond[level] = cmd_eval_mob(pvnum, line, check, mob,
					   ch, arg1, arg2, rch);
	    }
	    else if (obj && (check = keyword_lookup(fn_keyword, control)) >= 0)
	    {
		cond[level] = cmd_eval_obj(pvnum, line, check, obj,
					   ch, arg1, arg2, rch);
	    }
	    else if (room && (check = keyword_lookup(fn_keyword, control)) >= 0)
	    {
		cond[level] = cmd_eval_room(pvnum, line, check, room,
					    ch, arg1, arg2, rch);
	    }
	    else
	    {
		if (mob)
		{
		    bugf("Mobprog: invalid if_check (if), mob %d prog %d",
			 mvnum, pvnum);
		}
		else if (obj)
		{
		    bugf("Objprog: invalid if_check (if), obj %d prog %d",
			 ovnum, pvnum);
		}
		else
		{
		    bugf("Roomprog: invalid if_check (if), room %d prog %d",
			 rvnum, pvnum);
		}

		return -1;
	    }
	    state[level] = END_BLOCK;
	}
	else if (!str_cmp(control, "or"))
	{
	    if (!level || state[level-1] != BEGIN_BLOCK)
	    {
		if (mob)
		    bugf("Mobprog: or without if, mob %d prog %d",
			 mvnum, pvnum);
		else if (obj)
		    bugf("Objprog: or without if, obj %d prog %d",
			 ovnum, pvnum);
		else
		    bugf("Roomprog: or without if, room %d prog %d",
			 rvnum, pvnum);

		return -1;
	    }

	    if (level && cond[level-1] == FALSE)
		continue;

	    line = one_argument(line, control);
	    if (mob && (check = keyword_lookup(fn_keyword, control)) >= 0)
	    {
		eval = cmd_eval_mob(pvnum, line, check, mob,
				    ch, arg1, arg2, rch);
	    }
	    else if (obj && (check = keyword_lookup(fn_keyword, control)) >= 0)
	    {
		eval = cmd_eval_obj(pvnum, line, check, obj,
				    ch, arg1, arg2, rch);
	    }
	    else if (room && (check = keyword_lookup(fn_keyword, control)) >= 0)
	    {
		eval = cmd_eval_room(pvnum, line, check, room,
				     ch, arg1, arg2, rch);
	    }
	    else
	    {
		if (mob)
		{
		    bugf("Mobprog: invalid if_check (or), mob %d prog %d",
			 mvnum, pvnum);
		}
		else if (obj)
		{
		    bugf("Objprog: invalid if_check (or), obj %d prog %d",
			 ovnum, pvnum);
		}
		else
		{
		    bugf("Roomprog: invalid if_check (or), room %d prog %d",
			 rvnum, pvnum);
		}

		return -1;
	    }

	    cond[level] = (eval == TRUE) ? TRUE : cond[level];
	}
	else if (!str_cmp(control, "and"))
	{
	    if (!level || state[level - 1] != BEGIN_BLOCK)
	    {
		if (mob)
		    bugf("Mobprog: and without if, mob %d prog %d",
			 mvnum, pvnum);
		else if (obj)
		    bugf("Objprog: and without if, obj %d prog %d",
			 ovnum, pvnum);
		else
		    bugf("Roomprog: and without if, room %d prog %d",
			 rvnum, pvnum);

		return -1;
	    }

	    if (level && cond[level-1] == FALSE)
		continue;

	    line = one_argument(line, control);
	    if (mob && (check = keyword_lookup(fn_keyword, control)) >= 0)
	    {
		eval = cmd_eval_mob(pvnum, line, check, mob,
				    ch, arg1, arg2, rch);
	    }
	    else if (obj && (check = keyword_lookup(fn_keyword, control)) >= 0)
	    {
		eval = cmd_eval_obj(pvnum, line, check, obj,
				    ch, arg1, arg2, rch);
	    }
	    else if (room && (check = keyword_lookup(fn_keyword, control)) >= 0)
	    {
		eval = cmd_eval_room(pvnum, line, check, room,
				     ch, arg1, arg2, rch);
	    }
	    else
	    {
		if (mob)
		    bugf("Mobprog: invalid if_check (and), mob %d prog %d",
			 mvnum, pvnum);
		else if (obj)
		    bugf("Objprog: invalid if_check (and), obj %d prog %d",
			 ovnum, pvnum);
		else
		    bugf("Roomprog: invalid if_check (and), room %d prog %d",
			 rvnum, pvnum);

		return -1;
	    }
	    cond[level] = (cond[level] == TRUE) && (eval) ? TRUE : FALSE;
	}
	else if (!str_cmp(control, "endif"))
	{
	    if (!level || state[level-1] != BEGIN_BLOCK)
	    {
		if (mob)
		    bugf("Mobprog: endif without if, mob %d prog %d",
			 mvnum, pvnum);
		else if (obj)
		    bugf("Objprog: endif without if, obj %d prog %d",
			 ovnum, pvnum);
		else
		    bugf("Roomprog: endif without if, room %d prog %d",
			 rvnum, pvnum);

		call_level--;
		return -1;
	    }

	    cond[level] = TRUE;
	    state[level] = IN_BLOCK;
	    state[--level] = END_BLOCK;
	}
	else if (!str_cmp(control, "else"))
	{
	    if (!level || state[level - 1] != BEGIN_BLOCK)
	    {
		if (mob)
		    bugf("Mobprog: else without if, mob %d prog %d",
			 mvnum, pvnum);
		else if (obj)
		    bugf("Objprog: else without if, obj %d prog %d",
			 ovnum, pvnum);
		else
		    bugf("Roomprog: else without if, room %d prog %d",
			 rvnum, pvnum);

		return -1;
	    }

	    if (level && cond[level-1] == FALSE)
		continue;

	    state[level] = IN_BLOCK;
	    cond[level] = (cond[level] == TRUE) ? FALSE : TRUE;
	}
	else if (!str_cmp(control, "output"))
	{
	    if (!str_cmp(data, "true"))
		Show_output = TRUE;
	    else if (!str_cmp(data, "false"))
		Show_output = FALSE;
	    else
		bugf("Bad output argument for output command, prog %d", pvnum);
	}
	else if (cond[level] == TRUE
		 && (!str_cmp(control, "break") || !str_cmp(control, "end")))
	{
	    call_level--;
	    return TRUE;
	}
	else if ((!level || cond[level] == TRUE) && buf[0] != '\0')
	{
	    state[level] = IN_BLOCK;

	    if (mob)
		expand_arg_mob(data, buf, mob, ch, arg1, arg2, rch);
	    else if (obj)
		expand_arg_other(data, buf, obj, NULL, ch, arg1, arg2, rch);
	    else
		expand_arg_other(data, buf, NULL, room, ch, arg1, arg2, rch);

	    if (!str_cmp(control, "mob"))
	    {
		/*
		 * Found a mob restricted command, pass it to mob interpreter
		 */
		line = one_argument(data, control);
		if (!mob)
		    bugf("mob command in non MOBprog", line);
		else
		    mob_interpret(mob, line);
	    }
	    else if (!str_cmp(control, "obj"))
	    {
		/*
		 * Found an obj restricted command, pass it to obj interpreter
		 */
		line = one_argument(data, control);
		if (!obj)
		    bugf("obj command in non OBJprog");
		else
		    obj_interpret(obj, line);
	    }
	    else if (!str_cmp(control, "room"))
	    {
		/*
		 * Found a room restricted command, pass it to room interpreter
		 */
		line = one_argument(data, control);
		if (!room)
		    bugf("room command in non ROOMprog");
		else
		    room_interpret(room, line);
	    }
	    else
	    {
		/*
		 * Found a normal mud command, pass it to interpreter
		 */
		if (!mob)
		    bugf("Normal MUD command (%s) in non-MOBprog, prog vnum %d",
			 control, pvnum);
		else
		    interpret(mob, data);
	    }
	}
    }
    call_level--;

    Show_output = TRUE;

    return TRUE;
}

/*
 * ---------------------------------------------------------------------
 * Trigger handlers. These are called from various parts of the code
 * when an event is triggered.
 * ---------------------------------------------------------------------
 */

int16_t bit2number(int64_t bit)
{
    int i;

    for (i = 1; i < 64; i++)
	if ((1 << i) == bit)
	    return i;

    return -1;
}

/*
 * A general purpose string trigger. Matches argument to a string trigger
 * phrase.
 */
void p_act_trigger(
		   char *argument, CHAR_DATA *mob, OBJ_DATA *obj,
		   ROOM_INDEX_DATA *room, CHAR_DATA *ch,
		   const void *arg1, const void *arg2, int type)
{
    PROG_LIST *prg;
//    struct lua_progs *lprg;
    int16_t trig;

    if ((mob && obj) || (mob && room) || (obj && room))
    {
	bugf("Multiple program types in ACT trigger.");
	return;
    }

    trig = bit2number(type);

    if (mob)
    {
	mob_holylight = FALSE;

/*
 	for (lprg = mob->pIndexData->lprg; lprg; lprg = lprg->next)
	{
	    if (lprg->trig == trig && !str_infix(lprg->arg, argument))
		raise_signal(lprg, mob, LTYPE_CHAR, ch, LTYPE_CHAR, "MM", arg1, arg2);
	}
 */

	for (prg = mob->pIndexData->progs; prg != NULL; prg = prg->next)
	{
	    if (prg->trig_type == type
		&& regex_compare(argument, prg->trig_phrase))
	    {
		program_flow(prg->vnum, prg->code, mob, NULL, NULL,
			     ch, arg1, arg2);
		break;
	    }
	}
	mob_holylight = FALSE;
    }
    else if (obj)
    {
/*	
	for (lprg = obj->pIndexData->lprg; lprg; lprg = lprg->next)
	{
	    if (lprg->trig == trig && !str_infix(lprg->arg, argument))
		raise_signal(lprg, obj, LTYPE_OBJ, ch, LTYPE_CHAR, "MM", arg1, arg2);
	}
 */
	for (prg = obj->pIndexData->progs; prg != NULL; prg = prg->next)
	{
	    if (prg->trig_type == type
		&& regex_compare(argument, prg->trig_phrase))
	    {
		program_flow(prg->vnum, prg->code, NULL, obj, NULL,
			     ch, arg1, arg2);
		break;
	    }
	}
    }
    else if (room)
    {
/*		
	for (lprg = room->lprg; lprg; lprg = lprg->next)
	{
	    if (lprg->trig == trig && !str_infix(lprg->arg, argument))
		raise_signal(lprg, room, LTYPE_ROOM, ch, LTYPE_CHAR, "MM", arg1, arg2);
	}
 */
	for (prg = room->progs; prg != NULL; prg = prg->next)
	{
	    if (prg->trig_type == type
		&& regex_compare(argument, prg->trig_phrase))
	    {
		program_flow(prg->vnum, prg->code, NULL, NULL,
			     room, ch, arg1, arg2);
		break;
	    }
	}
    }
    else
	bugf("ACT trigger with no program type.");

    return;
}

/*
 * A general purpose percentage trigger. Checks if a random percentage
 * number is less than trigger phrase
 */
bool p_percent_trigger(CHAR_DATA *mob, OBJ_DATA *obj, ROOM_INDEX_DATA *room,
		       CHAR_DATA *ch, const void *arg1, const void *arg2, int type)
{
    PROG_LIST *prg;
    int16_t trig;
//    struct lua_progs *lprg;

    if ((mob && obj) || (mob && room) || (obj && room))
    {
	bugf("Multiple program types in PERCENT trigger.");
	return (FALSE);
    }

    trig = bit2number(type);

    if (mob)
    {
	mob_holylight = FALSE;

/*	
	for (lprg = mob->pIndexData->lprg; lprg; lprg = lprg->next)
	{
	    if (lprg->trig == trig && number_percent() < atoi(lprg->arg))
		raise_signal(lprg, mob, LTYPE_CHAR, ch, LTYPE_CHAR, NULL);
	}
 */

	for (prg = mob->pIndexData->progs; prg != NULL; prg = prg->next)
	{
	    if (prg->trig_type == type
		&& number_percent() < atoi(prg->trig_phrase))
	    {
		program_flow(prg->vnum, prg->code, mob, NULL, NULL,
			     ch, arg1, arg2);
		return (TRUE);
	    }
	}
	mob_holylight = FALSE;
    }
    else if (obj)
    {
/*	
	for (lprg = obj->pIndexData->lprg; lprg; lprg = lprg->next)
	{
	    if (lprg->trig == trig && number_percent() < atoi(lprg->arg))
		raise_signal(lprg, obj, LTYPE_OBJ, ch, LTYPE_CHAR, NULL);
	}
 */
	for (prg = obj->pIndexData->progs; prg != NULL; prg = prg->next)
	{
	    if (prg->trig_type == type
		&& number_percent() < atoi(prg->trig_phrase))
	    {
		program_flow(prg->vnum, prg->code, NULL, obj, NULL,
			     ch, arg1, arg2);
		return (TRUE);
	    }
	}
    }
    else if (room)
    {
/*	
	for (lprg = room->lprg; lprg; lprg = lprg->next)
	{
	    if (lprg->trig == trig && number_percent() < atoi(lprg->arg))
		raise_signal(lprg, room, LTYPE_ROOM, ch, LTYPE_CHAR, NULL);
	}
 */
	for (prg = room->progs; prg != NULL; prg = prg->next)
	{
	    if (prg->trig_type == type
		&& number_percent() < atoi(prg->trig_phrase))
	    {
		program_flow(prg->vnum, prg->code, NULL, NULL,
			     room, ch, arg1, arg2);
		return (TRUE);
	    }
	}
    }
    else
	bugf("PERCENT trigger missing program type.");

    return (FALSE);
}

void p_bribe_trigger(CHAR_DATA *mob, CHAR_DATA *ch, int amount)
{
//    struct lua_progs *lprg;
    PROG_LIST *prg, *prg_exec = NULL;
    int max = 0, money;

    mob_holylight = FALSE;

/*    
	for (lprg = mob->pIndexData->lprg; lprg; lprg = lprg->next)
    {
	if (lprg->trig == LTRIG_BRIBE && number_percent() < atoi(lprg->arg))
	    raise_signal(lprg, mob, LTYPE_CHAR, ch, LTYPE_CHAR, NULL);
    }
*/

    for (prg = mob->pIndexData->progs; prg; prg = prg->next)
    {

	if (prg->trig_type == TRIG_BRIBE
	    && amount >= (money = atoi(prg->trig_phrase))
	    && money >  max)
	{
	    max = money;
	    prg_exec = prg;
	}
    }

    if (prg_exec)
	program_flow(prg_exec->vnum, prg_exec->code, mob, NULL, NULL, ch, NULL, NULL);

    mob_holylight = FALSE;

    return;
}

/* TODO: return values */
bool p_exit_trigger(CHAR_DATA *ch, int dir, int type)
{
    CHAR_DATA *mob, *safe_mob;
    OBJ_DATA *obj;
    ROOM_INDEX_DATA *room;
    PROG_LIST   *prg;
//    struct lua_progs *lprg;

    if (!ch->in_room)
	return FALSE;

    if (type == PRG_MPROG)
    {
	mob_holylight = FALSE;

	LIST_FOREACH_SAFE(mob, &ch->in_room->people, room_link, safe_mob)
	{
	    if (IS_NPC(mob)
		&& (has_trigger(mob->pIndexData->progs, TRIG_EXIT, EXEC_ALL)
		    || has_trigger(mob->pIndexData->progs, TRIG_EXALL, EXEC_ALL)))
	    {
		for (prg = mob->pIndexData->progs; prg; prg = prg->next)
		{
		    /*
		     * Exit trigger works only if the mobile is not busy
		     * (fighting etc.). If you want to be sure all players
		     * are caught, use ExAll trigger
		     */
		    if (prg->trig_type == TRIG_EXIT
			&& dir == atoi(prg->trig_phrase)
			&& mob->position == mob->pIndexData->default_pos
			&& can_see(mob, ch))
		    {
			program_flow(prg->vnum, prg->code, mob, NULL, NULL,
				     ch, NULL, NULL);
			mob_holylight = FALSE;
			return TRUE;
		    }
		    else if (prg->trig_type == TRIG_EXALL
			     && dir == atoi(prg->trig_phrase))
		    {
			program_flow(prg->vnum, prg->code, mob, NULL, NULL,
				     ch, NULL, NULL);
			mob_holylight = FALSE;
			return TRUE;
		    }
		}

/*
 		for (lprg = mob->pIndexData->lprg; lprg; lprg = lprg->next)
		{
		    if (lprg->trig == LTRIG_EXIT
			&& dir == atoi(lprg->arg)
			&& mob->position == mob->pIndexData->default_pos
			&& can_see(mob, ch))
		    {
			raise_signal(lprg, mob, LTYPE_CHAR, ch, LTYPE_CHAR, NULL);
		    }
		    else if (lprg->trig == LTRIG_EXALL
			     && dir == atoi(lprg->arg))
		    {
			raise_signal(lprg, mob, LTYPE_CHAR, ch, LTYPE_CHAR, NULL);
		    }
		}
 */
	    }
	}

	mob_holylight = FALSE;
    }
    else if (type == PRG_OPROG)
    {
	for (obj = ch->in_room->contents; obj != NULL; obj = obj->next_content)
	{
	    if (has_trigger(obj->pIndexData->progs, TRIG_EXALL, EXEC_ALL))
	    {
		for (prg = obj->pIndexData->progs; prg; prg = prg->next)
		{
		    if (prg->trig_type == TRIG_EXALL
			&& dir == atoi(prg->trig_phrase))
		    {
			program_flow(prg->vnum, prg->code, NULL,
				     obj, NULL, ch, NULL, NULL);
			return TRUE;
		    }
		}

/*		
		for (lprg = obj->pIndexData->lprg; lprg; lprg = lprg->next)
		{
		    if (lprg->trig == LTRIG_EXALL && dir == atoi(lprg->arg))
		    {
			raise_signal(lprg, obj, LTYPE_OBJ, ch, LTYPE_CHAR, NULL);
		    }
		}
 */
	    }
	}

	LIST_FOREACH_SAFE(mob, &ch->in_room->people, room_link, safe_mob)
	{
	    for (obj = mob->carrying; obj; obj = obj->next_content)
	    {
		if (has_trigger(obj->pIndexData->progs, TRIG_EXALL, EXEC_ALL))
		{
		    for (prg = obj->pIndexData->progs; prg; prg = prg->next)
		    {
			if (prg->trig_type == TRIG_EXALL
			    && dir == atoi(prg->trig_phrase))
			{
			    program_flow(prg->vnum, prg->code, NULL,
					 obj, NULL, ch, NULL, NULL);
			    return TRUE;
			}
		    }

/*		    
			for (lprg = obj->pIndexData->lprg; lprg; lprg = lprg->next)
		    {
			if (lprg->trig == LTRIG_EXALL
			    && dir == atoi(lprg->arg))
			{
			    raise_signal(lprg, obj, LTYPE_OBJ, ch, LTYPE_CHAR, NULL);
			}
		    }
 */
		}
	    }
	}
    }
    else if (type == PRG_RPROG)
    {
	room = ch->in_room;

	if (has_trigger(room->progs, TRIG_EXALL, EXEC_ALL))
	{
	    for (prg = room->progs; prg; prg = prg->next)
	    {
		if (prg->trig_type == TRIG_EXALL
		    && dir == atoi(prg->trig_phrase))
		{
		    program_flow(prg->vnum, prg->code, NULL, NULL,
				 room, ch, NULL, NULL);
		    return TRUE;
		}
	    }

/*	    
		for (lprg = room->lprg; lprg; lprg = lprg->next)
	    {
		if (lprg->trig == LTRIG_EXALL
		    && dir == atoi(lprg->arg))
		{
		    raise_signal(lprg, room, LTYPE_ROOM, ch, LTYPE_CHAR, "n", dir);
		}
	    }
 */
	}
    }

    return FALSE;
}

void p_wear_trigger(OBJ_DATA *obj, CHAR_DATA *ch)
{
    PROG_LIST   *prg;
//    struct lua_progs *lprg;

    for (prg = obj->pIndexData->progs; prg; prg = prg->next)
    {
	if (prg->trig_type == TRIG_WEAR)
	{
	    program_flow(prg->vnum, prg->code, NULL, obj, NULL, ch, NULL, NULL);
	    return;
	}
    }

/*
    for (lprg = obj->pIndexData->lprg; lprg; lprg = lprg->next)
	if (lprg->trig == LTRIG_WEAR)
	    raise_signal(lprg, obj, LTYPE_OBJ, ch, LTYPE_CHAR, NULL);
 */

    return;
}


void p_give_trigger(CHAR_DATA *mob, OBJ_DATA *obj, ROOM_INDEX_DATA *room,
		    CHAR_DATA *ch, OBJ_DATA *dropped, int type)
{

    char        buf[MAX_INPUT_LENGTH], *p;
    PROG_LIST  *prg;
    int16_t trig;
//    struct lua_progs *lprg;

    if ((mob && obj) || (mob && room) || (obj && room))
    {
	bugf("Multiple program types in GIVE trigger.");
	return;
    }

    trig = bit2number(type);

    if (mob)
    {
	mob_holylight = FALSE;

	for (prg = mob->pIndexData->progs; prg; prg = prg->next)
	    if (prg->trig_type == TRIG_GIVE)
	    {
		p = prg->trig_phrase;
		/*
		 * Vnum argument
		 */
		if (is_number(p))
		{
		    if (dropped->pIndexData->vnum == atoi(p))
		    {
			program_flow(prg->vnum, prg->code, mob, NULL, NULL,
				     ch, (void *) dropped, NULL);
			mob_holylight = FALSE;
			return;
		    }
		}
		/*
		 * Dropped object name argument, e.g. 'sword'
		 */
		else
		{
		    while(*p)
		    {
			p = one_argument(p, buf);

			if (is_name(buf, dropped->name)
			    || !str_cmp("all", buf))
			{
			    program_flow(prg->vnum, prg->code, mob, NULL,
					 NULL, ch, (void *) dropped, NULL);
			    mob_holylight = FALSE;
			    return;
			}
		    }
		}
	    }

	mob_holylight = FALSE;

/*	
	for (lprg = mob->pIndexData->lprg; lprg; lprg = lprg->next)
	    if (lprg->trig == LTRIG_GIVE)
	    {
			if (is_number(lprg->arg))
			{
		    	if (dropped->pIndexData->vnum == atoi(lprg->arg))
		    	{
					raise_signal(lprg, mob, LTYPE_CHAR, ch, LTYPE_CHAR,	"O", dropped);
					return;
		    	}
			}
  			else
			{
		    	one_argument(lprg->arg, buf);

		    	if (!str_cmp("all", buf) || is_name(buf, dropped->name))
		    	{
					raise_signal(lprg, mob, LTYPE_CHAR, ch, LTYPE_CHAR, "O", dropped);
					return;
		    	}
			}
	    }
 */
    }
    else if (obj)
    {
	for (prg = obj->pIndexData->progs; prg; prg = prg->next)
	    if (prg->trig_type == type)
	    {
		program_flow(prg->vnum, prg->code, NULL, obj, NULL,
			     ch, (void *) obj, NULL);
		return;
	    }
/*
	for (lprg = obj->pIndexData->lprg; lprg; lprg = lprg->next)
	    if (lprg->trig == trig)
	    {
			raise_signal(lprg, obj, LTYPE_OBJ, ch, LTYPE_CHAR, NULL);
	    }
 */
    }
    else if (room)
    {
	for (prg = room->progs; prg; prg = prg->next)
	    if (prg->trig_type == type)
	    {
		p = prg->trig_phrase;

		/*
		 * Vnum argument
		 */
		if (is_number(p))
		{
		    if (dropped->pIndexData->vnum == atoi(p))
		    {
			program_flow(prg->vnum, prg->code, NULL, NULL, room,
				     ch, (void *) dropped, NULL);
			return;
		    }
		}
		/*
		 * Dropped object name argument, e.g. 'sword'
		 */
		else
		{
		    while(*p)
		    {
			p = one_argument(p, buf);

			if (is_name(buf, dropped->name)
			    || !str_cmp("all", buf))
			{
			    program_flow(prg->vnum, prg->code, NULL, NULL,
					 room, ch, (void *) dropped, NULL);
			    return;
			}
		    }
		}
	    }

/*	
	for (lprg = room->lprg; lprg; lprg = lprg->next)
	    if (lprg->trig == trig)
	    {
			if (is_number(lprg->arg))
			{
		    	if (dropped->pIndexData->vnum == atoi(lprg->arg))
		    	{
					raise_signal(lprg, room, LTYPE_ROOM, ch, LTYPE_CHAR, "O", dropped);
					return;
		    	}
			}
			else
			{
		    	one_argument(lprg->arg, buf);

		    	if (!str_cmp("all", buf) || is_name(buf, dropped->name))
		    	{
					raise_signal(lprg, room, LTYPE_ROOM, ch, LTYPE_CHAR, "O", dropped);
					return;
		    	}
			}
	    }
 */
    }
}

void p_greet_trigger(CHAR_DATA *ch, int type)
{
    CHAR_DATA *mob, *safe_mob;
    OBJ_DATA *obj;
    ROOM_INDEX_DATA *room;

    if (!ch || !ch->in_room) return;

    if (type == PRG_MPROG)
    {
	mob_holylight = FALSE;

	LIST_FOREACH_SAFE(mob, &ch->in_room->people, room_link, safe_mob)
	{
	    if (IS_NPC(mob)
		&& (has_trigger(mob->pIndexData->progs, TRIG_GREET, EXEC_ALL)
		    || has_trigger(mob->pIndexData->progs,TRIG_GRALL, EXEC_ALL)))
	    {
		/*
		 * Greet trigger works only if the mobile is not busy
		 * (fighting etc.). If you want to catch all players, use
		 * GrAll trigger
		 */
		if (has_trigger(mob->pIndexData->progs,TRIG_GREET, EXEC_ALL)
		    && mob->position == mob->pIndexData->default_pos
		    && can_see(mob, ch))
		{
		    p_percent_trigger(mob, NULL, NULL, ch, NULL, NULL,
				      TRIG_GREET);
		}
		else if (has_trigger(mob->pIndexData->progs, TRIG_GRALL, EXEC_ALL))
		{
		    p_percent_trigger(mob, NULL, NULL, ch, NULL, NULL,
				      TRIG_GRALL);
		}
	    }
	}
	mob_holylight = FALSE;
    }
    else if (type == PRG_OPROG)
    {
	for (obj = ch->in_room->contents; obj != NULL; obj = obj->next_content)
	{
	    if (has_trigger(obj->pIndexData->progs, TRIG_GRALL, EXEC_ALL))
	    {
		p_percent_trigger(NULL, obj, NULL, ch, NULL, NULL, TRIG_GRALL);
		return;
	    }
	}

	LIST_FOREACH_SAFE(mob, &ch->in_room->people, room_link, safe_mob)
	{
	    for (obj = mob->carrying; obj; obj = obj->next_content)
	    {
		if (has_trigger(obj->pIndexData->progs, TRIG_GRALL, EXEC_ALL))
		{
		    p_percent_trigger(NULL, obj, NULL, ch, NULL, NULL,
				      TRIG_GRALL);
		    return;
		}
	    }
	}
    }
    else if (type == PRG_RPROG)
    {
	room = ch->in_room;

	if (has_trigger(room->progs, TRIG_GRALL, EXEC_ALL))
	    p_percent_trigger(NULL, NULL, room, ch, NULL, NULL, TRIG_GRALL);
    }

    return;
}

void p_hprct_trigger(CHAR_DATA *mob, CHAR_DATA *ch)
{
    PROG_LIST *prg;
//    struct lua_progs *lprg;

    mob_holylight = FALSE;

    for (prg = mob->pIndexData->progs; prg != NULL; prg = prg->next)
	if ((prg->trig_type == TRIG_HPCNT)
	    && ((100 * mob->hit / mob->max_hit) < atoi(prg->trig_phrase)))
	{
	    program_flow(prg->vnum, prg->code, mob, NULL, NULL, ch, NULL, NULL);
	    return;
	}

/*    
	for (lprg = mob->pIndexData->lprg; lprg; lprg = lprg->next)
	if (lprg->trig == LTRIG_HPCNT && ((100 * mob->hit / mob->max_hit) < atoi(lprg->arg)))
	{
	    raise_signal(lprg, mob, LTYPE_CHAR, ch, LTYPE_CHAR, "n", (double)mob->hit);
	    return;
	}
 */
    mob_holylight = FALSE;	
}

int p_command_trigger(CHAR_DATA *ch, const char *cmd, char *args, int type)
{
    PROG_LIST *prg;
    OBJ_DATA *obj;
    ROOM_INDEX_DATA *room;
    int found = FALSE;
    int retval = TRUE;
    bool new_trig = (type == TRIG_NEW_CMD);
//    int16_t trig = bit2number(type);
//    struct lua_progs *lprg;

    if (ch->in_room == NULL)
	return (!new_trig);

    mob_holylight = FALSE;

    for (obj = ch->in_room->contents; obj && !found; obj = obj->next_content)
    {
	if (has_trigger(obj->pIndexData->progs, type, EXEC_ALL))
	{
	    for (prg = obj->pIndexData->progs; prg; prg = prg->next)
		if (prg->trig_type == type
		    && !str_prefix(cmd, prg->trig_phrase))
		{
		    retval = program_flow(prg->vnum, prg->code, NULL, obj, NULL,
					  ch, args, NULL);
		    found = TRUE;
		    break;
		}

/*	    
		for (lprg = obj->pIndexData->lprg; lprg && !found; lprg = lprg->next)
		if (lprg->trig == trig && !str_prefix(cmd, lprg->arg))
		{
		    raise_signal(lprg, obj, LTYPE_OBJ, ch, LTYPE_CHAR, "s", args);
		    found = TRUE;
		    break;
		}
 */
	}
    }

    for (obj = ch->carrying; obj && !found; obj = obj->next_content)
    {
	if (has_trigger(obj->pIndexData->progs, type, EXEC_ALL))
	{
	    for (prg = obj->pIndexData->progs; prg; prg = prg->next)
		if (prg->trig_type == type
		    && !str_prefix(cmd, prg->trig_phrase))
		{
		    retval = program_flow(prg->vnum, prg->code,
					  NULL, obj, NULL,
					  ch, args, NULL);
		    found = TRUE;
		}

/*
	    for (lprg = obj->pIndexData->lprg; lprg && !found; lprg = lprg->next)
		if (lprg->trig == trig && !str_prefix(cmd, lprg->arg))
		{
		    raise_signal(lprg, obj, LTYPE_OBJ, ch, LTYPE_CHAR, "s", args);
		    found = TRUE;
		    break;
		}
 */
	}
    }

    if (!found)
    {
	room = ch->in_room;

	if (has_trigger(room->progs, type, EXEC_ALL))
	{
	    for (prg = room->progs; prg; prg = prg->next)
		if (prg->trig_type == type
		    && !str_prefix(cmd, prg->trig_phrase))
		{
		    retval = program_flow(prg->vnum, prg->code,
					  NULL, NULL, room,
					  ch, args, NULL);
		    found = TRUE;
		    break;
		}

/*
	    for (lprg = room->lprg; lprg && !found; lprg = lprg->next)
	    {
			if (lprg->trig == trig && !str_prefix(cmd, lprg->arg))
			{
		    	raise_signal(lprg, room, LTYPE_ROOM, ch, LTYPE_CHAR, "s", args);
		    	found = TRUE;
		    	break;
			}
	    }
 */
	}
    }

    mob_holylight = FALSE;

    if (new_trig && !found)
	return FALSE;
    else if (found && retval == -1)
    {
	if (new_trig)
	    return FALSE;
	else
	    return TRUE;
    }
    else
	return retval;
}

/* charset=cp1251 */

