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
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <zlib.h>

#include "merc.h"
#include "recycle.h"

#include <dirent.h>





extern  int     _filbuf         args((FILE *));


int rename(const char *oldfname, const char *newfname);

/*
 * Array of containers read for proper re-nesting of objects.
 */
#define MAX_NEST	100
static	OBJ_DATA *	rgObjNest	[MAX_NEST];

//int sobj_out  = 0;
//int sobj_in   = 0;

/*
 * Local functions.
 */
void	fwrite_char	args((CHAR_DATA *ch,  FILE *fp));
void	fwrite_obj	args((CHAR_DATA *ch,  OBJ_DATA  *obj,
			      FILE *fp, int iNest));
void	fwrite_pet	args((CHAR_DATA *pet, FILE *fp));
void	fread_char	args((CHAR_DATA *ch,  FILE *fp));
void    fread_pet	args((CHAR_DATA *ch,  FILE *fp, bool is_pet));
void	fread_obj	args((CHAR_DATA *ch,  FILE *fp, bool equip));

double get_explore_percent(CHAR_DATA *ch);

/*
 * Save a character and inventory.
 * Would be cool to save NPC's too for quest purposes,
 *   some of the infrastructure is provided.
 */
void save_char_obj(CHAR_DATA *ch, bool inval){
    char strsave[MAX_INPUT_LENGTH];
    FILE *fp;
    bool status;

    if (IS_NPC(ch))
	return;

    //
    // Don't save if the character is invalidated.
    // This might happen during the auto-logoff of players.
    // (or other places not yet found out)
    //

    if (IS_VALID(ch))
        status = TRUE;
    else
    {
        status = FALSE;
        if (inval)
	    VALIDATE(ch);
    }

    if (!IS_VALID(ch)){
		bugf("Save_char_obj: Trying to save an invalidated character.");
		return;
    }

    if (ch->desc != NULL && ch->desc->original != NULL)
	ch = ch->desc->original;

    /* create god log */
    if (IS_IMMORTAL(ch) || ch->level >= LEVEL_IMMORTAL){

		sprintf(strsave, "%s%s",GOD_DIR, capitalize(ch->name));
		if ((fp = file_open(strsave,"w")) == NULL){
			bugf("Save_char_obj: file_open(\"%s\")", strsave);
			_perror(strsave);
		} else{
			fprintf(fp,"Lev %2d Trust %2d  %s%s\n", ch->level, get_trust(ch), ch->name, ch->pcdata->title);

		}

		file_close(fp);
    }


    sprintf(strsave, "%s%s", PLAYER_DIR, capitalize(ch->name));

/*    sprintf(strsave, "%s%s/%s", PLAYER_DIR, ch->pcdata->email, capitalize(ch->name));*/

    if ((fp = file_open(TEMP_FILE, "w")) == NULL){
		bugf("Save_char_obj: file_open(\"%s\")", TEMP_FILE);
		_perror(strsave);
    } else {
		bool confirm = FALSE;

		if (IS_SET(ch->act, PLR_CONFIRM_DELETE)){
			REMOVE_BIT(ch->act, PLR_CONFIRM_DELETE);
			confirm = TRUE;
		}

		// Сохоаним параметры чара
		fwrite_char(ch, fp);

		if (confirm) SET_BIT(ch->act, PLR_CONFIRM_DELETE);

	//	sobj_out  = 0;
		if (ch->carrying != NULL)
			fwrite_obj(ch, ch->carrying, fp, 0);
	//	ch->pcdata->obj_count_out = sobj_out;

		/* save the pets */
		if (ch->pet != NULL && ch->pet->in_room == ch->in_room){
			fprintf(fp, "#PET\n");
			fwrite_pet(ch->pet,fp);
		}

		/* Сохранять только тех лошадей, которых в мире более 1-го экземпляра */
		if (ch->mount && ch->mount->in_room == ch->in_room && ch->mount->pIndexData && ch->mount->pIndexData->count > 1){
			fprintf(fp, "#MOUNT\n");
			fwrite_pet(ch->mount, fp);
		}

		fprintf(fp, "#END\n");
    }
    file_close(fp);
    rename(TEMP_FILE,strsave);

    if (inval){
        if (status) VALIDATE(ch);
        else INVALIDATE(ch);
    }

    return;
}



/*
 * #Сохранение чара в файл #save #char_save
 */
