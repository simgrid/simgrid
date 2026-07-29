/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import java.util.Random;
import org.simgrid.s4u.*;

/* ------------------------------------------------------------------------ */
/* Definitions shared by all the nodes.                                     */
/* ------------------------------------------------------------------------ */

enum MessageType {
  FIND_SUCCESSOR,
  FIND_SUCCESSOR_ANSWER,
  GET_PREDECESSOR,
  GET_PREDECESSOR_ANSWER,
  NOTIFY,
  SUCCESSOR_LEAVING,
  PREDECESSOR_LEAVING,
  PREDECESSOR_ALIVE,
  PREDECESSOR_ALIVE_ANSWER
}

class ChordMessage {
  public MessageType type;
  public String issuer_host_name;   // used for logging
  public int request_id     = -1;   // id (used by some types of messages)
  public int request_finger = 1;    // finger parameter (used by some types of messages)
  public int answer_id      = -1;   // answer (used by some types of messages)
  public Mailbox answer_to  = null; // mailbox to send an answer to (if any)

  public ChordMessage(MessageType type)
  {
    this.type             = type;
    this.issuer_host_name = Host.current().get_name();
  }
}

/* ------------------------------------------------------------------------ */
/* The node                                                                 */
/* ------------------------------------------------------------------------ */

class Node extends Actor {
  static int nb_bits_;
  static int nb_keys_;
  static int timeout_;

  int known_id_      = -1;
  double start_time_ = -1;
  double deadline_   = -1;
  boolean joined_    = false;
  int id_;                 // my id
  int pred_id_ = -1;       // predecessor id
  Random random_;          // random number generator for this node
  Mailbox mailbox_;        // my mailbox
  int[] fingers_;          // finger table, (fingers[0] is my successor)
  int next_finger_to_fix_; // index of the next finger to fix in fixFingers()

  /* Returns whether an id belongs to the interval [start, end].
   *
   * The parameters are normalized to make sure they are between 0 and nb_keys_ - 1).
   * 1 belongs to [62, 3]
   * 1 does not belong to [3, 62]
   * 63 belongs to [62, 3]
   * 63 does not belong to [3, 62]
   * 24 belongs to [21, 29]
   * 24 does not belong to [29, 21]
   */
  static boolean is_in_interval(int id, int start, int end)
  {
    int i = id % nb_keys_;
    int s = start % nb_keys_;
    int e = end % nb_keys_;

    // make sure end >= start and id >= start
    if (e < s)
      e += nb_keys_;
    if (i < s)
      i += nb_keys_;

    return i <= e;
  }

  static void set_parameters(int nb_bits, int nb_keys, int timeout)
  {
    nb_bits_ = nb_bits;
    nb_keys_ = nb_keys;
    timeout_ = timeout;
  }

  /* Initializes the current node as the first one of the system, or plans to join it later on. */
  public Node(String[] args)
  {
    if (args.length != 2 && args.length != 4)
      Engine.die("Wrong number of arguments for this node");

    // initialize my node
    id_ = Integer.parseInt(args[0]);
    Engine.debug("Initialize node with id: %d", id_);
    random_             = new Random(id_);
    mailbox_            = this.get_engine().mailbox_by_name(String.valueOf(id_));
    next_finger_to_fix_ = 0;
    fingers_            = new int[nb_bits_];
    for (int i = 0; i < nb_bits_; i++)
      fingers_[i] = id_;

    if (args.length == 2) { // first ring
      deadline_   = Double.parseDouble(args[1]);
      start_time_ = Engine.get_clock();
      Engine.debug("Create a new Chord ring...");
    } else {
      known_id_   = Integer.parseInt(args[1]);
      start_time_ = Double.parseDouble(args[2]);
      deadline_   = Double.parseDouble(args[3]);
      Engine.debug("Hey! Let's join the system in %f seconds (shall leave at time %f)", start_time_,
                   start_time_ + deadline_);
    }
  }

  /** Makes the current node join the ring, knowing the id of a node already in the ring */
  void join(int known_id)
  {
    Engine.info("Joining the ring with id %d, knowing node %d", id_, known_id);
    setPredecessor(-1); // no predecessor (yet)

    int successor_id = remoteFindSuccessor(known_id, id_);
    if (successor_id == -1) {
      Engine.info("Cannot join the ring.");
    } else {
      setFinger(0, successor_id);
      printFingerTable();
      joined_ = true;
    }
  }

  /** Makes the current node quit the system */
  void leave()
  {
    Engine.info("Well Guys! I Think it's time for me to leave ;)");
    notifyAndQuit();
    joined_ = false;
  }

