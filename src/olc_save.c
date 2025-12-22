/**************************************************************************
 *  File: olc_save.c                                                       *
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
/* OLC_SAVE.C
 * This takes care of saving all the .are information.
 * Notes:
 * -If a good syntax checker is used for setting vnum ranges of areas
 *  then it would become possible to just cycle through vnums instead
 *  of using the iHash stuff and checking that the room or reset or
 *  mob etc is part of that area.
 */

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "olc.h"
#include "config.h"



/* dbdump.c */
void dbdump(AREA_DATA *area);

#define DIF(a,b) (~((~a)|(b)))

/*
 *  Verbose writes reset data in plain english into the comments
 *  section of the resets.  It makes areas considerably larger but
 *  may aid in debugging.
 */
#define VERBOSE

char *weapon_name_eng(int weapon_type)
{
    int type;
 
    for (type = 0; weapon_table[type].name != NULL; type++)
        if (weapon_type == weapon_table[type].type)
            return weapon_table[type].name;
    return "exotic";
}


/*****************************************************************************
 Name:		fix_string
 Purpose:	Returns a string without \r and ~.
 ****************************************************************************/
char *fix_string(const char *str)
{
    static char strfix[MAX_STRING_LENGTH * 2];
    int i;
    int o;

    if (str == NULL)
        return '\0';

    for (o = i = 0; str[i+o] != '\0'; i++)
    {
        if (str[i+o] == '\r' || str[i+o] == '~')
            o++;
        if ((strfix[i] = str[i+o]) == '\0')
            break;
    }
    strfix[i] = '\0';
    return strfix;
}



/*****************************************************************************
 Name:		save_area_list
 Purpose:	Saves the listing of files to be loaded at startup.
 Called by:	do_asave(olc_save.c).
 ****************************************************************************/
/* void save_area_list()
{
    FILE *fp;
    AREA_DATA *pArea;
    extern HELP_AREA * had_list;
    HELP_AREA * ha;

    if ((fp = fopen("area.lst", "w")) == NULL)
    {
	bugf("Save_area_list: fopen");
	perror("area.lst");
    }
    else
    { */
	/*
	 * Add any help files that need to be loaded at
	 * startup to this section.
	 */
/*	fprintf(fp, "social.hlp\n");    

	for (ha = had_list; ha; ha = ha->next)
		if (ha->area == NULL)
			fprintf(fp, "%s\n", ha->filename);

	for(pArea = area_first; pArea; pArea = pArea->next)
	{
	    fprintf(fp, "%s\n", pArea->file_name);
	}

	fprintf(fp, "$\n");
	fclose(fp);
    }
	
    return;
} */

#define NBITS 52
#if 0
#define NBUF 5
char *fwrite_flag(int64_t flags)
{
    static int cnt;
    static char buf[NBUF][NBITS+1];
    int count, pos = 0;

    cnt = (cnt + 1) % NBUF;

    for (count = 0; count < NBITS;  count++)
        if (IS_SET(flags, (int64_t) 1 << count))
        {
            if (count < 26)
	        buf[cnt][pos] = 'A' + count;
	    else
                buf[cnt][pos] = 'a' + (count - 26);
            pos++;
        }

    if (pos == 0) 
        buf[cnt][pos++] = '0';

    buf[cnt][pos] = '\0';
    return buf[cnt];
}
#endif /* 0 */

char *_fwrite_flag(int64_t flags, char *file, int line)
{
    char offset;
    static char buf[100];
    int i;

    if (flags == 0)
    {
        sprintf(buf, "0");
        return buf;
    }

    /* 64 -- number of bits in a int64_t */
    for (offset = 0, i = 0; offset < NBITS; offset++)
        if (IS_SET(flags, ((int64_t) 1 << offset)))
        {
            if (offset <= 'Z' - 'A')
                buf[i] = 'A' + offset;
            else
                buf[i] = 'a' + offset - ('Z' - 'A' + 1);
	    i++;
        }

    buf[i] = '\0';

    if (strlen(buf) > 64)
	bugf("File:%s  Line:%d   Flags:%s   (%llu)", file, line, buf, flags);

    return buf;
}


void save_mobprogs(FILE *fp, AREA_DATA *pArea)
{
	PROG_CODE *pMprog;
        int i;

        fprintf(fp, "#MOBPROGS\n");

	for(i = pArea->min_vnum; i <= pArea->max_vnum; i++)
        {
          if ((pMprog = get_prog_index(i, PRG_MPROG)) != NULL)
		{
		          fprintf(fp, "#%d\n", i);
		          fprintf(fp, "%s~\n", fix_string(pMprog->code));
		}
        }

        fprintf(fp,"#0\n\n");
        return;
}
void save_objprogs(FILE *fp, AREA_DATA *pArea)
{
	PROG_CODE *pOprog;
        int i;

        fprintf(fp, "#OBJPROGS\n");

	for(i = pArea->min_vnum; i <= pArea->max_vnum; i++)
        {
          if ((pOprog = get_prog_index(i, PRG_OPROG)) != NULL)
		{
		          fprintf(fp, "#%d\n", i);
		          fprintf(fp, "%s~\n", fix_string(pOprog->code));
		}
        }

        fprintf(fp,"#0\n\n");
        return;
}

void save_roomprogs(FILE *fp, AREA_DATA *pArea)
{
	PROG_CODE *pRprog;
        int i;

        fprintf(fp, "#ROOMPROGS\n");

	for(i = pArea->min_vnum; i <= pArea->max_vnum; i++)
        {
          if ((pRprog = get_prog_index(i,PRG_RPROG)) != NULL)
		{
		          fprintf(fp, "#%d\n", i);
		          fprintf(fp, "%s~\n", fix_string(pRprog->code));
		}
        }

        fprintf(fp,"#0\n\n");
        return;
}

void save_chat_data(FILE *fp, AREA_DATA *pArea)
{
        CHAT_KEYS *k;
         
        fprintf(fp, "#CHAT\n");

	for(k = chatkeys_list; k != NULL; k = k->next)
        {
          if (k->area == pArea)
	  {
	     CHAT_KEY *ck;

             fprintf(fp, "#%d\n", k->vnum);

	     for(ck = k->list; ck != NULL; ck = ck->next)	
	     {
		REPLY *r;
                
                fprintf(fp, "=%s~\n", ck->regexp);

                for(r = ck->replys; r != NULL; r = r->next)
		  fprintf(fp, "%d%s~%s~%s~\n", 
		    r->weight, 
		    r->race > 0 ? race_table[r->race].name : "", 
		    r->sex >= 0 ? sex_table[r->sex].name : "", 
		    r->sent);
		
		fprintf(fp, "\n");
	     }	
	  }
        }

        fprintf(fp,"#0\n\n");
        return;
}


