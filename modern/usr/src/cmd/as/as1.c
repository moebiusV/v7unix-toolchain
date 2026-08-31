/*
 * as1.c -- PDP-11 assembler pass 1, translated from as11.s..as19.s.
 *
 * Faithful translation of V7 /usr/src/cmd/as.  The .s sources are the
 * conformance oracle; this file reproduces their data layout, control flow,
 * and on-disk byte format so pass 1 and pass 2 interlock through the same
 * intermediate files.
 *
 * Translation conventions (carried over from the assembly):
 *   - a "word" is 16 bits; the output buffer and symbol-table words are
 *     uint16_t so the temp files are byte-identical (host is little-endian,
 *     matching the PDP-11).
 *   - `sys name; a; b` == name(a, b), a in r0, error bit via `jes`.
 *   - `jsr pc, sub`  == sub()                    (ordinary call)
 *   - `jsr r5, sub; a; b` == sub(a, b)          (inline-argument call)
 *   - `jmp X` / `br X` / `beq X` == goto X / if(..) goto X
 *   - `jmp *table(rN)`  == switch (rN)          (the 4 dispatch tables)
 *   - `rts pc` / `rts r5` == return
 *   - `tst (sp)+; rts pc` and `add $2,(sp); rts pc` are the skip-return
 *     idiom: the function returns to the caller's *next* instruction.  The C
 *     models it as a returned flag the caller branches on.
 *   - registers r0..r5 are kept as globals only where the original's flow
 *     reads naturally that way (the expression evaluator); elsewhere the C
 *     uses named locals + pointers.  Each function is tagged `asNN.s:LINE`.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Where pass 2 lives.  The original hardcodes `/lib/as2` (as11.s go:); a
 * host cross-assembler cannot, so the compiled-in default is the install
 * location (V7_LIBEXECDIR) and as2locate() prefers a sibling of the running
 * `as` (build tree) or $AS2. */
#ifndef V7_LIBEXECDIR
#define V7_LIBEXECDIR "/usr/local/libexec/v7unix"
#endif
#ifndef AS2_PATH
#define AS2_PATH V7_LIBEXECDIR "/as2"
#endif

/* the original's working registers (expression evaluator, token scanner).
 * r4 holds either a small token value or a symbol-entry pointer, so it is
 * intptr_t (the original's 16-bit address fits either). */
int r0, r1, r2, r3, r5;
intptr_t r4;

/* ------------------------------------------------------------- globals
 * Layout order follows as18.s (.data then .bss).  Byte-addressed things in
 * the original become C pointers / byte arrays; 16-bit words become uint16_t.
 */

/* character class table: classifies each input byte (as18.s) */
static const signed char chartab[128] = {
	(int8_t)-014,(int8_t)-014,(int8_t)-014,(int8_t)-014,(int8_t)-02,(int8_t)-014,(int8_t)-014,(int8_t)-014,
	(int8_t)-014,(int8_t)-022, (int8_t)-02,(int8_t)-014,(int8_t)-014,(int8_t)-022,(int8_t)-014,(int8_t)-014,
	(int8_t)-014,(int8_t)-014,(int8_t)-014,(int8_t)-014,(int8_t)-014,(int8_t)-014,(int8_t)-014,(int8_t)-014,
	(int8_t)-014,(int8_t)-014,(int8_t)-014,(int8_t)-014,(int8_t)-014,(int8_t)-014,(int8_t)-014,(int8_t)-014,
	(int8_t)-022,(int8_t)-020,(int8_t)-016,(int8_t)-014,(int8_t)-020,(int8_t)-020,(int8_t)-020,(int8_t)-012,
	(int8_t)-020,(int8_t)-020,(int8_t)-020,(int8_t)-020,(int8_t)-020,(int8_t)-020,(int8_t)056,(int8_t)-06,
	(int8_t)060,(int8_t)061,(int8_t)062,(int8_t)063,(int8_t)064,(int8_t)065,(int8_t)066,(int8_t)067,
	(int8_t)070,(int8_t)071,(int8_t)-020,(int8_t)-02,(int8_t)-00,(int8_t)-020,(int8_t)-014,(int8_t)-014,
	(int8_t)-014,(int8_t)0101,(int8_t)0102,(int8_t)0103,(int8_t)0104,(int8_t)0105,(int8_t)0106,(int8_t)0107,
	(int8_t)0110,(int8_t)0111,(int8_t)0112,(int8_t)0113,(int8_t)0114,(int8_t)0115,(int8_t)0116,(int8_t)0117,
	(int8_t)0120,(int8_t)0121,(int8_t)0122,(int8_t)0123,(int8_t)0124,(int8_t)0125,(int8_t)0126,(int8_t)0127,
	(int8_t)0130,(int8_t)0131,(int8_t)0132,(int8_t)-020,(int8_t)-024,(int8_t)-020,(int8_t)-020,(int8_t)0137,
	(int8_t)-014,(int8_t)0141,(int8_t)0142,(int8_t)0143,(int8_t)0144,(int8_t)0145,(int8_t)0146,(int8_t)0147,
	(int8_t)0150,(int8_t)0151,(int8_t)0152,(int8_t)0153,(int8_t)0154,(int8_t)0155,(int8_t)0156,(int8_t)0157,
	(int8_t)0160,(int8_t)0161,(int8_t)0162,(int8_t)0163,(int8_t)0164,(int8_t)0165,(int8_t)0166,(int8_t)0167,
	(int8_t)0170,(int8_t)0171,(int8_t)0172,(int8_t)-014,(int8_t)-026,(int8_t)-014,(int8_t)0176,(int8_t)-014,
};

/* a symbol-table entry: 8-byte name (4 words), 2-byte type, 2-byte value.
 * sizeof == 12, matching the original's `add $12.,r1` stride exactly. */
struct sym {
	unsigned char name[8];
	uint16_t       type;
	uint16_t       value;
};

/*
 * Builtin symbol table, transcribed byte-for-byte from as19.s.  type/value
 * fields are the original's octal literals (C 0-prefixed octal is the same
 * value).  The first two entries carry the location counter, so the code
 * reads it as symtab[0].value (dot) and symtab[0].type (dotrel).
 */
