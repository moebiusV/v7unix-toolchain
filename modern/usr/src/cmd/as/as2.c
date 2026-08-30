/*
 * as2.c -- PDP-11 assembler pass 2, translated from as21.s..as29.s.
 *
 * Faithful translation of V7 /usr/src/cmd/as, pass 2 (the /lib/as2 binary).
 * Pass 2 reads pass 1's intermediate files and emits the final a.out: text,
 * data, relocation, and a symbol table.  The .s sources are the conformance
 * oracle; conventions are as in as1.c.
 *
 * The one structural difference from pass 1: pass 2's symbol table has
 * 4-byte entries (type + value; the name is only a comment in as29.s).  That
 * is why pass 1's symbol code is base + (entry-table)/3: the /3 turns pass 1's
 * 12-byte stride into the byte offset of pass 2's 4-byte table.
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

int r0, r1, r2, r3, r5;                /* working registers */
intptr_t r4;                           /* token value or symbol pointer */

/* ------------------------------------------------------------- symbol table
 * as29.s: 4-byte entries (type, value); the name is a comment only.  The
 * location counter lives in symtab[0], as in pass 1.
 */
struct sym2 { uint16_t type; uint16_t value; };
static struct sym2 symtab[] = {
	{ 002, 000000 },   /* .   */
	{ 001, 000000 },   /* ..  */
	{ 024, 000000 },   /* r0 */
	{ 024, 000001 },   /* r1 */
	{ 024, 000002 },   /* r2 */
	{ 024, 000003 },   /* r3 */
	{ 024, 000004 },   /* r4 */
	{ 024, 000005 },   /* r5 */
	{ 024, 000006 },   /* sp */
	{ 024, 000007 },   /* pc */
	/* double operand */
	{ 013, 0010000 }, { 013, 0110000 }, { 013, 0020000 }, { 013, 0120000 },
	{ 013, 0030000 }, { 013, 0130000 }, { 013, 0040000 }, { 013, 0140000 },
	{ 013, 0050000 }, { 013, 0150000 }, { 013, 0060000 }, { 013, 0160000 },
	/* branch */
	{ 006, 0000400 }, { 006, 0001000 }, { 006, 0001400 }, { 006, 0002000 },
	{ 006, 0002400 }, { 006, 0003000 }, { 006, 0003400 }, { 006, 0100000 },
	{ 006, 0100400 }, { 006, 0101000 }, { 006, 0101400 }, { 006, 0102000 },
	{ 006, 0102400 }, { 006, 0103000 }, { 006, 0103000 }, { 006, 0103000 },
	{ 006, 0103400 }, { 006, 0103400 }, { 006, 0103400 },
	/* jump/branch type */
	{ 035, 0000400 }, { 036, 0001000 }, { 036, 0001400 }, { 036, 0002000 },
	{ 036, 0002400 }, { 036, 0003000 }, { 036, 0003400 }, { 036, 0100000 },
	{ 036, 0100400 }, { 036, 0101000 }, { 036, 0101400 }, { 036, 0102000 },
	{ 036, 0102400 }, { 036, 0103000 }, { 036, 0103000 }, { 036, 0103000 },
	{ 036, 0103400 }, { 036, 0103400 }, { 036, 0103400 },
	/* single operand */
	{ 015, 0005000 }, { 015, 0105000 }, { 015, 0005100 }, { 015, 0105100 },
	{ 015, 0005200 }, { 015, 0105200 }, { 015, 0005300 }, { 015, 0105300 },
	{ 015, 0005400 }, { 015, 0105400 }, { 015, 0005500 }, { 015, 0105500 },
	{ 015, 0005600 }, { 015, 0105600 }, { 015, 0005700 }, { 015, 0105700 },
	{ 015, 0006000 }, { 015, 0106000 }, { 015, 0006100 }, { 015, 0106100 },
	{ 015, 0006200 }, { 015, 0106200 }, { 015, 0006300 }, { 015, 0106300 },
	{ 015, 0000100 }, { 015, 0000300 },
	/* jsr / rts / sys */
	{ 007, 0004000 }, { 010, 000200 }, { 011, 0104400 },
	/* flag-setting */
	{ 001, 0000241 }, { 001, 0000242 }, { 001, 0000244 }, { 001, 0000250 },
	{ 001, 0000261 }, { 001, 0000262 }, { 001, 0000264 }, { 001, 0000270 },
	/* floating point ops */
	{ 001, 0170000 }, { 001, 0170001 }, { 001, 0170011 }, { 001, 0170002 },
	{ 001, 0170012 }, { 015, 0170400 }, { 015, 0170700 }, { 015, 0170600 },
	{ 015, 0170500 }, { 012, 0172400 }, { 014, 0177000 }, { 005, 0175400 },
	{ 014, 0177400 }, { 005, 0176000 }, { 014, 0172000 }, { 014, 0173000 },
	{ 014, 0171000 }, { 014, 0174400 }, { 014, 0173400 }, { 014, 0171400 },
	{ 014, 0176400 }, { 005, 0175000 }, { 015, 0170100 }, { 015, 0170200 },
	{ 024, 000000 }, { 024, 000001 }, { 024, 000002 }, { 024, 000003 },
	{ 024, 000004 }, { 024, 000005 },
	/* 11/45 operations */
	{ 030, 072000 }, { 030, 073000 }, { 030, 070000 }, { 030, 071000 },
	{ 007, 074000 }, { 015, 006700 }, { 011, 006400 }, { 031, 077000 },
	/* specials */
	{ 016, 000000 }, { 020, 000000 }, { 021, 000000 }, { 022, 000000 },
	{ 023, 000000 }, { 025, 000000 }, { 026, 000000 }, { 027, 000000 },
	{ 032, 000000 },
	/* EIS aliases (`.if eae-1`, eae = 0): mul/div/ash/ashc == mpy/dvd/als/alsc */
	{ 030, 070000 },   /* mul */
	{ 030, 071000 },   /* div */
	{ 030, 072000 },   /* ash */
	{ 030, 073000 },   /* ashc */
};

