#! /usr/bin/env perl

# Copyright (c) 2012-2015. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.
eval 'exec perl -S $0 ${1+"$@"}'
  if $running_under_some_shell;

# If you change this file, please stick to the formatting you got with:
# perltidy --backup-and-modify-in-place --maximum-line-length=180 --output-line-ending=unix --cuddled-else

=encoding UTF-8

=head1 NAME

tesh -- testing shell

=head1 SYNOPSIS

B<tesh> [I<options>]... I<testsuite>

=head1 DESCRIPTION

Tesh is the testing shell, a specialized shell for running tests. It
provides the specified input to the tested commands, and check that
they produce the expected output and return the expected value.

=head1 OPTIONS

  --cd some/directory : ask tesh to switch the working directory before
                        launching the tests
  --setenv var=value  : set a specific environment variable
  --cfg arg           : add parameter --cfg=arg to each command line
  --enable-coverage   : ignore output lines starting with "profiling:"

=head1 TEST SUITE FILE SYTAX

A test suite is composed of one or several I<command blocks> separated
by empty lines, each of them being composed of a command to run, its
input text and the expected output.

The first char of each line specifies the type of line according to
the following list. The second char of each line is ignored.

 `$' command to run in foreground
 `&' command to run in background

 `<' input to pass to the command
 `>' output expected from the command

 `!' metacommand, which can be one of:
     `timeout' <integer>|no
     `expect signal' <signal name>
     `expect return' <integer>
     `output' <ignore|display>
     `setenv <key>=<val>'

 `p' an informative message to print

If the expected output do not match the produced output, or if the
command did not end as expected, Tesh provides an error message (see
the OUTPUT section below) and stops.

=head2 Command blocks examples

In a given command block, you can declare the command, its input and
its expected output in the order that you see fit.

    $ cat
    < TOTO
    > TOTO

    > TOTO
    $ cat
    < TOTO

    > TOTO
    < TOTO
    $ cat

You can group several commands together, provided that they don't have
any input nor output.

    $ mkdir testdir
    $ cd testdir

=head2 Enforcing the command return code

By default, Tesh enforces that the tested command returns 0. If not,
it fails with an appropriate message and returns I<code+40> itself.

You specify that a given command block is expected to return another
code as follows:

    # This command MUST return 42
    ! expect return 42
    $ sh -e "exit 42"

The I<expect return> construct applies only to the next command block.

=head2 Commands that are expected to raise signals

By default, Tesh detects when the command is killed by a signal (such
as SEGV on segfaults). This is usually unexpected and unfortunate. But
if not, you can specify that a given command block is expected to fail
with a signal as follows:

    # This command MUST raise a segfault
    ! expect signal SIGSEGV
    $ ./some_failing_code

The I<expect signal> construct applies only to the next command block.

=head2 Timeouts

By default, no command is allowed to run more than 5 seconds. You can
change this value as follows:

    # Allow some more time to the command
    ! timeout 60
    $ ./some_longer_command

You can also disable the timeout completely by passing "no" as a value:

    # This command will never timeout
    ! timeout no
    $ ./some_very_long_but_safe_command

=head2 Setting environment variables

You can modify the environment of the tested commands as follows:

    ! setenv PATH=/bin
    $ my_command

=head2 Not enforcing the expected output 

By default, the commands output is matched against the one expected,
and an error is raised on discrepancy. Metacommands to change this:

=over 4

=item output ignore

The output is completely discarded.

=item output display

The output is displayed, but no error is issued if it differs from the
expected output.

=item output sort

The output is sorted before comparison (see next section).

=back

=head2 Sorting output

If the order of the command output changes between runs, you want to
sort it before enforcing that it is exactly what you expect. In
SimGrid for example, this happens when parallel execution is
activated: User processes are run in parallel at each timestamp, and
the output is not reproducible anymore. Until you sort the lines.

You can sort the command output as follows:

    ! output sort
    $ ./some_multithreaded_command

Sorting lines this ways often makes the tesh output very intricate,
complicating the error analysis: the process logical order is defeated
by the lexicographical sort.

The solution is to prefix each line of your output with temporal
information so that lines can be grouped by timestamps. The
lexicographical sort then only applies to lines that occured at the
same timestamp. Here is a SimGrid example:

    # Sort only lines depending on the first 19 chars
    ! output sort 19
    $ ./some_simgrid_simulator --log=root.fmt:[%10.6r]%e(%i:%P@%h)%e%m%n

This approach may seem surprizing at the first glance but it does its job:

=over 4

=item Every timestamps remain separated, as it should; 

=item In each timestamp, the output order of processes become
   reproducible: that's the lexicographical order of their name;

=item For each process, the order of its execution is preserved: its
   messages within a given timestamp are not reordered.

=back

That way, tesh can do its job (no false positive, no false negative)
despite the unpredictable order of executions of processes within a
timestamp, and reported errors remain easy to analyze (execution of a
given process preserved).

This example is very SimGrid oriented, but the feature could even be
usable by others, who knows?


=head1 BUILTIN COMMANDS

=head2 mkfile: creating a file

This command creates a file of the name provided as argument, and adds
the content it gets as input.

  $ mkfile myFile
  > some content
  > to the file

It is not possible to use the cat command, as one would expect,
because stream redirections are currently not implemented in Tesh.

=head1 BUGS, LIMITATIONS AND POSSIBLE IMPROVEMENTS

The main limitation is the lack of stream redirections in the commands
(">", "<" and "|" shell constructs and friends). The B<mkfile> builtin
command makes this situation bearable.

It would be nice if we could replace the tesh file completely with
command line flags when the output is not to be verified.

=cut

BEGIN {
    # Disabling IPC::Run::Debug saves tons of useless calls.
    $ENV{'IPCRUNDEBUG'} = 'none'
      unless exists $ENV{'IPCRUNDEBUG'};
}

my $enable_coverage        = 0;
my $diff_tool              = 0;
my $diff_tool_tmp_fh       = 0;
my $diff_tool_tmp_filename = 0;
my $sort_prefix            = -1;
my $tesh_file;
my $tesh_name;
my $error    = 0;
my $exitcode = 0;
my @bg_cmds;
my (%environ);
$SIG{'PIPE'} = 'IGNORE';

my $path = $0;
$path =~ s|[^/]*$||;
push @INC, $path;

use lib "@CMAKE_BINARY_DIR@/bin";

use Diff qw(diff);    # postpone a bit to have time to change INC

use Getopt::Long qw(GetOptions);
use strict;
use Text::ParseWords;
use IPC::Run qw(start run timeout finish);
use IO::File;
use English;

####
#### Portability bits for windows
####

use constant RUNNING_ON_WINDOWS => ( $OSNAME =~ /^(?:mswin|dos|os2)/oi );
use POSIX qw(:sys_wait_h WIFEXITED WIFSIGNALED WIFSTOPPED WEXITSTATUS WTERMSIG WSTOPSIG
  :signal_h SIGINT SIGTERM SIGKILL SIGABRT SIGSEGV);

BEGIN {
    if (RUNNING_ON_WINDOWS) { # Missing on windows
        *WIFEXITED   = sub { not $_[0] & 127 };
        *WEXITSTATUS = sub { $_[0] >> 8 };
        *WIFSIGNALED = sub { ( $_[0] & 127 ) && ( $_[0] & 127 != 127 ) };
        *WTERMSIG    = sub { $_[0] & 127 };

	# used on the command lines
	$environ{'EXEEXT'} = ".exe";
    }
}


####
#### Command line option handling
####

my %opts = ( "debug" => 0,
             "timeout" => 5, # No command should run any longer than 5 seconds by default
           );

Getopt::Long::config( 'bundling', 'no_getopt_compat', 'no_auto_abbrev' );
GetOptions(
    'debug|d' => \$opts{"debug"},

    'difftool=s' => \$diff_tool,

    'cd=s'      => sub { cd_cmd( $_[1] ) },
    'timeout=s' => \$opts{'timeout'},
    'setenv=s'  => sub { setenv_cmd( $_[1] ) },
    'cfg=s' => sub { $opts{'cfg'} .= " --cfg=$_[1]" },
    'enable-coverage+' => \$enable_coverage,
);

$tesh_file = pop @ARGV;
$tesh_name = $tesh_file;
$tesh_name =~ s|^.*?/([^/]*)$|$1|;

print "Enable coverage\n" if ($enable_coverage);

if ($diff_tool) {
    use File::Temp qw/ tempfile /;
    ( $diff_tool_tmp_fh, $diff_tool_tmp_filename ) = tempfile();
    print "New tesh: $diff_tool_tmp_filename\n";
}

if ( $tesh_file =~ m/(.*)\.tesh/ ) {
    print "Test suite `$tesh_file'\n";
} else {
    $tesh_name = "(stdin)";
    print "Test suite from stdin\n";
}

