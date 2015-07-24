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
import java.io.*;

/**
* Implements static methods to load and write graphs.
*/
public class GraphIO {
private GraphIO() {}


// ================== public static methods =========================
// ==================================================================


/**
* Prints graph in edge list format. Each line contains exactly two
* node IDs separated by whitespace.
*/
public static void writeEdgeList( Graph g, PrintStream out ) {

	for(int i=0; i<g.size(); ++i)
	{
		Iterator it=g.getNeighbours(i).iterator();
		while(it.hasNext())
		{
			out.println(i+" "+it.next());
		}
	}
}

// ------------------------------------------------------------------

/**
* Prints graph in neighbor list format. Each line starts with the
* id of a node followed by the ids of its neighbors separated by space.
*/
public static void writeNeighborList( Graph g, PrintStream out ) {

	out.println("# "+g.size());
	
	for(int i=0; i<g.size(); ++i)
	{
		out.print(i+" ");
		Iterator it=g.getNeighbours(i).iterator();
		while(it.hasNext())
		{
			out.print(it.next()+" ");
		}
		out.println();
	}
}

// ------------------------------------------------------------------

/**
* Saves the given graph to
* the given stream in DOT format. Good for the graphviz package.
*/
public static void writeDOT( Graph g, PrintStream out ) {

	out.println((g.directed()?"digraph":"graph")+" {");
	
	for(int i=0; i<g.size(); ++i)
	{
		Iterator<Integer> it=g.getNeighbours(i).iterator();
		while(it.hasNext())
		{
			final int j = it.next();
			if(g.directed())
				out.println(i+" -> "+j+";");
			else if( i<=j )
				out.println(i+" -- "+j+";");
		}
	}
	
	out.println("}");
}

// ------------------------------------------------------------------

/**
* Saves the given graph to
* the given stream in GML format.
*/
public static void writeGML( Graph g, PrintStream out ) {

	out.println("graph [ directed "+(g.directed()?"1":"0"));
	
	for(int i=0; i<g.size(); ++i)
		out.println("node [ id "+i+" ]");
	
	for(int i=0; i<g.size(); ++i)
	{
		Iterator it=g.getNeighbours(i).iterator();
		while(it.hasNext())
		{
			out.println(
				"edge [ source "+i+" target "+it.next()+" ]");
		}
	}
	
	out.println("]");
}

// --------------------------------------------------------------------

/**
* Saves the given graph to
* the given stream to be read by NETMETER. It should be ok also for Pajek.
*/
public static void writeNetmeter( Graph g, PrintStream out ) {

	out.println("*Vertices "+g.size());
	for(int i=0; i<g.size(); ++i)
		out.println((i+1)+" \""+(i+1)+"\"");
	
	out.println("*Arcs");
	for(int i=0; i<g.size(); ++i)
	{
		Iterator it=g.getNeighbours(i).iterator();
		while(it.hasNext())
		{
			out.println((i+1)+" "+
				(((Integer)it.next()).intValue()+1)+" 1");
		}
	}
	out.println("*Edges");
}

// --------------------------------------------------------------------

/**
* Saves the given graph to
* the given stream in UCINET DL nodelist format.
*/
public static void writeUCINET_DL( Graph g, PrintStream out ) {

	out.println("DL\nN="+g.size()+"\nFORMAT=NODELIST\nDATA:");
	
	for(int i=0; i<g.size(); ++i)
	{
		out.print(" " + (i+1));
		Iterator it=g.getNeighbours(i).iterator();
		while(it.hasNext())
		{
			out.print(" "+(((Integer)it.next()).intValue()+1));
		}
		out.println();
	}
	out.println();
}

// --------------------------------------------------------------------

/**
* Saves the given graph to
* the given stream in UCINET DL matrix format.
*/
public static void writeUCINET_DLMatrix( Graph g, PrintStream out ) {

	out.println("DL\nN="+g.size()+"\nDATA:");
	
	for(int i=0; i<g.size(); ++i)
	{
		BitSet bs = new BitSet(g.size());
		Iterator it=g.getNeighbours(i).iterator();
		while(it.hasNext())
		{
			bs.set( ((Integer)it.next()).intValue() );
		}
		for(int j=0; j<g.size(); ++j)
		{
			out.print(bs.get(j)?" 1":" 0");
		}
		out.println();
	}
	out.println();
}

// --------------------------------------------------------------------

/**
* Saves the given graph to
* the given stream in Chaco format. We need to output the number of edges
* so they have to be counted first which might not be very efficient.
* Note that this format is designed for undirected graphs only.
*/
public static void writeChaco( Graph g, PrintStream out ) {

	if( g.directed() ) System.err.println(
		"warning: you're saving a directed graph in Chaco format");
	
	long edges = 0;
	for(int i=0; i<g.size(); ++i) edges += g.getNeighbours(i).size();
	
	out.println( g.size() + " " + edges/2 );
	
	for(int i=0; i<g.size(); ++i)
	{
		Iterator it=g.getNeighbours(i).iterator();
		while(it.hasNext())
		{
			out.print((((Integer)it.next()).intValue()+1)+" ");
		}
		out.println();
	}
	
	out.println();
}

// -------------------------------------------------------------------

/**
* Read a graph in newscast graph format.
* The format depends on mode, the parameter.
* The file begins with the three byte latin 1 coded "NCG" string followed
* by the int MODE which is the
* given parameter. The formats are the following as a function of mode:
* <ul>
* <li> 1: Begins with cacheSize in binary format (int), followed by the
*     numberOfNodes (int), and then a continuous series of exactly
*     numberOfNodes records, where a record describes a node's
*     neighbours and their timestamps.
*     A record is a series of exactly cacheSize (int,long) pairs where
*     the int is the node id, and the long is the timestamp.
*     Node id-s start from 1. Node id 0 means no node and used if the parent
*     node has less that cacheSize nodes.</li>
* </ul>
* @param file Filename to read
* @param direction If 0, the original directionality is preserved, if 1,
* than each edge is reversed, if 2 then directionality is dropped and the
* returned graph will be undirected.
*/
public static Graph readNewscastGraph( String file, int direction )
throws IOException {
	
	NeighbourListGraph gr = new NeighbourListGraph( direction != 2 );
	FileInputStream fis = new FileInputStream(file);
	DataInputStream dis = new DataInputStream(fis);

	dis.readByte();
	dis.readByte();
	dis.readByte();
	
	final int MODE = dis.readInt();
	if( MODE != 1 ) throw new IOException("Unknown mode "+MODE);
	
	final int CACHESIZE = dis.readInt(); 
	final int GRAPHSIZE = dis.readInt(); 
	
//System.out.println("header: "+MODE+" "+CACHESIZE+" "+GRAPHSIZE);
	
	for(int i=1; i<=GRAPHSIZE; ++i)
	{
		int iind = gr.addNode(i);
		
		for(int j=0; j<CACHESIZE; ++j)
		{
			int a = dis.readInt();
			dis.readLong();
			
			int agentIndex = gr.addNode(a);
			if( direction == 0 ) gr.setEdge(iind,agentIndex);
			else gr.setEdge(agentIndex,iind);
		}
	}
	
	dis.close();

	return gr;
}


}

