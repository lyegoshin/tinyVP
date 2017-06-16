/*
 * Copyright (c) 2017 Leonid Yegoshin
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

#include    "mipsasm.h"

.section .text.init, "ax"
/*
 * Data for bootloader.
 */
	.set    virt

	.set    push
	.set    noreorder
	.set    noat

Resets:

#if 0

    1.      set WATCH regs
    2.      clear cache L2
    3.      set Config0.K0
    4.      clear L1I/L1D
    //5.      clear RAM
    6.      set EBase   -- align to page boundary. It means - a second vector.
    7.      Jump to K0

#endif

.org    0
check_nmi:                              # Check whether we are here due to a reset or NMI.
	mfc0    s1, CP0_STATUS      # Read Status
	ext     s1, s1, 19, 1           # extract NMI
	beqz    s1, init_cp0            # Branch if this is NOT an NMI exception.
	nop

	# Call nmi_exception().
	jal     _nmi_exception          # call in KSEG1!
	 nop

.org    0x180
	jal     _bev_exception
	 nop

.org    0x200
	jal     _bev_exception
	 nop

.org    0x300
	jal     _bev_exception
	 nop

.org    0x380
	jal     _bev_exception
	 nop

.org    0x400
	jal     _bev_exception
	 nop

.org    0x480
	jal     _bev_exception
	 nop

.org    0x500

init_cp0:

	mtc0    zero, CP0_COUNT     # Clear cp0 Count (Used to measure boot time.)

	//
	// Set all GPRs of all register sets to predefined state.
	//
init_gpr:
	li      $1, 0xdeadbeef          # 0xdeadbeef stands out, kseg2 mapped, odd.

	# Determine how many shadow sets are implemented (in addition to the base register set.)
	# the first time thru the loop it will initialize using $1 set above.
	# At the bottom og the loop, 1 is  subtract from $30
	# and loop back to next_shadow_set to start the next loop and the next lowest set number.
	mfc0    $29, CP0_SRSCTL     # read SRSCtl
	ext     $30, $29, 26, 4         # extract HSS

next_shadow_set:                        # set PSS to shadow set to be initialized
	ins     $29, $30, 6, 4          # insert PSS
	mtc0    $29, CP0_SRSCTL     # write SRSCtl

	wrpgpr  $1, $1
	wrpgpr  $2, $1
	wrpgpr  $3, $1
	wrpgpr  $4, $1
	wrpgpr  $5, $1
	wrpgpr  $6, $1
	wrpgpr  $7, $1
	wrpgpr  $8, $1
	wrpgpr  $9, $1
	wrpgpr  $10, $1
	wrpgpr  $11, $1
	wrpgpr  $12, $1
	wrpgpr  $13, $1
	wrpgpr  $14, $1
	wrpgpr  $15, $1
	wrpgpr  $16, $1
	wrpgpr  $17, $1
	wrpgpr  $18, $1
	wrpgpr  $19, $1
	wrpgpr  $20, $1
	wrpgpr  $21, $1
	wrpgpr  $22, $1
	wrpgpr  $23, $1
	wrpgpr  $24, $1
	wrpgpr  $25, $1
	wrpgpr  $26, $1
	wrpgpr  $27, $1
	wrpgpr  $28, $1
	beqz    $30, init_cpu
	wrpgpr  $29, $1

	wrpgpr  $30, $1
	wrpgpr  $31, $1
	b       next_shadow_set
	add     $30, -1                 # Decrement to the next lower number

	//
	// Init CP0 Status, Count, Compare, Watch*, and Cause.
	//
