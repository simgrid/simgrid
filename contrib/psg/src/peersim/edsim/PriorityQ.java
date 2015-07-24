/*
 * Copyright (c)2008 The Peersim Team
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

package peersim.edsim;

import peersim.core.Node;

/**
 * The interface to be implemented by the event queue of the evend based
 * engine. An implementation must also provide the standard cosntructor
 * required by any peersim components: one that takes a String argument,
 * the component name in the configuration.
 */
public interface PriorityQ {


/**
 * Returns the current number of events in the queue.
 */
public int size();

/**
 * Add a new event, to be scheduled at the specified time. If there are other
 * events scheduled at the same time, then the time of execution if this event
 * relative to the other events is unspecified.
 * 
 * @param time The time at which this event should be scheduled. It is
 * guaranteed to be non-negative (so no extra checks are needed)
 * @param event the object describing the event
 * @param node the node at which the event has to be delivered
 * @param pid the protocol that handles the event
 */
public void add(long time, Object event, Node node, byte pid);

/**
 * Add a new event, to be scheduled at the specified time, specifying also
 * the priority of the event, should there be other events scheduled at the
 * same time. If both time and priority is the same for an event, then the
 * scheduling order is unspecified.
 * 
 * @param time The time at which this event should be scheduled. It is
 * guaranteed to be non-negative (so no extra checks are needed)
 * @param event the object describing the event
 * @param node the node at which the event has to be delivered
 * @param pid the protocol that handles the event
 * @param priority if for two events the "time" value is the same, this
 * value should be used to order them. Lower value means higher priority.
 * Like with time, non-negativity as assumed.
 */
public void add(long time, Object event, Node node, byte pid, long priority);

/**
 * Removes the first event in the heap and returns it.
 * The returned object is not guaranteed to be a freshly generated object,
 * that is, we allow for an implementation that keeps one copy of an event
 * object and always returns a reference to that copy.
 * @return first event or null if size is zero
 */
public Event removeFirst();

/**
* Maximal value of time this interpretation can represent.
*/
public long maxTime();

/**
* Maximal value of priority this interpretation can deal with. That is,
* the number of different priority levels is <tt>maxPriority()+1</tt> because
* 0 is also a valid level.
* @see #add(long,Object,Node,byte,long)
*/
public long maxPriority();

/**
 * Return type of {@link #removeFirst()}.
 */
public class Event
{
	public Object event;
	public long time;
	public Node node;
	public byte pid;
	public String toString() {
		return event+" to node "+node+"prot "+pid+"at "+time; }
}

}
