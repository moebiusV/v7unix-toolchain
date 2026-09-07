/ Unsigned 16-bit remainder.
/ r0 = dividend, r1 = divisor; result r0 = remainder.

.globl	urem
urem:
	mov	r1,r3		/ r3 = divisor
	mov	r0,r1		/ r1 = dividend (low word)
	clr	r0		/ r0 = 0 (high word)
	tst	r3
	bmi	1f		/ divisor >= 32768
	div	r3,r0		/ r0 = quotient, r1 = remainder
	mov	r1,r0
	rts	pc
1:
	cmp	r1,r3
	blo	2f		/ dividend < divisor -> remainder = dividend
	sub	r3,r1		/ remainder = dividend - divisor
2:
	mov	r1,r0
	rts	pc
