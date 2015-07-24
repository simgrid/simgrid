/*
 * Copyright (c) 2003-2005 The BISON Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
		
package peersim.reports;

import peersim.config.Configuration;
import peersim.graph.GraphAlgorithms;
import peersim.util.IncrementalStats;

/**
* Prints reports on the graph like average clustering and average path length,
* based on random sampling of the nodes.
* In fact its functionality is a subset of the union of {@link Clustering}
* and {@link BallExpansion}, and therefore is redundant,
* but it is there for historical reasons.
* @see BallExpansion
* @see Clustering
*/
public class GraphStats extends GraphObserver {


// ===================== fields =======================================
// ====================================================================

/** 
* The number of nodes to use for
* sampling average path length.
* Statistics are printed over a set of node pairs.
* To create the set of pairs, we select the given number of different nodes
* first, and then pair all these nodes with every other node in the network.
* If zero is given, then no statistics
* will be printed about path length. If a negative value is given then
* the value is the full size of the graph.
* Defaults to zero.
* @config
*/
private static final String PAR_NL = "nl";

/** 
* The number of nodes to use to sample
* average clustering.
* If zero is given, then no statistics
* will be printed about clustering. If a negative value is given then
* the value is the full size of the graph.
* Defaults to zero.
* @config
*/
private static final String PAR_NC = "nc";

private final int nc;

private final int nl;


// ===================== initialization ================================
// =====================================================================


/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param name the configuration prefix for this class
 */
public GraphStats(String name) {

	super(name);
	nl = Configuration.getInt(name+"."+PAR_NL,0);
	nc = Configuration.getInt(name+"."+PAR_NC,0);
}


// ====================== methods ======================================
// =====================================================================

/**
* Returns statistics over minimal path length and clustering.
* The output is the average over the set of
* clustering coefficients of randomly selected nodes, and the
* set of distances from randomly selected nodes to all the other nodes.
* The output is always concatenated in one line, containing zero, one or two
* numbers (averages) as defined by the config parameters.
* Note that the path length between a pair of nodes can be infinite, in which
* case the statistics will reflect this (the average will be infinite, etc).
* See also the configuration parameters.
* @return always false
* @see BallExpansion
* @see Clustering
*/
public boolean execute() {
	
	System.out.print(name+": ");
	
	IncrementalStats stats = new IncrementalStats();
	updateGraph();
	
	if( nc != 0 )
	{
		stats.reset();
		final int n = ( nc<0 ? g.size() : nc );
		for(int i=0; i<n && i<g.size(); ++i)
		{
			stats.add(GraphAlgorithms.clustering(g,i));
		}
		System.out.print(stats.getAverage()+" ");
	}
	
	if( nl != 0 )
	{
		stats.reset();
		final int n = ( nl<0 ? g.size() : nl );
		outerloop:
		for(int i=0; i<n && i<g.size(); ++i)
		{
			ga.dist(g,i);
			for(int j=0; j<g.size(); ++j)
			{
				if( j==i ) continue;
				if (ga.d[j] == -1)
				{
					stats.add(Double.POSITIVE_INFINITY);
					break outerloop;
				}
				else
					stats.add(ga.d[j]); 
			}
		}
		System.out.print(stats.getAverage());
	}
	
	System.out.println();
	return false;
}

}