/*****************************************************************************
 Name:		save_mobile
 Purpose:	Save one mobile to file, new format -- Hugin
 Called by:	save_mobiles (below).
 ****************************************************************************/
void save_mobile(FILE *fp, MOB_INDEX_DATA *pMobIndex)
{
    int16_t race = pMobIndex->race;
    PROG_LIST *pMprog;
    int64_t temp;
    int i, count = 0;
    char buf[MSL];

    fprintf(fp, "#%d\n",	pMobIndex->vnum);
    fprintf(fp, "%s~\n",	pMobIndex->player_name);
    fprintf(fp, "%s~\n",	pMobIndex->short_descr);
    fprintf(fp, "%s~\n",	fix_string(pMobIndex->long_descr));
    fprintf(fp, "%s~\n",	fix_string(pMobIndex->description));
    fprintf(fp, "%s~\n",	race_table[race].name);

    fprintf(fp, "%s ",		fwrite_flag(pMobIndex->act));
    fprintf(fp, "%s ",		fwrite_flag(pMobIndex->affected_by));
    fprintf(fp, "%d %d %d\n",	pMobIndex->saving_throw, pMobIndex->alignment , pMobIndex->group);
    fprintf(fp, "%d ",		pMobIndex->level);
    fprintf(fp, "%d ",		pMobIndex->hitroll);
    fprintf(fp, "%dd%d+%d ",	pMobIndex->hit[DICE_NUMBER], 
				pMobIndex->hit[DICE_TYPE], 
				pMobIndex->hit[DICE_BONUS]);
    fprintf(fp, "%dd%d+%d ",	pMobIndex->mana[DICE_NUMBER], 
				pMobIndex->mana[DICE_TYPE], 
				pMobIndex->mana[DICE_BONUS]);
    fprintf(fp, "%dd%d+%d ",	pMobIndex->damage[DICE_NUMBER], 
				pMobIndex->damage[DICE_TYPE], 
				pMobIndex->damage[DICE_BONUS]);
    fprintf(fp, "%s\n",	attack_table[pMobIndex->dam_type].name);
    fprintf(fp, "%d %d %d %d\n",
				pMobIndex->ac[AC_PIERCE] / 10, 
				pMobIndex->ac[AC_BASH]   / 10, 
				pMobIndex->ac[AC_SLASH]  / 10, 
				pMobIndex->ac[AC_EXOTIC] / 10);
    fprintf(fp, "%s ",		fwrite_flag(pMobIndex->off_flags));
    fprintf(fp, "%s %s %s %ld\n",
				position_table[pMobIndex->start_pos].short_name,
				position_table[pMobIndex->default_pos].short_name,
				sex_table[pMobIndex->sex].name,
				pMobIndex->wealth); 
    fprintf(fp, "%s ",		fwrite_flag(pMobIndex->form));
    fprintf(fp, "%s ",		fwrite_flag(pMobIndex->parts));

    fprintf(fp, "%s ",		size_table[pMobIndex->size].name);
    fprintf(fp, "%s\n",	IS_NULLSTR(pMobIndex->material) ? pMobIndex->material : "unknown");

    if ((temp = DIF(race_table[race].act,pMobIndex->act)))
     	fprintf(fp, "F act %s\n", fwrite_flag(temp));

    if ((temp = DIF(race_table[race].aff,pMobIndex->affected_by)))
     	fprintf(fp, "F aff %s\n", fwrite_flag(temp));

    if ((temp = DIF(race_table[race].off,pMobIndex->off_flags)))
     	fprintf(fp, "F off %s\n", fwrite_flag(temp));

    if ((temp = DIF(race_table[race].form,pMobIndex->form)))
     	fprintf(fp, "F for %s\n", fwrite_flag(temp));

    if ((temp = DIF(race_table[race].parts,pMobIndex->parts)))
    	fprintf(fp, "F par %s\n", fwrite_flag(temp));

    for (i = 0; i < DAM_MAX; i++)
	if (pMobIndex->resists[i] != 0)
	{
	    fprintf(fp, "P %s %d\n",
	    	    flag_string(dam_flags, i, FALSE),
		    pMobIndex->resists[i]);
	}

    for (pMprog = pMobIndex->progs; pMprog; pMprog = pMprog->next)
    {
        fprintf(fp, "M%s %s %d %s~\n",
        IS_SET(pMprog->trig_flags, EXEC_BEFORE) ? "B" : IS_SET(pMprog->trig_flags, EXEC_AFTER) ? "A" : "",
        prog_type_to_name(pMprog->trig_type), pMprog->vnum,
                pMprog->trig_phrase);
    }

    buf[0] = '\0';

    for(i = 0; i < SECT_MAX; i++)
      	if (pMobIndex->areal[i])
	{
	    strcat(buf, flag_string(sector_flags, i, FALSE));
	    strcat(buf, " ");
	    count++;
	}

    if (count < SECT_MAX)
	fprintf(fp, "A %s~\n", buf);  

    if (!IS_NULLSTR(pMobIndex->msg_arrive))
	fprintf(fp, "E %s~\n", pMobIndex->msg_arrive);

    if (!IS_NULLSTR(pMobIndex->msg_gone))
	fprintf(fp, "G %s~\n", pMobIndex->msg_gone);

    if (pMobIndex->chat_vnum[0])
    {
	fprintf(fp, "C");

	for(i = 0; pMobIndex->chat_vnum[i] && i < MAX_CHATBOOKS; i++)
	  fprintf(fp, " %d", pMobIndex->chat_vnum[i]);

        fprintf(fp, "\n");
    }	
     
    if (pMobIndex->clan != 0)
      fprintf(fp, "L %s~\n", clan_table[pMobIndex->clan].name);

    if (pMobIndex->rating != 0)
      fprintf(fp, "R %d\n", pMobIndex->rating);

    if (!IS_NULLSTR(pMobIndex->last_edited))     
      fprintf(fp, "Z %s~\n", pMobIndex->last_edited);
    
    if (!IS_NULLSTR(pMobIndex->comments))
      fprintf(fp, "* %s~\n", pMobIndex->comments);

    return;
}


