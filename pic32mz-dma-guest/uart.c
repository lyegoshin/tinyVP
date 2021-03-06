/*
 * Include processor definitions.
 */
#include "pic32mz.h"

#define MHZ     200             /* CPU clock. */

/*
 * Chip configuration.
 */
#if 0
PIC32_DEVCFG (
    DEVCFG0_JTAG_DISABLE |      /* Disable JTAG port */
    DEVCFG0_TRC_DISABLE,        /* Disable trace port */
#if 0
    /* Case #1: using internal fast RC oscillator.
     * The frequency is around 8 MHz.
     * PLL multiplies it to 200 MHz. */
    DEVCFG1_FNOSC_SPLL |        /* System clock supplied by SPLL */
    DEVCFG1_POSCMOD_DISABLE |   /* Primary oscillator disabled */
    DEVCFG1_CLKO_DISABLE,       /* CLKO output disable */

    DEVCFG2_FPLLIDIV_1 |        /* PLL input divider = 1 */
    DEVCFG2_FPLLRNG_5_10 |      /* PLL input range is 5-10 MHz */
    DEVCFG2_FPLLICLK_FRC |      /* Select FRC as input to PLL */
    DEVCFG2_FPLLMULT(50) |      /* PLL multiplier = 50x */
    DEVCFG2_FPLLODIV_2,         /* PLL postscaler = 1/2 */
#endif
#if 1
    /* Case #2: using primary oscillator with external crystal 24 MHz.
     * PLL multiplies it to 200 MHz. */
    DEVCFG1_FNOSC_SPLL |        /* System clock supplied by SPLL */
    DEVCFG1_POSCMOD_EXT |       /* External generator */
    DEVCFG1_FCKS_ENABLE |       /* Enable clock switching */
    DEVCFG1_FCKM_ENABLE |       /* Enable fail-safe clock monitoring */
    DEVCFG1_IESO |              /* Internal-external switch over enable */
    DEVCFG1_CLKO_DISABLE,       /* CLKO output disable */

    DEVCFG2_FPLLIDIV_3 |        /* PLL input divider = 3 */
    DEVCFG2_FPLLRNG_5_10 |      /* PLL input range is 5-10 MHz */
    DEVCFG2_FPLLMULT(50) |      /* PLL multiplier = 50x */
    DEVCFG2_FPLLODIV_2,         /* PLL postscaler = 1/2 */
#endif
    DEVCFG3_FETHIO |            /* Default Ethernet pins */
    DEVCFG3_USERID(0xffff));    /* User-defined ID */
#endif

/*
 * Boot code at bfc00000.
 * Setup stack pointer and $gp registers, and jump to main().
 */
asm ("          .section .exception");
asm ("          .globl _start");
asm ("          .type _start, function");
asm ("_start:   la      $sp, _estack");
asm ("          la      $ra, main");
asm ("          la      $gp, _gp");
asm ("          jr      $ra");
asm ("           nop       ");

asm ("          .org    0x180");
asm ("          .word 0x42008028");
asm ("          .org    0x1d0");

//asm ("1:          b    1b");
//asm ("          .word 0x42000028");

asm ("          la      $k0, 0xbf822600");
asm ("          lw      $k1, 0x10($k0)");
asm ("          rotr    $k1, 1");
asm ("          bgez    $k1, _return");
asm ("           nop       ");
asm ("          lw      $k1, 0x30($k0)");
asm ("          la      $k0, rxsymbol");
asm ("          sw      $k1, 0($k0)");
asm ("          la      $k0, 0xbf810094");
asm ("          li      $k1, 0x0800");
asm ("          sw      $k1, 0($k0)");
asm ("_return:          ");
//asm ("          .word 0x42000028");
asm ("          eret       ");
asm ("           nop       ");

asm ("          .text");
asm ("_estack   = _end + 0x1000");

void ic_voff_compare_set(unsigned off)
{
	*(volatile unsigned int *)0xbf810540 = off;
}

void ic_voff_u4rx_set(unsigned off)
{
	*(volatile unsigned int *)0xbf8107ec = off;
}

void ic_voff_dma0_set(unsigned off)
{
	OFF(134) = off;
}

void ic_iec_compare_set(void)
{
	*(volatile unsigned int *)0xbf8100c8 = 1;
}

