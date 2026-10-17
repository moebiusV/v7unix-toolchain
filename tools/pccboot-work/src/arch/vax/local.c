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
 *	This product includes software developed or owned by Caldera
 *	International, Inc.
 * Neither the name of Caldera International, Inc. nor the names of other
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.	IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OFLIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

# include "unicode.h"
# include "pass1.h"

#ifndef LANG_CXX
#define NODE P1ND
#undef NIL
#define	NIL NULL
#define	nfree p1nfree
#define	tfree p1tfree
#define	fwalk p1fwalk
#define sap sss
#undef  n_df
#define n_df pdf
#undef  n_ap
#define n_ap pss
#define n_type ptype
#endif

static void r1arg(NODE *p, NODE *q);

#define IALLOC(sz)      (isinlining ? permalloc(sz) : tmpalloc(sz))

/*
 * If n is the address of a static-storage symbol plus a constant, possibly
 * through pointer casts, return the symbol and the total offset.
 */
static struct symtab *
symconst(NODE *n, CONSZ *off)
{
	struct symtab *sp;
	CONSZ o;

	while (n->n_op == PCONV || n->n_op == SCONV)
		n = n->n_left;
	if (n->n_op == ICON && n->n_sp != NULL) {
		sp = n->n_sp;
		*off = glval(n);
	} else if (n->n_op == ADDROF && n->n_left->n_op == NAME &&
	    n->n_left->n_sp != NULL) {
		sp = n->n_left->n_sp;
		*off = glval(n->n_left);
	} else if (n->n_op == PLUS && n->n_right->n_op == ICON &&
	    n->n_right->n_sp == NULL &&
	    (sp = symconst(n->n_left, &o)) != NULL) {
		*off = o + glval(n->n_right);
		return sp;
	} else
		return NULL;
	if (sp->sclass == EXTERN || sp->sclass == EXTDEF ||
	    sp->sclass == STATIC || sp->sclass == USTATIC)
		return sp;
	return NULL;
}

/*	this file contains code which is dependent on the target machine */

