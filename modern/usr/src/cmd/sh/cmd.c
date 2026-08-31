#
/*
 * UNIX shell
 *
 * S. R. Bourne
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include	"sym.h"
#include <stdint.h>


/* ========	command line decoding	========*/




TREPTR makefork(INT flgs, TREPTR i)
{
	register FORKPTR	t;

	t=getstak(FORKTYPE);
	t->forktyp=flgs|TFORK; t->forktre=i; t->forkio=0;
	return((TREPTR)t);
}

static VOID chksym(int sym);
static VOID chkword(void);
static IOPTR inout(IOPTR lastio);
static TREPTR item(BOOL flag);
static TREPTR list(int flg);
static INT skipnl(void);
static VOID synbad(void);
static TREPTR term(int flg);

static TREPTR makelist(INT type, TREPTR i, TREPTR r)
{
	register LSTPTR	t;

	if( i==0 || r==0
	){	synbad();
	} else {	t = getstak(LSTTYPE);
		t->lsttyp = type;
		t->lstlef = i; t->lstrit = r;
	;}
	return((TREPTR)t);
}

/*
 * cmd
 *	empty
 *	list
 *	list & [ cmd ]
 *	list [ ; cmd ]
 */

TREPTR cmd(INT sym, INT flg)
{
	register TREPTR	i, e;

	i = list(flg);

	if( wdval==NL
	){	if( flg&NLFLG
		){	wdval=';'; chkpr(NL);
		;}
	} else if ( i==0 && (flg&MTFLG)==0
	){	synbad();
	;}

	switch( wdval ){

	    case '&':
		if( i
		){	i = makefork(FINT|FPRS|FAMP, i);
		} else {	synbad();
		;}

	    case ';':
		if( e=cmd(sym,flg|MTFLG)
		){	i=makelist(TLST, i, e);
		;}
		break;

	    case EOFSYM:
		if( sym==NL
		){	break;
		;}

	    default:
		if( sym
		){	chksym(sym);
		;}

	}
	return(i);
}

/*
 * list
 *	term
 *	list && term
 *	list || term
 */

static TREPTR list(int flg)
{
	register TREPTR	r;
	register INT		b;

	r = term(flg);
	while( r && ((b=(wdval==ANDFSYM)) || wdval==ORFSYM)
	){	r = makelist((b ? TAND : TORF), r, term(NLFLG));
	;}
	return(r);
}

/*
 * term
 *	item
 *	item |^ term
 */

static TREPTR term(int flg)
{
	register TREPTR	t;

	reserv++;
	if( flg&NLFLG
	){	skipnl();
	} else {	word();
	;}

	if( (t=item(1)) && (wdval=='^' || wdval=='|')
	){	return(makelist(TFIL, makefork(FPOU,t), makefork(FPIN|FPCL,term(NLFLG))));
	} else {	return(t);
	;}
}

static REGPTR syncase(INT esym)
{
	skipnl();
	if( wdval==esym
	){	return(0);
	} else {	register REGPTR	r=getstak(REGTYPE);
		r->regptr=0;
		for(;;){ wdarg->argnxt=r->regptr;
		     r->regptr=wdarg;
		     if( wdval || ( word()!=')' && wdval!='|' )
		     ){ synbad();
		     ;}
		     if( wdval=='|'
		     ){ word();
		     } else { break;
		     ;}
		}
		r->regcom=cmd(0,NLFLG|MTFLG);
		if( wdval==ECSYM
		){	r->regnxt=syncase(esym);
		} else {	chksym(esym);
			r->regnxt=0;
		;}
		return(r);
	;}
}

/*
 * item
 *
 *	( cmd ) [ < in  ] [ > out ]
 *	word word* [ < in ] [ > out ]
 *	if ... then ... else ... fi
 *	for ... while ... do ... done
 *	case ... in ... esac
 *	begin ... end
 */

