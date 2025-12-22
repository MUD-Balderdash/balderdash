#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <regex.h>

#include "merc.h"

#define MAX_REGEXP_TO_ANGRY	10
#define NMATCH 			8



char regexp_vars[NMATCH][MAX_INPUT_LENGTH];

CHAT_KEYS *get_chat_index(int vnum)
{
    CHAT_KEYS *k;

    for (k = chatkeys_list; k != NULL; k = k->next)
	if (k->vnum == vnum)
	    return k;

    return NULL;
}

REPLY *reply_free;

REPLY *new_reply()
{
    REPLY *newone;

    if (!reply_free) 
	newone = alloc_perm(sizeof(*newone));
    else
    {
	newone	 = reply_free;
	reply_free = reply_free->next;
    }
    
    newone->weight = 0;
    newone->sex  = -1;
    newone->race = 0;
    newone->sent = NULL;
    newone->next = NULL;

    return newone;
}

void free_reply(REPLY *r)
{
    free_string(r->sent);

    r->next = reply_free;
    reply_free  = r;
}

CHAT_KEY *ck_free;

CHAT_KEY *new_chatkey()
{
    CHAT_KEY *newone;

    if (!ck_free)
    {
	newone = alloc_perm(sizeof(*newone));
    }
    else
    {
        newone	    = ck_free;
        ck_free     = ck_free->next;
    }

    newone->regexp = NULL;
    newone->replys = NULL;
    newone->next   = NULL;

    return newone;
}

void free_chatkey(CHAT_KEY *ck)
{
    REPLY *r, *r_next;

    free_string(ck->regexp);

    for (r = ck->replys; r != NULL; r = r_next)
    {
	r_next = r->next;
	free_reply(r);
    }

    ck->next = ck_free;
    ck_free  = ck;
}

CHAT_KEYS *chatkeys_free;

CHAT_KEYS *new_chatkeys()
{
    CHAT_KEYS *newone;

    if (!chatkeys_free)
    {
	newone = alloc_perm(sizeof(*newone));
    }
    else
    {
        newone	    	= chatkeys_free;
        chatkeys_free   = chatkeys_free->next;
    }

    newone->next   = NULL;
    newone->list   = NULL;
    newone->vnum   = 0;
    newone->area   = NULL;

    return newone;
}

void free_chatkeys(CHAT_KEYS *cks)
{
    CHAT_KEY *k, *k_next;

    for (k = cks->list; k != NULL; k = k_next)
    {
	k_next = k->next;
	free_chatkey(k);
    }

    cks->next     = chatkeys_free;
    chatkeys_free = cks;
}


void load_chat(FILE *fp)
{
    extern bool fBootDb;
    
    if (area_last == NULL)
    {
	bugf("Load_chat: no #AREA seen yet.");
	exit(1);
    }

    for (;;)
    {
	int vnum;
	
	char c;
        CHAT_KEYS *chatkeys_new;
        CHAT_KEY *akey_new = NULL;

	c = fread_letter(fp);
	if (c != '#')
	{
	    bugf("Load_chat: # not found.");
	    exit(1);
	}

	vnum		 = fread_number(fp);
	
	if (vnum == 0)
	    break;

	chatkeys_new = new_chatkeys();
        chatkeys_new->next = chatkeys_list;
        chatkeys_list = chatkeys_new;
        chatkeys_list->vnum = vnum;
        chatkeys_list->area = area_last;
	
	for (;;)
	{
	    c = fread_letter(fp); 

	    if (c == '#')
	    {
	      	ungetc(c, fp);
		break;
	    }

	    if (c != '*')
	    {
		if (c == '=')
		{
		    CHAT_KEY *tmp;

                    akey_new = new_chatkey();

		    if (chatkeys_new->list)
		    {
		  	for(tmp = chatkeys_new->list; tmp->next != NULL; tmp = tmp->next)
		      	    ;
		      
			tmp->next = akey_new;
		    }
                    else
			chatkeys_new->list = akey_new; 

                    akey_new->regexp = fread_string(fp);
		}
		else if (akey_new && c >= '0' && c <= '9')
		{         
		    REPLY *reply_new = new_reply();
                    char *str;

		    reply_new->next = akey_new->replys;
		    akey_new->replys = reply_new;

		    reply_new->weight = c - '0';

		    fBootDb = FALSE;

                    str = fread_string(fp);
		      
		    if (!IS_NULLSTR(str))
			reply_new->race = race_lookup(str);
		      
		    free_string(str);

		    str = fread_string(fp);
		
		    if (!IS_NULLSTR(str))
			reply_new->sex  = sex_lookup(str);

		    free_string(str);
		    fBootDb = TRUE;

		    reply_new->sent = fread_string(fp);
		}
	    }

	    fread_to_eol(fp);
	}
    }
    return;
}

