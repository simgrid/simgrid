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

/*
 * Created on Jan 30, 2005 by Spyros Voulgaris
 *
 */
package peersim.graph;

import java.util.ArrayList;
import java.util.BitSet;

/**
* Speeds up {@link ConstUndirGraph#isEdge} by storing the links in an
* adjacency matrix (in fact in a triangle).
* Its memory consumption is huge but it's much faster if the isEdge method
* of the original underlying graph is slow.
*/
public class FastUndirGraph extends ConstUndirGraph
{

private BitSet[] triangle;


// ======================= initializarion ==========================
// =================================================================


/** Calls super constructor */
public FastUndirGraph(Graph graph)
{
	super(graph);
}

// -----------------------------------------------------------------

protected void initGraph()
{
	final int max = g.size();
	triangle = new BitSet[max];
	for (int i=0; i<max; ++i)
	{
		in[i] = new ArrayList<Integer>();
		triangle[i] = new BitSet(i);
	}

	for(int i=0; i<max; ++i)
	{
		for(Integer out:g.getNeighbours(i))
		{
			int j=out.intValue();
			if( ! g.isEdge(j,i) )
				in[j].add(i);
			// But always add the link to the triangle
			if (i>j) // make sure i>j
				triangle[i].set(j);
			else
				triangle[j].set(i);
		}
	}
}


// ============================ Graph functions ====================
// =================================================================


public boolean isEdge(int i, int j)
{
	// make sure i>j
	if (i<j)
	{
		int ii=i;
		i=j;
		j=ii;
	}
	return triangle[i].get(j);
}
}

