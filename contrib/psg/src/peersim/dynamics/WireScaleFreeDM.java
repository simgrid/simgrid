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

package peersim.dynamics;

import peersim.graph.Graph;
import peersim.config.*;
import peersim.core.*;

/**
 * Wires a scale free graph using a method described in
 * <a href="http://xxx.lanl.gov/abs/cond-mat/0106144">this paper</a>.
 * It is an incremental technique, where the new nodes are connected to
 * the two ends of an edge that is already in the network.
 * This model always wires undirected links.
 */
public class WireScaleFreeDM extends WireGraph {


//--------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------

/** 
 * The number of edges added to each new
 * node (apart from those forming the initial network) is twice this
 * value.
 * @config
 */
private static final String PAR_EDGES = "k";


//--------------------------------------------------------------------------
// Fields
//--------------------------------------------------------------------------


/** The number of edges created for a new node is 2*k. */	
private final int k;


//--------------------------------------------------------------------------
// Constructor
//--------------------------------------------------------------------------

/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param prefix the configuration prefix for this class
 */
public WireScaleFreeDM(String prefix)
{
	super(prefix);
	k = Configuration.getInt(prefix + "." + PAR_EDGES);
}


//--------------------------------------------------------------------------
// Methods
//--------------------------------------------------------------------------

/**
 * Wires a scale free graph using a method described in
 * <a href="http://xxx.lanl.gov/abs/cond-mat/0106144">this paper</a>.
 * It is an incremental technique, where the new nodes are connected to
 * the two ends of an edge that is already in the network.
 * This model always wires undirected links.
*/
public void wire(Graph g) {

	int nodes=g.size();
	int[] links = new int[4*k*nodes];

	// Initial number of nodes connected as a clique
	int clique = (k > 3 ? k : 3);

	// Add initial edges, to form a clique
	int len=0;
	for (int i=0; i < clique; i++)
	for (int j=0; j < clique; j++)
	{
		if (i != j)
		{
			g.setEdge(i,j);
			g.setEdge(j,i);
			links[len*2] = i;
			links[len*2+1] = j;
			len++;
		}
	}

	for (int i=clique; i < nodes; i++)
	for (int l=0; l < k; l++)
	{
		int edge = CommonState.r.nextInt(len);
		int m = links[edge*2];
		int j = links[edge*2+1];
		g.setEdge(i, m);
		g.setEdge(m, i);
		g.setEdge(j, m);
		g.setEdge(m, j);
		links[len*2] = i;
		links[len*2+1] = m;
		len++;
		links[len*2] = j;
		links[len*2+1] = m;
		len++;
	}
}
		
}
