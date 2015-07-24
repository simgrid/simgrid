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

import peersim.core.*;

/**
 * Serves as an abstract superclass for {@link Control}s that deal
 * with vectors.
 * It reads a {@link Setter} to be used by extending classes.
 */
public abstract class VectControl implements Control {


// --------------------------------------------------------------------------
// Parameter names
// --------------------------------------------------------------------------

/**
 * The protocol to operate on.
 * @config
 */
protected static final String PAR_PROT = "protocol";

/**
 * The setter method used to set values in the protocol instances. Defaults to
 * <code>setValue</code>
 * (for backward compatibility with previous implementation of this
 * class, that were based on the {@link SingleValue} interface). Refer to the
 * {@linkplain peersim.vector vector package description} for more
 * information about getters and setters.
 * @config
 */
protected static final String PAR_METHOD = "setter";

/**
 * The getter method used to obtain the protocol values. 
 * Defaults to <code>getValue</code>
 * (for backward compatibility with previous 
 * implementation of this class, that were based on the 
 * {@link SingleValue} interface).
 * Refer to the {@linkplain peersim.vector vector package description} for more 
 * information about getters and setters.
 * @config
 */
protected static final String PAR_GETTER = "getter";

// --------------------------------------------------------------------------
// Fields
// --------------------------------------------------------------------------

/** The setter to be used to write a vector. */
protected final Setter setter;

/** The getter to be used to read a vector. */
protected final Getter getter;

// --------------------------------------------------------------------------
// Initialization
// --------------------------------------------------------------------------

/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param prefix the configuration prefix for this class
 */
protected VectControl(String prefix)
{
	setter = new Setter(prefix,PAR_PROT,PAR_METHOD);
	getter = new Getter(prefix,PAR_PROT,PAR_GETTER);
}

}