static struct sym symtab[] = {
	/* special variables */
	{ ".",        002, 000000 },   /* dotrel:02, dot:000000 */
	{ "..",       001, 000000 },   /* 01, dotdot:000000 */
	/* registers */
	{ "r0",       024, 000000 },
	{ "r1",       024, 000001 },
	{ "r2",       024, 000002 },
	{ "r3",       024, 000003 },
	{ "r4",       024, 000004 },
	{ "r5",       024, 000005 },
	{ "sp",       024, 000006 },
	{ "pc",       024, 000007 },
	/* double operand */
	{ "mov",      013, 0010000 },
	{ "movb",     013, 0110000 },
	{ "cmp",      013, 0020000 },
	{ "cmpb",     013, 0120000 },
	{ "bit",      013, 0030000 },
	{ "bitb",     013, 0130000 },
	{ "bic",      013, 0040000 },
	{ "bicb",     013, 0140000 },
	{ "bis",      013, 0050000 },
	{ "bisb",     013, 0150000 },
	{ "add",      013, 0060000 },
	{ "sub",      013, 0160000 },
	/* branch */
	{ "br",       006, 0000400 },
	{ "bne",      006, 0001000 },
	{ "beq",      006, 0001400 },
	{ "bge",      006, 0002000 },
	{ "blt",      006, 0002400 },
	{ "bgt",      006, 0003000 },
	{ "ble",      006, 0003400 },
	{ "bpl",      006, 0100000 },
	{ "bmi",      006, 0100400 },
	{ "bhi",      006, 0101000 },
	{ "blos",     006, 0101400 },
	{ "bvc",      006, 0102000 },
	{ "bvs",      006, 0102400 },
	{ "bhis",     006, 0103000 },
	{ "bec",      006, 0103000 },
	{ "bcc",      006, 0103000 },
	{ "blo",      006, 0103400 },
	{ "bcs",      006, 0103400 },
	{ "bes",      006, 0103400 },
	/* jump/branch type */
	{ "jbr",      035, 0000400 },
	{ "jne",      036, 0001000 },
	{ "jeq",      036, 0001400 },
	{ "jge",      036, 0002000 },
	{ "jlt",      036, 0002400 },
	{ "jgt",      036, 0003000 },
	{ "jle",      036, 0003400 },
	{ "jpl",      036, 0100000 },
	{ "jmi",      036, 0100400 },
	{ "jhi",      036, 0101000 },
	{ "jlos",     036, 0101400 },
	{ "jvc",      036, 0102000 },
	{ "jvs",      036, 0102400 },
	{ "jhis",     036, 0103000 },
	{ "jec",      036, 0103000 },
	{ "jcc",      036, 0103000 },
	{ "jlo",      036, 0103400 },
	{ "jcs",      036, 0103400 },
	{ "jes",      036, 0103400 },
	/* single operand */
	{ "clr",      015, 0005000 },
	{ "clrb",     015, 0105000 },
	{ "com",      015, 0005100 },
	{ "comb",     015, 0105100 },
	{ "inc",      015, 0005200 },
	{ "incb",     015, 0105200 },
	{ "dec",      015, 0005300 },
	{ "decb",     015, 0105300 },
	{ "neg",      015, 0005400 },
	{ "negb",     015, 0105400 },
	{ "adc",      015, 0005500 },
	{ "adcb",     015, 0105500 },
	{ "sbc",      015, 0005600 },
	{ "sbcb",     015, 0105600 },
	{ "tst",      015, 0005700 },
	{ "tstb",     015, 0105700 },
	{ "ror",      015, 0006000 },
	{ "rorb",     015, 0106000 },
	{ "rol",      015, 0006100 },
	{ "rolb",     015, 0106100 },
	{ "asr",      015, 0006200 },
	{ "asrb",     015, 0106200 },
	{ "asl",      015, 0006300 },
	{ "aslb",     015, 0106300 },
	{ "jmp",      015, 0000100 },
	{ "swab",     015, 0000300 },
	/* jsr / rts / sys */
	{ "jsr",      007, 0004000 },
	{ "rts",      010, 000200 },
	{ "sys",      011, 0104400 },
	/* flag-setting */
	{ "clc",      001, 0000241 },
	{ "clv",      001, 0000242 },
	{ "clz",      001, 0000244 },
	{ "cln",      001, 0000250 },
	{ "sec",      001, 0000261 },
	{ "sev",      001, 0000262 },
	{ "sez",      001, 0000264 },
	{ "sen",      001, 0000270 },
	/* floating point ops */
	{ "cfcc",     001, 0170000 },
	{ "setf",     001, 0170001 },
	{ "setd",     001, 0170011 },
	{ "seti",     001, 0170002 },
	{ "setl",     001, 0170012 },
	{ "clrf",     015, 0170400 },
	{ "negf",     015, 0170700 },
	{ "absf",     015, 0170600 },
	{ "tstf",     015, 0170500 },
	{ "movf",     012, 0172400 },
	{ "movif",    014, 0177000 },
	{ "movfi",    005, 0175400 },
	{ "movof",    014, 0177400 },
	{ "movfo",    005, 0176000 },
	{ "addf",     014, 0172000 },
	{ "subf",     014, 0173000 },
	{ "mulf",     014, 0171000 },
	{ "divf",     014, 0174400 },
	{ "cmpf",     014, 0173400 },
	{ "modf",     014, 0171400 },
	{ "movie",    014, 0176400 },
	{ "movei",    005, 0175000 },
	{ "ldfps",    015, 0170100 },
	{ "stfps",    015, 0170200 },
	{ "fr0",      024, 000000 },
	{ "fr1",      024, 000001 },
	{ "fr2",      024, 000002 },
	{ "fr3",      024, 000003 },
	{ "fr4",      024, 000004 },
	{ "fr5",      024, 000005 },
	/* 11/45 operations */
	{ "als",      030, 072000 },
	{ "alsc",     030, 073000 },
	{ "mpy",      030, 070000 },
	{ "dvd",      030, 071000 },
	{ "xor",      007, 074000 },
	{ "sxt",      015, 006700 },
	{ "mark",     011, 006400 },
	{ "sob",      031, 077000 },
	/* specials */
	{ ".byte",    016, 000000 },
	{ ".even",    020, 000000 },
	{ ".if",      021, 000000 },
	{ ".endif",   022, 000000 },
	{ ".globl",   023, 000000 },
	{ ".text",    025, 000000 },
	{ ".data",    026, 000000 },
	{ ".bss",     027, 000000 },
	{ ".comm",    032, 000000 },
	/* EIS aliases (`.if eae-1`, eae = 0): mul/div/ash/ashc == mpy/dvd/als/alsc */
	{ "mul",      030, 070000 },
	{ "div",      030, 071000 },
	{ "ash",      030, 072000 },
	{ "ashc",     030, 073000 },
};
enum { NSYM = (int)(sizeof symtab / sizeof symtab[0]) };

/* the location counter and its relocation type live in symtab[0] */

/* .data section (as18.s).  `namedone` (as18.s:26) is declared in the source
 * but never referenced anywhere in pass 1, so it is omitted here. */
