#!/usr/bin/python

#
# Copyright (c) 2017 Leonid Yegoshin
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files
# (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

import os

from pprint import pprint
import pprint
import sys
import math
import __builtin__

pmd = []

def parse_board_file_key(f, key, value):
    if key == "pmd-registers-base":
	pmdtext = value.split()
	for p in pmdtext:
	    i = int(p,0)
	    pmd.append(i)

def parse_board_file_line(f, line):
    # currently no option lines without '=' due to "make"
    return

def board_parse_check(configuration):
    # check VM DMA-capability
    dmaset = {}
    dmagroup = 0
    for vmid in configuration:
	vm = configuration[vmid]
	dmarequired = 0
	if "device" in vm:
	    for dev in vm["device"]:
		if "dma-cfgpg-position" in dev:
		    if dmarequired == 0:
			dmagroup += 1
		    dmarequired = 1
	    if "dma" in vm:
		for dev in vm["device"]:
		    if "dma-set" in dev:
			ds = int(dev["dma-set"])
			if ds in dmaset:
			    if dmaset[ds] != vmid:
				print "Error: two VMs use the same DMA Device Set. Can't have DMA protection of both in PIC32MZEF"
				exit(1)
			else:
			    dmaset[ds] = vmid
	#print "dmarequired", dmarequired, " vmid", vm['id']
	if dmarequired == 1:
	    if "dma" not in vm:
		print "Warning: VM has DMA-capable device but not DMA zone, vm =", vm["id"]
	elif "dma" in vm:
		del vm["dma"]
    if dmagroup > 2:
	print "Error: More than 2 VMs have DMA devices, it is not supported on PIC32MZEF"
	exit(1)

def output_irqmask(ofile,irqs):
    for w in range(0,7):
	if (w % 2) == 0:
	    ofile.write("\n\t")
	ofile.write("0b"),
	for i in range(31,-1,-1):
	    irq = w * 32 + i
	    if irq in irqs:
		ofile.write("1")
	    else:
		ofile.write("0")
	ofile.write(", ")

def output_ipcmask(ofile,irqs):
    for w in range(0,54):
	if (w % 2) == 0:
	    ofile.write("\n\t")
	ofile.write("0b"),
	for i in range(3,-1,-1):
	    irq = w * 4 + i
	    if irq in irqs:
		ofile.write("00011111")
	    else:
		ofile.write("00000000")
	ofile.write(", ")

