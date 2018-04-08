/* Copyright (C) 1982 by Jeffrey Mogul */

putchar(c)
{/* if (c != 033) emt_putchar(c);*/}
main()
{
	char c;
/*	initscreen(); */

	while(1) {
		c = getchar();
		fwritechar(c);
	}
}
