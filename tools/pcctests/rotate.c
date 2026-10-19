/* expect: 1 -- rotates by a constant: (x << n) | (x >> (w - n)), all counts, written both ways round, with a mask, 16-bit ones, and a side effect in the operand (not a rotate) */

unsigned long x;
unsigned short s;
int calls;
unsigned long next() { calls++; return x; }

int tv0()
{
	unsigned long r;

	x = 0x80000001L;
	r = (x << 1) | (x >> 31); if (r != 0x00000003L) return 2;
	r = (x >> 31) | (x << 1); if (r != 0x00000003L) return 3;
	r = (x << 2) | (x >> 30); if (r != 0x00000006L) return 4;
	r = (x >> 30) | (x << 2); if (r != 0x00000006L) return 5;
	r = (x << 3) | (x >> 29); if (r != 0x0000000cL) return 6;
	r = (x >> 29) | (x << 3); if (r != 0x0000000cL) return 7;
	r = (x << 4) | (x >> 28); if (r != 0x00000018L) return 8;
	r = (x >> 28) | (x << 4); if (r != 0x00000018L) return 9;
	r = (x << 5) | (x >> 27); if (r != 0x00000030L) return 10;
	r = (x >> 27) | (x << 5); if (r != 0x00000030L) return 11;
	r = (x << 6) | (x >> 26); if (r != 0x00000060L) return 12;
	r = (x >> 26) | (x << 6); if (r != 0x00000060L) return 13;
	r = (x << 7) | (x >> 25); if (r != 0x000000c0L) return 14;
	r = (x >> 25) | (x << 7); if (r != 0x000000c0L) return 15;
	r = ((x & 0xFFFFFFFF) >> 25) | (x << 7); if (r != 0x000000c0L) return 16;
	r = (x << 8) | (x >> 24); if (r != 0x00000180L) return 17;
	r = (x >> 24) | (x << 8); if (r != 0x00000180L) return 18;
	r = (x << 9) | (x >> 23); if (r != 0x00000300L) return 19;
	r = (x >> 23) | (x << 9); if (r != 0x00000300L) return 20;
	r = (x << 10) | (x >> 22); if (r != 0x00000600L) return 21;
	r = (x >> 22) | (x << 10); if (r != 0x00000600L) return 22;
	r = (x << 11) | (x >> 21); if (r != 0x00000c00L) return 23;
	r = (x >> 21) | (x << 11); if (r != 0x00000c00L) return 24;
	r = (x << 12) | (x >> 20); if (r != 0x00001800L) return 25;
	r = (x >> 20) | (x << 12); if (r != 0x00001800L) return 26;
	r = (x << 13) | (x >> 19); if (r != 0x00003000L) return 27;
	r = (x >> 19) | (x << 13); if (r != 0x00003000L) return 28;
	r = (x << 14) | (x >> 18); if (r != 0x00006000L) return 29;
	r = (x >> 18) | (x << 14); if (r != 0x00006000L) return 30;
	r = ((x & 0xFFFFFFFF) >> 18) | (x << 14); if (r != 0x00006000L) return 31;
	r = (x << 15) | (x >> 17); if (r != 0x0000c000L) return 32;
	r = (x >> 17) | (x << 15); if (r != 0x0000c000L) return 33;
	r = (x << 16) | (x >> 16); if (r != 0x00018000L) return 34;
	r = (x >> 16) | (x << 16); if (r != 0x00018000L) return 35;
	r = (x << 17) | (x >> 15); if (r != 0x00030000L) return 36;
	r = (x >> 15) | (x << 17); if (r != 0x00030000L) return 37;
	r = (x << 18) | (x >> 14); if (r != 0x00060000L) return 38;
	r = (x >> 14) | (x << 18); if (r != 0x00060000L) return 39;
	r = (x << 19) | (x >> 13); if (r != 0x000c0000L) return 40;
	r = (x >> 13) | (x << 19); if (r != 0x000c0000L) return 41;
	r = (x << 20) | (x >> 12); if (r != 0x00180000L) return 42;
	r = (x >> 12) | (x << 20); if (r != 0x00180000L) return 43;
	r = (x << 21) | (x >> 11); if (r != 0x00300000L) return 44;
	r = (x >> 11) | (x << 21); if (r != 0x00300000L) return 45;
	r = ((x & 0xFFFFFFFF) >> 11) | (x << 21); if (r != 0x00300000L) return 46;
	r = (x << 22) | (x >> 10); if (r != 0x00600000L) return 47;
	r = (x >> 10) | (x << 22); if (r != 0x00600000L) return 48;
	r = (x << 23) | (x >> 9); if (r != 0x00c00000L) return 49;
	r = (x >> 9) | (x << 23); if (r != 0x00c00000L) return 50;
	r = (x << 24) | (x >> 8); if (r != 0x01800000L) return 51;
	r = (x >> 8) | (x << 24); if (r != 0x01800000L) return 52;
	r = (x << 25) | (x >> 7); if (r != 0x03000000L) return 53;
	r = (x >> 7) | (x << 25); if (r != 0x03000000L) return 54;
	r = (x << 26) | (x >> 6); if (r != 0x06000000L) return 55;
	r = (x >> 6) | (x << 26); if (r != 0x06000000L) return 56;
	r = (x << 27) | (x >> 5); if (r != 0x0c000000L) return 57;
	r = (x >> 5) | (x << 27); if (r != 0x0c000000L) return 58;
	r = (x << 28) | (x >> 4); if (r != 0x18000000L) return 59;
	r = (x >> 4) | (x << 28); if (r != 0x18000000L) return 60;
	r = ((x & 0xFFFFFFFF) >> 4) | (x << 28); if (r != 0x18000000L) return 61;
	r = (x << 29) | (x >> 3); if (r != 0x30000000L) return 62;
	r = (x >> 3) | (x << 29); if (r != 0x30000000L) return 63;
	r = (x << 30) | (x >> 2); if (r != 0x60000000L) return 64;
	r = (x >> 2) | (x << 30); if (r != 0x60000000L) return 65;
	r = (x << 31) | (x >> 1); if (r != 0xc0000000L) return 66;
	r = (x >> 1) | (x << 31); if (r != 0xc0000000L) return 67;
	return 0;
}

