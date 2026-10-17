/* expect: 1 -- dense switches (jump tables), with and without default; negative, offset, unsigned and char cases; fallthrough */
int dense(c) int c;
{
	switch (c) {
	case 10: return 1;
	case 11: return 2;
	case 12: case 13: return 3;
	case 15: return 4;
	case 16: return 5;
	case 17: return 6;
	default: return -1;
	}
}

int nodef(c) int c;
{
	int r;

	r = 100;
	switch (c) {
	case 0: r = 1; break;
	case 1: r = 2; break;
	case 2: r = 3; break;
	case 3: r = 4; break;
	case 5: r = 5; break;
	}
	return r;
}

int negs(c) int c;
{
	switch (c) {
	case -3: return 30;
	case -2: return 20;
	case -1: return 10;
	case 0: return 0;
	case 1: return 11;
	case 2: return 22;
	default: return 99;
	}
}

int fall(c) int c;
{
	int n;

	n = 0;
	switch (c) {
	case 1: n += 1;
	case 2: n += 2;
	case 3: n += 4;
	case 4: n += 8; break;
	case 5: n += 16;
	case 7: n += 32; break;
	default: n = -1;
	}
	return n;
}

int uns(c) unsigned c;
{
	switch (c) {
	case 65530: return 1;
	case 65531: return 2;
	case 65532: return 3;
	case 65533: return 4;
	case 65535: return 5;
	case 0: return 6;
	default: return 0;
	}
}

int chars(c) char c;
{
	switch (c) {
	case 'a': case 'b': case 'c': case 'd': return 1;
	case 'e': case 'f': return 2;
	case 'h': return 3;
	case 'i': return 4;
	case 'j': return 5;
	}
	return 0;
}

int loop(n) int n;
{
	int i, s;

	s = 0;
	for (i = 0; i < n; i++) {
		switch (i % 8) {
		case 0: continue;
		case 1: s += 1; break;
		case 2: s += 10; break;
		case 3: s += 100; break;
		case 4: s += 1000; break;
		case 5: s += 3; break;
		case 6: s += 30; break;
		default: s += 300;
		}
	}
	return s;
}

int test(void)
{
	int bad = 0, i;
	unsigned u;

	if (dense(10) != 1 || dense(11) != 2 || dense(12) != 3 || dense(13) != 3) bad |= 1;
	if (dense(14) != -1 || dense(15) != 4 || dense(17) != 6 || dense(18) != -1 || dense(9) != -1) bad |= 2;
	if (dense(-32768) != -1 || dense(32767) != -1 || dense(0) != -1) bad |= 4;
	if (nodef(0) != 1 || nodef(3) != 4 || nodef(4) != 100 || nodef(5) != 5 || nodef(6) != 100 || nodef(-1) != 100) bad |= 8;
	if (negs(-3) != 30 || negs(-1) != 10 || negs(0) != 0 || negs(2) != 22 || negs(3) != 99 || negs(-4) != 99) bad |= 16;
	if (fall(1) != 15 || fall(2) != 14 || fall(4) != 8 || fall(5) != 48 || fall(6) != -1 || fall(7) != 32 || fall(0) != -1) bad |= 32;
	u = 65530; if (uns(u) != 1) bad |= 64;
	u = 65533; if (uns(u) != 4) bad |= 64;
	u = 65534; if (uns(u) != 0) bad |= 64;
	u = 65535; if (uns(u) != 5) bad |= 64;
	u = 0; if (uns(u) != 6) bad |= 64;
	u = 1; if (uns(u) != 0) bad |= 64;
	u = 32768; if (uns(u) != 0) bad |= 64;
	if (chars('a') != 1 || chars('d') != 1 || chars('f') != 2 || chars('g') != 0 || chars('j') != 5 || chars('k') != 0 || chars('`') != 0) bad |= 128;
	if (loop(8) != 1 + 10 + 100 + 1000 + 3 + 30 + 300) bad |= 256;
	if (loop(17) != 2 * (1 + 10 + 100 + 1000 + 3 + 30 + 300)) bad |= 512;
	for (i = 37; i < 300; i += 37)
		if (nodef(i) != 100) bad |= 1024;
	return bad == 0;
}
