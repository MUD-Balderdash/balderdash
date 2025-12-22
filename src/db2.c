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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>

#include "merc.h"
#include "db.h"



/* values for db2.c */
struct		social_type	social_table		[MAX_SOCIALS];
int		social_count;

/* handler.c */
void set_resist_flag(int64_t *flag, int dam);
int set_resist_num(int64_t res);
void set_resist(int16_t *resist, int64_t imm, int64_t res, int64_t vuln);

int64_t get_prog_when(FILE *fp)
{
    char c = fgetc(fp);

    if (c == 'B')
        return EXEC_BEFORE;
    else if (c == 'A')
        return EXEC_AFTER;
    else
        return EXEC_DEFAULT;
}

/* snarf a socials file */
void load_socials(FILE *fp)
{
    for (; ;) 
    {
    	struct social_type social;
    	char *temp;

        /* clear social */
	social.char_no_arg = NULL;
	social.others_no_arg = NULL;
	social.char_found = NULL;
	social.others_found = NULL;
	social.vict_found = NULL; 
	social.char_not_found = NULL;
	social.char_auto = NULL;
	social.others_auto = NULL;

    	temp = fread_word(fp);
    	if (!strcmp(temp,"#0"))
    	{
	    return;  /* done */
	}
#if defined(social_debug) 
	else 
	    fprintf(stderr,"%s\n\r",temp);
#endif

    	strcpy(social.name,temp);
    	
    	fread_to_eol(fp);

	temp = fread_string_eol(fp);
	if (!strcmp(temp,"$"))
	     social.char_no_arg = NULL;
	else if (!strcmp(temp,"#"))
	{
	     social_table[social_count] = social;
	     social_count++;
	     free_string(temp);
	     continue; 
	}
        else
	    social.char_no_arg = str_dup(temp);

	free_string(temp);

        temp = fread_string_eol(fp);
        if (!strcmp(temp,"$"))
             social.others_no_arg = NULL;
        else if (!strcmp(temp,"#"))
        {
	     social_table[social_count] = social;
             social_count++;
	     free_string(temp);
             continue;
        }
        else
	    social.others_no_arg = str_dup(temp);

	free_string(temp);

        temp = fread_string_eol(fp);
        if (!strcmp(temp,"$"))
             social.char_found = NULL;
        else if (!strcmp(temp,"#"))
        {
	     social_table[social_count] = social;
             social_count++;
	     free_string(temp);
             continue;
        }
       	else
	    social.char_found = str_dup(temp);

	free_string(temp);

        temp = fread_string_eol(fp);
        if (!strcmp(temp,"$"))
             social.others_found = NULL;
        else if (!strcmp(temp,"#"))
        {
	     social_table[social_count] = social;
             social_count++;
	     free_string(temp);
             continue;
        }
        else
	    social.others_found = str_dup(temp); 

	free_string(temp);

        temp = fread_string_eol(fp);
        if (!strcmp(temp,"$"))
             social.vict_found = NULL;
        else if (!strcmp(temp,"#"))
        {
	     social_table[social_count] = social;
             social_count++;
	     free_string(temp);
             continue;
        }
        else
	    social.vict_found = str_dup(temp);

	free_string(temp);

        temp = fread_string_eol(fp);
        if (!strcmp(temp,"$"))
             social.char_not_found = NULL;
        else if (!strcmp(temp,"#"))
        {
	     social_table[social_count] = social;
             social_count++;
	     free_string(temp);
             continue;
        }
        else
	    social.char_not_found = str_dup(temp);

	free_string(temp);

        temp = fread_string_eol(fp);
        if (!strcmp(temp,"$"))
             social.char_auto = NULL;
        else if (!strcmp(temp,"#"))
        {
	     social_table[social_count] = social;
             social_count++;
	     free_string(temp);
             continue;
        }
        else
	    social.char_auto = str_dup(temp);
         
	free_string(temp);

        temp = fread_string_eol(fp);
        if (!strcmp(temp,"$"))
             social.others_auto = NULL;
        else if (!strcmp(temp,"#"))
        {
             social_table[social_count] = social;
             social_count++;
	     free_string(temp);
             continue;
        }
        else
	    social.others_auto = str_dup(temp); 
	
	free_string(temp);

	social_table[social_count] = social;
    	social_count++;
   }
   return;
}

/*
 * Snarf a mob section.  new style
 */
