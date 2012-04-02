#! /usr/bin/perl

# L.Bobelin (Perl newbie) 24th of November
# Quick script to generate hierarchical clusters. Usage : add the special cluster tag (description below) in your "normal" platform file. Then run the script :
# - First arg : the input file where you midified your cluster tag
# - Second one : the output file where all the stuff will be generated.
# Builds a complete tree to access clusters ; each node of the tree is inclosed in an AS, where full routing applies.
#
# Number of cluster per leaf is given by cabinetnodes attr.
#
#
# Choosed to modify a cluster tag to allow to give additional informations : 
# - nbsons : degree of inner  
# - height : tree heigth
# - cabinetnodes : cluster per leaf
#
# Each node is numbered by a DFS in the tree. Each cluster is numbered by the DFS number of the leaf it is attached to and the number of cluster for each leaf. 
#
#
# Example syntax for hierarchical cluster creation : 
# <cluster id="AS_cb1" prefix="cb1-" suffix=".dc1.acloud.com" power="5.2297E9" bw="1.25E8" lat="1.0E-4 bb_bw="1.25E9" bb_lat="1.0E-4" radical="0-99" cabinetnodes="4" height="3" nbsons="2" links_lat="1.0E-4" links_bw="1.25E9"/>
# Other infos : 
# - special tag has to be on one line because I don't want to bother with parsing issues
# - Same bb_lat used for any routers inside (not that complicated to modify too)
# - lame perl ? I'm a script kiddie in perl, it may well be my first perl stuff. 
# - Don't try to check or validate the modified file with the DTD, of course, as this is not a part of it.

# Counter for giving unique IDs to ASes.
$ASnumber;
$ASnumber = 0;

$infile;
$outfile; 