/*****************************************************************************
 Name:		save_mobiles
 Purpose:	Save #MOBILES secion of an area file.
 Called by:	save_area(olc_save.c).
 Notes:         Changed for ROM OLC.
 ****************************************************************************/
void save_mobiles(FILE *fp, AREA_DATA *pArea)
{
    int i;
    MOB_INDEX_DATA *pMob;

    fprintf(fp, "#MOBILES\n");

    for(i = pArea->min_vnum; i <= pArea->max_vnum; i++)
    {
	if ((pMob = get_mob_index(i)))
	    save_mobile(fp, pMob);
    }

    fprintf(fp, "#0\n\n\n\n");
    return;
}





/*****************************************************************************
 Name:		save_object
 Purpose:	Save one object to file.
                new ROM format saving -- Hugin
 Called by:	save_objects (below).
 ****************************************************************************/
void save_object(FILE *fp, OBJ_INDEX_DATA *pObjIndex)
{
    AFFECT_DATA *pAf;
    EXTRA_DESCR_DATA *pEd;
    PROG_LIST *pOprog;
    int cnt;


    fprintf(fp, "#%d\n",    pObjIndex->vnum);
    fprintf(fp, "%s~\n",    pObjIndex->name);
    fprintf(fp, "%s~\n",    pObjIndex->short_descr);
    fprintf(fp, "%s~\n",    fix_string(pObjIndex->description));
    fprintf(fp, "%s~\n",    pObjIndex->material);
    fprintf(fp, "%s ",      item_name(pObjIndex->item_type,1));
    fprintf(fp, "%s ",      fwrite_flag(pObjIndex->extra_flags));
    fprintf(fp, "%s\n",     fwrite_flag(pObjIndex->wear_flags));

    /*
     *  Using fwrite_flag to write most values gives a strange
     *  looking area file, consider making a case for each
     *  item type later.
     */

    switch (pObjIndex->item_type)
    {
        default:
	    fprintf(fp, "%s ",  fwrite_flag(pObjIndex->value[0]));
	    fprintf(fp, "%s ",  fwrite_flag(pObjIndex->value[1]));
	    fprintf(fp, "%s ",  fwrite_flag(pObjIndex->value[2]));
	    fprintf(fp, "%s ",  fwrite_flag(pObjIndex->value[3]));
	    fprintf(fp, "%s\n", fwrite_flag(pObjIndex->value[4]));
	    break;

        case ITEM_LIGHT:
	    fprintf(fp, "0 0 %d 0 0\n",
		     pObjIndex->value[2] < 1 ? 999
		     : pObjIndex->value[2]);
	    break;

        case ITEM_MONEY:
            fprintf(fp, "%d %d 0 0 0\n",
                     pObjIndex->value[0],
                     pObjIndex->value[1]);
            break;

	case ITEM_ROD:
	    fprintf(fp, "%d '%s' %d 0 0\n\r",
	    		pObjIndex->value[0],
	                pObjIndex->value[1] != -1
				? skill_table[pObjIndex->value[1]].name
				: "",
	    		pObjIndex->value[2]);
	    break;

	case ITEM_TRAP:
	    fprintf(fp, "%d %d 0 '%s' %d\n\r",
	    		pObjIndex->value[0],
	                pObjIndex->value[1],
			attack_table[URANGE(0, pObjIndex->value[3], MAX_DAMAGE_MESSAGE)].name,
			pObjIndex->value[4]);
	    break;

        case ITEM_DRINK_CON:
            fprintf(fp, "%d %d '%s' %d 0\n",
                     pObjIndex->value[0],
                     pObjIndex->value[1],
                     liq_table[pObjIndex->value[2]].liq_name,
		     pObjIndex->value[3]);
            break;
                    
	case ITEM_FOUNTAIN:
	    fprintf(fp, "%d %d '%s' 0 0\n",
	             pObjIndex->value[0],
	             pObjIndex->value[1],
	             liq_table[pObjIndex->value[2]].liq_name);
	    break;

        case ITEM_CONTAINER:
            fprintf(fp, "%d %s %d %d %d\n",
                     pObjIndex->value[0],
                     fwrite_flag(pObjIndex->value[1]),
                     pObjIndex->value[2],
                     pObjIndex->value[3],
                     pObjIndex->value[4]);
            break;

        case ITEM_FOOD:
            fprintf(fp, "%d %d 0 %s 0\n",
                     pObjIndex->value[0],
                     pObjIndex->value[1],
                     fwrite_flag(pObjIndex->value[3]));
            break;
            
        case ITEM_PORTAL:
            fprintf(fp, "%d %s ",
                     pObjIndex->value[0],
                     fwrite_flag(pObjIndex->value[1]));
	    fprintf(fp, "%s %d %d\n",
                     fwrite_flag(pObjIndex->value[2]),
                     pObjIndex->value[3], pObjIndex->value[4]);
            break;
            
        case ITEM_FURNITURE:
            fprintf(fp, "%d %d %s %d %d\n",
                     pObjIndex->value[0],
                     pObjIndex->value[1],
                     fwrite_flag(pObjIndex->value[2]),
                     pObjIndex->value[3],
                     pObjIndex->value[4]);
            break;

        case ITEM_WEAPON:
            fprintf(fp, "%s %d %d '%s' %s\n",
                     weapon_name_eng(pObjIndex->value[0]),
                     pObjIndex->value[1],
                     pObjIndex->value[2],
                     attack_table[pObjIndex->value[3]].name,
                     fwrite_flag(pObjIndex->value[4]));
            break;

        case ITEM_ARMOR:
        case ITEM_MAP:
            fprintf(fp, "%d %d %d %d %d\n",
                     pObjIndex->value[0],
                     pObjIndex->value[1],
                     pObjIndex->value[2],
                     pObjIndex->value[3],
                     pObjIndex->value[4]);
            break;            

        case ITEM_PILL:
        case ITEM_POTION:
        case ITEM_SCROLL:
	    fprintf(fp, "%d '%s' '%s' '%s' '%s'\n",
		     pObjIndex->value[0] > 0 ? /* no negative numbers */
		     pObjIndex->value[0]
		     : 0,
		     pObjIndex->value[1] != -1 ?
		     skill_table[pObjIndex->value[1]].name
		     : "",
		     pObjIndex->value[2] != -1 ?
		     skill_table[pObjIndex->value[2]].name
		     : "",
		     pObjIndex->value[3] != -1 ?
		     skill_table[pObjIndex->value[3]].name
		     : "",
		     pObjIndex->value[4] != -1 ?
		     skill_table[pObjIndex->value[4]].name
		     : "");
	    break;

        case ITEM_STAFF:
        case ITEM_WAND:
	    fprintf(fp, "%d %d %d '%s' %d\n",
	    			pObjIndex->value[0],
	    			pObjIndex->value[1],
	    			pObjIndex->value[2],
	    			pObjIndex->value[3] != -1 ?
	    				skill_table[pObjIndex->value[3]].name :
	    				"",
	    			pObjIndex->value[4]);
	    break;

        case ITEM_CHEST:
            fprintf(fp, "%d %s %d %d %d\n",
                     pObjIndex->value[0],
                     fwrite_flag(pObjIndex->value[1]),
                     pObjIndex->value[2],
                     pObjIndex->value[3],
                     pObjIndex->value[4]);
            break;

        case ITEM_MORTAR:
            fprintf(fp, "%d %s %d %d %d\n",
                     pObjIndex->value[0],		/*макс вместимость*/
                     fwrite_flag(pObjIndex->value[1]),
                     pObjIndex->value[2],		/*количество предметов*/
                     pObjIndex->value[3],               /*макс масса предмета*/
                     pObjIndex->value[4]);              /*мультиплер*/
            break;

        case ITEM_INGREDIENT:
            fprintf(fp, "%d %s %d ",
                     pObjIndex->value[0],		/*время поиска*/
                     fwrite_flag(pObjIndex->value[1]),	/*местность*/
                     pObjIndex->value[2]);		/*раса моба-чара*/
            fprintf(fp, "%s %d\n",
                     fwrite_flag(pObjIndex->value[3]),	/*погода, время года*/
                     pObjIndex->value[4]);		/*шанс найти*/
            break;

    }

    fprintf(fp, "%d ", pObjIndex->level);
    fprintf(fp, "%d ", pObjIndex->weight);
    fprintf(fp, "%d ", pObjIndex->cost);

    fprintf(fp, "%d\n", pObjIndex->condition);

    fprintf(fp, "%s ",
	    fwrite_flag(pObjIndex->uncomf));
    fprintf(fp, "%s\n",
	    fwrite_flag(pObjIndex->unusable));

    for(pAf = pObjIndex->affected; pAf; pAf = pAf->next)
    {
	if (pAf->where == TO_OBJECT || pAf->bitvector == 0)
	        if (pAf->location == APPLY_SKILL)
	            fprintf(fp, "S '%s' %d\n",  skill_table[pAf->type].name, pAf->modifier);
	        else
	            fprintf(fp, "A\n%d %d\n",  pAf->location, pAf->modifier);
	else
	{
		fprintf(fp, "F\n");

		switch(pAf->where)
		{
			case TO_AFFECTS:
				fprintf(fp, "A ");
				break;
			case TO_RESIST:
				fprintf(fp, "R ");
				break;
			default:
				bugf("olc_save: Invalid Affect->where");
				break;
		}
		
		fprintf(fp, "%d %d %s\n", pAf->location, pAf->modifier,
				fwrite_flag(pAf->bitvector));
	}
    }

    for (cnt = 0; cnt < MAX_STATS; cnt++)
      if (pObjIndex->require[cnt] > 0)
        fprintf(fp, "R %s %d\n", flag_string(attr_flags, cnt, FALSE), pObjIndex->require[cnt]);

    if (!IS_NULLSTR(pObjIndex->last_edited))     
      fprintf(fp, "Z %s~\n", pObjIndex->last_edited);
    
    if (!IS_NULLSTR(pObjIndex->comments))
      fprintf(fp, "* %s~\n", pObjIndex->comments);

    if (pObjIndex->timer)
      fprintf(fp, "T %d\n", pObjIndex->timer);
    
    for(pEd = pObjIndex->extra_descr; pEd; pEd = pEd->next)
    {
        fprintf(fp, "E\n%s~\n%s~\n", pEd->keyword,
		 fix_string(pEd->description));
    }
    for (pOprog = pObjIndex->progs; pOprog; pOprog = pOprog->next)
    {
        fprintf(fp, "O%s %s %d %s~\n",
        IS_SET(pOprog->trig_flags, EXEC_BEFORE) ? "B" : IS_SET(pOprog->trig_flags, EXEC_AFTER) ? "A" : "",
        prog_type_to_name(pOprog->trig_type), pOprog->vnum,
                pOprog->trig_phrase);
    }

    return;
}
 



