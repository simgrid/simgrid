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
 * Do testing the vector package.
 */
public class TestVectors extends SingleValueHolder
{

//--------------------------------------------------------------------------
//Fields
//--------------------------------------------------------------------------
	
/** Value held by this protocol */
protected float fvalue;

/** Value held by this protocol */
protected int ivalue;

/** Value held by this protocol */
protected long lvalue;
	

//--------------------------------------------------------------------------
//Initialization
//--------------------------------------------------------------------------

/**
 * Builds a new (not initialized) value holder.
 * Calls super constructor.
 */
public TestVectors(String prefix) { super(prefix); }

//--------------------------------------------------------------------------
//methods
//--------------------------------------------------------------------------

/**
 * 
 */
public int getIValue() { return ivalue; }

//--------------------------------------------------------------------------

/**
 * 
 */
public void setIValue(int value) { ivalue = value; }

//--------------------------------------------------------------------------

/**
 * 
 */
public float getFValue() { return fvalue; }

//--------------------------------------------------------------------------

/**
 * 
 */
public void setFValue(float value) { fvalue = value; }

//--------------------------------------------------------------------------

/**
 * 
 */
public long getLValue() { return lvalue; }

//--------------------------------------------------------------------------

/**
 * 
 */
public void setLValue(long value) { lvalue = value; }

//--------------------------------------------------------------------------

/**
 * Returns the value as a string.
 */
public String toString() { return value+" "+fvalue+" "+ivalue+" "+lvalue; }

}

