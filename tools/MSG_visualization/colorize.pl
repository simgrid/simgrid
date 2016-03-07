#!/usr/bin/env perl

# Copyright (c) 2005, 2007, 2010, 2014. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

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

# Get options
while (($_ = $ARGV[0]) =~ /^-/) {
    shift;
    if (/-location/i) {
        $opt_print_location = 1;  shift;
    } elsif (/-h(elp)?$|-u(sage)?$/i) {
        print<<EOH
colorize.pl - Log colorizer for SimGrid
Syntax:

    colorize.pl [options] <file>

    where <file> is a text file of values or '-' for STDIN

Options: () denote short version

    -location          Print the location in the code of the message.
EOH
;
	exit;
    }
}

sub pidcolor {
    my $pid = shift;

    unless (defined($pid{$pid})) {
	# first time we see this pid. Affect it a color
	$pid{$pid}=(scalar keys %pid) % (scalar @coltab);
    }
    return $coltab[$pid{$pid}];
}

sub print_line {
    my($host,$procname,$pid,$date,$location,$xbt_channel,$message)=@_;

    print $col_norm;
    printf "[% 10.6f]",$date;

    print pidcolor($pid);

    if(defined($location)) {
	printf "[%10s:%-10s] %s ",$host,$procname,$location;
    } else {
	printf "[%10s:%-10s]",$host,$procname;
    }
    print " $message";
    print $col_norm."\n";
}

# Read the messages and do the job
while (<>) {
    $orgline = $thisline = $_;

    # Typical line  [Gatien:slave:(9) 11.243148] msg/gos.c:137: [msg_gos/DEBUG] Action terminated
    if ($thisline =~ /^\[(.+):([^:]+):\((\d+)\) ([\d\.]*)\] ([^\[]*) \[([^\[]*)\] (.*)$/) {
	$host=$1;
	$procname=$2;
	$pid=$3;
	$date=$4;
	if($opt_print_location) {
	    $location=$5;
	    $location =~ s/:$//;
	} else {
	    $location = undef;
	}
	$xbt_channel=$6;
	$message=$7;

	print_line($host,$procname,$pid,$date,$location,$xbt_channel,$message);
    # Typical line  [Boivin:slave:(2) 9.269357] [pmm/INFO] ROW: step(2)<>myrow(0). Receive data from TeX
    } elsif ($thisline =~ /^\[([^:]+):([^:]+):\((\d+)\) ([\d\.]*)\] \[([^\[]*)\] (.*)$/) {
	$host=$1;
	$procname=$2;
	$pid=$3;
	$date=$4;
	$xbt_channel=$5;
	$message=$6;
	print_line($host,$procname,$pid,$date,undef,$xbt_channel,$message);
    } elsif ( $thisline =~ /^==(\d+)== (.*)$/) {
	# take care of valgrind outputs
	print pidcolor($1)."$2\n";

    } else {
	print $col_default. $orgline;
    }
}

print $col_norm;
