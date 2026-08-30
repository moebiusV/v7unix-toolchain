#
/*
* 	C compiler-- first pass header
*/

#define	_DEFAULT_SOURCE	1	/* sbrk(), on glibc -- must precede any glibc header */
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

/*
 * parameters
 */

typedef int32_t LTYPE; /* change to int if no long consts */
enum {
	MAXINT = 077777,	/* Largest positive short integer */
	MAXUINT = 0177777,	/* largest unsigned integer */
	NCPS = 8,	/* # chars per symbol */
	HSHSIZ = 400,	/* # entries in hash table for names */
	CMSIZ = 40,	/* size of expression stack */
	SSIZE = 20,	/* size of other expression stack */
	SWSIZ = 230,	/* size of switch table */
	NMEMS = 128,	/* Number of members in a structure */
	NBPW = 16,	/* bits per word, object machine */
	NBPC = 8,	/* bits per character, object machine */
	NCPW = 2,	/* chars per word, object machine */
	LNCPW = 2,	/* chars per word, compiler's machine */
	STAUTO = (-6),	/* offset of first auto variable */
	STARG = 4,	/* offset of first argument */
};


/*
 * # bytes in primitive types
 */
enum {
	SZCHAR = 1,
	SZINT = 2,
	SZPTR = 2,
	SZFLOAT = 4,
	SZLONG = 4,
	SZDOUB = 8,
};

struct node;	/* forward: node is a tagged union of tree + namelist shapes */

/*
 * format of a structure description
 */
struct str {
	int16_t	ssize;			/* structure size */
	struct node **memlist;		/* member list */
};

/*
 * For fields, strp points here instead.
 */
struct field {
	int16_t	flen;		/* field width in bits */
	int16_t	bitoffs;	/* shift count */
};

/*
 * A node is a tagged union of the operator/constant shapes AND the namelist
 * entry.  V7 kept one global member-name space, so a `struct tnode *` could
 * reach `value` (cnode), `hclass`/`htype`/`hoffset` (hshtab), `tr1` (tnode),
 * and vice versa.  C99 forbids that, so every member access is qualified by its
 * sub-struct (`->u.tnode.op`, `->u.cnode.value`, `->u.hshtab.hclass`).
 */
struct	node {
	union {
		struct {
			int16_t op, type;
			int16_t *subsp;
			struct str *strp;
			struct node *tr1, *tr2;
		} tnode;
		struct {
			int16_t op, type;
			int16_t *subsp;
			struct str *strp;
			int16_t value;
		} cnode;
		struct {
			int16_t op, type;
			int16_t *subsp;
			struct str *strp;
			int32_t lvalue;
		} lnode;
		struct {
			int16_t op, type;
			int16_t *subsp;
			struct str *strp;
			char *cstr;
		} fnode;
		struct {
			char hclass, hflag;
			int16_t htype;
			int16_t *hsubsp;
			struct str *hstrp;
			int16_t hoffset;
			struct node *hpdown;
			char hblklev;
			char name[NCPS];
			struct node *next;	/* parameter-list link (V7 reused hoffset) */
		} hshtab;
		struct {
			char hclass, hflag;
			int16_t htype;
			int16_t *hsubsp;
			struct str *hstrp;
			int16_t hoffset;
			struct node *hpdown;
			char hblklev;
		} phshtab;
	} u;
};

/*
 * Place used to keep dimensions
 * during declarations
 */
struct	tdim {
	int16_t	rank;
	int16_t	dimens[5];
};

/*
 * Table for recording switches.
 */
struct swtab {
	int16_t	swlab;
	int16_t	swval;
};

