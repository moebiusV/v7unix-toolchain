#define _DEFAULT_SOURCE 1
/*
 * ar -- V7 archive maintainer, host-ported (orig/usr/src/cmd/ar.c).
 *
 * Creates and maintains V7 (PDP-11) archives.  The archive format is not the
 * modern GNU/BSD one: ARMAG (0177545, i.e. 0xff65) as a 2-byte little-endian
 * magic, then per member a 26-byte struct ar_hdr whose 32-bit ar_date and
 * ar_size are *middle-endian* (high 16-bit word first, each word little-endian)
 * -- the PDP-11's long layout.  Those two fields are therefore carried as
 * int16_t[2] and converted with mkl()/mkint(), exactly as the ld port does.
 *
 * Also host-adapted: ARMAG is written as 2 bytes (V7's `sizeof(int)` was 2);
 * mktemp templates carry six X's (glibc's mktemp requires exactly 6); the
 * V7 `%D` printf format is `%ld`.  Two V7 identifiers (the global `tmpnam`
 * and the `select()` mode printer) clash with the host libc, so they are
 * renamed `tmpnam_ar` and `select_ar` (the `_<tool>` suffix, as sh uses
 * `_sh`).
 */

#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>

#define	ARMAG	0177545

/* V7 /usr/include/ar.h -- 26 bytes, no padding (all offsets stay even). */
struct ar_hdr {
	char	ar_name[14];
	int16_t	ar_date[2];	/* middle-endian: [0]=high word, [1]=low word */
	char	ar_uid;
	char	ar_gid;
	int16_t	ar_mode;
	int16_t	ar_size[2];	/* middle-endian: [0]=high word, [1]=low word */
};

struct stat stbuf;
struct ar_hdr arbuf;

#define	SKIP	1
#define	IODD	2
#define	OODD	4
#define	HEAD	8

char	*man	= "mrxtdpq";
char	*opt	= "uvnbail";

int	signum[] = { SIGHUP, SIGINT, SIGQUIT, 0 };
int	(*comfun)(void);
char	flg[26];
char	**namv;
int	namc;
char	*arnam;
char	*ponam;
char	tmpnam_ar[]	= "/tmp/vXXXXXX";
char	tmp1nam[]	= "/tmp/v1XXXXXX";
char	tmp2nam[]	= "/tmp/v2XXXXXX";
char	*tfnam;
char	*tf1nam;
char	*tf2nam;
char	*file;
char	name[16];
int	af;
int	tf;
int	tf1;
int	tf2;
int	qf;
int	bastate;
char	buf[512];

/* host-port: reassemble a PDP-11 middle-endian 32-bit value from its two
 * 16-bit words, and its inverse. */
static int32_t mkl(int16_t *w)
{
	return (int32_t)(((uint32_t)(uint16_t)w[0] << 16) | (uint16_t)w[1]);
}

static void mkint(int32_t v, int16_t *w)
{
	w[0] = (int16_t)((v >> 16) & 0xFFFF);
	w[1] = (int16_t)(v & 0xFFFF);
}

int main(int argc, char *argv[]);
void setcom(int (*fun)(void));
int rcmd(void);
int dcmd(void);
int xcmd(void);
int tcmd(void);
int pcmd(void);
int mcmd(void);
int qcmd(void);
void init(void);
int getaf(void);
void getqf(void);
void usage(void);
void noar(void);
void sigdone(int sig);
void done(int c);
int notfound(void);
int morefil(void);
void cleanup(void);
void install(void);
void movefil(int f);
int stats(void);
void copyfil(int fi, int fo, int flag);
int getdir(void);
int match(void);
void bamatch(void);
void phserr(void);
void mesg(int c);
char *trim(char *s);
void longt(void);
void pmode(void);
void select_ar(int *pairp);
void wrerr(void);

