/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * boottable.c
 *
 * Table defining correspondance between boot-code (from
 * configuration register) and (filename, bootdevice) pairs.
 *
 * Jeffrey Mogul	6 March 1982
 *
 * Philip Almquist  January 7, 1986
 *	- Change from Jim Powell:  remove "net/" from bootfile names so
 *	  that its possible to boot them from non-Unix systems.
 */

#include "../h/autoboot.h"

#ifndef AUTOBOOTNAME

/*
 * Since boot codes BOOT_TO_MON (0) and BOOT_SELF_TEST (15)
 * do not invoke autoboot(), this table has 14 instead of 16
 * entries, and is based at 1 instead of 0.
 */
struct BootTableEntry BootTable[14] = {
#ifdef	LFL
	/* 1  */	"dk(0,2)unix",	1,
	/* 2  */	"db(0,2)unix",	1,
	/* 3  */	"",		0,
	/* 4  */	"",		0,
	/* 5  */	"",		0,
	/* 6  */	"",		0,
	/* 7  */	"",		0,
	/* 8  */	"",		0,
	/* 9  */	"",		0,
	/* 10 */	"",		0,
	/* 11 */	"",		0,
	/* 12 */	"",		0,
	/* 13 */	"",		0,
	/* 14 */	"",		0
#else
	/* 1  */	"tip",		1,
	/* 2  */	"bridge",	1,
	/* 3  */	"gateway",	1,
	/* 4  */	"suntty",	1,
	/* 5  */	"Vload", 	1,
	/* 6  */	"dk(0,2)unix",	1,
	/* 7  */	"",		1,
	/* 8  */	"testtip",	1,
	/* 9  */	"sumextip", 	1,
	/* 10 */	"sunboot10",	1,
	/* 11 */	"sunboot11",	1,
	/* 12 */	"sunboot12",	1,
	/* 13 */	"sunboot13",	1,
	/* 14 */	"sunboot14",	1
#endif	LFL
};

#endif AUTOBOOTNAME
