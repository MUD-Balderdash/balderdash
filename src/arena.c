/**************************************************************************
 * Copyright (c) 2003                                                      *
 *      The staff of Balderdash MUD.  All rights reserved.                 *
 *                                                                         *
 * Redistribution and use in source and binary forms, with or without      *
 * modification, are permitted provided that the following conditions      *
 * are met:                                                                *
 * 1. Redistributions of source code must retain the above copyright       *
 *    notice, this list of conditions and the following disclaimer.        *
 * 2. Redistributions in binary form must reproduce the above copyright    *
 *    notice, this list of conditions and the following disclaimer in the  *
 *    documentation and/or other materials provided with the distribution. *
 * 3. Neither the name of Balderdash MUD the names of its contributors may *
 *    be used to endorse or promote products derived from this software    *
 *    without specific prior written permission.                           *
 *                                                                         *
 * THIS SOFTWARE IS PROVIDED BY BALDERDASH MUD AND CONTRIBUTORS ``AS       *
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT          *
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS       *
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE         *
 * STAFF OF FATAL DIMENSIONS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,     *
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES      *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR      *
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)      *
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,     *
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)           *
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED     *
 * OF THE POSSIBILITY OF SUCH DAMAGE.                                      *
 ***************************************************************************/
/* Alexander Speransky aka Sure                                   */
/* Alexey V. Antipovsky aka Kemm aka Ssart                        */
/* Victor N. Kachulin aka Vicus                                   */


#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "recycle.h"


CHALLENGER_DATA *challenger_list;

void load_arenastats(void)
{
    FILE *fpList;
    FILE *fpChallenger;
    char *strChallenger, buf[MIL];

    if ((fpList = file_open(CHALLENGER_LIST, "r")) == NULL)
    {
	_perror(CHALLENGER_LIST);
	exit(1);
    }

    for (; ;)
    {
	CHALLENGER_DATA *pChallenger;

	strChallenger = fread_word(fpList);
	if (strChallenger[0] == '$')
	    break;

	sprintf( buf, "%s%s", CHALLENGER_DIR, strChallenger );
	if ((fpChallenger = file_open(buf, "r")) == NULL)
	{
	    _perror(strChallenger);
	    exit(1);
	}

	pChallenger = new_challenger();
	pChallenger->name = str_dup(strChallenger);

	for (; ;)
	{
	    char *word;

	    if (fread_letter(fpChallenger) != '#')
	    {
		bugf("Load_arenastats: # not found.");
		exit(1);
	    }

	    word = fread_word(fpChallenger);

	    if (!str_cmp(word, "END"))
		break;
	    else if (!str_cmp(word, "SCOREDATA"))
	    {
		load_scores(fpChallenger, pChallenger);
	    }
	    else
	    {
		bugf("Load_arenastats: bad section name.");
		exit(1);
	    }
	}
	file_close(fpChallenger);

	if (challenger_list == NULL)
	    challenger_list = pChallenger;
	else
	{
	    pChallenger->next = challenger_list;
	    challenger_list = pChallenger;
	}
    }
    file_close(fpList);
    return;
}

void load_scores(FILE *fp, CHALLENGER_DATA *challenger)
{
    SCORE_DATA *pScore;

    pScore = new_score();

    pScore->time        = fread_number(fp);
    pScore->opponent    = fread_string(fp);
    pScore->won         = fread_number(fp);

    challenger->ball += (pScore->won == 2 ? 3 : pScore->won);
    challenger->game += 1;

    if (challenger->score == NULL)
	challenger->score = pScore;
    else
    {
	pScore->next = challenger->score;
	challenger->score = pScore;
    }

    return;
}

CHALLENGER_DATA *add_challenger(char *name)
{
    CHALLENGER_DATA *challenger;

    challenger = new_challenger();
    challenger->name = str_dup(name);

    if (challenger_list == NULL)
	challenger_list = challenger;
    else
    {
	challenger->next = challenger_list;
	challenger_list = challenger;
    }

    save_arenastats();
    return challenger;
}

