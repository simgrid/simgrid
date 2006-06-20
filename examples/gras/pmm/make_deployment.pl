#! /usr/bin/perl

use strict;

sub usage {
    print STDERR <<EOH
Usage: make_deployment.pl platform_file.xml nb_host 
  
This script generates a deployment file for the PMM program. It takes 
a SimGrid platform file as first argument and the number of wanted peers as 
second argument. If the amount of peers exceeds the amount of available 
hosts in the deployment file, several peers will be placed on the same host.
      
EOH
      ;
    die "\n";
}

my $input    = shift @ARGV || usage();
my $nb_hosts = shift @ARGV || usage();
my $source   = shift || "";

my @host;

open IN,$input || die "Cannot open $input: $!\n";

while (<IN>) {
  next unless /<cpu name="([^"]*)"/; # "
  
  push @host, $1;
}

# map { print "$_\n" } @host;

die "No host found in $input. Is it really a SimGrid platform file?\nCheck that you didn't pass a deployment file, for example.\n"
  unless (scalar @host);


#
# generate the file. Master first (quite logical, indeed)

print "<?xml version='1.0'?>\n";
print "<!DOCTYPE platform_description SYSTEM \"surfxml.dtd\">\n";
print "<platform_description version=\"1\">\n\n";
print "  <!-- The master, arguments :: all others nodes -->\n";
print "  <process host=\"$host[0]\" function=\"master\">\n";

# put the slaves as argument of the master

my $it_host=0;    # iterator
my $it_port=4000; # iterator, in case we have so much processes to add that we must change it
for (my $i=0; $i<$nb_hosts; $i++) {
  print "    <argument value=\"$host[$it_host]:$it_port\"/>\n";
  $it_host ++;
  if ($it_host == scalar @host) {
    $it_host=0;
    $it_port++;
  }
}
print "  </process>\n";

# Start the slaves also
# reset iterators
$it_port=4000;
$it_host=0;

for (my $i=0; $i<$nb_hosts; $i++) {
  print "  <process host=\"".$host[$it_host]."\" function=\"slave\"><argument value=\"$it_port\"/></process>\n";
    
  $it_host ++;
  if ($it_host == scalar @host) {
    $it_host=0;
    $it_port++;
  }
}


print "</platform_description>\n";

