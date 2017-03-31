#!/usr/bin/python

import os

from pprint import pprint
import pprint
import sys

import vzparser
from vzparser import *

KB = 1024

class MyPrettyPrinter(pprint.PrettyPrinter):
    def format(self, object, context, maxlevels, level):
	if isinstance(object, int):
	    return hex(object), True, False
	else:
	    return pprint.PrettyPrinter.format(self, object, context, maxlevels, level)

def parse_config(filename):
    vm = parse_config_file(filename, "map")

parse_platform(sys.argv[1])
parse_devicelib("device.lib")
parse_config(sys.argv[2])
#mpp2 = MyPrettyPrinter()
#mpp2.pprint(configuration)

