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
#include <unistd.h>
#include <stdint.h>


/* ========	error handling	======== */

void exitset(void)
{
	assnum(&exitadr,exitval);
}

void done(void);
void exitsh(INT xno);
void rmtemp(IOPTR base);

void sigchk(void)
{
	/* Find out if it is time to go away.
	 * `trapnote' is set to SIGSET when fault is seen and
	 * no trap has been set.
	 */
	if( trapnote&SIGSET
	){	exitsh(SIGFAIL);
	;}
}

void failed(STRING s1, STRING s2)
{
	prp(); prs(s1); 
	if( s2
	){	prs(colon); prs(s2);
	;}
	newline(); exitsh(ERROR);
}

void error(STRING s)
{
	failed(s,NIL);
}

void exitsh(INT xno)
{
	/* Arrive here from `FATAL' errors
	 *  a) exit command,
	 *  b) default trap,
	 *  c) fault with no trap set.
	 *
	 * Action is to return to command level or exit.
	 */
	exitval=xno;
	if( (flags & (forked|errflg|ttyflg)) != ttyflg
	){	done();
	} else {	clearup();
		longjmp(errshell,1);
	;}
}

void done(void)
{
	register STRING	t;
	if( t=trapcom[0]
	){	trapcom[0]=0; /*should free but not long */
		execexp(t,0);
	;}
	rmtemp(0);
	exit(exitval);
}

void rmtemp(IOPTR base)
{
	while( iotemp>base
	){  unlink(iotemp->ioname);
	    iotemp=iotemp->iolst;
	;}
}
