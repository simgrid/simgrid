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
 * The task of this protocol is to store a single double value and make it
 * available through the {@link SingleValue} interface.
 *
 * @author Alberto Montresor
 * @version $Revision: 1.6 $
 */
public class SingleValueHolder 
implements SingleValue, Protocol
{

//--------------------------------------------------------------------------
//Fields
//--------------------------------------------------------------------------
	
/** Value held by this protocol */
protected double value;
	

//--------------------------------------------------------------------------
//Initialization
//--------------------------------------------------------------------------

/**
 * Does nothing.
 */
public SingleValueHolder(String prefix)
{
}

//--------------------------------------------------------------------------

/**
 * Clones the value holder.
 */
public Object clone()
{
	SingleValueHolder svh=null;
	try { svh=(SingleValueHolder)super.clone(); }
	catch( CloneNotSupportedException e ) {} // never happens
	return svh;
}

//--------------------------------------------------------------------------
//methods
//--------------------------------------------------------------------------

/**
 * @inheritDoc
 */
public double getValue()
{
	return value;
}

//--------------------------------------------------------------------------

/**
 * @inheritDoc
 */
public void setValue(double value)
{
	this.value = value;
}

//--------------------------------------------------------------------------

/**
 * Returns the value as a string.
 */
public String toString() { return ""+value; }

}
