/* expect: 1 -- members of a function's own static struct, by name, index and address */
struct direct { unsigned short d_ino; char d_name[14]; };

char *where(p) struct direct *p; { return p->d_name; }

int test(void)
{
	static struct direct d;
	char *q;
	int i, bad = 0;

	d.d_ino = 7;
	for (i = 0; i < 14; i++)
		d.d_name[i] = 'a' + i;
	q = d.d_name;
	if (q[0] != 'a' || q[13] != 'n') bad |= 1;
	if (d.d_name[3] != 'd') bad |= 2;
	if (where(&d) != q || where(&d) != (char *)&d + 2) bad |= 4;
	if (*(q + 5) != 'f') bad |= 8;
	if (d.d_ino != 7) bad |= 16;
	return bad == 0;
}
