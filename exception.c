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

#include    <asm/inst.h>
#include    <asm/branch.h>

#include    "mips.h"
#include    "stdio.h"
#include    "uart.h"
#include    "thread.h"

unsigned int reschedule_vm;

char panicbuf[128];

extern volatile unsigned long long current_wall_time;

void panic_thread(struct exception_frame *a0, char *message);

unsigned long gpr_read(struct exception_frame *exfr, unsigned int rt)
{
	unsigned int ie;
	unsigned long val;

	if (vz_mode(exfr)) {
		im_up(ie);
		write_cp0_srsctl(exfr->cp0_srsctl);
		ehb();
		switch (rt) {
		case 0: __asm__ __volatile__ ("rdpgpr %0,$0":"=r"(val)); break;
		case 1: __asm__ __volatile__ (".set push\n.set noat\nrdpgpr %0,$1\n.set pop":"=r"(val)); break;
		case 2: __asm__ __volatile__ ("rdpgpr %0,$2":"=r"(val)); break;
		case 3: __asm__ __volatile__ ("rdpgpr %0,$3":"=r"(val)); break;
		case 4: __asm__ __volatile__ ("rdpgpr %0,$4":"=r"(val)); break;
		case 5: __asm__ __volatile__ ("rdpgpr %0,$5":"=r"(val)); break;
		case 6: __asm__ __volatile__ ("rdpgpr %0,$6":"=r"(val)); break;
		case 7: __asm__ __volatile__ ("rdpgpr %0,$7":"=r"(val)); break;
		case 8: __asm__ __volatile__ ("rdpgpr %0,$8":"=r"(val)); break;
		case 9: __asm__ __volatile__ ("rdpgpr %0,$9":"=r"(val)); break;
		case 10: __asm__ __volatile__ ("rdpgpr %0,$10":"=r"(val)); break;
		case 11: __asm__ __volatile__ ("rdpgpr %0,$11":"=r"(val)); break;
		case 12: __asm__ __volatile__ ("rdpgpr %0,$12":"=r"(val)); break;
		case 13: __asm__ __volatile__ ("rdpgpr %0,$13":"=r"(val)); break;
		case 14: __asm__ __volatile__ ("rdpgpr %0,$14":"=r"(val)); break;
		case 15: __asm__ __volatile__ ("rdpgpr %0,$15":"=r"(val)); break;
		case 16: __asm__ __volatile__ ("rdpgpr %0,$16":"=r"(val)); break;
		case 17: __asm__ __volatile__ ("rdpgpr %0,$17":"=r"(val)); break;
		case 18: __asm__ __volatile__ ("rdpgpr %0,$18":"=r"(val)); break;
		case 19: __asm__ __volatile__ ("rdpgpr %0,$19":"=r"(val)); break;
		case 20: __asm__ __volatile__ ("rdpgpr %0,$20":"=r"(val)); break;
		case 21: __asm__ __volatile__ ("rdpgpr %0,$21":"=r"(val)); break;
		case 22: __asm__ __volatile__ ("rdpgpr %0,$22":"=r"(val)); break;
		case 23: __asm__ __volatile__ ("rdpgpr %0,$23":"=r"(val)); break;
		case 24: __asm__ __volatile__ ("rdpgpr %0,$24":"=r"(val)); break;
		case 25: __asm__ __volatile__ ("rdpgpr %0,$25":"=r"(val)); break;
		case 26: __asm__ __volatile__ ("rdpgpr %0,$26":"=r"(val)); break;
		case 27: __asm__ __volatile__ ("rdpgpr %0,$27":"=r"(val)); break;
		case 28: __asm__ __volatile__ ("rdpgpr %0,$28":"=r"(val)); break;
		case 29: __asm__ __volatile__ ("rdpgpr %0,$29":"=r"(val)); break;
		case 30: __asm__ __volatile__ ("rdpgpr %0,$30":"=r"(val)); break;
		case 31: __asm__ __volatile__ ("rdpgpr %0,$31":"=r"(val)); break;
		}
		im_down(ie);
		return val;
	} else {
		return exfr->gpr[rt];
	}
}

