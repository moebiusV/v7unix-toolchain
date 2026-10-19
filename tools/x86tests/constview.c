/* expect: 1 -- reading a const integer object: whole, and the first byte through a char pointer (the byte order of the machine) */
static const unsigned short det = { 0x100 };
static const unsigned short det2 = 0x100;
static const unsigned short one = { 1 };
static const char c = { -3 };
static unsigned short var = 0x100;

int g1() { return det; }
int g2() { return det2; }
int g3() { return *(const unsigned char *)&det; }
int g4() { return *(const unsigned char *)&one; }
int g5() { return *(const unsigned char *)&det2 == 0; }
int g6() { return *(const char *)&c; }
int g7() { var = 1; return *(unsigned char *)&var; }
int g8() { return *(const unsigned char *)&det == 1; }

int test()
{
	if (g1() != 0x100 || g2() != 0x100) return 2;
	if (g3() != 0) return 3;
	if (g4() != 1) return 4;
	if (g5() != 1) return 5;
	if (g6() != -3) return 6;
	if (g7() != 1) return 7;
	if (g8() != 0) return 8;
	return 1;
}
