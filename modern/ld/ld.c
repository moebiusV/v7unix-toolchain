#define _DEFAULT_SOURCE 1
/*
 *  link editor
 */

#include <signal.h>
#include "sys/types.h"
#include "sys/stat.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef V7_LIBDIR
#define V7_LIBDIR "/usr/local/lib/v7unix"
#endif

/*	Layout of a.out file :
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
enum {
	TRUE = 1,
	FALSE = 0,
};


enum {
	ARCMAGIC = 0177545,
	OMAGIC = 0405,
	FMAGIC = 0407,
	NMAGIC = 0410,
	IMAGIC = 0411,
};

enum {
	EXTERN = 040,
	UNDEF = 00,
	ABS = 01,
	TEXT = 02,
	DATA = 03,
	BSS = 04,
	COMM = 05,	/* internal use only */
};

enum {
	RABS = 00,
	RTEXT = 02,
	RDATA = 04,
	RBSS = 06,
	REXT = 010,
};

enum {
	NOVLY = 16,
	RELFLG = 01,
	NROUT = 256,
	NSYM = 1103,
	NSYMPR = 1000,
};

char	premeof[] = "Premature EOF";
char	goodnm[] = "__.SYMDEF";

/* table of contents stuff */
enum {
	TABSZ = 700,
};
struct tab
{	char cname[8];
	int16_t cloc[2];	/* middle-endian: cloc[0]=high word, cloc[1]=low word */
} tab[TABSZ];
int16_t tnum;


/* overlay management */
int16_t	vindex;
struct overlay {
	int16_t	argsav;
	int16_t	symsav;
	struct liblist	*libsav;
	char	*vname;
	int16_t	ctsav, cdsav, cbsav;
	int16_t	offt, offd, offb, offs;
} vnodes[NOVLY];

/* input management */
struct page {
	int16_t	nuser;
	int16_t	bno;
	int16_t	nibuf;
	int16_t	buff[256];
} page[2];

struct {
	int16_t	nuser;
	int16_t	bno;
} fpage;

struct stream {
	int16_t	*ptr;
	int16_t	bno;
	int16_t	nibuf;
	int16_t	size;
	struct page	*pno;
};

struct stream text;
struct stream reloc;

struct {
	char	aname[14];
	int16_t	atime[2];	/* middle-endian: atime[0]=high word, atime[1]=low */
	char	auid, agid;
	int16_t	amode;
	int16_t	asize[2];	/* middle-endian: asize[0]=high word, asize[1]=low */
} archdr;

struct {
	int16_t	fmagic;
	int16_t	tsize;
	int16_t	dsize;
	int16_t	bsize;
	int16_t	ssize;
	int16_t	entry;
	int16_t	pad;
	int16_t	relflg;
} filhdr;

/* host-port: the PDP-11 stores 32-bit values middle-endian -- the high
 * 16-bit word first -- so a little-endian host reads the two words in the
 * opposite order.  File-format structs (archdr, tab) keep such fields as
 * two 16-bit words and reassemble them with mkl().  */
int32_t mkl(int16_t *w)
{
	return (int32_t)(((uint32_t)(uint16_t)w[0] << 16) | (uint16_t)w[1]);
}


/* one entry for each archive member referenced;
 * set in first pass; needs restoring for overlays
 */
struct liblist {
	int32_t	loc;
};

struct liblist	liblist[NROUT];
struct liblist	*libp = liblist;


/* symbol management */
struct symbol {
	char	sname[8];
	char	stype;
	char	spare;
	int16_t	svalue;
};

struct local {
	int16_t locindex;		/* index to symbol in file */
	struct symbol *locsymbol;	/* ptr to symbol table */
};

struct symbol	cursym;			/* current symbol */
struct symbol	symtab[NSYM];		/* actual symbols */
struct symbol	**symhash[NSYM];	/* ptr to hash table entry */
struct symbol	*lastsym;		/* last symbol entered */
int16_t	symindex;		/* next available symbol table entry */
struct symbol	*hshtab[NSYM+2];	/* hash table for symbols */
struct local	local[NSYMPR];

/* internal symbols */
struct symbol	*p_etext;
struct symbol	*p_edata;
struct symbol	*p_end;
struct symbol	*entrypt;

