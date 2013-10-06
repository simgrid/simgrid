#! /usr/bin/perl
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
my $OS;
my $enable_coverage=0;
my $tesh_file;
my $tesh_name;
my $error=0;
my $exitcode=0;

$path =~ s|[^/]*$||;
push @INC,$path;

use Getopt::Long qw(GetOptions);
use strict;
use Term::ANSIColor;
use IPC::Open3;

if($^O eq "linux"){
    $OS = "UNIX";
}
else{
    $OS = "WIN";
}


sub trim($)
{
    my $string = shift;
    $string =~ s/^\s+//;
    $string =~ s/\s+$//;
    return $string;
}

# make sure we received a tesh file
scalar @ARGV > 0 || die "Usage:\n  tesh [*options*] *tesh_file*\n";

#Add current directory to path
$ENV{PATH} = "$ENV{PATH}:.";

##
## Command line option handling
##

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
    
    if($var =~ /bindir/){
        print "[Tesh/INFO] setenv $var=$ctn\n";
        $bindir = $ctn;
    }
    else
    {
        if($var =~ /srcdir/){
            $srcdir = $ctn;
        }
        else{
            $ENV{$var} = $ctn;
            print "[Tesh/INFO] setenv $var=$ctn\n";
        }
    }    
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

#eval {
  use POSIX;

  sub exit_status {
    my $status = shift;
    if (POSIX::WIFEXITED($status)) {
      return "returned code ".POSIX::WEXITSTATUS($status);
    } elsif (POSIX::WIFSIGNALED($status)) {
        my $code;
        if (POSIX::WTERMSIG($status) == SIGINT){$code="SIGINT"; }
        elsif  (POSIX::WTERMSIG($status) == SIGTERM) {$code="SIGTERM"; }
        elsif  (POSIX::WTERMSIG($status) == SIGKILL) {$code= "SIGKILL"; }
        elsif  (POSIX::WTERMSIG($status) == SIGABRT) {$code="SIGABRT"; }
        elsif  (POSIX::WTERMSIG($status) == SIGSEGV) {$code="SIGSEGV" ;}
        $exitcode=POSIX::WTERMSIG($status)+4;
        return "got signal $code";
    }
    return "Unparsable status. Is the process stopped?";
  }
#};
#if ($@) { # no POSIX available?
#  warn "POSIX not usable to parse the return value of forked child: $@\n";
#  sub exit_status {
#    return "returned code -1 $@ ";
#  }
#}

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
  if($OS eq "WIN"){
        $cmd{'cmd'} =~ s/\${EXEEXT:=}/.exe/g;
        $cmd{'cmd'} =~ s/\${EXEEXT}/.exe/g;
        $cmd{'cmd'} =~ s/\$EXEEXT/.exe/g;
    }
    else{
        $cmd{'cmd'} =~ s/\${EXEEXT:=}//g;
    }
  $cmd{'cmd'} =~ s/\${bindir:=}/$bindir/g;
  $cmd{'cmd'} =~ s/\${srcdir:=}/$srcdir/g;
  $cmd{'cmd'} =~ s/\${bindir:=.}/$bindir/g;
  $cmd{'cmd'} =~ s/\${srcdir:=.}/$srcdir/g;
  $cmd{'cmd'} =~ s/\${bindir}/$bindir/g;
  $cmd{'cmd'} =~ s/\${srcdir}/$srcdir/g;
