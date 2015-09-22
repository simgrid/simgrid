#! /usr/bin/env perl

# Copyright (c) 2012-2014. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.
eval 'exec perl -S $0 ${1+"$@"}'
  if $running_under_some_shell;

=encoding UTF-8

=head1 NAME

tesh -- testing shell

=head1 SYNOPSIS

B<tesh> [I<options>] I<tesh_file>

=cut
my($bindir)=".";
my($srcdir)=".";
my($timeout)=0;
my($time_to_wait)=0;
my $path = $0;
my $enable_coverage=0;
my $diff_tool=0;
my $diff_tool_tmp_fh=0;
my $diff_tool_tmp_filename=0;
my $sort_prefix = -1;
my $tesh_file;
my $tesh_name;
my $error=0;
my $exitcode=0;
my @bg_cmds;
my (%environ);
$SIG{'PIPE'} = 'IGNORE';
$path =~ s|[^/]*$||;
push @INC,$path;

use Getopt::Long qw(GetOptions);
use strict;
use Term::ANSIColor;
use Text::ParseWords;
use IPC::Open3;
use IO::File;
use English;

## 
## Portability bits for windows
##

use constant RUNNING_ON_WINDOWS => ($^O =~ /^(?:mswin|dos|os2)/oi);
use POSIX qw(:sys_wait_h WIFEXITED WIFSIGNALED WIFSTOPPED WEXITSTATUS WTERMSIG WSTOPSIG
             :signal_h SIGINT SIGTERM SIGKILL SIGABRT SIGSEGV);
# These are not implemented on windows (see bug 6798 and 6470)
BEGIN {
    if (RUNNING_ON_WINDOWS) {
	*WIFEXITED   = sub { not $_[0] & 127 };
	*WEXITSTATUS = sub { $_[0] >> 8 };
	*WIFSIGNALED = sub { ($_[0] & 127) && ($_[0] & 127 != 127) };
	*WTERMSIG    = sub { $_[0] & 127 };
    }
}


##
## Command line option handling
##

if ($ARGV[0] eq "--internal-killer-process") {
    # We fork+exec a waiter process in charge of killing the command after timeout
    # If the command stops earlier, that's fine: the killer sends a signal to an already stopped process, fails, and quits. 
    #    Nobody cares about the killer issues. 
    #    The only problem could arise if another process is given the same PID than cmd. We bet it won't happen :)
    my $time_to_wait = $ARGV[1];
    my $pid = $ARGV[2];
    sleep $time_to_wait;
    kill('TERM', $pid);
    sleep 1;
    kill('KILL', $pid);
    exit $time_to_wait;
}


sub var_subst {
    my ($text, $name, $value) = @_;
    if ($value) {
        $text =~ s/\${$name(?::[=-][^}]*)?}/$value/g;
        $text =~ s/\$$name(\W|$)/$value$1/g;
    }
    else {
        $text =~ s/\${$name:=([^}]*)}/$1/g;
        $text =~ s/\${$name}//g;
        $text =~ s/\$$name(\W|$)/$1/g;
    }
    return $text;
}

# option handling helper subs
sub cd_cmd {
    my $directory=$_[1];
    my $failure=1;
    if (-e $directory && -d $directory) {
	chdir("$directory");
	print "[Tesh/INFO] change directory to $directory\n";
	$failure=0;
    } elsif (-e $directory) {
	print "Cannot change directory to '$directory': it is not a directory\n";
    } else {
	print "Chdir to $directory failed: No such file or directory\n";
    }
    if($failure==1){
	$error=1;
	$exitcode=4;
	print "Test suite `$tesh_file': NOK (system error)\n";
	exit 4;
    }
}

sub setenv_cmd {
  my($var,$ctn);
  if ($_[0] =~ /^(.*)=(.*)$/) {
    ($var,$ctn)=($1,$2);
  }elsif ($_[1] =~ /^(.*)=(.*)$/) {
    ($var,$ctn)=($1,$2);
  } else {
      die "[Tesh/CRITICAL] Malformed argument to setenv: expected 'name=value' but got '$_[1]'\n";
  }

  print "[Tesh/INFO] setenv $var=$ctn\n";
  $environ{$var} = $ctn;
}