int16_t	trace;
/* flags */
int16_t	xflag;		/* discard local symbols */
int16_t	Xflag;		/* discard locals starting with 'L' */
int16_t	Sflag;		/* discard all except locals and globals*/
int16_t	rflag;		/* preserve relocation bits, don't define common */
int16_t	arflag;		/* original copy of rflag */
int16_t	sflag;		/* discard all symbols */
int16_t	nflag;		/* pure procedure */
int16_t	Oflag;		/* set magic # to 0405 (overlay) */
int16_t	dflag;		/* define common even with rflag */
int16_t	iflag;		/* I/D space separated */
int16_t	vflag;		/* overlays used */

int16_t	ofilfnd;
char	*ofilename = "l.out";
int16_t	infil;
char	*filname;
/* Host port: -L search dirs (V7's ld had none — it hardcoded /lib and
   /usr/lib).  Filled during pass 1, used by getfile() to resolve -lname. */
enum {
	NLIBDIR = 16,
};
char	*libdirs[NLIBDIR];
int	nlibdir;

/* cumulative sizes set in pass 1 */
int16_t	tsize;
int16_t	dsize;
int16_t	bsize;
int16_t	ssize;

/* symbol relocation; both passes */
int16_t	ctrel;
int16_t	cdrel;
int16_t	cbrel;

int16_t	errlev;
int16_t	delarg	= 4;
char	tfname[] = "/tmp/ldaXXXXXX";


/* output management */
struct buf {
	int16_t	fildes;
	int16_t	nleft;
	int16_t	*xnext;
	int16_t	iobuf[256];
};
struct buf	toutb;
struct buf	doutb;
struct buf	troutb;
struct buf	droutb;
struct buf	soutb;


int16_t delexit(void)
{
	unlink("l.out");
	if (delarg==0)
		chmod(ofilename, 0777 & ~umask(0));
	exit(delarg);
}

int16_t copy(struct buf *buf);
int16_t cp8c(char *from, char *to);
int16_t dseek(struct stream *asp, int32_t aloc, int16_t s);
int16_t endload(int16_t argc, char **argv);
int16_t enter(struct symbol **hp);
int16_t eq(char *s1, char *s2);
int16_t error(int16_t n, char *s);
int16_t finishout(void);
int16_t flush(struct buf *b);
int16_t get(struct stream *asp);
int16_t getfile(char *acp);
int16_t half(int16_t i);
int16_t ldrand(void);
void ldrsym(struct symbol *asp, int16_t val, int16_t type);
int16_t load1(int16_t libflg, int32_t loc);
int16_t load1arg(char *acp);
int16_t load2(int32_t loc);
int16_t load2arg(char *acp);
int16_t load2td(struct local *lp, int16_t creloc, struct buf *b1, struct buf *b2);
struct symbol * lookloc(struct local *alp, int16_t r);
struct symbol * * lookup(void);
void mget(int16_t *aloc, int16_t an);
int16_t middle(void);
void mkfsym(char *s);
int16_t mput(struct buf *buf, int16_t *aloc, int16_t an);
int16_t ld_putw(int16_t w, struct buf *b);
int16_t readhdr(int32_t loc);
int16_t record(int16_t c, char *nam);
int16_t restore(int16_t vscan);
int16_t setupout(void);
struct symbol * * slookup(char *s);
int16_t step(int32_t nloc);
void symreloc(void);
int16_t tcreat(struct buf *buf, int16_t tempflg);

int16_t main(int16_t argc, char **argv)
{
	register int16_t c, i; 
	int16_t num;
	register char *ap, **p;
	int16_t found; 
	int16_t vscan; 
	char save;

	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, (void (*)(int))delexit);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, (void (*)(int))delexit);
	if (argc == 1)
		exit(4);
	p = argv+1;

	/* scan files once to find symdefs */
	for (c=1; c<argc; c++) {
		if (trace) printf("%s:\n", *p);
		filname = 0;
		ap = *p++;

		if (*ap == '-') {
			for (i=1; ap[i]; i++) {
			switch (ap[i]) {
			case 'o':
				if (++c >= argc)
					error(2, "Bad output file");
				ofilename = *p++;
				ofilfnd++;
				continue;

			case 'u':
			case 'e':
				if (++c >= argc)
					error(2, "Bad 'use' or 'entry'");
				enter(slookup(*p++));
				if (ap[i]=='e')
					entrypt = lastsym;
				continue;

			case 'v':
				if (++c >= argc)
					error(2, "-v: arg missing");
				vflag=TRUE;
				vscan = vindex; 
				found=FALSE;
				while (--vscan>=0 && found==FALSE)
					found = eq(vnodes[vscan].vname, *p);
				if (found) {
					endload(c, argv);
					restore(vscan);
				} else
					record(c, *p);
				p++;
				continue;

			case 'D':
				if (++c >= argc)
					error(2, "-D: arg missing");
				num = atoi(*p++);
				if (dsize>num)
					error(2, "-D: too small");
				dsize = num;
				continue;

			case 'L':
				if (++c >= argc)
					error(2, "-L: arg missing");
				if (nlibdir < NLIBDIR)
					libdirs[nlibdir++] = *p++;
				else
					p++;
				continue;

			case 'l':
				save = ap[--i]; 
				ap[i]='-';
				load1arg(&ap[i]); 
				ap[i]=save;
				break;

			case 'x':
				xflag++;
				continue;

			case 'X':
				Xflag++;
				continue;

			case 'S':
				Sflag++; 
				continue;

			case 'r':
				rflag++;
				arflag++;
				continue;

			case 's':
				sflag++;
				xflag++;
				continue;

			case 'n':
				nflag++;
				continue;

			case 'd':
				dflag++;
				continue;

			case 'i':
				iflag++;
				continue;

			case 'O':
				Oflag++;
				continue;

			case 't':
				trace++;
				continue;

			default:
				error(2, "bad flag");
			} /*endsw*/
			break;
			} /*endfor*/
		} else
			load1arg(ap);
	}
	endload(argc, argv);
}

