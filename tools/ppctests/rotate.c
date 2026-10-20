/* expect: 1 -- rotates by a constant: (x << n) | (x >> (w - n)), all counts, written both ways round, with a mask, 16-bit ones, and a side effect in the operand (not a rotate) */

unsigned x;
unsigned short s;
int calls;
unsigned next() { calls++; return x; }

int tv0()
{
	unsigned r;

	x = 0x80000001U;
	r = (x << 1) | (x >> 31); if (r != 0x00000003U) return 2;
	r = (x >> 31) | (x << 1); if (r != 0x00000003U) return 3;
	r = (x << 2) | (x >> 30); if (r != 0x00000006U) return 4;
	r = (x >> 30) | (x << 2); if (r != 0x00000006U) return 5;
	r = (x << 3) | (x >> 29); if (r != 0x0000000cU) return 6;
	r = (x >> 29) | (x << 3); if (r != 0x0000000cU) return 7;
	r = (x << 4) | (x >> 28); if (r != 0x00000018U) return 8;
	r = (x >> 28) | (x << 4); if (r != 0x00000018U) return 9;
	r = (x << 5) | (x >> 27); if (r != 0x00000030U) return 10;
	r = (x >> 27) | (x << 5); if (r != 0x00000030U) return 11;
	r = (x << 6) | (x >> 26); if (r != 0x00000060U) return 12;
	r = (x >> 26) | (x << 6); if (r != 0x00000060U) return 13;
	r = (x << 7) | (x >> 25); if (r != 0x000000c0U) return 14;
	r = (x >> 25) | (x << 7); if (r != 0x000000c0U) return 15;
	r = ((x & 0xFFFFFFFF) >> 25) | (x << 7); if (r != 0x000000c0U) return 16;
	r = (x << 8) | (x >> 24); if (r != 0x00000180U) return 17;
	r = (x >> 24) | (x << 8); if (r != 0x00000180U) return 18;
	r = (x << 9) | (x >> 23); if (r != 0x00000300U) return 19;
	r = (x >> 23) | (x << 9); if (r != 0x00000300U) return 20;
	r = (x << 10) | (x >> 22); if (r != 0x00000600U) return 21;
	r = (x >> 22) | (x << 10); if (r != 0x00000600U) return 22;
	r = (x << 11) | (x >> 21); if (r != 0x00000c00U) return 23;
	r = (x >> 21) | (x << 11); if (r != 0x00000c00U) return 24;
	r = (x << 12) | (x >> 20); if (r != 0x00001800U) return 25;
	r = (x >> 20) | (x << 12); if (r != 0x00001800U) return 26;
	r = (x << 13) | (x >> 19); if (r != 0x00003000U) return 27;
	r = (x >> 19) | (x << 13); if (r != 0x00003000U) return 28;
	r = (x << 14) | (x >> 18); if (r != 0x00006000U) return 29;
	r = (x >> 18) | (x << 14); if (r != 0x00006000U) return 30;
	r = ((x & 0xFFFFFFFF) >> 18) | (x << 14); if (r != 0x00006000U) return 31;
	r = (x << 15) | (x >> 17); if (r != 0x0000c000U) return 32;
	r = (x >> 17) | (x << 15); if (r != 0x0000c000U) return 33;
	r = (x << 16) | (x >> 16); if (r != 0x00018000U) return 34;
	r = (x >> 16) | (x << 16); if (r != 0x00018000U) return 35;
	r = (x << 17) | (x >> 15); if (r != 0x00030000U) return 36;
	r = (x >> 15) | (x << 17); if (r != 0x00030000U) return 37;
	r = (x << 18) | (x >> 14); if (r != 0x00060000U) return 38;
	r = (x >> 14) | (x << 18); if (r != 0x00060000U) return 39;
	r = (x << 19) | (x >> 13); if (r != 0x000c0000U) return 40;
	r = (x >> 13) | (x << 19); if (r != 0x000c0000U) return 41;
	r = (x << 20) | (x >> 12); if (r != 0x00180000U) return 42;
	r = (x >> 12) | (x << 20); if (r != 0x00180000U) return 43;
	r = (x << 21) | (x >> 11); if (r != 0x00300000U) return 44;
	r = (x >> 11) | (x << 21); if (r != 0x00300000U) return 45;
	r = ((x & 0xFFFFFFFF) >> 11) | (x << 21); if (r != 0x00300000U) return 46;
	r = (x << 22) | (x >> 10); if (r != 0x00600000U) return 47;
	r = (x >> 10) | (x << 22); if (r != 0x00600000U) return 48;
	r = (x << 23) | (x >> 9); if (r != 0x00c00000U) return 49;
	r = (x >> 9) | (x << 23); if (r != 0x00c00000U) return 50;
	r = (x << 24) | (x >> 8); if (r != 0x01800000U) return 51;
	r = (x >> 8) | (x << 24); if (r != 0x01800000U) return 52;
	r = (x << 25) | (x >> 7); if (r != 0x03000000U) return 53;
	r = (x >> 7) | (x << 25); if (r != 0x03000000U) return 54;
	r = (x << 26) | (x >> 6); if (r != 0x06000000U) return 55;
	r = (x >> 6) | (x << 26); if (r != 0x06000000U) return 56;
	r = (x << 27) | (x >> 5); if (r != 0x0c000000U) return 57;
	r = (x >> 5) | (x << 27); if (r != 0x0c000000U) return 58;
	r = (x << 28) | (x >> 4); if (r != 0x18000000U) return 59;
	r = (x >> 4) | (x << 28); if (r != 0x18000000U) return 60;
	r = ((x & 0xFFFFFFFF) >> 4) | (x << 28); if (r != 0x18000000U) return 61;
	r = (x << 29) | (x >> 3); if (r != 0x30000000U) return 62;
	r = (x >> 3) | (x << 29); if (r != 0x30000000U) return 63;
	r = (x << 30) | (x >> 2); if (r != 0x60000000U) return 64;
	r = (x >> 2) | (x << 30); if (r != 0x60000000U) return 65;
	r = (x << 31) | (x >> 1); if (r != 0xc0000000U) return 66;
	r = (x >> 1) | (x << 31); if (r != 0xc0000000U) return 67;
	return 0;
}

