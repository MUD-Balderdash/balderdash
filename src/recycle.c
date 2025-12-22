/***************************************************************************
 *  File: recycle.c                                                        *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/


#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define IN_RECYCLE_C
#include "merc.h"
#undef IN_RECYCLE_C
#include "recycle.h"


/*
 * Globals
 */
extern          int                     top_reset;
extern          int                     top_area;
extern          int                     top_exit;
extern          int                     top_ed;
extern          int                     top_room;
extern		int			top_mprog_index;
extern		int			top_oprog_index;
extern		int			top_rprog_index;
void    free_oprog              args ((PROG_LIST *op));
void    free_rprog              args ((PROG_LIST *rp));


AREA_DATA		*	area_free;
EXTRA_DESCR_DATA	*	extra_descr_free;
EXIT_DATA		*	exit_free;
ROOM_INDEX_DATA		*	room_index_free;
OBJ_INDEX_DATA		*	obj_index_free;
SHOP_DATA		*	shop_free;
MOB_INDEX_DATA		*	mob_index_free;
RESET_DATA		*	reset_free;

HELP_DATA		*	help_last;

void	free_extra_descr	args((EXTRA_DESCR_DATA *pExtra));
void	free_affect		args((AFFECT_DATA *af));
void	free_mprog              args ((PROG_LIST *mp));


RESET_DATA *new_reset_data(void)
{
    RESET_DATA *pReset;

    if (!reset_free)
    {
        pReset          =   alloc_perm(sizeof(*pReset));
        top_reset++;
    }
    else
    {
        pReset          =   reset_free;
        reset_free      =   reset_free->next;
    }

    pReset->next        =   NULL;
    pReset->command     =   'X';
    pReset->arg1        =   0;
    pReset->arg2        =   0;
    pReset->arg3        =   0;
    pReset->arg4	=   0;

    return pReset;
}



void free_reset_data(RESET_DATA *pReset)
{
    pReset->next            = reset_free;
    reset_free              = pReset;
    return;
}



AREA_DATA *new_area(void)
{
    AREA_DATA *pArea;
    char buf[MAX_INPUT_LENGTH];

    if (!area_free)
    {
        pArea   =   alloc_perm(sizeof(*pArea));
        top_area++;
    }
    else
    {
        pArea       =   area_free;
        area_free   =   area_free->next;
    }

    pArea->next             =   NULL;
    pArea->name             =   str_dup("New area");
/*    pArea->recall           =   ROOM_VNUM_TEMPLE;      ROM OLC */
    pArea->area_flags       =   AREA_ADDED;
    pArea->security         =   1;
    pArea->builders         =   str_dup("None");
    pArea->min_vnum            =   0;
    pArea->max_vnum            =   0;
    pArea->age              =   0;
    pArea->nplayer          =   0;
    pArea->empty            =   TRUE;              /* ROM patch */
    sprintf(buf, "area%d.are", pArea->vnum);
    pArea->file_name        =   str_dup(buf);
    pArea->vnum             =   top_area-1;

    return pArea;
}



void free_area(AREA_DATA *pArea)
{
    free_string(pArea->name);
    free_string(pArea->file_name);
    free_string(pArea->builders);
    free_string(pArea->credits);

    pArea->next         =   area_free;
    area_free           =   pArea;
    return;
}



EXIT_DATA *new_exit(void)
{
    EXIT_DATA *pExit;

    if (!exit_free)
    {
        pExit           =   alloc_perm(sizeof(*pExit));
        top_exit++;
    }
    else
    {
        pExit           =   exit_free;
        exit_free       =   exit_free->next;
    }

    pExit->u1.to_room   =   NULL;                  /* ROM OLC */
    pExit->next         =   NULL;
/*  pExit->vnum         =   0;                        ROM OLC */
    pExit->exit_info    =   0;
    pExit->key          =   0;
    pExit->keyword      =   str_dup("");
    pExit->description  =   str_dup("");
    pExit->rs_flags     =   0;

    return pExit;
}



void free_exit(EXIT_DATA *pExit)
{
    free_string(pExit->keyword);
    free_string(pExit->description);

    pExit->next         =   exit_free;
    exit_free           =   pExit;
    return;
}

/*
struct lua_progs *new_lprg()
{
    struct lua_progs *ret;

    ret = alloc_mem(sizeof(*ret));
    ret->next = NULL;
    ret->trig = 0;
    ret->func = str_dup("");
    ret->arg = str_dup("");

    return ret;
}

void free_lprg(struct lua_progs *lprg)
{
    free_string(lprg->func);
    free_string(lprg->arg);
    free_mem(lprg, sizeof(struct lua_progs));
    lprg = NULL;
}
*/

ROOM_INDEX_DATA *new_room_index(void)
{
    ROOM_INDEX_DATA *pRoom;
    int door;

    if (!room_index_free)
    {
        pRoom           =   alloc_perm(sizeof(*pRoom));
        top_room++;
    }
    else
    {
        pRoom           =   room_index_free;
        room_index_free =   room_index_free->next;
    }

    LIST_INIT(&pRoom->people);
    pRoom->next             =   NULL;
    pRoom->contents         =   NULL;
    pRoom->extra_descr      =   NULL;
    pRoom->area             =   NULL;

    for (door=0; door < MAX_DIR; door++)
        pRoom->exit[door]   =   NULL;

    pRoom->name             =   str_dup("");
    pRoom->description      =   str_dup("");
    pRoom->owner	    =	str_dup("");
    pRoom->comments	    =	str_dup("");
    pRoom->vnum             =   0;
    pRoom->room_flags       =   0;
    pRoom->light            =   0;
    pRoom->sector_type      =   0;
    pRoom->clan		    =	0;
    pRoom->heal_rate	    =   100;
    pRoom->mana_rate	    =   100;

/*    pRoom->lprg = NULL; */
/*    pRoom->lprg_last = NULL; */

    return pRoom;
}

