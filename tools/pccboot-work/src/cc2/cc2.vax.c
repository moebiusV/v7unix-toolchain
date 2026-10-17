/*
 * cc2.vax.c -- the VAX peephole optimizer of Seventh Edition Unix for the
 * VAX (32V), modernized to C99.  Derived from 32V cmd/c2/{c2.h,c20.c,c21.c,
 * c22.c}; the original is Copyright (C) Bell Telephone Laboratories (and the
 * University of California, Berkeley, for the VAX parts) and is released
 * under the Caldera "ancient UNIX" license (BSD-style, 2002).
 *
 * Run by "pcc -O" on the assembly ccom writes for the unix32v target
 * (V7 as dialect), as 32V's cc ran c2 on pcc's output.  It does, over the
 * whole function: redundant load/store/test removal, forwarding a result to
 * its destination (3-address forms), addl/subl of $sym to movab/pushab,
 * jump chains, loop rotation, branch reversal by distance, sobgeq/sobgtr/
 * aoblss/acbl loops, jbc/jbs from bit tests, extzv/movzbl from masks.
 *
 * Changes from the original: K&R to C99 (tools/knr2c99.py --dialect vax),
 * 'combop' is a union with 'op' and 'subop' (little-endian hosts), the sbrk
 * arena is a chunk list, pointer/int punning replaced by uintptr_t, copy()
 * takes one string, abs() is iabs(), getline() is c2getline().
 */
/* @(#)c2.h 1.13 78/09/23 16:37:28 */
/*
 * Header for object code improver
 */

#define	JBR	1
#define	CBR	2
#define	JMP	3
#define	LABEL	4
#define	DLABEL	5
#define	EROU	7
#define	JSW	9
#define	MOV	10
#define	CLR	11
#define	INC	12
#define	DEC	13
#define	TST	14
#define	PUSH	15
#define CVT 16
#define	CMP	17
#define	ADD	18
#define	SUB	19
#define	BIT	20
#define	BIC	21
#define	BIS	22
#define	XOR	23
#define	COM	24
#define	NEG	25
#define	MUL	26
#define	DIV	27
#define	ASH	28
#define EXTV	29
#define EXTZV	30
#define INSV	31
#define	CALLS	32
#define RET	33
#define	CASE	34
#define	SOB	35
#define	TEXT	36
#define	DATA	37
#define	BSS	38
#define	ALIGN	39
#define	END	40
#define MOVZ 41
#define WGEN 42
#define SOBGEQ 43
#define SOBGTR 44
#define AOBLEQ 45
#define AOBLSS 46
#define ACB 47
#define MOVA 48
#define PUSHA 49
#define LGEN 50
#define SET 51

#define	JEQ	0
#define	JNE	1
#define	JLE	2
#define	JGE	3
#define	JLT	4
#define	JGT	5
/* rearranged for unsigned branches so that jxxu = jxx + 6 */
#define	JLOS	8
#define	JHIS	9
#define	JLO	10
#define	JHI	11

#define JBC 12
#define JBS 13
#define JLBC 14
#define JLBS 15
#define JBCC 16
#define JBSC 17
#define JBCS 18
#define JBSS 19

#define	BYTE	1
#define	WORD	2
#define LONG	3
#define	FLOAT	4
#define	DOUBLE	5
#define QUAD	6
#define OP2	7
#define OP3	8
#define OPB 9
#define OPX 10

#define T(a,b) (a|((b)<<8))
#define U(a,b) (a|((b)<<4))

struct optab {
	char	opstring[7];
	short	opcode;
};
extern struct optab optab[];

struct node {
	union {
		struct { unsigned char op, subop; };	/* little-endian overlay */
		short combop;
	};
	short	refc;
	struct	node	*forw;
	struct	node	*back;
	struct	node	*ref;
	char	*code;
	struct	optab	*pop;
	short	labno;
	short	seq;
};


char	line[512];
struct	node	first;
char	*curlp;
int	nbrbr;
int	nsaddr;
int	redunm;
int	iaftbr;
int	njp1;
int	nrlab;
int	nxjump;
int	ncmot;
int	nrevbr;
int	loopiv;
int	nredunj;
int	nskip;
int	ncomj;
int	nsob;
int	nrtst;
int nbj;
int nfield;

int	nchange;
int	isn;
int	debug;
char	revbr[];
#define	NREG	12
char	*regs[NREG+5]; /* 0-11, 4 for operands, 1 for running off end */
char	conloc[20];
char	conval[20];
char	ccloc[20];
int	ccarith;	/* the condition codes come from incX/decX: only Z and N match tstX */

#define	RT1	12
#define	RT2	13
#define RT3 14
#define RT4 15
#define	LABHS	127




#
char C20[] = {"@(#)c20.c 1.27 78/10/23 14:06:38"}; /* sccs ident */
/*
 *	 C object code improver
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>

char _sibuf[BUFSIZ], _sobuf[BUFSIZ];
int ioflag;
int	isn	= 20000;
struct optab *oplook();
struct optab *c2getline();

/*
 * Memory comes from an arena that is emptied after every input function:
 * alloc() hands out zeroed memory, mark/release forget all of it at once.
 */
struct chunk { struct chunk *next; size_t used, size; double data[1]; };
static struct chunk *chunks, *curchunk, *markchunk;
static size_t markused;

static void c2abort(int n) { fprintf(stderr, "C2: internal error %d\n", n); exit(1); }

void *alloc(int an)
{
	size_t n = ((size_t)an + 7) & ~(size_t)7;
	void *p;

	while (curchunk == NULL || curchunk->used + n > curchunk->size) {
		if (curchunk != NULL && curchunk->next != NULL) {
			curchunk = curchunk->next;
			curchunk->used = 0;
			continue;
		}
		size_t sz = n > (1 << 20) ? n : (1 << 20);
		struct chunk *c = malloc(sizeof(struct chunk) + sz);

		if (c == NULL) {
			fprintf(stderr, "Optimizer: out of space\n");
			exit(1);
		}
		c->next = NULL;
		c->used = 0;
		c->size = sz;
		if (curchunk != NULL)
			curchunk->next = c;
		else
			chunks = c;
		curchunk = c;
	}
	p = (char *)curchunk->data + curchunk->used;
	curchunk->used += n;
	return memset(p, 0, n);
}

static void arena_mark(void)
{
	(void)alloc(0);
	markchunk = curchunk;
	markused = curchunk->used;
}

static void arena_release(void)
{
	curchunk = markchunk;
	curchunk->used = markused;
}

int addsob(void);
void backjmp(struct node *ap1, struct node *ap2);
struct node * bflow(struct node *p);
struct node * bicopt(struct node *p);
void bitopt(struct node *p);
int bixprep(struct node *p, int bix);
int bmove(void);
struct optab * c2getline(void);
int clearreg(void);
int clearuse(void);
static void liveall(struct node *p, int islab);
static void findlive(void);
struct node * codemove(struct node *ap);
int comjump(void);
int compare(int op, char *acp1, char *acp2);
char * copy(char *ap);
int decref(struct node *p);
int delnode(struct node *p);
int dest(char *s, int type);
int equop(struct node *p1, struct node *p2);
int equstr(char *p1, char *p2);
int equtype(int t1, int t2);
char * findcon(int i, int type);
int findrand(char *as, int type);
int32_t getnum(char *p);
int iabs(int x);
int indexa(char *p);
int input(void);
struct node * insertl(struct node *op);
int isreg(char *s);
int iterate(void);
int jumpsw(void);
int natural(char *p);
int newcode(struct node *p);
struct node * nonlab(struct node *ap);
int okio(char *p);
struct optab * oplook(void);
int opsetup(void);
void output(void);
void redunbr(struct node *p);
int refcount(void);
int repladdr(struct node *p);
int rmove(void);
void savereg(int ai, char *s, int type);
void setcc(char *ap, int type);
void setcon(char *cv, char *cl, int type);
int source(char *ap);
int splitrand(struct node *p);
int xjump(struct node *p1);

int main(int argc, char **argv)
{
	register int niter, maxiter, isend;
	int nflag,infound;

	nflag = 0; infound=0; argc--; argv++;
	while (argc>0) {/* get flags */
		if (**argv=='+') debug++;
		else if (**argv=='-') {
			if ((*argv)[1]=='i') ioflag++; else nflag++;
		} else if (infound==0) {
			if (freopen(*argv, "r", stdin) ==NULL) {
				fprintf(stderr,"C2: can't find %s\n", *argv);
				exit(1);
			}
			setbuf(stdin,_sibuf); ++infound;
		} else if (freopen(*argv, "w", stdout) ==NULL) {
			fprintf(stderr,"C2: can't create %s\n", *argv);
			exit(1);
		}
		setbuf(stdout,_sobuf);
		argc--; argv++;
	}
	opsetup();
	arena_mark();
	maxiter = 0;
	do {
		isend = input();
		niter = 0;
		bmove();
		do {
			refcount();
			do {
				iterate();
				clearreg();
				niter++;
			} while (nchange);
			comjump();
			rmove();
		} while (nchange || jumpsw());
		addsob();
		output();
		if (niter > maxiter)
			maxiter = niter;
		arena_release();
	} while (isend);
	if (nflag) {
		fprintf(stderr,"%d iterations\n", maxiter);
		fprintf(stderr,"%d jumps to jumps\n", nbrbr);
		fprintf(stderr,"%d inst. after jumps\n", iaftbr);
		fprintf(stderr,"%d jumps to .+1\n", njp1);
		fprintf(stderr,"%d redundant labels\n", nrlab);
		fprintf(stderr,"%d cross-jumps\n", nxjump);
		fprintf(stderr,"%d code motions\n", ncmot);
		fprintf(stderr,"%d branches reversed\n", nrevbr);
		fprintf(stderr,"%d redundant moves\n", redunm);
		fprintf(stderr,"%d simplified addresses\n", nsaddr);
		fprintf(stderr,"%d loops inverted\n", loopiv);
		fprintf(stderr,"%d redundant jumps\n", nredunj);
		fprintf(stderr,"%d common seqs before jmp's\n", ncomj);
		fprintf(stderr,"%d skips over jumps\n", nskip);
		fprintf(stderr,"%d sob's added\n", nsob);
		fprintf(stderr,"%d redundant tst's\n", nrtst);
		fprintf(stderr,"%d jump on bit\n", nbj);
		fprintf(stderr,"%d field operations\n", nfield);
	}
	fflush(stdout); exit(0);
}

