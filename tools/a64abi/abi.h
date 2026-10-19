/*
 * AAPCS64 struct passing, checked against clang.  This file is compiled twice, once by pcc
 * (PFX = p_, OFX = c_) and once by clang (PFX = c_, OFX = p_).  Each side defines the callees
 * with its own prefix and a check() that calls the other side's, so every class of struct is
 * exercised as a caller and as a callee against an independent compiler.
 * check() returns 1, or 100 + the number of the first call that went wrong.
 */
#define CAT2(a, b) a ## b
#define CAT(a, b) CAT2(a, b)
#define N(x) CAT(PFX, x)
#define O(x) CAT(OFX, x)

struct S4   { int a; };
struct S8   { int a, b; };
struct S12  { int a, b, c; };
struct S16  { long a, b; };
struct S24  { long a, b, c; };
struct S40  { long v[5]; };
struct H1   { double x; };
struct H2   { double x, y; };
struct H3f  { float a, b, c; };
struct H4d  { double a, b, c, d; };
struct Mix  { double d; long l; };
struct Chars { char c[3]; };
struct Nest { struct S8 in; int z; };
struct Arr  { int v[4]; };

long N(f_s4)(struct S4 s)    { return s.a; }
long N(f_s8)(struct S8 s)    { return s.a * 10 + s.b; }
long N(f_s12)(struct S12 s)  { return s.a * 100 + s.b * 10 + s.c; }
long N(f_s16)(struct S16 s)  { return s.a * 1000 + s.b; }
long N(f_s24)(struct S24 s)  { return s.a * 100 + s.b * 10 + s.c; }
long N(f_s40)(struct S40 s)  { return s.v[0] + s.v[1] * 2 + s.v[2] * 3 + s.v[3] * 4 + s.v[4] * 5; }
double N(f_h1)(struct H1 h)  { return h.x; }
double N(f_h2)(struct H2 h)  { return h.x + h.y + h.y; }
double N(f_h3)(struct H3f h) { return h.a + h.b + h.b + h.c + h.c + h.c; }
double N(f_h4)(struct H4d h) { return h.a + h.b + h.b + h.c + h.c + h.c + h.d + h.d + h.d + h.d; }
long N(f_mix)(struct Mix m)  { return (long)m.d * 100 + m.l; }
long N(f_ch)(struct Chars c) { return c.c[0] * 100 + c.c[1] * 10 + c.c[2]; }
long N(f_nest)(struct Nest n) { return n.in.a * 100 + n.in.b * 10 + n.z; }
long N(f_arr)(struct Arr a)  { return a.v[0] * 1000 + a.v[1] * 100 + a.v[2] * 10 + a.v[3]; }

/* arguments around structs, and register exhaustion */
long N(f_mixargs)(int a, struct S16 s, int b, struct H2 h, long c)
{
	return a + s.a * 10 + s.b * 100 + b * 1000 + (long)h.x * 10000 + (long)h.y * 100000 + c * 1000000;
}
long N(f_two)(struct S16 s, struct S16 t, struct S16 u, struct S16 v, struct S16 w)
{
	return s.a + t.a * 2 + u.a * 3 + v.a * 4 + w.a * 5 + w.b * 6;
}
long N(f_exhaust)(long a1, long a2, long a3, long a4, long a5, long a6, long a7,
    struct S16 s, long z)
{
	return a1 + a2 + a3 + a4 + a5 + a6 + a7 + s.a * 10 + s.b * 100 + z * 1000;
}
long N(f_big)(int a, struct S40 s, int b, struct S24 t, int c)
{
	return a + s.v[4] * 10 + b * 100 + t.c * 1000 + c * 10000;
}
double N(f_hexhaust)(double a1, double a2, double a3, double a4, double a5, double a6,
    struct H3f h, double z)
{
	return a1 + a2 + a3 + a4 + a5 + a6 + h.a + h.c + z;
}

