
#
/*
 *
 *	UNIX debugger - common definitions
 *
 */



/*	Layout of a.out file (fsym):
 *
 *	header of 8 words	magic number 405, 407, 410, 411
 *				text size	)
 *				data size	) in bytes but even
 *				bss size	)
 *				symbol table size
 *				entry point
 *				{unused}
 *				flag set if no relocation
 *
 *
 *	header:		0
 *	text:		16
 *	data:		16+textsize
 *	relocation:	16+textsize+datasize
 *	symbol table:	16+2*(textsize+datasize) or 16+textsize+datasize
 *
 */


#include <sys/param.h>
#include <sys/dir.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <sgtty.h>
#include "mac.h"
#include "mode.h"
#include <setjmp.h>


#define VARB	11
#define VARD	13
#define VARE	14
#define VARM	22
#define VARS	28
#define VART	29

#define COREMAGIC 0140000

#define RD	0
#define WT	1
#define NSP	0
#define	ISP	1
#define	DSP	2
#define STAR	4
#define STARCOM 0200
#define DSYM	7
#define ISYM	2
#define ASYM	1
#define NSYM	0
#define ESYM	(-1)
#define BKPTSET	1
#define BKPTEXEC 2
#define	SYMSIZ	100
#define MAXSIG	20

#define USERPS	2*(512-1)
#define USERPC	2*(512-2)
#define BPT	03
#define FD	0200
#define	SETTRC	0
#define	RDUSER	2
#define	RIUSER	1
#define	WDUSER	5
#define WIUSER	4
#define	RUREGS	3
#define	WUREGS	6
#define	CONTIN	7
#define	SINGLE	9
#define	EXIT	8

#define FROFF	((int16_t)(&((struct adb_fpsave *)0)->fpsr))
#define FRLEN	25
#define FRMAX	6

#define	ps	-1
#define	pc	-2
#define	sp	-6
#define	r5	-9
#define	r4	-10
#define	r3	-11
#define	r2	-12
#define	r1	-5
#define	r0	-3

#define MAXOFF	255
#define MAXPOS	80
#define MAXLIN	128
#define EOF	0
#define EOR	'\n'
#define TB	'\t'
#define QUOTE	0200
#define STRIP	0177
#define LOBYTE	0377
#define EVEN	-2


/* long to ints and back (puns) */
union {
	INT	I[2];
	L_INT	L;
} itolws;

#define leng(a)		((int32_t)((uint16_t)(a)))
#define shorten(a)	((int16_t)(a))
#define itol(a,b)	(itolws.I[0]=(a), itolws.I[1]=(b), itolws.L)



/* result type declarations */
extern BKPTR scanbkpt(int16_t adr);
extern L_INT inkdot(int16_t incr);
extern L_INT round(L_INT a, L_INT b);
extern POS chkget(L_INT n, int16_t space);
extern POS findsym(POS svalue, INT type);
extern POS get(L_INT adr, int16_t space);
extern STRING exform(INT fcount, STRING ifp, int16_t itype, int16_t ptype);
extern SYMPTR lookupsym(STRING symstr);
extern SYMPTR symget(void);
extern int16_t adaccess(int16_t mode, L_INT adr, int16_t space, int16_t value);
extern int16_t bpwait(void);
extern int16_t branch(STRING s, INT ins);
extern int16_t charpos(void);
extern int16_t chkerr(void);
extern int16_t chkloc(L_INT frame);
extern int16_t chkmap(L_INT *adr, INT space);
extern int16_t command(STRING buf, CHAR defcom);
extern int16_t convdig(CHAR c);
extern int16_t convert(STRING *cp);
extern int16_t digit(char c);
extern int16_t letter(char c);
extern int16_t create(STRING f);
extern int16_t delbp(void);
extern int16_t doexec(void);
extern int16_t done(void);
extern int16_t doubl(int16_t a, int16_t b);
extern int16_t endline(void);
extern int16_t endpcs(void);
extern int16_t eol(CHAR c);
extern int16_t eqstr(STRING s1, STRING s2);
extern int16_t eqsym(STRING s1, STRING s2, CHAR c);
extern int16_t error(STRING n);
extern int16_t execbkpt(BKPTR bkptr);
extern int16_t expr(int16_t a);
extern int16_t fault(int16_t a);
extern int16_t findroutine(L_INT cframe);
extern int16_t flushbuf(void);
extern int16_t getfile(STRING filnam, int16_t cnt);
extern int16_t getformat(STRING deformat);
extern int16_t getreg(int16_t regnam);
extern int16_t getsig(int16_t sig);
extern int16_t hexdigit(CHAR c);
extern int16_t iclose(void);
extern int16_t item(int16_t a);
extern int16_t length(STRING s);
extern int16_t localsym(L_INT cframe);
extern int16_t longseek(int16_t f, L_INT a);
extern int16_t main(INT argc, STRING *argv);
extern int16_t newline(void);
extern int16_t nextchar(void);
extern int16_t nextsym(void);
extern int16_t oclose(void);
extern int16_t printdate(L_INT tvec);
extern int16_t printdbl(INT lx, INT ly, char fmat, int16_t base);
extern int16_t printesc(int16_t c);
extern int16_t aprintf();	/* V7 pointer-to-args varargs (no va_list on pcc pdp11) */
extern int16_t printfregs(int16_t longpr);
extern int16_t printins(int16_t f, int16_t idsp, INT ins);
extern int16_t printmap(STRING s, MAP *amap);
extern int16_t printnum(INT n, int16_t fmat, int16_t base);
extern int16_t printoct(L_INT o, INT s);
extern int16_t printpc(void);
extern int16_t printregs(void);
extern int16_t prints(char *s);
extern int16_t psymoff(L_INT v, int16_t type, char *s);
extern int16_t put(L_INT adr, int16_t space, int16_t value);
extern int16_t quotchar(void);
extern int16_t rdc(void);
extern int16_t readchar(void);
extern int16_t readregs(void);
extern int16_t readsym(void);
extern int16_t runpcs(int16_t runmode, int16_t execsig);
extern int16_t scanform(L_INT icount, STRING ifp, int16_t itype, int16_t ptype);
extern int16_t setbp(void);
extern int16_t setcor(void);
extern int16_t setsym(void);
extern int16_t setup(void);
extern int16_t sigprint(void);
extern int16_t symchar(int16_t dig);
extern int16_t symread(void);
extern int16_t symset(void);
extern int16_t term(int16_t a);
extern int16_t unox(void);
extern int16_t valpr(int16_t v, int16_t idsp);
extern int16_t varchk(int16_t name);
extern int16_t within(L_INT adr, L_INT lbd, L_INT ubd);
extern void paddr(STRING s, INT a);
extern void printc(CHAR c);
extern void printtrace(int16_t modif);
extern void subpcs(int16_t modif);

/* cross-file globals (V7 common symbols) */
extern STRING symfil;
extern STRING corfil;

/* V7 libc/syscalls with no header at all (implicit-int in K&R).
   setjmp/longjmp, gtty/stty, ctime now live in c99/usr/include/{setjmp,sgtty,time}.h. */
extern char *sbrk(int16_t);
extern int16_t ptrace(int16_t, int16_t, int16_t, int16_t);
extern int16_t wait(int16_t *);
extern char *ecvt(double, int16_t, int16_t *, int16_t *);

typedef struct sgttyb TTY;
TTY	adbtty, usrtty;
jmp_buf erradb;
