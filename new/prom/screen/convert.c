/* Copyright (C) 1982 by Jeffrey Mogul */

#include <stdio.h>
#include "../h/screen.h"

extern short BaseFont[128][16];

main(argc,argv)
int argc;
char **argv;
{
	int i,j,h,w;
	long now = time(0);
	
	h = CHARHEIGHT;
	w = CHARWIDTH;
	
	printf("/* font.c - created %s",ctime(&now));
	printf(" */\n");
	printf("/* height = %d, width = %d */\n",h,w);
	printf("/* minchar = 0%o, maxchar = 0%o */\n",MINCHAR,MAXCHAR);

#if	CHARWIDTH > 8
	printf("short MapChar[%d][%d] = {\n",MAXCHAR+1-MINCHAR,CHARHEIGHT);
#endif

#if	CHARWIDTH <= 8
	printf("char MapChar[%d][%d] = {\n",MAXCHAR+1-MINCHAR,CHARHEIGHT);
#endif

	for (i = MINCHAR; i <= MAXCHAR; i++) {
	    printf("/* 0%o */ {",i);
	    for (j = 0; j < h; j++) {
	    	if (j) printf(", ");
	    	printf("0%o",(unsigned char)(BaseFont[i][j]>>8));
	    }
	    printf("},\n");
	}
	printf("};\n");
}

