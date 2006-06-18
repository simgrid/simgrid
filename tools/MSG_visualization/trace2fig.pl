#!/usr/bin/perl -w
use strict;
use Data::Dumper;
use XFig;

my($grid_Y_size)=225; # xfig 
my($grid_X_size)=225; # xfig 

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


sub set_cat_position {
    my($Cat,$cat_list)=@_;
    my($i)=0;
    my($cat);
    foreach $cat (@$cat_list) {
	$$Cat{$cat}{Y_min} = $i;
	$$Cat{$cat}{Y_max} = $i+1;
	$i++;
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
    my($fig,$Cat)=@_;
    my($cat,$e);
    foreach $cat (keys %$Cat) {
	next unless (defined($$Cat{$cat}{Y_min}) && 
		     defined($$Cat{$cat}{Y_max}));
	my($text) = new XFig ('text');
	$text->{'text'} = $cat;
	$text->{'y'} = ($$Cat{$cat}{Y_min}+$$Cat{$cat}{Y_max})/2*$grid_Y_size+68;
	$fig->add ($text);
    }
    foreach $cat (keys %$Cat) {
	next unless (defined($$Cat{$cat}{Y_min}) && 
		     defined($$Cat{$cat}{Y_max}));
	my(@states)=();
	my($e);
	foreach $e (@{$$Cat{$cat}{state}}) {
	    my($new_date,$state) = ($$e[0],$$e[1]);
	    if(defined($state)) {
		push @states, $e;
	    } else {
		my($old_event) = pop @states;
		my($old_date) = $$old_event[0];
		$state = $$old_event[1];
		
		my($line) = new XFig ('polyline');

		$line->{'subtype'} = 1;  # line
		$line->{'points'} = [ [$old_date*$grid_X_size, $$Cat{$cat}{Y_min}*$grid_Y_size],
				      [$new_date*$grid_X_size, $$Cat{$cat}{Y_min}*$grid_Y_size],
				      [$new_date*$grid_X_size, $$Cat{$cat}{Y_max}*$grid_Y_size],
				      [$old_date*$grid_X_size, $$Cat{$cat}{Y_max}*$grid_Y_size] ];
		$line->{'areafill'} = 20;
		if($state eq "S") {
		    $line->{'fillcolor'} = 1;
		} elsif ($state eq "E") {
		    $line->{'fillcolor'} = 2;
		} elsif ($state eq "B") {
		    $line->{'fillcolor'} = 3;
		} elsif ($state eq "C") {
		    $line->{'fillcolor'} = 4;
		}
		$fig->add ($line);
	    }
	}
    }
}

sub main {
    my($Cat) = read_cat($ARGV[0]);
    my($cat_tree)=build_cat_tree("0",$Cat);
    read_event($ARGV[0],$Cat);
#    print Dumper($cat_tree);
#    print Dumper($Cat);
    my($cat_list)=[];
    build_cat_list($cat_tree,$cat_list);
    shift @$cat_list;
    shift @$cat_list;
    print "@$cat_list \n";

    set_cat_position($Cat,$cat_list);
    
    my($fig)=create_fig("toto.fig");
    draw_cat($fig,$Cat);
    $fig->writefile ();
    system "fig2dev -L pdf toto.fig toto.pdf";
}

main;
