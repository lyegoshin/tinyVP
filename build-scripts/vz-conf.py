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

#
# Configuration utility: creates pte.c + ic-tables.c + some .h
#

import os

from pprint import pprint
import pprint
import sys

import vzparser
from vzparser import *

import importlib
import __builtin__

class MyPrettyPrinter(pprint.PrettyPrinter):
    def format(self, object, context, maxlevels, level):
	if isinstance(object, int):
	    return hex(object), True, False
	else:
	    return pprint.PrettyPrinter.format(self, object, context, maxlevels, level)

CPTE_PAGEMASK_VPN2_SHIFT = 13
ENTRYLO_PFN_SHIFT = 6
ENTRYLO_SW_CDG = 0xF     # G bit is reused for "write-ignore"
CPTE_SHIFT_SIZE = 5
CPTE_BLOCK_SIZE_SHIFT = 4

irq_emulator_list = []
emulator_list = []
__builtin__.vm7_is_absent = None

KB = 1024
MB = (1024 * KB)

dblpagelistsizes = (32, 128, 512, 2*KB, 8*KB, 32*KB, 128*KB, 512*KB, 2*MB, 8*MB, 32*MB, 128*MB, 512*MB)
pagelistsizes = (16, 64, 256, 1*KB, 4*KB, 16*KB, 64*KB, 256*KB, 1*MB, 4*MB, 16*MB, 64*MB, 256*MB)
basepagesize = 4*KB

#
# Parse configuration and verify/adjust it
#

def parse_config(filename):
    #global vm7_is_absent
    vm = parse_config_file(filename, "build")
    #
    #  setup ERET page addresses in VM7 in accordance with platform file definitions
    if 7 in configuration:
	vm = configuration[7]
    else:
	__builtin__.vm7_is_absent = 1
	vm = {}
	vm['id'] = '7'
    mmap = []
    if "mmap" in vm:
	mmap = vm["mmap"]
    mmap.extend([(hex(kphys(str(vzparser.eretpage))), '0x1000', 'xds', hex(kphys(str(vzparser.vzcode))), 0)])
    vm["mmap"] = mmap
    configuration[7] = vm
    #
    # Select the only last ROM and RAM definitions in each VM
    # It is needed to run an automatic ROM/RAM allocation - in case of manual
    # replacement in config file.
    for vmid in configuration:
	vm = configuration[vmid]
	if "mmap" in vm:
	    mmap = vm["mmap"]
	    newmmap = []
	    region1 = None
	    region2 = None
	    for region in mmap:
		type = region[4]
		if type == 0:
		    newmmap.append(region)
		    continue
		if type == 1:
		    if region1 is not None:
			print "Warning: multiple ROM definitions, and last is used in VM",str(vmid)
		    region1 = region
		    continue
		if type == 2:
		    if region2 is not None:
			print "Warning: multiple RAM definitions, and last is used in VM",str(vmid)
		    region2 = region
		    continue
	    if region1 is not None:
		newmmap.append(region1)
	    if region2 is not None:
		newmmap.append(region2)
	    vm["mmap"] = newmmap
    #
    #
    #mpp2 = MyPrettyPrinter()
    #mpp2.pprint(configuration)
    #
    # check VM DMA-capability, create IRQ list
    for vmid in configuration:
	vm = configuration[vmid]
	dmarequired = 0
	if "device" in vm:
	    irqs = [];
	    emlist = {}
	    for dev in vm["device"]:
		if ("dma" in dev) and (dev["dma"] == '1'):
		    dmarequired = 1
		irqlist = []
		if "irq" in dev:
		    irqlist = dev["irq"].split()
		    irqlist = [int(i) for i in irqlist]
		    if "emulator" in dev:
			emulator = dev["emulator"]
			if emulator not in irq_emulator_list:
			    irq_emulator_list.append(emulator)
			for irq in irqlist:
			     emlist[irq] = emulator
		    else:
			irqs.extend(irqlist)
		dev["irqlist"] = irqlist
		if ("regblock" in dev) and ("emulator" in dev):
		    emulator_list.append(dev["emulator"])
	    vm["irqlist"] = irqs
	    vm["emlist"] = emlist
	#print "dmarequired", dmarequired, " vmid", vm['id']
	if dmarequired == 1:
	    if ("dma" not in vm) or (vm["dma"] != '1'):
		print "VM has DMA-capable device but not designated for VA=PA, vm =", vm["id"]
		exit()
    #
    # check IRQ uniq
    #
    error = 0
    for vmid in configuration:
	vm = configuration[vmid]
	if "device" in vm:
	    for dev1 in vm["device"]:
		for vmid2 in configuration:
		    if vmid2 == vmid:
			continue
		    vm2 = configuration[vmid2]
		    if "device" in vm2:
			for dev2 in vm2["device"]:
			    if ("irqlist" in dev1) and ("share" not in dev1) and \
			       ("irqlist" in dev2) and ("share" not in dev2):
				for irq in dev1["irqlist"]:
				    if irq in dev2["irqlist"]:
					 print "Non-shared, non-emulated IRQ", irq, " is used by twice"
					 error = 1
    if error == 1:
	exit(1)
    return

