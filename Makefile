CC = gcc
LINK = gcc
CFLAGS = -funsigned-char -Wall -Wno-char-subscripts -Zomf -Zcrtdll -Zmt -DOS2 -O2 -s
O = obj
LFLAGS = $(CFLAGS)

OBJ = objemx/gplay.$(O) objemx/scrlmenu.$(O) objemx/db.$(O) objemx/config.$(O)

.c.obj:
	$(CC) -c $(CFLAGS) -o $@ $<

All:	gplay.exe dbmgr.exe

objemx/gplay.$(O): gplay.c gplay.h
	$(CC) -c $(CFLAGS) -o $@ $<
objemx/db.$(O): db.c gplay.h
	$(CC) -c $(CFLAGS) -o $@ $<
objemx/scrlmenu.$(O): scrlmenu.c
	$(CC) -c $(CFLAGS) -o $@ $<
objemx/dbmgr.$(O): dbmgr.c gplay.h
	$(CC) -c $(CFLAGS) -o $@ $<
objemx/config.$(O): config.c gplay.h
	$(CC) -c $(CFLAGS) -o $@ $<

gplay.exe:	$(OBJ)
	$(LINK) $(LFLAGS) -o $@ $(OBJ) -lglibpe
dbmgr.exe:	objemx/db.$(O) objemx/dbmgr.$(O) objemx/config.$(O)
	$(LINK) $(LFLAGS) -o $@ $^

clean:
	rm -f objemx/*.obj objemx/*.o gplaye.exe dbmgr.exe