NODE *
clocal(NODE *p)
{

	/* this is called to do local transformations on
	   an expression tree preparitory to its being
	   written out in intermediate code.
	*/

	/* the major essential job is rewriting the
	   automatic variables and arguments in terms of
	   REG and OREG nodes */
	/* conversion ops which are not necessary are also clobbered here */
	/* in addition, any special features (such as rewriting
	   exclusive or) are easily handled here as well */

	register struct symtab *q;
	register NODE *r, *l;
	register int o;
	register int ml;
	CONSZ c;


#ifdef PCC_DEBUG
	if (xdebug) {
		printf("clocal(%p)\n", p);
		if (xdebug>1)
			fwalk(p, eprint, 0);
	}
#endif

	switch( o = p->n_op ){

	case NAME:
		if((q = p->n_sp) == 0 ) { /* already processed; ignore... */
			return(p);
			}
		switch( q->sclass ){

		case REGISTER:
		case AUTO:
		case PARAM:
			/* fake up a structure reference */
			r = block( REG, NIL, NIL, PTR+STRTY, 0, 0 );
			slval(r, 0);
			regno(r) = q->sclass==PARAM?ARGREG:FPREG;
			p = stref( block( STREF, r, p, 0, 0, 0 ) );
			break;
		}
		break;

	case UMUL:
		/* *(&x + k) is just the name x+k, a direct memory operand */
		if (xdmr && (q = symconst(p->n_left, &c)) != NULL &&
		    !ISFTN(DECREF(p->n_left->n_type))) {
			/* n_left and the offset share storage: free first */
			tfree(p->n_left);
			p->n_op = NAME;
			p->n_sp = q;
			slval(p, c);
		}
		break;

	case CBRANCH:
		/* compare a char or short with a constant without widening it */
		l = p->n_left;
		if (!xdmr || coptype(l->n_op) != BITYPE)
			break;
		/* two narrow values of the same kind compared with each other */
		if (l->n_left->n_op == SCONV && l->n_right->n_op == SCONV &&
		    (r = l->n_left->n_left)->n_type == l->n_right->n_left->n_type &&
		    (r->n_type == CHAR || r->n_type == UCHAR ||
		    r->n_type == SHORT || r->n_type == USHORT)) {
			NODE *cv = l->n_right;

			if (r->n_type == UCHAR || r->n_type == USHORT) {
				switch (l->n_op) {
				case LT: l->n_op = ULT; break;
				case LE: l->n_op = ULE; break;
				case GT: l->n_op = UGT; break;
				case GE: l->n_op = UGE; break;
				}
			}
			l->n_right = cv->n_left;
			nfree(cv);
			nfree(l->n_left);
			l->n_left = r;
			break;
		}
		if (l->n_left->n_op != SCONV || l->n_right->n_op != ICON ||
		    l->n_right->n_sp != NULL)
			break;
		if ((r = l->n_left->n_left)->n_type > INT)
			break;
		if (r->n_type != CHAR && r->n_type != UCHAR &&
		    r->n_type != SHORT && r->n_type != USHORT)
			break;
		c = glval(l->n_right);
		if (r->n_type == CHAR && (c < -128 || c > 127))
			break;
		if (r->n_type == UCHAR && (c < 0 || c > 255))
			break;
		if (r->n_type == SHORT && (c < -32768 || c > 32767))
			break;
		if (r->n_type == USHORT && (c < 0 || c > 65535))
			break;
		/* an unsigned operand must use the unsigned relations */
		if (r->n_type == UCHAR || r->n_type == USHORT) {
			switch (l->n_op) {
			case LT: l->n_op = ULT; break;
			case LE: l->n_op = ULE; break;
			case GT: l->n_op = UGT; break;
			case GE: l->n_op = UGE; break;
			}
		}
		nfree(l->n_left);
		l->n_left = r;
		l->n_right->n_type = l->n_left->n_type;
		break;

	case FORCE:
		p->n_op = ASSIGN;
		p->n_right = p->n_left;
		p->n_left = block(REG, NIL, NIL, p->n_type, 0, 0);
		p->n_left->n_rval = p->n_left->n_type == BOOL ? 
		    RETREG(CHAR) : RETREG(p->n_type);
		break;

	case SCONV:
		l = p->n_left;
		ml = p->n_type;
#if 0
		if (ml == INT && l->n_type == UNSIGNED) {
			p = nfree(p);
			break;
		}
#endif
		if (l->n_op == ICON) {
			if (l->n_sp == 0) {
				p->n_type = UNSIGNED;
				concast(l, p->n_type);
			} else if (ml != INT && ml != UNSIGNED)
				break;
			l->n_type = ml;
			l->n_ap = 0;
			p = nfree(p);
			break;
		}
		break;

	case STCALL:
		/* see if we have been here before */
		for (r = p->n_right; r->n_op == CM; r = r->n_left)
			;
		if (r->n_op == ASSIGN)
			break;

		/* FALLTHROUGH */
	case USTCALL:
		/* Allocate buffer on stack to bounce via */
		/* create fake symtab here */
		q = getsymtab("77fake", STEMP);
		q->stype = BTYPE(p->n_type);
		q->sdf = p->n_df;
		q->sap = p->n_ap;
		q->soffset = NOOFFSET;
		q->sclass = AUTO;
		oalloc(q, &autooff);
		r1arg(p, buildtree(ADDROF, nametree(q), 0));
		break;
	}
#ifdef PCC_DEBUG
	if (xdebug) {
		printf("clocal end(%p)\n", p);
		if (xdebug>1)
			fwalk(p, eprint, 0);
	}
#endif

	return(p);
}

/*
 * Add R1 with the dest address as arg to a struct return call.
 */