  /** Notifies the successor and the predecessor of the current node before leaving */
  void notifyAndQuit()
  {
    // send the PREDECESSOR_LEAVING to our successor
    ChordMessage pred_msg = new ChordMessage(MessageType.PREDECESSOR_LEAVING);
    pred_msg.request_id   = pred_id_;
    pred_msg.answer_to    = mailbox_;

    Engine.debug("Sending a 'PREDECESSOR_LEAVING' to my successor %d", fingers_[0]);
    try {
      Comm comm = this.get_engine().mailbox_by_name(String.valueOf(fingers_[0])).put_async(pred_msg, 10);
      comm.await_for_or_cancel(timeout_);
    } catch (TimeoutException e) {
      Engine.debug("Timeout expired when sending a 'PREDECESSOR_LEAVING' to my successor %d", fingers_[0]);
    }

    if (pred_id_ != -1 && pred_id_ != id_) {
      // send the SUCCESSOR_LEAVING to our predecessor (only if I have one that is not me)
      ChordMessage succ_msg = new ChordMessage(MessageType.SUCCESSOR_LEAVING);
      succ_msg.request_id   = fingers_[0];
      succ_msg.answer_to    = mailbox_;
      Engine.debug("Sending a 'SUCCESSOR_LEAVING' to my predecessor %d", pred_id_);

      try {
        Comm comm = this.get_engine().mailbox_by_name(String.valueOf(pred_id_)).put_async(succ_msg, 10);
        comm.await_for_or_cancel(timeout_);
      } catch (TimeoutException e) {
        Engine.debug("Timeout expired when sending a 'SUCCESSOR_LEAVING' to my predecessor %d", pred_id_);
      }
    }
  }

  /** Performs a find successor request to a random id */
  void randomLookup()
  {
    int res          = id_;
    int random_index = random_.nextInt(nb_bits_);
    int random_id    = fingers_[random_index];
    Engine.debug("Making a lookup request for id %d", random_id);
    if (random_id != id_)
      res = findSuccessor(random_id);
    Engine.debug("The successor of node %d is %d", random_id, res);
  }

  /** Sets a finger of the current node. */
  void setFinger(int finger_index, int id)
  {
    if (id != fingers_[finger_index]) {
      fingers_[finger_index] = id;
      Engine.info("My new finger #%d is %d", finger_index, id);
    }
  }

  /** Sets the predecessor of the current node, or -1 to unset the predecessor */
  void setPredecessor(int predecessor_id)
  {
    if (predecessor_id != pred_id_) {
      pred_id_ = predecessor_id;
      Engine.info("My new predecessor is %d", predecessor_id);
    }
  }

  /** refreshes the finger table of the current node (called periodically) */
  void fixFingers()
  {
    Engine.debug("Fixing fingers");
    int id = findSuccessor(id_ + (1 << next_finger_to_fix_));
    if (id != -1) {
      if (id != fingers_[next_finger_to_fix_]) {
        setFinger(next_finger_to_fix_, id);
        printFingerTable();
      }
      next_finger_to_fix_ = (next_finger_to_fix_ + 1) % nb_bits_;
    }
  }

  /** Displays the finger table of a node. */
  void printFingerTable()
  {
    Engine.info("My finger table:");
    Engine.info("Start | Succ");
    for (int i = 0; i < nb_bits_; i++)
      Engine.info(" %3d  | %3d", (id_ + (1 << i)) % nb_keys_, fingers_[i]);
    Engine.info("Predecessor: %d", pred_id_);
  }

  /** checks whether the predecessor has failed (called periodically) */
  void checkPredecessor()
  {
    Engine.debug("Checking whether my predecessor is alive");
    if (pred_id_ == -1)
      return;

    Mailbox mailbox        = this.get_engine().mailbox_by_name(String.valueOf(pred_id_));
    Mailbox return_mailbox = this.get_engine().mailbox_by_name(id_ + "_is_alive");

    ChordMessage message = new ChordMessage(MessageType.PREDECESSOR_ALIVE);
    message.request_id   = pred_id_;
    message.answer_to    = return_mailbox;

    Engine.debug("Sending a 'Predecessor Alive' request to my predecessor %d", pred_id_);
    try {
      Comm comm = mailbox.put_async(message, 10);
      comm.await_for_or_cancel(timeout_);
    } catch (TimeoutException e) {
      Engine.debug("Failed to send the 'Predecessor Alive' request to %d", pred_id_);
      return;
    }

    // receive the answer
    Engine.debug("Sent 'Predecessor Alive' request to %d, waiting for the answer on my mailbox '%s'", pred_id_,
                 return_mailbox.get_name());
    Comm comm = return_mailbox.get_async();
    try {
      comm.await_for_or_cancel(timeout_);
      Engine.debug("Received the answer to my 'Predecessor Alive': my predecessor %d is alive", pred_id_);
    } catch (TimeoutException e) {
      Engine.debug("Failed to receive the answer to my 'Predecessor Alive' request");
      pred_id_ = -1;
    }
  }

