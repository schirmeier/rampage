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

import math
import time
import frame
import physmem


class SlowSchedulerFactory():
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
       return SlowScheduler( self.physmem_device, self.frame_tests, frame_stati, self.kpageflags, self.kpagecount, self.timestamping, self.reporting, self.max_untested_age)

    def name(self):
        return "Slow Blockwise Allocation Scheduler"

class SlowScheduler(object):
    '''
    This scheduler iterates over all frames in the status and tests the frame, based on the evaluation function, testing very slowly
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
        return "Slow Blockwise Allocation Scheduler"

    # pfns_at_once: number of PFNs to test between sleeps
    # time_limit: (approximate) time limit for this run in seconds
    # full_time: (approximate) time for testing all pfns in range in seconds (note: every pfn is attempted only once even if time_limit is higher)
    def run(self, first_frame,last_frame, allowed_sources, pfns_at_once=0, time_limit=0, full_time=0):
        # page_blocks are 512 pages on the current test box
        max_blocksize = 512
        max_non_matching = 512

        cur_non_matching = 0

        attempted_pfns = 0
        total_pfns = last_frame-first_frame

        if pfns_at_once == 0:
            pfns_at_once = total_pfns

        time_per_xblock = full_time / float(total_pfns) * pfns_at_once

        test_start_time = time.time()
        block = []
        block_start_time = test_start_time

        print "pfns at once %d in %d seconds, limit %d seconds" % (pfns_at_once,time_per_xblock,time_limit)
        tested = 0
        last_tested = 0

        for pfn in xrange(first_frame, last_frame):
            frame_status = self._pfn_status(pfn)

            if (self.should_test(frame_status)):
                block.append(frame_status)
                cur_non_matching = 0
            else:
                cur_non_matching += 1

            # Test current block if
            #  a) it contains as many pfns as we want to test at once
            #  b) it contains exactly as many pfns as we are still allowed in this iteration
            #  c) we reached the limit of not-to-be-tested pages
            #  d) the current pfn loop iteration is the last one
            if ((len(block) == max_blocksize) or
                (len(block) == pfns_at_once - attempted_pfns) or
                (cur_non_matching >= max_non_matching) or
                (pfn == last_frame-1)):
                if (len(block) > 0 ):
                    attempted_pfns += len(block)
                    tested += self.test_frames_and_record_result(block, allowed_sources)
                    block = []
                cur_non_matching = 0

            if attempted_pfns >= pfns_at_once:
                # end test if the time limit is reached
                if (time_limit != 0) and (time.time() - test_start_time > time_limit):
                    print "---time limit per test reached"
                    break

                # sleep for a while
                if len(block) > 0:
                    print "***FEHLER*** Blocklaenge != 0 bei Schlafversuch!"

                sleeptime = time_per_xblock - (time.time() - block_start_time)

                # exit early if sleeping would exceed our time limit
                if (time_limit != 0) and (time.time() - test_start_time + sleeptime > time_limit):
                    print "---want to sleep %d seconds, but that would exceed the time limit" % sleeptime
                    break

                if sleeptime > 0:
                    print ">>> sleeping for %d seconds (time since start: %d, got %d/%d pfns)" % (sleeptime, time.time() - test_start_time, tested-last_tested, attempted_pfns)
                    time.sleep(sleeptime)
                    last_tested = tested

                block_start_time = time.time()
                attempted_pfns = 0

        return tested


    def should_test(self,frame_status):
        # Don't test pages marked as PG_Reserved or PG_Nopage
        # FIXME: Wie kommt man an KPageFlags.RESERVED?
        if frame_status.flags.any_set_in((1 << 32) | (1 << 20)):
            return False

        now = self.timestamping.timestamp()
        time_untested = now - frame_status.last_successful_test

        return  (time_untested > self.max_untested_age )

    def test_frames_and_record_result(self,frame_stati, allowed_sources):

        pfns = [frame_status.pfn for  frame_status in frame_stati]
#        print "TO TEST: ", pfns

        status_by_pfn = dict([ (frame_status.pfn, frame_status) for  frame_status in frame_stati])

        results = self._claim_pfns(pfns, allowed_sources)

        num_frames_claimed = 0
        claimed_physaddr = []

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

        for frame in results:
            if frame.pfn in status_by_pfn:
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