static void
r1arg(NODE *p, NODE *q)
{
	NODE *r;

	r = block(REG, NIL, NIL, PTR|VOID, 0, 0);
	regno(r) = R1;
	r = buildtree(ASSIGN, r, q);
	if (p->n_op == USTCALL) {
		p->n_op = STCALL;
		p->n_right = r;
	} else if (p->n_right->n_op != CM) {
		p->n_right = block(CM, r, p->n_right, INT, 0, 0);
	} else {
		for (q = p->n_right; q->n_left->n_op == CM; q = q->n_left)
			;
		q->n_left = block(CM, r, q->n_left, INT, 0, 0);
	}
}

void
myp2tree(NODE *p)
{
	struct symtab *sp;

	if ((cdope(p->n_op) & CALLFLG) && p->n_left->n_op == ADDROF &&
	    p->n_left->n_left->n_op == NAME) {
		NODE *q = p->n_left->n_left;
		nfree(p->n_left);
		p->n_left = q;
		q->n_op = ICON;
	}

	if (xdmr && p->n_op == PLUS && p->n_right->n_op == ICON &&
	    p->n_right->n_sp == NULL && p->n_left->n_op == ADDROF &&
	    p->n_left->n_left->n_op == NAME) {
		/* &sym + k is &(sym+k) */
		NODE *a = p->n_left, *k = p->n_right;
		CONSZ v = glval(k);

		slval(a->n_left, glval(a->n_left) + v);
		nfree(k);
		*p = *a;
		nfree(a);
		return;
	}
	if (p->n_op != FCON) 
		return;

	sp = IALLOC(sizeof(struct symtab));
	/* all of it: "sap" is a macro for "sss" here, so a store through it
	 * leaves the real attribute list (and the type description) garbage */
	memset(sp, 0, sizeof(struct symtab));
	sp->sclass = STATIC;
	sp->sap = 0;
	sp->slevel = 1; /* fake numeric label */
	sp->soffset = getlab();
	sp->sflags = 0;
	sp->stype = p->n_type;
	sp->squal = (CON >> TSHIFT);
	sp->sname = NULL;

	locctr(DATA, sp);
	defloc(sp);
	inval(0, tsize(sp->stype, sp->sdf, sp->sap), p);

	p->n_op = NAME;
	slval(p, 0);
	p->n_sp = sp;

}

/*
 * Can we take & of a NAME?
 */
int
andable(NODE *p)
{
	/* for now, delay name reference to table, for PIC code generation */
	/* functions are called by name, convert they in myp2tree */
	return 0;
}

/* is an automatic variable of type t OK for a register variable */
int
cisreg(TWORD t)
{
	return(1);	/* all are now */
}

/*
 * Allocate off bits on the stack.  p is a tree that when evaluated
 * is the multiply count for off, t is a storeable node where to write
 * the allocated address.
 */
void
spalloc(NODE *t, NODE *p, OFFSZ off)
{
	NODE *sp;

	p = buildtree(MUL, p, bcon(off/SZCHAR)); /* XXX word alignment? */

	/* sub the size from sp */
	sp = block(REG, NIL, NIL, p->n_type, 0, 0);
	slval(sp, 0);
	sp->n_rval = STKREG;
	ecomp(buildtree(MINUSEQ, sp, p));

	/* save the address of sp */
	sp = block(REG, NIL, NIL, PTR+INT, t->n_df, t->n_ap);
	slval(sp, 0);
	sp->n_rval = STKREG;
	t->n_type = sp->n_type;
	ecomp(buildtree(ASSIGN, t, sp)); /* Emit! */

}

char *
exname(char *p)
{
	/* make a name look like an external name in the local machine */
	/* vad is elf now */
	if (p == NULL)
		return "";
#if defined(VAX_AS_32V)
	{	/* a.out style: a _ in front of every C name */
		static char text[258];

		text[0] = '_';
		strncpy(text + 1, p, 256);
		text[257] = '\0';
		return text;
	}
#else
	return( p );
#endif
	}

