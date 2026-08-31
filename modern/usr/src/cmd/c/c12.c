#
/*
 *		C compiler part 2 -- expression optimizer
 *
 */

#include "c1.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>

/*
 * PDP-11 floating point (V7's /usr/src/libc fakfp.s etc.): a 64-bit double is
 * 4 words, word 0 = sign(1) + exponent(8, excess-128) + high 7 mantissa bits,
 * words 1-3 the low 48 mantissa bits; the implicit leading bit makes the
 * mantissa a fraction in [0.5, 1.0).  A 32-bit float is the same with 2 words
 * (23 explicit mantissa bits).  We convert the host's IEEE-754 double *by
 * value* via frexp/ldexp, so the result is independent of the host's byte
 * order and of whether its double is IEEE-754.
 */
void pdp11_double(double d, uint16_t w[4])
{
	int e, sign;
	double m;
	uint64_t M;

	sign = 0;
	if (d < 0) { sign = 1; d = -d; }
	if (d == 0) {
		w[0] = (uint16_t)(sign << 15);	/* -0.0 keeps its sign bit */
		w[1] = w[2] = w[3] = 0;
		return;
	}
	m = frexp(d, &e);		/* m in [0.5, 1.0) */
	M = (uint64_t)ldexp(m, 56);	/* 56-bit mantissa, leading 1 in bit 55 */
	w[0] = (uint16_t)((sign << 15) | (((e + 128) & 0xFF) << 7) | ((M >> 48) & 0x7F));
	w[1] = (uint16_t)((M >> 32) & 0xFFFF);
	w[2] = (uint16_t)((M >> 16) & 0xFFFF);
	w[3] = (uint16_t)(M & 0xFFFF);
}

void pdp11_float(double d, uint16_t w[2])
{
	int e, sign;
	double m;
	uint32_t M;

	sign = 0;
	if (d < 0) { sign = 1; d = -d; }
	if (d == 0) {
		w[0] = (uint16_t)(sign << 15);
		w[1] = 0;
		return;
	}
	m = frexp(d, &e);
	M = (uint32_t)ldexp(m, 24);	/* 24-bit mantissa, leading 1 in bit 23 */
	w[0] = (uint16_t)((sign << 15) | (((e + 128) & 0xFF) << 7) | ((M >> 16) & 0x7F));
	w[1] = (uint16_t)(M & 0xFFFF);
}

struct node *acommute(struct node *atree);
void c1_const(int16_t op, int16_t *vp, int16_t av);
void distrib(struct acl *list);
void *getblk(int16_t size);
struct node *hardlongs(struct node *at);
void insert(int16_t op, struct node *atree, struct acl *alist);
struct node *isconstant(struct node *at);
int16_t islong(int16_t t);
struct node * lconst(int16_t op, struct node *lp, struct node *rp);
struct node *lvfield(struct node *at);
int16_t squash(struct node **p, struct node **maxp);
struct node *tconst(int16_t val, int16_t type);
struct node *tnode(int16_t op, int16_t type, struct node *tr1, struct node *tr2);
struct node *unoptim(struct node *atree);

struct node *optim(struct node *atree)
{
	register int16_t op, dope;
	int16_t d1, d2;
	struct node *t;
	register struct node *tree;

