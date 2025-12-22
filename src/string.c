/***************************************************************************
 *  File: string.c                                                         *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
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




char *nouns[]  = {

/* Фамилии
    "ова","овой","овой","ову","овой","овой","ову",
    "ева","евой","евой","еву","евой","евой","еву",
    "ина","иной","иной","ину","иной","иной","ину",  */
    
    "унья","уньи","унье","унью","уньей","унье","унья",

    "ень","ня","ню","ня","нем","не","ень",
    "знь","зни","зни","знь","знью","зни","знь",

    "чий","чего","чему","чего","чим","чем","чий",
    "гий","гого","гому","гого","гим","гом","гий",
    "кий","кего","кему","кего","ким","кем","кий",
    "хий","хего","хему","хего","хим","хем","хий",
    "жий","жего","жему","жего","жим","жем","жий",
    "ший","шего","шему","шего","шим","шем","ший",
    "щий","щего","щему","щего","щим","щем","щий",

    "чей","чья","чью","чья","чьем","чье","чей",

    "ний","него","нему","него","ним","нем","ний",
    "няя","ней","ней","нюю","ней","ней","нюю",
    "нее","него","нему","нее","ним","нем","нее",
/*  "ние","них","ним","них","ними","них","ние",  */

    "еек","ейка","ейку","ейка","ейком","ейке","еек",
    "тки","ток","ткам","ток","тками","тках","тки",
    "дья","дьи","дье","дью","дьей","дье","дья",
    "лец","льца","льцу","льца","льцом","льце","лец",
    "лек","лька","льку","лька","льком","льке","лек",

    "ки","ков","кам","ков","ками","ках","ки",
    "чи","чей","чам","чей","чами","чах","чи",

    "ый","ого","ому","ого","ым","ом","ый",
    "ой","ого","ому","ого","ым","ом","ой",
    "ий","ого","ому","ого","ым","ом","ий",
    "ая","ой","ой","ую","ой","ой","ую",
    "ое","ого","ому","ое","ым","ом","ое",
/*  "ые","ых","ым","ых","ыми","ых","ые", */


/* Фамилии
    "ов","ова","ову","ова","овым","ове","ова",
    "ев","ева","еву","ева","евым","еве","ева",
    "ин","ина","ину","ина","иным","ине","ина",  */

    "ец","ца","цу","ца","цом","це","ец",
    "ье","ья","ью","ья","ьем","ье","ье",


    "ре","ря","рю","ре","рем","ре","ре",
    "ле","ля","лю","ле","лем","ле","ле",

    "мя","мени","мени","мя","менем","мени","мя",

    "ие","ия","ию","ие","ием","ии","ие",

    "ия","ии","ии","ию","ией","ии","ию",

    "ть","ти","ти","ть","тью","ти","ть",

    "ек","ка","ку","ка","ком","ке","ек",
    "ок","ка","ку","ка","ком","ке","ок",

    "га","ги","ге","гу","гой","ге","гу",
    "ка","ки","ке","ку","кой","ке","ку",
    "ха","хи","хе","ху","хой","хе","ху",
    "жа","жи","же","жу","жой","же","жу",
    "ша","ши","ше","шу","шой","ше","шу",
    "ча","чи","че","чу","чой","че","чу",
    "ща","щи","ще","щу","щой","ще","щу",
    "ца","цы","це","цу","цей","це","цу",
    "а","ы","е","у","ой","е","у",

    "о","а","у","о","ом","е","о",
    "е","а","у","е","ем","е","е",

    "б","ба","бу","ба","бом","бе","б",
    "в","ва","ву","ва","вом","ве","в",
    "г","га","гу","га","гом","ге","г",
    "д","да","ду","да","дом","де","д",
    "з","за","зу","за","зом","зе","з",
    "й","я","ю","я","ем","е","й",
    "ж","жа","жу","жа","жом","же","ж",
    "к","ка","ку","ка","ком","ке","к",
    "л","ла","лу","ла","лом","ле","л",
    "м","ма","му","ма","мом","ме","м",
    "н","на","ну","на","ном","не","н",
    "п","па","пу","па","пом","пе","п",
    "р","ра","ру","ра","ром","ре","р",
    "с","са","су","са","сом","се","с",
    "т","та","ту","та","том","те","т",
    "ф","фа","фу","фа","фом","фе","ф",
    "х","ха","ху","ха","хом","хе","х",
    "ч","ча","чу","ча","чом","че","ч",
    "ш","ша","шу","ша","шом","ше","ш",
    "щ","ща","щу","ща","щом","ще","щ",
    "ц","ца","цу","ца","цом","це","ц",

    "ь","я","ю","я","ем","е","ь",
    "ы","ов","ам","ов","ами","ах","ы",

    "я","и","е","ю","ей","е","ю",
    NULL

};

