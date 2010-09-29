#! /usr/bin/perl

# This script updates the simgrid XML file passed as argument (modification in place)
# It is built to do the conversion from a XML platfrom version 2 to a version 3

# Copyright (C) 2006-2010. The SimGrid team. All rights reserved.
#
# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.

use strict;

my($line);
my($command);
my($Filecount)=1;
my($file);
my($filename_old);
my($AScount);

if($#ARGV!=0) {
    die "Error:  Wrong number of parameters\nUsage:\tperl transform_platform.perl [platform_v2.xml]\n";
}

my($file)=$ARGV[0];

open FILE, "$file" or die "Error:  Unable to open file: \"$file\".\n$!\n";
while(defined($line=<FILE>))
{ 
	  $AScount=0;
	  if($line =~ /^(.*)<platform version=\"2\"(.*)$/) 
	  {
		  print "<platform version=\"3\">\n";
		  print "<AS  id=\"AS$AScount\"  routing=\"Full\">\n";
		  $AScount++;
	  }
	  else
	  {
		  if($line =~ /^(.*)<\/platform(.*)$/) 
		  {
			print "<\/AS>\n";
			print "$line";
		  }
		  else
		  {
			print "$line";
		  }
	  }
}
close(FILE);