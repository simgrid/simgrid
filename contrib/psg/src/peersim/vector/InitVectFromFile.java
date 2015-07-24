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

package peersim.vector;

import java.io.*;
import java.util.*;
import peersim.config.*;
import peersim.core.*;

/**
 * Initializes a protocol vector from data read from a file.
 * The file format is as follows:
 * lines starting with # or lines that contain only
 * whitespace are ignored.
 * From the rest of the lines the first field separated by whitespace is
 * read. Only the first field is read from each line, the rest of the line
 * is ignored.
 * The file can contain more values than necessary but
 * enough values must be present.
 * @see VectControl
 * @see peersim.vector
 */
public class InitVectFromFile extends VectControl
{

// --------------------------------------------------------------------------
// Parameter names
// --------------------------------------------------------------------------

/**
 * The filename to load links from.
 * @config
 */
private static final String PAR_FILE = "file";

// --------------------------------------------------------------------------
// Fields
// --------------------------------------------------------------------------

/** The file to be read */
private final String file;

// --------------------------------------------------------------------------
// Initialization
// --------------------------------------------------------------------------

/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param prefix the configuration prefix for this class
 */
public InitVectFromFile(String prefix)
{
	super(prefix);
	file = Configuration.getString(prefix + "." + PAR_FILE);
}

// --------------------------------------------------------------------------
// Methods
// --------------------------------------------------------------------------

/**
 * Initializes values from a file.
 * The file format is as follows:
 * lines starting with # or lines that contain only
 * whitespace are ignored.
 * From the rest of the lines the first field separated by whitespace is
 * read. Only the first field is read from each line, the rest of the line
 * is ignored.
 * The file can contain more values than necessary but
 * enough values must be present.
 * @throws RuntimeException if the file cannot be read or contains too few
 * values
 * @return always false
 */
public boolean execute() {

	int i = 0;

try {
	FileReader fr = new FileReader(file);
	LineNumberReader lnr = new LineNumberReader(fr);
	String line;
	while ((line = lnr.readLine()) != null && i < Network.size()) {
		if (line.startsWith("#"))
			continue;
		StringTokenizer st = new StringTokenizer(line);
		if (!st.hasMoreTokens())
			continue;
		if( setter.isInteger() )
			setter.set(i,Long.parseLong(st.nextToken()));
		else	setter.set(i,Double.parseDouble(st.nextToken()));
		i++;
	}
	lnr.close();
}
catch(IOException e)
{
	throw new RuntimeException("Unable to read file: " + e);
}
	
	if (i < Network.size())
		throw new RuntimeException(
		"Too few values in file '" + file + "' (only "
		+ i + "); we need " + Network.size() + ".");
	
	return false;
}

// --------------------------------------------------------------------------

}
