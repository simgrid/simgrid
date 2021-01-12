/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package async.yield;
import org.simgrid.msg.Host;
import org.simgrid.msg.Msg;
import org.simgrid.msg.Process;

public class Yielder extends Process {
  public Yielder (Host host, String name, String[] args) {
    super(host, name, args);
  }

  @Override
  public void main(String[] args) {
    int yieldsCount = Integer.parseInt(args[0]);
    for (int i=0; i<yieldsCount; i++)
        Process.yield();
    Msg.info("Yielded "+yieldsCount+". Good bye now!");
  }
}
