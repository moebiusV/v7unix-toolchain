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

# include "pass2.h"
# include <ctype.h>
# include <string.h>

#define	BITTEST	0x4000	/* n_qual of a compare that is really a bit test */
#define	BYTE_BIS 0x4001	/* n_qual of an ASSIGN that is really bisb */
#define	BYTE_BIC 0x4002	/* ... or bicb */
#define	BYTE_INC 0x4003	/* ... or incb */
#define	LONG_ADD 0x4007	/* a long store that adds in place... */
#define	LONG_SUB 0x4008	/* ... or subtracts */
#define	BYTE_DEC 0x4004	/* ... or decb */
#define	WORD_INC 0x4005	/* ... or inc of a word */
#define	WORD_DEC 0x4006	/* ... or dec of a word */
static int spcoff;
static int argsiz(NODE *p);
static void negcon(FILE *fp, int con);

void
deflab(int label)
{
	/* c1 writes a label and the instruction after it on one line */
	printf(LABFMT ":", label);
}

void
prologue(struct interpass_prolog *ipp)
{
	int addto;

#ifdef LANG_F77
	if (ipp->ipp_flags & IF_VISIBLE)
		printf("	.globl %s\n", ipp->ipp_name);
	printf("%s:\n", ipp->ipp_name);
#endif
	printf("jsr	r5,csv\n");
	addto = p2maxautooff;
	if (addto >= AUTOINIT/SZCHAR)
		addto -= AUTOINIT/SZCHAR;
	if (addto & 1)
		addto++;
	if (addto == 2)
		printf("tst	-(sp)\n");
	else if (addto == 4)
		printf("cmp	-(sp),-(sp)\n");
	else if (addto > 4)
		printf("sub	$%o,sp\n", addto);
	spcoff = 0;
}

void
eoftn(struct interpass_prolog *ipp)
{
	if (spcoff)
		comperr("spcoff == %d", spcoff);
	if (ipp->ipp_ip.ip_lbl == 0)
		return; /* no code needs to be generated */
	printf("jmp	cret\n");
}

/*
 * add/sub/...
 *
 * Param given:
 */
void
hopcode(int f, int o)
{
	char *str;

	switch (o) {
	case PLUS:
		str = "add";
		break;
	case MINUS:
		str = "sub";
		break;
	case AND:
		str = "and";
		break;
	case OR:
		str = "or";
		break;
	case ER:
		str = "xor";
		break;
	default:
		comperr("hopcode2: %d", o);
		str = 0; /* XXX gcc */
	}
	printf("%s%c", str, f);
}

/*
 * Return type size in bytes.  Used by R2REGS, arg 2 to offset().
 */
int
tlen(p) NODE *p;
{
	switch(p->n_type) {
		case CHAR:
		case UCHAR:
			return(1);

		case SHORT:
		case USHORT:
			return(SZSHORT/SZCHAR);

		case DOUBLE:
			return(SZDOUBLE/SZCHAR);

		case INT:
		case UNSIGNED:
		case LONG:
		case ULONG:
			return(SZINT/SZCHAR);

		case LONGLONG:
		case ULONGLONG:
			return SZLONGLONG/SZCHAR;

		default:
			if (!ISPTR(p->n_type))
				comperr("tlen type %d not pointer");
			return SZPOINT(p->n_type)/SZCHAR;
		}
}

/*
 * Emit code to compare two long numbers.
 */
static void
twolcomp(NODE *p)
{
	int u;
	int s = getlab2();
	int e = p->n_label;
	int cb1, cb2;

	switch (u = p->n_op) {
	case NE:
		cb1 = 0;
		cb2 = NE;
		break;
	case EQ:
		cb1 = NE;
		cb2 = 0;
		break;
	case LE:
	case LT:
		u += (ULE-LE);
		/* FALLTHROUGH */
	case ULE:
	case ULT:
		cb1 = GT;
		cb2 = LT;
		break;
	case GE:
	case GT:
		u += (ULE-LE);
		/* FALLTHROUGH */
	case UGE:
	case UGT:
		cb1 = LT;
		cb2 = GT;
		break;

	default:
		cb1 = cb2 = 0; /* XXX gcc */
	}
	if (p->n_op >= ULE)
		cb1 += 4, cb2 += 4;
	if (xdmr && p->n_right->n_op == ICON && p->n_right->n_name[0] == '\0' &&
	    getlval(p->n_right) == 0) {
		/* against zero: tst does for cmp $0, and the sign says it all */
		switch (p->n_op) {
		case LT:
		case GE:
			expand(p, 0, "tst	AL\n");
			cbgen(p->n_op, e);
			deflab(s);
			return;
		case NE:
			expand(p, 0, "tst	AL\n");
			cbgen(NE, e);
			expand(p, 0, "tst	UL\n");
			cbgen(NE, e);
			deflab(s);
			return;
		case EQ:
			expand(p, 0, "tst	AL\n");
			cbgen(NE, s);
			expand(p, 0, "tst	UL\n");
			cbgen(EQ, e);
			deflab(s);
			return;
		}
		expand(p, 0, "tst	AL\n");
		if (cb1) cbgen(cb1, s);
		if (cb2) cbgen(cb2, e);
		expand(p, 0, "tst	UL\n");
		cbgen(u, e);
		deflab(s);
		return;
	}
	expand(p, 0, "cmp	AL,AR\n");
	if (cb1) cbgen(cb1, s);
	if (cb2) cbgen(cb2, e);
        expand(p, 0, "cmp	UL,UR\n");
        cbgen(u, e);
        deflab(s);
}

/*
 * print out a 32-bit constant as arg.
 */
static void
lconarg(NODE *p)
{
	CONSZ c = getlval(p);
	char *m = spcoff ? "-" : "";

	spcoff += 4;

	if (c == 0) {
		printf("clr	%s(sp)\nclr	-(sp)\n", m);
	} else if (c >= 0 && c < 65536) {
		printf("mov	$%llo,%s(sp)\nclr	-(sp)\n", c, m);
	} else if (c >= -32768 && c < 32768) {
		printf("mov	$%llo,%s(sp)\nsxt	-(sp)\n",
		    c & 0177777, m);
	} else {
		printf("mov	$%llo,%s(sp)\nmov	$%llo,-(sp)\n",
		    c & 0177777, m, (c >> 16) & 0177777);
	}
}

/*
 * p->n_left += or -= p->n_right on two-word longs (also when p is the
 * PLUS or MINUS itself).  A constant under 64K needs no second word.
 */
static void
longaddsub(NODE *p, int sub)
{
	NODE *r = p->n_right;
	long v;

	if (r->n_op == ICON && r->n_name[0] == '\0') {
		v = (long)(getlval(r) & 0xffffffffL);
		if (v > 0xffff0000L) {		/* -n: the other way */
			v = 0x100000000L - v;
			sub = !sub;
		}
		if (xdmr && v <= 0xffff) {
			if (v != 0) {
				printf("%s	$%lo,", sub ? "sub" : "add", v);
				expand(p, FOREFF, "UL\n");
				printf("%s	", sub ? "sbc" : "adc");
				expand(p, FOREFF, "AL\n");
			}
			return;
		}
	}
	expand(p, FOREFF, sub ? "sub	AR,AL\nsub	UR,UL\nsbc	AL\n" :
	    "add	AR,AL\nadd	UR,UL\nadc	AL\n");
}