/* used after pass 1 */
int16_t	nsym;
int16_t	torigin;
int16_t	dorigin;
int16_t	borigin;

int16_t endload(int16_t argc, char **argv)
{
	register int16_t c, i; 
	int16_t dnum;
	register char *ap, **p;
	filname = 0;
	middle();
	setupout();
	p = argv+1;
	libp = liblist;
	for (c=1; c<argc; c++) {
		ap = *p++;
		if (trace) printf("%s:\n", ap);
		if (*ap == '-') {
			for (i=1; ap[i]; i++) {
			switch (ap[i]) {
			case 'D':
				for (dnum = atoi(*p); dorigin<dnum; dorigin += 2) {
					ld_putw(0, &doutb);
					if (rflag)
						ld_putw(0, &droutb);
				}
			case 'L':
			case 'u':
			case 'e':
			case 'o':
			case 'v':
				++c;
				++p;

			default:
				continue;

			case 'l':
				ap[--i]='-'; 
				load2arg(&ap[i]);
				break;
			} /*endsw*/
			break;
			} /*endfor*/
		} else
			load2arg(ap);
	}
	finishout();
}

int16_t record(int16_t c, char *nam)
{
	register struct overlay *v;

	v = &vnodes[vindex++];
	v->argsav = c;
	v->symsav = symindex;
	v->libsav = libp;
	v->vname = nam;
	v->offt = tsize; 
	v->offd = dsize; 
	v->offb = bsize; 
	v->offs = ssize;
	v->ctsav = ctrel; 
	v->cdsav = cdrel; 
	v->cbsav = cbrel;
}

int16_t restore(int16_t vscan)
{
	register struct overlay *v;
	register int16_t saved;

	v = &vnodes[vscan];
	vindex = vscan+1;
	libp = v->libsav;
	ctrel = v->ctsav; 
	cdrel = v->cdsav; 
	cbrel = v->cbsav;
	tsize = v->offt; 
	dsize = v->offd; 
	bsize = v->offb; 
	ssize = v->offs;
	saved = v->symsav;
	while (symindex>saved)
		*symhash[--symindex]=0;
}

/* scan file to find defined symbols */
int16_t load1arg(char *acp)
{
	register char *cp;
	int32_t nloc;

	cp = acp;
	switch ( getfile(cp)) {
	case 0:
		load1(0, 0L);
		break;

	/* regular archive */
	case 1:
		nloc = 1;
		while ( step(nloc))
			nloc += (mkl(archdr.asize) + sizeof(archdr) + 1) >> 1;
		break;

	/* table of contents */
	case 2:
		tnum = mkl(archdr.asize) / sizeof(struct tab);
		if (tnum >= TABSZ) {
			error(2, "fast load buffer too small");
		}
		lseek(infil, (int32_t)(sizeof(filhdr.fmagic)+sizeof(archdr)), 0);
		read(infil, (char *)tab, tnum * sizeof(struct tab));
		while (ldrand());
		libp->loc = -1;
		libp++;
		break;
	/* out of date table of contents */
	case 3:
		error(0, "out of date (warning)");
		for(nloc = 1+((mkl(archdr.asize)+sizeof(archdr)+1) >> 1); step(nloc);
			nloc += (mkl(archdr.asize) + sizeof(archdr) + 1) >> 1);
		break;
	}
	close(infil);
}

