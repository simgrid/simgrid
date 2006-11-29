#!/usr/bin/perl -w
use strict;
#use Data::Dumper;
use XFig;

my($grid_Y_size)=225; # xfig 
my($grid_X_size)=550; # xfig 

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
	    my($event,$date,$type,$father,$channel,$src,$key) = split(/\s+/,$line);
	    $link{$key}{src}=$src;
	    $link{$key}{src_date}=$date;
	}
	if($line =~ /^17\s/) {
	    my($event,$date,$type,$father,$channel,$dst,$key) = split(/\s+/,$line);
	    $link{$key}{dst}=$dst;
	    $link{$key}{dst_date}=$date;
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


sub set_cat_position {
    my($Cat,$cat_list)=@_;
    my($i)=0;
    my($cat);
    foreach $cat (@$cat_list) {
	if(defined($$Cat{$cat}{state})) {
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
    foreach $cat (keys %$Cat) {
	next unless (defined($$Cat{$cat}{Y_min}) && 
		     defined($$Cat{$cat}{Y_max}));
	my($text) = new XFig ('text');
	$text->{'text'} = $$Cat{$cat}{name};
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

    foreach $link (keys %$Link) {
	my($line) = new XFig ('polyline');
	my($src_date)=$$Link{$link}{src_date};
	my($src)=$$Link{$link}{src};
	my($dst_date)=$$Link{$link}{dst_date};
	my($dst)=$$Link{$link}{dst};
	$line->{'subtype'} = 1;  # line

	$line->{'points'} = [ [$src_date*$grid_X_size, 
			       ($$Cat{$src}{Y_min}+$$Cat{$src}{Y_max})/2*$grid_Y_size],
			      [$dst_date*$grid_X_size, 
			       ($$Cat{$dst}{Y_min}+$$Cat{$dst}{Y_max})/2*$grid_Y_size] ];
	$line->{'forwardarrow'} = ['1', '1', '1.00', '60.00', '120.00'];
	$fig->add ($line);
    }
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
    print "@$cat_list \n";

    set_cat_position($Cat,$cat_list);
    
    my($fig)=create_fig("toto.fig");
    draw_cat($fig,$Cat,$Link);
    $fig->writefile ();
    system "fig2dev -L pdf toto.fig toto.pdf";
}

main;
