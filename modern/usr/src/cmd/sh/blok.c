#
/*
 *	UNIX shell
 *
 *	S. R. Bourne
 *	Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include <stdint.h>


/*
 *	storage allocator
 *	(circular first fit strategy)
 */

enum { BUSY = 01 };
static inline int busy(BLKPTR x) { return Rcheat(x->word) & BUSY; }

POS		brkincr=BRKINCR;
BLKPTR		blokp;			/*current search pointer*/
BLKPTR		bloktop;	/*top of arena (last blok)*/



VOID addblok(POS reqd);

void *alloc_sh(POS nbytes)
{
	register POS		rbytes = (POS)roundup(nbytes+BYTESPERWORD,BYTESPERWORD);

	for(;;){	INT		c=0;
		register BLKPTR	p = blokp;
		register BLKPTR	q;
		do{	if( !busy(p)
			){	while( !busy(q = p->word) ){ p->word = q->word ;}
				if( ADR(q)-ADR(p) >= rbytes
				){	blokp = BLK(ADR(p)+rbytes);
					if( q > blokp
					){	blokp->word = p->word;
					;}
					p->word=BLK(Rcheat(blokp)|BUSY);
					return(ADR(p+1));
				;}
			;}
			q = p; p = BLK(Rcheat(p->word)&~BUSY);
		}while(	p>q || (c++)==0 );
		addblok(rbytes);
	}
}

VOID addblok(POS reqd)
{
	if( stakbas!=staktop
	){	register STKPTR	rndstak;
		register BLKPTR	blokstak;

		pushstak(0);
		rndstak=(STKPTR)roundup((uintptr_t)staktop,BYTESPERWORD);
		blokstak=BLK(stakbas)-1;
		blokstak->word=stakbsy; stakbsy=blokstak;
		bloktop->word=BLK(Rcheat(rndstak)|BUSY);
		bloktop=BLK(rndstak);
	;}
	reqd += brkincr; reqd &= ~(brkincr-1);
	setbrk(reqd);	/* v7 grew the brk on the MEMF trap; do it proactively */
	blokp=bloktop;
	bloktop=bloktop->word=BLK(Rcheat(bloktop)+reqd);
	bloktop->word=BLK(end+1);
	{
	   register STKPTR stakadr=STK(bloktop+2);
	   staktop=movstr(stakbot,stakadr);
	   stakbas=stakbot=stakadr;
	}
}

VOID free_sh(BLKPTR ap)
{
	register BLKPTR	p;

	if( (p=ap) && p<bloktop
	){	Lcheat((--p)->word) &= ~BUSY;
	;}
}

#ifdef DEBUG
chkbptr(ptr)
	BLKPTR	ptr;
{
	INT		exf=0;
	register BLKPTR	p = end;
	register BLKPTR	q;
	INT		us=0, un=0;

	for(;;){
	   q = Rcheat(p->word)&~BUSY;
	   if( p==ptr ){ exf++ ;}
	   if( q<end || q>bloktop ){ abort(3) ;}
	   if( p==bloktop ){ break ;}
	   if( busy(p)
	   ){ us += q-p;
	   } else { un += q-p;
	   ;}
	   if( p>=q ){ abort(4) ;}
	   p=q;
	}
	if( exf==0 ){ abort(1) ;}
	prn(un); prc(SP); prn(us); prc(NL);
}
#endif
