/*
 * cc2.pdp11.c -- Ritchie's PDP-11 peephole optimizer, modernized to C99.
 * Derived from V7 cmd/c/c20.c + c21.c + c2.h via knr2c99.py.
 *
 * The original is Copyright (C) Bell Telephone Laboratories, released
 * under the Caldera "ancient UNIX" license (BSD-style, 2002).
 *
 * Run by "pcc -O" on the assembly ccom writes, as dmr-cc ran c2 on c1's.
 * Option "=" (ours): track a register's contents correctly through
 * inc/dec in dest() (see conloc); the default output is unchanged.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdarg.h>

/*
 * Header for object code improver
 */


enum {
	JBR = 1,
	CBR = 2,
	JMP = 3,
	LABEL = 4,
	DLABEL = 5,
	EROU = 7,
	JSW = 9,
	MOV = 10,
	CLR = 11,
	COM = 12,
	INC = 13,
	DEC = 14,
	NEG = 15,
	TST = 16,
	ASR = 17,
	ASL = 18,
	SXT = 19,
	CMP = 20,
	ADD = 21,
	SUB = 22,
	BIT = 23,
	BIC = 24,
	BIS = 25,
	MUL = 26,
	DIV = 27,
	ASH = 28,
	XOR = 29,
	TEXT = 30,
	DATA = 31,
	BSS = 32,
	EVEN = 33,
	MOVF = 34,
	MOVOF = 35,
	MOVFO = 36,
	ADDF = 37,
	SUBF = 38,
	DIVF = 39,
	MULF = 40,
	CLRF = 41,
	CMPF = 42,
	NEGF = 43,
	TSTF = 44,
	CFCC = 45,
	SOB = 46,
	JSR = 47,
	END = 48,
};

enum {
	JEQ = 0,
	JNE = 1,
	JLE = 2,
	JGE = 3,
	JLT = 4,
	JGT = 5,
	JLO = 6,
	JHI = 7,
	JLOS = 8,
	JHIS = 9,
};

enum {
	BYTE = 100,
	LSIZE = 512,
};

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
int16_t	nauto;
int16_t	nspillp;

int16_t	nchange;
int16_t	isn;
int16_t	debug;
int16_t	lastseg;
char	revbr[];
char	regs[12][20];
char	conloc[20];
int16_t	fixconloc;	/* the "=" option: see dest() */
char	conval[20];
char	ccloc[20];

enum {
	RT1 = 10,
	RT2 = 11,
	FREG = 5,
	NREG = 5,
	LABHS = 127,
	OPHS = 57,
};

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
/*
 * Does q leave the condition codes those of register r, as a test of r would
 * (the overflow bit clear, so that signed branches agree)?  mov, bic, bis,
 * com and clr do; the arithmetic ones can set overflow.
 */
static int ccofreg(struct node *q, char *r)
{
	char *d;

	if (q->code == 0)
		return 0;
	switch (q->op) {
	case MOV:
	case BIC:
	case BIS:
		if (q->subop == BYTE && q->op != MOV)
			return 0;
		if ((d = strrchr(q->code, ',')) == 0)
			return 0;
		d++;
		break;
	case COM:
	case CLR:
		if (q->subop == BYTE)
			return 0;
		d = q->code;
		break;
	default:
		return 0;
	}
	return strcmp(d, r) == 0;
}

/*
 * The label l is followed by a test of register r.  If every way to l sets the
 * codes from r (the code before it, unless that jumps away, and every jump to
 * it, all unconditional), the test is redundant.
 */
static int joinscc(struct node *l, char *r)
{
	struct node *q;
	int n;

	if (l->op != LABEL || isreg(r) < 0)
		return 0;
	q = l->back;
	if (q && q->op != JBR && q->op != JMP && q->op != LABEL && !ccofreg(q, r))
		return 0;
	if (q && q->op == LABEL)
		return 0;
	n = 0;
	for (q = first.forw; q != 0; q = q->forw) {
		if (q->ref != l)
			continue;
		if (q->op != JBR || q->back == 0 || !ccofreg(q->back, r))
			return 0;
		n++;
	}
	return n == l->refc && n > 0;
}