int main(int argc, char *argv[])
{
	int i;
	char *cp;

	for (i = 0; signum[i]; i++)
		if (signal(signum[i], SIG_IGN) != SIG_IGN)
			signal(signum[i], sigdone);
	if (argc < 3)
		usage();
	for (cp = argv[1]; *cp; cp++)
		switch (*cp) {
		case 'l':
		case 'v':
		case 'u':
		case 'n':
		case 'a':
		case 'b':
		case 'c':
		case 'i':
			flg[*cp - 'a']++;
			continue;

		case 'r':
			setcom(rcmd);
			continue;

		case 'd':
			setcom(dcmd);
			continue;

		case 'x':
			setcom(xcmd);
			continue;

		case 't':
			setcom(tcmd);
			continue;

		case 'p':
			setcom(pcmd);
			continue;

		case 'm':
			setcom(mcmd);
			continue;

		case 'q':
			setcom(qcmd);
			continue;

		default:
			fprintf(stderr, "ar: bad option `%c'\n", *cp);
			done(1);
		}
	if (flg['l' - 'a']) {
		strcpy(tmpnam_ar, "vXXXXXX");
		strcpy(tmp1nam, "v1XXXXXX");
		strcpy(tmp2nam, "v2XXXXXX");
	}
	if (flg['i' - 'a'])
		flg['b' - 'a']++;
	if (flg['a' - 'a'] || flg['b' - 'a']) {
		bastate = 1;
		ponam = trim(argv[2]);
		argv++;
		argc--;
		if (argc < 3)
			usage();
	}
	arnam = argv[2];
	namv = argv + 3;
	namc = argc - 3;
	if (comfun == 0) {
		if (flg['u' - 'a'] == 0) {
			fprintf(stderr, "ar: one of [%s] must be specified\n", man);
			done(1);
		}
		setcom(rcmd);
	}
	(*comfun)();
	done(notfound());
}

void setcom(int (*fun)(void))
{
	if (comfun != 0) {
		fprintf(stderr, "ar: only one of [%s] allowed\n", man);
		done(1);
	}
	comfun = fun;
}

int rcmd(void)
{
	int f;

	init();
	getaf();
	while (!getdir()) {
		bamatch();
		if (namc == 0 || match()) {
			f = stats();
			if (f < 0) {
				if (namc)
					fprintf(stderr, "ar: cannot open %s\n", file);
				goto cp;
			}
			if (flg['u' - 'a'])
				if (stbuf.st_mtime <= mkl(arbuf.ar_date)) {
					close(f);
					goto cp;
				}
			mesg('r');
			copyfil(af, -1, IODD + SKIP);
			movefil(f);
			continue;
		}
cp:
		mesg('c');
		copyfil(af, tf, IODD + OODD + HEAD);
	}
	cleanup();
	return 0;
}

int dcmd(void)
{
	init();
	if (getaf())
		noar();
	while (!getdir()) {
		if (match()) {
			mesg('d');
			copyfil(af, -1, IODD + SKIP);
			continue;
		}
		mesg('c');
		copyfil(af, tf, IODD + OODD + HEAD);
	}
	install();
	return 0;
}

int xcmd(void)
{
	int f;

	if (getaf())
		noar();
	while (!getdir()) {
		if (namc == 0 || match()) {
			f = creat(file, arbuf.ar_mode & 0777);
			if (f < 0) {
				fprintf(stderr, "ar: %s cannot create\n", file);
				goto sk;
			}
			mesg('x');
			copyfil(af, f, IODD);
			close(f);
			continue;
		}
sk:
		mesg('c');
		copyfil(af, -1, IODD + SKIP);
		if (namc > 0 && !morefil())
			done(0);
	}
	return 0;
}

int pcmd(void)
{
	if (getaf())
		noar();
	while (!getdir()) {
		if (namc == 0 || match()) {
			if (flg['v' - 'a']) {
				printf("\n<%s>\n\n", file);
				fflush(stdout);
			}
			copyfil(af, 1, IODD);
			continue;
		}
		copyfil(af, -1, IODD + SKIP);
	}
	return 0;
}