struct S4  N(r_s4)(int a)              { struct S4 s; s.a = a; return s; }
struct S8  N(r_s8)(int a, int b)       { struct S8 s; s.a = a; s.b = b; return s; }
struct S12 N(r_s12)(int a, int b, int c) { struct S12 s; s.a = a; s.b = b; s.c = c; return s; }
struct S16 N(r_s16)(long a, long b)    { struct S16 s; s.a = a; s.b = b; return s; }
struct S24 N(r_s24)(long a, long b, long c) { struct S24 s; s.a = a; s.b = b; s.c = c; return s; }
struct S40 N(r_s40)(long a, long e)    { struct S40 s; s.v[0] = a; s.v[1] = 2; s.v[2] = 3; s.v[3] = 4; s.v[4] = e; return s; }
struct H1  N(r_h1)(double x)           { struct H1 h; h.x = x; return h; }
struct H2  N(r_h2)(double x, double y) { struct H2 h; h.x = x; h.y = y; return h; }
struct H3f N(r_h3)(float a, float b, float c) { struct H3f h; h.a = a; h.b = b; h.c = c; return h; }
struct H4d N(r_h4)(double a, double d) { struct H4d h; h.a = a; h.b = 2; h.c = 3; h.d = d; return h; }
struct Mix N(r_mix)(double d, long l)  { struct Mix m; m.d = d; m.l = l; return m; }
struct Chars N(r_ch)(char a, char b, char c) { struct Chars s; s.c[0] = a; s.c[1] = b; s.c[2] = c; return s; }
struct Nest N(r_nest)(int a, int b, int z) { struct Nest n; n.in.a = a; n.in.b = b; n.z = z; return n; }

/* the other side's functions */
long O(f_s4)(struct S4); long O(f_s8)(struct S8); long O(f_s12)(struct S12); long O(f_s16)(struct S16);
long O(f_s24)(struct S24); long O(f_s40)(struct S40); double O(f_h1)(struct H1); double O(f_h2)(struct H2);
double O(f_h3)(struct H3f); double O(f_h4)(struct H4d); long O(f_mix)(struct Mix); long O(f_ch)(struct Chars);
long O(f_nest)(struct Nest); long O(f_arr)(struct Arr);
long O(f_mixargs)(int, struct S16, int, struct H2, long);
long O(f_two)(struct S16, struct S16, struct S16, struct S16, struct S16);
long O(f_exhaust)(long, long, long, long, long, long, long, struct S16, long);
long O(f_big)(int, struct S40, int, struct S24, int);
double O(f_hexhaust)(double, double, double, double, double, double, struct H3f, double);
struct S4 O(r_s4)(int); struct S8 O(r_s8)(int, int); struct S12 O(r_s12)(int, int, int);
struct S16 O(r_s16)(long, long); struct S24 O(r_s24)(long, long, long); struct S40 O(r_s40)(long, long);
struct H1 O(r_h1)(double); struct H2 O(r_h2)(double, double); struct H3f O(r_h3)(float, float, float);
struct H4d O(r_h4)(double, double); struct Mix O(r_mix)(double, long); struct Chars O(r_ch)(char, char, char);
struct Nest O(r_nest)(int, int, int);

#ifndef STOPAT
#define STOPAT 999
#endif
#define CHK(n, cond) do { if (!(cond)) return 100 + (n); if ((n) >= STOPAT) return 1; } while (0)

