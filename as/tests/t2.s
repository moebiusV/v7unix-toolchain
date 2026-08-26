.globl _start
_start:
	mov	$5+3,r0
	mov	r1,r2
	mov	(r3),r4
	mov	-(r5),r0
	mov	(r1)+,r2
	mov	*$100,r0
	mov	4(r2),r3
	mov	$tbl,r0
	add	$2,r0
	cmp	r0,r1
	beq	1f
	br	2f
1:	inc	r0
2:	dec	r1
	jsr	pc,_sub
	sys	0
	rts	pc
_sub:
	mov	r0,r1
	rts	pc
.data
tbl:	.byte	1,2,3,4
str:	<hello\0>
.comm	_buf,10
