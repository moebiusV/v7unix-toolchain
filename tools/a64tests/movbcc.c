/* expect: 1 -- the condition codes after movb to a register serve the branch: negative, zero and positive bytes */
int cnt(p) register char *p; { register int c; int n; n = 0; while (c = *p++) n++; return n; }
int sgn(p) char *p; { register int c; c = *p; if (c < 0) return -1; if (c == 0) return 0; return 1; }
int sgn2(p) register char *p; { register int c; if ((c = *p++) < 0) return -1; else if (c) return 1; return 0; }
int neg(p) char *p; { register int c; int n; n = 0; while ((c = *p++) >= 0) { if (c == 0) break; n++; } return n; }

int test(void)
{
	int bad = 0;
	char a[6], z[2], m[2], u[3];

	a[0] = 1; a[1] = -1; a[2] = 127; a[3] = -128; a[4] = 5; a[5] = 0;
	z[0] = 0; z[1] = 9;
	m[0] = -7; m[1] = 0;
	u[0] = 3; u[1] = 4; u[2] = 0;
	if (cnt(a) != 5) bad |= 1;
	if (cnt(z) != 0) bad |= 2;
	if (cnt(m) != 1) bad |= 4;
	if (sgn(m) != -1 || sgn(z) != 0 || sgn(u) != 1) bad |= 8;
	if (sgn2(m) != -1 || sgn2(z) != 0 || sgn2(u) != 1) bad |= 16;
	if (neg(u) != 2 || neg(a) != 1 || neg(m) != 0) bad |= 32;
	return bad == 0;
}
