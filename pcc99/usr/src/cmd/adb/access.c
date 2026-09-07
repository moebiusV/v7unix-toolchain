
#
/*
 *
 *	UNIX debugger
 *
 */

#include "defs.h"
#include <unistd.h>
#include <stdint.h>


MSG		ODDADR;
MSG		BADDAT;
MSG		BADTXT;
MAP		txtmap;
MAP		datmap;
INT		wtflag;
STRING		errflg;
INT		errno;

INT		pid;




/* file handling and adaccess routines */

int16_t adaccess(int16_t mode, L_INT adr, int16_t space, int16_t value);
int16_t chkmap(L_INT *adr, INT space);
int16_t within(L_INT adr, L_INT lbd, L_INT ubd);

int16_t put(L_INT adr, int16_t space, int16_t value)
{
	adaccess(WT,adr,space,value);
}

POS get(L_INT adr, int16_t space)
{
	return(adaccess(RD,adr,space,0));
}

POS chkget(L_INT n, int16_t space)
{
	REG INT		w;

	w = get(n, space);
	chkerr();
	return(w);
}

int16_t adaccess(int16_t mode, L_INT adr, int16_t space, int16_t value)
{
	INT		w, w1, pmode, rd, file;
	rd = mode==RD;

	IF space == NSP THEN return(0); FI

	IF pid		/* tracing on? */
	THEN IF (adr&01) ANDF !rd THEN error(ODDADR); FI
	     pmode = (space&DSP?(rd?RDUSER:WDUSER):(rd?RIUSER:WIUSER));
	     w = ptrace(pmode, pid, shorten(adr&~01), value);
	     IF adr&01
	     THEN w1 = ptrace(pmode, pid, shorten(adr+1), value);
		  w = (w>>8)&LOBYTE | (w1<<8);
	     FI
	     IF errno
	     THEN errflg = (space&DSP ? BADDAT : BADTXT);
	     FI
	     return(w);
	FI
	w = 0;
	IF mode==WT ANDF wtflag==0
	THEN	error("not in write mode");
	FI
	IF !chkmap(&adr,space)
	THEN return(0);
	FI
	file=(space&DSP?datmap.ufd:txtmap.ufd);
	IF longseek(file,adr)==0 ORF
	   (rd ? read(file,&w,2) : write(file,&value,2)) < 1
	THEN	errflg=(space&DSP?BADDAT:BADTXT);
	FI
	return(w);

}

int16_t chkmap(L_INT *adr, INT space)
{
	REG MAPPTR amap;
	amap=((space&DSP?&datmap:&txtmap));
	IF space&STAR ORF !within(*adr,amap->b1,amap->e1)
	THEN IF within(*adr,amap->b2,amap->e2)
	     THEN *adr += (amap->f2)-(amap->b2);
	     ELSE errflg=(space&DSP?BADDAT:BADTXT); return(0);
	     FI
	ELSE *adr += (amap->f1)-(amap->b1);
	FI
	return(1);
}

int16_t within(L_INT adr, L_INT lbd, L_INT ubd)
{
	return(adr>=lbd && adr<ubd);
}
