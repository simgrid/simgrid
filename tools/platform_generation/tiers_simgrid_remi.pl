#!/usr/bin/perl -w
use strict;

use AlvinMisc;
use graph_viz;
use graph_tbx;
use tiers;

my (%machine);
$machine{"canaria.ens-lyon.fr"}{"CPU_clock"} = "347.669";
$machine{"canaria.ens-lyon.fr"}{"CPU_Mflops"} = "34.333";
$machine{"canaria.ens-lyon.fr"}{"CPU_model"} = "Pentium II (Deschutes)";
$machine{"canaria.ens-lyon.fr"}{"CPU_num"} = "2";
$machine{"canaria.ens-lyon.fr"}{"Machine_type"} = "i686";
$machine{"canaria.ens-lyon.fr"}{"OS_version"} = "Linux 2.2.19pre17";

$machine{"moby.ens-lyon.fr"}{"CPU_clock"} = "996.698";
$machine{"moby.ens-lyon.fr"}{"CPU_Mflops"} = "114.444";
$machine{"moby.ens-lyon.fr"}{"CPU_model"} = "Intel(R) Pentium(R) III Mobile CPU 1000MHz";
$machine{"moby.ens-lyon.fr"}{"CPU_num"} = "1";
$machine{"moby.ens-lyon.fr"}{"Machine_type"} = "i686";
$machine{"moby.ens-lyon.fr"}{"OS_version"} = "Linux 2.4.18-accelerated";

$machine{"popc0.ens-lyon.fr"}{"CPU_clock"} = "199.095";
$machine{"popc0.ens-lyon.fr"}{"CPU_Mflops"} = "22.151";
$machine{"popc0.ens-lyon.fr"}{"CPU_model"} = "Pentium Pro";
$machine{"popc0.ens-lyon.fr"}{"CPU_num"} = "1";
$machine{"popc0.ens-lyon.fr"}{"Machine_type"} = "i686";
$machine{"popc0.ens-lyon.fr"}{"OS_version"} = "Linux 2.2.19";

$machine{"sci0.ens-lyon.fr"}{"CPU_clock"} = "451.032446";
$machine{"sci0.ens-lyon.fr"}{"CPU_Mflops"} = "48.492";
$machine{"sci0.ens-lyon.fr"}{"CPU_model"} = "Pentium II (Deschutes)";
$machine{"sci0.ens-lyon.fr"}{"CPU_num"} = "2";
$machine{"sci0.ens-lyon.fr"}{"Machine_type"} = "i686";
$machine{"sci0.ens-lyon.fr"}{"OS_version"} = "Linux 2.2.13";

$machine{"lancelot.u-strasbg.fr"}{"OS_version"} = "Linux 2.2.17-21mdk";
$machine{"lancelot.u-strasbg.fr"}{"CPU_clock"} = "400.916";
$machine{"lancelot.u-strasbg.fr"}{"CPU_model"} = "Celeron (Mendocino)";
$machine{"lancelot.u-strasbg.fr"}{"CPU_Mflops"} = "42.917000000000002";
$machine{"lancelot.u-strasbg.fr"}{"Machine_type"} = "i686";
$machine{"lancelot.u-strasbg.fr"}{"CPU_num"} = "1";

$machine{"caseb.u-strasbg.fr"}{"OS_version"} = "Linux 2.4.8-26mdk";
$machine{"caseb.u-strasbg.fr"}{"CPU_clock"} = "1399.763";
$machine{"caseb.u-strasbg.fr"}{"CPU_model"} = "AMD Athlon(tm) 4 Processor";
$machine{"caseb.u-strasbg.fr"}{"CPU_Mflops"} = "137.333";
$machine{"caseb.u-strasbg.fr"}{"Machine_type"} = "i686";
$machine{"caseb.u-strasbg.fr"}{"CPU_num"} = "1";

