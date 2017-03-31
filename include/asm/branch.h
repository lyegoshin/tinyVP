/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996, 1997, 1998, 2001 by Ralf Baechle
 */
#ifndef _ASM_BRANCH_H
#define _ASM_BRANCH_H

//#include <asm/ptrace.h>
#include <asm/inst.h>
#include "mips.h"

//extern int __isa_exception_epc(struct pt_regs *regs);
//extern int __compute_return_epc(struct pt_regs *regs);
//extern int __compute_return_epc_for_insn(struct pt_regs *regs,
//                                         union mips_instruction insn);
//extern int __microMIPS_compute_return_epc(struct pt_regs *regs);
//extern int __MIPS16e_compute_return_epc(struct pt_regs *regs);


static inline int delay_slot(struct exception_frame *exfr)
{
	return exfr->cp0_cause & CP0_CAUSE_BD;
}

#if 0
static inline unsigned long exception_epc(struct pt_regs *regs)
{
	if (likely(!delay_slot(regs)))
		return regs->cp0_epc;

	if (get_isa16_mode(regs->cp0_epc))
		return __isa_exception_epc(regs);

	return regs->cp0_epc + 4;
}
#endif

#define BRANCH_LIKELY_TAKEN 0x0001

static inline int compute_return_epc(struct exception_frame *exfr)
{
#if 0
	if (get_isa16_mode(regs->cp0_epc)) {
		if (cpu_has_mmips)
			return __microMIPS_compute_return_epc(regs);
		if (cpu_has_mips16)
			return __MIPS16e_compute_return_epc(regs);
		return regs->cp0_epc;
	}
#endif
	if (!delay_slot(exfr)) {
		exfr->cp0_epc += 4;
		return 0;
	}

	return __compute_return_epc(exfr);
}

#endif /* _ASM_BRANCH_H */
