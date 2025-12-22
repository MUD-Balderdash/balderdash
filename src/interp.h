/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,	   *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *									   *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael	   *
 *  Chastain, Michael Quan, and Mitchell Tse.				   *
 *									   *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc	   *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.						   *
 *									   *
 *  Much time and thought has gone into this software and you are	   *
 *  benefitting.  We hope that you share your changes too.  What goes	   *
 *  around, comes around.						   *
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
/* $Id: interp.h,v 1.61.2.29 2012/09/14 14:33:54 mud Exp $ */

#ifndef INTERP_H
#define INTERP_H

/* this is a listing of all the commands and command related data */

/* wrapper function for safe command execution */
void do_function args((CHAR_DATA *ch, DO_FUN *do_fun, char *argument));
int  cmd_lookup(CHAR_DATA *ch, char *command);

/* for command types */
#define ML 	MAX_LEVEL	/* implementor */
#define L1	MAX_LEVEL - 1  	/* creator */
#define L2	MAX_LEVEL - 2	/* supreme being */
#define L3	MAX_LEVEL - 3	/* deity */
#define L4 	MAX_LEVEL - 4	/* god */
#define L5	MAX_LEVEL - 5	/* immortal */
#define L6	MAX_LEVEL - 6	/* demigod */
#define L7	MAX_LEVEL - 7	/* angel */
#define L8	MAX_LEVEL - 8	/* avatar */
#define IM	LEVEL_IMMORTAL 	/* avatar */
#define HE	LEVEL_HERO	/* hero */

#define COM_INGORE	1

/*
 * Structure for a command in the command lookup table.
 */
struct	cmd_type
{
    char 	*name;
    DO_FUN	*do_fun;
    int16_t	position;
    int16_t	level;
    int16_t	log;
    int16_t	show;
};

/* the command table itself */
extern const struct cmd_type cmd_table[];

/*
 * Command functions.
 * Defined in act_*.c (mostly).
 */
DECLARE_DO_FUN(	do_absolutely   );
DECLARE_DO_FUN(	do_choice	);
DECLARE_DO_FUN(	do_advance	);
DECLARE_DO_FUN( do_affects	);
DECLARE_DO_FUN( do_afk		);
DECLARE_DO_FUN( do_alia		);
DECLARE_DO_FUN( do_alias	);
DECLARE_DO_FUN(	do_allow	);
/*DECLARE_DO_FUN( do_answer	);
*/
DECLARE_DO_FUN(	do_areas	);
DECLARE_DO_FUN(	do_at		);
DECLARE_DO_FUN(	do_auction	);
DECLARE_DO_FUN( do_autoassist	);
DECLARE_DO_FUN( do_autoexit	);
DECLARE_DO_FUN( do_autogold	);
DECLARE_DO_FUN( do_automoney	);
DECLARE_DO_FUN( do_autolist	);
DECLARE_DO_FUN( do_autoloot	);
DECLARE_DO_FUN( do_autoattack	);
DECLARE_DO_FUN( do_autosac	);
DECLARE_DO_FUN( do_autosplit	);
DECLARE_DO_FUN(	do_backstab	);
DECLARE_DO_FUN(	do_bamfin	);
DECLARE_DO_FUN(	do_bamfout	);
DECLARE_DO_FUN(	do_ban		);
DECLARE_DO_FUN( do_bash		);
DECLARE_DO_FUN( do_berserk	);
DECLARE_DO_FUN(	do_brandish	);
DECLARE_DO_FUN( do_brief	);
DECLARE_DO_FUN(	do_bug		);
DECLARE_DO_FUN(	do_todo		);
DECLARE_DO_FUN(	do_buy		);
DECLARE_DO_FUN(	do_cast		);
DECLARE_DO_FUN( do_changes	);
DECLARE_DO_FUN( do_channels	);
DECLARE_DO_FUN( do_clone	);
DECLARE_DO_FUN(	do_close_	);
DECLARE_DO_FUN(	do_colour	);
DECLARE_DO_FUN(	do_commands	);
/* DECLARE_DO_FUN( do_combine	); 
*/

