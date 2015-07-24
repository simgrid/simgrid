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

import java.util.*;

/**
 * Fully static class to store configuration information. It defines a
 * method, {@link #setConfig(Properties)}, to set configuration data. This
 * method is called by the simulator engines as the very first thing they
 * do. It can be called only once, after that the class becomes read only.
 * All components can then access this configuration and utility methods to
 * read property values based on their names.
 * <p>
 * The design of this class also hides the actual implementation of the
 * configuration which can be Properties, XML, whatever. Currently only
 * Properties is supported.
 * <p>
 * Apart from storing (name,value) pairs, this class also does some
 * processing, and offers some utility functions. This extended
 * functionality consists of the following: reading values with type
 * checking, ordering of entries, pre-processing protocol names, parsing
 * expressions, resolving underspecified classnames, and finally some basic
 * debugging possibilities. We discuss these in the following.
 * <p>
 * Note that the configuration is initialized using a Properties object.
 * The class of this object might implement some additional pre-processing
 * on the file or provide an extended syntax for defining property files.
 * See {@link ParsedProperties} for more details. This is the class that is
 * currently used by simulation engines.
 * <h3>Typed reading of values</h3>
 * Properties can have arbitrary values of type String. This class offers a
 * set of read methods that perform the appropriate conversion of the
 * string value to the given type, eg long. They also allow for specifying
 * default values in case the given property is not specified.
 * <h3>Resolving class names</h3>
 * 
 * The possibilities for the typed reading of a value includes interpreting
 * the value as a class name. In this case an object will be constructed.
 * It is described at method {@link #getInstance(String)} how this is
 * achieved exactly. What needs to be noted here is that the property value
 * need not be a fully specified classname. It might contain only the short
 * class name without the package specification. In this case, it is
 * attempted to locate a class with that name in the classpath, and if a
 * unique class is found, it will be used. This simplifies the
 * configuration files and also allows to remove their dependence on the
 * exact location of the class.
 * 
 * <h3>Components and their ordering</h3>
 * The idea of the configuration is that it mostly contains components and
 * their descriptions (parameters). Although this class is blind to the
 * semantics of these components, it offers some low level functionality
 * that helps dealing with them. This functionality is based on the
 * assumption that components have a type and a name. Both types and names
 * are strings of alphanumeric and underscore characters. For example,
 * {@value #PAR_PROT} is a type, "foo" can be a name. Method
 * {@link #getNames} allow the caller to get the list of names for a given
 * type. Some other methods, like {@link #getInstanceArray} use this list
 * to return a list of components.
 * 
 * <p>
 * Assuming the configuration is in Properties format (which is currently
 * the only format available) component types and names are defined as
 * follows. Property names containing two non-empty words separated by one
 * dot (".") character are treated specially (the words contain word
 * characters: alphanumeric and underscore ("_")). The first word will be
 * the type, and the second is the name of a component. For example,
 * 
 * <pre>
 *   control.conn ConnectivityObserver
 *   control.1 WireKOut
 *   control.2 PrintGraph
 * </pre>
 * 
 * defines control components of names "conn","1" an "2" (arguments of the
 * components not shown). When {@link #getNames} or
 * {@link #getInstanceArray} are called, eg
 * <code>getNames("control")</code>, then the order in which these are
 * returned is alphabetical:
 * <code>["control.1","control.2","control.conn"]</code>. If you are not
 * satisfied with lexicographic order, you can specify the order in this
 * way.
 * 
 * <pre>
 *   order.control 1,conn,2
 * </pre>
 * 
 * where the names are separated by any non-word character (non
 * alphanumeric or underscore). If not all names are listed then the given
 * order is followed by alphabetical order of the rest of the items, e.g.
 * 
 * <pre>
 *   order.control 2
 * </pre>
 * 
 * results in <code>["control.2","control.1","control.conn"]</code>.
 * <p>
 * It is also possible to exclude elements from the list, while ordering
 * them. The syntax is identical to that of the above, only the parameter
 * name begins with <code>include</code>. For example
 * 
 * <pre>
 *   include.control conn 2
 * </pre>
 * 
 * will result in returning <em>only</em> <code>control.conn</code> and
 * <code>control.2</code>, in this order. Note that for example the
 * empty list results in a zero length array in this case.
 * <em>Important!</em> If include is defined then ordering is ignored.
 * That is, include is stronger than order.
 * <h3>Protocol names</h3>
 * As mentioned, the configuration is generally blind to the actual names
 * of the components. There is an exception: the components of type
 * {@value #PAR_PROT}. These are pre-processed a bit to enhance
 * performance: protocol names are mapped to numeric protocol identifiers.
 * The numeric identifier of a protocol is its index in the array returned
 * by {@link #getNames}. See above how to control this order. The numeric
 * identifiers then can be looked up based on the name and vice versa.
 * Besides, the identifier can be directly requested based on a property
 * name when the protocol name is the value of a property which is
 * frequently the case.
 * <p>
 * <h3>Expressions</h3>
 * Numeric property values can be complex expressions, that are parsed
 * using <a href="http://www.singularsys.com/jep/">JEP</a>. You can write
 * expression using the syntax that you can find <a
 * href="http://www.singularsys.com/jep/doc/html/op_and_func.html"> here</a>.
 * For example,
 * 
 * <pre>
 *   MAG 2
 *   SIZE 2&circ;MAG
 * </pre>
 * 
 * SIZE=4. You can also have complex expression trees like this:
 * 
 * <pre>
 *   A B+C
 *   B D+E
 *   C E+F
 *   D 1
 *   E F
 *   F 2
 * </pre>
 * 
 * that results in A=7, B=3, C=4, D=1, E=2, F=2
 * 
 * <p>
 * Expressions like "sub-expression op sub-expression" are computed based
 * on the type of the sub-expressions. If both sub-expressions are integer,
 * the computation is done using integer arithmetics and the result is an
 * integer. So, for example, 5/2 returns 2. If one of the sub-expression is
 * floating point, the computation is based on floating-point arithmetics
 * (double precision) and the result is a floating point value. So, for
 * example, 5.0/2 returns 2.5.
 * 
 * <p>
 * Expressions are parsed recursively. Note that no optimization is done,
 * so expression F is evaluated three times here (due to the fact that
 * appears twice in C and once in B). But since properties are read just
 * once at initialization, this is not a performance problem.
 * 
 * <p>
 * Finally, recursive definitions are not allowed (and without function
 * definitions, they make no sense). Since it is difficult to discover
 * complex recursive chains, a simple trick is used: if the depth of
 * recursion is greater than a given threshold (configurable, currently
 * {@value #DEFAULT_MAXDEPTH}, an error message is printed. This avoids to
 * fill the stack, that results in an anonymous OutOfMemoryError. So, if
 * you write
 * 
 * <pre>
 *   overlay.size SIZE
 *   SIZE SIZE-1
 * </pre>
 * 
 * you get an error message: Parameter "overlay.size": Probable recursive
 * definition - exceeded maximum depth {@value #DEFAULT_MAXDEPTH}
 * 
 * <h3>Debug</h3>
 * 
 * It is possible to obtain debug information about the configuration
 * properties by activating special configuration properties.
 * <p>
 * If property {@value #PAR_DEBUG} is defined, each config property and the
 * associated value are printed. Properties that are not present in the
 * config file but have default values are postfixed with the string
 * "(DEFAULT)".
 * <p>
 * If property {@value #PAR_DEBUG} is defined and it is equal to
 * {@value #DEBUG_EXTENDED}, information about the configuration method
 * invoked, and where this method is invoked, is also printed. If it is
 * equal to {@value #DEBUG_FULL}, all the properties are printed, even if
 * they are not read.
 * <p>
 * Each line printed by this debug feature is prefixed by the string
 * "DEBUG".
 * 
 * <h3>Use of brackets</h3>
 * 
 * For the sake of completeness, we mention it here that if this class is
 * initialized using {@link ParsedProperties}, then it is possible to use
 * some more compressed format to specify the components. See
 * {@link ParsedProperties#load}.
 * 
 */