###########################################################################

sub exec_cmd {
    my %cmd = %{ $_[0] };
    if ( $opts{'debug'} ) {
        map { print "IN: $_\n" } @{ $cmd{'in'} };
        map { print "OUT: $_\n" } @{ $cmd{'out'} };
        print "CMD: $cmd{'cmd'}\n";
    }

    # substitute environment variables
    foreach my $key ( keys %environ ) {
        $cmd{'cmd'} = var_subst( $cmd{'cmd'}, $key, $environ{$key} );
    }

    # substitute remaining variables, if any
    while ( $cmd{'cmd'} =~ /\${(\w+)(?::[=-][^}]*)?}/ ) {
        $cmd{'cmd'} = var_subst( $cmd{'cmd'}, $1, "" );
    }
    while ( $cmd{'cmd'} =~ /\$(\w+)/ ) {
        $cmd{'cmd'} = var_subst( $cmd{'cmd'}, $1, "" );
    }

    # add cfg options
    $cmd{'cmd'} .= " $opts{'cfg'}"
      if ( defined( $opts{'cfg'} ) && length( $opts{'cfg'} ) );

    # finally trim any remaining space chars
    $cmd{'cmd'} =~ s/^\s+//;
    $cmd{'cmd'} =~ s/\s+$//;

    print "[$tesh_name:$cmd{'line'}] $cmd{'cmd'}\n";

    $cmd{'return'} ||= 0;
    $cmd{'timeout'} ||= $opts{'timeout'};
    

    ###
    # exec the command line

    my @cmdline = quotewords( '\s+', 0, $cmd{'cmd'} );
    my $input = defined($cmd{'in'})? join("\n",@{$cmd{'in'}}) : "";
    my $output = " " x 10240; $output = ""; # Preallocate 10kB, and reset length to 0
    $cmd{'got'} = \$output;
    $cmd{'job'} = start \@cmdline, '<', \$input, '>&', \$output, 
                  ($cmd{'timeout'} eq 'no' ? () : timeout($cmd{'timeout'}));

    if ( $cmd{'background'} ) {
	# Just enqueue the job. It will be dealed with at the end
        push @bg_cmds, \%cmd;
    } else {
	# Deal with its ending conditions right away
        analyze_result( \%cmd );
    }
}

