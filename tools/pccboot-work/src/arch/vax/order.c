/*	$Id$	*/
/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code and documentation must retain the above
 * copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditionsand the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 * 	This product includes software developed or owned by Caldera
 *	International, Inc.
 * Neither the name of Caldera International, Inc. nor the names of other
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OFLIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

# include "pass2.h"

int canaddr(NODE *);

#define INCMARK	0x4003	/* n_qual of a UMUL whose address offstar() left to (rN)+ */

int maxargs = { -1 };

#if 0
stoasg( p, o ) register NODE *p; {
	/* should the assignment op p be stored,
	   given that it lies as the right operand of o
	   (or the left, if o==UNARY MUL) */
/*
	if( p->op == INCR || p->op == DECR ) return;
	if( o==UNARY MUL && p->left->op == REG && !isbreg(p->left->rval) ) SETSTO(p,INAREG);
 */
	}

int
deltest( p ) register NODE *p; {
	/* should we delay the INCR or DECR operation p */
	p = p->n_left;
	return( p->n_op == REG || p->n_op == NAME || p->n_op == OREG );
	}

autoincr( p ) NODE *p; {
	register NODE *q = p->left, *r;

	if( q->op == INCR && (r=q->left)->op == REG &&
	    ISPTR(q->type) && p->type == DECREF(q->type) &&
	    tlen(p) == q->right->lval ) return(1);

	return(0);
	}

mkadrs(p) register NODE *p; {
	register o;

	o = p->op;

	if( asgop(o) ){
		if( p->left->su >= p->right->su ){
			if( p->left->op == UNARY MUL ){
				SETSTO( p->left->left, INTEMP );
				}
			else if( p->left->op == FLD && p->left->left->op == UNARY MUL ){
				SETSTO( p->left->left->left, INTEMP );
				}
			else { /* should be only structure assignment */
				SETSTO( p->left, INTEMP );
				}
			}
		else SETSTO( p->right, INTEMP );
		}
	else {
		if( p->left->su > p->right->su ){
			SETSTO( p->left, INTEMP );
			}
		else {
			SETSTO( p->right, INTEMP );
			}
		}
	}
#endif

int
notoff(TWORD t, int r, CONSZ off, char *cp)
{
	/* is it legal to make an OREG or NAME entry which has an
	 * offset of off, (from a register of r), if the
	 * resulting thing had type t */

/*	if( r == R0 ) return( 1 );  / * NO */
	return(0);  /* YES */
	}

# define max(x,y) ((x)<(y)?(y):(x))

#if 0
sucomp( p ) register NODE *p; {

	/* set the su field in the node to the sethi-ullman
	   number, or local equivalent */

	register o, ty, sul, sur, r;

	o = p->op;
	ty = optype( o );
	p->su = szty( p->type );   /* 2 for float or double, else 1 */;

	if( ty == LTYPE ){
		if( o == OREG ){
			r = p->rval;
			/* oreg cost is (worst case) 1 + number of temp registers used */
			if( R2TEST(r) ){
				if( R2UPK1(r)!=100 && istreg(R2UPK1(r)) ) ++p->su;
				if( istreg(R2UPK2(r)) ) ++p->su;
				}
			else {
				if( istreg( r ) ) ++p->su;
				}
			}
		if( p->su == szty(p->type) &&
		   (p->op!=REG || !istreg(p->rval)) &&
		   (p->type==INT || p->type==UNSIGNED || p->type==DOUBLE) )
			p->su = 0;
		return;
		}

	else if( ty == UTYPE ){
		switch( o ) {
		case UNARY CALL:
		case UNARY STCALL:
			p->su = fregs;  /* all regs needed */
			return;

		default:
			p->su =  p->left->su + (szty( p->type ) > 1 ? 2 : 0) ;
			return;
			}
		}


	/* If rhs needs n, lhs needs m, regular su computation */

	sul = p->left->su;
	sur = p->right->su;

	if( o == ASSIGN ){
		/* computed by doing right, then left (if not in mem), then doing it */
		p->su = max(sur,sul+1);
		return;
		}

	if( o == CALL || o == STCALL ){
		/* in effect, takes all free registers */
		p->su = fregs;
		return;
		}

	if( o == STASG ){
		/* right, then left */
		p->su = max( max( 1+sul, sur), fregs );
		return;
		}

	if( asgop(o) ){
		/* computed by doing right, doing left address, doing left, op, and store */
		p->su = max(sur,sul+2);
/*
		if( o==ASG MUL || o==ASG DIV || o==ASG MOD) p->su = max(p->su,fregs);
 */
		return;
		}

	switch( o ){
	case ANDAND:
	case OROR:
	case QUEST:
	case COLON:
	case COMOP:
		p->su = max( max(sul,sur), 1);
		return;

	case PLUS:
	case OR:
	case ER:
		/* commutative ops; put harder on left */
		if( p->right->su > p->left->su && !istnode(p->left) ){
			register NODE *temp;
			temp = p->left;
			p->left = p->right;
			p->right = temp;
			}
		break;
		}

	/* binary op, computed by left, then right, then do op */
	p->su = max(sul,szty(p->right->type)+sur);
/*
	if( o==MUL||o==DIV||o==MOD) p->su = max(p->su,fregs);
 */

	}

