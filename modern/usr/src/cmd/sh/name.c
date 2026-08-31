#
/*
 * UNIX shell
 *
 * S. R. Bourne
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

 static BOOL	chkid();


NAMNOD	ps2nod	= {	NIL,		NIL,		ps2name},
	fngnod	= {	NIL,		NIL,		fngname},
	pathnod = {	NIL,		NIL,		pathname},
	ifsnod	= {	NIL,		NIL,		ifsname},
	ps1nod	= {	&pathnod,	&ps2nod,	ps1name},
	homenod = {	&fngnod,	&ifsnod,	homename},
	mailnod = {	&homenod,	&ps1nod,	mailname};

NAMPTR		namep = &mailnod;


/* ========	variable and string handling	======== */

int syslook(STRING w, SYSTAB syswds)
{
	register CHAR	first;
	register STRING	s;
	register SYSPTR	syscan;

	syscan=syswds; first = *w;

	while( s=syscan->sysnam
	){  if( first == *s
		&& eq(w,s)
	    ){ return(syscan->sysval);
	    ;}
	    syscan++;
	;}
	return(0);
}

void assign(NAMPTR n, STRING v);
static BOOL chkid(STRING nam);
NAMPTR lookup(STRING nam);
STRING make(STRING v);
static VOID namwalk(NAMPTR np);
VOID setname(STRING argi, INT xp);

void setlist(ARGPTR arg, INT xp)
{
	while( arg
	){ register STRING	s=mactrim(arg->argval);
	   setname(s, xp);
	   arg=arg->argnxt;
	   if( flags&execpr
	   ){ prs(s);
		if( arg ){ blank(); } else { newline(); ;}
	   ;}
	;}
}

VOID setname(STRING argi, INT xp)
{
	register STRING	argscan=argi;
	register NAMPTR	n;

	if( letter(*argscan)
	){	while( alphanum(*argscan) ){ argscan++ ;}
		if( *argscan=='='
		){	*argscan = 0;
			n=lookup(argi);
			*argscan++ = '=';
			attrib(n, xp);
			if( xp&N_ENVNAM
			){	n->namenv = n->namval = argscan;
			} else {	assign(n, argscan);
			;}
			return;
		;}
	;}
	failed(argi,notid);
}

void replace(STRING *a, STRING v)
{
	free_sh(*a); *a=make(v);
}

void dfault(NAMPTR n, STRING v)
{
	if( n->namval==0
	){	assign(n,v)
	;}
}

void assign(NAMPTR n, STRING v)
{
	if( n->namflg&N_RDONLY
	){	failed(n->namid,wtfailed);
	} else {	replace(&n->namval,v);
	;}
}

INT readvar(STRING *names)
{
	FILEBLK		fb;
	register FILE	f = &fb;
	register CHAR	c;
	register INT		rc=0;
	NAMPTR		n=lookup(*names++); /* done now to avoid storage mess */
	int		rel=relstak();

	push(f); initf(dup(0));
	if( lseek(0,0L,1)==-1
	){	f->fsiz=1;
	;}

	for(;;){	c=nextc(0);
		if( (*names && any(c, ifsnod.namval)) || eolchar(c)
		){	zerostak();
			assign(n,absstak(rel)); setstak(rel);
			if( *names
			){	n=lookup(*names++);
			} else {	n=0;
			;}
			if( eolchar(c)
			){	break;
			;}
		} else {	pushstak(c);
		;}
	}
	while( n
	){ assign(n, nullstr);
	   if( *names ){ n=lookup(*names++); } else { n=0; ;}
	;}

	if( eof ){ rc=1 ;}
	lseek(0, (int)(f->fnxt-f->fend), 1);
	pop();
	return(rc);
}

void assnum(STRING *p, INT i)
{
	itos(i); replace(p,numbuf);
}

STRING make(STRING v)
{
	register STRING	p;

	if( v
	){	movstr(v,p=alloc_sh(length(v)));
		return(p);
	} else {	return(0);
	;}
}


NAMPTR lookup(STRING nam)
{
	register NAMPTR	nscan=namep;
	register NAMPTR	*prev;
	INT		LR;

	if( !chkid(nam)
	){	failed(nam,notid);
	;}
	while( nscan
	){	if( (LR=cf(nam,nscan->namid))==0
		){	return(nscan);
		} else if ( LR<0
		){	prev = &(nscan->namlft);
		} else {	prev = &(nscan->namrgt);
		;}
		nscan = *prev;
	;}

	/* add name node */
	nscan=alloc_sh(sizeof *nscan);
	nscan->namlft=nscan->namrgt=NIL;
	nscan->namid=make(nam);
	nscan->namval=0; nscan->namflg=N_DEFAULT; nscan->namenv=0;
	return(*prev = nscan);
}

static BOOL chkid(STRING nam)
{
	register CHAR *	cp=nam;

	if( !letter(*cp)
	){	return(0);
	} else {	while( *++cp
		){ if( !alphanum(*cp)
		   ){	return(0);
		   ;}
		;}
	;}
	return(1);
}

static VOID (*namfn)(NAMPTR);
VOID namscan(void (*fn)(NAMPTR))
{
	namfn=fn;
	namwalk(namep);
}

static VOID namwalk(NAMPTR np)
{
	if( np
	){	namwalk(np->namlft);
		(*namfn)(np);
		namwalk(np->namrgt);
	;}
}

VOID printnam(NAMPTR n)
{
	register STRING	s;

	sigchk();
	if( s=n->namval
	){	prs(n->namid);
		prc('='); prs(s);
		newline();
	;}
}

static STRING staknam(NAMPTR n)
{
	register STRING	p;

	p=movstr(n->namid,staktop);
	p=movstr("=",p);
	p=movstr(n->namval,p);
	return(getstak(p+1-ADR(stakbot)));
}

VOID exname(NAMPTR n)
{
	if( n->namflg&N_EXPORT
	){	free_sh(n->namenv);
		n->namenv = make(n->namval);
	} else {	free_sh(n->namval);
		n->namval = make(n->namenv);
	;}
}

VOID printflg(NAMPTR n)
{
	if( n->namflg&N_EXPORT
	){	prs(export); blank();
	;}
	if( n->namflg&N_RDONLY
	){	prs(readonly); blank();
	;}
	if( n->namflg&(N_EXPORT|N_RDONLY)
	){	prs(n->namid); newline();
	;}
}

VOID getenv_sh(void)
{
	register STRING	*e=environ;

	while( *e
	){ setname(*e++, N_ENVNAM) ;}
}

static INT	namec;

VOID countnam(NAMPTR n)
{
	namec++;
}

static STRING 	*argnam;

VOID pushnam(NAMPTR n)
{
	if( n->namval
	){	*argnam++ = staknam(n);
	;}
}

STRING * setenv_sh(void)
{
	register STRING	*er;

	namec=0;
	namscan(countnam);
	argnam = er = getstak(namec*BYTESPERWORD+BYTESPERWORD);
	namscan(pushnam);
	*argnam++ = 0;
	return(er);
}