/*****************************************************************************
 Name:		save_objects
 Purpose:	Save #OBJECTS section of an area file.
 Called by:	save_area(olc_save.c).
 Notes:         Changed for ROM OLC.
 ****************************************************************************/
void save_objects(FILE *fp, AREA_DATA *pArea)
{
    int i;
    OBJ_INDEX_DATA *pObj;

    fprintf(fp, "#OBJECTS\n");

    for (i = pArea->min_vnum; i <= pArea->max_vnum; i++)
    {
	if ((pObj = get_obj_index(i)))
	    save_object(fp, pObj);
    }

    fprintf(fp, "#0\n\n\n\n");
    return;
}
 




/*****************************************************************************
 Name:		save_rooms
 Purpose:	Save #ROOMS section of an area file.
 Called by:	save_area(olc_save.c).
 ****************************************************************************/
void save_rooms(FILE *fp, AREA_DATA *pArea)
{
    ROOM_INDEX_DATA *pRoomIndex;
    EXTRA_DESCR_DATA *pEd;
    EXIT_DATA *pExit;
    int iHash;
    int door;
    PROG_LIST *pRprog;

    fprintf(fp, "#ROOMS\n");
    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
        for (pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = pRoomIndex->next)
        {
            if (pRoomIndex->area == pArea)
            {
                AFFECT_DATA *paf, *paf_next, *paf_list = NULL;


                for (paf = pRoomIndex->affected; paf != NULL; paf = paf_next)
                {
                  AFFECT_DATA *paf_temp;

                  paf_next = paf->next;

                  paf_temp = new_affect();

                  if (paf_list)
                  {
                    paf_temp->next = paf_list->next;
                    paf_list->next = paf_temp;
                  }
                  else
                    paf_list = paf_temp;

                  *paf_temp = *paf;

                  affect_remove_room(pRoomIndex, paf);
                }
                
                
                fprintf(fp, "#%d\n",		pRoomIndex->vnum);
                fprintf(fp, "%s~\n",		pRoomIndex->name);
                fprintf(fp, "%s~\n",		fix_string(pRoomIndex->description));
		fprintf(fp, "0 ");
                fprintf(fp, "%ld ",		pRoomIndex->room_flags);
                fprintf(fp, "%d\n",		pRoomIndex->sector_type);

                for (pEd = pRoomIndex->extra_descr; pEd;
                      pEd = pEd->next)
                {
                    fprintf(fp, "E\n%s~\n%s~\n", pEd->keyword,
                                                  fix_string(pEd->description));
                }
                for (door = 0; door < MAX_DIR; door++)	/* I hate this! */
                {
                    if ((pExit = pRoomIndex->exit[door])
                          && pExit->u1.to_room)
                    {
			int locks = 0;
			if (IS_SET(pExit->rs_flags, EX_ISDOOR) 
			&& (!IS_SET(pExit->rs_flags, EX_PICKPROOF)) 
		    	&& (!IS_SET(pExit->rs_flags, EX_NOPASS)))
			    locks = 1;
			if (IS_SET(pExit->rs_flags, EX_ISDOOR)
			&& (IS_SET(pExit->rs_flags, EX_PICKPROOF))
		        && (!IS_SET(pExit->rs_flags, EX_NOPASS)))
			    locks = 2;
			if (IS_SET(pExit->rs_flags, EX_ISDOOR)
			&& (!IS_SET(pExit->rs_flags, EX_PICKPROOF))
		        && (IS_SET(pExit->rs_flags, EX_NOPASS)))
			    locks = 3;
			if (IS_SET(pExit->rs_flags, EX_ISDOOR)
			&& (IS_SET(pExit->rs_flags, EX_PICKPROOF))
		        && (IS_SET(pExit->rs_flags, EX_NOPASS)))
			    locks = 4;

			if (IS_SET(pExit->rs_flags, EX_RACE))
			    locks += 0x10;

                        fprintf(fp, "D%d\n",      pExit->orig_door);
                        fprintf(fp, "%s~\n",      fix_string(pExit->description));
                        fprintf(fp, "%s~\n",      pExit->keyword);
                        fprintf(fp, "%d %d %d\n", locks,
                                                   pExit->key,
                                                   pExit->u1.to_room->vnum);
                    }
                }
		if (pRoomIndex->mana_rate != 100 || pRoomIndex->heal_rate != 100)
		 fprintf (fp, "M %d H %d\n",pRoomIndex->mana_rate,
		                             pRoomIndex->heal_rate);
		if (pRoomIndex->clan > 0)
		    fprintf (fp, "C %s~\n" , clan_table[pRoomIndex->clan].name);

		if (pRoomIndex->min_level != 0 || pRoomIndex->max_level != MAX_LEVEL)
		    fprintf (fp, "L %d %d\n" , pRoomIndex->min_level, pRoomIndex->max_level);
		 			     
		if (!IS_NULLSTR(pRoomIndex->owner))
		    fprintf (fp, "O %s~\n" , pRoomIndex->owner);

    		if (!IS_NULLSTR(pRoomIndex->last_edited))     
      			fprintf(fp, "Z %s~\n", pRoomIndex->last_edited);
    
    		if (!IS_NULLSTR(pRoomIndex->comments))
      			fprintf(fp, "* %s~\n", pRoomIndex->comments);

                for (pRprog = pRoomIndex->progs; pRprog; pRprog = pRprog->next)
	 	  fprintf(fp, "R%s %s %d %s~\n", 
	 	  IS_SET(pRprog->trig_flags, EXEC_BEFORE) ? "B" : IS_SET(pRprog->trig_flags, EXEC_AFTER) ? "A" : "",
	 	  prog_type_to_name(pRprog->trig_type), pRprog->vnum, pRprog->trig_phrase);

		fprintf(fp, "S\n");

                for (paf = paf_list; paf != NULL; paf = paf_next)
                {
                  paf_next = paf->next;

                  affect_to_room(pRoomIndex, paf);

                  free_affect(paf);
                }
            }
        }
    }
    fprintf(fp, "#0\n\n\n\n");
    return;
}