# Main option parsing sub

sub get_options {
  # remove the tesh file from the ARGV used
  my @ARGV = @_;
  $tesh_file = pop @ARGV;

  # temporary arrays for GetOption
  my @verbose = ();
  my @cfg;
  my $log; # ignored


  my %opt = (
    "help"  => 0,
    "debug"   => 0,
    "verbose" => 0
    );

  Getopt::Long::config('bundling', 'no_getopt_compat', 'no_auto_abbrev');

  GetOptions(
    'help|h'   => \$opt{'help'},

    'verbose|v'  => \@verbose,
    'debug|d'  => \$opt{"debug"},

    'difftool=s' => \$diff_tool,

    'cd=s'     => \&cd_cmd,
    'timeout=s'  => \$opt{'timeout'},
    'setenv=s'   => \&setenv_cmd,
    'cfg=s'    => \@cfg,
    'log=s'    => \$log,
    'enable-coverage+'  => \$enable_coverage,
    );

  if($enable_coverage){
    print "Enable coverage\n";
  }

  if($diff_tool){
    use File::Temp qw/ tempfile /;
    ($diff_tool_tmp_fh, $diff_tool_tmp_filename) = tempfile();
    print "New tesh: $diff_tool_tmp_filename\n";
  }

  unless($tesh_file=~/(.*)\.tesh/){
    $tesh_file="(stdin)";
    $tesh_name="(stdin)";
    print "Test suite from stdin\n";
  }else{
    $tesh_name=$1;
    print "Test suite `$tesh_name'\n";
  }

  $opt{'verbose'} = scalar @verbose;
  foreach (@cfg) {
    $opt{'cfg'} .= " --cfg=$_";
  }
  return %opt;
}

my %opts = get_options(@ARGV);

##
## File parsing
##
my($nb_arg)=0;
my($old_buffer);
my($linebis);
my($SIGABRT)=0;
my($verbose)=0;
my($return)=-1;
my($pid);
my($result);
my($result_err);
my($forked);
my($config)="";
my($tesh_command)=0;
my(@buffer_tesh)=();

###########################################################################

sub exit_status {
    my $status = shift;
    if (WIFEXITED($status)) {
	$exitcode=WEXITSTATUS($status)+40;
	return "returned code ".WEXITSTATUS($status);
    } elsif (WIFSIGNALED($status)) {
	my $code;
	if (WTERMSIG($status) == SIGINT){$code="SIGINT"; }
	elsif  (WTERMSIG($status) == SIGTERM) {$code="SIGTERM"; }
	elsif  (WTERMSIG($status) == SIGKILL) {$code= "SIGKILL"; }
	elsif  (WTERMSIG($status) == SIGABRT) {$code="SIGABRT"; }
	elsif  (WTERMSIG($status) == SIGSEGV) {$code="SIGSEGV" ;}
	$exitcode=WTERMSIG($status)+4;
	return "got signal $code";
    }
    return "Unparsable status. Is the process stopped?";
}

