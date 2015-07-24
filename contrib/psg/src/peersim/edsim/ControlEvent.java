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

package peersim.edsim;

import peersim.core.Control;
import peersim.core.Scheduler;


/**
 * Wrapper for {@link Control}s to be executed in an event driven simulation.
 *
 * @author Alberto Montresor
 * @version $Revision: 1.5 $
 */
class ControlEvent
{

//---------------------------------------------------------------------
//Fields
//---------------------------------------------------------------------

/** 
 * The reference to the dynamics to be executed; null if this cycle event
 * refers to an observer.
 */
private Control control;

/** Order index used to maintain order between cycle-based events */
private int order;


//---------------------------------------------------------------------
//Initialization
//---------------------------------------------------------------------

/** 
 * Scheduler object to obtain the next schedule time of this event 
 */
private Scheduler scheduler;

/**
 * Creates a cycle event for a control object. It also schedules the object
 * for the first execution adding it to the priority queue of the event driven
 * simulation.
 */
public ControlEvent(Control control, Scheduler scheduler, int order)
{
	this.control = control;
	this.order = order;
	this.scheduler = scheduler;
	long next = scheduler.getNext();
	if( next>=0 ) EDSimulator.addControlEvent(next, order, this);
}

//---------------------------------------------------------------------
//Methods
//---------------------------------------------------------------------

/**
* Executes the control object, and schedules the object for the next execution
* adding it to the priority queue of the event driven simulation.
*/
public boolean execute() {

	boolean ret = control.execute();
	long next = scheduler.getNext();
	if( next>=0 ) EDSimulator.addControlEvent(next, order, this);
	return ret;
}

}


