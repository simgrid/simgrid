#! /usr/bin/perl

use strict;
use Getopt::Long qw(GetOptions);

#open TMP,">mk_supernovae.pl.args";
#map {print TMP "$_ "} @ARGV;
#close TMP;

sub usage($) {
    my $ret;
    print "USAGE: mk_supernovae.pl [--fragile=file]* --out=file file1 file2*\n";
    print "  --help: show this message\n";
    print "  --fragile=file: specify that file is fragile and shouldn't be supernovaed\n";
    print "  --out=file: specify the name of the output file\n";
    print "elements may be separated by semi-columns (;) instead of spaces, too\n";
    exit $ret;
}

my @fragile_files=undef;
my $outfile=undef;
my $help;

Getopt::Long::config('permute','no_getopt_compat', 'no_auto_abbrev');
GetOptions(
    'help|h'                => \$help,

    'fragile=s' =>\@fragile_files,
    'out=s'     =>\$outfile) or usage(1);

@fragile_files = split(/;/,join(';',@fragile_files));
@fragile_files = split(/ /,join(' ',@fragile_files));

usage(0) if (defined($help));
unless(defined($outfile)) {
    print "ERROR: No outfile defined.\n";
    usage(1);
}

#print "mk_supernovae: generate $outfile\n";  

open OUT, ">$outfile" or die "ERROR: cannot open $outfile: $!\n";

print OUT <<EOF
#define SUPERNOVAE_MODE 1
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE   /* for getline() with older libc */
#endif
#include <ctype.h>
#include "portable.h"
#include "xbt.h"
  
EOF
  ;

sub readfile($) {
    my $filename=shift;
    open IN,"$filename" || die "ERROR: cannot read $filename: $!\n";
    my $res;
    while (<IN>) {
	$res .= $_;
    }
    close IN;  
    return $res;
}


my %fragile;
map {$fragile{$_}=1} @fragile_files;
my @args = split(/;/,join(';',@ARGV));
@args = split(/ /,join(' ',@args));
my $nbfile=0;
foreach my $file (@args) {
    if ($fragile{$file}) {
	print "mk_supernovae: $file is fragile, skip it\n";
	next;
    } 
#	print "mk_supernovae: process $file\n";
    $nbfile++;

    my $needundef=1;
    print OUT "/* file $file */\n";
    if ($file eq "xbt/log.c") {
	print OUT "  #define _simgrid_log_category__default &_simgrid_log_category__log\n";
    } else {
	my $ctn = readfile($file);
	if ($ctn =~ m/XBT_LOG_[^ ]*?DEFAULT_[^ ]*?CATEGORY/s) {
	    my $default=$ctn;
	    $default =~ s/.*XBT_LOG_[^ ]*?DEFAULT_[^ ]*?CATEGORY[^(]*\(([^,)]*).*$/$1/s;
	    print OUT "  #define _simgrid_log_category__default &_simgrid_log_category__$default\n";
	} else {
	    print OUT "  /* no default category in file $file */\n";
	    $needundef = 0;
	}
    }
    print OUT "  #include \"$file\"\n";
    print OUT "  #undef _simgrid_log_category__default\n" if $needundef;
    print OUT "\n";
}
close OUT;
print "mk_supernovae: $outfile contains $nbfile files inlined\n";
