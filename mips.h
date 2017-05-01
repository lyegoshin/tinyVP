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

#ifndef _MIPS_H
#define _MIPS_H

#include "mipsasm.h"
#include "bits.h"
#include "thread.h"

#define __stringify_1(x...)     #x
#define __stringify(x...)       __stringify_1(x)

//  usefull macros

static inline unsigned int di(void)
{  unsigned int reg;
   __asm__ __volatile__("di %0\nehb":"=r"(reg): );
   return reg;
}

static inline unsigned int ei(void)
{  unsigned int reg;
   __asm__ __volatile__("ei %0\nehb":"=r"(reg): );
   return reg;
}

#define im_up(ie)       { ie = di(); }
#define im_down(ie)     { if (ie & CP0_STATUS_IE) ei(); }

#define ehb()       __asm__ __volatile__("ehb"::)
#define tlbgr()     __asm__ __volatile__(".set\tvirt\ntlbgr"::)
#define tlbgwi()    __asm__ __volatile__(".set\tvirt\ntlbgwi"::)

#define CP0_REGISTER(name,ASM,ASM2)                     \
enum  cp0_name_##name  {                                \
	CP0_##name##_MERGED = (((ASM)<< 11) | ASM2),    \
};                                                      \
static inline unsigned int                              \
read_cp0_##name(void)                                   \
{  unsigned int reg;                                    \
   __asm__ __volatile__("mfc0 %0, $" __stringify(ASM) "," \
		      __stringify(ASM2) :"=r"(reg): );  \
   return reg;                                          \
}                                                       \
							\
static inline void                                      \
write_cp0_##name(unsigned int value)                    \
{  register unsigned int reg = value;                   \
   __asm__ __volatile__("mtc0 %0, $" __stringify(ASM) "," \
		      __stringify(ASM2) ::"r"(reg) );  \
}                                                       \
							\
static inline unsigned int                              \
read_g_cp0_##name(void)                                 \
{  unsigned int reg;                                    \
   __asm__ __volatile__(".set virt\nmfgc0 %0, $" __stringify(ASM) "," \
		      __stringify(ASM2) :"=r"(reg): );  \
   return reg;                                          \
}                                                       \
							\
static inline void                                      \
write_g_cp0_##name(unsigned int value)                  \
{  register unsigned int reg = value;                   \
   __asm__ __volatile__(".set virt\nmtgc0 %0, $" __stringify(ASM) "," \
		      __stringify(ASM2) ::"r"(reg) );  \
}                                                       \
							\
static inline unsigned int                              \
set_cp0_##name(unsigned int mask)                       \
{                                                       \
	unsigned int old;                               \
	unsigned int new;                               \
							\
	old = read_cp0_##name();                        \
	new = old | mask;                               \
	write_cp0_##name(new);                          \
							\
	return old;                                     \
}                                                       \
							\
static inline unsigned int                              \
clear_cp0_##name(unsigned int mask)                     \
{                                                       \
	unsigned int old;                               \
	unsigned int new;                               \
							\
	old = read_cp0_##name();                        \
	new = old & ~mask ;                             \
	write_cp0_##name(new);                          \
							\
	return old;                                     \
}                                                       \
							\
static inline unsigned int                              \
change_cp0_##name(unsigned int mask, unsigned int value)\
{                                                       \
	unsigned int old;                               \
	unsigned int new;                               \
							\
	old = read_cp0_##name();                        \
	new = old & ~mask;                              \
	new |= (value & mask);                          \
	write_cp0_##name(new);                          \
							\
	return old;                                     \
}                                                       \
							\
static inline unsigned int                              \
set_g_cp0_##name(unsigned int mask)                     \
{                                                       \
	unsigned int old;                               \
	unsigned int new;                               \
							\
	old = read_g_cp0_##name();                      \
	new = old | mask;                               \
	write_g_cp0_##name(new);                        \
							\
	return old;                                     \
}                                                       \
							\
static inline unsigned int                              \
clear_g_cp0_##name(unsigned int mask)                   \
{                                                       \
	unsigned int old;                               \
	unsigned int new;                               \
							\
	old = read_g_cp0_##name();                      \
	new = old & ~mask ;                             \
	write_g_cp0_##name(new);                        \
							\
	return old;                                     \
}                                                       \
							\