int tv1()
{
	unsigned long r;

	x = 0xffffffffL;
	r = (x << 1) | (x >> 31); if (r != 0xffffffffL) return 68;
	r = (x >> 31) | (x << 1); if (r != 0xffffffffL) return 69;
	r = (x << 2) | (x >> 30); if (r != 0xffffffffL) return 70;
	r = (x >> 30) | (x << 2); if (r != 0xffffffffL) return 71;
	r = (x << 3) | (x >> 29); if (r != 0xffffffffL) return 72;
	r = (x >> 29) | (x << 3); if (r != 0xffffffffL) return 73;
	r = (x << 4) | (x >> 28); if (r != 0xffffffffL) return 74;
	r = (x >> 28) | (x << 4); if (r != 0xffffffffL) return 75;
	r = (x << 5) | (x >> 27); if (r != 0xffffffffL) return 76;
	r = (x >> 27) | (x << 5); if (r != 0xffffffffL) return 77;
	r = (x << 6) | (x >> 26); if (r != 0xffffffffL) return 78;
	r = (x >> 26) | (x << 6); if (r != 0xffffffffL) return 79;
	r = (x << 7) | (x >> 25); if (r != 0xffffffffL) return 80;
	r = (x >> 25) | (x << 7); if (r != 0xffffffffL) return 81;
	r = ((x & 0xFFFFFFFF) >> 25) | (x << 7); if (r != 0xffffffffL) return 82;
	r = (x << 8) | (x >> 24); if (r != 0xffffffffL) return 83;
	r = (x >> 24) | (x << 8); if (r != 0xffffffffL) return 84;
	r = (x << 9) | (x >> 23); if (r != 0xffffffffL) return 85;
	r = (x >> 23) | (x << 9); if (r != 0xffffffffL) return 86;
	r = (x << 10) | (x >> 22); if (r != 0xffffffffL) return 87;
	r = (x >> 22) | (x << 10); if (r != 0xffffffffL) return 88;
	r = (x << 11) | (x >> 21); if (r != 0xffffffffL) return 89;
	r = (x >> 21) | (x << 11); if (r != 0xffffffffL) return 90;
	r = (x << 12) | (x >> 20); if (r != 0xffffffffL) return 91;
	r = (x >> 20) | (x << 12); if (r != 0xffffffffL) return 92;
	r = (x << 13) | (x >> 19); if (r != 0xffffffffL) return 93;
	r = (x >> 19) | (x << 13); if (r != 0xffffffffL) return 94;
	r = (x << 14) | (x >> 18); if (r != 0xffffffffL) return 95;
	r = (x >> 18) | (x << 14); if (r != 0xffffffffL) return 96;
	r = ((x & 0xFFFFFFFF) >> 18) | (x << 14); if (r != 0xffffffffL) return 97;
	r = (x << 15) | (x >> 17); if (r != 0xffffffffL) return 98;
	r = (x >> 17) | (x << 15); if (r != 0xffffffffL) return 99;
	r = (x << 16) | (x >> 16); if (r != 0xffffffffL) return 100;
	r = (x >> 16) | (x << 16); if (r != 0xffffffffL) return 101;
	r = (x << 17) | (x >> 15); if (r != 0xffffffffL) return 102;
	r = (x >> 15) | (x << 17); if (r != 0xffffffffL) return 103;
	r = (x << 18) | (x >> 14); if (r != 0xffffffffL) return 104;
	r = (x >> 14) | (x << 18); if (r != 0xffffffffL) return 105;
	r = (x << 19) | (x >> 13); if (r != 0xffffffffL) return 106;
	r = (x >> 13) | (x << 19); if (r != 0xffffffffL) return 107;
	r = (x << 20) | (x >> 12); if (r != 0xffffffffL) return 108;
	r = (x >> 12) | (x << 20); if (r != 0xffffffffL) return 109;
	r = (x << 21) | (x >> 11); if (r != 0xffffffffL) return 110;
	r = (x >> 11) | (x << 21); if (r != 0xffffffffL) return 111;
	r = ((x & 0xFFFFFFFF) >> 11) | (x << 21); if (r != 0xffffffffL) return 112;
	r = (x << 22) | (x >> 10); if (r != 0xffffffffL) return 113;
	r = (x >> 10) | (x << 22); if (r != 0xffffffffL) return 114;
	r = (x << 23) | (x >> 9); if (r != 0xffffffffL) return 115;
	r = (x >> 9) | (x << 23); if (r != 0xffffffffL) return 116;
	r = (x << 24) | (x >> 8); if (r != 0xffffffffL) return 117;
	r = (x >> 8) | (x << 24); if (r != 0xffffffffL) return 118;
	r = (x << 25) | (x >> 7); if (r != 0xffffffffL) return 119;
	r = (x >> 7) | (x << 25); if (r != 0xffffffffL) return 120;
	r = (x << 26) | (x >> 6); if (r != 0xffffffffL) return 121;
	r = (x >> 6) | (x << 26); if (r != 0xffffffffL) return 122;
	r = (x << 27) | (x >> 5); if (r != 0xffffffffL) return 123;
	r = (x >> 5) | (x << 27); if (r != 0xffffffffL) return 124;
	r = (x << 28) | (x >> 4); if (r != 0xffffffffL) return 125;
	r = (x >> 4) | (x << 28); if (r != 0xffffffffL) return 126;
	r = ((x & 0xFFFFFFFF) >> 4) | (x << 28); if (r != 0xffffffffL) return 127;
	r = (x << 29) | (x >> 3); if (r != 0xffffffffL) return 128;
	r = (x >> 3) | (x << 29); if (r != 0xffffffffL) return 129;
	r = (x << 30) | (x >> 2); if (r != 0xffffffffL) return 130;
	r = (x >> 2) | (x << 30); if (r != 0xffffffffL) return 131;
	r = (x << 31) | (x >> 1); if (r != 0xffffffffL) return 132;
	r = (x >> 1) | (x << 31); if (r != 0xffffffffL) return 133;
	return 0;
}