/* ------------------------------------------------------------- data (as28.s) */
static char qnl[] = "?\n";
static char aout_name[] = "a.out\0";
static char a_tmp1[20], a_tmp2[20], a_tmp3[20];   /* filled from argv */
static char *a_outp = aout_name;                   /* -o name */

/* the seek table (as28.s): txtseek, datseek, gap, trelseek, drelseek,
 * gap, symseek -- indexed by 4*(type-025) bytes in the section switch. */
static uint32_t seeks[7];

static int sizes[6];   /* txtsiz,datsiz,bsssiz,symsiz,stksiz,exorig */

/* an output buffer: 8-byte header + 512 bytes of words */
struct obuf {
	uint16_t *next;         /* next slot to fill */
	uint16_t *end;          /* one past the buffer */
	uint32_t  seek;         /* file offset of this buffer */
	uint16_t  data[256];
};
static struct obuf txtp, relp;

static uint16_t inbuf[256];            /* input buffer (512 bytes) */
static int  ibufc, ibufp;
static char fin, fout, fbfil, txtfil, symf;

static int  brlen = 1024;
static char brtab[128];                /* brlen/8 */
static int  brtabp, brdelt;
static int  fbbufp, defund;
static int  savdot[3];                 /* 6 bytes */
static int  datbase, bssbase;
static int  adrbuf[6];                 /* 12 bytes: 3 entries */
static intptr_t xsymbol;               /* external symbol pointer */
static int  errflg;
static char argb[22];                  /* filename buffer */
static int  line;
static intptr_t savop;                 /* token save: holds a symbol pointer */
static int  curfb[20];                 /* curfb[0..9] + nxtfb[0..9], adjacent
                                          in the original (as28.s) */
static int  numval, maxtyp;
static int  swapf, rlimit, passno;
static int  endtable;
static unsigned char *usymtab;         /* grows; 4-byte entries */
static unsigned char *symend;
static int  outmod = 0777;             /* 0666 = "nonexecutable" */

/* ------------------------------------------------------------- shims */
static void saexit(void);

static void wrterr(void)
{
	static const char msg[] = "as: write error on output\n";
	write(1, msg, sizeof(msg) - 1);
	saexit();
}

static void error(int code);

static void filerr(const char *name)
{
	if (name)
		write(1, name, strlen(name));
	write(1, qnl, 2);
	saexit();
}

/* error(code) -- as22.s:110.  Prints the filename buffer, then code+line. */
static void error(int code)
{
	errflg = 1;
	outmod = 0666;                     /* make nonexecutable */
	{
		char *p = argb;
		while (*p) {
			write(1, p, 1);
			*p++ = 0;
		}
	}
	{
		char msg[8];
		msg[0] = (char)code;
		msg[1] = ' ';
		msg[6] = '\n';
		{
			int n = line, i;
			for (i = 5; i >= 2; i--) {
				msg[i] = (char)('0' + n % 10);
				n /= 10;
			}
		}
		write(1, msg, 7);
	}
}

static void saexit(void)
{
	unlink(a_tmp1);
	unlink(a_tmp2);
	unlink(a_tmp3);
	if (errflg)
		exit(2);
	(void)umask(0);
	outmod &= 0777;
	(void)chmod(a_outp, outmod);
	exit(0);
}

/* ofile(name) -- as21.s:271: open a file, filerr on failure. */
static int ofile(const char *name)
{
	int fd = open(name, 0);
	if (fd < 0)
		filerr(name);
	return fd;
}

/* ------------------------------------------------------------- input
 * getw -- as24.s:102.  Read one word from the intermediate file; returns 1
 * on EOF (the `bvs` path), 0 with the word in r4 otherwise.
 */
static int getw(void)
{
	if (savop) {
		r4 = savop;
		savop = 0;
		return 0;
	}
	for (;;) {
		if (--ibufc > 0) {             /* dec ibufc; bgt 1f */
			r4 = inbuf[ibufp++];       /* mov *ibufp,r4; add $2,ibufp */
			return 0;
		}
		{                              /* refill the buffer */
			int n = read(fin, inbuf, 512);
			if (n < 0)
				return 1;              /* bes 3f */
			n >>= 1;                   /* asr r0: bytes -> words */
			if (n == 0)
				return 1;              /* 3: EOF */
			ibufc = n;
			ibufp = 0;                 /* 2: mov $inbuf,ibufp */
		}
		r4 = inbuf[ibufp++];           /* 1: first word of the fresh buffer */
		return 0;
	}
}

/* readop -- as24.s:84.  Read a token word; decode symbol codes to pointers. */
static void readop(void)
{
	if (savop) {
		r4 = savop;
		savop = 0;
		return;
	}
	(void)getw();
	if (r4 >= 0x200) {
		if (r4 < 0x800)
			r4 = (intptr_t)((unsigned char *)symtab + (r4 - 0x200));
		else
			r4 = (intptr_t)(usymtab + (r4 - 0x800));
	}
}

/* fbadv -- as23.s:147: resolve a forward reference.  Searches the f-b record
 * table (4-byte entries: type+31 | index<<8, value) for the matching index. */