char *nouns_many[]  = {
    "ья","ьев","ьям","ьев","ьями","ьях","ья",
    "на","ен","нам","ен","нами","нах","на",
    "ы","","ам","","ами","ах","ы",
    "и","","ам","","ами","ах","и",
    NULL
};

char *adject[] = {

    "ийся","егося","емуся","егося","имся","емся","ийся",
    "иние","иних","иним","иние","иними","иних","иние",
    "ской","ского","скому","ского","ским","ском","ской",
    "ские","ских","ским","ских","скими","ских","ские",

    "чий","чего","чему","чего","чим","чем","чий",
    "чья","чью","чьей","чью","чьей","чьей","чья",
    "гий","гого","гому","гого","гим","гом","гий",
    "кий","кого","кому","кого","ким","ком","кий",
    "кое","кого","кому","кого","ким","ком","кое",
    "кие","ких","ким","кие","ким","ких","кие",
    "хий","хего","хему","хего","хим","хем","хий",
    "жий","жего","жему","жего","жим","жем","жий",
    "ший","шего","шему","шего","шим","шем","ший",
    "шой","шого","шому","шого","шим","шом","шой",
    "щий","щего","щему","щего","щим","щем","щий",
    "щие","щих","щим","щих","щими","щих","щие",
    "щая","щей","щей","щую","щей","щей","щую",
    "чая","чей","чей","чую","чей","чей","чую",

    "ных","ных","ных","ных","ных","ных","ных",
    "ний","него","нему","него","ним","нем","ний",
    "няя","ней","ней","нюю","ней","ней","нюю",
    "нее","него","нему","нее","ним","нем","нее",
    "ее","его","ему","ее","им","ем","ее",
/*  "ние","них","ним","них","ними","них","ние", */

    "ый","ого","ому","ого","ым","ом","ый",
    "ой","ого","ому","ого","ым","ом","ой",
    "ий","ого","ому","ого","им","ом","ий",
    "ая","ой","ой","ую","ой","ой","ую",
    "ое","ого","ому","ое","ым","ом","ое",
    "ые","ых","ым","ых","ыми","ых","ые",
    "ья","ьей","ьей","ью","ьей","ьей","ью",

    NULL

};



#define CHECK_DELIMITER(c) 	(isspace(c) || c == '.' || c == ',' || c == '!' || c == '\n' || c == '\r' || c == ':' || c == '?')





char *numlines(char *string);
char *string_linedel(char *string, int line);
char *string_lineadd(char *string, int line, char *newstr);


/*****************************************************************************
 Name:		string_edit
 Purpose:	Clears string and puts player into editing mode.
 Called by:	none
 ****************************************************************************/
void string_edit(CHAR_DATA *ch, char **pString)
{
    send_to_char("-========- Entering EDIT Mode -=========-\n\r", ch);
    send_to_char("    Type :h on a new line for help\n\r", ch);
    send_to_char(" Terminate with a ~ or @ on a blank line.\n\r", ch);
    send_to_char("-=======================================-\n\r", ch);

    if (*pString == NULL)
    {
        *pString = str_dup("");
    }
    else
    {
        **pString = '\0';
    }

    ch->desc->pString = pString;

    return;
}



/*****************************************************************************
 Name:		string_append
 Purpose:	Puts player into append mode for given string.
 Called by:	(many)olc_act.c
 ****************************************************************************/
void string_append(CHAR_DATA *ch, char **pString)
{
    send_to_char("-====- Вход в режим редактирования -====-\n\r", ch);
    send_to_char("  Набери :h на новой строке для помощи\n\r", ch);
    send_to_char(" Для окончания редактирования набери  @\n\r", ch);
    send_to_char("            на новой строке.\n\r", ch);
    send_to_char("-=======================================-\n\r", ch);

    if (*pString == NULL)
    {
        *pString = str_dup("");
    }
    send_to_char(numlines(*pString), ch);

    if (strlen(*pString) == 0 || *(*pString + strlen(*pString) - 1) != '\r')
	send_to_char("\n\r", ch);

    ch->desc->pString = pString;

    return;
}



/*****************************************************************************
 Name:		string_replace
 Purpose:	Substitutes one string for another.
 Called by:	string_add(string.c) (aedit_builder)olc_act.c.
 ****************************************************************************/
char *string_replace(char *orig, char *old, char *newstr)
{
    char xbuf[MAX_STRING_LENGTH];
    int i;

    xbuf[0] = '\0';
    strcpy(xbuf, orig);
    if (strstr(orig, old) != NULL)
    {
        i = strlen(orig) - strlen(strstr(orig, old));
        xbuf[i] = '\0';
        strcat(xbuf, newstr);
        strcat(xbuf, &orig[i+strlen(old)]);
        free_string(orig);
    }

    return str_dup(xbuf);
}