/* helper routines a file needs (see helpers.h and ejobcode()) */
#include "helpers.h"
#define	HELP_UDIV	1
#define	HELP_UREM	2
#define	HELP_ULDIV	4
#define	HELP_ULREM	8
int helpers_used;

/* write the helpers this file called, as local routines */
void
puthelpers(void)
{
	if (helpers_used == 0)
		return;
	printf(".text\n");
	if (helpers_used & HELP_UDIV)
		fputs(helper_udiv, stdout);
	if (helpers_used & HELP_UREM)
		fputs(helper_urem, stdout);
	if (helpers_used & HELP_ULDIV)
		fputs(helper_uldiv, stdout);
	if (helpers_used & HELP_ULREM)
		fputs(helper_ulrem, stdout);
}

void
zzzcode(NODE *p, int c)
{
	struct attr *ap;
	NODE *l;
	char *s;
	int o;

	switch (c) {
	case 'A': /* print out - if not first arg */
		if (spcoff || (p->n_type == FLOAT || p->n_type == DOUBLE))
			printf("-");
		spcoff += argsiz(p);
		break;

	case 'B': /* arg is pointer to block */
		expand(p->n_left, FOREFF, "mov	AL,ZA(sp)\n");
		expand(p->n_left, FOREFF, p->n_left->n_op == PLUS ?
		    "add	AR,(sp)\n" : "sub	AR,(sp)\n");
		break;
		
	case 'C': /* subtract stack after call */
		spcoff -= p->n_qual;
		if (spcoff == 0 /* && !(p->n_flags & NLOCAL1) XXX FIXME */)
			p->n_qual -= 2;
		if (p->n_qual == 2)
			printf("tst	(sp)+\n");
		else if (p->n_qual == 4)
			printf("cmp	(sp)+,(sp)+\n");
		else if (p->n_qual > 2)
			printf("add	$%o,sp\n", (int)p->n_qual);
		break;

	case 'D': /* long constant as arg */
		lconarg(p->n_left);
		break;

	case 'E': /* long move */
		rmove(regno(p->n_right), regno(p->n_left), p->n_type);
		break;

	case 'L': /* long add or subtract, cheaper for a constant under 64K */
		longaddsub(p, p->n_op == MINUS);
		break;

	case 'X': /* long store; or add/subtract in place (see longrw()) */
		if (p->n_qual == LONG_ADD || p->n_qual == LONG_SUB)
			longaddsub(p, p->n_qual == LONG_SUB);
		else
			expand(p, FOREFF, "mov	AR,AL\nmov	UR,UL\n");
		break;

	case 'F': /* long comparision */
		twolcomp(p);
		break;

	case 'G': /* printout a subnode for post-inc */
		adrput(stdout, p->n_left->n_left);
		break;

	case 'P': /* value of a post-incremented word: (x += 1) - 1 */
	case 'Q': /* ... of a byte */
		l = p->n_left;
		if (l->n_op == ASSIGN) {
			/* the increment is still ours to do: copy, then step */
			printf(c == 'P' ? "mov	" : "movb	");
			adrput(stdout, l->n_left);
			printf(",");
			expand(p, FOREFF, "A1\n");
			printf(c == 'P' ? "inc	" : "incb	");
			adrput(stdout, l->n_left);
			printf("\n");
		} else {
			/*
			 * The assignment has already been generated and l is
			 * the new value in a register: the old one is one less.
			 */
			printf("mov	");
			adrput(stdout, l);
			printf(",");
			expand(p, FOREFF, "A1\ndec	A1\n");
		}
		break;

	case 'H': /* arg with post-inc */
		expand(p->n_left->n_left, FOREFF, "mov	AL,ZA(sp)\n");
		expand(p->n_left->n_left, FOREFF, "inc	AL\n");
		break;

	case 'I': /* struct assign. Right in R0, left R1, counter R2. */
		ap = attr_find(p->n_ap, ATTR_P2STRUCT);
		l = p->n_left;
		if (l->n_op == OREG) {
			int r = l->n_rval;
			if (R2TEST(r)) {
				l->n_rval = R2UPK1(r);
				expand(p, FOREFF, "mov	AL,r1\n");
				l->n_rval = r;
			} else {
				printf("mov	%s,r1\n", rnames[r]);
				if (getlval(l)) {
					printf("add	$");
					negcon(stdout, getlval(l));
					printf(",r1\n");
				}
			}
		} else
			printf("mov	$%s,r1\n", l->n_name);
		printf("mov	$%o,r2\n", (ap->iarg(0)+1) >> 1);
		printf("1: mov	(r0)+,(r1)+\n");
		printf("sob	r2,1b\n");
		break;

	case 'J': /* struct argument */
		ap = attr_find(p->n_ap, ATTR_P2STRUCT);
		o = (ap->iarg(0) + 1) & ~1;
		printf("sub	$%o,sp\n", spcoff == 0 ? o-2 : o);
		printf("mov	sp,r0\n");
		printf("mov	$%o,r2\n", o >> 1);
		printf("1: mov	(r1)+,(r0)+\n");
		printf("sob	r2,1b\n");
		spcoff += argsiz(p);
		break;

	case 'V': /* word store; or inc/dec in place (see wordrw()) */
		expand(p, FOREFF, p->n_qual == WORD_INC ? "inc	AL\n" :
		    p->n_qual == WORD_DEC ? "dec	AL\n" : "mov	AR,AL\n");
		break;

	case 'W': /* byte store; or bisb/bicb in place (see byterw()) */
		expand(p, FOREFF, p->n_qual == BYTE_BIS ? "bisb	AR,AL\n" :
		    p->n_qual == BYTE_BIC ? "bicb	AR,AL\n" :
		    p->n_qual == BYTE_INC ? "incb	AL\n" :
		    p->n_qual == BYTE_DEC ? "decb	AL\n" : "movb	AR,AL\n");
		break;

	case 'T': /* compare in memory, or a bit test (bit/bitb) */
		o = p->n_left->n_type == CHAR || p->n_left->n_type == UCHAR;
		if (p->n_qual == BITTEST)
			expand(p, FORCC, o ? "bitb	AR,AL\n" : "bit	AR,AL\n");
		else
			expand(p, FORCC, o ? "cmpb	AL,AR\n" : "cmp	AL,AR\n");
		break;

	case 'N': /* constant call target, in c1's CALL1/CALL2 form */
		/*
		 * c1 writes "*$_f" when the first argument went to (sp)
		 * without a push (the outermost call has arguments), and plain
		 * "_f" otherwise.
		 */
		if (p->n_qual > 0 && spcoff == p->n_qual)
			printf("*$");
		conput(stdout, p->n_left);
		break;

	case 'k': /* call the unsigned int divide or remainder helper */
		if (p->n_op == DIV) {
			helpers_used |= HELP_UDIV;
			printf("jsr	pc,udiv\n");
		} else {
			helpers_used |= HELP_UREM;
			printf("jsr	pc,urem\n");
		}
		break;

	case 'K': /* long long mul/div */
		o = spcoff++;
		if (p->n_right->n_op == ICON) {
			lconarg(p->n_right);
		} else
			expand(p, FOREFF, "mov	UR,-(sp)\nmov	AR,-(sp)\n");
		if (p->n_left->n_op == ICON) {
			lconarg(p->n_left);
		} else
			expand(p, FOREFF, "mov	UL,-(sp)\nmov	AL,-(sp)\n");
		s = p->n_op == MUL ? "lmul" :
		    p->n_op == DIV ? (p->n_type == ULONG ? "uldiv" : "ldiv") :
		    (p->n_type == ULONG ? "ulrem" : "lrem");
		if (strcmp(s, "uldiv") == 0)
			helpers_used |= HELP_ULDIV;
		else if (strcmp(s, "ulrem") == 0)
			helpers_used |= HELP_ULREM;
		printf("jsr	pc,%s\nadd	$10,sp\n", s);
		spcoff = o;
		break;

	default:
		comperr("zzzcode %c", c);
	}
}