#
# processing the data
#

def check_range_compat(a1, a2):
    if (("s" in a1) and "s" in a2):
	return 0
    if ((not ("w" in a1)) and ((not ("w" in a2)) or ("s" in a2))) or \
	((not ("w" in a2)) and ("s" in a1)):
	return 0
    return 1

def check_uniq_address_range(vm):
    pte = vm["pte"]
    for reg in pte:
	for vm2id in configuration:
	    vm2 = configuration[vm2id]
	    pte2 = vm2["pte"]
	    for reg2 in pte2:
		if reg is reg2:
		    continue
		ra = reg[0]
		if reg[3] is not None:
		    ra = reg[3]
		ra2 = reg[0]
		if reg2[3] is not None:
		    ra2 = reg2[3]
		if ((ra2 >= ra) and (ra2 < (ra + reg[1]))) or \
		    (((ra2 + reg2[1]) > ra) and ((ra2 + reg2[1]) < \
		    (ra + reg[1]))):
		    if check_range_compat(reg[2],reg2[2]):
			print "10 Configuration file parse error, vm", vm["id"]
			print "...address ranges overlap:",reg,reg2
			exit(1)
    for vm2id in configuration:
	vm2 = configuration[vm2id]
	if vm2 is vm:
	    continue
	if ("dma" in vm2) and (vm2["dma"] == '1'):
	    if ("dma" in vm) and (vm["dma"] == '1'):
		print "WARNING: Two VMs require DMA-compatible memory:",vm["id"], vm2["id"]


def create_pte_list(vm):
    dmarequired = 0
    if ("dma" in vm) and (vm["dma"] == '1'):
	dmarequired = 1
    pte = []
    # create a plain list of memory region tuples
    if "mmap" in vm:
	for block in vm["mmap"]:
	    addr = int(block[0],0)
	    size = int(block[1],0)
	    flags = block[2]
	    physaddr = int(block[3],0)
	    pte.append((addr, size, block[2], physaddr, None))
	    if (dmarequired == 1) and (addr != physaddr) and ("d" not in flags):
		print "VM has designated for DMA but VA != PA, vm =", vm["id"], "memblock ", block
		exit(1)
    if "device" in vm:
	for device in vm["device"]:
	    if "regblock" in device:
		for block in device["regblock"]:
		    addr = int(block[0],0)
		    size = int(block[1],0)
		    flag = block[2]
		    physaddr = int(block[3],0)
		    if ("share" in device) and (device["share"] != 0):
			if not "s" in flag:
			    flag = flag + 's'
		    emulator = None
		    if ("emulator" in device):
			emulator = device["emulator"]
		    if (emulator is None) and ("e" in flag):
			print "Device", device["name"], " has no emulator but register block", block, " require emulation"
			exit(1)
		    pte.append((addr, size, flag, physaddr, emulator))
    for tpte in pte:
	# check X-page size >= 4K
	if ("x" in tpte[2]) and (tpte[1] < 0x1000):
	    print "X-page size < 4K, vm =", vm["id"], "region =",tpte
	    exit(1)
    pte.sort(key=lambda k: k[0])
    vm["pte"] = pte

#
# bitmask handling for emulated access to device registers
#

def add_bit(bitmask, n):
    nw = n / 32
    w = bitmask[nw]
    w |= (1 << (n % 32))
    bitmask[nw] = w
    return bitmask

def pte_with_bitmask(addr, paddr, pte, size):
    l = (size + (32 * 4) - 1)/(32 * 4)
    bitmask = [0 for i in range(0, l)]
    for i in range(0, size/4):
	j = i * 4
	if (j >= (pte[0] - addr)) and \
	   (j < (pte[0] + pte[1] - addr)):
	    bitmask = add_bit(bitmask,i)
    s = pte[2]
    if not "e" in s:
	s += "e"
    newpte = (addr, size, s, paddr, pte[4], bitmask)
    #print "PWB: newpte:",newpte
    return newpte

def merge_bitmask(bm1, bm2):
    nbm = bm1
    for i in range(0, len(bm1)):
	nbm[i] |= bm2[i]
    return nbm

