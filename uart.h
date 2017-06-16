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

#ifndef _UART_H
#define _UART_H

struct uart_device {
	volatile unsigned int    UxMODE;
	volatile unsigned int    UxMODECLR;
	volatile unsigned int    UxMODESET;
	volatile unsigned int    UxMODEINV;
	volatile unsigned int    UxSTA;
	volatile unsigned int    UxSTACLR;
	volatile unsigned int    UxSTASET;
	volatile unsigned int    UxSTAINV;
	volatile unsigned int    UxTXREG;
	volatile unsigned int    UxTXREG_unused1;
	volatile unsigned int    UxTXREG_unused2;
	volatile unsigned int    UxTXREG_unused3;
	volatile unsigned int    UxRXREG;
	volatile unsigned int    UxRXREG_unused1;
	volatile unsigned int    UxRXREG_unused2;
	volatile unsigned int    UxRXREG_unused3;
	volatile unsigned int    UxBRG;
	volatile unsigned int    UxBRGCLR;
	volatile unsigned int    UxBRGSET;
	volatile unsigned int    UxBRGINV;
};

#define UART_BRG_CLOCK_RATIO_115209     53

#define UART_MODE_ENABLE                0x8000
#define UART_MODE_WAKEUP_ON_START       0x0080
#define UART_MODE_STSEL_2STOPBIT        0x0001

#define UART_STA_TXMODE_TXBEMPTY        0x4000
#define UART_STA_RXENA                  0x1000
#define UART_STA_TXENA                  0x0400
#define UART_STA_UTXBF                  0x0200
#define UART_STA_TRMT                   0x0100
#define UART_STA_URXDA                  0x0001

/* console */
#define _UART1_IRQ_RX                   113
#define _UART1_IRQ_TX                   114
#define _UART2_IRQ_RX                   146
#define _UART2_IRQ_TX                   147
#define _UART3_IRQ_RX                   158
#define _UART3_IRQ_TX                   159
#define _UART4_IRQ_RX                   171
#define _UART4_IRQ_TX                   172
#define _UART5_IRQ_RX                   180
#define _UART5_IRQ_TX                   181
#define _UART6_IRQ_RX                   189
#define _UART6_IRQ_TX                   190
//#define _UART4ADDR                      0xbf822600
//#define _UART4PADDR                     0x1f822600

int uart_init(struct uart_device *const uad);
int uart_writeline(struct uart_device *const uad, unsigned char *l);
int uart_write(struct uart_device *const uad, unsigned char *l, unsigned len);
unsigned int uart_readchar(struct uart_device *const uad);

extern struct uart_device *const console_uart;
extern unsigned long const console_paddr;

#endif
