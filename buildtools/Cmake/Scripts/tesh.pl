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
my $path = $0;
my $OS;
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

print "OS: ".$OS."\n";

# make sure we received a tesh file
scalar @ARGV > 0 || die "Usage:\n    tesh [*options*] *tesh_file*\n";

#Add current directory to path
$ENV{PATH} = "$ENV{PATH}:.";


##
## Command line option handling
##

# option handling helper subs
sub cd_cmd {
    my $directory=$_[1];
    if (-e $directory && -d $directory) {
	chdir("$directory");
	print "[Tesh/INFO] change directory to $directory\n";
    } elsif (-e $directory) {
	die "[Tesh/CRITICAL] Cannot change directory to '$directory': it is not a directory\n";
    } else {
	die "[Tesh/CRITICAL] Cannot change directory to '$directory': no such directory\n";
    }
}

sub timeout_cmd{
    $timeout=$_[1];
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
my $tesh_file;
sub get_options {
    # remove the tesh file from the ARGV used
    my @ARGV = @_;
    $tesh_file = pop @ARGV;

    # temporary arrays for GetOption
    my @verbose = ();
    my @cfg;
    my $log; # ignored

    my %opt = (
	"help"    => 0,
	"debug"   => 0,
	"verbose" => 0
	);

    Getopt::Long::config('bundling', 'no_getopt_compat', 'no_auto_abbrev');
    
    GetOptions(
	'help|h'     => \$opt{'help'},

        'verbose|v'  => \@verbose,
        'debug|d'    => \$opt{"debug"},

	'cd=s'       => \&cd_cmd,
	'timeout=s'  => \&timeout_cmd,	
	'setenv=s'   => \&setenv_cmd,
	'cfg=s'      => \@cfg,
	'log=s'      => \$log,
	);

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
my($sort)=0;
my($nb_arg)=0;
my($old_buffer);
my($linebis);
my($SIGABRT)=0;
my($no_output_ignore)=1;
my($verbose)=0;
my($return)=-1;
my($pid);
my($result);
my($result_err);
my($forked);
my($config)="";

my($tesh_command)=0;
my(@buffer_tesh)=();

eval {
    use POSIX;
    sub exit_status {
	my $status = shift;
	if (POSIX::WIFEXITED($status)) {
	    return "returned code ".POSIX::WEXITSTATUS($status);
	} elsif (POSIX::WIFSIGNALED($status)) {
	    return "got signal ".$SIG{POSIX::WTERMSIG($status)};
	}
	return "Unparsable status. Is the process stopped?";
    }
};
if ($@) { # no POSIX available?
    warn "POSIX not usable to parse the return value of forked child: $@\n";
    sub exit_status {
	return "returned code 0";
    }
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
    $cmd{'cmd'} =~ s|^\./||g;
#    $cmd{'cmd'} =~ s|tesh|tesh.pl|g;
    $cmd{'cmd'} =~ s/\(%i:%P@%h\)/\\\(%i:%P@%h\\\)/g;
    $cmd{'cmd'} .= " $opts{'cfg'}" if (defined($opts{'cfg'}) && length($opts{'cfg'}));

    print "[$cmd{'file'}:$cmd{'line'}] $cmd{'cmd'}\n";

    ###
    # exec the command line
    ###
    $pid = open3(\*IN, \*OUT, \*OUT, $cmd{'cmd'} );

    # if timeout specified, fork and kill executing child at the end of timeout
    if ($timeout){
	$forked = fork();
	die "fork() failed: $!" unless defined $forked;
	if ( $forked == 0 ) { # child
	    sleep $timeout;
	    kill(9, $pid);
	    exit;
	}
    }

    # push all provided input to executing child
    map { print IN "$_\n" } $cmd{'in'};
    close IN;

    # pop all output from executing child
    my @got;
    while(defined(my $got=<OUT>)) {
	$got =~ s/\r//g;
	$got =~ s/^( )*//g;
	chomp $got;
    $got=trim($got);
	if( $got ne ""){
        push @got, "$got";
    }
    }	
    close OUT;
   
    if ($sort){   
      sub mysort{
        $a cmp $b
        }
      use sort qw(defaults _quicksort); # force quicksort
      @got = sort mysort @got;
      #also resort the other one, as perl sort is not the same as the C one used to generate teshes
      @{$cmd{'out'}}=sort mysort @{$cmd{'out'}};
    }
  
    # Cleanup the executing child, and kill the timeouter brother on need
    $cmd{'return'} = 0 unless defined($cmd{'return'});
    my $wantret = "returned code ".(defined($cmd{'return'})? $cmd{'return'} : 0);
    waitpid ($pid, 0);
    my $gotret = exit_status($?);
    if($gotret ne $wantret) {
	my $msg = "Test suite `$cmd{'file'}': NOK (<$cmd{'file'}:$cmd{'line'}> $gotret)\n".
	    "Output of <$cmd{'file'}:$cmd{'line'}> so far:\n";      
	map {$msg .=  "|| $_\n"} @got;
	print STDERR "$msg";
	exit(1);
    }
    if($timeout){kill(9, $forked);$timeout=0;}
    $timeout = 0;
	    
    ###
    # Check the result of execution 
    ###
    my $diff = build_diff(\@{$cmd{'out'}}, \@got);
    if (length $diff) {
	print color("red")."[TESH/CRITICAL$$] Output mismatch\n";
	map { print "[TESH/CRITICAL] $_\n" } split(/\n/,$diff);
	print color("reset");
	die "Tesh failed\n";
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
print "Test suite $tesh_file\n";
open TESH_FILE, $tesh_file or die "[Tesh/CRITICAL] Unable to open $tesh_file $!\n";


my %cmd; # everything about the next command to run
my $line_num=0;
LINE: while (defined(my $line=<TESH_FILE>)) {
    $line_num++;
    chomp $line;
    print "[TESH/debug] $line_num: $line\n" if $opts{'debug'};

    # deal with line continuations
    while ($line =~ /^(.*?)\\$/) {
	my $next=<TESH_FILE>;
	die "[TESH/CRITICAL] Continued line at end of file\n"
	    unless defined($next);
	chomp $next;
	print "[TESH/debug] $line_num: $next\n" if $opts{'debug'};
	$line = $1.$next;
    }

    # Push delayed commands on empty lines
    unless ($line =~ m/^(..)(.*)$/) {
	if (defined($cmd{'cmd'})) {
	    exec_cmd(\%cmd);
	    %cmd = ();
	}
	next LINE;
    } 	
 
    my ($cmd,$arg) = ($1,$2);
    $arg =~ s/\r//g;

    # handle the commands
    if ($cmd =~ /^#/) {	#comment
    } elsif ($cmd eq '> '){	#expected result line
	print "[TESH/debug] push expected result\n" if $opts{'debug'};
    $arg=trim($arg);
	if($arg ne ""){
        push @{$cmd{'out'}}, $arg;
    }

    } elsif ($cmd eq '< ') {	# provided input
	print "[TESH/debug] push provided input\n" if $opts{'debug'};
	push @{$cmd{'in'}}, $arg;

    } elsif ($cmd eq 'p ') {	# comment
	print "[Tesh/INFO] $arg\n";

    } elsif ($cmd eq '$ ') {  # Command
	# if we have something buffered, run it now
	if (defined($cmd{'cmd'})) {
	    exec_cmd(\%cmd);
	    %cmd = ();
	}
	if ($arg =~ /^ *mkfile /){  	# "mkfile" command line
	    die "[TESH/CRITICAL] Output expected from mkfile command!\n" if scalar @{cmd{'out'}};

	    $cmd{'arg'} = $arg;
	    $cmd{'arg'} =~ s/ *mkfile //;
	    mkfile_cmd(\%cmd);
	    %cmd = ();

	} elsif ($arg =~ /^ *cd /) {
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
    elsif($cmd eq '& '){  	# parallel command line
	$cmd{'background'} = 1;
	$cmd{'cmd'} = $arg;
    }	
    elsif($line =~ /^! output sort/){	#output sort
    $sort=1;
	$cmd{'sort'} = 1;
    }
    elsif($line =~ /^! output ignore/){	#output ignore
	$cmd{'output ignore'} = 1;
    }
    elsif($line =~ /^! expect signal SIGABRT$/) {#expect signal SIGABRT
	$cmd{'expect'} = "SIGABRT";
    }
    elsif($line =~ /^! expect return/){	#expect return
	$line =~ s/^! expect return //g;
	$line =~ s/\r//g;
	$cmd{'return'} = $line;
    }
    elsif($line =~ /^! setenv/){	#setenv
	$line =~ s/^! setenv //g;
	$line =~ s/\r//g;
	setenv_cmd($line);
    }
    elsif($line =~ /^! include/){	#output sort
	print color("red"), "[Tesh/CRITICAL] need include";
	print color("reset"), "\n";
	die;
    }
    elsif($line =~ /^! timeout/){	#timeout
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
    return ""     if ($chunk_count == 1 && $diff->Same());
    $diff->Reset();
    while(  $diff->Next()  ) {
	my @same = $diff->Same();
	if ($diff->Same() ) {
	    if ($diff->Next(0) > 1) { # not first chunk: print 2 first lines
		$res .= '  '.$same[0]."\n" ;
		$res .= '  '.$same[1]."\n" if (scalar @same>1);
	    } 	
	    $res .= "...\n"  if (scalar @same>2);
#	$res .= $diff->Next(0)."/$chunk_count\n";
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