/* Is r0 dead from q on: only moves and adds that do not mention it, then a call? */
static int r0dead(struct node *q)
{
	for (; ; q = q->forw) {
		if (q == 0 || q->code == 0 || strstr(q->code, "r0") != 0)
			return 0;
		if (q->op == JSR)
			return strncmp(q->code, "pc,", 3) == 0;
		if (q->op != MOV && q->op != CLR && q->op != ADD && q->op != SUB)
			return 0;
	}
}

/*
 * clr r0 ... mov r0,D with r0 dead afterwards, only plain moves between:
 * clr D at the place of the second (a long argument whose high word is zero:
 * clr -(sp), not through r0).  p is the clr.
 */
static int clrmov(struct node *c)
{
	struct node *q;

	if (c->op != CLR || c->subop == BYTE || c->code == 0 || strcmp(c->code, "r0") != 0)
		return 0;
	for (q = c->forw; q != 0 && q->code != 0 && q->op == MOV && q->subop != BYTE; q = q->forw) {
		if (strncmp(q->code, "r0,", 3) == 0) {
			if (strstr(q->code + 3, "r0") != 0 || !r0dead(q->forw))
				return 0;
			q->op = CLR;
			q->code = copy(1, q->code + 3);
			c->back->forw = c->forw;
			c->forw->back = c->back;
			nchange++;
			return 1;
		}
		if (strstr(q->code, "r0") != 0)
			return 0;
	}
	return 0;
}

/*
 * A word is made in r0 by each way into a label, and the label moves r0 on
 * ("c ? 1 : 2" as a call argument):  mov S,r0 ... L: mov r0,D.  When r0 is
 * dead afterwards (the call comes before any other use of it), each way can
 * move to D itself and the move at the label goes, as dmr-cc generates it.
 */
static int movjoin(struct node *p)
{
	struct node *l, *q, *pred[8];
	char *d, *c, buf[80];
	int n, i;

	if (p->op != MOV || p->subop == BYTE || p->code == 0 ||
	    strncmp(p->code, "r0,", 3) != 0 || strstr(p->code + 3, "r0") != 0)
		return 0;
	d = p->code + 3;
	l = p->back;
	if (l->op != LABEL)
		return 0;
	if (!r0dead(p->forw))
		return 0;
	n = 0;
	q = l->back;
	if (q->op != JBR && q->op != JMP) {
		if (q->op == LABEL)
			return 0;
		pred[n++] = q;
	}
	for (q = first.forw; q != 0; q = q->forw) {
		if (q->ref != l)
			continue;
		if (q->op != JBR || n >= 8 || q->back == 0)
			return 0;
		pred[n++] = q->back;
	}
	if (n == 0 || n - (l->back->op != JBR && l->back->op != JMP) != l->refc)
		return 0;
	for (i = 0; i < n; i++) {
		q = pred[i];
		if (q->op != MOV || q->subop == BYTE || q->code == 0 ||
		    (c = strrchr(q->code, ',')) == 0 || strcmp(c + 1, "r0") != 0 ||
		    c - q->code > 60)
			return 0;
	}
	for (i = 0; i < n; i++) {
		q = pred[i];
		c = strrchr(q->code, ',');
		memcpy(buf, q->code, c - q->code);
		buf[c - q->code] = 0;
		q->code = copy(3, buf, ",", d);
	}
	p->back->forw = p->forw;
	p->forw->back = p->back;
	nchange++;
	return 1;
}

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


/*
 * How many times does the text s name register rn (as a register, not as part
 * of a symbol)?
 */
