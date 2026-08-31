#
/*
 *	UNIX shell — S. R. Bourne
 *
 *	Global data definitions.  V7 relied on tentative definitions (common
 *	symbols) for these; a modern linker needs each one defined exactly once.
 *	The strings (msg.c), name nodes (name.c) and a few initialised globals
 *	(main.c/blok.c/stak.c/fault.c/print.c/args.c) live elsewhere; everything
 *	without an initialiser is here.
 */

#include	"defs.h"

INT		ioset;
IOPTR		iotemp;		/* files to be deleted sometime */
IOPTR		iopend;		/* documents waiting to be read at NL */

INT		dolc;
STRING		*dolv;
DOLPTR		argfor;
ARGPTR		gchain;

INT		wdval;
INT		wdnum;
ARGPTR		wdarg;
INT		wdset;
BOOL		reserv;

STRING		cmdadr;
STRING		exitadr;
STRING		dolladr;
STRING		pcsadr;
STRING		pidadr;

STRING		tmpnam;
INT		serial;
INT		peekc;
STRING		comdiv;

INT		flags;

jmp_buf		subshell;
jmp_buf		errshell;

BOOL		trapnote;

INT		exitval;
BOOL		execbrk;
INT		loopcnt;
INT		breakcnt;

BLKPTR		stakbsy;
STKPTR		stakbas;
STKPTR		brkend;
STKPTR		staktop;

char		*end;
