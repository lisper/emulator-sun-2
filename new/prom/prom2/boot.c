/* Copyright (c) 1982 by Jeffrey Mogul */

/*
 * boot.c
 *
 * device-independent bootstrap routines for SUN ROM monitor
 *
 * Jeffrey Mogul	23 February 1982
 */

#include "../h/sunmon.h"
#include "../h/globram.h"

struct DevTabEntry {
	char *devname;		/* device name */
	int (*loader)();	/* loader routine */
};

int noloader();
#ifdef	ENETBOOT
int enbootload();
#endif	ENETBOOT
#ifdef	TFTPBOOT
int tftpboot();
#endif	TFTPBOOT
#ifdef	DISKBOOT
int diskboot();
#endif	DISKBOOT

struct DevTabEntry DevTab[] = {
#ifdef	TFTPBOOT
	"\tBOOTP/TFTP ethernet\n",	tftpboot,
#endif	TFTPBOOT
#ifdef	ENETBOOT
	"\tPUP ethernet\n",		enbootload,
#endif	ENETBOOT
#ifdef	DISKBOOT
	"\tDisk\n",			diskboot,
#endif	DISKBOOT
#if	(ENETBOOT == 0) && (DISKBOOT == 0) && (TFTPBOOT == 0)
	/* insert this entry if no others exist */
	"\tno loader\n",	noloader
#endif
};	
/* Note:
 *	for now, table entries contain leading tab and trailing
 *	newline; this saves code space unless table gets large.
 */

#define	DTSIZE	(sizeof(DevTab)/sizeof(struct DevTabEntry))

/*
 * The bootstrap routines load their code at "loadat" which is
 * initialized here as "USERCODE" (0x1000).  However this value
 * can be patched with ddt (e.g. to 10000) to allow a RAM version
 * of the bootcode to be debugged at 1000.
 */
int	loadat	=	USERCODE;

boottype(code)
register int code;
{
	if ((code <= 0) || (code > DTSIZE)) {
		/* list possibilites */
		message("\tBootstrap Devices\nNumber\tDevice\n");
		for (code = 0; code < DTSIZE; code++) {
			printhex(code+1,1);	/* 1 digit for now */
			message(DevTab[code].devname);
		}
	}
	else {
		GlobPtr->BootLoadDef = (code - 1);
	}
	message("Current Loader is");
	message(DevTab[GlobPtr->BootLoadDef].devname);
}

bootload(fname)
char *fname;
{
	return(DevTab[GlobPtr->BootLoadDef].loader(fname));
}

noloader(fname)
char *fname;
{
	message(fname);
	message(" not loaded (no loader)\n");
	return(0);
}
