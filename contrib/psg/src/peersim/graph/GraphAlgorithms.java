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
* Implements graph algorithms. The current implementation is NOT thread
* safe. Some algorithms are not static, many times the result of an
* algorithm can be read from non-static fields.
*/
public class GraphAlgorithms {

// =================== public fields ==================================
// ====================================================================

/** output of some algorithms is passed here */
public int[] root = null;
private Stack<Integer> stack = new Stack<Integer>();
private int counter=0;

private Graph g=null;

public final static int WHITE=0;
public final static int GREY=1;
public final static int BLACK=2;

/** output of some algorithms is passed here */
public int[] color = null;

/** output of some algorithms is passed here */
public Set<Integer> cluster = null;

/** output of some algorithms is passed here */
public int[] d = null;

// =================== private methods ================================
// ====================================================================


/**
* Collects nodes accessible from node "from" using depth-first search.
* Works on the array {@link #color} which must be of the same length as
* the size of the graph, and must contain values according to the
* following semantics: 
* WHITE (0): not seen yet, GREY (1): currently worked upon. BLACK
* (other than 0 or 1): finished.
* If a negative color is met, it is saved in the {@link #cluster} set
* and is treated as black. This can be used to check if the currently
* visited cluster is weakly connected to another cluster.
* On exit no nodes are GREY.
* The result is the modified array {@link #color} and the modified set
* {@link #cluster}.
*/
private void dfs( int from ) {

	color[from]=GREY;

	for(int j:g.getNeighbours(from))
	{
		if( color[j]==WHITE )
		{
			dfs(j);
		}
		else
		{
			if( color[j]<0 ) cluster.add(color[j]);
		}
	}

	color[from]=BLACK;
}

// --------------------------------------------------------------------

/**
* Collects nodes accessible from node "from" using breadth-first search.
* Its parameters and side-effects are identical to those of dfs.
* In addition, it stores the shortest distances from "from" in {@link #d},
* if it is not null. On return, <code>d[i]</code> contains the length of
* the shortest path from "from" to "i", if such a path exists, or it is
* unchanged (ie the original value of <code>d[i]</code> is kept,
* whatever that was.
* <code>d</code> must either be long enough or null.
*/
private void bfs( int from ) {

	List<Integer> q = new LinkedList<Integer>();
	int u, du;
	
	q.add(from);
	q.add(0);
	if( d != null ) d[from] = 0;

	color[from]=GREY;

	while( ! q.isEmpty() )
	{
		u = q.remove(0).intValue();
		du = q.remove(0).intValue();
		
		for(int j:g.getNeighbours(u))
		{
			if( color[j]==WHITE )
			{
				color[j]=GREY;
				
				q.add(j);
				q.add(du+1);
				if( d != null ) d[j] = du+1;
			}
			else
			{
				if( color[j]<0 )
					cluster.add(color[j]);
			}
		}
		color[u]=BLACK;
	}
}

// --------------------------------------------------------------------

/** The recursive part of the Tarjan algorithm. */
private void tarjanVisit(int i) {

	color[i]=counter++;
	root[i]=i;
	stack.push(i);
	
	for(int j:g.getNeighbours(i))
	{
		if( color[j]==WHITE )
		{
			tarjanVisit(j);
		}
		if( color[j]>0 && color[root[j]]<color[root[i]] )
		// inComponent is false and have to update root
		{
			root[i]=root[j];
		}
	}

	int j;
	if(root[i]==i) //this node is the root of its cluster
	{
		do
		{
			j=stack.pop();
			color[j]=-color[j];
			root[j]=i;
		}
		while(j!=i);
	}
}

// =================== public methods ================================
// ====================================================================

/** Returns the weakly connected cluster indexes with size as a value.
* Cluster membership can be seen from the content of the array {@link #color};
* each node has the cluster index as color. The cluster indexes carry no
* information; we guarantee only that different clusters have different indexes.
*/
public Map weaklyConnectedClusters( Graph g ) {

	this.g=g;
	if( cluster == null ) cluster = new HashSet<Integer>();
	if( color==null || color.length<g.size() ) color = new int[g.size()];

	// cluster numbers are negative integers
	int i, j, actCluster=0;
	for(i=0; i<g.size(); ++i) color[i]=WHITE;
	for(i=0; i<g.size(); ++i)
	{
		if( color[i]==WHITE )
		{
			cluster.clear();
			bfs(i); // dfs is recursive, for large graphs not ok
			--actCluster;
			for(j=0; j<g.size(); ++j)
			{
				if( color[j] == BLACK ||
						cluster.contains(color[j]) )
					color[j] = actCluster;
			}
		}
	}

	Hashtable<Integer,Integer> ht = new Hashtable<Integer,Integer>();
	for(j=0; j<g.size(); ++j)
	{
		Integer num = ht.get(color[j]);
		if( num == null ) ht.put(color[j],Integer.valueOf(1));
		else ht.put(color[j],num+1);
	}
	
	return ht;
}

// --------------------------------------------------------------------

/**
* In <code>{@link #d}[j]</code> returns the length of the shortest path between
* i and j. The value -1 indicates that j is not accessible from i.
*/
public void dist( Graph g, int i ) {

	this.g=g;
	if( d==null || d.length<g.size() ) d = new int[g.size()];
	if( color==null || color.length<g.size() ) color = new int[g.size()];
	
	for(int j=0; j<g.size(); ++j)
	{
		color[j]=WHITE;
		d[j] = -1;
	}
	
	bfs(i);
}

// --------------------------------------------------------------------

/**
* Calculates the clustering coefficient for the given node in the given
* graph. The clustering coefficient is the number of edges between
* the neighbours of i divided by the number of possible edges.
* If the graph is directed, an exception is thrown.
* If the number of neighbours is 1, returns 1. For zero neighbours
* returns NAN.
* @throws IllegalArgumentException if g is directed
*/
public static double clustering( Graph g, int i ) {

	if( g.directed() ) throw new IllegalArgumentException(
		"graph is directed");
		
	Object[] n = g.getNeighbours(i).toArray();
	
	if( n.length==1 ) return 1.0;
	
	int edges = 0;
	
	for(int j=0; j<n.length; ++j)
	for(int k=j+1; k<n.length; ++k)
		if( g.isEdge((Integer)n[j],(Integer)n[k]) ) ++edges;

	return ((edges*2.0)/n.length)/(n.length-1);
}

// --------------------------------------------------------------------

/**
* Performs anti-entropy epidemic multicasting from node 0.
* As a result the number of nodes that have been reached in cycle i
* is put into <code>b[i]</code>.
* The number of cycles performed is determined by <code>b.length</code>.
* In each cycle each node contacts a random neighbour and exchanges
* information. The simulation is generational: when a node contacts a neighbor
* in cycle i, it sees their state as in cycle i-1, besides, all nodes update
* their state at the same time point, synchronously.
*/
public static void multicast( Graph g, int[] b, Random r ) {

	int c1[] = new int[g.size()];
	int c2[] = new int[g.size()];
	for(int i=0; i<c1.length; ++i) c2[i]=c1[i]=WHITE;
	c2[0]=c1[0]=BLACK;
	Collection<Integer> neighbours=null;
	int black=1;
	
	int k=0;
	for(; k<b.length || black<g.size(); ++k)
	{
		for(int i=0; i<c2.length; ++i)
		{
			neighbours=g.getNeighbours(i);
			Iterator<Integer> it=neighbours.iterator();
			for(int j=r.nextInt(neighbours.size()); j>0; --j)
				it.next();
			int randn = it.next();
			
			// push pull exchane with random neighbour
			if( c1[i]==BLACK ) //c2[i] is black too
			{
				if(c2[randn]==WHITE) ++black;
				c2[randn]=BLACK;
			}
			else if( c1[randn]==BLACK )
			{
				if(c2[i]==WHITE) ++black;
				c2[i]=BLACK;
			}
		}
		System.arraycopy(c2,0,c1,0,c1.length);
		b[k]=black;
	}
	
	for(; k<b.length; ++k) b[k]=g.size();
}

// --------------------------------------------------------------------

/**
* Performs flooding from given node.
* As a result <code>b[i]</code> contains the number of nodes
* reached in exactly i steps, and always <code>b[0]=1</code>.
* If the maximal distance from k is lower than <code>b.length</code>,
* then the remaining elements of b are zero.
*/
public void flooding( Graph g, int[] b, int k ) {

	dist(g, k);

	for(int i=0; i<b.length; ++i) b[i]=0;
	for(int i=0; i<d.length; ++i)
	{
		if( d[i] >= 0 && d[i] < b.length ) b[d[i]]++;
	}
}

// --------------------------------------------------------------------

/** Returns the strongly connected cluster roots with size as a value.
* Cluster membership can be seen from the content of the array {@link #root};
* each node has the root of the strongly connected cluster it belongs to.
* A word of caution: for large graphs that have a large diameter and that
* are strongly connected (such as large rings) you can get stack overflow
* because of the large depth of recursion.
*/
//XXX implement a non-recursive version ASAP!!!
public Map tarjan( Graph g ) {
	
	this.g=g;
	stack.clear();
	if( root==null || root.length<g.size() ) root = new int[g.size()];
	if( color==null || color.length<g.size() ) color = new int[g.size()];
	for( int i=0; i<g.size(); ++i) color[i]=WHITE;
	counter = 1;
	
	// color is WHITE (0): not visited
	// not WHITE, positive (c>1): visited as the c-th node
	// color is negative (c<1): inComponent true
	for(int i=0; i<g.size(); ++i)
	{
		if( color[i]==WHITE ) tarjanVisit(i);
	}
	for( int i=0; i<g.size(); ++i) color[i]=0;
	for( int i=0; i<g.size(); ++i) color[root[i]]++;
	Hashtable<Integer,Integer> ht = new Hashtable<Integer,Integer>();
	for(int j=0; j<g.size(); ++j)
	{
		if(color[j]>0)
		{
			ht.put(j,color[j]);
		}
	}
	
	return ht;
}

}