	if ((tree=atree)==0)
		return(0);
	if ((op = tree->op)==0)
		return(tree);
	if (op==NAME && tree->u.tname.class==AUTO) {
		tree->u.tname.class = OFFS;
		tree->u.tname.regno = 5;
		tree->u.tname.offset = tree->u.tname.nloc;
	}
	dope = opdope[op];
	if ((dope&LEAF) != 0) {
		if (op==FCON) {
			uint16_t w[4];
			pdp11_double(tree->u.ftconst.fvalue, w);
			if (w[1]==0 && w[2]==0 && w[3]==0) {
				tree->op = SFCON;
				tree->u.tconst.value = (int16_t)w[0];
			}
		}
		return(tree);
	}
	if ((dope&BINARY) == 0)
		return(unoptim(tree));
	/* is known to be binary */
	if (tree->type==CHAR)
		tree->type = INT;
	switch(op) {
	/*
	 * PDP-11 special:
	 * generate new =&~ operator out of =&
	 * by complementing the RHS.
	 */
	case ASAND:
		tree->op = ASANDN;
		tree->u.tnode.tr2 = tnode(COMPL, tree->u.tnode.tr2->type, tree->u.tnode.tr2, NULL);
		break;

	/*
	 * On the PDP-11, int->ptr via multiplication
	 * Longs are just truncated.
	 */
	case LTOP:
		tree->op = ITOP;
		tree->u.tnode.tr1 = unoptim(tnode(LTOI,INT,tree->u.tnode.tr1, NULL));
	case ITOP:
		tree->op = TIMES;
		break;

	case MINUS:
		if ((t = isconstant(tree->u.tnode.tr2)) && (t->type!=UNSIGN || tree->type!=LONG)) {
			tree->op = PLUS;
			if (t->type==DOUBLE)
				/* PDP-11 FP representation */
				t->u.tconst.value ^= 0100000;
			else
				t->u.tconst.value = -t->u.tconst.value;
		}
		break;
	}
	op = tree->op;
	dope = opdope[op];
	if (dope&LVALUE && tree->u.tnode.tr1->op==FSEL)
		return(lvfield(tree));
	if ((dope&COMMUTE)!=0) {
		d1 = tree->type;
		tree = acommute(tree);
		if (tree->op == op)
			tree->type = d1;
		/*
		 * PDP-11 special:
		 * replace a&b by a ANDN ~ b.
		 * This will be undone when in
		 * truth-value context.
		 */
		if (tree->op!=AND)
			return(tree);
		/*
		 * long & pos-int is simpler
		 */
		if (tree->type==LONG && tree->u.tnode.tr2->op==ITOL
		 && (tree->u.tnode.tr2->u.tnode.tr1->op==CON && tree->u.tnode.tr2->u.tnode.tr1->u.tconst.value>=0
		   || tree->u.tnode.tr2->u.tnode.tr1->type==UNSIGN)) {
			tree->type = UNSIGN;
			t = tree->u.tnode.tr2;
			tree->u.tnode.tr2 = tree->u.tnode.tr2->u.tnode.tr1;
			t->u.tnode.tr1 = tree;
			tree->u.tnode.tr1 = tnode(LTOI, UNSIGN, tree->u.tnode.tr1, NULL);
			return(optim(t));
		}
		/*
		 * Keep constants to the right
		 */
		if ((tree->u.tnode.tr1->op==ITOL && tree->u.tnode.tr1->u.tnode.tr1->op==CON)
		  || tree->u.tnode.tr1->op==LCON) {
			t = tree->u.tnode.tr1;
			tree->u.tnode.tr1 = tree->u.tnode.tr2;
			tree->u.tnode.tr2 = t;
		}
		tree->op = ANDN;
		op = ANDN;
		tree->u.tnode.tr2 = tnode(COMPL, tree->u.tnode.tr2->type, tree->u.tnode.tr2, NULL);
	}
    again:
	tree->u.tnode.tr1 = optim(tree->u.tnode.tr1);
	tree->u.tnode.tr2 = optim(tree->u.tnode.tr2);
	if (tree->type == LONG) {
		t = lconst(tree->op, tree->u.tnode.tr1, tree->u.tnode.tr2);
		if (t)
			return(t);
	}
	if ((dope&RELAT) != 0) {
		if ((d1=degree(tree->u.tnode.tr1)) < (d2=degree(tree->u.tnode.tr2))
		 || d1==d2 && tree->u.tnode.tr1->op==NAME && tree->u.tnode.tr2->op!=NAME) {
			t = tree->u.tnode.tr1;
			tree->u.tnode.tr1 = tree->u.tnode.tr2;
			tree->u.tnode.tr2 = t;
			tree->op = maprel[op-EQUAL];
		}
		if (tree->u.tnode.tr1->type==CHAR && tree->u.tnode.tr2->op==CON
		 && (dcalc(tree->u.tnode.tr1, 0) <= 12 || tree->u.tnode.tr1->op==STAR)
		 && tree->u.tnode.tr2->u.tconst.value <= 127 && tree->u.tnode.tr2->u.tconst.value >= 0)
			tree->u.tnode.tr2->type = CHAR;
	}
	d1 = max(degree(tree->u.tnode.tr1), islong(tree->type));
	d2 = max(degree(tree->u.tnode.tr2), 0);
	switch (op) {

	/*
	 * In assignment to fields, treat all-zero and all-1 specially.
	 */
	case FSELA:
		if (tree->u.tnode.tr2->op==CON && tree->u.tnode.tr2->u.tconst.value==0) {
			tree->op = ASAND;
			tree->u.tnode.tr2->u.tconst.value = ~tree->u.fasgn.mask;
			return(optim(tree));
		}
		if (tree->u.tnode.tr2->op==CON && tree->u.fasgn.mask==tree->u.tnode.tr2->u.tconst.value) {
			tree->op = ASOR;
			return(optim(tree));
		}

	case LTIMES:
	case LDIV:
	case LMOD:
	case LASTIMES:
	case LASDIV:
	case LASMOD:
		tree->u.tnode.degree = 10;
		break;

	case ANDN:
		if (isconstant(tree->u.tnode.tr2) && tree->u.tnode.tr2->u.tconst.value==0) {
			return(tree->u.tnode.tr1);
		}
		goto def;

	case CALL:
		tree->u.tnode.degree = 10;
		break;

	case QUEST:
	case COLON:
		tree->u.tnode.degree = max(d1, d2);
		break;

	case DIVIDE:
	case ASDIV:
	case ASTIMES:
	case PTOI:
		if (tree->u.tnode.tr2->op==CON && tree->u.tnode.tr2->u.tconst.value==1)
			return(tree->u.tnode.tr1);
	case MOD:
	case ASMOD:
		if (tree->u.tnode.tr1->type==UNSIGN && ispow2(tree))
			return(pow2(tree));
		if ((op==MOD||op==ASMOD) && tree->type==DOUBLE) {
			error("Floating %% not defined");
			tree->type = INT;
		}
	case ULSH:
	case ASULSH:
		d1 += 2;
		d2 += 2;
		if (tree->type==LONG)
			return(hardlongs(tree));
		goto constant;

	case LSHIFT:
	case RSHIFT:
	case ASRSH:
	case ASLSH:
		if (tree->u.tnode.tr2->op==CON && tree->u.tnode.tr2->u.tconst.value==0) {
			return(tree->u.tnode.tr1);
		}
		/*
		 * PDP-11 special: turn right shifts into negative
		 * left shifts
		 */
		if (tree->type == LONG) {
			d1++;
			d2++;
		}
		if (op==LSHIFT||op==ASLSH)
			goto constant;
		if (tree->u.tnode.tr2->op==CON && tree->u.tnode.tr2->u.tconst.value==1
		 && tree->u.tnode.tr1->type!=UNSIGN)
			goto constant;
		op += (LSHIFT-RSHIFT);
		tree->op = op;
		tree->u.tnode.tr2 = tnode(NEG, tree->type, tree->u.tnode.tr2, NULL);
		if (tree->u.tnode.tr1->type==UNSIGN) {
			if (tree->op==LSHIFT)
				tree->op = ULSH;
			else if (tree->op==ASLSH)
				tree->op = ASULSH;
		}
		goto again;

	constant:
		if (tree->u.tnode.tr1->op==CON && tree->u.tnode.tr2->op==CON) {
			c1_const(op, &tree->u.tnode.tr1->u.tconst.value, tree->u.tnode.tr2->u.tconst.value);
			return(tree->u.tnode.tr1);
		}


	def:
	default:
		if (dope&RELAT) {
			if (tree->u.tnode.tr1->type==LONG)	/* long relations are a mess */
				d1 = 10;
			if (opdope[tree->u.tnode.tr1->op]&RELAT && tree->u.tnode.tr2->op==CON
			 && tree->u.tnode.tr2->u.tconst.value==0) {
				tree = tree->u.tnode.tr1;
				switch(op) {
				case GREATEQ:
					return(&cone);
				case LESS:
					return(&czero);
				case LESSEQ:
				case EQUAL:
					tree->op = notrel[tree->op-EQUAL];
				}
				return(tree);
			}
		}
		tree->u.tnode.degree = d1==d2? d1+islong(tree->type): max(d1, d2);
		break;
	}
	return(tree);
}

