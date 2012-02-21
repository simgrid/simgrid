#! /usr/bin/perl

# L.Bobelin (Perl newbie) 25th of November
# Quick script to generate hierarchical clusters. Usage : <the script> p s d  where :
# - p : 2^p gives the total number of hosts.
# - s : cluster size
# - d : degree of inner nodes.
#
# output is the standard one. 
# 
#
#Each node is numbered by a DFS in the tree. Each cluster is numbered by the DFS number of the leaf it is attached to and the number of cluster for each leaf. 
# Other infos : 
# - Same bb_lat used for any routers inside (not that complicated to modify too).
# - constants defined in the first part of the script corresponding to classic cluster parameters. links_bw and links_lat added for the inner tree links
# - bb_lat and bb_bw used in any backbone of the tree.
# - fails if you set an obviously too small total number of hosts compared to the cluster size (generates a lot of stuff for nothing actually).
# 

use Math::BigInt;

$prefix= ""; 
$suffix= "";
$bw= "125000000";
$power= "1000000000";
$lat= "5E-5";
$bb_bw= "2250000000";
$bb_lat= "5E-4"; 
$links_bw= "2250000000";
$links_lat= "5E-5";
$id= "";

$p = $ARGV[0];
$s = $ARGV[1];
$d = $ARGV[2];

$p = Math::BigInt->new($p);
$d = Math::BigInt->new($d);
$s = Math::BigInt->new($s);

$cabinetnodes= $d;
$nbsons= $d;
$radical= "1-" . $s;
$last=$s;

# Number of clusters to generate ? Update: I hate this bigInt package, the way it behaves is SO stupid 
$totalnumberofhosts = Math::BigInt->new("2");
$totalnumberofhosts->bpow($p);

$totalnumberofCluster= $totalnumberofhosts->copy();

$totalnumberofCluster->bdiv($s);

# checking if we have to have something non homogeneous
if ($totalnumberofhosts->copy()->bmod($s) != 0 ) 
	{
		$totalnumberofCluster++;
		$last= $totalnumberofhosts->copy()->bmod($s);
	}

# Calculating height

$height= $totalnumberofCluster->copy();
$height->broot($d);

# Checking if an exact root exists
if ( $height->bcmp(Math::BigInt->new("1")) != 0 && ($height->copy()->bpow($d))->bcmp($totalnumberofCluster)) {	
	
	$height++; #will have to deal with empty set of clusters.	
	}
# debug stuff	
# print "Computed : \n";
# print STDERR "height: " . $height . "\n";
# print STDERR "totalnumberofhosts: " . $totalnumberofhosts . "\n";
# print STDERR "totalnumberofcluster: " .  $totalnumberofCluster . "\n";
# print STDERR "last cluster size (if equals to cluster size, then all clusters will be homogeneous) : " . $last . "\n";

# Counter for giving unique IDs to ASes.
$ASnumber;
$ASnumber = 0;

# Printing preamble
print "<?xml version='1.0'?>\n";
print "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid.dtd\">\n";
print "<platform version=\"3\">\n\n";

	
# Initiate recursion ...
&DF_creation(0);

# Closing tag, and then back home
print "</platform>\n";	