init_cpu:
	# Initialize Status
	li      v1, CP0_STATUS_BEV | CP0_STATUS_ERL
	mtc0    v1, CP0_STATUS      # write Status

	# Initialize Watch registers if implemented.
	mfc0    v0, CP0_CONFIG1     # read Config1
	ext     v1, v0, 3, 1            # extract bit 3 WR (Watch registers implemented)
	beq     v1, zero, done_wr
	li      v1, 0x7                 # (M_WatchHiI | M_WatchHiR | M_WatchHiW)

	# Clear Watch Status bits and disable watch exceptions
	mtc0    v1, CP0_WATCH_HI     # write WatchHi0
	mfc0    v0, CP0_WATCH_HI     # read WatchHi0
	bgez    v0, done_wr             # Check for bit 31 (sign bit) for more Watch registers
	mtc0    zero, CP0_WATCH_LO   # clear WatchLo0

	mtc0    v1, CP0_WATCH_HI, 1  # write WatchHi1
	mfc0    v0, CP0_WATCH_HI, 1  # read WatchHi1
	bgez    v0, done_wr             # Check for bit 31 (sign bit) for more Watch registers
	mtc0    zero, CP0_WATCH_LO,1 # clear WatchLo1

	mtc0    v1, CP0_WATCH_HI, 2  # write WatchHi2
	mfc0    v0, CP0_WATCH_HI, 2  # read WatchHi2
	bgez    v0, done_wr             # Check for bit 31 (sign bit) for more Watch registers
	mtc0    zero, CP0_WATCH_LO,2 # clear WatchLo2

	mtc0    v1, CP0_WATCH_HI, 3  # write WatchHi3
	mfc0    v0, CP0_WATCH_HI, 3  # read WatchHi3
	bgez    v0, done_wr             # Check for bit 31 (sign bit) for more Watch registers
	mtc0    zero, CP0_WATCH_LO,3 # clear WatchLo3

	mtc0    v1, CP0_WATCH_HI, 4  # write WatchHi4
	mfc0    v0, CP0_WATCH_HI, 4  # read WatchHi4
	bgez    v0, done_wr             # Check for bit 31 (sign bit) for more Watch registers
	mtc0    zero, CP0_WATCH_LO,4 # clear WatchLo4

	mtc0    v1, CP0_WATCH_HI, 5  # write WatchHi5
	mfc0    v0, CP0_WATCH_HI, 5  # read WatchHi5
	bgez    v0, done_wr             # Check for bit 31 (sign bit) for more Watch registers
	mtc0    zero, CP0_WATCH_LO,5 # clear WatchLo5

	mtc0    v1, CP0_WATCH_HI, 6  # write WatchHi6
	mfc0    v0, CP0_WATCH_HI, 6  # read WatchHi6
	bgez    v0, done_wr             # Check for bit 31 (sign bit) for more Watch registers
	mtc0    zero, CP0_WATCH_LO,6 # clear WatchLo6

	mtc0    v1, CP0_WATCH_HI, 7  # write WatchHi7
	mtc0    zero, CP0_WATCH_LO,7 # clear WatchLo7

done_wr:
	# Clear WP bit to avoid watch exception upon user code entry, IV, and software interrupts.
	mtc0    zero, CP0_CAUSE     # clear Cause: init AFTER init of WatchHi/Lo registers.

	# Clear timer interrupt. (Count was cleared at the reset vector to allow timing boot.)
	mtc0    zero, CP0_COMPARE   # clear Compare

/*-----------------------------------
 * Initialization.
 */
	//
	// Clear TLB: generate unique EntryHi contents per entry pair.
	//
init_tlb:
	# Determine if we have a TLB
	mfc0    v1, CP0_CONFIG      # read Config
	ext     v1, v1, 7, 3            # extract MT field
	li      a3, 0x1                 # load a 1 to check against
	bne     v1, a3, init_icache

	# Config1MMUSize == Number of TLB entries - 1
	mfc0    v0, CP0_CONFIG1     # Config1
	ext     v1, v0, 25, 6           # extract MMU Size
	mtc0    zero, CP0_ENTRYLO0  # clear EntryLo0
	mtc0    zero, CP0_ENTRYLO1  # clear EntryLo1
	mtc0    zero, CP0_PAGEMASK  # clear PageMask
	mtc0    zero, CP0_WIRED     # clear Wired
	li      a0, 0x80000000

next_tlb_entry:
	mtc0    v1, CP0_INDEX       # write Index
	mtc0    a0, CP0_ENTRYHI     # write EntryHi
	ehb
	tlbwi
	add     a0, 2<<13               # Add 8K to the address to avoid TLB conflict with previous entry

	bne     v1, zero, next_tlb_entry
	add     v1, -1

init_l23:

    # L2 Cache initialization routine
    # Check L2 cache size
    mfc0    v0, CP0_CONFIG2     # C0_Config2
    # Isolate L2$ Line Size
    ext     v1, v0, 4, 4      # S_Config2SL, W_Config2SL
    # Skip ahead if No L2$
    beq     v1, $0, done_l2cache
    nop

    mfc0    v1, CP0_CONFIG7         # Read Config7
    ext     v1, v1, 18, 1     # extract HCI
    bnez    v1, done_l2cache   # Skip when Hardware Cache Initialization bit set

    li      a2, 2
    sllv    v1, a2, v1       # Now have true L2$ line size in bytes

    # Isolate L2$ Sets per Way
    ext     a0, v0, 8, 4      # S_Config2SS, W_Config2SS
    li      a2, 64
    sllv    a0, a2, a0       # L2$ Sets per way

    # Isolate L2$ Associativity
    # L2$ Assoc (-1)
    ext     a1, v0, 0, 4      # S_Config2SA, W_Config2SA
    add     a1, 1
    mul     a0, a0, a1       # Get total number of sets
    lui     a2, 0x8000     # Get a KSeg0 address for cacheops

    # Clear L23TagLo/L23TagHi registers
    mtc0    $0, CP0_L23TAG_LO
    ehb
    mtc0    $0, CP0_L23TAG_HI
    ehb
    move    a3, a0