sub exec_cmd {
  my %cmd = %{$_[0]};
  if ($opts{'debug'}) {
    print "IN BEGIN\n";
    map {print "  $_"} @{$cmd{'in'}};
    print "IN END\n";
    print "OUT BEGIN\n";
    map {print "  $_"} @{$cmd{'out'}};
    print "OUT END\n";
    print "CMD: $cmd{'cmd'}\n";
  }

  # cleanup the command line
  if(RUNNING_ON_WINDOWS) {
      var_subst($cmd{'cmd'}, "EXEEXT", ".exe");
  } else {
      var_subst($cmd{'cmd'}, "EXEEXT", "");
  }

  # substitute environ variables
  foreach my $key (keys %environ) {
      $cmd{'cmd'} = var_subst($cmd{'cmd'}, $key, $environ{$key});
  }
  # substitute remaining variables, if any
  while ($cmd{'cmd'} =~ /\${(\w+)(?::[=-][^}]*)?}/) {
      $cmd{'cmd'} = var_subst($cmd{'cmd'}, $1, "");
  }
  while ($cmd{'cmd'} =~ /\$(\w+)/) {
      $cmd{'cmd'} = var_subst($cmd{'cmd'}, $1, "");
  }

  # add cfg options
  $cmd{'cmd'} .= " $opts{'cfg'}" if (defined($opts{'cfg'}) && length($opts{'cfg'}));

  # final cleanup
  $cmd{'cmd'} =~ s/^\s+//;
  $cmd{'cmd'} =~ s/\s+$//;

  print "[$tesh_name:$cmd{'line'}] $cmd{'cmd'}\n" ;

  ###
  # exec the command line
  ###  $line =~ s/\r//g;

  $cmd{'got'} = IO::File->new_tmpfile;
  $cmd{'got'}->autoflush(1);
  local *E = $cmd{'got'};
  $cmd{'pid'} = open3(\*CHILD_IN,  ">&E",  ">&E",
                      quotewords('\s+', 0, $cmd{'cmd'}));

  # push all provided input to executing child
  map { print CHILD_IN "$_\n"; }  @{$cmd{'in'}};
  close CHILD_IN;

  # if timeout specified, fork and kill executing child at the end of timeout
  if (defined($cmd{'timeout'}) or defined($opts{'timeout'})){
    $time_to_wait= defined($cmd{'timeout'}) ? $cmd{'timeout'} : $opts{'timeout'};
    $forked = fork();
    $timeout=-1;
    die "fork() failed: $!" unless defined $forked;
    if ( $forked == 0 ) { # child
	exec("$PROGRAM_NAME --internal-killer-process $time_to_wait $cmd{'pid'}");
    }
  }

  # Cleanup the executing child, and kill the timeouter brother on need
  $cmd{'return'} = 0 unless defined($cmd{'return'});
  if($cmd{'background'} != 1){
    waitpid ($cmd{'pid'}, 0);
    $cmd{'gotret'} = exit_status($?);
    parse_out(\%cmd);
  }else{
    # & commands, which will be handled at the end
    push @bg_cmds, \%cmd;
    # no timeout for background commands
    if($forked){
       kill(SIGKILL, $forked);
       $timeout=0;
       $forked=0;
    }
  }
}


