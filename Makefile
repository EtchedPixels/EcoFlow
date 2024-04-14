all: ecoset ecoget ecodump

ecodump.o: ecodump.c ecolib.h
ecoget.o: ecoget.c ecolib.h
ecoset.o: ecoset.c ecolib.h
ecolib.o: ecolib.c ecolib.h

CFLAGS += -Wall -pedantic

ecodump: ecodump.o ecolib.o
	gcc ecodump.o ecolib.o -o ecodump

ecoget: ecoget.o ecolib.o
	gcc ecoget.o ecolib.o -o ecoget

ecoset: ecoset.o ecolib.o
	gcc ecoset.o ecolib.o -o ecoset

clean:
	rm -f *.o *~ ecoset ecoget ecodump