sub analyze_result {
    my %cmd    = %{ $_[0] };
    $cmd{'timeouted'} = 0; # initialization

    # Wait for the end of the child process
    #####
    eval {
	finish( $cmd{'job'} );
    };
    if ($@) { # deal with the errors that occured in the child process
	if ($@ =~ /timeout/) {
	    $cmd{'job'}->kill_kill;
	    $cmd{'timeouted'} = 1;
	} elsif ($@ =~ /^ack / and $@ =~ /pipe/) { # IPC::Run is not very expressive about the pipes that it gets :(
	    print STDERR "Tesh: Broken pipe (ignored).\n";
	} else {
	    die $@; # Don't know what it is, so let it go.
	}
    } 

    # Gather information
    ####
    
    # pop all output from executing child
    my @got;
    map { print "GOT: $_\n" } ${$cmd{'got'}} if $opts{'debug'};
    foreach my $got ( split("\n", ${$cmd{'got'}}) ) {
        $got =~ s/\r//g;
        chomp $got;
        print $diff_tool_tmp_fh "> $got\n" if ($diff_tool);

        unless ( $enable_coverage and $got =~ /^profiling:/ ) {
            push @got, $got;
        }
    }

    # How did the child process terminate?
    my $status = $?;
    $cmd{'gotret'} = "Unparsable status. Please report this tesh bug.";
    if ( $cmd{'timeouted'} ) {
        $cmd{'gotret'} = "timeout after $cmd{'timeout'} sec";
        $error    = 1;
        $exitcode = 3;
    } elsif ( WIFEXITED($status) ) {
        $exitcode = WEXITSTATUS($status) + 40;
	$cmd{'gotret'} = "returned code " . WEXITSTATUS($status);
    } elsif ( WIFSIGNALED($status) ) {
        my $code;
        if    ( WTERMSIG($status) == SIGINT )  { $code = "SIGINT"; }
        elsif ( WTERMSIG($status) == SIGTERM ) { $code = "SIGTERM"; }
        elsif ( WTERMSIG($status) == SIGKILL ) { $code = "SIGKILL"; }
        elsif ( WTERMSIG($status) == SIGABRT ) { $code = "SIGABRT"; }
        elsif ( WTERMSIG($status) == SIGSEGV ) { $code = "SIGSEGV"; }
        $exitcode = WTERMSIG($status) + 4;
        $cmd{'gotret'} = "got signal $code";
    }

    # How was it supposed to terminate?
    my $wantret;
    if ( defined( $cmd{'expect'} ) and ( $cmd{'expect'} ne "" ) ) {
        $wantret = "got signal $cmd{'expect'}";
    } else {
        $wantret = "returned code " . ( defined( $cmd{'return'} ) ? $cmd{'return'} : 0 );
    }

    # Enforce the outcome
    ####
    
    # Did it end as expected?
    if ( $cmd{'gotret'} ne $wantret ) {
        $error = 1;
        my $msg = "Test suite `$tesh_name': NOK (<$tesh_name:$cmd{'line'}> $cmd{'gotret'})\n";
        if ( scalar @got ) {
            $msg = $msg . "Output of <$tesh_name:$cmd{'line'}> so far:\n";
	    map { $msg .= "|| $_\n" } @got;
        } else {
	    $msg .= "<$tesh_name:$cmd{'line'}> No output so far.\n";
	}
        print STDERR "$msg";
    }

    # Does the output match?
    if ( $cmd{'sort'} ) {
        sub mysort {
            substr( $a, 0, $sort_prefix ) cmp substr( $b, 0, $sort_prefix );
        }
        use sort 'stable';
        if ( $sort_prefix > 0 ) {
            @got = sort mysort @got;
        } else {
            @got = sort @got;
        }
        while ( @got and $got[0] eq "" ) {
            shift @got;
        }

        # Sort the expected output too, to make tesh files easier to write for humans
        if ( defined( $cmd{'out'} ) ) {
            if ( $sort_prefix > 0 ) {
                @{ $cmd{'out'} } = sort mysort @{ $cmd{'out'} };
            } else {
                @{ $cmd{'out'} } = sort @{ $cmd{'out'} };
            }
            while ( @{ $cmd{'out'} } and ${ $cmd{'out'} }[0] eq "" ) {
                shift @{ $cmd{'out'} };
            }
        }
    }

    # Report the output if asked so or if it differs
    if ( defined( $cmd{'output display'} ) ) {
        print "[Tesh/INFO] Here is the (ignored) command output:\n";
        map { print "||$_\n" } @got;
    } elsif ( defined( $cmd{'output ignore'} ) ) {
        print "(ignoring the output of <$tesh_name:$cmd{'line'}> as requested)\n";
    } else {
        my $diff = build_diff( \@{ $cmd{'out'} }, \@got );
    
	if ( length $diff ) {
	    print "Output of <$tesh_name:$cmd{'line'}> mismatch" . ( $cmd{'sort'} ? " (even after sorting)" : "" ) . ":\n";
	    map { print "$_\n" } split( /\n/, $diff );
	    if ( $cmd{'sort'} ) {
		print "WARNING: Both the observed output and expected output were sorted as requested.\n";
		print "WARNING: Output were only sorted using the $sort_prefix first chars.\n"
		    if ( $sort_prefix > 0 );
		print "WARNING: Use <! output sort 19> to sort by simulated date and process ID only.\n";

		# print "----8<---------------  Begin of unprocessed observed output (as it should appear in file):\n";
		# map {print "> $_\n"} @{$cmd{'unsorted got'}};
		# print "--------------->8----  End of the unprocessed observed output.\n";
	    }
	    
	    print "Test suite `$tesh_name': NOK (<$tesh_name:$cmd{'line'}> output mismatch)\n";
	    exit 2;
	}
    }
}

