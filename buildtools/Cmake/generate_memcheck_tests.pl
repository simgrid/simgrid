#!/usr/bin/perl -w
use strict;

# input file = AddTest.txt

if($#ARGV!=1) {
    die "Usage: generate_memcheck_tests.pl <CMAKE_HOME_DIRECTORY> AddTests.cmake\n";
}

my($proj_dir)=$ARGV[0];
open MAKETEST, $ARGV[1] or die "Unable to open $ARGV[1]. $!\n";

my(@test_list)=();
my($nb_test)=0;
my($line);
my($path);
my($dump)=0;
my($srcdir);
my($bindir);
my($tesh_file);
my($config_var);
my($name_test);

while(defined($line=<MAKETEST>))
{
    chomp $line;
    if($line =~ /BEGIN TESH TESTS/) {
		$dump = 1;
		next;
	    } 
    if($line =~ /END TESH TESTS/) {
		$dump = 0;
		last;
	    }
    if($dump) 
    {
		if($line =~ /ADD_TEST/) 
		{	$srcdir = "";
			$bindir = "";
			$config_var = "";
			$path = "";
			$nb_test++;
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
				$path=($1);
				$path=~ s/\"//g;
				$path =~ s/\$\{CMAKE_BINARY_DIR\}/$proj_dir/g;
				$path =~ s/\$\{CMAKE_HOME_DIRECTORY\}/$proj_dir/g;
			}
			if($line =~ /--setenv\s*\t*(\S*)\=(\S*)/)
			{
				my($env_var,$value_var)=($1,$2);
				$value_var =~ s/\$\{CMAKE_BINARY_DIR\}/$proj_dir/g;
				$value_var =~ s/\$\{CMAKE_HOME_DIRECTORY\}/$proj_dir/g;
				if($env_var =~ /srcdir/)
				{
					$srcdir = $value_var;
				}
				if($env_var =~ /bindir/)
				{
					$bindir = $value_var; 					
				}
			}
			if($line =~ /([\S]+)[)]$/)
			{
				$tesh_file =($1);
				$tesh_file =~ s/\$\{CMAKE_BINARY_DIR\}/$proj_dir/g;
				$tesh_file =~ s/\$\{CMAKE_HOME_DIRECTORY\}/$proj_dir/g;
				if ( -e "$tesh_file")
				{
					
				}
				elsif( -e "$path/$tesh_file")
				{
					$tesh_file = "$path\/$tesh_file";
					
				}
				else
				{
					die;
				}
				
			}
			
#			print "test_name = $name_test\n";
#			print "$config_var\n";
#			print "path = $path\n";
#			print "srcdir=$srcdir\n";
#			print "bindir=$bindir\n";
#			print "tesh_file = $tesh_file\n";
#			print "\n\n";
			
			my($count)=0;
			my($count_first)=0;
			my($count_second)=0;
			open TESH_FILE, $tesh_file or die "Unable to open $tesh_file $!\n";
			my($l);
			while(defined($l=<TESH_FILE>))
			{
			    chomp $l;
			    if($l =~ /^\$ (.*)$/) 
			    {
				my($command) = $1;
				$command =~ s/\${srcdir:=.}/$srcdir/g;
				$command =~ s/\${bindir:=.}/$bindir/g;
				$command =~ s/\${EXEEXT:=}//g;
				$command =~ s/\$SG_TEST_EXENV //g;
				$command =~ s/\$SG_TEST_ENV //g;
				$command =~ s/\$SG_EXENV_TEST //g; 
				$command =~ s/\$EXEEXT//g;
				$command =~ s/\${EXEEXT}//g;
				$command =~ s/\${srcdir}/\${CMAKE_HOME_DIRECTORY}\/src/g;
				$command =~ s/ \$ARGS//g;
				$command =~ s/ \$@ //g;	
				$command =~ s/..\/..\/bin\/smpirun/\${CMAKE_BINARY_DIR\}\/bin\/smpirun/g;
                if($command =~ /^[^\/\$\s]+\//) {
                	$command = $path."/".$command;
                	$command =~ s/\/(.?\/)+/\//g;
                }
                if ($config_var)
                {
                	$command = "$command $config_var";
                }
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
		elsif($line =~ /set_tests_properties/)
		{
			if($line =~ /set_tests_properties\(([\S]+)/)
			{
				my($name_temp)=($1);
				$line =~ s/$name_temp/memcheck-$name_temp-0/g;
				print $line."\n";
			}
		}
		else
		{
		    print $line."\n";
		}
	}   
}
close(MAKETEST);
#print "nb_test = $nb_test\n";
#print "set(MEMCHECK_LIST\n";
#print (join("\n", @test_list));
#print ")\n";
