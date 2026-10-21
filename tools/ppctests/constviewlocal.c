/* expect: 1 -- the first byte of an automatic whose constant value is known (zlib's byte-order test, big-endian: the first byte of 1 is 0); folds only on the straight line */
int g;
int f(p) int *p; { return *p; }

int fold()
{
	unsigned short endian;

	endian = 1;
	if (*(unsigned char *)&endian)
		return 1;
	return 2;
}

int later()
{
	unsigned short e;
	int r;

	e = 1;
	r = *(unsigned char *)&e;
	e = 0x200;
	return r + *(unsigned char *)&e;	/* 1 + 0 */
}

int loop()
{
	unsigned short e;
	int i, s;

	e = 1;
	s = 0;
	for (i = 0; i < 3; i++) {
		s += *(unsigned char *)&e;
		e = 0x100;
	}
	return s;				/* 0 + 1 + 1 */
}

int escaped()
{
	unsigned short e;
	unsigned short *q;

	q = &e;
	e = 1;
	*q = 0x300;
	return *(unsigned char *)&e;		/* 3 */
}

int test()
{
	if (fold() != 2)
		return 2;
	if (later() != 2)
		return 3;
	if (loop() != 2)
		return 4;
	if (escaped() != 3)
		return 5;
	return 1;
}
