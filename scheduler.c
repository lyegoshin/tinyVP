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

#include    "mips.h"
#include    "thread.h"
#include    "time.h"

unsigned long vm0_thread;
unsigned long thread_size = VM_FRAME_SIZE;
unsigned int fpu_owner;
unsigned int dsp_owner;
unsigned int reschedule_flag;

#include "uart.h"
void switch_to_vm(struct exception_frame *exfr, unsigned int vmid)
{
	register struct thread *v0 = get_vm_fp(vmid);

	if (v0->thread_flags & THREAD_FLAGS_STOPPED)
		return;

	if (current->vmid) {

		current->g_cp0_index        = read_g_cp0_index();
		current->g_cp0_entryhi      = read_g_cp0_entryhi();
		current->g_cp0_entrylo0     = read_g_cp0_entrylo0();
		current->g_cp0_entrylo1     = read_g_cp0_entrylo1();
		current->g_cp0_pagemask     = read_g_cp0_pagemask();

		if (current->g_cp0_wired = read_g_cp0_wired()) {
			int i;
			for ( i=0; i < current->g_cp0_wired; i++ ) {
				write_g_cp0_index(i);
				ehb();
				tlbgr();
				ehb();
				current->g_cp0_wired_hi[i] = read_g_cp0_entryhi();
				current->g_cp0_wired_lo0[i] = read_g_cp0_entrylo0();
				current->g_cp0_wired_lo1[i] = read_g_cp0_entrylo1();
				current->g_cp0_wired_pm[i] = read_g_cp0_pagemask();
			}
		}

		current->g_cp0_context      = read_g_cp0_context();

		current->g_cp0_userlocal    = read_g_cp0_userlocal();
		current->g_cp0_pagegrain    = read_g_cp0_pagegrain();
		current->g_cp0_hwrena       = read_g_cp0_hwrena();

		current->g_cp0_badvaddr     = read_g_cp0_badvaddr();
		current->g_cp0_badinst      = read_g_cp0_badinst();
		current->g_cp0_badinstp     = read_g_cp0_badinstp();

		current->g_cp0_compare      = read_g_cp0_compare();
		current->g_cp0_status       = read_g_cp0_status();
		current->g_cp0_cause        = read_g_cp0_cause();
		current->g_cp0_epc          = read_g_cp0_epc();

		current->g_cp0_nested_epc   = read_g_cp0_nestedepc();
		current->g_cp0_ebase        = read_g_cp0_ebase();
		current->g_cp0_errorepc     = read_g_cp0_errorepc();
		current->g_cp0_kscratch0    = read_g_cp0_kscratch0();

		current->g_cp0_kscratch1    = read_g_cp0_kscratch1();

		current->g_cp0_intctl       = read_g_cp0_intctl();
		current->cp0_guestctl0ext   = read_cp0_guestctl0ext();
		current->cp0_guestctl2      = read_cp0_guestctl2();

		__asm__ __volatile__(
			"jal    _clear_g_ll"
			::: "$1", "$31", "memory");
	}

	current = v0;

	write_cp0_guestctl1(current->cp0_guestctl1);
	current->exfr.cp0_status &= ~(CP0_STATUS_CU1|CP0_STATUS_MX|CP0_STATUS_CPU_CONTROL);
	current->exfr.cp0_status |= (exfr->cp0_status & CP0_STATUS_CPU_CONTROL);
	current->exfr.gpr[SP] = exfr->gpr[SP]; // move stack ptr to today's level

	if (vmid) {
		if (current->g_cp0_wired) {
			int i;
			for ( i=0; i < current->g_cp0_wired; i++ ) {
				write_g_cp0_index(i);
				write_g_cp0_entryhi(current->g_cp0_wired_hi[i]);
				write_g_cp0_entrylo0(current->g_cp0_wired_lo0[i]);
				write_g_cp0_entrylo1(current->g_cp0_wired_lo1[i]);
				write_g_cp0_pagemask(current->g_cp0_wired_pm[i]);
				ehb();
				tlbgwi();
				ehb();
			}
		}
		write_g_cp0_wired(current->g_cp0_wired);

		write_g_cp0_index(    current->g_cp0_index        );
		write_g_cp0_entryhi(  current->g_cp0_entryhi      );
		write_g_cp0_entrylo0( current->g_cp0_entrylo0     );
		write_g_cp0_entrylo1( current->g_cp0_entrylo1     );
		write_g_cp0_pagegrain(current->g_cp0_pagegrain    );

		write_g_cp0_context(  current->g_cp0_context      );

		write_g_cp0_userlocal(current->g_cp0_userlocal    );
		write_g_cp0_pagemask( current->g_cp0_pagemask     );
		write_g_cp0_hwrena(   current->g_cp0_hwrena       );

		write_g_cp0_badvaddr( current->g_cp0_badvaddr     );
		write_g_cp0_badinst(  current->g_cp0_badinst      );
		write_g_cp0_badinstp( current->g_cp0_badinstp     );

		write_g_cp0_status(   current->g_cp0_status       );
		write_g_cp0_nestedepc(current->g_cp0_nested_epc   );
		ehb();  /* errata E5 */
		write_g_cp0_epc(      current->g_cp0_epc          );

		write_g_cp0_ebase(    current->g_cp0_ebase        );
		write_g_cp0_errorepc( current->g_cp0_errorepc     );
		write_g_cp0_kscratch0(current->g_cp0_kscratch0    );

		write_g_cp0_kscratch1(current->g_cp0_kscratch1    );
		write_g_cp0_intctl(   current->g_cp0_intctl       );

		write_g_cp0_cause(    current->g_cp0_cause        );
		ehb();
		// CP0 registers
		write_cp0_gtoffset(   current->cp0_gtoffset       );
		ehb();
		write_g_cp0_compare(  current->g_cp0_compare      );
		ehb();
		if (current->g_cp0_cause & CP0_CAUSE_TI)
			write_g_cp0_cause(    current->g_cp0_cause        );
		write_cp0_guestctl0ext( current->cp0_guestctl0ext );
		write_cp0_guestctl2(  current->cp0_guestctl2      );
		write_cp0_guestctl3(  current->cp0_guestctl3      );
		ehb();
	}

	current->reschedule_count++;
	current->thread_flags |= THREAD_FLAGS_RUNNING;
	return;
}