void save_clans();

/*****************************************************************************
 Name:		string_add
 Purpose:	Interpreter for string editing.
 Called by:	game_loop_xxxx(comm.c).
 ****************************************************************************/
void string_add(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    int ln;

    /*
     * Thanks to James Seng
     */
    smash_tilde(argument);

    if (*argument == '.'
	|| *argument == ':')
    {
        char arg1 [MAX_INPUT_LENGTH];
        char arg2 [MAX_INPUT_LENGTH];
        char arg3 [MAX_INPUT_LENGTH];

        argument = one_argument(argument, arg1);
        argument = first_arg(argument, arg2, FALSE);
        argument = first_arg(argument, arg3, FALSE);

        if (!str_cmp(arg1, ".c")
	    || !str_cmp(arg1, ":c"))
        {
            send_to_char("Строки удалены.\n\r", ch);

            free_string(*ch->desc->pString);
            *ch->desc->pString = str_dup("");
            return;
        }

        if (!str_cmp(arg1, ".s")
	    || !str_cmp(arg1, ":s"))
        {
            send_to_char("Все строки:\n\r", ch);
            send_to_char(numlines(*ch->desc->pString), ch);
            return;
        }

        if (!str_cmp(arg1, ".r")
	    || !str_cmp(arg1, ":r"))
        {
            if (arg2[0] == '\0')
            {
                send_to_char(
                    "usage:  :r \"old string\" \"new string\"\n\r", ch);
                return;
            }

	    smash_tilde(arg3);   /* Just to be sure -- Hugin */
            *ch->desc->pString =
                string_replace(*ch->desc->pString, arg2, arg3);
            sprintf(buf, "'%s' заменено на '%s'.\n\r", arg2, arg3);
            send_to_char(buf, ch);
            return;
        }

        if (!str_cmp(arg1, ".f")
	    || !str_cmp(arg1, ":f"))
        {
            *ch->desc->pString = format_string(*ch->desc->pString);
            send_to_char("Строка отформатирована.\n\r", ch);
            return;
        }

	if (!str_cmp(arg1, ".ld")
	    || !str_cmp(arg1, ":ld"))
	{
	    if (!is_number(arg2)
		|| (ln = atoi(arg2)) < 1)
	    {
		send_to_char("Параметр должен быть ненулевым полжительным числом.\n\r", ch);
		return;
	    }

	    *ch->desc->pString = string_linedel(*ch->desc->pString, ln);
	    sprintf(buf, "Строка #%d удалена.\n\r", ln);
	    send_to_char(buf, ch);
	    return;
	}

	if (!str_cmp(arg1, ".li")
	    || !str_cmp(arg1, ":li"))
	{
	    if (!is_number(arg2)
		|| (ln = atoi(arg2)) < 1)
	    {
		send_to_char("Параметр должен быть ненулевым полжительным числом.\n\r", ch);
		return;
	    }

	    if (arg3[0] == '\0')
	    {
		send_to_char("Вставить что?\n\r", ch);
		return;
	    }

	    *ch->desc->pString = string_lineadd(*ch->desc->pString, ln, arg3);
	    sprintf(buf, "Строка #%d была вставлена.\n\r--> %s\n\r", ln, arg3);
	    send_to_char(buf, ch);
	    return;
	}

	if (!str_cmp(arg1, ".lr")
	    || !str_cmp(arg1, ":lr"))
	{
	    if (!is_number(arg2)
		|| (ln = atoi(arg2)) < 1)
	    {
		send_to_char("Второй параметр должен быть ненулевым положительным числом.\n\r", ch);
		return;
	    }

	    if (arg3[0] == '\0')
	    {
		send_to_char("Заменить эту строку чем?\n\r", ch);
		return;
	    }

	    *ch->desc->pString = string_linedel(*ch->desc->pString, ln);
	    *ch->desc->pString = string_lineadd(*ch->desc->pString, ln, arg3);

	    sprintf(buf, "Строка #%d заменена на '%s'.\n\r", ln, arg3);
	    send_to_char(buf, ch);
	    return;
	}


        if (!str_cmp(arg1, ".h")
	    || !str_cmp(arg1, ":h"))
        {
            send_to_char("Помощь по редактору строк (команды вводятся на новой строке):\n\r", ch);
            send_to_char(":h                    - посмотреть помощь (эта информация)\n\r", ch);
            send_to_char(":s                    - показать все существующие строки\n\r", ch);
            send_to_char(":r 'старая' 'новая'   - изменить подстроку\n\r", ch);
            send_to_char("                        (требует '', \"\") \n\r", ch);
            send_to_char(":ld <номер>           - удалить стороку <номер>\n\r", ch);
            send_to_char(":li <номер> 'строка'  - вставить 'строку' {yдо{x строки <номер>\n\r", ch);
            send_to_char("                        (требует '', \"\") \n\r", ch);
            send_to_char(":lr <номер> 'строка'  - заменить строку <номер> со 'строкой'\n\r", ch);
            send_to_char("                        (требует '', \"\") \n\r", ch);
            send_to_char(":f                    - (подогнать) строки \n\r", ch);
            send_to_char(":c                    - очистить все сообщение\n\r", ch);
            send_to_char("@                     - закончить редактирование\n\r", ch);
            return;
        }


        send_to_char("SEdit:  Неизвестная команда.\n\r", ch);
        return;
    }

    if (*argument == '~' || *argument == '@')
    {
	if (ch->desc->editor == ED_MPCODE)
	{
	    MOB_INDEX_DATA *mob;
	    int hash;
    	    PROG_LIST *mpl;
	    PROG_CODE *mpc;

	    EDIT_MPCODE(ch, mpc);

	    if (mpc != NULL)
		for (hash = 0; hash < MAX_KEY_HASH; hash++)
		    for (mob = mob_index_hash[hash]; mob; mob = mob->next)
			for (mpl = mob->progs; mpl; mpl = mpl->next)
			    if (mpl->vnum == mpc->vnum)
			    {
				sprintf(buf, "Arreglando mob %d.\n\r", mob->vnum);
				send_to_char(buf, ch);
				mpl->code = mpc->code;
	    		    }
	}
	if (ch->desc->editor == ED_OPCODE) /* for the objprogs */
	{
	    OBJ_INDEX_DATA *obj;
	    int hash;
	    PROG_LIST *opl;
	    PROG_CODE *opc;

    	    EDIT_OPCODE(ch, opc);

	    if (opc != NULL)
		for (hash = 0; hash < MAX_KEY_HASH; hash++)
		    for (obj = obj_index_hash[hash]; obj; obj = obj->next)
			for (opl = obj->progs; opl; opl = opl->next)
			    if (opl->vnum == opc->vnum)
			    {
				sprintf(buf, "Fixing object %d.\n\r", obj->vnum);
				send_to_char(buf, ch);
				opl->code = opc->code;
    			    }
	}

	if (ch->desc->editor == ED_RPCODE) /* for the roomprogs */
	{
	    ROOM_INDEX_DATA *room;
	    int hash;
	    PROG_LIST *rpl;
	    PROG_CODE *rpc;

    	    EDIT_RPCODE(ch, rpc);

	    if (rpc != NULL)
		for (hash = 0; hash < MAX_KEY_HASH; hash++)
		    for (room = room_index_hash[hash]; room; room = room->next)
			for (rpl = room->progs; rpl; rpl = rpl->next)
			    if (rpl->vnum == rpc->vnum)
			    {
				sprintf(buf, "Fixing room %d.\n\r", room->vnum);
				send_to_char(buf, ch);
				rpl->code = rpc->code;
    			    }
	}

	save_clans();

        ch->desc->pString = NULL;
        return;
 
    }

    /*
     * Truncate strings to MAX_STRING_LENGTH.
     * --------------------------------------
     */
    if (strlen(*ch->desc->pString) + strlen(argument) >= (MAX_STRING_LENGTH - 4))
    {
        send_to_char("Строка слишком длинная, обрезана.\n\r", ch);

	/* Force character out of editing mode. */
        ch->desc->pString = NULL;
        return;
    }
    
    strcpy(buf, *ch->desc->pString);
    /*
     * Ensure no tilde's inside string.
     * --------------------------------
     */
    smash_tilde(argument);

    strcat(buf, argument);
    strcat(buf, "\n\r");
    free_string(*ch->desc->pString);
    *ch->desc->pString = str_dup(buf);
    return;
}



