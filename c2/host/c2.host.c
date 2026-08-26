/*
 * cc2.pdp11.c -- Ritchie's PDP-11 peephole optimizer, modernized to C99.
 * Derived from V7 cmd/c/c20.c + c21.c + c2.h via knr2c99.py.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

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
#define	COM	12
#define	INC	13
#define	DEC	14
#define	NEG	15
#define	TST	16
#define	ASR	17
#define	ASL	18
#define	SXT	19
#define	CMP	20
#define	ADD	21
#define	SUB	22
#define	BIT	23
#define	BIC	24
#define	BIS	25
#define	MUL	26
#define	DIV	27
#define	ASH	28
#define	XOR	29
#define	TEXT	30
#define	DATA	31
#define	BSS	32
#define	EVEN	33
#define	MOVF	34
#define	MOVOF	35
#define	MOVFO	36
#define	ADDF	37
#define	SUBF	38
#define	DIVF	39
#define	MULF	40
#define	CLRF	41
#define	CMPF	42
#define	NEGF	43
#define	TSTF	44
#define	CFCC	45
#define	SOB	46
#define	JSR	47
#define	END	48

#define	JEQ	0
#define	JNE	1
#define	JLE	2
#define	JGE	3
#define	JLT	4
#define	JGT	5
#define	JLO	6
#define	JHI	7
#define	JLOS	8
#define	JHIS	9

#define	BYTE	100
#define	LSIZE	512

struct node {
	char	op;
	char	subop;
	struct	node	*forw;
	struct	node	*back;
	struct	node	*ref;
	int16_t	labno;
	char	*code;
	int16_t	refc;
};

struct optab {
	char	*opstring;
	int16_t	opcode;
} optab[];

char	line[LSIZE];
struct	node	first;
char	*curlp;
int16_t	nbrbr;
int16_t	nsaddr;
int16_t	redunm;
int16_t	iaftbr;
int16_t	njp1;
int16_t	nrlab;
int16_t	nxjump;
int16_t	ncmot;
int16_t	nrevbr;
int16_t	loopiv;
int16_t	nredunj;
int16_t	nskip;
int16_t	ncomj;
int16_t	nsob;
int16_t	nrtst;
int16_t	nlit;

int16_t	nchange;
int16_t	isn;
int16_t	debug;
int16_t	lastseg;
char	revbr[];
char	regs[12][20];
char	conloc[20];
char	conval[20];
char	ccloc[20];

#define	RT1	10
#define	RT2	11
#define	FREG	5
#define	NREG	5
#define	LABHS	127
#define	OPHS	57

struct optab *ophash[OPHS];

int16_t input(void);
int16_t c2_getline(void);
int16_t getnum(char *ap);
void output(void);
void reducelit(struct node *at);
char *copy(int16_t na, char *ap, ...);
int16_t opsetup(void);
int16_t oplook(void);
int16_t refcount(void);
int16_t iterate(void);
void xjump(struct node *p1);
struct node * insertl(struct node *oldp);
struct node * codemove(struct node *p);
int16_t comjump(void);
void backjmp(struct node *ap1, struct node *ap2);
int16_t rmove(void);
int16_t jumpsw(void);
int16_t addsob(void);
int16_t toofar(struct node *p);
int16_t ilen(struct node *p);
int16_t adrlen(char *s);
int16_t c2_abs(int16_t x);
int16_t equop(struct node *ap1, struct node *p2);
int16_t decref(struct node *p);
struct node * nonlab(struct node *p);
char * alloc(int16_t n);
int16_t clearreg(void);
void savereg(int16_t ai, char *as);
void dest(char *as, int16_t flt);
int16_t singop(struct node *ap);
void dualop(struct node *ap);
int16_t findrand(char *as, int16_t flt);
int16_t isreg(char *as);
int16_t check(void);
int16_t source(char *ap);
int16_t repladdr(struct node *p, int16_t f, int16_t flt);
void movedat(void);
void redunbr(struct node *p);
char * findcon(int16_t i);
int16_t compare(int16_t oper, char *cp1, char *cp2);
void setcon(char *ar1, char *ar2);
int16_t equstr(char *ap1, char *ap2);
void setcc(char *ap);
int16_t natural(char *ap);

/*
 *	 C object code improver
 */


