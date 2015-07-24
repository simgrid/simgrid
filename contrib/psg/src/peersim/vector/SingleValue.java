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

/**
* The implementor class has a single parameter. This interface
* provides access to that parameter.
*/
public interface SingleValue {

/**
 * Returns the value of the parameter hold by the implementor
 * of this interface. 
 */
public double getValue();

/**
 * Modifies the value of the parameter hold by the implementor
 * of this interface. 
 */
public void setValue(double value);

}

