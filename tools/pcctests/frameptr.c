/* expect: 1 -- stores, loads and increments through a pointer held in a local (a deferred operand off the frame) */
struct f { int cnt; char *p; int *ip; };
int sink(b) struct f *b; { return b->cnt; }

int test()
{
	struct f b;
	char buf[8];
	int arr[4];
	int bad, r;

	bad = 0;
	b.cnt = 3; b.p = buf; b.ip = arr;
	sink(&b);
	*b.p++ = 'a';
	*b.p++ = 'b';
	*b.p = 0;
	if (buf[0] != 'a' || buf[1] != 'b' || buf[2] != 0 || b.p != buf + 2) bad |= 1;
	*b.ip++ = 10;
	*b.ip++ = 20;
	*b.ip = 30;
	if (arr[0] != 10 || arr[1] != 20 || arr[2] != 30 || b.ip != arr + 2) bad |= 2;
	r = *b.p++;
	if (r != 0 || b.p != buf + 3) bad |= 4;
	b.p = buf;
	r = *b.p++ + *b.p++;
	if (r != 'a' + 'b' || b.p != buf + 2) bad |= 8;
	(*b.ip)++;
	*b.ip += 5;
	if (arr[2] != 36) bad |= 16;
	b.ip = arr;
	b.cnt = *b.ip++ + *b.ip;
	if (b.cnt != 30 || b.ip != arr + 1) bad |= 32;
	return bad ? bad : 1;
}
