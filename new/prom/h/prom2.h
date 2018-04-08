/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * prom2.h
 *
 * Sun ROM monitor - dispatch codes for second prom
 *
 * Jeffrey Mogul @ Stanford	13 August 1981
 *
 */
#ifndef	ADRSPC_PROM1
#include <pcmap.h>
#endif	ADRSPC_PROM1

#define	PRM2_WRITECHAR	1	/* writechar(arg) [on screen] */
#define	PRM2_BOOTLOAD	2	/* bootload(name) */
#define	PRM2_VERSION	3	/* version() [of prom2 code] */
#define	PRM2_BOOTTYPE	4	/* set/list boot device type */

#define	PROM2_DISPATCH(t,a)	((int (*)())ADRSPC_PROM1)(t,a)

#define	prm2_writechar(x)	PROM2_DISPATCH(PRM2_WRITECHAR,x)
#define	prm2_bootload(x)	PROM2_DISPATCH(PRM2_BOOTLOAD,x)
#define	prm2_version()		PROM2_DISPATCH(PRM2_VERSION,0)
#define prm2_boottype(x)	PROM2_DISPATCH(PRM2_BOOTTYPE,x)
