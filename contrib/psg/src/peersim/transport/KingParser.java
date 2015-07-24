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
import java.util.*;
import peersim.config.*;
import peersim.core.Control;

/**
 * Initializes static singleton {@link E2ENetwork} by reading a king data set.
 * 
 * @author Alberto Montresor
 * @version $Revision: 1.9 $
 */
public class KingParser implements Control
{

// ---------------------------------------------------------------------
// Parameters
// ---------------------------------------------------------------------

/**
 * The file containing the King measurements.
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

/** Name of the file containing the King measurements. */
private String filename;

/**
 * Ratio between the time units used in the configuration file and the time
 * units used in the Peersim simulator.
 */
private double ratio;

/** Prefix for reading parameters */
private String prefix;

// ---------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------

/**
 * Read the configuration parameters.
 */
public KingParser(String prefix)
{
	this.prefix = prefix;
	ratio = Configuration.getDouble(prefix + "." + PAR_RATIO, 1);
	filename = Configuration.getString(prefix + "." + PAR_FILE, null);
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
	BufferedReader in = null;
	if (filename != null) {
		try {
			in = new BufferedReader(new FileReader(filename));
		} catch (FileNotFoundException e) {
			throw new IllegalParameterException(prefix + "." + PAR_FILE, filename
					+ " does not exist");
		}
	} else {
		in = new BufferedReader( new InputStreamReader(
						ClassLoader.getSystemResourceAsStream("t-king.map")
					)	);
	}
		
	// XXX If the file format is not correct, we will get quite obscure
	// exceptions. To be improved.

	String line = null;
	// Skip initial lines
	int size = 0;
	int lc = 1;
	try {
		while ((line = in.readLine()) != null && !line.startsWith("node")) lc++;
		while (line != null && line.startsWith("node")) {
			size++;
			lc++;
			line = in.readLine();
		}
	} catch (IOException e) {
		System.err.println("KingParser: " + filename + ", line " + lc + ":");
		e.printStackTrace();
		try { in.close(); } catch (IOException e1) { };
		System.exit(1);
	}
	E2ENetwork.reset(size, true);
	if (line == null) {
		System.err.println("KingParser: " + filename + ", line " + lc + ":");
		System.err.println("No latency matrix contained in the specified file");
		try { in.close(); } catch (IOException e1) { };
		System.exit(1);
	}
	
	System.err.println("KingParser: read " + size + " entries");
	
	try {
		do {
			StringTokenizer tok = new StringTokenizer(line, ", ");
			if (tok.countTokens() != 3) {
				System.err.println("KingParser: " + filename + ", line " + lc + ":");
				System.err.println("Specified line does not contain a <node1, node2, latency> triple");
				try { in.close(); } catch (IOException e1) { };
				System.exit(1);
			}
			int n1 = Integer.parseInt(tok.nextToken()) - 1;
			int n2 = Integer.parseInt(tok.nextToken()) - 1;
			int latency = (int) (Double.parseDouble(tok.nextToken()) * ratio);
			E2ENetwork.setLatency(n1, n2, latency);
			lc++;
			line = in.readLine();
		} while (line != null);
		
		in.close();
	
	} catch (IOException e) {
		System.err.println("KingParser: " + filename + ", line " + lc + ":");
		e.printStackTrace();
		try { in.close(); } catch (IOException e1) { };
		System.exit(1);
	}


	return false;
}

}