int input(void)
{
	register struct node *p, *lastp;
	struct optab *op; register char *cp1;
	static struct optab F77JSW = {".long", T(JSW,1)};

	lastp = &first;
	for (;;) {
	  top:
		op = c2getline();
		if (debug && op==0) fprintf(stderr,"? %s\n",line);
		switch (op->opcode&0377) {
	
		case LABEL:
			p = alloc(sizeof first);
			if (line[0] == 'L') {
				p->combop = LABEL;
				p->labno = getnum(line+1);
				if (isn<=p->labno) isn=1+p->labno;
				p->code = 0;
			} else {
				p->combop = DLABEL;
				p->labno = 0;
				p->code = copy(line);
			}
			break;
	
		case LGEN:
			if (*curlp!='L') goto std;
			op= &F77JSW;
		case JBR:
			if (op->opcode==T(JBR,RET)) goto std;
		case CBR:
		case JMP:
		case JSW:
		case SOBGEQ: case SOBGTR: case AOBLEQ: case AOBLSS: case ACB:
			p = alloc(sizeof first);
			p->combop = op->opcode; p->code=0; cp1=curlp;
			if (*cp1!='L' || 0==(p->labno = getnum(cp1+1))) {/* jbs, etc.? */
				while (*cp1++); while (*--cp1!=',' && cp1!=curlp);
				if (cp1==curlp || *++cp1!='L' || 0==(p->labno=getnum(cp1+1)))
					p->labno = 0;
				else *--cp1=0;
				p->code = copy(curlp);
			}
			if (isn<=p->labno) isn=1+p->labno;
			break;

		case MOVA:
			p=alloc(sizeof first);
			p->combop=op->opcode; p->code=0; cp1=curlp;
			if (*cp1++=='L') {
				while (*cp1++!=','); *--cp1=0;
				if (0!=(p->labno=getnum(curlp+1))) p->code=copy(cp1+1);
				else {*cp1=','; p->code=copy(curlp);}
			} else p->code=copy(--cp1);
			break;

		case SET:
			printf("%s\n",line); goto top;

		case BSS:
		case DATA:
			for (;;) {
				printf("%s%c",line,(op->opcode==LABEL ? ':' : '\n'));
				if (op->opcode==TEXT) goto top;
				if (END==(op=c2getline())->opcode) {/* dangling .data is bad for you */
					printf(".text\n");
					break;
				}
			}

		std:
		default:
			p = alloc(sizeof first);
			p->combop = op->opcode;
			p->labno = 0;
			p->code = copy(curlp);
			break;

		}
		p->forw = 0;
		p->back = lastp;
		p->pop = op;
		lastp->forw = p;
		lastp = p;
		p->ref = 0;
		if (p->op==CASE) {
			char *lp; int ncase;
			lp=curlp; while (*lp++); while (*--lp!='$'); ncase=getnum(lp+1);
			if (LABEL!=(c2getline())->opcode) c2abort(-2);
			do {
				if (WGEN!=(c2getline())->opcode) c2abort(-3);
				p = alloc(sizeof first); p->combop = JSW; p->code = 0;
				lp=curlp; while(*lp++!='-'); *--lp=0; p->labno=getnum(curlp+1);
				if (isn<=p->labno) isn=1+p->labno;
				p->forw = 0; p->back = lastp; lastp->forw = p; lastp = p;
				p->ref = 0; p->pop=0;
			} while (--ncase>=0);
		}
		if (op->opcode==EROU)
			return(1);
		if (op->opcode==END)
			return(0);
	}
}

struct optab * c2getline(void)
{
	register char *lp;
	register int c;
	static struct optab OPLABEL={"",LABEL};
	static struct optab OPEND={"",END};

	lp = line;
	curlp = line;	/* the end and a label have no operands: not NULL */
	while (EOF!=(c=getchar()) && isspace(c));
	while (EOF!=c) {
		if (c==':') {
			*lp++ = 0;
			return(&OPLABEL);
		}
		if (c=='\n') {
			*lp++ = 0;
			return(oplook());
		}
		*lp++ = c;
		c = getchar();
	}
	*lp++ = 0;
	return(&OPEND);
}

int32_t getnum(char *p)
{
	register int c; int neg; register int32_t n;

	n = 0; neg=0; if (*p=='-') {++neg; ++p;}
	while (isdigit(c = *p++)) {
		 c -= '0'; n *= 10; if (neg) n -= c; else n += c;
	}
	if (*--p != 0)
		return(0);
	return(n);
}

void output(void)
{
	register struct node *t;
	int casebas;

	t = &first;
	while (t = t->forw) switch (t->op) {

	case END:
		return;

	case LABEL:
		printf("L%d:", t->labno);
		continue;

	case DLABEL:
		printf("%s:", t->code);
		continue;

	case CASE:
		casebas=0;

	default: std:
		if (t->pop==0) {/* must find it */
			register struct optab *p;
			for (p=optab; p->opstring[0]; ++p)
				if (p->opcode==t->combop) {t->pop=p; break;}
		}
		printf("%s", t->pop->opstring);
		if (t->code) printf("\t%s", t->code);
		if (t->labno!=0) printf("%cL%d\n",
							(t->code ? ',' : '\t'),
							t->labno);
		else printf("\n");
		continue;

	case MOVA:
		if (t->labno==0) goto std;
		printf("mova%c\tL%d,%s\n","bwlq"[t->subop-BYTE],t->labno,t->code);
		continue;

	case JSW:
		if (t->subop!=0) {/* F77JSW */
			printf(".long\tL%d\n",t->labno); continue;
		}
		if (casebas==0) printf("L%d:\n",casebas=isn++);
		printf(".word	L%d-L%d\n", t->labno, casebas);
		continue;

	}
}

char * copy(char *ap)
{
	char *np;

	if (*ap == 0)
		return(0);
	np = alloc(strlen(ap) + 1);
	return strcpy(np, ap);
}

#define	OPHS	560
struct optab *ophash[OPHS];

int opsetup(void)
{
	register struct optab *optp, **ophp;
	register int i,t;

	for(i=NREG+5;--i>=0;) regs[i]=alloc(20);
	for (optp = optab; optp->opstring[0]; optp++) {
		t=7; i=0; while (--t>=0) i+= i+optp->opstring[t];
		ophp = &ophash[i % OPHS];
		while (*ophp++) {
/*			fprintf(stderr,"\ncollision: %d %s %s",
/*				ophp-1-ophash,optp->opstring,(*(ophp-1))->opstring);
*/
			if (ophp >= &ophash[OPHS])
				ophp = ophash;
		}
		*--ophp = optp;
	}
}

struct optab * oplook(void)
{
	register struct optab *optp,**ophp;
	register char *p,*p2;
	register int t;
	char tempop[20];
	static struct optab OPNULL={"",0};

	for (p=line, p2=tempop; *p && !isspace(*p); *p2++= *p++); *p2=0; p2=p;
	while (isspace(*p2)) ++p2; curlp=p2;
	t=0; while(--p>=line) t += t+*p; ophp = &ophash[t % OPHS];
	while (optp = *ophp) {
		if (equstr(tempop,optp->opstring)) return(optp);
		if ((++ophp) >= &ophash[OPHS]) ophp = ophash;
	}
	curlp = line;
	return(&OPNULL);
}

int refcount(void)
{
	register struct node *p, *lp;
	struct node *labhash[LABHS];
	register struct node **hp;
	struct node *hn;

	for (hp = labhash; hp < &labhash[LABHS];)
		*hp++ = 0;
	for (p = first.forw; p!=0; p = p->forw)
		if (p->op==LABEL) {
			labhash[p->labno % LABHS] = p;
			p->refc = 0;
		}
	for (p = first.forw; p!=0; p = p->forw) {
		if (p->op==JBR || p->op==CBR || p->op==JSW
		  || p->op==SOBGEQ || p->op==SOBGTR || p->op==AOBLEQ || p->op==AOBLSS
		  || p->op==ACB || (p->op==MOVA && p->labno!=0)) {
			p->ref = 0;
			lp = labhash[p->labno % LABHS];
			if (lp==0 || p->labno!=lp->labno)
			for (lp = first.forw; lp!=0; lp = lp->forw) {
				if (lp->op==LABEL && p->labno==lp->labno)
					break;
			}
			if (lp) {
				hn = nonlab(lp)->back;
				if (hn!=lp) {
					p->labno = hn->labno;
					lp = hn;
				}
				p->ref = lp;
				lp->refc++;
			}
		}
	}
	for (p = first.forw; p!=0; p = p->forw)
		if (p->op==LABEL && p->refc==0
		 && (lp = nonlab(p))->op && lp->op!=JSW)
			decref(p);
}

int iterate(void)
{
	register struct node *p, *rp, *p1;

	nchange = 0;
	for (p = first.forw; p!=0; p = p->forw) {
		if ((p->op==JBR||p->op==CBR||p->op==JSW) && p->ref) {
			rp = nonlab(p->ref);
			if (rp->op==JBR && rp->labno && p->labno!=rp->labno) {
				nbrbr++;
				p->labno = rp->labno;
				decref(p->ref);
				rp->ref->refc++;
				p->ref = rp->ref;
				nchange++;
			}
		}
		if (p->op==CBR && (p1 = p->forw)->combop==JBR) {/* combop: RET problems */
			rp = p->ref;
			do
				rp = rp->back;
			while (rp->op==LABEL);
			if (rp==p1) {
				decref(p->ref);
				p->ref = p1->ref;
				p->labno = p1->labno;
				p1->forw->back = p;
				p->forw = p1->forw;
				p->subop = revbr[p->subop];
				p->pop=0;
				nchange++;
				nskip++;
			}
		}
		if (p->op==JBR || p->op==JMP) {
			while (p->forw && p->forw->op!=LABEL && p->forw->op!=DLABEL
				&& p->forw->op!=EROU && p->forw->op!=END
				&& p->forw->op!=ALIGN
				&& p->forw->op!=0 && p->forw->op!=DATA) {
				nchange++;
				iaftbr++;
				if (p->forw->ref)
					decref(p->forw->ref);
				p->forw = p->forw->forw;
				p->forw->back = p;
			}
			rp = p->forw;
			while (rp && rp->op==LABEL) {
				if (p->ref == rp) {
					p->back->forw = p->forw;
					p->forw->back = p->back;
					p = p->back;
					decref(rp);
					nchange++;
					njp1++;
					break;
				}
				rp = rp->forw;
			}
			xjump(p);
			p = codemove(p);
		}
	}
}

int xjump(struct node *p1)
{
	register struct node *p2, *p3;
	int nxj;

	nxj = 0;
	if ((p2 = p1->ref)==0)
		return(0);
	for (;;) {
		while ((p1 = p1->back) && p1->op==LABEL);
		while ((p2 = p2->back) && p2->op==LABEL);
		if (!equop(p1, p2) || p1==p2)
			return(nxj);
		p3 = insertl(p2);
		p1->combop = JBR;
		p1->pop=0;
		p1->ref = p3;
		p1->labno = p3->labno;
		p1->code = 0;
		nxj++;
		nxjump++;
		nchange++;
	}
}

