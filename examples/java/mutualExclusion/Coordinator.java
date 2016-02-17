/* Copyright (c) 2012-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package mutualExclusion;
import java.util.LinkedList;

import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;
import org.simgrid.msg.Task;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;

public class Coordinator extends Process {
  public Coordinator(Host host, String name, String[]args) {
    super(host,name,args);
  }
  LinkedList<RequestTask> waitingQueue=new LinkedList<RequestTask>();
  int CsToServe;

  public void main(String[] args) throws MsgException {
    CsToServe = Integer.parseInt(args[0]);
    Task task;
    while (CsToServe >0) {
      task = Task.receive("coordinator");
      if (task instanceof RequestTask) {
        RequestTask t = (RequestTask) task;
        if (waitingQueue.isEmpty()) {
          Msg.info("Got a request from "+t.from+". Queue empty: grant it");
          GrantTask tosend =  new GrantTask();
          tosend.send(t.from);
        } else {
          waitingQueue.addFirst(t);
        }
      } else if (task instanceof ReleaseTask) {
        if (!waitingQueue.isEmpty()) {
          RequestTask req = waitingQueue.removeLast();
          GrantTask tosend = new GrantTask();
          tosend.send(req.from);
        }
        CsToServe--;
        if (waitingQueue.isEmpty() && CsToServe==0) {
          Msg.info("we should shutdown the simulation now");
        }
      }
    }
  }
}
