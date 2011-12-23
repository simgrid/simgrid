#!/usr/bin/perl -w
use strict;
use Switch;
my $toversion=3;

if($#ARGV!=1) {
    die "Usage: ./generate_g5k_platform.pl g5k_username g5k_password\n";
}

print "<?xml version='1.0'?>\n";
print "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid.dtd\">\n";
print "<platform version=\"$toversion\">\n";
print "\t<AS id=\"AS_grid5000\" routing=\"Floyd\" >\n";

`rm -rf temp_g5k`;
`mkdir -p temp_g5k`;
chdir("./temp_g5k");
`wget https://api.grid5000.fr/2.0/grid5000/sites --http-user="$ARGV[0]" --http-password="$ARGV[1]" --no-check-certificate --quiet`;

open SITES_LIGNE, 'sites' or die "Unable to open sites $!\n";
my $line = "";
my $site = "";
my $cluster = "";
my $nodes = "";
my @AS_route = ();

print "\t\t<AS id=\"AS_interne\" routing=\"Floyd\">\n";
printf "\t\t\t<router id=\"lille\"/>\n";
printf "\t\t\t<router id=\"paris\"/>\n";
printf "\t\t\t<router id=\"nancy\"/>\n";
printf "\t\t\t<router id=\"rennes\"/>\n";
printf "\t\t\t<router id=\"lyon\"/>\n";
printf "\t\t\t<router id=\"bordeaux\"/>\n";
printf "\t\t\t<router id=\"grenoble\"/>\n";
printf "\t\t\t<router id=\"marseille\"/>\n";
printf "\t\t\t<router id=\"toulouse\"/>\n";
printf "\t\t\t<router id=\"sophia\"/>\n";
#printf "\t\t\t<router id=\"luxembourg\">\n";
print "\n";
printf "\t\t\t<link id=\"Lille_Paris\"        bandwidth=\"1.25E9\" latency=\"1.0E-4\"/>\n";
printf "\t\t\t<link id=\"Paris_Nancy\"        bandwidth=\"1.25E9\" latency=\"1.0E-4\"/>\n";
printf "\t\t\t<link id=\"Paris_Rennes\"       bandwidth=\"1.25E9\" latency=\"1.0E-4\"/>\n";
printf "\t\t\t<link id=\"Paris_Lyon\"         bandwidth=\"1.25E9\" latency=\"1.0E-4\"/>\n";
printf "\t\t\t<link id=\"Bordeaux_Lyon\"      bandwidth=\"1.25E9\" latency=\"1.0E-4\"/>\n";
printf "\t\t\t<link id=\"Lyon_Grenoble\"      bandwidth=\"1.25E9\" latency=\"1.0E-4\"/>\n";
printf "\t\t\t<link id=\"Lyon_Marseille\"     bandwidth=\"1.25E9\" latency=\"1.0E-4\"/>\n";
printf "\t\t\t<link id=\"Marseille_Sophia\"   bandwidth=\"1.25E9\" latency=\"1.0E-4\"/>\n";
printf "\t\t\t<link id=\"Marseille_Toulouse\" bandwidth=\"1.25E9\" latency=\"1.0E-4\"/>\n";
#printf "\t\t<link id=\"Luxemburg_Nancy\"    bandwidth=\"1.25E9\" latency=\"1.0E-4\"/>\n";
print "\n";

