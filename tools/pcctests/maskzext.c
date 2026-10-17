/* expect: 1 -- (char)x & 0377 and (short)x & 0xffff are the value zero-extended */
char gc; short gs; unsigned char guc;

int zc(p) char *p; { return *p & 0377; }
unsigned zs(p) short *p; { return *p & 0xffff; }
int zg() { return gc & 0377; }
int zr(c) char c; { register char r; r = c; return r & 0377; }
int zi(p) char *p; { int v; v = *p++ & 0377; return v + (*p & 0377); }

int test()
{
	int bad;
	char c, buf[2];
	short s;

	bad = 0;
	c = -1; if (zc(&c) != 255) bad |= 1;
	c = 5; if (zc(&c) != 5) bad |= 2;
	c = -128; if (zc(&c) != 128) bad |= 4;
	s = -1; if (zs(&s) != 65535u) bad |= 8;
	s = 300; if (zs(&s) != 300) bad |= 16;
	gc = -2; if (zg() != 254) bad |= 32;
	if (zr(-3) != 253) bad |= 64;
	buf[0] = -1; buf[1] = 1;
	if (zi(buf) != 256) bad |= 128;
	if (((int)gc & 0377) + ((int)gc & 0377) != 508) bad |= 256;
	return bad ? bad : 1;
}