void ic_iec_u4rx_set(void)
{
	*(volatile unsigned int *)0xbf810118 = 0x00000800;
}

void ic_ifs_compare_clr(void)
{
	*(volatile unsigned int *)0xbf810044 = 1;
}

void ic_ifs_u4rx_clr(void)
{
	*(volatile unsigned int *)0xbf810094 = 0x00000800;
}

unsigned ic_ifs_dma0(void)
{
	return IFS4 & 0x00000040;
}

void ic_ifs_dma0_clr(void)
{
	IFSCLR(4) = 0x00000040;
}

void ic_ipc_compare_set(unsigned prio)
{
	*(volatile unsigned int *)0xbf810140 = prio << 2;
}

void ic_ipc_u4rx_set(unsigned prio)
{
	*(volatile unsigned int *)0xbf8103e8 = prio << (2 + 24);
}

void ic_ipc_dma0_set(unsigned prio)
{
	IPC33 = prio << (2 + 16);
}

volatile unsigned loop;

volatile unsigned int symbol;
volatile unsigned int rxsymbol;

void printreg (const char *p, unsigned val);
void putch (unsigned char c);

/*
 * Delay for a given number of microseconds.
 * The processor has a 32-bit hardware Count register,
 * which increments at half CPU rate.
 * We use it to get a precise delay.
 */
void udelay (unsigned usec)
{
    unsigned now = mfc0 (C0_COUNT, 0);
    unsigned final = now + usec * MHZ / 2;
    unsigned cause;

    ic_iec_compare_set();
////    ic_iec_u4rx_set();
    mtc0(C0_COMPARE, 0, final);
//lp:
    __asm__ __volatile__ ("ehb\nwait");
    if (U4STA & PIC32_USTA_URXDA) {
	symbol = U4RXREG;
//        ic_ifs_u4rx_clr();
    }
    if (ic_ifs_dma0()) {
	cause = DCH0INT;
	ic_ifs_dma0_clr();
//        printreg("@DCH0INT  ", DCH0INT);
    }
    cause = mfc0(C0_CAUSE, 0);
    if (cause & 0x40000000)
	ic_ifs_compare_clr();
//    else
//        goto lp;

#if 0
lp:
    __asm__ __volatile__ ("ehb\nwait");
    __asm__ __volatile__ ("ei\nehb");
    __asm__ __volatile__ ("nop");
    __asm__ __volatile__ ("di\nehb");
    if (rxsymbol & 0xFF) {
	symbol = rxsymbol & 0xFF;
	rxsymbol = 0;
//        ic_ifs_u4rx_clr();
    }
    cause = mfc0(C0_CAUSE, 0);
    if (cause & 0x40000000)
	ic_ifs_compare_clr();
    else
	goto lp;
#endif
#if 0
    for (;;) {
	now = mfc0 (C0_COUNT, 0);

	/* This comparison is valid only when using a signed type. */
	if ((int) (now - final) >= 0)
	    break;
    }
#endif
}

/*
 * Send a byte to the UART transmitter, with interrupts disabled.
 */
void putch (unsigned char c)
{
    /* Wait for transmitter shift register empty. */
    while (! (U4STA & PIC32_USTA_TRMT))
	continue;

again:
    /* Send byte. */
    U4TXREG = c;

    /* Wait for transmitter shift register empty. */
    while (! (U4STA & PIC32_USTA_TRMT))
	continue;

    if (c == '\n') {
	c = '\r';
	goto again;
    }
}

/*
 * Wait for the byte to be received and return it.
 */
unsigned getch (void)
{
    unsigned c;

    for (;;) {
	/* Wait until receive data available. */
	if (U4STA & PIC32_USTA_URXDA) {
	    c = (unsigned char) U4RXREG;
	    break;
	}
    }
    return c;
}

int hexchar (unsigned val)
{
    return "0123456789abcdef" [val & 0xf];
}

void printreg (const char *p, unsigned val)
{
    for (; *p; ++p)
	putch (*p);

    putch ('=');
    putch (' ');
    putch (hexchar (val >> 28));
    putch (hexchar (val >> 24));
    putch (hexchar (val >> 20));
    putch (hexchar (val >> 16));
    putch (hexchar (val >> 12));
    putch (hexchar (val >> 8));
    putch (hexchar (val >> 4));
    putch (hexchar (val));
    putch ('\n');
}

