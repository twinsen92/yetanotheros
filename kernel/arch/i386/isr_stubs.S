/* isr_stubs.S - interrupt handler stubs, which redirect to generic_interrupt_handler() */
#include <arch/selectors.h>

.text
.global isr_entry
.func isr_entry
isr_entry:
	/* Save caller's registers. */
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
	pushal

	/* Set up kernel environment. */
	cld		/* String instructions go upward. */
	mov		$KERNEL_DATA_SELECTOR, %eax	/* Initialize segment registers. */
	mov		%eax, %ds
	mov 	%eax, %es
	leal 	56(%esp), %ebp	/* Set up frame pointer. */

	/* Call the generic interrupt handler. */
	pushl	%esp
	call	generic_interrupt_handler
	addl	$4, %esp
.endfunc

.global isr_exit
.func isr_exit
isr_exit:
	/* Restore caller's registers. */
	popal
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds

	/* Discard vec_no, error_code, frame_pointer members. */
	addl	$12, %esp

	/* Return to caller. */
	iret
.endfunc

/* This implements steps 1 and 2, described above, in the common case where we just push a 0 error
   code. */
#define ZERO		\
	pushl %ebp;		\
	pushl $0

/* This implements steps 1 and 2, described above, in the case where the CPU already pushed an error
   code. */
#define REAL			\
	pushl (%esp);		\
	movl %ebp, 4(%esp)

/* Emits a stub for interrupt vector NUMBER. TYPE is ZERO, for the case where we push a 0 error
   code, or REAL, if the CPU pushes an error code for us. */
#define STUB(NUMBER, TYPE)		\
	.text;						\
	.func isr##NUMBER##_stub;	\
	isr##NUMBER##_stub:			\
		TYPE;					\
		push $0x##NUMBER;		\
		jmp isr_entry;			\
	.endfunc;					\
								\
	.data;						\
	.long isr##NUMBER##_stub;