$machine{"pellinore.u-strasbg.fr"}{"OS_version"} = "Linux 2.4.18-6mdksmp";
$machine{"pellinore.u-strasbg.fr"}{"CPU_clock"} = "802.922";
$machine{"pellinore.u-strasbg.fr"}{"CPU_model"} = "Pentium III (Coppermine)";
$machine{"pellinore.u-strasbg.fr"}{"CPU_Mflops"} = "68.667000000000002";
$machine{"pellinore.u-strasbg.fr"}{"Machine_type"} = "i686";
$machine{"pellinore.u-strasbg.fr"}{"CPU_num"} = "2";

$machine{"dinadan.u-strasbg.fr"}{"OS_version"} = "Linux 2.4.19-686";
$machine{"dinadan.u-strasbg.fr"}{"CPU_clock"} = "933.375";
$machine{"dinadan.u-strasbg.fr"}{"CPU_model"} = "Pentium III (Coppermine)";
$machine{"dinadan.u-strasbg.fr"}{"CPU_Mflops"} = "85.832999999999998";
$machine{"dinadan.u-strasbg.fr"}{"Machine_type"} = "i686";
$machine{"dinadan.u-strasbg.fr"}{"CPU_num"} = "1";

$machine{"darjeeling.u-strasbg.fr"}{"OS_version"} = "Linux 2.4.18-5";
$machine{"darjeeling.u-strasbg.fr"}{"CPU_clock"} = "1793.371";
$machine{"darjeeling.u-strasbg.fr"}{"CPU_model"} = "Intel(R) Pentium(R) 4 CPU 1.80GHz";
$machine{"darjeeling.u-strasbg.fr"}{"CPU_Mflops"} = "98.094999999999999";
$machine{"darjeeling.u-strasbg.fr"}{"Machine_type"} = "i686";
$machine{"darjeeling.u-strasbg.fr"}{"CPU_num"} = "1";

$machine{"gauvain.u-strasbg.fr"}{"OS_version"} = "Linux 2.4.17";
$machine{"gauvain.u-strasbg.fr"}{"CPU_clock"} = "1050.034";
$machine{"gauvain.u-strasbg.fr"}{"CPU_model"} = "AMD Athlon(tm) Processor";
$machine{"gauvain.u-strasbg.fr"}{"CPU_Mflops"} = "114.444";
$machine{"gauvain.u-strasbg.fr"}{"Machine_type"} = "i686";
$machine{"gauvain.u-strasbg.fr"}{"CPU_num"} = "1";

$machine{"sekhmet.u-strasbg.fr"}{"OS_version"} = "Linux 2.4.18-k7";
$machine{"sekhmet.u-strasbg.fr"}{"CPU_clock"} = "1399.803";
$machine{"sekhmet.u-strasbg.fr"}{"CPU_model"} = "AMD Athlon(tm) 4 Processor";
$machine{"sekhmet.u-strasbg.fr"}{"CPU_Mflops"} = "171.667";
$machine{"sekhmet.u-strasbg.fr"}{"Machine_type"} = "i686";
$machine{"sekhmet.u-strasbg.fr"}{"CPU_num"} = "1";

$machine{"shaitan.u-strasbg.fr"}{"OS_version"} = "Linux 2.4.18-6mdk";
$machine{"shaitan.u-strasbg.fr"}{"CPU_clock"} = "800.030";
$machine{"shaitan.u-strasbg.fr"}{"CPU_model"} = "Pentium III (Coppermine)";
$machine{"shaitan.u-strasbg.fr"}{"CPU_Mflops"} = "76.296000000000006";
$machine{"shaitan.u-strasbg.fr"}{"Machine_type"} = "i686";
$machine{"shaitan.u-strasbg.fr"}{"CPU_num"} = "1";

my (%network);
#0.587500,0.655,0.681

$network{"bw"}{10} = [ [274285,0.514433], [330233,0.059904],
                [949460,0.136931], [1063823,0.131098],
                [2041829,7.413073] ];

$network{"bw"}{100} = [ [64121,35.076518], [65264,0.270544],
                [67418,0.156056], [80797,0.479780], [82517,6.932556],
                [92951,0.189980], [94763,0.370788],
                [123015,35.083019], [171318,295.890617],
                [223570,0.278066], [274285,0.514433],
                [330233,0.059904] ];