struct optab optab[] = {
	"jbr",	JBR,
	"jeq",	CBR | JEQ<<8,
	"jne",	CBR | JNE<<8,
	"jle",	CBR | JLE<<8,
	"jge",	CBR | JGE<<8,
	"jlt",	CBR | JLT<<8,
	"jgt",	CBR | JGT<<8,
	"jlo",	CBR | JLO<<8,
	"jhi",	CBR | JHI<<8,
	"jlos",	CBR | JLOS<<8,
	"jhis",	CBR | JHIS<<8,
	"jmp",	JMP,
	".globl",EROU,
	"mov",	MOV,
	"clr",	CLR,
	"com",	COM,
	"inc",	INC,
	"dec",	DEC,
	"neg",	NEG,
	"tst",	TST,
	"asr",	ASR,
	"asl",	ASL,
	"sxt",	SXT,
	"cmp",	CMP,
	"add",	ADD,
	"sub",	SUB,
	"bit",	BIT,
	"bic",	BIC,
	"bis",	BIS,
	"mul",	MUL,
	"ash",	ASH,
	"xor",	XOR,
	".text",TEXT,
	".data",DATA,
	".bss",	BSS,
	".even",EVEN,
	"movf",	MOVF,
	"movof",MOVOF,
	"movfo",MOVFO,
	"addf",	ADDF,
	"subf",	SUBF,
	"divf",	DIVF,
	"mulf",	MULF,
	"clrf",	CLRF,
	"cmpf",	CMPF,
	"negf",	NEGF,
	"tstf",	TSTF,
	"cfcc",	CFCC,
	"sob",	SOB,
	"jsr",	JSR,
	".end",	END,
	0,	0};

char	revbr[] = { JNE, JEQ, JGT, JLT, JGE, JLE, JHIS, JLOS, JHI, JLO };
int16_t	isn	= 20000;
int16_t	lastseg	= -1;


int16_t main(int16_t argc, char **argv)
{
	register int16_t niter, maxiter, isend;
	extern int16_t end;
	int16_t nflag;

	if (argc>1 && argv[1][0]=='+') {
		argc--;
		argv++;
		debug++;
	}
	nflag = 0;
	if (argc>1 && argv[1][0]=='-') {
		argc--;
		argv++;
		nflag++;
	}
	if (argc>1) {
		if (freopen(argv[1], "r", stdin) == NULL) {
			fprintf(stderr, "C2: can't find %s\n", argv[1]);
			exit(1);
		}
	}
	if (argc>2) {
		if (freopen(argv[2], "w", stdout) == NULL) {
			fprintf(stderr, "C2: can't create %s\n", argv[2]);
			exit(1);
		}
	}
	maxiter = 0;
	opsetup();
	do {
		isend = input();
		movedat();
		niter = 0;
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
	} while (isend);
	if (nflag) {
		fprintf(stderr, "%d iterations\n", maxiter);
		fprintf(stderr, "%d jumps to jumps\n", nbrbr);
		fprintf(stderr, "%d inst. after jumps\n", iaftbr);
		fprintf(stderr, "%d jumps to .+2\n", njp1);
		fprintf(stderr, "%d redundant labels\n", nrlab);
		fprintf(stderr, "%d cross-jumps\n", nxjump);
		fprintf(stderr, "%d code motions\n", ncmot);
		fprintf(stderr, "%d branches reversed\n", nrevbr);
		fprintf(stderr, "%d redundant moves\n", redunm);
		fprintf(stderr, "%d simplified addresses\n", nsaddr);
		fprintf(stderr, "%d loops inverted\n", loopiv);
		fprintf(stderr, "%d redundant jumps\n", nredunj);
		fprintf(stderr, "%d common seqs before jmp's\n", ncomj);
		fprintf(stderr, "%d skips over jumps\n", nskip);
		fprintf(stderr, "%d sob's added\n", nsob);
		fprintf(stderr, "%d redundant tst's\n", nrtst);
		fprintf(stderr, "%d literals eliminated\n", nlit);
	}
	exit(0);
}

int16_t input(void)
{
	register struct node *p, *lastp;
	register int16_t oper;

	lastp = &first;
	for (;;) {
		oper = c2_getline();
		switch (oper&0377) {
	
		case LABEL:
			p = (struct node *)alloc(sizeof first);
			if (line[0] == 'L') {
				p->op = LABEL;
				p->subop = 0;
				p->labno = getnum(line+1);
				p->code = 0;
			} else {
				p->op = DLABEL;
				p->subop = 0;
				p->labno = 0;
				p->code = copy(1, line);
			}
			break;
	
		case JBR:
		case CBR:
		case JMP:
		case JSW:
			p = (struct node *)alloc(sizeof first);
			p->op = oper&0377;
			p->subop = oper>>8;
			if (*curlp=='L' && (p->labno = getnum(curlp+1)))
				p->code = 0;
			else {
				p->labno = 0;
				p->code = copy(1, curlp);
			}
			break;

		default:
			p = (struct node *)alloc(sizeof first);
			p->op = oper&0377;
			p->subop = oper>>8;
			p->labno = 0;
			p->code = copy(1, curlp);
			break;

		}
		p->forw = 0;
		p->back = lastp;
		lastp->forw = p;
		lastp = p;
		p->ref = 0;
		if (oper==EROU)
			return(1);
		if (oper==END)
			return(0);
	}
}