int request_switch_to_vm(struct exception_frame *exfr, unsigned int vmid)
{
	if (get_context__shortprolog(exfr->cp0_context)) {
		switch_to_vm(exfr, vmid);
		return 1;
	}
	if (reschedule_vm < vmid)
		reschedule_vm = vmid;
	return 0;
}

void reschedule(struct exception_frame *exfr)
{
	struct thread *next;
	unsigned int weight;
	unsigned int tmp;
	int ipl;
	int tpl;
	int pri;
	int vm;
	int vmid;
	int irq;
	unsigned int voff;

	/* TBD: ... !get_context__shortprolog ? ... */
	if (!current->vmid) {
		// VM0 has an absolute priority and can't execute G.WAIT
		// ... only if no WAIT st...
		if (!get_exfr_context__waitflag(exfr))
			return;
	}

	weight = (unsigned int)(-1);
	vm = 0;
	ipl = get_status__ipl(exfr->cp0_status);
	pri = 0;
	for (vmid=1; vmid<=MAX_NUM_GUEST; vmid++) {
		if (!vm_list[vmid])
			continue;

		next = get_vm_fp(vmid);
		if (!(next->thread_flags & THREAD_FLAGS_RUNNING))
			continue;
		if (next->thread_flags & THREAD_FLAGS_STOPPED)
			continue;

		// skip low IPL VMs, IRQ should go first
		tpl = get_status__ipl(next->exfr.cp0_status);
		if (tpl < ipl)
			continue;
		if (tpl < pri)
			continue;

		tmp = next->reschedule_count + (2 * vmid);
		if (tmp < weight) {
			pri = tpl;
			weight = tmp;
			vm = vmid;
		}
	}

	if (vm != current->vmid) {
		switch_to_vm(exfr, vm);

	}
}

void init_vm(unsigned int vmid, int traceflag)
{
	struct thread *thread = get_vm_fp(vmid);
	struct exception_frame *exfr = &(thread->exfr);

	bzero((unsigned long)thread, sizeof(struct thread));

	thread->vmid                =   vmid;
	thread->injected_irq        =   -1;
	thread->interrupted_irq        =   -1;
	thread->last_interrupted_irq   =   -1;
	thread->last_used_lcount = current_lcount;
	thread->g_cp0_count = (unsigned int)current_lcount;
	thread->lcompare = current_lcount - 1;
	thread->g_cp0_compare = (unsigned int)(thread->lcompare);
	init_vmic(thread);

	thread->g_cp0_status        = CP0_STATUS_BEV | CP0_STATUS_CU0 | CP0_STATUS_ERL;
//        thread->g_cp0_status        = 0;

	thread->g_cp0_epc           = vm_list[vmid];

	thread->g_cp0_ebase         = CP0_G_EBASE_INIT;

	thread->g_cp0_intctl        = CP0_G_INTCTL_INIT;

	thread->g_cp0_compare       = read_cp0_count();

	thread->cp0_guestctl0ext    = CP0_GUESTCTL0EXT_INIT;
	thread->cp0_guestctl1       = (vmid << CP0_GUESTCTL1_RID_SHIFT) | vmid;
	thread->cp0_guestctl3       = vmid;

	exfr->cp0_context       = vmid << CP0_CONTEXT_VM_SHIFT;
//        exfr->cp0_epc           = (unsigned long)&IRQ_wait_exit;
	exfr->cp0_epc           = vm_list[vmid];
	exfr->cp0_srsctl        = vmid << CP0_SRSCTL_PSS_SHIFT;
	exfr->cp0_guestctl0     = CP0_GUESTCTL0_INIT;
	exfr->cp0_status        = CP0_STATUS_INIT;

	if (!traceflag)
		thread->thread_flags        =   THREAD_FLAGS_RUNNING;
	else
		thread->thread_flags        =   traceflag;
}

void init_scheduler()
{
	int vmid;

	write_cp0_guestctl1(0);

	current->vmid               =   0;
	current->thread_flags       =   THREAD_FLAGS_RUNNING;
	//current->preempt_count      =   0;
	//current->reschedule_count   =   0;

	// common guest configuration
	write_g_cp0_srsctl(0);
	write_g_cp0_srsmap(0);
	write_g_cp0_srsmap2(0);

	for (vmid=1; vmid<MAX_NUM_GUEST; vmid++) {
		if (!vm_list[vmid])
			continue;

		init_vm(vmid, 0);
	}
}