/*ARGSUSED*/
int
rewfld(NODE *p)
{
	return(1);
}

int canaddr(NODE *);
int
canaddr(NODE *p)
{
	int o = p->n_op;

	if (o==NAME || o==REG || o==ICON || o==OREG ||
	    (o==UMUL && shumul(p->n_left, STARNM|SOREG)))
		return(1);
	return(0);
}

/*
 * Does the bitfield shape match?
 */
int
flshape(NODE *p)
{
	int o = p->n_op;

	if (o == OREG || o == REG || o == NAME)
		return SRDIR; /* Direct match */
	if (o == UMUL && shumul(p->n_left, SOREG))
		return SROREG; /* Convert into oreg */
	return SRREG; /* put it into a register */
}

/* INTEMP shapes must not contain any temporary registers */
/* XXX should this go away now? */
int
shtemp(NODE *p)
{
	return 0;
}

static void
negcon(FILE *fp, int con)
{
	if (con < 0)
		fprintf(fp, "-"), con = -con;
	fprintf(fp, "%o", con & 0177777);
}

void
adrcon(CONSZ val)
{
	printf("$" CONFMT, val);
}

void
conput(FILE *fp, NODE *p)
{
	int val = getlval(p);

	switch (p->n_op) {
	case ICON:
		if (p->n_name[0] != '\0') {
			/* c1 spells it "off+name" */
			if (val)
				fprintf(fp, "%o+", val & 0177777);
			fprintf(fp, "%s", p->n_name);
		} else if (p->n_type == LONG || p->n_type == ULONG)
			negcon(fp, val >> 16);
		else
			negcon(fp, val);
		return;

	default:
		comperr("illegal conput, p %p", p);
	}
}

/*ARGSUSED*/
void
insput(NODE *p)
{
	comperr("insput");
}

/*
 * Write out the upper address, like the upper register of a 2-register
 * reference, or the next memory location.
 */
void
upput(NODE *p, int size)
{
	size /= SZINT;
	switch (p->n_op) {
	case NAME:
	case OREG:
		setlval(p, getlval(p) + size);
		adrput(stdout, p);
		setlval(p, getlval(p) - size);
		break;
	case REG:
		printf("r%c", rnames[p->n_rval][2]);
		break;
	case ICON:
		/* On PDP11 upper value is low 16 bits */
		printf("$");
		negcon(stdout, getlval(p) & 0177777);
		break;
	default:
		comperr("upput bad op %d size %d", p->n_op, size);
	}
}

/*
 * output an address, with offsets, from p
 */
void
adrput(FILE *io, NODE *p)
{
	int r;

	if (p->n_op == FLD)
		p = p->n_left;

	switch (p->n_op) {
	case NAME:
		if (p->n_name[0] != '\0') {
			/* c1 spells it "off+name" */
			if (getlval(p) != 0)
				fprintf(io, "%o+", (int)(getlval(p) & 0177777));
			fputs(p->n_name, io);
		} else
			negcon(io, getlval(p));
		return;

	case OREG:
		r = p->n_rval;
		if (R2TEST(r) && R2UPK3(r) == 0)
			printf("*");
		if (R2TEST(r) && R2UPK3(r) == 2)
			fprintf(io, "-");
		if (p->n_name[0]) {
			/* c1 spells it "off+name" */
			if (getlval(p)) {
				negcon(io, getlval(p));
				printf("+");
			}
			printf("%s", p->n_name);
		} else if (getlval(p))
			negcon(io, getlval(p));
		if (R2TEST(r)) {
			fprintf(io, "(%s)", rnames[R2UPK1(r)]);
			if (R2UPK3(r) == 1)
				fprintf(io, "+");
		} else
			fprintf(io, "(%s)", rnames[p->n_rval]);
		return;
	case ICON:
		/* addressable value of the constant */
		printf("$");
		conput(io, p);
		return;

	case REG:
		switch (p->n_type) {
		case LONG:
		case ULONG:
			fprintf(io, "r%c", rnames[p->n_rval][1]);
			break;
		default:
			fprintf(io, "%s", rnames[p->n_rval]);
		}
		return;

	case UMUL:
		if (tshape(p, STARNM)) {
			printf("*");
			adrput(io, p->n_left);
			break;
		}
		/* FALLTHROUGH */
	default:
		comperr("illegal address, op %d, node %p", p->n_op, p);
		return;

	}
}

static char *
ccbranches[] = {
	"jeq",		/* jumpe */
	"jne",		/* jumpn */
	"jle",		/* jumple */
	"jlt",		/* jumpl */
	"jge",		/* jumpge */
	"jgt",		/* jumpg */
	"jlos",		/* jumple (jlequ) */
	"jlo",		/* jumpl (jlssu) */
	"jhis",		/* jumpge (jgequ) */
	"jhi",		/* jumpg (jgtru) */
};


/*   printf conditional and unconditional branches */
void
cbgen(int o, int lab)
{
	if (o < EQ || o > UGT)
		comperr("bad conditional branch: %s", opst[o]);
	printf("%s	" LABFMT "\n", ccbranches[o-EQ], lab);
}

#define	IS1CON(p) ((p)->n_op == ICON && getlval(p) == 1)

/*
 * Move postfix operators to the next statement, unless they are 
 * within a function call or a branch.
 */
static void
cvtree(NODE *p, struct interpass *ip2)
{
	struct interpass *ip;
	NODE *q;

	if (callop(p->n_op) || p->n_op == CBRANCH)
		return;

	if ((p->n_op == PLUS || p->n_op == MINUS) &&
	    IS1CON(p->n_right) && (q = p->n_left)->n_op == ASSIGN &&
	    treecmp(q->n_left, q->n_right->n_left) &&
	    IS1CON(q->n_right->n_right)) {
		if ((p->n_op == PLUS && q->n_right->n_op == MINUS) ||
		    (p->n_op == MINUS && q->n_right->n_op == PLUS)) {
			nfree(p->n_right);
			*p = *q->n_left;
			if (optype(p->n_op) != LTYPE)
				p->n_left = tcopy(p->n_left);
			ip = ipnode(q);
			DLIST_INSERT_AFTER(ip2, ip, qelem);
			return;
		}
	}
	if (optype(p->n_op) == BITYPE)
		cvtree(p->n_right, ip2);
	if (optype(p->n_op) != LTYPE)
		cvtree(p->n_left, ip2);
}

extern int fzerolab;

/*
 * x compared with 0.0: the zero was made a stored constant by myp2tree();
 * make it a real zero again so that tstf x can be used.
 */
static int
fpzero(NODE *p)
{
	char buf[24];
	NODE *r = p->n_right;

	if (fzerolab < 0 || r->n_op != NAME || getlval(r) != 0 ||
	    (r->n_type != DOUBLE && r->n_type != FLOAT && r->n_type != LDOUBLE))
		return 0;
	snprintf(buf, sizeof buf, LABFMT, fzerolab);
	if (strcmp(r->n_name, buf) != 0)
		return 0;
	nfree(r);
	p->n_right = mklnode(ICON, 0, 0, INT);
	return 1;
}

