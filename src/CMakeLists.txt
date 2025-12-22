cmake_minimum_required(VERSION 3.12)
project(balderdash)

find_package(PkgConfig)
find_library(DL_LIB dl REQUIRED)
find_package(ZLIB REQUIRED)
pkg_check_modules(mysqlclient mysqlclient REQUIRED)

include_directories(${mysqlclient_INCLUDE_DIRS})

add_definitions(-D_GNU_SOURCE)
set(SRCS
    act_comm.c act_info.c act_move.c act_obj.c act_wiz.c
    alias.c ban.c comm.c const.c db.c db2.c effects.c fight.c flags.c
    house.c handler.c healer.c interp.c note.c lookup.c magic.c magic2.c
    music.c recycle.c save.c scan.c skills.c special.c tables.c time.c
    update.c mob_cmds.c mob_prog.c string.c
    olc.c olc_act.c olc_save.c bank.c drunk.c summoner.c quest.c
    olc_mpcode.c clan.c marry.c mount.c limit.c mccp.c rdns_cache.c
    check_proxy.c mob_ai.c load.c spell_generic.c dbdump.c eliza.c
    config.c mapmaker.c olc_races.c recipe.c fight2.c olc_skills.c
    olc_classes.c olc_groups.c query.c)

add_executable(rom ${SRCS})
target_link_libraries(rom m ${DL_LIB} ${ZLIB_LIBRARIES} ${mysqlclient_LIBRARIES})
