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
#include    "irq.h"
#include    "ic.h"
#include    "thread.h"

void cancel_inject_IRQ(struct exception_frame *exfr)
{
	int irq;
	unsigned int tpl;
	unsigned int voff;

	if ((irq = pickup_irq(exfr, &tpl, &voff, 0)) >= 0) {
		inject_irq(irq, tpl, voff);

		current->injected_irq = irq;
		current->injected_ipl = tpl;
		return;
	}
	write_cp0_guestctl0ext(CP0_GUESTCTL0EXT_INIT);
	current->waiting_irq_number = 0;
	write_cp0_guestctl2(0);
	current->injected_irq = -1;
	current->injected_ipl = 0;
}

void exc_injected_handler(struct exception_frame *exfr)
{
	current->interrupted_irq = current->injected_irq;
	current->interrupted_ipl = current->injected_ipl;

	cancel_inject_IRQ(exfr);
}

// insert IRQ
void insert_IRQ(struct exception_frame *exfr, unsigned int vm, int irq, unsigned int ipl)
{
	unsigned int oldstatus;
	unsigned int oldvm;
	unsigned int voff;
	unsigned int gc2;
	struct thread *next;

	next  = get_vm_fp(vm);
	if (next->injected_ipl) {
		if (next == current)
			gc2 = read_cp0_guestctl2();
		else
			gc2 = next->cp0_guestctl2;
		if (!get__guestctl2__gripl(gc2)) {
			next->interrupted_irq = next->injected_irq;
			next->interrupted_ipl = next->injected_ipl;
			next->waiting_irq_number--;
			next->injected_irq = -1;
			next->injected_ipl = 0;
		}
	}
	next->waiting_irq_number++;
	if (next->injected_ipl >= ipl)
		return;

	oldvm = current->vmid;
	if (vm != oldvm) {
		if (get_status__ipl(next->g_cp0_status) >= ipl)
			return;
		if (!request_switch_to_vm(exfr, vm)) {
			next->thread_flags |= THREAD_FLAGS_RUNNING;

			voff = get_thread_irq2off(next, irq);
			next->cp0_guestctl2 = inject_irq_gc2(irq, ipl, voff);
			next->injected_irq = irq;
			next->injected_ipl = ipl;
			if (next->injected_ipl &&
			    !(vm_options[next->vmid] & VM_OPTION_POLLING))
				next->cp0_guestctl0ext = CP0_GUESTCTL0EXT_NOFCD;

			return;
		}
	} else if (get__ipl(read_g_cp0_viewipl()) >= ipl)
		return;
	next->thread_flags |= THREAD_FLAGS_RUNNING;

	voff = get_irq2off(irq);
	inject_irq(irq, ipl, voff);
	next->injected_irq = irq;
	next->injected_ipl = ipl;
	if (next->injected_ipl && !(vm_options[next->vmid] & VM_OPTION_POLLING))
			write_cp0_guestctl0ext(CP0_GUESTCTL0EXT_NOFCD);
	// return to guest
	return;
}

// SW emulated IRQ to vm=vmid from time.c
void execute_timer_IRQ(struct exception_frame *exfr, unsigned int vmid)
{
	unsigned int oldipl;
	unsigned int oldstatus;
	unsigned int oldvm;
	unsigned int gstatus;
	unsigned int ipl;
	struct thread *next  = get_vm_fp(vmid);

	ipl = get_thread_timer_ipl(next);
	set_thread_timer_irq(next);       // write IFS
	if (current->vmid == vmid)
		set_g_cp0_cause(CP0_CAUSE_TI);
	else
		set_thread_g_cp0_cause__ti(next);

	insert_sw_irq(vmid, IC_TIMER_IRQ);
	next->waiting_irq_number++;
	enforce_irq(exfr, vmid, IC_TIMER_IRQ);
}

//  IC write hook - move to proper file?
void insert_timer_IRQ(struct exception_frame *exfr, int irq)
{
	unsigned int gipl;
	unsigned int tipl;

	if ((read_g_cp0_cause() & CP0_CAUSE_TI) && get_thread_timer_mask(current)) {
		insert_IRQ(exfr, current->vmid, irq, get_thread_timer_ipl(current));
	}
}

