/*
 * "(x & mask) == 0" is a bit/bitb, which takes the mask as it is.  fixops()
 * has just turned the AND below p into a bic with a complemented mask; take
 * it apart again and give the comparison x and mask directly, marked with
 * BITTEST in n_qual so that ZT prints bit/bitb instead of cmp/cmpb.
 */
static void
bittest(NODE *p)
{
	NODE *a, *x, *m;
	TWORD t;
	CONSZ v;

	a = p->n_left;
	if (a->n_op != AND || p->n_right->n_op != ICON ||
	    p->n_right->n_name[0] != '\0' || getlval(p->n_right) != 0)
		return;
	x = a->n_left;
	m = a->n_right;
	if (m->n_op == ICON && m->n_name[0] == '\0')
		v = (~getlval(m)) & 0177777;	/* undo fixops */
	else if (m->n_op != COMPL)
		return;				/* complemented expression */
	else
		v = 0;

	if (m->n_op == ICON && v == 0)
		return;				/* nothing to test; tst would do */

	t = a->n_type;
	if (x->n_op == SCONV && (x->n_left->n_type == CHAR ||
	    x->n_left->n_type == UCHAR)) {
		/* a byte flag tested through int promotion: test the byte */
		if (m->n_op != ICON || v > 0377)
			return;
		NODE *c = x->n_left;

		t = c->n_type;
		nfree(x);
		x = c;
	} else if ((x->n_op == PCONV || x->n_op == SCONV) && ISPTR(x->n_left->n_type) &&
	    !ISPTR(x->n_type) && (x->n_left->n_op == OREG ||
	    x->n_left->n_op == NAME || x->n_left->n_op == UMUL)) {
		/* ((int)p->ptr) & 1: the word is tested as it lies */
		NODE *c = x->n_left;

		nfree(x);
		x = c;
		t = INT;
	} else if (t != INT && t != UNSIGNED && !ISPTR(t))
		return;

	if (m->n_op == ICON) {
		setlval(m, v);
		m->n_type = t;
	} else {
		/* mask expression: drop the COMPL fixops put on it */
		NODE *q = m->n_left;
		nfree(m);
		m = q;
	}
	nfree(p->n_right);
	nfree(a);
	p->n_left = x;
	p->n_right = m;
	p->n_qual = BITTEST;
}

/*
 * Convert AND to BIC.
 */
static void
fixops(NODE *p, void *arg)
{
	static int fltwritten;
	NODE *r;

	if (!fltwritten && (p->n_type == FLOAT || p->n_type == DOUBLE)) {
		printf(".globl	fltused\n");
		fltwritten = 1;
	}
	switch (p->n_op) {
	case AND:
		if (p->n_right->n_op == ICON) {
			CONSZ val = getlval(p->n_right);
			TWORD t = p->n_right->n_type;
			if (t == LONGLONG || t == ULONGLONG)
				val = ~val;
			else if (t == LONG || t == ULONG)
				val = ((~val) & 0xffffffff);
			else
				val = ((~val) & 0177777);
			setlval(p->n_right, val);
		} else if (p->n_right->n_op == COMPL) {
			NODE *q = p->n_right->n_left;
			nfree(p->n_right);
			p->n_right = q;
		} else
			p->n_right = mkunode(COMPL, p->n_right, 0, p->n_type);
		break;
	case RS:
		r = p->n_right;
		if (r->n_op == ICON)
			setlval(r, -getlval(r));
		else
			p->n_right = mkunode(UMINUS, r, 0, r->n_type);
		if (p->n_type == UNSIGNED || p->n_type == UCHAR)
			break;
		p->n_op = LS;
		break;
	case EQ:
	case NE:
	case LT:
	case LE:
	case GT:
	case GE:
		if (fpzero(p))
			break;
		if (p->n_op == EQ || p->n_op == NE)
			if (xdmr)
				bittest(p);
		break;
	}
}

/*
 * Two trees that name the same thing without side effects.
 */
static int
sametree(NODE *a, NODE *b)
{
	if (a->n_op != b->n_op || a->n_type != b->n_type)
		return 0;
	switch (optype(a->n_op)) {
	case LTYPE:
		if (getlval(a) != getlval(b) || a->n_rval != b->n_rval)
			return 0;
		if (a->n_op == NAME || a->n_op == OREG || a->n_op == ICON)
			return a->n_name[0] == b->n_name[0] &&
			    strcmp(a->n_name, b->n_name) == 0;
		return 1;
	case UTYPE:
		return sametree(a->n_left, b->n_left);
	default:
		return sametree(a->n_left, b->n_left) &&
		    sametree(a->n_right, b->n_right);
	}
}

/* An expression whose evaluation has no side effects, so naming it twice is
 * naming it once. */
static int
pure(NODE *n)
{
	switch (optype(n->n_op)) {
	case LTYPE:
		return 1;
	case UTYPE:
		return (n->n_op == UMUL || n->n_op == SCONV ||
		    n->n_op == PCONV) && pure(n->n_left);
	default:
		return (n->n_op == PLUS || n->n_op == MINUS ||
		    n->n_op == MUL || n->n_op == LS) &&
		    pure(n->n_left) && pure(n->n_right);
	}
}

static int
isbyte(TWORD t)
{
	return t == CHAR || t == UCHAR;
}

/*
 * Statement-level stores to bytes, whose value nobody uses:
 *	x |= c, x &= c   on a char:  bisb/bicb c,x  in place, as dmr-cc does,
 *	                 not load, bis, sign-extend, store;
 *	x = <int expr>   to a char:  store the low byte, without first
 *	                 sign-extending the int in its register.
 */
static void
byterw(NODE *p)
{
	NODE *l, *r, *o, *t;
	CONSZ v;

	/* a discarded cast of the whole statement: look at what it casts */
	while (p->n_op == SCONV || p->n_op == PCONV)
		p = p->n_left;
	if (p->n_op != ASSIGN || !isbyte(p->n_type))
		return;
	l = p->n_left;
	r = p->n_right;
	if (r->n_op != SCONV || !isbyte(r->n_type))
		return;
	o = r->n_left;
	if ((l->n_op == OREG || l->n_op == NAME || (l->n_op == UMUL && pure(l))) &&
	    (o->n_op == PLUS || o->n_op == MINUS) && o->n_type == INT &&
	    o->n_left->n_op == SCONV && isbyte(o->n_left->n_left->n_type) &&
	    sametree(o->n_left->n_left, l) && o->n_right->n_op == ICON &&
	    o->n_right->n_name[0] == '\0' &&
	    (getlval(o->n_right) == 1 || getlval(o->n_right) == -1)) {
		/* c++ and c-- on a byte in memory: incb / decb */
		v = getlval(o->n_right);
		if (o->n_op == MINUS)
			v = -v;
		t = o->n_left;
		p->n_qual = v > 0 ? BYTE_INC : BYTE_DEC;
		p->n_right = o->n_right;
		setlval(p->n_right, 1);
		p->n_right->n_type = p->n_type;
		tfree(t->n_left);
		nfree(t);
		nfree(o);
		nfree(r);
		return;
	}
	if ((l->n_op == OREG || l->n_op == NAME || (l->n_op == UMUL && pure(l))) &&
	    (o->n_op == OR || o->n_op == AND) && o->n_type == INT &&
	    o->n_left->n_op == SCONV && isbyte(o->n_left->n_left->n_type) &&
	    sametree(o->n_left->n_left, l) && o->n_right->n_op == ICON &&
	    o->n_right->n_name[0] == '\0') {
		/* AND already carries bic's complemented mask (see fixops) */
		v = getlval(o->n_right) & 0377;
		if (v == 0)
			return;			/* a no-op; not worth a bisb */
		t = o->n_left;
		p->n_qual = o->n_op == OR ? BYTE_BIS : BYTE_BIC;
		p->n_right = o->n_right;
		setlval(p->n_right, v);
		p->n_right->n_type = p->n_type;
		tfree(t->n_left);
		nfree(t);
		nfree(o);
		nfree(r);
		return;
	}
	{
		/* int <-> unsigned conversions change nothing: look through them */
		NODE *oo = o, *x, *nx;

		while (oo->n_op == SCONV && (oo->n_type == INT || oo->n_type == UNSIGNED) &&
		    (oo->n_left->n_type == INT || oo->n_left->n_type == UNSIGNED))
			oo = oo->n_left;
		if ((l->n_op == OREG || l->n_op == NAME || l->n_op == UMUL) &&
		    (oo->n_type == INT || oo->n_type == UNSIGNED) &&
		    oo->n_op != ICON && oo->n_op != SCONV) {
			/* the int goes to a register; movb stores its low byte */
			for (x = o; x != oo; x = nx) {
				nx = x->n_left;
				nfree(x);
			}
			p->n_right = oo;
			nfree(r);
		}
	}
}

