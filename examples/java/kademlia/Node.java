/* Copyright (c) 2012-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package kademlia;

import org.simgrid.msg.Host;

import org.simgrid.msg.Msg;
import org.simgrid.msg.Comm;
import org.simgrid.msg.Task;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;

public class Node extends Process {
  protected int id;
  protected RoutingTable table;
  protected int deadline;
  protected int findNodeSuccedded = 0;
  protected int findNodeFailed = 0;
  protected Comm comm;

  public Node(Host host, String name, String[]args) {
    super(host,name,args);
  }

  @Override
  public void main(String[] args) throws MsgException {
    //Check the number of arguments.
    if (args.length != 2 && args.length != 3) {
      Msg.info("Wrong argument count.");
      return;
    }
    this.id = Integer.valueOf(args[0]);
    this.table = new RoutingTable(this.id);

    if (args.length == 3) {
      this.deadline = Integer.valueOf(args[2]).intValue();
      Msg.info("Hi, I'm going to join the network with the id " + id + "!");
      if (joinNetwork(Integer.valueOf(args[1]))) {
        this.mainLoop();
      }
      else {
        Msg.info("I couldn't join the network :(");
      }
    }
    else {
      this.deadline = Integer.valueOf(args[1]).intValue();
      Msg.info("Hi, I'm going to create the network with the id " + id + "!");
      table.update(this.id);
      this.mainLoop();
    }
    Msg.debug("I'm leaving the network");
    Msg.debug("Here is my routing table:" + table);
  }

  public void mainLoop() {
    double next_lookup_time = Msg.getClock() + Common.RANDOM_LOOKUP_INTERVAL;
    while (Msg.getClock() < this.deadline) {
      try {
        if (comm == null) {
          comm = Task.irecv(Integer.toString(id));
        }
        if (!comm.test()) {
          if (Msg.getClock() >= next_lookup_time) {
            randomLookup();
            next_lookup_time += Common.RANDOM_LOOKUP_INTERVAL;
          } else {
            waitFor(1);
          }
        } else {
          Task task = comm.getTask();
          handleTask(task);
          comm = null;
        }
      }
      catch (Exception e) {
      }
    }
    Msg.info(findNodeSuccedded + "/"  + (findNodeSuccedded + findNodeFailed) + " FIND_NODE have succedded.");
  }

  /**
   * @brief Try to make the node join the network
   * @param idKnown Id of someone we know in the system
   */
  public boolean joinNetwork(int idKnown) {
    boolean answerGot = false;
    double timeBegin = Msg.getClock();
    Msg.debug("Joining the network knowing " + idKnown);
    //Add ourselves and the node we know to our routing table
    table.update(this.id);
    table.update(idKnown);
    //Send a "FIND_NODE" to the node we know.
    sendFindNode(idKnown,this.id);
    //Wait for the answer.
    int trials = 0;

    do {
      try {
        if (comm == null) {
          comm = Task.irecv(Integer.toString(id));
        }
        if (comm != null) {
          if (!comm.test()) {
            waitFor(1);
          } else {
            Task task = comm.getTask();
            if (task instanceof FindNodeAnswerTask) {
              answerGot = true;
              //Retrieve the node list and ping them
              FindNodeAnswerTask answerTask = (FindNodeAnswerTask)task;
              Answer answer = answerTask.getAnswer();
              answerGot = true;
              //answersGotten++;
              if (answer.getDestinationId() == this.id) {
                //Ping everyone in the list
                for (Contact c : answer.getNodes()) {
                  table.update(c.getId());
                }
              }
            } else {
              handleTask(task);
            }
            comm = null;
          }
        }
      }
      catch (Exception ex) {
        trials++;
        Msg.info("FIND_NODE failed");
      }
    } while (!answerGot && trials < Common.MAX_JOIN_TRIALS);
    /* Second step: Send a FIND_NODE in a node in each bucket */
    int bucketId = table.findBucket(idKnown).getId();
    for (int i = 0; ((bucketId - i) > 0 || 
       (bucketId + i) <= Common.IDENTIFIER_SIZE) && 
       i < Common.JOIN_BUCKETS_QUERIES; i++) {
      if (bucketId - i > 0) {
        int idInBucket = table.getIdInPrefix(this.id,bucketId - i);
        this.findNode(idInBucket,false);
      }
      if (bucketId + i <= Common.IDENTIFIER_SIZE) {
        int idInBucket = table.getIdInPrefix(this.id,bucketId + i);
        findNode(idInBucket,false);
      }
    }
    Msg.debug("Time spent:" + (Msg.getClock() - timeBegin));
    return answerGot;
  }

  /* Send a request to find a node in the node's routing table. */
  public boolean findNode(int destination, boolean counts) {
    int queries, answers;
    int nodesAdded = 0;
    boolean destinationFound = false;
    int steps = 0;
    double timeBeginReceive;
    double timeout, globalTimeout = Msg.getClock() + Common.FIND_NODE_GLOBAL_TIMEOUT;
    //Build a list of the closest nodes we already know.
    Answer nodeList = table.findClosest(destination);
    Msg.verb("Doing a FIND_NODE on " + destination);
    do {
      timeBeginReceive = Msg.getClock();
      answers = 0;
      queries = this.sendFindNodeToBest(nodeList);
      nodesAdded = 0;
      timeout = Msg.getClock() + Common.FIND_NODE_TIMEOUT;
      steps++;
      do {
        try {
          if (comm == null) {
            comm = Task.irecv(Integer.toString(id));
          }
          if (!comm.test()) {
            waitFor(1);
          } else {
            Task task = comm.getTask();  
            if (task instanceof FindNodeAnswerTask) {
              FindNodeAnswerTask answerTask = (FindNodeAnswerTask)task;
              //Check if we received what we are looking for.
              if (answerTask.getDestinationId() == destination) {
                table.update(answerTask.getSenderId());
                //Add the answer to our routing table
                for (Contact c: answerTask.getAnswer().getNodes()) {
                  table.update(c.getId());
                }
                answers++;
                
                nodesAdded = nodeList.merge(answerTask.getAnswer());
              } else {
              /* If it's not our answer, we answer to the node that has queried us anyway */
                handleTask(task);
                //Update the timeout if it's not our answer.
                timeout += Msg.getClock() - timeBeginReceive;
                timeBeginReceive = Msg.getClock();
              }
            } else {
              handleTask(task);
              timeout += Msg.getClock() - timeBeginReceive;
              timeBeginReceive = Msg.getClock();
            }
            comm = null;
          }
        }
        catch (Exception e) {
          comm = null;
        }
      } while (answers < queries && Msg.getClock() < timeout);
      destinationFound = nodeList.destinationFound();
    } while (!destinationFound && (nodesAdded > 0 || answers == 0) && Msg.getClock() < globalTimeout 
             && steps < Common.MAX_STEPS);

    if (destinationFound) {
      if (counts) {
        findNodeSuccedded++;
      }
      Msg.debug("Find node on " + destination + " succedded");
    } else {
      Msg.debug("Find node on " + destination + " failed");
      Msg.debug("Queried " + queries + " nodes to find "  + destination);
      Msg.debug(nodeList.toString());
      if (counts) {
        findNodeFailed++;
      }
    }
    return destinationFound;
  }

  /**
   * @brief Sends a "PING" request to a node
   * @param destination Ping destination id.
   */
  public void ping(int destination) {
    boolean destinationFound = false;
    double timeout = Msg.getClock() + Common.PING_TIMEOUT;
    PingTask pingTask = new PingTask(this.id);
    /* Sending the ping task */
    pingTask.dsend(Integer.toString(destination));
    do {
      try {
        Task task = Task.receive(Integer.toString(this.id),Common.PING_TIMEOUT);
        if (task instanceof PingAnswerTask) {
          PingAnswerTask answerTask = (PingAnswerTask)task;
          if (answerTask.getSenderId() == destination) {
            this.table.update(destination);
            destinationFound = true;
          } else {
            handleTask(task);
          }
        } else {
          handleTask(task);
        }
        waitFor(1);
      }
      catch (Exception ex) {
      }
    } while (Msg.getClock() < timeout && !destinationFound);
  }

  /**
   * @brief Sends a "FIND_NODE" request (task) to a node we know.
   * @param id Id of the node we are querying
   * @param destination id of the node we are trying to find.
   */
  public void sendFindNode(int id, int destination) {
    Msg.debug("Sending a FIND_NODE to " + Integer.toString(id) + " to find " + destination  );
    FindNodeTask task = new FindNodeTask(this.id,destination);
    task.dsend(Integer.toString(id));
  }

  /** Sends a "FIND_NODE" request to the best "alpha" nodes in a node list */
  public int sendFindNodeToBest(Answer nodeList) {
    int destination = nodeList.getDestinationId();
    int i;
    for (i = 0; i < Common.alpha && i < nodeList.size(); i++) {
      Contact node = nodeList.getNodes().get(i);
      if (node.getId() != this.id) {
        this.sendFindNode(node.getId(),destination);
      }
    }
    return i;
  }

  public void randomLookup() {
    findNode(0,true);
  }

  /**
   * @brief Handles an incomming task
   * @param task The task we need to handle
   */
  public void handleTask(Task task) {
    if (task instanceof KademliaTask) {
      table.update(((KademliaTask) task).getSenderId());
      if (task instanceof FindNodeTask) {
        handleFindNode((FindNodeTask)task);
      }
      else if (task instanceof PingTask) {
        handlePing((PingTask)task);
      }
    }
  }

  public void handleFindNode(FindNodeTask task) {
    Msg.debug("Received a FIND_NODE from " + task.getSenderId());
    Answer answer = table.findClosest(task.getDestination());
    FindNodeAnswerTask taskToSend = new FindNodeAnswerTask(this.id,task.getDestination(),answer);
    taskToSend.dsend(Integer.toString(task.getSenderId()));
  }

  public void handlePing(PingTask task) {
    Msg.debug("Received a PING from " + task.getSenderId());
    PingAnswerTask taskToSend = new PingAnswerTask(this.id);
    taskToSend.dsend(Integer.toString(task.getSenderId()));
  }
}