sub parse_out {
  my %cmd = %{$_[0]};
  my $gotret=$cmd{'gotret'};

  my $wantret;

  if(defined($cmd{'expect'}) and ($cmd{'expect'} ne "")){
    $wantret = "got signal $cmd{'expect'}";
  }else{
    $wantret = "returned code ".(defined($cmd{'return'})? $cmd{'return'} : 0);
  }

  local *got = $cmd{'got'};
  seek(got,0,0);
  # pop all output from executing child
  my @got;
  while(defined(my $got=<got>)) {
    $got =~ s/\r//g;
    chomp $got;
    print $diff_tool_tmp_fh "> $got\n" if ($diff_tool);

    if (!($enable_coverage and $got=~ /^profiling:/)){
      push @got, $got;
    }
  }

  if ($cmd{'sort'}){
    # Save the unsorted observed output to report it on error.
    map { push @{$cmd{'unsorted got'}}, $_ } @got;

    sub mysort{
        substr($a, 0, $sort_prefix) cmp substr($b, 0, $sort_prefix)
    }
    use sort 'stable';
    if ($sort_prefix>0) {
	@got = sort mysort @got;
    } else {
	@got = sort @got;
    }	    
    while (@got and $got[0] eq "") {
      shift @got;
    }

    # Sort the expected output to make it easier to write for humans
    if(defined($cmd{'out'})){
      if ($sort_prefix>0) {
	  @{$cmd{'out'}} = sort mysort @{$cmd{'out'}};
      } else {
	  @{$cmd{'out'}} = sort @{$cmd{'out'}};
      }
      while (@{$cmd{'out'}} and ${$cmd{'out'}}[0] eq "") {
        shift @{$cmd{'out'}};
      }
    }
  }

  # Did we timeout ? If yes, handle it. If not, kill the forked process.

  if($timeout==-1 and ($gotret eq "got signal SIGTERM" or $gotret eq "got signal SIGKILL")){
    $gotret="return code 0";
    $timeout=1;
    $gotret= "timeout after $time_to_wait sec";
    $error=1;
    $exitcode=3;
    print STDERR "<$cmd{'file'}:$cmd{'line'}> timeouted. Kill the process.\n";
  }else{
    $timeout=0;
  }
  if($gotret ne $wantret) {
    $error=1;
    my $msg = "Test suite `$cmd{'file'}': NOK (<$cmd{'file'}:$cmd{'line'}> $gotret)\n";
    if ($timeout!=1) {
        $msg=$msg."Output of <$cmd{'file'}:$cmd{'line'}> so far:\n";
    }
    map {$msg .=  "|| $_\n"} @got;
    if(!@got) {
        if($timeout==1){
        print STDERR "<$cmd{'file'}:$cmd{'line'}> No output before timeout\n";
        }else{
        $msg .= "||\n";
        }
    }
    $timeout = 0;
    print STDERR "$msg";
  }


  ###
  # Check the result of execution
  ###
  my $diff;
  if (defined($cmd{'output display'})){
    print "[Tesh/INFO] Here is the (ignored) command output:\n";
    map { print "||$_\n" } @got;
  }
  elsif (!defined($cmd{'output ignore'})){
    $diff = build_diff(\@{$cmd{'out'}}, \@got);
  }else{
    print "(ignoring the output of <$cmd{'file'}:$cmd{'line'}> as requested)\n"
  }
  if (length $diff) {
    print "Output of <$cmd{'file'}:$cmd{'line'}> mismatch".($cmd{'sort'}?" (even after sorting)":"").":\n";
    map { print "$_\n" } split(/\n/,$diff);
    if ($cmd{'sort'}) {
	print "WARNING: Both the observed output and expected output were sorted as requested.\n";
	print "WARNING: Output were only sorted using the $sort_prefix first chars.\n"
	  if ($sort_prefix>0);
	print "WARNING: Use <! output sort 19> to sort by simulated date and process ID only.\n";
	# print "----8<---------------  Begin of unprocessed observed output (as it should appear in file):\n";
	# map {print "> $_\n"} @{$cmd{'unsorted got'}};
	# print "--------------->8----  End of the unprocessed observed output.\n";
    }

    print "Test suite `$cmd{'file'}': NOK (<$cmd{'file'}:$cmd{'line'}> output mismatch)\n";
    $error=1;
    $exitcode=2;
  }
}

sub mkfile_cmd {
  my %cmd = %{$_[0]};
  my $file = $cmd{'arg'};
  print "[Tesh/INFO] mkfile $file\n";

  unlink($file);
  open(FILE,">$file") or die "[Tesh/CRITICAL] Unable to create file $file: $!\n";
  print FILE join("\n", @{$cmd{'in'}});
  print FILE "\n" if (scalar @{$cmd{'in'}} > 0);
  close(FILE);
}

# parse tesh file
#my $teshfile=$tesh_file;
#$teshfile=~ s{\.[^.]+$}{};

unless($tesh_file eq "(stdin)"){
  open TESH_FILE, $tesh_file or die "[Tesh/CRITICAL] Unable to open $tesh_file $!\n";
}

