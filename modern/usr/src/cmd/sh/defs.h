# /*
#  *	UNIX shell — S. R. Bourne
#  *
#  *	defs.h was a thicket of #define in V7; here the constants are enums and
#  *	the value/statement macros are functions.  What remains as #define are
#  *	the pointer-tagging casts (BLK/BYT/STK/ADR) and the lvalue aliases
#  *	(input/eof) — none of which has a function or enum form.  The V7
#  *	declarations had no `extern' (they relied on common symbols); here they
#  *	are extern so every .o does not carry its own copy.
#  */

/* feature-test: expose S_IFMT/S_IFDIR, sbrk(), etc. under -std=c99 */
#define _DEFAULT_SOURCE 1

#include <unistd.h>	/* close(), isatty() etc. used by the inline helpers below */
#include <sys/wait.h>	/* wait() */
#include <errno.h>	/* errno, ENOEXEC/ENOMEM/E2BIG/ETXTBSY/ENOENT */

/* error exits from various parts of shell */
enum { ERROR = 1, SYNBAD = 2, SIGFAIL = 3, SIGFLG = 0200 };

/* command tree */
enum {
	FPRS = 020, FINT = 040, FAMP = 0100, FPIN = 0400, FPOU = 01000,
	FPCL = 02000, FCMD = 04000, COMMSK = 017,

	TCOM = 0, TPAR = 1, TFIL = 2, TLST = 3, TIF = 4, TWH = 5, TUN = 6,
	TSW = 7, TAND = 8, TORF = 9, TFORK = 10, TFOR = 11,
};

/* execute table */
enum {
	SYSSET = 1, SYSCD = 2, SYSEXEC = 3, SYSLOGIN = 4, SYSTRAP = 5,
	SYSEXIT = 6, SYSSHFT = 7, SYSWAIT = 8, SYSCONT = 9, SYSBREAK = 10,
	SYSEVAL = 11, SYSDOT = 12, SYSRDONLY = 13, SYSTIMES = 14,
	SYSXPORT = 15, SYSNULL = 16, SYSREAD = 17, SYSTST = 18, SYSUMASK = 19,
};

/* used for input and output of shell */
enum { INIO = 10, OTIO = 11 };

/* io nodes */
enum {
	USERIO = 10, IOUFD = 15, IODOC = 16, IOPUT = 32, IOAPP = 64,
	IOMOV = 128, IORDW = 256, INPIPE = 0, OTPIPE = 1,
};

/* arg list terminator */
enum { ENDARGS = 0 };

#include	"mac.h"
#include	"mode.h"
#include	"name.h"

/* result type declarations */
void		*alloc_sh();
VOID		addblok();
STRING		make();
STRING		movstr();
TREPTR		cmd();
TREPTR		makefork();
NAMPTR		lookup();
VOID		setname();
VOID		setargs();
DOLPTR		useargs();
REAL		expr();
STRING		catpath();
STRING		getpath();
STRING		*scan();
STRING		mactrim();
STRING		macro();
VOID		await();
VOID		post();
VOID		exname();
VOID		printnam();
VOID		printflg();
VOID		prs();
VOID		prc(CHAR);
VOID		getenv_sh();	/* sh's own (void) — renamed: libc getenv() differs */
STRING		*setenv_sh();	/* sh's own (char **) — renamed: libc setenv() differs */
INT		cf();
INT		any(CHAR, STRING);
INT		length();
VOID		free_sh();	/* sh's own arena free — v7 `free' (renamed to avoid libc) */
VOID		rename_sh();	/* sh's fd-rename (dup2+close) — v7 `rename' (renamed to avoid libc) */

/* ===== cross-file functions the K&R original called undeclared ===== */
/* error.c */
VOID		sigchk();
VOID		error() __attribute__((noreturn));
VOID		failed() __attribute__((noreturn));
VOID		exitset();
VOID		exitsh() __attribute__((noreturn));
VOID		clearup();
VOID		done() __attribute__((noreturn));
VOID		rmtemp();
/* print.c */
VOID		prp();
VOID		prn();
VOID		blank();
VOID		newline();
VOID		itos();
VOID		prt();
/* word.c */
INT		nextc(CHAR);
INT		readc();
INT		word();
/* fault.c */
VOID		stdsigs();
INT		ignsig();
VOID		clrsig();
VOID		oldsigs();
VOID		getsig();
VOID		chktrap();
/* service.c */
VOID		initio();
INT		pathopen();
VOID		execa();
VOID		postclr();
VOID		trim();
INT		getarg();
VOID		subst();
INT		stoi();
/* io.c */
VOID		initf();
INT		estabf();
VOID		push();
INT		pop();
VOID		chkpipe();
INT		chkopen();
INT		create();
INT		tmpfil();
VOID		copy();
/* main.c */
VOID		chkpr(CHAR);
VOID		settmp();
/* name.c */
VOID		setlist();
INT		syslook();
VOID		assign();
VOID		replace();
VOID		dfault();
INT		readvar();
VOID		assnum();
VOID		namscan();
/* args.c */
INT		options();
DOLPTR		freeargs();
/* xec.c */
INT		execute();
VOID		execexp();
/* builtin.c */
INT		builtin();
/* setbrk.c */
BYTPTR		setbrk();
/* stak.c */
VOID		tdystak();
VOID		stakchk();
/* expand.c */
INT		expand();
INT		gmatch();
VOID		makearg();