int16_t step(int32_t nloc)
{
	dseek(&text, nloc, sizeof archdr);
	if (text.size <= 0) {
		libp->loc = -1;
		libp++;
		return(0);
	}
	mget((int16_t *)&archdr, sizeof archdr);
	if (load1(1, nloc + (sizeof archdr) / 2)) {
		libp->loc = nloc;
		libp++;
	}
	return(1);
}

int16_t ldrand(void)
{
	int16_t i;
	struct symbol *sp, **pp;
	struct liblist *oldp = libp;
	for(i = 0; i<tnum; i++) {
		if ((pp = slookup(tab[i].cname)) == 0)
			continue;
		sp = *pp;
		if (sp->stype != EXTERN+UNDEF)
			continue;
		step(mkl(tab[i].cloc) >> 1);
	}
	return(oldp != libp);
}

int16_t add(int16_t a, int16_t b, char *s)
{
	int32_t r;

	r = (int32_t)(unsigned)a + (unsigned)b;
	if (r >= 0200000)
		error(1,s);
	return(r);
}


/* single file or archive member */
int16_t load1(int16_t libflg, int32_t loc)
{
	register struct symbol *sp;
	int16_t savindex;
	int16_t ndef, nloc, type, mtype;

	readhdr(loc);
	ctrel = tsize;
	cdrel += dsize;
	cbrel += bsize;
	ndef = 0;
	nloc = sizeof cursym;
	savindex = symindex;
	if ((filhdr.relflg&RELFLG)==1) {
		error(1, "No relocation bits");
		return(0);
	}
	loc += (sizeof filhdr)/2 + filhdr.tsize + filhdr.dsize;
	dseek(&text, loc, filhdr.ssize);
	while (text.size > 0) {
		mget((int16_t *)&cursym, sizeof cursym);
		type = cursym.stype;
		if (Sflag) {
			mtype = type&037;
			if (mtype==1 || mtype>4) {
				continue;
			}
		}
		if ((type&EXTERN)==0) {
			if (Xflag==0 || cursym.sname[0]!='L')
				nloc += sizeof cursym;
			continue;
		}
		symreloc();
		if (enter(lookup()))
			continue;
		if ((sp = lastsym)->stype != EXTERN+UNDEF)
			continue;
		if (cursym.stype == EXTERN+UNDEF) {
			if (cursym.svalue > sp->svalue)
				sp->svalue = cursym.svalue;
			continue;
		}
		if (sp->svalue != 0 && cursym.stype == EXTERN+TEXT)
			continue;
		ndef++;
		sp->stype = cursym.stype;
		sp->svalue = cursym.svalue;
	}
	if (libflg==0 || ndef) {
		tsize = add(tsize,filhdr.tsize,"text overflow");
		dsize = add(dsize,filhdr.dsize,"data overflow");
		bsize = add(bsize,filhdr.bsize,"bss overflow");
		ssize = add(ssize,nloc,"symbol table overflow");
		return(1);
	}
	/*
	 * No symbols defined by this library member.
	 * Rip out the hash table entries and reset the symbol table.
	 */
	while (symindex>savindex)
		*symhash[--symindex]=0;
	return(0);
}

