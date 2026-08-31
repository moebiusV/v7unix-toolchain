/*
 * mv file1 file2
 */

#define _DEFAULT_SOURCE 1

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <utime.h>

enum { MAXN = 100, MODEBITS = 07777, ROOTINO = 2, DELIM = '/', DIRSIZ = 14 };
static const char dot[] = ".";
static const char dotdot[] = "..";
static const char sdelim[] = "/";

struct	stat s1, s2;

int check(char *spth, ino_t dinode);
int chkdot(char *s);
char * dname(char *name);
int move(char *source, char *target);
int mvdir(char *source, char *target);
char * pname(char *name);

int main(int argc, char *argv[])
{
	register int i, r;

	if (argc < 3)
		goto usage;
	if (stat(argv[1], &s1) < 0) {
		fprintf(stderr, "mv: cannot access %s\n", argv[1]);
		return(1);
	}
	if ((s1.st_mode & S_IFMT) == S_IFDIR) {
		if (argc != 3)
			goto usage;
		return mvdir(argv[1], argv[2]);
	}
	setuid(getuid());
	if (argc > 3)
		if (stat(argv[argc-1], &s2) < 0 || (s2.st_mode & S_IFMT) != S_IFDIR)
			goto usage;
	r = 0;
	for (i=1; i<argc-1; i++)
		r |= move(argv[i], argv[argc-1]);
	return(r);
usage:
	fprintf(stderr, "usage: mv f1 f2; or mv d1 d2; or mv f1 ... fn d1\n");
	return(1);
}

int move(char *source, char *target)
{
	register int c, i;
	int	status;
	char	buf[MAXN];

	if (stat(source, &s1) < 0) {
		fprintf(stderr, "mv: cannot access %s\n", source);
		return(1);
	}
	if ((s1.st_mode & S_IFMT) == S_IFDIR) {
		fprintf(stderr, "mv: directory rename only\n");
		return(1);
	}
	if (stat(target, &s2) >= 0) {
		if ((s2.st_mode & S_IFMT) == S_IFDIR) {
			sprintf(buf, "%s/%s", target, dname(source));
			target = buf;
		}
		if (stat(target, &s2) >= 0) {
			if ((s2.st_mode & S_IFMT) == S_IFDIR) {
				fprintf(stderr, "mv: %s is a directory\n", target);
				return(1);
			}
			if (s1.st_dev==s2.st_dev && s1.st_ino==s2.st_ino) {
				fprintf(stderr, "mv: %s and %s are identical\n",
						source, target);
				return(1);
			}
			if (access(target, 2) < 0 && isatty(fileno(stdin))) {
				fprintf(stderr, "mv: %s: %o mode ", target,
					s2.st_mode & MODEBITS);
				i = c = getchar();
				while (c != '\n' && c != EOF)
					c = getchar();
				if (i != 'y')
					return(1);
			}
			if (unlink(target) < 0) {
				fprintf(stderr, "mv: cannot unlink %s\n", target);
				return(1);
			}
		}
	}
	if (link(source, target) < 0) {
		i = fork();
		if (i == -1) {
			fprintf(stderr, "mv: try again\n");
			return(1);
		}
		if (i == 0) {
			execl("/bin/cp", "cp", source, target, (char *)0);
			fprintf(stderr, "mv: cannot exec cp\n");
			exit(1);
		}
		while ((c = wait(&status)) != i && c != -1)
			;
		if (status != 0)
			return(1);
		/* V7 utime took (name, time_t *atime) and read mtime as the next
		 * word; a 64-bit host's utime(2) takes a struct utimbuf. */
		{
		struct utimbuf ut;
		ut.actime = s1.st_atime;
		ut.modtime = s1.st_mtime;
		utime(target, &ut);
		}
	}
	if (unlink(source) < 0) {
		fprintf(stderr, "mv: cannot unlink %s\n", source);
		return(1);
	}
	return(0);
}