/*
 * Parameters are copied to temporaries at entry.  One that is never assigned
 * again and used just once is better left where the caller put it: the copy
 * costs two words, the use of the stack slot only one.
 */
struct ptcount {
	int regno;
	int total;	/* occurrences of the temporary */
	int defs;	/* ... as the target of an assignment */
};

static void
ptcount_cb(NODE *p, void *arg)
{
	struct ptcount *c = arg;

	if (p->n_op == TEMP && regno(p) == c->regno)
		c->total++;
	if (p->n_op == ASSIGN && p->n_left->n_op == TEMP &&
	    regno(p->n_left) == c->regno)
		c->defs++;
}

/* Replace the one use of temporary t by with; returns 1 if done. */
static int
ptreplace(NODE **pp, int t, NODE *with)
{
	NODE *p = *pp;

	if (p->n_op == TEMP && regno(p) == t) {
		/* the use may see the word as another type of the same size */
		with->n_type = p->n_type;
		nfree(p);
		*pp = with;
		return 1;
	}
	if (optype(p->n_op) == LTYPE)
		return 0;
	if (ptreplace(&p->n_left, t, with))
		return 1;
	return optype(p->n_op) == BITYPE && ptreplace(&p->n_right, t, with);
}

static void
parmprop(struct interpass *ipole)
{
	struct interpass *ip, *jp, *next;
	struct ptcount c;
	NODE *p, *use;

	for (ip = DLIST_NEXT(ipole, qelem); ip != ipole; ip = next) {
		next = DLIST_NEXT(ip, qelem);
		if (ip->type != IP_NODE)
			continue;
		p = ip->ip_node;
		if (p->n_op != ASSIGN || p->n_left->n_op != TEMP ||
		    p->n_right->n_op != OREG || p->n_right->n_rval != FPREG ||
		    getlval(p->n_right) < 4 || p->n_right->n_name[0] != '\0' ||
		    p->n_left->n_type != p->n_right->n_type)
			continue;
		c.regno = regno(p->n_left);
		c.total = c.defs = 0;
		for (jp = DLIST_NEXT(ipole, qelem); jp != ipole;
		    jp = DLIST_NEXT(jp, qelem))
			if (jp->type == IP_NODE)
				walkf(jp->ip_node, ptcount_cb, &c);
		/* the entry assignment is one occurrence and one def */
		if (c.defs != 1 || c.total != 2)
			continue;
		use = p->n_right;
		for (jp = DLIST_NEXT(ipole, qelem); jp != ipole;
		    jp = DLIST_NEXT(jp, qelem)) {
			if (jp == ip || jp->type != IP_NODE)
				continue;
			if (ptreplace(&jp->ip_node, c.regno, use))
				break;
		}
		if (jp == ipole)
			continue;
		/* the copy is gone: drop the assignment shell */
		nfree(p->n_left);
		nfree(p);
		DLIST_REMOVE(ip, qelem);
	}
}

/*
 * x *= e, x /= e and x %= e on a long in memory, statements: the library's
 * almul, aldiv and alrem take the address of x and do the update there, as
 * dmr-cc calls them (-xdmr); no load and store of x around ldiv.
 */
static void
longcall(NODE *p, NODE *l, NODE *o)
{
	NODE *a, *r, *fn, *cm, *call;
	TWORD pt = INCREF(p->n_type);
	char *name;

	switch (o->n_op) {
	case MUL: name = "almul"; break;
	case DIV: name = "aldiv"; break;
	default: name = "alrem"; break;
	}
	r = o->n_right;
	if (l->n_op == UMUL) {
		a = l->n_left;
		nfree(l);
	} else if (l->n_op == NAME) {
		a = mklnode(ICON, getlval(l), 0, pt);
		a->n_name = l->n_name;
		nfree(l);
	} else {			/* OREG: base register and offset */
		if (R2TEST(l->n_rval))
			return;
		a = mkbinode(PLUS, mklnode(REG, 0, l->n_rval, pt),
		    mklnode(ICON, getlval(l), 0, INT), pt);
		nfree(l);
	}
	a->n_type = pt;
	tfree(o->n_left);
	nfree(o);
	cm = mkbinode(CM, mkunode(FUNARG, a, 0, pt),
	    mkunode(FUNARG, r, 0, r->n_type), INT);
	fn = mklnode(ICON, 0, 0, INCREF(FTN + INT));
	fn->n_name = name;
	call = mkbinode(CALL, fn, cm, INT);
	*p = *call;
	nfree(call);
}

/*
 * "a += b" and "a -= b" on a long in memory whose value is not used:
 * add and adc straight into the variable, as dmr-cc does, not load,
 * add, store.
 */
static void
longrw(NODE *p)
{
	NODE *l, *o;

	while (p->n_op == SCONV || p->n_op == PCONV)
		p = p->n_left;
	if (p->n_op != ASSIGN || (p->n_type != LONG && p->n_type != ULONG))
		return;
	l = p->n_left;
	o = p->n_right;
	if (!(l->n_op == NAME || l->n_op == OREG || (l->n_op == UMUL && pure(l))) ||
	    (o->n_op != PLUS && o->n_op != MINUS && o->n_op != MUL &&
	    o->n_op != DIV && o->n_op != MOD) ||
	    o->n_type != p->n_type || !sametree(o->n_left, l))
		return;
	if (o->n_op == MUL || (p->n_type == LONG && (o->n_op == DIV || o->n_op == MOD))) {
		longcall(p, l, o);
		return;
	}
	if (o->n_op != PLUS && o->n_op != MINUS)
		return;
	p->n_qual = o->n_op == PLUS ? LONG_ADD : LONG_SUB;
	p->n_right = o->n_right;
	tfree(o->n_left);
	nfree(o);
}

/*
 * "(*p)++" and friends on a word reached through a pointer expression:
 * a statement of its own, so that the expression can be named just once
 * (see fwdauto()).  Plain variables are already handled by the matcher.
 */
