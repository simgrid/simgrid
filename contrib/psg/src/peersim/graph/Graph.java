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

import java.util.Collection;

/**
* A general graph interface. It follows the following model:
* the graph has n nodes which are indexed from 0 to n-1.
* The parameters of operators refer to indices only.
* Implementations might return objects that represent the
* nodes or edges, although this is not required.
*
* Undirected graphs are modelled by the interface as directed graphs in which
* every edge (i,j) has a corresponding reverse edge (j,i).
*/
public interface Graph {

	/**
	* Returns true if there is a directed edge between node i
	* and node j.
	*/
	boolean isEdge(int i, int j);

	/**
	* Returns a collection view to all outgoing edges from
	* i. The collection should ideally be unmodifiable.
	* In any case, modifying the returned collection is not safe and
	* may result in unspecified behavior.
	*/
	Collection<Integer> getNeighbours(int i);

	/**
	* Returns the node object associated with the index. Optional
	* operation.
	*/
	Object getNode(int i);
	
	/**
	* Returns the edge object associated with the index. Optional
	* operation.
	*/
	Object getEdge(int i, int j);

	/**
	* The number of nodes in the graph.
	*/
	int size();

	/**
	* Returns true if the graph is directed otherwise false.
	*/
	boolean directed();

	/**
	* Sets given edge, returns true if it did not exist before.
	* If the graph is
	* undirected, sets the edge (j,i) as well. Optional operation.
	*/
	public boolean setEdge(int i, int j);

	/**
	* Removes given edge, returns true if it existed before. If the graph is
	* undirected, removes the edge (j,i) as well. Optional operation.
	*/
	public boolean clearEdge(int i, int j);

	/**
	* Returns the degree of the given node. If the graph is directed,
	* returns out degree.
	*/
	public int degree(int i);
}
