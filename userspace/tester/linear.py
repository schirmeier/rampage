'''
Created on Jun 1, 2010

@author: jens
'''
from scanner_baseclass import ScannerBaseclass
import physmem

class LinearScanner(ScannerBaseclass):
    '''
    Scans a memory region (mmap) with linear complexity
    '''


    def __init__(self, reporting):
        '''
        Constructor
        reporting.report_bad_memory(bad_offset, expected_value, actual_value)
        '''
        self.reporting = reporting

    @staticmethod
    def name():
        return "Linear time test"

    @staticmethod
    def shortname():
        return "linear"

    def test_single(self, region, offset, physaddr):
        """
        - region supports __getitem(x)__ and __setitem(x,b)__, with b being casted to a byte 
        - offset is the first byte tested, The bytes region[offset..offset+PAGE_SIZE-1] are tested
        
        return: None if page ok, offset of an error if test failed
        """

        error_offset = None

        last_element = offset+physmem.PAGE_SIZE-1
        # Test all ZEROes
        for index in xrange(offset, last_element):
            region[index] = 0
        
        for index in xrange(offset, last_element):
            v =  region[index]
            if not (v == 0):
                self.reporting.report_bad_memory(physaddr+index-offset, 0, v)
                error_offset = index

        # Test all ONEs
        for index in xrange(offset, last_element):
            region[index] = 0xff
       
        for index in xrange(offset, last_element):
            v =  region[index]
            if not (v == 0xff):
                self.reporting.report_bad_memory(physaddr+index-offset, 0xff, v)
                error_offset = index

        # Test all ADDRESS
        for index in xrange(offset, last_element):
            region[index] = index % 0xff
       
        for index in xrange(offset, last_element):
            v =  region[index]
            if not (v == (index % 0xff)):
                self.reporting.report_bad_memory(physaddr+index-offset, (index % 0xff), v)
                error_offset = index
                
        return error_offset

ScannerBaseclass.register(LinearScanner)