static inline unsigned int                              \
change_g_cp0_##name(unsigned int mask, unsigned int value)\
{                                                       \
	unsigned int old;                               \
	unsigned int new;                               \
							\
	old = read_g_cp0_##name();                      \
	new = old & ~mask;                              \
	new |= (value & mask);                          \
	write_g_cp0_##name(new);                        \
							\
	return old;                                     \
}

CP0_REGISTER(index,             0,  0)
CP0_REGISTER(entrylo0,          2,  0)
CP0_REGISTER(entrylo1,          3,  0)
CP0_REGISTER(context,           4,  0)
CP0_REGISTER(userlocal,         4,  2)
CP0_REGISTER(pagemask,          5,  0)
CP0_REGISTER(pagegrain,         5,  1)
CP0_REGISTER(wired,             6,  0)
CP0_REGISTER(hwrena,            7,  0)
CP0_REGISTER(badvaddr,          8,  0)
CP0_REGISTER(badinst,           8,  1)
CP0_REGISTER(badinstp,          8,  2)
CP0_REGISTER(count,             9,  0)
CP0_REGISTER(entryhi,           10, 0)
CP0_REGISTER(guestctl1,         10, 4)
CP0_REGISTER(guestctl2,         10, 5)
CP0_REGISTER(guestctl3,         10, 6)
CP0_REGISTER(compare,           11, 0)
CP0_REGISTER(guestctl0ext,      11, 4)
CP0_REGISTER(status,            12, 0)
CP0_REGISTER(intctl,            12, 1)
CP0_REGISTER(srsctl,            12, 2)
CP0_REGISTER(viewipl,           12, 4)
CP0_REGISTER(srsmap,            12, 3)
CP0_REGISTER(srsmap2,           12, 5)
CP0_REGISTER(guestctl0,         12, 6)
CP0_REGISTER(gtoffset,          12, 7)
CP0_REGISTER(cause,             13, 0)
CP0_REGISTER(viewripl,          13, 4)
CP0_REGISTER(epc,               14, 0)
CP0_REGISTER(nestedepc,         14, 2)
CP0_REGISTER(prid,              15, 0)
CP0_REGISTER(ebase,             15, 1)
CP0_REGISTER(cdmmbase,          15, 2)
CP0_REGISTER(config0,           16, 0)
CP0_REGISTER(config1,           16, 1)
CP0_REGISTER(config2,           16, 2)
CP0_REGISTER(config3,           16, 3)
CP0_REGISTER(config4,           16, 4)
CP0_REGISTER(config5,           16, 5)
CP0_REGISTER(config6,           16, 6)
CP0_REGISTER(config7,           16, 7)
CP0_REGISTER(watchlo,           18, 0)
CP0_REGISTER(watchhi,           19, 0)
CP0_REGISTER(debug,             23, 0)
CP0_REGISTER(perfctl0,          25, 0)
CP0_REGISTER(perfctl1,          25, 1)
CP0_REGISTER(perfctl2,          25, 2)
CP0_REGISTER(perfctl3,          25, 3)
CP0_REGISTER(errctl,            26, 0)
CP0_REGISTER(cacheerr,          27, 0)
CP0_REGISTER(taglo,             28, 0)
CP0_REGISTER(datalo,            28, 1)
CP0_REGISTER(errorepc,          30, 0)
CP0_REGISTER(kscratch0,         31, 2)
CP0_REGISTER(kscratch1,         31, 3)

#define rddsp(mask)                                                     \
({                                                                      \
	unsigned int __dspctl;                                          \
									\
	__asm__ __volatile__(                                           \
	"       .set push                                       \n"     \
	"       .set dsp                                        \n"     \
	"       rddsp   %0, %x1                                 \n"     \
	"       .set pop                                        \n"     \
	: "=r" (__dspctl)                                               \
	: "i" (mask));                                                  \
	__dspctl;                                                       \
})

#define CP1_REGISTER(name,ASMCODE)                          \
static inline unsigned int                                  \
read_cp1_##name(void)                                       \
{  unsigned int reg;                                        \
   __asm__ __volatile__("cfc0 %0," ASMCODE :"=r"(reg): );   \
   return reg;                                              \
}                                                           \
							    \
