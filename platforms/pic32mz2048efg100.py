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

import __builtin__

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
		    exit()
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


