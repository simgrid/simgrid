/* Copyright (c) 2012-2021. The SimGrid Team. All rights reserved.          */

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
import org.simgrid.msg.MsgException;

public class Node extends Process {
  private static String file1 = "/doc/simgrid/examples/platforms/g5k.xml";
  private static String file2 = "/include/surf/simgrid_dtd.h";
  private static String file3 = "/doc/simgrid/examples/platforms/g5k_cabinets.xml";
  private static String file4 = "/doc/simgrid/examples/platforms/nancy.xml";

  protected int rank;

  public Node(Host host, int number) {
    super(host, Integer.toString(number), null);
    this.rank = number;
  }

  public void main(String[] args) throws MsgException {
    String mount = "/home";
    String fileName;
    switch (rank) {
      case 4:
        fileName = mount + file1;
      break;
      case 0:
        mount = "/tmp";
        fileName = mount + file2;
      break;
      case 2:
        fileName = mount + file3;
      break;
      case 1:
        fileName = mount + file4;
      break;
      default:
        fileName = mount + file1;
      break;
    }

    Msg.info("Open file " + fileName);
    File file = new File(fileName);

    long read = file.read(10000,1);
    Msg.info("Having read " + read + " on " + fileName);

    long write = file.write(100000,1);
    Msg.info("Having write " + write + " on " + fileName);

    Msg.info("Seek back to the beginning of " + fileName);
    file.seek(0,File.SEEK_SET);

    read = file.read(150000,1);
    Msg.info("Having read " + read + " on " + fileName);  
  }
}