struct node * insertl(struct node *op)
{
	register struct node *lp;

	if (op->op == LABEL) {
		op->refc++;
		return(op);
	}
	if (op->back->op == LABEL) {
		op = op->back;
		op->refc++;
		return(op);
	}
	lp = alloc(sizeof first);
	lp->combop = LABEL;
	lp->labno = isn++;
	lp->ref = 0;
	lp->code = 0;
	lp->refc = 1;
	lp->back = op->back;
	lp->forw = op;
	op->back->forw = lp;
	op->back = lp;
	return(lp);
}

struct node * codemove(struct node *ap)
{
	register struct node *p1, *p2, *p3;
	struct node *t, *tl;
	int n;

	p1 = ap;
	if (p1->op!=JBR || (p2 = p1->ref)==0)
		return(p1);
	while (p2->op == LABEL)
		if ((p2 = p2->back) == 0)
			return(p1);
	if (p2->op!=JBR && p2->op!=JMP)
		goto ivloop;
	p2 = p2->forw;
	p3 = p1->ref;
	while (p3) {
		if (p3->op==JBR || p3->op==JMP) {
			if (p1==p3)
				return(p1);
			ncmot++;
			nchange++;
			p1->back->forw = p2;
			p1->forw->back = p3;
			p2->back->forw = p3->forw;
			p3->forw->back = p2->back;
			p2->back = p1->back;
			p3->forw = p1->forw;
			decref(p1->ref);
			return(p2);
		} else
			p3 = p3->forw;
	}
	return(p1);
ivloop:
	if (p1->forw->op!=LABEL)
		return(p1);
	p3 = p2 = p2->forw;
	n = 16;
	do {
		if ((p3 = p3->forw) == 0 || p3==p1 || --n==0)
			return(p1);
	} while (p3->op!=CBR || p3->labno!=p1->forw->labno);
	do 
		if ((p1 = p1->back) == 0)
			return(ap);
	while (p1!=p3);
	p1 = ap;
	tl = insertl(p1);
	p3->subop = revbr[p3->subop];
	p3->pop=0;
	decref(p3->ref);
	p2->back->forw = p1;
	p3->forw->back = p1;
	p1->back->forw = p2;
	p1->forw->back = p3;
	t = p1->back;
	p1->back = p2->back;
	p2->back = t;
	t = p1->forw;
	p1->forw = p3->forw;
	p3->forw = t;
	p2 = insertl(p1->forw);
	p3->labno = p2->labno;
	p3->ref = p2;
	decref(tl);
	if (tl->refc<=0)
		nrlab--;
	loopiv++;
	nchange++;
	return(p3);
}

int comjump(void)
{
	register struct node *p1, *p2, *p3;

	for (p1 = first.forw; p1!=0; p1 = p1->forw)
		if (p1->op==JBR && ((p2 = p1->ref) && p2->refc > 1 || p1->subop==RET))
			for (p3 = p1->forw; p3!=0; p3 = p3->forw)
				if (p3->op==JBR && p3->ref == p2)
					backjmp(p1, p3);
}

void backjmp(struct node *ap1, struct node *ap2)
{
	register struct node *p1, *p2, *p3;

	p1 = ap1;
	p2 = ap2;
	for(;;) {
		while ((p1 = p1->back) && p1->op==LABEL);
		p2 = p2->back;
		if (equop(p1, p2)) {
			p3 = insertl(p1);
			p2->back->forw = p2->forw;
			p2->forw->back = p2->back;
			p2 = p2->forw;
			decref(p2->ref);
			p2->combop = JBR; /* to handle RET */
			p2->pop=0;
			p2->labno = p3->labno;
			p2->ref = p3;
			nchange++;
			ncomj++;
		} else
			return;
	}
}

#
char C21[] = {"@(#)c21.c 1.59 79/01/20 11:07:52"}; /* sccs ident */
/*
 * C object code improver-- second part
 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#define NUSE 6
int ioflag;
int biti[NUSE] = {1,2,4,8,16,32};
int bitsize[4] = {0,8,16,32}; /* index by type codes */
int pos,siz; int32_t f; /* for bit field communication */
struct node *uses[NUSE]; /* for backwards flow analysis */
char *lastrand; /* last operand of instruction */

int redun3(struct node *p, int split){
/* check for 3 addr instr which should be 2 addr */
	if (OP3==((p->subop>>4)&0xF)) {
		if (split) splitrand(p);
		if (equstr(regs[RT1],regs[RT3])
		  && (p->op==ADD || p->op==MUL || p->op==BIS || p->op==XOR)) {
			register char *t=regs[RT1]; regs[RT1]=regs[RT2]; regs[RT2]=t;
		}
		if (equstr(regs[RT2],regs[RT3])) {
			p->subop=(p->subop&0xF)|(OP2<<4); p->pop=0;
			lastrand=regs[RT2]; *regs[RT3]=0; return(1);
		}
	} return(0);
}

int bmove(void){
	register struct node *p, *lastp; register char *cp1,*cp2; register int r;
	refcount();
	for (p=lastp= &first; 0!=(p=p->forw); lastp=p);
	clearreg(); clearuse(); findlive();
	for (p=lastp; p!= &first; p=p->back) {
	if (debug) {
		for (r=NUSE;--r>=0;) if (uses[r]) printf("%d: %s\n",r,uses[r]->code);
		printf("-\n");
	}
	if (OP3==((p->subop>>4)&0xF) && 0!=redun3(p,1)) {newcode(p); redunm++;}
	switch (p->op) {
	case LABEL: case DLABEL:
	if (p->back && p->back->op==JBR && p->back->subop!=RET)
		for (r=NUSE; --r>=0;) if (uses[r] && regs[r][0]!= -1)
			p->ref=(struct node *) (((uintptr_t)p->ref)|biti[r]);
		liveall(p, 1); break;
	case CALLS: case 0:
		clearuse(); break;
	case SUB:
		if ((p->subop&0xF)!=LONG) goto std; cp1=p->code;
		if (*cp1++!='$') goto std; splitrand(p);
		if (equstr(regs[RT2],"fp") && !indexa(regs[RT1])) {/* address comp. */
			char buf[50]; cp2=buf; *cp2++='-'; 
			cp1=regs[RT1]+1; while (*cp2++= *cp1++); --cp2;
			cp1="(fp),"; while (*cp2++= *cp1++); --cp2;
			cp1=regs[RT3]; while (*cp2++= *cp1++);
			p->code=copy(buf); p->combop=T(MOVA,LONG); p->pop=0;
		} else if (*cp1++=='-' && 0<=(r=getnum(cp1))) {
			p->op=ADD; p->pop=0; *--cp1='$'; p->code=cp1;
		} goto std;
	case ADD:
		if ((p->subop&0xF)!=LONG) goto std; cp1=p->code;
		if (*cp1++!='$') goto std; splitrand(p);
		if (equstr(regs[RT2],"fp") && !indexa(regs[RT1])) {/* address comp. */
			cp2=cp1-1; cp1=regs[RT1]+1; while (*cp2++= *cp1++); --cp2;
			cp1="(fp)"; while (*cp2++= *cp1++); *--cp2=',';
			p->combop=T(MOVA,LONG); p->pop=0;
		} else if (*cp1++=='-' && 0<=(r=getnum(cp1))) {
			p->op=SUB; p->pop=0; *--cp1='$'; p->code=cp1;
		}
	case CASE:
	default: std:
		p=bflow(p); break;
	case JBR:
		if (p->subop==RET) {uses[0]=p; regs[0][0]= -1; break;}
		else if (p->ref->ref!=0) for (r=NUSE;--r>=0;)
			if (biti[r] & (uintptr_t)p->ref->ref) {uses[r]=p; regs[r][0]= -1;}
		liveall(p, 0); break;
	case CBR: case JMP: case JSW:
		liveall(p, 0); break;
	case EROU:
	case TEXT: case DATA: case BSS: case ALIGN: case WGEN: case END: ;
	}
	}
}

int rmove(void)
{
	register struct node *p, *lastp;
	register int r;
	int r1;

	clearreg();
	for (p=first.forw; p!=0; p = p->forw) {
	lastp=p;
	if (debug) {
		for (r=0; r<NREG; r++)
			if (regs[r][0]) {
				r1=regs[r][0];
				printf("%d: %d%d %s\n", r, r1&0xF, r1>>4, regs[r]+1);
			}
		printf("-\n");
	}
	switch (p->op) {

	case CVT:
		splitrand(p); goto mov;

	case MOV:
		splitrand(p);
		if ((r = findrand(regs[RT1],p->subop)) >= 0) {
			if (r == isreg(regs[RT2]) && p->forw->op!=CBR) {
				delnode(p); redunm++; break;
			}
		}
	mov:
		repladdr(p);
		r = isreg(regs[RT1]);
		r1 = isreg(regs[RT2]);
		dest(regs[RT2],p->subop);
		if (r >= 0)
			if (r1 >= 0) savereg(r1, regs[r]+1, p->subop);
			else savereg(r, regs[RT2], p->subop);
		else
			if (r1 >= 0) savereg(r1, regs[RT1], p->subop);
			else setcon(regs[RT1], regs[RT2], p->subop);
		break;

/* .rx,.wx */
	case COM:
	case NEG:
/* .rx,.wx or .rx,.rx,.wx */
	case ADD:
	case SUB:
	case BIC:
	case BIS:
	case XOR:
	case MUL:
	case DIV:
	case ASH:
	case MOVZ:
/* .rx,.rx,.rx,.wx */
	case EXTV:
	case EXTZV:
	case INSV:
		splitrand(p);
		repladdr(p);
		dest(lastrand,p->subop);
		if (p->op==INSV) ccloc[0]=0;
		break;

/* .mx or .wx */
	case CLR:
	case INC:
	case DEC:
		splitrand(p);
		dest(lastrand,p->subop);
		if (p->op != CLR) {
			/* incX and decX leave the codes of the result, as tstX
			 * would (the branches that look at V and C differ) */
			setcc(lastrand,p->subop);
			ccarith = 1;
		}
		if (p->op==CLR)
			if ((r = isreg(regs[RT1])) >= 0)
				savereg(r, "$0", p->subop);
			else
				setcon("$0", regs[RT1], p->subop);
		break;

/* .rx */
	case TST:
	case PUSH:
		splitrand(p);
		lastrand=regs[RT1+1]; /* fool repladdr into doing 1 operand */
		repladdr(p);
		/* decl x; tstl x with nothing between them: the same codes
		 * (even for operands ccloc will not trust, like *sym) */
		if (p->op==TST && p->back->op>=INC && p->back->op<=DEC
		  && p->back->subop==p->subop && p->back->code
		  && equstr(regs[RT1], p->back->code) && !source(regs[RT1])
		  && p->forw->op==CBR && p->forw->subop<=JGT) {
			delnode(p); p = p->back; nrtst++; nchange++;
			setcc(regs[RT1],p->subop); ccarith = 1;
			break;
		}
		if (p->op==TST && equstr(lastrand=regs[RT1], ccloc+1)
		  && equtype(ccloc[0],p->subop) &&!source(lastrand)
		  && (!ccarith || (p->forw->op==CBR && p->forw->subop<=JGT))) {
			delnode(p); p = p->back; nrtst++; nchange++;
		}
		setcc(lastrand,p->subop);
		break;

/* .rx,.rx,.rx */
	case CASE:
/* .rx,.rx */
	case CALLS:
	case CMP:
	case BIT:
		splitrand(p);
		/* fool repladdr into doing right number of operands */
		if (p->op!=CASE) lastrand=regs[RT3]; else lastrand=regs[RT4];
		repladdr(p);
		if (p->op==CALLS) clearreg();
		if (p->op==BIT) bitopt(p);
		ccloc[0]=0; break;

	case CBR:
		if (p->subop>=JBC) {/* 2 operands can be optimized */
			splitrand(p); lastrand=regs[RT3]; repladdr(p);
		}
		ccloc[0] = 0;
		break;

	case JBR:
		redunbr(p);

/* .wx,.bb */
	case SOB:

	default:
		clearreg();
	}
	}
}