int tv2()
{
	unsigned long r;

	x = 0x12345678L;
	r = (x << 1) | (x >> 31); if (r != 0x2468acf0L) return 134;
	r = (x >> 31) | (x << 1); if (r != 0x2468acf0L) return 135;
	r = (x << 2) | (x >> 30); if (r != 0x48d159e0L) return 136;
	r = (x >> 30) | (x << 2); if (r != 0x48d159e0L) return 137;
	r = (x << 3) | (x >> 29); if (r != 0x91a2b3c0L) return 138;
	r = (x >> 29) | (x << 3); if (r != 0x91a2b3c0L) return 139;
	r = (x << 4) | (x >> 28); if (r != 0x23456781L) return 140;
	r = (x >> 28) | (x << 4); if (r != 0x23456781L) return 141;
	r = (x << 5) | (x >> 27); if (r != 0x468acf02L) return 142;
	r = (x >> 27) | (x << 5); if (r != 0x468acf02L) return 143;
	r = (x << 6) | (x >> 26); if (r != 0x8d159e04L) return 144;
	r = (x >> 26) | (x << 6); if (r != 0x8d159e04L) return 145;
	r = (x << 7) | (x >> 25); if (r != 0x1a2b3c09L) return 146;
	r = (x >> 25) | (x << 7); if (r != 0x1a2b3c09L) return 147;
	r = ((x & 0xFFFFFFFF) >> 25) | (x << 7); if (r != 0x1a2b3c09L) return 148;
	r = (x << 8) | (x >> 24); if (r != 0x34567812L) return 149;
	r = (x >> 24) | (x << 8); if (r != 0x34567812L) return 150;
	r = (x << 9) | (x >> 23); if (r != 0x68acf024L) return 151;
	r = (x >> 23) | (x << 9); if (r != 0x68acf024L) return 152;
	r = (x << 10) | (x >> 22); if (r != 0xd159e048L) return 153;
	r = (x >> 22) | (x << 10); if (r != 0xd159e048L) return 154;
	r = (x << 11) | (x >> 21); if (r != 0xa2b3c091L) return 155;
	r = (x >> 21) | (x << 11); if (r != 0xa2b3c091L) return 156;
	r = (x << 12) | (x >> 20); if (r != 0x45678123L) return 157;
	r = (x >> 20) | (x << 12); if (r != 0x45678123L) return 158;
	r = (x << 13) | (x >> 19); if (r != 0x8acf0246L) return 159;
	r = (x >> 19) | (x << 13); if (r != 0x8acf0246L) return 160;
	r = (x << 14) | (x >> 18); if (r != 0x159e048dL) return 161;
	r = (x >> 18) | (x << 14); if (r != 0x159e048dL) return 162;
	r = ((x & 0xFFFFFFFF) >> 18) | (x << 14); if (r != 0x159e048dL) return 163;
	r = (x << 15) | (x >> 17); if (r != 0x2b3c091aL) return 164;
	r = (x >> 17) | (x << 15); if (r != 0x2b3c091aL) return 165;
	r = (x << 16) | (x >> 16); if (r != 0x56781234L) return 166;
	r = (x >> 16) | (x << 16); if (r != 0x56781234L) return 167;
	r = (x << 17) | (x >> 15); if (r != 0xacf02468L) return 168;
	r = (x >> 15) | (x << 17); if (r != 0xacf02468L) return 169;
	r = (x << 18) | (x >> 14); if (r != 0x59e048d1L) return 170;
	r = (x >> 14) | (x << 18); if (r != 0x59e048d1L) return 171;
	r = (x << 19) | (x >> 13); if (r != 0xb3c091a2L) return 172;
	r = (x >> 13) | (x << 19); if (r != 0xb3c091a2L) return 173;
	r = (x << 20) | (x >> 12); if (r != 0x67812345L) return 174;
	r = (x >> 12) | (x << 20); if (r != 0x67812345L) return 175;
	r = (x << 21) | (x >> 11); if (r != 0xcf02468aL) return 176;
	r = (x >> 11) | (x << 21); if (r != 0xcf02468aL) return 177;
	r = ((x & 0xFFFFFFFF) >> 11) | (x << 21); if (r != 0xcf02468aL) return 178;
	r = (x << 22) | (x >> 10); if (r != 0x9e048d15L) return 179;
	r = (x >> 10) | (x << 22); if (r != 0x9e048d15L) return 180;
	r = (x << 23) | (x >> 9); if (r != 0x3c091a2bL) return 181;
	r = (x >> 9) | (x << 23); if (r != 0x3c091a2bL) return 182;
	r = (x << 24) | (x >> 8); if (r != 0x78123456L) return 183;
	r = (x >> 8) | (x << 24); if (r != 0x78123456L) return 184;
	r = (x << 25) | (x >> 7); if (r != 0xf02468acL) return 185;
	r = (x >> 7) | (x << 25); if (r != 0xf02468acL) return 186;
	r = (x << 26) | (x >> 6); if (r != 0xe048d159L) return 187;
	r = (x >> 6) | (x << 26); if (r != 0xe048d159L) return 188;
	r = (x << 27) | (x >> 5); if (r != 0xc091a2b3L) return 189;
	r = (x >> 5) | (x << 27); if (r != 0xc091a2b3L) return 190;
	r = (x << 28) | (x >> 4); if (r != 0x81234567L) return 191;
	r = (x >> 4) | (x << 28); if (r != 0x81234567L) return 192;
	r = ((x & 0xFFFFFFFF) >> 4) | (x << 28); if (r != 0x81234567L) return 193;
	r = (x << 29) | (x >> 3); if (r != 0x02468acfL) return 194;
	r = (x >> 3) | (x << 29); if (r != 0x02468acfL) return 195;
	r = (x << 30) | (x >> 2); if (r != 0x048d159eL) return 196;
	r = (x >> 2) | (x << 30); if (r != 0x048d159eL) return 197;
	r = (x << 31) | (x >> 1); if (r != 0x091a2b3cL) return 198;
	r = (x >> 1) | (x << 31); if (r != 0x091a2b3cL) return 199;
	return 0;
}