int mcmd(void)
{
	init();
	if (getaf())
		noar();
	tf2nam = mktemp(tmp2nam);
	close(creat(tf2nam, 0600));
	tf2 = open(tf2nam, 2);
	if (tf2 < 0) {
		fprintf(stderr, "ar: cannot create third temp\n");
		done(1);
	}
	while (!getdir()) {
		bamatch();
		if (match()) {
			mesg('m');
			copyfil(af, tf2, IODD + OODD + HEAD);
			continue;
		}
		mesg('c');
		copyfil(af, tf, IODD + OODD + HEAD);
	}
	install();
	return 0;
}

int tcmd(void)
{
	if (getaf())
		noar();
	while (!getdir()) {
		if (namc == 0 || match()) {
			if (flg['v' - 'a'])
				longt();
			printf("%s\n", trim(file));
		}
		copyfil(af, -1, IODD + SKIP);
	}
	return 0;
}

int qcmd(void)
{
	int i, f;

	if (flg['a' - 'a'] || flg['b' - 'a']) {
		fprintf(stderr, "ar: abi not allowed with q\n");
		done(1);
	}
	getqf();
	for (i = 0; signum[i]; i++)
		signal(signum[i], SIG_IGN);
	lseek(qf, 0L, 2);
	for (i = 0; i < namc; i++) {
		file = namv[i];
		if (file == 0)
			continue;
		namv[i] = 0;
		mesg('q');
		f = stats();
		if (f < 0) {
			fprintf(stderr, "ar: %s cannot open\n", file);
			continue;
		}
		tf = qf;
		movefil(f);
		qf = tf;
	}
	return 0;
}

/* ARMAG is a 2-byte magic (0xff65).  V7 wrote it with `sizeof(int)` — 2 bytes
 * on the PDP-11 but 4 on this host.  The buffer is uint16_t: a fixed-width
 * 16-bit type (unlike `short`, which C only guarantees to be *at least* 16
 * bits), and unsigned because ARMAG 0xff65 overflows signed 16-bit. */
void init(void)
{
	uint16_t mbuf = ARMAG;

	tfnam = mktemp(tmpnam_ar);
	close(creat(tfnam, 0600));
	tf = open(tfnam, 2);
	if (tf < 0) {
		fprintf(stderr, "ar: cannot create temp file\n");
		done(1);
	}
	if (write(tf, (char *)&mbuf, sizeof mbuf) != sizeof mbuf)
		wrerr();
}

int getaf(void)
{
	uint16_t mbuf;

	af = open(arnam, 0);
	if (af < 0)
		return 1;
	if (read(af, (char *)&mbuf, sizeof mbuf) != sizeof mbuf
	    || mbuf != ARMAG) {
		fprintf(stderr, "ar: %s not in archive format\n", arnam);
		done(1);
	}
	return 0;
}

void getqf(void)
{
	uint16_t mbuf;

	if ((qf = open(arnam, 2)) < 0) {
		if (!flg['c' - 'a'])
			fprintf(stderr, "ar: creating %s\n", arnam);
		close(creat(arnam, 0666));
		if ((qf = open(arnam, 2)) < 0) {
			fprintf(stderr, "ar: cannot create %s\n", arnam);
			done(1);
		}
		mbuf = ARMAG;
		if (write(qf, (char *)&mbuf, sizeof mbuf) != sizeof mbuf)
			wrerr();
	} else if (read(qf, (char *)&mbuf, sizeof mbuf) != sizeof mbuf
		   || mbuf != ARMAG) {
		fprintf(stderr, "ar: %s not in archive format\n", arnam);
		done(1);
	}
}

void usage(void)
{
	printf("usage: ar [%s][%s] archive files ...\n", opt, man);
	done(1);
}

