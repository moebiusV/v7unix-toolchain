/* expect: 1 -- AAPCS64 structs, the pcc side: pcc calls clang's functions and clang calls pcc's */
#define PFX p_
#define OFX c_
#include "abi.h"
int c_check(void);
int test(void)
{
	int r;

	r = p_check();
	if (r != 1)
		return r + 1000;
	r = c_check();
	if (r != 1)
		return r + 2000;
	return 1;
}