void load_mobiles(FILE *fp)
{
    MOB_INDEX_DATA *pMobIndex;
    int64_t imm_fl = 0;
    int64_t res_fl = 0;
    int64_t vuln_fl = 0;
    int i;

    if (!area_last)   /* OLC */
    {
        bugf("Load_mobiles: no #AREA seen yet.");
        exit(1);
    }

    for (; ;)
    {
        int  vnum;
        int  iHash;
        char letter;
	char *tstr;
        bool a_sect = FALSE;
 
        letter                          = fread_letter(fp);
        if (letter != '#')
        {
            bugf("Load_mobiles: # not found.");
            exit(1);
        }
 
        vnum                            = fread_number(fp);
        if (vnum == 0)
            break;
 
        fBootDb = FALSE;
        if (get_mob_index(vnum) != NULL)
        {
            bugf("Load_mobiles: vnum %d duplicated.", vnum);
            exit(1);
        }
        fBootDb = TRUE;
        pMobIndex                       = alloc_perm(sizeof(*pMobIndex));
        pMobIndex->vnum                 = vnum;
        pMobIndex->area                 = area_last;               /* OLC */
	pMobIndex->new_format		= TRUE;
	newmobs++;
        pMobIndex->player_name          = fread_string(fp);
        pMobIndex->short_descr          = fread_string(fp);
        pMobIndex->long_descr           = fread_string(fp);
        pMobIndex->description          = fread_string(fp);
	tstr				= fread_string(fp);
	pMobIndex->race		 	= race_lookup(tstr);
	if (pMobIndex->race == -1)
	{
	    bugf("Unknown race [%s] for mob [%d]", tstr, pMobIndex->vnum);
	    pMobIndex->race = 0;	/* spec. value for unique */
	}
	free_string(tstr);
 
        pMobIndex->long_descr[0]        = UPPER(pMobIndex->long_descr[0]);
        pMobIndex->description[0]       = UPPER(pMobIndex->description[0]);
 
        pMobIndex->act                  = fread_flag(fp) | ACT_IS_NPC
					| race_table[pMobIndex->race].act;
        pMobIndex->affected_by          = fread_flag(fp)
					| race_table[pMobIndex->race].aff;

        pMobIndex->pShop                = NULL;
	if (area_last->version < 4)
	    pMobIndex->saving_throw	= 0 - pMobIndex->level;
	else
	    pMobIndex->saving_throw	= fread_number(fp);
        pMobIndex->alignment            = fread_number(fp);
        pMobIndex->group                = fread_number(fp);

        pMobIndex->level                = fread_number(fp);
        pMobIndex->hitroll              = fread_number(fp);  

	/* read hit dice */
        pMobIndex->hit[DICE_NUMBER]     = fread_number(fp);  
        /* 'd'          */                fread_letter(fp); 
        pMobIndex->hit[DICE_TYPE]   	= fread_number(fp);
        /* '+'          */                fread_letter(fp);   
        pMobIndex->hit[DICE_BONUS]      = fread_number(fp); 

 	/* read mana dice */
	pMobIndex->mana[DICE_NUMBER]	= fread_number(fp);
					  fread_letter(fp);
	pMobIndex->mana[DICE_TYPE]	= fread_number(fp);
					  fread_letter(fp);
	pMobIndex->mana[DICE_BONUS]	= fread_number(fp);

        
        /* Коротенький фикс для мобов без маны */
        if (pMobIndex->mana[DICE_BONUS] + 
            dice(pMobIndex->mana[DICE_NUMBER], pMobIndex->mana[DICE_TYPE]) == 0)
            pMobIndex->mana[DICE_BONUS] = pMobIndex->level * 10;

	/* read damage dice */
	pMobIndex->damage[DICE_NUMBER]	= fread_number(fp);
					  fread_letter(fp);
	pMobIndex->damage[DICE_TYPE]	= fread_number(fp);
					  fread_letter(fp);
	pMobIndex->damage[DICE_BONUS]	= fread_number(fp);
	pMobIndex->dam_type		= attack_lookup(fread_word(fp));

	/* read armor class */
	pMobIndex->ac[AC_PIERCE]	= fread_number(fp) * 10;
	pMobIndex->ac[AC_BASH]		= fread_number(fp) * 10;
	pMobIndex->ac[AC_SLASH]		= fread_number(fp) * 10;
	pMobIndex->ac[AC_EXOTIC]	= fread_number(fp) * 10;

	/* read flags and add in data from the race table */
	pMobIndex->off_flags		= fread_flag(fp) 
					| race_table[pMobIndex->race].off;
	if (area_last->version < 2)
	{
	    imm_fl			= fread_flag(fp);
	    res_fl			= fread_flag(fp);
	    vuln_fl			= fread_flag(fp);

	    /* Fix vulns/resists/immunes */
    	    for (i = 0; i < DAM_MAX; i++)
    	    {
    		if (race_table[pMobIndex->race].resists[i] == 100)
    		    set_resist_flag(&imm_fl, i);
    		else if (race_table[pMobIndex->race].resists[i] > 0)
    		    set_resist_flag(&res_fl, i);
    		else if (race_table[pMobIndex->race].resists[i] < 0)
    		    set_resist_flag(&vuln_fl, i);
    	    }
	}

	/* vital statistics */
	pMobIndex->start_pos		= position_lookup(fread_word(fp));
	pMobIndex->default_pos		= position_lookup(fread_word(fp));
	pMobIndex->sex			= sex_lookup(fread_word(fp));

	pMobIndex->wealth		= fread_number(fp);

	
//	if (pMobIndex->vnum < 3700 || pMobIndex->vnum > 3799)
//	  pMobIndex->wealth /= 10;

	pMobIndex->form			= fread_flag(fp)
					| race_table[pMobIndex->race].form;
	pMobIndex->parts		= fread_flag(fp)
					| race_table[pMobIndex->race].parts;
	/* size */
/*	pMobIndex->size			= size_lookup(fread_word(fp));*/
	CHECK_POS(pMobIndex->size, size_lookup(fread_word(fp)), "size");

	pMobIndex->material		= str_dup(fread_word(fp));
	pMobIndex->progs		= NULL;

        for (i = 0; i < MAX_CHATBOOKS; i++)
          pMobIndex->chat_vnum[i] = 0;

	for (i = 0; i < SECT_MAX; i++)
	    pMobIndex->areal[i] = FALSE;
	
	pMobIndex->rating = 0;

	pMobIndex->comments		= str_dup("");
	pMobIndex->last_edited		= str_dup("");

	for (; ;)
        {
            letter = fread_letter(fp);

	    if (letter == 'F')
            {
		char *word;
		int64_t vector;

                word                    = fread_word(fp);
		vector			= fread_flag(fp);

		if (!str_prefix(word,"act"))
		    REMOVE_BIT(pMobIndex->act, vector);
                else if (!str_prefix(word,"aff"))
		    REMOVE_BIT(pMobIndex->affected_by, vector);
		else if (!str_prefix(word,"off"))
		    REMOVE_BIT(pMobIndex->off_flags, vector);
		else if (!str_prefix(word,"imm") && area_last->version < 2)
		    REMOVE_BIT(imm_fl, vector);
		else if (!str_prefix(word,"res") && area_last->version < 2)
		    REMOVE_BIT(res_fl, vector);
		else if (!str_prefix(word,"vul") && area_last->version < 2)
		    REMOVE_BIT(vuln_fl, vector);
		else if (!str_prefix(word,"for"))
		    REMOVE_BIT(pMobIndex->form, vector);
		else if (!str_prefix(word,"par"))
		    REMOVE_BIT(pMobIndex->parts, vector);
		else
		{
		    bugf("Flag remove: flag not found.");
		    exit(1);
		}
	    }
	    else if (letter == 'A')
	    {
	        char *str, *tmp, arg[MIL];

	        fBootDb = FALSE;

	        str = fread_string(fp);

	        for (tmp = one_argument(str, arg);
		     arg[0] != '\0';
		     tmp = one_argument(tmp, arg))
	        {
		    int sect;  

		    if ((sect = flag_lookup(arg, sector_flags)) != NO_FLAG)
		    {
			pMobIndex->areal[sect] = TRUE; 
			a_sect = TRUE;
		    }
	        }

	        free_string(str);
                fBootDb = TRUE;
	    }
	    else if (letter == 'E')
	        pMobIndex->msg_arrive = fread_string(fp);
	    else if (letter == 'G')
	        pMobIndex->msg_gone = fread_string(fp);
	    else if (letter == 'M')
	    {
		PROG_LIST *pMprog;
		char *word;
		int trigger = 0;
		
		pMprog              = alloc_perm(sizeof(*pMprog));
		SET_BIT(pMprog->trig_flags, get_prog_when(fp));

		word   		    = fread_word(fp);
		if (!(trigger = flag_lookup(word, mprog_flags)))
		{
		    bugf("MOBprogs: invalid trigger.");
		    exit(1);
		}
		SET_BIT(pMprog->trig_flags, trigger);
		pMprog->trig_type   = trigger;
		pMprog->vnum        = fread_number(fp);
		pMprog->trig_phrase = fread_string(fp);
		pMprog->next        = pMobIndex->progs;
		pMobIndex->progs   = pMprog;
	    }
	    else if (letter == 'P' && area_last->version > 1)
	    {
		char *name = fread_word(fp);
		int percent = fread_number(fp);
		int res;

		res = flag_lookup(name, dam_flags);
		if (res == NO_FLAG)
		{
		    bugf("Unknown resist type '%s'.", name);
		    exit(1);
		}
		pMobIndex->resists[res] = percent;
       	    }
	    else if (letter == 'C')
	    {
		char *tmp, *argument, arg[MAX_INPUT_LENGTH];

		fBootDb = FALSE;

		tmp = fread_string_eol(fp);

		for (i = 0, argument = one_argument(tmp, arg);
		     i < MAX_CHATBOOKS && arg[0] != '\0';
		     argument = one_argument(argument, arg))
		{
		    if (is_number(arg))
			pMobIndex->chat_vnum[i++] = atoi(arg);
		}

		free_string(tmp);
	    }	
	    else if (letter == 'L')
	    {
		char *tmp;

		fBootDb = FALSE;

		tmp = fread_string(fp);

		pMobIndex->clan = clan_lookup(tmp);

		free_string(tmp);
		fBootDb = TRUE;
	    }	
	    else if (letter == 'R')
	        pMobIndex->rating = fread_number(fp);
	    else if (letter == '*')
	    {
		free_string(pMobIndex->comments);
	        pMobIndex->comments = fread_string(fp);
	    }
	    else if (letter == 'Z')
	    {
		free_string(pMobIndex->last_edited);
	        pMobIndex->last_edited = fread_string(fp);
	    }
	    else
	    {
    		ungetc(letter, fp);
		break;
	    }
	}
	
	if (!a_sect)
  	    for(i = 0; i < SECT_MAX; i++)
    		pMobIndex->areal[i] = TRUE;


	if (area_last->version < 2)
	{
	    /* Fix vulns/resists/immunes */
	    set_resist(pMobIndex->resists, imm_fl, res_fl, vuln_fl);
	}
        iHash                   = vnum % MAX_KEY_HASH;
        pMobIndex->next         = mob_index_hash[iHash];
        mob_index_hash[iHash]   = pMobIndex;
        top_mob_index++;
        top_vnum_mob = top_vnum_mob < vnum ? vnum : top_vnum_mob;  /* OLC */
        top_vnum = top_vnum < vnum ? vnum : top_vnum;
        assign_area_vnum(vnum);                                  /* OLC */
        kill_table[URANGE(0, pMobIndex->level, MAX_LEVEL-1)].number++;
    }
    return;
}

