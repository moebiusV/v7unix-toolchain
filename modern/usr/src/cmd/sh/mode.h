# /* UNIX shell — type vocabulary.  V7 wrote these through TYPE/STRUCT/UNION
#  * macros and had no `void`; here they are plain typedefs (VOID -> void). */

#include <stdint.h>

enum { BYTESPERWORD = sizeof(char *) };

typedef char	CHAR;
typedef char	BOOL;
typedef int	UFD;
typedef int	INT;
typedef float	REAL;
typedef char	*ADDRESS;
typedef long	L_INT;
typedef void	VOID;
typedef unsigned POS;
typedef char	*STRING;
typedef char	MSG[];
typedef int	PIPE[];
typedef char	*STKPTR;
typedef char	*BYTPTR;

struct sysnod {
	STRING	sysnam;
	INT	sysval;
};

typedef struct stat	STATBUF;
typedef struct blk	*BLKPTR;
typedef struct fileblk	FILEBLK;
typedef struct filehdr	FILEHDR;
typedef struct fileblk	*FILE;
typedef struct trenod	*TREPTR;
typedef struct forknod	*FORKPTR;
typedef struct comnod	*COMPTR;
typedef struct swnod	*SWPTR;
typedef struct regnod	*REGPTR;
typedef struct parnod	*PARPTR;
typedef struct ifnod	*IFPTR;
typedef struct whnod	*WHPTR;
typedef struct fornod	*FORPTR;
typedef struct lstnod	*LSTPTR;
typedef struct argnod	*ARGPTR;
typedef struct dolnod	*DOLPTR;
typedef struct ionod	*IOPTR;
typedef struct namnod	NAMNOD;
typedef struct namnod	*NAMPTR;
typedef struct sysnod	SYSNOD;
typedef struct sysnod	*SYSPTR;
typedef struct sysnod	SYSTAB[];

#define NIL	((void*)0)

/* Pointer-tagging cheats.  V7 stored flag bits (BUSY/ARGMK) in the low bits of
 * a pointer and used these two casts to read/toggle them — `Rcheat()` reads a
 * pointer as an integer (rvalue), `Lcheat()` gives an integer lvalue aliasing a
 * pointer's storage so the tag can be ORed in place.  They are type puns, not
 * constants or functions, so they stay as macros (with uintptr_t so the tag
 * survives on a 64-bit host). */
#define Lcheat(a)	(*(uintptr_t *)(&(a)))
#define Rcheat(a)	((uintptr_t)(a))

/* address puns for storage allocation */
typedef union {
	FORKPTR	_forkptr;
	COMPTR	_comptr;
	PARPTR	_parptr;
	IFPTR	_ifptr;
	WHPTR	_whptr;
	FORPTR	_forptr;
	LSTPTR	_lstptr;
	BLKPTR	_blkptr;
	NAMPTR	_namptr;
	BYTPTR	_bytptr;
	}	address;

/* heap storage */
struct blk {
	BLKPTR	word;
};

enum { BUFSIZ = 64 };

struct fileblk {
	UFD	fdes;
	POS	flin;
	BOOL	feof;
	CHAR	fsiz;
	STRING	fnxt;
	STRING	fend;
	STRING	*feval;
	FILE	fstak;
	CHAR	fbuf[BUFSIZ];
};

/* for files not used with file descriptors */
struct filehdr {
	UFD	fdes;
	POS	flin;
	BOOL	feof;
	CHAR	fsiz;
	STRING	fnxt;
	STRING	fend;
	STRING	*feval;
	FILE	fstak;
	CHAR	_fbuf[1];
};

/* this node is a proforma for those that follow */
struct trenod {
	INT	tretyp;
	IOPTR	treio;
};

/* dummy for access only */
struct argnod {
	ARGPTR	argnxt;
	CHAR	argval[1];
};

struct dolnod {
	DOLPTR	dolnxt;
	INT	doluse;
	CHAR	dolarg[1];
};

struct forknod {
	INT	forktyp;
	IOPTR	forkio;
	TREPTR	forktre;
};

struct comnod {
	INT	comtyp;
	IOPTR	comio;
	ARGPTR	comarg;
	ARGPTR	comset;
};

struct ifnod {
	INT	iftyp;
	TREPTR	iftre;
	TREPTR	thtre;
	TREPTR	eltre;
};

struct whnod {
	INT	whtyp;
	TREPTR	whtre;
	TREPTR	dotre;
};

struct fornod {
	INT	fortyp;
	TREPTR	fortre;
	STRING	fornam;
	COMPTR	forlst;
};

struct swnod {
	INT	swtyp;
	STRING	swarg;
	REGPTR	swlst;
};

struct regnod {
	ARGPTR	regptr;
	TREPTR	regcom;
	REGPTR	regnxt;
};

struct parnod {
	INT	partyp;
	TREPTR	partre;
};

struct lstnod {
	INT	lsttyp;
	TREPTR	lstlef;
	TREPTR	lstrit;
};

struct ionod {
	INT	iofile;
	STRING	ioname;
	IOPTR	ionxt;
	IOPTR	iolst;
};

enum {
	FORKTYPE = sizeof(struct forknod),
	COMTYPE = sizeof(struct comnod),
	IFTYPE = sizeof(struct ifnod),
	WHTYPE = sizeof(struct whnod),
	FORTYPE = sizeof(struct fornod),
	SWTYPE = sizeof(struct swnod),
	REGTYPE = sizeof(struct regnod),
	PARTYPE = sizeof(struct parnod),
	LSTTYPE = sizeof(struct lstnod),
	IOTYPE = sizeof(struct ionod),
};