int16_t c2_getline(void)
{
	register char *lp;
	register int16_t c;

	lp = line;
	while ((c = getchar())==' ' || c=='\t')
		;
	do {
		if (c==':') {
			*lp++ = 0;
			return(LABEL);
		}
		if (c=='\n') {
			*lp++ = 0;
			return(oplook());
		}
		if (lp >= &line[LSIZE-2]) {
			fprintf(stderr, "C2: Sorry, input line too long\n");
			exit(1);
		}
		*lp++ = c;
	} while ((c = getchar()) != EOF);
	*lp++ = 0;
	return(END);
}

int16_t getnum(char *ap)
{
	register char *p;
	register int16_t n, c;

	p = ap;
	n = 0;
	while ((c = *p++) >= '0' && c <= '9')
		n = n*10 + c - '0';
	if (*--p != 0)
		return(0);
	return(n);
}

void output(void)
{
	register struct node *t;
	register struct optab *oper;
	register int16_t byte;

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

	case TEXT:
	case DATA:
	case BSS:
		lastseg = t->op;

	default:
		if ((byte = t->subop) == BYTE)
			t->subop = 0;
		for (oper = optab; oper->opstring!=0; oper++) 
			if ((oper->opcode&0377) == t->op
			 && (oper->opcode>>8) == t->subop) {
				printf("%s", oper->opstring);
				if (byte==BYTE)
					printf("b");
				break;
			}
		if (t->code) {
			reducelit(t);
			printf("\t%s\n", t->code);
		} else if (t->op==JBR || t->op==CBR)
			printf("\tL%d\n", t->labno);
		else
			printf("\n");
		continue;

	case JSW:
		printf("L%d\n", t->labno);
		continue;

	case SOB:
		printf("sob	%s", t->code);
		if (t->labno)
			printf(",L%d", t->labno);
		printf("\n");
		continue;

	case 0:
		if (t->code)
			printf("%s", t->code);
		printf("\n");
		continue;
	}
}

/*
 * Notice addresses of the form
 * $xx,xx(r)
 * and replace them with (pc),xx(r)
 *     -- Thanx and a tip of the Hatlo hat to Bliss-11.
 */
void reducelit(struct node *at)
{
	register char *c1, *c2;
	char *c2s;
	register struct node *t;

	t = at;
	if (*t->code != '$')
		return;
	c1 = t->code;
	while (*c1 != ',')
		if (*c1++ == '\0')
			return;
	c2s = c1;
	c1++;
	if (*c1=='*')
		c1++;
	c2 = t->code+1;
	while (*c1++ == *c2++);
	if (*--c1!='(' || *--c2!=',')
		return;
	t->code = copy(2, "(pc)", c2s);
	nlit++;
}

char * copy(int16_t na, char *ap, ...)
{
	va_list apv;
	register char *p, *np;
	char *onp;
	register int16_t n, i;

	p = ap;
	n = 0;
	if (*p==0)
		return(0);
	do
		n++;
	while (*p++);
	va_start(apv, ap);
	for (i=1; i<na; i++) {
		p = va_arg(apv, char *);
		while (*p++)
			n++;
	}
	va_end(apv);
	onp = np = alloc(n);
	p = ap;
	while (*np++ = *p++)
		;
	va_start(apv, ap);
	for (i=1; i<na; i++) {
		p = va_arg(apv, char *);
		np--;
		while (*np++ = *p++);
	}
	va_end(apv);
	return(onp);
}

int16_t opsetup(void)
{
	register struct optab *optp, **ophp;
	register char *p;

	for (optp = optab; p = optp->opstring; optp++) {
		ophp = &ophash[(((p[0]<<3)+(p[1]<<1)+p[2])&077777) % OPHS];
		while (*ophp++)
			if (ophp > &ophash[OPHS])
				ophp = ophash;
		*--ophp = optp;
	}
}

