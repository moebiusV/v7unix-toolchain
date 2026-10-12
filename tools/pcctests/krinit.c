/* expect: 153 -- K&R initializers without "=": scalars, negatives, strings, arrays, structs */
struct tc { int op; int type; int val; };
int	isn	1;
int	peeksym	-1;
int	x = 5, y 6;
char	mov[]	"mov";
int	arr[]	{ 1, 2, 3 };
struct tc czero { 7, 8, 0 };
struct tc cone  { 7, 8, 1 };
char	buf[16 - 1];
int	sz = 3 - 1;
struct tc plain = { 1, 2, 3 };
int test(void)
{
	return isn + peeksym + x + y + mov[1] + arr[2] + czero.op + cone.val + sizeof buf + sz + plain.val;
}
