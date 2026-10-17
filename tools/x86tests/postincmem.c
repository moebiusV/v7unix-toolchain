/* expect: 1 -- *p++ with p in memory: the store or load goes through the pointer, the increment follows */
char *p, *q, buf[8], src[8];
int *ip, ibuf[4];
struct S { char *ptr; int cnt; } s, *sp;

int test(void)
{
	int bad = 0, i, c;
	char *save;

	for (i = 0; i < 8; i++) src[i] = 'a' + i;
	p = buf;
	*p++ = 'x';
	*p++ = 'y';
	if (p != buf + 2 || buf[0] != 'x' || buf[1] != 'y') bad |= 1;
	q = src;
	p = buf;
	*p++ = *q++;
	*p++ = *q++;
	if (p != buf + 2 || q != src + 2 || buf[0] != 'a' || buf[1] != 'b') bad |= 2;
	ip = ibuf;
	*ip++ = 7;
	*ip++ = 9;
	if (ip != ibuf + 2 || ibuf[0] != 7 || ibuf[1] != 9) bad |= 4;
	q = src;
	c = *q++;
	if (c != 'a' || q != src + 1) bad |= 8;
	c = *q++ + *q++;
	if (c != 'b' + 'c' || q != src + 3) bad |= 16;
	s.ptr = buf;
	*s.ptr++ = 'k';
	if (s.ptr != buf + 1 || buf[0] != 'k') bad |= 32;
	sp = &s;
	sp->ptr = buf;
	*sp->ptr++ = 'm';
	*sp->ptr++ = 'n';
	if (sp->ptr != buf + 2 || buf[0] != 'm' || buf[1] != 'n') bad |= 64;
	/* the pointer variable itself is the target */
	p = buf;
	save = p;
	*p++ = 'z';
	if (save != buf || buf[0] != 'z' || p != buf + 1) bad |= 128;
	return bad == 0;
}
