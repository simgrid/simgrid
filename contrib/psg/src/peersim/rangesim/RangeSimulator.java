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

import peersim.*;
import peersim.config.*;
import peersim.core.*;
import peersim.util.*;

/**
 * This class is the main class for the Range Simulator. A range is
 * a collection of values <em>S</em> to be assigned to a variable 
 * <em>v</em>. The Range Simulator invokes the standard Peersim 
 * simulator once for each distinct value. If multiple ranges 
 * <em>S1, S2, ..., Sn</em> are specified, the standard Peersim
 * simulator is invoked for each element in 
 * <em>S1 * S2 * ... * Sn</em>.  
 * <p>  
 * Ranges are specified with the following syntax:
<pre>
range.[id] [var];[range]
</pre>
 * where:
 * <UL>
 * <LI> {@value #PAR_RANGE} is the prefix for all range
 * specifications;</LI>
 * <LI> <code>id</code> is an identifier; since they are not referred anywhere else,
 * consecutive numbers are a good choice for range identifiers;</LI>
 * <LI> <code>var</code> is a variable parameter </LI>
 * <LI> <code>range</code> describes the collection of values to be associated
 * to <code>var</code>, whose syntax and semantics is defined in 
 * {@link peersim.util.StringListParser}. </LI>
 * </UL>
 * Examples of range specifications are the following:
<pre>
range.0 SIZE;2^10:2^18|*2
range.1 K;20:30
range.2 CHURN;0.05,0.10,0.20 
</pre>
 * With this specification, the collection of values associated to 
 * <code>SIZE</code> is {2^10,2^11,...,2^18}; <code>K</code> contains 
 * {20,21,22,...,30}, while <code>CHURN</code> contains just the
 * specified values.
 * <p>
 * Values can be specified as constant expressions (like 2^10, (5+10), etc.)
 * but variables cannot be used. 
 * <p>
 * A separate Java virtual machine is invoked to run each of the
 * experiments. An attempt is done to run the same JVM version as
 * the one running the Range Simulator; if this is not possible
 * (for example due to path problems), the command shell mechanism
 * is used to run the first JVM version found in the path.
 * </p>
 * It is possible to specify options for the forked JVM using the
 * {@value #PAR_JVM} parameter on the command line. For example,
 * a command line like this:
<pre>
java peersim.rangesim.RangeSimulator config.file jvm.options=-Xmx256m
</pre>
 * can be used to run the forked JVM with a maximum heap of 256MB.
 * <p>
 * The new JVM inherits the same classpath as the JVM running the
 * RangeSimulator. The {@value #PAR_JVM} parameter can be used to
 * specify additional classpath specification.
 * 
 * @author Alberto Montresor
 * @version $Revision: 1.11 $
 */
