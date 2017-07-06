/*
 * Copyright (c) 2017 Leonid Yegoshin
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include    "mipsasm.h"
#include    <asm/pic32mz.h>

.type _pic32mzef_unlock, @function
	.globl  _pic32mzef_unlock
_pic32mzef_unlock:

	.set    push
	.set    noreorder
	.set    noat

//
//  magic sequence to unlock PIC32MZEF critical board registers
//
	la      v1, SYSKEY
	li      a0, 0xAA996655
	li      a1, 0x556699AA
	sw      a0, 0(v1)
	sw      a1, 0(v1)
	jr      ra
	 nop


.type _pic32mzef_setup, @function
	.globl  _pic32mzef_setup
_pic32mzef_setup:

	la      v0, board_setup_tbl
1:
	lw      a0, 0(v0)
	beqz    a0, 2f
	 nop
	lw      a1, 4(v0)
	sw      a1, 0(a0)
	b       1b
	 addiu  v0, 8
2:
	jr.hb   ra
	 nop

.type _pic32mzef_lock, @function
	.globl  _pic32mzef_lock
_pic32mzef_lock:

	la      v1, SYSKEY
	li      a0, 0x33333333  // lock it back
	sw      a0, 0(v1)
	jr      ra
	 nop