int16_t middle(void)
{
	register struct symbol *sp, *symp;
	register int16_t t, csize;
	int16_t nund, corigin;

	torigin=0; 
	dorigin=0; 
	borigin=0;

	p_etext = *slookup("_etext");
	p_edata = *slookup("_edata");
	p_end = *slookup("_end");
	/*
	 * If there are any undefined symbols, save the relocation bits.
	 */
	symp = &symtab[symindex];
	if (rflag==0) {
		for (sp = symtab; sp<symp; sp++)
			if (sp->stype==EXTERN+UNDEF && sp->svalue==0
				&& sp!=p_end && sp!=p_edata && sp!=p_etext) {
				rflag++;
				dflag = 0;
				break;
			}
	}
	if (rflag)
		nflag = sflag = iflag = Oflag = 0;
	/*
	 * Assign common locations.
	 */
	csize = 0;
	if (dflag || rflag==0) {
		ldrsym(p_etext, tsize, EXTERN+TEXT);
		ldrsym(p_edata, dsize, EXTERN+DATA);
		ldrsym(p_end, bsize, EXTERN+BSS);
		for (sp = symtab; sp<symp; sp++)
			if (sp->stype==EXTERN+UNDEF && (t = sp->svalue)!=0) {
				t = (t+1) & ~01;
				sp->svalue = csize;
				sp->stype = EXTERN+COMM;
				csize = add(csize, t, "bss overflow");
			}
	}
	/*
	 * Now set symbols to their final value
	 */
	if (nflag || iflag)
		tsize = (tsize + 077) & ~077;
	dorigin = tsize;
	if (nflag)
		dorigin = (tsize+017777) & ~017777;
	if (iflag)
		dorigin = 0;
	corigin = dorigin + dsize;
	borigin = corigin + csize;
	nund = 0;
	for (sp = symtab; sp<symp; sp++) switch (sp->stype) {
	case EXTERN+UNDEF:
		errlev |= 01;
		if (arflag==0 && sp->svalue==0) {
			if (nund==0)
				printf("Undefined:\n");
			nund++;
			printf("%.8s\n", sp->sname);
		}
		continue;

	case EXTERN+ABS:
	default:
		continue;

	case EXTERN+TEXT:
		sp->svalue += torigin;
		continue;

	case EXTERN+DATA:
		sp->svalue += dorigin;
		continue;

	case EXTERN+BSS:
		sp->svalue += borigin;
		continue;

	case EXTERN+COMM:
		sp->stype = EXTERN+BSS;
		sp->svalue += corigin;
		continue;
	}
	if (sflag || xflag)
		ssize = 0;
	bsize = add(bsize, csize, "bss overflow");
	nsym = ssize / (sizeof cursym);
}

void ldrsym(struct symbol *asp, int16_t val, int16_t type)
{
	register struct symbol *sp;

	if ((sp = asp) == 0)
		return;
	if (sp->stype != EXTERN+UNDEF || sp->svalue) {
		printf("%.8s: ", sp->sname);
		error(1, "Multiply defined");
		return;
	}
	sp->stype = type;
	sp->svalue = val;
}

int16_t setupout(void)
{
	tcreat(&toutb, 0);
	mktemp(tfname);
	tcreat(&doutb, 1);
	if (sflag==0 || xflag==0)
		tcreat(&soutb, 1);
	if (rflag) {
		tcreat(&troutb, 1);
		tcreat(&droutb, 1);
	}
	filhdr.fmagic = (Oflag ? OMAGIC :( iflag ? IMAGIC : ( nflag ? NMAGIC : FMAGIC )));
	filhdr.tsize = tsize;
	filhdr.dsize = dsize;
	filhdr.bsize = bsize;
	filhdr.ssize = sflag? 0: (ssize + (sizeof cursym)*symindex);
	if (entrypt) {
		if (entrypt->stype!=EXTERN+TEXT)
			error(1, "Entry point not in text");
		else
			filhdr.entry = entrypt->svalue | 01;
	} else
		filhdr.entry=0;
	filhdr.pad = 0;
	filhdr.relflg = (rflag==0);
	mput(&toutb, (int16_t *)&filhdr, sizeof filhdr);
}

int16_t tcreat(struct buf *buf, int16_t tempflg)
{
	register int16_t ufd; 
	char *nam;
	nam = (tempflg ? tfname : ofilename);
	if ((ufd = creat(nam, 0666)) < 0)
		error(2, tempflg?"cannot create temp":"cannot create output");
	close(ufd); 
	buf->fildes = open(nam, 2);
	if (tempflg)
		unlink(tfname);
	buf->nleft = sizeof(buf->iobuf)/sizeof(int16_t);
	buf->xnext = buf->iobuf;
}

int16_t load2arg(char *acp)
{
	register char *cp;
	register struct liblist *lp;

	cp = acp;
	if (getfile(cp) == 0) {
		while (*cp)
			cp++;
		while (cp >= acp && *--cp != '/');
		mkfsym(++cp);
		load2(0L);
	} else {	/* scan archive members referenced */
		for (lp = libp; lp->loc != -1; lp++) {
			dseek(&text, lp->loc, sizeof archdr);
			mget((int16_t *)&archdr, sizeof archdr);
			mkfsym(archdr.aname);
			load2(lp->loc + (sizeof archdr) / 2);
		}
		libp = ++lp;
	}
	close(infil);
}

