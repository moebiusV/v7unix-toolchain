#
/*
 * UNIX shell
 *
 * S. R. Bourne
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>


STRING		trapcom[MAXTRAP];
BOOL		trapflg[MAXTRAP];

/* ========	fault handling routines	   ======== */


VOID fault(INT sig)
{
	register INT		flag;

	signal(sig,(void (*)(int))fault);
	if( sig==MEMF
	){	if( setbrk(brkincr) == (BYTPTR)-1
		){	error(nospace);
		;}
	} else if ( sig==ALARM
	){	if( flags&waiting
		){	done();
		;}
	} else {	flag = (trapcom[sig] ? TRAPSET : SIGSET);
		trapnote |= flag;
		trapflg[sig] |= flag;
	;}
}

void clrsig(INT i);
void getsig(int n);
int ignsig(int n);

void stdsigs(void)
{
	ignsig(QUIT);
	getsig(INTR);
	getsig(MEMF);
	getsig(ALARM);
}

int ignsig(int n)
{
	register intptr_t	s;
	register INT		i;

	/* v7: (s=signal(i=n,1)&01) — 1 was SIG_IGN */
	if( (s=(intptr_t)signal(i=n,SIG_IGN)&01)==0
	){	trapflg[i] |= SIGMOD;
	;}
	return((int)s);
}

void getsig(int n)
{
	register INT		i;

	if( trapflg[i=n]&SIGMOD || ignsig(i)==0
	){	signal(i,(void (*)(int))fault);
	;}
}

void oldsigs(void)
{
	register INT		i;
	register STRING	t;

	i=MAXTRAP;
	while( i--
	){  t=trapcom[i];
	    if( t==0 || *t
	    ){ clrsig(i);
	    ;}
	    trapflg[i]=0;
	;}
	trapnote=0;
}

void clrsig(INT i)
{
	free_sh(trapcom[i]); trapcom[i]=0;
	if( trapflg[i]&SIGMOD
	){	signal(i,(void (*)(int))fault);
		trapflg[i] &= ~SIGMOD;
	;}
}

void chktrap(void)
{
	/* check for traps */
	register INT		i=MAXTRAP;
	register STRING	t;

	trapnote &= ~TRAPSET;
	while( --i
	){ if( trapflg[i]&TRAPSET
	   ){ trapflg[i] &= ~TRAPSET;
		if( t=trapcom[i]
		){	INT	savxit=exitval;
			execexp(t,0);
			exitval=savxit; exitset();
		;}
	   ;}
	;}
}
