#!/usr/bin/perl

use strict;
use warnings;
use File::Copy;
use feature ':5.10';

if (scalar(@ARGV) != 4) {
    say "Usage: $0 coremark_dir threads iterations outputfile";
    say "       (iterations=0 -> automatic choice)";
    exit 1
}

my $coremark_dir = shift;
my $threads      = shift;
my $iterations   = shift;
my $outfile      = shift;

my @makeargs = ("REBUILD=1","SHELL=/bin/bash");

if ($threads != 1) {
    push @makeargs, "XCFLAGS=-DMULTITHREAD=$threads -DUSE_PTHREAD";
}

if ($iterations != 0) {
    push @makeargs, "ITERATIONS=$iterations";
}

chdir($coremark_dir);
system "make",@makeargs;
copy("run1.log", $outfile);

