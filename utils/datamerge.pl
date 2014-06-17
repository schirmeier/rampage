#!/usr/bin/perl
#
# Reads memtest_status (/tmp), kpageflags (/proc) and the output of pfnspy.pl.
# Calculates which untested page frames are not flagged as "reserved" and were
# identified as user-space frames (or not).

use warnings;
use strict;
use feature ':5.10';

if (scalar(@ARGV) != 3) {
    say "Usage: $0 memtest_status kpageflags pfnlist.txt";
    exit 1;
}

# read memtest_status
sub read_memtest_status {
    my $fname = shift;
    my $buffer;

    open IN,$fname or die "Can't open $fname: $!";
    read IN,$buffer,(-s $fname);
    close IN;

    my $i=0;
    my $entrylen = 4*8+2*4;

    my @untested_pfns;

    while (length($buffer) > $i*$entrylen) {
        my ($s1, $s2, $f1, $f2, $c1, $c2, $a1, $a2, $num_errors, $last_method) =
            unpack("LLLLLLLLLL", substr($buffer,$i*$entrylen,$entrylen));

        # Grr, das Perl auf kos kann kein Q
        my $last_succ       = $s1 | ($s2 << 32);
        my $last_fail       = $f1 | ($f2 << 32);
        my $last_claim_time = $c1 | ($c2 << 32);
        my $last_attempt    = $a1 | ($a2 << 32);

        if ($last_succ == 0 && $last_attempt == 0) {
            push @untested_pfns, $i;
        }

        #printf("%.2f [%08x]: %d %d %d\n",($i*4)/1024,$i,$last_succ?1:0,$last_attempt?1:0, $num_errors);
        $i++;
    }

    return sort @untested_pfns;
}

# read kpageflags and return array of PFNs with reserved flag
sub read_kpageflags {
    my $fname = shift;
    my $buffer;

    open IN,$fname or die "Can't open $fname: $!";
    read IN, $buffer, (-s $fname);
    close IN;

    my $pfn = 0;
    my @reserved_pfns;

    while (length($buffer) > 8 * $pfn) {
        my $flags = unpack("Q", substr($buffer, 8*$pfn, 8));

        if ($flags & (1 << 32)) {
            push @reserved_pfns, $pfn;
        }

        $pfn++;
    }

    return sort @reserved_pfns;
}

# read pfnspy output
sub read_pfnspy {
    my $fname = shift;
    my @user_pages;

    open IN, $fname or die "Can't open $fname: $!";
    while (<IN>) {
        chomp;
        if (/^([0-9a-f]+)-([0-9a-f]+)$/) {
            for (my $i=hex $1; $i <= hex $2; $i++) {
                push @user_pages, $i;
            }
        } elsif (/^([0-9a-f]+)$/) {
            push @user_pages, hex $1;
        } else {
            say STDERR "Error parsing pfnspy output: unexpected line >$_<";
            exit 2;
        }
    }
    close IN;

    return @user_pages;
}

# report a PFN list in condensed format using ranges
sub report_list {
    my $prev = shift;
    my $prevshown = $prev;
    printf "%08x", $prev;

    foreach (@_) {
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
}

my @untested = read_memtest_status($ARGV[0]);
my @reserved = read_kpageflags($ARGV[1]);
my @user_pfns = read_pfnspy($ARGV[2]);

# calculate list of pages that are not reserved but unclaimed
my %unclaimed;

foreach (@untested) {
    $unclaimed{$_} = 1;
}

foreach (@reserved) {
    delete $unclaimed{$_};
}

my @unclaimed = sort { $a <=> $b } keys %unclaimed;

# calculate list of pages that are unclaimed and (not) part of user space
my @in_user;
my %not_user;

foreach (@unclaimed) {
    $not_user{$_} = 1;
}

foreach (@user_pfns) {
    delete $not_user{$_};
    push @in_user, $_ if exists($unclaimed{$_});
}

@in_user = sort { $a <=> $b } @in_user;
my @not_user = sort { $a <=> $b } keys %not_user;

# output results
say "===== Unclaimed pages not part of user space: ", scalar(@not_user);
report_list(@not_user);

say "===== Unclaimed pages part of user space: ", scalar(@in_user);
report_list(@in_user);

#say "===== test:";
#foreach ((sort { $a <=> $b } @reserved)[0..10]) {
#    say;
#}