static char  a_tmp1[] = "/tmp/atm1a";
static char  a_tmp2[] = "/tmp/atm2a";
static char  a_tmp3[] = "/tmp/atm3a";
static int   curfb[10] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
static uint16_t *obufp;                /* points into outbuf, a word at a time */

/* .bss section (as18.s), in layout order */
char   curfbr[10];                     /* forward-ref types, 10 bytes */
int    savdot[3];                      /* save area for .text/.data/.bss dot */
int    bufcnt;
enum { HSHSIZ = 3001 };
unsigned char *hshtab[HSHSIZ];         /* hash buckets: pointer to a sym entry */
char   pof;                            /* output file descriptor */
char   wordf;
char   fin;                            /* input file descriptor */
char   fbfil;                          /* forward-ref file descriptor */
char   fileflg;
char   errflg;
char   ch;                             /* one-char pushback */
unsigned char symbol[8];               /* current name (8 bytes) */
int    obufc;
uint16_t outbuf[256];                  /* 512 bytes */
int    line;
int    inbfcnt;
int    ifflg;
int    inbfp;
int    nargs;
char **curarg;                         /* points one slot before current file */
int    opfound;
intptr_t savop;                        /* token save: holds a symbol pointer */
int    numval;
unsigned char nxtfb[4];                /* forward-ref record: type, idx, value */
/* user symbol table: grows via brk() in the original; realloc here */
unsigned char *usymtab;
unsigned char *symend;                 /* current end of user symtab */

enum { SYMENT = 12 };                  /* bytes per symbol-table entry */

static void aexit(void);               /* as11.s:33 */

/* ------------------------------------------------------------- syscall shims
 * `sys name; a; b` in the original is `name(a, b)`, with a in r0 and the
 * error bit tested by `jes`.  Translated as ordinary C.
 */

/* wrterr -- as11.s:124.  Write the error message, then `jbr aexit`. */
static void wrterr(void)
{
	static const char msg[] = "as: Write error on temp file.\n";
	write(1, msg, sizeof(msg) - 1);
	errflg++;
	aexit();
}

/* filerr(name, arg) -- as11.s:64.  Writes the filename then the inline arg
 * (1 or 2 bytes depending on whether arg[1] is nonzero).  The original reads
 * `arg` off r5; translated as an explicit second parameter. */
static void filerr(const char *name, const char *arg)
{
	if (name)
		write(1, name, strlen(name));
	if (arg)
		write(1, arg, arg[1] ? 2 : 1);
}

/* fcreat(name) -- as11.s:96.  Open-or-create a temp file; on name clash bump
 * the 9th char (`incb 9.(r4)`), reproduced literally. */
static int fcreat(char *name)
{
	int fd;

	for (;;) {
		if ((fd = open(name, O_RDWR)) >= 0) {
			struct stat st;
			if (fstat(fd, &st) == 0 && st.st_size == 0)
				return fd;
			close(fd);
			unlink(name);
		}
		if ((fd = creat(name, 0444)) >= 0)
			return fd;
		name[9]++;
		if (name[9] > 'z') {
			filerr(name, "?\n");
			exit(3);
		}
	}
}

/* ------------------------------------------------------------- pass-1 helpers
 * as12.s: error(), betwen(), putw().
 */

/* error(code) -- as12.s:6.  If there is a current filename, print it once
 * (then clear it), followed by a newline; then print `<code> <line:4>\n`.
 * The original builds the line number into "<f xxxx\n>" via dvd/10. */
static void error(int code)
{
	errflg++;
	if (curarg && *curarg) {
		filerr(*curarg, "\n");
		*curarg = NULL;             /* print the filename only once */
	}
	{
		char msg[8];
		msg[0] = (char)code;          /* overwrites the 'f' placeholder */
		msg[1] = ' ';
		msg[6] = '\n';
		{
			int n = line;
			int i;
			for (i = 5; i >= 2; i--) {
				msg[i] = (char)('0' + n % 10);
				n /= 10;
			}
		}
		write(1, msg, 7);
	}
}

/* betwen(v, lo, hi) -- as12.s:45.  True iff lo <= v <= hi (inclusive).
 * Skip-return idiom: the caller's `br 1f` after the inline args is skipped
 * when this returns true. */
static int betwen(int v, int lo, int hi)
{
	return v >= lo && v <= hi;
}

/* putw(w) -- as12.s:55.  Buffer one word into outbuf, flush on full.  Inside
 * a false .if block (ifflg != 0) only the newline token is still emitted. */
static void putw(int w)
{
	if (ifflg && w != '\n')
		return;
	*obufp++ = (uint16_t)w;
	if (obufp == &outbuf[256]) {
		obufp = outbuf;
		if (write(pof, outbuf, sizeof outbuf) != sizeof outbuf)
			wrterr();
	}
}

/* ------------------------------------------------------------- input
 * rch() -- as14.s:174.  Return the next input character (byte), refilling the
 * 512-byte buffer from the current file, switching to the next argument file
 * on EOF, and honouring `if` nesting via the `fin`/`ifflg` machinery.
 */

static char inbuf[512];

static int rch(void)
{
	for (;;) {
		if (ch) {
			int c = ch;
			ch = 0;
			return c;
		}
		if (--inbfcnt >= 0) {
			int c = inbuf[inbfp++] & 0177;
			if (c == 0)
				continue;
			return c;
		}
		/* buffer empty: refill, or step to the next file */
		if (fin) {
			int n = read(fin, inbuf, 512);
			if (n > 0) {
				inbfcnt = n;
				inbfp = 0;
				continue;
			}
			close(fin);
			fin = 0;
		}
		/* no more input on this file: step to the next argument file */
		if (--nargs > 0) {
			char *name;
			int fd;
			if (ifflg) {
				error('i');
				aexit();            /* jmp aexit */
			}
			name = *++curarg;       /* `tst (r0)+; mov (r0),0f; mov r0,curarg` */
			fileflg++;
			fd = open(name, 0);
			if (fd < 0) {
				filerr(name, "?\n");
				aexit();
			}
			fin = (char)fd;
			line = 1;
			/* emit the filename record: header 5, chars, -1 terminator */
			putw(5);
			while (*name)
				putw(*name++);
			putw(-1);
			continue;
		}
		return '\004';              /* end of input (EOT, V7 as '\e) */
	}
}

/* ------------------------------------------------------------- symbol table
 * rname() -- as14.s:6.  Read a name into symbol[8], hash it, look it up (or
 * create a new user symbol), and emit the symbol code via putw.  Skip-return
 * idiom: the caller (readop) does not emit its trailing word for a symbol.
 */

