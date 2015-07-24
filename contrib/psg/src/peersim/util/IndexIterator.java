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
		
package peersim.util;

/**
* This class provides iterations over the set of integers [0...k-1].
*/
public interface IndexIterator {

	/**
	* This resets the iteration. The set of integers will be 0,..,k-1.
	*/
	public void reset(int k);
	
	/**
	* Returns next index.
	*/
	public int next();

	/**
	* Returns true if {@link #next} can be called at least one more time.
	* Note that {@link #next} can be called k times after {@link #reset}.
	*/
	public boolean hasNext();
}

