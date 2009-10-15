#! /usr/bin/perl

use strict;

my $node_count = int($ARGV[0]) || die "Usage: $0 node_count level_count\n";
my $level_count = int($ARGV[1]) || die "Usage: $0 node_count level_count\n";

print "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
print "<!-- generated: ".(localtime())." -->\n";
print "<adag xmlns=\"http://pegasus.isi.edu/schema/DAX\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://pegasus.isi.edu/schema/DAX http://pegasus.isi.edu/schema/dax-2.1.xsd\" version=\"2.1\" count=\"1\" index=\"0\" name=\"test\" jobCount=\"25\" fileCount=\"0\" childCount=\"20\">\n";

for (my $level=1;$level<=$level_count;$level++) {
    for (my $i=1;$i<=$node_count;$i++) {
	print "<job id=\"node$i\" namespace=\"ForkJoin\" name=\"level$level\" version=\"1.0\" runtime=\"10\">\n";
	print "  <uses file=\"n${i}_l${level}_in\" link=\"input\" register=\"true\" transfer=\"true\" optional=\"false\" type=\"data\" size=\"1000\"/>\n";
	print "  <uses file=\"n${i}_l${level}_out\" link=\"output\" register=\"true\" transfer=\"true\" optional=\"false\" type=\"data\" size=\"1000\"/>\n";
	print "</job>\n";
    }

    if ($level<$level_count) {
	print "<job id=\"join$level\" namespace=\"ForkJoin\" name=\"join\" version=\"1.0\" runtime=\"10\">\n";
	for (my $i=1;$i<=$node_count;$i++) {
	    print "  <uses file=\"n${i}_l${level}_out\" link=\"input\" register=\"true\" transfer=\"true\" optional=\"false\" type=\"data\" size=\"1000\"/>\n";
	    print "  <uses file=\"n${i}_l".(${level}+1)."_in\" link=\"output\" register=\"true\" transfer=\"true\" optional=\"false\" type=\"data\" size=\"1000\"/>\n";
	}
	print "</job>\n";
    }
}

print "</adag>\n";