int radebug = 0;

rallo( p, down ) NODE *p; {
	/* do register allocation */
	register o, type, down1, down2, ty;

	if( radebug ) printf( "rallo( %o, %d )\n", p, down );

	down2 = NOPREF;
	p->rall = down;
	down1 = ( down &= ~MUSTDO );

	ty = optype( o = p->op );
	type = p->type;


	if( type == DOUBLE || type == FLOAT ){
		if( o == FORCE ) down1 = R0|MUSTDO;
		}
	else switch( o ) {
	case ASSIGN:	
		down1 = NOPREF;
		down2 = down;
		break;

/*
	case MUL:
	case DIV:
	case MOD:
		down1 = R3|MUSTDO;
		down2 = R5|MUSTDO;
		break;

	case ASG MUL:
	case ASG DIV:
	case ASG MOD:
		p->left->rall = down1 = R3|MUSTDO;
		if( p->left->op == UNARY MUL ){
			rallo( p->left->left, R4|MUSTDO );
			}
		else if( p->left->op == FLD  && p->left->left->op == UNARY MUL ){
			rallo( p->left->left->left, R4|MUSTDO );
			}
		else rallo( p->left, R3|MUSTDO );
		rallo( p->right, R5|MUSTDO );
		return;
 */

	case CALL:
	case STASG:
	case EQ:
	case NE:
	case GT:
	case GE:
	case LT:
	case LE:
	case NOT:
	case ANDAND:
	case OROR:
		down1 = NOPREF;
		break;

	case FORCE:	
		down1 = R0|MUSTDO;
		break;

		}

	if( ty != LTYPE ) rallo( p->left, down1 );
	if( ty == BITYPE ) rallo( p->right, down2 );

	}
#endif

/*
 * Turn a UMUL-referenced node into OREG.
 * Be careful about register classes, this is a place where classes change.
 */
static int
samereg(NODE *a, NODE *b)
{
	return a->n_op == REG && b->n_op == REG && regno(a) == regno(b);
}

/* size in bytes of what a pointer to t points to; 0 if it has no index/step */
static int
pointee(TWORD t)
{
	if (!ISPTR(t))
		return 0;
	t = DECREF(t);
	if (ISPTR(t))
		return 4;
	switch (t) {
	case CHAR: case UCHAR:
		return 1;
	case SHORT: case USHORT:
		return 2;
	case INT: case UNSIGNED: case LONG: case ULONG: case FLOAT:
		return 4;
	case DOUBLE: case LONGLONG: case ULONGLONG:
		return 8;
	}
	return 0;
}

