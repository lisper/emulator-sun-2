/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * buserr.h
 *
 * definitions for structure showing details of Bus Error
 * or Address Error on MC68000
 *
 * Jeffrey Mogul  @ Stanford	15 June 1981
 */

struct BusErrInfo {
	unsigned int :11;		/* padding */
	unsigned int Read:1;		/* Read = 1, Write = 0; */
	unsigned int Instr:1;		/* FALSE if reference was a fetch */
	unsigned int FuncCode:3;	/* "Function Code */
	unsigned short AccAddrLo;	/* Access Address */
	unsigned short AccAddrHi;
	unsigned short InstrReg;	/* Instruction Register */
};

/*
 * values in FuncCode field
 */
#define	BERR_USERDATA	1
#define	BERR_USERPROG	2
#define	BERR_SUPERDATA	5
#define BERR_SUPERPROG	6
#define BERR_INTACK	7