$infile = $ARGV[0];
$outfile = $ARGV[1];
open IN, "$infile" || die "Cannot parse " . $infile . " ...\n";
open OUT,">$outfile" || die "Cannot use the output file " . $outfile . " ...\n";
my $line;
while ($line = <IN>) {
# looking for good lines. 
if ($line =~ / cabinetnodes=/) 
{ #Retrieving informations
	($line=~ /cabinetnodes=\"([^\"]*)/);
	$cabinetnodes= $1;
	($line=~ /height=\"([^\"]*)/);
	$height= $1;
	($line=~ /nbsons=\"([^\"]*)/);
	$nbsons= $1;
	 ($line=~ /id=\"([^\"]*)/);
	$id= $1;
	($line=~ /prefix=\"([^\"]*)/);
	$prefix= $1; 
	($line=~ /suffix=\"([^\"]*)/);
	$suffix= $1;
	($line=~ /bw=\"([^\"]*)/);
	$bw= $1;
	($line=~ /power=\"([^\"]*)/);
	$power= $1;
	($line=~ /lat=\"([^\"]*)/);
	$lat= $1;
	($line=~ /bb_bw=\"([^\"]*)/);
	$bb_bw= $1;
	($line=~ /bb_lat=\"([^\"]*)/);
	$bb_lat= $1; 
	($line=~ /links_bw=\"([^\"]*)/);
	$links_bw= $1;
	($line=~ /links_lat=\"([^\"]*)/);
	$links_lat= $1;
	($line=~ /radical=\"([^\"]*)/);
	$radical= $1;

	print "Variables read : \n";
	print "number of clusters in each cabinet: " . $cabinetnodes . "\n";
	print "Tree heigth: " . $height . "\n";
	print "Degree of each node: " . $nbsons . "\n";
	print "General id: ". $id . "\n";
	print "General prefix: " . $prefix . "\n";
	print "General suffix: ". $suffix . "\n";
	print "Bandwidth for cluster inner links: " . $bw . "\n";
	print "Power for cluster nodes: " . $power . "\n";
	print "Latency for clusters inner links :" . $lat . "\n";
	print "Backbone bandwwidth (used in all backbones, including the tree ones):" . $bb_bw . "\n";
	print "Backbone latency (used in all backbones, including the tree ones):" . $bb_lat . "\n";
	print "Tree links bandwidth: " . $links_bw . "\n";
	print "Tree links latency: " . $links_lat . "\n";
	print "Radical: " . $radical . "\n";

	
	
	&DF_creation(0);
	}
else {
print OUT $line;
}
} #End while
close IN;
close OUT;
print $infile . " -> " . $outfile . " ... Done.\n";

# Recursive stuff for depth first Se... Creation
sub DF_creation {
	my($currDepth) = @_;
	
	# Curr AS creation
	print OUT "<AS id=\"". $prefix . "AS_" . $ASnumber . $suffix . "\"  routing=\"Full\">\n";	
	
	# Curr router AS creation stuff
	print OUT "<AS id=\"". $prefix . "exitAS_" . $ASnumber . $suffix . "\"  routing=\"Full\">\n";			 
	print OUT "	<router id=\"" . $prefix . "router_" . $ASnumber . $suffix . "\"/>\n";
	print OUT "</AS>\n";
	# Saving my current number to return it to my father
	my $toReturn = $ASnumber;
	$ASnumber++;
	if ($currDepth<$height)
		{				
		# Creating current AS inner stuff
		# I should have a table of sons numbers.
		my @tsons = ();
		for (my $i =1; $i<=$nbsons ; $i++)
		{
		#saving this son in my tab ...  recursive call to create ASes and cluster underneath
		push(@tsons, &DF_creation($currDepth + 1)); 
		# 		
		# Creating link to this son
		print OUT "<link id=\"". $prefix . $tsons[$i-1] . $suffix . "\" bandwidth=\"" . $links_bw . "\" latency=\"" . $links_lat . "\"/>\n";	
		}
		# curr backbone creation 
		print OUT "<link id=\"". $prefix . "bb_" . $toReturn . $suffix . "\" bandwidth=\"" . $bb_bw . "\" latency=\"" . $bb_lat . "\"/>\n";
		# Full routing AS to AS declaration
		for (my $i =1; $i<=$nbsons ; $i++)
		{
					for (my $j =$i+1; $j<=$nbsons ; $j++)
					{
						print OUT  "<ASroute src=\"" . $prefix . "AS_" . $tsons[$i-1] . $suffix . "\"\n";
						print OUT "	dst=\"" . $prefix . "AS_" . $tsons[$j-1] . $suffix . "\"\n";
						print OUT "	gw_src=\"" . $prefix . "router_" . $tsons[$i-1] . $suffix . "\"\n";
						print OUT "	gw_dst=\"" . $prefix . "router_" . $tsons[$j-1] . $suffix . "\"\n";
						print OUT "	symmetrical=\"YES\">\n";
						
						print OUT "		<link_ctn id=\"" . $prefix . $tsons[$i-1] . $suffix . "\"/>\n";
						print OUT "		<link_ctn id=\"" . $prefix . "bb_" . $toReturn . $suffix . "\"/>\n"; 
						print OUT "		<link_ctn id=\"" . $prefix . $tsons[$j-1] . $suffix . "\"/>\n";
						print OUT "</ASroute>\n";
					}
		}
		# Now routes to the exit AS
		for (my $i =1; $i<=$nbsons ; $i++)
		{
			print OUT  "<ASroute src=\"" . $prefix . "AS_" . $tsons[$i-1] . $suffix . "\"\n";
			print OUT "	dst=\"" . $prefix . "exitAS_" . $toReturn . $suffix . "\"\n";
			print OUT "	gw_src=\"" . $prefix . "router_" . $tsons[$i-1] . $suffix . "\"\n";
			print OUT "	gw_dst=\"" . $prefix . "router_" . $toReturn . $suffix . "\"\n";
			print OUT "	symmetrical=\"YES\">\n";						
			print OUT "		<link_ctn id=\"" . $prefix . $tsons[$i-1] . $suffix . "\"/>\n";
			print OUT "		<link_ctn id=\"" . $prefix . "bb_" . $toReturn . $suffix . "\"/>\n"; 
			print OUT "</ASroute>\n";			
		}
		print OUT "</AS>\n";
		# DO I have extra stuff to add ? I don't think so. 		
		return $toReturn;
		}
	else { # On leaves, 
			
		#I must create clusters now
		for(my $i = 1; $i <= $cabinetnodes; $i++) {
			print OUT "<cluster id=\"". $prefix . "cluster_" . $toReturn . $i . $suffix . "\" prefix=\"" . $prefix . "c_" . $toReturn. $i . "-\" suffix=\"" . $suffix . "\" radical=\""
				. $radical . "\" power=\"" . $power . "\" bw=\"" . $bw . "\" lat=\"" . $lat . "\" bb_bw=\"" . $bb_bw . "\" bb_lat=\"" . $bb_lat . "\"/>\n";	
			}	
		# Creating links to clusters
		for(my $i = 1; $i <= $cabinetnodes; $i++) {
			print OUT "<link id=\"". $prefix . $toReturn . "_" . $i . $suffix . "\" bandwidth=\"" . $links_bw . "\" latency=\"" . $links_lat . "\"/>\n";
		}

		# 
		# curr backbone creation 
		print OUT "<link id=\"". $prefix . "bb_" . $toReturn . $suffix . "\" bandwidth=\"" . $bb_bw . "\" latency=\"" . $bb_lat . "\"/>\n";
	
		# I must create routes between clusters now
		for (my $i =1; $i<=$cabinetnodes ; $i++)
			{
					for (my $j =$i+1; $j<=$cabinetnodes ; $j++)
					{
						print OUT  "<ASroute src=\"" . $prefix . "cluster_" . $toReturn . $i . $suffix .  "\"\n";
						print OUT "	dst=\"" .  $prefix . "cluster_" . $toReturn . $j . $suffix .  "\"\n";

						print OUT "	gw_src=\"" . $prefix . "c_" . $toReturn. $i . "-" . $prefix . "cluster_" . $toReturn . $i . $suffix . "_router" . $suffix  ."\"\n";
						print OUT "	gw_dst=\"" . $prefix . "c_" . $toReturn. $j . "-" . $prefix . "cluster_" . $toReturn . $j . $suffix  . "_router" . $suffix . "\"\n";
						print OUT "	symmetrical=\"YES\">\n";
						
						print OUT "		<link_ctn id=\"" . $prefix . $toReturn. "_" . $i . $suffix . "\"/>\n";
						print OUT "		<link_ctn id=\"" . $prefix . "bb_" . $toReturn . $suffix . "\"/>\n"; 
						print OUT "		<link_ctn id=\"" . $prefix . $toReturn . "_" . $j . $suffix . "\"/>\n";
						print OUT "</ASroute>\n";
					}
			}
		# Now routes to the exit AS
		for (my $i =1; $i<=$cabinetnodes ; $i++)
		{
			print OUT  "<ASroute src=\""  . $prefix . "cluster_" . $toReturn . $i . $suffix  . "\"\n";
			print OUT "	dst=\"" . $prefix . "exitAS_" . $toReturn . $suffix . "\"\n";
			# SAME HERE !!
			print OUT "	gw_src=\"" . $prefix . "c_" . $toReturn. $i . "-" . $prefix . "cluster_" . $toReturn . $i . $suffix . "_router" . $suffix  ."\"\n";
			print OUT "	gw_dst=\"" . $prefix . "router_" . $toReturn . $suffix . "\"\n";
			print OUT "	symmetrical=\"YES\">\n";						
			print OUT "		<link_ctn id=\"" . $prefix . $toReturn . "_" . $i . $suffix . "\"/>\n";
			print OUT "		<link_ctn id=\"" . $prefix . "bb_" . $toReturn . $suffix . "\"/>\n"; 
			print OUT "</ASroute>\n";			
		}
		print OUT "</AS>\n";
	# Should be done with it...
	return $toReturn;
	}

}
