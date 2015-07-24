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

/**
 * This class defines a simple message.
 * A simple message has its type and the reference of the sender node.
 * @see SimpleEvent
 */
public class SimpleMsg extends SimpleEvent {
	
	/**
	* The sender of the message.
	*/
	protected Node sender;
	
	public SimpleMsg(){
	}
	
	/**
	 * Initializes the simple message with its type and sender.
	 * @param type The identifier of the type of the message
	 * @param sender The sender of the message
	 */
	public SimpleMsg(int type, Node sender){
		super.type = type;
		this.sender = sender;
	}
	
	/**
     * Gets the sender of the message.
	 * @return The sender of the message.
	 */
	public Node getSender(){
		return this.sender;	
	}
}