static void fbadv(void)
{
	int idx = r4;
	int p;
	r4 <<= 1;                              /* asl r4: r4 = 2*idx */
	p = (curfb + 10)[idx];
	curfb[idx] = p;
	if (p != 0)
		p += 4;
	else
		p = fbbufp;
	for (;;) {
		if (usymtab[p + 1] == (r4 & 0xFF)) /* cmpb 1(r1),r4; beq 1f */
			break;
		if (usymtab[p + 1] & 0x80)         /* tst (r1); bpl fails: end marker */
			break;
		p += 4;                            /* add $4,r1 */
	}
	(curfb + 10)[idx] = p;
	r4 >>= 1;                              /* asr r4 */
}

/* ------------------------------------------------------------- output
 * oset / putw / flush -- as24.s.  Buffered, seeked output to the object file.
 */

static void flush(struct obuf *b);

static void oset(struct obuf *b, uint32_t seek)
{
	b->next = b->data;
	b->end = b->data + 256;
	b->seek = seek;
}

static void putw(struct obuf *b, int w)
{
	if (b->next >= b->end) {
		flush(b);
	}
	*b->next++ = (uint16_t)w;
}

static void flush(struct obuf *b)
{
	size_t n = b->next - b->data;
	if (n == 0)
		return;
	if (lseek(fout, b->seek, 0) < 0)
		wrterr();
	if (write(fout, b->data, n * 2) != (ssize_t)(n * 2))
		wrterr();
	b->seek += n * 2;
	b->next = b->data;
}

/* ------------------------------------------------------------- output
 * outw / outb -- as22.s.  Emit a word/byte with its relocation entry.
 */

static uint32_t *tseekp = &seeks[0];    /* text seek/size accumulator */
static uint32_t *rseekp = &seeks[3];   /* text-relocation seek/size */

static void outb(void);

static void outw(void)
{
	int pcrel;
	int rel;

	if (symtab[0].type == 4) {                 /* cmp dot-2,$4: bss mode */
		error('x');
		return;
	}
	if (symtab[0].value & 1) {                     /* bit $1,dot: odd address */
		error('o');
		r3 = 0;
		outb();
		return;
	}
	symtab[0].value += 2;
	if (!passno)                       /* tstb passno; beq 8f */
		return;
	pcrel = (r3 & 0100000) ? 1 : 0;    /* rol r3; adc (sp): pcrel flag is bit 15 */
	r3 &= 077777;                      /* asr r3: drop the pcrel flag from r3 */
	if (r3 == 040) {                   /* external reference */
		outmod = 0666;
		rel = (((xsymbol - (intptr_t)usymtab) << 1) | 4);
	} else {
		r3 &= ~040;                    /* bic $40,r3 */
		if (r3 >= 5) {                 /* cmp r3,$5; blo 4f */
			if (r3 == 033 || r3 == 034)
				error('r');
			r3 = 1;                    /* mov $1,r3 */
		}
		if (r3 >= 2 && r3 <= 4) {      /* text/data/bss */
			if (!pcrel)
				r2 += symtab[1].value;          /* add symtab[1].value,r2 */
		} else if (pcrel) {
			r2 -= symtab[1].value;              /* sub symtab[1].value,r2 */
		}
		rel = r3 - 1;                  /* dec r3; bpl 3f */
		if (rel < 0)
			rel = 0;                   /* clr r3 */
	}
	rel = (rel << 1) | pcrel;          /* asl r3; bis (sp)+,r3 */
	putw(&txtp, r2);
	*tseekp += 2;                      /* add $2,2(tseekp); adc */
	putw(&relp, rel);
	*rseekp += 2;
}

static void outb(void)
{
	if (symtab[0].type == 4) {                 /* bss */
		error('x');
		return;
	}
	if (r3 > 1)                        /* relocatable byte */
		error('r');
	if (!passno) {
		symtab[0].value++;
		return;
	}
	if (symtab[0].value & 1) {                     /* odd: patch the last word's high byte */
		((unsigned char *)txtp.next)[-1] = (unsigned char)(r2 & 0xFF);
	} else {
		putw(&txtp, r2);
		putw(&relp, 0);
		*tseekp += 2;
		*rseekp += 2;
	}
	symtab[0].value++;
}

/* ------------------------------------------------------------- expression
 * evaluator (as27.s) with the pass-2 relocation matrix.
 */

enum { ESTK = 64 };
static int estk[ESTK];
static int *sp = estk + ESTK;
static void push(int x) { *--sp = x; }
static int  pop(void)   { return *sp++; }

enum { M = -1, X = -2 };
static const signed char reltp2[36] = {
	0,0,0,0,0,0,  0,M,2,3,4,040,  0,2,X,X,X,X,
	0,3,X,X,X,X,  0,4,X,X,X,X,    0,040,X,X,X,X,
};
static const signed char reltm2[36] = {
	0,0,0,0,0,0,  0,M,2,3,4,040,  0,X,1,X,X,X,
	0,X,X,1,X,X,  0,X,X,X,1,X,    0,X,X,X,X,X,
};
static const signed char relte2[36] = {
	0,0,0,0,0,0,  0,M,X,X,X,X,    0,X,X,X,X,X,
	0,X,X,X,X,X,  0,X,X,X,X,X,    0,X,X,X,X,X,
};

static void errore(void) { error('e'); }

static void combin(const signed char *matrix);

/* maprel(t) -> 0..5 (5 = external); tracks maxtyp */
static int maprel(int t)
{
	if (t == 040)
		return 5;
	t &= 037;
	if (t > maxtyp)
		maxtyp = t;
	if (t >= 5)
		return 1;
	return t;
}

