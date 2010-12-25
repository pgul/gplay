CC = gcc
LINK = gcc
CFLAGS = -funsigned-char -Wall -Wno-char-subscripts -DUNIX -I../gulib
CFLAGS += -DHAVE_RVOLUME -DHAVE_ICONV
CFLAGS += -DMACOS
CFLAGS += -g
O = o
LFLAGS = $(CFLAGS)
LIBS = -lpthread
LIBS += -lncurses
#LIBS += -lslang
LIBS += -liconv

OBJ = obj/gplay.$(O) obj/db.$(O) obj/config.$(O) obj/debug.${O}
OBJ += obj/getopt.${O}

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

All:	gplay dbmgr

obj/gplay.$(O): gplay.c gplay.h Makefile
	$(CC) -c $(CFLAGS) -o $@ $<
obj/db.$(O): db.c gplay.h Makefile
	$(CC) -c $(CFLAGS) -o $@ $<
obj/dbmgr.$(O): dbmgr.c gplay.h Makefile
	$(CC) -c $(CFLAGS) -o $@ $<
obj/config.$(O): config.c gplay.h Makefile
	$(CC) -c $(CFLAGS) -o $@ $<
obj/debug.$(O): debug.c gplay.h Makefile
	$(CC) -c $(CFLAGS) -o $@ $<
obj/getopt.$(O): getopt.c gplay.h Makefile
	$(CC) -c $(CFLAGS) -o $@ $<

gplay:	$(OBJ) ../gulib/gulib.a Makefile
	$(LINK) $(LFLAGS) -o $@ $(LIBS) $(OBJ) ../gulib/gulib.a
dbmgr:	obj/db.$(O) obj/dbmgr.$(O) obj/config.${O} obj/debug.${O}
	$(LINK) $(LFLAGS) -o $@ $^

clean:
	rm -f obj/*.o obj/*.o
