/*
 * Copyright (c) 2016 Leonid Yegoshin
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

#include    "tinyVP.h"
#include    "mips.h"
#include    "uart.h"
#include    "tlb.h"
#include    "irq.h"
#include    "console.h"

#define     MAX_INPUT_BUFFER_SIZE   8

#define     BELL_CHAR   (007)
#define     CONSOLE_ESC (005)

unsigned int console_reg_sta[MAX_NUM_GUEST + 1];
unsigned int console_reg_mode[MAX_NUM_GUEST + 1];
unsigned int console_rx_vmid;
unsigned char console_esc;
unsigned char console_buffer[MAX_NUM_GUEST + 1][MAX_INPUT_BUFFER_SIZE];
unsigned char console_buffer_start[MAX_NUM_GUEST + 1];
unsigned char console_buffer_end[MAX_NUM_GUEST + 1];

void console_input_char(unsigned char value)
{
	short pos;

	pos = console_buffer_end[console_rx_vmid] + 1;
	if (pos >= MAX_INPUT_BUFFER_SIZE)
		pos = 0;
	if (console_buffer_start[console_rx_vmid] == pos) {
		printf("%c", BELL_CHAR);
		return;
	}
	console_buffer[console_rx_vmid][console_buffer_end[console_rx_vmid]] = value;
	console_buffer_end[console_rx_vmid] = pos;
}

inline unsigned char console_has_next_char(unsigned int vmid)
{
	return console_buffer_start[vmid] != console_buffer_end[vmid];
}

inline unsigned char console_next_char(unsigned int vmid)
{
	unsigned char c;

	c = console_buffer[vmid][console_buffer_start[vmid]];
	if ( ++console_buffer_start[vmid] >= MAX_INPUT_BUFFER_SIZE)
		console_buffer_start[vmid] = 0;
	return c;
}

extern emulator console;

unsigned int  console(struct exception_frame *exfr,
		     struct cpte_nd const *cpte,
		     unsigned long pagemask,
		     unsigned long addr, unsigned long ELo,
		     int flag, unsigned long value)
{
	unsigned long offset;
	unsigned int vmid = current->tid;
	unsigned int i;

	offset = ((unsigned long *)addr) - ((unsigned long *)console_paddr);
	if (flag > 0) {
		/* write to console */
		switch (offset) {
		case (&(console_uart->UxTXREG) - &(console_uart->UxMODE)):
			printf("%c", value);
			break;
		case (&(console_uart->UxSTA) - &(console_uart->UxMODE)):
			console_reg_sta[vmid] = value;
			break;
		case (&(console_uart->UxSTACLR) - &(console_uart->UxMODE)):
			console_reg_sta[vmid] &= ~value;
			break;
		case (&(console_uart->UxSTASET) - &(console_uart->UxMODE)):
			console_reg_sta[vmid] |= value;
			break;
		case (&(console_uart->UxSTAINV) - &(console_uart->UxMODE)):
			console_reg_sta[vmid] ^= value;
			break;
		case (&(console_uart->UxMODE) - &(console_uart->UxMODE)):
			console_reg_mode[vmid] = value;
			break;
		case (&(console_uart->UxMODECLR) - &(console_uart->UxMODE)):
			console_reg_mode[vmid] &= ~value;
			break;
		case (&(console_uart->UxMODESET) - &(console_uart->UxMODE)):
			console_reg_mode[vmid] |= value;
			break;
		case (&(console_uart->UxMODEINV) - &(console_uart->UxMODE)):
			console_reg_mode[vmid] ^= value;
			break;
		default:
			; /* ignore write */
		}
		if ((console_reg_sta[vmid] & UART_STA_TXENA) &&
		    (console_reg_mode[vmid] & UART_MODE_ENABLE)) {
			insert_sw_irq(current->tid, console_irq_tx);
			enforce_irq(exfr, current->tid, console_irq_tx);
		}
		return 1;
	};

	if (flag < 0) {
		/* read from console */
		switch (offset) {
		case (&(console_uart->UxRXREG) - &(console_uart->UxMODE)):
			value = console_next_char(vmid);
			load_by_instruction(exfr, value);
			if (!console_has_next_char(vmid))
				console_reg_sta[vmid] &= ~UART_STA_URXDA;
			return 1;
		case (&(console_uart->UxSTA) - &(console_uart->UxMODE)):
			value = console_reg_sta[vmid] & ~(UART_STA_UTXBF|UART_STA_URXDA);
			value |= UART_STA_TRMT;
			if (console_has_next_char(vmid))
				value |= UART_STA_URXDA;
			load_by_instruction(exfr, value);
			return 1;
		case (&(console_uart->UxMODE) - &(console_uart->UxMODE)):
			value = console_reg_mode[vmid];
			load_by_instruction(exfr, value);
			return 1;
		case (&(console_uart->UxSTACLR) - &(console_uart->UxMODE)):
		case (&(console_uart->UxSTASET) - &(console_uart->UxMODE)):
		case (&(console_uart->UxSTAINV) - &(console_uart->UxMODE)):
		case (&(console_uart->UxMODECLR) - &(console_uart->UxMODE)):
		case (&(console_uart->UxMODESET) - &(console_uart->UxMODE)):
		case (&(console_uart->UxMODEINV) - &(console_uart->UxMODE)):
			return 0;
		default:
			i = emulate_rw(exfr, addr, ELo);
			return i;
		}
	}
	return 0;
}

extern emulator_irq    console_irq;

unsigned int    console_irq(struct exception_frame *exfr, unsigned int irq,
			    unsigned int ipl)
{
	return 0;
}

/* character taken from console UART */
void    console_rx(struct exception_frame *exfr, unsigned int value)
{
	unsigned int vmid;

	if (console_esc) {
		console_esc = 0;
		vmid = value - '0';
		if ((vmid >= 0) || (vmid <= MAX_NUM_GUEST)) {
			if (vm_list[vmid] || !vmid)
				console_rx_vmid = vmid;
			return;
		}
		printf("%c", BELL_CHAR);
		return;
	} else {
		if ((value & 0x7f) == CONSOLE_ESC) {
			if (!console_esc) {
				console_esc = 1;
				return;
			}
		}
	}


	console_esc = 0;
	console_input_char(value);
	console_reg_sta[console_rx_vmid] |= UART_STA_URXDA;
	if (!console_rx_vmid) {
		struct thread *next;

		if (value == '\r')
			printf("%c", '\n');
		if ((value & 0x60) || (value == '\r') || (value == '\t') ||
		    (value == '\a') || (value == '\b'))
			printf("%c", value);

		next = get_thread_fp(0);
		reschedule_flag = 1;
		next->thread_flags |= THREAD_FLAGS_RUNNING;
		if (current->tid)
			request_switch_to_thread(exfr, 0);
		return;
	}
	/* enact and start IRQ in vm ... */
	insert_sw_irq(console_rx_vmid, console_irq_rx);
	enforce_irq(exfr, console_rx_vmid, console_irq_rx);
}

emulator_ic_write   console_ic;

unsigned int        console_ic(struct exception_frame *exfr, unsigned int irq)
{
	/* start IRQ in VM ... */
	if ((console_reg_sta[current->gid] & UART_STA_TXENA) &&
	    (console_reg_mode[current->gid] & UART_MODE_ENABLE)) {
		insert_sw_irq(current->tid, console_irq_tx);
		enforce_irq(exfr, current->tid, console_irq_tx);
	} else if (irq == console_irq_rx)
		enforce_irq(exfr, current->tid, console_irq_rx);
}