void free_room_index(ROOM_INDEX_DATA *pRoom)
{
    int door;
    EXTRA_DESCR_DATA *pExtra, *wExtra;
    RESET_DATA *pReset, *wReset;
//    struct lua_progs *lprg, *lprg_next;

    free_string(pRoom->name);
    free_string(pRoom->description);
    free_string(pRoom->owner);
    free_string(pRoom->comments);

    for (door = 0; door < MAX_DIR; door++)
	if (pRoom->exit[door])
	{
	    free_exit(pRoom->exit[door]);
	    pRoom->exit[door] = NULL;
	}

    for (pExtra = pRoom->extra_descr; pExtra; pExtra = wExtra)
    {
	wExtra = pExtra->next;
	free_extra_descr(pExtra);
    }

    for (pReset = pRoom->reset_first; pReset; pReset = wReset)
    {
	wReset = pReset->next;
	free_reset_data(pReset);
    }

/*    
	for (lprg = pRoom->lprg; lprg; lprg = lprg_next)
    {
	lprg_next = lprg;
	free_lprg(lprg);
    }

    pRoom->lprg_last = NULL;
*/

    pRoom->next = room_index_free;
    room_index_free = pRoom;
    return;
}



extern AFFECT_DATA *affect_free;


SHOP_DATA *new_shop(void)
{
    SHOP_DATA *pShop;
    int buy;

    if (!shop_free)
    {
        pShop           =   alloc_perm(sizeof(*pShop));
        top_shop++;
    }
    else
    {
        pShop           =   shop_free;
        shop_free       =   shop_free->next;
    }

    pShop->next         =   NULL;
    pShop->keeper       =   0;

    for (buy=0; buy<MAX_TRADE; buy++)
        pShop->buy_type[buy]    =   0;

    pShop->profit_buy   =   100;
    pShop->profit_sell  =   100;
    pShop->open_hour    =   0;
    pShop->close_hour   =   23;

    return pShop;
}



void free_shop(SHOP_DATA *pShop)
{
    pShop->next = shop_free;
    shop_free   = pShop;
    return;
}



OBJ_INDEX_DATA *new_obj_index(void)
{
    OBJ_INDEX_DATA *pObj;
    int value;

    if (!obj_index_free)
    {
        pObj           =   alloc_perm(sizeof(*pObj));
        top_obj_index++;
    }
    else
    {
        pObj            =   obj_index_free;
        obj_index_free  =   obj_index_free->next;
    }

    pObj->next          =   NULL;
    pObj->extra_descr   =   NULL;
    pObj->affected      =   NULL;
    pObj->area          =   NULL;
    pObj->name          =   str_dup("no name");
    pObj->short_descr   =   str_dup("(no short description)");
    pObj->description   =   str_dup("(no description)");
    pObj->vnum          =   0;
    pObj->item_type     =   ITEM_TRASH;
    pObj->extra_flags   =   0;
    pObj->wear_flags    =   0;
    pObj->count         =   0;
    pObj->weight        =   0;
    pObj->cost          =   0;
    pObj->material      =   str_dup("unknown");      /* ROM */
    pObj->condition     =   100;                        /* ROM */
    pObj->uncomf        =   0;
    pObj->unusable      =   0;
    pObj->comments      =   str_dup("");
    
    for (value = 0; value < 5; value++)               /* 5 - ROM */
        pObj->value[value]  =   0;

    pObj->new_format    = TRUE; /* ROM */

    return pObj;
}



void free_obj_index(OBJ_INDEX_DATA *pObj)
{
    EXTRA_DESCR_DATA *pExtra, *wExtra;
    AFFECT_DATA *pAf, *pAf_next;
    PROG_LIST *op, *op_next;

    free_string(pObj->name);
    free_string(pObj->short_descr);
    free_string(pObj->description);
    free_string(pObj->material   );
    free_string(pObj->comments   );

    for (pAf = pObj->affected; pAf; pAf = pAf_next)
    {
        pAf_next = pAf->next;
	pAf->prev = NULL;
        free_affect(pAf);
    }

    for (pExtra = pObj->extra_descr; pExtra; pExtra = wExtra)
    {
        wExtra = pExtra->next;
        free_extra_descr(pExtra);
    }

    for (op = pObj->progs;op;op = op_next)
    {
	op_next = op->next;
	free_oprog(op);
    }
    
    pObj->next              = obj_index_free;
    obj_index_free          = pObj;
    return;
}

