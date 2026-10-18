/* expect: 1 -- the byte-copy loop, a store through a pointer in memory that is stepped, pre-decrement stored through, and a converted value tested */
struct iob { int cnt; char *ptr; int flag; };
struct iob it;
char dst[16], src[16];

copy(d, s) char *d, *s; { char *o; o = d; while (*d++ = *s++) ; return d - o; }
put(c, p) int c; struct iob *p; { *p->ptr++ = c; return p->cnt; }
unput(c, p) int c; struct iob *p; { if (c < 0) return -1; p->cnt++; *--p->ptr = c; return 0; }
skip(p) struct iob *p; { *p->ptr++; return p->cnt; }
count(s) char *s; { int c, n; n = 0; while ((c = *s++)) n++; return n; }
setflag(p, b) struct iob *p; char *b;
{
	if ((p->ptr = b) == 0) p->flag |= 4;
	else p->cnt = 9;
	p->flag &= ~12;
}

int test()
{
	int bad;

	bad = 0;
	src[0] = 'o'; src[1] = 'k'; src[2] = 0;
	if (copy(dst, src) != 3 || dst[0] != 'o' || dst[1] != 'k' || dst[2] != 0) bad |= 2;
	it.ptr = dst; it.cnt = 5;
	if (put('Z', &it) != 5 || dst[0] != 'Z' || it.ptr != dst + 1) bad |= 4;
	if (unput('Y', &it) != 0 || it.cnt != 6 || it.ptr != dst || dst[0] != 'Y') bad |= 8;
	it.ptr = dst;
	if (skip(&it) != 6 || it.ptr != dst + 1) bad |= 16;
	if (count("hello") != 5 || count("") != 0) bad |= 32;
	it.flag = 15;
	setflag(&it, 0);
	if (it.flag != 3) bad |= 64;
	it.flag = 15;
	setflag(&it, dst);
	if (it.flag != 3 || it.cnt != 9) bad |= 128;
	return bad ? 2 * bad + 1000 : 1;
}
