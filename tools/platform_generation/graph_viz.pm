#!/usr/bin/perl -w
use strict;

#################################
###    GRAPH VISUALISATION    ###
#################################

sub graph2viz{
    my($nodes,$edges,$filename) = @_;

    open VIZ, "> $filename.dot";
    print VIZ "graph essai  { \n";
    print VIZ "  graph [overlap=scale]\n";
    #print VIZ "  graph [overlap=false spline=true]\n";
    print VIZ "  node [shape=box, style=filled]\n";
    print VIZ "  node [width=.3, height=.3, style=filled, color=skyblue]\n";

    my($u,$e);
    foreach $u (sort (keys %$nodes)){
	print VIZ "  \"$u\" [label=\"$u\"";
#	if((defined($$nodes{$u}{max_interf}))&&($$nodes{$u}{max_interf}==1)) {print VIZ ",color=green";}
	if(defined($$nodes{$u}{host}) && ($$nodes{$u}{host}==1)) {print VIZ ",color=red";}
	print VIZ "];\n";
    }

    foreach $e (sort (keys %$edges)){
	my($src)=$$edges{$e}{src};
	my($dst)=$$edges{$e}{dst};
#	my($len)=log(10000/$$edges{$e}{bw});
	print VIZ "  \"$src\" -- \"$dst\" [";
#	print VIZ "len=$len";
#	print VIZ "label=$e";
	if($#{$$edges{$e}{using_path}}>=0) {
	    print VIZ "color=red";
	} else {
	    print VIZ "color=gray";
	}
	print VIZ "];\n";
    }

    print VIZ "}\n";
    close VIZ;

    system("neato -Tps $filename.dot  > $filename.ps");
#    system("gv $filename.ps");
    system("pstoedit -f fig $filename.ps $filename.fig 2>/dev/null");
#    system("xfig -startg 0 $filename.fig ");
}

sub digraph2viz{
    my($nodes,$edges,$filename) = @_;

    open VIZ, "> $filename.dot";
    print VIZ "digraph essai  { \n";
    print VIZ "  graph [overlap=scale]\n";
    #print VIZ "  graph [overlap=false spline=true]\n";
    print VIZ "  node [shape=box, style=filled]\n";
    print VIZ "  node [width=.3, height=.3, style=filled, color=skyblue]\n";

    my($u,$e);
    foreach $u (sort (keys %$nodes)){
	print VIZ "  $u [label=$u";
	if((defined($$nodes{$u}{host}))&&($$nodes{$u}{host}==1)) {print VIZ ",color=red";}
	print VIZ "];\n";
    }

    foreach $e (sort (keys %$edges)){
	my($src)=$$edges{$e}{src};
	my($dst)=$$edges{$e}{dst};
#	my($len)=log(10000/$$edges{$e}{bw});
	print VIZ "  $src -> $dst\n";
#	print VIZ "len=$len";
#	print VIZ "label=$e";
    }

    print VIZ "}\n";
    close VIZ;

    system("dot -Tps $filename.dot  > $filename.ps");
#    system("gv $filename.ps");
    system("pstoedit -f fig $filename.ps $filename.fig 2>/dev/null");
    system("xfig -startg 0 $filename.fig ");
}

sub graph2fig_basic{
    my($nodes,$edges,$filename) = @_;

    open VIZ, "> $filename.fig";

    print VIZ "#FIG 3.2\n";
    print VIZ "Landscape\n";
    print VIZ "Center\n";
    print VIZ "Metric\n";
    print VIZ "A4\n";
    print VIZ "100.00\n";
    print VIZ "Single\n";
    print VIZ "-2\n";
    print VIZ "1200 2\n";

    my($scale)=5;

    my($u,$e);
    foreach $u (sort (keys %$nodes)){
	my($x) = $scale * $$nodes{$u}{X};
	my($y) = $scale * $$nodes{$u}{Y};
	print VIZ "4 1 0 50 -1 0 12 0.0000 0 135 270 $x $y $u\\001\n";
    }

    foreach $e (sort (keys %$edges)){
	my($src)=$$edges{$e}{src};
	my($dst)=$$edges{$e}{dst};
	my($x1) = $scale * $$nodes{$src}{X};
	my($y1) = $scale * $$nodes{$src}{Y};
	my($x2) = $scale * $$nodes{$dst}{X};
	my($y2) = $scale * $$nodes{$dst}{Y};
	print VIZ "2 1 0 1 4 7 50 -1 -1 0.000 0 0 -1 0 0 2\n";
	print VIZ "\t $x1 $y1 $x2 $y2\n";
    }
    close VIZ;

    system("xfig -startg 0 $filename.fig ");
}

sub interfgraph2viz{
    my($nodes,$edges,$count_interferences,$value_interferences,$figbase,$filename) = @_;

    system("cp $figbase.fig $filename.fig");
    open FIG, ">> $filename.fig";
    my($e);
    foreach $e (@{$$count_interferences{$value_interferences}}){
	my($src)=$$e[0];
	my($dst)=$$e[1];
	my(@tab);
	print STDERR "($src - $dst)\n";
	$src = `grep ' $src\\\\001' $filename.fig`;
	$dst = `grep ' $dst\\\\001' $filename.fig`;
	@tab = split(/\s+/,$src);
	my($x1) = $tab[11];
	my($y1) = $tab[12];
	@tab = split(/\s+/,$dst);
	my($x2) = $tab[11];
	my($y2) = $tab[12];
	print FIG "2 1 0 3 14 7 999 -1 -1 0.000 0 0 -1 0 0 2\n";
	print FIG "\t $x1 $y1 $x2 $y2\n";
#	print VIZ "  $src -- $dst [color=green];\n";
    }
    close(FIG);
#    system("xfig -startg 0 $filename.fig ");
    system("fig2dev -L png $filename.fig $filename.png");
    unlink("$filename.fig");
}


