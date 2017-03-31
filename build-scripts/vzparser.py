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

COMMENT_CHAR = '#'
OPTION_CHAR =  '='
KB = 1024
MB = (1024 * KB)

device_lib = {}
configuration = {}
objcopy = []

romaddress = None
romend = None
ramaddress = None
ramend = None
eretpage = None
vzcode = None
vzentry = None
vzdata = None
vzstacktop = None

def parse_regblock(f, flagdefault, type):
    regblock = []
    for line in f:
	# First, remove comments:
	if COMMENT_CHAR in line:
	    # split on comment char, keep only the part before
	    line, comment = line.split(COMMENT_CHAR, 1)
	line = line.strip()
	if line == '.':
	    break
	try:
	    address, size, roflag, physaddr = line.split()
	except:
	    try:
		address, size, roflag = line.split()
	    except:
		address, size = line.split()
		roflag = flagdefault
	    physaddr = address
	if (not "r" in roflag) and (not "w" in roflag) and \
	    (not "x" in roflag):
	    roflag = roflag + flagdefault
	regblock.append((address, size, roflag, physaddr, type))
    return regblock

def parse_device(f):
    device = {}
    for line in f:
	# First, remove comments:
	if COMMENT_CHAR in line:
	    # split on comment char, keep only the part before
	    line, comment = line.split(COMMENT_CHAR, 1)
	if '=' in line:
	    key, value = line.split('=', 1)
	    key = key.strip()
	    value = value.strip()
	    if (key == "regs") or (key == "registers"):
		if value == '':
		    regblock = parse_regblock(f, "rwd", 0)
		else:
		    try:
			address, size, roflag, physaddr = value.split()
		    except:
			try:
			    address, size, roflag = value.split()
			except:
			    address, size = value.split()
			    roflag = "rwd"
			physaddr = address
		    regblock = [(address, size, roflag, physaddr, 0)]
		device["regblock"] = regblock
		continue
	    device[key] = value
	else:
	    line = line.strip()
	    if line == '':
		continue
	    if line == ".":
		return device
    return device

def parse_devicelib(filename):
    device = {}
    f = open(filename)
    for line in f:
	# First, remove comments:
	lline = line
	if COMMENT_CHAR in line:
	    # split on comment char, keep only the part before
	    line, comment = line.split(COMMENT_CHAR, 1)
	line = line.strip()
	if line == '':
	    continue
	if line == ".device":
	    device = parse_device(f)
	    if device == None:
		print "1 Configuration library error, last device has no '.', file", f.name
		exit()
	    if "name" in device:
		devicename = device["name"]
	    else:
		print "2 Configuration library error, device name is absent, file", f.name
		exit()
	    device_lib[devicename] = device
	    device = {}
	else:
	    print "3 Configuration library file parse error, line ", lline
	    exit()
    f.close()
    return

def parse_mmap(f, mmap):
    for line in f:
	# First, remove comments:
	lline = line
	if COMMENT_CHAR in line:
	    # split on comment char, keep only the part before
	    line, comment = line.split(COMMENT_CHAR, 1)
	line = line.strip()
	if line == '':
	    continue
	if line == '.':
	    return mmap
	if '=' in line:
	    key, value = line.split('=', 1)
	    key = key.strip()
	    value = value.strip()
	    if (key == "code") or (key == "rom"):
		if value == '':
		    if key == "code":
			regblock = parse_regblock(f, "rx", 0)
		    else:
			regblock = parse_regblock(f, "rx", 1)
		else:
		    try:
			address, size, flag, physaddr = value.split()
		    except:
			try:
			    address, size, flag = value.split()
			except:
			    address, size = value.split()
			    flag = "rx"
			physaddr = address
		    if (not "r" in flag) and (not "w" in flag) and \
			(not "x" in flag):
			flag = flag + "rx"
		    if key == "code":
			regblock = [(address, size, flag, physaddr, 0)]
		    else:
			regblock = [(address, size, flag, physaddr, 1)]
		mmap.extend(regblock)
		continue
	    if (key == "data") or (key == "ram"):
		if value == '':
		    if key == "data":
			regblock = parse_regblock(f, "rw", 0)
		    else:
			regblock = parse_regblock(f, "rw", 2)
		else:
		    try:
			address, size, flag, physaddr = value.split()
		    except:
			try:
			    address, size, flag = value.split()
			except:
			    address, size = value.split()
			    flag = "rw"
			physaddr = address
		    if (not "r" in flag) and (not "w" in flag) and \
			(not "x" in flag):
			flag = flag + "rw"
		    if key == "data":
			regblock = [(address, size, flag, physaddr, 0)]
		    else:
			regblock = [(address, size, flag, physaddr, 2)]
		mmap.extend(regblock)
		continue
	    if key == "rodata":
		if value == '':
		    regblock = parse_regblock(f, "r", 0)
		else:
		    try:
			address, size, flag, physaddr = value.split()
		    except:
			try:
			    address, size, flag = value.split()
			except:
			    address, size = value.split()
			    flag = "r"
			physaddr = address
		    regblock = [(address, size, flag, physaddr, 0)]
		mmap.extend(regblock)
		continue
	    print "4 Configuration file parse error, line ", lline
	    exit()
    return mmap

