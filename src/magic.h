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
/* $Id: magic.h,v 1.22.2.7 2009/06/27 20:51:31 key Exp $ */

#ifndef MAGIC_H
#define MAGIC_H

bool check_dispel args ( ( int dis_level, CHAR_DATA *victim, int sn) );
void check_offensive(int sn, int target, CHAR_DATA *ch, CHAR_DATA *victim, bool tar);
bool move_for_recall(CHAR_DATA *victim, ROOM_INDEX_DATA *location);
bool is_replace(int sn, int level, CHAR_DATA *victim);
/*
 * Spell functions.
 * Defined in magic.c.
 */
DECLARE_SPELL_FUN(	spell_null		);
DECLARE_SPELL_FUN(	spell_acid_blast	);
DECLARE_SPELL_FUN(	spell_die_sun	);
DECLARE_SPELL_FUN(	spell_swarm_insect	);
DECLARE_SPELL_FUN(	spell_armor		);
DECLARE_SPELL_FUN(	spell_bless		);
DECLARE_SPELL_FUN(	spell_blindness		);
DECLARE_SPELL_FUN(	spell_burning_hands	);
DECLARE_SPELL_FUN(	spell_call_lightning	);
DECLARE_SPELL_FUN(      spell_calm		);
DECLARE_SPELL_FUN(      spell_cancellation	);
DECLARE_SPELL_FUN(	spell_cause_critical	);
DECLARE_SPELL_FUN(	spell_cause_light	);
DECLARE_SPELL_FUN(	spell_cause_serious	);
DECLARE_SPELL_FUN(	spell_change_sex	);
DECLARE_SPELL_FUN(      spell_chain_lightning   );
DECLARE_SPELL_FUN(	spell_charm_person	);
DECLARE_SPELL_FUN(	spell_chill_touch	);
DECLARE_SPELL_FUN(	spell_colour_spray	);
DECLARE_SPELL_FUN(	spell_continual_light	);
DECLARE_SPELL_FUN(	spell_control_weather	);
DECLARE_SPELL_FUN(	spell_create_food	);
DECLARE_SPELL_FUN(	spell_create_rose	);
DECLARE_SPELL_FUN(	spell_create_spring	);
DECLARE_SPELL_FUN(	spell_create_water	);
DECLARE_SPELL_FUN(	spell_cure_blindness	);
DECLARE_SPELL_FUN(	spell_cure_critical	);
DECLARE_SPELL_FUN(      spell_cure_disease	);
DECLARE_SPELL_FUN(	spell_cure_light	);
DECLARE_SPELL_FUN(	spell_cure_poison	);
DECLARE_SPELL_FUN(	spell_cure_serious	);
DECLARE_SPELL_FUN(	spell_curse		);
DECLARE_SPELL_FUN(      spell_demonfire		);
DECLARE_SPELL_FUN(	spell_detect_evil	);
DECLARE_SPELL_FUN(	spell_detect_good	);
DECLARE_SPELL_FUN(	spell_detect_hidden	);
DECLARE_SPELL_FUN(	spell_detect_invis	);
DECLARE_SPELL_FUN(	spell_detect_magic	);
DECLARE_SPELL_FUN(	spell_detect_poison	);
DECLARE_SPELL_FUN(	spell_dispel_evil	);
DECLARE_SPELL_FUN(      spell_dispel_good       );
DECLARE_SPELL_FUN(	spell_dispel_magic	);
DECLARE_SPELL_FUN(	spell_earthquake	);
DECLARE_SPELL_FUN(	spell_enchant_armor	);
DECLARE_SPELL_FUN(	spell_enchant_weapon	);
DECLARE_SPELL_FUN(	spell_energy_drain	);
DECLARE_SPELL_FUN(	spell_faerie_fire	);
DECLARE_SPELL_FUN(	spell_faerie_fog	);
DECLARE_SPELL_FUN(	spell_farsight		);
DECLARE_SPELL_FUN(	spell_fireball		);
DECLARE_SPELL_FUN(	spell_fireproof		);
DECLARE_SPELL_FUN(	spell_flamestrike	);
DECLARE_SPELL_FUN(	spell_floating_disc	);
DECLARE_SPELL_FUN(	spell_fly		);
DECLARE_SPELL_FUN(      spell_frenzy		);
DECLARE_SPELL_FUN(	spell_gate		);
DECLARE_SPELL_FUN(	spell_giant_strength	);
DECLARE_SPELL_FUN(	spell_harm		);
DECLARE_SPELL_FUN(      spell_haste		);
DECLARE_SPELL_FUN(	spell_heal		);
DECLARE_SPELL_FUN(	spell_heat_metal	);
DECLARE_SPELL_FUN(      spell_holy_word		);
DECLARE_SPELL_FUN(	spell_identify		);
DECLARE_SPELL_FUN(	spell_infravision	);
DECLARE_SPELL_FUN(	spell_invis		);
DECLARE_SPELL_FUN(	spell_know_alignment	);
DECLARE_SPELL_FUN(	spell_lightning_bolt	);
DECLARE_SPELL_FUN(	spell_locate_object	);
DECLARE_SPELL_FUN(	spell_magic_missile	);
DECLARE_SPELL_FUN(      spell_mass_healing	);
DECLARE_SPELL_FUN(	spell_mass_invis	);
DECLARE_SPELL_FUN(	spell_nexus		);
DECLARE_SPELL_FUN(	spell_pass_door		);
DECLARE_SPELL_FUN(      spell_plague		);
DECLARE_SPELL_FUN(	spell_poison		);
DECLARE_SPELL_FUN(	spell_portal		);
DECLARE_SPELL_FUN(	spell_protection_evil	);
DECLARE_SPELL_FUN(	spell_protection_good	);
DECLARE_SPELL_FUN(	spell_ray_of_truth	);
DECLARE_SPELL_FUN(	spell_recharge		);
DECLARE_SPELL_FUN(	spell_refresh		);
DECLARE_SPELL_FUN(	spell_remove_curse	);
DECLARE_SPELL_FUN(	spell_sanctuary		);
DECLARE_SPELL_FUN(	spell_shocking_grasp	);
DECLARE_SPELL_FUN(	spell_shield		);
DECLARE_SPELL_FUN(	spell_sleep		);
DECLARE_SPELL_FUN(	spell_slow		);
DECLARE_SPELL_FUN(	spell_stone_skin	);
DECLARE_SPELL_FUN(	spell_summon		);
DECLARE_SPELL_FUN(	spell_teleport		);
DECLARE_SPELL_FUN(	spell_ventriloquate	);
DECLARE_SPELL_FUN(	spell_weaken		);
DECLARE_SPELL_FUN(	spell_word_of_recall	);
DECLARE_SPELL_FUN(	spell_acid_breath	);
DECLARE_SPELL_FUN(	spell_fire_breath	);
DECLARE_SPELL_FUN(	spell_frost_breath	);
DECLARE_SPELL_FUN(	spell_gas_breath	);
DECLARE_SPELL_FUN(	spell_lightning_breath	);
/*DECLARE_SPELL_FUN(	spell_general_purpose	);*/
/*DECLARE_SPELL_FUN(	spell_high_explosive	);*/