# $cmd{'cmd'} =~ s|^\./||g;
#  $cmd{'cmd'} =~ s|tesh|tesh.pl|g;
  $cmd{'cmd'} =~ s/\(%i:%P@%h\)/\\\(%i:%P@%h\\\)/g;
  $cmd{'cmd'} .= " $opts{'cfg'}" if (defined($opts{'cfg'}) && length($opts{'cfg'}));

  print "[$tesh_name:$cmd{'line'}] $cmd{'cmd'}\n" ;

  ###
  # exec the command line
  ###
  $pid = open3(\*CHILD_IN, \*OUT, \*OUT, $cmd{'cmd'} );

  # push all provided input to executing child
  map { print CHILD_IN "$_\n" }  @{$cmd{'in'}};
  close CHILD_IN;

  # if timeout specified, fork and kill executing child at the end of timeout
  if (defined($cmd{'timeout'}) or defined($opts{'timeout'})){
    $time_to_wait= defined($cmd{'timeout'}) ? $cmd{'timeout'} : $opts{'timeout'};
    $forked = fork();
    $timeout=-1;
    die "fork() failed: $!" unless defined $forked;
    if ( $forked == 0 ) { # child
      sleep $time_to_wait;
      kill(9, $pid);
      exit $time_to_wait;
    }
  }


  # pop all output from executing child
  my @got;
  while(defined(my $got=<OUT>)) {
    $got =~ s/\r//g;
    $got =~ s/^( )*//g;
    chomp $got;
    $got=trim($got);
    if( $got ne ""){
        if (!($enable_coverage and $got=~ /^profiling:/)){    
        push @got, "$got";
     }
  }
  }    
  close OUT;
   
  if ($cmd{'sort'}){   
    sub mysort{
    $a cmp $b
    }
    use sort qw(defaults _quicksort); # force quicksort
    @got = sort mysort @got;
    #also resort the other one, as perl sort is not the same as the C one used to generate teshes
    if(defined($cmd{'out'})){
      @{$cmd{'out'}}=sort mysort @{$cmd{'out'}};
    }
  }
  
  # Cleanup the executing child, and kill the timeouter brother on need
  $cmd{'return'} = 0 unless defined($cmd{'return'});
  my $wantret;
  if(defined($cmd{'expect'}) and ($cmd{'expect'} ne "")){
    $wantret = "got signal $cmd{'expect'}";
  }else{
    $wantret = "returned code ".(defined($cmd{'return'})? $cmd{'return'} : 0);
    $exitcode= 41;
  }
  my $gotret;
  waitpid ($pid, 0);
  $gotret = exit_status($?);
  #Did we timeout ? If yes, handle it. If not, kill the forked process.

  if($timeout==-1 and $gotret eq "got signal SIGKILL"){
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
  if (!defined($cmd{'output ignore'})){
    $diff = build_diff(\@{$cmd{'out'}}, \@got);
  }else{
  print "(ignoring the output of <$cmd{'file'}:$cmd{'line'}> as requested)\n"
  }
  if (length $diff) {
    print "Output of <$cmd{'file'}:$cmd{'line'}> mismatch:\n";
    map { print "$_\n" } split(/\n/,$diff);

    print "Test suite `$cmd{'file'}': NOK (<$cmd{'file'}:$cmd{'line'}> output mismatch)\n";
    $error=1;
    $exitcode=2;
  }
}

sub mkfile_cmd {
  my %cmd = %{$_[0]};
  my $file = $cmd{'arg'};
  print "[Tesh/INFO] mkfile $file\n";

  die "[TESH/CRITICAL] no input provided to mkfile\n" unless defined($cmd{'in'}) && scalar @{$cmd{'in'}};
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
  unless ($line =~ m/^(.).(.*)$/) {
    if (defined($cmd{'cmd'})) {
      exec_cmd(\%cmd);
      %cmd = ();
    }
    next LINE;
  }     
 
  my ($cmd,$arg) = ($1,$2);
  $arg =~ s/\r//g;
  $arg =~ s/\\\\/\\/g;
  # handle the commands
  if ($cmd =~ /^#/) {    #comment
  } elsif ($cmd eq '>'){    #expected result line
    print "[TESH/debug] push expected result\n" if $opts{'debug'};
  $arg=trim($arg);
    if($arg ne ""){
    push @{$cmd{'out'}}, $arg;
  }

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
  }
  elsif($line =~ /^!\s*output ignore/){    #output ignore
    if (defined($cmd{'cmd'})) {
      exec_cmd(\%cmd);
      %cmd = ();
    }
    $cmd{'output ignore'} = 1;
  }
  elsif($line =~ /^!\s*expect signal (\w*)$/) {#expect signal SIGABRT
    if (defined($cmd{'cmd'})) {
      exec_cmd(\%cmd);
      %cmd = ();
    }
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
  elsif($line =~ /^!\s*include/){    #output sort
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
}



# Deal with last command
if (defined($cmd{'cmd'})) {
  exec_cmd(\%cmd);
  %cmd = ();
}


if($forked){
   kill(9, $forked);
   $timeout=0;
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


