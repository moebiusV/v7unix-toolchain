/* expect: 1 -- (x & mask) tested against zero: char, uchar, short and int operands, through pointers and indexed */
char gc; unsigned char guc; short gs; int gi;
char carr[4]; short sarr[4];

int bc(p) char *p; { if (*p & 0200) return 1; return 0; }
int buc(p) unsigned char *p; { if (*p & 0200) return 1; return 0; }
int bs(p) short *p; { if (*p & 0x4000) return 1; return 0; }
int bi(x) int x; { if (x & 0x100) return 1; return 0; }
int bn(p) char *p; { if (!(*p & 1)) return 1; return 0; }
int bidx(i) int i; { if (carr[i] & 4) return 1; if (sarr[i] & 0x10) return 2; return 0; }
int bboth(p) char *p; { if ((*p & 1) && (*p & 0100)) return 1; if ((*p & 2) || (*p & 0200)) return 2; return 0; }
int bg() { if (gc & 0200) return 1; if (guc & 0200) return 2; if (gs & 0x100) return 3; if (gi & 0x40) return 4; return 0; }

int test()
{
	int bad;
	char c;
	unsigned char uc;
	short s;

	bad = 0;
	c = -1; if (bc(&c) != 1) bad |= 1;
	c = 0x7f; if (bc(&c) != 0) bad |= 2;
	c = 0x80; if (bc(&c) != 1) bad |= 4;
	uc = 0x80; if (buc(&uc) != 1) bad |= 8;
	uc = 0x7f; if (buc(&uc) != 0) bad |= 16;
	s = 0x4000; if (bs(&s) != 1) bad |= 32;
	s = 0x3fff; if (bs(&s) != 0) bad |= 64;
	if (bi(0x100) != 1 || bi(0xff) != 0 || bi(-1) != 1) bad |= 128;
	c = 2; if (bn(&c) != 1) bad |= 256;
	c = 3; if (bn(&c) != 0) bad |= 512;
	carr[1] = 4; sarr[2] = 0x10; carr[2] = 0; sarr[1] = 0;
	if (bidx(1) != 1 || bidx(2) != 2 || bidx(3) != 0) bad |= 1024;
	c = 0x41; if (bboth(&c) != 1) bad |= 2048;
	c = 0x02; if (bboth(&c) != 2) bad |= 2048;
	c = 0x04; if (bboth(&c) != 0) bad |= 2048;
	gc = 0; guc = 0; gs = 0; gi = 0;
	if (bg() != 0) bad |= 4096;
	gi = 0x40; if (bg() != 4) bad |= 4096;
	gs = 0x100; if (bg() != 3) bad |= 4096;
	guc = 0x80; if (bg() != 2) bad |= 4096;
	gc = -128; if (bg() != 1) bad |= 4096;
	return bad ? bad : 1;
}