DECLARE_DO_FUN( do_compact	);
DECLARE_DO_FUN(	do_compare	);
DECLARE_DO_FUN(	do_consider	);
DECLARE_DO_FUN( do_count	);
DECLARE_DO_FUN(	do_credits	);
DECLARE_DO_FUN( do_deaf		);
DECLARE_DO_FUN( do_delet	);
DECLARE_DO_FUN( do_delete	);
DECLARE_DO_FUN(	do_deny		);
DECLARE_DO_FUN(	do_cemail	);
DECLARE_DO_FUN(	do_nopk		);
DECLARE_DO_FUN(	do_success		);
DECLARE_DO_FUN(	do_full_silence	);
DECLARE_DO_FUN(	do_description	);
DECLARE_DO_FUN( do_dirt		);
DECLARE_DO_FUN(	do_disarm	);
DECLARE_DO_FUN(	do_disconnect	);
DECLARE_DO_FUN(	do_down		);
DECLARE_DO_FUN(	do_drink	);
DECLARE_DO_FUN(	do_drop		);
DECLARE_DO_FUN( do_dump		);
DECLARE_DO_FUN(	do_east		);
DECLARE_DO_FUN(	do_eat		);
DECLARE_DO_FUN(	do_echo		);
DECLARE_DO_FUN(	do_emote	);
DECLARE_DO_FUN( do_enter	);
DECLARE_DO_FUN( do_envenom	);
DECLARE_DO_FUN(	do_equipment	);
DECLARE_DO_FUN(	do_examine	);
DECLARE_DO_FUN(	do_exits	);
DECLARE_DO_FUN(	do_fill		);
DECLARE_DO_FUN( do_flag		);
DECLARE_DO_FUN(	do_flee		);
DECLARE_DO_FUN(	do_follow	);
DECLARE_DO_FUN(	do_force	);
DECLARE_DO_FUN(	do_freeze	);
DECLARE_DO_FUN( do_gain		);
DECLARE_DO_FUN(	do_get		);
DECLARE_DO_FUN(	do_give		);
DECLARE_DO_FUN( do_gossip	);
DECLARE_DO_FUN(	do_goto		);
DECLARE_DO_FUN( do_grats	);
DECLARE_DO_FUN(	do_group	);
DECLARE_DO_FUN( do_groups	);
DECLARE_DO_FUN(	do_gtell	);
DECLARE_DO_FUN( do_heal		);
DECLARE_DO_FUN(	do_help		);
DECLARE_DO_FUN(	do_hide		);
DECLARE_DO_FUN(	do_holylight	);
DECLARE_DO_FUN(	do_idea		);
DECLARE_DO_FUN(	do_immtalk	);
DECLARE_DO_FUN(	do_racetalk	);
DECLARE_DO_FUN(	do_btalk	);
DECLARE_DO_FUN(	do_ttalk	);
DECLARE_DO_FUN( do_incognito	);
DECLARE_DO_FUN( do_clantalk	);
DECLARE_DO_FUN( do_clantalkunion);
DECLARE_DO_FUN( do_imotd	);
DECLARE_DO_FUN(	do_inventory	);
DECLARE_DO_FUN(	do_invis	);
DECLARE_DO_FUN(	do_kick		);
DECLARE_DO_FUN(	do_kill		);
DECLARE_DO_FUN(	do_list		);
DECLARE_DO_FUN( do_load		);
DECLARE_DO_FUN(	do_lock		);
DECLARE_DO_FUN(	do_log		);
DECLARE_DO_FUN(	do_look		);
DECLARE_DO_FUN(	do_suicid	);
DECLARE_DO_FUN(	do_suicide	);
DECLARE_DO_FUN(	do_memory	);
DECLARE_DO_FUN(	do_mfind	);
DECLARE_DO_FUN(	do_mload	);
DECLARE_DO_FUN(	do_mset		);
DECLARE_DO_FUN(	do_mwhere	);
DECLARE_DO_FUN( do_mob		);
DECLARE_DO_FUN( do_motd		);
DECLARE_DO_FUN( do_mpstat	);
DECLARE_DO_FUN( do_mpdump	);
DECLARE_DO_FUN( do_music	);
DECLARE_DO_FUN( do_declaim	);
DECLARE_DO_FUN( do_newlock	);
DECLARE_DO_FUN( do_news		);
DECLARE_DO_FUN( do_nochannels	);
DECLARE_DO_FUN( do_extranochannels	);
DECLARE_DO_FUN(	do_noemote	);
DECLARE_DO_FUN( do_nofollow	);
/* DECLARE_DO_FUN( do_noloot	); 
*/
DECLARE_DO_FUN(	do_north	);
DECLARE_DO_FUN(	do_noshout	);
DECLARE_DO_FUN(	do_note		);
DECLARE_DO_FUN(	do_notell	);
DECLARE_DO_FUN(	do_ofind	);
DECLARE_DO_FUN(	do_oload	);
DECLARE_DO_FUN(	do_open_	);
DECLARE_DO_FUN(	do_order	);
DECLARE_DO_FUN(	do_oset		);
DECLARE_DO_FUN( do_outfit	);
DECLARE_DO_FUN( do_owhere	);
DECLARE_DO_FUN(	do_pardon	);
DECLARE_DO_FUN(	do_password	);
DECLARE_DO_FUN(	do_peace	);
DECLARE_DO_FUN( do_pecho	);
DECLARE_DO_FUN( do_penalty	);
DECLARE_DO_FUN( do_permban	);
DECLARE_DO_FUN(	do_pick		);
DECLARE_DO_FUN( do_play		);
/* DECLARE_DO_FUN( do_pmote	); 
*/
DECLARE_DO_FUN( do_pour		);
DECLARE_DO_FUN(	do_practice	);
DECLARE_DO_FUN( do_prefi	);
DECLARE_DO_FUN( do_prefix	);
DECLARE_DO_FUN( do_prompt	);
DECLARE_DO_FUN( do_protect	);
DECLARE_DO_FUN(	do_purge	);
DECLARE_DO_FUN(	do_put		);
DECLARE_DO_FUN(	do_quaff	);
/*DECLARE_DO_FUN( do_question	);
*/
DECLARE_DO_FUN(	do_qui		);
DECLARE_DO_FUN( do_quiet	);
DECLARE_DO_FUN(	do_quit		);
/* DECLARE_DO_FUN( do_quote	); 
*/
DECLARE_DO_FUN( do_read		);
DECLARE_DO_FUN(	do_reboo	);
DECLARE_DO_FUN(	do_reboot	);
DECLARE_DO_FUN(	do_recall	);
DECLARE_DO_FUN(	do_recho	);
DECLARE_DO_FUN(	do_recite	);
DECLARE_DO_FUN(	do_remove	);
DECLARE_DO_FUN(	do_rent		);
DECLARE_DO_FUN( do_replay	);
DECLARE_DO_FUN(	do_reply	);
DECLARE_DO_FUN(	do_report	);
DECLARE_DO_FUN(	do_buy_add	);
DECLARE_DO_FUN(	do_rescue	);
DECLARE_DO_FUN(	do_rest		);
DECLARE_DO_FUN(	do_restore	);
DECLARE_DO_FUN(	do_return	);
DECLARE_DO_FUN(	do_rset		);
DECLARE_DO_FUN( do_rules	);
DECLARE_DO_FUN( do_rwhere	);
DECLARE_DO_FUN(	do_sacrifice	);
DECLARE_DO_FUN(	do_save		);
DECLARE_DO_FUN(	do_say		);
DECLARE_DO_FUN(	do_scan		);
DECLARE_DO_FUN(	do_score	);
DECLARE_DO_FUN( do_scroll	);
DECLARE_DO_FUN(	do_sell		);
DECLARE_DO_FUN( do_set		);
DECLARE_DO_FUN(	do_shout	);
DECLARE_DO_FUN( do_show		);
DECLARE_DO_FUN(	do_shutdow	);
DECLARE_DO_FUN(	do_shutdown	);
DECLARE_DO_FUN( do_sit		);
DECLARE_DO_FUN( do_skills	);
DECLARE_DO_FUN(	do_sla		);
DECLARE_DO_FUN(	do_slay		);
DECLARE_DO_FUN(	do_sleep	);
DECLARE_DO_FUN(	do_slookup	);
/* DECLARE_DO_FUN( do_smote	); 
*/
DECLARE_DO_FUN(	do_sneak	);
DECLARE_DO_FUN(	do_snoop	);
DECLARE_DO_FUN( do_socials	);
DECLARE_DO_FUN(	do_south	);
DECLARE_DO_FUN( do_sockets	);
DECLARE_DO_FUN( do_spells	);
DECLARE_DO_FUN(	do_split	);
DECLARE_DO_FUN(	do_sset		);
DECLARE_DO_FUN(	do_stand	);
DECLARE_DO_FUN( do_stat		);
DECLARE_DO_FUN(	do_steal	);
DECLARE_DO_FUN( do_story	);
DECLARE_DO_FUN( do_string	);
/* DECLARE_DO_FUN(	do_surrender	); 
*/
DECLARE_DO_FUN(	do_switch	);
DECLARE_DO_FUN(	do_tell_	);
DECLARE_DO_FUN(	do_time		);
DECLARE_DO_FUN(	do_systime	);
DECLARE_DO_FUN(	do_title	);
DECLARE_DO_FUN(	do_train	);
DECLARE_DO_FUN(	do_transfer	);
DECLARE_DO_FUN( do_trip		);
DECLARE_DO_FUN(	do_trust	);
DECLARE_DO_FUN(	do_typo		);
DECLARE_DO_FUN( do_unalias	);
DECLARE_DO_FUN(	do_unlock	);
DECLARE_DO_FUN( do_unread	);
DECLARE_DO_FUN(	do_up		);
DECLARE_DO_FUN(	do_value	);
DECLARE_DO_FUN(	do_visible	);
DECLARE_DO_FUN(	do_unfly	);
DECLARE_DO_FUN( do_violate	);
DECLARE_DO_FUN( do_vnum		);
DECLARE_DO_FUN(	do_wake		);
DECLARE_DO_FUN(	do_wear		);
DECLARE_DO_FUN(	do_weather	);
DECLARE_DO_FUN(	do_west		);
DECLARE_DO_FUN(	do_where	);
DECLARE_DO_FUN(	do_who		);
DECLARE_DO_FUN( do_whois	);
DECLARE_DO_FUN(	do_wimpy	);
DECLARE_DO_FUN(	do_wizhelp	);
DECLARE_DO_FUN(	do_wizlock	);
DECLARE_DO_FUN( do_wizlist	);
DECLARE_DO_FUN( do_wiznet	);
DECLARE_DO_FUN( do_worth	);
DECLARE_DO_FUN(	do_yell		);
DECLARE_DO_FUN(	do_zap		);
DECLARE_DO_FUN( do_zecho	);

