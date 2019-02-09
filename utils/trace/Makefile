
all: rc tdump

rc: rc.c
	cc -o rc -O2 rc.c

tdump: tdump.c ../sim/m68kdasm.o
	cc -o tdump -g -O2 -I. -I../sim tdump.c ../sim/m68kdasm.o

clean:
	rm -f *.o
	rm -f rc tdump