static inline void                                          \
write_cp1_##name(unsigned int value)                        \
{                                                           \
  __asm__ __volatile__("ctc0 %0," ASMCODE ::"r"(value));    \
}

CP1_REGISTER(fcr31, "$31")

#define mfc0(reg, sel) ({ int __value;                          \
	__asm__ __volatile__ (                                          \
	"mfc0   %0, $%1, %2"                                    \
	: "=r" (__value) : "K" (reg), "K" (sel));               \
	__value; })

#define mfgc0(reg, sel) ({ int __value;                          \
	__asm__ __volatile__ (                                          \
	".set virt\nmfgc0   %0, $%1, %2"                                    \
	: "=r" (__value) : "K" (reg), "K" (sel));               \
	__value; })


#define GET_INST_RT(inst)               (((inst) >> 16) & 0x1F)
#define GET_INST_RD(inst)               (((inst) >> 11) & 0x1F)
#define GET_INST_OPCODE(inst)           ((inst) >> 26)
#define IS_STORE_INST(opcode)           (opcode & 0x08)     /* MIPS R1/R2/R3/R5 */
#define EXTRACT_CP0_REG_AND_SEL(inst)   ((inst) & 0xFFFF)

#define COP0                    (0x10)
#define inst_CACHE(inst)        (GET_INST_OPCODE(inst) == 0x2f)
#define inst_WAIT(inst)         (((inst) & 0xfe00003f) == 0x42000020)
#define inst_MFC0(inst)         (((inst) & 0xffe007f8) == 0x40000000)
#define inst_MTC0(inst)         (((inst) & 0xffe007f8) == 0x40800000)
#define inst_RDHWR(inst)        (((inst) & 0xffe0063f) == 0x7c00003b)
#define inst_SYSCALL_F0000(in)  ((in) == 0x03c0000c)

#define vz_mode(exfr)       \
	(exfr->cp0_guestctl0 & (1 << CP0_GUESTCTL0_GM))

struct cp0_status {
	BITFIELD_FIELD(unsigned int CU3 : 1,
	BITFIELD_FIELD(unsigned int CU2 : 1,
	BITFIELD_FIELD(unsigned int CU1 : 1,
	BITFIELD_FIELD(unsigned int CU0 : 1,

	BITFIELD_FIELD(unsigned int RP  : 1,
	BITFIELD_FIELD(unsigned int FR  : 1,
	BITFIELD_FIELD(unsigned int RE  : 1,
	BITFIELD_FIELD(unsigned int MX  : 1,

	BITFIELD_FIELD(unsigned int R   : 1,
	BITFIELD_FIELD(unsigned int BEV : 1,
	BITFIELD_FIELD(unsigned int TS  : 1,
	BITFIELD_FIELD(unsigned int SR  : 1,

	BITFIELD_FIELD(unsigned int NMI : 1,
	BITFIELD_FIELD(unsigned int IM9 : 1,
	BITFIELD_FIELD(unsigned int CEE : 1,
	BITFIELD_FIELD(unsigned int IMX : 7,

	BITFIELD_FIELD(unsigned int IM1 : 1,
	BITFIELD_FIELD(unsigned int IM0 : 1,
	BITFIELD_FIELD(unsigned int KX  : 1,
	BITFIELD_FIELD(unsigned int SX  : 1,

	BITFIELD_FIELD(unsigned int UX  : 1,
	BITFIELD_FIELD(unsigned int KSU : 2,
	BITFIELD_FIELD(unsigned int ERL : 1,
	BITFIELD_FIELD(unsigned int EXL : 1,

	BITFIELD_FIELD(unsigned int IE  : 1,
	;)))) )))) )))) )))) )))) )))) )
};

static inline unsigned int get_status__ipl(unsigned int s)
{
	struct cp0_status status = *(struct cp0_status*)&s;

	return ((status.IM9 << 7) | status.IMX);
}