DECLARE_DO_FUN( do_good_look	);
DECLARE_DO_FUN( do_cases	);
DECLARE_DO_FUN( do_lunge	);
DECLARE_DO_FUN( do_balance	);
DECLARE_DO_FUN( do_withdraw	);
DECLARE_DO_FUN( do_deposit	);
DECLARE_DO_FUN( do_autotitle	);
DECLARE_DO_FUN( do_port		);
DECLARE_DO_FUN( do_sharpen	);
DECLARE_DO_FUN( do_csharpen	);
DECLARE_DO_FUN( do_rename	);
DECLARE_DO_FUN( do_bounty	);
DECLARE_DO_FUN( do_addapply	);
DECLARE_DO_FUN( do_limits	);

DECLARE_DO_FUN( do_identify	);
DECLARE_DO_FUN( do_wpeace	);
DECLARE_DO_FUN( do_travel	);

DECLARE_DO_FUN( do_shortestpath );
DECLARE_DO_FUN( do_immquest 	);
DECLARE_DO_FUN( do_charquest 	);

DECLARE_DO_FUN( do_cleave 	);
DECLARE_DO_FUN( do_setalign 	);

DECLARE_DO_FUN( do_olevel 	);
DECLARE_DO_FUN( do_mlevel 	);
DECLARE_DO_FUN( do_otype 	);