/*
 * Is q an address that an autoincrement/decrement operand can express?
 *	(p += n) - n	the old value of p, then p advances by what it points
 *			to:	(rN)+
 *	p -= n		p goes back first:	-(rN)
 * Returns 1 for (rN)+, 2 for -(rN) and the register node in *rp.
 */
static int
inctree(NODE *q, int sz, NODE **rp)
{
	NODE *a, *x;
	CONSZ n;
	int mode;

	if (!xdmr)
		return 0;
	if (q->n_op == MINUS && q->n_left->n_op == ASSIGN) {
		a = q->n_left;
		if (a->n_right->n_op != PLUS ||
		    a->n_right->n_right->n_op != ICON ||
		    q->n_right->n_op != ICON ||
		    getlval(a->n_right->n_right) != getlval(q->n_right) ||
		    !samereg(a->n_left, a->n_right->n_left))
			return 0;
		x = a->n_left;
		n = getlval(q->n_right);
		mode = 1;
	} else if (q->n_op == ASSIGN && q->n_right->n_op == MINUS &&
	    q->n_right->n_right->n_op == ICON &&
	    samereg(q->n_left, q->n_right->n_left)) {
		x = q->n_left;
		n = getlval(q->n_right->n_right);
		mode = 2;
	} else
		return 0;

	if (x->n_op != REG || pointee(x->n_type) != sz || n != sz)
		return 0;
	*rp = x;
	return mode;
}

/*
 * An index term: x << k with 1 << k the size of the data, or x itself for
 * bytes, x any int expression.  A constant added to it moves into the
 * displacement: name+4[rI] for a[i+1].  VAX index mode multiplies by the
 * operand size itself.  *kp gets the displacement in bytes.
 */
static NODE *
indexterm(NODE *p, int sz, CONSZ *kp)
{
	NODE *x;

	*kp = 0;
	if (p->n_op == LS && p->n_right->n_op == ICON &&
	    p->n_right->n_name[0] == '\0' && (1 << getlval(p->n_right)) == sz)
		x = p->n_left;
	else if (sz == 1)
		x = p;
	else
		return NULL;
	if (x->n_type != INT && x->n_type != UNSIGNED)
		return NULL;
	if ((x->n_op == PLUS || x->n_op == MINUS) && x->n_right->n_op == ICON &&
	    x->n_right->n_name[0] == '\0' &&
	    (x->n_left->n_type == INT || x->n_left->n_type == UNSIGNED)) {
		*kp = (x->n_op == PLUS ? 1 : -1) * getlval(x->n_right) * sz;
		x = x->n_left;
	}
	return x;
}

/* A symbol address: ICON with a name, or the address of a NAME; returns the
 * node that carries the name and offset. */
static NODE *
symaddr(NODE *n)
{
	if (n->n_op == ICON && n->n_name[0] != '\0')
		return n;
	if (n->n_op == ADDROF && n->n_left->n_op == NAME &&
	    n->n_left->n_name[0] != '\0')
		return n->n_left;
	return NULL;
}

/*
 * sym[rI] and off(rB)[rI]: find the base and the index of an address.
 * *bp is NULL for a symbol (the node carrying it in *cp), else the base node;
 * *dp is the displacement.
 */
static int
indexform(NODE *p, int sz, NODE **bp, NODE **ip, NODE **cp, CONSZ *dp)
{
	NODE *l, *r, *b;
	CONSZ k;

	if (!xdmr || p->n_op != PLUS || (sz != 1 && sz != 2 && sz != 4 && sz != 8))
		return 0;
	l = p->n_left;
	r = p->n_right;
	if ((*cp = symaddr(l)) != NULL && (*ip = indexterm(r, sz, &k))) {
		*bp = NULL;
		*dp = getlval(*cp) + k;
		return 1;
	}
	if ((*cp = symaddr(r)) != NULL && (*ip = indexterm(l, sz, &k))) {
		*bp = NULL;
		*dp = getlval(*cp) + k;
		return 1;
	}
	*cp = NULL;
	if (ISPTR(l->n_type) && l->n_op != ICON && (*ip = indexterm(r, sz, &k))) {
		b = l;
		*dp = k;
		if (b->n_op == PLUS && b->n_right->n_op == ICON &&
		    b->n_right->n_name[0] == '\0' && ISPTR(b->n_left->n_type)) {
			*dp += getlval(b->n_right);
			b = b->n_left;
		}
		*bp = b;
		return 1;
	}
	return 0;
}

