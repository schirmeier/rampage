'''
Created on Oct 4, 2010

@author: hsc
'''

from scanner_baseclass import ScannerBaseclass
import memtest86pcext
import physmem

# metaprogramming =) - function returns a memory tester class named clsname that uses testfunc for testing
def build_class(clsname, lname, sname, testfunc):
    def name():
        return lname

    def shortname():
        return sname

    def test(self, region, offset, length, physaddr):
        """
        - region supports __getitem(x)__ and __setitem(x,b)__, with b being casted to a byte 
        - offset is the first byte tested, length is the length (1..X). The bytes region[offset..offset+(length -1)] are tested
        
        return: None if page ok, offset of all detected errors if test failed
        """
        
        error_offsets = testfunc(region.get_mmap(), offset, length)

        # TODO: do proper reporting
        if len(error_offsets) > 0:
            for item in error_offsets:
                frame = item/physmem.PAGE_SIZE
                self.reporting.report_bad_memory(physaddr[frame] + item % physmem.PAGE_SIZE, 0x55, 0xaa)

        return error_offsets

    return type(clsname, (ScannerBaseclass,), {
            'name': staticmethod(name),
            'shortname': staticmethod(shortname),
            'test': test
            })

Memtest86AddressBits = build_class("Memtest86AddressBits",
                                   "Memtest86+ #0: Address test, walking ones",
                                   "xmt86-addressbits", # renamed because it needs a really large memory block
                                   memtest86pcext.address_bits)
ScannerBaseclass.register(Memtest86AddressBits)

Memtest86AddressOwn = build_class("Memtest86AddressOwn",
                                  "Memtest86+ #1: Address test, own address",
                                  "mt86-addressown",
                                  memtest86pcext.address_own)
ScannerBaseclass.register(Memtest86AddressOwn)

Memtest86MovingInvOnesZeros = build_class("Memtest86MovingInvOnesZeros",
                                          "Memtest86+ #2: Moving inversions, ones & zeros",
                                          "mt86-mvinv10",
                                          memtest86pcext.mv_inv_onezeros)
ScannerBaseclass.register(Memtest86MovingInvOnesZeros)

Memtest86MovingInv8Bit = build_class("Memtest86MovingInv8Bit",
                                     "Memtest86+ #3: Moving inversions, 8 bit pattern",
                                     "mt86-mvinv8bit",
                                     memtest86pcext.mv_inv_8bit)
ScannerBaseclass.register(Memtest86MovingInv8Bit)

Memtest86MovingInvRand = build_class("Memtest86MovingInvRand",
                                     "Memtest86+ #4: Moving inversions, random pattern",
                                     "mt86-mvinvrand",
                                     memtest86pcext.mv_inv_rand)
ScannerBaseclass.register(Memtest86MovingInvRand)

Memtest86BlockMove = build_class("Memtest86BlockMove",
                                 "Memtest86+ #5: Block move, 80 moves",
                                 "mt86-blockmove",
                                 memtest86pcext.blockmove)
ScannerBaseclass.register(Memtest86BlockMove)

Memtest86MovingInv32Bit = build_class("Memtest86MovingInv32Bit",
                                      "Memtest86+ #6: Moving inversions, 32 bit pattern",
                                      "mt86-mvinv32bit",
                                      memtest86pcext.mv_inv_32bit)
ScannerBaseclass.register(Memtest86MovingInv32Bit)

Memtest86RandomNums = build_class("Memtest86RandomNums",
                                  "Memtest86+ #7: Random number sequence",
                                  "mt86-randseq",
                                  memtest86pcext.random_numbers)
ScannerBaseclass.register(Memtest86RandomNums)

Memtest86Mod20 = build_class("Memtest86Mod20",
                             "Memtest86+ #8: Modulo 20, Random pattern",
                             "mt86-mod20",
                             memtest86pcext.mod20)
ScannerBaseclass.register(Memtest86Mod20)