/*
 * Snarf an obj section. new style
 */
void load_objects(FILE *fp)
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
        char letter, *word;
        int iHash, i;
 
        letter                          = fread_letter(fp);
        if (letter != '#')
        {
            bugf("Load_objects: # not found.");
            exit(1);
        }
 
        vnum                            = fread_number(fp);
        if (vnum == 0)
            break;
 
        fBootDb = FALSE;
        if (get_obj_index(vnum) != NULL)
        {
            bugf("Load_objects: vnum %d duplicated.", vnum);
            exit(1);
        }
        fBootDb = TRUE;
 
        pObjIndex                       = alloc_mem(sizeof(*pObjIndex));
        pObjIndex->vnum                 = vnum;
        pObjIndex->area                 = area_last;            /* OLC */
        pObjIndex->new_format           = TRUE;
	pObjIndex->reset_num		= 0;
	newobjs++;
        pObjIndex->name                 = fread_string(fp);
        pObjIndex->short_descr          = fread_string(fp);
        pObjIndex->description          = fread_string(fp);
        pObjIndex->material		= fread_string(fp);
        CHECK_POS(pObjIndex->item_type, item_lookup(fread_word(fp)), "item_type");
        pObjIndex->extra_flags          = fread_flag(fp);
        pObjIndex->wear_flags           = fread_flag(fp);
        pObjIndex->edited 		= 0;
	pObjIndex->affected		= NULL;
	pObjIndex->progs		= NULL;

	pObjIndex->comments		= str_dup("");
	pObjIndex->last_edited		= str_dup("");

	switch(pObjIndex->item_type)
	{
	case ITEM_WEAPON:
	    pObjIndex->value[0]		= weapon_type(fread_word(fp));
	    pObjIndex->value[1]		= fread_number(fp);
	    pObjIndex->value[2]		= fread_number(fp);
	    pObjIndex->value[3]		= attack_lookup(fread_word(fp));
	    pObjIndex->value[4]		= fread_flag(fp);
	    break;
	case ITEM_CONTAINER:
	    pObjIndex->value[0]		= fread_number(fp);
	    pObjIndex->value[1]		= fread_flag(fp);
	    pObjIndex->value[2]		= fread_number(fp);
	    pObjIndex->value[3]		= fread_number(fp);
	    pObjIndex->value[4]		= fread_number(fp);
	    break;
	case ITEM_CHEST:
	    pObjIndex->value[0]		= fread_number(fp);
	    pObjIndex->value[1]		= fread_flag(fp);
	    pObjIndex->value[2]		= fread_number(fp);
	    pObjIndex->value[3]		= fread_number(fp);
	    pObjIndex->value[4]		= fread_number(fp);
	    break;
	case ITEM_MORTAR:
	    pObjIndex->value[0]		= fread_number(fp);
	    pObjIndex->value[1]		= fread_flag(fp);
	    pObjIndex->value[2]		= fread_number(fp);
	    pObjIndex->value[3]		= fread_number(fp);
	    pObjIndex->value[4]		= fread_number(fp);
	    break;
	case ITEM_INGREDIENT:
	    pObjIndex->value[0]		= fread_number(fp);
	    pObjIndex->value[1]		= fread_flag(fp);
	    pObjIndex->value[2]		= fread_number(fp);
	    pObjIndex->value[3]		= fread_flag(fp);
	    pObjIndex->value[4]		= fread_number(fp);
	    break;
        case ITEM_DRINK_CON:
	case ITEM_FOUNTAIN:
            pObjIndex->value[0]         = fread_number(fp);
            pObjIndex->value[1]         = fread_number(fp);
            pObjIndex->value[2]         = liq_lookup(fread_word(fp));
            pObjIndex->value[3]         = fread_number(fp);
            pObjIndex->value[4]         = fread_number(fp);
            break;
	case ITEM_WAND:
	case ITEM_STAFF:
	    pObjIndex->value[0]		= fread_number(fp);
	    pObjIndex->value[1]		= fread_number(fp);
            pObjIndex->value[2]		= fread_number(fp);
	    pObjIndex->value[3]		= skill_lookup(fread_word(fp));
	    pObjIndex->value[4]		= fread_number(fp);
	    break;
	case ITEM_ROD:
	    pObjIndex->value[0]		= fread_number(fp);
	    pObjIndex->value[1]		= skill_lookup(fread_word(fp));
	    pObjIndex->value[2]		= fread_number(fp);
	    pObjIndex->value[3]		= fread_number(fp);
            pObjIndex->value[3]		= pObjIndex->value[2];
	    pObjIndex->value[4]		= fread_number(fp);	/* unused */
	    break;
	case ITEM_TRAP:
	    pObjIndex->value[0]		= fread_number(fp);
	    pObjIndex->value[1]		= fread_number(fp);
	    pObjIndex->value[2]		= fread_number(fp);     /* unused */
	    word			= fread_word(fp);
	    pObjIndex->value[3]		= is_number(word) ? atoi(word) : attack_lookup(word);
	    if (pObjIndex->value[3] <= 0 || pObjIndex->value[3] >= MAX_DAMAGE_MESSAGE)
		pObjIndex->value[3] = DAM_SLASH;
	    pObjIndex->value[4]		= fread_number(fp);
	    if (pObjIndex->value[4] == 0)
		pObjIndex->value[4] = 3 * PULSE_VIOLENCE;
	    break;
	case ITEM_POTION:
	case ITEM_PILL:
	case ITEM_SCROLL:
 	    pObjIndex->value[0]		= fread_number(fp);
	    pObjIndex->value[1]		= skill_lookup(fread_word(fp));
	    pObjIndex->value[2]		= skill_lookup(fread_word(fp));
	    pObjIndex->value[3]		= skill_lookup(fread_word(fp));
	    pObjIndex->value[4]		= skill_lookup(fread_word(fp));
	    break;
	case ITEM_FURNITURE:
	    pObjIndex->value[0]		= fread_number(fp);
	    pObjIndex->value[1]		= fread_number(fp);
	    pObjIndex->value[2]		= fread_flag(fp);
	    pObjIndex->value[3]		= fread_number(fp);
	    pObjIndex->value[4]		= fread_flag(fp);
	    break;
	default:
            pObjIndex->value[0]             = fread_flag(fp);
            pObjIndex->value[1]             = fread_flag(fp);
            pObjIndex->value[2]             = fread_flag(fp);
            pObjIndex->value[3]             = fread_flag(fp);
	    pObjIndex->value[4]		    = fread_flag(fp);
	    break;
	}
	pObjIndex->level		= fread_number(fp);
        pObjIndex->weight               = fread_number(fp);
        pObjIndex->weight		= UMAX(10, pObjIndex->weight);
        pObjIndex->cost                 = fread_number(fp); 

        /* condition */
        word 				= fread_word(fp);

        if (!is_number(word))
	  switch (*word)
 	  {
	    case ('P') :		pObjIndex->condition = 100; break;
	    case ('G') :		pObjIndex->condition =  90; break;
	    case ('A') :		pObjIndex->condition =  75; break;
	    case ('W') :		pObjIndex->condition =  50; break;
	    case ('D') :		pObjIndex->condition =  25; break;
	    case ('B') :		pObjIndex->condition =  10; break;
	    case ('R') :		pObjIndex->condition =   0; break;
	    default:			pObjIndex->condition = 100; break;
	  }
	else
	  pObjIndex->condition = URANGE(0, atoi(word), 100);
        
        for(i = 0; i < MAX_STATS; i++)
          pObjIndex->require[i] = 0;
        
	if (area_last->version < 3)
	{
	    pObjIndex->uncomf = 0;
	    pObjIndex->unusable = 0;

	    if (IS_SET(pObjIndex->extra_flags, ITEM_ANTI_GOOD))
	    {
		SET_BIT(pObjIndex->unusable, FOR_GOOD);
		REMOVE_BIT(pObjIndex->extra_flags, ITEM_ANTI_GOOD);
	    }
	    if (IS_SET(pObjIndex->extra_flags, ITEM_ANTI_NEUTRAL))
	    {
		SET_BIT(pObjIndex->unusable, FOR_NEUTRAL);
		REMOVE_BIT(pObjIndex->extra_flags, ITEM_ANTI_NEUTRAL);
	    }
	    if (IS_SET(pObjIndex->extra_flags, ITEM_ANTI_EVIL))
	    {
		SET_BIT(pObjIndex->unusable, FOR_EVIL);
		REMOVE_BIT(pObjIndex->extra_flags, ITEM_ANTI_EVIL);
	    }

	    if (IS_SET(pObjIndex->extra_flags, ITEM_ANTI_MAGE))
	    {
		SET_BIT(pObjIndex->unusable, FOR_MAGE);
		REMOVE_BIT(pObjIndex->extra_flags, ITEM_ANTI_MAGE);
	    }
	    if (IS_SET(pObjIndex->extra_flags, ITEM_ANTI_CLERIC))
	    {
		SET_BIT(pObjIndex->unusable, FOR_CLERIC);
		REMOVE_BIT(pObjIndex->extra_flags, ITEM_ANTI_CLERIC);
	    }
	    if (IS_SET(pObjIndex->extra_flags, ITEM_ANTI_THIEF))
	    {
		SET_BIT(pObjIndex->unusable, FOR_THIEF);
		REMOVE_BIT(pObjIndex->extra_flags, ITEM_ANTI_THIEF);
	    }
	    if (IS_SET(pObjIndex->extra_flags, ITEM_ANTI_WARRIOR))
	    {
		SET_BIT(pObjIndex->unusable, FOR_WARRIOR);
		REMOVE_BIT(pObjIndex->extra_flags, ITEM_ANTI_WARRIOR);
	    }
	    if (IS_SET(pObjIndex->extra_flags, ITEM_ANTI_NECROS))
	    {
		SET_BIT(pObjIndex->unusable, FOR_NECROMANCER);
		REMOVE_BIT(pObjIndex->extra_flags, ITEM_ANTI_NECROS);
	    }
	    if (IS_SET(pObjIndex->extra_flags, ITEM_ANTI_PALADIN))
	    {
		SET_BIT(pObjIndex->unusable, FOR_PALADIN);
		REMOVE_BIT(pObjIndex->extra_flags, ITEM_ANTI_PALADIN);
	    }
	    if (IS_SET(pObjIndex->extra_flags, ITEM_ANTI_NAZGUL))
	    {
		SET_BIT(pObjIndex->unusable, FOR_NAZGUL);
		REMOVE_BIT(pObjIndex->extra_flags, ITEM_ANTI_NAZGUL);
	    }
	    if (IS_SET(pObjIndex->extra_flags, ITEM_ANTI_DRUID))
	    {
		SET_BIT(pObjIndex->unusable, FOR_DRUID);
		REMOVE_BIT(pObjIndex->extra_flags, ITEM_ANTI_DRUID);
	    }
	    if (IS_SET(pObjIndex->extra_flags, ITEM_ANTI_RANGER))
	    {
		SET_BIT(pObjIndex->unusable, FOR_RANGER);
		REMOVE_BIT(pObjIndex->extra_flags, ITEM_ANTI_RANGER);
	    }
	    if (IS_SET(pObjIndex->extra_flags, ITEM_ANTI_VAMPIRE))
	    {
		SET_BIT(pObjIndex->unusable, FOR_VAMPIRE);
		REMOVE_BIT(pObjIndex->extra_flags, ITEM_ANTI_VAMPIRE);
	    }
/*	    if (IS_SET(pObjIndex->extra_flags, ITEM_ANTI_ALCHEMIST))
	    {
		SET_BIT(pObjIndex->unusable, FOR_ALCHEMIST);
		REMOVE_BIT(pObjIndex->extra_flags, ITEM_ANTI_ALCHEMIST);
	    }
	    if (IS_SET(pObjIndex->extra_flags, ITEM_ANTI_LYCANTHROPE))
	    {
		SET_BIT(pObjIndex->unusable, FOR_LYCANTHROPE);
		REMOVE_BIT(pObjIndex->extra_flags, ITEM_ANTI_LYCANTHROPE);
	    }
*/
	    if (IS_SET(pObjIndex->extra_flags, ITEM_FOR_MAN))
	    {
		SET_BIT(pObjIndex->uncomf, FOR_WOMAN);
		REMOVE_BIT(pObjIndex->extra_flags, ITEM_FOR_MAN);
	    }
	    if (IS_SET(pObjIndex->extra_flags, ITEM_FOR_WOMAN))
	    {
		SET_BIT(pObjIndex->uncomf, FOR_MAN);
		REMOVE_BIT(pObjIndex->extra_flags, ITEM_FOR_WOMAN);
	    }
	}
	else
	{
	    pObjIndex->uncomf   = fread_flag(fp);
	    pObjIndex->unusable = fread_flag(fp);
	}
	
        for (; ;)
        {
            char letter;
 
            letter = fread_letter(fp);
 
            if (letter == 'A')
            {
                AFFECT_DATA *paf;
 
                paf                     = alloc_perm(sizeof(*paf));
		paf->where		= TO_OBJECT;
                paf->type               = -1;
                paf->level              = pObjIndex->level;
                paf->duration           = -1;
                paf->location           = fread_number(fp);
                paf->modifier           = fread_number(fp);
                paf->bitvector          = 0;
                paf->next               = pObjIndex->affected;
		paf->prev		= NULL;
		if (pObjIndex->affected)
		    pObjIndex->affected->prev = paf;
                pObjIndex->affected     = paf;
                top_affect++;
            }
            else if (letter == 'S')
            {
                AFFECT_DATA *paf;
                char *s;
                int sn;
 
                paf                     = alloc_perm(sizeof(*paf));
		paf->where		= TO_OBJECT;

		s = fread_word(fp);
		if (is_number(s))
		    sn = atoi(s);
		else
		    sn = skill_lookup(s);

                paf->type               = sn;
                paf->level              = pObjIndex->level;
                paf->duration           = -1;
                paf->location           = APPLY_SKILL;
                paf->modifier           = fread_number(fp);
                paf->bitvector          = 0;
                paf->next               = pObjIndex->affected;
		paf->prev		= NULL;
		if (pObjIndex->affected)
		    pObjIndex->affected->prev = paf;
                pObjIndex->affected     = paf;
                top_affect++;
            } 
	    else if (letter == 'F')
            {
                AFFECT_DATA *paf;
                int64_t flag,loc,mod,i,where = 0;
 
                
		letter 			= fread_letter(fp);
		switch (letter)
	 	{
		case 'A':
                    where          	= TO_AFFECTS;
		    break;
		case 'I':
		    if (area_last->version < 2)
			where		= TO_IMMUNE;
		    else
			bugf("Old style apply flag 'I' in obj %d.",
			     pObjIndex->vnum);
		    break;
		case 'R':
		    where		= TO_RESIST;
		    break;
		case 'V':
		    if (area_last->version < 2)
			where		= TO_VULN;
		    else
			bugf("Old style apply flag 'V' in obj %d.",
			     pObjIndex->vnum);
		    break;
		default:
            	    bugf("Load_objects: Bad where on flag set.");
            	   exit(1);
		}

                
		loc = fread_number(fp);
		mod = fread_number(fp);
                flag = fread_flag(fp);

		if (area_last->version < 2
		    && (where == TO_IMMUNE
			|| where == TO_RESIST
			|| where == TO_VULN))
		{
		    switch (where)
		    {
		    case TO_IMMUNE:
			mod = 100;
			break;
		    case TO_RESIST:
			mod = 33;
			break;
		    case TO_VULN:
			mod = -50;
			break;
		    }
		    where = TO_RESIST;
		}

		if (area_last->version < 2 || where != TO_RESIST)
		{
		    for (i = 1; i <= flag; i *= 2) 
			if (IS_SET(flag, i)) 
			{
			    paf                     = alloc_perm(sizeof(*paf));
			    paf->where		    = where;
			    paf->type               = -1;
			    paf->level              = pObjIndex->level;
			    paf->duration           = -1;
			    paf->location           = loc;
			    paf->modifier           = mod;

			    if (where != TO_RESIST)
				paf->bitvector      = i;
			    else
				paf->bitvector      = set_resist_num(i); 
			    
			    paf->next               = pObjIndex->affected;
			    paf->prev               = NULL;
			    if (pObjIndex->affected)
				pObjIndex->affected->prev = paf;
			    pObjIndex->affected     = paf;
			    top_affect++;
		      	}
		}
		else
		{
		    if (flag >= DAM_MAX)
		    {
			bugf("Incorrect resist on obj %s [%d].",
			     pObjIndex->name, pObjIndex->vnum);
			flag = DAM_NONE;
		    }

		    paf            = alloc_perm(sizeof(*paf));
		    paf->where     = where;
		    paf->type      = -1;
		    paf->level     = pObjIndex->level;
		    paf->duration  = -1;
		    paf->location  = APPLY_NONE;
		    paf->modifier  = mod;
		    paf->bitvector = flag;

		    paf->next      = pObjIndex->affected;
		    paf->prev      = NULL;
		    if (pObjIndex->affected)
			pObjIndex->affected->prev = paf;
		    pObjIndex->affected = paf;
		    top_affect++;
		}
            }
 
            else if (letter == 'E')
            {
                EXTRA_DESCR_DATA *ed;
 
                ed                      = alloc_perm(sizeof(*ed));
                ed->keyword             = fread_string(fp);
                ed->description         = fread_string(fp);
                ed->next                = pObjIndex->extra_descr;
                pObjIndex->extra_descr  = ed;
                top_ed++;
            }
	    else if (letter == 'O')
	    {
		PROG_LIST *pOprog;
		char *word;
		int trigger = 0;

		pOprog			= alloc_perm(sizeof(*pOprog));
		SET_BIT(pOprog->trig_flags, get_prog_when(fp));
		word			= fread_word(fp);
		if (!(trigger = flag_lookup(word, oprog_flags)))
		{
		    bugf("OBJprogs: invalid trigger.");
		    exit(1);
		}
		SET_BIT(pOprog->trig_flags, trigger);
		pOprog->trig_type	= trigger;
		pOprog->vnum	 	= fread_number(fp);
		pOprog->trig_phrase	= fread_string(fp);
		pOprog->next		= pObjIndex->progs;
		pObjIndex->progs	= pOprog;
	    }
	    else if (letter == 'R')
	    {
	        char *word;
	        int  stat;
	        
	        word = fread_word(fp);
	        i = fread_number(fp);
	        
	        if ((stat = flag_lookup(word, attr_flags)) != NO_FLAG)
	          pObjIndex->require[stat] = i;
            }
	    else if (letter == '*')
	    {
		free_string(pObjIndex->comments);
	        pObjIndex->comments = fread_string(fp);
	    }
	    else if (letter == 'Z')
	    {
		free_string(pObjIndex->last_edited);
	        pObjIndex->last_edited = fread_string(fp);
	    }
	    else if (letter == 'T')
	    {
	        pObjIndex->timer = fread_number(fp);
	        SET_BIT(pObjIndex->extra_flags, ITEM_HAD_TIMER);
	    }
            else
            {
                ungetc(letter, fp);
                break;
            }
        }

        if (pObjIndex->item_type == ITEM_ARTIFACT && 
            (pObjIndex->vnum < ARTIFACT_MIN_VNUM || pObjIndex->vnum > ARTIFACT_MAX_VNUM))
        {
            bugf("Artifact not in artifact.are zone, vnum %d", pObjIndex->vnum);
        }
        else
          top_obj_index++;

        iHash                   = vnum % MAX_KEY_HASH;
        pObjIndex->next         = obj_index_hash[iHash];
        obj_index_hash[iHash]   = pObjIndex;
        top_vnum_obj = top_vnum_obj < vnum ? vnum : top_vnum_obj;   /* OLC */
        top_vnum = top_vnum < vnum ? vnum : top_vnum;
        assign_area_vnum(vnum);                                   /* OLC */
    }
 
    return;
}

