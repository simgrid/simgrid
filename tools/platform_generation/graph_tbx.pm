#!/usr/bin/perl -w
use strict;

# Un graphe c'est un hachage nodes et un hachage edges avec trucs dedans
# nodes{nom}{type}, nodes{nom}{X}, nodes{nom}{Y}, nodes{nom}{in}, nodes{nom}{out} par exemple
# edges{nom}{src}, edges{nom}{dst}, edges{nom}{type}, edges{nom}{delay}, edges{nom}{bw}


sub G_new_graph {
    my(%nodes,%edges); # a new graph...
    return(\%nodes,\%edges);
}

sub G_new_node {
    my($nodes,$edges,$name) = @_;
    $$nodes{$name}{name}=$name;
    $$nodes{$name}{type}=2;
}

sub G_connect {
    my($nodes,$edges,$src,$dst) = @_;
    my($edge_count)=scalar(keys(%{$edges}));
    $$edges{$edge_count}{src}=$src;
    $$edges{$edge_count}{dst}=$dst;
#    $$edges{$edge_count}{delay} = $delay;
#    $$edges{$edge_count}{bw} = $bw;
#    $$edges{$edge_count}{type} = $type;
#    $$edges{$edge_count}{using_path} = [];
    $$nodes{$src}{out}{$dst} = $edge_count;
    $$nodes{$dst}{in}{$src} = $edge_count;
    $$nodes{$dst}{out}{$src} = $edge_count;
    $$nodes{$src}{in}{$dst} = $edge_count;
    return($edge_count);
}

sub GO_connect {
    my($nodes,$edges,$src,$dst) = @_;
    my($edge_count)=scalar(keys(%{$edges}));
    $$edges{$edge_count}{src}=$src;
    $$edges{$edge_count}{dst}=$dst;
#    $$edges{$edge_count}{delay} = $delay;
#    $$edges{$edge_count}{bw} = $bw;
#    $$edges{$edge_count}{type} = $type;
#    $$edges{$edge_count}{using_path} = [];
    $$nodes{$src}{out}{$dst} = $edge_count;
    $$nodes{$dst}{in}{$src} = $edge_count;
    return($edge_count);
}

sub G_simgrid_export {
    my($nodes,$edges,$filename) = @_;
    my($u,$v,$w,$e);
    my(@host_list)=();
    
    open MSG_OUTPUT, "> $filename";

    print MSG_OUTPUT "HOSTS\n";
    foreach $u (keys %{$nodes}) {
	if (!((defined($$nodes{$u}{host}))&&($$nodes{$u}{host}==1))) { next; }
	if (!((defined($$nodes{$u}{Mflops}))&&($$nodes{$u}{host}==1))) { 
	    die "Lacking Mflops for $u\n"; 
	}
	print MSG_OUTPUT "$u $$nodes{$u}{Mflops}\n";
	push @host_list,$u;
    }
    print MSG_OUTPUT "LINKS\n";
    foreach $e (keys %{$edges}) {
	(defined($$edges{$e}{bw})) or die "Lacking bw for $u\n"; 
	(defined($$edges{$e}{delay})) or die "Lacking bw for $u\n"; 
	print MSG_OUTPUT "$e $$edges{$e}{bw} $$edges{$e}{delay}\n";
    }
    print MSG_OUTPUT "ROUTES\n";
    foreach $u (@host_list){
        foreach $v (@host_list){
	    if($u ne $v) {
		my(@path)=();
		$w = $u;
		while ($w ne $v) {
		    my($next) = $$nodes{$w}{shortest_route}{$v};
		    $e = $$nodes{$w}{out}{$next};
		    push(@path,$e);
		    $w = $next;
		}
		print MSG_OUTPUT "$u $v (@path)\n";
	    }
	}
    }
    close(MSG_OUTPUT);
}

