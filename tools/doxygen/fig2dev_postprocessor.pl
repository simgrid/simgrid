#! /usr/bin/perl

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
