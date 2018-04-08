OBJS = y.tab.o main.o lex.yy.o mkmakefile.o mkheaders.o
CFLAGS=-w -O
LDFLAGS=-ll
DESTDIR=../conf

sunconfig: $(OBJS)
	$(CC) $(OBJS) -o sunconfig $(LDFLAGS)

install: sunconfig
	cp sunconfig $(DESTDIR)
	cp sunconfig $(DESTDIR)2

y.tab.o: y.tab.c

y.tab.c: config.y
	yacc -d config.y

lex.yy.c: config.l
	lex config.l

clean:
	rm -f $(OBJS) lex.yy.c y.tab.c

print:
	yapp main.c config.y config.l mk*.c