SCORE_DATA *add_score(CHALLENGER_DATA *challenger, time_t time, char *opponent, int won)
{
    SCORE_DATA *score;

    score = new_score();

    score->time = time;
    score->opponent = str_dup(opponent);
    score->won = won;

    challenger->ball += (won == 2 ? 3 : won);
    challenger->game += 1;

    if (challenger->score == NULL)
	challenger->score = score;
    else
    {	
	score->next = challenger->score;
	challenger->score = score;
    }
    save_arenastats();
    return score;
}

CHALLENGER_DATA *find_challenger(char *name)
{
    CHALLENGER_DATA *challenger;

    for (challenger  = challenger_list;
	 challenger != NULL;
	 challenger  = challenger->next)
    {
	if (!str_cmp(challenger->name, name))
	    break;
    }
    return challenger;
}

SCORE_DATA *find_score_by_time(CHALLENGER_DATA *challenger, time_t time)
{
    SCORE_DATA *score;

    for (score = challenger->score; score != NULL; score = score->next)
    {
	if (score->time == time)
	    break;
    }
    return score;
}

SCORE_DATA *find_score_by_opponent(CHALLENGER_DATA *challenger, char *name)
{
    SCORE_DATA *score;

    for (score = challenger->score; score != NULL; score = score->next)
    {
	if (!str_cmp(score->opponent, name))
	    break;
    }
    return score;
}

SCORE_DATA *find_score_by_won(CHALLENGER_DATA *challenger, bool won)
{
    SCORE_DATA *score;

    for (score = challenger->score; score != NULL; score = score->next)
    {
	if (score->won == won)
	    break;
    }
    return score;
}

void delete_challenger(CHALLENGER_DATA *challenger)
{
    SCORE_DATA *score;

    if (challenger == NULL)
	return;

    for (score = challenger->score; score != NULL; score = score->next)
	delete_score(challenger, score->time);

    if (challenger == challenger_list)
    {
	challenger_list = challenger->next;
    }
    else
    {
	CHALLENGER_DATA *prev;

	for (prev = challenger_list; prev != NULL; prev = prev->next)
	{
	    if (prev->next == challenger)
	    {
		prev->next = challenger->next;
		break;
	    }
	}

	if (prev == NULL)
	    return;
    }
    free_challenger(challenger);
    save_arenastats();
    return;
}

void delete_score(CHALLENGER_DATA *challenger, time_t time)
{
    SCORE_DATA *score;

    if ((score = find_score_by_time(challenger, time)) == NULL)
	return;

    challenger->ball -= (score->won == 2 ? 3 : score->won);
    challenger->game -= 1;

    if (score == challenger->score)
    {
	challenger->score = score->next;
    }
    else
    {
	SCORE_DATA *prev;

	for (prev = challenger->score; prev != NULL; prev = prev->next)
	{
	    if (prev->next == score)
	    {
		prev->next = score->next;
		break;
	    }
	}

	if (prev == NULL)
	    return;
    }
    free_score(score);
    save_arenastats();
    return;
}

void save_arenastats(void)
{
    FILE *fpList, *fpChallenger;
    CHALLENGER_DATA *challenger;
    SCORE_DATA *score;

    if ((fpList = file_open(CHALLENGER_LIST, "w")) == NULL)
    {
	bugf("Save_arenastats: Cannot open challenger_list file.");
	_perror(CHALLENGER_LIST);
    }
    else
    {
	for (challenger  = challenger_list;
	     challenger != NULL;
	     challenger  = challenger->next)
	{
	    char strsave[MIL];

	    sprintf(strsave, "%s%s", CHALLENGER_DIR, capitalize(challenger->name));
	    if ((fpChallenger = file_open(strsave, "w")) == NULL)
	    {
		bugf("Save_arenastats: Cannot open challenger file.");
		_perror(strsave);
	    }
	    else
	    {
		fprintf(fpList, "%s\n", capitalize(challenger->name));
		for (score  = challenger->score;
		     score != NULL;
		     score  = score->next)
		{
		    fprintf(fpChallenger, "#SCOREDATA\n");
		    fprintf(fpChallenger, "%ld\n", (long)score->time);
		    fprintf(fpChallenger, "%s~\n", capitalize(score->opponent));
		    fprintf(fpChallenger, "%d\n", score->won);
		}
		fprintf(fpChallenger, "#END\n");
	    }
	    file_close(fpChallenger);
	}
	fprintf(fpList, "$\n");
    }
    file_close(fpList);

    return;
}

/* charset=cp1251 */
