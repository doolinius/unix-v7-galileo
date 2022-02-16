// V7/x86 source code: see www.nordier.com/v7x86 for details.
// Copyright (c) 1999 Robert Nordier.  All rights reserved.

// These are runtime functions for pcc.

.noprefix
.code16

.globl lmul
lmul:
	push	bp
	mov	sp,bp
	cli
	movl	4(bp),eax
	imull	8(bp)
	movl	eax,4(bp)
	sti
	mov	4(bp),ax
	mov	6(bp),dx
	pop	bp
	ret

.globl lrem
lrem:
	push	bp
	mov	sp,bp
	cli
	xorl	edx,edx
	movl	4(bp),eax
	idivl	8(bp)
	movl	edx,4(bp)
	sti
	mov	4(bp),ax
	mov	6(bp),dx
	pop	bp
	ret

.globl	aldiv
aldiv:
	push	bp
	mov	sp,bp
	mov	4(bp),bx
	cli
	xorl	edx,edx
	movl	(bx),eax
	idivl	6(bp)
	movl	eax,(bx)
	sti
	mov	(bx),ax
	mov	2(bx),dx
	pop	bp
	ret