int tv1()
{
	unsigned r;

	x = 0xffffffffU;
	r = (x << 1) | (x >> 31); if (r != 0xffffffffU) return 68;
	r = (x >> 31) | (x << 1); if (r != 0xffffffffU) return 69;
	r = (x << 2) | (x >> 30); if (r != 0xffffffffU) return 70;
	r = (x >> 30) | (x << 2); if (r != 0xffffffffU) return 71;
	r = (x << 3) | (x >> 29); if (r != 0xffffffffU) return 72;
	r = (x >> 29) | (x << 3); if (r != 0xffffffffU) return 73;
	r = (x << 4) | (x >> 28); if (r != 0xffffffffU) return 74;
	r = (x >> 28) | (x << 4); if (r != 0xffffffffU) return 75;
	r = (x << 5) | (x >> 27); if (r != 0xffffffffU) return 76;
	r = (x >> 27) | (x << 5); if (r != 0xffffffffU) return 77;
	r = (x << 6) | (x >> 26); if (r != 0xffffffffU) return 78;
	r = (x >> 26) | (x << 6); if (r != 0xffffffffU) return 79;
	r = (x << 7) | (x >> 25); if (r != 0xffffffffU) return 80;
	r = (x >> 25) | (x << 7); if (r != 0xffffffffU) return 81;
	r = ((x & 0xFFFFFFFF) >> 25) | (x << 7); if (r != 0xffffffffU) return 82;
	r = (x << 8) | (x >> 24); if (r != 0xffffffffU) return 83;
	r = (x >> 24) | (x << 8); if (r != 0xffffffffU) return 84;
	r = (x << 9) | (x >> 23); if (r != 0xffffffffU) return 85;
	r = (x >> 23) | (x << 9); if (r != 0xffffffffU) return 86;
	r = (x << 10) | (x >> 22); if (r != 0xffffffffU) return 87;
	r = (x >> 22) | (x << 10); if (r != 0xffffffffU) return 88;
	r = (x << 11) | (x >> 21); if (r != 0xffffffffU) return 89;
	r = (x >> 21) | (x << 11); if (r != 0xffffffffU) return 90;
	r = (x << 12) | (x >> 20); if (r != 0xffffffffU) return 91;
	r = (x >> 20) | (x << 12); if (r != 0xffffffffU) return 92;
	r = (x << 13) | (x >> 19); if (r != 0xffffffffU) return 93;
	r = (x >> 19) | (x << 13); if (r != 0xffffffffU) return 94;
	r = (x << 14) | (x >> 18); if (r != 0xffffffffU) return 95;
	r = (x >> 18) | (x << 14); if (r != 0xffffffffU) return 96;
	r = ((x & 0xFFFFFFFF) >> 18) | (x << 14); if (r != 0xffffffffU) return 97;
	r = (x << 15) | (x >> 17); if (r != 0xffffffffU) return 98;
	r = (x >> 17) | (x << 15); if (r != 0xffffffffU) return 99;
	r = (x << 16) | (x >> 16); if (r != 0xffffffffU) return 100;
	r = (x >> 16) | (x << 16); if (r != 0xffffffffU) return 101;
	r = (x << 17) | (x >> 15); if (r != 0xffffffffU) return 102;
	r = (x >> 15) | (x << 17); if (r != 0xffffffffU) return 103;
	r = (x << 18) | (x >> 14); if (r != 0xffffffffU) return 104;
	r = (x >> 14) | (x << 18); if (r != 0xffffffffU) return 105;
	r = (x << 19) | (x >> 13); if (r != 0xffffffffU) return 106;
	r = (x >> 13) | (x << 19); if (r != 0xffffffffU) return 107;
	r = (x << 20) | (x >> 12); if (r != 0xffffffffU) return 108;
	r = (x >> 12) | (x << 20); if (r != 0xffffffffU) return 109;
	r = (x << 21) | (x >> 11); if (r != 0xffffffffU) return 110;
	r = (x >> 11) | (x << 21); if (r != 0xffffffffU) return 111;
	r = ((x & 0xFFFFFFFF) >> 11) | (x << 21); if (r != 0xffffffffU) return 112;
	r = (x << 22) | (x >> 10); if (r != 0xffffffffU) return 113;
	r = (x >> 10) | (x << 22); if (r != 0xffffffffU) return 114;
	r = (x << 23) | (x >> 9); if (r != 0xffffffffU) return 115;
	r = (x >> 9) | (x << 23); if (r != 0xffffffffU) return 116;
	r = (x << 24) | (x >> 8); if (r != 0xffffffffU) return 117;
	r = (x >> 8) | (x << 24); if (r != 0xffffffffU) return 118;
	r = (x << 25) | (x >> 7); if (r != 0xffffffffU) return 119;
	r = (x >> 7) | (x << 25); if (r != 0xffffffffU) return 120;
	r = (x << 26) | (x >> 6); if (r != 0xffffffffU) return 121;
	r = (x >> 6) | (x << 26); if (r != 0xffffffffU) return 122;
	r = (x << 27) | (x >> 5); if (r != 0xffffffffU) return 123;
	r = (x >> 5) | (x << 27); if (r != 0xffffffffU) return 124;
	r = (x << 28) | (x >> 4); if (r != 0xffffffffU) return 125;
	r = (x >> 4) | (x << 28); if (r != 0xffffffffU) return 126;
	r = ((x & 0xFFFFFFFF) >> 4) | (x << 28); if (r != 0xffffffffU) return 127;
	r = (x << 29) | (x >> 3); if (r != 0xffffffffU) return 128;
	r = (x >> 3) | (x << 29); if (r != 0xffffffffU) return 129;
	r = (x << 30) | (x >> 2); if (r != 0xffffffffU) return 130;
	r = (x >> 2) | (x << 30); if (r != 0xffffffffU) return 131;
	r = (x << 31) | (x >> 1); if (r != 0xffffffffU) return 132;
	r = (x >> 1) | (x << 31); if (r != 0xffffffffU) return 133;
	return 0;
}

