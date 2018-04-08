/* Copyright (C) 1982 by Jeffrey Mogul */

#include <stdio.h>
#include "screen.h"

char bp[1024];
char cm[100];
initscreen()
{
	char area[100];
	char *a;
	
	tgetent(bp,getenv("TERM"));
	
	a = area;
	
	tgetstr("cl",&a);
	a = cm;
	tgetstr("cm",&a);
	
	a = area;
	while (*a) putchar(*a++);
	
	Xcurr = 0;
	Ycurr = 0;
	
}

putat(x,y,c)
int x,y;
char c;
{
	char *cp;
	char *tgoto();
	
	cp = tgoto(cm,x,y);

	while (*cp) write(1,cp++,1);

	write(1,&c,1);
}
