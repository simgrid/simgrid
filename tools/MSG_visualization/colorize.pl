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

while (<>) {
    $orgline = $thisline = $_;

    if ( $thisline =~ /^\[[0-9\.]*\] P[0-9]* \|/ ) {
        ( $number, $message ) = split ( / \| /, $thisline );
        chomp $message;
        $head = $number;
        $number =~ s/^\[[0-9\.]*\] P//;
        $number =~ s/^ .*$//;
        $head   =~ s/^(\[.*\]) (.*)$/$col_norm$1 $coltab[($number-1) % scalar(@coltab)]$2/;
        print $head. " " . $message . $col_norm . "\n";
        next;
    }
    print $col_default. $orgline;
}

print $col_norm;
