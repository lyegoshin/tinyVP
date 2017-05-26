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

#ifndef _THREAD_H
#define _THREAD_H

//#include    "mips.h"
#include    "vmdev.h"

struct exception_frame {
//  SP/FP: common block
	struct exception_frame  *next;
// CP0 - insert by pairs, for double alignment
	unsigned int            cp0_context;
	unsigned int            cp0_cause;
	unsigned int            cp0_epc;
	unsigned int            cp0_srsctl;
	unsigned int            cp0_guestctl0;
	unsigned int            cp0_status;
	unsigned int            cp0_badvaddr;
	unsigned int            cp0_badinst;
	unsigned int            cp0_badinstp;
	unsigned int            cp0_nested_epc;
	unsigned int            cp0_nested_exc;
	unsigned int            lo;
	unsigned int            hi;
// short common block finishes here
// SP: extended exception frame starts here, used for VM0
	unsigned int    gpr[32];
};

struct thread {
	struct  exception_frame exfr;
	double              fpr[32];
	unsigned int        fcr31;
	unsigned int        dspcontrol;
	unsigned long       dspr[6];

	unsigned int        vmid;
	unsigned int        thread_flags;
	unsigned int        preempt_count;
	unsigned int        reschedule_count;
	unsigned int        waiting_irq_number;
	int        injected_irq;
	unsigned int        injected_ipl;
	int        interrupted_irq;
	unsigned int        last_interrupted_irq;
	unsigned int        interrupted_ipl;

	int        exception_cause;
	int        exception_gcause;

	unsigned int        cp0_guestctl0ext;
	unsigned int        cp0_guestctl1;
	unsigned int        cp0_guestctl2;
	unsigned int        cp0_guestctl3;
	unsigned int        cp0_gtoffset;

	struct  vmdev       vmdev;

	unsigned int        g_cp0_index;
	//// unsigned int        g_cp0_random;
	unsigned int        g_cp0_entrylo0;
	unsigned int        g_cp0_entrylo1;
	unsigned int        g_cp0_context;
	//unsigned int        g_cp0_contextconfig;
	unsigned int        g_cp0_userlocal;
	//// unsigned int        g_cp0_xcontextconfig;
	unsigned int        g_cp0_pagemask;
	unsigned int        g_cp0_pagegrain;
	//unsigned int        g_cp0_segctl0;
	//unsigned int        g_cp0_segctl1;
	//unsigned int        g_cp0_segctl2;
	//unsigned int        g_cp0_pwbase;
	//unsigned int        g_cp0_pwfield;
	//unsigned int        g_cp0_pwsize;
	unsigned int        g_cp0_wired;
	//unsigned int        g_cp0_pwctl;
	unsigned int        g_cp0_hwrena;
	unsigned int        g_cp0_badvaddr;
	unsigned int        g_cp0_badinst;
	unsigned int        g_cp0_badinstp;
	  unsigned int        g_cp0_count;
	unsigned int        g_cp0_entryhi;
	unsigned int        g_cp0_compare;
	unsigned int        g_cp0_status;
	unsigned int        g_cp0_intctl;
	  unsigned int        g_cp0_srsctl;
	  unsigned int        g_cp0_srsmap;
	  unsigned int        g_cp0_srsmap2;
	unsigned int        g_cp0_cause;
	  unsigned int        g_cp0_nested_exc;
	unsigned int        g_cp0_epc;
	unsigned int        g_cp0_nested_epc;
	  unsigned int        g_cp0_prid;
	unsigned int        g_cp0_ebase;
	  unsigned int        g_cp0_cdmmbase;
	//unsigned int        g_cp0_cmgcrbase;
	  unsigned int        g_cp0_config;
	  unsigned int        g_cp0_config1;
	  unsigned int        g_cp0_config2;
	  unsigned int        g_cp0_config3;
	  unsigned int        g_cp0_config4;
	  unsigned int        g_cp0_config5;
	  unsigned int        g_cp0_config6;
	  unsigned int        g_cp0_config7;
//      unsigned int        g_cp0_lladdr;
	//   unsigned int        g_cp0_maar[6];
	//   unsigned int        g_cp0_maari;
	//   unsigned int        g_cp0_watchlo;
	//   unsigned int        g_cp0_watchhi;
	//// unsigned int        g_cp0_xcontext;
	//  ... security ???
	  unsigned int        g_cp0_debug;
	  unsigned int        g_cp0_depc;
	  unsigned int        g_cp0_perfcnt[7];
	  unsigned int        g_cp0_errctl;
	  unsigned int        g_cp0_cacherr;
	  unsigned int        g_cp0_tagdatalo[4];
	  unsigned int        g_cp0_tagdatahi[4];
	unsigned int        g_cp0_errorepc;
	  unsigned int        g_cp0_desave;
	unsigned int        g_cp0_kscratch0;
	unsigned int        g_cp0_kscratch1;
	//unsigned int        g_cp0_kscratch[4];
	unsigned int        g_cp0_wired_hi[63];
	unsigned int        g_cp0_wired_lo0[63];
	unsigned int        g_cp0_wired_lo1[63];
	unsigned int        g_cp0_wired_pm[63];

	unsigned int        guest_read_count;
	unsigned long long  guest_read_time;
};
#define VM_FRAME_SIZE   ((sizeof(struct thread) + 0x1f) & ~0x1f)
//#define VM_FRAME_SIZE   ((sizeof(struct thread) + 0x7ff) & ~0x7ff)

#define THREAD_FLAGS_RUNNING    0x00000001
#define THREAD_FLAGS_STOPPED    0x00000002
#define THREAD_FLAGS_DEBUG      0x00000004
#define THREAD_FLAGS_EXCTRACE   0x00000008
#define THREAD_FLAGS_INTTRACE   0x00000010
#define THREAD_FLAGS_TIMETRACE  0x00000020

#define is_exc_trace()      ((current->thread_flags & (THREAD_FLAGS_DEBUG|THREAD_FLAGS_EXCTRACE)) == \
				(THREAD_FLAGS_DEBUG|THREAD_FLAGS_EXCTRACE))
#define is_int_trace()      ((current->thread_flags & (THREAD_FLAGS_DEBUG|THREAD_FLAGS_INTTRACE)) == \
				(THREAD_FLAGS_DEBUG|THREAD_FLAGS_INTTRACE))
#define is_time_trace()     ((current->thread_flags & (THREAD_FLAGS_DEBUG|THREAD_FLAGS_TIMETRACE)) == \
				(THREAD_FLAGS_DEBUG|THREAD_FLAGS_TIMETRACE))

register struct thread * current __asm__("$30");

static void inline preempt_disable(void)
{
	current->preempt_count++;
}

static void inline preempt_enable(void)
{
	current->preempt_count--;
}

extern unsigned int fpu_owner;
extern unsigned int dsp_owner;

#define is_fpu_owner()  (fpu_owner == current->vmid)
#define fpu_get_fcr31() read_cp1_fcr31()

extern int const vm_list[];
extern int const vm_options[];
#define VM_OPTION_POLLING       0x00000001

extern unsigned long vm0_thread;
#define get_vm_fp(vm)   ((struct thread *)(vm0_thread - ((vm) * VM_FRAME_SIZE)))
extern void IRQ_wait_exit(void);
extern unsigned int reschedule_flag;
extern unsigned int reschedule_vm;

extern void switch_to_vm(struct exception_frame *exfr, unsigned int vmid);

#endif