/*
 * UMUL of a word in memory: the pointer is fetched and used by the operand
 * itself, *name and *off(rB) (deferred modes), with no register for it.
 * The displacement belongs to the pointer's own address, so only
 * *(name+k) is expressible, not (*name)+k.
 */
static int
deferred(NODE *p, int sz)
{
	if (!xdmr || sz == 0)
		return 0;
	if (p->n_op == NAME) {
		if (p->n_name[0] == '\0')
			return 0;
	} else if (p->n_op == OREG) {
		if (R2TEST(regno(p)) || (getlval(p) == 0 && p->n_name[0] == '\0'))
			return 0;
	} else
		return 0;
	return ISPTR(p->n_type) || p->n_type == INT || p->n_type == UNSIGNED ||
	    p->n_type == LONG || p->n_type == ULONG;
}

/*
 * Turn a UMUL-referenced node into OREG.
 * Be careful about register classes, this is a place where classes change.
 */
void
offstar(NODE *p, int shape)
{
	NODE *r, *b, *i, *c, *x;
	CONSZ d;
	int sz = offstar_um != NULL ? tlen(offstar_um) : 0;

	if (x2debug)
		printf("offstar(%p)\n", p);

	if (isreg(p))
		return; /* Is already OREG */

	if (deferred(p, sz))
		return;	/* *name, *off(rB): converted in myormake() */
	if (inctree(p, sz, &x)) {
		/* (rN)+ and -(rN): converted in myormake(), which must know
		 * that the increment has not been given an instruction */
		if (offstar_um != NULL)
			offstar_um->n_qual = INCMARK;
		return;
	}
	if (indexform(p, sz, &b, &i, &c, &d)) {
		/* base and index in registers; converted in myormake() */
		if (b != NULL && isreg(b) == 0)
			(void)geninsn(b, INAREG);
		if (isreg(i) == 0)
			(void)geninsn(i, INAREG);
		return;
	}

	r = p->n_right;
	if( p->n_op == PLUS || p->n_op == MINUS ){
		if( r->n_op == ICON ){
			if (isreg(p->n_left) == 0)
				(void)geninsn(p->n_left, INAREG);
			/* Converted in ormake() */
			return;
		}
	}
	(void)geninsn(p, INAREG);
}


#if 0
void
offstar( p, s ) register NODE *p; {
	if( p->n_op == PLUS ) {
		if( p->n_left->n_su == fregs ) {
			order( p->n_left, INAREG );
			return;
		} else if( p->n_right->n_su == fregs ) {
			order( p->n_right, INAREG );
			return;
		}
		if( p->n_left->n_op==LS && 
		  (p->n_left->n_left->n_op!=REG || tlen(p->n_left->n_left)!=sizeof(int) ) ) {
			order( p->n_left->n_left, INAREG );
			return;
		}
		if( p->n_right->n_op==LS &&
		  (p->n_right->n_left->n_op!=REG || tlen(p->n_right->n_left)!=sizeof(int) ) ) {
			order( p->n_right->n_left, INAREG );
			return;
		}
		if( p->n_type == (PTR|CHAR) || p->n_type == (PTR|UCHAR) ) {
			if( p->n_left->n_op!=REG || tlen(p->n_left)!=sizeof(int) ) {
				order( p->n_left, INAREG );
				return;
			}
			else if( p->n_right->n_op!=REG || tlen(p->n_right)!=sizeof(int) ) {
				order(p->n_right, INAREG);
				return;
			}
		}
	}
	if( p->n_op == PLUS || p->n_op == MINUS ){
		if( p->n_right->n_op == ICON ){
			p = p->n_left;
			order( p , INAREG);
			return;
			}
		}

	if( p->n_op == UMUL && !canaddr(p) ) {
		offstar( p->n_left, 0 );
		return;
	}

	order( p, INAREG );
	}
