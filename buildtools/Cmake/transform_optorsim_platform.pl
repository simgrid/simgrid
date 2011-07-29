#!/usr/bin/perl -w
use strict;

if($#ARGV!=0) {
    die "Usage: perl transfrom_optorsim_platform.pl <file.conf>\n";
}

my($conf_file)=$ARGV[0];

open FILE, $conf_file or die "Unable to open $conf_file";

print "<?xml version='1.0'?>\n";
print "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid.dtd\">\n";
print "<platform version=\"3\">\n";
print "<AS  id=\"AS0\"  routing=\"Floyd\">\n";

my $line;
my @tokens;
my $token;
my $dst = 0;
my $src = 0;
my @list_of_name;
my $num_link = 0;

my @links = ();
my @routers = ();
my @hosts = ();
my @routes = ();
my $power;
while(defined($line=<FILE>))
{
	
	if($line =~ /^#(.*)\)(.*)$/)
	{
		print "<!--$1 $2-->\n";
		push @list_of_name, $2;
	}
	elsif($line =~ /^([0-9]*) ([0-9]*) ([0-9]*) (.*)$/)
	{
		if($1.$2.$3 == "000"){
			push @routers, "\t<router id=\"$list_of_name[$src]\"/>\n";
		}
		else{
			$power = $1 * $3;
			if($power == 0){
				$power=1;
			}
			push @hosts, "\t<host id=\"$list_of_name[$src]\" power=\"$power\"/>\n";
		}		
		my $table = $4;
		@tokens = split(/ /,$table);
		foreach $token (@tokens) {
			if($token != "0"){
#				print "from \"$list_of_name[$src]\" to \"$list_of_name[$dst]\" bdw=\"$token\"\n";
				if($src <= $dst){
					
					push @links, "\t<link id=\"link$num_link\" bandwidth=\"$token\"/>\n";
					
					push @routes, "\t<route src=\"$list_of_name[$src]\" dst=\"$list_of_name[$dst]\">\n";
					push @routes, "\t\t<link_ctn id=\"link$num_link\"/>\n";
					push @routes, "\t</route>\n";
					$num_link++;
				}
			}
 			$dst++;
		}
		$src++;
		$dst = 0;
    }
    else
    {
    	die;
    }
	
	
}
close(FILE);
	
print @hosts;
print "\n";
print @routers;
print "\n";
print @links;
print "\n";
print @routes;

print "</AS>\n";
print "</platform>";

print " \n";