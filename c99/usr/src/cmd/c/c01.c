#
/*
 * C compiler
 *
 *
 */

#include "c0.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

/*
 * Called from tree, this routine takes the top 1, 2, or 3
 * operands on the expression stack, makes a new node with
 * the operator op, and puts it on the stack.
 * Essentially all the work is in inserting
 * appropriate conversions.
 */
struct node * block(int16_t op, int16_t t, int16_t *subs, struct str *str, struct node *p1, struct node *p2);
struct node * cblock(int16_t v);
struct node * chkfun(struct node *ap);
int16_t chklval(struct node *ap);
void chkw(struct node *p, int16_t okt);
struct node * convert(struct node *p, int16_t t, int16_t cvn, int16_t len);
struct node * disarray(struct node *ap);
int16_t error(int16_t s, int16_t p1, int16_t p2, int16_t p3, int16_t p4, int16_t p5, int16_t p6);
int16_t fold(int16_t op, struct node *ap1, struct node *ap2);
char * gblock(int16_t n);
int16_t lintyp(int16_t t);
int16_t setype(struct node *ap, int16_t at, struct node *anewp);

void build(int16_t op)
{
	register int16_t t1;
	int16_t t2, t;
	register struct node *p1, *p2;
	struct node *p3;
	int16_t dope, leftc, cvn, pcvn;

	/*
	 * a[i] => *(a+i)
	 */
	if (op==LBRACK) {
		build(PLUS);
		op = STAR;
	}
	dope = opdope[op];
	if ((dope&BINARY)!=0) {
		p2 = chkfun(disarray(*--cp));
		t2 = p2->u.tnode.type;
	}
	p1 = *--cp;
	/*
	 * sizeof gets turned into a number here.
	 */
	if (op==SIZEOF) {
		struct node *tn = cblock(length(p1));
		tn->u.tnode.type = UNSIGN;
		*cp++ = tn;
		return;
	}
	if (op!=AMPER) {
		p1 = disarray(p1);
		if (op!=CALL)
			p1 = chkfun(p1);
	}
	t1 = p1->u.tnode.type;
	pcvn = 0;
	t = INT;
	switch (op) {

	case CAST:
		if ((t1&XTYPE)==FUNC || (t1&XTYPE)==ARRAY)
			error("Disallowed conversion");
		if (t1==UNSIGN && t2==CHAR) {
			t2 = INT;
			p2 = block(AND,INT,NULL,NULL,p2,cblock(0377));
		}
		break;

	/* end of expression */
	case 0:
		*cp++ = p1;
		return;

	/* no-conversion operators */
	case QUEST:
		if (p2->u.tnode.op!=COLON)
			error("Illegal conditional");
		else
			if (fold(QUEST, p1, p2))
				return;

	case SEQNC:
		t = t2;

	case COMMA:
	case LOGAND:
	case LOGOR:
		*cp++ = block(op, t, NULL, NULL, p1, p2);
		return;

	case EXCLA:
		t1 = INT;
		break;

	case CALL:
		if ((t1&XTYPE) != FUNC)
			error("Call of non-function");
		*cp++ = block(CALL,decref(t1),p1->u.tnode.subsp,p1->u.tnode.strp,p1,p2);
		return;

	case STAR:
		if ((t1&XTYPE) == FUNC)
			error("Illegal indirection");
		*cp++ = block(STAR, decref(t1), p1->u.tnode.subsp, p1->u.tnode.strp, p1);
		return;

	case AMPER:
		if (p1->u.tnode.op==NAME || p1->u.tnode.op==STAR) {
			*cp++ = block(op,incref(t1),p1->u.tnode.subsp,p1->u.tnode.strp,p1);
			return;
		}
		error("Illegal lvalue");
		break;

	/*
	 * a.b goes to (&a)->b
	 */
	case DOT:
		if (p1->u.tnode.op==CALL && t1==STRUCT) {
			t1 = incref(t1);
			setype(p1, t1, p1);
		} else {
			*cp++ = p1;
			build(AMPER);
			p1 = *--cp;
		}

	/*
	 * In a->b, a is given the type ptr-to-structure element;
	 * then the offset is added in without conversion;
	 * then * is tacked on to access the member.
	 */
	case ARROW:
		if (p2->u.tnode.op!=NAME || p2->u.tnode.tr1->u.hshtab.hclass!=MOS) {
			error("Illegal structure ref");
			*cp++ = p1;
			return;
		}
		if (t2==INT && p2->u.tnode.tr1->u.hshtab.hflag&FFIELD)
			t2 = UNSIGN;
		t = incref(t2);
		chkw(p1, -1);
		setype(p1, t, p2);
		*cp++ = block(PLUS,t,p2->u.tnode.subsp,p2->u.tnode.strp,p1,cblock(p2->u.tnode.tr1->u.hshtab.hoffset));
		build(STAR);
		if (p2->u.tnode.tr1->u.hshtab.hflag&FFIELD)
			*cp++ = block(FSEL,UNSIGN,NULL,NULL,*--cp,p2->u.tnode.tr1->u.hshtab.hstrp);
		return;
	}
	if ((dope&LVALUE)!=0)
		chklval(p1);
	if ((dope&LWORD)!=0)
		chkw(p1, LONG);
	if ((dope&RWORD)!=0)
		chkw(p2, LONG);
	if ((dope&BINARY)==0) {
		if (op==ITOF)
			t1 = DOUBLE;
		else if (op==FTOI)
			t1 = INT;
		if (!fold(op, p1, 0))
			*cp++ = block(op,t1,p1->u.tnode.subsp,p1->u.tnode.strp,p1);
		return;
	}
	cvn = 0;
	if (t1==STRUCT || t2==STRUCT) {
		if (t1!=t2 || p1->u.tnode.strp != p2->u.tnode.strp)
			error("Incompatible structures");
		cvn = 0;
	} else
		cvn = cvtab[lintyp(t1)][lintyp(t2)];
	leftc = (cvn>>4)&017;
	cvn &= 017;
	t = leftc? t2:t1;
	if ((t==INT||t==CHAR) && (t1==UNSIGN||t2==UNSIGN))
		t = UNSIGN;
	if (dope&ASSGOP || op==CAST) {
		t = t1;
		if (op==ASSIGN || op==CAST) {
			if (cvn==ITP||cvn==PTI)
				cvn = leftc = 0;
			else if (cvn==LTP) {
				if (leftc==0)
					cvn = LTI;
				else {
					cvn = ITL;
					leftc = 0;
				}
			}
		}
		if (leftc)
			cvn = leftc;
		leftc = 0;
	} else if (op==COLON || op==MAX || op==MIN) {
		if (t1>=PTR && t1==t2)
			cvn = 0;
		if (op!=COLON && (t1>=PTR || t2>=PTR))
			op += MAXP-MAX;
	} else if (dope&RELAT) {
		if (op>=LESSEQ && (t1>=PTR||t2>=PTR||(t1==UNSIGN||t2==UNSIGN)
		 && (t==INT||t==CHAR||t==UNSIGN)))
			op += LESSEQP-LESSEQ;
		if (cvn==ITP || cvn==PTI)
			cvn = 0;
	}
	if (cvn==PTI) {
		cvn = 0;
		if (op==MINUS) {
			t = INT;
			pcvn++;
		} else {
			if (t1!=t2 || t1!=(PTR+CHAR))
				cvn = XX;
		}
	}
	if (cvn) {
		t1 = plength(p1);
		t2 = plength(p2);
		if (cvn==XX || (cvn==PTI&&t1!=t2))
			error("Illegal conversion");
		else if (leftc)
			p1 = convert(p1, t, cvn, t2);
		else
			p2 = convert(p2, t, cvn, t1);
	}
	if (dope&RELAT)
		t = INT;
	if (t==FLOAT)
		t = DOUBLE;
	if (t==CHAR)
		t = INT;
	if (op==CAST) {
		if (t!=DOUBLE && (t!=INT || p2->u.tnode.type!=CHAR)) {
			p2->u.tnode.type = t;
			p2->u.tnode.subsp = p1->u.tnode.subsp;
			p2->u.tnode.strp = p1->u.tnode.strp;
		}
		if (t==INT && p1->u.tnode.type==CHAR)
			p2 = block(ITOC, INT, NULL, NULL, p2);
		*cp++ = p2;
		return;
	}
	if (fold(op, p1, p2)==0) {
		p3 = leftc?p2:p1;
		*cp++ = block(op, t, p3->u.tnode.subsp, p3->u.tnode.strp, p1, p2);
	}
	if (pcvn && t1!=(PTR+CHAR)) {
		p1 = *--cp;
		*cp++ = convert(p1, 0, PTI, plength(p1->u.tnode.tr1));
	}
}

