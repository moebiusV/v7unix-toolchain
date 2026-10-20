/* expect: 1 -- inline functions that need a jump table or return a pointer: they are kept as ordinary functions or expanded, never an error and never wrong */
struct S { int a; };
static struct S g = { 7 };

static inline int sw(int x)
{
	switch (x) {
	case 1: return 10;
	case 2: return 20;
	case 3: return 30;
	case 4: return 40;
	case 5: return 50;
	case 6: return 60;
	default: return 0;
	}
}

static inline struct S *gp(int i)
{
	return i ? &g : 0;
}

static inline const char *pick(const char *a, const char *b, int c)
{
	return c ? a : b;
}

int test(void)
{
	const char *s;

	if (sw(1) != 10 || sw(5) != 50 || sw(6) != 60 || sw(7) != 0 || sw(0) != 0)
		return 0;
	if (gp(1)->a != 7 || gp(0) != 0)
		return 0;
	s = pick("yes", "no", 1);
	return s[0] == 'y' && pick("yes", "no", 0)[0] == 'n';
}
