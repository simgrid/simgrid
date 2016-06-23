#! /usr/bin/env perl
eval 'exec perl -S $0 ${1+"$@"}'
    if $running_under_some_shell;

# This script updates the simgrid XML file passed as argument (modification in place)
# It is built to do the conversion incrementally.

# Copyright (c) 2006-2016. The SimGrid Team.
# All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

=encoding UTF-8

=head1 NAME

simgrid_update_xml - updates simgrid XML files to latest version
  
=head1 SYNOPSIS

B<simgrid_update_xml> I<xml_file>
  
=head1 DESCRIPTION

simgrid_update_xml updates the simgrid XML file passed as argument.  The file
is modified in place, without any kind of backup. You may want to save a copy
before running the script.

In SimGrid XML files, the standard version is indicated in the version
attribute of the platform tag. Current version is 4. Here is a list of major
changes in each version.

=over 4

=item B<Version 0:> Used before SimGrid 3.3

=item B<Version 1:> Introduced in SimGrid 3.3

=over 4

=item 

The version attribute of platform were added to allow file versioning.

=item

The link bandwidth changed from Mb/s to b/s; and the CPU power were changed
from MFlop/s to Flop/s

=back

=item B<Version 2:> Introduced in SimGrid 3.4

=over 

=item 

Several tags were renamed: 

  CPU -> HOST 
  NETWORK_LINK -> LINK
  ROUTE_ELEMENT ->  LINK_CTN
  PLATFORM_DESCRIPTION -> PLATFORM

=back

=item B<Version 3:> Introduced in SimGrid 3.5

=over 4

=item

The AS tag were introduced. Every plaform should now contain an englobing AS
tag.

=item 

Routes are now symmetric by default.

=item

Several tags were renamed (for sake of XML sanity):

  LINK:CTN -> LINK_CTN
  TRACE:CONNECT -> TRACE_CONNECT

=back

=item B<Version 4:> Introduced in SimGrid 3.13 (this is the current version)

=over 4

=item

Rename the attributes describing the amount of flop that a host / peer / cluster / cabinet can deliver per second.

  <host power=...> -> <host speed=...>

=item

In <trace_connect>, attribute kind="POWER" is now kind="SPEED".

=item

The DOCTYPE points to the right URL: http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd

=item

Units are now mandatory in attributes. USE THE SCRIPT sg_xml_unit_converter.py TO CONVERT THIS

     - speed. Old default: 'f' or 'flops'. Also defined: 
        'Yf',         'Zf',         'Ef',       'Pf',        'Tf',        'Gf',        'Mf',        'kf' 
        'yottaflops', 'zettaflops', 'exaflops', 'petaflops', 'teraflops', 'gigaflops', 'megaflops', 'kiloflops'
        
     - bandwidth. Old default: 'Bps' bytes per second (or 'bps' but 1 Bps = 8 bps)
       Also defined in bytes: 'TiBps', 'GiBps', 'MiBps', 'KiBps', 'TBps', 'GBps', 'MBps', 'kBps', 'Bps'
       And the same in bits:  'Tibps', 'Gibps', 'Mibps', 'Kibps', 'Tbps', 'Gbps', 'Mbps', 'kbps', 'bps' 
       
     - latency. Old default: 's' second. Also defined:
       'w' week, 'd' day, 'h' hour, 'm' minute, 'ms' millisecond, 'us' microsecond, 'ns' nanosecond, 'ps' picosecond   


=back

=back

=head1 AUTHORS

 The SimGrid team (simgrid-devel@lists.gforge.inria.fr)
  
=head1 COPYRIGHT AND LICENSE

Copyright (c) 2006-2016. The SimGrid Team. All rights reserved.
  
This program is free software; you may redistribute it and/or modify it
under the terms of GNU LGPL (v2.1) license.
  
=cut


use strict;

my $fromversion=-1;
my $toversion=4;

my $filename = $ARGV[0] or die "Usage: simgrid_update_xml.pl file_to_convert.xml\nPlease provide an XML to convert as a parameter.\n";
open INPUT, "$filename" or die "Cannot open input file $filename: $!\n";

my $output_string = "<?xml version='1.0'?>\n".
    "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">\n".
    "<platform version=\"$toversion\">\n";

my($AS_opened)=0;

my $line;
while (defined($line = <INPUT>)) {
    chomp $line;
    # eat the header, whatever form it has
    next if ($line =~ s/<\?xml[^>]*>//           && ! $line =~ /\S/); # just in case several tags are on the same line
    next if ($line =~ s/<!DOCTYPE[^>]*>//        && ! $line =~ /\S/);
    
    if ($line =~ s/<platform(_description)? *>//) {
	$fromversion = 0;
	print "$filename was using version 0\n";
	next if !$line =~ /\S/;
    } elsif ($line =~ s/<platform.*version=["']*([0-9.])["']*>//) {
	$fromversion = $1;
	if ($fromversion == $toversion) {
	    die "Input platform file $filename is already conformant to version $fromversion. This should be a no-op.\n";
	}
	if ($fromversion > $toversion) {
	    die "Input platform file $filename is more recent than this script (file version: $fromversion; script version: $toversion)\n";
	}
	next if !$line =~ /\S/;
	print "$filename was using version $fromversion\n";
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
    
    if ($fromversion < 3)  {
	$line =~ s/\blink:ctn\b/link_ctn/g;
	$line =~ s/\btrace:connect\b/trace_connect/g;

	if($AS_opened && (($line=~ /<\/platform>/) || ($line=~ /<process/))) {
	    $output_string .= "</AS>\n";
	    $AS_opened = 0;
	}

	if( (!$AS_opened) && (
		($line =~ /<host/)    ||
		($line =~ /<link/)    ||
		($line =~ /<cluster/) ||
		($line =~ /<router/)
	    )) {
	    $output_string .=  " <AS  id=\"AS0\"  routing=\"Full\">\n";
	    $AS_opened=1;
	}
	
	if($line=~/<route /){$line =~ s/\<route/\<route symmetrical=\"NO\"/g;}
    }
    if ($fromversion < 4) {
	$line =~ s/\bpower\b/speed/g;	
	$line =~ s/\bkind="POWER"/kind="SPEED"/g;
    }
	
    $output_string .= "$line\n";
}

close INPUT;

if ($fromversion == -1) {
    die "Cannot retrieve the platform version of $filename\n";
}

open OUTPUT, "> $filename";
print OUTPUT $output_string;
close OUTPUT;
