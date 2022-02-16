// V7/x86 source code: see www.nordier.com/v7x86 for details.
// Copyright (c) 1999 Robert Nordier.  All rights reserved.

.noprefix
.code16

.globl	_main, _edata, _end

.globl	_buf1, _buf2, _buf3, _ind1, _ind2
.set	_buf1,0x7600
.set	_buf2,0x7800
.set	_buf3,0x7a00
.set	_ind1,0x7c00
.set	_ind2,0x7e00

startx:
	cld
	mov	cs,ax
	mov	ax,es
	mov	ax,ds
	mov	ax,ss
	mov	$0x7000,sp
	call	_main
1:
	jmp	1b

.globl	_putchar
_putchar:
	push	bp
	mov	sp,bp
	mov	4(bp),al
	cmp	$0xa,al
	jne	1f
	mov	$0xd,al
	mov	$0x7,bx
	mov	$0xe,ah
	int	$0x10
	mov	$0xa,al
1:
	mov	$0x7,bx
	mov	$0xe,ah
	int	$0x10
	xor	ah,ah
	pop	bp
	ret

.globl	_getchar
_getchar:
	xor	ah,ah
	int	$0x16
	cmp	$0xd,al
	jne	1f
	mov	$0xa,al
1:
	cbtw
	ret

.globl	_ddread
_ddread:
	push	bp
	mov	sp,bp
	push	es
	push	si
	push	di
	mov	4(bp),dl
	mov	6(bp),si
	call	read
	mov	$0,ax
	sbb	ax,ax
	pop	di
	pop	si
	pop	es
	pop	bp
	ret

read:
	test	dl,dl
	jns	read1
	mov	$0x55aa,bx	// try to use bios extensions
	push	dx
	mov	$0x41,ah
	int	$0x13
	pop	dx
	jc	read1
	cmp	$0xaa55,bx
	jne	read1
	test	$0x1,cl
	jz	read1
	mov	$0x42,ah
	int	$0x13
	ret

read1:
	push	dx
	mov	$0x8,ah
	int	$0x13
	jc	2f
	and	$0x3f,cl
	jz	2f
	mov	dh,ch
	mov	0x8(si),ax	// lba
	mov	0xa(si),dx
	xor	bh,bh
	mov	cl,bl
	call	div
	xchg	ch,bl
	inc	bx
	call	div
	test	dx,dx
	jnz	2f
	cmp	$0x3ff,ax
	ja	2f
	xchg	ah,al
	ror	$0x2,al
	or	ch,al
	inc	ax
	xchg	ax,cx
	pop	dx
	mov	bl,dh
	mov	$5,di
1:
	les	0x4(si),bx	// buf
	mov	0x2(si),al	// cnt
	mov	$0x2,ah
	int	$0x13
	jnc	4f
	dec	di
	jz	3f
	xor	ah,ah
	int	$0x13
	jmp	1b

2:
	pop	dx
3:
	stc
4:
	ret

// divide dx:ax by bx, leaving the remainder in bx.
div:
	push	cx
	xor	cx,cx
	xchg	ax,cx
	xchg	ax,dx
	div	bx
	xchg	ax,cx
	div	bx
	xchg	dx,cx
	mov	cx,bx
	pop	cx
	ret

.globl _dcopy
_dcopy:
	push	bp
	mov	sp,bp
	push	si
	push	di
	push	es
	mov	4(bp),si
	mov	6(bp),ax
	mov	8(bp),dx
	mov	10(bp),cx
	mov	ax,di
	and	$0xf,di
	shrd	$4,dx,ax
	mov	ax,es
	rep
	movsb
	pop	es
	pop	di
	pop	si
	pop	bp
	ret

.globl _exec
_exec:
	push	bp
	mov	sp,bp
	mov	4(bp),ax
	mov	6(bp),dx
	mov	ax,cx
	and	$0xf,cx
	shrd	$4,dx,ax
	push	ax
	push	cx
	xor	cx,cx
	mov	8(bp),dx
	lret
	pop	bp
	ret
