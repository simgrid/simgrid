package psgsim;

import java.io.*;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.TreeMap;

import org.jdom2.*;
import org.jdom2.output.*;
import org.simgrid.msg.Host;
import org.simgrid.msg.Msg;

import peersim.config.Configuration;
import peersim.core.Control;
import peersim.core.Protocol;

/**
 * A class store different configuration information for simulation. It creates
 * the deployment file according to this informations.
 * 
 * @author Khaled Baati 26/10/2014
 * @version version 1.1
 */

public class PSGPlatform {

	enum timeUnit {
		us, ms, sec;
	}

	/** unit of measure. **/
	static int unit;

	/** the clock. **/
	static double time;

	/** the default unit of measure **/
	static final String sec = "sec";

	/** All protocols defined in the configuration file. **/
	static Protocol[] protocolsName;

	/** A numeric identifier associated for each protocol. **/
	static int[] pid;

	/** List of hos.t **/
	static Host[] hostList;

	/** A collection map represents the Control and its associated step. **/
	static Map<Control, Double> controlStepMap = new LinkedHashMap<Control, Double>();

	/** A collection map represents the protocol and its associated pid. **/
	static TreeMap<Protocol, Integer> protocolsPidsMap = new TreeMap<Protocol, Integer>(
			new Comparator<Protocol>() {
				public int compare(Protocol p1, Protocol p2) {
					return p1.toString().compareTo(p2.toString());
				}
			});

	/** A collection map represents all CDProtocol and its associated step **/
	static TreeMap<Protocol, Double> cdProtocolsStepMap = new TreeMap<Protocol, Double>(
			new Comparator<Protocol>() {
				public int compare(Protocol p1, Protocol p2) {
					return p1.toString().compareTo(p2.toString());
				}
			});
	/** the default platform file **/
	static final String platformFile = "platforms/psg.xml";

	/** the deployment file **/
	static final String deploymentFile = "deployment.xml";

	static Element racine;
	static Document document;
	static boolean interfED = false;
	static boolean interfCD = false;

	/** Prepare the deployment file **/
	static {
		DocType dtype = new DocType("platform",
				"http://simgrid.gforge.inria.fr/simgrid.dtd");
		racine = new Element("platform");
		document = new Document(racine, dtype);
		Attribute version = new Attribute("version", "3");
		racine.setAttribute(version);
	}

	// ========================== methods ==================================
	// =====================================================================

	/**
	 * Convert PS unit time to Simgrid unit time
	 * 
	 * @param valeur
	 *            the value to convert
	 * @return time converted
	 */
	public static double psToSgTime(long valeur) {
		timeUnit unit = unit();
		switch (unit) {
		case us:
			return ((double) valeur) / 1000000;
		case ms:
			return ((double) valeur) / 1000;
		default:
			return (double) valeur;

		}
	}

	/**
	 * Convert Simgrid unit time to PS unit time
	 * 
	 * @param valeur
	 *            the value to convert
	 * @return time converted
	 */
	public static long sgToPsTime(double valeur) {
		timeUnit unit = unit();
		switch (unit) {
		case us:
			return (long) valeur * 1000000;
		case ms:
			return (long) valeur * 1000;
		default:
			return (long) valeur;

		}
	}

	/**
	 * 
	 * @return the duration of simulation.
	 */
	public static long getDuration() {
		return Configuration.getLong("simulation.duration");
	}

	/**
	 * 
	 * @return PeerSim Time
	 */
	public static long getTime() {
		return sgToPsTime(Msg.getClock());
	}

	/**
	 * 
	 * @return the Simgrid Clock
	 */
	public static double getClock() {
		return Msg.getClock();
	}

	/**
	 * Load and run initializers.
	 */
	public static void init() {
		Object[] inits = Configuration.getInstanceArray("init");
		String names[] = Configuration.getNames("init");
		for (int i = 0; i < inits.length; ++i) {
			System.err.println("- Running initializer " + names[i] + ": "
					+ inits[i].getClass().toString());
			((Control) inits[i]).execute();
		}
	}

