/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package master_slave_bypass;

import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.NativeException;

public class MsBypass {

   /* This only contains the launcher. If you do nothing more than than you can run
    *   java simgrid.msg.Msg
    * which also contains such a launcher
    */

    public static void main(String[] args) throws NativeException,HostNotFoundException {

    /* initialize the MSG simulation. Must be done before anything else (even logging). */
    Msg.init(args);
    Msg.createEnvironment(args[0]);

    /* bypass deployment */
    new Master("Boivin","process1").start();

	/*  execute the simulation. */
    Msg.run();
    }
}
