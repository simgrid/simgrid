#! /usr/bin/perl -w

=encoding UTF-8

=head1 NAME

tesh -- testing shell

=head1 SYNOPSIS

B<tesh> [I<options>] I<tesh_file>

=cut

use Pod::Usage qw(pod2usage);
use Getopt::Long qw(GetOptions);
use strict;
use Term::ANSIColor;
use IPC::Open3;


my($line1,$line2,$execline,$command,$command_tesh);
my($command_executed)=0;
my($expected_result_line)=0;
my($sort)=0;
my($nb_arg)=0;
my(@list1,@list2,@list3,@list_of_commands)=();
my(@buffer)=();
my($timeout)=0;
my($encore)=0;
my($old_buffer);
my($linebis);
my($SIGABRT)=0;
my($no_output_ignore)=1;
my($verbose)=0;
my($return)=-1;
my($pid);
my($result);
my($result_err);
my($fich_name);
my($forked);
my($config)="";

my($tesh_command)=0;
my(@buffer_tesh)=();

# make sure we received a tesh file
scalar @ARGV > 0 || pod2usage(-exitval => 1);

#Add current directory to path
$ENV{PATH} = "$ENV{PATH}:.";


#options
sub cd_cmd {
    my $directory=$_[1];
    if (-e $directory) {
	chdir("$directory");
	print "[Tesh/INFO] change directory to $directory\n";
    } else {
	die "[Tesh/CRITICAL] Directory not found : \"$directory\"\n";
    }
}

sub setenv_cmd {
    if ($_[1] =~ /^(.*)=(.*)$/) {
	my($var,$ctn)=($1,$2);
	$ENV{$var} = $ctn;
	print "[Tesh/INFO] setenv $var=$ctn\n";
    } else { 
	die "[Tesh/CRITICAL] Malformed argument to setenv: expected 'name=value' but got '$_[1]'\n";
    }
}

my $tesh_file;
sub get_options {
    # remove the tesh file from the ARGV used
    my @ARGV = @_;
    $tesh_file = pop @ARGV;

    # temporary arrays for GetOption
    my @verbose = ();
    my @cfg;

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
	'setenv=s'   => \&setenv_cmd,
	'cfg=s'      => \@cfg,
	);

    $opt{'verbose'} = scalar @verbose;
    foreach (@cfg) {
	$opt{'cfg'} .= " --cfg=$_";
    }
    return %opt;
}

my %opts = get_options(@ARGV);

# parse tesh file
open SH_LIGNE, $tesh_file or die "[Tesh/CRITICAL] Unable to open $tesh_file: $!\n";

