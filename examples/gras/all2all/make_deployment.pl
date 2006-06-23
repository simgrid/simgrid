#! /usr/bin/perl

use strict;

sub usage {
    print STDERR <<EOH
Usage: all2all_make_deployment.pl platform_file.xml nb_host size_msg (bcast source?)
  
This script generates a deployment file for the all2all program. It takes 
a SimGrid platform file as first argument and the number of wanted peers as 
second argument. If the amount of peers exceeds the amount of available 
hosts in the deployment file, several peers will be placed on the same host.

The third argument is a size of the message to send ( octets )  

If a fourth argument is passed, this is the source of the broadcast
(given as a number between 0 and nb_host-1).
EOH
      ;
    die "\n";
}

my $input    = shift @ARGV || usage();
my $nb_hosts = shift @ARGV || usage();
my $size_msg = shift @ARGV || usage();
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
# Build the receiver string

my $receivers;    # strings containing sender argument describing all receivers.

my $it_host=0;    # iterator
my $it_port=4000; # iterator, in case we have so much processes to add that we must change it

for (my $i=0; $i<$nb_hosts; $i++) {
  $receivers .= "    <argument value=\"$host[$it_host]:$it_port\"/>\n";
  $it_host ++;
  if ($it_host == scalar @host) {
    $it_host=0;
    $it_port++;
  }
}
$receivers .= "    <argument value=\"$size_msg\"/>\n";

#
# and now, really generate the file. Receiver first.

print "<?xml version='1.0'?>\n";
print "<!DOCTYPE platform_description SYSTEM \"surfxml.dtd\">\n";
print "<platform_description version=\"1\">\n\n";

# reset iterators
$it_port=4000;
$it_host=0;

for (my $i=0; $i<$nb_hosts; $i++) {
  print "  <process host=\"".$host[$it_host]."\" function=\"receiver\">\n";
  print "    <argument value=\"$it_port\"/><argument value=\"".(length($source)?1:$nb_hosts)."\"/>\n";
  print "  </process>\n\n";
    
  $it_host ++;
  if ($it_host == scalar @host) {
    $it_host=0;
    $it_port++;
  }
}

#
# Here come the sender(s)

# reset iterators
$it_port=4000;
$it_host=0;

for (my $i=0; $i<$nb_hosts; $i++) {
  if (!length($source) || $source == $i) {
      print "  <process host=\"".$host[$it_host]."\" function=\"sender\">\n";
      print $receivers;
      print "  </process>\n";
  }
    
  $it_host ++;
  if ($it_host == scalar @host) {
    $it_host=0;
    $it_port++;
  }
}

print "</platform_description>\n";

# print "source='$source' nb_hosts=$nb_hosts\n";