int tv2()
{
	unsigned r;

	x = 0x12345678U;
	r = (x << 1) | (x >> 31); if (r != 0x2468acf0U) return 134;
	r = (x >> 31) | (x << 1); if (r != 0x2468acf0U) return 135;
	r = (x << 2) | (x >> 30); if (r != 0x48d159e0U) return 136;
	r = (x >> 30) | (x << 2); if (r != 0x48d159e0U) return 137;
	r = (x << 3) | (x >> 29); if (r != 0x91a2b3c0U) return 138;
	r = (x >> 29) | (x << 3); if (r != 0x91a2b3c0U) return 139;
	r = (x << 4) | (x >> 28); if (r != 0x23456781U) return 140;
	r = (x >> 28) | (x << 4); if (r != 0x23456781U) return 141;
	r = (x << 5) | (x >> 27); if (r != 0x468acf02U) return 142;
	r = (x >> 27) | (x << 5); if (r != 0x468acf02U) return 143;
	r = (x << 6) | (x >> 26); if (r != 0x8d159e04U) return 144;
	r = (x >> 26) | (x << 6); if (r != 0x8d159e04U) return 145;
	r = (x << 7) | (x >> 25); if (r != 0x1a2b3c09U) return 146;
	r = (x >> 25) | (x << 7); if (r != 0x1a2b3c09U) return 147;
	r = ((x & 0xFFFFFFFF) >> 25) | (x << 7); if (r != 0x1a2b3c09U) return 148;
	r = (x << 8) | (x >> 24); if (r != 0x34567812U) return 149;
	r = (x >> 24) | (x << 8); if (r != 0x34567812U) return 150;
	r = (x << 9) | (x >> 23); if (r != 0x68acf024U) return 151;
	r = (x >> 23) | (x << 9); if (r != 0x68acf024U) return 152;
	r = (x << 10) | (x >> 22); if (r != 0xd159e048U) return 153;
	r = (x >> 22) | (x << 10); if (r != 0xd159e048U) return 154;
	r = (x << 11) | (x >> 21); if (r != 0xa2b3c091U) return 155;
	r = (x >> 21) | (x << 11); if (r != 0xa2b3c091U) return 156;
	r = (x << 12) | (x >> 20); if (r != 0x45678123U) return 157;
	r = (x >> 20) | (x << 12); if (r != 0x45678123U) return 158;
	r = (x << 13) | (x >> 19); if (r != 0x8acf0246U) return 159;
	r = (x >> 19) | (x << 13); if (r != 0x8acf0246U) return 160;
	r = (x << 14) | (x >> 18); if (r != 0x159e048dU) return 161;
	r = (x >> 18) | (x << 14); if (r != 0x159e048dU) return 162;
	r = ((x & 0xFFFFFFFF) >> 18) | (x << 14); if (r != 0x159e048dU) return 163;
	r = (x << 15) | (x >> 17); if (r != 0x2b3c091aU) return 164;
	r = (x >> 17) | (x << 15); if (r != 0x2b3c091aU) return 165;
	r = (x << 16) | (x >> 16); if (r != 0x56781234U) return 166;
	r = (x >> 16) | (x << 16); if (r != 0x56781234U) return 167;
	r = (x << 17) | (x >> 15); if (r != 0xacf02468U) return 168;
	r = (x >> 15) | (x << 17); if (r != 0xacf02468U) return 169;
	r = (x << 18) | (x >> 14); if (r != 0x59e048d1U) return 170;
	r = (x >> 14) | (x << 18); if (r != 0x59e048d1U) return 171;
	r = (x << 19) | (x >> 13); if (r != 0xb3c091a2U) return 172;
	r = (x >> 13) | (x << 19); if (r != 0xb3c091a2U) return 173;
	r = (x << 20) | (x >> 12); if (r != 0x67812345U) return 174;
	r = (x >> 12) | (x << 20); if (r != 0x67812345U) return 175;
	r = (x << 21) | (x >> 11); if (r != 0xcf02468aU) return 176;
	r = (x >> 11) | (x << 21); if (r != 0xcf02468aU) return 177;
	r = ((x & 0xFFFFFFFF) >> 11) | (x << 21); if (r != 0xcf02468aU) return 178;
	r = (x << 22) | (x >> 10); if (r != 0x9e048d15U) return 179;
	r = (x >> 10) | (x << 22); if (r != 0x9e048d15U) return 180;
	r = (x << 23) | (x >> 9); if (r != 0x3c091a2bU) return 181;
	r = (x >> 9) | (x << 23); if (r != 0x3c091a2bU) return 182;
	r = (x << 24) | (x >> 8); if (r != 0x78123456U) return 183;
	r = (x >> 8) | (x << 24); if (r != 0x78123456U) return 184;
	r = (x << 25) | (x >> 7); if (r != 0xf02468acU) return 185;
	r = (x >> 7) | (x << 25); if (r != 0xf02468acU) return 186;
	r = (x << 26) | (x >> 6); if (r != 0xe048d159U) return 187;
	r = (x >> 6) | (x << 26); if (r != 0xe048d159U) return 188;
	r = (x << 27) | (x >> 5); if (r != 0xc091a2b3U) return 189;
	r = (x >> 5) | (x << 27); if (r != 0xc091a2b3U) return 190;
	r = (x << 28) | (x >> 4); if (r != 0x81234567U) return 191;
	r = (x >> 4) | (x << 28); if (r != 0x81234567U) return 192;
	r = ((x & 0xFFFFFFFF) >> 4) | (x << 28); if (r != 0x81234567U) return 193;
	r = (x << 29) | (x >> 3); if (r != 0x02468acfU) return 194;
	r = (x >> 3) | (x << 29); if (r != 0x02468acfU) return 195;
	r = (x << 30) | (x >> 2); if (r != 0x048d159eU) return 196;
	r = (x >> 2) | (x << 30); if (r != 0x048d159eU) return 197;
	r = (x << 31) | (x >> 1); if (r != 0x091a2b3cU) return 198;
	r = (x >> 1) | (x << 31); if (r != 0x091a2b3cU) return 199;
	return 0;
}