/*****************************************************************************
 Name:		save_specials
 Purpose:	Save #SPECIALS section of area file.
 Called by:	save_area(olc_save.c).
 ****************************************************************************/
void save_specials(FILE *fp, AREA_DATA *pArea)
{
    int iHash;
    MOB_INDEX_DATA *pMobIndex;
    
    fprintf(fp, "#SPECIALS\n");

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
        for (pMobIndex = mob_index_hash[iHash];
	     pMobIndex;
	     pMobIndex = pMobIndex->next)
        {
            if (pMobIndex && pMobIndex->area == pArea && pMobIndex->spec_fun)
            {
#if defined(VERBOSE)
                fprintf(fp, "M %d %s Load to: %s\n", pMobIndex->vnum,
			spec_name(pMobIndex->spec_fun),
			pMobIndex->short_descr);
#else
                fprintf(fp, "M %d %s\n", pMobIndex->vnum,
			spec_name(pMobIndex->spec_fun));
#endif
            }
        }
    }

    fprintf(fp, "S\n\n\n\n");
    return;
}



/*
 * This function is obsolete.  It it not needed but has been left here
 * for historical reasons.  It is used currently for the same reason.
 *
 * I don't think it's obsolete in ROM -- Hugin.
 */
void save_door_resets(FILE *fp, AREA_DATA *pArea)
{
    int iHash;
    ROOM_INDEX_DATA *pRoomIndex;
    EXIT_DATA *pExit;
    int door;

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
        for (pRoomIndex = room_index_hash[iHash];
	     pRoomIndex;
	     pRoomIndex = pRoomIndex->next)
        {
            if (pRoomIndex->area == pArea)
            {
                for (door = 0; door < MAX_DIR; door++)
                {
                    if ((pExit = pRoomIndex->exit[door])
		   	&& pExit->u1.to_room 
		 	&& (IS_SET(pExit->rs_flags, EX_CLOSED)
		   	    || IS_SET(pExit->rs_flags, EX_LOCKED)))
		    {
#if defined(VERBOSE)
			fprintf(fp, "D 0 %5d %2d %5d    "
				    "* The %s door of %s is %s\n", 
				pRoomIndex->vnum,
				pExit->orig_door,
				IS_SET(pExit->rs_flags, EX_LOCKED) ? 2 : 1,
				dir_name[ pExit->orig_door ],
				pRoomIndex->name,
				IS_SET(pExit->rs_flags, EX_LOCKED)
				    ? "closed and locked"
				    : "closed");
#else
			fprintf(fp, "D 0 %d %d %d\n", 
				pRoomIndex->vnum,
				pExit->orig_door,
				IS_SET(pExit->rs_flags, EX_LOCKED) ? 2 : 1);
#endif
		    }
		}
	    }
	}
    }
    return;
}