bool check_regexp(char *reg, char *msg)
{
    regex_t preg;
    regmatch_t pmatch[NMATCH];
    int result, i;

    regcomp(&preg, reg, REG_EXTENDED);
    result = regexec(&preg, msg, NMATCH, pmatch, 0);
    regfree(&preg);

    if (result != 0)
	return FALSE;

    for (i = 0; i < NMATCH; i++)
	if (pmatch[i].rm_so != -1)
	{
	    memcpy(regexp_vars[i], msg + pmatch[i].rm_so,
		   pmatch[i].rm_eo - pmatch[i].rm_so);
	    regexp_vars[i][pmatch[i].rm_eo - pmatch[i].rm_so] = '\0';
	}
	else
	    regexp_vars[i][0] = '\0';

    return TRUE;
}

void process_vars(char *replied, CHAR_DATA *ch, CHAR_DATA *victim, char *rep)
{
    char *i;
    char *point   = rep;
    char *str     = replied;
    
    while (*str != '\0')    
    {
        int cs, delta, varindex;

	if (*str != '$')
	{
	    *point++ = *str++;
	    continue;
	}
	++str;
	
	varindex = *str - '0';
        
        cs = *(str+1)-'0';
        if (cs < 0 || cs > 6) 
        {
	    cs = 0;
	    delta = 1;
        } 
        else
	    delta = 2;

	if (varindex > 0 && varindex < NMATCH)
	{
	    i = regexp_vars[varindex];
	    delta = 1;
	}
	else
	{
	    switch (*str)         /*add $Variable commands here*/
	    {
	    case 'n':         
		i = PERS(ch, victim, cs);
		break;
	    case 't': 
		i = PERS(victim, ch, cs);
		break;
	    case '$': 
		i = "$";
		break;
	    default:
		i = "";
		break;
	    }
	}
	str += delta;
	while ((*point = *i) != '\0')
	{
	    ++point;
	    ++i;
	}
    }
    
    *point++ = '\0';
}

char *dochat(CHAR_DATA *ch, char *msg, CHAR_DATA *victim)
{
    CHAT_KEYS *k;
    char *p;
    int regexp_count = 0, i;
    bool angry = FALSE;
    static char rpl[MAX_INPUT_LENGTH];

    if (!can_see(ch, victim))
    {
	strcpy(msg, "cannotsee");
	angry = TRUE;
    }
    else if (ch->angry >= MAX_REGEXP_TO_ANGRY)
    {
	strcpy(msg, "angry");
	angry = TRUE;
    }

    for (p = msg; *p != '\0'; p++)
	*p = LOWER(*p);

    p = NULL;

    for(i = 0; ch->pIndexData->chat_vnum[i] && i < MAX_CHATBOOKS; i++)
    {
      for (k = chatkeys_list; k != NULL; k = k->next)
      {
	if (k->vnum == ch->pIndexData->chat_vnum[i] || angry) 
	{
	    CHAT_KEY *ck;

	    for (ck = k->list; ck != NULL; ck = ck->next)
	    {  
		if (check_regexp(ck->regexp, msg))
		{
		    REPLY *r;
		    int totalweight = 0, i;

		    for (r = ck->replys; r != NULL; r = r->next)
		    {
			r->tmp = 1;

			if (r->race == ch->race)
			    r->tmp++;
			else if (r->race != 0)
			    r->tmp = 0;

			if (r->tmp > 0)
			{
			    if (r->sex == ch->sex)
				r->tmp++;
			    else if (r->sex != -1)
				r->tmp = 0;
			}
        
			r->tmp *= r->weight;
			totalweight += r->tmp;
		    }

		    for (i = 0; i < 10; i++)
		    {
			for (r = ck->replys; r != NULL; r = r->next)
			{
			    if (r->tmp > number_range(0, totalweight))
			    {
				char buf[MAX_STRING_LENGTH], strsave[100];

				if (regexp_count == ch->last_regexp)
				{
				    if (!angry)
					ch->angry++;
				}
				else
				{
				    if (ch->angry > 0 && !angry)
					ch->angry--;
				    ch->last_regexp = regexp_count;
				}
				process_vars(r->sent, ch, victim, rpl);

				sprintf(buf, "Input   : %s\nResponse: %s\n", msg, rpl);
				sprintf(strsave,"%s%s",LOG_DIR, CHAT_LOG);
				_log_string(buf, strsave);

				return rpl;
			    }
			}
		    }	
		}
	    }
	}
        regexp_count++;
      }
    }

    return p;
}

/* charset=cp1251 */