/*
 * Thanks to Kalgen for the new procedure (no more bug!)
 * Original wordwrap() written by Surreality.
 */
/*****************************************************************************
 Name:		format_string
 Purpose:	Special string formating and word-wrapping.
 Called by:	string_add(string.c) (many)olc_act.c
 ****************************************************************************/
char *format_string(char *oldstring /*, bool fSpace */)
{
    char xbuf[MAX_STRING_LENGTH];
    char xbuf2[MAX_STRING_LENGTH];
    char *rdesc;
    int i = 0;
    bool cap = TRUE;

    xbuf[0] = xbuf2[0] = 0;

    i = 0;

    for (rdesc = oldstring; *rdesc; rdesc++)
    {
	if (*rdesc=='\n')
	{
	    if (xbuf[i - 1] != ' ')
	    {
		xbuf[i] = ' ';
		i++;
	    }
	}
	else if (*rdesc == '\r')
	    ;
	else if (*rdesc == ' ')
	{
	    if (xbuf[i - 1] != ' ')
	    {
		xbuf[i] = ' ';
		i++;
	    }
	}
	else if (*rdesc == ')')
	{
	    if (xbuf[i - 1] == ' ' 
		&& (xbuf[i - 2] == '.'
		    || xbuf[i - 2] == '?'
		    || xbuf[i - 2] == '!'))
	    {
		xbuf[i - 1] = *rdesc;
		xbuf[i] = ' ';
		i++;
	    }
	    else
	    {
		xbuf[i] = *rdesc;
		xbuf[i + 1] = ' ';
		i += 2;
	    }
	}
	else if (*rdesc == '.' || *rdesc == '?' || *rdesc == '!')
	{
	    if (xbuf[i - 1] == ' '
		&& (xbuf[i - 2] == '.'
		    || xbuf[i - 2] == '?'
		    || xbuf[i - 2] == '!'
		    || xbuf[i - 2] == ')'))
	    {
		xbuf[i - 1] = *rdesc;
		if (*(rdesc + 1) != '"')
		{
		    xbuf[i] = ' ';
		    i++;
		}
		else
		{
		    xbuf[i - 1] = '"';
		    xbuf[i] = ' ';
		    i++;
		    rdesc++;
		}
	    }
	    else
	    {
		xbuf[i] = *rdesc;
		if (*(rdesc + 1) != '"')
		{
		    xbuf[i + 1] = ' ';
		    i += 2;
		}
		else
		{
		    xbuf[i + 1] = '"';
		    xbuf[i + 2] = ' ';
		    i += 3;
		    rdesc++;
		}
	    }
    	    cap = TRUE;
	}
	else if (*rdesc == ',')
	{
	    xbuf[i] = *rdesc;
	    xbuf[i + 1] = ' ';
	    i += 2;
	}
	else if (*rdesc == '(' &&
		 (xbuf[i - 1] != ' '
		  || xbuf[i - 1] != '\t'
		  || xbuf[i - 1] != '\n'
		  || xbuf[i - 1] != '\r'))
	{
	    xbuf[i] = ' ';
	    xbuf[i + 1] = *rdesc;
	    i += 2;
	}
	else
	{
	    xbuf[i] = *rdesc;
	    if (cap)
	    {
		cap = FALSE;
		xbuf[i] = UPPER(xbuf[i]);
	    }
	    i++;
	}
    }
    xbuf[i] = 0;
    strcpy(xbuf2, xbuf);

    rdesc = xbuf2;

    xbuf[0] = 0;

    for (;;)
    {
	for (i = 0; i < 77; i++)
	{
	    if (!*(rdesc + i))
		break;
	}
	
	if (i < 77)
	{
	    break;
	}
	
	for (i = (xbuf[0] ? 76 : 73); i; i--)
	{
	    if (*(rdesc + i) == ' ') break;
	}
	
	if (i)
	{
	    *(rdesc + i) = 0;
	    strcat(xbuf, rdesc);
	    strcat(xbuf, "\n\r");
	    rdesc += i + 1;
	    while (*rdesc == ' ') rdesc++;
	}
	else
	{
	    bugf("No spaces");
	    *(rdesc + 75) = 0;
	    strcat(xbuf, rdesc);
	    strcat(xbuf, "-\n\r");
	    rdesc += 76;
	}
    }

    while (*(rdesc + i)
	   && (*(rdesc + i) == ' '
	       || *(rdesc + i) == '\n'
	       || *(rdesc + i) == '\r'))
    {
	i--;
    }
    
    *(rdesc + i + 1) = 0;
    strcat(xbuf, rdesc);
    if (xbuf[strlen(xbuf) - 2] != '\n')
	strcat(xbuf, "\n\r");

    free_string(oldstring);
    return(str_dup(xbuf));
}



