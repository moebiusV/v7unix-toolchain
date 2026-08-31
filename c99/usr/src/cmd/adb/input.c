
#
/*
 *
 *	UNIX debugger
 *
 */

#include "defs.h"
#include <unistd.h>
#include <stdint.h>

INT		mkfault;
CHAR		line[LINSIZ];
INT		infile;
CHAR		*lp;
CHAR		lastc = EOR;
INT		eof;

/* input routines */

int16_t eol(CHAR c)
{
	return(c==EOR ORF c==';');
}

int16_t readchar(void);

int16_t rdc(void)
{	REP	readchar();
	PER	lastc==SP ORF lastc==TB
	DONE
	return(lastc);
}

int16_t readchar(void)
{
	IF eof
	THEN	lastc=EOF;
	ELSE	IF lp==0
		THEN	lp=line;
			REP eof = read(infile,lp,1)==0;
			    IF mkfault THEN error(0); FI
			PER eof==0 ANDF *lp++!=EOR DONE
			*lp=0; lp=line;
		FI
		IF lastc = *lp THEN lp++; FI
	FI
	return(lastc);
}

int16_t nextchar(void)
{
	IF eol(rdc())
	THEN lp--; return(0);
	ELSE return(lastc);
	FI
}

int16_t quotchar(void)
{
	IF readchar()=='\\'
	THEN	return(readchar());
	ELIF lastc=='\''
	THEN	return(0);
	ELSE	return(lastc);
	FI
}

int16_t getformat(STRING deformat)
{
	REG STRING	fptr;
	REG BOOL	quote;
	fptr=deformat; quote=FALSE;
	WHILE (quote ? readchar()!=EOR : !eol(readchar()))
	DO  IF (*fptr++ = lastc)=='"'
	    THEN quote = ~quote;
	    FI
	OD
	lp--;
	IF fptr!=deformat THEN *fptr++ = '\0'; FI
}