/* swab16(x): the PDP-11 `swab` -- byte-swap a 16-bit word */
static int swab16(int x)
{
	x &= 0xFFFF;
	return (x >> 8) | ((x & 0xFF) << 8);
}

/* symhash(name): the hash used by setup() (as19.s:296): fold the name bytes
 * with `add r4,r3; swab r3`.  rname() folds chartab[] classes instead, but
 * chartab[c]==c for every char a symbol name may contain, so they agree. */
static int symhash(const unsigned char *name)
{
	int h = 0;
	while (*name)
		h = swab16(h + *name++);
	return h;
}

/* grow_usymtab(): the original extends the break by 512 bytes via
 * `add $512.,0f; sys break; 0:end` when the user symbol table is near full;
 * realloc is the malloc-world equivalent. */
static void grow_usymtab(void)
{
	size_t off = symend - usymtab;
	bufcnt += 512;
	usymtab = realloc(usymtab, bufcnt);
	if (!usymtab) {
		wrterr();
	}
	symend = usymtab + off;
}

static void rname(void)
{
	unsigned char *name = symbol;      /* r2 walks this as 4 words */
	int hash = 0;                      /* stack word at (sp) */
	int nothash = 0;                   /* stack word at 2(sp) */
	int cnt = 8;                       /* r5 */
	int i;

	/* clear the 8-byte name */
	for (i = 0; i < 8; i++)
		name[i] = 0;

	/* `cmp r0,$'~` -> symbol not for hash table */
	if (r0 == '~') {
		nothash = 1;
		ch = 0;
	}

	/* read name chars: hash all of them, store first 8 bytes */
	{
		int pos = 0;
		for (;;) {
			int c;
			r0 = rch();
			c = chartab[r0 & 0177];
			if (c <= 0)
				break;
			hash = swab16(hash + c);       /* add r3,(sp); swab (sp) */
			if (--cnt >= 0)
				name[pos++] = (unsigned char)c;
		}
	}
	ch = (char)r0;                     /* push back the delimiter */

	{
		unsigned char *entry = NULL;
		if (!nothash) {
			int idx = hash % HSHSIZ;
			int step = hash / HSHSIZ;
			int wrapped = 0;
			for (;;) {
				idx -= step;
				if (idx <= 0) {
					if (wrapped) {    /* symbol table overflow */
						write(1, "as: symbol table overflow\n", 26);
						aexit();
					}
					wrapped = 1;
					idx += HSHSIZ;
				}
				idx--;                /* `mov -(r1),r4` pre-decrement */
				entry = hshtab[idx];
				if (entry == NULL) {
					/* create: grow if needed, link bucket, copy name */
					if (symend + SYMENT + 16 > usymtab + bufcnt)
						grow_usymtab();
					hshtab[idx] = symend;
					memcpy(symend, symbol, 8);
					symend[8] = symend[9] = 0;
					entry = symend;
					symend += SYMENT;
					break;
				}
				if (memcmp(symbol, entry, 8) == 0)
					break;
			}
		} else {
			/* no hash: create a new symbol directly (`mov symend,r4; br 4f`) */
			if (symend + SYMENT + 16 > usymtab + bufcnt)
				grow_usymtab();
			entry = symend;
			memcpy(symend, symbol, 8);
			symend[8] = symend[9] = 0;
			symend += SYMENT;
		}

		/* emit the symbol code: base + (entry - table)/3, and leave
		 * r4 = &entry.type for the expression evaluator. */
		{
			int code;
			if (entry < usymtab)
				code = 0x200 + (entry - (unsigned char *)symtab) / 3;
			else
				code = 0x800 + (entry - usymtab) / 3;
			putw(code);
		}
		r4 = (intptr_t)(entry + 8);    /* mov (sp)+,r4 in the epilogue */
	}
}

/* fbcheck(v) -- as13.s:113.  Forward-reference index must be <= 9. */
static int fbcheck(int v)
{
	if (v > 9) {
		error('f');
		return 0;
	}
	return v;
}

/* number() -- as14.s:126.  Read a decimal/octal number; a trailing '.', or a
 * 'b'/'f' suffix marks float/binary forward refs.  Skip-return when 'b'/'f'.
 */
static int number(void)
{
	int dec = 0;                       /* r5: decimal accumulation */
	int oct = 0;                       /* r1: octal accumulation */

	for (;;) {
		r0 = rch();
		if (!betwen(r0, '0', '9'))
			break;
		dec = dec * 10 + (r0 - '0');
		oct = oct * 8 + (r0 - '0');
	}

	if (r0 == 'b' || r0 == 'f') {
		int suff = r0;                 /* r3 */
		int res = 0141 + fbcheck(dec) + (suff == 'f' ? 10 : 0);
		r4 = res;
		return 1;                      /* skip-return (b/f) */
	}

	if (r0 == '.') {                   /* trailing '.' -> decimal value */
		oct = dec;
		ch = 0;                        /* consume the '.' */
	} else {
		ch = (char)r0;                 /* push back the delimiter */
	}
	r0 = oct;
	return 0;                          /* normal return */
}

/* ------------------------------------------------------------- bootstrap
 * setup() -- as19.s:294.  Hash every builtin symtab entry into hshtab.
 */
static void setup(void)
{
	struct sym *s;
	for (s = symtab; (unsigned char *)s < (unsigned char *)(symtab + NSYM); s++) {
		int hash = symhash(s->name);
		int idx = hash % HSHSIZ;
		int step = hash / HSHSIZ;
		for (;;) {
			idx -= step;
			if (idx <= 0)
				idx += HSHSIZ;
			idx--;                     /* `tst -(r3)` */
			if (hshtab[idx] == NULL) {
				hshtab[idx] = (unsigned char *)s;
				break;
			}
		}
	}
}

/* ------------------------------------------------------------- forward decls
 * remaining pass-1 routines, translated in later files' commits.
 */
static void assem(void);               /* as13.s */
static void readop(void);              /* as15.s */
static void expres(void);              /* as17.s */
static void opline(void);              /* as16.s */

static void aexit(void)
{
	unlink(a_tmp1);
	unlink(a_tmp2);
	unlink(a_tmp3);
	exit(3);
}

/* as2locate() -- host adaptation of the `/lib/as2` exec in as11.s go:.
 * Resolution order:
 *   1. $AS2                  (explicit override)
 *   2. `as2` beside argv[0]  (build-tree run, or a co-located install)
 *   3. the compiled-in AS2_PATH (the install location; default `/lib/as2`)
 * Returns a pointer into a static buffer or a string literal. */
