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
		
package peersim.graph;

import java.util.*;

/**
* Contains static methods for wiring certain kinds of graphs. The general
* contract of all methods is that they accept any graph and add edges
* as specified in the documentation.
*/
public class GraphFactory {

/** Disable instance construction */
private GraphFactory() {}

// ===================== public static methods ======================
// ==================================================================

/**
* Wires a ring lattice.
* The added connections are defined as follows. If k is even, links to
* i-k/2, i-k/2+1, ..., i+k/2 are added (but not to i), thus adding an
* equal number of predecessors and successors.
* If k is odd, then we add one more successors than predecessors.
* For example, for k=4: 2 predecessors, 2 successors.
* For k=5: 2 predecessors, 3 successors.
* For k=1: each node is linked only to its successor.
* All values are understood mod n to make the lattice circular, where n is the
* number of nodes in g.
* @param g the graph to be wired
* @param k lattice parameter
* @return returns g for convenience
*/
public static Graph wireRingLattice(Graph g, int k) {
	
	final int n = g.size();

	int pred = k/2;
	int succ = k-pred;

	for(int i=0; i<n; ++i)
	for(int j=-pred; j<=succ; ++j)
	{
		if( j==0 ) continue;
		final int v = (i+j+n)%n;
		g.setEdge(i,v);
	}
	return g;
}

// -------------------------------------------------------------------

/**
* Watts-Strogatz model. A bit modified though: by default assumes a directed
* graph. This means that directed
* links are re-wired, and the undirected edges in the original (undirected)
* lattice are modeled
* by double directed links pointing in opposite directions. Rewiring is done
* with replacement, so the possibility of wiring two links to the same target
* is positive (though very small).
* <p>
* Note that it is possible to pass an undirected graph as a parameter. In that
* case the output is the directed graph produced by the method, converted to
* an undirected graph by dropping directionality of the edges. This graph is
* still not from the original undirected WS model though.
* @param g the graph to be wired
* @param k lattice parameter: this is the out-degree of a node in the
* ring lattice before rewiring
* @param p the probability of rewiring each 
* @param r source of randomness
* @return returns g for convenience
*/
public static Graph wireWS( Graph g, int k, double p, Random r ) {
//XXX unintuitive to call it WS due to the slight mods
	final int n = g.size();
	for(int i=0; i<n; ++i)
	for(int j=-k/2; j<=k/2; ++j)
	{
		if( j==0 ) continue;
		int newedge = (i+j+n)%n;
		if( r.nextDouble() < p )
		{
			newedge = r.nextInt(n-1);
			if( newedge >= i ) newedge++; // random _other_ node
		}
		g.setEdge(i,newedge);
	}
	return g;
}

// -------------------------------------------------------------------

/**
* Random graph. Generates randomly k directed edges out of each node.
* The neighbors
* (edge targets) are chosen randomly without replacement from the nodes of the
* graph other than the source node (i.e. no loop edge is added).
* If k is larger than N-1 (where N is the number of nodes) then k is set to
* be N-1 and a complete graph is returned.
* @param g the graph to be wired
* @param k samples to be drawn for each node
* @param r source of randomness
* @return returns g for convenience
*/
public static Graph wireKOut( Graph g, int k, Random r ) {

	final int n = g.size();
	if( n < 2 ) return g;
	if( n <= k ) k=n-1;
	int[] nodes = new int[n];
	for(int i=0; i<nodes.length; ++i) nodes[i]=i;
	for(int i=0; i<n; ++i)
	{
		int j=0;
		while(j<k)
		{
			int newedge = j+r.nextInt(n-j);
			int tmp = nodes[j];
			nodes[j] = nodes[newedge];
			nodes[newedge] = tmp;
			if( nodes[j] != i )
			{
				g.setEdge(i,nodes[j]);
				j++;
			}
		}
	}
	return g;
}

// -------------------------------------------------------------------

/**
* A sink star.
* Wires a sink star topology adding a link to 0 from all other nodes.
* @param g the graph to be wired
* @return returns g for convenience
*/
public static Graph wireStar( Graph g ) {

	final int n = g.size();
	for(int i=1; i<n; ++i) g.setEdge(i,0);
	return g;
}

// -------------------------------------------------------------------

/**
* A regular rooted tree.
* Wires a regular rooted tree. The root is 0, it has links to 1,...,k.
* In general, node i has links to i*k+1,...,i*k+k.
* @param g the graph to be wired
* @param k the number of outgoing links of nodes in the tree (except
* leaves that have zero out-links, and exactly one node that might have
* less than k).
* @return returns g for convenience
*/
public static Graph wireRegRootedTree( Graph g, int k ) {

	if( k==0 ) return g;
	final int n = g.size();
	int i=0; // node we wire
	int j=1; // next free node to link to
	while(j<n)
	{
		for(int l=0; l<k && j<n; ++l,++j) g.setEdge(i,j);
		++i;
	}
	return g;
}

// -------------------------------------------------------------------

/**
* A hypercube.
* Wires a hypercube.
* For a node i the following links are added: i xor 2^0, i xor 2^1, etc.
* this define a log(graphsize) dimensional hypercube (if the log is an
* integer).
* @param g the graph to be wired
* @return returns g for convenience
*/
public static Graph wireHypercube( Graph g ) {

	final int n = g.size();
	if(n<=1) return g;
	final int highestone = Integer.highestOneBit(n-1); // not zero
	for(int i=0; i<n; ++i)
	{
		int mask = highestone;
		while(mask>0)
		{
			int j = i^mask;
			if(j<n) g.setEdge(i,j);
			mask = mask >> 1;
		}
		
	}
	return g;
}

// -------------------------------------------------------------------

/**
* This contains the implementation of the Barabasi-Albert model
* of growing scale free networks. The original model is described in
* <a href="http://arxiv.org/abs/cond-mat/0106096">
http://arxiv.org/abs/cond-mat/0106096</a>.
* It also works if the graph is directed, in which case the model is a
* variation of the BA model
* described in <a href="http://arxiv.org/pdf/cond-mat/0408391">
http://arxiv.org/pdf/cond-mat/0408391</a>. In both cases, the number of the
* initial set of nodes is the same as the degree parameter, and no links are
* added. The first added node is connected to all of the initial nodes,
* and after that the BA model is used normally.
* @param k the number of edges that are generated for each new node, also
* the number of initial nodes (that have no edges).
* @param r the randomness to be used
* @return returns g for convenience
*/
public static Graph wireScaleFreeBA( Graph g, int k, Random r ) {

	final int nodes = g.size();
	if( nodes <= k ) return g;
	
	// edge i has ends (ends[2*i],ends[2*i+1])
	int[] ends = new int[2*k*(nodes-k)];
	
	// Add initial edges from k to 0,1,...,k-1
	for(int i=0; i < k; i++)
	{
		g.setEdge(k,i);
		ends[2*i]=k;
		ends[2*i+1]=i;
	}
	
	int len = 2*k; // edges drawn so far is len/2
	for(int i=k+1; i < nodes; i++) // over the remaining nodes
	{
		for (int j=0; j < k; j++) // over the new edges
		{
			int target;
			do
			{
				target = ends[r.nextInt(len)]; 
				int m=0;
				while( m<j && ends[len+2*m+1]!=target) ++m;
				if(m==j) break;
				// we don't check in the graph because
				// this wire method should accept graphs
				// that already have edges.
			}
			while(true);
			g.setEdge(i,target);
			ends[len+2*j]=i;
			ends[len+2*j+1]=target;
		}
		len += 2*k;
	}

	return g;
}

// -------------------------------------------------------------------
/*
public static void main(String[] pars) {
	
	int n = Integer.parseInt(pars[0]);
	//int k = Integer.parseInt(pars[1]);
	Graph g = new BitMatrixGraph(n);
	
	//wireWS(g,20,.1,new Random());
	//GraphIO.writeChaco(new UndirectedGraph(g),System.out);
	
	//wireScaleFreeBA(g,3,new Random());
	//wireKOut(g,k,new Random());
	//wireRegRootedTree(g,k);
	wireHypercube(g);
	GraphIO.writeNeighborList(g,System.out);
}
*/
}

