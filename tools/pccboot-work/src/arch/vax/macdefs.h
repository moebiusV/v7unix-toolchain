/*	$Id$	*/
/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code and documentation must retain the above
 * copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditionsand the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 * 	This product includes software developed or owned by Caldera
 *	International, Inc.
 * Neither the name of Caldera International, Inc. nor the names of other
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OFLIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

# define makecc(val,i)  lastcon = (lastcon<<8)|((val<<24)>>24);  

# define  ARGINIT 32 
# define  AUTOINIT 0 
# define  SZCHAR 8
# define  SZBOOL 8
# define  SZINT 32
# define  SZFLOAT 32
# define  SZDOUBLE 64
# define  SZLDOUBLE 64	/* XXX use longer? */
# define  SZLONG 32
# define  SZLONGLONG 64
# define  SZSHORT 16
# define SZPOINT(t) 32
# define ALCHAR 8
# define ALBOOL 8
# define ALINT 32
# define ALFLOAT 32
# define ALDOUBLE 32
# define ALLDOUBLE 32
# define ALLONG 32
# define ALLONGLONG 32
# define ALSHORT 16
# define ALPOINT 32
# define ALSTRUCT 8
# define  ALSTACK 32 
#define MYVAARGSZ SZINT

/*
 * Min/max values.
 */
#define MIN_CHAR        -128
#define MAX_CHAR        127
#define MAX_UCHAR       255
#define MIN_SHORT       -32768
#define MAX_SHORT       32767
#define MAX_USHORT      65535
#define MIN_INT         (-0x7fffffff-1)
#define MAX_INT         0x7fffffff
#define MAX_UNSIGNED    0xffffffff
#define MIN_LONG        MIN_INT
#define MAX_LONG        MAX_INT
#define MAX_ULONG       MAX_UNSIGNED
#define MIN_LONGLONG    0x8000000000000000LL
#define MAX_LONGLONG    0x7fffffffffffffffLL
#define MAX_ULONGLONG   0xffffffffffffffffULL

/* Default char is signed */
#undef  CHAR_UNSIGNED
#define BOOL_TYPE       CHAR    /* what used to store _Bool */
#define	HASP2ALIGN
/*	size in which constants are converted */
/*	should be long if feasable */

typedef long long CONSZ;
typedef unsigned long long U_CONSZ;

# define CONFMT "%lld"
/*
 * VAX_AS_32V: write the dialect of the V7 assembler for the VAX (32V's as),
 * which is what this target gets.  It has no % on registers, labels are L%d,
 * C names have a _ in front, there are no ELF directives, and strings are
 * .byte lists (the assembler has no .ascii).
 */
#if defined(os_unix32v)
# define VAX_AS_32V
#endif

#if defined(VAX_AS_32V)
# define LABFMT "L%d"
# define STABLBL "LL%d"
# define REGPFX ""
# define SPN "sp"
# define MYINSTRING
#else
# define LABFMT ".L%d"
# define STABLBL ".LL%d"
# define REGPFX "%%"
# define SPN "%sp"
#endif

/*	size in which offsets are kept
 *	should be large enough to cover address space in bits
 */
typedef long long OFFSZ;

/* register cookie for stack poINTer */


/* show stack grows negatively */
#define STACK_DOWN

/* show field hardware support on VAX */
/* XXX notyet */
#undef FIELDOPS

/* bytes are numbered from right to left */
#define TARGET_ENDIAN TARGET_LE
#define	UNALIGNED_ACCESS

/* we want prtree included */
# define STDPRTREE

/*	VAX-11/780 Registers */

	/* scratch registers */
# define R0 0
# define R1 1
# define R2 2
# define R3 3
# define R4 4
# define R5 5

	/* register variables */
# define R6 6
# define R7 7
# define R8 8
# define R9 9
# define R10 10
# define R11 11

	/* special purpose */
# define AP 12		/* argument pointer */
# define FP 13		/* frame pointer */
# define SP 14	/* stack pointer */
# define PC 15	/* program counter */

	/* floating registers */

	/* there are no floating point registers on the VAX */
	/* but there are concatenated regs */
	/* we call them XR? */
#define	XR0	16
#define	XR1	17
#define	XR2	18
#define	XR3	19
#define	XR4	20
#define	XR5	21
#define	XR6	22
#define	XR7	23
#define	XR8	24
#define	XR9	25
#define	XR10	26




extern int fregs;
extern int maxargs;

# define BYTEOFF(x) ((x)&03)
# define wdal(k) (BYTEOFF(k)==0)

# define REGSZ 16

# define TMPREG FP

//# define R2REGS   /* permit double indexing */
#define HELP_UDIV	1	/* the file calls __udiv */
#define HELP_UREM	2	/* the file calls __urem */
extern int vax_helpers;
#define SAVEREGS_BY_MASK	/* callee-saved registers in use go in the entry mask, not copies */
#define REGVARS		6	/* 'register' variables per function: r11 down to r6 */
#define REGVAR0		R11	/* first one; the next go downwards */
#define AUTOINC		/* p++ and --p of a register pointer: (rN)+ and -(rN) */
#define AUTOINC_SIZE(s)	((s) == SZCHAR || (s) == SZSHORT || (s) == SZINT || \
			(s) == SZLONG || (s) == SZFLOAT || (s) == SZDOUBLE)
