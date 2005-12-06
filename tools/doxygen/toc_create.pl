#!/usr/bin/perl -w

($#ARGV >= 1) or die "Usage: toc_create.pl <input-tag-file> <output-doc-file>";

my(@toc);
my($level,$label,$name);

$input  = $ARGV[0];
$output = $ARGV[1];
open FILE,$input;

while($line=<FILE>) {
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

open OUTPUT,"> $output";
my($current_level)=-1;
my($entry);
print OUTPUT "<!-- Automatically generated table of contents --!>\n";
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
print OUTPUT "<!-- End of automatically generated table of contents --!>\n";


# foreach $type qw(define enumeration enumvalue function typedef) {
#     if(defined $database{$type}) {
# 	print OUTPUT "<h2>$type</h2> \n  <ul>\n";
# 	foreach $name (sort keys %{$database{$type}}) {
# 	    if($type eq "function") {
# 		print OUTPUT "\t<LI> $name()</LI>\n";
# 	    } else {
# 		print OUTPUT "\t<LI> #$name</LI>\n";
# 	    }
# 	}
# 	print OUTPUT "\n  </ul>\n";
#     }
# }
# print OUTPUT "*/";
# close OUTPUT;

