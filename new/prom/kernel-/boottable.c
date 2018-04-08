/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * boottable.c
 *
 * Table defining correspondance between boot-code (from
 * configuration register) and (filename, bootdevice) pairs.
 *
 * Jeffrey Mogul	6 March 1982
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
	/* 1  */	"tip",		2,
	/* 2  */	"bridge",	2,
	/* 3  */	"net/gateway",	2,
	/* 4  */	"suntty",	2,
	/* 5  */	"Vload", 	2,
	/* 6  */	"dk(0,2)unix",	2,
	/* 7  */	"",		1,	/* bootp/tftp */
	/* 8  */	"test/tip",	2,
	/* 9  */	"sumextip", 	2,
	/* 10 */	"sunboot10",	2,
	/* 11 */	"sunboot11",	2,
	/* 12 */	"sunboot12",	2,
	/* 13 */	"sunboot13",	2,
	/* 14 */	"sunboot14",	2
#endif	LFL
};

#endif AUTOBOOTNAME