void fwrite_char(CHAR_DATA *ch, FILE *fp){
    AFFECT_DATA *paf;
    int sn, gn, pos;
    char buf[MSL];
    NAME_LIST *ip;



    fprintf(fp, "#%s\n", IS_NPC(ch) ? "MOB" : "PLAYER");

    fprintf(fp, "Name %s~\n", ch->name);
    fprintf(fp, "Id   %ld\n", ch->id);
    fprintf(fp, "GenIP %s~\n", ch->pcdata->genip);
	fprintf(fp, "Platinum %d\n", ch->pcdata->platinum);
	fprintf(fp, "Bronze %d\n", ch->pcdata->bronze);
    fprintf(fp, "LogO %ld\n", (long)current_time);
    fprintf(fp, "Vers %d\n", 6);
 //  fprintf(fp, "Host %s~\n", ch->pcdata->lasthost);
//    fprintf(fp, "IP   %s~\n", ch->pcdata->lastip);
    fprintf(fp, "Host %s~\n", ch->desc->ip);
    fprintf(fp, "IP   %s~\n", ch->desc->ip);


    if (ch->short_descr[0] != '\0')
	fprintf(fp, "ShD  %s~\n", ch->short_descr);
    if (ch->long_descr[0] != '\0')
	fprintf(fp, "LnD  %s~\n", ch->long_descr);
    if (ch->description[0] != '\0')
	fprintf(fp, "Desc %s~\n", ch->description);
    if (ch->prompt != NULL
	|| !str_cmp(ch->prompt,"<%hhp %mm %vmv> ")
	|| !str_cmp(ch->prompt,"{c<%hhp %mm %vmv>{x "))
    {
	fprintf(fp, "Prom %s~\n", ch->prompt);
    }
    fprintf(fp, "Race %s~\n", pc_race_table[ch->race].name);
    if (is_clan(ch))
	fprintf(fp, "Clan %s~\n", clan_table[ch->clan].name);
    if (ch->war_command != NULL && ch->war_command[0] != '\0')
	fprintf(fp, "WarC %s~\n", ch->war_command);
    fprintf(fp, "WarOn %d\n", ch->war_command_on);
    //команда при реколле
    if (ch->lost_command != NULL && ch->lost_command[0] != '\0')
	fprintf(fp, "LostC %s~\n", ch->lost_command);
    //контрольный вопрос/ответ
    if (ch->pcdata->reg_answer != NULL && ch->pcdata->reg_answer[0] != '\0')
	fprintf(fp, "RAnswer %s~\n", ch->pcdata->reg_answer);

    fprintf(fp, "Sex  %d\n", ch->sex);

//    fprintf(fp, "ObjCountOut  %d\n", ch->pcdata->obj_count_out);

    fprintf(fp, "Cla  %d\n", ch->classid);
    fprintf(fp, "Levl %d\n", ch->level);
    fprintf(fp, "MLvl %d\n", ch->pcdata->max_level);
    fprintf(fp, "Count  %d\n", ch->pcdata->count_entries_errors);
    if (ch->trust != 0)
	fprintf(fp, "Tru  %d\n", ch->trust);
    fprintf(fp, "Sec  %d\n",    ch->pcdata->security);	/* OLC */

    if (!IS_NULLSTR(ch->pcdata->grants))
	fprintf(fp, "Grants %s~\n", ch->pcdata->grants);

    if (!IS_NULLSTR(ch->pcdata->pretitle))
	fprintf(fp, "PreTitle %s~\n", ch->pcdata->pretitle);

    fprintf(fp, "Plyd %ld\n", (long)(((current_time - ch->logon) < 0) ? ch->played : ch->played + (current_time - ch->logon)));
    //поставить проверку - проверочное время меньше или нет...

    fprintf(fp, "Atheist  %d\n", ch->pcdata->atheist);

    fprintf(fp, "Not  %ld %ld %ld %ld %ld %ld\n",
	    (long)ch->pcdata->last_note,
	    (long)ch->pcdata->last_idea,
	    (long)ch->pcdata->last_penalty,
	    (long)ch->pcdata->last_news,
	    (long)ch->pcdata->last_changes,
	    (long)ch->pcdata->last_votes);
    fprintf(fp, "Scro %d\n", ch->lines);
    fprintf(fp, "Kill %d %d\n", ch->pcdata->kills_mob, ch->pcdata->kills_pc);
    fprintf(fp, "Successed %d\n", ch->pcdata->successed);
    fprintf(fp, "Deat %d %d\n", ch->pcdata->deaths_mob, ch->pcdata->deaths_pc);
    fprintf(fp, "Flee %d\n", ch->pcdata->flees);

    fprintf(fp, "Quest_Curr    %d\n", ch->pcdata->quest_curr);
    fprintf(fp, "Quest_Accum  %d\n", ch->pcdata->quest_accum);

    if (ch->pcdata->nextquest != 0)
	fprintf(fp, "QuestNext   %d\n", ch->pcdata->nextquest);

    if (ch->pcdata->countdown != 0)
	fprintf(fp, "QCountDown   %d\n", ch->pcdata->countdown);

    if (ch->pcdata->questgiver != NULL)
	fprintf(fp, "QuestGiver   %d\n",
		ch->pcdata->questgiver->pIndexData->vnum);

    if (ch->pcdata->questmob != 0)
	fprintf(fp, "QuestMob   %d\n", ch->pcdata->questmob);

    if (ch->pcdata->questobj != 0)
	fprintf(fp, "QuestObj   %d\n", ch->pcdata->questobj);

    if (ch->pcdata->quest_type != 0)
	fprintf(fp, "QuestType   %d\n", ch->pcdata->quest_type);
    
	if (ch->pcdata->temp_RIP != 0)
	fprintf(fp, "TempRIP   %d\n", ch->pcdata->temp_RIP);


    fprintf(fp, "Room %d\n",
	    (ch->in_room == get_room_index(ROOM_VNUM_LIMBO)
	     && ch->was_in_room != NULL)
	    ? ch->was_in_room->vnum
	    : ch->in_room == NULL ? 3001 : ch->in_room->vnum);

    if (!IS_NULLSTR(ch->pcdata->email))
	fprintf(fp, "Email %s~\n", ch->pcdata->email);

    fprintf(fp, "HolyC %d\n", ch->count_holy_attacks);
    fprintf(fp, "GuildC %d\n", ch->count_guild_attacks);

    if (!IS_NULLSTR(ch->pcdata->email_key))
	fprintf(fp, "EmailKey %s~\n", ch->pcdata->email_key);

    if (!IS_NULLSTR(ch->pcdata->old_names))
	fprintf(fp, "ONames %s~\n", ch->pcdata->old_names);

    if(!IS_NULLSTR(ch->pcdata->spouse))
	fprintf(fp, "Spou %s~\n", ch->pcdata->spouse);

    if (IS_SET(ch->act, PLR_FREEZE))
    {
	fprintf(fp, "Freeze %ld\n", (long)ch->pcdata->freeze);
	fprintf(fp, "FWho %s~\n", ch->pcdata->freeze_who);
    }

    if (check_channels(ch))
	fprintf(fp, "NoChan %ld\n", (long)ch->pcdata->nochan);

    if (IS_SET(ch->comm, COMM_NONOTES))
	fprintf(fp, "NoNote %ld\n", (long)ch->pcdata->nonotes);

    if (IS_SET(ch->comm, COMM_NOTITLE))
	fprintf(fp, "NoTitl %ld\n", (long)ch->pcdata->notitle);

    if (ch->pcdata->beacon != NULL && ch->pcdata->beacon->in_room != NULL)
    {
	fprintf(fp, "BeacRoom %d\n", ch->pcdata->beacon->in_room->vnum);
	fprintf(fp, "BeacTime %d\n", ch->pcdata->beacon->timer);
    }

    if (ch->style != 0)
	fprintf(fp, "Style %d\n", ch->style);

    fprintf(fp, "ADam %d\n", ch->pcdata->avg_dam);

    fprintf(fp, "HMV  %d %d %d %d %d %d\n",
	    ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move);
    if (ch->gold > 0)
	fprintf(fp, "Gold %ld\n",	ch->gold		);
    else
	fprintf(fp, "Gold %d\n", 0			);
    if (ch->silver > 0)
	fprintf(fp, "Silv %ld\n",ch->silver		);
    else
	fprintf(fp, "Silv %d\n",0			);
    fprintf(fp, "Exp  %d\n",	ch->exp			);
    if (ch->act != 0)
	fprintf(fp, "Act  %s\n",   fwrite_flag(ch->act));
    /*    if (ch->affected_by != 0)
     fprintf(fp, "AfBy %s\n",   fwrite_flag(ch->affected_by)); */
    fprintf(fp, "Comm %s\n",       fwrite_flag(ch->comm));
    if (ch->wiznet)
	fprintf(fp, "Wizn %s\n",   fwrite_flag(ch->wiznet));
    if (ch->invis_level)
	fprintf(fp, "Invi %d\n", 	ch->invis_level	);
    if (ch->incog_level)
	fprintf(fp,"Inco %d\n",ch->incog_level);
    fprintf(fp, "Pos  %d\n",
	    (POS_FIGHT(ch)) ? POS_STANDING : ch->position);
    if (ch->practice != 0)
	fprintf(fp, "Prac %d\n",	ch->practice	);
    if (ch->train != 0)
	fprintf(fp, "Trai %d\n",	ch->train	);
    if (ch->saving_throw != 0)
	fprintf(fp, "Save  %d\n",	ch->saving_throw);
    fprintf(fp, "Alig  %d\n",	ch->alignment		);
    fprintf(fp, "OAlg  %d\n",	ch->pcdata->orig_align	);
    if (ch->hitroll != 0)
	fprintf(fp, "Hit   %d\n",	ch->hitroll	);
    if (ch->damroll != 0)
	fprintf(fp, "Dam   %d\n",	ch->damroll	);
    fprintf(fp, "ACs %d %d %d %d\n",
	    ch->armor[0],ch->armor[1],ch->armor[2],ch->armor[3]);
    if (ch->wimpy !=0)
	fprintf(fp, "Wimp  %d\n",	ch->wimpy	);
    fprintf(fp, "Attr %d %d %d %d %d\n",
	    ch->perm_stat[STAT_STR],
	    ch->perm_stat[STAT_INT],
	    ch->perm_stat[STAT_WIS],
	    ch->perm_stat[STAT_DEX],
	    ch->perm_stat[STAT_CON]);

    fprintf (fp, "AMod %d %d %d %d %d\n",
	     ch->mod_stat[STAT_STR],
	     ch->mod_stat[STAT_INT],
	     ch->mod_stat[STAT_WIS],
	     ch->mod_stat[STAT_DEX],
	     ch->mod_stat[STAT_CON]);

    if (IS_NPC(ch)){
	    fprintf(fp, "Vnum %d\n",	ch->pIndexData->vnum	);
    } else {
        fprintf(fp, "Pass %s~\n",	ch->pcdata->pwd		);
        if (ch->pcdata->bamfin[0] != '\0')
            fprintf(fp, "Bin  %s~\n",	ch->pcdata->bamfin);
        if (ch->pcdata->bamfout[0] != '\0')
            fprintf(fp, "Bout %s~\n",	ch->pcdata->bamfout);
        fprintf(fp, "Titl %s~\n",	ch->pcdata->title	);
        fprintf(fp, "Bounty %d\n",	ch->pcdata->bounty );
        fprintf(fp, "Pnts %d\n",   	ch->pcdata->points     );
        fprintf(fp, "TSex %d\n",	ch->pcdata->true_sex	);
        fprintf(fp, "LLev %d\n",	ch->pcdata->last_level	);
        fprintf(fp, "Home %d\n",	ch->pcdata->vnum_house	);
        fprintf(fp, "HMVP %d %d %d\n", ch->pcdata->perm_hit,
            ch->pcdata->perm_mana,
            ch->pcdata->perm_move);

        fprintf(fp, "TrainHM %d %d\n", ch->pcdata->train_hit, ch->pcdata->train_mana);

        fprintf(fp, "Cnd  %d %d %d %d\n",
            ch->pcdata->condition[0],
            ch->pcdata->condition[1],
            ch->pcdata->condition[2],
            ch->pcdata->condition[3]);

        /*
         * Write Colour Config Information.
         */
    #if 1
        fprintf(fp, "Coloura     %d%d%d %d%d%d %d%d%d %d%d%d %d%d%d\n",
            ch->pcdata->text[2],
            ch->pcdata->text[0],
            ch->pcdata->text[1],
            ch->pcdata->auction[2],
            ch->pcdata->auction[0],
            ch->pcdata->auction[1],
            ch->pcdata->gossip[2],
            ch->pcdata->gossip[0],
            ch->pcdata->gossip[1],
            ch->pcdata->music[2],
            ch->pcdata->music[0],
            ch->pcdata->music[1],
            ch->pcdata->question[2],
            ch->pcdata->question[0],
            ch->pcdata->question[1]);
        fprintf(fp, "Colourb     %d%d%d %d%d%d %d%d%d %d%d%d %d%d%d\n",
            ch->pcdata->answer[2],
            ch->pcdata->answer[0],
            ch->pcdata->answer[1],
            ch->pcdata->quote[2],
            ch->pcdata->quote[0],
            ch->pcdata->quote[1],
            ch->pcdata->quote_text[2],
            ch->pcdata->quote_text[0],
            ch->pcdata->quote_text[1],
            ch->pcdata->immtalk_text[2],
            ch->pcdata->immtalk_text[0],
            ch->pcdata->immtalk_text[1],
            ch->pcdata->immtalk_type[2],
            ch->pcdata->immtalk_type[0],
            ch->pcdata->immtalk_type[1]);
        fprintf(fp, "Colourc     %d%d%d %d%d%d %d%d%d %d%d%d %d%d%d\n",
            ch->pcdata->info[2],
            ch->pcdata->info[0],
            ch->pcdata->info[1],
            ch->pcdata->tell[2],
            ch->pcdata->tell[0],
            ch->pcdata->tell[1],
            ch->pcdata->reply[2],
            ch->pcdata->reply[0],
            ch->pcdata->reply[1],
            ch->pcdata->gtell_text[2],
            ch->pcdata->gtell_text[0],
            ch->pcdata->gtell_text[1],
            ch->pcdata->gtell_type[2],
            ch->pcdata->gtell_type[0],
            ch->pcdata->gtell_type[1]);
        fprintf(fp, "Colourd     %d%d%d %d%d%d %d%d%d %d%d%d %d%d%d\n",
            ch->pcdata->room_title[2],
            ch->pcdata->room_title[0],
            ch->pcdata->room_title[1],
            ch->pcdata->room_text[2],
            ch->pcdata->room_text[0],
            ch->pcdata->room_text[1],
            ch->pcdata->room_exits[2],
            ch->pcdata->room_exits[0],
            ch->pcdata->room_exits[1],
            ch->pcdata->room_things[2],
            ch->pcdata->room_things[0],
            ch->pcdata->room_things[1],
            ch->pcdata->prompt[2],
            ch->pcdata->prompt[0],
            ch->pcdata->prompt[1]);
        fprintf(fp, "Coloure     %d%d%d %d%d%d %d%d%d %d%d%d %d%d%d\n",
            ch->pcdata->fight_death[2],
            ch->pcdata->fight_death[0],
            ch->pcdata->fight_death[1],
            ch->pcdata->fight_yhit[2],
            ch->pcdata->fight_yhit[0],
            ch->pcdata->fight_yhit[1],
            ch->pcdata->fight_ohit[2],
            ch->pcdata->fight_ohit[0],
            ch->pcdata->fight_ohit[1],
            ch->pcdata->fight_thit[2],
            ch->pcdata->fight_thit[0],
            ch->pcdata->fight_thit[1],
            ch->pcdata->fight_skill[2],
            ch->pcdata->fight_skill[0],
            ch->pcdata->fight_skill[1]);
        fprintf(fp, "Colourf     %d%d%d %d%d%d %d%d%d %d%d%d %d%d%d\n",
            ch->pcdata->wiznet[2],
            ch->pcdata->wiznet[0],
            ch->pcdata->wiznet[1],
            ch->pcdata->say[2],
            ch->pcdata->say[0],
            ch->pcdata->say[1],
            ch->pcdata->say_text[2],
            ch->pcdata->say_text[0],
            ch->pcdata->say_text[1],
            ch->pcdata->tell_text[2],
            ch->pcdata->tell_text[0],
            ch->pcdata->tell_text[1],
            ch->pcdata->reply_text[2],
            ch->pcdata->reply_text[0],
            ch->pcdata->reply_text[1]);
        fprintf(fp, "Colourg     %d%d%d %d%d%d %d%d%d %d%d%d %d%d%d\n",
            ch->pcdata->auction_text[2],
            ch->pcdata->auction_text[0],
            ch->pcdata->auction_text[1],
            ch->pcdata->gossip_text[2],
            ch->pcdata->gossip_text[0],
            ch->pcdata->gossip_text[1],
            ch->pcdata->music_text[2],
            ch->pcdata->music_text[0],
            ch->pcdata->music_text[1],
            ch->pcdata->question_text[2],
            ch->pcdata->question_text[0],
            ch->pcdata->question_text[1],
            ch->pcdata->answer_text[2],
            ch->pcdata->answer_text[0],
            ch->pcdata->answer_text[1]);
    #endif

        /* write alias */
        for (pos = 0; pos < MAX_ALIAS; pos++)
        {
            if (ch->pcdata->alias[pos] == NULL
            ||  ch->pcdata->alias_sub[pos] == NULL)
            break;

            fprintf(fp,"Alias %s %s~\n",ch->pcdata->alias[pos],
                ch->pcdata->alias_sub[pos]);
        }

        for (sn = 0; sn < max_skills; sn++)
            if (!IS_NULLSTR(skill_table[sn].name) && ch->pcdata->learned[sn] != 0)
            fprintf(fp, "Sk %d %d '%s'\n", ch->pcdata->learned[sn], ch->pcdata->over_skill[sn], skill_table[sn].name);

        for (gn = 0; gn < max_groups; gn++)
            if (!IS_NULLSTR(group_table[gn].name) && ch->pcdata->group_known[gn])
            fprintf(fp, "Gr '%s'\n",group_table[gn].name);
    }


    /* ЗАПИСЫВАЕМ ЭФФЕКТЫ ЧАРА */
    for (paf = ch->affected; paf != NULL; paf = paf->next){
	    if (paf->type < 1 || paf->type>= max_skills) continue;

        /* Affc 'camouflage'  0  51  48   0   0 0 1*/
	    fprintf(fp, "Affc '%s' %3d %3d %3d %3d %3d %lu %ld\n",
		skill_table[paf->type].name,
		paf->where,
		paf->level,
		paf->duration,
		paf->modifier,
		paf->location,
		paf->bitvector,
		paf->caster_id
	    );
    }

    
	
    for (pos = 0; pos < 5; pos++){
	    smash_tilde(ch->pcdata->cases[pos]);
	    fprintf(fp, "Case%d %s~\n", pos + 1, ch->pcdata->cases[pos]);
    }

    fprintf(fp, "Bank %ld\n", ch->pcdata->bank);

    buf[0] = '\0';

    for (pos = 0; pos < MAX_FILTERS; pos++) {
        if (ch->pcdata->filter[pos].ch) {
            strcat(buf, ch->pcdata->filter[pos].ch);
            strcat(buf, " ");
        }
    }

    if (buf[0] != '\0')
	fprintf(fp, "Filter %s~\n", buf);

    if (ch->pcdata->ips){
        buf[0] = '\0';

        for(ip = ch->pcdata->ips; ip; ip = ip->next)
        {
            strcat(buf, " ");
            strcat(buf, ip->name);
        }

        fprintf(fp, "AllowedIP%s~\n", buf);
    }

    if (ch->pcdata->visited_size > 0){
        uLong comprLen;
        Bytef *compr;
        int i;

        comprLen = compressBound(ch->pcdata->visited_size);
        compr = calloc(comprLen, 1);

        compress(compr, &comprLen, ch->pcdata->visited, ch->pcdata->visited_size);

        fprintf(fp, "VizitP %d\n", (int) (100 * get_explore_percent(ch)));
        fprintf(fp, "Vizited %ld ", comprLen);

        for (i = 0; i < comprLen; i++)
            fputc(compr[i], fp);

        fprintf(fp, "\n");

        free(compr);
    }

    if (ch->notepad[0] != '\0')
	fprintf(fp, "NPad %s~\n", ch->notepad);

    fprintf(fp, "End\n\n");
    return;
}

