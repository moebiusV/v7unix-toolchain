/* expect: 1 -- (a != b) == 0 and (a == b) != 1 and the other ways of asking a comparison about itself */
int ck1(a, b) int a, b; { if ((a != b) == 0) return 1; return 2; }
int ck2(a, b) int a, b; { if ((a == b) == 0) return 1; return 2; }
int ck3(a, b) int a, b; { if ((a == b) != 1) return 1; return 2; }
int ck4(a, b) int a, b; { if ((a != b) != 1) return 1; return 2; }
int ck5(a, b) int a, b; { if ((a != b) == 1) return 1; return 2; }
int ck6(a, b) int a, b; { if ((a == b) == 1) return 1; return 2; }
int ck7(a, b) int a, b; { if ((a != b) != 0) return 1; return 2; }
int ck8(a, b) int a, b; { if ((a == b) != 0) return 1; return 2; }
int vl1(a, b) int a, b; { return (a != b) == 0; }
int vl2(a, b) int a, b; { return (a == b) != 1; }
int vl3(a, b) int a, b; { return ((a == b) == 0) == 0; }
int vl4(a, b) int a, b; { return ((a != b) != 1) != 1; }

int test()
{
	if (ck1(1, 1) != 1 || ck1(1, 2) != 2) return 2;
	if (ck2(1, 1) != 2 || ck2(1, 2) != 1) return 3;
	if (ck3(1, 1) != 2 || ck3(1, 2) != 1) return 4;
	if (ck4(1, 1) != 1 || ck4(1, 2) != 2) return 5;
	if (ck5(1, 1) != 2 || ck5(1, 2) != 1) return 6;
	if (ck6(1, 1) != 1 || ck6(1, 2) != 2) return 7;
	if (ck7(1, 1) != 2 || ck7(1, 2) != 1) return 8;
	if (ck8(1, 1) != 1 || ck8(1, 2) != 2) return 9;
	if (vl1(1, 1) != 1 || vl1(1, 2) != 0) return 10;
	if (vl2(1, 1) != 0 || vl2(1, 2) != 1) return 11;
	if (vl3(1, 1) != 1 || vl3(1, 2) != 0) return 12;
	if (vl4(1, 1) != 0 || vl4(1, 2) != 1) return 13;
	return 1;
}
