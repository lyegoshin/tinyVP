#include    "mipsasm.h"
#include    <include/asm/pic32mz.h>

.section .text.init, "ax"
.type _board_init, @function
.globl  _board_init

_board_init:
	// LED1-3
	li  $2, 7
	sw  $2, LATHCLR
	sw  $2, TRISHCLR
	sw  $2, ANSELHCLR
	// Light yellow LED
	li  $2, 2
	sw  $2, LATHSET // yellow only

	// setup a console routing for 144pin SoC

	// UART2 - through onboard UART2USB chip. Doesn't work properly
	//      until configured via USB with Microchip tool,
	//      so it is excluded in favor of UART1.
	//li  $2, 0x4000
	//sw  $2, TRISBCLR  // 0xBF860114  // TRISB.14 = 0
	//sw  $2, ANSELBCLR // 0xBF860104  // ANSELB.14 = 0
	//li  $2, 0x40
	//sw  $2, ANSELGCLR // 0xBF860604  // ANSELG.6 = 0
	//li  $2, 1
	//sw  $2, U2RXR     // 0xBF801470  // U2RXR = RG6
	//li  $2, 2
	//sw  $2, RPB14R    // 0xBF801578  // RPB14R = U2TX

	// UART1 - pins 4 and 6 in PICtail connector of MEBII
	//          (Microchip Extension Board II)
	li  $2, 13
	sw  $2, U1RXR       // 0xBF801468      // U1RXR = RA14
	li  $2, 1
	sw  $2, RPA15R      // 0xBF80153C      // RPA15R = U1TX

	jr  $31
	 nop

.type _board_init_end, @function
.globl  _board_init_end
_board_init_end:
	li  $2, 7
	sw  $2, LATHCLR
	li  $2, 4
	sw  $2, LATHSET // green only
	jr  $31
	 nop

.type _nmi_exception, @function
.globl  _nmi_exception
_nmi_exception:
	// LED1-3
	li  $2, 7
	sw  $2, LATHCLR
	sw  $2, TRISHCLR
	sw  $2, ANSELHCLR
	// Light RGB
	li  $2, 7
	sw  $2, LATHSET // red+yellow+green
1:      b   1b
	 nop

.type _bev_exception, @function
.globl  _bev_exception
_bev_exception:
	// LED1-3
	li  $2, 7
	sw  $2, LATHCLR
	sw  $2, TRISHCLR
	sw  $2, ANSELHCLR
	// Light RED
	li  $2, 7
	sw  $2, LATHCLR
	li  $2, 1
	sw  $2, LATHSET // red only
1:      b   1b
	 nop


.section .rodata
	// UART2 as a console
	// UART1 as a console
.type console_uart, @object
.globl  console_uart
console_uart:
//        .word   U2MODE
	.word   U1MODE
.type console_paddr, @object
.globl  console_paddr
console_paddr:
//        .word   KPHYS(U2MODE)
	.word   KPHYS(U1MODE)
.type console_irq_rx, @object
.globl  console_irq_rx
console_irq_rx:
//        .short  PIC32_IRQ_U2RX
	.short  PIC32_IRQ_U1RX
.type console_irq_tx, @object
.globl  console_irq_tx
console_irq_tx:
//        .short  PIC32_IRQ_U2TX
	.short  PIC32_IRQ_U1TX
