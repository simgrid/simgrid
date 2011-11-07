#!/usr/bin/perl -w
use strict;

my $toversion=3;
my $nb_peer = $ARGV[0];
my $i;

print "<?xml version='1.0'?>\n";
print "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid.dtd\">\n";
print "<platform version=\"$toversion\">\n";

print "\n<config id=\"General\">\n";
print "\t<prop id=\"coordinates\" value=\"yes\"></prop>\n";
print "</config>\n\n";

print "\t<AS  id=\"AS0\"  routing=\"Vivaldi\">\n";

my $line;

open SITES_LIGNE, $ARGV[0] or die "Unable to open $ARGV[1]\n";
while(defined($line=<SITES_LIGNE>))
{
	#278 7.2 -9.4 h 2.3 
		if($line =~ /^(.*) (.*) (.*) h (.*)$/)
		{
		print "\t\t<peer id=\"peer-$1\" coordinates=\"$2 $3 $4\" power=\"730000000.0\"\n";
		print "\t\tbw_in=\"13380000\" bw_out=\"1024000\" lat=\"5E-4\" />\n\n";
		}
}
			
print "\t</AS>\n";
print "</platform>";

print " \n";
