#!/usr/bin/perl -w
use strict;

if( -e "/usr/bin/apt-get")
{
	my($ruby_version) = `apt-cache search ruby1.9.1-dev`;
	
	if( $ruby_version=~ /^$/){
	$ruby_version = `apt-cache search ruby1.9-dev`;
	}
	
	if( $ruby_version=~ /^$/){
	return;
	}
	
	$ruby_version =~ s/-dev.*//g;
	chomp $ruby_version;
	
	print "ruby to install $ruby_version $ruby_version-dev\n";
	`sudo apt-get install -y $ruby_version $ruby_version-dev`;
	`sudo ln -sf /usr/bin/$ruby_version /usr/bin/ruby`;
	
	$ruby_version=~ s/-dev//g;
	$ruby_version=~ s/ruby/ruby\*/g;
	my($ruby_lib)=`ls /usr/lib/lib$ruby_version.so`;
	chomp $ruby_lib;
	`sudo ln -sf "$ruby_lib" /usr/lib/libruby.so`;
	
	$ruby_version=`ruby --version`;
	print "ruby = $ruby_version";
	print "libruby = ".`ls /usr/lib/libruby.so`;
}
elsif(-e "/usr/bin/yum")
{
	`sudo yum -y -q install ruby-devel ruby`
}
