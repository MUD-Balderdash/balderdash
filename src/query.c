/**************************************************************************
***************************************************************************/



#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "recycle.h"


QUERY_DATA *query_list;

void load_query(void)
{
    FILE *fpquery;

    if ((fpquery = file_open(QUERY_FILE, "r")) == NULL){
		_perror(QUERY_FILE);
		exit(1);
    }

    for (; ;)
    {
	QUERY_DATA *pquery;
	char *word;

	if (fread_letter(fpquery) != '#')
	{
	    bugf("Load_query: # not found.");
	    exit(1);
	}

	word = fread_word(fpquery);
	if (!str_cmp(word, "END"))
	    break;
	
	pquery         = new_query();
	pquery->name   = str_dup(fread_string(fpquery));
	pquery->text   = str_dup(fread_string(fpquery));

	if (query_list == NULL)
	    query_list = pquery;
	else
	{
	    pquery->next = query_list;
	    query_list = pquery;
	}
    }
    file_close(fpquery);
    return;
}

QUERY_DATA *add_query(char *name)
{
    QUERY_DATA *query;

    query = new_query();
    query->name = str_dup(name);

    if (query_list == NULL)
	query_list = query;
    else
    {
	query->next = query_list;
	query_list = query;
    }

    save_query();
    return query;
}

void show_query(CHAR_DATA *ch, QUERY_DATA *query)
{
    char buf[MSL];

    sprintf(buf, "Name : %s\n\r",query->name);
    send_to_char(buf,ch);
    sprintf(buf, "Text :\n\r %s\n\r",query->text);
    send_to_char(buf,ch);
    return;
}

void list_query(CHAR_DATA *ch)
{
    QUERY_DATA *query;
    int t = 1;
    char buf[MSL];

    send_to_char(" N.   Название\n\r",ch);
    send_to_char("====|========================================================================\n\r", ch);
    for (query  = query_list; query != NULL; query  = query->next)
    {
	sprintf(buf, "%-3d | %s\n\r",t++,query->name);
	send_to_char(buf,ch);
    }
    send_to_char("=============================================================================\n\r", ch);
}                                                                                                            

QUERY_DATA *get_query(int num)
{
    QUERY_DATA *query;
    int i = 1;

    for (query  = query_list;
	 query != NULL;
	 query  = query->next)
    {
	if (i++ == num)
	    break;
    }
    return query;
}

void delete_query(QUERY_DATA *query)
{
    if (query == NULL)
	return;

    if (query == query_list)
    {
	query_list = query->next;
    }
    else
    {
	QUERY_DATA *prev;

	for (prev = query_list; prev != NULL; prev = prev->next)
	{
	    if (prev->next == query)
	    {
		prev->next = query->next;
		break;
	    }
	}

	if (prev == NULL)
	    return;
    }
    free_query(query);
    save_query();
    return;
}

void save_query(void)
{
    FILE *fpquery;
    QUERY_DATA *query;

    if ((fpquery = file_open(QUERY_FILE, "w")) == NULL)
    {
	bugf("Save_query: Cannot open QUERY_FILE file.");
	_perror(QUERY_FILE);
    }
    else
    {
	for (query  = query_list;
	     query != NULL;
	     query  = query->next)
	{
	    fprintf(fpquery, "#0\n");
	    fprintf(fpquery, "%s~\n", query->name);
	    fprintf(fpquery, "%s~\n", query->text);
	    fprintf(fpquery, "\n");
	}
	fprintf(fpquery, "#END\n\r");
    }
    file_close(fpquery);

    return;
}