DECLARE_DO_FUN( do_quest 	);
DECLARE_DO_FUN( do_immtitle 	);
DECLARE_DO_FUN( do_clan 	);

DECLARE_DO_FUN( do_popularity	);

DECLARE_DO_FUN( do_olc		);
DECLARE_DO_FUN( do_asave	);
DECLARE_DO_FUN( do_alist	);
DECLARE_DO_FUN( do_resets	);
DECLARE_DO_FUN( do_redit	);
DECLARE_DO_FUN( do_aedit	);
DECLARE_DO_FUN( do_medit	);
DECLARE_DO_FUN( do_oedit	);
DECLARE_DO_FUN( do_mpedit	);
DECLARE_DO_FUN( do_pass		);
DECLARE_DO_FUN( do_lore		);
DECLARE_DO_FUN( do_opedit	);
DECLARE_DO_FUN( do_rpedit	);
DECLARE_DO_FUN( do_hedit	);
DECLARE_DO_FUN( do_hsave	);
DECLARE_DO_FUN( do_skedit	);
DECLARE_DO_FUN( do_sksave	);
DECLARE_DO_FUN( do_gredit	);
DECLARE_DO_FUN( do_grsave	);
DECLARE_DO_FUN( do_cledit	);
DECLARE_DO_FUN( do_clsave	);
DECLARE_DO_FUN( do_raedit	);
DECLARE_DO_FUN( do_rasave	);
DECLARE_DO_FUN( do_opdump	);
DECLARE_DO_FUN( do_opstat	);
DECLARE_DO_FUN( do_rpdump	);
DECLARE_DO_FUN( do_rpstat	);
DECLARE_DO_FUN( do_excempt	);
DECLARE_DO_FUN( do_arealinks	);
DECLARE_DO_FUN( do_house	);