$network{"bw"}{1000} = [ [937,53.930106], [2013,4.455826],
                [2022,5.704550], [2025,5.652577], [2073,4.460898],
                [2179,5.922616], [2195,4.669142], [2321,4.522355],
                [2327,4.477270], [2427,4.062241], [2539,4.583831],
                [3777,5.161451], [4448,3.101854], [4629,5.473705],
                [4840,87.981858], [5773,0.006406], [6150,8.762440],
                [7413,0.601375], [7837,0.424305], [7867,2.885584],
                [7924,1.742977], [8394,9.647856], [9015,0.287840],
                [9612,0.468130], [9842,1.502106], [10069,1.340162],
                [10255,6.104672], [10609,1.402769], [11014,0.449267],
                [11724,0.863872], [11741,0.869727], [11753,1.114548],
                [12100,1.200141], [12122,0.844683], [12513,0.788956],
                [13022,0.278175], [14341,7.877863], [14864,0.820952],
                [15084,0.950938], [15111,1.081287], [15141,0.162735],
                [15449,0.951830], [15797,0.380044], [15868,0.848211],
                [17433,0.320114], [17819,0.907120], [17906,1.043314],
                [18382,1.087968], [18788,0.259761], [18944,9.547561],
                [20667,0.410463], [20864,0.637001], [22546,0.247605],
                [24227,0.677908], [24547,0.040300], [25404,0.472524],
                [26205,0.658142], [26382,0.595883], [26970,0.666676],
                [27441,0.536941], [28416,3.870785], [29714,3.866813],
                [31020,0.863123], [31452,1.913591], [31964,0.678645],
                [33067,9.693542], [33378,0.728103], [34162,0.672289],
                [34363,0.539000], [35178,0.677601], [35333,0.019773],
                [35689,0.106949], [35881,0.126045], [37202,0.705967],
                [37438,0.848712], [38536,0.117352], [38723,0.751810],
                [39826,7.164412], [41518,0.630529], [41827,0.039417],
                [42392,0.520693], [43729,0.272268], [44597,0.227430],
                [45776,0.789218], [46068,4.760145], [46531,0.164758],
                [52408,0.522878], [54216,0.533340], [57678,1.461517],
                [60272,0.125428] ];

sub assign_host_speed{
    my($nodes,$edges) = @_;

    my($u);

    my(@label_list) = keys %machine;
    foreach $u (keys %$nodes) {
	my($mach_type_nb) = scalar(@label_list);
	my($mach_type) = int rand($mach_type_nb);
	$$nodes{$u}{Mflops} = $machine{$label_list[$mach_type]}{CPU_Mflops};
    }
}

