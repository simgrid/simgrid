#!/usr/bin/perl -w
use strict;

# input file = AddTest.txt

if ( $#ARGV != 1 ) {
    die "Usage: generate_memcheck_tests.pl <CMAKE_HOME_DIRECTORY> AddTests.cmake\n";
}

my ($proj_dir) = $ARGV[0];
open MAKETEST, $ARGV[1] or die "Unable to open $ARGV[1]. $!\n";

sub var_subst {
    my ($text, $name, $value) = @_;
    if ($value) {
        $text =~ s/\${$name(?::=[^}]*)?}/$value/g;
        $text =~ s/\$$name(\W|$)/$value$1/g;
    }
    else {
        $text =~ s/\${$name:=([^}]*)}/$1/g;
        $text =~ s/\${$name}//g;
        $text =~ s/\$$name(\W|$)/$1/g;
    }
    return $text;
}

my (@test_list) = ();
my ($nb_test)   = 0;
my ($line);
my ($path);
my ($dump) = 0;
my ($srcdir);
my ($bindir);
my ($tesh_file);
my ($config_var);
my ($name_test);

while ( defined( $line = <MAKETEST> ) ) {
    chomp $line;
    if ( $line =~ /BEGIN TESH TESTS/ ) {
        $dump = 1;
        next;
    }
    if ( $line =~ /END TESH TESTS/ ) {
        $dump = 0;
        last;
    }
    if ($dump) {
        if ( $line =~ /^\s*ADD_TEST\(\S+\s+\S*\/tesh\s/ ) {
            $srcdir     = "";
            $bindir     = "";
            $config_var = "";
            $path       = "";
            $nb_test++;
            $tesh_file = "";
            $name_test = "";

            if ( $line =~ /ADD_TEST\((\S+)/ ) {
                $name_test = ($1);
            }
            while ( $line =~ /--cfg\s+(\S+)/g ) {
                $config_var = "--cfg=$1 $config_var";
            }
            while ( $line =~ /--cd\s+(\S+)/g ) {
                $path = ($1);
                $path =~ s/\"//g;
            }
            while ( $line =~ /--setenv\s+(\S+)\=(\S+)/g ) {
                my ( $env_var, $value_var ) = ( $1, $2 );
                if ( $env_var =~ /srcdir/ ) {
                    $srcdir = $value_var;
                }
                elsif ( $env_var =~ /bindir/ ) {
                    $bindir = $value_var;
                }
            }
            if ( $line =~ /(\S+)\)$/ ) {
                $tesh_file = $1;
                $tesh_file =~ s/^[^\/\$]/$path\/$&/;
                $tesh_file =~ s/\${CMAKE_HOME_DIRECTORY}/$proj_dir/g;
                if ( ! -e "$tesh_file" ) {
                    print "# tesh_file: $tesh_file does not exist!\n";
                    print "# $line\n";
                    next;
                }
            }

            if (0) {
                print "test_name = $name_test\n";
                print "$config_var\n";
                print "path = $path\n";
                print "srcdir=$srcdir\n";
                print "bindir=$bindir\n";
                print "tesh_file = $tesh_file\n";
                print "\n\n";
            }

            my ($count)        = 0;
            my ($count_first)  = 0;
            my ($count_second) = 0;
            open TESH_FILE, $tesh_file or die "Unable to open $tesh_file $!\n";
            my ($l);
            while ( defined( $l = <TESH_FILE> ) ) {
                chomp $l;
                if ( $l =~ /^\$ (.*)$/ ) {
                    my ($command) = $1;
                    $command = var_subst($command, "srcdir", $srcdir);
                    $command = var_subst($command, "bindir", $bindir);
                    $command = var_subst($command, "EXEEXT", "");
                    $command = var_subst($command, "SG_TEST_EXENV", "");
                    $command = var_subst($command, "SG_TEST_ENV", "");
                    $command = var_subst($command, "SG_EXENV_TEST", "");
                    $command = var_subst($command, "ARGS", "");
                    $command =~ s/\$@//g;
#                    $command =~ s/..\/..\/bin\/smpirun/\${CMAKE_BINARY_DIR\}\/bin\/smpirun/g;
                    $command =~ s/^\s+//;
                    $command =~ s/^[^\/\$]\S*\//$path\/$&/;
                    $command =~ s/^(\S*\/)(?:\.\/)+/$1/g;

                    if ($config_var) {
                        $command = "$command $config_var";
                    }
                    print "ADD_TEST(memcheck-$name_test-$count $command --cd $path\/)\n";

                    #push @test_list, "memcheck-$name_test-$count";
                    $count++;
                }
                if ( $l =~ /^\& (.*)$/ ) {
                    last;
                }
            }
            close(TESH_FILE);
        }
        elsif ( $line =~ /^\s*set_tests_properties/ ) {
            if ( $line =~ /set_tests_properties\(([\S]+)/ ) {
                my ($name_temp) = ($1);
                $line =~ s/$name_temp/memcheck-$name_temp-0/g;
                print $line. "\n";
            }
        }
        else {
            print $line. "\n";
        }
    }
}
close(MAKETEST);

#print "nb_test = $nb_test\n";
#print "set(MEMCHECK_LIST\n";
#print (join("\n", @test_list));
#print ")\n";