static int nregref(const char *s, int rn)
{
	int i, n = 0;

	for (i = 0; s[i]; i++)
		if (s[i] == 'r' && s[i+1] == '0' + rn && !isalnum((unsigned char)s[i+2]) &&
		    (i == 0 || (!isalnum((unsigned char)s[i-1]) && s[i-1] != '_' && s[i-1] != '$')))
			n++;
	return n;
}

/*
 * add $2,rN (or inc rN, for a byte) just before the instruction that uses
 * -2(rN) once is (rN)+ in that instruction; sub $2,rN (dec rN) before a use of
 * (rN) is -(rN).  The PDP-11 does the step as the operand is evaluated, so
 * nothing else changes.  Both instructions set the codes, so those are the
 * instruction's own either way.  pcc makes pointers in registers that are not
 * `register` variables, and only those variables got the mode before.
 */
static int autoinc(void)
{
	struct node *a, *b;
	int rn, k, up, nb;
	char *c, pat[24], *at, buf[100];

	nb = 0;
	for (b = first.forw; b != 0; b = b->forw) {
		if (b->code == 0 || b->back == 0 || (a = b->back)->code == 0)
			continue;
		switch (b->op) {
		case MOV: case ADD: case SUB: case CMP: case BIT: case BIC: case BIS:
		case CLR: case TST: case INC: case DEC: case COM: case NEG: case ASR: case ASL:
			break;
		default:
			continue;
		}
		/* the step */
		c = a->code;
		if ((a->op == ADD || a->op == SUB) && a->subop != BYTE &&
		    c[0] == '$' && (c[1] == '2' || c[1] == '1') && c[2] == ',' &&
		    c[3] == 'r' && c[4] >= '0' && c[4] <= '4' && c[5] == 0) {
			k = c[1] - '0'; rn = c[4] - '0'; up = a->op == ADD;
		} else if ((a->op == INC || a->op == DEC) && c[0] == 'r' && c[1] >= '0' &&
		    c[1] <= '4' && c[2] == 0) {
			k = 1; rn = c[1] - '0'; up = a->op == INC;
		} else
			continue;
		/* a word steps by 2, a byte by 1 */
		if (k != (b->subop == BYTE ? 1 : 2))
			continue;
		if (up)
			snprintf(pat, sizeof pat, "-%d(r%d)", k, rn);
		else
			snprintf(pat, sizeof pat, "(r%d)", rn);
		if ((at = strstr(b->code, pat)) == 0 || (at > b->code && at[-1] == '*') ||
		    (!up && at > b->code && at[-1] == '-') ||
		    (!up && (at[strlen(pat)] == '+')))
			continue;
		if (strstr(at + 1, pat) != 0)
			continue;
		/* the register named once, or the destination of a source we step */
		if (nregref(b->code, rn) != 1) {
			char *comma = strrchr(b->code, ',');
			char dst[8];

			if (nregref(b->code, rn) != 2 || comma == 0 || at > comma)
				continue;
			snprintf(dst, sizeof dst, "r%d", rn);
			if (strcmp(comma + 1, dst) != 0)
				continue;
		}
		if ((size_t)(at - b->code) + strlen(b->code) > sizeof buf - 8)
			continue;
		memcpy(buf, b->code, at - b->code);
		buf[at - b->code] = 0;
		if (up)
			snprintf(buf + strlen(buf), sizeof buf - strlen(buf), "(r%d)+%s", rn, at + strlen(pat));
		else
			snprintf(buf + strlen(buf), sizeof buf - strlen(buf), "-(r%d)%s", rn, at + strlen(pat));
		b->code = copy(1, buf);
		a->back->forw = b;
		b->back = a->back;
		nb++;
	}
	return nb;
}

/*
 * The extent of the function that contains node n: from its csv call to the next.
 */
