#!/usr/bin/env perl

# Copyright (c) 2006-2007, 2014. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

use strict;
use warnings;

#use Data::Dumper;
use XFig;
use POSIX;

my($grid_Y_size)=225; 
my($grid_X_size)=100550; # Can be changed to improve readability in function of the total execution time

my($color_suspended)=1;
my($color_compute)=2;
my($color_wait_for_recpt)=3;
my($color_communicate)=4;

# Determine the order of the colors in the legend
my(@color_list)=($color_compute,$color_communicate,$color_wait_for_recpt,$color_suspended);

sub read_cat {
    my(%Cat);
    my($filename)=@_;
    my($line);

    open INPUT, $filename;

    while (defined($line=<INPUT>)) {
	chomp $line;
	if($line =~ /^7\s/) {
	    my($event,$date,$id,$type,$father,@name) = split(/\s+/,$line);

	    $Cat{$id}{name}="@name ";
	    $Cat{$id}{name}=~s/\"//g;
	    $Cat{$id}{father}=$father;
	    $Cat{$id}{type}=$type;
	    $Cat{$id}{date}=$date;
	}
    }
    close INPUT;
    return \%Cat;
}

sub read_event {
    my($filename,$Cat)=@_;
    my($line);

    open INPUT, $filename;

    while (defined($line=<INPUT>)) {
	chomp $line;
	if($line =~ /^11\s/) {
	    my($event,$date,$type,$id,$state) = split(/\s+/,$line);
	    push @{$$Cat{$id}{state}}, [$date,$state];
	}
	if($line =~ /^12\s/) {
	    my($event,$date,$type,$id) = split(/\s+/,$line);
	    push @{$$Cat{$id}{state}}, [$date];
	}
    }
    close INPUT;
}

sub read_link {
    my($filename)=@_;
    my($line);
    my(%link);

    open INPUT, $filename;

    while (defined($line=<INPUT>)) {
	chomp $line;
	if($line =~ /^16\s/) {
	    my($event,$date,$type,$father,$channel,$src,$key,$trash) = split(/\t+/,$line);
	    my($numkey)=hex "$key";
	    while (defined($link{$numkey})) {$numkey++;}
	    $link{$numkey}{src}=$src;
	    $link{$numkey}{src_date}=$date;

	}
	if($line =~ /^17\s/) {
	    my($event,$date,$type,$father,$channel,$dst,$key,$trash) = split(/\t+/,$line);
	    my($numkey)=hex "$key"; 
	    while (defined($link{$numkey}{dst})) {$numkey++;}
	    $link{$numkey}{dst}=$dst;
	    $link{$numkey}{dst_date}=$date;	 
	}
    }
    close INPUT;
    return \%link;
}

sub build_cat_tree {
    my($root,$Cat)=@_;
    my(@childs)=();
    my($cat);

    foreach $cat (keys %$Cat) {
	if($$Cat{$cat}{father} eq $root) {
	    push @childs, build_cat_tree($cat,$Cat);
	}
#	print "$$Cat{$cat}{name}\t\t $Cat{$cat}{father}\n";
    }
    return [$root,@childs];
}

sub build_cat_list {
    my($tree,$cat_list)=@_;
    my($root) = shift @$tree;
    my($u);
    
    push @$cat_list,$root;

    foreach $u (@$tree) {
	build_cat_list($u,$cat_list);
    }
    unshift @$tree, $root;
}

sub cat_sorting_function {
    my($cat1,$cat2,$Cat)=@_;
    if (!defined($$Cat{$cat1}{state})) {
	if (!defined($$Cat{$cat2}{state})) {
	    return 0;
	} else {
	    return 1;
	}
    }

    if (!defined($$Cat{$cat2}{state})) {
	return -1;
    }

    my($comp) = $$Cat{$$Cat{$cat1}{'father'}}{'name'} cmp $$Cat{$$Cat{$cat2}{'father'}}{'name'};
    if ($comp == 0) {
	return $$Cat{$cat1}{'name'} cmp $$Cat{$cat2}{'name'};
    } else {
	return $comp;
    }
}

