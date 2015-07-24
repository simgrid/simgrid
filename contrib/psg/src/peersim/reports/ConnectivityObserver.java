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

import java.util.Iterator;
import java.util.Map;
import peersim.config.Configuration;
import peersim.util.IncrementalStats;

/**
 * Reports statistics about connectivity properties of the network, such as
 * weakly or strongly connected clusters.
 */
public class ConnectivityObserver extends GraphObserver
{

//--------------------------------------------------------------------------
//Parameters
//--------------------------------------------------------------------------

/**
 * The parameter used to request cluster size statistics instead of the usual
 * list of clusters. Not set by default.
 * @config
 */
private static final String PAR_STATS = "stats";

/**
 * Defines the types of connected clusters to discover.
 * Possible values are
 * <ul>
 * <li>"wcc": weakly connected clusters</li>
 * <li>"scc": strongly connected clusters</li>
 * </ul>
 * Defaults to "wcc".
 * @config
 */
private static final String PAR_TYPE = "type";

//--------------------------------------------------------------------------
//Fields
//--------------------------------------------------------------------------

/** {@link #PAR_STATS} */
private final boolean sizestats;

/** {@link #PAR_TYPE} */
private final String type;

//--------------------------------------------------------------------------
//Initialization
//--------------------------------------------------------------------------

/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param name the configuration prefix for this class
 */
public ConnectivityObserver(String name)
{
	super(name);
	sizestats = Configuration.contains(name + "." + PAR_STATS);
	type = Configuration.getString(name + "." + PAR_TYPE,"wcc");
}

//--------------------------------------------------------------------------
//Methods
//--------------------------------------------------------------------------

/**
* Prints information about clusters.
* If parameter {@value #PAR_STATS} is defined then the output is
* produced by {@link IncrementalStats#toString}, over the sizes of the
* clusters.
* Otherwise one line is printed that contains the string representation of
* a map, that holds cluster IDs mapped to cluster sizes.
* The meaning of the cluster IDs is not specified, but is printed for
* debugging purposes.
* @return always false
* @see peersim.graph.GraphAlgorithms#tarjan
* @see peersim.graph.GraphAlgorithms#weaklyConnectedClusters
*/
public boolean execute()
{
	Map clst;
	updateGraph();
	
	if(type.equals("wcc"))
		clst=ga.weaklyConnectedClusters(g);
	else if(type.equals("scc"))
		clst=ga.tarjan(g);
	else
		throw new RuntimeException(
		"Unsupported connted cluster type '"+type+"'");

	if (!sizestats) {
		System.out.println(name + ": " + clst);
	} else {
		IncrementalStats stats = new IncrementalStats();
		Iterator it = clst.values().iterator();
		while (it.hasNext()) {
			stats.add(((Integer) it.next()).intValue());
		}
		System.out.println(name + ": " + stats);
	}
	return false;
}

}