void gpr_write(struct exception_frame *exfr, unsigned int rt, unsigned long val)
{
	unsigned int ie;

	if (vz_mode(exfr)) {
		im_up(ie);
		write_cp0_srsctl(exfr->cp0_srsctl);
		ehb();
		switch (rt) {
		case 0: __asm__ __volatile__ ("wrpgpr $0,%0"::"r"(val)); break;
		case 1: __asm__ __volatile__ (".set push\n.set noat\nwrpgpr $1,%0\n.set pop"::"r"(val)); break;
		case 2: __asm__ __volatile__ ("wrpgpr $2,%0"::"r"(val)); break;
		case 3: __asm__ __volatile__ ("wrpgpr $3,%0"::"r"(val)); break;
		case 4: __asm__ __volatile__ ("wrpgpr $4,%0"::"r"(val)); break;
		case 5: __asm__ __volatile__ ("wrpgpr $5,%0"::"r"(val)); break;
		case 6: __asm__ __volatile__ ("wrpgpr $6,%0"::"r"(val)); break;
		case 7: __asm__ __volatile__ ("wrpgpr $7,%0"::"r"(val)); break;
		case 8: __asm__ __volatile__ ("wrpgpr $8,%0"::"r"(val)); break;
		case 9: __asm__ __volatile__ ("wrpgpr $9,%0"::"r"(val)); break;
		case 10: __asm__ __volatile__ ("wrpgpr $10,%0"::"r"(val)); break;
		case 11: __asm__ __volatile__ ("wrpgpr $11,%0"::"r"(val)); break;
		case 12: __asm__ __volatile__ ("wrpgpr $12,%0"::"r"(val)); break;
		case 13: __asm__ __volatile__ ("wrpgpr $13,%0"::"r"(val)); break;
		case 14: __asm__ __volatile__ ("wrpgpr $14,%0"::"r"(val)); break;
		case 15: __asm__ __volatile__ ("wrpgpr $15,%0"::"r"(val)); break;
		case 16: __asm__ __volatile__ ("wrpgpr $16,%0"::"r"(val)); break;
		case 17: __asm__ __volatile__ ("wrpgpr $17,%0"::"r"(val)); break;
		case 18: __asm__ __volatile__ ("wrpgpr $18,%0"::"r"(val)); break;
		case 19: __asm__ __volatile__ ("wrpgpr $19,%0"::"r"(val)); break;
		case 20: __asm__ __volatile__ ("wrpgpr $20,%0"::"r"(val)); break;
		case 21: __asm__ __volatile__ ("wrpgpr $21,%0"::"r"(val)); break;
		case 22: __asm__ __volatile__ ("wrpgpr $22,%0"::"r"(val)); break;
		case 23: __asm__ __volatile__ ("wrpgpr $23,%0"::"r"(val)); break;
		case 24: __asm__ __volatile__ ("wrpgpr $24,%0"::"r"(val)); break;
		case 25: __asm__ __volatile__ ("wrpgpr $25,%0"::"r"(val)); break;
		case 26: __asm__ __volatile__ ("wrpgpr $26,%0"::"r"(val)); break;
		case 27: __asm__ __volatile__ ("wrpgpr $27,%0"::"r"(val)); break;
		case 28: __asm__ __volatile__ ("wrpgpr $28,%0"::"r"(val)); break;
		case 29: __asm__ __volatile__ ("wrpgpr $29,%0"::"r"(val)); break;
		case 30: __asm__ __volatile__ ("wrpgpr $30,%0"::"r"(val)); break;
		case 31: __asm__ __volatile__ ("wrpgpr $31,%0"::"r"(val)); break;
		}
		im_down(ie);
	} else {
		exfr->gpr[rt] = val;
	}
}

int load_by_instruction(struct exception_frame *exfr, unsigned long value)
{
	unsigned int rt;
	unsigned int opcode;
	unsigned long vaddr;
	unsigned long gpr;
	unsigned int ie;

	rt = GET_INST_RT(exfr->cp0_badinst);
	opcode = GET_INST_OPCODE(exfr->cp0_badinst);

	switch (opcode) {
	case lb_op:
		gpr = (char)value;
		break;
	case lbu_op:
		gpr = (unsigned char)value;
		break;
	case lh_op:
		gpr = (short)value;
		break;
	case lhu_op:
		gpr = (unsigned short)value;
		break;
	case lw_op:
		gpr = (unsigned int)value;
		break;
	default:
		// something wrong here
		panic_thread(exfr, "Wrong load instruction from device\n");
		//compute_return_epc(exfr);
		return 0;
	}
	gpr_write(exfr, rt, gpr);
	return 1;
}

