#!/usr/bin/perl -w

use strict;

($#ARGV >= 1) or die "Usage: toc_create.pl <input-doc-file>+";

sub handle_file {
  my $infile = shift;
  open FILE,$infile;

  my(@toc);
  my($level,$label,$name);

  while(my $line=<FILE>) {
    chomp $line;
    if($line=~/\\section\s*(\S\S*)\s*(.*)$/) {
#	print "$line\n";
	$label = $1;
	$name = $2;
	$level=0;
#	print "$label : $name\n";
	push @toc,[$level,$label,$name];
    } elsif($line=~/\\subsection\s*(\S\S*)\s*(.*)$/) {
#	print "$line\n";
	$label = $1;
	$name = $2;
	$level=1;
#	print "\t$label : $name\n";
	push @toc,[$level,$label,$name];
    } elsif($line=~/\\subsubsection\s*(\S\S*)\s*(.*)$/) {
#	print "$line\n";
	$label = $1;
	$name = $2;
	$level=2;
#	print "\t\t$label : $name\n";
	push @toc,[$level,$label,$name];
    }
  }
  close FILE;

  my $outfile = ".$infile.toc";
  $outfile =~ s|\.\./||g;
  $outfile =~ s|/|_|g;
  open OUTPUT,"> $outfile";
  my($current_level)=-1;
  my($entry);
  print OUTPUT "<!-- Automatically generated table of contents --!>\n";
  print OUTPUT "<div class=\"toc\">\n";
  print OUTPUT "<div class=\"tocTitle\">Table of content</div>\n";
  foreach $entry (@toc) {
      ($level,$label,$name) = @$entry;

      while($current_level<$level) {
  	  print OUTPUT "<ol type=\"1\">\n";
  	  $current_level++;
      }	
      while($current_level>$level) {
	  print OUTPUT "</ol>\n";
	  $current_level--;
      }
      foreach (1..$current_level) {
	  print OUTPUT "\t";
      }
      print OUTPUT "<li> <a href=\"#$label\">$name</a>\n";
  }

  while($current_level>-1) {
      print OUTPUT "</ol>\n";
      $current_level--;
  }
  print OUTPUT "</div>\n";
  print OUTPUT "<!-- End of automatically generated table of contents --!>\n";
} # sub handle_file


map { handle_file($_) } @ARGV;