while(defined($line1=<SH_LIGNE>))
{
	if($line1 =~ /^\< \$ /){  	# arg command line
		$line1 =~ s/\${EXEEXT:=}//g;
		$line1 =~ s/^\< \$\ *//g;
		$line1 =~ s/^.\/lua/lua/g;
		$line1 =~ s/^.\/ruby/ruby/g;
		$line1 =~ s/^.\///g;
		$line1 =~ s/^tesh/.\/tesh/g;
		$line1 =~ s/\(%i:%P@%h\)/\\\(%i:%P@%h\\\)/g;
		chomp $line1;
		$command_tesh = $line1;
		print "[Tesh/INFO] arg_exec_line   : $command_tesh\n";
	}
	elsif($line1 =~ /^\< \< /){  # arg buffer line
		$line1 =~ s/^\< \<\ *//g;
		chomp $line1;
		print "[Tesh/INFO] arg_buffer_line : $line1\n";
		push @buffer_tesh, "$line1\n";
	}
	elsif($line1 =~ /^\< \> /){  # arg output line
		$line1 =~ s/^\< \>\ *//g;
		$line1 =~ s/\r//g;
		chomp $line1;
		push @list2, $line1;
		print "[Tesh/INFO] arg_output_line : $line1\n";
		$expected_result_line = 1;
	}
	elsif($line1 =~ /^\$ mkfile/){  	# "mkfile" command line
		$line1 =~ s/\$ //g;
		$line1 =~ s/mkfile//g;
		chomp $line1;
		$fich_name = $line1;
		$line1 =();
		print "[Tesh/INFO] exec_line : mkfile $fich_name\n";
		`rm -f $fich_name`;
		open(FILE,">$fich_name") or die "[Tesh/CRITICAL] Unable to make file : $fich_name. $!\n";
		print FILE @buffer;
		close(FILE);
		@buffer = ();	
	}
	elsif($line1 =~ /^\$ cd/){  	# "cd" command line
		$line1 =~ s/\$ //g;
		chomp $line1;
		print "[Tesh/INFO] exec_line : $line1\n";
		$line1 =~ s/cd //g;
		chdir("$line1") or die "[Tesh/CRITICAL] Unable to open $line1. $!\n";	
	}
	elsif($line1 =~ /^\$ /){  	#command line
		if($line1 =~ /^\$ .\/tesh/){  	# tesh command line
			$tesh_command = 1;
			@buffer = @buffer_tesh;
			@buffer_tesh=();
		}
		$line1 =~ s/\${EXEEXT:=}//g;
		$line1 =~ s/^\$\ *//g;
		$line1 =~ s/^.\/lua/lua/g;
		$line1 =~ s/^.\/ruby/ruby/g;
		$line1 =~ s/^.\///g;
		$line1 =~ s/^tesh/.\/tesh/g;
		$line1 =~ s/\(%i:%P@%h\)/\\\(%i:%P@%h\\\)/g;
		chomp $line1;
		$line1 = "$line1 $config";
		
		if(@list1){
			print color("red");
			print "[Tesh/CRITICAL] -\n";
			print "[Tesh/CRITICAL] + @list1";
			print color("reset"), "\n";
			die;}		
		if(@list_of_commands){ # need parallel execution
			push @list_of_commands, $line1;
			print "[Tesh/INFO] exec_line : $line1\n";
		}
		else{
			print "[Tesh/INFO] exec_line : $line1\n";
			if($tesh_command)
			{	$execline = $command_tesh;
				$tesh_command=0;}
			else
			{	$execline = $line1;}
			$pid = open3(\*IN, \*OUT, \*OUT, $execline );
			if( $timeout){
				$forked = fork();die "fork() failed: $!" unless defined $forked;
				if ( $forked == 0 )
				{
				sleep $timeout;
				kill(9, $pid);
				exit;
				}
			}
			
			while(@buffer)
			{
				$line1 = shift (@buffer);
				print IN $line1;
			}
			close IN ;
			
			waitpid( $pid, 0 );
			if($timeout){kill(9, $forked);$timeout=0;}
			$timeout = 0;
			
			while(defined($linebis=<OUT>))
			{
				$linebis =~ s/\r//g;
				$linebis =~ s/^( )*//g;
				chomp $linebis;
				push @list1,"$linebis";
			}	
			close OUT;
			$command_executed = 1;
		}
	}
	elsif($line1 =~ /^\& /){  	# parallel command line
		$command_executed = 0;
		$expected_result_line = 0;
		$line1 =~ s/\${EXEEXT:=}//g;
		$line1 =~ s/^\& //g;
		$line1 =~ s/^.\/lua/lua/g;
		$line1 =~ s/^.\/ruby/ruby/g;
		$line1 =~ s/^.\///g;
		$line1 =~ s/\(%i:%P@%h\)/\\\(%i:%P@%h\\\)/g;
		chomp $line1;
		$line1 = "$line1 $config";
		
		$execline = "$line1";
		print "[Tesh/INFO] exec_line : $execline\n";
		push @list_of_commands, $execline;	
	}	
	elsif($line1 =~ /^\>/){	#expected result line
		$line1 =~ s/^\>( )*//g;
		$line1 =~ s/\r//g;
		chomp $line1;
		push @list2, $line1;
		$expected_result_line = 1;
	}
	elsif($line1 =~ /^\</ or $encore==1){	#need to buffer
		$line1 =~ s/^\<( )*//g; #delete < with space or tab after
		$line1 =~ s/\r//g;
		chomp $line1;
		if($line1 =~ /\\$/) # need to store this line into old_buff
		{
			$encore=1;
			$line1 =~ s/\\$//g;
			$old_buffer = "$old_buffer$line1";
		}
		else
		{
			if($encore == 1){push @buffer, "$old_buffer$line1";}
			else{push @buffer, "$line1\n";}
			$old_buffer = ();
			$encore = 0;	
		}
	}
	elsif($line1 =~ /^p/){	#comment
		$line1 =~ s/^p //g;
		$line1 =~ s/\r//g;
		chomp $line1;
		print "[Tesh/INFO] comment_line :$line1\n";
	}
	elsif($line1 =~ /^! output sort/){	#output sort
		$sort=1;
	}
	elsif($line1 =~ /^! output ignore/){	#output ignore
		$no_output_ignore=0;
	}
	elsif($line1 =~ /^! expect signal SIGABRT$/) #expect signal SIGABRT
	{
		$SIGABRT=1;
	}
	elsif($line1 =~ /^! expect return/){	#expect return
		$line1 =~ s/^! expect return //g;
		$line1 =~ s/\r//g;
		chomp $line1;
		$return = $line1;
		print color("red"), "[Tesh/INFO] expect return $return";
		print color("reset"), "\n";
		die;
	}
	elsif($line1 =~ /^! setenv/){	#setenv
		$line1 =~ s/^! setenv //g;
		$line1 =~ s/\r//g;
		chomp $line1;
		$line1 =~ /^(.*)=(.*)$/;
		$ENV{$1} = $2;
		print "[Tesh/INFO] setenv: $1=$2\n";
	}
	elsif($line1 =~ /^! include/){	#output sort
		print color("red"), "[Tesh/CRITICAL] need include";
		print color("reset"), "\n";
		die;
	}
	elsif($line1 =~ /^! timeout/){	#timeout
		$line1 =~ s/^! timeout //g;
		$line1 =~ s/\r//g;
		chomp $line1;
		if($timeout != $line1){
		$timeout = $line1;
		print "[Tesh/INFO] timeout   : $timeout\n";}
	}
	elsif($line1 =~ /^! need execute/){	#need execute // commands
		$execline = ();
		$sort = 1; # need sort output
		while(@list_of_commands)
		{
			$command = shift (@list_of_commands);
			if($execline){$execline = "$command & $execline";}
			else{$execline = "$command";}
		}
		$pid = open3(\*IN, \*OUT, \*OUT, $execline);
		if( $timeout){
			$forked = fork();die "fork() failed: $!" unless defined $forked;
			if ( $forked == 0 )
			{
			sleep $timeout;
			kill(9, $pid);
			exit;
			}
		}
		
		while(@buffer)
		{
			$line1 = shift (@buffer);
			print IN $line1;
		}
		close IN ;
		waitpid( $pid, 0 );
		if($timeout){kill(9, $forked);$timeout=0;}
		$timeout = 0;

		@list1=();
		while(defined($linebis=<OUT>))
		{
			$linebis =~ s/\r//g;
			$linebis =~ s/^( )*//g;
			chomp $linebis;
			push @list1,"$linebis";
		}	
		close OUT;
		$command_executed = 1;
	}
	elsif($command_executed and $expected_result_line)
	{
		if($no_output_ignore){
		@buffer = ();
		if($sort == 1)
		{
			@list3 = sort @list1;
			@list1=();
			@list1=@list3;
			@list3=();
			
			@list3 = sort @list2;
			@list2=();
			@list2=@list3;
			@list3=();
			
			$sort=0;
		}
		if($SIGABRT)
		{
			push @list2,"Aborted";
			$SIGABRT = 0;
		}
				
		while(@list1 or @list2)
		{
			if(@list1){$line1 = shift (@list1);$expected_result_line = "x$line1";}
			if(@list2){$line2 = shift (@list2);$command_executed = "x$line2";}
			if($command_executed and $expected_result_line)
			{
				
				if($line1 eq $line2){
					if($verbose == 1){print color("green"),"[Tesh/VERB] $line1\n",color("reset");}
					else{push @buffer, "[Tesh/CRITICAL]   $line1\n";}
					
				}
				else
				{	if($verbose == 0){print color("green"),@buffer,color("reset");}
					if($line2) {print color("red"), "[Tesh/CRITICAL] - $line2",color("reset"),"\n";}
					if($line1) {print color("red"), "[Tesh/CRITICAL] + $line1",color("reset"),"\n";}
					die;
				}
			}
			else
			{	if($verbose == 0){print color("green"),@buffer,color("reset");}
				if($line2) {print color("red"), "[Tesh/CRITICAL] - $line2",color("reset"),"\n";}
				if($line1) {print color("red"), "[Tesh/CRITICAL] + $line1",color("reset"),"\n";}
				die;
			}
		}
		}else{$no_output_ignore = 1;}
		$command_executed = 0;
		$expected_result_line = 0;
		@list1=();
		@list2=();
		@buffer = ();
		$tesh_command=0;
		@buffer_tesh=();
	}

}