static inline void set_exfr_status__ipl(struct exception_frame *exfr,
			      unsigned int s)
{
	struct cp0_status status = *(struct cp0_status*)&(exfr->cp0_status);

	status.IMX = s & 0x3f;
	status.IM9 = (s >> 7) & 0x1;
	exfr->cp0_status = *(unsigned int *)&status;
}

static inline void set_exfr_status__cu1(struct exception_frame *exfr)
{
	struct cp0_status status = *(struct cp0_status*)&(exfr->cp0_status);

	status.CU1 = 1;
	exfr->cp0_status = *(unsigned int *)&status;
}

static inline void set_exfr_status__mx(struct exception_frame *exfr)
{
	struct cp0_status status = *(struct cp0_status*)&(exfr->cp0_status);

	status.MX = 1;
	exfr->cp0_status = *(unsigned int *)&status;
}

struct cp0_cause {
	BITFIELD_FIELD(unsigned int BD  : 1,
	BITFIELD_FIELD(unsigned int TI  : 1,
	BITFIELD_FIELD(unsigned int CE  : 2,
	BITFIELD_FIELD(unsigned int DC  : 1,

	BITFIELD_FIELD(unsigned int PCI : 1,
	BITFIELD_FIELD(unsigned int IC  : 1,
	BITFIELD_FIELD(unsigned int AP  : 1,
	BITFIELD_FIELD(unsigned int IV  : 1,

	BITFIELD_FIELD(unsigned int WP  : 1,
	BITFIELD_FIELD(unsigned int FDCI: 1,
	BITFIELD_FIELD(unsigned int res1: 3,
	BITFIELD_FIELD(unsigned int RIPL: 8,

	BITFIELD_FIELD(unsigned int IP1 : 1,
	BITFIELD_FIELD(unsigned int IP0 : 1,
	BITFIELD_FIELD(unsigned int res2: 1,
	BITFIELD_FIELD(unsigned int EXC : 5,

	BITFIELD_FIELD(unsigned int res3: 2,
	;)))) )))) )))) )))) )
};

static inline unsigned int get_cause__ce(unsigned int v)
{
	struct cp0_cause cause = *(struct cp0_cause*)&v;

	return cause.CE;
}

static inline void set_thread_g_cp0_cause__ti(struct thread *thread)
{
	struct cp0_cause *cause = (struct cp0_cause*)&(thread->g_cp0_cause);

	cause->TI = 1;
}

struct cp0_viewipl {
	BITFIELD_FIELD(unsigned int rsv : 22,
	BITFIELD_FIELD(unsigned int IPL : 8,
	BITFIELD_FIELD(unsigned int IP1 : 1,
	BITFIELD_FIELD(unsigned int IP0 : 1,
	;))))
};

static inline unsigned int get__ipl(unsigned int v)
{
	struct cp0_viewipl viewipl = *(struct cp0_viewipl*)&v;

	return viewipl.IPL;
}

struct cp0_context {
	BITFIELD_FIELD(unsigned int waitflag    : 1,
	BITFIELD_FIELD(unsigned int vm          : 8,
	BITFIELD_FIELD(unsigned int BADVPN2     : 23,
	;)))
};

static inline unsigned int get_exfr_context__vm(struct exception_frame *exfr)
{
	struct cp0_context context = *(struct cp0_context*)&(exfr->cp0_context);

	return context.vm;
}

static inline unsigned int get_exfr_context__waitflag(struct exception_frame *exfr)
{
	struct cp0_context context = *(struct cp0_context*)&(exfr->cp0_context);

	return context.waitflag;
}

static inline unsigned int get_context__shortprolog(unsigned int cp0_context)
{
	return (cp0_context >> 23);
}

