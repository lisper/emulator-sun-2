/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * givehelp.c
 *
 * Sun ROM Monitor - help function
 *
 * Jeffrey Mogul @ Stanford	18 August 1981
 */


/*
 * give a little help
 */

givehelp()
{
#ifdef	BOOTHELP
	int (*calladx)();	/* must NOT be in register */

	*((long*)&calladx) = bootload("sunmonhelp");
	if (calladx)
	    calladx();
	else
	    localhelp();
#else
	localhelp();
#endif	BOOTHELP
}

localhelp()
{
	message("Commands:\n");
	message("An,Dn,Mn,Pn - open A, D, SegMap, or Page reg #n\n");
	message("R - open Regs SSP, USP, SR, PC\n");
	message("E a - examine word @a\nO a - examine byte @a\n");
	message("G [a] - go\nC - continue\nB - set breakpoint\n");
	message("I T - transparent mode\nX <char> - set transp. escape\n");
	message("L <cmd> - download with <cmd>\nW [#] - set boot device\n");
	message("F <file> - load file (N <file> - load & go)\n");
	message("K - reset monitor stack\nK1 - total reset\n");
}
