#! /usr/bin/perl

use strict;

my $input    = shift @ARGV || die "Usage: $ARGV[0] platform_file.xml nb_host\n";
my $nb_hosts = shift @ARGV || 0;

my @host;

open IN,$input || die "Cannot open $input: $!\n";

while (<IN>) {
  next unless /<cpu name="([^"]*)"/; # "
  
  push @host, $1;
}

die "No host found in $input. Is it really a SimGrid platform file?\nCheck that you didn't pass a deployment file, for example.\n"
  unless (scalar @host);

if (! $nb_hosts) {
    $nb_hosts = scalar @host;
}

# map { print "$_\n" } @host;

print "<?xml version='1.0'?>\n";
print "<!DOCTYPE platform_description SYSTEM \"surfxml.dtd\">\n";
print "<platform_description version=\"1\">\n\n";


my $it_port=4000;
my $it_host=0;

for (my $i=0; $i<$nb_hosts -1; $i++) {

  print "  <process host=\"".$host[$it_host]."\" function=\"node\">\n";
  print "    <argument value=\"". $it_port ."\"/>     <!-- port on which I am listening -->\n";
  $it_host ++;
  if ($it_host == scalar @host) {
    $it_host=0;
    $it_port++;
  }
  print "    <argument value=\"". $host[ $it_host ]."\"/>   <!-- peer (successor) host -->\n";
  print "    <argument value=\"".($it_port)."\"/>     <!-- port on which peer is listening -->\n";
  print "  </process>\n\n";
                
}

print "  <process host=\"".$host[$it_host]."\" function=\"node\">\n";
print "    <argument value=\"". $it_port ."\"/>     <!-- port on which I am listening -->\n";
print "    <argument value=\"". $host[ 0 ]."\"/>   <!-- peer (successor) host -->\n";
print "    <argument value=\"4000\"/>     <!-- port on which peer is listening -->\n";
print "    <argument value=\"--create-token\"/>   <!-- I'm first client, ie I have to create the token -->\n";
print "  </process>\n\n";

print "</platform_description>\n";
