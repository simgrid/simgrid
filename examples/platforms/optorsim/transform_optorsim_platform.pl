#!/usr/bin/env perl

# Copyright (c) 2011, 2014, 2016. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

use strict;
use warnings;

if($#ARGV!=0) {
    die "Usage: perl transfrom_optorsim_platform.pl <file.conf>\n";
}

my($conf_file)=$ARGV[0];

open FILE, $conf_file or die "Unable to open $conf_file";

print "<!-- This platform was automatically converted from the OptorSim platform.\n";
print "\n";
print "  OptorSim files only describes information of the cluster interconnexion.\n";
print "  In a sense, it reflects the topology of a National Research and Education\n";
print "  Network (NREN), but not of a full-fledged computational platform.\n";
print "  The caracteristics of each cluster have been artificially added.\n";
print "\n";
print "  We hope that you find it useful anyway. If you know how to complete\n";
print "  this information with data on the cluster configurations, please\n";
print "  drop us a mail so that we can add this information. -->\n\n";

print "<?xml version='1.0'?>\n";
print "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid.dtd\">\n";
print "<platform version=\"3\">\n";
print "<AS id=\"AS0\" routing=\"Floyd\">\n";

my $line;
my @tokens;
my $token;
my $dst = 0;
my $src = 0;
my @list_of_name;
my $num_link = 0;

my @links = ();
my @links_router = ();
my @routers = ();
my @hosts = ();
my @is_router = ();
my @routes = ();
my @routes_router = ();
my @routes_cluster = ();
my $nb_host;
while(defined($line=<FILE>)){
    if($line =~ /^#(.*)\)(.*)$/)
    {
	print "<!--$1 $2-->\n";
	push @list_of_name, $2;
    }
    elsif($line =~ /^([0-9]*) ([0-9]*) ([0-9]*) (.*)$/)
    {
	if($1 == "0"){
	    push @is_router, 1;
	    if(@list_of_name){
		push @routers, "    <router id=\"$list_of_name[$src]\"/>\n";
	    }
	    else{
		push @routers, "    <router id=\"router$src\"/>\n";
	    }
	}
	else{
	    push @is_router, 0;
	    $nb_host = $1;
	    if(@list_of_name){
		push @hosts, "  <cluster id=\"$list_of_name[$src]\" prefix=\"$list_of_name[$src]-\" suffix=\"\"\n";
		push @hosts, "           radical=\"1-$nb_host\" power=\"1000000000\" bw=\"125000000\" lat=\"5E-5\"\n";
		push @hosts, "           router_id=\"$list_of_name[$src]-router\"/>\n";
	    }
	    else{
		push @hosts, "  <cluster id=\"cluster$src\" prefix=\"$src-\" suffix=\"\"\n";
		push @hosts, "           radical=\"1-$nb_host\" power=\"1000000000\" bw=\"125000000\" lat=\"5E-5\"\n";
		push @hosts, "           router_id=\"cluster$src-router\"/>\n";
	    }
	}		
	my $table = $4;
	@tokens = split(/ /,$table);
	foreach $token (@tokens) {
	    if($src >= $dst){
		if($token != "0") #if there is a link between src and dst
		{	
		    #Create a link				
		    if($1 == "0"){ 
			push @links_router, "    <link id=\"link$num_link\" bandwidth=\"$token\"/>\n";
		    }
		    else{
			push @links, "  <link id=\"link$num_link\" bandwidth=\"$token\"/>\n";
		    }
		    
		    #Create the route between router and router
		    if($is_router[$src] && $is_router[$dst]) 
		    {
			if(@list_of_name){
			    push @routes_router, "    <route src=\"$list_of_name[$src]\" dst=\"$list_of_name[$dst]\">";
			}
			else{
			    push @routes_router, "    <route src=\"router$src\" dst=\"router$dst\">";
			}
			push @routes_router, " <link_ctn id=\"link$num_link\"/>";
			push @routes_router, " </route>\n";
		    }
		    
		    #Create the route between cluster and cluster
		    elsif(!$is_router[$src] && !$is_router[$dst]) 
		    {
			if(@list_of_name){
			    push @routes_cluster, "    <ASroute src=\"$list_of_name[$src]\" dst=\"$list_of_name[$dst]\"";
			    push @routes_cluster, " gw_src=\"$list_of_name[$src]-router\" gw_dst=\"$list_of_name[$dst]-router\">\n";
			}
			else{
			    push @routes_cluster, "    <ASroute src=\"cluster$src\" dst=\"cluster$dst\"";
			    push @routes_cluster, " gw_src=\"cluster$src-router\" dst=\"cluster$dst-router\">\n";
			}
			push @routes_cluster, "      <link_ctn id=\"link$num_link\"/>\n";
			push @routes_cluster, "    </ASroute>\n";
		    }				
		    else
		    {
			push @routes, "  <ASroute ";
			if(@list_of_name){
			    if($is_router[$src]) 	#router
			    {push @routes, "src=\"AS_intern\" gw_src=\"$list_of_name[$src]\" ";}
			    else			#cluster
			    {push @routes, "src=\"$list_of_name[$src]\" gw_src=\"$list_of_name[$src]-router\" ";}
			    
			    
			    if($is_router[$dst]) 	#router
			    {push @routes, "dst=\"AS_intern\" gw_dst=\"$list_of_name[$dst]\">\n";}
			    else			#cluster
			    {push @routes, "dst=\"$list_of_name[$dst]\" gw_dst=\"$list_of_name[$dst]-router\">\n";}
			}
			else{
			    if($is_router[$src]) 	#router
			    {push @routes, "src=\"AS_intern\" gw_src=\"router$src\" ";}
			    else			#cluster
			    {push @routes, "src=\"cluster$src\" gw_src=\"cluster$src-router\" ";}
			    
			    
			    if($is_router[$dst]) 	#router
			    {push @routes, "dst=\"AS_intern\" gw_dst=\"router$dst\">\n";}
			    else			#cluster
			    {push @routes, "dst=\"cluster$dst\" gw_dst=\"cluster$dst-router\">\n";}
			}
			push @routes, "    <link_ctn id=\"link$num_link\"/>\n";
			push @routes, "  </ASroute>\n";
			
		    }
		    
		    $num_link++;	
		}
	    }
	    $dst++;
	}
	$src++;
	$dst = 0;
    }
    else
    {
	die;
    }
}
close(FILE);

print "  <AS id=\"AS_intern\" routing=\"Floyd\">\n";
print @routers;
print @links_router;
print @routes_router;
print "  </AS>\n";
print "\n";	
print @hosts;
print @routes_cluster;
print "\n";

print @links;
print "\n";
print @routes;
print "</AS>\n";
print "</platform>";
print " \n";