/*
 * Used above in string_add.  Because this function does not
 * modify case if fCase is FALSE and because it understands
 * parenthesis, it would probably make a nice replacement
 * for one_argument.
 */
/*****************************************************************************
 Name:		first_arg
 Purpose:	Pick off one argument from a string and return the rest.
 		Understands quates, parenthesis (barring) ('s) and
 		percentages.
 Called by:	string_add(string.c)
 ****************************************************************************/
char *first_arg(char *argument, char *arg_first, bool fCase)
{
    char cEnd;

    while (*argument == ' ')
	argument++;

    cEnd = ' ';
    if (*argument == '\'' || *argument == '"'
	|| *argument == '%'  || *argument == '(')
    {
	if (*argument == '(')
	{
	    cEnd = ')';
	    argument++;
	}
	else cEnd = *argument++;
    }

    while (*argument != '\0')
    {
	if (*argument == cEnd)
	{
	    argument++;
	    break;
	}
	
	if (fCase)
	    *arg_first = LOWER(*argument);
	else
	    *arg_first = *argument;
	
	arg_first++;
	argument++;
    }
    *arg_first = '\0';

    while (*argument == ' ')
	argument++;

    return argument;
}




/*
 * Used in olc_act.c for aedit_builders.
 */
char *string_unpad(char *argument)
{
    char buf[2*MAX_STRING_LENGTH];
    char *s;

    s = argument;

    while (*s == ' ')
        s++;

    strcpy(buf, s);
    s = buf;

    if (*s != '\0')
    {
        while (*s != '\0')
            s++;
        s--;

        while(*s == ' ')
            s--;
        s++;
        *s = '\0';
    }

    free_string(argument);
    return str_dup(buf);
}