void panic_print(struct exception_frame *exfr, unsigned long sp, unsigned long fp,
		 unsigned long gp, unsigned long ra)
{
	sprintf(panicbuf, "Phys:   EPC=%08x Status=%08x Cause=%08x SRSctl=%08x Context=%08x ErrEPC=%08x\n",
	    mfc0(14, 0), mfc0(12, 0), mfc0(13, 0), mfc0(12, 2), mfc0(4, 0), mfc0(30, 0));
	uart_writeline(console_uart, panicbuf);
	sprintf(panicbuf, "\tGuestCtl0=%08x GC1=%08x GC2=%08x GC3=%08x GC0Ext=%08x BadVA=%08x BadInstP=%08x BadInst=%08x\n",
	    mfc0(12, 6), mfc0(10, 4), mfc0(10, 5), mfc0(10, 6), mfc0(11, 4), mfc0(8, 0), mfc0(8, 2), mfc0(8, 1));
	uart_writeline(console_uart, panicbuf);
	sprintf(panicbuf, "\tNestedEPC=%08x EXC=%08x SP=%08x FP=%08x GP=%08x RA=%08x KSCR0=%08x KSCR1=%08x\n",
	    mfc0(14, 2), mfc0(13, 5), sp, fp, gp, ra, mfc0(31, 2), mfc0(31, 3));
	uart_writeline(console_uart, panicbuf);

	sprintf(panicbuf, "Exfr: EPC=%08x Status=%08x Cause=%08x SRSctl=%08x Context=%08x\n",
	    exfr->cp0_epc, exfr->cp0_status, exfr->cp0_cause, exfr->cp0_srsctl, exfr->cp0_context);
	uart_writeline(console_uart, panicbuf);
	sprintf(panicbuf, "Guest:  EPC=%08x Status=%08x Cause=%08x NestedEPC=%08x EXC=%08x Ebase=%08x ErrEPC=%08x\n",
	    mfgc0(14, 0), mfgc0(12, 0), mfgc0(13, 0), mfgc0(14, 2), mfgc0(13, 5), mfgc0(15, 1), mfgc0(30, 0));
	uart_writeline(console_uart, panicbuf);

	sprintf(panicbuf, "Thread: vmid=%d IRQ=%d IPL=%d\n",
	    current->vmid, current->injected_irq, current->injected_ipl);
	uart_writeline(console_uart, panicbuf);
	sprintf(panicbuf, "\tGuest:  EPC=%08x Status=%08x Cause=%08x NestedEPC=%08x EXC=%08x\n",
	    current->g_cp0_epc, current->g_cp0_status, current->g_cp0_cause, current->g_cp0_nested_epc, current->g_cp0_nested_exc);
	uart_writeline(console_uart, panicbuf);
	dump_tlb();
}

void panic(void)
{
	unsigned int gp,sp,fp,ra;

	__asm__ __volatile__("move  %0, $28"
			    : "=&r" (gp)::);
	__asm__ __volatile__("move  %0, $29"
			    : "=&r" (sp)::);
	__asm__ __volatile__("move  %0, $30"
			    : "=&r" (fp)::);
	__asm__ __volatile__("move  %0, $31"
			    : "=&r" (ra)::);

	uart_writeline(console_uart, "\nPANIC:\n");
	panic_print(&(current->exfr), sp, fp, gp, ra);

	do { } while(1);
}

void panic_thread(struct exception_frame *exfr, char *message)
{
	unsigned int gp,sp,fp,ra;

	if (!get_context__shortprolog(exfr->cp0_context))
		panic();

	__asm__ __volatile__("move  %0, $28"
			    : "=&r" (gp)::);
	__asm__ __volatile__("move  %0, $29"
			    : "=&r" (sp)::);
	__asm__ __volatile__("move  %0, $30"
			    : "=&r" (fp)::);
	__asm__ __volatile__("move  %0, $31"
			    : "=&r" (ra)::);

	uart_writeline(console_uart, "\nPANIC_THREAD: ");
	uart_writeline(console_uart, message);
	uart_writeline(console_uart, "\n");
	panic_print(exfr, sp, fp, gp, ra);

	IRQ_wait_exit();
}

static void do_cu(struct exception_frame *exfr)
{
	if (get_cause__ce(exfr->cp0_cause) == 1) {
		set_exfr_status__cu1(exfr);
		if (fpu_owner == current->vmid)
			return;
		if (fpu_owner >= 0)
			save_fpu_regs(fpu_owner);
		fpu_owner = current->vmid;
		restore_fpu_regs();
		return;
	}
	panic_thread(exfr, "CUx exception\n");
}

