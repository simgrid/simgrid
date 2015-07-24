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

import java.io.*;

/**
 * A subclass of PrintStream whose methods ignore the content 
 * being written. 
 * 
 * @author Alberto Montresor
 * @version $Revision: 1.1 $
 */
public class NullPrintStream extends PrintStream
{

/**
 * Creates a null print stream that does not print anything.
 */
public NullPrintStream()
{
	super(System.out);
}

/**
 * This methods does not print anything.
 */
public synchronized void write(byte[] b, int off, int len)
{
}

/**
 * This methods does not print anything.
 */
public synchronized void write(int b)
{
}

/**
 * This methods does not print anything.
 */
private void printLine()
{
}

}
