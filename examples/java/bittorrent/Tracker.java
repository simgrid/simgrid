/* Copyright (c) 2006-2014, 2026. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package bittorrent;
import java.util.ArrayList;

import org.simgrid.msg.Msg;
import org.simgrid.msg.Comm;
import org.simgrid.msg.Host;
import org.simgrid.msg.Task;
import org.simgrid.msg.Process;
import org.simgrid.msg.RngStream;
import org.simgrid.msg.MsgException;

public class Tracker extends Process {
  protected RngStream stream;
  protected ArrayList<Integer> peersList;
  protected double deadline;
  protected Comm commReceived = null;

  public Tracker(Host host, String name, String[]args) {
    super(host,name,args);
  }

  @Override
  public void main(String[] args) throws MsgException {
    if (args.length != 1) {
      Msg.info("Wrong number of arguments for the tracker.");
      return;
    }
    //Build the RngStream object for randomness
    stream = new RngStream("tracker");
    //Retrieve the end time
    deadline = Double.valueOf(args[0]);
    //Building peers array
    peersList = new ArrayList<Integer>();

    Msg.info("Tracker launched.");    
    while (Msg.getClock() < deadline) {
      if (commReceived == null) {
        commReceived = Task.irecv(Common.TRACKER_MAILBOX);
      }
      try {
        if (commReceived.test()) {
          Task task = commReceived.getTask();
          if (task instanceof TrackerTask) {
            TrackerTask tTask = (TrackerTask)task;
            //Sending peers to the peer
            int nbPeers = 0;
            while (nbPeers < Common.MAXIMUM_PEERS && nbPeers < peersList.size()) {
              int nextPeer;
              do {
                nextPeer = stream.randInt(0, peersList.size() - 1);
              } while (tTask.peers.contains(nextPeer));
              tTask.peers.add(peersList.get(nextPeer));
              nbPeers++;
            }
            //Adding the peer to our list
            peersList.add(tTask.peerId);
            tTask.type = TrackerTask.Type.ANSWER;
            //Setting the interval
            tTask.interval = Common.TRACKER_QUERY_INTERVAL;
            //Sending the task back to the peer
            tTask.dsend(tTask.mailbox);
          }
          commReceived = null;
        } else {
          waitFor(1);
        }
      }
      catch (MsgException e) {
        commReceived = null;
      }
    }
    Msg.info("Tracker is leaving");
  }
}