print "\t\t<route src=\"lille\"     dst=\"paris\"     ><link_ctn id=\"Lille_Paris\"/></route>\n";
print "\t\t<route src=\"paris\"     dst=\"nancy\"     ><link_ctn id=\"Paris_Nancy\"/></route>\n";
print "\t\t<route src=\"paris\"     dst=\"rennes\"    ><link_ctn id=\"Paris_Rennes\"/></route>\n";
print "\t\t<route src=\"paris\"     dst=\"lyon\"      ><link_ctn id=\"Paris_Lyon\"/></route>\n";
print "\t\t<route src=\"bordeaux\"  dst=\"lyon\"      ><link_ctn id=\"Bordeaux_Lyon\"/></route>\n";
print "\t\t<route src=\"lyon\"      dst=\"grenoble\"  ><link_ctn id=\"Lyon_Grenoble\"/></route>\n";
print "\t\t<route src=\"lyon\"      dst=\"marseille\" ><link_ctn id=\"Lyon_Marseille\"/></route>\n";
print "\t\t<route src=\"marseille\" dst=\"sophia\"    ><link_ctn id=\"Marseille_Sophia\"/></route>\n";
print "\t\t<route src=\"marseille\" dst=\"toulouse\"  ><link_ctn id=\"Marseille_Toulouse\"/></route>\n";
#print "\t\t<route src=\"Luxemburg\" dst=\"Nancy\"     ><link_ctn id=\"Luxemburg_Nancy\"/></route>\n";
print "\t\t</AS>\n";    

while(defined($line=<SITES_LIGNE>))
{
	if($line =~ /"uid": "(.*)",/){
		$site = $1;
		print "\t\t<AS id=\"AS_$site\" routing=\"RuleBased\" >\n";
		`wget https://api.grid5000.fr/2.0/grid5000/sites/$site/clusters --http-user="$ARGV[0]" --http-password="$ARGV[1]" --no-check-certificate --quiet`;
		open CLUSTERS_LIGNE, 'clusters' or die "Unable to open clusters $!\n";
		while(defined($line=<CLUSTERS_LIGNE>))
		{
			if($line =~ /"uid": "(.*)",/){
				$cluster = $1;
				print "\t\t\t<cluster id=\"AS_$cluster\" prefix=\"$cluster-\" suffix=\".$site.grid5000.fr\"\n";
				`wget https://api.grid5000.fr/2.0/grid5000/sites/$site/clusters/$cluster/nodes --http-user="$ARGV[0]" --http-password="$ARGV[1]" --no-check-certificate --quiet`;
				open NODES_LIGNE, 'nodes' or die "Unable to open nodes $!\n";
				while(defined($line=<NODES_LIGNE>))
				{
					if($line =~ /"total": (.*),/){
						print "\t\t\t\tradical=\"1-$1\" power=\"";
						&get_gflops($cluster);
						print "\" bw=\"1.25E8\" lat=\"1.0E-4\""."\n";
   						print "\t\t\t\tbb_bw=\"1.25E9\" bb_lat=\"1.0E-4\"></cluster>\n";
					}
				}
				print "\t\t\t<link   id=\"link_$cluster\" bandwidth=\"1.25E9\" latency=\"1.0E-4\"/>\n\n";
				close NODES_LIGNE;
				`rm nodes`;
			}
		}
		close CLUSTERS_LIGNE;
		`rm clusters`;
		
		print "\t\t\t<AS id=\"gw_AS_$site\" routing=\"Full\">\n";
		print "\t\t\t\t<router id=\"gw_$site\"/>\n";
		print "\t\t\t</AS>\n";
		print "\t\t\t<link   id=\"link_gw_$site\" bandwidth=\"1.25E9\" latency=\"1.0E-4\"/>\n";
		print "\n";
		
		print "\t\t\t<ASroute src=\"^AS_(.*)\$\" dst=\"^AS_(.*)\$\"\n";
		print "\t\t\t\tgw_src=\"\$1src-AS_\$1src_router.$site.grid5000.fr\"\n";
		print "\t\t\t\tgw_dst=\"\$1dst-AS_\$1dst_router.$site.grid5000.fr\"\n";
		print "\t\t\t\tsymmetrical=\"YES\">\n";
		print "\t\t\t\t\t<link_ctn id=\"link_\$1src\"/>\n";
		print "\t\t\t\t\t<link_ctn id=\"link_\$1dst\"/>\n";
		print "\t\t\t</ASroute>\n\n"; 

		print "\t\t\t<ASroute src=\"^AS_(.*)\$\" dst=\"^gw_AS_(.*)\$\"\n";
		print "\t\t\t\tgw_src=\"\$1src-AS_\$1src_router.$site.grid5000.fr\"\n";
		print "\t\t\t\tgw_dst=\"gw_\$1dst\"\n";
		print "\t\t\t\tsymmetrical=\"NO\">\n";
		print "\t\t\t\t\t<link_ctn id=\"link_\$1src\"/>\n";
		print "\t\t\t</ASroute>\n\n"; 
		
		print "\t\t\t<ASroute src=\"^gw_AS_(.*)\$\" dst=\"^AS_(.*)\$\"\n";
		print "\t\t\t\tgw_src=\"gw_\$1src\"\n";
		print "\t\t\t\tgw_dst=\"\$1dst-AS_\$1dst_router.$site.grid5000.fr\"\n";
		print "\t\t\t\tsymmetrical=\"NO\">\n";
		print "\t\t\t\t\t<link_ctn id=\"link_\$1dst\"/>\n";
		print "\t\t\t</ASroute>\n\n"; 
		
		print "\t\t</AS>\n";
		
		push @AS_route, $site;
	}
}