int N(check)(void)
{
	struct S4 s4; struct S8 s8; struct S12 s12; struct S16 s16, t16, u16, v16, w16; struct S24 s24;
	struct S40 s40; struct H1 h1; struct H2 h2; struct H3f h3; struct H4d h4; struct Mix mx;
	struct Chars ch; struct Nest ne; struct Arr ar;
	struct S8 r8; struct S12 r12; struct S16 r16; struct S24 r24; struct S40 r40; struct H2 rh2;
	struct H3f rh3; struct H4d rh4; struct Mix rmx; struct Chars rch; struct Nest rne; struct S4 r4; struct H1 rh1;

	s4.a = 7;
	CHK(1, O(f_s4)(s4) == 7);
	s8.a = 3; s8.b = 4;
	CHK(2, O(f_s8)(s8) == 34);
	s12.a = 1; s12.b = 2; s12.c = 3;
	CHK(3, O(f_s12)(s12) == 123);
	s16.a = 5; s16.b = 6;
	CHK(4, O(f_s16)(s16) == 5006);
	s24.a = 1; s24.b = 2; s24.c = 3;
	CHK(5, O(f_s24)(s24) == 123);
	s40.v[0] = 1; s40.v[1] = 1; s40.v[2] = 1; s40.v[3] = 1; s40.v[4] = 1;
	CHK(6, O(f_s40)(s40) == 15);
	h1.x = 2.5;
	CHK(7, O(f_h1)(h1) == 2.5);
	h2.x = 1.0; h2.y = 2.0;
	CHK(8, O(f_h2)(h2) == 5.0);
	h3.a = 1.0f; h3.b = 2.0f; h3.c = 3.0f;
	CHK(9, O(f_h3)(h3) == 14.0);
	h4.a = 1.0; h4.b = 2.0; h4.c = 3.0; h4.d = 4.0;
	CHK(10, O(f_h4)(h4) == 1 + 4 + 9 + 16);
	mx.d = 3.0; mx.l = 21;
	CHK(11, O(f_mix)(mx) == 321);
	ch.c[0] = 1; ch.c[1] = 2; ch.c[2] = 3;
	CHK(12, O(f_ch)(ch) == 123);
	ne.in.a = 4; ne.in.b = 5; ne.z = 6;
	CHK(13, O(f_nest)(ne) == 456);
	ar.v[0] = 1; ar.v[1] = 2; ar.v[2] = 3; ar.v[3] = 4;
	CHK(14, O(f_arr)(ar) == 1234);

	h2.x = 4.0; h2.y = 5.0;
	CHK(15, O(f_mixargs)(1, s16, 2, h2, 3) == 1 + 50 + 600 + 2000 + 40000 + 500000 + 3000000);
	t16.a = 1; t16.b = 0; u16.a = 1; u16.b = 0; v16.a = 1; v16.b = 0; w16.a = 1; w16.b = 1;
	CHK(16, O(f_two)(t16, t16, u16, v16, w16) == 1 + 2 + 3 + 4 + 5 + 6);
	CHK(17, O(f_exhaust)(1, 2, 3, 4, 5, 6, 7, s16, 9) == 28 + 50 + 600 + 9000);
	s24.c = 3;
	CHK(18, O(f_big)(1, s40, 2, s24, 3) == 1 + 10 + 200 + 3000 + 30000);
	h3.a = 1.0f; h3.c = 5.0f;
	CHK(19, O(f_hexhaust)(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, h3, 10.0) == 21 + 6 + 10);

	r4 = O(r_s4)(9);            CHK(20, r4.a == 9);
	r8 = O(r_s8)(3, 4);         CHK(21, r8.a == 3 && r8.b == 4);
	r12 = O(r_s12)(1, 2, 3);    CHK(22, r12.a == 1 && r12.b == 2 && r12.c == 3);
	r16 = O(r_s16)(5, 6);       CHK(23, r16.a == 5 && r16.b == 6);
	r24 = O(r_s24)(1, 2, 3);    CHK(24, r24.a == 1 && r24.b == 2 && r24.c == 3);
	r40 = O(r_s40)(7, 8);       CHK(25, r40.v[0] == 7 && r40.v[1] == 2 && r40.v[4] == 8);
	rh1 = O(r_h1)(1.5);         CHK(26, rh1.x == 1.5);
	rh2 = O(r_h2)(1.0, 2.0);    CHK(27, rh2.x == 1.0 && rh2.y == 2.0);
	rh3 = O(r_h3)(1.0f, 2.0f, 3.0f); CHK(28, rh3.a == 1.0f && rh3.b == 2.0f && rh3.c == 3.0f);
	rh4 = O(r_h4)(1.0, 4.0);    CHK(29, rh4.a == 1.0 && rh4.b == 2.0 && rh4.c == 3.0 && rh4.d == 4.0);
	rmx = O(r_mix)(2.0, 9);     CHK(30, rmx.d == 2.0 && rmx.l == 9);
	rch = O(r_ch)(1, 2, 3);     CHK(31, rch.c[0] == 1 && rch.c[1] == 2 && rch.c[2] == 3);
	rne = O(r_nest)(4, 5, 6);   CHK(32, rne.in.a == 4 && rne.in.b == 5 && rne.z == 6);
	/* a returned struct used at once */
	CHK(33, O(f_s16)(O(r_s16)(1, 2)) == 1002);
	CHK(34, O(f_h2)(O(r_h2)(1.0, 2.0)) == 5.0);
	return 1;
}
