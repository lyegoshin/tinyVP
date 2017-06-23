#include    "mipsasm.h"
#include    <include/asm/pic32mz.h>

.section .text.init, "ax"
.type _board_init, @function
.globl  _board_init

_board_init:
	li  $2, 7 << 8
	sw  $2, LATBCLR
	sw  $2, TRISBCLR
	li  $2, 1 << 9             // yellow LED2=RB9
	sw  $2, LATBSET
	// setup a console routing for 64pin SoC
	li  $2, 3
	sw  $2, U4RXR // = RD0
	li  $2, 2
	sw  $2, RPD4R // = U4TX
	jr  $31
	 nop

.type _board_init_end, @function
.globl  _board_init_end

_board_init_end:
	li  $2, 7 << 8
	sw  $2, LATBCLR
	li  $2, 1 << 8          // green LED2=RB8 only
	sw  $2, LATBSET
	jr  $31
	 nop

.type _nmi_exception, @function
.globl  _nmi_exception
_nmi_exception:
	// LED1-3
	li  $2, 7 << 8
	sw  $2, LATBCLR
	sw  $2, TRISBCLR
	// Light RGB
	li  $2, 7 << 8
	sw  $2, LATBSET // red+yellow+green
1:      b   1b
	 nop

.type _bev_exception, @function
.globl  _bev_exception
_bev_exception:
	// LED1-3
	li  $2, 7 << 8
	sw  $2, LATBCLR
	sw  $2, TRISBCLR
	// Light RGB
	li  $2, 1 << 10
	sw  $2, LATBSET // red only
1:      b   1b
	 nop

.section .rodata
	// UART4 as a console
.type console_uart, @object
.globl  console_uart
console_uart:
	.word   U4MODE
.type console_paddr, @object
.globl  console_paddr
console_paddr:
	.word   KPHYS(U4MODE)
.type console_irq_rx, @object
.globl  console_irq_rx
console_irq_rx:
	.short  PIC32_IRQ_U4RX
.type console_irq_tx, @object
.globl  console_irq_tx
console_irq_tx:
	.short  PIC32_IRQ_U4TX