DECLARE_DO_FUN( do_tail		);
DECLARE_DO_FUN( do_fight_off	);
DECLARE_DO_FUN( do_filter	);
DECLARE_DO_FUN( do_races	);
DECLARE_DO_FUN( do_check	);
DECLARE_DO_FUN( do_doas		);
DECLARE_DO_FUN( do_security	);
DECLARE_DO_FUN( do_whowas	);

DECLARE_DO_FUN(do_marry		);
DECLARE_DO_FUN(do_divorce	);
DECLARE_DO_FUN(do_spousetalk	);
DECLARE_DO_FUN(do_consent	);

DECLARE_DO_FUN(do_detect_hidden );
DECLARE_DO_FUN(do_gaffect	);

DECLARE_DO_FUN( do_mount	);
DECLARE_DO_FUN( do_dismount	);

DECLARE_DO_FUN( do_autodreams	);

DECLARE_DO_FUN( do_host		);

DECLARE_DO_FUN( do_fileident	);

DECLARE_DO_FUN( do_style	);
DECLARE_DO_FUN( do_ooc		);
DECLARE_DO_FUN( do_slang	);

DECLARE_DO_FUN( do_deafen	);

DECLARE_DO_FUN( do_loadquester	);

DECLARE_DO_FUN(	do_reinforce	);
DECLARE_DO_FUN(	do_select	);
DECLARE_DO_FUN(	do_activate	);
DECLARE_DO_FUN(	do_camp		);
DECLARE_DO_FUN(	do_herbs	);
DECLARE_DO_FUN(	do_track	);

DECLARE_DO_FUN( do_camouflage	);
DECLARE_DO_FUN( do_attention	);
DECLARE_DO_FUN( do_ambush	);
DECLARE_DO_FUN( do_butcher	);
DECLARE_DO_FUN( do_find_spring	);

DECLARE_DO_FUN( do_noreply	);
DECLARE_DO_FUN( do_trap		);
DECLARE_DO_FUN( do_torch	);

DECLARE_DO_FUN( do_disarm_trap	);
DECLARE_DO_FUN( do_hoof		);

DECLARE_DO_FUN( do_email	);
DECLARE_DO_FUN( do_repair       );
DECLARE_DO_FUN( do_immdamage    );
DECLARE_DO_FUN( do_tick		);
DECLARE_DO_FUN( do_nocast	);

DECLARE_DO_FUN( do_levitate	);
DECLARE_DO_FUN( do_bite		);
DECLARE_DO_FUN( do_gods_curse	);
DECLARE_DO_FUN( do_skillaffect	);

DECLARE_DO_FUN( do_nocancel	);
DECLARE_DO_FUN( do_last_com	);

DECLARE_DO_FUN( do_look_sky	);

DECLARE_DO_FUN( do_deathtraps	);
DECLARE_DO_FUN( do_resetlist	);

DECLARE_DO_FUN( do_weigh	);
DECLARE_DO_FUN( do_pload	);
DECLARE_DO_FUN( do_punload	);