static const char *as2locate(const char *argv0)
{
	const char *e = getenv("AS2");
	if (e && e[0])
		return e;

	{
		static char buf[4096];
		const char *slash = strrchr(argv0, '/');
		if (slash != NULL) {
			size_t n = (size_t)(slash - argv0) + 1;  /* keep the '/' */
			if (n + 4 <= sizeof buf) {               /* "as2" + NUL */
				memcpy(buf, argv0, n);
				strcpy(buf + n, "as2");
				if (access(buf, X_OK) == 0)
					return buf;
			}
		}
	}
	return AS2_PATH;
}

/* start() -- as19.s:253 + as11.s go:.  Parse args, make temp files, run.
 * The pass-2 exec (as11.s go:) builds its own argv from globfl/outfl/outfp
 * and the three temp-file names. */
int main(int argc, char **argv)
{
	static char globfl[] = "-\0";      /* "-" with a spare byte for "-g" */
	static char *unglob = globfl + 1;  /* unglob[0]='g' turns globfl into "-g" */
	static char outfl[] = "-o";
	static const char *outfile = "a.out";
	const char *outfp = outfile;

	/* signal(2,1) / signal(2,aexit) -- ignore SIGINT, or install aexit */
	(void)signal(2, SIG_IGN);

	/* parse leading -u / -g / -o flags */
	{
		int n = argc - 1;
		char **ap = &argv[1];
		while (n > 0 && ap[0][0] == '-') {
			if (ap[0][1] == 'u' || ap[0][1] == '\0') {
				unglob[0] = 'g';
				ap++; n--;
			} else if (ap[0][1] == 'o') {
				ap++; n--;
				if (n > 0) {
					outfp = ap[0];
					ap++; n--;
				}
			} else {
				break;
			}
		}
		nargs = n + 1;                 /* original counts argv[0] too */
		curarg = ap - 1;               /* one slot before the first file */
	}

	/* init the user symbol area (36 bytes reserved in .bss) */
	usymtab = malloc(36);
	bufcnt = 36;
	symend = usymtab;

	pof = (char)fcreat(a_tmp1);
	fbfil = (char)fcreat(a_tmp2);
	obufp = outbuf;

	setup();

	/* go: -- as11.s:7 */
	assem();

	if (write(pof, outbuf, sizeof outbuf) != sizeof outbuf)
		wrterr();
	close(pof);
	close(fbfil);

	if (errflg)
		aexit();

	{
		int fd = fcreat(a_tmp3);
		if (write(fd, usymtab, symend - usymtab) != symend - usymtab)
			wrterr();
		close(fd);
	}

	/* exec as2 with the reconstructed argument vector */
	{
		const char *as2 = as2locate(argv[0]);
		char *pass2argv[8];
		int k = 0;
		pass2argv[k++] = (char *)as2;
		pass2argv[k++] = globfl;       /* "-" or "-g" */
		pass2argv[k++] = outfl;        /* "-o" */
		pass2argv[k++] = (char *)outfp;
		pass2argv[k++] = a_tmp1;
		pass2argv[k++] = a_tmp2;
		pass2argv[k++] = a_tmp3;
		pass2argv[k] = NULL;
		execv(as2, pass2argv);
		filerr(as2, "?\n");
		aexit();
	}
	return 0;
}

/* ------------------------------------------------------------- token scan
 * readop() and helpers -- as15.s.  The `jmp *1f-2(r1)` dispatch (chartab class
 * -> handler) becomes the switch in readop().  Handlers that end in
 * `tst (sp)+; rts pc` (skip-return) emit their own words and return directly;
 * the rest leave r4 for readop's trailing putw(r4).
 */

/* esctab -- as15.s:51: the `\<char>` operator escapes */
static const char esctab[] = { '/', '/', '<', 035, '>', 036, '%', 037, 0, 0 };

/* schar -- as15.s:161: `\x` escapes inside strings / quoted chars */
static const char schar[] = {
	'n', 012, 's', 040, 't', 011, 'e', 004,
	'0', 000, 'r', 015, 'a', 006, 'p', 033, 0, -1
};

/* rsch() -- as15.s:130.  Read a char honouring `\x` escapes; r0 = char,
 * r1 = 1 if the char was an un-escaped `>`. */
static void rsch(void)
{
	r0 = rch();
	if (r0 == '\004' || r0 == '\n') {      /* cmp $'\e / $'\n */
		error('<');
		aexit();
	}
	r1 = 0;
	if (r0 == '\\') {                      /* cmp r0,$'\ */
		int i;
		r0 = rch();
		for (i = 0; schar[i]; i += 2)
			if (r0 == schar[i]) {
				r0 = schar[i+1];
				break;
			}
		return;                            /* r1 stays 0 */
	}
	if (r0 == '>')                         /* cmp r0,$'> */
		r1 = 1;
}

/* escp() -- as15.s:38.  Map a `\<char>` operator escape. */
static void escp(void)
{
	int i;
	r0 = rch();
	for (i = 0; esctab[i]; i += 2)
		if (r0 == esctab[i]) {
			r4 = esctab[i+1];
			return;
		}
	/* no match: r4 keeps the '\' already in it */
}

/* rdnum() -- as15.s:72.  number(): decimal emits (1,val); b/f emits r4. */
static void rdnum(void)
{
	if (number()) {                        /* b/f: skip-return path */
		putw(r4);                          /* emit the forward-ref code */
		return;
	}
	numval = r0;                           /* normal number: emit (1,value) */
	putw(1);
	putw(numval);
	r4 = 1;                                /* evaluator sees the number marker */
}

/* rdname() -- as15.s:63.  A name char: digit routes to rdnum, else rname. */
static void rdname(void)
{
	ch = (char)r0;                         /* movb r0,ch */
	if (r1 >= '0' && r1 <= '9') {          /* cmp r1,$'0; ... $'9 */
		rdnum();
		return;
	}
	rname();                               /* jmp rname */
}

/* squote() / dquote() -- as15.s:77-85.  Quoted char(s); numval carries the
 * value for the evaluator, r4 ends 1. */
static void squote(void)
{
	rsch();
	numval = r0;
	putw(1);
	putw(numval);
	r4 = 1;
}
static void dquote(void)
{
	int c1, c2;
	rsch();
	c1 = r0;
	rsch();
	c2 = r0;
	numval = ((c2 << 8) | c1) & 0xFFFF;   /* swab r0; bis (sp)+,r0 */
	putw(1);
	putw(numval);
	r4 = 1;
}

/* string() -- as15.s:110.  `<...>` string; numval counts the chars. */
static void string(void)
{
	putw('<');
	numval = 0;
	for (;;) {
		rsch();
		if (r1)                            /* `>` terminates */
			break;
		putw(r0 | 0400);                   /* bis $400,r4 */
		numval++;
	}
	putw(-1);
	r4 = '<';
}