static void
wordrw(NODE *p)
{
	NODE *l, *o;
	CONSZ v;

	while (p->n_op == SCONV || p->n_op == PCONV)
		p = p->n_left;
	if (p->n_op != ASSIGN || (p->n_type != INT && p->n_type != UNSIGNED))
		return;
	l = p->n_left;
	o = p->n_right;
	if (l->n_op != UMUL || !pure(l) || (o->n_op != PLUS && o->n_op != MINUS) ||
	    o->n_type != p->n_type || !sametree(o->n_left, l) ||
	    o->n_right->n_op != ICON || o->n_right->n_name[0] != '\0')
		return;
	v = getlval(o->n_right);
	if (v != 1 && v != -1)
		return;
	if (o->n_op == MINUS)
		v = -v;
	p->n_qual = v > 0 ? WORD_INC : WORD_DEC;
	p->n_right = o->n_right;
	setlval(p->n_right, 1);
	tfree(o->n_left);
	nfree(o);
}

/* x = (q += n) - n, or x = (q -= n): stepping the pointer q by n */
static int
autostep(NODE *x, int *size)
{
	NODE *a;

	if (x->n_op == MINUS && x->n_left->n_op == ASSIGN &&
	    x->n_right->n_op == ICON) {
		a = x->n_left;
		if (a->n_right->n_op == PLUS && a->n_right->n_right->n_op == ICON &&
		    getlval(a->n_right->n_right) == getlval(x->n_right) &&
		    sametree(a->n_left, a->n_right->n_left)) {
			*size = (int)getlval(x->n_right);
			return a->n_left->n_op == REG || a->n_left->n_op == TEMP;
		}
	}
	if (x->n_op == ASSIGN && x->n_right->n_op == MINUS &&
	    x->n_right->n_right->n_op == ICON &&
	    sametree(x->n_left, x->n_right->n_left)) {
		*size = (int)getlval(x->n_right->n_right);
		return x->n_left->n_op == REG || x->n_left->n_op == TEMP;
	}
	return 0;
}

/*
 * ++*q++ is "t = (q += 1) - 1; ++*t".  When t is used just there, put the
 * step into the use, where it can become the address mode (rN)+ or -(rN);
 * otherwise q is stepped in one instruction and used in another.
 */
static void
fwdauto(struct interpass *ipole)
{
	struct interpass *ip, *nx, *jp;
	struct ptcount c;
	NODE *a, *b, *u;
	int size, want;

	for (ip = DLIST_NEXT(ipole, qelem); ip != ipole; ip = nx) {
		nx = DLIST_NEXT(ip, qelem);
		if (ip->type != IP_NODE || nx == ipole || nx->type != IP_NODE)
			continue;
		a = ip->ip_node;
		b = nx->ip_node;
		if (a->n_op != ASSIGN || a->n_left->n_op != TEMP ||
		    !autostep(a->n_right, &size))
			continue;
		if (b->n_op != ASSIGN || b->n_left->n_op != UMUL ||
		    b->n_left->n_left->n_op != TEMP ||
		    regno(b->n_left->n_left) != regno(a->n_left))
			continue;
		switch (b->n_qual) {
		case BYTE_INC: case BYTE_DEC: want = 1; break;
		case WORD_INC: case WORD_DEC: want = 2; break;
		default: continue;
		}
		if (size != want)
			continue;
		c.regno = regno(a->n_left);
		c.total = c.defs = 0;
		for (jp = DLIST_NEXT(ipole, qelem); jp != ipole;
		    jp = DLIST_NEXT(jp, qelem))
			if (jp->type == IP_NODE)
				walkf(jp->ip_node, ptcount_cb, &c);
		if (c.defs != 1 || c.total != 2)
			continue;
		u = b->n_left->n_left;		/* the TEMP use */
		b->n_left->n_left = a->n_right;
		nfree(u);
		nfree(a->n_left);
		nfree(a);
		DLIST_REMOVE(ip, qelem);
	}
}

/*
 * Longs that are updated in place with something that is not a constant,
 * p += c - lseek(...), stay in memory, as dmr-cc has them (-xdmr).  A long in
 * a register pair that the allocator has to spill is loaded, updated and stored
 * around every use; in memory it is  add r1,-10(r5); add r0,-12(r5); adc ...
 * A parameter is used where it is, not copied.
 */
static int lm_reg, lm_off;
static int lm_idx;			/* index of the statement being walked */
static int lm_seen[64], lm_first[64], lm_last[64], lm_nseen;
static int lm_calls[512], lm_ncalls;

static int
lm_hascall(NODE *p)
{
	/* long multiply, divide and remainder are calls of library routines */
	if (callop(p->n_op) || ((p->n_op == MUL || p->n_op == DIV || p->n_op == MOD) &&
	    (p->n_type == LONG || p->n_type == ULONG)))
		return 1;
	switch (optype(p->n_op)) {
	case UTYPE: return lm_hascall(p->n_left);
	case BITYPE: return lm_hascall(p->n_left) || lm_hascall(p->n_right);
	}
	return 0;
}

/* first and last statement that mention each long temporary */
static void
lm_count(NODE *p, void *arg)
{
	int i;

	if (p->n_op != TEMP || (p->n_type != LONG && p->n_type != ULONG))
		return;
	for (i = 0; i < lm_nseen; i++)
		if (lm_seen[i] == regno(p))
			break;
	if (i == lm_nseen) {
		if (lm_nseen >= 64)
			return;
		lm_seen[lm_nseen] = regno(p);
		lm_first[lm_nseen++] = lm_idx;
	}
	lm_last[i] = lm_idx;
}

/* is the long temporary live across a call? */
static int
lm_across(int reg)
{
	int i, c;

	for (i = 0; i < lm_nseen; i++)
		if (lm_seen[i] == reg)
			break;
	if (i == lm_nseen)
		return 0;
	for (c = 0; c < lm_ncalls; c++)
		if (lm_calls[c] > lm_first[i] && lm_calls[c] < lm_last[i])
			return 1;
	return 0;
}

static void
lm_replace(NODE *p, void *arg)
{
	if (p->n_op == TEMP && regno(p) == lm_reg)
		storemod(p, lm_off, FPREG);
}

static int lm_nonconst[32];
static int lm_param(struct interpass *, int);
static void lm_demote(struct interpass *, int);

static void
lm_find(NODE *p, void *arg)
{
	int *tab = arg;		/* tab[0] = count, then register numbers */
	NODE *r;
	int i, nc;

	if (p->n_op != ASSIGN || p->n_left->n_op != TEMP ||
	    (p->n_left->n_type != LONG && p->n_left->n_type != ULONG))
		return;
	r = p->n_right;
	if ((r->n_op != PLUS && r->n_op != MINUS && r->n_op != MUL &&
	    r->n_op != DIV && r->n_op != MOD) || r->n_left->n_op != TEMP ||
	    regno(r->n_left) != regno(p->n_left))
		return;
	/* the update the register-pair rule above is about: x +- (not a constant) */
	nc = (r->n_op == PLUS || r->n_op == MINUS) && r->n_right->n_op != ICON;
	for (i = 1; i <= tab[0]; i++)
		if (tab[i] == regno(p->n_left)) {
			lm_nonconst[i] |= nc;
			return;
		}
	if (tab[0] < 30) {
		tab[++tab[0]] = regno(p->n_left);
		lm_nonconst[tab[0]] = nc;
	}
}