def join_bitmask(bm1, bm1len, bm2, bm2len):
    if bm1 is None:
	if bm2 is None:
	    return None
	l = (bm2len + (32 * 4) - 1)/(32 * 4)
	bm1 = [0 for i in range(0, l)]
	bl = bm2len/4
    if bm2 is None:
	l = (bm1len + (32 * 4) - 1)/(32 * 4)
	bm2 = [0 for i in range(0, l)]
	bl = bm1len/4
    else:
	l = (bm1len + (32 * 4) - 1)/(32 * 4)
	bl = bm1len/4
    nbm = [0 for i in range(0, ((2 * bl * 4) + (32 * 4) - 1)/(32 * 4))]
    #print "JB: bl=",bl," l=",l," nbm:",nbm
    for i in range(0, 2 * bl):
	if i < bl:
	    ind = i / 32
	    bitnum = i % 32
	    #print "... i=",i," ind=",ind," bitnum=",bitnum
	    if (bm1[ind] & (1<<bitnum)) != 0:
		add_bit(nbm,i)
	else:
	    ind = (i - bl) / 32
	    bitnum = (i - bl) % 32
	    #print "___ i=",i," ind=",ind," bitnum=",bitnum
	    if (bm2[ind] & (1<<bitnum)) != 0:
		add_bit(nbm,i)
    #print "JB: bm1len=",bm1len," bm2len=",bm2len," bm1:",bm1," bm2:",bm2," nbm:",nbm
    #nbm.extend(bm2)
    return nbm

#
# handle a small PTE blocks (size < 1 page size) for emulated access
#
def small_pte_blocks(vm):
    ptelist = vm["pte"]
    newptelist = []
    # extend small blocks and add bitmasks
    for pte in ptelist:
	# skip PTE with zero size, it is an artifact from build process
	if pte[1] == 0:
	    continue
	if pte[1] < basepagesize:
	    size = pte[1]
	    sz0 = pagelistsizes[-1]
	    for sz in reversed(pagelistsizes):
		if size > sz:
		    size = sz0
		    break
		sz0 = sz
	    else:
		size = pagelistsizes[0]
	    #size = pagelistsizes[0]
	    addr = pte[0] & ~(size - 1)
	    paddr = pte[3] & ~(size - 1)
	    #print "SPB: pte:",pte," size=",size
	    npte = pte_with_bitmask(addr,paddr,pte,size)
	else:
	    npte = (pte[0], pte[1], pte[2], pte[3], pte[4], None)
	newptelist.append(npte)
    # join the same pages
    ptelist = newptelist
    newptelist = []
    for pte in ptelist:
	if pte[5] is not None:
	    removedptelist = []
	    for pte2 in newptelist:
		if (pte[0] >= pte2[0]) and \
		   ((pte[0] + pte[1]) <= (pte2[0] + pte2[1])):
		    if (pte[0] != pte2[0]) or (pte[1] != pte2[1]) or \
		       (pte2[5] is None) or \
		       (pte[4] != pte2[4]):
			print "Address ranges overlap: emulated with non-emulated, vm",vm["id"]
			print pte
			exit(1)
		    #print "SPB2: pte:",pte," pte2:", pte2
		    bitm = merge_bitmask(pte[5],pte2[5])
		    s = pte[2]
		    if not "e" in s:
			s += "e"
		    pte = (pte[0], pte[1], s, pte[3], pte[4], bitm)
		else:
		    removedptelist.append(pte2)
	    npte = pte
	    removedptelist.append(npte)
	    newptelist = removedptelist
	else:
	    npte = pte
	    #print "PTE:", npte
	    newptelist.append(npte)
    newptelist.sort(key=lambda k: k[0])
    #mpp3 = MyPrettyPrinter()
    #mpp3.pprint(newptelist)
    vm["pte"] = newptelist

def create_pte_lists():
    for vm in configuration:
	create_pte_list(configuration[vm])
	small_pte_blocks(configuration[vm])
    for vm in configuration:
	check_uniq_address_range(configuration[vm])

def page_attr_match(attr1, attr2):
    if len(attr1) != len(attr2):
	return 0
    for c in attr1:
	if not c in attr2:
	    return 0
    return 1

#
# split big PTE, to fit a region by a smaller size PTEs
#
def split_pte(pteelem):
#    print "   (%x, %x, %s, %x)" % pteelem
    ptecopy = []
    size = 2 ** (pteelem[1] .bit_length() - 1)
    addr = pteelem[0]
    physaddr = pteelem[3]
    for sz in reversed(pagelistsizes):
	if size >= sz:
	    size = sz
	    break
    else:
	size = pagelistsizes[0]
    chunk = addr % size
#    print "................ chunk A %x, size %x" % (chunk, size)
    if chunk != 0:
	chunk = size - chunk
	pteadd = split_pte((addr, chunk, pteelem[2], physaddr, pteelem[4], pteelem[5]))
	ptecopy.extend(pteadd)
	addr = addr + chunk
	physaddr = physaddr + chunk
    while (addr + size) <= (pteelem[0] + pteelem[1]):
	ptecopy.append((addr, size, pteelem[2], physaddr, pteelem[4], pteelem[5]))
	#print " + (%x, %x, %s, %x)" % (addr, size, pteelem[2], physaddr)
	addr = addr + size
	physaddr = physaddr + size
    addr = pteelem[0] + pteelem[1]
    chunk = addr % size
    addr = addr - chunk
    physaddr = pteelem[3] + pteelem[1] - chunk
