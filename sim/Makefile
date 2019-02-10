# sun-2 emulator
#
# 10/2014  Brad Parker <brad@heeltoe.com>
# Copyright (C) 2014-2019 Brad Parker <brad@heeltoe.com>

OBJ = sim.o sim68k.o sun2.o scc.o am9513.o mm58167.o sc.o scsi.o utils.o \
	m68kcpu.o m68kops.o m68kopac.o m68kopdm.o m68kopnz.o m68kdasm.o 

M68K_SRC = ../m68k
DEFS = -DM68K_V33
#M68K_SRC = ../m68k-3.4

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
	EXTRA_INC = -I/Library/Frameworks/SDL.framework/Headers
	LIBS = -F /Library/Frameworks -framework SDL -framework Cocoa
else
	EXTRA_INC =
	LIBS = -lSDL -lpthread
endif

CFLAGS = -I. -I$(M68K_SRC) $(EXTRA_INC) -g -O2 $(DEFS)


all: sim 

ifeq ($(UNAME_S),Darwin)
SDLMain.o: SDLMain.m
	$(CC) -c $(CFLAGS) -o $@ $<

OBJ += SDLMain.o
endif

sim: $(OBJ) 
	$(CC) -o sim $(OBJ) $(LIBS)

sim.o: sim.c
	$(CC) -c $(CFLAGS) $<

sim68k.o: sim68k.c
	$(CC) -c $(CFLAGS) $<

sun2.o: sun2.c
	$(CC) -c $(CFLAGS) $<

m68kcpu.o: $(M68K_SRC)/m68kcpu.c $(M68K_SRC)/m68kops.h $(M68K_SRC)/m68kcpu.h
	$(CC) -c $(CFLAGS) -o $@ $<

m68kops.o: $(M68K_SRC)/m68kmake $(M68K_SRC)/m68kops.h $(M68K_SRC)/m68kops.c
	$(CC) -c $(CFLAGS) $(M68K_SRC)/m68kops.c -o m68kops.o

m68kopac.o: $(M68K_SRC)/m68kmake $(M68K_SRC)/m68kops.h $(M68K_SRC)/m68kopac.c
	$(CC) -c $(CFLAGS) $(M68K_SRC)/m68kopac.c -o m68kopac.o

m68kopdm.o: $(M68K_SRC)/m68kmake $(M68K_SRC)/m68kops.h $(M68K_SRC)/m68kopdm.c
	$(CC) -c $(CFLAGS) $(M68K_SRC)/m68kopdm.c -o m68kopdm.o

m68kopnz.o: $(M68K_SRC)/m68kmake $(M68K_SRC)/m68kops.h $(M68K_SRC)/m68kopnz.c
	$(CC) -c $(CFLAGS) $(M68K_SRC)/m68kopnz.c -o m68kopnz.o

m68kdasm.o: $(M68K_SRC)/m68kdasm.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f *.o
	rm -f sim

