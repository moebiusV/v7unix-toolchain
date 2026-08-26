#!/usr/bin/env python3
"""Emit a host-compilable c2.c from the PDP-11-target c2.c.

c2.c is V7's c2 modernized for the PDP-11 target (int16_t, sbrk()).
The host build needs malloc() instead of sbrk(), and <stdarg.h> instead of the
K&R `(&ap)[1]` varargs walk.  This is a mechanical transform, not a fork: the
committed source stays the single c2.c.
"""
import sys

def transform(src):
    s = src
    s = s.replace(
        '#include <unistd.h>\n#include <stdint.h>\n',
        '#include <stdint.h>\n#include <stdarg.h>\n')
    s = s.replace(
        'int16_t\tlastseg;\nchar\t*lasta;\nchar\t*lastr;\nchar\t*firstr;\nchar\trevbr[];',
        'int16_t\tlastseg;\nchar\trevbr[];')
    s = s.replace('\tlasta = firstr = lastr = sbrk(2);\n\tmaxiter = 0;',
                  '\tmaxiter = 0;')
    s = s.replace('\t\t\tmaxiter = niter;\n\t\tlasta = firstr;\n\t} while (isend);',
                  '\t\t\tmaxiter = niter;\n\t} while (isend);')
    s = s.replace('\t\tfprintf(stderr, "%d literals eliminated\\n", nlit);\n'
                  '\t\tfprintf(stderr, "%dK core\\n", (((int16_t)lastr+01777)>>10)&077);\n\t}',
                  '\t\tfprintf(stderr, "%d literals eliminated\\n", nlit);\n\t}')
    # sbrk-based alloc -> malloc
    s = s.replace('''char * alloc(int16_t n)
{
	register char *p;

	n++;
	n &= ~01;
	if (lasta+n >= lastr) {
		if (sbrk(2000) == (char *)-1) {
			fprintf(stderr, "C Optimizer: out of space\\n");
			exit(1);
		}
		lastr += 2000;
	}
	p = lasta;
	lasta += n;
	return(p);
}''', '''char * alloc(int16_t n)
{
	n++;
	n &= ~01;
	return((char *)malloc(n));
}''')
    # K&R varargs copy -> <stdarg.h>
    s = s.replace('''char * copy(int16_t na, char *ap, ...)
{
	register char *p, *np;
	char *onp;
	register int16_t n;

	p = ap;
	n = 0;
	if (*p==0)
		return(0);
	do
		n++;
	while (*p++);
	if (na>1) {
		p = (&ap)[1];
		while (*p++)
			n++;
	}
	onp = np = alloc(n);
	p = ap;
	while (*np++ = *p++)
		;
	if (na>1) {
		p = (&ap)[1];
		np--;
		while (*np++ = *p++);
	}
	return(onp);
}''', '''char * copy(int16_t na, char *ap, ...)
{
	va_list apv;
	register char *p, *np;
	char *onp;
	register int16_t n, i;

	p = ap;
	n = 0;
	if (*p==0)
		return(0);
	do
		n++;
	while (*p++);
	va_start(apv, ap);
	for (i=1; i<na; i++) {
		p = va_arg(apv, char *);
		while (*p++)
			n++;
	}
	va_end(apv);
	onp = np = alloc(n);
	p = ap;
	while (*np++ = *p++)
		;
	va_start(apv, ap);
	for (i=1; i<na; i++) {
		p = va_arg(apv, char *);
		np--;
		while (*np++ = *p++);
	}
	va_end(apv);
	return(onp);
}''')
    return s

def main():
    if len(sys.argv) != 2:
        print("usage: build-host.py c2.c", file=sys.stderr)
        return 1
    with open(sys.argv[1]) as f:
        src = f.read()
    sys.stdout.write(transform(src))
    return 0

if __name__ == '__main__':
    sys.exit(main())
