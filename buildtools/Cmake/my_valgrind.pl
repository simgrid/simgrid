#!/usr/bin/perl -w
use strict;
my($arg)="";
my($val_path)="/usr/bin"; #By default
my($count)=0;

while($count!=$#ARGV+1)
{
	print "arg($count)$ARGV[$count]\n";
	if($ARGV[$count] eq "--cd"){
		print "cd $ARGV[$count+1]\n";
		chdir ($ARGV[$count+1]);
		$count++;
	}
	elsif($ARGV[$count] eq "--valgrind") {
		print "valgrind_path $ARGV[$count+1]\n";
		$val_path = $ARGV[$count+1];
		$count++;
	}
	else{
		$arg="$arg $ARGV[$count]";
	}
	$count++;
}

#print "COMMAND : $bin $option $cd $path\n";
#print "cd $path\n";
#print "$val_path\/valgrind --trace-children=yes --leak-check=full --show-reachable=yes --track-origins=yes --read-var-info=no $bin $option\n";
print "$val_path\/valgrind $arg\n\n";
system "$val_path\/valgrind $arg";
