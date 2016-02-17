/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/********************* Files and Storage handling ****************************
 * This example implements all main storage and file functions of the MSG API
 *
 * Scenario :
 * - display information on the disks mounted by the current host
 * - create a 200,000 bytes file
 * - completely read the created file
 * - write 100,000 bytes in the file
 * - rename the created file
 * - attach some user data to a disk
 * - dump disk's contents
 *
******************************************************************************/

package io;

import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;
import org.simgrid.msg.Process;
import org.simgrid.msg.Storage;
import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.MsgException;

public class Client extends Process {
  public Client(Host host, int number) throws HostNotFoundException {
    super(host, Integer.toString(number), null);
  }

  public void main(String[] args) throws MsgException {
   // Retrieve all mount points of current host
    Storage[] storages = getHost().getMountedStorage();

    for (int i = 0; i < storages.length; i++) {
      // For each disk mounted on host
      Msg.info("------------------------------------");
      Msg.info("Disk name: "+storages[i].getName());
      Msg.info("Size: "+storages[i].getSize()+" bytes.");
      Msg.info("Free Size: "+storages[i].getFreeSize()+" bytes.");
      Msg.info("Used Size: "+storages[i].getUsedSize()+" bytes.");
    }

    Storage st = Storage.getByName("Disk2");
    Msg.info("Disk name: "+st.getName());
    Msg.info("Attached to host:"+st.getHost());

    st.setProperty("key","Pierre");
    Msg.info("Property key: "+st.getProperty("key"));

    Host h = Host.currentHost();
    h.setProperty("key2","Pierre");
    Msg.info("Property key2: "+h.getProperty("key2"));

    String[] attach = h.getAttachedStorage();
    for (int j = 0; j < attach.length; j++) {
      Msg.info("Disk attached: "+attach[j]);
    }

    Msg.info("**************** ALL *************************");
    Storage[] stos = Storage.all();
    for (int i = 0; i < stos.length; i++) {
      Msg.info("Disk: "+ stos[i].getName());
    }
  }
}