.data
.global isr_stubs
isr_stubs:
	/* All the stubs. */
	STUB(00, ZERO) STUB(01, ZERO) STUB(02, ZERO) STUB(03, ZERO)
	STUB(04, ZERO) STUB(05, ZERO) STUB(06, ZERO) STUB(07, ZERO)
	STUB(08, REAL) STUB(09, ZERO) STUB(0a, REAL) STUB(0b, REAL)
	STUB(0c, ZERO) STUB(0d, REAL) STUB(0e, REAL) STUB(0f, ZERO)

	STUB(10, ZERO) STUB(11, REAL) STUB(12, ZERO) STUB(13, ZERO)
	STUB(14, ZERO) STUB(15, ZERO) STUB(16, ZERO) STUB(17, ZERO)
	STUB(18, REAL) STUB(19, ZERO) STUB(1a, REAL) STUB(1b, REAL)
	STUB(1c, ZERO) STUB(1d, REAL) STUB(1e, REAL) STUB(1f, ZERO)

	STUB(20, ZERO) STUB(21, ZERO) STUB(22, ZERO) STUB(23, ZERO)
	STUB(24, ZERO) STUB(25, ZERO) STUB(26, ZERO) STUB(27, ZERO)
	STUB(28, ZERO) STUB(29, ZERO) STUB(2a, ZERO) STUB(2b, ZERO)
	STUB(2c, ZERO) STUB(2d, ZERO) STUB(2e, ZERO) STUB(2f, ZERO)

	STUB(30, ZERO) STUB(31, ZERO) STUB(32, ZERO) STUB(33, ZERO)
	STUB(34, ZERO) STUB(35, ZERO) STUB(36, ZERO) STUB(37, ZERO)
	STUB(38, ZERO) STUB(39, ZERO) STUB(3a, ZERO) STUB(3b, ZERO)
	STUB(3c, ZERO) STUB(3d, ZERO) STUB(3e, ZERO) STUB(3f, ZERO)

	STUB(40, ZERO) STUB(41, ZERO) STUB(42, ZERO) STUB(43, ZERO)
	STUB(44, ZERO) STUB(45, ZERO) STUB(46, ZERO) STUB(47, ZERO)
	STUB(48, ZERO) STUB(49, ZERO) STUB(4a, ZERO) STUB(4b, ZERO)
	STUB(4c, ZERO) STUB(4d, ZERO) STUB(4e, ZERO) STUB(4f, ZERO)

	STUB(50, ZERO) STUB(51, ZERO) STUB(52, ZERO) STUB(53, ZERO)
	STUB(54, ZERO) STUB(55, ZERO) STUB(56, ZERO) STUB(57, ZERO)
	STUB(58, ZERO) STUB(59, ZERO) STUB(5a, ZERO) STUB(5b, ZERO)
	STUB(5c, ZERO) STUB(5d, ZERO) STUB(5e, ZERO) STUB(5f, ZERO)

	STUB(60, ZERO) STUB(61, ZERO) STUB(62, ZERO) STUB(63, ZERO)
	STUB(64, ZERO) STUB(65, ZERO) STUB(66, ZERO) STUB(67, ZERO)
	STUB(68, ZERO) STUB(69, ZERO) STUB(6a, ZERO) STUB(6b, ZERO)
	STUB(6c, ZERO) STUB(6d, ZERO) STUB(6e, ZERO) STUB(6f, ZERO)

	STUB(70, ZERO) STUB(71, ZERO) STUB(72, ZERO) STUB(73, ZERO)
	STUB(74, ZERO) STUB(75, ZERO) STUB(76, ZERO) STUB(77, ZERO)
	STUB(78, ZERO) STUB(79, ZERO) STUB(7a, ZERO) STUB(7b, ZERO)
	STUB(7c, ZERO) STUB(7d, ZERO) STUB(7e, ZERO) STUB(7f, ZERO)

	STUB(80, ZERO) STUB(81, ZERO) STUB(82, ZERO) STUB(83, ZERO)
	STUB(84, ZERO) STUB(85, ZERO) STUB(86, ZERO) STUB(87, ZERO)
	STUB(88, ZERO) STUB(89, ZERO) STUB(8a, ZERO) STUB(8b, ZERO)
	STUB(8c, ZERO) STUB(8d, ZERO) STUB(8e, ZERO) STUB(8f, ZERO)

	STUB(90, ZERO) STUB(91, ZERO) STUB(92, ZERO) STUB(93, ZERO)
	STUB(94, ZERO) STUB(95, ZERO) STUB(96, ZERO) STUB(97, ZERO)
	STUB(98, ZERO) STUB(99, ZERO) STUB(9a, ZERO) STUB(9b, ZERO)
	STUB(9c, ZERO) STUB(9d, ZERO) STUB(9e, ZERO) STUB(9f, ZERO)

	STUB(a0, ZERO) STUB(a1, ZERO) STUB(a2, ZERO) STUB(a3, ZERO)
	STUB(a4, ZERO) STUB(a5, ZERO) STUB(a6, ZERO) STUB(a7, ZERO)
	STUB(a8, ZERO) STUB(a9, ZERO) STUB(aa, ZERO) STUB(ab, ZERO)
	STUB(ac, ZERO) STUB(ad, ZERO) STUB(ae, ZERO) STUB(af, ZERO)

	STUB(b0, ZERO) STUB(b1, ZERO) STUB(b2, ZERO) STUB(b3, ZERO)
	STUB(b4, ZERO) STUB(b5, ZERO) STUB(b6, ZERO) STUB(b7, ZERO)
	STUB(b8, ZERO) STUB(b9, ZERO) STUB(ba, ZERO) STUB(bb, ZERO)
	STUB(bc, ZERO) STUB(bd, ZERO) STUB(be, ZERO) STUB(bf, ZERO)

	STUB(c0, ZERO) STUB(c1, ZERO) STUB(c2, ZERO) STUB(c3, ZERO)
	STUB(c4, ZERO) STUB(c5, ZERO) STUB(c6, ZERO) STUB(c7, ZERO)
	STUB(c8, ZERO) STUB(c9, ZERO) STUB(ca, ZERO) STUB(cb, ZERO)
	STUB(cc, ZERO) STUB(cd, ZERO) STUB(ce, ZERO) STUB(cf, ZERO)

	STUB(d0, ZERO) STUB(d1, ZERO) STUB(d2, ZERO) STUB(d3, ZERO)
	STUB(d4, ZERO) STUB(d5, ZERO) STUB(d6, ZERO) STUB(d7, ZERO)
	STUB(d8, ZERO) STUB(d9, ZERO) STUB(da, ZERO) STUB(db, ZERO)
	STUB(dc, ZERO) STUB(dd, ZERO) STUB(de, ZERO) STUB(df, ZERO)

	STUB(e0, ZERO) STUB(e1, ZERO) STUB(e2, ZERO) STUB(e3, ZERO)
	STUB(e4, ZERO) STUB(e5, ZERO) STUB(e6, ZERO) STUB(e7, ZERO)
	STUB(e8, ZERO) STUB(e9, ZERO) STUB(ea, ZERO) STUB(eb, ZERO)
	STUB(ec, ZERO) STUB(ed, ZERO) STUB(ee, ZERO) STUB(ef, ZERO)

	STUB(f0, ZERO) STUB(f1, ZERO) STUB(f2, ZERO) STUB(f3, ZERO)
	STUB(f4, ZERO) STUB(f5, ZERO) STUB(f6, ZERO) STUB(f7, ZERO)
	STUB(f8, ZERO) STUB(f9, ZERO) STUB(fa, ZERO) STUB(fb, ZERO)
	STUB(fc, ZERO) STUB(fd, ZERO) STUB(fe, ZERO) STUB(ff, ZERO)
