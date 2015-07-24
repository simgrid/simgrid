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

import peersim.config.*;
import peersim.core.*;
import peersim.util.*;

/**
 * Dump the content of a vector in a file. Each line
 * represent a single node.
 * Values are dumped to a file whose name is obtained from a
 * configurable prefix (set by {@value #PAR_BASENAME}), a number that is
 * increased before each dump by one, and the extension ".vec".
 * <p>
 * This observer class can observe any protocol field containing a 
 * primitive value, provided that the field is associated with a getter method 
 * that reads it.
 * @see VectControl
 * @see peersim.vector
 */
public class ValueDumper extends VectControl {


// --------------------------------------------------------------------------
// Parameter names
// --------------------------------------------------------------------------

/**
 * This is the base name of the file where the values are saved. The full name
 * will be baseName+cycleid+".vec".
 * @config
 */
private static final String PAR_BASENAME = "outf";

// --------------------------------------------------------------------------
// Fields
// --------------------------------------------------------------------------

/** Prefix name of this observer */
private final String prefix;

/** Base name of the file to be written */
private final String baseName;

private final FileNameGenerator fng;

// --------------------------------------------------------------------------
// Constructor
// --------------------------------------------------------------------------

/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param prefix the configuration prefix for this class
 */
public ValueDumper(String prefix) {

	super(prefix);
	this.prefix = prefix;
	baseName = Configuration.getString(prefix + "." + PAR_BASENAME, null);
	if(baseName!=null) fng = new FileNameGenerator(baseName,".vec");
	else fng = null;
}

// --------------------------------------------------------------------------
// Methods
// --------------------------------------------------------------------------

/**
 * Dump the content of a vector in a file. Each line
 * represent a single node.
 * Values are dumped to a file whose name is obtained from a
 * configurable prefix (set by {@value #PAR_BASENAME}), a number that is
 * increased before each dump by one, and the extension ".vec".
 * @return always false
 * @throws RuntimeException if there is an I/O problem
 */
public boolean execute() {
try
{
	System.out.print(prefix + ": ");
	
	// initialize output streams
	if (baseName != null)
	{
		String filename = fng.nextCounterName();
		System.out.println("writing "+filename);
		PrintStream pstr =
			new PrintStream(new FileOutputStream(filename));
		for (int i = 0; i < Network.size(); ++i)
		{
			pstr.println(getter.get(i));
		}
		pstr.close();
	}
	else
	{
		System.out.println();
		for (int i = 0; i < Network.size(); ++i)
		{
			System.out.println(getter.get(i));
		}
	}
}
catch (IOException e)
{
	throw new RuntimeException(prefix + ": Unable to write to file: " + e);
}

	return false;
}

// ---------------------------------------------------------------------

}
