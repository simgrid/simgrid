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

for($i=0 ; $i<$nb_peer ; $i++){
		print "\t\t<peer id=\"$i\" coordinates=\"-12.7 -9.9 2.1\" power=\"730000000.0\"\n";
		print "\t\tbw_in=\"2250000000\" bw_out=\"2250000000\" lat=\"5E-4\" />\n\n";
}

print "\t\t<ASroute src=\"(.*)\"\n\t\t\tdst=\"(.*)\"\n\t\t\tgw_src=\"router_\$1src\"\n\t\t\tgw_dst=\"router_\$1dst\">\n\t\t</ASroute>\n";		
			
print "\t</AS>\n";
print "</platform>";

print " \n";