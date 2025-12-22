#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "merc.h"
#include "interp.h"
#include "recycle.h"




#define QUEST_COPPER 1218
#define QUEST_BRONZE 1219
#define QUEST_SILVER 1220
#define QUEST_GOLD   1221
#define QUEST_PLATIN 1222

struct reward_type
{
    char *name;
    char *keyword;
    int cost;
    bool object;
    int value;
    void *where;
};

struct quest_desc_type
{
    char *name;
    char *short_descr;
    char *long_descr;
};
/* Descriptions of quest items go here:
 Format is: "keywords", "Short description", "Long description" */
const struct quest_desc_type quest_desc[] =
{
    {
	"квестовый жезл", 	"Жезл отваги",
	"Жезл Отваги лежит здесь, ожидая возвращения к своему хозяину."
    },

    {
	"квестовая корона", 	"Корона мудрости",
	"Корона Мудрости лежит здесь, ожидая возвращения к своему хозяину."
    },

    {
	"квестовые перчатки", 	"Перчатки силы",
	"Перчатки силы лежат здесь, ожидая возвращения к своему хозяину."
    },

    {
	NULL, NULL, NULL
    }
};

/* Local functions */
void generate_quest(CHAR_DATA *ch, CHAR_DATA *questman, bool is_quest);
bool quest_level_diff(int clevel, int mlevel);

void log_group_quest(CHAR_DATA *qm, char *reason)
{
    /*    GROUP_QUEST_DATA *gq;
     NAME_LIST *nl;
     char logname[50], *tm, bfr[MSL], tmp[MSL];
     int i = 0;

     tm = ctime(&current_time);
     tm[strlen(tm) - 1] = '\0';
     sprintf(logname, "%s/%s", LOG_DIR, QUEST_LOG);

     sprintf(bfr, "%s :: %s\n-----------------------------------------\n", tm, reason);

     for (gq = qm->group_quest; gq; gq = gq->next)
     {

     if (!IS_VALID(gq))
     {
     bugf("Trying to work with invalidated group_quest.");
     continue;
     }

     sprintf(tmp, "Group (ticks - %d):\n", gq->time);
     strcat(bfr, tmp);
     i++;
     for (nl = gq->names; nl; nl = nl->next)
     {
     sprintf(tmp, "%s\n", nl->name);
     strcat(bfr, tmp);
     }
     }    

     if (i > 0)
     _log_string(bfr, logname); */
}


void *check_quest_complete(CHAR_DATA *ch, CHAR_DATA *qm, bool quit)
{
    OBJ_DATA *obj = NULL;
    void *ret = NULL;
    GROUP_QUEST_DATA *gq = NULL;

    if (ch->pcdata->countdown < 1)
	return NULL;

    if (qm && IS_SET(ch->pcdata->quest_type, QUEST_GROUP))
    {
	CHAR_DATA *fch;
	NAME_LIST *nl;
	bool flag = FALSE;

	for (gq = qm->group_quest; gq; gq = gq->next)
	{

	    if (!IS_VALID(gq))
	    {
		bugf("Trying to work with invalidated group_quest.");
		continue;
	    }

	    for (nl = gq->names; nl; nl = nl->next)
	    {
		if (!str_cmp(nl->name, ch->name))
		{
		    flag = TRUE;
		    break;
		}
	    }	

	    if (flag)
		break;
	}

	if (!gq)
	{
	    bugf("Group_quest: Cannot find group for char %s.", ch->name);
	    log_group_quest(qm, "check_quest_complete");
	    return NULL;
	}

	for (nl = gq->names; nl; nl = nl->next)
	{
	    LIST_FOREACH(fch, &qm->in_room->people, room_link)
	    {
		if (!str_cmp(fch->name, nl->name)
		    && !IS_NPC(fch)
		    && is_same_group(ch, fch)
		    && IS_SET(fch->act, PLR_QUESTING))
		{
		    break;
		}
	    }

	    if (!fch)
	    {
		do_say(qm, "Похоже, что здесь немного не та группа, которой давался квест.");

		if (!quit)
		    return NULL;
		else
		{
		    do_say(qm, "Для выхода из квеста необходимо собрать исходную группу.");
		    return (void *) qm;
		}
	    }
	}
    }  

    if (ch->pcdata->questobj)
    {
	for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
	    if (obj->pIndexData->vnum == ch->pcdata->questobj)
	    {
		ret = (void *) obj;
		break;
	    }	
    }
    else if (ch->pcdata->questmob == -1)
    {
	if (gq)
	    gq->time = 0;

	ret = (void *) ch;
    }	

    return ret;
}

void end_quest(CHAR_DATA *ch, int to_next)
{
    REMOVE_BIT(ch->act, PLR_QUESTING);
    ch->pcdata->questgiver = NULL;
    ch->pcdata->qtime = 0;
    ch->pcdata->countdown = 0;
    ch->pcdata->questmob = 0;
    ch->pcdata->questobj = 0;
    ch->pcdata->quest_type = 0;
    ch->pcdata->nextquest = to_next;
}

