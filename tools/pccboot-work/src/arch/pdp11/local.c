/*	$Id$	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "unicode.h"
#include "pass1.h"

#ifndef LANG_CXX
#define	NODE P1ND
#undef NIL
#define NIL NULL
#define fwalk p1fwalk
#define nfree p1nfree
#define	n_type	ptype
#undef n_df
#define	n_df pdf
#undef n_ap
#define	n_ap pss
#define	sap sss
#endif

/*
 * If n is the address of a static-storage symbol plus a constant, possibly
 * through pointer casts, return the symbol's ICON node and the total offset.
 */
static NODE *
symconst(NODE *n, CONSZ *off)
{
	NODE *q;
	CONSZ o;

	while (n->n_op == PCONV || n->n_op == SCONV)
		n = n->n_left;
	if (n->n_op == ICON && n->n_sp != NULL &&
	    (n->n_sp->sclass == EXTERN || n->n_sp->sclass == EXTDEF ||
	    n->n_sp->sclass == STATIC || n->n_sp->sclass == USTATIC)) {
		*off = glval(n);
		return n;
	}
	if (n->n_op == PLUS && n->n_right->n_op == ICON &&
	    n->n_right->n_sp == NULL && (q = symconst(n->n_left, &o)) != NULL) {
		*off = o + glval(n->n_right);
		return q;
	}
	return NULL;
}

/*
 * A double whose last three words are zero (1.0, 10.0, 0.5, ...) is the
 * first word alone: the FP11 takes it as an immediate operand, as dmr-cc
 * does ("mulf $41040,r0"), with no stored constant.  Returns 1 and the word
 * if q is such a constant.
 */
static int
fimm(NODE *q, unsigned int *w)
{
	if (q->n_op != FCON || (q->n_type != DOUBLE && q->n_type != LDOUBLE))
		return 0;
	if ((q->n_scon->fp[0] >> 16) != 0 || q->n_scon->fp[1] != 0)
		return 0;
	*w = q->n_scon->fp[0] & 0xffff;
	return 1;
}

static void
makeimm(NODE *q, unsigned int w)
{
	q->n_op = ICON;
	q->n_sp = NULL;
	slval(q, w);
}

/*	this file contains code which is dependent on the target machine */

/* clocal() is called to do local transformations on
 * an expression tree preparitory to its being
 * written out in intermediate code.
 *
 * the major essential job is rewriting the
 * automatic variables and arguments in terms of
 * REG and OREG nodes
 * conversion ops which are not necessary are also clobbered here
 * in addition, any special features (such as rewriting
 * exclusive or) are easily handled here as well
 */
extern P1ND *llulshr(P1ND *, P1ND *);

