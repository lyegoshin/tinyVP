#include    "irq.h"
#include    "uart.h"
#include    "stdio.h"
#include    "mips.h"
#include    "time.h"
#include    "thread.h"
#include    "console.h"

//struct uart_device *const uad = (struct uart_device *)_UART4ADDR;
//unsigned long upaddr = _UART4PADDR;
//unsigned short uad_irq_tx = _UART4_IRQ_TX;
//unsigned short uad_irq_rx = _UART4_IRQ_RX;

//struct uart_device *const uad;
//unsigned short const console_irq_tx;
//unsigned short const console_irq_rx;

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
uart_writeline(console_uart, str);
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

	uart_init(console_uart);

	bzero((unsigned long)current, sizeof(struct thread));

	sprintf(str,"Status=0x%08x, Cause=0x%08x, gp=0x%08x sp=0x%08x console_irq_rx=%d console_irq_tx=%d\n",a1,a2,a3,a4,console_irq_rx,console_irq_tx);
	uart_writeline(console_uart, str);

	init_IRQ();
	irq_set_prio_and_unmask(console_irq_rx, _CONSOLE_PRIO);
	init_SBA();

	// Finish a board specific initialization
	_board_init_end();
	clear_cp0_status(CP0_STATUS_BEV);

	init_time();
	write_time_values();
	init_scheduler();

	ei();

	uart_writeline(console_uart, "After init_IRQ...\n");

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
	printf("PRECON=%08x PRESTAT=%08x\n",*(volatile unsigned int*)PRECON, *(volatile unsigned int*)PRESTAT);
	printf("PMD1=%08x, PMD2=%08x, PMD3=%08x\n",
		*(volatile unsigned int*)0xbf800040, *(volatile unsigned int*)0xbf800050,
		*(volatile unsigned int*)0xbf800060);
	printf("PMD4=%08x, PMD5=%08x, PMD6=%08x, PMD7=%08x\n",
		*(volatile unsigned int*)0xbf800070, *(volatile unsigned int*)0xbf800080,
		*(volatile unsigned int*)0xbf800090, *(volatile unsigned int*)0xbf8000a0);
	printf("CFGPG=%08x, CFGCON=%08x\n",
		*(volatile unsigned int*)0xbf8000E0, *(volatile unsigned int*)0xbf800000);
	printf("SBT2REG0=%08x, SBT2RD0=%08x, SBT2WR0=%08x\n",
		*(volatile unsigned int*)0xbf8f8840, *(volatile unsigned int*)0xbf8f8850, *(volatile unsigned int*)0xbf8f8858);
	printf("SBT2REG1=%08x, SBT2RD1=%08x, SBT2WR1=%08x\n",
		*(volatile unsigned int*)0xbf8f8860, *(volatile unsigned int*)0xbf8f8870, *(volatile unsigned int*)0xbf8f8878);
	printf("SBT2REG2=%08x, SBT2RD2=%08x, SBT2WR2=%08x\n",
		*(volatile unsigned int*)0xbf8f8880, *(volatile unsigned int*)0xbf8f8890, *(volatile unsigned int*)0xbf8f8898);
uart_start_console();
#if 0
//*(volatile unsigned int*)0xbf8f8850 = 7;
//*(volatile unsigned int*)0xbf8f8858 = 7;
*(volatile unsigned int*)0xbf8f8870 = 7;
*(volatile unsigned int*)0xbf8f8878 = 7;
*(volatile unsigned int*)0xbf8f8890 = 6;
*(volatile unsigned int*)0xbf8f8898 = 6;
//*(volatile unsigned int*)0xbf8f8840 = 0x0;
*(volatile unsigned int*)0xbf8f8860 = 0x30;
*(volatile unsigned int*)0xbf8f8880 = 0x80;
	printf(" SBT2REG0=%08x, SBT2RD0=%08x, SBT2WR0=%08x\n",
		*(volatile unsigned int*)0xbf8f8840, *(volatile unsigned int*)0xbf8f8850, *(volatile unsigned int*)0xbf8f8858);
	printf(" SBT2REG1=%08x, SBT2RD1=%08x, SBT2WR1=%08x\n",
		*(volatile unsigned int*)0xbf8f8860, *(volatile unsigned int*)0xbf8f8870, *(volatile unsigned int*)0xbf8f8878);
	printf(" SBT2REG2=%08x, SBT2RD2=%08x, SBT2WR2=%08x\n",
		*(volatile unsigned int*)0xbf8f8880, *(volatile unsigned int*)0xbf8f8890, *(volatile unsigned int*)0xbf8f8898);