char * byondrd(struct node *p){
/* return pointer to register which is "beyond last read/modify operand" */
	if (OP2==(p->subop>>4)) return(regs[RT3]);
	switch (p->op) {
		case PUSHA:
		case TST: case INC: case DEC: case PUSH:	return(regs[RT2]);
		case BIT: case CMP: case CALLS:				return(regs[RT3]);
		case CASE:									return(regs[RT4]);
	}
	return(lastrand);
}

static int fwdaddr(struct node *p, int rb, int ra);


/*
 * p is  movl rA,rB  and the next instruction reads rB only inside addresses.
 * Rewrite that instruction to use rA and delete p.  Returns 1 if done.
 */
static int
fwdaddr(struct node *p, int rb, int ra)
{
	struct node *f = p->forw;
	char src[8], pat[8], *q, *w, **op;
	char buf[sizeof(line)];
	int n = 0, nb, i;

	if (f == 0 || f->code == 0 || f->op==LABEL || f->op==DLABEL || f->op==CALLS ||
	    f->op==CBR || f->op==JBR || f->op==JMP || f->op==JSW || f->op==CASE)
		return 0;
	snprintf(src, sizeof src, "r%d", ra);
	snprintf(pat, sizeof pat, "r%d", rb);
	splitrand(f);
	lastrand = byondrd(f);
	for (op = regs+RT1; *op != lastrand && **op; op++) {
		if (isreg(*op) == rb)
			return 0;		/* a plain read of rB */
	}
	/* every operand, the destination too */
	nb = 0;
	for (op = regs+RT1; **op; op++) {
		q = *op;
		/* a side effect on rA or rB in an address: leave it */
		if (strstr(q, "(r") && (strchr(q, '+') || q[0]=='-'))
			return 0;
		if (isreg(q) == rb && *op != lastrand)
			return 0;
		for (w = q; *w; w++) {
			if ((*w == '(' || *w == '[') && w[1] == 'r') {
				int k = strlen(pat);
				if (strncmp(w+1, pat, k) == 0 && (w[1+k] == ')' || w[1+k] == ']'))
					nb++;
			}
		}
	}
	if (nb == 0)
		return 0;
	for (op = regs+RT1; **op; op++) {
		q = *op; i = 0;
		for (w = q; *w; w++) {
			if ((*w == '(' || *w == '[') && w[1] == 'r' &&
			    strncmp(w+1, pat, strlen(pat)) == 0 &&
			    (w[1+strlen(pat)] == ')' || w[1+strlen(pat)] == ']')) {
				buf[i++] = *w;
				memcpy(buf+i, src, strlen(src)); i += strlen(src);
				w += strlen(pat);
				continue;
			}
			buf[i++] = *w;
		}
		buf[i] = 0;
		if (i >= 20)
			return 0;		/* regs[] strings are small */
		strcpy(q, buf);
	}
	(void)n;
	newcode(f);
	delnode(p);
	redunm++;
	uses[rb] = 0; (void)memset(regs[rb], 0, 2);
	return 1;
}

struct node * bflow(struct node *p){
	register char *cp1,*cp2,**preg; register int r;
	int i_ra;
	splitrand(p);
	/* movab sym,r is moval sym,r unless it is indexed: an address is an address */
	if (p->op==MOVA && p->subop==BYTE && !indexa(regs[RT1])) {
		p->subop=LONG; p->pop=0;
	}
	/*
	 * movl rA,rB followed only by an instruction that uses rB inside an
	 * address, (rB) d(rB) [rB]: use rA there and drop the move.
	 */
	if (p->op==MOV && p->subop==LONG && 0<=(r=isreg(lastrand)) && r<NUSE &&
	  uses[r]==p->forw && (regs[r][0]==(char)(OPB<<4) || regs[r][0]==(char)(OPX<<4)) &&
	  0<=(i_ra=isreg(regs[RT1])) && i_ra!=r && i_ra<NUSE+6 && fwdaddr(p,r,i_ra))
		p=p->forw;
	if (0<=(r=isreg(lastrand)) && r<NUSE && p->op!=PUSH &&
	  uses[r]==p->forw && p->subop &&
	  (equtype(regs[r][0],p->subop) ||
	  /* cvtbl/movzbl: what is written is the destination type, not the source's */
	  ((p->op==CVT || p->op==MOVZ) && ((p->subop>>4)&0xF)==(regs[r][0]&0xF)))) {
		if (regs[r][1]!=0) {/* send directly to destination */
			if (p->op==INC || p->op==DEC) {
				if (p->op==DEC) p->op=SUB; else p->op=ADD;
				p->subop=(OP2<<4)+(p->subop&0xF); /* use 2 now, convert to 3 later */
				p->pop=0;
				cp1=lastrand; cp2=regs[RT2]; while (*cp2++= *cp1++); /* copy reg */
				cp1=lastrand; *cp1++='$'; *cp1++='1'; *cp1=0;
			}
			cp1=regs[r]+1; cp2=lastrand;
			if (OP2==(p->subop>>4)) {/* use 3 operand form of instruction */
				p->pop=0;
				p->subop += (OP3-OP2)<<4; lastrand=cp2=regs[RT3];
			}
			while (*cp2++= *cp1++);
			if (p->op==MOVA && p->forw->op==PUSH) {
				p->op=PUSHA; *regs[RT2]=0; p->pop=0;
				if (p->labno) {	/* mova Ln,r: the label is not in the code */
					sprintf(regs[RT1],"L%d",p->labno);
					p->labno=0;
				}
			}
			delnode(p->forw);
			redun3(p,0);
			newcode(p); redunm++; uses[r]=0; (void)memset(regs[r],0,2);
		} else if (p->op==MOV && p->forw->op!=EXTV && p->forw->op!=EXTZV
		  && p->forw->op!=INC && p->forw->op!=DEC /* modify their operand */
		  && p->forw->op!=SOBGEQ && p->forw->op!=SOBGTR && p->forw->op!=AOBLEQ
		  && p->forw->op!=AOBLSS && p->forw->op!=ACB) {
			/* superfluous fetch */
			int nmatch;
			char src[20]; cp2=src; cp1=regs[RT1]; while (*cp2++= *cp1++);
			int modreg;	/* is the eliminated register the one p->forw modifies? */
			splitrand(p->forw); lastrand=byondrd(p->forw); nmatch=0;
			modreg = OP2==(p->forw->subop>>4) && r==isreg(regs[RT2]);
			for (preg=regs+RT1;*preg!=lastrand;preg++) if (r==isreg(*preg)) {
				cp2= *preg; cp1=src; while (*cp2++= *cp1++); ++nmatch;
			}
			if (nmatch==1) {
				if (modreg && equstr(src,regs[RT2])) {
					p->forw->pop=0;
					p->forw->subop += (OP3-OP2)<<4; cp1=regs[RT3];
					*cp1++='r'; *cp1++=r+'0'; *cp1=0;
				}
				delnode(p); p=p->forw;
				redun3(p,0);
				newcode(p); redunm++; uses[r]=0; (void)memset(regs[r],0,2);
			}
		}
	}
	/* adjust 'lastrand' past any 'read' or 'modify' operands. */
	lastrand=byondrd(p);
	/* a 'write' clobbers the register. */
	if (0<=(r=isreg(lastrand)) && r<NUSE
	  || OP2==(p->subop>>4) && 0<=(r=isreg(regs[RT2])) && r<NUSE)
		{uses[r]=0; (void)memset(regs[r],0,2);}
	if (p->op==BIC) {p=bicopt(p); splitrand(p); lastrand=byondrd(p);}
	if (p->op==BIS) bixprep(p,JBSS);
	/* now look for 'read' or 'modify' (read & write) uses */
	preg=regs+RT1; 
	while (*(cp1= *preg++)) {
		/* check for  r  */
		if (lastrand!=cp1 && 0<=(r=isreg(cp1)) && r<NUSE && uses[r]==0) {
			uses[r]=p; cp2=regs[r]; *cp2++=p->subop;
			if (p->op==MOV || p->op==PUSH) {
				if (p->op==PUSH) cp1="-(sp)"; else cp1=regs[RT2];
				while (*cp2++= *cp1++);
			} else *cp2++=0;
			continue;
		}
		/* check for (r),(r)+,-(r),[r] */
		do if (*cp1=='(' || *cp1=='[') {/* get register number */
			char t;
			cp2= ++cp1; while (*++cp1!=')' && *cp1!=']'); t= *cp1; *cp1=0;
			if (0<=(r=isreg(cp2)) && r<NUSE && (uses[r]==0 || uses[r]==p)) {
				uses[r]=p; regs[r][0]=(*--cp2=='[' ? OPX<<4 : OPB<<4);
			}
			*cp1=t;
		} while (*++cp1);
	}
	/* pushax or movax possibility? */
	cp1=regs[RT1];
	if (*cp1++=='$'
	  && (*cp1=='_' || *cp1=='L' || (*cp1++=='v' && *cp1=='.'))
	  && natural(regs[RT1])) {
		if (p->combop==T(MOV,LONG)) {
			if (regs[RT1][1]=='L' && 0!=(p->labno=getnum(regs[RT1]+2))) {
				cp1=p->code; while (*cp1++!=','); p->code= --cp1;
			}
			p->combop=T(MOVA,LONG); ++p->code; p->pop=0;
		} else if (p->combop==T(PUSH,LONG)) {
			p->combop=T(PUSHA,LONG); ++p->code; p->pop=0;
		} else if ((p->combop&0xFFFF)==T(ADD,U(LONG,OP3))
				 && 0<=(r=isreg(regs[RT2]))) {
			cp1=cp2=p->code; ++cp1;
			do *cp2++= *cp1; while (*cp1++!=','); cp2[-1]='[';
			do *cp2++= *cp1; while (*cp1++!=','); cp2[-1]=']';
			if (!equstr(regs[RT3],"-(sp)")) p->combop=T(MOVA,BYTE);
			else {p->combop=T(PUSHA,BYTE); *cp2=0;}
			if (uses[r]==0) {uses[r]=p; regs[r][0]=OPX<<4;}
			p->pop=0;
		}
	}
	/* movl src,-(sp) is pushl src */
	if (p->op==MOV && (p->subop&0xF)==LONG) {
		splitrand(p);
		if (equstr(regs[RT2],"-(sp)") && !*regs[RT3] && *regs[RT1]) {
			*regs[RT2]=0;
			p->op=PUSH; p->subop=LONG; p->pop=0;
			newcode(p); redunm++;
		}
	}
	/* addX2 $1,dst and subX2 $1,dst are incX dst and decX dst (cc.: 32V's
	 * ccom wrote these itself; ours leaves it to the optimizer) */
	if ((p->op==ADD || p->op==SUB) && OP2==(p->subop>>4)
	  && (p->subop&0xF)>=BYTE && (p->subop&0xF)<=LONG) {
		splitrand(p);
		if (equstr(regs[RT1],"$1") && *regs[RT2] && !*regs[RT3]) {
			cp1=regs[RT2]; cp2=regs[RT1]; while (*cp2++= *cp1++);
			*regs[RT2]=0;
			p->op = (p->op==ADD ? INC : DEC);
			p->subop &= 0xF; p->pop=0;
			newcode(p); redunm++;
		}
	}
	return(p);
}

