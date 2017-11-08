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

#include    <asm/inst.h>
#include    <asm/branch.h>

#include    "mips.h"
#include    "tlb.h"
#include    "uart.h"

struct cpte_cache cpte_cache[16] = {
	{ -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 },
	{ -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 }
};
struct cpte_cache sw_cpte_cache[16] = {
	{ -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 },
	{ -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 }
};

static char str[128];

unsigned long get_pa_from_cpte(struct cpte_dl const *cpte_double_leaf, unsigned long pagemask,
			       unsigned long badvaddr, unsigned long *fulloffset,
			       unsigned long *ELo, unsigned long pagemask_min)
{
	unsigned long max_offset;
	unsigned long offset;
	unsigned long pa;

	max_offset = (1 << (32 - clz(pagemask|pagemask_min)));
	offset = badvaddr & (max_offset - 1);
	*fulloffset = offset;
	if (offset < (max_offset/2))
		pa = cpte_double_leaf->el0;
	else {
		pa = cpte_double_leaf->el1;
		offset -= (max_offset/2);
	}
	if (pagemask_min) {
		// physical access via 4K page
		*ELo = pa;
		pa = ((pa & ENTRYLO_PFN) << (CPTE_PAGEMASK_VPN2_SHIFT - 1 - ENTRYLO_PFN_SHIFT)) + offset;
	} else {
		// SW access into short regions
		*ELo = (pa & ENTRYLO_SW) | ENTRYLO_SW_C;    // set C or UC
		pa = (pa & ~ENTRYLO_SW) + offset;
	}

	return pa;
}

unsigned long map_tmp_address(unsigned int *ie, unsigned long address, unsigned long ELo)
{
	unsigned int im;
	im_up(im);
	*ie = im;
	write_cp0_pagemask(MAP_TMP_PAGEMASK);
	write_cp0_entryhi(MAP_TMP_ADDRESS);
	write_cp0_guestctl1(0);
	ehb();
	__asm__ __volatile__("tlbp");
	ehb();
	write_cp0_entrylo0(((address & ~(PAGE_TMP_SIZE - 1)) >> ENTRYLO_PFN_SHIFT)
			   |ENTRYLO_V| (ELo & ENTRYLO_HWBITS));
	write_cp0_entrylo1(0);
	ehb();
	if (read_cp0_index() & 0x80000000)
		__asm__ __volatile__("tlbwr"::);
	else
		__asm__ __volatile__("tlbwi"::);
	return MAP_TMP_ADDRESS | (address & (PAGE_TMP_SIZE - 1));
}

void unmap_tmp_address(unsigned int *ie, unsigned long address)
{
	unsigned int gid = current->gid;

	write_cp0_entryhi(MAP_TMP_ADDRESS);
	__asm__ __volatile__("tlbp");
	ehb();
	if (!(read_cp0_index() & 0x80000000)) {
		write_cp0_entryhi(MAP_TMP_ADDRESS|ENTRYHI_EHINV);
		ehb();
		__asm__ __volatile__("tlbwi"::);
	}
	write_cp0_guestctl1((gid << CP0_GUESTCTL1_RID_SHIFT) | gid);
	im_down(*ie);
}

int     decode_exception_instruction(struct exception_frame *exfr,
				     unsigned long *value)
{
	unsigned int rt;
	unsigned int opcode;
	int ret;

	rt = GET_INST_RT(exfr->cp0_badinst);
	opcode = GET_INST_OPCODE(exfr->cp0_badinst);

	switch (opcode) {
	case lb_op:
	case lbu_op:
		return -1;
	case lh_op:
	case lhu_op:
		return -2;
	case lw_op:
		return -4;
	case sb_op:
		ret = 8;
		break;
	case sh_op:
		ret = 16;
		break;
	case sw_op:
		ret = 32;
		break;
	default:
		return  0;
	}
	rt = gpr_read(exfr, rt);
	*value = (rt << (32 - ret)) >> (32 - ret);
	return  ret/8;
}

