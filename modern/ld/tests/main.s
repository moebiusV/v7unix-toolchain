.globl	_main
.globl	_foo
.globl	_glob
_main:
	jsr	pc,_foo
	mov	_glob,r0
	add	$2,r0
	sys	0
	rts	pc
.comm	_buf,20
