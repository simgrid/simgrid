/*
 * Copyright (c) 2001 The Anthill Team
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
import peersim.core.CommonState;
import peersim.config.Configuration;
import peersim.config.IllegalParameterException;

/**
 *  The Heap data structure used to maintain events "sorted" by 
 *  scheduled time and to obtain the next event to be executed.
 *  
 *  @author Alberto Montresor
 *  @version $Revision: 1.10 $
 */
public class Heap implements PriorityQ {

//--------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------

/** 
 * This parameter specifies how many
 * bits are used to order events that occur at the same time. Defaults
 * to 8. A value smaller than 8 causes an IllegalParameterException.
 * Higher values allow for a better discrimination, but reduce
 * the maximal time steps that can be simulated.
 * @config 
 */	
private static final String PAR_PBITS = "pbits";
private static final String PAR_PBITS_LEGACY = "simulation.timebits";

/** 
 * Specifies the initial capacity of the heap. Defaults to 65536.
 * @config 
 */	
private static final String PAR_SIZE = "size";


//--------------------------------------------------------------------------
// Fields
//--------------------------------------------------------------------------

// The following arrays are four heaps ordered by time. The alternative
// approach (i.e. to store event objects) requires much more memory,
// and based on some tests that I've done is not really much faster.

/** Event component of the heap */
private Object[] events;

/** Time component of the heap */
private long[] times;

/** Node component of the heap */
private Node[] nodes;

/** Pid component of the heap */
private byte[] pids;

/** Number of elements */
private int size;

/** Singleton event object used to return (event, time, node, pid) tuples */
private final Event ev = new Event();

/** The number of bits reserved to order event with the same timestamp */
private final int pbits;

/** The mask to test whether the time value fits into the range we can
represent */
private final long overflowMask;

//--------------------------------------------------------------------------
// Contructor
//--------------------------------------------------------------------------

/**
 * Initializes a new heap using defaults.
 */
public Heap() {
	this(""); // "" is not a valid prefix for a component
}

//--------------------------------------------------------------------------

/**
 * Initializes a new heap using the configuration.
 */
public Heap(String prefix) {

	int size = Configuration.getInt(prefix+"."+PAR_SIZE,65536);
	
	// some complex stuff to deal with legacy parameter names...
	if( !Configuration.contains(PAR_PBITS_LEGACY) )
		pbits = Configuration.getInt(prefix+"."+PAR_PBITS,8);
	else
	{
		pbits = Configuration.getInt(PAR_PBITS_LEGACY);
		if( Configuration.contains(prefix+"."+PAR_PBITS) )
			throw new IllegalParameterException(PAR_PBITS_LEGACY,
				"Your configuration file contains both "+
				prefix+"."+PAR_PBITS+ " and "+
				PAR_PBITS_LEGACY+"; please remove "+
				PAR_PBITS_LEGACY);
	}

	if (pbits < 8 || pbits >= 31) {
		throw new IllegalParameterException(prefix+"."+PAR_PBITS,
		"This parameter should be >= 8 or < 31");
	}
	overflowMask = ~maxTime();
	events = new Object[size];
	times = new long[size];
	nodes = new Node[size];
	pids = new byte[size];
}

//--------------------------------------------------------------------------
// Methods
//--------------------------------------------------------------------------

/**
 * Returns the current number of events in the system.
 */
public int size()
{
	return size;
}

//--------------------------------------------------------------------------

/**
 * Add a new event, to be scheduled at the specified time.
 * 
 * @param time the time at which this event should be scheduled
 * @param event the object describing the event
 * @param node the node at which the event has to be delivered
 * @param pid the protocol that handles the event
 */
public void add(long time, Object event, Node node, byte pid) 
{
	add(time,event,node,pid,CommonState.r.nextInt(1 << pbits));
}

//--------------------------------------------------------------------------

/**
 * Add a new event, to be scheduled at the specified time.
 * 
 * @param time the time at which this event should be scheduled
 * @param event the object describing the event
 * @param node the node at which the event has to be delivered
 * @param pid the protocol that handles the event
 */
public void add(long time, Object event, Node node, byte pid, long priority) 
{
	if( (time&overflowMask) != 0 ) throw new
		IllegalArgumentException("Time overflow: time="+time);
//XXX should we test priority overflow? How much does it cost?
	
	time = (time << pbits) | priority;
	
	size++;
	int pos = size;
	put(pos, time, event, node, pid);
	while (pos > 1 && getTime(pos / 2) > time) {
		swap(pos, pos / 2);
		pos = pos / 2;
	}
}

//--------------------------------------------------------------------------

/**
 * Removes the first event in the heap and returns it.
 * Note that, to avoid garbage collection, a singleton instance of
 * the Event class is used. This means that data contained in the
 * returned event are overwritten when a new invocation of this
 * method is performed.
 * @return first event or null if size is zero
 */
public Event removeFirst() {
	
	if(size==0) return null;

	ev.time = times[0] >> pbits;
	ev.event = events[0];
	ev.node = nodes[0];
	ev.pid = pids[0];
	swap(1, size);
	size--;
	minHeapify(1);
	return ev;
}

//--------------------------------------------------------------------------

public long maxTime() { return Long.MAX_VALUE >> pbits; }

//--------------------------------------------------------------------------

public long maxPriority() { return (1L << pbits)-1; }

//--------------------------------------------------------------------------

/** 
 *  Prints the time values contained in the heap.
 */
public String toString()
{
	StringBuffer buffer = new StringBuffer();
	buffer.append("[Size: " + size + " Times: ");
	for (int i=1; i <= size; i++) {
		buffer.append(getTime(i)+",");
	}
	buffer.append("]");
	return buffer.toString();
}


//--------------------------------------------------------------------------
// Private methods
//--------------------------------------------------------------------------

/**
 * 
 */
private void minHeapify(int index) 
{
	// The time to be placed of the current node
	long time = getTime(index);  
	// Left, right children of the current index
	int l,r; 
	// Their associated time
	long lt, rt;
	// The minimum time between val, lt, rt
	long mintime;
	// The index of the mininum time
	int minindex = index; 
	do {
		index = minindex;
		mintime = time;
		l = index << 1;
		r = l + 1;
		if (l <= size && (lt = getTime(l)) < mintime) {
			minindex = l;
			mintime = lt;
		}
		if (r <= size && (rt = getTime(r)) < mintime) {
			minindex = r;
			mintime = rt;
		}
		if (minindex != index) {
			swap(minindex, index);
		}
	} while (minindex != index);
}

//--------------------------------------------------------------------------

/**
 * 
 */
private void swap(int i1, int i2) {
	
	i1--;
	i2--;
	
	Object te = events[i1];
	events[i1] = events[i2];
	events[i2] = te;
	
	long tt = times[i1];
	times[i1] = times[i2];
	times[i2] = tt;
	
	Node tn = nodes[i1];
	nodes[i1] = nodes[i2];
	nodes[i2] = tn;

	byte tp = pids[i1];
	pids[i1] = pids[i2];
	pids[i2] = tp;
}

//--------------------------------------------------------------------------

/**
 * 
 */
private long getTime(int index) {
	/* Compute first and second index, and return the value */
	index--;
	return times[index];
}

//--------------------------------------------------------------------------

/**
 * 
 */
private void put(int index, long time, Object event, Node node, byte pid) {
	
	index--;
	if (index >= events.length) {
		doubleCapacity();
	}
	events[index] = event;
	times[index] = time;
	nodes[index] = node;
	pids[index] = pid;
}

//--------------------------------------------------------------------------

/**
 * 
 */
private void doubleCapacity() {
	int oldsize = events.length;
	int newsize = oldsize*2;
	Object[] te = new Object[newsize];
	System.arraycopy(events, 0, te, 0, oldsize);
	events = te;
	long[] tt = new long[newsize];
	System.arraycopy(times, 0, tt, 0, oldsize);
	times = tt;
	Node[] tn = new Node[newsize];
	System.arraycopy(nodes, 0, tn, 0, oldsize);
	nodes = tn;
	byte[] tp = new byte[newsize];
	System.arraycopy(pids, 0, tp, 0, oldsize);
	pids = tp;
}

//--------------------------------------------------------------------------
// Testing
//--------------------------------------------------------------------------

/*
public static void main(String[] args) {
	Random random = new Random();
	Heap heap = new Heap();
	int rep = 1000000;
	if( args.length > 0 ) rep = Integer.parseInt(args[0]);
	int[] values1 = new int[rep];
	long[] values2 = new long[rep];
	for (int i = 0; i < rep; i++) {
		values1[i] = random.nextInt(1000000000); 
	}
	
	long time1 = System.currentTimeMillis();
	for (int i = 0; i < rep; i++) {
		heap.add(values1[i], null, null, (byte) 1);
	}
	long time2 = System.currentTimeMillis();
	System.out.println("Inserting: " + (time2-time1));
	
	time1 = System.currentTimeMillis();
	for (int i = 0; i < rep; i++) {
		values2[i] = heap.removeFirst().time;
	}
	time2 = System.currentTimeMillis();
	System.out.println("Removing: " + (time2-time1));
	
	Arrays.sort(values1);
	for (int i=0; i<rep; i++) {
		if (values1[i] != values2[i])
			System.out.print("+");
	}
}
*/

} // END Heap
