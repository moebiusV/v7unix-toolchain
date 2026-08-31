#
/*
 * UNIX shell
 *
 * S. R. Bourne
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>


 static VOID	gsort();

enum { ARGMK = 01 };


/* fault handling */



/* service routines for `execute' */

STRING catpath(STRING path, STRING name);
static STRING execs(STRING ap, STRING t[]);
static VOID gsort(STRING from[], STRING to[]);
STRING mactrim(STRING s);
static INT split(STRING s);
void trim(STRING at);

VOID initio(IOPTR iop)
{
	register STRING	ion;
	register INT		iof, fd;

	if( iop
	){	iof=iop->iofile;
		ion=mactrim(iop->ioname);
		if( *ion && (flags&noexec)==0
		){	if( iof&IODOC
			){	subst(chkopen(ion),(fd=tmpfil()));
				close(fd); fd=chkopen(tmpout); unlink(tmpout);
			} else if ( iof&IOMOV
			){	if( eq(minus,ion)
				){	fd = -1;
					close(iof&IOUFD);
				} else if ( (fd=stoi(ion))>=USERIO
				){	failed(ion,badfile);
				} else {	fd=dup(fd);
				;}
			} else if ( (iof&IOPUT)==0
			){	fd=chkopen(ion);
			} else if ( flags&rshflg
			){	failed(ion,restricted);
			} else if ( iof&IOAPP && (fd=open(ion,1))>=0
			){	lseek(fd, 0L, 2);
			} else {	fd=create(ion);
			;}
			if( fd>=0
			){	rename_sh(fd,iof&IOUFD);
			;}
		;}
		initio(iop->ionxt);
	;}
}

STRING getpath(STRING s)
{
	register STRING	path;
	if( any('/',s)
	){	if( flags&rshflg
		){	failed(s, restricted);
		} else {	return(nullstr);
		;}
	} else if ( (path = pathnod.namval)==0
	){	return(defpath);
	} else {	return(cpystak(path));
	;}
}

INT pathopen(STRING path, STRING name)
{
	register UFD		f;

	do{ path=catpath(path,name);
	}while( (f=open(curstak(),0))<0 && path );
	return(f);
}

STRING catpath(STRING path, STRING name)
{
	/* leaves result on top of stack */
	register STRING	scanp = path,
			argp = locstak();

	while( *scanp && *scanp!=COLON ){ *argp++ = *scanp++ ;}
	if( scanp!=path ){ *argp++='/' ;}
	if( *scanp==COLON ){ scanp++ ;}
	path=(*scanp ? scanp : 0); scanp=name;
	while( (*argp++ = *scanp++) );
	return(path);
}

static STRING	xecmsg;
static STRING	*xecenv;

VOID execa(STRING at[])
{
	register STRING	path;
	register STRING	*t = at;

	if( (flags&noexec)==0
	){	xecmsg=notfound; path=getpath(*t);
		namscan(exname);
		xecenv=setenv_sh();
		while( path=execs(path,t) );
		failed(*t,xecmsg);
	;}
}

static STRING execs(STRING ap, STRING t[])
{
	register STRING	p, prefix;

	prefix=catpath(ap,t[0]);
	trim(p=curstak());

	sigchk();
	execve(p, &t[0] ,xecenv);
	switch( errno ){

	    case ENOEXEC:
		flags=0;
		comdiv=0; ioset=0;
		clearup(); /* remove open files and for loop junk */
		if( input ){ close(input) ;}
		close(output); output=2;
		input=chkopen(p);

		/* set up new args */
		setargs(t);
		longjmp(subshell,1);

	    case ENOMEM:
		failed(p,toobig);

	    case E2BIG:
		failed(p,arglist);

	    case ETXTBSY:
		failed(p,txtbsy);

	    default:
		xecmsg=badexec;
	    case ENOENT:
		return(prefix);
	}
}

/* for processes to be waited for */
enum { MAXP = 20 };
static INT	pwlist[MAXP];
static INT	pwc;

void postclr(void)
{
	register INT		*pw = pwlist;

	while( pw <= &pwlist[pwc]
	){ *pw++ = 0 ;}
	pwc=0;
}