struct node *unoptim(struct node *atree)
{
	register struct node *subtre, *tree;
	register struct node *p;
	double static fv;
	struct node *fp;

	if ((tree=atree)==0)
		return(0);
    again:
	if (tree->op==AMPER && tree->u.tnode.tr1->op==STAR) {
		subtre = tree->u.tnode.tr1->u.tnode.tr1;
		subtre->type = tree->type;
		return(optim(subtre));
	}
	subtre = tree->u.tnode.tr1 = optim(tree->u.tnode.tr1);
	switch (tree->op) {

	case ITOL:
		if (subtre->op==CON && subtre->type==INT && subtre->u.tconst.value<0) {
			subtre = getblk(sizeof(struct node));
			subtre->op = LCON;
			subtre->type = LONG;
			subtre->u.lconst.lvalue = tree->u.tnode.tr1->u.tconst.value;
			return(subtre);
		}
		break;

	case FTOI:
		if (tree->type==UNSIGN) {
			tree->op = FTOL;
			tree->type = LONG;
			tree = tnode(LTOI, UNSIGN, tree, NULL);
		}
		break;

	case LTOF:
		if (subtre->op==LCON) {
			tree = getblk(sizeof(*fp));
			tree->op = FCON;
			tree->type = DOUBLE;
			tree->u.tconst.value = isn++;
			tree->u.ftconst.fvalue = subtre->u.lconst.lvalue;
			return(optim(tree));
		}
		break;

	case ITOF:
		if (tree->u.tnode.tr1->type==UNSIGN) {
			tree->u.tnode.tr1 = tnode(ITOL, LONG, tree->u.tnode.tr1, NULL);
			tree->op = LTOF;
			tree = optim(tree);
		}
		if (subtre->op!=CON)
			break;
		fv = subtre->u.tconst.value;
		{
			uint16_t w[4];
			pdp11_double(fv, w);
			if (w[1]==0 && w[2]==0 && w[3]==0) {
				tree = getblk(sizeof(*fp));
				tree->op = SFCON;
				tree->type = DOUBLE;
				tree->u.tconst.value = (int16_t)w[0];
				tree->u.ftconst.fvalue = fv;
				return(tree);
			}
		}
		break;

	case ITOC:
		p = tree->u.tnode.tr1;
		/*
		 * Sign-extend PDP-11 characters
		 */
		if (p->op==CON) {
			p->u.tconst.value = p->u.tconst.value << 8 >> 8;
			return(p);
		} else if (p->op==NAME) {
			p->type = CHAR;
			return(p);
		}
		break;

	case LTOI:
		p = tree->u.tnode.tr1;
		switch (p->op) {

		case LCON:
			p->op = CON;
			p->type = tree->type;
			p->u.tconst.value = p->u.lconst.lvalue;
			return(p);

		case NAME:
			p->u.tname.offset += 2;
			p->type = tree->type;
			return(p);

		case STAR:
			p->type = tree->type;
			p->u.tnode.tr1->type = tree->type+PTR;
			p->u.tnode.tr1 = tnode(PLUS, tree->type, p->u.tnode.tr1, tconst(2, INT));
			return(optim(p));

		case ITOL:
			return(p->u.tnode.tr1);

		case PLUS:
		case MINUS:
		case AND:
		case ANDN:
		case OR:
		case EXOR:
			p->u.tnode.tr2 = tnode(LTOI, tree->type, p->u.tnode.tr2, NULL);
		case NEG:
		case COMPL:
			p->u.tnode.tr1 = tnode(LTOI, tree->type, p->u.tnode.tr1, NULL);
			p->type = tree->type;
			return(optim(p));
		}
		break;

	case FSEL:
		tree->op = AND;
		tree->u.tnode.tr1 = tree->u.tnode.tr2->u.tnode.tr1;
		tree->u.tnode.tr2->u.tnode.tr1 = subtre;
		tree->u.tnode.tr2->op = RSHIFT;
		tree->u.tnode.tr1->u.tconst.value = (1 << tree->u.tnode.tr1->u.tconst.value) - 1;
		return(optim(tree));

	case FSELR:
		tree->op = LSHIFT;
		tree->type = UNSIGN;
		tree->u.tnode.tr1 = tree->u.tnode.tr2;
		tree->u.tnode.tr1->op = AND;
		tree->u.tnode.tr2 = tree->u.tnode.tr2->u.tnode.tr2;
		tree->u.tnode.tr1->u.tnode.tr2 = subtre;
		tree->u.tnode.tr1->u.tnode.tr1->u.tconst.value = (1 << tree->u.tnode.tr1->u.tnode.tr1->u.tconst.value) -1;
		return(optim(tree));

	case AMPER:
		if (subtre->op==STAR)
			return(subtre->u.tnode.tr1);
		if (subtre->op==NAME && subtre->u.tname.class == OFFS) {
			p = tnode(PLUS, tree->type, subtre, tree);
			subtre->type = tree->type;
			tree->op = CON;
			tree->type = INT;
			tree->u.tnode.degree = 0;
			tree->u.tconst.value = subtre->u.tname.offset;
			subtre->u.tname.class = REG;
			subtre->u.tname.nloc = subtre->u.tname.regno;
			subtre->u.tname.offset = 0;
			return(optim(p));
		}
		break;

	case STAR:
		if (subtre->op==AMPER) {
			subtre->u.tnode.tr1->type = tree->type;
			return(subtre->u.tnode.tr1);
		}
		if (tree->type==STRUCT)
			break;
		if (subtre->op==NAME && subtre->u.tname.class==REG) {
			subtre->type = tree->type;
			subtre->u.tname.class = OFFS;
			subtre->u.tname.regno = subtre->u.tname.nloc;
			return(subtre);
		}
		p = subtre->u.tnode.tr1;
		if ((subtre->op==INCAFT||subtre->op==DECBEF)&&tree->type!=LONG
		 && p->op==NAME && p->u.tname.class==REG && p->type==subtre->type) {
			p->type = tree->type;
			p->op = subtre->op==INCAFT? AUTOI: AUTOD;
			return(p);
		}
		if (subtre->op==PLUS && p->op==NAME && p->u.tname.class==REG) {
			if (subtre->u.tnode.tr2->op==CON) {
				p->u.tname.offset += subtre->u.tnode.tr2->u.tconst.value;
				p->u.tname.class = OFFS;
				p->type = tree->type;
				p->u.tname.regno = p->u.tname.nloc;
				return(p);
			}
			if (subtre->u.tnode.tr2->op==AMPER) {
				subtre = subtre->u.tnode.tr2->u.tnode.tr1;
				subtre->u.tname.class += XOFFS-EXTERN;
				subtre->u.tname.regno = p->u.tname.nloc;
				subtre->type = tree->type;
				return(subtre);
			}
		}
		break;
	case EXCLA:
		if ((opdope[subtre->op]&RELAT)==0)
			break;
		tree = subtre;
		tree->op = notrel[tree->op-EQUAL];
		break;

	case COMPL:
		if (tree->type==CHAR)
			tree->type = INT;
		if (tree->op == subtre->op)
			return(subtre->u.tnode.tr1);
		if (subtre->op==CON) {
			subtre->u.tconst.value = ~subtre->u.tconst.value;
			return(subtre);
		}
		if (subtre->op==LCON) {
			subtre->u.lconst.lvalue = ~subtre->u.lconst.lvalue;
			return(subtre);
		}
		if (subtre->op==ITOL) {
			if (subtre->u.tnode.tr1->op==CON) {
				tree = getblk(sizeof(struct node));
				tree->op = LCON;
				tree->type = LONG;
				if (subtre->u.tnode.tr1->type==UNSIGN)
					tree->u.lconst.lvalue = ~(int32_t)(unsigned)subtre->u.tnode.tr1->u.tconst.value;
				else
					tree->u.lconst.lvalue = ~subtre->u.tnode.tr1->u.tconst.value;
				return(tree);
			}
			if (subtre->u.tnode.tr1->type==UNSIGN)
				break;
			subtre->op = tree->op;
			subtre->type = subtre->u.tnode.tr1->type;
			tree->op = ITOL;
			tree->type = LONG;
			goto again;
		}

	case NEG:
		if (tree->type==CHAR)
			tree->type = INT;
		if (tree->op==subtre->op)
			return(subtre->u.tnode.tr1);
		if (subtre->op==CON) {
			subtre->u.tconst.value = -subtre->u.tconst.value;
			return(subtre);
		}
		if (subtre->op==LCON) {
			subtre->u.lconst.lvalue = -subtre->u.lconst.lvalue;
			return(subtre);
		}
		if (subtre->op==ITOL && subtre->u.tnode.tr1->op==CON) {
			tree = getblk(sizeof(struct node));
			tree->op = LCON;
			tree->type = LONG;
			if (subtre->u.tnode.tr1->type==UNSIGN)
				tree->u.lconst.lvalue = -(int32_t)(unsigned)subtre->u.tnode.tr1->u.tconst.value;
			else
				tree->u.lconst.lvalue = -subtre->u.tnode.tr1->u.tconst.value;
			return(tree);
		}
		/*
		 * PDP-11 FP negation
		 */
		if (subtre->op==SFCON) {
			subtre->u.tconst.value ^= 0100000;
			subtre->u.ftconst.fvalue = -subtre->u.ftconst.fvalue;
			return(subtre);
		}
		if (subtre->op==FCON) {
			subtre->u.ftconst.fvalue = -subtre->u.ftconst.fvalue;
			return(subtre);
		}
	}
	if ((opdope[tree->op]&LEAF)==0)
		tree->u.tnode.degree = max(islong(tree->type), degree(subtre));
	return(tree);
}

