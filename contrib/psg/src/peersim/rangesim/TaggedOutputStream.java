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

package peersim.rangesim;

import java.io.*;
import java.util.*;

import peersim.config.*;
import peersim.core.*;

/**
 * This OutputStream uses an underlying stream to output
 * data. Each line (terminated with `\n`) is augmented
 * with a tag character. This is used to discriminate
 * among standard error and standard output. This 
 * feature is needed for launching new JVMs; it should
 * not be used for other purposes. 
 * 
 * @author Alberto Montresor
 * @version $Revision: 1.5 $
 */
public class TaggedOutputStream extends PrintStream
{

//--------------------------------------------------------------------------
//Constants
//--------------------------------------------------------------------------

/** 
 * This character is appended at the end of each line. 
 */
public static final int TAG = 1;

//--------------------------------------------------------------------------
//Parameters
//--------------------------------------------------------------------------

/**
 * This parameter contains the string that should be printed on each 
 * line, containing the values of the range parameters for the experiment
 * which is being run. The full name of this configuration string is
 * prefixed by {@value peersim.Simulator#PAR_REDIRECT}.
 * @config
 */
public static final String PAR_RANGES = "ranges";

/**
 * This parameter contains the list of observers for which the string
 * contained in parameter {@value #PAR_RANGES} should be augmented with 
 * a "TIME &lt;t&gt;" specification regarding current time. Observers are 
 * separated by one of this characters: ' ' - ',' - ';'.
 * @config
 */
public static final String PAR_TIME = "simulation.timed-observers";


//--------------------------------------------------------------------------
//Fields
//--------------------------------------------------------------------------

/** Variable used to save the original System.out to simplify printing */
private PrintStream stdout;

/** Buffer used to store a single line; it can grow */
private byte[] buffer = new byte[1024];

/** Current size of the buffer */
private int size;

/** The value of the PAR_RANGES parameter */
private final String ranges;

/** The value of the PAR_TIME parameter */
private final ArrayList<String> obstime;

//--------------------------------------------------------------------------
//Initialization
//--------------------------------------------------------------------------


/**
 * Creates a tagged output stream that prints the tagged
 * output on the specified stream.
 */
public TaggedOutputStream(String prefix)
{
	super(System.out);
	
	obstime = new ArrayList<String>();
	String[] obs = Configuration.getString(PAR_TIME, "").split("[ :,]");
	for (int i=0; i < obs.length; i++) {
		obstime.add("control." + obs[i]);
	}
	ranges = Configuration.getString(prefix + "." + PAR_RANGES, "");
	stdout = System.out;
	size = 0;
}

//--------------------------------------------------------------------------
//Methods
//--------------------------------------------------------------------------

// Comment inherited from interface
@Override
public synchronized void write(byte[] b, int off, int len)
{
	if (size+len > buffer.length) {
		byte[] tmp = new byte[Math.max(buffer.length*2, size+len)];
		System.arraycopy(buffer, 0, tmp, 0, size);
		buffer = tmp;
	}
	int last = off+len;
	for (int i=off; i < last; i++) {
		if (b[i] == '\n') {
			buffer[size++] = TAG;
			buffer[size++] = b[i];
			printLine();
		}  else {
			buffer[size++] = b[i];
		}
	}
}

// Comment inherited from interface
@Override
public synchronized void write(int b)
{
	if (size == buffer.length) {
		byte[] tmp = new byte[buffer.length*2];
		System.arraycopy(buffer, 0, tmp, 0, size);
		buffer = tmp;
	}
	if (b == '\n') {
		buffer[size++] = TAG;
		buffer[size++] = (byte) b;
		printLine();
	}  else {
		buffer[size++] = (byte) b;
	}
}

/** 
 * Actually prints a line, inserting ranges and time
 * when needed.
 */
private void printLine()
{
	String line = new String(buffer, 0, size);
	String[] parts = line.split(":");
	if (parts.length == 2) {
		stdout.print(parts[0]);
		stdout.print(": ");
		stdout.print(ranges);
		if (obstime.contains(parts[0]))
			stdout.print(" TIME " + CommonState.getTime() + " ");
		stdout.print(parts[1]);
	} else {
		stdout.print(line);
	}
	size = 0;
}

}
