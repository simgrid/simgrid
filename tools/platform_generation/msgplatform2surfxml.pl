#!/usr/bin/perl -w

use strict;

$ARGV[0] or die "msg2surf.pl <platform.txt>";
open INPUT, "$ARGV[0]" or die;

my($line);
my($parsing_host) = 0;
my($parsing_link) = 0;
my($parsing_route) = 0;
my($lat);
my(@link_list) = ();
my($link);

print "<?xml version='1.0'?>\n";
print "<!DOCTYPE platform_description SYSTEM \"surfxml.dtd\">\n";
print "<platform_description>\n";

while(defined($line=<INPUT>)){
    chomp $line;
    if($line=~/HOSTS/) {
	$parsing_host=1;
	next;
    }
    if($line=~/LINKS/) {
	$parsing_host=0;
	$parsing_link=1;
	next;
    }
    if($line=~/ROUTES/) {
	$parsing_link=0;
	$parsing_route=1;
	next;
    }

    if($parsing_host) {
	if($line=~ /^\s*(\w+)\s+([0-9\.]+)\s*$/) {
	    print "  <cpu name=\"$1\" power=\"$2\"/>\n";
	} else die "Cannot understand line \"$line\"\n";
    }
    if($parsing_link) {
	if($line=~ /^\s*(\w+)\s+([0-9\.]+)\s*([0-9\.]+)\s*$/) {
	    $lat=$3; 
	    $lat/=1000;
	    print "  <network_link name=\"$1\" bandwidth=\"$2\" latency=\"$lat\"/>\n";
	} else die "Cannot understand line \"$line\"\n";
    }
    if($parsing_route) {
	if($line=~ /^\s*(\w+)\s+(\w+)\s+\((.*)\)\s*$/) {
	    @link_list=split(/\s+/, $3);
	    print "  <route src=\"$1\" dst=\"$2\">";
	    foreach $link (@link_list) {
	      print "<route_element name=\"$link\"/>";
	    }
	    print "</route>\n";
	} else die "Cannot understand line \"$line\"\n";
    }
}
print "</platform_description>\n";