#define NOBASE		100	/* R2PACK base: none, name[rI] */
#define NOINDEX		0177	/* R2PACK index: none, (rN)+ */
#define R2NOINDEX	NOINDEX	/* regs.c: an R2PACK index of this value is no register */
#define RINCR		2	/* R2PACK flag: (rN)+ */
#define RDECR		4	/* R2PACK flag: -(rN) */

# define STOARG(p)     /* just evaluate the arguments, and be done with it... */
# define STOFARG(p)
# define STOSTARG(p)
# define genfcall(a,b) gencall(a,b)

# define NESTCALL

/*
 * Register allocator stuff.
 * The register allocator sees this as 16 general regs (AREGs)
 * and 11 64-bit concatenated regs. (BREGs)
 */
#define MAXREGS 033     /* 27 registers */

#define RSTATUS \
        SAREG|TEMPREG, SAREG|TEMPREG, SAREG|TEMPREG, SAREG|TEMPREG,     \
        SAREG|TEMPREG, SAREG|TEMPREG, SAREG|PERMREG, SAREG|PERMREG,	\
        SAREG|PERMREG, SAREG|PERMREG, SAREG|PERMREG, SAREG|PERMREG,	\
	0, 0, 0, 0, /* do not care about ap, fp, sp or pc */		\
	SBREG, SBREG, SBREG, SBREG, SBREG, SBREG, SBREG, SBREG,		\
	SBREG, SBREG, SBREG,

#define ROVERLAP \
	{ XR0, -1 },			\
	{ XR0, XR1, -1 },		\
	{ XR1, XR2, -1 },		\
	{ XR2, XR3, -1 },		\
	{ XR3, XR4, -1 },		\
	{ XR4, XR5, -1 },		\
	{ XR5, XR6, -1 },		\
	{ XR6, XR7, -1 },		\
	{ XR7, XR8, -1 },		\
	{ XR8, XR9, -1 },		\
	{ XR9, XR10, -1 },		\
	{ XR10, -1 },			\
	{ -1 },				\
	{ -1 },				\
	{ -1 },				\
	{ -1 },				\
	{ R0, R1, XR1, -1 },		\
	{ R1, R2, XR0, XR2, -1 },	\
	{ R2, R3, XR1, XR3, -1 },	\
	{ R3, R4, XR2, XR4, -1 },	\
	{ R4, R5, XR3, XR5, -1 },	\
	{ R5, R6, XR4, XR6, -1 },	\
	{ R6, R7, XR5, XR7, -1 },	\
	{ R7, R8, XR6, XR8, -1 },	\
	{ R8, R9, XR7, XR9, -1 },	\
	{ R9, R10, XR8, XR10, -1 },	\
	{ R10, R11, XR9, -1 },

#define NUMCLASS        2       /* highest number of reg classes used */

/* size, in registers, needed to hold thing of type t */
#define	szty(t)	(((t) == DOUBLE || (t) == LONGLONG || (t) == ULONGLONG) ? 2 : 1)
#define MEM_DEFERRED 1	/* deferred modes: *p with p in memory (optim2.c postincfold) */
#define FPREG	FP	/* frame pointer */
#define STKREG	SP
#define ARGREG	AP

#define DECRA(x,y)      (((x) >> (y*6)) & 63)   /* decode encoded regs */
#define ENCRD(x)        (x)             /* Encode dest reg in n_reg */
#define ENCRA1(x)       ((x) << 6)      /* A1 */
#define ENCRA2(x)       ((x) << 12)     /* A2 */
#define ENCRA(x,y)      ((x) << (6+y*6))        /* encode regs in int */

#define PCLASS(p)	(szty(p->n_type) == 2 ? SBREG : SAREG)
#define RETREG(x)	(szty(x) == 2 ? XR0 : R0)
#define GCLASS(x)	(x < XR0 ? CLASSA : CLASSB)
int xasmconstregs(char *s);
#define XASMCONSTREGS(x) xasmconstregs(x)
int COLORMAP(int c, int *r);

#define	SNCON		(MAXSPECIAL+1)	/* named constand */
#define	QADDR		0x4010	/* n_qual of a quad UMUL that is only an address (movaq) */
#define	SSTRSRC		(MAXSPECIAL+4)	/* &name or &frame-object: a struct copy reads it in place (-xdmr) */
#define	SCVTARG		(MAXSPECIAL+3)	/* char/short in memory widened as a call argument: cvtbl m,-(sp) (-xdmr) */
#define	SADDRN		(MAXSPECIAL+2)	/* &name: an address constant, printed $name+off (-xdmr) */

#define TARGET_FLT_EVAL_METHOD	0	/* all as their type */
/*
 * Builtins.
 */
#ifdef LANG_CXX
#define P1ND struct node
#else
#define P1ND struct p1node
#endif
struct p1node;
struct bitable;

#define	TARGET_FFS		/* target-specific ffs */
P1ND *builtin_ffs(const struct bitable *, P1ND *a);
P1ND *builtin_ffsl(const struct bitable *, P1ND *a);
P1ND *builtin_ffsll(const struct bitable *, P1ND *a);
#undef P1ND

/* floating point definitions */
#define FDFLOAT
#define DEFAULT_FPI_DEFS { &fpi_ffloat, &fpi_dfloat, &fpi_dfloat }