def output_ic_tables(configuration,ofile):
    #global uniqlblnum
    #global vm7_is_absent
    # output masks words
    print >>ofile,"void insert_timer_IRQ(struct exception_frame *exfr);"
    for vmid in sorted(configuration):
	# vm0 has no IC emulation
	if vmid == 0:
	    continue
	# skip VM7 IC emulation if VM7 has just ERET page
	if (vmid == 7) and (__builtin__.vm7_is_absent == 1):
	    continue
	vm = configuration[vmid]
	#mpp = MyPrettyPrinter()
	#mpp.pprint(vm)
	#
	# begin output IRQ masks
	#
	irqs = []
	if "irqlist" in vm:
	    irqs.extend(vm["irqlist"])
	newlabel = "vm" + str(vm["id"]) + "_" + str(__builtin__.uniqlblnum)
	__builtin__.uniqlblnum += 1
	vm["masklabel"] = newlabel
	print >>ofile, "\nunsigned int const %s[70] = { // vm%d irqmasks" % (newlabel, vmid),
	# out IFS
	output_irqmask(ofile,irqs)
	print >>ofile, "0,"
	# out IEC
	output_irqmask(ofile,irqs)
	print >>ofile, "0,"
	output_ipcmask(ofile,irqs)
	print >>ofile, "\n};"
	#
	# begin output IRQ emulation masks
	#
	ems = {}
	if "emlist" in vm:
	    ems = dict(vm["emlist"])
	ems[0] = "insert_timer"
	newlabel = "vm" + str(vm["id"]) + "_" + str(__builtin__.uniqlblnum)
	__builtin__.uniqlblnum += 1
	vm["emlabel"] = newlabel
	print >>ofile, "\nunsigned int const %s[70] = { // vm%d emulator masks" % (newlabel, vmid),
	# out IFS
	output_irqmask(ofile,ems)
	print >>ofile, "\n\t0,"
	# out IEC
	output_irqmask(ofile,ems)
	print >>ofile, "\n\t0,"
	output_ipcmask(ofile,ems)
	print >>ofile, "\n};"
    print >>ofile, "\nunsigned int const * const vm_ic_masks[8] = {"
    # vm0 has no IC emulation
    print >>ofile, "\t(void *)0x0,"
    for vmid in range(1,8):
	if vmid in configuration:
	    # skip VM7 IC emulation if VM7 has just ERET page
	    if (vmid == 7) and (__builtin__.vm7_is_absent == 1):
		continue
	    vm = configuration[vmid]
	    print >>ofile, "\t&%s[0]," % (vm["masklabel"])
	else:
	    print >>ofile, "\t(void *)0x0,"
    print >>ofile, "};"
    print >>ofile, "\nunsigned int const * const vm_ic_emulators[8] = {"
    # vm0 has no IC emulation
    print >>ofile, "\t(void *)0x0,"
    for vmid in range(1,8):
	if vmid in configuration:
	    # skip VM7 IC emulation if VM7 has just ERET page
	    if (vmid == 7) and (__builtin__.vm7_is_absent == 1):
		continue
	    vm = configuration[vmid]
	    print >>ofile, "\t&%s[0]," % (vm["emlabel"])
	else:
	    print >>ofile, "\t(void *)0x0,"
    print >>ofile, "};"
    # irq2vm
    print >>ofile, "\nunsigned char const irq2vm[216] = {"
    for irq in range(0,216):
	if (irq % 16) == 0:
	    print >>ofile, "\n\t",
	for vmid in configuration:
	    vm = configuration[vmid]
	    if ("irqlist" in vm) and (irq in vm["irqlist"]):
		# print "vm", vmid, "irqlist", vm["irqlist"]
		print >>ofile, "%d, " % (vmid),
		break
	else:
	    print >>ofile, "0, ",
    print >>ofile, "\n};"
    # irq2emulator
    print >>ofile, "\nemulator_irq * const irq2emulator[216] = {"
    for irq in range(0,216):
	if (irq % 4) == 0:
	    print >>ofile, "\n\t",
	for vmid in configuration:
	    vm = configuration[vmid]
	    if ("emlist" in vm) and (irq in vm["emlist"]):
		emlist = vm["emlist"]
		print >>ofile, "&%s_irq," % (emlist[irq]),
		break
	else:
	    print >>ofile, "(void*)0, ",
    print >>ofile, "\n};"
    # irq2emulator_ic_write
    print >>ofile, "\nemulator_ic_write * const irq2emulator_ic_write[216] = {"
    print >>ofile, "\n\t(emulator_ic_write *)&insert_timer_IRQ,",
    for irq in range(1,216):
	if (irq % 4) == 0:
	    print >>ofile, "\n\t",
	for vmid in configuration:
	    vm = configuration[vmid]
	    if ("emlist" in vm) and (irq in vm["emlist"]):
		emlist = vm["emlist"]
		print >>ofile, "&%s_ic," % (emlist[irq]),
		break
	else:
	    print >>ofile, "(void*)0, ",
    print >>ofile, "\n};"
    # output the scheduler VM list
    print >>ofile, "\nint const vm_list[8] = { ",
    print >>ofile, "0,",
    for vmid in range(1,8):
	if vmid in configuration:
	    if (vmid == 7) and (__builtin__.vm7_is_absent == 1):
		print >>ofile, "0,",
	    else:
		vm = configuration[vmid]
		if "entry" not in vm:
		    print "Missed 'entry' option in vm%s" % (vmid)
		    exit(1)
		print >>ofile, "%s," % (vm["entry"]),
	else:
	    print >>ofile, "0,",
    print >>ofile, " };"
    # output VM options
    print >>ofile, "\nint const vm_options[8] = { ",
    print >>ofile, "0,",
    for vmid in range(1,8):
	if vmid in configuration:
	    vm = configuration[vmid]
	    if ("irqpolling" not in vm) or (vm["irqpolling"] == 0):
		print >>ofile, "0,",
	    else:
		print >>ofile, "1,",
	else:
	    print >>ofile, "0,",
    print >>ofile, " };"


