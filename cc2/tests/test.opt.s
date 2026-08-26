.comm	_array,24
.comm	_g,2
.globl	_sum
.text
_sum:
~~sum:
jsr	r5,csv
~a=4
~b=6
~c=10
sub	$4,sp
~i=177770
~t=177766
clr	-12(r5)
clr	-10(r5)
L20001:mov	-10(r5),r1
asl	r1
mov	_array(r1),r1
mul	4(r5),r1
add	6(r5),r1
sub	10(r5),r1
add	r1,-12(r5)
inc	-10(r5)
cmp	$12,-10(r5)
jgt	L20001
bit	$1,-12(r5)
jeq	L7
mov	-12(r5),r0
neg	r0
mov	r0,-12(r5)
L7:mov	-12(r5),_g
mov	-12(r5),r0
jmp	cret
.globl
.data