my %cmd; # everything about the next command to run
my $line_num=0;
my $finished =0;
LINE: while (not $finished and not $error) {
  my $line;


  if ($tesh_file ne "(stdin)" and !defined($line=<TESH_FILE>)){
    $finished=1;
    next LINE;
  }elsif ($tesh_file eq "(stdin)" and !defined($line=<>)){
    $finished=1;
    next LINE;
  }

  $line_num++;
  chomp $line;
  $line =~ s/\r//g;
  print "[TESH/debug] $line_num: $line\n" if $opts{'debug'};
  my $next;
  # deal with line continuations
  while ($line =~ /^(.*?)\\$/) {
    $next=<TESH_FILE>;
    die "[TESH/CRITICAL] Continued line at end of file\n"
      unless defined($next);
    $line_num++;
    chomp $next;
    print "[TESH/debug] $line_num: $next\n" if $opts{'debug'};
    $line = $1.$next;
  }

  # Push delayed commands on empty lines
  unless ($line =~ m/^(.)(.*)$/) {
    if (defined($cmd{'cmd'}))  {
      exec_cmd(\%cmd);
      %cmd = ();
    }
    print $diff_tool_tmp_fh "$line\n" if ($diff_tool);
    next LINE;
  }

  my ($cmd,$arg) = ($1,$2);
  print $diff_tool_tmp_fh "$line\n" if ($diff_tool and $cmd ne '>');
  $arg =~ s/^ //g;
  $arg =~ s/\r//g;
  $arg =~ s/\\\\/\\/g;
  # handle the commands
  if ($cmd =~ /^#/) {    #comment
  } elsif ($cmd eq '>'){    #expected result line
    print "[TESH/debug] push expected result\n" if $opts{'debug'};
    push @{$cmd{'out'}}, $arg;

  } elsif ($cmd eq '<') {    # provided input
    print "[TESH/debug] push provided input\n" if $opts{'debug'};
    push @{$cmd{'in'}}, $arg;

  } elsif ($cmd eq 'p') {    # comment
    print "[$tesh_name:$line_num] $arg\n";

  } elsif ($cmd eq '$') {  # Command
    # if we have something buffered, run it now
    if (defined($cmd{'cmd'})) {
      exec_cmd(\%cmd);
      %cmd = ();
    }
    if ($arg =~ /^\s*mkfile /){      # "mkfile" command line
      die "[TESH/CRITICAL] Output expected from mkfile command!\n" if scalar @{cmd{'out'}};

      $cmd{'arg'} = $arg;
      $cmd{'arg'} =~ s/\s*mkfile //;
      mkfile_cmd(\%cmd);
      %cmd = ();

    } elsif ($arg =~ /^\s*cd /) {
      die "[TESH/CRITICAL] Input provided to cd command!\n" if scalar @{cmd{'in'}};
      die "[TESH/CRITICAL] Output expected from cd command!\n" if scalar @{cmd{'out'}};

      $arg =~ s/^ *cd //;
      cd_cmd("",$arg);
      %cmd = ();

    } else { # regular command
      $cmd{'cmd'} = $arg;
      $cmd{'file'} = $tesh_file;
      $cmd{'line'} = $line_num;
    }
  }
  elsif($cmd eq '&'){      # parallel command line

    if (defined($cmd{'cmd'})) {
      exec_cmd(\%cmd);
      %cmd = ();
    }
    $cmd{'background'} = 1;
    $cmd{'cmd'} = $arg;
    $cmd{'file'} = $tesh_file;
    $cmd{'line'} = $line_num;
  }
  elsif($line =~ /^!\s*output sort/){    #output sort
    if (defined($cmd{'cmd'})) {
      exec_cmd(\%cmd);
      %cmd = ();
    }
    $cmd{'sort'} = 1;
    if ($line =~ /^!\s*output sort\s+(\d+)/) {
        $sort_prefix = $1;
    }
  }
  elsif($line =~ /^!\s*output ignore/){    #output ignore
    if (defined($cmd{'cmd'})) {
      exec_cmd(\%cmd);
      %cmd = ();
    }
    $cmd{'output ignore'} = 1;
  }
  elsif($line =~ /^!\s*output display/){    #output display
    if (defined($cmd{'cmd'})) {
      exec_cmd(\%cmd);
      %cmd = ();
    }
    $cmd{'output display'} = 1;
  }
  elsif($line =~ /^!\s*expect signal (\w*)/) {#expect signal SIGABRT
    if (defined($cmd{'cmd'})) {
      exec_cmd(\%cmd);
      %cmd = ();
    }
print "hey\n";
    $cmd{'expect'} = "$1";
  }
  elsif($line =~ /^!\s*expect return/){    #expect return
    if (defined($cmd{'cmd'})) {
      exec_cmd(\%cmd);
      %cmd = ();
    }
    $line =~ s/^! expect return //g;
    $line =~ s/\r//g;
    $cmd{'return'} = $line;
  }
  elsif($line =~ /^!\s*setenv/){    #setenv
    if (defined($cmd{'cmd'})) {
      exec_cmd(\%cmd);
      %cmd = ();
    }
    $line =~ s/^! setenv //g;
    $line =~ s/\r//g;
    setenv_cmd($line);
  }
  elsif($line =~ /^!\s*include/){    #include
    if (defined($cmd{'cmd'})) {
      exec_cmd(\%cmd);
      %cmd = ();
    }
    print color("red"), "[Tesh/CRITICAL] need include";
    print color("reset"), "\n";
    die;
  }
  elsif($line =~ /^!\s*timeout/){    #timeout
    if (defined($cmd{'cmd'})) {
      exec_cmd(\%cmd);
      %cmd = ();
    }
    $line =~ s/^! timeout //;
    $line =~ s/\r//g;
    $cmd{'timeout'} = $line;
  } else {
    die "[TESH/CRITICAL] parse error: $line\n";
  }
  if($forked){
   kill(SIGKILL, $forked);
   $timeout=0;
  }

}



