
#
/*
 *
 *	UNIX debugger
 *
 */

#include "defs.h"
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>


MSG		BADFIL;

SYMTAB		symbol;
BOOL		localok;
INT		lastframe;
SYMSLAVE		*symvec;
POS		maxoff;
L_INT		maxstor;

/* symbol management */
L_INT		symbas;
L_INT		symcnt;
L_INT		symnum;
L_INT		localval;
char		symrqd = -1;
SYMTAB		symbuf[SYMSIZ];
SYMPTR		symnxt;
SYMPTR		symend;


INT		fsym;
STRING		errflg;


/* symbol table and file handling service routines */

int16_t longseek(int16_t f, L_INT a)
{
	return(lseek(f,a,0) != -1);
}

POS findsym(POS svalue, INT type);
int16_t nextsym(void);
int16_t symread(void);

int16_t valpr(int16_t v, int16_t idsp)
{
	POS		d;
	d = findsym(v,idsp);
	IF d < maxoff
	THEN	aprintf("%.8s", symbol.symc);
		IF d
		THEN	aprintf(OFFMODE, d);
		FI
	FI
}

int16_t localsym(L_INT cframe)
{
	INT symflg;
	WHILE nextsym() ANDF localok
		ANDF symbol.symc[0]!='~'
		ANDF (symflg=symbol.symf)!=037
	DO IF symflg>=2 ANDF symflg<=4
	   THEN localval=symbol.symv;
		return(TRUE);
	   ELIF symflg==1
	   THEN localval=leng(shorten(cframe)+symbol.symv);
		return(TRUE);
	   ELIF symflg==20 ANDF lastframe
	   THEN localval=leng(lastframe+2*symbol.symv-10);
		return(TRUE);
	   FI
	OD
	return(FALSE);
}
int16_t psymoff(L_INT v, int16_t type, char *s)
{
	POS		w;
	w = findsym(shorten(v),type);
	IF w >= maxoff
	THEN aprintf(LPRMODE,v);
	ELSE aprintf("%.8s", symbol.symc);
	     IF w THEN aprintf(OFFMODE,w); FI
	FI
	aprintf(s);
}

POS findsym(POS svalue, INT type)
{
	L_INT		diff, value, symval, offset;
	INT		symtyp;
	REG SYMSLAVE	*symptr;
	SYMSLAVE	*symsav;
	value=svalue; diff = 0377777L; symsav=0;
	IF type!=NSYM ANDF (symptr=symvec)
	THEN	WHILE diff ANDF (symtyp=symptr->typslave)!=ESYM
		DO  IF symtyp==type
		    THEN symval=leng(symptr->valslave);
			 IF value-symval<diff
			    ANDF value>=symval
			 THEN diff = value-symval;
			      symsav=symptr;
			 FI
		    FI
		    symptr++;
		OD
		IF symsav
		THEN	offset=leng(symsav-symvec);
			symcnt=symnum-offset;
			longseek(fsym, symbas+offset*SYMTABSIZ);
			read(fsym,&symbol,SYMTABSIZ);
		FI
	FI
	return(shorten(diff));
}

int16_t nextsym(void)
{
	IF (--symcnt)<0
	THEN	return(FALSE);
	ELSE	return(longseek(fsym, symbas+(symnum-symcnt)*SYMTABSIZ)!=0 ANDF
			read(fsym,&symbol,SYMTABSIZ)==SYMTABSIZ);
	FI
}



/* sequential search through file */
int16_t symset(void)
{
	symcnt = symnum;
	symnxt = symbuf;
	IF symrqd
	THEN	longseek(fsym, symbas);
		symread(); symrqd=FALSE;
	ELSE	longseek(fsym, symbas+sizeof symbuf);
	FI
}

SYMPTR symget(void)
{
	REG INT	rc;
	IF symnxt >= symend
	THEN	rc=symread(); symrqd=TRUE;
	ELSE	rc=TRUE;
	FI
	IF --symcnt>0 ANDF rc==0 THEN errflg=BADFIL; FI
	return( (symcnt>=0 && rc) ? symnxt++ : 0);
}

int16_t symread(void)
{
	INT		symlen;

	IF (symlen=read(fsym,symbuf,sizeof symbuf))>=SYMTABSIZ
	THEN	symnxt = symbuf;
		symend = &symbuf[symlen/SYMTABSIZ];
		return(TRUE);
	ELSE	return(FALSE);
	FI
}