/*****************************************************************************
 Name:		save_resets
 Purpose:	Saves the #RESETS section of an area file.
 Called by:	save_area(olc_save.c)
 ****************************************************************************/
void save_resets(FILE *fp, AREA_DATA *pArea)
{
    RESET_DATA *pReset;
    MOB_INDEX_DATA *pLastMob = NULL;
    OBJ_INDEX_DATA *pLastObj;
    ROOM_INDEX_DATA *pRoom;
    int iHash;

    fprintf(fp, "#RESETS\n");

    save_door_resets(fp, pArea);

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
        for (pRoom = room_index_hash[iHash]; pRoom; pRoom = pRoom->next)
        {
            if (pRoom->area == pArea)
	    {
		for (pReset = pRoom->reset_first; pReset; pReset = pReset->next)
		{
		    switch (pReset->command)
		    {
		    default:
			bugf("Save_resets: bad command %c.", pReset->command);
			break;

#if defined(VERBOSE)
		    case 'H':
			fprintf(fp, "H 0 %10d %2d %10d %2d "
				    "* Horseman %s on horse %s\n",
				pReset->arg1,
				pReset->arg2,
				pReset->arg3,
				pReset->arg4,
				get_mob_index(pReset->arg1)->short_descr,
				pLastMob ? pLastMob->short_descr : "!NO MOB!");
			pLastMob = get_mob_index(pReset->arg1);
			break;

		    case 'M':
			if ((pLastMob = get_mob_index(pReset->arg1)) == NULL)
			{
			    bugf("Save_resets: cannot find last mob %d in reset.", pReset->arg1);
			    break;
			}
			fprintf(fp, "M 0 %10d %2d %10d %2d * Load %s\n", 
				pReset->arg1,
				pReset->arg2,
				pReset->arg3,
				pReset->arg4,
				pLastMob->short_descr);
			break;

		    case 'O':
			if ((pLastObj = get_obj_index(pReset->arg1)) == NULL 
			 || (pRoom = get_room_index(pReset->arg3)) == NULL)
			{
			    bugf("Save_resets: cannot find last obj %d or last room in reset.", pReset->arg1);
			    break;
			}

			fprintf(fp, "O 0 %10d  0 %10d    "
				    "* %s loaded to %s\n", 
				pReset->arg1,
				pReset->arg3,
				capitalize(pLastObj->short_descr),
				pRoom->name);
			break;

	    	    case 'P':
			if ((pLastObj = get_obj_index(pReset->arg1)) == NULL)
			{
			    bugf("Save_resets: cannot find last obj %d in reset.", pReset->arg1);
			    break;
			}

			fprintf(fp, "P 0 %10d %2d %10d %2d * %s put inside %s\n", 
				pReset->arg1,
				pReset->arg2,
				pReset->arg3,
				pReset->arg4,
				capitalize(
				    get_obj_index(pReset->arg1)->short_descr),
				pLastObj->short_descr);
			break;

	    	    case 'G':
	    	        if ((pLastObj = get_obj_index(pReset->arg1)) == NULL)
			{
			    bugf("Save_resets: cannot find last obj %d in reset.", pReset->arg1);
			    break;
			}

			fprintf(fp, "G 0 %10d   0      "
				    "* %s is given to %s\n",
				pReset->arg1,
				capitalize(pLastObj->short_descr),
				pLastMob ? pLastMob->short_descr : "!NO_MOB!");
			if (!pLastMob)
			{
			    bugf("Save_resets: !NO_MOB! in [%s]",
				 pArea->file_name);
			}
			break;

	    	    case 'E':
			fprintf(fp, "E 0 %10d  0 %10d    "
				    "* %s is loaded %s of %s\n",
				pReset->arg1,
				pReset->arg3,
				capitalize(
				    get_obj_index(pReset->arg1)->short_descr),
				flag_string(wear_loc_strings,
					    pReset->arg3, FALSE),
				pLastMob ? pLastMob->short_descr : "!NO_MOB!");
			if (!pLastMob)
			{
			    bugf("Save_resets: !NO_MOB! in [%s]",
				 pArea->file_name);
			}
			break;

		    case 'D':
			break;

		    case 'R':
			if ((pRoom = get_room_index(pReset->arg1)) == NULL)
			{
			    bugf("Save_resets: cannot find last room %d in reset.", pReset->arg1);
			    break;
			}

			fprintf(fp, "R 0 %10d %2d       "
				    "* Randomize %s\n", 
				pReset->arg1,
				pReset->arg2,
				pRoom->name);
			break;

		    case 'T':
			fprintf(fp, "T 0 %10d %10d\n",
				pReset->arg1,
				pReset->arg2);
			break;

		    case 'C':
			fprintf(fp, "C 0 %10d %10d %5d\n",
				pReset->arg1,
				pReset->arg2,
				pReset->arg3);
			break;

		    case 'X':
			fprintf(fp, "X 0 %10d %10d %5d %10d\n",
				pReset->arg1,
				pReset->arg2,
				pReset->arg3,
				pReset->arg4);
			break;
		    }
#else
		    case 'H':
			fprintf(fp, "H 0 %d %d %d %d\n",
				pReset->arg1,
				pReset->arg2,
				pReset->arg3,
				pReset->arg4);
			pLastMob = get_mob_index(pReset->arg1);
			break;

		    case 'M':
			pLastMob = get_mob_index(pReset->arg1);
			fprintf(fp, "M 0 %d %d %d %d\n", 
				pReset->arg1,
				pReset->arg2,
				pReset->arg3,
				pReset->arg4);
			break;

	    	    case 'O':
			pLastObj = get_obj_index(pReset->arg1);
			pRoom = get_room_index(pReset->arg3);
			fprintf(fp, "O 0 %d 0 %d\n", 
				pReset->arg1,
				pReset->arg3);
			break;

		    case 'P':
			pLastObj = get_obj_index(pReset->arg1);
			fprintf(fp, "P 0 %d %d %d %d\n", 
				pReset->arg1,
				pReset->arg2,
				pReset->arg3,
				pReset->arg4);
    			break;

		    case 'G':
			fprintf(fp, "G 0 %d 0\n", pReset->arg1);
		     	if (!pLastMob)
	    		{
    			    bugf("Save_resets: !NO_MOB! in [%s]",
				 pArea->file_name);
			}
	    		break;

	    	    case 'E':
			fprintf(fp, "E 0 %d 0 %d\n",
				pReset->arg1,
				pReset->arg3);
			if (!pLastMob)
			{
			    bugf("Save_resets: !NO_MOB! in [%s]",
				 pArea->file_name);
			}
			break;

		    case 'D':
			break;

		    case 'R':
			pRoom = get_room_index(pReset->arg1);
			fprintf(fp, "R 0 %d %d\n", 
				pReset->arg1,
				pReset->arg2);
			break;
			
		    case 'T':
			fprintf(fp, "T 0 %10d %10d\n",
				pReset->arg1,
				pReset->arg2);
			break;

		    case 'C':
			fprintf(fp, "C 0 %10d %10d %5d\n",
				pReset->arg1,
				pReset->arg2,
				pReset->arg3);
			break;

		    case 'X':
			fprintf(fp, "X 0 %10d %10d %5d %10d\n",
				pReset->arg1,
				pReset->arg2,
				pReset->arg3,
				pReset->arg4);
			break;
		    }
#endif
		}
	    }	/* End if correct area */
	}	/* End for pRoom */
    }	/* End for iHash */
    fprintf(fp, "S\n\n\n\n");
    return;
}



