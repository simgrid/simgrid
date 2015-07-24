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

import java.lang.reflect.*;
import java.util.*;
import org.lsmp.djep.groupJep.*;

/**
 * This class is the container for the configuration data used in
 * {@link Configuration}; see that class for more information.
 */
public class ConfigContainer
{

// =================== static fields =================================
// ===================================================================

/** Symbolic constant for no debug */
private static final int DEBUG_NO = 0;

/** Symbolic constant for regular debug */
private static final int DEBUG_REG = 1;

/** Symbolic constant for extended debug */
private static final int DEBUG_CONTEXT = 2;

//========================== fields =================================
//===================================================================

/**
 * The properties object that stores all configuration information.
 */
private Properties config;

/**
 * Map associating string protocol names to the numeric protocol
 * identifiers. The protocol names are understood without prefix.
 */
private Map<String, Integer> protocols;

/**
 * The maximum depth that can be reached when analyzing expressions. This
 * value can be substituted by setting the configuration parameter
 * PAR_MAXDEPTH.
 */
private int maxdepth;

/** Debug level */
private int debugLevel;

/**
 * If true, no exception is thrown. Instead, an error is printed and the
 * Configuration tries to return a reasonable return value
 */
private boolean check = false;

// =================== initialization ================================
// ===================================================================

public ConfigContainer(Properties config, boolean check)
{
	this.config = config;
	this.check = check;
	maxdepth = getInt(Configuration.PAR_MAXDEPTH, Configuration.DEFAULT_MAXDEPTH);

	// initialize protocol id-s
	protocols = new HashMap<String, Integer>();
	String[] prots = getNames(Configuration.PAR_PROT);// they're returned in correct order
	for (int i = 0; i < prots.length; ++i) {
		protocols.put(prots[i].substring(Configuration.PAR_PROT.length() + 1), Integer.valueOf(i));
	}
	String debug = config.getProperty(Configuration.PAR_DEBUG);
	if (Configuration.DEBUG_EXTENDED.equals(debug))
		debugLevel = DEBUG_CONTEXT;
	else if (Configuration.DEBUG_FULL.equals(debug)) {
		Map<String, String> map = new TreeMap<String, String>();
		Enumeration e = config.propertyNames();
		while (e.hasMoreElements()) {
			String name = (String) e.nextElement();
			String value = config.getProperty(name);
			map.put(name, value);
		}
		Iterator i = map.keySet().iterator();
		while (i.hasNext()) {
			String name = (String) i.next();
			System.err.println("DEBUG " + name
					+ ("".equals(map.get(name)) ? "" : " = " + map.get(name)));
		}
	} else if (debug != null) {
		debugLevel = DEBUG_REG;
	} else {
		debugLevel = DEBUG_NO;
	}
}

// =================== static public methods =========================
// ===================================================================

/**
 * @return true if and only if name is a specified (existing) property.
 */
public boolean contains(String name)
{
	boolean ret = config.containsKey(name);
	debug(name, "" + ret);
	return ret;
}

// -------------------------------------------------------------------

/**
 * Reads given configuration property. If not found, throws a
 * {@link MissingParameterException}.
 * @param name
 *          Name of configuration property
 * @param def
 *          default value
 */
public boolean getBoolean(String name, boolean def)
{
	try {
		return getBool(name);
	} catch (RuntimeException e) {
		manageDefault(name, def, e);
		return def;
	}
}

// -------------------------------------------------------------------


/**
 * Reads given property. If not found, or the value is empty string then
 * throws a {@link MissingParameterException}. Empty string is not
 * accepted as false due to the similar function of {@link #contains} which
 * returns true in that case. True is returned if the lowercase value of
 * the property is "true", otherwise false is returned.
 * @param name
 *          Name of configuration property
 */
public boolean getBoolean(String name)
{
	try {
		return getBool(name);
	} catch (RuntimeException e) {
		manageException(name, e);
		return false;
	}
}

//-------------------------------------------------------------------

/**
 * The actual methods that implements getBoolean.
 */
private boolean getBool(String name)
{
	if (config.getProperty(name) == null) {
		throw new MissingParameterException(name);
//				"\nPossibly incorrect property: " + getSimilarProperty(name));
	}
	if (config.getProperty(name).matches("\\p{Blank}*")) {
		throw new MissingParameterException(name,
				"Blank value is not accepted when parsing Boolean.");
	}
	boolean ret = Boolean.valueOf(config.getProperty(name));
	debug(name, "" + ret);
	return ret;
}

// -------------------------------------------------------------------

/**
 * Reads given configuration property. If not found, returns the default
 * value.
 * @param name
 *          Name of configuration property
 * @param def
 *          default value
 */
public int getInt(String name, int def)
{
	try {
		Number ret = getVal(name, name, 0);
		debug(name, "" + ret);
		return ret.intValue();
	} catch (RuntimeException e) {
		manageDefault(name, def, e);
		return def;
	}
}

// -------------------------------------------------------------------

/**
 * Reads given configuration property. If not found, throws a
 * {@link MissingParameterException}.
 * @param name
 *          Name of configuration property
 */
public int getInt(String name)
{
	try {
		Number ret = getVal(name, name, 0);
		debug(name, "" + ret);
		return ret.intValue();
	} catch (RuntimeException e) {
		manageException(name, e);
		return 0;
	}
}

// -------------------------------------------------------------------

/**
 * Reads given configuration property. If not found, returns the default
 * value.
 * @param name
 *          Name of configuration property
 * @param def
 *          default value
 */
public long getLong(String name, long def)
{
	try {
		Number ret = getVal(name, name, 0);
		debug(name, "" + ret);
		return ret.longValue();
	} catch (RuntimeException e) {
		manageDefault(name, def, e);
		return def;
	}
}

// -------------------------------------------------------------------

/**
 * Reads given configuration property. If not found, throws a
 * {@link MissingParameterException}.
 * @param name
 *          Name of configuration property
 */
public long getLong(String name)
{
	try {
		Number ret = getVal(name, name, 0);
		debug(name, "" + ret);
		return ret.longValue();
	} catch (RuntimeException e) {
		manageException(name, e);
			return 0;
	}
}

// -------------------------------------------------------------------

/**
 * Reads given configuration property. If not found, returns the default
 * value.
 * @param name
 *          Name of configuration property
 * @param def
 *          default value
 */
public double getDouble(String name, double def)
{
	try {
		Number ret = getVal(name, name, 0);
		debug(name, "" + ret);
		return ret.doubleValue();
	} catch (RuntimeException e) {
		manageDefault(name, def, e);
		return def;
	}
}

// -------------------------------------------------------------------

/**
 * Reads given configuration property. If not found, throws a
 * MissingParameterException.
 * @param name
 *          Name of configuration property
 */
public double getDouble(String name)
{
	try {
		Number ret = getVal(name, name, 0);
		debug(name, "" + ret);
		return ret.doubleValue();
	} catch (RuntimeException e) {
		manageException(name, e);
		return 0;
	}
}

// -------------------------------------------------------------------

/**
 * Read numeric property values, parsing expression if necessary.
 * 
 * @param initial
 *          the property name that started this expression evaluation
 * @param property
 *          the current property name to be evaluated
 * @param depth
 *          the depth reached so far
 * @return the evaluation of the expression associated to property
 */
private Number getVal(String initial, String property, int depth)
{
	if (depth > maxdepth) {
		throw new IllegalParameterException(initial,
				"Probable recursive definition - exceeded maximum depth " + 
				maxdepth);
	}

	String s = config.getProperty(property);
	if (s == null || s.equals("")) {
		throw new MissingParameterException(property,
				" when evaluating property " + initial);
//						+ "\nPossibly incorrect property: " + getSimilarProperty(property));
	}

	GroupJep jep = new GroupJep(new Operators());
	jep.setAllowUndeclared(true);

	jep.parseExpression(s);
	String[] symbols = getSymbols(jep);
	for (int i = 0; i < symbols.length; i++) {
		Object d = getVal(initial, symbols[i], depth + 1);
		jep.addVariable(symbols[i], d);
	}
	Object ret = jep.getValueAsObject();
	if (jep.hasError())
		System.err.println(jep.getErrorInfo());
	return (Number) ret;
}

// -------------------------------------------------------------------

/**
 * Returns an array of string, containing the symbols contained in the
 * expression parsed by the specified JEP parser.
 * @param jep
 *          the java expression parser containing the list of variables
 * @return an array of strings.
 */
private String[] getSymbols(org.nfunk.jep.JEP jep)
{
	Hashtable h = jep.getSymbolTable();
	String[] ret = new String[h.size()];
	Enumeration e = h.keys();
	int i = 0;
	while (e.hasMoreElements()) {
		ret[i++] = (String) e.nextElement();
	}
	return ret;
}

// -------------------------------------------------------------------

/**
 * Reads given configuration property. If not found, returns the default
 * value.
 * @param name
 *          Name of configuration property
 * @param def
 *          default value
 */
public String getString(String name, String def)
{
	try {
		return getStr(name);
	} catch (RuntimeException e) {
		manageDefault(name, def, e);
		return def;
	} 
}

// -------------------------------------------------------------------

/**
 * Reads given configuration property. If not found, throws a
 * MissingParameterException. Removes trailing whitespace characters.
 * @param name
 *          Name of configuration property
 */
public String getString(String name)
{
	try {
		return getStr(name);
	} catch (RuntimeException e) {
		manageException(name, e);
		return "";
	}
}

/**
 * The actual method implementing getString().
 */
private String getStr(String name)
{
	String result = config.getProperty(name);
	if (result == null) {
		throw new MissingParameterException(name);
//				"\nPossibly incorrect property: " + getSimilarProperty(name));
	}
	debug(name, "" + result);

	return result.trim();
}

// -------------------------------------------------------------------

/**
 * Reads the given property from the configuration interpreting it as a
 * protocol name. Returns the numeric protocol identifier of this protocol
 * name. See the discussion of protocol name at {@link Configuration} for
 * details on how this numeric id is calculated
 * 
 * @param name
 *          Name of configuration property
 * @return the numeric protocol identifier associated to the value of the
 *         property
 */
public int getPid(String name)
{
	try {
		String protname = getStr(name);
		return lookupPid(protname);
	} catch (RuntimeException e) {
		manageException(name, e);
		return 0;
	}
}

// -------------------------------------------------------------------

/**
 * Calls {@link #getPid(String)}, and returns the default if no property
 * is defined with the given name.
 * 
 * @param name
 *          Name of configuration property
 * @param pid
 *          the default protocol identifier
 * @return the numeric protocol identifier associated to the value of the
 *         property, or the default if not defined
 */
public int getPid(String name, int pid)
{
	try {
		String protname = getStr(name);
		return lookupPid(protname);
	} catch (RuntimeException e) {
		manageDefault(name, pid, e);
		return pid;
	}
}

// -------------------------------------------------------------------

/**
 * Returns the numeric protocol identifier of the given protocol name.
 * 
 * @param protname
 *          the protocol name.
 * @return the numeric protocol identifier associated to the protocol name
 */
public int lookupPid(String protname)
{
	Integer ret = protocols.get(protname);
	if (ret == null) {
		throw new MissingParameterException(Configuration.PAR_PROT + "." + protname);
//				"\nPossibly incorrect property: "
//				+ getSimilarProperty(PAR_PROT + "." + protname));
	}
	return ret.intValue();
}

// -------------------------------------------------------------------

/**
 * Returns the name of a protocol that has the given identifier.
 * <p>
 * Note that this is not a constant time operation in the number of
 * protocols, although typically there are very few protocols defined.
 * 
 * @param pid
 *          numeric protocol identifier.
 * @return name of the protocol that has the given id. null if no protocols
 *         have the given id.
 */
public String lookupPid(int pid)
{

	if (!protocols.containsValue(pid))
		return null;
	for (Map.Entry<String, Integer> i : protocols.entrySet()) {
		if (i.getValue().intValue() == pid)
			return i.getKey();
	}

	// never reached but java needs it...
	return null;
}

// -------------------------------------------------------------------

/**
 * Reads given configuration property. If not found, throws a
 * {@link MissingParameterException}. When creating the Class object, a
 * few attempts are done to resolve the classname. See
 * {@link Configuration} for details.
 * @param name
 *          Name of configuration property
 */
public Class getClass(String name)
{
	try {
		return getClazz(name);
	} catch (RuntimeException e) {
		manageException(name, e);
		return null;
	}
}

private Class getClazz(String name)
{
	String classname = config.getProperty(name);
	if (classname == null) {
		throw new MissingParameterException(name);
//				"\nPossibly incorrect property: " + getSimilarProperty(name));
	}
	debug(name, classname);

	Class c = null;

	try {
		// Maybe classname is just a fully-qualified name
		c = Class.forName(classname);
	} catch (ClassNotFoundException e) {
	}
	if (c == null) {
		// Maybe classname is a non-qualified name?
		String fullname = ClassFinder.getQualifiedName(classname);
		if (fullname != null) {
			try {
				c = Class.forName(fullname);
			} catch (ClassNotFoundException e) {
			}
		}
	}
	if (c == null) {
		// Maybe there are multiple classes with the same
		// non-qualified name.
		String fullname = ClassFinder.getQualifiedName(classname);
		if (fullname != null) {
			String[] names = fullname.split(",");
			if (names.length > 1) {
				for (int i = 0; i < names.length; i++) {
					for (int j = i + 1; j < names.length; j++) {
						if (names[i].equals(names[j])) {
							throw new IllegalParameterException(name,
									"The class " + names[i]
								+ " appears more than once in the classpath; please check"
								+ " your classpath to avoid duplications.");
						}
					}
				}
				throw new IllegalParameterException(name,
						"The non-qualified class name " + classname
								+ "corresponds to multiple fully-qualified classes:" + fullname);
			}
		}
	}
	if (c == null) {
		// Last attempt: maybe the fully classified name is wrong,
		// but the classname is correct.
		String shortname = ClassFinder.getShortName(classname);
		String fullname = ClassFinder.getQualifiedName(shortname);
		if (fullname != null) {
			throw new IllegalParameterException(name, "Class "
					+ classname + " does not exist. Possible candidate(s): " + fullname);
		}
	}
	if (c == null) {
		throw new IllegalParameterException(name, "Class "
				+ classname + " not found");
	}
	return c;
}

// -------------------------------------------------------------------

/**
 * Reads given configuration property. If not found, returns the default
 * value.
 * @param name
 *          Name of configuration property
 * @param def
 *          default value
 * @see #getClass(String)
 */
public Class getClass(String name, Class def)
{

	try {
		return Configuration.getClass(name);
	} catch (RuntimeException e) {
		manageDefault(name, def, e);
		return def;
	}
}

// -------------------------------------------------------------------

/**
 * Reads given configuration property for a class name. It returns an
 * instance of the class. The class must implement a constructor that takes
 * a String as an argument. The value of this string will be <tt>name</tt>.
 * The constructor of the class can see the configuration so it can make
 * use of this name to read its own parameters from it.
 * @param name
 *          Name of configuration property
 * @throws MissingParameterException
 *           if the given property is not defined
 * @throws IllegalParameterException
 *           if there is any problem creating the instance
 */
public Object getInstance(String name)
{
  try {
  	return getInst(name);
  } catch (RuntimeException e) {
		manageException(name, e);
		return null;
  }
}

/**
 * The actual method implementing getInstance().
 */
private Object getInst(String name)
{
	Class c = getClass(name);
	if (c == null)
		return null;
	final String classname = c.getSimpleName();

	try {
		Class pars[] = {String.class};
		Constructor cons = c.getConstructor(pars);
		Object objpars[] = {name};
		return cons.newInstance(objpars);
	} catch (NoSuchMethodException e) {
		throw new IllegalParameterException(name, "Class "
				+ classname + " has no " + classname + "(String) constructor");
	} catch (InvocationTargetException e) {
		if (e.getTargetException() instanceof RuntimeException) {
			throw (RuntimeException) e.getTargetException();
		} else {
			e.getTargetException().printStackTrace();
			throw new RuntimeException("" + e.getTargetException());
		}
	} catch (Exception e) {
		throw new IllegalParameterException(name, e + "");
	}
}

// -------------------------------------------------------------------

/**
 * Reads given configuration property for a class name. It returns an
 * instance of the class. The class must implement a constructor that takes
 * a String as an argument. The value of this string will be <tt>name</tt>.
 * The constructor of the class can see the configuration so it can make
 * use of this name to read its own parameters from it.
 * @param name
 *          Name of configuration property
 * @param def
 *          The default object that is returned if there is no property
 *          defined with the given name
 * @throws IllegalParameterException
 *           if the given name is defined but there is a problem creating
 *           the instance.
 */
public Object getInstance(String name, Object def)
{
  if (!contains(name)) 
  	return def;
	try {
		return getInst(name);
	} catch (RuntimeException e) {
		manageException(name, e);
		return def;
	}
}

// -------------------------------------------------------------------

/**
 * It returns an array of class instances. The instances are constructed by
 * calling {@link #getInstance(String)} on the names returned by
 * {@link #getNames(String)}.
 * @param name
 *          The component type (i.e. prefix of the list of configuration
 *          properties) which will be passed to {@link #getNames(String)}.
 */
public Object[] getInstanceArray(String name)
{

	String names[] = getNames(name);
	Object[] result = new Object[names.length];

	for (int i = 0; i < names.length; ++i) {
		result[i] = getInstance(names[i]);
	}

	return result;
}

// -------------------------------------------------------------------

/**
 * Returns an array of names prefixed by the specified name. The array is
 * sorted as follows. If there is no config entry
 * <code>{@value peersim.config.Configuration#PAR_INCLUDE}+"."+name</code> or
 * <code>{@value peersim.config.Configuration#PAR_ORDER}+"."+name</code> then the order is
 * alphabetical. Otherwise this entry defines the order. For more
 * information see {@link Configuration}.
 * @param name
 *          the component type (i.e., the prefix)
 * @return the full property names in the order specified by the
 *         configuration
 */
public String[] getNames(String name)
{
	ArrayList<String> ll = new ArrayList<String>();
	final String pref = name + ".";

	Enumeration e = config.propertyNames();
	while (e.hasMoreElements()) {
		String key = (String) e.nextElement();
		if (key.startsWith(pref) && key.indexOf(".", pref.length()) < 0)
			ll.add(key);
	}
	String[] ret = ll.toArray(new String[ll.size()]);
	return order(ret, name);
}

// -------------------------------------------------------------------

/**
 * The input of this method is a set of property <code>names</code> (e.g.
 * initializers, controls and protocols) and a string specifying the type
 * (prefix) of these. The output is in <code>names</code>, which will
 * contain a permutation of the original array. Parameter
 * PAR_INCLUDE+"."+type, or if not present, PAR_ORDER+"."+type is read from
 * the configuration. If none of them are defined then the order is
 * identical to that of <code>names</code>. Otherwise the configuration
 * entry must contain entries from <code>names</code>. It is assumed
 * that the entries in <code>names</code> contain only word characters
 * (alphanumeric and underscore '_'. The order configuration entry thus
 * contains a list of entries from <code>names</code> separated by any
 * non-word characters.
 * <p>
 * It is not required that all entries are listed. If PAR_INCLUDE is used,
 * then only those entries are returned that are listed. If PAR_ORDER is
 * used, then all names are returned, but the array will start with those
 * that are listed. The rest of the names follow in alphabetical order.
 * 
 * 
 * @param names
 *          the set of property names to be searched
 * @param type
 *          the string identifying the particular set of properties to be
 *          inspected
 */
private String[] order(String[] names, String type)
{
	String order = getString(Configuration.PAR_INCLUDE + "." + type, null);
	boolean include = order != null;
	if (!include)
		order = getString(Configuration.PAR_ORDER + "." + type, null);

	int i = 0;
	if (order != null && !order.equals("")) {
		// split around non-word characters
		String[] sret = order.split("\\W+");
		for (; i < sret.length; i++) {
			int j = i;
			for (; j < names.length; ++j)
				if (names[j].equals(type + "." + sret[i]))
					break;
			if (j == names.length) {
				throw new IllegalParameterException(
						(include ? Configuration.PAR_INCLUDE : Configuration.PAR_ORDER)
						+ "." + type, type + "." + sret[i] + " is not defined.");
			} else // swap the element to current position
			{
				String tmps = names[j];
				names[j] = names[i];
				names[i] = tmps;
			}
		}
	}

	Arrays.sort(names, i, names.length);
	int retsize = (include ? i : names.length);
	String[] ret = new String[retsize];
	for (int j = 0; j < retsize; ++j)
		ret[j] = names[j];
	return ret;
}

// -------------------------------------------------------------------

/**
 * Print debug information for configuration. The amount of information
 * depends on the debug level DEBUG. 0 = nothing 1 = just the config name 2 =
 * config name plus method calling
 * 
 * @param name
 */
private void debug(String name, String result)
{
	if (debugLevel == DEBUG_NO)
		return;
	StringBuffer buffer = new StringBuffer();
	buffer.append("DEBUG ");
	buffer.append(name);
	buffer.append(" = ");
	buffer.append(result);

	// Additional info
	if (debugLevel == DEBUG_CONTEXT) {

		buffer.append("\n  at ");
		// Obtain the stack trace
		StackTraceElement[] stack = null;
		try {
			throw new Exception();
		} catch (Exception e) {
			stack = e.getStackTrace();
		}

		// Search the element that invoked Configuration
		// It's the first whose class is different from Configuration
		int pos;
		for (pos = 0; pos < stack.length; pos++) {
			if (!stack[pos].getClassName().equals(Configuration.class.getName()))
				break;
		}

		buffer.append(stack[pos].getClassName());
		buffer.append(":");
		buffer.append(stack[pos].getLineNumber());
		buffer.append(", method ");
		buffer.append(stack[pos - 1].getMethodName());
		buffer.append("()");
	}

	System.err.println(buffer);
}

// -------------------------------------------------------------------

/**
 * @return an array of adjacent letter pairs contained in the input string
 *         http://www.catalysoft.com/articles/StrikeAMatch.html
 */
private String[] letterPairs(String str)
{
	int numPairs = str.length() - 1;
	String[] pairs = new String[numPairs];
	for (int i = 0; i < numPairs; i++) {
		pairs[i] = str.substring(i, i + 2);
	}
	return pairs;
}

// -------------------------------------------------------------------

/**
 * @return an ArrayList of 2-character Strings.
 *         http://www.catalysoft.com/articles/StrikeAMatch.html
 */
private ArrayList<String> wordLetterPairs(String str)
{
	ArrayList<String> allPairs = new ArrayList<String>();
	// Tokenize the string and put the tokens/words into an array
	String[] words = str.split("\\s");
	// For each word
	for (int w = 0; w < words.length; w++) {
		// Find the pairs of characters
		String[] pairsInWord = letterPairs(words[w]);
		for (int p = 0; p < pairsInWord.length; p++) {
			allPairs.add(pairsInWord[p]);
		}
	}
	return allPairs;
}

// -------------------------------------------------------------------

/**
 * @return lexical similarity value in the range [0,1]
 *         http://www.catalysoft.com/articles/StrikeAMatch.html
 */
private double compareStrings(String str1, String str2)
{
	ArrayList pairs1 = wordLetterPairs(str1.toUpperCase());
	ArrayList pairs2 = wordLetterPairs(str2.toUpperCase());
	int intersection = 0;
	int union_ = pairs1.size() + pairs2.size();
	for (int i = 0; i < pairs1.size(); i++) {
		Object pair1 = pairs1.get(i);
		for (int j = 0; j < pairs2.size(); j++) {
			Object pair2 = pairs2.get(j);
			if (pair1.equals(pair2)) {
				intersection++;
				pairs2.remove(j);
				break;
			}
		}
	}
	return (2.0 * intersection) / union_;
}

// -------------------------------------------------------------------

/**
 * Among the defined properties, returns the one more similar to String
 * property
 */
private String getSimilarProperty(String property)
{
	String bestProperty = null;
	double bestValue = 0.0;
	Enumeration e = config.keys();
	while (e.hasMoreElements()) {
		String key = (String) e.nextElement();
		double compare = compareStrings(key, property);
		if (compare > bestValue) {
			bestValue = compare;
			bestProperty = key;
		}
	}
	return bestProperty;
}

//-------------------------------------------------------------------

private void manageDefault(String name, Object def, 
		RuntimeException e)
{
	debug(name, "" + def + " (DEFAULT)");
	if (check) {
		System.out.println("Warning: Property " + name + " = " + 
				def + " (DEFAULT)");
	}
	if (e instanceof MissingParameterException) {
		// Do nothing
	} else {
		manageException(name, e);
	}
}

//-------------------------------------------------------------------

private void manageException(String name, RuntimeException e)
{
	if (check) {
		if (e instanceof MissingParameterException) {
			// Print just the short message in this case
			System.out.println("Error: " + 
					((MissingParameterException) e).getShortMessage());
		} else if (e instanceof IllegalParameterException) {
			// Print just the short message in this case
			System.out.println("Error: " + 
					((IllegalParameterException) e).getShortMessage());
		} else {
			System.out.println("Error: " + e.getMessage());
		}
	} else {
		throw e;
	}
}

//-------------------------------------------------------------------

}
