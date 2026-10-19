/* expect: 1 -- a function returning a pointer to an array, T (*f(args))[N] { ... }, is a function definition and not a K&R initializer without '=' */
int tab[3] = { 1, 2, 3 };
int arr[] { 4, 5 };

int (*get(k))[3]
int k;
{
	return k ? &tab : 0;
}

int (*get2(int k))[3]
{
	return k ? &tab : 0;
}

test()
{
	int (*p)[3];

	p = get(1);
	if (!p || (*p)[1] != 2 || get(0) != 0)
		return 0;
	p = get2(1);
	if (!p || (*p)[2] != 3 || get2(0) != 0)
		return 0;
	return arr[0] == 4 && arr[1] == 5;
}
