/* expect: 1 -- structs through "...": pcc and clang as caller and as callee */
#define PFX p_
#define OFX c_
#include "abiva.h"
/* the key strings are made here and handed over: the harness cannot place clang's data references */
int c_check(char **);
int test(void)
{
	char *K[9];
	int r;

	K[0] = "8"; K[1] = "c"; K[2] = "g"; K[3] = "x"; K[4] = "8cgx"; K[5] = "i8cgx";
	K[6] = "iiiiii8gx"; K[7] = "iiiiiii8l"; K[8] = "gggg";
	r = p_check(K);
	if (r != 1)
		return r + 1000;
	r = c_check(K);
	return r == 1 ? 1 : r + 2000;
}