int ispow2(int32_t n){/* -1 -> no; else -> log to base 2 */
	register int log;
	if (n==0 || n&(n-1)) return(-1); log=0;
	for (;;) {n >>= 1; if (n==0) return(log); ++log; if (n== -1) return(log);}
}

void bitopt(struct node *p){
	/* change "bitx $<power_of_2>,a" followed by JEQ or JNE
	/* into JBC or JBS.  watch out for I/O registers. (?)
	/* assumes that 'splitrand' has already been called.
	*/
	register char *cp1,*cp2; int b;
	cp1=regs[RT1]; cp2=regs[RT2];
	if (*cp1++!='$' || !okio(cp2) || p->forw->op!=CBR || p->forw->subop&-2 ||
		0>(b=ispow2(getnum(cp1))) || p->subop!=BYTE && indexa(cp2)) return;
	if (b>=bitsize[p->subop]) {/* you dummy! */
		if (source(cp2)) {/* side effect: auto increment or decrement */
			p->pop=0;
			p->op=TST; --cp1; while (*cp1++= *cp2++);
			regs[RT2][0]=0; newcode(p);
		} else delnode(p);
		p = p->forw;
		if (p->subop==JEQ) {p->combop=JBR; p->pop=0;}
		else delnode(p);
		nchange++; nbj++; return;
	}
	if (cp1=p->forw->code) {/* destination is not an internal label */
		cp2=regs[RT3]; while (*cp2++= *cp1++);
	}
	if (b==0 && (p->subop==LONG || !indexa(regs[RT2]))) {/* JLB optimization, ala BLISS */
		cp2=regs[RT1]; cp1=regs[RT2]; while (*cp2++= *cp1++);
		cp2=regs[RT2]; cp1=regs[RT3]; while (*cp2++= *cp1++);
		*(regs[RT3])=0; p->forw->subop += JLBC-JBC;
		p->forw->pop=0;
	} else {
		cp1=regs[RT1]+1;
		if (b>9) *cp1++= b/10 +'0'; *cp1++= b%10 +'0'; *cp1=0; /* $<bit_number> */
	}
	nbj++; newcode(p); p->combop = p->forw->combop+((JBC-JEQ)<<8);
	p->labno = p->forw->labno; delnode(p->forw);
	p->pop=0;
}

int isfield(int32_t n){/* -1 -> no; else -> position of low bit */
	register int pos; register int32_t t;
	t= ((n-1)|n) +1;
	if (n!=0 && (0==t || 0==n || 0<=ispow2(t))) {
		pos=0; while(!(n&1)) {n >>= 1; ++pos;} return(pos);
	} else return(-1);
}

int bixprep(struct node *p, int bix){
/* initial setup, single-bit checking for bisopt, bicopt.
/* return: 0->don't bother any more; 1->worthwhile trying
*/
	register char *cp1,*cp2;
	splitrand(p); cp1=regs[RT1]; cp2=regs[RT2];
	if (*cp1++!='$' || 0>(pos=isfield(f=getnum(cp1)))
	  || !okio(cp2) || indexa(cp2) || source(cp2) || !okio(lastrand)) return(0);
	f |= f-1; if (++f==0) siz=32-pos; else siz=ispow2(f)-pos;
	if (siz==1 && pos>5 && (p->subop>>4)==OP2 && (p->subop&0xF)!=BYTE
	  && pos<bitsize[p->subop&0xF]) {
		p->ref = insertl(p->forw); p->combop = CBR | (bix<<8);
		p->pop=0;
		p->labno = p->ref->labno;
		if (pos>9) {*cp1++= pos/10 +'0'; pos %= 10;}
		*cp1++=pos+'0'; *cp1=0; newcode(p); nbj++; return(0);
	}
	return(1);
}


struct node * bicopt(struct node *p){
/* use field operations or MOVZ if possible.  done as part of 'bflow'.
*/
	register char *cp1,*cp2; int r;
	char src[50];
	if (!bixprep(p,JBCC)) return(p);
	if (f<=0) {/* the BIC isolates low order bits */
		siz=pos; pos=0;
		if ((p->subop&0xF)==LONG && *(regs[RT2])!='$') {/* result of EXTZV is long */
			/* save source of BICL in 'src' */
			cp1=regs[RT2]; cp2=src; while (*cp2++= *cp1++);
			if (p->back->op==ASH) {/* try for more */
				splitrand(p->back); cp1=regs[RT1]; cp2=regs[RT3];
				if (*cp1++=='$' && *(regs[RT2])!='$' && !indexa(regs[RT2])
				  && 0>(f=getnum(cp1)) && equstr(src,cp2)
				  && 0<=(r=isreg(cp2)) && r<NUSE && uses[r]==0) {/* a good ASH */
					pos -= f; cp1=regs[RT2]; cp2=src; while (*cp2++= *cp1++);
					delnode(p->back);
				}
			}
			if (p->back->op==CVT || p->back->op==MOVZ) {/* greedy, aren't we? */
				splitrand(p->back); cp1=regs[RT1]; cp2=regs[RT2];
				if (equstr(src,cp2) && okio(cp1) && !indexa(cp1)
				  && 0<=(r=isreg(cp2)) && r<NUSE && uses[r]==0
				  && bitsize[p->back->subop&0xF]>=(pos+siz)
				  && bitsize[p->back->subop>>4]>=(pos+siz)) {/* good CVT */
					cp1=regs[RT1]; cp2=src; while (*cp2++= *cp1++);
					delnode(p->back);
				}
			}
			/* 'pos', 'siz' known; source of field is in 'src' */
			splitrand(p); /* retrieve destination of BICL */
			if (siz==8 && pos==0) {
				p->combop = T(MOVZ,U(BYTE,LONG));
				sprintf(line,"%s,%s",src,lastrand);
			} else {
				p->combop = T(EXTZV,LONG);
				sprintf(line,"$%d,$%d,%s,%s",pos,siz,src,lastrand);
			}
			p->pop=0;
			p->code = copy(line); nfield++; return(p);
		}/* end EXTZV possibility */
	}/* end low order bits */
/* unfortunately, INSV clears the condition codes, thus cannot be used */
/*	else {/* see if BICL2 of positive field should be INSV $0 */
/*		if (p->subop==(LONG | (OP2<<4)) && 6<=(pos+siz)) {
/*			p->combop = INSV;
/*			sprintf(line,"$0,$%d,$%d,%s",pos,siz,lastrand);
/*			p->code = copy(line); nfield++; return(p);
/*		}
/*	}
*/
	return(p);
}

int jumpsw(void)
{
	register struct node *p, *p1, *tn;
	register int t;
	int nj;

	t = 0;
	nj = 0;
	for (p=first.forw; p!=0; p = p->forw)
		p->seq = ++t;
	for (p=first.forw; p!=0; p = p1) {
		p1 = p->forw;
		if (p->op == CBR && p1->op==JBR && p->ref && p1->ref
		 && iabs(p->seq - p->ref->seq) > iabs(p1->seq - p1->ref->seq)) {
			if (p->ref==p1->ref)
				continue;
			p->subop = revbr[p->subop];
			p->pop=0;
			tn = p1->ref;
			p1->ref = p->ref;
			p->ref = tn;
			t = p1->labno;
			p1->labno = p->labno;
			p->labno = t;
			nrevbr++;
			nj++;
		}
	}
	return(nj);
}

int addsob(void)
{
	register struct node *p, *p1, *p2, *p3;

	for (p = &first; (p1 = p->forw)!=0; p = p1) {
	if (p->combop==T(DEC,LONG) && p1->op==CBR) {
		if (iabs(p->seq - p1->ref->seq) > 12) continue;
		if (p1->subop==JGE || p1->subop==JGT) {
			if (p1->subop==JGE) p->combop=SOBGEQ; else p->combop=SOBGTR;
			p->pop=0;
			p->labno = p1->labno; delnode(p1); nsob++;
		}
	} else if (p->combop==T(INC,LONG)) {
		if (p1->op==LABEL && p1->refc==1 && p1->forw->combop==T(CMP,LONG)
		  && (p2=p1->forw->forw)->combop==T(CBR,JLE)
		  && (p3=p2->ref->back)->combop==JBR && p3->ref==p1
		  && p3->forw->op==LABEL && p3->forw==p2->ref) {
			/* change	INC LAB: CMP	to	LAB: INC CMP */
			p->back->forw=p1; p1->back=p->back;
			p->forw=p1->forw; p1->forw->back=p;
			p->back=p1; p1->forw=p;
			p1=p->forw;
			/* adjust beginning value by 1 */
				p2=alloc(sizeof first); p2->combop=T(DEC,LONG);
				p2->pop=0;
				p2->forw=p3; p2->back=p3->back; p3->back->forw=p2;
				p3->back=p2; p2->code=p->code; p2->labno=0;
		}
		if (p1->combop==T(CMP,LONG) && (p2=p1->forw)->op==CBR) {
			register char *cp1,*cp2;
			splitrand(p1); if (!equstr(p->code,regs[RT1])) continue;
			if (iabs(p->seq - p2->ref->seq)>12) {/* outside byte displ range */
				if (p2->subop!=JLE) continue;
				p->combop=T(ACB,LONG);
				cp2=regs[RT1]; cp1=regs[RT2]; while (*cp2++= *cp1++); /* limit */
				cp2=regs[RT2]; cp1="$1"; while (*cp2++= *cp1++); /* increment */
				cp2=regs[RT3]; cp1=p->code; while (*cp2++= *cp1++); /* index */
				p->pop=0; newcode(p);
				p->labno = p2->labno; delnode(p2); delnode(p1); nsob++;
			} else if (p2->subop==JLE || p2->subop==JLT) {
				if (p2->subop==JLE) p->combop=AOBLEQ; else p->combop=AOBLSS;
				cp2=regs[RT1]; cp1=regs[RT2]; while (*cp2++= *cp1++); /* limit */
				cp2=regs[RT2]; cp1=p->code; while (*cp2++= *cp1++); /* index */
				p->pop=0; newcode(p);
				p->labno = p2->labno; delnode(p2); delnode(p1); nsob++;
			}
		}
	}
	}
}