#    print "................ chunk B %x, size %x" % (chunk, size)
    if chunk != 0:
	pteadd = split_pte((addr, chunk, pteelem[2], physaddr, pteelem[4], pteelem[5]))
	ptecopy.extend(pteadd)
    return ptecopy

def split_ptes(vm):
    pte = vm["pte"]
    ptecopy = []
    for pteelem in pte:
	ptecopy.extend(split_pte(pteelem))
    vm["pte"] = ptecopy
    for pteelem in ptecopy:
	if (pteelem[0] % pteelem[1]) != 0:
	    print "Internal error, page %x, size %x" %(pteelem[0], pteelem[1])
    return vm

#
# split one big PTE into 4, to allow a pair of PTEs with two pages each
#
def split_page(pte):
    ptetree = []
    size = pte[1] / 4
    addr = pte[0]
    paddr = pte[3]
    ptetree.append((addr, size, pte[2], pte[2], paddr, paddr + size, pte[4], pte[5]))
    addr += size * 2
    paddr += size * 2
    ptetree.append((addr, size, pte[2], pte[2], paddr, paddr + size, pte[4], pte[5]))
    return ptetree

def address_alignment_check(addr, size, physaddr):
    if (physaddr % size) != 0:
	print "Physical address alignment doesn't match GVA page alignment: guest GVA=%08x Size=%08x RPA=%08x" % (addr, size, physaddr)
	exit(1)

#
# let's try join pages into bigger
#
def join_pairs(vm):
    ptet = vm["pte"]
    ptesave = None
    lastaddr = 0
    ptetree = []
    for pte in ptet:
	if ptesave is None:
	    if (pte[0] % (2 * pte[1])) == 0:
		# first page in pair
		ptesave = pte
		continue
	    # second page. Check that first is empty
	    if lastaddr + pte[1] <= pte[0]:
		ptetree.append((pte[0] - pte[1], pte[1], None, pte[2], None, pte[3], pte[4], join_bitmask(None,0,pte[5],pte[1])))
		lastaddr = pte[0] + pte[1]
		continue
	    # split a page
	    ptea = split_page(pte)
	    lastaddr = pte[0] + pte[1]
	    ptetree.extend(ptea)
	    continue
	# Potential second page or discontigues page
	if (ptesave[1] == pte[1]) and \
	   (ptesave[4] == pte[4]) and \
	   ((ptesave[0] + pte[1]) == pte[0]):
	    # a regular page pair
	    ptetree.append((ptesave[0], pte[1], ptesave[2], pte[2], ptesave[3], pte[3], pte[4], join_bitmask(ptesave[5],ptesave[1],pte[5],pte[1])))
	    lastaddr = pte[0] + pte[1]
	    ptesave = None
	    continue
	if ((ptesave[0] + (2 * ptesave[1])) <= pte[0]) and \
	    not ((pte[0] % (2 * pte[1]) != 0) and \
	    ((ptesave[0] + (2 * ptesave[1])) > (pte[0] - pte[1]))) :
	    # gap, big enough
	    # first, create a saved page as first in pair with empty second
	    ptetree.append((ptesave[0], ptesave[1], ptesave[2], None, ptesave[3], None, ptesave[4], join_bitmask(ptesave[5],ptesave[1],None,0)))
	    lastaddr = ptesave[0] + ptesave[1]
	    ptesave = None
	    # next, process pte
	    if (pte[0] % (2 * pte[1])) == 0:
		# first page in pair
		ptesave = pte
		continue
	    # just a second page
	    ptetree.append((pte[0] - pte[1], pte[1], None, pte[2], None, pte[3], pte[4], join_bitmask(None,0,pte[5],pte[1])))
	    lastaddr = pte[0] + pte[1]
	    continue
	# split one page. Check which one
	if (ptesave[0] + (2 * ptesave[1])) <= pte[0]:
	    # split a second, it is bigger. But first - add first
	    ptetree.append((ptesave[0], ptesave[1], ptesave[2], None, ptesave[3], None, ptesave[4], join_bitmask(ptesave[5],ptesave[1],None,0)))
	    ptea = split_page(pte)
	    lastaddr = pte[0] + pte[1]
	    ptesave = None
	    ptetree.extend(ptea)
	    continue
	# split a first
	ptea = split_page(ptesave)
	lastaddr = ptesave[0] + ptesave[1]
	ptesave = None
	ptetree.extend(ptea)
	# process a second
	if (pte[0] % (2 * pte[1])) == 0:
	    # first page in pair
	    ptesave = pte
	    continue
	# second page
	ptetree.append((pte[0] - pte[1], pte[1], None, pte[2], None, pte[3], pte[4], join_bitmask(None,0,pte[5],pte[1])))
	lastaddr = pte[0] + pte[1]
	continue
    if ptesave is not None:
	# it is a first page
	ptetree.append((ptesave[0], ptesave[1], ptesave[2], None, ptesave[3], None, ptesave[4], join_bitmask(ptesave[5],ptesave[1],None,0)))
    # set size to area size from page size
    ptecopy = []
    for pt in ptetree:
	ptecopy.append((pt[0], 2 * pt[1], pt[2], pt[3], pt[4], pt[5], pt[6], pt[7]))
    vm["pteplaintree"] = ptecopy
    for pte in ptecopy:
	if (pte[2] is not None) and (pte[3] is not None):
	    #print " : (%x, %x, %s, %s, %x, %x)" % (pte[0], pte[1], pte[2], pte[3], pte[4], pte[5])
	    address_alignment_check(pte[0], pte[1]/2, pte[4]);
	    address_alignment_check(pte[0] + pte[1]/2, pte[1]/2, pte[5]);
	elif pte[2] is not None:
	    #print " : (%x, %x, %s, -, %x, - )" % (pte[0],pte[1],pte[2],pte[4])
	    address_alignment_check(pte[0], pte[1]/2, pte[4]);
	else:
	    #print " : (%x, %x, -, %s, -, %x)" % (pte[0],pte[1],pte[3],pte[5])
	    address_alignment_check(pte[0] + pte[1]/2, pte[1]/2, pte[5]);
    return vm

