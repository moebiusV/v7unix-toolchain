/* expect: 1 -- name(rN) indexed addressing: element sizes 1, 2, 4, 8, 2-D, structs, statics */
char  cv[20];
int   iv[10];
long  lv[6];
double dv[4];
char  *pv[4];
char  grid[4][6];
int   igrid[3][5];
struct r { char tag; int val; long big; } rv[4];
static int sv[8];
unsigned char uv[8];

int run(register int i, register int n)
{
	int bad, j;

	bad = 0;
	for (j = 0; j < 20; j++) cv[j] = j;
	for (j = 0; j < 10; j++) iv[j] = j * 100;
	for (j = 0; j < 6; j++) lv[j] = j * 70000L;
	for (j = 0; j < 4; j++) dv[j] = j + 0.5;
	for (j = 0; j < 4; j++) pv[j] = cv + j * 3;
	for (j = 0; j < 4; j++) rv[j].tag = 'a' + j, rv[j].val = -j, rv[j].big = j * 100000L;
	for (j = 0; j < 8; j++) sv[j] = j * 7, uv[j] = 250 + j;

	if (cv[i] != 2 || cv[i + 1] != 3 || cv[n - 1] != 6) bad |= 1;
	if (iv[i] != 200 || iv[n] != 700 || iv[i + 2] != 400) bad |= 2;
	if (lv[i] != 140000L || lv[n - 3] != 4 * 70000L) bad |= 4;
	if ((int)(dv[i] * 2) != 5 || (int)(dv[n - 6] * 2) != 3) bad |= 8;
	if (pv[i] != cv + 6 || *pv[i] != 6) bad |= 16;
	if (grid[i][1] != 0) bad |= 32;				/* zero-initialised */
	grid[i][3] = 9; grid[1][i] = 8;
	if (grid[2][3] != 9 || grid[1][2] != 8) bad |= 64;
	igrid[i][n - 3] = 77;
	if (igrid[2][4] != 77) bad |= 128;
	if (rv[i].tag != 'c' || rv[i].val != -2 || rv[i].big != 200000L) bad |= 256;
	rv[i].val += 5; rv[n - 4].big++;
	if (rv[2].val != 3 || rv[3].big != 300001L) bad |= 512;
	if (sv[i] != 14 || sv[n] != 49) bad |= 1024;
	if (uv[i] != 252 || uv[n] != 1) bad |= 2048;
	cv[i] = cv[n] = 99;
	if (cv[2] != 99 || cv[7] != 99) bad |= 4096;
	return bad;
}

int test(void)
{
	return run(2, 7) == 0;
}
