/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package dht.chord;

import org.simgrid.msg.Msg;
import org.simgrid.msg.Comm;
import org.simgrid.msg.Host;
import org.simgrid.msg.Task;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.TimeoutException;
public class Node extends Process {
  protected int id;
  protected String mailbox;
  protected int predId;
  protected String predMailbox;
  protected int nextFingerToFix;
  protected Comm commReceive;
  ///Last time I changed a finger or my predecessor
  protected double lastChangeDate;
  private int[] fingers;

  public Node(Host host, String name, String[] args) {
    super(host,name,args);
  }

  @Override
  public void main(String[] args) throws MsgException {
    if (args.length != 2 && args.length != 4) {
      Msg.info("You need to provide 2 or 4 arguments.");
      return;
    }
    double initTime = Msg.getClock();
    int i;
    boolean joinSuccess;
    double deadline;

    double nextStabilizeDate = initTime + Common.PERIODIC_STABILIZE_DELAY;
    double nextFixFingersDate = initTime + Common.PERIODIC_FIX_FINGERS_DELAY;
    double nextCheckPredecessorDate = initTime + Common.PERIODIC_CHECK_PREDECESSOR_DELAY;
    double nextLookupDate = initTime + Common.PERIODIC_LOOKUP_DELAY;

    mailbox = args[0];
    id = Integer.parseInt(args[0]);

    fingers = new int[Common.NB_BITS];
    for (i = 0; i < Common.NB_BITS; i++) {
      fingers[i] = -1;
      setFinger(i,this.id);
    }

    //First node
    if (args.length == 2) {
      deadline = Integer.parseInt(args[1]);
      create();
      joinSuccess = true;
    } else {
      int knownId = Integer.parseInt(args[1]);
      deadline = Integer.parseInt(args[3]);
      Msg.debug("Hey! Let's join the system with the id " + id + ".");

      joinSuccess = join(knownId);
    }

    if (!joinSuccess) {
      Msg.info("I couldn't join the ring");
      return;
    }

    double currentClock = Msg.getClock();
    while (currentClock < (initTime + deadline) && currentClock < Common.MAX_SIMULATION_TIME) {
      if (commReceive == null) {
        commReceive = Task.irecv(this.mailbox);
      }
      try {
        if (!commReceive.test()) {
          if (currentClock >= nextStabilizeDate) {
            stabilize();
            nextStabilizeDate = Msg.getClock() + Common.PERIODIC_STABILIZE_DELAY;
          } else if (currentClock >= nextFixFingersDate) {
            fixFingers();
            nextFixFingersDate = Msg.getClock() + Common.PERIODIC_FIX_FINGERS_DELAY;
          } else if (currentClock >= nextCheckPredecessorDate) {
            this.checkPredecessor();
            nextCheckPredecessorDate = Msg.getClock() + Common.PERIODIC_CHECK_PREDECESSOR_DELAY;
          } else if (currentClock >= nextLookupDate) {
            this.randomLookup();
            nextLookupDate = Msg.getClock() + Common.PERIODIC_LOOKUP_DELAY;
          } else {
            waitFor(5);
          }
          currentClock = Msg.getClock();
        } else {
          handleTask(commReceive.getTask());
          currentClock = Msg.getClock();
          commReceive = null;
        }
      }
      catch (Exception e) {
        currentClock = Msg.getClock();
        commReceive = null;
      }
    }
    leave();
    if (commReceive != null) {
      commReceive = null;
    }
  }

  private void handleTask(Task task) {
    if (task instanceof FindSuccessorTask) {
      FindSuccessorTask fTask = (FindSuccessorTask)task;
      Msg.debug("Receiving a 'Find Successor' request from " + fTask.getIssuerHostName() + " for id " +
                fTask.getRequestId());
      // is my successor the successor?
      if (isInInterval(fTask.getRequestId(), this.id + 1, fingers[0])) {
        Msg.debug("Send the request to " + fTask.getAnswerTo() + " with answer " + fingers[0]);
        FindSuccessorAnswerTask answer = new FindSuccessorAnswerTask(getHost().getName(), mailbox, fingers[0]);
        answer.dsend(fTask.getAnswerTo());
      } else {
        // otherwise, forward the request to the closest preceding finger in my table
        int closest = closestPrecedingNode(fTask.getRequestId());
        Msg.debug("Forward the request to " + closest);
        fTask.dsend(Integer.toString(closest));
      }
    } else if (task instanceof GetPredecessorTask) {
      GetPredecessorTask gTask = (GetPredecessorTask)(task);
      Msg.debug("Receiving a 'Get Predecessor' request from " + gTask.getIssuerHostName());
      GetPredecessorAnswerTask answer = new GetPredecessorAnswerTask(getHost().getName(), mailbox, predId);
      answer.dsend(gTask.getAnswerTo());
    } else if (task instanceof NotifyTask) {
      NotifyTask nTask = (NotifyTask)task;
      notify(nTask.getRequestId());
    } else {
      Msg.debug("Ignoring unexpected task of type:" + task);
    }
  }