sub graph_get_layout{
    my($nodes,$edges,$filename) = @_;

    graph2viz($nodes,$edges,$filename);
    
    my($u);
    foreach $u (sort (keys %$nodes)){
	my($val_u) = `grep ' $u\\\\001' $filename.fig`;
	my(@tab) = split(/\s+/,$val_u);
	my($x1) = $tab[11];
	my($y1) = $tab[12];
	$$nodes{$u}{X}=$x1;
	$$nodes{$u}{Y}=$y1;
    }
}

sub graph2viz_withlayout{
    my($nodes,$edges,$filename) = @_;

    open FIG, ">> $filename.fig";
    my($e,$u);
    foreach $u (sort (keys %$nodes)){
	my($x1) = int ($$nodes{$u}{X});
	my($y1) = int ($$nodes{$u}{Y});
	print FIG "1 3 0 1 0 31 50 -1 20 0.000 1 0.0000 $x1 $y1 75 75 $x1 $y1 ".($x1+75)." $y1\n";
    }
    foreach $e (sort (keys %$edges)){
	my($src)=$$edges{$e}{src};
	my($dst)=$$edges{$e}{dst};
	my($x1) = int ($$nodes{$src}{X});
	my($y1) = int ($$nodes{$src}{Y});
	my($x2) = int ($$nodes{$dst}{X});
	my($y2) = int ($$nodes{$dst}{Y});
	print FIG "2 1 0 3 14 7 60 -1 -1 0.000 0 0 -1 0 0 2\n";
	print FIG "\t $x1 $y1 $x2 $y2\n";
    }
    close(FIG);
#    system("xfig -startg 0 $filename.fig ");
#    system("fig2dev -L png $filename.fig $filename.png");
}

sub print_matrix{
    my($M) = shift;

    if(!defined($M)) {return;}
    my($u,$v);

    my(@node_list) = sort (keys %$M);
    my($n) =  scalar(@node_list);

    foreach $u (0..$n-1){
        print "==";
    }
    print "\n";

    foreach $u (@node_list){
	if(defined($$M{$u})) {
	    foreach $v (@node_list){
		if(defined($$M{$u}{$v})){
		    print "$$M{$u}{$v} ";
		} else {
		    print "  ";
		}
	    }
	}
        print "\n";
    }
    foreach $u (0..$n-1){
        print "==";
    }
    print "\n\n";
}

sub matrix2viz{
    my($M) = shift;
    my($filename) = "foo";
    open VIZ, "> $filename.dot";

    print VIZ "# Brite topology translated by Alvin\n";
    print VIZ "graph essai  { \n";
    print VIZ "  graph [overlap=scale]\n";

    #print VIZ "  graph [overlap=false spline=true]\n";
    print VIZ "  node [shape=box, style=filled]\n";
    print VIZ "  node [width=.3, height=.3, style=filled, color=skyblue]\n";
    my(@node_list) = sort (keys %$M);
    my($n) =  scalar(@node_list);

    my($u,$v);
    foreach $u (@node_list){
	print VIZ "  $u [label=$u, color = red];\n";
    }

    foreach $u (@node_list){
        foreach $v (@node_list){
            if($$M{$u}{$v}!=0){
                print VIZ "  $u -- $v ;\n";
            }
        }
    }
    print VIZ "}\n";
    close VIZ;

    system("neato -Tps $filename.dot  > $filename.ps");
    system("gv $filename.ps");
}


sub print_object {
    my($M) = @_;
    my($key);
    if(!defined($M)) {
    } elsif (!ref($M)) {
	print "\"$M\"";
	return;
    } elsif (ref($M) eq "SCALAR") {
	print "\"$$M\"";
	return;
    } elsif (ref($M) eq "HASH") {
	print "{";
	foreach $key (sort (keys %{$M})) {
	    if($key eq "number_hops") {next;}
	    if($key eq "shortest_route") {next;}
	    if($key eq "using_path") {next;}
	    print "\"$key\" => ";
	    print_object($$M{$key});
	    print ","
	}
	print "}";
    } elsif (ref($M) eq "ARRAY") {
	print "[";
	foreach $key (0..$#$M) {
	    print_object($$M[$key]);
	    print ","
	}
	print "]";
    } else {
    }
}

# sub print_object{
#     my($indent,$M) = @_;
#     my($key);
#     if (!ref($M)) {
# 	print "$M";
# 	return;
#     } elsif (ref($M) eq "SCALAR") {
# 	print "$$M";
# 	return;
#     } elsif (ref($M) eq "HASH") {
# 	print "\n";
# 	foreach $key (sort (keys %{$M})) {
# 	    print "$indent $key => { ";
# 	    print_object($indent."  ", $$M{$key});
# 	    print "$indent }\n"
# 	}
#     } elsif (ref($M) eq "ARRAY") {
# 	print "\n";
# 	foreach $key (0..$#$M) {
# 	    print "$indent $key : [";
# 	    print_object($indent."  ", $$M[$key]);
# 	    print "$indent ]\n"
# 	}
#     } else {
#     }
# }

1;