/*
 * Deal with assignments to partial-word fields.
 * The game is that select(x) =+ y turns into
 * select(x =+ select(y)) where the shifts and masks
 * are chosen properly.  The outer select
 * is discarded where the value doesn't matter.
 * Sadly, overflow is undetected on =+ and the like.
 * Pure assignment is handled specially.
 */

struct node *lvfield(struct node *at)
{
	register struct node *t, *t1;
	register struct node *t2;

	t = at;
	switch (t->op) {

	case ASSIGN:
		t2 = getblk(sizeof(*t2));
		t2->op = FSELA;
		t2->type = UNSIGN;
		t1 = t->u.tnode.tr1->u.tnode.tr2;
		t2->u.fasgn.mask = ((1<<t1->u.tnode.tr1->u.tconst.value)-1)<<t1->u.tnode.tr2->u.tconst.value;
		t2->u.tnode.tr1 = t->u.tnode.tr1;
		t2->u.tnode.tr2 = t->u.tnode.tr2;
		t = t2;

	case ASANDN:
	case ASPLUS:
	case ASMINUS:
	case ASOR:
	case ASXOR:
	case INCBEF:
	case INCAFT:
	case DECBEF:
	case DECAFT:
		t1 = t->u.tnode.tr1;
		t1->op = FSELR;
		t->u.tnode.tr1 = t1->u.tnode.tr1;
		t1->u.tnode.tr1 = t->u.tnode.tr2;
		t->u.tnode.tr2 = t1;
		t1 = t1->u.tnode.tr2;
		t1 = tnode(COMMA, INT, tconst(t1->u.tnode.tr1->u.tconst.value, INT),
			tconst(t1->u.tnode.tr2->u.tconst.value, INT));
		return(optim(tnode(FSELT, UNSIGN, t, t1)));

	}
	error("Unimplemented field operator");
	return(t);
}