# parse tesh file
my $infh;    # The file descriptor from which we should read the teshfile
if ( $tesh_name eq "(stdin)" ) {
    $infh = *STDIN;
} else {
    open $infh, $tesh_file
      or die "[Tesh/CRITICAL] Unable to open $tesh_file: $!\n";
}

my %cmd;     # everything about the next command to run
my $line_num = 0;
LINE: while ( not $error and defined( my $line = <$infh> )) {
    chomp $line;
    $line =~ s/\r//g;

    $line_num++;
    print "[TESH/debug] $line_num: $line\n" if $opts{'debug'};

    # deal with line continuations
    while ( $line =~ /^(.*?)\\$/ ) {
        my $next = <$infh>;
        die "[TESH/CRITICAL] Continued line at end of file\n"
          unless defined($next);
        $line_num++;
        chomp $next;
        print "[TESH/debug] $line_num: $next\n" if $opts{'debug'};
        $line = $1 . $next;
    }

    # If the line is empty, run any previously defined block and proceed to next line
    unless ( $line =~ m/^(.)(.*)$/ ) {
        if ( defined( $cmd{'cmd'} ) ) {
            exec_cmd( \%cmd );
            %cmd = ();
        }
        print $diff_tool_tmp_fh "$line\n" if ($diff_tool);
        next LINE;
    }

    my ( $cmd, $arg ) = ( $1, $2 );
    print $diff_tool_tmp_fh "$line\n" if ( $diff_tool and $cmd ne '>' );
    $arg =~ s/^ //g;
    $arg =~ s/\r//g;
    $arg =~ s/\\\\/\\/g;

    # Deal with the lines that can contribute to the current command block
    if ( $cmd =~ /^#/ ) {    # comment
	next LINE;
    } elsif ( $cmd eq '>' ) {    # expected result line
        print "[TESH/debug] push expected result\n" if $opts{'debug'};
        push @{ $cmd{'out'} }, $arg;
	next LINE;

    } elsif ( $cmd eq '<' ) {    # provided input
        print "[TESH/debug] push provided input\n" if $opts{'debug'};
        push @{ $cmd{'in'} }, $arg;
	next LINE;

    } elsif ( $cmd eq 'p' ) {    # comment
        print "[$tesh_name:$line_num] $arg\n";
	next LINE;

    } 

    # We dealt with all sort of lines that can contribute to a command block, so we have something else here.
    # If we have something buffered, run it now and start a new block
    if ( defined( $cmd{'cmd'} ) ) {
	exec_cmd( \%cmd );
	%cmd = ();
    }

    # Deal with the lines that must be placed before a command block
    if ( $cmd eq '$' ) {    # Command
        if ( $arg =~ /^mkfile / ) {    # "mkfile" command line
            die "[TESH/CRITICAL] Output expected from mkfile command!\n"
              if scalar @{ cmd { 'out' } };

            $cmd{'arg'} = $arg;
            $cmd{'arg'} =~ s/mkfile //;
            mkfile_cmd( \%cmd );
            %cmd = ();

        } elsif ( $arg =~ /^\s*cd / ) {
            die "[TESH/CRITICAL] Input provided to cd command!\n"
              if scalar @{ cmd { 'in' } };
            die "[TESH/CRITICAL] Output expected from cd command!\n"
              if scalar @{ cmd { 'out' } };

            $arg =~ s/^ *cd //;
            cd_cmd($arg);
            %cmd = ();

        } else {    # regular command
            $cmd{'cmd'}  = $arg;
            $cmd{'line'} = $line_num;
        }

    } elsif ( $cmd eq '&' ) {    # background command line
	die "[TESH/CRITICAL] mkfile cannot be run in background\n"
	    if ($arg =~ /^mkfile/);
	die "[TESH/CRITICAL] cd cannot be run in background\n"
	    if ($arg =~ /^cd/);
	
        $cmd{'background'} = 1;
        $cmd{'cmd'}        = $arg;
        $cmd{'line'}       = $line_num;

    # Deal with the meta-commands
    } elsif ( $line =~ /^! (.*)/) {
	$line = $1;

	if ( $line =~ /^output sort/ ) {
	    $cmd{'sort'} = 1;
	    if ( $line =~ /^output sort\s+(\d+)/ ) {
		$sort_prefix = $1;
	    }
	} elsif ($line =~ /^output ignore/ ) {
	    $cmd{'output ignore'} = 1;
	} elsif ( $line =~ /^output display/ ) {
	    $cmd{'output display'} = 1;
	} elsif ( $line =~ /^expect signal (\w*)/ ) {
	    $cmd{'expect'} = $1;
	} elsif ( $line =~ /^expect return/ ) {
	    $line =~ s/^expect return //g;
	    $line =~ s/\r//g;
	    $cmd{'return'} = $line;
	} elsif ( $line =~ /^setenv/ ) {
	    $line =~ s/^setenv //g;
	    $line =~ s/\r//g;
	    setenv_cmd($line);
	} elsif ( $line =~ /^timeout/ ) {
	    $line =~ s/^timeout //;
	    $line =~ s/\r//g;
	    $cmd{'timeout'} = $line;
	}
    } else {
        die "[TESH/CRITICAL] parse error: $line\n";
    }
}