#
# simplify tree - cat&drop some intermediate nodes if there is only one leaf down
#
def simplify_tree(currnode):
    if type(currnode) is tuple:
	return currnode
    descendants = []
    for node in currnode[3]:
	newnode = simplify_tree(node)
	descendants.append(newnode)
    currnode[3] = descendants
    if len(descendants) != 1:
	return currnode
    desc = descendants[0]
    if type(desc) is tuple:
	if (currnode[1] == currnode[2]) and \
	   (currnode[1] == desc[1]):
	    return desc
	currnode[0] = desc[0]
	currnode[1] = desc[1]
	return currnode
    return desc
    #if len(desc[3]) != 1:
    #    #return currnode
    #    return desc
    #desc = desc[3][0]
    ##if type(desc) is tuple:
    ##    if desc[0] != currnode[0]:
    ##        return currnode
    #currnode[3] = [desc]
    #currnode[2] = desc[1]
    #currnode[1] = desc[1]
    #currnode[0] = desc[0]
    #return currnode

def create_pte_tree(vm):
    ptetree = vm["pteplaintree"]
    for idx, nodesize in enumerate(dblpagelistsizes[1:]):
	size = dblpagelistsizes[idx]
	ptecopy = []
	currim = None
	#print "Idx",idx," nodesize",nodesize
	oldaddr = 0
	for pte in ptetree:
	    if (not currim is None) and \
	       (pte[1] == size):
		newaddr = currim[0] & ~(2 * currim[1])
		if (currim[0] + currim[1]) >= (pte[0] + size):
		    # incorporate in currim
		    currim[3].append(pte)
		elif (nodesize != dblpagelistsizes[-1]) and \
		     ((currim[0] + currim[1]) <= pte[0]) and \
		     (newaddr >= oldaddr) and \
		     ((newaddr + (2 * currim[1])) >= (pte[0] + size)):
		    # extend currim
		    currim[1] *= 2
		    currim[0] = newaddr
		    # incorporate in currim
		    currim[3].append(pte)
		else:
		    ptecopy.append(currim)
		    currim = [pte[0] & ~(nodesize-1), nodesize, size, [pte]]
	    else:
		# finish currim and start new
		if not currim is None:
		    ptecopy.append(currim)
		    currim = None
		if pte[1] >= nodesize:
		    ptecopy.append(pte)
		    oldaddr = pte[0] + pte[1]
		    continue
		# create a single node
		currim = [pte[0] & ~(nodesize-1), nodesize, size, [pte]]
	    oldaddr = pte[0] + pte[1]
	if not currim is None:
	    ptecopy.append(currim)
	ptetree = ptecopy
    #mpp = MyPrettyPrinter()
    #mpp.pprint(ptetree)
    for idx, node in enumerate(ptetree):
	ptetree[idx] = simplify_tree(node)
    #mpp.pprint(ptetree)
    vm["ptetree"] = ptetree
    return vm

#
# output tables
#

def clz(val):
    for i in range(0, 32):
	if (val & 0x80000000) == 0x80000000:
	    val -= 1
	    break
	val <<= 1
    return i