/* write a pet */
void fwrite_pet(CHAR_DATA *pet, FILE *fp)
{
    AFFECT_DATA *paf;

    fprintf(fp,"Vnum %d\n",pet->pIndexData->vnum);

    fprintf(fp,"Name %s~\n", pet->name);
    fprintf(fp,"LogO %ld\n", (long)current_time);
    if (pet->short_descr != pet->pIndexData->short_descr)
	fprintf(fp,"ShD  %s~\n", pet->short_descr);
    if (pet->long_descr != pet->pIndexData->long_descr)
	fprintf(fp,"LnD  %s~\n", pet->long_descr);
    if (pet->description != pet->pIndexData->description)
	fprintf(fp,"Desc %s~\n", pet->description);
    if (pet->race != pet->pIndexData->race)
	fprintf(fp,"Race %s~\n", race_table[pet->race].name);
    if (is_clan(pet))
	fprintf(fp, "Clan %s~\n",clan_table[pet->clan].name);
    fprintf(fp,"Sex  %d\n", pet->sex);
    if (pet->level != pet->pIndexData->level)
	fprintf(fp,"Levl %d\n", pet->level);
    fprintf(fp, "HMV  %d %d %d %d %d %d\n",
	    pet->hit, pet->max_hit, pet->mana, pet->max_mana, pet->move, pet->max_move);
    if (pet->gold > 0)
	fprintf(fp,"Gold %ld\n",pet->gold);
    if (pet->silver > 0)
	fprintf(fp,"Silv %ld\n",pet->silver);
    if (pet->exp > 0)
	fprintf(fp, "Exp  %d\n", pet->exp);
    if (pet->act != pet->pIndexData->act)
	fprintf(fp, "Act  %s\n", fwrite_flag(pet->act));
    if (pet->affected_by != pet->pIndexData->affected_by)
	fprintf(fp, "AfBy %s\n", fwrite_flag(pet->affected_by));
    if (pet->comm != 0)
	fprintf(fp, "Comm %s\n", fwrite_flag(pet->comm));
    fprintf(fp,"Pos  %d\n",
	    (POS_FIGHT(pet)) ? POS_STANDING : pet->position);
    if (pet->saving_throw != 0)
	fprintf(fp, "Save %d\n", pet->saving_throw);
    if (pet->alignment != pet->pIndexData->alignment)
	fprintf(fp, "Alig %d\n", pet->alignment);
    if (pet->hitroll != pet->pIndexData->hitroll)
	fprintf(fp, "Hit  %d\n", pet->hitroll);
    if (pet->damroll != pet->pIndexData->damage[DICE_BONUS])
	fprintf(fp, "Dam  %d\n", pet->damroll);
    fprintf(fp, "ACs  %d %d %d %d\n",
	    pet->armor[0],pet->armor[1],pet->armor[2],pet->armor[3]);
    fprintf(fp, "Attr %d %d %d %d %d\n",
	    pet->perm_stat[STAT_STR], pet->perm_stat[STAT_INT],
	    pet->perm_stat[STAT_WIS], pet->perm_stat[STAT_DEX],
	    pet->perm_stat[STAT_CON]);
    fprintf(fp, "AMod %d %d %d %d %d\n",
	    pet->mod_stat[STAT_STR], pet->mod_stat[STAT_INT],
	    pet->mod_stat[STAT_WIS], pet->mod_stat[STAT_DEX],
	    pet->mod_stat[STAT_CON]);

    for (paf = pet->affected; paf != NULL; paf = paf->next)
    {
	if (paf->type < 0 || paf->type >= max_skills)
	    continue;

	fprintf(fp, "Affc '%s' %3d %3d %3d %3d %3d %10ld %ld\n",
		skill_table[paf->type].name,
		paf->where, paf->level, paf->duration, paf->modifier,paf->location,
		paf->bitvector, paf->caster_id);
    }

    fprintf(fp,"End\n");
    return;
}


/*
 * Write an object and its contents.
 */
void fwrite_obj(CHAR_DATA *ch, OBJ_DATA *obj, FILE *fp, int iNest)
{
    EXTRA_DESCR_DATA *ed;
    AFFECT_DATA *paf;

    /*
     * Slick recursion to write lists backwards,
     *   so loading them will load in forwards order.
     */
    if (obj->next_content != NULL){
        if (obj->item_type == ITEM_CHEST && obj->in_room != NULL){
        // Необходимо для того, чтобы в файл не сохранялись объекты
        // с других сундуков. Т.е. первый элемент файла - объект сундук (iNest = 0)
        // У остальных объектов iNest > 0 увеличивается.
        } else {
            fwrite_obj(ch, obj->next_content, fp, iNest);
        }
    }

    /*
     * Castrate storage characters.
     */
    if ((ch && ch->level < obj->level - 2 && obj->item_type != ITEM_CONTAINER)	||   obj->item_type == ITEM_KEY)
	return;


//    sobj_out++;

    fprintf(fp, "#O\n");
    fprintf(fp, "Vnum %d\n",   obj->pIndexData->vnum       );

    if (is_limit(obj) != -1)
	fprintf(fp, "LifeTime %ld\n",	(long)obj->lifetime	    );

    if (!obj->pIndexData->new_format)
	fprintf(fp, "Oldstyle\n");
    if (obj->enchanted)
	fprintf(fp,"Enchanted\n");
    fprintf(fp, "Nest %d\n",	iNest	  	    );

    /* these data are only used if they do not match the defaults */

    if (obj->name != obj->pIndexData->name)
	fprintf(fp, "Name %s~\n",	obj->name		    );
    if (obj->short_descr != obj->pIndexData->short_descr)
	fprintf(fp, "ShD  %s~\n",	obj->short_descr	    );
    if (obj->description != obj->pIndexData->description)
	fprintf(fp, "Desc %s~\n",	obj->description	    );
    if (!IS_NULLSTR(obj->owner))
	fprintf(fp, "Owner %s~\n",	obj->owner		    );
    if (!IS_NULLSTR(obj->person))
	fprintf(fp, "Person %s~\n",	obj->person		    );


    if (obj->extra_flags != obj->pIndexData->extra_flags){
        bool auct = FALSE;

        if (IS_SET(obj->extra_flags, ITEM_AUCTION)){
            auct = TRUE;
            REMOVE_BIT(obj->extra_flags, ITEM_AUCTION);
        }

        fprintf(fp, "ExtF %lu\n",	obj->extra_flags);

        if (auct) SET_BIT(obj->extra_flags, ITEM_AUCTION);
    }

    if (obj->wear_flags != obj->pIndexData->wear_flags)
	fprintf(fp, "WeaF %d\n",	obj->wear_flags		    );
    if (obj->item_type != obj->pIndexData->item_type)
	fprintf(fp, "Ityp %d\n",	obj->item_type		    );
    if (obj->weight != obj->pIndexData->weight)
	fprintf(fp, "Wt   %d\n",	obj->weight		    );
    if (obj->condition != obj->pIndexData->condition)
	fprintf(fp, "Cond %d\n",	obj->condition		    );

    /* variable data */

    fprintf(fp, "Wear %d\n",   obj->wear_loc               );
    if (obj->level != obj->pIndexData->level)
	fprintf(fp, "Lev  %d\n",	obj->level		    );

    if (obj->timer != 0)
	fprintf(fp, "Time %d\n",	obj->timer	    );

    fprintf(fp, "Cost %d\n",	obj->cost		    );
    if (obj->value[0] != obj->pIndexData->value[0]
	||  obj->value[1] != obj->pIndexData->value[1]
	||  obj->value[2] != obj->pIndexData->value[2]
	||  obj->value[3] != obj->pIndexData->value[3]
	||  obj->value[4] != obj->pIndexData->value[4])
	fprintf(fp, "Val  %d %d %d %d %d\n",
		obj->value[0], obj->value[1], obj->value[2], obj->value[3],
		obj->value[4]	    );

    switch (obj->item_type)
    {
    case ITEM_POTION:
    case ITEM_SCROLL:
    case ITEM_PILL:
	if (obj->value[1] > 0)
	{
	    fprintf(fp, "Spell 1 '%s'\n",
		    skill_table[obj->value[1]].name);
	}

	if (obj->value[2] > 0)
	{
	    fprintf(fp, "Spell 2 '%s'\n",
		    skill_table[obj->value[2]].name);
	}

	if (obj->value[3] > 0)
	{
	    fprintf(fp, "Spell 3 '%s'\n",
		    skill_table[obj->value[3]].name);
	}

	break;

    case ITEM_STAFF:
    case ITEM_WAND:
	if (obj->value[3] > 0)
	{
	    fprintf(fp, "Spell 3 '%s'\n",
		    skill_table[obj->value[3]].name);
	}

	break;
    }

    /* [10847] Оберег при вставленном артефакте криво записываются эффекты */

    for (paf = obj->affected; paf != NULL; paf = paf->next)
    {
	if (paf->type < 0 || paf->type >= max_skills)
	    continue;
	fprintf(fp, "Affc '%s' %3d %3d %3d %3d %3d %10ld\n",
		skill_table[paf->type].name,
		paf->where,
		paf->level,
		paf->duration,
		paf->modifier,
		paf->location,
		paf->bitvector
	       );
    }

    for (ed = obj->extra_descr; ed != NULL; ed = ed->next)
    {
	fprintf(fp, "ExDe %s~ %s~\n",
		ed->keyword, ed->description);
    }

    if (obj->in_room != NULL)
	fprintf(fp, "Room %d\n", obj->in_room->vnum);

    fprintf(fp, "End\n\n");

    if (obj->contains != NULL)
	fwrite_obj(ch, obj->contains, fp, iNest + 1);

    return;
}



/*
 * Load a char and inventory into a new ch structure.
 */