sub assign_host_names{
    my($nodes,$edges) = @_;

    my(@name_list) = qw(Abbott Adoncourt Aikin Alain Alfred Amadeus
                        Angie Anjou Anne_Marie Apple April Archibald
                        Aubertin Auclair Audy AutoCAD Barry BASIC
                        Beaudoin Beaulac Bellemarre Bellevue
                        Bell_Northern Benoit Bentz Bernard Bescherelle
                        Blais Boily Boivin Borduas Boston Boucherville
                        Bourassa Bousquet Boyer Brian Brosseau Brown
                        Browne Cadieux Cambridge Canada Carole
                        Casavant Chambly Charles Charron Christian
                        Claude Cloutier Colin Comeau Corp Coulombe
                        Cousineau Croteau Daniel Decelles Denis Denise
                        Desjardins Dick Dionne Disney Dodge Domey
                        Dominique Doris Dorval Doyon Drouin Dumoulin
                        EDF Emacs Ethernet Europe Fafard Fernand
                        Fernet Flamand Florient Foisy Forget Fourier
                        FrameMaker France Francine Frank Fraser
                        Freedman Gagnon Gaston Gatien Gaudreault
                        Gauthier Gavrel Gendron Gentilly Geoff
                        Geoffray George Georges Gilles Ginette Girard
                        Goodwin Gordon Gosselin Gratton Greg Gregory
                        Guy Harry Harwell Hayward Hollerbach Horne
                        Houde Hubert Hz Inc Inmos Intel Interleaf
                        Internet iRMX iRMXII iRMXIII Isabelle ISPELL
                        Jackson Jacobsen Jacquelin Jacques
                        Jacques_Cartier Jamie Jean Jean_Claude
                        Jean_Louis Jean_Maurice Jeannine Jean_Paul
                        Jean_Pierre Jean_Yves Jill Jobin Jocelyne John
                        Jones Joynes Jude Julian Julien Juneau Jupiter
                        Kansas Kent Khan King Kuenning kV Lachapelle
                        Laflamme Lafontaine Lamothe Lapointe Laroche
                        LaSalle LaTeX Laugier Laurendeau Laval Lavoie
                        Leblanc Lecavalier Leclerc Lepage Lessard
                        Letarte Linda LISP Longueuil Louis Louise
                        Louis_Marc Ltd Lucie Mahoney Maltais Manseau
                        Marc Marcel Marcoux Marie Marielle Mark
                        Marseille Martin Masson Mathematica Matlab
                        McGee McGill Messier METAFONT Michel Mike
                        Minneapolis Mireille Mongenot Monique
                        Mont_Tremblant Morin Morissette Moshe Mulhouse
                        mW Nagle Nelligan Nestor Nicole OHara Olivier
                        Ontario Ottawa Ouellet Owen Ozias Papineau
                        Paul Pellan Pelletier PERL Phaneuf Phil Pierre
                        Pierrefonds Plante Pointe_Claire PostScript
                        Poussart Pronovost Provost Raymond Re README
                        Renato Ricard Richard Ringuet Riopelle Rioux
                        Roberge Robert Rochefort Roger Romano Ronald
                        Roy Rubin Sacramento Saint_Amand Sainte_Foy
                        Sainte_Julie Saint_Marc_sur_Richelieu Seattle
                        Shawinigan Sherbrooke Sirois Smith Sorel Soucy
                        SPARC SPARCs SPICE St_Antoine St_Bruno
                        Ste_Anne Steele Ste_Julie Stephen St_Jacques
                        St_Jean St_Paul Strasbourg Sun SunOS Suzanne
                        Tanguay Tessier TeX Texas Thibault Thierry
                        Todd Tokyo Toronto Toulouse Tremblay Turcotte
                        Uintas UniPress Unix UNIX Utah Vancouver
                        Varennes Verville Victoria Victoriaville Viger
                        Vincent VxWorks Wilfrid William Williams
                        Wright Yolande Yvan Yves Zawinski);

    AlvinMisc::melange(\@name_list);

    my($u);
    foreach $u (keys %$nodes) {
	if($$nodes{$u}{host}==1) {
	    $$nodes{$u}{name} = shift @name_list;
	}
    }
}

sub assign_link_speed{
    my($nodes,$edges) = @_;

    my($e);

    foreach $e (keys %$edges) {
	my($bw) = $$edges{$e}{bw};

	my(@choice) = @{$network{"bw"}{$bw}};
	my($choice_nb) = scalar(@choice);
	my($chosen_link) = $choice[(int rand($choice_nb))];
	$$edges{$e}{bw} = $$chosen_link[0]/8000;
	$$edges{$e}{delay} = $$chosen_link[1];
    }
}


