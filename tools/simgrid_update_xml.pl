#! /usr/bin/perl

# This script updates the simgrid XML file passed as argument (modification in place)
# It is built to do the conversion incrementally (even if for now, only 2 versions are defined)

# Copyright (C) 2006-2010. The SimGrid team. All rights reserved.
#
# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.

use strict;

my $fromversion=-1;
my $toversion=3;

my($output_string);

$ARGV[0] or die "simgrid_update_xml.pl <platform.xml>\n";
open INPUT, "$ARGV[0]" or die "Cannot open input file $ARGV[0]: $!\n";

$output_string .=  "<?xml version='1.0'?>\n";
$output_string .=  "<!DOCTYPE platform SYSTEM \"simgrid.dtd\">\n";
$output_string .=  "<platform version=\"$toversion\">\n";
$output_string .=  " <AS  id=\"AS0\"  routing=\"Full\">\n";

my $line;
while (defined($line = <INPUT>)) {
    chomp $line;
    # eat the header, whatever form it has
    next if ($line =~ s/<\?xml[^>]*>//           && ! $line =~ /\S/); # just in case several tags are on the same line
    next if ($line =~ s/<!DOCTYPE[^>]*>//        && ! $line =~ /\S/);
    
    if ($line =~ s/<platform(_description)? *>//) {
	$fromversion = 0;
	print "version 0\n";
	next if !$line =~ /\S/;
    } elsif ($line =~ s/<platform.*version=["]*([0-9.])["]*>//) {
	$fromversion = $1;
	print "version $fromversion\n";
	if ($fromversion == $toversion) {
	    die "Input platform file version is already $fromversion. This should be a no-op.\n";
	}
	if ($fromversion > $toversion) {
	    die "Input platform file version is more recent than this script (file version: $fromversion; script version: $toversion)\n";
	}
	next if !$line =~ /\S/;
    }
    
    if ($fromversion == 0) {
	while ($line =~ m|^(.*?)<cpu(.*?)power="([^"]*)"(.*)$|) {
	    $line = "$1TOTOTUTUTATA${2}TOTOTUTUTATA".($3*1000000)."TOTOTUTUTATA${4}";
	}
	while ($line =~ /^(.*?)TOTOTUTUTATA(.*?)TOTOTUTUTATA(.*?)TOTOTUTUTATA(.*)$/) {
	    $line = "$1<cpu${2}power=\"$3\"$4";
	}
	while ($line =~ m|^(.*?)<network_link(.*?)bandwidth="([^"]*)"(.*?)$|) {
	    $line = "$1TOTOTUTUTATA${2}TOTOTUTUTATA".($3*1000000)."TOTOTUTUTATA${4}";
	}
	while ($line =~ /^(.*?)TOTOTUTUTATA(.*?)TOTOTUTUTATA(.*?)TOTOTUTUTATA(.*?)$/) {
	    $line = "$1<network_link${2}bandwidth=\"$3\"$4";
	}
    }

    if ($fromversion < 2)  {
	# The renamings (\b=zero-width word boundary check)
	$line =~ s/\bplatform_description\b/platform/g;
	$line =~ s/\bname\b/id/g;
	$line =~ s/\bcpu\b/host/g;
	$line =~ s/\bnetwork_link\b/link/g;
	$line =~ s/\broute_element\b/link:ctn/g;
    }
    
    if($line =~ /^(.*)<\/platform>(.*)$/) {
	$output_string .=  " <\/AS>\n<\/platform>";
    } else {
    	$output_string .=  "$line\n";
    }
}

close INPUT;

if ($fromversion == -1) {
    die "Cannot retrieve the platform version\n";
}

open OUTPUT, "> $ARGV[0]";
print OUTPUT $output_string;
close OUTPUT;
