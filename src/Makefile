# $Id: Makefile,v 1.39.2.29 2009/04/11 12:30:49 key Exp $

# game
#CC      = gcc -m32 -O3  -march=i386

#debug
CC      = gcc -O0 -g
# -m32 -O0 -g  -march=i386

# -std=gnu99
# -std=gnu99
PROF    =
NOCRYPT =
#C_FLAGS =  -pipe -Wall $(PROF) $(NOCRYPT) -DNO_MYSQL -DNOTHREAD -DMEMDEBUG -D_USE_GNU -D_GNU_SOURCE -fPIC
C_FLAGS =  -pipe -Wall $(PROF) $(NOCRYPT) -DNO_MYSQL -DLINUX -D_USE_GNU -D_GNU_SOURCE -fPIC
L_FLAGS =  $(PROF) -export-dynamic
LIBS	= -lcrypt -lz -ldl -lm -lpthread
#-static-libgcc
# -laspell -melf_i386


#if !defined(WITHOUT_MYSQL)
#LIBS+= -lmysqlclient
#else
#C_FLAGS+= -DNO_MYSQL
#endif

#if!defined(WITHOUT_THREADS)
#L_FLAGS+= -pthread
#C_FLAGS+= -DTHREADED_DNS
##else
#C_FLAGS+= -DNOTHREAD -DMEMDEBUG -O3
#endif

O_FILES = act_comm.o act_info.o act_move.o act_obj.o act_wiz.o\
		  alias.o ban.o comm.o const.o db.o db2.o effects.o fight.o flags.o\
		  house.o handler.o healer.o interp.o note.o lookup.o magic.o magic2.o\
		  music.o recycle.o save.o scan.o skills.o special.o tables.o time.o\
		  update.o mob_cmds.o mob_prog.o string.o\
		  olc.o olc_act.o olc_save.o bank.o drunk.o summoner.o quest.o\
		  olc_mpcode.o clan.o marry.o mount.o limit.o mccp.o rdns_cache.o\
		  check_proxy.o mob_ai.o load.o spell_generic.o dbdump.o eliza.o\
		  config.o mapmaker.o olc_races.o recipe.o fight2.o olc_skills.o\
		  olc_classes.o olc_groups.o query.o

rom: $(O_FILES)
	rm -f rom
	$(CC) $(L_FLAGS) -o rom $(O_FILES) $(LIBS)

devel_rom: $(O_FILES)
	rm -f rom
	$(CC) $(L_FLAGS) -o rom $(O_FILES) $(LIBS)

clean:
	rm -f $(O_FILES)

devel: devel_rom
	rm -f /home/mud/mud/devel/area/rom
	install -m 0755 rom /home/mud/mud/devel/area

install: rom
	rm -f /home/mud/mud/bin/rom.last
	cp -f /home/mud/mud/area/rom /home/mud/mud/bin/rom.last
	rm -f /home/mud/mud/area/rom
	install -m 0755 rom /home/mud/mud/area

ctags:
	ctags -dtw $(O_FILES:.o=.c) *.h

depend:
	makedepend -- ${C_FLAGS} -- $(O_FILES:.o=.c)

.c.o: merc.h
	$(CC) -c  $(C_FLAGS) $<