# We are done reading the input file
close $infh unless ( $tesh_name eq "(stdin)" );

# Deal with last command, if any
if ( defined( $cmd{'cmd'} ) ) {
    exec_cmd( \%cmd );
    %cmd = ();
}

foreach (@bg_cmds) {
    my %test = %{$_};
    analyze_result( \%test );
}

if ($diff_tool) {
    close $diff_tool_tmp_fh;
    system("$diff_tool $diff_tool_tmp_filename $tesh_file");
    unlink $diff_tool_tmp_filename;
}

if ( $error != 0 ) {
    exit $exitcode;
} elsif ( $tesh_name eq "(stdin)" ) {
    print "Test suite from stdin OK\n";
} else {
    print "Test suite `$tesh_name' OK\n";
}

exit 0;

####
#### Helper functions
####

sub build_diff {
    my $res;
    my $diff = Diff->new(@_);

    $diff->Base(1);    # Return line numbers, not indices
    my $chunk_count = $diff->Next(-1);    # Compute the amount of chuncks
    return "" if ( $chunk_count == 1 && $diff->Same() );
    $diff->Reset();
    while ( $diff->Next() ) {
        my @same = $diff->Same();
        if ( $diff->Same() ) {
            if ( $diff->Next(0) > 1 ) {    # not first chunk: print 2 first lines
                $res .= '  ' . $same[0] . "\n";
                $res .= '  ' . $same[1] . "\n" if ( scalar @same > 1 );
            }
            $res .= "...\n" if ( scalar @same > 2 );

            #    $res .= $diff->Next(0)."/$chunk_count\n";
            if ( $diff->Next(0) < $chunk_count ) {    # not last chunk: print 2 last lines
                $res .= '  ' . $same[ scalar @same - 2 ] . "\n"
                  if ( scalar @same > 1 );
                $res .= '  ' . $same[ scalar @same - 1 ] . "\n";
            }
        }
        next if $diff->Same();
        map { $res .= "- $_\n" } $diff->Items(1);
        map { $res .= "+ $_\n" } $diff->Items(2);
    }
    return $res;
}