DECLARE_SPELL_FUN(	spell_advance_skill	);
DECLARE_SPELL_FUN(	spell_resurrect		);
DECLARE_SPELL_FUN(	spell_control_undead	);
DECLARE_SPELL_FUN(	spell_detect_undead	);
DECLARE_SPELL_FUN(	spell_power_kill	);
DECLARE_SPELL_FUN(	spell_beacon		);
DECLARE_SPELL_FUN(	spell_find_beacon	);
DECLARE_SPELL_FUN(	spell_beacon_identify	);
DECLARE_SPELL_FUN(	spell_power_word_fear	);
DECLARE_SPELL_FUN(	spell_call_horse	);
DECLARE_SPELL_FUN(	spell_call_horse	);
DECLARE_SPELL_FUN(	spell_aura_of_fear	);
DECLARE_SPELL_FUN(	spell_fire_shield	);
DECLARE_SPELL_FUN(	spell_ice_shield	);
DECLARE_SPELL_FUN(	spell_coniferous_shield);
DECLARE_SPELL_FUN(	spell_bark_skin		);
DECLARE_SPELL_FUN(	spell_forestshield	);
DECLARE_SPELL_FUN(	spell_pathfinding	);
DECLARE_SPELL_FUN(	spell_detect_camouflage	);
DECLARE_SPELL_FUN(	spell_charm_animal	);
DECLARE_SPELL_FUN(	spell_puffball		);
DECLARE_SPELL_FUN(	spell_detect_animal	);
DECLARE_SPELL_FUN(	spell_animal_taming	);
DECLARE_SPELL_FUN(	spell_needlestorm	);
DECLARE_SPELL_FUN(	spell_hunger		);
DECLARE_SPELL_FUN(	spell_create_scimitar	);
DECLARE_SPELL_FUN(	spell_thornwrack        );
DECLARE_SPELL_FUN(	spell_swamp		);
DECLARE_SPELL_FUN(	spell_camouflage	);
DECLARE_SPELL_FUN(	spell_earthmaw		);
DECLARE_SPELL_FUN(	spell_animate_tree	);
DECLARE_SPELL_FUN(	spell_call_animal	);
DECLARE_SPELL_FUN(	spell_shapechange	);
DECLARE_SPELL_FUN(	spell_detect_landscape	);
DECLARE_SPELL_FUN(	spell_cultivate_forest	);
DECLARE_SPELL_FUN(	spell_stunning_word	);
DECLARE_SPELL_FUN(	spell_wrath		);
DECLARE_SPELL_FUN(	spell_bone_shield	);
DECLARE_SPELL_FUN(	spell_make_old		);
DECLARE_SPELL_FUN(	spell_wind_of_death	);
DECLARE_SPELL_FUN(	spell_aura_of_dust	);
DECLARE_SPELL_FUN(	spell_bone_whirlwind	);
DECLARE_SPELL_FUN(	spell_blood_signs	);
DECLARE_SPELL_FUN(	spell_nets		);
DECLARE_SPELL_FUN(	spell_turn_undead	);
DECLARE_SPELL_FUN(      spell_soul_frenzy	);
DECLARE_SPELL_FUN(      spell_fade		);
DECLARE_SPELL_FUN(      spell_dead_warrior	);
DECLARE_SPELL_FUN(      spell_protection_light	);
DECLARE_SPELL_FUN(      spell_vampiric_touch	);
DECLARE_SPELL_FUN(      spell_mist		);
DECLARE_SPELL_FUN(      spell_darkness		);
DECLARE_SPELL_FUN(      spell_trickle		);
DECLARE_SPELL_FUN(      spell_order		);
DECLARE_SPELL_FUN(      spell_death_aura	);
DECLARE_SPELL_FUN(	spell_acid_rain		);
DECLARE_SPELL_FUN(	spell_vam_blast		);
DECLARE_SPELL_FUN(	spell_nightfall		);
DECLARE_SPELL_FUN(      spell_sense_life	);
DECLARE_SPELL_FUN(      spell_shadow_cloak	);
DECLARE_SPELL_FUN(	spell_roots		);
DECLARE_SPELL_FUN(	spell_cursed_lands	);
DECLARE_SPELL_FUN(	spell_vaccine		);
DECLARE_SPELL_FUN(	spell_mirror_image	);
DECLARE_SPELL_FUN(	spell_holy_shield	);
DECLARE_SPELL_FUN(	spell_evil_aura		);
DECLARE_SPELL_FUN(	spell_identify_corpse	);
DECLARE_SPELL_FUN(	spell_preserve		);
DECLARE_SPELL_FUN(	spell_restore_mana	);
DECLARE_SPELL_FUN(	spell_generic		);
DECLARE_SPELL_FUN(	spell_sober		);
DECLARE_SPELL_FUN(	spell_ask_nature	);
DECLARE_SPELL_FUN(	spell_forest_tport	);
DECLARE_SPELL_FUN(	spell_st_aura		);
DECLARE_SPELL_FUN(	spell_bless_forest	);
DECLARE_SPELL_FUN(	spell_diagnosis		);
DECLARE_SPELL_FUN(	spell_ghostaura		);
DECLARE_SPELL_FUN(	spell_manaleak		);
DECLARE_SPELL_FUN(	spell_protection_sphere );
DECLARE_SPELL_FUN(	spell_protection_elements );
DECLARE_SPELL_FUN(	spell_absorb_thing	);
DECLARE_SPELL_FUN(	spell_stinking_cloud	);
DECLARE_SPELL_FUN(	spell_flame_cloud	);
DECLARE_SPELL_FUN(	spell_acid_fog		);
DECLARE_SPELL_FUN(	spell_slime		);
DECLARE_SPELL_FUN(	spell_gritstorm		);
DECLARE_SPELL_FUN(	spell_energy_potion	);
DECLARE_SPELL_FUN(	spell_create_mortar	);
DECLARE_SPELL_FUN(	spell_rename_potion	);
DECLARE_SPELL_FUN(	spell_create_potion	);
DECLARE_SPELL_FUN(	spell_create_creature	);

DECLARE_SPELL_FUN(	spell_spirit_simbol	);
DECLARE_SPELL_FUN(	spell_hair	);
DECLARE_SPELL_FUN(	spell_call_bear	);
DECLARE_SPELL_FUN(	spell_call_wolf	);
DECLARE_SPELL_FUN(	spell_bear_strong	);
DECLARE_SPELL_FUN(	spell_animal_dodge	);
DECLARE_SPELL_FUN(	spell_coarse_leather	);

DECLARE_SPELL_FUN(	spell_dissolve	);

DECLARE_SPELL_FUN(	spell_earthen_whirlwind	);
DECLARE_SPELL_FUN(	spell_waterfall		);
DECLARE_SPELL_FUN(	spell_ray_of_light	);
DECLARE_SPELL_FUN(	spell_ultrasound	);
DECLARE_SPELL_FUN(	spell_clarification	);

#endif /* MAGIC_H */



/* charset=cp1251 */


