/* expect: 1 -- loops whose test moves to the bottom: continue, break, nesting, a test with side effects, a float test, a loop at the top of the function */
int lr_sum(n) int n; { int i, s; s = 0; i = 0; while (i < n) { s += i; i++; } return s; }
int lr_cont(n) int n; { int i, s; s = 0; for (i = 0; i < n; i++) { if (i == 3) continue; s += i; } return s; }
int lr_brk(n) int n; { int i; for (i = 0; i < 100; i++) if (i * i > n) break; return i; }
int lr_nest(n) int n; { int i, j, s; s = 0; for (i = 0; i < n; i++) for (j = 0; j <= i; j++) s += j; return s; }
int lr_zero(n) int n; { int c; c = 0; while (n-- > 0) c++; return c; }
int lr_dow(n) int n; { int c; c = 0; do c += 2; while (--n > 0); return c; }
int lr_side(p) char *p; { int c; c = 0; while (*p++) c++; return c; }
int lr_fl(n) int n; { double d; int c; c = 0; for (d = 0.5; d < n; d += 1.0) c++; return c; }
int lr_top(n) int n; { while (n > 10) n -= 3; return n; }

int test(void)
{
	int bad = 0;

	if (lr_sum(5) != 10 || lr_sum(0) != 0) bad |= 1;
	if (lr_cont(6) != 12 || lr_cont(2) != 1) bad |= 2;
	if (lr_brk(50) != 8 || lr_brk(0) != 1) bad |= 4;
	if (lr_nest(4) != 10 || lr_nest(0) != 0) bad |= 8;
	if (lr_zero(5) != 5 || lr_zero(0) != 0 || lr_zero(-3) != 0) bad |= 16;
	if (lr_dow(3) != 6 || lr_dow(1) != 2) bad |= 32;
	if (lr_side("abc") != 3 || lr_side("") != 0) bad |= 64;
	if (lr_fl(3) != 3 || lr_fl(0) != 0) bad |= 128;
	if (lr_top(25) != 10 || lr_top(4) != 4) bad |= 256;
	return bad == 0;
}