int tv3()
{
	unsigned r;

	x = 0x00010000U;
	r = (x << 1) | (x >> 31); if (r != 0x00020000U) return 200;
	r = (x >> 31) | (x << 1); if (r != 0x00020000U) return 201;
	r = (x << 2) | (x >> 30); if (r != 0x00040000U) return 202;
	r = (x >> 30) | (x << 2); if (r != 0x00040000U) return 203;
	r = (x << 3) | (x >> 29); if (r != 0x00080000U) return 204;
	r = (x >> 29) | (x << 3); if (r != 0x00080000U) return 205;
	r = (x << 4) | (x >> 28); if (r != 0x00100000U) return 206;
	r = (x >> 28) | (x << 4); if (r != 0x00100000U) return 207;
	r = (x << 5) | (x >> 27); if (r != 0x00200000U) return 208;
	r = (x >> 27) | (x << 5); if (r != 0x00200000U) return 209;
	r = (x << 6) | (x >> 26); if (r != 0x00400000U) return 210;
	r = (x >> 26) | (x << 6); if (r != 0x00400000U) return 211;
	r = (x << 7) | (x >> 25); if (r != 0x00800000U) return 212;
	r = (x >> 25) | (x << 7); if (r != 0x00800000U) return 213;
	r = ((x & 0xFFFFFFFF) >> 25) | (x << 7); if (r != 0x00800000U) return 214;
	r = (x << 8) | (x >> 24); if (r != 0x01000000U) return 215;
	r = (x >> 24) | (x << 8); if (r != 0x01000000U) return 216;
	r = (x << 9) | (x >> 23); if (r != 0x02000000U) return 217;
	r = (x >> 23) | (x << 9); if (r != 0x02000000U) return 218;
	r = (x << 10) | (x >> 22); if (r != 0x04000000U) return 219;
	r = (x >> 22) | (x << 10); if (r != 0x04000000U) return 220;
	r = (x << 11) | (x >> 21); if (r != 0x08000000U) return 221;
	r = (x >> 21) | (x << 11); if (r != 0x08000000U) return 222;
	r = (x << 12) | (x >> 20); if (r != 0x10000000U) return 223;
	r = (x >> 20) | (x << 12); if (r != 0x10000000U) return 224;
	r = (x << 13) | (x >> 19); if (r != 0x20000000U) return 225;
	r = (x >> 19) | (x << 13); if (r != 0x20000000U) return 226;
	r = (x << 14) | (x >> 18); if (r != 0x40000000U) return 227;
	r = (x >> 18) | (x << 14); if (r != 0x40000000U) return 228;
	r = ((x & 0xFFFFFFFF) >> 18) | (x << 14); if (r != 0x40000000U) return 229;
	r = (x << 15) | (x >> 17); if (r != 0x80000000U) return 230;
	r = (x >> 17) | (x << 15); if (r != 0x80000000U) return 231;
	r = (x << 16) | (x >> 16); if (r != 0x00000001U) return 232;
	r = (x >> 16) | (x << 16); if (r != 0x00000001U) return 233;
	r = (x << 17) | (x >> 15); if (r != 0x00000002U) return 234;
	r = (x >> 15) | (x << 17); if (r != 0x00000002U) return 235;
	r = (x << 18) | (x >> 14); if (r != 0x00000004U) return 236;
	r = (x >> 14) | (x << 18); if (r != 0x00000004U) return 237;
	r = (x << 19) | (x >> 13); if (r != 0x00000008U) return 238;
	r = (x >> 13) | (x << 19); if (r != 0x00000008U) return 239;
	r = (x << 20) | (x >> 12); if (r != 0x00000010U) return 240;
	r = (x >> 12) | (x << 20); if (r != 0x00000010U) return 241;
	r = (x << 21) | (x >> 11); if (r != 0x00000020U) return 242;
	r = (x >> 11) | (x << 21); if (r != 0x00000020U) return 243;
	r = ((x & 0xFFFFFFFF) >> 11) | (x << 21); if (r != 0x00000020U) return 244;
	r = (x << 22) | (x >> 10); if (r != 0x00000040U) return 245;
	r = (x >> 10) | (x << 22); if (r != 0x00000040U) return 246;
	r = (x << 23) | (x >> 9); if (r != 0x00000080U) return 247;
	r = (x >> 9) | (x << 23); if (r != 0x00000080U) return 248;
	r = (x << 24) | (x >> 8); if (r != 0x00000100U) return 249;
	r = (x >> 8) | (x << 24); if (r != 0x00000100U) return 250;
	r = (x << 25) | (x >> 7); if (r != 0x00000200U) return 251;
	r = (x >> 7) | (x << 25); if (r != 0x00000200U) return 252;
	r = (x << 26) | (x >> 6); if (r != 0x00000400U) return 253;
	r = (x >> 6) | (x << 26); if (r != 0x00000400U) return 254;
	r = (x << 27) | (x >> 5); if (r != 0x00000800U) return 255;
	r = (x >> 5) | (x << 27); if (r != 0x00000800U) return 256;
	r = (x << 28) | (x >> 4); if (r != 0x00001000U) return 257;
	r = (x >> 4) | (x << 28); if (r != 0x00001000U) return 258;
	r = ((x & 0xFFFFFFFF) >> 4) | (x << 28); if (r != 0x00001000U) return 259;
	r = (x << 29) | (x >> 3); if (r != 0x00002000U) return 260;
	r = (x >> 3) | (x << 29); if (r != 0x00002000U) return 261;
	r = (x << 30) | (x >> 2); if (r != 0x00004000U) return 262;
	r = (x >> 2) | (x << 30); if (r != 0x00004000U) return 263;
	r = (x << 31) | (x >> 1); if (r != 0x00008000U) return 264;
	r = (x >> 1) | (x << 31); if (r != 0x00008000U) return 265;
	return 0;
}

