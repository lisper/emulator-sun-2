/* Copyright (C) 1982 by Jeffrey Mogul  */

/*
 * message.c
 *
 * common subroutines for Sun ROM monitor: print string on console
 *
 * original author: Luis Trabb Pardo
 * largely re-written by Jeffrey Mogul	April, 1981  Stanford University
 * restructured again by Jeffrey Mogul  February, 1982
 */

#include "../h/sunmon.h"
#include "../h/asmdef.h"
#include "../h/globram.h"

/*
 *Prints message mess on the controlling terminal.
 */
message(mess)
register char *mess;
{
	for(;*mess!='\0';putchar(*(mess++)));
}