/*
 * Generate the appropriate conversion operator.
 */
struct node * convert(struct node *p, int16_t t, int16_t cvn, int16_t len)
{
	register int16_t op;

	op = cvntab[cvn];
	if (opdope[op]&BINARY) {
		if (len==0)
			error("Illegal conversion");
		return(block(op, t, NULL, NULL, p, cblock(len)));
	}
	return(block(op, t, NULL, NULL, p));
}

/*
 * Traverse an expression tree, adjust things
 * so the types of things in it are consistent
 * with the view that its top node has
 * type at.
 * Used with structure references.
 */
int16_t setype(struct node *ap, int16_t at, struct node *anewp)
{
	register struct node *p, *newp;
	register int16_t t;

	p = ap;
	t = at;
	newp = anewp;
	for (;; p = p->u.tnode.tr1) {
		p->u.tnode.subsp = newp->u.tnode.subsp;
		p->u.tnode.strp = newp->u.tnode.strp;
		p->u.tnode.type = t;
		if (p->u.tnode.op==AMPER)
			t = decref(t);
		else if (p->u.tnode.op==STAR)
			t = incref(t);
		else if (p->u.tnode.op!=PLUS)
			break;
	}
}

/*
 * A mention of a function name is turned into
 * a pointer to that function.
 */