int iabs(int x)
{
	return(x<0? -x: x);
}

int equop(struct node *p1, struct node *p2)
{
	register char *cp1, *cp2;

	if (p1->combop != p2->combop)
		return(0);
	if (p1->op>0 && p1->op<MOV)
		return(0);
	if (p1->op==MOVA && p1->labno!=p2->labno) return(0);
	cp1 = p1->code;
	cp2 = p2->code;
	if (cp1==0 && cp2==0)
		return(1);
	if (cp1==0 || cp2==0)
		return(0);
	while (*cp1 == *cp2++)
		if (*cp1++ == 0)
			return(1);
	return(0);
}

int delnode(struct node *p){
	p->back->forw = p->forw;
	p->forw->back = p->back;
}

int decref(struct node *p)
{
	if (p && --p->refc <= 0) {
		nrlab++;
		delnode(p);
	}
}

struct node * nonlab(struct node *ap)
{
	register struct node *p;

	p = ap;
	while (p && p->op==LABEL)
		p = p->forw;
	return(p);
}

/*
 * Which of r0-r5 are live at a label or a branch?  The original ccom kept its
 * temporaries within a statement, so none were; pcc's register allocation
 * (-xtemps) carries loop counters and the like around loops.  So: dataflow over
 * the labels.  Registers are masks of bits 0-5; anything not understood reads
 * them all, and only a write that is certain to replace the whole register
 * (see simplew) kills one.
 */
#define	ALLREGS	((1<<NUSE)-1)

/*
 * Does any function in the file return a double or long long, in r0 and r1?
 * ccom, which knows, sets this to 0 and then to 1 as it meets such a function;
 * run alone, c2 has to assume it.  When it is 0, ret leaves only r0 live.
 */
int c2_quadret = 1;
static int *labin;	/* live registers at the label with this number */
static int nlabin;

static int
simplew(struct node *p)	/* does p write the whole of its last operand, without reading it? */
{
	int t = p->subop & 0xF;

	switch (p->op) {
	case MOV: case CLR: case COM: case NEG:
		return p->subop == LONG;
	case MOVA:
		return 1;
	case ADD: case SUB: case MUL: case DIV: case BIC: case BIS: case XOR:
		return (p->subop >> 4) == OP3 && t == LONG;
	}
	return 0;
}

static int
simpleop(struct node *p)	/* operands are single registers: no pairs */
{
	int t = p->subop & 0xF;

	if (t == DOUBLE || t == QUAD)
		return 0;
	switch (p->op) {
	case MOV: case CLR: case INC: case DEC: case TST: case PUSH: case CMP:
	case ADD: case SUB: case MUL: case DIV: case BIC: case BIS: case XOR:
	case BIT: case COM: case NEG: case MOVA: case PUSHA: case CBR:
		return 1;
	}
	return 0;
}

/* the registers p reads and the ones it certainly writes */
static void
insrw(struct node *p, int *rd, int *wr)
{
	char *c, *last, *end;
	int r;

	*rd = *wr = 0;
	if (p->op == 0) {
		*rd = ALLREGS;
		return;
	}
	if (p->op == JBR && p->subop == RET) {
		*rd = 3;
		return;
	}
	if ((c = p->code) == 0)
		return;
	last = 0;
	if (simplew(p)) {
		last = c + strlen(c);
		while (last > c && last[-1] != ',') last--;
	}
	for (; *c; c++) {
		if (*c != 'r' || !isdigit((unsigned char)c[1]) ||
		    (c > p->code && (isalnum((unsigned char)c[-1]) || c[-1] == '_')))
			continue;
		r = c[1] - '0';
		end = c + 2;
		if (isdigit((unsigned char)*end)) {	/* r10, r11: not ours */
			c = end;
			continue;
		}
		if (isalnum((unsigned char)*end) || *end == '_')
			continue;
		if (last && c == last && c[2] == 0)
			continue;		/* the register written */
		if (r < NUSE) *rd |= 1 << r;
		if (!simpleop(p) && r + 1 < NUSE) *rd |= 1 << (r + 1);
	}
	if (last && last[0] == 'r' && isdigit((unsigned char)last[1]) && last[2] == 0
	    && last[1] - '0' < NUSE)
		*wr |= 1 << (last[1] - '0');
	if (p->op == CALLS)
		*wr |= 3;
}

static int
isbranch(struct node *p)
{
	switch (p->op) {
	case JBR: case CBR: case JMP: case JSW: case CASE:
	case SOBGEQ: case SOBGTR: case AOBLEQ: case AOBLSS: case ACB:
		return 1;
	}
	return 0;
}

static int
isdatanode(struct node *p)
{
	switch (p->op) {
	case TEXT: case DATA: case BSS: case ALIGN: case WGEN: case LGEN:
	case SET: case END: case EROU:
		return 1;
	}
	return 0;
}

/* the registers live where branch p goes */
static int
liveto(struct node *p)
{
	int n;

	if (p->op == JBR && p->subop == RET)
		return c2_quadret ? 3 : 1;	/* r1 only if some function returns a quad */
	if (p->op == CASE || p->ref == 0 || p->ref->op != LABEL)
		return ALLREGS;
	n = p->ref->labno;
	return n > 0 && n < nlabin ? labin[n] : ALLREGS;
}

static void
findlive(void)
{
	struct node *p, *q, **bs = 0;
	int nb = 0, cap = 0, b, changed, live, rd, wr, mx = 0, *bin;

	for (p = first.forw; p != 0; p = p->forw) {
		if (p->op == LABEL && p->labno > mx) mx = p->labno;
		if (p->op == LABEL || p->op == DLABEL) {
			if (nb == cap)
				bs = realloc(bs, (cap = cap ? 2 * cap : 256) * sizeof *bs);
			bs[nb++] = p;
		}
	}
	free(labin);
	nlabin = mx + 1;
	labin = calloc(nlabin, sizeof *labin);
	bin = calloc(nb + 1, sizeof *bin);
	do {
		changed = 0;
		for (b = nb; --b >= 0;) {
			/* the nodes of block b, backwards */
			struct node *end = b + 1 < nb ? bs[b + 1] : 0;
			struct node *stack[1];
			(void)stack;
			/* walk forward to find the last node, then back via ->back */
			for (q = bs[b]; q->forw != end && q->forw != 0; q = q->forw);
			live = b + 1 < nb ? bin[b + 1] : 0;
			for (; q != bs[b]; q = q->back) {
				if (isdatanode(q))
					continue;
				if (q->op == JBR) {
					live = liveto(q);
					continue;
				}
				if (q->op == CASE)
					live = ALLREGS;
				else if (isbranch(q))
					live |= liveto(q);
				insrw(q, &rd, &wr);
				live = (live & ~wr) | rd;
			}
			if (bin[b] != live) {
				bin[b] = live;
				changed = 1;
				if (bs[b]->op == LABEL && bs[b]->labno > 0)
					labin[bs[b]->labno] = live;
			}
		}
	} while (changed);
	free(bin);
	free(bs);
}

/* at a branch (islab == 0) or a label (islab != 0): mark what is live there */
static void liveall(struct node *p, int islab){
	register int r, m;
	if (islab)
		m = p->op == LABEL && p->labno > 0 && p->labno < nlabin ? labin[p->labno] : ALLREGS;
	else
		m = liveto(p);
	for (r=NUSE; --r>=0;) {
		if (m & (1 << r)) {uses[r]=p; regs[r][0]= -1;}
		else if (islab) uses[r]=0;
	}
}

int clearuse(void){
	register struct node **i;
	for (i=uses+NUSE; i>uses;) *--i=0;
}

int clearreg(void){
	register char **i;
	for (i=regs+NREG; i>regs;) (void)memset(*--i, 0, 2);
	conloc[0] = 0; ccloc[0] = 0;
}

void savereg(int ai, char *s, int type)
{
	register char *p, *sp;

	sp = p = regs[ai];
	if (source(s)) /* side effects in addressing */
		return;
	/* if any indexing, must be parameter or local */
	/* indirection (as in "*-4(fp)") is ok, however */
	*p++ = type;
	while (*p++ = *s)
		if (*s=='[' || *s++=='(' && *s!='a' && *s!='f') {*sp = 0; return;}
}

int dest(char *s, int type)
{
	register int i;

	source(s); /* handle addressing side effects */
	if ((i = isreg(s)) >= 0) {
		(void)memset(regs[i],0,2); /* if register destination, that reg is a goner */
		if (DOUBLE==(type&0xF) || DOUBLE==((type>>4)&0xF))
			(void)memset(regs[i+1],0,2); /* clobber two at once */
	}
	for (i=NREG; --i>=0;)
		if (regs[i][1]=='*' && equstr(s, regs[i]+2))
			(void)memset(regs[i],0,2); /* previous indirection through destination is invalid */
	while ((i = findrand(s,0)) >= 0) /* previous values of destination are invalid */
		(void)memset(regs[i],0,2);
	if (!natural(s)) {/* wild store, everything except constants vanishes */
		for (i=NREG; --i>=0;) if (regs[i][1] != '$') (void)memset(regs[i],0,2);
		conloc[0] = 0; ccloc[0] = 0;
	} else setcc(s,type); /* natural destinations set condition codes */
}

int splitrand(struct node *p){
/* separate operands at commas, set up 'regs' and 'lastrand' */
register char *p1, *p2; register char **preg;
preg=regs+RT1;
if (p1=p->code) while (*p1) {
	lastrand=p2= *preg++;
	while (*p1) if (','==(*p2++= *p1++)) {--p2; break;}
	*p2=0;
}
while (preg<(regs+RT1+5)) *(*preg++)=0;
}

