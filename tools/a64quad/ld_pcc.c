/* expect: 1 -- long double (binary128) between pcc and clang: values, structs, the stack, "..." */
#define PFX p_
#define OFX c_
#include "ldabi.h"
int c_check(void);
int test(void)
{
	int r = p_check();

	if (r != 1)
		return r + 1000;
	r = c_check();
	return r == 1 ? 1 : r + 2000;
}
