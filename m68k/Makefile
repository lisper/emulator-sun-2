
all: m68kops.h m68kmake

m68kops.h: m68kmake
	./m68kmake ../m68k m68k_in.c

m68kmake: m68kmake.c m68k_in.c m68kcpu.h
	$(CC) -o $@ $<



clean:
	rm -f *.o
	rm -f m68kmake
	rm -f m68kopac.c m68kopdm.c m68kopnz.c m68kops.c m68kops.h
