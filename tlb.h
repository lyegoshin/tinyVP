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

#ifndef _TLB_H
#define _TLB_H

#include "bits.h"
#include "mips.h"

//  TLB fields

#define ENTRYLO_CCA_UC          0x00000010  /* CCA = 2 */
#define ENTRYLO_CCA_CA          0x00000018  /* CCA = 3 */
#define ENTRYLO_D               0x00000004
#define ENTRYLO_V               0x00000002
#define ENTRYLO_PFN             0x03FFFFC0
#define ENTRYLO_PFN_SHIFT       6           /* 4KB pages !!! */
#define ENTRYLO_HWBITS          0x0000003E  /* G bit is used for "write-ignore" */
#define ENTRYLO_SW              0x0000000F  // SW emulated HWBITS, again - G bit is reused
#define ENTRYLO_SW_C            0x00000010  // middle CCA bit, to complete UC/C */
#define ENTRYLO_SW_I            0x00000001  /* G bit, used as "write-ignore" */
#define PAGE_TMP_SIZE           4096
#define MAP_TMP_ADDRESS         (0 - (PAGE_TMP_SIZE * 2))
#define MAP_TMP_PAGEMASK        0x00001FFF
#define PAGEMASK_MIN            0x00001FFF  /* 4KB pages */

#define ENTRYHI_EHINV           0x00000400
#define CPTE_PAGEMASK_VPN2_SHIFT    13      /* aligned to 4KB pages */
#define CPTE_SHIFT_SIZE             5
#define CPTE_BLOCK_SIZE_MASK        0xf

struct cpte_nd;

typedef unsigned int (emulator)(struct exception_frame *exfr,
				struct cpte_nd const *cpte,
				unsigned long pagemask,
				unsigned long addr, unsigned long elo,
				int flag, unsigned long value);

struct cpte_nd {
	union  cpte         const * const next;
	unsigned long       mask;
	unsigned long       golden;
	BITFIELD_FIELD(unsigned long pm : 20,
	BITFIELD_FIELD(unsigned long rs : CPTE_SHIFT_SIZE,
	BITFIELD_FIELD(unsigned long ls : CPTE_SHIFT_SIZE,
	;)))
};

struct cpte_dl {
	emulator            const * const f;
	unsigned long       const (* const bm)[];
	unsigned long       el0;
	unsigned long       el1;
};

union cpte {
	struct cpte_nd  cpte_nd;
	struct cpte_dl  cpte_dl;
};

extern struct cpte_nd const * const cpt_base[];

struct cpte_cache {
	unsigned long   cp0_context;
	unsigned long   pagemask;
	struct cpte_nd const *cpte;
};

// 16 elements in cache for now, hardcoded in initialization
#define cpte_address_hash(x)    (((x >> 23) ^ (x >> 7) ^ (x >> 3)) & 0xf)
extern struct cpte_cache cpte_cache[16];
// 16 elements in cache for now, hardcoded in initialization
#define sw_cpte_address_hash(x) (((x >> 12) ^ (x >> 8)) & 0xf)
extern struct cpte_cache sw_cpte_cache[16];

#endif  /* _TLB_H */