int compat(int have, int want){
register int hsrc, hdst;
if (0==(want &= 0xF)) return(1); /* anything satisfies a wildcard want */
hsrc=have&0xF; if (0==(hdst=((have>>4)&0xF)) || hdst>=OP2) hdst=hsrc;
/* last term prevents floats, doubles from satisfying a request for an int */
return(hsrc>=want && hdst>=want && !((want>=FLOAT) ^ (hdst>=FLOAT)));
}

int equtype(int t1, int t2){return(compat(t1,t2) && compat(t2,t1));}

int findrand(char *as, int type)
{
	register char **i;
	for (i = regs+NREG; --i>=regs;) {
		if (**i && equstr(*i+1, as) && compat(**i,type))
			return(i-regs);
	}
	return(-1);
}

int isreg(char *s)
{
	if (*s++!='r' || !isdigit(*s++)) return(-1);
	if (*s==0) return(*--s-'0');
	if (*(s-1)=='1' && isdigit(*s++) && *s==0) return(10+*--s-'0');
	return(-1);
}

int check(void)
{
	register struct node *p, *lp;

	lp = &first;
	for (p=first.forw; p!=0; p = p->forw) {
		if (p->back != lp)
			c2abort(-1);
		lp = p;
	}
}

int source(char *ap)
{
	register char *p1, *p2;

	p1 = ap;
	p2 = p1;
	if (*p1==0)
		return(0);
	while (*p2++ && *(p2-1)!='[');
	if (*p1=='-' && *(p1+1)=='('
	 || *p1=='*' && *(p1+1)=='-' && *(p1+2)=='('
	 || *(p2-2)=='+') {
		while (*p1 && *p1++!='r');
		if (isdigit(*p1++))
			if (isdigit(*p1)) (void)memset(regs[10+*p1-'0'],0,2);
			else (void)memset(regs[*--p1-'0'],0,2);
		return(1);
	}
	return(0);
}

int newcode(struct node *p){
	register char *p1,*p2,**preg;
	preg=regs+RT1; p2=line;
	while (*(p1= *preg++)) {while (*p2++= *p1++); *(p2-1)=',';}
	*--p2=0;
	p->code=copy(line);
}

int repladdr(struct node *p)
{
	register int r;
	register char *p1, *p2;
	char **preg; int nrepl;

	preg=regs+RT1; nrepl=0;
	/*
	 * addl3 $1,X,rN followed by movl rN,X is incl X once the move is folded
	 * in.  Put a register in place of X here and the two no longer name the
	 * same operand: leave this one alone.
	 */
	if ((p->op==ADD || p->op==SUB) && OP3==((p->subop>>4)&0xF) &&
	    equstr(regs[RT1],"$1") && p->forw && p->forw->op==MOV && p->forw->code) {
		char buf[100];

		snprintf(buf, sizeof buf, "%s,%s", regs[RT3], regs[RT2]);
		if (strcmp(p->forw->code, buf)==0)
			return 0;
	}
	while (lastrand!=(p1= *preg++))
		if (!source(p1) && 0<=(r=findrand(p1,p->subop))) {
			*p1++='r'; if (r>9) {*p1++='1'; r -= 10;} *p1++=r+'0'; *p1=0;
			nrepl++; nsaddr++;
		}
	if (nrepl) newcode(p);
}

/* movedat()
/* {
/* 	register struct node *p1, *p2;
/* 	struct node *p3;
/* 	register seg;
/* 	struct node data;
/* 	struct node *datp;
/* 
/* 	if (first.forw == 0)
/* 		return;
/* 	datp = &data;
/* 	for (p1 = first.forw; p1!=0; p1 = p1->forw) {
/* 		if (p1->op == DATA) {
/* 			p2 = p1->forw;
/* 			while (p2 && p2->op!=TEXT)
/* 				p2 = p2->forw;
/* 			if (p2==0)
/* 				break;
/* 			p3 = p1->back;
/* 			p1->back->forw = p2->forw;
/* 			p2->forw->back = p3;
/* 			p2->forw = 0;
/* 			datp->forw = p1;
/* 			p1->back = datp;
/* 			p1 = p3;
/* 			datp = p2;
/* 		}
/* 	}
/* 	if (data.forw) {
/* 		datp->forw = first.forw;
/* 		first.forw->back = datp;
/* 		data.forw->back = &first;
/* 		first.forw = data.forw;
/* 	}
/* 	seg = -1;
/* 	for (p1 = first.forw; p1!=0; p1 = p1->forw) {
/* 		if (p1->op==TEXT||p1->op==DATA||p1->op==BSS) {
/* 			if (p1->op == seg || p1->forw&&p1->forw->op==seg) {
/* 				p1->back->forw = p1->forw;
/* 				p1->forw->back = p1->back;
/* 				p1 = p1->back;
/* 				continue;
/* 			}
/* 			seg = p1->op;
/* 		}
/* 	}
/* }
*/

void redunbr(struct node *p)
{
	register struct node *p1;
	register char *ap1;
	char *ap2;

	if ((p1 = p->ref) == 0)
		return;
	p1 = nonlab(p1);
	if (p1->op==TST) {
		splitrand(p1);
		savereg(RT2, "$0", p1->subop);
	} else if (p1->op==CMP)
		splitrand(p1);
	else
		return;
	if (p1->forw->op==CBR) {
		ap1 = findcon(RT1, p1->subop);
		ap2 = findcon(RT2, p1->subop);
		p1 = p1->forw;
		if (compare(p1->subop, ap1, ap2)) {
			nredunj++;
			nchange++;
			decref(p->ref);
			p->ref = p1->ref;
			p->labno = p1->labno;
			p->ref->refc++;
		}
	} else if (p1->op==TST && equstr(regs[RT1],ccloc+1) &&
			equtype(ccloc[0],p1->subop)) {
		p1=insertl(p1->forw); decref(p->ref); p->ref=p1; 
		nrtst++; nchange++;
	}
}

char * findcon(int i, int type)
{
	register char *p;
	register int r;

	p = regs[i];
	if (*p=='$')
		return(p);
	if ((r = isreg(p)) >= 0 && compat(regs[r][0],type))
		return(regs[r]+1);
	if (equstr(p, conloc))
		return(conval+1);
	return(p);
}

int compare(int op, char *acp1, char *acp2)
{
	register char *cp1, *cp2;
	register int n1;
	int n2;	int sign;

	cp1 = acp1;
	cp2 = acp2;
	if (*cp1++ != '$' || *cp2++ != '$')
		return(0);
	n1 = 0; sign=1; if (*cp2=='-') {++cp2; sign= -1;}
	while (isdigit(*cp2)) {n1 *= 10; n1 += (*cp2++ - '0')*sign;}
	n2 = n1;
	n1 = 0; sign=1; if (*cp1=='-') {++cp1; sign= -1;}
	while (isdigit(*cp1)) {n1 *= 10; n1 += (*cp1++ - '0')*sign;}
	if (*cp1=='+')
		cp1++;
	if (*cp2=='+')
		cp2++;
	do {
		if (*cp1++ != *cp2)
			return(0);
	} while (*cp2++);
	switch(op) {

	case JEQ:
		return(n1 == n2);
	case JNE:
		return(n1 != n2);
	case JLE:
		return(n1 <= n2);
	case JGE:
		return(n1 >= n2);
	case JLT:
		return(n1 < n2);
	case JGT:
		return(n1 > n2);
	case JLO:
		return((unsigned)n1 < (unsigned)n2);
	case JHI:
		return((unsigned)n1 > (unsigned)n2);
	case JLOS:
		return((unsigned)n1 <= (unsigned)n2);
	case JHIS:
		return((unsigned)n1 >= (unsigned)n2);
	}
	return(0);
}

void setcon(char *cv, char *cl, int type)
{
	register char *p;

	if (*cv != '$')
		return;
	if (!natural(cl))
		return;
	p = conloc;
	while (*p++ = *cl++);
	p = conval;
	*p++ = type;
	while (*p++ = *cv++);
}

int equstr(char *p1, char *p2)
{
	do {
		if (*p1++ != *p2)
			return(0);
	} while (*p2++);
	return(1);
}

void setcc(char *ap, int type)
{
	register char *p, *p1;

	ccarith = 0;
	p = ap;
	if (!natural(p)) {
		ccloc[0] = 0;
		return;
	}
	p1 = ccloc;
	*p1++ = type;
	while (*p1++ = *p++);
}

int okio(char *p){/* 0->probable I/O space address; 1->not */
	if (ioflag && (!natural(p) || 0>getnum(p))) return(0);
	return(1);
}

int indexa(char *p){/* 1-> uses [r] addressing mode; 0->doesn't */
	while (*p) if (*p++=='[') return(1);
	return(0);
}

int natural(char *p)
{/* 1->simple local, parameter, global, or register; 0->otherwise */
	if (*p=='*' || *p=='(' || *p=='-'&&*(p+1)=='(' || *p=='$'&&getnum(p+1))
		return(0);
	while (*p++);
	p--;
	if (*--p=='+' || *p==']' || *p==')' && *(p-2)!='a' && *(p-2)!='f')
		return(0);
	return(1);
}

char c22[] = "@(#)c22.c 1.5 78/09/23 16:37:33";
#define readonly

readonly char revbr[] = {
	JNE, JEQ, JGT, JLT, JGE, JLE,
	JNE, JEQ, JHI, JLO, JHIS, JLOS,
	JBS, JBC, JLBS, JLBC, JBSC, JBCC, JBSS, JBCS };

/* cursed be the preprocessor, whose impotence and stupidity
/* prevented this table being macro-generated from ../as/instrs
*/

