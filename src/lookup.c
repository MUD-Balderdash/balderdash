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
#include <stdlib.h>
#include <time.h>
#include "merc.h"


CHAR_DATA *pc_id_lookup(long id)
{
    DESCRIPTOR_DATA *d;
    CHAR_DATA *ch;

    SLIST_FOREACH(d, &descriptor_list, link)
    {
	ch = CH(d);

	if (ch && ch->id == id)
	    return ch;
    }

    return NULL;
}

int64_t flag_lookup (const char *name, const struct flag_type *flag_table)
{
    int flag;

    if (IS_NULLSTR(name))
	return NO_FLAG;

    for (flag = 0; flag_table[flag].name != NULL; flag++)
    {
	if ((LOWER(name[0]) == LOWER(flag_table[flag].name[0])
		&& !str_prefix(name, flag_table[flag].name))
	    || (LOWER(name[0]) == LOWER(flag_table[flag].rname[0])
		&& !str_prefix(name, flag_table[flag].rname)))
	{
	    return flag_table[flag].bit;
	}
    }

    return NO_FLAG;
}
 
int clan_lookup(const char *name)
{
    int clan;
    
    if (IS_NULLSTR(name))
  	return 0;

    for (clan = 0; clan < MAX_CLAN; clan++)
	if  (clan_table[clan].valid && !str_prefix(name,clan_table[clan].name))
	    return clan;

    return 0;
}

int position_lookup (const char *name)
{
   int pos;

   if (IS_NULLSTR(name))
      return -1;

   for (pos = 0; position_table[pos].name != NULL; pos++)
   {
	if (LOWER(name[0]) == LOWER(position_table[pos].name[0])
	&&  !str_prefix(name,position_table[pos].name))
	    return pos;
   }
   
   return -1;
}

int sex_lookup (const char *name)
{
   int sex;

   if (IS_NULLSTR(name))
      return 0;
   
   for (sex = 0; sex_table[sex].name != NULL; sex++)
   {
	if (LOWER(name[0]) == LOWER(sex_table[sex].name[0])
	&&  !str_prefix(name,sex_table[sex].name))
	    return sex;
   }

   return 0;
}

int size_lookup(const char *name)
{
   int size;

   if (IS_NULLSTR(name))
      return 0;
   
   for (size = 0; size_table[size].name != NULL; size++)
   {
        if (LOWER(name[0]) == LOWER(size_table[size].name[0])
	    && !str_prefix(name,size_table[size].name))
	{
            return size;
	}
   }
 
   return 0;
}

/* returns race number */
int race_lookup(const char *name)
{
    int race;

    if (IS_NULLSTR(name))
	return -1;

    for (race = 0; race < max_races; race++)
    {
	if ((LOWER(name[0]) == LOWER(race_table[race].name[0])
	     && !str_prefix(name,race_table[race].name))
	    || (LOWER(name[0]) == LOWER(race_table[race].rname[0])
		&& !str_prefix(name,race_table[race].rname)))
	{
	    return race;
	}
    }

    return -1;
} 

int liq_lookup(const char *name)
{
    int liq;
    
    if (IS_NULLSTR(name))
      return 0;

    for (liq = 0; liq_table[liq].liq_name != NULL; liq++)
    {
	if ((LOWER(name[0]) == LOWER(liq_table[liq].liq_name[0])
	&& !str_prefix(name,liq_table[liq].liq_name)) ||
	(LOWER(name[0]) == LOWER(liq_table[liq].liq_rname[0])
	&& !str_prefix(name,liq_table[liq].liq_rname)))
	    return liq;
    }

    return 0;
}

int weapon_lookup (const char *name)
{
    int type;

    if (IS_NULLSTR(name))
      return -1;

    for (type = 0; weapon_table[type].name != NULL; type++)
    {
	if ((LOWER(name[0]) == LOWER(weapon_table[type].name[0])
	&&  !str_prefix(name,weapon_table[type].name)) ||
         (LOWER(name[0]) == LOWER(weapon_table[type].rname[0])
	&&  !str_prefix(name,weapon_table[type].rname)))
	    return type;
    }
 
    return -1;
}


int item_lookup(const char *name)
{
    int type;

    if (IS_NULLSTR(name))
      return -1;

    for (type = 0; item_table[type].name != NULL; type++)
    {
        if (LOWER(name[0]) == LOWER(item_table[type].name[0])
        &&  !str_prefix(name,item_table[type].name))
            return item_table[type].type;
    }
 
    return -1;
}

