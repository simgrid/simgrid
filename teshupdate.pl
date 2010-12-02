#! /usr/bin/perl

# This script is only there temporarly for the time that we convert all the tesh files into the new log formating allowing to sort the output
# Ouf, cette phrase est trop longue et illisible, desole (mt)

use strict;

while (<>) {
    if (/\$ (.*)$/) {
	print "! output sort\n";
	print "\$ $1 ".'--log=root.fmt:[%10.6r]%e(%i:%P@%h)%e%m%n'."\n";
    } elsif (/\> \[([^:]*):([^:]*):([^(]*)\(([0-9]*)\) ([0-9.]*)\] \[([^\]]*)\] (.*)/) { # ) {
      my ($host,$proc,$thread,$pid,$time,$msg) = ($1,$2,$3,$4,$5,$7);
#      print "h=$host,p=$proc,pid=$pid,t=$time; $_";
      $time = sprintf "%10.6f",$time;
      print "> [$time] ($pid:$proc\@$host) $msg\n";
    } elsif (/\> \[([0-9.]*)\] \[([^\]]*)\] (.*)/) {
      my ($time,$msg) = ($1,$3);
      $time = sprintf "%10.6f",$time;
      print "> [$time] (0:@) $msg\n";
    } else {
	print $_;
    }
}
