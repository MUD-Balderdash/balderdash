/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *   order to use any part of this Merc Diku Mud, you must comply with     *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Thanks to abaddon for proof-reading our comm.c and pointing out bugs.  *
 *  Any remaining bugs are, of course, our work, not his.  :)              *
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

/*
 * This file contains all of the OS-dependent stuff:
 *   startup, signals, BSD sockets for tcp/ip, i/o, timing.
 *
 * The data flow for input is:
 *    Game_loop ---> Read_from_descriptor ---> Read
 *    Game_loop ---> Read_from_buffer
 *
 * The data flow for output is:
 *    Game_loop ---> Process_Output ---> Write_to_descriptor -> Write
 *
 * The OS-dependent functions are Read_from_descriptor and Write_to_descriptor.
 * -- Furey  26 Jan 1993
 */

#include <sys/types.h>
#include <sys/time.h>


#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <locale.h>
#include <time.h>

/* This removes annoying warnings (FreeBSD) */
#include <unistd.h>


#include <netinet/in.h>
#include <arpa/inet.h>



extern int reboot_time;

/*
 * Signal handling.
 * Apollo has a problem with __attribute(atomic) in signal.h,
 *   I dance around it.
 */
#if defined(apollo)
#define __attribute(x)
#endif

#include <signal.h>

#if defined(apollo)
#undef __attribute
#endif

extern char *help_greeting;


/*
 * Socket and TCP/IP stuff.
 */

#include <fcntl.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "telnet.h"
unsigned char			echo_off_str		[] = { IAC, WILL, TELOPT_ECHO, '\0' };
unsigned char			echo_on_str			[] = { IAC, WONT, TELOPT_ECHO, '\0' };
unsigned char			echo_do_str			[] = { IAC, DO_ , TELOPT_ECHO, '\0' };
unsigned char			echo_dont_str		[] = { IAC, DONT, TELOPT_ECHO, '\0' };

const unsigned char 	go_ahead_str		[] = { IAC, GA, '\0' };
const unsigned char		telopt_binary		[] = { IAC, DO_,  TELOPT_BINARY, '\0' };
const unsigned char     eor_on_str      	[] = { IAC, WILL, TELOPT_EOR, '\0' };

/* mccp: compression negotiation strings */
const unsigned char    	compress_will 		[] = { IAC, WILL, TELOPT_COMPRESS,  '\0'};
const unsigned char    	compress_do   		[] = { IAC, DO_,  TELOPT_COMPRESS,  '\0'};
const unsigned char    	compress_dont 		[] = { IAC, DONT, TELOPT_COMPRESS,  '\0'};
const unsigned char    	compress_will2		[] = { IAC, WILL, TELOPT_COMPRESS2, '\0'};
const unsigned char    	compress_do2  		[] = { IAC, DO_,  TELOPT_COMPRESS2, '\0'};
const unsigned char    	compress_dont2		[] = { IAC, DONT, TELOPT_COMPRESS2, '\0'};
const unsigned char    	compress_wont 		[] = { IAC, WONT, TELOPT_COMPRESS,  '\0'};

const unsigned char    	compress_wont2[] = { IAC, WONT, TELOPT_COMPRESS2, '\0'};


/* MXP support */
const unsigned char 	will_mxp_str [] = { IAC, WILL, TELOPT_MXP, '\0' };
const unsigned char 	start_mxp_str[] = { IAC, SB, TELOPT_MXP, IAC, SE, '\0'};
const unsigned char 	do_mxp_str   [] = { IAC, DO_, TELOPT_MXP, '\0' };
const unsigned char 	dont_mxp_str [] = { IAC, DONT, TELOPT_MXP, '\0' };
const unsigned char 	wont_mxp_str [] = { IAC, WONT, TELOPT_MXP, '\0' };

#define IN_COMM_C

/* Our headers was moved here due to conflicts in linux signal.h */
#include "merc.h"

#include "interp.h"
#include "recycle.h"
#include "olc.h"
#include "config.h"




void install_other_handlers args((void));

/*
 * Global variables.
 */
/* All open descriptors		*/
SLIST_HEAD(, descriptor_data) descriptor_list = SLIST_HEAD_INITIALIZER(descriptor_list);

FILE *		    fpReserve;		/* Reserved file handle		*/
bool		    merc_down;		/* Shutdown			*/
time_t		    boot_time;
time_t		    current_time;	/* time of this pulse */
bool		    MOBtrigger = TRUE;  /* act() switch                 */




/*
 * OS-dependent local functions.
 */
bool	read_from_descriptor	args((DESCRIPTOR_DATA *d));
/* bool	write_to_descriptor	args((DESCRIPTOR_DATA *d, char *txt, int length)); */

bool	write_to_descriptor	args((DESCRIPTOR_DATA *d, char *txt, int length));

void	game_loop_unix		args((int control));
bool	write_to_descriptor_2	args((int d, char *txt, int length));
void	init_descriptor		args((int control));
int	init_socket		args((int port));





extern          char                    last_command[MAX_STRING_LENGTH];
void init_signals   args((void));
sig_t  sig_bus, sig_term, sig_abrt, sig_segv;
static int sig_counter = 0;



char 	*direct(unsigned char dir, bool go);
void	generate_character_password(CHAR_DATA *ch);
void 	do_email_pass(CHAR_DATA *ch, char *argument);


/********** Constants for recode: *********************************************/
typedef unsigned char uchar;

/* Alternative -> Windows (CP1251) */
uchar aw[] =
{
    192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
    208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
    224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151,
    152, 153, 154, 129, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167,
    168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
    130, 132, 135, 134, 128, 133, 131, 155, 184, 185, 186, 187, 188, 189, 190, 191
};

/* KOI8 -> Windows (CP1251) */
uchar kw[] =
{
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
    144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
    160, 161, 162, 184, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
    176, 177, 178, 168, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
    254, 224, 225, 246, 228, 229, 244, 227, 245, 232, 233, 234, 235, 236, 237, 238,
    239, 255, 240, 241, 242, 243, 230, 226, 252, 251, 231, 248, 253, 249, 247, 250,
    222, 192, 193, 214, 196, 197, 212, 195, 213, 200, 201, 202, 203, 204, 205, 206,
    207, 223, 208, 209, 210, 211, 198, 194, 220, 219, 199, 216, 221, 217, 215, 218
};

/* Windows (CP1251) -> Alternative */
uchar wa[] =
{
    244, 195, 240, 246, 241, 245, 243, 242, 176, 177, 178, 179, 180, 181, 182, 183,
    184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 247, 196, 197, 198, 199,
    200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215,
    216, 217, 218, 219, 220, 221, 222, 223, 248, 249, 250, 251, 252, 253, 254, 255,
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
    144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
    224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239
};


/* Windows (CP1251) -> KOI8 */
uchar wk[] =
{
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
    144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
    160, 161, 162, 163, 164, 165, 166, 167, 179, 169, 170, 171, 172, 173, 174, 175,
    176, 177, 178, 179, 180, 181, 182, 183, 163, 185, 186, 187, 188, 189, 190, 191,
    225, 226, 247, 231, 228, 229, 246, 250, 233, 234, 235, 236, 237, 238, 239, 240,
    242, 243, 244, 245, 230, 232, 227, 254, 251, 253, 255, 249, 248, 252, 224, 241,
    193, 194, 215, 199, 196, 197, 214, 218, 201, 202, 203, 204, 205, 206, 207, 208,
    210, 211, 212, 213, 198, 200, 195, 222, 219, 221, 223, 217, 216, 220, 192, 209
};

/* MAC -> Windows (CP1251) */
uchar mw[] =
{
    192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
    208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
    134, 176, 63, 63, 167, 149, 182, 178, 174, 169, 153, 128, 144, 63, 129, 131,
    63, 177, 63, 63, 179, 181, 63, 163, 170, 186, 175, 191, 138, 154, 140, 156,
    188, 189, 172, 118, 63, 63, 63, 171, 187, 133, 160, 142, 158, 141, 157, 190,
    150, 151, 147, 148, 145, 146, 63, 132, 161, 162, 143, 159, 185, 168, 184, 255,
    224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 164
};

/* Windows (CP1251) -> MAC */
uchar wm[] =
{
    171, 174, 63, 175, 215, 201, 160, 63, 63, 63, 188, 63, 190, 205, 203, 218,
    172, 212, 213, 210, 211, 165, 208, 209, 63, 170, 189, 63, 191, 206, 204, 219,
    202, 216, 217, 183, 255, 63, 63, 164, 221, 169, 184, 199, 194, 63, 168, 186,
    161, 177, 167, 180, 63, 181, 166, 63, 222, 220, 185, 200, 192, 193, 207, 187,
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
    144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
    224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 223
};


struct translit_type
{
    char	rus;
    char	eng[3];
};

struct translit_type		translit	[]	= {
    {'Ч', "CH"},
    {'ч', "ch"},
    {'Ш', "SH" },
    {'ш', "sh" },
    {'Щ', "SH"},
    {'щ', "sh"},
    {'Й', "YJ" },
    {'й', "yj" },
    {'Ю', "YU"},
    {'ю', "yu"},
    {'Я', "YA"},
    {'я', "ya"},
    {'А', "A"} ,
    {'а', "a"} ,
    {'Б', "B"} ,
    {'б', "b"} ,
    {'В', "V"} ,
    {'в', "v"} ,
    {'Г', "G"} ,
    {'г', "g"} ,
    {'Д', "D"} ,
    {'д', "d"} ,
    {'Е', "E"} ,
    {'е', "e"} ,
    {'Ё', "E"} ,
    {'ё', "e"} ,
    {'Ж', "J"} ,
    {'ж', "j"} ,
    {'З', "Z"} ,
    {'з', "z"} ,
    {'И', "I"} ,
    {'и', "i"} ,
    {'К', "K"} ,
    {'к', "k"} ,
    {'Л', "L"} ,
    {'л', "l"} ,
    {'М', "M"} ,
    {'м', "m"} ,
    {'Н', "N"} ,
    {'н', "n"} ,
    {'О', "O"} ,
    {'о', "o"} ,
    {'П', "P"} ,
    {'п', "p"} ,
    {'Р', "R"} ,
    {'р', "r"} ,
    {'С', "S"} ,
    {'с', "s"} ,
    {'Т', "T"} ,
    {'т', "t"} ,
    {'У', "U"} ,
    {'у', "u"} ,
    {'Ф', "F"} ,
    {'ф', "f"} ,
    {'Х', "H"} ,
    {'х', "h"} ,
    {'Ь', "'"} ,
    {'ь', "'"} ,
    {'Ъ', "'"} ,
    {'ъ', "'"} ,
    {'Ц', "C"},
    {'ц', "c"},
    {'Ы', "Y"},
    {'ы', "y"},
    {'Э', "E"},
    {'э', "e"},
    {'\0', ""}
};


struct antitrigger_type
{
    char	rus;
    char	eng;
};

struct antitrigger_type		antitrigger	[]	= {
    {'а', 'a'},
    {'е', 'e'},
    {'о', 'o'},
    {'р', 'p'},
    {'с', 'c'},
    {'х', 'x'},
    {'А', 'A'},
    {'В', 'B'},
    {'Е', 'E'},
    {'М', 'M'},
    {'Н', 'H'},
    {'О', 'O'},
    {'Р', 'P'},
    {'С', 'C'},
    {'Т', 'T'},
    {'Х', 'X'},
    {'\0','\0'}
};


//void init_aspell();
/*
 * Other local functions (OS-independent).
 */
bool	check_parse_name	args((char *name));

bool	check_reconnect		args((DESCRIPTOR_DATA *d, char *name,
				      bool fConn));
bool	check_playing		args((DESCRIPTOR_DATA *d, char *name));
int	main			args((int argc, char *argv[]));
void	nanny			args((DESCRIPTOR_DATA *d, char *argument));
bool	process_output		args((DESCRIPTOR_DATA *d, bool fPrompt));
void	read_from_buffer	args((DESCRIPTOR_DATA *d));
void	stop_idling		args((CHAR_DATA *ch));
void    bust_a_prompt           args((CHAR_DATA *ch));


/* set up MXP */
void turn_on_mxp (DESCRIPTOR_DATA *d)
{
    d->mxp = TRUE;  /* turn it on now */

    /*  write(d->descriptor, start_mxp_str, strlen(start_mxp_str)); */

    write_to_buffer(d, MXPMODE (6), 0);   /* permanent secure mode */
    write_to_buffer(d, MXPTAG ("!-- Set up MXP elements --"), 0);
    /* Exit tag */
    write_to_buffer(d, MXPTAG ("!ELEMENT Ex '<send>' FLAG=RoomExit"), 0);
    /* Room description tag */
    write_to_buffer(d, MXPTAG ("!ELEMENT rdesc '<p>' FLAG=RoomDesc"), 0);
    /* Get an item tag (for things on the ground) */
    write_to_buffer(d, MXPTAG
		    ("!ELEMENT Get \"<send href='"
		     "взять &#39;&name;&#39;|"
		     "проверить &#39;&name;&#39;|"
		     "пить &#39;&name;&#39;"
		     "' "
		     "hint='Для использования этого объекта нажмите правую кнопку мыши|"
		     "Взять &desc;|"
		     "Проверить &desc;|"
		     "Пить из &desc;"
		     "'>\" ATT='name desc'"),
		    0);
    /* Drop an item tag (for things in the inventory) */
    write_to_buffer(d, MXPTAG
		    ("!ELEMENT Drop \"<send href='"
		     "бросить &#39;&name;&#39;|"
		     "проверить &#39;&name;&#39;|"
		     "смотреть в &#39;&name;&#39;|"
		     "одеть &#39;&name;&#39;|"
		     "есть &#39;&name;&#39;|"
		     "пить &#39;&name;&#39;"
		     "' "
		     "hint='Для использования этого объекта нажмите правую кнопку мыши|"
		     "Бросить &desc;|"
		     "Проверить &desc;|"
		     "Смотреть в &desc;|"
		     "Одеть &desc;|"
		     "Есть &desc;|"
		     "Пить &desc;"
		     "'>\" ATT='name desc'"),
		    0);
    /* List an item tag (for things in a shop) */
    write_to_buffer(d, MXPTAG
		    ("!ELEMENT List \"<send href='купить &#39;&name;&#39;' "
		     "hint='Купить &desc;'>\" "
		     "ATT='name desc'"),
		    0);
    /* Player tag (for who lists, tells etc.) */
    /*  write_to_buffer(d, MXPTAG
     ("!ELEMENT Player \"<send href='гов &#39;&name;&#39; ' "
     "hint='Послать сообщение &name;' prompt>\" "
     "ATT='name'"),
     0); */
} /* end of turn_on_mxp */


char *dns_gethostname(char *s)
{
    return gethostname_cached(s, 4, FALSE);
}

void usage(const char *name)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s <options>\n\n", name);
    fprintf(stderr, "    -p <portnum>   : start MUD on specified port\n");
    fprintf(stderr, "    -n             : start MUD in 'newlock' mode\n");
    fprintf(stderr, "    -w             : start MUD in 'wizlock' node\n");
    fprintf(stderr, "    -l             : log all commands from players\n");
    fprintf(stderr, "    -u             : do not use DB\n");
    fprintf(stderr, "    -P <db_port>   : MySQL port (default 3306)\n");
    fprintf(stderr, "    -N <db_name>   : MySQL database (default 'mud')\n");
    fprintf(stderr, "    -U <db_user>   : MySQL user (default 'mud')\n");
    fprintf(stderr, "    -S <db_passwd> : MySQL password\n");
    fprintf(stderr, "    -H <db_host>   : MySQL host\n");
    fprintf(stderr, "    -f <cfg_file>  : config file (default '%s')\n",
	    CONFIG_FILE);
    fprintf(stderr, "NOTE: command line arguments are replace options defined\n"
	    "      in config file\n");
}

void init_str(); /* db.c */
//void init_lua(); /* lua/lua.c */

int main(int argc, char *argv[])
{
    struct timeval now_time;

    int control;

    char buf[MAX_INPUT_LENGTH];
    struct mud_config conf;
    int ch;
    char *conf_file = NULL;

#if !defined(ONEUSER)
    FILE	*fp = NULL;
    extern 	int max_ever;
#endif



    setlocale(LC_ALL, "ru");
    /*
     * Init time.
     */
    gettimeofday(&now_time, NULL);
    current_time 	= (time_t) now_time.tv_sec;
    boot_time		= current_time;

    /*
     * Reserve one channel for our use.
     */
    if ((fpReserve = fopen(NULL_FILE, "r")) == NULL)
    {
	_perror(NULL_FILE);
	exit(1);
    }

    /* Set top_string */
    init_str();

    /*
     * Parse command line arguments and config file.
     */
    conf.port = 0;
    conf.newlock = FALSE;
    conf.wizlock = FALSE;
    conf.log_all = FALSE;
    conf.use_db  = TRUE;
    conf.db_port = 0;
    conf.db_host = NULL;
    conf.db_name = NULL;
    conf.db_user = NULL;
    conf.db_passwd = NULL;

    while ((ch = getopt(argc, argv, "p:nwlf:P:H:N:U:S:")) != -1)
    {
	switch (ch)
	{
	case 'p':
	    if (!is_number(optarg))
	    {
		usage(argv[0]);
		exit(2);
	    }
	    else
		conf.port = atoi(optarg);
	    break;

	case 'n':
	    conf.newlock = TRUE;
	    break;

	case 'w':
	    conf.wizlock = TRUE;
	    break;

	case 'l':
	    conf.log_all = TRUE;
	    break;

	case 'f':
	    conf_file = str_dup(optarg);
	    break;

	case 'u':
	    conf.use_db = FALSE;
	    break;

	case 'P':
	    if (!is_number(optarg))
	    {
		usage(argv[0]);
		exit(2);
	    }
	    else
		conf.db_port = atoi(optarg);
	    break;

	case 'H':
	    conf.db_host = str_dup(optarg);
	    break;

	case 'N':
	    conf.db_name = str_dup(optarg);
	    break;

	case 'U':
	    conf.db_user = str_dup(optarg);
	    break;

	case 'S':
	    conf.db_passwd = str_dup(optarg);
	    break;

	case 'h':
	case '?':
	default:
	    usage(argv[0]);
	    exit(2);
	}
    }

    argc -= optind;
    argv += optind;

    read_config(conf_file);
    free_string(conf_file);

    if (conf.port)
	cfg.port = conf.port;

    if (conf.newlock)
	cfg.newlock = conf.newlock;

    if (conf.wizlock)
	cfg.wizlock = conf.wizlock;

    if (conf.log_all)
	cfg.log_all = conf.log_all;

    if (!conf.use_db)
	cfg.use_db = conf.use_db;

    if (conf.db_port)
	cfg.db_port = conf.db_port;

    if (conf.db_host)
    {
	free_string(cfg.db_host);
	cfg.db_host = conf.db_host;
    }

    if (conf.db_name)
    {
	free_string(cfg.db_name);
	cfg.db_name = conf.db_name;
    }

    if (conf.db_user)
    {
	free_string(cfg.db_user);
	cfg.db_user = conf.db_user;
    }

    if (conf.db_passwd)
    {
	free_string(cfg.db_passwd);
	cfg.db_passwd = conf.db_passwd;
    }



    /* Signal handler initialization */
    init_signals();

    /* Write PID to file */
    sprintf(buf, "rom.%d", cfg.port);

    if ((fp = fopen(buf, "w")) == NULL)
	bugf("Unable to open PID file.");
    else
    {
	fprintf(fp, "%d", getpid());
	fclose(fp);
    }

    //init_aspell();
#ifndef linux
    rdns_cache_set_ttl(12*60*60);       // twelve hours is moooore than enough
#endif /* linux */


#if !defined(ONEUSER)
    if ((fp = fopen(MAX_EVER_FILE, "r")) == NULL)
	bugf("Unable to open MAX file.");
    else
    {
	max_ever = fread_number(fp);
	fclose(fp);
    }
#endif /* ONEUSER */

    /*
     * Run the game.
     */

//    init_lua();



    control = init_socket(cfg.port);

    boot_db();

    auctions[0].valid = auctions[1].valid = auctions[2].valid = FALSE;
    sprintf(buf, "Balderdash is ready to rock on port %d at %s.", cfg.port, ctime(&boot_time));
    log_string(buf);

    game_loop_unix(control);

    close(control);


    /*
     * That's all, folks.
     */
    log_string("Normal termination of game.");
    exit(0);
    return 0;
}



int init_socket(int port)
{
    static struct sockaddr_in sa_zero;
    struct sockaddr_in sa;

    int x = 1;
    int fd;



    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
	_perror("Init_socket: socket");
	exit(1);
    }


    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
		   (char *) &x, sizeof(x)) < 0)
    {
	_perror("Init_socket: SO_REUSEADDR");
	close(fd);
	exit(1);
    }


#if defined(SO_DONTLINGER) && !defined(SYSV)
    {
	struct	linger	ld;

	ld.l_onoff  = 1;
	ld.l_linger = 1000;

	if (setsockopt(fd, SOL_SOCKET, SO_DONTLINGER,
		       (char *) &ld, sizeof(ld)) < 0)
	{
	    _perror("Init_socket: SO_DONTLINGER");

	    close(fd);

	    exit(1);
	}
    }
#endif

    sa		    = sa_zero;
    sa.sin_family   = AF_INET;
    sa.sin_port	    = htons(port);

    if (bind(fd, (struct sockaddr *) &sa, sizeof(sa)) < 0)
    {
	_perror("Init socket: bind");

	close(fd);
	exit(1);
    }


    if (listen(fd, 3) < 0)
    {
	_perror("Init socket: listen");

	close(fd);

	exit(1);
    }

    return fd;
}


void game_loop_unix(int control){
    static struct timeval null_time;
    struct timeval last_time;

    signal(SIGPIPE, SIG_IGN);

    gettimeofday(&last_time, NULL);
    current_time = (time_t) last_time.tv_sec;

    /* Main loop */

    while (!merc_down){
	fd_set in_set;
	fd_set out_set;
	fd_set exc_set;
	DESCRIPTOR_DATA *d, *d_next;
	int maxdesc;

	int cp_state;
	char buf[MSL];


	/*
	 * Poll all active descriptors.
	 */
	FD_ZERO(&in_set );
	FD_ZERO(&out_set);
	FD_ZERO(&exc_set);
	FD_SET(control, &in_set);
	maxdesc	= control;
	SLIST_FOREACH(d, &descriptor_list, link)
	{
	    maxdesc = UMAX(maxdesc, d->descriptor);
	    FD_SET(d->descriptor, &in_set );
	    FD_SET(d->descriptor, &out_set);
	    FD_SET(d->descriptor, &exc_set);
	}

	if (select(maxdesc+1, &in_set, &out_set, &exc_set, &null_time) < 0)
	{
	    _perror("Game_loop: select: poll");
	    exit(1);
	}

	/*
	 * New connection?
	 */


	if (FD_ISSET(control, &in_set))
	    init_descriptor(control);

	/*
	 * Kick out the freaky folks.
	 */
	SLIST_FOREACH_SAFE(d, &descriptor_list, link, d_next)
	{
	    if (FD_ISSET(d->descriptor, &exc_set))
	    {
		FD_CLR(d->descriptor, &in_set );
		FD_CLR(d->descriptor, &out_set);
		if (d->character && d->connected == CON_PLAYING)
		{
		    CHAR_DATA *ch = CH(d);

		    VALIDATE(ch);
		    save_char_obj(ch, FALSE);
		}
		d->outtop	= 0;
		close_socket(d);
		continue;
	    }
	    /*  Kick out idling players */

	    ++d->idle_tics;

	    if (((d->connected == CON_GET_OLD_PASSWORD || d->connected == CON_GET_NAME || d->connected == CON_GET_CODEPAGE || d->connected == CON_GET_NUM_REGANSWER) && d->idle_tics > 300)
		|| (d->connected != CON_PLAYING && d->idle_tics > 5000))
	    {
		write_to_descriptor(d, "\r\nTimed out... goodbye.\r\n", 0);
		close_socket(d);
		continue;
	    }


	    /*  Kick out players from anonymous proxy*/
	    if ((cp_state = cp_get_state(inet_addr(d->ip))))
	    {
		if (d->character && d->connected == CON_PLAYING)
		{
		    CHAR_DATA *ch = CH(d);

		    VALIDATE(ch);
		    save_char_obj(ch, FALSE);
		}

		switch (cp_state)
		{
		case CP_IS_SOCKS4:
		    write_to_descriptor(d, "Anyonymous SOCKS4 proxy is not allowed!\n\r", 0);
		    break;
		case CP_IS_SOCKS5:
		    write_to_descriptor(d, "Anyonymous SOCKS5 proxy is not allowed!\n\r", 0);
		    break;
		case CP_IS_HTTP:
		    write_to_descriptor(d, "Anyonymous HTTP proxy is not allowed!\n\r", 0);
		    break;
		case CP_IS_WHOIS:
		    sprintf(buf, "Your country (%s) is not allowed!\n\r", cp_get_country(inet_addr(d->ip)));
		    write_to_descriptor(d, buf, 0);
		    break;
		default:
		    break;
		}     

		d->outtop	= 0;
		close_socket(d);
	    } 

	}

	/*
	 * Process input.
	 */

#if !defined(NOLIMIT) && defined(ONEUSER)
	count_desc = 0;
#endif
	SLIST_FOREACH_SAFE(d, &descriptor_list, link, d_next)
	{
	    d->fcommand	= FALSE;

#if !defined(NOLIMIT) && defined(ONEUSER)
	    if (++count_desc > 3)
		break;
#endif

	    if (FD_ISSET(d->descriptor, &in_set))
	    {
		if (d->character != NULL)
		    d->character->timer = 0;

		d->idle_tics = 0;

		if (!read_from_descriptor(d))
		{
		    FD_CLR(d->descriptor, &out_set);
		    if (d->character != NULL && d->connected == CON_PLAYING)
		    {
			CHAR_DATA *ch = CH(d);

			VALIDATE(ch);
			save_char_obj(ch, FALSE);
		    }
		    d->outtop	= 0;
		    close_socket(d);
		    continue;
		}
	    }

	    if (d->character != NULL && d->character->daze > 0)
		--d->character->daze;

	    if (d->character != NULL && d->character->wait > 0)
	    {
		--d->character->wait;
		continue;
	    }

	    read_from_buffer(d);

	    if (d->incomm[0] != '\0')
	    {
		d->fcommand	= TRUE;
		stop_idling(d->character);
		/* OLC */
		if (d->showstr_point)
		    show_string(d, d->incomm);
		else
		    if (d->pString)
			string_add(d->character, d->incomm);
		    else
			switch (d->connected)
			{
			case CON_PLAYING:
			    if (!run_olc_editor(d))
				substitute_alias(d, d->incomm);
			    break;
			default:
			    nanny(d, d->incomm);
			    break;
			}

		d->incomm[0]	= '\0';
	    }
	}



	/*
	 * Autonomous game motion.
	 */
	update_handler();

	/*
	 * Output.
	 */
	SLIST_FOREACH_SAFE(d, &descriptor_list, link, d_next)
	{
	    if ((d->fcommand || d->outtop > 0 || d->out_compress)
		&& FD_ISSET(d->descriptor, &out_set))
	    {
		bool ok = TRUE;

		if (d->fcommand || d->outtop > 0)
		    ok = process_output(d, TRUE);

		if (ok && d->out_compress)
		    ok = processCompressed(d);

		if (!ok)
		{
		    if (d->character != NULL && d->connected == CON_PLAYING)
		    {
			CHAR_DATA *ch = CH(d);

			VALIDATE(ch);
			save_char_obj(ch, FALSE);
		    }
		    d->outtop	= 0;
		    close_socket(d);
		}
	    }
	}



	/*
	 * Synchronize to a clock.
	 * Sleep(last_time + 1/PULSE_PER_SECOND - now).
	 * Careful here of signed versus unsigned arithmetic.
	 */
	{
	    struct timeval now_time;
	    long secDelta;
	    long usecDelta;

	    gettimeofday(&now_time, NULL);
	    usecDelta	= ((int) last_time.tv_usec) - ((int) now_time.tv_usec)
		+ 1000000 / PULSE_PER_SECOND;
	    secDelta	= ((int) last_time.tv_sec) - ((int) now_time.tv_sec);
	    while (usecDelta < 0)
	    {
		usecDelta += 1000000;
		secDelta  -= 1;
	    }

	    while (usecDelta >= 1000000)
	    {
		usecDelta -= 1000000;
		secDelta  += 1;
	    }

	    if (secDelta > 0 || (secDelta == 0 && usecDelta > 0))
	    {

		struct timeval stall_time;

		stall_time.tv_usec = usecDelta;
		stall_time.tv_sec  = secDelta;
		if (select(0, NULL, NULL, NULL, &stall_time) < 0)
		{
		    _perror("Game_loop: select: stall");
		    exit(1);
		}

	    }
	}

	gettimeofday(&last_time, NULL);
	current_time = (time_t) last_time.tv_sec;
    }

    return;
}



void init_descriptor(int control)

{
    DESCRIPTOR_DATA *dnew;
    struct sockaddr_in sock;
    /* #if defined(ONEUSER)
     struct hostent *from;
#endif  */
    int size;

    int desc;

    size = sizeof(sock);
    getsockname(control, (struct sockaddr *) &sock, &size);
    if ((desc = accept(control, (struct sockaddr *) &sock, &size)) < 0)
    {
	_perror("New_descriptor: accept");
	return;
    }


#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif


	if (fcntl(desc, F_SETFL, FNDELAY) == -1)

	{
	    _perror("New_descriptor: fcntl: FNDELAY");
	    return;
	}

    /*
     * Cons a new descriptor.
     */
    dnew = new_descriptor();

    dnew->descriptor	= desc;
    dnew->connected	= CON_GET_CODEPAGE;
    dnew->showstr_head	= NULL;
    dnew->showstr_point = NULL;
    /*    dnew->outsize	= 2000;*/ ;
    dnew->pEdit		= NULL;			/* OLC */
    dnew->pString	= NULL;			/* OLC */
    dnew->editor	= 0;			/* OLC */
    dnew->outbuf[0] 	= '\0';
    /*    dnew->outbuf	= alloc_mem(dnew->outsize); */

    dnew->idle_tics	= 0;
    dnew->codepage	= 0;
    dnew->mxp = FALSE;   /* NJG - initially MXP is off */
    dnew->mccp_version	= 0;
    dnew->grep[0]	= '\0';

    size = sizeof(sock);
    if (getpeername(desc, (struct sockaddr *) &sock, &size) < 0)
    {
	_perror("New_descriptor: getpeername");

	bzero(dnew->Host, sizeof(dnew->Host));

	dnew->ip   = str_dup("(unknown)");
	dnew->sock = NULL;
    }
    else
    {
	/*
	 * Would be nice to use inet_ntoa here but it takes a struct arg,
	 * which ain't very compatible between gcc and system libraries.
	 */
	unsigned long addr;
	char bfr[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
//	char new_buf[MAX_STRING_LENGTH];

	addr = ntohl(sock.sin_addr.s_addr);

	sprintf(bfr, "%lu.%lu.%lu.%lu",
		(addr >> 24) & 0xFF, (addr >> 16) & 0xFF,
		(addr >>  8) & 0xFF, (addr      ) & 0xFF
	       );
	sprintf(buf, "Sock.sinaddr:  %s", bfr);
	log_string(buf);
	
#if !defined(NOTHREAD)
	memcpy(dnew->Host, &sock.sin_addr, sizeof(addr));
	gethostname_cached(dnew->Host, 4, TRUE);
	check_proxy(sock.sin_addr.s_addr);
#else

	/*	from = gethostbyaddr((char *) &sock.sin_addr,
	 sizeof(sock.sin_addr), AF_INET);  */
	/*	dnew->Host = str_dup(from ? from->h_name : buf);   */
	dnew->Host = str_dup(bfr);
#endif
	dnew->ip = str_dup(bfr);
	dnew->sock = str_dup ((char *) &sock.sin_addr);
	/*      	dnew->socksize = sizeof(sock.sin_addr); */
    }

    /*
     * Swiftest: I added the following to ban sites.  I don't
     * endorse banning of sites, but Copper has few descriptors now
     * and some people from certain sites keep abusing access by
     * using automated 'autodialers' and leaving connections hanging.
     *
     * Furey: added suffix check by request of Nickel of HiddenWorlds.
     */
    if (check_ban(dnew->ip, BAN_ALL))
    {
	write_to_descriptor_2(desc, "Your site has been banned from this mud.\n\r", 0);

	close(desc);

	free_descriptor(dnew);
	return;
    }
    /*
     * Init descriptor data.
     */
    SLIST_INSERT_HEAD(&descriptor_list, dnew, link);

    /* Binary Connection */

    send(desc, telopt_binary, 3, 0);

    write_to_buffer(dnew, will_mxp_str,  -1);
    write_to_buffer(dnew, compress_will2, -1);
    write_to_buffer(dnew, compress_will, -1);

    /*
     * Send the greeting.
     */

    if (help_greeting)
    {
	if (help_greeting[0] == '.')
	    write_to_buffer(dnew, help_greeting+1, 0);
	else
	    write_to_buffer(dnew, help_greeting  , 0);
    }

    return;
}


void close_socket(DESCRIPTOR_DATA *dclose){
    CHAR_DATA *ch;

    if (dclose->outtop > 0)
	process_output(dclose, FALSE);

    if (dclose->snoop_by != NULL){
	    write_to_buffer(dclose->snoop_by, "Твоя цель вышла из игры.\n\r", 0);
    }


	DESCRIPTOR_DATA *d;

	SLIST_FOREACH(d, &descriptor_list, link){
	    if (d->snoop_by == dclose)
		d->snoop_by = NULL;
	}


    if ((ch = dclose->character) != NULL){
        char bfr[MAX_INPUT_LENGTH];


        sprintf(bfr, "Closing link to %s.", ch->name);
        log_string(bfr);

        if (ch->pet && ch->pet->in_room == NULL)
        {
            char_from_room(ch->pet);
            char_to_room(ch->pet, get_room_index(ROOM_VNUM_LIMBO), FALSE);
            extract_char(ch->pet, TRUE);
        }

        /* cut down on wiznet spam when rebooting */

        check_auctions(ch, NULL, "исчезновения владельца неизвестно куда");

        if (dclose->connected == CON_PLAYING && !merc_down)
        {
            if (!IS_IMMORTAL(ch))
            {
                act("$n теряет сознание.", ch, NULL, NULL, TO_ROOM);
            }

            edit_done(ch);	/* Снимаем флаг редактирования */

            sprintf(bfr, "$N@%s теряет связь.", dns_gethostname(dclose->Host));
            wiznet(bfr, ch, NULL, WIZ_LINKS, 0, get_trust(ch));

            if (ch->desc && ch->desc->original && !ch->desc->original->in_room)
            ch->desc->original->in_room = ch->in_room;

            ch->desc = NULL;
        }
        else
        {
            free_char(CH(dclose));
        }
    }

    SLIST_REMOVE(&descriptor_list, dclose, descriptor_data, link);

    if (dclose->out_compress){
        deflateEnd(dclose->out_compress);
        free_mem(dclose->out_compress_buf, COMPRESS_BUF_SIZE);
        free_mem(dclose->out_compress, sizeof(z_stream));
    }


    close(dclose->descriptor);

    free_descriptor(dclose);
    return;
}



bool read_from_descriptor(DESCRIPTOR_DATA *d){
    int iStart;


    /* Hold horses if pending command already. */
    if (d->incomm[0] != '\0')
	return TRUE;

    /* Check for overflow. */
    iStart = strlen(d->inbuf);
    if (iStart >= (int)(sizeof(d->inbuf) - 10))
    {
	char bfr[MAX_STRING_LENGTH];

	sprintf(bfr, "%s input overflow!", d->ip);
	log_string(bfr);
	write_to_buffer(d, "\n\r{R*** ЭТО СПАМ!!! ***{x\n\r", 0);
	return FALSE;
    }

    /* Snarf input. */
    for (; ;)
    {
	int nRead;

	nRead = recv(d->descriptor, d->inbuf + iStart, sizeof(d->inbuf) - 10 - iStart, 0);
	if (nRead > 0)
	{
	    iStart += nRead;
	    if (d->inbuf[iStart-1] == '\n' || d->inbuf[iStart-1] == '\r')
		break;
	}
	else if (nRead == 0)
	{
	    log_string("EOF encountered on read.");
	    return FALSE;
	}

	else if (errno == EWOULDBLOCK)

	    break;
	else
	{
	    _perror("Read_from_descriptor");
	    return FALSE;
	}
    }

    d->inbuf[iStart] = '\0';

    return TRUE;
}


int cmd_lookup(CHAR_DATA *ch, char *command);

/*
 * Transfer one line from input buffer to input line.
 */
void read_from_buffer(DESCRIPTOR_DATA *d)
{
    char bfr[MAX_STRING_LENGTH];
    int i, j, k;
    bool flag = TRUE;
    CHAR_DATA *ch;


    d->grep[0] = '\0';

    /* Очистка буфера команд */

    for (i = 0; d->inbuf[i] != '\0'; i++)
    {
	if (flag && d->inbuf[i] == '-')
	{
	    d->inbuf[0] = '\0';
	    write_to_buffer(d, "Буфер команд очищен.\n\r", 0);
	    return;
	}

	flag = FALSE;

	if (d->inbuf[i] == '\n' || d->inbuf[i] == '\r')
	    flag = TRUE;
    }

    /*
     * Hold horses if pending command already.
     */
    if (d->incomm[0] != '\0')
	return;


    /*
     * Look for at least one new line.
     */

    for (i = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++)
	if (d->inbuf[i] == '\0')
	    return;

    /*
     * Canonical input processing.
     */
    /* if (d->inbuf[0] != '\0')
     {
     log_string("INBUF");
     log_string(d->inbuf);
     }  */

    for (i = 0, k = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++)
    {
	if (k >= MAX_INPUT_LENGTH - 2)
	{
	    write_to_buffer(d, "Слишком длинная строка.\n\r", 0);

	    /* skip the rest of the line */
	    for (; d->inbuf[i] != '\0'; i++)
	    {
		if (d->inbuf[i] == '\n' || d->inbuf[i] == '\r')
		    break;
	    }
	    d->inbuf[i]   = '\n';
	    d->inbuf[i+1] = '\0';

	    break;
	}

	if (d->inbuf[i] == (signed char)IAC)
	{
	    /*            log_string(d->ip);
	     log_string(d->inbuf); */
	    if (!memcmp(&d->inbuf[i], compress_do, strlen(compress_do)))
	    {
		compressStart(d, FALSE);
		i += strlen(compress_do) - 1;
		continue;
	    }
	    if (!memcmp(&d->inbuf[i], compress_do2, strlen(compress_do2)))
	    {
		compressStart(d, TRUE);
		i += strlen(compress_do2) - 1;
		continue;
	    }
	    if (!memcmp(&d->inbuf[i], echo_do_str, strlen(echo_do_str)))
	    {
		i += strlen(echo_do_str) - 1;
		continue;
	    }
	    if (!memcmp(&d->inbuf[i], echo_dont_str, strlen(echo_dont_str)))
	    {
		i += strlen(echo_dont_str) - 1;
		continue;
	    }
	    else if (!memcmp(&d->inbuf[i], compress_dont, strlen(compress_dont)))
	    {
		if (d->mccp_version != 2)
		    compressEnd(d);
		i += strlen(compress_dont) - 1;
		continue;
	    }
	    else if (!memcmp(&d->inbuf[i], compress_dont2,
			     strlen(compress_dont2)))
	    {
		if (d->mccp_version != 1)
		    compressEnd(d);

		i += strlen(compress_dont2) - 1;
		continue;
	    }
	    else if (!memcmp (&d->inbuf[i], do_mxp_str, strlen(do_mxp_str)))
	    {
		turn_on_mxp (d);
		/* remove string from input buffer */
		i += strlen(do_mxp_str) - 1;
		continue;
	    } /* end of turning on MXP */
	    else  if (!memcmp (&d->inbuf[i], dont_mxp_str, strlen(dont_mxp_str)))
	    {
		d->mxp = FALSE;
		i += strlen(dont_mxp_str) - 1;
		continue;
	    } /* end of turning off MXP */
	    else if (d->codepage == 0)
	    {
		i++;
		continue;
	    }
	}
	else if (d->inbuf[i] == '\b' && k > 0)
	{
	    --k;
	    continue;
	}

	d->incomm[k++] = d->inbuf[i];
    }

    /*
     * Finish off the line.
     */
    if (k == 0)
	d->incomm[k++] = ' ';
    d->incomm[k] = '\0';

    //    log_string(d->incomm);

    /*
     * Deal with bozos with #repeat 1000 ...
     */

    if (k > 1 || d->incomm[0] == '!')
    {
	if (d->incomm[0] != '!' && strcmp(d->incomm, d->inlast))
	{
	    d->repeat = 0;
	}
	else
	{
	    one_argument(d->incomm, bfr);

	    if (++d->repeat >= 25 && d->character
		&& d->connected == CON_PLAYING
		&& !is_name(bfr, "север юг запад восток верх вверх низ north "
			    "south east west up down новост news письм пись ново нов новости новос пис пи ид иде письмо"
			    "note изменения changes идея ideas конец quit изм из изме измен измене изменен изменени")
		&& !(IS_IMMORTAL(d->character)))
	    {
		sprintf(bfr, "%s input spamming!", d->ip);
		log_string(bfr);
		wiznet("$N замечен в спаме!", d->character, NULL,
		       WIZ_SPAM, 0, get_trust(d->character));

		if (d->incomm[0] == '!')
		{
		    convert_dollars(d->inlast);
		    wiznet(d->inlast, d->character, NULL, WIZ_SPAM, 0,
			   get_trust(d->character));
		}
		else
		{
		    convert_dollars(d->incomm);
		    wiznet(d->incomm, d->character, NULL, WIZ_SPAM, 0,
			   get_trust(d->character));
		}

		d->repeat = 0;

		write_to_buffer(d, "\n\r*** СПАМ НЕ РАЗРЕШЕН!!! ***\n\r", 0);
		strcpy(d->incomm, "quit");

	    }
	}
    }

    /*
     * Do '!' substitution.
     */
    if (d->incomm[0] == '!')
	strcpy(d->incomm, d->inlast);
    else
	strcpy(d->inlast, d->incomm);

    /*
     * Shift the input buffer.
     */
    while (d->inbuf[i] == '\n' || d->inbuf[i] == '\r')
	i++;

    for (j = 0; (d->inbuf[j] = d->inbuf[i+j]) != '\0'; j++)
	;

    if (d->codepage > 0)
    {
	if (d->codepage == CODEPAGE_WIN)
	{
	    for (i=0;d->incomm[i] != '\0';i++)
	    {
		if (d->incomm[i] == (signed char)IAC
		    && d->incomm[i + 1] == (signed char)IAC)
		{
		    for (j= i + 1; d->incomm[j] != '\0'; j++)
			d->incomm[j] = d->incomm[j + 1];
		}
	    }
	}

	if (d->codepage == CODEPAGE_TRANS)
	{
	    if (d->connected == CON_PLAYING)
	    {
		char arg[MIL];

		one_argument(d->incomm, arg);

		log_string(arg);

		if (cmd_lookup(d->character, arg) < 0)
		    recode(d->incomm, d->codepage, RECODE_INPUT);
	    }
	}
	else
	    recode(d->incomm, d->codepage, RECODE_INPUT);

	for (i=0;stoplist[i] != NULL && i < MAX_STOP_WORDS;i++)
	{
	    if (!str_infix(stoplist[i], d->incomm))
	    {
		write_to_buffer(d, "ВЫБИРАЙ ВЫРАЖЕНИЯ!\n\r\n\r", 0);
		sprintf(bfr, "Нецензурщина: '%s' Host: %s",
			d->incomm, d->ip);
		log_string(bfr);
		sprintf(d->incomm, "\n\r");
		return;
	    }
	}
    }

    if (d->incomm[0] == ' ')
	return;

    if (d->connected == CON_PLAYING && (ch = CH(d)) != NULL && IS_SET(ch->act, PLR_LOG_OUTPUT))
    {
	char logname[50];

	sprintf(logname, "%s%s.out", LOG_DIR, ch->name);
	/*recode(logname, CODEPAGE_ALT, RECODE_OUTPUT|RECODE_NOANTITRIGGER);
*/

	sprintf(bfr, " %s\n", d->incomm);

	_log_string(bfr, logname);
    }

    if (d->editor == ED_NONE)
    {
	for (i = 1; d->incomm[i] != '\0'; i++)
	{
	    if (d->incomm[i] == '|' && d->incomm[i - 1] != '\\')
	    {
		int j;

		strcpy(d->grep, d->incomm + i + 1);
		d->incomm[i] = '\0';

		for (i = 0; d->grep[i] == ' '; i++)
		    ;

		if (i > 0)
		{
		    for (j = 0; d->grep[j+i] != '\0'; j++)
			d->grep[j] = d->grep[j+i];

		    d->grep[j] = '\0';
		    i = j - 1;
		}
		else
		    i = strlen(d->grep) - 1;

		for (; d->grep[i] == ' '; i--)
		    d->grep[i] = '\0';

		break;
	    }
	    else if (d->incomm[i - 1] == '\\')
	    {
		/* strcpy() is safe here */
		strcpy(&(d->incomm[i - 1]), &(d->incomm[i]));
	    }
	}
    }
    return;
}



/*
 * Low level output function.
 */
bool process_output(DESCRIPTOR_DATA *d, bool fPrompt)
{
    extern bool merc_down;
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *ch;


    /*
     * Bust a prompt.
     * OLC changed
     */
    if (!merc_down)
    {
	if (fPrompt && d->showstr_point)
	{
	    char tmp[MIL];

	    colourconv(buf, "{Y[Нажмите Enter для продолжения]{x\n\r", d->character);
	    strcpy(tmp, d->grep);
	    d->grep[0] = '\0';
	    write_to_buffer(d, buf, 0);
	    strcpy(d->grep, tmp);
	}
	else if (fPrompt && d->pString && d->connected == CON_PLAYING)
	    write_to_buffer(d, "> ", 2);
	else if (fPrompt && d->connected == CON_PLAYING)
	{
	    if ((ch = d->character) != NULL)
	    {
	        CHAR_DATA *victim;

	        d->grep[0] = '\0';

	        /* battle prompt */
	        if ((victim = ch->fighting) != NULL && can_see(ch, victim))
	        { 
		    int percent;
		    char wound[100];
		    char buf[MAX_STRING_LENGTH];
		    char pbuff[MAX_STRING_LENGTH];

		    percent = (victim->max_hit > 0) ? victim->hit * 100 / victim->max_hit : -1;

		    if (percent >= 100)
		        sprintf(wound, "{Gв прекрасном состоянии.{x");
		    else if (percent >= 90)
		        sprintf(wound, "{gслегка поцарапан%s.{x", SEX_ENDING(victim));
		    else if (percent >= 75)
		        sprintf(wound, "{Cлегко ранен%s.{x", SEX_ENDING(victim));
		    else if (percent >= 50)
		        sprintf(wound, "{cранен%s.{x", SEX_ENDING(victim));
		    else if (percent >= 30)
		        sprintf(wound, "{Yсерьезно ранен%s.{x", SEX_ENDING(victim));
		    else if (percent >= 15)
		        sprintf(wound, "{yвыглядит очень плохо.{x");
		    else if (percent >= 0)
		        sprintf(wound, "{rв ужасном состоянии.{x");
		    else
		        sprintf(wound, "{Rистекает кровью.{x");

		    sprintf(pbuff, "%s %s \n\r",
			IS_NPC(victim) ? victim->short_descr : victim->name, wound);
		    colourconv(buf, pbuff, ch);
		    buf[0] = UPPER(buf[0]);
		    write_to_buffer(d, buf, 0);
	        }
	     
	        ch = CH(d);
	        if (!IS_SET(ch->comm, COMM_COMPACT))
		    write_to_buffer(d, "\n\r", 2);

	        if (IS_SET(ch->comm, COMM_PROMPT))
		    bust_a_prompt(d->character);

	        if (IS_SET(ch->comm, COMM_TELNET_GA))
		    write_to_buffer(d, go_ahead_str, -1);
	    }
	}
    }

    /*
     * Short-circuit if nothing to write.
     */
    if (d->outtop == 0)
	return TRUE;

    /*
     * Snoop-o-rama.
     */
    if (d->snoop_by != NULL)
    {
	if (d->character != NULL)
	    write_to_buffer(d->snoop_by, d->character->name, 0);

	write_to_buffer(d->snoop_by, "> ", 2);
	write_to_buffer(d->snoop_by, d->outbuf, d->outtop);
    }

    if (d->connected == CON_PLAYING && (ch = CH(d)) != NULL && IS_SET(ch->act, PLR_LOG_OUTPUT))
    {
	char logname[50];
	char tmp[OUTBUF_SIZE];
	int i, j;

	sprintf(logname, "%s%s.out", LOG_DIR, ch->name);
	/*recode(logname, CODEPAGE_ALT, RECODE_OUTPUT|RECODE_NOANTITRIGGER);
*/

	for (i = 0, j = 0; i < d->outtop; i++)
	    if (d->outbuf[i] != '\r')
		tmp[j++] = d->outbuf[i];

	tmp[j] = '\0';

	_log_string(tmp, logname);
    }

    /*
     * OS-dependent output.
     */
    if (!write_to_descriptor(d, d->outbuf, d->outtop))
    {
	d->outtop = 0;
	return FALSE;
    }
    else
    {
	d->outtop = 0;
	return TRUE;
    }
}

/*
 * Bust a prompt (player settable prompt)
 * coded by Morgenes for Aldara Mud
 */
void bust_a_prompt(CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char buf3[MAX_STRING_LENGTH];
    const char *str;
    const char *i;
    char *point;
    char *pbuff;
    char buffer[ MAX_STRING_LENGTH*2 ];
    char doors[MAX_INPUT_LENGTH];
    EXIT_DATA *pexit;
    bool found;
    const char *dir_name[] = {"N", "E", "S", "W", "U", "D"};
    const char *dir_rname[] = {"С", "В", "Ю", "З", "Вв", "Н"};
    int door;

    CHAR_DATA *victim;
    char wound[40];
    int percent;

    if (!ch || !ch->desc)
	return;

    /* reset MXP to default operation */
    if (ch->desc->mxp)
	write_to_buffer(ch->desc, ESC "[3z", 0);

    switch (ch->desc->editor)
    {
    case ED_AREA:
	sprintf(buf3, "<AEDIT:%s> ", olc_ed_vnum(ch));
	send_to_char(buf3, ch);
	break;
    case ED_ROOM:
	sprintf(buf3, "<REDIT:%s> ", olc_ed_vnum(ch));
	send_to_char(buf3, ch);
	break;
    case ED_OBJECT:
	sprintf(buf3, "<OEDIT:%s> ", olc_ed_vnum(ch));
	send_to_char(buf3, ch);
	break;
    case ED_MOBILE:
	sprintf(buf3, "<MEDIT:%s> ", olc_ed_vnum(ch));
	send_to_char(buf3, ch);
	break;
    case ED_MPCODE:
	sprintf(buf3, "<MPEDIT:%s> ", olc_ed_vnum(ch));
	send_to_char(buf3, ch);
	break;
    case ED_OPCODE:
	sprintf(buf3, "<OPEDIT:%s> ", olc_ed_vnum(ch));
	send_to_char(buf3, ch);
	break;
    case ED_RPCODE:
	sprintf(buf3, "<RPEDIT:%s> ", olc_ed_vnum(ch));
	send_to_char(buf3, ch);
	break;
    case ED_HELP:
	sprintf(buf3, "<HEDIT> ");
	send_to_char(buf3, ch);
	break;
    case ED_SKILL:
	sprintf(buf3, "<SKEDIT> ");
	send_to_char(buf3, ch);
	break;
    case ED_CLASS:
	sprintf(buf3, "<CLEDIT> ");
	send_to_char(buf3, ch);
	break;
    case ED_CHAT:
	sprintf(buf3, "<CEDIT:%s> ", olc_ed_vnum(ch));
	send_to_char(buf3, ch);
	break;
    case ED_RACE:
	printf_to_char("<RaEDIT:%s> ", ch, olc_ed_vnum(ch));
	break;
    case ED_GROUP:
	sprintf(buf3, "<GREDIT> ");
	send_to_char(buf3, ch);
	break;

    default:
	break;
    }

    point = buf;
    str = ch->prompt;
    if (str == NULL || str[0] == '\0')
    {
	sprintf(buf, "{c<%dhp %dm %dmv>{x %s ",
		ch->hit, ch->mana, ch->move, ch->prefix);
	send_to_char(buf, ch);
	return;
    }

    if (IS_SET(ch->comm, COMM_AFK))
    {
	send_to_char("{c<ВОК>{x ", ch);
	return;
    }

    while(*str != '\0')
    {
	if(*str != '%')
	{
	    *point++ = *str++;
	    continue;
	}
	++str;
	switch(*str)
	{
	default :
	    i = " "; break;


	case 'T' :
	    if (IS_IMMORTAL(ch))
	    {
		pbuff = ctime(&current_time);
		pbuff+=4;
		pbuff[strlen(pbuff)-6] = '\0';
	    }
	    else
		pbuff="";
	    i=pbuff;
	    break;

	case 'b' :

	    /*    this is the graphical battle damage prompt
	     *
	     *    <- Gothar 1997 ->
	     */

	    if ((victim = ch->fighting) != NULL && can_see(ch, victim))
	    {

		percent = (victim->max_hit > 0) ? victim->hit * 100 / victim->max_hit : -1;

		if (percent >= 100)
		    sprintf(wound, "{c[{r###{Y###{g####{c]{x");
		else if (percent >= 90)
		    sprintf(wound, "{c[{r###{Y###{g### {c]{x");
		else if (percent >= 80)
		    sprintf(wound, "{c[{r###{Y###{g##  {c]{x");
		else if (percent >= 70)
		    sprintf(wound, "{c[{r###{Y###{g#   {c]{x");
		else if (percent >= 58)
		    sprintf(wound, "{c[{r###{Y###    {c]{x");
		else if (percent >= 45)
		    sprintf(wound, "{c[{r###{Y##     {c]{x");
		else if (percent >= 30)
		    sprintf(wound, "{c[{r###{Y#      {c]{x");
		else if (percent >= 28)
		    sprintf(wound, "{c[{r###{x       {c]{x");
		else if (percent >= 15)
		    sprintf(wound, "{c[{r##{x        {c]{x");
		else if (percent >= 8)
		    sprintf(wound, "{c[{r#{x         {c]{x");
		else
		    sprintf(wound, "{c[          {c]{x");
		i = wound;
	    }
	    else
		i = "";
	    break;
	case '^' :

	    /*    this is the percentage battle damage prompt
	     *    The prompt changes colour to show the
	     *    condition of the mob.
	     *    <- Gothar 1997 ->
	     */
	    if (IS_IMMORTAL(ch) && (victim = ch->fighting) != NULL)
	    {
		percent = (victim->max_hit > 0) ? victim->hit * 100 / victim->max_hit : -1;

		if(percent >= 65)
		    sprintf(buf2, "{c[{g%d%%{c]{x", percent);
		else if(percent >= 25 && percent < 65)
		    sprintf(buf2, "{c[{Y%d%%{c]{x", percent);
		else
		    sprintf(buf2, "{c[{r%d%%{c]{x", percent);
		i = buf2;
	    }
	    else
		i = "";
	    break;

	case 'e':
	case 'E':
	    doors[0] = '\0';
	    found = FALSE;
	    if (IS_AWAKE(ch))
	    {
		if (IS_AFFECTED(ch, AFF_BLIND)
		    && !IS_SET(ch->act, PLR_HOLYLIGHT))
		{
		    found=TRUE;
		    strcpy(doors, (*str == 'e') ? "blind" : "слеп");
		}
		else if (ch->in_room)
		{
		    if (!IS_NPC(ch)
			&& !IS_SET(ch->act, PLR_HOLYLIGHT)
			&& room_is_dark_new(ch))
		    {
			found=TRUE;
			strcpy(doors, (*str == 'e') ? "dark" : "темно");
		    }
		    else
		    {
			for (door = 0; door < 6; door++)
			{
			    if ((pexit = ch->in_room->exit[door]) != NULL
				&& pexit->u1.to_room != NULL
				&& (can_see_room(ch, pexit->u1.to_room)
				    || IS_AFFECTED(ch, AFF_INFRARED))
				&& !IS_AFFECTED(ch, AFF_BLIND)
				&& !IS_SET(pexit->exit_info, EX_CLOSED))
			    {
				found = TRUE;
				strcat(doors, (*str == 'e') ? dir_name[door] : dir_rname[door]);
			    }
			}
		    }
		}
	    }
	    else
	    {
		strcpy(doors, (*str == 'e') ? "sleeping" : "сон");
		found=TRUE;
	    }
	    if (!found)
		strcpy(doors, (*str == 'e') ? "none" : "нет");

	    strcpy(buf2, doors);
	    i = buf2;
	    break;
	case 'y' :
	    sprintf(buf2, "%s", ch->style == NORM_STYLE ? "Норма" 
				: (ch->style == AGGR_STYLE ? "Атака" : "Оборона"));
	    i = buf2; break;

//бессмертие и посмертие
	case 'i' :
		if (!IS_NPC(ch))
		{
			if (ch->pcdata->temp_RIP == 0)
			{
			    sprintf(buf2, "%s", "нет"); 
			}
			else
			{
				if (ch->pcdata->temp_RIP == 1)
				{
				    sprintf(buf2, "%s", "выкл"); 
				}
				else
				{
					if (ch->pcdata->temp_RIP == 2)
					{
						sprintf(buf2, "%s", "вкл");
					}
					else
					{
						if (ch->pcdata->temp_RIP == 3)
						{
							sprintf(buf2, "%s", "посм");
						}
					}
				}
			}
		}
		else
		{
		    sprintf(buf2, "%s", "неопознано"); 
		}
		i = buf2; break;


	case 'I' :
		if (!IS_NPC(ch))
		{
			if (ch->pcdata->temp_RIP == 0)
			{
			    sprintf(buf2, "%s", "не куплено"); 
			}
			else
			{
				if (ch->pcdata->temp_RIP == 1)
				{
				    sprintf(buf2, "%s", "бессмертие выключено"); 
				}
				else
				{
					if (ch->pcdata->temp_RIP == 2)
					{
						sprintf(buf2, "%s", "бессмертие включено");
					}
					else
					{
						if (ch->pcdata->temp_RIP == 3)
						{
							sprintf(buf2, "%s", "посмертие");
						}
					}
				}
			}
		}
		else
		{

		    sprintf(buf2, "%s", "неопознано"); 
		}
		i = buf2; break;



	case 'c' :
	    strcpy(buf2, "\n\r");
	    i = buf2; break;
	case 'h' :
	    sprintf(buf2, "%d", ch->hit);
	    i = buf2; break;
	case 'H' :
	    sprintf(buf2, "%d", ch->max_hit);
	    i = buf2; break;
	case 'm' :
	    sprintf(buf2, "%d", ch->mana);
	    i = buf2; break;
	case 'M' :
	    sprintf(buf2, "%d", ch->max_mana);
	    i = buf2; break;
	case 'v' :
	    sprintf(buf2, "%d", ch->move);
	    i = buf2; break;
	case 'V' :
	    sprintf(buf2, "%d", ch->max_move);
	    i = buf2; break;
	case 'x' :
	    sprintf(buf2, "%d", ch->exp);
	    i = buf2; break;
	case 'X' :
	    sprintf(buf2, "%d", TNL(ch));
	    i = buf2; break;
	case 'g' :
	    sprintf(buf2, "%ld", ch->gold);
	    i = buf2; break;
	case 's' :
	    sprintf(buf2, "%ld", ch->silver);
	    i = buf2; break;
	case 'a' :
	    if(ch->level > 9)
		sprintf(buf2, "%d", ch->alignment);
	    else if (ch->alignment >  900)
		sprintf(buf2, "%s", "ангел");
	    else if (ch->alignment >  700)
		sprintf(buf2, "%s", "святая");
	    else if (ch->alignment >  350)
		sprintf(buf2, "%s", "хорошая");
	    else if (ch->alignment >  100)
		sprintf(buf2, "%s", "любезная");
	    else if (ch->alignment > -100)
		sprintf(buf2, "%s", "нейтральная");
	    else if (ch->alignment > -350)
		sprintf(buf2, "%s", "подлая");
	    else if (ch->alignment > -700)
		sprintf(buf2, "%s", "злая");
	    else if (ch->alignment > -900)
		sprintf(buf2, "%s", "демон");
	    else
		sprintf(buf2, "%s", "сатана");

	    i = buf2;
	    break;
	case 'r' :
	    if(SUPPRESS_OUTPUT(check_blind(ch)) && ch->in_room != NULL && IS_AWAKE(ch) && can_see_room(ch, ch->in_room))
		sprintf(buf2, "%s", room_is_dark_new(ch) ? "темно" : ch->in_room->name);
	    else
		strcpy(buf2, " ");
	    i = buf2; break;
	case 'R' :
	    if(IS_IMMORTAL(ch) && ch->in_room != NULL)
		sprintf(buf2, "%d", ch->in_room->vnum);
	    else
		strcpy(buf2, " ");
	    i = buf2; break;
	case 'z' :
	    if(IS_IMMORTAL(ch) && ch->in_room != NULL)
		sprintf(buf2, "%s", ch->in_room->area->name);
	    else
		sprintf(buf2, " ");
	    i = buf2; break;
	case 't' :
	    sprintf(buf2, "%d", time_info.hour);
	    i = buf2; break;
	case '%' :
	    strcpy(buf2, "%%");
	    i = buf2; break;
	case 'o' :
	    sprintf(buf2, "%s", olc_ed_name(ch));
	    i = buf2; break;
	case 'O' :
	    sprintf(buf2, "%s", olc_ed_vnum(ch));
	    i = buf2; break;
	    /*
	     * Общее время и время до окончания квеста.
	     */
	case 'q' :
	    if (IS_NPC(ch))
		buf2[0] = '\0';
	    else if (IS_SET(ch->act, PLR_QUESTING))
		sprintf(buf2, "%d", ch->pcdata->countdown);
	    else if (ch->pcdata->nextquest)
		sprintf(buf2, "%d tnq", ch->pcdata->nextquest);
	    else
		strcpy(buf2, " ");
	    i = buf2;
	    break;
	case 'Q' :
	    if (IS_NPC(ch))
		buf2[0] = '\0';
	    else if (IS_SET(ch->act, PLR_QUESTING))
		sprintf(buf2, "%d", ch->pcdata->qtime);
	    else
		strcpy(buf2, " ");
	    i = buf2;
	    break;
	case 'p' :
	    if (IS_NPC(ch))
		buf2[0] = '\0';
	    else
		sprintf(buf2, "%d", ch->pcdata->quest_curr);
	    i = buf2;
	    break;
	case 'P' :
	    if (IS_NPC(ch))
		buf2[0] = '\0';
	    else
		sprintf(buf2, "%d", ch->pcdata->quest_accum);
	    i = buf2;
	    break;

	    /*
	     * Погоду в промпт. Для вампиров и кастеров полезно.
	     */
	case 'w' :
	    if (IS_OUTSIDE(ch) && IS_AWAKE(ch))
	    {
		switch (weather_info.sky)
		{
		case SKY_CLOUDLESS:
		    sprintf(buf2, "{x[{Yбезоблачно{x]");
		    break;
		case SKY_CLOUDY:
		    sprintf(buf2, "{x[{yоблачно{x]");
		    break;
		case SKY_RAINING:
		    sprintf(buf2, "{x[{wидет дождь{x]");
		    break;
		case SKY_LIGHTNING:
		    sprintf(buf2, "{x[{Dгроза{x]");
		    break;
		default:
		    strcpy(buf2, " ");
		    break;
		}
	    }
	    else
		strcpy(buf2, " ");
	    i = buf2; break;
	}
	++str;
	while ((*point = *i) != '\0')
	    ++point, ++i;
    }

    *point      = '\0';
    pbuff       = buffer;
    colourconv(pbuff, buf, ch);
    send_to_char("{c", ch);
    write_to_buffer(ch->desc, buffer, 0);
    send_to_char("{x", ch);

    if (ch->prefix[0] != '\0')
    {
	write_to_buffer(ch->desc, ch->prefix, 0);
	write_to_buffer(ch->desc, " ", 0);
    }
    return;
}

/*
 * Count number of mxp tags need converting
 *    ie. < becomes &lt;
 *        > becomes &gt;
 *        & becomes &amp;
 */

int count_mxp_tags (const int bMXP, const char *txt, int length)
{
    char c;
    const char * p;
    int count;
    int bInTag = FALSE;
    int bInEntity = FALSE;

    for (p = txt, count = 0;
	 length > 0;
	 p++, length--)
    {
	c = *p;

	if (bInTag)  /* in a tag, eg. <send> */
	{
	    if (!bMXP)
		count--;     /* not output if not MXP */
	    if (c == MXP_ENDc)
		bInTag = FALSE;
	} /* end of being inside a tag */
	else if (bInEntity)  /* in a tag, eg. <send> */
	{
	    if (!bMXP)
		count--;     /* not output if not MXP */
	    if (c == ';')
		bInEntity = FALSE;
	} /* end of being inside a tag */
	else switch (c)
	{

	case MXP_BEGc:
	    bInTag = TRUE;
	    if (!bMXP)
		count--;     /* not output if not MXP */
	    break;

	case MXP_ENDc:   /* shouldn't get this case */
	    if (!bMXP)
		count--;     /* not output if not MXP */
	    break;

	case MXP_AMPc:
	    bInEntity = TRUE;
	    if (!bMXP)
		count--;     /* not output if not MXP */
	    break;

	default:
	    if (bMXP)
	    {
		switch (c)
		{
		case '<':       /* < becomes &lt; */
		case '>':       /* > becomes &gt; */
		    count += 3;
		    break;

		case '&':
		    count += 4;    /* & becomes &amp; */
		    break;

		case '"':        /* " becomes &quot; */
		    count += 5;
		    break;

		} /* end of inner switch */
	    }   /* end of MXP enabled */
	} /* end of switch on character */

    }   /* end of counting special characters */

    return count;
} /* end of count_mxp_tags */

void convert_mxp_tags (const int bMXP, char * dest, const char *src, int length)
{
    char c;
    const char * ps;
    char * pd;
    int bInTag = FALSE;
    int bInEntity = FALSE;

    for (ps = src, pd = dest;
	 length > 0;
	 ps++, length--)
    {
	c = *ps;
	if (bInTag)  /* in a tag, eg. <send> */
	{
	    if (c == MXP_ENDc)
	    {
		bInTag = FALSE;
		if (bMXP)
		    *pd++ = '>';
	    }
	    else if (bMXP)
		*pd++ = c;  /* copy tag only in MXP mode */
	} /* end of being inside a tag */
	else if (bInEntity)  /* in a tag, eg. <send> */
	{
	    if (bMXP)
		*pd++ = c;  /* copy tag only in MXP mode */
	    if (c == ';')
		bInEntity = FALSE;
	} /* end of being inside a tag */
	else switch (c)
	{
	case MXP_BEGc:
	    bInTag = TRUE;
	    if (bMXP)
		*pd++ = '<';
	    break;

	case MXP_ENDc:    /* shouldn't get this case */
	    if (bMXP)
		*pd++ = '>';
	    break;

	case MXP_AMPc:
	    bInEntity = TRUE;
	    if (bMXP)
		*pd++ = '&';
	    break;

	default:
	    if (bMXP)
	    {
		switch (c)
		{
		case '<':
		    memcpy (pd, "&lt;", 4);
		    pd += 4;
		    break;

		case '>':
		    memcpy (pd, "&gt;", 4);
		    pd += 4;
		    break;

		case '&':
		    memcpy (pd, "&amp;", 5);
		    pd += 5;
		    break;

		case '"':
		    memcpy (pd, "&quot;", 6);
		    pd += 6;
		    break;

		default:
		    *pd++ = c;
		    break;  /* end of default */

		} /* end of inner switch */
	    }
	    else
		*pd++ = c;  /* not MXP - just copy character */ /* КРЕШАЕТ МИР НА pload Гриэль; стат моб Гриэль; immdamage Гриэль; punload Гриэль*/
			/* В
			354 ==10804== Process terminating with default action of signal 11 (SIGSEGV): dumping core
			355 ==10804==  Access not within mapped region at address 0xFF362904
			356 ==10804==    at 0x80D78F2: convert_mxp_tags (comm.c:2577)
			357 ==10804==    by 0x80D7BD3: write_to_buffer (comm.c:2682)
			358 ==10804==    by 0x80DDA67: send_to_char (comm.c:9027)
			359 ==10804==    by 0x8125F4E: equip_char (handler.c:2507)
			360 ==10804==    by 0x816DC1C: fread_obj (save.c:2398)
			361 ==10804==    by 0x8167224: load_char_obj (save.c:1143)
			362 ==10804==    by 0x80CBD8C: do_pload (act_wiz.c:8497)
			363 ==10804==    by 0x812DF43: interpret (interp.c:1428)
			364 ==10804==    by 0x80D0B7D: substitute_alias (alias.c:162)
			365 ==10804==    by 0x80D3673: game_loop_unix (comm.c:1025)
			366 ==10804==    by 0x80D2CDA: main (comm.c:724)
			367 ==10804==  If you believe this happened as a result of a stack
			368 ==10804==  overflow in your program's main thread (unlikely but
			369 ==10804==  possible), you can try to increase the size of the
			370 ==10804==  main thread stack using the --main-stacksize= flag.
			371 ==10804==  The main thread stack size used in this run was 8388608.
			 */
	    break;

	} /* end of switch on character */

    }   /* end of converting special characters */
} /* end of convert_mxp_tags */


/*
 * Append onto an output buffer.
 */
void write_to_buffer(DESCRIPTOR_DATA *d, const char *txt, int length)
{
    int origlength;
    char buf[OUTBUF_SIZE];

    buf[0] = '\0';

    if (!Show_output)
	return;

    if (!IS_NULLSTR(d->grep))
    {
	int i, j = 0;
	char tmp[OUTBUF_SIZE];

	for (i = 0; ; i++)
	{
	    tmp[j++] = txt[i];

	    if (txt[i] == '\n' || txt[i] == '\r' || txt[i+1] == '\0')
	    {
		if (txt[i+1] == '\n' || txt[i+1] == '\r')
		    tmp[j++] = txt[++i];

		tmp[j] = '\0';

		if (!str_infix(d->grep, tmp))
		    strcat(buf, tmp);

		if (txt[i+1] == '\0')
		    break;

		j = 0;
	    }
	}

	length = strlen(buf);
    }
    else
    {
	/*
	 * Find length in case caller didn't.
	 */

	if (length <= 0)
	    length = strlen(txt);

	strcpy(buf, txt);
    }
    origlength = length;

    /* work out how much we need to expand/contract it */
    length += count_mxp_tags (d->mxp, buf, length);

    /*
     * Initial \n\r if needed.
     */
    if (d->outtop == 0 && !d->fcommand)
    {
	d->outbuf[0]	= '\n';
	d->outbuf[1]	= '\r';
	d->outtop	= 2;
    }

    /*
     * Expand the buffer as needed.
     */
    /*    while (d->outtop + length >= d->outsize)
     {
     char *outbuf; */

    if (/* d->outsize >= 32000 */ strlen(d->outbuf) + length >= OUTBUF_SIZE)
    {
	char buf[MSL];

	strlcpy(buf, d->character ? d->character->name : "(unknown)", MSL);
	d->outtop = 0;
	close_socket(d);
	bugf("Buffer overflow from char '%s'. Closing.", buf);
	return;
    }
    /*	outbuf      = alloc_mem(2 * d->outsize);
     strncpy(outbuf, d->outbuf, d->outtop);
     free_mem(d->outbuf, d->outsize);
     d->outbuf   = outbuf;
     d->outsize *= 2;
     }   */

    /*
     * Copy.
     */
    /* MXP remove:   strncpy(d->outbuf + d->outtop, txt, length); */

    convert_mxp_tags(d->mxp, d->outbuf + d->outtop, buf, origlength);
    d->outtop += length;
    return;
}



/*
 * Lowest level output function.
 * Write a block of text to the file descriptor.
 * If this gives errors on very long blocks (like 'ofind all'),
 *   try lowering the max block size.
 */
/*bool write_to_descriptor(DESCRIPTOR_DATA *d, char *txt, int length) */

bool write_to_descriptor_2(int d, char *txt, int length)

{
    int iStart;
    int nWrite;
    int nBlock;

    if (length <= 0)
	length = strlen(txt);

    for (iStart = 0; iStart < length; iStart += nWrite)
    {
	nBlock = UMIN(length - iStart, 4096);
	if ((nWrite = send(d, txt + iStart, nBlock, 0)) < 0)
	{ _perror("Write_to_descriptor2"); return FALSE; }
    }

    return TRUE;
}

/* mccp: write_to_descriptor wrapper */
bool write_to_descriptor(DESCRIPTOR_DATA *d, char *txt, int length)
{
    int i;
    char buf[2*OUTBUF_SIZE];
    bool flag = TRUE;

    buf[0] = '\0';

    if (length < 0)
	flag = FALSE;

    if (length <= 0)
	length = strlen(txt);

    strncpy(buf, txt, length);
    buf[length]='\0';

    if (flag && d->codepage > 0)
    {
	length += recode(buf, d->codepage, IS_IMMORTAL(CH(d)) ? RECODE_OUTPUT|RECODE_NOANTITRIGGER : RECODE_OUTPUT);

	if (d->codepage != CODEPAGE_WIN && d->codepage != CODEPAGE_TRANS)
	{
	    for (i=0;buf[i] != '\0';i++)
	    {
		if (buf[i] == (signed char)IAC)
		{
		    int j;

		    for (j=length;j >= i;j--)
			buf[j+1]=buf[j];

		    length++;
		    i++;
		}
	    }
	}
	else
	{
	    for(i=0;buf[i] != '\0';i++)
		if (buf[i] == (signed char)IAC) buf[i] = UPPER(buf[i]);
	}
    }

    if (d->out_compress)
	return writeCompressed(d, buf, length);
    else
	return write_to_descriptor_2(d->descriptor, buf, length);
}

int get_valid_align(CHAR_DATA *ch)
{
    return pc_race_table[ch->race].valid_align & class_table[ch->classid].valid_align;
}


int is_only_align(CHAR_DATA *ch)
{
    switch(get_valid_align(ch))
    {
    case ALIGN_GOOD: 	 return ALIGN_GOOD;
    case ALIGN_NEUTRAL: return ALIGN_NEUTRAL;
    case ALIGN_EVIL:	 return ALIGN_EVIL;
    default:		 return 0;
    }
}

void show_valid_align(CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];
    bool tm = FALSE;
    int va, cra;

    buf[0] = '\0';
     
    strlcpy(buf, "Ваш выбор [ ", sizeof(buf));

    cra = get_valid_align(ch);

    va = is_only_align(ch);
    write_to_buffer(ch->desc, "Вы можете быть", 0);
    if (va > 0)
	write_to_buffer(ch->desc," только", 0);

    if (IS_SET(cra, ALIGN_GOOD))
    {
	write_to_buffer(ch->desc," добрым", 0);
	if (va == 0)
	    strlcat(buf, "(Д)обрый", sizeof(buf));
	tm = TRUE;
    }

    if (IS_SET(cra, ALIGN_NEUTRAL))
    {
	if (va == 0)
	{
	    if (tm)
		strcat(buf, "/");
	    strlcat(buf, "(Н)ейтрал", sizeof(buf));
	}
	if (tm)
	    write_to_buffer(ch->desc,",", 0);
	write_to_buffer(ch->desc, " нейтралом", 0);
	tm = TRUE;
    }

    if (IS_SET(cra, ALIGN_EVIL))
    {
	if (va == 0)
	{
	    if (tm)
		strlcat(buf, "/", sizeof(buf));
	    strlcat(buf, "(З)лой", sizeof(buf));
	}
	if (tm)
	    write_to_buffer(ch->desc," или", 0);
	    
	write_to_buffer(ch->desc, " злым", 0);
    }

   write_to_buffer(ch->desc,".\n\r", 0);
   if (va > 0)
	write_to_buffer(ch->desc, "\n\r[Нажмите Enter для продолжения]\n\r", 0);
    else
    {
	strlcat(buf, " ] :\n\r", sizeof(buf));
	write_to_buffer(ch->desc, buf, 0);
    }
}

void strip_colors(char *str)
{
    char *p;

    for (p = str; *p != '\0'; p++)
    {
	while (*p == '{')
	{
	    char *t;

	    if (*(p+1) == '\0')
	    {
		*p = '\0';
		break;
	    }

	    for(t = p; *t != '\0'; t++)
		*t = *(t + 2);
	}

	if (*p == '\0')
	    break;

    }
}

bool check_case(CHAR_DATA *ch, char *cs, int c)
{
    if (memcmp(cs, ch->name, strlen(ch->name) - 2))
    {
	char buf[MSL];

	sprintf(buf, "\n\r{WНеправильный падеж (%s)!{x Нажмите просто <Enter>, если согласны с падежом по-умолчанию [%s].{x\n\r",
		cs, cases(ch->name, c));
	send_to_char(buf, ch);
	return FALSE;
    }

    return TRUE;
}



static int compensation_race[4524][7] =
{
	/*race,		class,     level,hp,mana,mv,pract*/
	{RACE_VAMPIRE,CLASS_VAMPIRE,0,0,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,1,0,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,2,0,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,3,0,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,4,0,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,5,0,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,6,0,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,7,0,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,8,0,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,9,0,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,10,79,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,11,81,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,12,83,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,13,85,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,14,87,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,15,89,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,16,91,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,17,93,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,18,95,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,19,97,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,20,99,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,21,101,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,22,103,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,23,105,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,24,107,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,25,109,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,26,111,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,27,113,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,28,115,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,29,117,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,30,119,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,31,121,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,32,123,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,33,125,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,34,127,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,35,129,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,36,131,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,37,133,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,38,135,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,39,137,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,40,139,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,41,141,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,42,143,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,43,145,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,44,147,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,45,149,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,46,151,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,47,153,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,48,155,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,49,157,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,50,159,0,0,0},
	{RACE_VAMPIRE,CLASS_VAMPIRE,51,162,0,0,0},
	{RACE_DWARF,CLASS_WARRIOR,0,0,0,0,0},
	{RACE_DWARF,CLASS_WARRIOR,1,2,0,0,0},
	{RACE_DWARF,CLASS_WARRIOR,2,4,0,0,0},
	{RACE_DWARF,CLASS_WARRIOR,3,6,0,0,0},
	{RACE_DWARF,CLASS_WARRIOR,4,8,1,0,0},
	{RACE_DWARF,CLASS_WARRIOR,5,10,1,0,0},
	{RACE_DWARF,CLASS_WARRIOR,6,12,2,0,0},
	{RACE_DWARF,CLASS_WARRIOR,7,14,2,0,0},
	{RACE_DWARF,CLASS_WARRIOR,8,16,1,0,0},
	{RACE_DWARF,CLASS_WARRIOR,9,18,2,0,0},
	{RACE_DWARF,CLASS_WARRIOR,10,20,2,0,0},
	{RACE_DWARF,CLASS_WARRIOR,11,22,3,0,0},
	{RACE_DWARF,CLASS_WARRIOR,12,24,3,0,0},
	{RACE_DWARF,CLASS_WARRIOR,13,26,3,0,0},
	{RACE_DWARF,CLASS_WARRIOR,14,28,3,0,0},
	{RACE_DWARF,CLASS_WARRIOR,15,30,4,0,0},
	{RACE_DWARF,CLASS_WARRIOR,16,32,5,0,0},
	{RACE_DWARF,CLASS_WARRIOR,17,34,4,0,0},
	{RACE_DWARF,CLASS_WARRIOR,18,36,3,0,0},
	{RACE_DWARF,CLASS_WARRIOR,19,38,3,0,0},
	{RACE_DWARF,CLASS_WARRIOR,20,40,2,0,0},
	{RACE_DWARF,CLASS_WARRIOR,21,42,2,0,0},
	{RACE_DWARF,CLASS_WARRIOR,22,44,2,0,0},
	{RACE_DWARF,CLASS_WARRIOR,23,46,2,0,0},
	{RACE_DWARF,CLASS_WARRIOR,24,48,3,0,0},
	{RACE_DWARF,CLASS_WARRIOR,25,50,3,0,0},
	{RACE_DWARF,CLASS_WARRIOR,26,52,3,0,0},
	{RACE_DWARF,CLASS_WARRIOR,27,54,2,0,0},
	{RACE_DWARF,CLASS_WARRIOR,28,56,1,0,0},
	{RACE_DWARF,CLASS_WARRIOR,29,58,0,0,0},
	{RACE_DWARF,CLASS_WARRIOR,30,60,0,0,0},
	{RACE_DWARF,CLASS_WARRIOR,31,62,0,0,0},
	{RACE_DWARF,CLASS_WARRIOR,32,64,0,0,0},
	{RACE_DWARF,CLASS_WARRIOR,33,66,0,0,0},
	{RACE_DWARF,CLASS_WARRIOR,34,68,1,0,0},
	{RACE_DWARF,CLASS_WARRIOR,35,70,1,0,0},
	{RACE_DWARF,CLASS_WARRIOR,36,72,1,0,0},
	{RACE_DWARF,CLASS_WARRIOR,37,74,2,0,0},
	{RACE_DWARF,CLASS_WARRIOR,38,76,2,0,0},
	{RACE_DWARF,CLASS_WARRIOR,39,78,2,0,0},
	{RACE_DWARF,CLASS_WARRIOR,40,80,2,0,0},
	{RACE_DWARF,CLASS_WARRIOR,41,82,1,0,0},
	{RACE_DWARF,CLASS_WARRIOR,42,84,2,0,0},
	{RACE_DWARF,CLASS_WARRIOR,43,86,3,0,0},
	{RACE_DWARF,CLASS_WARRIOR,44,88,2,0,0},
	{RACE_DWARF,CLASS_WARRIOR,45,90,1,0,0},
	{RACE_DWARF,CLASS_WARRIOR,46,92,1,0,0},
	{RACE_DWARF,CLASS_WARRIOR,47,94,1,0,0},
	{RACE_DWARF,CLASS_WARRIOR,48,96,1,0,0},
	{RACE_DWARF,CLASS_WARRIOR,49,98,1,0,0},
	{RACE_DWARF,CLASS_WARRIOR,50,100,2,0,0},
	{RACE_DWARF,CLASS_WARRIOR,51,102,2,0,0},
	{RACE_DWARF,CLASS_DRUID,0,0,0,0,0},
	{RACE_DWARF,CLASS_DRUID,1,2,0,0,1},
	{RACE_DWARF,CLASS_DRUID,2,4,0,0,2},
	{RACE_DWARF,CLASS_DRUID,3,6,0,0,3},
	{RACE_DWARF,CLASS_DRUID,4,8,0,0,4},
	{RACE_DWARF,CLASS_DRUID,5,10,0,0,5},
	{RACE_DWARF,CLASS_DRUID,6,12,0,0,6},
	{RACE_DWARF,CLASS_DRUID,7,14,0,0,7},
	{RACE_DWARF,CLASS_DRUID,8,16,0,0,8},
	{RACE_DWARF,CLASS_DRUID,9,18,0,0,9},
	{RACE_DWARF,CLASS_DRUID,10,20,0,0,10},
	{RACE_DWARF,CLASS_DRUID,11,22,0,0,11},
	{RACE_DWARF,CLASS_DRUID,12,24,0,0,12},
	{RACE_DWARF,CLASS_DRUID,13,26,0,0,13},
	{RACE_DWARF,CLASS_DRUID,14,28,0,0,14},
	{RACE_DWARF,CLASS_DRUID,15,30,0,0,15},
	{RACE_DWARF,CLASS_DRUID,16,32,0,0,16},
	{RACE_DWARF,CLASS_DRUID,17,34,0,0,17},
	{RACE_DWARF,CLASS_DRUID,18,36,0,0,18},
	{RACE_DWARF,CLASS_DRUID,19,38,0,0,19},
	{RACE_DWARF,CLASS_DRUID,20,40,0,0,20},
	{RACE_DWARF,CLASS_DRUID,21,42,0,0,21},
	{RACE_DWARF,CLASS_DRUID,22,44,0,0,22},
	{RACE_DWARF,CLASS_DRUID,23,46,0,0,23},
	{RACE_DWARF,CLASS_DRUID,24,48,0,0,24},
	{RACE_DWARF,CLASS_DRUID,25,50,0,0,25},
	{RACE_DWARF,CLASS_DRUID,26,52,0,0,26},
	{RACE_DWARF,CLASS_DRUID,27,54,0,0,27},
	{RACE_DWARF,CLASS_DRUID,28,56,0,0,28},
	{RACE_DWARF,CLASS_DRUID,29,58,0,0,29},
	{RACE_DWARF,CLASS_DRUID,30,60,0,0,30},
	{RACE_DWARF,CLASS_DRUID,31,62,0,0,31},
	{RACE_DWARF,CLASS_DRUID,32,64,0,0,32},
	{RACE_DWARF,CLASS_DRUID,33,66,0,0,33},
	{RACE_DWARF,CLASS_DRUID,34,68,0,0,34},
	{RACE_DWARF,CLASS_DRUID,35,70,0,0,35},
	{RACE_DWARF,CLASS_DRUID,36,72,0,0,36},
	{RACE_DWARF,CLASS_DRUID,37,74,0,0,37},
	{RACE_DWARF,CLASS_DRUID,38,76,0,0,38},
	{RACE_DWARF,CLASS_DRUID,39,78,0,0,39},
	{RACE_DWARF,CLASS_DRUID,40,80,0,0,40},
	{RACE_DWARF,CLASS_DRUID,41,82,0,0,41},
	{RACE_DWARF,CLASS_DRUID,42,84,0,0,42},
	{RACE_DWARF,CLASS_DRUID,43,86,0,0,43},
	{RACE_DWARF,CLASS_DRUID,44,88,0,0,44},
	{RACE_DWARF,CLASS_DRUID,45,90,0,0,45},
	{RACE_DWARF,CLASS_DRUID,46,92,0,0,46},
	{RACE_DWARF,CLASS_DRUID,47,94,0,0,47},
	{RACE_DWARF,CLASS_DRUID,48,96,0,0,48},
	{RACE_DWARF,CLASS_DRUID,49,98,0,0,49},
	{RACE_DWARF,CLASS_DRUID,50,100,0,0,50},
	{RACE_DWARF,CLASS_DRUID,51,102,0,0,51},
	{RACE_DWARF,CLASS_CLERIC,0,0,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,1,2,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,2,4,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,3,6,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,4,8,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,5,10,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,6,12,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,7,14,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,8,16,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,9,18,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,10,20,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,11,22,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,12,24,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,13,26,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,14,28,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,15,30,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,16,32,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,17,34,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,18,36,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,19,38,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,20,40,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,21,42,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,22,44,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,23,46,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,24,48,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,25,50,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,26,52,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,27,54,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,28,56,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,29,58,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,30,60,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,31,62,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,32,64,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,33,66,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,34,68,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,35,70,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,36,72,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,37,74,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,38,76,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,39,78,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,40,80,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,41,82,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,42,84,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,43,86,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,44,88,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,45,90,0,0,0},
	{RACE_DWARF,CLASS_CLERIC,46,92,1,0,0},
	{RACE_DWARF,CLASS_CLERIC,47,94,1,0,0},
	{RACE_DWARF,CLASS_CLERIC,48,96,1,0,0},
	{RACE_DWARF,CLASS_CLERIC,49,98,1,0,0},
	{RACE_DWARF,CLASS_CLERIC,50,100,1,0,0},
	{RACE_DWARF,CLASS_CLERIC,51,102,1,0,0},
	{RACE_DWARF,CLASS_THIEF,0,0,0,0,0},
	{RACE_DWARF,CLASS_THIEF,1,2,1,0,0},
	{RACE_DWARF,CLASS_THIEF,2,4,1,0,0},
	{RACE_DWARF,CLASS_THIEF,3,6,1,0,0},
	{RACE_DWARF,CLASS_THIEF,4,8,1,0,0},
	{RACE_DWARF,CLASS_THIEF,5,10,0,0,0},
	{RACE_DWARF,CLASS_THIEF,6,12,0,0,0},
	{RACE_DWARF,CLASS_THIEF,7,14,1,0,0},
	{RACE_DWARF,CLASS_THIEF,8,16,1,0,0},
	{RACE_DWARF,CLASS_THIEF,9,18,1,0,0},
	{RACE_DWARF,CLASS_THIEF,10,20,2,0,0},
	{RACE_DWARF,CLASS_THIEF,11,22,2,0,0},
	{RACE_DWARF,CLASS_THIEF,12,24,2,0,0},
	{RACE_DWARF,CLASS_THIEF,13,26,2,0,0},
	{RACE_DWARF,CLASS_THIEF,14,28,3,0,0},
	{RACE_DWARF,CLASS_THIEF,15,30,3,0,0},
	{RACE_DWARF,CLASS_THIEF,16,32,3,0,0},
	{RACE_DWARF,CLASS_THIEF,17,34,2,0,0},
	{RACE_DWARF,CLASS_THIEF,18,36,1,0,0},
	{RACE_DWARF,CLASS_THIEF,19,38,2,0,0},
	{RACE_DWARF,CLASS_THIEF,20,40,1,0,0},
	{RACE_DWARF,CLASS_THIEF,21,42,2,0,0},
	{RACE_DWARF,CLASS_THIEF,22,44,1,0,0},
	{RACE_DWARF,CLASS_THIEF,23,46,1,0,0},
	{RACE_DWARF,CLASS_THIEF,24,48,0,0,0},
	{RACE_DWARF,CLASS_THIEF,25,50,1,0,0},
	{RACE_DWARF,CLASS_THIEF,26,52,1,0,0},
	{RACE_DWARF,CLASS_THIEF,27,54,2,0,0},
	{RACE_DWARF,CLASS_THIEF,28,56,2,0,0},
	{RACE_DWARF,CLASS_THIEF,29,58,1,0,0},
	{RACE_DWARF,CLASS_THIEF,30,60,0,0,0},
	{RACE_DWARF,CLASS_THIEF,31,62,1,0,0},
	{RACE_DWARF,CLASS_THIEF,32,64,0,0,0},
	{RACE_DWARF,CLASS_THIEF,33,66,1,0,0},
	{RACE_DWARF,CLASS_THIEF,34,68,1,0,0},
	{RACE_DWARF,CLASS_THIEF,35,70,1,0,0},
	{RACE_DWARF,CLASS_THIEF,36,72,2,0,0},
	{RACE_DWARF,CLASS_THIEF,37,74,1,0,0},
	{RACE_DWARF,CLASS_THIEF,38,76,1,0,0},
	{RACE_DWARF,CLASS_THIEF,39,78,0,0,0},
	{RACE_DWARF,CLASS_THIEF,40,80,1,0,0},
	{RACE_DWARF,CLASS_THIEF,41,82,0,0,0},
	{RACE_DWARF,CLASS_THIEF,42,84,0,0,0},
	{RACE_DWARF,CLASS_THIEF,43,86,1,0,0},
	{RACE_DWARF,CLASS_THIEF,44,88,1,0,0},
	{RACE_DWARF,CLASS_THIEF,45,90,1,0,0},
	{RACE_DWARF,CLASS_THIEF,46,92,2,0,0},
	{RACE_DWARF,CLASS_THIEF,47,94,1,0,0},
	{RACE_DWARF,CLASS_THIEF,48,96,2,0,0},
	{RACE_DWARF,CLASS_THIEF,49,98,2,0,0},
	{RACE_DWARF,CLASS_THIEF,50,100,3,0,0},
	{RACE_DWARF,CLASS_THIEF,51,102,3,0,0},
	{RACE_DWARF,CLASS_RANGER,0,0,0,0,0},
	{RACE_DWARF,CLASS_RANGER,1,2,0,0,0},
	{RACE_DWARF,CLASS_RANGER,2,4,0,0,0},
	{RACE_DWARF,CLASS_RANGER,3,6,0,0,0},
	{RACE_DWARF,CLASS_RANGER,4,8,0,0,0},
	{RACE_DWARF,CLASS_RANGER,5,10,0,0,0},
	{RACE_DWARF,CLASS_RANGER,6,12,1,0,0},
	{RACE_DWARF,CLASS_RANGER,7,14,0,0,0},
	{RACE_DWARF,CLASS_RANGER,8,16,0,0,0},
	{RACE_DWARF,CLASS_RANGER,9,18,0,0,0},
	{RACE_DWARF,CLASS_RANGER,10,20,0,0,0},
	{RACE_DWARF,CLASS_RANGER,11,22,1,0,0},
	{RACE_DWARF,CLASS_RANGER,12,24,0,0,0},
	{RACE_DWARF,CLASS_RANGER,13,26,1,0,0},
	{RACE_DWARF,CLASS_RANGER,14,28,2,0,0},
	{RACE_DWARF,CLASS_RANGER,15,30,3,0,0},
	{RACE_DWARF,CLASS_RANGER,16,32,4,0,0},
	{RACE_DWARF,CLASS_RANGER,17,34,5,0,0},
	{RACE_DWARF,CLASS_RANGER,18,36,5,0,0},
	{RACE_DWARF,CLASS_RANGER,19,38,5,0,0},
	{RACE_DWARF,CLASS_RANGER,20,40,4,0,0},
	{RACE_DWARF,CLASS_RANGER,21,42,5,0,0},
	{RACE_DWARF,CLASS_RANGER,22,44,5,0,0},
	{RACE_DWARF,CLASS_RANGER,23,46,5,0,0},
	{RACE_DWARF,CLASS_RANGER,24,48,6,0,0},
	{RACE_DWARF,CLASS_RANGER,25,50,5,0,0},
	{RACE_DWARF,CLASS_RANGER,26,52,5,0,0},
	{RACE_DWARF,CLASS_RANGER,27,54,5,0,0},
	{RACE_DWARF,CLASS_RANGER,28,56,6,0,0},
	{RACE_DWARF,CLASS_RANGER,29,58,7,0,0},
	{RACE_DWARF,CLASS_RANGER,30,60,7,0,0},
	{RACE_DWARF,CLASS_RANGER,31,62,7,0,0},
	{RACE_DWARF,CLASS_RANGER,32,64,8,0,0},
	{RACE_DWARF,CLASS_RANGER,33,66,8,0,0},
	{RACE_DWARF,CLASS_RANGER,34,68,8,0,0},
	{RACE_DWARF,CLASS_RANGER,35,70,9,0,0},
	{RACE_DWARF,CLASS_RANGER,36,72,8,0,0},
	{RACE_DWARF,CLASS_RANGER,37,74,7,0,0},
	{RACE_DWARF,CLASS_RANGER,38,76,8,0,0},
	{RACE_DWARF,CLASS_RANGER,39,78,9,0,0},
	{RACE_DWARF,CLASS_RANGER,40,80,10,0,0},
	{RACE_DWARF,CLASS_RANGER,41,82,11,0,0},
	{RACE_DWARF,CLASS_RANGER,42,84,12,0,0},
	{RACE_DWARF,CLASS_RANGER,43,86,11,0,0},
	{RACE_DWARF,CLASS_RANGER,44,88,12,0,0},
	{RACE_DWARF,CLASS_RANGER,45,90,12,0,0},
	{RACE_DWARF,CLASS_RANGER,46,92,13,0,0},
	{RACE_DWARF,CLASS_RANGER,47,94,13,0,0},
	{RACE_DWARF,CLASS_RANGER,48,96,13,0,0},
	{RACE_DWARF,CLASS_RANGER,49,98,14,0,0},
	{RACE_DWARF,CLASS_RANGER,50,100,14,0,0},
	{RACE_DWARF,CLASS_RANGER,51,102,13,0,0},
	{RACE_DWARF,CLASS_MAGE,0,0,0,0,0},
	{RACE_DWARF,CLASS_MAGE,1,2,0,0,0},
	{RACE_DWARF,CLASS_MAGE,2,4,1,0,0},
	{RACE_DWARF,CLASS_MAGE,3,6,1,0,0},
	{RACE_DWARF,CLASS_MAGE,4,8,2,0,0},
	{RACE_DWARF,CLASS_MAGE,5,11,3,0,0},
	{RACE_DWARF,CLASS_MAGE,6,14,4,0,0},
	{RACE_DWARF,CLASS_MAGE,7,16,5,0,0},
	{RACE_DWARF,CLASS_MAGE,8,18,5,0,0},
	{RACE_DWARF,CLASS_MAGE,9,20,5,0,0},
	{RACE_DWARF,CLASS_MAGE,10,22,6,0,0},
	{RACE_DWARF,CLASS_MAGE,11,25,7,0,0},
	{RACE_DWARF,CLASS_MAGE,12,28,8,0,0},
	{RACE_DWARF,CLASS_MAGE,13,30,9,0,0},
	{RACE_DWARF,CLASS_MAGE,14,33,10,0,0},
	{RACE_DWARF,CLASS_MAGE,15,35,10,0,0},
	{RACE_DWARF,CLASS_MAGE,16,38,11,0,0},
	{RACE_DWARF,CLASS_MAGE,17,40,12,0,0},
	{RACE_DWARF,CLASS_MAGE,18,42,13,0,0},
	{RACE_DWARF,CLASS_MAGE,19,45,14,0,0},
	{RACE_DWARF,CLASS_MAGE,20,46,14,0,0},
	{RACE_DWARF,CLASS_MAGE,21,48,15,0,0},
	{RACE_DWARF,CLASS_MAGE,22,50,15,0,0},
	{RACE_DWARF,CLASS_MAGE,23,53,16,0,0},
	{RACE_DWARF,CLASS_MAGE,24,55,17,0,0},
	{RACE_DWARF,CLASS_MAGE,25,57,17,0,0},
	{RACE_DWARF,CLASS_MAGE,26,59,18,0,0},
	{RACE_DWARF,CLASS_MAGE,27,61,18,0,0},
	{RACE_DWARF,CLASS_MAGE,28,63,19,0,0},
	{RACE_DWARF,CLASS_MAGE,29,65,19,0,0},
	{RACE_DWARF,CLASS_MAGE,30,67,20,0,0},
	{RACE_DWARF,CLASS_MAGE,31,69,20,0,0},
	{RACE_DWARF,CLASS_MAGE,32,71,21,0,0},
	{RACE_DWARF,CLASS_MAGE,33,73,22,0,0},
	{RACE_DWARF,CLASS_MAGE,34,75,23,0,0},
	{RACE_DWARF,CLASS_MAGE,35,77,23,0,0},
	{RACE_DWARF,CLASS_MAGE,36,80,24,0,0},
	{RACE_DWARF,CLASS_MAGE,37,81,24,0,0},
	{RACE_DWARF,CLASS_MAGE,38,83,24,0,0},
	{RACE_DWARF,CLASS_MAGE,39,85,25,0,0},
	{RACE_DWARF,CLASS_MAGE,40,87,26,0,0},
	{RACE_DWARF,CLASS_MAGE,41,88,26,0,0},
	{RACE_DWARF,CLASS_MAGE,42,89,27,0,0},
	{RACE_DWARF,CLASS_MAGE,43,91,28,0,0},
	{RACE_DWARF,CLASS_MAGE,44,93,28,0,0},
	{RACE_DWARF,CLASS_MAGE,45,95,29,0,0},
	{RACE_DWARF,CLASS_MAGE,46,97,30,0,0},
	{RACE_DWARF,CLASS_MAGE,47,99,31,0,0},
	{RACE_DWARF,CLASS_MAGE,48,101,32,0,0},
	{RACE_DWARF,CLASS_MAGE,49,103,33,0,0},
	{RACE_DWARF,CLASS_MAGE,50,106,33,0,0},
	{RACE_DWARF,CLASS_MAGE,51,108,33,0,0},
	{RACE_DWARF,CLASS_ALCHEMIST,0,0,0,0,0},
	{RACE_DWARF,CLASS_ALCHEMIST,1,2,0,0,1},
	{RACE_DWARF,CLASS_ALCHEMIST,2,4,0,0,2},
	{RACE_DWARF,CLASS_ALCHEMIST,3,6,0,0,3},
	{RACE_DWARF,CLASS_ALCHEMIST,4,8,0,0,4},
	{RACE_DWARF,CLASS_ALCHEMIST,5,10,0,0,5},
	{RACE_DWARF,CLASS_ALCHEMIST,6,12,0,0,6},
	{RACE_DWARF,CLASS_ALCHEMIST,7,14,0,0,7},
	{RACE_DWARF,CLASS_ALCHEMIST,8,16,0,0,8},
	{RACE_DWARF,CLASS_ALCHEMIST,9,18,0,0,9},
	{RACE_DWARF,CLASS_ALCHEMIST,10,20,0,0,10},
	{RACE_DWARF,CLASS_ALCHEMIST,11,22,0,0,11},
	{RACE_DWARF,CLASS_ALCHEMIST,12,24,0,0,12},
	{RACE_DWARF,CLASS_ALCHEMIST,13,26,0,0,13},
	{RACE_DWARF,CLASS_ALCHEMIST,14,28,0,0,14},
	{RACE_DWARF,CLASS_ALCHEMIST,15,30,0,0,15},
	{RACE_DWARF,CLASS_ALCHEMIST,16,32,0,0,16},
	{RACE_DWARF,CLASS_ALCHEMIST,17,34,0,0,17},
	{RACE_DWARF,CLASS_ALCHEMIST,18,36,0,0,18},
	{RACE_DWARF,CLASS_ALCHEMIST,19,38,0,0,19},
	{RACE_DWARF,CLASS_ALCHEMIST,20,40,0,0,20},
	{RACE_DWARF,CLASS_ALCHEMIST,21,42,0,0,21},
	{RACE_DWARF,CLASS_ALCHEMIST,22,44,0,0,22},
	{RACE_DWARF,CLASS_ALCHEMIST,23,46,0,0,23},
	{RACE_DWARF,CLASS_ALCHEMIST,24,48,0,0,24},
	{RACE_DWARF,CLASS_ALCHEMIST,25,50,0,0,25},
	{RACE_DWARF,CLASS_ALCHEMIST,26,52,0,0,26},
	{RACE_DWARF,CLASS_ALCHEMIST,27,54,0,0,27},
	{RACE_DWARF,CLASS_ALCHEMIST,28,56,0,0,28},
	{RACE_DWARF,CLASS_ALCHEMIST,29,58,0,0,29},
	{RACE_DWARF,CLASS_ALCHEMIST,30,60,0,0,30},
	{RACE_DWARF,CLASS_ALCHEMIST,31,62,0,0,31},
	{RACE_DWARF,CLASS_ALCHEMIST,32,64,0,0,32},
	{RACE_DWARF,CLASS_ALCHEMIST,33,66,0,0,33},
	{RACE_DWARF,CLASS_ALCHEMIST,34,68,0,0,34},
	{RACE_DWARF,CLASS_ALCHEMIST,35,70,0,0,35},
	{RACE_DWARF,CLASS_ALCHEMIST,36,72,0,0,36},
	{RACE_DWARF,CLASS_ALCHEMIST,37,74,0,0,37},
	{RACE_DWARF,CLASS_ALCHEMIST,38,76,0,0,38},
	{RACE_DWARF,CLASS_ALCHEMIST,39,78,0,0,39},
	{RACE_DWARF,CLASS_ALCHEMIST,40,80,0,0,40},
	{RACE_DWARF,CLASS_ALCHEMIST,41,82,0,0,41},
	{RACE_DWARF,CLASS_ALCHEMIST,42,84,0,0,42},
	{RACE_DWARF,CLASS_ALCHEMIST,43,86,0,0,43},
	{RACE_DWARF,CLASS_ALCHEMIST,44,88,0,0,44},
	{RACE_DWARF,CLASS_ALCHEMIST,45,90,0,0,45},
	{RACE_DWARF,CLASS_ALCHEMIST,46,92,0,0,46},
	{RACE_DWARF,CLASS_ALCHEMIST,47,94,0,0,47},
	{RACE_DWARF,CLASS_ALCHEMIST,48,96,0,0,48},
	{RACE_DWARF,CLASS_ALCHEMIST,49,98,0,0,49},
	{RACE_DWARF,CLASS_ALCHEMIST,50,100,0,0,50},
	{RACE_DWARF,CLASS_ALCHEMIST,51,102,0,0,51},
	{RACE_DWARF,CLASS_PALADIN,0,0,0,0,0},
	{RACE_DWARF,CLASS_PALADIN,1,1,0,0,1},
	{RACE_DWARF,CLASS_PALADIN,2,2,0,0,2},
	{RACE_DWARF,CLASS_PALADIN,3,4,0,0,3},
	{RACE_DWARF,CLASS_PALADIN,4,5,0,0,4},
	{RACE_DWARF,CLASS_PALADIN,5,7,0,0,5},
	{RACE_DWARF,CLASS_PALADIN,6,9,0,0,6},
	{RACE_DWARF,CLASS_PALADIN,7,10,0,0,7},
	{RACE_DWARF,CLASS_PALADIN,8,12,1,0,8},
	{RACE_DWARF,CLASS_PALADIN,9,14,1,0,9},
	{RACE_DWARF,CLASS_PALADIN,10,15,1,0,10},
	{RACE_DWARF,CLASS_PALADIN,11,17,1,0,11},
	{RACE_DWARF,CLASS_PALADIN,12,19,1,0,12},
	{RACE_DWARF,CLASS_PALADIN,13,21,1,0,13},
	{RACE_DWARF,CLASS_PALADIN,14,23,1,0,14},
	{RACE_DWARF,CLASS_PALADIN,15,24,1,0,15},
	{RACE_DWARF,CLASS_PALADIN,16,26,1,0,16},
	{RACE_DWARF,CLASS_PALADIN,17,27,1,0,17},
	{RACE_DWARF,CLASS_PALADIN,18,29,1,0,18},
	{RACE_DWARF,CLASS_PALADIN,19,30,1,0,19},
	{RACE_DWARF,CLASS_PALADIN,20,32,1,0,20},
	{RACE_DWARF,CLASS_PALADIN,21,34,1,0,21},
	{RACE_DWARF,CLASS_PALADIN,22,36,1,0,22},
	{RACE_DWARF,CLASS_PALADIN,23,37,1,0,23},
	{RACE_DWARF,CLASS_PALADIN,24,39,1,0,24},
	{RACE_DWARF,CLASS_PALADIN,25,41,1,0,25},
	{RACE_DWARF,CLASS_PALADIN,26,43,1,0,26},
	{RACE_DWARF,CLASS_PALADIN,27,45,1,0,27},
	{RACE_DWARF,CLASS_PALADIN,28,47,1,0,28},
	{RACE_DWARF,CLASS_PALADIN,29,48,1,0,29},
	{RACE_DWARF,CLASS_PALADIN,30,50,1,0,30},
	{RACE_DWARF,CLASS_PALADIN,31,52,1,0,31},
	{RACE_DWARF,CLASS_PALADIN,32,54,1,0,32},
	{RACE_DWARF,CLASS_PALADIN,33,56,1,0,33},
	{RACE_DWARF,CLASS_PALADIN,34,58,1,0,34},
	{RACE_DWARF,CLASS_PALADIN,35,60,1,0,35},
	{RACE_DWARF,CLASS_PALADIN,36,61,1,0,36},
	{RACE_DWARF,CLASS_PALADIN,37,63,1,0,37},
	{RACE_DWARF,CLASS_PALADIN,38,65,1,0,38},
	{RACE_DWARF,CLASS_PALADIN,39,67,1,0,39},
	{RACE_DWARF,CLASS_PALADIN,40,69,1,0,40},
	{RACE_DWARF,CLASS_PALADIN,41,70,1,0,41},
	{RACE_DWARF,CLASS_PALADIN,42,72,1,0,42},
	{RACE_DWARF,CLASS_PALADIN,43,73,1,0,43},
	{RACE_DWARF,CLASS_PALADIN,44,75,1,0,44},
	{RACE_DWARF,CLASS_PALADIN,45,76,1,0,45},
	{RACE_DWARF,CLASS_PALADIN,46,78,1,0,46},
	{RACE_DWARF,CLASS_PALADIN,47,80,1,0,47},
	{RACE_DWARF,CLASS_PALADIN,48,82,1,0,48},
	{RACE_DWARF,CLASS_PALADIN,49,84,1,0,49},
	{RACE_DWARF,CLASS_PALADIN,50,86,1,0,50},
	{RACE_DWARF,CLASS_PALADIN,51,88,1,0,51},
	{RACE_DROW,CLASS_WARRIOR,0,0,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,1,0,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,2,0,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,3,0,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,4,0,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,5,0,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,6,1,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,7,1,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,8,1,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,9,0,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,10,0,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,11,0,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,12,0,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,13,0,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,14,1,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,15,1,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,16,1,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,17,0,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,18,0,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,19,1,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,20,2,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,21,2,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,22,2,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,23,2,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,24,2,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,25,2,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,26,1,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,27,1,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,28,1,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,29,2,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,30,1,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,31,1,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,32,2,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,33,3,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,34,3,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,35,3,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,36,3,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,37,3,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,38,3,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,39,4,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,40,3,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,41,3,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,42,3,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,43,3,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,44,2,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,45,1,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,46,1,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,47,2,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,48,2,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,49,2,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,50,3,0,0,0},
	{RACE_DROW,CLASS_WARRIOR,51,3,0,0,0},
	{RACE_DROW,CLASS_NECROMANT,0,0,0,0,0},
	{RACE_DROW,CLASS_NECROMANT,1,0,0,0,0},
	{RACE_DROW,CLASS_NECROMANT,2,0,0,0,0},
	{RACE_DROW,CLASS_NECROMANT,3,0,1,0,0},
	{RACE_DROW,CLASS_NECROMANT,4,0,1,0,0},
	{RACE_DROW,CLASS_NECROMANT,5,0,1,0,0},
	{RACE_DROW,CLASS_NECROMANT,6,0,1,0,0},
	{RACE_DROW,CLASS_NECROMANT,7,0,2,0,0},
	{RACE_DROW,CLASS_NECROMANT,8,0,2,0,0},
	{RACE_DROW,CLASS_NECROMANT,9,0,1,0,0},
	{RACE_DROW,CLASS_NECROMANT,10,0,1,0,0},
	{RACE_DROW,CLASS_NECROMANT,11,0,2,0,0},
	{RACE_DROW,CLASS_NECROMANT,12,0,3,0,0},
	{RACE_DROW,CLASS_NECROMANT,13,0,2,0,0},
	{RACE_DROW,CLASS_NECROMANT,14,0,2,0,0},
	{RACE_DROW,CLASS_NECROMANT,15,0,2,0,0},
	{RACE_DROW,CLASS_NECROMANT,16,0,3,0,0},
	{RACE_DROW,CLASS_NECROMANT,17,0,3,0,0},
	{RACE_DROW,CLASS_NECROMANT,18,0,4,0,0},
	{RACE_DROW,CLASS_NECROMANT,19,0,4,0,0},
	{RACE_DROW,CLASS_NECROMANT,20,0,3,0,0},
	{RACE_DROW,CLASS_NECROMANT,21,0,2,0,0},
	{RACE_DROW,CLASS_NECROMANT,22,0,2,0,0},
	{RACE_DROW,CLASS_NECROMANT,23,0,1,0,0},
	{RACE_DROW,CLASS_NECROMANT,24,0,1,0,0},
	{RACE_DROW,CLASS_NECROMANT,25,0,1,0,0},
	{RACE_DROW,CLASS_NECROMANT,26,0,2,0,0},
	{RACE_DROW,CLASS_NECROMANT,27,0,2,0,0},
	{RACE_DROW,CLASS_NECROMANT,28,0,2,0,0},
	{RACE_DROW,CLASS_NECROMANT,29,0,2,0,0},
	{RACE_DROW,CLASS_NECROMANT,30,0,1,0,0},
	{RACE_DROW,CLASS_NECROMANT,31,0,1,0,0},
	{RACE_DROW,CLASS_NECROMANT,32,0,1,0,0},
	{RACE_DROW,CLASS_NECROMANT,33,0,1,0,0},
	{RACE_DROW,CLASS_NECROMANT,34,0,0,0,0},
	{RACE_DROW,CLASS_NECROMANT,35,0,0,0,0},
	{RACE_DROW,CLASS_NECROMANT,36,0,1,0,0},
	{RACE_DROW,CLASS_NECROMANT,37,0,0,0,0},
	{RACE_DROW,CLASS_NECROMANT,38,0,0,0,0},
	{RACE_DROW,CLASS_NECROMANT,39,0,1,0,0},
	{RACE_DROW,CLASS_NECROMANT,40,0,0,0,0},
	{RACE_DROW,CLASS_NECROMANT,41,0,0,0,0},
	{RACE_DROW,CLASS_NECROMANT,42,0,1,0,0},
	{RACE_DROW,CLASS_NECROMANT,43,0,1,0,0},
	{RACE_DROW,CLASS_NECROMANT,44,0,2,0,0},
	{RACE_DROW,CLASS_NECROMANT,45,0,2,0,0},
	{RACE_DROW,CLASS_NECROMANT,46,0,2,0,0},
	{RACE_DROW,CLASS_NECROMANT,47,0,2,0,0},
	{RACE_DROW,CLASS_NECROMANT,48,0,2,0,0},
	{RACE_DROW,CLASS_NECROMANT,49,0,2,0,0},
	{RACE_DROW,CLASS_NECROMANT,50,0,2,0,0},
	{RACE_DROW,CLASS_NECROMANT,51,0,1,0,0},
	{RACE_DROW,CLASS_DRUID,0,0,0,0,0},
	{RACE_DROW,CLASS_DRUID,1,0,1,0,0},
	{RACE_DROW,CLASS_DRUID,2,0,2,0,0},
	{RACE_DROW,CLASS_DRUID,3,1,3,0,0},
	{RACE_DROW,CLASS_DRUID,4,1,4,0,0},
	{RACE_DROW,CLASS_DRUID,5,1,5,0,0},
	{RACE_DROW,CLASS_DRUID,6,1,6,0,0},
	{RACE_DROW,CLASS_DRUID,7,1,6,0,0},
	{RACE_DROW,CLASS_DRUID,8,1,7,0,0},
	{RACE_DROW,CLASS_DRUID,9,1,8,0,0},
	{RACE_DROW,CLASS_DRUID,10,2,8,0,0},
	{RACE_DROW,CLASS_DRUID,11,1,9,0,0},
	{RACE_DROW,CLASS_DRUID,12,2,9,0,0},
	{RACE_DROW,CLASS_DRUID,13,2,9,0,0},
	{RACE_DROW,CLASS_DRUID,14,2,10,0,0},
	{RACE_DROW,CLASS_DRUID,15,3,10,0,0},
	{RACE_DROW,CLASS_DRUID,16,2,10,0,0},
	{RACE_DROW,CLASS_DRUID,17,2,11,0,0},
	{RACE_DROW,CLASS_DRUID,18,2,11,0,0},
	{RACE_DROW,CLASS_DRUID,19,2,11,0,0},
	{RACE_DROW,CLASS_DRUID,20,2,11,0,0},
	{RACE_DROW,CLASS_DRUID,21,3,11,0,0},
	{RACE_DROW,CLASS_DRUID,22,3,11,0,0},
	{RACE_DROW,CLASS_DRUID,23,4,12,0,0},
	{RACE_DROW,CLASS_DRUID,24,4,13,0,0},
	{RACE_DROW,CLASS_DRUID,25,3,14,0,0},
	{RACE_DROW,CLASS_DRUID,26,4,15,0,0},
	{RACE_DROW,CLASS_DRUID,27,4,16,0,0},
	{RACE_DROW,CLASS_DRUID,28,3,16,0,0},
	{RACE_DROW,CLASS_DRUID,29,3,16,0,0},
	{RACE_DROW,CLASS_DRUID,30,3,16,0,0},
	{RACE_DROW,CLASS_DRUID,31,3,17,0,0},
	{RACE_DROW,CLASS_DRUID,32,2,17,0,0},
	{RACE_DROW,CLASS_DRUID,33,2,17,0,0},
	{RACE_DROW,CLASS_DRUID,34,1,17,0,0},
	{RACE_DROW,CLASS_DRUID,35,1,17,0,0},
	{RACE_DROW,CLASS_DRUID,36,2,17,0,0},
	{RACE_DROW,CLASS_DRUID,37,2,18,0,0},
	{RACE_DROW,CLASS_DRUID,38,1,18,0,0},
	{RACE_DROW,CLASS_DRUID,39,0,19,0,0},
	{RACE_DROW,CLASS_DRUID,40,0,20,0,0},
	{RACE_DROW,CLASS_DRUID,41,0,21,0,0},
	{RACE_DROW,CLASS_DRUID,42,0,22,0,0},
	{RACE_DROW,CLASS_DRUID,43,0,23,0,0},
	{RACE_DROW,CLASS_DRUID,44,0,24,0,0},
	{RACE_DROW,CLASS_DRUID,45,1,24,0,0},
	{RACE_DROW,CLASS_DRUID,46,1,25,0,0},
	{RACE_DROW,CLASS_DRUID,47,1,25,0,0},
	{RACE_DROW,CLASS_DRUID,48,1,25,0,0},
	{RACE_DROW,CLASS_DRUID,49,1,26,0,0},
	{RACE_DROW,CLASS_DRUID,50,0,27,0,0},
	{RACE_DROW,CLASS_DRUID,51,0,28,0,0},
	{RACE_DROW,CLASS_CLERIC,0,0,0,0,0},
	{RACE_DROW,CLASS_CLERIC,1,1,1,0,0},
	{RACE_DROW,CLASS_CLERIC,2,1,3,0,0},
	{RACE_DROW,CLASS_CLERIC,3,2,4,0,0},
	{RACE_DROW,CLASS_CLERIC,4,3,6,0,0},
	{RACE_DROW,CLASS_CLERIC,5,3,8,0,0},
	{RACE_DROW,CLASS_CLERIC,6,3,10,0,0},
	{RACE_DROW,CLASS_CLERIC,7,4,12,0,0},
	{RACE_DROW,CLASS_CLERIC,8,4,14,0,0},
	{RACE_DROW,CLASS_CLERIC,9,4,15,0,0},
	{RACE_DROW,CLASS_CLERIC,10,5,17,0,0},
	{RACE_DROW,CLASS_CLERIC,11,4,18,0,0},
	{RACE_DROW,CLASS_CLERIC,12,4,19,0,0},
	{RACE_DROW,CLASS_CLERIC,13,4,19,0,0},
	{RACE_DROW,CLASS_CLERIC,14,4,20,0,0},
	{RACE_DROW,CLASS_CLERIC,15,3,20,0,0},
	{RACE_DROW,CLASS_CLERIC,16,2,20,0,0},
	{RACE_DROW,CLASS_CLERIC,17,3,20,0,0},
	{RACE_DROW,CLASS_CLERIC,18,3,20,0,0},
	{RACE_DROW,CLASS_CLERIC,19,3,21,0,0},
	{RACE_DROW,CLASS_CLERIC,20,3,22,0,0},
	{RACE_DROW,CLASS_CLERIC,21,4,23,0,0},
	{RACE_DROW,CLASS_CLERIC,22,4,25,0,0},
	{RACE_DROW,CLASS_CLERIC,23,4,27,0,0},
	{RACE_DROW,CLASS_CLERIC,24,5,28,0,0},
	{RACE_DROW,CLASS_CLERIC,25,5,28,0,0},
	{RACE_DROW,CLASS_CLERIC,26,5,29,0,0},
	{RACE_DROW,CLASS_CLERIC,27,5,30,0,0},
	{RACE_DROW,CLASS_CLERIC,28,5,31,0,0},
	{RACE_DROW,CLASS_CLERIC,29,4,33,0,0},
	{RACE_DROW,CLASS_CLERIC,30,4,34,0,0},
	{RACE_DROW,CLASS_CLERIC,31,3,35,0,0},
	{RACE_DROW,CLASS_CLERIC,32,3,36,0,0},
	{RACE_DROW,CLASS_CLERIC,33,2,37,0,0},
	{RACE_DROW,CLASS_CLERIC,34,1,38,0,0},
	{RACE_DROW,CLASS_CLERIC,35,0,38,0,0},
	{RACE_DROW,CLASS_CLERIC,36,0,40,0,0},
	{RACE_DROW,CLASS_CLERIC,37,1,42,0,0},
	{RACE_DROW,CLASS_CLERIC,38,2,44,0,0},
	{RACE_DROW,CLASS_CLERIC,39,3,44,0,0},
	{RACE_DROW,CLASS_CLERIC,40,3,44,0,0},
	{RACE_DROW,CLASS_CLERIC,41,2,44,0,0},
	{RACE_DROW,CLASS_CLERIC,42,2,46,0,0},
	{RACE_DROW,CLASS_CLERIC,43,2,47,0,0},
	{RACE_DROW,CLASS_CLERIC,44,2,48,0,0},
	{RACE_DROW,CLASS_CLERIC,45,3,49,0,0},
	{RACE_DROW,CLASS_CLERIC,46,4,49,0,0},
	{RACE_DROW,CLASS_CLERIC,47,4,50,0,0},
	{RACE_DROW,CLASS_CLERIC,48,4,50,0,0},
	{RACE_DROW,CLASS_CLERIC,49,4,52,0,0},
	{RACE_DROW,CLASS_CLERIC,50,5,53,0,0},
	{RACE_DROW,CLASS_CLERIC,51,5,54,0,0},
	{RACE_DROW,CLASS_THIEF,0,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,1,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,2,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,3,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,4,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,5,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,6,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,7,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,8,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,9,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,10,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,11,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,12,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,13,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,14,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,15,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,16,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,17,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,18,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,19,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,20,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,21,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,22,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,23,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,24,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,25,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,26,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,27,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,28,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,29,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,30,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,31,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,32,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,33,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,34,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,35,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,36,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,37,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,38,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,39,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,40,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,41,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,42,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,43,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,44,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,45,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,46,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,47,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,48,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,49,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,50,0,0,0,0},
	{RACE_DROW,CLASS_THIEF,51,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,0,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,1,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,2,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,3,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,4,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,5,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,6,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,7,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,8,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,9,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,10,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,11,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,12,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,13,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,14,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,15,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,16,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,17,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,18,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,19,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,20,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,21,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,22,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,23,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,24,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,25,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,26,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,27,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,28,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,29,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,30,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,31,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,32,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,33,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,34,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,35,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,36,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,37,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,38,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,39,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,40,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,41,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,42,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,43,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,44,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,45,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,46,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,47,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,48,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,49,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,50,0,0,0,0},
	{RACE_DROW,CLASS_RANGER,51,0,0,0,0},
	{RACE_DROW,CLASS_NAZGUL,0,0,0,0,0},
	{RACE_DROW,CLASS_NAZGUL,1,0,0,0,0},
	{RACE_DROW,CLASS_NAZGUL,2,0,1,0,0},
	{RACE_DROW,CLASS_NAZGUL,3,0,1,0,0},
	{RACE_DROW,CLASS_NAZGUL,4,0,2,0,0},
	{RACE_DROW,CLASS_NAZGUL,5,0,3,0,0},
	{RACE_DROW,CLASS_NAZGUL,6,0,4,0,0},
	{RACE_DROW,CLASS_NAZGUL,7,0,4,0,0},
	{RACE_DROW,CLASS_NAZGUL,8,0,4,0,0},
	{RACE_DROW,CLASS_NAZGUL,9,0,4,0,0},
	{RACE_DROW,CLASS_NAZGUL,10,0,4,0,0},
	{RACE_DROW,CLASS_NAZGUL,11,0,5,0,0},
	{RACE_DROW,CLASS_NAZGUL,12,0,6,0,0},
	{RACE_DROW,CLASS_NAZGUL,13,0,6,0,0},
	{RACE_DROW,CLASS_NAZGUL,14,0,6,0,0},
	{RACE_DROW,CLASS_NAZGUL,15,0,6,0,0},
	{RACE_DROW,CLASS_NAZGUL,16,0,7,0,0},
	{RACE_DROW,CLASS_NAZGUL,17,0,7,0,0},
	{RACE_DROW,CLASS_NAZGUL,18,0,7,0,0},
	{RACE_DROW,CLASS_NAZGUL,19,0,7,0,0},
	{RACE_DROW,CLASS_NAZGUL,20,0,7,0,0},
	{RACE_DROW,CLASS_NAZGUL,21,0,7,0,0},
	{RACE_DROW,CLASS_NAZGUL,22,0,7,0,0},
	{RACE_DROW,CLASS_NAZGUL,23,0,7,0,0},
	{RACE_DROW,CLASS_NAZGUL,24,0,7,0,0},
	{RACE_DROW,CLASS_NAZGUL,25,0,7,0,0},
	{RACE_DROW,CLASS_NAZGUL,26,0,7,0,0},
	{RACE_DROW,CLASS_NAZGUL,27,0,7,0,0},
	{RACE_DROW,CLASS_NAZGUL,28,0,8,0,0},
	{RACE_DROW,CLASS_NAZGUL,29,0,8,0,0},
	{RACE_DROW,CLASS_NAZGUL,30,0,8,0,0},
	{RACE_DROW,CLASS_NAZGUL,31,0,8,0,0},
	{RACE_DROW,CLASS_NAZGUL,32,0,9,0,0},
	{RACE_DROW,CLASS_NAZGUL,33,0,10,0,0},
	{RACE_DROW,CLASS_NAZGUL,34,0,11,0,0},
	{RACE_DROW,CLASS_NAZGUL,35,0,12,0,0},
	{RACE_DROW,CLASS_NAZGUL,36,0,12,0,0},
	{RACE_DROW,CLASS_NAZGUL,37,0,13,0,0},
	{RACE_DROW,CLASS_NAZGUL,38,0,13,0,0},
	{RACE_DROW,CLASS_NAZGUL,39,0,14,0,0},
	{RACE_DROW,CLASS_NAZGUL,40,0,15,0,0},
	{RACE_DROW,CLASS_NAZGUL,41,0,15,0,0},
	{RACE_DROW,CLASS_NAZGUL,42,0,16,0,0},
	{RACE_DROW,CLASS_NAZGUL,43,0,16,0,0},
	{RACE_DROW,CLASS_NAZGUL,44,0,17,0,0},
	{RACE_DROW,CLASS_NAZGUL,45,0,17,0,0},
	{RACE_DROW,CLASS_NAZGUL,46,0,17,0,0},
	{RACE_DROW,CLASS_NAZGUL,47,0,18,0,0},
	{RACE_DROW,CLASS_NAZGUL,48,0,19,0,0},
	{RACE_DROW,CLASS_NAZGUL,49,0,19,0,0},
	{RACE_DROW,CLASS_NAZGUL,50,0,19,0,0},
	{RACE_DROW,CLASS_NAZGUL,51,0,20,0,0},
	{RACE_DROW,CLASS_MAGE,0,0,0,0,0},
	{RACE_DROW,CLASS_MAGE,1,0,0,0,0},
	{RACE_DROW,CLASS_MAGE,2,0,0,0,0},
	{RACE_DROW,CLASS_MAGE,3,1,0,0,0},
	{RACE_DROW,CLASS_MAGE,4,1,1,0,0},
	{RACE_DROW,CLASS_MAGE,5,0,2,0,0},
	{RACE_DROW,CLASS_MAGE,6,0,2,0,0},
	{RACE_DROW,CLASS_MAGE,7,1,3,0,0},
	{RACE_DROW,CLASS_MAGE,8,2,3,0,0},
	{RACE_DROW,CLASS_MAGE,9,2,4,0,0},
	{RACE_DROW,CLASS_MAGE,10,2,4,0,0},
	{RACE_DROW,CLASS_MAGE,11,1,3,0,0},
	{RACE_DROW,CLASS_MAGE,12,1,3,0,0},
	{RACE_DROW,CLASS_MAGE,13,2,2,0,0},
	{RACE_DROW,CLASS_MAGE,14,2,2,0,0},
	{RACE_DROW,CLASS_MAGE,15,2,2,0,0},
	{RACE_DROW,CLASS_MAGE,16,1,2,0,0},
	{RACE_DROW,CLASS_MAGE,17,1,1,0,0},
	{RACE_DROW,CLASS_MAGE,18,2,2,0,0},
	{RACE_DROW,CLASS_MAGE,19,2,3,0,0},
	{RACE_DROW,CLASS_MAGE,20,3,4,0,0},
	{RACE_DROW,CLASS_MAGE,21,4,3,0,0},
	{RACE_DROW,CLASS_MAGE,22,4,3,0,0},
	{RACE_DROW,CLASS_MAGE,23,4,3,0,0},
	{RACE_DROW,CLASS_MAGE,24,5,2,0,0},
	{RACE_DROW,CLASS_MAGE,25,4,3,0,0},
	{RACE_DROW,CLASS_MAGE,26,5,2,0,0},
	{RACE_DROW,CLASS_MAGE,27,4,2,0,0},
	{RACE_DROW,CLASS_MAGE,28,4,2,0,0},
	{RACE_DROW,CLASS_MAGE,29,3,2,0,0},
	{RACE_DROW,CLASS_MAGE,30,3,2,0,0},
	{RACE_DROW,CLASS_MAGE,31,3,2,0,0},
	{RACE_DROW,CLASS_MAGE,32,4,2,0,0},
	{RACE_DROW,CLASS_MAGE,33,4,1,0,0},
	{RACE_DROW,CLASS_MAGE,34,4,0,0,0},
	{RACE_DROW,CLASS_MAGE,35,4,1,0,0},
	{RACE_DROW,CLASS_MAGE,36,5,1,0,0},
	{RACE_DROW,CLASS_MAGE,37,5,1,0,0},
	{RACE_DROW,CLASS_MAGE,38,5,1,0,0},
	{RACE_DROW,CLASS_MAGE,39,5,1,0,0},
	{RACE_DROW,CLASS_MAGE,40,6,0,0,0},
	{RACE_DROW,CLASS_MAGE,41,5,1,0,0},
	{RACE_DROW,CLASS_MAGE,42,5,1,0,0},
	{RACE_DROW,CLASS_MAGE,43,5,1,0,0},
	{RACE_DROW,CLASS_MAGE,44,5,2,0,0},
	{RACE_DROW,CLASS_MAGE,45,5,2,0,0},
	{RACE_DROW,CLASS_MAGE,46,5,3,0,0},
	{RACE_DROW,CLASS_MAGE,47,6,3,0,0},
	{RACE_DROW,CLASS_MAGE,48,6,4,0,0},
	{RACE_DROW,CLASS_MAGE,49,6,5,0,0},
	{RACE_DROW,CLASS_MAGE,50,6,5,0,0},
	{RACE_DROW,CLASS_MAGE,51,6,4,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,0,0,0,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,1,0,1,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,2,0,2,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,3,0,2,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,4,0,2,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,5,0,2,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,6,0,2,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,7,0,2,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,8,0,3,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,9,0,3,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,10,0,4,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,11,0,5,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,12,0,6,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,13,0,7,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,14,0,8,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,15,0,9,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,16,0,9,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,17,0,9,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,18,0,10,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,19,0,10,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,20,0,10,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,21,0,11,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,22,0,11,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,23,0,11,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,24,0,12,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,25,0,13,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,26,0,14,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,27,0,14,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,28,0,15,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,29,0,15,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,30,0,16,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,31,0,17,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,32,0,18,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,33,0,18,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,34,0,18,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,35,0,19,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,36,0,20,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,37,0,20,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,38,0,20,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,39,0,20,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,40,0,21,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,41,0,22,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,42,0,22,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,43,0,23,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,44,0,24,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,45,0,25,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,46,0,26,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,47,0,27,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,48,0,28,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,49,0,29,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,50,0,30,0,0},
	{RACE_DROW,CLASS_ALCHEMIST,51,0,30,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,0,0,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,1,0,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,2,0,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,3,0,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,4,0,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,5,0,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,6,1,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,7,1,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,8,1,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,9,1,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,10,1,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,11,1,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,12,1,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,13,1,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,14,1,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,15,1,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,16,1,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,17,1,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,18,1,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,19,1,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,20,1,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,21,1,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,22,2,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,23,2,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,24,2,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,25,2,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,26,2,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,27,2,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,28,3,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,29,3,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,30,3,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,31,3,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,32,4,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,33,4,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,34,4,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,35,5,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,36,6,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,37,6,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,38,7,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,39,7,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,40,8,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,41,8,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,42,9,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,43,9,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,44,9,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,45,10,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,46,10,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,47,10,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,48,10,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,49,11,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,50,12,0,0,0},
	{RACE_ZOMBIE,CLASS_WARRIOR,51,13,0,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,0,0,0,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,1,1,0,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,2,2,1,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,3,2,1,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,4,3,2,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,5,4,3,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,6,6,4,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,7,7,4,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,8,8,4,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,9,9,4,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,10,10,4,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,11,11,4,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,12,11,4,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,13,11,4,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,14,13,4,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,15,14,5,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,16,15,5,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,17,16,5,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,18,17,5,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,19,18,5,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,20,20,5,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,21,21,5,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,22,22,5,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,23,23,5,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,24,24,5,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,25,25,5,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,26,26,6,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,27,27,7,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,28,27,7,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,29,29,8,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,30,30,8,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,31,32,8,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,32,34,8,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,33,35,8,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,34,37,9,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,35,38,10,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,36,39,10,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,37,41,10,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,38,41,10,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,39,42,10,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,40,43,10,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,41,43,11,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,42,44,12,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,43,44,13,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,44,45,13,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,45,47,13,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,46,48,14,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,47,49,15,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,48,50,16,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,49,51,16,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,50,53,17,0,0},
	{RACE_ZOMBIE,CLASS_NECROMANT,51,55,17,0,0},
	{RACE_ZOMBIE,CLASS_CLERIC,0,0,0,0,0},
	{RACE_ZOMBIE,CLASS_CLERIC,1,1,0,0,1},
	{RACE_ZOMBIE,CLASS_CLERIC,2,2,0,0,2},
	{RACE_ZOMBIE,CLASS_CLERIC,3,3,0,0,3},
	{RACE_ZOMBIE,CLASS_CLERIC,4,4,0,0,4},
	{RACE_ZOMBIE,CLASS_CLERIC,5,5,0,0,5},
	{RACE_ZOMBIE,CLASS_CLERIC,6,6,0,0,6},
	{RACE_ZOMBIE,CLASS_CLERIC,7,7,0,0,7},
	{RACE_ZOMBIE,CLASS_CLERIC,8,8,0,0,8},
	{RACE_ZOMBIE,CLASS_CLERIC,9,9,0,0,9},
	{RACE_ZOMBIE,CLASS_CLERIC,10,10,0,0,10},
	{RACE_ZOMBIE,CLASS_CLERIC,11,11,0,0,11},
	{RACE_ZOMBIE,CLASS_CLERIC,12,12,0,0,12},
	{RACE_ZOMBIE,CLASS_CLERIC,13,13,0,0,13},
	{RACE_ZOMBIE,CLASS_CLERIC,14,14,0,0,14},
	{RACE_ZOMBIE,CLASS_CLERIC,15,15,0,0,15},
	{RACE_ZOMBIE,CLASS_CLERIC,16,16,0,0,16},
	{RACE_ZOMBIE,CLASS_CLERIC,17,17,0,0,17},
	{RACE_ZOMBIE,CLASS_CLERIC,18,18,0,0,18},
	{RACE_ZOMBIE,CLASS_CLERIC,19,19,0,0,19},
	{RACE_ZOMBIE,CLASS_CLERIC,20,20,0,0,20},
	{RACE_ZOMBIE,CLASS_CLERIC,21,21,0,0,21},
	{RACE_ZOMBIE,CLASS_CLERIC,22,22,0,0,22},
	{RACE_ZOMBIE,CLASS_CLERIC,23,23,0,0,23},
	{RACE_ZOMBIE,CLASS_CLERIC,24,24,0,0,24},
	{RACE_ZOMBIE,CLASS_CLERIC,25,25,0,0,25},
	{RACE_ZOMBIE,CLASS_CLERIC,26,26,0,0,26},
	{RACE_ZOMBIE,CLASS_CLERIC,27,27,0,0,27},
	{RACE_ZOMBIE,CLASS_CLERIC,28,28,0,0,28},
	{RACE_ZOMBIE,CLASS_CLERIC,29,29,0,0,29},
	{RACE_ZOMBIE,CLASS_CLERIC,30,30,0,0,30},
	{RACE_ZOMBIE,CLASS_CLERIC,31,31,0,0,31},
	{RACE_ZOMBIE,CLASS_CLERIC,32,32,0,0,32},
	{RACE_ZOMBIE,CLASS_CLERIC,33,33,0,0,33},
	{RACE_ZOMBIE,CLASS_CLERIC,34,34,0,0,34},
	{RACE_ZOMBIE,CLASS_CLERIC,35,35,0,0,35},
	{RACE_ZOMBIE,CLASS_CLERIC,36,36,0,0,36},
	{RACE_ZOMBIE,CLASS_CLERIC,37,37,0,0,37},
	{RACE_ZOMBIE,CLASS_CLERIC,38,38,0,0,38},
	{RACE_ZOMBIE,CLASS_CLERIC,39,39,0,0,39},
	{RACE_ZOMBIE,CLASS_CLERIC,40,40,0,0,40},
	{RACE_ZOMBIE,CLASS_CLERIC,41,41,0,0,41},
	{RACE_ZOMBIE,CLASS_CLERIC,42,42,0,0,42},
	{RACE_ZOMBIE,CLASS_CLERIC,43,43,0,0,43},
	{RACE_ZOMBIE,CLASS_CLERIC,44,44,0,0,44},
	{RACE_ZOMBIE,CLASS_CLERIC,45,45,0,0,45},
	{RACE_ZOMBIE,CLASS_CLERIC,46,46,0,0,46},
	{RACE_ZOMBIE,CLASS_CLERIC,47,47,0,0,47},
	{RACE_ZOMBIE,CLASS_CLERIC,48,48,0,0,48},
	{RACE_ZOMBIE,CLASS_CLERIC,49,49,0,0,49},
	{RACE_ZOMBIE,CLASS_CLERIC,50,50,0,0,50},
	{RACE_ZOMBIE,CLASS_CLERIC,51,51,0,0,51},
	{RACE_ZOMBIE,CLASS_THIEF,0,0,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,1,1,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,2,2,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,3,3,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,4,4,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,5,5,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,6,6,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,7,7,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,8,8,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,9,9,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,10,10,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,11,11,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,12,12,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,13,13,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,14,14,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,15,15,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,16,16,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,17,17,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,18,18,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,19,19,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,20,20,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,21,21,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,22,22,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,23,23,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,24,24,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,25,25,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,26,26,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,27,27,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,28,28,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,29,29,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,30,30,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,31,31,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,32,32,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,33,33,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,34,34,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,35,35,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,36,36,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,37,37,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,38,38,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,39,39,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,40,40,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,41,41,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,42,42,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,43,43,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,44,44,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,45,45,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,46,46,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,47,47,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,48,48,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,49,49,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,50,50,0,0,0},
	{RACE_ZOMBIE,CLASS_THIEF,51,51,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,0,0,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,1,1,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,2,2,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,3,2,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,4,3,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,5,4,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,6,6,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,7,6,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,8,7,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,9,8,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,10,9,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,11,11,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,12,12,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,13,13,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,14,14,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,15,15,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,16,17,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,17,18,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,18,19,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,19,20,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,20,21,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,21,22,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,22,24,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,23,24,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,24,25,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,25,26,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,26,27,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,27,28,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,28,29,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,29,29,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,30,30,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,31,30,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,32,30,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,33,31,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,34,31,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,35,33,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,36,34,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,37,35,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,38,36,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,39,36,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,40,37,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,41,37,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,42,39,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,43,40,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,44,42,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,45,43,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,46,44,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,47,45,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,48,47,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,49,49,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,50,51,0,0,0},
	{RACE_ZOMBIE,CLASS_NAZGUL,51,52,0,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,0,0,0,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,1,1,1,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,2,1,2,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,3,2,3,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,4,3,4,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,5,4,4,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,6,5,5,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,7,6,5,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,8,8,5,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,9,9,5,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,10,11,6,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,11,12,7,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,12,13,8,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,13,14,9,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,14,14,10,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,15,15,11,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,16,15,12,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,17,16,12,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,18,17,12,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,19,18,13,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,20,19,14,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,21,21,15,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,22,22,16,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,23,22,16,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,24,24,16,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,25,25,16,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,26,26,16,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,27,27,16,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,28,28,16,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,29,29,17,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,30,31,17,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,31,33,17,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,32,34,17,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,33,36,18,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,34,37,18,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,35,39,19,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,36,40,20,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,37,42,21,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,38,44,21,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,39,45,22,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,40,46,23,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,41,46,23,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,42,47,23,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,43,48,23,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,44,48,24,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,45,49,24,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,46,50,24,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,47,51,25,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,48,52,25,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,49,53,25,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,50,54,25,0,0},
	{RACE_ZOMBIE,CLASS_MAGE,51,55,26,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,0,0,0,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,1,1,1,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,2,2,0,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,3,3,0,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,4,4,1,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,5,5,1,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,6,6,1,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,7,6,2,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,8,7,2,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,9,7,2,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,10,7,3,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,11,7,2,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,12,8,2,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,13,9,2,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,14,10,2,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,15,11,3,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,16,11,3,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,17,13,3,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,18,14,3,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,19,15,3,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,20,16,3,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,21,17,4,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,22,18,5,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,23,19,5,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,24,20,6,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,25,22,6,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,26,22,5,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,27,23,4,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,28,24,3,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,29,25,3,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,30,26,4,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,31,26,4,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,32,27,4,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,33,29,4,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,34,30,4,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,35,32,3,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,36,34,2,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,37,34,1,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,38,36,1,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,39,38,1,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,40,40,1,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,41,41,2,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,42,42,2,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,43,43,2,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,44,45,1,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,45,46,1,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,46,47,1,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,47,47,1,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,48,48,1,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,49,48,1,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,50,49,0,0,0},
	{RACE_TIGGUAN,CLASS_WARRIOR,51,49,0,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,0,0,0,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,1,1,1,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,2,2,2,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,3,3,2,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,4,4,3,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,5,5,2,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,6,6,2,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,7,7,1,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,8,8,2,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,9,9,2,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,10,10,2,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,11,11,1,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,12,12,1,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,13,13,1,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,14,14,2,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,15,15,1,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,16,16,1,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,17,17,1,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,18,18,1,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,19,19,1,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,20,20,0,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,21,21,1,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,22,22,1,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,23,23,1,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,24,24,0,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,25,25,1,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,26,26,2,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,27,27,2,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,28,28,3,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,29,29,3,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,30,30,4,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,31,31,4,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,32,32,5,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,33,33,5,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,34,34,6,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,35,35,6,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,36,36,6,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,37,37,6,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,38,38,7,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,39,39,8,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,40,40,8,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,41,41,8,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,42,42,8,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,43,43,9,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,44,44,10,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,45,45,10,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,46,46,10,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,47,47,10,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,48,48,10,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,49,49,10,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,50,50,10,0,0},
	{RACE_TIGGUAN,CLASS_DRUID,51,51,9,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,0,0,0,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,1,1,0,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,2,2,1,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,3,3,1,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,4,4,1,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,5,5,1,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,6,6,2,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,7,7,2,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,8,8,2,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,9,9,1,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,10,10,1,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,11,11,2,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,12,12,2,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,13,13,1,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,14,14,2,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,15,15,2,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,16,16,2,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,17,17,1,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,18,18,2,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,19,19,1,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,20,20,2,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,21,21,2,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,22,22,3,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,23,23,3,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,24,24,2,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,25,25,2,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,26,26,3,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,27,27,2,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,28,28,2,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,29,29,1,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,30,30,1,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,31,31,0,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,32,32,1,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,33,33,1,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,34,34,2,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,35,35,3,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,36,36,4,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,37,37,5,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,38,38,4,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,39,39,4,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,40,40,4,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,41,41,3,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,42,42,3,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,43,43,3,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,44,44,3,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,45,45,2,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,46,46,2,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,47,47,3,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,48,48,3,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,49,49,3,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,50,50,3,0,0},
	{RACE_TIGGUAN,CLASS_CLERIC,51,51,4,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,0,0,0,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,1,1,1,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,2,2,1,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,3,3,1,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,4,4,1,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,5,5,1,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,6,6,2,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,7,7,2,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,8,8,2,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,9,9,1,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,10,10,2,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,11,11,2,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,12,12,1,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,13,13,0,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,14,14,1,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,15,15,1,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,16,16,1,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,17,17,1,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,18,18,1,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,19,19,1,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,20,20,1,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,21,21,1,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,22,22,0,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,23,23,1,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,24,24,2,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,25,25,3,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,26,26,3,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,27,27,3,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,28,28,3,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,29,29,3,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,30,30,3,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,31,31,3,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,32,32,3,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,33,33,4,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,34,34,4,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,35,35,4,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,36,36,3,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,37,37,3,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,38,38,3,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,39,39,2,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,40,40,2,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,41,41,2,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,42,42,2,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,43,43,1,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,44,44,1,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,45,45,0,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,46,46,0,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,47,47,0,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,48,48,1,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,49,49,2,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,50,50,2,0,0},
	{RACE_TIGGUAN,CLASS_THIEF,51,51,2,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,0,0,0,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,1,1,0,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,2,2,1,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,3,3,1,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,4,4,1,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,5,5,1,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,6,6,0,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,7,7,1,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,8,8,1,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,9,9,2,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,10,10,1,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,11,11,0,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,12,12,0,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,13,13,1,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,14,14,0,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,15,15,1,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,16,16,1,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,17,17,1,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,18,18,2,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,19,19,2,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,20,20,3,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,21,21,3,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,22,22,2,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,23,23,2,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,24,24,3,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,25,25,4,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,26,26,5,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,27,27,4,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,28,28,4,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,29,29,4,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,30,30,3,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,31,31,3,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,32,32,4,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,33,33,3,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,34,34,3,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,35,35,4,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,36,36,3,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,37,37,2,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,38,38,3,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,39,39,3,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,40,40,3,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,41,41,3,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,42,42,3,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,43,43,2,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,44,44,1,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,45,45,1,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,46,46,2,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,47,47,2,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,48,48,2,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,49,49,2,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,50,50,1,0,0},
	{RACE_TIGGUAN,CLASS_RANGER,51,51,2,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,0,0,0,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,1,1,0,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,2,2,0,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,3,3,1,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,4,4,1,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,5,5,2,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,6,6,2,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,7,7,1,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,8,8,1,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,9,9,0,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,10,10,0,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,11,11,1,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,12,12,1,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,13,13,2,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,14,14,2,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,15,15,2,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,16,16,2,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,17,17,2,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,18,18,1,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,19,19,0,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,20,20,0,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,21,21,0,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,22,22,1,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,23,23,1,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,24,24,1,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,25,25,0,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,26,26,1,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,27,27,1,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,28,28,1,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,29,29,1,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,30,30,0,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,31,31,1,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,32,32,0,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,33,33,1,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,34,34,2,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,35,35,2,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,36,36,1,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,37,37,1,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,38,38,0,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,39,39,1,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,40,40,2,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,41,41,3,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,42,42,4,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,43,43,4,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,44,44,3,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,45,45,3,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,46,46,2,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,47,47,2,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,48,48,2,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,49,49,1,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,50,50,2,0,0},
	{RACE_TIGGUAN,CLASS_MAGE,51,51,1,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,0,0,0,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,1,1,1,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,2,2,1,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,3,3,1,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,4,4,2,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,5,5,2,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,6,6,2,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,7,7,2,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,8,8,1,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,9,9,1,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,10,10,1,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,11,11,1,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,12,12,1,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,13,13,2,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,14,14,2,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,15,15,2,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,16,16,1,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,17,17,0,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,18,18,1,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,19,19,2,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,20,20,2,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,21,21,2,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,22,22,3,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,23,23,2,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,24,24,1,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,25,25,2,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,26,26,3,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,27,27,3,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,28,28,4,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,29,29,5,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,30,30,4,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,31,31,3,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,32,32,2,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,33,33,1,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,34,34,2,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,35,35,1,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,36,36,1,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,37,37,1,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,38,38,1,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,39,39,1,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,40,40,2,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,41,41,2,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,42,42,1,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,43,43,2,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,44,44,3,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,45,45,3,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,46,46,4,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,47,47,5,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,48,48,5,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,49,49,4,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,50,50,4,0,0},
	{RACE_TIGGUAN,CLASS_ALCHEMIST,51,51,3,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,0,0,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,1,1,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,2,2,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,3,3,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,4,4,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,5,5,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,6,6,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,7,7,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,8,8,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,9,9,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,10,10,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,11,11,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,12,12,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,13,13,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,14,14,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,15,15,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,16,16,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,17,17,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,18,18,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,19,19,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,20,20,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,21,21,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,22,22,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,23,23,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,24,24,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,25,25,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,26,26,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,27,27,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,28,28,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,29,29,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,30,30,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,31,31,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,32,32,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,33,33,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,34,34,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,35,35,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,36,36,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,37,37,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,38,38,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,39,39,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,40,40,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,41,41,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,42,42,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,43,43,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,44,44,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,45,45,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,46,46,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,47,47,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,48,48,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,49,49,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,50,50,0,0,0},
	{RACE_TIGGUAN,CLASS_PALADIN,51,51,0,0,0},
	{RACE_SNAKE,CLASS_WARRIOR,0,0,0,0,0},
	{RACE_SNAKE,CLASS_WARRIOR,1,0,0,0,1},
	{RACE_SNAKE,CLASS_WARRIOR,2,1,0,0,2},
	{RACE_SNAKE,CLASS_WARRIOR,3,3,0,0,3},
	{RACE_SNAKE,CLASS_WARRIOR,4,4,0,0,4},
	{RACE_SNAKE,CLASS_WARRIOR,5,4,0,0,5},
	{RACE_SNAKE,CLASS_WARRIOR,6,5,1,0,6},
	{RACE_SNAKE,CLASS_WARRIOR,7,6,1,0,7},
	{RACE_SNAKE,CLASS_WARRIOR,8,8,2,0,8},
	{RACE_SNAKE,CLASS_WARRIOR,9,9,3,0,9},
	{RACE_SNAKE,CLASS_WARRIOR,10,9,3,0,10},
	{RACE_SNAKE,CLASS_WARRIOR,11,10,4,0,11},
	{RACE_SNAKE,CLASS_WARRIOR,12,10,5,0,12},
	{RACE_SNAKE,CLASS_WARRIOR,13,11,5,0,13},
	{RACE_SNAKE,CLASS_WARRIOR,14,12,6,0,14},
	{RACE_SNAKE,CLASS_WARRIOR,15,14,7,0,15},
	{RACE_SNAKE,CLASS_WARRIOR,16,14,7,0,16},
	{RACE_SNAKE,CLASS_WARRIOR,17,16,7,0,17},
	{RACE_SNAKE,CLASS_WARRIOR,18,17,7,0,18},
	{RACE_SNAKE,CLASS_WARRIOR,19,19,8,0,19},
	{RACE_SNAKE,CLASS_WARRIOR,20,20,9,0,20},
	{RACE_SNAKE,CLASS_WARRIOR,21,21,10,0,21},
	{RACE_SNAKE,CLASS_WARRIOR,22,22,10,0,22},
	{RACE_SNAKE,CLASS_WARRIOR,23,23,11,0,23},
	{RACE_SNAKE,CLASS_WARRIOR,24,24,11,0,24},
	{RACE_SNAKE,CLASS_WARRIOR,25,25,12,0,25},
	{RACE_SNAKE,CLASS_WARRIOR,26,27,13,0,26},
	{RACE_SNAKE,CLASS_WARRIOR,27,27,13,0,27},
	{RACE_SNAKE,CLASS_WARRIOR,28,28,13,0,28},
	{RACE_SNAKE,CLASS_WARRIOR,29,29,14,0,29},
	{RACE_SNAKE,CLASS_WARRIOR,30,30,15,0,30},
	{RACE_SNAKE,CLASS_WARRIOR,31,30,16,0,31},
	{RACE_SNAKE,CLASS_WARRIOR,32,31,16,0,32},
	{RACE_SNAKE,CLASS_WARRIOR,33,32,16,0,33},
	{RACE_SNAKE,CLASS_WARRIOR,34,34,16,0,34},
	{RACE_SNAKE,CLASS_WARRIOR,35,36,16,0,35},
	{RACE_SNAKE,CLASS_WARRIOR,36,37,16,0,36},
	{RACE_SNAKE,CLASS_WARRIOR,37,38,16,0,37},
	{RACE_SNAKE,CLASS_WARRIOR,38,38,17,0,38},
	{RACE_SNAKE,CLASS_WARRIOR,39,39,17,0,39},
	{RACE_SNAKE,CLASS_WARRIOR,40,40,18,0,40},
	{RACE_SNAKE,CLASS_WARRIOR,41,41,19,0,41},
	{RACE_SNAKE,CLASS_WARRIOR,42,42,19,0,42},
	{RACE_SNAKE,CLASS_WARRIOR,43,43,19,0,43},
	{RACE_SNAKE,CLASS_WARRIOR,44,44,20,0,44},
	{RACE_SNAKE,CLASS_WARRIOR,45,44,20,0,45},
	{RACE_SNAKE,CLASS_WARRIOR,46,45,20,0,46},
	{RACE_SNAKE,CLASS_WARRIOR,47,46,21,0,47},
	{RACE_SNAKE,CLASS_WARRIOR,48,47,21,0,48},
	{RACE_SNAKE,CLASS_WARRIOR,49,49,22,0,49},
	{RACE_SNAKE,CLASS_WARRIOR,50,50,23,0,50},
	{RACE_SNAKE,CLASS_WARRIOR,51,52,24,0,51},
	{RACE_SNAKE,CLASS_NECROMANT,0,0,0,0,0},
	{RACE_SNAKE,CLASS_NECROMANT,1,1,0,0,1},
	{RACE_SNAKE,CLASS_NECROMANT,2,2,1,0,2},
	{RACE_SNAKE,CLASS_NECROMANT,3,3,1,0,3},
	{RACE_SNAKE,CLASS_NECROMANT,4,4,1,0,4},
	{RACE_SNAKE,CLASS_NECROMANT,5,5,2,0,5},
	{RACE_SNAKE,CLASS_NECROMANT,6,6,2,0,6},
	{RACE_SNAKE,CLASS_NECROMANT,7,7,2,0,7},
	{RACE_SNAKE,CLASS_NECROMANT,8,8,3,0,8},
	{RACE_SNAKE,CLASS_NECROMANT,9,9,4,0,9},
	{RACE_SNAKE,CLASS_NECROMANT,10,10,4,0,10},
	{RACE_SNAKE,CLASS_NECROMANT,11,10,4,0,11},
	{RACE_SNAKE,CLASS_NECROMANT,12,11,4,0,12},
	{RACE_SNAKE,CLASS_NECROMANT,13,12,5,0,13},
	{RACE_SNAKE,CLASS_NECROMANT,14,13,6,0,14},
	{RACE_SNAKE,CLASS_NECROMANT,15,14,6,0,15},
	{RACE_SNAKE,CLASS_NECROMANT,16,14,7,0,16},
	{RACE_SNAKE,CLASS_NECROMANT,17,15,8,0,17},
	{RACE_SNAKE,CLASS_NECROMANT,18,16,8,0,18},
	{RACE_SNAKE,CLASS_NECROMANT,19,16,8,0,19},
	{RACE_SNAKE,CLASS_NECROMANT,20,17,9,0,20},
	{RACE_SNAKE,CLASS_NECROMANT,21,17,10,0,21},
	{RACE_SNAKE,CLASS_NECROMANT,22,18,10,0,22},
	{RACE_SNAKE,CLASS_NECROMANT,23,19,10,0,23},
	{RACE_SNAKE,CLASS_NECROMANT,24,20,10,0,24},
	{RACE_SNAKE,CLASS_NECROMANT,25,21,10,0,25},
	{RACE_SNAKE,CLASS_NECROMANT,26,22,10,0,26},
	{RACE_SNAKE,CLASS_NECROMANT,27,23,10,0,27},
	{RACE_SNAKE,CLASS_NECROMANT,28,24,11,0,28},
	{RACE_SNAKE,CLASS_NECROMANT,29,25,11,0,29},
	{RACE_SNAKE,CLASS_NECROMANT,30,25,11,0,30},
	{RACE_SNAKE,CLASS_NECROMANT,31,26,11,0,31},
	{RACE_SNAKE,CLASS_NECROMANT,32,27,12,0,32},
	{RACE_SNAKE,CLASS_NECROMANT,33,28,13,0,33},
	{RACE_SNAKE,CLASS_NECROMANT,34,29,13,0,34},
	{RACE_SNAKE,CLASS_NECROMANT,35,30,14,0,35},
	{RACE_SNAKE,CLASS_NECROMANT,36,31,15,0,36},
	{RACE_SNAKE,CLASS_NECROMANT,37,32,15,0,37},
	{RACE_SNAKE,CLASS_NECROMANT,38,32,16,0,38},
	{RACE_SNAKE,CLASS_NECROMANT,39,33,16,0,39},
	{RACE_SNAKE,CLASS_NECROMANT,40,33,16,0,40},
	{RACE_SNAKE,CLASS_NECROMANT,41,34,17,0,41},
	{RACE_SNAKE,CLASS_NECROMANT,42,35,18,0,42},
	{RACE_SNAKE,CLASS_NECROMANT,43,36,18,0,43},
	{RACE_SNAKE,CLASS_NECROMANT,44,37,18,0,44},
	{RACE_SNAKE,CLASS_NECROMANT,45,38,18,0,45},
	{RACE_SNAKE,CLASS_NECROMANT,46,39,19,0,46},
	{RACE_SNAKE,CLASS_NECROMANT,47,40,19,0,47},
	{RACE_SNAKE,CLASS_NECROMANT,48,41,20,0,48},
	{RACE_SNAKE,CLASS_NECROMANT,49,42,21,0,49},
	{RACE_SNAKE,CLASS_NECROMANT,50,43,21,0,50},
	{RACE_SNAKE,CLASS_NECROMANT,51,44,21,0,51},
	{RACE_SNAKE,CLASS_DRUID,0,0,0,0,0},
	{RACE_SNAKE,CLASS_DRUID,1,1,0,0,1},
	{RACE_SNAKE,CLASS_DRUID,2,2,1,0,2},
	{RACE_SNAKE,CLASS_DRUID,3,3,1,0,3},
	{RACE_SNAKE,CLASS_DRUID,4,4,2,0,4},
	{RACE_SNAKE,CLASS_DRUID,5,5,2,0,5},
	{RACE_SNAKE,CLASS_DRUID,6,6,3,0,6},
	{RACE_SNAKE,CLASS_DRUID,7,7,3,0,7},
	{RACE_SNAKE,CLASS_DRUID,8,8,4,0,8},
	{RACE_SNAKE,CLASS_DRUID,9,9,5,0,9},
	{RACE_SNAKE,CLASS_DRUID,10,10,6,0,10},
	{RACE_SNAKE,CLASS_DRUID,11,11,7,0,11},
	{RACE_SNAKE,CLASS_DRUID,12,12,7,0,12},
	{RACE_SNAKE,CLASS_DRUID,13,13,7,0,13},
	{RACE_SNAKE,CLASS_DRUID,14,14,7,0,14},
	{RACE_SNAKE,CLASS_DRUID,15,15,7,0,15},
	{RACE_SNAKE,CLASS_DRUID,16,16,7,0,16},
	{RACE_SNAKE,CLASS_DRUID,17,17,7,0,17},
	{RACE_SNAKE,CLASS_DRUID,18,18,7,0,18},
	{RACE_SNAKE,CLASS_DRUID,19,19,7,0,19},
	{RACE_SNAKE,CLASS_DRUID,20,20,8,0,20},
	{RACE_SNAKE,CLASS_DRUID,21,21,8,0,21},
	{RACE_SNAKE,CLASS_DRUID,22,22,8,0,22},
	{RACE_SNAKE,CLASS_DRUID,23,23,8,0,23},
	{RACE_SNAKE,CLASS_DRUID,24,24,9,0,24},
	{RACE_SNAKE,CLASS_DRUID,25,25,9,0,25},
	{RACE_SNAKE,CLASS_DRUID,26,26,9,0,26},
	{RACE_SNAKE,CLASS_DRUID,27,27,10,0,27},
	{RACE_SNAKE,CLASS_DRUID,28,28,10,0,28},
	{RACE_SNAKE,CLASS_DRUID,29,29,10,0,29},
	{RACE_SNAKE,CLASS_DRUID,30,30,10,0,30},
	{RACE_SNAKE,CLASS_DRUID,31,31,10,0,31},
	{RACE_SNAKE,CLASS_DRUID,32,32,11,0,32},
	{RACE_SNAKE,CLASS_DRUID,33,33,12,0,33},
	{RACE_SNAKE,CLASS_DRUID,34,34,12,0,34},
	{RACE_SNAKE,CLASS_DRUID,35,35,12,0,35},
	{RACE_SNAKE,CLASS_DRUID,36,36,13,0,36},
	{RACE_SNAKE,CLASS_DRUID,37,37,13,0,37},
	{RACE_SNAKE,CLASS_DRUID,38,38,13,0,38},
	{RACE_SNAKE,CLASS_DRUID,39,39,14,0,39},
	{RACE_SNAKE,CLASS_DRUID,40,40,14,0,40},
	{RACE_SNAKE,CLASS_DRUID,41,41,15,0,41},
	{RACE_SNAKE,CLASS_DRUID,42,42,15,0,42},
	{RACE_SNAKE,CLASS_DRUID,43,43,15,0,43},
	{RACE_SNAKE,CLASS_DRUID,44,44,15,0,44},
	{RACE_SNAKE,CLASS_DRUID,45,45,16,0,45},
	{RACE_SNAKE,CLASS_DRUID,46,46,16,0,46},
	{RACE_SNAKE,CLASS_DRUID,47,47,16,0,47},
	{RACE_SNAKE,CLASS_DRUID,48,48,16,0,48},
	{RACE_SNAKE,CLASS_DRUID,49,49,16,0,49},
	{RACE_SNAKE,CLASS_DRUID,50,50,16,0,50},
	{RACE_SNAKE,CLASS_DRUID,51,51,17,0,51},
	{RACE_SNAKE,CLASS_CLERIC,0,0,0,0,0},
	{RACE_SNAKE,CLASS_CLERIC,1,1,1,0,2},
	{RACE_SNAKE,CLASS_CLERIC,2,2,0,0,4},
	{RACE_SNAKE,CLASS_CLERIC,3,3,0,0,6},
	{RACE_SNAKE,CLASS_CLERIC,4,4,0,0,8},
	{RACE_SNAKE,CLASS_CLERIC,5,5,0,0,10},
	{RACE_SNAKE,CLASS_CLERIC,6,6,0,0,12},
	{RACE_SNAKE,CLASS_CLERIC,7,7,1,0,14},
	{RACE_SNAKE,CLASS_CLERIC,8,8,1,0,16},
	{RACE_SNAKE,CLASS_CLERIC,9,9,2,0,18},
	{RACE_SNAKE,CLASS_CLERIC,10,10,2,0,20},
	{RACE_SNAKE,CLASS_CLERIC,11,11,3,0,22},
	{RACE_SNAKE,CLASS_CLERIC,12,12,4,0,24},
	{RACE_SNAKE,CLASS_CLERIC,13,13,5,0,26},
	{RACE_SNAKE,CLASS_CLERIC,14,14,5,0,28},
	{RACE_SNAKE,CLASS_CLERIC,15,15,5,0,30},
	{RACE_SNAKE,CLASS_CLERIC,16,16,4,0,32},
	{RACE_SNAKE,CLASS_CLERIC,17,17,4,0,34},
	{RACE_SNAKE,CLASS_CLERIC,18,18,4,0,36},
	{RACE_SNAKE,CLASS_CLERIC,19,19,4,0,38},
	{RACE_SNAKE,CLASS_CLERIC,20,20,4,0,40},
	{RACE_SNAKE,CLASS_CLERIC,21,21,4,0,42},
	{RACE_SNAKE,CLASS_CLERIC,22,22,4,0,44},
	{RACE_SNAKE,CLASS_CLERIC,23,23,5,0,46},
	{RACE_SNAKE,CLASS_CLERIC,24,24,4,0,48},
	{RACE_SNAKE,CLASS_CLERIC,25,25,4,0,50},
	{RACE_SNAKE,CLASS_CLERIC,26,26,4,0,52},
	{RACE_SNAKE,CLASS_CLERIC,27,27,4,0,54},
	{RACE_SNAKE,CLASS_CLERIC,28,28,4,0,56},
	{RACE_SNAKE,CLASS_CLERIC,29,29,5,0,58},
	{RACE_SNAKE,CLASS_CLERIC,30,30,4,0,60},
	{RACE_SNAKE,CLASS_CLERIC,31,31,4,0,62},
	{RACE_SNAKE,CLASS_CLERIC,32,32,4,0,64},
	{RACE_SNAKE,CLASS_CLERIC,33,33,5,0,66},
	{RACE_SNAKE,CLASS_CLERIC,34,34,5,0,68},
	{RACE_SNAKE,CLASS_CLERIC,35,35,5,0,70},
	{RACE_SNAKE,CLASS_CLERIC,36,36,4,0,72},
	{RACE_SNAKE,CLASS_CLERIC,37,37,4,0,74},
	{RACE_SNAKE,CLASS_CLERIC,38,38,5,0,76},
	{RACE_SNAKE,CLASS_CLERIC,39,39,5,0,78},
	{RACE_SNAKE,CLASS_CLERIC,40,40,5,0,80},
	{RACE_SNAKE,CLASS_CLERIC,41,41,5,0,82},
	{RACE_SNAKE,CLASS_CLERIC,42,42,4,0,84},
	{RACE_SNAKE,CLASS_CLERIC,43,43,4,0,86},
	{RACE_SNAKE,CLASS_CLERIC,44,44,3,0,88},
	{RACE_SNAKE,CLASS_CLERIC,45,45,4,0,90},
	{RACE_SNAKE,CLASS_CLERIC,46,46,4,0,92},
	{RACE_SNAKE,CLASS_CLERIC,47,47,4,0,94},
	{RACE_SNAKE,CLASS_CLERIC,48,48,4,0,96},
	{RACE_SNAKE,CLASS_CLERIC,49,49,4,0,98},
	{RACE_SNAKE,CLASS_CLERIC,50,50,3,0,100},
	{RACE_SNAKE,CLASS_CLERIC,51,51,3,0,102},
	{RACE_SNAKE,CLASS_THIEF,0,0,0,0,0},
	{RACE_SNAKE,CLASS_THIEF,1,1,0,0,1},
	{RACE_SNAKE,CLASS_THIEF,2,2,0,0,2},
	{RACE_SNAKE,CLASS_THIEF,3,3,1,0,3},
	{RACE_SNAKE,CLASS_THIEF,4,4,1,0,4},
	{RACE_SNAKE,CLASS_THIEF,5,5,1,0,5},
	{RACE_SNAKE,CLASS_THIEF,6,6,1,0,6},
	{RACE_SNAKE,CLASS_THIEF,7,7,2,0,7},
	{RACE_SNAKE,CLASS_THIEF,8,8,2,0,8},
	{RACE_SNAKE,CLASS_THIEF,9,9,2,0,9},
	{RACE_SNAKE,CLASS_THIEF,10,10,3,0,10},
	{RACE_SNAKE,CLASS_THIEF,11,11,4,0,11},
	{RACE_SNAKE,CLASS_THIEF,12,12,5,0,12},
	{RACE_SNAKE,CLASS_THIEF,13,13,6,0,13},
	{RACE_SNAKE,CLASS_THIEF,14,14,6,0,14},
	{RACE_SNAKE,CLASS_THIEF,15,15,6,0,15},
	{RACE_SNAKE,CLASS_THIEF,16,16,6,0,16},
	{RACE_SNAKE,CLASS_THIEF,17,17,7,0,17},
	{RACE_SNAKE,CLASS_THIEF,18,18,7,0,18},
	{RACE_SNAKE,CLASS_THIEF,19,19,7,0,19},
	{RACE_SNAKE,CLASS_THIEF,20,20,7,0,20},
	{RACE_SNAKE,CLASS_THIEF,21,21,7,0,21},
	{RACE_SNAKE,CLASS_THIEF,22,22,7,0,22},
	{RACE_SNAKE,CLASS_THIEF,23,23,8,0,23},
	{RACE_SNAKE,CLASS_THIEF,24,24,8,0,24},
	{RACE_SNAKE,CLASS_THIEF,25,25,9,0,25},
	{RACE_SNAKE,CLASS_THIEF,26,26,9,0,26},
	{RACE_SNAKE,CLASS_THIEF,27,27,10,0,27},
	{RACE_SNAKE,CLASS_THIEF,28,28,11,0,28},
	{RACE_SNAKE,CLASS_THIEF,29,29,12,0,29},
	{RACE_SNAKE,CLASS_THIEF,30,30,12,0,30},
	{RACE_SNAKE,CLASS_THIEF,31,31,12,0,31},
	{RACE_SNAKE,CLASS_THIEF,32,32,12,0,32},
	{RACE_SNAKE,CLASS_THIEF,33,33,13,0,33},
	{RACE_SNAKE,CLASS_THIEF,34,34,13,0,34},
	{RACE_SNAKE,CLASS_THIEF,35,35,14,0,35},
	{RACE_SNAKE,CLASS_THIEF,36,36,14,0,36},
	{RACE_SNAKE,CLASS_THIEF,37,37,14,0,37},
	{RACE_SNAKE,CLASS_THIEF,38,38,14,0,38},
	{RACE_SNAKE,CLASS_THIEF,39,39,14,0,39},
	{RACE_SNAKE,CLASS_THIEF,40,40,15,0,40},
	{RACE_SNAKE,CLASS_THIEF,41,41,15,0,41},
	{RACE_SNAKE,CLASS_THIEF,42,42,15,0,42},
	{RACE_SNAKE,CLASS_THIEF,43,43,15,0,43},
	{RACE_SNAKE,CLASS_THIEF,44,44,16,0,44},
	{RACE_SNAKE,CLASS_THIEF,45,45,16,0,45},
	{RACE_SNAKE,CLASS_THIEF,46,46,16,0,46},
	{RACE_SNAKE,CLASS_THIEF,47,47,16,0,47},
	{RACE_SNAKE,CLASS_THIEF,48,48,16,0,48},
	{RACE_SNAKE,CLASS_THIEF,49,49,17,0,49},
	{RACE_SNAKE,CLASS_THIEF,50,50,17,0,50},
	{RACE_SNAKE,CLASS_THIEF,51,51,17,0,51},
	{RACE_SNAKE,CLASS_RANGER,0,0,0,0,0},
	{RACE_SNAKE,CLASS_RANGER,1,1,1,0,1},
	{RACE_SNAKE,CLASS_RANGER,2,2,2,0,2},
	{RACE_SNAKE,CLASS_RANGER,3,3,2,0,3},
	{RACE_SNAKE,CLASS_RANGER,4,4,2,0,4},
	{RACE_SNAKE,CLASS_RANGER,5,5,2,0,5},
	{RACE_SNAKE,CLASS_RANGER,6,6,3,0,6},
	{RACE_SNAKE,CLASS_RANGER,7,7,3,0,7},
	{RACE_SNAKE,CLASS_RANGER,8,8,3,0,8},
	{RACE_SNAKE,CLASS_RANGER,9,9,3,0,9},
	{RACE_SNAKE,CLASS_RANGER,10,10,4,0,10},
	{RACE_SNAKE,CLASS_RANGER,11,11,4,0,11},
	{RACE_SNAKE,CLASS_RANGER,12,12,4,0,12},
	{RACE_SNAKE,CLASS_RANGER,13,13,5,0,13},
	{RACE_SNAKE,CLASS_RANGER,14,14,5,0,14},
	{RACE_SNAKE,CLASS_RANGER,15,15,6,0,15},
	{RACE_SNAKE,CLASS_RANGER,16,16,6,0,16},
	{RACE_SNAKE,CLASS_RANGER,17,17,6,0,17},
	{RACE_SNAKE,CLASS_RANGER,18,18,7,0,18},
	{RACE_SNAKE,CLASS_RANGER,19,19,8,0,19},
	{RACE_SNAKE,CLASS_RANGER,20,20,8,0,20},
	{RACE_SNAKE,CLASS_RANGER,21,21,8,0,21},
	{RACE_SNAKE,CLASS_RANGER,22,22,8,0,22},
	{RACE_SNAKE,CLASS_RANGER,23,23,9,0,23},
	{RACE_SNAKE,CLASS_RANGER,24,24,9,0,24},
	{RACE_SNAKE,CLASS_RANGER,25,25,9,0,25},
	{RACE_SNAKE,CLASS_RANGER,26,26,10,0,26},
	{RACE_SNAKE,CLASS_RANGER,27,27,10,0,27},
	{RACE_SNAKE,CLASS_RANGER,28,28,10,0,28},
	{RACE_SNAKE,CLASS_RANGER,29,29,10,0,29},
	{RACE_SNAKE,CLASS_RANGER,30,30,11,0,30},
	{RACE_SNAKE,CLASS_RANGER,31,31,12,0,31},
	{RACE_SNAKE,CLASS_RANGER,32,32,13,0,32},
	{RACE_SNAKE,CLASS_RANGER,33,33,14,0,33},
	{RACE_SNAKE,CLASS_RANGER,34,34,15,0,34},
	{RACE_SNAKE,CLASS_RANGER,35,35,16,0,35},
	{RACE_SNAKE,CLASS_RANGER,36,36,17,0,36},
	{RACE_SNAKE,CLASS_RANGER,37,37,18,0,37},
	{RACE_SNAKE,CLASS_RANGER,38,38,18,0,38},
	{RACE_SNAKE,CLASS_RANGER,39,39,18,0,39},
	{RACE_SNAKE,CLASS_RANGER,40,40,18,0,40},
	{RACE_SNAKE,CLASS_RANGER,41,41,18,0,41},
	{RACE_SNAKE,CLASS_RANGER,42,42,19,0,42},
	{RACE_SNAKE,CLASS_RANGER,43,43,20,0,43},
	{RACE_SNAKE,CLASS_RANGER,44,44,21,0,44},
	{RACE_SNAKE,CLASS_RANGER,45,45,21,0,45},
	{RACE_SNAKE,CLASS_RANGER,46,46,22,0,46},
	{RACE_SNAKE,CLASS_RANGER,47,47,22,0,47},
	{RACE_SNAKE,CLASS_RANGER,48,48,22,0,48},
	{RACE_SNAKE,CLASS_RANGER,49,49,22,0,49},
	{RACE_SNAKE,CLASS_RANGER,50,50,22,0,50},
	{RACE_SNAKE,CLASS_RANGER,51,51,23,0,51},
	{RACE_SNAKE,CLASS_NAZGUL,0,0,0,0,0},
	{RACE_SNAKE,CLASS_NAZGUL,1,1,0,0,1},
	{RACE_SNAKE,CLASS_NAZGUL,2,2,1,0,2},
	{RACE_SNAKE,CLASS_NAZGUL,3,4,1,0,3},
	{RACE_SNAKE,CLASS_NAZGUL,4,5,2,0,4},
	{RACE_SNAKE,CLASS_NAZGUL,5,5,3,0,5},
	{RACE_SNAKE,CLASS_NAZGUL,6,6,4,0,6},
	{RACE_SNAKE,CLASS_NAZGUL,7,7,4,0,7},
	{RACE_SNAKE,CLASS_NAZGUL,8,8,3,0,8},
	{RACE_SNAKE,CLASS_NAZGUL,9,9,3,0,9},
	{RACE_SNAKE,CLASS_NAZGUL,10,10,3,0,10},
	{RACE_SNAKE,CLASS_NAZGUL,11,10,2,0,11},
	{RACE_SNAKE,CLASS_NAZGUL,12,10,2,0,12},
	{RACE_SNAKE,CLASS_NAZGUL,13,11,2,0,13},
	{RACE_SNAKE,CLASS_NAZGUL,14,12,2,0,14},
	{RACE_SNAKE,CLASS_NAZGUL,15,13,2,0,15},
	{RACE_SNAKE,CLASS_NAZGUL,16,13,1,0,16},
	{RACE_SNAKE,CLASS_NAZGUL,17,13,0,0,17},
	{RACE_SNAKE,CLASS_NAZGUL,18,15,0,0,18},
	{RACE_SNAKE,CLASS_NAZGUL,19,17,0,0,19},
	{RACE_SNAKE,CLASS_NAZGUL,20,18,1,0,20},
	{RACE_SNAKE,CLASS_NAZGUL,21,19,0,0,21},
	{RACE_SNAKE,CLASS_NAZGUL,22,21,1,0,22},
	{RACE_SNAKE,CLASS_NAZGUL,23,23,1,0,23},
	{RACE_SNAKE,CLASS_NAZGUL,24,23,1,0,24},
	{RACE_SNAKE,CLASS_NAZGUL,25,24,1,0,25},
	{RACE_SNAKE,CLASS_NAZGUL,26,25,1,0,26},
	{RACE_SNAKE,CLASS_NAZGUL,27,25,0,0,27},
	{RACE_SNAKE,CLASS_NAZGUL,28,26,0,0,28},
	{RACE_SNAKE,CLASS_NAZGUL,29,28,0,0,29},
	{RACE_SNAKE,CLASS_NAZGUL,30,29,0,0,30},
	{RACE_SNAKE,CLASS_NAZGUL,31,30,0,0,31},
	{RACE_SNAKE,CLASS_NAZGUL,32,31,0,0,32},
	{RACE_SNAKE,CLASS_NAZGUL,33,32,0,0,33},
	{RACE_SNAKE,CLASS_NAZGUL,34,32,1,0,34},
	{RACE_SNAKE,CLASS_NAZGUL,35,33,2,0,35},
	{RACE_SNAKE,CLASS_NAZGUL,36,33,2,0,36},
	{RACE_SNAKE,CLASS_NAZGUL,37,34,1,0,37},
	{RACE_SNAKE,CLASS_NAZGUL,38,35,1,0,38},
	{RACE_SNAKE,CLASS_NAZGUL,39,36,2,0,39},
	{RACE_SNAKE,CLASS_NAZGUL,40,37,2,0,40},
	{RACE_SNAKE,CLASS_NAZGUL,41,37,3,0,41},
	{RACE_SNAKE,CLASS_NAZGUL,42,38,3,0,42},
	{RACE_SNAKE,CLASS_NAZGUL,43,40,3,0,43},
	{RACE_SNAKE,CLASS_NAZGUL,44,41,3,0,44},
	{RACE_SNAKE,CLASS_NAZGUL,45,43,3,0,45},
	{RACE_SNAKE,CLASS_NAZGUL,46,45,3,0,46},
	{RACE_SNAKE,CLASS_NAZGUL,47,46,3,0,47},
	{RACE_SNAKE,CLASS_NAZGUL,48,47,3,0,48},
	{RACE_SNAKE,CLASS_NAZGUL,49,49,2,0,49},
	{RACE_SNAKE,CLASS_NAZGUL,50,51,2,0,50},
	{RACE_SNAKE,CLASS_NAZGUL,51,52,2,0,51},
	{RACE_SNAKE,CLASS_MAGE,0,0,0,0,0},
	{RACE_SNAKE,CLASS_MAGE,1,1,0,0,1},
	{RACE_SNAKE,CLASS_MAGE,2,2,0,0,2},
	{RACE_SNAKE,CLASS_MAGE,3,3,0,0,3},
	{RACE_SNAKE,CLASS_MAGE,4,4,0,0,4},
	{RACE_SNAKE,CLASS_MAGE,5,5,1,0,5},
	{RACE_SNAKE,CLASS_MAGE,6,6,2,0,6},
	{RACE_SNAKE,CLASS_MAGE,7,7,2,0,7},
	{RACE_SNAKE,CLASS_MAGE,8,8,2,0,8},
	{RACE_SNAKE,CLASS_MAGE,9,9,3,0,9},
	{RACE_SNAKE,CLASS_MAGE,10,10,4,0,10},
	{RACE_SNAKE,CLASS_MAGE,11,11,5,0,11},
	{RACE_SNAKE,CLASS_MAGE,12,12,6,0,12},
	{RACE_SNAKE,CLASS_MAGE,13,13,7,0,13},
	{RACE_SNAKE,CLASS_MAGE,14,14,8,0,14},
	{RACE_SNAKE,CLASS_MAGE,15,15,8,0,15},
	{RACE_SNAKE,CLASS_MAGE,16,16,10,0,16},
	{RACE_SNAKE,CLASS_MAGE,17,17,10,0,17},
	{RACE_SNAKE,CLASS_MAGE,18,18,10,0,18},
	{RACE_SNAKE,CLASS_MAGE,19,19,11,0,19},
	{RACE_SNAKE,CLASS_MAGE,20,20,11,0,20},
	{RACE_SNAKE,CLASS_MAGE,21,21,12,0,21},
	{RACE_SNAKE,CLASS_MAGE,22,22,12,0,22},
	{RACE_SNAKE,CLASS_MAGE,23,23,12,0,23},
	{RACE_SNAKE,CLASS_MAGE,24,24,13,0,24},
	{RACE_SNAKE,CLASS_MAGE,25,25,13,0,25},
	{RACE_SNAKE,CLASS_MAGE,26,26,14,0,26},
	{RACE_SNAKE,CLASS_MAGE,27,27,14,0,27},
	{RACE_SNAKE,CLASS_MAGE,28,28,14,0,28},
	{RACE_SNAKE,CLASS_MAGE,29,29,14,0,29},
	{RACE_SNAKE,CLASS_MAGE,30,30,14,0,30},
	{RACE_SNAKE,CLASS_MAGE,31,31,14,0,31},
	{RACE_SNAKE,CLASS_MAGE,32,32,15,0,32},
	{RACE_SNAKE,CLASS_MAGE,33,33,15,0,33},
	{RACE_SNAKE,CLASS_MAGE,34,34,15,0,34},
	{RACE_SNAKE,CLASS_MAGE,35,35,15,0,35},
	{RACE_SNAKE,CLASS_MAGE,36,36,16,0,36},
	{RACE_SNAKE,CLASS_MAGE,37,37,17,0,37},
	{RACE_SNAKE,CLASS_MAGE,38,38,17,0,38},
	{RACE_SNAKE,CLASS_MAGE,39,39,18,0,39},
	{RACE_SNAKE,CLASS_MAGE,40,40,19,0,40},
	{RACE_SNAKE,CLASS_MAGE,41,41,19,0,41},
	{RACE_SNAKE,CLASS_MAGE,42,42,19,0,42},
	{RACE_SNAKE,CLASS_MAGE,43,43,19,0,43},
	{RACE_SNAKE,CLASS_MAGE,44,44,19,0,44},
	{RACE_SNAKE,CLASS_MAGE,45,45,20,0,45},
	{RACE_SNAKE,CLASS_MAGE,46,46,21,0,46},
	{RACE_SNAKE,CLASS_MAGE,47,47,22,0,47},
	{RACE_SNAKE,CLASS_MAGE,48,48,22,0,48},
	{RACE_SNAKE,CLASS_MAGE,49,49,23,0,49},
	{RACE_SNAKE,CLASS_MAGE,50,50,23,0,50},
	{RACE_SNAKE,CLASS_MAGE,51,51,24,0,51},
	{RACE_SNAKE,CLASS_ALCHEMIST,0,0,0,0,0},
	{RACE_SNAKE,CLASS_ALCHEMIST,1,1,1,0,1},
	{RACE_SNAKE,CLASS_ALCHEMIST,2,2,1,0,2},
	{RACE_SNAKE,CLASS_ALCHEMIST,3,3,1,0,3},
	{RACE_SNAKE,CLASS_ALCHEMIST,4,4,2,0,4},
	{RACE_SNAKE,CLASS_ALCHEMIST,5,5,2,0,5},
	{RACE_SNAKE,CLASS_ALCHEMIST,6,6,2,0,6},
	{RACE_SNAKE,CLASS_ALCHEMIST,7,7,2,0,7},
	{RACE_SNAKE,CLASS_ALCHEMIST,8,8,2,0,8},
	{RACE_SNAKE,CLASS_ALCHEMIST,9,9,2,0,9},
	{RACE_SNAKE,CLASS_ALCHEMIST,10,10,2,0,10},
	{RACE_SNAKE,CLASS_ALCHEMIST,11,11,3,0,11},
	{RACE_SNAKE,CLASS_ALCHEMIST,12,12,3,0,12},
	{RACE_SNAKE,CLASS_ALCHEMIST,13,13,3,0,13},
	{RACE_SNAKE,CLASS_ALCHEMIST,14,14,4,0,14},
	{RACE_SNAKE,CLASS_ALCHEMIST,15,15,4,0,15},
	{RACE_SNAKE,CLASS_ALCHEMIST,16,16,4,0,16},
	{RACE_SNAKE,CLASS_ALCHEMIST,17,17,4,0,17},
	{RACE_SNAKE,CLASS_ALCHEMIST,18,18,4,0,18},
	{RACE_SNAKE,CLASS_ALCHEMIST,19,19,4,0,19},
	{RACE_SNAKE,CLASS_ALCHEMIST,20,20,4,0,20},
	{RACE_SNAKE,CLASS_ALCHEMIST,21,21,4,0,21},
	{RACE_SNAKE,CLASS_ALCHEMIST,22,22,5,0,22},
	{RACE_SNAKE,CLASS_ALCHEMIST,23,23,5,0,23},
	{RACE_SNAKE,CLASS_ALCHEMIST,24,24,5,0,24},
	{RACE_SNAKE,CLASS_ALCHEMIST,25,25,5,0,25},
	{RACE_SNAKE,CLASS_ALCHEMIST,26,26,5,0,26},
	{RACE_SNAKE,CLASS_ALCHEMIST,27,27,5,0,27},
	{RACE_SNAKE,CLASS_ALCHEMIST,28,28,6,0,28},
	{RACE_SNAKE,CLASS_ALCHEMIST,29,29,7,0,29},
	{RACE_SNAKE,CLASS_ALCHEMIST,30,30,7,0,30},
	{RACE_SNAKE,CLASS_ALCHEMIST,31,31,7,0,31},
	{RACE_SNAKE,CLASS_ALCHEMIST,32,32,8,0,32},
	{RACE_SNAKE,CLASS_ALCHEMIST,33,33,8,0,33},
	{RACE_SNAKE,CLASS_ALCHEMIST,34,34,9,0,34},
	{RACE_SNAKE,CLASS_ALCHEMIST,35,35,9,0,35},
	{RACE_SNAKE,CLASS_ALCHEMIST,36,36,9,0,36},
	{RACE_SNAKE,CLASS_ALCHEMIST,37,37,9,0,37},
	{RACE_SNAKE,CLASS_ALCHEMIST,38,38,10,0,38},
	{RACE_SNAKE,CLASS_ALCHEMIST,39,39,11,0,39},
	{RACE_SNAKE,CLASS_ALCHEMIST,40,40,12,0,40},
	{RACE_SNAKE,CLASS_ALCHEMIST,41,41,13,0,41},
	{RACE_SNAKE,CLASS_ALCHEMIST,42,42,14,0,42},
	{RACE_SNAKE,CLASS_ALCHEMIST,43,43,15,0,43},
	{RACE_SNAKE,CLASS_ALCHEMIST,44,44,15,0,44},
	{RACE_SNAKE,CLASS_ALCHEMIST,45,45,16,0,45},
	{RACE_SNAKE,CLASS_ALCHEMIST,46,46,16,0,46},
	{RACE_SNAKE,CLASS_ALCHEMIST,47,47,17,0,47},
	{RACE_SNAKE,CLASS_ALCHEMIST,48,48,18,0,48},
	{RACE_SNAKE,CLASS_ALCHEMIST,49,49,18,0,49},
	{RACE_SNAKE,CLASS_ALCHEMIST,50,50,18,0,50},
	{RACE_SNAKE,CLASS_ALCHEMIST,51,51,19,0,51},
	{RACE_SNAKE,CLASS_PALADIN,0,0,0,0,0},
	{RACE_SNAKE,CLASS_PALADIN,1,1,0,0,1},
	{RACE_SNAKE,CLASS_PALADIN,2,2,0,0,2},
	{RACE_SNAKE,CLASS_PALADIN,3,3,0,0,3},
	{RACE_SNAKE,CLASS_PALADIN,4,4,0,0,4},
	{RACE_SNAKE,CLASS_PALADIN,5,5,0,0,5},
	{RACE_SNAKE,CLASS_PALADIN,6,6,0,0,6},
	{RACE_SNAKE,CLASS_PALADIN,7,7,0,0,7},
	{RACE_SNAKE,CLASS_PALADIN,8,8,0,0,8},
	{RACE_SNAKE,CLASS_PALADIN,9,9,0,0,9},
	{RACE_SNAKE,CLASS_PALADIN,10,10,0,0,10},
	{RACE_SNAKE,CLASS_PALADIN,11,11,0,0,11},
	{RACE_SNAKE,CLASS_PALADIN,12,12,0,0,12},
	{RACE_SNAKE,CLASS_PALADIN,13,13,0,0,13},
	{RACE_SNAKE,CLASS_PALADIN,14,14,0,0,14},
	{RACE_SNAKE,CLASS_PALADIN,15,15,0,0,15},
	{RACE_SNAKE,CLASS_PALADIN,16,16,0,0,16},
	{RACE_SNAKE,CLASS_PALADIN,17,17,0,0,17},
	{RACE_SNAKE,CLASS_PALADIN,18,18,0,0,18},
	{RACE_SNAKE,CLASS_PALADIN,19,19,0,0,19},
	{RACE_SNAKE,CLASS_PALADIN,20,20,0,0,20},
	{RACE_SNAKE,CLASS_PALADIN,21,21,0,0,21},
	{RACE_SNAKE,CLASS_PALADIN,22,22,0,0,22},
	{RACE_SNAKE,CLASS_PALADIN,23,23,0,0,23},
	{RACE_SNAKE,CLASS_PALADIN,24,24,0,0,24},
	{RACE_SNAKE,CLASS_PALADIN,25,25,0,0,25},
	{RACE_SNAKE,CLASS_PALADIN,26,26,0,0,26},
	{RACE_SNAKE,CLASS_PALADIN,27,27,0,0,27},
	{RACE_SNAKE,CLASS_PALADIN,28,28,0,0,28},
	{RACE_SNAKE,CLASS_PALADIN,29,29,0,0,29},
	{RACE_SNAKE,CLASS_PALADIN,30,30,0,0,30},
	{RACE_SNAKE,CLASS_PALADIN,31,31,0,0,31},
	{RACE_SNAKE,CLASS_PALADIN,32,32,0,0,32},
	{RACE_SNAKE,CLASS_PALADIN,33,33,0,0,33},
	{RACE_SNAKE,CLASS_PALADIN,34,34,0,0,34},
	{RACE_SNAKE,CLASS_PALADIN,35,35,0,0,35},
	{RACE_SNAKE,CLASS_PALADIN,36,36,0,0,36},
	{RACE_SNAKE,CLASS_PALADIN,37,37,0,0,37},
	{RACE_SNAKE,CLASS_PALADIN,38,38,0,0,38},
	{RACE_SNAKE,CLASS_PALADIN,39,39,0,0,39},
	{RACE_SNAKE,CLASS_PALADIN,40,40,0,0,40},
	{RACE_SNAKE,CLASS_PALADIN,41,41,0,0,41},
	{RACE_SNAKE,CLASS_PALADIN,42,42,0,0,42},
	{RACE_SNAKE,CLASS_PALADIN,43,43,0,0,43},
	{RACE_SNAKE,CLASS_PALADIN,44,44,0,0,44},
	{RACE_SNAKE,CLASS_PALADIN,45,45,0,0,45},
	{RACE_SNAKE,CLASS_PALADIN,46,46,0,0,46},
	{RACE_SNAKE,CLASS_PALADIN,47,47,0,0,47},
	{RACE_SNAKE,CLASS_PALADIN,48,48,0,0,48},
	{RACE_SNAKE,CLASS_PALADIN,49,49,0,0,49},
	{RACE_SNAKE,CLASS_PALADIN,50,50,0,0,50},
	{RACE_SNAKE,CLASS_PALADIN,51,51,0,0,51},
	{RACE_HUMAN,CLASS_WARRIOR,0,0,0,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,1,1,0,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,2,1,0,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,3,1,0,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,4,1,0,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,5,1,0,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,6,1,1,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,7,2,2,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,8,2,2,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,9,3,1,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,10,4,2,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,11,5,2,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,12,6,2,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,13,7,1,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,14,8,1,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,15,8,1,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,16,8,1,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,17,8,2,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,18,8,2,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,19,8,1,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,20,9,1,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,21,10,1,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,22,10,2,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,23,10,2,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,24,10,2,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,25,10,2,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,26,10,1,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,27,10,2,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,28,10,2,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,29,10,2,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,30,11,3,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,31,11,3,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,32,11,3,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,33,11,2,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,34,11,2,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,35,11,1,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,36,11,1,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,37,11,0,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,38,11,1,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,39,11,1,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,40,11,1,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,41,11,2,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,42,11,3,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,43,11,3,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,44,12,3,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,45,12,3,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,46,12,4,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,47,12,4,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,48,12,4,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,49,13,5,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,50,13,5,0,0},
	{RACE_HUMAN,CLASS_WARRIOR,51,13,5,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,0,0,0,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,1,2,1,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,2,4,1,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,3,5,1,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,4,6,2,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,5,7,2,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,6,8,3,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,7,8,4,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,8,9,4,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,9,10,4,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,10,11,5,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,11,11,5,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,12,13,5,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,13,13,6,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,14,14,7,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,15,15,7,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,16,16,8,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,17,17,9,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,18,18,10,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,19,19,10,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,20,20,11,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,21,20,11,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,22,21,11,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,23,21,11,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,24,22,12,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,25,24,13,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,26,25,14,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,27,25,14,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,28,27,15,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,29,29,15,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,30,29,16,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,31,30,16,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,32,31,17,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,33,31,18,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,34,32,19,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,35,34,20,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,36,35,21,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,37,35,22,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,38,35,23,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,39,36,23,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,40,37,23,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,41,38,23,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,42,38,23,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,43,39,24,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,44,40,24,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,45,42,24,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,46,43,24,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,47,44,24,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,48,46,25,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,49,47,26,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,50,48,26,0,0},
	{RACE_HUMAN,CLASS_NECROMANT,51,49,26,0,0},
	{RACE_HUMAN,CLASS_DRUID,0,0,0,0,0},
	{RACE_HUMAN,CLASS_DRUID,1,1,1,0,0},
	{RACE_HUMAN,CLASS_DRUID,2,2,2,0,0},
	{RACE_HUMAN,CLASS_DRUID,3,3,3,0,0},
	{RACE_HUMAN,CLASS_DRUID,4,4,4,0,0},
	{RACE_HUMAN,CLASS_DRUID,5,5,4,0,0},
	{RACE_HUMAN,CLASS_DRUID,6,6,5,0,0},
	{RACE_HUMAN,CLASS_DRUID,7,7,6,0,0},
	{RACE_HUMAN,CLASS_DRUID,8,8,7,0,0},
	{RACE_HUMAN,CLASS_DRUID,9,9,8,0,0},
	{RACE_HUMAN,CLASS_DRUID,10,10,8,0,0},
	{RACE_HUMAN,CLASS_DRUID,11,11,8,0,0},
	{RACE_HUMAN,CLASS_DRUID,12,12,9,0,0},
	{RACE_HUMAN,CLASS_DRUID,13,13,10,0,0},
	{RACE_HUMAN,CLASS_DRUID,14,14,11,0,0},
	{RACE_HUMAN,CLASS_DRUID,15,15,12,0,0},
	{RACE_HUMAN,CLASS_DRUID,16,16,12,0,0},
	{RACE_HUMAN,CLASS_DRUID,17,17,13,0,0},
	{RACE_HUMAN,CLASS_DRUID,18,18,14,0,0},
	{RACE_HUMAN,CLASS_DRUID,19,19,15,0,0},
	{RACE_HUMAN,CLASS_DRUID,20,20,15,0,0},
	{RACE_HUMAN,CLASS_DRUID,21,21,16,0,0},
	{RACE_HUMAN,CLASS_DRUID,22,22,17,0,0},
	{RACE_HUMAN,CLASS_DRUID,23,23,17,0,0},
	{RACE_HUMAN,CLASS_DRUID,24,24,17,0,0},
	{RACE_HUMAN,CLASS_DRUID,25,25,18,0,0},
	{RACE_HUMAN,CLASS_DRUID,26,26,19,0,0},
	{RACE_HUMAN,CLASS_DRUID,27,27,19,0,0},
	{RACE_HUMAN,CLASS_DRUID,28,28,20,0,0},
	{RACE_HUMAN,CLASS_DRUID,29,29,21,0,0},
	{RACE_HUMAN,CLASS_DRUID,30,30,22,0,0},
	{RACE_HUMAN,CLASS_DRUID,31,31,22,0,0},
	{RACE_HUMAN,CLASS_DRUID,32,32,23,0,0},
	{RACE_HUMAN,CLASS_DRUID,33,33,23,0,0},
	{RACE_HUMAN,CLASS_DRUID,34,34,23,0,0},
	{RACE_HUMAN,CLASS_DRUID,35,35,24,0,0},
	{RACE_HUMAN,CLASS_DRUID,36,36,25,0,0},
	{RACE_HUMAN,CLASS_DRUID,37,37,26,0,0},
	{RACE_HUMAN,CLASS_DRUID,38,38,26,0,0},
	{RACE_HUMAN,CLASS_DRUID,39,39,26,0,0},
	{RACE_HUMAN,CLASS_DRUID,40,40,27,0,0},
	{RACE_HUMAN,CLASS_DRUID,41,41,27,0,0},
	{RACE_HUMAN,CLASS_DRUID,42,42,27,0,0},
	{RACE_HUMAN,CLASS_DRUID,43,43,28,0,0},
	{RACE_HUMAN,CLASS_DRUID,44,44,28,0,0},
	{RACE_HUMAN,CLASS_DRUID,45,45,28,0,0},
	{RACE_HUMAN,CLASS_DRUID,46,46,29,0,0},
	{RACE_HUMAN,CLASS_DRUID,47,47,30,0,0},
	{RACE_HUMAN,CLASS_DRUID,48,48,31,0,0},
	{RACE_HUMAN,CLASS_DRUID,49,49,32,0,0},
	{RACE_HUMAN,CLASS_DRUID,50,50,33,0,0},
	{RACE_HUMAN,CLASS_DRUID,51,51,33,0,0},
	{RACE_HUMAN,CLASS_CLERIC,0,0,0,0,0},
	{RACE_HUMAN,CLASS_CLERIC,1,1,1,0,1},
	{RACE_HUMAN,CLASS_CLERIC,2,2,2,0,2},
	{RACE_HUMAN,CLASS_CLERIC,3,3,2,0,3},
	{RACE_HUMAN,CLASS_CLERIC,4,4,3,0,4},
	{RACE_HUMAN,CLASS_CLERIC,5,5,4,0,5},
	{RACE_HUMAN,CLASS_CLERIC,6,6,4,0,6},
	{RACE_HUMAN,CLASS_CLERIC,7,7,4,0,7},
	{RACE_HUMAN,CLASS_CLERIC,8,8,5,0,8},
	{RACE_HUMAN,CLASS_CLERIC,9,9,6,0,9},
	{RACE_HUMAN,CLASS_CLERIC,10,10,6,0,10},
	{RACE_HUMAN,CLASS_CLERIC,11,11,6,0,11},
	{RACE_HUMAN,CLASS_CLERIC,12,12,6,0,12},
	{RACE_HUMAN,CLASS_CLERIC,13,13,6,0,13},
	{RACE_HUMAN,CLASS_CLERIC,14,14,7,0,14},
	{RACE_HUMAN,CLASS_CLERIC,15,15,7,0,15},
	{RACE_HUMAN,CLASS_CLERIC,16,16,8,0,16},
	{RACE_HUMAN,CLASS_CLERIC,17,17,9,0,17},
	{RACE_HUMAN,CLASS_CLERIC,18,18,10,0,18},
	{RACE_HUMAN,CLASS_CLERIC,19,19,11,0,19},
	{RACE_HUMAN,CLASS_CLERIC,20,20,11,0,20},
	{RACE_HUMAN,CLASS_CLERIC,21,21,12,0,21},
	{RACE_HUMAN,CLASS_CLERIC,22,22,13,0,22},
	{RACE_HUMAN,CLASS_CLERIC,23,23,14,0,23},
	{RACE_HUMAN,CLASS_CLERIC,24,24,14,0,24},
	{RACE_HUMAN,CLASS_CLERIC,25,25,14,0,25},
	{RACE_HUMAN,CLASS_CLERIC,26,26,15,0,26},
	{RACE_HUMAN,CLASS_CLERIC,27,27,16,0,27},
	{RACE_HUMAN,CLASS_CLERIC,28,28,17,0,28},
	{RACE_HUMAN,CLASS_CLERIC,29,29,17,0,29},
	{RACE_HUMAN,CLASS_CLERIC,30,30,17,0,30},
	{RACE_HUMAN,CLASS_CLERIC,31,31,18,0,31},
	{RACE_HUMAN,CLASS_CLERIC,32,32,18,0,32},
	{RACE_HUMAN,CLASS_CLERIC,33,33,18,0,33},
	{RACE_HUMAN,CLASS_CLERIC,34,34,19,0,34},
	{RACE_HUMAN,CLASS_CLERIC,35,35,20,0,35},
	{RACE_HUMAN,CLASS_CLERIC,36,36,21,0,36},
	{RACE_HUMAN,CLASS_CLERIC,37,37,21,0,37},
	{RACE_HUMAN,CLASS_CLERIC,38,38,22,0,38},
	{RACE_HUMAN,CLASS_CLERIC,39,39,23,0,39},
	{RACE_HUMAN,CLASS_CLERIC,40,40,23,0,40},
	{RACE_HUMAN,CLASS_CLERIC,41,41,24,0,41},
	{RACE_HUMAN,CLASS_CLERIC,42,42,25,0,42},
	{RACE_HUMAN,CLASS_CLERIC,43,43,25,0,43},
	{RACE_HUMAN,CLASS_CLERIC,44,44,26,0,44},
	{RACE_HUMAN,CLASS_CLERIC,45,45,27,0,45},
	{RACE_HUMAN,CLASS_CLERIC,46,46,28,0,46},
	{RACE_HUMAN,CLASS_CLERIC,47,47,29,0,47},
	{RACE_HUMAN,CLASS_CLERIC,48,48,29,0,48},
	{RACE_HUMAN,CLASS_CLERIC,49,49,30,0,49},
	{RACE_HUMAN,CLASS_CLERIC,50,50,30,0,50},
	{RACE_HUMAN,CLASS_CLERIC,51,51,31,0,51},
	{RACE_HUMAN,CLASS_THIEF,0,0,0,0,0},
	{RACE_HUMAN,CLASS_THIEF,1,1,1,0,0},
	{RACE_HUMAN,CLASS_THIEF,2,2,0,0,0},
	{RACE_HUMAN,CLASS_THIEF,3,3,1,0,0},
	{RACE_HUMAN,CLASS_THIEF,4,4,2,0,0},
	{RACE_HUMAN,CLASS_THIEF,5,5,2,0,0},
	{RACE_HUMAN,CLASS_THIEF,6,6,1,0,0},
	{RACE_HUMAN,CLASS_THIEF,7,7,1,0,0},
	{RACE_HUMAN,CLASS_THIEF,8,8,2,0,0},
	{RACE_HUMAN,CLASS_THIEF,9,9,1,0,0},
	{RACE_HUMAN,CLASS_THIEF,10,10,0,0,0},
	{RACE_HUMAN,CLASS_THIEF,11,11,1,0,0},
	{RACE_HUMAN,CLASS_THIEF,12,12,0,0,0},
	{RACE_HUMAN,CLASS_THIEF,13,13,0,0,0},
	{RACE_HUMAN,CLASS_THIEF,14,14,0,0,0},
	{RACE_HUMAN,CLASS_THIEF,15,15,1,0,0},
	{RACE_HUMAN,CLASS_THIEF,16,16,0,0,0},
	{RACE_HUMAN,CLASS_THIEF,17,17,0,0,0},
	{RACE_HUMAN,CLASS_THIEF,18,18,0,0,0},
	{RACE_HUMAN,CLASS_THIEF,19,19,1,0,0},
	{RACE_HUMAN,CLASS_THIEF,20,20,1,0,0},
	{RACE_HUMAN,CLASS_THIEF,21,21,2,0,0},
	{RACE_HUMAN,CLASS_THIEF,22,22,2,0,0},
	{RACE_HUMAN,CLASS_THIEF,23,23,2,0,0},
	{RACE_HUMAN,CLASS_THIEF,24,24,2,0,0},
	{RACE_HUMAN,CLASS_THIEF,25,25,2,0,0},
	{RACE_HUMAN,CLASS_THIEF,26,26,2,0,0},
	{RACE_HUMAN,CLASS_THIEF,27,27,2,0,0},
	{RACE_HUMAN,CLASS_THIEF,28,28,2,0,0},
	{RACE_HUMAN,CLASS_THIEF,29,29,3,0,0},
	{RACE_HUMAN,CLASS_THIEF,30,30,3,0,0},
	{RACE_HUMAN,CLASS_THIEF,31,31,3,0,0},
	{RACE_HUMAN,CLASS_THIEF,32,32,3,0,0},
	{RACE_HUMAN,CLASS_THIEF,33,33,4,0,0},
	{RACE_HUMAN,CLASS_THIEF,34,34,4,0,0},
	{RACE_HUMAN,CLASS_THIEF,35,35,4,0,0},
	{RACE_HUMAN,CLASS_THIEF,36,36,3,0,0},
	{RACE_HUMAN,CLASS_THIEF,37,37,3,0,0},
	{RACE_HUMAN,CLASS_THIEF,38,38,4,0,0},
	{RACE_HUMAN,CLASS_THIEF,39,39,4,0,0},
	{RACE_HUMAN,CLASS_THIEF,40,40,5,0,0},
	{RACE_HUMAN,CLASS_THIEF,41,41,5,0,0},
	{RACE_HUMAN,CLASS_THIEF,42,42,4,0,0},
	{RACE_HUMAN,CLASS_THIEF,43,43,3,0,0},
	{RACE_HUMAN,CLASS_THIEF,44,44,3,0,0},
	{RACE_HUMAN,CLASS_THIEF,45,45,3,0,0},
	{RACE_HUMAN,CLASS_THIEF,46,46,3,0,0},
	{RACE_HUMAN,CLASS_THIEF,47,47,2,0,0},
	{RACE_HUMAN,CLASS_THIEF,48,48,3,0,0},
	{RACE_HUMAN,CLASS_THIEF,49,49,2,0,0},
	{RACE_HUMAN,CLASS_THIEF,50,50,1,0,0},
	{RACE_HUMAN,CLASS_THIEF,51,51,2,0,0},
	{RACE_HUMAN,CLASS_RANGER,0,0,0,0,0},
	{RACE_HUMAN,CLASS_RANGER,1,1,1,0,0},
	{RACE_HUMAN,CLASS_RANGER,2,2,1,0,0},
	{RACE_HUMAN,CLASS_RANGER,3,3,0,0,0},
	{RACE_HUMAN,CLASS_RANGER,4,4,0,0,0},
	{RACE_HUMAN,CLASS_RANGER,5,5,0,0,0},
	{RACE_HUMAN,CLASS_RANGER,6,6,0,0,0},
	{RACE_HUMAN,CLASS_RANGER,7,7,0,0,0},
	{RACE_HUMAN,CLASS_RANGER,8,8,0,0,0},
	{RACE_HUMAN,CLASS_RANGER,9,9,1,0,0},
	{RACE_HUMAN,CLASS_RANGER,10,10,1,0,0},
	{RACE_HUMAN,CLASS_RANGER,11,11,2,0,0},
	{RACE_HUMAN,CLASS_RANGER,12,12,2,0,0},
	{RACE_HUMAN,CLASS_RANGER,13,13,2,0,0},
	{RACE_HUMAN,CLASS_RANGER,14,14,3,0,0},
	{RACE_HUMAN,CLASS_RANGER,15,15,3,0,0},
	{RACE_HUMAN,CLASS_RANGER,16,16,3,0,0},
	{RACE_HUMAN,CLASS_RANGER,17,17,2,0,0},
	{RACE_HUMAN,CLASS_RANGER,18,18,1,0,0},
	{RACE_HUMAN,CLASS_RANGER,19,19,1,0,0},
	{RACE_HUMAN,CLASS_RANGER,20,20,1,0,0},
	{RACE_HUMAN,CLASS_RANGER,21,21,0,0,0},
	{RACE_HUMAN,CLASS_RANGER,22,22,1,0,0},
	{RACE_HUMAN,CLASS_RANGER,23,23,1,0,0},
	{RACE_HUMAN,CLASS_RANGER,24,24,2,0,0},
	{RACE_HUMAN,CLASS_RANGER,25,25,3,0,0},
	{RACE_HUMAN,CLASS_RANGER,26,26,4,0,0},
	{RACE_HUMAN,CLASS_RANGER,27,27,4,0,0},
	{RACE_HUMAN,CLASS_RANGER,28,28,4,0,0},
	{RACE_HUMAN,CLASS_RANGER,29,29,3,0,0},
	{RACE_HUMAN,CLASS_RANGER,30,30,2,0,0},
	{RACE_HUMAN,CLASS_RANGER,31,31,1,0,0},
	{RACE_HUMAN,CLASS_RANGER,32,32,2,0,0},
	{RACE_HUMAN,CLASS_RANGER,33,33,2,0,0},
	{RACE_HUMAN,CLASS_RANGER,34,34,2,0,0},
	{RACE_HUMAN,CLASS_RANGER,35,35,2,0,0},
	{RACE_HUMAN,CLASS_RANGER,36,36,3,0,0},
	{RACE_HUMAN,CLASS_RANGER,37,37,4,0,0},
	{RACE_HUMAN,CLASS_RANGER,38,38,3,0,0},
	{RACE_HUMAN,CLASS_RANGER,39,39,3,0,0},
	{RACE_HUMAN,CLASS_RANGER,40,40,2,0,0},
	{RACE_HUMAN,CLASS_RANGER,41,41,1,0,0},
	{RACE_HUMAN,CLASS_RANGER,42,42,1,0,0},
	{RACE_HUMAN,CLASS_RANGER,43,43,1,0,0},
	{RACE_HUMAN,CLASS_RANGER,44,44,2,0,0},
	{RACE_HUMAN,CLASS_RANGER,45,45,2,0,0},
	{RACE_HUMAN,CLASS_RANGER,46,46,3,0,0},
	{RACE_HUMAN,CLASS_RANGER,47,47,2,0,0},
	{RACE_HUMAN,CLASS_RANGER,48,48,2,0,0},
	{RACE_HUMAN,CLASS_RANGER,49,49,3,0,0},
	{RACE_HUMAN,CLASS_RANGER,50,50,3,0,0},
	{RACE_HUMAN,CLASS_RANGER,51,51,3,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,0,0,0,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,1,2,1,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,2,4,2,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,3,5,3,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,4,6,4,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,5,7,5,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,6,9,6,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,7,10,7,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,8,10,8,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,9,12,9,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,10,12,10,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,11,14,11,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,12,16,12,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,13,18,13,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,14,18,14,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,15,19,14,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,16,20,14,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,17,20,15,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,18,22,16,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,19,23,17,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,20,24,18,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,21,25,19,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,22,26,20,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,23,27,21,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,24,29,21,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,25,31,22,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,26,32,23,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,27,32,24,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,28,33,25,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,29,34,26,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,30,34,27,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,31,35,27,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,32,36,28,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,33,37,29,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,34,38,30,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,35,39,30,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,36,40,31,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,37,41,32,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,38,43,33,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,39,44,34,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,40,44,35,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,41,46,36,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,42,46,36,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,43,47,37,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,44,48,38,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,45,49,39,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,46,50,40,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,47,51,41,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,48,53,42,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,49,54,43,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,50,55,44,0,0},
	{RACE_HUMAN,CLASS_NAZGUL,51,55,45,0,0},
	{RACE_HUMAN,CLASS_MAGE,0,0,0,0,0},
	{RACE_HUMAN,CLASS_MAGE,1,1,0,0,0},
	{RACE_HUMAN,CLASS_MAGE,2,2,0,0,0},
	{RACE_HUMAN,CLASS_MAGE,3,3,0,0,0},
	{RACE_HUMAN,CLASS_MAGE,4,4,0,0,0},
	{RACE_HUMAN,CLASS_MAGE,5,5,0,0,0},
	{RACE_HUMAN,CLASS_MAGE,6,7,0,0,0},
	{RACE_HUMAN,CLASS_MAGE,7,8,0,0,0},
	{RACE_HUMAN,CLASS_MAGE,8,10,0,0,0},
	{RACE_HUMAN,CLASS_MAGE,9,11,1,0,0},
	{RACE_HUMAN,CLASS_MAGE,10,12,2,0,0},
	{RACE_HUMAN,CLASS_MAGE,11,13,2,0,0},
	{RACE_HUMAN,CLASS_MAGE,12,15,2,0,0},
	{RACE_HUMAN,CLASS_MAGE,13,16,2,0,0},
	{RACE_HUMAN,CLASS_MAGE,14,18,3,0,0},
	{RACE_HUMAN,CLASS_MAGE,15,20,4,0,0},
	{RACE_HUMAN,CLASS_MAGE,16,22,5,0,0},
	{RACE_HUMAN,CLASS_MAGE,17,23,5,0,0},
	{RACE_HUMAN,CLASS_MAGE,18,24,5,0,0},
	{RACE_HUMAN,CLASS_MAGE,19,25,6,0,0},
	{RACE_HUMAN,CLASS_MAGE,20,26,7,0,0},
	{RACE_HUMAN,CLASS_MAGE,21,27,7,0,0},
	{RACE_HUMAN,CLASS_MAGE,22,28,7,0,0},
	{RACE_HUMAN,CLASS_MAGE,23,30,8,0,0},
	{RACE_HUMAN,CLASS_MAGE,24,30,9,0,0},
	{RACE_HUMAN,CLASS_MAGE,25,31,9,0,0},
	{RACE_HUMAN,CLASS_MAGE,26,32,10,0,0},
	{RACE_HUMAN,CLASS_MAGE,27,33,10,0,0},
	{RACE_HUMAN,CLASS_MAGE,28,33,10,0,0},
	{RACE_HUMAN,CLASS_MAGE,29,35,11,0,0},
	{RACE_HUMAN,CLASS_MAGE,30,37,11,0,0},
	{RACE_HUMAN,CLASS_MAGE,31,38,11,0,0},
	{RACE_HUMAN,CLASS_MAGE,32,39,11,0,0},
	{RACE_HUMAN,CLASS_MAGE,33,41,11,0,0},
	{RACE_HUMAN,CLASS_MAGE,34,42,12,0,0},
	{RACE_HUMAN,CLASS_MAGE,35,43,12,0,0},
	{RACE_HUMAN,CLASS_MAGE,36,44,13,0,0},
	{RACE_HUMAN,CLASS_MAGE,37,45,13,0,0},
	{RACE_HUMAN,CLASS_MAGE,38,46,13,0,0},
	{RACE_HUMAN,CLASS_MAGE,39,47,14,0,0},
	{RACE_HUMAN,CLASS_MAGE,40,47,15,0,0},
	{RACE_HUMAN,CLASS_MAGE,41,47,16,0,0},
	{RACE_HUMAN,CLASS_MAGE,42,48,17,0,0},
	{RACE_HUMAN,CLASS_MAGE,43,49,18,0,0},
	{RACE_HUMAN,CLASS_MAGE,44,50,18,0,0},
	{RACE_HUMAN,CLASS_MAGE,45,51,19,0,0},
	{RACE_HUMAN,CLASS_MAGE,46,52,20,0,0},
	{RACE_HUMAN,CLASS_MAGE,47,52,20,0,0},
	{RACE_HUMAN,CLASS_MAGE,48,53,21,0,0},
	{RACE_HUMAN,CLASS_MAGE,49,55,22,0,0},
	{RACE_HUMAN,CLASS_MAGE,50,56,22,0,0},
	{RACE_HUMAN,CLASS_MAGE,51,58,22,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,0,0,0,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,1,1,0,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,2,2,0,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,3,3,0,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,4,4,0,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,5,5,0,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,6,6,1,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,7,7,1,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,8,8,1,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,9,9,2,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,10,10,2,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,11,11,2,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,12,12,2,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,13,13,3,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,14,14,4,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,15,15,4,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,16,16,4,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,17,17,4,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,18,18,5,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,19,19,5,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,20,20,5,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,21,21,5,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,22,22,5,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,23,23,6,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,24,24,6,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,25,25,6,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,26,26,6,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,27,27,7,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,28,28,8,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,29,29,9,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,30,30,9,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,31,31,9,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,32,32,10,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,33,33,11,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,34,34,11,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,35,35,11,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,36,36,11,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,37,37,12,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,38,38,13,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,39,39,13,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,40,40,14,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,41,41,15,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,42,42,15,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,43,43,16,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,44,44,17,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,45,45,17,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,46,46,17,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,47,47,17,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,48,48,17,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,49,49,18,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,50,50,19,0,0},
	{RACE_HUMAN,CLASS_ALCHEMIST,51,51,19,0,0},
	{RACE_HUMAN,CLASS_PALADIN,0,0,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,1,1,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,2,2,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,3,3,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,4,4,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,5,5,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,6,6,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,7,7,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,8,8,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,9,9,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,10,10,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,11,11,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,12,12,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,13,13,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,14,14,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,15,15,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,16,16,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,17,17,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,18,18,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,19,19,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,20,20,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,21,21,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,22,22,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,23,23,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,24,24,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,25,25,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,26,26,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,27,27,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,28,28,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,29,29,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,30,30,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,31,31,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,32,32,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,33,33,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,34,34,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,35,35,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,36,36,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,37,37,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,38,38,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,39,39,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,40,40,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,41,41,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,42,42,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,43,43,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,44,44,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,45,45,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,46,46,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,47,47,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,48,48,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,49,49,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,50,50,0,0,0},
	{RACE_HUMAN,CLASS_PALADIN,51,51,0,0,0},
	{RACE_ORC,CLASS_WARRIOR,0,0,0,0,0},
	{RACE_ORC,CLASS_WARRIOR,1,1,1,0,0},
	{RACE_ORC,CLASS_WARRIOR,2,1,1,0,0},
	{RACE_ORC,CLASS_WARRIOR,3,1,1,0,0},
	{RACE_ORC,CLASS_WARRIOR,4,1,1,0,0},
	{RACE_ORC,CLASS_WARRIOR,5,1,0,0,0},
	{RACE_ORC,CLASS_WARRIOR,6,0,0,0,0},
	{RACE_ORC,CLASS_WARRIOR,7,0,0,0,0},
	{RACE_ORC,CLASS_WARRIOR,8,0,0,0,0},
	{RACE_ORC,CLASS_WARRIOR,9,1,0,0,0},
	{RACE_ORC,CLASS_WARRIOR,10,0,0,0,0},
	{RACE_ORC,CLASS_WARRIOR,11,0,1,0,0},
	{RACE_ORC,CLASS_WARRIOR,12,1,0,0,0},
	{RACE_ORC,CLASS_WARRIOR,13,0,0,0,0},
	{RACE_ORC,CLASS_WARRIOR,14,0,0,0,0},
	{RACE_ORC,CLASS_WARRIOR,15,0,1,0,0},
	{RACE_ORC,CLASS_WARRIOR,16,0,1,0,0},
	{RACE_ORC,CLASS_WARRIOR,17,1,1,0,0},
	{RACE_ORC,CLASS_WARRIOR,18,1,1,0,0},
	{RACE_ORC,CLASS_WARRIOR,19,0,2,0,0},
	{RACE_ORC,CLASS_WARRIOR,20,1,2,0,0},
	{RACE_ORC,CLASS_WARRIOR,21,2,3,0,0},
	{RACE_ORC,CLASS_WARRIOR,22,1,2,0,0},
	{RACE_ORC,CLASS_WARRIOR,23,1,2,0,0},
	{RACE_ORC,CLASS_WARRIOR,24,1,2,0,0},
	{RACE_ORC,CLASS_WARRIOR,25,0,2,0,0},
	{RACE_ORC,CLASS_WARRIOR,26,0,2,0,0},
	{RACE_ORC,CLASS_WARRIOR,27,0,1,0,0},
	{RACE_ORC,CLASS_WARRIOR,28,0,1,0,0},
	{RACE_ORC,CLASS_WARRIOR,29,0,1,0,0},
	{RACE_ORC,CLASS_WARRIOR,30,1,0,0,0},
	{RACE_ORC,CLASS_WARRIOR,31,2,1,0,0},
	{RACE_ORC,CLASS_WARRIOR,32,1,1,0,0},
	{RACE_ORC,CLASS_WARRIOR,33,1,2,0,0},
	{RACE_ORC,CLASS_WARRIOR,34,1,3,0,0},
	{RACE_ORC,CLASS_WARRIOR,35,1,4,0,0},
	{RACE_ORC,CLASS_WARRIOR,36,0,5,0,0},
	{RACE_ORC,CLASS_WARRIOR,37,1,5,0,0},
	{RACE_ORC,CLASS_WARRIOR,38,1,5,0,0},
	{RACE_ORC,CLASS_WARRIOR,39,2,6,0,0},
	{RACE_ORC,CLASS_WARRIOR,40,2,6,0,0},
	{RACE_ORC,CLASS_WARRIOR,41,2,6,0,0},
	{RACE_ORC,CLASS_WARRIOR,42,2,6,0,0},
	{RACE_ORC,CLASS_WARRIOR,43,1,6,0,0},
	{RACE_ORC,CLASS_WARRIOR,44,1,5,0,0},
	{RACE_ORC,CLASS_WARRIOR,45,0,4,0,0},
	{RACE_ORC,CLASS_WARRIOR,46,1,3,0,0},
	{RACE_ORC,CLASS_WARRIOR,47,1,3,0,0},
	{RACE_ORC,CLASS_WARRIOR,48,1,4,0,0},
	{RACE_ORC,CLASS_WARRIOR,49,2,5,0,0},
	{RACE_ORC,CLASS_WARRIOR,50,2,5,0,0},
	{RACE_ORC,CLASS_WARRIOR,51,2,5,0,0},
	{RACE_ORC,CLASS_NECROMANT,0,0,0,0,0},
	{RACE_ORC,CLASS_NECROMANT,1,1,0,0,0},
	{RACE_ORC,CLASS_NECROMANT,2,1,0,0,0},
	{RACE_ORC,CLASS_NECROMANT,3,1,1,0,0},
	{RACE_ORC,CLASS_NECROMANT,4,1,0,0,0},
	{RACE_ORC,CLASS_NECROMANT,5,1,1,0,0},
	{RACE_ORC,CLASS_NECROMANT,6,1,0,0,0},
	{RACE_ORC,CLASS_NECROMANT,7,1,0,0,0},
	{RACE_ORC,CLASS_NECROMANT,8,0,1,0,0},
	{RACE_ORC,CLASS_NECROMANT,9,1,0,0,0},
	{RACE_ORC,CLASS_NECROMANT,10,1,1,0,0},
	{RACE_ORC,CLASS_NECROMANT,11,1,1,0,0},
	{RACE_ORC,CLASS_NECROMANT,12,1,1,0,0},
	{RACE_ORC,CLASS_NECROMANT,13,1,1,0,0},
	{RACE_ORC,CLASS_NECROMANT,14,2,0,0,0},
	{RACE_ORC,CLASS_NECROMANT,15,3,1,0,0},
	{RACE_ORC,CLASS_NECROMANT,16,2,1,0,0},
	{RACE_ORC,CLASS_NECROMANT,17,2,2,0,0},
	{RACE_ORC,CLASS_NECROMANT,18,1,2,0,0},
	{RACE_ORC,CLASS_NECROMANT,19,0,3,0,0},
	{RACE_ORC,CLASS_NECROMANT,20,0,3,0,0},
	{RACE_ORC,CLASS_NECROMANT,21,1,3,0,0},
	{RACE_ORC,CLASS_NECROMANT,22,0,3,0,0},
	{RACE_ORC,CLASS_NECROMANT,23,0,4,0,0},
	{RACE_ORC,CLASS_NECROMANT,24,0,4,0,0},
	{RACE_ORC,CLASS_NECROMANT,25,1,4,0,0},
	{RACE_ORC,CLASS_NECROMANT,26,0,5,0,0},
	{RACE_ORC,CLASS_NECROMANT,27,0,5,0,0},
	{RACE_ORC,CLASS_NECROMANT,28,0,5,0,0},
	{RACE_ORC,CLASS_NECROMANT,29,0,5,0,0},
	{RACE_ORC,CLASS_NECROMANT,30,0,5,0,0},
	{RACE_ORC,CLASS_NECROMANT,31,0,5,0,0},
	{RACE_ORC,CLASS_NECROMANT,32,0,5,0,0},
	{RACE_ORC,CLASS_NECROMANT,33,0,4,0,0},
	{RACE_ORC,CLASS_NECROMANT,34,0,3,0,0},
	{RACE_ORC,CLASS_NECROMANT,35,0,2,0,0},
	{RACE_ORC,CLASS_NECROMANT,36,0,2,0,0},
	{RACE_ORC,CLASS_NECROMANT,37,1,3,0,0},
	{RACE_ORC,CLASS_NECROMANT,38,1,4,0,0},
	{RACE_ORC,CLASS_NECROMANT,39,2,4,0,0},
	{RACE_ORC,CLASS_NECROMANT,40,2,4,0,0},
	{RACE_ORC,CLASS_NECROMANT,41,2,5,0,0},
	{RACE_ORC,CLASS_NECROMANT,42,2,5,0,0},
	{RACE_ORC,CLASS_NECROMANT,43,2,6,0,0},
	{RACE_ORC,CLASS_NECROMANT,44,2,6,0,0},
	{RACE_ORC,CLASS_NECROMANT,45,2,5,0,0},
	{RACE_ORC,CLASS_NECROMANT,46,2,4,0,0},
	{RACE_ORC,CLASS_NECROMANT,47,2,4,0,0},
	{RACE_ORC,CLASS_NECROMANT,48,1,4,0,0},
	{RACE_ORC,CLASS_NECROMANT,49,1,4,0,0},
	{RACE_ORC,CLASS_NECROMANT,50,1,4,0,0},
	{RACE_ORC,CLASS_NECROMANT,51,1,4,0,0},
	{RACE_ORC,CLASS_DRUID,0,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,1,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,2,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,3,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,4,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,5,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,6,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,7,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,8,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,9,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,10,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,11,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,12,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,13,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,14,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,15,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,16,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,17,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,18,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,19,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,20,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,21,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,22,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,23,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,24,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,25,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,26,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,27,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,28,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,29,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,30,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,31,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,32,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,33,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,34,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,35,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,36,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,37,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,38,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,39,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,40,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,41,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,42,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,43,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,44,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,45,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,46,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,47,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,48,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,49,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,50,0,0,0,0},
	{RACE_ORC,CLASS_DRUID,51,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,0,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,1,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,2,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,3,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,4,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,5,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,6,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,7,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,8,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,9,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,10,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,11,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,12,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,13,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,14,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,15,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,16,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,17,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,18,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,19,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,20,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,21,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,22,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,23,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,24,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,25,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,26,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,27,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,28,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,29,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,30,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,31,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,32,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,33,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,34,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,35,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,36,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,37,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,38,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,39,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,40,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,41,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,42,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,43,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,44,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,45,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,46,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,47,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,48,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,49,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,50,0,0,0,0},
	{RACE_ORC,CLASS_CLERIC,51,0,0,0,0},
	{RACE_ORC,CLASS_THIEF,0,0,0,0,0},
	{RACE_ORC,CLASS_THIEF,1,0,1,0,0},
	{RACE_ORC,CLASS_THIEF,2,0,1,0,0},
	{RACE_ORC,CLASS_THIEF,3,0,1,0,0},
	{RACE_ORC,CLASS_THIEF,4,0,1,0,0},
	{RACE_ORC,CLASS_THIEF,5,0,0,0,0},
	{RACE_ORC,CLASS_THIEF,6,0,0,0,0},
	{RACE_ORC,CLASS_THIEF,7,0,1,0,0},
	{RACE_ORC,CLASS_THIEF,8,0,0,0,0},
	{RACE_ORC,CLASS_THIEF,9,0,0,0,0},
	{RACE_ORC,CLASS_THIEF,10,0,0,0,0},
	{RACE_ORC,CLASS_THIEF,11,0,0,0,0},
	{RACE_ORC,CLASS_THIEF,12,0,1,0,0},
	{RACE_ORC,CLASS_THIEF,13,0,0,0,0},
	{RACE_ORC,CLASS_THIEF,14,0,1,0,0},
	{RACE_ORC,CLASS_THIEF,15,0,1,0,0},
	{RACE_ORC,CLASS_THIEF,16,0,1,0,0},
	{RACE_ORC,CLASS_THIEF,17,0,1,0,0},
	{RACE_ORC,CLASS_THIEF,18,0,1,0,0},
	{RACE_ORC,CLASS_THIEF,19,0,2,0,0},
	{RACE_ORC,CLASS_THIEF,20,0,2,0,0},
	{RACE_ORC,CLASS_THIEF,21,0,3,0,0},
	{RACE_ORC,CLASS_THIEF,22,0,4,0,0},
	{RACE_ORC,CLASS_THIEF,23,0,3,0,0},
	{RACE_ORC,CLASS_THIEF,24,0,4,0,0},
	{RACE_ORC,CLASS_THIEF,25,0,3,0,0},
	{RACE_ORC,CLASS_THIEF,26,0,2,0,0},
	{RACE_ORC,CLASS_THIEF,27,0,2,0,0},
	{RACE_ORC,CLASS_THIEF,28,0,2,0,0},
	{RACE_ORC,CLASS_THIEF,29,0,2,0,0},
	{RACE_ORC,CLASS_THIEF,30,0,2,0,0},
	{RACE_ORC,CLASS_THIEF,31,0,3,0,0},
	{RACE_ORC,CLASS_THIEF,32,0,2,0,0},
	{RACE_ORC,CLASS_THIEF,33,0,1,0,0},
	{RACE_ORC,CLASS_THIEF,34,0,0,0,0},
	{RACE_ORC,CLASS_THIEF,35,0,1,0,0},
	{RACE_ORC,CLASS_THIEF,36,0,1,0,0},
	{RACE_ORC,CLASS_THIEF,37,0,2,0,0},
	{RACE_ORC,CLASS_THIEF,38,0,3,0,0},
	{RACE_ORC,CLASS_THIEF,39,0,4,0,0},
	{RACE_ORC,CLASS_THIEF,40,0,5,0,0},
	{RACE_ORC,CLASS_THIEF,41,0,5,0,0},
	{RACE_ORC,CLASS_THIEF,42,0,5,0,0},
	{RACE_ORC,CLASS_THIEF,43,0,5,0,0},
	{RACE_ORC,CLASS_THIEF,44,0,4,0,0},
	{RACE_ORC,CLASS_THIEF,45,0,5,0,0},
	{RACE_ORC,CLASS_THIEF,46,0,5,0,0},
	{RACE_ORC,CLASS_THIEF,47,0,5,0,0},
	{RACE_ORC,CLASS_THIEF,48,0,5,0,0},
	{RACE_ORC,CLASS_THIEF,49,0,5,0,0},
	{RACE_ORC,CLASS_THIEF,50,0,5,0,0},
	{RACE_ORC,CLASS_THIEF,51,0,6,0,0},
	{RACE_ORC,CLASS_RANGER,0,0,0,0,0},
	{RACE_ORC,CLASS_RANGER,1,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,2,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,3,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,4,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,5,0,0,0,0},
	{RACE_ORC,CLASS_RANGER,6,0,0,0,0},
	{RACE_ORC,CLASS_RANGER,7,0,0,0,0},
	{RACE_ORC,CLASS_RANGER,8,0,0,0,0},
	{RACE_ORC,CLASS_RANGER,9,0,0,0,0},
	{RACE_ORC,CLASS_RANGER,10,0,0,0,0},
	{RACE_ORC,CLASS_RANGER,11,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,12,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,13,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,14,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,15,0,0,0,0},
	{RACE_ORC,CLASS_RANGER,16,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,17,0,0,0,0},
	{RACE_ORC,CLASS_RANGER,18,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,19,0,0,0,0},
	{RACE_ORC,CLASS_RANGER,20,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,21,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,22,0,0,0,0},
	{RACE_ORC,CLASS_RANGER,23,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,24,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,25,0,2,0,0},
	{RACE_ORC,CLASS_RANGER,26,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,27,0,0,0,0},
	{RACE_ORC,CLASS_RANGER,28,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,29,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,30,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,31,0,0,0,0},
	{RACE_ORC,CLASS_RANGER,32,0,0,0,0},
	{RACE_ORC,CLASS_RANGER,33,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,34,0,0,0,0},
	{RACE_ORC,CLASS_RANGER,35,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,36,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,37,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,38,0,2,0,0},
	{RACE_ORC,CLASS_RANGER,39,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,40,0,0,0,0},
	{RACE_ORC,CLASS_RANGER,41,0,0,0,0},
	{RACE_ORC,CLASS_RANGER,42,0,0,0,0},
	{RACE_ORC,CLASS_RANGER,43,0,0,0,0},
	{RACE_ORC,CLASS_RANGER,44,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,45,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,46,0,0,0,0},
	{RACE_ORC,CLASS_RANGER,47,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,48,0,2,0,0},
	{RACE_ORC,CLASS_RANGER,49,0,1,0,0},
	{RACE_ORC,CLASS_RANGER,50,0,2,0,0},
	{RACE_ORC,CLASS_RANGER,51,0,1,0,0},
	{RACE_ORC,CLASS_NAZGUL,0,0,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,1,1,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,2,1,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,3,1,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,4,1,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,5,0,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,6,0,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,7,0,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,8,0,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,9,1,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,10,1,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,11,1,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,12,1,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,13,2,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,14,3,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,15,3,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,16,3,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,17,4,1,0,0},
	{RACE_ORC,CLASS_NAZGUL,18,3,1,0,0},
	{RACE_ORC,CLASS_NAZGUL,19,3,1,0,0},
	{RACE_ORC,CLASS_NAZGUL,20,2,1,0,0},
	{RACE_ORC,CLASS_NAZGUL,21,1,1,0,0},
	{RACE_ORC,CLASS_NAZGUL,22,1,1,0,0},
	{RACE_ORC,CLASS_NAZGUL,23,1,1,0,0},
	{RACE_ORC,CLASS_NAZGUL,24,0,1,0,0},
	{RACE_ORC,CLASS_NAZGUL,25,1,1,0,0},
	{RACE_ORC,CLASS_NAZGUL,26,1,1,0,0},
	{RACE_ORC,CLASS_NAZGUL,27,0,1,0,0},
	{RACE_ORC,CLASS_NAZGUL,28,1,1,0,0},
	{RACE_ORC,CLASS_NAZGUL,29,1,1,0,0},
	{RACE_ORC,CLASS_NAZGUL,30,0,1,0,0},
	{RACE_ORC,CLASS_NAZGUL,31,1,1,0,0},
	{RACE_ORC,CLASS_NAZGUL,32,1,1,0,0},
	{RACE_ORC,CLASS_NAZGUL,33,1,1,0,0},
	{RACE_ORC,CLASS_NAZGUL,34,1,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,35,0,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,36,1,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,37,1,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,38,2,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,39,2,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,40,2,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,41,3,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,42,2,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,43,3,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,44,4,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,45,3,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,46,2,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,47,1,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,48,0,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,49,1,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,50,1,0,0,0},
	{RACE_ORC,CLASS_NAZGUL,51,1,0,0,0},
	{RACE_ORC,CLASS_MAGE,0,0,0,0,0},
	{RACE_ORC,CLASS_MAGE,1,0,1,0,0},
	{RACE_ORC,CLASS_MAGE,2,0,1,0,0},
	{RACE_ORC,CLASS_MAGE,3,0,0,0,0},
	{RACE_ORC,CLASS_MAGE,4,0,1,0,0},
	{RACE_ORC,CLASS_MAGE,5,0,1,0,0},
	{RACE_ORC,CLASS_MAGE,6,0,1,0,0},
	{RACE_ORC,CLASS_MAGE,7,0,1,0,0},
	{RACE_ORC,CLASS_MAGE,8,0,2,0,0},
	{RACE_ORC,CLASS_MAGE,9,0,3,0,0},
	{RACE_ORC,CLASS_MAGE,10,0,4,0,0},
	{RACE_ORC,CLASS_MAGE,11,0,4,0,0},
	{RACE_ORC,CLASS_MAGE,12,0,4,0,0},
	{RACE_ORC,CLASS_MAGE,13,0,3,0,0},
	{RACE_ORC,CLASS_MAGE,14,0,3,0,0},
	{RACE_ORC,CLASS_MAGE,15,0,4,0,0},
	{RACE_ORC,CLASS_MAGE,16,0,4,0,0},
	{RACE_ORC,CLASS_MAGE,17,0,3,0,0},
	{RACE_ORC,CLASS_MAGE,18,0,3,0,0},
	{RACE_ORC,CLASS_MAGE,19,0,2,0,0},
	{RACE_ORC,CLASS_MAGE,20,0,2,0,0},
	{RACE_ORC,CLASS_MAGE,21,0,3,0,0},
	{RACE_ORC,CLASS_MAGE,22,0,3,0,0},
	{RACE_ORC,CLASS_MAGE,23,0,3,0,0},
	{RACE_ORC,CLASS_MAGE,24,0,4,0,0},
	{RACE_ORC,CLASS_MAGE,25,0,3,0,0},
	{RACE_ORC,CLASS_MAGE,26,0,3,0,0},
	{RACE_ORC,CLASS_MAGE,27,0,3,0,0},
	{RACE_ORC,CLASS_MAGE,28,0,4,0,0},
	{RACE_ORC,CLASS_MAGE,29,0,4,0,0},
	{RACE_ORC,CLASS_MAGE,30,0,4,0,0},
	{RACE_ORC,CLASS_MAGE,31,0,4,0,0},
	{RACE_ORC,CLASS_MAGE,32,0,5,0,0},
	{RACE_ORC,CLASS_MAGE,33,0,5,0,0},
	{RACE_ORC,CLASS_MAGE,34,0,4,0,0},
	{RACE_ORC,CLASS_MAGE,35,0,4,0,0},
	{RACE_ORC,CLASS_MAGE,36,0,4,0,0},
	{RACE_ORC,CLASS_MAGE,37,0,4,0,0},
	{RACE_ORC,CLASS_MAGE,38,0,3,0,0},
	{RACE_ORC,CLASS_MAGE,39,0,3,0,0},
	{RACE_ORC,CLASS_MAGE,40,0,3,0,0},
	{RACE_ORC,CLASS_MAGE,41,0,3,0,0},
	{RACE_ORC,CLASS_MAGE,42,0,3,0,0},
	{RACE_ORC,CLASS_MAGE,43,0,2,0,0},
	{RACE_ORC,CLASS_MAGE,44,0,3,0,0},
	{RACE_ORC,CLASS_MAGE,45,0,2,0,0},
	{RACE_ORC,CLASS_MAGE,46,0,2,0,0},
	{RACE_ORC,CLASS_MAGE,47,0,3,0,0},
	{RACE_ORC,CLASS_MAGE,48,0,3,0,0},
	{RACE_ORC,CLASS_MAGE,49,0,3,0,0},
	{RACE_ORC,CLASS_MAGE,50,0,3,0,0},
	{RACE_ORC,CLASS_MAGE,51,0,2,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,0,0,0,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,1,0,0,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,2,0,0,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,3,0,0,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,4,0,0,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,5,0,0,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,6,0,0,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,7,0,0,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,8,0,0,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,9,0,0,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,10,0,0,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,11,0,0,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,12,0,0,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,13,0,0,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,14,0,0,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,15,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,16,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,17,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,18,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,19,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,20,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,21,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,22,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,23,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,24,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,25,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,26,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,27,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,28,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,29,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,30,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,31,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,32,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,33,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,34,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,35,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,36,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,37,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,38,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,39,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,40,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,41,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,42,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,43,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,44,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,45,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,46,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,47,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,48,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,49,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,50,0,1,0,0},
	{RACE_ORC,CLASS_ALCHEMIST,51,0,1,0,0},
	{RACE_TROLL,CLASS_WARRIOR,0,0,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,1,1,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,2,2,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,3,3,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,4,4,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,5,5,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,6,6,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,7,7,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,8,8,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,9,9,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,10,10,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,11,11,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,12,12,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,13,13,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,14,14,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,15,15,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,16,16,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,17,17,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,18,18,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,19,19,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,20,20,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,21,21,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,22,22,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,23,23,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,24,24,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,25,25,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,26,26,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,27,27,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,28,28,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,29,29,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,30,30,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,31,31,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,32,32,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,33,33,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,34,34,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,35,35,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,36,36,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,37,37,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,38,38,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,39,39,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,40,40,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,41,41,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,42,42,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,43,43,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,44,44,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,45,45,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,46,46,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,47,47,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,48,48,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,49,49,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,50,50,0,0,0},
	{RACE_TROLL,CLASS_WARRIOR,51,51,0,0,0},
	{RACE_TROLL,CLASS_NECROMANT,0,0,0,0,0},
	{RACE_TROLL,CLASS_NECROMANT,1,2,0,0,0},
	{RACE_TROLL,CLASS_NECROMANT,2,2,0,0,0},
	{RACE_TROLL,CLASS_NECROMANT,3,3,0,0,0},
	{RACE_TROLL,CLASS_NECROMANT,4,4,1,0,0},
	{RACE_TROLL,CLASS_NECROMANT,5,4,1,0,0},
	{RACE_TROLL,CLASS_NECROMANT,6,5,1,0,0},
	{RACE_TROLL,CLASS_NECROMANT,7,6,0,0,0},
	{RACE_TROLL,CLASS_NECROMANT,8,7,1,0,0},
	{RACE_TROLL,CLASS_NECROMANT,9,8,1,0,0},
	{RACE_TROLL,CLASS_NECROMANT,10,9,2,0,0},
	{RACE_TROLL,CLASS_NECROMANT,11,11,2,0,0},
	{RACE_TROLL,CLASS_NECROMANT,12,12,3,0,0},
	{RACE_TROLL,CLASS_NECROMANT,13,13,2,0,0},
	{RACE_TROLL,CLASS_NECROMANT,14,15,2,0,0},
	{RACE_TROLL,CLASS_NECROMANT,15,15,1,0,0},
	{RACE_TROLL,CLASS_NECROMANT,16,16,1,0,0},
	{RACE_TROLL,CLASS_NECROMANT,17,16,1,0,0},
	{RACE_TROLL,CLASS_NECROMANT,18,17,2,0,0},
	{RACE_TROLL,CLASS_NECROMANT,19,18,2,0,0},
	{RACE_TROLL,CLASS_NECROMANT,20,19,1,0,0},
	{RACE_TROLL,CLASS_NECROMANT,21,19,1,0,0},
	{RACE_TROLL,CLASS_NECROMANT,22,20,1,0,0},
	{RACE_TROLL,CLASS_NECROMANT,23,21,1,0,0},
	{RACE_TROLL,CLASS_NECROMANT,24,22,1,0,0},
	{RACE_TROLL,CLASS_NECROMANT,25,23,0,0,0},
	{RACE_TROLL,CLASS_NECROMANT,26,24,1,0,0},
	{RACE_TROLL,CLASS_NECROMANT,27,25,0,0,0},
	{RACE_TROLL,CLASS_NECROMANT,28,26,0,0,0},
	{RACE_TROLL,CLASS_NECROMANT,29,28,0,0,0},
	{RACE_TROLL,CLASS_NECROMANT,30,29,1,0,0},
	{RACE_TROLL,CLASS_NECROMANT,31,31,1,0,0},
	{RACE_TROLL,CLASS_NECROMANT,32,32,0,0,0},
	{RACE_TROLL,CLASS_NECROMANT,33,33,1,0,0},
	{RACE_TROLL,CLASS_NECROMANT,34,34,1,0,0},
	{RACE_TROLL,CLASS_NECROMANT,35,35,0,0,0},
	{RACE_TROLL,CLASS_NECROMANT,36,36,1,0,0},
	{RACE_TROLL,CLASS_NECROMANT,37,38,0,0,0},
	{RACE_TROLL,CLASS_NECROMANT,38,39,0,0,0},
	{RACE_TROLL,CLASS_NECROMANT,39,40,0,0,0},
	{RACE_TROLL,CLASS_NECROMANT,40,41,0,0,0},
	{RACE_TROLL,CLASS_NECROMANT,41,42,1,0,0},
	{RACE_TROLL,CLASS_NECROMANT,42,43,2,0,0},
	{RACE_TROLL,CLASS_NECROMANT,43,43,2,0,0},
	{RACE_TROLL,CLASS_NECROMANT,44,44,2,0,0},
	{RACE_TROLL,CLASS_NECROMANT,45,45,1,0,0},
	{RACE_TROLL,CLASS_NECROMANT,46,47,1,0,0},
	{RACE_TROLL,CLASS_NECROMANT,47,48,2,0,0},
	{RACE_TROLL,CLASS_NECROMANT,48,49,2,0,0},
	{RACE_TROLL,CLASS_NECROMANT,49,50,2,0,0},
	{RACE_TROLL,CLASS_NECROMANT,50,51,3,0,0},
	{RACE_TROLL,CLASS_NECROMANT,51,53,4,0,0},
	{RACE_TROLL,CLASS_THIEF,0,0,0,0,0},
	{RACE_TROLL,CLASS_THIEF,1,1,0,0,0},
	{RACE_TROLL,CLASS_THIEF,2,2,0,0,0},
	{RACE_TROLL,CLASS_THIEF,3,3,0,0,0},
	{RACE_TROLL,CLASS_THIEF,4,4,0,0,0},
	{RACE_TROLL,CLASS_THIEF,5,5,0,0,0},
	{RACE_TROLL,CLASS_THIEF,6,6,0,0,0},
	{RACE_TROLL,CLASS_THIEF,7,7,0,0,0},
	{RACE_TROLL,CLASS_THIEF,8,8,0,0,0},
	{RACE_TROLL,CLASS_THIEF,9,9,0,0,0},
	{RACE_TROLL,CLASS_THIEF,10,10,0,0,0},
	{RACE_TROLL,CLASS_THIEF,11,11,0,0,0},
	{RACE_TROLL,CLASS_THIEF,12,12,0,0,0},
	{RACE_TROLL,CLASS_THIEF,13,13,0,0,0},
	{RACE_TROLL,CLASS_THIEF,14,14,0,0,0},
	{RACE_TROLL,CLASS_THIEF,15,15,0,0,0},
	{RACE_TROLL,CLASS_THIEF,16,16,0,0,0},
	{RACE_TROLL,CLASS_THIEF,17,17,0,0,0},
	{RACE_TROLL,CLASS_THIEF,18,18,0,0,0},
	{RACE_TROLL,CLASS_THIEF,19,19,0,0,0},
	{RACE_TROLL,CLASS_THIEF,20,20,0,0,0},
	{RACE_TROLL,CLASS_THIEF,21,21,0,0,0},
	{RACE_TROLL,CLASS_THIEF,22,22,0,0,0},
	{RACE_TROLL,CLASS_THIEF,23,23,0,0,0},
	{RACE_TROLL,CLASS_THIEF,24,24,0,0,0},
	{RACE_TROLL,CLASS_THIEF,25,25,0,0,0},
	{RACE_TROLL,CLASS_THIEF,26,26,0,0,0},
	{RACE_TROLL,CLASS_THIEF,27,27,0,0,0},
	{RACE_TROLL,CLASS_THIEF,28,28,0,0,0},
	{RACE_TROLL,CLASS_THIEF,29,29,0,0,0},
	{RACE_TROLL,CLASS_THIEF,30,30,0,0,0},
	{RACE_TROLL,CLASS_THIEF,31,31,0,0,0},
	{RACE_TROLL,CLASS_THIEF,32,32,0,0,0},
	{RACE_TROLL,CLASS_THIEF,33,33,0,0,0},
	{RACE_TROLL,CLASS_THIEF,34,34,0,0,0},
	{RACE_TROLL,CLASS_THIEF,35,35,0,0,0},
	{RACE_TROLL,CLASS_THIEF,36,36,0,0,0},
	{RACE_TROLL,CLASS_THIEF,37,37,0,0,0},
	{RACE_TROLL,CLASS_THIEF,38,38,0,0,0},
	{RACE_TROLL,CLASS_THIEF,39,39,0,0,0},
	{RACE_TROLL,CLASS_THIEF,40,40,0,0,0},
	{RACE_TROLL,CLASS_THIEF,41,41,0,0,0},
	{RACE_TROLL,CLASS_THIEF,42,42,0,0,0},
	{RACE_TROLL,CLASS_THIEF,43,43,0,0,0},
	{RACE_TROLL,CLASS_THIEF,44,44,0,0,0},
	{RACE_TROLL,CLASS_THIEF,45,45,0,0,0},
	{RACE_TROLL,CLASS_THIEF,46,46,0,0,0},
	{RACE_TROLL,CLASS_THIEF,47,47,0,0,0},
	{RACE_TROLL,CLASS_THIEF,48,48,0,0,0},
	{RACE_TROLL,CLASS_THIEF,49,49,0,0,0},
	{RACE_TROLL,CLASS_THIEF,50,50,0,0,0},
	{RACE_TROLL,CLASS_THIEF,51,51,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,0,0,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,1,1,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,2,1,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,3,2,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,4,3,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,5,3,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,6,3,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,7,4,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,8,5,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,9,5,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,10,5,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,11,6,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,12,6,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,13,6,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,14,6,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,15,6,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,16,7,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,17,8,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,18,8,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,19,9,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,20,9,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,21,9,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,22,9,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,23,9,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,24,10,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,25,11,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,26,11,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,27,11,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,28,11,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,29,11,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,30,11,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,31,11,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,32,11,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,33,12,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,34,12,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,35,12,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,36,13,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,37,13,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,38,13,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,39,13,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,40,14,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,41,14,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,42,15,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,43,15,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,44,15,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,45,15,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,46,15,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,47,15,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,48,16,0,0,0},
	{RACE_TROLL,CLASS_NAZGUL,49,17,1,0,0},
	{RACE_TROLL,CLASS_NAZGUL,50,17,1,0,0},
	{RACE_TROLL,CLASS_NAZGUL,51,18,1,0,0},
	{RACE_GIANT,CLASS_WARRIOR,0,0,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,1,4,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,2,8,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,3,11,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,4,15,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,5,19,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,6,23,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,7,27,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,8,31,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,9,35,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,10,38,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,11,42,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,12,46,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,13,49,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,14,53,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,15,57,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,16,61,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,17,65,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,18,69,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,19,73,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,20,76,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,21,80,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,22,83,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,23,87,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,24,91,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,25,94,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,26,98,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,27,102,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,28,106,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,29,110,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,30,114,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,31,118,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,32,122,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,33,126,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,34,130,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,35,134,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,36,137,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,37,141,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,38,144,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,39,148,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,40,152,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,41,156,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,42,160,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,43,164,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,44,168,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,45,172,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,46,176,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,47,180,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,48,184,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,49,188,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,50,191,0,0,0},
	{RACE_GIANT,CLASS_WARRIOR,51,194,0,0,0},
	{RACE_GIANT,CLASS_DRUID,0,0,0,0,0},
	{RACE_GIANT,CLASS_DRUID,1,4,0,0,0},
	{RACE_GIANT,CLASS_DRUID,2,8,0,0,0},
	{RACE_GIANT,CLASS_DRUID,3,12,0,0,0},
	{RACE_GIANT,CLASS_DRUID,4,16,0,0,0},
	{RACE_GIANT,CLASS_DRUID,5,20,0,0,0},
	{RACE_GIANT,CLASS_DRUID,6,24,0,0,0},
	{RACE_GIANT,CLASS_DRUID,7,28,1,0,0},
	{RACE_GIANT,CLASS_DRUID,8,32,0,0,0},
	{RACE_GIANT,CLASS_DRUID,9,36,0,0,0},
	{RACE_GIANT,CLASS_DRUID,10,40,1,0,0},
	{RACE_GIANT,CLASS_DRUID,11,44,1,0,0},
	{RACE_GIANT,CLASS_DRUID,12,48,1,0,0},
	{RACE_GIANT,CLASS_DRUID,13,52,0,0,0},
	{RACE_GIANT,CLASS_DRUID,14,56,0,0,0},
	{RACE_GIANT,CLASS_DRUID,15,60,0,0,0},
	{RACE_GIANT,CLASS_DRUID,16,64,1,0,0},
	{RACE_GIANT,CLASS_DRUID,17,68,0,0,0},
	{RACE_GIANT,CLASS_DRUID,18,72,1,0,0},
	{RACE_GIANT,CLASS_DRUID,19,76,2,0,0},
	{RACE_GIANT,CLASS_DRUID,20,80,3,0,0},
	{RACE_GIANT,CLASS_DRUID,21,84,3,0,0},
	{RACE_GIANT,CLASS_DRUID,22,88,4,0,0},
	{RACE_GIANT,CLASS_DRUID,23,92,3,0,0},
	{RACE_GIANT,CLASS_DRUID,24,96,3,0,0},
	{RACE_GIANT,CLASS_DRUID,25,100,3,0,0},
	{RACE_GIANT,CLASS_DRUID,26,104,3,0,0},
	{RACE_GIANT,CLASS_DRUID,27,108,3,0,0},
	{RACE_GIANT,CLASS_DRUID,28,112,2,0,0},
	{RACE_GIANT,CLASS_DRUID,29,116,1,0,0},
	{RACE_GIANT,CLASS_DRUID,30,120,2,0,0},
	{RACE_GIANT,CLASS_DRUID,31,124,3,0,0},
	{RACE_GIANT,CLASS_DRUID,32,128,2,0,0},
	{RACE_GIANT,CLASS_DRUID,33,132,2,0,0},
	{RACE_GIANT,CLASS_DRUID,34,136,3,0,0},
	{RACE_GIANT,CLASS_DRUID,35,140,3,0,0},
	{RACE_GIANT,CLASS_DRUID,36,144,3,0,0},
	{RACE_GIANT,CLASS_DRUID,37,148,3,0,0},
	{RACE_GIANT,CLASS_DRUID,38,152,4,0,0},
	{RACE_GIANT,CLASS_DRUID,39,156,4,0,0},
	{RACE_GIANT,CLASS_DRUID,40,160,4,0,0},
	{RACE_GIANT,CLASS_DRUID,41,164,3,0,0},
	{RACE_GIANT,CLASS_DRUID,42,168,3,0,0},
	{RACE_GIANT,CLASS_DRUID,43,172,4,0,0},
	{RACE_GIANT,CLASS_DRUID,44,176,5,0,0},
	{RACE_GIANT,CLASS_DRUID,45,180,6,0,0},
	{RACE_GIANT,CLASS_DRUID,46,184,6,0,0},
	{RACE_GIANT,CLASS_DRUID,47,188,7,0,0},
	{RACE_GIANT,CLASS_DRUID,48,192,7,0,0},
	{RACE_GIANT,CLASS_DRUID,49,196,7,0,0},
	{RACE_GIANT,CLASS_DRUID,50,200,6,0,0},
	{RACE_GIANT,CLASS_DRUID,51,204,7,0,0},
	{RACE_GIANT,CLASS_THIEF,0,0,0,0,0},
	{RACE_GIANT,CLASS_THIEF,1,3,0,0,0},
	{RACE_GIANT,CLASS_THIEF,2,6,0,0,0},
	{RACE_GIANT,CLASS_THIEF,3,9,0,0,0},
	{RACE_GIANT,CLASS_THIEF,4,12,0,0,0},
	{RACE_GIANT,CLASS_THIEF,5,16,0,0,0},
	{RACE_GIANT,CLASS_THIEF,6,19,0,0,0},
	{RACE_GIANT,CLASS_THIEF,7,22,0,0,0},
	{RACE_GIANT,CLASS_THIEF,8,25,0,0,0},
	{RACE_GIANT,CLASS_THIEF,9,28,0,0,0},
	{RACE_GIANT,CLASS_THIEF,10,31,0,0,0},
	{RACE_GIANT,CLASS_THIEF,11,35,0,0,0},
	{RACE_GIANT,CLASS_THIEF,12,38,0,0,0},
	{RACE_GIANT,CLASS_THIEF,13,41,0,0,0},
	{RACE_GIANT,CLASS_THIEF,14,44,0,0,0},
	{RACE_GIANT,CLASS_THIEF,15,47,0,0,0},
	{RACE_GIANT,CLASS_THIEF,16,50,0,0,0},
	{RACE_GIANT,CLASS_THIEF,17,53,0,0,0},
	{RACE_GIANT,CLASS_THIEF,18,56,0,0,0},
	{RACE_GIANT,CLASS_THIEF,19,59,0,0,0},
	{RACE_GIANT,CLASS_THIEF,20,62,0,0,0},
	{RACE_GIANT,CLASS_THIEF,21,65,0,0,0},
	{RACE_GIANT,CLASS_THIEF,22,68,0,0,0},
	{RACE_GIANT,CLASS_THIEF,23,71,0,0,0},
	{RACE_GIANT,CLASS_THIEF,24,74,0,0,0},
	{RACE_GIANT,CLASS_THIEF,25,77,0,0,0},
	{RACE_GIANT,CLASS_THIEF,26,80,0,0,0},
	{RACE_GIANT,CLASS_THIEF,27,83,0,0,0},
	{RACE_GIANT,CLASS_THIEF,28,86,0,0,0},
	{RACE_GIANT,CLASS_THIEF,29,89,0,0,0},
	{RACE_GIANT,CLASS_THIEF,30,92,0,0,0},
	{RACE_GIANT,CLASS_THIEF,31,95,0,0,0},
	{RACE_GIANT,CLASS_THIEF,32,98,0,0,0},
	{RACE_GIANT,CLASS_THIEF,33,101,0,0,0},
	{RACE_GIANT,CLASS_THIEF,34,104,0,0,0},
	{RACE_GIANT,CLASS_THIEF,35,107,0,0,0},
	{RACE_GIANT,CLASS_THIEF,36,110,0,0,0},
	{RACE_GIANT,CLASS_THIEF,37,113,0,0,0},
	{RACE_GIANT,CLASS_THIEF,38,117,0,0,0},
	{RACE_GIANT,CLASS_THIEF,39,120,0,0,0},
	{RACE_GIANT,CLASS_THIEF,40,123,0,0,0},
	{RACE_GIANT,CLASS_THIEF,41,126,0,0,0},
	{RACE_GIANT,CLASS_THIEF,42,129,0,0,0},
	{RACE_GIANT,CLASS_THIEF,43,132,0,0,0},
	{RACE_GIANT,CLASS_THIEF,44,135,0,0,0},
	{RACE_GIANT,CLASS_THIEF,45,138,0,0,0},
	{RACE_GIANT,CLASS_THIEF,46,141,0,0,0},
	{RACE_GIANT,CLASS_THIEF,47,144,0,0,0},
	{RACE_GIANT,CLASS_THIEF,48,147,0,0,0},
	{RACE_GIANT,CLASS_THIEF,49,150,0,0,0},
	{RACE_GIANT,CLASS_THIEF,50,153,0,0,0},
	{RACE_GIANT,CLASS_THIEF,51,156,0,0,0},
	{RACE_GIANT,CLASS_RANGER,0,0,0,0,0},
	{RACE_GIANT,CLASS_RANGER,1,3,0,0,0},
	{RACE_GIANT,CLASS_RANGER,2,6,0,0,0},
	{RACE_GIANT,CLASS_RANGER,3,9,0,0,0},
	{RACE_GIANT,CLASS_RANGER,4,12,0,0,0},
	{RACE_GIANT,CLASS_RANGER,5,15,0,0,0},
	{RACE_GIANT,CLASS_RANGER,6,18,0,0,0},
	{RACE_GIANT,CLASS_RANGER,7,21,0,0,0},
	{RACE_GIANT,CLASS_RANGER,8,24,0,0,0},
	{RACE_GIANT,CLASS_RANGER,9,27,0,0,0},
	{RACE_GIANT,CLASS_RANGER,10,30,0,0,0},
	{RACE_GIANT,CLASS_RANGER,11,33,0,0,0},
	{RACE_GIANT,CLASS_RANGER,12,36,0,0,0},
	{RACE_GIANT,CLASS_RANGER,13,39,0,0,0},
	{RACE_GIANT,CLASS_RANGER,14,42,0,0,0},
	{RACE_GIANT,CLASS_RANGER,15,45,0,0,0},
	{RACE_GIANT,CLASS_RANGER,16,48,0,0,0},
	{RACE_GIANT,CLASS_RANGER,17,51,0,0,0},
	{RACE_GIANT,CLASS_RANGER,18,55,0,0,0},
	{RACE_GIANT,CLASS_RANGER,19,58,0,0,0},
	{RACE_GIANT,CLASS_RANGER,20,61,0,0,0},
	{RACE_GIANT,CLASS_RANGER,21,64,0,0,0},
	{RACE_GIANT,CLASS_RANGER,22,67,0,0,0},
	{RACE_GIANT,CLASS_RANGER,23,70,0,0,0},
	{RACE_GIANT,CLASS_RANGER,24,73,0,0,0},
	{RACE_GIANT,CLASS_RANGER,25,76,0,0,0},
	{RACE_GIANT,CLASS_RANGER,26,79,0,0,0},
	{RACE_GIANT,CLASS_RANGER,27,82,0,0,0},
	{RACE_GIANT,CLASS_RANGER,28,85,0,0,0},
	{RACE_GIANT,CLASS_RANGER,29,88,0,0,0},
	{RACE_GIANT,CLASS_RANGER,30,91,0,0,0},
	{RACE_GIANT,CLASS_RANGER,31,94,0,0,0},
	{RACE_GIANT,CLASS_RANGER,32,97,0,0,0},
	{RACE_GIANT,CLASS_RANGER,33,100,0,0,0},
	{RACE_GIANT,CLASS_RANGER,34,103,0,0,0},
	{RACE_GIANT,CLASS_RANGER,35,106,0,0,0},
	{RACE_GIANT,CLASS_RANGER,36,109,0,0,0},
	{RACE_GIANT,CLASS_RANGER,37,112,0,0,0},
	{RACE_GIANT,CLASS_RANGER,38,115,0,0,0},
	{RACE_GIANT,CLASS_RANGER,39,118,0,0,0},
	{RACE_GIANT,CLASS_RANGER,40,121,0,0,0},
	{RACE_GIANT,CLASS_RANGER,41,124,0,0,0},
	{RACE_GIANT,CLASS_RANGER,42,127,0,0,0},
	{RACE_GIANT,CLASS_RANGER,43,130,0,0,0},
	{RACE_GIANT,CLASS_RANGER,44,133,0,0,0},
	{RACE_GIANT,CLASS_RANGER,45,136,0,0,0},
	{RACE_GIANT,CLASS_RANGER,46,139,0,0,0},
	{RACE_GIANT,CLASS_RANGER,47,142,0,0,0},
	{RACE_GIANT,CLASS_RANGER,48,145,0,0,0},
	{RACE_GIANT,CLASS_RANGER,49,148,0,0,0},
	{RACE_GIANT,CLASS_RANGER,50,151,0,0,0},
	{RACE_GIANT,CLASS_RANGER,51,154,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,0,0,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,1,3,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,2,6,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,3,9,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,4,12,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,5,15,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,6,18,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,7,21,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,8,24,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,9,27,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,10,30,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,11,33,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,12,36,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,13,39,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,14,42,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,15,45,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,16,48,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,17,51,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,18,54,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,19,57,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,20,60,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,21,63,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,22,66,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,23,69,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,24,72,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,25,75,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,26,78,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,27,81,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,28,84,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,29,87,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,30,90,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,31,93,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,32,96,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,33,99,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,34,102,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,35,105,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,36,108,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,37,111,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,38,114,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,39,117,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,40,120,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,41,123,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,42,126,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,43,129,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,44,132,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,45,135,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,46,138,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,47,141,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,48,144,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,49,147,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,50,150,0,0,0},
	{RACE_GIANT,CLASS_PALADIN,51,153,0,0,0},
	{RACE_ELF,CLASS_WARRIOR,0,0,0,0,0},
	{RACE_ELF,CLASS_WARRIOR,1,1,0,0,0},
	{RACE_ELF,CLASS_WARRIOR,2,2,0,0,0},
	{RACE_ELF,CLASS_WARRIOR,3,4,1,0,0},
	{RACE_ELF,CLASS_WARRIOR,4,5,1,0,0},
	{RACE_ELF,CLASS_WARRIOR,5,6,2,0,0},
	{RACE_ELF,CLASS_WARRIOR,6,7,2,0,0},
	{RACE_ELF,CLASS_WARRIOR,7,9,3,0,0},
	{RACE_ELF,CLASS_WARRIOR,8,9,3,0,0},
	{RACE_ELF,CLASS_WARRIOR,9,10,3,0,0},
	{RACE_ELF,CLASS_WARRIOR,10,12,3,0,0},
	{RACE_ELF,CLASS_WARRIOR,11,13,4,0,0},
	{RACE_ELF,CLASS_WARRIOR,12,15,4,0,0},
	{RACE_ELF,CLASS_WARRIOR,13,17,4,0,0},
	{RACE_ELF,CLASS_WARRIOR,14,18,4,0,0},
	{RACE_ELF,CLASS_WARRIOR,15,18,4,0,0},
	{RACE_ELF,CLASS_WARRIOR,16,19,4,0,0},
	{RACE_ELF,CLASS_WARRIOR,17,21,4,0,0},
	{RACE_ELF,CLASS_WARRIOR,18,21,4,0,0},
	{RACE_ELF,CLASS_WARRIOR,19,22,4,0,0},
	{RACE_ELF,CLASS_WARRIOR,20,23,5,0,0},
	{RACE_ELF,CLASS_WARRIOR,21,24,6,0,0},
	{RACE_ELF,CLASS_WARRIOR,22,25,7,0,0},
	{RACE_ELF,CLASS_WARRIOR,23,25,7,0,0},
	{RACE_ELF,CLASS_WARRIOR,24,26,8,0,0},
	{RACE_ELF,CLASS_WARRIOR,25,28,8,0,0},
	{RACE_ELF,CLASS_WARRIOR,26,29,8,0,0},
	{RACE_ELF,CLASS_WARRIOR,27,30,9,0,0},
	{RACE_ELF,CLASS_WARRIOR,28,31,10,0,0},
	{RACE_ELF,CLASS_WARRIOR,29,31,11,0,0},
	{RACE_ELF,CLASS_WARRIOR,30,31,11,0,0},
	{RACE_ELF,CLASS_WARRIOR,31,33,11,0,0},
	{RACE_ELF,CLASS_WARRIOR,32,33,11,0,0},
	{RACE_ELF,CLASS_WARRIOR,33,34,12,0,0},
	{RACE_ELF,CLASS_WARRIOR,34,34,13,0,0},
	{RACE_ELF,CLASS_WARRIOR,35,34,14,0,0},
	{RACE_ELF,CLASS_WARRIOR,36,35,14,0,0},
	{RACE_ELF,CLASS_WARRIOR,37,37,15,0,0},
	{RACE_ELF,CLASS_WARRIOR,38,38,15,0,0},
	{RACE_ELF,CLASS_WARRIOR,39,38,15,0,0},
	{RACE_ELF,CLASS_WARRIOR,40,40,16,0,0},
	{RACE_ELF,CLASS_WARRIOR,41,41,17,0,0},
	{RACE_ELF,CLASS_WARRIOR,42,43,17,0,0},
	{RACE_ELF,CLASS_WARRIOR,43,45,17,0,0},
	{RACE_ELF,CLASS_WARRIOR,44,46,18,0,0},
	{RACE_ELF,CLASS_WARRIOR,45,46,18,0,0},
	{RACE_ELF,CLASS_WARRIOR,46,47,18,0,0},
	{RACE_ELF,CLASS_WARRIOR,47,48,19,0,0},
	{RACE_ELF,CLASS_WARRIOR,48,49,19,0,0},
	{RACE_ELF,CLASS_WARRIOR,49,51,20,0,0},
	{RACE_ELF,CLASS_WARRIOR,50,53,20,0,0},
	{RACE_ELF,CLASS_WARRIOR,51,55,20,0,0},
	{RACE_ELF,CLASS_DRUID,0,0,0,0,0},
	{RACE_ELF,CLASS_DRUID,1,1,2,0,0},
	{RACE_ELF,CLASS_DRUID,2,2,3,0,0},
	{RACE_ELF,CLASS_DRUID,3,3,4,0,0},
	{RACE_ELF,CLASS_DRUID,4,4,5,0,0},
	{RACE_ELF,CLASS_DRUID,5,5,7,0,0},
	{RACE_ELF,CLASS_DRUID,6,6,7,0,0},
	{RACE_ELF,CLASS_DRUID,7,7,8,0,0},
	{RACE_ELF,CLASS_DRUID,8,8,10,0,0},
	{RACE_ELF,CLASS_DRUID,9,9,11,0,0},
	{RACE_ELF,CLASS_DRUID,10,10,11,0,0},
	{RACE_ELF,CLASS_DRUID,11,10,12,0,0},
	{RACE_ELF,CLASS_DRUID,12,11,13,0,0},
	{RACE_ELF,CLASS_DRUID,13,12,14,0,0},
	{RACE_ELF,CLASS_DRUID,14,13,15,0,0},
	{RACE_ELF,CLASS_DRUID,15,14,16,0,0},
	{RACE_ELF,CLASS_DRUID,16,15,18,0,0},
	{RACE_ELF,CLASS_DRUID,17,16,19,0,0},
	{RACE_ELF,CLASS_DRUID,18,17,20,0,0},
	{RACE_ELF,CLASS_DRUID,19,17,21,0,0},
	{RACE_ELF,CLASS_DRUID,20,17,22,0,0},
	{RACE_ELF,CLASS_DRUID,21,18,22,0,0},
	{RACE_ELF,CLASS_DRUID,22,19,24,0,0},
	{RACE_ELF,CLASS_DRUID,23,20,24,0,0},
	{RACE_ELF,CLASS_DRUID,24,21,25,0,0},
	{RACE_ELF,CLASS_DRUID,25,22,26,0,0},
	{RACE_ELF,CLASS_DRUID,26,22,28,0,0},
	{RACE_ELF,CLASS_DRUID,27,22,29,0,0},
	{RACE_ELF,CLASS_DRUID,28,23,30,0,0},
	{RACE_ELF,CLASS_DRUID,29,23,30,0,0},
	{RACE_ELF,CLASS_DRUID,30,23,31,0,0},
	{RACE_ELF,CLASS_DRUID,31,24,31,0,0},
	{RACE_ELF,CLASS_DRUID,32,25,31,0,0},
	{RACE_ELF,CLASS_DRUID,33,26,31,0,0},
	{RACE_ELF,CLASS_DRUID,34,27,33,0,0},
	{RACE_ELF,CLASS_DRUID,35,28,34,0,0},
	{RACE_ELF,CLASS_DRUID,36,29,36,0,0},
	{RACE_ELF,CLASS_DRUID,37,30,36,0,0},
	{RACE_ELF,CLASS_DRUID,38,30,38,0,0},
	{RACE_ELF,CLASS_DRUID,39,31,39,0,0},
	{RACE_ELF,CLASS_DRUID,40,32,41,0,0},
	{RACE_ELF,CLASS_DRUID,41,33,42,0,0},
	{RACE_ELF,CLASS_DRUID,42,34,44,0,0},
	{RACE_ELF,CLASS_DRUID,43,34,45,0,0},
	{RACE_ELF,CLASS_DRUID,44,35,46,0,0},
	{RACE_ELF,CLASS_DRUID,45,36,47,0,0},
	{RACE_ELF,CLASS_DRUID,46,37,48,0,0},
	{RACE_ELF,CLASS_DRUID,47,38,49,0,0},
	{RACE_ELF,CLASS_DRUID,48,39,50,0,0},
	{RACE_ELF,CLASS_DRUID,49,39,51,0,0},
	{RACE_ELF,CLASS_DRUID,50,39,52,0,0},
	{RACE_ELF,CLASS_DRUID,51,39,53,0,0},
	{RACE_ELF,CLASS_CLERIC,0,0,0,0,0},
	{RACE_ELF,CLASS_CLERIC,1,0,1,0,0},
	{RACE_ELF,CLASS_CLERIC,2,1,1,0,0},
	{RACE_ELF,CLASS_CLERIC,3,2,1,0,0},
	{RACE_ELF,CLASS_CLERIC,4,3,1,0,0},
	{RACE_ELF,CLASS_CLERIC,5,4,2,0,0},
	{RACE_ELF,CLASS_CLERIC,6,4,3,0,0},
	{RACE_ELF,CLASS_CLERIC,7,4,4,0,0},
	{RACE_ELF,CLASS_CLERIC,8,4,4,0,0},
	{RACE_ELF,CLASS_CLERIC,9,5,5,0,0},
	{RACE_ELF,CLASS_CLERIC,10,6,6,0,0},
	{RACE_ELF,CLASS_CLERIC,11,7,6,0,0},
	{RACE_ELF,CLASS_CLERIC,12,7,7,0,0},
	{RACE_ELF,CLASS_CLERIC,13,7,7,0,0},
	{RACE_ELF,CLASS_CLERIC,14,8,7,0,0},
	{RACE_ELF,CLASS_CLERIC,15,9,8,0,0},
	{RACE_ELF,CLASS_CLERIC,16,10,8,0,0},
	{RACE_ELF,CLASS_CLERIC,17,11,8,0,0},
	{RACE_ELF,CLASS_CLERIC,18,12,9,0,0},
	{RACE_ELF,CLASS_CLERIC,19,12,10,0,0},
	{RACE_ELF,CLASS_CLERIC,20,12,11,0,0},
	{RACE_ELF,CLASS_CLERIC,21,13,11,0,0},
	{RACE_ELF,CLASS_CLERIC,22,13,13,0,0},
	{RACE_ELF,CLASS_CLERIC,23,14,13,0,0},
	{RACE_ELF,CLASS_CLERIC,24,15,14,0,0},
	{RACE_ELF,CLASS_CLERIC,25,16,14,0,0},
	{RACE_ELF,CLASS_CLERIC,26,17,14,0,0},
	{RACE_ELF,CLASS_CLERIC,27,18,15,0,0},
	{RACE_ELF,CLASS_CLERIC,28,18,15,0,0},
	{RACE_ELF,CLASS_CLERIC,29,19,15,0,0},
	{RACE_ELF,CLASS_CLERIC,30,20,15,0,0},
	{RACE_ELF,CLASS_CLERIC,31,21,16,0,0},
	{RACE_ELF,CLASS_CLERIC,32,21,16,0,0},
	{RACE_ELF,CLASS_CLERIC,33,22,16,0,0},
	{RACE_ELF,CLASS_CLERIC,34,23,17,0,0},
	{RACE_ELF,CLASS_CLERIC,35,23,17,0,0},
	{RACE_ELF,CLASS_CLERIC,36,24,17,0,0},
	{RACE_ELF,CLASS_CLERIC,37,25,17,0,0},
	{RACE_ELF,CLASS_CLERIC,38,25,17,0,0},
	{RACE_ELF,CLASS_CLERIC,39,26,17,0,0},
	{RACE_ELF,CLASS_CLERIC,40,26,17,0,0},
	{RACE_ELF,CLASS_CLERIC,41,27,18,0,0},
	{RACE_ELF,CLASS_CLERIC,42,28,18,0,0},
	{RACE_ELF,CLASS_CLERIC,43,29,18,0,0},
	{RACE_ELF,CLASS_CLERIC,44,29,19,0,0},
	{RACE_ELF,CLASS_CLERIC,45,30,19,0,0},
	{RACE_ELF,CLASS_CLERIC,46,31,20,0,0},
	{RACE_ELF,CLASS_CLERIC,47,32,20,0,0},
	{RACE_ELF,CLASS_CLERIC,48,32,20,0,0},
	{RACE_ELF,CLASS_CLERIC,49,32,20,0,0},
	{RACE_ELF,CLASS_CLERIC,50,33,20,0,0},
	{RACE_ELF,CLASS_CLERIC,51,34,21,0,0},
	{RACE_ELF,CLASS_THIEF,0,0,0,0,0},
	{RACE_ELF,CLASS_THIEF,1,1,0,0,0},
	{RACE_ELF,CLASS_THIEF,2,2,0,0,0},
	{RACE_ELF,CLASS_THIEF,3,3,0,0,0},
	{RACE_ELF,CLASS_THIEF,4,4,1,0,0},
	{RACE_ELF,CLASS_THIEF,5,5,1,0,0},
	{RACE_ELF,CLASS_THIEF,6,6,2,0,0},
	{RACE_ELF,CLASS_THIEF,7,7,3,0,0},
	{RACE_ELF,CLASS_THIEF,8,8,4,0,0},
	{RACE_ELF,CLASS_THIEF,9,9,5,0,0},
	{RACE_ELF,CLASS_THIEF,10,10,6,0,0},
	{RACE_ELF,CLASS_THIEF,11,11,7,0,0},
	{RACE_ELF,CLASS_THIEF,12,12,8,0,0},
	{RACE_ELF,CLASS_THIEF,13,13,8,0,0},
	{RACE_ELF,CLASS_THIEF,14,14,9,0,0},
	{RACE_ELF,CLASS_THIEF,15,15,10,0,0},
	{RACE_ELF,CLASS_THIEF,16,16,10,0,0},
	{RACE_ELF,CLASS_THIEF,17,17,10,0,0},
	{RACE_ELF,CLASS_THIEF,18,18,11,0,0},
	{RACE_ELF,CLASS_THIEF,19,19,12,0,0},
	{RACE_ELF,CLASS_THIEF,20,20,13,0,0},
	{RACE_ELF,CLASS_THIEF,21,21,14,0,0},
	{RACE_ELF,CLASS_THIEF,22,22,15,0,0},
	{RACE_ELF,CLASS_THIEF,23,23,16,0,0},
	{RACE_ELF,CLASS_THIEF,24,24,16,0,0},
	{RACE_ELF,CLASS_THIEF,25,25,17,0,0},
	{RACE_ELF,CLASS_THIEF,26,26,18,0,0},
	{RACE_ELF,CLASS_THIEF,27,27,19,0,0},
	{RACE_ELF,CLASS_THIEF,28,28,19,0,0},
	{RACE_ELF,CLASS_THIEF,29,29,19,0,0},
	{RACE_ELF,CLASS_THIEF,30,30,19,0,0},
	{RACE_ELF,CLASS_THIEF,31,31,20,0,0},
	{RACE_ELF,CLASS_THIEF,32,32,20,0,0},
	{RACE_ELF,CLASS_THIEF,33,33,20,0,0},
	{RACE_ELF,CLASS_THIEF,34,34,21,0,0},
	{RACE_ELF,CLASS_THIEF,35,35,21,0,0},
	{RACE_ELF,CLASS_THIEF,36,36,22,0,0},
	{RACE_ELF,CLASS_THIEF,37,37,23,0,0},
	{RACE_ELF,CLASS_THIEF,38,38,24,0,0},
	{RACE_ELF,CLASS_THIEF,39,39,25,0,0},
	{RACE_ELF,CLASS_THIEF,40,40,25,0,0},
	{RACE_ELF,CLASS_THIEF,41,41,26,0,0},
	{RACE_ELF,CLASS_THIEF,42,42,27,0,0},
	{RACE_ELF,CLASS_THIEF,43,43,27,0,0},
	{RACE_ELF,CLASS_THIEF,44,44,28,0,0},
	{RACE_ELF,CLASS_THIEF,45,45,28,0,0},
	{RACE_ELF,CLASS_THIEF,46,46,28,0,0},
	{RACE_ELF,CLASS_THIEF,47,47,28,0,0},
	{RACE_ELF,CLASS_THIEF,48,48,29,0,0},
	{RACE_ELF,CLASS_THIEF,49,49,30,0,0},
	{RACE_ELF,CLASS_THIEF,50,50,30,0,0},
	{RACE_ELF,CLASS_THIEF,51,50,30,0,0},
	{RACE_ELF,CLASS_RANGER,0,0,0,0,0},
	{RACE_ELF,CLASS_RANGER,1,1,0,0,0},
	{RACE_ELF,CLASS_RANGER,2,2,0,0,0},
	{RACE_ELF,CLASS_RANGER,3,3,0,0,0},
	{RACE_ELF,CLASS_RANGER,4,4,1,0,0},
	{RACE_ELF,CLASS_RANGER,5,5,2,0,0},
	{RACE_ELF,CLASS_RANGER,6,6,3,0,0},
	{RACE_ELF,CLASS_RANGER,7,7,4,0,0},
	{RACE_ELF,CLASS_RANGER,8,8,4,0,0},
	{RACE_ELF,CLASS_RANGER,9,9,4,0,0},
	{RACE_ELF,CLASS_RANGER,10,10,5,0,0},
	{RACE_ELF,CLASS_RANGER,11,11,6,0,0},
	{RACE_ELF,CLASS_RANGER,12,12,6,0,0},
	{RACE_ELF,CLASS_RANGER,13,13,6,0,0},
	{RACE_ELF,CLASS_RANGER,14,14,6,0,0},
	{RACE_ELF,CLASS_RANGER,15,15,6,0,0},
	{RACE_ELF,CLASS_RANGER,16,16,6,0,0},
	{RACE_ELF,CLASS_RANGER,17,17,7,0,0},
	{RACE_ELF,CLASS_RANGER,18,18,8,0,0},
	{RACE_ELF,CLASS_RANGER,19,19,9,0,0},
	{RACE_ELF,CLASS_RANGER,20,20,10,0,0},
	{RACE_ELF,CLASS_RANGER,21,21,11,0,0},
	{RACE_ELF,CLASS_RANGER,22,22,11,0,0},
	{RACE_ELF,CLASS_RANGER,23,23,11,0,0},
	{RACE_ELF,CLASS_RANGER,24,24,11,0,0},
	{RACE_ELF,CLASS_RANGER,25,25,12,0,0},
	{RACE_ELF,CLASS_RANGER,26,26,13,0,0},
	{RACE_ELF,CLASS_RANGER,27,27,14,0,0},
	{RACE_ELF,CLASS_RANGER,28,28,15,0,0},
	{RACE_ELF,CLASS_RANGER,29,29,15,0,0},
	{RACE_ELF,CLASS_RANGER,30,30,16,0,0},
	{RACE_ELF,CLASS_RANGER,31,31,17,0,0},
	{RACE_ELF,CLASS_RANGER,32,32,18,0,0},
	{RACE_ELF,CLASS_RANGER,33,33,19,0,0},
	{RACE_ELF,CLASS_RANGER,34,34,20,0,0},
	{RACE_ELF,CLASS_RANGER,35,35,20,0,0},
	{RACE_ELF,CLASS_RANGER,36,36,20,0,0},
	{RACE_ELF,CLASS_RANGER,37,37,20,0,0},
	{RACE_ELF,CLASS_RANGER,38,38,20,0,0},
	{RACE_ELF,CLASS_RANGER,39,39,20,0,0},
	{RACE_ELF,CLASS_RANGER,40,40,20,0,0},
	{RACE_ELF,CLASS_RANGER,41,41,20,0,0},
	{RACE_ELF,CLASS_RANGER,42,42,21,0,0},
	{RACE_ELF,CLASS_RANGER,43,43,21,0,0},
	{RACE_ELF,CLASS_RANGER,44,44,22,0,0},
	{RACE_ELF,CLASS_RANGER,45,45,23,0,0},
	{RACE_ELF,CLASS_RANGER,46,46,24,0,0},
	{RACE_ELF,CLASS_RANGER,47,47,25,0,0},
	{RACE_ELF,CLASS_RANGER,48,48,25,0,0},
	{RACE_ELF,CLASS_RANGER,49,49,25,0,0},
	{RACE_ELF,CLASS_RANGER,50,50,25,0,0},
	{RACE_ELF,CLASS_RANGER,51,51,26,0,0},
	{RACE_ELF,CLASS_MAGE,0,0,0,0,0},
	{RACE_ELF,CLASS_MAGE,1,1,0,0,0},
	{RACE_ELF,CLASS_MAGE,2,2,1,0,0},
	{RACE_ELF,CLASS_MAGE,3,4,3,0,0},
	{RACE_ELF,CLASS_MAGE,4,5,4,0,0},
	{RACE_ELF,CLASS_MAGE,5,7,5,0,0},
	{RACE_ELF,CLASS_MAGE,6,9,5,0,0},
	{RACE_ELF,CLASS_MAGE,7,10,7,0,0},
	{RACE_ELF,CLASS_MAGE,8,11,8,0,0},
	{RACE_ELF,CLASS_MAGE,9,13,9,0,0},
	{RACE_ELF,CLASS_MAGE,10,14,10,0,0},
	{RACE_ELF,CLASS_MAGE,11,15,11,0,0},
	{RACE_ELF,CLASS_MAGE,12,16,12,0,0},
	{RACE_ELF,CLASS_MAGE,13,17,13,0,0},
	{RACE_ELF,CLASS_MAGE,14,18,13,0,0},
	{RACE_ELF,CLASS_MAGE,15,19,14,0,0},
	{RACE_ELF,CLASS_MAGE,16,20,15,0,0},
	{RACE_ELF,CLASS_MAGE,17,21,15,0,0},
	{RACE_ELF,CLASS_MAGE,18,22,17,0,0},
	{RACE_ELF,CLASS_MAGE,19,23,18,0,0},
	{RACE_ELF,CLASS_MAGE,20,25,20,0,0},
	{RACE_ELF,CLASS_MAGE,21,27,20,0,0},
	{RACE_ELF,CLASS_MAGE,22,29,21,0,0},
	{RACE_ELF,CLASS_MAGE,23,30,23,0,0},
	{RACE_ELF,CLASS_MAGE,24,32,25,0,0},
	{RACE_ELF,CLASS_MAGE,25,33,26,0,0},
	{RACE_ELF,CLASS_MAGE,26,34,27,0,0},
	{RACE_ELF,CLASS_MAGE,27,35,27,0,0},
	{RACE_ELF,CLASS_MAGE,28,35,27,0,0},
	{RACE_ELF,CLASS_MAGE,29,36,27,0,0},
	{RACE_ELF,CLASS_MAGE,30,38,28,0,0},
	{RACE_ELF,CLASS_MAGE,31,38,28,0,0},
	{RACE_ELF,CLASS_MAGE,32,38,29,0,0},
	{RACE_ELF,CLASS_MAGE,33,40,30,0,0},
	{RACE_ELF,CLASS_MAGE,34,41,32,0,0},
	{RACE_ELF,CLASS_MAGE,35,42,32,0,0},
	{RACE_ELF,CLASS_MAGE,36,43,33,0,0},
	{RACE_ELF,CLASS_MAGE,37,44,34,0,0},
	{RACE_ELF,CLASS_MAGE,38,45,34,0,0},
	{RACE_ELF,CLASS_MAGE,39,46,35,0,0},
	{RACE_ELF,CLASS_MAGE,40,48,37,0,0},
	{RACE_ELF,CLASS_MAGE,41,50,38,0,0},
	{RACE_ELF,CLASS_MAGE,42,52,38,0,0},
	{RACE_ELF,CLASS_MAGE,43,53,40,0,0},
	{RACE_ELF,CLASS_MAGE,44,54,41,0,0},
	{RACE_ELF,CLASS_MAGE,45,54,41,0,0},
	{RACE_ELF,CLASS_MAGE,46,55,42,0,0},
	{RACE_ELF,CLASS_MAGE,47,57,43,0,0},
	{RACE_ELF,CLASS_MAGE,48,58,43,0,0},
	{RACE_ELF,CLASS_MAGE,49,59,43,0,0},
	{RACE_ELF,CLASS_MAGE,50,60,45,0,0},
	{RACE_ELF,CLASS_MAGE,51,61,46,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,0,0,0,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,1,0,2,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,2,0,2,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,3,0,2,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,4,1,4,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,5,2,5,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,6,3,6,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,7,3,8,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,8,3,10,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,9,3,10,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,10,4,12,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,11,4,12,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,12,5,14,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,13,5,15,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,14,6,15,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,15,6,16,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,16,6,18,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,17,7,19,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,18,7,20,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,19,7,20,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,20,7,22,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,21,7,22,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,22,7,23,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,23,7,23,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,24,8,23,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,25,8,25,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,26,8,25,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,27,8,27,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,28,8,28,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,29,8,28,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,30,8,29,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,31,8,30,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,32,8,31,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,33,9,32,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,34,9,32,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,35,9,34,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,36,9,35,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,37,9,37,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,38,9,37,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,39,9,37,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,40,9,37,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,41,9,38,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,42,10,39,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,43,11,40,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,44,12,41,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,45,12,41,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,46,12,42,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,47,12,44,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,48,12,44,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,49,12,45,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,50,12,46,0,0},
	{RACE_ELF,CLASS_ALCHEMIST,51,13,46,0,0},
	{RACE_ELF,CLASS_PALADIN,0,0,0,0,0},
	{RACE_ELF,CLASS_PALADIN,1,1,0,0,0},
	{RACE_ELF,CLASS_PALADIN,2,2,0,0,0},
	{RACE_ELF,CLASS_PALADIN,3,3,0,0,0},
	{RACE_ELF,CLASS_PALADIN,4,4,0,0,0},
	{RACE_ELF,CLASS_PALADIN,5,5,0,0,0},
	{RACE_ELF,CLASS_PALADIN,6,6,0,0,0},
	{RACE_ELF,CLASS_PALADIN,7,7,0,0,0},
	{RACE_ELF,CLASS_PALADIN,8,8,0,0,0},
	{RACE_ELF,CLASS_PALADIN,9,9,0,0,0},
	{RACE_ELF,CLASS_PALADIN,10,10,0,0,0},
	{RACE_ELF,CLASS_PALADIN,11,11,0,0,0},
	{RACE_ELF,CLASS_PALADIN,12,12,0,0,0},
	{RACE_ELF,CLASS_PALADIN,13,13,0,0,0},
	{RACE_ELF,CLASS_PALADIN,14,14,0,0,0},
	{RACE_ELF,CLASS_PALADIN,15,15,0,0,0},
	{RACE_ELF,CLASS_PALADIN,16,16,0,0,0},
	{RACE_ELF,CLASS_PALADIN,17,17,0,0,0},
	{RACE_ELF,CLASS_PALADIN,18,18,0,0,0},
	{RACE_ELF,CLASS_PALADIN,19,19,0,0,0},
	{RACE_ELF,CLASS_PALADIN,20,20,0,0,0},
	{RACE_ELF,CLASS_PALADIN,21,21,0,0,0},
	{RACE_ELF,CLASS_PALADIN,22,22,0,0,0},
	{RACE_ELF,CLASS_PALADIN,23,23,0,0,0},
	{RACE_ELF,CLASS_PALADIN,24,24,0,0,0},
	{RACE_ELF,CLASS_PALADIN,25,25,0,0,0},
	{RACE_ELF,CLASS_PALADIN,26,26,0,0,0},
	{RACE_ELF,CLASS_PALADIN,27,27,0,0,0},
	{RACE_ELF,CLASS_PALADIN,28,28,0,0,0},
	{RACE_ELF,CLASS_PALADIN,29,29,0,0,0},
	{RACE_ELF,CLASS_PALADIN,30,30,0,0,0},
	{RACE_ELF,CLASS_PALADIN,31,31,0,0,0},
	{RACE_ELF,CLASS_PALADIN,32,32,0,0,0},
	{RACE_ELF,CLASS_PALADIN,33,33,0,0,0},
	{RACE_ELF,CLASS_PALADIN,34,34,0,0,0},
	{RACE_ELF,CLASS_PALADIN,35,35,0,0,0},
	{RACE_ELF,CLASS_PALADIN,36,36,0,0,0},
	{RACE_ELF,CLASS_PALADIN,37,37,0,0,0},
	{RACE_ELF,CLASS_PALADIN,38,38,0,0,0},
	{RACE_ELF,CLASS_PALADIN,39,39,0,0,0},
	{RACE_ELF,CLASS_PALADIN,40,40,0,0,0},
	{RACE_ELF,CLASS_PALADIN,41,41,0,0,0},
	{RACE_ELF,CLASS_PALADIN,42,42,0,0,0},
	{RACE_ELF,CLASS_PALADIN,43,43,0,0,0},
	{RACE_ELF,CLASS_PALADIN,44,44,0,0,0},
	{RACE_ELF,CLASS_PALADIN,45,45,0,0,0},
	{RACE_ELF,CLASS_PALADIN,46,46,0,0,0},
	{RACE_ELF,CLASS_PALADIN,47,47,0,0,0},
	{RACE_ELF,CLASS_PALADIN,48,48,0,0,0},
	{RACE_ELF,CLASS_PALADIN,49,49,0,0,0},
	{RACE_ELF,CLASS_PALADIN,50,50,0,0,0},
	{RACE_ELF,CLASS_PALADIN,51,51,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,0,0,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,1,1,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,2,2,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,3,3,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,4,4,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,5,6,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,6,8,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,7,9,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,8,10,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,9,11,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,10,11,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,11,12,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,12,13,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,13,14,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,14,15,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,15,17,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,16,18,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,17,20,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,18,20,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,19,22,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,20,22,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,21,24,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,22,25,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,23,26,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,24,28,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,25,29,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,26,30,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,27,32,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,28,33,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,29,34,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,30,36,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,31,38,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,32,38,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,33,38,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,34,39,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,35,39,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,36,40,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,37,40,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,38,42,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,39,43,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,40,44,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,41,45,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,42,46,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,43,46,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,44,47,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,45,47,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,46,48,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,47,50,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,48,51,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,49,52,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,50,53,0,0,0},
	{RACE_LYCANTHROPE,CLASS_LYCANTHROPE,51,54,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,0,0,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,1,1,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,2,2,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,3,3,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,4,4,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,5,4,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,6,3,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,7,3,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,8,2,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,9,2,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,10,2,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,11,1,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,12,1,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,13,2,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,14,1,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,15,1,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,16,2,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,17,3,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,18,3,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,19,2,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,20,2,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,21,2,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,22,2,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,23,1,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,24,1,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,25,2,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,26,2,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,27,1,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,28,0,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,29,1,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,30,1,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,31,1,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,32,0,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,33,0,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,34,0,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,35,0,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,36,0,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,37,0,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,38,1,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,39,1,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,40,1,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,41,0,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,42,0,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,43,1,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,44,2,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,45,2,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,46,3,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,47,4,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,48,4,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,49,3,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,50,3,0,0,0},
	{RACE_HOBBIT,CLASS_WARRIOR,51,3,0,0,0},
	{RACE_HOBBIT,CLASS_DRUID,0,0,0,0,0},
	{RACE_HOBBIT,CLASS_DRUID,1,0,1,0,0},
	{RACE_HOBBIT,CLASS_DRUID,2,0,2,0,0},
	{RACE_HOBBIT,CLASS_DRUID,3,0,2,0,0},
	{RACE_HOBBIT,CLASS_DRUID,4,0,2,0,0},
	{RACE_HOBBIT,CLASS_DRUID,5,0,2,0,0},
	{RACE_HOBBIT,CLASS_DRUID,6,0,3,0,0},
	{RACE_HOBBIT,CLASS_DRUID,7,0,2,0,0},
	{RACE_HOBBIT,CLASS_DRUID,8,0,1,0,0},
	{RACE_HOBBIT,CLASS_DRUID,9,0,0,0,0},
	{RACE_HOBBIT,CLASS_DRUID,10,0,1,0,0},
	{RACE_HOBBIT,CLASS_DRUID,11,0,0,0,0},
	{RACE_HOBBIT,CLASS_DRUID,12,0,1,0,0},
	{RACE_HOBBIT,CLASS_DRUID,13,0,0,0,0},
	{RACE_HOBBIT,CLASS_DRUID,14,0,0,0,0},
	{RACE_HOBBIT,CLASS_DRUID,15,0,1,0,0},
	{RACE_HOBBIT,CLASS_DRUID,16,0,0,0,0},
	{RACE_HOBBIT,CLASS_DRUID,17,0,1,0,0},
	{RACE_HOBBIT,CLASS_DRUID,18,0,0,0,0},
	{RACE_HOBBIT,CLASS_DRUID,19,0,1,0,0},
	{RACE_HOBBIT,CLASS_DRUID,20,0,1,0,0},
	{RACE_HOBBIT,CLASS_DRUID,21,0,1,0,0},
	{RACE_HOBBIT,CLASS_DRUID,22,0,2,0,0},
	{RACE_HOBBIT,CLASS_DRUID,23,0,2,0,0},
	{RACE_HOBBIT,CLASS_DRUID,24,0,2,0,0},
	{RACE_HOBBIT,CLASS_DRUID,25,0,2,0,0},
	{RACE_HOBBIT,CLASS_DRUID,26,0,2,0,0},
	{RACE_HOBBIT,CLASS_DRUID,27,0,2,0,0},
	{RACE_HOBBIT,CLASS_DRUID,28,0,3,0,0},
	{RACE_HOBBIT,CLASS_DRUID,29,0,3,0,0},
	{RACE_HOBBIT,CLASS_DRUID,30,0,4,0,0},
	{RACE_HOBBIT,CLASS_DRUID,31,0,5,0,0},
	{RACE_HOBBIT,CLASS_DRUID,32,0,6,0,0},
	{RACE_HOBBIT,CLASS_DRUID,33,0,6,0,0},
	{RACE_HOBBIT,CLASS_DRUID,34,0,7,0,0},
	{RACE_HOBBIT,CLASS_DRUID,35,0,8,0,0},
	{RACE_HOBBIT,CLASS_DRUID,36,0,8,0,0},
	{RACE_HOBBIT,CLASS_DRUID,37,0,8,0,0},
	{RACE_HOBBIT,CLASS_DRUID,38,0,9,0,0},
	{RACE_HOBBIT,CLASS_DRUID,39,0,10,0,0},
	{RACE_HOBBIT,CLASS_DRUID,40,0,10,0,0},
	{RACE_HOBBIT,CLASS_DRUID,41,0,10,0,0},
	{RACE_HOBBIT,CLASS_DRUID,42,0,10,0,0},
	{RACE_HOBBIT,CLASS_DRUID,43,0,10,0,0},
	{RACE_HOBBIT,CLASS_DRUID,44,0,11,0,0},
	{RACE_HOBBIT,CLASS_DRUID,45,0,11,0,0},
	{RACE_HOBBIT,CLASS_DRUID,46,0,11,0,0},
	{RACE_HOBBIT,CLASS_DRUID,47,0,10,0,0},
	{RACE_HOBBIT,CLASS_DRUID,48,0,11,0,0},
	{RACE_HOBBIT,CLASS_DRUID,49,0,11,0,0},
	{RACE_HOBBIT,CLASS_DRUID,50,0,10,0,0},
	{RACE_HOBBIT,CLASS_DRUID,51,0,10,0,0},
	{RACE_HOBBIT,CLASS_CLERIC,0,0,0,0,0},
	{RACE_HOBBIT,CLASS_CLERIC,1,0,0,0,1},
	{RACE_HOBBIT,CLASS_CLERIC,2,0,0,0,2},
	{RACE_HOBBIT,CLASS_CLERIC,3,0,1,0,3},
	{RACE_HOBBIT,CLASS_CLERIC,4,0,1,0,4},
	{RACE_HOBBIT,CLASS_CLERIC,5,0,2,0,5},
	{RACE_HOBBIT,CLASS_CLERIC,6,0,3,0,6},
	{RACE_HOBBIT,CLASS_CLERIC,7,0,4,0,7},
	{RACE_HOBBIT,CLASS_CLERIC,8,0,5,0,8},
	{RACE_HOBBIT,CLASS_CLERIC,9,0,6,0,9},
	{RACE_HOBBIT,CLASS_CLERIC,10,0,6,0,10},
	{RACE_HOBBIT,CLASS_CLERIC,11,0,7,0,11},
	{RACE_HOBBIT,CLASS_CLERIC,12,0,8,0,12},
	{RACE_HOBBIT,CLASS_CLERIC,13,0,8,0,13},
	{RACE_HOBBIT,CLASS_CLERIC,14,0,9,0,14},
	{RACE_HOBBIT,CLASS_CLERIC,15,0,10,0,15},
	{RACE_HOBBIT,CLASS_CLERIC,16,0,11,0,16},
	{RACE_HOBBIT,CLASS_CLERIC,17,0,12,0,17},
	{RACE_HOBBIT,CLASS_CLERIC,18,0,12,0,18},
	{RACE_HOBBIT,CLASS_CLERIC,19,0,12,0,19},
	{RACE_HOBBIT,CLASS_CLERIC,20,0,12,0,20},
	{RACE_HOBBIT,CLASS_CLERIC,21,0,13,0,21},
	{RACE_HOBBIT,CLASS_CLERIC,22,0,13,0,22},
	{RACE_HOBBIT,CLASS_CLERIC,23,0,14,0,23},
	{RACE_HOBBIT,CLASS_CLERIC,24,0,15,0,24},
	{RACE_HOBBIT,CLASS_CLERIC,25,0,16,0,25},
	{RACE_HOBBIT,CLASS_CLERIC,26,0,16,0,26},
	{RACE_HOBBIT,CLASS_CLERIC,27,0,17,0,27},
	{RACE_HOBBIT,CLASS_CLERIC,28,0,17,0,28},
	{RACE_HOBBIT,CLASS_CLERIC,29,0,18,0,29},
	{RACE_HOBBIT,CLASS_CLERIC,30,0,19,0,30},
	{RACE_HOBBIT,CLASS_CLERIC,31,0,19,0,31},
	{RACE_HOBBIT,CLASS_CLERIC,32,0,19,0,32},
	{RACE_HOBBIT,CLASS_CLERIC,33,0,20,0,33},
	{RACE_HOBBIT,CLASS_CLERIC,34,0,20,0,34},
	{RACE_HOBBIT,CLASS_CLERIC,35,0,20,0,35},
	{RACE_HOBBIT,CLASS_CLERIC,36,0,21,0,36},
	{RACE_HOBBIT,CLASS_CLERIC,37,0,22,0,37},
	{RACE_HOBBIT,CLASS_CLERIC,38,0,23,0,38},
	{RACE_HOBBIT,CLASS_CLERIC,39,0,24,0,39},
	{RACE_HOBBIT,CLASS_CLERIC,40,0,25,0,40},
	{RACE_HOBBIT,CLASS_CLERIC,41,0,25,0,41},
	{RACE_HOBBIT,CLASS_CLERIC,42,0,26,0,42},
	{RACE_HOBBIT,CLASS_CLERIC,43,0,27,0,43},
	{RACE_HOBBIT,CLASS_CLERIC,44,0,28,0,44},
	{RACE_HOBBIT,CLASS_CLERIC,45,0,28,0,45},
	{RACE_HOBBIT,CLASS_CLERIC,46,0,29,0,46},
	{RACE_HOBBIT,CLASS_CLERIC,47,0,30,0,47},
	{RACE_HOBBIT,CLASS_CLERIC,48,0,30,0,48},
	{RACE_HOBBIT,CLASS_CLERIC,49,0,30,0,49},
	{RACE_HOBBIT,CLASS_CLERIC,50,0,30,0,50},
	{RACE_HOBBIT,CLASS_CLERIC,51,0,30,0,51},
	{RACE_HOBBIT,CLASS_THIEF,0,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,1,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,2,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,3,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,4,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,5,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,6,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,7,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,8,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,9,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,10,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,11,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,12,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,13,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,14,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,15,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,16,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,17,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,18,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,19,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,20,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,21,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,22,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,23,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,24,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,25,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,26,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,27,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,28,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,29,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,30,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,31,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,32,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,33,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,34,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,35,0,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,36,1,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,37,1,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,38,1,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,39,1,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,40,1,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,41,1,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,42,1,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,43,1,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,44,1,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,45,1,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,46,1,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,47,1,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,48,1,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,49,1,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,50,1,0,0,0},
	{RACE_HOBBIT,CLASS_THIEF,51,1,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,0,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,1,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,2,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,3,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,4,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,5,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,6,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,7,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,8,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,9,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,10,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,11,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,12,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,13,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,14,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,15,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,16,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,17,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,18,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,19,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,20,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,21,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,22,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,23,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,24,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,25,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,26,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,27,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,28,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,29,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,30,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,31,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,32,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,33,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,34,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,35,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,36,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,37,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,38,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,39,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,40,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,41,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,42,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,43,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,44,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,45,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,46,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,47,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,48,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,49,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,50,0,0,0,0},
	{RACE_HOBBIT,CLASS_RANGER,51,0,0,0,0},
	{RACE_HOBBIT,CLASS_MAGE,0,0,0,0,0},
	{RACE_HOBBIT,CLASS_MAGE,1,0,1,0,0},
	{RACE_HOBBIT,CLASS_MAGE,2,1,2,0,0},
	{RACE_HOBBIT,CLASS_MAGE,3,0,3,0,0},
	{RACE_HOBBIT,CLASS_MAGE,4,0,3,0,0},
	{RACE_HOBBIT,CLASS_MAGE,5,0,3,0,0},
	{RACE_HOBBIT,CLASS_MAGE,6,0,4,0,0},
	{RACE_HOBBIT,CLASS_MAGE,7,0,4,0,0},
	{RACE_HOBBIT,CLASS_MAGE,8,0,4,0,0},
	{RACE_HOBBIT,CLASS_MAGE,9,1,4,0,0},
	{RACE_HOBBIT,CLASS_MAGE,10,1,5,0,0},
	{RACE_HOBBIT,CLASS_MAGE,11,2,5,0,0},
	{RACE_HOBBIT,CLASS_MAGE,12,3,5,0,0},
	{RACE_HOBBIT,CLASS_MAGE,13,2,6,0,0},
	{RACE_HOBBIT,CLASS_MAGE,14,2,7,0,0},
	{RACE_HOBBIT,CLASS_MAGE,15,2,7,0,0},
	{RACE_HOBBIT,CLASS_MAGE,16,3,7,0,0},
	{RACE_HOBBIT,CLASS_MAGE,17,3,8,0,0},
	{RACE_HOBBIT,CLASS_MAGE,18,3,8,0,0},
	{RACE_HOBBIT,CLASS_MAGE,19,3,8,0,0},
	{RACE_HOBBIT,CLASS_MAGE,20,3,8,0,0},
	{RACE_HOBBIT,CLASS_MAGE,21,3,8,0,0},
	{RACE_HOBBIT,CLASS_MAGE,22,3,7,0,0},
	{RACE_HOBBIT,CLASS_MAGE,23,4,8,0,0},
	{RACE_HOBBIT,CLASS_MAGE,24,5,9,0,0},
	{RACE_HOBBIT,CLASS_MAGE,25,4,8,0,0},
	{RACE_HOBBIT,CLASS_MAGE,26,4,9,0,0},
	{RACE_HOBBIT,CLASS_MAGE,27,4,9,0,0},
	{RACE_HOBBIT,CLASS_MAGE,28,3,9,0,0},
	{RACE_HOBBIT,CLASS_MAGE,29,3,10,0,0},
	{RACE_HOBBIT,CLASS_MAGE,30,2,10,0,0},
	{RACE_HOBBIT,CLASS_MAGE,31,3,10,0,0},
	{RACE_HOBBIT,CLASS_MAGE,32,3,11,0,0},
	{RACE_HOBBIT,CLASS_MAGE,33,3,12,0,0},
	{RACE_HOBBIT,CLASS_MAGE,34,4,13,0,0},
	{RACE_HOBBIT,CLASS_MAGE,35,4,12,0,0},
	{RACE_HOBBIT,CLASS_MAGE,36,3,12,0,0},
	{RACE_HOBBIT,CLASS_MAGE,37,4,12,0,0},
	{RACE_HOBBIT,CLASS_MAGE,38,4,13,0,0},
	{RACE_HOBBIT,CLASS_MAGE,39,4,13,0,0},
	{RACE_HOBBIT,CLASS_MAGE,40,4,13,0,0},
	{RACE_HOBBIT,CLASS_MAGE,41,4,13,0,0},
	{RACE_HOBBIT,CLASS_MAGE,42,3,14,0,0},
	{RACE_HOBBIT,CLASS_MAGE,43,4,14,0,0},
	{RACE_HOBBIT,CLASS_MAGE,44,4,15,0,0},
	{RACE_HOBBIT,CLASS_MAGE,45,5,16,0,0},
	{RACE_HOBBIT,CLASS_MAGE,46,5,17,0,0},
	{RACE_HOBBIT,CLASS_MAGE,47,5,18,0,0},
	{RACE_HOBBIT,CLASS_MAGE,48,6,18,0,0},
	{RACE_HOBBIT,CLASS_MAGE,49,6,19,0,0},
	{RACE_HOBBIT,CLASS_MAGE,50,6,19,0,0},
	{RACE_HOBBIT,CLASS_MAGE,51,6,19,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,0,0,0,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,1,1,0,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,2,2,0,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,3,2,0,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,4,2,0,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,5,3,0,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,6,3,0,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,7,2,0,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,8,2,0,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,9,3,1,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,10,4,1,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,11,4,1,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,12,4,1,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,13,4,1,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,14,4,2,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,15,5,2,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,16,6,1,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,17,6,2,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,18,6,3,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,19,6,4,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,20,6,4,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,21,5,5,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,22,5,5,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,23,5,6,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,24,5,6,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,25,5,6,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,26,5,6,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,27,5,6,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,28,5,6,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,29,5,6,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,30,4,5,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,31,3,5,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,32,3,6,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,33,3,6,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,34,4,6,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,35,4,5,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,36,3,5,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,37,3,6,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,38,3,6,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,39,4,6,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,40,4,6,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,41,4,5,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,42,4,6,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,43,5,5,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,44,6,4,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,45,5,4,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,46,4,4,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,47,4,5,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,48,3,5,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,49,2,6,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,50,2,5,0,0},
	{RACE_HOBBIT,CLASS_ALCHEMIST,51,1,4,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,0,0,0,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,1,0,0,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,2,0,1,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,3,0,1,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,4,0,2,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,5,0,2,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,6,0,2,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,7,0,2,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,8,0,3,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,9,0,3,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,10,0,2,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,11,0,2,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,12,0,1,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,13,0,0,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,14,0,1,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,15,0,2,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,16,0,2,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,17,0,3,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,18,0,2,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,19,0,1,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,20,0,1,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,21,0,1,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,22,0,2,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,23,0,3,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,24,0,3,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,25,0,2,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,26,0,2,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,27,0,1,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,28,0,2,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,29,0,3,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,30,0,4,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,31,0,5,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,32,0,5,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,33,0,6,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,34,0,5,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,35,0,5,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,36,0,4,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,37,0,5,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,38,0,4,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,39,0,3,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,40,0,3,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,41,0,3,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,42,0,3,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,43,0,4,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,44,0,4,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,45,0,4,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,46,0,5,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,47,0,5,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,48,0,4,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,49,0,5,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,50,0,5,0,0},
	{RACE_HOBBIT,CLASS_PALADIN,51,0,5,0,0}
};





/*
 * Deal with sockets that haven't logged in yet.
 */
void nanny(DESCRIPTOR_DATA *d, char *argument){
    DESCRIPTOR_DATA *d_old, *d_next;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *ch, *wch;
    int iClass, race, i, weapon;
    bool fOld;

    while (IS_SPACE(*argument)){
		argument++;
    }


    ch = d->character;

    if (d->codepage == CODEPAGE_TRANS){
        switch(d->connected){
        case CON_GET_OLD_PASSWORD:
        case CON_GET_NUM_REGANSWER:
        case CON_GET_NEW_PASSWORD:
        case CON_CONFIRM_NEW_PASSWORD:
        case CON_GET_EMAIL:
            break;
        default:
            recode_trans(argument, FALSE);
            break;
        }
    }

    switch (d->connected) {
        default:
            bugf("Nanny: bad d->connected %d.", d->connected);
            close_socket(d);
            return;

            /*ВЫБОР КОДИРОВКИ*/
        case CON_GET_CODEPAGE:
            bugf("select codepage. %s.", d->ip);
            switch (argument[0]) {
                case '1':
                case '\0':
                    d->codepage = CODEPAGE_WIN;
                    break;
                case '2':
                    d->codepage = CODEPAGE_KOI;
                    break;
                case '3':
                    d->codepage = CODEPAGE_ALT;
                    break;
                case '4':
                    d->codepage = CODEPAGE_WINIAC;
                    break;
                case '5':
                    d->codepage = CODEPAGE_MAC;
                    break;
                case '6':
                    d->codepage = CODEPAGE_TRANS;
                    break;
                default:
                    write_to_buffer(d, "Please, 1, 2, 3, 4, 5 or 6?", 0);
                    return;
            }

            write_to_buffer(d, "Под каким именем ты здесь будешь?", 0);
            d->connected = CON_GET_NAME;
            break;

            /*****************************
            * ВВОД ИМЕНИ
            *****************************/
        case CON_GET_NAME:
            if (argument[0] == '\0') {
                close_socket(d);
                return;
            }

            bugf("Get name %s. ip: %s", argument, d->ip);
            if (!check_parse_name(argument)) {
                write_to_buffer(d, "Неправильное имя, выберите другое.\n\r"
                        "(Имя должно состоять из 3-10 русских букв и не совпадать с существующими)\n\r"
                        "Имя: ", 0);
                return;
            }

            convert_name(argument);

            fOld = load_char_obj(d, argument, FALSE, FALSE);
            ch = d->character;

            if (IS_SET(ch->act, PLR_DENY)) {
                sprintf(buf, "Denying access to %s@%s.", argument, d->ip);
                log_string(buf);
                write_to_buffer(d, "У Вас нет доступа.\n\r", 0);
                close_socket(d);
                return;
            }

            if (check_ban(d->ip, BAN_PERMIT) && !IS_SET(ch->act, PLR_PERMIT)) {
                write_to_buffer(d, "Ваш хост был заблокирован в этом мире.\n\r", 0);
                close_socket(d);
                return;
            }

            if (check_ban(d->ip, BAN_NEWBIES) && ch->level <= 5) {
                write_to_buffer(d, "Игра новыми персонажами с вашего хоста "
                        "ЗАПРЕЩЕНА!\n\r", 0);
                close_socket(d);
                return;
            }

            if (ch->pcdata->ips) {
                NAME_LIST *ip;

                for (ip = ch->pcdata->ips; ip; ip = ip->next)
                    if (!str_prefix(ip->name, d->ip))
                        break;

                if (!ip) {
                    write_to_buffer(d, "\n\rЭтому персонажу запрещено играть с такого IP-адреса.\n\r", 0);
                    close_socket(d);
                    return;
                }
            }

            if (check_reconnect(d, argument, FALSE)) {
                fOld = TRUE;
            } else {
                if (cfg.wizlock && !IS_IMMORTAL(ch))
                    //	    if (!IS_IMMORTAL(ch))
                {
                    write_to_buffer(d, "Игра закрыта богами. Попробуйте через "
                            "несколько минут.\n\r", 0);
                    close_socket(d);
                    return;
                }
            }


            if (fOld) {
                /* Old player */
                write_to_descriptor(d, echo_off_str, -1);
                write_to_buffer(d, "Пароль: ", 0);
                d->connected = CON_GET_OLD_PASSWORD;
                return;
            } else {
                bugf("create new player. %s %s", argument, d->ip);
                /* New player */
                if (cfg.newlock) {
                    write_to_buffer(d, "Игра закрыта для новых игроков.\n\r", 0);
                    close_socket(d);
                    return;
                }

                if (check_ban(d->ip, BAN_NEWBIES)) {
                    write_to_buffer(d, "С Вашего хоста запрещены новые игроки.\n\r",
                                    0);
                    close_socket(d);
                    return;
                }

                do_function(d->character, &do_help, "имя");

                sprintf(buf, "%s, это правильно (Да/Нет)? ", argument);
                write_to_buffer(d, buf, 0);
                d->connected = CON_CONFIRM_NEW_NAME;
                return;
            }
            break;

        case CON_GET_OLD_PASSWORD:

            write_to_buffer(d, "\n\r", 2);

            if (!ch->pcdata || strcmp(argument, ch->pcdata->pwd)) {
                bugf("CON GET OLD PWD: not correct pass: %s ip: %s pass: %s", ch->name, d->ip, argument);

                write_to_buffer(d, "Неправильный пароль.\n\r", 0);
                write_to_buffer(d,
                                "Неправильный пароль может быть следствием автоматической смены пароля, если вашего персонажа\n\r",
                                0);
                write_to_buffer(d,
                                "пытались взломать (ввели подряд 3 раза неверный ответ на запрос контрольной буквы).\n\r",
                                0);
                write_to_buffer(d,
                                "В этом случае пароль высылается на почту, указанную при регистрации. Проверьте свою почту.\n\r",
                                0);
                close_socket(d);
                return;
            }


            if (ch->pcdata->reg_answer != NULL && (strlen(ch->pcdata->reg_answer) > 0)) {
                write_to_descriptor(d, echo_on_str, -1);
                ch->pcdata->num_char = number_range(1, strlen(ch->pcdata->reg_answer));
                sprintf(buf,
                        "Введите %d-ю букву вашего ответа на регистрационный вопрос (девичья фамилия вашей мамы): ",
                        ch->pcdata->num_char);
                write_to_buffer(d, "Запрос контрольной буквы:\n\r", 0);
                write_to_buffer(d, buf, 0);

                d->connected = CON_GET_NUM_REGANSWER;
            } else {
                if (IS_IMMORTAL(ch)) {
                    do_function(ch, &do_help, "imotd");
                    d->connected = CON_READ_IMOTD;
                } else {
                    do_function(ch, &do_help, "motd");
                    d->connected = CON_READ_MOTD;
                }
            }
            break;

            /* RT code for breaking link */

        case CON_GET_NUM_REGANSWER:
            write_to_buffer(d, "\n\r", 2);

            if (IS_NULLSTR(argument)) {
                write_to_buffer(d, "Вы не ввели контрольную букву. Попробуйте еще раз.\n\r", 0);
                return;
            }

            if (!ch->pcdata || (LOWER(argument[0]) != LOWER(ch->pcdata->reg_answer[ch->pcdata->num_char - 1]))) {
                bugf("Not correct reganswer:%s ch:%s ip:%s", argument, ch->name, d->ip);
                write_to_buffer(d, "Неправильный ответ.\n\r", 0);
                ch->pcdata->count_entries_errors++;

                if (ch->pcdata->count_entries_errors > 2) {
                    bugf("3x Not correct reganswer:%s ch:%s ip:%s", argument, ch->name, d->ip);

                    /*	free_string(ch->pcdata->pwd);
            */
                    generate_character_password(ch);
                    do_email_pass(NULL, ch->name);
                }
                save_char_obj(ch, FALSE);
                close_socket(d);
                return;
            }

            write_to_descriptor(d, echo_on_str, -1);

            if (check_playing(d, ch->name))
                return;

            if (check_reconnect(d, ch->name, TRUE))
                return;

            if (IS_IMMORTAL(ch)) {
                do_function(ch, &do_help, "imotd");
                d->connected = CON_READ_IMOTD;
            } else {
                do_function(ch, &do_help, "motd");
                d->connected = CON_READ_MOTD;
            }
            break;

        case CON_BREAK_CONNECT:

            switch (*argument) {
                case 'y' :
                case 'Y':
                case 'д' :
                case 'Д':
                    SLIST_FOREACH_SAFE(d_old, &descriptor_list, link, d_next) {
                        if (d_old == d || d_old->character == NULL)
                            continue;

                        if (str_cmp(ch->name, CH(d_old)->name))
                            continue;

                        close_socket(d_old);
                    }

                    if (check_reconnect(d, ch->name, TRUE))
                        return;

                    write_to_buffer(d, "Не удалось переподключиться.\n\rИмя: ", 0);
                    if (d->character != NULL) {
                        free_char(d->character);
                        d->character = NULL;
                    }
                    d->connected = CON_GET_NAME;
                    break;

                case 'n' :
                case 'N':
                case 'н' :
                case 'Н':
                    write_to_buffer(d, "Имя: ", 0);
                    if (d->character != NULL) {
                        free_char(d->character);
                        d->character = NULL;
                    }
                    d->connected = CON_GET_NAME;
                    break;

                default:
                    write_to_buffer(d, "Пожалуйста, Да или Нет? ", 0);
                    break;
            }
            break;

        case CON_CONFIRM_NEW_NAME:
            switch (*argument) {
                case 'y':
                case 'Y':
                case 'д':
                case 'Д':
                    /*
                     * check names of people playing. Yes, this is necessary for multiple
                     * newbies with the same name (thanks Saro)
                     */
                    if (!SLIST_EMPTY(&descriptor_list)) {
                        DESCRIPTOR_DATA *dc, *d_next;
                        int count = 0;
                        char name[MIL];

                        strlcpy(name, ch->name, MIL);

                        SLIST_FOREACH_SAFE(dc, &descriptor_list, link, d_next) {
                            if (d != dc && dc->character && !IS_NULLSTR(dc->character->name)
                                && !str_cmp(dc->character->name, name)
                                && dc->connected != CON_PLAYING) {
                                count++;
                                write_to_buffer(d, "В этот момент кто-то еще создает персонажа с таким же именем!\n\r",
                                                0);
                                close_socket(d);
                            }
                        }
                        if (count) {
                            char bfr[MAX_INPUT_LENGTH];

                            sprintf(bfr, "Double newbie alert (%s)", name);
                            wiznet(bfr, NULL, NULL, WIZ_LOGINS, 0, 0);
                            return;
                        }
                    }

                    sprintf(buf, "\n\rНовый персонаж.\n\r\n\r"
                            "Введите Ваш пароль для %s: ", ch->name);
                    write_to_buffer(d, buf, 0);
                    write_to_descriptor(d, echo_off_str, -1);
                    d->connected = CON_GET_NEW_PASSWORD;
                    break;

                case 'n':
                case 'N':
                case 'н':
                case 'Н':
                    write_to_buffer(d, "Ok, что дальше? ", 0);
                    free_char(d->character);
                    d->character = NULL;
                    d->connected = CON_GET_NAME;
                    break;

                default:
                    write_to_buffer(d, "Пожалуйста, Да или Нет? ", 0);
                    break;
            }
            break;

        case CON_GET_NEW_PASSWORD:
            write_to_buffer(d, "\n\r", 2);

            if (!check_password(d, argument)) {
                write_to_buffer(d, "Пароль: ", 0);
                return;
            }

            free_string(ch->pcdata->pwd);
            ch->pcdata->pwd = str_dup(argument);
            write_to_buffer(d, "Повторите пароль: ", 0);
            d->connected = CON_CONFIRM_NEW_PASSWORD;
            break;

        case CON_CONFIRM_NEW_PASSWORD:
            write_to_buffer(d, "\n\r", 2);
            if (strcmp(argument, ch->pcdata->pwd)) {
                write_to_buffer(d, "Пароли не совпадают.\n\rПароль: ",
                                0);
                d->connected = CON_GET_NEW_PASSWORD;
                return;
            }

            write_to_descriptor(d, echo_on_str, -1);
            write_to_buffer(d, "Будете ли Вы использовать цвета (Да/Нет)? ", 0);
            d->connected = CON_GET_COLOR;
            break;

        case CON_GET_COLOR:
            switch (argument[0]) {
                case 'Y':
                case 'y':
                case 'Д':
                case 'д': {
                    write_to_buffer(d, "\n\r", 2);
                    do_function(ch, &do_colour, "noprint");
                    /*     	    write_to_buffer(d, "\n\r", 2); */
                    break;
                }
                case 'N':
                case 'n':
                case 'Н':
                case 'н':
                    break;
                default: {
                    write_to_buffer(d, "Пожалуйста, Да или Нет? ", 0);
                    return;
                }
            }
            /*       	write_to_buffer(d, echo_on_str, 0); */

            write_to_buffer(d, "Выберите свой пол [ (М)ужской/(Ж)енский ]: ", 0);
            d->connected = CON_GET_NEW_SEX;
            break;


        case CON_GET_NEW_RACE:
            one_argument(argument, arg);

            if (!str_prefix(arg, "help")
                || !str_prefix(arg, "помощь")
                || !str_cmp(arg, "?")) {
                write_to_buffer(d, "\n\r", 0);
                argument = one_argument(argument, arg);
                if (argument[0] == '\0')
                    do_function(ch, &do_help, "race help");
                else
                    do_function(ch, &do_help, argument);

                write_to_buffer(d, "\n\rВыберите расу ('помощь' для информации):",
                                0);
                break;
            }

            race = race_lookup(argument);
            //findme
            if (race == -1 || !race_table[race].pc_race) {
                write_to_buffer(d, "Такой расы в этом мире нет.\n\r", 0);
                write_to_buffer(d, "Доступны следующие расы:\n\r  [ ", 0);
                for (race = 1; race < MAX_PC_RACE; race++) {
                    write_to_buffer(d, race_table[race].rname, 0);
                    write_to_buffer(d, " ", 1);
                }
                write_to_buffer(d, "]\n\r", 0);
                write_to_buffer(d, "\n\rВаша раса ('помощь' для информации): ", 0);
                break;
            }

            ch->race = race;
            /* initialize stats */
            for (i = 0; i < MAX_STATS; i++)
                ch->perm_stat[i] = pc_race_table[race].stats[i];

            ch->affected_by = ch->affected_by | race_table[race].aff;
            ch->form = race_table[race].form;
            ch->parts = race_table[race].parts;
            for (i = 0; i < DAM_MAX; i++){
                ch->resists[i] = race_table[race].resists[i];
            }
        /* ХЗ ЗАЧЕМ РАНЬШЕ ВЕШАЛИ РЕЗИЗСТ К МАГИИ НА ГЕНЕРИРУЕМОГО ЧАРА*/
        //ch->resists[DAM_MAGIC] = 10;

        /* ДАДИМ НЕМНОГО ЗОЛОТА В НАЧАЛЕ*/
        ch->gold += 100;
        ch->pcdata->quest_curr += 1000;
        ch->pcdata->quest_accum += 1000;

        /* add skills */
        for (i = 0; i < 5; i++)
        {
            if (pc_race_table[race].skills[i] == NULL)
            break;
            group_add(ch, pc_race_table[race].skills[i], FALSE);
        }
        /* add cost */
        ch->pcdata->points = pc_race_table[race].points;
        ch->size = pc_race_table[race].size;

        strcpy(buf, "\n\rВам доступны следующие профессии:\n\r  [");

        for (iClass = 0; iClass < MAX_CLASS; iClass++)
        {
            if (pc_race_table[ch->race].class_mult[iClass] == 0
            || (pc_race_table[ch->race].valid_align
                & class_table[iClass].valid_align) == 0)
            {
            continue;
            }

            strcat(buf, " ");
            strcat(buf, class_table[iClass].name);
        }
        strcat(buf, " ] ");
        write_to_buffer(d, buf, 0);
        d->connected = CON_GET_NEW_CLASS;
        break;


        case CON_GET_NEW_SEX:
        switch (argument[0])
        {
        case 'м':
        case 'М':
        case 'm':
        case 'M':
            ch->sex = SEX_MALE;
            ch->pcdata->true_sex = SEX_MALE;
            break;

        case 'ж':
        case 'Ж':
        case 'f':
        case 'F':
            ch->sex = SEX_FEMALE;
            ch->pcdata->true_sex = SEX_FEMALE;
            break;

        default:
            write_to_buffer(d, "\n\rНеправильный пол.\n\rВаш пол? ", 0);
            return;
        }

        write_to_buffer(d, "\n\rДоступны следующие расы:\n\r  [ ", 0);

        for (race = 1; race < MAX_PC_RACE; race++)
        {
            if (!race_table[race].pc_race)
            break;
            write_to_buffer(d, race_table[race].rname, 0);
            write_to_buffer(d, " ", 1);
        }

        write_to_buffer(d, "]\n\r", 0);
        write_to_buffer(d, "Выберите свою расу (для подробной информации наберите помощь): ", 0);
        d->connected = CON_GET_NEW_RACE;
        break;

        case CON_GET_NEW_CLASS:
        iClass = class_lookup(argument);

    /* класс алхимиков при генерации нельзя выбрать findme */
        if (iClass == -1)
        {
            write_to_buffer(d,	"\n\rТакой профессии нет.\n\rВаша профессия? ", 0);
            return;
        }

        if (pc_race_table[ch->race].class_mult[iClass] == 0
            || (pc_race_table[ch->race].valid_align
            & class_table[iClass].valid_align) == 0)
        {
            write_to_buffer(d,	"\n\rЭта профессия Вам недоступна.\n\rВаша профессия? ", 0);
            return;
        }

        ch->classid = iClass;

        sprintf(buf, "%s@%s new player.", ch->name, d->ip);
        log_string(buf);
        wiznet("Новый игрок - $N !", ch, NULL, WIZ_NEWBIE, 0, 0);
        wiznet(buf, NULL, NULL, WIZ_SITES, 0, get_trust(ch));

        write_to_buffer(d, "\n\r", 2);

        show_valid_align(ch);

        d->connected = CON_GET_ALIGNMENT;
        break;

        case CON_GET_ALIGNMENT:

        if (((argument[0] == 'g' || argument[0] == 'G' || argument[0] == 'д' || argument[0] == 'Д')
             && IS_SET(get_valid_align(ch), ALIGN_GOOD)) || is_only_align(ch) == ALIGN_GOOD)
            ch->alignment = 750;
        else
            if (((argument[0] == 'n' || argument[0] == 'N' || argument[0] == 'н' || argument[0] == 'Н')
             && IS_SET(get_valid_align(ch), ALIGN_NEUTRAL)) || is_only_align(ch) == ALIGN_NEUTRAL)
            ch->alignment = 0;
            else
            if (((argument[0] == 'e' || argument[0] == 'E' || argument[0] == 'з' || argument[0] == 'З')
                 && IS_SET(get_valid_align(ch), ALIGN_EVIL)) || is_only_align(ch) == ALIGN_EVIL)
                ch->alignment = -750;
            else
            {
                write_to_buffer(d, "Неправильный выбор.\n\r", 0);
                show_valid_align(ch);
                return;
            }

        ch->pcdata->orig_align = ch->alignment;

        write_to_buffer(d, "\n\r", 0);

        group_add(ch, "rom basics", FALSE);
        group_add(ch, class_table[ch->classid].base_group, FALSE);
        ch->pcdata->learned[gsn_recall] = 50;
        write_to_buffer(d, "Вы хотите дополнительно настроить своего персонажа?\n\r", 0);
        write_to_buffer(d, "Это займет некоторое время, но позволит добавить дополнительные умения и возможности.\n\r", 0);
        write_to_buffer(d, "Настроить (Да/Нет)? ", 0);
        d->connected = CON_DEFAULT_CHOICE;
        break;

        case CON_DEFAULT_CHOICE:
        write_to_buffer(d, "\n\r", 2);
        switch (argument[0])
        {
        case 'y': case 'Y': case 'д': case 'Д':
            ch->gen_data = new_gen_data();
            group_add(ch, class_table[ch->classid].default_group, TRUE);
            ch->gen_data->points_chosen = ch->pcdata->points;
            /*	    do_function(ch, &do_help, "group header"); */
            list_group_costs(ch);
            write_to_buffer(d, "У Вас уже есть следующие умения:\n\r", 0);
            do_function(ch, &do_skills, "");
            write_to_buffer(d, "\n\r", 0);
            do_function(ch, &do_help, "menu choice");
            d->connected = CON_GEN_GROUPS;
            break;
        case 'n': case 'N':  case 'н': case 'Н':
            group_add(ch, class_table[ch->classid].default_group, TRUE);
            write_to_buffer(d, "\n\rВыберите оружие из списка:\n\r    ", 0);
            buf[0] = '\0';
            for (i = 0; weapon_table[i].rname != NULL; i++)
            if (weapon_table[i].can_select && ch->pcdata->learned[*weapon_table[i].gsn] > 0)
            {
                strcat(buf, weapon_table[i].rname);
                strcat(buf, " ");
            }
            strcat(buf, "\n\rВаш выбор? ");
            write_to_buffer(d, buf, 0);
            d->connected = CON_PICK_WEAPON;
            break;
        default:
            write_to_buffer(d, "Пожалуйста, Да или Нет? ", 0);
            return;
        }
        break;

        case CON_PICK_WEAPON:
        write_to_buffer(d, "\n\r", 2);
        weapon = weapon_lookup(argument);
        if (weapon == -1
            || ch->pcdata->learned[*weapon_table[weapon].gsn] <= 0
            || !weapon_table[weapon].can_select)
        {
            write_to_buffer(d,
                    "Выбрано неправильное оружие. Выберите из списка:\n\r    ", 0);
            buf[0] = '\0';
            for (i = 0; weapon_table[i].rname != NULL; i++)
            if (weapon_table[i].can_select && ch->pcdata->learned[*weapon_table[i].gsn] > 0)
            {
                strcat(buf, weapon_table[i].rname);
                strcat(buf, " ");
            }
            strcat(buf, "\n\rВаш выбор? ");
            write_to_buffer(d, buf, 0);
            return;
        }

        ch->pcdata->learned[*weapon_table[weapon].gsn] = 40;
        write_to_buffer(d, "Введите девичью фамилию вашей матери (необходимо на случай кражи чара/утери пароля):\n\r", 0);
        d->connected = CON_GET_QUESTION;

        break;


        case CON_GET_QUESTION:
        if (IS_NULLSTR(argument))
        {
            write_to_buffer(d, "Введите, пожалуйста, девичью фамилию вашей матери.\n\r", 0);
            return;
        }

        free_string(ch->pcdata->reg_answer);
        smash_tilde(argument);
        ch->pcdata->reg_answer = str_dup(argument);

        write_to_buffer(d, "Выберите, как будет склоняться Ваше имя:\n\r", 0);
        sprintf(buf, "Родительный падеж (вещь кого?)    : [%s]", capitalize(cases(ch->name, 1)));
        write_to_buffer(d, buf, 0);
        d->connected = CON_GET_CASES_1;

        break;


        case CON_GET_CASES_1:
        sprintf(buf, "%s", capitalize(IS_NULLSTR(argument) ? cases(ch->name, 1) : argument));

        if (!check_case(ch, buf, 1))
            break;

        free_string(ch->pcdata->cases[0]);
        smash_tilde(buf);
        strip_colors(buf);
        ch->pcdata->cases[0] = str_dup(buf);
        sprintf(buf, "Дательный падеж (дать кому?)      : [%s]", capitalize(cases(ch->name, 2)));
        write_to_buffer(d, buf, 0);
        d->connected = CON_GET_CASES_2;
        break;

        case CON_GET_CASES_2:
        sprintf(buf, "%s", capitalize(IS_NULLSTR(argument) ? cases(ch->name, 2) : argument));

        if (!check_case(ch, buf, 2))
            break;

        free_string(ch->pcdata->cases[1]);
        smash_tilde(buf);
        strip_colors(buf);
        ch->pcdata->cases[1] = str_dup(buf);

        sprintf(buf, "Винительный падеж (ударить кого?) : [%s]", capitalize(cases(ch->name, 3)));
        write_to_buffer(d, buf, 0);
        d->connected = CON_GET_CASES_3;
        break;

        case CON_GET_CASES_3:
        sprintf(buf, "%s", capitalize(IS_NULLSTR(argument) ? cases(ch->name, 3) : argument));

        if (!check_case(ch, buf, 3))
            break;

        free_string(ch->pcdata->cases[2]);
        smash_tilde(buf);
        strip_colors(buf);
        ch->pcdata->cases[2] = str_dup(buf);
        sprintf(buf, "Творительный падеж (сделано кем?) : [%s]", capitalize(cases(ch->name, 4)));
        write_to_buffer(d, buf, 0);
        d->connected = CON_GET_CASES_4;
        break;

        case CON_GET_CASES_4:
        sprintf(buf, "%s", capitalize(IS_NULLSTR(argument) ? cases(ch->name, 4) : argument));

        if (!check_case(ch, buf, 4))
            break;

        free_string(ch->pcdata->cases[3]);
        smash_tilde(buf);
        strip_colors(buf);
        ch->pcdata->cases[3] = str_dup(buf);
        sprintf(buf, "Предложный падеж (говорить о ком?): [%s]", capitalize(cases(ch->name, 5)));
        write_to_buffer(d, buf, 0);
        d->connected = CON_GET_CASES_5;
        break;

        case CON_GET_CASES_5:
        sprintf(buf, "%s", capitalize(IS_NULLSTR(argument) ? cases(ch->name, 5) : argument));

        if (!check_case(ch, buf, 5))
            break;

        free_string(ch->pcdata->cases[4]);
        smash_tilde(buf);
        strip_colors(buf);
        ch->pcdata->cases[4] = str_dup(buf);
        write_to_buffer(d, "\n\r\n\rВведите Ваш адрес электронной почты. Он нужен только для технических целей, как то: восстановление пароля, сообщение различной технической информации и т.п. Поэтому не советуем Вам сообщать ерунду 'ради галочки'. Это нужно не столько нам, сколько Вам! И еще - этот адрес не будет использован для спама!\n\rВаш E-mail:\n\r", 0);
        d->connected = CON_GET_EMAIL;
        break;

        case CON_GET_EMAIL:

        if (!IS_NULLSTR(argument) && !CHECK_EMAIL(argument))
        {
            write_to_buffer(d, "Таких адресов не бывает... Попробуй еще.\n\r", 0);
            return;
        }

        if (IS_NULLSTR(argument))
        {
            write_to_buffer(d, "Введите адрес e-mail.\n\r", 0);
            return;
        }

        free_string(ch->pcdata->email);
        ch->pcdata->email = str_dup(argument);
        d->connected = CON_GET_ATHEIST;
        write_to_buffer(d, "\n\r\n\rВы будете атеистом? Наберите ? для получения информации. Внимание: для новичков крайне рекомендуется выбрать НЕТ. [Д/Н] (по умолчанию НЕТ): \n\r", 0);
        break;

        case CON_GET_ATHEIST:
        switch(argument[0])
        {
            case 'Y': case 'y': case 'Д': case 'д':
                {
                write_to_buffer(d, "\n\r", 2);
                ch->pcdata->atheist = 1; //yes
                break;
                }
            case 'N': case 'n': case 'Н': case 'н':
            {
                break;
            }
            default:
            {
                break;
            }
            case '?':
                {
                write_to_buffer(d, "Атеисты - существа, не верящие в богов. Они живут также, как все остальные,\n\r", 0);
                write_to_buffer(d, "но Боги не даруют им ни возможности совершать возврат в свой храм, ни\n\r", 0);
                write_to_buffer(d, "жертвовать им что-либо, ни воскрешать после смерти.\n\r", 0);
                write_to_buffer(d, "Иными словами - для атеиста первая смерть - последняя.\n\r", 0);
                return;
                }

        }

        d->connected = CON_READ_MOTD;
        write_to_buffer(d, "\n\r\n\r", 4);
        do_function(ch, &do_help, "motd");
        break;

        case CON_GEN_GROUPS:
        send_to_char("\n\r", ch);

        if (!str_cmp(argument, "done") || !str_cmp(argument, "готово"))
        {
            if (ch->pcdata->points == pc_race_table[ch->race].points)
            {
            send_to_char("Вы ничего не выбрали.\n\r", ch);
            break;
            }

            if (ch->pcdata->points < 40 + pc_race_table[ch->race].points)
            {
            sprintf(buf, "Вы должны набрать минимум %d %s.\n\r",
                40 + pc_race_table[ch->race].points,
                hours(40 + pc_race_table[ch->race].points, TYPE_POINTS));
            send_to_char(buf, ch);
            break;
            }

            sprintf(buf, "Пункты генерации: %d\n\r", ch->pcdata->points);
            send_to_char(buf, ch);
            sprintf(buf, "Количество очков на уровень: %d\n\r\n\r",
                exp_per_level(ch, ch->gen_data->points_chosen));
            if (ch->pcdata->points < 40)
            ch->train = (40 - ch->pcdata->points + 1) / 2;
            free_gen_data(ch->gen_data);
            ch->gen_data = NULL;
            send_to_char(buf, ch);
            write_to_buffer(d, "\n\rВыберите оружие из списка:\n\r    ", 0);
            buf[0] = '\0';
            for (i = 0; weapon_table[i].rname != NULL; i++)
            if (weapon_table[i].can_select && ch->pcdata->learned[*weapon_table[i].gsn] > 0)
            {
                strcat(buf, weapon_table[i].rname);
                strcat(buf, " ");
            }
            strcat(buf, "\n\rВаш выбор? ");
            write_to_buffer(d, buf, 0);
            d->connected = CON_PICK_WEAPON;
            break;
        }

        if (!parse_gen_groups(ch, argument))
        {
            send_to_char("Доступные команды:\n\r"
                 "список, приобретенные, добавить, сбросить, "
                 "инфо, помощь, и готово.\n\r", ch);
        }

        write_to_buffer(d, "\n\r", 0);
        do_function(ch, &do_help, "menu choice");
        break;

        case CON_READ_IMOTD:
        write_to_buffer(d, "\n\r", 2);
        do_function(ch, &do_help, "motd");
        d->connected = CON_READ_MOTD;
        break;

        case CON_READ_MOTD:

        if (ch->pcdata == NULL || ch->pcdata->pwd[0] == '\0'){
            write_to_buffer(d, "Внимание! Пустой пароль!\n\r", 0);
            write_to_buffer(d, "Сообщите старый пароль командой 'глюк'.\n\r", 0);
            write_to_buffer(d, "Для исправления наберите 'password null <новый пароль>'.\n\r", 0);
        }

        write_to_buffer(d, "\n\r===========================================================================", 0);
        write_to_buffer(d, "\n\rДобро пожаловать в БАЛДЕРДАШ. Пожалуйста, не кормите и не пугайте мобов :-)", 0);
        write_to_buffer(d, "\n\r===========================================================================\n\r\n\r", 0);

        if (IS_IMMORTAL(ch))
            write_to_buffer(d, COMPILE_TIME, 0);

        if (reboot_time > 0 && reboot_time < 60){
            if (reboot_time == 1)
            sprintf(buf, "{gДо запланированной перезагрузки осталось менее минуты!{x\n\r\n\r");
            else
            sprintf(buf, "{gДо запланированной перезагрузки осталось %d %s.{x\n\r\n\r",
                reboot_time, hours(reboot_time, TYPE_MINUTES));
            send_to_char(buf, ch);
        }



        if (ch->level == 0){
            OBJ_INDEX_DATA *pIndex;

            LIST_INSERT_HEAD(&char_list, ch, link);
            reset_char(ch);

            ch->perm_stat[class_table[ch->classid].attr_prime] += 2;
            ch->perm_stat[class_table[ch->classid].attr_secondary] += 1;


            ch->level	= 1;
            ch->pcdata->max_level = 1;
            ch->exp	= exp_per_level(ch, ch->pcdata->points);
            ch->hit	= ch->max_hit;
            ch->mana	= ch->max_mana;
            ch->move	= ch->max_move;
            ch->pcdata->last_level	= 0;
            ch->pcdata->vnum_house = 0;

            ch->train	 = 3;
            ch->practice = 5;


            set_title(ch, title_table[ch->classid][ch->level][ch->sex == SEX_FEMALE ? 1 : 0]);
            do_function (ch, &do_outfit, "");

            if ((pIndex = get_obj_index(pc_race_table[ch->race].map_vnum)) != NULL)
            obj_to_char(create_object(pIndex, 0), ch);

            char_to_room(ch, get_room_index(ROOM_VNUM_SCHOOL), FALSE);
            send_to_char("\n\r", ch);
            do_function(ch, &do_help, "newbie info");
            send_to_char("\n\r", ch);
            SET_BIT(ch->act, PLR_AUTOTITLE);
            SET_BIT(ch->act, PLR_AUTOMONEY);
            SET_BIT(ch->comm, COMM_HELPER);
            SET_BIT(ch->act, PLR_AUTOATTACK);

            ch->pcdata->genip = str_dup(d->ip);
        } else {
            if (ch->pcdata){
                sprintf(buf, "Последний раз ты заходил%s в этот мир %sХост     %s\n\rIP адрес %s\n\r\n\r",
                        SEX_ENDING(ch),
                        ctime(&ch->pcdata->lastlogof),
                        ch->pcdata->lasthost,
                        ch->pcdata->lastip);
                write_to_buffer(d, buf, 0);
            }

            if (ch->pcdata && ch->pcdata->count_entries_errors){
                send_to_char("{RС момента твоего последнего входа в игру твоим персонажем пытались зайти,\n\r", ch);
                sprintf(buf, "{Rно неверно вводили букву регистрационного ответа, %d раз(а).{x\n\r", ch->pcdata->count_entries_errors);
                send_to_char(buf, ch);
                send_to_char("{RСРОЧНО смени пароль от персонажа. И разберись, почему твой пароль кто-то знает.{x\n\r\n\r", ch);
                ch->pcdata->count_entries_errors = 0;
                save_char_obj(ch, FALSE);
            }

            strcpy(buf, ch->name);

            extract_char_for_quit(ch, FALSE);

            if (!load_char_obj(d, buf, TRUE, TRUE)){
				close_socket(d);
				return;
            }


            ch = d->character;

            sprintf(buf, "Loading %s.", ch->name);
            log_string(buf);

            LIST_INSERT_HEAD(&char_list, ch, link);
            reset_char(ch);


            if (IS_NULLSTR(ch->pcdata->email))
            send_to_char("{RУ тебя не введен адрес электронной почты!{x\n\rСоветуем сделать это (команда 'почта сменить <твой email>'), иначе в случае утери пароля ты не сможешь его восстановить.\n\r\n\r", ch);

            if (IS_NULLSTR(ch->pcdata->reg_answer))
            send_to_char("{RУ тебя не введен ответ на регистрационный вопрос!{x\n\rОБЯЗАТЕЛЬНО сделай это (команда 'регвопрос').\n\r\n\r", ch);

            if (ch->level >= MAX_NOCLAN_LEVEL && !IS_IMMORTAL(ch) && !is_clan(ch)){
                ch->clan = clan_lookup(CLAN_INDEPEND);
                clan_table[ch->clan].count++;
                printf_to_char("Вай-вай-вай... Неужели ты все еще не в клане?\n\r"
                    "Ты присоединяешься к клану %s.\n\r",
                    ch, clan_table[ch->clan].short_descr);
            }


            if (ch->in_room != NULL){
				char_to_room(ch, ch->in_room, FALSE);
            } else if (IS_IMMORTAL(ch)) {
            	char_to_room(ch, get_room_index(ROOM_VNUM_CHAT), FALSE);
			} else {
				char_to_room(ch, get_room_index(ROOM_VNUM_TEMPLE), FALSE);
            }


            //если чар входит в мир в чужом доме, то кидаю его в реколл.
            if (ch->in_room != NULL && !is_room_owner(ch, ch->in_room)) {
				ROOM_INDEX_DATA *location;

				if ((location = get_recall(ch)) == NULL)
					send_to_char("Ты окончательно потерялся...\n\r", ch);

					char_from_room(ch);
					char_to_room(ch, location, TRUE);
            }





			if (ch->pcdata == NULL || ch->pcdata->pwd[0] == '\0'){
				write_to_buffer(d, "Внимание! Пустой пароль!\n\r", 0);
				write_to_buffer(d, "Сообщите старый пароль командой 'глюк'.\n\r", 0);
				write_to_buffer(d, "Для исправления наберите 'password null <новый пароль>'.\n\r", 0);
			}
        }




        d->connected	= CON_PLAYING;
        SET_BIT(ch->act, PLR_LOG);
        free_string(ch->pcdata->lasthost);
        free_string(ch->pcdata->lastip);

        ch->pcdata->lasthost = str_dup(d->ip);
        ch->pcdata->lastip = str_dup(d->ip);


        smash_tilde(ch->pcdata->lasthost);

        act("$n входит в этот мир.", ch, NULL, NULL, TO_ROOM);
        do_function(ch, &do_look, "auto");

        wiznet("$N входит в этот мир.", ch, NULL,	WIZ_LOGINS, WIZ_SITES, get_trust(ch));


    /* Отладка Бага с камуфляжем при входе друля */
        /*
        if (IS_AFFECTED(ch, AFF_CAMOUFLAGE)){
            bugf("Камуфляж есть");
            wiznet("$N Камуфляж есть", ch, NULL,	WIZ_LOGINS, WIZ_SITES, get_trust(ch));
            } else{
            wiznet("$N Камуфляж нет", ch, NULL,	WIZ_LOGINS, WIZ_SITES, get_trust(ch));
            bugf("Камуфляжа нет");
        }

            if (IS_IN_WILD(ch)){
            wiznet("$N Природа в комнате: есть", ch, NULL,	WIZ_LOGINS, WIZ_SITES, get_trust(ch));
            bugf("Природа есть");
        }else{
            wiznet("$N Природа в комнате: нет", ch, NULL,	WIZ_LOGINS, WIZ_SITES, get_trust(ch));
                bugf("Природы нет");
            }
    */

        if (ch->pet != NULL)
        {
            char_to_room(ch->pet, ch->in_room, TRUE);
            act("$n входит в этот мир.", ch->pet, NULL, NULL, TO_ROOM);
        }

        if (ch->mount != NULL)
        {
            char_to_room(ch->mount, ch->in_room, TRUE);
            act("$n входит в игру.", ch->mount, NULL, NULL, TO_ROOM);

            if (IS_AWAKE(ch))
            do_mount(ch, "");
        }

        sprintf(buf, "[R:%5d] %s@%s входит в игру%s.",
        		ch->in_room != NULL ? ch->in_room->vnum : 0,
        		ch->name,
        		d->ip,
        		check_ban(d->ip, BAN_NEWBIES) && ch->level < 5 ? " ({rBAN_NEWBIES ALERT!{g)" : ""
          		 );

        log_string(buf);
        convert_dollars(buf);
        wiznet(buf, NULL, NULL, WIZ_SITES, 0, get_trust(ch));

        if (ch->pcdata->bank > 10000)
        {
            sprintf(buf, "[R:%5d] %s имеет на счету %ld. (больше 10 000)", ch->in_room != NULL ? ch->in_room->vnum : 0, ch->name, ch->pcdata->bank);
            log_string(buf);
            convert_dollars(buf);
            wiznet(buf, NULL, NULL, WIZ_SITES, 0, get_trust(ch));
        }

        SLIST_FOREACH(d_old, &descriptor_list, link)
        {
            if (d_old->connected == CON_PLAYING)
            {
            wch = CH(d_old);

            if (wch && !strcmp(d->ip, d_old->ip) && d->character != wch)
            {
                sprintf(buf, "[R:%5d] %s и %s вошли в игру c одного IP-адреса %s.",
                    wch->in_room != NULL ? wch->in_room->vnum : 0,
                    wch->name, ch->name, d->ip);
                log_string(buf);
                wiznet(buf, NULL, NULL, WIZ_SITES, 0, get_trust(ch));

                if (IS_SET(wch->act, PLR_FREEZE) || IS_SET(d->character->act, PLR_FREEZE))
                continue;

                if (!strcmp(wch->pcdata->pwd, d->character->pcdata->pwd))
                {
                sprintf(buf, "[R:%5d] %s и %s имеют одинаковый пароль!",
                    wch->in_room != NULL ? wch->in_room->vnum : 0,
                    wch->name, ch->name);
                log_string(buf);
                wiznet(buf, NULL, NULL, WIZ_SITES, 0, get_trust(ch));
                }
                if (!IS_NULLSTR(wch->pcdata->email)
                && !IS_NULLSTR(d->character->pcdata->email)
                && !str_cmp(wch->pcdata->email, d->character->pcdata->email))
                {
                sprintf(buf, "[R:%5d] %s и %s имеют одинаковый e-mail!",
                    wch->in_room != NULL ? wch->in_room->vnum : 0,
                    wch->name, ch->name);
                log_string(buf);
                convert_dollars(buf);
                wiznet(buf, NULL, NULL, WIZ_SITES, 0, get_trust(ch));
                }
                if (!IS_NULLSTR(wch->pcdata->reg_answer)
                && !IS_NULLSTR(d->character->pcdata->reg_answer)
                && !str_cmp(wch->pcdata->reg_answer, d->character->pcdata->reg_answer))
                {
                sprintf(buf, "[R:%5d] %s и %s имеют одинаковый регистрационный ответ!",
                    wch->in_room != NULL ? wch->in_room->vnum : 0,
                    wch->name, ch->name);
                log_string(buf);
                convert_dollars(buf);
                wiznet(buf, NULL, NULL, WIZ_SITES, 0, get_trust(ch));
                }

            }
            }
        }

        do_function(ch, &do_unread, "entering");

        if (!IS_NULLSTR(ch->pcdata->filter[0].ch))
        {
            send_to_char("\n\r", ch);
            do_filter(ch, "");
            send_to_char("\n\r", ch);
        }

        if (check_channels(ch))
        {
            sprintf(buf, "У тебя отобрали каналы до: %s", (char *) c_time(&ch->pcdata->nochan));
            send_to_char(buf, ch);
        }

        if (IS_SET(ch->comm, COMM_NONOTES) && ch->pcdata->nonotes > 0)
        {
            sprintf(buf, "Тебе запрещено писать письма до: %s", (char *) c_time(&ch->pcdata->nonotes));
            send_to_char(buf, ch);
        }

        if (IS_SET(ch->comm, COMM_NOTITLE) && ch->pcdata->notitle > 0)
        {
            sprintf(buf, "Тебе запрещено менять титул до: %s", (char *) c_time(&ch->pcdata->notitle));
            send_to_char(buf, ch);
        }

        if (room_is_private(ch->in_room, MOUNTED(ch)))
        {
            ROOM_INDEX_DATA *to_room;
            int i;
            EXIT_DATA *pexit;

            for (i = 0; i < MAX_DIR;i++)
            if ((pexit = ch->in_room->exit[i]) != NULL
                && (to_room = pexit->u1.to_room) != NULL
                && can_see_room(ch, to_room))
            {
                act("$n уходит $t.", ch, direct(i, TRUE), NULL, TO_ROOM);
                act("$N приходит $t.", LIST_FIRST(&to_room->people), direct(i, FALSE), ch, TO_ALL);
                char_from_room(ch);
                char_to_room(ch, to_room, TRUE);
                break;
            }
        }

        if (ch->level >= MAX_NOCLAN_LEVEL && !IS_IMMORTAL(ch) && !is_clan(ch))
        {
            ch->clan = clan_lookup(CLAN_INDEPEND);
            clan_table[ch->clan].count++;
        }

        ch->pcdata->old_age = get_age(ch);
        do_count(ch, "count");
        break;
    }

    return;
}



/*
 * Parse a name for acceptability.
 */
bool check_parse_name(char *name)
{
    int clan;
    char c;

    /*
     * Length restrictions.
     */

    if (strlen(name) >  10 || strlen(name) <  3)
	return FALSE;

    /*
     * Reserved words.
     */

    if (is_exact_name(name, "все список что кто"))
	return FALSE;

    if ((c = UPPER(name[0])) == 'Ъ' || c == 'Ь' || c == 'Ы')
	return FALSE;


    /* check clans */

    for (clan = 1; clan < MAX_CLAN; clan++)
	if (clan_table[clan].valid && !str_cmp(name, clan_table[clan].name))
	    return FALSE;

    /*
     * Alpha only.
     * Lock out IllIll twits.
     */
    {
	char *pc;

	for (pc = name; *pc != '\0'; pc++)
	    if (!IS_RUSSIAN(*pc))
		return FALSE;
    }

    /*
     * Prevent players from naming themselves after mobs.
     */

    {
	extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
	MOB_INDEX_DATA *pMobIndex;
	int iHash;

	for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
	{
	    for (pMobIndex  = mob_index_hash[iHash];
		 pMobIndex != NULL;
		 pMobIndex  = pMobIndex->next)
	    {
		if (is_exact_name(name, pMobIndex->player_name))
		    return FALSE;
	    }
	}
    }

    /* Check for badnames */
    for (clan = 0; clan < MAX_BAD_NAMES && !IS_NULLSTR(badnames[clan]); clan++)
	if (!str_cmp(name, badnames[clan]))
	    return FALSE;

    return TRUE;
}


/*
 * Look for link-dead player to reconnect.
 */
bool check_reconnect(DESCRIPTOR_DATA *d, char *name, bool fConn)
{
    CHAR_DATA *ch;

    LIST_FOREACH(ch, &char_list, link)
    {
	if (!IS_NPC(ch)
	    && (!fConn || ch->desc == NULL)
	    && !str_cmp(d->character->name, ch->name))
	{
	    if (fConn == FALSE)
	    {
		free_string(d->character->pcdata->pwd);
		d->character->pcdata->pwd = str_dup(ch->pcdata->pwd);
	    }
	    else
	    {
		char bfr[MAX_INPUT_LENGTH];

		stop_idling(d->character);
		free_char(d->character);

		d->character = ch;
		ch->desc	 = d;
		ch->timer	 = 0;

		send_to_char("Переподключение. Наберите 'воспроизвести' для просмотра пропущенных сообщений.\n\r", ch);
		if (!IS_IMMORTAL(ch))
		{
			act("$n возвращается в сознание.", ch, NULL, NULL, TO_ROOM);
		}

		sprintf(bfr, "%s@%s reconnected.", ch->name, d->ip);
		log_string(bfr);

		sprintf(bfr, "$N@%s восстанавливает связь.", d->ip);
		wiznet(bfr, ch, NULL, WIZ_LINKS, 0, get_trust(ch));
		d->connected = CON_PLAYING;
	    }

	    return TRUE;
	}
    }

    return FALSE;
}



/*
 * Check if already playing.
 */
bool check_playing(DESCRIPTOR_DATA *d, char *name)
{
    DESCRIPTOR_DATA *dold;

    SLIST_FOREACH(dold, &descriptor_list, link)
    {
	if (dold != d
	    && dold->character != NULL
	    && dold->connected != CON_GET_NAME
	    && dold->connected != CON_GET_OLD_PASSWORD
	    && !str_cmp(name, CH(dold)->name))
	{
	    write_to_buffer(d, "Этот персонаж уже в игре.\n\r", 0);
	    write_to_buffer(d, "Установить связь (Да/Нет)?", 0);
	    d->connected = CON_BREAK_CONNECT;
	    return TRUE;
	}
    }

    return FALSE;
}



void stop_idling(CHAR_DATA *ch)
{
    if (ch == NULL
	|| ch->desc == NULL
	|| ch->desc->connected != CON_PLAYING
	|| ch->was_in_room == NULL
	|| ch->in_room != get_room_index(ROOM_VNUM_LIMBO))
    {
	return;
    }

    ch->timer = 0;
    char_from_room(ch);
    char_to_room(ch, ch->was_in_room, FALSE);
    ch->was_in_room	= NULL;
    act("$n возвращается из ниоткуда.", ch, NULL, NULL, TO_ROOM);
    do_function(ch, &do_look, "auto");
    return;
}

bool Show_output = TRUE;
bool Show_output_old = TRUE;
bool Show_result = TRUE;
/*
 * Write to one char, new colour version, by Lope.
 */
void send_to_char(const char *txt, CHAR_DATA *ch)
{
    const char *point;
    char *point2;
    char buf[8 * OUTBUF_SIZE];
    int skip = 0;

    if (!ch || !Show_output)
	return;

    buf[0] = '\0';
    point2 = buf;

    if (txt && ch->desc)
    {
	if (IS_SET(ch->act, PLR_COLOUR))
	{
	    for (point = txt ; *point; point++)
	    {
		if (*point == '{')
		{
		    point++;
		    skip = colour(*point, ch, point2);
		    while (skip-- > 0)
			++point2;
		    continue;
		}
		*point2 = *point;
		*++point2 = '\0';
	    }
	    *point2 = '\0';

	    write_to_buffer(ch->desc, buf, point2 - buf);
	}
	else
	{
	    for (point = txt ; *point; point++)
	    {
		if (*point == '{')
		{
		    point++;
		    continue;
		}
		*point2 = *point;
		*++point2 = '\0';
	    }
	    *point2 = '\0';

	    write_to_buffer(ch->desc, buf, point2 - buf);
	}
    }
    return;
}

/*
 * Write to one char, printf(3)-alike version
 */
void printf_to_char(const char *fmt, CHAR_DATA *ch, ...)
{
    char buf[MAX_STRING_LENGTH];
    va_list va;

    if (!Show_output)
	return;

    va_start(va, ch);
    vsnprintf(buf, sizeof(buf), fmt, va);
    va_end(va);

    send_to_char(buf, ch);
}

void printf_to_buffer(BUFFER *buf, const char *fmt, ...)
{
    char tmp[MAX_STRING_LENGTH];
    va_list va;

    va_start(va, fmt);
    vsnprintf(tmp, sizeof(tmp), fmt, va);
    va_end(va);

    add_buf(buf, tmp);
}

/*
 * Page to one char, new colour version, by Lope.
 */
void page_to_char(const char *txt, CHAR_DATA *ch)
{
    const char *point;
    char *point2;
    char buf[8 * OUTBUF_SIZE];
    int skip = 0;

    if (!Show_output)
	return;

    buf[0] = '\0';
    point2 = buf;
    if(txt && ch->desc)
    {
	if(IS_SET(ch->act, PLR_COLOUR))
	{
	    for(point = txt ; *point ; point++)
	    {
		if(*point == '{')
		{
		    point++;
		    skip = colour(*point, ch, point2);

		    while(skip-- > 0)
			++point2;

		    continue;
		}
		*point2 = *point;
		*++point2 = '\0';

	    }
	    *point2 = '\0';

	    if (ch->desc->showstr_head != NULL)
		free_mem(ch->desc->showstr_head, strlen(ch->desc->showstr_head + 1));

	    ch->desc->showstr_head  = alloc_mem(strlen(buf) + 1);
	    strcpy(ch->desc->showstr_head, buf);
	    ch->desc->showstr_point = ch->desc->showstr_head;
	    show_string(ch->desc, "");
	}
	else
	{
	    for(point = txt ; *point ; point++)
	    {
		if(*point == '{')
		{
		    point++;
		    continue;
		}
		*point2 = *point;
		*++point2 = '\0';
	    }
	    *point2 = '\0';

	    if (ch->desc->showstr_head != NULL)
		free_mem(ch->desc->showstr_head, strlen(ch->desc->showstr_head + 1));

	    ch->desc->showstr_head  = alloc_mem(strlen(buf) + 1);
	    strcpy(ch->desc->showstr_head, buf);
	    ch->desc->showstr_point = ch->desc->showstr_head;
	    show_string(ch->desc, "");
	}
    }
    return;
}


/* string pager */
void show_string(DESCRIPTOR_DATA *d, char *input)
{
    char buffer[OUTBUF_SIZE];
    char buf[MAX_INPUT_LENGTH];
    register char *scan, *chk;
    int lines = 0, toggle = 1;
    int show_lines;
    bool stop = FALSE;

    if (!Show_output)
	return;

    one_argument(input, buf);
    if (buf[0] != '\0')
    {
	if (d->showstr_head)
	{
	    free_mem(d->showstr_head, strlen(d->showstr_head) + 1);
	    d->showstr_head = 0;
	}
	d->showstr_point  = 0;
	return;
    }

    if (d->character)
	show_lines = d->character->lines;
    else
	show_lines = 0;

    for (scan = buffer; ; scan++, d->showstr_point++)
    {
	if (scan - buffer >= OUTBUF_SIZE - 1)
	{
	    write_to_descriptor(d, "Слишком большой объем информации!\n\r", 0);
	    stop = TRUE;
	}

	if (!stop && ((*scan = *d->showstr_point) == '\n' || *scan == '\r')
	    && (toggle = -toggle) < 0)
	    lines++;
	else if (stop || !*scan || (show_lines > 0 && lines >= show_lines))
	{
	    *scan = '\0';

	    write_to_buffer(d, buffer, 0);

	    for (chk = d->showstr_point; IS_SPACE(*chk); chk++);
	    {
		if (!*chk)
		{
		    if (d->showstr_head)
		    {
			free_mem(d->showstr_head, strlen(d->showstr_head) + 1);
			d->showstr_head = 0;
		    }
		    d->showstr_point  = 0;
		}
	    }
	    return;
	}
    }
    return;
}


/* quick sex fixer */
void fix_sex(CHAR_DATA *ch)
{
    if (ch->sex < 0 || ch->sex > GET_MAX_SEX(ch))
	ch->sex = IS_NPC(ch) ? SEX_NEUTRAL : ch->pcdata->true_sex;
}

char *strip_$(char *s)
{
    for(; *s != '\0'; s++)
	if (*s == '$')
	    *s = 'S';

    return s;
}

void act_new(const char *format, CHAR_DATA *ch, void *arg1,
	     void *arg2, int type, int min_pos)
{
    CHAR_DATA *to, *to_next;
    CHAR_DATA *vch = (CHAR_DATA *) arg2;
    OBJ_DATA *obj1 = (OBJ_DATA  *) arg1;
    OBJ_DATA *obj2 = (OBJ_DATA  *) arg2;
    void *arg2_new;
    const char *str;
    int cs, cl;
    int delta = 1;
    char *i = NULL;
    char *point;
    char *pbuff;
    char buffer[MAX_STRING_LENGTH * 2];
    char buf[MAX_STRING_LENGTH];
    bool fColour = FALSE;


    /*
     * Discard null and zero-length messages.
     */
    if (!format || !*format)
	return;

    /* discard null rooms and chars */
    if (!ch || !ch->in_room)
	return;

    to = LIST_FIRST(&ch->in_room->people);
    if (type == TO_VICT)
    {
	if (!vch)
	{
	    bugf("Act: null vch with TO_VICT (%s).", format);
	    return;
	}

	if (!vch->in_room)
	    return;

	to = LIST_FIRST(&vch->in_room->people);
    }

    for (; to; to = to_next)
    {
	/*
	 * MOBProgram fix to allow MOBs to see acts()
	 *   -- Thunder (thunder1@fast.net)
	 */
	to_next = LIST_NEXT(to, room_link);

	if ((to->desc == NULL
	     && (!IS_NPC(to) || !has_trigger(to->pIndexData->progs, TRIG_ACT, EXEC_ALL)))
	    || to->position < min_pos)
	{
	    continue;
	}

	if ((type == TO_CHAR) && to != ch)
	    continue;

	if (type == TO_VICT && (to != vch || to == ch))
	    continue;

	if (type == TO_ROOM && to == ch)
	    continue;

	if (type == TO_SOUND && to == ch)
	    continue;

	if (type == TO_NOTVICT && (to == ch || to == vch))
	    continue;

	if ((type == TO_ROOM || type == TO_NOTVICT) && !can_see(to, ch))
	{
	    if (to->desc != NULL)
		write_to_buffer(to->desc, "Ты слышишь непонятные звуки и ощущаешь, что рядом происходит что-то неизвестное...\n\r", 0);
	    continue;
	}

	point   = buf;
	str = format;

	if (is_lycanthrope(ch)
	    && !is_lycanthrope(to)
	    && !IS_IMMORTAL(to)
	    && type == TO_SOUND)
	    arg2_new = makehowl(arg2,ch);
	else
	    arg2_new = arg2;

	while (*str != '\0')
	{
	    char bfr[MSL];

	    if (*str != '$' || (str != format && *(str - 1) == '{'))
	    {
		*point++ = *str++;
		continue;
	    }

	    bfr[0] = '\0';

	    fColour = TRUE;
	    ++str;
	    i = " <@@@> ";

	    if (!arg2_new && *str >= 'A' && *str <= 'Z' && *str != 'X')
		bugf("Act: missing arg2 for code %c (%s).", *str, format);
	    else
	    {
		cs = *(str+1)-'0';
		if (cs < 0 || cs > 6)
		{
		    cs = 0;
		    delta = 1;
		}
		else
		    delta = 2;

		switch (*str)
		{
		default:
		    strcpy(bfr, format);
		    bugf("Act: bad code %c (%s).", *str, strip_$(bfr));
		    i = " <@@@> ";
		    break;

		    /* Thx alex for 't' idea */
		case 't':
		    i = arg1 != NULL ? cases((char *) arg1, cs) : "";
		    break;

		case 'T':
		    i = arg2_new != NULL ? cases((char *) arg2_new, cs) : "";
		    break;

		case 'n':
		    i = ch  && to ? PERS(ch,  to, cs) : "";
		    break;

		case 'N':
		    i = vch && to ? PERS(vch, to, cs) : "";
		    break;

		case 'e':
		    if (ch)
		    {
			if (can_see(to, ch))
			{
			    if (cs == 1)
				strcat(bfr, "н");

			    strcat(bfr, he_she[URANGE(0, ch->sex, GET_MAX_SEX(ch))]);
			}
			else
			    strcpy(bfr, "кто-то");
		    }

		    i = bfr;
		    break;

		case 'E':
		    if (vch)
		    {
			if (can_see(to, vch))
			{
			    if (cs == 1)
				strcat(bfr, "н");

			    strcat(bfr, he_she[URANGE(0, vch->sex, GET_MAX_SEX(ch))]);
			}
			else
			    strcpy(bfr, "кто-то");
		    }

		    i = bfr;
		    break;

		case 'm':
		    if (ch)
		    {
			if (can_see(to, ch))
			{
			    if (cs == 1)
				strcat(bfr, "н");

			    strcat(bfr, him_her[URANGE(0, ch->sex, GET_MAX_SEX(ch))]);
			}
			else
			    strcpy(bfr, "кому-то");
		    }

		    i = bfr;
		    break;

		case 'M':
		    if (vch)
		    {
			if (can_see(to, vch))
			{
			    if (cs == 1)
				strcat(bfr, "н");

			    strcat(bfr, him_her[URANGE(0, vch->sex, GET_MAX_SEX(ch))]);
			}
			else
			{
			    strcpy(bfr, "кому-то");
			}
		    }

		    i = bfr;
		    break;

		case 's':
		    if (ch)
		    {
			if (can_see(to, ch))
			{
			    if (cs == 1)
				strcat(bfr, "н");

			    strcat(bfr, his_her[URANGE(0, ch->sex, GET_MAX_SEX(ch))]);
			}
			else
			{
			    switch(cs)
			    {
			    case  2:
				strcpy(bfr, "чьи-то");
				break;
			    case  3:
				strcpy(bfr, "чьей-то");
				break;
			    default:
				strcpy(bfr, "кого-то");
				break;
			    }
			}
		    }

		    i = bfr;
		    break;

		case 'S':
		    if (vch)
		    {
			if (can_see(to, vch))
			{
			    if (cs == 1)
				strcat(bfr, "н");

			    strcat(bfr, his_her[URANGE(0, vch->sex, GET_MAX_SEX(ch))]);
			}
			else
			{
			    switch(cs)
			    {
			    case  2:
				strcpy(bfr, "чьи-то");
				break;
			    case  3:
				strcpy(bfr, "чьей-то");
				break;
			    default:
				strcpy(bfr, "кого-то");
				break;
			    }
			}
		    }

		    i = bfr;
		    break;

		case 'p':
		    i = obj1 != NULL ? can_see_obj(to, obj1)
			? cases(obj1->short_descr, cs)
			: cases("что-то", cs) : "";
		    break;

		case 'P':
		    i = obj2 != NULL ? can_see_obj(to, obj2)
			? cases(obj2->short_descr, cs)
			: cases("что-то", cs) : "";
		    break;

		case 'd':
		    i = cases(IS_NULLSTR((char *) arg2_new) ? "дверь" : (char *) arg2_new, cs);
		    break;

		case 'r':
		    i = obj1 != NULL ?
			decompose_end(can_see_obj(to, obj1)
				      ? obj1->short_descr : "что-то") : "";
		    break;

		case 'R':
		    i = obj2 != NULL ?
			decompose_end(can_see_obj(to, obj2)
				      ? obj2->short_descr : "что-то") : "";
		    break;

		case 'x':

		    switch (ch->in_room->sector_type)
		    {
		    case SECT_FIELD:
			i = "на траву";
			break;

		    case SECT_MOUNTAIN:
			i = "на камни";
			break;
		    case SECT_WATER_SWIM:
		    case SECT_WATER_NOSWIM:
			i = "в воду";
			break;

		    case SECT_AIR:
			i = "в воздух";
			break;

		    default:
			i = "на землю";
			break;
		    }
		    break;

		case 'X':

		    switch (ch->in_room->sector_type)
		    {
		    case SECT_FIELD:
			i = "над травой";
			break;

		    case SECT_MOUNTAIN:
			i = "над камнями";
			break;
		    case SECT_WATER_SWIM:
		    case SECT_WATER_NOSWIM:
			i = "над водой";
			break;

		    case SECT_AIR:
			i = "в воздухе";
			break;

		    default:
			i = "над землей";
			break;
		    }

		    break;
		}
	    }

	    str+=delta;

	    if (i != NULL)
	    {
		while ((*point = *i) != '\0')
		{
		    ++point;
		    ++i;
		}
	    }
	}

	*point++ = '\n';
	*point++ = '\r';
	*point   = '\0';

	cl = 0;
	while(buf[cl] == '{')
	{
	    cl++;

	    if (buf[cl] == '\0')
		break;

	    cl++;
	}

	buf[cl]   = UPPER(buf[cl]);

	pbuff    = buffer;
	colourconv(pbuff, buf, to);

	if (to->desc != NULL)
	    write_to_buffer(to->desc, buffer, 0);
	else if (MOBtrigger)
	{
	    p_act_trigger(buf, to, NULL, NULL, ch, arg1, arg2_new, TRIG_ACT);
	}
    }

    if (!ch || !ch->in_room)
	return;

    if (type == TO_ROOM || type == TO_NOTVICT || type == TO_SOUND)
    {
	OBJ_DATA *obj, *obj_next;
	CHAR_DATA *tch, *tch_next;
	point   = buf;
	str     = format;

	while (*str != '\0')
	{
	    *point++ = *str++;
	}

	*point   = '\0';

	for (obj = ch->in_room->contents; obj; obj = obj_next)
	{
	    obj_next = obj->next_content;

	    if (has_trigger(obj->pIndexData->progs, TRIG_ACT, EXEC_ALL))
		p_act_trigger(buf, NULL, obj, NULL, ch, NULL, NULL, TRIG_ACT);
	}

	for (tch = ch; tch; tch = tch_next)
	{
	    tch_next = LIST_NEXT(tch, room_link);

	    for (obj = tch->carrying; obj; obj = obj_next)
	    {
		obj_next = obj->next_content;
		if (has_trigger(obj->pIndexData->progs, TRIG_ACT, EXEC_ALL))
		    p_act_trigger(buf, NULL, obj, NULL,
				  ch, NULL, NULL, TRIG_ACT);
	    }
	}

	if (has_trigger(ch->in_room->progs, TRIG_ACT, EXEC_ALL))
	    p_act_trigger(buf, NULL, NULL, ch->in_room,
			  ch, NULL, NULL, TRIG_ACT);
    }

    return;
}

int colour(char type, CHAR_DATA *ch, char *string)
{
    PC_DATA *col;
    char code[20];
    char *p = '\0';

    if (IS_NPC(ch))
	return 0;

    col = ch->pcdata;

    switch (type)
    {
    default:
	strcpy(code, CLEAR);
	break;
    case 'x':
	strcpy(code, CLEAR);
	break; 
    case 'p':
	if (col->prompt[2])
	    sprintf(code, "\e[%d;3%dm%c", col->prompt[0], col->prompt[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->prompt[0], col->prompt[1]);
	break;
    case 's':
	if (col->room_title[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->room_title[0], col->room_title[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->room_title[0], col->room_title[1]);
	break;
    case 'S':
	if (col->room_text[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->room_text[0], col->room_text[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->room_text[0], col->room_text[1]);
	break;
    case 'd':
	if (col->gossip[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->gossip[0], col->gossip[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->gossip[0], col->gossip[1]);
	break;
    case '9':
	if (col->gossip_text[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->gossip_text[0], col->gossip_text[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm",
		    col->gossip_text[0], col->gossip_text[1]);
	break;
    case 'Z':
	if (col->wiznet[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->wiznet[0], col->wiznet[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->wiznet[0], col->wiznet[1]);
	break;
    case 'o':
	if (col->room_exits[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->room_exits[0], col->room_exits[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->room_exits[0], col->room_exits[1]);
	break;
    case 'O':
	if (col->room_things[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->room_things[0], col->room_things[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm",
		    col->room_things[0], col->room_things[1]);
	break;
    case 'i':
	if (col->immtalk_text[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->immtalk_text[0], col->immtalk_text[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm",
		    col->immtalk_text[0], col->immtalk_text[1]);
	break;
    case 'I':
	if (col->immtalk_type[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->immtalk_type[0], col->immtalk_type[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm",
		    col->immtalk_type[0], col->immtalk_type[1]);
	break;
    case '2':
	if (col->fight_yhit[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->fight_yhit[0], col->fight_yhit[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->fight_yhit[0], col->fight_yhit[1]);
	break;
    case '3':
	if (col->fight_ohit[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->fight_ohit[0], col->fight_ohit[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->fight_ohit[0], col->fight_ohit[1]);
	break;
    case '4':
	if (col->fight_thit[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->fight_thit[0], col->fight_thit[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->fight_thit[0], col->fight_thit[1]);
	break;
    case '5':
	if (col->fight_skill[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->fight_skill[0], col->fight_skill[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm",
		    col->fight_skill[0], col->fight_skill[1]);
	break;
    case '1':
	if (col->fight_death[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->fight_death[0], col->fight_death[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm",
		    col->fight_death[0], col->fight_death[1]);
	break;
    case '6':
	if (col->say[2])
	    sprintf(code, "\e[%d;3%dm%c", col->say[0], col->say[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->say[0], col->say[1]);
	break;
    case '7':
	if (col->say_text[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->say_text[0], col->say_text[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->say_text[0], col->say_text[1]);
	break;
    case 'k':
	if (col->tell[2])
	    sprintf(code, "\e[%d;3%dm%c", col->tell[0], col->tell[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->tell[0], col->tell[1]);
	break;
    case 'K':
	if (col->tell_text[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->tell_text[0], col->tell_text[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->tell_text[0], col->tell_text[1]);
	break;
    case 'l':
	if (col->reply[2])
	    sprintf(code, "\e[%d;3%dm%c", col->reply[0], col->reply[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->reply[0], col->reply[1]);
	break;
    case 'L':
	if (col->reply_text[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->reply_text[0], col->reply_text[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->reply_text[0], col->reply_text[1]);
	break;
    case 'n':
	if (col->gtell_text[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->gtell_text[0], col->gtell_text[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->gtell_text[0], col->gtell_text[1]);
	break;
    case 'N':
	if (col->gtell_type[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->gtell_type[0], col->gtell_type[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->gtell_type[0], col->gtell_type[1]);
	break;
    case 'a':
	if (col->auction[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->auction[0], col->auction[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->auction[0], col->auction[1]);
	break;
    case 'A':
	if (col->auction_text[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->auction_text[0], col->auction_text[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm",
		    col->auction_text[0], col->auction_text[1]);
	break;
    case 'q':
	if (col->question[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->question[0], col->question[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->question[0], col->question[1]);
	break;
    case 'Q':
	if (col->question_text[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->question_text[0], col->question_text[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm",
		    col->question_text[0], col->question_text[1]);
	break;
    case 'f':
	if (col->answer[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->answer[0], col->answer[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->answer[0], col->answer[1]);
	break;
    case 'F':
	if (col->answer_text[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->answer_text[0], col->answer_text[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm",
		    col->answer_text[0], col->answer_text[1]);
	break;
    case 'e':
	if (col->music[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->music[0], col->music[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->music[0], col->music[1]);
	break;
    case 'E':
	if (col->music_text[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->music_text[0], col->music_text[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->music_text[0], col->music_text[1]);
	break;
    case 'h':
	if (col->quote[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->quote[0], col->quote[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->quote[0], col->quote[1]);
	break;
    case 'H':
	if (col->quote_text[2])
	    sprintf(code, "\e[%d;3%dm%c",
		    col->quote_text[0], col->quote_text[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->quote_text[0], col->quote_text[1]);
	break;
    case 'j':
	if (col->info[2])
	    sprintf(code, "\e[%d;3%dm%c", col->info[0], col->info[1], '\a');
	else
	    sprintf(code, "\e[%d;3%dm", col->info[0], col->info[1]);
	break;
    case 'b':
	strcpy(code, C_BLUE);
	break;
    case 'c':
	strcpy(code, C_CYAN);
	break;
    case 'g':
	strcpy(code, C_GREEN);
	break;
    case 'm':
	strcpy(code, C_MAGENTA);
	break;
    case 'r':
	strcpy(code, C_RED);
	break;
    case 'w':
	strcpy(code, C_WHITE);
	break;
    case 'y':
	strcpy(code, C_YELLOW);
	break;
    case 'B':
	strcpy(code, C_B_BLUE);
	break;
    case 'C':
	strcpy(code, C_B_CYAN);
	break;
    case 'G':
	strcpy(code, C_B_GREEN);
	break;
    case 'M':
	strcpy(code, C_B_MAGENTA);
	break;
    case 'R':
	strcpy(code, C_B_RED);
	break;
    case 'W':
	strcpy(code, C_B_WHITE);
	break;
    case 'Y':
	strcpy(code, C_B_YELLOW);
	break;
    case 'D':
	strcpy(code, C_D_GREY);
	break;
    case '*':
	sprintf(code, "%c", '\a');
	break;
    case '/':
    case '%':
    case '&':
    case ':':
    case '$':
	strcpy(code, "");
	break;
    case '-':
	sprintf(code, "%c", '~');
	break;
    case '{':
	sprintf(code, "%c", '{');
	break;
    }

    p = code;
    while (*p != '\0')
    {
	*string = *p++;
	*++string = '\0';
    } 

    return strlen(code);
}

#if 0
// New colour version
int colour(char type, CHAR_DATA *ch, char *string)
{
    PC_DATA *col;
    char code[20];
    char *p = '\0';
    char l;

    if (IS_NPC(ch))
	return 0;

    col = ch->pcdata;

    switch (type)
    {
    default:
	strcpy(code, CLEAR);
	break;
    case 'x':
	strcpy(code, CLEAR);
	break;
    case '<':
	l = ch->desc->last_color;
	ch->desc->last_color = 'x';
	return colour(l, ch, string);
    case '9':
	if (col->codes[COLOUR_GOSSIP].code)
	    strcpy(code, col->codes[COLOUR_GOSSIP].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_GOSSIP].code);

	ch->desc->last_color = type;
	break;
    case 'd':
	if (col->codes[COLOUR_GOSSIP_NAME].code)
	    strcpy(code, col->codes[COLOUR_GOSSIP_NAME].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_GOSSIP_NAME].code);

	ch->desc->last_color = type;
	break;
    case 'h':
	if (col->codes[COLOUR_SHOUT].code)
	    strcpy(code, col->codes[COLOUR_SHOUT].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_SHOUT].code);

	ch->desc->last_color = type;
	break;
    case 'H':
	if (col->codes[COLOUR_SHOUT_NAME].code)
	    strcpy(code, col->codes[COLOUR_SHOUT_NAME].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_SHOUT_NAME].code);

	ch->desc->last_color = type;
	break;
    case '8':
	if (col->codes[COLOUR_YELL].code)
	    strcpy(code, col->codes[COLOUR_YELL].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_YELL].code);

	ch->desc->last_color = type;
	break;
    case '!':
	if (col->codes[COLOUR_YELL_NAME].code)
	    strcpy(code, col->codes[COLOUR_YELL_NAME].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_YELL_NAME].code);

	ch->desc->last_color = type;
	break;
    case 'k':
	if (col->codes[COLOUR_TELL].code)
	    strcpy(code, col->codes[COLOUR_TELL].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_TELL].code);

	ch->desc->last_color = type;
	break;
    case 'K':
	if (col->codes[COLOUR_TELL_NAME].code)
	    strcpy(code, col->codes[COLOUR_TELL_NAME].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_TELL_NAME].code);

	ch->desc->last_color = type;
	break;
    case 'l':
	if (col->codes[COLOUR_ANSWER].code)
	    strcpy(code, col->codes[COLOUR_ANSWER].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_ANSWER].code);

	ch->desc->last_color = type;
	break;
    case 'L':
	if (col->codes[COLOUR_ANSWER_NAME].code)
	    strcpy(code, col->codes[COLOUR_ANSWER_NAME].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_ANSWER_NAME].code);

	ch->desc->last_color = type;
	break;
    case '6':
	if (col->codes[COLOUR_SAY].code)
	    strcpy(code, col->codes[COLOUR_SAY].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_SAY].code);

	ch->desc->last_color = type;
	break;
    case '7':
	if (col->codes[COLOUR_SAY_NAME].code)
	    strcpy(code, col->codes[COLOUR_SAY_NAME].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_SAY_NAME].code);

	ch->desc->last_color = type;
	break;
    case 'A':
	if (col->codes[COLOUR_EMOTE].code)
	    strcpy(code, col->codes[COLOUR_EMOTE].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_EMOTE].code);

	ch->desc->last_color = type;
	break;
    case 'q':
	if (col->codes[COLOUR_OOC].code)
	    strcpy(code, col->codes[COLOUR_OOC].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_OOC].code);

	ch->desc->last_color = type;
	break;
    case 'Q':
	if (col->codes[COLOUR_OOC_NAME].code)
	    strcpy(code, col->codes[COLOUR_OOC_NAME].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_OOC_NAME].code);

	ch->desc->last_color = type;
	break;
    case 'f':
	if (col->codes[COLOUR_NEWBIES].code)
	    strcpy(code, col->codes[COLOUR_NEWBIES].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_NEWBIES].code);

	ch->desc->last_color = type;
	break;
    case 'F':
	if (col->codes[COLOUR_NEWBIES_NAME].code)
	    strcpy(code, col->codes[COLOUR_NEWBIES_NAME].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_NEWBIES_NAME].code);

	ch->desc->last_color = type;
	break;
    case 'Z':
	if (col->codes[COLOUR_WIZNET].code)
	    strcpy(code, col->codes[COLOUR_WIZNET].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_WIZNET].code);

	ch->desc->last_color = type;
	break;
    case 'i':
	if (col->codes[COLOUR_IMMTALK].code)
	    strcpy(code, col->codes[COLOUR_IMMTALK].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_IMMTALK].code);

	ch->desc->last_color = type;
	break;
    case 'I':
	if (col->codes[COLOUR_IMMTALK_NAME].code)
	    strcpy(code, col->codes[COLOUR_IMMTALK_NAME].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_IMMTALK_NAME].code);

	ch->desc->last_color = type;
	break;
    case 'j':
	if (col->codes[COLOUR_BTALK].code)
	    strcpy(code, col->codes[COLOUR_BTALK].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_BTALK].code);

	ch->desc->last_color = type;
	break;
    case 'J':
	if (col->codes[COLOUR_BTALK_NAME].code)
	    strcpy(code, col->codes[COLOUR_BTALK_NAME].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_BTALK_NAME].code);

	ch->desc->last_color = type;
	break;
    case 'p':
	if (col->codes[COLOUR_CLANTALK].code)
	    strcpy(code, col->codes[COLOUR_CLANTALK].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_CLANTALK].code);

	ch->desc->last_color = type;
	break;
    case 'P':
	if (col->codes[COLOUR_CLANTALK_NAME].code)
	    strcpy(code, col->codes[COLOUR_CLANTALK_NAME].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_CLANTALK_NAME].code);

	ch->desc->last_color = type;
	break;
    case 'n':
	if (col->codes[COLOUR_GROUPTALK].code)
	    strcpy(code, col->codes[COLOUR_GROUPTALK].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_GROUPTALK].code);

	ch->desc->last_color = type;
	break;
    case 'N':
	if (col->codes[COLOUR_GROUPTALK_NAME].code)
	    strcpy(code, col->codes[COLOUR_GROUPTALK_NAME].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_GROUPTALK_NAME].code);

	ch->desc->last_color = type;
	break;
    case 'a':
	if (col->codes[COLOUR_AUCTION].code)
	    strcpy(code, col->codes[COLOUR_AUCTION].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_AUCTION].code);

	ch->desc->last_color = type;
	break;
    case 'e':
	if (col->codes[COLOUR_MUSIC].code)
	    strcpy(code, col->codes[COLOUR_MUSIC].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_MUSIC].code);

	ch->desc->last_color = type;
	break;
    case 'E':
	if (col->codes[COLOUR_MUSIC_NAME].code)
	    strcpy(code, col->codes[COLOUR_MUSIC_NAME].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_MUSIC_NAME].code);

	ch->desc->last_color = type;
	break;
    case '@':
	if (col->codes[COLOUR_GRATS].code)
	    strcpy(code, col->codes[COLOUR_GRATS].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_GRATS].code);

	ch->desc->last_color = type;
	break;
    case 's':
	if (col->codes[COLOUR_ROOMNAMES].code)
	    strcpy(code, col->codes[COLOUR_ROOMNAMES].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_ROOMNAMES].code);

	ch->desc->last_color = type;
	break;
    case 'S':
	if (col->codes[COLOUR_ROOMDESC].code)
	    strcpy(code, col->codes[COLOUR_ROOMDESC].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_ROOMDESC].code);

	ch->desc->last_color = type;
	break;
    case 't':
	if (col->codes[COLOUR_MOBS].code)
	    strcpy(code, col->codes[COLOUR_MOBS].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_MOBS].code);

	ch->desc->last_color = type;
	break;
    case 'u':
	if (col->codes[COLOUR_CHARS].code)
	    strcpy(code, col->codes[COLOUR_CHARS].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_CHARS].code);

	ch->desc->last_color = type;
	break;
    case 'U':
	if (col->codes[COLOUR_GROUPCHARS].code)
	    strcpy(code, col->codes[COLOUR_GROUPCHARS].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_GROUPCHARS].code);

	ch->desc->last_color = type;
	break;
    case 'O':
	if (col->codes[COLOUR_OBJS].code)
	    strcpy(code, col->codes[COLOUR_OBJS].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_OBJS].code);

	ch->desc->last_color = type;
	break;
    case 'o':
	if (col->codes[COLOUR_EXITS].code)
	    strcpy(code, col->codes[COLOUR_EXITS].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_EXITS].code);

	ch->desc->last_color = type;
	break;
    case 'v':
	if (col->codes[COLOUR_AFFON].code)
	    strcpy(code, col->codes[COLOUR_AFFON].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_AFFON].code);

	ch->desc->last_color = type;
	break;
    case 'V':
	if (col->codes[COLOUR_AFFOFF].code)
	    strcpy(code, col->codes[COLOUR_AFFOFF].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_AFFOFF].code);

	ch->desc->last_color = type;
	break;
    case 'z':
	if (col->codes[COLOUR_CAST].code)
	    strcpy(code, col->codes[COLOUR_CAST].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_CAST].code);

	ch->desc->last_color = type;
	break;
    case 'T':
	if (col->codes[COLOUR_CASTNAME].code)
	    strcpy(code, col->codes[COLOUR_CASTNAME].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_CASTNAME].code);

	ch->desc->last_color = type;
	break;
    case '#':
	if (col->codes[COLOUR_AFFECT].code)
	    strcpy(code, col->codes[COLOUR_AFFECT].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_AFFECT].code);

	ch->desc->last_color = type;
	break;
    case '0':
	if (col->codes[COLOUR_YOURHIT].code)
	    strcpy(code, col->codes[COLOUR_YOURHIT].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_YOURHIT].code);

	ch->desc->last_color = type;
	break;
    case '2':
	if (col->codes[COLOUR_ENEMYHIT].code)
	    strcpy(code, col->codes[COLOUR_ENEMYHIT].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_ENEMYHIT].code);

	ch->desc->last_color = type;
	break;
    case '3':
	if (col->codes[COLOUR_OTHERHIT].code)
	    strcpy(code, col->codes[COLOUR_OTHERHIT].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_OTHERHIT].code);

	ch->desc->last_color = type;
	break;
    case '5':
	if (col->codes[COLOUR_SKILL].code)
	    strcpy(code, col->codes[COLOUR_SKILL].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_SKILL].code);

	ch->desc->last_color = type;
	break;
    case '1':
	if (col->codes[COLOUR_DEATH].code)
	    strcpy(code, col->codes[COLOUR_DEATH].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_DEATH].code);

	ch->desc->last_color = type;
	break;
    case '4':
	if (col->codes[COLOUR_CRIT].code)
	    strcpy(code, col->codes[COLOUR_CRIT].code);
	else
	    strcpy(code, colour_schema_default->colours[COLOUR_CRIT].code);

	ch->desc->last_color = type;
	break;
    case 'b':
	strcpy(code, C_BLUE);
	break;
    case 'c':
	strcpy(code, C_CYAN);
	break;
    case 'g':
	strcpy(code, C_GREEN);
	break;
    case 'm':
	strcpy(code, C_MAGENTA);
	break;
    case 'r':
	strcpy(code, C_RED);
	break;
    case 'w':
	strcpy(code, C_WHITE);
	break;
    case 'y':
	strcpy(code, C_YELLOW);
	break;
    case 'B':
	strcpy(code, C_B_BLUE);
	break;
    case 'C':
	strcpy(code, C_B_CYAN);
	break;
    case 'G':
	strcpy(code, C_B_GREEN);
	break;
    case 'M':
	strcpy(code, C_B_MAGENTA);
	break;
    case 'R':
	strcpy(code, C_B_RED);
	break;
    case 'W':
	strcpy(code, C_B_WHITE);
	break;
    case 'Y':
	strcpy(code, C_B_YELLOW);
	break;
    case 'D':
	strcpy(code, C_D_GREY);
	break;
    case '*':
	sprintf(code, "%c", '\a');
	break;
    case '/':
	strcpy(code, "");
	break;
    case '-':
	sprintf(code, "%c", '~');
	break;
    case '{':
	sprintf(code, "%c", '{');
	break;
    }

    p = code;
    while (*p != '\0')
    {
	*string = *p++;
	*++string = '\0';
    }

    return strlen(code);
}

#endif

void colourconv(char *buffer, const char *txt, CHAR_DATA *ch)
{
    const      char    *point;
    int     skip = 0;

    if(ch != NULL && ch->desc && txt)
    {
	if(IS_SET(ch->act, PLR_COLOUR))
	{
	    for(point = txt ; *point ; point++)
	    {
		if(*point == '{')
		{
		    point++;
		    skip = colour(*point, ch, buffer);
		    while(skip-- > 0)
			++buffer;
		    continue;
		}
		*buffer = *point;
		*++buffer = '\0';
	    }
	    *buffer = '\0';
	}
	else
	{
	    for(point = txt ; *point ; point++)
	    {
		if(*point == '{')
		{
		    point++;
		    continue;
		}
		*buffer = *point;
		*++buffer = '\0';
	    }
	    *buffer = '\0';
	}
    }
    return;
}

int recode_trans(char *argument, bool outp)
{
    char *p;
    int count = 0;
    int i;

    if (outp)
    {
	char buf[OUTBUF_SIZE];
	bool found;

	buf[0] = '\0';
	p = argument;

	while (*p != '\0')
	{
	    found = FALSE;

	    if (*p & 0x80)
	    {
		for (i = 0;translit[i].rus != '\0';i++)
		    if (translit[i].rus == *p)
		    {
			strcat(buf, translit[i].eng);
			count += strlen(translit[i].eng) - 1;
			found = TRUE;
			break;
		    }
	    }
	    if (!found)
		strncat(buf, p, 1);

	    if (strlen(buf) > OUTBUF_SIZE - 3)
		break;

	    p++;
	}
	strcpy(argument, buf);
    }
    else
    {
	p = argument;

	while (*p != '\0')
	{
	    if (!(*p & 0x80))
	    {
		for (i = 0;translit[i].rus != '\0';i++)
		{
		    int len;

		    if (!memcmp(p, translit[i].eng, (len = strlen(translit[i].eng))))
		    {
			char *tm;

			*p = translit[i].rus;

			if (len > 1)
			    for (tm = p + 1; *tm != '\0'; tm++)
				*tm = *(tm + len - 1);

			break;
		    }
		}
	    }

	    p++;
	}
    }

    return count;
}

int recode(char *argument, int codepage, int16_t outp)
{
    uchar	*CodeTable;       /* Current conversion table. */
    char 	*p;

    /* Identify codepages to convert to/from : */
    switch (codepage)
    {
    default:
	CodeTable = NULL;
	break;
    case CODEPAGE_KOI:
	CodeTable = IS_SET(outp, RECODE_OUTPUT) ? wk : kw;
	break;
    case CODEPAGE_ALT:
	CodeTable = IS_SET(outp, RECODE_OUTPUT) ? wa : aw;
	break;
    case CODEPAGE_MAC:
	CodeTable = IS_SET(outp, RECODE_OUTPUT) ? wm : mw;
	break;
    case CODEPAGE_TRANS:
	return /* IS_SET(outp, RECODE_OUTPUT) ? */ recode_trans(argument, IS_SET(outp, RECODE_OUTPUT) /* TRUE */) /* : 0*/;
    }

    p = argument;

    /* Recode while there are still characters in input file : */
    while (*p != '\0')
    {
	/* CurrChar &= 255 (convert to CHAR) */
	*p &= 0xFF;

	if (*p == '<')
	    SET_BIT(outp, RECODE_NOANTITRIGGER);

	/* Antitrigger */
	if (cfg.antitrigger && IS_SET(outp, RECODE_OUTPUT)
	    && !IS_SET(outp, RECODE_NOANTITRIGGER) && number_bits(1) == 0)
	{
	    int i;

	    for (i = 0; antitrigger[i].rus != '\0'; i++)
		if (*p == antitrigger[i].rus)
		{
		    *p = antitrigger[i].eng;
		    break;
		}
	}

	/* If it's EXTENDED ASCII : */
	if (CodeTable && *p & 0x80)
	    *p = CodeTable[*p & 0x7F];
	p++;
    }

    return 0;
}

void do_port(CHAR_DATA *ch, char *argument)
{
    char buf[30];

    sprintf(buf, "Номер порта: %d\n\r", cfg.port);
    send_to_char(buf, ch);
}

void save_corpses();

void safe_exit(int flag)
{
    DESCRIPTOR_DATA *d, *d_next;

    SLIST_FOREACH_SAFE(d, &descriptor_list, link, d_next)
    {
	CHAR_DATA *ch;

	ch = CH(d);

	if (ch != NULL)
	{
	    write_to_buffer(d, "Ненормальное завершение работы!\n\rСохранение - подожди немного.\n\r", 0);
	    VALIDATE(ch);
	    save_char_obj(ch, FALSE);
	}

	close_socket(d);
    }

    save_corpses();

    if (flag == -1)
	return;

    exit(flag);
}

/*AUTO SHUTDOWN*/
void do_auto_shutdown()
{
    /*This allows for a shutdown without somebody in-game actually calling it.
     -Ferric*/
    FILE *fp;

    /* This is to write to the file. */

    if ((fp = file_open(LAST_COMMAND, "a")) == NULL)
	bugf("Error in do_auto_save opening last_command.txt");
    else
    {
	char *strtime;

	strtime                    = ctime(&current_time);
	strtime			  += 4;
	strtime[strlen(strtime)-6] = '\0';
	fprintf(fp, "[%s] Last Command: %s\n", strtime, last_command);
    }
    file_close(fp);

    safe_exit(-1);

    return;
}

void sig_handler(int sig)
{
    if (++sig_counter > 2)
    {
	bugf("Too many sig_handler's calls!");
	exit(1);
    }

    switch(sig)
    {
    case SIGBUS:
	bugf("Sig handler SIGBUS.");
	do_auto_shutdown();
	signal(SIGBUS, sig_bus);
	break;
    case SIGTERM:
	bugf("Sig handler SIGTERM.");
	do_auto_shutdown();
	signal(SIGTERM, sig_term);
	break;
    case SIGABRT:
	bugf("Sig handler SIGABRT");
	do_auto_shutdown();
	signal(SIGABRT, sig_abrt);
	break;
    case SIGSEGV:
	bugf("Sig handler SIGSEGV");
	do_auto_shutdown();
	signal(SIGSEGV, sig_segv);
	break;
    }

    return;
}


void init_signals()
{
    sig_bus  = signal(SIGBUS, sig_handler);
    sig_term = signal(SIGTERM, sig_handler);
    sig_abrt = signal(SIGABRT, sig_handler);
    sig_segv = signal(SIGSEGV, sig_handler);
}
/* AUTO SHUTDOWN*/

void do_host(CHAR_DATA *ch, char *argument)
{

    CHAR_DATA *victim;

    if (argument[0] == '\0')
    {
	send_to_char("Хост какого игрока ты хотел посмотреть?\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, argument)) == NULL)
    {
	send_to_char("Таких игроков нет в игре.\n\r", ch);
	return;
    }

    if (victim->desc == NULL)
    {
	send_to_char("Victim's descriptor is NULL!\n\r", ch);
	return;
    }

    send_to_char(dns_gethostname(victim->desc->Host), ch);
    send_to_char("\n\r", ch);

    return;
}

bool check_password(DESCRIPTOR_DATA *d, char *argument)
{
    char *p;
    bool alpha = FALSE;

    if (strlen(argument) < 5)
    {
	write_to_buffer(d, "Пароль должен состоять как минимум из 5-ти символов.\n\r", 0);
	return FALSE;
    }

    for (p = argument; *p != '\0'; p++)
    {
	if (*p == '~')
	{
	    alpha = FALSE;
	    break;
	}

	if (*p < '0' || *p > '9')
	    alpha = TRUE;
    }

    if (!alpha)
	write_to_buffer(d, "Новый пароль неприемлем, попытайтесь еще раз.\n\r", 0);

    return alpha;
}

char *fgetf(char *s, int n, register FILE *iop)
{
    register int c;
    register char *cs;

    c = '\0';
    cs = s;
    while(--n > 0 && (c = getc(iop)) != EOF)
	if ((*cs++ = c) == '\0')
	    break;
    *cs = '\0';
    return((c == EOF && cs == s) ? NULL : s);
}

void fputf(char *s, register FILE *iop)
{
    register int c;
    register char *cs;

    cs = s;
    while((c = *cs++) != '\0' && fputc(c, iop) != EOF)
	;
}

void p_open(CHAR_DATA *ch, char *argument)
{
    char buf[5002];
    FILE *fp;

    fp = popen(argument, "r");

    fgetf(buf, 5000, fp);

    strcat(buf, "\n\r");
    page_to_char(buf, ch);

    pclose(fp);

    return;
}

void p_openw(char *command, char *argument)
{
    FILE *fp;

    fp = popen(command, "w");

    fputf(argument, fp);

    pclose(fp);

    return;
}


void do_gdb(CHAR_DATA *ch, char *argument)
{

    char buf[MAX_INPUT_LENGTH + 20];

    buf[0] = '\0';

    if (argument[0] != '\0')
	sprintf(buf, "echo '%s' | ", argument);

    strcat(buf, "gdb rom rom.core");
    p_open(ch, buf);

    return;
}

void do_email_pass(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d;
    char buf[MSL];
    CHAR_DATA *victim;

    one_argument(argument, buf);

    if (buf[0] == '\0')
    {
		if (ch != NULL)
		{
			send_to_char("Кому ты хотел отправить пароль?\n\r", ch);
		}
		return;
    }

    if ((victim = get_char_world(NULL, buf)) != NULL && !IS_NPC(victim))
    {
		if (ch != NULL)
		{
			send_to_char("Этот персонаж сейчас в игре.\n\r", ch);
		}
		return;
    }

    d = new_descriptor();

    if (!load_char_obj(d, buf, FALSE, FALSE))
    {
		if (ch != NULL)
		{
			send_to_char("Таких нет в этом мире.\n\r", ch);
		}
		free_descriptor(d);
		return;
    }

    victim = d->character;
    check_light_status(victim);

    if (!IS_NULLSTR(victim->pcdata->email))
    {
    		mailing(victim->pcdata->email, "BalderDash", victim->pcdata->pwd);
		if (ch != NULL)
		{
			send_to_char("Ok.\n\r", ch);
		}
    }
    else
	{
		if (ch != NULL)
		{
			send_to_char("У этого персонажа не прописан адрес электронной почты.\n\r", ch);
		}
	}

    extract_char_for_quit(victim, FALSE);
    free_descriptor(d);

    return;
}

void do_ident(CHAR_DATA *ch, char *argument)
{
    p_open(ch, "ident rom");

    return;
}

int compare_transl(const void *v1, const void *v2)
{
    struct translit_type a1 = *(struct translit_type *) v1;
    struct translit_type a2 = *(struct translit_type *) v2;

    log_string("1");

    if (a1.rus < a2.rus)
	return -1;

    if (a1.rus > a2.rus)
	return +1;
    else
	return 0;
}


void do_translit(CHAR_DATA *ch, char *argument)
{
    int i, j;
    struct translit_type tr_tmp[33];

    do_help(ch, "translit");

    for (i = 0, j = 0;translit[i].rus != '\0';i++)
	if (UPPER(translit[i].rus) != translit[i].rus)
	{
	    tr_tmp[j].rus = translit[i].rus;
	    strcpy(tr_tmp[j].eng, translit[i].eng);
	    j++;
	}

    qsort(&tr_tmp, j , sizeof(tr_tmp[0]), compare_transl);

    send_to_char("\n\rYou can use following combinations:\n\r\n\r", ch);
    for (i = 0;i < j;i++)
    {
	send_to_char(tr_tmp[i].eng, ch);
	send_to_char("  ", ch);
    }

    send_to_char("\n\r", ch);
}

/*
 * Tagline Code
 * From ftp.mudmagic.com, modified by Falagar
 */

bool COMPARE_TAG(int tag, int value, char *comp)
{
    if (!str_cmp(comp, ">"))
	return (tag > value);
    else if (!str_cmp(comp, "<"))
	return (tag < value);
    else if (!str_cmp(comp, "=") || !str_cmp(comp, "=="))
	return (tag == value);
    else if (!str_cmp(comp, "<="))
	return (tag <= value);
    else if (!str_cmp(comp, ">="))
	return (tag >= value);
    else if (!str_cmp(comp, "!=") || !str_cmp(comp, "<>"))
	return (tag != value);
    else
	return FALSE;
}

bool has_item(CHAR_DATA *ch, int vnum, int item_type, int wear);

void tagline_to_char(const char *text, CHAR_DATA *ch, CHAR_DATA *looker)
{
    char output[MAX_STRING_LENGTH];
    char tagline[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char arg[256];
    char arg2[256];
    int value, sn;

    bool bGo = FALSE;

    const char *desc;
    char *p_output = output;
    char *common;

    /* Если строка пустая - делать ничего не надо */
    if (strlen(text) <= 0)
	return;

    for (desc = text; *desc; desc++)
    {
	bGo      = FALSE;
	value    = 0;

	if (*desc != '[')
	{
	    *p_output++ = *desc;
	    continue;
	}

	if (*++desc != '#')
	{
	    *p_output = *--desc;
	    continue;
	}

	/* Пропускаем начало тэга '[#'. */
	desc++;

	common = buf;
	while(*desc != ':')
	{
	    if (*desc == '\0')
	    {
		bugf("Ошибка: В тэге нет разделителя ':'. Комната: %d.", ch->in_room->vnum);
		*common = *desc;
		break;
	    }
	    *common = *desc;
	    desc++;
	    common++;
	}
	*common = '\0';  // Завершаем тэг
	/* Пропускаем ':' и пробельные символы */
	desc++;
	while (isspace(*desc)) desc++;

	/* Читаем описание тэга */
	common = tagline;
	while (*desc != ']')
	{
	    if (*desc == '\0')
	    {
		bugf("Error: tag has no ending ']'. Room: %d.", ch->in_room->vnum);
		*common = *desc;
		break;
	    }
	    *common = *desc;
	    common++;
	    desc++;
	}
	*common = '\0'; // Завершаем команду

	/* Выделяем аргументы */
	common = buf;
	common = one_argument(common, arg);
	common = one_argument(common, arg2);
	if (is_number(common))
	    value = atoi(common);
	else if (str_cmp(arg, "pos") && strlen(common) > 0)
	    bugf("Ошибка в тэге в комнате %d: %s %s %s.", ch->in_room->vnum, arg, arg2, common);

	/*======[  Случаи  ]======*/
	switch (UPPER(arg[0]))
	{
	case 'A':
	    if (!IS_NPC(ch) && !str_cmp(arg, "age"))
	    {
		bGo = COMPARE_TAG(get_age(ch), value, arg2);
	    }
	    else if (!str_cmp(arg, "align") || !str_cmp(arg, "alignment"))
	    {
		if (!str_cmp(arg2, "good") && IS_GOOD(ch))
		    bGo = TRUE;
		else if (!str_cmp(arg2, "evil") && IS_EVIL(ch))
		    bGo = TRUE;
		else if (!str_cmp(arg2, "neutral") && IS_NEUTRAL(ch))
		    bGo = TRUE;
	    }
	    else if (!str_cmp(arg, "affect")
		     && (sn = skill_lookup(arg2)) >= 0 && is_affected(ch, sn))
	    {
		bGo = TRUE;
	    }
	    break;

	case 'C':
	    if (!str_cmp(arg, "con"))
	    {
		bGo = COMPARE_TAG(get_curr_stat(ch, STAT_CON), value, arg2);
	    }
	    else if (!IS_NPC(ch) && !str_cmp(arg, "class"))
	    {
		if (class_lookup(arg2) == ch->classid)
		    bGo = TRUE;
	    }
	    else if (!str_cmp(arg, "carries"))
	    {
		if (is_number(arg2))
		    if (has_item(ch, atoi(arg2), -1, WEAR_ANYWHERE))
			bGo = TRUE; 
		    else 
			bGo = FALSE;
		else
		    if (get_obj_carry(ch, buf, ch) != NULL)
			bGo = TRUE;
		    else
			bGo = FALSE;
	    }
	    break;

	case 'D':
	    if (!str_cmp(arg, "dex"))
		bGo = COMPARE_TAG(get_curr_stat(ch, STAT_DEX), value, arg2);
	    else if (!str_cmp(arg, "day"))
		bGo = COMPARE_TAG(time_info.day, value, arg2);
	    break;

	case 'G':
	    if (!str_cmp(arg, "gold"))
	    {
		bGo = COMPARE_TAG(ch->gold, value, arg2);
	    }
	    else if (!str_cmp(arg, "group"))
	    {
		CHAR_DATA *vch;
		int count = 0;

		LIST_FOREACH(vch, &ch->in_room->people, room_link)
		{
		    if (is_same_group(ch, vch))
			count++;
		}

		bGo = COMPARE_TAG(count, value, arg2);
	    }

	    break;

	case 'I':
	    if (!str_cmp(arg, "int"))
	    {
		bGo = COMPARE_TAG(get_curr_stat(ch, STAT_INT), value, arg2);
	    }
	    break;

	case 'L':
	    if (!str_cmp(arg, "level"))
	    {
		bGo = COMPARE_TAG(ch->level, value, arg2);
	    }
	    break;
	case 'M':
	    if (!str_cmp(arg, "month"))
	    {
		bGo = COMPARE_TAG(time_info.month, value, arg2);
	    }
	    break;

	case 'N':
	    if (!str_cmp(arg, "name"))
	    {
		char p_name[128];
		char *t = tagline;
		char *n;
		bGo = TRUE;
		common = buf;

		while (*t != '\0')
		{
		    if (*t != '*')
		    {
			*common = *t;
			common++;
			t++;
			continue;
		    }

		    sprintf(p_name, "%s", ch->name);
		    n = p_name;

		    while (*n != '\0')
		    {
			*common = *n;
			common++;
			n++;
		    }
		    t++;
		}
		*common = '\0';
		sprintf(tagline, "%s", buf);
	    }
	    else if (!str_cmp(arg, "noclass"))
	    {
		if (class_lookup(arg2) != ch->classid)
		    bGo = TRUE;
	    }
	    else if (!str_cmp(arg, "norace"))
	    {
		if (race_lookup(arg2) != ch->race)
		    bGo = TRUE;
	    }
	    break;

	case 'P':
	    if (!str_cmp(arg, "pos"))
	    {
		bGo = COMPARE_TAG(ch->position, flag_lookup(common, position_flags), arg2);
	    }
	    break;

	case 'R':
	    if (!str_cmp(arg, "race"))
	    {
		if (race_lookup(arg2) == ch->race)
		    bGo = TRUE;
	    }
	    break;

	case 'S':
	    if (!str_cmp(arg, "sex"))
	    {
		if (sex_lookup(arg2) == ch->sex)
		    bGo = TRUE;
	    }
	    else if (!str_cmp(arg, "silver"))
	    {
		bGo = COMPARE_TAG(ch->silver, value, arg2);
	    }
	    else if (!str_cmp(arg, "str"))
	    {
		bGo = COMPARE_TAG(get_curr_stat(ch, STAT_STR), value, arg2);
	    }
	    else if (!str_cmp(arg, "sun"))
	    {
		if (!str_cmp(arg2, "day") && (weather_info.sunlight == SUN_RISE))
		    bGo = TRUE;
		else if (!str_cmp(arg2, "sunset") && (weather_info.sunlight == SUN_SET))
		    bGo = TRUE;
		else if (!str_cmp(arg2, "sunrise") && (weather_info.sunlight == SUN_LIGHT))
		    bGo = TRUE;
		else if (!str_cmp(arg2, "night") && (weather_info.sunlight == SUN_DARK))
		    bGo = TRUE;
	    }
	    break;

	case 'T':
	    if (!str_cmp(arg, "time"))
	    {
		if (value < 0 || value > 24)
		    bGo = FALSE;
		else
		{
		    bGo = COMPARE_TAG(time_info.hour, value, arg2);
		}
	    }
	    break;

	case 'W':
	    if (!str_cmp(arg, "wis"))
	    {
		bGo = COMPARE_TAG(get_curr_stat(ch, STAT_WIS), value, arg2);
	    }
	    else if (!str_cmp(arg, "weather"))
	    {
		if (!str_cmp(arg2, "sunny") && (weather_info.sky == SKY_CLOUDLESS))
		    bGo = TRUE;
		else if (!str_cmp(arg2, "cloudy") && (weather_info.sky == SKY_CLOUDY))
		    bGo = TRUE;
		else if (!str_cmp(arg2, "rainy") && (weather_info.sky == SKY_RAINING))
		    bGo = TRUE;
		else if (!str_cmp(arg2, "lightning") && (weather_info.sky == SKY_LIGHTNING))
		    bGo = TRUE;
	    }
	    break;

	default:
	    bGo = FALSE;
	    break;
	}
	/* ======[Все, случаи кончались]====== */

	if (!bGo && (sn = skill_lookup(arg)) >= 0)
	    bGo = COMPARE_TAG(get_skill(ch, sn), value, arg2);

	/* Копируем полученное в ответ */
	if (bGo == TRUE)
	{
	    common = tagline;
	    while (*common != '\0')
	    {
		if (strlen(output) >= (MAX_STRING_LENGTH - 1))
		    break;

		*p_output = *common;
		++common;
		*++p_output = '\0';
	    }
	}
    }

    /* Отправляем полученное игроку */
    *p_output = '\0';          // Конец строки
    send_to_char(output, looker);  // Отправляем строку чару
    return;
}

void generate_character_password(CHAR_DATA *ch)
{
	char buf[15];
	int i;

	for (i=0; i<14; i++)
	{
		buf[i] = number_range('a', 'z');
	}
	
	buf[i] = '\0';

	free_string(ch->pcdata->pwd);
	ch->pcdata->pwd = str_dup(buf);
	save_char_obj(ch, FALSE);

	return;
}		   

void mailing(char *to, char *subj, char *body)
{
	char mailer[] = "/usr/sbin/sendmail -t";
	char buf[MAX_STRING_LENGTH];
			
	sprintf(buf, 	"To: %s\n"
			"From: brullek@yandex.ru\n"
			"Content-Type: text/plain; charset=Windows-1251\n"
			"Subject: %s\n\n%s",
			to, subj, body);
	p_openw(mailer, buf);
}

/* charset=cp1251 */



