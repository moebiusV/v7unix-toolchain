/* expect: 10 -- 32-bit shifts need an even register pair (ashc) */
char ctab[128];
char numbuf[64];
long lcval;
int cval, peekc;
char *inp;
int getchar(void) { return *inp ? *inp++ : 0; }

int g(void)
{
	register char *np;
	register int c, base;
	int sym, ndigit, maxdigit;

	lcval = 0;
	base = 10;
	maxdigit = 0;
	np = numbuf;
	ndigit = 0;
	sym = 1;
	if ((c = getchar()) == '0')
		base = 8;
	for (;; c = getchar()) {
		*np++ = c;
		if (ctab[c] == 1) {
			if (base == 8)
				lcval <<= 3;
			else
				lcval = ((lcval<<2) + lcval)<<1;
			c -= '0';
			lcval += c;
			ndigit++;
			if (c > maxdigit)
				maxdigit = c;
			continue;
		}
		if (c == '.') {
			sym = 2;
			base = 10;
			continue;
		}
		break;
	}
	peekc = c;
	cval = lcval;
	return sym;
}

int test(void)
{
	int i;
	for (i = '0'; i <= '9'; i++)
		ctab[i] = 1;
	inp = "10]";
	g();
	return cval;
}