static void funcextent(struct node *n, struct node **lo, struct node **hi)
{
	struct node *q;

	*lo = &first;
	for (q = n; q != 0 && q != &first; q = q->back)
		if (q->op == JSR && q->code && strncmp(q->code, "r5,csv", 6) == 0) {
			*lo = q;
			break;
		}
	*hi = 0;
	for (q = n->forw; q != 0; q = q->forw)
		if (q->op == JSR && q->code && strncmp(q->code, "r5,csv", 6) == 0) {
			*hi = q;
			break;
		}
}

/* How often does a frame slot, "off(r5)", occur in the function around n? */
static int slotrefs(struct node *n, int off)
{
	struct node *lo, *hi, *q;
	char pat[24];
	int c = 0;

	snprintf(pat, sizeof pat, "%d(r5)", off);
	funcextent(n, &lo, &hi);
	for (q = lo->forw; q != 0 && q != hi; q = q->forw) {
		char *at = q->code;

		while (q->code && (at = strstr(at, pat)) != 0) {
			/* the whole number, not the tail of a longer one */
			if (at == q->code || !(isdigit((unsigned char)at[-1])))
				c++;
			at += strlen(pat);
		}
	}
	return c;
}

/*
 * A value spilled to a frame slot and reloaded straight after, a long that has
 * to wait for another to be computed:
 *	mov r0,-16(r5); mov r1,-14(r5); <loads>; sub -16(r5),r0; sub -14(r5),r1
 * is a push and a pop, as dmr-cc does it:
 *	mov r1,-(sp); mov r0,-(sp); <loads>; sub (sp)+,r0; sub (sp)+,r1
 * when the slots are used only here and nothing in between touches the stack
 * or leaves the straight line.  A one-word spill is the same with one push.
 */