struct node * chkfun(struct node *ap)
{
	register struct node *p;
	register int16_t t;

	p = ap;
	if (((t = p->u.tnode.type)&XTYPE)==FUNC && p->u.tnode.op!=ETYPE)
		return(block(AMPER,incref(t),p->u.tnode.subsp,p->u.tnode.strp,p));
	return(p);
}

/*
 * A mention of an array is turned into
 * a pointer to the base of the array.
 */
struct node * disarray(struct node *ap)
{
	register int16_t t;
	register struct node *p;

	p = ap;
	/* check array & not MOS and not typer */
	if (((t = p->u.tnode.type)&XTYPE)!=ARRAY || p->u.tnode.op==NAME&&p->u.tnode.tr1->u.hshtab.hclass==MOS
	 || p->u.tnode.op==ETYPE)
		return(p);
	p->u.tnode.subsp++;
	*cp++ = p;
	setype(p, decref(t), p);
	build(AMPER);
	return(*--cp);
}

/*
 * make sure that p is a ptr to a node
 * with type int or char or 'okt.'
 * okt might be nonexistent or 'long'
 * (e.g. for <<).
 */
void chkw(struct node *p, int16_t okt)
{
	register int16_t t;

	if ((t=p->u.tnode.type)!=INT && t<PTR && t!=CHAR && t!=UNSIGN && t!=okt)
		error("Illegal type of operand");
	return;
}

/*
 *'linearize' a type for looking up in the
 * conversion table
 */
int16_t lintyp(int16_t t)
{
	switch(t) {

	case INT:
	case CHAR:
	case UNSIGN:
		return(0);

	case FLOAT:
	case DOUBLE:
		return(1);

	case LONG:
		return(2);

	default:
		return(3);
	}
}

/*
 * Report an error.
 */
int16_t error(int16_t s, int16_t p1, int16_t p2, int16_t p3, int16_t p4, int16_t p5, int16_t p6)
{
	nerror++;
	if (filename[0])
		fprintf(stderr, "%s:", filename);
	fprintf(stderr, "%d: ", line);
	fprintf(stderr, s, p1, p2, p3, p4, p5, p6);
	fprintf(stderr, "\n");
}

/*
 * Generate a node in an expression tree,
 * setting the operator, type, dimen/struct table ptrs,
 * and the operands.
 */
struct node * block(int16_t op, int16_t t, int16_t *subs, struct str *str, struct node *p1, struct node *p2)
{
	register struct node *p;

	p = gblock(sizeof(*p));
	p->u.tnode.op = op;
	p->u.tnode.type = t;
	p->u.tnode.subsp = subs;
	p->u.tnode.strp = str;
	p->u.tnode.tr1 = p1;
	if (opdope[op]&BINARY)
		p->u.tnode.tr2 = p2;
	else
		p->u.tnode.tr2 = NULL;
	return(p);
}

struct node * nblock(struct node *ads)
{
	register struct node *ds;

	ds = ads;
	return(block(NAME, ds->u.hshtab.htype, ds->u.hshtab.hsubsp, ds->u.hshtab.hstrp, ds));
}

/*
 * Generate a block for a constant
 */
struct node * cblock(int16_t v)
{
	register struct node *p;