MOB_INDEX_DATA *new_mob_index(void)
{
    MOB_INDEX_DATA *pMob;
    int i;

    if (!mob_index_free)
    {
        pMob           =   alloc_perm(sizeof(*pMob));
        top_mob_index++;
    }
    else
    {
        pMob            =   mob_index_free;
        mob_index_free  =   mob_index_free->next;
    }

    pMob->next          =   NULL;
    pMob->spec_fun      =   NULL;
    pMob->pShop         =   NULL;
    pMob->area          =   NULL;
    pMob->player_name   =   str_dup("no name");
    pMob->short_descr   =   str_dup("(no short description)");
    pMob->long_descr    =   str_dup("(no long description)\n\r");
    pMob->description   =   str_dup("");
    pMob->comments      =   str_dup("");
    pMob->vnum          =   0;
    pMob->count         =   0;
    pMob->killed        =   0;
    pMob->sex           =   0;
    pMob->level         =   0;
    pMob->act           =   ACT_IS_NPC;
    pMob->affected_by   =   0;
    pMob->alignment     =   0;
	pMob->saving_throw	=	0;
    pMob->hitroll	=   0;
    pMob->race          =   race_lookup("human"); /* - Hugin */
    pMob->form          =   0;           /* ROM patch -- Hugin */
    pMob->parts         =   0;           /* ROM patch -- Hugin */
    pMob->material      =   str_dup("unknown"); /* -- Hugin */
    pMob->off_flags     =   0;           /* ROM patch -- Hugin */
    pMob->size          =   SIZE_MEDIUM; /* ROM patch -- Hugin */
    pMob->ac[AC_PIERCE]	=   0;           /* ROM patch -- Hugin */
    pMob->ac[AC_BASH]	=   0;           /* ROM patch -- Hugin */
    pMob->ac[AC_SLASH]	=   0;           /* ROM patch -- Hugin */
    pMob->ac[AC_EXOTIC]	=   0;           /* ROM patch -- Hugin */
    pMob->hit[DICE_NUMBER]	=   0;   /* ROM patch -- Hugin */
    pMob->hit[DICE_TYPE]	=   0;   /* ROM patch -- Hugin */
    pMob->hit[DICE_BONUS]	=   0;   /* ROM patch -- Hugin */
    pMob->mana[DICE_NUMBER]	=   0;   /* ROM patch -- Hugin */
    pMob->mana[DICE_TYPE]	=   0;   /* ROM patch -- Hugin */
    pMob->mana[DICE_BONUS]	=   0;   /* ROM patch -- Hugin */
    pMob->damage[DICE_NUMBER]	=   0;   /* ROM patch -- Hugin */
    pMob->damage[DICE_TYPE]	=   0;   /* ROM patch -- Hugin */
    pMob->damage[DICE_NUMBER]	=   0;   /* ROM patch -- Hugin */
    pMob->start_pos             =   POS_STANDING; /*  -- Hugin */
    pMob->default_pos           =   POS_STANDING; /*  -- Hugin */
    pMob->wealth                =   0;

    pMob->new_format            = TRUE;  /* ROM */

    for (i = 0; i < DAM_MAX; i++)
	pMob->resists[i] = 0;

    return pMob;
}

void free_mob_index(MOB_INDEX_DATA *pMob)
{
    PROG_LIST *list, *mp_next;

    free_string(pMob->player_name);
    free_string(pMob->short_descr);
    free_string(pMob->long_descr);
    free_string(pMob->description);
    free_string(pMob->comments);

    for (list = pMob->progs; list; list = mp_next)
    {
	mp_next = list->next;
	free_mprog(pMob->progs);
    }

    if (pMob->pShop)
	free_shop(pMob->pShop);

    pMob->next = mob_index_free;
    mob_index_free = pMob;
    return;
}




PROG_CODE              *       mpcode_free;
PROG_CODE	       *       opcode_free;
PROG_CODE	       *       rpcode_free;


PROG_CODE *new_mpcode(void)
{
    PROG_CODE *NewCode;

    if (!mpcode_free)
    {
	NewCode = alloc_perm(sizeof(*NewCode));
	top_mprog_index++;
    }
    else
    {
	NewCode     = mpcode_free;
	mpcode_free = mpcode_free->next;
    }

    NewCode->vnum    = 0;
    NewCode->code    = str_dup("");
    NewCode->next    = NULL;

    return NewCode;
}

void free_mpcode(PROG_CODE *pMcode)
{
    free_string(pMcode->code);
    pMcode->next = mpcode_free;
    mpcode_free  = pMcode;
    return;
}

PROG_CODE *new_opcode(void)
{
    PROG_CODE *NewCode;

    if (!rpcode_free)
    {
	NewCode = alloc_perm(sizeof(*NewCode));
	top_oprog_index++;
    }
    else
    {
	NewCode     = opcode_free;
	opcode_free = opcode_free->next;
    }

    NewCode->vnum    = 0;
    NewCode->code    = str_dup("");
    NewCode->next    = NULL;

    return NewCode;
}

PROG_CODE *new_rpcode(void)
{
    PROG_CODE *NewCode;

    if (!rpcode_free)
    {
	NewCode = alloc_perm(sizeof(*NewCode));
	top_rprog_index++;
    }
    else
    {
	NewCode     = rpcode_free;
	rpcode_free = rpcode_free->next;
    }

    NewCode->vnum    = 0;
    NewCode->code    = str_dup("");
    NewCode->next    = NULL;

    return NewCode;
}

void free_opcode(PROG_CODE *pOcode)
{
    free_string(pOcode->code);
    pOcode->next = opcode_free;
    opcode_free  = pOcode;
    return;
}

void free_rpcode(PROG_CODE *pRcode)
{
    free_string(pRcode->code);
    pRcode->next = rpcode_free;
    rpcode_free  = pRcode;
    return;
}

/* stuff for recyling notes */
NOTE_DATA *note_free;

NOTE_DATA *new_note()
{
    NOTE_DATA *note;

    if (note_free == NULL)
	note = alloc_perm(sizeof(*note));
    else
    { 
	note = note_free;
	note_free = note_free->next;
    }

    note->text    = str_dup("");
    note->subject = str_dup("");
    note->to_list = str_dup("");
    note->date    = str_dup("");
    note->sender  = str_dup("");
    note->vote    = NULL;
    
    VALIDATE(note);
    return note;
}

