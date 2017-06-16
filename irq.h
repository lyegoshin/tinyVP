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

#ifndef _IRQ_H
#define _IRQ_H

#include    "mips.h"
#include    "uart.h"
#include    "time.h"

void    irq_mask(unsigned irq);
void    irq_unmask(unsigned irq);
void    irq_ack(unsigned irq);
void    irq_mask_and_ack(unsigned irq);

void    do_IRQ(struct exception_frame *exfr);
void    init_IRQ(void);

typedef unsigned int (emulator_irq)(struct exception_frame *exfr, unsigned int irq,
				     unsigned int ipl);
typedef unsigned int (emulator_ic_write)(struct exception_frame *exfr,
					  unsigned int irq);

extern unsigned char const irq2vm[];
extern emulator_irq * const irq2emulator[];
extern emulator_ic_write * const irq2emulator_ic_write[];

static inline unsigned int get_irq2vm(unsigned int irq)
{
	return irq2vm[irq];
}

static inline emulator_irq * get_irq2emulator(unsigned int irq)
{
	return irq2emulator[irq];
}

static inline unsigned int emulate_irq(struct exception_frame *exfr, emulator_irq emulator,
				unsigned int irq, unsigned int ipl)
{
	return emulator(exfr, irq, ipl);
}

static inline emulator_ic_write * get_irq2emulator_ic_write(unsigned int irq)
{
	return irq2emulator_ic_write[irq];
}

static inline unsigned int emulate_ic_write(struct exception_frame *exfr,
				     emulator_ic_write emulator,
				     unsigned int irq)
{
	return emulator(exfr, irq);
}

#endif