/* map types which are not defined on the local machine */
TWORD
ctype(TWORD type) {
	switch( BTYPE(type) ){

	case LONG:
		MODTYPE(type,INT);
		break;

	case ULONG:
		MODTYPE(type,UNSIGNED);
		break;

	case LDOUBLE:	/* for now */
		MODTYPE(type,DOUBLE);
		}
	return( type );
	}

void
calldec(NODE *p, NODE *q) 
{
}

void
extdec(struct symtab *q)
{
}

/* make a common declaration for id, if reasonable */
void
defzero(struct symtab *sp)
{
	int off, al;
	char *name;

	name = getexname(sp);
	off = tsize(sp->stype, sp->sdf, sp->sap);
	SETOFF(off,SZCHAR);
	off /= SZCHAR;
	al = talign(sp->stype, sp->sap)/SZCHAR;

#if defined(VAX_AS_32V)
	/* .comm and .lcomm take a name and a size, nothing more */
	(void)al;
	printf("\t.%scomm ", sp->sclass == STATIC ? "l" : "");
	if (sp->slevel == 0)
		printf("%s,%d\n", name, off);
	else
		printf(LABFMT ",%d\n", sp->soffset, off);
#else
	if (sp->sclass == STATIC) {
		if (sp->slevel == 0)
			printf("\t.local %s\n", name);
		else
			printf("\t.local " LABFMT "\n", sp->soffset);
	}
	if (sp->slevel == 0) {
		printf("\t.comm %s,0%o,%d\n", name, off, al);
	} else
		printf("\t.comm " LABFMT ",0%o,%d\n", sp->soffset, off, al);
#endif
}

/*
 * print out a constant node, may be associated with a label.
 * Do not free the node after use.
 * off is bit offset from the beginning of the aggregate
 * fsz is the number of bits this is referring to
 * XXX - floating point constants may be wrong if cross-compiling.
 */
int
ninval(CONSZ off, int fsz, NODE *p)
{
	return 0;
}
/*
 * Give target the opportunity of handling pragmas.
 */
int
mypragma(char *str)
{
	return 0;
}

/*
 * Called when a identifier has been declared, to give target last word.
 */
void
fixdef(struct symtab *sp)
{
}

void
pass1_lastchance(struct interpass *ip)
{
}

#if defined(VAX_AS_32V)
/*
 * Print out a string of characters as .byte lists; the classic assembler
 * has neither .ascii nor .string.
 */
void
instring(struct symtab *sp)
{
	unsigned short sh[2];
	int val, cnt;
	TWORD t;
	char *s;

	locctr(STRNG, sp);
	defloc(sp);
	t = BTYPE(sp->stype);
	s = sp->sname;
	if (t == UNSIGNED || t == INT || t == ctype(UNSIGNED) || t == ctype(INT)) {
		/* wide characters, 32 bits each */
		while (*s)
			printf("\t.long\t0x%x\n", (unsigned)u82cp(&s));
		printf("\t.long\t0\n");
	} else if (t == USHORT || t == SHORT) {
		while (*s) {
			cp2u16(u82cp(&s), sh);
			if (sh[0]) printf("\t.word\t0x%x\n", sh[0]);
			if (sh[1]) printf("\t.word\t0x%x\n", sh[1]);
		}
		printf("\t.word\t0\n");
	} else if (t == CHAR || t == UCHAR) {
		for (cnt = 0; *s != 0; ) {
			if (cnt++ == 0)
				printf("\t.byte\t");
			if (*s == '\\')
				val = esccon(&s);
			else
				val = *s++;
			printf("0x%x", val & 0377);
			if (cnt > 15) {
				cnt = 0;
				printf("\n");
			} else
				printf(",");
		}
		printf("%s0\n", cnt ? "" : "\t.byte\t");
	} else
		cerror("instring");
}
#endif