sub G_surfxml_export {
    my($nodes,$edges,$filename) = @_;
    my($u,$v,$w,$e);
    my(@host_list)=();
    
    open MSG_OUTPUT, "> $filename";

    print MSG_OUTPUT "<?xml version='1.0'?>\n";
    print MSG_OUTPUT "<!DOCTYPE platform_description SYSTEM \"surfxml.dtd\">\n";
    print MSG_OUTPUT "<platform_description>\n";

    foreach $u (keys %{$nodes}) {
	if (!((defined($$nodes{$u}{host}))&&($$nodes{$u}{host}==1))) { next; }
	if (!((defined($$nodes{$u}{Mflops}))&&($$nodes{$u}{host}==1))) { 
	    die "Lacking Mflops for $u\n"; 
	}
	print MSG_OUTPUT "  <cpu name=\"$$nodes{$u}{name}\" power=\"$$nodes{$u}{Mflops}\"/>\n";
	push @host_list,$u;
    }
    foreach $e (keys %{$edges}) {
	(defined($$edges{$e}{bw})) or die "Lacking bw for $u\n"; 
	(defined($$edges{$e}{delay})) or die "Lacking bw for $u\n"; 
	my($lat);
	$lat = $$edges{$e}{delay} / 1000;
	print MSG_OUTPUT "  <network_link name=\"$e\" bandwidth=\"$$edges{$e}{bw}\" latency=\"$lat\"/>\n";
    }
    foreach $u (@host_list){
        foreach $v (@host_list){
	    if($u ne $v) {
		my(@path)=();
		$w = $u;
		while ($w ne $v) {
		    my($next) = $$nodes{$w}{shortest_route}{$v};
		    $e = $$nodes{$w}{out}{$next};
		    push(@path,$e);
		    $w = $next;
		}
		print MSG_OUTPUT "  <route src=\"$$nodes{$u}{name}\" dst=\"$$nodes{$v}{name}\">";
		foreach $w (@path) {
		    print MSG_OUTPUT "<route_element name=\"$w\"/>";
		}
		print MSG_OUTPUT "  </route>\n";
	    }
	}
    }
    print MSG_OUTPUT "</platform_description>\n";
    close(MSG_OUTPUT);
}

##############################
###    GRAPH ALGORITHMS    ###
##############################

sub shortest_paths{
    my($nodes,$edges) = @_;

    my(%connexion);
    my(%distance);
    my(%shortest);

    my(@node_list) = sort (keys %$nodes);
    my($n) =  scalar(@node_list);

    my($u,$v,$w,$step,$e);

    foreach $u (@node_list){
        foreach $v (@node_list){
            $connexion{$u}{$v} = 0;
            $distance{$u}{$v} = 0;
            $shortest{$u}{$v} = -1;
        }
    }

    foreach $e (sort (keys %$edges)){
	my($x1)=$$edges{$e}{src};
	my($x2)=$$edges{$e}{dst};
        $connexion{$x1}{$x2} = $connexion{$x2}{$x1} = 1;
        $distance{$x1}{$x2} = $distance{$x2}{$x1} = 1;
        $shortest{$x1}{$x2} = $x2;
        $shortest{$x2}{$x1} = $x1;
    }

#    print_matrix(\%connexion);
#    matrix2viz(\%connexion);

    foreach $step (0..$n-1){
	my($modif) = 0;
        foreach $u (@node_list){
            foreach $v (@node_list){
                foreach $w (@node_list){
                    if(($connexion{$u}{$w} != 0)
                        && ($distance{$w}{$v}!=0)
                        && (($distance{$u}{$v} >
			     $distance{$u}{$w} + $distance{$w}{$v}) || ($distance{$u}{$v}==0))
		       ){
                        $distance{$u}{$v} =
			    $distance{$u}{$w} + $distance{$w}{$v};
			$shortest{$u}{$v} = $w;
			$modif = 1;
                    }
                }
            }
        }
	if($modif == 0) {last;}
    }

    foreach $u (@node_list){
	foreach $v (@node_list){
	    if($u eq $v) {
		$$nodes{$u}{shortest_route}{$v} = $u;
		$$nodes{$u}{number_hops}{$v} = 0;
	    } else {
		$$nodes{$u}{shortest_route}{$v} = $shortest{$u}{$v};
		$$nodes{$u}{number_hops}{$v} = $distance{$u}{$v};
	    }
	}
    }
}

sub build_interferences{
    my($nodes,$edges,$host_list) = @_;

    my($u,$v,$w,$e);
    my(%interference);

    foreach $u (@$host_list){
        foreach $v (@$host_list){
	    if($u ne $v) {
		$w = $u;
		push(@{ $$nodes{$u}{using_path}},[$u,$v]);
		while ($w ne $v) {
		    my($next) = $$nodes{$w}{shortest_route}{$v};
		    $e = $$nodes{$w}{out}{$next};
		    push(@{ $$edges{$e}{using_path}},[$u,$v]);
		    push(@{ $$nodes{$next}{using_path}},[$u,$v]);
		    $w = $next;
		}
	    }
	}
    }
#     foreach $e (keys %$edges){
# 	my($e1,$e2);
# 	foreach $e1 (@{$$edges{$e}{using_path}}) {
# 	    my($p1,$q1) = @$e1;
# 	    foreach $e2 (@{$$edges{$e}{using_path}}) {
# 		my($p2,$q2) = @$e2;
# 		$interference{$p1}{$p2}{$q1}{$q2} = 1;
# 	    }
# 	}	
#     }
    my($p1,$p2,$q1,$q2);

    foreach $p1 (@$host_list) {
        foreach $p2 (@$host_list) {
            foreach $q1 (@$host_list) {
                foreach $q2 (@$host_list) {
                    $interference{$p1}{$p2}{$q1}{$q2}=0;
                }
            }
        }
    }

    foreach $e (keys %$nodes){
	my($e1,$e2);
	foreach $e1 (@{$$nodes{$e}{using_path}}) {
	    my($p1,$q1) = @$e1;
	    foreach $e2 (@{$$nodes{$e}{using_path}}) {
		my($p2,$q2) = @$e2;
		$interference{$p1}{$p2}{$q1}{$q2} = 1;
	    }
	}	
    }

    foreach $e (keys %$nodes){
	undef(@{$$nodes{$e}{using_path}});
    }

    foreach $e (keys %$edges){
	undef(@{$$edges{$e}{using_path}});
    }

#    foreach $u (@host_list){
#        foreach $v (@host_list){
#	    if((defined($interference[$u]))&&(defined($interference[$u][$v]))) {
#		print_matrix($interference[$u][$v]);
#	    }
#	}
#    }
    return(\%interference);
}