def output_el01(pte, ofile, flag):
    hwb = 0
    if (pte is None) or (pte[4] is None):
	elo0 = 0
    else:
	hwb = 0
	if "u" in pte[2]:
	    hwb |= 0x10
	elif "a" in pte[2]:
	    hwb |= 0x38
	elif "n" in pte[2]:
	    hwb |= 0
	elif "t" in pte[2]:
	    hwb |= 0x8
	else:
	    hwb |= 0x18
	if "i" in pte[2]:
	    hwb |= 0x1
	elif "w" in pte[2]:
	    hwb |= 0x4
	if not "e" in pte[2]:
	    hwb |= 0x2
	if not "x" in pte[2]:
	    hwb |= 0x40000000
	if not "r" in pte[2]:
	    hwb |= 0x80000000
	if flag == 0:
	    elo0 = (pte[4] >> (CPTE_PAGEMASK_VPN2_SHIFT - 1 - ENTRYLO_PFN_SHIFT)) | hwb
	else:
	    elo0 = pte[4] | (hwb & ENTRYLO_SW_CDG)
    if (pte is None) or (pte[5] is None):
	elo1 = 0
    else:
	hwb = 0
	if "u" in pte[3]:
	    hwb |= 0x10
	elif "a" in pte[3]:
	    hwb |= 0x38
	elif "n" in pte[3]:
	    hwb |= 0
	elif "t" in pte[3]:
	    hwb |= 0x8
	else:
	    hwb |= 0x18
	if "i" in pte[3]:
	    hwb |= 0x1
	elif "w" in pte[3]:
	    hwb |= 0x4
	if not "e" in pte[3]:
	    hwb |= 0x2
	if not "x" in pte[3]:
	    hwb |= 0x40000000
	if not "r" in pte[3]:
	    hwb |= 0x80000000
	if flag == 0:
	    elo1 = (pte[5] >> (CPTE_PAGEMASK_VPN2_SHIFT - 1 - ENTRYLO_PFN_SHIFT)) | hwb
	else:
	    elo1 = pte[5] | (hwb & ENTRYLO_SW_CDG)
    emulator = "0x0"
    bitmask = "(void *)0x0"
    if pte is not None:
	if pte[6] is not None:
	    emulator = "&" + pte[6]
	if pte[7] is not None:
	    bitmask = "&" + pte[7]
    if pte is None:
	    print >>ofile, "    { .cpte_dl.f=%s, %s, 0, 0}," % (emulator, bitmask)
    elif pte[4] is None:
	if pte[5] is None:
	    print >>ofile, "    { .cpte_dl.f=%s, %s, 0, 0 }," % (emulator, bitmask)
	else:
	    print >>ofile, "    { .cpte_dl.f=%s, %s, 0x%0x, 0x%0x },\t// ..........  0x%0x" % (emulator, bitmask, elo0, elo1, pte[5])
    elif pte[5] is None:
	print >>ofile, "    { .cpte_dl.f=%s, %s, 0x%0x, 0x%0x }, //\t0x%0x" % (emulator, bitmask, elo0, elo1, pte[4])
    else:
	print >>ofile, "    { .cpte_dl.f=%s, %s, 0x%0x, 0x%0x }, //\t0x%0x 0x%0x" % (emulator, bitmask, elo0, elo1, pte[4], pte[5])

__builtin__.uniqlblnum = 0

def output_bitmasks(pt, vm, bfile):
    #global uniqlblnum
    ptlist = []
    for idx, pte in enumerate(pt[3]):
	if type(pte) is not tuple:
	    output_bitmasks(pte, vm, bfile)
	    continue
	if pte[7] is None:
	    continue
	newlabel = "bm" + str(vm["id"]) + "_" + str(__builtin__.uniqlblnum)
	__builtin__.uniqlblnum += 1
	print >>bfile, "\nunsigned long const %s[] = {" % (newlabel),
	for idx2, w in enumerate(pte[7]):
	    if (idx2 % 8) == 0:
		print >>bfile, "\n\t",
	    print >>bfile, "0x%0x, " % (w),
	print >>bfile, "\n};"
	newpte = (pte[0], pte[1], pte[2], pte[3], pte[4], pte[5], pte[6], newlabel)
	pt[3][idx] = newpte