static void combin(const signed char *matrix)
{
	if (!passno) {                     /* pass 1: as17.s combin */
		int relbit = (r0 | r3) & 040;
		int a = r0 & 037, b = r3 & 037, cls;
		if (a > b) { int t = a; a = b; b = t; }
		if (a == 0)
			cls = 0;
		else if (matrix == reltm2 && a == b)   /* cmp (r5)+,$reltm2 */
			cls = 1;
		else
			cls = b;
		r3 = cls | relbit;
		return;
	}
	{
		int row, col;
		signed char c;
		maxtyp = 0;                        /* clr maxtyp (as27.s:233) */
		row = maprel(r0) * 6;
		col = maprel(r3);
		c = matrix[row + col];
		if (c >= 0)
			r3 = c;
		else if (c == M)
			r3 = maxtyp;
		else {
			error('r');
			r3 = maxtyp;
		}
	}
}

static void exadd(void)  { combin(reltp2); r2 = (r2 + r1) & 0xFFFF; }
static void exsub(void)  { combin(reltm2); r2 = (r2 - r1) & 0xFFFF; }
static void exmul(void)  { combin(relte2); r2 = (r1 * r2) & 0xFFFF; }
static void exdiv(void)  { combin(relte2); r2 = (int16_t)((int16_t)r2 / (int16_t)r1); }
static void exor(void)   { combin(relte2); r2 = (r2 | r1) & 0xFFFF; }
static void exand(void)  { combin(relte2); r2 = (r2 & ~r1) & 0xFFFF; }
static void exmod(void)  { combin(relte2); r2 = (int16_t)((int16_t)r2 % (int16_t)r1); }
static void exnot(void)  { combin(relte2); r2 = (r2 + ~r1) & 0xFFFF; }
static void excmbin(void){ r3 = r0; }
static void exlsh(void)  { combin(relte2); r2 = (r2 << r1) & 0xFFFF; }
static void exrsh(void)
{
	int count = r1;
	if (count == 0) {
		combin(relte2);
		return;
	}
	r2 = (r2 & 0xFFFF) >> 1;
	combin(relte2);
	r2 = ((int16_t)r2 >> (count - 1)) & 0xFFFF;
}

static void eoprnd(void);
static void advanc(void);
static void oprand(void);
static void binop(void);
static void brack(void);
static void exnum(void);
static void exnum1(void);
static void esw1_dispatch(void);
static void proc_token(void);
static void expres1(void);

static void binop(void)
{
	if (*sp != '+')
		errore();
	*sp = r4;
	advanc();
}

static void exnum(void)
{
	(void)getw();
	r1 = r4;
	r0 = 1;
	oprand();
}

static void exnum1(void)
{
	r1 = numval;
	r0 = 1;
	oprand();
}

static void brack(void)
{
	push(r2);
	push(r3);
	readop();
	expres1();
	if (r4 != ']')
		error(']');
	r0 = r3;
	r1 = r2;
	r3 = pop();
	r2 = pop();
	oprand();
}

static void oprand(void)
{
	switch (*sp) {
	case '+':  exadd(); break;
	case '-':  exsub(); break;
	case '*':  exmul(); break;
	case '/':  exdiv(); break;
	case 037:  exor(); break;
	case '&':  exand(); break;
	case 035:  exlsh(); break;
	case 036:  exrsh(); break;
	case '%':  exmod(); break;
	case '^':  excmbin(); break;
	case '!':  exnot(); break;
	default:   break;
	}
	eoprnd();
}

static void eoprnd(void)
{
	*sp = '+';
	advanc();
}

static void esw1_dispatch(void)
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
	case 2:
		exnum1();
		return;
	default:                           /* 200; 0 terminator */
		sp++;                          /* tst (sp)+ */
		return;
	}
}

static void proc_token(void)
{
	r0 = r4;
	if (r4 >= 0 && r4 <= 0177) {
		if (r4 < 0141) {               /* blo 1f -> esw1 */
			esw1_dispatch();
			return;
		}
		{                              /* forward reference */
			int p = curfb[(int)r4 - 0141];
			r1 = usymtab[p + 2] | (usymtab[p + 3] << 8);
			r0 = usymtab[p];
		}
		oprand();
		return;
	}
	{                                  /* symbol pointer: r4 = &entry.type */
		unsigned char *p = (unsigned char *)r4;
		r0 = p[0];
		if (r0 == 0 && passno)
			error('u');
		if (r0 == 040) {
			xsymbol = r4;
			r1 = 0;
		} else {
			r1 = p[2] | (p[3] << 8);
		}
	}
	oprand();
}

static void advanc(void)
{
	readop();
	proc_token();
}

static void expres1(void)
{
	push('+');
	r2 = 0;
	r3 = 1;
	proc_token();
}

static void expres(void)
{
	xsymbol = 0;                       /* clr xsymbol */
	expres1();
}

/* ------------------------------------------------------------- opline
 * (as26.s) -- the opcode dispatch and instruction encoding, plus the
 * addressing modes and the branch table.
 */

static int swab16(int x)
{
	x &= 0xFFFF;
	return (x >> 8) | ((x & 0xFF) << 8);
}

static int *adrp;                      /* r5: the adrbuf write pointer */

static void errora(void) { error('a'); }

static void checkreg(void)
{
	if (r2 > 7 || (r1 > 1 && r3 < 5)) {
		errora();
		r2 = 0;
		r3 = 0;
	}
}

static void checkrp(void)
{
	if (r4 != ')')
		error(')');
	readop();
}

