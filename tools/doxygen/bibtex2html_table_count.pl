#!/usr/bin/perl -w
use utf8;

$first=1;
while($line = <STDIN>) {
    chomp $line;
    if($line =~ /^\s*@[\w\s]*{/) {
	if(!$first) {
	    $count{$cat}{$year}++;
	} else {
	    $first=0;
	}
	next;
    }
    if($line =~ /^\s*year\s*=\s*/i) {
	$year = $line;
	$year =~ s/\D*//g;
    }
    if($line =~ /^\s*category\s*=\s*/i) {
	$cat = $line;
	$cat =~ s/^.*=//;
	$cat =~ s/\s*//g;
	$cat =~ s/\W*//g;
    }
}
$count{$cat}{$year}++;

%pretty_print = (
    "core" => "Other publications about the SimGrid framework",
    "extern" => "Papers that use SimGrid-generated results (not counting our owns)",
    "intra" => "Our own papers that use SimGrid-generated results"
    );

@years=();
foreach $cat (keys %count) {
    push @years, keys %{$count{$cat}};
}

@years = sort {$a <=> $b} @years;
$year_min = $years[0];
$year_max = $years[$#years];

#Print
print "<table border='1' cellspacing='3' cellpadding='3'>
<tr><td>Year</td>";
foreach $year ($year_min..$year_max) {
    print "<td>$year</td> ";
}
print "<td>Total</td>\n";
print "</td>\n";

foreach $cat (keys %count) {
    $sum = 0;
    print "<tr><td>$pretty_print{$cat}</td>";
    
    foreach $year ($year_min..$year_max) {
	if(defined($count{$cat}{$year})) {
	    print "<td>$count{$cat}{$year}</td> ";
	    $sum+=$count{$cat}{$year};
	} else {
	    print "<td>-</td> ";
	}
    }
    print "<td>$sum</td>\n";
    print "</tr>\n";
}



print "<tr><td>Total </td>";

$ssum=0;
foreach $year ($year_min..$year_max) {
    $sum = 0;
    foreach $cat (keys %count) {
	if(defined($count{$cat}{$year})) {
	    $sum+=$count{$cat}{$year};
	}
    } 
    $ssum+=$sum;
    print "<td>$sum</td> ";
}
print "<td>$ssum</td> ";
print "</tr>\n";


print "</table>\n";