sub update_host_Y {
    my($host,$i) = @_;
    if (!defined($$host{'Y_min_host'})) {
	$$host{'Y_min_host'} = $i;
    } else {
	if ($$host{'Y_min_host'} > $i) {
	    $$host{'Y_min_host'} = $i;
	}
    }
    if (!defined($$host{'Y_max_host'})) {
	$$host{'Y_max_host'} = $i+1;
    } else {
	if ($$host{'Y_max_host'} < $i+1) {
	    $$host{'Y_max_host'} = $i+1;
	}
    }
}

sub set_cat_position {
    my($Cat,$cat_list)=@_;
    my($i)=0;
    my($cat);
    foreach $cat (sort {cat_sorting_function($a,$b,$Cat)} @$cat_list) {
	if(defined($$Cat{$cat}{state})) {
	    update_host_Y($$Cat{$$Cat{$cat}{'father'}},$i);
	    $$Cat{$cat}{Y_min} = $i;
	    $$Cat{$cat}{Y_max} = $i+1;
	    $i++;
	}
    }
}

sub create_fig {
    my($filename)=shift;
    my($fig)=new XFig;
    $fig->{object} = 'compound'; # Compound
    $fig->{elements} = [];
    $fig->{version} = 3.2;
    $fig->{orientation}   = 'Landscape';
    $fig->{justification} = 'Center';
    $fig->{units}         = 'Metric';
    $fig->{papersize}     = 'A4';
    $fig->{magnification} = '100.00';
    $fig->{multiplepage}  = 'Single';
    $fig->{transparent}   = '-2';
    $fig->{resolution}    = '1200';
    $fig->{coordsystem}   = '2';
    $fig->{filename}   = $filename;
    return $fig;
}

