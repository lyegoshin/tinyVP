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

#ifndef _IC_H
#define _IC_H

#include    "mipsasm.h"
#include    <include/asm/pic32mz.h>
//#include    "thread.h"

// CLR,SET,INV offsets in words
#define     IC_CLR_WOFFSET          1
#define     IC_SET_WOFFSET          2
#define     IC_INV_WOFFSET          3
// CLR,SET,INV offsets in bytes
#define     IC_CLR_OFFSET           4
#define     IC_SET_OFFSET           8
#define     IC_INV_OFFSET           12
// WRT,CLR,SET,INV address mask
#define     IC_CSI_MASK             0xf

#define     IC_BASE                 KPHYS(INTCON) // 0x1F810000
#define     IC_PTR                  KSEG1(IC_BASE)
#define     IC_IFS_BASE_OFFSET      (0x40)
#define     IC_IFS_BASE             (IC_BASE + 0x40)
#define     IC_IFS                  KSEG1(IC_IFS_BASE)
#define     IC_IEC_BASE_OFFSET      (0xC0)
#define     IC_IEC_BASE             (IC_BASE + 0xC0)
#define     IC_IEC                  KSEG1(IC_IEC_BASE)
#define     IC_IPC_BASE_OFFSET      (0x140)
#define     IC_IPC_BASE             (IC_BASE + 0x140)
#define     IC_IPC                  KSEG1(IC_IPC_BASE)
#define     IC_OFF_BASE_OFFSET      (0x540)
#define     IC_OFF_BASE             (IC_BASE + 0x540)
#define     IC_OFF                  KSEG1(IC_OFF_BASE)
#define     IC_OFF_END_BASE_OFFSET  (0x898)
#define     IC_OFF_END_BASE         (IC_BASE + 0x898)

#define     IC_INTCON_BASE          (IC_BASE)
#define     IC_INTCON               KSEG1(IC_INTCON_BASE)
#define     IC_INTCON_INIT          0

#define     IC_PRISS_BASE           (IC_BASE + 0x10)
#define     IC_PRISS                KSEG1(IC_PRISS_BASE)
#define     IC_PRISS_SS0            0x1

#define     IC_INTSTAT_BASE         (IC_BASE + 0x20)
#define     IC_INTSTAT              KSEG1(IC_INTSTAT_BASE)
#define     IC_INTSTAT_SIRQ_MASK    0xFF
#define     IC_INTSTAT_SRIPL_SHIFT  8

#define     IC_IPTMR_BASE           (IC_BASE + 0x30)
#define     IC_IPTMR                KSEG1(IC_IPTMR_BASE)

#define     IC_INT_NUM                  214

#define     MAX_IRQ_LEVEL               8

#define     IC_CP0_TIMER_MASK           0x00000001
#define     IC_CP0_TIMER_WORD_NUMBER    0
#define     IC_TIMER_IRQ                0

#define     IC_IPC_LIMIT_MASK           0x0000001c
#define     IC_IPC_LIMIT                0x00000018

struct vmic {
	unsigned int w4[74];    // hdr=4, IFS=7+1, IEC=7+1 and IPC=54 words
	unsigned int off[214];  // OFF regs
};

extern unsigned int const *const vm_ic_masks[];
extern unsigned int const *const vm_ic_emulators[];

// only IFS and IEC addrs are here
static inline unsigned int get_icaddr2irq(unsigned long addr, unsigned int bit)
{
	unsigned int irq;

	if (addr >= IC_IEC_BASE_OFFSET)
		// convert to IFS addrs
		addr -= (IC_IEC_BASE - IC_IFS_BASE);
	irq = ((addr - IC_IFS_BASE_OFFSET)/16) * 32 + bit;
	return irq;
}

extern unsigned int read_vmic_off(unsigned long off);

static inline unsigned int get_irq2off(unsigned int irq)
{
	return  read_vmic_off(IC_OFF_BASE_OFFSET + (irq * 4));
}

static inline volatile unsigned int get_ic_mask(unsigned int tid, unsigned long vaoff,
			 unsigned int *emulator_flag)
{
	vaoff -= IC_IFS_BASE_OFFSET;    // skip headers
	*emulator_flag = *(vm_ic_emulators[tid] + (vaoff/16));
	return *(vm_ic_masks[tid] + (vaoff/16));
}

static inline volatile unsigned int *ic_ifs(unsigned int irq, unsigned int *ifs_mask)
{
	*ifs_mask = 1 << (irq % 32);
	// mult by 4 is required because of WRT,SET,CLR,INV
	return (IC_IFS + (irq/32)*4);
}

static inline volatile unsigned int *ic_iec(unsigned int irq, unsigned int *iec_mask)
{
	*iec_mask = 1 << (irq % 32);
	// mult by 4 is required because of WRT,SET,CLR,INV
	return (IC_IEC + (irq/32)*4);
}

static inline volatile unsigned int *ic_ipc(unsigned int irq, unsigned int *ipc_mask,
				   unsigned int *bit_shift)
{
	*bit_shift = (irq % 4) * 8;
	*ipc_mask = 0x1F << (*bit_shift);
	// mult by 4 is required because of WRT,SET,CLR,INV
	return (IC_IPC + (irq/4)*4);
}

static inline int   irq_available(unsigned int irq)
{
	switch (irq) {
	case 108:
	case 191:
	case 195:
	case 197:
	case 203:
	case 204:
	case 211:
	case 212:
		return 0;
	}

	return 1;
}
#endif