/** OLC Inserts here **/

/*****************************************************************************
 Name:	        convert_objects
 Purpose:	Converts all old format objects to new format
 Called by:	boot_db (db.c).
 Note:          Loops over all resets to find the level of the mob
                loaded before the object to determine the level of
                the object.
		It might be better to update the levels in load_resets().
		This function is not pretty.. Sorry about that :)
 Author:        Hugin
 ****************************************************************************/
void convert_objects(void)
{
    int vnum;
    AREA_DATA  *pArea;
    RESET_DATA *pReset;
    MOB_INDEX_DATA *pMob = NULL;
    OBJ_INDEX_DATA *pObj;
    ROOM_INDEX_DATA *pRoom;

    if (newobjs == top_obj_index)
	return; /* all objects in new format */

    for (pArea = area_first; pArea; pArea = pArea->next)
    {
        for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++)
	{
	    if (!(pRoom = get_room_index(vnum)))
		continue;

	    for (pReset = pRoom->reset_first; pReset; pReset = pReset->next)
	    {
		switch (pReset->command)
		{
		case 'M':
		    if (!(pMob = get_mob_index(pReset->arg1)))
			bugf("Convert_objects: 'M': bad vnum %d.", pReset->arg1);
		    break;

		case 'O':
		    if (!(pObj = get_obj_index(pReset->arg1)))
		    {
			bugf("Convert_objects: 'O': bad vnum %d.", pReset->arg1);
			break;
		    }

		    if (pObj->new_format)
			continue;

		    if (!pMob)
		    {
			bugf("Convert_objects: 'O': No mob reset yet.");
			break;
		    }

		    pObj->level = pObj->level < 1 ? pMob->level - 2
					: UMIN(pObj->level, pMob->level - 2);
		    break;

		case 'P':
		    {
			OBJ_INDEX_DATA *pObj, *pObjTo;

			if (!(pObj = get_obj_index(pReset->arg1)))
			{
			    bugf("Convert_objects: 'P': bad vnum %d.",
				pReset->arg1);
			    break;
			}

			if (pObj->new_format)
			    continue;

			if (!(pObjTo = get_obj_index(pReset->arg3)))
			{
			    bugf("Convert_objects: 'P': bad vnum %d.",
				pReset->arg3);
			    break;
			}

			pObj->level = pObj->level < 1 ? pObjTo->level
					    : UMIN(pObj->level, pObjTo->level);
		    }
		    break;

		case 'G':
		case 'E':
		    if (!(pObj = get_obj_index(pReset->arg1)))
		    {
			bugf("Convert_objects: 'E' or 'G': bad vnum %d.",
			    pReset->arg1);
			break;
		    }

		    if (!pMob)
		    {
			bugf("Convert_objects: 'E' 'G': null mob for vnum %d.",
			    pReset->arg1);
			break;
		    }

		    if (pObj->new_format)
			continue;

		    if (pMob->pShop)
		    {
			switch (pObj->item_type)
			{
			default:
			    pObj->level = UMAX(0, pObj->level);
			    break;
			case ITEM_PILL:
			case ITEM_POTION:
			    pObj->level = UMAX(5, pObj->level);
			    break;
			case ITEM_SCROLL:
			case ITEM_ARMOR:
			case ITEM_WEAPON:
			    pObj->level = UMAX(10, pObj->level);
			    break;
			case ITEM_WAND:
			case ITEM_TREASURE:
			    pObj->level = UMAX(15, pObj->level);
			    break;
			case ITEM_STAFF:
			    pObj->level = UMAX(20, pObj->level);
			    break;
			}
		    }
		    else
			pObj->level = pObj->level < 1 ? pMob->level
					    : UMIN(pObj->level, pMob->level);
		    break;
		} /* switch (pReset->command) */
	    }
	}
    }

    /* do the conversion: */

    for (pArea = area_first; pArea ; pArea = pArea->next)
	for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++)
	    if ((pObj = get_obj_index(vnum)))
 		if (!pObj->new_format)
		    convert_object(pObj);


    return;
}