1:
    # L2$ Index Store Tag Cache Op
    # Will invalidate the tag entry, clear the lock bit, and clear the LRF bit
    cache   0xB, 0(a2)     # SCIndexStTag
    add     a3, -1         # Decrement set counter
    bne     a3, $0, 1b
    add     a2, v1        # Get next line address

done_l2cache:
	//
	// Clear L1 instruction cache.
	//
init_icache:
	# Determine how big the I-cache is
	mfc0    v0, CP0_CONFIG1     # read Config1
	ext     v1, v0, 19, 3           # extract I-cache line size
	beq     v1, zero, done_icache   # Skip ahead if no I-cache
	nop

	mfc0    s1, CP0_CONFIG7     # Read Config7
	ext     s1, s1, 18, 1           # extract HCI
	bnez    s1, done_icache         # Skip when Hardware Cache Initialization bit set

	li      a2, 2
	sllv    v1, a2, v1              # Now have true I-cache line size in bytes

	ext     a0, v0, 22, 3           # extract IS
	li      a2, 64
	sllv    a0, a2, a0              # I-cache sets per way

	ext     a1, v0, 16, 3           # extract I-cache Assoc - 1
	add     a1, 1
	mul     a0, a0, a1              # Total number of sets
	lui     a2, 0x8000              # Get a KSeg0 address for cacheops

	mtc0    zero, CP0_ITAG_LO    # Clear ITagLo register
	mtc0    zero, CP0_ITAG_HI    # Clear ITagHi register
	move    a3, a0

next_icache_tag:
	# Index Store Tag Cache Op
	# Will invalidate the tag entry, clear the lock bit, and clear the LRF bit
	cache   0x8, 0(a2)              # ICIndexStTag
	add     a3, -1                  # Decrement set counter
	bne     a3, zero, next_icache_tag
	add     a2, v1                  # Get next line address
done_icache:

	//
	// Enable cacheability of kseg0 segment.
	// Until this point the code is executed from segment bfc00000,
	// (i.e. kseg1), so I-cache is not used.
	// Here we jump to kseg0 and run with I-cache enabled.
	//
enable_k0_cache:
	# Set CCA for kseg0 to cacheable.
	# NOTE! This code must be executed in KSEG1 (not KSEG0 uncached)
	mfc0    v0, CP0_CONFIG      # read Config
	li      v1, 3                   # CCA for single-core processors
	ins     v0, v1, 0, 3            # insert K0
	mtc0    v0, CP0_CONFIG      # write Config

	la      a2, init_dcache
	jr      a2                      # switch back to KSEG0
	ehb

	//
	// Initialize the L1 data cache
	//
init_dcache:
	mfc0    v0, CP0_CONFIG1     # read Config1
	ext     v1, v0, 10, 3           # extract D-cache line size
	beq     v1, zero, done_dcache   # Skip ahead if no D-cache
	nop

	mfc0    s1, CP0_CONFIG7     # Read Config7
	ext     s1, s1, 18, 1           # extract HCI
	bnez    s1, done_dcache         # Skip when Hardware Cache Initialization bit set

	li      a2, 2
	sllv    v1, a2, v1              # Now have true D-cache line size in bytes

	ext     a0, v0, 13, 3           # extract DS
	li      a2, 64
	sllv    a0, a2, a0              # D-cache sets per way

	ext     a1, v0, 7, 3            # extract D-cache Assoc - 1
	add     a1, 1
	mul     a0, a0, a1              # Get total number of sets
	lui     a2, 0x8000              # Get a KSeg0 address for cacheops

	mtc0    zero, CP0_ITAG_LO    # Clear ITagLo/DTagLo registers
	mtc0    zero, CP0_DTAG_LO
	mtc0    zero, CP0_ITAG_HI    # Clear ITagHi/DTagHi registers
	mtc0    zero, CP0_DTAG_HI
	move    a3, a0

next_dcache_tag:
	# Index Store Tag Cache Op
	# Will invalidate the tag entry, clear the lock bit, and clear the LRF bit
	cache   0x9, 0(a2)              # DCIndexStTag
	add     a3, -1                  # Decrement set counter
	bne     a3, zero, next_dcache_tag
	add     a2, v1                  # Get next line address

done_dcache:

    lw      $14, _ebase
    mtc0    $14, $15, 1     # C0_EBase

    j       __start
     nop
