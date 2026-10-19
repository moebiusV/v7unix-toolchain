#!/bin/sh
#
# tf-check -- tf.c against the host's own binary128 (libgcc's __addtf3 and friends, libquadmath for
# printing), on random operands and the awkward ones: zeros, infinities, NaNs, subnormals, near-ties.
#
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
W=$(mktemp -d "${TMPDIR:-/tmp}/tfcheck.XXXXXX")
trap 'rm -rf "$W"' EXIT
cat > "$W/main.c" <<'EOC'
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
typedef __float128 qd;
typedef unsigned __int128 u128;
qd my_addtf3(qd, qd); qd my_subtf3(qd, qd); qd my_multf3(qd, qd); qd my_divtf3(qd, qd);
int my_eqtf2(qd, qd), my_netf2(qd, qd), my_lttf2(qd, qd), my_letf2(qd, qd), my_gttf2(qd, qd), my_getf2(qd, qd);
qd my_extendsftf2(float); qd my_extenddftf2(double); float my_trunctfsf2(qd); double my_trunctfdf2(qd);
qd my_floatsitf(int); qd my_floatunsitf(unsigned); qd my_floatditf(long long); qd my_floatunditf(unsigned long long);
int my_fixtfsi(qd); long long my_fixtfdi(qd); unsigned my_fixunstfsi(qd); unsigned long long my_fixunstfdi(qd);

static uint64_t s = 88172645463325252ULL;
static uint64_t rnd(void) { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return s; }
static qd mk(void)
{
	u128 v;
	int k = rnd() % 12;

	v = ((u128)rnd() << 64) | rnd();
	switch (k) {
	case 0: v &= ~((u128)1 << 127); v = (v & (((u128)1 << 112) - 1)) | ((u128)(rnd() % 40 + 16383 - 20) << 112); break;	/* near 1 */
	case 1: v = (v & (((u128)1 << 112) - 1)) | (v & ((u128)1 << 127)); break;						/* subnormal */
	case 2: v = (v & ((u128)1 << 127)); break;									/* +-0 */
	case 3: v = (v & ((u128)1 << 127)) | ((u128)0x7fff << 112); break;						/* inf */
	case 4: v = (v & ((u128)1 << 127)) | ((u128)0x7fff << 112) | 1; break;						/* nan */
	case 5: v = (v & (((u128)1 << 112) - 1)) | (v & ((u128)1 << 127)) | ((u128)1 << 112); break;			/* tiny normal */
	case 6: v = (v & ((u128)1 << 127)) | ((u128)0x7ffe << 112) | (v & (((u128)1 << 112) - 1)); break;		/* huge */
	case 7: v &= ~(((u128)1 << 60) - 1); break;									/* few bits set */
	default: break;
	}
	{ qd q; memcpy(&q, &v, 16); return q; }
}
static int same(qd a, qd b)
{
	u128 x, y;
	memcpy(&x, &a, 16); memcpy(&y, &b, 16);
	if (a != a && b != b) return 1;		/* both NaN */
	return x == y;
}
static int sgn(int x) { return x < 0 ? -1 : x > 0; }
int main(void)
{
	long bad = 0, n = 0, i;
	for (i = 0; i < 400000; i++) {
		qd a = mk(), b = mk();
		qd r;
#define CHK(name, mine, host) do { n++; r = mine; if (!same(r, host)) { if (bad++ < 12) { u128 x, y, z, w; memcpy(&x,&a,16); memcpy(&y,&b,16); memcpy(&z,&r,16); r = host; memcpy(&w,&r,16); printf("BAD %s %016llx%016llx %016llx%016llx: %016llx%016llx want %016llx%016llx\n", name, (unsigned long long)(x>>64),(unsigned long long)x,(unsigned long long)(y>>64),(unsigned long long)y,(unsigned long long)(z>>64),(unsigned long long)z,(unsigned long long)(w>>64),(unsigned long long)w); } } } while (0)
		CHK("add", my_addtf3(a, b), a + b);
		CHK("sub", my_subtf3(a, b), a - b);
		CHK("mul", my_multf3(a, b), a * b);
		CHK("div", my_divtf3(a, b), a / b);
#define CMP(name, mine, expr) do { n++; if ((mine) != (expr)) { if (bad++ < 12) printf("BAD %s\n", name); } } while (0)
		CMP("lt", my_lttf2(a, b) < 0, a < b);
		CMP("le", my_letf2(a, b) <= 0, a <= b);
		CMP("gt", my_gttf2(a, b) > 0, a > b);
		CMP("ge", my_getf2(a, b) >= 0, a >= b);
		CMP("eq", my_eqtf2(a, b) == 0, a == b);
		CMP("ne", my_netf2(a, b) != 0, a != b);
		CHK("trunc-d", my_extenddftf2(my_trunctfdf2(a)), (qd)(double)a);
		CHK("trunc-f", my_extendsftf2(my_trunctfsf2(a)), (qd)(float)a);
		{ double d; float f; unsigned long long w = rnd(); unsigned w32 = (unsigned)rnd(); memcpy(&d, &w, 8); memcpy(&f, &w32, 4);
		  CHK("ext-d", my_extenddftf2(d), (qd)d);
		  CHK("ext-f", my_extendsftf2(f), (qd)f); }
		{ long long v = (long long)rnd() >> (rnd() % 64); unsigned long long u = rnd() >> (rnd() % 64); int iv = (int)rnd() >> (rnd() % 32); unsigned uv = (unsigned)rnd() >> (rnd() % 32);
		  CHK("fromll", my_floatditf(v), (qd)v);
		  CHK("fromull", my_floatunditf(u), (qd)u);
		  CHK("fromi", my_floatsitf(iv), (qd)iv);
		  CHK("fromu", my_floatunsitf(uv), (qd)uv);
		  { qd q = (qd)v + (qd)0.75Q; CMP("tolonglong", my_fixtfdi(q), (long long)q); q = (qd)iv * 1.5Q; CMP("toint", my_fixtfsi(q), (int)q);
		    q = (qd)u + (qd)0.5Q; CMP("toulonglong", my_fixunstfdi(q), (unsigned long long)q); q = (qd)uv + (qd)0.25Q; CMP("touint", my_fixunstfsi(q), (unsigned)q); } }
	}
	printf("tf-check: %ld checks, %ld bad\n", n, bad);
	return bad != 0;
}
EOC
gcc -O1 -w -DTFHOST -DTFPFX=my_ -c "$here/tf.c" -o "$W/tf.o"
gcc -O1 -w -o "$W/main" "$W/main.c" "$W/tf.o" -lquadmath
"$W/main"