void free_note(NOTE_DATA *note)
{
    VOTE_DATA *vd, *vd_next;

    if (!IS_VALID(note))
	return;

    free_string(note->text   );
    free_string(note->subject);
    free_string(note->to_list);
    free_string(note->date   );
    free_string(note->sender );

    for(vd = note->vote; vd; vd = vd_next)
    {
        vd_next = vd->next;
        free_vote_data(vd);
    }
    INVALIDATE(note);

    note->next = note_free;
    note_free   = note;
}

    
/* stuff for recycling ban structures */
SLIST_HEAD(, ban_data) ban_free = SLIST_HEAD_INITIALIZER(ban_free);

BAN_DATA *new_ban(void)
{
    static BAN_DATA ban_zero;
    BAN_DATA *ban;

    if (SLIST_EMPTY(&ban_free))
	ban = alloc_perm(sizeof(*ban));
    else
    {
	ban = SLIST_FIRST(&ban_free);
	SLIST_REMOVE_HEAD(&ban_free, link);
    }

    *ban = ban_zero;
    VALIDATE(ban);
    return ban;
}

void free_ban(BAN_DATA *ban)
{
    if (!IS_VALID(ban))
	return;

    free_string(ban->name);
    free_string (ban->reason);
    free_string (ban->banner);

    INVALIDATE(ban);

    SLIST_INSERT_HEAD(&ban_free, ban, link);
}

/* stuff for recycling descriptors */
SLIST_HEAD(, descriptor_data) descriptor_free = SLIST_HEAD_INITIALIZER(descriptor_free);

DESCRIPTOR_DATA *new_descriptor(void)
{
    static DESCRIPTOR_DATA d_zero;
    DESCRIPTOR_DATA *d;

    if (SLIST_EMPTY(&descriptor_free))
	d = alloc_perm(sizeof(*d));
    else
    {
	d = SLIST_FIRST(&descriptor_free);
	SLIST_REMOVE_HEAD(&descriptor_free, link);
    }
	
    *d = d_zero;
    VALIDATE(d);
    return d;
}

void free_descriptor(DESCRIPTOR_DATA *d)
{
    if (!IS_VALID(d))
	return;

    free_string(d->ip);
    free_string(d->sock);

    INVALIDATE(d);
    SLIST_INSERT_HEAD(&descriptor_free, d, link);
}

/* stuff for recycling gen_data */
GEN_DATA *gen_data_free;

GEN_DATA *new_gen_data(void)
{
    static GEN_DATA gen_zero;
    GEN_DATA *gen;
    int i;

    if (gen_data_free == NULL)
	gen = alloc_perm(sizeof(*gen));
    else
    {
	gen = gen_data_free;
	gen_data_free = gen_data_free->next;
    }
    *gen = gen_zero;
    gen->skill_chosen = alloc_mem(sizeof(bool) * max_skills);

    for (i = 0; i < max_skills; i++)
	gen->skill_chosen[i] = FALSE;
    
    VALIDATE(gen);
    return gen;
}

void free_gen_data(GEN_DATA *gen)
{
    if (!IS_VALID(gen))
	return;

    INVALIDATE(gen);

    free_mem(gen->skill_chosen, sizeof(bool) * max_skills);

    gen->next = gen_data_free;
    gen_data_free = gen;
} 

/* stuff for recycling extended descs */
EXTRA_DESCR_DATA *extra_descr_free;

EXTRA_DESCR_DATA *new_extra_descr(void)
{
    EXTRA_DESCR_DATA *ed;

    if (extra_descr_free == NULL)
	ed = alloc_perm(sizeof(*ed));
    else
    {
	ed = extra_descr_free;
	extra_descr_free = extra_descr_free->next;
    }

    ed->keyword = str_dup("");
    ed->description = str_dup("");
    VALIDATE(ed);
    return ed;
}

void free_extra_descr(EXTRA_DESCR_DATA *ed)
{
    if (!IS_VALID(ed))
	return;

    free_string(ed->keyword);
    free_string(ed->description);
    INVALIDATE(ed);
    
    ed->next = extra_descr_free;
    extra_descr_free = ed;
}

SPELLAFF *new_spellaff()
{
    SPELLAFF *saff;

    saff = alloc_mem(sizeof(SPELLAFF));

    return saff;
}

/* stuff for recycling affects */
AFFECT_DATA *affect_free;

AFFECT_DATA *new_affect(void)
{
    static AFFECT_DATA af_zero;
    AFFECT_DATA *af;

    if (affect_free == NULL)
	af = alloc_perm(sizeof(AFFECT_DATA));
    else
    {
	af = affect_free;
	affect_free = affect_free->next;
    }

    *af = af_zero;

    VALIDATE(af);
    return af;
}

void free_affect(AFFECT_DATA *af)
{
    if (!IS_VALID(af))
	return;

    INVALIDATE(af);
    af->next = affect_free;
    affect_free = af;
}

/* stuff for recycling objects */
OBJ_DATA *obj_free;

OBJ_DATA *new_obj(void)
{
    static OBJ_DATA obj_zero;
    OBJ_DATA *obj;

    if (obj_free == NULL)
	obj = alloc_perm(sizeof(*obj));
    else
    {
	obj = obj_free;
	obj_free = obj_free->next;
    }
    *obj = obj_zero;
    VALIDATE(obj);

    return obj;
}

