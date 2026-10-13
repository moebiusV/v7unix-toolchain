/* expect: 1 -- bit test of a pointer word cast to int: bit $1,(rN) */
union store { union store *ptr; int dummy; };
union store a[2];

int busy(register union store *p)
{
	if (((int)(p->ptr)) & 1)
		return 1;
	return 0;
}

int test(void)
{
	a[0].ptr = (union store *)5;
	a[1].ptr = (union store *)4;
	return busy(&a[0]) == 1 && busy(&a[1]) == 0;
}
