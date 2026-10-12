/* expect: 0 -- c0 getnum(), the original -xssa miscompile */
/* getnum() from c00.c, with its environment stubbed out */
#define CON 21
#define FCON 22
#define LCON 23
#define DOT 24
#define DIGIT 124
#define MAXINT 077777
typedef int int16_t;

char ctab[128];
char numbuf[64];
long lcval;
int cval, peekc, nerrors;
char *inp;

int error(char *s) { nerrors++; return 0; }
int getchar(void) { return *inp ? *inp++ : 0; }
int spnextchar(void) { return getchar(); }

int16_t getnum(void)
{
	register char *np;
	register int16_t c, base;
	int16_t expseen, sym, ndigit;
	char *nsyn;
	int16_t maxdigit;

	nsyn = "Number syntax";
	lcval = 0;
	base = 10;
	maxdigit = 0;
	np = numbuf;
	ndigit = 0;
	sym = CON;
	expseen = 0;
	if ((c=spnextchar()) == '0')
		base = 8;
	for (;; c = getchar()) {
		*np++ = c;
		if (ctab[c]==DIGIT || (base==16) && ('a'<=c&&c<='f'||'A'<=c&&c<='F')) {
			if (base==8)
				lcval <<= 3;
			else if (base==10)
				lcval = ((lcval<<2) + lcval)<<1;
			else
				lcval <<= 4;
			if (ctab[c]==DIGIT)
				c -= '0';
			else if (c>='a')
				c -= 'a'-10;
			else
				c -= 'A'-10;
			lcval += c;
			ndigit++;
			if (c>maxdigit)
				maxdigit = c;
			continue;
		}
		if (c=='.') {
			if (base==16 || sym==FCON)
				error(nsyn);
			sym = FCON;
			base = 10;
			continue;
		}
		if (ndigit==0) {
			sym = DOT;
			break;
		}
		if ((c=='e'||c=='E') && expseen==0) {
			expseen++;
			sym = FCON;
			if (base==16 || maxdigit>=10)
				error(nsyn);
			base = 10;
			*np++ = c = getchar();
			if (c!='+' && c!='-' && ctab[c]!=DIGIT)
				break;
		} else if (c=='x' || c=='X') {
			if (base!=8 || lcval!=0 || sym!=CON)
				error(nsyn);
			base = 16;
		} else if ((c=='l' || c=='L') && sym==CON) {
			c = getchar();
			sym = LCON;
			break;
		} else
			break;
	}
	peekc = c;
	if (maxdigit >= base)
		error(nsyn);
	if (sym==FCON) {
		np[-1] = 0;
		cval = np-numbuf;
		return(FCON);
	}
	if (sym==CON && (lcval<0 || lcval>MAXINT&&base==10 || (lcval>>1)>MAXINT)) {
		sym = LCON;
	}
	cval = lcval;
	return(sym);
}

int test(void)
{
	int i, bad;

	for (i = '0'; i <= '9'; i++)
		ctab[i] = DIGIT;
	bad = 0;
	inp = "10]";   if (getnum() != CON || cval != 10) bad |= 1;
	inp = "123]";  if (getnum() != CON || cval != 123) bad |= 2;
	inp = "0777]"; if (getnum() != CON || cval != 511) bad |= 4;
	inp = "26]";   if (getnum() != CON || cval != 26) bad |= 8;
	inp = "5]";    if (getnum() != CON || cval != 5) bad |= 16;
	return bad;
}