bool load_char_obj(DESCRIPTOR_DATA *d, char *name, bool beac, bool equip){
    char strsave[MAX_INPUT_LENGTH];
    char buf[100];
    CHAR_DATA *ch;
    FILE *fp;
    bool found;
    int stat;

    ch = new_char();
    ch->pcdata = new_pcdata();

    d->character			= ch;
    ch->desc				= d;
    free_string(ch->name);
    ch->name				= str_dup(name);
    ch->id				= get_pc_id();
    ch->race				= race_lookup("human");
    ch->act				= 0;
    ch->pcdata->max_level		= 0;
    ch->comm				= COMM_COMBINE | COMM_PROMPT;

    ch->pcdata->lastip			= str_dup("неизвестен");
    ch->pcdata->lasthost		= str_dup("неизвестен");
    ch->pcdata->genip			= str_dup("неизвестен");

    free_string(ch->prompt);
    ch->prompt 				= str_dup("<%hhp %mm %vmv> ");
    ch->pcdata->pwd			= str_dup("");
    ch->pcdata->bamfin			= str_dup("");
    ch->pcdata->bamfout			= str_dup("");
    ch->pcdata->title			= str_dup("");

    ch->pcdata->spouse			= str_dup("");

    ch->pcdata->bounty			= 0;
    for (stat =0; stat < MAX_STATS; stat++)
    ch->perm_stat[stat]		= 13;
    ch->pcdata->condition[COND_THIRST]	= 48;
    ch->pcdata->condition[COND_FULL]	= 48;
    ch->pcdata->condition[COND_HUNGER]	= 48;

    ch->pcdata->kills_mob	= 0;
    ch->pcdata->kills_pc	= 0;
    ch->pcdata->deaths_mob	= 0;
    ch->pcdata->deaths_pc	= 0;
    ch->pcdata->flees		= 0;
	ch->pcdata->successed	= 0;

    ch->pcdata->quest_curr  	= 0;
    ch->pcdata->quest_type  	= 0;
    ch->pcdata->quest_accum 	= 0;
    ch->pcdata->nextquest 	= 0;
    ch->pcdata->countdown 	= 0;
    ch->pcdata->questgiver 	= NULL;
    ch->pcdata->questmob 	= 0;
    ch->pcdata->questobj 	= 0;

    ch->style			= 0;
    ch->pcdata->avg_dam		= 0;
    ch->pcdata->dam_tick	= 0;
    ch->pcdata->platinum	= 0;
    ch->pcdata->bronze		= 0;

    ch->pcdata->freeze		= 0;
    ch->pcdata->freeze_who	= str_dup("");

    ch->pcdata->nochan		= 0;

    ch->pcdata->security	= 0;	/* OLC */

    ch->pcdata->grants		= str_dup("");
    ch->pcdata->pretitle	= str_dup("");
    ch->pcdata->old_names	= str_dup("");
    ch->pcdata->email		= str_dup("");
    ch->pcdata->email_key[0]	= '\0';

    ch->pcdata->train_hit	= 0;
    ch->pcdata->train_mana	= 0;

#if 1
    ch->pcdata->text[0]		= (NORMAL_);
    ch->pcdata->text[1]		= (WHITE);
    ch->pcdata->text[2]		= 0;
    ch->pcdata->auction[0]	= (BRIGHT);
    ch->pcdata->auction[1]	= (YELLOW);
    ch->pcdata->auction[2]	= 0;
    ch->pcdata->auction_text[0]	= (BRIGHT);
    ch->pcdata->auction_text[1]	= (WHITE);
    ch->pcdata->auction_text[2]	= 0;
    ch->pcdata->gossip[0]	= (NORMAL_);
    ch->pcdata->gossip[1]	= (MAGENTA);
    ch->pcdata->gossip[2]	= 0;
    ch->pcdata->gossip_text[0]	= (BRIGHT);
    ch->pcdata->gossip_text[1]	= (MAGENTA);
    ch->pcdata->gossip_text[2]	= 0;
    ch->pcdata->music[0]	= (NORMAL_);
    ch->pcdata->music[1]	= (RED);
    ch->pcdata->music[2]	= 0;
    ch->pcdata->music_text[0]	= (BRIGHT);
    ch->pcdata->music_text[1]	= (RED);
    ch->pcdata->music_text[2]	= 0;
    ch->pcdata->question[0]	= (BRIGHT);
    ch->pcdata->question[1]	= (YELLOW);
    ch->pcdata->question[2]	= 0;
    ch->pcdata->question_text[0] = (BRIGHT);
    ch->pcdata->question_text[1] = (WHITE);
    ch->pcdata->question_text[2] = 0;
    ch->pcdata->answer[0]	= (BRIGHT);
    ch->pcdata->answer[1]	= (YELLOW);
    ch->pcdata->answer[2]	= 0;
    ch->pcdata->answer_text[0]	= (BRIGHT);
    ch->pcdata->answer_text[1]	= (WHITE);
    ch->pcdata->answer_text[2]	= 0;
    ch->pcdata->quote[0]	= (NORMAL_);
    ch->pcdata->quote[1]	= (YELLOW);
    ch->pcdata->quote[2]	= 0;
    ch->pcdata->quote_text[0]	= (NORMAL_);
    ch->pcdata->quote_text[1]	= (GREEN);
    ch->pcdata->quote_text[2]	= 0;
    ch->pcdata->immtalk_text[0]	= (NORMAL_);
    ch->pcdata->immtalk_text[1]	= (CYAN);
    ch->pcdata->immtalk_text[2]	= 0;
    ch->pcdata->immtalk_type[0]	= (NORMAL_);
    ch->pcdata->immtalk_type[1]	= (YELLOW);
    ch->pcdata->immtalk_type[2]	= 0;
    ch->pcdata->info[0]		= (BRIGHT);
    ch->pcdata->info[1]		= (YELLOW);
    ch->pcdata->info[2]		= 1;
    ch->pcdata->say[0]		= (NORMAL_);
    ch->pcdata->say[1]		= (GREEN);
    ch->pcdata->say[2]		= 0;
    ch->pcdata->say_text[0]	= (BRIGHT);
    ch->pcdata->say_text[1]	= (GREEN);
    ch->pcdata->say_text[2]	= 0;
    ch->pcdata->tell[0]		= (NORMAL_);
    ch->pcdata->tell[1]		= (GREEN);
    ch->pcdata->tell[2]		= 0;
    ch->pcdata->tell_text[0]	= (BRIGHT);
    ch->pcdata->tell_text[1]	= (GREEN);
    ch->pcdata->tell_text[2]	= 0;
    ch->pcdata->reply[0]	= (NORMAL_);
    ch->pcdata->reply[1]	= (GREEN);
    ch->pcdata->reply[2]	= 0;
    ch->pcdata->reply_text[0]	= (BRIGHT);
    ch->pcdata->reply_text[1]	= (GREEN);
    ch->pcdata->reply_text[2]	= 0;
    ch->pcdata->gtell_text[0]	= (NORMAL_);
    ch->pcdata->gtell_text[1]	= (GREEN);
    ch->pcdata->gtell_text[2]	= 0;
    ch->pcdata->gtell_type[0]	= (NORMAL_);
    ch->pcdata->gtell_type[1]	= (RED);
    ch->pcdata->gtell_type[2]	= 0;
    ch->pcdata->wiznet[0]	= (NORMAL_);
    ch->pcdata->wiznet[1]	= (GREEN);
    ch->pcdata->wiznet[2]	= 0;
    ch->pcdata->room_title[0]	= (NORMAL_);
    ch->pcdata->room_title[1]	= (CYAN);
    ch->pcdata->room_title[2]	= 0;
    ch->pcdata->room_text[0]	= (NORMAL_);
    ch->pcdata->room_text[1]	= (WHITE);
    ch->pcdata->room_text[2]	= 0;
    ch->pcdata->room_exits[0]	= (NORMAL_);
    ch->pcdata->room_exits[1]	= (GREEN);
    ch->pcdata->room_exits[2]	= 0;
    ch->pcdata->room_things[0]	= (NORMAL_);
    ch->pcdata->room_things[1]	= (CYAN);
    ch->pcdata->room_things[2]	= 0;
    ch->pcdata->prompt[0]	= (NORMAL_);
    ch->pcdata->prompt[1]	= (CYAN);
    ch->pcdata->prompt[2]	= 0;
    ch->pcdata->fight_death[0]	= (BRIGHT);
    ch->pcdata->fight_death[1]	= (RED);
    ch->pcdata->fight_death[2]	= 0;
    ch->pcdata->fight_yhit[0]	= (NORMAL_);
    ch->pcdata->fight_yhit[1]	= (GREEN);
    ch->pcdata->fight_yhit[2]	= 0;
    ch->pcdata->fight_ohit[0]	= (NORMAL_);
    ch->pcdata->fight_ohit[1]	= (YELLOW);
    ch->pcdata->fight_ohit[2]	= 0;
    ch->pcdata->fight_thit[0]	= (NORMAL_);
    ch->pcdata->fight_thit[1]	= (RED);
    ch->pcdata->fight_thit[2]	= 0;
    ch->pcdata->fight_skill[0]	= (BRIGHT);
    ch->pcdata->fight_skill[1]	= (WHITE);
    ch->pcdata->fight_skill[2]	= 0;
#endif

    ch->pcdata->beacon		= NULL;
    ch->pcdata->ips		= NULL;
    ch->pcdata->visited_size	= -1;

    ch->pcdata->quaff = 0;

    for (stat = 0; stat < MAX_FILTERS; stat++)
	ch->pcdata->filter[stat].ch = NULL;

    found = FALSE;

    /* decompress if .gz file exists */
    sprintf(strsave, "%s%s%s", PLAYER_DIR, capitalize(name),".gz");
    if ((fp = file_open(strsave, "r")) != NULL)
    {
	sprintf(buf,"gzip -dfq %s",strsave);
	system(buf);
    }
    file_close(fp);

    sprintf(strsave, "%s%s", PLAYER_DIR, capitalize(name));
    if ((fp = file_open(strsave, "r")) != NULL){
		int iNest;

		for (iNest = 0; iNest < MAX_NEST; iNest++)
			rgObjNest[iNest] = NULL;

	//	sobj_in  = 0;
		found = TRUE;
		for (; ;){
			char letter;
			char *word;

			letter = fread_letter(fp);
			if (letter == '*'){
				fread_to_eol(fp);
				continue;
			}

			if (letter != '#'){
				bugf("Load_char_obj: # not found.");
				break;
			}

			word = fread_word(fp);
			if      (!str_cmp(word, "PLAYER")) fread_char (ch, fp);
			else if (!str_cmp(word, "OBJECT")) fread_obj  (ch, fp, equip);
			else if (!str_cmp(word, "O"     )) fread_obj  (ch, fp, equip);
			else if (!str_cmp(word, "PET"   )) fread_pet  (ch, fp, TRUE);
			else if (!str_cmp(word, "MOUNT" )) fread_pet  (ch, fp, FALSE);
			else if (!str_cmp(word, "END"   )) break;
			else
			{
			bugf("Load_char_obj: bad section.");
			break;
			}
		}
		set_lycanthrope(ch);
    }
    file_close(fp);
//    ch->pcdata->obj_count_in = sobj_in;

/* ЗАПИШЕМ В ЛОГ IP вошедшего чара, его ник, его почту, время и дату*/
    if (equip) {
		sprintf(strsave, "%sip.log", LOG_DIR, capitalize(name));
		if ((fp = file_open(strsave, "a")) != NULL){
			char this_date[20], this_time[10];
			strftime(this_date, 20, "%Y.%m.%d", localtime(&current_time));
			strftime(this_time, 10, "%T", localtime(&current_time));

				fprintf(fp, "%s %s %s %s %s\n", d->ip,  ch->name, ch->pcdata->email, this_date, this_time);
			file_close(fp);
		}
    }

/* ################################## */

    /* initialize race */
    if (found)
    {
	int i;

	if (ch->race == 0)
	    ch->race = race_lookup("human");

	ch->size = pc_race_table[ch->race].size;
	ch->dam_type = 17; /*punch */

	for (i = 0; i < 5; i++)
	{
	    if (pc_race_table[ch->race].skills[i] == NULL)
		break;
	    group_add(ch,pc_race_table[ch->race].skills[i],FALSE);
	}
	ch->affected_by = ch->affected_by|race_table[ch->race].aff;
	for (i = 0; i < DAM_MAX; i++)
	    ch->resists[i] = ch->resists[i] + race_table[ch->race].resists[i];
	ch->form	= race_table[ch->race].form;
	ch->parts	= race_table[ch->race].parts;
    }


    /* RT initialize skills */

    if (found && ch->version < 2)  /* need to add the new skills */
    {
	group_add(ch,"rom basics",FALSE);
	group_add(ch,class_table[ch->classid].base_group,FALSE);
	group_add(ch,class_table[ch->classid].default_group,TRUE);
	ch->pcdata->learned[gsn_recall] = 50;
    }

    /* fix levels */
    if (found && ch->version < 3 && (ch->level > 35 || ch->trust > 35))
    {
	switch (ch->level)
	{
	case(40) : ch->level = 60;	break;  /* imp -> imp */
	case(39) : ch->level = 58; 	break;	/* god -> supreme */
	case(38) : ch->level = 56;  break;	/* deity -> god */
	case(37) : ch->level = 53;  break;	/* angel -> demigod */
	}

	switch (ch->trust)
	{
	case(40) : ch->trust = 60;  break;	/* imp -> imp */
	case(39) : ch->trust = 58;  break;	/* god -> supreme */
	case(38) : ch->trust = 56;  break;	/* deity -> god */
	case(37) : ch->trust = 53;  break;	/* angel -> demigod */
	case(36) : ch->trust = 51;  break;	/* hero -> hero */
	}
    }

    /* ream gold */
    if (found && ch->version < 4)
    {
	ch->gold   /= 100;
    }

    if (found && !beac)
    {
	if (ch->pcdata->beacon != NULL)
	{
		extract_obj(ch->pcdata->beacon, FALSE, FALSE);
		ch->pcdata->beacon = NULL;
	}

	if (ch->pet)
	{
		extract_char(ch->pet, TRUE);
		ch->pet = NULL;
	}

	if (ch->mount)
	{
		extract_char(ch->mount, TRUE);
		ch->mount = NULL;
	}
    }

    return found;
}