if(@list_of_commands){ # need parallel execution
	$execline = ();
	$sort = 1; # need sort output
	while(@list_of_commands)
	{
		$command = shift (@list_of_commands);
		if($execline){$execline = "$command & $execline";}
		else{$execline = "$command";}
	}
	print "[Tesh/INFO] exec_line : $execline\n";
	$pid = open3(\*IN, \*OUT, \*OUT,"$execline");

	if( $timeout){
		$forked = fork();die "fork() failed: $!" unless defined $forked;
		if ( $forked == 0 )
		{
		sleep $timeout;
		kill(9, $pid);
		exit;
		}
	}
	
	while(@buffer)
	{
		$line1 = shift (@buffer);
		print IN $line1;
	}
	close IN ;
	waitpid( $pid, 0 );
	if($timeout){kill(9, $forked);$timeout=0;}
	$timeout = 0;

	@list1=();
	while(defined($linebis=<OUT>))
	{
		$linebis =~ s/\r//g;
		$linebis =~ s/^( )*//g;
		chomp $linebis;
		push @list1,"$linebis";
	}	
	close OUT;
	$command_executed = 1;
}

if($command_executed and $expected_result_line ){
			if($no_output_ignore){
			@buffer = ();
			if($sort == 1)
			{
				@list3 = sort @list1;
				@list1=();
				@list1=@list3;
				@list3=();
				
				@list3 = sort @list2;
				@list2=();
				@list2=@list3;
				@list3=();
				
				$sort=0;
			}
			if($SIGABRT)
			{
				push @list2,"Aborted";
				$SIGABRT = 0;
			}
			
			while(@list1 or @list2)
			{
				if(@list1){$line1 = shift (@list1);$expected_result_line = "x$line1";}
				if(@list2){$line2 = shift (@list2);$command_executed = "x$line2";}
				if($command_executed and $expected_result_line)
				{
					if($line1 eq $line2){
						if($verbose == 1){print color("green"),"[Tesh/VERB] $line1\n",color("reset");}
						else{push @buffer, "[Tesh/CRITICAL]   $line1\n";}
						
					}
					else
					{	if($verbose == 0){print color("green"),@buffer,color("reset");}
						if($line2) {print color("red"), "[Tesh/CRITICAL] - $line2",color("reset"),"\n";}
						if($line1) {print color("red"), "[Tesh/CRITICAL] + $line1",color("reset"),"\n";}
						die;
					}
				}
				else
				{	if($verbose == 0){print color("green"),@buffer,color("reset");}
					if($line2) {print color("red"), "[Tesh/CRITICAL] - $line2",color("reset"),"\n";}
					if($line1) {print color("red"), "[Tesh/CRITICAL] + $line1",color("reset"),"\n";}
					die;
				}
			}
			}else{$no_output_ignore = 1;}
			$command_executed = 0;
			$expected_result_line= 0;
			@list1=();
			@list2=();
			@buffer = ();
}