sub affiche_graph{
    my($nodes,$edges,$filename) = @_;
    my($u,$v,$w,$e);
    
    my(%node_to_export);
    my(%edge_to_export);

    my(@host_list,@routeur_list,@link_list);

    foreach $u (keys %{$nodes}) {
	if (!((defined($$nodes{$u}{host}))&&($$nodes{$u}{host}==1))) { next; }
	if (!((defined($$nodes{$u}{Mflops}))&&($$nodes{$u}{host}==1))) { 
	    die "Lacking Mflops for $u\n"; 
	}
	push @host_list,$u;
	$node_to_export{$u}=$#host_list;
    }

    foreach $u (@host_list){
        foreach $v (@host_list){
	    if($u ne $v) {
		$w = $u;
		if(!defined($node_to_export{$w})) {
		    push @routeur_list,$w;
		    $node_to_export{$w}=$#host_list + $#routeur_list + 1;
		}
		while ($w ne $v) {
		    my($next) = $$nodes{$w}{shortest_route}{$v};
		    my($e) = $$nodes{$w}{out}{$next};
		    if(!defined($edge_to_export{$e})) {
			push @link_list,$e;
			$edge_to_export{$e}=$#link_list;
		    }
		    $w = $next;
		    if(!defined($node_to_export{$w})) {
			push @routeur_list,$w;
			$node_to_export{$w}=$#host_list + $#routeur_list + 1;
		    }
		}
	    }
	}
    }

    open VIZ, "> $filename.dot";
    print VIZ "graph essai  { \n";
    print VIZ "  graph [overlap=scale]\n";
    #print VIZ "  graph [overlap=false spline=true]\n";
    print VIZ "  node [shape=box, style=filled]\n";
    print VIZ "  node [width=.3, height=.3, style=filled]\n";

    foreach $u (@host_list) {
	print VIZ "  \"$u\" [label=\"$$nodes{$u}{name}\",color=red,shape=box];\n";
    }
    foreach $u (@routeur_list) {
	    print VIZ "  \"$u\" [label=\"$$nodes{$u}{name}\",fontsize=2,color=skyblue,shape=circle];\n";
    }
    foreach $e (@link_list) {
	my($src)=$$edges{$e}{src};
	my($dst)=$$edges{$e}{dst};
	(defined($$edges{$e}{bw})) or die "Lacking bw for $u\n"; 
	(defined($$edges{$e}{delay})) or die "Lacking bw for $u\n"; 
	print VIZ "  \"$src\" -- \"$dst\";\n";
    }

    print VIZ "}\n";
    close VIZ;

    system("neato -Tps $filename.dot  > $filename.ps");
#    system("gv $filename.ps");
    system("pstoedit -f fig $filename.ps $filename.fig 2>/dev/null");
#    system("xfig -startg 0 $filename.fig ");
}

sub build_lat_matrix {
    my($nodes,$edges) = @_;
    my($u,$v,$w,$e);
    my(@host_list)=();
    my($matrix);
    foreach $u (keys %{$nodes}) {
	if (!((defined($$nodes{$u}{host}))&&($$nodes{$u}{host}==1))) { next; }
	push @host_list,$u;
    }

    foreach $u (@host_list){
        foreach $v (@host_list){
	    $$matrix{$$nodes{$u}{name}}{$$nodes{$v}{name}}=0;
	    if($u ne $v) {
		$w = $u;
		while ($w ne $v) {
		    my($next) = $$nodes{$w}{shortest_route}{$v};
		    $e = $$nodes{$w}{out}{$next};
		    $$matrix{$$nodes{$u}{name}}{$$nodes{$v}{name}}+=$$edges{$e}{delay};
		    $w = $next;
		}
	    }
	    print STDERR "$$nodes{$u}{name} -> $$nodes{$v}{name} : $$matrix{$$nodes{$u}{name}}{$$nodes{$v}{name}}\n";
	}
    }
    return $matrix;
}

sub cluster_matrix {
    my($matrix)=@_;
    my($nodes,$edges) = G_new_graph();

    my(@host_list)=keys %$matrix;

    my($root);
    my($u,$v);
    my($min,$min_u,$min_v);
    my(%taken);
    foreach $u (@host_list){
	G_new_node($nodes,$edges,$u);
    }
    
    $root=$host_list[0];
    $taken{$root} = 1;

    while(scalar(keys %taken) != scalar(@host_list)) {
	$min = 10000000000000;
	foreach $u (keys %taken) {
	    foreach $v (@host_list) {
		if(!$taken{$v}) {
		    if($u ne $v) {
			if($$matrix{$u}{$v}<$min) {
			    $min=$$matrix{$u}{$v};
			    $min_u=$u;
			    $min_v=$v;
			}
		    }
		}
	    }
	}
	$taken{$min_u} = $taken{$min_v} = 1;
	G_connect($nodes,$edges,$min_u,$min_v);
	print STDERR "Connecting $min_u and $min_v\n"
    }
    return ($nodes,$edges,$root);
}

