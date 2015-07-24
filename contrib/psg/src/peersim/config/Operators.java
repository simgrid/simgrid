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
package peersim.config;
import java.math.*;
import org.lsmp.djep.groupJep.groups.*;
import org.lsmp.djep.groupJep.interfaces.*;

/**
 * This class implements the <code>Group</code> interface of JEP,
 * enabling the configuration system to read integers with arbitrary 
 * length. 
 */
public class Operators extends Group implements IntegralDomainI,HasDivI,
	OrderedSetI,HasModI,HasPowerI {
	

	/**
	 * Operations on the reals (Implemented as BigInteger).
	 */
	public Operators() {
	}

	public Number getZERO() {
		return BigInteger.ZERO;
	}

	public Number getONE() {
		return BigInteger.ONE;
	}

	public Number getInverse(Number num) {
		if (num instanceof BigInteger) {
			BigInteger a = (BigInteger) num;
			return a.negate();
		} else {
			return -num.doubleValue();
		}
	}

	public Number add(Number num1, Number num2) {
		if (num1 instanceof Double || num2 instanceof Double) {
			// One is double
			return num1.doubleValue() + num2.doubleValue();
		} else {
			// Both integer
			BigInteger a = (BigInteger) num1;
			BigInteger b = (BigInteger) num2;
			return a.add(b);
		}
	}

	public Number sub(Number num1, Number num2) {
		if (num1 instanceof Double || num2 instanceof Double) {
			// One is double
			return num1.doubleValue() - num2.doubleValue();
		} else {
			// Both integer
			BigInteger a = (BigInteger) num1;
			BigInteger b = (BigInteger) num2;
			return a.subtract(b);
		}
	}

	public Number mul(Number num1, Number num2) {
		if (num1 instanceof Double || num2 instanceof Double) {
			// One is double
			return num1.doubleValue() * num2.doubleValue();
		} else {
			// Both integer
			BigInteger a = (BigInteger) num1;
			BigInteger b = (BigInteger) num2;
			return a.multiply(b);
		}
	}

	public Number div(Number num1, Number num2) {
		if (num1 instanceof Double || num2 instanceof Double) {
			// One is double
			return num1.doubleValue() / num2.doubleValue();
		} else {
			// Both integer
			BigInteger a = (BigInteger) num1;
			BigInteger b = (BigInteger) num2;
			return a.divide(b);
		}
	}

	 
	public Number mod(Number num1, Number num2) {
		if (num1 instanceof Double || num2 instanceof Double) {
			// One is double
			return num1.doubleValue() % num2.doubleValue();
		} else {
			// Both integer
			BigInteger a = (BigInteger) num1;
			BigInteger b = (BigInteger) num2;
			return a.remainder(b);
		}
	}
	
	public Number pow(Number num1, Number num2) {
		if (num1 instanceof Double || num2 instanceof Double) {
			// One is double
			return Math.pow(num1.doubleValue(), num2.doubleValue());
		} else {
			// Both integer
			BigInteger a = (BigInteger) num1;
			BigInteger b = (BigInteger) num2;
			return a.pow(b.intValue());
		}
	}
	
	public boolean equals(Number num1, Number num2)	{
		if (num1 instanceof Double || num2 instanceof Double) {
			// One is double
			return num1.doubleValue() == num2.doubleValue();
		} else {
			// Both integer
			BigInteger a = (BigInteger) num1;
			BigInteger b = (BigInteger) num2;
			return a.equals(b);
		}
	}
	
	public int compare(Number num1,Number num2)	{
		if (num1 instanceof Double || num2 instanceof Double) {
			// One is double
			double n1 = num1.doubleValue();
			double n2 = num2.doubleValue();
			return (n1 < n2 ? -1 : (n1 == n2 ? 0 : 1));
		} else {
			// Both integer
			BigInteger a = (BigInteger) num1;
			BigInteger b = (BigInteger) num2;
			return a.compareTo(b);
		}
	}

	public Number valueOf(String str) {
		try {
			return new BigInteger(str);
		} catch (NumberFormatException e) {
			return new Double(str);
		}
	}
	
	public String toString() { return ""; }
}
