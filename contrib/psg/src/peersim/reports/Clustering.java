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
 * Control to observe the clustering coefficient.
 * @see GraphAlgorithms#clustering
 */
public class Clustering extends GraphObserver
{

// ===================== fields =======================================
// ====================================================================

/**
 * The number of nodes to collect info about. Defaults to the size of the graph.
 * @config
 */
private static final String PAR_N = "n";

private final int n;

// ===================== initialization ================================
// =====================================================================

/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param name the configuration prefix for this class
 */
public Clustering(String name)
{
	super(name);
	n = Configuration.getInt(name + "." + PAR_N, Integer.MAX_VALUE);
}

// ====================== methods ======================================
// =====================================================================

/**
* Prints information about the clustering coefficient.
* It uses {@value #PAR_N} nodes to collect statistics.
* The output is
* produced by {@link IncrementalStats#toString}, over the values of
* the clustering coefficients of the given number of nodes.
* Clustering coefficients are calculated by {@link GraphAlgorithms#clustering}.
* @return always false
*/
public boolean execute()
{
	IncrementalStats stats = new IncrementalStats();
	updateGraph();
	for (int i = 0; i < n && i < g.size(); ++i) {
		stats.add(GraphAlgorithms.clustering(g, i));
	}
	System.out.println(name + ": " + stats);
	return false;
}

}