/* skip() -- as15.s:96.  Comment to end of line. */
static void skip(void)
{
	for (;;) {
		r0 = rch();
		r4 = r0;
		if (r0 == '\004' || r0 == '\n')
			break;
	}
}

/* readop() -- as15.s:6.  Scan one operand token and emit its encoding. */
static void readop(void)
{
	if (savop) {                           /* mov savop,r4; beq 1f; clr savop */
		r4 = savop;
		savop = 0;
		return;
	}
	for (;;) {
		r0 = rch();                        /* 8: jsr pc,rch */
		r4 = r0;                           /* mov r0,r4 */
		r1 = chartab[r0 & 0177];           /* movb chartab(r0),r1 */
		if (r1 > 0) {                      /* bgt rdname */
			rdname();
			return;
		}
		switch (r1) {                      /* jmp *1f-2(r1) */
		case 0:                            /* '<' string */
			string();
			return;
		case -2:                           /* ';' LF retread */
			putw(r4);
			return;
		case -4:                           /* rdnum (unreachable) */
			rdnum();
			return;
		case -6:                           /* '/' skip */
			skip();
			putw(r4);
			return;
		case -8:                           /* rdname (unreachable) */
			rdname();
			return;
		case -10:                          /* "'" squote */
			squote();
			return;
		case -12:                          /* garb: error, re-read */
			error('g');
			continue;
		case -14:                          /* '"' dquote */
			dquote();
			return;
		case -16:                          /* operators retread */
			putw(r4);
			return;
		case -18:                          /* whitespace 8b re-read */
			continue;
		case -20:                          /* '\' escp */
			escp();
			putw(r4);
			return;
		case -22:                          /* '|' fixor: emit 037 */
			putw(037);
			return;
		default:
			return;
		}
	}
}

/* ------------------------------------------------------------- expression
 * evaluator (as17.s).  Left-to-right, no operator precedence (a V7 as quirk):
 * the pending operator is applied as soon as the next operand arrives.  The
 * two dispatch tables -- esw1 (incoming token) and exsw2 (pending operator) --
 * become the two switches below.
 */

enum { ESTK = 64 };
static int estk[ESTK];                 /* the PDP-11 SP stack */
static int *sp = estk + ESTK;
static void push(int x) { *--sp = x; }
static int  pop(void)   { return *sp++; }

static void errore(void) { error('e'); }   /* as16.s:279 */

/* combin(arg) -- as17.s:195.  Merge r0 (operand class) and r3 (acc class)
 * into the accumulator's relocation class r3. */
static void combin(int arg)
{
	int relbit = (r0 | r3) & 040;      /* bis r3,(sp); bic $!40,(sp) */
	int a = r0 & 037;                  /* bic $!37,r0 */
	int b = r3 & 037;                  /* bic $!37,r3 */
	int cls;

	if (a > b) { int t = a; a = b; b = t; }   /* cmp r0,r3; ble 1f */
	if (a == 0)                        /* tst r0; beq 1f -> clr r3 */
		cls = 0;
	else if (arg && a == b)            /* tst (r5)+; cmp; bne 2f */
		cls = 1;                       /* mov $1,r3 */
	else
		cls = b;
	r3 = cls | relbit;                 /* bis (sp)+,r3 */
}

/* the ex* operators: apply r1 to the accumulator r2/r3, then eoprnd */
static void exadd(void)  { combin(0); r2 = (r2 + r1) & 0xFFFF; }
static void exsub(void)  { combin(1); r2 = (r2 - r1) & 0xFFFF; }
static void exmul(void)  { combin(0); r2 = (r1 * r2) & 0xFFFF; }
static void exdiv(void)  { combin(0); r2 = (int16_t)((int16_t)r2 / (int16_t)r1); }
static void exor(void)   { combin(0); r2 = (r2 | r1) & 0xFFFF; }
static void exand(void)  { combin(0); r2 = (r2 & ~r1) & 0xFFFF; }
static void exmod(void)  { combin(0); r2 = (int16_t)((int16_t)r2 % (int16_t)r1); }
static void exnot(void)  { combin(0); r2 = (r2 + ~r1) & 0xFFFF; }
static void excmbin(void){ r3 = r0; }
static void exlsh(void)  { combin(0); r2 = (r2 << r1) & 0xFFFF; }
static void exrsh(void)
{
	int count = r1;
	if (count == 0) {                  /* neg r1; beq exlsh -> als 0 */
		combin(0);
		return;
	}
	r2 = (r2 & 0xFFFF) >> 1;           /* clc; ror r2 (logical, bit15 <- 0) */
	combin(0);
	r2 = ((int16_t)r2 >> (count - 1)) & 0xFFFF;   /* als -(count-1) */
}

static void eoprnd(void);
static void advanc(void);
static void oprand(void);
static void binop(void);
static void brack(void);
static void exnum(void);
static void esw1_dispatch(void);
static void proc_token(void);

static void binop(void)               /* as17.s:72 */
{
	if (*sp != '+')                    /* cmpb (sp),$'+; beq 1f; errore */
		errore();
	*sp = r4;                          /* movb r4,(sp) */
	advanc();
}

static void exnum(void)                /* as17.s:80 */
{
	r1 = numval;
	r0 = 1;
	oprand();
}

static void brack(void)                /* as17.s:85 */
{
	push(r2);
	push(r3);
	readop();
	expres();
	if (r4 != ']')
		error(']');
	r0 = r3;
	r1 = r2;
	r3 = pop();
	r2 = pop();
	oprand();
}

static void oprand(void)               /* as17.s:99 */
{
	opfound = 1;
	switch (*sp) {                     /* cmp (sp),(r5)+ ... jmp *(r5) */
	case '+':  exadd(); break;
	case '-':  exsub(); break;
	case '*':  exmul(); break;
	case '/':  exdiv(); break;
	case 037:  exor(); break;
	case '&':  exand(); break;
	case 035:  exlsh(); break;
	case 036:  exrsh(); break;
	case '%':  exmod(); break;
	case '!':  exnot(); break;
	case '^':  excmbin(); break;
	default:   break;                   /* no pending operator */
	}
	eoprnd();
}

static void eoprnd(void)               /* as17.s:191 */
{
	*sp = '+';                         /* mov $'+,(sp) */
	advanc();                          /* jmp advanc */
}

static void esw1_dispatch(void)        /* as17.s:40 esw1 table */
{
	switch (r4) {
	case '+': case '-': case '*': case '/': case '&':
	case 037: case 035: case 036: case '%': case '^': case '!':
		binop();
		return;
	case '[':
		brack();
		return;
	case 1:
		exnum();
		return;
	default:                           /* 0; 0 terminator: end of expression */
		if (!opfound)
			errore();
		sp++;                          /* tst (sp)+: pop the pending operator */
		return;
	}
}

