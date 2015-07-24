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
* Implements a graph which uses the neighbour list representation.
* No multiple edges are allowed. The implementation also supports the
* growing of the graph. This is very useful when the number of nodes is
* not known in advance or when we construct a graph reading a file.
*/
public class NeighbourListGraph implements Graph, java.io.Serializable {

// =================== private fields ============================
// ===============================================================

/** Contains the objects associated with the node indeces.*/
private final ArrayList<Object> nodes;

/**
* Contains the indices of the nodes. The vector "nodes" contains this
* information implicitly but this way we can find indexes in log time at
* the cost of memory (node that the edge lists typically use much more memory
* than this anyway). Note that the nodes vector is still necessary to
* provide constant access to nodes based on indexes.
*/
private final HashMap<Object,Integer> nodeindex;

/** Contains sets of node indexes. If "nodes" is not null, indices are 
* defined by "nodes", otherwise they correspond to 0,1,... */
private final ArrayList<Set<Integer>> neighbors;

/** Indicates if the graph is directed. */
private final boolean directed;

// =================== public constructors ======================
// ===============================================================

/**
* Constructs an empty graph. That is, the graph has zero nodes, but any
* number of nodes and edges can be added later.
* @param directed if true the graph will be directed
*/
public NeighbourListGraph( boolean directed ) {

	nodes = new ArrayList<Object>(1000);	
	neighbors = new ArrayList<Set<Integer>>(1000);
	nodeindex = new HashMap<Object,Integer>(1000);
	this.directed = directed;
}

// ---------------------------------------------------------------

/**
* Constructs a graph with a fixed size without edges. If the graph is
* constructed this way, it is not possible to associate objects to nodes,
* nor it is possible to grow the graph using {@link #addNode}.
* @param directed if true the graph will be directed
*/
public NeighbourListGraph( int size, boolean directed ) {

	nodes = null;
	neighbors = new ArrayList<Set<Integer>>(size);
	for(int i=0; i<size; ++i) neighbors.add(new HashSet<Integer>());
	nodeindex = null;
	this.directed = directed;
}

// =================== public methods =============================
// ================================================================

/**
* If the given object is not associated with a node yet, adds a new
* node. Returns the index of the node. If the graph was constructed to have
* a specific size, it is not possible to add nodes and therefore calling
* this method will throw an exception.
* @throws NullPointerException if the size was specified at construction time.
*/
public int addNode( Object o ) {

	Integer index = nodeindex.get(o);
	if( index == null )
	{
		index = nodes.size();
		nodes.add(o);
		neighbors.add(new HashSet<Integer>());
		nodeindex.put(o,index);
	}

	return index;
}


// =================== graph implementations ======================
// ================================================================


public boolean setEdge( int i, int j ) {
	
	boolean ret = neighbors.get(i).add(j);
	if( ret && !directed ) neighbors.get(j).add(i);
	return ret;
}

// ---------------------------------------------------------------

public boolean clearEdge( int i, int j ) {
	
	boolean ret = neighbors.get(i).remove(j);
	if( ret && !directed ) neighbors.get(j).remove(i);
	return ret;
}

// ---------------------------------------------------------------

public boolean isEdge(int i, int j) {
	
	return neighbors.get(i).contains(j);
}

// ---------------------------------------------------------------

public Collection<Integer> getNeighbours(int i) {
	
	return Collections.unmodifiableCollection(neighbors.get(i));
}

// ---------------------------------------------------------------

/** If the graph was gradually grown using {@link #addNode}, returns the
* object associated with the node, otherwise null */
public Object getNode(int i) { return (nodes==null?null:nodes.get(i)); }
	
// ---------------------------------------------------------------

/**
* Returns null always. 
*/
public Object getEdge(int i, int j) { return null; }

// ---------------------------------------------------------------

public int size() { return neighbors.size(); }

// --------------------------------------------------------------------
	
public boolean directed() { return directed; }

// --------------------------------------------------------------------

public int degree(int i) { return neighbors.get(i).size(); }
}




