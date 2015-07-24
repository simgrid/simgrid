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
 *	This class is a {@link SimpleMsg} and represents the <tt>bitfield</tt>
 *	message.
 */
public class BitfieldMsg extends SimpleMsg{
	/**
	 *	The status of the file to transmit to neighbors nodes
	 */
	int[] array;
	
	/**
	 *	Defines the type of the Bitfield message. If <tt>isRequest</tt> is true, then
	 *	the message is a request of subscription; otherwise the message is a response.
	 */
	boolean isRequest;
	
	/**
	 *	<p>The ACK value used to implement <i>ack</i> and <i>nack</i> messages.</p>
	 *	<p>It has value <tt>true</tt> if the message is a reponse and the sender has inserted
	 *	the receiver in its own cache of neighbors.<br/>
	 *	If for some reason (for instance the cache had already 80 peer inside at the moment of the
	 *	request) it was not possible to insert the peer, the value is <tt>false</tt>.<br/>
	 *	It has value <tt>false</tt> also if the message is a request and is sent when occours
	 *	an unespected message.
	 *	</p>
	 *	@see "The documentation to understand the 4 different types of Bitfield messages"
	 */
	boolean ack;
	
	/**
	 *	The basic constructor of the Bitfield message.
	 *	@param type The type of the message, according to {@link SimpleMsg}
	 *	@param isRequest Defines if the message is a request or not
	 *	@param ack Defines if the message type is an <i>ack</i> or a <i>nack</i>
	 *	@param sender The sender node
	 *	@param source The array containing the status of the file
	 *	@param size The size of the array
	 */
	public BitfieldMsg(int type, boolean isRequest, boolean ack, Node sender, int source[], int size){
		super.type = type;
		super.sender = sender;
		this.isRequest = isRequest;
		this.ack = ack;
		this.array = new int[size];
		for(int i=0; i<size;i++){ // it sends a copy
			if(source[i]==16)
				this.array[i] = 1;
			else
				this.array[i] = 0;
		}
	}
	
	/**
	 *	Gets the array containing the status of the file.
	 *	@return The status of the file
	 */
	public int[] getArray(){
		return this.array;	
	}
}