/*
 * Read in a char.
 */

#if defined(KEY)
#undef KEY
#endif

#define KEY(literal, field, value)					\
    if (!str_cmp(word, literal))	\
{					\
    field  = value;			\
    fMatch = TRUE;			\
    break;				\
}

/* provided to free strings */
#if defined(KEYS)
#undef KEYS
#endif

#define KEYS(literal, field, value)					\
    if (!str_cmp(word, literal))	\
{					\
    free_string(field);			\
    field  = value;			\
    fMatch = TRUE;			\
    break;				\
}

/* ЗАГРУЖАЕМ ПАРАМЕТРЫ ЧАРА ИЗ ФАЙЛА */
void fread_char(CHAR_DATA *ch, FILE *fp){
    char buf[MAX_STRING_LENGTH];
    char *word;
    bool fMatch;
    int count = 0;
    int lastlogoff = current_time;
    int percent;
    int beac_room = -1, beac_time = -1;

    for (; ;){
		word   = feof(fp) ? "End" : fread_word(fp);
		fMatch = FALSE;

		if (strcmp(word, "End") && feof(fp)){
			bugf("Fread_char: not corrected player file.");
			break;
		}

		switch (UPPER(word[0])){
		case '*':
			fMatch = TRUE;
			fread_to_eol(fp);
			break;

		case 'A':
			KEY("Act",  		ch->act,		fread_flag(fp));
		/*  KEY("AffectedBy",	ch->affected_by,	fread_flag(fp)); 	*/
		/*  KEY("AfBy",	ch->affected_by,	fread_flag(fp)); 		*/
			KEY("Alignment",	ch->alignment,		fread_number(fp));
			KEY("Alig",		ch->alignment,		fread_number(fp));
			KEY("ADam",		ch->pcdata->avg_dam,		fread_number(fp));
			KEY("Atheist",	ch->pcdata->atheist,		fread_number(fp));

			if (!str_cmp(word, "Alia")){
				if (count >= MAX_ALIAS){
					fread_to_eol(fp);
					fMatch = TRUE;
					break;
				}

				ch->pcdata->alias[count] 	= str_dup(fread_word(fp));
				ch->pcdata->alias_sub[count]	= str_dup(fread_word(fp));
				count++;
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "Alias")){
				if (count >= MAX_ALIAS){
					fread_to_eol(fp);
					fMatch = TRUE;
					break;
				}

				ch->pcdata->alias[count]        = str_dup(fread_word(fp));
				ch->pcdata->alias_sub[count]    = fread_string(fp);
				count++;
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "AC") || !str_cmp(word,"Armor")){
				fread_to_eol(fp);
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word,"ACs")){
				int i;

				for (i = 0; i < 4; i++)
					ch->armor[i] = fread_number(fp);
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "Affc")){
				AFFECT_DATA af;
				int sn;
				sprintf(buf, "%s", fread_word(fp));
		/*		sn = skill_lookup(fread_word(fp));*/
				sn = skill_lookup(buf);
			//	bugf("SKILL %s (%d)", buf, sn);

				if (sn < 0)
					bugf("Fread_char: unknown skill.");
				else
					af.type = sn;

				af.where        = fread_number(fp);
				af.level	= fread_number(fp);
				af.duration	= fread_number(fp);
				af.modifier	= fread_number(fp);
				af.location	= fread_number(fp);
				af.bitvector	= fread_number(fp);
			//	bugf("BITVECTOR %d", af.bitvector);

				af.caster_id	= fread_number(fp);
			//	bugf("CASTER %d", af.caster_id);

				affect_to_char(ch, &af);

				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "AttrMod" ) || !str_cmp(word,"AMod")){
				int stat;
				for (stat = 0; stat < MAX_STATS; stat ++)
					ch->mod_stat[stat] = fread_number(fp);
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "AttrPerm") || !str_cmp(word,"Attr")){
				int stat;

				for (stat = 0; stat < MAX_STATS; stat++)
					ch->perm_stat[stat] = fread_number(fp);
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "AllowedIP")){
				char *tmp, fl[MAX_INPUT_LENGTH], *str = fread_string(fp);
				NAME_LIST *ip;

				for (tmp = one_argument(str, fl); fl[0] != '\0'; tmp = one_argument(tmp, fl)){
					ip = new_name_list();
					ip->name = str_dup(fl);

					ip->next = ch->pcdata->ips;
					ch->pcdata->ips = ip;
				}

				free_string(str);
				fMatch = TRUE;
			}

			break;


		case 'B':
			KEYS("Bamfin",  	ch->pcdata->bamfin,	    fread_string(fp));
			KEYS("Bamfout",	    ch->pcdata->bamfout,	fread_string(fp));
			KEYS("Bin",	    	ch->pcdata->bamfin,	    fread_string(fp));
			KEYS("Bout",    	ch->pcdata->bamfout,	fread_string(fp));
			KEY("Bank",		    ch->pcdata->bank,	    fread_number(fp));
			KEY("Bounty",   	ch->pcdata->bounty,	    fread_number(fp));
			KEY("BeacRoom", 	beac_room,	  	        fread_number(fp));
			KEY("BeacTime", 	beac_time,		        fread_number(fp));
			KEY("Bronze", 	    ch->pcdata->bronze,		fread_number(fp));
			break;

		case 'C':
			KEY("Class", ch->classid, fread_number(fp));
			KEY("Count", ch->pcdata->count_entries_errors, fread_number(fp));
			KEY("Cla", ch->classid, fread_number(fp));
			KEY("Comm",	ch->comm, fread_flag(fp));

			KEYS("Case1", ch->pcdata->cases[0], fread_string(fp));
			KEYS("Case2", ch->pcdata->cases[1], fread_string(fp));
			KEYS("Case3", ch->pcdata->cases[2], fread_string(fp));
			KEYS("Case4", ch->pcdata->cases[3], fread_string(fp));
			KEYS("Case5", ch->pcdata->cases[4], fread_string(fp));

			if (!str_cmp(word, "Clan")){
				char *tmp = fread_string(fp);
				ch->clan = clan_lookup(tmp);
				free_string(tmp);
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "Condition") || !str_cmp(word,"Cond")){
				ch->pcdata->condition[0] = fread_number(fp);
				ch->pcdata->condition[1] = fread_number(fp);
				ch->pcdata->condition[2] = fread_number(fp);
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word,"Cnd")){
				ch->pcdata->condition[0] = fread_number(fp);
				ch->pcdata->condition[1] = fread_number(fp);
				ch->pcdata->condition[2] = fread_number(fp);
				ch->pcdata->condition[3] = fread_number(fp);
				fMatch = TRUE;
				break;
			}

	#if 1
			if(!str_cmp(word, "Coloura"))
			{
			LOAD_COLOUR(text)
				LOAD_COLOUR(auction)
				LOAD_COLOUR(gossip)
				LOAD_COLOUR(music)
				LOAD_COLOUR(question)
				fMatch = TRUE;
			break;
			}
			if(!str_cmp(word, "Colourb"))
			{
			LOAD_COLOUR(answer)
				LOAD_COLOUR(quote)
				LOAD_COLOUR(quote_text)
				LOAD_COLOUR(immtalk_text)
				LOAD_COLOUR(immtalk_type)
				fMatch = TRUE;
			break;
			}
			if(!str_cmp(word, "Colourc"))
			{
			LOAD_COLOUR(info)
				LOAD_COLOUR(tell)
				LOAD_COLOUR(reply)
				LOAD_COLOUR(gtell_text)
				LOAD_COLOUR(gtell_type)
				fMatch = TRUE;
			break;
			}
			if(!str_cmp(word, "Colourd"))
			{
			LOAD_COLOUR(room_title)
				LOAD_COLOUR(room_text)
				LOAD_COLOUR(room_exits)
				LOAD_COLOUR(room_things)
				LOAD_COLOUR(prompt)
				fMatch = TRUE;
			break;
			}
			if(!str_cmp(word, "Coloure"))
			{
			LOAD_COLOUR(fight_death)
				LOAD_COLOUR(fight_yhit)
				LOAD_COLOUR(fight_ohit)
				LOAD_COLOUR(fight_thit)
				LOAD_COLOUR(fight_skill)
				fMatch = TRUE;
			break;
			}
			if(!str_cmp(word, "Colourf"))
			{
			LOAD_COLOUR(wiznet)
				LOAD_COLOUR(say)
				LOAD_COLOUR(say_text)
				LOAD_COLOUR(tell_text)
				LOAD_COLOUR(reply_text)
				fMatch = TRUE;
			break;
			}
			if(!str_cmp(word, "Colourg"))
			{
			LOAD_COLOUR(auction_text)
				LOAD_COLOUR(gossip_text)
				LOAD_COLOUR(music_text)
				LOAD_COLOUR(question_text)
				LOAD_COLOUR(answer_text)
				fMatch = TRUE;
			break;
			}
	#endif

			break;
		case 'D':
			KEY("Damroll",	ch->damroll,		fread_number(fp));
			KEY("Dam",		ch->damroll,		fread_number(fp));
			KEYS("Description",	ch->description,	fread_string(fp));
			KEYS("Desc",	ch->description,	fread_string(fp));
			if (!str_cmp(word, "Deat"))
			{
			ch->pcdata->deaths_mob = fread_number(fp);
			ch->pcdata->deaths_pc = fread_number(fp);
			fMatch = TRUE;
			break;
			}
			break;


		case 'E':

			KEYS("Email",	ch->pcdata->email,		fread_string(fp));

			if (!str_cmp(word, "EmailKey"))
			{
			char *tmp = fread_string(fp);

			strlcpy(ch->pcdata->email_key, tmp, sizeof(ch->pcdata->email_key));
			free_string(tmp);
			fMatch = TRUE;
			break;
			}

			if (!str_cmp(word, "End")){
				/* adjust hp mana move up  -- here for speed's sake */
				percent = (current_time - lastlogoff) * 25 / (2 * 60 * 60);
				ch->pcdata->lastlogof=(time_t) lastlogoff;
				percent = UMIN(percent,100);

				if (percent > 0 && !IS_AFFECTED(ch,AFF_POISON)
					&&  !IS_AFFECTED(ch,AFF_PLAGUE))
				{
					ch->hit	+= (ch->max_hit - ch->hit) * percent / 100;
					ch->mana    += (ch->max_mana - ch->mana) * percent / 100;
					ch->move    += (ch->max_move - ch->move)* percent / 100;
				}

				if (beac_time != -1 && beac_room != -1)
				{
					ROOM_INDEX_DATA *room;

					if ((room = get_room_index(beac_room)) != NULL)
					{
					ch->pcdata->beacon = create_object(get_obj_index(OBJ_VNUM_BEACON),0);
					ch->pcdata->beacon->timer = beac_time;
					obj_to_room(ch->pcdata->beacon, room);
					}
				}

				return;
			}
			KEY("Exp",		ch->exp,		fread_number(fp));
			break;

		case 'F':
			KEY("Freeze",	ch->pcdata->freeze,	fread_number(fp));
			KEY("Flee",		ch->pcdata->flees,	fread_number(fp));
			KEYS("FWho",	ch->pcdata->freeze_who,	fread_string(fp));

			if (!str_cmp( word, "Filter"))
			{
			int i;
			char *tmp, fl[MAX_INPUT_LENGTH], *str = fread_string(fp);

			for (tmp = one_argument(str, fl), i = 0;
				 fl[0] != '\0';
				 tmp = one_argument(tmp, fl), i++)
			{
				ch->pcdata->filter[i].ch = str_dup(fl);
			}

			free_string(str);
			fMatch = TRUE;
			}
			break;

		case 'G':
			KEY("Gold",		ch->gold,		fread_number(fp));
			KEY("GuildC",	ch->count_guild_attacks,fread_number(fp));
			KEYS("Grants",	ch->pcdata->grants,	fread_string(fp));
			KEYS("GenIP",	ch->pcdata->genip,	fread_string(fp));
			if (!str_cmp(word, "Group")  || !str_cmp(word,"Gr"))
			{
			int gn;
			char *temp;

			temp = fread_word(fp) ;
			gn = group_lookup(temp);
			/* gn    = group_lookup(fread_word(fp)); */
			if (gn < 0)
			{
				bugf("Fread_char: unknown group. ");
			}
			else
				gn_add(ch,gn);
			fMatch = TRUE;
			}
			break;

		case 'H':
			KEY("Hitroll",	ch->hitroll,		fread_number(fp));
			KEY("HolyC",	ch->count_holy_attacks,	fread_number(fp));
			KEY("Hit",		ch->hitroll,		fread_number(fp));
			KEY("Home",		ch->pcdata->vnum_house,	fread_number(fp));
			KEYS("Host",	ch->pcdata->lasthost,	fread_string(fp));

			if (!str_cmp(word, "HpManaMove") || !str_cmp(word,"HMV"))
			{
			ch->hit		= fread_number(fp);
			ch->max_hit	= fread_number(fp);
			ch->mana	= fread_number(fp);
			ch->max_mana	= fread_number(fp);
			ch->move	= fread_number(fp);
			ch->max_move	= fread_number(fp);
			fMatch = TRUE;
			break;
			}

			if (!str_cmp(word, "HpManaMovePerm") || !str_cmp(word,"HMVP"))
			{
			ch->pcdata->perm_hit	= fread_number(fp);
			ch->pcdata->perm_mana   = fread_number(fp);
			ch->pcdata->perm_move   = fread_number(fp);
			fMatch = TRUE;
			break;
			}

			break;

		case 'I':
			KEY("Id",		ch->id,			fread_number(fp));
			KEY("InvisLevel",	ch->invis_level,	fread_number(fp));
			KEY("Inco",		ch->incog_level,	fread_number(fp));
			KEY("Invi",		ch->invis_level,	fread_number(fp));
			KEYS("IP",		ch->pcdata->lastip,	fread_string(fp));
			break;

		case 'K':
			if (!str_cmp(word, "Kill"))
			{
			ch->pcdata->kills_mob = fread_number(fp);
			ch->pcdata->kills_pc = fread_number(fp);
			fMatch = TRUE;
			break;
			}
			break;

		case 'L':
			KEY("LastLevel",	ch->pcdata->last_level, fread_number(fp));
			KEY("LLev",		ch->pcdata->last_level, fread_number(fp));
			KEY("Level",	ch->level,		fread_number(fp));
			KEY("Lev",		ch->level,		fread_number(fp));
			KEY("Levl",		ch->level,		fread_number(fp));
			KEY("LogO",		lastlogoff,		fread_number(fp));
			KEYS("LongDescr",	ch->long_descr,		fread_string(fp));
			KEYS("LostC",	ch->lost_command,	fread_string(fp));
			KEYS("LnD",		ch->long_descr,		fread_string(fp));
			break;

		case 'M':
			KEY("MLvl",	ch->pcdata->max_level,	fread_number(fp));

		case 'N':
			KEYS("Name",	ch->name,		fread_string(fp));
			KEY("Note",		ch->pcdata->last_note,	fread_number(fp));
			KEY("NoChan",	ch->pcdata->nochan,	fread_number(fp));
			KEY("NoNote",	ch->pcdata->nonotes,	fread_number(fp));
			KEY("NoTitl",	ch->pcdata->notitle,	fread_number(fp));
			KEYS("NPad",	ch->notepad,		fread_string(fp));

			if (!str_cmp(word,"Not"))
			{
			char c;

			ch->pcdata->last_note			= fread_number(fp);
			ch->pcdata->last_idea			= fread_number(fp);
			ch->pcdata->last_penalty		= fread_number(fp);
			ch->pcdata->last_news			= fread_number(fp);
			ch->pcdata->last_changes		= fread_number(fp);
			if ((c = getc(fp)) != '\n')
			{
				ungetc(c, fp);
				ch->pcdata->last_votes		= fread_number(fp);
			}
			fMatch = TRUE;
			break;
			}
			break;

		case 'O':
			KEY("OAlg",	ch->pcdata->orig_align,	fread_number(fp));
			KEYS("ONames",     ch->pcdata->old_names,  fread_string(fp));
	//	    KEY("ObjCountOut",	ch->pcdata->obj_count_out,	fread_number(fp));

		case 'P':
			KEYS("Password",	ch->pcdata->pwd,	fread_string(fp));
			KEYS("Pass",	ch->pcdata->pwd,	fread_string(fp));
			KEY("Played",	ch->played,		fread_number(fp));
			KEY("Plyd",		ch->played,		fread_number(fp));
			KEY("Points",	ch->pcdata->points,	fread_number(fp));
			KEY("Pnts",		ch->pcdata->points,	fread_number(fp));
			KEY("Position",	ch->position,		fread_number(fp));
			KEY("Pos",		ch->position,		fread_number(fp));
			KEY("Practice",	ch->practice,		fread_number(fp));
			KEY("Prac",		ch->practice,		fread_number(fp));
			KEYS("Prompt",	ch->prompt,		fread_string(fp));
			KEYS("Prom",	ch->prompt,		fread_string(fp));
			KEYS("PreTitle",	ch->pcdata->pretitle,	fread_string(fp));
			KEY("Platinum", 	ch->pcdata->platinum,		fread_number(fp));
			break;
		case 'Q':
			KEY("Quest_Curr",  ch->pcdata->quest_curr,  fread_number(fp));
			KEY("Quest_Accum", ch->pcdata->quest_accum, fread_number(fp));
			KEY("QuestNext",   ch->pcdata->nextquest,   fread_number(fp));
			KEY("QCountDown",  ch->pcdata->countdown,   fread_number(fp));
			KEY("QuestMob",    ch->pcdata->questmob,    fread_number(fp));
			KEY("QuestObj",    ch->pcdata->questobj,    fread_number(fp));
			KEY("QuestType",   ch->pcdata->quest_type,  fread_number(fp));

			if (!str_cmp(word, "QuestGiver"))
			{
			CHAR_DATA *vch;
			int vnum = fread_number(fp);

			LIST_FOREACH(vch, &char_list, link)
				if (IS_NPC(vch) && vch->pIndexData->vnum == vnum)
				{
				ch->pcdata->questgiver = vch;
				break;
				}
			fMatch = TRUE;
			}
			break;

		case 'R':
			KEYS("RAnswer",   ch->pcdata->reg_answer,	fread_string(fp));

			if (!str_cmp(word, "Race"))
			{
			char *tmp = fread_string(fp);
			ch->race = race_lookup(tmp);
			free_string(tmp);
			fMatch = TRUE;
			break;
			}

			if (!str_cmp(word, "Room"))
			{
			ch->in_room = get_room_index(fread_number(fp));
			if (ch->in_room == NULL)
				ch->in_room = get_room_index(ROOM_VNUM_LIMBO);
			fMatch = TRUE;
			break;
			}

			break;

		case 'S':
			KEY("SavingThrow",	ch->saving_throw,	fread_number(fp));
			KEY("Save",	ch->saving_throw,	fread_number(fp));
			KEY("Scro",	ch->lines,		fread_number(fp));
			KEY("Sex",		ch->sex,		fread_number(fp));
			KEY("Style",	ch->style,		fread_number(fp));
			KEYS("ShortDescr",	ch->short_descr,	fread_string(fp));
			KEYS("ShD",		ch->short_descr,	fread_string(fp));
			KEY("Sec",         ch->pcdata->security,	fread_number(fp));	/* OLC */
			KEY("Silv",        ch->silver,             fread_number(fp));
			KEY("Successed",        ch->pcdata->successed,             fread_number(fp));
			KEYS("Spou",        ch->pcdata->spouse,     fread_string(fp));

			if (!str_cmp(word, "Skill") || !str_cmp(word,"Sk"))
			{
			int sn;
			int16_t value;
			int32_t over;
			char *temp;

			value = fread_number(fp);
			if (ch->version < 6)
				over = 0;
			else
				over = fread_number(fp);

			temp = fread_word(fp) ;
			sn = skill_lookup(temp);
			/* sn    = skill_lookup(fread_word(fp)); */
			if (sn < 0)
			{
				bugf("Fread_char: unknown skill [%s].", temp);
			}
			else
			{
				ch->pcdata->learned[sn] = value;
				ch->pcdata->over_skill[sn] = over;
			}
			fMatch = TRUE;
			}

			break;

		case 'T':
			KEY("TrueSex",     ch->pcdata->true_sex,  	fread_number(fp));
			KEY("TSex",	ch->pcdata->true_sex,   fread_number(fp));
			KEY("Trai",	ch->train,		fread_number(fp));
			KEY("Trust",	ch->trust,		fread_number(fp));
			KEY("Tru",		ch->trust,		fread_number(fp));
			KEY("TempRIP",	ch->pcdata->temp_RIP,		fread_number(fp));

			if (!str_cmp(word, "Title")  || !str_cmp(word, "Titl"))
			{
			free_string(ch->pcdata->title);
			ch->pcdata->title = fread_string(fp);
			if (ch->pcdata->title[0] != '.' && ch->pcdata->title[0] != ','
				&&  ch->pcdata->title[0] != '!' && ch->pcdata->title[0] != '?')
			{
				sprintf(buf, " %s", ch->pcdata->title);
				free_string(ch->pcdata->title);
				ch->pcdata->title = str_dup(buf);
			}
			fMatch = TRUE;
			break;
			}

			if (!str_cmp(word, "TrainHM"))
			{
			ch->pcdata->train_hit	= fread_number(fp);
			ch->pcdata->train_mana	= fread_number(fp);
			fMatch = TRUE;
			break;
			}

			break;

		case 'V':
			KEY("Version",     ch->version,		fread_number (fp));
			KEY("Vers",	ch->version,		fread_number (fp));
			if (!str_cmp(word, "Vnum"))
			{
			ch->pIndexData = get_mob_index(fread_number(fp));
			fMatch = TRUE;
			break;
			}

			if (!str_cmp(word, "Vizited")){
				uLong comprLen = top_vnum;
				Bytef *compr;
				int i, size;
				Bytef *tmp;

				size = fread_number(fp);

				tmp = malloc(size);

				for (i = 0; i < size; i++)
					tmp[i] = fgetc(fp);

				compr = malloc(comprLen);

				uncompress(compr, &comprLen, tmp, size);

				ch->pcdata->visited_size = comprLen;
				ch->pcdata->visited = alloc_mem(comprLen);
				memcpy(ch->pcdata->visited, compr, comprLen);

				free(compr);
				free(tmp);

				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "VizitP")){
				fread_number(fp);

				fMatch = TRUE;
				break;
			}

			break;

		case 'W':
			KEY("Wimpy",	ch->wimpy,		fread_number(fp));
			KEY("Wimp",	ch->wimpy,		fread_number(fp));
			KEY("Wizn",	ch->wiznet,		fread_flag(fp));
			KEYS("WarC",	ch->war_command,	fread_string(fp));
			KEY("WarOn",	ch->war_command_on,	fread_number(fp));
			break;
		}

		if (!fMatch){
			bugf("Fread_char: no match (%s).", word);
			fread_to_eol(fp);
		}
    }
}

