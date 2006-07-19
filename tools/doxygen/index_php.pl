#!/usr/bin/perl -w

($#ARGV >= 2) or die "Usage: index_php.pl <input-php.in-file> <input-html-file> <output-php-file>";

my(@toc);
my($level,$label,$name);

$inputphp  = $ARGV[0];
$inputhtml  = $ARGV[1];
$output = $ARGV[2];

my($onglets) = "";
my($body) = "";

open FILE,$inputhtml;
while(defined($line=<FILE>)) {
    if($line =~/<div class="tabs">/) {
	$onglets = $line;
	while(defined($line=<FILE>) && !($line =~/<\/div>/)) {
	    $onglets.=$line;
	}
	$onglets.=$line;
	$onglets.="<center></center><p><br>\n"
    } 
    if($line =~/<!-- ______BODY_BEGIN______ --!>(.*)$/) {
	$tmp=$1;
	if($tmp =~/(.*)<!-- ______BODY_END______ --!>/) {
	    $body .= $1;
	} else {
	    $body .= $tmp;
	    while(defined($line=<FILE>) && !($line =~/<!-- ______BODY_END______ --!>/)) {
		$body.=$line;
	    }
	    $line =~/^(.*)<!-- ______BODY_END______ --!>/;
	    $body.=$1;
	}
    }
}
close FILE;

# (?!http) : A zero-width negative look-ahead assertion. 
# For example "/foo(?!bar)/" matches any occurrence of "foo" that isn’t followed by "bar".

$onglets =~ s/href=\"(?!http)/href=\"doc\//gi;
$onglets =~ s/src=\"(?!http)/src=\"doc\//gi;

$body =~ s/href=\"(?!http)/href=\"doc\//gi;
$body =~ s/src=\"(?!http)/src=\"doc\//gi;

open FILE,$inputphp;
open OUTPUT,"> $output";

while(defined($line=<FILE>)) {
    chomp $line;
    if($line =~/______ONGLETS______/) {
	$line =~ s/______ONGLETS______/$onglets/g;
    } elsif($line =~/______BODY______/) {
	$line =~ s/______BODY______/$body/g;
    }
    print OUTPUT "$line\n";
}

close(FILE);
close(OUTPUT);
