#!/usr/bin/perl -w
use strict;

sub read_file {
    my($filename)=shift;
    my($line);
    my(@host_list);
    open(INPUT,"$filename");
    while(defined($line=<INPUT>)) {
	chomp $line;
	if($line=~/host id/) {
	    $line=~ s/.*host id="//;
	    $line=~ s/\".*$//;
	    push @host_list,$line;
	}
    }
    close(INPUT);
    return \@host_list;
}

sub generate_random_deployment{
    my($host_list,$nflows,$filename)=@_;
    my(%pairs);
    my($nhost) = scalar(@$host_list);

    $nflows< $nhost*$nhost-$nhost or die "Too much flows! I can't do it\n";
    
    open(OUTPUT,"> $filename");
    while(scalar(keys(%pairs))<$nflows) {
	my($src)=int(rand(scalar(@$host_list)));
	my($dst)=int(rand(scalar(@$host_list)));

	if($src!=$dst && !defined($pairs{"$$host_list[$src] $$host_list[$dst]"})) {
	    $pairs{"$$host_list[$src] $$host_list[$dst]"}=1;
	}
    }
    my($p);
    print OUTPUT <<EOF;
<?xml version='1.0'?>
<!DOCTYPE platform SYSTEM "simgrid.dtd">
<platform version="2">
EOF

    foreach $p (keys %pairs) {
	my($src,$dst)=split(/ /,$p);
	print OUTPUT "  <process host='$src' function='master'>\n";
	print OUTPUT "    <argument value='1000000'/>\n";
	print OUTPUT "    <argument value='$dst'/>\n";
	print OUTPUT "  </process>\n";
	print OUTPUT "  <process host='$dst' function='slave'/>\n";
    }
    print OUTPUT <<EOF;
</platform>
EOF
    close(OUTPUT);
}

sub main {
    my($nodes,$edges,$interferences,$host_list,$count_interferences);

    $#ARGV>=1 or die "Need a xml platform file and a number of flows!";
    my($filename)=$ARGV[0];
    my($nflows)=$ARGV[1];
    $filename =~ s/\.xml$//g;
    $filename =~ s/-p$//g;
    
    $host_list = read_file $ARGV[0];
    generate_random_deployment($host_list,$nflows,"$filename-d.xml");
}

main;
