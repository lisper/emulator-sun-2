/* Copyright (C) 1982 by Jeffrey Mogul  */

/*
 * ishex.c
 *
 * common subroutines for Sun ROM monitor: find hex value of character
 *
 * original author: Luis Trabb Pardo
 * largely re-written by Jeffrey Mogul	April, 1981  Stanford University
 * restructured again by Jeffrey Mogul  February, 1982
 * Completely rewritten (to reduce code space) by Ross Finlayson, April 1984
 */

/*
 * Returns the hex value of a char or -1 if the char is not hex
 */
int ishex(ch)
register char ch;
{
    if (ch >= '0' && ch <= '9')
        return (ch - '0');
    ch |= 040; /* Convert to lower case. */
    if (ch >= 'a' && ch <= 'f')
        return (ch - 'a' + 10);
    return (-1);
}