#endif

int
setbin( p ) register NODE *p; {

#if 0
	register int ro, rt;

	rt = p->n_right->n_type;
	ro = p->n_right->n_op;

	if( canaddr( p->n_left ) && !canaddr( p->n_right ) ) { /* address rhs */
		if( ro == UMUL ) {
			offstar( p->n_right->n_left, 0 );
			return(1);
		} else {
			order( p->n_right, INAREG|SOREG );
			return(1);
		}
	}
	if( !istnode( p->n_left) ) { /* try putting LHS into a reg */
/*		order( p->n_left, logop(p->n_op)?(INAREG|INBREG|INTAREG|INTBREG|SOREG):(INTAREG|INTBREG|SOREG) );*/
		order( p->n_left, INAREG|INTAREG|INBREG|INTBREG|SOREG );
		return(1);
		}
	else if( ro == UNARY MUL && rt != CHAR && rt != UCHAR ){
		offstar( p->n_right->n_left );
		return(1);
		}
	else if( rt == CHAR || rt == UCHAR || rt == SHORT || rt == USHORT || (ro != REG &&
			ro != NAME && ro != OREG && ro != ICON ) ){
		order( p->n_right, INAREG|INBREG );
		return(1);
		}
/*
	else if( logop(p->n_op) && rt==USHORT ){  / * must get rhs into register */
/*
		order( p->n_right, INAREG );
		return( 1 );
		}
 */
#endif
	return(0);
	}

#if 0
int
setstr( p ) register NODE *p; { /* structure assignment */
	if( p->right->op != REG ){
		order( p->right, INTAREG );
		return(1);
		}
	p = p->left;
	if( p->op != NAME && p->op != OREG ){
		if( p->op != UNARY MUL ) cerror( "bad setstr" );
		order( p->left, INTAREG );
		return( 1 );
		}
	return( 0 );
	}
#endif

int
setasg(NODE *p, int s)
{

#if 0
	/* setup for assignment operator */

	if( !canaddr(p->n_right) ) {
		if( p->n_right->n_op == UNARY MUL )
			offstar(p->n_right->n_left);
		else
			order( p->n_right, INAREG|INBREG|SOREG );
		return(1);
		}
	if( p->n_left->n_op == UMUL ) {
		offstar( p->n_left->n_left );
		return(1);
		}
	if( p->left->op == FLD && p->left->left->op == UNARY MUL ){
		offstar( p->left->left->left );
		return(1);
		}
/* FLD patch */
	if( p->left->op == FLD && !(p->right->type==INT || p->right->type==UNSIGNED)) {
		order( p->right, INAREG);
		return(1);
		}
/* end of FLD patch */
#endif
	return(0);
	}

/* setup for unary operator */
int
setuni(NODE *p, int cookie)
{
	return 0;
}


#if 0
int
setasop( p ) register NODE *p; {
	/* setup for =ops */
	register rt, ro;

	rt = p->right->type;
	ro = p->right->op;

	if( ro == UNARY MUL && rt != CHAR ){
		offstar( p->right->left );
		return(1);
		}
	if( ( rt == CHAR || rt == SHORT || rt == UCHAR || rt == USHORT ||
			( ro != REG && ro != ICON && ro != NAME && ro != OREG ) ) ){
		order( p->right, INAREG|INBREG );
		return(1);
		}
/*
	if( (p->op == ASG LS || p->op == ASG RS) && ro != ICON && ro != REG ){
		order( p->right, INAREG );
		return(1);
		}
 */


	p = p->left;
	if( p->op == FLD ) p = p->left;

	switch( p->op ){

	case REG:
	case ICON:
	case NAME:
	case OREG:
		return(0);

	case UNARY MUL:
		if( p->left->op==OREG )
			return(0);
		else
			offstar( p->left );
		return(1);

		}
	cerror( "illegal setasop" );
	}