uart_start_console();
#endif
	printf("SBT3REG0=%08x, SBT3RD0=%08x, SBT3WR0=%08x\n",
		*(volatile unsigned int*)0xbf8f8c40, *(volatile unsigned int*)0xbf8f8c50, *(volatile unsigned int*)0xbf8f8c58);
	printf("SBT3REG1=%08x, SBT3RD1=%08x, SBT3WR1=%08x\n",
		*(volatile unsigned int*)0xbf8f8c60, *(volatile unsigned int*)0xbf8f8c70, *(volatile unsigned int*)0xbf8f8c78);
	printf("SBT3REG2=%08x, SBT3RD2=%08x, SBT3WR2=%08x\n",
		*(volatile unsigned int*)0xbf8f8c80, *(volatile unsigned int*)0xbf8f8c90, *(volatile unsigned int*)0xbf8f8c98);
uart_start_console();
#if 0
*(volatile unsigned int*)0xbf8f8c50 = 1;
*(volatile unsigned int*)0xbf8f8c58 = 1;
*(volatile unsigned int*)0xbf8f8c70 = 6;
*(volatile unsigned int*)0xbf8f8c78 = 6;
*(volatile unsigned int*)0xbf8f8c40 = 0x40048;
*(volatile unsigned int*)0xbf8f8c60 = 0x40048;
	printf(" SBT3REG0=%08x, SBT3RD0=%08x, SBT3WR0=%08x\n",
		*(volatile unsigned int*)0xbf8f8c40, *(volatile unsigned int*)0xbf8f8c50, *(volatile unsigned int*)0xbf8f8c58);
	printf(" SBT3REG1=%08x, SBT3RD1=%08x, SBT3WR1=%08x\n",
		*(volatile unsigned int*)0xbf8f8c60, *(volatile unsigned int*)0xbf8f8c70, *(volatile unsigned int*)0xbf8f8c78);
	printf(" SBT3REG2=%08x, SBT3RD2=%08x, SBT3WR2=%08x\n",
		*(volatile unsigned int*)0xbf8f8c80, *(volatile unsigned int*)0xbf8f8c90, *(volatile unsigned int*)0xbf8f8c98);
uart_start_console();
#endif
#if 0
*(volatile unsigned int*)0xbf8f9058 = 0;
*(volatile unsigned int*)0xbf8f9040 = 0x20000048;
*(volatile unsigned int*)0xbf8f9060 = 0;
*(volatile unsigned int*)0xbf8f9070 = 7;
#endif
	printf("SBT4REG0=%08x, SBT4RD0=%08x, SBT4WR0=%08x\n",
		*(volatile unsigned int*)0xbf8f9040, *(volatile unsigned int*)0xbf8f9050, *(volatile unsigned int*)0xbf8f9058);
	printf("SBT4REG1=%08x, SBT4RD1=%08x, SBT4WR1=%08x\n",
		*(volatile unsigned int*)0xbf8f9060, *(volatile unsigned int*)0xbf8f9070, *(volatile unsigned int*)0xbf8f9078);
	printf("SBT4REG2=%08x, SBT4RD2=%08x, SBT4WR2=%08x\n",
		*(volatile unsigned int*)0xbf8f9080, *(volatile unsigned int*)0xbf8f9090, *(volatile unsigned int*)0xbf8f9098);
