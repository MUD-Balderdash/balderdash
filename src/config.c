#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "config.h"



#undef KEY
#undef SKEY
#undef BKEY

#define KEY(literal, field, value)              \
                if (!str_cmp(word, literal))    \
                {                               \
                    field  = value;             \
                    fMatch = TRUE;              \
                    break;                      \
                }

#define SKEY(string, field)                     \
                if (!str_cmp(word, string))     \
                {                               \
                    free_string(field);         \
                    field = fread_string(fp);   \
                    fMatch = TRUE;              \
                    break;                      \
                }

#define BKEY(string, field)                                             \
                if (!str_cmp(word, string))                             \
                {                                                       \
                    char *sval = fread_word(fp);                        \
                    fMatch = TRUE;                                      \
                    if (!str_cmp(sval, "true"))                         \
                        field = TRUE;                                   \
                    else if (!str_cmp(sval, "false"))                   \
                        field = FALSE;                                  \
                }


struct mud_config cfg;

void read_config(const char *filename)
{
    char *word;
    char *fname;
    bool fMatch;
    FILE *fp;

    /* Default values */
    cfg.port        = 9000;
    cfg.antitrigger = FALSE;
    cfg.use_db      = FALSE;
    cfg.newlock     = FALSE;
    cfg.wizlock     = FALSE;
    cfg.log_all     = FALSE;
    cfg.db_port     = 3306;
    cfg.db_host     = str_dup("localhost");
    cfg.db_name     = str_dup("mud");
    cfg.db_user     = str_dup("mud");
    cfg.db_passwd   = str_dup("balderdash");

    if (IS_NULLSTR(filename))
	fname = str_dup(CONFIG_FILE);
    else
	fname = str_dup(filename);
	
    if ((fp = fopen(fname, "r")) == NULL)
    {
	bugf("Can not open config file '%s': %s", fname, strerror(errno));
	goto EX_FREE;
    }

    for (;;)
    {
	fMatch = FALSE;
	word = feof(fp) ? "End" : fread_word(fp);

	switch (UPPER(word[0]))
	{
	case '*':
	    fMatch = TRUE;
	    fread_to_eol(fp);
	    break;

	case 'A':
	    BKEY("AntiTrigger", cfg.antitrigger);
	    break;

	case 'D':
	    KEY("DbPort", cfg.db_port, fread_number(fp));
	    SKEY("DbName", cfg.db_name);
	    SKEY("DbPasswd", cfg.db_passwd);
	    SKEY("DbUser", cfg.db_user);
	    SKEY("DbHost", cfg.db_host);
	    break;

	case 'E':
	    if (!str_cmp(word, "End"))
		goto EX_FREE;

	    break;

	case 'L':
	    BKEY("LogAll", cfg.log_all);
	    break;

	case 'N':
	    BKEY("NewLock", cfg.newlock);
	    break;

	case 'P':
	    KEY("Port", cfg.port, fread_number(fp));
	    break;

	case 'W':
	    BKEY("WizLock", cfg.wizlock);
	    break;

	case 'U':
	    BKEY("UseDB", cfg.use_db);
	    break;
	}

	if (!fMatch)
	    bugf("read_config(): no match '%s'.", word);
    }

EX_FREE:
    free_string(fname);
}

/* charset=cp1251 */
