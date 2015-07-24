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
		
package peersim.config;

import java.io.*;
import java.util.*;

/**
* Extends the class {@link ConfigProperties} with basic parsing capabilities.
* @see #load
*/
public class ParsedProperties extends ConfigProperties {

//================= variables ======================================
//==================================================================

// ================= initialization =================================
// ==================================================================

/**
* Calls super constructor.
* @see ConfigProperties#ConfigProperties(String[])
*/
public	ParsedProperties( String[] pars ) {

	super( pars );
}

// ------------------------------------------------------------------

/**
* Calls super constructor.
* @see ConfigProperties#ConfigProperties(String)
*/
public	ParsedProperties( String filename ) throws IOException {

	super( filename );
}


// =========== Public methods ========================================
// ===================================================================


/**
* Loads given file. It works exactly as <code>Properties.load</code>
* with a file input stream to the given file, except that the file is parsed
* the following way allowing to compress some property names
* using <code>{</code> and <code>}</code>.
  
  When a bracket is present, it must
  be the only non-space element of a line. The last property defined 
  before the opening bracket define the prefix that is added to all the 
  properties defined included between brackets.
  In other words, a construct like this:
  <pre>
  control.degree GraphObserver 
  {
    protocol newscast
    undir
  }
  </pre>
  is equivalent to the definition of these three properties:
  <pre>
  control.degree GraphObserver 
  control.degree.protocol newscast
  control.degree.undir
  </pre>
  
  Nested brackets are possible. The rule of the last property before 
  the opening bracket applies also to the inside brackets, with
  the prefix being the complete property definition (so, including
  the prefix observed before). Example:
  <pre>
	control.1 DynamicNetwork
	{
	  add CRASH
	  substitute
	  init.0 WireKOut 
	  {
	    degree DEGREE
	    protocol 0
	  }
	}
  </pre>
  defines the following properties:
  <pre>
	control.1 DynamicNetwork
	control.1.add CRASH
	control.1.substitute
	control.1.init.0 WireKOut 
	control.1.init.0.degree DEGREE
	control.1.init.0.protocol 0
  </pre>
  
  <p>
  Know limitations: 
  The parsing mechanism is very simple; no complex error messages
  are provided. In case of missing closing brackets, the method
  will stop reporting the number of missing brackets. Additional
  closing brackets (i.e., missing opening brackets) produce an
  error messages reporting the line where the closing bracket
  is present. Misplaced brackets (included in lines that
  contains other characters) are ignored, thus may indirectly produce
  the previous error messages.
*/
public void load( String fileName ) throws IOException {

	/* This set is used to store prefixes that have been associated
	 * to brackets blocks. If a prefix is inserted twice, this means
	 * that there are two blocks referring to the same prefix - 
	 * which may be caused by a commented prefix in the config
	 * file, something like this:
	 * 
	 *  prefix1
	 *  {
	 *    property 1
	 *  }
	 *  #prefix2
	 *  {
	 *    property 2
	 *  }
	 *
	 */
	Set<String> prefixes = new HashSet<String>();
	
	BufferedReader f = 
		new BufferedReader(new FileReader( fileName ));
	int lines = 0;
	parseStream(f, "", 0, lines, prefixes);

	f.close();
}

// --------------------------------------------------------------------

private int parseStream(BufferedReader f, String prefix, int pars, 
		int lines, Set prefixes)
throws IOException {

	if (prefix.equals(".")) {
		System.err.println("Error at line " + lines + ": bracket block not " +
				"associated with any configuration entry");
		System.exit(1);
	}
	if (prefixes.contains(prefix)) {
		System.err.println("Error at line " + lines + ": multiple bracket " +
				"blocks referring to the same configuration entry " + prefix);
		System.exit(1);
	} else {
		prefixes.add(prefix);
	}
	
	boolean complete = true;
	String part;
	String line = "";
	String last = "";
	while ((part = f.readLine()) != null)
	{
		lines++;
		
		// Reset line
		if (complete) line = "";
		
		// Check if the line is a comment line
		// If so, remove the comment
		int index = part.indexOf('#');
		if (index >= 0)
		{
			part = part.substring(0, index);
		} 

		// Check if the line is empty
		part = part.trim();
		if ("".equals(part)) continue;

		complete = (part.charAt(part.length()-1) != '\\'); 
		if (!complete)
		{  
			line = line + part.substring(0, part.length()-2) + " ";
			continue;
		}
		
		// Complete the line
		line = line + part;
		if (line.equals("{"))
		{
			lines = parseStream(f, last+".", pars+1, lines, prefixes);
		} 
		else if (line.equals("}"))
		{
			if (pars == 0)
			{
				System.err.println(
					"Error: Additional } at line " + lines + 
					" when parsing the configuration file");
				System.exit(1);
			}
			return lines;
		}
		else
		{
			// Search the first token
			String[] tokens = line.split("[\\s:=]+", 2);
			if (tokens.length == 1)
			{
				setProperty(prefix+tokens[0], "");
			}
			else
			{
				setProperty(prefix+tokens[0], tokens[1]);
			}
			last = prefix + tokens[0];
		}
	}
	if (pars == 1)
	{
		System.err.println("Error: One closing bracket ('}') is missing");
		System.exit(1);
	} 
	else if (pars > 1)
	{
		System.err.println("Error: " + pars+" closing brackets ('}') are missing");
		System.exit(1);
	}
	return lines;
}

// --------------------------------------------------------------------

/*
public static void main(String[] args)
{
	java.util.Properties prop = new ParsedProperties(args);
}
*/
}