int tside()
{
	unsigned r;

	x = 0x12345678U; calls = 0;
	r = (next() << 5) | (next() >> 27); if (calls != 2 || r != 0x468acf02U) return 662;
	return 0;
}

int t16()
{
	unsigned r;

	s = 0x8001;
	r = (unsigned short)((s << 1) | (s >> 15)); if (r != 0x0003U) return 663;
	r = (unsigned short)((s << 3) | (s >> 13)); if (r != 0x000cU) return 664;
	r = (unsigned short)((s << 8) | (s >> 8)); if (r != 0x0180U) return 665;
	r = (unsigned short)((s << 13) | (s >> 3)); if (r != 0x3000U) return 666;
	r = (unsigned short)((s << 15) | (s >> 1)); if (r != 0xc000U) return 667;
	s = 0xabcd;
	r = (unsigned short)((s << 1) | (s >> 15)); if (r != 0x579bU) return 668;
	r = (unsigned short)((s << 3) | (s >> 13)); if (r != 0x5e6dU) return 669;
	r = (unsigned short)((s << 8) | (s >> 8)); if (r != 0xcdabU) return 670;
	r = (unsigned short)((s << 13) | (s >> 3)); if (r != 0xb579U) return 671;
	r = (unsigned short)((s << 15) | (s >> 1)); if (r != 0xd5e6U) return 672;
	s = 0x0001;
	r = (unsigned short)((s << 1) | (s >> 15)); if (r != 0x0002U) return 673;
	r = (unsigned short)((s << 3) | (s >> 13)); if (r != 0x0008U) return 674;
	r = (unsigned short)((s << 8) | (s >> 8)); if (r != 0x0100U) return 675;
	r = (unsigned short)((s << 13) | (s >> 3)); if (r != 0x2000U) return 676;
	r = (unsigned short)((s << 15) | (s >> 1)); if (r != 0x8000U) return 677;
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
