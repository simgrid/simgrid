#!/usr/bin/perl -w
use strict;

if( -e "/usr/bin/apt-get")
{
	my($ruby_version)=`apt-cache search ruby1.9.*-dev`;
	
	$ruby_version =~ s/-dev.*//g;
	chomp $ruby_version;
	
	print "ruby to install $ruby_version $ruby_version-dev\n";
	`sudo apt-get install -y $ruby_version $ruby_version-dev`;
	
	print "ln -sf /usr/bin/$ruby_version /usr/bin/ruby\n";
	`sudo ln -sf /usr/bin/$ruby_version /usr/bin/ruby`;
	
	$ruby_version=`ruby --version`;
	print "ruby = $ruby_version";
}
elsif(-e "/usr/bin/yum")
{
	`sudo yum -y -q install ruby-devel ruby`
}