enum { LSTSIZ = 20 };
struct acl {
	int16_t nextl;
	int16_t nextn;
	struct node *nlist[LSTSIZ];
	struct node *llist[LSTSIZ+1];
};

struct node *acommute(struct node *atree)
{
	struct acl acl;
	int16_t d, i, op, flt, d1;
	register struct node *t1, **t2, *tree;
	struct node *t;

	acl.nextl = 0;
	acl.nextn = 0;
	tree = atree;
	op = tree->op;
	flt = isfloat(tree);
	insert(op, tree, &acl);
	acl.nextl--;
	t2 = &acl.llist[acl.nextl];
	if (!flt) {
		/* put constants together */
		for (i=acl.nextl; i>0; i--) {
			if (t2[0]->op==CON && t2[-1]->op==CON) {
				acl.nextl--;
				t2--;
				c1_const(op, &t2[0]->u.tconst.value, t2[1]->u.tconst.value);
			} else if (t = lconst(op, t2[-1], t2[0])) {
				acl.nextl--;
				t2--;
				t2[0] = t;
			}
		}
	}
	if (op==PLUS || op==OR) {
		/* toss out "+0" */
		if (acl.nextl>0 && (t1 = isconstant(*t2)) && t1->u.tconst.value==0
		 || (*t2)->op==LCON && (*t2)->u.lconst.lvalue==0) {
			acl.nextl--;
			t2--;
		}
		if (acl.nextl <= 0) {
			if ((*t2)->type==CHAR)
				*t2 = tnode(LOAD, tree->type, *t2, NULL);
			(*t2)->type = tree->type;
			return(*t2);
		}
		/* subsume constant in "&x+c" */
		if (op==PLUS && t2[0]->op==CON && t2[-1]->op==AMPER) {
			t2--;
			t2[0]->u.tnode.tr1->u.tname.offset += t2[1]->u.tconst.value;
			acl.nextl--;
		}
	} else if (op==TIMES || op==AND) {
		t1 = acl.llist[acl.nextl];
		if (t1->op==CON) {
			if (t1->u.tconst.value==0)
				return(t1);
			if (op==TIMES && t1->u.tconst.value==1 && acl.nextl>0)
				if (--acl.nextl <= 0) {
					t1 = acl.llist[0];
					if (tree->type == UNSIGN)
						t1->type = tree->type;
					return(t1);
				}
		}
	}
	if (op==PLUS && !flt)
		distrib(&acl);
	tree = *(t2 = &acl.llist[0]);
	d = max(degree(tree), islong(tree->type));
	if (op==TIMES && !flt)
		d++;
	for (i=0; i<acl.nextl; i++) {
		t1 = acl.nlist[i];
		t1->u.tnode.tr2 = t = *++t2;
		d1 = degree(t);
		/*
		 * PDP-11 strangeness:
		 * rt. op of ^ must be in a register.
		 */
		if (op==EXOR && dcalc(t, 0)<=12) {
			t1->u.tnode.tr2 = t = optim(tnode(LOAD, t->type, t, NULL));
			d1 = t->u.tnode.degree;
		}
		t1->u.tnode.degree = d = d==d1? d+islong(t1->type): max(d, d1);
		t1->u.tnode.tr1 = tree;
		tree = t1;
		if (tree->type==LONG) {
			if (tree->op==TIMES)
				tree = hardlongs(tree);
			else if (tree->op==PLUS && (t = isconstant(tree->u.tnode.tr1))
			       && t->u.tconst.value < 0 && t->type!=UNSIGN) {
				tree->op = MINUS;
				t->u.tconst.value = - t->u.tconst.value;
				t = tree->u.tnode.tr1;
				tree->u.tnode.tr1 = tree->u.tnode.tr2;
				tree->u.tnode.tr2 = t;
			}
		}
	}
	if (tree->op==TIMES && ispow2(tree))
		tree->u.tnode.degree = max(degree(tree->u.tnode.tr1), islong(tree->type));
	return(tree);
}