  private void leave() {
    Msg.debug("Well Guys! I Think it's time for me to quit ;)");
    // TODO: Notify my successor and predecessor.
  }

  /** @brief Initializes the current node as the first one of the system  */
  private void create() {
    Msg.debug("Create a new Chord ring...");
    setPredecessor(-1);
  }

  // Makes the current node join the ring, knowing the id of a node already in the ring
  private boolean join(int knownId) {
    Msg.info("Joining the ring with id " + this.id + " knowing node " + knownId);
    setPredecessor(-1);
    int successorId = remoteFindSuccessor(knownId, this.id);
    if (successorId == -1) {
      Msg.info("Cannot join the ring.");
    } else {
      setFinger(0, successorId);
    }
    return successorId != -1;
  }

  private void setPredecessor(int predecessorId) {
    if (predecessorId != predId) {
      predId = predecessorId;
      if (predecessorId != -1) {
        predMailbox = Integer.toString(predId);
      }
      lastChangeDate = Msg.getClock();
    }
  }

  /**
   * @brief Asks another node its predecessor.
   * @param askTo the node to ask to
   * @return the id of its predecessor node, or -1 if the request failed(or if the node does not know its predecessor)
   */
  private int remoteGetPredecessor(int askTo) {
    int predecessorId = -1;
    boolean stop = false;
    Msg.debug("Sending a 'Get Predecessor' request to " + askTo);
    String mailboxTo = Integer.toString(askTo);
    GetPredecessorTask sendTask = new GetPredecessorTask(getHost().getName(), this.mailbox);
    try {
      sendTask.send(mailboxTo, Common.TIMEOUT);
      do {
        if (commReceive == null) {
          commReceive = Task.irecv(this.mailbox);
        }
        commReceive.waitCompletion(Common.TIMEOUT);
        Task taskReceived = commReceive.getTask();
        if (taskReceived instanceof GetPredecessorAnswerTask) {
          predecessorId = ((GetPredecessorAnswerTask) taskReceived).getAnswerId();
          stop = true;
        } else {
          handleTask(taskReceived);
        }
        commReceive = null;
      } while (!stop);
    }
    catch (MsgException e) {
      Msg.debug("Failed to send the Get Predecessor request");
    }
    commReceive = null;

    return predecessorId;
  }

  /**
   * @brief Makes the current node find the successor node of an id.
   * @param node the current node
   * @param id the id to find
   * @return the id of the successor node, or -1 if the request failed
   */
  private int findSuccessor(int id) {
    if (isInInterval(id, this.id + 1, fingers[0])) {
      return fingers[0];
    }

    int closest = this.closestPrecedingNode(id);
    return remoteFindSuccessor(closest, id);
  }

  // Asks another node the successor node of an id.
  private int remoteFindSuccessor(int askTo, int id) {
    int successor = -1;
    boolean stop = false;
    String askToMailbox = Integer.toString(askTo);
    Task sendTask = new FindSuccessorTask(getHost().getName(), this.mailbox, id);
    Msg.debug("Sending a 'Find Successor' request to " + askToMailbox + " for id " + id);
    try {
      sendTask.send(askToMailbox, Common.TIMEOUT);
      do {
        if (commReceive == null) {
          commReceive = Task.irecv(this.mailbox);
        }
        commReceive.waitCompletion(Common.TIMEOUT);
        Task task = commReceive.getTask();
        if (task instanceof FindSuccessorAnswerTask) {
          //TODO: Check if this this our answer.
          FindSuccessorAnswerTask fTask = (FindSuccessorAnswerTask) task;
          stop = true;
          successor = fTask.getAnswerId();
        } else {
          handleTask(task);
        }
        commReceive = null;
      } while (!stop);
    }
    catch (TimeoutException e) {
      Msg.debug("Failed to send the 'Find Successor' request");
    }
    catch (MsgException e) {
      Msg.debug("Failed to receive Find Successor");
    }
    commReceive = null;

    return successor;
  }

