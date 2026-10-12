/* expect: 1 -- ++*q++, --*q++, ++*--q folded into incb/decb/inc with (rN)+ and -(rN) */
char cv[8];
int iv[6];

int run(register char *q, register int *ip)
{
	int bad;

	bad = 0;
	cv[0] = 10; cv[1] = 20; cv[2] = 30; cv[3] = 40;
	q = cv;
	++*q++;				/* cv[0]=11, q -> cv[1] */
	--*q++;				/* cv[1]=19, q -> cv[2] */
	if (cv[0] != 11 || cv[1] != 19 || q != cv + 2) bad |= 1;
	++*--q;				/* back to cv[1]: 20 */
	if (cv[1] != 20 || q != cv + 1) bad |= 2;
	--*--q;				/* cv[0]: 10 */
	if (cv[0] != 10 || q != cv) bad |= 4;
	++*q++; ++*q++; ++*q++;
	if (cv[0] != 11 || cv[1] != 21 || cv[2] != 31 || q != cv + 3) bad |= 8;
	if (cv[3] != 40) bad |= 16;				/* untouched */

	iv[0] = 100; iv[1] = 200; iv[2] = 300;
	ip = iv;
	++*ip++; ++*ip++;
	if (iv[0] != 101 || iv[1] != 201 || iv[2] != 300 || ip != iv + 2) bad |= 32;
	--*--ip;
	if (iv[1] != 200 || ip != iv + 1) bad |= 64;
	return bad;
}

int test(void)
{
	return run(cv, iv) == 0;
}