void distrib(struct acl *list)
{
/*
 * Find a list member of the form c1c2*x such
 * that c1c2 divides no other such constant, is divided by
 * at least one other (say in the form c1*y), and which has
 * fewest divisors. Reduce this pair to c1*(y+c2*x)
 * and iterate until no reductions occur.
 */
	register struct node **p1, **p2;
	struct node *t;
	int16_t ndmaj, ndmin;
	struct node **dividend, **divisor;
	struct node **maxnod, **mindiv;

    loop:
	maxnod = &list->llist[list->nextl];
	ndmaj = 1000;
	dividend = 0;
	for (p1 = list->llist; p1 <= maxnod; p1++) {
		if ((*p1)->op!=TIMES || (*p1)->u.tnode.tr2->op!=CON)
			continue;
		ndmin = 0;
		for (p2 = list->llist; p2 <= maxnod; p2++) {
			if (p1==p2 || (*p2)->op!=TIMES || (*p2)->u.tnode.tr2->op!=CON)
				continue;
			if ((*p1)->u.tnode.tr2->u.tconst.value == (*p2)->u.tnode.tr2->u.tconst.value) {
				(*p2)->u.tnode.tr2 = (*p1)->u.tnode.tr1;
				(*p2)->op = PLUS;
				(*p1)->u.tnode.tr1 = (*p2);
				*p1 = optim(*p1);
				squash(p2, maxnod);
				list->nextl--;
				goto loop;
			}
			if (((*p2)->u.tnode.tr2->u.tconst.value % (*p1)->u.tnode.tr2->u.tconst.value) == 0)
				goto contmaj;
			if (((*p1)->u.tnode.tr2->u.tconst.value % (*p2)->u.tnode.tr2->u.tconst.value) == 0) {
				ndmin++;
				mindiv = p2;
			}
		}
		if (ndmin > 0 && ndmin < ndmaj) {
			ndmaj = ndmin;
			dividend = p1;
			divisor = mindiv;
		}
    contmaj:;
	}
	if (dividend==0)
		return;
	t = list->nlist[--list->nextn];
	p1 = dividend;
	p2 = divisor;
	t->op = PLUS;
	t->type = (*p1)->type;
	t->u.tnode.tr1 = (*p1);
	t->u.tnode.tr2 = (*p2)->u.tnode.tr1;
	(*p1)->u.tnode.tr2->u.tconst.value /= (*p2)->u.tnode.tr2->u.tconst.value;
	(*p2)->u.tnode.tr1 = t;
	t = optim(*p2);
	if (p1 < p2) {
		*p1 = t;
		squash(p2, maxnod);
	} else {
		*p2 = t;
		squash(p1, maxnod);
	}
	list->nextl--;
	goto loop;
}

