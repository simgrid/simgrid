#!/usr/bin/perl -w
use strict;

if($#ARGV!=2) {
    die "Usage: perl make_tesh.pl <directory> <old.tesh> <new.tesh>\n";
}

my($directory)=$ARGV[0];
my($old)=$ARGV[1];
my($new)=$ARGV[2];

chdir("$directory");

open SH_LIGNE, $old or die "Unable to open $old. $!\n";

my($line);
my($line_exec);
my($l);
my($tmp);

print "#! ./tesh\n";

while(defined($line=<SH_LIGNE>))
{
	if($line =~ /^p(.*)$/)
	{
		print "$line\n";
	}
	else
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
			open (FILE, "$line 2>&1|");
			while(defined($l=<FILE>))
			{
			chomp $l;
			print "\> $l\n";
			}

		}
		close(FILE);
    	}
}

close(SH_LIGNE);
