#!/usr/bin/perl

use warnings;
no warnings 'portable'; # silence warning for 64 bit hex numbers
use strict;
#use feature ':5.10';
use Fcntl qw/:DEFAULT :seek/;

opendir PROC,"/proc" or die "Failed to opendir /proc: $!";
my @dirs = readdir PROC;
close PROC;

my %user_pfns;

foreach my $pid (@dirs) {
    next unless $pid =~ /^\d+$/;

    # read /proc/$pid/maps to get a list of mapped virtual pages
    open MAP, "<", "/proc/$pid/maps" or die "Can't open /proc/$pid/maps: $!";

    my @ranges;

    while (<MAP>) {
        next unless /^([0-9a-f]+).([0-9a-f]+) /;
        my $startframe = (hex $1) / 4096; # FIXME: Assumes 4K pages
        my $endframe   = (hex $2) / 4096;
        push @ranges, [$startframe, $endframe];
    }

    close MAP;

    # read /proc/$pid/pagemap to get the PFN for each page
    sysopen PAGEMAP, "/proc/$pid/pagemap", O_RDONLY or die "Can't open /proc/$pid/pagemap: $!";

    foreach my $range (@ranges) {
        my $buffer;

        # read the current range from pagemap
        sysseek PAGEMAP, $$range[0] * 8, SEEK_SET
            or die "Failed to seek in /proc/$pid/pagemap: $!";
        my $cnt = sysread PAGEMAP, $buffer, ($$range[1] - $$range[0]) * 8;
        if (!defined($cnt)) { # sysread may return 0
            die "Failed to read from /proc/$pid/pagemap: $!";
        }

        # decode frame information
        my @frameinfos = unpack("Q*", $buffer);
        if (scalar(@frameinfos) > 0 &&
            scalar(@frameinfos) != $$range[1] - $$range[0]) {
            print STDERR "WARNING: Got less frame information than requested at frame $$range[0]\n";
        }
        foreach my $fi (@frameinfos) {
            if ($fi & (1 << 63)) { # check if page is present
                $user_pfns{$fi & ((1 << 55) - 1)} = 1;
            }
        }
    }

    close PAGEMAP;
}

# dump list of all userspace PFNs
my @pfns = sort { $a <=> $b } keys %user_pfns;

my $prev = shift @pfns;
my $prevshown = $prev;
printf "%08x", $prev;

foreach (@pfns) {
    if ($_ != $prev + 1) {
        printf "-%08x", $prev unless $prev == $prevshown;
        printf "\n%08x", $_;
        $prevshown = $_;
    }
    $prev = $_;
}

if ($prev == $prevshown) {
    print "\n";
} else {
    printf "-%08x\n",$prev;
}