DECLARE_DO_FUN( do_immskills	);
DECLARE_DO_FUN( do_immspells	);
DECLARE_DO_FUN( do_grant	);
DECLARE_DO_FUN( do_immprompt	);
DECLARE_DO_FUN( do_gdb		);
DECLARE_DO_FUN( do_ident	);
DECLARE_DO_FUN( do_translit	);
DECLARE_DO_FUN( do_run		);
DECLARE_DO_FUN( do_gallop	);
DECLARE_DO_FUN( do_immcompare   );

DECLARE_DO_FUN( do_scribe	);
DECLARE_DO_FUN( do_beep		);
DECLARE_DO_FUN( do_bandage	);

DECLARE_DO_FUN( do_gift		);

/*команды приглашения/отказа от него домой
*/

DECLARE_DO_FUN( do_invite	);
DECLARE_DO_FUN( do_refuse	);

/*команды постройки дома.
*/

DECLARE_DO_FUN( do_build_room	);
DECLARE_DO_FUN( do_build_mob	);
DECLARE_DO_FUN( do_build_obj	);

DECLARE_DO_FUN( do_lookmap	);

DECLARE_DO_FUN( do_players	);
DECLARE_DO_FUN( do_nonotes	);
DECLARE_DO_FUN( do_notitle	);

DECLARE_DO_FUN( do_make_bag	);
DECLARE_DO_FUN( do_immclantalk	);
DECLARE_DO_FUN( do_immracetalk  );
DECLARE_DO_FUN( do_repent	);

DECLARE_DO_FUN( do_insert	);
//DECLARE_DO_FUN( do_arena        );
DECLARE_DO_FUN( do_socketing	);
DECLARE_DO_FUN( do_renaming	);
DECLARE_DO_FUN( do_recipe       );

DECLARE_DO_FUN( do_iptalk	);
DECLARE_DO_FUN( do_email_pass	);

DECLARE_DO_FUN( do_cedit	);

DECLARE_DO_FUN(do_attract_attention);

DECLARE_DO_FUN( do_helper	);
DECLARE_DO_FUN( do_slot		);

DECLARE_DO_FUN( do_dig		);

DECLARE_DO_FUN( do_offense	);
DECLARE_DO_FUN( do_sayspells	);
DECLARE_DO_FUN( do_blood_ritual );
DECLARE_DO_FUN( do_concentrate );
DECLARE_DO_FUN( do_prayer );
DECLARE_DO_FUN( do_grimuar );
DECLARE_DO_FUN( do_collection 	);
DECLARE_DO_FUN( do_exchange 	);
DECLARE_DO_FUN( do_mix 		);
DECLARE_DO_FUN( do_pound 	);

DECLARE_DO_FUN( do_spirit 	);
DECLARE_DO_FUN( do_form 	);
DECLARE_DO_FUN( do_howl 	);
DECLARE_DO_FUN( do_growl 	);
DECLARE_DO_FUN( do_claws	);
DECLARE_DO_FUN( do_find_victim 	);
DECLARE_DO_FUN( do_silent_step 	);
DECLARE_DO_FUN( do_jump 	);
DECLARE_DO_FUN( do_clutch_lycanthrope 	);

DECLARE_DO_FUN( do_setip	);
DECLARE_DO_FUN( do_noexp	);
DECLARE_DO_FUN( do_camomile	);

DECLARE_DO_FUN( do_pierce	);
DECLARE_DO_FUN( do_reg_answer );

DECLARE_DO_FUN( do_cover	);
/*функция установки команды выполнения при лосте чара и попадания того в бой
*/

DECLARE_DO_FUN( do_lost_command	);
/*функция свадьбы у моба
*/

DECLARE_DO_FUN( do_automarry	);

DECLARE_DO_FUN( do_whoislist	);
DECLARE_DO_FUN( do_map		);

DECLARE_DO_FUN( do_wield	);

DECLARE_DO_FUN( do_titl		);
DECLARE_DO_FUN( do_descriptio	);
DECLARE_DO_FUN( do_char_notepa	);
DECLARE_DO_FUN( do_char_notepad	);
DECLARE_DO_FUN( do_immnotepad	);


#if defined(ONEUSER)
DECLARE_DO_FUN( do_promote	);
#endif

DECLARE_DO_FUN( do_votes	);
DECLARE_DO_FUN( do_temp_RIP	);
DECLARE_DO_FUN( do_immaffects	);


#endif /* INTERP_H */

/* charset=cp1251 */