char *string_remspaces(char *argument)
{
    char buf[2*MAX_STRING_LENGTH];
    char *s;
    int i = 0;

    s = argument;

    while (*s == ' ')
	s++;

    while(*s != '\0')
    {
	buf[i++] = *s;
	s++;

	if (*s == ' ')
	{
	    buf[i++] = *s;
	    while (*s == ' ')
		s++;
	}
    }

    buf[i] = '\0';
    return str_dup(buf);
}


/*
 * Same as capitalize but changes the pointer's data.
 * Used in olc_act.c in aedit_builder.
 */
char *string_proper(char *argument)
{
    char *s;

    s = argument;

    while (*s != '\0')
    {
        if (*s != ' ')
        {
            *s = UPPER(*s);
            while (*s != ' ' && *s != '\0')
                s++;
        }
        else
        {
            s++;
        }
    }

    return argument;
}


char *endings(char *word, int num, bool noun_or_adj, bool plural) 
{
    int i;
    char *tmp;
    static char wd[MAX_INPUT_LENGTH];
    EXCEPT_DATA *ex;
    size_t word_len = strlen(word);
    size_t tmp_len;

    strcpy(wd, word);

    for (ex = exceptions; ex != NULL; ex = ex->next)
	if (!str_cmp(wd, ex->cases[0]))
	    return ex->cases[num];

    if (noun_or_adj)        
    {
	if (plural)
	{
	    for (i = 0; nouns_many[i] != NULL; i += 7)
	    {
	        tmp_len = strlen(nouns_many[i]);
	        if (word_len < tmp_len)
		    continue;

	        tmp = wd + word_len - tmp_len;
	        if (!str_cmp(tmp, nouns_many[i]))
	        {
		    strcpy(tmp, nouns_many[i + num]);
		    return wd;  
	        }
	    }
	}
	else
	{ 
	    for (i = 0; nouns[i] != NULL; i += 7)
	    {
	        tmp_len = strlen(nouns[i]);
	        if (word_len < tmp_len)
		    continue;

	        tmp = wd + word_len - tmp_len;
	        if (!str_cmp(tmp, nouns[i]))
	        {
		   strcpy(tmp, nouns[i + num]);
	   	   return wd;  
	        }
	    }
	} 

    }
    else
    {
	for (i = 0; adject[i] != NULL; i += 7)
	{
	    tmp_len = strlen(adject[i]);
	    if (word_len < tmp_len)
		continue;

	    tmp = wd + word_len - tmp_len;
	    if (!str_cmp(tmp, adject[i]))
	    {
		strcpy(tmp, adject[i + num]);
		return wd;  
	    }
	} 
    } 
    return wd;
}

char *one_arg(char *argument, char *arg_first)
{
    if (IS_NULLSTR(argument))
    {
	*arg_first = '\0';
	return argument;
    }

    while (IS_SPACE(*argument))
	argument++;

    while (*argument != '\0')
    {
	if (*argument == ' ')
	{
	    argument++;
	    break;
	}
	*arg_first = *argument;
	arg_first++;
	argument++;
    }
    *arg_first = '\0';

    while (IS_SPACE(*argument))
    	argument++;

    return argument;
}

bool is_adjective(char *wd, bool plural)
{
    EXCEPT_DATA *ex;
    int i, aln = 0, nln = 0;
    size_t wd_len = strlen(wd);
    size_t tmp_len;

    if (is_number(wd) || wd_len < 2)
	return TRUE;
    
    if (plural)
        return FALSE;
        	
    for (ex = exceptions; ex != NULL; ex = ex->next)
	if (!str_cmp(wd, ex->cases[0]))
	    return (ex->type == ADJECT);

    for (i = 0; adject[i] != NULL; i += 7)
    {
	tmp_len = strlen(adject[i]);

	if (wd_len < tmp_len)
	    continue;
	
	if (!str_cmp(adject[i], wd + wd_len - tmp_len))
	{
	    aln = tmp_len;
	    break;
	}
    }

    for (i = 0; nouns[i] != NULL; i += 7)
    {
	tmp_len = strlen(nouns[i]);

	if (wd_len < tmp_len)
	    continue;
	
	if (!str_cmp(nouns[i], wd + wd_len - tmp_len))
	{
	    nln = tmp_len;
	    break;
	}
    }

    if (aln == 0 && nln == 0)
	return FALSE;

    return (aln >= nln);
}

