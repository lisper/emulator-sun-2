
OBJ = sim.o sim68k.o sun2.o scc.o am9513.o sc.o scsi.o utils.o \
	m68kcpu.o m68kops.o m68kopac.o m68kopdm.o m68kopnz.o m68kdasm.o 

CFLAGS = -I. -I../m68k -g -O2
LIBS = -lSDL -lpthread

all: sim rc

sim: $(OBJ) 
	$(CC) -o sim $(OBJ) $(LIBS)

sim.o: sim.c
	$(CC) -c $(CFLAGS) $<

sim68k.o: sim68k.c
	$(CC) -c $(CFLAGS) $<

sun2.o: sun2.c
	$(CC) -c $(CFLAGS) $<

m68kcpu.o: ../m68k/m68kcpu.c ../m68k/m68kops.h ../m68k/m68kcpu.h
	$(CC) -c $(CFLAGS) -o $@ $<

m68kops.o: ../m68k/m68kmake ../m68k/m68kops.h ../m68k/m68kops.c
	$(CC) -c $(CFLAGS) ../m68k/m68kops.c -o m68kops.o

m68kopac.o: ../m68k/m68kmake ../m68k/m68kops.h ../m68k/m68kopac.c
	$(CC) -c $(CFLAGS) ../m68k/m68kopac.c -o m68kopac.o

m68kopdm.o: ../m68k/m68kmake ../m68k/m68kops.h ../m68k/m68kopdm.c
	$(CC) -c $(CFLAGS) ../m68k/m68kopdm.c -o m68kopdm.o

m68kopnz.o: ../m68k/m68kmake ../m68k/m68kops.h ../m68k/m68kopnz.c
	$(CC) -c $(CFLAGS) ../m68k/m68kopnz.c -o m68kopnz.o

m68kdasm.o: ../m68k/m68kdasm.c
	$(CC) -c $(CFLAGS) -o $@ $<

rc: rc.c
	cc -o rc -O2 rc.c

clean:
	rm -f *.o
	rm -f rc