	p = gblock(sizeof(*p));
	p->u.tnode.op = CON;
	p->u.tnode.type = INT;
	p->u.tnode.subsp = NULL;
	p->u.tnode.strp = NULL;
	p->u.cnode.value = v;
	return(p);
}

/*
 * A block for a float or long constant
 */
struct node * fblock(int16_t t, char *string)
{
	register struct node *p;

	p = gblock(sizeof(*p));
	p->u.tnode.op = FCON;
	p->u.tnode.type = t;
	p->u.tnode.subsp = NULL;
	p->u.tnode.strp = NULL;
	p->u.fnode.cstr = string;
	return(p);
}

/*
 * Assign a block for use in the
 * expression tree.
 */
char * gblock(int16_t n)
{
	register int16_t *p;

	p = curbase;
	if ((curbase += n) >= coremax) {
		if (sbrk(1024) == -1) {
			error("Out of space");
			exit(1);
		}
		coremax += 1024;
	}
	return(p);
}

/*
 * Check that a tree can be used as an lvalue.
 */
int16_t chklval(struct node *ap)
{
	register struct node *p;

	p = ap;
	if (p->u.tnode.op==FSEL)
		p = p->u.tnode.tr1;
	if (p->u.tnode.op!=NAME && p->u.tnode.op!=STAR)
		error("Lvalue required");
}

/*
 * reduce some forms of `constant op constant'
 * to a constant.  More of this is done in the next pass
 * but this is used to allow constant expressions
 * to be used in switches and array bounds.
 */
int16_t fold(int16_t op, struct node *ap1, struct node *ap2)
{
	register struct node *p1;
	register int16_t v1, v2;
	int16_t unsignf;

	p1 = ap1;
	if (p1->u.tnode.op!=CON)
		return(0);
	unsignf = p1->u.tnode.type==UNSIGN;
	if (op==QUEST) {
		if (ap2->u.tnode.tr1->u.tnode.op==CON && ap2->u.tnode.tr2->u.tnode.op==CON) {
			p1->u.cnode.value = p1->u.cnode.value? ap2->u.tnode.tr1->u.cnode.value: ap2->u.tnode.tr2->u.cnode.value;
			*cp++ = p1;
			return(1);
		}
		return(0);
	}
	if (ap2) {
		if (ap2->u.tnode.op!=CON)
			return(0);
		v2 = ap2->u.cnode.value;
		unsignf |= ap2->u.tnode.type==UNSIGN;
	}
	v1 = p1->u.cnode.value;
	switch (op) {

	case PLUS:
		v1 += v2;
		break;

	case MINUS:
		v1 -= v2;
		break;

	case TIMES:
		v1 *= v2;
		break;

	case DIVIDE:
		if (v2==0)
			goto divchk;
		if (unsignf) {
			v1 = (unsigned)v1 / v2;
			break;
		}
		v1 /= v2;
		break;

	case MOD:
		if (v2==0)
			goto divchk;
		if (unsignf) {
			v1 = (unsigned)v1 % v2;
			break;
		}
		v1 %= v2;
		break;

	case AND:
		v1 &= v2;
		break;

	case OR:
		v1 |= v2;
		break;

	case EXOR:
		v1 ^= v2;
		break;

	case NEG:
		v1 = - v1;
		break;

	case COMPL:
		v1 = ~ v1;
		break;

	case LSHIFT:
		v1 <<= v2;
		break;

	case RSHIFT:
		if (unsignf) {
			v1 = (unsigned)v1 >> v2;
			break;
		}
		v1 >>= v2;
		break;

	case EQUAL:
		v1 = v1==v2;
		break;

	case NEQUAL:
		v1 = v1!=v2;
		break;

	case LESS:
		v1 = v1<v2;
		break;

	case GREAT:
		v1 = v1>v2;
		break;

	case LESSEQ:
		v1 = v1<=v2;
		break;

	case GREATEQ:
		v1 = v1>=v2;
		break;

	divchk:
		error("Divide check");
	default:
		return(0);
	}
	p1->u.cnode.value = v1;
	*cp++ = p1;
	return(1);
}

/*
 * Compile an expression expected to have constant value,
 * for example an array bound or a case value.
 */
int16_t conexp(void)
{
	register struct node *t;

	initflg++;
	if (t = tree())
		if (t->u.tnode.op != CON)
			error("Constant required");
	initflg--;
	curbase = funcbase;
	return(t->u.cnode.value);
}