static void proc_token(void)           /* as17.s:15 (the 1: label) */
{
	r0 = r4;
	if (r4 >= 0 && r4 <= 0177) {       /* betwen r4,0,177 */
		if (r4 < 0141) {               /* blo 1f -> esw1 */
			esw1_dispatch();
			return;
		}
		if (r4 < 0141 + 10) {          /* forward reference (0141..0152) */
			int i = (int)r4 - 0141;
			r0 = curfbr[i];            /* movb curfbr-141(r4),r0 */
			r2 = curfb[i];             /* mov curfb-[2*141](r4),r2 */
			if (r2 == -1)
				error('f');
			oprand();
			return;
		}
		r3 = 0;                        /* 2: clr r3; clr r2 */
		r2 = 0;
		oprand();
		return;
	}
	/* symbol pointer: r4 = &entry.type */
	{
		unsigned char *p = (unsigned char *)r4;
		r0 = p[0];                     /* movb (r4),r0 (type) */
		r1 = p[2] | (p[3] << 8);       /* mov 2(r4),r1 (value) */
	}
	oprand();
}

static void advanc(void)               /* as17.s:13 */
{
	readop();
	proc_token();                      /* fall into 1: */
}

static void expres(void)               /* as17.s:6 */
{
	push('+');                         /* mov $'+,-(sp) */
	opfound = 0;
	r2 = 0;
	r3 = 1;
	proc_token();                      /* br 1f */
}

/* ------------------------------------------------------------- stubs
 * Placeholders until the corresponding .s is translated; they must not be
 * reached before then.
 */
/* checkeos -- as13.s:122: true unless r4 is end-of-line/statement/input. */
static int checkeos(void)
{
	return !(r4 == '\n' || r4 == ';' || r4 == '\004');
}

/* ealoop -- as13.s:88: end-of-line handling.  Returns 1 to keep assembling,
 * 0 at end of input. */
static int ealoop(void)
{
	if (r4 == ';')
		return 1;
	if (r4 == '\n') {
		line++;
		return 1;
	}
	if (r4 == '\004') {
		if (ifflg)
			error('x');
		return 0;
	}
	error('x');                            /* stray token: skip to EOL */
	while (checkeos())
		readop();
	return 1;
}

/* assem -- as13.s:6: the main line loop. */
static void assem(void)
{
	for (;;) {
		intptr_t label;
		readop();
		if (!checkeos()) {                 /* at end of line */
			if (ealoop())
				continue;
			return;
		}
		if (ifflg) {                       /* inside a false .if: track nesting */
			if (r4 <= 0200)
				continue;
			if (((unsigned char *)r4)[0] == 021)
				ifflg++;                   /* .if */
			else if (((unsigned char *)r4)[0] == 022)
				ifflg--;                   /* .endif */
			continue;
		}
		/* normal processing */
		label = r4;
		readop();
		if (r4 == '=') {                   /* assignment */
			readop();
			expres();
			if (label < 0200) {
				error('x');
				if (ealoop())
					continue;
				return;
			}
			if (label == (intptr_t)&symtab[0].type) {   /* assigning to dot */
				r3 &= ~040;
				if (r3 != symtab[0].type) {
					error('.');
					symtab[0].type = 2;
					if (ealoop())
						continue;
					return;
				}
			}
			{
				unsigned char *p = (unsigned char *)label;
				p[0] &= ~037;              /* bicb $37,(r1) */
				r3 &= 037;
				if (r3 == 0)
					r2 = 0;                /* clr r2 */
				p[0] |= r3;                /* bisb r3,(r1) */
				*(uint16_t *)(p + 2) = r2; /* mov r2,2(r1) */
			}
			if (ealoop())
				continue;
			return;
		}
		if (r4 == ':') {                   /* label definition */
			if (label >= 0200) {           /* symbol */
				unsigned char *p = (unsigned char *)label;
				if (p[0] & 037)            /* bitb $37,(r4) */
					error('m');
				p[0] |= (unsigned char)symtab[0].type;     /* bisb dot-2,(r4) */
				*(uint16_t *)(p + 2) = symtab[0].value;        /* mov dot,2(r4) */
			} else if (label == 1) {       /* numeric label (forward ref) */
				int idx = fbcheck(numval);
				curfbr[idx] = (char)symtab[0].type;        /* movb dotrel,curfbr(r0) */
				nxtfb[0] = (unsigned char)symtab[0].type;  /* movb dotrel,nxtfb */
				nxtfb[1] = (unsigned char)(2 * idx);   /* movb r0,nxtfb+1 */
				nxtfb[2] = (unsigned char)(symtab[0].value & 0xFF); /* mov dot,nxtfb+2 */
				nxtfb[3] = (unsigned char)((symtab[0].value >> 8) & 0xFF);
				curfb[idx] = symtab[0].value;          /* mov dot,curfb(r0) */
				if (write(fbfil, nxtfb, 4) != 4)
					wrterr();
			} else {
				error('x');
			}
			continue;                      /* br assem */
		}
		/* ordinary line: operator */
		savop = r4;
		r4 = label;
		opline();
		if (ealoop())
			continue;
		return;
	}
}
/* ------------------------------------------------------------- opline
 * (as16.s).  The opcode dispatch (`jmp *1f-12(r0)` keyed on the type byte) and
 * the operand addressing modes.  The types are octal and contiguous 5..30
 * decimal, so the switch below runs over type-5.  Note r0 == 2*type at handler
 * entry in the original (the dispatch's `asl r0`); the C uses the raw type.
 */

static void errora(void) { error('a'); }

static void addres(void);
static void getx(void);
static void alp(void);
static void amin(void);
static void adoll(void);
static void astar(void);
static void checkreg(void);
static void checkrp(void);
static void xpr(void);
static void opl6(void);
static void opl13(void);
static void opl15(void);
static void opl16(void);
static void opl17(void);
static void opl20(void);
static void opl21(void);
static void opl23(void);
static void opl25(void);
static void opl26(void);
static void opl27(void);
static void opl31(void);
static void opl32(void);
static void opl35(void);
static void opl36(void);

/* xpr -- as16.s:13: an expression operand occupies one word. */
static void xpr(void)
{
	expres();
	symtab[0].value += 2;
}

/* checkreg -- as16.s:267: register number 0..7, absolute or special type. */
static void checkreg(void)
{
	if (!(r2 <= 7 && (r3 == 1 || r3 > 4)))
		errora();
}