int16_t squash(struct node **p, struct node **maxp)
{
	register struct node **np;

	for (np = p; np < maxp; np++)
		*np = *(np+1);
}

void c1_const(int16_t op, int16_t *vp, int16_t av)
{
	register int16_t v;

	v = av;
	switch (op) {

	case PTOI:
		*(uint16_t *)vp /= v;
		return;

	case PLUS:
		*vp += v;
		return;

	case TIMES:
		*vp *= v;
		return;

	case AND:
		*vp &= v;
		return;

	case OR:
		*vp |= v;
		return;

	case EXOR:
		*vp ^= v;
		return;

	case DIVIDE:
	case MOD:
		if (v==0)
			error("Divide check");
		else
			if (op==DIVIDE)
				*vp /= v;
			else
				*vp %= v;
		return;

	case RSHIFT:
		*vp >>= v;
		return;

	case LSHIFT:
		*vp <<= v;
		return;

	case ANDN:
		*vp &= ~ v;
		return;
	}
	error("C error: const");
}

struct node * lconst(int16_t op, struct node *lp, struct node *rp)
{
	int32_t l, r;

	if (lp->op==LCON)
		l = lp->u.lconst.lvalue;
	else if (lp->op==ITOL && lp->u.tnode.tr1->op==CON) {
		if (lp->u.tnode.tr1->type==INT)
			l = lp->u.tnode.tr1->u.tconst.value;
		else
			l = (unsigned)lp->u.tnode.tr1->u.tconst.value;
	} else
		return(0);
	if (rp->op==LCON)
		r = rp->u.lconst.lvalue;
	else if (rp->op==ITOL && rp->u.tnode.tr1->op==CON) {
		if (rp->u.tnode.tr1->type==INT)
			r = rp->u.tnode.tr1->u.tconst.value;
		else
			r = (unsigned)rp->u.tnode.tr1->u.tconst.value;
	} else
		return(0);
	switch (op) {

	case PLUS:
		l += r;
		break;

	case MINUS:
		l -= r;
		break;

	case TIMES:
	case LTIMES:
		l *= r;
		break;

	case DIVIDE:
	case LDIV:
		if (r==0)
			error("Divide check");
		else
			l /= r;
		break;

	case MOD:
	case LMOD:
		if (r==0)
			error("Divide check");
		else
			l %= r;
		break;

	case AND:
		l &= r;
		break;

	case ANDN:
		l &= ~r;
		break;

	case OR:
		l |= r;
		break;

	case EXOR:
		l ^= r;
		break;

	case LSHIFT:
		l <<= r;
		break;

	case RSHIFT:
		l >>= r;
		break;

	default:
		return(0);
	}
	if (lp->op==LCON) {
		lp->u.lconst.lvalue = l;
		return(lp);
	}
	lp = getblk(sizeof(struct node));
	lp->op = LCON;
	lp->type = LONG;
	lp->u.lconst.lvalue = l;
	return(lp);
}

