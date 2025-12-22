/* $Id: dbdump.c,v 1.9.2.1 2009/01/21 10:38:49 sasha Exp $ */

#if defined(ONEUSER)
void db_tables()
{
}

void dbdump()
{
}
#else

#include <sys/types.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef NO_MYSQL
#include <mysql.h>
#endif

#include "merc.h"
#include "config.h"

void strip_colors(char *str);
#ifndef NO_MYSQL
static MYSQL *con;

/*
 * NOTE: this macro use GCC-specific extentsion
 */
#define MYSQL_QUERY(con, query, args...)				  \
	    {								  \
		sprintf(buf, query, ## args);				  \
		if (mysql_query(con, buf) != 0)				  \
		{							  \
		    bugf("[MySQL] Error while proceeding query '%s': %s", \
			 buf, mysql_error(con));			  \
		    return;						  \
		}							  \
	    }

static void dumpall(MYSQL *con);
static void dumpvnums(MYSQL *con, int from, int to);
 
void dbdump(AREA_DATA *area)
{
    int from, to;
    char buf[5 * MAX_STRING_LENGTH];

    con = mysql_init(NULL);
    if (!mysql_real_connect(con, cfg.db_host, cfg.db_user, cfg.db_passwd,
     			    cfg.db_name, cfg.db_port, NULL, 0))
    {
	bugf("[MySQL] Can't connect to database '%s' on %s:%d as '%s' "
	     "identified by '%s': %s",
	     cfg.db_name, cfg.db_host, cfg.db_port, cfg.db_user, cfg.db_passwd,
	     mysql_error(con));
	return;
    }
    
    if (!area)
    {
	/* Clear table */
	MYSQL_QUERY(con, "DELETE FROM affects");
	MYSQL_QUERY(con, "DELETE FROM objects");
	
	dumpall(con);
    }
    else
    {
	from = area->min_vnum;
	to = area->max_vnum;
	
	MYSQL_QUERY(con, "DELETE FROM affects "
			 "WHERE object_id >= %d AND object_id <= %d",
		    from, to);
	MYSQL_QUERY(con, "DELETE FROM objects "
			 "WHERE vnum >= %d AND vnum <= %d",
		    from, to);

	dumpvnums(con, from, to);
    }

    mysql_close(con);
}    

static void dumpall(MYSQL *con)
{
    dumpvnums(con, -1, MAX_VNUM);
}

static void dumpvnums(MYSQL *con, int from, int to)
{
    int i;
    OBJ_INDEX_DATA *obj;
    AFFECT_DATA *aff;
    char name[MAX_STRING_LENGTH],
	 sh_desc[MAX_STRING_LENGTH],
	 l_desc[MAX_STRING_LENGTH];
    char buf[5 * MAX_STRING_LENGTH];

    for (i = 0; i < MAX_KEY_HASH; i++)
	for (obj = obj_index_hash[i]; obj; obj = obj->next)
	{
	    if (obj->vnum < from || obj->vnum > to)
		continue;

	    mysql_escape_string(name, obj->name, strlen(obj->name));
	    mysql_escape_string(sh_desc, obj->short_descr,
				strlen(obj->short_descr));
	    mysql_escape_string(l_desc, obj->description,
				strlen(obj->description));

	    MYSQL_QUERY(con, "INSERT INTO objects VALUES"
			     "("
			     "  %u,"	/* vnum		*/
			     "  '%s',"	/* name		*/
			     "  '%s',"	/* sh_desc	*/
			     "  '%s',"	/* l_desc	*/
			     "  %u,"	/* type		*/
			     "  %u,"	/* condition	*/
			     "  %u,"	/* weight	*/
			     "  %u,"	/* level	*/
			     "  %u,"	/* cost		*/
			     "  %u,"	/* material	*/
			     "  %lu,"	/* flags	*/
			     "  %lu,"	/* uncomf	*/
			     "  %lu,"	/* unusable	*/
			     "  %d,"	/* wear		*/
			     "  %lld,"	/* value0	*/
			     "  %lld,"	/* value1	*/
			     "  %lld,"	/* value2	*/
			     "  %lld,"	/* value3	*/
			     "  %lld,"	/* value4	*/
			     "  %d,"	/* req_str	*/
			     "  %d,"	/* req_dex	*/
			     "  %d,"	/* req_con	*/
			     "  %d,"	/* req_int	*/
			     "  %d)",	/* req_wis	*/
			obj->vnum,
			name,
			sh_desc,
			l_desc,
			obj->item_type,
			obj->condition,
			obj->weight,
			obj->level,
			obj->cost,
			material_lookup(obj->material),
			obj->extra_flags,
			obj->uncomf,
			obj->unusable,
			obj->wear_flags,
			(long long int) obj->value[0],
			(long long int) obj->value[1],
			(long long int) obj->value[2],
			(long long int) obj->value[3],
			(long long int) obj->value[4],
			obj->require[STAT_STR],
			obj->require[STAT_DEX],
			obj->require[STAT_CON],
			obj->require[STAT_INT],
			obj->require[STAT_WIS]);

	    for (aff = obj->affected; aff; aff = aff->next)
	    {
		MYSQL_QUERY(con, "INSERT INTO affects "
				  "VALUES('', %d, %d, %d, %lu, %d)",
			     aff->where, aff->location, aff->modifier,
			     aff->bitvector, obj->vnum);
	    }
	}
}

void db_tables()
{
    char buf[5 * MAX_STRING_LENGTH];
    int i, j;

    con = mysql_init(NULL);
    if (!mysql_real_connect(con, cfg.db_host, cfg.db_user, cfg.db_passwd,
     			    cfg.db_name, cfg.db_port, NULL, 0))
    {
	bugf("[MySQL] Can't connect to database '%s' on %s:%d as '%s' "
	     "identified by '%s': %s",
	     cfg.db_name, cfg.db_host, cfg.db_port, cfg.db_user, cfg.db_passwd,
	     mysql_error(con));
	return;
    }

    MYSQL_QUERY(con, "DELETE FROM tables_list");

    for (i = 0; flag_stat_table[i].name; i++)
	if (!IS_NULLSTR(flag_stat_table[i].name))
	    for (j = 0; flag_stat_table[i].structure[j].name; j++)
		if (flag_stat_table[i].structure[j].bit != 0
		    || flag_stat_table[i].stat)
		{
		    MYSQL_QUERY(con, "INSERT INTO tables_list "
				     "VALUES('%s', '%s', '%s', %lu)",
				flag_stat_table[i].name,
				flag_stat_table[i].structure[j].name,
				flag_stat_table[i].structure[j].rname,
				flag_stat_table[i].structure[j].bit);
		}

    mysql_close(con);
}
#else /* NO_MYSQL */

void dbdump(AREA_DATA *area) {
}

void db_tables() {
}

#endif
    
#endif

/* charset=cp1251 */