void noar(void)
{
	fprintf(stderr, "ar: %s does not exist\n", arnam);
	done(1);
}

void sigdone(int sig)
{
	done(100);
}

void done(int c)
{
	if (tfnam)
		unlink(tfnam);
	if (tf1nam)
		unlink(tf1nam);
	if (tf2nam)
		unlink(tf2nam);
	exit(c);
}

int notfound(void)
{
	int i, n;

	n = 0;
	for (i = 0; i < namc; i++)
		if (namv[i]) {
			fprintf(stderr, "ar: %s not found\n", namv[i]);
			n++;
		}
	return n;
}

int morefil(void)
{
	int i, n;

	n = 0;
	for (i = 0; i < namc; i++)
		if (namv[i])
			n++;
	return n;
}

void cleanup(void)
{
	int i, f;

	for (i = 0; i < namc; i++) {
		file = namv[i];
		if (file == 0)
			continue;
		namv[i] = 0;
		mesg('a');
		f = stats();
		if (f < 0) {
			fprintf(stderr, "ar: %s cannot open\n", file);
			continue;
		}
		movefil(f);
	}
	install();
}

void install(void)
{
	int i;

	for (i = 0; signum[i]; i++)
		signal(signum[i], SIG_IGN);
	if (af < 0)
		if (!flg['c' - 'a'])
			fprintf(stderr, "ar: creating %s\n", arnam);
	close(af);
	af = creat(arnam, 0666);
	if (af < 0) {
		fprintf(stderr, "ar: cannot create %s\n", arnam);
		done(1);
	}
	if (tfnam) {
		lseek(tf, 0L, 0);
		while ((i = read(tf, buf, 512)) > 0)
			if (write(af, buf, i) != i)
				wrerr();
	}
	if (tf2nam) {
		lseek(tf2, 0L, 0);
		while ((i = read(tf2, buf, 512)) > 0)
			if (write(af, buf, i) != i)
				wrerr();
	}
	if (tf1nam) {
		lseek(tf1, 0L, 0);
		while ((i = read(tf1, buf, 512)) > 0)
			if (write(af, buf, i) != i)
				wrerr();
	}
}

/*
 * insert the file 'file' into the temporary file
 */
void movefil(int f)
{
	char *cp;
	int i;

	cp = trim(file);
	for (i = 0; i < 14; i++)
		if (arbuf.ar_name[i] = *cp)
			cp++;
	mkint((int32_t)stbuf.st_size, arbuf.ar_size);
	mkint((int32_t)stbuf.st_mtime, arbuf.ar_date);
	arbuf.ar_uid = (char)stbuf.st_uid;
	arbuf.ar_gid = (char)stbuf.st_gid;
	arbuf.ar_mode = (int16_t)stbuf.st_mode;
	copyfil(f, tf, OODD + HEAD);
	close(f);
}

int stats(void)
{
	int f;

	f = open(file, 0);
	if (f < 0)
		return f;
	if (fstat(f, &stbuf) < 0) {
		close(f);
		return -1;
	}
	return f;
}

/*
 * copy next file; size given in arbuf
 */
void copyfil(int fi, int fo, int flag)
{
	int i, o;
	int pe;
	int32_t size;

	if (flag & HEAD)
		if (write(fo, (char *)&arbuf, sizeof arbuf) != sizeof arbuf)
			wrerr();
	pe = 0;
	size = mkl(arbuf.ar_size);
	while (size > 0) {
		i = o = 512;
		if (size < i) {
			i = o = (int)size;
			if (i & 1) {
				if (flag & IODD)
					i++;
				if (flag & OODD)
					o++;
			}
		}
		if (read(fi, buf, i) != i)
			pe++;
		if ((flag & SKIP) == 0)
			if (write(fo, buf, o) != o)
				wrerr();
		size -= 512;
	}
	if (pe)
		phserr();
}