def output_board_setup(configuration,ofile):
    print >>ofile, "#include <asm/pic32mz.h>\n"
    print >>ofile, "unsigned int const board_setup_tbl[] = {"
    #
    #           Output DMA protection to System Bus Arbitration registers
    #
    dma_group = 0
    cfgpg = 0
    address = {}
    size = {}
    dmaset = {}
    for vmid in sorted(configuration):
	vm = configuration[vmid]
	if "setup" in vm:
	    for pair in vm["setup"]:
		# convert physaddr into kernel VA
		npair = ((pair[0]|0x80000000),pair[1])
		print >>ofile, "\t0x%08x, %s," % npair
	    print >>ofile, ""
	if "device" in vm:
	    for dev in vm["device"]:
		# adjust Power Managment Disable registers value
		if "pmd1" in dev:
		    pmd[0] = pmd[0] & ~(1 << int(dev["pmd1"],0))
		if "pmd2" in dev:
		    pmd[1] = pmd[1] & ~(1 << int(dev["pmd2"],0))
		if "pmd3" in dev:
		    pmd[2] = pmd[2] & ~(1 << int(dev["pmd3"],0))
		if "pmd4" in dev:
		    pmd[3] = pmd[3] & ~(1 << int(dev["pmd4"],0))
		if "pmd5" in dev:
		    pmd[4] = pmd[4] & ~(1 << int(dev["pmd5"],0))
		if "pmd6" in dev:
		    pmd[5] = pmd[5] & ~(1 << int(dev["pmd6"],0))
		if "pmd7" in dev:
		    pmd[6] = pmd[6] & ~(1 << int(dev["pmd7"],0))
	# vm0 has no IC emulation
	if vmid == 0:
	    continue
	# skip VM7 IC emulation if VM7 has just ERET page
	if (vmid == 7) and (__builtin__.vm7_is_absent == 1):
	    continue
	vm = configuration[vmid]
	if "dma" in vm:
	    dmazone = vm["dma"]
	    dma_group += 1
	    vm["dma_group"] = dma_group
	    address[dma_group] = int(dmazone[0],0)
	    size[dma_group] = int(dmazone[1],0)
	    if "device" in vm:
		for dev in vm["device"]:
		    if "dma-cfgpg-position" in dev:
			cfgpg |= (dma_group << int(dev["dma-cfgpg-position"]))
		    if "dma-set" in dev:
			dmaset[int(dev["dma-set"])] = dma_group

    #      Configure a standard access of Group 0
    #
    # Configure CPU access to first half of RAM 0-256KB
    print >>ofile, "\tSBT2REG0, 0x00000048, \t// addr=0, size=256KB"
    print >>ofile, "\tSBT2RD0,  0x00000001, \t// Group 0 (CPU)"
    print >>ofile, "\tSBT2WR0,  0x00000001, \t// Group 0 (CPU)"
    #
    # Configure CPU access to second half of RAM 256KB-512KB
    print >>ofile, "\tSBT3REG0, 0x00040048, \t// addr=256KB, size=256KB"
    print >>ofile, "\tSBT3RD0,  0x00000001, \t// Group 0 (CPU)"
    print >>ofile, "\tSBT3WR0,  0x00000001, \t// Group 0 (CPU)"
    #
    # Configure CPU access to Peripheral Set 2 (SPI, I2C, UART, PMP regs)
    print >>ofile, "\tSBT6REG0, 0x1F820038, \t// addr=0x1F82000, size=64KB"
    print >>ofile, "\tSBT6RD0,  0x00000001, \t// Group 0 (CPU)"
    print >>ofile, "\tSBT6WR0,  0x00000001, \t// Group 0 (CPU)"
    #
    # Configure CPU access to Peripheral Set 3 (OC, IC, ADC, Timers, Comparators regs)
    print >>ofile, "\tSBT7REG0, 0x1F840038, \t// addr=0x1F84000, size=64KB"
    print >>ofile, "\tSBT7RD0,  0x00000001, \t// Group 0 (CPU)"
    print >>ofile, "\tSBT7WR0,  0x00000001, \t// Group 0 (CPU)"
    #
    # Configure CPU access to Peripheral Set 4 (PORTA-PORTK regs)
    print >>ofile, "\tSBT8REG0, 0x1F860038, \t// addr=0x1F86000, size=64KB"
    print >>ofile, "\tSBT8RD0,  0x00000001, \t// Group 0 (CPU)"
    print >>ofile, "\tSBT8WR0,  0x00000001, \t// Group 0 (CPU)"
    #
    #      Configure memory for access by DMA Group 1
    if 1 in address:
	if address[1] < 0x40000:
	    # Configure CPU access to first half of RAM 0-256KB
	    log = int(math.log(size[1],2)) - 9
	    rd = address[1]
	    rd |= (log << 3)
	    print >>ofile, "\tSBT2REG1, 0x%08x, \t// size=0x%x" % (rd,size[1])
	    print >>ofile, "\tSBT2RD1,  0x00000003, \t// Group 0,1"
	    print >>ofile, "\tSBT2WR1,  0x00000003, \t// Group 0,1"
	    # Shut another half
	    print >>ofile, "\tSBT3REG1, 0x00040000,"
	    print >>ofile, "\tSBT3RD1,  0x00000001, \t// Group 0"
	    print >>ofile, "\tSBT3WR1,  0x00000001, \t// Group 0"
	else:
	    # Shut another half
	    print >>ofile, "\tSBT2REG1, 0x00000000, \t"
	    print >>ofile, "\tSBT2RD1,  0x00000001, \t// Group 0"
	    print >>ofile, "\tSBT2WR1,  0x00000001, \t// Group 0"
	    # Configure CPU access to second half of RAM 256KB-512KB
	    log = int(math.log(size[1],2)) - 9
	    rd = address[1]
	    rd |= (log << 3)
	    print >>ofile, "\tSBT3REG1, 0x%08x, \t// size=0x%x" % (rd,size[1])
	    print >>ofile, "\tSBT3RD1, 0x00000003, \t// Group 0,1"
	    print >>ofile, "\tSBT3WR1, 0x00000003, \t// Group 0,1"
    #
    #      Configure memory for access by DMA Group 2
    if 2 in address:
	if address[2] < 0x40000:
	    # Configure CPU access to first half of RAM 0-256KB
	    log = int(math.log(size[2],2)) - 9
	    rd = address[2]
	    rd |= (log << 3)
	    print >>ofile, "\tSBT2REG2, 0x%08x, \t// size=0x%x" % (rd,size[2])
	    print >>ofile, "\tSBT2RD2,  0x00000005, \t// Group 0,2"
	    print >>ofile, "\tSBT2WR2,  0x00000005, \t// Group 0,2"
	    # Shut another half
	    print >>ofile, "\tSBT3REG2, 0x00040000,"
	    print >>ofile, "\tSBT3RD2,  0x00000001, \t// Group 0"
	    print >>ofile, "\tSBT3WR2,  0x00000001, \t// Group 0"
	else:
	    # Shut another half
	    print >>ofile, "\tSBT2REG2, 0x00000000,"
	    print >>ofile, "\tSBT2RD2,  0x00000001, \t// Group 0"
	    print >>ofile, "\tSBT2WR2,  0x00000001, \t// Group 0"
	    # Configure CPU access to second half of RAM 256KB-512KB
	    log = int(math.log(size[2],2)) - 9
	    rd = address[2]
	    rd |= (log << 3)
	    print >>ofile, "\tSBT3REG2, 0x%08x, \t// size=0x%x" % (rd,size[2])
	    print >>ofile, "\tSBT3RD2,  0x00000005, \t// Group 0,2"
	    print >>ofile, "\tSBT3WR2,  0x00000005, \t// Group 0,2"
    #
    #      Configure DMA Peripheral Set 2 for access by DMA Group
    if 2 in dmaset:
	gr = (1 << dmaset[2]) | 1
	print >>ofile, "\tSBT6REG1, 0x1F820038, \t// addr=0x1F82000, size=64KB"
	print >>ofile, "\tSBT6RD1,  0x%08x," % (gr)
	print >>ofile, "\tSBT6WR1,  0x%08x," % (gr)
    else:
	print >>ofile, "\tSBT6REG1, 0x1F820000, \t// addr=0x1F82000, size=null"
	print >>ofile, "\tSBT6RD1,  0x00000001, \t// Group 0"
	print >>ofile, "\tSBT6WR1,  0x00000001, \t// Group 0"
    #
    #      Configure DMA Peripheral Set 3 for access by DMA Group
    if 3 in dmaset:
	gr = (1 << dmaset[3]) | 1
	print >>ofile, "\tSBT7REG1, 0x1F840038, \t// addr=0x1F84000, size=64KB"
	print >>ofile, "\tSBT7RD1,  0x%08x," % (gr)
	print >>ofile, "\tSBT7WR1,  0x%08x," % (gr)
    else:
	print >>ofile, "\tSBT7REG1, 0x1F840000, \t// addr=0x1F84000, size=null"
	print >>ofile, "\tSBT7RD1,  0x00000001, \t// Group 0"
	print >>ofile, "\tSBT7WR1,  0x00000001, \t// Group 0"
    #
    #      Configure DMA Peripheral Set 4 for access by DMA Group
    if 4 in dmaset:
	gr = (1 << dmaset[4]) | 1
	print >>ofile, "\tSBT8REG1, 0x1F860038, \t// addr=0x1F86000, size=64KB"
	print >>ofile, "\tSBT8RD1,  0x%08x," % (gr)
	print >>ofile, "\tSBT8WR1,  0x%08x," % (gr)
    else:
	print >>ofile, "\tSBT8REG1, 0x1F860000, \t// addr=0x1F86000, size=null"
	print >>ofile, "\tSBT8RD1,  0x00000001, \t// Group 0"
	print >>ofile, "\tSBT8WR1,  0x00000001, \t// Group 0"
    #
    #       Configure CFGPG
    print >>ofile, "\n\tCFGPG,  0x%08x,\n" % (cfgpg)
    #
    #       Configure PMD (Power Management Disable)
    for i in range(0,len(pmd)):
	if i == 0:
	    print >>ofile, "\tPMD1, 0x%08x, \t// ADC, CVref" % (pmd[i])
	if i == 1:
	    print >>ofile, "\tPMD2, 0x%08x, \t// Comparators" % (pmd[i])
	if i == 2:
	    print >>ofile, "\tPMD3, 0x%08x, \t// Input Capture, Output Compare" % (pmd[i])
	if i == 3:
	    print >>ofile, "\tPMD4, 0x%08x, \t// Timers" % (pmd[i])
	if i == 4:
	    print >>ofile, "\tPMD5, 0x%08x, \t// UART, SPI, I2C, USB, CAN" % (pmd[i])
	if i == 5:
	    print >>ofile, "\tPMD6, 0x%08x, \t// REFCLK, PMP, EBI, SQI, Eth" % (pmd[i])
	if i == 6:
	    print >>ofile, "\tPMD7, 0x%08x, \t// DMA, Random, Crypto" % (pmd[i])
    if len(pmd) != 0:
	print >>ofile, ""
    #
    #   End of board register configuration list
    #
    print >>ofile, "\t0, \t// STOP"
    print >>ofile, "};"
