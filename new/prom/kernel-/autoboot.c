/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * autoboot.c
 *
 * Sun ROM monitor: automatic bootstrap code
 *
 * Jeffrey Mogul  February 25, 1982
 *
 * John Seamons	  April 19, 1982
 *
 * Modified to pass along file name from boot table as argument to
 * booted program.  This allows more specialized boot programs loaded
 * by PROM to further parse the autoboot information.
 *
 */

#include "../h/sunmon.h"
#include "../h/globram.h"
#include "../h/confreg.h"
#include "../h/autoboot.h"

#ifdef	AUTOBOOTNAME

autoboot(dummy)
int dummy;
{
	int (*calladx)();	/* must not be in a register */

#define	QUOTE(X) "X"
	/* automatically boot-load the specified file */
	*((long*)&calladx) = bootload(QUOTE(AUTOBOOTNAME));
	if (calladx) 	/* file was found, start the program */
		calladx();
	else {	/* file not found - notify and drop into monitor */
		message(QUOTE(AUTOBOOTNAME));
		message(" could not be loaded!\n");
	}
}

listboots()
{
#define	BOOTMSG(X) "Auto-bootload file is X.\n"
	message(BOOTMSG(AUTOBOOTNAME));
}

#else

/*
 * Since boot codes BOOT_TO_MON (0) and BOOT_SELF_TEST (15)
 * do not invoke autoboot(), this table has 14 instead of 16
 * entries, and is based at 1 instead of 0.
 */
extern struct BootTableEntry BootTable[14];

autoboot(code)
int code;
{
	register GlobDes *gp = GlobPtr;
	register char *fname;
	register int bootdev;
	int (*calladx)();	/* must not be in a register */

	if ((--code > 13) || (code < 0))
	    return(0);	/* invalid code, should never happen */

	message("Booting ");
	message(fname = BootTable[code].filename);
	if ((bootdev = BootTable[code].devtype) == 0) {
	    message(" is invalid\n");
	    return(0);
	}
	else {
	    putchar('\n');
	    boottype(bootdev);
	}
	
	/* automatically boot-load the specified file */
	*((long*)&calladx) = bootload(fname);
	if (calladx) 	/* file was found, start the program */
		calladx(fname);		/* pass along file name */
	else {	/* file not found - notify and drop into monitor */
		message("Load failed\n");
	}
}

listboots()
{
	register int i;
	register int bootdev;
	
	message("  Auto-boot files:\ncode\tname\ton dev\n");
	for (i = 0; i < 14; i++) {
	    if (bootdev = BootTable[i].devtype) {
		printhex(i+1,1);
		putchar('\t');
		message(BootTable[i].filename);
		putchar('\t');
		printhex(bootdev,1);
		putchar('\n');
	    }
	}
}
#endif	AUTOBOOTNAME	