static TREPTR item(BOOL flag)
{
	register TREPTR	t;
	register IOPTR	io;

	if( flag
	){	io=inout((IOPTR)0);
	} else {	io=0;
	;}

	switch( wdval ){

	    case CASYM:
		{
		   t=getstak(SWTYPE);
		   chkword();
		   ((SWPTR)t)->swarg=wdarg->argval;
		   skipnl(); chksym(INSYM|BRSYM);
		   ((SWPTR)t)->swlst=syncase(wdval==INSYM?ESSYM:KTSYM);
		   ((SWPTR)t)->swtyp=TSW;
		   break;
		}

	    case IFSYM:
		{
		   register INT	w;
		   t=getstak(IFTYPE);
		   ((IFPTR)t)->iftyp=TIF;
		   ((IFPTR)t)->iftre=cmd(THSYM,NLFLG);
		   ((IFPTR)t)->thtre=cmd(ELSYM|FISYM|EFSYM,NLFLG);
		   ((IFPTR)t)->eltre=((w=wdval)==ELSYM ? cmd(FISYM,NLFLG) : (w==EFSYM ? (wdval=IFSYM, item(0)) : 0));
		   if( w==EFSYM ){ return(t) ;}
		   break;
		}

	    case FORSYM:
		{
		   t=getstak(FORTYPE);
		   ((FORPTR)t)->fortyp=TFOR;
		   ((FORPTR)t)->forlst=0;
		   chkword();
		   ((FORPTR)t)->fornam=wdarg->argval;
		   if( skipnl()==INSYM
		   ){	chkword();
			((FORPTR)t)->forlst=(COMPTR)item(0);
			if( wdval!=NL && wdval!=';'
			){	synbad();
			;}
			chkpr(wdval); skipnl();
		   ;}
		   chksym(DOSYM|BRSYM);
		   ((FORPTR)t)->fortre=cmd(wdval==DOSYM?ODSYM:KTSYM,NLFLG);
		   break;
		}

	    case WHSYM:
	    case UNSYM:
		{
		   t=getstak(WHTYPE);
		   ((WHPTR)t)->whtyp=(wdval==WHSYM ? TWH : TUN);
		   ((WHPTR)t)->whtre = cmd(DOSYM,NLFLG);
		   ((WHPTR)t)->dotre = cmd(ODSYM,NLFLG);
		   break;
		}

	    case BRSYM:
		t=cmd(KTSYM,NLFLG);
		break;

	    case '(':
		{
		   register PARPTR	 p;
		   p=getstak(PARTYPE);
		   p->partre=cmd(')',NLFLG);
		   p->partyp=TPAR;
		   t=makefork(0,(TREPTR)p);
		   break;
		}

	    default:
		if( io==0
		){	return(0);
		;}

	    case 0:
		{
		   register ARGPTR	argp;
		   register ARGPTR	*argtail;
		   register ARGPTR	argset=0;
		   INT		keywd=1;
		   t=getstak(COMTYPE);
		   ((COMPTR)t)->comio=io; /*initial io chain*/
		   argtail = &(((COMPTR)t)->comarg);
		   while( wdval==0
		   ){	argp = wdarg;
			if( wdset && keywd
			){	argp->argnxt=argset; argset=argp;
			} else {	*argtail=argp; argtail = &(argp->argnxt); keywd=flags&keyflg;
			;}
			word();
			if( flag
			){ ((COMPTR)t)->comio=inout(((COMPTR)t)->comio);
			;}
		   ;}

		   ((COMPTR)t)->comtyp=TCOM; ((COMPTR)t)->comset=argset; *argtail=0;
		   return(t);
		}

	}
	reserv++; word();
	if( io=inout(io)
	){	t=makefork(0,t); t->treio=io;
	;}
	return(t);
}


static INT skipnl(void)
{
	while( (reserv++, word()==NL) ){ chkpr(NL) ;}
	return(wdval);
}

static IOPTR inout(IOPTR lastio)
{
	register INT		iof;
	register IOPTR	iop;
	register CHAR	c;

	iof=wdnum;

	switch( wdval ){

	    case DOCSYM:
		iof |= IODOC; break;

	    case APPSYM:
	    case '>':
		if( wdnum==0 ){ iof |= 1 ;}
		iof |= IOPUT;
		if( wdval==APPSYM
		){	iof |= IOAPP; break;
		;}

	    case '<':
		if( (c=nextc(0))=='&'
		){	iof |= IOMOV;
		} else if ( c=='>'
		){	iof |= IORDW;
		} else {	peekc=c|MARK;
		;}
		break;

	    default:
		return(lastio);
	}

	chkword();
	iop=getstak(IOTYPE); iop->ioname=wdarg->argval; iop->iofile=iof;
	if( iof&IODOC
	){ iop->iolst=iopend; iopend=iop;
	;}
	word(); iop->ionxt=inout(lastio);
	return(iop);
}

static VOID chkword(void)
{
	if( word()
	){	synbad();
	;}
}

static VOID chksym(int sym)
{
	register INT		x = sym&wdval;
	if( ((x&SYMFLG) ? x : sym) != wdval
	){	synbad();
	;}
}

static VOID prsym(int sym)
{
	if( sym&SYMFLG
	){	register SYSPTR	sp=reserved;
		while( sp->sysval
			&& sp->sysval!=sym
		){ sp++ ;}
		prs(sp->sysnam);
	} else if ( sym==EOFSYM
	){	prs(endoffile);
	} else {	if( sym&SYMREP ){ prc(sym) ;}
		if( sym==NL
		){	prs("newline");
		} else {	prc(sym);
		;}
	;}
}

static VOID synbad(void)
{
	prp(); prs(synmsg);
	if( (flags&ttyflg)==0
	){	prs(atline); prn(standin->flin);
	;}
	prs(colon);
	prc(LQ);
	if( wdval
	){	prsym(wdval);
	} else {	prs(wdarg->argval);
	;}
	prc(RQ); prs(unexpected);
	newline();
	exitsh(SYNBAD);
}
