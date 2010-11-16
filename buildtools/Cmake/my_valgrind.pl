#!/usr/bin/perl -w
use strict;
my @argv = ("valgrind");
my $count = 0;

while (my $arg = shift) {
    print "arg($count)$arg\n";
    if($arg eq "--cd"){
        $arg = shift;
        print "cd $arg\n";
        chdir ($arg);
        $count++;
    } else{
        push @argv, $arg;
    }
    $count++;
}

#print "COMMAND : $bin $option $cd $path\n";
#print "cd $path\n";
#print "valgrind --trace-children=yes --leak-check=full --show-reachable=yes --track-origins=yes --read-var-info=no $bin $option\n";
print "@argv\n\n";
system @argv;
