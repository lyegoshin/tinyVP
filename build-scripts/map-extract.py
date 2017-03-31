#!/usr/bin/python

import sys
import os

readelf = "mips-linux-gnu-readelf"
if "MIPS-LINUX-GNU-" in os.environ:
    readelf = os.environ["MIPS-LINUX-GNU-"] + "readelf"
filename = sys.argv[1]
cmd = readelf + " -l " + filename
f = os.popen(cmd)
ofl = os.path.splitext(filename)[0] + '.map'
out = open(ofl,"w")
first = 1;
for line in f:
    line2 = line.strip()
    if line2 == '':
	continue
    words = line2.split()
    if (words[0] == "Entry") and (words[1] == "point"):
	entry = words[2]
	print >>out, "\tentry =", entry
    if words[0] == "LOAD":
	if first:
	    print >>out, "\t.mmap"
	    first = 0;
	virtaddr = words[2]
	size = words[5]
	type = ""
	i = 6
	flag = words[6]
	while "0" not in flag:
	    type = type + flag
	    i = i + 1
	    flag = words[i]
	# Ignore alignment from ELF - it is user VA
	#align = flag
	align = 0x1000  # 1 page
	if ("E" in type) and ("R" in type) and ("W" not in type):
	    print >>out, "\t\trom =",virtaddr,size,"rx",align
	if ("E" in type) and ("R" in type) and ("W" in type):
	    print >>out, "\t\tram =",virtaddr,size,"rwx",align
	elif ("R" in type) and ("W" in type):
	    print >>out, "\t\tram =",virtaddr,size,"rw",align
if first == 0:
    print >>out, "\t."
f.close()
out.close()