static void do_dsp(struct exception_frame *exfr)
{
	set_exfr_status__mx(exfr);
	if (dsp_owner == current->vmid)
		return;
	if (dsp_owner >= 0)
		save_dsp_regs(dsp_owner);
	dsp_owner = current->vmid;
	restore_dsp_regs();
}

void do_gpsi(struct exception_frame *exfr)
{
	unsigned int inst = exfr->cp0_badinst;
	unsigned long gpr;
	unsigned long cp0;
	unsigned long val;
	unsigned int diff1;
	unsigned long long diff2;

	compute_return_epc(exfr);
	if (GET_INST_OPCODE(inst) == COP0) {
		if (inst_WAIT(inst)) {
			if (current->injected_irq < 0) {
				current->thread_flags &= ~THREAD_FLAGS_RUNNING;
				reschedule(exfr);
			}
			return;
		}
		if (inst_MFC0(inst)) {
			gpr = GET_INST_RT(inst);
			cp0 = EXTRACT_CP0_REG_AND_SEL(inst);
			switch (cp0) {
			case CP0_prid_MERGED:
				gpr_write(exfr, gpr, read_cp0_prid());
				return;
			case CP0_count_MERGED:
				current->guest_read_count = read_g_cp0_count();
				current->guest_read_time = current_wall_time;
				gpr_write(exfr, gpr, current->guest_read_count);
				return;
			case CP0_compare_MERGED:
				gpr_write(exfr, gpr, read_g_cp0_compare());
				return;
			case CP0_cdmmbase_MERGED:
				gpr_write(exfr, gpr, read_cp0_cdmmbase());
				return;
			case CP0_srsctl_MERGED:
				gpr_write(exfr, gpr, read_g_cp0_srsctl());
				return;
			case CP0_srsmap_MERGED:
				gpr_write(exfr, gpr, read_g_cp0_srsmap());
				return;
			case CP0_srsmap2_MERGED:
				gpr_write(exfr, gpr, read_g_cp0_srsmap2());
				return;

			case CP0_taglo_MERGED:
			case CP0_datalo_MERGED:
			case CP0_errctl_MERGED:
			case CP0_cacheerr_MERGED:
			case CP0_watchlo_MERGED:
			case CP0_watchhi_MERGED:
			case CP0_debug_MERGED:
			case CP0_perfctl0_MERGED:
			case CP0_perfctl1_MERGED:
			case CP0_perfctl2_MERGED:
			case CP0_perfctl3_MERGED:
				gpr_write(exfr, gpr, 0);
				return;
			}
		}
		if (inst_MTC0(inst)) {
			gpr = GET_INST_RT(inst);
			cp0 = EXTRACT_CP0_REG_AND_SEL(inst);
			switch (cp0) {
			case CP0_count_MERGED:
				current->cp0_gtoffset = gpr_read(exfr, gpr) - read_cp0_count();
				write_cp0_gtoffset(current->cp0_gtoffset);
				ehb();
				timer_g_irq_reschedule(current->vmid, read_g_cp0_compare() - current->cp0_gtoffset);
				return;
			case CP0_compare_MERGED:
				val = gpr_read(exfr, gpr);
				write_g_cp0_compare(val);
				ehb();
				clear_g_cp0_cause(CP0_CAUSE_TI);
				clear_timer_irq();
				if (current->injected_irq == IC_TIMER_IRQ)
					cancel_inject_IRQ(exfr);
				diff1 = val - current->guest_read_count;
				diff2 = current_wall_time - current->guest_read_time;
				if (diff2 > time_convert_clocks(diff1)) {
					execute_timer_IRQ(exfr, current->vmid);
					return;
				}
				timer_g_irq_reschedule(current->vmid, val - current->cp0_gtoffset);
				return;
			case CP0_srsctl_MERGED:
			case CP0_srsmap_MERGED:
			case CP0_srsmap2_MERGED:
				if (val = gpr_read(exfr, gpr))
					panic_thread(exfr, "Non-zero SRS is used\n");
				return;
			case CP0_config5_MERGED:
				write_g_cp0_config5(gpr_read(exfr, gpr));
				return;
			case CP0_config0_MERGED:
			case CP0_config6_MERGED:
			case CP0_config7_MERGED:
			case CP0_taglo_MERGED:
			case CP0_datalo_MERGED:
			case CP0_errctl_MERGED:
			case CP0_watchlo_MERGED:
			case CP0_watchhi_MERGED:
			case CP0_debug_MERGED:
			case CP0_perfctl0_MERGED:
			case CP0_perfctl1_MERGED:
			case CP0_perfctl2_MERGED:
			case CP0_perfctl3_MERGED:
				// ignore
				return;
			}
		}
	}
	if (inst_RDHWR(inst)) {
		if (GET_INST_RD(inst) == 4) {
			// read count
			gpr = GET_INST_RT(inst);
			gpr_write(exfr, gpr, read_g_cp0_count());
			return;
		}
	}
	if (inst_CACHE(inst)) {
		return;
	}
	panic_thread(exfr, "Unknown GPSI\n");
}