int     emulate_rw(struct exception_frame *exfr, unsigned long address, unsigned long ELo)
{
	unsigned int rt;
	unsigned int opcode;
	unsigned long vaddr;
	unsigned long gpr;
	unsigned int ie;

	opcode = GET_INST_OPCODE(exfr->cp0_badinst);
	if ((ELo & ENTRYLO_SW_I) && IS_STORE_INST(opcode)) {
		if (ELo & ENTRYLO_D) {
			rt = decode_exception_instruction(exfr, &gpr);
			printf("Thread%d: write %x (%d bytes) to %08x ignored\n", current->tid, gpr, rt, address);
		}
		return 1;
	}
	rt = GET_INST_RT(exfr->cp0_badinst);
	vaddr = map_tmp_address(&ie, address, ELo);

	switch (opcode) {
	case lb_op:
		gpr = *(char *)vaddr;
		break;
	case lbu_op:
		gpr = *(unsigned char *)vaddr;
		break;
	case lh_op:
		gpr = *(short *)vaddr;
		break;
	case lhu_op:
		gpr = *(unsigned short *)vaddr;
		break;
	case lw_op:
		gpr = *(unsigned int *)vaddr;
		break;
	case sb_op:
		*(char *)vaddr = gpr_read(exfr, rt);
		goto finish;
	case sh_op:
		*(short *)vaddr = gpr_read(exfr, rt);
		goto finish;
	case sw_op:
		*(int *)vaddr = gpr_read(exfr, rt);
		goto finish;
	default:
		unmap_tmp_address(&ie, address);
		return 0;
	}
	gpr_write(exfr, rt, gpr);
finish:
	unmap_tmp_address(&ie, address);
	return 1;
}