# Helper function replacing any occurence of variable '$name' by its '$value'
# As in Bash, ${$value:=BLABLA} is rewritten to $value if set or to BLABLA if $value is not set
sub var_subst {
    my ( $text, $name, $value ) = @_;
    if ($value) {
        $text =~ s/\${$name(?::[=-][^}]*)?}/$value/g;
        $text =~ s/\$$name(\W|$)/$value$1/g;
    } else {
        $text =~ s/\${$name:=([^}]*)}/$1/g;
        $text =~ s/\${$name}//g;
        $text =~ s/\$$name(\W|$)/$1/g;
    }
    return $text;
}

################################  The possible commands  ################################

sub mkfile_cmd($) {
    my %cmd  = %{ $_[0] };
    my $file = $cmd{'arg'};
    print STDERR "[Tesh/INFO] mkfile $file. Ctn: >>".join( '\n', @{ $cmd{'in'} })."<<\n"
      if $opts{'debug'};

    unlink($file);
    open( FILE, ">$file" )
      or die "[Tesh/CRITICAL] Unable to create file $file: $!\n";
    print FILE join( "\n", @{ $cmd{'in'} } );
    print FILE "\n" if ( scalar @{ $cmd{'in'} } > 0 );
    close(FILE);
}

# Command CD. Just change to the provided directory
sub cd_cmd($) {
    my $directory = shift;
    my $failure   = 1;
    if ( -e $directory && -d $directory ) {
        chdir("$directory");
        print "[Tesh/INFO] change directory to $directory\n";
        $failure = 0;
    } elsif ( -e $directory ) {
        print "Cannot change directory to '$directory': it is not a directory\n";
    } else {
        print "Chdir to $directory failed: No such file or directory\n";
    }
    if ( $failure == 1 ) {
        print "Test suite `$tesh_name': NOK (system error)\n";
        exit 4;
    }
}

# Command setenv. Gets "variable=content", and update the environment accordingly
sub setenv_cmd($) {
    my $arg = shift;
    if ( $arg =~ /^(.*)=(.*)$/ ) {
        my ( $var, $ctn ) = ( $1, $2 );
        print "[Tesh/INFO] setenv $var=$ctn\n";
        $environ{$var} = $ctn;
	$ENV{$var} = $ctn;
    } else {
        die "[Tesh/CRITICAL] Malformed argument to setenv: expected 'name=value' but got '$arg'\n";
    }
}
