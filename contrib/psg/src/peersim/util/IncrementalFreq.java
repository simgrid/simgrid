/*
 * Copyright (c) 2003-2005 The BISON Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
		
package peersim.util;

import java.io.PrintStream;

//XXX This implementation is very restricted, to be made more flexible
// using hashtables.
/**
* A class that can collect frequency information on integer input.
* right now it can handle only unsigned input. It simply ignores negative
* numbers.
*/
public class IncrementalFreq implements Cloneable {


// ===================== fields ========================================
// =====================================================================

/** The number of items inserted. */
private int n;

/** freq[i] holds the frequency of i. primitive implementation, to be changed */
private int[] freq = null; 

/**
* The capacity, if larger than 0. Added values larger than or equal to
* this one will be ignored.
*/
private final int N;


// ====================== initialization ==============================
// ====================================================================


/**
* @param maxvalue Values in the input set larger than this one will be ignored.
* However, if it is negative, no values are ignored.
*/
public IncrementalFreq(int maxvalue) {
	
	N = maxvalue+1;
	reset();
}

// --------------------------------------------------------------------

/** Calls <code>this(-1)</code>, that is, no values will be ignored.
* @see #IncrementalFreq(int) */
public IncrementalFreq() {
	
	this(-1);
}

// --------------------------------------------------------------------

/** Reset the state of the object. After calling this, all public methods
* behave the same as they did after constructing the object.
*/
public void reset() {

	if( freq==null || N==0 ) freq = new int[0];
	else for(int i=0; i<freq.length; ++i) freq[i]=0;
	n = 0;
}


// ======================== methods ===================================
// ====================================================================

/**
 * Adds item <code>i</code> to the input set.
 * It calls <code>add(i,1)</code>.
 * @see #add(int,int)
 */
public final void add( int i ) { add(i,1); }


// --------------------------------------------------------------------

/**
 * Adds item <code>i</code> to the input set <code>k</code> times.
 * That is, it increments counter <code>i</code> by <code>k</code>.
 * If, however, <code>i</code> is negative, or larger than the maximum defined
 * at construction time (if a maximum was set at all) the operation is ignored.
 */
public void add( int i, int k ) {
  
  if( N>0 && i>=N ) return;
  if( i<0 || k<=0 ) return;

  // Increase number of items by k.
  n+=k;

  // If index i is out of bounds for the current array of counters,
  // increase the size of the array to i+1.
  if( i>=freq.length )
  {
    int tmp[] = new int[i+1];
    System.arraycopy(freq, 0, tmp, 0, freq.length);
    freq=tmp;
  }

  // Finally, increase counter i by k.
  freq[i]+=k;
}

// --------------------------------------------------------------------

/** Returns number of processed data items.
* This is the number of items over which the class holds statistics.
*/
public int getN() { return n; }

// --------------------------------------------------------------------

/** Returns the number of occurrences of the given integer. */
public int getFreq(int i) {
	
	if( i>=0 && i<freq.length ) return freq[i];
	else return 0;
}

// --------------------------------------------------------------------
	

/**
 * Performs an element-by-element vector subtraction of the
 * frequency vectors. If <code>strict</code> is true, it
 * throws an IllegalArgumentException if <code>this</code> is
 * not strictly larger than <code>other</code> (element by element)
 * (Note that both frequency vectors are positive.)
 * Otherwise just sets those elements in <code>this</code> to zero
 * that are smaller than those of <code>other</code>.
 * @param other The instance of IncrementalFreq to subtract
 * @param strict See above explanation
 */
public void remove(IncrementalFreq other, boolean strict) {

	// check if other has non-zero elements in non-overlapping part
	if(strict && other.freq.length>freq.length)
	{
		for(int i=other.freq.length-1; i>=freq.length; --i)
		{
			if (other.freq[i]!=0)
				throw new IllegalArgumentException();
		}
	}
	
	final int minLength = Math.min(other.freq.length, freq.length);
	for (int i=minLength-1; i>=0; i--)
	{
		if (strict && freq[i] < other.freq[i])
			throw new IllegalArgumentException();
		final int remove = Math.min(other.freq[i], freq[i]);
		n -= remove;
		freq[i] -= remove;
	}
}

// ---------------------------------------------------------------------

/**
* Prints current frequency information. Prints a separate line for
* all values from 0 to the capacity of the internal representation using the
* format
* <pre>
* value occurrences
* </pre>
* That is, numbers with zero occurrences will also be printed. 
*/
public void printAll( PrintStream out ) {
	
	for(int i=0; i<freq.length; ++i)
	{
		out.println(i+" "+freq[i]);
	}
}

// ---------------------------------------------------------------------

/**
* Prints current frequency information. Prints a separate line for
* all values that have a number of occurrences different from zero using the 
* format
* <pre>
* value occurrences
* </pre>
*/
public void print( PrintStream out ) {

	for(int i=0; i<freq.length; ++i)
	{
		if(freq[i]!=0) out.println(i+" "+freq[i]);
	}
}

// ---------------------------------------------------------------------

public String toString() {
	
	StringBuilder result=new StringBuilder("");
	for(int i=0; i<freq.length; ++i)
	{
		if (freq[i] != 0)
			result.append(" (").append(i).append(","
			).append(freq[i]).append(")");
	}
	return result.toString();
}

// ---------------------------------------------------------------------

/** An alternative method to convert the object to String */
public String toArithmeticExpression() {

	StringBuilder result=new StringBuilder("");
	for (int i=freq.length-1; i>=0; i--)
	{
		if (freq[i] != 0)
			result.append(freq[i]).append(
			"*").append(i).append("+");
	}
	
	if (result.length()==0)
		return "(empty)";
	else
		return result.substring(0, result.length()-1);
}

// ---------------------------------------------------------------------

public Object clone() throws CloneNotSupportedException {

	IncrementalFreq result = (IncrementalFreq)super.clone();
	if( freq != null ) result.freq = freq.clone();
	return result;
}

// ---------------------------------------------------------------------

/**
* Tests equality between two IncrementalFreq instances.
* Two objects are equal if both hold the same set of numbers that have
* occurred non-zero times and the number of occurrences is also equal for
* these numbers.
*/
public boolean equals(Object obj) {

	if( !( obj instanceof IncrementalFreq) ) return false;
	IncrementalFreq other = (IncrementalFreq)obj;
	final int minlength = Math.min(other.freq.length, freq.length);
	
	for (int i=minlength-1; i>=0; i--)
		if (freq[i] != other.freq[i])
			return false;

	if( freq.length > minlength ) other=this;
	for (int i=minlength; i<other.freq.length; i++)
		if( other.freq[i] != 0 )
			return false;

	return true;
}

// ---------------------------------------------------------------------

/**
* Hashcode (consistent with {@link #equals}). Probably you will never want to
* use this, but since we have {@link #equals}, we must implement it.
*/
public int hashCode() {

	int sum = 0;
	for(int i=0; i<freq.length; ++i) sum += freq[i]*i;
	return sum;
}

// ---------------------------------------------------------------------

/*
public static void main(String[] pars) {
	
	IncrementalFreq ifq = new IncrementalFreq(Integer.parseInt(pars[0]));
	for(int i=1; i<pars.length; ++i)
	{
		ifq.add(Integer.parseInt(pars[i]));
	}
	ifq.print(System.out);
	System.out.println(ifq);
}
*/
}


