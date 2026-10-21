/* expect: 1 -- memcpy of one scalar variable, to or from it: a load or a store of it, also in a function saved for inlining; the value, when used, is the destination */
extern void *memcpy();

static inline unsigned get32(p) const void *p; { unsigned r; memcpy(&r, p, sizeof(r)); return r; }
static inline void put32(p, x) void *p; unsigned x; { memcpy(p, &x, sizeof(x)); }
static inline unsigned short get16(p) const void *p; { unsigned short r; memcpy(&r, p, sizeof(r)); return r; }

unsigned char buf[16] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
unsigned char out[16];
int calls;
unsigned char *next(p) unsigned char *p; { calls++; return p; }

int test()
{
	unsigned v, w;
	float f;
	unsigned bits;
	void *r;
	int i;

	/* loads, from an odd address too */
	v = get32(buf);
	w = get32(buf + 1);
	if (v != (unsigned)(buf[0] | buf[1] << 8 | buf[2] << 16 | (unsigned)buf[3] << 24) &&
	    v != ((unsigned)buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3]))
		return 2;
	if (w == v)
		return 3;
	if (get16(buf + 3) != (unsigned short)(buf[3] | buf[4] << 8) &&
	    get16(buf + 3) != (unsigned short)(buf[3] << 8 | buf[4]))
		return 4;
	/* stores, to an odd address too */
	for (i = 0; i < 16; i++) out[i] = 0;
	put32(out + 1, 0x01020304u);
	if (out[0] != 0 || out[5] != 0)
		return 5;
	if (get32(out + 1) != 0x01020304u)
		return 6;
	/* a float read and written as bits */
	bits = 0x3f800000u;
	memcpy(&f, &bits, sizeof(f));
	if (f != 1.0f)
		return 7;
	f = 2.0f;
	memcpy(&bits, &f, sizeof(bits));
	if (bits != 0x40000000u)
		return 8;
	/* the address expression is evaluated once, and the value is the destination */
	calls = 0;
	memcpy(&v, next(buf + 4), sizeof(v));
	if (calls != 1)
		return 9;
	r = memcpy(&w, buf, sizeof(w));
	if (r != (void *)&w)
		return 10;
	r = memcpy(out + 8, &v, sizeof(v));
	if (r != (void *)(out + 8) || get32(out + 8) != v)
		return 11;
	return 1;
}