int16_t oplook(void)
{
	register struct optab *optp;
	register char *lp, *np;
	static char tmpop[32];
	struct optab **ophp;

	if (line[0]=='\0') {
		curlp = line;
		return(0);
	}
	np = tmpop;
	for (lp = line; *lp && *lp!=' ' && *lp!='\t';)
		*np++ = *lp++;
	*np++ = 0;
	while (*lp=='\t' || *lp==' ')
		lp++;
	curlp = lp;
	ophp = &ophash[(((tmpop[0]<<3)+(tmpop[1]<<1)+tmpop[2])&077777) % OPHS];
	while (optp = *ophp) {
		np = optp->opstring;
		lp = tmpop;
		while (*lp == *np++)
			if (*lp++ == 0)
				return(optp->opcode);
		if (*lp++=='b' && *lp++==0 && *--np==0)
			return(optp->opcode + (BYTE<<8));
		ophp++;
		if (ophp >= &ophash[OPHS])
			ophp = ophash;
	}
	if (line[0]=='L') {
		lp = &line[1];
		while (*lp)
			if (*lp<'0' || *lp++>'9')
				return(0);
		curlp = line;
		return(JSW);
	}
	curlp = line;
	return(0);
}

int16_t refcount(void)
{
	register struct node *p, *lp;
	static struct node *labhash[LABHS];
	register struct node **hp, *tp;

	for (hp = labhash; hp < &labhash[LABHS];)
		*hp++ = 0;
	for (p = first.forw; p!=0; p = p->forw)
		if (p->op==LABEL) {
			labhash[p->labno % LABHS] = p;
			p->refc = 0;
		}
	for (p = first.forw; p!=0; p = p->forw) {
		if (p->op==JBR || p->op==CBR || p->op==JSW) {
			p->ref = 0;
			lp = labhash[p->labno % LABHS];
			if (lp==0 || p->labno!=lp->labno)
			for (lp = first.forw; lp!=0; lp = lp->forw) {
				if (lp->op==LABEL && p->labno==lp->labno)
					break;
			}
			if (lp) {
				tp = nonlab(lp)->back;
				if (tp!=lp) {
					p->labno = tp->labno;
					lp = tp;
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

int16_t iterate(void)
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
		if (p->op==CBR && (p1 = p->forw)->op==JBR) {
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
				nchange++;
				nskip++;
			}
		}
		if (p->op==JBR || p->op==JMP) {
			while (p->forw && p->forw->op!=LABEL
				&& p->forw->op!=DLABEL
				&& p->forw->op!=EROU && p->forw->op!=END
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
		}
		if (p->op==JBR || p->op==JMP) {
			xjump(p);
			p = codemove(p);
		}
	}
}

void xjump(struct node *p1)
{
	register struct node *p2, *p3;

	if ((p2 = p1->ref)==0)
		return;
	for (;;) {
		while ((p1 = p1->back) && p1->op==LABEL);
		while ((p2 = p2->back) && p2->op==LABEL);
		if (!equop(p1, p2) || p1==p2)
			return;
		p3 = insertl(p2);
		p1->op = JBR;
		p1->subop = 0;
		p1->ref = p3;
		p1->labno = p3->labno;
		p1->code = 0;
		nxjump++;
		nchange++;
	}
}

struct node * insertl(struct node *oldp)
{
	register struct node *lp;

	if (oldp->op == LABEL) {
		oldp->refc++;
		return(oldp);
	}
	if (oldp->back->op == LABEL) {
		oldp = oldp->back;
		oldp->refc++;
		return(oldp);
	}
	lp = (struct node *)alloc(sizeof first);
	lp->op = LABEL;
	lp->subop = 0;
	lp->labno = isn++;
	lp->ref = 0;
	lp->code = 0;
	lp->refc = 1;
	lp->back = oldp->back;
	lp->forw = oldp;
	oldp->back->forw = lp;
	oldp->back = lp;
	return(lp);
}

struct node * codemove(struct node *p)
{
	register struct node *p1, *p2, *p3;
	struct node *t, *tl;
	int16_t n;

	p1 = p;
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
			return(p);
	while (p1!=p3);
	p1 = p;
	tl = insertl(p1);
	p3->subop = revbr[p3->subop];
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

int16_t comjump(void)
{
	register struct node *p1, *p2, *p3;

	for (p1 = first.forw; p1!=0; p1 = p1->forw)
		if (p1->op==JBR && (p2 = p1->ref) && p2->refc > 1)
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
			p2->labno = p3->labno;
			p2->ref = p3;
			nchange++;
			ncomj++;
		} else
			return;
	}
}

/*
 * C object code improver-- second part
 */



int16_t rmove(void)
{
	register struct node *p;
	register int16_t r;
	register  int16_t r1, flt;

	for (p=first.forw; p!=0; p = p->forw) {
	flt = 0;
	switch (p->op) {

	case MOVF:
	case MOVFO:
	case MOVOF:
		flt = NREG;

	case MOV:
		if (p->subop==BYTE)
			goto dble;
		dualop(p);
		if ((r = findrand(regs[RT1], flt)) >= 0) {
			if (r == flt+isreg(regs[RT2]) && p->forw->op!=CBR
			   && p->forw->op!=SXT
			   && p->forw->op!=CFCC) {
				p->forw->back = p->back;
				p->back->forw = p->forw;
				redunm++;
				continue;
			}
		}
		if (equstr(regs[RT1], "$0")) {
			p->op = CLR;
			strcpy(regs[RT1], regs[RT2]);
			regs[RT2][0] = 0;
			p->code = copy(1, regs[RT1]);
			goto sngl;
		}
		repladdr(p, 0, flt);
		r = isreg(regs[RT1]);
		r1 = isreg(regs[RT2]);
		dest(regs[RT2], flt);
		if (r >= 0)
			if (r1 >= 0)
				savereg(r1+flt, regs[r+flt]);
			else
				savereg(r+flt, regs[RT2]);
		else
			if (r1 >= 0)
				savereg(r1+flt, regs[RT1]);
			else
				setcon(regs[RT1], regs[RT2]);
		source(regs[RT1]);
		setcc(regs[RT2]);
		continue;

	case ADDF:
	case SUBF:
	case DIVF:
	case MULF:
		flt = NREG;
		goto dble;

	case ADD:
	case SUB:
	case BIC:
	case BIS:
	case MUL:
	case DIV:
	case ASH:
	dble:
		dualop(p);
		if (p->op==BIC && (equstr(regs[RT1], "$-1") || equstr(regs[RT1], "$177777"))) {
			p->op = CLR;
			strcpy(regs[RT1], regs[RT2]);
			regs[RT2][0] = 0;
			p->code = copy(1, regs[RT1]);
			goto sngl;
		}
		if ((p->op==BIC || p->op==BIS) && equstr(regs[RT1], "$0")) {
			if (p->forw->op!=CBR) {
				p->back->forw = p->forw;
				p->forw->back = p->back;
				continue;
			}
		}
		repladdr(p, 0, flt);
		source(regs[RT1]);
		dest(regs[RT2], flt);
		if (p->op==DIV && (r = isreg(regs[RT2])>=0))
			regs[r+1][0] = 0;
		ccloc[0] = 0;
		continue;

	case CLRF:
	case NEGF:
		flt = NREG;

	case CLR:
	case COM:
	case INC:
	case DEC:
	case NEG:
	case ASR:
	case ASL:
	case SXT:
		singop(p);
	sngl:
		dest(regs[RT1], flt);
		if (p->op==CLR && flt==0)
			if ((r = isreg(regs[RT1])) >= 0)
				savereg(r, "$0");
			else
				setcon("$0", regs[RT1]);
		ccloc[0] = 0;
		continue;

	case TSTF:
		flt = NREG;

	case TST:
		singop(p);
		repladdr(p, 0, flt);
		source(regs[RT1]);
		if (equstr(regs[RT1], ccloc)) {
			p->back->forw = p->forw;
			p->forw->back = p->back;
			p = p->back;
			nrtst++;
			nchange++;
		}
		continue;

	case CMPF:
		flt = NREG;

	case CMP:
	case BIT:
		dualop(p);
		source(regs[RT1]);
		source(regs[RT2]);
		if(p->op==BIT) {
			if (equstr(regs[RT1], "$-1") || equstr(regs[RT1], "$177777")) {
				p->op = TST;
				strcpy(regs[RT1], regs[RT2]);
				regs[RT2][0] = 0;
				p->code = copy(1, regs[RT1]);
				nchange++;
				nsaddr++;
			} else if (equstr(regs[RT2], "$-1") || equstr(regs[RT2], "$177777")) {
				p->op = TST;
				regs[RT2][0] = 0;
				p->code = copy(1, regs[RT1]);
				nchange++;
				nsaddr++;
			}
			if (equstr(regs[RT1], "$0")) {
				p->op = TST;
				regs[RT2][0] = 0;
				p->code = copy(1, regs[RT1]);
				nchange++;
				nsaddr++;
			} else if (equstr(regs[RT2], "$0")) {
				p->op = TST;
				strcpy(regs[RT1], regs[RT2]);
				regs[RT2][0] = 0;
				p->code = copy(1, regs[RT1]);
				nchange++;
				nsaddr++;
			}
		}
		repladdr(p, 1, flt);
		ccloc[0] = 0;
		continue;

	case CBR:
		if (p->back->op==TST || p->back->op==CMP) {
			if (p->back->op==TST) {
				singop(p->back);
				savereg(RT2, "$0");
			} else
				dualop(p->back);
			r = compare(p->subop, findcon(RT1), findcon(RT2));
			if (r==0) {
				p->back->back->forw = p->forw;
				p->forw->back = p->back->back;
				decref(p->ref);
				p = p->back->back;
				nchange++;
			} else if (r>0) {
				p->op = JBR;
				p->subop = 0;
				p->back->back->forw = p;
				p->back = p->back->back;
				p = p->back;
				nchange++;
			}
		}
	case CFCC:
		ccloc[0] = 0;
		continue;

	case JBR:
		redunbr(p);

	default:
		clearreg();
	}
	}
}

int16_t jumpsw(void)
{
	register struct node *p, *p1;
	register int16_t t;
	register struct node *tp;
	int16_t nj;

	t = 0;
	nj = 0;
	for (p=first.forw; p!=0; p = p->forw)
		p->refc = ++t;
	for (p=first.forw; p!=0; p = p1) {
		p1 = p->forw;
		if (p->op == CBR && p1->op==JBR && p->ref && p1->ref
		 && c2_abs(p->refc - p->ref->refc) > c2_abs(p1->refc - p1->ref->refc)) {
			if (p->ref==p1->ref)
				continue;
			p->subop = revbr[p->subop];
			tp = p1->ref;
			p1->ref = p->ref;
			p->ref = tp;
			t = p1->labno;
			p1->labno = p->labno;
			p->labno = t;
			nrevbr++;
			nj++;
		}
	}
	return(nj);
}

int16_t addsob(void)
{
	register struct node *p, *p1;

	for (p = &first; (p1 = p->forw)!=0; p = p1) {
		if (p->op==DEC && isreg(p->code)>=0
		 && p1->op==CBR && p1->subop==JNE) {
			if (p->refc < p1->ref->refc)
				continue;
			if (toofar(p1))
				continue;
			p->labno = p1->labno;
			p->op = SOB;
			p->subop = 0;
			p1->forw->back = p;
			p->forw = p1->forw;
			nsob++;
		}
	}
}

int16_t toofar(struct node *p)
{
	register struct node *p1;
	int16_t len;

	len = 0;
	for (p1 = p->ref; p1 && p1!=p; p1 = p1->forw)
		len += ilen(p1);
	if (len < 128)
		return(0);
	return(1);
}

int16_t ilen(struct node *p)
{
	register int16_t l;

	switch (p->op) {
	case LABEL:
	case DLABEL:
	case TEXT:
	case EROU:
	case EVEN:
		return(0);

	case CBR:
		return(6);

	default:
		dualop(p);
		return(2 + adrlen(regs[RT1]) + adrlen(regs[RT2]));
	}
}

int16_t adrlen(char *s)
{
	if (*s == 0)
		return(0);
	if (*s=='r')
		return(0);
	if (*s=='(' && *(s+1)=='r')
		return(0);
	if (*s=='-' && *(s+1)=='(')
		return(0);
	return(2);
}

int16_t c2_abs(int16_t x)
{
	return(x<0? -x: x);
}

int16_t equop(struct node *ap1, struct node *p2)
{
	register char *cp1, *cp2;
	register struct node *p1;

	p1 = ap1;
	if (p1->op!=p2->op || p1->subop!=p2->subop)
		return(0);
	if (p1->op>0 && p1->op<MOV)
		return(0);
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

int16_t decref(struct node *p)
{
	if (--p->refc <= 0) {
		nrlab++;
		p->back->forw = p->forw;
		p->forw->back = p->back;
	}
}

struct node * nonlab(struct node *p)
{
	while (p && p->op==LABEL)
		p = p->forw;
	return(p);
}

char * alloc(int16_t n)
{
	n++;
	n &= ~01;
	return((char *)malloc(n));
}

int16_t clearreg(void)
{
	register int16_t i;

	for (i=0; i<2*NREG; i++)
		regs[i][0] = '\0';
	conloc[0] = 0;
	ccloc[0] = 0;
}

void savereg(int16_t ai, char *as)
{
	register char *p, *s, *sp;

	sp = p = regs[ai];
	s = as;
	if (source(s))
		return;
	while (*p++ = *s) {
		if (s[0]=='(' && s[1]=='r' && s[2]<'5') {
			*sp = 0;
			return;
		}
		if (*s++ == ',')
			break;
	}
	*--p = '\0';
}

void dest(char *as, int16_t flt)
{
	register char *s;
	register int16_t i;

	s = as;
	source(s);
	if ((i = isreg(s)) >= 0)
		regs[i+flt][0] = 0;
	for (i=0; i<NREG+NREG; i++)
		if (*regs[i]=='*' && equstr(s, regs[i]+1))
			regs[i][0] = 0;
	while ((i = findrand(s, flt)) >= 0)
		regs[i][0] = 0;
	while (*s) {
		if ((*s=='(' && (*(s+1)!='r' || *(s+2)!='5')) || *s++=='*') {
			for (i=0; i<NREG+NREG; i++) {
				if (regs[i][0] != '$')
					regs[i][0] = 0;
				conloc[0] = 0;
			}
			return;
		}
	}
}

int16_t singop(struct node *ap)
{
	register char *p1, *p2;

	p1 = ap->code;
	p2 = regs[RT1];
	while (*p2++ = *p1++);
	regs[RT2][0] = 0;
}


void dualop(struct node *ap)
{
	register char *p1, *p2;
	register struct node *p;

	p = ap;
	p1 = p->code;
	p2 = regs[RT1];
	while (*p1 && *p1!=',')
		*p2++ = *p1++;
	*p2++ = 0;
	p2 = regs[RT2];
	*p2 = 0;
	if (*p1++ !=',')
		return;
	while (*p2++ = *p1++);
}

int16_t findrand(char *as, int16_t flt)
{
	register int16_t i;
	for (i = flt; i<NREG+flt; i++) {
		if (equstr(regs[i], as))
			return(i);
	}
	return(-1);
}

int16_t isreg(char *as)
{
	register char *s;

	s = as;
	if (s[0]=='r' && s[1]>='0' && s[1]<='4' && s[2]==0)
		return(s[1]-'0');
	return(-1);
}

int16_t check(void)
{
	register struct node *p, *lp;

	lp = &first;
	for (p=first.forw; p!=0; p = p->forw) {
		if (p->back != lp)
			abort();
		lp = p;
	}
}

int16_t source(char *ap)
{
	register char *p1, *p2;

	p1 = ap;
	p2 = p1;
	if (*p1==0)
		return(0);
	while (*p2++);
	if (*p1=='-' && *(p1+1)=='('
	 || *p1=='*' && *(p1+1)=='-' && *(p1+2)=='('
	 || *(p2-2)=='+') {
		while (*p1 && *p1++!='r');
		if (*p1>='0' && *p1<='4')
			regs[*p1 - '0'][0] = 0;
		return(1);
	}
	return(0);
}

int16_t repladdr(struct node *p, int16_t f, int16_t flt)
{
	register int16_t r;
	int16_t r1;
	register char *p1, *p2;
	static char rt1[50], rt2[50];

	if (f)
		r1 = findrand(regs[RT2], flt);
	else
		r1 = -1;
	r = findrand(regs[RT1], flt);
	if (r1 >= NREG)
		r1 -= NREG;
	if (r >= NREG)
		r -= NREG;
	if (r>=0 || r1>=0) {
		p2 = regs[RT1];
		for (p1 = rt1; *p1++ = *p2++;);
		if (regs[RT2][0]) {
			p1 = rt2;
			*p1++ = ',';
			for (p2 = regs[RT2]; *p1++ = *p2++;);
		} else
			rt2[0] = 0;
		if (r>=0) {
			rt1[0] = 'r';
			rt1[1] = r + '0';
			rt1[2] = 0;
			nsaddr++;
		}
		if (r1>=0) {
			rt2[1] = 'r';
			rt2[2] = r1 + '0';
			rt2[3] = 0;
			nsaddr++;
		}
		p->code = copy(2, rt1, rt2);
	}
}

void movedat(void)
{
	register struct node *p1, *p2;
	struct node *p3;
	register int16_t seg;
	struct node data;
	struct node *datp;

	if (first.forw == 0)
		return;
	if (lastseg != TEXT && lastseg != -1) {
		p1 = (struct node *)alloc(sizeof(first));
		p1->op = lastseg;
		p1->subop = 0;
		p1->code = NULL;
		p1->forw = first.forw;
		p1->back = &first;
		first.forw->back = p1;
		first.forw = p1;
	}
	datp = &data;
	for (p1 = first.forw; p1!=0; p1 = p1->forw) {
		if (p1->op == DATA) {
			p2 = p1->forw;
			while (p2 && p2->op!=TEXT)
				p2 = p2->forw;
			if (p2==0)
				break;
			p3 = p1->back;
			p1->back->forw = p2->forw;
			p2->forw->back = p3;
			p2->forw = 0;
			datp->forw = p1;
			p1->back = datp;
			p1 = p3;
			datp = p2;
		}
	}
	if (data.forw) {
		datp->forw = first.forw;
		first.forw->back = datp;
		data.forw->back = &first;
		first.forw = data.forw;
	}
	seg = lastseg;
	for (p1 = first.forw; p1!=0; p1 = p1->forw) {
		if (p1->op==TEXT||p1->op==DATA||p1->op==BSS) {
			if (p2 = p1->forw) {
				if (p2->op==TEXT||p2->op==DATA||p2->op==BSS)
					p1->op  = p2->op;
			}
			if (p1->op == seg || p1->forw&&p1->forw->op==seg) {
				p1->back->forw = p1->forw;
				p1->forw->back = p1->back;
				p1 = p1->back;
				continue;
			}
			seg = p1->op;
		}
	}
}

void redunbr(struct node *p)
{
	register struct node *p1;
	register char *ap1;
	char *ap2;

	if ((p1 = p->ref) == 0)
		return;
	p1 = nonlab(p1);
	if (p1->op==TST) {
		singop(p1);
		savereg(RT2, "$0");
	} else if (p1->op==CMP)
		dualop(p1);
	else
		return;
	if (p1->forw->op!=CBR)
		return;
	ap1 = findcon(RT1);
	ap2 = findcon(RT2);
	p1 = p1->forw;
	if (compare(p1->subop, ap1, ap2)>0) {
		nredunj++;
		nchange++;
		decref(p->ref);
		p->ref = p1->ref;
		p->labno = p1->labno;
		p->ref->refc++;
	}
}

char * findcon(int16_t i)
{
	register char *p;
	register int16_t r;

	p = regs[i];
	if (*p=='$')
		return(p);
	if ((r = isreg(p)) >= 0)
		return(regs[r]);
	if (equstr(p, conloc))
		return(conval);
	return(p);
}

int16_t compare(int16_t oper, char *cp1, char *cp2)
{
	register unsigned n1, n2;

	if (*cp1++ != '$' || *cp2++ != '$')
		return(-1);
	n1 = 0;
	while (*cp2 >= '0' && *cp2 <= '7') {
		n1 <<= 3;
		n1 += *cp2++ - '0';
	}
	n2 = n1;
	n1 = 0;
	while (*cp1 >= '0' && *cp1 <= '7') {
		n1 <<= 3;
		n1 += *cp1++ - '0';
	}
	if (*cp1=='+')
		cp1++;
	if (*cp2=='+')
		cp2++;
	do {
		if (*cp1++ != *cp2)
			return(-1);
	} while (*cp2++);
	switch(oper) {

	case JEQ:
		return(n1 == n2);
	case JNE:
		return(n1 != n2);
	case JLE:
		return((int16_t)n1 <= (int16_t)n2);
	case JGE:
		return((int16_t)n1 >= (int16_t)n2);
	case JLT:
		return((int16_t)n1 < (int16_t)n2);
	case JGT:
		return((int16_t)n1 > (int16_t)n2);
	case JLO:
		return(n1 < n2);
	case JHI:
		return(n1 > n2);
	case JLOS:
		return(n1 <= n2);
	case JHIS:
		return(n1 >= n2);
	}
	return(-1);
}

void setcon(char *ar1, char *ar2)
{
	register char *cl, *cv, *p;

	cl = ar2;
	cv = ar1;
	if (*cv != '$')
		return;
	if (!natural(cl))
		return;
	p = conloc;
	while (*p++ = *cl++);
	p = conval;
	while (*p++ = *cv++);
}

int16_t equstr(char *ap1, char *ap2)
{
	char *p1, *p2;

	p1 = ap1;
	p2 = ap2;
	do {
		if (*p1++ != *p2)
			return(0);
	} while (*p2++);
	return(1);
}

void setcc(char *ap)
{
	register char *p, *p1;

	p = ap;
	if (!natural(p)) {
		ccloc[0] = 0;
		return;
	}
	p1 = ccloc;
	while (*p1++ = *p++);
}

int16_t natural(char *ap)
{
	register char *p;

	p = ap;
	if (*p=='*' || *p=='(' || *p=='-'&&*(p+1)=='(')
		return(0);
	while (*p++);
	p--;
	if (*--p == '+' || *p ==')' && *--p != '5')
		return(0);
	return(1);
}

