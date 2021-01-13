#! /usr/bin/env perl

# Copyright (c) 2010-2021. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

use strict;

my $print = 0;
my $line;

while($line=<STDIN>) {
    chomp $line;
    if($line=~/IMG/) {$print=1;}
    if($print) {print $line."\n";}
    if($line=~/\/MAP/) {$print=0;}
}
#perl -pe 's/imagemap/simgrid_modules/g'| perl -pe 's/<IMG/<IMG style=border:0px/g'		