public class Configuration
{

// =================== static fields =================================
// ===================================================================

/** Default max depth limit to avoid recursive definitions */
public static final int DEFAULT_MAXDEPTH = 100;

/**
 * The debug level for the configuration. If defined, a line is printed for
 * each configuration parameter read. If defined and equal to
 * {@value #DEBUG_EXTENDED}, additional context information for debug is
 * printed. If defined and equal to {@value #DEBUG_FULL}, all the
 * configuration properties are printed at the beginning, not just when
 * they are called.
 * @config
 */
static final String PAR_DEBUG = "debug.config";

/**
 * If parameter {@value #PAR_DEBUG} is equal to this string, additional
 * context information for debug is printed.
 */
static final String DEBUG_EXTENDED = "context";

/**
 * If parameter {value #PAR_DEBUG} is equal to this string, all the
 * configuration properties are printed at the beginning, not just when
 * they are called.
 */
static final String DEBUG_FULL = "full";

/**
 * The maximum depth for expressions. This is a simple mechanism to avoid
 * unbounded recursion. The default is {@value #DEFAULT_MAXDEPTH}, and you
 * probably don't want to change it.
 * @config
 */
static final String PAR_MAXDEPTH = "expressions.maxdepth";

/**
 * Used to configure ordering of the components. Determines the ordering in
 * the array as returned by {@link #getNames}. See the general description
 * of {@link Configuration} for details.
 * @config
 */
static final String PAR_ORDER = "order";

/**
 * Used to configure ordering of the components. Determines the ordering in
 * the array as returned by {@link #getNames}, and can bu used to also
 * exclude elements. See the general description of {@link Configuration}
 * for details.
 * @config
 */
static final String PAR_INCLUDE = "include";

// XXX it's ugly because it replicates the definition of Node.PAR_PROT, but
// this would be the only dependence on the rest of the core...
/**
 * The type name of components describing protocols. This is the only point
 * at which the class is not blind to the actual semantics of the
 * configuration.
 */
static final String PAR_PROT = "protocol";

/**
 * The properties object that stores all configuration information.
 */
private static ConfigContainer config = null;

// =================== initialization ================================
// ===================================================================

/** to prevent construction */
private Configuration()
{
}

// =================== static public methods =========================
// ===================================================================

/**
 * Sets the system-wide configuration in Properties format. It can be
 * called only once. After that the configuration becomes unmodifiable
 * (read only). If modification is attempted, a RuntimeException is thrown
 * and no change is made.
 * @param p
 *          The Properties object containing configuration info
 */
public static void setConfig(Properties p)
{
	if (config != null) {
		throw new RuntimeException("Setting configuration was attempted twice.");
	}
	config = new ConfigContainer(p, false);
}

// -------------------------------------------------------------------

/**
 * Sets the system-wide configuration in Properties format. It can be
 * called only once. After that the configuration becomes unmodifiable
 * (read only). If modification is attempted, a RuntimeException is thrown
 * and no change is made.
 * @param p
 *          The Properties object containing configuration info
 */
public static void setConfig(Properties p, boolean check)
{
	if (config != null) {
		throw new RuntimeException("Setting configuration was attempted twice.");
	}
	config = new ConfigContainer(p, check);
}

// -------------------------------------------------------------------

/**
 * @return true if and only if name is a specified (existing) property.
 */
public static boolean contains(String name)
{
	return config.contains(name);
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
public static boolean getBoolean(String name, boolean def)
{
	return config.getBoolean(name, def);
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
public static boolean getBoolean(String name)
{
	return config.getBoolean(name);
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
public static int getInt(String name, int def)
{
	return config.getInt(name, def);
}

// -------------------------------------------------------------------

/**
 * Reads given configuration property. If not found, throws a
 * {@link MissingParameterException}.
 * @param name
 *          Name of configuration property
 */
public static int getInt(String name)
{
	return config.getInt(name);
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
public static long getLong(String name, long def)
{
	return config.getLong(name, def);
}

// -------------------------------------------------------------------

/**
 * Reads given configuration property. If not found, throws a
 * {@link MissingParameterException}.
 * @param name
 *          Name of configuration property
 */
public static long getLong(String name)
{
	return config.getLong(name);
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
public static double getDouble(String name, double def)
{
	return config.getDouble(name, def);
}

// -------------------------------------------------------------------

/**
 * Reads given configuration property. If not found, throws a
 * MissingParameterException.
 * @param name
 *          Name of configuration property
 */
public static double getDouble(String name)
{
	return config.getDouble(name);
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
public static String getString(String name, String def)
{
	return config.getString(name, def);
}

// -------------------------------------------------------------------

/**
 * Reads given configuration property. If not found, throws a
 * MissingParameterException. Removes trailing whitespace characters.
 * @param name
 *          Name of configuration property
 */
public static String getString(String name)
{
	return config.getString(name);
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
public static int getPid(String name)
{
	return config.getPid(name);
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
public static int getPid(String name, int pid)
{	
	return config.getPid(name, pid);
}

// -------------------------------------------------------------------

/**
 * Returns the numeric protocol identifier of the given protocol name.
 * 
 * @param protname
 *          the protocol name.
 * @return the numeric protocol identifier associated to the protocol name
 */
public static int lookupPid(String protname)
{
	return config.lookupPid(protname);
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
public static String lookupPid(int pid)
{
	return config.lookupPid(pid);
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
public static Class getClass(String name)
{
	return config.getClass(name);
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
public static Class getClass(String name, Class def)
{
	return config.getClass(name, def);
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
public static Object getInstance(String name)
{
	return config.getInstance(name);
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
public static Object getInstance(String name, Object def)
{
	return config.getInstance(name, def);
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
public static Object[] getInstanceArray(String name)
{
	return config.getInstanceArray(name);
}

// -------------------------------------------------------------------

/**
 * Returns an array of names prefixed by the specified name. The array is
 * sorted as follows. If there is no config entry
 * <code>{@value #PAR_INCLUDE}+"."+name</code> or
 * <code>{@value #PAR_ORDER}+"."+name</code> then the order is
 * alphabetical. Otherwise this entry defines the order. For more
 * information see {@link Configuration}.
 * @param name
 *          the component type (i.e., the prefix)
 * @return the full property names in the order specified by the
 *         configuration
 */
public static String[] getNames(String name)
{
	return config.getNames(name);
}

}