int tv3()
{
	unsigned long r;

	x = 0x00010000L;
	r = (x << 1) | (x >> 31); if (r != 0x00020000L) return 200;
	r = (x >> 31) | (x << 1); if (r != 0x00020000L) return 201;
	r = (x << 2) | (x >> 30); if (r != 0x00040000L) return 202;
	r = (x >> 30) | (x << 2); if (r != 0x00040000L) return 203;
	r = (x << 3) | (x >> 29); if (r != 0x00080000L) return 204;
	r = (x >> 29) | (x << 3); if (r != 0x00080000L) return 205;
	r = (x << 4) | (x >> 28); if (r != 0x00100000L) return 206;
	r = (x >> 28) | (x << 4); if (r != 0x00100000L) return 207;
	r = (x << 5) | (x >> 27); if (r != 0x00200000L) return 208;
	r = (x >> 27) | (x << 5); if (r != 0x00200000L) return 209;
	r = (x << 6) | (x >> 26); if (r != 0x00400000L) return 210;
	r = (x >> 26) | (x << 6); if (r != 0x00400000L) return 211;
	r = (x << 7) | (x >> 25); if (r != 0x00800000L) return 212;
	r = (x >> 25) | (x << 7); if (r != 0x00800000L) return 213;
	r = ((x & 0xFFFFFFFF) >> 25) | (x << 7); if (r != 0x00800000L) return 214;
	r = (x << 8) | (x >> 24); if (r != 0x01000000L) return 215;
	r = (x >> 24) | (x << 8); if (r != 0x01000000L) return 216;
	r = (x << 9) | (x >> 23); if (r != 0x02000000L) return 217;
	r = (x >> 23) | (x << 9); if (r != 0x02000000L) return 218;
	r = (x << 10) | (x >> 22); if (r != 0x04000000L) return 219;
	r = (x >> 22) | (x << 10); if (r != 0x04000000L) return 220;
	r = (x << 11) | (x >> 21); if (r != 0x08000000L) return 221;
	r = (x >> 21) | (x << 11); if (r != 0x08000000L) return 222;
	r = (x << 12) | (x >> 20); if (r != 0x10000000L) return 223;
	r = (x >> 20) | (x << 12); if (r != 0x10000000L) return 224;
	r = (x << 13) | (x >> 19); if (r != 0x20000000L) return 225;
	r = (x >> 19) | (x << 13); if (r != 0x20000000L) return 226;
	r = (x << 14) | (x >> 18); if (r != 0x40000000L) return 227;
	r = (x >> 18) | (x << 14); if (r != 0x40000000L) return 228;
	r = ((x & 0xFFFFFFFF) >> 18) | (x << 14); if (r != 0x40000000L) return 229;
	r = (x << 15) | (x >> 17); if (r != 0x80000000L) return 230;
	r = (x >> 17) | (x << 15); if (r != 0x80000000L) return 231;
	r = (x << 16) | (x >> 16); if (r != 0x00000001L) return 232;
	r = (x >> 16) | (x << 16); if (r != 0x00000001L) return 233;
	r = (x << 17) | (x >> 15); if (r != 0x00000002L) return 234;
	r = (x >> 15) | (x << 17); if (r != 0x00000002L) return 235;
	r = (x << 18) | (x >> 14); if (r != 0x00000004L) return 236;
	r = (x >> 14) | (x << 18); if (r != 0x00000004L) return 237;
	r = (x << 19) | (x >> 13); if (r != 0x00000008L) return 238;
	r = (x >> 13) | (x << 19); if (r != 0x00000008L) return 239;
	r = (x << 20) | (x >> 12); if (r != 0x00000010L) return 240;
	r = (x >> 12) | (x << 20); if (r != 0x00000010L) return 241;
	r = (x << 21) | (x >> 11); if (r != 0x00000020L) return 242;
	r = (x >> 11) | (x << 21); if (r != 0x00000020L) return 243;
	r = ((x & 0xFFFFFFFF) >> 11) | (x << 21); if (r != 0x00000020L) return 244;
	r = (x << 22) | (x >> 10); if (r != 0x00000040L) return 245;
	r = (x >> 10) | (x << 22); if (r != 0x00000040L) return 246;
	r = (x << 23) | (x >> 9); if (r != 0x00000080L) return 247;
	r = (x >> 9) | (x << 23); if (r != 0x00000080L) return 248;
	r = (x << 24) | (x >> 8); if (r != 0x00000100L) return 249;
	r = (x >> 8) | (x << 24); if (r != 0x00000100L) return 250;
	r = (x << 25) | (x >> 7); if (r != 0x00000200L) return 251;
	r = (x >> 7) | (x << 25); if (r != 0x00000200L) return 252;
	r = (x << 26) | (x >> 6); if (r != 0x00000400L) return 253;
	r = (x >> 6) | (x << 26); if (r != 0x00000400L) return 254;
	r = (x << 27) | (x >> 5); if (r != 0x00000800L) return 255;
	r = (x >> 5) | (x << 27); if (r != 0x00000800L) return 256;
	r = (x << 28) | (x >> 4); if (r != 0x00001000L) return 257;
	r = (x >> 4) | (x << 28); if (r != 0x00001000L) return 258;
	r = ((x & 0xFFFFFFFF) >> 4) | (x << 28); if (r != 0x00001000L) return 259;
	r = (x << 29) | (x >> 3); if (r != 0x00002000L) return 260;
	r = (x >> 3) | (x << 29); if (r != 0x00002000L) return 261;
	r = (x << 30) | (x >> 2); if (r != 0x00004000L) return 262;
	r = (x >> 2) | (x << 30); if (r != 0x00004000L) return 263;
	r = (x << 31) | (x >> 1); if (r != 0x00008000L) return 264;
	r = (x >> 1) | (x << 31); if (r != 0x00008000L) return 265;
	return 0;
}

