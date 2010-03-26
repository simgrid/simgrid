#!/usr/bin/perl -w
use strict;

# input file = CMakeTest.txt

if($#ARGV!=1) {
    die "Usage: generate_memcheck_tests.pl <project_directory> <CMakeTests.txt>\n";
}

my($proj_dir)=$ARGV[0];
open MAKETEST, $ARGV[1] or die "Unable to open $ARGV[1]. $!\n";

my(@test_list)=();

my($line);
my($dump)=0;
while(defined($line=<MAKETEST>)) {
    chomp $line;
    if($line =~ /BEGIN TESH TESTS/) {
	$dump = 1;
	next;
    } 
    if($line =~ /END TESH TESTS/) {
	$dump = 0;
	last;
    }
    if($dump) {
	if($line =~ /ADD_TEST/) {
	    if($line =~ /ADD_TEST\(([\S]+)\s+.*\/tools\/tesh\/tesh\s*--cd\s*(\S+)\s+(.*)\)$/) {
		my($name_test,$path,$tesh_file)=($1,$2,$3);
		$path=~ s/\"//g;
		my($complete_tesh_file)=$path."/".$tesh_file;
		my($count)=0;
		$complete_tesh_file =~ s/\${PROJECT_DIRECTORY}/$proj_dir/g;
		open TESH_FILE, $complete_tesh_file or die "Unable to open $complete_tesh_file. $!\n";
		my($l);
		while(defined($l=<TESH_FILE>)) {
		    chomp $l;
		    if($l =~ /^\$ (.*)$/) {
			my($command)=$1;
			$command =~ s/\${srcdir:=.}/\${PROJECT_DIRECTORY}\/src/g;
			$command =~ s/\${EXEEXT:=}//g;
			print "ADD_TEST(memcheck-$name_test-$count /bin/sh -c 'cd $path && $command')\n";
			push @test_list, "memcheck-$name_test-$count";
			$count++;
		    }
		}
		close(TESH_FILE);
	    } else {
		next;
	    }
	} else {
	    print $line."\n";
	}
    }   
}
close(MAKETEST);

print "set(MEMCHECK_LIST\n";
print (join("\n", @test_list));
print ")\n";
