.globl	_foo
.globl	_glob
.text
_foo:
	mov	$1,r0
	rts	pc
.data
_glob:	1234