/*DEBUG002 */
void get_real_word(char *word, char *begin, char *end){
    *end = '\0';
    *begin = '\0';

    if (strlen(word) < 3)
	return;

    if (word[0] == '{'){
		strcpy(begin, word);
		begin[2] = '\0';
		strcpy(word, word + 2);
		// Переписать For i=2 to len(word) {
		// 	word[y] =
		// }
    }

    if (word[strlen(word) - 2] == '{'){
		strcpy(end, word + strlen(word) - 2);
		end[2] = '\0';
		word[strlen(word) - 2] = '\0';
    }
}

char *get_other_cases_options(void *t, char **word, int *num, bool *plural)
{
    char *delim;
    char *w1 = (char *) t;
   
    *plural = FALSE;

    if (*num == 6) 
    {
	if (!memcmp(w1, "{%", 2))
	    *num = 3;
    }
    else if (*num == 3) 
    {
	if (!memcmp(w1, "{&", 2))
	    *num = 6;
    }

    delim = " ";

    if (!memcmp(w1, "{:", 2))
    {
        char *c;
        int i = 0;

        for(c = w1; *c != '-' && *c != '\0'; c++, i++)
            ;

        if (*c == '-')
        {
            for (c = *word; *c != '-'; c--)
                ;

            *word = c + 1;
            w1[i] = '\0';
            delim = "-";
        }
    }
    else if (!memcmp(w1, "{$", 2))
    {
        *plural = TRUE;
    }

    return delim;
}

// DEBUG003
char *cases(char *phrase, int num){
    char *word, *delim;
    static char buff[MAX_INPUT_LENGTH] = ""; 
    char w1[MAX_INPUT_LENGTH] = "", begin[MIL] = "", end[MIL] = "";
    bool flag;
    int n;
    bool plural;

    if (num < 1 || num > 6 || IS_NULLSTR(phrase)) 
	return phrase;

    word = one_arg(phrase, w1);

    if (IS_NULLSTR(word)){
		CHAR_DATA *wch;

		LIST_FOREACH(wch, &char_list, link){
			if (IS_NPC(wch))
			continue;

			if (!str_cmp(w1, wch->name))
			{
			if (num == 6)
			  num = 3;

			return wch->pcdata->cases[num - 1];
			}
		}
    }

    n = num;

    delim = get_other_cases_options(&w1, &word, &n, &plural);

    get_real_word(w1, begin, end);

    sprintf(buff, "%s%s%s", begin, endings(w1, n, (flag = !is_adjective(w1, plural)), plural), end);

    for (word = one_arg(word, w1); w1[0] != '\0'; word = one_arg(word, w1))
    {
	strcat(buff, delim);

	if (flag && ((*w1 == UPPER(*w1) && IS_ALPHA(*w1)) || !strcmp(delim, "-")))
	    flag = FALSE;

	if (!flag)
	{
	    n = num;

	    delim = get_other_cases_options(&w1, &word, &n, &plural);

	    get_real_word(w1, begin, end);
	    strcat(buff, begin);
	    strcat(buff, endings(w1, n, (flag = !is_adjective(w1, plural)), plural));
	    strcat(buff, end);
	}
	else
	{
	    strcat(buff, w1);
	    if (word[0] != '\0')
	    {
		strcat(buff, delim);
		strcat(buff, word);
	    }
	    break;
	}  
    }

    return buff;
}

int strlen_color(char *argument)
{
    char *str;
    int length;

    if (argument == NULL || argument[0] == '\0')
        return 0;

    length = 0;
    str = argument;

    while (*str != '\0')
    {
        if (*str != '{')
        {
            str++;
            length++;
            continue;
        }

        if (*(++str) == '{')
	    length++;

        str++;
    }

    return length;
}

char *color_to_str(char *argument)
{
    static char buf[MAX_STRING_LENGTH];
    char *str = argument;
    int i = 0;

    while (*str != '\0' && i < MAX_STRING_LENGTH)
    {
	buf[i++] = *str;
	if (*str == '{')
	    buf[i++] = '{';

	str++;
    }
    
    buf[i] = '\0';
    return buf;
}

