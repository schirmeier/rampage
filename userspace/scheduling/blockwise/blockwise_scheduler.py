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

import frame
import physmem


class SimpleBlockwiseSchedulerFactory():
    def __init__(self, physmem_device,frame_tests,  kpageflags, kpagecount, timestamping, reporting, untested_age):
        '''
        Constructor
        '''
        self.kpageflags = kpageflags
        self.kpagecount = kpagecount
        self.physmem_device = physmem_device
        self.frame_tests = frame_tests
        self.timestamping = timestamping
        self.max_untested_age = untested_age
#        self.max_untested_age = 0
        self.reporting =  reporting

    def new_instance(self, frame_stati):
       return SimpleBlockwiseScheduler( self.physmem_device, self.frame_tests, frame_stati, self.kpageflags, self.kpagecount, self.timestamping, self.reporting, self.max_untested_age)

    def name(self):
        return "Blockwise Allocation Scheduler"
    
class SimpleBlockwiseScheduler(object):
    '''
    This scheduler iterates over all frames in the status and tests the frame, based on the evaluation function
    '''

    def __init__(self, physmem_device,frame_tests, frame_stati, kpageflags, kpagecount, timestamping, reporting, untested_age):
        '''
        Constructor
        '''
        self.frame_stati = frame_stati
        self.kpageflags = kpageflags
        self.kpagecount = kpagecount
        self.physmem_device = physmem_device
        self.frame_tests = frame_tests
        self.timestamping = timestamping 
        self.max_untested_age = self.timestamping.seconds_to_timestamp(untested_age)
#        self.max_untested_age = 0
        self.reporting =  reporting

    def name(self):
        return "Blockwise Allocation Scheduler"
        
    def run(self, first_frame,last_frame, allowed_sources, unused1=0, unused2=0, unused3=0):
        
        # page_blocks are 512 pages on the current test box
        max_blocksize = 512
        max_non_matching = 512
        
        cur_non_matching = 0
        
        block = []
        tested = 0

        for pfn in xrange(first_frame, last_frame):
            frame_status = self._pfn_status(pfn)
            
            if (self.should_test(frame_status)):
                block.append(frame_status)
                cur_non_matching = 0
            else:
                cur_non_matching += 1
                
            if (len(block) == max_blocksize ) or (cur_non_matching >= max_non_matching): 
                if (len(block) > 0 ):   
                    tested += self.test_frames_and_record_result(block, allowed_sources)
                    block = []
                cur_non_matching = 0

        if (len(block) > 0  ):
                self.test_frames_and_record_result(block, allowed_sources)
                
        return tested
            
    def should_test(self,frame_status):
        # Don't test pages marked as PG_Reserved or PG_Nopage
        # FIXME: Wie kommt man an KPageFlags.RESERVED?
        if frame_status.flags.any_set_in((1 << 32) | (1 << 20)):
            return False

        now = self.timestamping.timestamp()
        time_untested = now - frame_status.last_successful_test
        
        return  (time_untested > self.max_untested_age )

    def  test_frames_and_record_result(self,frame_stati, allowed_sources):
        
        # extract list of PFNs to test
        pfns = [frame_status.pfn for  frame_status in frame_stati]
#        print "TO TEST: ", pfns
        
        # create dict that maps a PFN to its frame_status instance
        status_by_pfn = dict([ (frame_status.pfn, frame_status) for  frame_status in frame_stati])

        # attempt to claim
        results = self._claim_pfns(pfns, allowed_sources)

        num_frames_claimed = 0
        claimed_physaddr = []

        # gather list of claimed frames
        for frame in results:
            if  frame.pfn in status_by_pfn:
                if frame.is_claimed():    
                    num_frames_claimed += 1
                    claimed_physaddr.append(frame.pfn * physmem.PAGE_SIZE)

        # map all claimed frames and test them together
        error_offsets = []
        if num_frames_claimed > 0:
            with self.physmem_device.mmap(physmem.PAGE_SIZE * num_frames_claimed) as map:
                for test in self.frame_tests:
                    error_offsets += test.test(map, 0, physmem.PAGE_SIZE * num_frames_claimed, claimed_physaddr)

        # create mapping from vma-pagenumber to error offset
        errors_by_page = dict([ (ofs/physmem.PAGE_SIZE, ofs) for ofs in error_offsets])

        # iterate over claimer output list (i.e. all frames) and update their status
        for frame in results:
            if frame.pfn in status_by_pfn:
                # PFN was part of the original list of frames to test
                frame_status = status_by_pfn[frame.pfn]
                frame_status.last_claiming_attempt =  self.timestamping.timestamp()
                    
                if frame.is_claimed():
                    frame_status.last_claiming_time_jiffies = frame.allocation_cost_jiffies
                    frame_status.last_successful_claiming_method = frame.actual_source

                    if frame.vma_offset_of_first_byte/physmem.PAGE_SIZE in errors_by_page:
                        # page is in error list
                        frame_status.num_errors += 1
                        frame_status.last_failed_test = self.timestamping.timestamp()
                        self.physmem_device.mark_pfn_bad(frame.pfn)
                        self._report_bad_frame(frame.pfn)         
                    else:
                        # page is not in error list
                        frame_status.num_errors = 0
                        frame_status.last_successful_test = self.timestamping.timestamp()
                        self._report_good_frame(frame.pfn)

            else:
                # PFN wasn't in the list
                # The pfn field in a frame_status is set to 0 by the kernel module,
                # so we'd have to check the original request entry at the same index
                # to figure out which PFN failed here. The only time the "if frame.is_claimed()"
                # in the block above will not be true is when PFN 0 is part of
                # the list of pages passed as frame_stati and in that case a failure of
                # any PFN in the list will be interpreted as a failure to claim PFN 0.
                #
                # Original comment:
                # Hmm, better luck next time
                self._report_not_aquired_frame(frame_status.pfn)

        return num_frames_claimed

    def _report_not_aquired_frame(self, pfn):
        pass
            
    def _report_good_frame(self, pfn):
        self.reporting.report_good_frame(pfn)

    def _report_bad_frame(self, pfn):
        self.reporting.report_bad_frame(pfn)
    
    def _claim_pfns(self, pfns,allowed_sources):
        requests = []
        for pfn in pfns:
            requests.append(physmem.Phys_mem_frame_request(pfn, allowed_sources))
         
        self.physmem_device.configure(requests)
        config = self.physmem_device.read_configuration()
        
        if not ( len(requests) ==  len(config) ) :
            # Error
            raise RuntimeError("The result read from the physmem-device contains %d elements, but I expected %d elements! " %  (len(config),len(requests)))
        
        return config
            
    def _pfn_status(self, pfn):
        flags = self.kpageflags[pfn]
        mapcount = self.kpagecount[pfn]
        
        status = self.frame_stati[pfn]
        
        status.pfn = pfn
        status.flags =flags
        status.mapcount = mapcount
        
        return status
    

