#include    "mipsasm.h"
#include    <include/asm/pic32mz.h>

.section .text.init, "ax"
.type _board_init, @function
.type _board_init_end, @function
.globl  _board_init
.globl  _board_init_end

_board_init:
_board_init_end:
	// No clever color LEDs on board, so skip
	jr  $31
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
