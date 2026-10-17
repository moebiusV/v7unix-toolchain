/* expect: 1 -- if/else on constants, const objects folded, dead arms keep their syntax checked but are not run */
#define V7 1
#define BSD 0
enum { ON = 1, OFF = 0 };
static const int sdebug = 0;
const int gon = 1;
const char csmall = 7;
int hits;
int marker() { hits++; return 1; }
int addr_ok(p) const int *p; { return p == &gon; }

int test(void)
{
	int bad = 0;
	const int local = 5;
	const int zero = 0;
	int x;

	hits = 0;
	if (V7) x = 1; else x = marker();
	if (x != 1 || hits) bad |= 1;
	if (BSD) x = marker(); else x = 2;
	if (x != 2 || hits) bad |= 2;
	if (sdebug) marker();
	if (hits) bad |= 4;
	if (gon) x = 3; else marker();
	if (x != 3 || hits) bad |= 8;
	if (zero) marker(); else x = local + 1;
	if (x != 6 || hits) bad |= 16;
	if (ON) { x = 7; } else { x = marker(); }
	if (OFF) { x = marker(); } else if (V7) { x = 8; } else { x = marker(); }
	if (x != 8 || hits) bad |= 32;
	if (csmall + local != 12) bad |= 64;
	if (!addr_ok(&gon)) bad |= 128;
	/* a label in a dead arm can still be reached */
	x = 0;
	goto in_dead;
	if (OFF) {
in_dead:
		x = 9;
	}
	if (x != 9) bad |= 256;
	/* not constants: both arms exist */
	x = 1;
	if (x) hits = 10; else hits = 20;
	if (hits != 10) bad |= 512;
	return bad == 0;
}
