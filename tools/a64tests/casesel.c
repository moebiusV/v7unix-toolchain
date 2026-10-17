/* expect: 1 -- dense switches (jump tables), with and without default, negative and byte selectors */
int hit;

int dense(c) int c;
{
	switch (c) {
	case 1: return 10;
	case 2: return 20;
	case 3: return 30;
	case 4: return 40;
	case 6: return 60;
	case 7: return 70;
	case 8: return 80;
	}
	return -1;
}

int neg(c) int c;
{
	int r;

	r = 0;
	switch (c) {
	case -3: r += 1;
	case -2: r += 2; break;
	case -1: r += 4; break;
	case 0: r += 8; break;
	case 1: r += 16; break;
	case 2: r += 32; break;
	default: r = 99;
	}
	return r;
}

int byte(c) char c;
{
	switch (c) {
	case 'a': return 1;
	case 'b': return 2;
	case 'c': return 3;
	case 'd': return 4;
	case 'e': return 5;
	case 'g': return 7;
	}
	return 0;
}

unsigned uns(u) unsigned u;
{
	switch (u) {
	case 0: return 100;
	case 1: return 101;
	case 2: return 102;
	case 3: return 103;
	case 5: return 105;
	case 6: return 106;
	}
	return 7;
}

int test()
{
	int bad, i;

	bad = 0;
	if (dense(1) != 10 || dense(4) != 40 || dense(8) != 80 || dense(6) != 60) bad |= 1;
	if (dense(5) != -1 || dense(0) != -1 || dense(9) != -1 || dense(-1) != -1 || dense(1000) != -1) bad |= 2;
	if (neg(-3) != 3 || neg(-2) != 2 || neg(-1) != 4 || neg(0) != 8 || neg(2) != 32) bad |= 4;
	if (neg(-4) != 99 || neg(3) != 99 || neg(-1000) != 99) bad |= 8;
	if (byte('a') != 1 || byte('e') != 5 || byte('g') != 7 || byte('f') != 0 || byte('z') != 0) bad |= 16;
	if (uns(0) != 100 || uns(6) != 106 || uns(4) != 7 || uns(7) != 7 || uns(~0u) != 7) bad |= 32;
	i = 0;
	switch (i) { case 0: hit = 1; break; case 1: hit = 2; break; case 2: hit = 3; break; case 3: hit = 4; }
	if (hit != 1) bad |= 64;
	return bad ? bad : 1;
}
