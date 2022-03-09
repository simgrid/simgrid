#!/usr/bin/env perl

# Copyright (c) 2012-2022. The SimGrid Team. All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

use strict;
use warnings;

my @argv = ("valgrind", "--quiet");
my $count = 0;

while (defined(my $arg = shift)) {
#    print "arg($count)$arg\n";
    if ($arg =~ m!/smpirun$!) {
        @argv = ( $arg, "-keep-temps", "-quiet", "-wrapper", "@argv" );
    } elsif ($arg eq "--cd") {
        $arg = shift;
        print "cd $arg\n";
        chdir ($arg);
        $count++;
    } else {
        push @argv, $arg;
    }
    $count++;
}

# print "COMMAND:";
# map { print "$_ " } @argv;
# print "\n";

exec @argv;
