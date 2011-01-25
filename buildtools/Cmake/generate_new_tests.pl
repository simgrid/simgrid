#!/usr/bin/perl -w
use strict;

# input file = AddTest.txt

if($#ARGV!=0) {
    die "Usage: generate_new_tests.pl AddTests.cmake\n";
}

open MAKETEST, $ARGV[0] or die "Unable to open $ARGV[1]. $!\n";

my($line);
my($path);
my($dump)=0;
my($setenv);
my($tesh_file);
my($config_var);
my($name_test);

while(defined($line=<MAKETEST>))
{
    chomp $line;
    if($line =~ /BEGIN CONTEXTS FACTORY/) {
    	print "$line\n";
		$dump = 1;
		next;
	    } 
    if($line =~ /HAVE_TRACING/) {
    	print "$line\n";
		$dump = !$dump;
		next;
	    }
	if($line =~ /HAVE_LUA/) {
    	print "$line\n";
		$dump = !$dump;
		next;
	    }
	if($line =~ /HAVE_RUBY/) {
    	print "$line\n";
		$dump = !$dump;
		next;
	    }
    if($dump) 
    {
		if($line =~ /ADD_TEST.*\/bin\/tesh/) 
		{	
			$setenv = "";
			$config_var = "";
			$path = "";
			$tesh_file = "";
			$name_test = "";
			
			if($line =~ /ADD_TEST\(([\S]+)/)
			{
				$name_test =($1);
			}
			if($line =~ /--cfg\s*\t*(\S*)/)
			{
				$config_var = "--cfg $1 $config_var";
			}
			if($line =~ /--cd\s*(\S+)/)
			{
				$path="--cd $1";
			}
			if($line =~ /--setenv\s*\t*(\S*)\=(\S*)/)
			{
				$setenv = "--setenv $1=$2 $setenv";
			}
			if($line =~ /([\S]+)[)]$/)
			{
				$tesh_file =($1);				
			}
			
			print "ADD_TEST($name_test-raw		\$\{CMAKE_BINARY_DIR\}/bin/tesh	--cfg contexts/factory:raw	$config_var	$setenv	$path $tesh_file)\n";
			print "ADD_TEST($name_test-thread	\$\{CMAKE_BINARY_DIR\}/bin/tesh	--cfg contexts/factory:thread	$config_var	$setenv	$path $tesh_file)\n";	
			print "ADD_TEST($name_test-ucontext	\$\{CMAKE_BINARY_DIR\}/bin/tesh	--cfg contexts/factory:ucontext	$config_var	$setenv	$path $tesh_file)\n";	
		}
		elsif($line =~ /set_tests_properties\(([\S]+)/)
		{
				my($name_temp)=($1);
				$line =~ s/$name_temp/$name_temp-raw $name_temp-thread $name_temp-ucontext/g;
				print $line."\n";
		}
		else
		{
			print "$line\n";
		}
	
	}
	else
	{
		print "$line\n";
	}   
}
close(MAKETEST);
