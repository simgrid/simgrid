#!perl -w
use strict;

if($#ARGV!=1){die "Usage: perl tesh.pl <directory> <teshfile.tesh>\n";}
my($directory)=$ARGV[0];
my($file)=$ARGV[1];
my($line1);
my($line2);
my($execline);
my($ok)=0;
chdir("$directory");
print "Change directory to \"$directory\"\n";

open SH_LIGNE, $file or die "Unable to open $file. $!\n";

while(defined($line1=<SH_LIGNE>))
{
		if($line1 =~ /^\$/){  	#command line
			$ok = 1;
			$line1 =~ s/\$\{srcdir\:\=\.\}/./g;
			$line1 =~ s/\$SG_TEST_EXENV//g;
			$line1 =~ s/\$SG_EXENV_TEST//g;
			$line1 =~ s/\$EXEEXT//g;
			$line1 =~ s/\${EXEEXT:=}//g;
			$line1 =~ s/^\$\ *//g;
			$line1 =~ s/^.\/lua/lua/g;
			$line1 =~ s/^.\/ruby/ruby/g;
			chomp $line1;
			$execline = $line1;
			print "$execline\n";
			system "$execline 1>output_tesh.txt 2>output_tesh_err.txt";
			close(FILE_ERR);
			close(FILE);
			open (FILE, "output_tesh.txt");
			open (FILE_ERR, "output_tesh_err.txt");}
			
		if($line1 =~ /^\>/){	#expected result line
			if($ok == 0){die "No command line$!";}
			$line1 =~ s/^\> //g;
			$line1 =~ s/\r//g;
			chomp $line1;
			
			if($line1 =~ /^.*\[.*\].*\[.*\/INFO\].*$/)
				{if(!defined($line2=<FILE_ERR>))
					{	print "- $line1\n";
						die;}}
			elsif($line1 =~ /^.*\[.*\].*\[.*\/WARNING\].*$/)
				{if(!defined($line2=<FILE_ERR>))
					{	print "- $line1\n";
						die;}}
			elsif($line1 =~ /^.*\[.*\].*\[.*\/CRITICAL\].*$/)
				{if(!defined($line2=<FILE_ERR>))
					{	print "- $line1\n";
						die;}}
			elsif($line1 =~ /^.*\[.*\].*\[.*\/DEBUG\].*$/)
				{if(!defined($line2=<FILE_ERR>))
					{	print "- $line1\n";
						die;}}
			else{if(!defined($line2=<FILE>))
					{	print "- $line1\n";
						die;}}
			
			$line2 =~ s/\r//g;							
			chomp $line2;
			
			if($line2 eq $line1){}
			else
			{	print "- $line1\n";
				print "+ $line2\n";
				die;}}	
}
if($ok == 1){
	print "Test of \"$file\" OK\n";
	$ok = 0;}
	
close(SH_LIGNE);
close(FILE_ERR);
close(FILE);