/*****************************************************************************
 Name:		convert_object
 Purpose:	Converts an old_format obj to new_format
 Called by:	convert_objects (db2.c).
 Note:          Dug out of create_obj (db.c)
 Author:        Hugin
 ****************************************************************************/
void convert_object(OBJ_INDEX_DATA *pObjIndex)
{
    int level;
    int number, type;  /* for dice-conversion */

    if (!pObjIndex || pObjIndex->new_format) return;

    level = pObjIndex->level;

    pObjIndex->level    = UMAX(0, pObjIndex->level); /* just to be sure */
    pObjIndex->cost     = 10*level;
    switch (pObjIndex->item_type)
    {
    default:
	bugf("Obj_convert: vnum %d bad type.", pObjIndex->item_type);
    	break;

    case ITEM_LIGHT:
    case ITEM_TREASURE:
    case ITEM_FURNITURE:
    case ITEM_TRASH:
    case ITEM_CONTAINER:
    case ITEM_CHEST:
    case ITEM_DRINK_CON:
    case ITEM_KEY:
    case ITEM_FOOD:
    case ITEM_BOAT:
    case ITEM_CORPSE_NPC:
    case ITEM_CORPSE_PC:
    case ITEM_FOUNTAIN:
    case ITEM_MAP:
    case ITEM_CLOTHING:
    case ITEM_SCROLL:
	break;

    case ITEM_WAND:
    case ITEM_STAFF:
	pObjIndex->value[2] = pObjIndex->value[1];
	break;

    case ITEM_WEAPON:
    	/*
	 * The conversion below is based on the values generated
	 * in one_hit() (fight.c).  Since I don't want a lvl 50 
	 * weapon to do 15d3 damage, the min value will be below
	 * the one in one_hit, and to make up for it, I've made 
	 * the max value higher.
	 * (I don't want 15d2 because this will hardly ever roll
	 * 15 or 30, it will only roll damage close to 23.
	 * I can't do 4d8+11, because one_hit there is no dice-
	 * bounus value to set...)
	 *
	 * The conversion below gives:

	     level:   dice      min      max      mean
	       1:     1d8      1(2)    8(7)     5(5)
	       2:     2d5      2(3)   10(8)     6(6)
	       3:     2d5      2(3)   10(8)     6(6)
	       5:     2d6      2(3)   12(10)     7(7)
	      10:     4d5      4(5)   20(14)    12(10)
	      20:     5d5      5(7)   25(21)    15(14)
	      30:     5d7      5(10)   35(29)    20(20)
	      50:     5d11     5(15)   55(44)    30(30)

	 */

    	number = UMIN(level/4 + 1, 5);
	type   = (level + 7)/number;

	pObjIndex->value[1] = number;
	pObjIndex->value[2] = type;
	break;

    case ITEM_ARMOR:
	pObjIndex->value[0] = level / 5 + 3;
	pObjIndex->value[1] = pObjIndex->value[0];
	pObjIndex->value[2] = pObjIndex->value[0];
	break;

    case ITEM_POTION:
    case ITEM_PILL:
	break;

    case ITEM_MONEY:
	pObjIndex->value[0] = pObjIndex->cost;
	break;
    }

    pObjIndex->new_format = TRUE;
    ++newobjs;

    return;
}