int tside()
{
	unsigned long r;

	x = 0x12345678L; calls = 0;
	r = (next() << 5) | (next() >> 27); if (calls != 2 || r != 0x468acf02L) return 662;
	return 0;
}

int t16()
{
	unsigned long r;

	s = 0x8001;
	r = (unsigned short)((s << 1) | (s >> 15)); if (r != 0x0003L) return 663;
	r = (unsigned short)((s << 3) | (s >> 13)); if (r != 0x000cL) return 664;
	r = (unsigned short)((s << 8) | (s >> 8)); if (r != 0x0180L) return 665;
	r = (unsigned short)((s << 13) | (s >> 3)); if (r != 0x3000L) return 666;
	r = (unsigned short)((s << 15) | (s >> 1)); if (r != 0xc000L) return 667;
	s = 0xabcd;
	r = (unsigned short)((s << 1) | (s >> 15)); if (r != 0x579bL) return 668;
	r = (unsigned short)((s << 3) | (s >> 13)); if (r != 0x5e6dL) return 669;
	r = (unsigned short)((s << 8) | (s >> 8)); if (r != 0xcdabL) return 670;
	r = (unsigned short)((s << 13) | (s >> 3)); if (r != 0xb579L) return 671;
	r = (unsigned short)((s << 15) | (s >> 1)); if (r != 0xd5e6L) return 672;
	s = 0x0001;
	r = (unsigned short)((s << 1) | (s >> 15)); if (r != 0x0002L) return 673;
	r = (unsigned short)((s << 3) | (s >> 13)); if (r != 0x0008L) return 674;
	r = (unsigned short)((s << 8) | (s >> 8)); if (r != 0x0100L) return 675;
	r = (unsigned short)((s << 13) | (s >> 3)); if (r != 0x2000L) return 676;
	r = (unsigned short)((s << 15) | (s >> 1)); if (r != 0x8000L) return 677;
	return 0;
}

int test()
{
	int r;

	if ((r = tv0()) != 0) return r;
	if ((r = tv1()) != 0) return r;
	if ((r = tv2()) != 0) return r;
	if ((r = tv3()) != 0) return r;
	if ((r = tside()) != 0) return r;
	if ((r = t16()) != 0) return r;
	return 1;
}