extern char	cvtab[4][4];
extern char	filename[64];
extern int16_t	opdope[];
extern char	ctab[];
extern char	symbuf[NCPS+2];
extern int16_t	hshused;
extern struct node	hshtab[HSHSIZ];
extern struct node **cp;
extern int16_t	isn;
extern struct	swtab	swtab[SWSIZ];
extern struct	swtab	*swp;
extern int16_t	contlab;
extern int16_t	brklab;
extern int16_t	retlab;
extern int16_t	deflab;
extern unsigned autolen;		/* make these int if necessary */
extern unsigned maxauto;		/* ... will only cause trouble rarely */
extern int16_t	peeksym;
extern int16_t	peekc;
extern int16_t	eof;
extern int16_t	line;
extern char	*funcbase;
extern char	*curbase;
extern char	*coremax;
extern char	*maxdecl;
extern struct node	*defsym;
extern struct node	*funcsym;
extern int16_t	proflg;
extern struct node	*csym;
extern int16_t	cval;
extern int32_t	lcval;
extern int16_t	nchstr;
extern int16_t	nerror;
extern struct node	*paraml;
extern struct node	*parame;
extern int16_t	strflg;
extern int16_t	mosflg;
extern int16_t	initflg;
extern int16_t	inhdr;
extern char	sbuf[BUFSIZ];
extern FILE	*sbufp;
extern int16_t	regvar;
extern int16_t	bitoffs;
extern struct node	funcblk;
extern char	cvntab[];
extern char	numbuf[64];
extern struct node **memlist;
extern int16_t	nmems;
extern struct node	structhole;
extern int16_t	blklev;
extern int16_t	mossym;

/*
 * Function prototypes (cross-file; host).
 */
int16_t lookup(void);
int16_t findkw(void);
int16_t symbol(void);
int16_t getnum(void);
int16_t subseq(int16_t c, int16_t a, int16_t b);
int16_t putstr(int16_t lab, int16_t amax);
int16_t getcc(void);
int16_t mapch(int16_t ac);
struct node * tree(void);
struct node * xprtype(struct node *atyb);
char * copnum(int16_t len);
void build(int16_t op);
struct node * convert(struct node *p, int16_t t, int16_t cvn, int16_t len);
int16_t setype(struct node *ap, int16_t at, struct node *anewp);
struct node * chkfun(struct node *ap);
struct node * disarray(struct node *ap);
void chkw(struct node *p, int16_t okt);
int16_t lintyp(int16_t t);
int error(char *s, ...);
struct node * block(int16_t op, int16_t t, int16_t *subs, struct str *str, struct node *p1, struct node *p2);
struct node * nblock(struct node *ads);
struct node * cblock(int16_t v);
struct node * fblock(int16_t t, char *string);
void * gblock(int16_t n);
int16_t chklval(struct node *ap);
int16_t fold(int16_t op, struct node *ap1, struct node *ap2);
int16_t conexp(void);
void extdef(void);
int16_t cfunc(void);
int16_t cinit(struct node *anp, int16_t flex, int16_t sclass);
int16_t strinit(struct node *np, int16_t sclass);
int16_t setinit(struct node *anp);
void statement(void);
int16_t forstmt(void);
struct node * pexpr(void);
int16_t pswitch(void);
int16_t funchead(void);
int16_t blockhead(void);
int16_t blkend(void);
void prste(struct node *acs);
int16_t errflush(int16_t ao);
int16_t declist(int16_t sclass);
int16_t getkeywords(int16_t *scptr, struct node *tptr);
struct str * strdec(int16_t mosf, int16_t kind);
int16_t declare(int16_t askw, struct node *tptr, int16_t offset);
int16_t decl1(int16_t askw, struct node *atptr, int16_t offset, struct node *absname);
int16_t pushdecl(struct node *asp);
int16_t cpysymb(struct node *s1, struct node *s2);
int16_t getype(struct tdim *adimp, struct node *absname);
int16_t typov(void);
int16_t align(int16_t type, int16_t offset, int16_t aflen);
int16_t decsyn(int16_t o);
int16_t redec(void);
int16_t goodreg(struct node *hp);
int16_t decref(int16_t at);
int16_t incref(int16_t t);
int16_t cbranch(struct node *t, int16_t lbl, int16_t cond);
void rcexpr(struct node *atp);
void treeout(struct node *atp, int16_t isstruct);
int16_t branch(int16_t lab);
int16_t label(int16_t l);
int16_t plength(struct node *ap);
int16_t length(struct node *acs);
int16_t rlength(struct node *cs);
int16_t simplegoto(void);
int16_t nextchar(void);
int16_t spnextchar(void);
int16_t chconbrk(int16_t l);
int16_t dogoto(void);
int16_t doret(void);
void outcode(char *s, ...);