static void
longmem(struct interpass *ipole)
{
	struct interpass *ip;
	int tab[32], i;

	tab[0] = 0;
	DLIST_FOREACH(ip, ipole, qelem)
		if (ip->type == IP_NODE)
			walkf(ip->ip_node, lm_find, tab);
	if (tab[0] == 0)
		return;
	/*
	 * A long that lives across a call can have the one callee-saved pair.
	 * When two do, one is spilled, and a spilled long is loaded, updated and
	 * stored around each use.  Then the ones that are updated go to memory.
	 */
	lm_nseen = lm_ncalls = lm_idx = 0;
	DLIST_FOREACH(ip, ipole, qelem) {
		if (ip->type != IP_NODE)
			continue;
		lm_idx++;
		walkf(ip->ip_node, lm_count, 0);
		if (lm_hascall(ip->ip_node) && lm_ncalls < 512)
			lm_calls[lm_ncalls++] = lm_idx;
	}
	{
		int nacross = 0;

		for (i = 0; i < lm_nseen; i++)
			if (lm_across(lm_seen[i]))
				nacross++;
		if (nacross < 2)
			nacross = 0;
		else
			nacross = 1;
		if (nacross == 0)
			goto params;
	}
	for (i = 1; i <= tab[0]; i++) {
		if (tab[i] == 0 || !lm_nonconst[i] || !lm_across(tab[i]))
			continue;
		lm_demote(ipole, tab[i]);
		tab[i] = 0;
	}
params:
	/* a parameter that is updated is used in its own slot, whatever the update */
	for (i = 1; i <= tab[0]; i++)
		if (tab[i] != 0 && lm_param(ipole, tab[i]))
			lm_demote(ipole, tab[i]);
}

/* the temporary has its value at entry from a parameter slot? */
static int
lm_param(struct interpass *ipole, int reg)
{
	struct interpass *ip;
	NODE *a, *b, *x;

	DLIST_FOREACH(ip, ipole, qelem) {
		if (ip->type != IP_NODE)
			continue;
		a = ip->ip_node;
		if (a->n_op == ASSIGN && a->n_left->n_op == TEMP &&
		    regno(a->n_left) == reg &&
		    (b = a->n_right)->n_op == UMUL &&
		    (x = b->n_left)->n_op == PLUS &&
		    x->n_left->n_op == REG && regno(x->n_left) == FPREG &&
		    x->n_right->n_op == ICON && getlval(x->n_right) > 0)
			return 1;
	}
	return 0;
}

static void
lm_demote(struct interpass *ipole, int reg)
{
	{
		struct interpass *ip, *nip;
		NODE *a, *b, *x;

		lm_reg = reg;
		lm_off = 0;
		/* a parameter copied into the temporary at entry: use its slot */
		DLIST_FOREACH(ip, ipole, qelem) {
			if (ip->type != IP_NODE)
				continue;
			a = ip->ip_node;
			if (a->n_op == ASSIGN && a->n_left->n_op == TEMP &&
			    regno(a->n_left) == lm_reg &&
			    (b = a->n_right)->n_op == UMUL &&
			    (x = b->n_left)->n_op == PLUS &&
			    x->n_left->n_op == REG && regno(x->n_left) == FPREG &&
			    x->n_right->n_op == ICON && getlval(x->n_right) > 0) {
				lm_off = getlval(x->n_right);
				break;
			}
		}
		if (lm_off == 0)
			lm_off = freetemp(2);	/* two words */
		else {
			/* the copy goes: it would copy the slot onto itself */
			nip = ip;
			ip = DLIST_PREV(ip, qelem);
			tfree(nip->ip_node);
			DLIST_REMOVE(nip, qelem);
		}
		DLIST_FOREACH(ip, ipole, qelem)
			if (ip->type == IP_NODE)
				walkf(ip->ip_node, lm_replace, 0);
	}
}

/*
 * A structure of 2 or 4 bytes assigned as a statement is an int or a long
 * assignment, one or two movs, not the general block move with a counter
 * (-xdmr): *t++ = *s++ on a union of one word is  mov (r0)+,(r1)+.
 */
static void
smallstasg(struct interpass *ip)
{
	NODE *root = ip->ip_node, *st, *l, *r, *u;
	struct attr *ap;
	int sz;
	TWORD t;

	st = root;
	if (root->n_op == UMUL && root->n_left->n_op == STASG)
		st = root->n_left;
	if (st->n_op != STASG)
		return;
	if ((ap = attr_find(st->n_ap, ATTR_P2STRUCT)) == NULL)
		return;
	sz = ap->iarg(0);
	if (sz != 2 && sz != 4)
		return;
	if (ap->iarg(1) < 2)	/* needs word alignment */
		return;
	t = sz == 2 ? INT : LONG;
	l = st->n_left;
	r = st->n_right;
	l->n_type = t;
	if (r->n_op == ADDROF) {
		u = r->n_left;
		nfree(r);
		u->n_type = t;
	} else if (r->n_op == ICON && r->n_name[0] != '\0') {
		/* the address of a named object: the object itself */
		u = r;
		u->n_op = NAME;
		u->n_type = t;
	} else
		u = mkunode(UMUL, r, 0, t);
	st->n_op = ASSIGN;
	st->n_type = t;
	st->n_ap = NULL;
	st->n_left = l;
	st->n_right = u;
	if (root != st) {
		ip->ip_node = st;
		nfree(root);
	}
}

/*
 * (int)x of a long x in memory is its low word, the second of the two: use that
 * word where it is, add -10(r5),(r4), not load it into a register first.
 */
static void
lowword(NODE *p, void *arg)
{
	NODE *l, *a;

	if (p->n_op != SCONV || (p->n_type != INT && p->n_type != UNSIGNED &&
	    p->n_type != SHORT && p->n_type != USHORT))
		return;
	l = p->n_left;
	if (l->n_type != LONG && l->n_type != ULONG)
		return;
	if (l->n_op == NAME && l->n_name[0] == '\0')
		return;
	if (l->n_op == NAME) {
		setlval(l, getlval(l) + 2);
	} else if (l->n_op == UMUL && (a = l->n_left)->n_op == PLUS &&
	    a->n_right->n_op == ICON && a->n_right->n_name[0] == '\0') {
		setlval(a->n_right, getlval(a->n_right) + 2);
	} else if (l->n_op == UMUL && (a = l->n_left)->n_op == MINUS &&
	    a->n_right->n_op == ICON && a->n_right->n_name[0] == '\0') {
		setlval(a->n_right, getlval(a->n_right) - 2);
	} else
		return;
	/* the word is the node: the conversion goes */
	l->n_type = p->n_type == SHORT || p->n_type == USHORT ? INT : p->n_type;
	*p = *l;
	nfree(l);
}

/*
 * (-xdmr) A char made from a word in memory is the low byte of it, at the
 * same address (little-endian): movb 4(r5),r0, not  mov 4(r5),r0; movb r0,r0.
 */
static void
narrowload(NODE *p, void *arg)
{
	NODE *l;

	if (p->n_op != SCONV || (p->n_type != CHAR && p->n_type != UCHAR))
		return;
	l = p->n_left;
	if ((l->n_type != INT && l->n_type != UNSIGNED) ||
	    !(l->n_op == NAME || (l->n_op == OREG && !R2TEST(l->n_rval))) ||
	    ISVOL(l->n_qual))
		return;
	l->n_type = p->n_type;
	*p = *l;
	nfree(l);
}