void    do_TLB(struct exception_frame *exfr, unsigned int cause)
{
	struct cpte_nd const *cpte;
	unsigned long badvaddr;
	unsigned long offset;
	unsigned long pagemask;
	unsigned long address;
	unsigned long const (* bitmask)[];
	unsigned long ELo;
	int bit;
	unsigned int i;
	unsigned int context;
	unsigned long value;

	switch (cause) {
	case CAUSE_TLBL:
	case CAUSE_TLBS:
	case CAUSE_TLBM:
		break;
	default:
		// no RI/XI/DBE/IBE failures support
		goto bad_area;
	}

	badvaddr = exfr->cp0_badvaddr;
	offset = badvaddr >> 29;    // 8 elements in top level
	pagemask = ~0xE0000000;
	cpte = cpt_base[current->tid] + offset;

	context = exfr->cp0_context << (CP0_CONTEXT_VM_LEN + 1);
	i = cpte_address_hash(context);
	// is it in cache? - skip tree search
	if (cpte_cache[i].cp0_context ==
	    (badvaddr & ~(cpte_cache[i].pagemask /* |PAGEMASK_MIN */))) {
		cpte = cpte_cache[i].cpte;
		pagemask = cpte_cache[i].pagemask;
		goto skip_search;
	}

loop:
	while (cpte->mask & 0x1) {
		if (cpte->golden != (badvaddr & cpte->mask & ~0x1))
			goto bad_area;

		offset = ((badvaddr << cpte->ls) >> cpte->rs) & ~CPTE_BLOCK_SIZE_MASK;
		pagemask = cpte->pm << (CPTE_SHIFT_SIZE + CPTE_SHIFT_SIZE);
		cpte = (struct cpte_nd *)((unsigned char *)(cpte->next) + offset);
	};

	pagemask |= PAGEMASK_MIN;

	// save cpte in cache for faster emulation
	cpte_cache[i].cp0_context = (badvaddr & ~(pagemask /* |PAGEMASK_MIN */));
	cpte_cache[i].pagemask = pagemask;
	cpte_cache[i].cpte = cpte;

skip_search:

	if ((!((struct cpte_dl *)cpte)->el0) &&
	    (!((struct cpte_dl *)cpte)->el1)) {
		if (!(cpte->next))
			goto bad_area;

		i = sw_cpte_address_hash(badvaddr);
		// is it in sw cache? - skip tree search
		if (sw_cpte_cache[i].cp0_context ==
		    (badvaddr & ~sw_cpte_cache[i].pagemask)) {
			cpte = sw_cpte_cache[i].cpte;
			pagemask = sw_cpte_cache[i].pagemask;
			goto skip_sw_search;
		}

//                pagemask = cpte->mask | 1; // low bit must be 1
		cpte = (struct cpte_nd *)((unsigned char *)(cpte->next));

		while (cpte->mask & 0x1) {
			if (cpte->golden != (badvaddr & cpte->mask & ~0x1))
				goto bad_area;

			offset = ((badvaddr << cpte->ls) >> cpte->rs) & ~CPTE_BLOCK_SIZE_MASK;
			pagemask = cpte->pm;
			cpte = (struct cpte_nd *)((unsigned char *)(cpte->next) + offset);
		};

		// save cpte in cache for faster emulation
		sw_cpte_cache[i].cp0_context = (badvaddr & ~pagemask);
		sw_cpte_cache[i].pagemask = pagemask;
		sw_cpte_cache[i].cpte = cpte;
skip_sw_search:
		address = get_pa_from_cpte((struct cpte_dl*)cpte, pagemask,
					   badvaddr, &offset, &ELo, 0);
	} else
		address = get_pa_from_cpte((struct cpte_dl*)cpte, pagemask,
					   badvaddr, &offset, &ELo, PAGEMASK_MIN);

	// start emulation here, cpte_double_leaf is available
	if (!(((struct cpte_dl*)cpte)->f)) {
		if (((struct cpte_dl *)cpte)->bm) {
			bitmask = ((struct cpte_dl *)cpte)->bm;
			bit = GETBIT(bitmask, offset/4);    // convert byte offset to word
		} else if ((ELo ^ ENTRYLO_V) & (ENTRYLO_SW_I | ENTRYLO_V))
			bit = 1;
		if (bit) {
			if (!emulate_rw(exfr, address, ELo))
				goto bad_area;  // something wrong: LWR/SWL/LWC1/SDC1/LL
			compute_return_epc(exfr);
			return;
		}
	} else {
		// here should be a real emulation of device. CPTE - only bm can be used
		i = decode_exception_instruction(exfr, &value);
		if (((struct cpte_dl*)cpte)->f(exfr, cpte, pagemask, address, ELo, i, value)) {
		    compute_return_epc(exfr);
		    return;
		}
	}

bad_area:   ;

	panic_thread(exfr, "TLB exception - bad address\n");
}

void dump_tlb()
{
	unsigned long l0, l1, hi, ind, ind0, pm, gctl1;
	unsigned int ie, find;

	im_up(ie);
	sprintf(str,"EntryHI=0x%08x Lo0=0x%08x Lo1=0x%08x PM=0x%08x PG=%08x IND=0x%08x\n",
	       mfc0(10, 0), mfc0(2, 0), mfc0(3, 0), mfc0(5, 0), mfc0(5, 1), mfc0(0, 0));
	uart_writeline(console_uart, str);
	im_down(ie);

	ind0 = 0;
	find = read_cp0_config1();
	find = (find & 0x7E000000) >> 25;
	do {
	    im_up(ie);
	    write_cp0_index(ind0);
	    ehb();
	    __asm__ __volatile__("tlbr"::);
	    ehb();
	    ind = read_cp0_index();
		    l0 = read_cp0_entrylo0();
		    l1 = read_cp0_entrylo1();
		    hi = read_cp0_entryhi();
		    pm = read_cp0_pagemask();
		    gctl1 = read_cp0_guestctl1();
		    sprintf(str,"TLB%d: HI=0x%08x Lo0=0x%08x Lo1=0x%08x PM=0x%08x GCTL1=0x%08x\n",
			   ind0,hi,l0,l1,pm,gctl1);
		    uart_writeline(console_uart, str);
	    im_down(ie);
	    if (ind0 == find)
		    break;
	    ind0++;
	} while(1);
}
