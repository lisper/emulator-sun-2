# Makefile for SUN confreg program
# Jeffrey Mogul	17 February, 1982

all: install

confreg: confreg.c ../h/confreg.h ../h/sunmon.h
	cc68 -o confreg -r confreg.c

install: confreg
	chmod g+x confreg
	mv confreg /usr/sun/bootfile
