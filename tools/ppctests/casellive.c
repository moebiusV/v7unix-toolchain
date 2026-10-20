/* expect: 1 -- a variable live across a dense switch (VAX casel jumps through a table the flow graph must know about) */
int sel;

int viaglobal(a) int a;
{
	int r;

	r = a;
	switch (sel) {
	case 0: r += 1; break;
	case 1: r += 2; break;
	case 2: r += 4; break;
	case 3: r += 8; break;
	case 5: r += 16; break;
	}
	return r;
}

int viaparam(c, a) int c, a;
{
	int r, s;

	r = a;
	s = a + 1;
	switch (c) {
	case 1: r += s; break;
	case 2: r -= s; break;
	case 3: r += 2 * s; break;
	case 4: r = s; break;
	}
	return r + s;
}

int test()
{
	int bad;

	bad = 0;
	sel = 0; if (viaglobal(100) != 101) bad |= 2;
	sel = 3; if (viaglobal(100) != 108) bad |= 4;
	sel = 5; if (viaglobal(100) != 116) bad |= 8;
	sel = 4; if (viaglobal(100) != 100) bad |= 16;
	if (viaparam(1, 10) != 32) bad |= 32;
	if (viaparam(2, 10) != 10) bad |= 64;
	if (viaparam(3, 10) != 43) bad |= 128;
	if (viaparam(4, 10) != 22) bad |= 256;
	if (viaparam(9, 10) != 21) bad |= 512;
	return bad ? 2 * bad + 1000 : 1;
}
