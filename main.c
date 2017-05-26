#include    "irq.h"
#include    "uart.h"
#include    "stdio.h"
#include    "mips.h"
#include    "time.h"
#include    "thread.h"
#include    "console.h"

struct uart_device *const uad = (struct uart_device *)_UART4ADDR;
unsigned long upaddr = _UART4PADDR;
unsigned short uad_irq_tx = _UART4_IRQ_TX;
unsigned short uad_irq_rx = _UART4_IRQ_RX;

struct uart_device *console_uart;
unsigned long console_paddr;
unsigned short console_irq_tx;
unsigned short console_irq_rx;

extern volatile unsigned long long current_wall_time;

struct timer tmp_timer;

void    tmp_prt(void)
{
	register unsigned long sp __asm__("$29");
	register unsigned long ra __asm__("$31");
	static int cnt = 0;
	unsigned long *p = (unsigned long*)&current_wall_time;
char str[128];
sprintf(str,"\nStatus=0x%08x, Cause=0x%08x sp=%08x ra=%08x\n",read_cp0_status(),read_cp0_cause(),sp,ra);
uart_writeline(uad, str);
	printf("|------- [%T] --------|\n",current_wall_time);
	timer_request(&tmp_timer, TIME_SECOND*2, tmp_prt);
}


void    __main(/* void */ unsigned int a1, unsigned int a2, unsigned int a3, unsigned int a4)
{
	volatile int i;
	volatile int j;
	char str[256];
	static int once = 0;
	static int vmid = 0;

	console_uart = uad;
	console_paddr = upaddr;
	console_irq_tx = uad_irq_tx;
	console_irq_rx = uad_irq_rx;
	uart_init(console_uart);

	bzero((unsigned long)current, sizeof(struct thread));

	sprintf(str,"Status=0x%08x, Cause=0x%08x, gp=0x%08x sp=0x%08x uad_irq_tx=%d console_irq_tx=%d\n",a1,a2,a3,a4,uad_irq_tx,console_irq_tx);
	uart_writeline(uad, str);

	init_IRQ();
	irq_set_prio_and_unmask(console_irq_rx, _CONSOLE_PRIO);
	init_time();
	init_scheduler();

	ei();

	uart_writeline(uad, "After init_IRQ...\n");

	printf("It is printf\n");
		__asm__ __volatile__("mfc0  %0, $12, 1"
				    : "=&r" (i)::);
		__asm__ __volatile__(".set virt\nmfgc0  %0, $12, 1"
				    : "=&r" (j)::);
		printf("IntCtl=0x%08x\tG.IntCtl=0x%08x\n",i,j);
	printf("DEVCFG3=%08x, DEVCFG2=%08x, DEVCFG1=%08x, DEVCFG0=%08x\n",
		*(volatile unsigned int*)0xbfc0FFC0, *(volatile unsigned int*)0xbfc0FFC4,
		*(volatile unsigned int*)0xbfc0FFC8, *(volatile unsigned int*)0xbfc0FFCC);
	printf("ADEVCFG3=%08x, ADEVCFG2=%08x, ADEVCFG1=%08x, ADEVCFG0=%08x\n",
		*(volatile unsigned int*)0xbfc0FF40, *(volatile unsigned int*)0xbfc0FF44,
		*(volatile unsigned int*)0xbfc0FF48, *(volatile unsigned int*)0xbfc0FF4C);
	printf("BF1: DEVCFG3=%08x, DEVCFG2=%08x, DEVCFG1=%08x, DEVCFG0=%08x, ABF1SEQ0=%08x, BF1SEQ0=%08x\n",
		*(volatile unsigned int*)0xbfc4FFC0, *(volatile unsigned int*)0xbfc4FFC4,
		*(volatile unsigned int*)0xbfc4FFC8, *(volatile unsigned int*)0xbfc4FFCC,
		*(volatile unsigned int*)0xbfc4FF7C, *(volatile unsigned int*)0xbfc4FFFC);
	printf("BF2: DEVCFG3=%08x, DEVCFG2=%08x, DEVCFG1=%08x, DEVCFG0=%08x, ABF2SEQ0=%08x, BF2SEQ0=%08x\n",
		*(volatile unsigned int*)0xbfc6FFC0, *(volatile unsigned int*)0xbfc6FFC4,
		*(volatile unsigned int*)0xbfc6FFC8, *(volatile unsigned int*)0xbfc6FFCC,
		*(volatile unsigned int*)0xbfc6FF7C, *(volatile unsigned int*)0xbfc6FFFC);
	printf("PMD1=%08x, PMD2=%08x, PMD3=%08x\n",
		*(volatile unsigned int*)0xbf800040, *(volatile unsigned int*)0xbf800050,
		*(volatile unsigned int*)0xbf800060);
	printf("PMD4=%08x, PMD5=%08x, PMD6=%08x, PMD7=%08x\n",
		*(volatile unsigned int*)0xbf800070, *(volatile unsigned int*)0xbf800080,
		*(volatile unsigned int*)0xbf800090, *(volatile unsigned int*)0xbf8000a0);

	do {
//                printf("-------- [%T] ---------\n",current_wall_time);
//                uart_writeline(uad, "-----------------\n");
		if (!once++)
			timer_request(&tmp_timer, TIME_SECOND, tmp_prt);

	    wait();
	    if (console_has_next_char(0)) {
		unsigned int ie;
		unsigned char c = console_next_char(0);
		struct thread *next  = get_vm_fp(vmid);

		im_up(ie);

		switch (c & 0x7f) {
		case 'T':
		    dump_tlb();
		    break;

		case 'q':
		    if (vmid) {
			sprintf(str,"\nVM%d: EPC=%08x SRSctl=%08x Context=%08x gc2=%08x Cause=%08x BVA=%08x BI=%08x; thread_flags=%08x\n",
			    vmid,next->exfr.cp0_epc,next->exfr.cp0_srsctl,next->exfr.cp0_context,next->cp0_guestctl2,next->exfr.cp0_cause,
			    next->exfr.cp0_badvaddr,next->exfr.cp0_badinst,next->thread_flags);
			uart_writeline(uad, str);
			sprintf(str,"Guest: EPC=%08x Status=%08x Cause=%08x BVA=%08x BI=%08x; InjectedIRQ=%d\n",
			    next->g_cp0_epc,next->g_cp0_status,next->g_cp0_cause,next->g_cp0_badvaddr,next->g_cp0_badinst,next->injected_irq);
			uart_writeline(uad, str);
			sprintf(str,"LastIRQ=%d %d %d %d; Cause=%d %d %d %d; GCause=%d %d %d %d\n",
			  (int)(next->last_interrupted_irq & 0xff),(int)((next->last_interrupted_irq >> 8) & 0xff),(int)((next->last_interrupted_irq >> 16) & 0xff),(int)((next->last_interrupted_irq >> 24) & 0xff),
			  (int)(next->exception_cause & 0xff),(int)((next->exception_cause >> 8) & 0xff),(int)((next->exception_cause >> 16) & 0xff),(int)((next->exception_cause >> 24) & 0xff),
			  (int)(next->exception_gcause & 0xff),(int)((next->exception_gcause >> 8) & 0xff),(int)((next->exception_gcause >> 16) & 0xff),(int)((next->exception_gcause >> 24) & 0xff));
			uart_writeline(uad, str);
		    }
		    break;

		case 's':
		    if (vmid) {
			next->thread_flags &= ~THREAD_FLAGS_RUNNING;
			next->thread_flags |= THREAD_FLAGS_STOPPED;
			sprintf(str,"\nVM%d: stopped\n",vmid);
			uart_writeline(uad, str);
		    }
		    break;

		case 'c':
		    if (vmid) {
			next->thread_flags &= ~THREAD_FLAGS_STOPPED;
			next->thread_flags |= THREAD_FLAGS_RUNNING;
			sprintf(str,"\nVM%d: continued\n",vmid);
			uart_writeline(uad, str);
		    }
		    break;

		case 'r':
		    if (vmid) {
			next->thread_flags &= ~THREAD_FLAGS_STOPPED;
			init_vm(vmid, next->thread_flags & THREAD_FLAGS_DEBUG? next->thread_flags : 0);
			sprintf(str,"\nVM%d: restarted\n",vmid);
			uart_writeline(uad, str);
		    }
		    break;

		case 'd':
		    if (vmid) {
			next->thread_flags ^= THREAD_FLAGS_DEBUG;
			sprintf(str,"\nVM%d: debug %s\n",vmid, next->thread_flags & THREAD_FLAGS_DEBUG? "on" : "off");
			uart_writeline(uad, str);
		    }
		    break;

		case 'e':
		    if (vmid) {
			next->thread_flags ^= THREAD_FLAGS_EXCTRACE;
			sprintf(str,"\nVM%d: exception trace %s\n",vmid, next->thread_flags & THREAD_FLAGS_EXCTRACE? "on" : "off");
			uart_writeline(uad, str);
		    }
		    break;

		case 'i':
		    if (vmid) {
			next->thread_flags ^= THREAD_FLAGS_INTTRACE;
			sprintf(str,"\nVM%d: interrupt trace %s\n",vmid, next->thread_flags & THREAD_FLAGS_INTTRACE? "on" : "off");
			uart_writeline(uad, str);
		    }
		    break;

		case 't':
		    if (vmid) {
			next->thread_flags ^= THREAD_FLAGS_TIMETRACE;
			sprintf(str,"\nVM%d: time trace %s\n",vmid, next->thread_flags & THREAD_FLAGS_TIMETRACE? "on" : "off");
			uart_writeline(uad, str);
		    }
		    break;

		case 'I':
		    if (vmid)
			ic_print(next);
		    break;

		case '0'...'7':
		    vmid = (c & 0x7f) - '0';
		    break;
		}

		im_down(ie);
	    }
	} while (1);
}



