#!/usr/bin/perl

use Time::Local 'timelocal_nocheck';

my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst);

my $delim = "\t";
while (<>) {
	chomp;
	# 1080911183913 32 01 3 1.5 W 11.5 A 12.5 V 
	if (/^\d(\d{2})(\d{2})(\d{2})(\d{2})(\d{2})(\d{2})\s(\d{2})\s(\d{2})\s(\d)\s([\d.]+)\sW/) {
		$year = 2000 + $1;
		$mon = $2*1;
		$mday = $3*1;
		$hour = $4*1;
		$min = $5*1;
		$sec = $6*1;
		$n1 = $7;
		$interval = $8*1;
		print(join($delim, ("DateTime","Watt","Amp","Volt"))."\n");
	} else {
		# 0155.8 00.822 225 
		if (/^(\d{4}\.\d)\s([\d.]+)\s(\d{3})/) {
			my $timstamp = sprintf("%4d-%02d-%02d %02d:%02d:%02d",$year,$mon,$mday,$hour,$min,$sec);
			printf(join($delim, ($timstamp,$1,$2,$3))."\n");
			$min += $interval;
			my $time = timelocal_nocheck($sec,$min,$hour,$mday,$mon-1,$year-1900);
			($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime($time);
			$mon++;
			$year += 1900;
		} else {
			warn("# Malfomed data: '" + $_ + "'\n");
		}
	}
}