int16_t load2(int32_t loc)
{
	register struct symbol *sp;
	register struct local *lp;
	register int16_t symno;
	int16_t type, mtype;

	readhdr(loc);
	ctrel = torigin;
	cdrel += dorigin;
	cbrel += borigin;
	/*
	 * Reread the symbol table, recording the numbering
	 * of symbols for fixing external references.
	 */
	lp = local;
	symno = -1;
	loc += (sizeof filhdr)/2;
	dseek(&text, loc + filhdr.tsize + filhdr.dsize, filhdr.ssize);
	while (text.size > 0) {
		symno++;
		mget((int16_t *)&cursym, sizeof cursym);
		symreloc();
		type = cursym.stype;
		if (Sflag) {
			mtype = type&037;
			if (mtype==1 || mtype>4) continue;
		}
		if ((type&EXTERN) == 0) {
			if (!sflag&&!xflag&&(!Xflag||cursym.sname[0]!='L'))
				mput(&soutb, (int16_t *)&cursym, sizeof cursym);
			continue;
		}
		if ((sp = *lookup()) == 0)
			error(2, "internal error: symbol not found");
		if (cursym.stype == EXTERN+UNDEF) {
			if (lp >= &local[NSYMPR])
				error(2, "Local symbol overflow");
			lp->locindex = symno;
			lp++->locsymbol = sp;
			continue;
		}
		if (cursym.stype!=sp->stype || cursym.svalue!=sp->svalue) {
			printf("%.8s: ", cursym.sname);
			error(1, "Multiply defined");
		}
	}
	dseek(&text, loc, filhdr.tsize);
	dseek(&reloc, loc + half(filhdr.tsize + filhdr.dsize), filhdr.tsize);
	load2td(lp, ctrel, &toutb, &troutb);
	dseek(&text, loc+half(filhdr.tsize), filhdr.dsize);
	dseek(&reloc, loc+filhdr.tsize+half(filhdr.dsize), filhdr.dsize);
	load2td(lp, cdrel, &doutb, &droutb);
	torigin += filhdr.tsize;
	dorigin += filhdr.dsize;
	borigin += filhdr.bsize;
}

int16_t load2td(struct local *lp, int16_t creloc, struct buf *b1, struct buf *b2)
{
	register int16_t r, t;
	register struct symbol *sp;

	for (;;) {
		/*
			 * The pickup code is copied from "get" for speed.
			 */

		/* next text or data word */
		if (--text.size <= 0) {
			if (text.size < 0)
				break;
			text.size++;
			t = get(&text);
		} else if (--text.nibuf < 0) {
			text.nibuf++;
			text.size++;
			t = get(&text);
		} else
			t = *text.ptr++;

		/* next relocation word */
		if (--reloc.size <= 0) {
			if (reloc.size < 0)
				error(2, "Relocation error");
			reloc.size++;
			r = get(&reloc);
		} else if (--reloc.nibuf < 0) {
			reloc.nibuf++;
			reloc.size++;
			r = get(&reloc);
		} else
			r = *reloc.ptr++;

		switch (r&016) {

		case RTEXT:
			t += ctrel;
			break;

		case RDATA:
			t += cdrel;
			break;

		case RBSS:
			t += cbrel;
			break;

		case REXT:
			sp = lookloc(lp, r);
			if (sp->stype==EXTERN+UNDEF) {
				r = (r&01) + ((nsym+(sp-symtab))<<4) + REXT;
				break;
			}
			t += sp->svalue;
			r = (r&01) + ((sp->stype-(EXTERN+ABS))<<1);
			break;
		}
		if (r&01)
			t -= creloc;
		ld_putw(t, b1);
		if (rflag)
			ld_putw(r, b2);
	}
}

int16_t finishout(void)
{
	register int16_t n, *p;

	if (nflag||iflag) {
		n = torigin;
		while (n&077) {
			n += 2;
			ld_putw(0, &toutb);
			if (rflag)
				ld_putw(0, &troutb);
		}
	}
	copy(&doutb);
	if (rflag) {
		copy(&troutb);
		copy(&droutb);
	}
	if (sflag==0) {
		if (xflag==0)
			copy(&soutb);
		for (p = (int16_t *)symtab; p < (int16_t *)&symtab[symindex];)
			ld_putw(*p++, &toutb);
	}
	flush(&toutb);
	close(toutb.fildes);
	if (!ofilfnd) {
		unlink("a.out");
		link("l.out", "a.out");
		ofilename = "a.out";
	}
	delarg = errlev;
	delexit();
}

int16_t copy(struct buf *buf)
{
	register int16_t f, *p, n;

	flush(buf);
	lseek(f = buf->fildes, (int32_t)0, 0);
	while ((n = read(f, (char *)doutb.iobuf, sizeof(doutb.iobuf))) > 1) {
		n >>= 1;
		p = (int16_t *)doutb.iobuf;
		do
			ld_putw(*p++, &toutb);
		while (--n);
	}
	close(f);
}

