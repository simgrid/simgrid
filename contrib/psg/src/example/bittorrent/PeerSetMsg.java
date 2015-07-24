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
 *	This class is a {@link SimpleMsg} and represents the <tt>peerset</tt>
 *	message used by the tracker to send to the peers a list of neighbors.
 */
public class PeerSetMsg extends SimpleMsg{
	
	/**
	 *	The set of "friends" peers sent by the tracker to each node.
	 */
	private Neighbor[] peerSet;
	
	/**
	 *	Initializes a new <tt>peerset</tt> message.
	 *	@param type is the type of the message (it should be 12)
	 *	@param array is the array containing the references to the neighbor nodes
	 *	@param sender the sender node
	 *	@see SimpleEvent
	 */
	public PeerSetMsg(int type, Neighbor []array, Node sender){
		super.type = type;
		peerSet = array; // references to the effective nodes
		super.sender = sender;
	}
	
	/**
	 *	Gets the peer set.
	 *	@return the peer set, namely the set of neighbor nodes.
	 */
	public Neighbor[] getPeerSet(){
		return this.peerSet;	
	}
}
