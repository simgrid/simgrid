#!/usr/bin/env perl

# Copyright (c) 2012-2014. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

use strict;
use warnings;

# input file = Test.txt

if ( $#ARGV != 1 ) {
    die "Usage: generate_memcheck_tests.pl <CMAKE_HOME_DIRECTORY> Tests.cmake\n";
}

my ($proj_dir) = $ARGV[0];
open MAKETEST, $ARGV[1] or die "Unable to open file: \"$ARGV[1]\". $!\n";

sub var_subst {
    my ($text, $name, $value) = @_;
    if ($value) {
        $text =~ s/\$\{$name(?::[=-][^}]*)?\}/$value/g;
        $text =~ s/\$$name(\W|$)/$value$1/g;
    }
    else {
        $text =~ s/\$\{$name:=([^}]*)\}/$1/g;
        $text =~ s/\$\{$name}//g;
        $text =~ s/\$$name(\W|$)/$1/g;
    }
    return $text;
}

my (@test_list) = ();
my ($nb_test)   = 0;
my ($line);
my ($path);
my ($dump) = 0;
my (%environ);
my ($tesh_file);
my ($factories);
my ($config_var);
my ($name_test);
my ($indent);

while ( defined( $line = <MAKETEST> ) ) {
    chomp $line;
    if ( $line =~ /BEGIN TESH TESTS/ ) {
        $dump = 1;
        next;
    }
    if ( $line =~ /END TESH TESTS/ ) {
        $dump = 0;
        next;
    }
    if ($dump) {
        $line =~ s/^  //;
        if ( $line =~ /^\s*(?:ADD_TEST\(\S+\s+\S*TESH\_COMMAND\}\s|ADD_TESH\(|ADD_TESH_FACTORIES)/ ) {
            undef %environ;
            $config_var = "";
            $factories  = "";
            $path       = "";
            $nb_test++;
            $tesh_file = "";
            $name_test = "";
            $indent = "";
            if ( $line =~ /^(\s*)ADD_(?:TEST|TESH)\((\S+)/ ) {
                $indent = ($1);
                $name_test = ($2);
            }
            if ( $line =~ /^(\s*)ADD_TESH_FACTORIES\((\S+)\s+\"(\S+)\"/ ) {
                $indent = ($1);
                $name_test = ($2);
                $factories = ($3);
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
                $environ{$env_var} = $value_var;
            }
            if ( $line =~ /(\S+)\s*\)$/ ) {
                $tesh_file = $1;
                $tesh_file =~ s/^[^\/\$]/$path\/$&/;
                $tesh_file =~ s/\$\{CMAKE_HOME_DIRECTORY\}/$proj_dir/g;
                if ( ! -e "$tesh_file" ) {
                    print "# tesh_file: $tesh_file does not exist!\n";
                    print "# $line\n";
                    next;
                }
            }

            if (0) {
                print "test_name = $name_test\n";
                print "config_var = $config_var\n";
                print "path = $path\n";
                foreach my $key (keys %environ) {
                    print "$key = $environ{$key}\n";
                }
                print "tesh_file = $tesh_file\n";
                print "\n\n";
            }

            my ($count)        = 0;
            my ($close)        = 0;
            open TESH_FILE, $tesh_file or die "Unable to open tesh file: \"$tesh_file\". $!\n";
            my ($input) = "";
            my ($l);
            while ( defined( $l = <TESH_FILE> ) ) {
                chomp $l;
                if ( $l =~ /^< (.*)$/ ) {
                    $input = $input . $1 . "\n";
                }
                if ( $l =~ /^\$ (.*)$/ ) {
                    my ($command) = $1;
                    foreach my $key (keys %environ) {
                        $command = var_subst($command, $key, $environ{$key});
                    }
                    # substitute remaining known variables, if any
                    $command = var_subst($command, "srcdir", "");
                    $command = var_subst($command, "bindir", "");
                    $command = var_subst($command, "EXEEXT", "");
                    $command = var_subst($command, "SG_TEST_EXENV", "");
                    $command = var_subst($command, "SG_TEST_ENV", "");
                    $command = var_subst($command, "SG_EXENV_TEST", "");
                    $command =~ s/\$@//g;
#                    $command =~ s/..\/..\/bin\/smpirun/\${CMAKE_BINARY_DIR\}\/bin\/smpirun/g;
                    $command =~ s/^\s+//;
                    $command =~ s/^[^\/\$]\S*\//$path\/$&/;
                    $command =~ s/^(\S*\/)(?:\.\/)+/$1/g;

                    if ($config_var) {
                        $command = "$command $config_var";
                    }
                    if ( $command =~ /^mkfile\s+(\S+)/) {
                        my $file = $1;
                        # don't ask me why so many backslashes. http://xkcd.com/1638/
                        $input =~ s/\\/\\\\\\\\/g;
                        $input =~ s/\n/\\\\n/g;
                        $input =~ s/"/\\\\042/g;
                        $input =~ s/'/\\\\047/g;
                        $input =~ s/%/%%/g;
                        $command = "sh -c \"printf '$input' > $file\"";
                    }
                    if ($factories) {
                      foreach my $factory (split(';', $factories)) {
                        $close=0;
                        if( $factory =~ /^raw/) {
                            print "${indent}if(HAVE_RAW_CONTEXTS)\n";
                            $close=1;
                        }elsif( $factory =~ /^boost/) {
                            print "${indent}if(HAVE_BOOST_CONTEXTS)\n";
                            $close=1;
                        }elsif( $factory =~ /^ucontext/) {
                            print "${indent}if(HAVE_UCONTEXT_CONTEXTS)\n";
                            $close=1;
                        }elsif( $factory =~ /^thread/) {
                            print "${indent}if(HAVE_THREAD_CONTEXTS)\n";
                            $close=1;
                        }
                        print "${indent}ADD_TEST(NAME memcheck-$name_test-$factory-$count\n";
                        print "${indent}         WORKING_DIRECTORY $path\/\n";
                        print "${indent}         COMMAND $command --cfg=contexts/factory:$factory)\n";
                        if ($count > 0) {
                            print "${indent}set_tests_properties(memcheck-$name_test-$factory-$count\n";
                            print "${indent}                     PROPERTIES DEPENDS memcheck-$name_test-$factory-" . ($count - 1) . ")\n";
                        }
                       if ($close > 0){
                        print "${indent}endif()\n";
                        }
                      }
                    } else {
                      print "${indent}ADD_TEST(NAME memcheck-$name_test-$count\n";
                      print "${indent}         WORKING_DIRECTORY $path\/\n";
                      print "${indent}         COMMAND $command)\n";
                      if ($count > 0) {
                          print "${indent}set_tests_properties(memcheck-$name_test-$count\n";
                          print "${indent}                     PROPERTIES DEPENDS memcheck-$name_test-" . ($count - 1) . ")\n";
                      }
                    }
                    $input = "";
                    #push @test_list, "memcheck-$name_test-$count";
                    $count++;
                }
                if ( $l =~ /^\& (.*)$/ ) {
                    last;
                }
            }
            close(TESH_FILE);
        }
        elsif ( $line =~ /^\s*SET_TESTS_PROPERTIES/ ) {
            if ( $line =~ /SET_TESTS_PROPERTIES\(([\S]+)/ ) {
                my ($name_temp) = ($1);
                $line =~ s/$name_temp/memcheck-$name_temp-0/g;
                print $line. "\n";
            }
        } elsif ( $line =~ /^(\s*)ADD_TEST\((.*)\)/ ) {
          print "$1ADD_TEST(memcheck-$2)\n";
        } else {
            print $line. "\n";
        }
    }
}
close(MAKETEST);

#print "nb_test = $nb_test\n";
#print "set(MEMCHECK_LIST\n";
#print (join("\n", @test_list));
#print ")\n";
