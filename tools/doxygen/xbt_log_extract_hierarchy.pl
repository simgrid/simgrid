#!/usr/bin/env perl

# Copyright (c) 2008, 2010, 2012-2014. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

use strict;
use warnings;

my $debug = 0;

print "/* Generated file, do not edit */\n";
print "/** \\addtogroup XBT_log_cats\n";
print "        \@{\n";

# Search for calls to macros defining new channels, and prepare the tree representation
my %ancestor;
my %desc;
# $ancestor{"toto"} is the ancestor of the toto channel as declared by XBT_LOG_NEW_SUBCATEGORY and 
# XBT_LOG_NEW_DEFAULT_SUBCATEGORY ie, when the channel toto is initialized (does not work under windows)

# $desc{"toto"} is its description
my %connected;
# $connected{"toto"} is defined if XBT_LOG_CONNECT("toto") is used

sub cleanup_ctn {
    my $ctn = shift;        # cleanup the content of a macro call
    $ctn =~ s/^\s*(.*)\s*$/$1/gs;
    my @elms;
    print "ctn=$ctn\n" if $debug > 1;
    if ($ctn =~ m/^(\w+)\s*,\s*(\w+)\s*,\s*"?([^"]*)"?$/s) {
	# Perfect, we got 0->name; 1->anc; 2->desc
	$elms[0] = $1;
	$elms[1] = $2;
	$elms[2] = $3;
    } elsif ($ctn =~ m/^(\w+)\s*,\s*"?([^"]*)"?$/s) {
	# Mmm. got no ancestor. Add the default one.
	$elms[0] = $1;
	$elms[1] = "XBT_LOG_ROOT_CAT";
	$elms[2] = $2;
    } else {
	die "Unparsable content: $ctn\n";
    }
    $elms[2] =~ s/\\\\/\\/gs;
    return @elms;
}

sub parse_file {
    my $filename = shift;

    my $data = "";

    print "Parse $filename\n" if $debug;
    open IN, "$filename" || die "Cannot read $filename: $!\n";
    while (<IN>) {
	$data .= $_;
    }
    close IN;

    # Purge $data from C comments
    $data =~ s|/\*.*?\*/||sg;

    # C++ comments are forbiden in SG for portability reasons, but deal with it anyway
    $data =~ s|//.*$||mg;

    my $connect_data = $data; # save a copy for second parsing phase
    while ($data =~ s/^.*?XBT_LOG_NEW(_DEFAULT)?_(SUB)?CATEGORY\(//s) {
	$data =~ s/([^"]*"[^"]*")\)//s || die "unparsable macro: $data";

        my ($name,$anc,$desc) = cleanup_ctn($1);
    
        # build the tree, checking for name conflict
        die "ERROR: Category name conflict: $name used several times (in $ancestor{$name} and $anc, last time in $filename)\n"
	   if defined ($ancestor{$name}) && $ancestor{$name} ne $anc && defined ($desc{$name}) && $desc{$name} ne $desc;
       $ancestor{$name}=$anc;
       $desc{$name}=$desc;

       print " $name -> $anc\n" if $debug;
   }

   # Now, look for XBT_LOG_CONNECT calls
   $data = $connect_data;
   while ($data =~ s/^.*?XBT_LOG_CONNECT\(//s) {
       $data =~ s/\s*(\w+)\s*\)//s || die "unparsable macro: $data";
       $connected{$1} = 1;
   }
}
# Retrieve all the file names, and add their content to $data
my $data;
open FILES, "find ../src/ ../tools/ ../include/ -name '*.c' -o -name '*.cpp' |" || die "Cannot search for the source file names: $!\n";
while (my $file=<FILES>) {
    chomp $file;
    parse_file($file); 	
}
close FILES;

# Display the tree, looking for disconnected elems    
my %used;

sub display_subtree {
    my $name=shift;
    my $indent=shift;
    
    $used{$name} = 1;
    unless ($name eq "XBT_LOG_ROOT_CAT") { # do not display the root
	print "$indent - $name: ".($desc{$name}|| "(undocumented)")."\n";
    }
    foreach my $cat (grep {$ancestor{$_} eq $name} sort keys %ancestor) {
	display_subtree($cat,"$indent  ");
    }
}

display_subtree("XBT_LOG_ROOT_CAT","");

map {
    if ($_ ne "mc_main") { # This one is not in libsimgrid
      warn "Category $_ does not seem to be connected.  Use XBT_LOG_CONNECT($_).\n";
    }
} grep {!defined $connected{$_}} sort keys %ancestor;
map {
    warn "Category $_ does not seem to be connected to the root (anc=$ancestor{$_})\n";
} grep {!defined $used{$_}} sort keys %ancestor;

print "@}*/\n";