static void addres(void);
static void getx(void);
static void alp(void);
static void amin(void);
static void adoll(void);
static void xpr(void);
static void opeof(void);
static void op2a(void);
static void opl6(void);
static void opl11(void);
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

static void xpr(void)
{
	expres();
	outw();
}

/* addres -- as26.s:378: parse an operand, fill adrbuf, return mode in r2 */
static void getx(void)
{
	expres();
	if (r4 == '(') {                   /* indexed expr(rN) */
		readop();
		*adrp++ = r2;                  /* index value */
		*adrp++ = r3;                  /* index type */
		*adrp++ = (int)xsymbol;        /* index global */
		expres();
		checkreg();
		checkrp();
		r2 |= 060;                     /* mode 6 */
		r2 |= pop();
		return;
	}
	if (r3 == 024) {                   /* register */
		checkreg();
		r2 |= pop();
		return;
	}
	{                                  /* relative: X(pc) */
		int off = r2 - symtab[0].value - 4;
		if (adrp != adrbuf)
			off -= 2;
		*adrp++ = off;                 /* index */
		*adrp++ = r3 | 0100000;        /* index reloc (mark estimated) */
		*adrp++ = (int)xsymbol;        /* index global */
		r2 = 067;                      /* mode 6, reg 7 */
		r2 |= pop();
	}
}

static void alp(void)                  /* (rN)+ */
{
	readop();
	expres();
	checkrp();
	checkreg();
	if (r4 == '+') {
		readop();
		r2 |= 020;                     /* mode 2 */
		r2 |= pop();
		return;
	}
	if (pop() != 0) {                  /* tst (sp)+: an index follows */
		r2 |= 070;                     /* mode 7 (indirect) */
		*adrp++ = 0;
		*adrp++ = 0;
		*adrp++ = (int)xsymbol;
		return;
	}
	r2 |= 010;                         /* mode 1 */
}

static void amin(void)                 /* -(rN) or unary minus */
{
	readop();
	if (r4 == '(') {
		readop();
		expres();
		checkrp();
		checkreg();
		r2 |= pop();
		r2 |= 040;                     /* mode 4 */
		return;
	}
	savop = r4;
	r4 = '-';
	getx();
}

static void adoll(void)                /* $immediate */
{
	readop();
	expres();
	*adrp++ = r2;
	*adrp++ = r3;
	*adrp++ = (int)xsymbol;
	r2 = pop();
	r2 |= 027;                         /* mode 2, reg 7 */
}

static void addres(void)
{
	push(0);                           /* clr -(sp): index-present flag */
	for (;;) {                         /* 4: (as26.s:380) */
		if (r4 == '(') { alp(); return; }
		if (r4 == '-') { amin(); return; }
		if (r4 == '$') { adoll(); return; }
		if (r4 != '*') { getx(); return; }
		/* *indirect (as26.s:478 astar) */
		if (*sp != 0)                  /* tst (sp) */
			error('*');
		*sp = 010;                     /* mov $10,(sp) */
		readop();
		/* jmp 4b: re-dispatch without re-pushing the flag */
	}
}

/* setbr / getbr -- as26.s:519/547: the branch table */
static int setbr(int target)
{
	if (brtabp >= brlen)
		return 2;
	{
		int bit = brtabp & 7, byte = brtabp >> 3;
		int off = target - symtab[0].value;
		brtabp++;
		if (off > 0)
			off -= brdelt;
		if (!(off >= -254 && off <= 256)) {
			brtab[byte] |= 1 << bit;
			return 2;
		}
		return 0;
	}
}

static int getbr(void)
{
	if (brtabp >= brlen)
		return 1;
	{
		int bit = brtabp & 7, byte = brtabp >> 3;
		brtabp++;
		return (brtab[byte] >> bit) & 1;
	}
}

/* the double-operand tail: push source mode, read dest, encode, emit */
static void op2a(void)
{
	push(r2);                          /* mov r2,-(sp) */
	readop();
	addres();                          /* dest */
	if (swapf) {                       /* swap source/dest */
		int t = *sp;
		*sp = r2;
		r2 = t;
	}
	*sp = swab16(*sp) >> 2;            /* source mode field */
	if ((unsigned)*sp >= (unsigned)rlimit)
		error('x');
	r2 |= pop();                       /* dest | source field */
	r2 |= pop();                       /* | opcode */
	r3 = 0;
	outw();
	{
		int *p = adrbuf;
		while (p < adrp) {
			r2 = *p++;
			r3 = *p++;
			xsymbol = *p++;
			outw();
		}
	}
}

static void opl13(void) { addres(); op2a(); }          /* double */
static void opl5(void)  { rlimit = 0400; opl13(); }    /* flop src,freg */
static void opl14(void) { swapf++; opl5(); }           /* flop freg,fsrc */
static void opl30(void) { swapf++; rlimit = 01000; opl13(); }  /* mpy/dvd */
static void opl12(void)                                /* movf */
{
	rlimit = 0400;
	addres();
	if (r2 >= 4)
		swapf++;
	else
		*sp = 0174000;                 /* preset fsrc=0 encoding */
	op2a();
}

static void opl7(void)                 /* jsr */
{
	expres();
	checkreg();
	op2a();
}

static void opl11(void)                /* sys, emt */
{
	expres();
	if ((unsigned)r2 >= 256 || r3 > 1)
		errora();
	r2 |= pop();
	outw();
}
static void opl10(void)                /* rts */
{
	expres();
	checkreg();
	r2 |= pop();
	outw();
}

