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
		
package peersim.core;


/**
* Instances of classes implementing this interface
* maintain a fail state, i.e. information about the availability
* of the object.
*/
public interface Fallible {
 

	/**
	* Fail state which indicates that the object is operating normally.
	*/
	public int OK = 0;

	/**
	* Fail state indicating that the object is dead and recovery is
	* not possible. When this state is set, it is a good idea to make sure
	* that the state of the object becomes such that any attempt to
	* operate on it causes a visible error of some kind.
	*/
	public int DEAD = 1;

	/**
	* Fail state indicating that the object is not dead, but is temporarily
	* not accessible.
	*/
	public int DOWN = 2;

	/**
	* Returns the state of this object. Must be one of the constants
	* defined in interface {@link Fallible}.
	*/
	public int getFailState();
	
	/**
	* Sets the fails state of this object. Parameter must be one of the
	* constants defined in interface {@link Fallible}.
	*/
	public void setFailState(int failState);

	/**
	* Convenience method to check if the node is up and running
	* @return must return true if and only if
	* <code>getFailState()==OK</code>
	*/
	public boolean isUp();
}