VOID post(INT pcsid)
{
	register INT		*pw = pwlist;

	if( pcsid
	){	while( *pw ){ pw++ ;}
		if( pwc >= MAXP-1
		){	pw--;
		} else {	pwc++;
		;}
		*pw = pcsid;
	;}
}

VOID await(INT i)
{
	INT		rc=0, wx=0;
	INT		w;
	INT		ipwc = pwc;

	post(i);
	while( pwc
	){	register INT		p;
		register INT		sig;
		INT		w_hi;

		{
		   register INT	*pw=pwlist;
		   p=wait(&w);
		   while( pw <= &pwlist[ipwc]
		   ){ if( *pw==p
		      ){ *pw=0; pwc--;
		      } else { pw++;
		      ;}
		   ;}
		}

		if( p == -1 ){ continue ;}

		w_hi = (w>>8)&LOBYTE;

		if( sig = w&0177
		){	if( sig == 0177	/* ptrace! return */
			){	prs("ptrace: ");
				sig = w_hi;
			;}
			if( sysmsg[sig]
			){	if( i!=p || (flags&prompt)==0 ){ prp(); prn(p); blank() ;}
				prs(sysmsg[sig]);
				if( w&0200 ){ prs(coredump) ;}
			;}
			newline();
		;}

		if( rc==0
		){	rc = (sig ? sig|SIGFLG : w_hi);
		;}
		wx |= w;
	;}

	if( wx && flags&errflg
	){	exitsh(rc);
	;}
	exitval=rc; exitset();
}


void trim(STRING at)
{
	register STRING	p;
	register CHAR	c;
	register CHAR	q=0;

	if( p=at
	){	while( c = *p
		){ *p++=c&STRIP; q |= c ;}
	;}
	nosubst=q&QUOTE;
}

STRING mactrim(STRING s)
{
	register STRING	t=macro(s);
	trim(t);
	return(t);
}

STRING * scan(INT argn)
{
	register ARGPTR	argp = (ARGPTR)(Rcheat(gchain)&~ARGMK);
	register STRING	*comargn, *comargm;

	comargn=getstak(BYTESPERWORD*argn+BYTESPERWORD); comargm = comargn += argn; *comargn = ENDARGS;

	while( argp
	){	*--comargn = argp->argval;
		if( argp = argp->argnxt
		){ trim(*comargn);
		;}
		if( argp==0 || Rcheat(argp)&ARGMK
		){	gsort(comargn,comargm);
			comargm = comargn;
		;}
		/* Lcheat(argp) &= ~ARGMK; */
		argp = (ARGPTR)(Rcheat(argp)&~ARGMK);
	;}
	return(comargn);
}

static VOID gsort(STRING from[], STRING to[])
{
	INT		k, m, n;
	register INT		i, j;

	if( (n=to-from)<=1 ){ return ;}

	for( j=1; j<=n; j*=2 );

	for( m=2*j-1; m/=2;
	){  k=n-m;
	    for( j=0; j<k; j++
	    ){	for( i=j; i>=0; i-=m
		){  register STRING *fromi; fromi = &from[i];
		    if( cf(fromi[m],fromi[0])>0
		    ){ break;
		    } else { STRING s; s=fromi[m]; fromi[m]=fromi[0]; fromi[0]=s;
		    ;}
		;}
	    ;}
	;}
}

/* Argument list generation */

INT getarg(COMPTR ac)
{
	register ARGPTR	argp;
	register INT		count=0;
	register COMPTR	c;

	if( c=ac
	){	argp=c->comarg;
		while( argp
		){	count += split(macro(argp->argval));
			argp=argp->argnxt;
		;}
	;}
	return(count);
}

static INT split(STRING s)
{
	register STRING	argp;
	register INT		c;
	INT		count=0;

	for(;;){	sigchk(); argp=locstak()+BYTESPERWORD;
		while( (c = *s++, !any(c,ifsnod.namval) && c)
		){ *argp++ = c ;}
		if( argp==staktop+BYTESPERWORD
		){	if( c
			){	continue;
			} else {	return(count);
			;}
		} else if ( c==0
		){	s--;
		;}
		if( c=expand(((ARGPTR)(argp=endstak(argp)))->argval,0)
		){	count += c;
		} else {	/* assign(&fngnod, argp->argval); */
			makearg((ARGPTR)argp); count++;
		;}
		Lcheat(gchain) |= ARGMK;
	}
}
