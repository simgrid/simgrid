#!/usr/bin/perl -w
use strict;

use graph_viz;
use graph_tbx;


##############################
###    GRAPH GENERATION    ###
##############################

sub parse_tiers_file {
    my($filename);
    my($nodes,$edges)=G_new_graph(); # a new graph...
    $filename=shift or die "Need a tiers file!";
    
    open INPUT, $filename;

    my($state) = 0;
    my($line);

    while(defined($line=<INPUT>)){
        if($line =~ /^\# NODES/) {$state=1; next;}
        if($line =~ /^\# EDGES/) {$state=2; next;}
        if(($line=~/^\s*$/) || ($line=~/^\#/)) {next;}
        if ($state==1){ # Getting the nodes
            # Node X Y Type (0 = WAN, 1 = MAN, 2 = LAN)
            my($name,$X,$Y,$type) = split(/\s+/,$line);
            G_new_node($nodes,$edges,$name);
            $$nodes{$name}{X} = $X;
            $$nodes{$name}{Y} = $Y;
            $$nodes{$name}{type} = $type;
            $$nodes{$name}{using_path} = [];
            print STDERR "$name [$X $Y $type]\n";
            next;
        }
        if ($state==2){ # Getting the edges 
            # From   To    Delay Band-  From  To    State (1:active, 2:redundant
            # Node  Node         width  Type  Type  3:internetwork, 4:red.inter.)
            my($src,$dst,$delay,$bw,$srct,$dstt,$type) = split(/\s+/,$line);
            if(!defined($$nodes{$src}{out}{$dst})) {
                # links are symetric and simple
                my($edge_count)=G_connect($nodes,$edges,$src,$dst);
                $$edges{$edge_count}{delay} = $delay;
                $$edges{$edge_count}{bw} = $bw;
                $$edges{$edge_count}{type} = $type;
                $$edges{$edge_count}{using_path} = [];
                print STDERR "($dst,$src) [$delay $bw $type]\n";
            }
            next;
        }
    }
    close(INPUT);
    return($nodes,$edges);
}

sub generate_host_list{
    my($nodes,$edges,$proba) = @_;
    my($u);
    my(@host_list)=();
    foreach $u (sort (keys %$nodes)){ # type 0 = WAN, 1 = MAN, 2 = LAN
        if(($$nodes{$u}{type}==2) && (rand() < $proba)) { 
            $$nodes{$u}{host}=1;
            push @host_list, $u;
        } else { 
            $$nodes{$u}{host}=0;
        }
    }
    @host_list = sort {$a <=> $b} @host_list;
    return(\@host_list);
}

1;
