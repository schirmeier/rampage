'''
Created on Apr 5, 2011

@author: ingo
'''
import physmem

class ScannerBaseclass(object):
    '''
    base class for all memory tests
    '''

    scanner_classes = []

    @staticmethod
    def register(cls):
        ScannerBaseclass.scanner_classes.append(cls)

    def __init__(self, reporting):
        '''
        Constructor
        reporting.report_bad_memory(bad_offset, expected_value, actual_value)
        '''
        self.reporting = reporting

    def name(self):
        raise NotImplementedError

    def shortname(self):
        raise NotImplementedError

    def test(self, region, offset, length, physaddr):
        '''
        calls test_single for each PAGE_SIZE-byte block in (offset, offset+length-1)
        returns a list of bad offsets; physaddr is a list of physical start addresses for each block
        '''
        error_offsets = []
        i = 0

        while length >= physmem.PAGE_SIZE:
            res = self.test_single(region, offset, physaddr[i])
            if res != None:
                error_offsets.append(res)

            length -= physmem.PAGE_SIZE
            offset += physmem.PAGE_SIZE
            i += 1

        return error_offsets

    def test_single(self, region, offset):
        raise NotImplementedError
