#
/*
 *	UNIX shell
 *
 *	S. R. Bourne
 *	Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include <unistd.h>
#include <stdint.h>

BYTPTR setbrk(int incr)
{
	register BYTPTR	a=sbrk(incr);
	brkend=a+incr;
	return(a);
}
