/*
 * C code generator header
 */

#define	_DEFAULT_SOURCE	1	/* sbrk(), on glibc */

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/mman.h>

struct	acl;	/* forward: defined in the optimizer (c12) */

#define	LTYPE	int32_t	/* change to int for no long consts */
#define	NCPS	8

/*
 * Tree node: a tagged union of every node shape.  The leading `op`/`type` pair
 * is common to all shapes; the rest hangs off `u`.  V7 C kept one global member-
 * name space (you could reach any member through any struct pointer), which C99
 * forbids, so each divergent member is qualified by its sub-struct below.
 */
struct	node {
	int16_t	op;
	int16_t	type;
	union {
		struct { int16_t degree; struct node *tr1, *tr2; }	tnode;
		struct { char class, regno; int16_t offset, nloc; }	tname;
		struct { char class, regno; int16_t offset; char name[NCPS]; } xtname;
		struct { int16_t value; }				tconst;
		struct { int32_t lvalue; }				lconst;
		struct { int16_t value; double fvalue; }		ftconst;
		struct { int16_t degree; struct node *tr1, *tr2; int16_t mask; } fasgn;
	} u;
};

struct	optab {
	char	tabdeg1;
	char	tabtyp1;
	char	tabdeg2;
	char	tabtyp2;
	char	*tabstring;
};

struct	table {
	int16_t	tabop;
	struct	optab *tabp;
};

struct	instab {
	int16_t	iop;
	char	*str1;
	char	*str2;
};

struct	swtab {
	int16_t	swlab;
	int16_t	swval;
};

/* cross-file function prototypes (generated) */
extern struct node * acommute(struct node *atree);
extern int16_t arlength(int16_t t);
extern int16_t branch(int16_t lbl, int16_t aop, int16_t c);
extern int16_t breq(int16_t v, int16_t l);
extern void c1_const(int16_t op, int16_t *vp, int16_t av);
extern void cbranch(struct node *atree, int16_t albl, int16_t cond, int16_t areg);
extern int16_t cexpr(struct node *atree, struct table *table, int16_t areg);
extern int16_t chkleaf(struct node *atree, struct table *table, int16_t reg);
extern int16_t collcon(struct node *ap);
extern int16_t comarg(struct node *atree, int16_t *flagp);
extern int16_t dcalc(struct node *ap, int16_t nrleft);
extern int16_t decref(int16_t at);
extern int16_t degree(struct node *at);
extern int16_t delay(struct node **treep, struct table *table, int16_t reg);
extern void distrib(struct acl *list);
extern void doinit(int16_t atype, struct node *atree);
extern int error(char *s, ...);
extern void * getblk(int16_t size);
extern int16_t geti(void);
extern void getree(void);
extern struct node * hardlongs(struct node *at);
extern int16_t incref(int16_t t);
extern void insert(int16_t op, struct node *atree, struct acl *alist);
extern struct node * isconstant(struct node *at);
extern int16_t isfloat(struct node *at);
extern int16_t islong(int16_t t);
extern int16_t ispow2(struct node *atree);
extern int16_t label(int16_t l);
extern struct node * lconst(int16_t op, struct node *lp, struct node *rp);
extern int16_t longrel(struct node *atree, int16_t lbl, int16_t cond, int16_t reg);
extern struct node * lvfield(struct node *at);
extern struct optab * match(struct node *atree, struct table *table, int16_t nrleft, int16_t nocvt);
extern int16_t max(int16_t a, int16_t b);
extern void movreg(int16_t r0, int16_t r1, struct node *tree);
extern struct node * ncopy(struct node *ap);
extern int16_t notcompat(struct node *ap, int16_t ast, int16_t op);
extern int16_t oddreg(struct node *t, int16_t areg);
extern struct node * optim(struct node *atree);
extern char * outname(char *s);
extern int16_t pbase(struct node *ap);
extern void pname(struct node *ap, int16_t flag);
extern void popstk(int16_t a);
extern struct node * pow2(struct node *atree);
extern void prins(int16_t op, int16_t c, struct instab *itable);
extern int16_t psoct(int16_t an);
extern void pswitch(struct swtab *afp, struct swtab *alp, int16_t deflab);
extern int16_t rcexpr(struct node *atree, struct table *atable, int16_t reg);
extern int16_t regerr(void);
extern int16_t reorder(struct node **treep, struct table *table, int16_t reg);
extern struct node * sdelay(struct node **ap);
extern int16_t setype(struct node *p, int16_t t);
extern int16_t sort(struct swtab *afp, struct swtab *alp);
extern int16_t squash(struct node **p, struct node **maxp);
extern int16_t sreorder(struct node **treep, struct table *table, int16_t reg, int16_t recurf);
extern void strasg(struct node *atp);
extern struct node * strfunc(struct node *atp);
extern struct node * tconst(int16_t val, int16_t type);
extern struct node * tnode(int16_t op, int16_t type, struct node *tr1, struct node *tr2);
extern struct node * unoptim(struct node *atree);
extern int16_t xdcalc(struct node *ap, int16_t nrleft);
extern int16_t xlongrel(int16_t f);


