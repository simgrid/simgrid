#!/usr/bin/perl -w
use utf8;

$write=0;
while($line = <STDIN>) {
    chomp $line;
    if($line =~ /table width/) {
	$line = <STDIN>;
	chomp $line;
	if($line =~ /height="50"/) {
	    ## Skip preembule
	    $write=1;
	    do {
		$line = <STDIN>;
		chomp $line;
	    } while (!($line =~ /table/));
	    next;
	}
	if($line =~ /height="35"/) {
	    ## Change this uggly table into a h2
	    do {
		$line = <STDIN>;
		chomp $line;
	    } while (($line =~ /</));
	    $title=$line;
	    do {
		$line = <STDIN>;
		chomp $line;
	    } while (!($line =~ /table/));
	    print "<h1>$title</h1>\n";
	    next;
	}
    }
    $line =~ s/<ol>/<ul>/g;
    $line =~ s/<\/ol>/<\/ul>/g;

    if($line =~ /Disclaimer:/ || $line =~ /body>/) {
	$write=0;
	next;
    }
    if($line =~ /This document was translated f/) {
	$write=1;
    }

    if($write) {
	print $line."\n";
    }
    
}