NODE *
clocal(NODE *p)
{

	CONSZ c;
	register struct symtab *q;
	register NODE *r, *l, *cv;
	register int o;
	TWORD m;

#ifdef PCC_DEBUG
	if (xdebug) {
		printf("clocal: %p\n", p);
		fwalk(p, eprint, 0);
	}
#endif
	switch( o = p->n_op ){

	case RS:
		/* ashc is an arithmetic shift: an unsigned long >> n is a call (lowerll.c) */
		if (p->n_type == ULONG && !(p->n_right->n_op == ICON && p->n_right->n_sp == NULL &&
		    glval(p->n_right) == 0)) {
			r = p->n_right;
			l = p->n_left;
			p1nfree(p);
			return llulshr(l, r);
		}
		break;

	case NAME:
		if ((q = p->n_sp) == NULL)
			return p; /* Nothing to care about */

		switch (q->sclass) {

		case PARAM:
		case AUTO:
			/* fake up a structure reference */
			r = block(REG, NIL, NIL, PTR+STRTY, 0, 0);
			slval(r, 0);
			r->n_rval = FPREG;
			p = stref(block(STREF, r, p, 0, 0, 0));
			break;

		case STATIC:
			if (q->slevel == 0)
				break;
			slval(p, 0);
			break;

		case REGISTER:
			p->n_op = REG;
			slval(p, 0);
			p->n_rval = q->soffset;
			break;

		case EXTERN:
		case EXTDEF:
			break;
		}
		break;

	case UMUL:
		/* *(&x + k) is just the name x+k, a direct memory operand */
		if (xdmr && (r = symconst(p->n_left, &c)) != NULL &&
		    !ISFTN(DECREF(p->n_left->n_type)) &&
		    !(c != 0 && r->n_sp->sclass == STATIC && r->n_sp->slevel > 0)) {
			/*
			 * (a function's own statics are left alone with an
			 * offset: the NAME case above resets it to 0 when the
			 * node is looked at again)
			 */
			struct symtab *ssp = r->n_sp;

			/* n_left and the offset share storage: free first */
			p1tfree(p->n_left);
			p->n_op = NAME;
			p->n_sp = ssp;
			slval(p, c);
		}
		break;

	case CBRANCH:
		l = p->n_left;
		if (coptype(l->n_op) != BITYPE)
			break;
		/*
		 * Two bytes of the same kind compared with each other: cmpb,
		 * without widening either (strcmp's  *s1 == *s2++).
		 */
		if (xdmr && l->n_left->n_op == SCONV && l->n_right->n_op == SCONV &&
		    (r = l->n_left->n_left)->n_type == l->n_right->n_left->n_type &&
		    (r->n_type == CHAR || r->n_type == UCHAR)) {
			if (r->n_type == UCHAR) {
				switch (l->n_op) {
				case LT: l->n_op = ULT; break;
				case LE: l->n_op = ULE; break;
				case GT: l->n_op = UGT; break;
				case GE: l->n_op = UGE; break;
				}
			}
			l->n_right = (cv = l->n_right)->n_left;
			nfree(cv);
			nfree(l->n_left);
			l->n_left = r;
			break;
		}
		if (l->n_left->n_op != SCONV || l->n_right->n_op != ICON)
			break;
		if ((r = l->n_left->n_left)->n_type > INT)
			break;
		c = glval(l->n_right);
		if (c < MIN_INT || c > MAX_UNSIGNED)
			break;
		/*
		 * Comparing bytes without widening them is only right if the
		 * constant is a byte of the same kind; an unsigned byte must
		 * also use the unsigned relations.
		 */
		if (r->n_type == CHAR) {
			if (c < -128 || c > 127)
				break;
		} else if (r->n_type == UCHAR) {
			if (c < 0 || c > 255)
				break;
			switch (l->n_op) {
			case LT: l->n_op = ULT; break;
			case LE: l->n_op = ULE; break;
			case GT: l->n_op = UGT; break;
			case GE: l->n_op = UGE; break;
			}
		}
		/* compare with constant without casting */
		nfree(l->n_left);
		l->n_left = r;
		l->n_right->n_type = l->n_left->n_type;
		break;

	case SCONV:
		if (p->n_left->n_op == COMOP)
			break;	/* may propagate wrong type later */
		l = p->n_left;

		if (p->n_type == l->n_type) {
			p1nfree(p);
			return l;
		}

		if ((p->n_type & TMASK) == 0 && (l->n_type & TMASK) == 0 &&
		    tsize(p->n_type, p->n_df, p->n_ap) ==
		    tsize(l->n_type, l->n_df, l->n_ap)) {
			if (p->n_type != FLOAT && p->n_type != DOUBLE &&
			    l->n_type != FLOAT && l->n_type != DOUBLE &&
			    l->n_type != LDOUBLE && p->n_type != LDOUBLE) {
				if (l->n_op == NAME || l->n_op == UMUL ||
				    l->n_op == TEMP) {
					l->n_type = p->n_type;
					p1nfree(p);
					return l;
				}
			}
		}

		if (DEUNSIGN(p->n_type) == INT && DEUNSIGN(l->n_type) == INT &&
		    coptype(l->n_op) == BITYPE && l->n_op != COMOP &&
		    l->n_op != QUEST && l->n_op != ASSIGN && l->n_op != RS && l->n_op != DIV && l->n_op != MOD) {
			l->n_type = p->n_type;
			p1nfree(p);
			return l;
		}

		o = l->n_op;
		m = p->n_type;
		if (o == ICON) {
			/*
			 * Can only end up here if o is an address,
			 * and in that case the only compile-time conversion
			 * possible is to int.
			 */

			if (l->n_sp == 0) {
				p->n_type = UNSIGNED;
				concast(l, m);
			} else if (m != INT && m != UNSIGNED)
				break;
			l->n_type = m;
			l->n_ap = 0;
			p1nfree(p);
			return l;
		}

		if (p->n_type != UCHAR || l->n_type != CHAR ||
		    l->n_op != UMUL || (l->n_left->n_op != TEMP))
			break;
		l->n_type = UCHAR;
		MODTYPE(l->n_left->n_type, UCHAR);
		p = nfree(p);
		break;

#if 0
	case STASG: /* struct assignment, modify left */
		l = p->n_left;
		if (ISSOU(l->n_type))
			p->n_left = buildtree(ADDROF, l, NIL);
		break;
#endif

	case FORCE:
		/* put return value in return reg */
		p->n_op = ASSIGN;
		p->n_right = p->n_left;
		p->n_left = block(REG, NIL, NIL, p->n_type, 0, 0);
		p->n_left->n_rval = p->n_left->n_type == BOOL ? 
		    RETREG(CHAR) : RETREG(p->n_type);
		break;

	}
#ifdef PCC_DEBUG
	if (xdebug) {
		printf("clocal end: %p\n", p);
		fwalk(p, eprint, 0);
	}
#endif
	return(p);
}

