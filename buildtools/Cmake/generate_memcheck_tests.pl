#!/usr/bin/perl -w
use strict;

# input file = AddTest.txt

if($#ARGV!=1) {
    die "Usage: generate_memcheck_tests.pl <project_directory> AddTests.cmake\n";
}

my($proj_dir)=$ARGV[0];
open MAKETEST, $ARGV[1] or die "Unable to open $ARGV[1]. $!\n";

my(@test_list)=();

my($line);
my($dump)=0;
print "if(enable_memcheck)\n";
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
	if($line =~ /ADD_TEST/) 
	{
	    if($line =~ /ADD_TEST\(([\S]+)\s+.*\$\{CMAKE_BINARY_DIR\}\/bin\/tesh\s*--cd\s*(\S+)\s+(.*)\)$/) 
	    {
		my($name_test,$path,$tesh_file)=($1,$2,$3);
		$path=~ s/\"//g;
		my($complete_tesh_file)=$path."/".$tesh_file;
		my($count)=0;
		my($count_first)=0;
		my($count_second)=0;
		$complete_tesh_file =~ s/\$\{CMAKE_BINARY_DIR\}/$proj_dir/g;
		$complete_tesh_file =~ s/\$\{PROJECT_DIRECTORY\}/$proj_dir/g;
		open TESH_FILE, $complete_tesh_file or die "Unable to open $complete_tesh_file $!\n";
		my($l);
		while(defined($l=<TESH_FILE>))
		{
		    chomp $l;
		    if($l =~ /^\$ (.*)$/) 
		    {
			my($command) = $1;
			$command =~ s/\${srcdir:=.}/./g;
			$command =~ s/\${EXEEXT:=}//g;
			$command =~ s/\$SG_TEST_EXENV //g;
			$command =~ s/\$SG_TEST_ENV //g;
			$command =~ s/\$SG_EXENV_TEST //g; 
			$command =~ s/\$EXEEXT//g;
			$command =~ s/\${EXEEXT}//g;
			$command =~ s/\${srcdir}/\${PROJECT_DIRECTORY}\/src/g;
			$command =~ s/ \$ARGS//g;
			$command =~ s/ \$@ //g;	
			$command =~ s/..\/..\/bin\/smpirun/\${CMAKE_BINARY_DIR\}\/bin\/smpirun/g;
			print "ADD_TEST(memcheck-$name_test-$count $command --cd $path\/)\n";
			#push @test_list, "memcheck-$name_test-$count";
			$count++;
		    }
		    if($l =~ /^\& (.*)$/) 
		    {
			last;
		    }
		}
		close(TESH_FILE);
	    } 
	    else 
	    {
		next;
	    }
	} 
	else 
	{
	    print $line."\n";
	}
    }   
}
close(MAKETEST);
print "endif(enable_memcheck)\n";
#print "set(MEMCHECK_LIST\n";
#print (join("\n", @test_list));
#print ")\n";
