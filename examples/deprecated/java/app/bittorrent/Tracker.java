/* Copyright (c) 2006-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package app.bittorrent;
import java.util.ArrayList;
import java.util.Random;

import org.simgrid.msg.Msg;
import org.simgrid.msg.Comm;
import org.simgrid.msg.Host;
import org.simgrid.msg.Task;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;

public class Tracker extends Process {
  Random rand = new Random();
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
    //Retrieve the end time
    deadline = Double.parseDouble(args[0]);
    //Building peers array
    peersList = new ArrayList<>();

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
                nextPeer = rand.nextInt(peersList.size());
              } while (tTask.peers.contains(peersList.get(nextPeer)));
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
