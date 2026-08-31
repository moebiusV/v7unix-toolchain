#
/*
 * UNIX shell
 *
 * S. R. Bourne
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include <stdlib.h>
#include <stdint.h>

STKPTR		stakbot=nullstr;



/* ========	storage allocation	======== */

void *getstak(INT asize)
{	/* allocate requested stack */
	register STKPTR	oldstak;
	register INT		size;

	size=(INT)roundup(asize,BYTESPERWORD);
	oldstak=stakbot;
	staktop = stakbot += size;
	return(oldstak);
}

STKPTR locstak(void)
{	/* set up stack for local use
	 * should be followed by `endstak'
	 */
	if( brkend-stakbot<BRKINCR
	){	setbrk(brkincr);
		if( brkincr < BRKMAX
		){	brkincr += 256;
		;}
	;}
	return(stakbot);
}

STKPTR savstak(void)
{
	assert(staktop==stakbot);
	return(stakbot);
}

void *endstak(STRING argp)
{	/* tidy up after `locstak' */
	register STKPTR	oldstak;
	*argp++=0;
	oldstak=stakbot; stakbot=staktop=(STKPTR)roundup((uintptr_t)argp,BYTESPERWORD);
	return(oldstak);
}

VOID tdystak(STKPTR x)
{
	/* try to bring stack back to x */
	while( ADR(stakbsy)>ADR(x)
	){ free_sh(stakbsy);
	   stakbsy = stakbsy->word;
	;}
	staktop=stakbot=max(ADR(x),ADR(stakbas));
	rmtemp(x);
}

void stakchk(void)
{
	if( (brkend-stakbas)>BRKINCR+BRKINCR
	){	setbrk(-BRKINCR);
	;}
}

STKPTR cpystak(STKPTR x)
{
	return(endstak(movstr(x,locstak())));
}