	/**
	 * Load all controls and stores them in {@link #controlStepMap} collection
	 * to be scheduled, and executed in {@link psgsim.PSGProcessController}.
	 */
	public static void control() {
		// load controls
		String[] names = Configuration.getNames("control");
		Control control;
		for (int i = 0; i < names.length; ++i) {
			control = (Control) Configuration.getInstance(names[i]);
			Long stepControl = Configuration.getLong(names[i] + "." + "step");
			controlStepMap.put(control, psToSgTime(stepControl));
		}
	}

	/**
	 * Lookup all protocols in the configuration file
	 */
	public static void protocols() {
		String[] names = Configuration.getNames("protocol");
		Class[] interfaces;
		protocolsName = new Protocol[names.length];
		pid = new int[names.length];
		boolean save = false;
		for (int i = 0; i < names.length; i++) {
			protocolsName[i] = (Protocol) Configuration.getInstance(names[i]);
			if (i == names.length - 1)
				save = true;
			userProtocol(protocolsName[i], names[i], save);
			pid[i] = i;
			protocolsPidsMap.put(protocolsName[i], pid[i]);
		}

	}

	/**
	 * Lookup CDProtocol and EDProtocol among all protocols
	 * 
	 * @param prot
	 *            the protocol class
	 * @param names
	 *            the protocol name
	 * @param save
	 *            parameter equal true when parsing all protocols
	 */
	public static void userProtocol(Protocol prot, String names, boolean save) {
		Class[] interfaces = prot.getClass().getInterfaces();
		for (int j = 0; j < interfaces.length; j++) {
			if (interfaces[j].getName().endsWith("EDProtocol")) {
				interfED = true;
			}
			if (interfaces[j].getName().endsWith("CDProtocol")) {
				String protName = names.substring("protocol".length() + 1);
				long step = Configuration.getLong("protocol" + "." + protName
						+ "." + "step");
				cdProtocolsStepMap.put(prot, psToSgTime(step));
			}
		}
		if (save) {
			edProt();
		}
	}

	/**
	 * 
	 */
	private static void edProt() {
		Host hostVal;
		hostList = Host.all();
		for (int i = 0; i < PSGSimulator.size; i++) {
			hostVal = hostList[i];
			Element process = new Element("process");
			racine.addContent(process);
			Attribute host = new Attribute("host", hostVal.getName());
			Attribute function = new Attribute("function",
					"psgsim.PSGProcessEvent");
			process.setAttribute(host);
			process.setAttribute(function);
		}
		save(deploymentFile);

	}

	/**
	 * 
	 */
	@Deprecated
	private static void cdProt() {
		for (int i = 0; i < PSGSimulator.size; i++) {
			Element process = new Element("process");
			racine.addContent(process);
			Attribute host = new Attribute("host", String.valueOf(i));
			Attribute function = new Attribute("function",
					"psgsim.PSGProcessCycle");
			process.setAttribute(host);
			process.setAttribute(function);

		}
		save("deployment.xml");

	}

	/**
	 * Reads given configuration property: "platform". If not found, returns the
	 * default value.
	 * 
	 * @return the platform file
	 */
	public static String platformFile() {
		String defFile = platformFile;
		String file = Configuration.getString("platform", defFile);
		return file;
	}

	/**
	 * Reads given configuration property: "unit". If not found, returns the
	 * default value (ms).
	 * 
	 * @return the unit of measure
	 */
	public static timeUnit unit() {
		String defUnit = sec;
		String unit = Configuration.getString("unit", defUnit);
		timeUnit t = timeUnit.valueOf(unit);
		return t;
	}

	/**
	 * Create the deployment file
	 * 
	 * @param file
	 *            the name of the deployment file
	 */
	public static void save(String file) {
		try {
			// On utilise ici un affichage classique avec getPrettyFormat()
			XMLOutputter out = new XMLOutputter(Format.getPrettyFormat());
			out.output(document, new FileOutputStream(file));
		} catch (java.io.IOException e) {
		}
	}

	/**
	 * Delete the deployment file
	 * 
	 * @param path
	 *            the path of the deployment file
	 */
	public static void delete(String path) {
		File file = new File(path);
		try {
			file.delete();
		} catch (Exception e) {
			System.err.println("deployment file not found");

		}
		System.err.println(path + "file deleted");

	}

}
