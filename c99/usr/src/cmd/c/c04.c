#
/*
 * C compiler
 *
 *
 */

#include "c0.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/*
 * Reduce the degree-of-reference by one.
 * e.g. turn "ptr-to-int" into "int".
 */
int16_t decref(int16_t at)
{
	register int16_t t;

	t = at;
	if ((t & ~TYPE) == 0) {
		error("Illegal indirection");
		return(t);
	}
	return((t>>TYLEN) & ~TYPE | t&TYPE);
}

/*
 * Increase the degree of reference by
 * one; e.g. turn "int" to "ptr-to-int".
 */
int16_t incref(int16_t t)
{
	return(((t&~TYPE)<<TYLEN) | (t&TYPE) | PTR);
}

/*
 * Make a tree that causes a branch to lbl
 * if the tree's value is non-zero together with the cond.
 */
int16_t length(struct node *acs);
int16_t nextchar(void);
void outcode(char *s, int16_t a, ...);
int16_t spnextchar(void);
void treeout(struct node *atp, int16_t isstruct);

int16_t cbranch(struct node *t, int16_t lbl, int16_t cond)
{
	treeout(t, 0);
	outcode("BNNN", CBRANCH, lbl, cond, line);
}

/*
 * Write out a tree.
 */
void rcexpr(struct node *atp)
{
	register struct node *tp;

	/*
	 * Special optimization
	 */
	if ((tp=atp)->u.tnode.op==INIT && tp->u.tnode.tr1->u.tnode.op==CON) {
		if (tp->u.tnode.type==CHAR) {
			outcode("B1N0", BDATA, tp->u.tnode.tr1->u.cnode.value);
			return;
		} else if (tp->u.tnode.type==INT || tp->u.tnode.type==UNSIGN) {
			outcode("BN", SINIT, tp->u.tnode.tr1->u.cnode.value);
			return;
		}
	}
	treeout(tp, 0);
	outcode("BN", EXPR, line);
}

void treeout(struct node *atp, int16_t isstruct)
{
	register struct node *tp;
	register struct node *hp;
	register int16_t nextisstruct;

	if ((tp = atp) == 0) {
		outcode("B", NULLOP);
		return;
	}
	nextisstruct = tp->u.tnode.type==STRUCT;
	switch(tp->u.tnode.op) {

	case NAME:
		hp = tp->u.tnode.tr1;
		if (hp->u.hshtab.hclass==TYPEDEF)
			error("Illegal use of type name");
		outcode("BNN", NAME, hp->u.hshtab.hclass==0?STATIC:hp->u.hshtab.hclass, tp->u.tnode.type);
		if (hp->u.hshtab.hclass==EXTERN)
			outcode("S", hp->u.hshtab.name);
		else
			outcode("N", hp->u.hshtab.hoffset);
		break;

	case LCON:
		outcode("BNNN", tp->u.tnode.op, tp->u.tnode.type, tp->u.lnode.lvalue);
		break;

	case CON:
		outcode("BNN", tp->u.tnode.op, tp->u.tnode.type, tp->u.cnode.value);
		break;

	case FCON:
		outcode("BNF", tp->u.tnode.op, tp->u.tnode.type, tp->u.fnode.cstr);
		break;

	case STRING:
		outcode("BNNN", NAME, STATIC, tp->u.tnode.type, tp->u.tnode.tr1);
		break;

	case FSEL:
		treeout(tp->u.tnode.tr1, nextisstruct);
		outcode("BNNN",tp->u.tnode.op,tp->u.tnode.type,((struct field *)tp->u.tnode.tr2)->bitoffs,((struct field *)tp->u.tnode.tr2)->flen);
		break;

	case ETYPE:
		error("Illegal use of type");
		break;

	case AMPER:
		treeout(tp->u.tnode.tr1, 1);
		outcode("BN", tp->u.tnode.op, tp->u.tnode.type);
		break;


	case CALL:
		treeout(tp->u.tnode.tr1, 1);
		treeout(tp->u.tnode.tr2, 0);
		outcode("BN", CALL, tp->u.tnode.type);
		break;

	default:
		treeout(tp->u.tnode.tr1, nextisstruct);
		if (opdope[tp->u.tnode.op]&BINARY)
			treeout(tp->u.tnode.tr2, nextisstruct);
		outcode("BN", tp->u.tnode.op, tp->u.tnode.type);
		break;
	}
	if (nextisstruct && isstruct==0)
		outcode("BNN", STRASG, STRUCT, tp->u.tnode.strp->ssize);
}

/*
 * Generate a branch
 */
int16_t branch(int16_t lab)
{
	outcode("BN", BRANCH, lab);
}

/*
 * Generate a label
 */
int16_t label(int16_t l)
{
	outcode("BN", LABEL, l);
}

/*
 * ap is a tree node whose type
 * is some kind of pointer; return the size of the object
 * to which the pointer points.
 */
int16_t plength(struct tname *ap)
{
	register int16_t t, l;
	register struct node *p;

	p = ap;
	if (p==0 || ((t=p->u.tnode.type)&~TYPE) == 0)		/* not a reference */
		return(1);
	p->u.tnode.type = decref(t);
	l = length(p);
	p->u.tnode.type = t;
	return(l);
}

/*
 * return the number of bytes in the object
 * whose tree node is acs.
 */
