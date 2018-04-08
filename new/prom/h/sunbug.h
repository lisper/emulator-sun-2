/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * sunbug.h
 *
 * Definitions for SunBug debugger
 *
 * Jeffrey Mogul @ Stanford	30 March 1981
 */

/*
 * Several Well-known Pup sockets should be defined in pupconstants.h;
 * these are SUNBUGSERVER, SUNBUGUSER, and SUNBOOTUSER.
 */
#define SUNBUGSERVER 0500
#define SUNBUGUSER   0501
#define SUNBOOTUSER  0502

/*
 * SunBug Pup types:
 *	even types go from Server to User
 *	odd types go from User to Server
 */

#define	SUNBUG_RETURN	0200	/* return from trap */
#define	SUNBUG_TRAP	0201	/* I trapped */
#define	SUNBUG_READMEM	0202	/* read memory */
#define	SUNBUG_RMACK	0203	/* acks read memory */
#define	SUNBUG_WRITEMEM	0204	/* write memory */
#define	SUNBUG_WMACK	0205	/* acks write memory */
#define	SUNBUG_READREG	0206	/* read register */
#define	SUNBUG_RRACK	0207	/* acks read register */
#define	SUNBUG_WRITEREG	0210	/* write register */
#define	SUNBUG_WRACK	0211	/* acks write register */
#define	SUNBUG_RESET	0212	/* reset Sun */
#define	SUNBUG_ERROR	0213	/* error */
#define	SUNBUG_BOOTYOURSELF	0214	/* Force Bootload */
#define	SUNBUG_BOOTDONE	0215	/* Bootload is done */

/*
 * Error codes for SUNBUG_ERROR
 */
#define	SBERR_BADCMD	(-1)	/* bad command */
#define	SBERR_RESET	(-2)	/* RESET done */