bool check_query_obj(QUERY_DATA *query, OBJ_INDEX_DATA *obj)
{              
    bool check1 = FALSE, check2 = TRUE;
    char *code, *line;
    char data[MSL];
    char arg1[MIL];
    char arg2[MIL];
    char arg3[MIL];
    char arg4[MIL];
    char *spar1 = NULL;
    int par1 = 0, par2 = 0, tpar = 0, wpar = 0;
    AFFECT_DATA *paf;

    if (query->text == '\0')
	return FALSE;

    code = query->text;

    while (*code)
    {
	char *d = data;

	while(IS_SPACE(*code) && *code)
	    code++;

	while (*code)
	{
	    if (*code == '\n' || *code == '\r')
	    {
		break;
	    }
	    else 
	    {
		*d++ = *code++;
	    }
	}
	*d = '\0';
	line = data;

	line = one_argument(line, arg1);
	line = one_argument(line, arg2);
	line = one_argument(line, arg3);
	line = one_argument(line, arg4);
	par1 = 0;
	tpar = -999;
	wpar = -999;
	par2 = 0;
	spar1 = str_dup("");

	if (arg1 == '\0' || arg2 == '\0' || arg3 == '\0' || arg4 == '\0') 
	    return FALSE;

	switch (UPPER(arg1[0]))
	{
	    case 'A':
		if (!str_prefix(arg1,"affects"))
		    wpar = 1;
	    break;
	    case 'O':
		if (!str_prefix(arg1,"object"))
		    wpar = 2;
	    break;
	    case 'R':
		if (!str_prefix(arg1,"resist"))
		    wpar = 3;
	    break;
	}

	if (wpar == 1)
	{
	    switch (UPPER(arg2[0]))
	    {
		case 'A':
		    if (!str_prefix(arg2,"ac"))
			tpar = APPLY_AC;
		    if (!str_prefix(arg2,"affected"))
			tpar = -5;
		break;
		case 'C':
		    if (!str_prefix(arg2,"con"))
			tpar = APPLY_CON;
		break;
		case 'D':
		    if (!str_prefix(arg2,"damroll"))
			tpar = APPLY_DAMROLL;
		    if (!str_prefix(arg2,"dex"))
			tpar = APPLY_DEX;
		break;
		case 'H':
		    if (!str_prefix(arg2,"hitroll"))
			tpar = APPLY_HITROLL;
		    if (!str_prefix(arg2,"hp"))
			tpar = APPLY_HIT;
		break;
		case 'I':
		    if (!str_prefix(arg2,"int"))
			tpar = APPLY_INT;
		break;
		case 'M':
		    if (!str_prefix(arg2,"mana"))
			tpar = APPLY_MANA;
		    if (!str_prefix(arg2,"move"))
			tpar = APPLY_MOVE;
		break;
		case 'S':
		    if (!str_prefix(arg2,"saves"))
			tpar = APPLY_SAVES;
		    if (!str_prefix(arg2,"str"))
			tpar = APPLY_STR;
		break;
		case 'W':
		    if (!str_prefix(arg2,"wis"))
			tpar = APPLY_WIS;
		break;
	    }
	    for (paf = obj->affected; paf != NULL; paf = paf->next)
	    {
		if (paf->location == tpar && tpar != -5)
		    par1 += paf->modifier;

		if (tpar == -5
		&& paf->bitvector
		&& paf->where == TO_AFFECTS
		&& str_str(flag_string(affect_flags, paf->bitvector, FALSE),arg4))
		    spar1 = arg4;
	    }
	}

	if (wpar == 2)
	{
	    switch (UPPER(arg2[0]))
	    {
		case 'C':
		    if (!str_prefix(arg2,"cost"))
			par1 = obj->cost;
		    if (!str_prefix(arg2,"condition"))
			par1 = obj->condition;
		break;
		case 'L':
		    if (!str_prefix(arg2,"level"))
			par1 = obj->level;
		break;
		case 'M':
		    if (!str_prefix(arg2,"material"))
			spar1 = obj->material;
		break;
		case 'T':
		    if (!str_prefix(arg2,"type"))			
			spar1 = flag_string(type_flags, obj->item_type, FALSE);
		break;
		case 'V':
		    if (!str_prefix(arg2,"vnum"))			
			par1 = obj->vnum;
		break;
		case 'W':
		    if (!str_prefix(arg2,"weight"))
			par1 = obj->weight;
		    if (!str_prefix(arg2,"wear"))			
			spar1 = flag_string(wear_flags, obj->wear_flags, FALSE);
		break;
	    }
	}

	if (wpar == 3)
	{
	    for (paf = obj->affected; paf != NULL; paf = paf->next)
	    {
		if (paf->bitvector 
		&& str_str(arg2, flag_string(dam_flags, paf->bitvector, FALSE)))
		    par1 += paf->modifier;
	    }
	}

	if (is_number(arg4))
	{
	    par2 = atoi(arg4);
	    check2 = (!str_prefix(arg3,"==") ? par1 == par2:
		    !str_prefix(arg3,">") ? par1 > par2:
		    !str_prefix(arg3,"<") ? par1 < par2:
		    !str_prefix(arg3,"!=") ? par1 != par2:
		    !str_prefix(arg3,">=") ? par1 >= par2:
		    !str_prefix(arg3,"<=") ? par1 <= par2: FALSE);
	}
	else
	{
	    check2 = FALSE;
	    if (spar1 && !str_prefix(arg3,"==") && str_str(spar1, arg4))
		check2 = TRUE;
	    if (spar1 && !str_prefix(arg3,"!=") && str_str(spar1, arg4))
		check2 = TRUE;
	}

	if (check2)
	    check1 = check2;
	else if (wpar > 0)	
	    return FALSE;
    }
    return check1;
}

bool check_query_mob(QUERY_DATA *query, MOB_INDEX_DATA *mob)
{   
    bool check = FALSE;

    if (query->text == '\0')
	return FALSE;
    
    return check;
}


/* charset=cp1251 */
