#!perl -w
use strict;

if($#ARGV!=1) {
    die "Usage: perl tesh.pl <directory> <teshfile.tesh>\n";
}

my($directory)=$ARGV[0];
my($file)=$ARGV[1];

chdir("$directory");

open SH_LIGNE, $file or die "Unable to open $file. $!\n";

my($line);
my($line_exec);
my($l);
my($tmp);

while(defined($line=<SH_LIGNE>))
{

		if($line =~ /^\$(.*)$/) 
	    	{
			$line_exec = $line;
			$line =~ s/\$\{srcdir\:\=\.\}/./g;
			$line =~ s/\$SG_TEST_EXENV//g;
			$line =~ s/\$EXEEXT//g;
			$line =~ s/^\$\ */.\//g;
			$line =~ s/^.\/lua/lua/g;
			$line =~ s/^.\/ruby/ruby/g;
			print "$line_exec\n";
			chomp $line;
			system "$line";
			}
}

close(SH_LIGNE);
