/* Copyright (C) 1982 by Jeffrey Mogul */

#include <stdio.h>

main()
{
	char c;
	
	setbuf(stdin,NULL);
	setbuf(stdout,NULL);

	system("stty -echo raw");
	initscreen();
	
	while ((read(0,&c,1) == 1) && (c != 4)) fwritechar(c);
	system("stty echo -raw");
}

	