/*****************************************************************************
 Name:		convert_mobile
 Purpose:	Converts an old_format mob into new_format
 Called by:	load_old_mob (db.c).
 Note:          Dug out of create_mobile (db.c)
 Author:        Hugin
 ****************************************************************************/
void convert_mobile(MOB_INDEX_DATA *pMobIndex)
{
    int i;
    int type, number, bonus;
    int level;

    if (!pMobIndex || pMobIndex->new_format) return;

    level = pMobIndex->level;

    pMobIndex->act              |= ACT_WARRIOR;

    /*
     * Calculate hit dice.  Gives close to the hitpoints
     * of old format mobs created with create_mobile()  (db.c)
     * A high number of dice makes for less variance in mobiles
     * hitpoints.
     * (might be a good idea to reduce the max number of dice)
     *
     * The conversion below gives:

       level:     dice         min         max        diff       mean
         1:       1d2+6       7( 7)     8(  8)     1(  1)     8(  8)
	 2:       1d3+15     16(15)    18( 18)     2(  3)    17( 17)
	 3:       1d6+24     25(24)    30( 30)     5(  6)    27( 27)
	 5:      1d17+42     43(42)    59( 59)    16( 17)    51( 51)
	10:      3d22+96     99(95)   162(162)    63( 67)   131(  )
	15:     5d30+161    166(159)   311(311)   145(150)   239(  )
	30:    10d61+416    426(419)  1026(1026)   600(607)   726(  )
	50:    10d169+920   930(923)  2610(2610)  1680(1688)  1770(  )

	The values in parenthesis give the values generated in create_mobile.
        Diff = max - min.  Mean is the arithmetic mean.
	(hmm.. must be some roundoff error in my calculations.. smurfette got
	 1d6+23 hp at level 3 ? -- anyway.. the values above should be
	 approximately right..)
     */
    type   = level*level*27/40;
    number = UMIN(type/40 + 1, 10); /* how do they get 11 ??? */
    type   = UMAX(2, type/number);
    bonus  = UMAX(0, level*(8 + level)*.9 - number*type);

    pMobIndex->hit[DICE_NUMBER]    = number;
    pMobIndex->hit[DICE_TYPE]      = type;
    pMobIndex->hit[DICE_BONUS]     = bonus;

    pMobIndex->mana[DICE_NUMBER]   = level;
    pMobIndex->mana[DICE_TYPE]     = 10;
    pMobIndex->mana[DICE_BONUS]    = 100;

    /*
     * Calculate dam dice.  Gives close to the damage
     * of old format mobs in damage()  (fight.c)
     */
    type   = level*7/4;
    number = UMIN(type/8 + 1, 5);
    type   = UMAX(2, type/number);
    bonus  = UMAX(0, level*9/4 - number*type);

    pMobIndex->damage[DICE_NUMBER] = number;
    pMobIndex->damage[DICE_TYPE]   = type;
    pMobIndex->damage[DICE_BONUS]  = bonus;

    switch (number_range(1, 3))
    {
        case (1): pMobIndex->dam_type =  3;       break;  /* slash  */
        case (2): pMobIndex->dam_type =  7;       break;  /* pound  */
        case (3): pMobIndex->dam_type = 11;       break;  /* pierce */
    }

    for (i = 0; i < 3; i++)
        pMobIndex->ac[i]         = interpolate(level, 100, -100);
    pMobIndex->ac[3]             = interpolate(level, 100, 0);    /* exotic */

    pMobIndex->wealth            /= 100;
    pMobIndex->size              = SIZE_MEDIUM;
    pMobIndex->material          = 0;

    pMobIndex->new_format        = TRUE;
    ++newmobs;

    return;
}

/* Function Name: FILE *file_open (const char *mode, char *fmt,...)
 *
 * this function closes fpReserve (if open), and opens a stream to the
 * file specified, with the specified modes.
 */
FILE *file_open (char *file, const char *mode)
{
    FILE *fp = NULL;

    /* fpReserve open? */
    if (fpReserve != NULL)
	{
	    /* Assuming that fpReserve *will* get re-opened sometime */
	    fclose(fpReserve);
	    fpReserve = NULL;
	}

    /* Open the new stream */
    fp = fopen(file, mode);

    return fp;
}

/* Function Name: bool file_close (FILE *fp)
 *
 * this function closes the stream specified, and attempts to re-open
 * fpReserve, if it's not already open.
 */
bool file_close (FILE * fp)
{
    if (fp != NULL)
	fclose(fp);

    /* fpReserve already open? */
    if (fpReserve == NULL)
	/* Nope, let's open */
	fpReserve = fopen(NULL_FILE, "r");

    return TRUE;
}
/* charset=cp1251 */