def parse_vm(f, vm, extension):
    for line in f:
	# First, remove comments:
	lline = line
	if COMMENT_CHAR in line:
	    # split on comment char, keep only the part before
	    line, comment = line.split(COMMENT_CHAR, 1)
	line = line.strip();
	if '=' in line:
	    key, value = line.split('=', 1)
	    key = key.strip()
	    value = value.strip()
	    if key == "device":
		if value in device_lib:
		    device = device_lib[value]
		    if "device" in vm:
			vm["device"].append(device)
		    else:
			vm["device"] = [device,]
		else:
		    print "5 Configuration file error, device absent in library", lline
		    exit()
	    elif key == "entry":
		if "entry" not in vm:
		    vm["entry"] = value
	    elif key == "elf":
		if "file" in vm:
		    print "101 Configuration file parse error - duplicate 'elf/'binary', line ", lline
		    exit()
		# start extracting mmap from ELF
		filename = os.path.splitext(value)[0]+'.'+extension
		# in 1st step ("map") map files are created unconditionally
		if (extension == "map") or not os.path.isfile(filename):
		    status = os.system("build-scripts/map-extract.py" + " " + value)
		    if status != 0:
			print "build-scripts/map-extract.py returns status",status
			exit()
		else:
		    binstatinfo = os.stat(value)
		    mapstatinfo = os.stat(filename)
		    if binstatinfo.st_mtime > mapstatinfo.st_mtime:
			status = os.system("build-scripts/map-extract.py" + " " + value)
			if status != 0:
			    print "build-scripts/map-extract.py returns status",status
			    exit()
		fl = open(filename)
		vm = parse_vm(fl, vm, extension)
		fl.close()
		vm["elf"] = value
		vm["file"] = value
	    elif key == "binary":
		# just reference to binary file, map is written manually/in configuration file
		if "file" in vm:
		    print "102 Configuration file parse error - duplicate 'elf/'binary', line ", lline
		    exit()
		vm["file"] = value
	    else:
		vm[key] = value
	else:
	    if line == '':
		continue
	    if line == ".":
		return vm
	    else:
		if line == ".mmap":
		    mmap = []
		    if "mmap" in vm:
			mmap = vm["mmap"]
		    mmap = parse_mmap(f, mmap)
		    vm["mmap"] = mmap
		elif line == ".device":
		    device = parse_device(f)
		    if "device" in vm:
			vm["device"].append(device)
		    else:
			vm["device"] = [device,]
		#elif line == ".externalirq":
		#    eirqs = parse_eirq(f)
		else:
		    print "6 Configuration file parse error, line ", lline
		    exit()
    f.close()
    return vm

def parse_config_file(filename, extension):
    vmid = "0"
    vm = {}
    f = open(filename)
    for line in f:
	# First, remove comments:
	lline = line
	if COMMENT_CHAR in line:
	    # split on comment char, keep only the part before
	    line, comment = line.split(COMMENT_CHAR, 1)
	line = line.strip();
	if line == '':
	    continue
	if line == ".vm":
	    vm = {}
	    vm = parse_vm(f, vm, extension)
	    try:
		vmid = vm['id']
	    except:
		print "7 Configuration file error, vm has no id",
		exit()
	    configuration[int(vmid)] = vm
	else:
	    print "8 Configuration file parse error, line ", lline
	    exit()
    f.close()
    return vm

def parse_platform(arg):
    global romaddress, romend, ramaddress, ramend, eretpage, vzcode, vzentry, vzdata, vzstacktop
    f = open(arg)
    for line in f:
	# First, remove comments:
	if COMMENT_CHAR in line:
	    # split on comment char, keep only the part before
	    line, comment = line.split(COMMENT_CHAR, 1)
	line = line.strip()
	if '=' in line:
	    key, value = line.split('=', 1)
	    key = key.strip()
	    value = value.strip()
	    if key == "rom":
		romaddress, romend = value.split()
		romaddress = int(romaddress,0)
		romend = int(romend,0)
	    if key == "ram":
		ramaddress, ramend = value.split()
		ramaddress = int(ramaddress,0)
		ramend = int(ramend,0)
	    if key == "eret_page":
		eretpage = int(value,0)
	    if key == "vzcode":
		vzcode = int(value,0)
	    if key == "vzentry":
		vzentry = int(value,0)
	    if key == "vzdata":
		vzdata = int(value,0)
	    if key == "vzstacktop":
		vzstacktop = int(value,0)
    f.close()

def kphys(arg):
    return int(arg,0) & 0x1fffffff

def round_region(size, align):
    return (int(size,0) + (4*KB - 1)) & ~(4*KB - 1)

def allocate_region(paddr, size, flags, align, ramflag):
    global romaddress, romend, ramaddress, ramend, vzcode, vzentry, vzdata, vzstacktop
    if ramflag == 0:
	newpaddr = romaddress
	newpaddr = (newpaddr + (int(align,0) - 1)) & ~(int(align,0) - 1)
	romaddress = newpaddr + size
	if romaddress >= romend:
	    print "ROM space is exhausted with ",paddr," region"
	    exit()
    else:
	newpaddr = ramaddress
	newpaddr = (newpaddr + (int(align,0) - 1)) & ~(int(align,0) - 1)
	ramaddress = newpaddr + size
	if ramaddress >= ramend:
	    print "RAM space is exhausted with ",paddr," region"
	    exit()
    return (hex(paddr), hex(size), flags, hex(newpaddr)), newpaddr - paddr

