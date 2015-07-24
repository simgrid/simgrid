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
* A class that can keep track of some statistics like variance, average, min,
* max incrementally. That is, when adding a new data item, it updates the
* statistics.
*/
public class IncrementalStats {


// ===================== fields ========================================
// =====================================================================


private double min;

private double max;

private double sum;

private double sqrsum;

private int n;

private int countmin;

private int countmax;

// ====================== initialization ==============================
// ====================================================================


/** Calls {@link #reset}. */
public IncrementalStats() { reset(); }

// --------------------------------------------------------------------

/** Resets the statistics to reflect the zero elements set.
* Min and max are set to positive and negative infinity, respectively.
*/
public void reset() {
	
	countmin=0;
	countmax=0;
	min = Double.POSITIVE_INFINITY;
	max = Double.NEGATIVE_INFINITY;
	sum = 0.0;
	sqrsum = 0.0;
	n = 0;
}


// ======================== methods ===================================
// ====================================================================


/** Updates the statistics according to this element. It calls
* <code>add(item,1)</code>.
* @see #add(double,int) */
public final void add( double item ) { add(item,1); }

// --------------------------------------------------------------------

/** Updates the statistics assuming element <code>item</code> is added
* <code>k</code> times.*/
public void add( double item, int k ) {
	
	if( item < min )
	{
		min = item;
		countmin = 0;
	} 
	if( item == min ) countmin+=k;
	if( item > max )
	{
		max = item;
		countmax = 0;
	}
	if(item == max) countmax+=k;  
	n+=k;
	if( k == 1 )
	{
		sum += item;
		sqrsum += item*item;
	}
	else
	{
		sum += item*k;
		sqrsum += item*item*k;
	}
}

// --------------------------------------------------------------------

/** The number of data items processed so far */
public int getN() { return n; }

// --------------------------------------------------------------------

/** The maximum of the data items */
public double getMax() { return max; }

// --------------------------------------------------------------------

/** The minimum of the data items */
public double getMin() { return min; }

// --------------------------------------------------------------------

/** Returns the number of data items whose value equals the maximum. */
public int getMaxCount() { return countmax; }

// --------------------------------------------------------------------

/** Returns the number of data items whose value equals the minimum. */
public int getMinCount() { return countmin; }

// --------------------------------------------------------------------

/** The sum of the data items */
public double getSum() { return sum; }

// --------------------------------------------------------------------

/** The sum of the squares of the data items */
public double getSqrSum() { return sqrsum; }

// --------------------------------------------------------------------

/** The average of the data items */
public double getAverage() { return sum/n; }

// --------------------------------------------------------------------

/** The empirical variance of the data items. Guaranteed to be larger or
equal to 0.0. If due to rounding errors the value becomes negative,
it returns 0.0.*/
public double getVar() {

	double var=
		(((double)n) / (n-1)) * (sqrsum/n - getAverage()*getAverage());
	return (var>=0.0?var:0.0);
	// XXX note that we have very little possibility to increase numeric
	// stability if this class is "greedy", ie, if it has no memory
	// In a more precise implementation we could delay the calculation of
	// statistics and store the data in some intelligent structure
}

// --------------------------------------------------------------------

/** the empirical standard deviation of the data items */
public double getStD() { return Math.sqrt(getVar()); }

// --------------------------------------------------------------------

/**
* Prints the following quantities separated by spaces in a single line
* in this order.
* Minimum, maximum, number of items, average, variance, number of minimal
* items, number of maximal items.
*/
public String toString() {

	return min+" "+max+" "+n+" "+sum/n+" "+getVar()+" "+
		countmin+" "+countmax;
}

}

