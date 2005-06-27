#!/usr/bin/perl

$col_white    = "\033[00m";
$col_black    = "\033[30m";
$col_red      = "\033[31m";
$col_green    = "\033[32m";
$col_yellow   = "\033[33m";
$col_blue     = "\033[34m";
$col_purple   = "\033[35m";
$col_cyan     = "\033[36m";
$col_ltgray   = "\033[37m";
$col_darkgray = "\033[30m";

$col_norm       = $col_white;
$col_background = "\033[07m";
$col_brighten   = "\033[01m";
$col_underline  = "\033[04m";
$col_blink      = "\033[05m";

# Customize colors here...
#
$col_default = $col_ltgray;
my (@coltab) = (
    $col_green,                    $col_yellow,
    $col_purple,                   $col_cyan,
    $col_red,                      $col_blue,
    $col_background . $col_green,
    $col_background . $col_yellow, $col_background . $col_purple,
    $col_background . $col_cyan,   $col_background . $col_red,
    $col_background . $col_blue,   $col_background . $col_magenta,
);

my %pid;

sub pidcolor {
    my $pid = shift;

    unless (defined($pid{$pid})) {
	# first time we see this pid. Affect it a color
	$pid{$pid}=(scalar keys %pid) % (scalar @coltab);
    }
    return $coltab[$pid{$pid}];
}

while (<>) {
    $orgline = $thisline = $_;

    if ( $thisline =~ /^\[(\w+):(\w+):\((\d+)\) ([0-9\.]*)\] ([^\[]*) \[([^\[]*)\] (.*)$/ ) {
	$host=$1;
	$procname=$2;
	$pid=$3;
	$date=$4;
	$location=$5;
	$xbt_channel=$6;
	$message=$7;

	print $col_norm;
	printf "[% 10.3f]",$date;

	print pidcolor($pid);
	printf "[%10s:%-10s]",$host,$procname;
	print " $message";
	print $col_norm."\n";
    } elsif ( $thisline =~ /^==(\d+)== (.*)$/) {
	# take care of valgrind outputs
	print pidcolor($1)."$2\n";

    } else {
	print $col_default. $orgline;
    }
}

print $col_norm;