int16_t length(struct node *acs)
{
	register int16_t t, elsz;
	int32_t n;
	register struct node *cs;
	int16_t nd;

	cs = acs;
	t = cs->u.tnode.type;
	n = 1;
	nd = 0;
	while ((t&XTYPE) == ARRAY) {
		t = decref(t);
		n *= cs->u.tnode.subsp[nd++];
	}
	if ((t&~TYPE)==FUNC)
		return(0);
	if (t>=PTR)
		elsz = SZPTR;
	else switch(t&TYPE) {

	case INT:
	case UNSIGN:
		elsz = SZINT;
		break;

	case CHAR:
		elsz = 1;
		break;

	case FLOAT:
		elsz = SZFLOAT;
		break;

	case LONG:
		elsz = SZLONG;
		break;

	case DOUBLE:
		elsz = SZDOUB;
		break;

	case STRUCT:
		if ((elsz = cs->u.tnode.strp->ssize) == 0)
			error("Undefined structure");
		break;
	default:
		error("Compiler error (length)");
		return(0);
	}
	n *= elsz;
	if (n >= (unsigned)50000) {
		error("Warning: very large data structure");
		nerror--;
	}
	return(n);
}

/*
 * The number of bytes in an object, rounded up to a word.
 */
int16_t rlength(struct node *cs)
{
	return((length(cs)+ALIGN) & ~ALIGN);
}

/*
 * After an "if (...) goto", look to see if the transfer
 * is to a simple label.
 */
int16_t simplegoto(void)
{
	register struct node *csp;

	if ((peeksym=symbol())==NAME && nextchar()==';') {
		csp = csym;
		if (csp->u.hshtab.hblklev == 0)
			pushdecl(csp);
		if (csp->u.hshtab.hclass==0 && csp->u.hshtab.htype==0) {
			csp->u.hshtab.htype = ARRAY;
			csp->u.hshtab.hflag |= FLABL;
			if (csp->u.hshtab.hoffset==0)
				csp->u.hshtab.hoffset = isn++;
		}
		if ((csp->u.hshtab.hclass==0||csp->u.hshtab.hclass==STATIC)
		 &&  csp->u.hshtab.htype==ARRAY) {
			peeksym = -1;
			return(csp->u.hshtab.hoffset);
		}
	}
	return(0);
}

/*
 * Return the next non-white-space character
 */
int16_t nextchar(void)
{
	while (spnextchar()==' ')
		peekc = 0;
	return(peekc);
}

/*
 * Return the next character, translating all white space
 * to blank and handling line-ends.
 */
int16_t spnextchar(void)
{
	register int16_t c;

	if ((c = peekc)==0)
		c = getchar();
	if (c=='\t' || c=='\014')	/* FF */
		c = ' ';
	else if (c=='\n') {
		c = ' ';
		if (inhdr==0)
			line++;
		inhdr = 0;
	} else if (c=='\001') {	/* SOH, insert marker */
		inhdr++;
		c = ' ';
	}
	peekc = c;
	return(c);
}

/*
 * is a break or continue legal?
 */
int16_t chconbrk(int16_t l)
{
	if (l==0)
		error("Break/continue error");
}

/*
 * The goto statement.
 */
int16_t dogoto(void)
{
	register struct node *np;

	*cp++ = tree();
	build(STAR);
	chkw(np = *--cp, -1);
	rcexpr(block(JUMP,0,NULL,NULL,np));
}

/*
 * The return statement, which has to convert
 * the returned object to the function's type.
 */
int16_t doret(void)
{
	register struct node *t;

	if (nextchar() != ';') {
		t = tree();
		*cp++ = &funcblk;
		*cp++ = t;
		build(ASSIGN);
		cp[-1] = cp[-1]->u.tnode.tr2;
		if (funcblk.u.tnode.type==CHAR)
			cp[-1] = block(ITOC, INT, NULL, NULL, cp[-1]);
		build(RFORCE);
		rcexpr(*--cp);
	}
	branch(retlab);
}

/*
 * Write a character on the error output.
 */
/*
 * Coded output:
 *   B: beginning of line; an operator
 *   N: a number
 *   S: a symbol (external)
 *   1: number 1
 *   0: number 0
 */
void outcode(char *s, int16_t a, ...)
{
	register int16_t *ap;
	register FILE *bufp;
	int16_t n;
	register char *np;

	bufp = stdout;
	if (strflg)
		bufp = sbufp;
	ap = &a;
	for (;;) switch(*s++) {
	case 'B':
		putc(*ap++, bufp);
		putc(0376, bufp);
		continue;

	case 'N':
		putc(*ap, bufp);
		putc(*ap++>>8, bufp);
		continue;

	case 'F':
		n = 1000;
		np = *ap++;
		goto str;

	case 'S':
		n = NCPS;
		np = *ap++;
		if (*np)
			putc('_', bufp);
	str:
		while (n-- && *np) {
			putc(*np++&0177, bufp);
		}
		putc(0, bufp);
		continue;

	case '1':
		putc(1, bufp);
		putc(0, bufp);
		continue;

	case '0':
		putc(0, bufp);
		putc(0, bufp);
		continue;

	case '\0':
		if (ferror(bufp)) {
			error("Write error on temp");
			exit(1);
		}
		return;

	default:
		error("Botch in outcode");
	}
}
