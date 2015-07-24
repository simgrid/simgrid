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
* This class is an adaptor making any Graph an undirected graph
* by making its edges bidirectional. The graph to be made undirected
* is passed to the constructor. Only the reference is stored.
* However, at construction time the incoming edges are stored
* for each node, so if the graph
* passed to the constructor changes over time then
* methods {@link #getNeighbours(int)} and {@link #degree(int)}
* become inconsistent (but only those).
* The upside of this inconvenience is that {@link #getNeighbours} will have
* constant time complexity.
* @see UndirectedGraph
*/
public class ConstUndirGraph implements Graph {


// ====================== private fileds ========================
// ==============================================================


protected final Graph g;

protected final List<Integer>[] in;

// ====================== public constructors ===================
// ==============================================================

/**
* Initialization based on given graph. Stores the graph and if necessary
* (if the graph is directed) searches for the incoming edges and stores
* them too. The given graph is stored by reference (not cloned) so it should
* not be modified while this object is in use.
*/
public ConstUndirGraph( Graph g ) {

	this.g = g;
	if( !g.directed() )
	{
		in = null;
	}
	else
	{
		in = new List[g.size()];
	}
	
	initGraph();
}
	
// --------------------------------------------------------------

/** Finds and stores incoming edges */
protected void initGraph() {

	final int max = g.size();
	for(int i=0; i<max; ++i) in[i] = new ArrayList<Integer>();
	for(int i=0; i<max; ++i)
	{
		for(Integer j:g.getNeighbours(i))
		{
			if( ! g.isEdge(j,i) ) in[j].add(i);
		}
	}
}


// ======================= Graph implementations ================
// ==============================================================


public boolean isEdge(int i, int j) {
	
	return g.isEdge(i,j) || g.isEdge(j,i);
}

// ---------------------------------------------------------------

/**
* Uses sets as collection so does not support multiple edges now, even if
* the underlying directed graph does.
*/
public Collection<Integer> getNeighbours(int i) {
	
	List<Integer> result = new ArrayList<Integer>();
	result.addAll(g.getNeighbours(i));
	if( in != null ) result.addAll(in[i]);
	return Collections.unmodifiableCollection(result);
}

// ---------------------------------------------------------------

/** Returns the node from the underlying graph */
public Object getNode(int i) { return g.getNode(i); }
	
// ---------------------------------------------------------------

/**
* If there is an (i,j) edge, returns that, otherwise if there is a (j,i)
* edge, returns that, otherwise returns null.
*/
public Object getEdge(int i, int j) {
	
	if( g.isEdge(i,j) ) return g.getEdge(i,j);
	if( g.isEdge(j,i) ) return g.getEdge(j,i);
	return null;
}

// ---------------------------------------------------------------

public int size() { return g.size(); }

// --------------------------------------------------------------------
	
public boolean directed() { return false; }

// --------------------------------------------------------------------

/** not supported */
public boolean setEdge( int i, int j ) {
	
	throw new UnsupportedOperationException();
}

// ---------------------------------------------------------------

/** not supported */
public boolean clearEdge( int i, int j ) {
	
	throw new UnsupportedOperationException();
}

// ---------------------------------------------------------------

public int degree(int i) { return g.degree(i)+(in==null?0:in[i].size()); }

// ---------------------------------------------------------------
/*
public static void main( String[] args ) {

	Graph net = new BitMatrixGraph(20);
	GraphFactory.wireKOut(net,5,new Random());
	ConstUndirGraph ug = new ConstUndirGraph(net);
	for(int i=0; i<net.size(); ++i)
		System.err.println(
			i+" "+net.getNeighbours(i)+" "+net.degree(i));
	System.err.println("============");
	for(int i=0; i<ug.size(); ++i)
		System.err.println(i+" "+ug.getNeighbours(i)+" "+ug.degree(i));
	System.err.println("============");
	for(int i=0; i<ug.size(); ++i)
		System.err.println(i+" "+ug.in[i]);
	for(int i=0; i<ug.size(); ++i)
	{
		for(int j=0; j<ug.size(); ++j)
			System.err.print(ug.isEdge(i,j)?"W ":"+ ");
		System.err.println();
	}

	GraphIO.writeUCINET_DL(net,System.out);
	GraphIO.writeUCINET_DL(ug,System.out);
}
*/
}