void free_obj(OBJ_DATA *obj)
{
    AFFECT_DATA *paf, *paf_next;
    EXTRA_DESCR_DATA *ed, *ed_next;

    if (!IS_VALID(obj))
	return;

    if (obj->next && obj->next->prev == obj)
    {
	bugf("free_obj(): obj->next->prev == obj (%s)", obj->name);
	obj->next->prev = obj->prev;
    }

    if (obj->prev && obj->prev->next == obj)
    {
	bugf("free_obj(): obj->prev->next == obj (%s)", obj->name);
	obj->prev->next = obj->next;
    }

    if (obj->next_content && obj->next_content->prev_content == obj)
    {
	bugf("free_obj(): obj->next_content->prev_content == obj (%s)", obj->name);
	obj->next_content->prev_content = obj->prev_content;
    }

    if (obj->prev_content && obj->prev_content->next_content == obj)
    {
	bugf("free_obj(): obj->prev_content->next_content == obj (%s)", obj->name);
	obj->prev_content->next_content = obj->next_content;
    }

    obj_count--;

    for (paf = obj->affected; paf != NULL; paf = paf_next)
    {
	paf_next = paf->next;
	paf->prev = NULL;
	free_affect(paf);
    }
    obj->affected = NULL;

    for (ed = obj->extra_descr; ed != NULL; ed = ed_next)
    {
	ed_next = ed->next;
	free_extra_descr(ed);
    }
    obj->extra_descr = NULL;
   
    free_string(obj->name       );
    free_string(obj->description);
    free_string(obj->short_descr);
    free_string(obj->owner      );
    free_string(obj->person     );
    free_string(obj->who_kill   );
    free_string(obj->material	);
    INVALIDATE(obj);

    obj->next   = obj_free;
    obj_free    = obj; 
}


/* stuff for recyling characters */
LIST_HEAD(, char_data) char_free = LIST_HEAD_INITIALIZER(char_free);

CHAR_DATA *new_char(void)
{
    static CHAR_DATA ch_zero;
    CHAR_DATA *ch;
    int i;

    if (LIST_EMPTY(&char_free))
	ch = alloc_perm(sizeof(*ch));
    else
    {
	ch = LIST_FIRST(&char_free);
	LIST_REMOVE(ch, link);
    }

    *ch				= ch_zero;
    VALIDATE(ch);
    ch->name                    = str_dup("");
    ch->short_descr             = str_dup("");
    ch->long_descr              = str_dup("");
    ch->description             = str_dup("");
    ch->notepad 		= str_dup("");
    ch->prompt                  = str_dup("");
    ch->prefix			= str_dup("");
    ch->logon                   = current_time;
    ch->lines                   = PAGELEN;
    for (i = 0; i < 4; i++)
        ch->armor[i]            = 100;
    ch->position                = POS_STANDING;
    ch->hit                     = 20;
    ch->max_hit                 = 20;
    ch->mana                    = 100;
    ch->max_mana                = 100;
    ch->move                    = 100;
    ch->max_move                = 100;
    for (i = 0; i < MAX_STATS; i ++)
    {
        ch->perm_stat[i] = 13;
        ch->mod_stat[i] = 0;
    }

    return ch;
}


void free_char (CHAR_DATA *ch)
{
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;
    GROUP_QUEST_DATA *gq, *gq_next;

    if (!IS_VALID(ch))
	return;

/*    if (ch->next && ch->next->prev == ch)
    {
	bugf("free_char(): ch->next->prev == ch (%s)", ch->name);
	ch->next->prev = ch->prev;
    }

    if (ch->prev && ch->prev->next == ch)
    {
	bugf("free_char(): ch->prev->next == ch (%s)", ch->name);
	ch->prev->next = ch->next;
    }

    if (ch->next_in_room && ch->next_in_room->prev_in_room == ch)
    {
	bugf("free_char(): ch->next_in_room->prev_in_room == ch (%s)", ch->name);
	ch->next_in_room->prev_in_room = ch->prev_in_room;
    }

    if (ch->prev_in_room && ch->prev_in_room->next_in_room == ch)
    {
	bugf("free_char(): ch->prev_in_room->next_in_room == ch (%s)", ch->name);
	ch->prev_in_room->next_in_room = ch->next_in_room;
    } */

    if (IS_NPC(ch))
	mobile_count--;

    for (obj = ch->carrying; obj != NULL; obj = obj_next)
    {
	obj_next = obj->next_content;

	if (obj->wear_loc == WEAR_LIGHT)
	    obj->value[2] = 0;

	extract_obj (obj, FALSE, FALSE);
    }

    for (paf = ch->affected; paf != NULL; paf = paf_next)
    {
	paf_next = paf->next;
	paf->prev = NULL;
	affect_remove(ch,paf);
    }

    free_string(ch->name);
    free_string(ch->short_descr);
    free_string(ch->long_descr);
    free_string(ch->description);
    free_string(ch->notepad);
    free_string(ch->prompt);
    free_string(ch->prefix);
    free_note  (ch->pnote);
    free_pcdata(ch->pcdata);
    free_string(ch->war_command);

    /* Memory leak fixes */    
    free_string(ch->material);
    
    if (ch->gen_data)
    {
        free_gen_data(ch->gen_data);
        ch->gen_data = NULL;
    }

    for (gq = ch->group_quest; gq; gq = gq_next)
    {
	gq_next = gq->next;
	free_group_quest(gq);
    }

    LIST_INSERT_HEAD(&char_free, ch, link);

    INVALIDATE(ch);
    return;
}

PC_DATA *pcdata_free;

PC_DATA *new_pcdata(void)
{
    int alias;

    static PC_DATA pcdata_zero;
    PC_DATA *pcdata;

    if (pcdata_free == NULL)
	pcdata = alloc_perm(sizeof(*pcdata));
    else
    {
	pcdata = pcdata_free;
	pcdata_free = pcdata_free->next;
    }

    *pcdata = pcdata_zero;

    for (alias = 0; alias < MAX_ALIAS; alias++)
    {
	pcdata->alias[alias] = NULL;
	pcdata->alias_sub[alias] = NULL;
    }

    pcdata->learned = alloc_mem(sizeof(int16_t) * max_skills);
    pcdata->skill_mod = alloc_mem(sizeof(int16_t) * max_skills);
    pcdata->over_skill = alloc_mem(sizeof(int32_t) * max_skills);

    for (alias = 0; alias < max_skills; alias++)
    {
	pcdata->learned[alias] = 0;
	pcdata->skill_mod[alias] = 0;
	pcdata->over_skill[alias] = 0;
    }

    pcdata->buffer = new_buf();
    pcdata->visited = NULL;
    
    VALIDATE(pcdata);
    return pcdata;
}
	