  // This function is called periodically. It checks the immediate successor of the current node.
  private void stabilize() {
    Msg.debug("Stabilizing node");
    int candidateId;
    int successorId = fingers[0];
    if (successorId != this.id){
      candidateId = remoteGetPredecessor(successorId);
    } else {
      candidateId = predId;
    }
    //This node is a candidate to become my new successor
    if (candidateId != -1 && isInInterval(candidateId, this.id + 1, successorId - 1)) {
      setFinger(0, candidateId);
    }
    if (successorId != this.id) {
      remoteNotify(successorId, this.id);
    }
  }

  /**
   * @brief Notifies the current node that its predecessor may have changed.
   * @param candidate_id the possible new predecessor
   */
  private void notify(int predecessorCandidateId) {
    if (predId == -1 || isInInterval(predecessorCandidateId, predId + 1, this.id - 1 )) {
      setPredecessor(predecessorCandidateId);
    }
  }

  /**
   * @brief Notifies a remote node that its predecessor may have changed.
   * @param notify_id id of the node to notify
   * @param candidate_id the possible new predecessor
   */
  private void remoteNotify(int notifyId, int predecessorCandidateId) {
    Msg.debug("Sending a 'Notify' request to " + notifyId);
    Task sentTask = new NotifyTask(getHost().getName(), this.mailbox, predecessorCandidateId);
    sentTask.dsend(Integer.toString(notifyId));
  }

  // This function is called periodically.
  // It refreshes the finger table of the current node.
  private void fixFingers() {
    Msg.debug("Fixing fingers");
    int successorId = findSuccessor(id + (1 << nextFingerToFix));
    if (successorId != -1) {
      if (successorId != fingers[nextFingerToFix]) {
        setFinger(nextFingerToFix, successorId);
      }
      nextFingerToFix = (nextFingerToFix + 1) % Common.NB_BITS;
    }
  }

  // This function is called periodically.
  // It checks whether the predecessor has failed
  private void checkPredecessor() {
    //TODO
  }

  // Performs a find successor request to a random id.
  private void randomLookup() {
    int dest = 1337;
    findSuccessor(dest);
  }

  /**
   * @brief Returns the closest preceding finger of an id with respect to the finger table of the current node.
   * @param id the id to find
   * @return the closest preceding finger of that id
   */
  private int closestPrecedingNode(int id) {
    for (int i = Common.NB_BITS - 1; i >= 0; i--) {
      if (isInInterval(fingers[i], this.id + 1, id - 1)) {
        return fingers[i];
      }
    }
    return this.id;
  }

  /**
   * @brief Returns whether an id belongs to the interval [start, end].
   *
   * The parameters are noramlized to make sure they are between 0 and nb_keys - 1).
   * 1 belongs to [62, 3]
   * 1 does not belong to [3, 62]
   * 63 belongs to [62, 3]
   * 63 does not belong to [3, 62]
   * 24 belongs to [21, 29]
   * 24 does not belong to [29, 21]
   *
   * @param id id to check
   * @param start lower bound
   * @param end upper bound
   * @return a non-zero value if id in in [start, end]
   */
  private static boolean isInInterval(int id, int start, int end) {
    int normId = normalize(id);
    int normStart = normalize(start);
    int normEnd = normalize(end);

    // make sure end >= start and id >= start
    if (normEnd < normStart) {
      normEnd += Common.NB_KEYS;
    }
    if (normId < normStart) {
      normId += Common.NB_KEYS;
    }
    return (normId <= normEnd);
  }

  /**
   * @brief Turns an id into an equivalent id in [0, nb_keys).
   * @param id an id
   * @return the corresponding normalized id
   */
  private static int normalize(int id) {
    return id & (Common.NB_KEYS - 1);
  }

  /**
   * @brief Sets a finger of the current node.
   * @param finger_index index of the finger to set (0 to nb_bits - 1)
   * @param id the id to set for this finger
   */
  private void setFinger(int fingerIndex, int id) {
    if (id != fingers[fingerIndex]) {
      fingers[fingerIndex] = id;
      lastChangeDate = Msg.getClock();
    }
  }
}