static int spillpush(void)
{
	struct node *s1, *s2, *q, *u1, *u2, *bn;
	int off1, off2 = 0, n, r1, r2 = 0, pairs, nb = 0, win;
	char a[8], b[8], buf[60];

	for (s1 = first.forw; s1 != 0; s1 = s1->forw) {
		if (s1->op != MOV || s1->subop == BYTE || s1->code == 0)
			continue;
		n = -1;
		if (sscanf(s1->code, "r%d,%d(r5)%n", &r1, &off1, &n) != 2 || n < 0 ||
		    s1->code[n] != 0 || r1 > 4 || off1 >= 0)
			continue;
		/* one word, or two if the next store is the next word of a long */
		s2 = s1->forw;
		pairs = 1;
		n = -1;
		if (s2 && s2->op == MOV && s2->subop != BYTE && s2->code &&
		    sscanf(s2->code, "r%d,%d(r5)%n", &r2, &off2, &n) == 2 && n >= 0 &&
		    s2->code[n] == 0 && off2 == off1 + 2 && r2 <= 4 && r2 != r1)
			pairs = 2;
		if (slotrefs(s1, off1) != 2 || (pairs == 2 && slotrefs(s1, off2) != 2))
			continue;
		/* the reloads: a straight line to them, no stack use on the way */
		snprintf(a, sizeof a, "%d(r5)", off1);
		snprintf(b, sizeof b, "%d(r5)", off2);
		bn = pairs == 2 ? s2 : s1;
		u1 = u2 = 0;
		for (q = bn->forw, win = 0; q != 0 && win < 10; q = q->forw, win++) {
			if (q->op == LABEL || q->op == JBR || q->op == CBR || q->op == JMP ||
			    q->op == JSR || q->op == JSW || q->code == 0 || strstr(q->code, "sp") != 0)
				break;
			if (!u1 && strncmp(q->code, a, strlen(a)) == 0 && q->code[strlen(a)] == ',' &&
			    q->subop != BYTE && (q->op == MOV || q->op == ADD || q->op == SUB ||
			    q->op == CMP || q->op == BIS || q->op == BIC)) {
				u1 = q;
				if (pairs == 1)
					break;
				continue;
			}
			if (pairs == 2 && u1 && !u2 && strncmp(q->code, b, strlen(b)) == 0 &&
			    q->code[strlen(b)] == ',' && q->subop != BYTE &&
			    (q->op == MOV || q->op == ADD || q->op == SUB || q->op == CMP ||
			    q->op == BIS || q->op == BIC)) {
				u2 = q;
				break;
			}
			/* any other mention of the slots in the window spoils it */
			if (strstr(q->code, a) != 0 || (pairs == 2 && strstr(q->code, b) != 0))
				break;
		}
		if (!u1 || (pairs == 2 && !u2))
			continue;
		/* the pops are in the order of the stores: push the later word first */
		if (pairs == 2) {
			snprintf(buf, sizeof buf, "r%d,-(sp)", r2);
			s1->code = copy(1, buf);
			snprintf(buf, sizeof buf, "r%d,-(sp)", r1);
			s2->code = copy(1, buf);
		} else {
			snprintf(buf, sizeof buf, "r%d,-(sp)", r1);
			s1->code = copy(1, buf);
		}
		snprintf(buf, sizeof buf, "(sp)+%s", u1->code + strlen(a));
		u1->code = copy(1, buf);
		if (pairs == 2) {
			snprintf(buf, sizeof buf, "(sp)+%s", u2->code + strlen(b));
			u2->code = copy(1, buf);
		}
		nb++;
	}
	return nb;
}

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
	if (argc>1 && argv[1][0]=='=') {
		/* fix V7's stale constant tracking, see dest() */
		argc--;
		argv++;
		fixconloc++;
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
		{ int n; while ((n = autoinc()) > 0) nauto += n; }
		nspillp += spillpush();
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
		fprintf(stderr, "%d autoincrement modes\n", nauto);
		fprintf(stderr, "%d spills made pushes\n", nspillp);
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
	curlp = line;	/* a line with nothing on it: not NULL */
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
			if (ophp >= &ophash[OPHS])
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
		if (movjoin(p)) {
			p = p->back;
			continue;
		}
		dualop(p);
		/*
		 * mov rN,rN moves nothing; it is a test of rN.  If the codes are
		 * those of rN already, it is not even that.
		 */
		if (flt == 0 && isreg(regs[RT1]) >= 0 && equstr(regs[RT1], regs[RT2]) &&
		    (p->forw->op != CBR || equstr(regs[RT2], ccloc) ||
		    joinscc(p->back, regs[RT2]))) {
			p->forw->back = p->back;
			p->back->forw = p->forw;
			redunm++;
			nchange++;
			continue;
		}
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
		/* movb to a register sets the codes of the sign-extended byte */
		if (p->op==MOV && p->subop==BYTE && flt==0 && isreg(regs[RT2]) >= 0)
			setcc(regs[RT2]);
		else
			ccloc[0] = 0;
		continue;

	case CLRF:
	case NEGF:
		flt = NREG;

	case CLR:
		if (p->op == CLR && clrmov(p)) {
			p = p->back;
			continue;
		}
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
		if (equstr(regs[RT1], ccloc) || joinscc(p->back, regs[RT1])) {
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
	/*
	 * V7's dest() forgets that a location holding a known constant
	 * (after clr or mov $k) has been changed by inc, dec, neg, ...: a
	 * later cmp or tst of it is then folded using the old value.  The
	 * "=" option turns the fix on; without it c2 stays exactly V7's.
	 */
	if (fixconloc && equstr(s, conloc))
		conloc[0] = 0;
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
	if (p1 == NULL) {	/* empty operand string (copy() returned 0); PDP-11 read the null page */
		regs[RT1][0] = 0;
		regs[RT2][0] = 0;
		return(0);
	}
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
	if (p1 == NULL) {	/* empty operand string (copy() returned 0); PDP-11 read the null page */
		regs[RT1][0] = 0;
		regs[RT2][0] = 0;
		return;
	}
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

