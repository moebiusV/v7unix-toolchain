/* expect: 1 -- varargs: integers and doubles, in registers and on the stack */
typedef __builtin_va_list va_list;
#define va_start(v,l) __builtin_va_start(v,l)
#define va_arg(v,t) __builtin_va_arg(v,t)
#define va_end(v) __builtin_va_end(v)
long sum(int n, ...)
{
	va_list ap;
	long s = 0;
	va_start(ap, n);
	while (n-- > 0)
		s += va_arg(ap, long);
	va_end(ap);
	return s;
}
double fsum(int n, ...)
{
	va_list ap;
	double s = 0;
	va_start(ap, n);
	while (n-- > 0)
		s += va_arg(ap, double);
	va_end(ap);
	return s;
}
int test(void)
{
	if (sum(3, 1L, 2L, 3L) != 6) return 0;
	if (sum(10, 1L,2L,3L,4L,5L,6L,7L,8L,9L,10L) != 55) return 0;
	if (fsum(2, 1.5, 2.5) != 4.0) return 0;
	if (fsum(10, 1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,10.0) != 55.0) return 0;
	return 1;
}
