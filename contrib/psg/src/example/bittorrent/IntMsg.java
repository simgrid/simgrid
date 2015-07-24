/*
 * Copyright (c) 2007-2008 Fabrizio Frioli, Michele Pedrolli
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
 * --
 *
 * Please send your questions/suggestions to:
 * {fabrizio.frioli, michele.pedrolli} at studenti dot unitn dot it
 *
 */

package example.bittorrent;

import peersim.core.*;
import psgsim.Sizable;

/**
 * This class is a {@link SimpleMsg} and acts as a container for a message that
 * uses only an integer value.
 */
public class IntMsg extends SimpleMsg implements Sizable {

	/**
	 * The data value (an integer) contained in the message.
	 */
	private int integer;
	private double size;

	/**
	 * The basic constructor of the message.
	 *
	 * @param type
	 *            the type of the message
	 * @param sender
	 *            The sender node
	 * @param value
	 *            The data value of the message
	 */
	public IntMsg(int type, Node sender, int value, double size) {
		super.type = type;
		super.sender = sender;
		this.integer = value;
		this.size = size;
	}

	/**
	 * Gets the value contained in the message.
	 *
	 * @return the integer value contained in the message
	 */
	public int getInt() {
		return this.integer;
	}

	@Override
	public double getSize() {
		// TODO Auto-generated method stub
		return size;
	}
}