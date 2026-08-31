/ Unsigned 16-bit quotient.
/ pcc's pdp11 backend emits `jsr pc,udiv` for `unsigned / unsigned` with
/ r0 = dividend, r1 = divisor; result r0 = quotient.  V7's dmr-cc inlined `div`
/ instead, so V7 libc has no udiv — pcc needs it.
/ The `div` instruction is signed; zero-extending the dividend makes it work
/ for divisors < 32768.  A divisor >= 32768 yields a 0-or-1 quotient.

.globl	udiv
udiv:
	mov	r1,r3		/ r3 = divisor
	mov	r0,r1		/ r1 = dividend (low word)
	clr	r0		/ r0 = 0 (high word); 32-bit dividend = 0:r1
	tst	r3
	bmi	1f		/ divisor >= 32768 (signed-negative)
	div	r3,r0		/ (r0:r1)/r3 -> r0 = quotient, r1 = remainder
	rts	pc
1:
	cmp	r1,r3
	blo	2f		/ dividend < divisor -> quotient 0
	mov	$1,r0
	rts	pc
2:
	clr	r0
	rts	pc