/* load a pet from the forgotten reaches */
void fread_pet(CHAR_DATA *ch, FILE *fp, bool is_pet)
{
    char *word;
    CHAR_DATA *pet;
    bool fMatch;
    int lastlogoff = current_time;
    int percent;
    AFFECT_DATA *paf, *paf_next;

    /* first entry had BETTER be the vnum or we barf */
    word = feof(fp) ? "END" : fread_word(fp);
    if (!str_cmp(word,"Vnum"))
    {
	int vnum;

	vnum = fread_number(fp);
	if (get_mob_index(vnum) == NULL)
	{
	    bugf("Fread_pet: bad vnum %d.", vnum);
	    pet = create_mobile(get_mob_index(MOB_VNUM_FIDO));
	}
	else
	    pet = create_mobile(get_mob_index(vnum));
    }
    else
    {
	bugf("Fread_pet: no vnum in file.");
	pet = create_mobile(get_mob_index(MOB_VNUM_FIDO));
    }

    for (paf = pet->affected; paf != NULL; paf = paf_next)
    {
	paf_next = paf->next;
	affect_remove(pet, paf);
    }

    for (; ;)
    {
	word 	= feof(fp) ? "END" : fread_word(fp);
	fMatch = FALSE;

	switch (UPPER(word[0]))
	{
	case '*':
	    fMatch = TRUE;
	    fread_to_eol(fp);
	    break;

	case 'A':
	    KEY("Act",		pet->act,	fread_flag(fp));
	    KEY("AfBy",	pet->affected_by,	fread_flag(fp));
	    KEY("Alig",	pet->alignment,		fread_number(fp));

	    if (!str_cmp(word,"ACs"))
	    {
		int i;

		for (i = 0; i < 4; i++)
		    pet->armor[i] = fread_number(fp);
		fMatch = TRUE;
		break;
	    }

	    if (!str_cmp(word,"Affc"))
	    {
		AFFECT_DATA af;
		int sn;

		sn = skill_lookup(fread_word(fp));
		if (sn < 0)
		    bugf("Fread_pet: unknown skill.");
		else
		    af.type = sn;

		af.where	= fread_number(fp);
		af.level	= fread_number(fp);
		af.duration	= fread_number(fp);
		af.modifier	= fread_number(fp);
		af.location	= fread_number(fp);
		af.bitvector	= fread_number(fp);
		af.caster_id	= fread_number(fp);
		affect_to_char(pet, &af);
		fMatch		= TRUE;
		break;
	    }

	    if (!str_cmp(word,"AMod"))
	    {
		int stat;

		for (stat = 0; stat < MAX_STATS; stat++)
		    pet->mod_stat[stat] = fread_number(fp);
		fMatch = TRUE;
		break;
	    }

	    if (!str_cmp(word,"Attr"))
	    {
		int stat;

		for (stat = 0; stat < MAX_STATS; stat++)
		    pet->perm_stat[stat] = fread_number(fp);
		fMatch = TRUE;
		break;
	    }
	    break;

	case 'C':
	    KEY("Comm",	pet->comm,		fread_flag(fp));

	    if (!str_cmp(word, "Clan"))
	    {
		char *tmp = fread_string(fp);
		pet->clan = clan_lookup(tmp);
		free_string(tmp);
		fMatch = TRUE;
		break;
	    }

	    break;

	case 'D':
	    KEY("Dam",	pet->damroll,		fread_number(fp));
	    KEYS("Desc",	pet->description,	fread_string(fp));
	    break;

	case 'E':
	    if (!str_cmp(word,"End"))
	    {
		if (is_pet)
		{
		    pet->leader = ch;
		    pet->master = ch;
		    ch->pet = pet;
		}
		else
		{
		    ch->mount = pet;
		    pet->mount = ch;
		}

		/* adjust hp mana move up  -- here for speed's sake */
		percent = (current_time - lastlogoff) * 25 / (2 * 60 * 60);

		if (percent > 0 && !IS_AFFECTED(ch,AFF_POISON)
		    &&  !IS_AFFECTED(ch,AFF_PLAGUE))
		{
		    percent = UMIN(percent,100);
		    pet->hit	+= (pet->max_hit - pet->hit) * percent / 100;
		    pet->mana   += (pet->max_mana - pet->mana) * percent / 100;
		    pet->move   += (pet->max_move - pet->move)* percent / 100;
		}

		return;
	    }
	    KEY("Exp",	pet->exp,		fread_number(fp));
	    break;

	case 'G':
	    KEY("Gold",	pet->gold,		fread_number(fp));
	    break;

	case 'H':
	    KEY("Hit",	pet->hitroll,		fread_number(fp));

	    if (!str_cmp(word,"HMV"))
	    {
		pet->hit	= fread_number(fp);
		pet->max_hit	= fread_number(fp);
		pet->mana	= fread_number(fp);
		pet->max_mana	= fread_number(fp);
		pet->move	= fread_number(fp);
		pet->max_move	= fread_number(fp);
		fMatch = TRUE;
		break;
	    }
	    break;

	case 'L':
	    KEY("Levl",	pet->level,		fread_number(fp));
	    KEYS("LnD",	pet->long_descr,	fread_string(fp));
	    KEY("LogO",	lastlogoff,		fread_number(fp));
	    break;

	case 'N':
	    KEYS("Name",	pet->name,		fread_string(fp));
	    break;

	case 'P':
	    KEY("Pos",	pet->position,		fread_number(fp));
	    break;

	case 'R':
	    if (!str_cmp(word, "Race"))
	    {
		char *tmp = fread_string(fp);
		pet->race = race_lookup(tmp);
		free_string(tmp);
		fMatch = TRUE;
		break;
	    }

	    break;

	case 'S' :
	    KEY("Save",	pet->saving_throw,	fread_number(fp));
	    KEY("Sex",		pet->sex,		fread_number(fp));
	    KEYS("ShD",		pet->short_descr,	fread_string(fp));
	    KEY("Silv",        pet->silver,            fread_number(fp));
	    break;

	    if (!fMatch)
	    {
		bugf("Fread_pet: no match (%s).", word);
		fread_to_eol(fp);
	    }

	}
    }
}

