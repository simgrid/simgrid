#! /usr/bin/perl

use strict;
use warnings;

my $debug = 0;


# Search for calls to macros defining new channels, and prepare the tree representation
my %ancestor;
my %desc;
# $ancestor{"toto"} is the ancestor of the toto channel
# $desc{"toto"} is its description

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

    while ($data =~ s/^.*?XBT_LOG_NEW(_DEFAULT)?_(SUB)?CATEGORY\(//s) {
	$data =~ s/([^"]*"[^"]*")\)//s || die "unparsable macro: $data"; # ]]);
	my $ctn = $1;
    
        # cleanup the content											 
	$ctn =~ s/ *\n//gs;										 
        $ctn =~ s/,\s*"/,"/gs;
        $ctn =~ s/"\s*$/"/gs;
        $ctn =~ s/,\s*/,/gs;
        my @elms_tmp=split (/,/,$ctn); 
        my @elms;
        print "ctn=$ctn\n" if $debug > 1;
        # There may be some ',' in the description. Remerge the stuff like: "description, really"
        while (1) {
	    my $acc = shift @elms_tmp;
	    last unless defined $acc;
  	    if ($acc =~ /^"/) { # ") {
	        while (shift @elms_tmp) { 
		   $acc .= $_;
 	        }
	        die "Unparsable content: $ctn\n"
	           unless ($acc =~ s/^"(.*)"$/$1/);
	    }
	    print "  seen $acc\n" if $debug > 1;
	    push @elms, $acc;
        }

        my ($name,$anc,$desc);
        # build the tree, checking for name conflict
        if (scalar(@elms) eq 3) {
	   $name = $elms[0];
	   $anc  = $elms[1];
	   $desc = $elms[2];
        } elsif (scalar(@elms) eq 2) {
	   $name = $elms[0];
	   $anc  = "XBT_LOG_ROOT_CAT";
	   $desc = $elms[1];
        } else {
	   my $l = scalar(@elms);
	   my $s = "";
	   map {$s .= $_;} @elms;
	   die "Unparsable content: $ctn (length=$l) (content=$s)\n";
        }
	$name =~ s/^\s*(\S*)\s*$/$1/; # trim
	$anc  =~ s/^\s*(\S*)\s*$/$1/; # trim
        die "ERROR: Category name conflict: $name used several times\n"
	   if defined ($ancestor{$name}) && $ancestor{$name} ne $anc &&
              defined ($desc{$name}) && $desc{$name} ne $desc;
       $ancestor{$name}=$anc;
       $desc{$name}=$desc;
   
       print " $name -> $anc\n" if $debug;
   }
}
# Retrieve all the file names, and add their content to $data
my $data;
open FILES, "find -name '*.c'|" || die "Cannot search for the source file names; $!\n";
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

map {warn "Category $_ does not seem to be connected to the root (anc=$ancestor{$_})\n";} grep {!defined $used{$_}} sort keys %ancestor;    
