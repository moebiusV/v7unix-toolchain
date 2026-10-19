/* expect: 1 -- memcpy of 1 to 4 bytes, a constant, from an odd address: copied in place, arguments evaluated once */
extern void *memcpy();

char src[40] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20 };
char dst[40];

int bump;
char *next(p) char *p; { bump++; return p; }

int test()
{
	int i, k;
	char *r;

	for (i = 0; i < 40; i++) dst[i] = -1;
	r = memcpy(dst + 2, src + 1, 1);
	if (r != dst + 2) return 101;
	for (i = 0; i < 40; i++) {
		k = (i >= 2 && i < 2 + 1) ? src[i - 1] : -1;
		if (dst[i] != k) return 201;
	}
	for (i = 0; i < 40; i++) dst[i] = -1;
	r = memcpy(dst + 2, src + 1, 2);
	if (r != dst + 2) return 102;
	for (i = 0; i < 40; i++) {
		k = (i >= 2 && i < 2 + 2) ? src[i - 1] : -1;
		if (dst[i] != k) return 202;
	}
	for (i = 0; i < 40; i++) dst[i] = -1;
	r = memcpy(dst + 2, src + 1, 3);
	if (r != dst + 2) return 103;
	for (i = 0; i < 40; i++) {
		k = (i >= 2 && i < 2 + 3) ? src[i - 1] : -1;
		if (dst[i] != k) return 203;
	}
	for (i = 0; i < 40; i++) dst[i] = -1;
	r = memcpy(dst + 2, src + 1, 4);
	if (r != dst + 2) return 104;
	for (i = 0; i < 40; i++) {
		k = (i >= 2 && i < 2 + 4) ? src[i - 1] : -1;
		if (dst[i] != k) return 204;
	}
	/* the arguments are evaluated once, and the result is the destination */
	bump = 0;
	r = memcpy(next(dst + 5), next(src + 3), 2);
	if (bump != 2 || r != dst + 5 || dst[5] != 4 || dst[6] != 5) return 2;
	return 1;
}
/* the library's memcpy, defined after the calls: they are copied in place with -xdce, called without it */
void *memcpy(d, s, n) char *d, *s; unsigned n;
{
	char *r = d;

	while (n--)
		*d++ = *s++;
	return r;
}
