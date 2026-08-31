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
	while( *b++ = *a++ );
	return(--b);
}

INT any(CHAR c, STRING s)
{
	register CHAR d;

	while( d = *s++
	){	if( d==c
		){	return(1);
		;}
	;}
	return(0);
}

INT cf(STRING s1, STRING s2)
{
	while( *s1++ == *s2
	){	if( *s2++==0
		){	return(0);
		;}
	;}
	return(*--s1 - *s2);
}

INT length(STRING as)
{
	register STRING s;

	if( s=as ){ while( *s++ ); ;}
	return(s-as);
}
