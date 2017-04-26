#!/usr/bin/python

import sys
import os

filename = sys.argv[1]
cmd = "mips-mti-linux-gnu-nm " + filename
f = os.popen(cmd)
for line in f:
    line2 = line.strip()
    if line2 == '':
	continue
    words = line2.split()
    if words[2] == "data_section":
	address = words[0]
	str = "0x" + str(address)
	print str
	exit(0)

