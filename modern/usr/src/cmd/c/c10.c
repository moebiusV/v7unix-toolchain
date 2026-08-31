#
/*

	    	C compiler, part 2


*/

#include "c1.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

static void dbprint(int op)
{
#ifdef	DEBUG
	printf("	/ %s", opntab[op]);
#else
	(void)op;
#endif
}

char	maprel[] = {	EQUAL, NEQUAL, GREATEQ, GREAT, LESSEQ,
			LESS, GREATQP, GREATP, LESSEQP, LESSP
};

char	notrel[] = {	NEQUAL, EQUAL, GREAT, GREATEQ, LESS,
			LESSEQ, GREATP, GREATQP, LESSP, LESSEQP
};

struct node czero = { CON, INT, { .tconst = { 0 } } };
struct node cone = { CON, INT, { .tconst = { 1 } } };

struct node sfuncr = { NAME, STRUCT, { .tname = { STATIC, 0, 0, 0 } } };

struct	table	*cregtab;

int16_t	nreg = 3;
int16_t	isn = 10000;

int main(int argc, char *argv[])
{

	if (argc<4) {
		error("Arg count");
		exit(1);
	}
	if (freopen(argv[1], "r", stdin)==NULL) {
		error("Missing temp file");
		exit(1);
	}
	if ((freopen(argv[3], "w", stdout)) == NULL) {
		error("Can't create %s", argv[3]);
		exit(1);
	}

	funcbase = curbase = mmap(NULL, 16<<20, PROT_READ|PROT_WRITE,
				   MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (funcbase == MAP_FAILED) {
		error("Out of space-- c1");
		exit(1);
	}
	coremax = funcbase + (16<<20);
	getree();
	/*
	 * If any floating-point instructions
	 * were used, generate a reference that
	 * pulls in the floating-point part of printf.
	 */
	if (nfloat)
		printf(".globl	fltused\n");
	/*
	 * tack on the string file.
	 */
	printf(".globl\n.data\n");
	if (*argv[2] != '-') {
		if (freopen(argv[2], "r", stdin)==NULL) {
			error("Missing temp file");
			exit(1);
		}
		getree();
	}
	if (totspace >= (unsigned)56000) {
		error("Warning: possibly too much data");
		nerror--;
	}
	exit(nerror!=0);
}

/*
 * Given a tree, a code table, and a
 * count of available registers, find the code table
 * for the appropriate operator such that the operands
 * are of the right type and the number of registers
 * required is not too large.
 * Return a ptr to the table entry or 0 if none found.
 */
struct optab *match(struct node *atree, struct table *table, int16_t nrleft, int16_t nocvt)
{
	enum { NOCVL = 1, NOCVR = 2 };
	int16_t op, d1, d2, dope;
	struct node *p2;
	register struct node *p1, *tree;
	register struct optab *opt;

	if ((tree=atree)==0)
		return(0);
	if (table==lsptab)
		table = sptab;
	if ((op = tree->op)==0)
		return(0);
	dope = opdope[op];
	if ((dope&LEAF) == 0)
		p1 = tree->u.tnode.tr1;
	else
		p1 = tree;
	d1 = dcalc(p1, nrleft);
	if ((dope&BINARY)!=0) {
		p2 = tree->u.tnode.tr2;
		/*
		 * If a subtree starts off with a conversion operator,
		 * try for a match with the conversion eliminated.
		 * E.g. int = double can be done without generating
		 * the converted int in a register by
		 * movf double,fr0; movfi fr0,int .
		 */
		if (opdope[p2->op]&CNVRT && (nocvt&NOCVR)==0
			 && (opdope[p2->u.tnode.tr1->op]&CNVRT)==0) {
			tree->u.tnode.tr2 = p2->u.tnode.tr1;
			if (opt = match(tree, table, nrleft, NOCVL))
				return(opt);
			tree->u.tnode.tr2 = p2;
		} else if (opdope[p1->op]&CNVRT && (nocvt&NOCVL)==0
		 && (opdope[p1->u.tnode.tr1->op]&CNVRT)==0) {
			tree->u.tnode.tr1 = p1->u.tnode.tr1;
			if (opt = match(tree, table, nrleft, NOCVR))
				return(opt);
			tree->u.tnode.tr1 = p1;
		}
		d2 = dcalc(p2, nrleft);
	}
	for (; table->tabop!=op; table++)
		if (table->tabop==0)
			return(0);
	for (opt = table->tabp; opt->tabdeg1!=0; opt++) {
		if (d1 > (opt->tabdeg1&077)
		 || (opt->tabdeg1 >= 0100 && (p1->op != STAR)))
			continue;
		if (notcompat(p1, opt->tabtyp1, op)) {
			continue;
		}
		if ((opdope[op]&BINARY)!=0 && p2!=0) {
			if (d2 > (opt->tabdeg2&077)
			 || (opt->tabdeg2 >= 0100) && (p2->op != STAR) )
				continue;
			if (notcompat(p2,opt->tabtyp2, 0))
				continue;
		}
		return(opt);
	}
	return(0);
}

/*
 * Given a tree, a code table, and a register,
 * produce code to evaluate the tree with the appropriate table.
 * Registers reg and upcan be used.
 * If there is a value, it is desired that it appear in reg.
 * The routine returns the register in which the value actually appears.
 * This routine must work or there is an error.
 * If the table called for is cctab, sptab, or efftab,
 * and tree can't be done using the called-for table,
 * another try is made.
 * If the tree can't be compiled using cctab, regtab is
 * used and a "tst" instruction is produced.
 * If the tree can't be compiled using sptab,
 * regtab is used and the register is pushed on the stack.
 * If the tree can't be compiled using efftab,
 * just use regtab.
 * Regtab must succeed or an "op not found" error results.
 *
 * A number of special cases are recognized, and
 * there is an interaction with the optimizer routines.
 */
int16_t cexpr(struct node *atree, struct table *table, int16_t areg);
int16_t chkleaf(struct node *atree, struct table *table, int16_t reg);
int16_t comarg(struct node *atree, int16_t *flagp);
int16_t delay(struct node **treep, struct table *table, int16_t reg);
void doinit(int16_t atype, struct node *atree);
void movreg(int16_t r0, int16_t r1, struct node *tree);
struct node *ncopy(struct node *ap);
int16_t reorder(struct node **treep, struct table *table, int16_t reg);
struct node *sdelay(struct node **ap);
int16_t sreorder(struct node **treep, struct table *table, int16_t reg, int16_t recurf);
struct node * strfunc(struct node *atp);

int16_t rcexpr(struct node *atree, struct table *atable, int16_t reg)
{
	register int16_t r;
	int16_t modf, nargs, recurf;
	register struct node *tree;
	register struct table *table;

	table = atable;
	recurf = 0;
	if (reg<0) {
		recurf++;
		reg = ~reg;
		if (reg>=020) {
			reg -= 020;
			recurf++;
		}
	}
again:
	if((tree=atree)==0)
		return(0);
	if (opdope[tree->op]&RELAT && tree->u.tnode.tr2->op==CON && tree->u.tnode.tr2->u.tconst.value==0
	 && table==cctab)
		tree = atree = tree->u.tnode.tr1;
	/*
	 * fieldselect(...) : in efftab mode,
	 * ignore the select, otherwise
	 * do the shift and mask.
	 */
	if (tree->op == FSELT) {
		if (table==efftab)
			atree = tree = tree->u.tnode.tr1;
		else {
			tree->op = FSEL;
			atree = tree = optim(tree);
		}
	}
	switch (tree->op)  {

	/*
	 * Structure assignments
	 */
	case STRASG:
		strasg(tree);
		return(0);

	/*
	 * An initializing expression
	 */
	case INIT:
		tree = optim(tree);
		doinit(tree->type, tree->u.tnode.tr1);
		return(0);

	/*
	 * Put the value of an expression in r0,
	 * for a switch or a return
	 */
	case RFORCE:
		tree = tree->u.tnode.tr1;
		if((r=rcexpr(tree, regtab, reg)) != 0)
			movreg(r, 0, tree);
		return(0);

	/*
	 * sequential execution
	 */
	case SEQNC:
		r = nstack;
		rcexpr(tree->u.tnode.tr1, efftab, reg);
		nstack = r;
		atree = tree = tree->u.tnode.tr2;
		goto again;

	/*
	 * In the generated &~ operator,
	 * fiddle things so a PDP-11 "bit"
	 * instruction will be produced when cctab is used.
	 */
	case ANDN:
		if (table==cctab) {
			tree->op = TAND;
			tree->u.tnode.tr2 = optim(tnode(COMPL, tree->type, tree->u.tnode.tr2, NULL));
		}
		break;

	/*
	 * Handle a subroutine call. It has to be done
	 * here because if cexpr got called twice, the
	 * arguments might be compiled twice.
	 * There is also some fiddling so the
	 * first argument, in favorable circumstances,
	 * goes to (sp) instead of -(sp), reducing
	 * the amount of stack-popping.
	 */
	case CALL:
		r = 0;
		nargs = 0;
		modf = 0;
		if (tree->u.tnode.tr1->op!=NAME || tree->u.tnode.tr1->u.tname.class!=EXTERN) {
			nargs++;
			nstack++;
		}
		tree = tree->u.tnode.tr2;
		if(tree->op) {
			while (tree->op==COMMA) {
				r += comarg(tree->u.tnode.tr2, &modf);
				tree = tree->u.tnode.tr1;
				nargs++;
			}
			r += comarg(tree, &modf);
			nargs++;
		}
		tree = atree;
		tree->op = CALL2;
		if (modf && tree->u.tnode.tr1->op==NAME && tree->u.tnode.tr1->u.tname.class==EXTERN)
			tree->op = CALL1;
		if (cexpr(tree, regtab, reg)<0)
			error("compiler botch: call");
		popstk(r);
		nstack -= nargs;
		if (table==efftab || table==regtab)
			return(0);
		r = 0;
		goto fixup;

	/*
	 * Longs need special treatment.
	 */
	case ASLSH:
	case LSHIFT:
		if (tree->type==LONG) {
			if (tree->u.tnode.tr2->op==ITOL)
				tree->u.tnode.tr2 = tree->u.tnode.tr2->u.tnode.tr1;
			else
				tree->u.tnode.tr2 = optim(tnode(LTOI,INT,tree->u.tnode.tr2, NULL));
			if (tree->op==ASLSH)
				tree->op = ASLSHL;
			else
				tree->op = LLSHIFT;
		}
		break;

	/*
	 * Try to change * to shift.
	 */
	case TIMES:
	case ASTIMES:
		tree = pow2(tree);
	}
	/*
	 * Try to find postfix ++ and -- operators that can be
	 * pulled out and done after the rest of the expression
	 */
	if (table!=cctab && table!=cregtab && recurf<2
	 && (opdope[tree->op]&LEAF)==0) {
		if (r=delay(&atree, table, reg)) {
			tree = atree;
			table = efftab;
			reg = r-1;
		}
	}
	/*
	 * Basically, try to reorder the computation
	 * so  reg = x+y  is done as  reg = x; reg =+ y
	 */
	if (recurf==0 && reorder(&atree, table, reg)) {
		if (table==cctab && atree->op==NAME)
			return(reg);
	}
	tree = atree;
	if (table==efftab && tree->op==NAME)
		return(reg);
	if ((r=cexpr(tree, table, reg))>=0)
		return(r);
	if (table!=regtab && (table!=cctab||(opdope[tree->op]&RELAT)==0)) {
		if((r=cexpr(tree, regtab, reg))>=0) {
	fixup:
			modf = isfloat(tree);
			dbprint(tree->op);
			if (table==sptab || table==lsptab) {
				if (tree->type==LONG) {
					printf("mov\tr%d,-(sp)\n",r+1);
					nstack++;
				}
				printf("mov%s	r%d,%s(sp)\n", modf?"f":"", r,
					table==sptab? "-":"");
				nstack++;
			}
			if (table==cctab)
				printf("tst%s	r%d\n", modf?"f":"", r);
			return(r);
		}
	}
	/*
	 * There's a last chance for this operator
	 */
	if (tree->op==LTOI) {
		r = rcexpr(tree->u.tnode.tr1, regtab, reg);
		if (r >= 0) {
			r++;
			goto fixup;
		}
	}
	if (tree->type == STRUCT)
		error("Illegal operation on structure");
	else if (tree->op>0 && tree->op<RFORCE && opntab[tree->op])
		error("No code table for op: %s", opntab[tree->op]);
	else
		error("No code table for op %d", tree->op);
	return(reg);
}

/*
 * Try to compile the tree with the code table using
 * registers areg and up.  If successful,
 * return the register where the value actually ended up.
 * If unsuccessful, return -1.
 *
 * Most of the work is the macro-expansion of the
 * code table.
 */
int16_t cexpr(struct node *atree, struct table *table, int16_t areg)
{
	int16_t c, r;
	register struct node *p, *p1, *tree;
	struct table *ctable;
	struct node *p2;
	char *string;
	int16_t reg, reg1, rreg, flag, opd;
	struct optab *opt;

	tree = atree;
	reg = areg;
	p1 = tree->u.tnode.tr2;
	c = tree->op;
	opd = opdope[c];
	/*
	 * When the value of a relational or a logical expression is
	 * desired, more work must be done.
	 */
	if ((opd&RELAT||c==LOGAND||c==LOGOR||c==EXCLA) && table!=cctab) {
		cbranch(tree, c=isn++, 1, reg);
		rcexpr(&czero, table, reg);
		branch(isn, 0, 0);
		label(c);
		rcexpr(&cone, table, reg);
		label(isn++);
		return(reg);
	}
	if(c==QUEST) {
		if (table==cctab)
			return(-1);
		cbranch(tree->u.tnode.tr1, c=isn++, 0, reg);
		flag = nstack;
		rreg = rcexpr(p1->u.tnode.tr1, table, reg);
		nstack = flag;
		branch(r=isn++, 0, 0);
		label(c);
		reg = rcexpr(p1->u.tnode.tr2, table, rreg);
		if (rreg!=reg)
			movreg(reg, rreg, tree->u.tnode.tr2);
		label(r);
		return(rreg);
	}
	reg = oddreg(tree, reg);
	reg1 = reg+1;
	/*
	 * long values take 2 registers.
	 */
	if ((tree->type==LONG||opd&RELAT&&tree->u.tnode.tr1->type==LONG) && tree->op!=ITOL)
		reg1++;
	/*
	 * Leaves of the expression tree
	 */
	if ((r = chkleaf(tree, table, reg)) >= 0)
		return(r);
	/*
	 * x + (-1) is better done as x-1.
	 */

	if ((tree->op==PLUS||tree->op==ASPLUS) &&
	    (p1=tree->u.tnode.tr2)->op == CON && p1->u.tconst.value == -1) {
		p1->u.tconst.value = 1;
		tree->op += (MINUS-PLUS);
	}
	/*
	 * Because of a peculiarity of the PDP11 table
	 * char = *intreg++ and *--intreg cannot go through.
 	 */
	if (tree->u.tnode.tr2!=NULL && tree->u.tnode.tr1->type==CHAR && tree->u.tnode.tr2->type!=CHAR
	 && (tree->u.tnode.tr2->op==AUTOI||tree->u.tnode.tr2->op==AUTOD))
		tree->u.tnode.tr2 = tnode(LOAD, tree->u.tnode.tr2->type, tree->u.tnode.tr2, NULL);
	if (table==cregtab)
		table = regtab;
	/*
	 * The following peculiar code depends on the fact that
	 * if you just want the codition codes set, efftab
	 * will generate the right code unless the operator is
	 * a shift or
	 * postfix ++ or --. Unravelled, if the table is
	 * cctab and the operator is not special, try first
	 * for efftab;  if the table isn't, if the operator is,
	 * or the first match fails, try to match
	 * with the table actually asked for.
	 */
	/*
	 * Account for longs and oddregs; below is really
	 * r = nreg - reg - (reg-areg) - (reg1-reg-1);
	 */
	r = nreg - reg + areg - reg1 + 1;
	if (table!=cctab || c==INCAFT || c==DECAFT || tree->type==LONG
	 || c==ASRSH || c==ASLSH || c==ASULSH
	 || (opt = match(tree, efftab, r, 0)) == 0)
		if ((opt=match(tree, table, r, 0))==0)
			return(-1);
	string = opt->tabstring;
	p1 = tree->u.tnode.tr1;
	if (p1->op==FCON && p1->u.tconst.value>0) {
		uint16_t w[4];
		pdp11_double(p1->u.ftconst.fvalue, w);
		printf(".data\nL%d:%o;%o;%o;%o\n.text\n",
			p1->u.tconst.value, (unsigned)w[0], (unsigned)w[1],
			(unsigned)w[2], (unsigned)w[3]);
		p1->u.tconst.value = -p1->u.tconst.value;
	}
	p2 = 0;
	if (opdope[tree->op]&BINARY) {
		p2 = tree->u.tnode.tr2;
		if (p2->op==FCON && p2->u.tconst.value>0) {
			uint16_t w[4];
			pdp11_double(p2->u.ftconst.fvalue, w);
			printf(".data\nL%d:%o;%o;%o;%o\n.text\n",
				p2->u.tconst.value, (unsigned)w[0], (unsigned)w[1],
				(unsigned)w[2], (unsigned)w[3]);
			p2->u.tconst.value = -p2->u.tconst.value;
		}
	}
loop:
	/*
	 * The 0200 bit asks for a tab.
	 */
	if ((c = *string++) & 0200) {
		c &= 0177;
		putchar('\t');
	}
	switch (c) {

	case '\n':
		dbprint(tree->op);
		break;

	case '\0':
		if (!isfloat(tree))
			if (tree->op==DIVIDE||tree->op==ASDIV||tree->op==PTOI)
				reg--;
		return(reg);

	/* A1 */
	case 'A':
		p = p1;
		goto adr;

	/* A2 */
	case 'B':
		p = p2;
		goto adr;

	adr:
		c = 0;
		while (*string=='\'') {
			c++;
			string++;
		}
		if (*string=='+') {
			c = 100;
			string++;
		}
		pname(p, c);
		goto loop;

	/* I */
	case 'M':
		if ((c = *string)=='\'')
			string++;
		else
			c = 0;
		prins(tree->op, c, instab);
		goto loop;

	/* B1 */
	case 'C':
		if ((opd&LEAF) != 0)
			p = tree;
		else
			p = p1;
		goto pbyte;

	/* BF */
	case 'P':
		p = tree;
		goto pb1;

	/* B2 */
	case 'D':
		p = p2;
	pbyte:
		if (p->type==CHAR)
			putchar('b');
	pb1:
		if (isfloat(p))
			putchar('f');
		goto loop;

	/* BE */
	case 'L':
		if (p1->type==CHAR || p2->type==CHAR)
			putchar('b');
		p = tree;
		goto pb1;

	/* F */
	case 'G':
		p = p1;
		flag = 01;
		goto subtre;

	/* S */
	case 'K':
		p = p2;
		flag = 02;
		goto subtre;

	/* H */
	case 'H':
		p = tree;
		flag = 04;

	subtre:
		ctable = regtab;
		if (flag&04)
			ctable = cregtab;
		c = *string++ - 'A';
		if (*string=='!') {
			string++;
			c |= 020;	/* force right register */
		}
		if ((c&02)!=0)
			ctable = sptab;
		if ((c&04)!=0)
			ctable = cctab;
		if ((flag&01) && ctable==regtab && (c&01)==0
		  && (tree->op==DIVIDE||tree->op==MOD
		   || tree->op==ASDIV||tree->op==ASMOD||tree->op==ITOL))
			ctable = cregtab;
		if ((c&01)!=0) {
			p = p->u.tnode.tr1;
			if(collcon(p) && ctable!=sptab) {
				if (p->op==STAR)
					p = p->u.tnode.tr1;
				p = p->u.tnode.tr1;
			}
		}
		if (table==lsptab && ctable==sptab)
			ctable = lsptab;
		if (c&010)
			r = reg1;
		else
			if (opdope[p->op]&LEAF || p->u.tnode.degree < 2)
				r = reg;
			else
				r = areg;
		rreg = rcexpr(p, ctable, r);
		if (ctable!=regtab && ctable!=cregtab)
			goto loop;
		if (c&010) {
			if (c&020 && rreg!=reg1)
				movreg(rreg, reg1, p);
			else
				reg1 = rreg;
		} else if (rreg!=reg)
			if ((c&020)==0 && oddreg(tree, 0)==0 && tree->type!=LONG
			&& (flag&04
			  || flag&01&&xdcalc(p2,nreg-rreg-1)<=(opt->tabdeg2&077)
			  || flag&02&&xdcalc(p1,nreg-rreg-1)<=(opt->tabdeg1&077))) {
				reg = rreg;
				reg1 = rreg+1;
			} else
				movreg(rreg, reg, p);
		goto loop;

	/* R */
	case 'I':
		r = reg;
		if (*string=='-') {
			string++;
			r--;
		}
		goto preg;

	/* R1 */
	case 'J':
		r = reg1;
	preg:
		if (*string=='+') {
			string++;
			r++;
		}
		if (r>nreg || r>=4 && tree->type==DOUBLE)
			error("Register overflow: simplify expression");
		printf("r%d", r);
		goto loop;

	case '-':		/* check -(sp) */
		if (*string=='(') {
			nstack++;
			if (table!=lsptab)
				putchar('-');
			goto loop;
		}
		break;

	case ')':		/* check (sp)+ */
		putchar(')');
		if (*string=='+')
			nstack--;
		goto loop;

	/* #1 */
	case '#':
		p = p1->u.tnode.tr1;
		goto nmbr;

	/* #2 */
	case '"':
		p = p2->u.tnode.tr1;

	nmbr:
		if(collcon(p)) {
			if (p->op==STAR) {
				printf("*");
				p = p->u.tnode.tr1;
			}
			if ((p = p->u.tnode.tr2)->op == CON) {
				if (p->u.tconst.value)
					psoct(p->u.tconst.value);
			} else if (p->op==AMPER)
				pname(p->u.tnode.tr1, 0);
		}
		goto loop;

	/*
	 * Certain adjustments for / % and PTOI
	 */
	case 'T':
		c = reg-1;
		if (tree->op == PTOI) {
			printf("bic	r%d,r%d\nsbc	r%d\n", c,c,c);
			goto loop;
		}
		if (p1->type==UNSIGN || p1->type&XTYPE) {
			printf("clr	r%d\n", c);
			goto loop;
		}
		if (dcalc(p1, 5)>12 && !match(p1, cctab, 10, 0))
			printf("tst	r%d\n", reg);
		printf("sxt	r%d\n", c);
		goto loop;

	case 'V':	/* adc sbc, clr, or sxt as required for longs */
		switch(tree->op) {
		case PLUS:
		case ASPLUS:
		case INCBEF:
		case INCAFT:
			printf("adc");
			break;

		case MINUS:
		case ASMINUS:
		case NEG:
		case DECBEF:
		case DECAFT:
			printf("sbc");
			break;

		case ASSIGN:
			p = tree->u.tnode.tr2;
			goto lcasev;

		case ASDIV:
		case ASMOD:
		case ASULSH:
			p = tree->u.tnode.tr1;
		lcasev:
			if (p->type!=LONG) {
				if (p->type==UNSIGN || p->type&XTYPE)
					printf("clr");
				else
					printf("sxt");
				goto loop;
			}
		default:
			while ((c = *string++)!='\n' && c!='\0');
			break;
		}
		goto loop;

	/*
	 * Mask used in field assignments
	 */
	case 'Z':
		printf("$%o", tree->u.fasgn.mask);
		goto loop;

	/*
	 * Relational on long values.
	 * Might bug out early. E.g.,
	 * (long<0) can be determined with only 1 test.
	 */
	case 'X':
		if (xlongrel(*string++ - '0'))
			return(reg);
		goto loop;
	}
	putchar(c);
	goto loop;
}

/*
 * This routine just calls sreorder (below)
 * on the subtrees and then on the tree itself.
 * It returns non-zero if anything changed.
 */
int16_t reorder(struct node **treep, struct table *table, int16_t reg)
{
	register int16_t r, o;
	register struct node *p;

	p = *treep;
	o = p->op;
	if (opdope[o]&LEAF || o==LOGOR || o==LOGAND)
		return(0);
	while(sreorder(&p->u.tnode.tr1, regtab, reg, 1))
		;
	if (opdope[o]&BINARY) 
		while(sreorder(&p->u.tnode.tr2, regtab, reg, 1))
			;
	r = 0;
	if (table!=cctab)
	while (sreorder(treep, table, reg, 0))
		r++;
	*treep = optim(*treep);
	return(r);
}

/*
 * Basically this routine carries out two kinds of optimization.
 * First, it observes that "x + (reg = y)" where actually
 * the = is any assignment op is better done as "reg=y; x+reg".
 * In this case rcexpr is called to do the first part and the
 * tree is modified so the name of the register
 * replaces the assignment.
 * Moreover, expressions like "reg = x+y" are best done as
 * "reg = x; reg =+ y" (so long as "reg" and "y" are not the same!).
 */
int16_t sreorder(struct node **treep, struct table *table, int16_t reg, int16_t recurf)
{
	register struct node *p, *p1;

	p = *treep;
	if (opdope[p->op]&LEAF)
		return(0);
	if (p->op==PLUS && recurf)
		if (reorder(&p->u.tnode.tr2, table, reg))
			*treep = p = optim(p);
	if (opdope[p->op]&LEAF)
		return(0);
	p1 = p->u.tnode.tr1;
	if (p->op==STAR || p->op==PLUS) {
		if (recurf && reorder(&p->u.tnode.tr1, table, reg))
			*treep = p = optim(p);
		if (opdope[p->op]&LEAF)
			return(0);
		p1 = p->u.tnode.tr1;
	}
	if (p1->op==NAME) switch(p->op) {
		case ASLSH:
		case ASRSH:
		case ASSIGN:
			if (p1->u.tname.class != REG||p1->type==CHAR||isfloat(p->u.tnode.tr2))
				return(0);
			if (p->op==ASSIGN) switch (p->u.tnode.tr2->op) {
			case TIMES:
				if (!ispow2(p->u.tnode.tr2))
					break;
				p->u.tnode.tr2 = pow2(p->u.tnode.tr2);
			case PLUS:
			case MINUS:
			case AND:
			case ANDN:
			case OR:
			case EXOR:
			case LSHIFT:
			case RSHIFT:
				p1 = p->u.tnode.tr2->u.tnode.tr2;
				if (xdcalc(p1, 16) > 12
				 || p1->op==NAME
				 &&(p1->u.tname.nloc==p->u.tnode.tr1->u.tname.nloc
				  || p1->u.tname.regno==p->u.tnode.tr1->u.tname.nloc))
					return(0);
				p1 = p->u.tnode.tr2;
				p->u.tnode.tr2 = p1->u.tnode.tr1;
				if (p1->u.tnode.tr1->op!=NAME
				 || p1->u.tnode.tr1->u.tname.class!=REG
				 || p1->u.tnode.tr1->u.tname.nloc!=p->u.tnode.tr1->u.tname.nloc)
					rcexpr(p, efftab, reg);
				p->u.tnode.tr2 = p1->u.tnode.tr2;
				p->op = p1->op + ASPLUS - PLUS;
				*treep = p;
				return(1);
			}
			goto OK;

		case ASTIMES:
			if (!ispow2(p))
				return(0);
		case ASPLUS:
		case ASMINUS:
		case ASAND:
		case ASANDN:
		case ASOR:
		case ASXOR:
		case INCBEF:
		case DECBEF:
		OK:
			if (table==cctab||table==cregtab)
				reg += 020;
			rcexpr(optim(p), efftab, ~reg);
			*treep = p1;
			return(1);
	}
	return(0);
}

/*
 * Delay handles postfix ++ and -- 
 * It observes that "x + y++" is better
 * treated as "x + y; y++".
 * If the operator is ++ or -- itself,
 * it calls rcexpr to load the operand, letting
 * the calling instance of rcexpr to do the
 * ++ using efftab.
 * Otherwise it uses sdelay to search for inc/dec
 * among the operands.
 */
int16_t delay(struct node **treep, struct table *table, int16_t reg)
{
	register struct node *p, *p1;
	register int16_t r;

	p = *treep;
	if ((p->op==INCAFT||p->op==DECAFT)
	 && p->u.tnode.tr1->op==NAME) {
		return(1+rcexpr(p->u.tnode.tr1, table, reg));
	}
	p1 = 0;
	if (opdope[p->op]&BINARY) {
		if (p->op==LOGAND || p->op==LOGOR)
			return(0);
		p1 = sdelay(&p->u.tnode.tr2);
	}
	if (p1==0)
		p1 = sdelay(&p->u.tnode.tr1);
	if (p1) {
		r = rcexpr(optim(p), table, reg);
		*treep = p1;
		return(r+1);
	}
	return(0);
}

struct node *sdelay(struct node **ap)
{
	register struct node *p, *p1;

	p = *ap;
	if ((p->op==INCAFT||p->op==DECAFT) && p->u.tnode.tr1->op==NAME) {
		*ap = ncopy(p->u.tnode.tr1);
		return(p);
	}
	if (p->op==STAR || p->op==PLUS)
		if (p1=sdelay(&p->u.tnode.tr1))
			return(p1);
	if (p->op==PLUS)
		return(sdelay(&p->u.tnode.tr2));
	return(0);
}

/*
 * Copy a tree node for a register variable.
 * Used by sdelay because if *reg-- is turned
 * into *reg; reg-- the *reg will in turn
 * be changed to some offset class, accidentally
 * modifying the reg--.
 */
struct node *ncopy(struct node *ap)
{
	register struct node *p, *q;

	p = ap;
	if (p->u.tname.class!=REG)
		return(p);
	q = getblk(sizeof(*p));
	q->op = p->op;
	q->type = p->type;
	q->u.tname.class = p->u.tname.class;
	q->u.tname.offset = p->u.tname.offset;
	q->u.tname.nloc = p->u.tname.nloc;
	return(q);
}

/*
 * If the tree can be immediately loaded into a register,
 * produce code to do so and return success.
 */
int16_t chkleaf(struct node *atree, struct table *table, int16_t reg)
{
	struct node lbuf;
	register struct node *tree;

	tree = atree;
	if (tree->op!=STAR && dcalc(tree, nreg-reg) > 12)
		return(-1);
	lbuf.op = LOAD;
	lbuf.type = tree->type;
	lbuf.u.tnode.degree = tree->u.tnode.degree;
	lbuf.u.tnode.tr1 = tree;
	lbuf.u.tnode.tr2 = NULL;	/* LOAD is unary; V7 left this garbage (null-page reads) */
	return(rcexpr(&lbuf, table, reg));
}

/*
 * Compile a function argument.
 * If the stack is currently empty, put it in (sp)
 * rather than -(sp); this will save a pop.
 * Return the number of bytes pushed,
 * for future popping.
 */
int16_t comarg(struct node *atree, int16_t *flagp)
{
	register struct node *tree;
	register int16_t retval;
	int16_t i;
	int16_t size;

	tree = atree;
	if (tree->op==STRASG) {
		size = tree->u.fasgn.mask;
		tree = tree->u.tnode.tr1;
		tree = strfunc(tree);
		if (size <= 2) {
			setype(tree, INT);
			goto normal;
		}
		if (size <= 4) {
			setype(tree, LONG);
			goto normal;
		}
		if (tree->op!=NAME && tree->op!=STAR) {
			error("Unimplemented structure assignment");
			return(0);
		}
		tree = tnode(AMPER, STRUCT+PTR, tree, NULL);
		tree = tnode(PLUS, STRUCT+PTR, tree, tconst(size, INT));
		tree = optim(tree);
		retval = rcexpr(tree, regtab, 0);
		size >>= 1;
		if (size <= 5) {
			for (i=0; i<size; i++)
				printf("mov	-(r%d),-(sp)\n", retval);
		} else {
			if (retval!=0)
				printf("mov	r%d,r0\n", retval);
			printf("mov	$%o,r1\n", size);
			printf("L%d:mov	-(r0),-(sp)\ndec\tr1\njne\tL%d\n", isn, isn);
			isn++;
		}
		nstack++;
		return(size*2);
	}
normal:
	if (nstack || isfloat(tree) || tree->type==LONG) {
		rcexpr(tree, sptab, 0);
		retval = arlength(tree->type);
	} else {
		(*flagp)++;
		rcexpr(tree, lsptab, 0);
		retval = 0;
	}
	return(retval);
}

struct node * strfunc(struct node *atp)
{
	register struct node *tp;

	tp = atp;
	if (tp->op != CALL)
		return(tp);
	setype(tp, STRUCT+PTR);
	return(tnode(STAR, STRUCT, tp, NULL));
}

/*
 * Compile an initializing expression
 */
void doinit(int16_t atype, struct node *atree)
{
	register struct node *tree;
	register int16_t type;
	long double sfval;
	long double fval;
	int32_t lval;

	tree = atree;
	type = atype;
	if (type==CHAR) {
		printf(".byte ");
		if (tree->type&XTYPE)
			goto illinit;
		type = INT;
	}
	if (type&XTYPE)
		type = INT;
	switch (type) {
	case INT:
	case UNSIGN:
		if (tree->op==FTOI) {
			if (tree->u.tnode.tr1->op!=FCON && tree->u.tnode.tr1->op!=SFCON)
				goto illinit;
			tree = tree->u.tnode.tr1;
			tree->u.tconst.value = tree->u.ftconst.fvalue;
			tree->op = CON;
		} else if (tree->op==LTOI) {
			if (tree->u.tnode.tr1->op!=LCON)
				goto illinit;
			tree = tree->u.tnode.tr1;
			lval = tree->u.lconst.lvalue;
			tree->op = CON;
			tree->u.tconst.value = lval;
		}
		if (tree->op == CON)
			printf("%o\n", tree->u.tconst.value);
		else if (tree->op==AMPER) {
			pname(tree->u.tnode.tr1, 0);
			putchar('\n');
		} else
			goto illinit;
		return;

	case DOUBLE:
	case FLOAT:
		if (tree->op==ITOF) {
			if (tree->u.tnode.tr1->op==CON) {
				fval = tree->u.tnode.tr1->u.tconst.value;
			} else
				goto illinit;
		} else if (tree->op==FCON || tree->op==SFCON)
			fval = tree->u.ftconst.fvalue;
		else if (tree->op==LTOF) {
			if (tree->u.tnode.tr1->op!=LCON)
				goto illinit;
			fval = tree->u.tnode.tr1->u.lconst.lvalue;
		} else
			goto illinit;
		if (type==FLOAT) {
			uint16_t w[2];
			sfval = fval;
			pdp11_float(sfval, w);
			printf("%o; %o\n", (unsigned)w[0], (unsigned)w[1]);
		} else {
			uint16_t w[4];
			pdp11_double(fval, w);
			printf("%o; %o; %o; %o\n",
				(unsigned)w[0], (unsigned)w[1], (unsigned)w[2], (unsigned)w[3]);
		}
		return;

	case LONG:
		if (tree->op==FTOL) {
			tree = tree->u.tnode.tr1;
			if (tree->op==SFCON)
				tree->op = FCON;
			if (tree->op!= FCON)
				goto illinit;
			lval = tree->u.ftconst.fvalue;
		} else if (tree->op==ITOL) {
			if (tree->u.tnode.tr1->op != CON)
				goto illinit;
			lval = tree->u.tnode.tr1->u.tconst.value;
		} else if (tree->op==LCON)
			lval = tree->u.lconst.lvalue;
		else
			goto illinit;
		{
			uint16_t w[2];
			pdp11_long(lval, w);
			printf("%o; %o\n", (unsigned)w[0], (unsigned)w[1]);
		}
		return;
	}
illinit:
	error("Illegal initialization");
}

void movreg(int16_t r0, int16_t r1, struct node *tree)
{
	register char *s;

	if (r0==r1)
		return;
	if (tree->type==LONG) {
		s = "mov	r%d,r%d\nmov	r%d,r%d\n";
		if (r0 < r1)
			printf(s, r0+1,r1+1,r0,r1);
		else
			printf(s, r0,r1,r0+1,r1+1);
		return;
	}
	printf("mov%s	r%d,r%d\n", isfloat(tree)?"f":"", r0, r1);
}