char *str_color_fmt(char *argument, int fmt)
{
    int len = 0;
    int i = 0;
    static char buf[MAX_STRING_LENGTH];
    char *s;

    for (s = argument; len < fmt && i < sizeof(buf); i++)
    {
	if (*s != '{')
	    len++;
	else if (*s == '{' && *(s + 1) != '{')
	    len--;
	else
	    len++;

	if (*s != '\0')
	{
	    buf[i] = *s;
	    s++;
	}
	else
	    buf[i] = ' ';
    }
    buf[i] = '\0';

    strlcat(buf, "{x", sizeof(buf));
    
    return buf;
}

char *decompose_end(char *name)
{
    char arg[MIL], b[MIL];
    char *argument, *p;
    char last_char;
    bool plural;
    int t = 0;
  
    for (argument = one_arg(name, arg); arg[0] != '\0'; argument = one_arg(argument, arg))
    {
        get_other_cases_options(arg, &p, &t, &plural);
        get_real_word(arg, b, b);

	if (!is_adjective(arg, plural))
	    break;
    }	

    if (arg[0] != '\0')
    {
	last_char = arg[strlen(arg) - 1];

	if (plural || last_char == 'и' || last_char == 'ы')
	    return "ют";
    }	

    return "ет";
}

char *string_linedel(char *string, int line)
{
    char *strtmp = string;
    char buf[MAX_STRING_LENGTH];
    int cnt = 1, tmp = 0;

    buf[0] = '\0';

    for (; *strtmp != '\0'; strtmp++)
    {
	if (cnt != line)
	    buf[tmp++] = *strtmp;
	if (*strtmp == '\n')
	    cnt++;
    }

    buf[tmp] = '\0';
    free_string(string);

    return str_dup(buf);
}

char *string_lineadd(char *string, int line, char *newstr)
{
    char *strtmp = string;
    int cnt = 1, tmp = 0;
    bool done = FALSE;
    char buf[MAX_STRING_LENGTH];

    buf[0] = '\0';

    if (newstr[0] == '.')
	newstr++;
		
    for (; *strtmp != '\0' || (!done && cnt == line); strtmp++)
    {
	if (cnt == line && !done)
	{
	    strcat(buf, newstr);
	    strcat(buf, "\n");
	    tmp += strlen(newstr) + 1;
	    cnt++;
	    done = TRUE;
	}

	buf[tmp++] = *strtmp;

	if (done && *strtmp == '\0')
	    break;

	if (*strtmp == '\n')
	    cnt++;

	buf[tmp] = '\0';
    }

    free_string(string);
    return str_dup(buf);
}

char *getline_balder(char *str, char *buf)
{
    int tmp = 0;
    bool found = FALSE;

    while (*str)
    {
	if (*str == '\n')
	{
	    found = TRUE;
	    break;
	}
	buf[tmp++] = *(str++);
    }

    if (found)
    {
	if (*(str + 1) == '\r')
	    str += 2;
	else
	    str += 1;
    }

    buf[tmp] = '\0';
    return str;
}

char *numlines(char *string)
{
    int cnt = 1;
    static char buf[MAX_STRING_LENGTH*2];
    char buf2[MAX_STRING_LENGTH], tmpb[MAX_STRING_LENGTH];

    buf[0] = '\0';

    while (*string)
    {
	string = getline_balder(string, tmpb);
	sprintf(buf2, "%2d. %s\n", cnt++, tmpb);
	strcat(buf, buf2);
    }

    return buf;
}

#if defined(NO_STRL_FUNCS)
/* This functions taken from FreeBSD libc */
size_t strlcat(char *dst, const char *src, size_t siz)
{
    register char *d = dst;
    register const char *s = src;
    register size_t n = siz;
    size_t dlen;

    /* Find the end of dst and adjust bytes left but don't go past end */
    while (n-- != 0 && *d != '\0')
	d++;

    dlen = d - dst;
    n = siz - dlen;
    if (n == 0)
	return (dlen + strlen(s));

    while (*s != '\0')
    {
	if (n != 1)
       	{
	    *d++ = *s;
	    n--;
	}
	s++;
    }

    *d = '\0';
    return (dlen + (s - src));       /* count does not include NUL */
}

size_t strlcpy(char *dst, const char *src, size_t siz)
{
    register char *d = dst;
    register const char *s = src;
    register size_t n = siz;

    /* Copy as many bytes as will fit */
    if (n != 0 && --n != 0)
    {
	do
	{
	    if ((*d++ = *s++) == 0)
		break;
	} while (--n != 0);
    }
    
    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0)
    {
	if (siz != 0)
	    *d = '\0';              /* NUL-terminate dst */
	while (*s++)
    	    ;
    }
    
    return (s - src - 1);    /* count does not include NUL */
}

#endif 




/* charset=cp1251 */
