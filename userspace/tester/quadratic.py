'''
This source code is distributed under the MIT License

Copyright (c) 2010, Jens Neuhalfen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
'''
from scanner_baseclass import ScannerBaseclass
import physmem

class QuadraticScanner(ScannerBaseclass):
    '''
    Scans a memory region (mmap) with n**2  complexity
    '''


    def __init__(self, reporting):
        '''
        Constructor
        reporting.report_bad_memory(bad_offset, expected_value, actual_value)
        '''
        self.reporting = reporting

    @staticmethod
    def name():
        return "Quadratic runtime test"

    @staticmethod
    def shortname():
        return "quadratic"
    
    def test_single(self, region, offset, physaddr):
        """
        - region supports __getitem(x)__ and __setitem(x,b)__, with b being casted to a byte 
        - offset is the first byte tested, The bytes region[offset..offset+PAGE_SIZE-1] are tested
        
        return: None if page ok, offset of an error if test failed
        """
        
        error_offset = None
        last_element = offset+physmem.PAGE_SIZE
        
        # Test all ZEROes - first reset
        for index in xrange(offset, last_element):
            region[index] = 0
        
        for index in xrange(offset, last_element):
            v =  region[index]
            if not (v == 0):
                self.reporting.report_bad_memory(physaddr + index - offset, 0, v)
                error_offset = index

        # Test all ONEs -- quadratic runtime (linear number of writes, quadratic number of reads)
        for index in xrange(offset, last_element):
            region[index] = 0xff
            
            for before in xrange(offset, index + 1):
                v =  region[before]
                if not (v == 0xff):
                    self.reporting.report_bad_memory(physaddr + before - offset, 0xff, v)
                    error_offset = before
            
            for after in xrange( index + 1, last_element):
                v =  region[after]
                if not (v == 0x00):
                    self.reporting.report_bad_memory(physaddr + after - offset, 0x00, v)
                    error_offset = after
                
         
        # Test all ZEROes - second reset
        for index in xrange(offset, last_element):
            region[index] = 0
        
        for index in xrange(offset, last_element):
            v =  region[index]
            if not (v == 0):
                self.reporting.report_bad_memory(physaddr + index - offset, 0, v)
                error_offset = index
   
        return error_offset

ScannerBaseclass.register(QuadraticScanner)