public class RangeSimulator implements ProcessHandler
{

// --------------------------------------------------------------------------
// Configuration parameters
// --------------------------------------------------------------------------

/**
 * This is the prefix of the config properties whose value vary during
 * a set of experiments.
 * @config
 */
private static final String PAR_RANGE = "range";

/**
 * This config property can be used to set options in the JVMs that
 * are forked to execute experiments with different configuration
 * parameters.
 * @config
 */
public static final String PAR_JVM = "jvm.options";


// --------------------------------------------------------------------------
// Static variables
// --------------------------------------------------------------------------

/** Names of range parameters */
private String[] pars;

/** Values to be simulated, for each parameter */
private String[][] values;

/** The jvm options to be used when creating jvms */
private String[] jvmoptions;

/** Command line arguments */
private String[] args;

/** The current process that is executed */
private Process p;


// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------

/**
 * Main method of the system.
 */
public static void main(String[] args)
{
	RangeSimulator r = new RangeSimulator(args);
	r.run();
}

//--------------------------------------------------------------------------
// Constructor
//--------------------------------------------------------------------------

public RangeSimulator(String[] args)
{

	// Check if there are no arguments or there is an explicit --help
	// flag; if so, print the usage of the class
	if (args.length == 0 || args[0].equals("--help")) {
		usage();
		System.exit(101);
	}

	this.args = args.clone();

	// Read property file
	System.err.println("Simulator: loading configuration");
	Properties properties = new ParsedProperties(args);
	Configuration.setConfig(properties);
	
	// Read jvm options and separate them in different strings
	String opt = Configuration.getString(PAR_JVM, null);
	if (opt == null)
		jvmoptions = new String[0];
	else
		jvmoptions = opt.split(" ");

	// Parse range parameters
	parseRanges();

}

/**
 * Main method to be executed
 */
public void run()
{
	// Shutdown thread management
	ProcessManager t = new ProcessManager();
	t.addThread(this);
	Runtime.getRuntime().addShutdownHook(t);

	// Executes experiments; report short messages about exceptions that are
	// handled by the configuration mechanism.
	try {
		doExperiments(args);
	} catch (MissingParameterException e) {
		Runtime.getRuntime().removeShutdownHook(t);
		System.err.println(e + "");
		System.exit(101);
	} catch (IllegalParameterException e) {
		Runtime.getRuntime().removeShutdownHook(t);
		System.err.println(e + "");
		System.exit(101);
	}
	Runtime.getRuntime().removeShutdownHook(t);
	System.exit(0);
}

// --------------------------------------------------------------------

/**
 * Parses a collection of range specifications and returns the set of
 * parameter that will change during the simulation and the values that
 * will be used for those parameters.
 */
private void parseRanges()
{
	// Get ranges
	String[] ranges = Configuration.getNames(PAR_RANGE);

	// Start is the first element in which ranges are stored
	int start;
	
	// If there is an explicit simulation.experiment or there are no 
	// ranges, put an experiment range at the beginning of the values.
	// Otherwise, just use the ranges.
	if (Configuration.contains(Simulator.PAR_EXPS) || ranges.length == 0) {
		pars = new String[ranges.length + 1];
		values = new String[ranges.length + 1][];
		pars[0] = "EXP";
		values[0] = StringListParser.parseList("1:"
				+ Configuration.getInt(Simulator.PAR_EXPS, 1));
		start = 1;
	} else {
		pars = new String[ranges.length];
		values = new String[ranges.length][];
		start = 0;
	}

	for (int i = start; i < pars.length; i++) {
		String[] array = Configuration.getString(ranges[i-start]).split(";");
		if (array.length != 2) {
			throw new IllegalParameterException(ranges[i],
					" should be formatted as <parameter>;<value list>");
		}
		pars[i] = array[0];
		values[i] = StringListParser.parseList(array[1]);
	}
}

// --------------------------------------------------------------------

/**
 * Selects the next set of values by incrementing the specified index
 * array. The index array is treated as a vector of digits; the first is
 * managed managed as a vector of digits.
 */
private void nextValues(int[] idx, String[][] values)
{
	idx[idx.length - 1]++;
	for (int j = idx.length - 1; j > 0; j--) {
		if (idx[j] == values[j].length) {
			idx[j] = 0;
			idx[j - 1]++;
		}
	}
}

// --------------------------------------------------------------------

private void doExperiments(String[] args)
{

	// Configure the java parameter for exception
	String filesep = System.getProperty("file.separator");
	String classpath = System.getProperty("java.class.path");
	String javapath = System.getProperty("java.home") + filesep + "bin" + filesep
			+ "java";
	ArrayList<String> list = new ArrayList<String>(20);
	list.add(javapath);
	list.add("-cp");
	list.add(classpath);
	
	// Add the jvm options
	for (int i=0; i < jvmoptions.length; i++)
		list.add(jvmoptions[i]);
	
	// The class to be run in the forked JVM
	list.add("peersim.Simulator");
	
	// Parameters specified on the command line
	for (int i=0; i < args.length; i++) {
		list.add(args[i]);
	}
	
	// Since multiple experiments are managed here, the value
	// of standard variable for multiple experiments is changed to 1
	list.add(Simulator.PAR_EXPS+"=1");

	// Activate redirection to separate stdout from stderr
	list.add(Simulator.PAR_REDIRECT+"="+TaggedOutputStream.class.getCanonicalName());
	int startlog = list.size();
	list.add(""); 
	
	// Create a placeholder for the seed
	int startseed = list.size();
	list.add("");
	
	// Create placeholders for the range parameters
	int startpar = list.size();
	for (int i=0; i < values.length; i++)
		list.add("");
		
	// Execute with different values
	int[] idx = new int[values.length]; // Initialized to 0
	while (idx[0] < values[0].length) {

		// Configure the argument string array
		for (int j = 0; j < pars.length; j++) {
			list.set(startpar + j, pars[j] + "=" + values[j][idx[j]]);
		}

		// Fill the log placeholder
		StringBuffer log = new StringBuffer();
		for (int j = 0; j < pars.length; j++) {
			log.append(pars[j]);
			log.append(" ");
			log.append(values[j][idx[j]]);
			log.append(" ");
		}
		list.set(startlog, Simulator.PAR_REDIRECT+"."+
				TaggedOutputStream.PAR_RANGES+"="+log);

		// Fill the seed place holder
		long seed = CommonState.r.nextLong();
		list.set(startseed, CommonState.PAR_SEED+"="+seed);

		System.err.println("Experiment: " + log);
		
		executeProcess(list);

		// Increment values
		nextValues(idx, values);
	
	}
}

//--------------------------------------------------------------------

/**
 * Execute the "command line" represented by this String list.
 * The first argument is the process to be executed. We try
 * to run the same JVM as the current one. If not possible,
 * we use the first java command found in the path.
 */
private void executeProcess(List<String> list)
{
	// Prepare the argument array for process forking
	String[] newargs = new String[list.size()];

	// Execute a new JVM
	try {
		ProcessBuilder pb = new ProcessBuilder(list.toArray(newargs));
		pb.redirectErrorStream(true);
		p = pb.start();
	} catch (IOException e1) {
		try {
			list.set(0, "java");
			ProcessBuilder pb = new ProcessBuilder(list.toArray(newargs));
			pb.redirectErrorStream(true);
			p = pb.start();
		} catch (IOException e2) {
			System.err.println("Unable to launch a Java virtual machine");
			System.exit(1);
		}
	}

	// Read the output from the process and redirect it to System.out
	// and System.err.
	BufferedReader toprint = new BufferedReader(new InputStreamReader(p
			.getInputStream()));
	String line;
	while ((line = getLine(toprint)) != null) {
		if (line.length() == 0) {
			System.out.println();
		} else {
			int last = line.charAt(line.length()-1);
			if (last != TaggedOutputStream.TAG) {
				System.err.println(line);
			} else {
				line = line.substring(0, line.length()-1);
				System.out.println(line);
			}
		}
	}

	// We close all the files and we destroy the process. They are not 
	// cleaned when the process is closed. See:
	// http://bugs.sun.com/bugdatabase/view_bug.do?bug_id=4784692
	// http://www.thescripts.com/forum/thread18019.html
	try {
		p.getErrorStream().close();
		p.getInputStream().close();
		p.getOutputStream().close();
		p.destroy();
	} catch (IOException e) {
		e.printStackTrace();
	}

	// The static variable p (used also by ShutdownThread) is back to
	// null - no process must be killed on shutdown.
	p = null;
	
	

}

//--------------------------------------------------------------------

private static String getLine(BufferedReader toprint)
{
	try {
		return toprint.readLine();
	} catch (IOException e) {
		// If we get here, this means that the forked process has
		// been killed by the shutdown thread. We just exit without
	  // printing this exception.
		System.exit(1);
		return null; // Never reached, but needed.
	}
}


// --------------------------------------------------------------------

private static void usage()
{
	System.err.println("Usage:");
	System.err.println("  peersim.RangeSimulator <configfile> [property]*");
}

//--------------------------------------------------------------------

RangeSimulator()
{
}

/**
 * Stop the process executing the external java virtual machine.
 */
public void doStop()
{
	if (p != null)
		p.destroy();
}

/**
 * Wait until the java virtual machine has terminated; it won't be
 * used in this class, but you never know.
 */
public void join() throws InterruptedException
{
	p.waitFor();
}

}
