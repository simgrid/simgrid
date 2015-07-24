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

import peersim.core.*;
import peersim.config.Configuration;
import peersim.graph.*;
import peersim.util.IncrementalStats;
import java.util.Map;
import java.util.Iterator;

/**
 * It tests the network for robustness to random node removal.
 * It does not actually remove
 * nodes, it is only an observer, so can be applied several times during the
 * simulation. A warning though: as a side effect it <em>may
 * shuffle the network</em> (see {@value #PAR_N}) so if this is an issue,
 * it should not be used,
 * or only after the simulation has finished.
 */
public class RandRemoval extends GraphObserver
{

// ===================== fields =======================================
// ====================================================================

// XXX remove side effect
/**
 * This parameter defines the number of runs of the iterative removal procedure
 * to get statistics. Look out: if set to a value larger than 1 then as a side
 * effect <em>the overlay will be shuffled</em>. Defaults to 1.
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
public RandRemoval(String name)
{
	super(name);
	n = Configuration.getInt(name + "." + PAR_N, 1);
}

// ====================== methods ======================================
// =====================================================================

/**
* Prints results of node removal tests. The following experiment is
* repeated {@value #PAR_N} times. From the graph 50%, 51%, ..., 99% of the nodes
* are removed at random. For all percentages it is calculated what is
* the maximal
* clustersize (weakly connected clusters) and the number of
* clusters in the remaining graph.
* These values are averaged over the experiments, and for all 50 different
* percentage values a line is printed that contains the respective averages,
* first the average maximal cluster size, followed by the average number
* of clusters.
* @return always false
*/
public boolean execute()
{
	if( n < 1 ) return false;
	updateGraph();
	
	System.out.println(name + ":");
	
	final int size = Network.size();
	final int steps = 50;
	IncrementalStats[] maxClust = new IncrementalStats[steps];
	IncrementalStats[] clustNum = new IncrementalStats[steps];
	for (int i = 0; i < steps; ++i) {
		maxClust[i] = new IncrementalStats();
		clustNum[i] = new IncrementalStats();
	}
	for (int j = 0; j < n; ++j) {
		PrefixSubGraph sg = new PrefixSubGraph(g);
		IncrementalStats stats = new IncrementalStats();
		for (int i = 0; i < steps; i++) {
			sg.setSize(size / 2 - i * (size / 100));
			Map clst = ga.weaklyConnectedClusters(sg);
			stats.reset();
			Iterator it = clst.values().iterator();
			while (it.hasNext()) {
				stats.add(((Integer) it.next()).intValue());
			}
			maxClust[i].add(stats.getMax());
			clustNum[i].add(clst.size());
		}
		if( j+1 < n ) Network.shuffle();
	}
	for (int i = 0; i < steps; ++i) {
		System.out.println(maxClust[i].getAverage() + " "
				+ clustNum[i].getAverage());
	}
	return false;
}

}