/*****************************************************************************
 Name:		save_shops
 Purpose:	Saves the #SHOPS section of an area file.
 Called by:	save_area(olc_save.c)
 ****************************************************************************/
void save_shops(FILE *fp, AREA_DATA *pArea)
{
    SHOP_DATA *pShopIndex;
    MOB_INDEX_DATA *pMobIndex;
    int iTrade;
    int iHash;
    
    fprintf(fp, "#SHOPS\n");

    for(iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
        for(pMobIndex = mob_index_hash[iHash]; pMobIndex; pMobIndex = pMobIndex->next)
        {
            if (pMobIndex && pMobIndex->area == pArea && pMobIndex->pShop)
            {
                pShopIndex = pMobIndex->pShop;

                fprintf(fp, "%d ", pShopIndex->keeper);
                for (iTrade = 0; iTrade < MAX_TRADE; iTrade++)
                {
                    if (pShopIndex->buy_type[iTrade] != 0)
                    {
                       fprintf(fp, "%d ", pShopIndex->buy_type[iTrade]);
                    }
                    else
                       fprintf(fp, "0 ");
                }
                fprintf(fp, "%d %d ", pShopIndex->profit_buy, pShopIndex->profit_sell);
                fprintf(fp, "%d %d\n", pShopIndex->open_hour, pShopIndex->close_hour);
            }
        }
    }

    fprintf(fp, "0\n\n\n\n");
    return;
}

void save_helps(FILE *fp, HELP_AREA *ha)
{
	HELP_DATA *help;

	fprintf(fp, "#HELPS\n");

	STAILQ_FOREACH(help, &ha->helps, area_link)
	{
	    fprintf(fp, "%d %s~\n", help->level, help->keyword);
	    fprintf(fp, "%s~\n\n", fix_string(help->text));
	}

	fprintf(fp, "-1 $~\n\n");

	return;
}

void save_area(AREA_DATA *pArea);

void save_other_helps(void)
{
    extern HELP_AREA * had_list;
    HELP_AREA *ha;
    FILE *fp;

    for (ha = had_list; ha; ha = ha->next)
	//		if (ha->area == NULL)
	if (ha->changed)
	{
	    if (!ha->area)
	    {
		fp = file_open(ha->filename, "w");

		if (!fp)
		{
		    _perror(ha->filename);
		    file_close(fp);
		    return;
		}

		save_helps(fp, ha);
		fprintf(fp, "#$\n");
		file_close(fp);
	    }
	    else
	    {
		save_area(ha->area);
	    }
	}

}

/*****************************************************************************
 Name:		save_area
 Purpose:	Save an area, note that this format is new.
 Called by:	do_asave(olc_save.c).
 ****************************************************************************/
void save_area(AREA_DATA *pArea)
{
    FILE *fp;

    if (!(fp = file_open(pArea->file_name, "w"))) {
	    bugf("Open_area: file_open(\"%s\"", pArea->file_name);
	    _perror(pArea->file_name);
        return;
    }
    //else if (pArea){
    fprintf(fp, "#AREADATA\n");
    fprintf(fp, "Version %d\n",   area_version);
    fprintf(fp, "Name %s~\n",     pArea->name);
    fprintf(fp, "Builders %s~\n", fix_string(pArea->builders));
    fprintf(fp, "VNUMs %d %d\n",  pArea->min_vnum, pArea->max_vnum);
    fprintf(fp, "Credits %s~\n",  pArea->credits);
    fprintf(fp, "Security %d\n",  pArea->security);

    if (pArea->area_flags)
        fprintf(fp, "Flags %s\n", fwrite_flag(pArea->area_flags));

    fprintf(fp, "End\n\n\n\n");

    save_mobiles(fp, pArea);
    save_objects(fp, pArea);
    save_rooms(fp, pArea);
    save_specials(fp, pArea);
    save_resets(fp, pArea);
    save_shops(fp, pArea);
    save_mobprogs(fp, pArea);
    save_objprogs(fp, pArea);
    save_roomprogs(fp, pArea);
    save_chat_data(fp, pArea);

    if (pArea->helps && !STAILQ_EMPTY(&pArea->helps->helps))
        save_helps(fp, pArea->helps);

    fprintf(fp, "#$\n");
    fprintf(fp, "* charset=cp1251");

    if (cfg.use_db)
        dbdump(pArea);
//    }
//    else {
//        return;
//    }


    file_close(fp);
    return;
}


