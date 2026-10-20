/* expect: 1 -- old-style forms: type-less declarations and initializers (upstream's grammar), initializers with no '=' (our scanner rule), an old-style struct parameter, a function returning a pointer to an array */
struct tc { int op; int val; };
x;
y, z;
w = 5;
int a[] { 1, 2, 3 };
int s 7;
char msg[] "ok";
struct tc czero { 1, 2 };
char *names[] { "a", "b", 0 };
int neg -3;
int tab[3] = { 10, 20, 30 };

int (*pick(k))[3]
int k;
{
	return k ? &tab : 0;
}

f(p)
struct tc *p;
{
	return p->op + p->val;
}

test()
{
	int (*t)[3];

	x = 1; y = 2; z = 3;
	if (x + y + z != 6 || w != 5 || s != 7 || neg != -3)
		return 0;
	if (a[0] + a[1] + a[2] != 6 || msg[0] != 'o' || msg[1] != 'k' || msg[2] != 0)
		return 0;
	if (czero.val != 2 || f(&czero) != 3 || names[1][0] != 'b' || names[2] != 0)
		return 0;
	t = pick(1);
	return t != 0 && (*t)[2] == 30 && pick(0) == 0;
}