sub __visit_pp {
    my($nodes,$edges,$u,$time,$direction,$stamp) = @_;
    my($v);
    $$nodes{$u}{"Couleur_$stamp"}=1;
    $$time++;
    $$nodes{$u}{"Debut_$stamp"}=$$time;
    foreach $v (keys (%{$$nodes{$u}{$direction}})) {
	if ($$nodes{$v}{"Couleur_$stamp"} == 0) {
	    $$nodes{$v}{"PI_$stamp"} = $u;
	    __visit_pp($nodes,$edges,$v,$time,$direction,$stamp);
	}
    }
    $$nodes{$u}{"Couleur_$stamp"}=2;
    $$time++;
    $$nodes{$u}{"Fin_$stamp"}=$$time;
}

sub __PP {
    my($nodes,$edges,$direction,$stamp,$stampSorter) = @_;

    my(@node_list) = (keys %$nodes);
    if(defined($stampSorter)) {
	@node_list = sort {
	    $$nodes{$b}{$stampSorter}
                       <=>
	    $$nodes{$a}{$stampSorter}
	} @node_list;
    }

    my($u,$time);

    foreach $u (@node_list) {
	$$nodes{$u}{"Couleur_$stamp"} = 0;
    }
    $time = 0;
    foreach $u (@node_list) {
	if($$nodes{$u}{"Couleur_$stamp"} == 0) {
	    __visit_pp($nodes,$edges,$u,\$time,$direction,$stamp);
	}
    }
    return $time;
}


sub GO_SCC_Topological_Sort{
    my($nodes,$edges) = @_;

    my(@node_list) = (keys %$nodes);

    ### Strongly Connected Components building
    __PP($nodes,$edges,"out","1");
    __PP($nodes,$edges,"in","2","Fin_1");

    @node_list = sort 
    {
	if ($$nodes{$a}{"Fin_2"}<$$nodes{$b}{"Debut_2"}) {
	    return -1;
	} elsif ($$nodes{$a}{"Debut_2"}<$$nodes{$b}{"Debut_2"}) {
	    return -1;
	} 
	return 1;
    }
    @node_list;

    my($u,$v);
    my($scc)=$node_list[0];
    my(%SCC);
    foreach $u (@node_list) {
	if($$nodes{$u}{Fin_2} > $$nodes{$scc}{Fin_2}) {
	    $scc = $u;
	}
	push @{$SCC{$scc}},$u;
	$$nodes{$u}{SCC_leader}=$scc;
    }

    ### Topological Sort
    my($n_SCC,$e_SCC)=G_new_graph();
    foreach $scc (keys %SCC) {
	G_new_node($n_SCC,$e_SCC,$$nodes{$scc}{SCC_leader});
	foreach $u (@{$SCC{$scc}}) {
	    foreach $v (keys (%{$$nodes{$u}{out}})) {
		if(!defined($$n_SCC{$$nodes{$u}{SCC_leader}}{out}{$$nodes{$v}{SCC_leader}})) {
		    GO_connect($n_SCC,$e_SCC,$$nodes{$u}{SCC_leader},$$nodes{$v}{SCC_leader});
		}
	    }
	}
    }

    __PP($n_SCC,$e_SCC,"out","TS");
    my(@SCC_list) = keys %SCC;
    @SCC_list = sort {$$n_SCC{$b}{Fin_TS} <=> $$n_SCC{$a}{Fin_TS}} @SCC_list;
    my(@ordering)=();
    foreach $scc (@SCC_list) {
	push @ordering, $SCC{$scc};
    }

    return \@ordering;
}


1;
