#! /usr/bin/perl

use strict;

sub usage {
    print STDERR <<EOH
Usage: make_deployment.pl platform_file.xml nb_host 
  
This script generates a deployment file for the PMM program. It takes 
a SimGrid platform file as first argument and the number of wanted peers as 
second (optional) argument. If the amount of peers exceeds the amount of 
available hosts in the deployment file, several peers will be placed on the
same host.
      
EOH
      ;
    die "\n";
}

my $input    = shift @ARGV || usage();
my $nb_hosts = shift @ARGV || "";
# my $source   = shift || "";

my @host;

open IN,$input || die "Cannot open $input: $!\n";

while (<IN>) {
  next unless /<cpu name="([^"]*)"/; # "
  
  push @host, $1;
}

# map { print "$_\n" } @host;

die "No host found in $input. Is it really a SimGrid platform file?\nCheck that you didn't pass a deployment file, for example.\n"
  unless (scalar @host);

if (! $nb_hosts) {
    $nb_hosts = scalar @host;
}

#
# generate the file. Master first (quite logical, indeed)

my $port_num = 4000;
my $master = $host[0];

print "<?xml version='1.0'?>\n";
print "<!DOCTYPE platform_description SYSTEM \"surfxml.dtd\">\n";
print "<platform_description version=\"1\">\n\n";
print "  <!-- The master, argument :: port number -->\n";
print "  <process host=\"$master\" function=\"master\">\n";
print "    <argument value=\"$port_num\"/>\n";
print "  </process>\n";

# Start the slaves also
# reset iterators
my $it_host=1;

for (my $i=0; $i<$nb_hosts; $i++) {
  print "  <process host=\"".$host[$it_host]."\" function=\"slave\"><argument value=\"$master:$port_num\"/></process>\n";
    
  $it_host ++;
  if ($it_host == scalar @host) {
    $it_host=1;
  }
}


print "</platform_description>\n";