/*
  operators
*/
enum {
	EOFC = 0,
	NULLOP = 218,
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
	CAST = 11,
	ETYPE = 12,
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
	SIZEOF = 91,
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
	ASSAND = 77,
	ASOR = 78,
	ASXOR = 79,
	ASSIGN = 80,
};

enum {
	QUEST = 90,
	MAX = 93,
	MAXP = 94,
	MIN = 95,
	MINP = 96,
	SEQNC = 97,
	CALL = 100,
	MCALL = 101,
	JUMP = 102,
	CBRANCH = 103,
	INIT = 104,
	SETREG = 105,
	RFORCE = 110,
	BRANCH = 111,
	LABEL = 112,
	NLABEL = 113,
	RLABEL = 114,
	STRASG = 115,
	ITOC = 109,
	SEOF = 200,	/* stack EOF marker in expr compilation */
};

/*
  types
*/
enum {
	INT = 0,
	CHAR = 1,
	FLOAT = 2,
	DOUBLE = 3,
	STRUCT = 4,
	LONG = 6,
	UNSIGN = 7,
	UNION = 8,	/* adjusted later to struct */
};

enum {
	ALIGN = 01,
	TYPE = 07,
	BIGTYPE = 060000,
	TYLEN = 2,
	XTYPE = (03<<3),
	PTR = 010,
	FUNC = 020,
	ARRAY = 030,
};

/*
  storage classes
*/
enum {
	KEYWC = 1,
	DEFXTRN = 20,
	TYPEDEF = 9,
	MOS = 10,
	AUTO = 11,
	EXTERN = 12,
	STATIC = 13,
	REG = 14,
	STRTAG = 15,
	ARG = 16,
	ARG1 = 17,
	AREG = 18,
	MOU = 21,
	ENUMTAG = 22,
	ENUMCON = 24,
};

/*
  keywords
*/
enum {
	GOTO = 20,
	RETURN = 21,
	IF = 22,
	WHILE = 23,
	ELSE = 24,
	SWITCH = 25,
	CASE = 26,
	BREAK = 27,
	CONTIN = 28,
	DO = 29,
	DEFAULT = 30,
	FOR = 31,
	ENUM = 32,
};

/*
  characters
*/
enum {
	BSLASH = 117,
	SHARP = 118,
	INSERT = 119,
	PERIOD = 120,
	SQUOTE = 121,
	DQUOTE = 122,
	LETTER = 123,
	DIGIT = 124,
	NEWLN = 125,
	SPACE = 126,
	UNKN = 127,
};

/*
 * Special operators in intermediate code
 */
enum {
	BDATA = 200,
	WDATA = 201,
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
	SETSTK = 219,
	SINIT = 220,
};

/*
  Flag bits
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
};

/*
 * Conversion codes
 */
enum {
	ITF = 1,
	ITL = 2,
	LTF = 3,
	ITP = 4,
	PTI = 5,
	FTI = 6,
	LTI = 7,
	FTL = 8,
	LTP = 9,
	ITC = 10,
	XX = 15,
};

/*
 * symbol table flags
 */

enum {
	FMOS = 01,
	FKEYW = 04,
	FFIELD = 020,
	FINIT = 040,
	FLABL = 0100,
};

