#!/usr/bin/env perl

# Copyright (c) 2012-2021. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

use strict;
use warnings;

if($#ARGV!=1) {
    die "Usage: perl update_tesh.pl <directory> <old.tesh>\n";
}

my($directory)=$ARGV[0];
my($old)=$ARGV[1];

chdir("$directory");

open SH_LIGNE, $old or die "Unable to open $old. $!\n";

my($line);
my($line_exec);
my($l);
my($tmp);

while(defined($line=<SH_LIGNE>))
{
	if($line =~ /^\$(.*)$/)
	{
		$line_exec = $line;
		$line =~ s/\$\{srcdir\:\=\.\}/./g;
		$line =~ s/\(/\\(/g;
		$line =~ s/\)/\\)/g;
		$line =~ s/^\$\ */.\//g;
		$line =~ s/^.\/lua/lua/g;
		$line =~ s/^.\/ruby/ruby/g;
		$line =~ s/--log=([^ ]*)/--log="$1"/g;
		print "$line_exec";
		chomp $line;
		open (FILE, "$line 2>&1|");
		while(defined($l=<FILE>))
		{
		chomp $l;
		print "\> $l\n";
	    	}
		close(FILE);
    	}
	else
	{
		if($line =~ /^\>(.*)$/)
		{
		}
		else
		{
		print "$line";
		}
	}
}

close(SH_LIGNE);
