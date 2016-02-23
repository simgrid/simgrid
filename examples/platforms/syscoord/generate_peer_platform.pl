#!/usr/bin/env perl

# Copyright (c) 2011, 2014, 2016. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

use strict;
use warnings;

my $toversion=4;
my $nb_peer = $ARGV[0];
my $i;

print "<?xml version='1.0'?>\n";
print "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">\n";
print "<platform version=\"$toversion\">\n";

print "\n<config id=\"General\">\n";
print "\t<prop id=\"coordinates\" value=\"yes\"></prop>\n";
print "</config>\n\n";

print "\t<AS id=\"AS0\" routing=\"Vivaldi\">\n";

my $line;

open SITES_LIGNE, $ARGV[0] or die "Unable to open $ARGV[1]\n";
while(defined($line=<SITES_LIGNE>))
{
    #278 7.2 -9.4 h 2.3 
    if($line =~ /^(.*) (.*) (.*) h (.*)$/)
    {
	print "\t\t<peer id=\"peer-$1\" coordinates=\"$2 $3 $4\" speed=\"730Mf\" ";
	print "bw_in=\"13.38MBps\" bw_out=\"1.024MBps\" lat=\"500us\"/>\n";
    }
}			
print "\t</AS>\n";
print "</platform>\n";
