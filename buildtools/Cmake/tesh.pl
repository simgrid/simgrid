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
			print "-- Tesh options   : Change directory to \"$directory\"\n";
		}
		else
		{
			die "Directory not found : \"$directory\"\n";
		}
		$nb_arg++;	
	}
	elsif($ARGV[$nb_arg] =~ /^--setenv$/)
	{
		$nb_arg++;
		if(!$ARGV[$nb_arg] or $ARGV[$nb_arg] =~ /^--/){die "Usage: tesh.pl --setenv environment_variable\n";}
		if(!$ARGV[$nb_arg+1] or $ARGV[$nb_arg+1] =~ /^--/){die "Usage: tesh.pl --setenv environment_variable\n";}
		$ENV{$ARGV[$nb_arg]} = "$ENV{$ARGV[$nb_arg]}:$ARGV[$nb_arg+1]";
		print "-- Tesh options   : export $ARGV[$nb_arg]=\"$ENV{$ARGV[$nb_arg]}\"\n";
		$nb_arg++;
		$nb_arg++;
	}
	else
	{
		print "-- Tesh options   : Unrecognized option : $ARGV[$nb_arg]\n";
		$nb_arg++;
	}
}while(($nb_arg) < $#ARGV);

#Add current directory to path
$ENV{PATH} = "$ENV{PATH}:.";

#tesh file
if(!$ARGV[$nb_arg]){die "tesh.pl <options> <teshfile.tesh>\n";}
print "-- Tesh load file : $ARGV[$nb_arg]\n";
my($file)=$ARGV[$nb_arg];
open SH_LIGNE, $file or die "Unable to open $file. $!\n";

while(defined($line1=<SH_LIGNE>))
{
		if($line1 =~ /^\$ mkfile/){  	#command line
			$line1 =~ s/\$ //g;
			chomp $line1;
			print "-- Tesh exec_line : $line1\n";
			$line1 =~ s/mkfile//g;
			`rm -f $line1`;
			foreach(@buffer)
			{
				`echo $_ >> $line1`;
			}
			$execline = $line1;
			@buffer = ();		
		}
		elsif($line1 =~ /^\$/){  	#command line
			$ok = 1;
			$line1 =~ s/\$\{srcdir\:\=\.\}/./g;
			$line1 =~ s/\$SG_TEST_EXENV//g;
			$line1 =~ s/\$SG_EXENV_TEST//g;
			$line1 =~ s/\$EXEEXT//g;
			$line1 =~ s/\${EXEEXT:=}//g;
			$line1 =~ s/^\$\ *//g;
			$line1 =~ s/^.\/lua/lua/g;
			$line1 =~ s/^.\/ruby/ruby/g;
			$line1 =~ s/^.\///g;
			$line1 =~ s/\(%i:%P@%h\)/\\\(%i:%P@%h\\\)/g;
			chomp $line1;
			$execline = $line1;
			print "-- Tesh exec_line : $execline\n";
			$result=`$execline 2>&1`;
			@list1 = split(/\n/,$result);
			
		}		
		elsif($line1 =~ /^\>/){	#expected result line
			if($ok == 0){die "No command line$!";}
			$ok1 = 1;
			$line1 =~ s/^\> //g;
			$line1 =~ s/\r//g;
			chomp $line1;
			push @list2, $line1;
		}
		elsif($line1 =~ /^\</){	#need to buffer
			$line1 =~ s/^\< //g;
			$line1 =~ s/\r//g;
			chomp $line1;
			push @buffer, $line1;
		}
		elsif($line1 =~ /^p/){	#comment
			$line1 =~ s/^p //g;
			$line1 =~ s/\r//g;
			chomp $line1;
			print "-- Tesh comment_line :$line1\n";
		}
		elsif($line1 =~ /^! output sort/){	#output sort
			$sort=1;
		}
		elsif($line1 =~ /^! include/){	#output sort
			die "-- Tesh need include\n";
		}
		elsif($ok == 1 and $ok1 == 1)
		{
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
				if($line1 eq $line2){}
				else
				{	print "- $line2\n";
					print "+ $line1\n";
					die;}
			}
			$ok = 0;
			$ok1= 0;
			@list1=();
			@list2=();
		}
}

if($ok == 1 and $ok1 == 1)
{
	while(@list1 or @list2)
	{
		$line1 = shift (@list1);
		$line2 = shift (@list2);
		if($line1 eq $line2){}
		else
		{	print "- $line1\n";
			print "+ $line2\n";
			die;}
	}
	$ok = 0;
	$ok1= 0;
	@list1=();
	@list2=();
}