while(@AS_route)
{
	$site = pop @AS_route;
	print "\t\t<ASroute src=\"AS_$site\" dst=\"AS_interne\" gw_src=\"gw_$site\"";
	if( $site =~ /^orsay$/ )
	{
		print " gw_dst=\"paris\"";
	}
	else
	{
		print " gw_dst=\"$site\"";
	}
	print " symmetrical=\"YES\">\n";
	print "\t\t\t<link_ctn id=\"link_gw_$site\"/>\n";
	print "\t\t</ASroute>\n";
	
#	print "\t\t<ASroute src=\"AS_interne\" dst=\"AS_$site\" gw_dst=\"gw_$site\"";
#	if( $site =~ /^orsay$/ )
#	{
#		print " gw_src=\"paris\"";
#	}
#	else
#	{
#		print " gw_src=\"$site\"";
#	}
#	print " symmetrical=\"NO\">\n";
#	print "\t\t\t<link_ctn id=\"link_gw_$site\"/>\n";
#	print "\t\t</ASroute>\n";
}

print "\t</AS>\n";
print "</platform>\n";
`rm sites`;
close SITES_LIGNE;


sub get_gflops {
	switch ($_[0]) {
		#Bordeaux
		case "bordeplage" { print "5.2297E9" }
		case "bordereau"  { print "8.8925E9" }
		case "borderline" { print "13.357E9" }
		#Lille
		case "chicon"     { print "8.9618E9" }
		case "chimint"    { print "23.531E9" }
		case "chinqchint" { print "22.270E9" }
		case "chirloute"  { print "24.473E9" }
		#Grenoble
		case "adonis"     { print "23.681E9" }
		case "edel"       { print "23.492E9" }
		case "genepi"     { print "21.175E9" }
		#Lyon
		case "capricorne" { print "4.7233E9" }
		case "sagittaire" { print "5.6693E9" }
		#Nancy
		case "graphene"   { print "16.673E9" }
		case "griffon"    { print "20.678E9" }
		#Orsay
		case "gdx"        { print "4.7153E9" }
		case "netgdx"     { print "4.7144E9" }
		#Rennes
		case "paradent"   { print "21.496E9" }
		case "paramount"  { print "12.910E9" }
		case "parapide"   { print "30.130E9" }
		case "parapluie"  { print "27.391E9" }
		#Sophia
		case "helios"     { print "7.7318E9" }
		case "sol"        { print "8.9388E9" }
		case "suno"       { print "23.530E9" }
		#Toulouse
		case "pastel"     { print "9.5674E9" }
		case "violette"   { print "5.1143E9" }
		#Reims
		case "stremi"     { print "TODO" }
		#Luxembourg
		case "granduc"    { print "TODO" }

		default: 	  { print "TODO" }
	}
}