int fzerolab = -1;	/* label of the 0.0 constant, if there is one */

#define IALLOC(sz)      (isinlining ? permalloc(sz) : tmpalloc(sz))
void
myp2tree(NODE *p)
{
	struct symtab *sp;
	/* floating constants seen so far in this file, to share their storage */
	static struct fconst {
		struct fconst *next;
		TWORD type;
		unsigned long w0, w1;
		struct symtab *sp;
	} *fcons;
	struct fconst *fc;
	unsigned long w0, w1;

	unsigned int w;

	/*
	 * Float immediates are chosen here, after all of pass 1's folding
	 * (which would treat them as integers), and before the constants
	 * below the node become stored data.
	 */
	switch (p->n_op) {
	case PLUS:
	case MINUS:
	case MUL:
	case DIV:
		if ((p->n_type == DOUBLE || p->n_type == LDOUBLE) &&
		    p->n_left->n_op != FCON && xdmr && fimm(p->n_right, &w))
			makeimm(p->n_right, w);
		break;
	case EQ:
	case NE:
	case LT:
	case LE:
	case GT:
	case GE:
		/*
		 * x < 1.0 is "cmpf $40200,x" with the sense reversed, because
		 * only the first operand of cmpf may be an immediate; a zero
		 * stays on the right for tstf.
		 */
		if (xdmr && p->n_left->n_op != FCON && fimm(p->n_right, &w)) {
			NODE *t;

			makeimm(p->n_right, w);
			if (w != 0) {
				t = p->n_left;
				p->n_left = p->n_right;
				p->n_right = t;
				switch (p->n_op) {
				case LT: p->n_op = GT; break;
				case LE: p->n_op = GE; break;
				case GT: p->n_op = LT; break;
				case GE: p->n_op = LE; break;
				}
			}
		}
		break;
	}

	if (p->n_op != FCON)
		return;

	w0 = p->n_scon->fp[0];
	w1 = (p->n_type == FLOAT) ? 0 : p->n_scon->fp[1];
	for (fc = fcons; fc != NULL; fc = fc->next) {
		if (fc->type == p->n_type && fc->w0 == w0 && fc->w1 == w1) {
			p->n_op = NAME;
			slval(p, 0);
			p->n_sp = fc->sp;
			return;
		}
	}

	sp = permalloc(sizeof(struct symtab));
	/*
	 * Everything must be zero: "sap" above is a macro for "sss" in this
	 * file, so assigning to it leaves the real attribute list as garbage.
	 */
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
	ninval(0, tsize(sp->stype, sp->sdf, sp->sap), p);

	if (w0 == 0 && w1 == 0 && fzerolab < 0)
		fzerolab = sp->soffset;		/* see fpzero() in local2.c */

	fc = permalloc(sizeof(struct fconst));
	fc->type = p->n_type;
	fc->w0 = w0;
	fc->w1 = w1;
	fc->sp = sp;
	fc->next = fcons;
	fcons = fc;

	p->n_op = NAME;
	slval(p, 0);
	p->n_sp = sp;
}

/*ARGSUSED*/
int
andable(NODE *p)
{
	return(1);	/* all names can have & taken on them */
}

/*
 * Return 1 if a variable of type type is OK to put in register.
 */
int
cisreg(TWORD t)
{
	/* a long long lives in memory: it is addressed, never held in a register */
	if (t == LONGLONG || t == ULONGLONG)
		return 0;
	return 1;
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
	sp = buildtree(PLUS, sp, bcon(1));
	t->n_type = sp->n_type;
	ecomp(buildtree(ASSIGN, t, sp)); /* Emit! */

}

/*
 * Print out a string of characters.
 * Assume that the assembler understands C-style escape
 * sequences.
 */
void
instring(struct symtab *sp)
{
	unsigned short sh[2];
	int val, cnt;
	TWORD t;
	char *s;

	defloc(sp);
	t = BTYPE(sp->stype);
	s = sp->sname;
	if (t == UNSIGNED) {
		/* convert to UTF-16 */
		while (*s) {
			cp2u16(u82cp(&s), sh);
			if (sh[0]) printf("%o\n", sh[0]);
			if (sh[1]) printf("%o\n", sh[1]);
		}
		printf("0\n");
	} else if (t == CHAR) {
		for (cnt = 0; *s != 0; ) {
			if (cnt++ == 0)
				printf(".byte ");
			if (*s == '\\')
				val = esccon(&s);
			else
				val = *s++;
			printf("%o", val & 0377);
			if (cnt > 15) {
				cnt = 0;
				printf("\n");
			} else
				printf(",");
		}
		printf("%s0\n", cnt ? "" : ".byte ");
	} else
		cerror("instring");
}