void free_pcdata(PC_DATA *pcdata){
    int alias;

    if (!IS_VALID(pcdata))
	return;

    free_string(pcdata->pwd);
    free_string(pcdata->bamfin);
    free_string(pcdata->bamfout);
    free_string(pcdata->title);
    free_buf(pcdata->buffer);

    for(alias = 0;alias < 5;alias++)
      free_string(pcdata->cases[alias]);

    free_string(pcdata->lastip);
    free_string(pcdata->genip);
    free_string(pcdata->lasthost);
    free_string(pcdata->spouse);
    free_string(pcdata->email);
    free_string(pcdata->questobjname);
    free_string(pcdata->grants);
    free_string(pcdata->freeze_who);
    free_string(pcdata->pretitle);
    free_string(pcdata->old_names);
    free_string(pcdata->reg_answer);

    for (alias = 0; alias < MAX_ALIAS; alias++)
    {
	free_string(pcdata->alias[alias]);
	free_string(pcdata->alias_sub[alias]);
    }
    
    for (alias = 0; pcdata->filter[alias].ch != NULL && alias < MAX_FILTERS; alias++)
      free_string(pcdata->filter[alias].ch);

    free_mem(pcdata->learned, sizeof(int16_t) * max_skills);
    free_mem(pcdata->skill_mod, sizeof(int16_t) * max_skills);
    free_mem(pcdata->over_skill, sizeof(int32_t) * max_skills);

    if (pcdata->visited_size > 0)
	free_mem(pcdata->visited, pcdata->visited_size);

    INVALIDATE(pcdata);
    pcdata->next = pcdata_free;
    pcdata_free = pcdata;

    return;
}

	


/* stuff for setting ids */
long	last_pc_id;
long	last_mob_id;
long	last_obj_id;

long get_pc_id(void)
{
    int val;

    val = (current_time <= last_pc_id) ? last_pc_id + 1 : current_time;
    last_pc_id = val;
    return val;
}

long get_mob_id(void)
{
    last_mob_id++;
    return last_mob_id;
}

long get_obj_id()
{
    last_obj_id++;
    return last_obj_id;
}

MEM_DATA *mem_data_free;

/* procedures and constants needed for buffering */

MEM_DATA *new_mem_data(void)
{
    MEM_DATA *memory;
  
    if (mem_data_free == NULL)
	memory = alloc_perm(sizeof(MEM_DATA));
    else
    {
	memory = mem_data_free;
	mem_data_free = mem_data_free->next;
    }

    memory->next = NULL;
    memory->id = 0;
    memory->reaction = 0;
    memory->when = 0;
    VALIDATE(memory);

    return memory;
}

void free_mem_data(MEM_DATA *memory)
{
    if (!IS_VALID(memory))
	return;

    memory->next = mem_data_free;
    mem_data_free = memory;
    INVALIDATE(memory);
}


RECIPE_DATA *recipe_free;

RECIPE_DATA *new_recipe (void)
{
    static RECIPE_DATA recipe_zero;
    RECIPE_DATA *recipe;
    int i;

    if (recipe_free == NULL)
	recipe = alloc_perm(sizeof(*recipe));
    else
    {
	recipe = recipe_free;
	recipe_free = recipe_free->next;
    }

    *recipe	  = recipe_zero;
    recipe->name  = str_dup("");
    recipe->level = 0;
    recipe->rlevel= 1;
    recipe->comp  = 0;
    recipe->type  = RECIPE_NONE;
    recipe->reserve = 0;
    for (i = 0; i != MAX_ING; i++)
	recipe->value[i] = 0;

    VALIDATE(recipe);
    return recipe;
}

void free_recipe (RECIPE_DATA *recipe)
{
    if (!IS_VALID(recipe))
	return;

    free_string(recipe->name);

    INVALIDATE(recipe);
    recipe->next = recipe_free;
    recipe_free  = recipe;
    return;
}

QUERY_DATA *query_free;

QUERY_DATA *new_query (void)
{
    static QUERY_DATA query_zero;
    QUERY_DATA *query;

    if (query_free == NULL)
	query = alloc_perm(sizeof(*query));
    else
    {
	query = query_free;
	query_free = query_free->next;
    }

    *query	  = query_zero;
    query->name  = str_dup("Без названия");
    query->text  = str_dup("");

    VALIDATE(query);
    return query;
}

void free_query (QUERY_DATA *query)
{
    if (!IS_VALID(query))
	return;

    free_string(query->name);

    INVALIDATE(query);
    query->next = query_free;
    query_free  = query;
    return;
}

CHALLENGER_DATA *challenger_free;

CHALLENGER_DATA *new_challenger (void)
{
    static CHALLENGER_DATA challenger_zero;
    CHALLENGER_DATA *challenger;

    if (challenger_free == NULL)
	challenger = alloc_perm(sizeof(*challenger));
    else
    {
	challenger = challenger_free;
	challenger_free = challenger_free->next;
    }

    *challenger	= challenger_zero;
    challenger->name = str_dup("");
    challenger->ball = 0;
    challenger->game = 0;
    challenger->sort = FALSE;

    VALIDATE(challenger);
    return challenger;
}

