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

import peersim.config.*;

// XXX a quite primitive scheduler, should be able to be configured
// much more flexibly using a simple syntax for time ranges.
/**
* A binary function over the time points. That is,
* for each time point returns a boolean value.
* 
* The concept of time depends on the simulation model. Current time
* has to be set by the simulation engine, irrespective of the model,
* and can be read using {@link CommonState#getTime()}. This scheduler
* is interpreted over those time points.
*
* <p>In this simple implementation the valid times will be
* <tt>from, from+step, from+2*step, etc,</tt>
* where the last element is strictly less than <tt>until</tt>.
* Alternatively, if <tt>at</tt> is defined, then the schedule will be a single
* time point. If FINAL is
* defined, it is also added to the set of active time points.
* It refers to the time after the simulation has finished (see
* {@link CommonState#getPhase}).
*/
public class Scheduler {


// ========================= fields =================================
// ==================================================================


/**
* Defaults to 1.
* @config
*/
private static final String PAR_STEP = "step";

/** 
* Defaults to -1. That is, defaults to be ineffective.
* @config
*/
private static final String PAR_AT = "at";


/** 
* Defaults to 0.
* @config
*/
private static final String PAR_FROM = "from";

/** 
* Defaults to <tt>Long.MAX_VALUE</tt>.
* @config
*/
private static final String PAR_UNTIL = "until";

/**
* Defines if component is active after the simulation has finished.
* Note that the exact time the simulation finishes is not know in advance
* because other components can stop the simulation at any time.
* By default not set.
* @see CommonState#getPhase
* @config
*/
private static final String PAR_FINAL = "FINAL";

public final long step;

public final long from;

public final long until;

public final boolean fin;

/** The next scheduled time point.*/
protected long next = -1;

// ==================== initialization ==============================
// ==================================================================


/** Reads configuration parameters from the component defined by
* <code>prefix</code>. {@value #PAR_STEP} defaults to 1.
*/
public Scheduler(String prefix) {
	
	this(prefix, true);
}

// ------------------------------------------------------------------

/** Reads configuration parameters from the component defined by
* <code>prefix</code>. If useDefault is false, then at least parameter
* {@value #PAR_STEP} must be explicitly defined. Otherwise {@value #PAR_STEP}
* defaults to 1.
*/
public Scheduler(String prefix, boolean useDefault)
{
	if (Configuration.contains(prefix+"."+PAR_AT)) {
		// FROM, UNTIL, and STEP should *not* be defined
		if (Configuration.contains(prefix+"."+PAR_FROM) ||
				Configuration.contains(prefix+"."+PAR_UNTIL) ||
				Configuration.contains(prefix+"."+PAR_STEP))
			throw new IllegalParameterException(prefix,
				"Cannot use \""+PAR_AT+"\" together with \""
				+PAR_FROM+"\", \""+PAR_UNTIL+"\", or \""+
				PAR_STEP+"\"");

		from = Configuration.getLong(prefix+"."+PAR_AT);
		until = from+1;
		step = 1;
	} else {
		if (useDefault) 
			step = Configuration.getLong(prefix+"."+PAR_STEP,1);
		else
			step = Configuration.getLong(prefix+"."+PAR_STEP);
		if( step < 1 )
			throw new IllegalParameterException(prefix,
				"\""+PAR_STEP+"\" must be >= 1");
		
		from = Configuration.getLong(prefix+"."+PAR_FROM,0);
		until = Configuration.getLong(prefix+"."+PAR_UNTIL,Long.MAX_VALUE);
	}

	if( from < until ) next = from;
	else next = -1;
	
	fin = Configuration.contains(prefix+"."+PAR_FINAL);
}


// ===================== public methods ==============================
// ===================================================================

/** true if given time point is covered by this scheduler */
public boolean active(long time) {
	
	if( time < from || time >= until ) return false;
	return (time - from)%step == 0; 
}

// -------------------------------------------------------------------

/** true if current time point is covered by this scheduler */
public boolean active() {
	
	return active( CommonState.getTime() );
}

//-------------------------------------------------------------------

/**
* Returns the next time point. If the returned value is negative, there are
* no more time points. As a side effect, it also updates the next time point,
* so repeated calls to this method return the scheduled times.
*/
public long getNext()
{
	long ret = next;
	// check like this to prevent integer overflow of "next"
	if( until-next > step ) next += step;
	else next = -1;
	return ret;
}

}


