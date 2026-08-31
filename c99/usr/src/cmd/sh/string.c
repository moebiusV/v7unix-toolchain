#
/*
 * UNIX shell
 *
 * S. R. Bourne
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include <stdint.h>


/* ========	general purpose string handling ======== */


STRING movstr(STRING a, STRING b)
{
	WHILE *b++ = *a++ DONE
	return(--b);
}

INT any(CHAR c, STRING s)
{
	REG CHAR d;

	WHILE d = *s++
	DO	IF d==c
		THEN	return(TRUE);
		FI
	OD
	return(FALSE);
}

INT cf(STRING s1, STRING s2)
{
	WHILE *s1++ == *s2
	DO	IF *s2++==0
		THEN	return(0);
		FI
	OD
	return(*--s1 - *s2);
}

INT length(STRING as)
{
	REG STRING s;

	IF s=as THEN WHILE *s++ DONE FI
	return(s-as);
}