# Deal with last command
if (defined($cmd{'cmd'})) {
  exec_cmd(\%cmd);
  %cmd = ();
}


if($forked){
   kill(SIGKILL, $forked);
   $timeout=0;
}

foreach(@bg_cmds){
  my %test=%{$_};
  waitpid ($test{'pid'}, 0);
  $test{'gotret'} = exit_status($?);
  parse_out(\%test);
}

@bg_cmds=();

if ($diff_tool) {
  close $diff_tool_tmp_fh;
  system("$diff_tool $diff_tool_tmp_filename $tesh_file");
  unlink $diff_tool_tmp_filename;
}

if($error !=0){
    exit $exitcode;
}elsif($tesh_file eq "(stdin)"){
    print "Test suite from stdin OK\n";
}else{
    print "Test suite `$tesh_name' OK\n";
}

#my (@a,@b);
#push @a,"bl1";   push @b,"bl1";
#push @a,"bl2";   push @b,"bl2";
#push @a,"bl3";   push @b,"bl3";
#push @a,"bl4";   push @b,"bl4";
#push @a,"bl5";   push @b,"bl5";
#push @a,"bl6";   push @b,"bl6";
#push @a,"bl7";   push @b,"bl7";
##push @a,"Perl";  push @b,"ruby";
#push @a,"END1";   push @b,"END1";
#push @a,"END2";   push @b,"END2";
#push @a,"END3";   push @b,"END3";
#push @a,"END4";   push @b,"END4";
#push @a,"END5";   push @b,"END5";
#push @a,"END6";   push @b,"END6";
#push @a,"END7";   push @b,"END7";
#print "Identical:\n". build_diff(\@a,\@b);

#@a = (); @b =();
#push @a,"AZE"; push @b,"EZA";
#print "Different:\n".build_diff(\@a,\@b);

use lib "@CMAKE_BINARY_DIR@/bin" ;

use Diff qw(diff); # postpone a bit to have time to change INC

sub build_diff {
  my $res;
  my $diff = Diff->new(@_);

  $diff->Base( 1 );   # Return line numbers, not indices
  my $chunk_count = $diff->Next(-1); # Compute the amount of chuncks
  return ""   if ($chunk_count == 1 && $diff->Same());
  $diff->Reset();
  while(  $diff->Next()  ) {
    my @same = $diff->Same();
    if ($diff->Same() ) {
      if ($diff->Next(0) > 1) { # not first chunk: print 2 first lines
        $res .= '  '.$same[0]."\n" ;
        $res .= '  '.$same[1]."\n" if (scalar @same>1);
      }
      $res .= "...\n"  if (scalar @same>2);
#    $res .= $diff->Next(0)."/$chunk_count\n";
      if ($diff->Next(0) < $chunk_count) { # not last chunk: print 2 last lines
        $res .= '  '.$same[scalar @same -2]."\n" if (scalar @same>1);
        $res .= '  '.$same[scalar @same -1]."\n";
      }
    }
    next if  $diff->Same();
    map { $res .= "- $_\n" } $diff->Items(1);
    map { $res .= "+ $_\n" } $diff->Items(2);
  }
  return $res;
}


