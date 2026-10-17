/* expect: 1 -- two bytes compared with each other, signed and unsigned, with autoincrement */
int lt_s(a, b) char *a, *b; { return *a < *b; }
int le_s(a, b) char *a, *b; { return *a <= *b; }
int gt_s(a, b) char *a, *b; { return *a > *b; }
int ge_s(a, b) char *a, *b; { return *a >= *b; }
int eq_s(a, b) char *a, *b; { return *a == *b; }
int lt_u(a, b) unsigned char *a, *b; { return *a < *b; }
int ge_u(a, b) unsigned char *a, *b; { return *a >= *b; }
int gt_u(a, b) unsigned char *a, *b; { return *a > *b; }
int same(s1, s2) register char *s1, *s2; { while (*s1 == *s2++) if (*s1++ == 0) return 1; return 0; }

int test(void)
{
	int bad = 0;
	char x, y;
	unsigned char u, v;

	x = -3; y = 5;
	if (!lt_s(&x, &y) || lt_s(&y, &x) || !le_s(&x, &x) || !gt_s(&y, &x) || gt_s(&x, &y)) bad |= 1;
	if (!ge_s(&y, &x) || ge_s(&x, &y) || eq_s(&x, &y) || !eq_s(&y, &y)) bad |= 2;
	u = 200; v = 100;
	if (lt_u(&u, &v) || !lt_u(&v, &u) || !ge_u(&u, &v) || ge_u(&v, &u) || !gt_u(&u, &v)) bad |= 4;
	if (!same("abc", "abc") || same("abc", "abd") || same("abc", "ab") || !same("", "")) bad |= 8;
	return bad == 0;
}