extern char	maprel[];
extern char	notrel[];
extern int16_t	nreg;
extern int16_t	isn;
extern int16_t	namsiz;
extern int16_t	line;
extern int16_t	nerror;
extern struct	table	cctab[];
extern struct	table	efftab[];
extern struct	table	regtab[];
extern struct	table	sptab[];
extern struct	table	lsptab[1];
extern struct	instab	instab[];
extern struct	instab	branchtab[];
extern int16_t	opdope[];
extern char	*opntab[];
extern int16_t	nstack;
extern int16_t	nfloat;
extern struct	node	sfuncr;
extern char	*funcbase;
extern char	*curbase;
extern char	*coremax;
extern struct	node	czero, cone;
extern struct	node	fczero;
extern int32_t	totspace;
/*
 * Some special stuff for long comparisons
 */
extern int16_t	xlab1, xlab2, xop, xzero;

/*
	operators
*/
#define	EOFC	0
#define	SEMI	1
#define	LBRACE	2
#define	RBRACE	3
#define	LBRACK	4
#define	RBRACK	5
#define	LPARN	6
#define	RPARN	7
#define	COLON	8
#define	COMMA	9
#define	FSEL	10
#define	FSELR	11
#define	FSELT	12
#define	FSELA	16
#define	ULSH	17
#define	ASULSH	18

#define	KEYW	19
#define	NAME	20
#define	CON	21
#define	STRING	22
#define	FCON	23
#define	SFCON	24
#define	LCON	25
#define	SLCON	26

#define	AUTOI	27
#define	AUTOD	28
#define	INCBEF	30
#define	DECBEF	31
#define	INCAFT	32
#define	DECAFT	33
#define	EXCLA	34
#define	AMPER	35
#define	STAR	36
#define	NEG	37
#define	COMPL	38

#define	DOT	39
#define	PLUS	40
#define	MINUS	41
#define	TIMES	42
#define	DIVIDE	43
#define	MOD	44
#define	RSHIFT	45
#define	LSHIFT	46
#define	AND	47
#define	ANDN	55
#define	OR	48
#define	EXOR	49
#define	ARROW	50
#define	ITOF	51
#define	FTOI	52
#define	LOGAND	53
#define	LOGOR	54
#define	FTOL	56
#define	LTOF	57
#define	ITOL	58
#define	LTOI	59
#define	ITOP	13
#define	PTOI	14
#define	LTOP	15

#define	EQUAL	60
#define	NEQUAL	61
#define	LESSEQ	62
#define	LESS	63
#define	GREATEQ	64
#define	GREAT	65
#define	LESSEQP	66
#define	LESSP	67
#define	GREATQP	68
#define	GREATP	69

#define	ASPLUS	70
#define	ASMINUS	71
#define	ASTIMES	72
#define	ASDIV	73
#define	ASMOD	74
#define	ASRSH	75
#define	ASLSH	76
#define	ASAND	77
#define	ASOR	78
#define	ASXOR	79
#define	ASSIGN	80
#define	TAND	81
#define	LTIMES	82
#define	LDIV	83
#define	LMOD	84
#define	ASANDN	85
#define	LASTIMES 86
#define	LASDIV	87
#define	LASMOD	88

#define	QUEST	90
#define	MAX	93
#define	MAXP	94
#define	MIN	95
#define	MINP	96
#define	LLSHIFT	91
#define	ASLSHL	92
#define	SEQNC	97
#define	CALL1	98
#define	CALL2	99
#define	CALL	100
#define	MCALL	101
#define	JUMP	102
#define	CBRANCH	103
#define	INIT	104
#define	SETREG	105
#define	LOAD	106
#define	ITOC	109
#define	RFORCE	110

/*
 * Intermediate code operators
 */
#define	BRANCH	111
#define	LABEL	112
#define	NLABEL	113
#define	RLABEL	114
#define	STRASG	115
#define	STRSET	116
#define	BDATA	200
#define	PROG	202
#define	DATA	203
#define	BSS	204
#define	CSPACE	205
#define	SSPACE	206
#define	SYMDEF	207
#define	SAVE	208
#define	RETRN	209
#define	EVEN	210
#define	PROFIL	212
#define	SWIT	213
#define	EXPR	214
#define	SNAME	215
#define	RNAME	216
#define	ANAME	217
#define	NULLOP	218
#define	SETSTK	219
#define	SINIT	220
#define	GLOBAL	221
#define	C3BRANCH	222

/*
 *	types
 */
#define	INT	0
#define	CHAR	1
#define	FLOAT	2
#define	DOUBLE	3
#define	STRUCT	4
#define	RSTRUCT	5
#define	LONG	6
#define	UNSIGN	7

#define	TYLEN	2
#define	TYPE	07
#define	XTYPE	(03<<3)
#define	PTR	010
#define	FUNC	020
#define	ARRAY	030

/*
	storage	classes
*/
#define	KEYWC	1
#define	MOS	10
#define	AUTO	11
#define	EXTERN	12
#define	STATIC	13
#define	REG	14
#define	STRTAG	15
#define	ARG	16
#define	OFFS	20
#define	XOFFS	21
#define	SOFFS	22

/*
	Flag	bits
*/

#define	BINARY	01
#define	LVALUE	02
#define	RELAT	04
#define	ASSGOP	010
#define	LWORD	020
#define	RWORD	040
#define	COMMUTE	0100
#define	RASSOC	0200
#define	LEAF	0400
#define	CNVRT	01000
