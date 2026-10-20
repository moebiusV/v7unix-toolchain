/* expect: 1 -- GNU "case lo ... hi:": short ranges, ranges too long to expand, signed and unsigned bounds, mixed with single cases */
int sm(int x)
{
	switch (x) {
	case 1 ... 3: return 10;
	case 5: return 11;
	case 7 ... 9: return 12;
	default: return 0;
	}
}

long neg(long x)
{
	switch (x) {
	case -100000L ... -5: return 1;
	case -4 ... 4: return 2;
	case 5 ... 100000L: return 3;
	default: return 0;
	}
}

/* both ends of an unsigned range at the top of the type */
unsigned long top(unsigned long x)
{
	switch (x) {
	case 4294967290ul ... 4294967295ul: return 1;
	case 0 ... 2: return 2;
	case 1000ul ... 4000000000ul: return 3;
	default: return 0;
	}
}

int test(void)
{
	if (sm(0) != 0 || sm(1) != 10 || sm(3) != 10 || sm(4) != 0 || sm(5) != 11 || sm(6) != 0 || sm(7) != 12 || sm(9) != 12 || sm(10) != 0)
		return 0;
	if (neg(-100001L) != 0 || neg(-100000L) != 1 || neg(-5) != 1 || neg(-4) != 2 || neg(0) != 2 || neg(4) != 2 || neg(5) != 3 || neg(100000L) != 3 || neg(100001L) != 0)
		return 0;
	if (top(0) != 2 || top(2) != 2 || top(3) != 0 || top(999) != 0 || top(1000) != 3 || top(4000000000ul) != 3 || top(4000000001ul) != 0 || top(4294967289ul) != 0 || top(4294967290ul) != 1 || top(4294967295ul) != 1)
		return 0;
	return 1;
}
