#!/usr/bin/python

import os

from pprint import pprint
import pprint
import sys

import vzparser
from vzparser import *

import importlib

KB = 1024

class MyPrettyPrinter(pprint.PrettyPrinter):
    def format(self, object, context, maxlevels, level):
	if isinstance(object, int):
	    return hex(object), True, False
	else:
	    return pprint.PrettyPrinter.format(self, object, context, maxlevels, level)

def parse_config(filename):
    vm = parse_config_file(filename, "map")

sys.path.append("platforms")
platform = importlib.import_module(sys.argv[3])
parse_board("boards/" + sys.argv[1],platform)
parse_devicelib("device.lib")
parse_config(sys.argv[2])
#mpp2 = MyPrettyPrinter()
#mpp2.pprint(configuration)

