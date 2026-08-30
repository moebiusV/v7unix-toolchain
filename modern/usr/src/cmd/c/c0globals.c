/*
 * c0 global state.
 *
 * V7 kept these as tentative (common) definitions in c0.h; C99 has no common
 * symbols, so they live here as the single definition.  c0.h declares them
 * `extern`.  (isn, peeksym, line, funcblk are in c00.c; opdope/cvtab/cvntab/
 * ctab are in c05.c.)
 */

#include "c0.h"

char	filename[64];
char	symbuf[NCPS+2];
int16_t	hshused;
struct node	hshtab[HSHSIZ];
struct node **cp;
struct swtab	swtab[SWSIZ];
struct swtab	*swp;
int16_t	contlab;
int16_t	brklab;
int16_t	retlab;
int16_t	deflab;
unsigned autolen;
unsigned maxauto;
int16_t	peekc;
int16_t	eof;
char	*funcbase;
char	*curbase;
char	*coremax;
char	*maxdecl;
struct node	*defsym;
struct node	*funcsym;
int16_t	proflg;
struct node	*csym;
int16_t	cval;
int32_t	lcval;
int16_t	nchstr;
int16_t	nerror;
struct node	*paraml;
struct node	*parame;
int16_t	strflg;
int16_t	mosflg;
int16_t	initflg;
int16_t	inhdr;
char	sbuf[BUFSIZ];
FILE	*sbufp;
int16_t	regvar;
int16_t	bitoffs;
char	numbuf[64];
struct node **memlist;
int16_t	nmems;
struct node	structhole;
int16_t	blklev;
int16_t	mossym;