int getdir(void)
{
	int i;

	i = read(af, (char *)&arbuf, sizeof arbuf);
	if (i != sizeof arbuf) {
		if (tf1nam) {
			i = tf;
			tf = tf1;
			tf1 = i;
		}
		return 1;
	}
	for (i = 0; i < 14; i++)
		name[i] = arbuf.ar_name[i];
	file = name;
	return 0;
}

int match(void)
{
	int i;

	for (i = 0; i < namc; i++) {
		if (namv[i] == 0)
			continue;
		if (strcmp(trim(namv[i]), file) == 0) {
			file = namv[i];
			namv[i] = 0;
			return 1;
		}
	}
	return 0;
}

void bamatch(void)
{
	int f;

	switch (bastate) {
	case 1:
		if (strcmp(file, ponam) != 0)
			return;
		bastate = 2;
		if (flg['a' - 'a'])
			return;
		/* fall through */
	case 2:
		bastate = 0;
		tf1nam = mktemp(tmp1nam);
		close(creat(tf1nam, 0600));
		f = open(tf1nam, 2);
		if (f < 0) {
			fprintf(stderr, "ar: cannot create second temp\n");
			return;
		}
		tf1 = tf;
		tf = f;
	}
}

void phserr(void)
{
	fprintf(stderr, "ar: phase error on %s\n", file);
}

void mesg(int c)
{
	if (flg['v' - 'a'])
		if (c != 'c' || flg['v' - 'a'] > 1)
			printf("%c - %s\n", c, file);
}

char *trim(char *s)
{
	char *p1, *p2;

	for (p1 = s; *p1; p1++)
		;
	while (p1 > s) {
		if (*--p1 != '/')
			break;
		*p1 = 0;
	}
	p2 = s;
	for (p1 = s; *p1; p1++)
		if (*p1 == '/')
			p2 = p1 + 1;
	return p2;
}

#define	IFMT	060000
#define	ISARG	01000
#define	LARGE	010000
#define	SUID	04000
#define	SGID	02000
#define	ROWN	0400
#define	WOWN	0200
#define	XOWN	0100
#define	RGRP	040
#define	WGRP	020
#define	XGRP	010
#define	ROTH	04
#define	WOTH	02
#define	XOTH	01
#define	STXT	01000

void longt(void)
{
	char *cp;
	time_t t;

	pmode();
	printf("%3d/%1d", (int)(unsigned char)arbuf.ar_uid,
	       (int)(unsigned char)arbuf.ar_gid);
	printf("%7ld", (long)mkl(arbuf.ar_size));
	t = (time_t)mkl(arbuf.ar_date);
	cp = ctime(&t);
	printf(" %-12.12s %-4.4s ", cp + 4, cp + 20);
}

int m1[] = { 1, ROWN, 'r', '-' };
int m2[] = { 1, WOWN, 'w', '-' };
int m3[] = { 2, SUID, 's', XOWN, 'x', '-' };
int m4[] = { 1, RGRP, 'r', '-' };
int m5[] = { 1, WGRP, 'w', '-' };
int m6[] = { 2, SGID, 's', XGRP, 'x', '-' };
int m7[] = { 1, ROTH, 'r', '-' };
int m8[] = { 1, WOTH, 'w', '-' };
int m9[] = { 2, STXT, 't', XOTH, 'x', '-' };

int *m[] = { m1, m2, m3, m4, m5, m6, m7, m8, m9 };

void pmode(void)
{
	int **mp;

	for (mp = &m[0]; mp < &m[9];)
		select_ar(*mp++);
}

void select_ar(int *pairp)
{
	int n, *ap;

	ap = pairp;
	n = *ap++;
	while (--n >= 0 && (arbuf.ar_mode & *ap++) == 0)
		ap++;
	putchar(*ap);
}

void wrerr(void)
{
	perror("ar write error");
	done(1);
}
