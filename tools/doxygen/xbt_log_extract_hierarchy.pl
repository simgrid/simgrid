#! /usr/bin/perl

use strict;
use warnings;

my $debug = 0;


# Search for calls to macros defining new channels, and prepare the tree representation
my %ancestor;
my %desc;
# $ancestor{"toto"} is the ancestor of the toto channel
#    as declared by XBT_LOG_NEW_SUBCATEGORY and XBT_LOG_NEW_DEFAULT_SUBCATEGORY
#    ie, when the channel toto is initialized (does not work under windows)

# $desc{"toto"} is its description
my %c_ancestor;
# $c_ancestor{"toto"} is the ancestor of the toto channel, as declared by XBT_LOG_CONNECT
#    ie, in a initialization function (only way to do so under windows)
#    we want $ancestor{"toto"} == $c_ancestor{"toto"} for each toto, or bad things will happen under windows

sub cleanup_ctn {
    my $ctn = shift;        # cleanup the content of a macro call
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
    if (scalar(@elms) eq 3) {
	# Perfect, we got 0->name; 1->anc; 2->desc
    } elsif (scalar(@elms) eq 2) {
	# Mmm. got no ancestor. Add the default one.
	$elms[2] = $elms[1]; # shift the desc
	$elms[1] = "XBT_LOG_ROOT_CAT";
    } else {
	my $l = scalar(@elms);
	my $s = "";
	map {$s .= $_;} @elms;
	die "Unparsable content: $ctn (length=$l) (content=$s)\n";
    }
    
    $elms[0] =~ s/^\s*(\S*)\s*$/$1/; # trim
    $elms[1]  =~ s/^\s*(\S*)\s*$/$1/; # trim

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
	$data =~ s/([^"]*"[^"]*")\)//s || die "unparsable macro: $data"; # ]]);
	    
        my ($name,$anc,$desc) = cleanup_ctn($1);
	    
        # build the tree, checking for name conflict
        die "ERROR: Category name conflict: $name used several times (in $ancestor{$name} and $anc, last time in $filename)\n"
	   if defined ($ancestor{$name}) && $ancestor{$name} ne $anc &&
              defined ($desc{$name}) && $desc{$name} ne $desc;
       $ancestor{$name}=$anc;
       $desc{$name}=$desc;
   
       print " $name -> $anc\n" if $debug;
   }

   # Now, look for XBT_LOG_CONNECT calls
   $data = $connect_data;
   while ($data =~ s/^.*?XBT_LOG_CONNECT\(//s) {
									 
	$data =~ s/([^\)]*)\)//s || die "unparsable macro: $data"; # ]]);	    
        my ($name, $ignoreme, $anc) = cleanup_ctn($1);
	    
        # build the tree, checking for name conflict
       $c_ancestor{$name}=$anc;
   
       print STDERR " $name -> $anc\n" if $debug;
   }
}
# Retrieve all the file names, and add their content to $data
my $data;
open FILES, "find -name '*.c'|" || die "Cannot search for the source file names: $!\n";
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

sub check_connection {
    my $name=shift;
    
    foreach my $cat (grep {$ancestor{$_} eq $name} sort keys %ancestor) {
	unless ($ancestor{$cat} eq "XBT_LOG_ROOT_CAT" || (defined($c_ancestor{$cat}) && $c_ancestor{$cat} eq $name)) {
	    warn "Category $cat will be disconnected under windows. Add the following to an initialization function:\n   XBT_LOG_CONNECT($cat, $ancestor{$cat});\n";
	} else {
	    warn "Correctly connected, even under windows: Category $cat.\n" if $debug;
	}
	check_connection($cat);
    }
}
check_connection("XBT_LOG_ROOT_CAT");	
map {warn "Category $_ does not seem to be connected to the root (anc=$ancestor{$_})\n";} grep {!defined $used{$_}} sort keys %ancestor;    
