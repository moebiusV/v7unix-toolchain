/* long accumulate and compare */
long acc, lim;
int test(void)
{
	long i;
	int n;

	acc = 0; lim = 300; n = 0;
	for (i = 0; i < lim; i++) {
		acc += i;
		acc -= 2;
		if (acc < 0) n++;
		if (acc == 0) n--;
	}
	return (int)(acc & 0x7fff) + n;
}