sub draw_cat {
    my($fig,$Cat,$Link)=@_;
    my($cat,$e,$link);
    my($max_string_length)=0;
    foreach $cat (keys %$Cat) {
	next unless (defined($$Cat{$cat}{Y_min}) && 
		     defined($$Cat{$cat}{Y_max}));
	my($text) = new XFig ('text');
#	$text->{'text'} = "$$Cat{$$Cat{$cat}{father}}{name}"."$$Cat{$cat}{name}";
	my($printed_name)= $$Cat{$cat}{name};
	$printed_name =~ s/\d+ \(0\)\s*$//;
	if (length($printed_name) > $max_string_length) {
	    $max_string_length = length($printed_name);
	}
	$text->{'text'} = "$printed_name";
	$text->{'y'} = ($$Cat{$cat}{Y_min}+$$Cat{$cat}{Y_max})/2*$grid_Y_size+68;
	$text->{'x'} = -100;
	$text->{'subtype'} = 2;
	$fig->add ($text);
    }

    my($max_date)=0;
    foreach $cat (keys %$Cat) {
	next unless (defined($$Cat{$cat}{Y_min}) && defined($$Cat{$cat}{Y_max}));
	my(@states)=();
	my($e);
	foreach $e (@{$$Cat{$cat}{state}}) {
	    my($new_date,$state) = ($$e[0],$$e[1]);
	    if ($new_date > $max_date) {
		$max_date = $new_date;
	    }
	    if(defined($state)) {
		push @states, $e;
	    } else {
		my($old_event) = pop @states;
		my($old_date) = $$old_event[0];
		$state = $$old_event[1];

# LM: I added the next line because of "undefined values"...
# normally, I think that this should not happen, but this part of code is a bit too cryptic to me
		next unless (defined($state));		

		my($line) = new XFig ('polyline');

		$line->{'depth'} = 50;  # line
		$line->{'subtype'} = 1;  # line
		$line->{'points'} = [ [$old_date*$grid_X_size, $$Cat{$cat}{Y_min}*$grid_Y_size],
				      [$new_date*$grid_X_size, $$Cat{$cat}{Y_min}*$grid_Y_size],
				      [$new_date*$grid_X_size, $$Cat{$cat}{Y_max}*$grid_Y_size],
				      [$old_date*$grid_X_size, $$Cat{$cat}{Y_max}*$grid_Y_size],
				      [$old_date*$grid_X_size, $$Cat{$cat}{Y_min}*$grid_Y_size] ];
		$line->{'areafill'} = 20;
		if($state eq "S") {
		    $line->{'fillcolor'} = $color_suspended;
		} elsif ($state eq "E") {
		    $line->{'fillcolor'} = $color_compute;
		} elsif ($state eq "B") {
		    $line->{'fillcolor'} = $color_wait_for_recpt;
		} elsif ($state eq "C") {
		    $line->{'fillcolor'} = $color_communicate;
		}
		$fig->add ($line);
	    }
	}
    }

    foreach $link (keys %$Link) {
	my($line) = new XFig ('polyline');
	my($src_date)=$$Link{$link}{src_date};
	my($src)=$$Link{$link}{src};
	my($dst_date)=$$Link{$link}{dst_date};
	my($dst)=$$Link{$link}{dst};
	$line->{'subtype'} = 1;  # line

	print STDERR "$link: $src ($src_date) -> $dst ($dst_date)\n";

	print STDERR "$$Cat{$src}{name} -> $$Cat{$dst}{name}\n";
	$line->{'points'} = [ [$src_date*$grid_X_size, 
			       ($$Cat{$src}{Y_min}+$$Cat{$src}{Y_max})/2*$grid_Y_size],
			      [$dst_date*$grid_X_size, 
			       ($$Cat{$dst}{Y_min}+$$Cat{$dst}{Y_max})/2*$grid_Y_size] ];
	$line->{'forwardarrow'} = ['1', '1', '1.00', '60.00', '120.00'];
	$fig->add ($line);
    }

# Host visualization
    my($max_Y)= 0;

    my($index_fill)=0;
    my($width_of_one_letter)=80;
    my($min_x_for_host)=-400 - $max_string_length*$width_of_one_letter;
    my($host_text_x)= $min_x_for_host + 200;

    foreach $cat (keys %$Cat) {
	next unless (defined($$Cat{$cat}{Y_min_host}) && defined($$Cat{$cat}{Y_max_host}));
	my($line) = new XFig ('polyline');
	
	$line->{'depth'} = 150;  
	$line->{'subtype'} = 1;  # line
	$line->{'points'} = [ [$min_x_for_host, $$Cat{$cat}{Y_min_host}*$grid_Y_size],
			      [$max_date*$grid_X_size+150, $$Cat{$cat}{Y_min_host}*$grid_Y_size],
			      [$max_date*$grid_X_size+150, $$Cat{$cat}{Y_max_host}*$grid_Y_size],
			      [$min_x_for_host, $$Cat{$cat}{Y_max_host}*$grid_Y_size] ];
	$line->{'areafill'} = 4+3*($index_fill % 2);
	$line->{'fillcolor'} = 0;
	$line->{'thickness'} = 0;
	$index_fill++;
	$fig->add ($line);
	
	my($text) = new XFig ('text');
	$text->{'text'} = "$$Cat{$cat}{name}";
	$text->{'angle'} = 3.14159265/2;
	$text->{'x'} = $host_text_x;
	$text->{'y'} = ($$Cat{$cat}{Y_min_host}+$$Cat{$cat}{Y_max_host})/2*$grid_Y_size;
	$text->{'subtype'} = 1;
	$text->{'font_size'} = 30;
	$fig->add ($text);

	if ($max_Y<$$Cat{$cat}{Y_max_host}) {
	    $max_Y = $$Cat{$cat}{Y_max_host};
	}
    }

# Legend:
    my($i)=1;
    my($color);
    foreach $color (@color_list) {
	my($min_x)=0;
	my($min_Y)=($max_Y+1)*$grid_Y_size;
	my($width)=1700;
	my($height)=$grid_Y_size;

	my($line) = new XFig ('polyline');

	$line->{'depth'} = 50;  
	$line->{'subtype'} = 1;  # line
	$line->{'points'} = [ [$min_x,$min_Y + ($i-1)*$height ],
			      [$min_x + $width,$min_Y + ($i-1)*$height],
			      [$min_x + $width,$min_Y+$height + ($i-1)*$height],
			      [$min_x,$min_Y+$height + ($i-1)*$height],
			      [$min_x,$min_Y+ ($i-1)*$height]];
	$line->{'areafill'} = 20;
	$line->{'fillcolor'} = $color;
	$fig->add ($line);

	my($text) = new XFig ('text');

	if ($color==$color_suspended) {
	    $text->{'text'} = "Suspended";
	}
	if ($color==$color_compute) {
	    $text->{'text'} = "Computing";
	}
	if ($color==$color_wait_for_recpt) {
	    $text->{'text'} = "Waiting for reception";
	}
	if ($color==$color_communicate) {
	    $text->{'text'} = "Communicating";
	}

	$text->{'y'} = $min_Y + ($i-0.5)*$height +68;
	$text->{'x'} = 50;
	$text->{'subtype'} = 0;
	$fig->add ($text);
	$i++;
    }
    
# Time axis
    my($line) = new XFig ('polyline');
    $line->{'depth'} = 0;  
    $line->{'subtype'} = 1;  # line
    $line->{'points'} = [ [0,0],[$max_date * $grid_X_size+150,0] ];
    $line->{'forwardarrow'} = ['1', '1', '1.00', '60.00', '120.00'];
    $fig->add ($line);
    
    my($digits)=POSIX::floor(log($max_date)/log(10));
    my($exponent) = 10**$digits;
    my($mantissa)= $max_date / $exponent;
    my($incr);
    if ($mantissa<2) {
	$incr = $exponent/10;
    } elsif ($mantissa<5) {
	$incr = $exponent/4;
    } else {
	$incr = $exponent/2;
    }

    print "$max_date $digits $exponent $mantissa $incr\n";
    my($x);
    for($x=0; $x < $max_date; $x += $incr) {
	print "$x\n";
	$line = new XFig ('polyline');
	$line->{'depth'} = 0;  
	$line->{'subtype'} = 1;  # line
	$line->{'points'} = [ [$x * $grid_X_size,0],[$x * $grid_X_size, -100] ];
	$line->{'forwardarrow'} = 0;
	$fig->add ($line);

	my($text) = new XFig ('text');
	$text->{'text'} = "$x";
	$text->{'y'} = -200;
	$text->{'x'} = $x * $grid_X_size;
	$text->{'subtype'} = 1;
	$fig->add ($text);
    }

# Empty line so that the text of the time axis can be seen on the pdf
    $line = new XFig ('polyline');
    $line->{'depth'} = 999;  
    $line->{'subtype'} = 1;  # line
    $line->{'thickness'} = 0;  
    $line->{'points'} = [ [0,0],[0, -400] ];
    $fig->add ($line);
}

sub main {
    my($Cat) = read_cat($ARGV[0]);
    my($cat_tree)=build_cat_tree("0",$Cat);
    read_event($ARGV[0],$Cat);
    my($Link)=read_link($ARGV[0]);
#    print Dumper($cat_tree);
#    print Dumper($Cat);
    my($cat_list)=[];
    build_cat_list($cat_tree,$cat_list);
    shift @$cat_list;
    shift @$cat_list;
#    print "@$cat_list \n";
    set_cat_position($Cat,$cat_list);
    
    my($fig)=create_fig("toto.fig");
    draw_cat($fig,$Cat,$Link);
    $fig->writefile ();
    system "fig2dev -L pdf toto.fig toto.pdf";
}

main;
