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

B<tesh> [I<options>] I<tesh_file>

=cut

BEGIN {
    # Disabling IPC::Run::Debug saves tons of useless calls.
    $ENV{'IPCRUNDEBUG'} = 'none'
      unless exists $ENV{'IPCRUNDEBUG'};
}

my ($timeout)              = 0;
my ($time_to_wait)         = 0;
my $path                   = $0;
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
    }
}


####
#### Command line option handling
####

if ( $ARGV[0] eq "--internal-killer-process" ) {

    # We fork+exec a waiter process in charge of killing the command after timeout
    # If the command stops earlier, that's fine: the killer sends a signal to an already stopped process, fails, and quits.
    #    Nobody cares about the killer issues.
    #    The only problem could arise if another process is given the same PID than cmd. We bet it won't happen :)
    my $time_to_wait = $ARGV[1];
    my $pid          = $ARGV[2];
    sleep $time_to_wait;
    kill( 'TERM', $pid );
    sleep 1;
    kill( 'KILL', $pid );
    exit $time_to_wait;
}

my %opts = ( "debug" => 0,
             "timeout" => 120, # No command should run any longer than 2 minutes by default
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

if ($enable_coverage) {
    print "Enable coverage\n";
}

if ($diff_tool) {
    use File::Temp qw/ tempfile /;
    ( $diff_tool_tmp_fh, $diff_tool_tmp_filename ) = tempfile();
    print "New tesh: $diff_tool_tmp_filename\n";
}

if ( $tesh_file =~ m/(.*)\.tesh/ ) {
    $tesh_name = $1;
    print "Test suite `$tesh_name'\n";
} else {
    $tesh_file = "(stdin)";
    $tesh_name = "(stdin)";
    print "Test suite from stdin\n";
}

##
## File parsing
##
my ($return) = -1;
my ($forked);
my ($config)      = "";
my (@buffer_tesh) = ();

###########################################################################

sub exit_status {
    my $status = shift;
    if ( WIFEXITED($status) ) {
        $exitcode = WEXITSTATUS($status) + 40;
        return "returned code " . WEXITSTATUS($status);
    } elsif ( WIFSIGNALED($status) ) {
        my $code;
        if    ( WTERMSIG($status) == SIGINT )  { $code = "SIGINT"; }
        elsif ( WTERMSIG($status) == SIGTERM ) { $code = "SIGTERM"; }
        elsif ( WTERMSIG($status) == SIGKILL ) { $code = "SIGKILL"; }
        elsif ( WTERMSIG($status) == SIGABRT ) { $code = "SIGABRT"; }
        elsif ( WTERMSIG($status) == SIGSEGV ) { $code = "SIGSEGV"; }
        $exitcode = WTERMSIG($status) + 4;
        return "got signal $code";
    }
    return "Unparsable status. Is the process stopped?";
}

sub exec_cmd {
    my %cmd = %{ $_[0] };
    if ( $opts{'debug'} ) {
        map { print "IN: $_\n" } @{ $cmd{'in'} };
        map { print "OUT: $_\n" } @{ $cmd{'out'} };
        print "CMD: $cmd{'cmd'}\n";
    }

    # cleanup the command line
    if (RUNNING_ON_WINDOWS) {
        var_subst( $cmd{'cmd'}, "EXEEXT", ".exe" );
    } else {
        var_subst( $cmd{'cmd'}, "EXEEXT", "" );
    }

    # substitute environ variables
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
    $cmd{'job'} = start \@cmdline, '<', \$input, '>&', \$output, timeout($cmd{'timeout'});

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
    
    eval {
	finish( $cmd{'job'} );
    };
    if ($@) {
	if ($@ =~ /timeout/) {
	    $cmd{'job'}->kill_kill;
	    $cmd{'timeouted'} = 1;
	} elsif ($@ =~ /^ack / and $@ =~ /pipe/) {
	    print STDERR "Tesh: Broken pipe (ignored).\n";
	} else {
	    die $@; # Don't know what it is, so let it go.
	}
    } 
    $cmd{'timeouted'} ||= 0;
    
    my $gotret = $cmd{'gotret'} = exit_status($?); 

    my $wantret;

    if ( defined( $cmd{'expect'} ) and ( $cmd{'expect'} ne "" ) ) {
        $wantret = "got signal $cmd{'expect'}";
    } else {
        $wantret = "returned code " . ( defined( $cmd{'return'} ) ? $cmd{'return'} : 0 );
    }

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

    if ( $cmd{'sort'} ) {

        # Save the unsorted observed output to report it on error.
        map { push @{ $cmd{'unsorted got'} }, $_ } @got;

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

        # Sort the expected output to make it easier to write for humans
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

    # Did we timeout ? If yes, handle it. If not, kill the forked process.

    if ( $cmd{'timeouted'} ) {
        $gotret   = "timeout after $cmd{'timeout'} sec";
        $error    = 1;
        $exitcode = 3;
        print STDERR "<$cmd{'file'}:$cmd{'line'}> timeouted. Kill the process.\n";
    }
    if ( $gotret ne $wantret ) {
        $error = 1;
        my $msg = "Test suite `$cmd{'file'}': NOK (<$cmd{'file'}:$cmd{'line'}> $gotret)\n";
        if ( scalar @got ) {
            $msg = $msg . "Output of <$cmd{'file'}:$cmd{'line'}> so far:\n";
	    map { $msg .= "|| $_\n" } @got;
        } else {
	    $msg .= "<$cmd{'file'}:$cmd{'line'}> No output so far.\n";
	}
        print STDERR "$msg";
    }

    ###
    # Check the result of execution
    ###
    my $diff;
    if ( defined( $cmd{'output display'} ) ) {
        print "[Tesh/INFO] Here is the (ignored) command output:\n";
        map { print "||$_\n" } @got;
    } elsif ( defined( $cmd{'output ignore'} ) ) {
        print "(ignoring the output of <$cmd{'file'}:$cmd{'line'}> as requested)\n";
    } else {
        $diff = build_diff( \@{ $cmd{'out'} }, \@got );
    }
    if ( length $diff ) {
        print "Output of <$cmd{'file'}:$cmd{'line'}> mismatch" . ( $cmd{'sort'} ? " (even after sorting)" : "" ) . ":\n";
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

        print "Test suite `$cmd{'file'}': NOK (<$cmd{'file'}:$cmd{'line'}> output mismatch)\n";
        exit 2;
    }
}

# parse tesh file
my $infh;    # The file descriptor from which we should read the teshfile
if ( $tesh_file eq "(stdin)" ) {
    $infh = *STDIN;
} else {
    open $infh, $tesh_file
      or die "[Tesh/CRITICAL] Unable to open $tesh_file: $!\n";
}

my %cmd;     # everything about the next command to run
my $line_num = 0;
LINE: while ( defined( my $line = <$infh> ) and not $error ) {
    chomp $line;
    $line =~ s/\r//g;

    $line_num++;
    print "[TESH/debug] $line_num: $line\n" if $opts{'debug'};
    my $next;

    # deal with line continuations
    while ( $line =~ /^(.*?)\\$/ ) {
        $next = <$infh>;
        die "[TESH/CRITICAL] Continued line at end of file\n"
          unless defined($next);
        $line_num++;
        chomp $next;
        print "[TESH/debug] $line_num: $next\n" if $opts{'debug'};
        $line = $1 . $next;
    }

    # Push delayed commands on empty lines
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

    # handle the commands
    if ( $cmd =~ /^#/ ) {    # comment
    } elsif ( $cmd eq '>' ) {    # expected result line
        print "[TESH/debug] push expected result\n" if $opts{'debug'};
        push @{ $cmd{'out'} }, $arg;

    } elsif ( $cmd eq '<' ) {    # provided input
        print "[TESH/debug] push provided input\n" if $opts{'debug'};
        push @{ $cmd{'in'} }, $arg;

    } elsif ( $cmd eq 'p' ) {    # comment
        print "[$tesh_name:$line_num] $arg\n";

    } elsif ( $cmd eq '$' ) {    # Command
                                 # if we have something buffered, run it now
        if ( defined( $cmd{'cmd'} ) ) {
            exec_cmd( \%cmd );
            %cmd = ();
        }
        if ( $arg =~ /^\s*mkfile / ) {    # "mkfile" command line
            die "[TESH/CRITICAL] Output expected from mkfile command!\n"
              if scalar @{ cmd { 'out' } };

            $cmd{'arg'} = $arg;
            $cmd{'arg'} =~ s/\s*mkfile //;
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
            $cmd{'file'} = $tesh_file;
            $cmd{'line'} = $line_num;
        }
    } elsif ( $cmd eq '&' ) {    # background command line

        if ( defined( $cmd{'cmd'} ) ) {
            exec_cmd( \%cmd );
            %cmd = ();
        }
        $cmd{'background'} = 1;
        $cmd{'cmd'}        = $arg;
        $cmd{'file'}       = $tesh_file;
        $cmd{'line'}       = $line_num;

    } elsif ( $line =~ /^!\s*output sort/ ) {    #output sort
        if ( defined( $cmd{'cmd'} ) ) {
            exec_cmd( \%cmd );
            %cmd = ();
        }
        $cmd{'sort'} = 1;
        if ( $line =~ /^!\s*output sort\s+(\d+)/ ) {
            $sort_prefix = $1;
        }
    } elsif ( $line =~ /^!\s*output ignore/ ) {    #output ignore
        if ( defined( $cmd{'cmd'} ) ) {
            exec_cmd( \%cmd );
            %cmd = ();
        }
        $cmd{'output ignore'} = 1;
    } elsif ( $line =~ /^!\s*output display/ ) {    #output display
        if ( defined( $cmd{'cmd'} ) ) {
            exec_cmd( \%cmd );
            %cmd = ();
        }
        $cmd{'output display'} = 1;
    } elsif ( $line =~ /^!\s*expect signal (\w*)/ ) {    #expect signal SIGABRT
        if ( defined( $cmd{'cmd'} ) ) {
            exec_cmd( \%cmd );
            %cmd = ();
        }
        print "hey\n";
        $cmd{'expect'} = "$1";
    } elsif ( $line =~ /^!\s*expect return/ ) {          #expect return
        if ( defined( $cmd{'cmd'} ) ) {
            exec_cmd( \%cmd );
            %cmd = ();
        }
        $line =~ s/^! expect return //g;
        $line =~ s/\r//g;
        $cmd{'return'} = $line;
    } elsif ( $line =~ /^!\s*setenv/ ) {                 #setenv
        if ( defined( $cmd{'cmd'} ) ) {
            exec_cmd( \%cmd );
            %cmd = ();
        }
        $line =~ s/^! setenv //g;
        $line =~ s/\r//g;
        setenv_cmd($line);
    } elsif ( $line =~ /^!\s*timeout/ ) {                #timeout
        if ( defined( $cmd{'cmd'} ) ) {
            exec_cmd( \%cmd );
            %cmd = ();
        }
        $line =~ s/^! timeout //;
        $line =~ s/\r//g;
        $cmd{'timeout'} = $line;
    } else {
        die "[TESH/CRITICAL] parse error: $line\n";
    }
    if ($forked) {
        kill( 'KILL', $forked );
        $timeout = 0;
    }
}

# We're done reading the input file
close $infh unless ( $tesh_file eq "(stdin)" );

# Deal with last command
if ( defined( $cmd{'cmd'} ) ) {
    exec_cmd( \%cmd );
    %cmd = ();
}

if ($forked) {
    kill( 'KILL', $forked );
    $timeout = 0;
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
} elsif ( $tesh_file eq "(stdin)" ) {
    print "Test suite from stdin OK\n";
} else {
    print "Test suite `$tesh_name' OK\n";
}

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
        print "Test suite `$tesh_file': NOK (system error)\n";
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
    } else {
        die "[Tesh/CRITICAL] Malformed argument to setenv: expected 'name=value' but got '$arg'\n";
    }
}
