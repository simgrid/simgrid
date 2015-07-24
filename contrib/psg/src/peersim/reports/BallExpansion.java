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

import peersim.config.*;
import peersim.core.*;
import peersim.util.*;

/**
 * Control to observe the ball expansion, that is,
 * the number of nodes that are
 * accessible from a given node in at most 1, 2, etc steps.
 */
public class BallExpansion extends GraphObserver
{

// ===================== fields =======================================
// ====================================================================

/**
 * This parameter defines the maximal distance we care about.
 * In other words, two nodes are further away, they will not be taken
 * into account when calculating statistics.
 * <p>
 * Defaults to the
 * network size (which means all distances are taken into account).
 * Note that this default is normally way too much; many low diameter graphs
 * have only short distances between the nodes. Setting a short
 * (but sufficient) distance saves memory.
 * Also note that the <em>initial</em> network
 * size is used if no value is given which might not be what you want if e.g. the
 * network is growing.
 * @config
 */
private static final String PAR_MAXD = "maxd";

/**
 * The number of nodes to print info about.
 * Defaults to 1000. If larger than the current network size, then the
 * current network size is used.
 * @config
 */
private static final String PAR_N = "n";

/**
 * If defined, statistics are printed instead over the minimal path lengths. Not
 * defined by default.
 * @config
 */
private static final String PAR_STATS = "stats";

private final int maxd;

private final int n;

private final boolean stats;

/** working variable */
private final int[] b;

private final RandPermutation rp = new RandPermutation(CommonState.r);

// ===================== initialization ================================
// =====================================================================

/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param name the configuration prefix for this class
 */
public BallExpansion(String name)
{
	super(name);
	maxd = Configuration.getInt(name + "." + PAR_MAXD, Network.size());
	n = Configuration.getInt(name + "." + PAR_N, 1000);
	stats = Configuration.contains(name + "." + PAR_STATS);
	b = new int[maxd];
}

// ====================== methods ======================================
// =====================================================================

/**
* Prints information about ball expansion. It uses {@value #PAR_N} nodes to
* collect statistics.
* If parameter {@value #PAR_STATS} is defined then the output is
* produced by {@link IncrementalStats#toString}, over the values of minimal
* distances from the given number of nodes to all other nodes in the network.
* If at least one node is unreachable from any selected starting node, then
* the path length is taken as infinity and statistics are calculated
* accordingly. In that case, {@link IncrementalStats#getMaxCount()} returns
* the number of nodes of "infinite distance", that is, the number of
* unreachable nodes.
* Otherwise one line is printed for all nodes we observe, containing the
* number of nodes at distance 1, 2, etc, separated by spaces.
* In this output format, unreachable nodes are simply ignored, but of course
* the sum of the numbers in one line can be used to detect partitioning if
* necessary.
* Finally, note that the {@value #PAR_N} nodes are not guaranteed to be the
* same nodes over consecutive calls to this method.
* @return always false
*/
public boolean execute() {

	updateGraph();
	System.out.print(name + ": ");
	rp.reset(g.size());
	if (stats)
	{
		IncrementalStats is = new IncrementalStats();
		for (int i = 0; i < n && i < g.size(); ++i)
		{
			ga.dist(g, rp.next());
			for (int j=0; j<g.size(); j++)
			{
				if (ga.d[j] > 0)
					is.add(ga.d[j]);
				else if (ga.d[j] == -1)
					is.add(Double.POSITIVE_INFINITY);
				// deliberately left ga.d[j]==0 out, as we don't
				// want to count trivial distance to oneself.
			}
		}
		System.out.println(is);
	}
	else
	{
		System.out.println();
		for (int i = 0; i < n && i < g.size(); ++i)
		{
			ga.flooding(g, b, rp.next());
			int j = 0;
			while (j < b.length && b[j] > 0)
			{
				System.out.print(b[j++] + " ");
			}
			System.out.println();
		}
	}
	return false;
}

}
