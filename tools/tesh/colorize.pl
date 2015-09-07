#!/usr/bin/env perl

# Copyright (c) 2010, 2014. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

use warnings;

use Term::ANSIColor qw{:constants};
$Term::ANSIColor::AUTORESET = 1;

while(<>){
    if($_ =~ m/^(\d+\:\s)?[-]\s.*/){
	print BOLD RED $_;
    }elsif ($_ =~ m/^(\d+\:\s)?[+]\s.*/){
	print BOLD GREEN $_;
    }else{
	print BOLD $_;
    }
}


