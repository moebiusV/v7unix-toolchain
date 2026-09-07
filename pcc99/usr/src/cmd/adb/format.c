
#
/*
 *
 *	UNIX debugger
 *
 */

#include "defs.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>


MSG		BADMOD;
MSG		NOFORK;
MSG		ADWRAP;

SYMTAB		symbol;

INT		mkfault;
CHAR		*lp;
INT		maxoff;
INT		sigint;
INT		sigqit;
STRING		errflg;
CHAR		lastc;
L_INT		dot;
INT		dotinc;
L_INT		var[];


STRING exform(INT fcount, STRING ifp, int16_t itype, int16_t ptype);
L_INT inkdot(int16_t incr);
int16_t printesc(int16_t c);

int16_t scanform(L_INT icount, STRING ifp, int16_t itype, int16_t ptype)
{
	STRING		fp;
	CHAR		modifier;
	INT		fcount, init=1;
	L_INT		savdot;

	WHILE icount
	DO  fp=ifp;
	    IF init==0 ANDF findsym(shorten(dot),ptype)==0 ANDF maxoff
	    THEN aprintf("\n%.8s:%16t",symbol.symc);
	    FI
	    savdot=dot; init=0;

	    /*now loop over format*/
	    WHILE *fp ANDF errflg==0
	    DO  IF digit(modifier = *fp)
		THEN fcount=0;
		     WHILE digit(modifier = *fp++)
		     DO fcount *= 10;
			fcount += modifier-'0';
		     OD
		     fp--;
		ELSE fcount=1;
		FI

		IF *fp==0 THEN break; FI
		fp=exform(fcount,fp,itype,ptype);
	    OD
	    dotinc=dot-savdot;
	    dot=savdot;

	    IF errflg
	    THEN IF icount<0
		 THEN errflg=0; break;
		 ELSE error(errflg);
		 FI
	    FI
	    IF --icount
	    THEN dot=inkdot(dotinc);
	    FI
	    IF mkfault THEN error(0); FI
	OD
}

STRING exform(INT fcount, STRING ifp, int16_t itype, int16_t ptype)
{
	/* execute single format item `fcount' times
	 * sets `dotinc' and moves `dot'
	 * returns address of next format item
	 */
	POS		w;
	L_INT		savdot, wx;
	STRING		fp;
	CHAR		c, modifier, longpr;
	union {
		L_REAL	f;
		struct {
			L_INT	sa;
			INT	sb, sc;
		} s;
	} fw;

	WHILE fcount>0
	DO	fp = ifp; c = *fp;
		longpr=(c>='A')&(c<='Z')|(c=='f');
		IF itype==NSP ORF *fp=='a'
		THEN wx=dot; w=dot;
		ELSE w=get(dot,itype);
		     IF longpr
		     THEN wx=itol(w,get(inkdot(2),itype));
		     ELSE wx=w;
		     FI
		FI
		IF c=='F'
		THEN fw.s.sb=get(inkdot(4),itype);
		     fw.s.sc=get(inkdot(6),itype);
		FI
		IF errflg THEN return(fp); FI
		IF mkfault THEN error(0); FI
		var[0]=wx;
		modifier = *fp++;
		dotinc=(longpr?4:2);;

		IF charpos()==0 ANDF modifier!='a' THEN aprintf("%16m"); FI

		switch(modifier) {

		    case SP: case TB:
			break;

		    case 't': case 'T':
			aprintf("%T",fcount); return(fp);

		    case 'r': case 'R':
			aprintf("%M",fcount); return(fp);

		    case 'a':
			psymoff(dot,ptype,":%16t"); dotinc=0; break;

		    case 'p':
			psymoff(var[0],ptype,"%16t"); break;

		    case 'u':
			aprintf("%-8u",w); break;

		    case 'U':
			aprintf("%-16U",wx); break;

		    case 'c': case 'C':
			IF modifier=='C'
			THEN printesc(w&LOBYTE);
			ELSE printc(w&LOBYTE);
			FI
			dotinc=1; break;

		    case 'b': case 'B':
			aprintf("%-8o", w&LOBYTE); dotinc=1; break;

		    case 's': case 'S':
			savdot=dot; dotinc=1;
			WHILE (c=get(dot,itype)&LOBYTE) ANDF errflg==0
			DO dot=inkdot(1);
			   IF modifier == 'S'
			   THEN printesc(c);
			   ELSE printc(c);
			   FI
			   endline();
			OD
			dotinc=dot-savdot+1; dot=savdot; break;

		    case 'x':
			aprintf("%-8x",w); break;

		    case 'X':
			aprintf("%-16X", wx); break;

		    case 'Y':
			aprintf("%-24Y", wx); break;

		    case 'q':
			aprintf("%-8q", w); break;

		    case 'Q':
			aprintf("%-16Q", wx); break;

		    case 'o':
		    case 'w':
			aprintf("%-8o", w); break;

		    case 'O':
		    case 'W':
			aprintf("%-16O", wx); break;

		    case 'i':
			printins(0,itype,w); printc(EOR); break;

		    case 'd':
			aprintf("%-8d", w); break;

		    case 'D':
			aprintf("%-16D", wx); break;

		    case 'f':
			fw.s.sa = 0; fw.s.sb = 0; fw.s.sc = 0;	/* was fw.f = 0; pcc pdp11 ICE on int->double union member */
			fw.s.sa = wx;
			aprintf("%-16.9f", fw.f);
			dotinc=4; break;

		    case 'F':
			fw.s.sa = wx;
			aprintf("%-32.18F", fw.f);
			dotinc=8; break;

		    case 'n': case 'N':
			printc('\n'); dotinc=0; break;

		    case '"':
			dotinc=0;
			WHILE *fp != '"' ANDF *fp
			DO printc(*fp++); OD
			IF *fp THEN fp++; FI
			break;

		    case '^':
			dot=inkdot(-dotinc*fcount); return(fp);

		    case '+':
			dot=inkdot(fcount); return(fp);

		    case '-':
			dot=inkdot(-fcount); return(fp);

		    default: error(BADMOD);
		}
		IF itype!=NSP
		THEN	dot=inkdot(dotinc);
		FI
		fcount--; endline();
	OD

	return(fp);
}

int16_t unox(void)
{
	INT		rc, status, unixpid;
	STRING		argp;	/* V7 `STRING argp lp;' declared argp (local) + left lp global */

	WHILE lastc!=EOR DO rdc(); OD
	IF (unixpid=fork())==0
	THEN	signal(SIGINT,(void (*)(int))sigint); signal(SIGQUIT,(void (*)(int))sigqit);
		*lp=0; execl("/bin/sh", "sh", "-c", argp, 0);
		exit(16);
	ELIF unixpid == -1
	THEN	error(NOFORK);
	ELSE	signal(SIGINT,1);
		WHILE (rc = wait(&status)) != unixpid ANDF rc != -1 DONE
		signal(SIGINT,(void (*)(int))sigint);
		prints("!"); lp--;
	FI
}


int16_t printesc(int16_t c)
{
	c &= STRIP;
	IF c<SP ORF c>'~' ORF c=='@'
	THEN aprintf("@%c",(c=='@' ? '@' : c^0140));
	ELSE printc(c);
	FI
}

L_INT inkdot(int16_t incr)
{
	L_INT		newdot;

	newdot=dot+incr;
	IF (dot NEQ newdot) >> 24 THEN error(ADWRAP); FI
	return(newdot);
}
