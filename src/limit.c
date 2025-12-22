#include <sys/time.h>
#include <stdio.h>
#include <math.h>

#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "merc.h"
#include "recycle.h"


int is_limit(OBJ_DATA *obj)
{
    int i;

    for (i = 0; limits[i] > 0 && i < MAX_LIMITS; i += 3)
	if (obj->pIndexData->vnum == limits[i])
	    return i;

    return -1;
}

void return_limit_from_char(CHAR_DATA *ch, bool extract)
{
    OBJ_DATA *lobj, *lobj_next;

    for (lobj = ch->carrying; lobj != NULL; lobj = lobj_next)
    {
	lobj_next = lobj->next_content;

	if (lobj->contains != NULL)
	{
	    OBJ_DATA *obj_in, *in_next;

	    for (obj_in = lobj->contains; obj_in != NULL; obj_in = in_next)
	    {
		in_next = obj_in->next_content;

		if (is_limit(obj_in) != -1)
		{
			extract_obj(obj_in, TRUE, FALSE);
		}
	    }
	}

	if (is_limit(lobj) != -1
	    && ((lobj->wear_loc == WEAR_NONE
		 && lobj->item_type != ITEM_CONTAINER)
		|| extract))
	{
		extract_obj(lobj, TRUE, FALSE);
	}
    }
}

bool is_have_limit(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj)
{
    OBJ_DATA *lobj, *obj_in;
    int indx;

    if (obj->contains != NULL)
	for (obj_in = obj->contains; obj_in; obj_in = obj_in->next_content)
	    if (is_have_limit(ch, victim, obj_in))
		return TRUE; 

    if ((indx = is_limit(obj)) == -1)
	return FALSE;

    for (lobj = ch->carrying; lobj != NULL; lobj = lobj->next_content)
    {
	if (lobj->pIndexData->vnum == limits[indx])
	{
	    if (victim != ch) 
	    {
		act("У $S1 уже есть $p.", victim, lobj, ch, TO_CHAR);
	    }	
	    else
	    {
		if (lobj->wear_loc == WEAR_NONE)
		{
		    act("Поищи $p6 у себя в инвентаре.", ch, lobj, NULL, TO_CHAR);
		}
		else
		{
		    act("Поищи $p6 на себе.", ch, lobj, NULL, TO_CHAR);
		}
	    }
	    return TRUE;
	} 
	if (lobj->contains != NULL)
	{
	    for (obj_in = lobj->contains; obj_in; obj_in = obj_in->next_content)
	    {
		if (obj_in->pIndexData->vnum == limits[indx] && obj_in != obj)
		{
		    if (victim != ch) 
		    {
			act("У $S1 уже есть $p.", victim, obj_in, ch, TO_CHAR);
		    }
		    else
		    {
			act("Поищи $p6 у себя в $P5.", ch, obj_in, lobj, TO_CHAR);
		    }

		    return TRUE; 
		}     
	    }
	}
    }

    for (indx = 0; indx < MAX_AUCTION; indx++)
	if ( auctions[indx].valid 
	     && auctions[indx].buyer == ch 
	     && auctions[indx].obj->pIndexData == obj->pIndexData)
	{
	    if (victim != ch)
	    {
		act("$E уже пытается купить $p6 на аукционе.", victim, obj, ch, TO_CHAR);
	    }
	    else
	    {
		act("Ты уже пытаешься купить $p6 на аукционе.", ch, obj, NULL, TO_CHAR);
	    }

	    return TRUE;
	}

    return FALSE;
}

bool check_container_limit(OBJ_DATA *container, OBJ_DATA *obj)
{
    OBJ_DATA *obj_in;
    int indx;

    if ((indx = is_limit(obj)) == -1)
	return FALSE;

    for (obj_in = container->contains; obj_in; obj_in = obj_in->next_content)
	if (obj_in->pIndexData->vnum == limits[indx])
	    return TRUE; 

    return FALSE;
}

bool container_have_limit(OBJ_DATA *container)
{
    OBJ_DATA *obj_in;

    if (container->contains != NULL)
    for (obj_in = container->contains; obj_in; obj_in = obj_in->next_content)
	if (is_limit(obj_in) != -1)
	    return TRUE; 

    return FALSE;
}

bool check_reset_for_limit(OBJ_DATA *obj, bool clone)
{
    int indx;

    if ((indx = is_limit(obj)) != -1)
    {   
	if (limits[indx+1] >= limits[indx+2])
	{
	    if (!clone)
		SET_BIT(obj->extra_flags, ITEM_INVENTORY);  

	    return TRUE;
	}
	else
	    limits[indx+1]++;
    }

    return FALSE;
}

bool check_prog_limit(OBJ_INDEX_DATA *pObjIndex)
{
    int i;

    for (i=0; limits[i] > 0 && i < MAX_LIMITS; i+=3)
	if (pObjIndex->vnum == limits[i] && limits[i+1] >= limits[i+2])
	    return TRUE;

    return FALSE;
}


void save_limits(void)
{
    FILE *fp;
    int i;

    if ((fp = file_open(LIMITS_FILE, "w")) == NULL)
	bugf("Couldn't open limits file for writing ...");
    else
    {
	for (i = 0; (i < MAX_LIMITS && limits[i] >= 0); i += 3)
	    fprintf(fp, "%d %d %d\n", limits[i], limits[i + 1], limits[i + 2]);

	fprintf(fp, "-1\n");
    }
    file_close(fp);
    return;
}

void do_limits(CHAR_DATA *ch, char *argument)
{
    int i, vnum = 0;
    OBJ_INDEX_DATA *obj;
    BUFFER *output;
    char bfr[MAX_STRING_LENGTH], arg[MAX_INPUT_LENGTH];

    vnum = 0;

    one_argument(argument, arg);

    if (is_number(arg))
	vnum = atoi(arg);

    output = new_buf();

    send_to_char("\n\rVnum   Description                      Status    {DLevel{x\n\r"
		 "===============================================================\n\r", ch);

    for (i = 0; (i < MAX_LIMITS && limits[i] >= 0); i += 3)
    {
	obj = get_obj_index(limits[i]);

	if (arg[0] == '\0' || limits[i] == vnum
	    || (obj && (is_name(arg, obj->name)
			|| is_name(arg, obj->short_descr))))
	{
	    char color;

	    if (limits[i+1] > limits[i+2])
	        color = 'R';
	    else
	        color = 'x';

	    sprintf(bfr, "%-5d  %s   {%c%2d (%2d){x     {D%-2d{x\n\r", limits[i],
		    str_color_fmt((obj == NULL)
				  ? "{RОбъект отсутствует{x"
				  : obj->short_descr, 30),
		    color, limits[i+1], limits[i+2], obj ? obj->level : 0);
	    add_buf(output, bfr);
	}
    }

    page_to_char(buf_string(output), ch);
    free_buf(output);
    return;
}

/* charset=cp1251 */


