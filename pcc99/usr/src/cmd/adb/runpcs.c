
#
/*
 *
 *	UNIX debugger
 *
 */

#include "defs.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>


MSG		NOFORK;
MSG		ENDPCS;
MSG		BADWAIT;

CHAR		*lp;
INT		sigint;
INT		sigqit;

/* breakpoints */
BKPTR		bkpthead;

REGLIST		reglist[];

CHAR		lastc;
POS		corhdr[];
POS		*endhdr;

INT		fcor;
INT		fsym;
STRING		errflg;
INT		errno;
INT		signo;

L_INT		dot;
STRING		symfil;
INT		wtflag;
INT		pid;
L_INT		expv;
INT		adrflg;
L_INT		loopcnt;





/* service routines for sub process control */

int16_t getsig(int16_t sig)
{	return(expr(0) ? shorten(expv) : sig);
}

INT		userpc=1;

int16_t bpwait(void);
int16_t doexec(void);
int16_t execbkpt(BKPTR bkptr);
int16_t readregs(void);
BKPTR scanbkpt(int16_t adr);
int16_t setbp(void);

int16_t runpcs(int16_t runmode, int16_t execsig)
{
	INT		rc;
	REG BKPTR	bkpt;
	IF adrflg
	THEN userpc=shorten(dot);
	FI
	setbp();
	aprintf("%s: running\n", symfil);

	WHILE (loopcnt--)>0
	DO
#ifdef DEBUG
		aprintf("\ncontinue %d %d\n",userpc,execsig);
#endif
		stty(0,&usrtty);
		ptrace(runmode,pid,userpc,execsig);
		bpwait(); chkerr(); readregs();

		/*look for bkpt*/
		IF signo==0 ANDF (bkpt=scanbkpt(endhdr[pc]-2))
		THEN /*stopped at bkpt*/
		     userpc=endhdr[pc]=bkpt->loc;
		     IF bkpt->flag==BKPTEXEC
			ORF ((bkpt->flag=BKPTEXEC, command(bkpt->comm,':')) ANDF --bkpt->count)
		     THEN execbkpt(bkpt); execsig=0; loopcnt++;
			  userpc=1;
		     ELSE bkpt->count=bkpt->initcnt;
			  rc=1;
		     FI
		ELSE rc=0; execsig=signo; userpc=1;
		FI
	OD
	return(rc);
}

int16_t endpcs(void)
{
	REG BKPTR	bkptr;
	IF pid
	THEN ptrace(EXIT,pid,0,0); pid=0; userpc=1;
	     FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	     DO IF bkptr->flag
		THEN bkptr->flag=BKPTSET;
		FI
	     OD
	FI
}

int16_t setup(void)
{
	close(fsym); fsym = -1;
	IF (pid = fork()) == 0
	THEN ptrace(SETTRC,0,0,0);
	     signal(SIGINT,(void (*)(int))sigint); signal(SIGQUIT,(void (*)(int))sigqit);
	     doexec(); exit(0);
	ELIF pid == -1
	THEN error(NOFORK);
	ELSE bpwait(); readregs(); lp[0]=EOR; lp[1]=0;
	     fsym=open(symfil,wtflag);
	     IF errflg
	     THEN aprintf("%s: cannot execute\n",symfil);
		  endpcs(); error(0);
	     FI
	FI
}

int16_t execbkpt(BKPTR bkptr)
{	INT		bkptloc;
#ifdef DEBUG
	aprintf("exbkpt: %d\n",bkptr->count);
#endif
	bkptloc = bkptr->loc;
	ptrace(WIUSER,pid,bkptloc,bkptr->ins);
	stty(0,&usrtty);
	ptrace(SINGLE,pid,bkptloc,0);
	bpwait(); chkerr();
	ptrace(WIUSER,pid,bkptloc,BPT);
	bkptr->flag=BKPTSET;
}


int16_t doexec(void)
{
	STRING		argl[MAXARG];
	CHAR		args[LINSIZ];
	STRING		p, *ap, filnam;
	ap=argl; p=args;
	*ap++=symfil;
	REP	IF rdc()==EOR THEN break; FI
		*ap = p;
		WHILE lastc!=EOR ANDF lastc!=SP ANDF lastc!=TB DO *p++=lastc; readchar(); OD
		*p++=0; filnam = *ap+1;
		IF **ap=='<'
		THEN	close(0);
			IF open(filnam,0)<0
			THEN	aprintf("%s: cannot open\n",filnam); exit(0);
			FI
		ELIF **ap=='>'
		THEN	close(1);
			IF creat(filnam,0666)<0
			THEN	aprintf("%s: cannot create\n",filnam); exit(0);
			FI
		ELSE	ap++;
		FI
	PER lastc!=EOR DONE
	*ap++=0;
	execv(symfil, argl);
}

BKPTR scanbkpt(int16_t adr)
{
	REG BKPTR	bkptr;
	FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	DO IF bkptr->flag ANDF bkptr->loc==adr
	   THEN break;
	   FI
	OD
	return(bkptr);
}

int16_t delbp(void)
{
	REG INT		a;
	REG BKPTR	bkptr;
	FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	DO IF bkptr->flag
	   THEN a=bkptr->loc;
		ptrace(WIUSER,pid,a,bkptr->ins);
	   FI
	OD
}

int16_t setbp(void)
{
	REG INT		a;
	REG BKPTR	bkptr;

	FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	DO IF bkptr->flag
	   THEN a = bkptr->loc;
		bkptr->ins = ptrace(RIUSER, pid, a, 0);
		ptrace(WIUSER, pid, a, BPT);
		IF errno
		THEN prints("cannot set breakpoint: ");
		     psymoff(leng(bkptr->loc),ISYM,"\n");
		FI
	   FI
	OD
}

int16_t bpwait(void)
{
	REG INT w;
	INT stat;

	signal(SIGINT, 1);
	WHILE (w = wait(&stat))!=pid ANDF w != -1 DONE
	signal(SIGINT,(void (*)(int))sigint);
	gtty(0,&usrtty);
	stty(0,&adbtty);
	IF w == -1
	THEN pid=0;
	     errflg=BADWAIT;
	ELIF (stat & 0177) != 0177
	THEN IF signo = stat&0177
	     THEN sigprint();
	     FI
	     IF stat&0200
	     THEN prints(" - core dumped");
		  close(fcor);
		  setcor();
	     FI
	     pid=0;
	     errflg=ENDPCS;
	ELSE signo = stat>>8;
	     IF signo!=SIGTRC
	     THEN sigprint();
	     ELSE signo=0;
	     FI
	     flushbuf();
	FI
}

int16_t readregs(void)
{
	/*get REG values from pcs*/
	REG int16_t i;
	FOR i=0; i<9; i++
	DO endhdr[reglist[i].roffs] =
		    ptrace(RUREGS, pid, 2*(512+reglist[i].roffs), 0);
	OD

	/* REALing poINT		*/
	FOR i=FROFF; i<FRLEN+FROFF; i++
	DO corhdr[i] = ptrace(RUREGS,pid,i,0); OD
}