void free_challenger (CHALLENGER_DATA *challenger)
{
    if (!IS_VALID(challenger))
	return;

    free_string(challenger->name);

    INVALIDATE(challenger);
    challenger->next = challenger_free;
    challenger_free  = challenger;
    return;
}

SCORE_DATA *score_free;

SCORE_DATA *new_score()
{
    static SCORE_DATA score_zero;
    SCORE_DATA *score;

    if (score_free == NULL)
	score = alloc_perm(sizeof(*score));
    else
    {
	score = score_free;
	score_free = score_free->next;
    }

    *score	    = score_zero;
    score->time     = current_time;
    score->opponent = str_dup("");
    score->won      = 0;

    VALIDATE(score);
    return score;
}

void free_score(SCORE_DATA *score)
{
    if (!IS_VALID(score))
	return;

    free_string(score->opponent);
    
    INVALIDATE(score);
    score->next = score_free;
    score_free  = score;
    return;
}

SLIST_HEAD(, buf_type) buf_free = SLIST_HEAD_INITIALIZER(buf_free);

/* buffer sizes */
const int buf_size[MAX_BUF_LIST] =
{
    16, 32, 64, 128, 256, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144
};

/* local procedure for finding the next acceptable size */
/* -1 indicates out-of-boundary error */
int get_size(int val)
{
    int i;

    for (i = 0; i < MAX_BUF_LIST; i++)
	if (buf_size[i] >= val)
	{
	    return buf_size[i];
	}
    
    return -1;
}

BUFFER *new_buf()
{
    BUFFER *buffer;

    if (SLIST_EMPTY(&buf_free)) 
	buffer = alloc_perm(sizeof(*buffer));
    else
    {
	buffer = SLIST_FIRST(&buf_free);
	SLIST_REMOVE_HEAD(&buf_free, link);
    }

    buffer->state	= BUFFER_SAFE;
    buffer->size	= get_size(BASE_BUF);

    buffer->string	= alloc_mem(buffer->size);
    buffer->string[0]	= '\0';
    VALIDATE(buffer);

    return buffer;
}

BUFFER *new_buf_size(int size)
{
    BUFFER *buffer;
 
    if (SLIST_EMPTY(&buf_free))
        buffer = alloc_perm(sizeof(*buffer));
    else
    {
        buffer = SLIST_FIRST(&buf_free);
	SLIST_REMOVE_HEAD(&buf_free, link);
    }
 
    buffer->state       = BUFFER_SAFE;
    buffer->size        = get_size(size);
    if (buffer->size == -1)
    {
        bugf("new_buf: buffer size %d too large.", size);
        exit(1);
    }
    buffer->string      = alloc_mem(buffer->size);
    buffer->string[0]   = '\0';
    VALIDATE(buffer);
 
    return buffer;
}


void free_buf(BUFFER *buffer)
{
    if (!IS_VALID(buffer))
	return;

    free_mem(buffer->string, buffer->size);
    buffer->string = NULL;
    buffer->size   = 0;
    buffer->state  = BUFFER_FREED;
    INVALIDATE(buffer);

    SLIST_INSERT_HEAD(&buf_free, buffer, link);
}


bool add_buf(BUFFER *buffer, char *string)
{
    unsigned int len;
    char *oldstr;
    unsigned int oldsize;

    if (buffer->state == BUFFER_OVERFLOW) /* don't waste time on bad strings! */
	return FALSE;

    oldstr = buffer->string;
    oldsize = buffer->size;

    len = strlen(buffer->string) + strlen(string) + 1;

    while (len >= buffer->size) /* increase the buffer size */
    {
	buffer->size 	= get_size(buffer->size + 1);
	{
	    if (buffer->size == -1) /* overflow */
	    {
		buffer->size = oldsize;
		buffer->state = BUFFER_OVERFLOW;
		bugf("buffer overflow past size %d", buffer->size);
		return FALSE;
	    }
  	}
    }

    if (buffer->size != oldsize)
    {
	buffer->string	= alloc_mem(buffer->size);

	strcpy(buffer->string,oldstr);
	free_mem(oldstr,oldsize);
    }

    strcat(buffer->string,string);
    return TRUE;
}


void clear_buf(BUFFER *buffer)
{
    buffer->string[0] = '\0';
    buffer->state     = BUFFER_SAFE;
}


char *buf_string(BUFFER *buffer)
{
    return buffer->string;
}

/* stuff for recycling mobprograms */
PROG_LIST *mprog_free;
PROG_LIST *oprog_free;
PROG_LIST *rprog_free;

 
PROG_LIST *new_mprog(void)
{
   static PROG_LIST mp_zero;
   PROG_LIST *mp;

   if (mprog_free == NULL)
       mp = alloc_perm(sizeof(*mp));
   else
   {
       mp = mprog_free;
       mprog_free=mprog_free->next;
   }

   *mp = mp_zero;
   mp->vnum             = 0;
   mp->trig_type        = 0;
   mp->code             = str_dup("");
   VALIDATE(mp);
   return mp;
}

void free_mprog(PROG_LIST *mp)
{
   if (!IS_VALID(mp))
      return;

   free_string(mp->code);

   INVALIDATE(mp);
   mp->next = mprog_free;
   mprog_free = mp;
}

HELP_AREA * had_free;

HELP_AREA * new_had (void)
{
    HELP_AREA * had;

    if (had_free)
    {
        had		= had_free;
        had_free	= had_free->next;
    }
    else
        had		= alloc_perm(sizeof(*had));

    STAILQ_INIT(&had->helps);

    return had;
}

STAILQ_HEAD(, help_data) help_free = STAILQ_HEAD_INITIALIZER(help_free);

