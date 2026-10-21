/* expect: 1 -- the first byte of an automatic whose constant value is known (zlib's byte-order test); folds only on the straight line */
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
	return s;				/* 1 + 0 + 0 */
}

int escaped()
{
	unsigned short e;
	unsigned short *q;

	q = &e;
	e = 1;
	*q = 0x300;
	return *(unsigned char *)&e;		/* 0 */
}

int peek(p) char *p; { return *p; }

/* a char local whose address is taken twice is read as itself, never as a view of itself */
int twice()
{
	char b;
	int r;

	r = 0;
	b = 1;
	if (peek(&b) != 1) r |= 1;
	b = 2;
	if (peek(&b) != 2) r |= 2;
	return r;
}

/* a store through a byte view of a local is a store */
int poke()
{
	unsigned short e;

	e = 0x101;
	*(unsigned char *)&e = 0;
	return e != 0x101;
}

int test()
{
	if (fold() != 1)
		return 2;
	if (later() != 1)
		return 3;
	if (loop() != 1)
		return 4;
	if (escaped() != 0)
		return 5;
	if (twice() != 0)
		return 6;
	if (poke() != 1)
		return 7;
	return 1;
}