int wear_loc_lookup(const char *name)
{
    int type;

    if (IS_NULLSTR(name))
      return WEAR_ANYWHERE;

    for (type = 0; wear_loc_flags[type].name != NULL; type++)
    {
        if (LOWER(name[0]) == LOWER(wear_loc_flags[type].name[0])
        &&  !str_prefix(name,wear_loc_flags[type].name))
            return (int) wear_loc_flags[type].bit;
    }

    return WEAR_ANYWHERE;
}

int material_lookup (const char *name)
{
    int mat;

    if (IS_NULLSTR(name))
      return -1;

    for (mat = 0; material_table[mat].name != NULL; mat++)
    {
	if (name[0] == material_table[mat].name[0]
	&& !str_prefix(name,material_table[mat].name)) 
	    return mat;
    }

    return -1;
}

int attack_lookup_dam  (int dt)
{
    int att;

    for (att = 0; attack_table[att].name != NULL; att++)
    {
	if (attack_table[att].damage == dt
	    && (!str_prefix(attack_table[att].name,"shbite")
	    || !str_prefix(attack_table[att].name,"frbite")
	    || !str_prefix(attack_table[att].name,"acbite")))
	    return att;
    }

    return 0;
}

int attack_lookup  (const char *name)
{
    int att;

    if (IS_NULLSTR(name))
      return 0;

    for (att = 0; attack_table[att].name != NULL; att++)
    {
	if (LOWER(name[0]) == LOWER(attack_table[att].name[0])
	&&  !str_prefix(name,attack_table[att].name))
	    return att;
    }

    return 0;
}

/* returns a flag for wiznet */
int wiznet_lookup (const char *name)
{
    int flag;

    if (IS_NULLSTR(name))
      return -1;

    for (flag = 0; wiznet_table[flag].name != NULL; flag++)
    {
	if (LOWER(name[0]) == LOWER(wiznet_table[flag].name[0])
	&& !str_prefix(name,wiznet_table[flag].name))
	    return flag;
    }

    return -1;
}

/* returns class number */
int class_lookup (const char *name)
{
   int classid;
 
   if (IS_NULLSTR(name))
      return -1;

   for (classid = 0; classid < MAX_CLASS && !IS_NULLSTR(class_table[classid].name); classid++)
   {	
        if (LOWER(name[0]) == LOWER(class_table[classid].name[0])
	    && !str_prefix(name,class_table[classid].name))
	{
            return classid;
	}
   }
 
   return -1;
}

int skill_lookup(const char *name)
{
    int sn;

    if (IS_NULLSTR(name)) return -1;

    for (sn = 0; sn < max_skills; sn++)
    {
	if (IS_NULLSTR(skill_table[sn].name))
	    break;

	if ((LOWER(name[0]) == LOWER(skill_table[sn].name[0])
		&& !str_prefix(name, skill_table[sn].name))
	    || (LOWER(name[0]) == LOWER(skill_table[sn].rname[0])
		&& !str_prefix(name, skill_table[sn].rname)))
	{
	    return sn;
	}
    }

    return -1;
}

/*
 * Lookup a skill by slot number.
 * Used for object loading.
 */
int slot_lookup(int slot)
{
    extern bool fBootDb;
    int sn;

    if (slot <= 0)
	return -1;

    for (sn = 0; sn < max_skills; sn++)
    {
	if (slot == skill_table[sn].slot)
	    return sn;
    }

    if (fBootDb)
    {
	bugf("Slot_lookup: bad slot %d.", slot);
	abort();
    }

    return -1;
}

int group_lookup(const char *name)
{
    int gn;

    if (IS_NULLSTR(name))
	return -1;

    for (gn = 0; gn < max_groups; gn++)
    {
	if (group_table[gn].name == NULL)
	    break;

	if ((LOWER(name[0]) == LOWER(group_table[gn].name[0])
	     && !str_prefix(name, group_table[gn].name)) ||
	    (LOWER(name[0]) == LOWER(group_table[gn].rname[0])
	     && !str_prefix(name, group_table[gn].rname)))
	{
	    return gn;
	}
    }

    return -1;
}

/*****************************************************************************
 Name:		spec_lookup
 Purpose:	Given a name, return the appropriate spec fun.
 Called by:	do_mset(act_wiz.c) load_specials,reset_area(db.c)
 ****************************************************************************/
SPEC_FUN *spec_lookup(const char *name)
{
   int i;
   
   if (IS_NULLSTR(name))
      return 0;
 
   for (i = 0; spec_table[i].name != NULL; i++)
   {
        if (LOWER(name[0]) == LOWER(spec_table[i].name[0])
	    && !str_prefix(name,spec_table[i].name))
	{
            return spec_table[i].function;
	}
   }
 
   return 0;
}
/* charset=cp1251 */