# Recursive stuff for depth first Se... Creation
sub DF_creation {
	my($currDepth) = @_;
	
	# Curr AS creation
	print "<AS id=\"". $prefix . "AS_" . $ASnumber . $suffix . "\"  routing=\"Full\">\n";	
	
	# Curr router AS creation stuff
	print "<AS id=\"". $prefix . "exitAS_" . $ASnumber . $suffix . "\"  routing=\"Full\">\n";			 
	print "	<router id=\"" . $prefix . "router_" . $ASnumber . $suffix . "\"/>\n";
	print "</AS>\n";
	# Saving my current number to return it to my father
	my $toReturn = $ASnumber;
	$ASnumber++;
	if ($currDepth<=$height && $totalnumberofCluster > 0)
		{		
		# Creating current AS inner stuff
		# I should have a table of sons numbers.
		my @tsons = ();
		my $createdSons = 0;
		for (my $i =1; $i<=$nbsons && $totalnumberofCluster > 0 ; $i++)
		{
		#saving this son in my tab ...  recursive call to create ASes and cluster underneath
		push(@tsons, &DF_creation($currDepth + 1)); 
		$createdSons++;
		# 		
		# Creating link to this son
		print "<link id=\"". $prefix . $tsons[$i-1] . $suffix . "\" bandwidth=\"" . $links_bw . "\" latency=\"" . $links_lat . "\"/>\n";	
		}
		# curr backbone creation 
		print "<link id=\"". $prefix . "bb_" . $toReturn . $suffix . "\" bandwidth=\"" . $bb_bw . "\" latency=\"" . $bb_lat . "\"/>\n";
		# Full routing AS to AS declaration
		for (my $i =1; $i<=$createdSons ; $i++)
		{
					for (my $j =$i+1; $j<=$createdSons ; $j++)
					{
						print  "<ASroute src=\"" . $prefix . "AS_" . $tsons[$i-1] . $suffix . "\"\n";
						print "	dst=\"" . $prefix . "AS_" . $tsons[$j-1] . $suffix . "\"\n";
						print "	gw_src=\"" . $prefix . "router_" . $tsons[$i-1] . $suffix . "\"\n";
						print "	gw_dst=\"" . $prefix . "router_" . $tsons[$j-1] . $suffix . "\"\n";
						print "	symmetrical=\"YES\">\n";
						
						print "		<link_ctn id=\"" . $prefix . $tsons[$i-1] . $suffix . "\"/>\n";
						print "		<link_ctn id=\"" . $prefix . "bb_" . $toReturn . $suffix . "\"/>\n"; 
						print "		<link_ctn id=\"" . $prefix . $tsons[$j-1] . $suffix . "\"/>\n";
						print "</ASroute>\n";
					}
		}
		# Now routes to the exit AS
		for (my $i =1; $i<=$createdSons ; $i++)
		{
			print  "<ASroute src=\"" . $prefix . "AS_" . $tsons[$i-1] . $suffix . "\"\n";
			print "	dst=\"" . $prefix . "exitAS_" . $toReturn . $suffix . "\"\n";
			print "	gw_src=\"" . $prefix . "router_" . $tsons[$i-1] . $suffix . "\"\n";
			print "	gw_dst=\"" . $prefix . "router_" . $toReturn . $suffix . "\"\n";
			print "	symmetrical=\"YES\">\n";						
			print "		<link_ctn id=\"" . $prefix . $tsons[$i-1] . $suffix . "\"/>\n";
			print "		<link_ctn id=\"" . $prefix . "bb_" . $toReturn . $suffix . "\"/>\n"; 
			print "</ASroute>\n";			
		}
		print "</AS>\n";
		# DO I have extra stuff to add ? I don't think so. 		
		return $toReturn;
		}
	else { # On leaves, 
		my $lastNumberOfClusterCreated = 0;	
		#I must create clusters now
		for(my $i = 1; $i <= $cabinetnodes && $totalnumberofCluster>0 ; $i++) {
			$lastNumberOfClusterCreated++;
			if ($totalnumberofCluster==1)
			{
			print "<cluster id=\"". $prefix . "cl_" . $toReturn . "_" . $i . $suffix . "\" prefix=\"" . $prefix . "c_" . $toReturn . "_" . $i . "-\" suffix=\"" . $suffix . "\" radical=\"1-"
				. $last . "\" power=\"" . $power . "\" bw=\"" . $bw . "\" lat=\"" . $lat . "\" bb_bw=\"" . $bb_bw . "\" bb_lat=\"" . $bb_lat . "\"/>\n";	
			}
			else 
			{	
			print "<cluster id=\"". $prefix . "cl_" . $toReturn . "_" . $i . $suffix . "\" prefix=\"" . $prefix . "c_" . $toReturn . "_" . $i . "-\" suffix=\"" . $suffix . "\" radical=\""
				. $radical . "\" power=\"" . $power . "\" bw=\"" . $bw . "\" lat=\"" . $lat . "\" bb_bw=\"" . $bb_bw . "\" bb_lat=\"" . $bb_lat . "\"/>\n";	
			}
			$totalnumberofCluster--;
			}	
		# Creating links to clusters 
		for(my $i = 1; $i <= $lastNumberOfClusterCreated ; $i++) {
			print "<link id=\"". $prefix . $toReturn . "_" . $i . $suffix . "\" bandwidth=\"" . $links_bw . "\" latency=\"" . $links_lat . "\"/>\n";
		}

		# 
		# curr backbone creation 
		print "<link id=\"". $prefix . "bb_" . $toReturn . $suffix . "\" bandwidth=\"" . $bb_bw . "\" latency=\"" . $bb_lat . "\"/>\n";
	
		# I must create routes between clusters now 
		for (my $i =1; $i<=$lastNumberOfClusterCreated ; $i++)
			{
					for (my $j =$i+1; $j<=$lastNumberOfClusterCreated ; $j++)
					{
						print  "<ASroute src=\"" . $prefix . "cl_" . $toReturn . "_" . $i . $suffix .  "\"\n";
						print "	dst=\"" .  $prefix . "cl_" . $toReturn . "_" . $j . $suffix .  "\"\n";

						print "	gw_src=\"" . $prefix . "c_" . $toReturn . "_" . $i . "-" . $prefix . "cl_" . $toReturn . "_" . $i . $suffix . "_router" . $suffix  ."\"\n";
						print "	gw_dst=\"" . $prefix . "c_" . $toReturn . "_" . $j . "-" . $prefix . "cl_" . $toReturn . "_" . $j . $suffix  . "_router" . $suffix . "\"\n";
						print "	symmetrical=\"YES\">\n";
						
						print "		<link_ctn id=\"" . $prefix . $toReturn. "_" . $i . $suffix . "\"/>\n";
						print "		<link_ctn id=\"" . $prefix . "bb_" . $toReturn . $suffix . "\"/>\n"; 
						print "		<link_ctn id=\"" . $prefix . $toReturn . "_" . $j . $suffix . "\"/>\n";
						print "</ASroute>\n";
					}
			}
		# Now routes to the exit AS
		for (my $i =1; $i<=$lastNumberOfClusterCreated ; $i++)
		{
			print  "<ASroute src=\""  . $prefix . "cl_" . $toReturn . "_" . $i . $suffix  . "\"\n";
			print "	dst=\"" . $prefix . "exitAS_" . $toReturn . $suffix . "\"\n";
			# SAME HERE !!
			print "	gw_src=\"" . $prefix . "c_" . $toReturn . "_" . $i . "-" . $prefix . "cl_" . $toReturn . "_" . $i . $suffix . "_router" . $suffix  ."\"\n";
			print "	gw_dst=\"" . $prefix . "router_" . $toReturn . $suffix . "\"\n";
			print "	symmetrical=\"YES\">\n";						
			print "		<link_ctn id=\"" . $prefix . $toReturn . "_" . $i . $suffix . "\"/>\n";
			print "		<link_ctn id=\"" . $prefix . "bb_" . $toReturn . $suffix . "\"/>\n"; 
			print "</ASroute>\n";			
		}
		print "</AS>\n";
	# Should be done with it...
	return $toReturn;
	}

}