// unfortunately, we need to track GSFC to get EXL change tracked
void do_gsfc(struct exception_frame *exfr)
{
	unsigned int inst = exfr->cp0_badinst;
	unsigned long gpr;
	unsigned long cp0;
	unsigned long val;

	if (GET_INST_OPCODE(inst) == COP0) {
		if (inst_MTC0(inst)) {
			gpr = GET_INST_RT(inst);
			cp0 = EXTRACT_CP0_REG_AND_SEL(inst);
			switch (cp0) {
			// next are cases for FCD=0 tracking
			case CP0_status_MERGED:
				val = gpr_read(exfr, gpr);
				if (val & (CP0_STATUS_CU2|CP0_STATUS_CU3))
					panic_thread(exfr, "CU2/CU3 is set\n");
				val &= ~CP0_STATUS_ROOTMASK;

				write_g_cp0_status(val);
				break;
			case CP0_cause_MERGED:
				val = gpr_read(exfr, gpr);
				val &= ~CP0_CAUSE_ROOTMASK;
				write_g_cp0_cause(val);
				break;
			case CP0_intctl_MERGED:
				val = gpr_read(exfr, gpr);
				write_g_cp0_intctl(val);
				break;
			case CP0_pagegrain_MERGED:
				val = gpr_read(exfr, gpr);
				write_g_cp0_pagegrain(val);
				break;
			default:
				panic_thread(exfr, "UNKNOWN GSFC!\n");
				return;
			}
			compute_return_epc(exfr);
			return;
		}
	}
	panic_thread(exfr, "UNKNOWN GSFC\n");
}

extern char *longlong_to_timestring(char *buf, size_t len, unsigned long long n);

void do_EXC(struct exception_frame *exfr)
{
	unsigned int cause = (exfr->cp0_cause & CP0_CAUSE_CODE) >> CP0_CAUSE_CODE_SHIFT;
	unsigned int gcause;
	unsigned int gstatus;

	switch (cause) {
	case CAUSE_TLBM:
	case CAUSE_TLBL:
	case CAUSE_TLBS:
	case CAUSE_TLBRI:
	case CAUSE_TLBXI:
	case CAUSE_IBE:
	case CAUSE_DBE:
		do_TLB(exfr, cause);
		return;
	case CAUSE_CU:
		do_cu(exfr);
		compute_return_epc(exfr);
		return;
	case CAUSE_DSP:
		do_dsp(exfr);
		compute_return_epc(exfr);
		return;
	case CAUSE_GE:
		gcause = (exfr->cp0_guestctl0 & CP0_CAUSE_CODE) >> CP0_CAUSE_CODE_SHIFT;
		switch (gcause) {
		case GUEST_CAUSE_GPSI:
			do_gpsi(exfr);
			return;
		case GUEST_CAUSE_GSFC:
			do_gsfc(exfr);
			return;
		case GUEST_CAUSE_GHFC:
			if (current->injected_ipl && !get__guestctl2__gripl(read_cp0_guestctl2()))
				exc_injected_handler(exfr);
			return;
		}
		break;
	case CAUSE_SYS:
		if (inst_SYSCALL_F0000(exfr->cp0_badinst)) {
			compute_return_epc(exfr);
			current->thread_flags &= ~THREAD_FLAGS_RUNNING;
			if (reschedule_vm) {
				unsigned int vmid;

				vmid = reschedule_vm;
				reschedule_vm = 0;
				switch_to_vm(exfr, vmid);
				return;
			}
			reschedule(exfr);
			return;
		}
		break;
	}
sprintf(panicbuf, "do_EXC: cause=%x exfr->cp0_badinst=0x%08x inst_SYSCALL_F0000?=%d\n",
cause, exfr->cp0_badinst, inst_SYSCALL_F0000(exfr->cp0_badinst));
uart_writeline(console_uart, panicbuf);
	panic_thread(exfr,"Unexpect type of exception\n");
}
