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
*	ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@efn.org)				   *
*	    Gabrielle Taylor						   *
*	    Brian Moore (zump@rom.org)					   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Delicate02/doc/rom.license                *
***************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"


/* How to make a string look drunk... by Apex (robink@htsa.hva.nl) */
/* Modified and enhanced for envy(2) by the Maniac from Mythran */
/* Further mods/upgrades for ROM 2.4 by Kohl Desenee */

char * makedrunk (char *string, CHAR_DATA * ch)
{
    /* This structure defines all changes for a character */
    struct struckdrunk drunk[] =
    {
	{3, 10,
	    {"à", "à", "à", "À", "àà", "àõ", "Àõ", "àî", "àâ", "îà", "àõõõõ"} },
	{8, 5,
	    {"á", "á", "á", "Á", "Á", "âá"}},
	{3, 5,
	    {"â", "â", "Â", "âè", "àâ", "ñâ"}},
	{3, 5,
	    {"ã", "ã", "Ããã", "ãû", "ãàâ", "ãàãà"}},
	{5, 2,
	    {"ä", "ä", "Ä"}},
	{3, 3,
	    {"å", "å", "åõ", "Å"}},
/*    {3, 3,
     {"¸", "¸¸", "¸ìîå", "¸¨Å"}}, */
	{4, 5,
	    {"æ", "æ", "ææ", "æææ", "æÆæ", "Æ"}},
	{8, 4,
	    {"ç", "ç", "Ç", "ççççç", "ççç"}},
	{9, 6,
	    {"è", "è", "èè", "èèè", "Èèè", "ÈèÈ", "È"}},
	{3, 3,
	    {"é", "éé", "éÉé", "èè"}},
	{7, 6,
	    {"ê", "ê", "Êêê", "êê", "êÊ", "Êê", "Ê"}},
	{9, 5,
	    {"ë", "ë", "ëë", "Ëë", "ëË", "ëÿ"}},
	{7, 5,
	    {"ì", "ì", "Ì", "ììì", "ììý", "ìàìà"}},
	{3, 5,
	    {"í", "í", "Í", "ííí", "Íí", "ííó"}},
	{5, 8,
	    {"î", "î", "îî", "îîî", "îîîî", "îîîîî", "îóó", "îå", "Î"}},
	{6, 6,
	    {"ï", "ï", "ïï", "Ïï", "ïïï", "ïÏ", "ïû"}},
	{3, 6,
	    {"ð", "ð", "ððð", "àð", "àÐðð", "Ððð", "ððÐð"}},
	{3, 5,
	    {"ñ", "ñ", "Ñ", "ñññ", "òñññ", "ñÑ"}},
	{5, 5,
	    {"ò", "ò", "Ò", "òû", "òóòó", "Òûòà"}},
	{4, 2,
	    {"ó", "óóóó", "Óóó"}},
	{2, 5,
	    {"ô", "ôô", "ôôÔÔ", "Ôñô", "ôûõ", "ôÔ"}},
	{5, 2,
	    {"õ", "õû", "Õ"}},
	{3, 6,
	    {"ö", "ö", "öûõ", "öà", "ÖööÖ", "öóõõ", "óö"}},
	{4, 3,
	    {"÷", "÷", "×", "÷÷÷÷"}},
	{4, 3,
	    {"ø", "ø", "Ù", "øøøø"}},
	{4, 3,
	    {"ù", "ù", "Ù", "øøøø"}},
	{4, 3,
	    {"ú", "ú", "ÜÜ", "úú"}},
	{4, 3,
	    {"û", "ûûûû", "Ûûû", "ûàó"}},
	{4, 3,
	    {"ü", "ü", "ÚÚ", "üü"}},
	{5, 6,
	    {"ý", "ý", "Ý", "ýýýý", "ýÝýó", "ýýýÝ", "Ýêýý"}},
	{3, 3,
	    {"þ", "þ", "Þ", "Þþ"}},
	{2, 9,
	    {"ÿ", "ÿ", "ÿÿÿÿÿ", "ÿßß", "ßÿ", "èÿ", "éà", "èéà", "ßàóó", "óéß"}}
    };

    static char buf[5 * MAX_INPUT_LENGTH];
    char temp;
    int pos = 0;
    int drunklevel;
    int randomnum;

    /* Check how drunk a person is... */
    if (IS_NPC(ch))
	drunklevel = 0;
    else
        drunklevel = ch->pcdata->condition[COND_DRUNK];

    if (drunklevel > 0)
    {
    	do
        {
      	    temp = UPPER(*string);
	    if (temp >= 'À' && temp <= 'ß')
            {
	  	if (drunklevel > drunk[temp - 'À'].min_drunk_level)
                {
	    	    randomnum = number_range(0,
					     drunk[temp - 'À'].number_of_rep);
	      	    strcpy(&buf[pos], drunk[temp - 'À'].replacement[randomnum]);
		    pos += strlen (drunk[temp - 'À'].replacement[randomnum]);
                }
		else
		    buf[pos++] = *string;
            }
	    else
            {
		if ((temp >= '0') && (temp <= '9'))
                {
		    temp = '0' + number_range (0, 9);
		    buf[pos++] = temp;
                }
		else
		    buf[pos++] = *string;
            }
        }
	while (*string++)
	    ;
	    
	buf[pos] = '\0';          /* Mark end of the string... */
	return buf;
    }
    return string;
}

/* charset=cp1251 */
