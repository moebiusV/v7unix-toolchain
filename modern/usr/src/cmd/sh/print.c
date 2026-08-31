#
/*
 * UNIX shell
 *
 * S. R. Bourne
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include <unistd.h>
#include <stdint.h>

CHAR		numbuf[6];


/* printing and io conversion */

void itos(int n);
VOID prc(CHAR c);
void prn(INT n);
VOID prs(STRING as);

void newline(void)
{	prc(NL);
}

void blank(void)
{	prc(SP);
}

void prp(void)
{
	if( (flags&prompt)==0 && cmdadr
	){	prs(cmdadr); prs(colon);
	;}
}

VOID prs(STRING as)
{
	register STRING	s;

	if( s=as
	){	write(output,s,length(s)-1);
	;}
}

VOID prc(CHAR c)
{
	if( c
	){	write(output,&c,1);
	;}
}

void prt(L_INT t)
{
	register INT	hr, min, sec;

	t += 30; t /= 60;
	sec=t%60; t /= 60;
	min=t%60;
	if( hr=t/60
	){	prn(hr); prc('h');
	;}
	prn(min); prc('m');
	prn(sec); prc('s');
}

void prn(INT n)
{
	itos(n); prs(numbuf);
}

void itos(int n)
{
	register char *abuf; register POS a, i; INT pr, d;
	abuf=numbuf; pr=0; a=n;
	for( i=10000; i!=1; i/=10
	){	if( (pr |= (d=a/i)) ){ *abuf++=d+'0' ;}
		a %= i;
	;}
	*abuf++=a+'0';
	*abuf++=0;
}

int stoi(STRING icp)
{
	register CHAR	*cp = icp;
	register INT		r = 0;
	register CHAR	c;

	while( (c = *cp, digit(c)) && c && r>=0
	){ r = r*10 + c - '0'; cp++ ;}
	if( r<0 || cp==icp
	){	failed(icp,badnum);
	} else {	return(r);
	;}
}