uart_start_console();

	do {
//                printf("-------- [%T] ---------\n",current_wall_time);
//                uart_writeline(console_uart, "-----------------\n");
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
			uart_writeline(console_uart, str);
			sprintf(str,"Guest: EPC=%08x Status=%08x Cause=%08x BVA=%08x BI=%08x; InjectedIRQ=%d TimeLateCnt=%d\n",
			    next->g_cp0_epc,next->g_cp0_status,next->g_cp0_cause,next->g_cp0_badvaddr,next->g_cp0_badinst,next->injected_irq,next->time_late_counter);
			uart_writeline(console_uart, str);
			sprintf(str,"LastIRQ=%d %d %d %d; Cause=%d %d %d %d; GCause=%d %d %d %d\n",
			  (int)(next->last_interrupted_irq & 0xff),(int)((next->last_interrupted_irq >> 8) & 0xff),(int)((next->last_interrupted_irq >> 16) & 0xff),(int)((next->last_interrupted_irq >> 24) & 0xff),
			  (int)(next->exception_cause & 0xff),(int)((next->exception_cause >> 8) & 0xff),(int)((next->exception_cause >> 16) & 0xff),(int)((next->exception_cause >> 24) & 0xff),
			  (int)(next->exception_gcause & 0xff),(int)((next->exception_gcause >> 8) & 0xff),(int)((next->exception_gcause >> 16) & 0xff),(int)((next->exception_gcause >> 24) & 0xff));
			uart_writeline(console_uart, str);
		    }
		    break;

		case 's':
		    if (vmid) {
			next->thread_flags &= ~THREAD_FLAGS_RUNNING;
			next->thread_flags |= THREAD_FLAGS_STOPPED;
			sprintf(str,"\nVM%d: stopped\n",vmid);
			uart_writeline(console_uart, str);
		    }
		    break;

		case 'c':
		    if (vmid) {
			next->thread_flags &= ~THREAD_FLAGS_STOPPED;
			next->thread_flags |= THREAD_FLAGS_RUNNING;
			sprintf(str,"\nVM%d: continued\n",vmid);
			uart_writeline(console_uart, str);
		    }
		    break;

		case 'r':
		    if (vmid) {
			next->thread_flags &= ~THREAD_FLAGS_STOPPED;
			init_vm(vmid, next->thread_flags & THREAD_FLAGS_DEBUG? next->thread_flags : 0);
			sprintf(str,"\nVM%d: restarted\n",vmid);
			uart_writeline(console_uart, str);
		    }
		    break;

		case 'd':
		    if (vmid) {
			next->thread_flags ^= THREAD_FLAGS_DEBUG;
			sprintf(str,"\nVM%d: debug %s\n",vmid, next->thread_flags & THREAD_FLAGS_DEBUG? "on" : "off");
			uart_writeline(console_uart, str);
		    }
		    break;

		case 'e':
		    if (vmid) {
			next->thread_flags ^= THREAD_FLAGS_EXCTRACE;
			sprintf(str,"\nVM%d: exception trace %s\n",vmid, next->thread_flags & THREAD_FLAGS_EXCTRACE? "on" : "off");
			uart_writeline(console_uart, str);
		    }
		    break;

		case 'i':
		    if (vmid) {
			next->thread_flags ^= THREAD_FLAGS_INTTRACE;
			sprintf(str,"\nVM%d: interrupt trace %s\n",vmid, next->thread_flags & THREAD_FLAGS_INTTRACE? "on" : "off");
			uart_writeline(console_uart, str);
		    }
		    break;

		case 't':
		    if (vmid) {
			next->thread_flags ^= THREAD_FLAGS_TIMETRACE;
			sprintf(str,"\nVM%d: time trace %s\n",vmid, next->thread_flags & THREAD_FLAGS_TIMETRACE? "on" : "off");
			uart_writeline(console_uart, str);
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