void do_quest(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *questman, *fch, *safe_fch;
    OBJ_DATA *obj = NULL;
    MOB_INDEX_DATA *questinfo;
    char buf [MAX_STRING_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    int i;

    /* Add your rewards here.  Works as follows:
     "Obj name shown when qwest list is typed", "keywords for buying",
     "Amount of quest points",  Does it load an object?,  IF it loads an
     object, then the vnum, otherwise a value to set in the next thing,  This
     is last field is a part of the char_data to be modified */

    const struct reward_type reward_table[]=
    {
	/*
	 { "A Flask of Endless Water","flask water",   500, TRUE,  46,    0},
	 { "Jug O' Moonshine",        "jug moonshine", 300, TRUE,  47,    0},
	 { "Potion of Extra Healing", "extra healing", 300, TRUE,  4639,  0},
	 { "Potion of Sanctuary",     "sanctuary",     150, TRUE,  3081,  0},
	 */
	{ "-- === Д О С П Е Х И === ---", "d1", 	0, FALSE, 0, &ch->gold },

	{ "{BКвестовый {gшлем{x",         "шлем",      450, TRUE, 1214,  0},
	{ "{BКвестовый {gпатронташ{x",    "патронташ", 500, TRUE, 1215,  0},
	{ "{gОрден {Bквестера{x",         "орден",     500, TRUE, 1216,  0},
	{ "{BКвестовый {Wплащ{x",         "плащ",      1000,TRUE, 1282,  0},
	{ "{BКвестовый {gбраслет{x",      "браслет",   1000, TRUE, 1284,  0},
	{ "{WКольцо {Bквестера{x",        "кольцо",    1000, TRUE, 1217,  0},
	{ "{RБотфорты {rквестера{x",      "ботфорты",  5000, TRUE, 1283,  0},
	{ "{WДух искателя{x",        	  "дух",       10000, TRUE, 1286,  0},

	{ "----- === Щ И Т Ы === -----", "d1", 	0, FALSE, 0, &ch->gold },

	
	{ "Маленький квестовый щит",      "маленький", 2500, TRUE, 1230,  0},
	{ "Средний квестовый щит",        "средний",   2500, TRUE, 1228,  0},
	{ "Большой квестовый щит",        "большой",   2500, TRUE, 1229,  0},

	{ "--- === О Р Д Е Н А === ---", "d2", 	0, FALSE, 0, &ch->gold },

	{ "{yМедный орден Квестера{x",    "медный",    10, TRUE, QUEST_COPPER, 0 },
	{ "{YБронзовый орден Квестера{x", "бронзовый", 200, TRUE, QUEST_BRONZE, 0 },
	{ "{CСеребряный орден Квестера{x","серебряный", 400, TRUE, QUEST_SILVER, 0 },
	{ "{RЗолотой орден Квестера{x",	  "золотой",   600, TRUE, QUEST_GOLD,   0 },
	{ "{WПлатиновый орден Квестера{x","платиновый", 1500, TRUE, QUEST_PLATIN, 0 },

	{ "--- === П Р О Ч Е Е === ---", "d3", 	0, FALSE, 0, &ch->gold },

	{ "Книга знаний",                 "книга",      5, TRUE,  3389, 0 },
	{ "зелье полного восстановления", "зелье",      50, TRUE, 1223, 0 },
	{ "фляжка", 			  "фляжка",      200, TRUE, 1225, 0 },
	{ "1000 золота",     		  "1000 золота", 250, FALSE, 1000, &ch->gold },
	{ "{GПосмертие{x",     		  "d4", 	3000, FALSE, 1000, &ch->gold },
	{ "{GБессмертие{x",    		  "d5", 	5000, FALSE, 1000, &ch->gold },

	{ "* === А Р Т Е Ф А К Т Ы === ", "d4", 	0, FALSE, 0, &ch->gold },

	{ "Руна (Золотая руна)",     	  "руна", 		30000, TRUE, 547, 0 },
	{ "Метеорит (Обломок метеорита)", "метеорит", 		20000, TRUE, 555, 0 },
	{ "Обломок необычного металла",   "обломок металла", 	20000, TRUE, 561, 0 },

	{ NULL, NULL, 0, FALSE, 0, 0  } /* Never remove this!!! */
    };


    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (IS_NPC(ch))
    {
	send_to_char("Мобы не могут выполнять квесты.\n\r", ch);
	return;
    }

    if (arg1[0] != '\0')
    {

	if (!str_cmp(arg1, "off") || !str_prefix(arg1, "выключить"))
	{
	    SET_BIT(ch->act, PLR_NOIMMQUEST);
	    send_to_char("Сообщения квестов от Богов не будут выводиться.\n\r", ch);
	    return;
	}

	if (!str_cmp(arg1, "on") || !str_prefix(arg1, "включить"))
	{
	    REMOVE_BIT(ch->act, PLR_NOIMMQUEST);
	    send_to_char("Теперь ты будешь видеть сообщения квестов от Богов.\n\r", ch);
	    return;
	}

	if (!str_prefix(arg1, "удвоение"))
	{
	    return;
	}

	if (!str_prefix(arg1, "утроение"))
	{
	    return;
	}


	if (!strcmp(arg1, "info") || !strcmp(arg1, "инфо"))
	{
	    if (IS_SET(ch->act, PLR_QUESTING))
	    {
		if (ch->pcdata->questmob == -1
		    && ch->pcdata->questgiver->short_descr != NULL)
		{
		    sprintf(buf, "Ты полностью выполнил%s квест!\n\r"
			    "Вернись скорее к %s, пока время не вышло!\n\r",
			    SEX_ENDING(ch),
			    cases(ch->pcdata->questgiver->short_descr, 2));
		    send_to_char(buf, ch);
		    return;
		}
		else if (ch->pcdata->questobj > 0)
		{
		    if (ch->pcdata->questobjname)
		    {
			sprintf(buf, "Ты сейчас ищешь легендарный предмет - %s!\n\r",
				cases(ch->pcdata->questobjname, 6));
			send_to_char(buf, ch);
			return;
		    }
		}
		else if (ch->pcdata->questmob > 0)
		{
		    questinfo = get_mob_index(ch->pcdata->questmob);
		    if (questinfo != NULL)
		    {
			sprintf(buf,"Тебе надо уничтожить %s!\n\r",
				cases(questinfo->short_descr, 3));
			send_to_char(buf, ch);
			return;
		    }
		}

		if (IS_SET(ch->pcdata->quest_type, QUEST_GROUP))
		    send_to_char("Кроме того, данный квест является групповым.\n\r", ch);
	    }
	    send_to_char("В данное время ты не выполняешь квестов.\n\r", ch);
	    return;
	}
	else if (!strcmp(arg1, "time") || !strcmp(arg1, "время"))
	{
	    if (!IS_SET(ch->act, PLR_QUESTING))
	    {
		send_to_char("В данное время ты не выполняешь квестов.\n\r", ch);
		if (ch->pcdata->nextquest > 1)
		{
		    sprintf(buf, "Для того, чтобы взять следующий квест, "
			    "тебе нужно подождать %d %s.\n\r",
			    ch->pcdata->nextquest,
			    hours(ch->pcdata->nextquest, TYPE_MINUTE));
		    send_to_char(buf, ch);
		}
		else if (ch->pcdata->nextquest == 1)
		    send_to_char("Осталось меньше минуты до того, как ты сможешь "
				 "взять еще один квест.\n\r", ch);
	    }
	    else if (ch->pcdata->countdown > 0)
	    {
		sprintf(buf, "До окончания текущего квеста осталось %d %s.\n\r",
			ch->pcdata->countdown,
			hours(ch->pcdata->countdown, TYPE_MINUTES));
		send_to_char(buf, ch);
	    }
	    return;
	}
	else if (!strcmp(arg1, "score") || !strcmp(arg1, "счет"))
	{
	    sprintf(buf, "У тебя сейчас %d %s удачи. ",
		    ch->pcdata->quest_curr,
		    hours(ch->pcdata->quest_curr, TYPE_POINTS));
	    send_to_char(buf, ch);

	    sprintf(buf, "За свою жизнь ты набрал%s всего %d %s.\n\r",
		    SEX_ENDING(ch),
		    ch->pcdata->quest_accum,
		    hours(ch->pcdata->quest_accum, TYPE_POINTS));
	    send_to_char(buf, ch);
	    return;
	}

	if (ch->position < POS_STANDING)
	{
	    send_to_char("Тебе нужно встать.\n\r", ch);
	    return;
	}


     /* СЛЕДУЮЩИЕ  КОМАНДЫ РАБОТАЮТ ТОЛЬКО У СЕКРЕТАРЕЙ */
	LIST_FOREACH(questman, &ch->in_room->people, room_link)
	{
	    if (IS_NPC(questman) && IS_SET(questman->act, ACT_QUESTER))
		break;
	}

	if (questman == NULL)
	{
	    send_to_char("Ты не можешь этого здесь.\n\r", ch);
	    return;
	}



	/** БЕССМЕРТИЕ */
	if (!strcmp(arg1, "бессмертие"))
	{
	    if (ch->pcdata->temp_RIP)
	    {
			send_to_char("Ты уже обрел бессмертие или посмертие.\n\r", ch);
			return;
		}

				
		if (ch->pcdata->quest_curr < 5000)
		{
			send_to_char("Тебе не хватает очков удачи. Бессмертие стоит 5000 очков.\n\r", ch);
			return;
		}

		ch->pcdata->quest_curr -= 5000;
		

		send_to_char("Ты обретаешь временное бессмертие.\n\r", ch);
		ch->pcdata->temp_RIP = 1;
		send_to_char("Сейчас ты его не желаешь. Чтобы узнать подробнее, введи {yбессмертие статус{x.\n\r", ch);
		return;
	}
	else if (!strcmp(arg1, "посмертие"))
	{
	    if (ch->pcdata->temp_RIP)
	    {
			send_to_char("Ты уже обрел посмертие или бессмертие.\n\r", ch);
			return;
		}
		
		if (ch->pcdata->quest_curr < 3000)
		{
			send_to_char("Тебе не хватает очков удачи. Посмертие стоит 3000 очков.\n\r", ch);
			return;
		}

		ch->pcdata->quest_curr -= 3000;
		send_to_char("Ты обретаешь посмертие.\n\r", ch);
		ch->pcdata->temp_RIP = 3;
		return;
	}
	/* ***********************/






	if (POS_FIGHT(questman))
	{
	    send_to_char("Подожди, пока окончится драка.\n\r", ch);
	    return;
	}
	
	if (!str_cmp(arg1, "artifact") || !str_prefix(arg1, "артефакт"))
	{
		int level = 0;
		OBJ_DATA *a_obj, *a_obj_next;

    	for (a_obj = ch->carrying; a_obj != NULL; a_obj = a_obj_next)
    	{
			a_obj_next = a_obj->next_content;

			if (a_obj->item_type == ITEM_ARTIFACT)
			{
				level += a_obj->level;
			    extract_obj(a_obj, TRUE, TRUE);
				continue;
			}

			if (a_obj->item_type == ITEM_CONTAINER)
			{
				OBJ_DATA *a_lobj, *a_lobj_next;
		    	
				for (a_lobj = a_obj->contains; a_lobj != NULL; a_lobj = a_lobj_next)
    			{
					a_lobj_next = a_lobj->next_content;
					if (a_lobj->item_type == ITEM_ARTIFACT)
					{
						level += a_lobj->level;
			    		extract_obj(a_lobj, TRUE, TRUE);
						continue;
					}
				}
			}
    	}

		if (level != 0 )
		{
			level = number_range(1, level/2 < 0 ? 1 : level/2);
			ch->pcdata->quest_curr += level;
	    	ch->pcdata->quest_accum += level;
	 		sprintf(buf, "Ты продаешь свои артефакты за %d QP.\n\r", level);
	    	send_to_char(buf, ch);
		}
		else
		{
			send_to_char("У тебя нет ни одного артефакта.\n\r", ch);
		}

	    return;
	}
	else if (!strcmp(arg1, "list") || !strcmp(arg1, "список"))
	{
	    act("$n запрашивает у $N1 список квестовых вещей.",
		ch, NULL, questman, TO_ROOM);
	    act("Ты запрашиваешь у $N1 список квестовых вещей.",
		ch, NULL, questman, TO_CHAR);
	    send_to_char("Пока доступны следующие вещи:\n\r\n\r", ch);
	    if (reward_table[0].name == NULL)
		send_to_char("  Ничего.\n\r", ch);
	    else
	    {
		send_to_char("  [{WЦена{x]   [{WУровень{x]   [{WНазвание{x]\n\r", ch);
		for (i = 0; reward_table[i].name != NULL; i++)
		{
		    int level;

		    if (reward_table[i].object)
		    {
			OBJ_INDEX_DATA *pObjIndex;

			pObjIndex = get_obj_index(reward_table[i].value);
			level = pObjIndex->level;
		    }	
		    else
			level = 0;

		    sprintf(buf, "   {W%-5d{x       {W%-2d{x       {W%s{x\n\r",
			    reward_table[i].cost, level, reward_table[i].name);
		    send_to_char(buf, ch);
		}

		sprintf(buf, "\n\rДля того, чтобы купить, набери 'квест купить "
			"<вещь>'.\n\rУ тебя есть %d %s удачи.\n\r",
			ch->pcdata->quest_curr,
			hours(ch->pcdata->quest_curr, TYPE_POINTS));
		send_to_char(buf, ch);
		return;
	    }
	}
	else if (!strcmp(arg1, "buy") || !strcmp(arg1, "купить"))
	{
	    bool found = FALSE;
	    if (arg2[0] == '\0')
	    {
		send_to_char("Чтобы купить что-то, набери "
			     "'квест купить <вещь>'.\n\r", ch);
		return;
	    }

	    /* Use keywords rather than the name listed in qwest list */
	    /* Do this to avoid problems with something like 'qwest buy the' */
	    /* And people getting things they don't want... */
	    for (i = 0; reward_table[i].name != NULL; i++)
		if (is_name(arg2, reward_table[i].keyword))
		{
		    found = TRUE;
		    if (ch->pcdata->quest_curr >= reward_table[i].cost)
		    {
			if (reward_table[i].object)
			{
			    OBJ_INDEX_DATA *pObjIndex;

			    pObjIndex = get_obj_index(reward_table[i].value);

			    if (pObjIndex->level > ch->level)
			    {
				send_to_char("Тебе еще рано носить эту вещь.\n\r",
					     ch);
				return;
			    }

			    obj = create_object(pObjIndex, ch->level);
			    //			    obj->owner = str_dup(ch->name);

			    sprintf(buf, "В обмен на твою удачу (%d %s), "
				    "%s дает тебе %s.\n\r",
				    reward_table[i].cost,
				    hours(reward_table[i].cost, TYPE_POINTS),
				    questman->short_descr,
				    cases(obj->short_descr, 6));
			    send_to_char(buf, ch);
			    obj_to_char(obj, ch);
			}
			else
			{
			    sprintf(buf, "В обмен на твою удачу (целых %d %s!), "
				    "%s дает тебе %s.\n\r",
				    reward_table[i].cost,
				    hours(reward_table[i].cost, TYPE_POINTS),
				    questman->short_descr,
				    cases(reward_table[i].name, 6));
			    send_to_char(buf, ch);
			    *(int *)reward_table[i].where += reward_table[i].value;
			}
			ch->pcdata->quest_curr -= reward_table[i].cost;
			break;
		    }
		    else
		    {

			sprintf(buf, "Извини, ты не настолько удачлив%s. Тебе "
				"нужно выполнить еще несколько квестов.", SEX_ENDING(ch));
			do_say(questman, buf);
			return;
		    }
		}

	    if (!found)
		do_say(questman, "У меня нет этого.");

	    return;
	}
	else if (!strcmp(arg1, "request") || !strcmp(arg1, "запросить"))
	{
	    bool is_group = FALSE;
	    int count = 0;
	    CHAR_DATA *fch, *safe_fch;

	    act("$n запрашивает у $N3 квест.", ch, NULL, questman, TO_ROOM);
	    act ("Ты запрашиваешь у $N3 квест.", ch, NULL, questman, TO_CHAR);


	    LIST_FOREACH_SAFE(fch, &ch->in_room->people, room_link, safe_fch)
	    {
		if (!IS_NPC(fch) && is_same_group(fch, ch))
		{
		    if (IS_SET(fch->act, PLR_QUESTING))
		    {
			sprintf(buf, "Гмм... %s, ты же уже выполняешь квест!", fch->name);
			do_say(questman, buf);
			return;
		    }

		    if (fch->pcdata->nextquest > 0)
		    {
			sprintf(buf, "Ты очень храбр%s, %s, но дай и другим шанс! Приходи попозже.",
				SEX_ENDING(fch), fch->name);
			do_say(questman, buf);
			return;
		    }

		    if (IS_RENEGATE(fch))
		    {
			sprintf(buf, "Извини, %s, я не могу доверить тебе это задание, поскольку ты изменяешь своим принципам.",
				fch->name);
			do_say(questman, buf);
			return;
		    }

		    count++;
		}
	    }

	    if (count > 1)
	    {
		sprintf(buf, "Спасибо вам %d-м.", count);
		is_group = TRUE;
	    }
	    else
	    {
		sprintf(buf, "Спасибо, %s!", ch->name);
		is_group = FALSE;
	    }

	    do_say(questman, buf);
	    generate_quest(ch, questman, is_group);

	    return;
	}
	else if (!strcmp(arg1, "complete") || !strcmp(arg1, "готово"))
	{
	    act("$n сообщает $N2 об окончании квеста.", ch, NULL, questman, TO_ROOM);
	    act("Ты сообщаешь $N2 об окончании квеста.", ch, NULL, questman, TO_CHAR);
	    if (ch->pcdata->questgiver != questman)
	    {
		sprintf(buf, "Хммм... А я и не просил%s тебя ни о чем таком.", SEX_ENDING(questman));
		do_say(questman, buf);
		return;
	    }

	    if (IS_SET(ch->act, PLR_QUESTING))
	    {
		if ((obj = (OBJ_DATA *) check_quest_complete(ch, questman, FALSE)) != NULL)
		{
		    int reward, pointreward, level;

		    level = group_level(ch, AVG_GROUP_LEVEL);

		    if (IS_SET(ch->pcdata->quest_type, QUEST_KILL_MOB))
			pointreward = number_range(level/3, 3 * level / 2);
		    else
			pointreward = number_range(level/4, level + 30 / (UMAX(1, ch->pcdata->qtime - ch->pcdata->countdown)));

		    LIST_FOREACH_SAFE(fch, &ch->in_room->people, room_link, safe_fch)
		    {
			if (ch == fch || (!IS_NPC(fch) && IS_SET(fch->pcdata->quest_type, QUEST_GROUP) && is_same_group(ch, fch))){
			    int pr, bonus = 0;

			    reward = number_range(1, fch->level);

			    pr = UMAX(1, pointreward) * (IS_SET(fch->pcdata->quest_type, QUEST_GROUP) ? UMAX(1, 3 * fch->level/(4 * level)) : 1);


//делаем постоянное утроение
			/*  pr *= 2.75; */
			    pr *= 12;

				/* При УСИЛЕНИЯХ КВЕСТОМ: x1=1; x2=1.4; x3=1.8; x4=2.2; x5=2.6 */
				pr *= (immquest.double_qp-1)/2 + 1 ;


			    if ((fch->count_holy_attacks > 0)||(fch->count_guild_attacks > 0))
			    {
				sprintf(buf, "%s говорит тебе: {RТы выполнил%s квест во славу Богов и Гильдии! Поздравляю тебя!{x\n\r", 
					capitalize(questman->short_descr), SEX_ENDING(fch));
				pr = 0;
				if (fch->count_holy_attacks > 0)
				{		
				    fch->count_holy_attacks-- ;
				}
				else
				{		
				    fch->count_guild_attacks-- ;
				}
			    }
			    else
			    {		
				sprintf(buf, "%s говорит тебе: {RПоздравляю тебя с завершением квеста!{x\n\r"
					"%s говорит тебе: {RВ награду я даю тебе %d %s удачи и %d золота.{x\n\r", 
					capitalize(questman->short_descr),
					capitalize(questman->short_descr),
					pr, hours(pr, TYPE_POINTS),
					reward);
				if ((fch->classid == CLASS_ALCHEMIST && number_percent() >= 50) || IS_IMMORTAL(fch)){
				    int rec, mrec, is;
				    RECIPE_DATA *recipe;

				    mrec = 0;				
				    for (recipe  = recipe_list;	recipe != NULL; recipe  = recipe->next)
					mrec++;

				    rec = number_range(1,mrec);
				    recipe = get_recipe(rec);
				    is = 0;


				    while ((recipe != NULL && recipe->rlevel == 1) || is < 20){
						rec = number_range(1,mrec);
						recipe = get_recipe(rec);
						is++;
				    }			    

				    if (recipe != NULL && recipe->value[0] != 0 && fch->level >= recipe->level){
					/* && number_percent() > 12*recipe->rlevel) */
						sprintf(buf,"%s%s говорит тебе: {RПовезло тебе, у меня тут рецептик завалялся! Сейчас покажу...{x\n\r"
							"%s говорит тебе: {RВот, смотри, написано %s.{x\n\r"
							"%s говорит тебе: {RИнгредиенты такие. ",
							buf,
							capitalize(questman->short_descr),
							capitalize(questman->short_descr),
							recipe->name,
							capitalize(questman->short_descr));

						for (i = 0; i != MAX_ING; i++)
							if (recipe->value[i] != 0)
							sprintf(buf, "%s%s. ",buf, capitalize(get_name_ing(recipe->value[i])));

						sprintf(buf, "%s{x\n\r",buf);
				    }
				}
			    }

			    send_to_char(buf, fch);

			    if (fch->desc && fch->desc->out_compress && pr >0 )
			    {
				bonus = UMAX(1, bonus/5);
				sprintf(buf, "%s говорит тебе: {RКроме того, за то, что ты экономишь ресурсы сервера, используя компрессию, я даю тебе еще %d %s удачи.{x\n\r", 
					capitalize(questman->short_descr), bonus, hours(bonus, TYPE_POINTS));
				send_to_char(buf, fch);
				pr += bonus;
			    }

			    if (IS_SET(fch->pcdata->quest_type, QUEST_GET_OBJ))
				extract_obj(obj, TRUE, TRUE);

			    end_quest(fch, TO_NEXT_QUEST);
			    fch->gold += reward;
			    fch->pcdata->quest_curr += pr;
			    fch->pcdata->quest_accum += pr;
			}
		    }
		    if (check_clan_group(ch) && IS_SET(ch->pcdata->quest_type, QUEST_GROUP))
			clan_table[ch->clan].rating += pointreward/2;
		}
		else if ((ch->pcdata->questmob > 0 || ch->pcdata->questobj > 0)
			 && ch->pcdata->countdown > 0)
		{
		    sprintf(buf, "Ты еще не закончил%s квест, но время у тебя еще есть!", SEX_ENDING(ch));
		    do_say(questman, buf);
		}

		return;    
	    }
	    if (ch->pcdata->nextquest > 0)
		strcpy(buf,"Погоди маленько, дам я тебе вскорости квест!");
	    else
		strcpy(buf, "Cначала запроси квест.");
	    do_say(questman, buf);
	    return;
	}
	else if (!strcmp(arg1, "quit") || !strcmp(arg1, "выход"))
	{
	    void *tmp;

	    act("$n информирует $N1 о том, что хочет выйти из квеста.", ch, NULL, questman, TO_ROOM);
	    act("Ты сообщаешь $N2 о том, что хочешь выйти из квеста.", ch, NULL, questman, TO_CHAR);

	    if (ch->pcdata->questgiver != questman)
	    {
		sprintf(buf, "Я не давал%s тебе квест! Может быть это сделал кто-то другой?", SEX_ENDING(questman));
		do_say(questman, buf);
		return;
	    }

	    if ((tmp = check_quest_complete(ch, questman, TRUE)) != NULL)
	    {
		if (tmp != (void *) questman)
		    do_say(questman, "Ты ничего не путаешь? Квест вроде бы выполнен, а ты хочешь выйти из него.");

		return;
	    }

	    if (IS_SET(ch->act, PLR_QUESTING))
	    {
		int loose;

		LIST_FOREACH_SAFE(fch, &ch->in_room->people, room_link, safe_fch)
		{
		    if (ch == fch || (!IS_NPC(fch) && IS_SET(fch->pcdata->quest_type, QUEST_GROUP) 
				      && is_same_group(ch, fch)))
		    {
			end_quest(fch, 2*TO_NEXT_QUEST);

			loose = number_range(1, 10);

			if (immquest.is_quest)
			    loose = 2 * loose * immquest.double_qp / 3;

			fch->pcdata->quest_curr -= loose;
			fch->pcdata->quest_accum -= loose;

			sprintf(buf, "%s говорит тебе: {RТвой квест окончен. Ты теряешь %d %s удачи.{x\n\r"
				"%s говорит тебе: {RКроме того, до следующего квеста тебе нужно подождать %d ",
				capitalize(questman->short_descr), loose, hours(loose, TYPE_POINTS),
				capitalize(questman->short_descr), 2*TO_NEXT_QUEST);
			send_to_char(buf, fch);
			sprintf(buf, "%s.{x\n\r", hours(2*TO_NEXT_QUEST, TYPE_MINUTE));
			send_to_char(buf, fch);
		    }
		}

		return;
	    }
	    else
	    {
		send_to_char("Но ты не выполняешь никаких квестов!", ch);
		return;
	    }
	}
    }
    send_to_char("Доступные команды: инфо, время, запросить, готово, выход, список, купить, счет, выключить, включить, бессмертие, посмертие, артефакт.\n\r", ch);
    send_to_char("Для подробной информации набери '? квест'.\n\r", ch);
    return;
}

void generate_quest(CHAR_DATA *ch, CHAR_DATA *questman, bool is_group)
{
    CHAR_DATA *victim = NULL, *fch;
    MOB_INDEX_DATA *vsearch;
    ROOM_INDEX_DATA *room = NULL;
    OBJ_DATA *questitem;
    char buf [MAX_STRING_LENGTH];
    int mob_vnum, mcounter, level, qtime;

    if (ch == NULL)
	return;

    level = is_group ? group_level(ch, MAX_GROUP_LEVEL) : ch->level;
    Show_output = FALSE;
    for (mcounter = 0; mcounter < 2500; mcounter ++)
    {
	mob_vnum = number_range(100, top_vnum_mob);

	if ((vsearch = get_mob_index(mob_vnum)) != NULL
	    && vsearch->area != NULL
	    && !IS_SET(vsearch->area->area_flags, AREA_NA)
	    && !IS_SET(vsearch->area->area_flags, AREA_TESTING)
	    && !IS_SET(vsearch->area->area_flags, AREA_NOQUEST))
	{	    
	    if ((victim = get_char_world( ch, vsearch->player_name)) != NULL
		&& victim->pIndexData == vsearch
		&& quest_level_diff(level, victim->level)
		&& !IS_SET(victim->act, ACT_PET)
		&& !IS_SET(victim->act, ACT_MOUNT)
		&& !IS_SET(victim->affected_by, AFF_CHARM)
		&& ((IS_EVIL(ch) && !IS_EVIL(victim)) || (IS_GOOD(ch) && !IS_GOOD(victim)) || IS_NEUTRAL(ch))
		&& (room = victim->in_room) != NULL
		&& can_see_room(ch, room)
		&& !IS_SET(room->room_flags, ROOM_KILL)
		&& !IS_SET(room->room_flags, ROOM_SLOW_DT)
		&& !IS_SET(room->room_flags, ROOM_NOFLY_DT)
		&& !IS_SET(room->room_flags, ROOM_PRIVATE)
		&& !IS_SET(room->room_flags, ROOM_SOLITARY)
		&& !IS_SET(room->room_flags, ROOM_GUILD)
		&& !IS_SET(room->room_flags, ROOM_HOLY)
		&& !IS_SET(room->room_flags, ROOM_SAFE)
		&& !IS_SET(room->room_flags, ROOM_HOUSE)
		&& !IS_SET(room->room_flags, ROOM_ARENA)
		&& !IS_SET(room->room_flags, ROOM_ARENA_MONITOR)
		&& !IS_SET(room->room_flags, ROOM_NOQUEST)
		&& !IS_SET(room->area->area_flags, AREA_NA)
		&& !IS_SET(room->area->area_flags, AREA_TESTING)
		&& !IS_SET(room->area->area_flags, AREA_NOQUEST)
		&& !is_safe(ch, victim)
		&& room->sector_type != SECT_WATER_SWIM
		&& room->sector_type != SECT_WATER_NOSWIM
		&& (!is_group || IS_SET(victim->affected_by, AFF_SANCTUARY) 
		    || IS_SET(victim->affected_by, AFF_HASTE))
		&& number_bits(1) == 0)
		{
		    break;
		}
	    else
		vsearch = NULL;
	}
    }

    Show_output = TRUE;

    if (vsearch == NULL || victim == NULL || room == NULL || !IS_NPC(victim))
    {

	sprintf(buf, "Извини%s, у меня нет сейчас квестов для %s.",
		is_group ? "те" : "", is_group ? "вас" : "тебя");
	do_say(questman, buf);
	sprintf(buf, "Попробуй%s подойти попозже.", is_group ? "те" : "");
	do_say(questman, buf);

	LIST_FOREACH(fch, &ch->in_room->people, room_link)
	{
	    if (!IS_NPC(fch) && is_same_group(fch, ch))
		fch->pcdata->nextquest = TO_NEXT_QUEST/3;
	}

	return;
    }

    qtime = number_range(10, 30);

    LIST_FOREACH(fch, &ch->in_room->people, room_link)
    {
	if (!IS_NPC(fch) && is_same_group(fch, ch))
	{
	    fch->pcdata->countdown = fch->pcdata->qtime = qtime;
	    fch->pcdata->questgiver = questman;
	    SET_BIT(fch->act, PLR_QUESTING);
	    fch->pcdata->quest_type = 0;
	}
    }

    /*  60% chance it will send the player on a 'recover item' quest. */
    if (!is_group && number_percent() < 60)
    {
	int descnum;

	for (descnum=0;quest_desc[descnum].name != NULL;descnum++)
	    ;

	descnum = number_range(0, descnum - 1);
	questitem = create_object( get_obj_index(QUEST_OBJ), ch->level );

	if (questitem->short_descr)
	    free_string(questitem->short_descr);
	if (questitem->owner)
	    free_string(questitem->owner);
	if (questitem->description)
	    free_string(questitem->description);
	if (questitem->name)
	    free_string(questitem->name);
	if (ch->pcdata->questobjname)
	    free_string(ch->pcdata->questobjname);

	questitem->name        = str_dup(quest_desc[descnum].name);
	questitem->owner       = str_dup(ch->name);
	questitem->description = str_dup(quest_desc[descnum].long_descr);
	questitem->short_descr = str_dup(quest_desc[descnum].short_descr);
	questitem->timer       = qtime + 1;
	ch->pcdata->questobjname = str_dup(quest_desc[descnum].short_descr);

	ch->pcdata->questobj = questitem->pIndexData->vnum;
	sprintf(buf, "Подлые воры украли %s из королевской сокровищницы!", cases(questitem->short_descr, 6));
	do_say(questman, buf);
	do_say(questman, "Придворный маг определил, где эта вещь теперь.");

	sprintf(buf, "Ищи это в землях под названием %s в местечке %s!",
		room->area->name, room->name);
	do_say(questman, buf);

	SET_BIT(ch->pcdata->quest_type, QUEST_GET_OBJ);

	if (number_percent() < 30 || is_animal(victim))
	    obj_to_room(questitem, room);
	else
	{
	    obj_to_char(questitem, victim);
	    sprintf(buf, "Эта вещь находится в руках у %s!", cases(victim->short_descr, 1));
	    do_say(questman, buf);
	    SET_BIT(ch->pcdata->quest_type, QUEST_KILL_MOB);
	}
    }
    /* Quest to kill a mob */
    else
    {
	GROUP_QUEST_DATA *gq = NULL;

	if (is_animal(victim))
	{
	    sprintf(buf, "Поступили слухи, что %s заражен%s бешенством, и уже многие пострадали от этого.",
		    victim->short_descr, SEX_ENDING(victim));
	    do_say(questman, buf);
	    sprintf(buf, "Убей%s эту тварь!", is_group ? "те" : "");
	    do_say(questman, buf);
	}
	else switch(number_range(0, 1))
	{
	case 0:
	    sprintf(buf, "%s задумал%s подлый заговор против правительства.",
		    victim->short_descr, SEX_ENDING(victim));
	    buf[0] = UPPER(buf[0]);
	    do_say(questman, buf);
	    sprintf(buf, "Не дай%s этому заговору свершиться!", is_group ? "те" : "");
	    do_say(questman, buf);
	    break;

	case 1:
	    sprintf(buf, "Гнусный преступник, %s, сбежал%s из тюрьмы!", victim->short_descr, SEX_ENDING(victim));
	    do_say(questman, buf);
+
	    sprintf(buf, "С тех пор %s убил%s уже %d мирных жителей!", victim->short_descr, SEX_ENDING(victim), number_range(5, 30));
	    do_say(questman, buf);
	    sprintf(buf, "Наказание за эти преступления - СМЕРТЬ! Иди%s!",
		    is_group ? "те и будьте палачами" : " и будь палачом");
	    do_say(questman, buf);
	    break;
	}

	if (room->name != NULL)
	{
	    sprintf(buf, "Ищи%s %s где-то поблизости от местечка %s!",
		    is_group ? "те" : "", cases(victim->short_descr, 3), room->name);
	    do_say(questman, buf);

	    sprintf(buf, "Это находится в землях под названием %s.",
		    room->area->name);

	    do_say(questman, buf);
	}
	if (IS_SET(victim->affected_by, AFF_HIDE)
	    || IS_SET(victim->affected_by, AFF_INVISIBLE)
	    || IS_SET(victim->affected_by, AFF_CAMOUFLAGE))
	{
	    sprintf(buf, "Но учти%s - %s, похоже, прячется от глаз.",
		    is_group ? "те" : "", victim->short_descr);
	    do_say(questman, buf);
	}

	if (is_group)
	{
	    gq = new_group_quest();

	    gq->next = questman->group_quest;
	    questman->group_quest = gq;
	    gq->time = qtime;
	}

	LIST_FOREACH(fch, &ch->in_room->people, room_link)
	{
	    if (!IS_NPC(fch) && is_same_group(ch, fch))
	    {
		fch->pcdata->questmob = victim->pIndexData->vnum;
		SET_BIT(fch->pcdata->quest_type, QUEST_KILL_MOB);
		if (is_group)
		{
		    NAME_LIST *nl = new_name_list();

		    SET_BIT(fch->pcdata->quest_type, QUEST_GROUP);

		    nl->next = gq->names;
		    gq->names = nl;
		    nl->name = str_dup(fch->name);
		}
	    }
	}
    }

    if (is_group)
	log_group_quest(questman, "generate_quest");

    sprintf(buf, "У %s есть %d %s для завершения этого квеста.",
	    is_group ? "вас" : "тебя",
	    qtime, hours(qtime, TYPE_MINUTES));
    do_say(questman, buf);
    sprintf(buf, "Да пребудет с %s великая сила!",
	    is_group ? "вами" : "тобой");
    do_say(questman, buf);

    return;
}

bool quest_level_diff(int clevel, int mlevel)
{
    int sq;

    sq = (int) sqrt(clevel);

    if (mlevel < clevel + sq && mlevel > clevel - sq)
	return TRUE;
    else
	return FALSE;
}

/* Called from update_handler() by pulse_area */

void q_update(CHAR_DATA *ch)
{
    if (IS_NPC(ch))
    {
	if (IS_SET(ch->act, ACT_QUESTER))
	{
	    GROUP_QUEST_DATA *gq, *gq_next, *gq_prev = NULL;
	    int i = 0;

	    for (gq = ch->group_quest; gq; gq = gq_next)
	    {
		gq_next = gq->next;

		i++;

		if (!IS_VALID(gq))
		{
		    bugf("Trying to work with invalidated group_quest (%d).", i);
		    continue;
		}

		if (--gq->time <= 0)
		{
		    if (gq_prev)
			gq_prev->next = gq_next;
		    else	
			ch->group_quest = gq_next;

		    free_group_quest(gq);
		}
		else
		    gq_prev = gq;
	    }

	    log_group_quest(ch, "q_update");
	}
	return;
    }

    if (ch->pcdata->nextquest > 0)
    {
	ch->pcdata->nextquest--;
	if (ch->pcdata->nextquest == 0)
	{
	    send_to_char("Ты теперь снова сможешь получить квест.\n\r", ch);
	    return;
	}
    }
    else if (IS_SET(ch->act, PLR_QUESTING))
    {
	if (--ch->pcdata->countdown <= 0)
	{
	    int loose;
	    char buf[MAX_STRING_LENGTH];

	    loose = number_range(2, 25);
	    sprintf(buf,"Время твоего квеста вышло!\n\rТы теряешь %d %s удачи!\n\r",
		    loose,
		    hours(loose, TYPE_POINTS));
	    send_to_char(buf, ch);

	    sprintf(buf, "Для того, чтобы запросить следующий квест, тебе нужно подождать %d %s.\n\r",
		    2 * TO_NEXT_QUEST,
		    hours(2 * TO_NEXT_QUEST, TYPE_MINUTES));
	    send_to_char(buf, ch);
	    end_quest(ch, 2 * TO_NEXT_QUEST);

	    if (immquest.is_quest)
		loose *= immquest.double_qp;

	    ch->pcdata->quest_curr -= loose;
	    ch->pcdata->quest_accum -= loose;
	    return;
	}

	if (ch->pcdata->countdown < 6)
	{
	    send_to_char("Быстрее, тебе надо закончить квест!\n\r", ch);
	    return;
	}
    }
}

void quest_update(CHAR_DATA *ch)
{
    CHAR_DATA *safe_ch;
    
    if (ch)
	q_update(ch);
    else
	LIST_FOREACH_SAFE(ch, &char_list, link, safe_ch)
	    q_update(ch);

    return;
}

void do_loadquester(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj;
    OBJ_INDEX_DATA *pObjIndex = NULL;
    char buf[MAX_STRING_LENGTH];

    one_argument(argument, buf);

    if (!str_prefix(buf, "медный"))
	pObjIndex = get_obj_index(QUEST_COPPER);
    else if (!str_prefix(buf, "бронзовый"))
	pObjIndex = get_obj_index(QUEST_BRONZE);
    else if (!str_prefix(buf, "серебряный"))
	pObjIndex = get_obj_index(QUEST_SILVER);
    else if (!str_prefix(buf, "золотой"))
	pObjIndex = get_obj_index(QUEST_GOLD);
    else if (!str_prefix(buf, "платиновый"))
	pObjIndex = get_obj_index(QUEST_PLATIN);

    if (pObjIndex != NULL)
    {
	obj = create_object(pObjIndex, 0);
	obj_to_char(obj, ch);
	send_to_char("Ok.\n\r", ch);
	return;
    }

    send_to_char("Таких квестеров нет.\n\r", ch);
}

/* charset=cp1251 */

