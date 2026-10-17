/* expect: 1 -- words and bytes through a pointer in a global: loads, stores, conversions, with ++ */
int *ip, ibuf[4];
char *cp, cbuf[8];
unsigned char *ucp;
int gi;
char gc;

int rdw() { return *ip; }
int rdc() { return *cp; }
int rdu() { return *ucp; }

int test(void)
{
	int bad = 0, i;

	ibuf[0] = 11; ibuf[1] = -22; ibuf[2] = 33;
	cbuf[0] = -3; cbuf[1] = 7; cbuf[2] = (char)200;
	ip = ibuf; cp = cbuf; ucp = (unsigned char *)cbuf;
	if (rdw() != 11) bad |= 1;
	if (rdc() != -3) bad |= 2;
	if (rdu() != 253) bad |= 4;
	gi = *ip;
	if (gi != 11) bad |= 8;
	*ip = 99;
	if (ibuf[0] != 99) bad |= 16;
	*ip = gi;
	if (ibuf[0] != 11) bad |= 32;
	gc = *cp;
	if (gc != -3) bad |= 64;
	*cp = 5;
	if (cbuf[0] != 5) bad |= 128;
	i = *ip++;
	if (i != 11 || ip != ibuf + 1) bad |= 256;
	i = *ip++;
	if (i != -22 || ip != ibuf + 2) bad |= 512;
	i = *cp++ & 0377;
	if (i != 5 || cp != cbuf + 1) bad |= 1024;
	i = *cp++;
	if (i != 7 || cp != cbuf + 2) bad |= 2048;
	i = *cp;
	if (i != -56) bad |= 4096;
	return bad == 0;
}