/*
 * print out a constant node, may be associated with a label.
 * Do not free the node after use.
 * off is bit offset from the beginning of the aggregate
 * fsz is the number of bits this is referring to
 */
int
ninval(CONSZ off, int fsz, NODE *p)
{
#ifndef LANG_CXX
	SFP sfp = p->n_scon;
#endif
	TWORD t;
	int i;

	t = p->n_type;
	switch (t) {
	case LONGLONG:
	case ULONGLONG:
		/* four words, the most significant first */
		{
			CONSZ v = glval(p);

			printf("%o ; %o ; %o ; %o\n", (int)((v >> 48) & 0177777),
			    (int)((v >> 32) & 0177777), (int)((v >> 16) & 0177777),
			    (int)(v & 0177777));
		}
		break;
	case LONG:
	case ULONG:
		printf("%o ; %o\n", (int)((glval(p) >> 16) & 0177777),
		    (int)(glval(p) & 0177777));
		break;
#ifndef LANG_CXX
	case FLOAT:
		printf("%o ; %o\n", sfp->fp[0] & 0xffff, sfp->fp[0] >> 16);
		break;
	case LDOUBLE:
	case DOUBLE:
		printf("%o ; %o ; %o ; %o\n", sfp->fp[0] & 0xffff,
		    sfp->fp[0] >> 16, sfp->fp[1] & 0xffff, sfp->fp[1] >> 16);
		break;
#endif
	default:
		return 0;
	}
	return 1;
}

/* make a name look like an external name in the local machine */
char *
exname(char *p)
{
#define NCHNAM  256
	static char text[NCHNAM+1];
	int i;

	if (p == NULL)
		return "";

	text[0] = '_';
	for (i=1; *p && i<NCHNAM; ++i)
		text[i] = *p++;

	text[i] = '\0';
	text[NCHNAM] = '\0';  /* truncate */

	return (text);

}

/*
 * map types which are not defined on the local machine
 */
TWORD
ctype(TWORD type)
{
	switch (BTYPE(type)) {
	case SHORT:
		MODTYPE(type,INT);
		break;

	case USHORT:
		MODTYPE(type,UNSIGNED);
		break;

	case LDOUBLE:
		MODTYPE(type,DOUBLE);
		break;

	}
	return (type);
}

void
calldec(NODE *p, NODE *q) 
{
}

void
extdec(struct symtab *q)
{
}

struct bssent {
	struct bssent *next;
	char *name;
	int off;
};
static struct bssent *bsshead, **bsstail = &bsshead;

/* Write the collected zero-initialised statics; called at end of file. */
void
flushbss(void)
{
	struct bssent *b;

	if (bsshead == NULL)
		return;
	/* the empty .globl is there to stop c2 deleting ".bss" after a jump */
	printf(".globl\n.bss\n");
	for (b = bsshead; b != NULL; b = b->next)
		printf("%s:.=.+%o\n", b->name, b->off);
}

/* make a common declaration for id, if reasonable */
void
defzero(struct symtab *sp)
{
	extern int lastloc;
	char *n;
	int off;

	off = tsize(sp->stype, sp->sdf, sp->sap);
	off = (off+(SZCHAR-1))/SZCHAR;
	n = getexname(sp);
	if (sp->sclass == STATIC) {
		/*
		 * Zero-initialised statics are collected and written as one
		 * ".bss" block at the end of the file (flushbss()).  Written
		 * where they are declared they end up after a function's
		 * "jmp cret", and c2 deletes ".bss" there (everything after
		 * a jump up to the next label goes), leaving the storage in
		 * the text or data segment.
		 */
		struct bssent *b = xmalloc(sizeof(struct bssent));

		b->next = NULL;
		b->off = off;
		if (sp->slevel == 0)
			b->name = xstrdup(n);
		else {
			b->name = xmalloc(24);
			snprintf(b->name, 24, LABFMT, (int)sp->soffset);
		}
		*bsstail = b;
		bsstail = &b->next;
		return;
	}
	printf(".comm ");
	if (sp->slevel == 0)
		printf("%s,%o\n", n, off);
	else
		printf(LABFMT ",%o\n", sp->soffset, off);
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
 * Called when a identifier has been declared.
 */
void
fixdef(struct symtab *sp)
{
}

void
pass1_lastchance(struct interpass *ip)
{
}

#include "lowerll.c"
