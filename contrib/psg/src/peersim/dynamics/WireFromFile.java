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


import java.io.IOException;
import java.io.FileReader;
import java.io.LineNumberReader;
import java.util.StringTokenizer;
import peersim.graph.Graph;
import peersim.core.*;
import peersim.config.Configuration;

/**
* Takes a {@link Linkable} protocol and adds connections that are stored in a
* file. Note that no
* connections are removed, they are only added. So it can be used in
* combination with other initializers.
* The format of the file is as follows. Each line begins with a node ID
* (IDs start from 0) followed by a list of neighbors, separated by whitespace.
* All node IDs larger than the actual network size will be discarded, but
* it does not trigger an error. Lines starting with a "#" character and
* empty lines are ignored.
*/
public class WireFromFile extends WireGraph {


// ========================= fields =================================
// ==================================================================


/** 
*  The filename to load links from.
*  @config
*/
private static final String PAR_FILE = "file";

/** 
*  The number of neighbors to be read from the file. If unset, the default
* behavior is to read all links in the file. If set, then the first k
* neighbors will be read only.
*  @config
*/
private static final String PAR_K = "k";

private final String file;

private final int k;

// ==================== initialization ==============================
// ==================================================================


/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param prefix the configuration prefix for this class
 */
public WireFromFile(String prefix) {

	super(prefix);
	file = Configuration.getString(prefix+"."+PAR_FILE);
	k = Configuration.getInt(prefix + "." + PAR_K, Integer.MAX_VALUE);
}


// ===================== public methods ==============================
// ===================================================================


/**
* Wires the graph from a file.
* The format of the file is as follows. Each line begins with a node ID
* (IDs start from 0) followed by a list of neighbors, separated by whitespace.
* All node IDs larger than the actual network size will be discarded, but
* it does not trigger an error. Lines starting with a "#" character and
* empty lines are ignored.
*/
public void wire(Graph g) {
try
{
	FileReader fr = new FileReader(file);
	LineNumberReader lnr = new LineNumberReader(fr);
	String line;
	boolean wasOutOfRange=false;
	while((line=lnr.readLine()) != null)
	{
		if( line.startsWith("#") ) continue;
		StringTokenizer st = new StringTokenizer(line);
		if(!st.hasMoreTokens()) continue;
		
		final int from = Integer.parseInt(st.nextToken());
		if( from < 0 || from >= Network.size() )
		{
			wasOutOfRange = true;
			continue;
		}
		
		for(int i=0; i<k && st.hasMoreTokens(); ++i)
		{
			final int to = Integer.parseInt(st.nextToken());
			if( to < 0 || to >= Network.size() )
				wasOutOfRange = true;
			else
				g.setEdge(from,to);
		}
	}

	if( wasOutOfRange )
		System.err.println("WireFromFile warning: in "+file+" "+
			"some nodes were out of range and so ignored.");
	lnr.close();
}
catch( IOException e )
{
	throw new RuntimeException(e);
}
}

}
