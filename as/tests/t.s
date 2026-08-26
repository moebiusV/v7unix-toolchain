.globl _main
_main:
	mov	$1,r0
	sys	1
	rts	pc
.data
.globl _data
_data:	.byte	5