int mvdir(char *source, char *target)
{
	register char *p;
	register int i;
	char buf[MAXN];

	if (stat(target, &s2) >= 0) {
		if ((s2.st_mode&S_IFMT) != S_IFDIR) {
			fprintf(stderr, "mv: %s exists\n", target);
			return(1);
		}
		if (strlen(target) > MAXN-DIRSIZ-2) {
			fprintf(stderr, "mv :target name too long\n");
			return(1);
		}
		strcpy(buf, target);
		target = buf;
		strcat(buf, sdelim);
		strcat(buf, dname(source));
		if (stat(target, &s2) >= 0) {
			fprintf(stderr, "mv: %s exists\n", buf);
			return(1);
		}
	}
	if (strcmp(source, target) == 0) {
		fprintf(stderr, "mv: ?? source == target, source exists and target doesnt\n");
		return(1);
	}
	p = dname(source);
	if (!strcmp(p, dot) || !strcmp(p, dotdot) || !strcmp(p, "") || p[strlen(p)-1]=='/') {
		fprintf(stderr, "mv: cannot rename %s\n", p);
		return(1);
	}
	if (stat(pname(source), &s1) < 0 || stat(pname(target), &s2) < 0) {
		fprintf(stderr, "mv: cannot locate parent\n");
		return(1);
	}
	if (access(pname(target), 2) < 0) {
		fprintf(stderr, "mv: no write access to %s\n", pname(target));
		return(1);
	}
	if (access(pname(source), 2) < 0) {
		fprintf(stderr, "mv: no write access to %s\n", pname(source));
		return(1);
	}
	if (access(source, 2) < 0) {
		fprintf(stderr, "mv: no write access to %s\n", source);
		return(1);
	}
	if (s1.st_dev != s2.st_dev) {
		fprintf(stderr, "mv: cannot move directories across devices\n");
		return(1);
	}
	if (s1.st_ino != s2.st_ino) {
		char dst[MAXN+5];

		if (chkdot(source) || chkdot(target)) {
			fprintf(stderr, "mv: Sorry, path names including %s aren't allowed\n", dotdot);
			return(1);
		}
		stat(source, &s1);
		if (check(pname(target), s1.st_ino))
			return(1);
		for (i = 1; i < NSIG; i++)
			signal(i, SIG_IGN);
		if (link(source, target) < 0) {
			fprintf(stderr, "mv: cannot link %s to %s\n", target, source);
			return(1);
		}
		if (unlink(source) < 0) {
			fprintf(stderr, "mv: %s: cannot unlink\n", source);
			unlink(target);
			return(1);
		}
		strcpy(dst, target);
		strcat(dst, "/");
		strcat(dst, dotdot);
		if (unlink(dst) < 0) {
			fprintf(stderr, "mv: %s: cannot unlink\n", dst);
			if (link(target, source) >= 0)
				unlink(target);
			return(1);
		}
		if (link(pname(target), dst) < 0) {
			fprintf(stderr, "mv: cannot link %s to %s\n",
				dst, pname(target));
			if (link(pname(source), dst) >= 0)
				if (link(target, source) >= 0)
					unlink(target);
			return(1);
		}
		return(0);
	}
	if (link(source, target) < 0) {
		fprintf(stderr, "mv: cannot link %s and %s\n",
			source, target);
		return(1);
	}
	if (unlink(source) < 0) {
		fprintf(stderr, "mv: ?? cannot unlink %s\n", source);
		return(1);
	}
	return(0);
}

char * pname(char *name)
{
	register int c;
	register char *p, *q;
	static	char buf[MAXN];

	p = q = buf;
	while (c = *p++ = *name++)
		if (c == DELIM)
			q = p-1;
	if (q == buf && *q == DELIM)
		q++;
	*q = 0;
	return buf[0]? buf : dot;
}

char * dname(char *name)
{
	register char *p;

	p = name;
	while (*p)
		if (*p++ == DELIM && *p)
			name = p;
	return name;
}

int check(char *spth, ino_t dinode)
{
	char nspth[MAXN];
	struct stat sbuf;

	sbuf.st_ino = 0;

	strcpy(nspth, spth);
	while (sbuf.st_ino != ROOTINO) {
		if (stat(nspth, &sbuf) < 0) {
			fprintf(stderr, "mv: cannot access %s\n", nspth);
			return(1);
		}
		if (sbuf.st_ino == dinode) {
			fprintf(stderr, "mv: cannot move a directory into itself\n");
			return(1);
		}
		if (strlen(nspth) > MAXN-2-sizeof(dotdot)) {
			fprintf(stderr, "mv: name too long\n");
			return(1);
		}
		strcat(nspth, sdelim);
		strcat(nspth, dotdot);
	}
	return(0);
}

int chkdot(char *s)
{
	do {
		if (strcmp(dname(s), dotdot) == 0)
			return(1);
		s = pname(s);
	} while (strcmp(s, dot) != 0 && strcmp(s, sdelim) != 0);
	return(0);
}