readonly struct optab optab[] = {
"jbr",JBR,
"jeql",T(CBR,JEQ),
"jneq",T(CBR,JNE),
"jleq",T(CBR,JLE),
"jgeq",T(CBR,JGE),
"jlss",T(CBR,JLT),
"jgtr",T(CBR,JGT),
"jbc",T(CBR,JBC),
"jbs",T(CBR,JBS),
"jlequ",T(CBR,JLOS),
"jgequ",T(CBR,JHIS),
"jlssu",T(CBR,JLO),
"jgtru",T(CBR,JHI),
"jlbc",T(CBR,JLBC),
"jlbs",T(CBR,JLBS),
"jbcc",T(CBR,JBCC),
"jbsc",T(CBR,JBSC),
"jbcs",T(CBR,JBCS),
"jbss",T(CBR,JBSS),
"acbb",T(ACB,BYTE),
"acbd",T(ACB,DOUBLE),
"acbf",T(ACB,FLOAT),
"acbl",T(ACB,LONG),
"acbw",T(ACB,WORD),
"addb2",T(ADD,U(BYTE,OP2)),
"addb3",T(ADD,U(BYTE,OP3)),
"addd2",T(ADD,U(DOUBLE,OP2)),
"addd3",T(ADD,U(DOUBLE,OP3)),
"addf2",T(ADD,U(FLOAT,OP2)),
"addf3",T(ADD,U(FLOAT,OP3)),
"addl2",T(ADD,U(LONG,OP2)),
"addl3",T(ADD,U(LONG,OP3)),
"addw2",T(ADD,U(WORD,OP2)),
"addw3",T(ADD,U(WORD,OP3)),
"aobleq",AOBLEQ,
"aoblss",AOBLSS,
"ashl",T(ASH,LONG),
"ashq",T(ASH,QUAD),
"bbc",T(CBR,JBC),
"bbcc",T(CBR,JBCC),
"bbcci",T(CBR,JBCC),
"bbcs",T(CBR,JBCS),
"bbs",T(CBR,JBS),
"bbsc",T(CBR,JBSC),
"bbss",T(CBR,JBSS),
"bbssi",T(CBR,JBSS),
"bcc",T(CBR,JHIS),
"bcs",T(CBR,JLO),
"beql",T(CBR,JEQ),
"beqlu",T(CBR,JEQ),
"bgeq",T(CBR,JGE),
"bgequ",T(CBR,JHIS),
"bgtr",T(CBR,JGT),
"bgtru",T(CBR,JHI),
"bicb2",T(BIC,U(BYTE,OP2)),
"bicb3",T(BIC,U(BYTE,OP3)),
"bicl2",T(BIC,U(LONG,OP2)),
"bicl3",T(BIC,U(LONG,OP3)),
"bicw2",T(BIC,U(WORD,OP2)),
"bicw3",T(BIC,U(WORD,OP3)),
"bisb2",T(BIS,U(BYTE,OP2)),
"bisb3",T(BIS,U(BYTE,OP3)),
"bisl2",T(BIS,U(LONG,OP2)),
"bisl3",T(BIS,U(LONG,OP3)),
"bisw2",T(BIS,U(WORD,OP2)),
"bisw3",T(BIS,U(WORD,OP3)),
"bitb",T(BIT,BYTE),
"bitl",T(BIT,LONG),
"bitw",T(BIT,WORD),
"blbs",T(CBR,JLBS),
"blbc",T(CBR,JLBC),
"bleq",T(CBR,JLE),
"blequ",T(CBR,JLOS),
"blss",T(CBR,JLT),
"blssu",T(CBR,JLO),
"bneq",T(CBR,JNE),
"bnequ",T(CBR,JNE),
"brb",JBR,
"brw",JBR,
"bvc",T(CBR,0),
"bvs",T(CBR,0),
"callg",CALLS,
"calls",CALLS,
"caseb",T(CASE,BYTE),
"casel",T(CASE,LONG),
"casew",T(CASE,WORD),
"clrb",T(CLR,BYTE),
"clrd",T(CLR,DOUBLE),
"clrf",T(CLR,FLOAT),
"clrl",T(CLR,LONG),
"clrq",T(CLR,QUAD),
"clrw",T(CLR,WORD),
"cmpb",T(CMP,BYTE),
"cmpd",T(CMP,DOUBLE),
"cmpf",T(CMP,FLOAT),
"cmpl",T(CMP,LONG),
"cmpw",T(CMP,WORD),
"cvtbd",T(CVT,U(BYTE,DOUBLE)),
"cvtbf",T(CVT,U(BYTE,FLOAT)),
"cvtbl",T(CVT,U(BYTE,LONG)),
"cvtbw",T(CVT,U(BYTE,WORD)),
"cvtdb",T(CVT,U(DOUBLE,BYTE)),
"cvtdf",T(CVT,U(DOUBLE,FLOAT)),
"cvtdl",T(CVT,U(DOUBLE,LONG)),
"cvtdw",T(CVT,U(DOUBLE,WORD)),
"cvtfb",T(CVT,U(FLOAT,BYTE)),
"cvtfd",T(CVT,U(FLOAT,DOUBLE)),
"cvtfl",T(CVT,U(FLOAT,LONG)),
"cvtfw",T(CVT,U(FLOAT,WORD)),
"cvtlb",T(CVT,U(LONG,BYTE)),
"cvtld",T(CVT,U(LONG,DOUBLE)),
"cvtlf",T(CVT,U(LONG,FLOAT)),
"cvtlw",T(CVT,U(LONG,WORD)),
"cvtrdl",T(CVT,U(DOUBLE,LONG)),
"cvtrfl",T(CVT,U(FLOAT,LONG)),
"cvtwb",T(CVT,U(WORD,BYTE)),
"cvtwd",T(CVT,U(WORD,DOUBLE)),
"cvtwf",T(CVT,U(WORD,FLOAT)),
"cvtwl",T(CVT,U(WORD,LONG)),
"decb",T(DEC,BYTE),
"decl",T(DEC,LONG),
"decw",T(DEC,WORD),
"divb2",T(DIV,U(BYTE,OP2)),
"divb3",T(DIV,U(BYTE,OP3)),
"divd2",T(DIV,U(DOUBLE,OP2)),
"divd3",T(DIV,U(DOUBLE,OP3)),
"divf2",T(DIV,U(FLOAT,OP2)),
"divf3",T(DIV,U(FLOAT,OP3)),
"divl2",T(DIV,U(LONG,OP2)),
"divl3",T(DIV,U(LONG,OP3)),
"divw2",T(DIV,U(WORD,OP2)),
"divw3",T(DIV,U(WORD,OP3)),
"extv",T(EXTV,LONG),
"extzv",T(EXTZV,LONG),
"incb",T(INC,BYTE),
"incl",T(INC,LONG),
"incw",T(INC,WORD),
"insv",T(INSV,-1),
"jmp",JMP,
"mcomb",T(COM,BYTE),
"mcoml",T(COM,LONG),
"mcomw",T(COM,WORD),
"mnegb",T(NEG,BYTE),
"mnegd",T(NEG,DOUBLE),
"mnegf",T(NEG,FLOAT),
"mnegl",T(NEG,LONG),
"mnegw",T(NEG,WORD),
"movab",T(MOVA,BYTE),
"movad",T(MOVA,DOUBLE),
"movaf",T(MOVA,FLOAT),
"moval",T(MOVA,LONG),
"movaq",T(MOVA,QUAD),
"movaw",T(MOVA,WORD),
"movb",T(MOV,BYTE),
"movd",T(MOV,DOUBLE),
"movf",T(MOV,FLOAT),
"movl",T(MOV,LONG),
"movq",T(MOV,QUAD),
"movw",T(MOV,WORD),
"movzbl",T(MOVZ,U(BYTE,LONG)),
"movzbw",T(MOVZ,U(BYTE,WORD)),
"movzwl",T(MOVZ,U(WORD,LONG)),
"mulb2",T(MUL,U(BYTE,OP2)),
"mulb3",T(MUL,U(BYTE,OP3)),
"muld2",T(MUL,U(DOUBLE,OP2)),
"muld3",T(MUL,U(DOUBLE,OP3)),
"mulf2",T(MUL,U(FLOAT,OP2)),
"mulf3",T(MUL,U(FLOAT,OP3)),
"mull2",T(MUL,U(LONG,OP2)),
"mull3",T(MUL,U(LONG,OP3)),
"mulw2",T(MUL,U(WORD,OP2)),
"mulw3",T(MUL,U(WORD,OP3)),
"pushab",T(PUSHA,BYTE),
"pushad",T(PUSHA,DOUBLE),
"pushaf",T(PUSHA,FLOAT),
"pushal",T(PUSHA,LONG),
"pushaq",T(PUSHA,QUAD),
"pushaw",T(PUSHA,WORD),
"pushl",T(PUSH,LONG),
"ret",T(JBR,RET),
"sobgeq",SOBGEQ,
"sobgtr",SOBGTR,
"subb2",T(SUB,U(BYTE,OP2)),
"subb3",T(SUB,U(BYTE,OP3)),
"subd2",T(SUB,U(DOUBLE,OP2)),
"subd3",T(SUB,U(DOUBLE,OP3)),
"subf2",T(SUB,U(FLOAT,OP2)),
"subf3",T(SUB,U(FLOAT,OP3)),
"subl2",T(SUB,U(LONG,OP2)),
"subl3",T(SUB,U(LONG,OP3)),
"subw2",T(SUB,U(WORD,OP2)),
"subw3",T(SUB,U(WORD,OP3)),
"tstb",T(TST,BYTE),
"tstd",T(TST,DOUBLE),
"tstf",T(TST,FLOAT),
"tstl",T(TST,LONG),
"tstw",T(TST,WORD),
"xorb2",T(XOR,U(BYTE,OP2)),
"xorb3",T(XOR,U(BYTE,OP3)),
"xorl2",T(XOR,U(LONG,OP2)),
"xorl3",T(XOR,U(LONG,OP3)),
"xorw2",T(XOR,U(WORD,OP2)),
"xorw3",T(XOR,U(WORD,OP3)),
".globl",EROU,
".text",TEXT,
".data",DATA,
".bss",BSS,
".align",ALIGN,
".word",WGEN,
".long",LGEN,
".set",SET,
".end",END,
"adawi",0,
"addp4",0,
"addp6",0,
"adwc",0,
"ashp",0,
"bicpsw",0,
"bispsw",0,
"bpt",0,
"bsbb",0,
"bsbw",0,
"chme",0,
"chmk",0,
"chms",0,
"chmu",0,
"cmpc3",0,
"cmpc5",0,
"cmpp3",0,
"cmpp4",0,
"cmpv",0,
"cmpzv",0,
"crc",0,
"cvtlp",0,
"cvtpl",0,
"cvttp",0,
"cvtpt",0,
"cvtps",0,
"cvtsp",0,
"divp",0,
"editpc",0,
"ediv",0,
"emodd",0,
"emodf",0,
"emul",0,
"ffc",0,
"ffs",0,
"halt",0,
"index",0,
"insque",0,
"jsb",0,
"ldpctx",0,
"locc",0,
"matchc",0,
"mfpr",0,
"movc3",0,
"movc5",0,
"movp",0,
"movpsl",0,
"movtc",0,
"movtuc",0,
"mtpr",0,
"mulp",0,
"nop",0,
"polyd",0,
"polyf",0,
"popr",0,
"prober",0,
"probew",0,
"pushr",0,
"rei",0,
"remque",0,
"rotl",0,
"rsb",0,
"sbwc",0,
"scanc",0,
"skpc",0,
"spanc",0,
"subp4",0,
"subp6",0,
"svpctx",0,
"xfc",0,
"escd",0,
"esce",0,
"escf",0,
	0,	0};
