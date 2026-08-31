
#
/*
 *
 *	UNIX debugger
 *
 */

#include "defs.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>


MSG		BADNAM;
MSG		BADMAG;

MAP		txtmap;
MAP		datmap;
SYMSLAVE	*symvec;
INT		wtflag;
INT		fcor;
INT		fsym;
L_INT		maxfile;
L_INT		maxstor;
L_INT		txtsiz;
L_INT		datsiz;
L_INT		datbas;
L_INT		stksiz;
STRING		errflg;
INT		magic;
L_INT		symbas;
L_INT		symnum;
L_INT		entrypt;

INT		argcount;
INT		signo;
POS		corhdr[512];
POS		*endhdr = &corhdr[512];

STRING		symfil = "a.out";
STRING		corfil = "core";

#define TXTHDRSIZ	(sizeof(txthdr))

int16_t getfile(STRING filnam, int16_t cnt);

int16_t setsym(void)
{
	INT		relflg;
	INT		symval, symflg;
	SYMSLAVE	*symptr;
	SYMPTR		symp;
	TXTHDR		txthdr;

	fsym=getfile(symfil,1);
	txtmap.ufd=fsym;
	IF read(fsym, txthdr, TXTHDRSIZ)==TXTHDRSIZ
	THEN	magic=txthdr[0];
		IF magic!=0411 ANDF magic!=0410 ANDF magic!=0407 ANDF magic!=0405
		THEN	magic=0;
		ELSE	symnum=txthdr[4]/SYMTABSIZ;
			txtsiz=txthdr[1];
			datsiz=txthdr[2];
			symbas=txtsiz+datsiz;
			txtmap.b1=0;
			txtmap.e1=(magic==0407?symbas:txtsiz);
			txtmap.f1 = TXTHDRSIZ;
			txtmap.b2=datbas=(magic==0410?round(txtsiz,TXTRNDSIZ):0);
			txtmap.e2=txtmap.b2+(magic==0407?symbas:datsiz);
			txtmap.f2 = TXTHDRSIZ+(magic==0407?0:txtmap.e1);
			entrypt=txthdr[5];
			relflg=txthdr[7];
			IF relflg!=1 THEN symbas <<= 1; FI
			symbas += TXTHDRSIZ;

			/* set up symvec */
			symvec=(SYMSLAVE *)sbrk(shorten((1+symnum))*sizeof (SYMSLAVE));
			IF (symptr=symvec)==-1
			THEN	aprintf("%s\n",BADNAM);
				symptr=symvec=(SYMSLAVE *)sbrk(sizeof (SYMSLAVE));
			ELSE	symset();
				WHILE (symp=symget()) ANDF errflg==0
				DO  symval=symp->symv; symflg=symp->symf;
				    symptr->valslave=symval;
				    symptr->typslave=SYMTYPE(symflg);
				    symptr++;
				OD
			FI
			symptr->typslave=ESYM;
		FI
	FI
	IF magic==0 THEN txtmap.e1=maxfile; FI
}

int16_t setcor(void)
{
	fcor=getfile(corfil,2);
	datmap.ufd=fcor;
	IF read(fcor, corhdr, ctob(USIZE))==ctob(USIZE)
	THEN	txtsiz = ((struct user *)corhdr)->u_tsize << 6;
		datsiz = ((struct user *)corhdr)->u_dsize << 6;
		stksiz = ((struct user *)corhdr)->u_ssize << 6;
		datmap.b1 = datbas = (magic==0410?round(txtsiz,TXTRNDSIZ):0);
		datmap.e1=(magic==0407?txtsiz:datmap.b1)+datsiz;
		datmap.f1 = ctob(USIZE);
		datmap.b2 = maxstor-stksiz;
		datmap.e2 = maxstor;
		datmap.f2 = ctob(USIZE)+(magic==0410?datsiz:datmap.e1);
		IF magic ANDF magic!=((struct user *)corhdr)->u_exdata.ux_mag
		THEN	aprintf("%s\n",BADMAG);
		FI
	ELSE	datmap.e1 = maxfile;
	FI
}

int16_t create(STRING f)
{	int16_t fd;
	IF (fd=creat(f,0644))>=0
	THEN close(fd); return(open(f,wtflag));
	ELSE return(-1);
	FI
}

int16_t getfile(STRING filnam, int16_t cnt)
{
	REG INT		fsym;

	IF !eqstr("-",filnam)
	THEN	fsym=open(filnam,wtflag);
		IF fsym<0 ANDF argcount>cnt
		THEN	IF wtflag
			THEN	fsym=create(filnam);
			FI
			IF fsym<0
			THEN aprintf("cannot open `%s'\n", filnam);
			FI
		FI
	ELSE	fsym = -1;
	FI
	return(fsym);
}
