CFLAGS = -O
all: sun

vax: main.o vaxscreen.o screen.o font.o
	cc -o vax main.o screen.o vaxscreen.o font.o -ltermcap

sun: screen.b sunscreen.b font.b main.b
	cc68 -r -o sun screen.b sunscreen.b font.b main.b

screen.b: screen.c ../h/screen.h
	cc68 -c -O -DSTANDALONE -o screen.b screen.c

sunscreen.b: sunscreen.c ../h/screen.h
	cc68 -c -O -o sunscreen.b sunscreen.c

font.b: font.c cmrx.c ../h/screen.h 
	cc68 -c -O -o font.b font.c

main.b: sunmain.c
	cc68 -c -O -o main.b sunmain.c

clearscreen.b: clearscreen.c
	cc68 -c -O -o clearscreen.b clearscreen.c

cmrx.c: convert
	convert >cmrx.c

font.c: convert
	convert >font.c

convert: convert.o basefont.o
	cc -o convert convert.o basefont.o

convert.o: convert.c ../h/screen.h
	cc -c convert.c

basefont.o: basefont.c cmr.c
	cc -c basefont.c
