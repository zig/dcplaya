! Adapted from the version in Newlib's mach/sh dir.

	.text
	.align	2
	.globl	_setjmp
_setjmp:
	add	#(13*4),r4
	sts.l	pr,@-r4

	fmov.s	fr15,@-r4	! call saved floating point registers
	fmov.s	fr14,@-r4
	fmov.s	fr13,@-r4
	fmov.s	fr12,@-r4

	mov.l	r15,@-r4	! call saved integer registers
	mov.l	r14,@-r4
	mov.l	r13,@-r4
	mov.l	r12,@-r4

	mov.l	r11,@-r4
	mov.l	r10,@-r4
	mov.l	r9,@-r4
	mov.l	r8,@-r4

	rts
	mov    #0,r0

	.align	2
	.globl	_longjmp
_longjmp:
	mov.l	@r4+,r8
	mov.l	@r4+,r9
	mov.l	@r4+,r10
	mov.l	@r4+,r11

	mov.l	@r4+,r12
	mov.l	@r4+,r13
	mov.l	@r4+,r14
	mov.l	@r4+,r15

	fmov.s	@r4+,fr12	! call saved floating point registers
	fmov.s	@r4+,fr13
	fmov.s	@r4+,fr14
	fmov.s	@r4+,fr15

	lds.l	@r4+,pr

	mov	r5,r0
	tst	r0,r0
	bf	retr4
	movt	r0
retr4:	rts
	nop




! KallistiOS 1.1.5
!
!   arch/dreamcast/kernel/entry.s
!   (c)2000-2001 Dan Potter
!
! Assembler code for entry and exit to/from the kernel via exceptions
!

! Routine that all exception handlers jump to after setting
! an error code. Out of neccessity, all of this function is in
! assembler instead of C. Once the registers are saved, we will
! jump into a shared routine. This register save and restore code
! is mostly from my sh-stub code. For now this is pretty high overhead
! for a context switcher (or especially a timer! =) but it can
! be optimized later.

	.align		2
	.globl		_irq_srt_addr
	.globl		_irq_handle_exception
	.globl		_irq_save_regs
	.globl		_irq_force_return

! irq_force_return() jumps here; make sure we're in register
! bank 1 (as opposed to 0)
_irq_force_return2:
	mov.l	_irqfr_or,r1
	stc	sr,r0
	or	r1,r0
	ldc	r0,sr
	bra	_save_regs_finish2
	nop
	
	.align 2
_irqfr_or:
	.long	0x20000000


	

! Now restore all the registers and jump back to the thread
_save_regs_finish2:
	! we finish with an rte so put pr into spc
!	mov.l	_tmp+4,r1
!	sts	pr,r1
	mov.l	_longjmp_addr,r1
	ldc	r1,spc
	
	mov.l	_irq_srt_addr2, r0	! Get register store address
	mov.l	@r0, r1
	
	ldc.l	@r1+,r0_bank		! restore R0	(r1 is now _irq_srt_addr+0)
	ldc.l	@r1+,r1_bank		! restore R1
	ldc.l	@r1+,r2_bank		! restore R2
	ldc.l	@r1+,r3_bank		! restore R3
	ldc.l	@r1+,r4_bank		! restore R4
	ldc.l	@r1+,r5_bank		! restore R5
	ldc.l	@r1+,r6_bank		! restore R6
	ldc.l	@r1+,r7_bank		! restore R7
	add	#-32,r1			! Go back to the front
	mov.l	@(0x20,r1), r8		! restore R8	(r1 is now ...+0)
	mov.l	@(0x24,r1), r9		! restore R9
	mov.l	@(0x28,r1), r10		! restore R10
	mov.l	@(0x2c,r1), r11		! restore R11
	mov.l	@(0x30,r1), r12		! restore R12
	mov.l	@(0x34,r1), r13		! restore R13
	mov.l	@(0x38,r1), r14		! restore R14
	mov.l	@(0x3c,r1), r15		! restore program's stack
	
	add	#0x40,r1		! jump up to status words
	! VP :	 do not restore SPC !!
!	ldc.l	@r1+,spc		! restore SPC 0x40	(r1 is now +0x40)
	add	#4,r1
	lds.l	@r1+,pr			! restore PR  0x44	(+0x44)
!	add	#4,r1
	ldc.l	@r1+,gbr		! restore GBR 0x48	(+0x48)
!	ldc.l	@r1+,vbr		! restore VBR (don't play with VBR)
	add	#4,r1			!			(+0x4c)
	lds.l	@r1+,mach		! restore MACH 0x50	(+0x50)
	lds.l	@r1+,macl		! restore MACL 0x54	(+0x54)
	ldc.l	@r1+,ssr		! restore SSR  0x58	(+0x58)

	mov	#0,r2			! Set known FP flags	(+0x5c)
	lds	r2,fpscr
	frchg				! Second FP bank
	fmov.s	@r1+,fr0		! restore FR0  0x5c
	fmov.s	@r1+,fr1		! restore FR1
	fmov.s	@r1+,fr2		! restore FR2
	fmov.s	@r1+,fr3		! restore FR3
	fmov.s	@r1+,fr4		! restore FR4
	fmov.s	@r1+,fr5		! restore FR5
	fmov.s	@r1+,fr6		! restore FR6
	fmov.s	@r1+,fr7		! restore FR7
	fmov.s	@r1+,fr8		! restore FR8
	fmov.s	@r1+,fr9		! restore FR9
	fmov.s	@r1+,fr10		! restore FR10
	fmov.s	@r1+,fr11		! restore FR11
	fmov.s	@r1+,fr12		! restore FR12
	fmov.s	@r1+,fr13		! restore FR13
	fmov.s	@r1+,fr14		! restore FR14
	fmov.s	@r1+,fr15		! restore FR15 0x98
	frchg				! First FP bank
	fmov.s	@r1+,fr0		! restore FR0  0x9c
	fmov.s	@r1+,fr1		! restore FR1
	fmov.s	@r1+,fr2		! restore FR2
	fmov.s	@r1+,fr3		! restore FR3
	fmov.s	@r1+,fr4		! restore FR4
	fmov.s	@r1+,fr5		! restore FR5
	fmov.s	@r1+,fr6		! restore FR6
	fmov.s	@r1+,fr7		! restore FR7
	fmov.s	@r1+,fr8		! restore FR8
	fmov.s	@r1+,fr9		! restore FR9
	fmov.s	@r1+,fr10		! restore FR10
	fmov.s	@r1+,fr11		! restore FR11
	fmov.s	@r1+,fr12		! restore FR12
	fmov.s	@r1+,fr13		! restore FR13
	fmov.s	@r1+,fr14		! restore FR14
	fmov.s	@r1+,fr15		! restore FR15  0xd8
	lds.l	@r1+,fpscr		! restore FPSCR 0xdc
	lds.l	@r1+,fpul		! restore FPUL  0xe0

!	add	#-0x70,r1		! jump back to registers
!	add	#-0x34,r1
!	mov.l	@(0,r1),r0		! restore R0
!	mov.l	@(4,r1),r1		! restore R1
	mov	#2,r0

!	stc	ssr,r0
!	ldc	r0,sr
!	rts				! return


	ldc	r5,r5_bank
	ldc	r4,r4_bank
	
	rte				! return
	nop
	


	.align	2
_irq_srt_addr2:
	.long	_irq_srt_addr	! Save Regs Table -- this is an indirection
			! so we can easily swap out pointers during a
			! context switch.

_tmp:
	.long	0
	.long	0

_longjmp_addr:
	.long	_longjmp
	