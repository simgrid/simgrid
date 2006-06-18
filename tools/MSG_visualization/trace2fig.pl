#!/usr/bin/perl -w
use strict;
use Data::Dumper;
use XFig;

my($grid_size)=225; # xfig 


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
	$$Cat{$cat}{X_min} = $i;
	$$Cat{$cat}{X_max} = $i+1;
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

sub main {
    my($Cat) = read_cat($ARGV[0]);
    my($cat_tree)=build_cat_tree("0",$Cat);
#    print Dumper($cat_tree);
    my($cat_list)=[];
    build_cat_list($cat_tree,$cat_list);
    shift @$cat_list;
    shift @$cat_list;
    print "@$cat_list \n";

    set_cat_position($Cat,$cat_list);
    
    my($fig)=create_fig("toto.fig");
    my($cat);
    foreach $cat (@$cat_list) {
	my($text) = new XFig ('text');
	$text->{'text'} = $cat;
	$text->{'y'} = ($$Cat{$cat}{X_min}+$$Cat{$cat}{X_max})/2*$grid_size+68;
	$fig->add ($text);
    }
    $fig->writefile ();
}

main;