sub PP {
    my($nodes, $edges, $taken, $root) = @_;
    my($u);
    my(@tab) = ();
    push @tab, $root;
    foreach $u (keys (%{$$nodes{$root}{out}})) {
	if($$taken{$u}) { next;}
	$$taken{$u}=1;
	push(@tab,PP($nodes,$edges, $taken, $u));
    }
    return \@tab;
}

sub create_orientation {
    my($nodes,$edges,$root) = @_;
    my(%taken);

    $taken{$root} = 1;
    
    return PP($nodes,$edges, \%taken,$root);
}

sub dump_orientation {
   my($nodes,$edges, $tab) = @_;
   my($u,$father);
   my($root) = shift @$tab;

   print DEPLOYMENT '  <process host="'.$$nodes{$root}{name}.'" function="msma_agent">'."\n";
   print DEPLOYMENT '     <argument value="blabla"/>     <!-- policy -->'."\n";

   {  # Need to find my father...
       my(@sons) = ();
       my(@neighboors) = keys %{$$nodes{$root}{in}};

       $father = -1;

       foreach $u (@$tab) {
	   push @sons,$$u[0];
       }
       @sons = sort @sons;
       @neighboors = sort @neighboors;

       if($#sons != $#neighboors) {
	   $u = 0;
	   while(defined($sons[$u])&&($sons[$u] eq $neighboors[$u])) {
	       $u++;
	   }
	   $father = $neighboors[$u];
       }
   }

   if($father ne -1) {
       print DEPLOYMENT "     <argument value=\"$$nodes{$root}{name}\"/>  <!-- father -->\n";
   } else {
       print DEPLOYMENT '     <argument value=""/>  <!-- father -->'."  <!-- I am the master -->\n";
   }


   foreach $u (@$tab) {
       print DEPLOYMENT "     <argument value=\"$$nodes{$$u[0]}{name}\"/>\n";
   }
   print DEPLOYMENT "  </process>\n";
   foreach $u (@$tab) {
       dump_orientation($nodes, $edges, $u);
   }

   unshift @$tab, $root;
}

sub main {
    my($nodes,$edges,$interferences,$host_list,$count_interferences);

    $#ARGV>=0 or die "Need a tiers file!";
    my($filename)=$ARGV[0];
    $filename =~ s/\.[^\.]*$//g;
    if(1) {
        print STDERR "Reading graph\n";
        ($nodes,$edges) = parse_tiers_file $ARGV[0];

        print STDERR "Generating host list\n";
        $host_list = generate_host_list($nodes,$edges,.7);
        assign_host_speed($nodes,$edges);
        assign_link_speed($nodes,$edges);
        assign_host_names($nodes,$edges);
        print STDERR "Computing Shortest Paths \n";
        shortest_paths($nodes,$edges);
        print STDERR "Exporting to surfxml\n";
	G_surfxml_export($nodes,$edges,"$filename.xml");
        print STDERR "Calling graphviz\n";
	affiche_graph($nodes,$edges,"$filename");
	print STDERR "Getting Latency Matrix\n";
	my($matrix)=build_lat_matrix($nodes,$edges);
	print STDERR "Clustering on Latency\n";
	my($n_nodes,$n_edges,$root)=cluster_matrix($matrix);
	print STDERR "Creating Orientation\n";
	my($orientation) = create_orientation($n_nodes,$n_edges,$root);
	print STDERR "Exporting deployment\n";
	open DEPLOYMENT, "> $ARGV[0]_deployment.xml";
	dump_orientation($n_nodes,$n_edges, $orientation);
	close DEPLOYMENT;
    }
}

main;