  /** Asks its predecessor to a remote node. Returns -1 if the request failed. */
  int remoteGetPredecessor(int ask_to)
  {
    int predecessor_id     = -1;
    Mailbox mailbox        = this.get_engine().mailbox_by_name(String.valueOf(ask_to));
    Mailbox return_mailbox = this.get_engine().mailbox_by_name(id_ + "_pred");

    ChordMessage message = new ChordMessage(MessageType.GET_PREDECESSOR);
    message.request_id   = id_;
    message.answer_to    = return_mailbox;

    // send a "Get Predecessor" request to ask_to_id
    Engine.debug("Sending a 'Get Predecessor' request to %d", ask_to);
    try {
      Comm comm = mailbox.put_async(message, 10);
      comm.await_for_or_cancel(timeout_);
    } catch (TimeoutException e) {
      Engine.debug("Failed to send the 'Get Predecessor' request to %d", ask_to);
      return predecessor_id;
    }

    // receive the answer
    Engine.debug("Sent 'Get Predecessor' request to %d, waiting for the answer on my mailbox '%s'", ask_to,
                 return_mailbox.get_name());
    Comm comm = return_mailbox.get_async();
    try {
      comm.await_for_or_cancel(timeout_);
      ChordMessage answer = (ChordMessage)comm.get_payload();
      Engine.debug("Received the answer to my 'Get Predecessor' request: the predecessor of node %d is %d", ask_to,
                   answer.answer_id);
      predecessor_id = answer.answer_id;
    } catch (TimeoutException e) {
      Engine.debug("Failed to receive the answer to my 'Get Predecessor' request");
    }

    return predecessor_id;
  }

  /** Returns the closest preceding finger of an id with respect to the finger table of the current node. */
  int closestPrecedingFinger(int id)
  {
    for (int i = nb_bits_ - 1; i >= 0; i--)
      if (is_in_interval(fingers_[i], id_ + 1, id - 1))
        return fingers_[i];
    return id_;
  }

  /** Makes the current node find the successor node of an id. Returns -1 if the request failed. */
  int findSuccessor(int id)
  {
    // is my successor the successor?
    if (is_in_interval(id, id_ + 1, fingers_[0]))
      return fingers_[0];

    // otherwise, ask the closest preceding finger in my table
    return remoteFindSuccessor(closestPrecedingFinger(id), id);
  }

  int remoteFindSuccessor(int ask_to, int id)
  {
    int successor          = -1;
    Mailbox mailbox        = this.get_engine().mailbox_by_name(String.valueOf(ask_to));
    Mailbox return_mailbox = this.get_engine().mailbox_by_name(id_ + "_succ");

    ChordMessage message = new ChordMessage(MessageType.FIND_SUCCESSOR);
    message.request_id   = id_;
    message.answer_to    = return_mailbox;

    // send a "Find Successor" request to ask_to_id
    Engine.debug("Sending a 'Find Successor' request to %d for id %d", ask_to, id);
    try {
      Comm comm = mailbox.put_async(message, 10);
      comm.await_for_or_cancel(timeout_);
    } catch (TimeoutException e) {
      Engine.debug("Failed to send the 'Find Successor' request to %d for id %d", ask_to, id_);
      return successor;
    }

    // receive the answer
    Engine.debug("Sent a 'Find Successor' request to %d for key %d, waiting for the answer", ask_to, id);
    Comm comm = return_mailbox.get_async();
    try {
      comm.await_for_or_cancel(timeout_);
      ChordMessage answer = (ChordMessage)comm.get_payload();
      Engine.debug("Received the answer to my 'Find Successor' request for id %d: the successor of key %d is %d",
                   answer.request_id, id_, answer.answer_id);
      successor = answer.answer_id;
    } catch (TimeoutException e) {
      Engine.debug("Failed to receive the answer to my 'Find Successor' request");
    }

    return successor;
  }

