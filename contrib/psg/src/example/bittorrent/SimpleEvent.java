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

/**
 * This class defines a simple event. A simple event is characterized only
 * by its type.
 */
public class SimpleEvent {
	
	/**
	* The identifier of the type of the event.
	* <p>
	* The available identifiers for event type are:<br/>
	* <ul>
	*  <li>1 is KEEP_ALIVE message</li>
	*  <li>2 is CHOKE message</li>
	*  <li>3 is UNCHOKE message</li>
	*  <li>4 is INTERESTED message</li>
	*  <li>5 is NOT_INTERESTED message</li>
	*  <li>6 is HAVE message</li>
	*  <li>7 is BITFIELD message</li>
	*  <li>8 is REQUEST message</li>
	*  <li>9 is PIECE message</li>
	*  <li>10 is CANCEL message</li>
	*  <li>11 is TRACKER message</li>
	*  <li>12 is PEERSET message</li>
	*  <li>13 is CHOKE_TIME event</li>
	*  <li>14 is OPTUNCHK_TIME event</li>
	*  <li>15 is ANTISNUB_TIME event</li>
	*  <li>16 is CHECKALIVE_TIME event</li>
	*  <li>17 is TRACKERALIVE_TIME event</li>
	*  <li>18 is DOWNLOAD_COMPLETED event</li>
	*</ul></p>
	*/
	protected int type;
	
	public SimpleEvent(){
	}
	
	/**
     * Initializes the type of the event.
	 * @param type The identifier of the type of the event
	 */
	public SimpleEvent(int type){
		this.type = type;
	}
	
	/**
	 * Gets the type of the event.
	 * @return The type of the current event.
	 */
	public int getType(){
		return this.type;	
	}
}


