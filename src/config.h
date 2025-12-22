/* $Id: config.h,v 1.3 2005/02/13 14:53:19 kemm Exp $ */
#ifndef CONFIG_H
#define CONFIG_H

#include <sys/types.h>
#include "merc.h"
#include "stdint.h"

#define CONFIG_FILE	"mud.cfg"

struct mud_config
{
    uint16_t port;
    bool newlock;
    bool wizlock;
    bool log_all;
    bool use_db;
    bool antitrigger;

    uint16_t db_port;
    char *db_host;
    char *db_user;
    char *db_passwd;
    char *db_name;
};

extern struct mud_config cfg;

void read_config(const char *filename);

#endif /* CONFIG_H */

/* charset=cp1251 */