/*****************************************************************************
 Name:		do_asave
 Purpose:	Entry point for saving area data.
 Called by:	interpreter(interp.c)
 ****************************************************************************/
void do_asave(CHAR_DATA *ch, char *argument)
{
    char arg1 [MAX_INPUT_LENGTH];
    AREA_DATA *pArea;
    FILE *fp;
    int value, sec;

    fp = NULL;

    if (!ch)       /* Do an autosave */
	sec = 9;
    else if (!IS_NPC(ch))
    	sec = ch->pcdata->security;
    else
    	return;

/*    {
	save_area_list();
	for(pArea = area_first; pArea; pArea = pArea->next)
	{
	    save_area(pArea);
	    REMOVE_BIT(pArea->area_flags, AREA_CHANGED);
	}
	return;
    } */

    smash_tilde(argument);
    strcpy(arg1, argument);

    if (arg1[0] == '\0')
    {
	if (ch)
	{
		send_to_char("Syntax:\n\r", ch);
		send_to_char("  asave <vnum>   - saves a particular area\n\r",	ch);
/*		send_to_char("  asave list     - saves the area.lst file\n\r",	ch); */
		send_to_char("  asave area     - saves the area being edited\n\r",	ch);
		send_to_char("  asave changed  - saves all changed zones\n\r",	ch);
/*		send_to_char("  asave world    - saves the world! (db dump)\n\r",	ch); */
		send_to_char("\n\r", ch);
	}

	return;
    }

    /* Snarf the value (which need not be numeric). */
    value = atoi(arg1);
    if (!(pArea = get_area_data(value)) && is_number(arg1))
    {
	if (ch)
		send_to_char("That area does not exist.\n\r", ch);
	return;
    }

    /* Save area of given vnum. */
    /* ------------------------ */
    if (is_number(arg1))
    {
	if (ch && !IS_BUILDER(ch, pArea))
	{
	    send_to_char("You are not a builder for this area.\n\r", ch);
	    return;
	}

/*	save_area_list(); */
	REMOVE_BIT(pArea->area_flags, AREA_CHANGED);
	return;
    }

    /* Save the world, only authorized areas. */
    /* -------------------------------------- */
/*    if (!str_cmp("world", arg1))
    {
	save_area_list();
	for(pArea = area_first; pArea; pArea = pArea->next)
	{
	    if (ch && !IS_BUILDER(ch, pArea))
		continue;

	    save_area(pArea);
	    REMOVE_BIT(pArea->area_flags, AREA_CHANGED);
	}

	if (ch)
		send_to_char("You saved the world.\n\r", ch);

	if (sec == 9)
		save_other_helps();

	return;
    } 
*/
    /* Save changed areas, only authorized areas. */
    /* ------------------------------------------ */
    if (!str_cmp("changed", arg1))
    {
	char buf[MAX_INPUT_LENGTH];

/*	save_area_list();  */

	if (ch)
	    send_to_char("Saved zones:\n\r", ch);
	else
	    log_string("Saved zones:");

	sprintf(buf, "None.\n\r");

	for (pArea = area_first; pArea; pArea = pArea->next)
	{
	    /* Builder must be assigned this area. */
	    if (ch && !IS_BUILDER(ch, pArea))
		continue;

	    /* Save changed areas. */
	    if (IS_SET(pArea->area_flags, AREA_CHANGED))
	    {
		REMOVE_BIT(pArea->area_flags, AREA_CHANGED);
		save_area(pArea);
		sprintf(buf, "%24s - '%s'", pArea->name, pArea->file_name);
		if (ch)
		{
		    send_to_char(buf, ch);
		    send_to_char("\n\r", ch);
		}
		else
		    log_string(buf);
	    }
	}

	if (!str_cmp(buf, "None.\n\r"))
	{
		if (ch)
			send_to_char(buf, ch);
		else
			log_string("None.");
	}			

	return;
    }

    /* Save the area.lst file. */
    /* ----------------------- */
/*    if (!str_cmp(arg1, "list"))
    {
	save_area_list();
	return;
    } */

    /* Save area being edited, if authorized. */
    /* -------------------------------------- */
    if (!str_cmp(arg1, "area"))
    {
	if (!ch || !ch->desc)
		return;

	/* Is character currently editing. */
	if (ch->desc->editor == ED_NONE)
	{
	    send_to_char("You are not editing an area, "
		"therefore an area vnum is required.\n\r", ch);
	    return;
	}
	
	/* Find the area to save. */
	switch (ch->desc->editor)
	{
	    case ED_AREA:
		pArea = (AREA_DATA *)ch->desc->pEdit;
		break;
	    case ED_ROOM:
		pArea = ch->in_room->area;
		break;
	    case ED_OBJECT:
		pArea = ((OBJ_INDEX_DATA *)ch->desc->pEdit)->area;
		break;
	    case ED_MOBILE:
		pArea = ((MOB_INDEX_DATA *)ch->desc->pEdit)->area;
		break;
	    default:
		pArea = ch->in_room->area;
		break;
	}

	if (!IS_BUILDER(ch, pArea))
	{
	    send_to_char("You are not a builder for this area.\n\r", ch);
	    return;
	}

/*	save_area_list(); */
	REMOVE_BIT(pArea->area_flags, AREA_CHANGED);
	save_area(pArea);
	send_to_char("Area saved.\n\r", ch);
	return;
    }

    /* Show correct syntax. */
    /* -------------------- */
    if (ch)
	do_asave(ch, "");

    return;
}

void do_hsave(CHAR_DATA *ch, char *argument)
{
    if (argument[0] != '\0')
    {
	send_to_char("Syntax:   hsave\n\r"
	             "Note, without any arguments."
		     "This command saves all changed helps.\n\r", ch);

	return;
    }

    save_other_helps();
    send_to_char("Helps saved.\n\r", ch);

}
/* charset=cp1251 */
