#!/usr/bin/env perl

# Copyright (c) 2013-2015. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

# C comment spell checker
# For each given source file, print the filename, a colon, and the number
# of misspelled words, then a list of misspelled words.
# Words contained in the file stopwords.txt are not considered spelling errors.
# Copyright 2003, Dan Kegel.  Licensed under GPL.  See the file ../COPYING for details.

use strict;
use warnings;

die "Please install iamerican to use that script.\n"
  unless (-r "/usr/lib/ispell/american.hash");

sub check_content($) {
	my $content = shift;
	$content =~ tr/*/ /;
	print POUT "$content\n";
}

my $TEMPFILE="/tmp/spell.tmp";
my $DICTFILE="tools/internal/spell_dict.txt";
$DICTFILE="./spell_dict.txt" unless (-e $DICTFILE);
die "Call this script from its location or from the SimGrid root directory\n" unless (-e $DICTFILE);

die "Usage: ". ($DICTFILE eq "./spell_dict.txt"? "./":"tools/internal/")."spell_comments.pl "
           ."`find ". ($DICTFILE eq "./spell_dict.txt"? "../../":".")." -name '*.[ch]' -o -name '*.hpp' -o -name '*.cpp' |grep -v umpire|grep -v smpi/mpich3-test|grep -v NAS`\n"
  unless scalar(@ARGV)>1;

my $total = 0;
foreach my $file (@ARGV) {
	open (FI, $file) || die "Cannot open $file: $!\n";
	my $content = join ("", <FI>);
	close (FI);

	open(POUT, "> $TEMPFILE") || die;
	$content =~ s!//(.+)$!check_content($1)!egm;
	$content =~ s!/\*(.+?)\*/!check_content($1)!egs;
	close(POUT);

	open(PIN, "ispell -d american -p $DICTFILE -l < $TEMPFILE | sort -uf |") || die;
	my @badwords;
	while (my $err = <PIN>) {
	    chomp $err;	    
	    push(@badwords, $err) if ($err =~ /\w/ && length($err)>0);
	}
	close(PIN) || die;

	if (@badwords) {
		print "$file: ".scalar(@badwords)." errors: '".join("','",@badwords)."'\n";
		$total += scalar(@badwords);    
	}
}

print "Total: $total\n";

unlink($TEMPFILE);