static void dobranch(void)             /* branch emit: r2/r3 already set by expres */
{
	if (passno) {
		int off = r2 - symtab[0].value;
		if (off >= -254 && off <= 256 && !(off & 1) && r3 == symtab[0].type)
			r2 = ((off >> 1) - 1) & 0377;
		else {
			error('b');
			r2 = 0;
		}
	}
	r2 |= pop();
	r3 = 0;
	outw();
}

static void opl6(void)                 /* branch */
{
	expres();
	dobranch();
}

static void opl31(void)                /* sob */
{
	expres();
	checkreg();
	r2 = swab16(r2) >> 2;              /* register field */
	*sp |= r2;                         /* bis r2,(sp): into the opcode */
	readop();
	expres();
	if (passno) {
		int off = symtab[0].value - r2 + 4;        /* sub dot,r2; neg r2; add $4,r2 */
		if (off >= -2 && off <= 0175 && !(off & 1) && r3 == symtab[0].type)
			r2 = ((off >> 1) - 1) & 0377;
		else {
			error('b');
			r2 = 0;
		}
	} else {
		r2 = 0;
	}
	r2 |= pop();
	r3 = 0;
	outw();
}

static void opl35(void)                /* jbr */
{
	expres();
	if (!passno) {
		int size = setbr(r2);
		if (size != 0 && *sp != 0400)  /* not br -> +jmp */
			size += 2;
		symtab[0].value += size + 2;
		sp++;
		return;
	}
	if (!getbr()) {                    /* short branch */
		dobranch();                    /* r2/r3 from our expres(); skip re-eval */
		return;
	}
	{
		int opcode = pop();
		push(r2);
		push(r3);
		if (opcode != 0400) {          /* invert condition + jmp */
			r2 = 0402 ^ opcode;
			r3 = 1;
			outw();
		}
		r3 = 1;
		r2 = 0000100 + 037;            /* jmp @(pc)+ */
		outw();
		r3 = pop();
		r2 = pop();
		outw();
	}
}

static void opl36(void) { opl35(); }   /* jeq, jne, ... share jbr */

static void opl15(void)                /* single operand */
{
	push(0);                           /* clr -(sp) */
	addres();                          /* operand already read by opline */
	/* (no swap for single) */
	*sp = swab16(*sp) >> 2;
	if ((unsigned)*sp >= (unsigned)rlimit)
		error('x');
	r2 |= pop();
	r2 |= pop();
	r3 = 0;
	outw();
	{
		int *p = adrbuf;
		while (p < adrp) {
			r2 = *p++;
			r3 = *p++;
			xsymbol = *p++;
			outw();
		}
	}
}

static void opl16(void)                /* .byte */
{
	expres();
	outb();
	if (r4 == ',') {
		readop();
		opl16();
		return;
	}
	sp++;
}

static void opl17(void)                /* .ascii */
{
	for (;;) {
		(void)getw();
		r3 = 1;
		r2 = r4;
		if ((int16_t)r4 < 0)
			break;
		r2 &= 0377;
		outb();
	}
	(void)getw();
}

static void opl20(void)                /* .even */
{
	if (symtab[0].value & 1) {
		if (symtab[0].type != 4) {
			r2 = 0;
			r3 = 0;
			outb();
		} else {
			symtab[0].value++;
		}
	}
	sp++;
}

static void opl21(void)                /* .if */
{
	expres();
	/* .endif (opl22) is a no-op here */
	sp++;
}

static void opl23(void)                /* .globl */
{
	for (;;) {
		if (r4 < 0200)
			break;
		((unsigned char *)r4)[0] |= 040;
		readop();
		if (r4 != ',')
			break;
		readop();
	}
	sp++;
}

static void section(int type)          /* .text/.data/.bss */
{
	symtab[0].value++;
	symtab[0].value &= ~1;
	savdot[symtab[0].type - 2] = symtab[0].value;          /* save current dot */
	if (passno) {
		flush(&txtp);
		flush(&relp);
		tseekp = &seeks[0] + (type - 025);
		oset(&txtp, *tseekp);
		rseekp = tseekp + 3;           /* seeks[3] is 3 entries on */
		oset(&relp, *rseekp);
	}
	symtab[0].value = savdot[type - 025];
	symtab[0].type = type - 023;
	sp++;                              /* tst (sp)+: pop the opcode */
}
static void opl25(void) { section(025); }
static void opl26(void) { section(026); }
static void opl27(void) { section(027); }

static void opl32(void)                /* .comm */
{
	if (r4 < 0200) {
		sp++;
		return;
	}
	{
		intptr_t sym = r4;
		readop();
		readop();
		expres();
		if (!(((unsigned char *)sym)[0] & 037)) {
			((unsigned char *)sym)[0] |= 040;
			*(uint16_t *)((unsigned char *)sym + 2) = r2;
		}
	}
	sp++;
}

static void opeof(void)                /* as26.s:69: read the filename record */
{
	int i = 0, count = 16;
	line = 1;
	for (;;) {
		if (getw())
			break;
		if ((int16_t)r4 < 0)
			break;
		argb[i++] = (char)r4;
		if (--count <= 0)
			i--;
	}
	argb[i++] = '\n';
	argb[i] = 0;
}

