#! /usr/bin/perl -w

use strict;
use Term::ANSIColor;
use IPC::Open3;

if($#ARGV < 0)
{
	die "Usage: tesh.pl <options> <teshfile.tesh>\n";
}

my($line1);
my($line2);
my($execline);
my($command);
my($command_tesh);
my($command_executed)=0;
my($expected_result_line)=0;
my($sort)=0;
my($nb_arg)=0;
my(@list1)=();
my(@list2)=();
my(@list3)=();
my(@list_of_commands)=();
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

my($tesh_command)=0;
my(@buffer_tesh)=();

#options
do{
	if($ARGV[$nb_arg] =~ /^--cd$/)
	{
		$nb_arg++;
		if(!$ARGV[$nb_arg] or $ARGV[$nb_arg] =~ /^--/){die "Usage: tesh.pl --cd <directory>\n";}
		my($directory)=$ARGV[$nb_arg];
		if( -e $directory) 
		{
			chdir("$directory");
			print "[Tesh/INFO] Change directory to \"$directory\"\n";
		}
		else
		{
			die "[Tesh/CRITICAL] Directory not found : \"$directory\"\n";
		}
		$nb_arg++;	
	}
	elsif($ARGV[$nb_arg] =~ /^--setenv$/)
	{
		$nb_arg++;
		if(!$ARGV[$nb_arg] or $ARGV[$nb_arg] =~ /^--/){die "Usage: tesh.pl --setenv environment_variable\n";}
		if(!$ARGV[$nb_arg+1] or $ARGV[$nb_arg+1] =~ /^--/){die "Usage: tesh.pl --setenv environment_variable\n";}
		$ENV{$ARGV[$nb_arg]} = "$ENV{$ARGV[$nb_arg]}:$ARGV[$nb_arg+1]";
		print "[Tesh/INFO] export $ARGV[$nb_arg]=\"$ENV{$ARGV[$nb_arg]}\"\n";
		$nb_arg++;
		$nb_arg++;
	}
	elsif($ARGV[$nb_arg] =~ /^--verbose$/)
	{
		$verbose=1;$nb_arg++;
	}
	else
	{
		print "[Tesh/CRITICAL] Unrecognized option : $ARGV[$nb_arg]\n";
		$nb_arg++;
	}
}while(($nb_arg) < $#ARGV);

#Add current directory to path
$ENV{PATH} = "$ENV{PATH}:.";

#tesh file
if(!$ARGV[$nb_arg]){die "tesh.pl <options> <teshfile.tesh>\n";}
print "[Tesh/INFO] load file : $ARGV[$nb_arg]\n";
my($file)=$ARGV[$nb_arg];
open SH_LIGNE, $file or die "[Tesh/CRITICAL] Unable to open $file. $!\n";

while(defined($line1=<SH_LIGNE>))
{
	if($line1 =~ /^\< \$ /){  	# arg command line
		$line1 =~ s/\$\{srcdir\:\=\.\}/./g;
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
		$line1 =~ s/\$\{srcdir\:\=\.\}/./g;
		$line1 =~ s/\${EXEEXT:=}//g;
		$line1 =~ s/^\$\ *//g;
		$line1 =~ s/^.\/lua/lua/g;
		$line1 =~ s/^.\/ruby/ruby/g;
		$line1 =~ s/^.\///g;
		$line1 =~ s/^tesh/.\/tesh/g;
		$line1 =~ s/\(%i:%P@%h\)/\\\(%i:%P@%h\\\)/g;
		chomp $line1;
		if(@list1){
			print color("red");
			print "[Tesh/CRITICAL] -\n";
			print "[Tesh/CRITICAL] + @list1\n";
			print color("reset");
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
		$line1 =~ s/\$\{srcdir\:\=\.\}/./g;
		$line1 =~ s/\${EXEEXT:=}//g;
		$line1 =~ s/^\& //g;
		$line1 =~ s/^.\/lua/lua/g;
		$line1 =~ s/^.\/ruby/ruby/g;
		$line1 =~ s/^.\///g;
		$line1 =~ s/\(%i:%P@%h\)/\\\(%i:%P@%h\\\)/g;
		chomp $line1;
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
		print color("red"), "[Tesh/INFO] expect return $return\n";
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
		color("red");die "[Tesh/CRITICAL] need include\n";
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
					if($line2) {print color("red"), "[Tesh/CRITICAL] - $line2\n",color("reset");}
					if($line1) {print color("red"), "[Tesh/CRITICAL] + $line1\n",color("reset");}
					die;
				}
			}
			else
			{	if($verbose == 0){print color("green"),@buffer,color("reset");}
				if($line2) {print color("red"), "[Tesh/CRITICAL] - $line2\n",color("reset");}
				if($line1) {print color("red"), "[Tesh/CRITICAL] + $line1\n",color("reset");}
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
						if($line2) {print color("red"), "[Tesh/CRITICAL] - $line2\n",color("reset");}
						if($line1) {print color("red"), "[Tesh/CRITICAL] + $line1\n",color("reset");}
						die;
					}
				}
				else
				{	if($verbose == 0){print color("green"),@buffer,color("reset");}
					if($line2) {print color("red"), "[Tesh/CRITICAL] - $line2\n",color("reset");}
					if($line1) {print color("red"), "[Tesh/CRITICAL] + $line1\n",color("reset");}
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