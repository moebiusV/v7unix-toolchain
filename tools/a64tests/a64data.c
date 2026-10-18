/* expect: 1 -- initialized data, strings, pointer tables, structs passed and returned by value, switch, static locals, bit-fields */
struct pt { int x, y; };
struct big { long a[6]; char tag; };
struct mix { char c; short s; int i; long l; double d; float f; };

struct pt origin = { 3, 4 };
struct pt pts[3] = { { 1, 2 }, { 3, 4 }, { 5, 6 } };
char msg[] = "hello";
char *names[] = { "zero", "one", "two" };
int tbl[5] = { 10, 20, 30, 40, 50 };
long ltab[3] = { -1, 1L << 40, 7 };
double dtab[2] = { 0.5, -2.25 };
int (*fnp)(int);
unsigned char bytes[4] = { 1, 200, 3, 255 };
short shorts[3] = { -1, 32767, -32768 };
int uninit;
long uninit2[100];
static int sfile = 5;
struct mix gm = { 'x', -2, 70000, 1L << 33, 1.5, 2.5f };

int twice(int x) { return 2 * x; }

struct pt padd(struct pt a, struct pt b) { struct pt r; r.x = a.x + b.x; r.y = a.y + b.y; return r; }
struct big bmake(long n) { struct big b; int i; for (i = 0; i < 6; i++) b.a[i] = n + i; b.tag = 'B'; return b; }
long bsum(struct big b) { long s = 0; int i; for (i = 0; i < 6; i++) s += b.a[i]; return s + b.tag; }
struct mix mmake(void) { struct mix m; m.c = 'q'; m.s = 300; m.i = -9; m.l = 1L << 40; m.d = 0.25; m.f = 1.5f; return m; }
double mread(struct mix m) { return m.c + m.s + m.i + m.l + m.d + m.f; }

int sw(int x)
{
	switch (x) {
	case 0: return 10;
	case 1: return 11;
	case 2: return 12;
	case 3: return 13;
	case 5: return 15;
	case 100: return 200;
	case 1000: return 2000;
	default: return -1;
	}
}
int swc(char c) { switch (c) { case 'a': return 1; case 'b': return 2; case 'z': return 26; } return 0; }
long swl(long v) { switch (v) { case 1L << 33: return 1; case -5: return 2; default: return 3; } }

int counter(void) { static int n; return ++n; }
int fldt(void)
{
	struct { unsigned a : 3; int b : 5; unsigned c : 20; } f;
	f.a = 5; f.b = -3; f.c = 1000000;
	f.a += 4; f.b -= 20;
	return f.a * 1000000 + f.b + (f.c == 1000000 ? 1 : 0);
}

int test()
{
	int bad = 0, i;
	struct pt a, b, c;
	struct big bg;
	struct mix mm;
	char *p;
	long s;

	if (origin.x != 3 || pts[2].y != 6 || pts[1].x != 3) bad |= 1;
	if (msg[4] != 'o' || names[1][0] != 'o' || names[2][2] != 'o' || sizeof(msg) != 6) bad |= 2;
	s = 0; for (i = 0; i < 5; i++) s += tbl[i];
	if (s != 150 || ltab[1] != 1099511627776L || ltab[0] != -1 || dtab[1] != -2.25) bad |= 4;
	fnp = twice;
	if (fnp(21) != 42 || uninit != 0 || uninit2[99] != 0) bad |= 8;
	if (bytes[1] != 200 || bytes[3] != 255 || shorts[0] != -1 || shorts[2] != -32768 || sfile != 5) bad |= 16;
	a.x = 1; a.y = 2; b.x = 10; b.y = 20;
	c = padd(a, b);
	if (c.x != 11 || c.y != 22 || padd(c, c).y != 44) bad |= 32;
	bg = bmake(100);
	if (bsum(bg) != 600 + 15 + 'B' || bg.a[5] != 105) bad |= 64;
	mm = mmake();
	if (mread(mm) != 'q' + 300 + -9 + 1099511627776. + 0.25 + 1.5 || mm.l != 1L << 40) bad |= 128;
	if (gm.c != 'x' || gm.s != -2 || gm.i != 70000 || gm.l != 8589934592L || gm.d != 1.5 || gm.f != 2.5f) bad |= 256;
	if (sw(0) != 10 || sw(3) != 13 || sw(4) != -1 || sw(5) != 15 || sw(100) != 200 || sw(1000) != 2000 || sw(-1) != -1) bad |= 512;
	if (swc('a') != 1 || swc('z') != 26 || swc('c') != 0 || swl(1L << 33) != 1 || swl(-5) != 2 || swl(0) != 3) bad |= 1024;
	if (counter() != 1 || counter() != 2 || counter() != 3) bad |= 2048;
	if (fldt() != 1000010) bad |= 4096;
	p = msg; p++; p += 2;
	if (*p != 'l' || p - msg != 3 || &msg[5] - p != 2) bad |= 8192;
	return bad ? 2 * bad + 1000 : 1;
}
