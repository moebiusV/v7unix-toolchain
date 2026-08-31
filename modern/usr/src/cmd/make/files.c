
/* UNIX DEPENDENT PROCEDURES */

#include "defs"
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


/* DEFAULT RULES FOR UNIX.
 * Writable (char[][64], not string literals): eqsign() splits a
 * NAME=value line *in place*, and a modern host puts literals in .rodata. */
char builtin[][64] =
	{
	".SUFFIXES : .out .o .c .f .e .r .y .yr .ye .l .s",
	"YACC=yacc",
	"YACCR=yacc -r",
	"YACCE=yacc -e",
	"YFLAGS=",
	"LEX=lex",
	"LFLAGS=",
	"CC=cc",
	"AS=as -",
	"CFLAGS=",
	"RC=f77",
	"RFLAGS=",
	"EC=f77",
	"EFLAGS=",
	"FFLAGS=",
	"LOADLIBES=",

	".c.o :",
	"\t$(CC) $(CFLAGS) -c $<",

	".e.o .r.o .f.o :",
	"\t$(EC) $(RFLAGS) $(EFLAGS) $(FFLAGS) -c $<",

	".s.o :",
	"\t$(AS) -o $@ $<",

	".y.o :",
	"\t$(YACC) $(YFLAGS) $<",
	"\t$(CC) $(CFLAGS) -c y.tab.c",
	"\trm y.tab.c",
	"\tmv y.tab.o $@",

	".yr.o:",
	"\t$(YACCR) $(YFLAGS) $<",
	"\t$(RC) $(RFLAGS) -c y.tab.r",
	"\trm y.tab.r",
	"\tmv y.tab.o $@",

	".ye.o :",
	"\t$(YACCE) $(YFLAGS) $<",
	"\t$(EC) $(RFLAGS) -c y.tab.e",
	"\trm y.tab.e",
	"\tmv y.tab.o $@",

	".l.o :",
	"\t$(LEX) $(LFLAGS) $<",
	"\t$(CC) $(CFLAGS) -c lex.yy.c",
	"\trm lex.yy.c",
	"\tmv lex.yy.o $@",

	".y.c :",
	"\t$(YACC) $(YFLAGS) $<",
	"\tmv y.tab.c $@",

	".l.c :",
	"\t$(LEX) $<",
	"\tmv lex.yy.c $@",

	".yr.r:",
	"\t$(YACCR) $(YFLAGS) $<",
	"\tmv y.tab.r $@",

	".ye.e :",
	"\t$(YACCE) $(YFLAGS) $<",
	"\tmv y.tab.e $@",

	".s.out .c.out .o.out :",
	"\t$(CC) $(CFLAGS) $< $(LOADLIBES) -o $@",

	".f.out .r.out .e.out :",
	"\t$(EC) $(EFLAGS) $(RFLAGS) $(FFLAGS) $< $(LOADLIBES) -o $@",
	"\t-rm $*.o",

	".y.out :",
	"\t$(YACC) $(YFLAGS) $<",
	"\t$(CC) $(CFLAGS) y.tab.c $(LOADLIBES) -ly -o $@",
	"\trm y.tab.c",

	".l.out :",
	"\t$(LEX) $<",
	"\t$(CC) $(CFLAGS) lex.yy.c $(LOADLIBES) -ll -o $@",
	"\trm lex.yy.c",

	"" };


static int amatch(char *s, char *p);
static int umatch(char *s, char *p);

TIMETYPE exists(char *filename)
{
struct stat buf;
register char *s;

for(s = filename ; *s!='\0' && *s!='(' ; ++s)
	;

if(*s == '(')
	return(lookarch(filename));

if(stat(filename,&buf) < 0)
	return(0);
else	return((TIMETYPE) buf.st_mtime);
}


TIMETYPE prestime(void)
{
time_t t;
time(&t);
return((TIMETYPE) t);
}