/* attrib() set a name flag in v7:  n->namflg |= f */
static inline void attrib(NAMPTR n, int f) { n->namflg |= f; }

/* closepipe() closed both ends of a pipe in v7 */
static inline void closepipe(int *x) { close(x[INPIPE]); close(x[OTPIPE]); }

/* eq() was cf(a,b)==0 in v7 */
static inline int eq(const char *a, const char *b) { return cf(a, b) == 0; }

/* roundup() rounded a byte address up to a multiple of b in v7's `round'
 * macro; the result is a uintptr_t so it survives on a 64-bit host */
static inline uintptr_t roundup(uintptr_t a, uintptr_t b)
{
	return (a + b - 1) & ~(b - 1);
}

/* assert() was a no-op in v7 */
#define assert(x)	((void)0)

/* temp files and io */
extern UFD	output;
extern INT	ioset;
extern IOPTR	iotemp;		/* files to be deleted sometime */
extern IOPTR	iopend;		/* documents waiting to be read at NL */

/* substitution */
extern INT	dolc;
extern STRING	*dolv;
extern DOLPTR	argfor;
extern ARGPTR	gchain;

/* stack — pointer-tagging casts (no function/enum form) */
#define	BLK(x)	((BLKPTR)(x))
#define	BYT(x)	((BYTPTR)(x))
#define	STK(x)	((STKPTR)(x))
#define	ADR(x)	((char *)(x))

/* stak stuff */
#include	"stak.h"

/* string constants */
extern MSG	atline;
extern MSG	readmsg;
extern MSG	colon;
extern MSG	minus;
extern MSG	nullstr;
extern MSG	sptbnl;
extern MSG	unexpected;
extern MSG	endoffile;
extern MSG	synmsg;

/* name tree and words */
extern SYSTAB	reserved;
extern SYSTAB	commands;
extern STRING	sysmsg[];
extern INT	wdval;
extern INT	wdnum;
extern ARGPTR	wdarg;
extern INT	wdset;
extern BOOL	reserv;

/* prompting */
extern MSG	stdprompt;
extern MSG	supprompt;
extern MSG	profile;

/* built in names */
extern NAMNOD	fngnod;
extern NAMNOD	ifsnod;
extern NAMNOD	homenod;
extern NAMNOD	mailnod;
extern NAMNOD	pathnod;
extern NAMNOD	ps1nod;
extern NAMNOD	ps2nod;

/* special names */
extern MSG	flagadr;
extern STRING	cmdadr;
extern STRING	exitadr;
extern STRING	dolladr;
extern STRING	pcsadr;
extern STRING	pidadr;

extern MSG	defpath;

/* names always present */
extern MSG	mailname;
extern MSG	homename;
extern MSG	pathname;
extern MSG	fngname;
extern MSG	ifsname;
extern MSG	ps1name;
extern MSG	ps2name;

/* transput */
extern BOOL	nosubst;	/* set by trim(), read by copy() */
extern CHAR	tmpout[];
extern STRING	tmpnam;
extern INT	serial;
enum { TMPNAM = 7 };
extern FILE	standin;
#define input	(standin->fdes)
#define eof	(standin->feof)
extern INT	peekc;
extern STRING	comdiv;
extern MSG	devnull;

/* flags */
enum {
	noexec = 01, intflg = 02, prompt = 04, setflg = 010, errflg = 020,
	ttyflg = 040, forked = 0100, oneflg = 0200, rshflg = 0400,
	waiting = 01000, stdflg = 02000, execpr = 04000, readpr = 010000,
	keyflg = 020000,
};
extern INT	flags;

/* error exits from various parts of shell */
#include	<setjmp.h>
extern jmp_buf	subshell;
extern jmp_buf	errshell;

/* fault handling */
#include	"brkincr.h"
extern POS	brkincr;

enum {
	MINTRAP = 0, MAXTRAP = 17,
	INTR = 2, QUIT = 3, MEMF = 11, ALARM = 14, KILL = 15,
	TRAPSET = 2, SIGSET = 4, SIGMOD = 8,
};

extern VOID	fault();
extern BOOL	trapnote;
extern STRING	trapcom[];
extern BOOL	trapflg[];

/* name tree and words */
extern STRING	*environ;
extern CHAR	numbuf[];
extern MSG	export;
extern MSG	readonly;

/* execflgs */
extern INT	exitval;
extern BOOL	execbrk;
extern INT	loopcnt;
extern INT	breakcnt;

/* messages */
extern MSG	mailmsg;
extern MSG	coredump;
extern MSG	badopt;
extern MSG	badparam;
extern MSG	badsub;
extern MSG	nospace;
extern MSG	notfound;
extern MSG	badfile;
extern MSG	badtrap;
extern MSG	baddir;
extern MSG	badshift;
extern MSG	illegal;
extern MSG	restricted;
extern MSG	execpmsg;
extern MSG	notid;
extern MSG	wtfailed;
extern MSG	badcreate;
extern MSG	piperr;
extern MSG	badopen;
extern MSG	badnum;
extern MSG	arglist;
extern MSG	txtbsy;
extern MSG	toobig;
extern MSG	badexec;

extern char	*end;
extern BLKPTR	bloktop;

#include	"ctype.h"