void fread_obj(CHAR_DATA *ch, FILE *fp, bool equip)
{
    OBJ_DATA *obj;
    char *word;
    int iNest;
    bool fMatch;
    bool fNest;
    bool fVnum;
    bool first;
    bool new_format;  /* to prevent errors */
    bool make_new;    /* update object */
    int  room_vnum = 0;

    fVnum = FALSE;
    obj = NULL;
    first = TRUE;  /* used to counter fp offset */
    new_format = FALSE;
    make_new = FALSE;

    word   = feof(fp) ? "End" : fread_word(fp);
    if (!str_cmp(word,"Vnum"))
    {
	int vnum;
	first = FALSE;  /* fp will be in right place */

//	sobj_in++;

	vnum = fread_number(fp);
	if ( get_obj_index(vnum)  == NULL)
	{
	    bugf("Fread_obj: bad vnum %d.", vnum);
	}
	else
	{
	    obj = create_object(get_obj_index(vnum),-1);
	    new_format = TRUE;
	}

    }

    if (obj == NULL)  /* either not found or old style */
    {
	obj = new_obj();
	obj->name		= str_dup("");
	obj->short_descr	= str_dup("");
	obj->description	= str_dup("");
    }

    fNest		= FALSE;
    fVnum		= TRUE;
    iNest		= 0;

    for (; ;)
    {
	if (first)
	    first = FALSE;
	else
	    word   = feof(fp) ? "End" : fread_word(fp);
	fMatch = FALSE;

	switch (UPPER(word[0]))
	{
	case '*':
	    fMatch = TRUE;
	    fread_to_eol(fp);
	    break;

	case 'A':
	    if (!str_cmp(word,"Affc"))
	    {
		AFFECT_DATA *paf;
		int sn;

		paf = new_affect();

		sn = skill_lookup(fread_word(fp));
		if (sn < 0)
		    bugf("Fread_obj: unknown skill.");
		else
		    paf->type = sn;

		paf->where	= fread_number(fp);
		paf->level	= fread_number(fp);
		paf->duration	= fread_number(fp);
		paf->modifier	= fread_number(fp);
		paf->location	= fread_number(fp);
		paf->bitvector	= fread_number(fp);
		paf->next	= obj->affected;
		paf->prev	= NULL;
		if (obj->affected)
		    obj->affected->prev = paf;
		obj->affected	= paf;
		fMatch		= TRUE;
		break;
	    }
	    break;

	case 'C':
	    KEY("Cond",	obj->condition,		fread_number(fp));
	    KEY("Cost",	obj->cost,		fread_number(fp));
	    break;

	case 'D':
	    KEYS("Description",	obj->description,	fread_string(fp));
	    KEYS("Desc",	obj->description,	fread_string(fp));
	    break;

	case 'E':

	    if (!str_cmp(word, "Enchanted"))
	    {
		obj->enchanted = TRUE;
		fMatch 	= TRUE;
		break;
	    }

	    KEY("ExtraFlags",	obj->extra_flags,	fread_number(fp));
	    KEY("ExtF",	obj->extra_flags,	fread_number(fp));

	    if (!str_cmp(word, "ExtraDescr") || !str_cmp(word,"ExDe"))
	    {
		EXTRA_DESCR_DATA *ed;

		ed = new_extra_descr();

		ed->keyword		= fread_string(fp);
		ed->description		= fread_string(fp);
		ed->next		= obj->extra_descr;
		obj->extra_descr	= ed;
		fMatch = TRUE;
	    }

	    if (!str_cmp(word, "End"))
	    {
		if (!fNest || (fVnum && obj->pIndexData == NULL))
		{
		    bugf("Fread_obj: incomplete object.");
		    extract_obj(obj, FALSE, FALSE);
		    rgObjNest[iNest] = NULL;
		    return;
		}
		else
		{
		    int wear;

		    if (!fVnum)
		    {
			free_obj(obj);
			obj = create_object(get_obj_index(OBJ_VNUM_DUMMY), 0);
		    }

		    if (!new_format)
		    {
			obj->next	= object_list;
			obj->prev	= NULL;
			if (object_list)
			    object_list->prev = obj;
			object_list	= obj;
			obj->pIndexData->count++;
		    }

		    if (!obj->pIndexData->new_format
			&& obj->item_type == ITEM_ARMOR
			&&  obj->value[1] == 0)
		    {
			obj->value[1] = obj->value[0];
			obj->value[2] = obj->value[0];
		    }

		    wear = obj->wear_loc;

		    if (make_new)
		    {
			if (obj->carried_by)
	    			bugf("уничтожаем объект %s у чара/моба %s.",obj->name,obj->carried_by->name);

			extract_obj(obj, TRUE, TRUE);
			obj = create_object(obj->pIndexData,0);
			obj->wear_loc = wear;
		    }

		    if (iNest == 0 || rgObjNest[iNest - 1] == NULL)
		    {
			ROOM_INDEX_DATA *room;

			if (ch)
			{
			    if (equip) 
				obj->wear_loc = WEAR_NONE;
			    else
				obj->wear_loc = wear; 

			    obj_to_char(obj, ch);

			    if (equip && wear != WEAR_NONE)
				equip_char(ch, obj, wear);

			}
			else if ((room = get_room_index(room_vnum)) != NULL)
			    obj_to_room(obj, room);
			else
			    bugf("Fread_obj: Null room vnum.");
		    }
		    else
			obj_to_obj(obj, rgObjNest[iNest-1]);
		    
		    return;
		}

	    }
	    break;

	case 'I':
	    KEY("ItemType",	obj->item_type,		fread_number(fp));
	    KEY("Ityp",	obj->item_type,		fread_number(fp));
	    break;

	case 'L':
	    KEY("Level",	obj->level,		fread_number(fp));
	    KEY("Lev",		obj->level,		fread_number(fp));
	    KEY("LifeTime",	obj->lifetime,		fread_number(fp));
	    break;

	case 'N':
	    KEYS("Name",	obj->name,		fread_string(fp));

	    if (!str_cmp(word, "Nest"))
	    {
		iNest = fread_number(fp);
		if (iNest < 0 || iNest >= MAX_NEST)
		{
		    bugf("Fread_obj: bad nest %d.", iNest);
		}
		else 
		{
		    rgObjNest[iNest] = obj;
		    fNest = TRUE;
		}
		fMatch = TRUE;
	    }
	    break;

	case 'O':

	    KEYS("Owner",	obj->owner,		fread_string(fp));

	    if (!str_cmp(word,"Oldstyle"))
	    {
		if (obj->pIndexData != NULL && obj->pIndexData->new_format)
		    make_new = TRUE;
		fMatch = TRUE;
	    }
	    break;

	case 'P':

	    KEYS("Person",	obj->person,		fread_string(fp));
	    break;

	case 'R':

	    KEY("Room",	room_vnum,		fread_number(fp));
	    break;

	case 'S':
	    KEYS("ShortDescr",	obj->short_descr,	fread_string(fp));
	    KEYS("ShD",		obj->short_descr,	fread_string(fp));

	    if (!str_cmp(word, "Spell"))
	    {
		int iValue;
		int sn;

		iValue = fread_number(fp);
		sn     = skill_lookup(fread_word(fp));
		if (iValue < 0 || iValue > 3)
		{
		    bugf("Fread_obj: bad iValue %d.", iValue);
		}
		else if (sn < 0)
		{
		    bugf("Fread_obj: unknown skill.");
		}
		else
		{
		    obj->value[iValue] = sn;
		}
		fMatch = TRUE;
		break;
	    }

	    break;

	case 'T':
	    KEY("Timer",	obj->timer,		fread_number(fp));
	    KEY("Time",	obj->timer,		fread_number(fp));
	    break;

	case 'V':
	    if (!str_cmp(word, "Values") || !str_cmp(word,"Vals"))
	    {
		obj->value[0]	= fread_number(fp);
		obj->value[1]	= fread_number(fp);
		obj->value[2]	= fread_number(fp);
		obj->value[3]	= fread_number(fp);
		if (obj->item_type == ITEM_WEAPON && obj->value[0] == 0)
		    obj->value[0] = obj->pIndexData->value[0];
		fMatch		= TRUE;
		break;
	    }

	    if (!str_cmp(word, "Val"))
	    {
		obj->value[0] 	= fread_number(fp);
		obj->value[1]	= fread_number(fp);
		obj->value[2] 	= fread_number(fp);
		obj->value[3]	= fread_number(fp);
		obj->value[4]	= fread_number(fp);
		fMatch = TRUE;
		break;
	    }

	    if (!str_cmp(word, "Vnum"))
	    {
		int vnum;	

		vnum = fread_number(fp);
		if ((obj->pIndexData = get_obj_index(vnum)) == NULL)
		    bugf("Fread_obj: bad vnum %d.", vnum);
		else
		{
		    fVnum = TRUE;
//		    sobj_in++;
		}
		fMatch = TRUE;
		break;
	    }
	    break;

	case 'W':
	    KEY("WearFlags",	obj->wear_flags,	fread_number(fp));
	    KEY("WeaF",	obj->wear_flags,	fread_number(fp));
	    KEY("WearLoc",	obj->wear_loc,		fread_number(fp));
	    KEY("Wear",	obj->wear_loc,		fread_number(fp));
	    KEY("Weight",	obj->weight,		fread_number(fp));
	    KEY("Wt",		obj->weight,		fread_number(fp));
	    break;

	}

	if (!fMatch)
	{
	    bugf("Fread_obj: no match.");
	    fread_to_eol(fp);
	}
    }
}

