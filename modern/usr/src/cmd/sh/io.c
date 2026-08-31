#
/*
 * UNIX shell
 *
 * S. R. Bourne
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include	"dup.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>


/* ========	input output and file copying ======== */

void initf(UFD fd)
{
	register FILE	f=standin;

	f->fdes=fd; f->fsiz=((flags&(oneflg|ttyflg))==0 ? BUFSIZ : 1);
	f->fnxt=f->fend=f->fbuf; f->feval=0; f->flin=1;
	f->feof=0;
}

int estabf(STRING s)
{
	register FILE	f;

	(f=standin)->fdes = -1;
	f->fend=length(s)+(f->fnxt=s);
	f->flin=1;
	return(f->feof=(s==0));
}

void push(FILE af)
{
	register FILE	f;

	(f=af)->fstak=standin;
	f->feof=0; f->feval=0;
	standin=f;
}

int pop(void)
{
	register FILE	f;

	if( (f=standin)->fstak
	){	if( f->fdes>=0 ){ close(f->fdes) ;}
		standin=f->fstak;
		return(1);
	} else {	return(0);
	;}
}

void chkpipe(INT *pv)
{
	if( pipe(pv)<0 || pv[INPIPE]<0 || pv[OTPIPE]<0
	){	error(piperr);
	;}
}

int chkopen(STRING idf)
{
	register INT		rc;

	if( (rc=open(idf,0))<0
	){	failed(idf,badopen);
	} else {	return(rc);
	;}
}

void rename_sh(INT f1, INT f2)
{
	if( f1!=f2
	){	dup2(f1, f2);
		close(f1);
		if( f2==0 ){ ioset|=1 ;}
	;}
}

int create(STRING s)
{
	register INT		rc;

	if( (rc=creat(s,0666))<0
	){	failed(s,badcreate);
	} else {	return(rc);
	;}
}

int tmpfil(void)
{
	itos(serial++); movstr(numbuf,tmpnam);
	return(create(tmpout));
}

/* set by trim */
BOOL		nosubst;

void copy(IOPTR ioparg)
{
	CHAR		c, *ends;
	register CHAR	*cline, *clinep;
	INT		fd;
	register IOPTR	iop;

	if( iop=ioparg
	){	copy(iop->iolst);
		ends=mactrim(iop->ioname); if( nosubst ){ iop->iofile &= ~IODOC ;}
		fd=tmpfil();
		iop->ioname=cpystak(tmpout);
		iop->iolst=iotemp; iotemp=iop;
		cline=locstak();

		for(;;){	clinep=cline; chkpr(NL);
			while( (c = (nosubst ? readc() :  nextc(*ends)),  !eolchar(c)) ){ *clinep++ = c ;}
			*clinep=0;
			if( eof || eq(cline,ends) ){ break ;}
			*clinep++=NL;
			write(fd,cline,clinep-cline);
		}
		close(fd);
	;}
}
