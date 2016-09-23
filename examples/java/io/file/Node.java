/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/********************* Files and Storage handling ****************************
 * This example implements all main file functions of the MSG API
 *
 * Scenario: Each host
 * - opens a file
 * - tries to read 10,000 bytes in this file
 * - writes 100,000 bytes in the file
 * - seeks back to the beginning of the file
 * - tries to read 150,000 bytes in this file
 *
******************************************************************************/

package io.file;

import org.simgrid.msg.Msg;
import org.simgrid.msg.File;
import org.simgrid.msg.Host;
import org.simgrid.msg.Process;
import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.MsgException;

public class Node extends Process {
  private static String FILENAME1 = "/doc/simgrid/examples/platforms/g5k.xml";
  private static String FILENAME2 = "\\Windows\\setupact.log";
  private static String FILENAME3 = "/doc/simgrid/examples/platforms/g5k_cabinets.xml";
  private static String FILENAME4 = "/doc/simgrid/examples/platforms/nancy.xml";

  protected int number;

  public Node(Host host, int number) throws HostNotFoundException {
    super(host, Integer.toString(number), null);
    this.number = number;
  }

  public void main(String[] args) throws MsgException {
    String mount;
    String filename;
    switch (number) {
      case 0:
        mount = "/home";
        filename = mount + FILENAME1;
      break;
      case 1:
        mount = "c:";
        filename = mount + FILENAME2;
      break;
      case 2:
        mount = "/home";
        filename = mount + FILENAME3;
      break;
      case 3:
        mount = "/home";
        filename = mount + FILENAME4;
      break;
      default:
        mount = "/home";
        filename = mount + FILENAME1;
    }

    Msg.info("Open file " + filename);
    File file = new File(filename);

    long read = file.read(10000,1);
    Msg.info("Having read " + read + " on " + filename);

    long write = file.write(100000,1);
    Msg.info("Having write " + write + " on " + filename);

    Msg.info("Seek back to the beginning of " + filename);
    file.seek(0,file.SEEK_SET);

    read = file.read(150000,1);
    Msg.info("Having read " + read + " on " + filename);  
  }
}
