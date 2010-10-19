#!perl -w
use strict;
use IPC::Open3;

if($#ARGV!=1) {
    die "Usage: perl tesh.pl <directory> <teshfile.tesh>\n";
}

my($directory)=$ARGV[0];
my($file)=$ARGV[1];

chdir("$directory");
print "Change directory to \"$directory\"\n";

open SH_LIGNE, $file or die "Unable to open $file. $!\n";

my($line1);
my($line2);
my($execline);
my($ok)=0;
my(@result)=();

while(defined($line1=<SH_LIGNE>))
{
		if($line1 =~ /^\$(.*)$/) #command line
		{ 
	    	$ok = 1;
	    	@result = ();
			$line1 =~ s/\$\{srcdir\:\=\.\}/./g;
			$line1 =~ s/\$SG_TEST_EXENV//g;
			$line1 =~ s/\$EXEEXT//g;
			$line1 =~ s/\${EXEEXT:=}//g;
			$line1 =~ s/^\$\ */.\//g;
			$line1 =~ s/^.\/lua/lua/g;
			$line1 =~ s/^.\/ruby/ruby/g;
			chomp $line1;
			$execline = $line1;
			close(FILE_ERR);
			close(FILE);
			open3("&STDIN",\*FILE,\*FILE_ERR,"$execline") or die "cannot run $execline:\n$!";	
			print "$execline";
			
		}
		elsif($line1 =~ /^\>(.*)$/) #expected result line	
		{
			if($ok == 0){die "No command line$!";}
			$line1 =~ s/^\> //g;
			chomp $line1;
			
			if($line1 =~ /^.*\[.*\].*\[.*\/INFO\]/)
			{
				if(!defined($line2=<FILE_ERR>)){die "Don't have FILE_ERR$!";}
			}
			else
			{
				if(!defined($line2=<FILE>)){die "Don't have FILE$!";}
			}
			
			if($line2 =~ /^.*$/) #command result line
			{
				$line2 =~ s/\r//g;							
				chomp $line2;
				
				if($line2 eq $line1)
				{
					push @result, "$line1 \n";
				}
				else
				{
					print @result;
					print "- $line1\n";
					print "+ $line2\n";
					print " NOK";
					die;
				}
			}
			else
			{
				print @result;
				print "- $line1\n";
				print "+ $line2\n";
				print " NOK";
				die;
			}
		}
		else
		{
			if($ok == 1)
			{
			print " OK\n";
			$ok = 0;				
			}
			else
			{
				print " erreur\n";
			}
		}
}
if($ok == 1)
{
	print " OK\n";
	$ok = 0;				
}
close(SH_LIGNE);
close(FILE_ERR);
close(FILE);