void dma_start(void)
{
int i;
	/* This code example illustrates the DMA channel 0 configuration for a data transfer. */
//        IECCLR(1)=0x00010000;  // disable DMA channel 0 interrupts

#if 0
printreg("IEC(4)    ",IEC4);
printreg("IFS(4)    ",IFS4);
printreg("DMACON    ",DMACON);
printreg("DCH0ECON  ",DCH0ECON);
printreg("DCH0CON   ",DCH0CON);
printreg("DCH0SSA   ",DCH0SSA);
printreg("DCH0DSA   ",DCH0DSA);
printreg("DCH0SSIZ  ",DCH0SSIZ);
printreg("DCH0DSIZ  ",DCH0DSIZ);
printreg("DCH0CSIZ  ",DCH0CSIZ);
printreg("DCH0INT   ",DCH0INT);
printreg("DCH0SPTR  ",DCH0SPTR);
printreg("DCH0DPTR  ",DCH0DPTR);
printreg("DCH0CPTR  ",DCH0CPTR);
printreg("DMASTAT   ",DMASTAT);
printreg("DMAADDR   ",DMAADDR);
#endif

DCH0ECONSET=0x00000040; // set ABORT to 1
DCH0CONCLR = 0x00000080;                // turn channel off

do {
    i = DCH0CON;
//    printreg("DCH0CON   ",i);
    if (i & 0x00008000)
	for (i=0; i<100; i++) ;
    else
	break;
} while(1);

	IECSET(4) = 0x00000040;
	IFSCLR(4) = 0x00000040;

//    DMACON=0x00008000;
	DMACONSET=0x00008000;
//       DCH0ECON = 0x40;
	DCH0CON=0x3;
	DCH0ECON=0;

	// channel off, priority 3, no chaining
	// no start or stop IRQs, no pattern match
	// program the transfer
	DCH0SSA=0x00000000;     // RAM 0,   source physaddr
	DCH0DSA=0x00008000;     // RAM 32K, dest physaddr. Change to 64K for DMA violation
	DCH0SSIZ=0x4000;                // source size in bytes
	DCH0DSIZ=0x4000;                // dest size in bytes
	DCH0CSIZ=0x4000;                // 16K bytes transferred per event
	DCH0INTCLR=0x00ff00ff;          // clear existing events, disable all interrupts
	DCH0INTSET = 0x00210000;
	DCH0CONSET = 0x00000080;                // turn channel on

#if 0
printreg("DMACON    ",DMACON);
printreg("DCH0ECON  ",DCH0ECON);
printreg("DCH0CON   ",DCH0CON);
printreg("DCH0SSA   ",DCH0SSA);
printreg("DCH0DSA   ",DCH0DSA);
printreg("DCH0SSIZ  ",DCH0SSIZ);
printreg("DCH0DSIZ  ",DCH0DSIZ);
printreg("DCH0CSIZ  ",DCH0CSIZ);
printreg("DCH0INT   ",DCH0INT);
printreg("DCH0SPTR  ",DCH0SPTR);
printreg("DCH0DPTR  ",DCH0DPTR);
printreg("DCH0CPTR  ",DCH0CPTR);
printreg("DMASTAT   ",DMASTAT);
printreg("DMAADDR   ",DMAADDR);
#endif
	// initiate a transfer
	DCH0ECONSET=0x00000080; // set CFORCE to 1

#if 0
	// do something else
	// poll to see that the transfer was done
	while(TRUE)
	{
		register int pollCnt;
		int dmaFlags=DCH0INT;
		if( (dmaFlags&0xb)
		{
			break;
		}

		pollCnt=100;
		while(pollCnt--);
		// use a poll counter.
		// continuously polling the DMA controller in a tight
		// loop would affect the performance of the DMA transfer
		// one of CHERIF (DCHxINT<0>), CHTAIF (DCHxINT<1>)
		// or CHBCIF (DCHxINT<3>) flags set
		// transfer completed
		// use an adjusted value here
		// wait before reading again the DMA controller
	}
	// check the transfer completion result
#endif
}

int main()
{
    /* Initialize coprocessor 0. */
    mtc0 (C0_COUNT, 0, 0);
    mtc0 (C0_COMPARE, 0, -1);
 mtc0 (C0_EBASE, 1, 0x9fc00000);     /* Vector base */
 mtc0 (C0_INTCTL, 1, 1 << 5 );        /* Vector spacing 32 bytes */
 mtc0 (C0_CAUSE, 0, 1 << 23 );        /* Set IV, 0x200 for IRQ */
 mtc0 (C0_STATUS, 0, 0x10000000);             /* Clear BEV, EXL and IE */

    /* Use pins PA0-PA3, PF13, PF12, PA6-PA7 as output: LED control. */
    //LATACLR = 0xCF;
    //TRISACLR = 0xCF;
    //LATFCLR = 0x3000;
    //TRISFCLR = 0x3000;

    /* Initialize UART. */
    //U4RXR = 2;                          /* assign UART1 receive to pin RPF4 */
    //RPF5R = 1;                          /* assign pin RPF5 to UART1 transmit */

    U4BRG = PIC32_BRG_BAUD (MHZ * 500000, 115200);
    U4STA = 0;
    U4MODE = PIC32_UMODE_PDSEL_8NPAR |      /* 8-bit data, no parity */
	     PIC32_UMODE_ON;                /* UART Enable */
    U4STASET = PIC32_USTA_URXEN |           /* Receiver Enable */
	       PIC32_USTA_UTXEN;            /* Transmit Enable */

    /*
     * Print initial state of control registers.
     */
    putch ('-');
    putch ('\n');
#if 0
    printreg ("Status  ", mfc0(12, 0));
    printreg ("IntCtl  ", mfc0(12, 1));
    printreg ("SRSCtl  ", mfc0(12, 2));
    printreg ("Cause   ", mfc0(13, 0));
    printreg ("PRId    ", mfc0(15, 0));
    printreg ("EBase   ", mfc0(15, 1));
    printreg ("CDMMBase", mfc0(15, 2));
    printreg ("Config  ", mfc0(16, 0));
    printreg ("Config1 ", mfc0(16, 1));
    printreg ("Config2 ", mfc0(16, 2));
    printreg ("Config3 ", mfc0(16, 3));
    printreg ("Config4 ", mfc0(16, 4));
    printreg ("Config5 ", mfc0(16, 5));
    printreg ("Config7 ", mfc0(16, 7));
    printreg ("WatchHi ", mfc0(19, 0));
    printreg ("WatchHi1", mfc0(19, 1));
    printreg ("WatchHi2", mfc0(19, 2));
    printreg ("WatchHi3", mfc0(19, 3));
    printreg ("Debug   ", mfc0(23, 0));
    printreg ("PerfCtl0", mfc0(25, 0));
    printreg ("PerfCtl1", mfc0(25, 2));
#endif
    printreg ("DEVID   ", DEVID);
    DEVID = 0;
    printreg ("OSCCON  ", OSCCON);
    printreg ("DEVCFG0 ", DEVCFG0);
    printreg ("DEVCFG1 ", DEVCFG1);
    printreg ("DEVCFG2 ", DEVCFG2);
    printreg ("DEVCFG3 ", DEVCFG3);
    printreg ("CFGCON  ", CFGCON);
    printreg ("Config3 ", mfc0(16, 3));
    printreg ("Config7 ", mfc0(16, 7));

    LATBCLR = (1 << 11);
    TRISBCLR = (1 << 11);
    LATDCLR = (1 << 4);
    TRISDCLR = (1 << 4);
    LATGCLR = (1 << 15)|(1 << 6);
    TRISGCLR = (1 << 15)|(1 << 6);
    LATGCLR = (1 << 6);
    TRISGCLR = (1 << 6);

    ic_ipc_compare_set(3);
//    ic_ipc_u4rx_set(3);
    ic_voff_compare_set(0x200);
//    ic_voff_u4rx_set(0x200);
    symbol = '#';
    ic_ipc_dma0_set(3);
    ic_voff_dma0_set(0x200);

    while (1) {
//        LATBINV = (1 << 11);  udelay (100000);
//        LATDINV = (1 << 4);  udelay (100000);
	dma_start();
	LATGINV = (1 << 6);  udelay (3000000);
//        LATGINV = (1 << 15);  udelay (100000);

	loop++;
   //     putch ('.');
	putch (symbol);
if (!(loop & 0x1f))
putch('\n');
    }
}