  /** Notifies the current node that its predecessor may have changed. */
  void notify(int predecessor_candidate_id)
  {
    if (pred_id_ == -1 || is_in_interval(predecessor_candidate_id, pred_id_ + 1, id_ - 1)) {
      setPredecessor(predecessor_candidate_id);
      printFingerTable();
    } else {
      Engine.debug("I don't have to change my predecessor to %d", predecessor_candidate_id);
    }
  }

  /** Notifies a remote node that its predecessor may have changed. */
  void remoteNotify(int notify_id, int predecessor_candidate_id)
  {
    ChordMessage message = new ChordMessage(MessageType.NOTIFY);
    message.request_id   = predecessor_candidate_id;
    message.answer_to    = null;

    // send a "Notify" request to notify_id
    Engine.debug("Sending a 'Notify' request to %d", notify_id);
    Mailbox mailbox = this.get_engine().mailbox_by_name(String.valueOf(notify_id));
    mailbox.put_init(message, 10).detach();
  }

  /** This function is called periodically. It checks the immediate successor of the current node. */
  void stabilize()
  {
    Engine.debug("Stabilizing node");

    // get the predecessor of my immediate successor
    int candidate_id = pred_id_;
    int successor_id = fingers_[0];
    if (successor_id != id_)
      candidate_id = remoteGetPredecessor(successor_id);

    // this node is a candidate to become my new successor
    if (candidate_id != -1 && is_in_interval(candidate_id, id_ + 1, successor_id - 1))
      setFinger(0, candidate_id);
    if (successor_id != id_)
      remoteNotify(successor_id, id_);
  }

  /** This function is called when a node receives a message. */
  void handleMessage(ChordMessage message)
  {
    switch (message.type) {
      case FIND_SUCCESSOR:
        Engine.debug("Received a 'Find Successor' request from %s for id %d", message.issuer_host_name,
                     message.request_id);
        // is my successor the successor?
        if (is_in_interval(message.request_id, id_ + 1, fingers_[0])) {
          message.type      = MessageType.FIND_SUCCESSOR_ANSWER;
          message.answer_id = fingers_[0];
          Engine.debug("Sending back a 'Find Successor Answer' to %s (mailbox %s): the successor of %d is %d",
                       message.issuer_host_name, message.answer_to.get_name(), message.request_id, message.answer_id);
          message.answer_to.put_init(message, 10).detach();
        } else {
          // otherwise, forward the request to the closest preceding finger in my table
          int closest = closestPrecedingFinger(message.request_id);
          Engine.debug("Forwarding the 'Find Successor' request for id %d to my closest preceding finger %d",
                       message.request_id, closest);
          Mailbox mailbox = this.get_engine().mailbox_by_name(String.valueOf(closest));
          mailbox.put_init(message, 10).detach();
        }
        break;

      case GET_PREDECESSOR:
        Engine.debug("Receiving a 'Get Predecessor' request from %s", message.issuer_host_name);
        message.type      = MessageType.GET_PREDECESSOR_ANSWER;
        message.answer_id = pred_id_;
        Engine.debug("Sending back a 'Get Predecessor Answer' to %s via mailbox '%s': my predecessor is %d",
                     message.issuer_host_name, message.answer_to.get_name(), message.answer_id);
        message.answer_to.put_init(message, 10).detach();
        break;

      case NOTIFY:
        // someone is telling me that he may be my new predecessor
        Engine.debug("Receiving a 'Notify' request from %s", message.issuer_host_name);
        notify(message.request_id);
        break;

      case PREDECESSOR_LEAVING:
        // my predecessor is about to quit
        Engine.debug("Receiving a 'Predecessor Leaving' message from %s", message.issuer_host_name);
        // modify my predecessor
        setPredecessor(message.request_id);
        /*TODO :
          >> notify my new predecessor
          >> send a notify_predecessors !!
         */
        break;

      case SUCCESSOR_LEAVING:
        // my successor is about to quit
        Engine.debug("Receiving a 'Successor Leaving' message from %s", message.issuer_host_name);
        // modify my successor FIXME : this should be implicit ?
        setFinger(0, message.request_id);
        /* TODO
           >> notify my new successor
           >> update my table & predecessors table */
        break;

      case PREDECESSOR_ALIVE:
        Engine.debug("Receiving a 'Predecessor Alive' request from %s", message.issuer_host_name);
        message.type = MessageType.PREDECESSOR_ALIVE_ANSWER;
        Engine.debug("Sending back a 'Predecessor Alive Answer' to %s (mailbox %s)", message.issuer_host_name,
                     message.answer_to.get_name());
        message.answer_to.put_init(message, 10).detach();
        break;

      default:
        Engine.debug("Ignoring unexpected message: %s from %s", message.type, message.issuer_host_name);
    }
  }