struct depblock *srchdir(char *pat, int mkchain, struct depblock *nextdbl)
{
DIR * dirf;
struct dirent *entry;
char *dirname, *dirpref, *endir, *filepat, *p, temp[100];
char fullname[100];
struct nameblock *q;
struct depblock *thisdbl;
struct opendir *od;
struct pattern *patp;


thisdbl = 0;

if(mkchain == NO)
	for(patp=firstpat ; patp ; patp = patp->nxtpattern)
		if(! strcmp(pat, patp->patval)) return(0);

patp = ckalloc(sizeof(struct pattern)) /* ALLOC(pattern) in v7 */;
patp->nxtpattern = firstpat;
firstpat = patp;
patp->patval = copys(pat);

endir = 0;

for(p=pat; *p!='\0'; ++p)
	if(*p=='/') endir = p;

if(endir==0)
	{
	dirname = ".";
	dirpref = "";
	filepat = pat;
	}
else	{
	dirname = pat;
	*endir = '\0';
	dirpref = concat(dirname, "/", temp);
	filepat = endir+1;
	}

dirf = NULL;

for(od = firstod ; od; od = od->nxtopendir)
	if(! strcmp(dirname, od->dirn) )
		{
		dirf = od->dirfc;
		rewinddir(dirf); /* start over at the beginning  */
		break;
		}

if(dirf == NULL)
	{
	dirf = opendir(dirname);
	od = ckalloc(sizeof(struct opendir)) /* ALLOC(opendir) in v7 */;
	od->nxtopendir = firstod;
	firstod = od;
	od->dirfc = dirf;
	od->dirn = copys(dirname);
	}

if(dirf == NULL)
	{
	fprintf(stderr, "Directory %s: ", dirname);
	fatal("Cannot open");
	}

else while( (entry = readdir(dirf)) != NULL)
	if(entry->d_ino != 0)
		{
		if( amatch(entry->d_name,filepat) )
			{
			concat(dirpref,entry->d_name,fullname);
			if( (q=srchname(fullname)) ==0)
				q = makename(copys(fullname));
			if(mkchain)
				{
				thisdbl = ckalloc(sizeof(struct depblock)) /* ALLOC(depblock) in v7 */;
				thisdbl->nxtdepblock = nextdbl;
				thisdbl->depname = q;
				nextdbl = thisdbl;
				}
			}
		}

if(endir != 0)  *endir = '/';

return(thisdbl);
}

/* stolen from glob through find */

static int amatch(char *s, char *p)
{
	register int cc, scc, k;
	int c, lc;

	scc = *s;
	lc = 077777;
	switch (c = *p) {

	case '[':
		k = 0;
		while (cc = *++p) {
			switch (cc) {

			case ']':
				if (k)
					return(amatch(++s, ++p));
				else
					return(0);

			case '-':
				k |= (lc <= scc)  & (scc <= (cc=p[1]) ) ;
			}
			if (scc==(lc=cc)) k++;
		}
		return(0);

	case '?':
	caseq:
		if(scc) return(amatch(++s, ++p));
		return(0);
	case '*':
		return(umatch(s, ++p));
	case 0:
		return(!scc);
	}
	if (c==scc) goto caseq;
	return(0);
}

static int umatch(char *s, char *p)
{
	if(*p==0) return(1);
	while(*s)
		if (amatch(s++,p)) return(1);
	return(0);
}


/* look inside archives for notations a(b) and a((b))
	a(b)	is file member   b   in archive a
	a((b))	is entry point  _b  in object archive a

   V7 ar.h / a.out.h on-disk formats, declared locally: the host's
   <ar.h>/<a.out.h> describe the GNU/BSD formats, not V7's.  PDP-11 32-bit
   fields (ar_date, ar_size) are middle-endian -- high word first -- so they
   are kept as int16_t[2] and reassembled with mkl(), as ld does (PORTING.md
   §4.3). */

enum { ARMAG = 0177545 };

