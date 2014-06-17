#!/usr/bin/python

from scanner_baseclass import ScannerBaseclass
from memtest86p import *
#import linear
#import linear_cext
#import quadratic
import mmap

map_size = 1*1024*1024

class FakeRegion:
    def __init__(self, mem):
        self.mem = mem

    def get_mmap(self):
        return self.mem

class FakeReporting:
    def report_bad_memory(offset, expected, actual):
        print "Report offset %d: 0x%x expected, 0x%x read" % (offset, expected, actual)

map = mmap.mmap(-1, map_size)
region = FakeRegion(map)
reporter = FakeReporting()

for cls in sorted(ScannerBaseclass.scanner_classes):
    print "%s: %s" % (cls.shortname(), cls.name())
    tester = cls(reporter)
    result = tester.test(region, 0, map_size)
    print len(result)