  public void run() throws SimgridException
  {
    this.sleep_for(start_time_);
    if (known_id_ == -1) {
      setPredecessor(-1); // -1 means that I have no predecessor
      printFingerTable();
      joined_ = true;
    } else {
      join(known_id_);
    }

    if (!joined_)
      return;

    double now                         = Engine.get_clock();
    double next_stabilize_date         = start_time_ + dht_chord.PERIODIC_STABILIZE_DELAY;
    double next_fix_fingers_date       = start_time_ + dht_chord.PERIODIC_FIX_FINGERS_DELAY;
    double next_check_predecessor_date = start_time_ + dht_chord.PERIODIC_CHECK_PREDECESSOR_DELAY;
    double next_lookup_date            = start_time_ + dht_chord.PERIODIC_LOOKUP_DELAY;
    Comm comm_receive                  = null;
    while (now < Math.min(start_time_ + deadline_, dht_chord.MAX_SIMULATION_TIME)) {
      if (comm_receive == null)
        comm_receive = mailbox_.get_async();

      if (comm_receive.test()) {
        ChordMessage message = (ChordMessage)comm_receive.get_payload();
        handleMessage(message);
        comm_receive = null;
      } else {
        // no task was received: make some periodic calls
        if (now >= next_stabilize_date) {
          stabilize();
          next_stabilize_date = Engine.get_clock() + dht_chord.PERIODIC_STABILIZE_DELAY;
        } else if (now >= next_fix_fingers_date) {
          fixFingers();
          next_fix_fingers_date = Engine.get_clock() + dht_chord.PERIODIC_FIX_FINGERS_DELAY;
        } else if (now >= next_check_predecessor_date) {
          checkPredecessor();
          next_check_predecessor_date = Engine.get_clock() + dht_chord.PERIODIC_CHECK_PREDECESSOR_DELAY;
        } else if (now >= next_lookup_date) {
          randomLookup();
          next_lookup_date = Engine.get_clock() + dht_chord.PERIODIC_LOOKUP_DELAY;
        } else {
          // nothing to do: sleep for a while
          this.sleep_for(dht_chord.SLEEP_DELAY);
        }
      }

      now = Engine.get_clock();
    }
    if (comm_receive != null) {
      if (!comm_receive.test())
        comm_receive.cancel();
    }
    // leave the ring
    leave();
  }
}

/* ------------------------------------------------------------------------ */
/* The main() function                                                      */
/* ------------------------------------------------------------------------ */

public class dht_chord {
  static final double MAX_SIMULATION_TIME              = 1000;
  static final double PERIODIC_STABILIZE_DELAY         = 20;
  static final double PERIODIC_FIX_FINGERS_DELAY       = 120;
  static final double PERIODIC_CHECK_PREDECESSOR_DELAY = 120;
  static final double PERIODIC_LOOKUP_DELAY            = 10;
  static final double SLEEP_DELAY                      = 4.9999;

  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    args     = e.get_args(); // Get ride of the log extra parameters
    if (args.length < 2)
      Engine.die("Usage: dht_chord [-nb_bits=n] [-timeout=t] platform_file deployment_file\n"
                 + "\tExample: ../platforms/cluster_backbone.xml ./dht_chord_d.xml\n");

    String platform_file   = "";
    String deployment_file = "";
    int nb_bits            = 24;
    int timeout            = 50;
    for (int i = 0; i < args.length - 1; i++) {
      String option = args[i];
      if (option.startsWith("-nb_bits=")) {
        nb_bits = Integer.parseInt(option.substring(option.indexOf('=') + 1));
        Engine.debug("Set nb_bits to %d", nb_bits);
      } else if (option.startsWith("-timeout=")) {
        timeout = Integer.parseInt(option.substring(option.indexOf('=') + 1));
        Engine.debug("Set timeout to %d", timeout);
      } else if (option.startsWith("-")) {
        Engine.die("Invalid chord option '%s'", option);
      } else {
        platform_file   = args[i];
        deployment_file = args[i + 1];
      }
    }
    int nb_keys = 1 << nb_bits;
    Engine.debug("Sets nb_keys to %d", nb_keys);

    e.load_platform(platform_file);

    /* Global initialization of the Chord simulation. */
    Node.set_parameters(nb_bits, nb_keys, timeout);

    e.load_deployment(deployment_file);

    e.run();

    Engine.info("Simulated time: %g", Engine.get_clock());

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