HELP_DATA * new_help (void)
{
    HELP_DATA * help;

    if (!STAILQ_EMPTY(&help_free))
    {
	help = STAILQ_FIRST(&help_free);
	STAILQ_REMOVE_HEAD(&help_free, link);
    }
    else
	help = alloc_perm(sizeof(*help));

    help->keyword = str_dup("");
    help->text    = str_dup("");
    
    return help;
}

void free_help(HELP_DATA *help)
{
    free_string(help->keyword);
    free_string(help->text);

    STAILQ_INSERT_HEAD(&help_free, help, link);
}

PROG_LIST *new_oprog(void)
{
   static PROG_LIST op_zero;
   PROG_LIST *op;

   if (oprog_free == NULL)
       op = alloc_perm(sizeof(*op));
   else
   {
       op = oprog_free;
       oprog_free=oprog_free->next;
   }

   *op = op_zero;
   op->vnum             = 0;
   op->trig_type        = 0;
   op->code             = str_dup("");
   VALIDATE(op);
   return op;
}

void free_oprog(PROG_LIST *op)
{
   if (!IS_VALID(op))
      return;

    free_string(op->code);

   INVALIDATE(op);
   op->next = oprog_free;
   oprog_free = op;
}

PROG_LIST *new_rprog(void)
{
   static PROG_LIST rp_zero;
   PROG_LIST *rp;

   if (rprog_free == NULL)
       rp = alloc_perm(sizeof(*rp));
   else
   {
       rp = rprog_free;
       rprog_free=rprog_free->next;
   }

   *rp = rp_zero;
   rp->vnum             = 0;
   rp->trig_type        = 0;
   rp->code             = str_dup("");
   VALIDATE(rp);
   return rp;
}

void free_rprog(PROG_LIST *rp)
{
   if (!IS_VALID(rp))
      return;

    free_string(rp->code);

   INVALIDATE(rp);
   rp->next = rprog_free;
   rprog_free = rp;
}

FIGHT_DATA *fights_list;

FIGHT_DATA *new_fight_data()
{
    FIGHT_DATA *fd;

    fd = alloc_mem(sizeof(FIGHT_DATA));

    if (fights_list == NULL)
	fights_list = fd;
    else
    {
	fd->next = fights_list;
	fights_list = fd;
    }

    return fd;
}

void free_fight_data(FIGHT_DATA *fd)
{
    FIGHT_DATA *i;

    if (fd == NULL)
	return;

    for (i = fights_list; i; i = i->next)
	if (i->next == fd)
	    i->next = fd->next;

    free_mem(fd, sizeof(FIGHT_DATA));
}

GROUP_QUEST_DATA *gq_free;
NAME_LIST        *nl_free;

NAME_LIST *new_name_list(void)
{
   static NAME_LIST nl_zero;
   NAME_LIST *nl;

   if (nl_free == NULL)
       nl = alloc_perm(sizeof(*nl));
   else
   {
       nl = nl_free;
       nl_free = nl_free->next;
   }

   *nl = nl_zero;
   nl->next  = NULL;
   VALIDATE(nl);
   return nl;
}

void free_name_list(NAME_LIST *nl)
{
   if (!IS_VALID(nl))
      return;

   free_string(nl->name);

   INVALIDATE(nl);
   nl->next = nl_free;
   nl_free = nl;
}

GROUP_QUEST_DATA *new_group_quest(void)
{
   static GROUP_QUEST_DATA gq_zero;
   GROUP_QUEST_DATA *gq;

   if (gq_free == NULL)
       gq = alloc_perm(sizeof(*gq));
   else
   {
       gq = gq_free;
       gq_free = gq_free->next;
   }

   *gq = gq_zero;
   gq->names = NULL;
   gq->time  = 0;
   gq->next  = NULL;
   VALIDATE(gq);
   return gq;
}

void free_group_quest(GROUP_QUEST_DATA *gq)
{
   NAME_LIST *nl, *nl_next;

   if (!IS_VALID(gq))
      return;

   for(nl = gq->names; nl; nl = nl_next)
   {
     nl_next = nl->next;
     free_name_list(nl);
   }

   INVALIDATE(gq);
   gq->next = gq_free;
   gq_free = gq;
}

CHAR_VOTES *cv_free;

CHAR_VOTES *new_char_vote(void)
{
   static CHAR_VOTES cv_zero;
   CHAR_VOTES *cv;

   if (cv_free == NULL)
       cv = alloc_perm(sizeof(*cv));
   else
   {
       cv = cv_free;
       cv_free = cv_free->next;
   }

   *cv = cv_zero;
   cv->next  = NULL;
   VALIDATE(cv);
   return cv;
}

void free_char_vote(CHAR_VOTES *cv)
{
   if (!IS_VALID(cv))
      return;

   INVALIDATE(cv);
   cv->next = cv_free;
   cv_free = cv;
}

VOTE_DATA *vd_free;

VOTE_DATA *new_vote_data(void)
{
   static VOTE_DATA vd_zero;
   VOTE_DATA *vd;

   if (vd_free == NULL)
       vd = alloc_perm(sizeof(*vd));
   else
   {
       vd = vd_free;
       vd_free = vd_free->next;
   }

   *vd = vd_zero;
   vd->next  = NULL;
   VALIDATE(vd);
   return vd;
}

void free_vote_data(VOTE_DATA *vd)
{
   CHAR_VOTES *cv, *cv_next;

   if (!IS_VALID(vd))
      return;

   free_string(vd->text);

   for (cv = vd->votes; cv != NULL; cv = cv_next)
   {
     cv_next = cv->next;
     free_char_vote(cv);
   }

   INVALIDATE(vd);
   vd->next = vd_free;
   vd_free = vd;
}


/* charset=cp1251 */