/* opline -- as26.s:6 */
static void opline(void)
{
	if (r4 >= 0 && r4 <= 0177) {       /* betwen 0; 177 */
		if (r4 == 5) { opeof(); return; }
		if (r4 == '<') { opl17(); return; }
		xpr();
		return;
	}
	{
		int type = ((unsigned char *)r4)[0];
		if (type == 024 || type == 033 || type == 034) {
			xpr();
			return;
		}
		if (type < 5 || type > 036) {
			xpr();
			return;
		}
		push(*(uint16_t *)((unsigned char *)r4 + 2));  /* mov 2(r4),-(sp) */
		adrp = adrbuf;                 /* mov $adrbuf,r5 */
		swapf = 0;
		rlimit = -1;
		readop();
		switch (type) {
		case 05:  opl5();  break;
		case 06:  opl6();  break;
		case 07:  opl7();  break;
		case 010: opl10(); break;
		case 011: opl11(); break;
		case 012: opl12(); break;
		case 013: opl13(); break;
		case 014: opl14(); break;
		case 015: opl15(); break;
		case 016: opl16(); break;
		case 017: opl17(); break;
		case 020: opl20(); break;
		case 021: opl21(); break;
		case 022: sp++;   break;      /* .endif no-op */
		case 023: opl23(); break;
		case 025: opl25(); break;
		case 026: opl26(); break;
		case 027: opl27(); break;
		case 030: opl30(); break;
		case 031: opl31(); break;
		case 032: opl32(); break;
		case 035: opl35(); break;
		case 036: opl36(); break;
		}
	}
}

/* ------------------------------------------------------------- main loop
 * assem (as23.s) -- the pass-2 line loop.
 */

static void assem(void);

static int checkeos(void)
{
	return !(r4 == '\n' || r4 == ';' || r4 == '\004');
}

static void ealoop(void)
{
	if (r4 == '\n')
		line++;
	else if (r4 == '\004')
		return;                        /* rts pc: end of input */
	assem();
}

static void assem(void)
{
	intptr_t label;

	readop();
	if (r4 == 5 || r4 == '<') {        /* opeof / string: process directly */
		opline();
		goto dotmax;
	}
	if (!checkeos()) {                 /* at end of line */
		ealoop();
		return;
	}
	label = r4;
	if (label == 1) {                  /* number: read its value */
		label = 2;
		(void)getw();
		numval = r4;
	}
	readop();                          /* the operator */
	if (r4 == '=') {                   /* assignment */
		readop();
		expres();
		if (label == (intptr_t)&symtab[0]) {   /* assigning to dot */
			r3 &= ~040;
			if (r3 != symtab[0].type) {
				error('.');
				goto ealoop_x;
			}
			if (r3 == 4) {             /* bss */
				symtab[0].value = r2;
				goto dotmax;
			}
			{
				int n = r2 - symtab[0].value;
				if (n < 0) {
					error('.');
					goto ealoop_x;
				}
				while (--n >= 0) {
					r2 = 0;
					r3 = 1;
					outb();
				}
			}
			goto dotmax;
		}
		/* normal assignment */
		if (r3 == 040)
			error('r');
		{
			unsigned char *p = (unsigned char *)label;
			p[0] &= ~037;
			r3 &= 037;
			if (r3 == 0)
				r2 = 0;
			p[0] |= r3;
			*(uint16_t *)(p + 2) = r2;
		}
		goto ealoop_x;
	}
	if (r4 == ':') {                   /* label definition */
		if (label >= 0200) {           /* symbol */
			unsigned char *p = (unsigned char *)label;
			if (passno) {
				if (*(uint16_t *)(p + 2) != symtab[0].value)
					error('p');        /* phase error */
			} else {
				int t = p[0] & 037;
				if (t != 0 && !(t >= 033 && t <= 034))
					error('m');
				p[0] &= ~037;
				p[0] |= symtab[0].type;
				brdelt = *(uint16_t *)(p + 2) - symtab[0].value;
				*(uint16_t *)(p + 2) = symtab[0].value;
			}
		} else if (label == 2) {       /* numeric label */
			r4 = numval;
			fbadv();
			{
				int p = curfb[numval];
				usymtab[p] = (unsigned char)symtab[0].type;
				brdelt = (usymtab[p + 2] | (usymtab[p + 3] << 8)) - symtab[0].value;
				usymtab[p + 2] = (unsigned char)(symtab[0].value & 0xFF);
				usymtab[p + 3] = (unsigned char)((symtab[0].value >> 8) & 0xFF);
			}
		} else {
			error('x');
		}
		assem();
		return;
	}
	/* ordinary line: operator */
	savop = r4;
	r4 = label;
	opline();
dotmax:
	if (!passno && symtab[0].value > sizes[symtab[0].type - 2])
		sizes[symtab[0].type - 2] = symtab[0].value;
	ealoop_x:
	ealoop();
}

/* ------------------------------------------------------------- driver
 * doreloc / setup (as21.s) -- relocate the symbol table, init the f-b table.
 */

static void doreloc(unsigned char *p)
{
	int t = p[0];
	if (t == 0)
		p[0] |= (unsigned char)defund;
	t &= 037;
	if (t >= 3 && t <= 4) {
		uint16_t v = p[2] | (p[3] << 8);
		v += (t == 3) ? datbase : bssbase;
		p[2] = (unsigned char)(v & 0xFF);
		p[3] = (unsigned char)(v >> 8);
	}
}

static void setup(void)
{
	int i;
	for (i = 0; i < 10; i++) {         /* curfb+40. == curfb[10] + (curfb + 10)[10] */
		curfb[i] = 0;
		(curfb + 10)[i] = 0;
	}
	fin = txtfil;
	ibufc = 0;
	for (i = 0; i < 10; i++) {
		r4 = i;
		fbadv();
	}
}