void insert(int16_t op, struct node *atree, struct acl *alist)
{
	register int16_t d;
	register struct acl *list;
	register struct node *tree;
	int16_t d1, i;
	struct node *t;

	tree = atree;
	list = alist;
ins:
	if (tree->op != op)
		tree = optim(tree);
	if (tree->op == op && list->nextn < LSTSIZ-2) {
		list->nlist[list->nextn++] = tree;
		insert(op, tree->u.tnode.tr1, list);
		insert(op, tree->u.tnode.tr2, list);
		return;
	}
	if (!isfloat(tree)) {
		/* c1*(x+c2) -> c1*x+c1*c2 */
		if ((tree->op==TIMES||tree->op==LSHIFT)
		  && tree->u.tnode.tr2->op==CON && tree->u.tnode.tr2->u.tconst.value>0
		  && tree->u.tnode.tr1->op==PLUS && tree->u.tnode.tr1->u.tnode.tr2->op==CON) {
			d = tree->u.tnode.tr2->u.tconst.value;
			if (tree->op==TIMES)
				tree->u.tnode.tr2->u.tconst.value *= tree->u.tnode.tr1->u.tnode.tr2->u.tconst.value;
			else
				tree->u.tnode.tr2->u.tconst.value = tree->u.tnode.tr1->u.tnode.tr2->u.tconst.value << d;
			tree->u.tnode.tr1->u.tnode.tr2->u.tconst.value = d;
			tree->u.tnode.tr1->op = tree->op;
			tree->op = PLUS;
			tree = optim(tree);
			if (op==PLUS)
				goto ins;
		}
	}
	d = degree(tree);
	for (i=0; i<list->nextl; i++) {
		if ((d1=degree(list->llist[i]))<d) {
			t = list->llist[i];
			list->llist[i] = tree;
			tree = t;
			d = d1;
		}
	}
	list->llist[list->nextl++] = tree;
}

struct node *tnode(int16_t op, int16_t type, struct node *tr1, struct node *tr2)
{
	register struct node *p;

	p = getblk(sizeof(*p));
	p->op = op;
	p->type = type;
	p->u.tnode.degree = 0;
	p->u.tnode.tr1 = tr1;
	if (opdope[op]&BINARY)
		p->u.tnode.tr2 = tr2;
	else
		p->u.tnode.tr2 = NULL;
	return(p);
}

struct node *tconst(int16_t val, int16_t type)
{
	register struct node *p;

	p = getblk(sizeof(*p));
	p->op = CON;
	p->type = type;
	p->u.tconst.value = val;
	return(p);
}

void *getblk(int16_t size)
{
	register char *p;

	if (size&01)
		abort();
	p = curbase;
	if ((curbase += size) >= coremax) {
		error("Out of space-- c1");
		exit(1);
	}
	return(p);
}

int16_t islong(int16_t t)
{
	if (t==LONG)
		return(2);
	return(1);
}

struct node *isconstant(struct node *at)
{
	register struct node *t;

	t = at;
	if (t->op==CON || t->op==SFCON)
		return(t);
	if (t->op==ITOL && t->u.tnode.tr1->op==CON)
		return(t->u.tnode.tr1);
	return(0);
}

struct node *hardlongs(struct node *at)
{
	register struct node *t;

	t = at;
	switch(t->op) {

	case TIMES:
	case DIVIDE:
	case MOD:
		t->op += LTIMES-TIMES;
		break;

	case ASTIMES:
	case ASDIV:
	case ASMOD:
		t->op += LASTIMES-ASTIMES;
		t->u.tnode.tr1 = tnode(AMPER, LONG+PTR, t->u.tnode.tr1, NULL);
		break;

	default:
		return(t);
	}
	return(optim(t));
}
