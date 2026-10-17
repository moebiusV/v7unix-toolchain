	.file "<command line>"
	.section .text
	.align 4
	.globl main
	.type main,@function
main:
	pushq %rbp
	movq %rsp,%rbp
	subq $16,%rsp
.L279:
.L283:
	movl $42,%esi
	movabsq $.L285,%rdi
	xorl %eax,%eax
	call printf
	movl $0,-4(%rbp)
	jmp .L281
.L281:
	movl -4(%rbp),%eax
	leave
	ret
	.size main,.-main
	.section .rodata
.L285:
	.ascii "hello %d\012\0"
	.ident "PCC: Portable C Compiler 1.2.0.DEVEL 20231021 for x86_64-pc-linux-gnu"
	.end
