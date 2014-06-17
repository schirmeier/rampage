#!/usr/bin/perl

use strict;
use warnings;
use feature ':5.10';

if (scalar(@ARGV) != 4) {
    say "Usage: $0 coremark_dir threads iterations timelimit";
    say "       (iterations=0 -> automatic choice)";
    exit 1
}

my $coremark_dir = shift;
my $threads      = shift;
my $iterations   = shift;
my $timelimit    = shift;

my @makeargs = ("SHELL=/bin/bash");

if ($threads != 1) {
    push @makeargs, "XCFLAGS=-DMULTITHREAD=$threads -DUSE_PTHREAD";
}

if ($iterations != 0) {
    push @makeargs, "ITERATIONS=$iterations";
}

chdir($coremark_dir);

# first run with compilation
my $starttime = time;

system "make","REBUILD=1",@makeargs;

while (time < $starttime + $timelimit) {
    # later runs without compilation
    system "make",@makeargs;
    last if $? != 0;
}
