'''
Created on Oct 4, 2010

@author: hsc
'''

from scanner_baseclass import ScannerBaseclass
import memtestcext
import physmem

class LinearCExtScanner(ScannerBaseclass):
    '''
    Scans a memory region (mmap) with linear complexity, utilizing a C extension for speedup
    '''

    def __init__(self, reporting):
        '''
        Constructor
        reporting.report_bad_memory(bad_offset, expected_value, actual_value)
        '''
        self.reporting = reporting

    @staticmethod
    def name():
        return "Linear time test (CExt)"

    @staticmethod
    def shortname():
        return "linear-cext"

    def test_single(self, region, offset, physaddr):
        """
        - region supports __getitem(x)__ and __setitem(x,b)__, with b being casted to a byte 
        - offset is the first byte tested, The bytes region[offset..offset+PAGE_SIZE-1] are tested
        
        return: None if page ok, offset of an error if test failed
        """
        
        error_offset = None
        ret = memtestcext.lineartest(region.get_mmap(), offset, physmem.PAGE_SIZE)

        # TODO: do proper reporting
        if ret >= 0:
            self.reporting.report_bad_memory(physaddr + ret, 0x55, 0xaa)
            error_offset = offset + ret

        return error_offset

ScannerBaseclass.register(LinearCExtScanner)