void
myreader(struct interpass *ipole)
{
	struct interpass *ip;

#ifdef PCC_DEBUG
	if (x2debug) {
		printf("myreader before\n");
		printip(ipole);
	}
#endif
	if (xdmr)
		longmem(ipole);
	DLIST_FOREACH(ip, ipole, qelem) {
		if (ip->type != IP_NODE)
			continue;
		if (xdmr)
			smallstasg(ip);
		if (xdmr)
			walkf(ip->ip_node, lowword, 0);
		if (xdmr)
			walkf(ip->ip_node, narrowload, 0);
		walkf(ip->ip_node, fixops, 0);
		canon(ip->ip_node); /* call it early */
		if (xdmr) {
			byterw(ip->ip_node);
			wordrw(ip->ip_node);
			longrw(ip->ip_node);
		}
	}
	if (xdmr) {
		fwdauto(ipole);
		parmprop(ipole);
	}
#ifdef PCC_DEBUG
	if (x2debug) {
		printf("myreader middle\n");
		printip(ipole);
	}
#endif
#if 0
	DLIST_FOREACH(ip, ipole, qelem) {
		if (ip->type == IP_NODE)
			cvtree(ip->ip_node, ip);
	}
#endif
#ifdef PCC_DEBUG
	if (x2debug) {
		printf("myreader after\n");
		printip(ipole);
	}
#endif
}

/*
 * Remove SCONVs where the left node is an OREG with a smaller type.
 */
static void
delsconv(NODE *p, void *arg)
{
#if 0
	NODE *l;

	if (p->n_op != SCONV || (l = p->n_left)->n_op != OREG)
		return;
	if (DEUNSIGN(p->n_type) == INT && DEUNSIGN(l->n_type) == LONG) {
		p->n_op = OREG;
		p->n_lval = l->n_lval; /* high word */
		p->n_rval = l->n_rval;
		nfree(l);
	}
#endif
	/* Could do this for char etc. also */
}

void
mycanon(NODE *p)
{
	walkf(p, delsconv, 0);
}

void
myoptim(struct interpass *ip)
{
}

void
rmove(int s, int d, TWORD t)
{
	if (t < LONG || t > BTMASK) {
		printf("mov%s	%s,%s\n", t < SHORT ? "b" : "",
		    rnames[s],rnames[d]); /* XXX char should be full reg? */
	} else if (t == LONG || t == ULONG) {
		/* avoid trashing double regs */
		if (d > s)
			printf("mov	r%c,r%c\nmov	r%c,r%c\n",
			    rnames[s][2],rnames[d][2],
			    rnames[s][1],rnames[d][1]);
		else
			printf("mov	r%c,r%c\nmov	r%c,r%c\n",
			    rnames[s][1],rnames[d][1],
			    rnames[s][2],rnames[d][2]);
	} else if (t == FLOAT || t == DOUBLE) {
		printf("movf	%s,%s\n", rnames[s],rnames[d]);
	} else
		comperr("bad float rmove: %d %d %x", s, d, t);

}

/*
 * For class c, find worst-case displacement of the number of
 * registers in the array r[] indexed by class.
 */
int
COLORMAP(int c, int *r)
{
	switch (c) {
	case CLASSA:
		return (r[CLASSB] * 2 + r[CLASSA]) < 5;
	case CLASSB:
		if (r[CLASSB] > 1) return 0;
		if (r[CLASSB] == 1 && r[CLASSA] > 0) return 0;
		if (r[CLASSA] > 2) return 0;
		return 1;
	case CLASSC:
		return r[CLASSC] < 4;
	}
	return 0;
}

char *rnames[] = {
	"r0", "r1", "r2", "r3", "r4", "r5", "sp", "pc",
	"r01", "r12", "r23", "r34", "XXX", "XXX", "XXX", "XXX",
	"fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "XXX", "XXX",
};

/*
 * Return a class suitable for a specific type.
 */
int
gclass(TWORD t)
{
	if (t < LONG || t > BTMASK)
		return CLASSA;
	if (t == LONG || t == ULONG)
		return CLASSB;
	if (t == FLOAT || t == DOUBLE || t == LDOUBLE)
		return CLASSC;
	comperr("gclass");
	return CLASSD;
}

static int
argsiz(NODE *p)  
{
	TWORD t = p->n_type;

	if (t == LONG || t == ULONG || t == FLOAT)
		return 4;
	if (t == DOUBLE)
		return 8;
	if (t == STRTY || t == UNIONTY)
		return attr_find(p->n_ap, ATTR_P2STRUCT)->iarg(0);
	return 2;
}

/*
 * Argument specialties.
 */
void
lastcall(NODE *p)
{
	NODE *op = p;
	int size = 0;

	/*
	 * Calculate arg sizes.
	 * Mark first arg not to have - before it.
	 */
	p->n_qual = 0;
	if (p->n_op != CALL && p->n_op != FORTCALL && p->n_op != STCALL)
		return;
	for (p = p->n_right; p->n_op == CM; p = p->n_left) {
		p->n_right->n_qual = 0;
		size += argsiz(p->n_right);
	}
	p->n_qual = 0;
	size += argsiz(p);
	p = op->n_right;

	if (p->n_op == CM)
		p = p->n_right;
#if 0 /* XXX fixme */
	if (p->n_type == FLOAT || p->n_type == DOUBLE ||
	    p->n_type == STRTY || p->n_type == UNIONTY)
		op->n_flags |= NLOCAL1;	/* Does not use stack slot */
	else
		op->n_flags &= ~NLOCAL1;
#endif
	op->n_qual = size; /* XXX */
}

static int
is1con(NODE *p)
{
	if (p->n_op == ICON && getlval(p) == 1)
		return 1;
	return 0;
}

/*
 * Special shapes.
 */
int
special(NODE *p, int shape)
{
	CONSZ s;

	switch (shape) {
	case SDIVCON:
		if (p->n_op == ICON && p->n_name[0] == '\0' &&
		    getlval(p) >= 2 && getlval(p) <= 32767)
			return SRDIR;
		break;
	case SANDSCON:
		s = ~getlval(p);
		if (s < 65536 || s > -65537)
			return SRDIR;
		break;
	case SINCB: /* Check if subject for post-inc */
		if (p->n_op == ASSIGN && p->n_right->n_op == PLUS &&
		    treecmp(p->n_left, p->n_right->n_left) &&
		    is1con(p->n_right->n_right))
			return SRDIR;
		break;
	case SSTARX:
		if (xdmr && p->n_op == UMUL && p->n_left->n_op == NAME &&
		    p->n_left->n_name[0] != '\0')
			return SRDIR;
		break;
	case SARGSUB:
		if (p->n_op == MINUS && p->n_right->n_op == ICON &&
		    p->n_left->n_op == REG)
			return SRDIR;
		/* &local: the frame pointer plus a constant (-xdmr) */
		if (xdmr && p->n_op == PLUS && p->n_right->n_op == ICON &&
		    p->n_right->n_name[0] == '\0' && p->n_left->n_op == REG &&
		    regno(p->n_left) == FPREG)
			return SRDIR;
		break;
	case SARGINC:
		if (p->n_op == MINUS && is1con(p->n_right))
			return special(p->n_left, SINCB);
		break;
	}
	return SRNOPE;
}

/*
 * Target-dependent command-line options.
 */
void
mflags(char *str)
{
}

/*
 * Do something target-dependent for xasm arguments.
 */
int
myxasm(struct interpass *ip, NODE *p)
{
	return 0;
}

int
fldexpand(NODE *p, int cookie, char **cp)
{
	return 0;
}

