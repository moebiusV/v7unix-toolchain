/* expect: 1 -- long double comparisons and conditions, NaNs included; expected values from the host's __float128 (mkarith.py) */
typedef unsigned long u64;
typedef unsigned int u32;
static long double mk(u64 hi, u64 lo) { u64 w[2]; w[0] = lo; w[1] = hi; return *(long double *)w; }
/* x has these bits (any NaN for an expected NaN) */
static int is(long double x, u64 hi, u64 lo)
{
	u64 *w = (u64 *)&x;
	u64 h = w[1] & 0x7fffffffffffffffUL;

	if (h > 0x7fff000000000000UL || (h == 0x7fff000000000000UL && w[0] != 0))
		return (hi & 0x7fffffffffffffffUL) > 0x7fff000000000000UL || ((hi & 0x7fffffffffffffffUL) == 0x7fff000000000000UL && lo != 0);
	return w[1] == hi && w[0] == lo;
}

int test(void)
{
	long double x, y, z, n, w, one;
	int i;

	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x < y) != 0) return 2;
	if (x < y) { if (!0) return 3; } else { if (0) return 4; }
	if (!(x < y)) { if (0) return 5; } else { if (!0) return 6; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x <= y) != 1) return 7;
	if (x <= y) { if (!1) return 8; } else { if (1) return 9; }
	if (!(x <= y)) { if (1) return 10; } else { if (!1) return 11; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x > y) != 0) return 12;
	if (x > y) { if (!0) return 13; } else { if (0) return 14; }
	if (!(x > y)) { if (0) return 15; } else { if (!0) return 16; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x >= y) != 1) return 17;
	if (x >= y) { if (!1) return 18; } else { if (1) return 19; }
	if (!(x >= y)) { if (1) return 20; } else { if (!1) return 21; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x == y) != 1) return 22;
	if (x == y) { if (!1) return 23; } else { if (1) return 24; }
	if (!(x == y)) { if (1) return 25; } else { if (!1) return 26; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x != y) != 0) return 27;
	if (x != y) { if (!0) return 28; } else { if (0) return 29; }
	if (!(x != y)) { if (0) return 30; } else { if (!0) return 31; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x < y) != 0) return 32;
	if (x < y) { if (!0) return 33; } else { if (0) return 34; }
	if (!(x < y)) { if (0) return 35; } else { if (!0) return 36; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x <= y) != 0) return 37;
	if (x <= y) { if (!0) return 38; } else { if (0) return 39; }
	if (!(x <= y)) { if (0) return 40; } else { if (!0) return 41; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x > y) != 1) return 42;
	if (x > y) { if (!1) return 43; } else { if (1) return 44; }
	if (!(x > y)) { if (1) return 45; } else { if (!1) return 46; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x >= y) != 1) return 47;
	if (x >= y) { if (!1) return 48; } else { if (1) return 49; }
	if (!(x >= y)) { if (1) return 50; } else { if (!1) return 51; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x == y) != 0) return 52;
	if (x == y) { if (!0) return 53; } else { if (0) return 54; }
	if (!(x == y)) { if (0) return 55; } else { if (!0) return 56; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x != y) != 1) return 57;
	if (x != y) { if (!1) return 58; } else { if (1) return 59; }
	if (!(x != y)) { if (1) return 60; } else { if (!1) return 61; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x < y) != 0) return 62;
	if (x < y) { if (!0) return 63; } else { if (0) return 64; }
	if (!(x < y)) { if (0) return 65; } else { if (!0) return 66; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x <= y) != 0) return 67;
	if (x <= y) { if (!0) return 68; } else { if (0) return 69; }
	if (!(x <= y)) { if (0) return 70; } else { if (!0) return 71; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x > y) != 1) return 72;
	if (x > y) { if (!1) return 73; } else { if (1) return 74; }
	if (!(x > y)) { if (1) return 75; } else { if (!1) return 76; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x >= y) != 1) return 77;
	if (x >= y) { if (!1) return 78; } else { if (1) return 79; }
	if (!(x >= y)) { if (1) return 80; } else { if (!1) return 81; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x == y) != 0) return 82;
	if (x == y) { if (!0) return 83; } else { if (0) return 84; }
	if (!(x == y)) { if (0) return 85; } else { if (!0) return 86; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x != y) != 1) return 87;
	if (x != y) { if (!1) return 88; } else { if (1) return 89; }
	if (!(x != y)) { if (1) return 90; } else { if (!1) return 91; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x < y) != 1) return 92;
	if (x < y) { if (!1) return 93; } else { if (1) return 94; }
	if (!(x < y)) { if (1) return 95; } else { if (!1) return 96; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x <= y) != 1) return 97;
	if (x <= y) { if (!1) return 98; } else { if (1) return 99; }
	if (!(x <= y)) { if (1) return 100; } else { if (!1) return 101; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x > y) != 0) return 102;
	if (x > y) { if (!0) return 103; } else { if (0) return 104; }
	if (!(x > y)) { if (0) return 105; } else { if (!0) return 106; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x >= y) != 0) return 107;
	if (x >= y) { if (!0) return 108; } else { if (0) return 109; }
	if (!(x >= y)) { if (0) return 110; } else { if (!0) return 111; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x == y) != 0) return 112;
	if (x == y) { if (!0) return 113; } else { if (0) return 114; }
	if (!(x == y)) { if (0) return 115; } else { if (!0) return 116; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x != y) != 1) return 117;
	if (x != y) { if (!1) return 118; } else { if (1) return 119; }
	if (!(x != y)) { if (1) return 120; } else { if (!1) return 121; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x < y) != 0) return 122;
	if (x < y) { if (!0) return 123; } else { if (0) return 124; }
	if (!(x < y)) { if (0) return 125; } else { if (!0) return 126; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x <= y) != 0) return 127;
	if (x <= y) { if (!0) return 128; } else { if (0) return 129; }
	if (!(x <= y)) { if (0) return 130; } else { if (!0) return 131; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x > y) != 1) return 132;
	if (x > y) { if (!1) return 133; } else { if (1) return 134; }
	if (!(x > y)) { if (1) return 135; } else { if (!1) return 136; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x >= y) != 1) return 137;
	if (x >= y) { if (!1) return 138; } else { if (1) return 139; }
	if (!(x >= y)) { if (1) return 140; } else { if (!1) return 141; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x == y) != 0) return 142;
	if (x == y) { if (!0) return 143; } else { if (0) return 144; }
	if (!(x == y)) { if (0) return 145; } else { if (!0) return 146; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x != y) != 1) return 147;
	if (x != y) { if (!1) return 148; } else { if (1) return 149; }
	if (!(x != y)) { if (1) return 150; } else { if (!1) return 151; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x < y) != 1) return 152;
	if (x < y) { if (!1) return 153; } else { if (1) return 154; }
	if (!(x < y)) { if (1) return 155; } else { if (!1) return 156; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x <= y) != 1) return 157;
	if (x <= y) { if (!1) return 158; } else { if (1) return 159; }
	if (!(x <= y)) { if (1) return 160; } else { if (!1) return 161; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x > y) != 0) return 162;
	if (x > y) { if (!0) return 163; } else { if (0) return 164; }
	if (!(x > y)) { if (0) return 165; } else { if (!0) return 166; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x >= y) != 0) return 167;
	if (x >= y) { if (!0) return 168; } else { if (0) return 169; }
	if (!(x >= y)) { if (0) return 170; } else { if (!0) return 171; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x == y) != 0) return 172;
	if (x == y) { if (!0) return 173; } else { if (0) return 174; }
	if (!(x == y)) { if (0) return 175; } else { if (!0) return 176; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x != y) != 1) return 177;
	if (x != y) { if (!1) return 178; } else { if (1) return 179; }
	if (!(x != y)) { if (1) return 180; } else { if (!1) return 181; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x < y) != 1) return 182;
	if (x < y) { if (!1) return 183; } else { if (1) return 184; }
	if (!(x < y)) { if (1) return 185; } else { if (!1) return 186; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x <= y) != 1) return 187;
	if (x <= y) { if (!1) return 188; } else { if (1) return 189; }
	if (!(x <= y)) { if (1) return 190; } else { if (!1) return 191; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x > y) != 0) return 192;
	if (x > y) { if (!0) return 193; } else { if (0) return 194; }
	if (!(x > y)) { if (0) return 195; } else { if (!0) return 196; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x >= y) != 0) return 197;
	if (x >= y) { if (!0) return 198; } else { if (0) return 199; }
	if (!(x >= y)) { if (0) return 200; } else { if (!0) return 201; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x == y) != 0) return 202;
	if (x == y) { if (!0) return 203; } else { if (0) return 204; }
	if (!(x == y)) { if (0) return 205; } else { if (!0) return 206; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x != y) != 1) return 207;
	if (x != y) { if (!1) return 208; } else { if (1) return 209; }
	if (!(x != y)) { if (1) return 210; } else { if (!1) return 211; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x < y) != 0) return 212;
	if (x < y) { if (!0) return 213; } else { if (0) return 214; }
	if (!(x < y)) { if (0) return 215; } else { if (!0) return 216; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x <= y) != 1) return 217;
	if (x <= y) { if (!1) return 218; } else { if (1) return 219; }
	if (!(x <= y)) { if (1) return 220; } else { if (!1) return 221; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x > y) != 0) return 222;
	if (x > y) { if (!0) return 223; } else { if (0) return 224; }
	if (!(x > y)) { if (0) return 225; } else { if (!0) return 226; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x >= y) != 1) return 227;
	if (x >= y) { if (!1) return 228; } else { if (1) return 229; }
	if (!(x >= y)) { if (1) return 230; } else { if (!1) return 231; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x == y) != 1) return 232;
	if (x == y) { if (!1) return 233; } else { if (1) return 234; }
	if (!(x == y)) { if (1) return 235; } else { if (!1) return 236; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x != y) != 0) return 237;
	if (x != y) { if (!0) return 238; } else { if (0) return 239; }
	if (!(x != y)) { if (0) return 240; } else { if (!0) return 241; }
	x = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x < y) != 0) return 242;
	if (x < y) { if (!0) return 243; } else { if (0) return 244; }
	if (!(x < y)) { if (0) return 245; } else { if (!0) return 246; }
	x = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x <= y) != 0) return 247;
	if (x <= y) { if (!0) return 248; } else { if (0) return 249; }
	if (!(x <= y)) { if (0) return 250; } else { if (!0) return 251; }
	x = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x > y) != 1) return 252;
	if (x > y) { if (!1) return 253; } else { if (1) return 254; }
	if (!(x > y)) { if (1) return 255; } else { if (!1) return 256; }
	x = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x >= y) != 1) return 257;
	if (x >= y) { if (!1) return 258; } else { if (1) return 259; }
	if (!(x >= y)) { if (1) return 260; } else { if (!1) return 261; }
	x = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x == y) != 0) return 262;
	if (x == y) { if (!0) return 263; } else { if (0) return 264; }
	if (!(x == y)) { if (0) return 265; } else { if (!0) return 266; }
	x = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x != y) != 1) return 267;
	if (x != y) { if (!1) return 268; } else { if (1) return 269; }
	if (!(x != y)) { if (1) return 270; } else { if (!1) return 271; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x < y) != 0) return 272;
	if (x < y) { if (!0) return 273; } else { if (0) return 274; }
	if (!(x < y)) { if (0) return 275; } else { if (!0) return 276; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x <= y) != 0) return 277;
	if (x <= y) { if (!0) return 278; } else { if (0) return 279; }
	if (!(x <= y)) { if (0) return 280; } else { if (!0) return 281; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x > y) != 1) return 282;
	if (x > y) { if (!1) return 283; } else { if (1) return 284; }
	if (!(x > y)) { if (1) return 285; } else { if (!1) return 286; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x >= y) != 1) return 287;
	if (x >= y) { if (!1) return 288; } else { if (1) return 289; }
	if (!(x >= y)) { if (1) return 290; } else { if (!1) return 291; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x == y) != 0) return 292;
	if (x == y) { if (!0) return 293; } else { if (0) return 294; }
	if (!(x == y)) { if (0) return 295; } else { if (!0) return 296; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x != y) != 1) return 297;
	if (x != y) { if (!1) return 298; } else { if (1) return 299; }
	if (!(x != y)) { if (1) return 300; } else { if (!1) return 301; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x < y) != 0) return 302;
	if (x < y) { if (!0) return 303; } else { if (0) return 304; }
	if (!(x < y)) { if (0) return 305; } else { if (!0) return 306; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x <= y) != 1) return 307;
	if (x <= y) { if (!1) return 308; } else { if (1) return 309; }
	if (!(x <= y)) { if (1) return 310; } else { if (!1) return 311; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x > y) != 0) return 312;
	if (x > y) { if (!0) return 313; } else { if (0) return 314; }
	if (!(x > y)) { if (0) return 315; } else { if (!0) return 316; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x >= y) != 1) return 317;
	if (x >= y) { if (!1) return 318; } else { if (1) return 319; }
	if (!(x >= y)) { if (1) return 320; } else { if (!1) return 321; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x == y) != 1) return 322;
	if (x == y) { if (!1) return 323; } else { if (1) return 324; }
	if (!(x == y)) { if (1) return 325; } else { if (!1) return 326; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x != y) != 0) return 327;
	if (x != y) { if (!0) return 328; } else { if (0) return 329; }
	if (!(x != y)) { if (0) return 330; } else { if (!0) return 331; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x < y) != 0) return 332;
	if (x < y) { if (!0) return 333; } else { if (0) return 334; }
	if (!(x < y)) { if (0) return 335; } else { if (!0) return 336; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x <= y) != 1) return 337;
	if (x <= y) { if (!1) return 338; } else { if (1) return 339; }
	if (!(x <= y)) { if (1) return 340; } else { if (!1) return 341; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x > y) != 0) return 342;
	if (x > y) { if (!0) return 343; } else { if (0) return 344; }
	if (!(x > y)) { if (0) return 345; } else { if (!0) return 346; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x >= y) != 1) return 347;
	if (x >= y) { if (!1) return 348; } else { if (1) return 349; }
	if (!(x >= y)) { if (1) return 350; } else { if (!1) return 351; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x == y) != 1) return 352;
	if (x == y) { if (!1) return 353; } else { if (1) return 354; }
	if (!(x == y)) { if (1) return 355; } else { if (!1) return 356; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x != y) != 0) return 357;
	if (x != y) { if (!0) return 358; } else { if (0) return 359; }
	if (!(x != y)) { if (0) return 360; } else { if (!0) return 361; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x < y) != 1) return 362;
	if (x < y) { if (!1) return 363; } else { if (1) return 364; }
	if (!(x < y)) { if (1) return 365; } else { if (!1) return 366; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x <= y) != 1) return 367;
	if (x <= y) { if (!1) return 368; } else { if (1) return 369; }
	if (!(x <= y)) { if (1) return 370; } else { if (!1) return 371; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x > y) != 0) return 372;
	if (x > y) { if (!0) return 373; } else { if (0) return 374; }
	if (!(x > y)) { if (0) return 375; } else { if (!0) return 376; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x >= y) != 0) return 377;
	if (x >= y) { if (!0) return 378; } else { if (0) return 379; }
	if (!(x >= y)) { if (0) return 380; } else { if (!0) return 381; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x == y) != 0) return 382;
	if (x == y) { if (!0) return 383; } else { if (0) return 384; }
	if (!(x == y)) { if (0) return 385; } else { if (!0) return 386; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x != y) != 1) return 387;
	if (x != y) { if (!1) return 388; } else { if (1) return 389; }
	if (!(x != y)) { if (1) return 390; } else { if (!1) return 391; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x < y) != 1) return 392;
	if (x < y) { if (!1) return 393; } else { if (1) return 394; }
	if (!(x < y)) { if (1) return 395; } else { if (!1) return 396; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x <= y) != 1) return 397;
	if (x <= y) { if (!1) return 398; } else { if (1) return 399; }
	if (!(x <= y)) { if (1) return 400; } else { if (!1) return 401; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x > y) != 0) return 402;
	if (x > y) { if (!0) return 403; } else { if (0) return 404; }
	if (!(x > y)) { if (0) return 405; } else { if (!0) return 406; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x >= y) != 0) return 407;
	if (x >= y) { if (!0) return 408; } else { if (0) return 409; }
	if (!(x >= y)) { if (0) return 410; } else { if (!0) return 411; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x == y) != 0) return 412;
	if (x == y) { if (!0) return 413; } else { if (0) return 414; }
	if (!(x == y)) { if (0) return 415; } else { if (!0) return 416; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x != y) != 1) return 417;
	if (x != y) { if (!1) return 418; } else { if (1) return 419; }
	if (!(x != y)) { if (1) return 420; } else { if (!1) return 421; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x < y) != 1) return 422;
	if (x < y) { if (!1) return 423; } else { if (1) return 424; }
	if (!(x < y)) { if (1) return 425; } else { if (!1) return 426; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x <= y) != 1) return 427;
	if (x <= y) { if (!1) return 428; } else { if (1) return 429; }
	if (!(x <= y)) { if (1) return 430; } else { if (!1) return 431; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x > y) != 0) return 432;
	if (x > y) { if (!0) return 433; } else { if (0) return 434; }
	if (!(x > y)) { if (0) return 435; } else { if (!0) return 436; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x >= y) != 0) return 437;
	if (x >= y) { if (!0) return 438; } else { if (0) return 439; }
	if (!(x >= y)) { if (0) return 440; } else { if (!0) return 441; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x == y) != 0) return 442;
	if (x == y) { if (!0) return 443; } else { if (0) return 444; }
	if (!(x == y)) { if (0) return 445; } else { if (!0) return 446; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x != y) != 1) return 447;
	if (x != y) { if (!1) return 448; } else { if (1) return 449; }
	if (!(x != y)) { if (1) return 450; } else { if (!1) return 451; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x < y) != 0) return 452;
	if (x < y) { if (!0) return 453; } else { if (0) return 454; }
	if (!(x < y)) { if (0) return 455; } else { if (!0) return 456; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x <= y) != 1) return 457;
	if (x <= y) { if (!1) return 458; } else { if (1) return 459; }
	if (!(x <= y)) { if (1) return 460; } else { if (!1) return 461; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x > y) != 0) return 462;
	if (x > y) { if (!0) return 463; } else { if (0) return 464; }
	if (!(x > y)) { if (0) return 465; } else { if (!0) return 466; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x >= y) != 1) return 467;
	if (x >= y) { if (!1) return 468; } else { if (1) return 469; }
	if (!(x >= y)) { if (1) return 470; } else { if (!1) return 471; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x == y) != 1) return 472;
	if (x == y) { if (!1) return 473; } else { if (1) return 474; }
	if (!(x == y)) { if (1) return 475; } else { if (!1) return 476; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x != y) != 0) return 477;
	if (x != y) { if (!0) return 478; } else { if (0) return 479; }
	if (!(x != y)) { if (0) return 480; } else { if (!0) return 481; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x < y) != 0) return 482;
	if (x < y) { if (!0) return 483; } else { if (0) return 484; }
	if (!(x < y)) { if (0) return 485; } else { if (!0) return 486; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x <= y) != 0) return 487;
	if (x <= y) { if (!0) return 488; } else { if (0) return 489; }
	if (!(x <= y)) { if (0) return 490; } else { if (!0) return 491; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x > y) != 1) return 492;
	if (x > y) { if (!1) return 493; } else { if (1) return 494; }
	if (!(x > y)) { if (1) return 495; } else { if (!1) return 496; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x >= y) != 1) return 497;
	if (x >= y) { if (!1) return 498; } else { if (1) return 499; }
	if (!(x >= y)) { if (1) return 500; } else { if (!1) return 501; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x == y) != 0) return 502;
	if (x == y) { if (!0) return 503; } else { if (0) return 504; }
	if (!(x == y)) { if (0) return 505; } else { if (!0) return 506; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x != y) != 1) return 507;
	if (x != y) { if (!1) return 508; } else { if (1) return 509; }
	if (!(x != y)) { if (1) return 510; } else { if (!1) return 511; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x < y) != 0) return 512;
	if (x < y) { if (!0) return 513; } else { if (0) return 514; }
	if (!(x < y)) { if (0) return 515; } else { if (!0) return 516; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x <= y) != 1) return 517;
	if (x <= y) { if (!1) return 518; } else { if (1) return 519; }
	if (!(x <= y)) { if (1) return 520; } else { if (!1) return 521; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x > y) != 0) return 522;
	if (x > y) { if (!0) return 523; } else { if (0) return 524; }
	if (!(x > y)) { if (0) return 525; } else { if (!0) return 526; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x >= y) != 1) return 527;
	if (x >= y) { if (!1) return 528; } else { if (1) return 529; }
	if (!(x >= y)) { if (1) return 530; } else { if (!1) return 531; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x == y) != 1) return 532;
	if (x == y) { if (!1) return 533; } else { if (1) return 534; }
	if (!(x == y)) { if (1) return 535; } else { if (!1) return 536; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x != y) != 0) return 537;
	if (x != y) { if (!0) return 538; } else { if (0) return 539; }
	if (!(x != y)) { if (0) return 540; } else { if (!0) return 541; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x < y) != 1) return 542;
	if (x < y) { if (!1) return 543; } else { if (1) return 544; }
	if (!(x < y)) { if (1) return 545; } else { if (!1) return 546; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x <= y) != 1) return 547;
	if (x <= y) { if (!1) return 548; } else { if (1) return 549; }
	if (!(x <= y)) { if (1) return 550; } else { if (!1) return 551; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x > y) != 0) return 552;
	if (x > y) { if (!0) return 553; } else { if (0) return 554; }
	if (!(x > y)) { if (0) return 555; } else { if (!0) return 556; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x >= y) != 0) return 557;
	if (x >= y) { if (!0) return 558; } else { if (0) return 559; }
	if (!(x >= y)) { if (0) return 560; } else { if (!0) return 561; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x == y) != 0) return 562;
	if (x == y) { if (!0) return 563; } else { if (0) return 564; }
	if (!(x == y)) { if (0) return 565; } else { if (!0) return 566; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x != y) != 1) return 567;
	if (x != y) { if (!1) return 568; } else { if (1) return 569; }
	if (!(x != y)) { if (1) return 570; } else { if (!1) return 571; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x < y) != 1) return 572;
	if (x < y) { if (!1) return 573; } else { if (1) return 574; }
	if (!(x < y)) { if (1) return 575; } else { if (!1) return 576; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x <= y) != 1) return 577;
	if (x <= y) { if (!1) return 578; } else { if (1) return 579; }
	if (!(x <= y)) { if (1) return 580; } else { if (!1) return 581; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x > y) != 0) return 582;
	if (x > y) { if (!0) return 583; } else { if (0) return 584; }
	if (!(x > y)) { if (0) return 585; } else { if (!0) return 586; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x >= y) != 0) return 587;
	if (x >= y) { if (!0) return 588; } else { if (0) return 589; }
	if (!(x >= y)) { if (0) return 590; } else { if (!0) return 591; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x == y) != 0) return 592;
	if (x == y) { if (!0) return 593; } else { if (0) return 594; }
	if (!(x == y)) { if (0) return 595; } else { if (!0) return 596; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x != y) != 1) return 597;
	if (x != y) { if (!1) return 598; } else { if (1) return 599; }
	if (!(x != y)) { if (1) return 600; } else { if (!1) return 601; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x < y) != 0) return 602;
	if (x < y) { if (!0) return 603; } else { if (0) return 604; }
	if (!(x < y)) { if (0) return 605; } else { if (!0) return 606; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x <= y) != 0) return 607;
	if (x <= y) { if (!0) return 608; } else { if (0) return 609; }
	if (!(x <= y)) { if (0) return 610; } else { if (!0) return 611; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x > y) != 1) return 612;
	if (x > y) { if (!1) return 613; } else { if (1) return 614; }
	if (!(x > y)) { if (1) return 615; } else { if (!1) return 616; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x >= y) != 1) return 617;
	if (x >= y) { if (!1) return 618; } else { if (1) return 619; }
	if (!(x >= y)) { if (1) return 620; } else { if (!1) return 621; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x == y) != 0) return 622;
	if (x == y) { if (!0) return 623; } else { if (0) return 624; }
	if (!(x == y)) { if (0) return 625; } else { if (!0) return 626; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x != y) != 1) return 627;
	if (x != y) { if (!1) return 628; } else { if (1) return 629; }
	if (!(x != y)) { if (1) return 630; } else { if (!1) return 631; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x < y) != 1) return 632;
	if (x < y) { if (!1) return 633; } else { if (1) return 634; }
	if (!(x < y)) { if (1) return 635; } else { if (!1) return 636; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x <= y) != 1) return 637;
	if (x <= y) { if (!1) return 638; } else { if (1) return 639; }
	if (!(x <= y)) { if (1) return 640; } else { if (!1) return 641; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x > y) != 0) return 642;
	if (x > y) { if (!0) return 643; } else { if (0) return 644; }
	if (!(x > y)) { if (0) return 645; } else { if (!0) return 646; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x >= y) != 0) return 647;
	if (x >= y) { if (!0) return 648; } else { if (0) return 649; }
	if (!(x >= y)) { if (0) return 650; } else { if (!0) return 651; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x == y) != 0) return 652;
	if (x == y) { if (!0) return 653; } else { if (0) return 654; }
	if (!(x == y)) { if (0) return 655; } else { if (!0) return 656; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x != y) != 1) return 657;
	if (x != y) { if (!1) return 658; } else { if (1) return 659; }
	if (!(x != y)) { if (1) return 660; } else { if (!1) return 661; }
	x = mk(0x7fff800000000000UL, 0x0000000000000000UL); y = mk(0x7fff800000000000UL, 0x0000000000000000UL);
	if ((x < y) != 0) return 662;
	if (x < y) { if (!0) return 663; } else { if (0) return 664; }
	if (!(x < y)) { if (0) return 665; } else { if (!0) return 666; }
	x = mk(0x7fff800000000000UL, 0x0000000000000000UL); y = mk(0x7fff800000000000UL, 0x0000000000000000UL);
	if ((x <= y) != 0) return 667;
	if (x <= y) { if (!0) return 668; } else { if (0) return 669; }
	if (!(x <= y)) { if (0) return 670; } else { if (!0) return 671; }
	x = mk(0x7fff800000000000UL, 0x0000000000000000UL); y = mk(0x7fff800000000000UL, 0x0000000000000000UL);
	if ((x > y) != 0) return 672;
	if (x > y) { if (!0) return 673; } else { if (0) return 674; }
	if (!(x > y)) { if (0) return 675; } else { if (!0) return 676; }
	x = mk(0x7fff800000000000UL, 0x0000000000000000UL); y = mk(0x7fff800000000000UL, 0x0000000000000000UL);
	if ((x >= y) != 0) return 677;
	if (x >= y) { if (!0) return 678; } else { if (0) return 679; }
	if (!(x >= y)) { if (0) return 680; } else { if (!0) return 681; }
	x = mk(0x7fff800000000000UL, 0x0000000000000000UL); y = mk(0x7fff800000000000UL, 0x0000000000000000UL);
	if ((x == y) != 0) return 682;
	if (x == y) { if (!0) return 683; } else { if (0) return 684; }
	if (!(x == y)) { if (0) return 685; } else { if (!0) return 686; }
	x = mk(0x7fff800000000000UL, 0x0000000000000000UL); y = mk(0x7fff800000000000UL, 0x0000000000000000UL);
	if ((x != y) != 1) return 687;
	if (x != y) { if (!1) return 688; } else { if (1) return 689; }
	if (!(x != y)) { if (1) return 690; } else { if (!1) return 691; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x < y) != 1) return 692;
	if (x < y) { if (!1) return 693; } else { if (1) return 694; }
	if (!(x < y)) { if (1) return 695; } else { if (!1) return 696; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x <= y) != 1) return 697;
	if (x <= y) { if (!1) return 698; } else { if (1) return 699; }
	if (!(x <= y)) { if (1) return 700; } else { if (!1) return 701; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x > y) != 0) return 702;
	if (x > y) { if (!0) return 703; } else { if (0) return 704; }
	if (!(x > y)) { if (0) return 705; } else { if (!0) return 706; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x >= y) != 0) return 707;
	if (x >= y) { if (!0) return 708; } else { if (0) return 709; }
	if (!(x >= y)) { if (0) return 710; } else { if (!0) return 711; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x == y) != 0) return 712;
	if (x == y) { if (!0) return 713; } else { if (0) return 714; }
	if (!(x == y)) { if (0) return 715; } else { if (!0) return 716; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x != y) != 1) return 717;
	if (x != y) { if (!1) return 718; } else { if (1) return 719; }
	if (!(x != y)) { if (1) return 720; } else { if (!1) return 721; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x < y) != 0) return 722;
	if (x < y) { if (!0) return 723; } else { if (0) return 724; }
	if (!(x < y)) { if (0) return 725; } else { if (!0) return 726; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x <= y) != 0) return 727;
	if (x <= y) { if (!0) return 728; } else { if (0) return 729; }
	if (!(x <= y)) { if (0) return 730; } else { if (!0) return 731; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x > y) != 1) return 732;
	if (x > y) { if (!1) return 733; } else { if (1) return 734; }
	if (!(x > y)) { if (1) return 735; } else { if (!1) return 736; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x >= y) != 1) return 737;
	if (x >= y) { if (!1) return 738; } else { if (1) return 739; }
	if (!(x >= y)) { if (1) return 740; } else { if (!1) return 741; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x == y) != 0) return 742;
	if (x == y) { if (!0) return 743; } else { if (0) return 744; }
	if (!(x == y)) { if (0) return 745; } else { if (!0) return 746; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x != y) != 1) return 747;
	if (x != y) { if (!1) return 748; } else { if (1) return 749; }
	if (!(x != y)) { if (1) return 750; } else { if (!1) return 751; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x < y) != 0) return 752;
	if (x < y) { if (!0) return 753; } else { if (0) return 754; }
	if (!(x < y)) { if (0) return 755; } else { if (!0) return 756; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x <= y) != 0) return 757;
	if (x <= y) { if (!0) return 758; } else { if (0) return 759; }
	if (!(x <= y)) { if (0) return 760; } else { if (!0) return 761; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x > y) != 1) return 762;
	if (x > y) { if (!1) return 763; } else { if (1) return 764; }
	if (!(x > y)) { if (1) return 765; } else { if (!1) return 766; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x >= y) != 1) return 767;
	if (x >= y) { if (!1) return 768; } else { if (1) return 769; }
	if (!(x >= y)) { if (1) return 770; } else { if (!1) return 771; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x == y) != 0) return 772;
	if (x == y) { if (!0) return 773; } else { if (0) return 774; }
	if (!(x == y)) { if (0) return 775; } else { if (!0) return 776; }
	x = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x != y) != 1) return 777;
	if (x != y) { if (!1) return 778; } else { if (1) return 779; }
	if (!(x != y)) { if (1) return 780; } else { if (!1) return 781; }
	x = mk(0xbfff000000000000UL, 0x0000000000000000UL); y = mk(0xbfff000000000000UL, 0x0000000000000000UL);
	if ((x < y) != 0) return 782;
	if (x < y) { if (!0) return 783; } else { if (0) return 784; }
	if (!(x < y)) { if (0) return 785; } else { if (!0) return 786; }
	x = mk(0xbfff000000000000UL, 0x0000000000000000UL); y = mk(0xbfff000000000000UL, 0x0000000000000000UL);
	if ((x <= y) != 1) return 787;
	if (x <= y) { if (!1) return 788; } else { if (1) return 789; }
	if (!(x <= y)) { if (1) return 790; } else { if (!1) return 791; }
	x = mk(0xbfff000000000000UL, 0x0000000000000000UL); y = mk(0xbfff000000000000UL, 0x0000000000000000UL);
	if ((x > y) != 0) return 792;
	if (x > y) { if (!0) return 793; } else { if (0) return 794; }
	if (!(x > y)) { if (0) return 795; } else { if (!0) return 796; }
	x = mk(0xbfff000000000000UL, 0x0000000000000000UL); y = mk(0xbfff000000000000UL, 0x0000000000000000UL);
	if ((x >= y) != 1) return 797;
	if (x >= y) { if (!1) return 798; } else { if (1) return 799; }
	if (!(x >= y)) { if (1) return 800; } else { if (!1) return 801; }
	x = mk(0xbfff000000000000UL, 0x0000000000000000UL); y = mk(0xbfff000000000000UL, 0x0000000000000000UL);
	if ((x == y) != 1) return 802;
	if (x == y) { if (!1) return 803; } else { if (1) return 804; }
	if (!(x == y)) { if (1) return 805; } else { if (!1) return 806; }
	x = mk(0xbfff000000000000UL, 0x0000000000000000UL); y = mk(0xbfff000000000000UL, 0x0000000000000000UL);
	if ((x != y) != 0) return 807;
	if (x != y) { if (!0) return 808; } else { if (0) return 809; }
	if (!(x != y)) { if (0) return 810; } else { if (!0) return 811; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x < y) != 0) return 812;
	if (x < y) { if (!0) return 813; } else { if (0) return 814; }
	if (!(x < y)) { if (0) return 815; } else { if (!0) return 816; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x <= y) != 0) return 817;
	if (x <= y) { if (!0) return 818; } else { if (0) return 819; }
	if (!(x <= y)) { if (0) return 820; } else { if (!0) return 821; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x > y) != 1) return 822;
	if (x > y) { if (!1) return 823; } else { if (1) return 824; }
	if (!(x > y)) { if (1) return 825; } else { if (!1) return 826; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x >= y) != 1) return 827;
	if (x >= y) { if (!1) return 828; } else { if (1) return 829; }
	if (!(x >= y)) { if (1) return 830; } else { if (!1) return 831; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x == y) != 0) return 832;
	if (x == y) { if (!0) return 833; } else { if (0) return 834; }
	if (!(x == y)) { if (0) return 835; } else { if (!0) return 836; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x != y) != 1) return 837;
	if (x != y) { if (!1) return 838; } else { if (1) return 839; }
	if (!(x != y)) { if (1) return 840; } else { if (!1) return 841; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x < y) != 1) return 842;
	if (x < y) { if (!1) return 843; } else { if (1) return 844; }
	if (!(x < y)) { if (1) return 845; } else { if (!1) return 846; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x <= y) != 1) return 847;
	if (x <= y) { if (!1) return 848; } else { if (1) return 849; }
	if (!(x <= y)) { if (1) return 850; } else { if (!1) return 851; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x > y) != 0) return 852;
	if (x > y) { if (!0) return 853; } else { if (0) return 854; }
	if (!(x > y)) { if (0) return 855; } else { if (!0) return 856; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x >= y) != 0) return 857;
	if (x >= y) { if (!0) return 858; } else { if (0) return 859; }
	if (!(x >= y)) { if (0) return 860; } else { if (!0) return 861; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x == y) != 0) return 862;
	if (x == y) { if (!0) return 863; } else { if (0) return 864; }
	if (!(x == y)) { if (0) return 865; } else { if (!0) return 866; }
	x = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL); y = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL);
	if ((x != y) != 1) return 867;
	if (x != y) { if (!1) return 868; } else { if (1) return 869; }
	if (!(x != y)) { if (1) return 870; } else { if (!1) return 871; }
	x = mk(0x8000000000000000UL, 0x0000000000000000UL); y = mk(0x8000000000000000UL, 0x0000000000000000UL);
	if ((x < y) != 0) return 872;
	if (x < y) { if (!0) return 873; } else { if (0) return 874; }
	if (!(x < y)) { if (0) return 875; } else { if (!0) return 876; }
	x = mk(0x8000000000000000UL, 0x0000000000000000UL); y = mk(0x8000000000000000UL, 0x0000000000000000UL);
	if ((x <= y) != 1) return 877;
	if (x <= y) { if (!1) return 878; } else { if (1) return 879; }
	if (!(x <= y)) { if (1) return 880; } else { if (!1) return 881; }
	x = mk(0x8000000000000000UL, 0x0000000000000000UL); y = mk(0x8000000000000000UL, 0x0000000000000000UL);
	if ((x > y) != 0) return 882;
	if (x > y) { if (!0) return 883; } else { if (0) return 884; }
	if (!(x > y)) { if (0) return 885; } else { if (!0) return 886; }
	x = mk(0x8000000000000000UL, 0x0000000000000000UL); y = mk(0x8000000000000000UL, 0x0000000000000000UL);
	if ((x >= y) != 1) return 887;
	if (x >= y) { if (!1) return 888; } else { if (1) return 889; }
	if (!(x >= y)) { if (1) return 890; } else { if (!1) return 891; }
	x = mk(0x8000000000000000UL, 0x0000000000000000UL); y = mk(0x8000000000000000UL, 0x0000000000000000UL);
	if ((x == y) != 1) return 892;
	if (x == y) { if (!1) return 893; } else { if (1) return 894; }
	if (!(x == y)) { if (1) return 895; } else { if (!1) return 896; }
	x = mk(0x8000000000000000UL, 0x0000000000000000UL); y = mk(0x8000000000000000UL, 0x0000000000000000UL);
	if ((x != y) != 0) return 897;
	if (x != y) { if (!0) return 898; } else { if (0) return 899; }
	if (!(x != y)) { if (0) return 900; } else { if (!0) return 901; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x < y) != 0) return 902;
	if (x < y) { if (!0) return 903; } else { if (0) return 904; }
	if (!(x < y)) { if (0) return 905; } else { if (!0) return 906; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x <= y) != 0) return 907;
	if (x <= y) { if (!0) return 908; } else { if (0) return 909; }
	if (!(x <= y)) { if (0) return 910; } else { if (!0) return 911; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x > y) != 1) return 912;
	if (x > y) { if (!1) return 913; } else { if (1) return 914; }
	if (!(x > y)) { if (1) return 915; } else { if (!1) return 916; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x >= y) != 1) return 917;
	if (x >= y) { if (!1) return 918; } else { if (1) return 919; }
	if (!(x >= y)) { if (1) return 920; } else { if (!1) return 921; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x == y) != 0) return 922;
	if (x == y) { if (!0) return 923; } else { if (0) return 924; }
	if (!(x == y)) { if (0) return 925; } else { if (!0) return 926; }
	x = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x != y) != 1) return 927;
	if (x != y) { if (!1) return 928; } else { if (1) return 929; }
	if (!(x != y)) { if (1) return 930; } else { if (!1) return 931; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x < y) != 0) return 932;
	if (x < y) { if (!0) return 933; } else { if (0) return 934; }
	if (!(x < y)) { if (0) return 935; } else { if (!0) return 936; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x <= y) != 1) return 937;
	if (x <= y) { if (!1) return 938; } else { if (1) return 939; }
	if (!(x <= y)) { if (1) return 940; } else { if (!1) return 941; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x > y) != 0) return 942;
	if (x > y) { if (!0) return 943; } else { if (0) return 944; }
	if (!(x > y)) { if (0) return 945; } else { if (!0) return 946; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x >= y) != 1) return 947;
	if (x >= y) { if (!1) return 948; } else { if (1) return 949; }
	if (!(x >= y)) { if (1) return 950; } else { if (!1) return 951; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x == y) != 1) return 952;
	if (x == y) { if (!1) return 953; } else { if (1) return 954; }
	if (!(x == y)) { if (1) return 955; } else { if (!1) return 956; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x != y) != 0) return 957;
	if (x != y) { if (!0) return 958; } else { if (0) return 959; }
	if (!(x != y)) { if (0) return 960; } else { if (!0) return 961; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x < y) != 1) return 962;
	if (x < y) { if (!1) return 963; } else { if (1) return 964; }
	if (!(x < y)) { if (1) return 965; } else { if (!1) return 966; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x <= y) != 1) return 967;
	if (x <= y) { if (!1) return 968; } else { if (1) return 969; }
	if (!(x <= y)) { if (1) return 970; } else { if (!1) return 971; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x > y) != 0) return 972;
	if (x > y) { if (!0) return 973; } else { if (0) return 974; }
	if (!(x > y)) { if (0) return 975; } else { if (!0) return 976; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x >= y) != 0) return 977;
	if (x >= y) { if (!0) return 978; } else { if (0) return 979; }
	if (!(x >= y)) { if (0) return 980; } else { if (!0) return 981; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x == y) != 0) return 982;
	if (x == y) { if (!0) return 983; } else { if (0) return 984; }
	if (!(x == y)) { if (0) return 985; } else { if (!0) return 986; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0xa6e0ae1e938538fcUL, 0xaceb11b8983bab28UL);
	if ((x != y) != 1) return 987;
	if (x != y) { if (!1) return 988; } else { if (1) return 989; }
	if (!(x != y)) { if (1) return 990; } else { if (!1) return 991; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x < y) != 0) return 992;
	if (x < y) { if (!0) return 993; } else { if (0) return 994; }
	if (!(x < y)) { if (0) return 995; } else { if (!0) return 996; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x <= y) != 0) return 997;
	if (x <= y) { if (!0) return 998; } else { if (0) return 999; }
	if (!(x <= y)) { if (0) return 1000; } else { if (!0) return 1001; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x > y) != 1) return 1002;
	if (x > y) { if (!1) return 1003; } else { if (1) return 1004; }
	if (!(x > y)) { if (1) return 1005; } else { if (!1) return 1006; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x >= y) != 1) return 1007;
	if (x >= y) { if (!1) return 1008; } else { if (1) return 1009; }
	if (!(x >= y)) { if (1) return 1010; } else { if (!1) return 1011; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x == y) != 0) return 1012;
	if (x == y) { if (!0) return 1013; } else { if (0) return 1014; }
	if (!(x == y)) { if (0) return 1015; } else { if (!0) return 1016; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x != y) != 1) return 1017;
	if (x != y) { if (!1) return 1018; } else { if (1) return 1019; }
	if (!(x != y)) { if (1) return 1020; } else { if (!1) return 1021; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x < y) != 0) return 1022;
	if (x < y) { if (!0) return 1023; } else { if (0) return 1024; }
	if (!(x < y)) { if (0) return 1025; } else { if (!0) return 1026; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x <= y) != 0) return 1027;
	if (x <= y) { if (!0) return 1028; } else { if (0) return 1029; }
	if (!(x <= y)) { if (0) return 1030; } else { if (!0) return 1031; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x > y) != 1) return 1032;
	if (x > y) { if (!1) return 1033; } else { if (1) return 1034; }
	if (!(x > y)) { if (1) return 1035; } else { if (!1) return 1036; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x >= y) != 1) return 1037;
	if (x >= y) { if (!1) return 1038; } else { if (1) return 1039; }
	if (!(x >= y)) { if (1) return 1040; } else { if (!1) return 1041; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x == y) != 0) return 1042;
	if (x == y) { if (!0) return 1043; } else { if (0) return 1044; }
	if (!(x == y)) { if (0) return 1045; } else { if (!0) return 1046; }
	x = mk(0x3ffd053140868dcbUL, 0xf000000000000000UL); y = mk(0x0000000000000000UL, 0x0000000000000000UL);
	if ((x != y) != 1) return 1047;
	if (x != y) { if (!1) return 1048; } else { if (1) return 1049; }
	if (!(x != y)) { if (1) return 1050; } else { if (!1) return 1051; }
	x = mk(0xbffd0148a0170756UL, 0xa200000000000000UL); y = mk(0xbffd0148a0170756UL, 0xa200000000000000UL);
	if ((x < y) != 0) return 1052;
	if (x < y) { if (!0) return 1053; } else { if (0) return 1054; }
	if (!(x < y)) { if (0) return 1055; } else { if (!0) return 1056; }
	x = mk(0xbffd0148a0170756UL, 0xa200000000000000UL); y = mk(0xbffd0148a0170756UL, 0xa200000000000000UL);
	if ((x <= y) != 1) return 1057;
	if (x <= y) { if (!1) return 1058; } else { if (1) return 1059; }
	if (!(x <= y)) { if (1) return 1060; } else { if (!1) return 1061; }
	x = mk(0xbffd0148a0170756UL, 0xa200000000000000UL); y = mk(0xbffd0148a0170756UL, 0xa200000000000000UL);
	if ((x > y) != 0) return 1062;
	if (x > y) { if (!0) return 1063; } else { if (0) return 1064; }
	if (!(x > y)) { if (0) return 1065; } else { if (!0) return 1066; }
	x = mk(0xbffd0148a0170756UL, 0xa200000000000000UL); y = mk(0xbffd0148a0170756UL, 0xa200000000000000UL);
	if ((x >= y) != 1) return 1067;
	if (x >= y) { if (!1) return 1068; } else { if (1) return 1069; }
	if (!(x >= y)) { if (1) return 1070; } else { if (!1) return 1071; }
	x = mk(0xbffd0148a0170756UL, 0xa200000000000000UL); y = mk(0xbffd0148a0170756UL, 0xa200000000000000UL);
	if ((x == y) != 1) return 1072;
	if (x == y) { if (!1) return 1073; } else { if (1) return 1074; }
	if (!(x == y)) { if (1) return 1075; } else { if (!1) return 1076; }
	x = mk(0xbffd0148a0170756UL, 0xa200000000000000UL); y = mk(0xbffd0148a0170756UL, 0xa200000000000000UL);
	if ((x != y) != 0) return 1077;
	if (x != y) { if (!0) return 1078; } else { if (0) return 1079; }
	if (!(x != y)) { if (0) return 1080; } else { if (!0) return 1081; }
	x = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x < y) != 0) return 1082;
	if (x < y) { if (!0) return 1083; } else { if (0) return 1084; }
	if (!(x < y)) { if (0) return 1085; } else { if (!0) return 1086; }
	x = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x <= y) != 0) return 1087;
	if (x <= y) { if (!0) return 1088; } else { if (0) return 1089; }
	if (!(x <= y)) { if (0) return 1090; } else { if (!0) return 1091; }
	x = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x > y) != 1) return 1092;
	if (x > y) { if (!1) return 1093; } else { if (1) return 1094; }
	if (!(x > y)) { if (1) return 1095; } else { if (!1) return 1096; }
	x = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x >= y) != 1) return 1097;
	if (x >= y) { if (!1) return 1098; } else { if (1) return 1099; }
	if (!(x >= y)) { if (1) return 1100; } else { if (!1) return 1101; }
	x = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x == y) != 0) return 1102;
	if (x == y) { if (!0) return 1103; } else { if (0) return 1104; }
	if (!(x == y)) { if (0) return 1105; } else { if (!0) return 1106; }
	x = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL); y = mk(0xbffc1d963ced2681UL, 0x4096000000000000UL);
	if ((x != y) != 1) return 1107;
	if (x != y) { if (!1) return 1108; } else { if (1) return 1109; }
	if (!(x != y)) { if (1) return 1110; } else { if (!1) return 1111; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x < y) != 0) return 1112;
	if (x < y) { if (!0) return 1113; } else { if (0) return 1114; }
	if (!(x < y)) { if (0) return 1115; } else { if (!0) return 1116; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x <= y) != 0) return 1117;
	if (x <= y) { if (!0) return 1118; } else { if (0) return 1119; }
	if (!(x <= y)) { if (0) return 1120; } else { if (!0) return 1121; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x > y) != 1) return 1122;
	if (x > y) { if (!1) return 1123; } else { if (1) return 1124; }
	if (!(x > y)) { if (1) return 1125; } else { if (!1) return 1126; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x >= y) != 1) return 1127;
	if (x >= y) { if (!1) return 1128; } else { if (1) return 1129; }
	if (!(x >= y)) { if (1) return 1130; } else { if (!1) return 1131; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x == y) != 0) return 1132;
	if (x == y) { if (!0) return 1133; } else { if (0) return 1134; }
	if (!(x == y)) { if (0) return 1135; } else { if (!0) return 1136; }
	x = mk(0x0000000000000000UL, 0x0000000000000000UL); y = mk(0xbffcc02078b1cb6cUL, 0xaa1bc2e33e100000UL);
	if ((x != y) != 1) return 1137;
	if (x != y) { if (!1) return 1138; } else { if (1) return 1139; }
	if (!(x != y)) { if (1) return 1140; } else { if (!1) return 1141; }
	x = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x < y) != 0) return 1142;
	if (x < y) { if (!0) return 1143; } else { if (0) return 1144; }
	if (!(x < y)) { if (0) return 1145; } else { if (!0) return 1146; }
	x = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x <= y) != 1) return 1147;
	if (x <= y) { if (!1) return 1148; } else { if (1) return 1149; }
	if (!(x <= y)) { if (1) return 1150; } else { if (!1) return 1151; }
	x = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x > y) != 0) return 1152;
	if (x > y) { if (!0) return 1153; } else { if (0) return 1154; }
	if (!(x > y)) { if (0) return 1155; } else { if (!0) return 1156; }
	x = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x >= y) != 1) return 1157;
	if (x >= y) { if (!1) return 1158; } else { if (1) return 1159; }
	if (!(x >= y)) { if (1) return 1160; } else { if (!1) return 1161; }
	x = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x == y) != 1) return 1162;
	if (x == y) { if (!1) return 1163; } else { if (1) return 1164; }
	if (!(x == y)) { if (1) return 1165; } else { if (!1) return 1166; }
	x = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL); y = mk(0x2747f8f102af2ef5UL, 0x88000e81bf1c0260UL);
	if ((x != y) != 0) return 1167;
	if (x != y) { if (!0) return 1168; } else { if (0) return 1169; }
	if (!(x != y)) { if (0) return 1170; } else { if (!0) return 1171; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x < y) != 0) return 1172;
	if (x < y) { if (!0) return 1173; } else { if (0) return 1174; }
	if (!(x < y)) { if (0) return 1175; } else { if (!0) return 1176; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x <= y) != 1) return 1177;
	if (x <= y) { if (!1) return 1178; } else { if (1) return 1179; }
	if (!(x <= y)) { if (1) return 1180; } else { if (!1) return 1181; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x > y) != 0) return 1182;
	if (x > y) { if (!0) return 1183; } else { if (0) return 1184; }
	if (!(x > y)) { if (0) return 1185; } else { if (!0) return 1186; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x >= y) != 1) return 1187;
	if (x >= y) { if (!1) return 1188; } else { if (1) return 1189; }
	if (!(x >= y)) { if (1) return 1190; } else { if (!1) return 1191; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x == y) != 1) return 1192;
	if (x == y) { if (!1) return 1193; } else { if (1) return 1194; }
	if (!(x == y)) { if (1) return 1195; } else { if (!1) return 1196; }
	x = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL); y = mk(0xc001208e1148da69UL, 0x1cf65d217a65e4e4UL);
	if ((x != y) != 0) return 1197;
	if (x != y) { if (!0) return 1198; } else { if (0) return 1199; }
	if (!(x != y)) { if (0) return 1200; } else { if (!0) return 1201; }
	/* truth values */
	x = mk(0x3fff000000000000UL, 0); y = mk(0, 0); z = mk(0x8000000000000000UL, 0); n = mk(0x7fff800000000000UL, 0);
	if (!x) return 1202; if (y) return 1203; if (z) return 1204; if (!n) return 1205;
	if (!!y) return 1206; if (!!x == 0) return 1207;
	if (!(x && n)) return 1208; if (x && y) return 1209; if (y || z) return 1210; if (!(y || x)) return 1211;
	if ((x ? 5 : 6) != 5 || (y ? 5 : 6) != 6) return 1212;
	i = 0; w = mk(0x4001400000000000UL, 0);	/* 5.0 */
	one = mk(0x3fff000000000000UL, 0);
	while (w) { w -= one; i++; if (i > 10) return 1213; }
	if (i != 5) return 1214;
	for (w = mk(0x4000000000000000UL, 0); w; w -= one) i++;
	if (i != 7) return 1215;
	return 1;
}
