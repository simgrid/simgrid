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

/**
 * This interface must be implemented by protocols that need to release
 * some references when the fail state of their host node is set to
 * {@link Fallible#DEAD}. Recall that this fail state means that the node
 * will never get back online. However, other nodes and protocols might
 * still have references to these dead nodes and protocols, and this fact
 * is not always a bug. So implementing this interface allows for removing
 * stuff that we know is no longer necessary. Especially for linkable
 * protocols in the presence of churn, this is essential: they typically
 * have to release their links to other nodes to prevent the formation of
 * trees of removed nodes with a live reference to the root.
 */
public interface Cleanable
{

/**
 * Performs cleanup when removed from the network. This is called by the
 * host node when its fail state is set to {@link Fallible#DEAD}.
 * It is very important that after calling this method, NONE of the methods
 * of the implementing object are guaranteed to work any longer.
 * They might throw arbitrary exceptions, etc. The idea is that after
 * calling this, typically no one should access this object.
 * However, as a recommendation, at least toString should be guaranteed to
 * execute normally, to aid debugging.
 */
public void onKill();

}