void mkfsym(char *s)
{

	if (sflag || xflag)
		return;
	cp8c(s, cursym.sname);
	cursym.stype = 037;
	cursym.svalue = torigin;
	mput(&soutb, (int16_t *)&cursym, sizeof cursym);
}

void mget(int16_t *aloc, int16_t an)
{
	register int16_t *loc, n;
	register int16_t *p;

	n = an;
	n >>= 1;
	loc = aloc;
	if ((text.nibuf -= n) >= 0) {
		if ((text.size -= n) > 0) {
			p = text.ptr;
			do
				*loc++ = *p++;
			while (--n);
			text.ptr = p;
			return;
		} else
			text.size += n;
	}
	text.nibuf += n;
	do {
		*loc++ = get(&text);
	} 
	while (--n);
}

int16_t mput(struct buf *buf, int16_t *aloc, int16_t an)
{
	register int16_t *loc;
	register int16_t n;

	loc = aloc;
	n = an>>1;
	do {
		ld_putw(*loc++, buf);
	} 
	while (--n);
}

int16_t dseek(struct stream *asp, int32_t aloc, int16_t s)
{
	register struct stream *sp;
	register struct page *p;
	/* register */ int32_t b, o;
	int16_t n;

	b = aloc >> 8;
	o = aloc & 0377;
	sp = asp;
	--sp->pno->nuser;
	if ((p = &page[0])->bno!=b && (p = &page[1])->bno!=b)
		if (p->nuser==0 || (p = &page[0])->nuser==0) {
			if (page[0].nuser==0 && page[1].nuser==0)
				if (page[0].bno < page[1].bno)
					p = &page[0];
			p->bno = b;
			lseek(infil, (aloc & ~0377L) << 1, 0);
			if ((n = read(infil, (char *)p->buff, 512)>>1) < 0)
				n = 0;
			p->nibuf = n;
	} else
		error(2, "No pages");
	++p->nuser;
	sp->bno = b;
	sp->pno = p;
	sp->ptr = p->buff + o;
	if (s != -1)
		sp->size = half(s);
	if ((sp->nibuf = p->nibuf-o) <= 0)
		sp->size = 0;
}

int16_t half(int16_t i)
{
	return((i>>1)&077777);
}

int16_t get(struct stream *asp)
{
	register struct stream *sp;

	sp = asp;
	if (--sp->nibuf < 0) {
		dseek(sp, (int32_t)(sp->bno + 1) << 8, -1);
		--sp->nibuf;
	}
	if (--sp->size <= 0) {
		if (sp->size < 0)
			error(2, premeof);
		++fpage.nuser;
		--sp->pno->nuser;
		sp->pno = (struct page *)&fpage;
	}
	return(*sp->ptr++);
}

int16_t getfile(char *acp)
{
	register char *cp;
	register int16_t c;
	struct stat x;

	cp = acp;
	infil = -1;
	archdr.aname[0] = '\0';
	filname = cp;
	if (cp[0]=='-' && cp[1]=='l') {
		static char pathbuf[512];	/* host fix: V7 wrote the string literal */
		char *name = cp[2] ? cp+2 : "a";
		/* resolve -lname against -L dirs, then the target lib dir */
		infil = -1;
		for (c = 0; infil < 0 && c < nlibdir; c++) {
			snprintf(pathbuf, sizeof pathbuf, "%s/lib%s.a", libdirs[c], name);
			infil = open(pathbuf, 0);
		}
		if (infil < 0) {
			snprintf(pathbuf, sizeof pathbuf, V7_LIBDIR "/lib%s.a", name);
			infil = open(pathbuf, 0);
		}
		if (infil < 0) {
			snprintf(pathbuf, sizeof pathbuf, V7_LIBDIR "/lib%s.a", name);
			infil = open(pathbuf, 0);
		}
		filname = pathbuf;
	}
	if (infil == -1 && (infil = open(filname, 0)) < 0)
		error(2, "cannot open");
	page[0].bno = page[1].bno = -1;
	page[0].nuser = page[1].nuser = 0;
	text.pno = reloc.pno = (struct page *)&fpage;
	fpage.nuser = 2;
	dseek(&text, 0L, 2);
	if (text.size <= 0)
		error(2, premeof);
	if((get(&text) & 0177777) != ARCMAGIC)
		return(0);	/* regualr file */
	dseek(&text, 1L, sizeof archdr);	/* word addressing */
	if(text.size <= 0)
		return(1);	/* regular archive */
	mget((int16_t *)&archdr, sizeof archdr);
	if(strncmp(archdr.aname, goodnm, 14) != 0)
		return(1);	/* regular archive */
	else {
		fstat(infil, &x);
		if(x.st_mtime > mkl(archdr.atime))
		{
			return(3);
		}
		else return(2);
	}
}

