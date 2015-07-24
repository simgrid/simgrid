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

package peersim.transport;

import java.io.*;

import peersim.config.*;
import peersim.core.*;

/**
 * Initializes static singleton {@link E2ENetwork} by reading a trace 
 * file containing the latency distance measured between a set of 
 * "virtual" routers. Latency is assumed to be symmetric, so the 
 * latency between x and y is equal to the latency to y and x.
 * 
 * The format of the file is as follows: all values are stored as
 * integers. The first value is the number of nodes considered.
 * The rest of the values correspond to a "strictly upper triangular 
 * matrix" (see this 
 * <a href="http://en.wikipedia.org/w/index.php?title=Triangular_matrix&oldid=82411128">
 * link</a>), ordered first by row than by column.
 * 
 * @author Alberto Montresor
 * @version $Revision: 1.4 $
 */
public class TriangularMatrixParser implements Control
{

// ---------------------------------------------------------------------
// Parameters
// ---------------------------------------------------------------------

/**
 * This configuration parameter identifies the filename of the file
 * containing the measurements. First, the file is used as a pathname 
 * in the local file system. If no file can be identified in this way, 
 * the file is searched in the local classpath. If the file cannot be 
 * identified again, an error message is reported.
 * @config
 */
private static final String PAR_FILE = "file";

/**
 * The ratio between the time units used in the configuration file and the
 * time units used in the Peersim simulator.
 * @config
 */
private static final String PAR_RATIO = "ratio";

// ---------------------------------------------------------------------
// Fields
// ---------------------------------------------------------------------

/** Name of the file containing the measurements. */
private String filename;

/** Ratio read from PAR_RATIO */
private double ratio;

// ---------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------

/**
 * Read the configuration parameters.
 */
public TriangularMatrixParser(String prefix)
{
	filename = Configuration.getString(prefix + "." + PAR_FILE);
	ratio = Configuration.getDouble(prefix + "." + PAR_RATIO);
}

// ---------------------------------------------------------------------
// Methods
// ---------------------------------------------------------------------

/**
 * Initializes static singleton {@link E2ENetwork} by reading a king data set.
* @return  always false
*/
public boolean execute()
{
	try {
		ObjectInputStream in = null;
		try {
			in = new ObjectInputStream(
					new BufferedInputStream(
							new FileInputStream(filename)));
			System.err.println("TriangularMatrixParser: Reading " + filename + " from local file system");
		} catch (FileNotFoundException e) {
			in = new ObjectInputStream(
					new BufferedInputStream(
							ClassLoader.getSystemResourceAsStream(filename)));
			System.err.println("TriangularMatrixParser: Reading " + filename + " through the class loader");
		}
	
		// Read the number of nodes in the file (first four bytes).
	  int size = in.readInt();
	  
		// Reset the E2E network
		E2ENetwork.reset(size, true);
		System.err.println("TriangularMatrixParser: reading " + size + " rows");
	
		// If the file format is not correct, data will be read 
		// incorrectly. Probably a good way to spot this is the 
		// presence of negative delays, or an end of file.
	
		// Reading data
		int count = 0;
		for (int r=0; r < size; r++) {
			for (int c = r+1; c < size; c++) {
				int x = (int) (ratio*in.readInt());
				count++;
				E2ENetwork.setLatency(r,c,x);
			}
		}
		System.err.println("TriangularMatrixParser: Read " + count + " entries");
	} catch (IOException e) {
		throw new RuntimeException(e.getMessage());
	}
	return false;
}

}
