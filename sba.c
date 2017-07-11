/*
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 *
 *
 * Copyright (C) 2017 Leonid Yegoshin
 *
 *  System Bus Arbitration interrupt for access violations
 *
 */

#include    "mips.h"
#include    "uart.h"
#include    <asm/pic32mz.h>

#define     _SBA_PRIO       0x1F

static char const * const sb_id[] = {
    "id 0",
    "CPU LRS",
    "CPU HIGH",
    "DMA Read LRS",
    "DMA Read HIGH",
    "DMA Write LRS",
    "DMA Write HIGH",
    "USB",
    "Ethernet Read",
    "Ethernet Write",
    "CAN1",
    "CAN2",
    "SQI1",
    "Flash",
    "Crypto",
    "ID15",
};

static char const * const sb_target[] = {
    "System Bus registers",
    "Flash or Prefetch registers",
    "RAM bank 0-256KB",
    "RAM bank 256KB-512KB",
    "EBI memory",
    "P.Set 1 registers",
    "P.Set 2 (SPI,I2C,UART,PMP)",
    "P.Set 3 (Timer,IC/OC/ADC,Comparator)",
    "P.Set 4 (PORTA-PORTK)",
    "P.Set 5 (CAN,Ethernet)",
    "USB",
    "SQI memory",
    "Crypto registers",
    "Random registers",
};

int sba_irq()
{
	int i;
	unsigned sbflag;
	unsigned elog1;
	unsigned elog2;
	unsigned q;
	char str[128];

	sprintf(str, "SBA: access violation, SBFLAG=%08x:\n",sbflag = *(volatile unsigned *)SBFLAG);
	uart_writeline(console_uart, str);

	for (i=0; i<14; i++) {
		if (sbflag & (1 << i)) {
			elog1 = *(volatile unsigned *)SBTxELOG1(i);
			elog2 = *(volatile unsigned *)SBTxELOG2(i);
			sprintf(str, "SBA: %s <-- %s, SBTxELOG1=%08x SBTxELOG2=%x\n",
				sb_target[i],(sb_id[elog1 & 0xf]), elog1, elog2);
			uart_writeline(console_uart, str);
			*(volatile unsigned *)SBTxELOG1(i) = (-1);
			q = *(volatile unsigned *)SBTxECLRM(i);   // clear multiple errors
		}
	}
	irq_ack(PIC32_IRQ_SB);
}

void init_SBA()
{
	int i;
	unsigned q;

	for (i=0; i<14; i++) {
		*(volatile unsigned *)SBTxELOG1(i) = (-1);
		q = *(volatile unsigned *)SBTxECLRM(i);   // clear multiple errors
		*(volatile unsigned *)SBTxECON(i) = 0x01000000; // enable interrupt
	}

	irq_set_prio_and_unmask(PIC32_IRQ_SB, _SBA_PRIO);
}
