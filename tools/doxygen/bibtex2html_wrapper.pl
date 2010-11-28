#!/usr/bin/perl -w
use utf8;
use strict;

my $file = shift @ARGV || die "USAGE: bibtex2html_wrapper <file>\n";

my $output;

open IN,"bibtex2html -single-output -nv -force -sort year -copy-icons ${file}.bib -output -|iconv -f latin1 -t utf8 -|";

$write=0;
while($line = <IN>) {
    chomp $line;
    next if $line =~ /WARNING: unknown field type/;
    
    if($line =~ /table width/) {
	$line = <IN>;
	chomp $line;
	if($line =~ /height="50"/) {
	    ## Skip preembule
	    $write=1;
	    do {
		$line = <IN>;
		chomp $line;
	    } while (!($line =~ /table/));
	    next;
	}
	if($line =~ /height="35"/) {
	    ## Change this uggly table into a h2
	    do {
		$line = <IN>;
		chomp $line;
	    } while (($line =~ /</));
	    $title=$line;
	    do {
		$line = <IN>;
		chomp $line;
	    } while (!($line =~ /table/));
	    $output .= "<h1>$title</h1>\n";
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
	$output .= $line."\n";
    }
    
}