struct symbol * * lookup(void)
{
	int16_t i; 
	int16_t clash;
	register struct symbol **hp;
	register char *cp, *cp1;

	i = 0;
	for (cp = cursym.sname; cp < &cursym.sname[8];)
		i = (i<<1) + *cp++;
	for (hp = &hshtab[(i&077777)%NSYM+2]; *hp!=0;) {
		cp1 = (*hp)->sname; 
		clash=FALSE;
		for (cp = cursym.sname; cp < &cursym.sname[8];)
			if (*cp++ != *cp1++) {
				clash=TRUE; 
				break;
			}
		if (clash) {
			if (++hp >= &hshtab[NSYM+2])
				hp = hshtab;
		} else
			break;
	}
	return(hp);
}

struct symbol * * slookup(char *s)
{
	cp8c(s, cursym.sname);
	cursym.stype = EXTERN+UNDEF;
	cursym.svalue = 0;
	return(lookup());
}

int16_t enter(struct symbol **hp)
{
	register struct symbol *sp;

	if (*hp==0) {
		if (symindex>=NSYM)
			error(2, "Symbol table overflow");
		symhash[symindex] = hp;
		*hp = lastsym = sp = &symtab[symindex++];
		cp8c(cursym.sname, sp->sname);
		sp->stype = cursym.stype;
		sp->svalue = cursym.svalue;
		return(1);
	} else {
		lastsym = *hp;
		return(0);
	}
}

void symreloc(void)
{
	switch (cursym.stype) {

	case TEXT:
	case EXTERN+TEXT:
		cursym.svalue += ctrel;
		return;

	case DATA:
	case EXTERN+DATA:
		cursym.svalue += cdrel;
		return;

	case BSS:
	case EXTERN+BSS:
		cursym.svalue += cbrel;
		return;

	case EXTERN+UNDEF:
		return;
	}
	if (cursym.stype&EXTERN)
		cursym.stype = EXTERN+ABS;
}

int16_t error(int16_t n, char *s)
{
	if (errlev==0)
		printf("ld:");
	if (filname) {
		printf("%s", filname);
		if (archdr.aname[0])
			printf("(%.14s)", archdr.aname);
		printf(": ");
	}
	printf("%s\n", s);
	if (n > 1)
		delexit();
	errlev = n;
}

struct symbol * lookloc(struct local *alp, int16_t r)
{
	register struct local *clp, *lp;
	register int16_t sn;

	lp = alp;
	sn = (r>>4) & 07777;
	for (clp = local; clp<lp; clp++)
		if (clp->locindex == sn)
			return(clp->locsymbol);
	error(2, "Local symbol botch");
}

int16_t readhdr(int32_t loc)
{
	register int16_t st, sd;

	dseek(&text, loc, sizeof filhdr);
	mget((int16_t *)&filhdr, sizeof filhdr);
	if (filhdr.fmagic != FMAGIC)
		error(2, "Bad format");
	st = (filhdr.tsize+01) & ~01;
	filhdr.tsize = st;
	cdrel = -st;
	sd = (filhdr.dsize+01) & ~01;
	cbrel = - (st+sd);
	filhdr.bsize = (filhdr.bsize+01) & ~01;
}

int16_t cp8c(char *from, char *to)
{
	register char *f, *t, *te;

	f = from;
	t = to;
	te = t+8;
	while ((*t++ = *f++) && t<te);
	while (t<te)
		*t++ = 0;
}

int16_t eq(char *s1, char *s2)
{
	while (*s1==*s2++)
		if ((*s1++)==0)
			return(TRUE);
	return(FALSE);
}

int16_t ld_putw(int16_t w, struct buf *b)
{
	*(b->xnext)++ = w;
	if (--b->nleft <= 0)
		flush(b);
}

int16_t flush(struct buf *b)
{
	register int16_t n;

	if ((n = (char *)b->xnext - (char *)b->iobuf) > 0)
		if (write(b->fildes, (char *)b->iobuf, n) != n)
			error(2, "output error");
	b->xnext = b->iobuf;
	b->nleft = sizeof(b->iobuf)/sizeof(int16_t);
}
