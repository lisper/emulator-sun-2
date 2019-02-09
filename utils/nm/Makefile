
all: nm nm2

nm: nm.c a.out.h exec.h
	cc -g -o nm nm.c

nm2: nm2.c
	cc -g -o nm2 -I. -I../../sim nm2.c ../../sim/m68kdasm.o

clean:
	rm -f nm nm2
