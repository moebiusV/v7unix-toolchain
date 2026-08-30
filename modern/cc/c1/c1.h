/*
 * C code generator header
 */

#define	_DEFAULT_SOURCE	1	/* sbrk(), on glibc */

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/mman.h>

struct	acl;	/* forward: defined in the optimizer (c12) */

typedef int32_t LTYPE; /* change to int for no long consts */
enum {
	NCPS = 8,
};

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
enum {
	EOFC = 0,
	SEMI = 1,
	LBRACE = 2,
	RBRACE = 3,
	LBRACK = 4,
	RBRACK = 5,
	LPARN = 6,
	RPARN = 7,
	COLON = 8,
	COMMA = 9,
	FSEL = 10,
	FSELR = 11,
	FSELT = 12,
	FSELA = 16,
	ULSH = 17,
	ASULSH = 18,
};

enum {
	KEYW = 19,
	NAME = 20,
	CON = 21,
	STRING = 22,
	FCON = 23,
	SFCON = 24,
	LCON = 25,
	SLCON = 26,
};

enum {
	AUTOI = 27,
	AUTOD = 28,
	INCBEF = 30,
	DECBEF = 31,
	INCAFT = 32,
	DECAFT = 33,
	EXCLA = 34,
	AMPER = 35,
	STAR = 36,
	NEG = 37,
	COMPL = 38,
};

enum {
	DOT = 39,
	PLUS = 40,
	MINUS = 41,
	TIMES = 42,
	DIVIDE = 43,
	MOD = 44,
	RSHIFT = 45,
	LSHIFT = 46,
	AND = 47,
	ANDN = 55,
	OR = 48,
	EXOR = 49,
	ARROW = 50,
	ITOF = 51,
	FTOI = 52,
	LOGAND = 53,
	LOGOR = 54,
	FTOL = 56,
	LTOF = 57,
	ITOL = 58,
	LTOI = 59,
	ITOP = 13,
	PTOI = 14,
	LTOP = 15,
};

enum {
	EQUAL = 60,
	NEQUAL = 61,
	LESSEQ = 62,
	LESS = 63,
	GREATEQ = 64,
	GREAT = 65,
	LESSEQP = 66,
	LESSP = 67,
	GREATQP = 68,
	GREATP = 69,
};

enum {
	ASPLUS = 70,
	ASMINUS = 71,
	ASTIMES = 72,
	ASDIV = 73,
	ASMOD = 74,
	ASRSH = 75,
	ASLSH = 76,
	ASAND = 77,
	ASOR = 78,
	ASXOR = 79,
	ASSIGN = 80,
	TAND = 81,
	LTIMES = 82,
	LDIV = 83,
	LMOD = 84,
	ASANDN = 85,
	LASTIMES = 86,
	LASDIV = 87,
	LASMOD = 88,
};

enum {
	QUEST = 90,
	MAX = 93,
	MAXP = 94,
	MIN = 95,
	MINP = 96,
	LLSHIFT = 91,
	ASLSHL = 92,
	SEQNC = 97,
	CALL1 = 98,
	CALL2 = 99,
	CALL = 100,
	MCALL = 101,
	JUMP = 102,
	CBRANCH = 103,
	INIT = 104,
	SETREG = 105,
	LOAD = 106,
	ITOC = 109,
	RFORCE = 110,
};

/*
 * Intermediate code operators
 */
enum {
	BRANCH = 111,
	LABEL = 112,
	NLABEL = 113,
	RLABEL = 114,
	STRASG = 115,
	STRSET = 116,
	BDATA = 200,
	PROG = 202,
	DATA = 203,
	BSS = 204,
	CSPACE = 205,
	SSPACE = 206,
	SYMDEF = 207,
	SAVE = 208,
	RETRN = 209,
	EVEN = 210,
	PROFIL = 212,
	SWIT = 213,
	EXPR = 214,
	SNAME = 215,
	RNAME = 216,
	ANAME = 217,
	NULLOP = 218,
	SETSTK = 219,
	SINIT = 220,
	GLOBAL = 221,
	C3BRANCH = 222,
};

/*
 *	types
 */
enum {
	INT = 0,
	CHAR = 1,
	FLOAT = 2,
	DOUBLE = 3,
	STRUCT = 4,
	RSTRUCT = 5,
	LONG = 6,
	UNSIGN = 7,
};

enum {
	TYLEN = 2,
	TYPE = 07,
	XTYPE = (03<<3),
	PTR = 010,
	FUNC = 020,
	ARRAY = 030,
};

/*
	storage	classes
*/
enum {
	KEYWC = 1,
	MOS = 10,
	AUTO = 11,
	EXTERN = 12,
	STATIC = 13,
	REG = 14,
	STRTAG = 15,
	ARG = 16,
	OFFS = 20,
	XOFFS = 21,
	SOFFS = 22,
};

/*
	Flag	bits
*/

enum {
	BINARY = 01,
	LVALUE = 02,
	RELAT = 04,
	ASSGOP = 010,
	LWORD = 020,
	RWORD = 040,
	COMMUTE = 0100,
	RASSOC = 0200,
	LEAF = 0400,
	CNVRT = 01000,
};