#
# output a small PTE tree - tree of "pages" less than 1 page, for emulation goal
#
def output_small_ptetree(pt, vm, ofile):
    #global uniqlblnum
    ptlist = []
    for pte in pt[3]:
	label = ""
	if type(pte) is not tuple:
	    label = output_small_ptetree(pte, vm, ofile)
	ptlist.append((label, pte))
    newlabel = "vm" + str(vm["id"]) + "_" + str(__builtin__.uniqlblnum) + "_sw"
    __builtin__.uniqlblnum += 1
    print >>ofile, "\nunion cpte const %s[] = { // 0x%08x / 0x%x" % (newlabel,pt[0],pt[1])
    slot = 0
    for label, pte in ptlist:
	nslot = (pte[0] - pt[0]) / pt[2]
	#print 'Label', label, ' nslot', nslot, '; pte[0]=%0x, pt[0]=%x, pt[2]=%x' % (pte[0],pt[0],pt[2])
	for i in range (slot, nslot):
	    output_el01(None, ofile, 1)
	slot = nslot + 1
	if type(pte) is tuple:
	    output_el01(pte, ofile, 1)
	    continue
	# output intermediate node here from pte, using label for ref
	maskofaddress = (pt[2] - 1) ^ (pte[1] - 1)
	goldenaddress = pte[0] & maskofaddress
	pagemask = pte[2] - 1
	lshift = clz(pte[1] - 1)
	rshift = 32 - (clz(pte[2] - 1) - lshift) - CPTE_BLOCK_SIZE_SHIFT
	#print " CLZ: pte[1] = %x, lshift=%d rshift=%d clz(%x -1)=%d" % (pte[1],lshift,rshift,pte[2],clz(pte[2] - 1))
	print >>ofile, "    { .cpte_nd.next=&%s[0], 0x%0x | 1, 0x%0x, .cpte_nd.pm=0x%x, .cpte_nd.rs=%d, .cpte_nd.ls=%d }, // 0x%08x : 0x%x" % \
	    (label, maskofaddress, goldenaddress, pagemask, rshift, lshift, pte[0], pte[1])
    nslot = pt[1] / pt[2]
    #print '... slot', slot, ' nslot', nslot,'; pt=%x %x %x' % (pt[0], pt[1], pt[2])
    for islot in range (slot, nslot):
	    output_el01(None, ofile, 1)
    print >>ofile, "};"
    return newlabel

def output_ptetree(pt, vm, ofile):
    #global uniqlblnum
    ptlist = []
    for pte in pt[3]:
	label = ""
	if type(pte) is not tuple:
	    if pte[2] < 0x1000:
		#
		# we have a small PTE, need to cut a page refill here and continue
		# with tree for an access emulation
		#
		label1 = output_small_ptetree(pte, vm, ofile)
		# output transit element - "stop-point"
		label = "vm" + str(vm["id"]) + "_" + str(__builtin__.uniqlblnum) + "_tr"
		__builtin__.uniqlblnum += 1
		maskofaddress = (pt[2] - 1) ^ (pte[1] - 1)
		goldenaddress = pte[0] & maskofaddress
		pagemask = (pte[2] - 1)
		lshift = clz(pte[1] - 1)
		rshift = 32 - (clz(pte[2] - 1) - lshift) - CPTE_BLOCK_SIZE_SHIFT
		#print " CLZ: pte[1] = %x, lshift=%d rshift=%d clz(%x -1)=%d" % (pte[1],lshift,rshift,pte[2],clz(pte[2] - 1))
		print >>ofile, "\nunion cpte const %s[] = // 0x%08x - 0x%x" % (label, pte[0], pte[1]),
		print >>ofile, "\n    {{ .cpte_nd.next=&%s[0], 0x%0x | 1, 0x%0x, .cpte_nd.pm=0x%x, .cpte_nd.rs=%d, .cpte_nd.ls=%d },}; // 0x%08x : 0x%x" % \
		    (label1, maskofaddress, goldenaddress, pagemask, rshift, lshift, pte[0], pte[1])
		ptlist.append((label, pte, 1))
	    else:
		label = output_ptetree(pte, vm, ofile)
		ptlist.append((label, pte, 0))
	else:
	    ptlist.append((label, pte, 0))
    newlabel = "vm" + str(vm["id"]) + "_" + str(__builtin__.uniqlblnum)
    __builtin__.uniqlblnum += 1
    print >>ofile, "\nunion cpte const %s[] = { // 0x%08x / 0x%x" % (newlabel,pt[0],pt[1])
    slot = 0
    for label, pte, transit in ptlist:
	nslot = (pte[0] - pt[0]) / pt[2]
	#print 'Label', label, ' nslot', nslot, '; pte[0]=%0x, pt[0]=%x, pt[2]=%x' % (pte[0],pt[0],pt[2])
	for i in range (slot, nslot):
	    output_el01(None, ofile, 0)
	slot = nslot + 1
	if type(pte) is tuple:
	    output_el01(pte, ofile, 0)
	    continue
	if (transit != 0):
	    print >>ofile, "    { .cpte_nd.next=&%s[0], 0x0, 0, .cpte_nd.pm=0, .cpte_nd.rs=0, .cpte_nd.ls=0 }, // 0x%08x : 0x%08x" % \
		(label, pte[0], pte[1])
	else:
	    # output intermediate node here from pte, using label for ref
	    maskofaddress = (pt[2] - 1) ^ (pte[1] - 1)
	    goldenaddress = pte[0] & maskofaddress
	    pagemask = (pte[2] - 1) >> (CPTE_SHIFT_SIZE + CPTE_SHIFT_SIZE)
	    lshift = clz(pte[1] - 1)
	    rshift = 32 - (clz(pte[2] - 1) - lshift) - CPTE_BLOCK_SIZE_SHIFT
	    #print " CLZ: pte[1] = %x, lshift=%d rshift=%d clz(%x -1)=%d" % (pte[1],lshift,rshift,pte[2],clz(pte[2] - 1))
	    print >>ofile, "    { .cpte_nd.next=&%s[0], 0x%0x | 1, 0x%0x, .cpte_nd.pm=0x%x, .cpte_nd.rs=%d, .cpte_nd.ls=%d }, // 0x%08x : 0x%x" % \
		(label, maskofaddress, goldenaddress, pagemask, rshift, lshift, pte[0], pte[1])
    nslot = pt[1] / pt[2]
    #print '... slot', slot, ' nslot', nslot,'; pt=%x %x %x' % (pt[0], pt[1], pt[2])
    for islot in range (slot, nslot):
	    output_el01(None, ofile, 0)
    print >>ofile, "};"
    return newlabel