struct ar_hdr {
	char	ar_name[14];
	int16_t	ar_date[2];	/* middle-endian: [0]=high, [1]=low */
	char	ar_uid;
	char	ar_gid;
	int16_t	ar_mode;
	int16_t	ar_size[2];	/* middle-endian: [0]=high, [1]=low */
};

struct exec {
	int16_t	a_magic;
	int16_t	a_text;
	int16_t	a_data;
	int16_t	a_bss;
	int16_t	a_syms;
	int16_t	a_entry;
	int16_t	a_unused;
	int16_t	a_flag;
};

struct nlist {
	char	n_name[8];
	int16_t	n_type;
	int16_t	n_value;
};

enum {
	A_MAGIC1 = 0407,
	A_MAGIC2 = 0410,
	A_MAGIC3 = 0411,
	A_MAGIC4 = 0405,
	N_EXT = 040,
};

int32_t mkl(int16_t *w)
{
	return (int32_t)(((uint32_t)(uint16_t)w[0] << 16) | (uint16_t)w[1]);
}

static struct ar_hdr arhead;
FILE *arfd;
int arpos, arlen;

static struct exec objhead;

static struct nlist objentry;


TIMETYPE lookarch(char *filename)
{
char *p, *q, *send, s[15];
int i, nc, nsym, objarch;

for(p = filename; *p!= '(' ; ++p)
	;
*p = '\0';
openarch(filename);
*p++ = '(';

if(*p == '(')
	{
	objarch = YES;
	nc = 8;
	++p;
	}
else
	{
	objarch = NO;
	nc = 14;
	}
send = s + nc;

for( q = s ; q<send && *p!='\0' && *p!=')' ; *q++ = *p++ )
	;
while(q < send)
	*q++ = '\0';
while(getarch())
	{
	if(objarch)
		{
		getobj();
		nsym = objhead.a_syms / sizeof(objentry);
		for(i = 0; i<nsym ; ++i)
			{
			fread( (char *) &objentry, sizeof(objentry),1,arfd);
			if( (objentry.n_type & N_EXT)
			   && ((objentry.n_type & ~N_EXT) || objentry.n_value)
			   && eqstr(objentry.n_name,s,nc))
				{
				clarch();
				return((TIMETYPE) mkl(arhead.ar_date));
				}
			}
		}

	else if( eqstr(arhead.ar_name, s, nc))
		{
		clarch();
		return((TIMETYPE) mkl(arhead.ar_date));
		}
	}

clarch();
return( 0);
}


void clarch(void)
{
fclose( arfd );
}


void openarch(char *f)
{
uint16_t word;
struct stat buf;

stat(f, &buf);
arlen = buf.st_size;

arfd = fopen(f, "r");
if(arfd == NULL)
	fatal1("cannot open %s", f);
fread( (char *) &word, sizeof(word), 1, arfd);
if(word != (uint16_t) ARMAG)
	fatal1("%s is not an archive", f);
arpos = sizeof(word);	/* first member header follows the ARMAG word */
}



int getarch(void)
{
if(arpos >= arlen)
	return(0);
fseek(arfd, arpos, 0);
fread( (char *) &arhead, sizeof(arhead), 1, arfd);
arpos += sizeof(arhead) + ((mkl(arhead.ar_size) + 1) & ~1L);
return(1);
}


void getobj(void)
{
int32_t skip;

fread( (char *) &objhead, sizeof(objhead), 1, arfd);
if( objhead.a_magic != A_MAGIC1 &&
    objhead.a_magic != A_MAGIC2 &&
    objhead.a_magic != A_MAGIC3 &&
    objhead.a_magic != A_MAGIC4 )
		fatal1("%s is not an object module", arhead.ar_name);
skip = objhead.a_text + objhead.a_data;
if(! objhead.a_flag )
	skip *= 2;
fseek(arfd, skip, 1);
}


int eqstr(char *a, char *b, int n)
{
register int i;
for(i = 0 ; i < n ; ++i)
	if(*a++ != *b++)
		return(NO);
return(YES);
}