/* go (as21.s:16) -- the pass-2 driver.  Called from main() after start(). */
static void go(void)
{
	int off = 0, cap = 512, i;
	uint16_t *w;

	usymtab = malloc(cap);

	/* read the symbol table into usymtab (4-byte entries) */
	for (;;) {
		if (getw()) break;             /* name word 0 */
		sizes[3] += 12;
		getw(); getw(); getw(); getw();/* name words 1-3, type -> r4 */
		if (off + 8 >= cap)
			usymtab = realloc(usymtab, cap += 512);
		{
			int t = r4 & 037;
			w = (uint16_t *)(usymtab + off);
			if (t >= 2 && t <= 3) {
				w[0] = r4 + 031;       /* mark "estimated" */
				getw();
				w[1] = r4;
			} else {
				w[0] = 0;
				w[1] = 0;
				getw();
			}
		}
		off += 4;
	}
	symend = usymtab + off;

	/* read the f-b definitions */
	fbbufp = off;
	fin = fbfil;
	ibufc = 0;
	for (;;) {
		if (getw()) break;
		if (off + 8 >= cap)
			usymtab = realloc(usymtab, cap += 512);
		w = (uint16_t *)(usymtab + off);
		w[0] = r4 + 031;
		getw();
		w[1] = r4;
		off += 4;
	}
	endtable = off;
	if (off + 4 >= cap)
		usymtab = realloc(usymtab, cap += 512);
	*(uint16_t *)(usymtab + off) = 0100000;   /* end marker */
	symend = usymtab + off + 2;

	setup();
	assem();                           /* pass 1 */

	if (outmod != 0777)
		saexit();

	/* prepare for pass 2 */
	symtab[0].value = 0;
	symtab[0].type = 2;
	symtab[1].value = 0;
	brtabp = 0;
	close(fin);
	fin = (char)ofile(a_tmp1);
	ibufc = 0;
	setup();
	passno++;
	sizes[2] = (sizes[2] + 1) & ~1;
	sizes[0] = (sizes[0] + 1) & ~1;
	sizes[1] = (sizes[1] + 1) & ~1;
	datbase = sizes[0];
	savdot[1] = sizes[0];
	bssbase = sizes[0] + sizes[1];
	savdot[2] = bssbase;
	seeks[6] = 2 * (sizes[0] + sizes[1]) + 16;
	seeks[4] = seeks[6] - sizes[1];       /* = 2*sizes[0] + sizes[1] + 16 */
	seeks[3] = seeks[4] - sizes[0];      /* = sizes[0] + sizes[1] + 16 */
	seeks[1] = seeks[3] - sizes[1];       /* = sizes[0] + 16 */
	seeks[0] = 16;                      /* as28.s: seeks[0]: 0; 20 */

	for (i = 0; i < endtable; i += 4)
		doreloc(usymtab + i);

	oset(&txtp, 0);
	oset(&relp, seeks[3]);
	/* the a.out header (8 words): 0407 magic + sizes */
	putw(&txtp, 0407);
	putw(&txtp, sizes[0]);
	putw(&txtp, sizes[1]);
	putw(&txtp, sizes[2]);
	putw(&txtp, sizes[3]);
	putw(&txtp, sizes[4]);
	putw(&txtp, sizes[5]);
	putw(&txtp, 0);

	assem();                           /* pass 2 */

	flush(&txtp);
	flush(&relp);

	/* append the full symbol table (name from file, type/value relocated) */
	fin = symf;
	(void)lseek(fin, 0, 0);
	ibufc = 0;
	oset(&txtp, seeks[6]);
	i = 0;
	for (;;) {
		if (getw()) break;
		putw(&txtp, r4);
		getw(); putw(&txtp, r4);
		getw(); putw(&txtp, r4);
		getw(); putw(&txtp, r4);
		putw(&txtp, *(uint16_t *)(usymtab + i));
		putw(&txtp, *(uint16_t *)(usymtab + i + 2));
		getw(); getw();               /* discard the file's type/value */
		i += 4;
	}
	flush(&txtp);
	saexit();
}

int main(int argc, char **argv)
{
	char *r1;

	(void)signal(2, SIG_IGN);
	/* start() -- as29.s: parse args, open files */
	{
		int r0 = argc;                 /* mov (sp)+,r0 */
		char **sp = &argv[1];          /* tst (sp)+: skip argv[0] */
		for (;;) {
			r1 = *sp++;                /* 1: mov (sp)+,r1 */
			if (r1[0] != '-')          /* cmpb (r1),$'-; bne 1f */
				break;
			r0--;                      /* dec r0 */
			if (r1[1] == 'g') {        /* cmpb 1(r1),$'g; bne 2f */
				defund = 040;          /* mov $40,defund */
				continue;              /* br 1b */
			}
			if (r1[1] == 'o') {        /* 2: cmpb 1(r1),$'o; bne 1b */
				r0--;                  /* dec r0 */
				r1 = *sp++;            /* mov (sp)+,r1 */
				a_outp = r1;           /* mov r1,a.outp */
				continue;              /* br 1b */
			}
			/* unrecognised flag: skip it (br 1b) */
		}
		if (r0 < 4) {                  /* cmp r0,$4; bge 1f; jmp aexit */
			errflg = 1;
			saexit();
		}
		strcpy(a_tmp1, r1);            /* mov r1,a.tmp1 */
		strcpy(a_tmp2, *sp++);         /* mov (sp)+,a.tmp2 */
		strcpy(a_tmp3, *sp++);         /* mov (sp)+,a.tmp3 */
	}
	txtfil = (char)ofile(a_tmp1);
	fbfil = (char)ofile(a_tmp2);
	symf = (char)ofile(a_tmp3);
	fin = symf;
	{
		int fd = creat(a_outp, 0666);
		if (fd < 0)
			filerr(a_outp);
		fout = (char)fd;
	}
	/* go: -- as21.s */
	go();
	return 0;
}
