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

#define	LTYPE	int32_t	/* change to int if no long consts */
#define	MAXINT	077777	/* Largest positive short integer */
#define	MAXUINT	0177777	/* largest unsigned integer */
#define	NCPS	8	/* # chars per symbol */
#define	HSHSIZ	400	/* # entries in hash table for names */
#define	CMSIZ	40	/* size of expression stack */
#define	SSIZE	20	/* size of other expression stack */
#define	SWSIZ	230	/* size of switch table */
#define	NMEMS	128	/* Number of members in a structure */
#define	NBPW	16	/* bits per word, object machine */
#define	NBPC	8	/* bits per character, object machine */
#define	NCPW	2	/* chars per word, object machine */
#define	LNCPW	2	/* chars per word, compiler's machine */
#define	STAUTO	(-6)	/* offset of first auto variable */
#define	STARG	4	/* offset of first argument */


/*
 * # bytes in primitive types
 */
#define	SZCHAR	1
#define	SZINT	2
#define	SZPTR	2
#define	SZFLOAT	4
#define	SZLONG	4
#define	SZDOUB	8

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
#define	EOFC	0
#define	NULLOP	218
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
#define	CAST	11
#define	ETYPE	12

#define	KEYW	19
#define	NAME	20
#define	CON	21
#define	STRING	22
#define	FCON	23
#define	SFCON	24
#define	LCON	25
#define	SLCON	26

#define	SIZEOF	91
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
#define	ASSAND	77
#define	ASOR	78
#define	ASXOR	79
#define	ASSIGN	80

#define	QUEST	90
#define	MAX	93
#define	MAXP	94
#define	MIN	95
#define	MINP	96
#define	SEQNC	97
#define	CALL	100
#define	MCALL	101
#define	JUMP	102
#define	CBRANCH	103
#define	INIT	104
#define	SETREG	105
#define	RFORCE	110
#define	BRANCH	111
#define	LABEL	112
#define	NLABEL	113
#define	RLABEL	114
#define	STRASG	115
#define	ITOC	109
#define	SEOF	200	/* stack EOF marker in expr compilation */

/*
  types
*/
#define	INT	0
#define	CHAR	1
#define	FLOAT	2
#define	DOUBLE	3
#define	STRUCT	4
#define	LONG	6
#define	UNSIGN	7
#define	UNION	8		/* adjusted later to struct */

#define	ALIGN	01
#define	TYPE	07
#define	BIGTYPE	060000
#define	TYLEN	2
#define	XTYPE	(03<<3)
#define	PTR	010
#define	FUNC	020
#define	ARRAY	030

/*
  storage classes
*/
#define	KEYWC	1
#define	DEFXTRN	20
#define	TYPEDEF	9
#define	MOS	10
#define	AUTO	11
#define	EXTERN	12
#define	STATIC	13
#define	REG	14
#define	STRTAG	15
#define ARG	16
#define	ARG1	17
#define	AREG	18
#define	MOU	21
#define	ENUMTAG	22
#define	ENUMCON	24

/*
  keywords
*/
#define	GOTO	20
#define	RETURN	21
#define	IF	22
#define	WHILE	23
#define	ELSE	24
#define	SWITCH	25
#define	CASE	26
#define	BREAK	27
#define	CONTIN	28
#define	DO	29
#define	DEFAULT	30
#define	FOR	31
#define	ENUM	32

/*
  characters
*/
#define	BSLASH	117
#define	SHARP	118
#define	INSERT	119
#define	PERIOD	120
#define	SQUOTE	121
#define	DQUOTE	122
#define	LETTER	123
#define	DIGIT	124
#define	NEWLN	125
#define	SPACE	126
#define	UNKN	127

/*
 * Special operators in intermediate code
 */
#define	BDATA	200
#define	WDATA	201
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
#define	SETSTK	219
#define	SINIT	220

/*
  Flag bits
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

/*
 * Conversion codes
 */
#define	ITF	1
#define	ITL	2
#define	LTF	3
#define	ITP	4
#define	PTI	5
#define	FTI	6
#define	LTI	7
#define	FTL	8
#define	LTP	9
#define	ITC	10
#define	XX	15

/*
 * symbol table flags
 */

#define	FMOS	01
#define	FKEYW	04
#define	FFIELD	020
#define	FINIT	040
#define	FLABL	0100

