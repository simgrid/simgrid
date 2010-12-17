#! /usr/bin/perl -w

use strict;

if($#ARGV<0){die "Usage: tesh.pl <options> <teshfile.tesh>\n";}
my($line1);
my($line2);
my($execline);
my($ok)=0;
my($ok1)=0;
my($sort)=0;
my($nb_arg)=0;
my($result);
my(@list1)=();
my(@list2)=();
my(@list3)=();
my(@buffer)=();
my($timeout)=-1;
my($parallel)=0;
my($verbose)=0;
my($encore)=0;
my($old_buffer);

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
		$verbose=1;
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
	if($line1 =~ /^\$ /){  	#command line
		$line1 =~ s/\$\{srcdir\:\=\.\}/./g;
		$line1 =~ s/\${EXEEXT:=}//g;
		$line1 =~ s/^\$\ *//g;
		$line1 =~ s/^.\/lua/lua/g;
		$line1 =~ s/^.\/ruby/ruby/g;
		$line1 =~ s/^.\///g;
		$line1 =~ s/\(%i:%P@%h\)/\\\(%i:%P@%h\\\)/g;
		chomp $line1;
		if(@list1){
			print "[Tesh/CRITICAL] Old result : @list1";
			print "[Tesh/CRITICAL] Previous result not check!";
			die;}
		if($parallel == 1)
		{
			$execline = "$execline \& $line1 2>&1";
			print "[Tesh/INFO] exec_line // : $execline\n";
		}
		else
		{
			$ok = 1;
			$execline = "$line1  2>&1";
			print "[Tesh/INFO] exec_line : $execline\n";
			$result=`$execline`;
			@list1 = split(/\n/,$result);
		}
	}
	elsif($line1 =~ /^\& /){  	# parallel command line
		$line1 =~ s/\$\{srcdir\:\=\.\}/./g;
		$line1 =~ s/\${EXEEXT:=}//g;
		$line1 =~ s/^\& //g;
		$line1 =~ s/^.\/lua/lua/g;
		$line1 =~ s/^.\/ruby/ruby/g;
		$line1 =~ s/^.\///g;
		$line1 =~ s/\(%i:%P@%h\)/\\\(%i:%P@%h\\\)/g;
		chomp $line1;
		if(@list1){
			print "Old result : @list1";
			die "Previous result not check!";}
		if($parallel == 1)
		{
			$execline = "$execline \& $line1  2>&1";
			print "[Tesh/INFO] exec_line // : $execline\n";
		}
		else
		{
			$parallel = 1;
			$execline = "$line1 2>&1";
			print "[Tesh/INFO] exec_line // : $execline\n";
		}
		
	}	
	elsif($line1 =~ /^\>/){	#expected result line
		if($ok == 0 and $parallel== 0){die "No command line\n";}
		$ok1 = 1;
		$line1 =~ s/^\> //g;
		$line1 =~ s/\r//g;
		chomp $line1;
		push @list2, $line1;
	}
	elsif($line1 =~ /^\</ or $encore==1){	#need to buffer
		$line1 =~ s/^\< //g;
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
			if($encore == 1)
			{	
				push @buffer, "$old_buffer$line1";
				$old_buffer = ();
				$encore = 0;				
			}
			else
			{
				push @buffer, "$line1\n";
				$old_buffer = ();
				$encore = 0;					
			}
		}
	}
	elsif($line1 =~ /^p/){	#comment
		$line1 =~ s/^p //g;
		$line1 =~ s/\r//g;
		chomp $line1;
		print "[Tesh/INFO] comment_line :$line1\n";
	}
	elsif($line1 =~ /^! output sort/){	#output sort
		print "[Tesh/INFO] output sort\n";
		$sort=1;
	}
	elsif($line1 =~ /^! expect return/){	#expect sort
		print "[Tesh/CRITICAL] expect return\n";
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
		die "[Tesh/CRITICAL] need include\n";
	}
	elsif($line1 =~ /^! timeout/){	#timeout
		$line1 =~ s/^! timeout //g;
		$line1 =~ s/\r//g;
		chomp $line1;
		$timeout = $line1;
		print "[Tesh/INFO] timeout   : $timeout\n";
	}
	elsif($ok == 1 and $ok1 == 1)
	{
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
		while(@list1 or @list2)
		{
			$line1 = shift (@list1);
			$line2 = shift (@list2);
			if($line2 and $line1)
			{
				if($line1 eq $line2){
					if($verbose == 1){print "$line1\n";}
					else{push @buffer, "$line1\n";}
					
				}
				else
				{	if($verbose == 0){print @buffer};
					if($line2) {print "- $line2\n"};
					if($line1) {print "+ $line1\n"};
					die;
				}
			}
			else
			{	if($verbose == 0){print @buffer};
				if($line2) {print "- $line2\n"};
				if($line1) {print "+ $line1\n"};
				die;
			}
		}
		$ok = 0;
		$ok1= 0;
		@list1=();
		@list2=();
		@buffer = ();
		$result = ();
	}
}

if($parallel == 1)
{
	$ok = 1;
	$result=`$execline`;
	@list1 = split(/\n/,$result);
}
elsif($ok == 1 and $ok1 == 1)
		{
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
			while(@list1 or @list2)
			{
				$line1 = shift (@list1);
				$line2 = shift (@list2);
				if($line2 and $line1)
				{
					if($line1 eq $line2){
						if($verbose == 1){print "$line1\n";}
						else{push @buffer, "$line1\n";}
						
					}
					else
					{	if($verbose == 0){print @buffer};
						if($line2) {print "- $line2\n"};
						if($line1) {print "+ $line1\n"};
						die;
					}
				}
				else
				{	if($verbose == 0){print @buffer};
					if($line2) {print "- $line2\n"};
					if($line1) {print "+ $line1\n"};
					die;
				}
			}
			$ok = 0;
			$ok1= 0;
			@list1=();
			@list2=();
			@buffer = ();
}