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

#ifndef _MIPSASM_H
#define _MIPSASM_H

#define EBASE                       0x9D000000
#define STACK_SIZE_MASK             0x1FFF
#define MAX_NUM_GUEST               7
#define VMID_ERET                   (7 | (7 << 16))   // use VM7 for LLbit cleaning

#define cpu_has_dsp             1

#define zero    $0
#define at      $1
#define v0      $2
#define v1      $3
#define a0      $4
#define a1      $5
#define a2      $6
#define a3      $7
#define t0      $8
#define t1      $9
#define t2      $10
#define t3      $11
#define t4      $12
#define t5      $13
#define t6      $14
#define t7      $15
#define s0      $16
#define s1      $17
#define s2      $18
#define s3      $19
#define s4      $20
#define s5      $21
#define s6      $22
#define s7      $23
#define t8      $24
#define t9      $25
#define k0      $26
#define k1      $27
#define gp      $28
#define sp      $29
#define SP      29
#define fp      $30
#define ra      $31

#define CP0_CONTEXT             $4
// #define CP0_CONTEXT_WAITFLAG    0x80000000
#define CP0_CONTEXT_VPN_SHIFT   (9)
#define CP0_CONTEXT_VM_SHIFT    (23)

#define CP0_COUNT               $9, 0

#define CP0_STATUS              $12
#define CP0_STATUS_CU0          0x10000000
#define CP0_STATUS_CU1          0x20000000
#define CP0_STATUS_CU2          0x40000000
#define CP0_STATUS_CU3          0x80000000
#define CP0_STATUS_FR           0x04000000
#define  CP0_STATUS_FR_SHIFT    26
#define CP0_STATUS_MX           0x01000000
#define  CP0_STATUS_MX_SHIFT    24
#define CP0_STATUS_BEV          0x00400000
#define CP0_STATUS_TS           0x00200000
#define CP0_STATUS_SR           0x00100000
#define CP0_STATUS_NMI          0x00080000
#define CP0_STATUS_MODE         0x00000010
#define CP0_STATUS_ERL          0x00000004
#define CP0_STATUS_EXL          0x00000002
#define CP0_STATUS_IE           0x00000001
#define CP0_STATUS_INIT         (CP0_STATUS_IE|CP0_STATUS_CU0)
#define CP0_STATUS_CPU_CONTROL  (0x0004FF00)
#define CP0_STATUS_ROOTMASK     (CP0_STATUS_SR|CP0_STATUS_NMI)

#define CP0_INTCTL              $12, 1
#define CP0_INTCTL_INIT         0x00000020
#define CP0_G_INTCTL_INIT       0x00000020


#define CP0_GUESTCTL0           $12, 6
#define CP0_GUESTCTL0_GM        31
#define CP0_GUESTCTL0_INIT      0xBD000001 /* GM,MC,CP0=1,AT=3,CG,SFC1 */
//#define CP0_GUESTCTL0_INIT      0x9D000001 /* GM,CP0=1,AT=3,CG,SFC1 */
//#define CP0_GUESTCTL0_INIT      0x9F000001 /* GM,CP0=1,GT=1,AT=3,CG,SFC1 */
//#define CP0_GUESTCTL0_INIT      0xBF000001 /* GM,MC,CP0=1,GT=1,AT=3,CG,SFC1 */
#define CP0_GUESTCTL1           $10, 4
#define CP0_GUESTCTL1_RID_SHIFT (16)

#define CP0_GUESTCTL0EXT        $11, 4
#define CP0_GUESTCTL0EXT_FCD    0x00000008
#define CP0_GUESTCTL0EXT_NOFCD  (CP0_GUESTCTL0EXT_INIT & ~CP0_GUESTCTL0EXT_FCD)
#define CP0_GUESTCTL0EXT_INIT   0x00000058 /* NCC=1,CGI,FCD */