#endif

void
deflab(int l)
{
	printf(LABFMT ":\n", l);
}

#if 0
genargs( p, ptemp ) register NODE *p, *ptemp; {
	register NODE *pasg;
	register align;
	register size;
	register TWORD type;

	/* generate code for the arguments */

	/*  first, do the arguments on the right */
	while( p->op == CM ){
		genargs( p->right, ptemp );
		p->op = FREE;
		p = p->left;
		}

	if( p->op == STARG ){ /* structure valued argument */

		size = p->stsize;
		align = p->stalign;

 /*		ptemp->lval = (ptemp->lval/align)*align;  / * SETOFF for negative numbers */
 		ptemp->lval = 0;	/* all moves to (sp) */

		p->op = STASG;
		p->right = p->left;
		p->left = tcopy( ptemp );

		/* the following line is done only with the knowledge
		that it will be undone by the STASG node, with the
		offset (lval) field retained */

		if( p->right->op == OREG ) p->right->op = REG;  /* only for temporaries */

 		order( p, FORARG );
		ptemp->lval += size;
		return;
		}

	/* ordinary case */

	order( p, FORARG );
	}

argsize( p ) register NODE *p; {
	register t;
	t = 0;
	if( p->op == CM ){
		t = argsize( p->left );
		p = p->right;
		}
	if( p->type == DOUBLE || p->type == FLOAT ){
		SETOFF( t, 4 );
		return( t+8 );
		}
	else if( p->op == STARG ){
 		SETOFF( t, 4 );  /* alignment */
 		return( t + ((p->stsize+3)/4)*4 );  /* size */
		}
	else {
		SETOFF( t, 4 );
		return( t+4 );
		}
	}
#endif

/*
 * Set evaluation order of a binary node if it differs from default.
 */
int
setorder(NODE *p)
{
	return 0; /* nothing differs on vax */
}

/*
 * Do the actual conversion of offstar-found OREGs into real OREGs.
 */
void
myormake(NODE *q)
{
	NODE *p = q->n_left, *b, *i, *c, *x;
	CONSZ d;
	int mode, sz = tlen(q);

	if (x2debug)
		printf("myormake(%p)\n", q);

	if (deferred(p, sz)) {
		q->n_op = OREG;
		setlval(q, getlval(p));
		q->n_name = p->n_name;
		q->n_rval = R2PACK((p->n_op == NAME ? NOBASE : regno(p)), NOINDEX, 1);
		tfree(p);
		return;
	}
	if (q->n_qual == INCMARK && (mode = inctree(p, sz, &x)) != 0) {
		q->n_op = OREG;
		setlval(q, 0);
		q->n_name = "";
		q->n_rval = R2PACK(regno(x), NOINDEX, (mode == 1 ? RINCR : RDECR));
		tfree(p);
		return;
	}
	if (indexform(p, sz, &b, &i, &c, &d) && i->n_op == REG &&
	    (b == NULL || b->n_op == REG)) {
		q->n_op = OREG;
		setlval(q, d);
		q->n_name = c != NULL ? c->n_name : "";
		q->n_rval = R2PACK((b != NULL ? regno(b) : NOBASE), regno(i), 0);
		tfree(p);
	}
}
/*
 * Set registers "live" at function calls (like arguments in registers).
 * This is for liveness analysis of registers.
 */
int *
livecall(NODE *p)
{
	static int r[1] = { -1 }; /* Terminate with -1 */

	return &r[0];
}

/*
 * Signal whether the instruction is acceptable for this target.
 */
int
acceptable(struct optab *op)
{
	return 1;
}
