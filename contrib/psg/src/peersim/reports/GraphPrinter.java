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

import peersim.config.Configuration;
import peersim.graph.GraphIO;
import peersim.util.FileNameGenerator;
import java.io.PrintStream;
import java.io.FileOutputStream;
import java.io.IOException;

/**
* Prints the whole graph in a given format.
*/
public class GraphPrinter extends GraphObserver {


// ===================== fields =======================================
// ====================================================================

/**
* This is the prefix of the filename where the graph is saved.
* The extension is ".graph" and after the prefix the basename contains
* a numeric index that is incremented at each saving point.
* If not given, the graph is dumped on the standard output.
* @config
*/
private static final String PAR_BASENAME = "outf";

/**
* The name for the format of the output. Defaults to "neighborlist",
* which is a plain dump of neighbors. The class
* {@link peersim.dynamics.WireFromFile} can read this format.
* Other supported formats are "chaco" to be used with Yehuda Koren's
* Embedder, "netmeter" to be used with Sergi Valverde's netmeter and also
* with pajek,
* "edgelist" that dumps one (directed) node pair in each line for each edge,
* "gml" that is a generic format of many graph tools, and "dot" that can
* be used with the graphviz package.
* @see GraphIO#writeEdgeList
* @see GraphIO#writeChaco
* @see GraphIO#writeNeighborList
* @see GraphIO#writeNetmeter
* @config
*/
private static final String PAR_FORMAT = "format";

private final String baseName;

private final FileNameGenerator fng;

private final String format;


// ===================== initialization ================================
// =====================================================================


/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param name the configuration prefix for this class
 */
public GraphPrinter(String name) {

	super(name);
	baseName = Configuration.getString(name+"."+PAR_BASENAME,null);
	format = Configuration.getString(name+"."+PAR_FORMAT,"neighborlist");
	if(baseName!=null) fng = new FileNameGenerator(baseName,".graph");
	else fng = null;
}


// ====================== methods ======================================
// =====================================================================


/**
* Saves the graph according to the specifications in the configuration.
* @return always false
*/
public boolean execute() {
try {	
	updateGraph();
	
	System.out.print(name+": ");
	
	// initialize output streams
	FileOutputStream fos = null;
	PrintStream pstr = System.out;
	if( baseName != null )
	{
		String fname = fng.nextCounterName();
		fos = new FileOutputStream(fname);
		System.out.println("writing to file "+fname);
		pstr = new PrintStream(fos);
	}
	else	System.out.println();
	
	if( format.equals("neighborlist") )
		GraphIO.writeNeighborList(g, pstr);
	else if( format.equals("edgelist") )
		GraphIO.writeEdgeList(g, pstr);
	else if( format.equals("chaco") )
		GraphIO.writeChaco(g, pstr);
	else if( format.equals("netmeter") )
		GraphIO.writeNetmeter(g, pstr);
	else if( format.equals("gml") )
		GraphIO.writeGML(g, pstr);
	else if( format.equals("dot") )
		GraphIO.writeDOT(g, pstr);
	else
		System.err.println(name+": unsupported format "+format);
	
	if( fos != null ) fos.close();
	
	return false;
}
catch( IOException e )
{
	throw new RuntimeException(e);
}
}

}