def output_emulator_list(ofile):
    print >>ofile, ""
    for i in emulator_list:
	print >>ofile, "emulator const %s;" % (i)

def output_emulator_lists(ofile):
    print >>ofile, ""
    for i in irq_emulator_list:
	print >>ofile, "emulator_irq const %s_irq;" % (i)
	print >>ofile, "emulator_ic_write const %s_ic;" % (i)

def output_tlb_tables(vm, ofile):
    ptetree = vm["ptetree"]
    pteroot = [0, 0x100000000, 0x20000000, ptetree]
    output_bitmasks(pteroot, vm, ofile)
    label = output_ptetree(pteroot, vm, ofile)
    return label


sys.path.append("platforms")
platform = importlib.import_module(sys.argv[3])
parse_board("boards/" + sys.argv[1])
parse_devicelib("device.lib")
parse_config(sys.argv[2])

objcopy = []
print "\nMEMORY MAPS:"
for vmid in configuration:
    vm = configuration[vmid]
    if "mmap" in vm:
	# let's allocate ROM/RAM space in area defined by platform file
	newmmap = []
	for region in vm["mmap"]:
		if region[4] == 0:
		    newmmap.append((region[0],region[1],region[2],region[3]))
		    if "w" not in region[2]:
			offset = 0
			if region[3] is not None:
			    offset = int(region[3],0) - int(region[0],0)
		    continue
		paddr = kphys(region[0])
		size = round_region(region[1], region[3])
		if region[4] == 1:
		    newregion, offset = allocate_region(paddr, size, region[2], region[3], 0)
		else:
		    newregion, offsetrw = allocate_region(paddr, size, region[2], region[3], 1)
		newmmap.append(newregion)
	vm["mmap"] = newmmap
	print "\nVM",str(vmid),":"
	for region in vm["mmap"]:
	    print "(%10s,\t%10s,\t%10s,\t%10s)" % region
    if "file" in vm:
	cmdname = os.path.splitext(vm["file"])[0]+'.cmd'
	if os.path.isfile(cmdname):
	    fl = open(cmdname)
	    for line in fl:
		if '$OFFSET' in line:
		    line = line.replace("$OFFSET", hex(offset))
		objcopy.append(line)
	    fl.close()
print ""
#
# create a final shell cmd to all VM binaries + root merge
#
flname = sys.argv[2] + '.objcopy'
fl = open(flname, "w")
for line in objcopy:
    print >>fl, line,
print >>fl, "srec_cat a.out.srec devcfg.srec",
for vmid in configuration:
    vm = configuration[vmid]
    if "file" in vm:
	filename = os.path.splitext(vm["file"])[0]+".srec"
	print >>fl, " ", filename,
print >>fl, " >a.merged.srec"
fl.close()
os.system("chmod +x " + flname)

#
# OK, now begin an essential part of configuration
#
create_pte_lists()
for vm in sorted(configuration):
    split_ptes(configuration[vm])
    join_pairs(configuration[vm])
    create_pte_tree(configuration[vm])
# create PTE tables
ofile = open("pte.c","w")
print >>ofile, '#include "tlb.h"'
output_emulator_list(ofile)
label_list = {}
for vm in sorted(configuration):
    label = output_tlb_tables(configuration[vm], ofile)
    label_list[vm] = label
print >>ofile, "\nextern long absent_vm_tlbtree[];"
print >>ofile, "\nstruct cpte_nd const * const cpt_base[] = {"
for i in range(0, 8):
    if i in configuration:
	print >>ofile, "\t(struct cpte_nd *)&%s," % (label_list[i])
    else:
	print >>ofile, "\t(struct cpte_nd *)&absent_vm_tlbtree,"
print >>ofile, "};"
# creat IC tables
ofile = open("ic-tables.c","w")
print >>ofile, '#include "ic.h"'
print >>ofile, '#include "tlb.h"'
print >>ofile, '#include "irq.h"'
output_emulator_lists(ofile)
platform.output_ic_tables(configuration,ofile)