struct cp0_guestctl0 {
	BITFIELD_FIELD(unsigned int GM  : 1,
	BITFIELD_FIELD(unsigned int RI  : 1,
	BITFIELD_FIELD(unsigned int MC  : 1,
	BITFIELD_FIELD(unsigned int CP0 : 1,

	BITFIELD_FIELD(unsigned int AT  : 2,
	BITFIELD_FIELD(unsigned int GT  : 1,
	BITFIELD_FIELD(unsigned int CG  : 1,
	BITFIELD_FIELD(unsigned int CF  : 1,

	BITFIELD_FIELD(unsigned int G1  : 1,
	BITFIELD_FIELD(unsigned int impl: 2,
	BITFIELD_FIELD(unsigned int G0E : 1,
	BITFIELD_FIELD(unsigned int PT  : 1,

	BITFIELD_FIELD(unsigned int PIP : 8,
	BITFIELD_FIELD(unsigned int RAD : 1,
	BITFIELD_FIELD(unsigned int DRG : 1,
	BITFIELD_FIELD(unsigned int G2  : 1,

	BITFIELD_FIELD(unsigned int GEC : 5,
	BITFIELD_FIELD(unsigned int SCF2: 1,
	BITFIELD_FIELD(unsigned int SFC1: 1,
	;)))) )))) )))) )))) )))
};

static inline void set_exfr__mc(struct exception_frame *exfr)
{
	(*(struct cp0_guestctl0*)&(exfr->cp0_guestctl0)).MC = 1;
}

static inline void clear_exfr__mc(struct exception_frame *exfr)
{
	(*(struct cp0_guestctl0*)&(exfr->cp0_guestctl0)).MC = 0;
}

struct cp0_guestctl2 {
	BITFIELD_FIELD(unsigned int GRIPL   : 8,
	BITFIELD_FIELD(unsigned int rsv1    : 2,
	BITFIELD_FIELD(unsigned int GEICSS  : 4,
	BITFIELD_FIELD(unsigned int rsv2    : 2,
	BITFIELD_FIELD(unsigned int GVEC    : 16,
	;)))))
};

static inline void inject_irq(unsigned int irq, unsigned int ipl,
		       unsigned int voff)
{
	unsigned int z = 0;
	struct cp0_guestctl2 gc2 = *(struct cp0_guestctl2*)&z;

	gc2.GRIPL = ipl;
	gc2.GVEC = voff;
	write_cp0_guestctl2(*(unsigned int *)&gc2);
}

static inline unsigned int inject_irq_gc2(unsigned int irq, unsigned int ipl,
		       unsigned int voff)
{
	unsigned int z = 0;
	struct cp0_guestctl2 gc2 = *(struct cp0_guestctl2*)&z;

	gc2.GRIPL = ipl;
	gc2.GVEC = voff;
	return *(unsigned int *)&gc2;
}

static inline unsigned int get__guestctl2__gripl(unsigned int rgc)
{
	unsigned int rgc2 = rgc;
	struct cp0_guestctl2 gc2 = *(struct cp0_guestctl2*)&rgc2;

	return gc2.GRIPL;
}

static inline unsigned int get_exfr_guestctl2__gripl(struct exception_frame *exfr)
{
	unsigned int rgc2 = read_cp0_guestctl2();
	struct cp0_guestctl2 gc2 = *(struct cp0_guestctl2*)&rgc2;

	return gc2.GRIPL;
}

static inline void bzero(unsigned long addr, unsigned long size)
{
	register unsigned long a0 __asm__("$4");
	register unsigned long a1 __asm__("$5");

	a0 = (addr);
	a1 = (size);
	__asm__ __volatile__(
		"jal    _bzero"
		:
		:
		: "$1", "$4", "$5", "$31");
}

static inline void restore_fpu_regs(void)
{
	__asm__ __volatile__(
		"jal    _restore_fpu_regs"
		:
		:
		: "$1", "$8", "$30", "$31");
}

static inline void restore_dsp_regs(void)
{
	__asm__ __volatile__(
		"jal    _restore_dsp_regs"
		:
		:
		: "$1", "$8", "$30", "$31");
}

static inline void save_fpu_regs(unsigned int vmid)
{
	register struct thread * v0 __asm__("$2") = get_vm_fp(vmid);

	__asm__ __volatile__(
		"jal    _save_fpu_regs"
		: "+r"(v0)
		:
		: "$1", "$8", "$30", "$31", "memory");
}

static inline void save_dsp_regs(unsigned int vmid)
{
	register struct thread * v0 __asm__("$2") = get_vm_fp(vmid);

	__asm__ __volatile__(
		"jal    _save_dsp_regs"
		: "+r"(v0)
		:
		: "$1", "$8", "$30", "$31", "memory");
}

#endif