void save_corpses()
{
    FILE *fp;
    OBJ_DATA *obj;
    int count = 0;
    char buf[MSL];

    if ((fp = file_open(CORPSES_FILE, "w")) == NULL)
    {
	bugf("Cannot open %s file! No corpses are saved.", CORPSES_FILE);
	return;
    }

    for(obj = object_list; obj; obj = obj->next)
	if (obj->item_type == ITEM_CORPSE_PC
	    && obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC
	    && obj->in_room != NULL && obj->contains)
	{
	    fwrite_obj(NULL, obj, fp, 0);
	    count++;
	}

    fprintf(fp, "#END\n");
    file_close(fp);

    if (count == 0)
	unlink(CORPSES_FILE);
    else
    {
	sprintf(buf, "Saved %d corpses.", count);
	log_string(buf);
    }
}

void load_corpses()
{
    FILE *fp;

    if ((fp = file_open(CORPSES_FILE, "r")) != NULL)
    {
	int iNest;

	for (iNest = 0; iNest < MAX_NEST; iNest++)
	    rgObjNest[iNest] = NULL;

	for (; ;)
	{
	    char letter;
	    char *word;

	    letter = fread_letter(fp);
	    if (letter == '*')
	    {
		fread_to_eol(fp);
		continue;
	    }

	    if (letter != '#')
	    {
		bugf("Load_corpses: # not found.");
		break;
	    }

	    word = fread_word(fp);
	    if (!str_cmp(word, "OBJECT")) fread_obj  (NULL, fp, FALSE);
	    else if (!str_cmp(word, "O"     )) fread_obj  (NULL, fp, FALSE);
	    else if (!str_cmp(word, "END"   )) break;
	    else
	    {
		bugf("Load_corpses: bad section.");
		break;
	    }
	}

	unlink(CORPSES_FILE);
    }
}

/*****************************************************************************
Name:		delete_temp_chest
Purpose:	Функция удаления темповых сундуков.
 ****************************************************************************/
void delete_temp_chest()
{
    DIR *dp;
    struct dirent *ent;
    int count = -2;

    if ((dp = opendir(CHEST_DIR)) == NULL)
    {
	bugf("Can't open chest dir %s!", CHEST_DIR);
	return;
    }    

    while ((ent = readdir(dp)) != NULL)
    {
        if (is_number(ent->d_name))
	    continue;
	count++;
    }

    if (count < 600)
	return;

    count = -2;

    while ((ent = readdir(dp)) != NULL)
    {
	char strtemp[MAX_STRING_LENGTH];

        if (is_number(ent->d_name))
	    continue;

	snprintf(strtemp, sizeof(strtemp), "%s%s", CHEST_DIR, ent->d_name);
	unlink(strtemp);
	count++;
    }
//    bugf("Удалено темповых сундуков %d!", count);
        
    return;
}

/*****************************************************************************
Name:		save_chest
Purpose:	Функция сохранения объектов сундук в файл.
Called by:	act_wiz.c: 	for_reboot, do_oload, do_purge,			
		act_obj.c: 	do_get, do_put, do_drop
 ****************************************************************************/
void save_chest(long id)
{
    OBJ_DATA *obj;
    int count = 0;   

    for(obj = object_list; obj; obj = obj->next)
    {
	if (obj->item_type == ITEM_CHEST && obj->in_room != NULL)
	{
	    if (id == 0)
	    {
		save_one_chest(obj);	// Сохраняем все
		count++;
	    }
	    else if (obj->id == id)
	    {
		save_one_chest(obj);	// Сохраняем конкретный сундук
	    }
	}
    }
    if (id == 0)
	bugf("Сохранено сундуков %d!", count);
    delete_temp_chest();	// Удаляем темповые сундуки
    return;
}

/*****************************************************************************
Name:		save_one_chest
Purpose:	Функция сохранения одного сундука в файл.
Called by:	save.c: save_chest
 ****************************************************************************/
void save_one_chest(OBJ_DATA *chest)
{
    FILE *fpchest;
    char strsave[MIL];

    sprintf(strsave, "%s%ld",CHEST_DIR,chest->id);
    if ((fpchest = file_open(strsave, "w")) == NULL)
    {
	bugf("Save_chest: Cannot open chest file.");
	_perror(strsave);		
    }
    else
    {
	fwrite_obj(NULL, chest, fpchest, 0);
	fprintf(fpchest, "#END\n");
	file_close(fpchest);
    }    
    return;
}


/*****************************************************************************
Name:		load_chest
Purpose:	Функция чтения файлов-сундуков.
Called by:	db.c:boot_db.
 ****************************************************************************/
void load_chest()
{
    DIR *dp;
    struct dirent *ent;

    if ((dp = opendir(CHEST_DIR)) == NULL)
    {
	bugf("Can't open chest dir %s!", CHEST_DIR);
	return;
    }    

    while ((ent = readdir(dp)) != NULL)
    {
	char strload[MAX_STRING_LENGTH], strtemp[MAX_STRING_LENGTH];
	FILE *fpchest = NULL;	

        if (!is_number(ent->d_name))
	    continue;    

	snprintf(strload, sizeof(strload), "%s%s", CHEST_DIR, ent->d_name);

	if ((fpchest = file_open(strload, "r")) == NULL)
	{
    	    _perror(strload);
    	    exit(1);
	}	

	for (; ;)
	{
	    char letter;
	    char *word;

	    letter = fread_letter(fpchest);

	    if (letter == '*')
	    {
		fread_to_eol(fpchest);
		continue;
    	    }

    	    if (letter != '#')
    	    {
		bugf("Load_chest: # not found.");
		break;
    	    }

	    word = fread_word(fpchest);
    	    if (!str_cmp(word, "OBJECT")) 
	 	fread_obj  (NULL, fpchest, FALSE);
    	    else if (!str_cmp(word, "O"     )) 
		fread_obj  (NULL, fpchest, FALSE);
    	    else if (!str_cmp(word, "END"   )) 
		break;
    	    else
    	    {
		bugf("Load_chest: bad section.");
		break;
   	    }			
	}
	file_close(fpchest);
	snprintf(strtemp, sizeof(strtemp), "%st%s", CHEST_DIR,ent->d_name);
	rename(strload,strtemp);
    }
    return;
}

/*****************************************************************************
Name:		delete_chest
Purpose:	Функция удаления одного сундука.
Called by:	act_obj.c: get_obj
 ****************************************************************************/
void delete_chest(long id)
{
    char buf[MIL];

    bugf("Удаляем сундук. ID = %ld", id);
    sprintf(buf, "%s%ld",CHEST_DIR,id);
    unlink(buf);

    return;
}



/* charset=cp1251 */
