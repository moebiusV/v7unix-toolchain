/* expect: 1 -- x++; and x--; as statements on int, long, unsigned, char and pointers in memory */
int gi;
long gl;
unsigned gu;
char gc;
int *gp, arr[4];
struct s { int a; long b; } st;

int test()
{
	int bad, li, r;
	long ll;
	char lc;
	int *lp;

	bad = 0;
	gi = 5; gi++; gi++; gi--;
	if (gi != 6) bad |= 1;
	gl = 70000L; gl++; gl--; gl--;
	if (gl != 69999L) bad |= 2;
	gu = ~0u; gu++;
	if (gu != 0) bad |= 4;
	gc = 127; gc++;
	if (gc != -128) bad |= 8;
	gp = arr; gp++; gp++; gp--;
	if (gp != arr + 1) bad |= 16;
	li = 9; li++; --li; li--;
	if (li != 8) bad |= 32;
	ll = -1L; ll++;
	if (ll != 0L) bad |= 64;
	lc = 0; lc--;
	if (lc != -1) bad |= 128;
	lp = arr + 3; lp--;
	if (lp != arr + 2) bad |= 256;
	st.a = 1; st.b = 2L; st.a++; st.b--;
	if (st.a != 2 || st.b != 1L) bad |= 512;
	r = gi++;
	if (r != 6 || gi != 7) bad |= 1024;
	r = --gi + gi--;
	if (gi != 5) bad |= 2048;
	return bad ? 2 * bad + 1000 : 1;
}