#define CP0_KSCR0               $31, 2
#define CP0_KSCR1               $31, 3
#define CP0_EPC                 $14
#define CP0_SRSCTL              $12, 2
#define CP0_SRSCTL_PSS_SHIFT    (6)
#define CP0_ENTRYHI             $10
#define CP0_ENTRYLO0            $2
#define CP0_ENTRYLO1            $3
#define CP0_PAGEMASK            $5

#define CP0_CAUSE               $13
#define CP0_CAUSE_BD            0x80000000
#define CP0_CAUSE_TI            0x40000000
#define CP0_CAUSE_CE            0x30000000
#define CP0_CAUSE_DC            0x08000000
#define CP0_CAUSE_IV            0x00800000
#define CP0_CAUSE_FDCI          0x00200000
#define CP0_CAUSE_RIPL          0x0001FC00
#define CP0_CAUSE_SWIP          0x00000300
#define CP0_CAUSE_CODE          0x0000007C
#define CP0_CAUSE_CODE_SHIFT    2
#define CP0_CAUSE_ROOTMASK      (CP0_CAUSE_BD|CP0_CAUSE_TI|CP0_CAUSE_CE|CP0_CAUSE_FDCI| \
				 CP0_CAUSE_RIPL|CP0_CAUSE_SWIP|CP0_CAUSE_CODE)

#define CAUSE_INTR      0
#define CAUSE_TLBM      1
#define CAUSE_TLBL      2
#define CAUSE_TLBS      3
#define CAUSE_IBE       6
#define CAUSE_DBE       7
#define CAUSE_SYS       8
#define CAUSE_CU        11
#define CAUSE_TLBRI     19
#define CAUSE_TLBXI     20
#define CAUSE_DSP       26
#define CAUSE_GE        27

#define GUEST_CAUSE_GPSI    0
#define GUEST_CAUSE_HYPCALL 2
#define GUEST_CAUSE_GSFC    1
#define GUEST_CAUSE_GHFC    9

#define CP0_IPL                 $12, 4
#define CP0_RIPL                $13, 4

#define CP0_G_EBASE_INIT        (0x9D000000)

#define CP0_BADVADDR            $8
#define CP0_BADINST             $8, 1
#define CP0_BADINSTP            $8, 2
#define CP0_NESTED_EXC          $13, 5
#define CP0_NESTED_EPC          $14, 2

// =====================================

#define ARGS_FRAME_SIZE     32          // first 4 dwords are args

//#define EXFR                ARGS_FRAME_SIZE
#define EXFR                0
#define PTR_CP0_CONTEXT     EXFR + 4
#define PTR_CP0_CAUSE       EXFR + 8
#define PTR_CP0_EPC         EXFR + 12

#define PTR_CP0_SRSCTL      EXFR + 16
#define PTR_CP0_GUESTCTL0   EXFR + 20
#define PTR_CP0_STATUS      EXFR + 24

#define PTR_CP0_BADVADDR    EXFR + 28
#define PTR_CP0_BADINST     EXFR + 32
#define PTR_CP0_BADINSTP    EXFR + 36
#define PTR_CP0_NESTED_EPC  EXFR + 40
#define PTR_CP0_NESTED_EXC  EXFR + 44

#define PTR_LO              EXFR + 48
#define PTR_HI              EXFR + 52

#define PTR_GPR             EXFR + 56
#define PTR_GP              PTR_GPR + 28 * 4
#define PTR_SP              PTR_GPR + 29 * 4
#define PTR_FP              PTR_GPR + 30 * 4

#define PTR_FPR             PTR_GPR + 32 * 4

#define PTR_FCR31           PTR_FPR + 32 * 8

#define SHORT_INTERRUPT_FRAME               (PTR_GPR)
#define FULL_INTERRUPT_FRAME_SIZE           (PTR_FPR)

#define KSEG0END            (0x9FFFF000)
#define KSEG1(x)            (volatile unsigned int *)(x | 0xA0000000)
#define KPHYS(x)            ((unsigned long)x & 0x1FFFFFFF)


#endif
