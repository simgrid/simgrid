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
 * Prints several statistics about the node degrees in the graph.
 */
public class DegreeStats extends GraphObserver
{

//--------------------------------------------------------------------------
//Parameter
//--------------------------------------------------------------------------

/**
 * The number of nodes to be used for sampling the degree. 
 * Defaults to full size of the graph.
 * @config
 */
private static final String PAR_N = "n";

/**
 * If defined, then the given number of nodes will be traced. That is, it is
 * guaranteed that in each call the same nodes will be picked in the same order.
 * If a node being traced fails, its degree will be considered 0. Not defined by
 * default.
 * @config
 */
private static final String PAR_TRACE = "trace";

/**
 * Selects a method to use when printing results. Three methods are known:
 * "stats" will use {@link IncrementalStats#toString}. "freq" will
 * use {@link IncrementalFreq#print}. "list" will print the
 * degrees of the sample nodes one by one in one line, separated by spaces.
 * Default is "stats".
 * @config
 */
private static final String PAR_METHOD = "method";

/**
 * Selects the types of links to print information about. Three methods are
 * known: "live": links pointing to live nodes, "dead": links pointing to nodes
 * that are unavailable and "all": both dead and live links summed. "all" and
 * "dead" require parameter {@value peersim.reports.GraphObserver#PAR_UNDIR} 
 * to be unset (graph must be directed). Default is "live".
 * @config
 */
private static final String PAR_TYPE = "linktype";

//--------------------------------------------------------------------------
//Parameter
//--------------------------------------------------------------------------

private final int n;

private final boolean trace;

private Node[] traced = null;

private final String method;

private final String type;

private final RandPermutation rp = new RandPermutation(CommonState.r);

private int nextnode = 0;

//--------------------------------------------------------------------------
//Initialization
//--------------------------------------------------------------------------

/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param name the configuration prefix for this class
 */
public DegreeStats(String name)
{
	super(name);
	n = Configuration.getInt(name + "." + PAR_N, -1);
	trace = Configuration.contains(name + "." + PAR_TRACE);
	method = Configuration.getString(name + "." + PAR_METHOD, "stats");
	type = Configuration.getString(name + "." + PAR_TYPE, "live");
	if ((type.equals("all") || type.equals("dead")) && undir) {
		throw new IllegalParameterException(
			name + "." + PAR_TYPE, " Parameter "+ name + "." +
			PAR_UNDIR + " must not be defined if " + name + "."
			+ PAR_TYPE + "=" + type + ".");
	}
}

//--------------------------------------------------------------------------
//Methods
//--------------------------------------------------------------------------

/**
 * Returns next node to get degree information about.
 */
private int nextNodeId()
{
	if (trace) {
		if (traced == null) {
			int nn = (n < 0 ? Network.size() : n);
			traced = new Node[nn];
			for (int j = 0; j < nn; ++j)
				traced[j] = Network.get(j);
		}
		return traced[nextnode++].getIndex();
	} else
		return rp.next();
}

// ---------------------------------------------------------------------

/**
 * Returns degree information about next node.
 */
private int nextDegree()
{
	final int nodeid = nextNodeId();
	if (type.equals("live")) {
		return g.degree(nodeid);
	} else if (type.equals("all")) {
		return ((OverlayGraph) g).fullDegree(nodeid);
	} else if (type.equals("dead")) {
		return ((OverlayGraph) g).fullDegree(nodeid) - g.degree(nodeid);
	} else
		throw new RuntimeException(name + ": invalid type");
}

// ---------------------------------------------------------------------

/**
 * Prints statistics about node degree. The format of the output is specified
 * by {@value #PAR_METHOD}. See also the rest of the configuration parameters.
 * @return always false
 */
public boolean execute()
{
	updateGraph();
	if (!trace)
		rp.reset(g.size());
	else
		nextnode = 0;
	final int nn = (n < 0 ? Network.size() : n);
	if (method.equals("stats")) {
		IncrementalStats stats = new IncrementalStats();
		for (int i = 0; i < nn; ++i)
			stats.add(nextDegree());
		System.out.println(name + ": " + stats);
	} else if (method.equals("freq")) {
		IncrementalFreq stats = new IncrementalFreq();
		for (int i = 0; i < nn; ++i)
			stats.add(nextDegree());
		stats.print(System.out);
		System.out.println("\n\n");
	} else if (method.equals("list")) {
		System.out.print(name + ": ");
		for (int i = 0; i < nn; ++i)
			System.out.print(nextDegree() + " ");
		System.out.println();
	}
	return false;
}

}