/* checkrp -- as16.s:283: expect `)`, then read on. */
static void checkrp(void)
{
	if (r4 != ')')
		error(')');
	readop();
}

/* getx -- as16.s:195: the ordinary expression operand. */
static void getx(void)
{
	expres();
	if (r4 == '(') {               /* indexed: expr(rN) */
		readop();
		expres();
		checkreg();
		checkrp();
		symtab[0].value += 2;
		r0 = 0;
		return;
	}
	if (r3 == 024) {               /* register */
		checkreg();
		r0 = 0;
		return;
	}
	symtab[0].value += 2;
	r0 = 0;
}

/* alp -- as16.s:217: (rN)+ autoincrement */
static void alp(void)
{
	readop();
	expres();
	checkrp();
	checkreg();
	if (r4 == '+') {
		readop();
		r0 = 0;
	} else {
		r0 = 2;
	}
}

/* amin -- as16.s:231: -(rN) autodecrement, or unary minus */
static void amin(void)
{
	readop();
	if (r4 == '(') {               /* -(rN) */
		readop();
		expres();
		checkrp();
		checkreg();
		r0 = 0;
		return;
	}
	savop = r4;                    /* unary minus: save operator, read expr */
	r4 = '-';
	getx();
}

/* adoll -- as16.s:246: $immediate */
static void adoll(void)
{
	readop();
	expres();
	symtab[0].value += 2;
	r0 = 0;
}

/* astar -- as16.s:253: *indirect */
static void astar(void)
{
	readop();
	if (r4 == '*')
		error('*');
	addres();
	symtab[0].value += r0;
}

/* addres -- as16.s:186: dispatch on the leading operand character. */
static void addres(void)
{
	if (r4 == '(') { alp(); return; }
	if (r4 == '-') { amin(); return; }
	if (r4 == '$') { adoll(); return; }
	if (r4 == '*') { astar(); return; }
	getx();
}

/* branch / rts / sys -- as16.s:103 */
static void opl6(void)
{
	expres();
	symtab[0].value += 2;
}

/* single operand -- as16.s:89 */
static void opl15(void)
{
	addres();
	symtab[0].value += 2;
}

/* double operand -- as16.s:79 */
static void opl13(void)
{
	addres();
	if (r4 != ',') {
		errora();
		return;
	}
	readop();
	opl15();
}

/* sob -- as16.s:94 */
static void opl31(void)
{
	expres();
	if (r4 != ',')
		errora();
	readop();
	opl6();
}

/* .byte -- as16.s:111 */
static void opl16(void)
{
	for (;;) {
		expres();
		symtab[0].value++;
		if (r4 != ',')
			break;
		readop();
	}
}

/* .ascii / <string> -- as16.s:122 */
static void opl17(void)
{
	symtab[0].value += numval;
	readop();
}

/* .even -- as16.s:128 */
static void opl20(void)
{
	symtab[0].value++;
	symtab[0].value &= ~1;
}

/* .if -- as16.s:134 */
static void opl21(void)
{
	expres();
	if (r3 == 0)
		error('U');
	if (r2 == 0)
		ifflg++;
}

/* .globl -- as16.s:147 */
static void opl23(void)
{
	for (;;) {
		if (r4 < 0200)
			return;
		((unsigned char *)r4)[0] |= 040;   /* bisb $40,(r4) */
		readop();
		if (r4 != ',')
			return;
		readop();
	}
}

/* .text/.data/.bss -- as16.s:159: switch the location counter */
static void section(int type)
{
	int ns = type - 023;                   /* 025->2, 026->3, 027->4 */
	savdot[symtab[0].type - 2] = symtab[0].value;
	symtab[0].value = savdot[ns - 2];
	symtab[0].type = ns;
}
static void opl25(void) { section(025); }
static void opl26(void) { section(026); }
static void opl27(void) { section(027); }

/* .comm -- as16.s:172 */
static void opl32(void)
{
	if (r4 >= 0200) {
		((unsigned char *)r4)[0] |= 040;
		readop();
		if (r4 == ',') {
			readop();
			expres();
			return;
		}
	}
	error('x');
}

/* jbr -- as16.s:58 */
static void opl35(void)
{
	int size = 4;
	expres();
	if (r3 == symtab[0].type) {
		int off = r2 - symtab[0].value;
		if (off < 0 && off >= -254)        /* $-376 octal = -254 decimal */
			size = 2;
	}
	symtab[0].value += size;
}

/* jeq, jne, ... -- as16.s:63 */
static void opl36(void)
{
	int size = 6;
	expres();
	if (r3 == symtab[0].type) {
		int off = r2 - symtab[0].value;
		if (off < 0 && off >= -254)
			size = 2;
	}
	symtab[0].value += size;
}

/* opline -- as16.s:6: dispatch one line's opcode/directive on its type. */
static void opline(void)
{
	if (r4 >= 0 && r4 <= 0200) {           /* betwen 0; 200 */
		if (r4 == '<')
			opl17();
		else
			xpr();
		return;
	}
	{
		int type = ((unsigned char *)r4)[0];   /* movb (r4),r0 */
		if (type == 024) {                     /* register -> xpr */
			xpr();
			return;
		}
		if (type < 5 || type > 036) {          /* betwen 5; 36 */
			xpr();
			return;
		}
		readop();                              /* read the operand token */
		switch (type) {
		case 05:  opl13(); break;   /* flop freg,dst */
		case 06:  opl6();  break;   /* branch */
		case 07:  opl13(); break;   /* jsr */
		case 010: opl6();  break;   /* rts */
		case 011: opl6();  break;   /* sys */
		case 012: opl13(); break;   /* movf */
		case 013: opl13(); break;   /* double operand */
		case 014: opl13(); break;   /* flop fsrc,freg */
		case 015: opl15(); break;   /* single operand */
		case 016: opl16(); break;   /* .byte */
		case 017: opl17(); break;   /* string */
		case 020: opl20(); break;   /* .even */
		case 021: opl21(); break;   /* .if */
		case 022: break;            /* .endif (no-op; ifflg handled in assem) */
		case 023: opl23(); break;   /* .globl */
		case 024: xpr();    break;  /* register (unreachable) */
		case 025: opl25(); break;   /* .text */
		case 026: opl26(); break;   /* .data */
		case 027: opl27(); break;   /* .bss */
		case 030: opl13(); break;   /* mul,div */
		case 031: opl31(); break;   /* sob */
		case 032: opl32(); break;   /* .comm */
		case 033: xpr();    break;  /* estimated text */
		case 034: xpr();    break;  /* estimated data */
		case 035: opl35(); break;   /* jbr */
		case 036: opl36(); break;   /* jeq, jne, ... */
		}
	}
}
