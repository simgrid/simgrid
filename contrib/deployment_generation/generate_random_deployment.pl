#!/usr/bin/perl -w
use strict;

sub melange {
    my $tableau=shift;
    my($i,$j);
    
    for($i = @$tableau ; --$i; ) {
	$j = int rand ($i+1);
	next if $i==$j;
	@$tableau[$i,$j] = @$tableau[$j,$i];
    }
}

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
    my(%taken);

    $nflows< $nhost*$nhost-$nhost or die "Too much flows! I can't do it\n";
    
    open(OUTPUT,"> $filename");
    while(scalar(keys(%pairs))<$nflows) {
	my($src)=int(rand(scalar(@$host_list)));
	my($dst)=int(rand(scalar(@$host_list)));

	if($src!=$dst && !defined($pairs{"$$host_list[$src] $$host_list[$dst]"})) {
	    $pairs{"$$host_list[$src] $$host_list[$dst]"}=1;
	    $taken{"$$host_list[$src]"}=1;
	    $taken{"$$host_list[$dst]"}=1;
# 	   && !$taken{$$host_list[$src]} && !$taken{$$host_list[$dst]}
	}
    }
    my($p);

    my($count)=0;

    print OUTPUT <<EOF;
<?xml version='1.0'?>
<!DOCTYPE platform SYSTEM "simgrid.dtd">
<platform version="2">
EOF

    foreach $p (keys %pairs) {
	my($src,$dst)=split(/ /,$p);
	print OUTPUT "  <process host='$src' function='master'>\n";
	print OUTPUT "    <argument value='10000000'/>\n";
	print OUTPUT "    <argument value='$dst'/>\n";
	print OUTPUT "    <argument value='$count'/>\n";
	print OUTPUT "  </process>\n";
	print OUTPUT "  <process host='$dst' function='slave'>\n";
	print OUTPUT "    <argument value='$count'/>\n";
	print OUTPUT "  </process>\n";
	$count++;
    }

    print OUTPUT <<EOF;
</platform>
EOF
    close(OUTPUT);
}


sub generate_random_deployment2{
    my($host_list,$nflows,$filename)=@_;
    my(%pairs);
    my($nhost) = scalar(@$host_list);
    my(%taken);

    melange($host_list);
    $nflows< $nhost/2 or die "Too much flows! I can't do it\n";
    
    open(OUTPUT,"> $filename");
    foreach (0..$nflows-1) {
	my($src)=shift(@$host_list);
	my($dst)=shift(@$host_list);

	$pairs{"$src $dst"}=1;
    }
    my($p);

    my($count)=0;

    print OUTPUT <<EOF;
<?xml version='1.0'?>
<!DOCTYPE platform SYSTEM "simgrid.dtd">
<platform version="2">
EOF

    foreach $p (keys %pairs) {
	my($src,$dst)=split(/ /,$p);
	print OUTPUT "  <process host='$src' function='master'>\n";
	print OUTPUT "    <argument value='10000000'/>\n";
	print OUTPUT "    <argument value='$dst'/>\n";
	print OUTPUT "    <argument value='$count'/>\n";
	print OUTPUT "  </process>\n";
	print OUTPUT "  <process host='$dst' function='slave'>\n";
	print OUTPUT "    <argument value='$count'/>\n";
	print OUTPUT "  </process>\n";
	$count++;
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
    generate_random_deployment2($host_list,$nflows,"$filename-d.xml");
}

main;
