/* Copyright (C) 1982 by Jeffrey Mogul  */

/*
 * queryval.c
 *
 * common subroutines for Sun ROM monitor: read number with prompt
 *
 * original author: Luis Trabb Pardo
 * largely re-written by Jeffrey Mogul	April, 1981  Stanford University
 * restructured again by Jeffrey Mogul  February, 1982
 */

#include "../h/sunmon.h"
#include "../h/asmdef.h"
#include "../h/globram.h"

/*
 * prints a message, prints the l hex digits of v, prints a
 * ?, and reads a new line into the buffer.
 */
queryval (m,v,l)
char *m;
longword v;
int l;
{
	message(m);
	printhex(v,l);
	message("? ");
	getline();
}
